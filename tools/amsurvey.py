#!/usr/bin/env python3
"""
Measure tools/automatch.py on a fixed sample and keep what each miss came closest to.

    ./tools/amsurvey.py sample build/scratch/am/sample.txt          # write the sample: every
                                                                    # unmatched candidate <= 0x100
    ./tools/amsurvey.py run build/scratch/am/sample.txt base --jobs 3   # -> build/scratch/am/base/
    ./tools/amsurvey.py near base                                    # the near misses, by stage
    ./tools/amsurvey.py cmp base fix1                                # gained / lost between runs

`run` drives automatch's own attempt() (nothing about it changes), recording every draft it
compiles: the stage (triple, with or without callee prototypes, which variant, C++ bool or
member/virtual redraft), the verdict, the candidate's size and how many words differ. For each
miss the closest draft -- the right size and the fewest differing words -- is kept with its
diff under <run>/near/, so a near miss can be classified by cause from the data. results.jsonl
holds one row per function; hits.txt the addresses that matched.
"""

from __future__ import annotations

import argparse
import re
import json
import sys
import time
from concurrent.futures import ProcessPoolExecutor
from pathlib import Path

HERE = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(HERE))
sys.path.insert(0, str(HERE / "tools"))
import scdecomp as E
import scdiff as D
import automatch as A

SCRATCH = HERE / "build" / "scratch" / "am"

_stage = {"triple": "", "ctx": False, "kind": "c", "n": 0, "prefix": ""}
_log: list[dict] = []
_best: dict = {}

_run_m2c, _try_draft, _bool_draft, _cxx_draft = A.run_m2c, A.try_draft, A.bool_draft, A.cxx_draft


def run_m2c(triple, asm, ctx=None):
    _stage.update(triple=triple.split("-")[1], ctx=ctx is not None)
    out = _run_m2c(triple, asm, ctx)
    if out is None:
        _log.append({"stage": f"{_stage['triple']}{'+ctx' if ctx else ''}", "verdict": "m2c"})
    return out


def try_draft(*a, **k):
    _stage.update(kind="c", n=0, prefix="")
    return _try_draft(*a, **k)


_repair = getattr(A, "repair", None)


def repair(*a, **k):
    _stage.update(kind="c", n=0, prefix="r")
    return _repair(*a, **k)


def bool_draft(*a, **k):
    _stage["kind"] = "bool"
    return _bool_draft(*a, **k)


def cxx_draft(*a, **k):
    _stage["kind"] = "cxx"
    return _cxx_draft(*a, **k)


def try_one(tmp, src, sym, real, addr, img, st, suffix=".c"):
    """automatch.try_one, keeping the comparison's details."""
    kind = _stage["kind"]
    if kind == "c":
        kind = f"{_stage['prefix']}v{_stage['n']}"
        _stage["n"] += 1
    label = f"{_stage['triple']}{'+ctx' if _stage['ctx'] else ''}:{kind}"
    (tmp / f"f{suffix}").write_text(src)
    rec = {"stage": label}
    try:
        D.compile_obj(tmp / f"f{suffix}", tmp / "f.o")
        if real != sym.name:
            import sccheck
            sccheck.rename_symbol(tmp / "f.o", sym.name, real)
        res = D.compare(tmp / "f.o", real, st.at_addr(addr), img, st)
    except Exception as e:                      # noqa: BLE001 -- the generator's own rule
        rec.update(verdict="build", err=first_error(str(e)))
        _log.append(rec)
        if "err_src" not in _best:
            _best.update(err_src=src, err_suffix=suffix, err=str(e)[-2000:])
        return "build"
    rec.update(verdict="hit" if res.match else "miss", size=res.cand_size, differing=res.differing,
               masked_equal=res.masked_equal, reloc_bad=len(res.reloc_bad), padding_ok=res.padding_ok)
    _log.append(rec)
    if not res.match and res.cand_size == res.target_size:
        score = res.differing if not res.masked_equal else 0
        if "score" not in _best or score < _best["score"]:
            _best.update(score=score, src=src, suffix=suffix, stage=label, report=D.report(res))
    return "hit" if res.match else "miss"


A.run_m2c, A.try_draft, A.bool_draft, A.cxx_draft, A.try_one = run_m2c, try_draft, bool_draft, cxx_draft, try_one
if _repair is not None:
    A.repair = repair


def first_error(msg: str) -> str:
    """MW's first diagnostic, its continuation lines joined (`f.c:24: illegal implicit
    conversion from 'int'` / `to` / `'int *'`), the line number dropped."""
    import re
    rows = [m.groups() for m in re.finditer(r"^.*?\bf\.c(?:pp)?:(\d+): (.*)$", msg, re.M)]
    if not rows:
        return msg.strip().splitlines()[-1][:300] if msg.strip() else ""
    line = rows[0][0]
    return " ".join(t.strip() for n, t in rows if n == line)[:300]


def work(addr: int):
    _log.clear()
    _best.clear()
    t = time.time()
    try:
        a, kind, src = A.attempt(addr)
    except Exception as e:                      # noqa: BLE001
        kind, src = f"crash: {e}", None
    return addr, kind, src, list(_log), dict(_best), time.time() - t


def cmd_sample(a) -> int:
    img, st = E.image(), E.symbols()
    hand = A.hand_defined()
    have = {int(f.stem, 16) for f in A.auto_files()}
    cands = [s.addr for s in st.symbols
             if img.is_code(s.addr) and a.min_size < s.size <= a.max_size
             and img.segment(s.addr).name != "MAIN" and s.addr % 16 == 0
             and s.name not in hand and s.addr not in have]
    a.out.parent.mkdir(parents=True, exist_ok=True)
    a.out.write_text("".join(f"{x:08X}\n" for x in sorted(cands)))
    print(f"{len(cands)} unmatched candidates ({a.min_size:#x}, {a.max_size:#x}] -> {a.out}")
    return 0


def cmd_run(a) -> int:
    img, st = E.image(), E.symbols()
    addrs = [int(x, 16) for x in a.sample.read_text().split()]
    out = SCRATCH / a.name
    (out / "near").mkdir(parents=True, exist_ok=True)
    (out / "hits").mkdir(parents=True, exist_ok=True)
    tally: dict[str, int] = {}
    t0 = time.time()
    with open(out / "results.jsonl", "w") as res, ProcessPoolExecutor(a.jobs) as ex:
        for i, (addr, kind, src, log, best, dt) in enumerate(ex.map(work, addrs, chunksize=4)):
            s = st.at_addr(addr)
            tally[kind] = tally.get(kind, 0) + 1
            row = {"addr": f"{addr:08X}", "name": s.name, "size": s.size, "seg": img.segment(addr).name,
                   "outcome": kind, "secs": round(dt, 2), "drafts": log}
            if src:
                ext = ".cpp" if "/* sc: c++ */" in src[:200] else ".c"
                (out / "hits" / f"{addr:08X}{ext}").write_text(src)
            if "score" in best:
                row["near"] = {"stage": best["stage"], "differing": best["score"]}
                (out / "near" / f"{addr:08X}{best['suffix']}").write_text(best["src"])
                (out / "near" / f"{addr:08X}.diff").write_text(best["report"] + "\n")
            elif "err" in best and kind != "hit":
                (out / "near" / f"{addr:08X}.err{best['err_suffix']}").write_text(best["err_src"])
                (out / "near" / f"{addr:08X}.err").write_text(best["err"] + "\n")
            res.write(json.dumps(row) + "\n")
            if (i + 1) % 200 == 0:
                print(f"  {i + 1}/{len(addrs)} {time.time() - t0:.0f}s {tally}", flush=True)
    hits = sorted(k["addr"] for k in (json.loads(l) for l in open(out / "results.jsonl")) if k["outcome"] == "hit")
    (out / "hits.txt").write_text("".join(f"{h}\n" for h in hits))
    total = sum(tally.values())
    import os
    toggles = " ".join(f"{k}={v}" for k, v in sorted(os.environ.items()) if k.startswith("SC_AM_"))
    line = f"{a.name} [{a.sample.name}{' ' + toggles if toggles else ''}]: {total} tried: " + \
           ", ".join(f"{k} {v}" for k, v in sorted(tally.items())) + f"  ({time.time() - t0:.0f}s)"
    print(line)
    with open(SCRATCH / "runs.txt", "a") as f:
        f.write(line + "\n")
    return 0


def load(name: str) -> dict[str, dict]:
    return {r["addr"]: r for r in (json.loads(l) for l in open(SCRATCH / name / "results.jsonl"))}


def cmd_near(a) -> int:
    rows = load(a.name)
    near = sorted((r for r in rows.values() if "near" in r and r["outcome"] != "hit"),
                  key=lambda r: (r["near"]["differing"], r["addr"]))
    hist: dict[int, int] = {}
    for r in near:
        d = r["near"]["differing"]
        hist[min(d, 10)] = hist.get(min(d, 10), 0) + 1
    print(f"{len(near)} misses with a draft of the right size; by differing words: "
          + ", ".join(f"{'>=' if k == 10 else ''}{k}: {v}" for k, v in sorted(hist.items())))
    for r in near[: a.show]:
        print(f"{r['addr']} {r['size']:#5x} {r['near']['differing']:3d} {r['near']['stage']:<14} {r['name']}")
    return 0


def _hunks(diff: str):
    """The removed (target) and added (candidate) instruction runs of a saved diff, paired."""
    runs, minus, plus = [], [], []
    for l in diff.splitlines():
        if l.startswith(("---", "+++", "@@")):
            continue
        if l.startswith("-"):
            minus.append(l[1:].split(None, 1))
        elif l.startswith("+"):
            plus.append(l[1:].split(None, 1))
        elif minus or plus:
            runs.append((minus, plus))
            minus, plus = [], []
    if minus or plus:
        runs.append((minus, plus))
    return runs


def _mn(x):
    return x[1].split()[0] if len(x) > 1 else "?"


def classify(diff: str) -> str:
    """A coarse cause for one near miss, from its target/candidate instruction runs."""
    runs = _hunks(diff)
    t = [x for m, _ in runs for x in m]
    c = [x for _, p in runs for x in p]
    if not t and not c:
        return "reloc-or-pad"
    tm, cm = [_mn(x) for x in t], [_mn(x) for x in c]
    tt = [x[1] if len(x) > 1 else "" for x in t]
    ct = [x[1] if len(x) > 1 else "" for x in c]
    if any(re.match(r"daddiu (\w+),zero,1$", x) for x in tt) and any(re.match(r"li (\w+),1$", x) for x in ct):
        return "true:daddiu-vs-li"
    if sorted(tt) == sorted(ct):
        return "order"
    if sorted(tm) == sorted(cm) and tm != cm:
        return "order+operands"
    if tm == cm:
        regs = lambda xs: [re.sub(r"(?<![\w$])-?(0x[0-9a-f]+|\d+)", "#", x) for x in xs]
        if regs(tt) == regs(ct):
            sp = all("sp" in x for x, y in zip(tt, ct) if x != y)
            return "immediates:stack" if sp else "immediates"
        if any("at," in x or ",at" in x for x in tt) and not any("at," in x or ",at" in x for x in ct):
            return "regs:$at"
        if any(re.search(r"\$f\d", x) for x, y in zip(tt, ct) if x != y):
            return "regs:fpr"
        return "regs:gpr"
    extra_t = sorted(set(tm) - set(cm))
    extra_c = sorted(set(cm) - set(tm))
    return f"ops:{','.join(extra_t) or '-'}|{','.join(extra_c) or '-'}"


def cmd_classify(a) -> int:
    import collections
    rows = load(a.name)
    near = [r for r in rows.values() if "near" in r and r["outcome"] != "hit"
            and r["near"]["differing"] <= a.max_diff]
    by = collections.defaultdict(list)
    for r in near:
        d = (SCRATCH / a.name / "near" / f"{r['addr']}.diff").read_text()
        by[classify(d)].append(r["addr"])
    for k, v in sorted(by.items(), key=lambda kv: -len(kv[1]))[: a.show]:
        print(f"{len(v):4d}  {k:<40} {' '.join(v[:6])}")
    print(f"{len(near)} near misses (<= {a.max_diff} differing)")
    return 0


def cmd_show(a) -> int:
    d = SCRATCH / a.name / "near"
    for addr in a.addrs:
        addr = addr.upper().removeprefix("0X").zfill(8)
        diff = d / f"{addr}.diff"
        if diff.exists():
            lines = diff.read_text().splitlines()
            print("\n".join(lines[:2] + [l for l in lines[2:] if l[:1] in "-+" and not l.startswith(("---", "+++"))]))
            src = next((p for p in d.glob(f"{addr}.c*")), None)
        else:
            err = d / f"{addr}.err"
            print(f"{addr}: " + (err.read_text().strip() if err.exists() else "no record"))
            src = next((p for p in d.glob(f"{addr}.err.c*")), None)
        if src is not None and not a.diff_only:
            body = src.read_text()
            print(body[body.index("#define NULL 0") + 15:] if "#define NULL 0" in body else body)
        print("=" * 60)
    return 0


def cmd_cmp(a) -> int:
    x, y = load(a.a), load(a.b)
    common = set(x) & set(y)
    hx = {k for k in common if x[k]["outcome"] == "hit"}
    hy = {k for k in common if y[k]["outcome"] == "hit"}
    print(f"{a.a}: {len(hx)} hits, {a.b}: {len(hy)} hits (of {len(common)} in both)")
    print(f"gained {len(hy - hx)}: {' '.join(sorted(hy - hx)[:40])}")
    print(f"lost {len(hx - hy)}: {' '.join(sorted(hx - hy))}")
    return 0


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="cmd", required=True)
    p = sub.add_parser("sample")
    p.add_argument("out", type=Path)
    p.add_argument("--max-size", type=lambda s: int(s, 0), default=0x100)
    p.add_argument("--min-size", type=lambda s: int(s, 0), default=0)
    p = sub.add_parser("run")
    p.add_argument("sample", type=Path)
    p.add_argument("name")
    p.add_argument("--jobs", type=int, default=3)
    p = sub.add_parser("near")
    p.add_argument("name")
    p.add_argument("--show", type=int, default=40)
    p = sub.add_parser("cmp")
    p.add_argument("a")
    p.add_argument("b")
    p = sub.add_parser("classify")
    p.add_argument("name")
    p.add_argument("--max-diff", type=int, default=6)
    p.add_argument("--show", type=int, default=50)
    p = sub.add_parser("show")
    p.add_argument("name")
    p.add_argument("addrs", nargs="+")
    p.add_argument("--diff-only", action="store_true")
    a = ap.parse_args(argv)
    return {"sample": cmd_sample, "run": cmd_run, "near": cmd_near, "cmp": cmd_cmp,
            "classify": cmd_classify, "show": cmd_show}[a.cmd](a)


if __name__ == "__main__":
    sys.exit(main())
