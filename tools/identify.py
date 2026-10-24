#!/usr/bin/env python3
"""
Identify known-source functions in the r0001 image: compile a TU whose source is
certain (zlib 1.1.4, later the MSL runtime), and for every function in the object
find the map entry whose bytes equal it once the object's own relocation fields
are masked on both sides.

    ./tools/identify.py SRC.c [SRC.c ...] [-I DIR] [--flags "..."]
    ./tools/identify.py ... --rename      # also rename the placeholders in symbols.txt

A match must be UNIQUE among map entries of that size, and its relocations must
then resolve consistently: the second pass re-checks every call and data reference
with the proposed names applied (scdiff.reloc_check), so a function that calls the
wrong neighbour is not credited just because its bytes look alike. A name is never
written over a non-placeholder.
"""

from __future__ import annotations

import argparse
import re
import sys
import tempfile
from collections import defaultdict
from pathlib import Path

HERE = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(HERE))
import scdecomp as E
import scdiff as D


def candidates(img: E.Image, st: E.SymbolTable, body: bytes, relocs) -> list[E.Symbol]:
    """Map entries holding these bytes. An entry may be LONGER than the function when the
    sweep swallowed padding (a `jr v0` tail call ends no sweep): the excess must be zero."""
    want = D.mask(body, relocs)
    hits = []
    for s in st.symbols:
        if s.size < len(body) or s.size - len(body) >= 16:
            continue
        if img.read(s.addr + len(body), s.size - len(body)).strip(b"\0"):
            continue
        if D.mask(img.read(s.addr, len(body)), relocs) == want:
            hits.append(E.Symbol(s.name, s.addr, len(body), s.note))
    return hits


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("sources", nargs="+", type=Path)
    ap.add_argument("-I", dest="includes", action="append", type=Path, default=[])
    ap.add_argument("--flags", default=" ".join(D.FLAGS))
    ap.add_argument("--compiler", default=D.COMPILER)
    ap.add_argument("--rename", action="store_true")
    a = ap.parse_args(argv)
    img, st = E.image(), E.symbols()

    found: dict[str, E.Symbol] = {}
    objs = {}
    out = HERE / "build" / "identify"
    for src in a.sources:
        obj = out / (src.stem + ".o")
        try:
            D.compile_obj(src, obj, a.compiler, a.flags.split(), a.includes)
        except RuntimeError as e:
            print(f"BUILD-FAIL {src}: {str(e).strip().splitlines()[-1]}")
            continue
        elf = E.Elf.read(obj)
        for f in elf.functions():
            body, relocs, _ = elf.function(f.name)
            hits = candidates(img, st, body, relocs)
            if len(hits) == 1:
                found[f.name] = hits[0]
                objs[f.name] = obj
                print(f"  {src.name:<14} {f.name:<28} = 0x{hits[0].addr:08X} {hits[0].name}")
            elif hits:
                print(f"  {src.name:<14} {f.name:<28} AMBIGUOUS {[hex(h.addr) for h in hits][:6]}")
            else:
                print(f"  {src.name:<14} {f.name:<28} -")

    # second pass: with the proposed names applied, every relocation must resolve
    renamed = {s.name: name for name, s in found.items()}
    st2 = E.SymbolTable()
    for s in st2.symbols:
        if s.name in renamed:
            st2._by_name[renamed[s.name]] = s
    good = {}
    for name, s in found.items():
        res = D.compare(objs[name], name, s, img, st2)   # s.size is the function's exact size
        bad = res.reloc_bad
        if res.match:
            good[name] = s
        print(f"{'ok  ' if res.match else 'BAD '} {name:<28} 0x{s.addr:08X}  {res.verdict}"
              + (f"  {[str(b) for b in bad][:2]}" if bad else ""))
    print(f"\n{len(good)} identified and verified, {len(found) - len(good)} rejected by the relocation check")

    if a.rename and good:
        text = E.SYMBOLS.read_text().splitlines()
        by_addr = {s.addr: n for n, s in good.items()}
        n = 0
        for i, line in enumerate(text):
            m = re.match(r"^(0x[0-9A-Fa-f]+)\s+(\S+)\s+(\S+)(.*)$", line)
            if not m:
                continue
            addr = int(m.group(1), 16)
            if addr in by_addr and E.is_placeholder(m.group(3)):
                size = good[by_addr[addr]].size          # exact, padding dropped
                text[i] = f"{m.group(1)} 0x{size:X} {by_addr[addr]}{m.group(4)}"
                n += 1
        E.SYMBOLS.write_text("\n".join(text) + "\n")
        print(f"renamed {n} rows in {E.SYMBOLS.name}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
