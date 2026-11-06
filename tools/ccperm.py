#!/usr/bin/env python3
"""Try every order of the statements between `/*P{*/` and `/*}P*/` lines in a probe and report
the orders that match (README "The GCC front"): ccperm.py PROBE.c FUNCTION [TOOLCHAIN [FLAGS]]. (GCC's scheduler moves
independent stores; which source order gives the target's is easier measured than predicted.)
TOOLCHAIN defaults to ee-gcc2.96, FLAGS to `-O2 -G0`. `--stem UNIT` (anywhere): score the probe's statics as unit
UNIT's (`NAME@UNIT` rows). -h, --help: this text."""
import itertools, os, re, subprocess, sys
from pathlib import Path
D = Path(__file__).resolve().parents[1]
# the usage, before anything reads an argument as a file (GCC round 44a, the Medius lane's round 25b)
if "-h" in sys.argv[1:] or "--help" in sys.argv[1:]:
    print(__doc__)
    sys.exit(0)
# `--stem UNIT` (anywhere): score the probe's statics as unit UNIT's (`NAME@UNIT` rows), as sccheck would
stem = []
if "--stem" in sys.argv:
    i = sys.argv.index("--stem")
    stem = ["--stem", sys.argv[i + 1]]
    del sys.argv[i:i + 2]
if len(sys.argv) < 3:
    sys.exit(__doc__)
src = Path(sys.argv[1]).read_text()
fn = sys.argv[2]
tc = sys.argv[3] if len(sys.argv) > 3 else "ee-gcc2.96"
fl = sys.argv[4] if len(sys.argv) > 4 else "-O2 -G0"
m = re.search(r"^[ \t]*/\*P\{\*/[^\n]*\n(.*?)^[ \t]*/\*\}P\*/[^\n]*\n", src, re.S | re.M)
stmts = [l for l in m.group(1).splitlines(True) if l.strip()]
out = []
for i, perm in enumerate(itertools.permutations(stmts)):
    out.append(f"//== p{i}\n" + src[:m.start()] + "".join(perm) + src[m.end():])
# one file per process: two lanes (or two probes) can run at once
vf = D / "build" / "ccsweep" / f"perm_variants_{os.getpid()}.c"
vf.parent.mkdir(parents=True, exist_ok=True)
vf.write_text("\n".join(out))
try:
    # the variants are written under build/ccsweep: the probe's own directory stays on the include
    # path, so a probe may #include the header beside it
    r = subprocess.run([sys.executable, str(D / "tools" / "ccsweep.py"), str(vf), "--variants", "-t", tc,
                        "-F", fl, "-f", f"^{fn}$", "-I", str(Path(sys.argv[1]).resolve().parent)] + stem,
                       capture_output=True, text=True, cwd=D)
    perms = list(itertools.permutations(stmts))
    hits = 0
    for line in r.stdout.splitlines():
        mm = re.match(r"^p(\d+) .*\s(=|R)\s*$", line)
        if mm:
            hits += 1
            # R: every byte right, a reference not (a static the map qualifies: give --stem)
            print(f"MATCH order {mm.group(1)}" + " (bytes only: a reference differs)" * (mm.group(2) == "R")
                  + ":\n" + "".join(perms[int(mm.group(1))]))
            if hits >= 3:
                break
    if hits:
        print(f"{len(perms)} orders tried, {hits} shown matching")
    else:
        # none matches: the nearest orders (words differing; `Ns` a size miss ranks last)
        near = []
        for line in r.stdout.splitlines():
            mm = re.match(r"^p(\d+) .*\s(\d+)(s?)\s*$", line)
            if mm:
                near.append((int(mm.group(2)) + 1000 * bool(mm.group(3)), int(mm.group(1))))
        print(f"{len(perms)} orders tried, none matches" + (f"; nearest {near and min(near)[0]} words:" if near else ""))
        for score, i in sorted(near)[:2]:
            print(f"order {i} ({score}):\n" + "".join(perms[i]))
        if not near:
            print("\n".join(r.stdout.splitlines()[:3]) + r.stderr[-400:])
finally:
    vf.unlink(missing_ok=True)
