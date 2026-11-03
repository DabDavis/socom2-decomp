#!/usr/bin/env python3
"""
Sweep one source over toolchains and flag sets, scoring every function it defines
against the map -- compiler identification for the GCC front (EQOA's eqsweep.py).

    ./tools/ccsweep.py SRC.c                          # every ee-gcc toolchain, -O2 -G0
    ./tools/ccsweep.py SRC.c -t ee-gcc2.96 -t ee-gcc2.9-991111 \\
                       -F "-O2 -G0" -F "-O1 -G0" -F "-O2 -G8"
    ./tools/ccsweep.py SRC.c -f '^sceDma' --diff      # a regex over the functions, with diffs
    ./tools/ccsweep.py src/lib/sdk/intr.c --unit      # the unit as it stands (its directive's
                                                      # toolchain and flags), one function a line
    ./tools/ccsweep.py VARIANTS.c --variants          # source shapes: each `//== name` line
                                                      # starts a variant, the text above the
                                                      # first one is every variant's prelude

A function scores as the number of instruction words that differ (masked by the object's own
relocations), `=` when byte-exact with every relocation resolved (scdiff.compare). A function
the object does not define, or a build that fails, is reported apart: neither is evidence
about the source. Builds go to build/ccsweep/<pid>/.
"""

from __future__ import annotations

import argparse
import os
import re
import sys
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

HERE = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(HERE))
import scdecomp as E
import scdiff as D


def toolchains() -> list[str]:
    return sorted(p.name for p in D.TOOLCHAINS.iterdir() if p.name.startswith("ee-gcc")
                  and any((p / x).is_file() for x in ("bin/ee-gcc", "bin/ee-gcc.exe")))


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("src", type=Path)
    ap.add_argument("-t", "--toolchain", action="append", help="toolchain (repeatable; default every ee-gcc*)")
    ap.add_argument("-F", "--flags", action="append", help='a flag set, quoted (repeatable; default "-O2 -G0")')
    ap.add_argument("-f", "--filter", help="regex over the functions scored")
    ap.add_argument("-I", dest="includes", action="append", type=Path, default=[])
    ap.add_argument("--diff", action="store_true", help="print the diff of each function's best miss")
    ap.add_argument("--variants", action="store_true", help="SRC holds `//== name` variants")
    ap.add_argument("--unit", action="store_true",
                    help="one unit as it stands: its own directive's toolchain and flags (unless -t/-F), "
                         "one function per line")
    ap.add_argument("--stem", help="qualify the object's statics as unit STEM's (`NAME@STEM` rows: "
                                   "a variants file or a probe scored as the unit it copies)")
    ap.add_argument("-j", "--jobs", type=int, default=2)
    a = ap.parse_args(argv)
    if a.unit:
        from sccheck import tu_options
        opts = tu_options(a.src)
        a.toolchain = a.toolchain or [opts.get("compiler", D.COMPILER)]
        a.flags = a.flags or [opts.get("flags", " ".join(D.GCC_FLAGS if D.is_gcc(a.toolchain[0]) else D.FLAGS))]
    tcs = a.toolchain or toolchains()
    flagsets = a.flags or ["-O2 -G0"]
    img, st = E.image(), E.symbols()
    rx = re.compile(a.filter) if a.filter else None
    work = HERE / "build" / "ccsweep" / str(os.getpid())
    work.mkdir(parents=True, exist_ok=True)
    includes = [HERE / "include", HERE / "src", a.src.resolve().parent] + a.includes
    srcs = {"": a.src.resolve()}
    if a.variants:
        parts = re.split(r"^//==\s*(\S+).*$", a.src.read_text(), flags=re.M)
        srcs = {}
        for name, body in zip(parts[1::2], parts[2::2]):
            p = work / f"v_{name}{a.src.suffix}"
            p.write_text(parts[0] + body)
            srcs[name] = p

    def one(job):
        k, (tc, fl, v) = job
        obj = work / f"{k}.o"
        label = f"{v} " * bool(v) + f"{tc} {fl}"
        try:
            D.compile_obj(srcs[v], obj, tc, fl.split(), includes)
            D.qualify_statics(obj, a.stem or a.src.stem, st)  # the unit's statics, as sccheck scores them
        except Exception as e:                      # a failed build is not evidence
            return label, None, (str(e).strip().splitlines() or ["?"])[-1][:120]
        out = {}
        for f in E.Elf.read(obj).functions():
            if rx and not rx.search(f.name):
                continue
            try:
                sym = st.get(f.name)
            except KeyError:
                out[f.name] = ("no-target", None)
                continue
            res = D.compare(obj, f.name, sym, img, st)
            out[f.name] = ("=" if res.match else
                           ("R" if res.masked_equal else (res.differing if res.cand_size == res.target_size
                                                          else f"{res.differing}s")), res)
        return label, out, ""

    jobs = list(enumerate((tc, fl, v) for v in srcs for tc in tcs for fl in flagsets))
    with ThreadPoolExecutor(a.jobs) as ex:
        results = list(ex.map(one, jobs))
    names = sorted({n for _, out, _ in results if out for n in out},
                   key=lambda n: st.get(n).addr if n in st._by_name else 0)
    if a.unit and len(results) == 1:
        label, out, err = results[0]
        if out is None:
            print(f"{label}: BUILD FAILED: {err}")
            return 1
        print(label)
        for n in names:
            v, res = out[n]
            print(f"  {str(v):>9}  {n}")
        print(f"{sum(1 for n in names if out[n][0] == '=')}/{len(names)} byte-exact")
        if a.diff:
            for n in names:
                res = out[n][1]
                if res is not None and not res.match:
                    print(f"\n{D.report(res)}")
        return 0
    w = max([len(label) for label, _, _ in results] + [10])
    print(" " * w + "  " + "  ".join(f"{n[:14]:>14}" for n in names))
    best: dict[str, tuple] = {}
    for label, out, err in results:
        if out is None:
            print(f"{label:<{w}}  BUILD FAILED: {err}")
            continue
        cells = []
        for n in names:
            v, res = out.get(n, ("-", None))
            cells.append(f"{str(v):>14}")
            if res is not None and not res.match:
                score = res.differing + (1000 if res.cand_size != res.target_size else 0)
                if n not in best or score < best[n][0]:
                    best[n] = (score, label, res)
        print(f"{label:<{w}}  " + "  ".join(cells))
    print("\n= byte-exact, R right bytes but a wrong reference, N words differ, Ns size differs too")
    if a.diff:
        for n, (_, label, res) in best.items():
            print(f"\n--- {n}: best miss {label}")
            print(D.report(res))
    return 0


if __name__ == "__main__":
    sys.exit(main())
