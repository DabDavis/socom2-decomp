#!/usr/bin/env python3
"""
autoprobe -- measure a function's size as MW's -inline auto sees it.

  tools/autoprobe.py PROBE.cpp [PROBE.cpp ...]

Each probe defines a function `f` and a `void caller()` that calls it. The size printed is
the least `#pragma inline_max_auto_size(N)` at which caller inlines f (by bisection, under
the settled flags). `-inline auto` takes a function whose size is at most the limit, which
is 15 by default (a sweep of zanim_anim.cpp at 15 matched the build without the pragma).

What the probes measured (README "What the source has to reproduce"): the size is the count
of statements in MW's lowered body -- an expression statement 1, a return 1 (the closing
return of a void function too), each branch 1 (`a || b` and `a && b` are two), a goto 1
(an if's else, a loop's jump to its test), a switch 1 -- the labels not counted, the
arguments and the expression's own size not counted, and the callees not counted (it is
measured before anything is inlined into the function, bottom-up or not). Statements that
compile to nothing still count: `(void)0;` is 1 and `if (0) f();` is 2, while `;`, `{}` and
`do {} while (0);` are 0.
"""

from __future__ import annotations

import re
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(HERE))
import scdecomp as E
import scdiff as D

OUT = HERE / "build" / "autoprobe"


def inlines_f(text: str, n: int) -> bool:
    OUT.mkdir(parents=True, exist_ok=True)
    src, obj = OUT / "probe.cpp", OUT / "probe.o"
    src.write_text(f"#pragma inline_max_auto_size({n})\n" + text)
    D.compile_obj(src, obj, includes=[HERE / "include"])
    dump = subprocess.run([E.tool("objdump"), "-dr", str(obj)], capture_output=True, text=True).stdout
    cur, calls = None, False
    for line in dump.splitlines():
        m = re.match(r"^[0-9a-f]+ <(.+)>:$", line)
        if m:
            cur = m.group(1)
        elif cur and cur.startswith("caller") and re.search(r"R_MIPS_26\s+f__", line):
            calls = True
    return not calls


def size(text: str, hi: int = 120) -> int | None:
    """The least limit that inlines f; None when even `hi` does not."""
    if not inlines_f(text, hi):
        return None
    lo = 0
    while hi - lo > 1:
        mid = (lo + hi) // 2
        if inlines_f(text, mid):
            hi = mid
        else:
            lo = mid
    return hi


def main(argv: list[str]) -> int:
    if not argv:
        print(__doc__)
        return 2
    for p in argv:
        print(f"{size(Path(p).read_text())!s:>5}  {p}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
