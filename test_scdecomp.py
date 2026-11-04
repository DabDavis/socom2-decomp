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


def test_explicit_instance_scores_in_its_units_overlay():
    """seal_tasks.cpp (OVL2) instantiates `ai::_max<unsigned char>` explicitly: a global, the
    unit's own, whose name the map gives OVL1's 0x2AFC40 and whose OVL2 copy is 0x5DEE70
    (`ovl-copy:`). By the unit's lowest address the instance made the unit an OVL1 one, so it was
    scored -- and credited -- at OVL1's row, OVL2's never (the kit lane's round 24, GCC round 25a).
    The unit's overlay is now the one its functions' rows lie in, counting the names the map holds
    once: OVL2, the instance scored at 0x5DEE70 and the span inside OVL2. With the copy's mark gone
    the named row is all there is again; the old rule's lowest address is OVL1's."""
    import contextlib
    import io
    img, st = E.image(), E.symbols()
    name = "_max<Uc>__2aiFRCUcRCUc_RCUc"
    files = r"^gamez/zseal/seal_tasks\.cpp$"
    scored = []
    orig, orig_sym = D.compare, E.symbols

    def run(table):
        def compare(obj, fn, sym, *a, **k):
            r = orig(obj, fn, sym, *a, **k)
            scored.append((fn, sym.addr, r.match))
            return r
        D.compare, E.symbols = compare, (lambda: table)
        scored.clear()
        try:
            with contextlib.redirect_stdout(io.StringIO()):
                C.main(["--files", files, "--build-dir", str(BUILD / "ovlhome"), "-j", "2"])
        finally:
            D.compare, E.symbols = orig, orig_sym
        return [(a, m) for fn, a, m in scored if fn == name]

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
