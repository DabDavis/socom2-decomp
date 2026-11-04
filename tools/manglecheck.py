#!/usr/bin/env python3
"""
manglecheck -- are the map's mangled names self-consistent?

    ./tools/manglecheck.py                 # symbols.txt and data.txt: every slip, by lane
    ./tools/manglecheck.py NAME ...        # check names given on the command line
    ./tools/manglecheck.py --all           # also list the names that parse (one per line)

A Metrowerks name spells every class, namespace and template by its length:
`clear__Q23std72__vector_deleter<15CDlgDesignModel,Q23std28allocator<15CDlgDesignModel>>Fv`.
The UI lane's round 16 found two rows that read `70` there (0x36AE50, 0x36ACF0) -- by hand, and
only because no source could ever match them: a compiler emits the right length, so a row whose
length is wrong is a name no unit can define, and its function scores `no-target` forever.

Each name that claims to be mangled (`base__` followed by a scope, a qualified name or `F`, a
thunk `@N@`, a `@LOCAL@`/`@GUARD@` static) is parsed with a backtracking parser of the
grammar the map's 7,700-odd mangled names use (cfront's, as Metrowerks writes it: `Q<n>`
qualified names, `<len>name<args>` templates, `@unnamed@<file>@` anonymous namespaces, `F...`
functions and `F..._<ret>` function types, `M<class>` member pointers, `A<n>_` arrays, `T<n>`
and `N<n><m>` repeats); a length is accepted only when the whole name parses with it. A name
that does not parse is a slip: for each, the tool tries every single length prefix changed to
every other value and prints the corrections that make the whole name parse (one, normally).
"""

from __future__ import annotations

import argparse
import re
import sys
from functools import lru_cache
from pathlib import Path

HERE = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(HERE))

BUILTIN = set("vbcsilxfdrw1")          # '1': the EE's 128-bit integer (cxx.py's u_long128)
QUALS = set("CVPRUS")
IDENT = re.compile(r"[A-Za-z_$@][A-Za-z0-9_$@]*")   # '@': `@unnamed@<file>_cpp@` is one name
# a constructor, destructor or operator is a function: `__dt__Q23std14length_errorFv` (12 for
# 14) would otherwise read as static data of a class `length_errorFv`
FUNCTION_ONLY = re.compile(r"__(?:ct|dt|as|pl|mi|ml|dv|md|eq|ne|lt|gt|le|ge|nw|dl|vc|cl|apl|ami|"
                           r"amu|adv|er|or|ad|co|nt|aa|oo|ls|rs|pp|mm|rf|rm|vn|vd|nwa|dla|opv|aer|"
                           r"aor|aad|als|ars|amd|op[A-Za-z0-9_]*)$")


class Parser:
    """For each rule and position, the end positions it can reach -- each with ONE derivation,
    the length-prefixed names it read (start of the digits, end of the name). The grammar is
    ambiguous at a few points ('1' a builtin or a length, a template argument a type or a
    literal) and only a parse of the whole name decides."""

    def __init__(self, s: str):
        self.s = s
        self.n = len(s)
        self.cache: dict = {}

    def memo(self, key, fn) -> dict:
        if key not in self.cache:
            self.cache[key] = {}                    # a guard against left recursion
            out: dict = {}
            for end, spans in fn():
                out.setdefault(end, spans)
            self.cache[key] = out
        return self.cache[key]

    # <len><name>: the digits are maximal (no name starts with a digit), the name exactly that long
    def lenname(self, i: int) -> dict:
        def go():
            m = re.compile(r"\d+").match(self.s, i)
            if not m:
                return
            n, j = int(m.group()), m.end()
            if n == 0 or j + n > self.n:
                return
            inner = self.name_text(j, j + n)
            if j + n in inner:
                yield j + n, ((i, j + n),) + inner[j + n]
        return self.memo(("len", i), go)

    # a name inside a length's span: an identifier, then template arguments ending the span
    def name_text(self, i: int, end: int) -> dict:
        def go():
            m = IDENT.match(self.s, i, end)
            if not m:
                return
            k = m.end()
            if k == end:
                yield end, ()
            elif self.s[k] == "<" and self.s[end - 1] == ">":
                ta = self.targs(k + 1)
                if end - 1 in ta:
                    yield end, ta[end - 1]
        return self.memo(("nt", i, end), go)

    def targs(self, i: int) -> dict:
        def go():
            for j, sp in self.targ(i).items():
                if j < self.n and self.s[j] == ",":
                    for k, sp2 in self.targs(j + 1).items():
                        yield k, sp + sp2
                yield j, sp
        return self.memo(("targs", i), go)

    def targ(self, i: int) -> dict:
        def go():
            yield from self.type(i).items()
            m = re.compile(r"-?\d+").match(self.s, i)      # a value: `,0>` (vector_imp's bool)
            if m:
                yield m.end(), ()
        return self.memo(("targ", i), go)

    def scope(self, i: int) -> dict:
        def go():
            if self.s.startswith("Q", i) and i + 1 < self.n and self.s[i + 1].isdigit():
                ends = {i + 2: ()}
                for _ in range(int(self.s[i + 1])):
                    nxt = {}
                    for p, sp in ends.items():
                        for e, sp2 in self.lenname(p).items():
                            nxt.setdefault(e, sp + sp2)
                    ends = nxt
                yield from ends.items()
            yield from self.lenname(i).items()
        return self.memo(("scope", i), go)

    def type(self, i: int) -> dict:
        def go():
            j = i
            while j < self.n and self.s[j] in QUALS:
                j += 1
            for k in range(i, j + 1):          # the qualifiers, then a base type (a qualifier
                if k < self.n:                 # letter may itself be a builtin: `S`, `U` are not)
                    yield from self.base_type(k).items()
        return self.memo(("type", i), go)

    def base_type(self, i: int) -> dict:
        def go():
            c = self.s[i]
            if c in BUILTIN:
                yield i + 1, ()
            if c.isdigit() or c == "Q":
                yield from self.scope(i).items()
            if c == "F":                                   # a function type: F<params>_<ret>
                for j, sp in self.params(i + 1).items():
                    if j < self.n and self.s[j] == "_":
                        for k, sp2 in self.type(j + 1).items():
                            yield k, sp + sp2
            if c == "A":                                   # an array: A<n>_<type>
                m = re.compile(r"A(\d+)_").match(self.s, i)
                if m:
                    yield from self.type(m.end()).items()
            if c == "M":                                   # a member pointer: M<class><type>
                for j, sp in self.scope(i + 1).items():
                    for k, sp2 in self.type(j).items():
                        yield k, sp + sp2
        return self.memo(("base", i), go)

    def params(self, i: int) -> dict:
        """Every end of a parameter list from i (possibly empty)."""
        def go():
            yield i, ()
            if i >= self.n:
                return
            c = self.s[i]
            nxt = {}
            if c == "T" and i + 1 < self.n and self.s[i + 1].isdigit():
                nxt[i + 2] = ()
            if c == "N" and i + 2 < self.n and self.s[i + 1].isdigit() and self.s[i + 2].isdigit():
                nxt[i + 3] = ()
            if c == "e":
                nxt[i + 1] = ()
            for j, sp in self.type(i).items():
                nxt.setdefault(j, sp)
            for j, sp in nxt.items():
                for k, sp2 in self.params(j).items():
                    yield k, sp + sp2
        return self.memo(("params", i), go)

    # a function's or a static member's whole name, from i
    def entity(self, i: int) -> dict:
        def go():
            s = self.s
            # base: an identifier (leading underscores and all), optionally template arguments
            m = IDENT.match(s, i)
            if not m:
                return
            bases = {}
            k = i
            while True:
                k = s.find("__", k + 1)
                if k < 0 or k > m.end():
                    break
                if k > i:
                    bases.setdefault(k, ())
            if m.end() < self.n and s[m.end()] == "<":          # a function template's name
                for t, sp in self.targs(m.end() + 1).items():
                    if t < self.n and s[t] == ">" and s.startswith("__", t + 1):
                        bases.setdefault(t + 1, sp)
            for b, sp in bases.items():
                r = b + 2
                if r >= self.n:
                    continue
                if s[r] == "F":
                    for e, sp2 in self.function(r).items():
                        yield e, sp + sp2
                for j, sp2 in self.scope(r).items():
                    if not FUNCTION_ONLY.match(s, i, b):
                        yield j, sp + sp2                       # a static member, a vtable, RTTI
                    k2 = j
                    while k2 < self.n and s[k2] in "CV":        # a const member function
                        k2 += 1
                    if k2 < self.n and s[k2] == "F":
                        for e, sp3 in self.function(k2).items():
                            yield e, sp + sp2 + sp3
        return self.memo(("entity", i), go)

    def function(self, i: int) -> dict:
        def go():
            for j, sp in self.params(i + 1).items():
                yield j, sp
                if j < self.n and self.s[j] == "_":             # a template function's return
                    for k, sp2 in self.type(j + 1).items():
                        yield k, sp + sp2
        return self.memo(("fn", i), go)

    def parse(self):
        """The length-prefixed names of one parse of the whole name, or None."""
        s = self.s
        m = re.match(r"@(LOCAL|GUARD)@", s)
        if m:                                       # @LOCAL@<function>@<static>
            for j, sp in self.entity(m.end()).items():
                if j < self.n and s[j] == "@" and IDENT.fullmatch(s, j + 1):
                    return sp
            return None
        m = re.match(r"@\d+@(?:\d+@)?", s)            # a thunk: @this-offset@[vtable offset@]<name>
        return self.entity(m.end() if m else 0).get(self.n)

    def whole(self) -> bool:
        return self.parse() is not None


def claims_mangled(name: str) -> bool:
    """A name that says it is Metrowerks-mangled: `base__` then a scope, `Q`, or `F`."""
    return bool(re.search(r"[A-Za-z0-9_>]__(?:\d|Q\d|F|C+F)", name))


def parses(name: str) -> bool:
    return Parser(name).whole()


def _with_length(name: str, start: int, end: int, n: int) -> str:
    return name[:start] + str(n) + name[end:]


def corrections(name: str, limit: int = 4) -> list[str]:
    """The names one changed length makes parse. Every digit run is tried, and a correction
    is kept only when it is the ONE value of its run (within 6) that parses: a digit inside an
    identifier (`C2DButton`) parses at every value and is no length."""
    out = []
    for m in re.finditer(r"\d+", name):
        ok = [n for n in range(max(1, int(m.group()) - 6), int(m.group()) + 7)
              if n != int(m.group()) and parses(_with_length(name, m.start(), m.end(), n))]
        if len(ok) == 1:
            cand = _with_length(name, m.start(), m.end(), ok[0])
            if cand not in out:
                out.append(cand)
        if len(out) >= limit:
            break
    return out


def vocabulary(names) -> "Counter":
    """How many of the map's names spell each length-prefixed name (`CDlgDesignModel`)."""
    from collections import Counter
    voc: Counter = Counter()
    for nm in names:
        sp = Parser(nm).parse()
        if sp:
            voc.update({nm[s:e].lstrip("0123456789") for s, e in sp})
    return voc


def suspects(name: str, voc, rare: int = 1, common: int = 3) -> list[tuple[str, str, str]]:
    """A slip the grammar cannot see: a length one or two too long that swallows a qualifier
    and still parses (`9bad_castCFv` for `8bad_castCFv`; `13ThrowContextP13...`). Flagged when
    the name the length spells occurs in at most `rare` of the map's names (and only once in
    this one: `ReadCellTable<Q26CAiMap7actions,...>__FR7CFileIORQ26CAiMap7actions_b` spells
    CAiMap::actions twice alike) and the name the other length spells in `common` or more, and
    the name parses with it."""
    out = []
    sp = Parser(name).parse() or ()
    for s, e in sp:
        m = re.compile(r"\d+").match(name, s)
        text = name[m.end():e]
        if voc[text] > rare or name.count(name[s:e]) > 1:     # spelled alike twice: consistent
            continue
        for d in (-3, -2, -1, 1, 2, 3):
            n = len(text) + d
            alt = name[m.end():m.end() + n]
            if n <= 0 or len(alt) != n or voc[alt] < common:
                continue
            cand = _with_length(name, m.start(), m.end(), n)
            if parses(cand):
                out.append((text, alt, cand))
    return out


def lane_of(addr: int, note: str) -> str:
    m = re.search(r"src/([\w/.\-]+?\.(?:cpp|c|s))", note or "")
    if m:
        return m.group(1)
    return "MAIN" if addr < 0x1E7000 else "OVL1" if addr < 0x4C5380 else "OVL2"


def map_rows(paths):
    for path in paths:
        for n, line in enumerate(path.read_text(errors="replace").splitlines(), 1):
            if line.startswith("0x"):
                parts = line.split(None, 3)
                note = line.split("#", 1)[1].strip() if "#" in line else ""
                yield path.name, n, int(parts[0], 16), parts[2], note


def check_map(paths, show_all=False) -> int:
    rows = [r for r in map_rows(paths) if claims_mangled(r[3])]
    voc = vocabulary(r[3] for r in rows)
    bad = sus = 0
    for fname, n, addr, name, note in rows:
        if parses(name):
            flagged = suspects(name, voc)
            if flagged:
                sus += 1
                print(f"SUSPECT {fname}:{n} 0x{addr:08X} {name}")
                print(f"     lane/unit: {lane_of(addr, note)}")
                for text, alt, cand in flagged:
                    print(f"     `{text}` is spelled nowhere else, `{alt}` by {voc[alt]} names: {cand}")
            elif show_all:
                print(f"ok   {fname}:{n} {name}")
            continue
        bad += 1
        fixes = corrections(name)
        print(f"SLIP {fname}:{n} 0x{addr:08X} {name}")
        print(f"     lane/unit: {lane_of(addr, note)}")
        for f in fixes:
            print(f"     fix: {f}")
        if not fixes:
            print("     fix: none by one length (a grammar this tool does not know, or two slips)")
    print(f"{len(rows)} mangled names checked: {bad} slip(s), {sus} suspect(s)")
    return 1 if bad else 0


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("names", nargs="*", help="names to check (default: the map's)")
    ap.add_argument("--all", action="store_true", help="list the names that parse too")
    a = ap.parse_args(argv)
    if a.names:
        rc = 0
        for nm in a.names:
            ok = parses(nm)
            print(f"{'ok  ' if ok else 'SLIP'} {nm}")
            if not ok:
                rc = 1
                for f in corrections(nm):
                    print(f"     fix: {f}")
        return rc
    return check_map([HERE / "symbols.txt", HERE / "data.txt"], a.all)


if __name__ == "__main__":
    sys.exit(main())
