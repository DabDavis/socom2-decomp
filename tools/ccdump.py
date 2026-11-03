#!/usr/bin/env python3
"""
ccdump -- cc1's own RTL dumps of a GCC unit, compiled as sccheck builds it: the unit's `sc:`
toolchain and flags, its include path. For a miss that is "registers only" or "store order",
where reading the compiler's decisions beats guessing at source shapes (README "The GCC front":
round 14c's three matches -- the allocation order, the first scheduling pass's register weight).

    ./tools/ccdump.py src/lib/asr/gender.c                     # -dl -dg -dS -dR -fsched-verbose=4
    ./tools/ccdump.py src/lib/asr/gender.c -f GenderDataCompute  # and that function's sections
    ./tools/ccdump.py PROBE.c --unit src/lib/asr/pitchdp.c     # a probe, under that unit's directive
    ./tools/ccdump.py UNIT.c --dump "-dc -dl -dg" --flags "-O2 -G0 -fno-gcse"

Writes into build/scratch/gcc/dumps/<stem>/ (or --out): the object, the dumps cc1 names
`<src>.NN.<pass>` (2.96; the 2.9 builds `<src>.<pass>`) -- .lreg (local-alloc), .greg (global-alloc: its `N regs to allocate:` line is
the priority order, refs x log2(refs) / live length), .sched (haifa-sched before reload; with
-fsched-verbose=4 each cycle's ready list, so INSN_REG_WEIGHT's tie-breaks show), .sched2 (after
reload; -dR, else its verbose output goes to stderr) -- and the compiler's stderr. (The 2.9 builds'
cc1 spells the verbosity `-fsched-verbose-4`: the `=` form is retried so when refused.) With -f, each
dump's `;; Function NAME` section is written apart as `<dump>.NAME`, and the allocation order and
the hard register each pseudo got (.greg's `Register dispositions`, named) are printed.

Metrowerks writes no such dumps: an MW unit (or an assembler unit) is refused.
"""

from __future__ import annotations

import argparse
import os
import re
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(HERE))
import scdiff as D
from sccheck import INCLUDE, SRC, tu_options

DEFAULT_DUMPS = "-dl -dg -dS -dR -fsched-verbose=4"


def sections(text: str) -> dict[str, str]:
    """A dump's `;; Function NAME` sections by function name."""
    out = {}
    parts = re.split(r"^;; Function (\S+)\s*$", text, flags=re.M)
    for name, body in zip(parts[1::2], parts[2::2]):
        out[name] = out.get(name, "") + f";; Function {name}\n" + body
    return out


GPR = ("zero at v0 v1 a0 a1 a2 a3 t0 t1 t2 t3 t4 t5 t6 t7 s0 s1 s2 s3 s4 s5 s6 s7 t8 t9 k0 k1 gp sp "
       "fp ra").split()     # objdump's (O32) names, as scd.py and the differ print them


def reg_name(n: int) -> str:
    return GPR[n] if n < 32 else f"$f{n - 32}" if n < 64 else f"hard{n}"


def allocation(greg: str) -> list[str]:
    """Who got which register: global-alloc's priority order (`N regs to allocate: ...`) and the
    final `;; Register dispositions:` (pseudo in hard register, named)."""
    lines = [l for l in greg.splitlines() if "regs to allocate" in l]
    m = re.search(r";; Register dispositions:\n(.*?)\n\n", greg, re.S)
    if m:
        pairs = re.findall(r"(\d+) in (\d+)", m.group(1))
        lines.append(";; dispositions: " + "  ".join(f"{p}={reg_name(int(h))}" for p, h in pairs))
    return lines


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("src", type=Path, help="a GCC unit (src/**/*.c) or a probe")
    ap.add_argument("-f", "--function", action="append", default=[], help="write and summarise this "
                    "function's sections (repeatable)")
    ap.add_argument("--unit", type=Path, help="take the directive from this unit instead (a probe's)")
    ap.add_argument("--compiler", help="override the directive's toolchain")
    ap.add_argument("--flags", help="override the directive's flags (quoted)")
    ap.add_argument("--dump", default=DEFAULT_DUMPS, help=f"the dump flags (default: {DEFAULT_DUMPS})")
    ap.add_argument("--out", type=Path, help="where the dumps go (default build/scratch/gcc/dumps/<stem>)")
    ap.add_argument("extra", nargs="*", help="more cc1 flags after the directive's")
    a = ap.parse_args(argv)
    src = a.src.resolve()
    if src.suffix not in (".c", ".cpp"):
        raise SystemExit(f"{src}: not C (an assembler unit has no RTL)")
    opts = tu_options((a.unit or src).resolve())
    compiler = a.compiler or opts.get("compiler", D.COMPILER)
    if not D.is_gcc(compiler):
        raise SystemExit(f"{src}: {compiler} is Metrowerks, which writes no RTL dumps (name an ee-gcc "
                         "toolchain with --compiler, or a GCC unit with --unit)")
    flags = (a.flags if a.flags is not None else opts.get("flags", " ".join(D.GCC_FLAGS))).split()
    out = (a.out or HERE / "build" / "scratch" / "gcc" / "dumps" / src.stem).resolve()
    out.mkdir(parents=True, exist_ok=True)
    for old in out.glob(src.name + ".*"):
        old.unlink()                          # a stale pass from an earlier run must not be read
    tmp = HERE / "build" / "tmp"
    tmp.mkdir(parents=True, exist_ok=True)
    env = dict(os.environ, TMPDIR=str(tmp), TMP=str(tmp), TEMP=str(tmp))
    obj = out / (src.stem + ".o")
    # the include path sccheck gives a unit (include/, src/), the source's own directory first as
    # GCC's quoted includes have it; the dumps are written into the compiler's cwd
    cmd = D.gcc_driver(compiler) + ["-c"] + flags + a.dump.split() + a.extra + [
        "-I" + str(INCLUDE), "-I" + str(SRC), str(src), "-o", str(obj)]
    r = subprocess.run(cmd, cwd=out, env=env, capture_output=True, text=True, timeout=300)
    if r.returncode != 0 and "Invalid option `-fsched-verbose=" in r.stderr:
        # the 2.9-991111 builds' cc1 spells it -fsched-verbose-N (2.96's -fsched-verbose=N)
        cmd = [re.sub(r"^-fsched-verbose=", "-fsched-verbose-", x) for x in cmd]
        r = subprocess.run(cmd, cwd=out, env=env, capture_output=True, text=True, timeout=300)
    (out / (src.stem + ".stderr")).write_text(r.stderr)
    if r.returncode != 0:
        print(r.stdout + r.stderr[-3000:])
        print(f"compile failed: {' '.join(cmd)}")
        return 1
    # 2.96 numbers its passes (`gender.c.20.greg`), the 2.9 builds do not (`sifcmd.c.greg`)
    dumps = sorted(p for p in out.iterdir() if p.name.startswith(src.name + ".")
                   and re.fullmatch(r"(\d+\.)?[a-z]\w*", p.name[len(src.name) + 1:]))
    print(f"{compiler} {' '.join(flags + a.dump.split() + a.extra)}")
    print(f"{out}: " + " ".join(p.name[len(src.name) + 1:] for p in dumps)
          + (f" (and {src.stem}.stderr, {len(r.stderr)} bytes)" if r.stderr else ""))
    for fn in a.function:
        text = {p: sections(p.read_text(errors="replace")) for p in dumps}
        found = False
        for p, secs in text.items():
            if fn in secs:
                found = True
                (out / f"{p.name}.{fn}").write_text(secs[fn])
        if not found:
            print(f"{fn}: in no dump (a static GCC inlined everywhere, or a name the unit does not define)")
            continue
        greg = next((s[fn] for p, s in text.items() if p.name.endswith(".greg") and fn in s), "")
        print(f"\n--- {fn}: " + ", ".join(f"{p.name}.{fn}" for p, s in text.items() if fn in s))
        for line in allocation(greg):
            print(line)
    return 0


if __name__ == "__main__":
    sys.exit(main())
