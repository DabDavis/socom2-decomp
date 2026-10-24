#!/usr/bin/env python3
"""
Name map placeholders off the build's OWN reference sites (claude-eqoa's eqcallnames.py).

Where a byte-exact function's object calls or addresses a real name, and the target
at that same site reaches a `func_<VA>` placeholder (or an address the map lacks), that
name is proposed for the placeholder. It is evidence, not a guess: the function is
already proven byte-exact, so the site is the same site.

    ./tools/callnames.py              # propose; write nothing
    ./tools/callnames.py --rename     # also rename the placeholders in symbols.txt

Conflicts (one name proposed for two addresses, or two names for one) are reported
and never applied. Only FUNCTION targets (R_MIPS_26) are renamed; a data address the
map lacks is listed, to be added by hand with its size.
"""

from __future__ import annotations

import argparse
import re
import sys
from collections import defaultdict
from pathlib import Path

HERE = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(HERE))
import scdecomp as E
import scdiff as D
import sccheck as C


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--rename", action="store_true")
    a = ap.parse_args(argv)
    img, st = E.image(), E.symbols()
    proposals: dict[int, set[str]] = defaultdict(set)
    data: dict[int, set[str]] = defaultdict(set)
    for src, obj, err in map(C.build_one, C.sources()):
        if obj is None:
            continue
        for fs in E.Elf.read(obj).functions():
            try:
                sym = st.get(fs.name)
            except KeyError:
                continue
            res = D.compare(obj, fs.name, sym, img, st)
            if not res.match:
                continue
            for f in res.reloc_unchecked:
                if f.want is None or f.symbol.endswith("(local)"):
                    continue
                if f.type == "R_MIPS_26":
                    proposals[f.want].add(f.symbol)
                else:
                    data[f.want].add(f.symbol)
    names: dict[str, set[int]] = defaultdict(set)
    for addr, ns in proposals.items():
        for n in ns:
            names[n].add(addr)
    apply = {}
    for addr, ns in sorted(proposals.items()):
        cur = st.at_addr(addr)
        if len(ns) > 1 or any(len(names[n]) > 1 for n in ns):
            print(f"CONFLICT 0x{addr:08X}: {sorted(ns)}")
            continue
        n = next(iter(ns))
        if cur is None:
            print(f"  0x{addr:08X} {n:<28} (no map row at this address: the sweep missed a start)")
        elif not E.is_placeholder(cur.name):
            print(f"  0x{addr:08X} {n:<28} already {cur.name}")
        else:
            print(f"  0x{addr:08X} {n:<28} <- {cur.name}")
            apply[addr] = n
    for addr, ns in sorted(data.items()):
        print(f"  data 0x{addr:08X} {sorted(ns)}  (add by hand, with its size)")
    if a.rename and apply:
        text = E.SYMBOLS.read_text().splitlines()
        n = 0
        for i, line in enumerate(text):
            m = re.match(r"^(0x[0-9A-Fa-f]+)\s+(\S+)\s+(\S+)(.*)$", line)
            if m and int(m.group(1), 16) in apply and E.is_placeholder(m.group(3)):
                text[i] = f"{m.group(1)} {m.group(2)} {apply[int(m.group(1), 16)]}{m.group(4)}"
                n += 1
        E.SYMBOLS.write_text("\n".join(text) + "\n")
        print(f"renamed {n} rows")
    return 0


if __name__ == "__main__":
    sys.exit(main())
