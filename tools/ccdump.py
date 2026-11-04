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
the hard register each pseudo got (.greg's `Register dispositions`, named) are printed -- and, since
GCC round 29a (the GCC c lane's round-21c prio.py folded in), each allocno's priority as global.c's
allocno_compare computes it from .lreg's flow info (`Register N used R times across L insns`):
(double)(floor_log2(R) x R) / L x 10000 x size, truncated, equal ones to the lower pseudo; the
calls it crosses (X), its register, `tie` where a neighbour's priority is equal. The order those
priorities give is checked against the order .greg printed, and a disagreement is said, never
hidden (sceCdStRead: `last` and SCE_CD_debug's %hi, pseudos 92 and 94, 909 each -- round 24a's tie).

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


def dispositions(greg: str) -> dict[int, int]:
    """`;; Register dispositions:`: pseudo -> the hard register it got."""
    m = re.search(r";; Register dispositions:\n(.*?)\n\n", greg, re.S)
    return {int(p): int(h) for p, h in re.findall(r"(\d+) in (\d+)", m.group(1))} if m else {}


def allocation(greg: str) -> list[str]:
    """Who got which register: global-alloc's priority order (`N regs to allocate: ...`) and the
    final `;; Register dispositions:` (pseudo in hard register, named)."""
    lines = [l for l in greg.splitlines() if "regs to allocate" in l]
    disp = dispositions(greg)
    if disp:
        lines.append(";; dispositions: " + "  ".join(f"{p}={reg_name(h)}" for p, h in disp.items()))
    return lines


def floor_log2(n: int) -> int:
    return n.bit_length() - 1 if n > 0 else -1


def reg_usage(lreg: str) -> dict[int, dict]:
    """The flow info local-alloc's dump ends with, which global-alloc then reads: each pseudo's
    `Register N used R times across L insns` -- REG_N_REFS, REG_LIVE_LENGTH -- the calls it
    crosses and its size when not a word's (`; 4 bytes`)."""
    out = {}
    for m in re.finditer(r"^Register (\d+) used (\d+) times across (-?\d+) insns([^\n]*)", lreg, re.M):
        rest = m.group(4)
        calls = re.search(r"crosses (\d+) calls?", rest)
        size = re.search(r"; (\d+) bytes", rest)
        out[int(m.group(1))] = {"refs": int(m.group(2)), "live": int(m.group(3)),
                                "calls": int(calls.group(1)) if calls else 0,
                                "bytes": int(size.group(1)) if size else None}
    return out


def to_allocate(greg: str) -> list[tuple[list[int], int]]:
    """global.c's dump_conflicts line, `;; N regs to allocate: 70 63+81 (2) ...`: each allocno in the
    order global-alloc allocates it -- its pseudos (`a+b`: pseudos sharing one allocno, the
    allocno's own last) and its size in hard registers (`(2)`, else 1)."""
    m = re.search(r"regs to allocate:([^\n]*)", greg)
    out: list[tuple[list[int], int]] = []
    for tok in (m.group(1).split() if m else []):
        if tok.startswith("(") and out:
            out[-1] = (out[-1][0], int(tok.strip("()")))
        elif re.fullmatch(r"\d+(\+\d+)*", tok):
            out.append(([int(x) for x in tok.split("+")], 1))
    return out


def priorities(lreg: str, greg: str) -> list[str]:
    """Global-alloc's priority of each allocno it allocates, as global.c's allocno_compare computes
    it: (double)(floor_log2(refs) x refs) / live x 10000 x size, truncated to an int; equal ones go
    to the lower allocno -- the lower first pseudo (the GCC c lane's round-21c prio.py, folded in by
    GCC round 29a). refs sums an allocno's pseudos, live is the longest. The order these give is
    checked against the order .greg printed (a stale or misread dump says so rather than mislead),
    and each pseudo's final register is shown: a saved one for a call-crossing pseudo (X)."""
    use, order, disp = reg_usage(lreg), to_allocate(greg), dispositions(greg)
    if not order:
        return []
    rows = []
    for pseudos, size in order:
        missing = [p for p in pseudos if p not in use]
        if missing:
            return [f";; priorities: pseudo {missing[0]} is not in the .lreg dump's flow info"]
        refs = sum(use[p]["refs"] for p in pseudos)
        live = max(use[p]["live"] for p in pseudos) or -1        # global.c: a length of 0 is -1
        pri = int((float(floor_log2(refs) * refs) / live) * 10000 * size)
        calls = sum(use[p]["calls"] for p in pseudos)
        rows.append((pseudos, size, refs, live, pri, calls))
    want = sorted(rows, key=lambda r: (-r[4], min(r[0])))
    same = [r[0] for r in want] == [r[0] for r in rows]
    lines = [";; global-alloc's priorities, floor_log2(refs) x refs / live x 10000 x size (ties: the lower "
             "pseudo) -- " + ("the order .greg printed" if same else
                              "NOT the order .greg printed (computed: "
                              + " ".join("+".join(map(str, r[0])) for r in want) + ")"),
             ";;   pseudo   refs   live      prio  calls  reg"]
    for i, (pseudos, size, refs, live, pri, calls) in enumerate(rows):
        tie = any(j != i and rows[j][4] == pri for j in (i - 1, i + 1) if 0 <= j < len(rows))
        reg = reg_name(disp[pseudos[-1]]) if pseudos[-1] in disp else "-"
        name = "+".join(map(str, pseudos)) + (f" ({size})" if size != 1 else "")
        lines.append(f";;   {name:>6} {refs:6d} {live:6d} {pri:9d}  {('X' * min(calls, 1)):>5}  {reg:<5}"
                     + ("  tie" if tie else ""))
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
        lreg = next((s[fn] for p, s in text.items() if p.name.endswith(".lreg") and fn in s), "")
        print(f"\n--- {fn}: " + ", ".join(f"{p.name}.{fn}" for p, s in text.items() if fn in s))
        for line in allocation(greg) + priorities(lreg, greg):
            print(line)
    return 0


if __name__ == "__main__":
    sys.exit(main())
