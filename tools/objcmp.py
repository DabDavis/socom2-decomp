#!/usr/bin/env python3
"""
objcmp -- prove a header (or any source) change code-neutral, object for object.

    ./tools/objcmp.py master                     # every hand-written unit, against master's text
    ./tools/objcmp.py HEAD~1 'zseal/|zkit/'      # the units whose path (under src/) matches
    ./tools/objcmp.py master --auto -j 2         # the generated drafts too
    ./tools/objcmp.py master 'zseal/' --by-address   # a placeholder respelled by its row's name is the same

Every hand-written unit (src/, not src/auto unless --auto) that REGEX names is built twice, each as
sccheck builds it (its own directive: toolchain, flags, `as=`, its statics qualified): once from a
scratch copy of src/ and include/ with every file that differs from BASE put back to BASE's text
(files added since BASE taken out), once from a copy of the tree as it is -- both copies at ONE path
(<work>/tree/, BASE's first, then the tree's in its place), so what a compiler writes of the path
it was given is the same on both sides: GCC round 34a, the online lobby's round 22 -- an assert GCC
drops keeps __FILE__ in .rodata, and sase_rfft2g.c's (before its #line) read as a difference with
its source unchanged when BASE was built from a scratch copy and the tree in place (the MSL's round
28, the UI's round 26). Then, unit by unit, the objects are compared:

  - every function: its bytes, and each relocation's offset, type, addend and target;
  - every allocated data section (`.data`, `.rodata`, `.sdata`, `.lit4`, `.bss`, ...): its size,
    its bytes and its relocations, so a constant, a string or a table that moved is a difference;
  - every data object's name, section, offset and size.

A target is named as the scorer resolves it, never by a number a header can shift: Metrowerks'
anonymous `@N` objects (a parser-wide counter: a header's extra declarations renumber them) by
their CONTENT (GCC round 36a, the game states' round 43: `@.data"name"`, `@.data:00000000`, the k-th
repeat `#k`), an unreferenced one left out with its section -- a header's literal bound to a const
reference (ai.h's ForEachCell) puts two unreferenced zeros in every unit including it, used or not,
and by rank every later object shifted; MW's linker drops a data section nothing references, so it
cannot reach the image (`--anon-by-rank`: rounds 27a-35a's rule, each by its rank) -- a
function-local static's `name$N` as `name$`, a GNU object's section symbol (`.rodata`+N, `.text`+N)
by its section's name -- the offset is in the field.

--by-address (GCC round 29a; the game states' round-34 objcmp_addr.py and the kit's round-33
byaddr.py folded in): every lane respells placeholders -- `func_001C5EC0()` becomes the row's real
name once the map has it -- and plain objcmp reports each renamed call as a difference. With the
flag, every name a map resolves is compared as its map address, the scorer's own rule
(scdiff._sym_address): a function's or a global's row -- from code at an address, its own
overlay's copy (`ovl-copy:`) -- or a placeholder by the address in its name. That covers each
relocation's target, each function's own name and each data object's. The tree's map is asked
first, then BASE's (a name the map has since renamed); a name neither resolves (a static, `@N`, a
section) is compared by name, as without the flag. Resolution is a function of the name alone, so
the flag can only equate more: a unit plain objcmp calls equal stays equal. A unit equal only by
address is listed with the names respelled (`SAME-BY-ADDRESS`).

A data reference is compared as the address it forms, symbol plus addend (GCC round 35a, the HUD's
round 54: hud_missioncams.cpp's `data_0043668C` respelled `theMission+0x1AC`, PlayCamera 0x228EE0,
printed DIFF while the scorer matched it). Both compilers write REL relocations, the addend in the
instruction's field, so a respelling changes the symbol AND the field; with --by-address each data
relocation whose symbol the map resolves is read as the scorer reads it and its field masked out of
the bytes: a %hi/%lo pair (R_MIPS_HI16/LO16) as symbol + (hi << 16) + (short)lo -- a Metrowerks %lo
paired with the latest %hi of its symbol, a GNU one by the relocation table, a %hi as its first
%lo's address -- a $gp-relative one (R_MIPS_GPREL16) as symbol + (short)field (a GNU local's plus
gp0), a 32-bit word (R_MIPS_32: a table's pointer, in code or data) as symbol + the word. A call
(R_MIPS_26) stays its symbol's, a GNU literal pool entry (R_MIPS_LITERAL) its section's, and a pair
missing its other half is compared as before. So `data_001D0BB0[1]` respelled as the row at 0x1D0BB8
is the same, `data_001D0BB0[2]` is not; the sum is a function of the name and the field, so the flag
still only equates more. --symbols-only keeps round 29a's rule (a symbol equated, the field compared).

THE TREE PATH'S LENGTH (GCC round 38a, the world's round 26: a first run in the default build/objcmp
printed `DIFF ai/ai_mapwalk.cpp` -- MAP_DIRCOUNT::GetBestDir 0x5350C0, byte-exact -- where a fresh
--work showed none; nothing persists between runs, the two directories' paths were of different
LENGTHS). Metrowerks' code can depend on how long the paths it is given are, never on their
characters: GetBestDir compiles a word short -- the heading's direction masked straight into s0
(`andi s0,v0,0xff`) where the target masks it into a0 and copies v0 to s0 -- at 6 of the 122 tree
lengths from 98 to 219 characters for the tree's text (108, 109, 125, 126, 180, 202) and at 5 others
for d14aeea6^1's (89 -- a worktree's build/objcmp/tree --, 110, 137, 173, 215): the heap's layout,
which the path strings shift, reaching a choice. The object path's length, the environment's size,
the working directory, the toolchain's own path and repeated builds change nothing. One function of
the 658 hand-written units does it at 1 of 5 lengths, none at 98, 160 or 167
(build/scratch/gcc/r38/census.py), so BASE and the tree, built at ONE path, differed at the default
directory's length alone. So both sides are built
in a directory whose whole path is a FIXED length wherever the work directory lies (`side_root`:
`<work>/tree___...`, 160 characters by default), and a unit that differs there is built again, both
sides, at a second length (167): one equal there is listed `PATH-DEPENDENT` -- the same source, the
compiler's layout -- and not counted as differing. `--path-lengths A,B` picks the two (`0`: the
unpadded `<work>/tree`, one path, the old behaviour); past ~200 characters Metrowerks fails on the
tree's deepest headers (ai_mapwalk.cpp at a 220-character tree: `zentity.h:340`, then a
"declaration syntax error"), so a longer length is refused. `--tree REV` builds the tree's side from a
revision too (two fixed texts: the canary's). Each side's stems (scdiff.shared_stems, cached per
path) are read again for it: both sides share the path, and BASE's set stood for the tree's.

Prints each unit that differs and what in it, then `N units compared, M differ`; rc 1 when any
does (a unit that builds on one side only differs). The SEALs' round-26 harness
(build/scratch/seal-r26/objcmp.py), adopted by the GCC lane's round 27a: include/ is now the base
revision's too (the harness built both sides against the tree's include/, so a change there could
never show), data is compared, and the scratch tree is the caller's (--work).
"""

from __future__ import annotations

import argparse
import hashlib
import re
import shutil
import struct
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

HERE = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(HERE))
import scdecomp as E
import scdiff as D
import sccheck

SHF_ALLOC, SHF_EXECINSTR = 0x2, 0x4
SHT_PROGBITS, SHT_NOBITS = 1, 8
# an address in each overlay, for a data section's references (scdecomp.SymbolTable.address_in
# reads only the overlay of `near`)
OVERLAY_AT = {"MAIN": 0x100000, "OVL1": 0x1E7000, "OVL2": 0x4C5380}


def git(*args: str) -> subprocess.CompletedProcess:
    return subprocess.run(["git", *args], capture_output=True, cwd=HERE)


def text_dir(base: str) -> Path | None:
    """`base` as a directory holding src/ (and include/) -- a side's text as it lies, the canary's
    fixtures (GCC round 38a) -- or None when it is a revision."""
    d = Path(base)
    if not (d / "src").is_dir():
        return None
    if git("rev-parse", "--verify", "--quiet", f"{base}^{{commit}}").returncode == 0:
        return None                      # a revision that happens to name a directory too
    return d.resolve()


def base_tree(base: str, work: Path) -> list[str]:
    """A copy of src/ (drafts left out) and include/ at `work` with every file that differs from
    `base` put back to its text there; the files reverted (paths relative to decomp/). A file renamed
    since BASE is its two paths (`--no-renames`: by rename detection `git diff --name-only` named
    the new path alone, and BASE's side lacked the old one -- a header renamed broke every BASE unit
    including it, GCC round 38a). `base` may be a directory (text_dir): its text, as it lies."""
    if work.exists():
        shutil.rmtree(work)
    d = text_dir(base)
    if d is not None:
        shutil.copytree(d / "src", work / "src", ignore=shutil.ignore_patterns("auto", "*.o"))
        if (d / "include").is_dir():
            shutil.copytree(d / "include", work / "include")
        else:
            (work / "include").mkdir(parents=True)
        return []
    shutil.copytree(HERE / "src", work / "src", ignore=shutil.ignore_patterns("auto", "*.o"))
    shutil.copytree(HERE / "include", work / "include")
    r = git("diff", "--name-only", "--no-renames", "--relative", base, "--", "src", "include")
    if r.returncode:
        raise SystemExit(f"git diff {base}: {r.stderr.decode().strip()}")
    changed = r.stdout.decode().split()
    added = git("ls-files", "--others", "--exclude-standard", "--", "src", "include").stdout.decode().split()
    for rel in changed + added:
        if rel.startswith("src/auto/"):
            continue
        dst = work / rel
        shown = git("show", f"{base}:./{rel}") if rel in changed else None
        if shown is not None and shown.returncode == 0:
            dst.parent.mkdir(parents=True, exist_ok=True)
            dst.write_bytes(shown.stdout)
        elif dst.exists():
            dst.unlink()                 # a file BASE does not have
    return [r for r in changed + added if not r.startswith("src/auto/")]


def tree_copy(dst: Path, auto: bool = False) -> None:
    """The tree as it is -- src/ (the drafts only with `auto`) and include/ -- copied to `dst`."""
    if dst.exists():
        shutil.rmtree(dst)
    shutil.copytree(HERE / "src", dst / "src",
                    ignore=shutil.ignore_patterns("*.o") if auto else shutil.ignore_patterns("auto", "*.o"))
    shutil.copytree(HERE / "include", dst / "include")


# The length of the path both sides are built at (GCC round 38a): Metrowerks' code can depend on
# it (the docstring's "THE TREE PATH'S LENGTH"), so it is fixed wherever the work directory lies; a
# unit differing at the first is built again at the second.
SIDE_LENGTHS = (160, 167)
MAX_SIDE_LENGTH = 200       # a 220-character tree breaks Metrowerks on the tree's deepest headers


def side_root(work: Path, length: int) -> Path:
    """The directory a side's src/ and include/ are laid out in: `<work>/tree` with `_` appended to
    its name until the whole path is `length` characters (0: `<work>/tree` as it is). A work path too
    long for `length`, or a length past MAX_SIDE_LENGTH, is refused rather than built elsewhere."""
    base = work / "tree"
    if length <= 0:
        return base
    if length > MAX_SIDE_LENGTH:
        raise SystemExit(f"objcmp: a {length}-character tree path is past {MAX_SIDE_LENGTH}: Metrowerks "
                         "fails on the deepest headers there")
    pad = length - len(str(base))
    if pad < 0:
        raise SystemExit(f"objcmp: --work {work} is too long for a {length}-character tree path "
                         f"({len(str(base))} before padding): give a shorter --work or --path-lengths")
    return work / ("tree" + "_" * pad)


def build_side(side: Path, rels: list[Path], objdir: Path, jobs: int = 2) -> dict:
    """{rel: (object or None, why not)}: every unit of `rels` that `side`/src holds, built as sccheck
    builds it against `side`/include, into `objdir`. The stems `side`/src shares (scdiff.shared_stems,
    which qualify its statics) are read again first: BASE's copy and the tree's lie at one path, and
    the set cached for BASE's stood for the tree's (GCC round 38a)."""
    D._SHARED_STEMS.pop((side / "src").resolve(), None)

    def one(rel: Path):
        p = side / "src" / rel
        if not p.exists():
            return None, "absent"
        _, o, err = sccheck.build_one(p, objdir, root=side / "src", include=side / "include")
        return o, ((err.strip().splitlines() or ["?"])[-1][:70] if o is None else "")
    with ThreadPoolExecutor(jobs) as ex:
        return dict(zip(rels, ex.map(one, rels)))


def base_map(base: str, work: Path) -> E.SymbolTable | None:
    """BASE's map (its symbols.txt and data.txt), for the names the tree's map no longer holds;
    None when BASE has none that parses."""
    work.mkdir(parents=True, exist_ok=True)
    d = text_dir(base)
    if d is not None:                    # a directory: its own map, when it holds one
        try:
            return E.SymbolTable(d / "symbols.txt", d / "data.txt") if (d / "symbols.txt").is_file() else None
        except ValueError:
            return None
    paths = []
    for name in ("symbols.txt", "data.txt"):
        r = git("show", f"{base}:./{name}")
        if r.returncode:
            return None
        (work / name).write_bytes(r.stdout)
        paths.append(work / name)
    try:
        return E.SymbolTable(*paths)
    except ValueError:
        return None


def resolver(*tables: E.SymbolTable | None):
    """The scorer's name -> address rule (scdiff._sym_address) over each map in turn: a function's
    or a global's row (from code at `near`, its own overlay's copy), else a placeholder's address;
    None when no map resolves the name."""
    tables = [t for t in tables if t is not None]
    cache: dict[tuple[str, int | None], int | None] = {}

    def resolve(name: str, near: int | None = None) -> int | None:
        if not name or name.startswith("section "):
            return None
        key = (name, None if near is None else E.SymbolTable.overlay_of(near))
        if key not in cache:
            cache[key] = next((a for a in (D._sym_address(t, name, near) for t in tables)
                               if a is not None), None)
        return cache[key]
    return resolve


SUMMED = ("R_MIPS_HI16", "R_MIPS_LO16", "R_MIPS_GPREL16", "R_MIPS_32")


def sum_addresses(elf: E.Elf, rels: list[E.Reloc], body: bytes, address, fsym: E.ElfSymbol | None = None
                  ) -> dict[int, tuple[int, int]]:
    """{index into `rels`: (the address the relocation forms, its addend)} for each data relocation
    whose symbol `address(reloc)` resolves, read as the scorer reads it (scdiff.reloc_check): both
    compilers write REL relocations, the addend in the field -- a %hi/%lo pair (R_MIPS_HI16 and
    R_MIPS_LO16) is the symbol plus the pair's (hi << 16) + (short)lo, a Metrowerks %lo paired with
    the latest %hi of its symbol before it, a GNU one by the relocation table (scdiff.gnu_hi_partners,
    `fsym` its function), a %hi standing for the address of its first %lo; a $gp-relative one
    (R_MIPS_GPREL16) the symbol plus the (short) field, a GNU local's less the object's gp0 as the
    linker adds it back; a 32-bit word (R_MIPS_32, a table's pointer) the symbol plus the word. A
    call (R_MIPS_26) is the symbol's, a literal pool entry (R_MIPS_LITERAL) its section's: neither
    is summed. A pair whose other half is missing is left out."""
    def field(off: int, mask: int = 0xFFFF) -> int:
        return struct.unpack_from("<I", body, off)[0] & mask if off + 4 <= len(body) else 0
    gnu = D.is_gnu(elf)
    gp0 = next((struct.unpack_from("<i", s.data, 20)[0] for s in elf.sections
                if s.name == ".reginfo" and len(s.data) >= 24), 0) if gnu else 0
    partner = D.gnu_hi_partners(elf, fsym, body) if gnu and fsym is not None else {}
    by_off = {r.offset: i for i, r in enumerate(rels) if r.type == "R_MIPS_HI16"}
    out: dict[int, tuple[int, int]] = {}
    pending: dict[str, int] = {}               # MW: a symbol's latest %hi, by its index
    for i, r in enumerate(rels):
        if r.type not in SUMMED or r.addend is not None:
            continue
        if r.type == "R_MIPS_HI16":
            pending[r.sym.name] = i
            continue
        base = address(r)
        if r.type == "R_MIPS_LO16":
            h = by_off.get(partner[r.offset]) if r.offset in partner else (
                None if gnu else pending.get(r.sym.name))
            if h is None or base is None:
                continue
            add = (field(rels[h].offset) << 16) + D._s16(field(r.offset))
            out[i] = ((base + add) & 0xFFFFFFFF, add)
            out.setdefault(h, out[i])          # a %hi: its first %lo's address
        elif base is not None and r.type == "R_MIPS_GPREL16":
            add = D._s16(field(r.offset)) + (gp0 if gnu and r.sym.bind == 0 else 0)
            out[i] = ((base + add) & 0xFFFFFFFF, add)
        elif base is not None:                 # R_MIPS_32
            add = field(r.offset, 0xFFFFFFFF)
            out[i] = ((base + add) & 0xFFFFFFFF, add)
    return out


ANON = re.compile(r"@\d+")


def _spell(sec: str, data: bytes | None, size: int) -> str:
    """An anonymous object's content as a name: a C string's text, a short object's bytes, a long
    one's size and digest (zeroed data: its size)."""
    if data is None:
        return f"@{sec}:{size}"
    text = data[:-1]
    if data.endswith(b"\0") and text and all(32 <= c < 127 for c in text):
        t = text.decode()
        return f'@{sec}"{t[:32]}"' + (f"+{hashlib.sha1(data).hexdigest()[:8]}" if len(t) > 32 else "")
    if size <= 16:
        return f"@{sec}:{data.hex()}"
    return f"@{sec}:{size}:{hashlib.sha1(data).hexdigest()[:10]}"


def anon_by_content(elf: E.Elf) -> tuple[dict[str, str], set[int], set[str]]:
    """GCC round 36a (the game states' round 43): Metrowerks' anonymous `@N` objects named by what
    they hold, not by N or its rank -- N is a parser-wide counter, and a header's unreferenced literal
    (ai.h's ForEachCell binds a 0 to a const reference: two 4-byte zeros in every unit including it,
    used or not) shifts every later rank while the code is the same. Returns ({the object's symbol
    name: its name by content}, the sections left out, the objects left out). An anonymous object nothing reaches -- no
    relocation of a function, a named object's section or a section reached from them -- is left out
    with its section: MW gives every data object a section of its own and the linker drops one nothing
    references (vehicle_rdr.cpp's three unreferenced zeros, sections before its referenced 0 and 4, are
    not in the image: those sit at 0x64F9A8/0x64F9B0 right after another unit's datum), so it cannot
    reach the image whatever it holds. The rest are named by section and content (`@.data"name"`,
    `@.data:00000000`, a long one by digest; one holding relocations by their targets too), the k-th
    repeat of one name in N's order `#k` -- so a relocation reads the object it names, aligned by
    content, and an object whose content changed is another name."""
    secs = elf.sections
    syms = [s for s in elf.symbols if 0 < s.shndx < len(secs)]
    anon = sorted((s for s in syms if s.type == "OBJECT" and ANON.fullmatch(s.name or "")),
                  key=lambda s: int(s.name[1:]))
    if not anon:
        return {}, set(), set()
    anon_names = {s.name for s in anon}
    alloc = {sec.index for sec in secs if sec.flags & SHF_ALLOC}
    holds: dict[int, set[str]] = {}
    for s in syms:
        if s.type in ("OBJECT", "FUNC"):
            holds.setdefault(s.shndx, set()).add(s.name)
    edges: dict[int, list[tuple[str, int]]] = {}       # section -> [(anon name or '', section)]
    relocs: dict[int, list] = {}
    for i in alloc:
        rels = relocs[i] = elf.relocs_for(i)
        for r in rels:
            if r.sym.name in anon_names:
                edges.setdefault(i, []).append((r.sym.name, r.sym.shndx))
            elif r.sym.type == "SECTION" and 0 < r.sym.shndx < len(secs):
                edges.setdefault(i, []).append(("", r.sym.shndx))
    roots = {i for i in alloc if not holds.get(i) or holds[i] - anon_names}
    seen_secs, reached = set(roots), set()
    todo = list(roots)
    while todo:
        i = todo.pop()
        for name, j in edges.get(i, ()):
            if name:
                reached.add(name)
            if j not in seen_secs:
                seen_secs.add(j)
                todo.append(j)
    reached |= {s.name for s in anon if s.shndx in roots}
    dropped = {i for i in alloc if i not in seen_secs}
    raw = {s.name: _spell(secs[s.shndx].name, None if secs[s.shndx].type == SHT_NOBITS
                          else secs[s.shndx].data[s.value:s.value + s.size], s.size) for s in anon}
    out: dict[str, str] = {}
    count: dict[str, int] = {}
    for s in anon:
        if s.name not in reached:
            continue
        name = raw[s.name]
        inner = [(r.offset - s.value, r.type, raw.get(r.sym.name, r.sym.name)) for r in relocs.get(s.shndx, ())
                 if s.value <= r.offset < s.value + max(s.size, 1)]
        if inner:
            name += "&" + hashlib.sha1(repr(inner).encode()).hexdigest()[:8]
        k = count[name] = count.get(name, -1) + 1
        out[s.name] = name + (f"#{k}" if k else "")
    return out, dropped, anon_names - reached


def summary(obj: Path, resolve=None, spelled: dict[str, list[str]] | None = None,
            sums: bool = False, anon_rank: bool = False) -> dict[str, object]:
    """What a unit's object holds, keyed so that two builds of one source compare equal. With
    `resolve` (--by-address: name, near -> map address or None) a name the map resolves -- a
    function's own, a relocation's target, a data object's -- is `@0xADDR`; with `sums` too (GCC
    round 35a) a data relocation the map resolves is the address it forms, symbol plus addend
    (sum_addresses), its field masked out of the bytes -- `data_0043668C` respelled `theMission+0x1AC`
    is the same reference. `spelled`, when given, receives each key's names as the object spells
    them, in order (a summed one with its addend). Metrowerks' anonymous objects are named by content
    and an unreferenced one left out (anon_by_content, GCC round 36a); `anon_rank` keeps rounds
    27a-35a's rule, each named by its rank among the object's `@N`."""
    elf = E.Elf.read(obj)
    if anon_rank:
        anon = sorted({int(s.name[1:]) for s in elf.symbols if ANON.fullmatch(s.name or "")})
        rank, dropped, dead = {f"@{n}": f"@#{i}" for i, n in enumerate(anon)}, set(), set()
    else:
        rank, dropped, dead = anon_by_content(elf)
        other = sorted({int(s.name[1:]) for s in elf.symbols if ANON.fullmatch(s.name or "")
                        and s.name not in rank and s.name not in dead})
        rank.update({f"@{n}": f"@#{i}" for i, n in enumerate(other)})   # an `@N` no object: by rank

    def target(sym: E.ElfSymbol) -> str:
        if sym.type == "SECTION" and 0 < sym.shndx < len(elf.sections):
            return "section " + elf.sections[sym.shndx].name
        name = rank.get(sym.name, sym.name)
        return re.sub(r"\$\d+$", "$", name)

    def at(name: str, near: int | None = None) -> int | None:
        return resolve(name, near) if resolve else None

    def relocs(rels, near: int | None, names: list[str], body: bytes | None = None,
               fsym: E.ElfSymbol | None = None) -> tuple[tuple, bytes | None]:
        """(the relocations' keys, `body` with each summed relocation's field masked out)"""
        summed = (sum_addresses(elf, rels, body, lambda r: at(target(r.sym), near), fsym)
                  if sums and resolve and body is not None else {})
        out = []
        for i, r in enumerate(rels):
            name = target(r.sym)
            if i in summed:
                addr, add = summed[i]
                names.append(name + (f"+{add:#x}" if add > 0 else f"-{-add:#x}" if add < 0 else ""))
                out.append((r.offset, r.type, f"@{addr:#x}", r.addend))
                continue
            names.append(name)
            a = at(name, near)
            out.append((r.offset, r.type, name if a is None else f"@{a:#x}", r.addend))
        if summed:
            body = D.mask(body, [rels[i] for i in summed])
        return tuple(out), body

    entries: list[tuple[str, str, object, list[str]]] = []     # (key by address, key by name, value, names)
    funcs = []
    for f in elf.functions():
        own = target(f)
        funcs.append((f, own, at(own)))
    home = None
    if resolve:
        count: dict[str, int] = {}
        for _, _, a in funcs:
            if a is not None:
                o = E.SymbolTable.overlay_of(a)
                count[o] = count.get(o, 0) + 1
        if count:
            low = E.SymbolTable.overlay_of(min(a for _, _, a in funcs if a is not None))
            home = OVERLAY_AT[max(count, key=lambda o: (count[o], o == low))]
    for f, own, a in funcs:
        code, rels, fsym = elf.function(f.name)
        names = [own]
        keys, code = relocs(rels, a if a is not None else home, names, code, fsym)
        entries.append((f"function @{a:#x}" if a is not None else "", f"function {own}", (code, keys), names))
    seen: dict[str, int] = {}
    for sec in elf.sections:
        if not sec.flags & SHF_ALLOC or sec.flags & SHF_EXECINSTR or sec.type not in (SHT_PROGBITS, SHT_NOBITS):
            continue
        if sec.index in dropped:        # an unreferenced anonymous object's: the linker drops it
            continue
        n = seen[sec.name] = seen.get(sec.name, 0) + 1
        body = None if sec.type == SHT_NOBITS else sec.data
        names: list[str] = []
        key = f"section {sec.name}#{n}"
        keys, body = relocs(elf.relocs_for(sec.index), home, names, body)
        entries.append(("", key, (sec.size, body, keys), names))
    for s in elf.symbols:
        if s.type == "OBJECT" and 0 < s.shndx < len(elf.sections) and s.name not in dead:
            own = target(s)
            a = at(own, home)
            entries.append((f"object @{a:#x}" if a is not None else "", f"object {own}",
                            (elf.sections[s.shndx].name, s.value, s.size), [own]))
    # a key by address stands only where it is unique (two names resolving to one row keep theirs)
    taken: dict[str, int] = {}
    for k, _, _, _ in entries:
        if k:
            taken[k] = taken.get(k, 0) + 1
    out: dict[str, object] = {}
    for k, named, val, names in entries:
        key = k if k and taken[k] == 1 else named
        out[key] = val
        if spelled is not None:
            spelled[key] = names
    return out


def respellings(sa: dict, na: dict, nb: dict) -> list[tuple[str, str]]:
    """The names two equal by-address summaries spell differently, (BASE's, the tree's), each once."""
    pairs = []
    for k in sa:
        for x, y in zip(na.get(k, ()), nb.get(k, ())):
            if x != y and (x, y) not in pairs:
                pairs.append((x, y))
    return pairs


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("base", help="the revision to compare against (master, HEAD~1, a hash), or a directory holding src/ and include/")
    ap.add_argument("files", nargs="?", help="regex over unit paths relative to src/ (default: every unit)")
    ap.add_argument("-j", "--jobs", type=int, default=2)
    ap.add_argument("--auto", action="store_true", help="the generated drafts too (src/auto)")
    ap.add_argument("--by-address", action="store_true",
                    help="a name the map resolves is compared as its map address (a respelled placeholder "
                         "is the same), a data reference as symbol + addend (data_0043668C as "
                         "theMission+0x1AC); a name no row resolves is compared by name")
    ap.add_argument("--symbols-only", action="store_true",
                    help="with --by-address, equate symbols only (round 29a's rule: an addend is compared "
                         "in its field)")
    ap.add_argument("--anon-by-rank", action="store_true",
                    help="Metrowerks' anonymous @N objects by their rank in the object (rounds 27a-35a's rule: "
                         "an unreferenced one inserted shifts every later rank), not by content")
    ap.add_argument("--work", type=Path, default=HERE / "build" / "objcmp",
                    help="the scratch tree and objects (one per concurrent user)")
    ap.add_argument("--path-lengths", default=",".join(map(str, SIDE_LENGTHS)),
                    help="the tree path's length both sides are built at, and the second length a unit "
                         "differing there is built at again (GCC round 38a: Metrowerks' code can depend on "
                         "the paths' length); `0`: the unpadded <work>/tree, one path (the old behaviour)")
    ap.add_argument("--tree", metavar="REV",
                    help="build the tree's side from REV's text (a revision, or a directory holding src/ and include/), not from the working tree")
    a = ap.parse_args(argv)
    work = a.work.resolve()
    lengths = [int(x) for x in a.path_lengths.split(",") if x.strip()] or [0]
    if lengths[0] <= 0:
        lengths = [0]
    sides = [side_root(work, n) for n in lengths[:2]]
    side = sides[0]                     # BASE's copy, then the tree's: both sides built at one path
    reverted = base_tree(a.base, side)
    print(f"against {a.base}: {' '.join(reverted) or '(no file differs)'}"
          + (f" (the tree's side: {a.tree}'s text)" if a.tree else ""), flush=True)
    resolve = None
    if a.by_address:
        bm = base_map(a.base, work / "base-map")
        tm = (base_map(a.tree, work / "tree-map") if a.tree else None) or E.symbols()
        resolve = resolver(tm, bm)
        print(f"by address: the tree's map, then {a.base}'s" if bm else
              f"by address: the tree's map ({a.base} has none that parses)", flush=True)
    rx = re.compile(a.files) if a.files else None
    src_now, src_base = HERE / "src", side / "src"
    keep = lambda rel: (a.auto or rel.parts[:1] != ("auto",)) and (not rx or rx.search(str(rel)))
    if a.tree and text_dir(a.tree) is not None:
        now_rels = {p.relative_to(text_dir(a.tree) / "src") for p in sccheck.sources(text_dir(a.tree) / "src")}
    elif a.tree:
        listed = git("ls-tree", "-r", "--name-only", a.tree, "--", "src")
        if listed.returncode:
            raise SystemExit(f"git ls-tree {a.tree}: {listed.stderr.decode().strip()}")
        now_rels = {Path(r).relative_to("src") for r in listed.stdout.decode().split()
                    if Path(r).suffix in sccheck.SUFFIXES}
    else:
        now_rels = {p.relative_to(src_now) for p in sccheck.sources(src_now)}
    rels = sorted(now_rels | {p.relative_to(src_base) for p in sccheck.sources(src_base)})
    rels = [r for r in rels if keep(r)]

    def lay_out(at: Path, which: str) -> None:
        """BASE's text (`base`) or the tree's (`now`: the working tree, or --tree's revision) at `at`."""
        if which == "base":
            if at != side:
                base_tree(a.base, at)
        elif a.tree:
            base_tree(a.tree, at)
        else:
            tree_copy(at, a.auto)       # the tree as it is, in BASE's place
        if a.auto and (which == "base" or a.tree) and (src_now / "auto").is_dir():
            shutil.copytree(src_now / "auto", at / "src" / "auto", dirs_exist_ok=True)

    def compare(rel: Path, built_base: dict, built_now: dict):
        """(what differs or None, the names respelled, whether both sides built)"""
        (oa, ea), (ob, eb) = built_base[rel], built_now[rel]
        if oa is None or ob is None:
            return (f"builds on one side only: base {ea or 'ok'!r}, now {eb or 'ok'!r}"
                    if (oa is None) != (ob is None) else None), [], False
        na: dict[str, list[str]] = {}
        nb: dict[str, list[str]] = {}
        sums = bool(resolve) and not a.symbols_only
        sa, sb = summary(oa, resolve, na, sums, a.anon_by_rank), summary(ob, resolve, nb, sums, a.anon_by_rank)
        diffs = [k for k in sorted(set(sa) | set(sb)) if sa.get(k) != sb.get(k)]
        if diffs:
            return "; ".join(diffs[:6]) + (f" (+{len(diffs) - 6})" if len(diffs) > 6 else ""), [], True
        return None, respellings(sa, na, nb) if resolve else [], True

    lay_out(side, "base")
    built_base = build_side(side, rels, work / "obj-base", a.jobs)
    lay_out(side, "now")
    built_now = build_side(side, rels, work / "obj-now", a.jobs)
    with ThreadPoolExecutor(a.jobs) as ex:
        results = dict(zip(rels, ex.map(lambda r: compare(r, built_base, built_now), rels)))
    # a unit differing at the first length is built again, both sides, at the second: equal there,
    # the difference was the compiler's layout (GetBestDir's), not the source's
    again = [r for r, (d, _, both) in results.items() if d and both]
    path_dep: set[Path] = set()
    if again and len(sides) > 1:
        lay_out(sides[1], "base")
        b2 = build_side(sides[1], again, work / "obj-base2", a.jobs)
        lay_out(sides[1], "now")
        n2 = build_side(sides[1], again, work / "obj-now2", a.jobs)
        path_dep = {r for r in again if compare(r, b2, n2)[::2] == (None, True)}

    bad = respelled = 0
    for rel in rels:
        d, pairs, _ = results[rel]
        if rel in path_dep:
            print(f"PATH-DEPENDENT {rel}: {d} at a {lengths[0]}-character tree path, the same at "
                  f"{lengths[1]} (the compiler's layout, not the source)", flush=True)
        elif d:
            bad += 1
            print(f"DIFF {rel}: {d}", flush=True)
        elif pairs:
            respelled += 1
            shown = ", ".join(f"{x} -> {y}" for x, y in pairs[:4])
            print(f"SAME-BY-ADDRESS {rel}: {len(pairs)} name(s) respelled: {shown}"
                  + (f" (+{len(pairs) - 4})" if len(pairs) > 4 else ""), flush=True)
    print(f"{len(rels)} units compared, {bad} differ"
          + (f", {respelled} the same only by address" if resolve else "")
          + (f", {len(path_dep)} path-dependent (the same at a {lengths[1]}-character tree path)"
             if path_dep else ""))
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
