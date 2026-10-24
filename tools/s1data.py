#!/usr/bin/env python3
"""
Carry SOCOM 1's GLOBAL DATA names onto SOCOM II, through functions already paired by name.

    ./tools/s1data.py                  # report
    ./tools/s1data.py --write          # add rows to data.txt (and function-pointer names to symbols.txt)

A function SOCOM II shares with the May 2002 demo addresses the same globals in the same
order: theGame, a static table, a VU microprogram label. So for every pair s1names made:

  exact   the pair is identical but for relocation fields, so the k-th data reference sits
          at the same instruction on both sides: SOCOM II's address there is the demo's
          symbol there.
  seq     any other pair (gap, calls, order, callee): when both sides make the same number
          of data references with the same access widths, the k-th is paired with the k-th.
          The demo reaches small data through $gp and SOCOM II never does, so this is how
          most scalars are named.

A reference is a lui/addiu|load|store pair (or a $gp-relative access), its target the
address it forms. A target inside a demo object is taken as that object plus the same
offset. A name is written only when every proposal agrees, both ways -- one address per
name and one name per address -- and an exact pair or two pairs support it. A target that
is a demo FUNCTION (a handler or callback taken by address) names the SOCOM II function
there instead, when that row is still a placeholder.
"""

from __future__ import annotations

import argparse
import bisect
import re
import struct
import sys
from collections import defaultdict
from pathlib import Path

HERE = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(HERE))
sys.path.insert(0, str(HERE / "tools"))
import scdecomp as E
import s1names as N

ROW = re.compile(r"^(0x[0-9A-Fa-f]+)\s+(\S+)\s+(\S+)(.*)$")
LOADS = {0x20: "b", 0x24: "b", 0x21: "h", 0x25: "h", 0x23: "w", 0x27: "w", 0x37: "d", 0x1E: "q",
         0x28: "b", 0x29: "h", 0x2B: "w", 0x3F: "d", 0x1F: "q", 0x31: "f", 0x39: "f",
         0x36: "v", 0x3E: "v", 0x1A: "d", 0x1B: "d", 0x2C: "d", 0x2D: "d", 0x22: "w", 0x26: "w",
         0x2A: "w", 0x2E: "w"}


def data_refs(word, start: int, size: int, gp: int | None) -> list[tuple[int, int, str]]:
    """(offset of the using instruction, address formed, access class) in code order."""
    from m2c import gpr_written
    out, live = [], {}
    for off in range(0, size, 4):
        w = word(start + off)
        opc, rs, rt = w >> 26, (w >> 21) & 31, (w >> 16) & 31
        lo = w & 0xFFFF
        slo = lo - 0x10000 if lo & 0x8000 else lo
        if opc == 0x0F:
            live[rt] = lo
            continue
        if opc == 0x09 or opc in LOADS:
            kind = "a" if opc == 0x09 else LOADS[opc]
            if rs in live:
                out.append((off, ((live[rs] << 16) + slo) & 0xFFFFFFFF, kind))
            elif rs == 28 and gp is not None:
                out.append((off, (gp + slo) & 0xFFFFFFFF, kind))
        written = gpr_written(w)
        if written in live:
            live.pop(written, None)
        if opc == 3 or (opc == 0 and w & 0x3F == 9):       # a call clobbers the caller-saved
            for r in [r for r in live if not (16 <= r <= 23 or r in (28, 29, 30))]:
                del live[r]
    return out


class DemoSymbols:
    """The demo's named addresses: objects, labels and functions, to resolve a target."""

    def __init__(self, demo: N.Demo):
        elf = demo.elf
        reginfo = next(s for s in elf.sections if s.name == ".reginfo")
        self.gp = struct.unpack_from("<I", reginfo.data, 20)[0]
        rows = {}
        for s in elf.symbols:
            if not s.name or s.name[0] in "@.$" and not s.name.startswith("_$") or s.type == "SECTION":
                continue
            if s.type in ("OBJECT", "NOTYPE", "FUNC") and s.value:
                rows.setdefault(s.value, (s.name, s.size, s.type))
        self.addrs = sorted(rows)
        self.rows = rows

    def resolve(self, addr: int) -> tuple[str, int, int, str] | None:
        """(name, offset into it, its size, type) of the demo symbol holding `addr`."""
        i = bisect.bisect_right(self.addrs, addr) - 1
        if i < 0:
            return None
        base = self.addrs[i]
        name, size, typ = self.rows[base]
        if addr != base and not (size and addr < base + size):
            return None
        return name, addr - base, size, typ


def pairs(demo: N.Demo, img: E.Image, st: E.SymbolTable):
    """(ours, demo, exact?) for every function the map names after a demo function."""
    exact, _, demo_of = N.run(demo, img, st)
    by_name = {f.name: f for f in demo.funcs}
    out = []
    for s in st.symbols:
        if "s1:" not in s.note and "SOCOM 1 name" not in s.note:     # the first, hand-named rows
            continue
        f = by_name.get(s.name)
        if s.addr in demo_of:
            out.append((s, demo_of[s.addr], True))
        elif f is not None:
            out.append((s, f.value, False))
    return out


def by_signature(ours, theirs):
    """Pair the distinct targets of two functions by how each is used -- the sorted access
    kinds, one per reference -- not by position: two compilers order the accesses
    differently (CStack::Multiply touches its counter and its pointer in opposite orders).
    Targets whose signature is shared are paired in first-use order. None when the two
    sides' signatures do not agree as multisets."""
    def group(refs):
        uses, order = defaultdict(list), []
        for _, t, k in refs:
            if t not in uses:
                order.append(t)
            uses[t].append(k)
        return {t: tuple(sorted(uses[t])) for t in order}, order
    if not ours or len(ours) != len(theirs):
        return None
    sa, oa = group(ours)
    sb, ob = group(theirs)
    if sorted(sa.values()) != sorted(sb.values()):
        return None
    left, right = [], []
    for sigv in sorted(set(sa.values())):
        xa = [t for t in oa if sa[t] == sigv]
        xb = [t for t in ob if sb[t] == sigv]
        kind = "unique" if len(xa) == 1 else "order"      # a shared signature pairs by first use
        left += [(0, t, kind) for t in xa]
        right += [(0, t, kind) for t in xb]
    return left, right


def propose(demo: N.Demo, img: E.Image, st: E.SymbolTable):
    ds = DemoSymbols(demo)
    dword = lambda a: struct.unpack_from("<I", demo.data, a - demo.base)[0]
    votes = defaultdict(lambda: defaultdict(list))      # our base -> demo name -> [(how, from)]
    meta = {}
    for s, da, is_exact in pairs(demo, img, st):
        dsize = next(f.size for f in demo.funcs if f.value == da)
        ours = data_refs(img.word, s.addr, s.size, E.GP)
        theirs = data_refs(dword, da, dsize, ds.gp)
        if is_exact:
            if len(ours) != len(theirs) or any(a[0] != b[0] for a, b in zip(ours, theirs)):
                continue
            ours = [(o, t, "exact") for o, t, _ in ours]
        else:
            matched = by_signature(ours, theirs)
            if matched is None:
                continue
            ours, theirs = matched
        for (_, ta, how), (_, tb, _) in zip(ours, theirs):
            r = ds.resolve(tb)
            if r is None or not img.in_memory(ta):
                continue
            name, off, size, typ = r
            votes[ta - off][name].append((how, s.name))
            meta[name] = (size, typ)
    return votes, meta


def decide(votes, meta, st: E.SymbolTable):
    """Unanimous, one-to-one, and supported by an exact pair, by a target used unlike any
    other in its function, or by two functions."""
    cand = {}
    for base, names in votes.items():
        if len(names) != 1:
            continue
        (name, sup), = names.items()
        if any(h in ("exact", "unique") for h, _ in sup) or len({f for _, f in sup}) >= 2:
            cand[base] = name
    per_name = defaultdict(set)
    for base, name in cand.items():
        per_name[name].add(base)
    return {b: n for b, n in cand.items() if len(per_name[n]) == 1}


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--write", action="store_true")
    a = ap.parse_args(argv)
    img, st, demo = E.image(), E.symbols(), N.Demo()
    votes, meta = propose(demo, img, st)
    named = decide(votes, meta, st)
    funcs = {b: n for b, n in named.items() if meta[n][1] == "FUNC"}
    data = {b: n for b, n in named.items() if meta[n][1] != "FUNC"}
    taken = set(st._by_name) | set(st.data_by_name)
    print(f"{len(votes)} addresses referenced by paired functions; {len(named)} named "
          f"({len(data)} data, {len(funcs)} functions taken by address)")
    fresh_data = {b: n for b, n in data.items() if n not in taken and b not in st.data_by_addr}
    fresh_funcs = {b: n for b, n in funcs.items()
                   if n not in taken and st.at_addr(b) is not None and E.is_placeholder(st.at_addr(b).name)
                   and "s1~" not in st.at_addr(b).note}
    print(f"  new: {len(fresh_data)} data rows, {len(fresh_funcs)} function names")
    if a.write:
        rows = E.DATA.read_text().rstrip("\n").splitlines() if E.DATA.exists() else []
        for b, n in sorted(fresh_data.items()):
            seg = next((g for g in img.segments if g.base <= b < g.base + g.memsz), None)
            label = seg.name + (" bss" if b >= seg.end else "") if seg else "?"
            hows = {h for h, _ in votes[b][n]}
            how = "exact" if "exact" in hows else "seq"
            users = sorted({f for _, f in votes[b][n]})
            rows.append(f"0x{b:08X} 0x{meta[n][0]:X} {n}  # {label}; s1:data-{how} "
                        f"(size SOCOM 1's; via {users[0]}{f' +{len(users) - 1}' if len(users) > 1 else ''})")
        head = [r for r in rows if not r.startswith("0x")]
        body = sorted((r for r in rows if r.startswith("0x")), key=lambda r: int(r.split()[0], 16))
        E.DATA.write_text("\n".join(head + body) + "\n")
        if fresh_funcs:
            import rename
            rename.rename([(b, n, f"s1:ref (taken by address where SOCOM 1 takes {n}'s)")
                           for b, n in sorted(fresh_funcs.items())])
        print(f"wrote {len(fresh_data)} rows to {E.DATA.name}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
