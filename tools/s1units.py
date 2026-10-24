#!/usr/bin/env python3
"""
Name SOCOM II's translation units: its static initialisers against SOCOM 1's named ones.

    ./tools/s1units.py                 # the aligned list
    ./tools/s1units.py --write         # add the initialisers to symbols.txt as __sinit_<file>

Metrowerks gives every unit with dynamic initialisation a function `__sinit_<file.cpp>` and
puts a pointer to it in the overlay's initialiser table, in LINK ORDER -- the MWo3 header's
ctors..ctors_end (OVL1: 125 entries, OVL2: 16). The thunks sit in the data area after the
code, so the function sweep never saw them. The May 2002 demo keeps the names: 296
`__sinit_*` in its own link order. A unit's initialiser constructs that unit's globals, so it
calls the same constructors and touches the same globals in both builds.

Each SOCOM II thunk is scored against each demo initialiser on what both maps name -- the
callees and the globals (a thunk constructing CQuat's identity and zero is zmath_main.cpp's)
-- and the two lists are aligned in order (a dynamic programme over the score, as the gap
pass pairs functions between anchors). A pair is kept when it scores, and its neighbours
agree with the order; a thunk that only builds the iostream sentries (`std::__nInit`, which
every unit including <iostream> has) is named only when the order pins it between named ones.
"""

from __future__ import annotations

import argparse
import difflib
import struct
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(HERE))
sys.path.insert(0, str(HERE / "tools"))
import scdecomp as E
import s1names as N
import s1data as S

GENERIC = {"__register_global_object", "__ct__Q23std7__nInitFv", "__ct__Q23std7__wInitFv",
           "__dt__Q23std7__nInitFv", "__dt__Q23std7__wInitFv", "__msl_ios_base_ninit__3std",
           "__msl_ios_base_winit__3std", "__construct_array"}


def our_thunks(img: E.Image) -> list[tuple[str, int, int]]:
    """(segment, address, size) of each overlay's initialisers, in table (link) order."""
    out = []
    for fname, seg in (("ftscore.bin", "OVL1"), ("zsealetc.bin", "OVL2")):
        b = (E.ROOT / "extracted" / "overlays" / fname).read_bytes()
        h = E.MWo3.parse(b)
        ptrs = [struct.unpack_from("<I", b, h.ctors - h.base + 4 * i)[0]
                for i in range((h.ctors_end - h.ctors) // 4)]
        ends = sorted(ptrs) + [h.ctors]
        for p in ptrs:
            n = ends[ends.index(p) + 1] - p
            while n > 8 and img.word(p + n - 4) == 0 and img.word(p + n - 8) != E.JR_RA:
                n -= 4                                   # the zero padding to the next one
            out.append((seg, p, n))
    return out


def features(word, start: int, size: int, gp, callee_name, data_name) -> set[str]:
    f = set()
    for a in range(start, start + size, 4):
        w = word(a)
        if w >> 26 in (2, 3):
            t = ((a + 4) & 0xF0000000) | ((w & 0x3FFFFFF) << 2)
            n = callee_name(t)
            if n:
                f.add("call:" + n)
    for _, t, _ in S.data_refs(word, start, size, gp):
        n = data_name(t)
        if n:
            f.add("data:" + n)
    return f


def skeleton(b: bytes) -> tuple:
    """Opcodes with every immediate, offset and target masked; $gp-relative and lui-pair
    accesses differ in count between the builds, so lui itself is dropped."""
    from beta_tu import maskw
    return tuple(maskw(w) for w in struct.unpack(f"<{len(b) // 4}I", b) if w >> 26 != 0x0F and w)


def score(a: set[str], b: set[str]) -> float:
    sa, sb = a - {"call:" + g for g in GENERIC} - {"data:" + g for g in GENERIC}, \
        b - {"call:" + g for g in GENERIC} - {"data:" + g for g in GENERIC}
    if not sa or not sb:
        return 0.0
    return len(sa & sb) / len(sa | sb)


def align(ours: list[set[str]], theirs: list[set[str]], floor: float = 0.2,
          exact: dict[int, int] | None = None,
          shape_scores: dict[tuple[int, int], float] | None = None) -> dict[int, tuple[int, float]]:
    """Monotone alignment maximising the summed score of pairs scoring at least `floor`; an
    exact pair (identical but for relocation fields, unique both ways) scores 2."""
    exact = exact or {}
    n, m = len(ours), len(theirs)
    best = [[0.0] * (m + 1) for _ in range(n + 1)]
    move = [[0] * (m + 1) for _ in range(n + 1)]

    def pair_score(i, j):
        if exact.get(i) == j:
            return 2.0
        return max(score(ours[i], theirs[j]), shape(i, j))
    shapes = shape_scores if shape_scores is not None else {}

    def shape(i, j):
        return shapes.get((i, j), 0.0)
    for i in range(n - 1, -1, -1):
        for j in range(m - 1, -1, -1):
            s = pair_score(i, j)
            opts = [(best[i + 1][j], 1), (best[i][j + 1], 2)]
            if s >= floor:
                opts.append((best[i + 1][j + 1] + s, 3))
            best[i][j], move[i][j] = max(opts)
    out, i, j = {}, 0, 0
    while i < n and j < m:
        mv = move[i][j]
        if mv == 3:
            out[i] = (j, pair_score(i, j))
            i, j = i + 1, j + 1
        elif mv == 1:
            i += 1
        else:
            j += 1
    return out


def run():
    img, st, demo = E.image(), E.symbols(), N.Demo()
    ds = S.DemoSymbols(demo)
    dword = lambda a: struct.unpack_from("<I", demo.data, a - demo.base)[0]
    sinit = sorted((s.value, s.size, s.name) for s in demo.elf.symbols
                   if s.name.startswith("__sinit_") and s.type == "FUNC")
    theirs = [features(dword, v, n, ds.gp, demo.name_at.get,
                       lambda t: (ds.resolve(t) or (None,))[0]) for v, n, _ in sinit]

    def our_callee(t):
        s = st.at_addr(t)
        return None if s is None or E.is_placeholder(s.name) else s.name

    def our_data(t):
        d = st.data_by_addr.get(t)
        if d is not None and not E.is_placeholder(d.name):
            return d.name
        s = st.at_addr(t)                       # a destructor registered by address
        return s.name if s is not None and not E.is_placeholder(s.name) else None

    their_sk = [skeleton(demo.read(v, n)) for v, n, _ in sinit]
    thunks = our_thunks(img)
    # exact: the same size, identical once the demo's relocation fields are masked, unique
    # both ways (most initialisers are; the order then places the rest)
    hits: dict[int, list[int]] = {}
    for j, (v, n, _) in enumerate(sinit):
        b, m = demo.read(v, n), demo.mask_of(v, n)
        want = N.masked(b, m)
        for seg in ("OVL1", "OVL2"):            # a unit linked into both overlays has a copy in each
            cands = [k for k, (g, p, sz) in enumerate(thunks)
                     if g == seg and sz == n and N.masked(img.read(p, n), m) == want]
            if len(cands) == 1:
                hits.setdefault(cands[0], []).append(j)
    exact_all = {k: js[0] for k, js in hits.items() if len(js) == 1}
    rows = []
    for seg in ("OVL1", "OVL2"):
        idx = [k for k, t in enumerate(thunks) if t[0] == seg]
        mine = [thunks[k] for k in idx]
        feats = [features(img.word, p, n, E.GP, our_callee, our_data) for _, p, n in mine]
        exact = {i: exact_all[k] for i, k in enumerate(idx) if k in exact_all}
        # the name-free score: how alike the two instruction streams are with every immediate
        # and target masked (what a unit constructs survives the compiler change; the order
        # of the stores and calls mostly does too), discounted so a name match outranks it
        mine_sk = [skeleton(img.read(p, n)) for _, p, n in mine]
        shapes = {}
        for i, a_sk in enumerate(mine_sk):
            for j, b_sk in enumerate(their_sk):
                if 0.5 <= len(a_sk) / max(len(b_sk), 1) <= 2.0:
                    r = difflib.SequenceMatcher(None, a_sk, b_sk, autojunk=False).ratio()
                    if r >= 0.75:
                        shapes[(i, j)] = 0.6 * r
        got = align(feats, theirs, exact=exact, shape_scores=shapes)
        for i, (_, p, n) in enumerate(mine):
            j, sc = got.get(i, (None, 0.0))
            unit = sinit[j][2][len("__sinit_"):] if j is not None else None
            # a name needs an exact pair or shared NAMES; a shape alone is only a hint
            strong = j is not None and (sc >= 2.0 or score(feats[i], theirs[j]) >= 0.5)
            rows.append((seg, p, n, unit, sc, sorted(feats[i]), strong))
    return rows


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--write", action="store_true")
    ap.add_argument("-v", "--verbose", action="store_true")
    a = ap.parse_args(argv)
    rows = run()
    named = [r for r in rows if r[6]]
    for seg, p, n, unit, sc, feats, strong in rows:
        if unit or a.verbose:
            mark = unit if strong else (f"{unit}?" if unit else "?")
            print(f"{seg} {p:08X} {n:4X} {mark:28} {sc:.2f}  {' '.join(f[5:] for f in feats)[:100]}")
    print(f"{len(named)} of {len(rows)} initialisers named, "
          f"{sum(1 for r in rows if r[3] and not r[6])} more placed by shape only (hints)")
    if a.write:
        lines = E.SYMBOLS.read_text().rstrip("\n").splitlines()
        have = {int(l.split()[0], 16) for l in lines if l.startswith("0x")}
        st = E.symbols()
        added = 0
        for seg, p, n, unit, sc, _, strong in rows:
            if p in have:
                continue
            name = (f"__sinit_{unit}" if strong and f"__sinit_{unit}" not in st._by_name
                    and not any(r[3] == unit and r[6] and r[1] < p for r in rows)   # a second overlay's copy
                    else f"func_{p:08X}")
            how = ("exact" if sc >= 2.0 else f"names {sc:.2f}") if strong else f"shape {sc:.2f}, a hint"
            note = f"{seg} data area; a static initialiser" + (
                f"; s1:unit {unit} ({how})" if strong and name.startswith("__sinit_") else
                f"; s1~ {unit} ({how})" if unit else "")
            lines.append(f"0x{p:08X} 0x{n:X} {name}  # {note}")
            added += 1
        head = [l for l in lines if not l.startswith("0x")]
        body = sorted((l for l in lines if l.startswith("0x")), key=lambda l: int(l.split()[0], 16))
        E.SYMBOLS.write_text("\n".join(head + body) + "\n")
        print(f"added {added} rows to {E.SYMBOLS.name}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
