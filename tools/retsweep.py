#!/usr/bin/env python3
"""
retsweep -- every open function of the GCC units tried with each callee's declared return type flipped, void <-> int
(GCC round 44a). The Medius lane's round 25b measured it: a call that sets v0 just before a block's first temporaries
keeps them off v0, and a void call leaves v0 to them -- so an unused result's type is not neutral, and a pair of registers
swapped right after a call is often the callee's declaration. sceGsResetGraph (open since round 6a, 4 words) was
`extern void GsPutIMR(u_long)`, the kernel's returning the old mask.

    ./tools/retsweep.py --files '^lib/sdk/'                 # the units under src/ matching, every open function
    ./tools/retsweep.py --files lib/sdk/gs_resetgraph.c -f sceGsResetGraph
    ./tools/retsweep.py --files '^lib/' --report-all        # every flip tried, not only those that help

Each unit is built as sccheck builds it (in tools/try.py's scratch tree: $SC_TRY_WORK, build/try by default), every function
scored. For each WRONG function, each callee its code calls by name (its R_MIPS_26 relocations, the function's own
name and the unit's own definitions left out) is found declared -- in the unit's text, else in the headers it
#includes, a declaration ending in `;` -- and the return type flipped there: void to int, anything else to void. A
callee no declaration names (an implicit int) gets `extern void NAME();` above the first use. The flip is one variant:
the unit rebuilt, the function scored (MATCH, or difflib's distance from its row, try.py's d=) and every other
function of the unit scored too. A flip that loses another function is said so.

Printed: `HIT` (byte-exact, nothing lost), `hit-loses`, `closer` (nearer, nothing lost), per function, and then a greedy
pass: the flips that bring it closer applied together, one at a time while they keep helping. Nothing is written to
the tree; apply a flip by hand and say why the callee returns what it does. GCC units only: Metrowerks allocates otherwise.
"""

from __future__ import annotations

import argparse
import importlib
import re
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(HERE))
sys.path.insert(0, str(HERE / "tools"))
import scdecomp as E
import scdiff as D
import sccheck

T = importlib.import_module("try")      # tools/try.py: the scratch tree, the build and the scores


def gcc_units(files: str | None) -> list[Path]:
    """The GCC units under src/ (drafts left out) whose path matches `files`."""
    rx = re.compile(files) if files else None
    out = []
    for p in sccheck.sources(sccheck.SRC):
        rel = p.relative_to(sccheck.SRC).as_posix()
        if rel.startswith("auto/") or p.suffix == ".s" or (rx and not rx.search(rel)):
            continue
        if D.is_gcc(sccheck.tu_options(p).get("compiler", D.COMPILER)):
            out.append(p)
    return out


def headers_of(unit: Path) -> list[Path]:
    """The unit's own #include "..." and <...> files that exist beside it, under src/ or include/ (one level
    of nesting per file, recursively, each once)."""
    seen: list[Path] = []
    todo = [unit]
    while todo:
        f = todo.pop()
        for m in re.finditer(r'^[ \t]*#[ \t]*include[ \t]+["<]([^">]+)[">]', f.read_text(errors="surrogateescape"), re.M):
            for base in (f.parent, unit.parent, sccheck.SRC, sccheck.INCLUDE):
                h = (base / m.group(1)).resolve()
                if h.is_file() and h not in seen:
                    seen.append(h)
                    todo.append(h)
                    break
    return seen


def declaration(text: str, name: str) -> re.Match | None:
    """The first file-scope-looking declaration of `name` in `text`: `[extern] TYPE [*]NAME(...);` on a line of its
    own start; groups `type` (with its stars) and `name`. A definition (the parameters followed by `{`) is not one."""
    for m in re.finditer(r"^(?P<pre>[ \t]*(?:extern[ \t]+)?)(?P<type>[A-Za-z_][\w \t]*?[ \t\*]+)(?P<name>" + re.escape(name)
                         + r")[ \t]*\(", text, re.M):
        if re.search(r"\b(return|else|if|typedef)\b", m.group("type")):
            continue
        depth, i = 1, m.end()
        while i < len(text) and depth:
            depth += {"(": 1, ")": -1}.get(text[i], 0)
            i += 1
        rest = text[i:].lstrip()
        if rest.startswith(";"):
            return m
    return None


def flipped(text: str, m: re.Match) -> tuple[str, str]:
    """`text` with the declaration's return type flipped; (the text, `old -> new`)."""
    old = m.group("type").strip()
    new = "int" if old == "void" else "void"
    sep = "" if m.group("type").endswith(("*",)) else " "
    return text[:m.start("type")] + new + " " + text[m.start("name"):], f"{old} -> {new}"


def callees(elf: E.Elf, name: str) -> list[str]:
    """Who `name`'s code calls by name, first call first (no local section-relative call, not itself)."""
    _, rels, _ = elf.function(name)
    defined = {f.name for f in elf.functions()}
    out = []
    for r in rels:
        n = r.sym.name
        if r.type == "R_MIPS_26" and n and r.sym.bind != 0 and n != name and n not in defined and n not in out:
            out.append(n)
    return out


class Unit:
    """A unit's base build in try.py's scratch tree: `matched`, and each open function's distance and callees."""

    def __init__(self, path: Path, text: str | None = None):
        """`text`: the scratch tree's copy of the unit holds this, not the file's text (the canary's fixture)."""
        self.path = path
        self.text = path.read_text(errors="surrogateescape") if text is None else text
        self.headers = headers_of(path)
        self.error = ""
        obj, err = T.build(path, path, None)
        self.obj = obj
        self.open: list[str] = []
        self.matched: set[str] = set()
        if obj is None:
            self.error = err
            return
        ok, n, misses = T.score_all(obj, path)
        elf = E.Elf.read(obj)
        self.matched = {f.name for f in elf.functions()} - set(misses)
        st = E.symbols()
        span = sccheck.unit_span(obj, st, sccheck.tu_options(path).get("overlay"))
        for name in misses:
            f = next(g for g in elf.functions() if g.name == name)
            self.open.append(name)
        self.elf = elf
        self.dist = {name: T.distance(obj, path, name) for name in self.open}
        self.calls = {name: callees(elf, name) for name in self.open}

    def where(self, name: str) -> tuple[Path, re.Match] | None:
        """The file declaring `name`: the unit, else a header it includes."""
        m = declaration(self.text, name)
        if m:
            return self.path, m
        for h in self.headers:
            m = declaration(h.read_text(errors="surrogateescape"), name)
            if m:
                return h, m
        return None


def edits_for(u: Unit, name: str) -> tuple[Path, str, str] | None:
    """(the file, its text flipped for callee `name`, a label); None when the callee's name is not in
    the unit's text (a macro's in a header, an `as=` row): nothing to flip by."""
    w = u.where(name)
    if w is None:                     # an implicit int: declared void above its first use
        use = re.search(r"\b" + re.escape(name) + r"[ \t]*\(", u.text)
        if use is None:
            return None
        i = use.start()
        line = u.text.rfind("\n", 0, i)
        # above the function it is first used in: the last blank line before the use
        j = u.text.rfind("\n\n", 0, line)
        at = j + 1 if j >= 0 else 0
        return u.path, u.text[:at] + f"extern void {name}();\n" + u.text[at:], "implicit int -> void"
    path, m = w
    text = u.text if path == u.path else path.read_text(errors="surrogateescape")
    new, label = flipped(text, m)
    return path, new, label


def try_variant(u: Unit, fn: str, files: dict[Path, str]) -> dict:
    """The unit built with `files`' texts in place, `fn` scored and every function of the unit."""
    wfiles = {p: T._in_tree(p) for p in files}
    orig = {p: w.read_bytes() for p, w in wfiles.items()}
    try:
        for p, text in files.items():
            wfiles[p].write_text(text, errors="surrogateescape")
        obj, err = T.build(u.path, u.path, None)
        if obj is None:
            return dict(verdict="BUILD-FAIL " + T._last_error(err), match=False, d=None, lost=[])
        verdict = T.score_obj(obj, u.path, fn)
        d = T.distance(obj, u.path, fn)
        ok, n, misses = T.score_all(obj, u.path)
        return dict(verdict=verdict, match=verdict == "MATCH", d=d[0] if d else None, size=d[1] * 4 if d else None,
                    lost=sorted(u.matched & set(misses)))
    finally:
        for p, data in orig.items():
            wfiles[p].write_bytes(data)


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--files", help="regex over unit paths under src/ (default: every hand-written GCC unit)")
    ap.add_argument("-f", "--function", help="regex over the open functions' names: only those")
    ap.add_argument("--report-all", action="store_true", help="print every flip tried, not only those that help")
    a = ap.parse_args(argv)
    units = gcc_units(a.files)
    if not units:
        raise SystemExit(f"retsweep: no hand-written GCC unit matches {a.files!r}")
    T.prepare()
    rx = re.compile(a.function) if a.function else None
    hits = closer = tried = 0
    for path in units:
        u = Unit(path)
        rel = path.relative_to(sccheck.SRC).as_posix()
        if u.error:
            print(f"{rel}: BUILD-FAIL as it stands: {T._last_error(u.error)}", flush=True)
            continue
        for fn in u.open:
            if rx and not rx.search(fn):
                continue
            base = u.dist.get(fn)
            if base is None:
                continue
            d0 = base[0]
            helpful = []
            for callee in u.calls.get(fn, []):
                edit = edits_for(u, callee)
                if edit is None:
                    continue
                file, text, label = edit
                r = try_variant(u, fn, {file: text})
                tried += 1
                kind = ("HIT" if not r["lost"] else "hit-loses") if r["match"] else (
                    "closer" if r["d"] is not None and r["d"] < d0 and not r["lost"] else "")
                if kind:
                    helpful.append((callee, r["d"]))
                if kind or a.report_all:
                    lost = f"  loses {', '.join(x[:30] for x in r['lost'][:3])}" if r["lost"] else ""
                    print(f"{rel}: {fn[:44]:<44} {kind or '-':<9} {callee[:36]:<36} {label:<28} d {d0} -> {r['d']}"
                          f" {r['verdict'][:40] if kind != 'HIT' else ''}{lost}", flush=True)
                hits += kind == "HIT"
                closer += kind == "closer"
            if len(helpful) > 1:          # the greedy pass: the helpful flips together
                files: dict[Path, str] = {}
                now = d0
                used = []
                for callee, _ in sorted(helpful, key=lambda c: c[1] if c[1] is not None else 1 << 30):
                    w = u.where(callee)
                    if w is None:
                        continue
                    p, _m = w
                    text = files.get(p, p.read_text(errors="surrogateescape"))
                    m = declaration(text, callee)
                    if m is None:
                        continue
                    trial = dict(files)
                    trial[p] = flipped(text, m)[0]
                    r = try_variant(u, fn, trial)
                    if r["d"] is not None and r["d"] < now and not r["lost"]:
                        files, now = trial, r["d"]
                        used.append(callee)
                        if r["match"]:
                            hits += 1
                            break
                if len(used) > 1:
                    print(f"{rel}: {fn[:44]:<44} together {', '.join(x[:30] for x in used)}: d {d0} -> {now}", flush=True)
    print(f"\n{len(units)} units, {tried} flips tried: {hits} HIT, {closer} closer")
    return 0


if __name__ == "__main__":
    sys.exit(main())
