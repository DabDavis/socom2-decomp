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
(files added since BASE taken out), once from the tree as it is. Then, unit by unit, the objects
are compared:

  - every function: its bytes, and each relocation's offset, type, addend and target;
  - every allocated data section (`.data`, `.rodata`, `.sdata`, `.lit4`, `.bss`, ...): its size,
    its bytes and its relocations, so a constant, a string or a table that moved is a difference;
  - every data object's name, section, offset and size.

A target is named as the scorer resolves it, never by a number a header can shift: Metrowerks'
anonymous `@N` objects (a parser-wide counter: a header's extra declarations renumber them) by
their rank within the object, a function-local static's `name$N` as `name$`, a GNU object's
section symbol (`.rodata`+N, `.text`+N) by its section's name -- the offset is in the field.

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
address is listed with the names respelled (`SAME-BY-ADDRESS`). Both compilers write REL
relocations, the offset in the instruction's field: `data_001D0BB0[1]` respelled as the row at
0x1D0BB8 still differs (its field holds 8), as the scorer would accept -- a symbol is equated, not
a sum.

Prints each unit that differs and what in it, then `N units compared, M differ`; rc 1 when any
does (a unit that builds on one side only differs). The SEALs' round-26 harness
(build/scratch/seal-r26/objcmp.py), adopted by the GCC lane's round 27a: include/ is now the base
revision's too (the harness built both sides against the tree's include/, so a change there could
never show), data is compared, and the scratch tree is the caller's (--work).
"""

from __future__ import annotations

import argparse
import re
import shutil
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


def base_tree(base: str, work: Path) -> list[str]:
    """A copy of src/ (drafts left out) and include/ at `work` with every file that differs from
    `base` put back to its text there; the files reverted (paths relative to decomp/)."""
    if work.exists():
        shutil.rmtree(work)
    shutil.copytree(HERE / "src", work / "src", ignore=shutil.ignore_patterns("auto", "*.o"))
    shutil.copytree(HERE / "include", work / "include")
    r = git("diff", "--name-only", "--relative", base, "--", "src", "include")
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


def base_map(base: str, work: Path) -> E.SymbolTable | None:
    """BASE's map (its symbols.txt and data.txt), for the names the tree's map no longer holds;
    None when BASE has none that parses."""
    work.mkdir(parents=True, exist_ok=True)
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


def summary(obj: Path, resolve=None, spelled: dict[str, list[str]] | None = None) -> dict[str, object]:
    """What a unit's object holds, keyed so that two builds of one source compare equal. With
    `resolve` (--by-address: name, near -> map address or None) a name the map resolves -- a
    function's own, a relocation's target, a data object's -- is `@0xADDR`; `spelled`, when given,
    receives each key's names as the object spells them, in order."""
    elf = E.Elf.read(obj)
    anon = sorted({int(s.name[1:]) for s in elf.symbols if re.fullmatch(r"@\d+", s.name or "")})
    rank = {f"@{n}": f"@#{i}" for i, n in enumerate(anon)}

    def target(sym: E.ElfSymbol) -> str:
        if sym.type == "SECTION" and 0 < sym.shndx < len(elf.sections):
            return "section " + elf.sections[sym.shndx].name
        name = rank.get(sym.name, sym.name)
        return re.sub(r"\$\d+$", "$", name)

    def at(name: str, near: int | None = None) -> int | None:
        return resolve(name, near) if resolve else None

    def relocs(rels, near: int | None, names: list[str]) -> tuple:
        out = []
        for r in rels:
            name = target(r.sym)
            names.append(name)
            a = at(name, near)
            out.append((r.offset, r.type, name if a is None else f"@{a:#x}", r.addend))
        return tuple(out)

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
        code, rels, _ = elf.function(f.name)
        names = [own]
        val = (code, relocs(rels, a if a is not None else home, names))
        entries.append((f"function @{a:#x}" if a is not None else "", f"function {own}", val, names))
    seen: dict[str, int] = {}
    for sec in elf.sections:
        if not sec.flags & SHF_ALLOC or sec.flags & SHF_EXECINSTR or sec.type not in (SHT_PROGBITS, SHT_NOBITS):
            continue
        n = seen[sec.name] = seen.get(sec.name, 0) + 1
        body = None if sec.type == SHT_NOBITS else sec.data
        names: list[str] = []
        key = f"section {sec.name}#{n}"
        entries.append(("", key, (sec.size, body, relocs(elf.relocs_for(sec.index), home, names)), names))
    for s in elf.symbols:
        if s.type == "OBJECT" and 0 < s.shndx < len(elf.sections):
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
    ap.add_argument("base", help="the revision to compare against (master, HEAD~1, a hash)")
    ap.add_argument("files", nargs="?", help="regex over unit paths relative to src/ (default: every unit)")
    ap.add_argument("-j", "--jobs", type=int, default=2)
    ap.add_argument("--auto", action="store_true", help="the generated drafts too (src/auto)")
    ap.add_argument("--by-address", action="store_true",
                    help="a name the map resolves is compared as its map address (a respelled placeholder "
                         "is the same); a name no row resolves is compared by name")
    ap.add_argument("--work", type=Path, default=HERE / "build" / "objcmp",
                    help="the scratch tree and objects (one per concurrent user)")
    a = ap.parse_args(argv)
    work = a.work.resolve()
    reverted = base_tree(a.base, work / "base")
    print(f"against {a.base}: {' '.join(reverted) or '(no file differs)'}", flush=True)
    resolve = None
    if a.by_address:
        bm = base_map(a.base, work / "base-map")
        resolve = resolver(E.symbols(), bm)
        print(f"by address: the tree's map, then {a.base}'s" if bm else
              f"by address: the tree's map ({a.base} has none that parses)", flush=True)
    rx = re.compile(a.files) if a.files else None
    src_now, src_base = HERE / "src", work / "base" / "src"
    keep = lambda rel: (a.auto or rel.parts[:1] != ("auto",)) and (not rx or rx.search(str(rel)))
    rels = sorted({p.relative_to(src_now) for p in sccheck.sources(src_now)}
                  | {p.relative_to(src_base) for p in sccheck.sources(src_base)})
    rels = [r for r in rels if keep(r)]
    if a.auto and (src_now / "auto").is_dir():
        shutil.copytree(src_now / "auto", src_base / "auto", dirs_exist_ok=True)

    def one(rel: Path):
        sides = []
        for root, inc, obj in ((src_base, work / "base" / "include", work / "obj-base"),
                               (src_now, HERE / "include", work / "obj-now")):
            p = root / rel
            if not p.exists():
                sides.append((None, "absent"))
                continue
            _, o, err = sccheck.build_one(p, obj, root=root, include=inc)
            sides.append((o, (err.strip().splitlines() or ["?"])[-1][:70] if o is None else ""))
        (oa, ea), (ob, eb) = sides
        if oa is None or ob is None:
            return (f"builds on one side only: base {ea or 'ok'!r}, now {eb or 'ok'!r}"
                    if (oa is None) != (ob is None) else None), []
        na: dict[str, list[str]] = {}
        nb: dict[str, list[str]] = {}
        sa, sb = summary(oa, resolve, na), summary(ob, resolve, nb)
        diffs = [k for k in sorted(set(sa) | set(sb)) if sa.get(k) != sb.get(k)]
        if diffs:
            return "; ".join(diffs[:6]) + (f" (+{len(diffs) - 6})" if len(diffs) > 6 else ""), []
        return None, respellings(sa, na, nb) if resolve else []

    bad = respelled = 0
    with ThreadPoolExecutor(a.jobs) as ex:
        for rel, (d, pairs) in zip(rels, ex.map(one, rels)):
            if d:
                bad += 1
                print(f"DIFF {rel}: {d}", flush=True)
            elif pairs:
                respelled += 1
                shown = ", ".join(f"{x} -> {y}" for x, y in pairs[:4])
                print(f"SAME-BY-ADDRESS {rel}: {len(pairs)} name(s) respelled: {shown}"
                      + (f" (+{len(pairs) - 4})" if len(pairs) > 4 else ""), flush=True)
    print(f"{len(rels)} units compared, {bad} differ"
          + (f", {respelled} the same only by address" if resolve else ""))
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
