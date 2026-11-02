#!/usr/bin/env python3
"""tools/unitscan.py SRC [0xLO 0xHI] -- where could one hand-written unit be two?

Compiles SRC three ways: master's flags (A), the auto_inline pins stripped (B), and with no
auto-inlining at all (C: -inline deferred,bottomup,level=5).
  pinned pairs  (F, X): F calls own X in A more than in B   -- the calls the pins exist for
  inlined pairs (F, X): F calls own X in C more than in A   -- X inlined into F in master
Then every map row start S in the unit's range is a candidate cut: a pinned pair it separates is
explained, an inlined pair it separates is impossible (auto-inlining never crosses units). Also
prints the TARGET's own calls between the range's rows (what the image itself keeps out of line).
A cut it allows is only a candidate: README "The inlining mode, measured" lists the checks that
decide one (the template tail, the initialisers, SOCOM 1's asserts and calls). Run from decomp/:

    ./tools/unitscan.py src/gamez/znode/int_main.cpp 0x2DBFB0 0x2DDC30
"""
import re
import sys
from collections import Counter
from pathlib import Path

DEC = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(DEC))
import scdecomp as E   # noqa: E402
import scdiff as SD    # noqa: E402

OUT = DEC / "build" / "scratch" / "unitscan"
NOAUTO = "-O4,p -sdatathreshold 0 -inline deferred,bottomup,level=5 -enum min -Cpp_exceptions off".split()
PIN = re.compile(r"^\s*#pragma\s+auto_inline\s+(off|reset)\b.*$", re.M)


def calls(obj):
    elf = E.Elf.read(obj)
    out = {}
    for f in elf.functions():
        c = Counter(r.sym.name for r in elf.relocs_for(f.shndx)
                    if f.value <= r.offset < f.value + f.size and r.type == "R_MIPS_26")
        out[f.name] = (c, f.bind)
    return out


def main():
    src = Path(sys.argv[1]).resolve()
    st, img = E.symbols(), E.image()
    inc = [DEC / "include", DEC / "src", src.parent]
    OUT.mkdir(parents=True, exist_ok=True)
    oa, ob, oc = OUT / (src.stem + ".A.o"), OUT / (src.stem + ".B.o"), OUT / (src.stem + ".C.o")
    SD.compile_obj(src, oa, SD.COMPILER, None, inc)
    text = src.read_text(errors="replace")
    nopin = OUT / ("nopin_" + src.name)    # outside src/: the source's directory is on -I
    nopin.write_text(PIN.sub("", text))
    SD.compile_obj(nopin, ob, SD.COMPILER, None, inc)
    SD.compile_obj(src, oc, SD.COMPILER, NOAUTO, inc)
    ca, cb, cc = calls(oa), calls(ob), calls(oc)
    own = {}
    for name, (_, bind) in ca.items():
        if bind == 13:
            continue
        try:
            own[name] = st.get(name).addr
        except KeyError:
            pass
    if len(sys.argv) > 3:
        lo, hi = int(sys.argv[2], 16), int(sys.argv[3], 16)
    else:
        lo, hi = min(own.values()), max(own.values()) + 1
    pinned, inlined = [], []
    for f, (c, _) in ca.items():
        if f not in own:
            continue
        for x, k in c.items():
            if x in own and x != f and cb.get(f, (Counter(), 0))[0][x] < k:
                pinned.append((f, x))
        for x, k in cc.get(f, (Counter(), 0))[0].items():
            if x in own and x != f and c[x] < k:
                inlined.append((f, x))
    rows = [s for s in st.symbols if lo <= s.addr < hi]
    rows.sort(key=lambda s: s.addr)
    name_at = {s.addr: s.name for s in rows}

    def short(n):
        return n if len(n) < 60 else n[:57] + "..."
    print(f"{src.name}: own {len(own)} functions in the map, range 0x{lo:X}-0x{hi:X}")
    print(f"pinned pairs ({len(pinned)}):")
    for f, x in pinned:
        print(f"   0x{own[f]:X} {short(f)} -> 0x{own[x]:X} {short(x)}")
    print(f"inlined pairs ({len(inlined)}):")
    for f, x in inlined:
        print(f"   0x{own[f]:X} {short(f)} -> 0x{own[x]:X} {short(x)}")
    # the target's own calls within the range
    tcalls = []
    for s in rows:
        b = img.read(s.addr, s.size)
        for off in range(0, len(b) - 3, 4):
            w = int.from_bytes(b[off:off + 4], "little")
            if w >> 26 in (2, 3):
                t = ((s.addr + off + 4) & 0xF0000000) | ((w & 0x03FFFFFF) << 2)
                if lo <= t < hi and t != s.addr and t in name_at:
                    tcalls.append((s.addr, t))
    print(f"target calls within the range ({len(tcalls)}):")
    for a, t in sorted(set(tcalls)):
        print(f"   0x{a:X} {short(name_at[a])} -> 0x{t:X} {short(name_at[t])}")
    print("cuts (S = first address of the upper unit):")
    for s in rows[1:]:
        S = s.addr
        k = sum(1 for f, x in pinned if min(own[f], own[x]) < S <= max(own[f], own[x]))
        bad = [(f, x) for f, x in inlined if min(own[f], own[x]) < S <= max(own[f], own[x])]
        tk = sum(1 for a, t in set(tcalls) if min(a, t) < S <= max(a, t))
        mark = "OK " if not bad else "-- "
        if k or not bad:
            print(f"  {mark}0x{S:X} explains {k}/{len(pinned)} pins, {len(bad)} inlined pairs cut,"
                  f" {tk} target calls cross  {short(s.name)}")


if __name__ == "__main__":
    main()
