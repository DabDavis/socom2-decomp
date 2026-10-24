#!/usr/bin/env python3
"""
sccheck -- build the decomp source tree and score every function in it.

The regression harness, modelled on claude-eqoa's eqcheck.py: compile src/*.c and
src/*.cpp with the settled Metrowerks toolchain and diff every function symbol in
the objects against the r0001 image, through scdiff (masked bytes AND every
relocation resolved to the address the target uses).

  ./sccheck.py                  # score everything
  ./sccheck.py -f LoadOverlay   # only symbols matching a regex
  ./sccheck.py --wrong          # and show the instruction diff of each mismatch
  ./sccheck.py --baseline       # rewrite src/MATCHED.txt from this run
  ./sccheck.py --files znode/ --build-dir build/obj-me   # only those units, built privately
                                (what a concurrent user -- an agent -- runs: no baseline, and
                                the regression check covers only the units built)

A symbol in the source that is NOT in symbols.txt is reported as `no-target`,
never skipped silently -- a typo'd mangled name would otherwise read as "nothing
to do". A TU that fails to build is a loud row. MATCHED.txt records what is
byte-exact; a function in it that no longer matches is a REGRESSION and a
non-zero exit, so a source change made for function N cannot quietly break N-1.

A TU may carry its own flags on its first line:   /* sc: flags=-O4,s */

An inline function's out-of-line copy (MW's binding 13: every unit that needs one emits one,
and the linker keeps the first) is scored like any function, but when it misses and its map
address lies outside its own unit's span it is `elsewhere`, not WRONG: the linked copy is an
earlier unit's, made where it was inlined differently (zcam_main.cpp's CNode::GetBBox against
zanim_cmd_tick.cpp's, 0x262550), and this unit's was dropped.
"""

from __future__ import annotations

import argparse
import re
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import scdecomp as E
import scdiff as D

SRC = HERE / "src"
BUILD = HERE / "build" / "obj"
INCLUDE = HERE / "include"
MATCHED = SRC / "MATCHED.txt"
ABSENT = SRC / "ABSENT.txt"

_DIRECTIVE = re.compile(r"sc:\s*(.*?)\s*(?:\*/|$)")


def sources() -> list[Path]:
    return sorted(p for p in SRC.rglob("*") if p.suffix in (".c", ".cpp") and p.is_file())


def tu_options(src: Path) -> dict[str, str]:
    opts = {}
    for line in src.read_text(errors="replace").splitlines()[:3]:
        m = _DIRECTIVE.search(line)
        if m:
            for part in m.group(1).split(";"):
                if "=" in part:
                    k, v = part.split("=", 1)
                    opts[k.strip()] = v.strip()
            break
    return opts


def rename_symbol(obj: Path, old: str, new: str) -> None:
    """A generated draft defines its row's placeholder when the map's name is no C identifier
    (a template instance); the object then carries the map's name, as the original's did."""
    subprocess.run([E.tool("objcopy"), f"--redefine-sym={old}={new}", str(obj)], check=True,
                   capture_output=True)


def build_one(src: Path, build: Path = BUILD) -> tuple[Path, Path | None, str]:
    rel = src.relative_to(SRC)
    obj = build / rel.with_suffix(".o")
    opts = tu_options(src)
    try:
        D.compile_obj(src, obj, opts.get("compiler", D.COMPILER),
                      opts["flags"].split() if "flags" in opts else None, [INCLUDE, SRC])
        if "as" in opts:
            old_name, new_name = opts["as"].split(None, 1)
            rename_symbol(obj, old_name, new_name)
        return src, obj, ""
    except RuntimeError as e:
        return src, None, str(e)


MULTIDEF = 13   # MW's binding for an inline function's (or a template's) out-of-line copy


def unit_span(obj: Path, st) -> tuple[int, int] | None:
    """The map addresses the unit's own (non-inline) functions cover."""
    lo = hi = None
    for f in E.Elf.read(obj).functions():
        if f.bind == MULTIDEF:
            continue
        try:
            sym = st.get(f.name)
        except KeyError:
            continue
        lo = sym.addr if lo is None else min(lo, sym.addr)
        hi = sym.addr + sym.size if hi is None else max(hi, sym.addr + sym.size)
    return (lo, hi) if lo is not None else None


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("-f", "--filter", help="regex over symbol names")
    ap.add_argument("--wrong", action="store_true", help="print the diff of every mismatch")
    ap.add_argument("--baseline", action="store_true", help="rewrite MATCHED.txt")
    ap.add_argument("-j", "--jobs", type=int, default=6)
    ap.add_argument("-v", "--verbose", action="store_true", help="list the generated tier's matches too")
    ap.add_argument("--files", help="regex over source paths (relative to src/): build and score only those")
    ap.add_argument("--build-dir", type=Path, default=BUILD, help="where the objects go (one per concurrent user)")
    a = ap.parse_args(argv)
    if a.files and a.baseline:
        raise SystemExit("--baseline needs the whole tree (no --files)")
    frx = re.compile(a.files) if a.files else None

    img, st = E.image(), E.symbols()
    rx = re.compile(a.filter) if a.filter else None
    absent = set()
    if ABSENT.exists():
        absent = {l.split("#")[0].strip() for l in ABSENT.read_text().splitlines()} - {""}
    clash = sorted(n for n in absent if n in st._by_name)
    if clash:
        raise SystemExit(f"{ABSENT.name} lists names symbols.txt maps: {clash}")
    with ThreadPoolExecutor(a.jobs) as ex:
        units = [p for p in sources() if not frx or frx.search(str(p.relative_to(SRC)))]
        built = list(ex.map(lambda p: build_one(p, a.build_dir.resolve()), units))

    rows, matched, failed_builds = [], [], 0
    auto_names: set[str] = set()
    hand_names: set[str] = set()
    for src, obj, err in built:
        if obj is not None:
            names = {f.name for f in E.Elf.read(obj).functions()}
            (auto_names if "auto" in src.relative_to(SRC).parts else hand_names).update(names)
    superseded = sorted(auto_names & hand_names)
    for src, obj, err in built:
        if obj is None:
            failed_builds += 1
            rows.append(("BUILD-FAIL", src.name, "", err.strip().splitlines()[-1] if err.strip() else ""))
            continue
        is_auto = "auto" in src.relative_to(SRC).parts
        span = unit_span(obj, st)
        for fs in E.Elf.read(obj).functions():
            if rx and not rx.search(fs.name):
                continue
            if is_auto and fs.name in hand_names:
                continue                     # hand-written source supersedes the generated tier
            if fs.name in absent:
                rows.append(("absent", src.name, fs.name, ""))
                continue
            try:
                sym = st.get(fs.name)
            except KeyError:
                rows.append(("no-target", src.name, fs.name, ""))
                continue
            res = D.compare(obj, fs.name, sym, img, st)
            if not res.match and fs.bind == MULTIDEF and span and not span[0] <= sym.addr < span[1]:
                rows.append(("elsewhere", src.name, fs.name, f"inline copy; linked at {sym.addr:#x}"))
                continue
            if res.match:
                matched.append(fs.name)
                note = f"{len(res.relocs)} relocs" + (f", {len(res.reloc_unchecked)} unchecked"
                                                       if res.reloc_unchecked else "")
                rows.append(("match", src.name, fs.name, note))
            else:
                rows.append(("WRONG", src.name, fs.name, res.verdict))
                if a.wrong:
                    print(D.report(res))
                    print()

    quiet = {"match"} if not a.verbose else set()
    for status, tu, name, note in rows:
        if status in quiet and tu[:1].isdigit():
            continue                         # the generated tier's matches are counted, not listed
        print(f"{status:<10} {tu:<28} {name:<48} {note}")

    # the baseline is keyed by ADDRESS: renaming a row (s1names does, by the hundred) must not
    # read as losing its match, nor a new name hide a real loss
    baseline: dict[int, str] = {}
    if MATCHED.exists():
        for l in MATCHED.read_text().splitlines():
            if l.strip() and not l.startswith("#"):
                addr, name = l.split()
                baseline[int(addr, 16)] = name
    now = {st.get(n).addr for n in matched}
    # a partial build (--files) can only lose what it scored: the addresses its units define
    scored_addrs = None
    if frx:
        scored_addrs = set()
        for r in rows:
            if r[0] in ("match", "WRONG"):
                try:
                    scored_addrs.add(st.get(r[2]).addr)
                except KeyError:
                    pass
    regressions = sorted(f"{a:#x} {baseline[a]}" + (f" (now {st.at_addr(a).name})" if st.at_addr(a)
                         and st.at_addr(a).name != baseline[a] else "")
                         for a in set(baseline) - now if (not rx or rx.search(baseline[a]))
                         and (scored_addrs is None or a in scored_addrs))
    total_code = sum(s.size for s in st.symbols)
    # a template instance two units emit (an MSL vector's reserve) is one function: count names
    matched = sorted(set(matched))
    scored = {r[2] for r in rows if r[0] in ('match', 'WRONG')}
    matched_bytes = sum(st.get(n).size for n in matched)
    n_auto = sum(1 for n in matched if n in auto_names and n not in hand_names)
    print(f"\n{len(matched)} byte-exact of {len(scored)} scored"
          f" ({matched_bytes:,} of {total_code:,} bytes in the map, {100 * matched_bytes / max(total_code, 1):.2f} %)")
    print(f"  {len(matched) - n_auto} hand-written, {n_auto} generated (src/auto, m2c drafts byte-exact as they stand)")
    if superseded:
        print(f"  {len(superseded)} generated function(s) superseded by hand-written source -- "
              f"run tools/automatch.py --emit to drop them: {', '.join(superseded[:5])}")
    if failed_builds:
        print(f"{failed_builds} TU(s) failed to build")
    if regressions:
        print(f"REGRESSIONS against {MATCHED.name}: {', '.join(regressions)}")
    if a.baseline:
        MATCHED.write_text("# byte-exact functions, address and name; rewritten by ./sccheck.py --baseline\n"
                           + "".join(f"0x{a:08X} {st.at_addr(a).name}\n" for a in sorted(now)))
        print(f"wrote {MATCHED} ({len(set(matched))} functions)")
    return 1 if regressions or failed_builds else 0


if __name__ == "__main__":
    sys.exit(main())
