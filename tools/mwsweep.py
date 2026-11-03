#!/usr/bin/env python3
"""MW compiler/flag sweep (ccsweep.py's scoring, extended to the MW builds): the MAIN runtime lane's
probe (rounds 1-2), moved from build/scratch/rt-r1/. A probe tool, not the scorer.

    mwsweep.py SRC [-t TC ...] [-F "flags" ...] [-f REGEX] [--diff] [--all]

Every MW toolchain by default; the unit's own `sc: flags=` (else the game's) when no -F is given. A build that rejects an option is retried with that option
adapted (bottomup/level dropped from -inline for the pre-3.0b38 builds, which do not know
them): the label then says `~` and which flags actually ran. Cells: `=` byte-exact with every
relocation resolved, R right bytes wrong reference, N words differ, Ns size differs too.
"""
from __future__ import annotations
import argparse, os, re, sys
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

HERE = Path(__file__).resolve().parents[1]          # decomp/
sys.path.insert(0, str(HERE))
import scdecomp as E
import scdiff as D

GAME = "-O4,p -sdatathreshold 0 -inline auto,deferred,bottomup,level=5 -enum min -Cpp_exceptions off"


def mw_toolchains() -> list[str]:
    return sorted(p.name for p in D.TOOLCHAINS.iterdir() if p.name.startswith("mwcps2")
                  and any((p / x).is_file() for x in ("mwccps2.exe", "mwccmips.exe")))


def adapt(flags: list[str], err: str) -> list[str] | None:
    """One step of adaptation for an option the build rejects; None when nothing to try."""
    out = list(flags)
    if "-inline" in out:
        i = out.index("-inline")
        parts = out[i + 1].split(",")
        if "bottomup" in err or "level" in err or "Unknown option" in err:
            for bad in ("bottomup",) + tuple(p for p in parts if p.startswith("level")):
                if bad in parts and (bad.split("=")[0] in err or "bottomup" in err and bad == "bottomup"):
                    parts.remove(bad)
                    out[i + 1] = ",".join(parts)
                    return out
            # unknown which: drop bottomup first, then level
            for bad in ("bottomup",) + tuple(p for p in parts if p.startswith("level")):
                if bad in parts:
                    parts.remove(bad)
                    out[i + 1] = ",".join(parts)
                    return out
    return None


def main(argv=None) -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("src", type=Path)
    ap.add_argument("-t", "--toolchain", action="append")
    ap.add_argument("-F", "--flags", action="append")
    ap.add_argument("-f", "--filter")
    ap.add_argument("--diff", action="store_true")
    ap.add_argument("-I", dest="includes", action="append", type=Path, default=[])
    ap.add_argument("-j", "--jobs", type=int, default=2)
    a = ap.parse_args(argv)
    tcs = a.toolchain or mw_toolchains()
    from sccheck import tu_options          # the unit's own `sc: flags=` when no -F is given
    flagsets = a.flags or [tu_options(a.src).get("flags", GAME)]
    img, st = E.image(), E.symbols()
    rx = re.compile(a.filter) if a.filter else None
    work = HERE / "build" / "scratch" / "rt" / "sweep" / str(os.getpid())
    work.mkdir(parents=True, exist_ok=True)
    includes = [a.src.resolve().parent] + a.includes

    def one(job):
        k, (tc, fl) = job
        obj = work / f"{k}.o"
        flags = fl.split()
        tag = ""
        for _ in range(4):
            try:
                D.compile_obj(a.src.resolve(), obj, tc, flags, includes)
                break
            except Exception as e:
                msg = str(e)
                nf = adapt(flags, msg)
                if nf is None:
                    return f"{tc} {fl}", None, (msg.strip().splitlines() or ["?"])[-3:]
                flags, tag = nf, "~"
        else:
            return f"{tc} {fl}", None, ["adapt loop"]
        D.qualify_statics(obj, a.src.stem, st)
        out = {}
        for f in E.Elf.read(obj).functions():
            if rx and not rx.search(f.name):
                continue
            try:
                sym = st.get(f.name)
            except KeyError:
                continue
            res = D.compare(obj, f.name, sym, img, st)
            out[f.name] = ("=" if res.match else
                           ("R" if res.masked_equal else (str(res.differing) if res.cand_size == res.target_size
                                                          else f"{res.differing}s")), res)
        label = f"{tc}{tag}" + (f" [{' '.join(flags)}]" if (len(flagsets) > 1 or tag) else "")
        return label, out, ""

    jobs = list(enumerate((tc, fl) for fl in flagsets for tc in tcs))
    with ThreadPoolExecutor(a.jobs) as ex:
        results = list(ex.map(one, jobs))
    names = sorted({n for _, out, _ in results if out for n in out},
                   key=lambda n: st.get(n).addr)
    short = {n: (n[:16]) for n in names}
    for i, n in enumerate(names):
        print(f"  c{i}: {n} @ {st.get(n).addr:#x}")
    w = max([len(l) for l, _, _ in results] + [10])
    print(" " * w + " " + "".join(f"{'c'+str(i):>6}" for i in range(len(names))) + "   exact")
    best = {}
    for label, out, err in results:
        if out is None:
            print(f"{label:<{w}} FAILED: {' | '.join(err)[:150]}")
            continue
        cells = []
        for n in names:
            v, res = out.get(n, ("-", None))
            cells.append(f"{v:>6}")
            if res is not None and not res.match:
                score = res.differing + (1000 if res.cand_size != res.target_size else 0)
                if n not in best or score < best[n][0]:
                    best[n] = (score, label, res)
        ex = sum(1 for n in names if out.get(n, ("-",))[0] == "=")
        print(f"{label:<{w}} " + "".join(cells) + f"   {ex}/{len(names)}")
    if a.diff:
        for n, (_, label, res) in best.items():
            print(f"\n--- {n}: best miss {label}")
            print(D.report(res))
    return 0


if __name__ == "__main__":
    sys.exit(main())
