#!/usr/bin/env python3
"""
pragmascan -- Metrowerks' optimiser pragmas tried on every OPEN function of the hand-written
Metrowerks units, one function and one pragma at a time (GCC round 43a; the kit's round 58 made
FaceXZ 0x60CAD0 -- 36 off for 42 rounds -- and CAiDodge::Destroy 0x5FAD50 byte-exact with
`#pragma opt_common_subs off` as the first line of the body and `#pragma opt_common_subs reset`
after its closing brace; its own scan covered only its units).

    ./tools/pragmascan.py                                  # every hand-written MW unit, 2 jobs
    ./tools/pragmascan.py --files '^ai/'                   # the units whose path under src/ matches
    ./tools/pragmascan.py --files ai/ai_group.cpp -f FaceXZ --pragmas opt_common_subs
    ./tools/pragmascan.py --report                         # the results file's hits, nothing built

Each unit (src/**/*.c|cpp outside src/auto whose directive names no ee-gcc toolchain) is built as it
stands, as sccheck builds it (its directive, its statics qualified, `overlay=`), in a scratch copy of
src/ (drafts left out) and include/ laid out at ONE path length (objcmp's side_root: Metrowerks' code
can depend on the tree path's length, GCC round 38a), and every function scored as sccheck scores it
-- an inline copy that misses at an address outside the unit's span is `elsewhere`, not open. Each
OPEN function (WRONG there) is found in the unit's own text -- its definition's name and scope read
off the mangled name, matched against every function body the text defines (namespaces, classes and
in-class bodies followed; an overload set told apart by one build with a probe in each candidate
body: the function whose code references candidate k's probe is k's) -- and, for each pragma P,
wrapped alone:

    <signature>
    {
    #pragma P off
        ... the body ...
    }
    #pragma P reset

The unit is built again and EVERY function scored: a (function, pragma) that makes the function
byte-exact and loses nothing else in the unit is a HIT; one that only brings it closer is `closer`
(its difflib distance from its row, try.py's d=, before -> after; a hit that loses another function
is listed with the losses). A function defined outside the unit's text (a header's inline, a
template instance), inside a macro, or in an `asm` body is SKIPPED, said so with the reason.

Each pragma is checked first: a fixture compiled under `-w all` with `#pragma P off` and `reset`,
under every Metrowerks toolchain the units name -- Metrowerks warns `illegal #pragma` for a name it
does not know, and a pragma it does not know would otherwise change nothing anywhere and read as "no
hit" (refused, the scan not run).

Resumable: every base build and every variant is one JSON line in the results file (--results;
default build/pragmascan/results.jsonl), keyed by the unit's path, its text's digest, the headers'
digest (src/**/*.h, include/) and the variant; a re-run skips what is recorded and rebuilds nothing
it already scored. --unit-budget SECONDS (default 900) stops a unit's variants when its time is up
(a `budget` line says how many were left; the next run takes them up). --jobs (default 2) units at
once, each in its own scratch tree.

The report ends with THE LIKELIEST NEXT HITS (GCC round 44a, the UI's round 38): `opt_loop_invariants` and
`opt_strength_reduction` both off keep IRO_FindLoops from running at all (IrOptimizer.c), and SkbParseComplete was exact
only under that pair, 0x2B0 under either alone -- the pairs run had reported it only as nearer, its source shape still
wrong. So a row whose best variant reaches the target row's SIZE is listed, nearest first: `at size` a variant brings
it there from another size (the strongest lead: a whole loop or block now exists or has gone), `closer at size` one
already there that a variant brings nearer. A variant that loses another function is shown with the loss.
"""

from __future__ import annotations

import argparse
import difflib
import hashlib
import json
import os
import re
import shutil
import subprocess
import sys
import threading
import time
from concurrent.futures import ThreadPoolExecutor
from dataclasses import dataclass, field
from pathlib import Path

HERE = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(HERE))
sys.path.insert(0, str(HERE / "tools"))
import scdecomp as E
import scdiff as D
import sccheck

PRAGMAS = ("opt_common_subs", "opt_loop_invariants", "opt_propagation", "opt_strength_reduction",
           "opt_lifetimes", "opt_dead_assignments", "peephole")
WORK = HERE / "build" / "pragmascan"
PATH_LENGTH = 160          # the scratch trees' path length (objcmp's first side length)
PROBE = "__pragmascan_probe_"


# --- the unit's text: its function bodies -------------------------------------------------------

def blank(text: str) -> str:
    """`text` with its comments, the insides of its string and character literals and its
    preprocessor lines turned to spaces -- newlines and the quotes kept, so every offset stays."""
    out = list(text)
    i, n = 0, len(text)
    line_start = True
    while i < n:
        c = text[i]
        if line_start and c in " \t":
            i += 1
            continue
        if line_start and c == "#":
            # a directive, its continuation lines with it
            while i < n and text[i] != "\n":
                if text[i] == "\\" and i + 1 < n and text[i + 1] == "\n":
                    out[i] = " "
                    i += 2
                    continue
                out[i] = " "
                i += 1
            continue
        line_start = False
        if c == "\n":
            line_start = True
            i += 1
        elif text.startswith("//", i):
            while i < n and text[i] != "\n":
                out[i] = " "
                i += 1
        elif text.startswith("/*", i):
            j = text.find("*/", i + 2)
            j = n if j < 0 else j + 2
            for k in range(i, j):
                if text[k] != "\n":
                    out[k] = " "
            i = j
        elif c in "\"'":
            j = i + 1
            while j < n and text[j] != c and text[j] != "\n":
                if text[j] == "\\":
                    out[j] = " "
                    j += 1
                if j < n and text[j] != "\n":
                    out[j] = " "
                j += 1
            i = j + 1
        else:
            i += 1
    return "".join(out)


@dataclass
class Site:
    """One function body the unit's text defines."""
    name: str            # the declarator's own name: `FaceXZ`, `CFoo`, `~CFoo`, `operator=`
    scope: tuple         # its scope, outermost first, template arguments dropped: ('CAiGroup',)
    head: int            # where its declaration starts (after the previous statement)
    open: int            # the body's `{`
    close: int           # the body's `}`
    asm: bool = False    # an `asm` function: no C may be put in it

    def where(self, text: str) -> int:
        return text.count("\n", 0, self.open) + 1


_QUALS = r"((?:[A-Za-z_]\w*\s*(?:<[^;{}]*?>)?\s*::\s*)*)"


def _untemplate(s: str) -> str:
    """`vector<int, alloc<int> >` -> `vector`; whitespace dropped."""
    out, depth = [], 0
    for c in s:
        if c == "<":
            depth += 1
        elif c == ">":
            depth -= 1
        elif depth == 0 and not c.isspace():
            out.append(c)
    return "".join(out)


def _split_scope(quals: str) -> tuple:
    parts = [p for p in re.split(r"::", quals) if p.strip()]
    return tuple(_untemplate(p) for p in parts)


def declarator(header: str) -> tuple[str, tuple] | None:
    """(name, its own qualifiers) of the function a declaration header (the text from the previous
    statement to the body's `{`) defines: the identifier before the top-level parameter list that is
    followed only by `const`/`volatile`/`throw(...)` and the body, or by a constructor's `:`
    initialisers -- so a macro call before the signature is never taken for it. None when the header
    is not a function's."""
    groups, depth, start = [], 0, 0
    for i, c in enumerate(header):
        if c == "(":
            if depth == 0:
                start = i
            depth += 1
        elif c == ")":
            depth -= 1
            if depth < 0:
                return None
            if depth == 0:
                groups.append((start, i))
    if depth:
        return None
    for s, e in groups:
        before = header[:s].rstrip()
        if re.search(r"\boperator$", before):
            continue                         # `operator()`'s own parentheses
        rest = header[e + 1:]
        while True:
            r2 = re.sub(r"^\s*(?:const|volatile)\b", "", rest)
            r2 = re.sub(r"^\s*throw\s*\([^()]*\)", "", r2)
            if r2 == rest:
                break
            rest = r2
        rest = rest.strip()
        if rest and not (rest.startswith(":") and not rest.startswith("::")):
            continue
        m = re.search(_QUALS + r"operator\b\s*(.*?)$", before, re.S)
        if m:
            return "operator" + re.sub(r"\s+", "", m.group(2)) if re.match(r"\W", m.group(2).strip() or "x") \
                else "operator " + m.group(2).strip(), _split_scope(m.group(1))
        m = re.search(_QUALS + r"(~?\s*[A-Za-z_]\w*)\s*(?:<[^;{}()]*>)?\s*$", before)
        if not m:
            return None
        name = re.sub(r"\s+", "", m.group(2))
        if name in ("if", "while", "for", "switch", "return", "sizeof", "catch"):
            return None
        return name, _split_scope(m.group(1))
    return None


def sites(text: str) -> list[Site]:
    """Every function body `text` defines: at file scope, in a namespace or `extern "C"` block, or
    in a class body (an in-class definition, scoped by its class)."""
    b = blank(text)
    out: list[Site] = []
    stack: list[tuple] = []          # (kind, name, its `{` offset, header start, Site or None)
    boundary = 0                     # where the current declaration began
    i, n = 0, len(b)
    while i < n:
        c = b[i]
        if c == "{":
            kind = stack[-1][0] if stack else "file"
            if kind in ("func", "block"):
                stack.append(("block", "", i, 0, None))
            else:
                header = b[boundary:i]
                h = header.strip()
                if re.fullmatch(r"namespace(\s+[A-Za-z_]\w*)?", h):
                    stack.append(("ns", h.split()[1] if len(h.split()) > 1 else "", i, boundary, None))
                elif re.fullmatch(r'extern\s*"\s*"', h):
                    stack.append(("extern", "", i, boundary, None))
                else:
                    d = declarator(header) if "(" in header else None
                    cm = re.search(r"\b(class|struct|union)\s+([A-Za-z_]\w*)[^(]*$", header)
                    if d and not re.search(r"=\s*$", header):
                        scope = tuple(nm for k, nm, *_ in stack if k in ("ns", "class") and nm) + d[1]
                        site = Site(d[0], scope, boundary, i, -1, bool(re.search(r"\basm\b", header)))
                        stack.append(("func", d[0], i, boundary, site))
                    elif cm:
                        stack.append(("class", cm.group(2), i, boundary, None))
                    else:
                        stack.append(("block", "", i, boundary, None))      # an enum, an initialiser
            boundary = i + 1
        elif c == "}":
            if stack:
                kind, _, _, _, site = stack.pop()
                if kind == "func":
                    site.close = i
                    out.append(site)
            boundary = i + 1
        elif c == ";":
            boundary = i + 1
        elif c == ":" and stack and stack[-1][0] == "class" and re.search(
                r"\b(public|private|protected)\s*$", b[boundary:i]):
            boundary = i + 1
        i += 1
    return sorted(out, key=lambda s: s.open)


# --- the mangled name ---------------------------------------------------------------------------

OPERATORS = {
    "__as": "operator=", "__eq": "operator==", "__ne": "operator!=", "__pl": "operator+",
    "__mi": "operator-", "__ml": "operator*", "__dv": "operator/", "__md": "operator%",
    "__vc": "operator[]", "__cl": "operator()", "__apl": "operator+=", "__ami": "operator-=",
    "__aml": "operator*=", "__adv": "operator/=", "__amd": "operator%=", "__lt": "operator<",
    "__gt": "operator>", "__le": "operator<=", "__ge": "operator>=", "__nw": "operatornew",
    "__dl": "operatordelete", "__nwa": "operatornew[]", "__dla": "operatordelete[]",
    "__rf": "operator->", "__nt": "operator!", "__co": "operator~", "__aa": "operator&&",
    "__oo": "operator||", "__or": "operator|", "__ad": "operator&", "__er": "operator^",
    "__ls": "operator<<", "__rs": "operator>>", "__pp": "operator++", "__mm": "operator--",
    "__als": "operator<<=", "__ars": "operator>>=", "__aad": "operator&=", "__aor": "operator|=",
    "__aer": "operator^=", "__cm": "operator,", "__rm": "operator->*",
}
_MANGLED = re.compile(r"(__[a-z]+|.+?)__(?=[QFC0-9])(.*)$")


def demangle_scope(name: str) -> tuple[str, tuple]:
    """(the declarator name a definition of `name` spells, its scope, outermost first, template
    arguments dropped): `Tick__Q28CAiGroup10act_cameraFP11CSealCtrlAif` -> ('Tick', ('CAiGroup',
    'act_camera')), `__ct__5CFooFv` -> ('CFoo', ('CFoo',)), `_max<unsigned char>__FRCUcRCUc` ->
    ('_max', ()); a C name (or a placeholder) is itself, unscoped."""
    m = _MANGLED.match(name)
    if not m:
        return name, ()
    base, rest = m.group(1), m.group(2)
    parts: list[str] = []

    def read_len(s: str) -> tuple[int, str]:
        k = 0
        while k < len(s) and s[k].isdigit():
            k += 1
        return int(s[:k]), s[k:]
    try:
        if rest.startswith("Q"):
            count, rest = int(rest[1]), rest[2:]
            for _ in range(count):
                ln, rest = read_len(rest)
                parts.append(rest[:ln])
                rest = rest[ln:]
        elif rest[:1].isdigit():
            ln, rest = read_len(rest)
            parts.append(rest[:ln])
    except ValueError:
        return _untemplate(base), ()
    scope = tuple(_untemplate(p) for p in parts)
    if base == "__ct" and scope:
        return scope[-1], scope
    if base == "__dt" and scope:
        return "~" + scope[-1], scope
    if base in OPERATORS:
        return OPERATORS[base], scope
    if base.startswith("__op"):
        return "operator ", scope            # a conversion: any `operator <type>` body
    return _untemplate(base), scope


def candidates(name: str, all_sites: list[Site]) -> list[Site]:
    """The sites a definition of mangled `name` may be: the same declarator name, and the same scope
    -- the whole scope (an anonymous namespace, MW's `@unnamed@<file>@`, is the text's `namespace {`),
    else its innermost class, else any scope (a typedef'd class name); the probe build says which."""
    want, scope = demangle_scope(name)
    scope = tuple(p for p in scope if not p.startswith("@unnamed@"))
    if want == "operator ":
        same = [s for s in all_sites if s.name.startswith("operator ")]
    else:
        same = [s for s in all_sites if s.name == want]
    full = [s for s in same if s.scope == scope]
    if full:
        return full
    inner = [s for s in same if s.scope[-1:] == scope[-1:]]
    return inner or same


# --- building and scoring -----------------------------------------------------------------------

def is_mw_unit(path: Path) -> bool:
    if path.suffix not in (".c", ".cpp"):
        return False
    return not D.is_gcc(sccheck.tu_options(path).get("compiler", D.COMPILER))


def units(files: str | None = None) -> list[str]:
    """The hand-written Metrowerks units (paths under src/), those `files` matches."""
    rx = re.compile(files) if files else None
    out = []
    for p in sccheck.sources(sccheck.SRC):
        rel = p.relative_to(sccheck.SRC)
        if rel.parts[0] == "auto" or not is_mw_unit(p):
            continue
        if rx and not rx.search(str(rel)):
            continue
        out.append(str(rel))
    return out


def headers_digest() -> str:
    """One digest over every header the units can include: include/, and src/'s non-unit files."""
    h = hashlib.sha1()
    for root in (sccheck.INCLUDE, sccheck.SRC):
        for p in sorted(root.rglob("*")):
            if not p.is_file() or p.suffix in (".c", ".cpp", ".s", ".o") or "auto" in p.relative_to(root).parts \
                    or p.name in ("MATCHED.txt", "ABSENT.txt"):
                continue
            h.update(str(p.relative_to(root)).encode() + b"\0" + p.read_bytes())
    return h.hexdigest()[:16]


def digest(text: str) -> str:
    return hashlib.sha1(text.encode("utf-8", "surrogateescape")).hexdigest()[:16]


class Tree:
    """One job's scratch copy of src/ (drafts left out) and include/, at PATH_LENGTH characters."""

    def __init__(self, work: Path, k: int, length: int = PATH_LENGTH):
        import objcmp
        self.root = objcmp.side_root(work / f"j{k}", length)
        if self.root.exists():
            shutil.rmtree(self.root)
        shutil.copytree(sccheck.SRC, self.root / "src", ignore=shutil.ignore_patterns("auto", "*.o"))
        shutil.copytree(sccheck.INCLUDE, self.root / "include")
        self.obj = work / f"j{k}" / "obj"
        D._SHARED_STEMS.pop((self.root / "src").resolve(), None)

    def build(self, rel: str, text: str) -> tuple[Path | None, Path, str]:
        """Build unit `rel` with `text` in the tree's copy; the copy's own text is put back after."""
        wunit = self.root / "src" / rel
        orig = wunit.read_bytes()
        try:
            wunit.write_text(text, errors="surrogateescape")
            _, obj, err = sccheck.build_one(wunit, self.obj, root=self.root / "src",
                                            include=self.root / "include")
        finally:
            wunit.write_bytes(orig)
        return obj, wunit, err


def _absent() -> set[str]:
    if not sccheck.ABSENT.exists():
        return set()
    return {l.split("#")[0].strip() for l in sccheck.ABSENT.read_text().splitlines()} - {""}


def score_unit(obj: Path, wunit: Path, text: str) -> dict[str, dict]:
    """Every function of the object as sccheck scores it: {name: {match, verdict, addr, d}}; an
    inline copy missing at an address outside the unit's span is `elsewhere` (not open). The directive
    is read off `text` (the variant's), as the build read it."""
    img, st = E.image(), E.symbols()
    elf = E.Elf.read(obj)
    opts = {}
    for line in text.splitlines()[:3]:
        m = sccheck._DIRECTIVE.search(line)
        if m:
            for part in m.group(1).split(";"):
                if "=" in part:
                    k, v = part.split("=", 1)
                    opts.setdefault(k.strip(), v.strip())
    overlay = opts.get("overlay")
    span = sccheck.unit_span(obj, st, overlay)
    home = sccheck.unit_overlay(elf, st, overlay)
    absent = _absent()
    out: dict[str, dict] = {}
    for fs in elf.functions():
        if fs.name in absent:
            continue
        try:
            sym = sccheck.scoring_row(st, fs.name, home)
        except KeyError:
            continue
        if overlay and sccheck.overlay_symbol(st, fs.name, overlay) is None:
            continue
        res = D.compare(obj, fs.name, sym, img, st)
        if not res.match and sccheck.is_copy(elf, fs) and span and not span[0] <= sym.addr < span[1]:
            out[fs.name] = {"match": False, "verdict": "elsewhere", "addr": sym.addr}
            continue
        rec = {"match": bool(res.match), "verdict": "MATCH" if res.match else res.verdict, "addr": sym.addr}
        if not res.match:
            rec["d"], rec["size"] = distance(elf, fs.name, sym, img)
            rec["tsize"] = sym.size
        out[fs.name] = rec
    return out


def distance(elf: E.Elf, name: str, sym, img) -> tuple[int, int]:
    """try.py's d=: difflib's edit distance between the function's words and its row's, both masked
    by the candidate's relocations (the longer side of every unequal stretch); and its size."""
    cand, relocs, _ = elf.function(name)
    tw = E.words(D.mask(img.read(sym.addr, sym.size), relocs))
    cw = E.words(D.mask(cand, relocs))
    sm = difflib.SequenceMatcher(None, tw, cw, autojunk=False)
    return sum(max(i2 - i1, j2 - j1) for tag, i1, i2, j1, j2 in sm.get_opcodes() if tag != "equal"), len(cand)


def wrap(text: str, site: Site, pragma: str) -> str:
    """`text` with `site`'s body wrapped: `#pragma P off` its first line, `#pragma P reset` the line
    after its closing brace (the kit's round-58 form). A combination `P+Q` (--pairs) puts each off
    line in that order after the brace and each reset line in that order after the body."""
    names = pragma.split("+")
    o, c = site.open, site.close
    after_open = o + 2 if text[o + 1:o + 2] == "\n" else o + 1
    on = ("" if text[o + 1:o + 2] == "\n" else "\n") + "".join(f"#pragma {p} off\n" for p in names)
    after_close = c + 2 if text[c + 1:c + 2] == "\n" else c + 1
    off = ("" if text[c + 1:c + 2] == "\n" else "\n") + "".join(f"#pragma {p} reset\n" for p in names)
    return text[:after_open] + on + text[after_open:after_close] + off + text[after_close:]


def combinations(pragmas, pairs: bool = False) -> list[str]:
    """The variants to try: each pragma alone, or (--pairs) each pair of them, `P+Q` in list order."""
    if not pairs:
        return list(pragmas)
    return [f"{a}+{b}" for i, a in enumerate(pragmas) for b in pragmas[i + 1:]]


def probe_text(text: str, chosen: list[tuple[int, Site]]) -> str:
    """`text` with candidate k's body opened by a reference to `__pragmascan_probe_<k>` (two
    declarations, so a C89 block stays well-formed): the function whose code then names probe k is
    the one site k defines."""
    out, last = [], 0
    for k, s in sorted(chosen, key=lambda ks: ks[1].open):
        out.append(text[last:s.open + 1])
        out.append(f" extern void {PROBE}{k}(void); void (* volatile __pragmascan_ref_{k})(void) = {PROBE}{k}; ")
        last = s.open + 1
    out.append(text[last:])
    return "".join(out)


def probe_map(elf: E.Elf) -> dict[str, list[int]]:
    """{function: the probes its code references, in the order of their first reference}."""
    out: dict[str, list[int]] = {}
    for f in elf.functions():
        _, relocs, _ = elf.function(f.name)
        seen: list[int] = []
        for r in relocs:
            m = re.match(re.escape(PROBE) + r"(\d+)", r.sym.name)
            if m and int(m.group(1)) not in seen:
                seen.append(int(m.group(1)))
        if seen:
            out[f.name] = seen
    return out


# --- the pragmas' own check ---------------------------------------------------------------------

_FIXTURE = """int pragmascan_g[10];
int pragmascan_f(int *p, int n)
{
#pragma %s off
    int s = 0;
    for (int i = 0; i < n; i++) s += p[i] * 4 + pragmascan_g[i];
    return s;
}
#pragma %s reset
"""


def illegal_pragmas(pragmas, compilers, work: Path) -> list[str]:
    """The pragmas Metrowerks calls `illegal #pragma` under `-w all`, per toolchain ("P (toolchain)")."""
    bad = []
    work.mkdir(parents=True, exist_ok=True)
    for cc in sorted(compilers):
        for p in pragmas:
            src = work / f"pragma_{p}.cpp"
            src.write_text(_FIXTURE % (p, p))
            obj = src.with_suffix(".o")
            cmd = D.compile_cmd(src, obj, cc, None, [sccheck.INCLUDE])
            cmd = cmd[:-3] + ["-w", "all"] + cmd[-3:]
            r = subprocess.run(cmd, capture_output=True, text=True, timeout=120,
                               env=dict(os.environ, WINEDEBUG="-all"))
            if r.returncode != 0 or "illegal #pragma" in r.stdout + r.stderr:
                bad.append(f"{p} ({cc})")
    return bad


# --- the scan -----------------------------------------------------------------------------------

class Results:
    """The results file: one JSON line per base build, variant and budget stop; thread-safe."""

    def __init__(self, path: Path, fresh: bool = False):
        self.path = path
        self.lock = threading.Lock()
        self.base: dict[tuple, dict] = {}
        self.variants: dict[tuple, dict] = {}
        path.parent.mkdir(parents=True, exist_ok=True)
        if fresh and path.exists():
            path.unlink()
        if path.exists():
            for line in path.read_text().splitlines():
                try:
                    r = json.loads(line)
                except ValueError:
                    continue                  # a line cut off by a killed run
                self._index(r)

    def _index(self, r: dict) -> None:
        key = (r["unit"], r["sha"], r["hdr"])
        if r["kind"] == "base":
            self.base[key] = r
        elif r["kind"] == "variant":
            self.variants[key + (r["func"], r["pragma"])] = r

    def add(self, r: dict) -> None:
        with self.lock:
            with self.path.open("a") as f:
                f.write(json.dumps(r, sort_keys=True) + "\n")
            self._index(r)


@dataclass
class UnitPlan:
    rel: str
    open: list[str] = field(default_factory=list)
    sites: dict[str, Site] = field(default_factory=dict)
    skipped: dict[str, str] = field(default_factory=dict)


def locate(rel: str, text: str, names: list[str], tree: Tree) -> tuple[dict[str, Site], dict[str, str], dict]:
    """Each open function's site in the unit's text: its candidates by name and scope, then ONE probe
    build with a probe in every candidate body -- the function is the body whose probe its code
    references first (an overload set told apart, and a lone candidate confirmed: a name read wrong
    is never wrapped in silence). When the probe build fails, a lone candidate stands unconfirmed.
    ({name: site}, {name: why skipped}, {probe: built, ok, unconfirmed: [names]})."""
    all_sites = sites(text)
    found: dict[str, Site] = {}
    skipped: dict[str, str] = {}
    cands: dict[str, list[Site]] = {}
    for name in names:
        cs = [s for s in candidates(name, all_sites)]
        if not cs:
            want, scope = demangle_scope(name)
            skipped[name] = (f"no body in the unit's text ({'::'.join(scope + (want,))}: a header's, a template's, "
                             "a macro's, the compiler's)")
        elif all(s.asm for s in cs):
            skipped[name] = "an asm function"
        else:
            cands[name] = [s for s in cs if not s.asm]
    info = {"probe": 0, "ok": False, "unconfirmed": []}
    if not cands:
        return found, skipped, info
    chosen = sorted({id(s): s for cs in cands.values() for s in cs}.values(), key=lambda s: s.open)
    index = {id(s): k for k, s in enumerate(chosen)}
    obj, _, err = tree.build(rel, probe_text(text, list(enumerate(chosen))))
    info["probe"] = 1
    info["ok"] = obj is not None
    refs = probe_map(E.Elf.read(obj)) if obj is not None else {}
    for name, cs in cands.items():
        mine = [k for k in refs.get(name, []) if k in {index[id(s)] for s in cs}]
        if mine:
            found[name] = chosen[mine[0]]
        elif obj is None and len(cs) == 1:
            found[name] = cs[0]
            info["unconfirmed"].append(name)
        elif obj is None:
            skipped[name] = f"{len(cs)} candidate bodies, and the probe build failed: {err.strip()[-90:]}"
        else:
            skipped[name] = f"{len(cs)} candidate bodies, none referenced by its code"
    return found, skipped, info


def scan_unit(rel: str, tree: Tree, results: Results, hdr: str, pragmas=PRAGMAS, only: str | None = None,
              budget: float = 900.0, text: str | None = None, log=print) -> dict:
    """Scan one unit (`text`: in place of its own, as a fixture does). Returns the unit's summary:
    {open, skipped, done, left, built}."""
    t0 = time.time()
    text = (sccheck.SRC / rel).read_text(errors="surrogateescape") if text is None else text
    sha = digest(text)
    key = (rel, sha, hdr)
    built = 0
    base = results.base.get(key)
    if base is None:
        obj, wunit, err = tree.build(rel, text)
        built += 1
        if obj is None:
            base = {"kind": "base", "unit": rel, "sha": sha, "hdr": hdr, "error": err.strip()[-200:]}
            results.add(base)
            log(f"{rel}: BUILD-FAIL as it stands: {err.strip().splitlines()[-1][:120] if err.strip() else '?'}")
            return {"open": [], "skipped": {}, "done": 0, "left": 0, "built": built}
        scores = score_unit(obj, wunit, text)
        names = [n for n, r in scores.items() if not r["match"] and r["verdict"] != "elsewhere"]
        found, skipped, info = locate(rel, text, names, tree)
        built += info["probe"]
        base = {"kind": "base", "unit": rel, "sha": sha, "hdr": hdr, "scores": scores,
                "open": names, "sites": {n: [s.open, s.close, s.where(text)] for n, s in found.items()},
                "skipped": skipped, "probe_ok": info["ok"], "unconfirmed": info["unconfirmed"],
                "secs": round(time.time() - t0, 1)}
        results.add(base)
    if "error" in base:
        return {"open": [], "skipped": {}, "done": 0, "left": 0, "built": built}
    scores = base["scores"]
    rx = re.compile(only) if only else None
    todo = [(n, p) for n in base["open"] if n in base["sites"] and (not rx or rx.search(n)) for p in pragmas]
    matched = {n for n, r in scores.items() if r["match"]}
    done = left = 0
    for name, pragma in todo:
        if key + (name, pragma) in results.variants:
            done += 1
            continue
        if time.time() - t0 > budget:
            left += 1
            continue
        o, c, line = base["sites"][name]
        site = Site(name, (), 0, o, c)
        vtext = wrap(text, site, pragma)
        t1 = time.time()
        obj, wunit, err = tree.build(rel, vtext)
        built += 1
        rec = {"kind": "variant", "unit": rel, "sha": sha, "hdr": hdr, "func": name, "pragma": pragma,
               "line": line, "d0": scores[name].get("d"), "size0": scores[name].get("size")}
        if obj is None:
            rec.update(match=False, verdict="BUILD-FAIL " + (err.strip().splitlines() or ["?"])[-1][:120])
        else:
            now = score_unit(obj, wunit, vtext)
            me = now.get(name)
            if me is None:
                rec.update(match=False, verdict="gone (no such function in the variant's object)")
            else:
                rec.update(match=me["match"], verdict=me["verdict"], d1=me.get("d", 0), size1=me.get("size"))
            rec["lost"] = sorted(n for n in matched if not now.get(n, {}).get("match"))
            rec["gained"] = sorted(n for n, r in now.items() if r["match"] and n not in matched and n != name)
        rec["secs"] = round(time.time() - t1, 1)
        results.add(rec)
        done += 1
    if left:
        results.add({"kind": "budget", "unit": rel, "sha": sha, "hdr": hdr, "left": left})
    return {"open": base["open"], "skipped": base.get("skipped", {}), "done": done, "left": left, "built": built}


# --- the report ---------------------------------------------------------------------------------

def classify(r: dict) -> str:
    """HIT (byte-exact, nothing else lost), `hit-loses` (byte-exact, another function lost), `closer`
    (nearer its row, nothing lost), or '' (no better)."""
    if r.get("match"):
        return "HIT" if not r.get("lost") else "hit-loses"
    d0, d1 = r.get("d0"), r.get("d1")
    if d0 is not None and d1 is not None and d1 < d0 and not r.get("lost"):
        return "closer"
    return ""


def likeliest(results: Results, bases: dict, sizes: dict[int, int] | None = None) -> list[dict]:
    """The rows a variant brings to the target's size (GCC round 44a): per open function not made
    byte-exact, its best variant whose candidate is exactly the target row's size -- nearest first, a
    variant that loses nothing before one that does. `kind` is `at size` when the function as it stands
    is another size, `closer at size` when it is already that size and the variant is nearer. `sizes`
    ({address: the row's size}) is for a results file older than the `tsize` field: else the map's."""
    if sizes is None:
        sizes = {s.addr: s.size for s in E.symbols().symbols}
    best: dict[tuple, dict] = {}
    hit = {(k[:3], r["func"]) for k, r in results.variants.items() if r.get("match")}
    for k, r in results.variants.items():
        key = k[:3]
        if key not in bases or "error" in bases[key] or r.get("match") or (key, r["func"]) in hit:
            continue
        score = bases[key]["scores"].get(r["func"], {})
        tsize = score.get("tsize", sizes.get(score.get("addr")))
        d0, d1, size0, size1 = r.get("d0"), r.get("d1"), r.get("size0"), r.get("size1")
        if tsize is None or size1 != tsize or d1 is None or d0 is None:
            continue
        kind = "at size" if size0 != tsize else ("closer at size" if d1 < d0 else "")
        if not kind:
            continue
        row = dict(unit=key[0], func=r["func"], addr=score.get("addr"), pragma=r["pragma"], d0=d0, d1=d1,
                   size0=size0, tsize=tsize, kind=kind, lost=len(r.get("lost") or []))
        rank = (row["lost"] > 0, d1, len(r["pragma"].split("+")))
        if (key, r["func"]) not in best or rank < best[(key, r["func"])][0]:
            best[(key, r["func"])] = (rank, row)
    rows = [row for _, row in best.values()]
    return sorted(rows, key=lambda x: (x["kind"] != "at size", x["lost"] > 0, x["d1"], x["func"]))


def report(results: Results, hdr: str | None = None, rels: list[str] | None = None, out=print) -> dict:
    """Print the hits and the near ones unit by unit, then the totals and the likeliest next hits;
    return the counts."""
    current = {}
    for p in rels or []:
        t = (sccheck.SRC / p).read_text(errors="surrogateescape")
        current[p] = digest(t)
    bases = {}
    for (unit, sha, h), b in results.base.items():
        if (hdr is None or h == hdr) and (not current or current.get(unit) == sha):
            bases[(unit, sha, h)] = b
    counts = {"units": 0, "open": 0, "located": 0, "skipped": 0, "variants": 0, "HIT": 0, "hit-loses": 0,
              "closer": 0, "build-fail": 0}
    hits_by_unit: dict[str, list] = {}
    for key in sorted(bases):
        b = bases[key]
        if "error" in b:
            continue
        counts["units"] += 1
        counts["open"] += len(b["open"])
        counts["located"] += len(b["sites"])
        counts["skipped"] += len(b.get("skipped", {}))
        rows = [r for k, r in results.variants.items() if k[:3] == key]
        counts["variants"] += len(rows)
        counts["build-fail"] += sum(1 for r in rows if r["verdict"].startswith("BUILD-FAIL"))
        lines = []
        order = lambda p: [PRAGMAS.index(x) if x in PRAGMAS else 99 for x in p.split("+")]
        for r in sorted(rows, key=lambda r: (["HIT", "hit-loses", "closer", ""].index(classify(r)),
                                             r["func"], len(r["pragma"].split("+")), order(r["pragma"]))):
            c = classify(r)
            if not c:
                continue
            counts[c] += 1
            d = f"d {r.get('d0')} -> {r.get('d1')}"
            extra = ""
            if c == "hit-loses":
                extra = f"  loses {len(r['lost'])}: {', '.join(x[:40] for x in r['lost'][:4])}"
            elif c == "closer" and r.get("verdict"):
                extra = f"  ({r['verdict'][:48]})"
            if r.get("gained"):
                extra += f"  also matches {', '.join(x[:40] for x in r['gained'][:3])}"
            addr = b["scores"].get(r["func"], {}).get("addr")
            lines.append(f"  {c:<9} {r['func'][:58]:<58} {'' if addr is None else f'{addr:#x}':>9}  "
                         f"{r['pragma']:<22} {d}{extra}")
        if lines:
            hits_by_unit[key[0]] = lines
            out(f"{key[0]}: {len(b['open'])} open, {len(rows)} variants")
            for l in lines:
                out(l)
    out(f"\n{counts['units']} units, {counts['open']} open functions ({counts['located']} located in their "
        f"unit's text, {counts['skipped']} skipped), {counts['variants']} variants built "
        f"({counts['build-fail']} build failures): {counts['HIT']} HIT, {counts['hit-loses']} byte-exact but "
        f"losing another function, {counts['closer']} closer")
    rows = likeliest(results, bases)
    counts["at-size"] = sum(1 for r in rows if r["kind"] == "at size")
    counts["closer-at-size"] = len(rows) - counts["at-size"]
    out(f"\nTHE LIKELIEST NEXT HITS: {counts['at-size']} open rows a variant brings to the target's size, "
        f"{counts['closer-at-size']} already there and brought nearer (best variant per row, nearest first):")
    for r in rows:
        at = "" if r["addr"] is None else f"{r['addr']:#x}"
        lost = f"  loses {r['lost']}" if r["lost"] else ""
        was = "" if r["kind"] != "at size" else f"size {r['size0']:#x} -> {r['tsize']:#x}, "
        out(f"  {r['kind']:<14} {r['func'][:58]:<58} {at:>9}  {r['pragma']:<42} {was}d {r['d0']} -> {r['d1']}"
            f"  {r['unit']}{lost}")
    return counts


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--files", help="regex over unit paths under src/ (default: every hand-written MW unit)")
    ap.add_argument("-f", "--function", help="regex over the open functions' names: only those")
    ap.add_argument("--pragmas", default=",".join(PRAGMAS), help=f"comma-separated (default {','.join(PRAGMAS)})")
    ap.add_argument("-j", "--jobs", type=int, default=2)
    ap.add_argument("--unit-budget", type=float, default=900.0, help="seconds per unit (default 900)")
    ap.add_argument("--work", type=Path, default=WORK, help=f"scratch trees and objects (default {WORK})")
    ap.add_argument("--results", type=Path, help="the results file (default <work>/results.jsonl)")
    ap.add_argument("--fresh", action="store_true", help="discard the results file first")
    ap.add_argument("--report", action="store_true", help="print the results file's findings; build nothing")
    ap.add_argument("--pairs", action="store_true", help="each PAIR of the pragmas, `P+Q` (both off lines after "
                    "the brace, both resets after the body), instead of each alone")
    ap.add_argument("--list", action="store_true", help="list each unit's open functions and where each was "
                    "found (one build per unit, no variant)")
    a = ap.parse_args(argv)
    work = a.work.resolve()
    results = Results((a.results or work / "results.jsonl").resolve(), a.fresh)
    singles = [p.strip() for p in a.pragmas.split(",") if p.strip()]
    pragmas = combinations(singles, a.pairs)
    rels = units(a.files)
    hdr = headers_digest()
    if a.report:
        report(results, hdr, rels)
        return 0
    if not rels:
        raise SystemExit(f"pragmascan: no hand-written Metrowerks unit matches {a.files!r}")
    compilers = {sccheck.tu_options(sccheck.SRC / r).get("compiler", D.COMPILER) for r in rels}
    bad = illegal_pragmas(singles, compilers, work / "fixture")
    if bad:
        raise SystemExit(f"pragmascan: Metrowerks calls these `illegal #pragma` under -w all: {', '.join(bad)}")
    print(f"pragmascan: {len(rels)} units, pragmas {', '.join(singles)}{' in pairs' if a.pairs else ''} (each legal under "
          f"{', '.join(sorted(compilers))}), {a.jobs} jobs, {a.unit_budget:.0f} s a unit; results {results.path}",
          flush=True)
    trees: list[Tree] = []
    free: list[Tree] = []
    lock = threading.Lock()
    for k in range(max(1, a.jobs)):
        t = Tree(work, k)
        trees.append(t)
        free.append(t)

    def one(rel: str):
        with lock:
            tree = free.pop()
        try:
            t0 = time.time()
            s = scan_unit(rel, tree, results, hdr, pragmas if not a.list else [], a.function, a.unit_budget,
                          log=lambda m: print(m, flush=True))
            if a.list:
                b = results.base.get((rel, digest((sccheck.SRC / rel).read_text(errors="surrogateescape")), hdr), {})
                for n in b.get("open", []):
                    where = b.get("sites", {}).get(n)
                    print(f"  {rel}: {n} " + (f"line {where[2]}" + (" (unconfirmed: the probe build failed)"
                                                                   if n in b.get("unconfirmed", []) else "")
                                              if where else f"SKIP {b.get('skipped', {}).get(n, '?')}"), flush=True)
                if b.get("sites") and not b.get("probe_ok"):
                    print(f"  {rel}: the probe build failed", flush=True)
            print(f"{rel}: {len(s['open'])} open, {len(s['skipped'])} skipped, {s['done']} variants recorded"
                  + (f", {s['left']} left (budget)" if s["left"] else "") + f", {s['built']} built, "
                  f"{time.time() - t0:.0f} s", flush=True)
        except Exception as e:                # one unit's trouble never stops the scan
            print(f"{rel}: ERROR {type(e).__name__}: {e}", flush=True)
        finally:
            with lock:
                free.append(tree)
    with ThreadPoolExecutor(len(trees)) as ex:
        list(ex.map(one, rels))
    print()
    report(results, hdr, rels)
    return 0


if __name__ == "__main__":
    sys.exit(main())
