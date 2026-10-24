#!/usr/bin/env python3
"""
Seed decomp/symbols.txt: sweep the r0001 image's code for function boundaries.

    ./tools/seed_symbols.py --check          # sweep and report; write nothing
    ./tools/seed_symbols.py --write          # (re)write symbols.txt -- refuses if it
                                             # holds a name that is not a placeholder

The walk is claude-eqoa's eqmodule.sweep, adapted to an absolutely linked image:
from the first non-zero word, follow the code, tracking the furthest address any
branch reaches; the function ends at a `jr ra` (or a tail-call `j` to a known
entry) that no branch reaches past. Call targets found inside a swept range split
it. Metrowerks adds one structural check the GCC modules did not have: it starts
EVERY function on a 16-byte boundary and pads with zero words, so a start that is
not 16-aligned is a sweep error, not a function -- reported, never written.
(Sony's prebuilt GCC libraries in MAIN align to 8; those are counted apart.)

Code regions: MAIN 0x180000..TEXT_END (the loader's first 512 KB are zero), and
each overlay's MWo3 text range (header +0x0C).
"""

from __future__ import annotations

import argparse
import struct
import sys
from collections import Counter
from pathlib import Path

HERE = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(HERE))
import scdecomp as E

# MAIN's code, by inspection (README "The image"): the loader's first 512 KB are zero;
# 0x1CA3D0-0x1CCD78 is a zero block plus Sony's libipu/libkernel tables INSIDE .text;
# 0x1CE2A0-0x1D4B00 is data (zlib 1.1.4's strings among it); 0x1D4B00-0x1D4F90 are the
# __sinit_* static initialisers MW places after rodata; then the linker's tables.
MAIN_TEXT = [(0x00180000, 0x001CA3D0), (0x001CCD78, 0x001CE2A0), (0x001D4B00, 0x001D4F90)]


def branch_target(addr: int, w: int) -> int | None:
    op, rs, rt = w >> 26, (w >> 21) & 31, (w >> 16) & 31
    imm = w & 0xFFFF
    rel = addr + 4 + ((imm - 0x10000 if imm & 0x8000 else imm) << 2)
    if op in (4, 5, 6, 7, 20, 21, 22, 23):              # beq bne blez bgtz + likely
        return rel
    if op == 1 and rt in (0, 1, 2, 3, 16, 17, 18, 19):  # REGIMM bltz bgez (al/l)
        return rel
    if op in (17, 18) and rs == 8:                      # bc1x / bc2x
        return rel
    return None


def jump_target(addr: int, w: int) -> int:
    return ((addr + 4) & 0xF0000000) | ((w & 0x03FFFFFF) << 2)


def regions(img: E.Image) -> list[tuple[str, int, int]]:
    out = [("MAIN", lo, hi) for lo, hi in MAIN_TEXT]
    for s in img.segments:
        if s.header is not None:
            out.append((s.name, *s.code))     # the MWo3 text range minus the VU1 microcode tail
    return out


def call_targets(img: E.Image, regs) -> set[int]:
    calls = set()
    for _n, lo, hi in regs:
        for a in range(lo, hi, 4):
            w = img.word(a)
            if w >> 26 == 3:
                t = jump_target(a, w)
                if any(l <= t < h for _, l, h in regs):
                    calls.add(t)
    return calls


def sweep(img: E.Image, lo: int, hi: int, starts: set[int]) -> list[tuple[int, int]]:
    funcs = []
    pos = lo
    while pos < hi:
        # padding: zero words, and the odd 0xFFFFFFFF (after OVL1's entry at 0x1E70F4;
        # `sd ra,-1(ra)` begins no real function)
        while pos < hi and img.word(pos) in (0, 0xFFFFFFFF):
            pos += 4
        if pos >= hi:
            break
        start = reach = off = pos
        while off < hi - 4:
            w = img.word(off)
            t = branch_target(off, w)
            if t is not None and t > reach:
                reach = t
            tail = False
            if w >> 26 == 2:                                  # j: a label, or a tail call
                t = jump_target(off, w)
                # MW never uses `j` for local flow (it branches with `b`); Sony's GCC
                # code does, but only a short hop forward with code after the delay
                # slot. Anything else is a tail call: counting it as reach let a
                # 16-byte thunk at 0x180E00 (`j 0x181AA0`) swallow 3 KB of MAIN.
                local = off < t <= off + 0x400 and img.word(off + 8) != 0
                if not start <= t <= off and (t in starts or not local):
                    tail = True
                elif lo <= t < hi and t > reach:
                    reach = t
            if (w == E.JR_RA or tail) and reach < off + 8:
                break
            # a loop that never returns: an unconditional branch back that nothing
            # reaches past, then a prologue at the next 16-byte boundary (OVL1's entry
            # at 0x1E7040 loops forever and is followed by a stray 0xFFFFFFFF word)
            if (w >> 16 == 0x1000 or w >> 26 == 2) and reach < off + 8:
                t = branch_target(off, w) if w >> 16 == 0x1000 else jump_target(off, w)
                nxt = off + 8
                while nxt < hi and img.word(nxt) == 0:
                    nxt += 4
                aligned = (off + 8 + 15) & ~15
                def prologue(a):
                    return a < hi and img.word(a) >> 16 == 0x27BD and img.word(a) & 0x8000
                if t <= off and (prologue(nxt) or (aligned < hi and prologue(aligned)
                                                   and aligned - (off + 8) <= 12)):
                    break
            off += 4
        funcs.append((start, off + 8))
        pos = off + 8
    out = []
    for start, end in funcs:
        for s in sorted(x for x in starts if start < x < end):
            out.append((start, s))
            start = s
        out.append((start, end))
    return out


def run(img: E.Image):
    regs = regions(img)
    starts = call_targets(img, regs)
    found = []
    for name, lo, hi in regs:
        fs = sweep(img, lo, hi, starts)
        found += [(name, a, b) for a, b in fs]
    return regs, starts, found


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    g = ap.add_mutually_exclusive_group(required=True)
    g.add_argument("--check", action="store_true")
    g.add_argument("--write", action="store_true")
    a = ap.parse_args(argv)
    img = E.image()
    regs, starts, found = run(img)
    per = Counter(n for n, _, _ in found)
    unaligned = [(n, s, e) for n, s, e in found if s % 16]
    for name, lo, hi in regs:
        print(f"{name}: code 0x{lo:08X}-0x{hi:08X}  {per[name]} functions, "
              f"{sum(1 for n, s, _ in unaligned if n == name)} not 16-aligned")
    print(f"call targets {len(starts)}, functions {len(found)}")
    if a.check:
        for n, s, e in unaligned[:20]:
            print(f"  unaligned start {n} 0x{s:08X} (0x{e - s:X} B)")
        return 0
    if E.SYMBOLS.exists():
        named = [s for s in E.SymbolTable().symbols if not E.is_placeholder(s.name)]
        if named:
            raise SystemExit(f"{E.SYMBOLS} holds {len(named)} real names; the map is kept by hand now")
    lines = ["# decomp/symbols.txt -- the r0001 function map: 0xADDR SIZE name  # note",
             "# Seeded by tools/seed_symbols.py; from here on it is kept BY HAND: rename a",
             "# func_<VA> to the mangled name its source defines. SIZE excludes MW's zero padding.",
             ""]
    for n, s, e in found:
        note = f"  # {n}" + (" unaligned" if s % 16 else "")
        lines.append(f"0x{s:08X} 0x{e - s:X} func_{s:08X}{note}")
    E.SYMBOLS.write_text("\n".join(lines) + "\n")
    print(f"wrote {E.SYMBOLS} ({len(found)} rows)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
