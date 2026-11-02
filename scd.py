#!/usr/bin/env python3
"""
scd -- look at the r0001 target, one function at a time.

  ./scd.py show  0x1C5DA0            # disassemble a map entry (name or address)
  ./scd.py show  func_001C5DA0 --raw # with the words, for pasting into a scratch
  ./scd.py list  'GetWeapon'         # map rows matching a regex: address, size, name
  ./scd.py where 0x3C4B50            # which function holds an address
  ./scd.py diff  src/x.c func_X      # = ./scdiff.py src/x.c func_X
  ./scd.py census                    # the map's shape per segment
  ./scd.py s1    'Tick__10CHUDFf'    # a SOCOM 1 demo function (regex on its names, or an
                                     # address), with the demo's relocations and callees
  ./scd.py s1 --units 0x15B000 0x165000   # the demo's input objects there (C: gcc2_compiled.;
                                     # else assembled), each with its functions, statics marked
  ./scd.py s1 --callers _sceSDC      # the demo functions that j/jal it
"""

from __future__ import annotations

import argparse
import sys
from collections import Counter
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import scdecomp as E


def cmd_show(a) -> int:
    img, st = E.image(), E.symbols()
    s = st.lookup(a.symbol)
    seg = img.segment(s.addr)
    print(f"{s.name}  0x{s.addr:08X}  0x{s.size:X} B  {seg.name if seg else '?'}"
          + (f"  # {s.note}" if s.note else ""))
    for addr, w, text in E.disasm(img.read(s.addr, s.size), s.addr):
        tgt = ""
        if w >> 26 in (2, 3):
            t = ((addr + 4) & 0xF0000000) | ((w & 0x03FFFFFF) << 2)
            callee = st.at_addr(t)
            tgt = f"   -> {callee.name}" if callee else ""
        print(f"  {addr:08x}:  {w:08x}  {text}{tgt}" if a.raw else f"  {addr:08x}:  {text}{tgt}")
    return 0


def cmd_list(a) -> int:
    for s in E.symbols().find(a.pattern):
        print(f"0x{s.addr:08X} 0x{s.size:05X} {s.name}" + (f"  # {s.note}" if s.note else ""))
    return 0


def cmd_where(a) -> int:
    addr = int(a.addr, 16)
    s = E.symbols().containing(addr)
    print(f"0x{addr:08X} is in {s.name} +0x{addr - s.addr:X}" if s else f"0x{addr:08X}: no function")
    return 0 if s else 1


def cmd_census(a) -> int:
    img, st = E.image(), E.symbols()
    per, size = Counter(), Counter()
    named = Counter()
    for s in st.symbols:
        seg = img.segment(s.addr)
        k = seg.name if seg else "?"
        per[k] += 1
        size[k] += s.size
        if not E.is_placeholder(s.name):
            named[k] += 1
    for k in per:
        print(f"{k:<5} {per[k]:>6} functions  {size[k]:>9,} bytes  {named[k]:>5} named")
    print(f"total {len(st):>6} functions  {sum(size.values()):>9,} bytes  {sum(named.values()):>5} named")
    return 0


def s1_units(demo, lo: int, hi: int) -> int:
    """The demo's input objects in [lo, hi): its symbol table keeps each object's `.text` SECTION
    symbol, and GCC marks every object it compiled with a local `gcc2_compiled.` at that address --
    an SDK or library object without one was assembled (README "The GCC front"). Each object with
    its functions in address order, a static one marked `L`."""
    starts = sorted({s.value for s in demo.elf.symbols if s.type == "SECTION" and s.bind == 0
                     and s.shndx == demo.sec and lo <= s.value < hi})
    gcc = {s.value for s in demo.elf.symbols if s.name == "gcc2_compiled."}
    funcs = sorted((s.value, s.size, s.bind, s.name) for s in demo.elf.symbols
                   if s.type == "FUNC" and s.shndx == demo.sec and lo <= s.value < hi)
    for i, t in enumerate(starts):
        end = starts[i + 1] if i + 1 < len(starts) else hi
        fs = [f for f in funcs if t <= f[0] < end]
        print(f"---- object 0x{t:08X}-0x{end:08X} {'C (gcc2_compiled.)' if t in gcc else 'assembler (no marker)'}"
              f", {len(fs)} function(s)")
        for v, size, bind, name in fs:
            print(f"  0x{v:08X} 0x{size:05X} {'L' if bind == 0 else ' '} {name}")
    return 0


def s1_callers(demo, name: str) -> int:
    """Every demo function holding a j/jal to the demo function `name`."""
    tgt = [f.value for f in demo.funcs if f.name == name]
    if not tgt:
        print(f"no demo function {name!r}")
        return 1
    for f in demo.funcs:
        data = demo.read(f.value, f.size)
        for i in range(0, len(data) - 3, 4):
            w = int.from_bytes(data[i:i + 4], "little")
            if w >> 26 in (2, 3):
                at = f.value + i
                if ((at + 4) & 0xF0000000) | ((w & 0x03FFFFFF) << 2) in tgt:
                    print(f"0x{f.value:08X} 0x{f.size:X} {f.name} ({'jal' if w >> 26 == 3 else 'j'} at 0x{at:08X})")
    return 0


def cmd_s1(a) -> int:
    """SOCOM 1's May 2002 demo, unstripped: its functions by name or address."""
    import re
    sys.path.insert(0, str(HERE / "tools"))
    import s1names as N
    demo = N.Demo()
    if a.units:
        return s1_units(demo, int(a.units[0], 16), int(a.units[1], 16))
    if a.callers:
        return s1_callers(demo, a.callers)
    if not a.symbol:
        print("name a demo function (regex or address), or give --units LO HI / --callers NAME")
        return 1
    fname = {}
    for s in demo.elf.symbols:
        if s.value and s.name and s.type in ("FUNC", "OBJECT"):
            fname.setdefault(s.value, s.name)
    try:
        at = int(a.symbol, 16)
        hits = [f for f in demo.funcs if f.value <= at < f.value + max(f.size, 1)]
    except ValueError:
        rx = re.compile(a.symbol)
        hits = [f for f in demo.funcs if rx.search(f.name)]
    if not hits:
        print(f"no demo function matches {a.symbol!r}")
        return 1
    if len(hits) > a.max:
        for f in hits:
            print(f"0x{f.value:08X} 0x{f.size:05X} {f.name}")
        print(f"{len(hits)} match: name one (or raise --max)")
        return 0
    for f in hits:
        print(f"== {f.name}  0x{f.value:08X}  0x{f.size:X} B")
        for addr, w, text in E.disasm(demo.read(f.value, f.size), f.value):
            r = demo.relocs.get(addr, "")
            tgt = ""
            if w >> 26 in (2, 3):
                t = ((addr + 4) & 0xF0000000) | ((w & 0x03FFFFFF) << 2)
                tgt = "   -> " + fname.get(t, hex(t))
            print(f"  {addr:08x}:  {text}" + (f"  <{r}>" if r else "") + tgt)
    return 0


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="cmd", required=True)
    p = sub.add_parser("show")
    p.add_argument("symbol")
    p.add_argument("--raw", action="store_true")
    p.set_defaults(fn=cmd_show)
    p = sub.add_parser("list")
    p.add_argument("pattern")
    p.set_defaults(fn=cmd_list)
    p = sub.add_parser("where")
    p.add_argument("addr")
    p.set_defaults(fn=cmd_where)
    p = sub.add_parser("census")
    p.set_defaults(fn=cmd_census)
    p = sub.add_parser("s1")
    p.add_argument("symbol", nargs="?")
    p.add_argument("--max", type=int, default=3)
    p.add_argument("--units", nargs=2, metavar=("LO", "HI"),
                   help="the demo's input objects in [LO, HI) (demo addresses), each with its functions")
    p.add_argument("--callers", metavar="NAME", help="the demo functions that j/jal the demo function NAME")
    p.set_defaults(fn=cmd_s1)
    p = sub.add_parser("diff")
    p.add_argument("rest", nargs=argparse.REMAINDER)
    p.set_defaults(fn=lambda a: __import__("scdiff").main(a.rest))
    a = ap.parse_args(argv)
    return a.fn(a)


if __name__ == "__main__":
    sys.exit(main())
