#!/usr/bin/env python3
"""Try every order of the statements between `/*P{*/` and `/*}P*/` lines in a probe and report
the orders that match (README "The GCC front"): ccperm.py PROBE.c FUNCTION [TOOLCHAIN [FLAGS]]. (GCC's scheduler moves
independent stores; which source order gives the target's is easier measured than predicted.)"""
import itertools, re, subprocess, sys
from pathlib import Path
D = Path(__file__).resolve().parents[1]
src = Path(sys.argv[1]).read_text()
fn = sys.argv[2]
tc = sys.argv[3] if len(sys.argv) > 3 else "ee-gcc2.96"
fl = sys.argv[4] if len(sys.argv) > 4 else "-O2 -G0"
m = re.search(r"^[ \t]*/\*P\{\*/[^\n]*\n(.*?)^[ \t]*/\*\}P\*/[^\n]*\n", src, re.S | re.M)
stmts = [l for l in m.group(1).splitlines(True) if l.strip()]
out = []
for i, perm in enumerate(itertools.permutations(stmts)):
    out.append(f"//== p{i}\n" + src[:m.start()] + "".join(perm) + src[m.end():])
vf = D / "build" / "ccsweep" / "perm_variants.c"
vf.parent.mkdir(parents=True, exist_ok=True)
vf.write_text("\n".join(out))
r = subprocess.run([sys.executable, str(D / "tools" / "ccsweep.py"), str(vf), "--variants", "-t", tc,
                    "-F", fl, "-f", f"^{fn}$"], capture_output=True, text=True, cwd=D)
perms = list(itertools.permutations(stmts))
hits = 0
for line in r.stdout.splitlines():
    mm = re.match(r"^p(\d+) .*\s=\s*$", line)
    if mm:
        hits += 1
        print(f"MATCH order {mm.group(1)}:\n" + "".join(perms[int(mm.group(1))]))
        if hits >= 3:
            break
print(f"{len(perms)} orders tried, {hits} shown matching" if hits else
      f"{len(perms)} orders tried, none matches\n" + "\n".join(r.stdout.splitlines()[:3]) + r.stderr[-400:])
