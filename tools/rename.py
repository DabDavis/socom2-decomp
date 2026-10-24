#!/usr/bin/env python3
"""
Rename map rows by hand, keeping each row's note and saying why.

    ./tools/rename.py 0x351480 __ct__3CVUFUi "s1:order (registers the same VIF0 handler)"
    ./tools/rename.py --file renames.txt     # one `ADDR NAME NOTE...` per line

The old note (segment, `tu:` hints, earlier verdicts) is kept and the new provenance is
appended. A name the map already holds elsewhere is refused (the map's names are unique).
Every file under src/ that still spells the OLD name is listed: generated drafts are
re-emitted by `tools/automatch.py --addrs LIST --redo --emit`, hand-written source is edited.
"""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(HERE))
import scdecomp as E

ROW = re.compile(r"^(0x[0-9A-Fa-f]+)\s+(\S+)\s+(\S+)(.*)$")


def rename(pairs: list[tuple[int, str, str]]) -> int:
    lines = E.SYMBOLS.read_text().splitlines()
    taken = {}
    for i, line in enumerate(lines):
        m = ROW.match(line)
        if m:
            taken[m.group(3)] = int(m.group(1), 16)
    old_names, bad = [], 0
    want = {a: (n, note) for a, n, note in pairs}
    for i, line in enumerate(lines):
        m = ROW.match(line)
        if not m or int(m.group(1), 16) not in want:
            continue
        addr = int(m.group(1), 16)
        name, note = want.pop(addr)
        if name in taken and taken[name] != addr:
            print(f"REFUSED {addr:#x} {name}: already the name of {taken[name]:#x}")
            bad += 1
            continue
        rest = m.group(4)
        sep = "; " if "#" in rest else "  # "
        lines[i] = f"{m.group(1)} {m.group(2)} {name}{rest}{sep}{note}" if note else f"{m.group(1)} {m.group(2)} {name}{rest}"
        old_names.append(m.group(3))
        print(f"{addr:#010x} {m.group(3)} -> {name}")
    for addr in want:
        print(f"NO ROW at {addr:#x}")
        bad += 1
    E.SYMBOLS.write_text("\n".join(lines) + "\n")
    users = {}
    for f in (HERE / "src").rglob("*"):
        if f.suffix in (".c", ".cpp", ".h") or f.name in ("list", "memory"):
            text = f.read_text(errors="replace")
            for n in old_names:
                if re.search(rf"\b{re.escape(n)}\b", text):
                    users.setdefault(n, []).append(str(f.relative_to(HERE)))
    for n, fs in sorted(users.items()):
        print(f"  {n} still spelled in: {', '.join(sorted(fs)[:6])}{' ...' if len(fs) > 6 else ''}")
    return 1 if bad else 0


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("addr", nargs="?")
    ap.add_argument("name", nargs="?")
    ap.add_argument("note", nargs="*")
    ap.add_argument("--file", type=Path)
    a = ap.parse_args(argv)
    pairs = []
    if a.file:
        for line in a.file.read_text().splitlines():
            if line.strip() and not line.startswith("#"):
                addr, name, *note = line.split(None, 2)
                pairs.append((int(addr, 16), name, note[0] if note else ""))
    if a.addr:
        pairs.append((int(a.addr, 16), a.name, " ".join(a.note)))
    return rename(pairs)


if __name__ == "__main__":
    sys.exit(main())
