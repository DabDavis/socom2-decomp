#!/usr/bin/env python3
"""
Name virtual functions through the vtables: SOCOM II's against SOCOM 1's, slot by slot.

    ./tools/s1vtables.py                 # report: names, contradictions, withheld
    ./tools/s1vtables.py --holdout       # hide each name the map already has; is it recovered?
    ./tools/s1vtables.py --write         # name functions (symbols.txt) and the tables (data.txt)

A Metrowerks vtable is `{RTTI *, 0, slot, slot, ...}` and its RTTI object `{const char *name,
bases}` -- the name the plain class name (`zdb::CCell`). So every SOCOM II vtable can be found
and named without symbols: a data word pointing at an RTTI object whose first word points at a
class name, a zero, then code pointers. The demo keeps `__vt__<class>` for 248 classes.

Slot position alone is not enough. SOCOM II inserted virtuals (CNode gained one after
FindChild) and dropped others, and an insertion plus a drop between two known slots leaves a
run of equal length that pairs every slot wrongly -- the C2D family does exactly that. So the
two tables are aligned by a dynamic programme: a slot whose function the map already names
pairs with the demo's slot of that name (an anchor); any other pair scores its instruction-
skeleton similarity (sizes within 2x, at least `--floor`); skipping costs nothing. A pair is
proposed only when it is forced: re-pairing either side elsewhere, or leaving it unpaired,
loses at least `--margin` -- so two identical stubs that could swap are withheld.

A function sits in every vtable that inherits it: its name is written only if every table
proposing one proposes the same. A slot the map names DIFFERENTLY from the demo's paired slot
is reported (a contradiction: one of the two names is wrong), never rewritten.
"""

from __future__ import annotations

import argparse
import difflib
import re
import struct
import sys
from collections import defaultdict
from pathlib import Path

HERE = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(HERE))
sys.path.insert(0, str(HERE / "tools"))
import scdecomp as E
import s1data as S1D
import s1names as N

NAME = re.compile(rb"^[A-Za-z_][A-Za-z0-9_:<>, *]{0,120}$")
ANCHOR = 5.0
NOTE = "s1:vtable (the same slot of the same class's vtable in both builds, code alike)"


def mangle_class(name: str) -> str | None:
    """`zdb::CCell` -> `Q23zdb5CCell`, `CEntity` -> `7CEntity` (templates are not handled)."""
    if "<" in name or " " in name:
        return None
    parts = name.split("::")
    body = "".join(f"{len(p)}{p}" for p in parts)
    return body if len(parts) == 1 else f"Q{len(parts)}{body}"


def cstring(read, a: int) -> bytes | None:
    try:
        b = read(a, 128).split(b"\0")[0]
    except Exception:
        return None
    return b if b and NAME.match(b) else None


def our_vtables(img: E.Image) -> list[tuple[int, int, str, list[int]]]:
    """(vtable address, RTTI address, class name, slot targets): primary tables in OVL1/OVL2."""
    out = []
    for seg in img.segments[1:]:
        data, base = seg.data, seg.base
        for i in range(0, len(data) - 12, 4):
            r = struct.unpack_from("<I", data, i)[0]
            if struct.unpack_from("<I", data, i + 4)[0] != 0:
                continue
            first = struct.unpack_from("<I", data, i + 8)[0]
            if not img.is_code(first) or not (base <= r < base + len(data) - 8):
                continue
            s = struct.unpack_from("<I", data, r - base)[0]
            if not img.in_memory(s):
                continue
            name = cstring(img.read, s)
            if name is None:
                continue
            slots, j = [], i + 8
            while j + 4 <= len(data):
                w = struct.unpack_from("<I", data, j)[0]
                if not img.is_code(w):
                    break
                slots.append(w)
                j += 4
            out.append((base + i, r, name.decode(), slots))
    return out


def demo_vtables(demo: N.Demo) -> dict[str, list[tuple[int, str]]]:
    """mangled class -> [(function address, name)]: the primary table only (a class with
    several bases has its secondary tables after it, each starting with a non-code word)."""
    funcs = {f.value: f for f in demo.funcs}
    out = {}
    for s in demo.elf.symbols:
        if s.name.startswith("__vt__") and s.size >= 12:
            slots = []
            for k in range(2, s.size // 4):
                w = struct.unpack_from("<I", demo.data, s.value - demo.base + 4 * k)[0]
                if w not in funcs:
                    break
                slots.append((w, funcs[w].name))
            out[s.name[6:]] = slots
    return out


def opcodes(b: bytes) -> tuple:
    """The opcode alone (SPECIAL's by its function field); lui and nop dropped. Registers are
    NOT kept: one inlined level more or less re-allocates every register of a small function,
    and the demo's CNode::UpdateBBox scores 0.09 against SOCOM II's with them, 0.60 without."""
    ws = struct.unpack(f"<{len(b) // 4}I", b[:len(b) // 4 * 4])
    return tuple(w >> 26 if w >> 26 else 0x40 | (w & 0x3F) for w in ws if w and w >> 26 != 0x0F)


class Aligner:
    def __init__(self, img, st, demo, floor: float, margin: float, skeleton: str = "ops"):
        self.img, self.st, self.demo = img, st, demo
        self.skeleton = opcodes if skeleton == "ops" else N._skeleton
        self.floor, self.margin = floor, margin
        self.dsize = {f.value: f.size for f in demo.funcs}
        self._sim = {}
        self._vt = {}

    def writes_vtable(self, f: int, vt: int) -> bool:
        """Does f form the vtable's own address -- a constructor or destructor of its class?"""
        if f not in self._vt:
            s = self.st.at_addr(f)
            word = lambda a: struct.unpack_from("<I", self.img.read(a, 4))[0]
            self._vt[f] = {addr for _, addr, kind in S1D.data_refs(word, f, s.size, None) if kind == "a"} if s else set()
        return vt in self._vt[f]

    def sim(self, ours: int, theirs: int) -> float:
        k = (ours, theirs)
        if k not in self._sim:
            s = self.st.at_addr(ours)
            a, b = (s.size if s else 0), self.dsize.get(theirs, 0)
            if not a or not b or max(a, b) > 2 * min(a, b):
                self._sim[k] = 0.0
            else:
                x = self.skeleton(self.img.read(ours, a))
                y = self.skeleton(self.demo.read(theirs, b))
                self._sim[k] = difflib.SequenceMatcher(None, x, y, autojunk=False).ratio()
        return self._sim[k]

    def align(self, vt: int, ours: list[int], theirs: list[tuple[int, str]], hide: int | None = None):
        """-> {addr: (demo name, similarity, forced)} for every pair of the best alignment that
        is not an anchor. Anchors are the map's STRONG names (see `strong`) only: a weaker name
        (s1:align, s1:calls ...) is scored like a placeholder, so the alignment can contradict
        it -- an anchor on a wrong name would drag its neighbours with it. A slot whose function
        stores this very vtable, paired with the demo's destructor, is an anchor too: a
        destructor's body changes between builds (the demo's freed members SOCOM II's does not
        own) but not that it writes its class's vtable."""
        known = []
        for f in ours:
            s = self.st.at_addr(f)
            known.append(s.name if s is not None and f != hide and strong(s, self.hand) else "")
        n, m = len(ours), len(theirs)
        sc = {}
        for i, f in enumerate(ours):
            for j, (g, name) in enumerate(theirs):
                if known[i] and known[i] == name:
                    sc[(i, j)] = ANCHOR
                elif known[i] or name in known:
                    continue                    # a strong name pairs only with itself
                elif name.startswith("__dt__") and self.writes_vtable(f, vt):
                    sc[(i, j)] = ANCHOR - 0.5
                else:
                    r = self.sim(f, g)
                    if r >= self.floor:
                        sc[(i, j)] = r
        F = [[0.0] * (m + 1) for _ in range(n + 1)]    # best over ours[:i], theirs[:j]
        for i in range(1, n + 1):
            for j in range(1, m + 1):
                v = max(F[i - 1][j], F[i][j - 1])
                p = sc.get((i - 1, j - 1))
                if p is not None:
                    v = max(v, F[i - 1][j - 1] + p)
                F[i][j] = v
        B = [[0.0] * (m + 1) for _ in range(n + 1)]    # best over ours[i:], theirs[j:]
        for i in range(n - 1, -1, -1):
            for j in range(m - 1, -1, -1):
                v = max(B[i + 1][j], B[i][j + 1])
                p = sc.get((i, j))
                if p is not None:
                    v = max(v, B[i + 1][j + 1] + p)
                B[i][j] = v
        opt = F[n][m]
        through = lambda i, j: F[i][j] + sc[(i, j)] + B[i + 1][j + 1]
        out = {}
        i = j = 0
        while i < n and j < m:
            p = sc.get((i, j))
            if p is not None and abs(through(i, j) - opt) < 1e-9:
                if p < ANCHOR:
                    # forced? every other fate of slot i (and of demo slot j) costs the margin
                    alt = [through(i, k) for k in range(m) if k != j and (i, k) in sc]
                    alt += [through(k, j) for k in range(n) if k != i and (k, j) in sc]
                    alt += [F[i][k] + B[i + 1][k] for k in range(m + 1)]      # ours[i] unpaired
                    alt += [F[k][j] + B[k][j + 1] for k in range(n + 1)]      # theirs[j] unpaired
                    forced = max(alt, default=-1) <= opt - self.margin
                    out[ours[i]] = (theirs[j][1], 1.0 if p >= ANCHOR - 1 else p, forced)
                i, j = i + 1, j + 1
            elif abs(B[i + 1][j] - B[i][j]) < 1e-9:
                i += 1
            else:
                j += 1
        return out


WEAK = ("s1:align", "s1:calls", "s1:gap", "s1:ref", "s1:order", "s1:unit")


def strong(sym: E.Symbol, hand: set[str]) -> bool:
    """A name to anchor on: hand-written source defines it, or it was found exactly (s1:exact,
    s1:callee, s1:cmd) or by hand. A name from a similarity or order pass is a claim to check."""
    if E.is_placeholder(sym.name):
        return False
    if sym.name in hand:
        return True
    return not any(w in (sym.note or "") for w in WEAK)


def run(a, img, st, demo):
    from automatch import hand_defined
    ours, theirs = our_vtables(img), demo_vtables(demo)
    al = Aligner(img, st, demo, a.floor, a.margin, a.skeleton)
    al.hand = hand_defined()
    votes, paired = defaultdict(lambda: defaultdict(list)), []
    for vt, rtti, cls, slots in ours:
        m = mangle_class(cls)
        if m is None or m not in theirs:
            continue
        paired.append((vt, cls, slots, theirs[m]))
        for f, (name, p, forced) in al.align(vt, slots, theirs[m]).items():
            votes[f][name].append((cls, p, forced))
    return ours, al, paired, votes


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--write", action="store_true")
    ap.add_argument("--holdout", action="store_true")
    ap.add_argument("--floor", type=float, default=0.5)
    ap.add_argument("--margin", type=float, default=0.03)
    ap.add_argument("--skeleton", choices=("ops", "full"), default="ops",
                    help="compare opcodes only (default) or s1names' skeleton, registers kept")
    ap.add_argument("-v", "--verbose", action="store_true")
    a = ap.parse_args(argv)
    img, st, demo = E.image(), E.symbols(), N.Demo()
    ours, al, paired, votes = run(a, img, st, demo)

    if a.holdout:
        # every slot holding a strong name the demo's table also has: hidden, re-derived
        tally = defaultdict(int)
        for vt, cls, slots, th in paired:
            names = {nm for _, nm in th}
            for f in dict.fromkeys(slots):
                s = st.at_addr(f)
                if s is None or not strong(s, al.hand) or s.name not in names or "s1:vtable" in (s.note or ""):
                    continue                    # (a name this pass gave proves nothing about it)
                got = al.align(vt, slots, th, hide=f).get(f)
                verdict = "none" if got is None or not got[2] else "right" if got[0] == s.name else "WRONG"
                tally[verdict] += 1
                if verdict == "WRONG":
                    print(f"  WRONG {f:#x} {cls}: map {s.name}, proposed {got[0]} ({got[1]:.2f})")
        print(f"holdout: {dict(tally)}")
        return 0

    new, fix, same, withheld, judged = {}, {}, defaultdict(int), {}, {}
    for f, by_name in votes.items():
        s = st.at_addr(f)
        if len(by_name) > 1:
            withheld[f] = sorted(by_name)
            continue
        (name, vs), = by_name.items()
        if not all(forced for _, _, forced in vs):
            continue
        if s.name == name:
            same["strong" if strong(s, al.hand) else "weak"] += 1
        elif "s1~" in (s.note or ""):
            judged[f] = name
        elif E.is_placeholder(s.name):
            new[f] = name
        elif not strong(s, al.hand):
            fix[f] = name
    # a name may move: one a correction frees can be given in the same batch
    holder = {x.name: x.addr for x in st.symbols}
    final = {**new, **fix}
    count = defaultdict(int)
    for n in final.values():
        count[n] += 1
    freed = {st.at_addr(f).name for f in final}
    blocked = {f for f, n in final.items()
               if count[n] > 1 or (n in holder and holder[n] != f and n not in freed)}
    for f in blocked:
        final.pop(f)
    print(f"{len(ours)} SOCOM II vtables, {len(paired)} with a SOCOM 1 counterpart")
    print(f"  confirmed: {same['strong']} strong names (anchors aside), {same['weak']} weaker ones")
    print(f"  named: {sum(1 for f in final if f in new)} placeholders; corrected: "
          f"{sum(1 for f in final if f in fix)} weaker names the alignment contradicts")
    print(f"  withheld: {len(withheld)} (tables disagree), {len(blocked)} (name held elsewhere), "
          f"{len(judged)} (rows carrying an s1~ verdict)")
    for f in sorted(fix):
        tag = "" if f in final else "  [blocked]"
        print(f"  FIX {f:#x} {st.at_addr(f).name[:70]} -> {fix[f][:70]}{tag}")
    if a.verbose:
        for f, ns in sorted(withheld.items()):
            print(f"  withheld {f:#x}: {ns}")
        for f in sorted(blocked):
            print(f"  blocked {f:#x} -> {({**new, **fix})[f]}")
        for f, n in sorted(judged.items()):
            print(f"  s1~ row {f:#x} {st.at_addr(f).name}: vtable says {n}")
        for f, n in sorted(final.items()):
            if f in new:
                print(f"  {f:#x} {n}")
    if a.write:
        import rename
        rename.rename([(f, n, NOTE if f in new else f"{NOTE}; was {st.at_addr(f).name}")
                       for f, n in sorted(final.items())])
        rows = E.DATA.read_text().rstrip("\n").splitlines()
        have = {int(r.split()[0], 16) for r in rows if r.startswith("0x")}
        dnames = {r.split()[2] for r in rows if r.startswith("0x")}
        added = 0
        for vt, rtti, cls, slots in ours:
            m = mangle_class(cls)
            if m is None:
                continue
            for addr, name, size, what in ((rtti, f"__RTTI__{m}", 8, "RTTI"),
                                           (vt, f"__vt__{m}", 8 + 4 * len(slots), "vtable")):
                if addr in have or name in dnames:
                    continue
                rows.append(f"0x{addr:08X} 0x{size:X} {name}  # {what} of {cls}: found by its RTTI's name")
                have.add(addr)
                dnames.add(name)
                added += 1
        head = [r for r in rows if not r.startswith("0x")]
        body = sorted((r for r in rows if r.startswith("0x")), key=lambda r: int(r.split()[0], 16))
        E.DATA.write_text("\n".join(head + body) + "\n")
        print(f"wrote {added} RTTI/vtable rows to {E.DATA.name}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
