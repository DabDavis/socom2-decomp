#!/usr/bin/env python3
"""Recompile every GCC draft in an address range with extra flags added to its directive's and
report which still match: ccflags.py LO HI "EXTRA FLAGS". A flag the vendor used loses none
(README "The GCC front": how strict aliasing was told apart per library)."""
import re, sys
from pathlib import Path
D = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(D))
import scdecomp as E
import scdiff
from sccheck import tu_options
lo, hi, extra = int(sys.argv[1], 16), int(sys.argv[2], 16), sys.argv[3].split()
img, st = E.image(), E.symbols()
tmp = D / "build" / "ccsweep" / "ccflags"
tmp.mkdir(parents=True, exist_ok=True)
kept = lost = 0
lost_at = []
for f in sorted((D / "src" / "auto").rglob("*.c")):
    a = int(f.stem, 16)
    if not lo <= a < hi:
        continue
    o = tu_options(f)
    if not o.get("compiler", "").startswith("ee-gcc"):
        continue
    sym = st.at_addr(a)
    try:
        scdiff.compile_obj(f, tmp / "f.o", o["compiler"], o["flags"].split() + extra)
        ok = scdiff.compare(tmp / "f.o", sym.name, sym, img, st).match
    except Exception:
        ok = False
    if ok:
        kept += 1
    else:
        lost += 1
        lost_at.append(f"{a:08X}")
print(f"{kept} still match, {lost} lost with {' '.join(extra)}" + (": " + " ".join(lost_at[:20]) if lost_at else ""))
