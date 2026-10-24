#!/usr/bin/env python3
"""
Carry SOCOM 1's symbol names onto SOCOM II, out of the May 13 2002 demo.

    ./tools/s1names.py                 # report what would be named, and how
    ./tools/s1names.py --write         # rename placeholders in symbols.txt (provenance in the note)
    ./tools/s1names.py --compare CSV   # agreement with another name table (Address,Name,Mangled,...)

The demo (extracted/socom1-demo-020513/SCUS_972.05, sha256 00c9cee7...) is unstripped:
9,703 named, sized functions, and `.relmain`, every relocation the linker applied. That
last part is what makes a match exact rather than plausible:

  exact     same size; identical once the fields the DEMO's relocations name are masked on
            both sides -- and nothing else is. Struct offsets, stack offsets and constants
            stay compared, so MakeXRot cannot be mistaken for MakeYRot.
  callee    a placeholder SOCOM II function called from the same instruction of an exact
            pair takes the demo callee's name (iterated: exact pairs only).
  calls     the same sequence of known callees and the same referenced strings, unique in
            both builds, informative (two known callees, or a string of 6+ characters), sizes
            within 2x: what a function does survives the compiler change that rewrote its
            bytes. Iterated, so each round's names inform the next.
  syscall   a kernel stub (li v1,N; syscall) takes the name of the demo's stub for N.
  gap       between two consecutive exact anchors (of the longest chain increasing in both
            builds' addresses), the same number of functions on each side with every pair's
            sizes within 35 %: paired in order. Units keep their functions' order across the
            builds; this is how CQuat's ToMatrix..MakeYXRot were named by hand first.

A name that would land on two SOCOM II addresses (an inline's out-of-line copies, template
instances) is withheld: the map needs unique names; the rows get an `s1~` note instead.
Names are written only over placeholders, and every one says how it was found. A row
hand-written source defines, or one already carrying an `s1~` note (a name judged and
refused), is never touched.
"""

from __future__ import annotations

import argparse
import csv
import re
import struct
import sys
from collections import defaultdict
from pathlib import Path

HERE = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(HERE))
sys.path.insert(0, str(HERE / "tools"))
import scdecomp as E
from beta_tu import sig

DEMO = E.ROOT / "extracted" / "socom1-demo-020513" / "SCUS_972.05"
DEMO_SHA = "00c9cee7f75b921fda1e848d04f64881b5a0b7841300591cac91371bb23d4f8a"
FIELD = {"R_MIPS_26": 0x03FFFFFF, "R_MIPS_HI16": 0xFFFF, "R_MIPS_LO16": 0xFFFF,
         "R_MIPS_GPREL16": 0xFFFF, "R_MIPS_32": 0xFFFFFFFF, "?": 0xFFFF}


class Demo:
    def __init__(self, path: Path = DEMO):
        import hashlib
        raw = path.read_bytes()
        if hashlib.sha256(raw).hexdigest() != DEMO_SHA:
            raise SystemExit(f"{path}: not the May 13 2002 demo this was measured on")
        self.elf = E.Elf(raw, str(path))
        main = next(s for s in self.elf.sections if s.name == "main")
        self.base, self.data, self.sec = main.addr, main.data, main.index
        self.funcs = sorted((s for s in self.elf.symbols if s.type == "FUNC" and s.size and s.shndx == main.index),
                            key=lambda s: s.value)
        self.name_at = {f.value: f.name for f in self.funcs}
        # address -> relocation type. r_offset is already a VIRTUAL ADDRESS in this executable,
        # not an offset into `main` (adding the base put every mask 1 MB away from its field --
        # test_scdecomp checks each R_MIPS_26 lands on a j/jal). Type 123 (ten of them, on beq)
        # is Metrowerks' own: a branch displacement, masked like the 16-bit fields.
        self.relocs: dict[int, str] = {}
        rel = next(s for s in self.elf.sections if s.name == ".relmain")
        for j in range(rel.size // 8):
            off, info = struct.unpack_from("<II", rel.data, j * 8)
            self.relocs[off] = E.R_MIPS.get(info & 0xFF, "?")

    def read(self, a: int, n: int) -> bytes:
        return self.data[a - self.base:a - self.base + n]

    def mask_of(self, a: int, n: int) -> list[int]:
        return [FIELD.get(self.relocs.get(a + i, ""), 0) for i in range(0, n, 4)]


def masked(b: bytes, mask: list[int]) -> tuple:
    ws = struct.unpack(f"<{len(b) // 4}I", b[:len(b) // 4 * 4])
    return tuple(w & ~m & 0xFFFFFFFF for w, m in zip(ws, mask))


def run(demo: Demo, img: E.Image, st: E.SymbolTable):
    # candidates by (size, opcode skeleton), confirmed under the demo's exact relocation mask
    ours = defaultdict(list)
    for s in st.symbols:
        seg = img.segment(s.addr)
        if seg is None or s.size < 16 or not (img.is_code(s.addr) or seg.name == "MAIN"):
            continue
        ours[(s.size, sig(img.read(s.addr, s.size)))].append(s)
    # unique BOTH ways: one SOCOM II function for the demo function, and one demo name for the
    # SOCOM II function -- a `return g(x);` wrapper masks to the same words as every other one
    hit_by: dict[int, list] = defaultdict(list)    # SOCOM II address -> demo functions that hit it alone
    for f in demo.funcs:
        if f.size < 16:
            continue
        b = demo.read(f.value, f.size)
        cands = ours.get((f.size, sig(b)), [])
        if not cands:
            continue
        m = demo.mask_of(f.value, f.size)
        want = masked(b, m)
        hits = [s for s in cands if masked(img.read(s.addr, s.size), m) == want
                and not callee_conflict(demo, img, st, f.value, s.addr, s.size)]
        if len(hits) == 1:
            hit_by[hits[0].addr].append(f)
    exact: dict[int, str] = {}                     # SOCOM II address -> demo name
    demo_of: dict[int, int] = {}                   # SOCOM II address -> demo address
    for addr, fs in hit_by.items():
        if len({f.name for f in fs}) == 1:
            exact[addr] = fs[0].name
            demo_of[addr] = fs[0].value
    # callees, from identical call sites of exact pairs, iterated
    # callees, from identical call sites of exact pairs, iterated; a target proposed two
    # different names by two call sites is withheld
    names_at = {f.value: f.name for f in demo.funcs}
    size_at = {f.value: f.size for f in demo.funcs}
    proposed: dict[int, set] = defaultdict(set)
    frontier = dict(demo_of)
    seen = set(frontier)
    while frontier:
        nxt = {}
        for ours_a, demo_a in frontier.items():
            size = st.at_addr(ours_a).size
            for i in range(0, size, 4):
                w1 = img.word(ours_a + i)
                w2 = struct.unpack_from("<I", demo.data, demo_a - demo.base + i)[0]
                if w1 >> 26 != 3 or w2 >> 26 != 3:
                    continue
                t1 = ((ours_a + i + 4) & 0xF0000000) | ((w1 & 0x3FFFFFF) << 2)
                t2 = ((demo_a + i + 4) & 0xF0000000) | ((w2 & 0x3FFFFFF) << 2)
                n = names_at.get(t2)
                if not n or t1 in exact or st.at_addr(t1) is None:
                    continue
                proposed[t1].add((n, t2))
                # recurse only through a callee of the same size: its call sites line up too
                if t1 not in seen and st.at_addr(t1).size == size_at.get(t2):
                    nxt[t1] = t2
                    seen.add(t1)
        frontier = nxt
    callee = {t: next(iter(ns))[0] for t, ns in proposed.items() if len({n for n, _ in ns}) == 1}
    return exact, callee, demo_of


def callee_conflict(demo: Demo, img: E.Image, st: E.SymbolTable, demo_a: int, ours_a: int, size: int) -> bool:
    """A call at the same instruction whose target BOTH builds name, differently: masked words
    cannot tell `return f(x);` from `return g(x);`, but the names can. Only ever rejects."""
    for i in range(0, size, 4):
        w1, w2 = img.word(ours_a + i), struct.unpack_from("<I", demo.data, demo_a - demo.base + i)[0]
        if w1 >> 26 not in (2, 3) or w2 >> 26 not in (2, 3):
            continue
        t1 = ((ours_a + i + 4) & 0xF0000000) | ((w1 & 0x3FFFFFF) << 2)
        t2 = ((demo_a + i + 4) & 0xF0000000) | ((w2 & 0x3FFFFFF) << 2)
        s1, n2 = st.at_addr(t1), demo.name_at.get(t2)
        if s1 is not None and n2 and not E.is_placeholder(s1.name) and s1.name != n2:
            return True
    return False


def lis(pairs: list[tuple[int, int]]) -> list[tuple[int, int]]:
    """The longest chain increasing in BOTH addresses (pairs sorted by the first)."""
    import bisect
    tails, idx, prev = [], [], [-1] * len(pairs)
    for i, (_, d) in enumerate(pairs):
        k = bisect.bisect_left(tails, d)
        if k == len(tails):
            tails.append(d); idx.append(i)
        else:
            tails[k] = d; idx[k] = i
        prev[i] = idx[k - 1] if k else -1
    out, i = [], idx[-1] if idx else -1
    while i >= 0:
        out.append(pairs[i]); i = prev[i]
    return out[::-1]


def gaps(demo: Demo, img: E.Image, st: E.SymbolTable, demo_of: dict[int, int],
         max_gap: int = 24, ratio: float = 1.35) -> dict[int, str]:
    """Between two consecutive anchors of the increasing chain, equal counts of functions on
    both sides, every pair's sizes within `ratio` of each other: pair them in order."""
    ours = [s for s in st.symbols if img.segment(s.addr) is not None and s.size >= 8
            and (img.is_code(s.addr) or img.segment(s.addr).name == "MAIN")]
    ours_addrs = [s.addr for s in ours]
    dem = demo.funcs
    dem_addrs = [f.value for f in dem]
    import bisect
    chain = lis(sorted(demo_of.items()))
    out = {}
    for (a1, d1), (a2, d2) in zip(chain, chain[1:]):
        i1, i2 = bisect.bisect_right(ours_addrs, a1), bisect.bisect_left(ours_addrs, a2)
        j1, j2 = bisect.bisect_right(dem_addrs, d1), bisect.bisect_left(dem_addrs, d2)
        S, D = ours[i1:i2], dem[j1:j2]
        if not S or len(S) != len(D) or len(S) > max_gap:
            continue
        if all(max(x.size, y.size) <= ratio * min(x.size, y.size) for x, y in zip(S, D)):
            for x, y in zip(S, D):
                if min(x.size, y.size) >= 0x20:   # stubs and one-liners are positionally ambiguous
                    out[x.addr] = y.name
    return out


def _features(read_word, start: int, size: int, callee_name, string_at) -> tuple:
    """(callee tokens in call order, referenced strings in order): what a function DOES,
    which survives a compiler change that rewrites its bytes."""
    calls, strs = [], []
    live = {}
    for a in range(start, start + size, 4):
        w = read_word(a)
        op = w >> 26
        if op == 3:
            calls.append(callee_name(((a + 4) & 0xF0000000) | ((w & 0x3FFFFFF) << 2)))
        elif op == 0x0F:
            live[(w >> 16) & 31] = w & 0xFFFF
        elif op == 0x09 and (w >> 21) & 31 in live:
            lo = w & 0xFFFF
            t = ((live[(w >> 21) & 31] << 16) + (lo - 0x10000 if lo & 0x8000 else lo)) & 0xFFFFFFFF
            st_ = string_at(t)
            if st_:
                strs.append(st_)
    return tuple(calls), tuple(strs)


def _cstring(read, addr: int) -> str | None:
    try:
        b = read(addr, 64)
    except Exception:
        return None
    if not b:
        return None
    end = b.find(b"\0")
    if end < 4:
        return None
    t = b[:end]
    if all(32 <= c < 127 for c in t):
        return t.decode()
    return None


def ra_first(b: bytes) -> bool | None:
    """Is ra the first register the prologue saves (Metrowerks), or not (GCC)? None for a
    leaf that saves nothing."""
    for w in struct.unpack(f"<{len(b) // 4}I", b[:len(b) // 4 * 4]):
        if w >> 26 in (0x3F, 0x1F, 0x2B) and (w >> 21) & 31 == 29:     # sd / sq / sw  x,(sp)
            return (w >> 16) & 31 == 31
    return None


def calls_pass(demo: Demo, img: E.Image, st: E.SymbolTable, named: dict[int, str],
               rounds: int = 6) -> dict[int, str]:
    """Pair unnamed SOCOM II functions with demo functions doing the same thing: the same
    sequence of known callees and the same referenced strings, unique on both sides, at
    least two known callees or one string of 6+ characters, sizes within 2x."""
    names = {s.addr: s.name for s in st.symbols if not E.is_placeholder(s.name)}
    names.update(named)
    demo_name_at = {f.value: f.name for f in demo.funcs}
    dwrd = lambda a: struct.unpack_from("<I", demo.data, a - demo.base)[0]
    dread = lambda a, n: demo.read(a, n)
    def iread(a, n):
        if not img.covers(a, n):
            return b""
        return img.read(a, n)
    found = {}
    for _ in range(rounds):
        known = set(names.values()) | set(found.values())
        dfeat = defaultdict(list)
        for f in demo.funcs:
            if f.name in known or f.size < 0x20:
                continue
            key = _features(dwrd, f.value, f.size,
                            lambda t: demo_name_at.get(t) if demo_name_at.get(t) in known else "?",
                            lambda t: _cstring(dread, t))
            dfeat[key].append(f)
        ofeat = defaultdict(list)
        cur = {**names, **found}
        for sym in st.symbols:
            if sym.addr in cur or sym.size < 0x20 or not img.is_code(sym.addr):
                continue
            key = _features(img.word, sym.addr, sym.size, lambda t: cur.get(t, "?"),
                            lambda t: _cstring(iread, t))
            ofeat[key].append(sym)
        new = {}
        for key, ds in dfeat.items():
            calls, strs = key
            # two DISTINCT known callees: one function called twice (DeleteSema, DeleteSema)
            # paired the memory card library's end with a voice server's shutdown
            informative = len({c for c in calls if c != "?"}) >= 2 or any(len(x) >= 6 for x in strs)
            if not informative or len(ds) != 1 or len(ofeat.get(key, [])) != 1:
                continue
            o, d = ofeat[key][0], ds[0]
            if max(o.size, d.size) > 2 * min(o.size, d.size):
                continue
            if ra_first(img.read(o.addr, min(o.size, 64))) != ra_first(demo.read(d.value, min(d.size, 64))):
                continue                    # Metrowerks saves ra first, GCC does not: other code
            new[o.addr] = d.name
        if not new:
            break
        found.update(new)
    return found


def syscall_no(b: bytes) -> int | None:
    """`li v1,N; syscall` (in either order with a jr ra): the stub's syscall number."""
    ws = struct.unpack(f"<{len(b) // 4}I", b)
    if len(ws) < 2 or not any(w & 0xFC00003F == 0x0000000C for w in ws):
        return None
    for w in ws:
        if w >> 16 in (0x2403, 0x3403):             # addiu/ori v1, zero, N
            return w & 0xFFFF
    return None


def syscalls(demo: Demo, img: E.Image, st: E.SymbolTable) -> dict[int, str]:
    """Kernel stubs, named by the syscall number the demo's stub of that name uses."""
    by_no = defaultdict(set)
    for f in demo.funcs:
        if f.size <= 0x10:
            n = syscall_no(demo.read(f.value, f.size))
            if n is not None:
                by_no[n].add(f.name)
    out = {}
    for s in st.symbols:
        if s.size <= 0x10 and img.segment(s.addr) is not None and img.segment(s.addr).name == "MAIN":
            n = syscall_no(img.read(s.addr, s.size))
            if n is not None and len(by_no.get(n, ())) == 1:
                out[s.addr] = next(iter(by_no[n]))
    return out


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--write", action="store_true")
    ap.add_argument("--compare", type=Path)
    a = ap.parse_args(argv)
    img, st, demo = E.image(), E.symbols(), Demo()
    exact, callee, demo_of = run(demo, img, st)
    gap = gaps(demo, img, st, {a: d for a, d in demo_of.items()})
    sysc = syscalls(demo, img, st)
    allp = {**gap, **callee, **sysc, **exact}
    behav = calls_pass(demo, img, st, allp)
    allp = {**behav, **allp}
    by_name = defaultdict(list)
    for addr, n in allp.items():
        by_name[n].append(addr)
    unique = {addr: n for addr, n in allp.items() if len(by_name[n]) == 1}
    multi = {addr: n for addr, n in allp.items() if len(by_name[n]) > 1}
    taken = {s.name for s in st.symbols if not E.is_placeholder(s.name)}
    # a placeholder hand-written source defines stays put: renaming the row would orphan the
    # definition (the CalcWorldToUV lesson -- a behavioural name carries SOCOM 1's signature)
    from automatch import hand_defined
    hand = hand_defined()
    print(f"demo: {len(demo.funcs)} functions, {len(demo.relocs)} relocations")
    print(f"exact {len(exact)}, callee {len(callee)}, syscall {len(set(sysc) - set(exact))}, "
          f"gap {len(set(gap) - set(exact) - set(callee) - set(sysc))}, calls {len(behav)}; "
          f"{len(unique)} unique names, "
          f"{len(multi)} rows share a name (withheld)")
    if a.compare:
        other = {int(r["Address"], 16): r.get("Mangled") or r["Name"] for r in csv.DictReader(open(a.compare))}
        both = [x for x in unique if x in other]
        agree = sum(1 for x in both if other[x] == unique[x])
        print(f"vs {a.compare.name}: {len(both)} addresses both name, {agree} agree, "
              f"{len(unique) - len(both)} new here, {len(set(other) - set(unique))} only there")
    if a.write:
        text = E.SYMBOLS.read_text().splitlines()
        named = withheld = upgraded = 0
        for i, line in enumerate(text):
            m = re.match(r"^(0x[0-9A-Fa-f]+)\s+(\S+)\s+(\S+)(.*)$", line)
            if not m:
                continue
            addr, cur, note = int(m.group(1), 16), m.group(3), m.group(4)
            sep = "; " if "#" in note else "  # "
            if cur in hand or "s1~" in note:
                continue
            weaker = re.search(r"s1:(gap|calls|callee|order)\b", note)
            if weaker and exact.get(addr) == cur:        # the same name, now on the strongest evidence
                text[i] = line.replace(weaker.group(0), "s1:exact")
                upgraded += 1
                continue
            if addr in unique and E.is_placeholder(cur) and unique[addr] not in taken:
                how = ("exact" if addr in exact else "syscall" if addr in sysc
                       else "callee" if addr in callee else "gap" if addr in gap else "calls")
                text[i] = f"{m.group(1)} {m.group(2)} {unique[addr]}{note}{sep}s1:{how}"
                named += 1
            elif addr in multi and E.is_placeholder(cur) and "s1~" not in note:
                text[i] = f"{line}{sep}s1~ {multi[addr]} (one of {len(by_name[multi[addr]])})"
                withheld += 1
        E.SYMBOLS.write_text("\n".join(text) + "\n")
        print(f"named {named} placeholders, noted {withheld} shared names, "
              f"{upgraded} names now confirmed exact")
    return 0


if __name__ == "__main__":
    sys.exit(main())
