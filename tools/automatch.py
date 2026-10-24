#!/usr/bin/env python3
"""
Try m2c's draft of each function as-is: compile it with the settled toolchain and see
whether it is already byte-exact. An inventory, not source: a hit says the function
is trivially matchable and hands over a correct (if ugly) starting body.

    ./tools/automatch.py --max-size 0x40 --sample 200     # a random sample of small functions
    ./tools/automatch.py --range 0x306000 0x30A000        # every function in a range

Writes build/automatch/<VA>.c for every hit and prints the hit rate. With --emit it
also (re)writes src/auto/<segment>/<VA>.c, one file per function -- m2c's prototypes for
two functions can disagree, so they never share a translation unit. A function that
hand-written source already defines (or whose map row has a real name) is skipped, and
an auto file whose function is no longer a placeholder is deleted: src/auto is the
GENERATED tier, counted apart by sccheck, and hand-written source always supersedes it.
"""

from __future__ import annotations

import argparse
import random
import re
import subprocess
import sys
from concurrent.futures import ProcessPoolExecutor
from pathlib import Path

HERE = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(HERE))
sys.path.insert(0, str(HERE / "tools"))
import scdecomp as E
import scdiff as D
import m2c as M

OUT = HERE / "build" / "automatch"
PRELUDE = """typedef signed char s8; typedef unsigned char u8; typedef short s16; typedef unsigned short u16;
typedef int s32; typedef unsigned int u32; typedef long s64; typedef unsigned long u64;
typedef float f32; typedef double f64; typedef int M2C_UNK; typedef int M2C_UNK32;
#define M2C_FIELD(expr, type, offset) (*(type)((char *)(expr) + (offset)))
#define M2C_ERROR(x) 0
#define NULL 0
"""


TRIPLES = ("mipsee-gcc-c", "mipsee-mwcc-c")   # the second converts about 1 % more of the misses


def attempt(addr: int):
    img, st = E.image(), E.symbols()
    sym = st.at_addr(addr)
    real = sym.name
    if not C_IDENT.match(real):                 # a template instance: draft the placeholder,
        sym = E.Symbol(f"func_{addr:08X}", sym.addr, sym.size, sym.note)   # rename the object
    asm = M.export(img, st, sym)
    tmp = OUT / "work" / f"{addr:08X}"
    tmp.mkdir(parents=True, exist_ok=True)
    (tmp / "f.s").write_text(asm)
    worst = "m2c"
    for triple in TRIPLES:
        r = subprocess.run([sys.executable, str(M.M2C), "-t", triple, "--valid-syntax", str(tmp / "f.s")],
                           capture_output=True, text=True)
        if r.returncode != 0 or "M2C_ERROR" in r.stdout:
            continue
        # m2c leans on GNU C's arithmetic on void *, which MW rejects; char * means the same
        # byte offsets and compiles to the same code
        body = fill_params(re.sub(r"\bvoid(\s*\*)", r"char\1", r.stdout), sym.name)
        for variant in variants(body):
            src = PRELUDE + variant
            hit = try_one(tmp, src, sym, real, addr, img, st)
            if hit == "hit":
                if real != sym.name:
                    src = f"/* sc: as={sym.name} {real} */\n" + src
                return addr, "hit", src
            worst = hit if worst in ("m2c", "build") else worst
    return addr, worst, None


def fill_params(body: str, name: str) -> str:
    """m2c names a parameter by its register (arg0..arg3) and leaves out the unused ones, so
    `f(s32 arg0, s32 arg2)` would pass arg2 in a1: put the skipped ones back, unused."""
    m = re.search(rf"\b{re.escape(name)}\(([^)]*)\)\s*\{{", body)
    if not m or not m.group(1).strip() or m.group(1).strip() == "void":
        return body
    parts = [x.strip() for x in m.group(1).split(",")]
    idx = {}
    for x in parts:
        mm = re.match(r"^(.*?)\barg(\d)$", x)
        if not mm:
            return body                       # floats or stack arguments: leave it alone
        idx[int(mm.group(2))] = x
    full = [idx.get(i, f"s32 arg{i}") for i in range(max(idx) + 1)]
    return body[:m.start(1)] + ", ".join(full) + body[m.end(1):]


def variants(body: str):
    """The draft, then with narrow fields unsigned (a byte stored as 0xFF is `li 255` to an
    unsigned char, `li -1` to a signed one), then signed."""
    yield body
    u = re.sub(r"\bs(8|16)\b", r"u\1", body)
    if u != body:
        yield u


def try_one(tmp: Path, src: str, sym, real: str, addr: int, img, st) -> str:
    (tmp / "f.c").write_text(src)
    try:
        D.compile_obj(tmp / "f.c", tmp / "f.o")
        if real != sym.name:
            import sccheck
            sccheck.rename_symbol(tmp / "f.o", sym.name, real)
        res = D.compare(tmp / "f.o", real, st.at_addr(addr), img, st)
    except Exception:
        return "build"
    return "hit" if res.match else "miss"


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--max-size", type=lambda s: int(s, 0), default=0x40)
    ap.add_argument("--min-size", type=lambda s: int(s, 0), default=0)
    ap.add_argument("--sample", type=int, default=0)
    ap.add_argument("--range", nargs=2, type=lambda s: int(s, 16))
    ap.add_argument("--seed", type=int, default=1)
    ap.add_argument("--jobs", type=int, default=7)
    ap.add_argument("--emit", action="store_true", help="write hits into src/auto/")
    ap.add_argument("--redo", action="store_true", help="retry functions src/auto already holds")
    ap.add_argument("--addrs", type=Path, help="only these addresses (one hex VA per line)")
    ap.add_argument("--stale", action="store_true",
                    help="redo the drafts a rename left behind (implies --redo --emit, any size)")
    a = ap.parse_args(argv)
    img, st = E.image(), E.symbols()
    if a.stale:
        a.redo, a.emit, a.max_size = True, True, 0x100000
        want_stale = stale(st)
        print(f"{len(want_stale)} drafts spell a name the map no longer has")
    cands = [s.addr for s in st.symbols
             if img.is_code(s.addr) and a.min_size < s.size <= a.max_size
             and img.segment(s.addr).name != "MAIN" and s.addr % 16 == 0]
    if a.range:
        cands = [x for x in cands if a.range[0] <= x < a.range[1]]
    if a.addrs:
        want = {int(l, 16) for l in a.addrs.read_text().split()}
        cands = [x for x in cands if x in want]
    if a.stale:
        cands = [x for x in cands if x in want_stale]
    if a.sample and len(cands) > a.sample:
        cands = random.Random(a.seed).sample(cands, a.sample)
    hand = hand_defined()
    have = {int(f.stem, 16) for f in AUTO.rglob("*.c")} if not a.redo else set()
    cands = [x for x in cands if st.at_addr(x).name not in hand and x not in have]
    OUT.mkdir(parents=True, exist_ok=True)
    tally = {}
    hits = {}
    with ProcessPoolExecutor(a.jobs) as ex:
        for addr, kind, src in ex.map(attempt, sorted(cands)):
            tally[kind] = tally.get(kind, 0) + 1
            if src:
                (OUT / f"{addr:08X}.c").write_text(src)
                hits[addr] = src
    total = sum(tally.values())
    print(f"{total} tried: " + ", ".join(f"{k} {v}" for k, v in sorted(tally.items())))
    print(f"hit rate {100 * tally.get('hit', 0) / max(total, 1):.1f} %  (hits in {OUT})")
    if a.emit:
        emit(hits, img, st)
    return 0


AUTO = HERE / "src" / "auto"
# a generated draft defines the MAP's name -- a placeholder, or a real mangled name when it
# is a valid C identifier (most MW names are: Exp__5CQuatFPC6CPnt3D), so naming a function
# never costs its generated match; template names (<, >, ,) are left to hand-written source
C_IDENT = re.compile(r"^[A-Za-z_][A-Za-z0-9_]*$")
HEADER = """/* GENERATED by tools/automatch.py: m2c's draft of {name}, byte-exact as it stands
 * (every relocation resolved). Replace it with real source; the generator then deletes it. */
"""


def stale(st: E.SymbolTable) -> set[int]:
    """Drafts that define a name other than their row's, or call/reference a placeholder
    the map renamed since."""
    names = {s.name for s in st.symbols}
    out = set()
    for f in AUTO.rglob("*.c"):
        a, txt = int(f.stem, 16), f.read_text()
        s = st.at_addr(a)
        own = f"func_{a:08X}"
        gone = any(r not in names and r != own for r in re.findall(r"\b(func_[0-9A-F]{8})\b", txt))
        if s is None or not (f"{s.name}(" in txt or f" {s.name} */" in txt) or gone:
            out.add(a)
    return out


def hand_defined() -> set[str]:
    """Function names hand-written source defines (anything under src/ but src/auto)."""
    names = set()
    obj_root = HERE / "build" / "obj"
    for o in obj_root.rglob("*.o"):
        if "auto" in o.relative_to(obj_root).parts:
            continue
        try:
            names |= {f.name for f in E.Elf.read(o).functions()}
        except Exception:
            pass
    return names


def emit(hits: dict[int, str], img: E.Image, st: E.SymbolTable) -> None:
    written = 0
    if not hits:
        print("no new hits")
    for addr, src in sorted(hits.items()):
        seg = img.segment(addr).name.lower()
        d = AUTO / seg
        d.mkdir(parents=True, exist_ok=True)
        name = st.at_addr(addr).name
        (d / f"{addr:08X}.c").write_text(HEADER.format(name=name) + src)
        written += 1
    stale = 0
    hand = hand_defined()
    for f in AUTO.rglob("*.c"):
        s = st.at_addr(int(f.stem, 16))
        txt = f.read_text()
        defines = (f"{s.name}(" in txt or f" {s.name} */" in txt) if s is not None else False
        if s is None or s.name in hand or not defines:
            f.unlink()                      # gone, hand-written now, or renamed since (re-emitted below)
            stale += 1
    print(f"emitted {written} into {AUTO}; removed {stale} superseded")


if __name__ == "__main__":
    sys.exit(main())
