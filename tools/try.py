#!/usr/bin/env python3
"""
Try source variants of one function: each variant is compiled on its own and scored.

    ./tools/try.py src/gamez/zmath/zmath.h _close_enough<f>__FRCfRCfRCf_b --unit src/gamez/zmath/zmath_main.cpp \\
        --sub 'return !(d < -eps) && d <= eps;' 'return d >= -eps && d <= eps;' \\
        --sub ...
    ./tools/try.py src/gamez/zmath/zmath_pnt3d.cpp Interp__6CPnt3DCFPC6CPnt3DfP6CPnt3D -d   # as it is, with the diff
    ./tools/try.py src/lib/sdk/libscf.c frombcd -n --sub 'a;\\n\\tb;' 'b;\\n\\ta;'          # `\\n`: a newline
    ./tools/try.py src/lib/sdk/sifrpc.c sceSifCallRpc --sub 'int pid;' 'u_int pid;' --and 'pid = ' 'pid = (u_int)'
    ./tools/try.py src/lib/sdk/sifrpc.c sceSifCallRpc --variants build/scratch/v_types.py   # SUBS / OLD+V
    ./tools/try.py src/main/msl/foo.cpp __sinit_foo_cpp --target 0x1D4C10                  # another row
    ./tools/try.py include/vector X --unit src/gamez/zui/ui_list.cpp --all                    # every function

FILE is the file the substitutions apply to (a header -- under src/ or include/ -- or the unit);
the unit compiled is --unit, or FILE itself. One variant is one --sub, with every --and after it
applied too (an --and before the first --sub applies to every variant, the unchanged run included);
--variants FILE.py names variants in Python -- `SUBS = {name: [(old, new), ...]}`, or `OLD = '...'`
and `V = {name: new}` -- each variant's substitutions applied together (the AI core's round-23
tryv.py, folded in by the GCC lane's round 27a: a variant needing two substitutions could not be
expressed as one --sub).

**--sub and --and replace OLD's FIRST occurrence only** (FILE's first, wherever it lies); --sub-all
and --and-all replace EVERY occurrence (GCC round 42a: the kit's round 57 read a three-site change
as one), and the variant's label says how many (`NEW (all 3)`); in --variants a substitution
`(old, new, 'all')` replaces every one and `(old, new, N)` the first N, a pair the first. A --sub or
--and whose OLD occurs more than once says so on stderr.

    ./tools/try.py src/lib/sdk/tty.c sceTtyRead --sub-all 'buf[i] ==' 'buf[i] !='              # both sites

OLD and NEW are taken AS TYPED: a `\\n` in them is a backslash and an n, so a C escape in the source
(`'\\n'`, `"%d\\n"`) is matched and written as it stands -- and a NEW meant to break a line does not
build. -n/--newlines makes `\\n` a newline and `\\t` a tab in OLD and NEW alike; -e/--escapes decodes
the usual escapes -- `\\n`, `\\t`, `\\r`, `\\\\`, `\\"`, `\\'` -- in each NEW alone, OLD left as typed
(GCC round 43a: the HUD's round 63 read the literal as a bug). A variant that fails to build with
such a NEW, given neither flag, says so on stderr. With both, OLD takes -n's and each NEW -e's.

    ./tools/try.py src/lib/sdk/tty.c sceTtyRead -e --sub 'return i + 1;' 'return i + 1;\\n\\t/* x */'

src/ (drafts left out) and include/ are copied once to a scratch tree (build/try/, or
$SC_TRY_WORK -- one per concurrent user), so the tree is never touched, and each variant is built
there as sccheck builds it: under the unit's own directive (a GCC unit's `sc: compiler=...;
flags=...`, an assembler unit, `as=`, its statics qualified `NAME@UNIT`, `overlay=`) -- a variant
may change the directive too -- the unit keeping its path and file name (a `__sinit_<file>` keeps
its name). SYMBOL is scored against its row (the copy in the unit's overlay when the map lists one:
`ovl-copy:`), or against --target (a map name or an address: MAIN runtime round 6's v2.py).
--all scores every function the object defines and lists the ones that miss (exp.py's all-functions
score: a header change's other users); --order REGEX prints the object's function order.

Prints one line per variant: MATCH, or the differ's verdict and, beside it, the variant's difflib
distance from its row -- `d=N (0xCAND vs 0xTARGET)`: the words of every unequal stretch of the two
bodies, each masked by the candidate's relocations, the longer side counted (the AI core's round-34
vd.py, GCC round 39a) -- so variants of another size are ranked too; --no-dist prints the verdict alone.
"""

from __future__ import annotations

import argparse
import os
import re
import runpy
import shutil
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(HERE))
import scdecomp as E
import scdiff as D
import sccheck

# SC_TRY_WORK gives each concurrent user (an agent) a scratch tree of its own
SRC, INCLUDE = HERE / "src", HERE / "include"
WORK = Path(os.environ["SC_TRY_WORK"]).resolve() if os.environ.get("SC_TRY_WORK") else HERE / "build" / "try"


def _in_tree(path: Path) -> Path:
    """`path` (under src/ or include/) as the scratch tree's copy of it."""
    p = path.resolve()
    for real, name in ((SRC, "src"), (INCLUDE, "include")):
        try:
            return WORK / name / p.relative_to(real)
        except ValueError:
            pass
    raise SystemExit(f"{path}: not under {SRC} or {INCLUDE}")


def prepare() -> None:
    """The scratch tree: src/ without the drafts, and include/ -- copied once per run."""
    if WORK.exists():
        shutil.rmtree(WORK)
    shutil.copytree(SRC, WORK / "src", ignore=shutil.ignore_patterns("auto", "*.o"))
    shutil.copytree(INCLUDE, WORK / "include")


def build(file: Path, unit: Path, text: str | None) -> tuple[Path | None, str]:
    """Build `unit` in the scratch tree with `file`'s text replaced by `text` (None: as it is);
    `file` is put back afterwards, so the next variant starts from the tree's text."""
    wfile, wunit = _in_tree(file), _in_tree(unit)
    orig = wfile.read_bytes()
    try:
        if text is not None:
            wfile.write_text(text)
        _, obj, err = sccheck.build_one(wunit, WORK / "__obj__", root=WORK / "src", include=WORK / "include")
    finally:
        wfile.write_bytes(orig)
    return obj, err


def _last_error(err: str) -> str:
    """A build's last diagnostic line, its scratch-tree paths shortened to the tree's own."""
    lines = [l for l in err.replace(str(WORK) + "/", "").strip().splitlines() if l.strip()]
    first = [l for l in lines if re.search(r":\d+: (?!warning)|[Ee]rror", l)]   # GCC's first error, MW's
    return ((first or lines or ["?"]) if first else (lines or ["?"])[-1:])[0][:140]


def target_row(st, key: str):
    """--target: a map name or an address."""
    if re.fullmatch(r"(0x)?[0-9a-fA-F]{5,8}", key):
        row = st.at_addr(int(key, 16))
        if row is None:
            raise SystemExit(f"--target {key}: no row starts there")
        return row
    return st.get(key)


def score_obj(obj: Path, unit: Path, symbol: str, target: str | None = None, show: bool = False) -> str:
    """SYMBOL in `obj` against its scoring row (or --target's)."""
    img, st = E.image(), E.symbols()
    overlay = sccheck.tu_options(_in_tree(unit)).get("overlay")
    if target:
        sym = target_row(st, target)
    else:
        if overlay and sccheck.overlay_symbol(st, symbol, overlay) is None:
            return f"NO SYMBOL in {overlay}: {symbol}"
        try:
            # the row sccheck scores it against: in the overlay the unit is linked in, its copy there
            # when the named row lies in another (`ovl-copy:`)
            sym = sccheck.scoring_row(st, symbol, sccheck.unit_overlay(E.Elf.read(obj), st, overlay))
        except KeyError as e:
            return f"NO SYMBOL {e}"
    try:
        r = D.compare(obj, symbol, sym, img, st)
    except KeyError as e:
        return f"NO SYMBOL {e}"
    if show and not r.match:
        print(D.report(r))
    return "MATCH" if r.match else r.verdict


def distance(obj: Path, unit: Path, symbol: str, target: str | None = None) -> tuple[int, int, int] | None:
    """(distance, the candidate's words, the target's) -- difflib's edit distance between SYMBOL's
    words and its scoring row's (or --target's), both masked by the candidate's relocations: the
    longer side of every opcode that is not `equal` (the AI core's round-34 vd.py, folded in by the
    GCC lane's round 39a). 0 for a byte-exact body (a reference may still be wrong); None when the
    symbol or its row is missing. It ranks near misses, which a verdict's count of differing words
    at fixed offsets does not once the sizes differ."""
    import difflib
    img, st = E.image(), E.symbols()
    try:
        elf = E.Elf.read(obj)
        if target:
            sym = target_row(st, target)
        else:
            overlay = sccheck.tu_options(_in_tree(unit)).get("overlay")
            sym = sccheck.scoring_row(st, symbol, sccheck.unit_overlay(elf, st, overlay))
        cand, relocs, _ = elf.function(symbol)
    except (KeyError, SystemExit):
        return None
    if sym is None:
        return None
    tw = E.words(D.mask(img.read(sym.addr, sym.size), relocs))
    cw = E.words(D.mask(cand, relocs))
    sm = difflib.SequenceMatcher(None, tw, cw, autojunk=False)
    d = sum(max(i2 - i1, j2 - j1) for tag, i1, i2, j1, j2 in sm.get_opcodes() if tag != "equal")
    return d, len(cw), len(tw)


def dist_note(obj: Path, unit: Path, symbol: str, target: str | None = None) -> str:
    """`d=N (0xCAND vs 0xTARGET)` for a verdict's line, '' when distance() has none."""
    r = distance(obj, unit, symbol, target)
    return "" if r is None else f"d={r[0]} ({r[1] * 4:#x} vs {r[2] * 4:#x})"


def score_all(obj: Path, unit: Path) -> tuple[int, int, list[str]]:
    """Every function the object defines against its scoring row: (matched, scored, the misses)."""
    img, st = E.image(), E.symbols()
    elf = E.Elf.read(obj)
    overlay = sccheck.tu_options(_in_tree(unit)).get("overlay")
    home = sccheck.unit_overlay(elf, st, overlay)
    ok, n, misses = 0, 0, []
    for f in elf.functions():
        try:
            sym = sccheck.scoring_row(st, f.name, home)
        except KeyError:
            continue
        n += 1
        if D.compare(obj, f.name, sym, img, st).match:
            ok += 1
        else:
            misses.append(f.name)
    return ok, n, misses


def score(file: Path, unit: Path, symbol: str, text: str | None, show: bool = False,
          target: str | None = None) -> str:
    """One variant from a fresh scratch tree (the single-variant entry point other tools call)."""
    prepare()
    obj, err = build(file, unit, text)
    if obj is None:
        return "BUILD-FAIL " + _last_error(err)
    return score_obj(obj, unit, symbol, target, show)


class _Ordered(argparse.Action):
    """--sub and --and (and their -all forms) in the order given: each --sub starts a variant, each
    --and joins the last."""
    def __call__(self, parser, ns, values, option_string=None):
        steps = getattr(ns, self.dest, None) or []
        steps.append((option_string, values))
        setattr(ns, self.dest, steps)


def add_sub_options(ap: argparse.ArgumentParser) -> None:
    """--sub, --and, --sub-all and --and-all on `ap` (try.py's and tools/igvar.py's parsers)."""
    ap.add_argument("--sub", nargs=2, action=_Ordered, dest="steps", metavar=("OLD", "NEW"),
                    help="a variant: OLD replaced by NEW (its FIRST occurrence only)")
    ap.add_argument("--and", nargs=2, action=_Ordered, dest="steps", metavar=("OLD", "NEW"),
                    help="one more substitution in the last --sub's variant, OLD's first occurrence (before any "
                         "--sub: in every one)")
    ap.add_argument("--sub-all", nargs=2, action=_Ordered, dest="steps", metavar=("OLD", "NEW"),
                    help="a variant: EVERY occurrence of OLD replaced by NEW")
    ap.add_argument("--and-all", nargs=2, action=_Ordered, dest="steps", metavar=("OLD", "NEW"),
                    help="as --and, every occurrence of OLD replaced")


def apply_subs(text: str, subs) -> tuple[str, str | None, list[tuple[str, int, int]]]:
    """`text` with each substitution applied in turn: (old, new) replaces OLD's first occurrence (--sub,
    --and), (old, new, 'all') every one (--sub-all, --and-all, GCC round 42a), (old, new, N) the first
    N. (the text, the first OLD not in it or None, [(old, its occurrences there, how many replaced)])."""
    sites = []
    for sub in subs:
        old, new = sub[0], sub[1]
        count = sub[2] if len(sub) > 2 else 1
        n = text.count(old)
        if not n:
            return text, old, sites
        k = n if count == "all" else min(int(count), n)
        if k < 1:
            raise ValueError(f"a substitution's count must be 'all' or at least 1: {sub!r}")
        text = text.replace(old, new, k)
        sites.append((old, n, k))
    return text, None, sites


_ESCAPES = {"n": "\n", "t": "\t", "r": "\r", "\\": "\\", '"': '"', "'": "'"}


def decode_escapes(s: str) -> str:
    """`s` with the usual backslash escapes decoded -- \\n, \\t, \\r, \\\\, \\" and \\' (-e/--escapes,
    GCC round 43a); any other backslash is kept as written (`\\0`, `\\x41` stay as typed)."""
    return re.sub(r"\\(.)", lambda m: _ESCAPES.get(m.group(1), m.group(0)), s, flags=re.S)


def literal_escape(new: str) -> bool:
    """Does `new` hold a `\\n` or `\\t` typed as a backslash and a letter (what -e or -n decodes)?"""
    return re.search(r"(?<!\\)\\[nt]", new) is not None


def variants_from(a, base: str) -> list[tuple[str, list[tuple]]]:
    """[(label, [(old, new) | (old, new, 'all'), ...])]; an empty list is the file as it is."""
    common: list[tuple] = []
    out: list[tuple[str, list[tuple]]] = []
    escapes = getattr(a, "escapes", False)          # tools/igvar.py's parser has no -e
    for opt, (old, new) in a.steps or []:
        if a.newlines:
            old = old.replace("\\n", "\n").replace("\\t", "\t")
            if not escapes:
                new = new.replace("\\n", "\n").replace("\\t", "\t")
        if escapes:
            new = decode_escapes(new)
        sub = (old, new, "all") if opt in ("--sub-all", "--and-all") else (old, new)
        if opt in ("--sub", "--sub-all"):
            out.append((repr(new[:90]) if a.newlines or escapes else new[:90], [sub]))
        elif out:
            out[-1][1].append(sub)
        else:
            common.append(sub)
    # a --sub variant is labelled by its first NEW, and says how many --and joined it
    out = [(label + (f" (+{len(s) - 1} more)" if len(s) > 1 else ""), s) for label, s in out]
    if a.variants:
        ns = runpy.run_path(str(a.variants))
        subs = ns.get("SUBS") or {k: [(ns["OLD"], v)] for k, v in ns["V"].items()}
        out += [(str(name), list(s)) for name, s in subs.items()]
    if not out:
        out = [("", [])]
    return [(label, common + s) for label, s in out]


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("file", type=Path)
    ap.add_argument("symbol")
    ap.add_argument("--unit", type=Path)
    add_sub_options(ap)
    ap.add_argument("--variants", type=Path, help="variants in Python: SUBS = {name: [(old, new), ...]} or OLD and V "
                    "(a pair replaces OLD's first occurrence, (old, new, 'all') every one, (old, new, N) the first N)")
    ap.add_argument("--target", help="score SYMBOL against this row (a map name or an address)")
    ap.add_argument("--all", action="store_true", help="score every function the unit defines; list the misses")
    ap.add_argument("--order", help="print the object's functions (in its order) whose names match this regex")
    ap.add_argument("-n", "--newlines", action="store_true", help="`\\n` in OLD and NEW is a newline, `\\t` a tab")
    ap.add_argument("-e", "--escapes", action="store_true", help="each NEW's usual escapes decoded (\\n \\t \\r \\\\ "
                    "\\\" \\'), OLD left as typed; without -e or -n both are taken as typed")
    ap.add_argument("-d", "--diff", action="store_true", help="print the diff of a variant that fails")
    ap.add_argument("--no-dist", action="store_true",
                    help="print each verdict alone, without its difflib distance (the output before GCC round 39a)")
    a = ap.parse_args(argv)
    unit = a.unit or a.file
    base = a.file.read_text()
    prepare()
    warned, hinted = set(), False
    for label, subs in variants_from(a, base):
        text, missing, sites = apply_subs(base, subs)
        if missing is not None:
            print(f"(not in {a.file.name}: {missing[:60]!r})" + (f" | {label}" if label else ""))
            continue
        for sub, (old, n, k) in zip(subs, sites):
            if len(sub) == 2 and n > 1 and old not in warned:
                warned.add(old)
                print(f"try.py: {old[:60]!r} occurs {n} times in {a.file.name}: --sub/--and replace the FIRST "
                      f"(--sub-all/--and-all: every one)", file=sys.stderr)
        every = [k for sub, (_, _, k) in zip(subs, sites) if len(sub) > 2 and sub[2] == "all"]
        if every and label:
            label += f" (all {sum(every)})"
        obj, err = build(a.file, unit, text if subs else None)
        if obj is None:
            verdict = "BUILD-FAIL " + _last_error(err)
            if not (a.newlines or a.escapes) and any(literal_escape(s[1]) for s in subs) and not hinted:
                hinted = True
                print("try.py: a NEW holds `\\n` or `\\t` as typed (a backslash and a letter); -e decodes the usual "
                      "escapes in each NEW, -n makes \\n and \\t a newline and a tab in OLD and NEW", file=sys.stderr)
        elif a.all:
            ok, n, misses = score_all(obj, unit)
            verdict = f"{ok} of {n} match" + (f"; miss: {', '.join(m[:40] for m in misses[:8])}" if misses else "")
        else:
            verdict = score_obj(obj, unit, a.symbol, a.target, a.diff)
            if verdict != "MATCH" and not a.no_dist:
                note = dist_note(obj, unit, a.symbol, a.target)
                verdict += f"  {note}" if note else ""
        if obj is not None and a.order:
            rx = re.compile(a.order)
            fs = sorted(E.Elf.read(obj).functions(), key=lambda f: (f.shndx, f.value))
            for i, f in enumerate(fs):
                if rx.search(f.name):
                    print(f"  {i:3d} {f.name[:150]}")
        print(f"{verdict:<60} | {label}" if label else verdict)
    return 0


if __name__ == "__main__":
    sys.exit(main())
