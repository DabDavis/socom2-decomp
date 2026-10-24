#!/usr/bin/env python3
"""
Source-file hints for SOCOM II functions, out of SOCOM 1's July 9 2002 press beta.

    ./tools/beta_tu.py                   # report
    ./tools/beta_tu.py --write           # add `tu:` hints to placeholder rows of symbols.txt

The press beta (SCUS_971.34, sha256 b55e8a7f..., extracted/socom1-press-beta-020709/) is
stripped -- no symbols, no DWARF -- but it keeps its asserts, and each assert names its
source file (`zmath_main.cpp`); SOCOM II compiled its own asserts out. So:

  1. sweep the beta's code into functions (seed_symbols' walk) and give each function the
     file names its asserts reference: a FACT about that function, in SOCOM 1;
  2. a function between two functions asserting the SAME file, with no other file between,
     is inferred to be that file too (claude-eqoa's `File.cpp~` hint) -- an inference;
  3. carry a hint onto a SOCOM II function only where the two are byte-identical with
     their relocation fields masked, at the same size, and the beta side is unique.

A hint is never a name: it says which file the function was in two years earlier.
"""

from __future__ import annotations

import argparse
import re
import struct
import sys
from collections import defaultdict
from pathlib import Path

HERE = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(HERE))
sys.path.insert(0, str(HERE / "tools"))
import scdecomp as E
import seed_symbols as S

BETA = Path(E.ROOT / "extracted" / "socom1-press-beta-020709" / "SCUS_971.34")
BETA_SHA = "b55e8a7f75a8fe087fa4ae94ef35c26ace2e6683e1cd75faf44eec36f12ee1be"


def maskw(x: int) -> int:
    op = x >> 26
    if op in (2, 3):
        return x & 0xFC000000                       # j / jal targets
    if op in (0x0F, 0x09) or 0x20 <= op <= 0x3F:
        return x & 0xFFFF0000                       # lui, addiu and every load/store immediate
    return x


def sig(b: bytes) -> tuple:
    return tuple(maskw(x) for x in struct.unpack(f"<{len(b) // 4}I", b[:len(b) // 4 * 4]))


class Beta:
    def __init__(self, path: Path = BETA):
        import hashlib
        raw = path.read_bytes()
        if hashlib.sha256(raw).hexdigest() != BETA_SHA:
            raise SystemExit(f"{path}: not the July 9 2002 press beta this was measured on")
        elf = E.Elf(raw, str(path))
        _, off, self.base, filesz, _, _ = elf.segments[0]
        self.data = raw[off:off + filesz]

    def word(self, a: int) -> int:
        return struct.unpack_from("<I", self.data, a - self.base)[0]

    def read(self, a: int, n: int) -> bytes:
        return self.data[a - self.base:a - self.base + n]

    def text_end(self) -> int:
        """The end of the code: the last jr ra before a run of 64 KB holding none."""
        jr = [a for a in range(self.base, self.base + len(self.data) - 4, 4) if self.word(a) == E.JR_RA]
        end = jr[0]
        for a, b in zip(jr, jr[1:]):
            if b - a > 0x10000:
                break
            end = b
        return end + 8

    def file_refs(self) -> dict[int, str]:
        """lui/addiu pairs that address a source-file string -> {instruction: file}."""
        names = {self.base + m.start(): m.group()[:-1].decode()
                 for m in re.finditer(rb"[A-Za-z0-9_./\\-]+\.(?:cpp|c)\x00", self.data)}
        out = {}
        end = self.base + len(self.data) - 4
        for a in range(self.base, end, 4):
            x = self.word(a)
            if x >> 26 != 0x0F:
                continue
            rt, hi = (x >> 16) & 31, x & 0xFFFF
            for b in range(a + 4, min(a + 40, end), 4):
                y = self.word(b)
                if y >> 26 == 0x09 and (y >> 21) & 31 == rt:
                    lo = y & 0xFFFF
                    t = ((hi << 16) + (lo - 0x10000 if lo & 0x8000 else lo)) & 0xFFFFFFFF
                    if t in names:
                        out[b] = names[t].replace("\\", "/").rsplit("/", 1)[-1]
                    break
        return out


def beta_functions(beta: Beta) -> list[tuple[int, int, set[str], str]]:
    """(start, end, files asserted, hint) for every function in the beta's code."""
    end = beta.text_end()
    calls = {((a + 4) & 0xF0000000) | ((beta.word(a) & 0x3FFFFFF) << 2)
             for a in range(beta.base, end, 4) if beta.word(a) >> 26 == 3}
    funcs = S.sweep(beta, beta.base, end, calls)
    refs = beta.file_refs()
    rows = []
    for s, e in funcs:
        files = {f for a, f in refs.items() if s <= a < e}
        rows.append([s, e, files, ""])
    # direct facts, then the between-two-anchors inference
    for r in rows:
        if len(r[2]) == 1:
            r[3] = next(iter(r[2]))
    anchors = [i for i, r in enumerate(rows) if len(r[2]) == 1]
    for i, j in zip(anchors, anchors[1:]):
        f = rows[i][3]
        if rows[j][3] == f:
            for k in range(i + 1, j):
                if not rows[k][2]:
                    rows[k][3] = f + "~"
    return [tuple(r) for r in rows]


def carry(img: E.Image, st: E.SymbolTable, beta: Beta, rows) -> dict[int, str]:
    by_sig = defaultdict(list)
    for s, e, files, hint in rows:
        if hint and e - s >= 16:
            by_sig[(e - s, sig(beta.read(s, e - s)))].append(hint)
    out = {}
    for sym in st.symbols:
        seg = img.segment(sym.addr)
        if seg is None or seg.name == "MAIN" or not img.is_code(sym.addr) or sym.size < 16:
            continue
        hits = by_sig.get((sym.size, sig(img.read(sym.addr, sym.size))))
        if hits and len(set(hits)) == 1:
            out[sym.addr] = hits[0]
    return out


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--write", action="store_true")
    a = ap.parse_args(argv)
    img, st, beta = E.image(), E.symbols(), Beta()
    rows = beta_functions(beta)
    facts = sum(1 for r in rows if r[3] and not r[3].endswith("~"))
    inferred = sum(1 for r in rows if r[3].endswith("~"))
    print(f"beta: {len(rows)} functions, {facts} assert a file, {inferred} inferred between two of the same file")
    hints = carry(img, st, beta, rows)
    print(f"SOCOM II: {len(hints)} functions byte-identical to a hinted beta function "
          f"({sum(1 for h in hints.values() if not h.endswith('~'))} of them asserting there)")
    per = defaultdict(int)
    for h in hints.values():
        per[h.rstrip("~")] += 1
    print("  " + ", ".join(f"{k} {v}" for k, v in sorted(per.items(), key=lambda kv: -kv[1])[:30]))
    if a.write:
        text = E.SYMBOLS.read_text().splitlines()
        n = 0
        for i, line in enumerate(text):
            m = re.match(r"^(0x[0-9A-Fa-f]+)\s+(\S+)\s+(\S+)(.*)$", line)
            if not m or int(m.group(1), 16) not in hints or "tu:" in line:
                continue
            note = m.group(4)
            sep = "; " if "#" in note else "  # "
            text[i] = line + f"{sep}tu: {hints[int(m.group(1), 16)]} (1.0 beta)"
            n += 1
        E.SYMBOLS.write_text("\n".join(text) + "\n")
        print(f"wrote {n} tu: hints into {E.SYMBOLS.name}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
