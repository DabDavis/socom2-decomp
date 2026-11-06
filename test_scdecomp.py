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

import os
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


def test_mangled_names_spell_their_lengths():
    """Every mangled row's length prefixes agree with its name (tools/manglecheck.py, GCC round
    23a): a row spelled with a wrong length is a name no unit can define. Negatives: the UI
    lane's round-16 slip (70 for 72, unbalanced) is caught and corrected, and a length that
    swallows the const qualifier (`9bad_castCFv`, still well-formed) is suspect."""
    import manglecheck as MC
    rows = [r for r in MC.map_rows([E.SYMBOLS, E.DATA]) if MC.claims_mangled(r[3])]
    ck(len(rows) > 7000, f"{len(rows)} mangled rows read")
    bad = [r[3] for r in rows if not MC.parses(r[3])]
    ck(not bad, f"{len(bad)} mangled row(s) whose lengths do not parse: {bad[:3]}")
    voc = MC.vocabulary(r[3] for r in rows)
    sus = [r[3] for r in rows if MC.suspects(r[3], voc)]
    ck(not sus, f"{len(sus)} mangled row(s) spelling a name nowhere else spelled: {sus[:3]}")
    slip = "clear__Q23std70__vector_deleter<15CDlgDesignModel,Q23std28allocator<15CDlgDesignModel>>Fv"
    ck(not MC.parses(slip), "the UI's slip does not parse")
    ck(MC.corrections(slip) == [slip.replace("std70", "std72")], "and its one correction is 72")
    ck(MC.parses("what__Q23std9bad_castCFv") and MC.suspects("what__Q23std9bad_castCFv", voc),
       "a length one too long that swallows `C` is suspect")
    ck(not MC.parses("__dt__Q23std14length_errorFv"), "a destructor is no static datum")
    ck(MC.parses("@GUARD@id__Q23std8ctype<w>") and not MC.parses("@GUARD@id__Q23std9ctype<w>"),
       "a class template static member's guard parses, and its length is still checked")
    ck(not MC.parses("@LOCAL@id__Q23std8ctype<w>"), "a function-local static still needs its function and name")


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


def test_directive_takes_effect_or_refuses():
    """A unit's `sc: compiler=` without `flags=` builds under that toolchain at the default flags
    (the kit lane's question, GCC round 16a): adler32 under 3.0b22-020926 must miss as above, built
    as sccheck builds a unit. And a directive the tools cannot read is a build failure, not the
    defaults in silence: one past the third line, an unknown key, a toolchain that is not there."""
    img, st = E.image(), E.symbols()
    tree = BUILD / f"directive.{os.getpid()}"
    shutil.rmtree(tree, ignore_errors=True)
    shutil.copytree(ZLIB, tree, ignore=shutil.ignore_patterns("*.c", "*.md"))
    base = (ZLIB / "adler32.c").read_text()
    unit = tree / "adler32.c"

    def built(text):
        unit.write_text(text)
        _, obj, err = C.build_one(unit, tree / "obj", root=tree)
        return obj, err
    try:
        obj, err = built(base)
        ck(obj is not None and D.compare(obj, "adler32", st.get("adler32"), img, st).match,
           f"no directive: the settled build matches ({err})")
        obj, err = built("/* sc: compiler=mwcps2-3.0b22-020926 */\n" + base)
        ck(obj is not None, f"compiler= alone builds: {err}")
        ck(not D.compare(obj, "adler32", st.get("adler32"), img, st).match,
           "compiler=mwcps2-3.0b22-020926 alone takes effect (adler32 misses)")
        for text, why in (("/*\n*\n*/\n/* sc: compiler=mwcps2-3.0b22-020926 */\n" + base, "past the third line"),
                          ("/* sc: compilr=mwcps2-3.0b22-020926 */\n" + base, "unknown `sc:` key"),
                          ("/* sc: compiler=mwcps2-NOPE */\n" + base, "no such toolchain")):
            obj, err = built(text)
            ck(obj is None and why in err, f"a directive {why} is refused: {err!r}")
    finally:
        shutil.rmtree(tree, ignore_errors=True)


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


def _image_with_word(img: E.Image, addr: int, value: int) -> E.Image:
    """A copy of the image with one word changed."""
    segs = []
    for seg in img.segments:
        if seg.base <= addr < seg.base + len(seg.data):
            bad = bytearray(seg.data)
            struct.pack_into("<I", bad, addr - seg.base, value)
            seg = replace(seg, data=bytes(bad))
        segs.append(seg)
    img2 = E.Image.__new__(E.Image)
    img2.segments = segs
    return img2


def test_gnu_table_is_compared_whole():
    """A GNU object relocates its statics against the section, so only the label's size says how
    far a table reaches. FillLanguageVirtualTable copies the recogniser's 79-slot _LangVirtualTable,
    whose first slot is null: read as a string, it was compared over one byte (round 6a). Whole, its
    slots resolve through the candidate's own functions by name (language.c's OPEN functions compile
    to other sizes, so 73 slots sit at other distances than the target's): a slot pointing elsewhere
    must fail. And a GNU float literal is compared over the width its load reads -- the codec's
    1.40234375f (0x3FB38000) starts with a zero byte, which the string rule read as the whole."""
    img, st = E.image(), E.symbols()
    lang = HERE / "src" / "lib" / "asr" / "language.c"
    opts = C.tu_options(lang)
    obj = build(lang, "language", opts["compiler"], opts["flags"].split())
    D.qualify_statics(obj, lang.stem, st)
    sym = st.get("FillLanguageVirtualTable")
    r = D.compare(obj, sym.name, sym, img, st)
    table = st.data_by_name["_LangVirtualTable"]
    ck(r.match and any(f.symbol == "_LangVirtualTable" and f.status == "ok" for f in r.relocs),
       f"the table resolves, slot by slot: {r.verdict}")
    slot40 = table.addr + 40 * 4
    r = D.compare(obj, sym.name, sym, _image_with_word(img, slot40, img.word(slot40) + 8), st)
    ck(not r.match and any(f.status == "CONTENT-MISMATCH" for f in r.relocs), "a slot naming another address fails")
    nelly = HERE / "src" / "lib" / "ovl1" / "nelly.c"
    opts = C.tu_options(nelly)
    obj = build(nelly, "nelly", opts["compiler"], opts["flags"].split())
    sym = st.get("func_00251700")
    lits = [f.want for f in D.compare(obj, sym.name, sym, img, st).relocs if f.type == "R_MIPS_LITERAL"]
    lit = next(a for a in lits if img.word(a) == 0x3FB38000)
    r = D.compare(obj, sym.name, sym, _image_with_word(img, lit, 0x3FB48000), st)
    ck(not r.match and any(f.status == "CONTENT-MISMATCH" for f in r.relocs), "a float literal's high bytes count")


def test_gnu_literal_is_its_own_entry():
    """A GNU .lit4 reference is compared over its own entry: the object's pool holds its constants
    in first-use order, the image's in the link's (round 12a). QP0SC3's dequantiser loads
    0.0117479693 (0x3C407A8E at 0x1D544C) and 5.0751734 (0x1D5444); the word after the first is
    another function's constant -- changing it must not matter, changing the entry itself must fail."""
    img, st = E.image(), E.symbols()
    unit = HERE / "src" / "lib" / "ovl1" / "sase_misc.c"
    opts = C.tu_options(unit)
    obj = build(unit, "sase_misc", opts["compiler"], opts["flags"].split())
    sym = st.get("func_00250C68")
    r = D.compare(obj, sym.name, sym, img, st)
    ck(r.match, f"the dequantiser's literals each match their entry: {r.verdict}")
    r = D.compare(obj, sym.name, sym, _image_with_word(img, 0x1D5450, img.word(0x1D5450) ^ 0x100), st)
    ck(r.match, "the next entry, another function's constant, is not this reference's")
    r = D.compare(obj, sym.name, sym, _image_with_word(img, 0x1D544C, 0x3C407A8F), st)
    ck(not r.match and any(f.status == "CONTENT-MISMATCH" for f in r.relocs), "the entry itself counts")


def test_gnu_code_label_resolves_by_the_map():
    """An asm's data label inside a GNU object's code (`.text`+N: GNU as relocates the local label
    against its section) resolves through the data map when it names the label: libmpeg's
    _copyAddRefImage loads _maxval, the clamp constant _copyRefImage's asm carries. By distance the
    reference went unchecked while the unit's layout before it is short; by the label it is checked
    -- and the label mapped elsewhere must fail."""
    img, st = E.image(), E.symbols()
    unit = HERE / "src" / "lib" / "sdk" / "mpeg_dec.c"
    opts = C.tu_options(unit)
    obj = build(unit, "mpeg_dec", opts["compiler"], opts["flags"].split())
    sym = st.get("_copyAddRefImage")
    r = D.compare(obj, sym.name, sym, img, st)
    ck(r.match and any(f.symbol == "_maxval" and f.status == "ok" for f in r.relocs),
       f"_maxval resolves by its data row: {r.verdict}")
    moved = HERE / "build" / "tmp" / f"data_moved.{os.getpid()}.txt"
    moved.parent.mkdir(parents=True, exist_ok=True)
    moved.write_text(E.DATA.read_text().replace("0x001B6E90 0x0 _maxval", "0x001B6EA0 0x0 _maxval"))
    try:
        r = D.compare(obj, sym.name, sym, img, E.SymbolTable(E.SYMBOLS, moved))
    finally:
        moved.unlink()
    ck(not r.match and any(f.symbol == "_maxval" and f.status == "MISMATCH" for f in r.relocs),
       "the label mapped elsewhere fails")


def test_shared_stem_qualifies_by_path():
    """A GNU unit's statics score as `NAME@UNIT` rows; libkernl's ExecPS2 object (sdk/exit.c) and
    newlib's (libc/exit.c) share the stem `exit`, so a bare `kCopy@exit` would score either
    object's static `kCopy` as the one row. A shared stem qualifies only by a tail of the unit's
    path (`kCopy@sdk/exit`): sdk/exit.c's static resolves so, libc/exit.c's does not, and the
    bare-stem row no longer resolves for either -- while a unique stem (tlbfunc) still does."""
    st = E.symbols()
    sdk, libc = HERE / "src" / "lib" / "sdk" / "exit.c", HERE / "src" / "lib" / "libc" / "exit.c"
    ck(D.unit_qualifiers(sdk) == ["lib/sdk/exit", "sdk/exit"], f"no bare stem: {D.unit_qualifiers(sdk)}")
    ck("exit" not in D.unit_qualifiers(libc), "nor for the other unit of that stem")
    ck(D.unit_qualifiers(HERE / "src" / "lib" / "sdk" / "tlbfunc.c")[-1] == "tlbfunc", "a unique stem stands")
    opts = C.tu_options(sdk)
    obj = build(sdk, "exit", opts["compiler"], opts["flags"].split())
    ck(("kCopy", "kCopy@sdk/exit") in D.qualify_statics(obj, sdk, st), "sdk/exit.c's kCopy is its row")
    obj = build(sdk, "exit", opts["compiler"], opts["flags"].split())
    ck(not any(b == "kCopy@sdk/exit" for _, b in D.qualify_statics(obj, libc, st)),
       "the other unit of that stem does not take it")
    rows = HERE / "build" / "tmp" / f"syms_bare.{os.getpid()}.txt"
    rows.parent.mkdir(parents=True, exist_ok=True)
    rows.write_text(E.SYMBOLS.read_text().replace(" kCopy@sdk/exit ", " kCopy@exit "))
    try:
        obj = build(sdk, "exit", opts["compiler"], opts["flags"].split())
        bare = D.qualify_statics(obj, sdk, E.SymbolTable(rows, E.DATA))
    finally:
        rows.unlink()
    ck(not any(b == "kCopy@exit" for _, b in bare), "a bare row of a shared stem resolves for neither")


def test_mw_statics_qualify_by_unit():
    """Two Metrowerks units' file statics can carry one name: SOCOM 1 calls both grid_di.cpp's and
    grid_clutter.cpp's `static CStack` `stack`, MW names each object's LOCAL symbol so, and the data
    map holds grid_di's bare and grid_clutter's as `stack@grid_clutter` (scdiff.qualify_statics, GNU
    objects' only before GCC round 25a, so the clutter's was spelt `clutterStack`). Built under
    SOCOM 1's name, grid_clutter's initialiser resolves its static through the qualified row; with
    no qualified row the bare name is grid_di's and the reference fails, so does the qualified row
    moved -- and grid_di.cpp's own `stack` still scores bare."""
    import re
    img, st = E.image(), E.symbols()
    znode = HERE / "src" / "gamez" / "znode"
    text = (znode / "grid_clutter.cpp").read_text()
    var = re.search(r"static CStack (\w+)\(false\);", text).group(1)
    tmp = HERE / "build" / "tmp" / f"qual.{os.getpid()}"
    tmp.mkdir(parents=True, exist_ok=True)
    unit = tmp / "grid_clutter.cpp"
    unit.write_text(re.sub(rf"\b{var}\b", "stack", text))
    data = E.DATA.read_text()
    clutter = next(l for l in data.splitlines(True) if l.startswith("0x0044F0F0 "))
    rows = tmp / "data.txt"
    try:
        def table(row: str) -> E.SymbolTable:
            rows.write_text(data.replace(clutter, row))
            return E.SymbolTable(E.SYMBOLS, rows)

        def score(row: str | None):
            obj = BUILD / "grid_clutter_q.o"
            D.compile_obj(unit, obj, includes=[HERE / "include", HERE / "src", znode])
            t = table(row) if row is not None else st
            pairs = D.qualify_statics(obj, unit, t)
            sym = st.get("__sinit_grid_clutter.cpp")
            return pairs, D.compare(obj, sym.name, sym, img, t)

        qual = "0x0044F0F0 0x1 stack@grid_clutter  # the canary's\n"
        pairs, r = score(qual)
        ck(("stack", "stack@grid_clutter") in pairs, f"the clutter's `stack` is its unit's row: {pairs}")
        ck(r.match and any(f.symbol == "stack@grid_clutter" and f.want == 0x44F0F0 and f.status == "ok"
                           for f in r.relocs), f"and its initialiser resolves it there: {r.verdict}")
        # the map itself holds the qualified row since the engine core's round 6 renamed the
        # static: take it out (an unqualified name at 0x44F0F0) rather than score the map as is
        pairs, r = score("0x0044F0F0 0x1 clutterStack  # the canary's: no qualified row\n")
        ck(not pairs and not r.match and any(f.symbol == "stack" and f.got == 0x44D750 and f.status == "MISMATCH"
                                             for f in r.relocs), "with no qualified row, the bare name is grid_di's: fails")
        pairs, r = score(qual.replace("0x0044F0F0", "0x0044F100"))
        ck(not r.match and any(f.symbol == "stack@grid_clutter" and f.status == "MISMATCH" for f in r.relocs),
           "the qualified row moved fails")
        obj = BUILD / "grid_di_q.o"
        D.compile_obj(znode / "grid_di.cpp", obj, includes=[HERE / "include", HERE / "src"])
        t = table(qual)
        ck(not any(a == "stack" for a, _ in D.qualify_statics(obj, znode / "grid_di.cpp", t)),
           "grid_di.cpp's own static is not renamed")
        sym = st.get("__sinit_grid_di.cpp")
        r = D.compare(obj, sym.name, sym, img, t)
        ck(r.match and any(f.symbol == "stack" and f.want == 0x44D750 and f.status == "ok" for f in r.relocs),
           f"and still resolves bare: {r.verdict}")
    finally:
        shutil.rmtree(tmp, ignore_errors=True)


def test_mw_function_static_resolves_by_its_function():
    """Metrowerks names a function-local static of a function emitted once `name$N` and its guard
    `@GUARD@name$N`, N a per-unit counter no map row can carry (an inline function's own are
    `@LOCAL@<fn>@name`, `@GUARD@<fn>@name`). The data map names both kinds the second way, and the
    first resolves through the compared function's row: the HUD lane's ShowArrows reads its
    `static CPnt3D s_grey` under its guard (8 references, unchecked before). Its rows moved in the
    map must fail; its rows gone, the references are unchecked again -- never passed."""
    img, st = E.image(), E.symbols()
    unit = HERE / "src" / "Apps" / "FTS" / "hud_mpweaponsel.cpp"
    obj = BUILD / "hud_mpweaponsel.o"
    D.compile_obj(unit, obj, includes=[HERE / "include", HERE / "src"])
    sym = st.get("ShowArrows__19CMPWeaponCategoriesFv")
    guard, local = (f"@{k}@{sym.name}@s_grey" for k in ("GUARD", "LOCAL"))
    r = D.compare(obj, sym.name, sym, img, st)
    ck(r.match and not r.reloc_unchecked, f"s_grey and its guard resolve by their rows: {r.verdict}")
    ck(sum(f.symbol == guard and f.status == "ok" for f in r.relocs) == 2
       and sum(f.symbol == local and f.status == "ok" for f in r.relocs) == 6, "two guard, six object references")
    rows = HERE / "build" / "tmp" / f"data_fnstatic.{os.getpid()}.txt"
    rows.parent.mkdir(parents=True, exist_ok=True)
    try:
        rows.write_text(E.DATA.read_text().replace(f"0x004147D0 0xC {local} ", f"0x004147E0 0xC {local} "))
        r = D.compare(obj, sym.name, sym, img, E.SymbolTable(E.SYMBOLS, rows))
        ck(not r.match and any(f.symbol == local and f.status == "MISMATCH" for f in r.relocs),
           "the object mapped elsewhere fails")
        rows.write_text("".join(l for l in E.DATA.read_text().splitlines(True) if "@s_grey " not in l))
        r = D.compare(obj, sym.name, sym, img, E.SymbolTable(E.SYMBOLS, rows))
        ck(r.match and len(r.reloc_unchecked) == 8, "with no rows, unchecked as before")
    finally:
        rows.unlink()


def test_mw_chain_record_resolves_by_its_object():
    """GCC round 37a (the kit's round 51): Metrowerks registers a static object with a destructor by
    __register_global_object(object, destructor, &record), the record an ANONYMOUS 12-byte `@N` in the
    .bss section right before the object's (a function-local static: its guard, the record, the object
    -- IssueCommand's 0x66B8C0, 0x66B8C8, 0x66B8E0). The data map names it `@CHAIN@<fn>@<name>` or, for
    a file-scope object, `@CHAIN@<object>`, and a reference resolves by that row. std::locale::global
    (0x18CF20, byte-exact) registers its function-local global_locale with the record at 0x1D5F38, and
    __sinit_camera.cpp (0x3FFDD0) its four file-scope DiIntersects dilr0-3 with theirs, 16 bytes before
    each. Negatives: a row moved fails; with no row the record is unchecked, as before; a row naming
    another object's record fails; no registering call, no row taken; and a slipped length in the new
    name form does not parse."""
    import manglecheck as M
    img = E.image()
    rows = HERE / "build" / "tmp" / f"data_chain.{os.getpid()}.txt"
    rows.parent.mkdir(parents=True, exist_ok=True)

    def scored(unit: str, fn: str, extra: str):
        rows.write_text(E.DATA.read_text() + extra)
        st = E.SymbolTable(E.SYMBOLS, rows)
        _, obj, err = C.build_one(HERE / "src" / unit, BUILD / "chain")
        assert obj is not None, err
        return D.compare(obj, fn, st.get(fn), img, st)

    loc = "@CHAIN@global__Q23std6localeFv@global_locale"
    try:
        r = scored("main/msl/locale.cpp", "global__Q23std6localeFv", f"0x001D5F38 0xC {loc}  # test\n")
        ck(r.match and any(f.symbol == loc and f.want == 0x1D5F38 and f.status == "ok" for f in r.relocs),
           f"a function-local static's record resolves by its row: {r.verdict}")
        unchecked = len(r.reloc_unchecked)
        r = scored("main/msl/locale.cpp", "global__Q23std6localeFv", f"0x001D5F48 0xC {loc}  # test\n")
        ck(not r.match and any(f.symbol == loc and f.status == "MISMATCH" for f in r.relocs),
           "the row moved fails")
        r = scored("main/msl/locale.cpp", "global__Q23std6localeFv", "")
        ck(r.match and len(r.reloc_unchecked) == unchecked + 1
           and any(f.symbol.startswith("@") and f.symbol.endswith("(local)") and f.want == 0x1D5F38
                   for f in r.reloc_unchecked), "with no row, unchecked as before")
        cam = "".join(f"0x{a:08X} 0xC @CHAIN@dilr{i}  # test\n"
                      for i, a in enumerate((0x416040, 0x4160A0, 0x416100, 0x416160)))
        r = scored("Apps/FTS/camera.cpp", "__sinit_camera.cpp", cam)
        ck(r.match and sorted(f.want for f in r.relocs if f.symbol.startswith("@CHAIN@dilr") and f.status == "ok")
           == [0x416040, 0x4160A0, 0x416100, 0x416160], f"four file-scope records by their objects' rows: {r.verdict}")
        swapped = cam.replace("@CHAIN@dilr0", "@CHAIN@dilrX").replace("@CHAIN@dilr1", "@CHAIN@dilr0") \
                     .replace("@CHAIN@dilrX", "@CHAIN@dilr1")
        r = scored("Apps/FTS/camera.cpp", "__sinit_camera.cpp", swapped)
        ck(not r.match and sum(f.status == "MISMATCH" for f in r.relocs) == 2, "two objects' rows swapped fail")
        rows.write_text(E.DATA.read_text() + f"0x001D5F38 0xC {loc}  # test\n")
        st = E.SymbolTable(E.SYMBOLS, rows)
        elf = E.Elf.read(BUILD / "chain" / "main" / "msl" / "locale.o")
        rec = next(s for s in elf.symbols if s.size == 12 and D._ANON_OBJECT.match(s.name)
                   and elf.symbols[0] is not s and elf.sections[s.shndx].type == 8
                   and any(x.shndx == s.shndx + 1 and x.name.startswith("global_locale$") for x in elf.symbols))
        fsym = next(f for f in elf.functions() if f.name == "global__Q23std6localeFv")
        ck(D.chain_record_row(elf, rec, fsym, st, 0x18CF20, True) == loc
           and D.chain_record_row(elf, rec, fsym, st, 0x18CF20, False) is None,
           "a record is taken only from a function that registers it")
    finally:
        rows.unlink(missing_ok=True)
    ck(M.parses("@CHAIN@IssueCommand__9CSealUnitFP10CZSealBody10FT_COMMANDPvb@proto") and M.parses("@CHAIN@dilr0x__4CCam")
       and not M.parses("@CHAIN@IssueCommand__8CSealUnitFP10CZSealBody10FT_COMMANDPvb@proto"),
       "manglecheck reads both forms and a slipped length")


def test_mw_local_function_resolves_by_its_code():
    """A Metrowerks static initialiser registers each static array's destructor, a local function
    `__arraydtor$N` -- N a per-unit counter that shifts with any header edit, so no map row can carry
    it. hud_newhudmap.cpp's __sinit_hud_newhudmap.cpp (0x3FDE80) takes four such addresses; compared
    as a local object (raw bytes, relocation fields included) each was a CONTENT-MISMATCH and the
    code-exact initialiser WRONG REFERENCE (the HUD lane's round 30). Each resolves by its code now:
    the candidate's destructor against the target's function at the referenced address, masked, its
    own relocations resolved. The target's destructor with another count (an unrelocated word) must
    fail, so must its array's %lo pointing 16 bytes on (masked alike, so only the destructor's own
    relocation check can see it) -- and with the resolution switched off the four fail again."""
    img, st = E.image(), E.symbols()
    unit = HERE / "src" / "Apps" / "FTS" / "hud_newhudmap.cpp"
    obj = BUILD / "hud_newhudmap.o"
    D.compile_obj(unit, obj, includes=[HERE / "include", HERE / "src"])
    sym = st.get("__sinit_hud_newhudmap.cpp")
    dtors = lambda r: [f for f in r.relocs if f.symbol.startswith("__arraydtor$")]
    r = D.compare(obj, sym.name, sym, img, st)
    ck(r.match and len(dtors(r)) == 4 and all(f.status == "content-ok" for f in dtors(r)),
       f"the four array destructors resolve by their code: {r.verdict}")
    ck(sorted(f.want for f in dtors(r)) == [0x20CD30, 0x20CE10, 0x20CE30, 0x20CE50], "at their own rows")
    for addr, word, what in ((0x20CE68, 0x24070008, "another count"), (0x20CE58, 0x24849F30, "the array 16 bytes on")):
        bad = _image_with_word(img, addr, word)
        r = D.compare(obj, sym.name, sym, bad, st)
        ck(not r.match and any(f.want == 0x20CE50 and f.status == "CONTENT-MISMATCH" for f in r.relocs),
           f"a destructor with {what} fails: {r.verdict}")
    depth, D.LOCAL_CODE_DEPTH = D.LOCAL_CODE_DEPTH, 0
    try:
        r = D.compare(obj, sym.name, sym, img, st)
    finally:
        D.LOCAL_CODE_DEPTH = depth
    ck(not r.match and sum(f.status == "CONTENT-MISMATCH" for f in r.relocs) == 4, "switched off, the four fail")


LOCAL_CALL_FIXTURE = """/* sc: compiler=ee-gcc2.9-991111-01-dtls13010; flags=-O2 -G0 */
typedef unsigned char u_char;
typedef struct {
	u_char stat, second, minute, hour, pad, day, month, year;
} sceCdCLOCK;

static u_char ig_tobcd(u_char x)
{
	return x + (x / 10) * 6;
}

void converttobcd(sceCdCLOCK *t)
{
	t->year = ig_tobcd(t->year);
	t->month = ig_tobcd(t->month);
	t->day = ig_tobcd(t->day);
	t->hour = ig_tobcd(t->hour);
	t->minute = ig_tobcd(t->minute);
	t->second = ig_tobcd(t->second);
}
"""


def test_local_call_is_checked_by_its_code():
    """A call to a TU-local function no map row names is checked by the callee's CODE (GCC round 42a,
    the integrator's request: the zdb world's round 31 saw CShadowCaster "match" with a jal to the wrong
    static, node_world.cpp's `static inline` NodeBBox -- local_code_ok said the code differed, by_code
    returned None and the call stayed `unchecked`, which passes). Strictly now: content-ok when the
    candidate's local function is the target's at the jal's address, CONTENT-MISMATCH when it is not --
    its bytes differ, or the map has no row of its size there. GCC: libscf's converttobcd calling its
    static tobcd under another name (no row; not at the target's distance) matches by tobcd's code, six
    calls content-ok; the same caller calling a static whose body differs (`* 7`) -- its own bytes
    unchanged -- does not, and under the old rule (SC_LOCAL_CALLS=lenient) it did. Metrowerks: ldimage.cpp
    with its static setLoadImageTagsYX renamed (setLoadImageTags' tail call by code, content-ok at 0x30B770);
    the target's jal retargeted to loadImage (a row of another size) or its callee's body changed by a
    word: no MATCH, where the old rule matched both."""
    img, st = E.image(), E.symbols()
    BUILD.mkdir(parents=True, exist_ok=True)

    def scored(src: Path, fn: str, image=None, home: Path | None = None):
        if home is None:                    # a GCC fixture: built as sccheck builds it, its directive's
            _, obj, err = C.build_one(src, BUILD / "localcall", root=src.parent)
            assert obj is not None, f"{src.name} did not build: {err[-300:]}"
        else:                               # a unit's copy: its own directory's headers on the path
            obj = BUILD / "localcall" / f"{src.stem}.o"
            D.compile_obj(src, obj, includes=[HERE / "include", HERE / "src", home])
        r = D.compare(obj, fn, st.get(fn), image or img, st)
        calls = [f for f in r.relocs if f.type == "R_MIPS_26"]
        D.STRICT_LOCAL_CODE = False
        try:
            old = D.compare(obj, fn, st.get(fn), image or img, st).match
        finally:
            D.STRICT_LOCAL_CODE = True
        return r, calls, old

    gcc = BUILD / "ig_localcall.c"
    gcc.write_text(LOCAL_CALL_FIXTURE)
    r, calls, old = scored(gcc, "converttobcd")
    ck(r.match and old and len(calls) == 6 and all(f.status == "content-ok" and f.want == st.get("tobcd").addr
                                                   for f in calls),
       f"GCC: six calls to a static the map does not name resolve by its code: {r.verdict}, {[f.status for f in calls]}")
    wrong = BUILD / "ig_localcall_wrong.c"
    wrong.write_text(LOCAL_CALL_FIXTURE.replace("(x / 10) * 6", "(x / 10) * 7"))
    r, calls, old = scored(wrong, "converttobcd")
    ck(r.masked_equal and not r.match and calls and all(f.status == "CONTENT-MISMATCH" for f in calls),
       f"GCC: calling a static whose code is not the target's callee's -- the caller's bytes the same -- fails: "
       f"{r.verdict}")
    ck(old, "and under the old rule (lenient) it matched: the integrity bug, reproduced")
    unit = HERE / "src" / "gamez" / "zmpeg" / "ldimage.cpp"
    mw = BUILD / "ig_ldimage.cpp"
    mw.write_text(unit.read_text().replace("setLoadImageTagsYX", "igLocalTagsYX"))
    fn = "setLoadImageTags__FPUiPviiUi"
    r, calls, old = scored(mw, fn, home=unit.parent)
    ck(r.match and old and len(calls) == 1 and calls[0].status == "content-ok" and calls[0].want == 0x30B770,
       f"MW: the renamed static's call resolves by its code: {r.verdict}, {[(f.status, hex(f.want)) for f in calls]}")
    row = st.get(fn)
    a, w = next((a, w) for a, w, t in E.disasm(img.read(row.addr, row.size), row.addr) if t.startswith(("jal ", "j ")))
    for image, what in ((_image_with_word(img, a, (w & ~0x03FFFFFF) | ((0x30B740 >> 2) & 0x03FFFFFF)),
                         "the jal retargeted to loadImage"),
                        (_image_with_word(img, 0x30B770, struct.unpack("<I", img.read(0x30B770, 4))[0] ^ 0x8),
                         "the callee's first word (its frame) changed")):
        r, calls, old = scored(mw, fn, image, home=unit.parent)
        ck(r.masked_equal and not r.match and calls and calls[0].status == "CONTENT-MISMATCH" and old,
           f"MW: {what} -- no MATCH (the old rule matched): {r.verdict}")


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
    # 0x319580 is CSaveLoad::LoadAssetLib, a thin wrapper of LoadAssetLib_PS2: once the callee is
    # named, the exact pass may pair it with SOCOM 1's own LoadAssetLib (its relocation names
    # agree) -- but never with the SetStance wrapper its masked words also equal
    ck("SetStance" not in exact.get(0x319580, ""), "the SetStance-shaped wrapper is not named SetStance")
    ck("DoSetStance" not in callee.get(0x3199C0, ""), "nor is its callee named DoSetStance through it")
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

# The canary's own fixture: m2c's draft of OVL1's ~vector<CZDecalDef> (0x2CDB30), byte-exact as it
# stood in src/auto/ovl1/002CDB30.c until the game states' round 28 matched the row by hand
# (fts_ovl2statics.cpp's template tail) and automatch's stale pass deleted it (7092f929's text).
DRAFT_002CDB30 = """\
/* GENERATED by tools/automatch.py: m2c's draft of __dt__Q23std52vector<10CZDecalDef,Q23std23allocator<10CZDecalDef>>Fv, byte-exact as it stands
 * (every relocation resolved). Replace it with real source; the generator then deletes it. */
/* sc: as=func_002CDB30 __dt__Q23std52vector<10CZDecalDef,Q23std23allocator<10CZDecalDef>>Fv */
typedef signed char s8; typedef unsigned char u8; typedef short s16; typedef unsigned short u16;
typedef int s32; typedef unsigned int u32; typedef long s64; typedef unsigned long u64;
typedef float f32; typedef double f64; typedef int M2C_UNK; typedef int M2C_UNK32;
#define M2C_FIELD(expr, type, offset) (*(type)((char *)(expr) + (offset)))
#define SC_MEMBER(base, type, offset) (*(type)((base) + (offset)))
#define M2C_ERROR(x) 0
#define NULL 0
M2C_UNK __dl__FPv(char *);                          /* extern */
M2C_UNK func_002CE770();                            /* extern */

char *func_002CDB30(char *arg0, s64 arg1) {
    char *temp_a0;

    if (arg0 != NULL) {
        if ((arg0 != NULL) && (arg0 != NULL)) {
            func_002CE770();
            temp_a0 = SC_MEMBER(arg0, char **, 8);
            if (temp_a0 != NULL) {
                __dl__FPv(temp_a0);
            }
        }
        if (((s64) (arg1 << 0x30) >> 0x30) > 0) {
            __dl__FPv(arg0);
        }
    }
    return arg0;
}
"""


def test_copy_supersedes_a_draft_only_where_it_matched():
    """A hand-written inline copy that matches supersedes a generated draft only at the address it
    matched (the integrator's fix, 2026-11-04): each overlay links its own template copies, so
    seal_env.cpp's ~vector<CZDecalDef> scores against OVL2's row 0x5A3C60 (`ovl-copy:`), while
    OVL1's 0x2CDB30, the named row, is its draft's. By name, the copy had dropped the draft from
    scoring (a false regression) and the summary listed it superseded. Built together, both must
    match and nothing be listed superseded; with the rows' roles swapped in the map (the name at
    0x5A3C60, so the draft's home is OVL2 and its address the copy's), the same draft is superseded
    and listed so. Self-contained since GCC round 25a: the draft is the fixture above, built with
    seal_env.cpp in a scratch copy of the hand-written tree (sccheck.main's `root`), so no unit
    src/ gains or loses can take the test's ground away."""
    import contextlib
    import io
    import re

    files = r"^(gamez/zseal/seal_env\.cpp|auto/ovl1/002CDB30\.c)$"
    name = "__dt__Q23std52vector<10CZDecalDef,Q23std23allocator<10CZDecalDef>>Fv"
    tree = HERE / "build" / "tmp" / f"supersede.{os.getpid()}"
    if tree.exists():
        shutil.rmtree(tree)
    shutil.copytree(HERE / "src", tree, ignore=shutil.ignore_patterns("auto", "*.o"))
    (tree / "auto" / "ovl1").mkdir(parents=True)
    (tree / "auto" / "ovl1" / "002CDB30.c").write_text(DRAFT_002CDB30)

    def run(st):
        orig, E.symbols = E.symbols, (lambda: st)
        out = io.StringIO()
        try:
            with contextlib.redirect_stdout(out):
                rc = C.main(["--files", files, "--build-dir", str(BUILD / "supersede"), "-j", "2", "-v"],
                            root=tree)
        finally:
            E.symbols = orig
        return rc, out.getvalue()

    try:
        _copy_supersedes(run, name)
    finally:
        shutil.rmtree(tree, ignore_errors=True)


def _copy_supersedes(run, name: str) -> None:
    import re
    rc, out = run(E.symbols())
    row = lambda unit: re.search(rf"^match\s+{re.escape(unit)}\s+{re.escape(name)}\s", out, re.M)
    ck(rc == 0 and row("002CDB30.c") and row("seal_env.cpp"), "the OVL1 draft and the OVL2 copy each match")
    ck("superseded" not in out, "and no draft is listed superseded")
    swapped = HERE / "build" / "tmp" / f"syms_swapped.{os.getpid()}.txt"
    swapped.parent.mkdir(parents=True, exist_ok=True)
    text = E.SYMBOLS.read_text()
    a = next(l for l in text.splitlines(True) if l.startswith(f"0x002CDB30 0x78 {name} "))
    b = next(l for l in text.splitlines(True) if l.startswith("0x005A3C60 0x60 func_005A3C60 "))
    swapped.write_text(text.replace(a, f"0x002CDB30 0x78 func_002CDB30  # ovl-copy: {name}\n")
                       .replace(b, f"0x005A3C60 0x60 {name}  # swapped by the canary\n"))
    try:
        rc, out = run(E.SymbolTable(swapped, E.DATA))
    finally:
        swapped.unlink()
    ck(row("seal_env.cpp") and not row("002CDB30.c"), "swapped, the copy matches at the draft's address")
    ck(re.search(rf"1 generated function\(s\) superseded .*{re.escape(name)}", out), "and the draft is superseded")


# OVL1's 0x210A60, `func_00210A60  # ovl-copy: __ml__...const_iterator CFv`: the const iterator's *, one of
# the ten `ovl-copy:` placeholder drafts the HUD's round 35 integration dropped (8d60f16a^'s text)
DRAFT_00210A60 = """\
/* GENERATED by tools/automatch.py: m2c's draft of func_00210A60, byte-exact as it stands
 * (every relocation resolved). Replace it with real source; the generator then deletes it. */
typedef signed char s8; typedef unsigned char u8; typedef short s16; typedef unsigned short u16;
typedef int s32; typedef unsigned int u32; typedef long s64; typedef unsigned long u64;
typedef float f32; typedef double f64; typedef int M2C_UNK; typedef int M2C_UNK32;
s32 func_00210A60(s32 *arg0) {
    return *arg0;
}
"""


def test_superseded_lists_drafts_by_address():
    """sccheck's "N generated function(s) superseded" line lists what automatch --emit drops, one entry
    per draft by its row's address (GCC round 28a). hud_linemap.cpp's inline copy of the const
    iterator's * scores at OVL1's copy row 0x210A60 (`ovl-copy:`), whose draft defines the row's
    placeholder, func_00210A60 -- by name the line never listed it, yet --emit drops it (the HUD's round
    35 integration dropped fourteen where the line named four). The draft is still scored (its match
    listed under -v, the scoring unchanged). With the row's mark taken out of a copy of the map, the
    copy scores at the named OVL2 row and nothing is superseded."""
    import contextlib
    import io
    import re

    files = r"^(Apps/FTS/hud_linemap\.cpp|auto/ovl1/00210A60\.c)$"
    tree = HERE / "build" / "tmp" / f"byaddr.{os.getpid()}"
    if tree.exists():
        shutil.rmtree(tree)
    shutil.copytree(HERE / "src", tree, ignore=shutil.ignore_patterns("auto", "*.o"))
    (tree / "auto" / "ovl1").mkdir(parents=True)
    (tree / "auto" / "ovl1" / "00210A60.c").write_text(DRAFT_00210A60)

    def run(st):
        orig, E.symbols = E.symbols, (lambda: st)
        out = io.StringIO()
        try:
            with contextlib.redirect_stdout(out):
                C.main(["--files", files, "--build-dir", str(BUILD / "byaddr"), "-j", "2", "-v"], root=tree)
        finally:
            E.symbols = orig
        return out.getvalue()

    unmarked = HERE / "build" / "tmp" / f"syms_byaddr.{os.getpid()}.txt"
    try:
        out = run(E.symbols())
        ck(re.search(r"^match\s+hud_linemap\.cpp\s+__ml__\S+14const_iteratorCFv\s", out, re.M),
           "the copy matches (at OVL1's 0x210A60)")
        ck(re.search(r"^match\s+00210A60\.c\s+func_00210A60\s", out, re.M), "the draft is still scored")
        ck(re.search(r"1 generated function\(s\) superseded .*func_00210A60", out),
           "and listed superseded at its address")
        text = E.SYMBOLS.read_text()
        row = next(l for l in text.splitlines(True) if l.startswith("0x00210A60 "))
        unmarked.parent.mkdir(parents=True, exist_ok=True)
        unmarked.write_text(text.replace(row, "0x00210A60 0x8 func_00210A60  # the canary's: no mark\n"))
        out = run(E.SymbolTable(unmarked, E.DATA))
        ck("superseded" not in out, "unmarked, the copy is not this row's: nothing superseded")
    finally:
        shutil.rmtree(tree, ignore_errors=True)
        unmarked.unlink(missing_ok=True)


def test_partial_check_holds_a_row_another_emitter_matches():
    """sccheck --files, GCC round 38a (the integrator's request): built alone, zsys_init.cpp's inline
    copy of ~vector<CIOPManager::CRegistrant> (6 words off) lost 0x39CD40 -- util_main.cpp's copy
    matches it, and the full check never lost it -- like the thirteen rows README "Working in
    parallel" lists, held by their drafts. A row a partial check loses only by inline copies is now
    tried against its other emitters outside --files (the row's draft, a hand-written unit naming the
    copy's own identifiers) and reported `not lost` when one matches. A fixture with a SYNTHETIC name
    on a real 8-byte row (0x39CDC0, `jr ra; lw v0,8(a0)`, in a copy of the map and of MATCHED.txt):
    one unit defines CFxGet::GetSize to load +8, another emits an inline copy loading +0. Built alone,
    the copy is WRONG and the row held by the definition, rc 0; with the definition gone it is a
    REGRESSION, rc 1 (the negative); a strong definition that misses is never held; and the row's
    draft holds it as the definition did."""
    import contextlib
    import io
    import re
    va, name = 0x39CDC0, "GetSize__6CFxGetCFv"
    tree = HERE / "build" / "tmp" / f"held.{os.getpid()}"
    sym = HERE / "build" / "tmp" / f"held_symbols.{os.getpid()}.txt"
    matched = HERE / "build" / "tmp" / f"held_matched.{os.getpid()}.txt"
    if tree.exists():
        shutil.rmtree(tree)
    (tree / "fx").mkdir(parents=True)
    text = E.SYMBOLS.read_text()
    row = next(l for l in text.splitlines(True) if l.startswith(f"0x{va:08X} 0x8 "))
    sym.write_text(text.replace(row, f"0x{va:08X} 0x8 {name}  # the canary's synthetic name\n"))
    matched.write_text(f"# the canary's baseline\n0x{va:08X} {name}\n")
    cls = "struct CFxGet {{ int a, b, c; int GetSize() const{}; }};\n"
    definition = cls.format("") + "int CFxGet::GetSize() const { return c; }\n"
    copy = cls.format(" { return a; }") + "int (CFxGet::*fx_keep)() const = &CFxGet::GetSize;\n"
    strong = cls.format("") + "int CFxGet::GetSize() const { return a; }\n"
    draft = ("/* sc: as=func_0039CDC0 GetSize__6CFxGetCFv */\n"
             "int func_0039CDC0(int *self) { return self[2]; }\n")

    def run(files: str):
        st = E.SymbolTable(sym, E.DATA)
        orig, orig_m, E.symbols = E.symbols, C.MATCHED, (lambda: st)
        C.MATCHED = matched
        out = io.StringIO()
        try:
            with contextlib.redirect_stdout(out):
                rc = C.main(["--files", files, "--build-dir", str(BUILD / "held"), "-j", "2"], root=tree)
        finally:
            E.symbols, C.MATCHED = orig, orig_m
        return rc, out.getvalue()

    try:
        (tree / "fx" / "held_def.cpp").write_text(definition)
        (tree / "fx" / "held_copy.cpp").write_text(copy)
        rc, out = run(r"^fx/held_def\.cpp$")
        ck(rc == 0 and re.search(rf"^match\s+held_def\.cpp\s+{name}\s", out, re.M), f"the definition matches: {out}")
        rc, out = run(r"^fx/held_copy\.cpp$")
        ck(re.search(rf"^WRONG\s+held_copy\.cpp\s+{name}\s", out, re.M), f"built alone, the copy is WRONG: {out}")
        ck(rc == 0 and "REGRESSIONS" not in out and re.search(
            rf"^not lost -- .*{va:#x} {name} by fx/held_def\.cpp", out, re.M), f"and the row is held: {out}")
        (tree / "fx" / "held_def.cpp").unlink()
        rc, out = run(r"^fx/held_copy\.cpp$")
        ck(rc == 1 and re.search(rf"^REGRESSIONS against \S+: {va:#x} {name}", out, re.M)
           and "not lost" not in out, f"negative: with no other emitter it is a regression: {out}")
        (tree / "fx" / "held_def.cpp").write_text(definition)
        (tree / "fx" / "held_copy.cpp").write_text(strong)
        rc, out = run(r"^fx/held_copy\.cpp$")
        ck(rc == 1 and "REGRESSIONS" in out and "not lost" not in out,
           f"negative: a strong definition that misses is never held: {out}")
        (tree / "fx" / "held_copy.cpp").write_text(copy)
        (tree / "fx" / "held_def.cpp").unlink()
        (tree / "auto" / "ovl1").mkdir(parents=True)
        (tree / "auto" / "ovl1" / f"{va:08X}.c").write_text(draft)
        rc, out = run(r"^fx/held_copy\.cpp$")
        ck(rc == 0 and re.search(rf"^not lost -- .*{va:#x} {name} by auto/ovl1/{va:08X}\.c", out, re.M),
           f"and the row's draft holds it: {out}")
    finally:
        shutil.rmtree(tree, ignore_errors=True)
        sym.unlink(missing_ok=True)
        matched.unlink(missing_ok=True)


def test_stale_follows_a_callee_rename():
    """automatch --stale redoes a draft that calls a name the map renamed, not only a placeholder
    (GCC round 28a): the UI's round 20 integration -- 0x2808D0's draft went on calling SkbOpen__Fv
    after the row took its signature, a call no row resolves, which only the full check caught. The
    map's old names are git's history of symbols.txt and data.txt; a fixture draft spelling the old
    name is stale, spelling the map's name is not, and with no history only the placeholder rule is
    left (the negative: the old rule's blind spot). The fixture's own row is SYNTHETIC (GCC round
    30a): a placeholder added to a copy of the map at an address no row can hold (past OVL2), so the
    test does not depend on a real row staying a placeholder -- it had used 0x2808D0's own, which the
    engine core then matched from source as UIGetTextInput (a draft whose row is renamed is stale by
    the own-row rule, so every `== set()` failed)."""
    import automatch as A
    real = E.symbols()
    hist = A.map_history_names()
    ck("SkbOpen__Fv" in hist and "SkbOpen__Fv" not in real._by_name, "the old name is in the map's history only")
    ck("SkbOpen__FP13C2DAnimCmdHdrPf" in real._by_name, "and the name it took is the map's")
    va = 0xFFFFF0
    ck(real.containing(va) is None and real.at_addr(va) is None, "the fixture's address is no real row's")
    auto = HERE / "build" / "tmp" / f"stale.{os.getpid()}"
    (auto / "fixture").mkdir(parents=True, exist_ok=True)
    sym, dat = auto.parent / f"stale_symbols.{os.getpid()}.txt", auto.parent / f"stale_data.{os.getpid()}.txt"
    sym.write_text(E.SYMBOLS.read_text() + f"0x{va:08X} 0x8 func_{va:08X}  # synthetic: the stale test's fixture row\n")
    shutil.copy(E.DATA, dat)
    draft = (f"/* GENERATED by tools/automatch.py: m2c's draft of func_{va:08X} */\n"
             "M2C_UNK {callee}();                              /* extern */\n\n"
             f"void func_{va:08X}(void) {{{{\n    {{callee}}();\n}}}}\n")
    path = auto / "fixture" / f"{va:08X}.c"
    orig = A.AUTO
    A.AUTO = auto
    try:
        st = E.SymbolTable(sym, dat)
        ck(st.at_addr(va) is not None and st.at_addr(va).name == f"func_{va:08X}", "the copy holds the synthetic row")
        path.write_text(draft.format(callee="SkbOpen__Fv"))
        ck(A.stale(st, hist) == {va}, "a draft calling the renamed name is stale")
        ck(A.stale(st, set()) == set(), "which the placeholder rule alone misses")
        path.write_text(draft.format(callee="SkbOpen__FP13C2DAnimCmdHdrPf"))
        ck(A.stale(st, hist) == set(), "calling the map's name, it is not")
        path.write_text(
            draft.format(callee="SkbOpen__FP13C2DAnimCmdHdrPf").replace("*/\n", "*/ /* was SkbOpen__Fv */\n", 1))
        ck(A.stale(st, hist) == set(), "and an old name in a comment is no call")
        ck(A.stale(real, hist) == {va}, "against the real map (no such row) the same draft is stale: its row is the copy's")
    finally:
        A.AUTO = orig
        shutil.rmtree(auto, ignore_errors=True)
        sym.unlink(missing_ok=True)
        dat.unlink(missing_ok=True)


def test_copy_index_is_never_read_half_built():
    """GCC round 37a (the AI core's round 32: `tools/objcmp.py -j 2` printed `DIFF ai/ai_astar.cpp`
    where -j 1 was clean): SymbolTable._copy_index published an EMPTY index and then filled it, so a
    thread asking the map while another built it read the index half-built -- OVL2's copy of the
    pointer lists' deleter constructor (0x50FFA0) not yet indexed, the call resolved to the named
    row in OVL1 (0x1EABB0), and objcmp's shared resolver kept one answer for a unit's one side and
    the other for its other. The index is built in a local and published once. A reader that
    interleaves with the build is made deterministic here: the marks' regex, at its first search,
    asks the table for that copy, as a second thread scheduled there would -- it must read
    0x50FFA0. The negative control is the old shape (published, then filled), which must read
    0x1EABB0; and every copy of the finished index is the one the marks give."""
    import types
    name = "__ct__Q23std42__list_deleter<PCv,Q23std14allocator<PCv>>Fv"
    ovl2 = 0x4C5380

    def published_first(self):                   # the method before round 37a, in its own shape
        if self._copies is None:
            self._copies = {}
            for s in self.symbols + self.data:
                m = self._OVL_COPY.search(s.note or "")
                if m:
                    self._copies[(self.overlay_of(s.addr), m.group(1))] = s.addr
        return self._copies

    def interleaved(method=None):
        """(what a reader mid-build got, the finished index) for a fresh table"""
        st = E.SymbolTable()
        if method is not None:
            st._copy_index = types.MethodType(method, st)
        real, seen = st._OVL_COPY, []

        class Reader:                             # the marks' regex, with a second reader inside it
            def search(self, note):
                if not seen:
                    seen.append(None)
                    seen[0] = st.address_in(name, ovl2)
                return real.search(note)
        st._OVL_COPY = Reader()
        return seen, st._copy_index()

    seen, index = interleaved()
    ck(seen == [0x50FFA0], f"a reader during the build reads OVL2's copy: {seen}")
    ck(index.get(("OVL2", name)) == 0x50FFA0 and index == E.SymbolTable()._copy_index(),
       "and the index it publishes is whole")
    seen, _ = interleaved(published_first)
    ck(seen == [0x1EABB0], f"the old shape (published, then filled) hands it the half-built index: {seen}")
    st = E.symbols()
    row = lambda a: st.at_addr(a) or st.data_by_addr.get(a)
    ck(all(row(a) is not None and f"ovl-copy: {n}" in row(a).note and st.overlay_of(a) == o
           for (o, n), a in st._copy_index().items()),
       "every indexed copy is a row carrying its mark, in its overlay")


def test_objcmp_by_address_equates_a_respelling():
    """tools/objcmp.py --by-address (GCC round 29a; the game states' round-34 objcmp_addr.py): every
    lane respells placeholders by their rows' real names, and plain objcmp reports each renamed call
    as a difference. By address, a unit whose defined function, call and global are respelled --
    func_001A5C88 as sceSifGetDataTable, func_001A6508 as sceSifExitRpc, data_001D0BB8 as
    __infinity -- is the same, the three respellings listed. Negatives: the plain comparison
    differs; a call to another row differs by address too; two names no map resolves are compared
    by name and differ; and an offset into a placeholder that reaches the global's address
    (`data_001D0BB0[1]`) differs -- MW's REL relocation keeps the 8 in the instruction's field: a
    symbol is equated, never a sum."""
    import objcmp as O
    src = BUILD / "objcmp_fixture"
    src.mkdir(parents=True, exist_ok=True)

    def obj(stem: str, fn: str, callee: str, data: str, ref: str | None = None) -> Path:
        """`double fn(void) { callee(); return ref; }`, ref the global `data` (or an element of it)"""
        p = src / f"{stem}.c"
        p.write_text(f"void {callee}(void);\nextern double {data};\n"
                     f"double {fn}(void) {{ {callee}(); return {ref or data}; }}\n")
        o = BUILD / f"objcmp_{stem}.o"
        D.compile_obj(p, o, D.COMPILER, None, [HERE / "include", src])
        return o

    holder = obj("placeholders", "func_001A5C88", "func_001A6508", "data_001D0BB8")
    named = obj("named", "sceSifGetDataTable", "sceSifExitRpc", "__infinity")
    offset = obj("offset", "sceSifGetDataTable", "sceSifExitRpc", "data_001D0BB0[2]", "data_001D0BB0[1]")
    other = obj("other", "sceSifGetDataTable", "sceSifInitRpc", "__infinity")
    nowhere_a = obj("nowhere_a", "sceSifGetDataTable", "not_a_row_a", "__infinity")
    nowhere_b = obj("nowhere_b", "sceSifGetDataTable", "not_a_row_b", "__infinity")
    res = O.resolver(E.symbols())
    ck(O.summary(holder) != O.summary(named), "plain, the respelled unit differs")
    na: dict = {}
    nb: dict = {}
    sa, sb = O.summary(holder, res, na), O.summary(named, res, nb)
    ck(sa == sb, "by address, the respelled unit is the same")
    ck(set(O.respellings(sa, na, nb)) == {("func_001A5C88", "sceSifGetDataTable"),
                                          ("func_001A6508", "sceSifExitRpc"), ("data_001D0BB8", "__infinity")},
       f"and its three respellings are listed: {O.respellings(sa, na, nb)}")
    ck(O.summary(other, res) != sb, "a call to another row differs by address too")
    ck(O.summary(offset, res) != sb, "an offset into a placeholder is no symbol: its field differs")
    ck(O.summary(nowhere_a, res) != O.summary(nowhere_b, res), "names no map resolves are compared by name")


def test_objcmp_by_address_resolves_a_sum():
    """tools/objcmp.py --by-address, GCC round 35a (the HUD's round 54: hud_missioncams.cpp's
    data_0043668C respelled theMission+0x1AC printed DIFF though the scorer matched it): a data
    relocation is the address it forms, symbol plus the addend REL keeps in its field, the field
    masked. Each kind, both ways -- a respelling that keeps the address is the same, one that moves it
    differs: Metrowerks' %hi/%lo pair (`data_001D0BB0[1]` as __infinity, the row at 0x1D0BB8, against
    `data_001D0BB0[2]`), a .data pointer word (R_MIPS_32: `&data_001D52F8[1]` as `&data_001D52FC`),
    GCC's %hi/%lo pair at -G0 (paired by the relocation table) and its $gp-relative load at -G8
    (R_MIPS_GPREL16). The respelling is listed with its addend; round 29a's rule (symbols only, the
    field compared) still tells each pair apart; and plain objcmp, without a map, still differs."""
    import objcmp as O
    src = BUILD / "objcmp_sum_fixture"
    src.mkdir(parents=True, exist_ok=True)

    def obj(stem: str, text: str, compiler: str = D.COMPILER, flags=None) -> Path:
        p = src / f"{stem}.c"
        p.write_text(text)
        o = BUILD / f"objcmp_sum_{stem}.o"
        D.compile_obj(p, o, compiler, flags, [HERE / "include", src])
        return o

    res = O.resolver(E.symbols())

    def same(a: Path, b: Path, sums: bool = True) -> bool:
        return O.summary(a, res, sums=sums) == O.summary(b, res, sums=sums)
    mw = "void sceSifExitRpc(void);\nextern double {d};\ndouble sceSifGetDataTable(void) {{ sceSifExitRpc(); return {r}; }}\n"
    named = obj("mw_named", mw.format(d="__infinity", r="__infinity"))
    summed = obj("mw_summed", mw.format(d="data_001D0BB0[3]", r="data_001D0BB0[1]"))
    moved = obj("mw_moved", mw.format(d="data_001D0BB0[3]", r="data_001D0BB0[2]"))
    ck(same(summed, named), "MW %hi/%lo: data_001D0BB0+8 is __infinity's address, the same")
    ck(not same(moved, named), "MW %hi/%lo: data_001D0BB0+16 is another address, it differs")
    ck(not same(summed, named, sums=False), "round 29a's rule (symbols only) still tells them apart")
    na: dict = {}
    nb: dict = {}
    sa, sb = O.summary(summed, res, na, sums=True), O.summary(named, res, nb, sums=True)
    ck(sa == sb and ("data_001D0BB0+0x8", "__infinity") in O.respellings(sa, na, nb),
       f"the respelling is listed with its addend ({O.respellings(sa, na, nb)})")
    word = "extern int {d};\nint *ig_word_p = &{r};\n"
    w_named = obj("mw_word_named", word.format(d="data_001D52FC", r="data_001D52FC"))
    w_summed = obj("mw_word_summed", word.format(d="data_001D52F8[2]", r="data_001D52F8[1]"))
    w_moved = obj("mw_word_moved", word.format(d="data_001D52F8[2]", r="data_001D52F8[0]"))
    ck(same(w_summed, w_named) and not same(w_moved, w_named) and not same(w_summed, w_named, sums=False),
       "a .data word (R_MIPS_32): the same address the same, another differs, symbols only differs")
    # (`return x;` alone is no fixture at -G8: GCC leaves the return's slot empty for the offset load)
    gnu = "extern int {d};\nint ig_load(int v) {{ return v + {r}; }}\n"
    for flags, kind in ((["-O2", "-G0"], "R_MIPS_LO16"), (["-O2", "-G8"], "R_MIPS_GPREL16")):
        tag = flags[1][1:]
        g_named = obj(f"gnu_{tag}_named", gnu.format(d="data_001D52FC", r="data_001D52FC"), "ee-gcc2.96", flags)
        g_summed = obj(f"gnu_{tag}_summed", gnu.format(d="data_001D52F8[2]", r="data_001D52F8[1]"), "ee-gcc2.96", flags)
        g_moved = obj(f"gnu_{tag}_moved", gnu.format(d="data_001D52F8[2]", r="data_001D52F8[0]"), "ee-gcc2.96", flags)
        _, rels, _ = E.Elf.read(g_summed).function("ig_load")
        ck(kind in [r.type for r in rels], f"GCC {flags[1]}: the load is {kind} ({[r.type for r in rels]})")
        ck(same(g_summed, g_named) and not same(g_moved, g_named) and not same(g_summed, g_named, sums=False),
           f"GCC {flags[1]} ({kind}): the same address the same, another differs, symbols only differs")
    ck(O.summary(summed) != O.summary(named), "plain, without a map, the respelled unit differs")


OBJCMP_ANON = """extern void ig_take(const int &v);
{header}
const char *ig_name(void)
{{
    return "{name}";
}}

void ig_pass(void)
{{
    ig_take({zero});
}}

const char *ig_other(void)
{{
    return "ig_other";
}}
"""
OBJCMP_ANON_HEADER = """
inline void ig_sat(int *v, const int &lo, const int &hi)
{{
    if (*v < lo)
        *v = lo;
    if (*v > hi)
        *v = hi;
}}

struct IgMap {{
    int n;
    void Walk(int *y) {{ ig_sat(y, {lo}, n); }}
}};
"""


def test_objcmp_aligns_anonymous_objects_by_content():
    """tools/objcmp.py, GCC round 36a (the game states' round 43: ai.h's ForEachCell binds a literal 0
    to a const reference, and every unit including ai.h gets two unreferenced 4-byte zero objects, used
    or not -- every later `@N` rank shifted with the code unchanged, and a proof touching ai.h said
    DIFF). Metrowerks' anonymous objects are now named by content and an unreferenced one left out
    with its section (the linker drops it: vehicle_rdr.cpp's unreferenced zeros are not in the image).
    A header whose uninstantiated inline binds 0 to a const reference INSERTS such a zero ahead of the
    unit's strings and its referenced zero (`ig_take(0)` passes one's address): by content the unit is
    the same, by rank (`--anon-by-rank`, rounds 27a-35a's rule) it differs, the ranks of every later
    object shifted. Negatives, each by content: a referenced string changed differs, the referenced zero
    made a 1 differs; and the inserted object changed (0 -> 1, still unreferenced) is the same."""
    import objcmp as O
    src = BUILD / "objcmp_anon_fixture"
    src.mkdir(parents=True, exist_ok=True)

    def obj(stem: str, header: str = "", name: str = "ig_name", zero: str = "0") -> Path:
        p = src / f"{stem}.cpp"
        p.write_text(OBJCMP_ANON.format(header=header, name=name, zero=zero))
        o = BUILD / f"objcmp_anon_{stem}.o"
        D.compile_obj(p, o, D.COMPILER, None, [HERE / "include", src])
        return o
    base = obj("base")
    inserted = obj("inserted", OBJCMP_ANON_HEADER.format(lo="0"))
    names, dropped, dead = O.anon_by_content(E.Elf.read(inserted))
    ck(len(dead) == 1 == len(dropped) and len(names) == 3 and "@.data:00000000" in names.values(),
       f"the header's zero is the one unreferenced object; the strings and the passed zero are named ({names}, {dead})")
    ck(O.summary(base) == O.summary(inserted), "an unreferenced anonymous object inserted: the same, by content")
    ck(O.summary(base, anon_rank=True) != O.summary(inserted, anon_rank=True),
       "negative: by rank (--anon-by-rank) the same unit differs")
    res = O.resolver(E.symbols())
    ck(O.summary(base, res, sums=True) == O.summary(inserted, res, sums=True), "and the same by address")
    ck(O.summary(base) != O.summary(obj("changed", OBJCMP_ANON_HEADER.format(lo="0"), name="ig_nane")),
       "a referenced string changed: it differs")
    ck(O.summary(base) != O.summary(obj("one", OBJCMP_ANON_HEADER.format(lo="0"), zero="1")),
       "the referenced zero made a 1: it differs")
    ck(O.summary(base) == O.summary(obj("dead", OBJCMP_ANON_HEADER.format(lo="1"))),
       "the inserted object changed, still unreferenced: the same (nothing can read it)")


def test_objcmp_verdict_is_independent_of_its_work_directory():
    """tools/objcmp.py, GCC round 38a (the world's round 26: a first run in the default build/objcmp
    printed `DIFF ai/ai_mapwalk.cpp` -- MAP_DIRCOUNT::GetBestDir 0x5350C0, byte-exact -- and a fresh
    --work showed none). Nothing persisted: the two directories' paths had different LENGTHS, and
    Metrowerks' code can depend on the length of the paths it is given (GetBestDir: 6 of 122 tree
    lengths of the tree's text, 5 others of d14aeea6^1's -- the heap's layout). So objcmp builds both
    sides at a path of one fixed length wherever its work directory lies, and a unit differing there
    is built again at a second length, `PATH-DEPENDENT` when equal there. A fixture stands in for the
    compiler (whose flips no small unit reproduces: GetBestDir alone compiles one way at all 98
    lengths tried): a GCC unit whose BASE returns whether sizeof(__FILE__) is even -- its code a
    function of the path's length -- against a tree returning 0. From two work directories of
    different lengths the verdict is the same line, PATH-DEPENDENT, rc 0; unpadded (`--path-lengths
    0`, the old behaviour) two directories one character apart disagree -- the negative; one length
    alone is a DIFF; a real change (BASE returns 5) differs at both lengths and stays a DIFF. And
    each side's stems are read again: BASE's cached set never stands for the tree's."""
    import contextlib
    import io
    import objcmp as O
    root = BUILD / "workdir"
    if root.exists():
        shutil.rmtree(root)

    def fixture(name: str, body: str) -> Path:
        d = root / name
        (d / "src" / "fx").mkdir(parents=True)
        (d / "include").mkdir()
        (d / "src" / "fx" / "pathlen.c").write_text(
            f"/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */\nint fx_pathlen(void) {{ {body} }}\n")
        return d

    base = fixture("base", "return sizeof(__FILE__) % 2 == 0;")
    now = fixture("now", "return 0;")
    real = fixture("real", "return 5;")

    def run(b: Path, work: Path, *more: str):
        out = io.StringIO()
        with contextlib.redirect_stdout(out):
            rc = O.main([str(b), "^fx/", "--tree", str(now), "--work", str(work), *more])
        return rc, [l for l in out.getvalue().splitlines() if not l.startswith("against")]

    short, longer = root / "w", root / "a_work_directory_longer_by_far"
    try:
        ck(len(str(O.side_root(short, 160))) == 160 == len(str(O.side_root(longer, 160))),
           "both sides lie at a 160-character path from either work directory")
        ck(O.side_root(short, 0) == short / "tree", "and at <work>/tree unpadded (0)")
        for n in (len(str(short / "tree")) - 1, O.MAX_SIDE_LENGTH + 1):
            try:
                O.side_root(short, n)
                refused = False
            except SystemExit:
                refused = True
            ck(refused, f"a {n}-character tree path is refused (too short for the work path, or past the limit)")
        side = O.side_root(short, 160)
        (side / "src").mkdir(parents=True)
        D._SHARED_STEMS[(side / "src").resolve()] = {"stale"}
        O.build_side(side, [], root / "obj-stems")
        ck(D.shared_stems(side / "src") == set(), "each side's stems are read again (BASE's set never stands)")
        shutil.rmtree(side)
        rs, rl = run(base, short), run(base, longer)
        ck(rs == rl, f"the same verdict from two work directories: {rs} / {rl}")
        ck(rs[0] == 0 and any(l.startswith("PATH-DEPENDENT fx/pathlen.c: function fx_pathlen at a 160-character")
                               for l in rs[1]) and "1 units compared, 0 differ, 1 path-dependent" in rs[1][-1],
           f"PATH-DEPENDENT at 160, the same at 167, rc 0: {rs}")
        a, b = run(base, root / "p", "--path-lengths", "0"), run(base, root / "pq", "--path-lengths", "0")
        ck({a[0], b[0]} == {0, 1} and a[1] != b[1],
           f"negative: unpadded, work directories one character apart disagree: {a} / {b}")
        one = run(base, short, "--path-lengths", "160")
        ck(one[0] == 1 and any(l.startswith("DIFF fx/pathlen.c") for l in one[1]), f"one length alone: DIFF {one}")
        r = run(real, short)
        ck(r[0] == 1 and any(l.startswith("DIFF fx/pathlen.c") for l in r[1])
           and not any(l.startswith("PATH-DEPENDENT") for l in r[1]), f"a real change differs at both lengths: {r}")
    finally:
        shutil.rmtree(root, ignore_errors=True)


def test_unit_builds_the_same_bytes_at_any_path():
    """A unit's object may not depend on where the tree lies (GCC round 34a). sase_rfft2g.c's
    asserts always hold, so GCC drops their calls -- but not their strings, __FILE__ among them,
    which was the path the unit was built at: its .rodata differed between tools/objcmp.py's
    scratch copy and the tree (the MSL's round 28, the UI's round 26) and between worktrees, while
    the scorer, which reads only referenced data, matched both. Its `#line` names the vendor's file
    now. Every hand-written unit whose text names __FILE__, __DATE__, __TIME__ or __BASE_FILE__ is
    built from two copies of src/ and include/ at paths of different lengths and the objects are
    compared as objcmp compares them (each function's bytes and relocations, each allocated
    section, each object); sase_rfft2g.c is built twice at one path besides, and its .rodata is the
    target's (0x3EB8A0: the two twiddle tables, then the file and the expression). Negative: the
    unit with its `#line` taken out differs at the two paths, in its .rodata alone. And objcmp
    (the online lobby's round 22: the fix belongs there too) builds both of its sides at one path:
    that bare unit built as its BASE side and again in the tree's copy put in its place compares
    equal, built at another path it differs."""
    import re
    import objcmp as O
    src = HERE / "src"
    rx = re.compile(r"__(FILE|DATE|TIME|BASE_FILE)__")
    units = [p.relative_to(src) for p in C.sources(src)
             if p.relative_to(src).parts[0] != "auto" and rx.search(p.read_text(errors="replace"))]
    rfft = Path("lib/ovl1/sase_rfft2g.c")
    ck(rfft in units and len(units) >= 20, f"the units naming __FILE__ are found: {len(units)}")
    work = BUILD / "anypath"
    if work.exists():
        shutil.rmtree(work)
    roots = []
    for name in ("a", "a_second_tree_at_a_longer_path"):
        shutil.copytree(src, work / name / "src", ignore=shutil.ignore_patterns("auto", "*.o"))
        shutil.copytree(HERE / "include", work / name / "include")
        roots.append(work / name)

    def built(root: Path, rel: Path, tag: str) -> Path:
        _, o, err = C.build_one(root / "src" / rel, work / f"obj-{tag}", root=root / "src",
                                include=root / "include")
        ck(o is not None, f"{rel} builds at {root.name}: {err.strip()[-200:]}")
        return o

    differ = [str(r) for r in units
              if O.summary(built(roots[0], r, "a")) != O.summary(built(roots[1], r, "b"))]
    ck(not differ, f"every unit naming __FILE__ builds the same bytes at two paths; these differ: {differ}")
    ck(O.summary(built(roots[0], rfft, "again")) == O.summary(work / "obj-a" / rfft.with_suffix(".o")),
       "sase_rfft2g.c built twice at one path: the same bytes")
    elf = E.Elf.read(work / "obj-a" / rfft.with_suffix(".o"))
    ro = next(s for s in elf.sections if s.name == ".rodata")
    ck(ro.data == E.image().read(0x3EB8A0, ro.size) and b"../../shared/rFft2G.c\0" in ro.data,
       "and its .rodata is the target's, the vendor's file name in it")
    for root in roots:                  # the negative: __FILE__ the path the tree lies at
        f = root / "src" / rfft
        text = f.read_text()
        bare = re.sub(r"^#line .*\n", "", text, flags=re.M)
        ck(bare != text, "the unit's #line is there to take out")
        f.write_text(bare)
    a, b = O.summary(built(roots[0], rfft, "bare-a")), O.summary(built(roots[1], rfft, "bare-b"))
    ck([k for k in sorted(set(a) | set(b)) if a.get(k) != b.get(k)] == ["section .rodata#1"],
       "without its #line, the unit's .rodata differs at the two paths (and nothing else)")
    # and objcmp builds both of its sides at ONE path (the online lobby's round 22): the unit without
    # its #line, built where objcmp builds BASE's copy and again in the tree's copy put in its place,
    # compares equal; at another path (the old sides: a scratch copy against the tree) it differs
    bare = (roots[0] / "src" / rfft).read_text()

    def side_build(side: Path, tag: str) -> dict:
        f = side / "src" / rfft
        f.parent.mkdir(parents=True, exist_ok=True)
        f.write_text(bare)
        (side / "include").mkdir(exist_ok=True)
        (o, err), = O.build_side(side, [rfft], work / f"objcmp-{tag}").values()
        ck(o is not None, f"objcmp's side builds at {side.name}: {err}")
        return O.summary(o)

    side = work / "objcmp" / "tree"
    first = side_build(side, "base")
    shutil.rmtree(side)                 # tree_copy() puts the tree's copy in BASE's place
    ck(side_build(side, "now") == first, "objcmp: an unchanged unit naming __FILE__ bare is the same on both sides")
    ck(side_build(work / "objcmp" / "another_tree", "other") != first,
       "negative: the same unit built at another path differs (objcmp's old two sides)")
    shutil.rmtree(work)


EXPLICIT_MAX = ("template const unsigned char &ai::_max<unsigned char>(const unsigned char &, "
                "const unsigned char &);\n")


def test_explicit_instance_scores_in_its_units_overlay():
    """An OVL2 unit that instantiates `ai::_max<unsigned char>` explicitly: a global, the unit's own,
    whose name the map gives OVL1's 0x2AFC40 and whose OVL2 copy is 0x5DEE70 (`ovl-copy:`). By the
    unit's lowest address the instance made the unit an OVL1 one, so it was scored -- and credited --
    at OVL1's row, OVL2's never (the kit lane's round 24, GCC round 25a). The unit's overlay is now the
    one its functions' rows lie in, counting the names the map holds once: OVL2, the instance scored
    at 0x5DEE70 and the span inside OVL2. With the copy's mark gone the named row is all there is
    again; the old rule's lowest address is OVL1's. Self-contained since GCC round 42a: the kit's round
    57 dropped seal_tasks.cpp's own explicit instance (scaffolding -- kept anywhere in the unit, MW
    inlines its caller 0x5D4770's calls), so the fixture is seal_tasks.cpp with the instance appended,
    in a scratch copy of the hand-written tree (sccheck.main's `root`); its negative, the unit as it
    stands, holds no global instance and scores none at OVL1's row."""
    import contextlib
    import io
    img, st = E.image(), E.symbols()
    name = "_max<Uc>__2aiFRCUcRCUc_RCUc"
    files = r"^gamez/zseal/seal_tasks\.cpp$"
    scored = []
    orig, orig_sym = D.compare, E.symbols
    tree = HERE / "build" / "tmp" / f"explicit.{os.getpid()}"
    if tree.exists():
        shutil.rmtree(tree)
    shutil.copytree(HERE / "src", tree, ignore=shutil.ignore_patterns("auto", "*.o"))
    unit = tree / "gamez" / "zseal" / "seal_tasks.cpp"
    plain = unit.read_text()

    def run(table, text=None):
        def compare(obj, fn, sym, *a, **k):
            r = orig(obj, fn, sym, *a, **k)
            scored.append((fn, sym.addr, r.match))
            return r
        unit.write_text(plain + "\n" + EXPLICIT_MAX if text is None else text)
        D.compare, E.symbols = compare, (lambda: table)
        scored.clear()
        try:
            with contextlib.redirect_stdout(io.StringIO()):
                C.main(["--files", files, "--build-dir", str(BUILD / "ovlhome"), "-j", "2"], root=tree)
        finally:
            D.compare, E.symbols = orig, orig_sym
        return [(a, m) for fn, a, m in scored if fn == name]

    try:
        _explicit_instance(run, st, name, plain)
    finally:
        shutil.rmtree(tree, ignore_errors=True)


def _explicit_instance(run, st, name: str, plain: str) -> None:
    ck(EXPLICIT_MAX.strip() not in plain, "seal_tasks.cpp itself instantiates nothing explicitly (the fixture adds it)")
    ck(run(st) == [(0x5DEE70, True)], "the instance scores, and matches, at OVL2's copy")
    obj = BUILD / "ovlhome" / "gamez" / "zseal" / "seal_tasks.o"
    elf = E.Elf.read(obj)
    ck(C.unit_overlay(elf, st) == "OVL2", "the unit is OVL2's")
    span = C.unit_span(obj, st)
    ck(span is not None and st.overlay_of(span[0]) == st.overlay_of(span[1] - 1) == "OVL2",
       f"its span lies in OVL2: {span}")
    own = [st.get(f.name).addr for f in elf.functions() if not C.is_copy(elf, f) and f.name in st._by_name]
    ck(st.overlay_of(min(own)) == "OVL1", "while its lowest named row is OVL1's (the old rule's pick)")
    unmarked = HERE / "build" / "tmp" / f"syms_unmarked.{os.getpid()}.txt"
    unmarked.parent.mkdir(parents=True, exist_ok=True)
    text = E.SYMBOLS.read_text()
    row = next(l for l in text.splitlines(True) if l.startswith("0x005DEE70 "))
    unmarked.write_text(text.replace(row, "0x005DEE70 0x18 func_005DEE70  # the canary's: no mark\n"))
    try:
        ck(run(E.SymbolTable(unmarked, E.DATA)) == [(0x2AFC40, True)], "unmarked, the named row is scored")
    finally:
        unmarked.unlink()
    elf = E.Elf.read(BUILD / "ovlhome" / "gamez" / "zseal" / "seal_tasks.o")
    ck(any(f.name == name and not C.is_copy(elf, f) for f in elf.functions()), "the fixture's instance is global")
    got = run(st, plain)
    elf = E.Elf.read(BUILD / "ovlhome" / "gamez" / "zseal" / "seal_tasks.o")
    ck(not any(f.name == name and not C.is_copy(elf, f) for f in elf.functions())
       and all(a != 0x2AFC40 for a, _ in got),
       f"negative: the unit as it stands defines no global instance and scores none at OVL1's row ({got})")


class _PatchedImage:
    """The image with some words of one row replaced (a target that differs as a fixture says)."""
    def __init__(self, img, words: dict[int, int]):
        self.img, self.words = img, words

    def read(self, addr: int, size: int) -> bytes:
        b = bytearray(self.img.read(addr, size))
        for a, w in self.words.items():
            if addr <= a and a + 4 <= addr + size:
                struct.pack_into("<I", b, a - addr, w)
        return bytes(b)


def test_structdiff_sees_structure_not_registers():
    """tools/structdiff.py (GCC round 31a, the game states' round-35 structdiff.py and mydiff.py):
    one function against its row with the registers and the link normalised. libmc's sceMcChdir,
    byte-exact, shows no difference of either kind; then the target is changed one word at a time
    and each change is read as what it is -- a register (`li v0,-210` as `li v1,-210`): no
    structural difference, one instruction differing in its registers alone, and one difference
    with --regs; a relocated immediate (`addiu s0,s0,%lo(currentDir)` given another %lo): nothing
    at all, the link's; and, each a structural difference, a field offset (`sw s0,16(s1)` as 20), an
    opcode (that `sw` as `sh`) and a callee (`jal SignalSema` as `jal WaitSema` -- a call is named,
    never masked)."""
    import structdiff as S
    img, st = E.image(), E.symbols()
    row = st.get("sceMcChdir")
    obj = S.build(HERE / "src" / "lib" / "sdk" / "libmc.c", tag="test")
    try:
        exact = S.compare(obj, row.name, row, st, img)
        ck(S.differences(*exact)[:2] == (0, 0), "byte-exact: no structural difference, no register one")
        ck(S.differences(*exact, regs=True)[0] == 0, "byte-exact: nothing with --regs either")
        text = {t: (a, w) for a, w, t in E.disasm(img.read(row.addr, row.size), row.addr)}
        li_a, li_w = text["li v0,-210"]
        lo_a, lo_w = text["addiu s0,s0,-3072"]
        sw_a, sw_w = text["sw s0,16(s1)"]
        jal_a, jal_w = next((a, w) for t, (a, w) in text.items() if t.startswith("jal 0x1a3b40"))
        wait = st.get("WaitSema").addr
        cases = {
            "register": ({li_a: li_w & ~(31 << 16) | (3 << 16)}, 0, 1, 1),
            "relocated immediate": ({lo_a: lo_w & ~0xFFFF | ((-3072 + 64) & 0xFFFF)}, 0, 0, 0),
            "field offset": ({sw_a: sw_w & ~0xFFFF | 20}, 1, 0, 1),
            "opcode": ({sw_a: sw_w & ~(0x3F << 26) | (0x29 << 26)}, 1, 0, 1),
            "callee": ({jal_a: jal_w & ~0x03FFFFFF | (wait >> 2)}, 1, 0, 1),
        }
        for what, (words, want_struct, want_regs, want_kept) in cases.items():
            patched = _PatchedImage(img, words)
            t, c = S.compare(obj, row.name, row, st, patched)
            n, nreg, _ = S.differences(t, c)
            ck((n, nreg) == (want_struct, want_regs),
               f"a changed {what}: {want_struct} structural, {want_regs} register-only (got {n}, {nreg})")
            ck(S.differences(t, c, regs=True)[0] == want_kept, f"a changed {what}: {want_kept} with --regs")
    finally:
        obj.unlink(missing_ok=True)


def test_cdis_names_every_relocation():
    """tools/cdis.py (GCC round 31a, the game states' round-35 cdis.py): the candidate's
    disassembly with each relocation named as the scorer resolves it. sceMcChdir's every relocated
    word carries its note and no other does; its calls name their callees at their rows (SignalSema
    0x1A3B40), a GNU section-relative %hi/%lo pair the object's own label (currentDir, at data.txt's
    address), and --at prints the row's addresses."""
    import cdis
    import structdiff as S
    st = E.symbols()
    row = st.get("sceMcChdir")
    obj = S.build(HERE / "src" / "lib" / "sdk" / "libmc.c", tag="test")
    try:
        lines = cdis.listing(obj, row.name, st, row.addr)
        _, relocs, _ = E.Elf.read(obj).function(row.name)
    finally:
        obj.unlink(missing_ok=True)
    noted = [l for l in lines if "  <R_MIPS_" in l]
    ck(len(noted) == len({r.offset for r in relocs}) > 10, f"every relocated word noted, once ({len(noted)})")
    ck(all(int(l.split(":")[0], 16) - row.addr in {r.offset for r in relocs} for l in noted),
       "and only the relocated words")
    ck(lines[0].strip().startswith(f"{row.addr:08x}:"), "--at: the row's addresses")
    ck(any("<R_MIPS_26 SignalSema = 0x1a3b40>" in l for l in lines), "a call names its callee at its row")
    cur = st.data_by_name["currentDir"].addr
    ck(any(f"<R_MIPS_HI16 currentDir = 0x{cur:x}>" in l for l in lines)
       and any(f"<R_MIPS_LO16 currentDir = 0x{cur:x}>" in l for l in lines),
       "a section-relative %hi/%lo pair by the object's label, at the data map's address")


# --- the register-numbering tools (GCC round 32a: the lanes' ig/ copies promoted) -----------

IG_FIXTURE = """extern int ig_g(int);

int ig_pair(int a, int b)
{
    int first;
    int second;

    first = ig_g(a);
    second = ig_g(b);
    ig_g(0);
    return first + second;
}
"""
IG_DECLS = ("int first;\n    int second;", "int second;\n    int first;")


def _ig_fixture(name: str = "ig_fixture.c", text: str = IG_FIXTURE) -> Path:
    BUILD.mkdir(parents=True, exist_ok=True)
    p = BUILD / name
    p.write_text(text)
    return p


def _ig_regs(rec: dict) -> dict[str, str]:
    import igdump
    return {igdump.var_name(rec, n["i"]): igdump.reg_name(rec["cls"], n["color"])
            for n in rec["nodes"][rec["nreal"]:] if igdump.var_name(rec, n["i"])}


def test_igdump_records_mws_colouring():
    """tools/igdump.py (GCC round 32a, the igdump.py every lane copied): a fixture compiled by
    mwccps2 3.0b52 under gdb, as sccheck builds it, records one GPR colouring of ig_pair whose
    variables are named and whose registers are the object's own -- first and second, both live
    across calls, in s1 and s0 (the first-declared local the higher saved register), the object
    moving each call's v0 into exactly those and returning their sum from them. It refuses where a
    record would be wrong or empty: a unit under 3.0b50 (the breakpoints are 3.0b52's addresses), a
    GCC unit (cc1's allocation is tools/ccdump.py's), a unit that does not build."""
    import igdump
    src = _ig_fixture()
    recs, out = igdump.dump(src, BUILD / "ig_fixture.jsonl")
    sel = igdump.select(recs, "^ig_pair$", 0)
    ck(len(sel) == 1, f"one GPR colouring of ig_pair ({len(sel)})")
    regs = _ig_regs(sel[0][1])
    ck({"a", "b", "first", "second"} <= set(regs), f"its parameters and locals named ({sorted(regs)})")
    ck((regs["first"], regs["second"]) == ("s1", "s0"), f"first s1, second s0 ({regs['first']}, {regs['second']})")
    obj = build(src, "ig_fixture")
    code, _, _ = E.Elf.read(obj).function("ig_pair")
    text = [t for _, _, t in E.disasm(code, 0)]
    ck(f"move {regs['first']},v0" in text and f"move {regs['second']},v0" in text
       and f"addu v0,{regs['first']},{regs['second']}" in text,
       "the recorded registers are the code's: each call's result moved into them, the sum read from them")
    b50 = _ig_fixture("ig_b50.c", "/* sc: compiler=mwcps2-3.0b50-030527 */\n" + IG_FIXTURE)
    gcc = HERE / "src" / "lib" / "sdk" / "libmc.c"
    broken = _ig_fixture("ig_broken.c", IG_FIXTURE.replace("return first + second;", "return first +;"))
    for unit, why in ((b50, "breakpoints are"), (gcc, "ccdump"), (broken, "did not build")):
        try:
            igdump.dump(unit, BUILD / "ig_refused.jsonl")
            ck(False, f"{unit.name}: dumped")
        except RuntimeError as e:
            ck(why in str(e), f"{unit.name} refused: {why} ({str(e)[:80]})")


IG_LONG_FIXTURE = """extern "C" int ig_g(int);

struct CIgProbe {
    int buildOrderedCellAtomList(int a);
};

int CIgProbe::buildOrderedCellAtomList(int a)
{
    int first = ig_g(a);
    return first + ig_g(first);
}

extern "C" int ig_a_function_named_past_24_characters(int a)
{
    int first = ig_g(a);
    return first + ig_g(first);
}
"""


def test_igdump_names_every_record():
    """tools/igdump.py names every record (GCC round 39a, the engine core's round 21: igdump wrote no
    name for buildOrderedCellAtomList, so `igreplay -f` selected it only as `-f '^$' --cls 0 -r N`):
    names are read in full -- the reader took 24 bytes and refused a name that filled them -- and each
    record carries the function's mangled name too (`link`, the Object's +0x30). A C++ method named in
    exactly 24 characters and a C function in 38 are named, bare and mangled; `-f` searches the bare
    name with igdump's `@0xK` suffix stripped, `--link` the mangled one, in igdump and igreplay alike;
    a static initialiser (its name blank-padded in its node) is named. The negative: with the old
    reader's 24 bytes (IGDUMP_NAME_MAX=24) both records are unnamed again and `-f` selects none."""
    import contextlib
    import io
    import igdump
    import igreplay as R
    src = _ig_fixture("ig_long.cpp", IG_LONG_FIXTURE)
    want = {"buildOrderedCellAtomList": "buildOrderedCellAtomList__8CIgProbeFi",
            "ig_a_function_named_past_24_characters": "ig_a_function_named_past_24_characters"}
    recs, out = igdump.dump(src, BUILD / "ig_long.jsonl")
    got = {igdump.proc_name(r): r.get("link") for r in recs if r["cls"] == 0}
    ck(got == want, f"both functions named, bare and mangled ({got})")
    ck(all(r.get("proc") for r in recs), "every record named")
    ck(len(igdump.select(recs, "^buildOrderedCellAtomList$", 0)) == 1
       and len(igdump.select(recs, None, 0, link=r"^buildOrderedCellAtomList__8CIgProbeFi$")) == 1
       and not igdump.select(recs, None, 0, link=r"^buildOrderedCellAtomList$"),
       "-f searches the bare name, --link the mangled one")

    def replay(*args):
        buf = io.StringIO()
        with contextlib.redirect_stdout(buf):
            rc = R.main([str(out), "--cls", "0", *args])
        return rc, buf.getvalue()
    rc, text = replay("-f", "^buildOrderedCellAtomList$")
    ck(rc == 0 and "1 of 1 records replay exactly" in text and "] buildOrderedCellAtomList cls 0" in text,
       f"igreplay -f selects it by its bare name ({text.strip()[-80:]!r})")
    rc, text = replay("--link", "^ig_a_function_named_past_24_characters$")
    ck(rc == 0 and "1 of 1 records replay exactly" in text, f"igreplay --link selects by the mangled name")
    srecs, _ = igdump.dump(HERE / "src" / "gamez" / "zmath" / "zmath_pnt3d.cpp", BUILD / "ig_pnt3d.jsonl")
    sinit = [r for r in srecs if igdump.proc_name(r).startswith("__sinit")]
    ck(sinit and all(igdump.proc_name(r) == r.get("link") == "__sinit_zmath_pnt3d.cpp" for r in sinit)
       and all(r.get("proc") for r in srecs), f"a static initialiser named ({len(sinit)} records)")
    old = os.environ.get("IGDUMP_NAME_MAX")
    os.environ["IGDUMP_NAME_MAX"] = "24"
    try:
        orecs, _ = igdump.dump(src, BUILD / "ig_long24.jsonl")
    finally:
        if old is None:
            os.environ.pop("IGDUMP_NAME_MAX", None)
        else:
            os.environ["IGDUMP_NAME_MAX"] = old
    ck(len(orecs) == len(recs) and not any(r.get("proc") for r in orecs)
       and not igdump.select(orecs, "^buildOrderedCellAtomList$", 0),
       "negative: the old 24-byte reader leaves both unnamed and -f selects nothing")


IG_EXTERNC_FIXTURE = """extern "C" int ig_g(int);

namespace ig_ns {
int ig_scale(int a);
}
using namespace ig_ns;

extern "C" inline int ig_c_twice(int t)
{
    int u = t + t;
    return u * ig_g(u);
}

struct CIgProbe {
    int Twice(int a);
};

int CIgProbe::Twice(int a)
{
    int first = ig_g(a);
    return first + ig_g(first);
}

extern "C" int ig_c_function(int a)
{
    int first = ig_g(a);
    return first + ig_g(first);
}

extern "C" int ig_c_other(int a, int b)
{
    int first = ig_g(a);
    int second = ig_g(b);
    return first + ig_c_twice(ig_g(first)) + second;
}
"""


def test_igdump_names_extern_c_functions():
    """tools/igdump.py names an extern "C" function by its own name and link name (GCC round 42a: the
    game states' round 51 found st_exit.cpp's six named `.l@0x4` and read them by record index). The
    name is read by scanning the function Object's words for one that reads as a name, and an extern "C"
    function's +0x4 is the GLOBAL namespace, whose +0x8 -- its using-directive list -- is a heap
    pointer: past 0x6C210000 its upper half reads as two printable bytes and the NULL after it ends the
    "name" (`.l` in st_exit.cpp, `7l` here), and the link name, which must spell the name, was refused
    with it. The fixture -- a `using namespace`, behind a 1 MB comment that lifts the heap -- gives
    ig_c_function and ig_c_other their names (+0x8, the link name spelling it) and links, and the
    extern "C" inline's origins its name (no link name is made for a copy never emitted); the C++
    method is named as before; -f and --link select them in igreplay. Negatives: the scan alone
    (IGDUMP_NO_LINKNAME=1) misnames both functions and the inline, with no link, every other field the
    same; without the using directive the scan alone names them right (the cause)."""
    import contextlib
    import io
    import json
    import igdump
    import igreplay as R
    pad = "/*\n" + ("x" * 63 + "\n") * 16384 + "*/\n"
    src = _ig_fixture("ig_externc.cpp", pad + IG_EXTERNC_FIXTURE)
    recs, out = igdump.dump(src, BUILD / "ig_externc.jsonl")
    named = {igdump.proc_name(r): r.get("link") for r in recs if r["cls"] == 0}
    ck(named == {"ig_c_function": "ig_c_function", "ig_c_other": "ig_c_other", "Twice": "Twice__8CIgProbeFi"},
       f"each record named and linked ({named})")
    inl = {(o.get("inline"), o.get("role"), o.get("var")) for r in recs for o in (r.get("origins") or {}).values()
           if "inline" in o}
    ck(inl == {("ig_c_twice", "argument", "t"), ("ig_c_twice", "local", "u")}, f"the inline's origins named ({inl})")
    buf = io.StringIO()
    with contextlib.redirect_stdout(buf):
        rc1 = R.main([str(out), "--cls", "0", "-f", "^ig_c_function$"])
        rc2 = R.main([str(out), "--cls", "0", "--link", "^ig_c_other$"])
    ck(rc1 == rc2 == 0 and buf.getvalue().count("1 of 1 records replay exactly") == 2,
       f"igreplay selects them by -f and --link ({buf.getvalue()[-120:]!r})")
    old = os.environ.get("IGDUMP_NO_LINKNAME")
    os.environ["IGDUMP_NO_LINKNAME"] = "1"
    try:
        orecs, _ = igdump.dump(src, BUILD / "ig_externc_scan.jsonl")
        nrecs, _ = igdump.dump(_ig_fixture("ig_externc_nousing.cpp", pad + IG_EXTERNC_FIXTURE.replace(
            "using namespace ig_ns;\n", "")), BUILD / "ig_externc_nousing.jsonl")
    finally:
        if old is None:
            os.environ.pop("IGDUMP_NO_LINKNAME", None)
        else:
            os.environ["IGDUMP_NO_LINKNAME"] = old
    bad = [(r.get("proc"), r.get("link")) for r in orecs if not igdump.proc_name(r).startswith("Twice")]
    ck(len(bad) == 2 and all(p.endswith("@0x4") and len(p.split("@")[0]) == 2 and lk is None for p, lk in bad),
       f"negative: the scan alone misnames both extern \"C\" functions, unlinked ({bad})")
    oinl = {o.get("inline") for r in orecs for o in (r.get("origins") or {}).values() if "inline" in o}
    ck(oinl and "ig_c_twice" not in oinl, f"negative: and the inline ({oinl})")

    def rest(r):
        r = json.loads(json.dumps(r))
        for k in ("proc", "link"):
            r.pop(k, None)
        for o in (r.get("origins") or {}).values():
            o.pop("inline", None)
        return r
    ck(len(orecs) == len(recs) and all(rest(a) == rest(b) for a, b in zip(orecs, recs))
       and [(r.get("proc"), r.get("link")) for r in orecs if igdump.proc_name(r) == "Twice"]
       == [(r.get("proc"), r.get("link")) for r in recs if igdump.proc_name(r) == "Twice"],
       "every other field the same; the C++ method named as before")
    nnamed = {igdump.proc_name(r): r.get("link") for r in nrecs if r["cls"] == 0}
    ck(nnamed == named, f"without the using directive the scan alone names them ({nnamed})")


IG_INLINE_FIXTURE = """extern "C" float ig_h(float);

static inline float Blend(float a, float b, float t)
{
    float d = b - a;
    return a + d * t;
}

static inline float Twice(float x, float y)
{
    float z = Blend(x, y, 0.5f);
    return Blend(z, ig_h(z), 0.25f) + z;
}

float ig_probe(float p, float q)
{
    float r = Blend(p, q, ig_h(p));
    float s = Twice(r, q);
    return Blend(s, r, ig_h(s));
}
"""


def test_igdump_pcode_view_names_each_temporarys_inline():
    """tools/igdump.py --pcode (GCC round 40a: the AI core's round-35 pcode.py folded in) and each
    `@N`'s maker (the kit's round 55). A fixture inlining Blend three times -- twice from its own
    text (lines 17 and 19), once inside Twice's copy (line 18, the inliner's second level) -- records
    which expansion made each temporary: Blend's non-trivial argument t at lines 17 and 19 (level 0),
    its argument b inside Twice's copy at line 18 (level 1); every `@N` node of the record has a maker.
    The view prints every PCode instruction, each virtual operand with the register its node took
    (the object's own: `mov.s $f21,$f12` is p's), the callee named, the GPR record's float operands
    coloured from the FPR record. Negatives: with the inliner's breakpoints off (IGDUMP_NO_ORIGINS=1)
    no maker is recorded and the first assertion's check fails; with Blend never inlined (`#pragma
    dont_inline on`) no temporary is an inline's and the view calls Blend by name."""
    import contextlib
    import io
    import igdump
    src = _ig_fixture("ig_inline.cpp", IG_INLINE_FIXTURE)
    recs, out = igdump.dump(src, BUILD / "ig_inline.jsonl")
    fsel = igdump.select(recs, "^ig_probe$", 1)
    ck(len(fsel) == 1 and "origins" in fsel[0][1] and "syms" in fsel[0][1],
       f"one FPR record of ig_probe, with origins and syms ({len(fsel)})")
    k, rec = fsel[0]

    def made(r):
        return {(o.get("inline"), o.get("role"), o.get("var"), o.get("line"), o.get("level"),
                 tuple(o.get("within", ()))) for o in (r.get("origins") or {}).values() if "inline" in o}
    want = {("Blend", "argument", "t", 17, 0, ()), ("Blend", "argument", "t", 19, 0, ()),
            ("Blend", "argument", "b", 18, 1, ("Twice",))}
    ck(made(rec) == want, f"each temporary's expansion: function, role, variable, line, level ({made(rec)})")
    unique = [i for i in range(rec["nreal"], rec["nused"]) if igdump.var_name(rec, i)[1:].isdigit()]
    ck(unique and all(str(i) in rec["origins"] for i in unique),
       f"every @N node has a maker ({len(unique)}: {sorted(rec['origins'])})")

    def view(path, *args):
        buf = io.StringIO()
        with contextlib.redirect_stdout(buf):
            rc = igdump.main(["--read", str(path), "-f", "^ig_probe$", "--pcode", *args])
        return rc, buf.getvalue().splitlines()
    rc, lines = view(out, "--cls", "1")
    body = [l for l in lines if l.startswith("    ") and l.strip().split()[0].isdigit()]
    n_ins = sum(1 for c in rec["code"] if not isinstance(c, str))
    ck(rc == 0 and len(body) == n_ins and n_ins > 10, f"every instruction listed ({len(body)} of {n_ins})")
    colour = {n["i"]: igdump.reg_name(1, n["color"]) for n in rec["nodes"]}
    seen = [(int(v), r) for l in body for v, r in __import__("re").findall(r"\bv(\d+)(?:<[^>]*>)?=(f\d+)", l)]
    ck(seen and all(colour[v] == r for v, r in seen), f"each virtual operand its node's register ({len(seen)})")
    ck(any(" mov.s " in l and "v32<p>=f21, f12" in l for l in body)
       and any(" jal " in l and "ig_h" in l for l in body), "p's move from f12 into f21, the callee named")
    obj = build(src, "ig_inline")
    text = [t for _, _, t in E.disasm(E.Elf.read(obj).function("ig_probe__Fff")[0], 0)]
    ck("mov.s $f21,$f12" in text, "the view's register is the object's: mov.s $f21,$f12")
    legend = [l for l in lines if "inlined at line" in l]
    ck(any("Blend's argument b, inlined at line 18 (level 1, inside Twice's copy)" in l for l in legend)
       and any("Blend's argument t, inlined at line 17 (level 0)" in l for l in legend),
       f"the variables' makers in the view ({legend})")
    rc, glines = view(out, "--cls", "0")
    ck(rc == 0 and any("fv32<p>=f21" in l for l in glines), "a GPR record's float operand coloured by the FPR record")
    old = os.environ.get("IGDUMP_NO_ORIGINS")
    os.environ["IGDUMP_NO_ORIGINS"] = "1"
    try:
        nrecs, _ = igdump.dump(src, BUILD / "ig_inline_off.jsonl")
    finally:
        if old is None:
            os.environ.pop("IGDUMP_NO_ORIGINS", None)
        else:
            os.environ["IGDUMP_NO_ORIGINS"] = old
    nrec = igdump.select(nrecs, "^ig_probe$", 1)[0][1]
    ck(not nrec["origins"] and made(nrec) != want and nrec["code"] == rec["code"],
       "negative: the inliner's breakpoints off, no maker recorded (the same code)")
    di = _ig_fixture("ig_inline_off.cpp", "#pragma dont_inline on\n" + IG_INLINE_FIXTURE)
    drecs, dout = igdump.dump(di, BUILD / "ig_dont_inline.jsonl")
    ck(not any(made(r) for r in drecs), "negative: Blend never inlined, no temporary is an inline's")
    rc, dlines = view(dout, "--cls", "-1")
    ck(any(" jal " in l and "Blend" in l for l in dlines), "and the view calls it by name")


IG_PASS_FIXTURE = """extern "C" int ig_g(int);
extern "C" int ig_k(int, int);

extern "C" int ig_cse(int x, int y)
{
    int a = ig_g(x * y + 7);
    int b = ig_g(x * y + 7);
    return a + b;
}

extern "C" int ig_split(int x, int y)
{
    int t = ig_g(x);
    ig_k(t, 1);
    t = ig_g(y);
    int u = ig_g(t + 1);
    ig_k(t, u);
    return u;
}
"""


def test_igdump_names_the_pass_of_each_temporary():
    """The temporaries no inline made (GCC round 41a; the kit's 0x60AA20, whose s0/s1 holders round
    40a traced to create_temp_object called from 0x5327F8): that call is IRO_CommonSubs' (0x532920,
    dumped as "IRO_CommonSubs"), one temporary per class of a common subexpression -- `x * y + 7`
    written twice is computed once into it and reused; the call from 0x533D7E is IRO_SplitLifetimes'
    -- a variable reused (t = ig_g(y) after t's first web) gives its later web a temporary, the
    earliest definition's web keeping t. Both are numbered after every parameter and named local
    (u, declared after t's reuse, is numbered before the split web): an Object's place in the
    locals. igdump --iro writes mwccps2's own optimiser log (its IRO dumper, switched on under
    gdb): the replacement and the split logged, the temporaries named in the dumps after their
    passes, and the colourings the same with the log on. Negatives: `#pragma opt_common_subs off`
    makes no common subexpression, `#pragma opt_lifetimes off` no split; IGDUMP_NO_PASSES=1 (the
    two breakpoints off) still makes both temporaries but names neither's pass."""
    import igdump
    src = _ig_fixture("ig_passes.cpp", IG_PASS_FIXTURE)
    log = BUILD / "ig_passes.iro.log"
    recs, out = igdump.dump(src, BUILD / "ig_passes.jsonl", iro=log)

    def made(r, p):
        return [(int(i), o) for i, o in (r.get("origins") or {}).items() if o.get("pass") == p]

    def node(r, name):
        return next((i for i in range(r["nreal"], r["nused"]) if igdump.var_name(r, i) == name), None)
    cse = igdump.select(recs, "^ig_cse$", 0)[0][1]
    cs = made(cse, "IRO_CommonSubs")
    ck(len(cs) == 1 and cs[0][1].get("expr") == "(x * y) + 7" and cs[0][1].get("computed")
       and cs[0][1].get("reused") and not set(cs[0][1]["computed"]) & set(cs[0][1]["reused"]),
       f"ig_cse: one IRO_CommonSubs temporary for (x * y) + 7, computed once and reused ({cs})")
    named = [node(cse, n) for n in ("x", "y", "a", "b")]
    ck(None not in named and cs and cs[0][0] > max(named),
       f"its node numbered after every parameter and named local ({cs[0][0] if cs else None} > {named})")
    spl = igdump.select(recs, "^ig_split$", 0)[0][1]
    sp = made(spl, "IRO_SplitLifetimes")
    ck(len(sp) == 1 and sp[0][1].get("var") == "t" and node(spl, "t") is not None,
       f"ig_split: t's later web an IRO_SplitLifetimes temporary, its first web still t ({sp})")
    ck(sp and node(spl, "u") is not None and sp[0][0] > node(spl, "u") > node(spl, "t"),
       f"the split web numbered after u, which is declared after it ({sp[0][0] if sp else None}, u "
       f"{node(spl, 'u')}, t {node(spl, 't')})")
    ck(all("pass" in o for r in recs for o in (r.get("origins") or {}).values() if o.get("creator") == "0x477270"
           and o.get("caller") in ("0x5327f8", "0x533d7e")), "every temporary of the two passes names its pass")
    funcs = {f["name"]: f for f in igdump.iro_read(log)}
    ev = {n: [e for e, _ in f["events"]] for n, f in funcs.items()}
    tname = lambda r, i: igdump.var_name(r, i)
    after = lambda f, ph: "\n".join(next((l for p, l in funcs[f]["phases"] if p == ph), []))
    ck(set(funcs) == {"ig_cse", "ig_split"} and any(e.startswith("Replacing common sub") for e in ev["ig_cse"])
       and any(e.startswith("Splitting range for variable") for e in ev["ig_split"]),
       f"--iro: the optimiser's log, the replacement and the split logged ({ {n: e[:3] for n, e in ev.items()} })")
    ck(cs and sp and f"Operand {tname(cse, cs[0][0])} " in after("ig_cse", "after IRO_CommonSubs")
       and f"Operand {tname(spl, sp[0][0])} " in after("ig_split", "after IRO_SplitLifetimes")
       and f"Operand {tname(spl, sp[0][0])} " not in after("ig_split", "before IRO pass loop"),
       "each temporary named in the dump after its pass, the split one absent before it")
    plain, _ = igdump.dump(src, BUILD / "ig_passes_plain.jsonl")

    def strip(rs):          # the log's FILE moves the heap: an operand's pointer by what it names
        import re as _re

        def op(a, syms):
            m = _re.fullmatch(r"k(\d+):([0-9a-f]{16})", a)
            if not m or m.group(1) not in ("4", "6"):
                return a
            w0, w1 = struct.unpack("<II", bytes.fromhex(m.group(2)))
            return (f"k4:{w0:#x}:{syms.get(f'{w1:#x}', '?') if w1 else 0}" if m.group(1) == "4"
                    else f"k6:{syms.get(f'{w0:#x}', '?')}")
        return [{**{k: v for k, v in r.items() if k not in ("origins", "syms", "code")},
                 "code": [c if isinstance(c, str) else [c[0], [op(a, r.get("syms") or {}) for a in c[1]]]
                          for c in r["code"]]} for r in rs]
    ck(strip(plain) == strip(recs), "the colourings the same with the log on (each operand's pointer by its name)")
    off = _ig_fixture("ig_passes_off.cpp", "#pragma opt_common_subs off\n#pragma opt_lifetimes off\n"
                      + IG_PASS_FIXTURE)
    orecs, _ = igdump.dump(off, BUILD / "ig_passes_off.jsonl")
    ck(not any(made(r, "IRO_CommonSubs") or made(r, "IRO_SplitLifetimes") for r in orecs),
       "negative: opt_common_subs and opt_lifetimes off, neither pass makes a temporary")
    old = os.environ.get("IGDUMP_NO_PASSES")
    os.environ["IGDUMP_NO_PASSES"] = "1"
    try:
        nrecs, _ = igdump.dump(src, BUILD / "ig_passes_nopass.jsonl")
    finally:
        if old is None:
            os.environ.pop("IGDUMP_NO_PASSES", None)
        else:
            os.environ["IGDUMP_NO_PASSES"] = old
    ncse = igdump.select(nrecs, "^ig_cse$", 0)[0][1]
    ck(not made(ncse, "IRO_CommonSubs") and strip([ncse])[0]["code"] == strip([cse])[0]["code"]
       and any(o.get("caller") == "0x5327f8" for o in ncse["origins"].values()),
       "negative: the two breakpoints off, the temporary still made (the same code) but no pass named")


def test_igreplay_replays_and_predicts():
    """tools/igreplay.py (GCC round 32a: the MSL lane's round-11 igreplay.py, the game states' round-37
    relabel.py and brute.py, the AI core's round-28 nbdeg.py): MW's simplify/colour replayed offline
    gives every recorded colouring of zmath_pnt3d.cpp exactly -- its 12 GPR and 12 FPR records, order
    and colours -- and the fixture's; with one interference removed (first-second) the replay differs
    from the record, so the check can fail. And it PREDICTS: renumbering first after second (`--move
    first=35.5`) gives first s0 and second s1 -- what the swapped declarations compile to, measured by
    igdump on the swapped source -- and --want/--movers finds that placement among the others."""
    import igdump
    import igreplay as R
    recs, _ = igdump.dump(HERE / "src" / "gamez" / "zmath" / "zmath_pnt3d.cpp", BUILD / "ig_pnt3d.jsonl")
    by_cls = {0: 0, 1: 0}
    exact = 0
    for r in recs:
        by_cls[r["cls"]] += 1
        bad, same = R.Graph(r).check()
        exact += not bad and same
    ck(by_cls[0] >= 10 and by_cls[1] >= 10 and exact == len(recs),
       f"every record of zmath_pnt3d.cpp replays ({exact} of {len(recs)}: {by_cls})")
    frecs, _ = igdump.dump(_ig_fixture(), BUILD / "ig_fixture.jsonl")
    rec = igdump.select(frecs, "^ig_pair$", 0)[0][1]
    bad, same = R.Graph(rec).check()
    ck(not bad and same, "the fixture's record replays")
    first, second = R.node_index(rec, "first"), R.node_index(rec, "second")
    colour, newidx, _ = R.replay_moved(rec, {}, drop=[(first, second)])
    ck((R.rname(colour[newidx[first]]), R.rname(colour[newidx[second]])) != ("s1", "s0"),
       "an interference removed: the replay no longer gives the record's registers")
    colour, newidx, _ = R.replay_moved(rec, {first: second + 0.5})
    predicted = (R.rname(colour[newidx[first]]), R.rname(colour[newidx[second]]))
    srecs, _ = igdump.dump(_ig_fixture("ig_swapped.c", IG_FIXTURE.replace(*IG_DECLS)), BUILD / "ig_swapped.jsonl")
    sregs = _ig_regs(igdump.select(srecs, "^ig_pair$", 0)[0][1])
    ck(predicted == ("s0", "s1") == (sregs["first"], sregs["second"]),
       f"--move first after second predicts the swapped source's registers ({predicted}, {sregs})")
    hits = R.brute(rec, {first: "s0", second: "s1"}, [first])
    ck(second + 0.5 in [h[0] for h in hits] and first + 0.5 not in [h[0] for h in hits],
       f"--want finds the placement after second, not first's own ({hits})")
    # --target (the MSL lane's igmap.py/igtarget.py, the kit's round 42): each virtual register's
    # register in the TARGET, by aligning the PCode with the final code. zlib's inflate_blocks_new is
    # byte-exact: every register it maps is the record's; with one word of the target changed (the
    # block pointer `s` moved into s4 instead of s0) that use is read as s4.
    unit = HERE / "src" / "zlib" / "infblock.c"
    zrecs, _ = igdump.dump(unit, BUILD / "ig_infblock.jsonl")
    zrec = igdump.select(zrecs, "^inflate_blocks_new$", 0)[0][1]
    tcol, conflicts, _ = R.target_colours(zrec, unit, "inflate_blocks_new")
    ck(len(tcol) >= 10 and all(tcol[v] == zrec["nodes"][v]["color"] for v in tcol) and not conflicts,
       f"a byte-exact function maps to its own colouring ({len(tcol)} mapped)")
    img, st = E.image(), E.symbols()
    row = st.get("inflate_blocks_new")
    a, w = next((a, w) for a, w, t in E.disasm(img.read(row.addr, row.size), row.addr) if t == "move s0,v0")
    s = R.node_index(zrec, "s")
    tcol2, conflicts2, _ = R.target_colours(zrec, unit, "inflate_blocks_new",
                                            img=_PatchedImage(img, {a: w & ~(31 << 11) | (20 << 11)}))
    ck(s in conflicts2 and conflicts2[s].get("s4") == 1, f"the target's s4 use of `s` is read ({conflicts2.get(s)})")


def _swap_gprs(w: int, a: int, b: int) -> int:
    """An integer instruction word with GPRs a and b exchanged in its register fields (SPECIAL and
    MMI: rs, rt, rd; REGIMM: rs; j/jal: none; the rest, I-type: rs, rt)."""
    op = w >> 26
    fields = {0: (21, 16, 11), 0x1C: (21, 16, 11), 1: (21,), 2: (), 3: ()}.get(op, (21, 16))
    for sh in fields:
        r = (w >> sh) & 31
        if r in (a, b):
            w = w & ~(31 << sh) | ((b if r == a else a) << sh)
    return w


def test_igreplay_target_scores_the_move_and_budgets_a_search():
    """tools/igreplay.py, GCC round 33a (the lanes' three requests). (a) --target scores the colouring
    --move gives: zlib's byte-exact inflate_blocks_new with `c` renumbered first gives z s2 and c s3
    (recorded s3, s2); against a target whose s2 and s3 are exchanged in every word that move differs
    nowhere and the recorded colouring in both nodes -- and against the real target the CLI says 2
    differ (round 32a's --target printed the recorded colouring's 0 whatever the move). (b) a node whose
    target uses disagree is listed even where its most frequent register is ours (one use of `s` read as
    s4: round 32a listed only a differing majority). (c) a search has a budget -- --max-iters and
    --timeout stop it and say so (rc 2), progress is reported -- and its faster replay (the graph
    renumbered in place) gives round 32a's deep-copied replay's colours placement for placement."""
    import contextlib
    import io
    import igdump
    import igreplay as R
    unit = HERE / "src" / "zlib" / "infblock.c"
    dump = BUILD / "ig_infblock.jsonl"
    zrecs, _ = igdump.dump(unit, dump)
    zrec = igdump.select(zrecs, "^inflate_blocks_new$", 0)[0][1]
    z, c, s = (R.node_index(zrec, n) for n in ("z", "c", "s"))
    rec_regs = tuple(R.rname(zrec["nodes"][v]["color"]) for v in (z, c))
    colour, newidx, _ = R.replay_moved(zrec, {c: zrec["nreal"] - 0.5})
    moved = tuple(R.rname(colour[newidx[v]]) for v in (z, c))
    ck(rec_regs == ("s3", "s2") and moved == ("s2", "s3"), f"the fixture's move swaps z and c ({rec_regs} -> {moved})")
    img, st = E.image(), E.symbols()
    row = st.get("inflate_blocks_new")
    words = {a: _swap_gprs(w, 18, 19) for a, w, _ in E.disasm(img.read(row.addr, row.size), row.addr)}
    tcol, conflicts, unmatched = R.target_colours(zrec, unit, "inflate_blocks_new", img=_PatchedImage(img, words))
    _, _, differ_moved = R.target_report(zrec, tcol, conflicts, unmatched, {c: zrec["nreal"] - 0.5})
    _, _, differ_rec = R.target_report(zrec, tcol, conflicts, unmatched)
    ck(differ_moved == 0 and differ_rec >= 2, f"(a) the move is scored against the target: moved {differ_moved} "
       f"differ, as recorded {differ_rec}")
    out = io.StringIO()
    with contextlib.redirect_stdout(out):
        R.main([str(dump), "-f", "^inflate_blocks_new$", "--cls", "0", "--target", str(unit),
                "inflate_blocks_new", "--move", f"c={zrec['nreal'] - 0.5}"])
    ck(", 2 differ," in out.getvalue() and "(as recorded: 0 differ)" in out.getvalue(),
       f"(a) the CLI scores the moved colouring ({out.getvalue().splitlines()[-1][:150]})")
    # (b): one use of `s` (in s0 throughout) read as s4 in the target
    a, w = next((a, w) for a, w, t in E.disasm(img.read(row.addr, row.size), row.addr) if t == "move s0,v0")
    tcol2, conflicts2, unm2 = R.target_colours(zrec, unit, "inflate_blocks_new",
                                               img=_PatchedImage(img, {a: w & ~(31 << 11) | (20 << 11)}))
    lines, _, differ2 = R.target_report(zrec, tcol2, conflicts2, unm2)
    ck(tcol2[s] == zrec["nodes"][s]["color"] and differ2 == 0
       and any(l.lstrip().startswith(f"v{s} ") and "the most frequent is ours" in l for l in lines),
       f"(b) a node whose uses disagree is listed though its majority is ours ({[l for l in lines if 'disagree' in l]})")
    # (c): budgets, progress, and the faster replay equal to round 32a's
    frecs, _ = igdump.dump(_ig_fixture(), BUILD / "ig_fixture.jsonl")
    rec = igdump.select(frecs, "^ig_pair$", 0)[0][1]
    first, second = R.node_index(rec, "first"), R.node_index(rec, "second")
    want = {first: "s0", second: "s1"}
    whole = R.search(rec, want, [first, second])
    ck(whole.stopped is None and whole.tried == whole.total == (len(rec["nodes"]) - rec["nreal"] + 1) ** 2
       and whole.hits == R.brute(rec, want, [first, second]) and whole.hits,
       f"(c) an unbudgeted search replays every placement ({whole.tried} of {whole.total}, {len(whole.hits)} hits)")
    ticks = []
    cut = R.search(rec, want, [first, second], max_iters=3, progress=lambda *t: ticks.append(t), every=0.0)
    ck(cut.stopped == "max-iters" and cut.tried == 3 and len(ticks) == 3,
       f"(c) --max-iters stops it ({cut.stopped}, {cut.tried} tried, {len(ticks)} progress calls)")
    ck(R.search(rec, want, [first, second], timeout=0.0).stopped == "timeout", "(c) --timeout stops it")
    out = io.StringIO()
    with contextlib.redirect_stdout(out):
        rc = R.main([str(BUILD / "ig_fixture.jsonl"), "-f", "^ig_pair$", "--cls", "0", "--want",
                     "first=s0,second=s1", "--movers", "first,second", "--max-iters", "3", "--progress", "0"])
    ck(rc == 2 and "STOPPED by the --max-iters budget after 3 of" in out.getvalue(),
       f"(c) the CLI says the search stopped short (rc {rc})")
    lo, hi = zrec["nreal"], len(zrec["nodes"])
    g0, same, n = R.Graph(zrec), 0, 0
    for v in range(lo, hi):
        for p in [x + 0.5 for x in range(lo - 1, hi, 3)]:
            fast, fi, _ = R.replay_moved(zrec, {v: p}, graph=g0)
            ref, ri, _ = R.replay_relabelled(zrec, {v: p})
            same += [fast[fi[i]] for i in range(hi)] == [ref[ri[i]] for i in range(hi)]
            n += 1
    ck(same == n, f"(c) the in-place renumbering replays as the deep copy did ({same} of {n} placements)")


IG_FFIXTURE = """extern float ig_f(float);

float ig_fpair(float a, float b)
{
    float first;
    float second;

    first = ig_f(a);
    second = ig_f(b);
    ig_f(0.0f);
    return first + second;
}
"""
IG_FDECLS = ("float first;\n    float second;", "float second;\n    float first;")


def _swap_fprs(w: int, a: int, b: int) -> int:
    """An instruction word with FPRs a and b exchanged in its float register fields (lwc1/swc1: ft;
    mfc1/mtc1: fs; cfc1/ctc1, bc1x: none; COP1's other operations: ft, fs, fd); the rest as it is."""
    op = w >> 26
    if op in (0x31, 0x39):
        fields = (16,)
    elif op == 0x11:
        rs = (w >> 21) & 31
        fields = (11,) if rs in (0, 4) else () if rs in (2, 6, 8) else (16, 11, 6)
    else:
        fields = ()
    for sh in fields:
        r = (w >> sh) & 31
        if r in (a, b):
            w = w & ~(31 << sh) | ((b if r == a else a) << sh)
    return w


def test_igreplay_reads_float_registers_in_the_target():
    """tools/igreplay.py's FPR class (GCC round 35a: the AI core's LeadTargetG, mapped by hand, and
    the `$f20,$f0,$f20` operand misses): igdump records an FPR colouring's PCode (before, the GPR
    class's alone), --target reads each float node's register in the target and --move scores an FPR
    record. The fixture keeps first in f21 and second in f20 across the calls, as its code moves each
    call's $f0 into them; --move first after second predicts f20 and f21 -- what the swapped
    declarations compile to, by igdump and in the code. Read with --target against the swapped
    object's code, the record differs in both nodes and the move in none, and a --want search finds
    the move. CQuat::Exp(const CPnt3D *) (byte-exact) maps to its own colouring with no PCode
    unmatched; with one of the four uses of `len` (f21) read as f22 in the target, that use is listed
    though f21 is the most frequent; and a record from before this round (no FPR PCode) is refused,
    not mapped."""
    import contextlib
    import io
    import igdump
    import igreplay as R
    src = _ig_fixture("ig_ffixture.c", IG_FFIXTURE)
    recs, _ = igdump.dump(src, BUILD / "ig_ffixture.jsonl")
    sel = igdump.select(recs, "^ig_fpair$", 1)
    ck(len(sel) == 1 and any(not isinstance(i, str) for i in sel[0][1]["code"]),
       f"one FPR colouring of ig_fpair, its PCode recorded ({len(sel)})")
    rec = sel[0][1]
    regs = _ig_regs(rec)
    ck((regs.get("first"), regs.get("second")) == ("f21", "f20"), f"first f21, second f20 ({regs})")

    def text(obj: Path) -> list[str]:
        code, rel, _ = E.Elf.read(obj).function("ig_fpair")
        return [t for _, _, t in E.disasm(D.mask(code, rel), 0)]
    ours = text(build(src, "ig_ffixture"))
    ck("mov.s $f21,$f0" in ours and "mov.s $f20,$f0" in ours and "add.s $f0,$f21,$f20" in ours,
       f"the recorded registers are the code's ({ours})")
    first, second = R.node_index(rec, "first"), R.node_index(rec, "second")
    colour, newidx, _ = R.replay_moved(rec, {first: second + 0.5})
    predicted = (R.rname(colour[newidx[first]], 1), R.rname(colour[newidx[second]], 1))
    swapped = _ig_fixture("ig_fswapped.c", IG_FFIXTURE.replace(*IG_FDECLS))
    srecs, _ = igdump.dump(swapped, BUILD / "ig_fswapped.jsonl")
    sregs = _ig_regs(igdump.select(srecs, "^ig_fpair$", 1)[0][1])
    theirs = text(build(swapped, "ig_fswapped"))
    ck(predicted == ("f20", "f21") == (sregs["first"], sregs["second"]) and "add.s $f0,$f20,$f21" in theirs,
       f"--move first after second predicts the swapped source's float registers ({predicted}, {sregs})")
    tcol, conflicts, unmatched = R.tmap(rec, ours, theirs)
    _, want, differ_rec = R.target_report(rec, tcol, conflicts, unmatched)
    _, _, differ_moved = R.target_report(rec, tcol, conflicts, unmatched, {first: second + 0.5})
    ck(R.rname(tcol[first], 1) == "f20" and R.rname(tcol[second], 1) == "f21" and differ_rec == 2
       and differ_moved == 0 and not unmatched,
       f"--target reads the swapped code's float registers: as recorded {differ_rec} differ, moved {differ_moved}")
    hits = R.brute(rec, want, [first])
    ck((want.get(first), want.get(second)) == ("f20", "f21") and second + 0.5 in [h[0] for h in hits]
       and first + 0.5 not in [h[0] for h in hits],
       f"--want/--movers over the saved FPRs finds the move, not first's own place ({want}, {hits})")
    unit = HERE / "src" / "gamez" / "zmath" / "zmath_main.cpp"
    dump = BUILD / "ig_zmath_main.jsonl"
    zrecs, _ = igdump.dump(unit, dump)
    exp = [r for _, r in igdump.select(zrecs, "^Exp$", 1) if "t" not in _ig_regs(r)]
    ck(len(exp) == 1, f"one FPR colouring of CQuat::Exp(const CPnt3D *) ({len(exp)})")
    zrec = exp[0]
    func = "Exp__5CQuatFPC6CPnt3D"
    tcol, conflicts, unmatched = R.target_colours(zrec, unit, func)
    ck(len(tcol) >= 10 and all(tcol[v] == zrec["nodes"][v]["color"] for v in tcol) and not conflicts
       and not unmatched, f"a byte-exact function maps to its own float colouring ({len(tcol)} mapped, "
       f"{unmatched} unmatched)")
    img, st = E.image(), E.symbols()
    row = st.get(func)
    ln = R.node_index(zrec, "len")
    ck(R.rname(zrec["nodes"][ln]["color"], 1) == "f21", "len is in f21")
    a, w = [(a, w) for a, w, t in E.disasm(img.read(row.addr, row.size), row.addr) if t == "mov.s $f12,$f21"][-1]
    patched = _PatchedImage(img, {a: _swap_fprs(w, 21, 22)})
    tcol2, conflicts2, unm2 = R.target_colours(zrec, unit, func, img=patched)
    lines, _, differ2 = R.target_report(zrec, tcol2, conflicts2, unm2)
    ck(tcol2[ln] == 21 and differ2 == 0 and conflicts2.get(ln, {}).get("f22") == 1
       and any(l.lstrip().startswith(f"v{ln} ") and "the most frequent is ours" in l for l in lines),
       f"a disagreeing float use is listed ({[l for l in lines if 'disagree' in l]})")
    old = BUILD / "ig_fpr_before_35a.jsonl"
    old.write_text("".join(__import__("json").dumps(dict(r, code=[] if r["cls"] == 1 else r["code"])) + "\n"
                           for r in zrecs))
    try:
        with contextlib.redirect_stdout(io.StringIO()):
            R.main([str(old), "-f", "^Exp$", "--cls", "1", "-r", "1", "--target", str(unit), func])
        ck(False, "a record without FPR PCode was mapped")
    except SystemExit as e:
        ck("dump the unit again" in str(e), f"a record without FPR PCode is refused ({e})")
    out = io.StringIO()
    with contextlib.redirect_stdout(out):
        R.main([str(dump), "-f", "^Exp$", "--cls", "1", "-r", "1", "--target", str(unit), func])
    ck(", 0 differ," in out.getvalue() and "0 PCode instructions unmatched" in out.getvalue(),
       f"the CLI maps an FPR record ({out.getvalue().splitlines()[-1][:120]})")


IG_BYTES = """extern int ig_g(int);

int ig_bytes(signed char *a, unsigned char *b, short *c, unsigned short *d)
{
    int first;
    int second;
    int third;
    int fourth;

    first = a[1];
    second = b[2];
    third = c[3];
    fourth = d[4];
    ig_g(0);
    return first + second + third + fourth;
}
"""


def test_igreplay_reads_byte_and_halfword_loads():
    """tools/igreplay.py --target, GCC round 36a (round 35a's find): mwccps2 registers its byte and
    halfword loads as 1030 lbu, 1037 lb, 1038 lh and 1039 lhu, and rounds 32a-35a's table read
    1040/1041/1044/1045 (ll, lld, l.s, lwc2) as them -- so no GPR byte or halfword load was ever
    matched, and its register was never read in the target. The fixture loads one of each through a
    pointer into four values live across a call (first s3, second s2, third s1, fourth s0): aligned
    with its own code every PCode instruction matches (the old table: the four loads unmatched), and
    against a target whose four loads write s7, s6, s5, s4 -- the adds still reading s3-s0 -- each
    node's load is read there as a disagreeing use (the old table: no node disagrees). And a memory
    access's displacement, which the PCode knows for a pointer and a constant offset, picks between
    instructions of one signature: two `lh t1,N(a1)` scheduled out of the PCode's order map each node
    to its own field's register (by signature alone, to each other's: c2dfont.cpp's vector erase lost
    41 of its 68 registers so). A candidate longer than its target maps instead of raising IndexError."""
    import igdump
    import igreplay as R
    src = _ig_fixture("ig_bytes.c", IG_BYTES)
    recs, _ = igdump.dump(src, BUILD / "ig_bytes.jsonl")
    sel = igdump.select(recs, "^ig_bytes$", 0)
    ck(len(sel) == 1, f"one GPR colouring of ig_bytes ({len(sel)})")
    rec = sel[0][1]
    ops = {str(i[0]) for i in rec["code"] if not isinstance(i, str)}
    ck({"1030", "1037", "1038", "1039"} <= ops and not ops & {"1040", "1041", "1044", "1045"},
       f"the PCode's byte and halfword loads are 1030/1037/1038/1039 ({sorted(ops)})")
    regs = _ig_regs(rec)
    names = ("first", "second", "third", "fourth")
    ck(tuple(regs.get(n) for n in names) == ("s3", "s2", "s1", "s0"), f"first-fourth in s3-s0 ({regs})")
    code, rel, _ = E.Elf.read(build(src, "ig_bytes")).function("ig_bytes")
    words = [w for _, w, _ in E.disasm(D.mask(code, rel), 0)]
    ours = [t for _, _, t in E.disasm(D.mask(code, rel), 0)]
    loads = ["lb s3,1(a0)", "lbu s2,2(a1)", "lh s1,6(a2)", "lhu s0,8(a3)"]
    ck(all(t in ours for t in loads), f"the code loads each into its register ({ours})")
    nodes = [R.node_index(rec, n) for n in names]
    tcol, conflicts, unmatched = R.tmap(rec, ours, ours)
    ck(unmatched == 0 and not conflicts and all(tcol[v] == rec["nodes"][v]["color"] for v in nodes),
       f"aligned with its own code, every PCode instruction matches ({unmatched} unmatched)")
    moved = {16: 20, 17: 21, 18: 22, 19: 23}
    theirs_w = [w & ~(31 << 16) | (moved[(w >> 16) & 31] << 16) if (w >> 26) in (0x20, 0x21, 0x24, 0x25) else w
                for w in words]
    theirs = [t for _, _, t in E.disasm(b"".join(struct.pack("<I", w) for w in theirs_w), 0)]
    ck(all(t.replace(f"s{r - 16}", f"s{moved[r] - 16}", 1) in theirs for t, r in zip(loads, (19, 18, 17, 16))),
       f"the target's loads write s7-s4 ({[t for t in theirs if t.startswith('l')]})")
    tcol2, conflicts2, unm2 = R.tmap(rec, ours, theirs)
    ck(all(conflicts2.get(v) == {f"s{moved[rec['nodes'][v]['color']] - 16}": 1, R.rname(rec["nodes"][v]["color"]): 1}
           for v in nodes) and unm2 == 0,
       f"each node's load is read in the target, against its add ({ {v: conflicts2.get(v) for v in nodes} })")
    lines, _, _ = R.target_report(rec, tcol2, conflicts2, unm2)
    ck(sum(l.lstrip().startswith("v") and "uses disagree" in l for l in lines) == 4,
       f"--target's report lists the four ({lines})")
    saved = dict(R.PSEUDO)
    try:
        for k in ("1030", "1037", "1038", "1039"):
            del R.PSEUDO[k]
        R.PSEUDO.update({"1041": "lb", "1040": "lbu", "1044": "lh", "1045": "lhu"})
        _, conflicts3, unm3 = R.tmap(rec, ours, theirs)
    finally:
        R.PSEUDO.clear()
        R.PSEUDO.update(saved)
    ck(unm3 == 4 and not conflicts3,
       f"negative: rounds 32a-35a's table leaves the four loads unmatched and reads none of them ({unm3}, {conflicts3})")
    syn = {"cls": 0, "nodes": [{"i": i, "color": i} for i in range(32)] + [{"i": 32, "color": 9}, {"i": 33, "color": 9}],
           "code": [[1038, ["r32", "k4:2800000000000000", "r5"]], [1038, ["r33", "k4:2a00000000000000", "r5"]]]}
    syn_ours, syn_targ = ["lh t1,42(a1)", "lh t1,40(a1)"], ["lh t2,42(a1)", "lh t1,40(a1)"]
    ck(R.tmap(syn, syn_ours, syn_targ)[0] == {32: 9, 33: 10} and R.tmap(syn, syn_ours, syn_targ, disp=False)[0] == {32: 10, 33: 9},
       "the displacement picks each field's load (by signature alone, the two trade)")
    tcol4, _, _ = R.tmap(rec, ours, ours[:13])
    ck(all(v in tcol4 for v in nodes) and len(tcol4) < len(tcol),
       f"a candidate longer than its target maps what both hold ({len(tcol4)} of {len(tcol)} mapped)")


IG_SPILL = """extern int ig_g(int);

int ig_spill(int n)
{
    int a = ig_g(0);
    int b = ig_g(1);
    int c = ig_g(2);
    int d = ig_g(3);
    int e = ig_g(4);
    int f = ig_g(5);
    int g = ig_g(6);
    int h = ig_g(7);
    int i = ig_g(8);
    char flag = ig_g(9);
    int j = ig_g(10);

    ig_g(n);
    return a + b + c + d + e + f + g + h + i + flag + j;
}
"""
IG_SPILL_DECLS = ("    int i = ig_g(8);\n    char flag = ig_g(9);\n", "    char flag = ig_g(9);\n    int i = ig_g(8);\n")


def test_igreplay_replays_spill_slots():
    """The spill slots (GCC round 34a, the weapons' round-27 request: CZWeapon::Fire 0x3D2D70 is 31 off
    in stack offsets alone, the target packing two word spills in one quadword). igdump records the
    slot pass after a colouring that spills (each spilled node's Object: size, alignment, offset) and
    the function's frame top; igreplay replays it -- the spilled nodes in INDEX order, each bumped
    down the local area by its size and rounded to its alignment -- under a --move. A fixture with
    twelve values live across a call spills h, i and flag (a char spilled as a 16-byte quadword): in
    index order -4, -8, -32 under a 192-byte frame top, the object's spill stores at sp 188, 184 and
    160; the replay gives the record's slots. Numbered flag before i (`--move flag=40.5`) the replay
    predicts -4, -32, -36 and a frame top of 208 -- and the source with the two declarations swapped
    compiles to exactly that (its record's slots, its code's sp 204, 176, 172). Negatives: the slots
    taken in the record's own order under the move (not by index) differ from the swapped compile's;
    a recorded alignment changed fails the record's own check. (Fire's record under the weapons'
    `--move 45=40.5` gives the target's 220, 216 and 192 -- README, round 34a -- not asserted here:
    its unit is another lane's to change.)"""
    import json
    import igdump
    import igreplay as R
    recs, _ = igdump.dump(_ig_fixture("ig_spill.c", IG_SPILL), BUILD / "ig_spill.jsonl")
    (k, rec), = igdump.select(recs, "^ig_spill$", 0)[:1]
    sp = rec.get("spills")
    ck(sp is not None and [(s["name"].split("@0x")[0], s["size"], s["align"], s["off"]) for s in sp["slots"]]
       == [("h", 4, 4, -4), ("i", 4, 4, -8), ("flag", 16, 16, -32)] and rec.get("frame") == 192,
       f"the slot pass recorded in index order: {sp and sp['slots']}, frame {rec.get('frame')}")
    ck(R.check_slots(rec) is True, "the slot model gives the record's own slots")
    obj = build(BUILD / "ig_spill.c", "ig_spill")
    code, _, _ = E.Elf.read(obj).function("ig_spill")
    text = [t for _, _, t in E.disasm(code, 0)]
    ck({"sw v0,188(sp)", "sw v0,184(sp)", "sb v0,160(sp)"} <= set(text), "the object's spill stores at frame + offset")
    flag, i = R.node_index(rec, "flag"), R.node_index(rec, "i")
    moved, newidx, _ = R.replay_moved(rec, {flag: i - 0.5})
    pred = R.stack_slots(rec, moved, newidx)
    ck([(nm, off, spo) for _, nm, _, _, off, spo, _ in pred["slots"]] == [("h", -4, 204), ("flag", -32, 176), ("i", -36, 172)]
       and pred["frame"] == 208, f"--move flag before i predicts the swapped slots: {pred}")
    swapped = IG_SPILL.replace(IG_SPILL_DECLS[0], IG_SPILL_DECLS[1])
    ck(swapped != IG_SPILL, "the swapped declarations differ")
    srecs, _ = igdump.dump(_ig_fixture("ig_spill_swapped.c", swapped), BUILD / "ig_spill_swapped.jsonl")
    (_, srec), = igdump.select(srecs, "^ig_spill$", 0)[:1]
    got = [(s["name"].split("@0x")[0], s["off"]) for s in srec["spills"]["slots"]]
    ck(got == [(nm, off) for _, nm, _, _, off, _, _ in pred["slots"]] and srec["frame"] == pred["frame"],
       f"and the swapped source compiles to it: {got}, frame {srec['frame']}")
    scode, _, _ = E.Elf.read(build(BUILD / "ig_spill_swapped.c", "ig_spill_swapped")).function("ig_spill")
    ck({"sw v0,204(sp)", "sb v0,176(sp)", "sw v0,172(sp)"} <= {t for _, _, t in E.disasm(scode, 0)},
       "the swapped object's spill stores at the predicted sp offsets")
    by_old = R.stack_slots(rec, moved, list(range(len(rec["nodes"]))))     # the record's order kept
    ck(by_old is None or [(nm, off) for _, nm, _, _, off, _, _ in by_old["slots"]] != got,
       "negative: the record's own order under the move is not the swapped compile's")
    bad = json.loads(json.dumps(rec))
    bad["spills"]["slots"][1]["align"] = 16
    ck(R.check_slots(bad) is False, "negative: a recorded alignment changed fails the record's check")


def test_igreplay_permutes_the_movers_among_their_slots():
    """tools/igreplay.py --permute (GCC round 42a, the engine core's round 24: SetCDIBBox 0x2DC7A0's
    eight common-subexpression temporaries are ~5e18 placements for --want/--movers, every half step
    for each): a search over only the movers' ORDERS AMONG THEIR OWN SLOTS -- k! placements, 40,320 for
    eight -- under the same budget and output. It finds the three known numberings no slower: round
    32a's GPR fixture (first after second: s0/s1), round 35a's FPR fixture (the swapped object's f20/f21,
    read by --target) and round 33a's zlib move (c renumbered first: z s2, c s3) are each the one hit of
    its 2 placements, the identity tried first and no hit, where the full search replays 10, 11 and 23
    placements before its first hit (81, 100, 2,401 in all) -- and each hit's colouring is the known
    move's, node for node. The CLI says what it searches and prints each hit as a --move per mover;
    --max-iters stops it (rc 2, STOPPED); a mover named twice and --permute without --movers are
    refused."""
    import contextlib
    import io
    import igdump
    import igreplay as R

    def colours(rec, moves):
        colour, newidx, _ = R.replay_moved(rec, moves)
        return [colour[newidx[i]] for i in range(len(rec["nodes"]))]

    def known(tag, rec, want, movers, move):
        a, b = movers
        full1 = R.search(rec, want, movers, limit=1)
        perm = R.search(rec, want, movers, permute=True)
        ck(perm.stopped is None and perm.tried == perm.total == 2 and perm.hits == [(float(b), float(a))],
           f"{tag}: the swap is the one hit of 2 placements, the identity none ({perm.hits}, {perm.tried})")
        perm1 = R.search(rec, want, movers, limit=1, permute=True)
        ck(full1.hits and perm1.hits and perm1.tried <= full1.tried and perm.total <= full1.total,
           f"{tag}: no slower -- {perm1.tried} placements to the first hit, the full search {full1.tried} "
           f"(of {full1.total})")
        ck(colours(rec, dict(zip(movers, perm.hits[0]))) == colours(rec, move),
           f"{tag}: the hit's colouring is the known move's, node for node")

    # round 32a: the GPR fixture
    frecs, _ = igdump.dump(_ig_fixture(), BUILD / "ig_fixture.jsonl")
    rec = igdump.select(frecs, "^ig_pair$", 0)[0][1]
    first, second = R.node_index(rec, "first"), R.node_index(rec, "second")
    known("32a GPR", rec, {first: "s0", second: "s1"}, [first, second], {first: second + 0.5})
    # round 35a: the FPR fixture against its swapped declarations' object, the want read by --target
    src = _ig_fixture("ig_ffixture.c", IG_FFIXTURE)
    frec = igdump.select(igdump.dump(src, BUILD / "ig_ffixture.jsonl")[0], "^ig_fpair$", 1)[0][1]
    swapped = _ig_fixture("ig_fswapped.c", IG_FFIXTURE.replace(*IG_FDECLS))

    def text(obj: Path) -> list[str]:
        code, rel, _ = E.Elf.read(obj).function("ig_fpair")
        return [t for _, _, t in E.disasm(D.mask(code, rel), 0)]
    tcol, conflicts, unmatched = R.tmap(frec, text(build(src, "ig_ffixture")), text(build(swapped, "ig_fswapped")))
    _, want, _ = R.target_report(frec, tcol, conflicts, unmatched)
    ffirst, fsecond = R.node_index(frec, "first"), R.node_index(frec, "second")
    ck((want.get(ffirst), want.get(fsecond)) == ("f20", "f21"), f"35a FPR: --target wants f20/f21 ({want})")
    known("35a FPR", frec, want, [ffirst, fsecond], {ffirst: fsecond + 0.5})
    # round 33a: zlib's inflate_blocks_new, c renumbered first
    zrecs, _ = igdump.dump(HERE / "src" / "zlib" / "infblock.c", BUILD / "ig_infblock.jsonl")
    zrec = igdump.select(zrecs, "^inflate_blocks_new$", 0)[0][1]
    z, c = R.node_index(zrec, "z"), R.node_index(zrec, "c")
    known("33a zlib", zrec, {z: "s2", c: "s3"}, [z, c], {c: zrec["nreal"] - 0.5})
    # the CLI: what it searches, each hit a --move per mover, the budget, the refusals
    out = io.StringIO()
    args = [str(BUILD / "ig_fixture.jsonl"), "-f", "^ig_pair$", "--cls", "0", "--want", "first=s0,second=s1",
            "--movers", "first,second", "--progress", "0"]
    with contextlib.redirect_stdout(out):
        rc = R.main(args + ["--permute"])
    ck(rc == 0 and f"searching 2 placements of v{first}, v{second} -- their orders among their own slots, "
       f"{first} {second}" in out.getvalue() and f"v{first}->{second}  v{second}->{first}" in out.getvalue()
       and "1 placements of" in out.getvalue(), f"the CLI's permutation search ({out.getvalue()!r})")
    out = io.StringIO()
    with contextlib.redirect_stdout(out):
        rc = R.main(args + ["--permute", "--max-iters", "1"])
    ck(rc == 2 and "STOPPED by the --max-iters budget after 1 of 2 placements" in out.getvalue(),
       f"--max-iters stops it, rc 2 ({rc})")
    out = io.StringIO()
    with contextlib.redirect_stdout(out):
        R.main(args)
    ck(f"searching {(len(rec['nodes']) - rec['nreal'] + 1) ** 2:,} placements" in out.getvalue(),
       "without --permute the full search, as before")
    try:
        R.search(rec, {first: "s0"}, [first, first], permute=True)
        ck(False, "a mover named twice was permuted")
    except ValueError:
        ck(True, "a mover named twice is refused")
    try:
        with contextlib.redirect_stdout(io.StringIO()):
            R.main([str(BUILD / "ig_fixture.jsonl"), "-f", "^ig_pair$", "--cls", "0", "--want", "first=s0",
                    "--permute"])
        ck(False, "--permute without --movers ran")
    except SystemExit as e:
        ck("--permute needs --movers" in str(e), f"--permute without --movers is refused ({e})")


def test_igvar_compares_variants():
    """tools/igvar.py (GCC round 32a: the AI core's round-28 igvar.py and the game states' round-37
    one): each variant of a unit built under igdump in a scratch tree, the saved registers' holders
    printed per variant. The fixture as it is holds second (with b) in s0 and first in s1; its
    declarations swapped, first and second trade -- a no-op variant reads `(as it is)`, a
    substitution not in the file is said and not built as the unchanged source, a variant that does
    not build is a BUILD-FAIL -- and the file itself is never touched."""
    import igvar
    src = _ig_fixture()
    variants = [("swapped", [IG_DECLS]), ("noop", [("ig_g(0);", "ig_g(0);")]),
                ("missing", [("int third;", "int fourth;")]), ("broken", [("first + second;", "first +;")])]
    work = BUILD / "igvar"
    res = dict(igvar.run(src, src, "^ig_pair$", variants, work=work))
    ck(res["(as it is)"] == ["[0] ig_pair: spilled -; s0=b/second s1=first"], f"as it is: {res['(as it is)']}")
    ck(res["swapped"] == ["[0] ig_pair: spilled -; s0=b/first s1=second"], f"swapped: {res['swapped']}")
    ck(res["noop"] == ["(as it is)"], f"a no-op variant: {res['noop']}")
    ck(res["missing"][0].startswith("(not in ig_fixture.c"), f"a missing substitution: {res['missing']}")
    ck(res["broken"][0].startswith("BUILD-FAIL"), f"a variant that does not build: {res['broken']}")
    ck(src.read_text() == IG_FIXTURE, "the unit itself untouched")


def test_try_prints_each_variants_distance():
    """tools/try.py prints each variant's difflib distance beside its verdict (GCC round 39a, the AI
    core's round-34 vd.py): the words of every unequal stretch of the candidate's and the row's bodies,
    each masked by the candidate's relocations, the longer side counted. tty.c's byte-exact sceTtyRead
    is MATCH with no distance; its newline test made a tab's is one word away (d=1, the sizes equal);
    returning `i + 2` for `i + 1` costs the loop's shared increment, a body two words shorter and 25
    away (vd.py's figure for it) -- each verdict, distance and both sizes on the variant's line;
    --no-dist prints the verdict as before, and the exit code and score_obj's verdict are unchanged."""
    import contextlib
    import importlib
    import io
    T = importlib.import_module("try")
    unit = HERE / "src" / "lib" / "sdk" / "tty.c"
    old = os.environ.get("SC_TRY_WORK")
    T.WORK = BUILD / "trywork"

    def run(*args):
        buf = io.StringIO()
        with contextlib.redirect_stdout(buf):
            rc = T.main([str(unit), "sceTtyRead", *args])
        return rc, [l for l in buf.getvalue().splitlines() if l.strip()]
    try:
        rc, lines = run("--sub", "buf[i] == '\\n'", "buf[i] == '\\t'", "--sub", "return i + 1;", "return i + 2;")
        ck(rc == 0 and len(lines) == 2 and all(l.startswith("FAIL") and " d=" in l for l in lines)
           and lines[1].rstrip().endswith("| return i + 2;"), f"two misses: verdict, distance, label ({lines})")
        size = E.symbols().get("sceTtyRead").size
        ck(f"d=1 ({size:#x} vs {size:#x})" in lines[0], f"one immediate changed is one word away ({lines[0]})")
        ck(f"d=25 ({size - 8:#x} vs {size:#x})" in lines[1] and "size" in lines[1],
           f"two words short, 25 away ({lines[1]})")
        rc, lines = run("--sub", "return i + 1;", "return i + 2;", "--no-dist")
        ck(rc == 0 and lines[0].startswith("FAIL") and " d=" not in lines[0], f"--no-dist: the verdict alone ({lines})")
        rc, lines = run()
        ck(rc == 0 and lines == ["MATCH"], f"as it is: MATCH, no distance ({lines})")
        T.prepare()
        obj, _ = T.build(unit, unit, None)
        ck(T.distance(obj, unit, "sceTtyRead") == (0, *(2 * [E.symbols().get("sceTtyRead").size // 4])),
           "a byte-exact body is distance 0, its sizes the row's")
        ck(T.score_obj(obj, unit, "sceTtyRead") == "MATCH" and T.distance(obj, unit, "no_such_function") is None,
           "score_obj unchanged; a missing symbol has no distance")
    finally:
        T.WORK = Path(old).resolve() if old else HERE / "build" / "try"


def test_try_sub_all_replaces_every_occurrence():
    """tools/try.py's --sub replaces OLD's FIRST occurrence -- its meaning since round 1, kept -- and
    --sub-all EVERY one (GCC round 42a: the kit's round 57 read a three-site change as one). tty.c's
    sceTtyRead tests `buf[i] ==` at two sites (the newline, the return): --sub makes only the first
    `!=`, --sub-all both -- two verdicts, two distances (d=8, the body 0xC4; d=46, 0x58), the second
    labelled `(all 2)` -- and the --sub's second site is said on stderr; --and-all joins a variant as
    --sub-all starts one; a --variants triple (old, new, 'all') is --sub-all's text and (old, new, 1)
    a pair's; a missing OLD is still `(not in ...)`; tools/igvar.py's parser takes the -all forms."""
    import argparse
    import contextlib
    import importlib
    import io
    T = importlib.import_module("try")
    unit = HERE / "src" / "lib" / "sdk" / "tty.c"
    base = unit.read_text()
    old = os.environ.get("SC_TRY_WORK")
    T.WORK = BUILD / "trywork"
    eq, ne = "buf[i] ==", "buf[i] !="

    def run(*args):
        buf, err = io.StringIO(), io.StringIO()
        with contextlib.redirect_stdout(buf), contextlib.redirect_stderr(err):
            rc = T.main([str(unit), "sceTtyRead", *args])
        return rc, [l for l in buf.getvalue().splitlines() if l.strip()], err.getvalue()
    try:
        one, miss, sites = T.apply_subs(base, [(eq, ne)])
        every, _, sites_all = T.apply_subs(base, [(eq, ne, "all")])
        ck(miss is None and sites == [(eq, 2, 1)] and sites_all == [(eq, 2, 2)] and one.count(ne) == 1
           and every.count(ne) == 2 and one.index(ne) < one.index(eq),
           f"apply_subs: a pair the first of 2 sites, 'all' both ({sites}, {sites_all})")
        ck(T.apply_subs(base, [(eq, ne, 1)])[0] == one and T.apply_subs(base, [(eq, ne, 2)])[0] == every
           and T.apply_subs(base, [("no such text", "x", "all")])[1] == "no such text",
           "a count of 1 is a pair, 2 here every one; a missing OLD is reported")
        rc, lines, err = run("--sub", eq, ne, "--sub-all", eq, ne)
        ck(rc == 0 and len(lines) == 2 and lines[0].rstrip().endswith(f"| {ne}")
           and lines[1].rstrip().endswith(f"| {ne} (all 2)") and "d=8 (0xc4 vs 0xd0)" in lines[0]
           and "d=46 (0x58 vs 0xd0)" in lines[1], f"--sub the first site, --sub-all both ({lines})")
        ck("occurs 2 times in tty.c" in err and "FIRST" in err, f"the --sub's second site said on stderr ({err!r})")
        rc, lines2, err2 = run("--sub", "return i + 1;", "return i + 1;", "--and-all", eq, ne,
                               "--sub-all", "no such text", "x")
        ck(lines2[0].split("|")[0] == lines[1].split("|")[0] and lines2[0].rstrip().endswith("(+1 more) (all 2)")
           and lines2[1].startswith("(not in tty.c: 'no such text')") and "occurs" not in err2,
           f"--and-all joins a variant; a missing OLD is said ({lines2}, {err2!r})")
        vf = BUILD / "v_suball.py"
        vf.write_text(f"SUBS = {{'all': [({eq!r}, {ne!r}, 'all')], 'one': [({eq!r}, {ne!r}, 1)],"
                      f" 'pair': [({eq!r}, {ne!r})]}}\n")
        rc, lines3, _ = run("--variants", str(vf))
        verdicts = {l.split("|")[1].strip(): l.split("|")[0] for l in lines3}
        ck(verdicts.get("all (all 2)") == lines[1].split("|")[0] and verdicts.get("one") == verdicts.get("pair")
           == lines[0].split("|")[0], f"--variants: 'all' is --sub-all's, 1 and a pair --sub's ({lines3})")
        ap = argparse.ArgumentParser()
        ap.add_argument("-n", "--newlines", action="store_true")
        ap.add_argument("--variants")
        igvar = importlib.import_module("igvar")
        ck(igvar.trymod is T, "igvar parses with try.py's helpers")
        T.add_sub_options(ap)
        vs = T.variants_from(ap.parse_args(["--sub-all", eq, ne, "--and", "a", "b"]), base)
        ck(vs == [(f"{ne} (+1 more)", [(eq, ne, "all"), ("a", "b")])], f"the parser's variants: {vs}")
    finally:
        T.WORK = Path(old).resolve() if old else HERE / "build" / "try"


def test_ccdump_takes_a_dump_flag_as_its_own_argument():
    """tools/ccdump.py --dump -dL (GCC round 43a, the GCC c lane's round 38c): argparse reads a lone
    value that starts with `-` as an option of its own, so `--dump -dL` was refused and only
    `--dump=-dL` worked. Now the argument after --dump (or --flags, or an abbreviation naming one of
    them alone) is its value: the separate form, the `=` form, a quoted list and the default each
    parse as before or as the `=` form does, `-fsched-verbose=4` is the dump's value and not `-f`'s,
    and `-f` after it is still a function; argparse alone refuses `--dump -dL` (the negative: the join
    is what takes it); end to end, tty.c's loop dump from both forms is the same file set, byte for byte."""
    import contextlib
    import importlib
    import io
    C = importlib.import_module("ccdump")
    p = C.parse_args
    ck(p(["u.c", "--dump", "-dL"]).dump == "-dL" == p(["u.c", "--dump=-dL"]).dump,
       "--dump -dL is --dump=-dL")
    ck(p(["u.c", "--dump", "-dc -dl"]).dump == "-dc -dl" and p(["u.c"]).dump == C.DEFAULT_DUMPS,
       "a quoted list and the default keep their meaning")
    a = p(["u.c", "--du", "-fsched-verbose=4", "-f", "foo", "--flags", "-O1", "--", "-fno-gcse"])
    ck(a.dump == "-fsched-verbose=4" and a.function == ["foo"] and a.flags == "-O1" and a.extra == ["-fno-gcse"],
       f"an abbreviation, --flags, -f and the extra flags after `--` ({a})")
    with contextlib.redirect_stderr(io.StringIO()):
        try:
            C.parser().parse_args(["u.c", "--dump", "-dL"])
            refused = False
        except SystemExit:
            refused = True
    ck(refused, "argparse alone refuses --dump -dL")
    unit = HERE / "src" / "lib" / "sdk" / "tty.c"
    got = []
    for i, args in enumerate((["--dump", "-dL"], ["--dump=-dL"])):
        out = BUILD / f"ccdump_sep{i}"
        with contextlib.redirect_stdout(io.StringIO()):
            rc = C.main([str(unit), *args, "--out", str(out)])
        loop = out / "tty.c.loop"
        got.append((rc, sorted(x.name for x in out.iterdir()), loop.read_bytes() if loop.exists() else b""))
    ck(got[0][0] == 0 and got[0] == got[1] and "tty.c.loop" in got[0][1] and b";; Function" in got[0][2],
       f"both forms write the same loop dump ({got[0][:2]}, {got[1][:2]})")


def test_pragmascan_finds_the_kits_cse_off():
    """tools/pragmascan.py (GCC round 43a): FaceXZ 0x60CAD0 as the kit's round 58 wrote it, its two
    pragma lines taken out (master's ai_group.cpp, in a scratch copy -- a3785ee6's body, the round's
    base, is 0x104 against 0xF4 under CSE off: the kit respelled it as well), is open; the scan finds
    its body by name and confirms it by its probe, wraps it into master's text byte for byte, and
    reports a HIT under opt_common_subs with nothing else in the unit lost -- and none under
    opt_propagation, the negative; a re-run builds nothing (resumed from the results file); a pragma
    Metrowerks does not know is refused (`illegal #pragma` under -w all), the seven it scans are not;
    the mangled names' scopes and the text's bodies read as the matcher needs them."""
    import importlib
    P = importlib.import_module("pragmascan")
    rel = "ai/ai_group.cpp"
    name = "FaceXZ__FPC6CPnt3DPC6CPnt3DP7CMatrix"
    real = (HERE / "src" / rel).read_text()
    sig = "static void FaceXZ(const CPnt3D *from, const CPnt3D *to, CMatrix *m)\n{\n"
    on, reset = "#pragma opt_common_subs off\n", "\n}\n#pragma opt_common_subs reset\n"
    ck(real.count(sig + on) == 1 and reset in real[real.index(sig):], "the fixture's premise: FaceXZ's pair in master's text")
    i = real.index(sig + on)
    j = real.index(reset, i)
    bare = real[:i] + sig + real[i + len(sig + on):j] + "\n}\n" + real[j + len(reset):]
    ck(len(bare) == len(real) - len(on) - len(reset) + 3, "the pair taken out, nothing else")
    work = BUILD / "pragmascan"
    tree = P.Tree(work, 0)
    res = P.Results(work / "results.jsonl", fresh=True)
    pragmas = ["opt_common_subs", "opt_propagation"]
    P.scan_unit(rel, tree, res, "hdr", pragmas, only="^FaceXZ__", text=bare, log=lambda m: None)
    base = res.base[(rel, P.digest(bare), "hdr")]
    ck(name in base["open"] and not base["scores"][name]["match"] and base["scores"][name]["d"] > 0,
       f"FaceXZ open without the pragma ({base['scores'].get(name)})")
    ck(base["probe_ok"] and name in base["sites"] and not base["unconfirmed"], "its body found and confirmed by its probe")
    site = P.Site(name, (), 0, *base["sites"][name][:2])
    ck(P.wrap(bare, site, "opt_common_subs") == real, "wrapped, it is master's text byte for byte")
    v = {r["pragma"]: r for r in res.variants.values() if r["func"] == name}
    ck(P.classify(v["opt_common_subs"]) == "HIT" and v["opt_common_subs"]["lost"] == [] and v["opt_common_subs"]["d1"] == 0,
       f"a HIT under opt_common_subs, nothing lost ({v['opt_common_subs']})")
    ck(not v["opt_propagation"]["match"] and P.classify(v["opt_propagation"]) != "HIT",
       f"no hit under opt_propagation ({v['opt_propagation']['verdict']})")
    again = P.Results(work / "results.jsonl")
    s = P.scan_unit(rel, tree, again, "hdr", pragmas, only="^FaceXZ__", text=bare, log=lambda m: None)
    ck(s["built"] == 0 and s["done"] == 2, f"resumed: nothing rebuilt ({s})")
    ck(P.illegal_pragmas(["opt_bogus_thing"] + list(P.PRAGMAS), {D.COMPILER}, work / "fixture")
       == [f"opt_bogus_thing ({D.COMPILER})"], "an unknown pragma refused, the seven legal")
    ck(P.demangle_scope("Tick__Q28CAiGroup10act_cameraFP11CSealCtrlAif") == ("Tick", ("CAiGroup", "act_camera"))
       and P.demangle_scope("__dt__Q23std48vector<9CMovieCue,Q23std21allocator<9CMovieCue>>Fv")
       == ("~vector", ("std", "vector")) and P.demangle_scope("func_0060AA20") == ("func_0060AA20", ()),
       "mangled scopes")
    text = ("namespace { struct S { int f(int a) const { return a; } }; }\n"
            "#define M(x) int x(void) { return 0; }\nM(g)\n"
            "S::S() : m(f(1)) { }\nint operator==(const S &, const S &) { return 1; }\n")
    got = [(s.name, s.scope) for s in P.sites(text)]
    ck(got == [("f", ("S",)), ("S", ("S",)), ("operator==", ())], f"the text's bodies ({got})")


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
