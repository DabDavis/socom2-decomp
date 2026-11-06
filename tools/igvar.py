#!/usr/bin/env python3
"""
igvar -- source variants compared by Metrowerks' own colouring: each variant built under
tools/igdump.py in a scratch tree, and per variant the colouring of the functions named -- who
holds each saved register, what was spilled -- in ~1-4 s a variant. Which numbering a register-only
miss needs is read here (with tools/igreplay.py's --move before any source is written); whether
the bytes then match is tools/try.py's question.

    ./tools/igvar.py src/ai/ai_maps.cpp WalkLine --variants build/scratch/ai/v_walk.py
    ./tools/igvar.py src/Apps/FTS/graph.cpp '^Read$' --sub 'int i, j;' 'int j, i;'
    ./tools/igvar.py include/vector insert --unit src/gamez/zui/ui_list.cpp --variants v.py --all
    ./tools/igvar.py src/gamez/zseal/seal_anim.cpp CanZoom --cls 1 --variants v.py   # the FPRs

FILE, --unit, --sub/--and, --variants and -n are tools/try.py's (the same parser: a variant is a --sub
and the --ands after it, or one of --variants FILE.py's `SUBS = {name: [(old, new), ...]}` or `OLD` and
`V = {name: new}`; --sub/--and replace OLD's first occurrence, --sub-all/--and-all every one, GCC round
42a); the unit is built as sccheck builds it (its directive's flags; include/ and src/
of a scratch copy -- build/igvar/, or $SC_IG_WORK, one per concurrent user -- so the tree is never
touched). PROC is a regex on the function's own name (igdump's `proc`, unmangled). Each variant
prints, per colouring of PROC in class --cls (0 GPR, 1 FPR), the spilled nodes and each saved
register's holders by variable (the AI core's round-28 igvar.py), `(as it is)` when a variant gives
the unchanged source's; --all lists every telling node instead (igdump's show: the game states'
round-37 igvar.py), --keep keeps each variant's dump (build/igvar/<label>.jsonl) for igreplay.
"""

from __future__ import annotations

import argparse
import importlib
import os
import re
import shutil
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(HERE))
sys.path.insert(0, str(HERE / "tools"))
import igdump
import sccheck

trymod = importlib.import_module("try")   # tools/try.py: its variant parser (`try` is a keyword)

SRC, INCLUDE = HERE / "src", HERE / "include"
WORK = Path(os.environ["SC_IG_WORK"]).resolve() if os.environ.get("SC_IG_WORK") else HERE / "build" / "igvar"


def in_tree(path: Path, work: Path = WORK) -> Path:
    """`path` (under src/ or include/) as the scratch tree's copy of it; a probe outside both is
    copied in as src/__probe__/<name> (its own directory's headers are not)."""
    p = path.resolve()
    for real, name in ((SRC, "src"), (INCLUDE, "include")):
        try:
            return work / name / p.relative_to(real)
        except ValueError:
            pass
    return work / "src" / "__probe__" / p.name


def prepare(work: Path = WORK, probes=()) -> None:
    """The scratch tree: src/ without the drafts, and include/ -- copied once per run -- and any
    probe outside them."""
    if work.exists():
        shutil.rmtree(work)
    shutil.copytree(SRC, work / "src", ignore=shutil.ignore_patterns("auto", "*.o"))
    shutil.copytree(INCLUDE, work / "include")
    for p in probes:
        dst = in_tree(Path(p), work)
        if not dst.exists():
            dst.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy(p, dst)


def dump_variant(file: Path, unit: Path, text: str | None, out: Path,
                 work: Path = WORK) -> tuple[list[dict] | None, str]:
    """igdump of `unit` in the scratch tree with `file`'s text replaced by `text` (None: as it is);
    `file` is put back afterwards. (records, "") or (None, why)."""
    wfile, wunit = in_tree(file, work), in_tree(unit, work)
    orig = wfile.read_bytes()
    try:
        if text is not None:
            wfile.write_text(text)
        bad = sccheck.directive_problem(wunit)
        if bad:
            return None, bad
        recs, _ = igdump.dump(wunit, out, includes=[work / "include", work / "src"])
        return recs, ""
    except RuntimeError as e:
        return None, str(e).replace(str(work) + "/", "")
    finally:
        wfile.write_bytes(orig)


def summary(recs: list[dict], proc: str, cls: int = 0, every: bool = False) -> list[str]:
    """Per colouring of `proc`: the spilled nodes and the saved registers' holders (or, `every`,
    igdump's show of each)."""
    out = []
    for k, r in igdump.select(recs, proc, cls):
        if every:
            out += igdump.show(k, r)
        else:
            sp, own = igdump.saved(r)
            out.append(f"[{k}] {igdump.proc_name(r)}: spilled {' '.join(sp) or '-'}; "
                       + (" ".join(f"{reg}={'/'.join(v)}" for reg, v in own.items()) or "no saved register"))
    return out or [f"no colouring of {proc!r} in class {cls}"]


def run(file: Path, unit: Path, proc: str, variants: list[tuple[str, list[tuple[str, str]]]],
        cls: int = 0, every: bool = False, keep: bool = False, work: Path = WORK) -> list[tuple[str, list[str]]]:
    """The unchanged source, then each variant: [(label, summary lines)]."""
    base = file.read_text()
    prepare(work, probes=(file, unit))
    results = []
    first = None
    for label, subs in [("(as it is)", [])] + [v for v in variants if v[1]]:
        text, missing, _ = trymod.apply_subs(base, subs)     # a pair its OLD's first occurrence, 'all' every one
        if missing is not None:
            results.append((label, [f"(not in {file.name}: {missing[:60]!r})"]))
            continue
        tag = re.sub(r"[^A-Za-z0-9_.-]+", "_", label)[:40] or "v"
        out = work / f"{tag}.{os.getpid()}.jsonl"
        recs, err = dump_variant(file, unit, text if subs else None, out, work)
        if keep and recs is not None:
            shutil.copy(out, work / f"{tag}.jsonl")
        out.unlink(missing_ok=True)
        if recs is None:
            results.append((label, ["BUILD-FAIL " + err.strip().splitlines()[-1][:140]]))
            continue
        lines = summary(recs, proc, cls, every)
        if first is None:
            first = lines
        elif lines == first:
            lines = ["(as it is)"]
        results.append((label, lines))
    return results


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("file", type=Path)
    ap.add_argument("proc", help="a regex on the function's own (unmangled) name")
    ap.add_argument("--unit", type=Path)
    trymod.add_sub_options(ap)          # --sub/--and (OLD's first occurrence), --sub-all/--and-all (every one)
    ap.add_argument("--variants", type=Path)
    ap.add_argument("-n", "--newlines", action="store_true", help="`\\n` in OLD and NEW is a newline, `\\t` a tab")
    ap.add_argument("--cls", type=int, default=0, help="register class: 0 GPR (default), 1 FPR")
    ap.add_argument("--all", action="store_true", help="every telling node of each colouring, not the summary")
    ap.add_argument("--keep", action="store_true", help="keep each variant's dump (build/igvar/<label>.jsonl)")
    a = ap.parse_args(argv)
    unit = a.unit or a.file
    variants = trymod.variants_from(a, a.file.read_text())
    for label, lines in run(a.file, unit, a.proc, variants, a.cls, a.all, a.keep):
        if len(lines) == 1:
            print(f"{label[:40]:40} {lines[0]}")
        else:
            print(f"{label[:40]}")
            for ln in lines:
                print(f"    {ln}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
