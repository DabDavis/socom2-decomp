#!/usr/bin/env python3
"""ovlcopies -- find the other overlays' copies of named functions.

Each of MAIN, OVL1 and OVL2 is linked on its own, so a template instance or an inline's
out-of-line copy exists once per overlay that uses it. The map keeps one name per function and
marks every other copy with an `ovl-copy: <name>` note (README "Each overlay links its own
template copies"); sccheck then scores the instance a unit emits against the copy in the unit's
own overlay. This tool finds the copies nobody has marked: placeholder rows whose words equal a
named row's in another overlay, once the address fields a link fills in are masked (jal/j
targets, lui immediates, the %lo offsets of loads, stores and addius off a lui'd register, and
gp-relative offsets). Branch offsets are relative, so they must agree.

    ./tools/ovlcopies.py                          # list the candidates
    ./tools/ovlcopies.py --min-size 0x20          # (default 0x18: shorter bodies coincide too often)
    ./tools/ovlcopies.py --apply 0x53FAD0,0x5410C0  # note the rows you have vetted

THE WORDS DO NOT PROVE THE INSTANCE. Template instances for different types compile to one body
(a pointer vector's end() for every T, a pod vector<unsigned int> and vector<int>, every
__vector_imp destructor of a trivially destructible T), and look-alike accessors coincide too
(CAiMaps::GetMapByIndex has CEntityList::GetEntityByIndex's 0x70 bytes; 0x50FFE0, the
CAiEventCallback list's destructor, has ~CWepCallbackList's). The first --apply-everything run
marked 32 rows and 24 of them had to be taken back. So --apply takes the addresses you vetted: an
instance that exists once in the program (basic_string<char>'s members, the pointer lists' shared
list_deleter<const void *> base), or a copy whose callers in its overlay call it as the named
function.
"""
from __future__ import annotations

import argparse
import re
import sys
from collections import defaultdict
from pathlib import Path

HERE = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(HERE))
import scdecomp as E  # noqa: E402

LO_OPS = {0x09, 0x0F, 0x20, 0x21, 0x23, 0x24, 0x25, 0x27, 0x28, 0x29, 0x2B, 0x31, 0x37, 0x39,
          0x3F, 0x1E, 0x1F, 0x1A, 0x1B, 0x0D, 0x0C}   # addiu, lui, lb/lh/lw/lbu/lhu/lwu, sb/sh/sw,
                                                    # lwc1, ld, swc1, sd, lq/sq, ldl/ldr, ori, andi
MEM_OPS = LO_OPS - {0x0F, 0x0D, 0x0C}


def masked(words: list[int]) -> tuple[int, ...]:
    out, hi = [], set()
    for w in words:
        op = w >> 26
        rs, rt = (w >> 21) & 31, (w >> 16) & 31
        if op in (2, 3):                       # j, jal
            w &= ~0x03FFFFFF
        elif op == 0x0F:                       # lui
            hi.add(rt)
            w &= ~0xFFFF
        elif op in MEM_OPS and (rs in hi or rs == 28):
            w &= ~0xFFFF
        elif op == 0x0D and rs in hi:          # ori off a lui'd register (an address's low half)
            w &= ~0xFFFF
        out.append(w)
    return tuple(out)


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--min-size", type=lambda s: int(s, 0), default=0x18)
    ap.add_argument("--apply", metavar="ADDRS",
                    help="comma-separated vetted copy addresses whose ovl-copy notes to append")
    a = ap.parse_args(argv)
    img, st = E.image(), E.symbols()
    has_note = re.compile(r"ovl-copy: ")
    reviewed = re.compile(r"ovl-copy: |tools/ovlcopies\.py|not an overlay copy")   # noted or looked at
    groups: dict[tuple, list] = defaultdict(list)
    for s in st.symbols:
        if s.size < a.min_size:
            continue
        try:
            words = E.words(img.read(s.addr, s.size))
        except Exception:
            continue
        groups[(s.size, masked(words))].append(s)
    cand = []
    for (size, _), members in groups.items():
        named = [s for s in members if not E.is_placeholder(s.name)]
        if len(named) != 1:
            continue                           # none to copy, or two names for one body: by hand
        n = named[0]
        per_ovl = defaultdict(list)
        for s in members:
            if s is n or not E.is_placeholder(s.name) or reviewed.search(s.note or ""):
                continue
            if st.overlay_of(s.addr) == st.overlay_of(n.addr):
                continue                       # two copies in one overlay: not a link's doing
            per_ovl[st.overlay_of(s.addr)].append(s)
        for ovl, ss in per_ovl.items():
            # more than one twin in an overlay: different instances with one body (a pointer
            # vector's end() for every T) -- which one is this name's is not the words' to say
            if len(ss) == 1:
                cand.append((ss[0], n))
    copy_of = {s.addr: n.addr for s, n in cand}
    for s in st.symbols:
        m = has_note.search(s.note or "") and re.search(r"ovl-copy: (\S+)", s.note)
        if m:
            a0 = st.address_of(m.group(1))
            if a0 is not None:
                copy_of[s.addr] = a0

    def calls(sym):
        out = []
        for i, w in enumerate(E.words(img.read(sym.addr, sym.size))):
            if (w >> 26) in (2, 3):
                out.append(((sym.addr + 4 * i) & 0xF0000000) | ((w & 0x03FFFFFF) << 2))
        return out

    def same_callee(x, y):
        # the copy calls the named body's callee, or its own overlay's copy of it
        if x == y or copy_of.get(y) == x or copy_of.get(x) == y:
            return True
        sx, sy = st.at_addr(x), st.at_addr(y)
        return (sx is not None and sy is not None and sx.size == sy.size
                and masked(E.words(img.read(x, sx.size))) == masked(E.words(img.read(y, sy.size))))

    found = []
    for s, n in cand:
        cs, cn = calls(s), calls(n)
        if len(cs) == len(cn) and all(same_callee(x, y) for x, y in zip(cn, cs)):
            found.append((s, n))
        else:
            print(f"  skipped {s.addr:#010x} (= {n.name[:60]}): its calls differ", file=sys.stderr)
    found.sort(key=lambda p: p[0].addr)
    for s, n in found:
        print(f"{s.addr:#010x} {s.size:#6x} {st.overlay_of(s.addr)} = {n.addr:#010x} "
              f"{st.overlay_of(n.addr)} {n.name}")
    print(f"{len(found)} unmarked copies", file=sys.stderr)
    if a.apply and found:
        vetted = {int(x, 16) for x in a.apply.split(",") if x.strip()}
        want = {s.addr: n for s, n in found if s.addr in vetted}
        missing = vetted - set(want)
        if missing:
            print("not candidates (or already noted): " + ", ".join(f"{x:#x}" for x in sorted(missing)),
                  file=sys.stderr)
        path = st.path
        out = []
        for line in path.read_text().splitlines(keepends=True):
            m = re.match(r"0x([0-9A-Fa-f]+)\s", line)
            if m and int(m.group(1), 16) in want:
                n = want[int(m.group(1), 16)]
                body = line.rstrip("\n")
                sep = "; " if "#" in body else "  # "
                line = (f"{body}{sep}{st.overlay_of(int(m.group(1), 16))}'s copy of {n.name} "
                        f"({n.addr:#x}, the same words bar the link's addresses -- tools/ovlcopies.py); "
                        f"ovl-copy: {n.name}\n")
            out.append(line)
        path.write_text("".join(out))
        print(f"noted {len(want)} rows in {path.name}", file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
