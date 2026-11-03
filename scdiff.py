#!/usr/bin/env python3
"""
scdiff -- compile a source file with the Metrowerks compiler and diff one function
against the r0001 image.

The candidate is always an unlinked ELF object straight out of mwccps2: its own
relocation table says which fields the linker writes, so exactly those fields are
masked -- on BOTH sides, because the target is absolutely linked and holds the
final values there (scdecomp's docstring). Masking alone would accept a call to
the wrong function, so every relocation is then RESOLVED and checked:

  R_MIPS_26          the target's jal destination == the map address of the
                     symbol the candidate calls (a `func_<VA>` placeholder
                     carries its address in its name)
  R_MIPS_HI16/LO16   the target's %hi/%lo pair, recombined == symbol + addend
  R_MIPS_GPREL16     $gp (0x1DD2F0) + the target's offset == symbol + addend

A reference to a TU-local object (a string literal `@123`, a section symbol) has
no map address; it is checked BY CONTENT instead -- the bytes the candidate's
section holds there against the bytes at the target's address. A relocation that
cannot be checked either way is reported, never passed silently.

  ./scdiff.py src/file.c LoadOverlay__FPCcPvPvi            # by symbol name
  ./scdiff.py src/file.c func_001C5DA0 --flags "-O4,p"
"""

from __future__ import annotations

import argparse
import difflib
import os
import struct
import subprocess
import sys
from dataclasses import dataclass, field
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import scdecomp as E

# --- the compiler ----------------------------------------------------------------

TOOLCHAINS = HERE / "toolchains"
# Settled by measurement, not by the ELF's .comment (which says 2.4.1.01 -- it does not
# name the compiler that built this code): zlib 1.1.4, compiled unmodified, matches all
# 21 of its functions in MAIN with 3.0b52-030722 or 3.0b50-030527 (3.0b38: 20,
# 3.0b22-020926: 17), and only at -O4,p (-O3,p: 18; -O4,s/-O4/-O3: 6). -sdatathreshold 0:
# no $gp small data (the target addresses zlib's version string absolutely); -inline
# auto,deferred: inflateReset is inlined and gone, and functions emit in REVERSE
# definition order, as the image holds them; bottomup: inlining starts at the leaves of the
# call graph, so a callee arrives with its own callees already in it (CZAnimExVector::Free's
# destructor inside std::for_each's function object, the recursive CZAnim walks one level
# deeper -- zlib is the same either way); level=5: bottom-up, a callee whose whole inline
# tree is five or fewer high takes all of it, a taller one only its own body -- the deque's
# teardown inlines five this-tested destructor levels (impossible at 4), CCompass::CreateAnchor
# inlines CCompassAnchor's constructor with its vector's whole, and CVisual's constructor
# CDynTexList's five levels; level 4 had been fitted with every MSL header a level too shallow
# (the inline-level lane, 2026-10-25: refitted at 5, every level-4 match kept but SaveNodeStates,
# whose cut is MW's other limit, inline_max_size, at any level). A callee is also inlined only
# one level when its expanded body exceeds #pragma inline_max_size (256 units by default; leaves
# always inline); README "The compiler". -enum min: inflate's mode is stored by sb;
# -Cpp_exceptions off: C++ tail calls (CMatrix::SetZero is `j memset`; with exceptions on, MW
# keeps a frame around every C++ call, while C code tail-calls either way).
# README "The compiler" has the derivation.
COMPILER = os.environ.get("SC_COMPILER", "mwcps2-3.0b52-030722")
FLAGS = os.environ.get("SC_FLAGS", "-O4,p -sdatathreshold 0 -inline auto,deferred,bottomup,level=5 -enum min -Cpp_exceptions off").split()
BASE_FLAGS = ["-c", "-nostdinc", "-gccinc", "-msgstyle", "gcc"]   # -gccinc: -I paths serve <...> too


def compiler_exe(name: str = COMPILER) -> Path:
    d = TOOLCHAINS / name
    for exe in ("mwccps2.exe", "mwccmips.exe"):
        if (d / exe).is_file():
            return d / exe
    raise FileNotFoundError(
        f"no Metrowerks compiler in {d}; unpack decompme/compilers' {name} there (README)")


# decompals' wibo runs the compiler in ~10 ms against wine's ~2 s, and runs the 2003
# builds that wait on a FlexLM licence prompt under wine. decomp.me uses it too.
WIBO = TOOLCHAINS / "bin" / "wibo"


# --- the GCC front -----------------------------------------------------------------
# About a quarter of the image is prebuilt library code built with Sony's EE-GCC, not MW:
# the SDK in MAIN, libnetb/inet and lgAud in OVL1, the speech recogniser and the Medius/DME
# libraries in OVL2 (README "The GCC front"). A unit whose directive names an `ee-gcc*`
# toolchain -- `/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */` -- is built by that toolchain's
# own driver and its own GNU assembler: natively for the Linux i386 builds, through wibo for
# the Windows PE32 ones (byte-identical to wine's objects, 0.02 s a compile against 4.6 s).
# The toolchains are EQOA's (toolchains/ee-gcc*, linked in; README "Inputs").
GCC_FLAGS = ["-O2", "-G0"]      # a GCC unit's flags when its directive gives none


def is_gcc(compiler: str) -> bool:
    return compiler.startswith("ee-gcc")


def gcc_driver(name: str) -> list[str]:
    d = TOOLCHAINS / name
    for exe in ("bin/ee-gcc", "bin/ee-gcc.exe"):
        p = d / exe
        if p.is_file():
            if p.suffix != ".exe":
                return [str(p)]
            return ([str(WIBO)] if WIBO.is_file() and not os.environ.get("SC_WINE") else ["wine"]) + [str(p)]
    raise FileNotFoundError(f"no ee-gcc driver in {d}; link EQOA's toolchains/{name} there (README)")


def compile_cmd(src: Path, obj: Path, compiler: str = COMPILER, flags: list[str] | None = None,
                includes: list[Path] | None = None) -> list[str]:
    # An assembler unit (`.s`) goes through the same ee-gcc driver, which hands it straight to its
    # own GNU as (`as -G0 -EL -O2 -mabi=eabi`: 2.9-ee-991111b under 2.9-991111, 2.10-ee-001003-1
    # under 2.96) -- no other tool and no other flag. The demo's symbols say which library objects
    # were assembly (README "The GCC front"); Metrowerks' code has none, so MW has no such path.
    if Path(src).suffix == ".s" and not is_gcc(compiler):
        raise RuntimeError(f"{src}: an assembler unit names its ee-gcc toolchain "
                           "(`# sc: compiler=ee-gcc2.9-991111`); Metrowerks has no assembler path here")
    if is_gcc(compiler):
        cmd = gcc_driver(compiler) + ["-c"] + (GCC_FLAGS if flags is None else flags)
        for inc in includes or []:
            cmd += ["-I" + str(inc)]
        return cmd + [str(src), "-o", str(obj)]
    runner = [str(WIBO)] if WIBO.is_file() and not os.environ.get("SC_WINE") else ["wine"]
    cmd = runner + [str(compiler_exe(compiler))] + BASE_FLAGS + (FLAGS if flags is None else flags)
    for inc in includes or []:
        cmd += ["-I" + str(inc)]
    return cmd + [str(src), "-o", str(obj)]


def compile_obj(src: Path, obj: Path, compiler: str = COMPILER, flags: list[str] | None = None,
                includes: list[Path] | None = None) -> subprocess.CompletedProcess:
    obj.parent.mkdir(parents=True, exist_ok=True)
    if obj.exists():
        obj.unlink()
    env = dict(os.environ, WINEDEBUG="-all")
    if is_gcc(compiler):
        # the driver's temporaries (the .i and .s it hands on) go where $TMPDIR says: never /tmp
        tmp = HERE / "build" / "tmp"
        tmp.mkdir(parents=True, exist_ok=True)
        env.update(TMPDIR=str(tmp), TMP=str(tmp), TEMP=str(tmp))
    try:
        # a build that wants a licence (mwcps2-2.3.3-000906 does) waits forever under wine
        r = subprocess.run(compile_cmd(src, obj, compiler, flags, includes), capture_output=True,
                           text=True, env=env, timeout=120)
    except subprocess.TimeoutExpired:
        raise RuntimeError(f"compile timed out (a licence prompt?): {compiler} {src}") from None
    if r.returncode != 0 or not obj.exists():
        raise RuntimeError(f"compile failed: {src}\n{r.stdout}{r.stderr}")
    return r


# --- masking and the relocation check --------------------------------------------

FIELD = {
    "R_MIPS_26": 0x03FFFFFF,
    "R_MIPS_HI16": 0xFFFF,
    "R_MIPS_LO16": 0xFFFF,
    "R_MIPS_GPREL16": 0xFFFF,
    "R_MIPS_LITERAL": 0xFFFF,
    "R_MIPS_PC16": 0xFFFF,
    "R_MIPS_32": 0xFFFFFFFF,
}

# known non-zero words in a function's 16-byte padding, by address (see compare())
PAD_EXCEPTIONS = {0x1E70F4: 0xFFFFFFFF}


def _s16(v: int) -> int:
    return v - 0x10000 if v & 0x8000 else v


@dataclass
class RelocFinding:
    offset: int
    type: str
    symbol: str
    want: int | None      # the address the target's field resolves to
    got: int | None       # the address the candidate's relocation names
    status: str           # ok | MISMATCH | content-ok | CONTENT-MISMATCH | UNKNOWN | unchecked

    def __str__(self) -> str:
        w = f"0x{self.want:08X}" if self.want is not None else "?"
        g = f"0x{self.got:08X}" if self.got is not None else "?"
        return f"+0x{self.offset:03X} {self.type:<14} {self.symbol:<40} target {w}  cand {g}  {self.status}"


def _sym_address(st: E.SymbolTable, name: str, near: int | None = None) -> int | None:
    # a function, or a global data.txt names; from code at `near`, its own overlay's copy
    a = st.address_of(name) if near is None else st.address_in(name, near)
    if a is not None:
        return a
    if E.is_placeholder(name):
        return int(name[-8:], 16)
    return None


SHF_EXECINSTR = 0x4

# the bytes a load or store moves, by its major opcode (lb lh lw lbu lhu lwu sb sh sw lwc1 swc1 ld sd,
# the EE's lq/sq and VU0's lqc2/sqc2); an addiu or ori only computes an address
ACCESS_WIDTH = {0x20: 1, 0x24: 1, 0x28: 1, 0x21: 2, 0x25: 2, 0x29: 2, 0x23: 4, 0x27: 4, 0x2B: 4,
                0x31: 4, 0x39: 4, 0x37: 8, 0x3F: 8, 0x1E: 16, 0x1F: 16, 0x36: 16, 0x3E: 16}


def lit_entry(elf: E.Elf, sec_index: int, off: int) -> int | None:
    """The bytes left in the literal-pool entry holding `off`, when section `sec_index` is GNU as's
    .lit4 or .lit8 (4- and 8-byte entries, each a constant of its own), else None."""
    width = {".lit4": 4, ".lit8": 8}.get(elf.sections[sec_index].name)
    return width - off % width if width else None


def access_width(code: bytes, off: int) -> int:
    """How many bytes the instruction at `off` loads or stores (0: it only takes an address)."""
    if off + 4 > len(code):
        return 0
    return ACCESS_WIDTH.get(struct.unpack_from("<I", code, off)[0] >> 26, 0)


def local_extent(elf: E.Elf, sec_index: int, off: int, rsym: E.ElfSymbol,
                 st: E.SymbolTable) -> tuple[int, int, str] | None:
    """The TU-local object a reference to `sec_index`+`off` lands in, whole: (its start, its size,
    its name). Metrowerks relocates against the object itself (`@123`, a static's own symbol); a
    GNU object relocates against the section, and GCC labels every static there with a sized LOCAL
    OBJECT symbol (`.size x,N`) -- the one containing `off`. A label with no size (an assembler
    unit's) takes the size of the data map's row of its name. None when no sized symbol covers
    `off` (a GNU string or float literal, which has no symbol)."""
    if rsym.type != "SECTION" and rsym.shndx == sec_index:
        cands = [rsym]
    else:
        cands = [s for s in elf.symbols if s.shndx == sec_index and s.bind == 0 and s.name
                 and s.type in ("OBJECT", "NOTYPE") and s.value <= off]
    best = None
    for s in cands:
        size = s.size or (st.data_by_name[s.name].size if s.name in st.data_by_name else 0)
        if size and s.value <= off < s.value + size and (best is None or s.value > best[0]):
            best = (s.value, size, s.name)
    return best


def table_word_ok(elf: E.Elf, s: E.ElfSymbol, field: int, tw: int, fsym: E.ElfSymbol, taddr: int,
                  st: E.SymbolTable, img: E.Image, depth: int) -> bool | None:
    """Does a relocated word of the candidate's TU-local object -- symbol `s` + `field` -- name what
    the target's word `tw` does? None when it cannot be resolved (it is then not compared).

    A pointer into code resolves through the CANDIDATE's own function at that offset: into the
    function under comparison (a switch's jump table) at the same offset into the target function;
    into another function by that function's map address (a GNU table of static functions is
    `.text`+N, and an OPEN function earlier in the unit, compiling to another size, moves every
    later one -- the recogniser's _LangVirtualTable); a function the map does not name by the same
    distance from this one as in the target. A pointer to a mapped symbol: its address. A pointer
    into the object's own data (a string, another static): what it points at, one level deep."""
    mask32 = 0xFFFFFFFF
    known = 0 < s.shndx < len(elf.sections)
    if known and elf.sections[s.shndx].flags & SHF_EXECINSTR:
        off = (0 if s.type == "SECTION" else s.value) + field
        f = next((g for g in elf.functions() if g.shndx == s.shndx
                  and g.value <= off < g.value + max(g.size, 1)), None)
        if f is not None and f.shndx == fsym.shndx and f.value == fsym.value:
            return tw == (taddr + off - fsym.value) & mask32
        if f is not None:
            a = _sym_address(st, f.name, taddr)
            if a is not None:
                return tw == (a + off - f.value) & mask32
        if s.shndx == fsym.shndx:
            return tw == (taddr - fsym.value + off) & mask32
        return None
    if s.name and s.type != "SECTION":
        a = _sym_address(st, s.name, taddr)
        if a is not None:
            return tw == (a + field) & mask32
    if known and s.bind == 0 and depth == 0:
        off = (0 if s.type == "SECTION" else s.value) + field
        ext = local_extent(elf, s.shndx, off, s, st)
        if ext is not None and ext[2] in st.data_by_name:
            return tw == (st.data_by_name[ext[2]].addr + off - ext[0]) & mask32
        start, size = (ext[0], ext[1]) if ext is not None else (off, 0)
        return local_object_ok(elf, s.shndx, start, (tw - (off - start)) & mask32, fsym, taddr, st,
                               img, size, depth=depth + 1)
    return None


def local_object_ok(elf: E.Elf, sec_index: int, start: int, want: int, fsym: E.ElfSymbol,
                    taddr: int, st: E.SymbolTable, img: E.Image, size: int = 0,
                    at_least: int = 0, depth: int = 0) -> bool | None:
    """Does the candidate's TU-local object at `sec_index`+`start` hold what the target
    holds at `want`? None when there is nothing to compare.

    Words the object's OWN relocations cover are resolved rather than compared raw
    (table_word_ok): a jump table's entries, a table of functions, of strings. Everything
    else (string text, float bits, a null slot) compares byte for byte. The extent is the
    object's whole size when a symbol gives one (local_extent); else, for an unnamed literal,
    the run of relocated words (a table), the string up to its terminator, or 16 bytes --
    and at least `at_least`, the width the referencing instruction loads (a float literal
    whose first byte is zero is not a one-byte string)."""
    sec = elf.sections[sec_index]
    data = sec.data
    if sec.type == 8:                           # SHT_NOBITS (.bss): a zeroed static's bytes
        data = bytes(sec.size)
    if not 0 <= start < len(data):
        return None
    rels = {r.offset: r for r in elf.relocs_for(sec_index)}
    if not size:
        if start in rels:
            while start + size in rels:
                size += 4
        elif b"\0" in data[start:start + 512]:
            size = data.index(b"\0", start) - start + 1
        else:
            size = min(16, len(data) - start)
        size = max(size, at_least)
    size = min(size, len(data) - start)
    if size <= 0 or not img.covers(want, size):
        return None
    tb = img.read(want, size)
    i = 0
    while i < size:
        r = rels.get(start + i)
        if r is not None and r.type == "R_MIPS_32" and i + 4 <= size:
            field = struct.unpack_from("<I", data, start + i)[0] if r.addend is None else r.addend
            tw = struct.unpack_from("<I", tb, i)[0]
            if table_word_ok(elf, r.sym, field, tw, fsym, taddr, st, img, depth) is False:
                return False
            i += 4                              # an unresolvable pointer is not compared
            continue
        if tb[i] != data[start + i]:
            return False
        i += 1
    return True


def is_gnu(elf: E.Elf) -> bool:
    """A GCC object (GNU as writes .reginfo; Metrowerks never does)."""
    return any(s.name == ".reginfo" for s in elf.sections)


def qualify_statics(obj: Path, stem: str, st: E.SymbolTable) -> list[tuple[str, str]]:
    """A GCC unit's static functions are its own, and several objects of one library can define a
    static of the same name -- libkernl's syscall wrappers `Copy`, `kCopy`, `setup` and
    `GetEntryAddress` sit in five objects. The map holds such a row as `NAME@UNIT` (UNIT the source
    file's stem: `Copy@tlbfunc`), and a GNU object's LOCAL function `NAME` is renamed so before it
    is scored, when the map has that row; a static the map names bare scores bare. Metrowerks
    objects are left alone. Returns the renames made."""
    elf = E.Elf.read(obj)
    if not is_gnu(elf):
        return []
    pairs = []
    for f in elf.functions():
        q = f"{f.name}@{stem}"
        if f.bind == 0 and f.name and q in st._by_name and (f.name, q) not in pairs:
            pairs.append((f.name, q))
    if pairs:
        subprocess.run([E.tool("objcopy")] + [f"--redefine-sym={a}={b}" for a, b in pairs] + [str(obj)],
                       check=True, capture_output=True)
    return pairs


def bss_local(elf: E.Elf, sec_index: int, off: int, st: E.SymbolTable) -> tuple[str, int] | None:
    """A GNU object's reference into its own .bss (`.bss`+N: GNU as relocates a local label
    against its section) as (label, address), when the label at or before N is one the data map
    names: zeroed data has no content to compare, so only the map can check it. None otherwise
    (and for any other section, whose content local_object_ok compares)."""
    if not is_gnu(elf) or not 0 < sec_index < len(elf.sections) or elf.sections[sec_index].type != 8:
        return None
    best = None
    for s in elf.symbols:
        if s.shndx == sec_index and s.bind == 0 and s.type != "SECTION" and s.name and s.value <= off:
            if best is None or s.value > best.value:
                best = s
    if best is None or best.name not in st.data_by_name:
        return None
    return best.name, st.data_by_name[best.name].addr + off - best.value


def code_label(elf: E.Elf, sec_index: int, off: int, st: E.SymbolTable) -> tuple[str, int] | None:
    """A GNU object's reference into its own code at a label that is no function (`.text`+N: an
    asm's data label, GNU as relocating a local label against its section) as (label, address),
    when a LOCAL label sits exactly at N and the data map names it. None otherwise."""
    for s in elf.symbols:
        if (s.shndx == sec_index and s.bind == 0 and s.value == off and s.name
                and s.type not in ("SECTION", "FUNC") and s.name in st.data_by_name):
            return s.name, st.data_by_name[s.name].addr
    return None


def gnu_hi_partners(elf: E.Elf, fsym: E.ElfSymbol, cand: bytes) -> dict[int, int]:
    """In a GNU object, each R_MIPS_LO16's R_MIPS_HI16 (offsets from the function's start).

    GNU as relocates every TU-local reference against its SECTION symbol -- each string
    literal is `.rodata`+N -- so two %hi/%lo pairs of one section interleave under one name
    (`lui a0,%hi(.rodata); lui a1,%hi(.rodata+8); addiu a0,a0,%lo(.rodata)`), and pairing by
    name in offset order mismatches them. The linker pairs them by the table's order, which
    GNU as writes with each %hi immediately before its first %lo; a later %lo sharing a %hi
    takes the latest one before it whose lui writes its base register (else the latest)."""
    out: dict[int, int] = {}
    lo, hi = fsym.value, fsym.value + fsym.size
    for sec in elf.sections:
        if sec.type not in (E.SHT_REL, E.SHT_RELA) or sec.info != fsym.shndx:
            continue
        step = 8 if sec.type == E.SHT_REL else 12
        seen: dict[int, list[int]] = {}        # symbol index -> its HI16 offsets so far
        prev = None
        for j in range(sec.size // step):
            off, info = struct.unpack_from("<II", sec.data, j * step)
            typ, si = info & 0xFF, info >> 8
            if not lo <= off < hi:
                prev = None
                continue
            off -= lo
            if typ == 5:                       # R_MIPS_HI16
                seen.setdefault(si, []).append(off)
                prev = (si, off)
                continue
            if typ == 6 and si in seen:        # R_MIPS_LO16
                if prev is not None and prev[0] == si:
                    out[off] = prev[1]
                else:
                    base = (struct.unpack_from("<I", cand, off)[0] >> 21) & 31 if off + 4 <= len(cand) else -1
                    regs = [h for h in seen[si]
                            if h + 4 <= len(cand) and (struct.unpack_from("<I", cand, h)[0] >> 16) & 31 == base]
                    out[off] = (regs or seen[si])[-1]
            prev = None
    return out


def reloc_check(elf: E.Elf, fsym: E.ElfSymbol, relocs: list[E.Reloc], target: bytes,
                taddr: int, cand: bytes, st: E.SymbolTable, img: E.Image) -> list[RelocFinding]:
    out: list[RelocFinding] = []
    hi_pending: dict[str, tuple[int, int]] = {}   # symbol -> (target hi, cand hi field)
    gnu = is_gnu(elf)
    partner = gnu_hi_partners(elf, fsym, cand) if gnu else {}
    gp0 = next((struct.unpack_from("<i", s.data, 20)[0] for s in elf.sections
                if s.name == ".reginfo" and len(s.data) >= 24), 0) if gnu else 0

    def code_symbol(sec_index: int, value: int) -> str | None:
        """A GNU object's local reference into its own code (`.text`+N): the function there."""
        for s in elf.functions():
            if s.shndx == sec_index and s.value == value:
                return s.name
        return None

    def local_code(r: E.Reloc, name: str, sec_index: int, value: int, want: int) -> RelocFinding | None:
        # a static callee (or a function's address) relocated against the section: resolved by
        # the function at that offset -- through the map when it names it (a placeholder by
        # the address in its name), else by the same distance from this function as in the target
        if not gnu or sec_index != fsym.shndx:
            return None
        callee = code_symbol(sec_index, value)
        a = _sym_address(st, callee, taddr) if callee else None
        if a is not None:
            return RelocFinding(r.offset, r.type, callee, want, a, "ok" if a == want else "MISMATCH")
        if callee is None and (lab := code_label(elf, sec_index, value, st)) is not None:
            # an asm label inside the unit's code that the data map names (libmpeg's _maxval, the
            # clamp constant _copyRefImage's asm carries and _copyAddRefImage loads): its address,
            # whatever the unit's layout before it
            return RelocFinding(r.offset, r.type, lab[0], want, lab[1], "ok" if lab[1] == want else "MISMATCH")
        if want - taddr == value - fsym.value:
            return RelocFinding(r.offset, r.type, callee or name, want, want, "ok")
        return RelocFinding(r.offset, r.type, callee or name, want, None, "unchecked")

    def symbol_addr(r: E.Reloc) -> tuple[str, int | None, int | None, int]:
        """(display name, map address or None, the local object's section index or None,
        the symbol's offset in that section). A named function -- static ones included --
        resolves through the map; a string literal or section symbol has only its content."""
        s = r.sym
        # a TU-local object the data map names (a static table) resolves by its address, as a
        # global does; an unnamed one -- a string literal, a float constant -- by content, and so
        # does MW's string pool of an inline function (@STRING@<function>, not local-bound so the
        # units that emit the function can share it)
        if (s.name and not s.name.startswith("@STRING@")
                and (s.bind != 0 or s.type == "FUNC" or s.name in st.data_by_name)):
            a = _sym_address(st, s.name, taddr)
            if a is not None or s.bind != 0:
                return s.name, a, None, 0
        ok = 0 < s.shndx < len(elf.sections)
        name = s.name or (elf.sections[s.shndx].name if ok else "?")
        return f"{name}(local)", None, (s.shndx if ok else None), (0 if s.type == "SECTION" else s.value)

    for i, r in enumerate(relocs):
        if r.offset + 4 > min(len(target), len(cand)):
            continue
        tw = struct.unpack_from("<I", target, r.offset)[0]
        cw = struct.unpack_from("<I", cand, r.offset)[0]
        name, addr, local_sec, local_base = symbol_addr(r)
        if r.type == "R_MIPS_26":
            want = ((taddr + r.offset + 4) & 0xF0000000) | ((tw & 0x03FFFFFF) << 2)
            add = (cw & 0x03FFFFFF) << 2 if r.addend is None else r.addend
            if addr is not None:
                got = addr + add
                out.append(RelocFinding(r.offset, r.type, name, want, got, "ok" if got == want else "MISMATCH"))
            elif r.sym.bind != 0 and r.sym.name:
                # a global callee the map does not name: the call cannot be shown right, and a
                # misspelt name (SOCOM 1's template mangling, without MW 3.0's return suffix)
                # must not pass as a match
                out.append(RelocFinding(r.offset, r.type, name, want, None, "UNKNOWN"))
            else:
                # an unnamed static callee: its address is only knowable once the map names it --
                # unless a GCC unit calls it through its section (`.text`+N): then by the function there
                found = local_code(r, name, local_sec, local_base + add, want) if local_sec is not None else None
                out.append(found or RelocFinding(r.offset, r.type, name, want, None, "unchecked"))
        elif r.type == "R_MIPS_HI16":
            hi_pending[name] = (tw & 0xFFFF, cw & 0xFFFF)
        elif r.type in ("R_MIPS_LO16", "R_MIPS_GPREL16") or (gnu and r.type == "R_MIPS_LITERAL"):
            if r.type == "R_MIPS_LO16":
                if r.offset in partner and partner[r.offset] + 4 <= min(len(target), len(cand)):
                    h = partner[r.offset]          # a GCC unit's pair, by the table (gnu_hi_partners)
                    thi = struct.unpack_from("<I", target, h)[0] & 0xFFFF
                    chi = struct.unpack_from("<I", cand, h)[0] & 0xFFFF
                elif name not in hi_pending:
                    out.append(RelocFinding(r.offset, r.type, name, None, None, "unchecked"))
                    continue
                else:
                    thi, chi = hi_pending[name]
                want = ((thi << 16) + _s16(tw & 0xFFFF)) & 0xFFFFFFFF
                add = ((chi << 16) + _s16(cw & 0xFFFF)) if r.addend is None else r.addend
            else:
                # $gp-relative: GPREL16, and a GCC unit's literal pool (.lit4/.lit8, -G 8 and up).
                # GNU as writes a TU-local symbol's field less the object's own gp (.reginfo's
                # ri_gp_value, 0x4000 from ee-gcc), which the linker adds back
                want = (E.GP + _s16(tw & 0xFFFF)) & 0xFFFFFFFF
                add = _s16(cw & 0xFFFF) if r.addend is None else r.addend
                if gnu and r.sym.bind == 0 and r.addend is None:
                    add += gp0
            found = local_code(r, name, local_sec, local_base + add, want) if local_sec is not None else None
            if found is not None:
                out.append(found)
            elif addr is not None:
                got = (addr + add) & 0xFFFFFFFF
                status = "ok" if got == want else "MISMATCH"
                s = r.sym
                if (status == "ok" and s.bind == 0 and s.type != "FUNC"
                        and 0 < s.shndx < len(elf.sections)):
                    # a TU-local static the data map names: its address, and what it holds -- whole
                    ext = local_extent(elf, s.shndx, s.value + add, s, st)
                    if ext is not None:
                        ok = local_object_ok(elf, s.shndx, ext[0], (want - (s.value + add - ext[0])) & 0xFFFFFFFF,
                                             fsym, taddr, st, img, ext[1])
                        status = "CONTENT-MISMATCH" if ok is False else status
                out.append(RelocFinding(r.offset, r.type, name, want, got, status))
            elif r.sym.bind != 0 and r.sym.name and not r.sym.name.startswith("@STRING@"):
                out.append(RelocFinding(r.offset, r.type, name, want, None, "UNKNOWN"))   # see R_MIPS_26
            elif local_sec is not None and (named := bss_local(elf, local_sec, local_base + add, st)) is not None:
                # a GNU object's own zeroed data (.bss+N), which has no content to compare: by the
                # local label there when the data map names it (an assembler unit's save areas)
                got = named[1]
                out.append(RelocFinding(r.offset, r.type, named[0], want, got, "ok" if got == want else "MISMATCH"))
            elif local_sec is not None:
                # a TU-local object: compared whole (local_extent), at the target address the
                # reference's offset into it implies; a GNU static the data map names (relocated
                # against its section, so only its label knows the name) by its address too
                off = local_base + add
                ext = local_extent(elf, local_sec, off, r.sym, st)
                if ext is not None:
                    start, size, oname = ext
                    ok = local_object_ok(elf, local_sec, start, (want - (off - start)) & 0xFFFFFFFF,
                                         fsym, taddr, st, img, size)
                elif (pool := lit_entry(elf, local_sec, off)) is not None:
                    # GNU as's literal pools (.lit4/.lit8, -G 8 and up) hold one constant per entry,
                    # merged entry by entry at link time: the reference's own entry, not its
                    # neighbours (whose order is the object's first uses, not the image's)
                    oname = None
                    ok = local_object_ok(elf, local_sec, off, want, fsym, taddr, st, img, pool)
                else:
                    oname = None
                    ok = local_object_ok(elf, local_sec, off, want, fsym, taddr, st, img,
                                         at_least=access_width(cand, r.offset))
                status = "unchecked" if ok is None else ("content-ok" if ok else "CONTENT-MISMATCH")
                got = None
                if gnu and r.sym.type == "SECTION" and oname in st.data_by_name:
                    got = (st.data_by_name[oname].addr + off - ext[0]) & 0xFFFFFFFF
                    name = oname
                    status = "MISMATCH" if got != want else ("CONTENT-MISMATCH" if ok is False else "ok")
                out.append(RelocFinding(r.offset, r.type, name, want, got, status))
            else:
                out.append(RelocFinding(r.offset, r.type, name, want, None, "unchecked"))
        else:
            out.append(RelocFinding(r.offset, r.type, name, None, None, "unchecked"))
    return out


# --- the diff --------------------------------------------------------------------

@dataclass
class Result:
    symbol: str
    addr: int
    target_size: int
    cand_size: int
    masked_equal: bool
    padding_ok: bool
    relocs: list[RelocFinding] = field(default_factory=list)
    diff_lines: list[str] = field(default_factory=list)
    differing: int = 0

    @property
    def reloc_bad(self) -> list[RelocFinding]:
        return [f for f in self.relocs if f.status in ("MISMATCH", "CONTENT-MISMATCH", "UNKNOWN")]

    @property
    def reloc_unchecked(self) -> list[RelocFinding]:
        return [f for f in self.relocs if f.status == "unchecked"]

    @property
    def match(self) -> bool:
        return self.masked_equal and self.padding_ok and not self.reloc_bad

    @property
    def verdict(self) -> str:
        if self.match:
            extra = f", {len(self.reloc_unchecked)} relocation(s) unchecked" if self.reloc_unchecked else ""
            return f"MATCH ({len(self.relocs)} relocations resolved{extra})"
        if self.masked_equal and self.reloc_bad:
            return f"WRONG REFERENCE ({len(self.reloc_bad)} relocation(s) name another address)"
        if self.cand_size != self.target_size:
            return f"FAIL (size 0x{self.cand_size:X} vs target 0x{self.target_size:X})"
        if not self.masked_equal:
            return f"FAIL ({self.differing} differing instruction(s))"
        return "FAIL (target padding after the function is not zero)"


def mask(b: bytes, relocs: list[E.Reloc]) -> bytes:
    out = bytearray(b)
    for r in relocs:
        m = FIELD.get(r.type)
        if m is not None and r.offset + 4 <= len(out):
            w = struct.unpack_from("<I", out, r.offset)[0]
            struct.pack_into("<I", out, r.offset, w & ~m)
    return bytes(out)


def compare(obj: Path, name: str, sym: E.Symbol, img: E.Image, st: E.SymbolTable,
            context: int = 3) -> Result:
    elf = E.Elf.read(obj)
    cand, relocs, fsym = elf.function(name)
    target = img.read(sym.addr, max(sym.size, len(cand)))
    # MW pads each function to 16 bytes; whatever follows the candidate up to the
    # next 16-byte boundary must be zero in the target, or the size is wrong -- unless the
    # next object starts sooner: the last function of a unit can be followed directly by an
    # 8-aligned GCC-built library object (func_001E7EF0, then flushbuffer at +0x68)
    pad_end = (sym.addr + len(cand) + 15) & ~15
    import bisect
    k = bisect.bisect_right(st._addrs, sym.addr)
    nxt = st._addrs[k] if k < len(st._addrs) else None
    if nxt is not None and sym.addr + sym.size <= nxt < pad_end:
        pad_end = max(nxt, sym.addr + len(cand))
    padding = bytearray(img.read(sym.addr + len(cand), pad_end - sym.addr - len(cand)))
    # the image's one known non-zero pad word: 0xFFFFFFFF at 0x1E70F4, after main, OVL1's
    # first function (tools/seed_symbols.py's sweep skips it too)
    for a, w in PAD_EXCEPTIONS.items():
        off = a - (sym.addr + len(cand))
        if 0 <= off <= len(padding) - 4 and padding[off:off + 4] == w.to_bytes(4, "little"):
            padding[off:off + 4] = b"\0\0\0\0"
    padding = bytes(padding)
    t = mask(target[:len(cand)], relocs)
    c = mask(cand, relocs)
    res = Result(name, sym.addr, sym.size, len(cand),
                 masked_equal=(t == c and len(cand) == sym.size),
                 padding_ok=(padding.strip(b"\0") == b""))
    res.relocs = reloc_check(elf, fsym, relocs, target, sym.addr, cand, st, img)
    if not res.masked_equal:
        tw, cw = E.words(mask(img.read(sym.addr, sym.size), relocs)), E.words(c)
        res.differing = sum(1 for a, b in zip(tw, cw) if a != b) + abs(len(tw) - len(cw))
        dt = E.disasm(mask(img.read(sym.addr, sym.size), relocs), sym.addr)
        dc = E.disasm(c, sym.addr)
        res.diff_lines = list(difflib.unified_diff(
            [f"{w:08x}  {x}" for _, w, x in dt], [f"{w:08x}  {x}" for _, w, x in dc],
            fromfile=f"target {sym.name} 0x{sym.addr:08X}", tofile=f"candidate {name}",
            lineterm="", n=context))
    return res


def report(res: Result, verbose: bool = False) -> str:
    out = [f"{res.symbol} @ 0x{res.addr:08X}: target 0x{res.target_size:X} B, candidate 0x{res.cand_size:X} B",
           f"  {res.verdict}"]
    shown = res.relocs if verbose else res.reloc_bad + res.reloc_unchecked
    for f in shown:
        out.append(f"    {f}")
    if res.diff_lines:
        out.append("")
        out.extend(res.diff_lines)
    return "\n".join(out)


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("src", type=Path)
    ap.add_argument("symbol", help="the function's name in the object (mangled for C++)")
    ap.add_argument("--target", help="map name or address to diff against (default: the symbol itself)")
    ap.add_argument("--compiler", help=f"toolchain (default: the unit's `sc: compiler=`, else {COMPILER})")
    ap.add_argument("--flags", help=f"compiler flags (default: the unit's `sc: flags=`, else {' '.join(FLAGS)})")
    ap.add_argument("-I", dest="includes", action="append", type=Path, default=[])
    ap.add_argument("-v", "--verbose", action="store_true", help="list every relocation")
    a = ap.parse_args(argv)
    from sccheck import tu_options             # (sccheck imports this module)
    opts = tu_options(a.src)
    a.compiler = a.compiler or opts.get("compiler", COMPILER)
    a.flags = a.flags or opts.get("flags")
    img, st = E.image(), E.symbols()
    key = a.target or a.symbol
    try:
        sym = st.lookup(key)
    except KeyError:
        if E.is_placeholder(key):
            raise SystemExit(f"{key} is not in symbols.txt (no size known)")
        raise
    # named after the whole path (and the process): concurrent diffs never share an object
    tag = str(a.src.resolve()).strip("/").replace("/", "_")
    obj = HERE / "build" / "scdiff" / f"{tag}.{os.getpid()}.o"
    compile_obj(a.src, obj, a.compiler, a.flags.split() if a.flags else None,
                [HERE / "include"] + a.includes)
    qualify_statics(obj, a.src.stem, st)
    try:
        res = compare(obj, a.symbol, sym, img, st)
    finally:
        obj.unlink(missing_ok=True)
    print(report(res, a.verbose))
    return 0 if res.match else 1


if __name__ == "__main__":
    sys.exit(main())
