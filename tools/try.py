#!/usr/bin/env python3
"""
Try source variants of one function: each substitution is compiled on its own and scored.

    ./tools/try.py src/gamez/zmath/zmath.h _close_enough<f>__FRCfRCfRCf_b --unit src/gamez/zmath/zmath_main.cpp \\
        --sub 'return !(d < -eps) && d <= eps;' 'return d >= -eps && d <= eps;' \\
        --sub ...
    ./tools/try.py src/gamez/zmath/zmath_pnt3d.cpp Interp__6CPnt3DCFPC6CPnt3DfP6CPnt3D -d   # as it is, with the diff

FILE is the file the substitutions apply to (a header or the unit); the unit compiled is
--unit, or FILE itself. Every variant is built in a scratch copy of src/ (build/try/, or
$SC_TRY_WORK -- one per concurrent user), so the tree is never touched. Prints one line per variant: MATCH, or the differ's verdict.
"""

from __future__ import annotations

import argparse
import shutil
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(HERE))
import scdecomp as E
import scdiff as D

import os

# SC_TRY_WORK gives each concurrent user (an agent) a scratch tree of its own
SRC, INCLUDE = HERE / "src", HERE / "include"
WORK = Path(os.environ["SC_TRY_WORK"]).resolve() if os.environ.get("SC_TRY_WORK") else HERE / "build" / "try"


def score(file: Path, unit: Path, symbol: str, text: str | None, show: bool = False) -> str:
    rel_file, rel_unit = file.resolve().relative_to(SRC), unit.resolve().relative_to(SRC)
    if WORK.exists():
        shutil.rmtree(WORK)
    shutil.copytree(SRC, WORK, ignore=shutil.ignore_patterns("auto", "*.o"))
    if text is not None:
        (WORK / rel_file).write_text(text)
    obj = WORK / rel_unit.with_suffix(".o")
    try:
        D.compile_obj(WORK / rel_unit, obj, includes=[INCLUDE, WORK])
    except RuntimeError as e:
        return "BUILD-FAIL " + str(e).strip().splitlines()[-1][:100]
    img, st = E.image(), E.symbols()
    try:
        r = D.compare(obj, symbol, st.get(symbol), img, st)
    except KeyError as e:
        return f"NO SYMBOL {e}"
    if show and not r.match:
        print(D.report(r))
    return "MATCH" if r.match else r.verdict


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("file", type=Path)
    ap.add_argument("symbol")
    ap.add_argument("--unit", type=Path)
    ap.add_argument("--sub", nargs=2, action="append", default=[], metavar=("OLD", "NEW"))
    ap.add_argument("-d", "--diff", action="store_true", help="print the diff of a variant that fails")
    a = ap.parse_args(argv)
    unit = a.unit or a.file
    base = a.file.read_text()
    if not a.sub:
        print(score(a.file, unit, a.symbol, None, a.diff))
        return 0
    for old, new in a.sub:
        if old not in base:
            print(f"(not in {a.file.name}: {old[:60]!r})")
            continue
        print(f"{score(a.file, unit, a.symbol, base.replace(old, new, 1), a.diff):<60} | {new[:90]}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
