#!/usr/bin/env bash
# A private m2c for the generator: a copy of claude-eqoa's checkout (or $M2C_SRC) with
# tools/m2c-r5900.patch applied -- the R5900 FPU accumulator ops (mula.s, adda.s, suba.s,
# madda.s, msuba.s, madd.s, msub.s), which MW emits for ordinary `a*b + c*d` and stock m2c
# refuses. The copy lives in build/m2c (ignored); claude-eqoa's checkout is never touched.
# m2c is GPL-3.0 (its LICENSE is copied with it); the patch is a change to it.
set -euo pipefail
HERE="$(cd "$(dirname "$0")/.." && pwd)"
SRC="${M2C_SRC:-$HOME/claude-eqoa/decomp/tools/m2c}"
DST="$HERE/build/m2c"
rm -rf "$DST"
mkdir -p "$DST"
cp -r "$SRC/m2c" "$SRC/m2c.py" "$SRC/m2c_pycparser" "$SRC/LICENSE" "$DST/"
find "$DST" -name __pycache__ -type d -exec rm -rf {} +
patch -s -d "$DST" -p1 < "$HERE/tools/m2c-r5900.patch"
echo "m2c with the R5900 patch: $DST/m2c.py"
