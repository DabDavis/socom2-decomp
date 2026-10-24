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
import struct
import subprocess
import sys
import tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(HERE))
import scdecomp as E

# build/m2c (tools/m2c_setup.sh: claude-eqoa's checkout plus the R5900 accumulator ops) when it
# exists, else claude-eqoa's own
_PRIVATE = HERE / "build" / "m2c" / "m2c.py"
M2C = Path(os.environ.get("M2C") or (_PRIVATE if _PRIVATE.exists() else
                                     Path.home() / "claude-eqoa" / "decomp" / "tools" / "m2c" / "m2c.py"))

REGS = ("zero at v0 v1 a0 a1 a2 a3 t0 t1 t2 t3 t4 t5 t6 t7 t8 t9 s0 s1 s2 s3 s4 s5 s6 s7 "
        "s8 fp k0 k1 gp sp ra").split()
_REG = re.compile(r"(?<![\w$.])(" + "|".join(REGS) + r")(?![\w])")
BRANCHES = {"b", "beq", "bne", "beqz", "bnez", "blez", "bgtz", "bltz", "bgez", "beql", "bnel",
            "beqzl", "bnezl",       # objdump's names for beql/bnel against $zero: left out, their
                                    # targets stayed raw addresses and m2c refused 74 functions
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
    s = st.at_addr(addr) or (None if func else st.data_by_addr.get(addr))
    if s is not None and re.match(r"^[A-Za-z_][A-Za-z0-9_]*$", s.name):
        return s.name                      # a template or anonymous-namespace name m2c cannot read
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
    pair_value: dict[int, tuple[int, int]] = {}   # lo instruction -> (its lui, the address)
    live: dict[int, tuple[int, int]] = {}      # reg -> (lui address, hi)
    after_jump = False
    for addr, w, text in rows:
        if after_jump is True:                 # past an unconditional jump and its delay slot,
            live.clear()                       # nothing falls through to here
            after_jump = False
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
                pair_value[addr] = (lui_at, full)
        written = gpr_written(w)
        if written in live:
            live.pop(written, None)
        if text.split()[0] in ("b", "j", "jr"):
            after_jump = "pending"
        elif after_jump == "pending":
            after_jump = True
    # a lui MW hoisted into a branch's delay slot serves the branch target too (`beqz v0,L;
    # lui at,hi` then `L: lw v0,lo(at)`): followed into the target, or m2c reads the load as an
    # absolute address -- 37 drafts missed by exactly these words
    index = {addr: i for i, (addr, _, _) in enumerate(rows)}
    for i, (addr, w, text) in enumerate(rows):
        if w >> 26 != 0x0F or addr in pair or i == 0 or text.split()[0] in ("j", "jal"):
            continue
        prev = rows[i - 1][2].split()
        if not prev or prev[0] not in BRANCHES:
            continue
        reg = (w >> 16) & 31
        tgt = index.get(int(prev[-1].split(",")[-1], 16))
        for k in range(tgt, len(rows)) if tgt is not None else ():
            w2 = rows[k][1]
            opc, rs = w2 >> 26, (w2 >> 21) & 31
            if rs == reg and (opc == 0x09 or 0x20 <= opc <= 0x3F):
                lo = w2 & 0xFFFF
                full = (((w & 0xFFFF) << 16) + (lo - 0x10000 if lo & 0x8000 else lo)) & 0xFFFFFFFF
                if img.in_memory(full):
                    name = symname(st, full, img.is_code(full))
                    if rows[k][0] not in pair or pair[rows[k][0]] == name:
                        pair[addr] = pair[rows[k][0]] = name
                break
            if gpr_written(w2) == reg or rows[k][2].split()[0] in BRANCHES | {"j", "jr", "jal", "jalr"}:
                break
    # a switch's jump table. MW forms its address with a lui/addiu pair, adds the scaled index
    # and loads from offset 0 (`lui v1,hi; sll v0,v0,2; addiu v1,v1,lo; addu v0,v0,v1;
    # lw v0,0(v0); jr v0`); IDO-style code folds lo into the load (`lui at,hi; addu at,at,v0;
    # lw v0,lo(at)`). m2c reads one only as a symbol named jtbl_* whose entries it can see, so
    # the table is exported too, each entry a label in this function; its length is the bound
    # check's (`sltiu _, _, N`) or the run of entries that land in the function
    jtbl: dict[int, str] = {}                  # instruction address -> table symbol
    tables: dict[str, list[int]] = {}

    def writer(i0: int, reg: int, back: int, pred) -> int | None:
        return next((i for i in range(i0 - 1, max(i0 - back, -1), -1)
                     if gpr_written(rows[i][1]) == reg and pred(rows[i][1])), None)

    for k, (addr, w, text) in enumerate(rows):
        if not (w >> 26 == 0 and (w & 0x3F) == 8 and (w >> 21) & 31 != 31):
            continue
        lw_i = writer(k, (w >> 21) & 31, 6, lambda x: x >> 26 == 0x23)
        if lw_i is None:
            continue
        lw = rows[lw_i][1]
        add_i = writer(lw_i, (lw >> 21) & 31, 6, lambda x: x >> 26 == 0 and x & 0x3F in (0x21, 0x2D))
        if add_i is None:
            continue
        add = rows[add_i][1]
        table = lui_i = lo_i = None
        for src in ((add >> 21) & 31, (add >> 16) & 31):
            ai = writer(add_i, src, 8, lambda x: x >> 26 == 0x09)
            if ai is not None and rows[ai][0] in pair_value and lw & 0xFFFF == 0:
                lui_at, table = pair_value[rows[ai][0]]
                lo_i = ai
                break
            li = writer(add_i, src, 8, lambda x: x >> 26 == 0x0F)
            if li is not None and src == (add >> 11) & 31:
                lo = lw & 0xFFFF
                table = (((rows[li][1] & 0xFFFF) << 16) + (lo - 0x10000 if lo & 0x8000 else lo)) & 0xFFFFFFFF
                lui_at, lo_i = rows[li][0], lw_i
                break
        if table is None:
            continue
        bound = next((rows[i][1] & 0xFFFF for i in range(add_i - 1, max(add_i - 12, -1), -1)
                      if rows[i][1] >> 26 == 0x0B), None)
        entries = []
        while bound is None or len(entries) < bound:
            if not img.in_memory(table + 4 * len(entries)):
                break
            e = struct.unpack("<I", img.read(table + 4 * len(entries), 4))[0]
            if not (sym.addr <= e < sym.addr + sym.size):
                break
            entries.append(e)
        if not entries or (bound is not None and len(entries) != bound):
            continue
        name = f"jtbl_{table:08X}"
        tables[name] = entries
        targets.update(entries)
        for a in (lui_at, rows[lo_i][0]):
            pair.pop(a, None)
            jtbl[a] = name
    # a tail call -- `j f` with the argument set up in its delay slot -- is rewritten as the
    # delay instruction, a call and a return: m2c reads the delay slot of a jal but not of a
    # j, and drops what it does (an MI thunk's `this` adjustment, a loaded argument)
    tail = {}
    for i, (addr, w, text) in enumerate(rows):
        if w >> 26 == 2 and i + 1 < len(rows):
            t = int(text.split()[1], 16)
            if st.at_addr(t) is not None and t != sym.addr and rows[i + 1][0] not in targets:
                tail[addr] = rows[i + 1]
    skip = {d[0] for d in tail.values()}
    out = []
    for name, entries in tables.items():
        out += [".section .rodata", f"glabel {name}"] + [f".word .L{e:08X}" for e in entries]
    if tables:
        out.append(".section .text")
    out.append(f"glabel {sym.name}")
    for addr, w, text in rows:
        if addr in targets:
            out.append(f".L{addr:08X}:")
        if addr in skip:
            continue
        if addr in tail:
            d_addr, d_w, d_text = tail[addr]
            t = int(text.split()[1], 16)
            d_op, _, d_args = d_text.partition(" ")
            d_args = _REG.sub(lambda m: "$" + ("fp" if m.group(1) == "s8" else m.group(1)), d_args)
            if d_addr in pair:
                d_args = re.sub(r"(-?0x[0-9a-f]+|-?\d+)(\(\$\w+\))", f"%lo({pair[d_addr]})\\2", d_args, count=1) \
                    if "(" in d_args else re.sub(r",\s*-?(0x[0-9a-f]+|\d+)$", f", %lo({pair[d_addr]})", d_args)
            if d_w:
                out.append(f"  {d_op:<10} {d_args}".rstrip())
            # an argument register the delay slot writes, above ones passed through untouched:
            # make those visible to m2c, or it shifts the written one down into a0
            written = gpr_written(d_w) if d_w else None
            if written is not None and 5 <= written <= 7:
                for r in range(4, written):
                    out.append(f"  {'move':<10} ${REGS[r]}, ${REGS[r]}")
            out += [f"  {'jal':<10} {symname(st, t, True)}", "  nop", f"  {'jr':<10} $ra", "  nop"]
            continue
        op, _, args = text.partition(" ")
        args = _REG.sub(lambda m: "$" + ("fp" if m.group(1) == "s8" else m.group(1)), args)
        if op in BRANCHES:
            parts = args.split(",")
            parts[-1] = f".L{int(parts[-1].split()[-1], 16):08X}"
            args = ", ".join(p.strip() for p in parts)
        elif op in ("jal", "j"):
            t = int(args.split()[0], 16)
            args = symname(st, t, True) if (op == "jal" or st.at_addr(t)) else f".L{t:08X}"
        elif addr in jtbl:
            if op == "lui":
                args = re.sub(r",\s*\S+$", f", %hi({jtbl[addr]})", args)
            elif "(" in args:
                args = re.sub(r"(-?0x[0-9a-f]+|-?\d+)(\(\$\w+\))", f"%lo({jtbl[addr]})\\2", args, count=1)
            else:
                args = re.sub(r",\s*-?(0x[0-9a-f]+|\d+)$", f", %lo({jtbl[addr]})", args)
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
