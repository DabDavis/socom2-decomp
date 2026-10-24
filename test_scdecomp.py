#!/usr/bin/env python3
"""
Canary for the SOCOM II decomp harness (the shape of claude-eqoa's test_eqdecomp.py).

A checker that cannot fail reports success having compared nothing. Every test here
therefore pairs a positive control with a negative one where that is possible, and
the runner prints the number of assertions actually executed -- an inert suite shows
up as a low count rather than as a pass.

Run: ./test_scdecomp.py   (also works under pytest)
"""

from __future__ import annotations

import shutil
import struct
import sys
import tempfile
from dataclasses import replace
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
sys.path.insert(0, str(HERE / "tools"))

import scdecomp as E
import scdiff as D
import sccheck as C
import seed_symbols as S

CHECKS = 0
ZLIB = HERE / "src" / "zlib"
BUILD = HERE / "build" / "test"


def ck(cond, msg):
    global CHECKS
    CHECKS += 1
    assert cond, msg


def build(src: Path, name: str, compiler: str = D.COMPILER, flags=None) -> Path:
    obj = BUILD / f"{name}.o"
    D.compile_obj(src, obj, compiler, flags, [HERE / "include", src.parent])
    return obj


# --- the target ---------------------------------------------------------------

def test_inputs_are_pinned():
    """The three pieces load, and a one-byte change to an overlay is refused."""
    img = E.image()
    ck([s.name for s in img.segments] == ["MAIN", "OVL1", "OVL2"], "MAIN + OVL1 + OVL2")
    with tempfile.TemporaryDirectory(dir=HERE / "build") as d:
        for f in ("ftscore.bin", "zsealetc.bin"):
            shutil.copy(E.OVERLAYS / f, Path(d) / f)
        b = bytearray((Path(d) / "zsealetc.bin").read_bytes())
        b[0x1000] ^= 1
        (Path(d) / "zsealetc.bin").write_bytes(b)
        try:
            E.Image(overlays=Path(d))
            ck(False, "a tampered overlay must be refused")
        except RuntimeError:
            ck(True, "")


def test_mwo3_header_and_code_ranges():
    """The header is 0x40 bytes and code starts right after it; OVL1's text ends in VU1
    microcode, which is not R5900 code and must not be swept."""
    img = E.image()
    o1, o2 = img.segments[1], img.segments[2]
    ck(o1.header.name == "FTSCore.bin" and o1.header.kind == 1, "OVL1 is FTSCore")
    ck(o2.header.name == "ZSealEtc.bin" and o2.header.kind == 3, "OVL2 is ZSealEtc")
    ck(o1.code[0] == 0x1E7040 and o2.code[0] == 0x4C53C0, "code starts at base+0x40 (0x4C53C0 is the boot's entry)")
    w = img.word(0x1E7040)
    ck(w >> 16 == 0x27BD, "OVL1's first word is a prologue")
    ck(o1.code[1] == 0x3D5980 and o1.vu == (0x3D5980, 0x3DBFF8), "OVL1: code ends 0x3D5980, microcode to text end")
    ck(img.word(0x3D5944) != 0 and img.word(0x3D5948) == 0, "the last R5900 function's delay slot is 0x3D5944")


def test_vu_tail_has_no_false_positive_on_r5900_code():
    """The microcode detector must say 'none' over a span that is all R5900 code."""
    img = E.image()
    m = img.segments[0]
    ck(E.vu_tail(m.data, m.base, 0x1C5000, 0x1CA3D0) == 0x1CA3D0, "no VU1 microcode in MAIN's zlib/loader code")


def test_disasm_keeps_every_word():
    """objdump collapses zero runs to '...' in binary mode; every word must come back."""
    img, st = E.image(), E.symbols()
    s = st.lookup("0x307270")                       # CQuat::Normalize: nop nop before the jal
    rows = E.disasm(img.read(s.addr, s.size), s.addr)
    ck(len(rows) == s.size // 4, f"{len(rows)} rows for {s.size // 4} words")
    ck([a for a, _, _ in rows] == list(range(s.addr, s.end, 4)), "no address skipped")
    zeros = E.disasm(bytes(32), 0)
    ck(len(zeros) == 8 and all(w == 0 for _, w, _ in zeros), "eight zero words disassemble as eight nops")


# --- the map --------------------------------------------------------------------

def test_symbol_map_is_sane():
    img, st = E.image(), E.symbols()
    ck(len(st) > 13000, f"the seeded map has {len(st)} rows")
    prev = None
    for s in st.symbols:
        if prev is not None and prev.addr != s.addr:
            assert prev.end <= s.addr, f"{prev.name} 0x{prev.addr:08X}+0x{prev.size:X} overlaps {s.name}"
        prev = s
    ck(True, "no two rows overlap")
    ck(all(s.size > 0 for s in st.symbols), "every row has a size")
    ck(all(img.segment(s.addr) is not None for s in st.symbols), "every row is inside the image")


def test_sweep_finds_the_hard_boundaries():
    """Three boundaries the first drafts got wrong, each now pinned."""
    img = E.image()
    regs, starts, found = S.run(img)
    got = {a: b - a for _, a, b in found}
    ck(got.get(0x180E00) == 0x10, "the 16-byte `j` thunk at 0x180E00 ends at its delay slot")
    ck(0x1E7100 in got, "OVL1's entry loops forever and is followed by 0xFFFFFFFF; 0x1E7100 is a function")
    ck(got.get(0x1C5DA0) == 0x114, "the loader's overlay decode is 0x114 bytes")
    ck(not any(img.segments[1].vu[0] <= a < img.segments[1].vu[1] for a in got), "no function inside the microcode")


# --- the toolchain ----------------------------------------------------------------

def test_toolchain_is_settled_on_zlib():
    """zlib 1.1.4, unmodified: the settled build matches; 3.0b22-020926 -- the build the
    ELF's '2.4.1.01' .comment points at -- does not, at the same flags."""
    img, st = E.image(), E.symbols()
    good = build(ZLIB / "adler32.c", "adler32_good")
    res = D.compare(good, "adler32", st.get("adler32"), img, st)
    ck(res.match, f"adler32 with {D.COMPILER}: {res.verdict}")
    bad = build(ZLIB / "adler32.c", "adler32_020926", compiler="mwcps2-3.0b22-020926")
    res = D.compare(bad, "adler32", st.get("adler32"), img, st)
    ck(not res.match, "adler32 with 3.0b22-020926 must NOT match (it misses the short-loop nops)")
    # -O3,p still matches 18 of zlib's 21 (inflate among them); inflate_fast is one it misses
    o3 = build(ZLIB / "inffast.c", "inffast_O3", flags=[f if f != "-O4,p" else "-O3,p" for f in D.FLAGS])
    res = D.compare(o3, "inflate_fast", st.get("inflate_fast"), img, st)
    ck(not res.match, "inflate_fast at -O3,p must NOT match")
    o4 = build(ZLIB / "inffast.c", "inffast_O4")
    res = D.compare(o4, "inflate_fast", st.get("inflate_fast"), img, st)
    ck(res.match, "inflate_fast at -O4,p matches")


def test_relocation_check_catches_a_wrong_callee():
    """Masking alone accepts a call to the wrong function; the resolver must not."""
    img, st = E.image(), E.symbols()
    src = (HERE / "src" / "main" / "loader.c").read_text()
    with tempfile.TemporaryDirectory(dir=HERE / "build") as d:
        good = Path(d) / "good.c"
        good.write_text(src)
        bad = Path(d) / "bad.c"
        bad.write_text(src.replace("func_001C5EC0", "func_001C5EC4"))
        sym = st.get("func_001C5DA0")
        r = D.compare(build(good, "loader_good"), "func_001C5DA0", sym, img, st)
        ck(r.match, f"the loader matches: {r.verdict}")
        r = D.compare(build(bad, "loader_bad"), "func_001C5DA0", sym, img, st)
        ck(r.masked_equal and not r.match, "same bytes, wrong callee: WRONG REFERENCE")
        ck(any(f.status == "MISMATCH" and f.want == 0x1C5EC0 for f in r.relocs), "the bad call is named")


def test_jump_table_is_checked_by_target():
    """inflate_blocks' switch table must point into inflate_blocks at the same offsets;
    corrupt one entry in a copy of the image and the check must fail."""
    img, st = E.image(), E.symbols()
    obj = build(ZLIB / "infblock.c", "infblock")
    sym = st.get("inflate_blocks")
    r = D.compare(obj, "inflate_blocks", sym, img, st)
    tables = [f for f in r.relocs if f.symbol.endswith("(local)") and f.status == "content-ok"]
    ck(r.match and tables, "the jump table resolves into the function")
    seg = img.segments[0]
    table = tables[0].want
    bad = bytearray(seg.data)
    struct.pack_into("<I", bad, table - seg.base, sym.addr + 0x10)
    img2 = E.Image.__new__(E.Image)
    img2.segments = [replace(seg, data=bytes(bad))] + img.segments[1:]
    r = D.compare(obj, "inflate_blocks", sym, img2, st)
    ck(not r.match and any(f.status == "CONTENT-MISMATCH" for f in r.relocs), "a corrupted entry is caught")


def test_padding_after_a_function_must_be_zero():
    """A candidate one instruction short of a function whose next word is code fails."""
    img, st = E.image(), E.symbols()
    sym = st.get("uncompress")
    short = E.Symbol(sym.name, sym.addr, sym.size - 4)
    obj = build(ZLIB / "uncompr.c", "uncompr")
    r = D.compare(obj, "uncompress", short, img, st)
    ck(not r.match, "a wrong size in the map cannot match")


# --- SOCOM 1's demo, the name source --------------------------------------------------

def test_demo_relocations_land_on_their_fields():
    """`.relmain`'s r_offset is a virtual address: keyed one section base off, every exact
    comparison masked words 1 MB away from the real fields (and named SetStance wrongly)."""
    import s1names as N
    if not N.DEMO.exists():
        print("      (skipped: the May 2002 demo is not extracted)")
        return
    demo = N.Demo()
    by_type = {}
    for a, t in demo.relocs.items():
        by_type.setdefault(t, []).append(struct.unpack_from("<I", demo.data, a - demo.base)[0] >> 26)
    ck(len(by_type["R_MIPS_26"]) > 50000 and all(op in (2, 3) for op in by_type["R_MIPS_26"]),
       "every R_MIPS_26 is on a j/jal")
    ck(all(op == 0x0F for op in by_type["R_MIPS_HI16"]), "every R_MIPS_HI16 is on a lui")
    ck(all(op == 0x09 or op >= 0x1A for op in by_type["R_MIPS_LO16"]), "every R_MIPS_LO16 is on an addiu or a load/store")
    # negative control: one section base off, almost none would be
    off = sum(1 for a, t in demo.relocs.items() if t == "R_MIPS_26" and a + 0x100000 - demo.base < len(demo.data)
              and struct.unpack_from("<I", demo.data, a + 0x100000 - demo.base)[0] >> 26 in (2, 3))
    ck(off < len(by_type["R_MIPS_26"]) // 10, "the same table read one base off lands on calls by chance only")


def test_exact_pass_refuses_a_thin_wrapper():
    """`return DoSetStance(x);` masks to the same words as every `return f(x);`: the exact
    pass must not name it, and the callee pass must not inherit a name through it."""
    import s1names as N
    if not N.DEMO.exists():
        print("      (skipped: the May 2002 demo is not extracted)")
        return
    img, st, demo = E.image(), E.symbols(), N.Demo()
    exact, callee, _ = N.run(demo, img, st)
    ck(0x319580 not in exact, "the SetStance-shaped wrapper is not exact")
    ck(0x3199C0 not in callee, "nor is its callee named through it")
    ck(exact.get(0x3070C0) == "Mul__5CQuatCFPC5CQuatP5CQuat", "positive control: CQuat::Mul is exact")


def test_vtables_are_found_by_their_rtti():
    """CNode's table is found from its RTTI's name alone, with SOCOM II's extra slot; its
    destructor is recognised by writing the table; the aligner refuses to swap CBatchRelocator's
    slots the way an anchor on the align pass's wrong StartBatchReloc once did."""
    import s1names as N
    import s1vtables as V
    if not N.DEMO.exists():
        print("      (skipped: the May 2002 demo is not extracted)")
        return
    img, st, demo = E.image(), E.symbols(), N.Demo()
    tables = {cls: (vt, slots) for vt, _, cls, slots in V.our_vtables(img)}
    vt, slots = tables.get("zdb::CNode", (0, []))
    ck(vt == 0x4067B0 and len(slots) == 15, "CNode's vtable at 0x4067B0, 15 slots (SOCOM 1: 14)")
    al = V.Aligner(img, st, demo, 0.5, 0.03)
    ck(al.writes_vtable(slots[0], vt) if slots else False, "CNode's destructor stores its own vtable")
    ck(V.mangle_class("zdb::CCell") == "Q23zdb5CCell", "class names mangle as the demo's symbols")
    al.hand = set()
    th = V.demo_vtables(demo)["Q23zdb15CBatchRelocator"]
    vt, slots = tables["zdb::CBatchRelocator"]
    got = al.align(vt, slots, th, hide=0x3559E0).get(0x3559E0)
    ck(got is not None and got[0].startswith("AddToBatch__") and got[2], "slot 0 is AddToBatch, forced")


# --- the tree ---------------------------------------------------------------------

def test_everything_baselined_still_matches():
    rc = C.main([])
    ck(rc == 0, "sccheck: no regressions against MATCHED.txt, every TU builds")


def main() -> int:
    BUILD.mkdir(parents=True, exist_ok=True)
    tests = [(n, f) for n, f in globals().items() if n.startswith("test_") and callable(f)]
    failed = 0
    for name, fn in tests:
        try:
            fn()
            print(f"ok    {name}")
        except AssertionError as e:
            failed += 1
            print(f"FAIL  {name}: {e}")
    print(f"\n{len(tests) - failed}/{len(tests)} tests, {CHECKS} assertions")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
