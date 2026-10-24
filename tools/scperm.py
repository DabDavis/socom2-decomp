#!/usr/bin/env python3
"""
The permuter: random source rewrites of ONE open function, compiled with the settled
Metrowerks toolchain and scored unattended, several a second on every core.

    .venv/bin/python tools/scperm.py UNIT SYMBOL [--minutes 10] [-j 6] [--out FILE]

UNIT is the .cpp holding the function; SYMBOL its mangled name (the map's). The function's
text is found in UNIT by its definition (`Class::Name(` / `name(`), and each candidate is
UNIT with that text replaced, built in a scratch copy of UNIT's directory (so its headers
resolve) -- the tree is never touched.

The mutations are claude-eqoa's (`eqperm_mut.py`, tree-sitter edits of one C++ function:
statement order, temporaries, declaration position and scope, if/else polarity, operand
order, `+=` forms, subscripts ...), imported from ~/claude-eqoa/decomp (or $EQPERM_MUT).
Needs `pip install tree-sitter tree-sitter-cpp` -- decomp/.venv has them.

The score is decomp-permuter's: the two instruction streams aligned on their opcode
skeletons (registers and immediates masked); an aligned pair that differs only in its
operands costs 5, an instruction on one side only 100. Register-only differences are
cheap, so the search can follow a gradient through allocation ties the byte count cannot
see. A candidate scoring 0 is re-scored by scdiff.compare (relocations resolved) before it
is called a match. A hit is bytes, not the original's spelling: read it before keeping it.
"""

from __future__ import annotations

import argparse
import difflib
import multiprocessing as mp
import os
import random
import re
import shutil
import sys
import tempfile
import time
from pathlib import Path

HERE = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(HERE))
import scdecomp as E
import scdiff as D

MUT = Path(os.environ.get("EQPERM_MUT") or Path.home() / "claude-eqoa" / "decomp")
sys.path.insert(0, str(MUT))


def find_function(text: str, symbol: str, line: int | None = None) -> tuple[int, int]:
    """[start, end) of the definition the symbol names: `Class::Name(...) ... {...}` -- the one
    starting at `line` when overloads share the name."""
    m = re.match(r"^(__ct__|__dt__|[A-Za-z_]\w*?)__(\d+)", symbol)
    if m and m.group(1) in ("__ct__", "__dt__"):
        n = int(re.match(r"\d+", symbol[6:]).group())
        cls = symbol[6 + len(str(n)):6 + len(str(n)) + n]
        name = cls if m.group(1) == "__ct__" else "~" + cls
        pat = rf"\b{cls}::{re.escape(name)}\s*\("
    elif m:
        n = int(m.group(2))
        cls = symbol[m.end():m.end() + n]               # MW mangling: the class name, length-prefixed
        pat = rf"\b{re.escape(cls)}::{re.escape(m.group(1))}\s*\("
    else:
        pat = rf"^[^\n;]*\b{re.escape(symbol.split('__')[0])}\s*\("
    hits = []
    for h in re.finditer(pat, text, flags=re.M):
        a = text.rfind("\n", 0, h.start()) + 1                 # the line the definition starts on
        brace = text.find("{", h.end())
        semi = text.find(";", h.end())
        if brace < 0 or (0 <= semi < brace):
            continue                                            # a declaration
        depth, i = 0, brace
        while True:
            depth += {"{": 1, "}": -1}.get(text[i], 0)
            if depth == 0:
                break
            i += 1
        hits.append((a, i + 1))
    if not hits:
        raise SystemExit(f"no definition of {symbol} found")
    if line is not None:
        starts = [(text.count("\n", 0, a) + 1, (a, b)) for a, b in hits]
        return min(starts, key=lambda x: abs(x[0] - line))[1]
    if len(hits) > 1:
        print(f"note: {len(hits)} definitions match; the first is taken (--line picks another)")
    return hits[0]


class Scorer:
    def __init__(self, unit: Path, symbol: str):
        self.unit, self.symbol = unit.resolve(), symbol
        self.img, self.st = E.image(), E.symbols()
        self.sym = self.st.get(symbol)
        self.target = self.img.read(self.sym.addr, self.sym.size)
        # a scratch copy of the whole tree, so a unit's relative includes resolve as in src/
        self.wd = Path(tempfile.mkdtemp(prefix="scperm", dir=HERE / "build"))
        shutil.copytree(HERE / "src", self.wd / "src", ignore=shutil.ignore_patterns("auto", "*.o"))
        self.src = self.wd / "src" / self.unit.relative_to((HERE / "src").resolve())
        self.obj = self.wd / "c.o"
        self.includes = [HERE / "include", self.wd / "src"]

    def build(self, text: str) -> tuple[bytes, list] | None:
        self.src.write_text(text)
        try:
            D.compile_obj(self.src, self.obj, includes=self.includes)
            cand, relocs, _ = E.Elf.read(self.obj).function(self.symbol)
        except Exception:
            return None
        return cand, relocs

    def score(self, text: str) -> int | None:
        built = self.build(text)
        if built is None:
            return None
        cand, relocs = built
        t = E.words(D.mask(self.target, relocs) if len(cand) <= len(self.target)
                    else D.mask(self.target + bytes(len(cand) - len(self.target)), relocs)[:len(self.target)])
        c = E.words(D.mask(cand, relocs))
        sk = lambda ws: [w >> 26 if w >> 26 else (0x40 | (w & 0x3F)) for w in ws]
        cost = 0
        for op, i1, i2, j1, j2 in difflib.SequenceMatcher(None, sk(t), sk(c), autojunk=False).get_opcodes():
            if op == "equal":
                cost += 5 * sum(1 for a, b in zip(t[i1:i2], c[j1:j2]) if a != b)
            else:
                cost += 100 * max(i2 - i1, j2 - j1)
        return cost

    def verify(self, text: str) -> bool:
        if self.build(text) is None:
            return False
        r = D.compare(self.obj, self.symbol, self.sym, self.img, self.st)
        return r.match


_W: dict = {}


def _init(unit: str, symbol: str, seed: int):
    import eqperm_mut as M
    _W.update(sc=Scorer(Path(unit), symbol), M=M, rng=random.Random(seed ^ os.getpid()))


def _step(args):
    base_text, fn_text, span = args
    M, rng, sc = _W["M"], _W["rng"], _W["sc"]
    mutated = fn_text
    for _ in range(rng.choice((1, 1, 1, 2, 2, 3))):
        try:
            nxt, _ = M.mutate(mutated, rng)          # (text, mutation name) or (None, None)
        except Exception:
            nxt = None
        if nxt:
            mutated = nxt
    if mutated == fn_text:
        return None
    mutated = mutated.replace("decltype(", "__typeof__(")
    a, b = span
    text = base_text[:a] + mutated + base_text[b:]
    s = sc.score(text)
    return (s, mutated) if s is not None else None


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("unit", type=Path)
    ap.add_argument("symbol")
    ap.add_argument("--minutes", type=float, default=10)
    ap.add_argument("-j", "--jobs", type=int, default=6)
    ap.add_argument("--seed", type=int, default=1)
    ap.add_argument("--out", type=Path)
    ap.add_argument("--line", type=int, help="the definition's line, when overloads share the name")
    a = ap.parse_args(argv)
    base = a.unit.read_text()
    span = find_function(base, a.symbol, a.line)
    fn = base[span[0]:span[1]]
    sc = Scorer(a.unit, a.symbol)
    best = sc.score(base)
    if best is None:
        raise SystemExit("the unit does not build as it is")
    print(f"start: {best}", flush=True)
    best_fn, elites = fn, [(best, fn)]
    deadline = time.time() + 60 * a.minutes
    tried = 0
    with mp.Pool(a.jobs, initializer=_init, initargs=(str(a.unit), a.symbol, a.seed)) as pool:
        while time.time() < deadline and best > 0:
            batch = []
            for _ in range(a.jobs * 4):
                # mostly from the best, sometimes from an elite: ties are walked, not refused
                s, f = elites[0] if random.random() < 0.6 else random.choice(elites)
                text_span = (span[0], span[0] + len(f))
                batch.append((base[:span[0]] + f + base[span[1]:], f, text_span))
            for res in pool.imap_unordered(_step, batch):
                tried += 1
                if res is None:
                    continue
                s, f = res
                if s <= best and f not in (e[1] for e in elites):
                    elites.append((s, f))
                    elites.sort(key=lambda e: e[0])
                    del elites[12:]
                    if s < best:
                        best, best_fn = s, f
                        print(f"  {tried:6} tried: {best}", flush=True)
                        if best == 0:
                            break
    out_text = base[:span[0]] + best_fn + base[span[1]:]
    if best == 0 and sc.verify(out_text):
        print(f"MATCH after {tried} candidates")
    else:
        print(f"best {best} after {tried} candidates")
    if a.out:
        a.out.write_text(out_text)
    print(best_fn)
    return 0 if best == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
