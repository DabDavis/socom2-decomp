#!/usr/bin/env python3
"""
scd -- look at the r0001 target, one function at a time.

  ./scd.py show  0x1C5DA0            # disassemble a map entry (name or address)
  ./scd.py show  func_001C5DA0 --raw # with the words, for pasting into a scratch
  ./scd.py list  'GetWeapon'         # map rows matching a regex: address, size, name
  ./scd.py where 0x3C4B50            # which function holds an address
  ./scd.py diff  src/x.c func_X      # = ./scdiff.py src/x.c func_X
  ./scd.py census                    # the map's shape per segment
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
    p = sub.add_parser("diff")
    p.add_argument("rest", nargs=argparse.REMAINDER)
    p.set_defaults(fn=lambda a: __import__("scdiff").main(a.rest))
    a = ap.parse_args(argv)
    return a.fn(a)


if __name__ == "__main__":
    sys.exit(main())
