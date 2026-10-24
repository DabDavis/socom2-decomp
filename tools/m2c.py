#!/usr/bin/env python3
"""
Draft C for a target function with m2c (the mips-to-c decompiler).

    ./tools/m2c.py 0x3C4B40                  # print m2c's draft
    ./tools/m2c.py func_003C4B40 --asm       # print the assembly it is fed instead
    ./tools/m2c.py 0x3C4B40 --ctx ctx.h      # with a context file of types

The function is exported from the r0001 image as the assembly m2c reads (the splat
style its tests use): `$` registers, `.L<VA>` branch labels, callees by their map
names, and every lui/addiu|load|store pair as %hi/%lo of a symbol -- the map's name
when it has one, `data_<VA>` otherwise (BSS included), which the relocation check
resolves by the address in the name. m2c is EQOA's checkout (M2C or
~/claude-eqoa/decomp/tools/m2c); the target triple is mipsee-gcc-c by default
(`--triple`). A draft is a starting point to rewrite, never source to commit.
"""

from __future__ import annotations

import argparse
import os
import re
import subprocess
import sys
import tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(HERE))
import scdecomp as E

M2C = Path(os.environ.get("M2C") or Path.home() / "claude-eqoa" / "decomp" / "tools" / "m2c" / "m2c.py")

REGS = ("zero at v0 v1 a0 a1 a2 a3 t0 t1 t2 t3 t4 t5 t6 t7 t8 t9 s0 s1 s2 s3 s4 s5 s6 s7 "
        "s8 fp k0 k1 gp sp ra").split()
_REG = re.compile(r"(?<![\w$.])(" + "|".join(REGS) + r")(?![\w])")
BRANCHES = {"b", "beq", "bne", "beqz", "bnez", "blez", "bgtz", "bltz", "bgez", "beql", "bnel",
            "blezl", "bgtzl", "bltzl", "bgezl", "bal", "bltzal", "bgezal", "bc1t", "bc1f",
            "bc1tl", "bc1fl", "bc0t", "bc0f"}


def gpr_written(w: int) -> int | None:
    """The general register an instruction writes, or None. FPU and VU loads (lwc1,
    lqc2) keep their destination in the rt field too -- it is not a GPR there."""
    opc, rs, rt, rd = w >> 26, (w >> 21) & 31, (w >> 16) & 31, (w >> 11) & 31
    if opc == 0:
        return rd or None                                  # R-type
    if opc == 0x1C:
        return rd or None                                  # MMI
    if opc == 3:
        return 31                                          # jal
    if 0x08 <= opc <= 0x0F or opc in (0x18, 0x19):
        return rt or None                                  # I-type ALU, daddi(u)
    if 0x20 <= opc <= 0x27 or opc in (0x1A, 0x1B, 0x1E, 0x37):
        return rt or None                                  # loads into a GPR
    if opc == 0x11 and rs in (0, 1, 2):
        return rt or None                                  # mfc1/dmfc1/cfc1
    if opc == 0x12 and rs in (1, 2):
        return rt or None                                  # qmfc2/cfc2
    return None


def symname(st: E.SymbolTable, addr: int, func: bool) -> str:
    s = st.at_addr(addr)
    if s is not None:
        return s.name
    return f"func_{addr:08X}" if func else f"data_{addr:08X}"


def export(img: E.Image, st: E.SymbolTable, sym: E.Symbol) -> str:
    rows = E.disasm(img.read(sym.addr, sym.size), sym.addr)
    targets = set()
    for addr, w, text in rows:
        op = text.split()[0]
        if op in BRANCHES:
            targets.add(int(text.split(",")[-1].split()[-1], 16))
    # %hi/%lo: a lui whose register a later instruction uses as a base or addiu source
    pair: dict[int, str] = {}                  # instruction address -> symbol of its pair
    live: dict[int, tuple[int, int]] = {}      # reg -> (lui address, hi)
    for addr, w, text in rows:
        opc, rs, rt, rd = w >> 26, (w >> 21) & 31, (w >> 16) & 31, (w >> 11) & 31
        if opc == 0x0F:                        # lui
            live[rt] = (addr, w & 0xFFFF)
            continue
        if rs in live and (opc == 0x09 or 0x20 <= opc <= 0x3F):   # addiu, loads, stores
            lui_at, hi = live[rs]
            lo = w & 0xFFFF
            full = ((hi << 16) + (lo - 0x10000 if lo & 0x8000 else lo)) & 0xFFFFFFFF
            if img.in_memory(full):
                name = symname(st, full, img.is_code(full))
                pair[lui_at] = pair[addr] = name
        written = gpr_written(w)
        if written in live:
            live.pop(written, None)
    out = [f"glabel {sym.name}"]
    for addr, w, text in rows:
        if addr in targets:
            out.append(f".L{addr:08X}:")
        op, _, args = text.partition(" ")
        args = _REG.sub(lambda m: "$" + ("fp" if m.group(1) == "s8" else m.group(1)), args)
        if op in BRANCHES:
            parts = args.split(",")
            parts[-1] = f".L{int(parts[-1].split()[-1], 16):08X}"
            args = ", ".join(p.strip() for p in parts)
        elif op in ("jal", "j"):
            t = int(args.split()[0], 16)
            args = symname(st, t, True) if (op == "jal" or st.at_addr(t)) else f".L{t:08X}"
        elif addr in pair:
            if op == "lui":
                args = re.sub(r",\s*\S+$", f", %hi({pair[addr]})", args)
            else:
                args = re.sub(r"(-?0x[0-9a-f]+|-?\d+)(\(\$\w+\))", f"%lo({pair[addr]})\\2", args, count=1) \
                    if "(" in args else re.sub(r",\s*-?(0x[0-9a-f]+|\d+)$", f", %lo({pair[addr]})", args)
        out.append(f"  {op:<10} {args}".rstrip())
    return "\n".join(out) + "\n"


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("symbol")
    ap.add_argument("--asm", action="store_true", help="print the exported assembly only")
    ap.add_argument("--ctx", type=Path, help="a C context file (types, prototypes)")
    ap.add_argument("--triple", default="mipsee-gcc-c")
    a = ap.parse_args(argv)
    img, st = E.image(), E.symbols()
    sym = st.lookup(a.symbol)
    asm = export(img, st, sym)
    if a.asm:
        print(asm, end="")
        return 0
    with tempfile.NamedTemporaryFile("w", suffix=".s", dir=HERE / "build", delete=False) as f:
        f.write(asm)
        path = f.name
    cmd = [sys.executable, str(M2C), "-t", a.triple, "--valid-syntax"]
    if a.ctx:
        cmd += ["--context", str(a.ctx)]
    r = subprocess.run(cmd + [path], capture_output=True, text=True)
    os.unlink(path)
    sys.stdout.write(r.stdout)
    sys.stderr.write(r.stderr)
    return r.returncode


if __name__ == "__main__":
    sys.exit(main())
