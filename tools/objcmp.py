#!/usr/bin/env python3
"""
objcmp -- prove a header (or any source) change code-neutral, object for object.

    ./tools/objcmp.py master                     # every hand-written unit, against master's text
    ./tools/objcmp.py HEAD~1 'zseal/|zkit/'      # the units whose path (under src/) matches
    ./tools/objcmp.py master --auto -j 2         # the generated drafts too

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
import sccheck

SHF_ALLOC, SHF_EXECINSTR = 0x2, 0x4
SHT_PROGBITS, SHT_NOBITS = 1, 8


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


def summary(obj: Path) -> dict[str, object]:
    """What a unit's object holds, keyed so that two builds of one source compare equal."""
    elf = E.Elf.read(obj)
    anon = sorted({int(s.name[1:]) for s in elf.symbols if re.fullmatch(r"@\d+", s.name or "")})
    rank = {f"@{n}": f"@#{i}" for i, n in enumerate(anon)}

    def target(sym: E.ElfSymbol) -> str:
        if sym.type == "SECTION" and 0 < sym.shndx < len(elf.sections):
            return "section " + elf.sections[sym.shndx].name
        name = rank.get(sym.name, sym.name)
        return re.sub(r"\$\d+$", "$", name)

    def relocs(rels) -> tuple:
        return tuple((r.offset, r.type, target(r.sym), r.addend) for r in rels)

    out: dict[str, object] = {}
    for f in elf.functions():
        code, rels, _ = elf.function(f.name)
        out[f"function {target(f)}"] = (code, relocs(rels))
    seen: dict[str, int] = {}
    for sec in elf.sections:
        if not sec.flags & SHF_ALLOC or sec.flags & SHF_EXECINSTR or sec.type not in (SHT_PROGBITS, SHT_NOBITS):
            continue
        n = seen[sec.name] = seen.get(sec.name, 0) + 1
        body = None if sec.type == SHT_NOBITS else sec.data
        out[f"section {sec.name}#{n}"] = (sec.size, body, relocs(elf.relocs_for(sec.index)))
    for s in elf.symbols:
        if s.type == "OBJECT" and 0 < s.shndx < len(elf.sections):
            out[f"object {target(s)}"] = (elf.sections[s.shndx].name, s.value, s.size)
    return out


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("base", help="the revision to compare against (master, HEAD~1, a hash)")
    ap.add_argument("files", nargs="?", help="regex over unit paths relative to src/ (default: every unit)")
    ap.add_argument("-j", "--jobs", type=int, default=2)
    ap.add_argument("--auto", action="store_true", help="the generated drafts too (src/auto)")
    ap.add_argument("--work", type=Path, default=HERE / "build" / "objcmp",
                    help="the scratch tree and objects (one per concurrent user)")
    a = ap.parse_args(argv)
    work = a.work.resolve()
    reverted = base_tree(a.base, work / "base")
    print(f"against {a.base}: {' '.join(reverted) or '(no file differs)'}", flush=True)
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
                    if (oa is None) != (ob is None) else None)
        sa, sb = summary(oa), summary(ob)
        diffs = [k for k in sorted(set(sa) | set(sb)) if sa.get(k) != sb.get(k)]
        return "; ".join(diffs[:6]) + (f" (+{len(diffs) - 6})" if len(diffs) > 6 else "") if diffs else None

    bad = 0
    with ThreadPoolExecutor(a.jobs) as ex:
        for rel, d in zip(rels, ex.map(one, rels)):
            if d:
                bad += 1
                print(f"DIFF {rel}: {d}", flush=True)
    print(f"{len(rels)} units compared, {bad} differ")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
