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
# definition order, as the image holds them; -enum min: inflate's mode is stored by sb.
# README "The compiler" has the derivation.
COMPILER = os.environ.get("SC_COMPILER", "mwcps2-3.0b52-030722")
FLAGS = os.environ.get("SC_FLAGS", "-O4,p -sdatathreshold 0 -inline auto,deferred -enum min").split()
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


def compile_cmd(src: Path, obj: Path, compiler: str = COMPILER, flags: list[str] | None = None,
                includes: list[Path] | None = None) -> list[str]:
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


def _s16(v: int) -> int:
    return v - 0x10000 if v & 0x8000 else v


@dataclass
class RelocFinding:
    offset: int
    type: str
    symbol: str
    want: int | None      # the address the target's field resolves to
    got: int | None       # the address the candidate's relocation names
    status: str           # ok | MISMATCH | content-ok | CONTENT-MISMATCH | unchecked

    def __str__(self) -> str:
        w = f"0x{self.want:08X}" if self.want is not None else "?"
        g = f"0x{self.got:08X}" if self.got is not None else "?"
        return f"+0x{self.offset:03X} {self.type:<14} {self.symbol:<40} target {w}  cand {g}  {self.status}"


def _sym_address(st: E.SymbolTable, name: str) -> int | None:
    s = st._by_name.get(name)
    if s is not None:
        return s.addr
    if E.is_placeholder(name):
        return int(name[-8:], 16)
    return None


def local_object_ok(elf: E.Elf, sec_index: int, start: int, want: int, fsym: E.ElfSymbol,
                    taddr: int, st: E.SymbolTable, img: E.Image, size: int = 0) -> bool | None:
    """Does the candidate's TU-local object at `sec_index`+`start` hold what the target
    holds at `want`? None when there is nothing to compare.

    Words the object's OWN relocations cover are resolved rather than compared raw: an
    entry pointing into the function under comparison -- a switch's jump table -- must
    point at the same offset into the target function, and one naming a mapped symbol
    at that symbol's address. Everything else (string text, float bits) compares byte
    for byte. The extent is the symbol's size when MW gives one, else the run of
    relocated words (a table), else the string up to its terminator."""
    sec = elf.sections[sec_index]
    data = sec.data
    rels = {r.offset: r for r in elf.relocs_for(sec_index)}
    if not size:
        if start in rels:
            while start + size in rels:
                size += 4
        elif b"\0" in data[start:start + 512]:
            size = data.index(b"\0", start) - start + 1
        else:
            size = min(16, len(data) - start)
    if size <= 0 or not img.covers(want, size):
        return None
    tb = img.read(want, size)
    func_sec_base = taddr - fsym.value          # where the function's section would sit
    i = 0
    while i < size:
        r = rels.get(start + i)
        if r is not None and r.type == "R_MIPS_32" and i + 4 <= size:
            field = struct.unpack_from("<I", data, start + i)[0] if r.addend is None else r.addend
            tw = struct.unpack_from("<I", tb, i)[0]
            s = r.sym
            if s.shndx == fsym.shndx:
                base = 0 if s.type == "SECTION" else s.value
                if tw != (func_sec_base + base + field) & 0xFFFFFFFF:
                    return False
            elif s.name and _sym_address(st, s.name) is not None:
                if tw != (_sym_address(st, s.name) + field) & 0xFFFFFFFF:
                    return False
            i += 4                              # an unresolvable pointer is not compared
            continue
        if tb[i] != data[start + i]:
            return False
        i += 1
    return True


def reloc_check(elf: E.Elf, fsym: E.ElfSymbol, relocs: list[E.Reloc], target: bytes,
                taddr: int, cand: bytes, st: E.SymbolTable, img: E.Image) -> list[RelocFinding]:
    out: list[RelocFinding] = []
    hi_pending: dict[str, tuple[int, int]] = {}   # symbol -> (target hi, cand hi field)

    def symbol_addr(r: E.Reloc) -> tuple[str, int | None, int | None, int]:
        """(display name, map address or None, the local object's section index or None,
        the symbol's offset in that section). A named function -- static ones included --
        resolves through the map; a string literal or section symbol has only its content."""
        s = r.sym
        if s.name and (s.bind != 0 or s.type == "FUNC"):
            a = _sym_address(st, s.name)
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
            else:
                # an unnamed static callee: its address is only knowable once the map names it
                out.append(RelocFinding(r.offset, r.type, name, want, None, "unchecked"))
        elif r.type == "R_MIPS_HI16":
            hi_pending[name] = (tw & 0xFFFF, cw & 0xFFFF)
        elif r.type in ("R_MIPS_LO16", "R_MIPS_GPREL16"):
            if r.type == "R_MIPS_LO16":
                if name not in hi_pending:
                    out.append(RelocFinding(r.offset, r.type, name, None, None, "unchecked"))
                    continue
                thi, chi = hi_pending[name]
                want = ((thi << 16) + _s16(tw & 0xFFFF)) & 0xFFFFFFFF
                add = ((chi << 16) + _s16(cw & 0xFFFF)) if r.addend is None else r.addend
            else:
                want = (E.GP + _s16(tw & 0xFFFF)) & 0xFFFFFFFF
                add = _s16(cw & 0xFFFF) if r.addend is None else r.addend
            if addr is not None:
                got = (addr + add) & 0xFFFFFFFF
                out.append(RelocFinding(r.offset, r.type, name, want, got, "ok" if got == want else "MISMATCH"))
            elif local_sec is not None:
                size = r.sym.size if (r.sym.type == "OBJECT" and add == 0) else 0
                ok = local_object_ok(elf, local_sec, local_base + add, want, fsym, taddr, st, img, size)
                status = "unchecked" if ok is None else ("content-ok" if ok else "CONTENT-MISMATCH")
                out.append(RelocFinding(r.offset, r.type, name, want, None, status))
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
        return [f for f in self.relocs if f.status in ("MISMATCH", "CONTENT-MISMATCH")]

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
    # next 16-byte boundary must be zero in the target, or the size is wrong
    pad_end = (sym.addr + len(cand) + 15) & ~15
    padding = img.read(sym.addr + len(cand), pad_end - sym.addr - len(cand))
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
    ap.add_argument("--compiler", default=COMPILER)
    ap.add_argument("--flags", help=f"compiler flags (default {' '.join(FLAGS)})")
    ap.add_argument("-I", dest="includes", action="append", type=Path, default=[])
    ap.add_argument("-v", "--verbose", action="store_true", help="list every relocation")
    a = ap.parse_args(argv)
    img, st = E.image(), E.symbols()
    key = a.target or a.symbol
    try:
        sym = st.lookup(key)
    except KeyError:
        if E.is_placeholder(key):
            raise SystemExit(f"{key} is not in symbols.txt (no size known)")
        raise
    obj = HERE / "build" / "scdiff" / (a.src.stem + ".o")
    compile_obj(a.src, obj, a.compiler, a.flags.split() if a.flags else None,
                [HERE / "include"] + a.includes)
    res = compare(obj, a.symbol, sym, img, st)
    print(report(res, a.verbose))
    return 0 if res.match else 1


if __name__ == "__main__":
    sys.exit(main())
