#!/usr/bin/env python3
"""
scdecomp -- the SOCOM II r0001 image, its symbol map, ELF reading and disassembly.

The core library of the decomp, modelled on claude-eqoa's eqdecomp.py. Everything
here is offline and stdlib-only; the binutils it shells out to are the ps2dev EE
ones ($PS2DEV/ee/bin, default ~/ps2dev/ps2dev).

THE TARGET is the disc as shipped: SCUS-97275 r0001 ("SOCOM 2 r0001 17:22:21 Oct 11
2003"), not the r0004 online update every memory dump in this project holds. It is
three pieces of one address space:

  MAIN  0x00100000  the loader ELF's one non-empty LOAD segment (SCUS_972.75)
  OVL1  0x001E7000  ftscore.bin   -- MWo3 overlay, decrypted off the disc
  OVL2  0x004C5380  zsealetc.bin  -- MWo3 overlay, decrypted off the disc

The overlays are produced by tools/decrypt_overlays.py from your own disc. Their
digests are PINNED below: a different disc or a drifted decrypt refuses to load
rather than scoring the source against bytes that are not the shipped code.

Unlike claude-eqoa's modules (relocatable images whose call and address fields
read zero until loaded), all three pieces are ABSOLUTELY LINKED: every jal target
and every %hi/%lo pair is filled in. So a comparison masks the TARGET at the
candidate object's own relocation sites, and a second check (scdiff.reloc_check)
resolves each relocation and demands it name the same address the target does --
byte-exact after masking is necessary, not sufficient.
"""

from __future__ import annotations

import hashlib
import os
import re
import struct
import subprocess
from dataclasses import dataclass, field
from functools import lru_cache
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = Path(os.environ.get("SOCOM_ROOT") or HERE.parent)

LOADER = Path(os.environ.get("SOCOM_LOADER") or ROOT / "extracted" / "SCUS_972.75")
OVERLAYS = Path(os.environ.get("SOCOM_OVERLAYS") or ROOT / "extracted" / "overlays")
SYMBOLS = HERE / "symbols.txt"
DATA = HERE / "data.txt"          # global data, the same row format; kept apart so every
                                  # tool that walks the function map stays a function walk

# (size, sha256) of each input, for the r0001 disc. The overlay digests agree with
# what socom-unzipped pins for the same disc (tools_py/disc_to_elf_expected.json).
PINNED = {
    "SCUS_972.75": (874792, "0172dc0bec19c83d1fe2d0fec0a290f41cc5d32efa14ed6859ca92773c05346c"),
    "ftscore.bin": (2233472, "cf09a7fabaf59a1d68f01411bd1db9e859af054df1f2fbc4775d8c221785f836"),
    "zsealetc.bin": (1723520, "cdb3fa8e707090ac879874e3987c6b7784808a4767e85d0e19fc6909d36549b1"),
}

MAIN_BASE = 0x00100000
OVL1_BASE = 0x001E7000
OVL2_BASE = 0x004C5380
GP = 0x001DD2F0           # SCUS_972.75 .reginfo ri_gp_value


def _check_pinned(path: Path, data: bytes) -> None:
    size, digest = PINNED[path.name]
    got = hashlib.sha256(data).hexdigest()
    if len(data) != size or got != digest:
        raise RuntimeError(
            f"{path}: {len(data)} B sha256 {got[:16]}..., expected {size} B {digest[:16]}... -- "
            "not the r0001 disc, or a different decrypt; see tools/decrypt_overlays.py")


# --- ELF ---------------------------------------------------------------------

SHT_SYMTAB, SHT_RELA, SHT_REL = 2, 4, 9
STT_NAMES = {0: "NOTYPE", 1: "OBJECT", 2: "FUNC", 3: "SECTION", 4: "FILE"}
R_MIPS = {0: "R_MIPS_NONE", 2: "R_MIPS_32", 4: "R_MIPS_26", 5: "R_MIPS_HI16",
          6: "R_MIPS_LO16", 7: "R_MIPS_GPREL16", 8: "R_MIPS_LITERAL", 10: "R_MIPS_PC16"}


@dataclass
class Section:
    index: int
    name: str
    type: int
    flags: int
    addr: int
    offset: int
    size: int
    link: int
    info: int
    align: int
    data: bytes


@dataclass
class ElfSymbol:
    name: str
    value: int
    size: int
    type: str
    bind: int        # 0 local, 1 global, 2 weak; MW gives a template instance 13 (one copy kept at link)
    shndx: int


@dataclass
class Reloc:
    offset: int      # within the section the relocation applies to
    type: str
    sym: ElfSymbol
    addend: int | None   # RELA only; REL keeps it in the field


class Elf:
    """A little-endian ELF32 file: an executable (the loader) or an unlinked object."""

    def __init__(self, data: bytes, path: str = "<bytes>"):
        if data[:4] != b"\x7fELF" or data[4] != 1 or data[5] != 1:
            raise ValueError(f"{path}: not a little-endian ELF32 file")
        self.path = path
        self.data = data
        (self.type, self.machine, _v, self.entry, self.phoff, self.shoff, _f, _eh,
         self.phentsize, self.phnum, self.shentsize, self.shnum, self.shstrndx
         ) = struct.unpack_from("<HHIIIIIHHHHHH", data, 16)
        raw = []
        for i in range(self.shnum):
            raw.append(struct.unpack_from("<IIIIIIIIII", data, self.shoff + i * self.shentsize))
        strtab = raw[self.shstrndx] if self.shnum else None
        self.sections: list[Section] = []
        for i, (nm, typ, flg, addr, off, size, link, info, align, _es) in enumerate(raw):
            name = self._cstr(strtab[4] + nm) if strtab else ""
            body = data[off:off + size] if typ != 8 else b""   # SHT_NOBITS has no bytes
            self.sections.append(Section(i, name, typ, flg, addr, off, size, link, info, align, body))
        self.segments = []
        for i in range(self.phnum):
            p_type, p_off, p_vaddr, _pa, p_filesz, p_memsz, p_flags, p_align = struct.unpack_from(
                "<IIIIIIII", data, self.phoff + i * self.phentsize)
            self.segments.append((p_type, p_off, p_vaddr, p_filesz, p_memsz, p_flags))
        self.symbols: list[ElfSymbol] = []
        for sec in self.sections:
            if sec.type == SHT_SYMTAB:
                names = self.sections[sec.link]
                for j in range(sec.size // 16):
                    nm, val, size, info, _other, shndx = struct.unpack_from("<IIIBBH", sec.data, j * 16)
                    # MW pads a static initialiser's name with spaces (`__sinit_zmath_main.cpp    `);
                    # the linker's table, like the demo's, has them stripped
                    self.symbols.append(ElfSymbol(self._cstr(names.offset + nm).rstrip(" "), val, size,
                                                  STT_NAMES.get(info & 0xF, str(info & 0xF)),
                                                  info >> 4, shndx))

    def _cstr(self, off: int) -> str:
        end = self.data.index(b"\0", off)
        return self.data[off:end].decode("latin-1")

    @classmethod
    def read(cls, path: Path | str) -> "Elf":
        return cls(Path(path).read_bytes(), str(path))

    def relocs_for(self, sec_index: int) -> list[Reloc]:
        """Every relocation applied to section `sec_index`."""
        out = []
        for sec in self.sections:
            if sec.type not in (SHT_REL, SHT_RELA) or sec.info != sec_index:
                continue
            step = 8 if sec.type == SHT_REL else 12
            for j in range(sec.size // step):
                off, info = struct.unpack_from("<II", sec.data, j * step)
                addend = struct.unpack_from("<i", sec.data, j * step + 8)[0] if step == 12 else None
                out.append(Reloc(off, R_MIPS.get(info & 0xFF, f"R_MIPS_{info & 0xFF}"),
                                 self.symbols[info >> 8], addend))
        return sorted(out, key=lambda r: r.offset)

    def functions(self) -> list[ElfSymbol]:
        return [s for s in self.symbols if s.type == "FUNC" and s.shndx not in (0, 0xFFF1, 0xFFF2)]

    def function(self, name: str) -> tuple[bytes, list[Reloc], ElfSymbol]:
        """(bytes, relocations rebased to the function's start, symbol) for one FUNC."""
        hits = [s for s in self.functions() if s.name == name]
        if not hits:
            raise KeyError(f"{self.path}: no function {name!r}")
        s = hits[0]
        sec = self.sections[s.shndx]
        body = sec.data[s.value:s.value + s.size]
        rel = [Reloc(r.offset - s.value, r.type, r.sym, r.addend)
               for r in self.relocs_for(s.shndx) if s.value <= r.offset < s.value + s.size]
        return body, rel, s


# --- the image -----------------------------------------------------------------

@dataclass
class MWo3:
    """The 0x40-byte Metrowerks overlay header (docs/storage.md). The image is loaded
    verbatim at `base` and its code starts at base+0x40 -- the overlay's entry is
    its first function (0x4C53C0 for ZSealEtc, where the boot jumps). The text
    range is [base+0x40, base+0x40+text): measured on FTSCore, where it ends exactly
    at the first float constant. (socom-unzipped research/05 says 0x80; it is not.)"""
    kind: int        # 1 OVL1 ftscore, 2 DNAS.BIN, 3 OVL2 zsealetc
    base: int
    text: int        # bytes of code after the header
    data: int        # bytes of data after the code (rodata, data, the __sinit thunks)
    bss: int
    ctors: int       # static-initialiser table start ...
    ctors_end: int   # ... and end; the loader runs it through 0x182840
    name: str

    @classmethod
    def parse(cls, b: bytes) -> "MWo3":
        if b[:4] != b"MWo3":
            raise ValueError("not an MWo3 overlay image")
        kind, base, text, data, bss, c0, c1 = struct.unpack_from("<7I", b, 4)
        name = b[0x20:0x40].split(b"\0")[0].decode("latin-1")
        return cls(kind, base, text, data, bss, c0, c1, name)

    @property
    def text_start(self) -> int:
        return self.base + 0x40

    @property
    def text_end(self) -> int:
        return self.base + 0x40 + self.text


@dataclass
class Segment:
    name: str
    base: int
    data: bytes
    memsz: int
    code: tuple[int, int]         # [start, end) holding R5900 code
    header: MWo3 | None = None
    vu: tuple[int, int] | None = None   # VU1 microcode linked into .text after the R5900 code

    @property
    def end(self) -> int:
        return self.base + len(self.data)


VU_LOWER_NOP, VU_UPPER_NOP = 0x8000033C, 0x000002FF


def vu_tail(data: bytes, base: int, lo: int, hi: int) -> int:
    """Where the VU1 microcode at the end of an overlay's text begins (== hi if none).

    The microprograms are 64-bit lower/upper pairs dense in the two NOPs. Two such
    pairs in one 64-byte block do not occur in R5900 code (`lb zero,828(zero)` and
    `dsra32 zero,zero,11` are both useless), so the first such block marks the tail,
    and the microcode starts at the first non-zero word after the zero padding before
    it. On FTSCore: code ends 0x3D5948, microcode 0x3D5980-0x3DBFF8."""
    w = lambda a: struct.unpack_from("<I", data, a - base)[0]
    a = lo + (-lo % 64)
    while a < hi:
        if sum(1 for q in range(a, min(a + 64, hi) - 4, 8)
               if w(q) == VU_LOWER_NOP or w(q + 4) == VU_UPPER_NOP) >= 2:
            s = a
            while s > lo and w(s - 4) != 0:   # back up to the padding before it
                s -= 4
            return s
        a += 64
    return hi


class Image:
    """MAIN + OVL1 + OVL2, addressed by virtual address."""

    def __init__(self, loader: Path = LOADER, overlays: Path = OVERLAYS, pinned: bool = True):
        self.segments: list[Segment] = []
        raw = loader.read_bytes()
        if pinned:
            _check_pinned(loader, raw)
        elf = Elf(raw, str(loader))
        p_type, off, vaddr, filesz, memsz, _fl = elf.segments[0]
        assert p_type == 1 and vaddr == MAIN_BASE and filesz, "SCUS_972.75's first LOAD is MAIN"
        self.segments.append(Segment("MAIN", vaddr, raw[off:off + filesz], memsz,
                                     (vaddr, vaddr + filesz)))
        for fname, seg, base in (("ftscore.bin", "OVL1", OVL1_BASE), ("zsealetc.bin", "OVL2", OVL2_BASE)):
            p = overlays / fname
            b = p.read_bytes()
            if pinned:
                _check_pinned(p, b)
            h = MWo3.parse(b)
            if h.base != base:
                raise RuntimeError(f"{p}: MWo3 base 0x{h.base:08X}, expected 0x{base:08X}")
            vu = vu_tail(b, base, h.text_start, h.text_end)
            self.segments.append(Segment(seg, base, b, len(b) + h.bss, (h.text_start, vu), h,
                                         (vu, h.text_end) if vu < h.text_end else None))

    def segment(self, addr: int) -> Segment | None:
        for s in self.segments:
            if s.base <= addr < s.end:
                return s
        return None

    def in_memory(self, addr: int) -> bool:
        """Inside a segment's loaded extent, BSS included (no bytes there to read)."""
        return any(s.base <= addr < s.base + s.memsz for s in self.segments)

    def covers(self, addr: int, length: int) -> bool:
        s = self.segment(addr)
        return s is not None and addr + length <= s.end

    def read(self, addr: int, length: int) -> bytes:
        s = self.segment(addr)
        if s is None or addr + length > s.end:
            raise ValueError(f"VA 0x{addr:08X}+{length} is not in the image")
        return s.data[addr - s.base:addr - s.base + length]

    def word(self, addr: int) -> int:
        return struct.unpack("<I", self.read(addr, 4))[0]

    def is_code(self, addr: int) -> bool:
        s = self.segment(addr)
        return s is not None and s.code[0] <= addr < s.code[1]


# --- the symbol map --------------------------------------------------------------

@dataclass
class Symbol:
    name: str
    addr: int
    size: int        # exact byte size: the last instruction's delay slot included, padding not
    note: str = ""

    @property
    def end(self) -> int:
        return self.addr + self.size


_PLACEHOLDER = re.compile(r"^(?:func|data)_[0-9A-F]{8}$")


def is_placeholder(name: str) -> bool:
    return bool(_PLACEHOLDER.match(name))


class SymbolTable:
    """decomp/symbols.txt: one row per function, `0xADDR SIZE name  # note`.

    The map is OURS and is kept by hand once seeded (tools/seed_symbols.py). A
    function not yet named carries the placeholder `func_<VA>`; a name is the
    MANGLED name the source defines, because the checker finds targets by the
    name the compiler emits. SIZE is the exact size (MW pads every function to
    16 bytes; the padding belongs to no one and must be zero)."""

    _LINE = re.compile(r"^(0x[0-9A-Fa-f]+)\s+(0x[0-9A-Fa-f]+|\d+)\s+(\S+)\s*(?:#\s?(.*))?$")

    def __init__(self, path: Path = SYMBOLS, data_path: Path | None = None):
        self.path = path
        self.symbols: list[Symbol] = []
        self._by_name: dict[str, Symbol] = {}
        self._by_addr: dict[int, Symbol] = {}
        # global data (data.txt): names the differ resolves, never walked as functions
        self.data: list[Symbol] = []
        self.data_by_name: dict[str, Symbol] = {}
        self.data_by_addr: dict[int, Symbol] = {}
        self._addrs = []
        if not path.exists():
            return
        for s in self._rows(path):
            self.symbols.append(s)
            if s.name in self._by_name:
                raise ValueError(f"{path}: {s.name} defined twice")
            self._by_name[s.name] = s
            self._by_addr.setdefault(s.addr, s)
        self.symbols.sort(key=lambda s: s.addr)
        self._addrs = [s.addr for s in self.symbols]
        data_path = data_path or (DATA if path == SYMBOLS else None)
        if data_path is not None and data_path.exists():
            for s in self._rows(data_path):
                if s.name in self._by_name or s.name in self.data_by_name:
                    raise ValueError(f"{data_path}: {s.name} named twice (functions and data share one namespace)")
                self.data.append(s)
                self.data_by_name[s.name] = s
                self.data_by_addr.setdefault(s.addr, s)
            self.data.sort(key=lambda s: s.addr)

    def _rows(self, path: Path):
        for n, line in enumerate(path.read_text(errors="replace").splitlines(), 1):
            if not line.startswith("0x"):
                continue
            m = self._LINE.match(line)
            if not m:
                raise ValueError(f"{path}:{n}: unparseable map row: {line!r}")
            yield Symbol(m.group(3), int(m.group(1), 16), int(m.group(2), 0), (m.group(4) or "").strip())

    def address_of(self, name: str) -> int | None:
        """A function's or a global's address, or None."""
        s = self._by_name.get(name) or self.data_by_name.get(name)
        return s.addr if s is not None else None

    def data_containing(self, addr: int) -> Symbol | None:
        import bisect
        keys = [s.addr for s in self.data]
        i = bisect.bisect_right(keys, addr) - 1
        if i >= 0 and self.data[i].addr <= addr < self.data[i].end + (self.data[i].size == 0):
            return self.data[i]
        return None

    def __len__(self) -> int:
        return len(self.symbols)

    def get(self, name: str) -> Symbol:
        try:
            return self._by_name[name]
        except KeyError:
            raise KeyError(f"no symbol {name!r} in {self.path.name}") from None

    def lookup(self, key: str) -> Symbol:
        """A name, or an address (`0x1C5DA0`, `1c5da0`)."""
        if key in self._by_name:
            return self._by_name[key]
        try:
            a = int(key, 16)
        except ValueError:
            raise KeyError(f"no symbol {key!r}") from None
        if a in self._by_addr:
            return self._by_addr[a]
        raise KeyError(f"no symbol at 0x{a:08X}")

    def at_addr(self, addr: int) -> Symbol | None:
        return self._by_addr.get(addr)

    def containing(self, addr: int) -> Symbol | None:
        import bisect
        i = bisect.bisect_right(self._addrs, addr) - 1
        if i >= 0 and self.symbols[i].addr <= addr < self.symbols[i].end:
            return self.symbols[i]
        return None

    def find(self, pattern: str) -> list[Symbol]:
        rx = re.compile(pattern)
        return [s for s in self.symbols if rx.search(s.name)]


# --- disassembly ---------------------------------------------------------------

def tool(suffix: str) -> str:
    ps2dev = os.environ.get("PS2DEV", os.path.expanduser("~/ps2dev/ps2dev"))
    cand = Path(ps2dev) / "ee" / "bin" / f"mips64r5900el-ps2-elf-{suffix}"
    if cand.is_file():
        return str(cand)
    import shutil
    for c in (f"mips64r5900el-ps2-elf-{suffix}", f"mips-linux-gnu-{suffix}"):
        if shutil.which(c):
            return c
    raise RuntimeError(f"no {suffix} for the EE target; set PS2DEV")


_INSN = re.compile(r"^\s*([0-9a-f]+):\s+([0-9a-f]{8})\s+(.*)$")


def disasm(data: bytes, vma: int = 0) -> list[tuple[int, int, str]]:
    """[(address, word, text)] for raw R5900 code, via objdump in binary mode."""
    if not data:
        return []
    import tempfile
    tmp = Path(os.environ.get("SC_TMP") or HERE / "build" / "tmp")   # never /tmp: it fills up here
    tmp.mkdir(parents=True, exist_ok=True)
    with tempfile.NamedTemporaryFile(suffix=".bin", dir=tmp) as f:
        f.write(data)
        f.flush()
        out = subprocess.run(
            # -z: objdump otherwise collapses a run of zero words (nop nop) into "...",
            # silently dropping instructions -- CQuat::Normalize's two hazard nops were lost
            [tool("objdump"), "-D", "-z", "-b", "binary", "-m", "mips:5900", "-EL",
             f"--adjust-vma=0x{vma:x}", f.name],
            capture_output=True, text=True, check=True).stdout
    rows = []
    for line in out.splitlines():
        m = _INSN.match(line)
        if m:
            rows.append((int(m.group(1), 16), int(m.group(2), 16), m.group(3).replace("\t", " ").strip()))
    return rows


# --- small helpers -------------------------------------------------------------

JR_RA = 0x03E00008


def words(b: bytes) -> list[int]:
    return list(struct.unpack(f"<{len(b) // 4}I", b[:len(b) // 4 * 4]))


@lru_cache(maxsize=1)
def image() -> Image:
    return Image()


@lru_cache(maxsize=1)
def symbols() -> SymbolTable:
    return SymbolTable()
