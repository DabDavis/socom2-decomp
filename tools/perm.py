#!/usr/bin/env python3
"""
A small permuter: reorder marked source lines until one function matches.

    ./tools/perm.py SRC FUNC [--target 0xADDR] [--seconds 300] [--jobs 7]

Mark what may move in the source (a scratch copy -- the input is never edited):

    // PERM_BEGIN          lines between these two are reorderable units
    ...                    (one statement per line)
    // PERM_END
    float a, b, c;  // PERM_DECL     the names in this declaration are reorderable

Each step mutates the best order so far (swap two units, or move one), compiles with
the settled toolchain and scores the function: differing words against the target
after the object's own relocation masking, plus a heavy penalty per word of size
difference. A step that scores no worse is kept, so the search drifts across ties;
parallel workers explore from the current best. It stops at a match (rc 0) or the
time limit (rc 1), printing the best source either way. Modelled on claude-eqoa's
eqperm.py, without its tree-sitter mutations: it only reorders what you mark.
"""

from __future__ import annotations

import argparse
import os
import random
import re
import sys
import tempfile
import time
from concurrent.futures import ProcessPoolExecutor
from pathlib import Path

HERE = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(HERE))
import scdecomp as E
import scdiff as D

_DECL = re.compile(r"^(\s*)([\w\s\*]+?)\s+([\w\s,\*\[\]]+);\s*//\s*PERM_DECL\s*$")


def parse(text: str):
    """(template lines, blocks): a block is ('lines', [line...]) or ('decl', indent, type, [names])."""
    lines = text.splitlines()
    out, blocks, i = [], [], 0
    while i < len(lines):
        ln = lines[i]
        if ln.strip() == "// PERM_BEGIN":
            j = i + 1
            unit = []
            while lines[j].strip() != "// PERM_END":
                unit.append(lines[j])
                j += 1
            out.append(("block", len(blocks)))
            blocks.append(["lines", unit])
            i = j + 1
            continue
        m = _DECL.match(ln)
        if m:
            out.append(("block", len(blocks)))
            blocks.append(["decl", m.group(1), m.group(2), [n.strip() for n in m.group(3).split(",")]])
            i += 1
            continue
        out.append(("text", ln))
        i += 1
    return out, blocks


def render(template, blocks, order) -> str:
    res = []
    for kind, v in template:
        if kind == "text":
            res.append(v)
            continue
        b, o = blocks[v], order[v]
        if b[0] == "lines":
            res.extend(b[1][k] for k in o)
        else:
            res.append(f"{b[1]}{b[2]} {', '.join(b[3][k] for k in o)};")
    return "\n".join(res) + "\n"


_CTX = {}


def _init(src_dir: str, func: str, target: int, includes: list[str]):
    img, st = E.image(), E.symbols()
    sym = st.at_addr(target) or st.lookup(hex(target))
    _CTX.update(img=img, st=st, sym=sym, func=func, src_dir=src_dir, includes=[Path(p) for p in includes],
                tmp=Path(tempfile.mkdtemp(prefix="perm", dir=HERE / "build")))


def score(text: str) -> int:
    c = _CTX
    src = c["tmp"] / ("cand" + c["suffix"]) if "suffix" in c else None
    src = c["tmp"] / "cand.cpp" if src is None else src
    src.write_text(text)
    obj = c["tmp"] / "cand.o"
    try:
        D.compile_obj(src, obj, includes=c["includes"] + [Path(c["src_dir"])])
        body, rel, _ = E.Elf.read(obj).function(c["func"])
    except Exception:
        return 1 << 30
    sym = c["sym"]
    t = D.mask(c["img"].read(sym.addr, max(sym.size, len(body))), rel)
    cw, tw = E.words(D.mask(body, rel)), E.words(t[:sym.size])
    diff = sum(1 for a, b in zip(tw, cw) if a != b) + 20 * abs(len(tw) - len(cw))
    if diff == 0:
        r = D.compare(obj, c["func"], sym, c["img"], c["st"])
        if not r.match:
            diff = 1
    return diff


def mutate(order, blocks, rng):
    o = [list(x) for x in order]
    movable = [i for i, x in enumerate(o) if len(x) > 1]
    i = rng.choice(movable)
    a, b = rng.sample(range(len(o[i])), 2)
    if rng.random() < 0.5:
        o[i][a], o[i][b] = o[i][b], o[i][a]
    else:
        o[i].insert(b, o[i].pop(a))
    return o


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("src", type=Path)
    ap.add_argument("func")
    ap.add_argument("--target", help="address (default: the map row named FUNC)")
    ap.add_argument("--seconds", type=float, default=300)
    ap.add_argument("--jobs", type=int, default=max(1, (os.cpu_count() or 2) - 1))
    ap.add_argument("--seed", type=int, default=None)
    ap.add_argument("-I", dest="includes", action="append", default=[])
    a = ap.parse_args(argv)
    st = E.symbols()
    target = int(a.target, 16) if a.target else st.get(a.func).addr
    template, blocks = parse(a.src.read_text())
    if not blocks:
        raise SystemExit("no PERM_BEGIN/PERM_END block or PERM_DECL line in the source")
    order = [list(range(len(b[1] if b[0] == "lines" else b[3]))) for b in blocks]
    includes = [str(HERE / "include")] + a.includes
    rng = random.Random(a.seed)
    deadline = time.time() + a.seconds
    with ProcessPoolExecutor(a.jobs, initializer=_init,
                             initargs=(str(a.src.parent.resolve()), a.func, target, includes)) as ex:
        best = list(ex.map(score, [render(template, blocks, order)]))[0]
        print(f"start: {best}", flush=True)
        steps = 0
        while best and time.time() < deadline:
            cands = [mutate(order, blocks, rng) for _ in range(a.jobs * 2)]
            scores = list(ex.map(score, [render(template, blocks, o) for o in cands]))
            steps += len(cands)
            k = min(range(len(cands)), key=lambda i: scores[i])
            if scores[k] <= best:
                if scores[k] < best:
                    print(f"  {steps:>6} steps: {scores[k]}", flush=True)
                best, order = scores[k], cands[k]
    print(f"\nbest {best} after {steps} steps\n")
    print(render(template, blocks, order))
    return 0 if best == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
