#!/usr/bin/env python3
"""
cdis -- the candidate's disassembly, each relocation named (the game states' round 35 cdis.py,
promoted by the GCC lane's round 31a): what the object holds before the link, read the way the
scorer reads it.

    ./tools/cdis.py src/lib/sdk/mpeg_dec.c dmaRefImage              # offsets from the function
    ./tools/cdis.py src/lib/sdk/mpeg_dec.c dmaRefImage 40 80        # offsets 0x40 up to 0x80 only
    ./tools/cdis.py src/lib/sdk/mpeg_dec.c dmaRefImage --at         # at its row's addresses
    ./tools/cdis.py src/lib/sdk/mpeg_dec.c --list                   # the object's functions

SRC is built as sccheck builds it (tools/structdiff.py's build: the unit's `sc:` directive, `as=`,
its statics qualified `NAME@UNIT`; --compiler/--flags/-I/--unit as scdiff.py's and structdiff's).
Each relocated instruction carries `<TYPE label = 0xADDRESS>`: the symbol as the scorer resolves it
-- a named function or global by the map (a placeholder by its address, an `ovl-copy:` in the
row's overlay), a GNU section reference by the object's own label at that offset (a static, a
qualified `_data_table@sifcmd`), else `.rodata+0x40` (no address: compared by content), a `.text`+N
callee by the function there, a jump into the function by its offset (`+0x40`). LO HI (hex) keep
the offsets from LO up to HI; --at prints the scoring row's addresses (or --target's), so the
listing lines up with `./scd.py show`.
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(HERE))
sys.path.insert(0, str(HERE / "tools"))
import scdecomp as E
import sccheck as C
import structdiff as S


def listing(obj: Path, symbol: str, st, base: int = 0, lo: int = 0, hi: int = 1 << 30,
            taddr: int | None = None) -> list[str]:
    """The function's lines: `  offset: text  <TYPE label = 0xADDR>` (offset rebased to `base`);
    `taddr` is where the scorer resolves from (an `ovl-copy:`'s overlay), default `base`."""
    elf = E.Elf.read(obj)
    cand, relocs, fsym = elf.function(symbol)
    names = S.reloc_names(elf, fsym, cand, relocs, taddr if taddr is not None else base, st)
    out = []
    for a, w, text in E.disasm(cand, base):          # branch targets at the same base as the lines
        if not lo <= a - base < hi:
            continue
        note = ""
        if a - base in names:
            typ, label, addr, own = names[a - base]
            where = f" = 0x{addr:x}" if addr is not None else (f" = +0x{own:x}" if own is not None else "")
            note = f"  <{typ} {label}{where}>"
        out.append(f"  {a:0{8 if base else 5}x}: {text}{note}")
    return out


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("src", type=Path)
    ap.add_argument("symbol", nargs="?", help="the function's name in the object (mangled for C++)")
    ap.add_argument("lo", nargs="?", default="0", help="first offset shown (hex)")
    ap.add_argument("hi", nargs="?", default=None, help="offset shown up to (hex)")
    ap.add_argument("--list", action="store_true", help="list the object's functions: size and name")
    ap.add_argument("--at", action="store_true", help="print the scoring row's addresses, not offsets")
    ap.add_argument("--target", help="the row --at prints and resolves at (a map name or an address)")
    ap.add_argument("--compiler", help="toolchain (default: the unit's `sc: compiler=`)")
    ap.add_argument("--flags", help="compiler flags (default: the unit's `sc: flags=`)")
    ap.add_argument("--unit", type=Path, help="compile SRC under this unit's directive, its statics as its")
    ap.add_argument("-I", dest="includes", action="append", type=Path, default=[])
    ap.add_argument("--obj", type=Path, help="an object already built (a --build-dir's): read, not compiled")
    a = ap.parse_args(argv)
    if not a.list and not a.symbol:
        ap.error("SYMBOL, or --list")
    st = E.symbols()
    obj = a.obj or S.build(a.src, a.compiler, a.flags, a.includes, a.unit, tag="cdis")
    try:
        if a.list:
            elf = E.Elf.read(obj)
            for f in sorted(elf.functions(), key=lambda f: (f.shndx, f.value)):
                print(f"0x{f.size:04x} {f.name}" + ("  (static)" if f.bind == 0 else ""))
            return 0
        base, taddr = 0, None
        try:     # names resolve from the row (its overlay's copies); a probe with no row, from MAIN
            row = S.target_row(st, obj, a.symbol, a.target, C.tu_options(a.unit or a.src).get("overlay"))
        except SystemExit:
            if a.at or a.target:
                raise
            row = None
        if row is not None:
            taddr = row.addr
            base = row.addr if a.at else 0
        lines = listing(obj, a.symbol, st, base, int(a.lo, 16), int(a.hi, 16) if a.hi else 1 << 30, taddr)
    finally:
        if not a.obj:
            obj.unlink(missing_ok=True)
    print("\n".join(lines))
    return 0


if __name__ == "__main__":
    sys.exit(main())
