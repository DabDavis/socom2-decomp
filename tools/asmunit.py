#!/usr/bin/env python3
"""
asmunit -- write an assembler unit (`.s`) for library code that was written in assembly, and
prove it: the unit is assembled by the toolchain named and every function in it scored.

    ./tools/asmunit.py src/lib/libc/memchr.s 0x195688 --compiler ee-gcc2.96 --head HEAD.txt
    ./tools/asmunit.py src/lib/sdk/exception.s 0x1AC500 0x1AC640 0x1AC740 \\
                       --label 0x1AC600=_xlaunch --head HEAD.txt

Which objects were assembly is not guessed: the SOCOM 1 demo's symbols keep each input object's
`.text` section symbol, and GCC marks every object it compiled with `gcc2_compiled.` -- an SDK or
newlib object without one was assembled (README "The GCC front"). Code GCC compiled, inline `asm`
included, is written as C.

Each map row named becomes a function at its own offset in the unit (gaps zero-filled), each
instruction in its canonical form (objdump -M no-aliases,reg-names=numeric: numeric GPRs and
coprocessor registers -- `ctc1 $0,$31`, where gpr-names alone left the FPU's `c1_fcsr`, which GNU
as refuses -- and `ori`, not objdump's `dli`, for an ori from $0: `canonical`) under `.set
noreorder`, `noat` and `nomacro`, so the assembler adds, fills and moves nothing; a branch goes to
a local label, a j/jal to the map's name, and each lui whose %lo uses address the image becomes
%hi/%lo of the symbol there (a data.txt name, a function's, else `data_<VA>`) -- so the object
carries the relocations the checker resolves, as the original's did. An instruction this
assembler spells differently or refuses is written as `.word` with its mnemonic beside it (never
one that carries a relocation), and the unit is assembled again until every function matches.
"""

from __future__ import annotations

import argparse
import os
import re
import struct
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(HERE))
import scdecomp as E
import scdiff as D

LOADS_STORES = {0x1A, 0x1B, 0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29,
                0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x31, 0x36, 0x37, 0x39, 0x3E, 0x3F}


def sext(v: int) -> int:
    return v - 0x10000 if v & 0x8000 else v


def branch_target(addr: int, w: int) -> int | None:
    op, rs, rt = w >> 26, (w >> 21) & 31, (w >> 16) & 31
    if op in (4, 5, 6, 7, 0x14, 0x15, 0x16, 0x17) or (op == 1 and rt in (0, 1, 2, 3, 0x10, 0x11, 0x12, 0x13)) \
            or (op in (0x10, 0x11, 0x12) and rs == 8):
        return addr + 4 + (sext(w & 0xFFFF) << 2)
    return None


GPR_LOADS = {0x1A, 0x1B, 0x1E, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x37}


def writes(w: int) -> set[int]:
    """The GPRs an instruction may write (generously: a miss only ends a %hi's scan early)."""
    op, rs, rt, rd = w >> 26, (w >> 21) & 31, (w >> 16) & 31, (w >> 11) & 31
    if op == 0:
        return {rd} if (w & 0x3F) not in (0x08, 0x0C, 0x0D, 0x0F) else set()   # jr, syscall, break, sync
    if op == 3:
        return {31}
    if op == 1:
        return {31} if rt in (0x10, 0x11, 0x12, 0x13) else set()
    if op in (0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x18, 0x19) or op in GPR_LOADS:
        return {rt}
    if op in (0x10, 0x11, 0x12):
        return {rt} if rs in (0, 1, 2) else set()                    # mfc/dmfc/cfc, qmfc2
    if op == 0x1C:
        return {rd, rt}
    return set()


def canonical(w: int, text: str) -> str:
    """The instruction as GNU as must read it back. objdump 2.45 prints `ori rt,$0,imm` as the
    `dli rt,imm` macro even under no-aliases (its opcode table does not mark that alias), and GNU
    as, reading `dli`, takes the first form that fits -- `addiu` for any immediate below 0x8000, the
    wrong encoding (round 14b wrote largeint_asm.s's two `ori $2,$0,0x1` by hand: the tool had left
    them as .word). The other aliases objdump keeps under no-aliases (`negu`, `dnegu`, one-operand
    `jalr`) are opcode-table entries every as encodes back to the same word."""
    if w >> 26 == 0x0D and (w >> 21) & 31 == 0:
        return f"ori ${(w >> 16) & 31},$0,0x{w & 0xFFFF:x}"
    return text


def disasm_canonical(b: bytes, vma: int) -> list[tuple[int, int, str]]:
    tmp = HERE / "build" / "tmp"
    tmp.mkdir(parents=True, exist_ok=True)
    p = tmp / f"asmunit.{os.getpid()}.bin"
    p.write_bytes(b)
    try:
        out = subprocess.run([E.tool("objdump"), "-D", "-z", "-b", "binary", "-m", "mips:5900", "-EL",
                              "-M", "no-aliases,reg-names=numeric",
                              f"--adjust-vma=0x{vma:x}", str(p)], capture_output=True, text=True, check=True).stdout
    finally:
        p.unlink(missing_ok=True)
    rows = []
    for line in out.splitlines():
        m = E._INSN.match(line)
        if m:
            w = int(m.group(2), 16)
            rows.append((int(m.group(1), 16), w, canonical(w, m.group(3).replace("\t", " ").strip())))
    return rows


def symbol_for(addr: int, st: E.SymbolTable, local: dict[int, str]) -> tuple[str, int]:
    """(symbol, its address) to spell an address the code builds: a label of the unit, a data.txt
    name, a function, else the placeholder `data_<VA>` (resolved by the address in its name)."""
    if addr in local:
        return local[addr], addr
    d = st.data_containing(addr)
    if d is not None:
        return d.name, d.addr
    f = st.containing(addr)
    if f is not None:                       # code: the function, plus the offset into it
        return f.name, f.addr
    return f"data_{addr:08X}", addr


def plan(rows: list[E.Symbol], labels: dict[int, str], img: E.Image, st: E.SymbolTable):
    """Per function: [(addr, word, text, relocated)] with symbols in place of addresses."""
    lo_unit, hi_unit = rows[0].addr, rows[-1].end
    local = {r.addr: r.name for r in rows}
    local.update(labels)
    out = {}
    targets = set()
    raw = {}
    for r in rows:
        raw[r.name] = disasm_canonical(img.read(r.addr, r.size), r.addr)
        for a, w, _ in raw[r.name]:
            t = branch_target(a, w)
            if t is not None:
                targets.add(t)
    for r in rows:
        ins = raw[r.name]
        text = {a: t for a, _, t in ins}
        reloc = set()
        # %hi/%lo: each lui, and the loads, stores and addius off its register until it is rewritten
        for i, (a, w, _) in enumerate(ins):
            if w >> 26 != 0x0F:
                continue
            reg, hi = (w >> 16) & 31, (w & 0xFFFF) << 16
            uses = []
            # within the basic block: up to a branch target, or a branch's delay slot
            for k, (a2, w2, _) in enumerate(ins[i + 1:], i + 1):
                if a2 in targets:
                    break
                op2, rs2 = w2 >> 26, (w2 >> 21) & 31
                if rs2 == reg and (op2 in LOADS_STORES or op2 in (0x09, 0x19)):
                    uses.append((a2, w2, (hi + sext(w2 & 0xFFFF)) & 0xFFFFFFFF))
                prev = ins[k - 1][1]
                if reg in writes(w2) or branch_target(ins[k - 1][0], prev) is not None or prev >> 26 in (2, 3) \
                        or (prev >> 26 == 0 and prev & 0x3F in (0x08, 0x09)):
                    break
            if not uses or not all(img.in_memory(x) for _, _, x in uses):
                continue
            sym, base = symbol_for(uses[0][2], st, local)
            text[a] = f"lui ${reg},%hi({sym}{'+' + str(uses[0][2] - base) if uses[0][2] != base else ''})"
            reloc.add(a)
            for a2, w2, x in uses:
                d = x - base
                expr = f"%lo({sym}{'+' + str(d) if d > 0 else (str(d) if d < 0 else '')})"
                mn, ops = text[a2].split(None, 1)
                if (w2 >> 26) in (0x09, 0x19):
                    rt2 = (w2 >> 16) & 31
                    text[a2] = f"{mn} ${rt2},${reg},{expr}"
                else:
                    first = ops.split(",")[0]
                    text[a2] = f"{mn} {first},{expr}(${reg})"
                reloc.add(a2)
        body = []
        for a, w, _ in ins:
            t = text[a]
            bt = branch_target(a, w)
            if bt is not None:
                if not lo_unit <= bt < hi_unit:
                    t, rel = f".word 0x{w:08x}", False          # a branch out of the unit: no relocation to give it
                else:
                    ops = t.split(None, 1)
                    args = ops[1].rsplit(",", 1)
                    t = f"{ops[0]} " + (f"{args[0]}," if len(args) == 2 else "") + (local.get(bt) or f".L{bt:08X}")
            elif w >> 26 in (2, 3):
                tgt = ((a + 4) & 0xF0000000) | ((w & 0x03FFFFFF) << 2)
                s = local.get(tgt) or (st.at_addr(tgt).name if st.at_addr(tgt) else f"func_{tgt:08X}")
                t = f"{t.split()[0]} {s}"
                reloc.add(a)
            body.append((a, w, t, a in reloc))
        out[r.name] = body
    return out, targets, local


def render(rows, body, targets, labels, head: str, compiler: str, words: dict[int, bool], statics: set[str],
           bss: list[E.Symbol] = ()) -> str:
    out = [f"# sc: compiler={compiler}\n", head if head.endswith("\n") or not head else head + "\n",
           "\n\t.set\tnoreorder\n\t.set\tnoat\n\t.set\tnomacro\n\n\t.text\n"]
    pos = rows[0].addr
    align = 16 if rows[0].addr % 16 == 0 else 8
    out.append(f"\t.align\t{align.bit_length() - 1}\n")
    for r in rows:
        if r.addr > pos:
            out.append(f"\t.space\t{r.addr - pos}\n")
        out.append("\n" + ("" if r.name in statics else f"\t.globl\t{r.name}\n")
                   + f"\t.type\t{r.name},@function\n\t.ent\t{r.name}\n{r.name}:\n")
        for a, w, t, rel in body[r.name]:
            if a in labels and labels[a] != r.name:
                out.append(f"\t.globl\t{labels[a]}\n{labels[a]}:\n")
            if a in targets and a != r.addr and a not in labels:
                out.append(f".L{a:08X}:\n")
            if words.get(a) and not rel:
                out.append(f"\t.word\t0x{w:08x}\t\t# {t}\n")
            else:
                mn, *ops = t.split(None, 1)
                out.append(f"\t{mn}\t{ops[0] if ops else ''}".rstrip() + "\n")
        out.append(f"\t.end\t{r.name}\n\t.size\t{r.name},.-{r.name}\n")
        pos = r.end
    if bss:
        # the unit's own zeroed data, as local labels (GNU as relocates them against .bss; the
        # checker resolves those through the data map's rows of the same names: scdiff.bss_local)
        out.append("\n\t.bss\n\t.align\t4\n")
        pos = bss[0].addr
        for d in bss:
            if d.addr > pos:
                out.append(f"\t.space\t{d.addr - pos}\n")
            out.append(f"{d.name}:\n\t.space\t{d.size}\n")
            pos = d.addr + d.size
    return "".join(out)


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("out", type=Path)
    ap.add_argument("rows", nargs="+", help="map rows (names or addresses), in address order")
    ap.add_argument("--compiler", default="ee-gcc2.9-991111", help="the toolchain whose GNU as builds it")
    ap.add_argument("--label", action="append", default=[], help="ADDR=NAME: a global label inside a function")
    ap.add_argument("--static", action="append", default=[], help="a function the unit keeps local")
    ap.add_argument("--head", type=Path, help="the unit's header comment (after its directive line)")
    ap.add_argument("--bss", action="append", default=[], help="a data.txt row the unit defines in its own .bss")
    a = ap.parse_args(argv)
    img, st = E.image(), E.symbols()
    rows = [st.lookup(k) for k in a.rows]
    labels = {int(k, 16): v for k, v in (x.split("=") for x in a.label)}
    head = a.head.read_text() if a.head else ""
    bss = sorted((st.data_by_name[n] for n in a.bss), key=lambda d: d.addr)
    body, targets, local = plan(rows, labels, img, st)
    words: dict[int, bool] = {}
    obj = HERE / "build" / "tmp" / f"asmunit.{os.getpid()}.o"
    for rnd in range(6):
        text = render(rows, body, targets, labels, head, a.compiler, words, set(a.static), bss)
        a.out.parent.mkdir(parents=True, exist_ok=True)
        a.out.write_text(text)
        lines = text.splitlines()
        try:
            D.compile_obj(a.out, obj, a.compiler)
        except RuntimeError as e:
            bad = 0
            for m in re.finditer(r":(\d+): Error:", str(e)):
                ln = lines[int(m.group(1)) - 1]
                for r in rows:
                    for ad, w, t, rel in body[r.name]:
                        if not rel and ln.strip().startswith(t.split(None, 1)[0]) and \
                                re.sub(r"\s+", " ", ln.strip()) == re.sub(r"\s+", " ", t):
                            if not words.get(ad):
                                words[ad] = True
                                bad += 1
            if not bad:
                print(str(e)[-2000:])
                return 1
            continue
        fixes = 0
        results = []
        for r in rows:
            res = D.compare(obj, r.name, r, img, st)
            results.append(res)
            if res.match:
                continue
            cand, relocs, _ = E.Elf.read(obj).function(r.name)
            tw = E.words(D.mask(img.read(r.addr, r.size), relocs))
            cw = E.words(D.mask(cand, relocs))
            if len(cw) != len(tw):
                print(f"{r.name}: size 0x{len(cand):X} vs 0x{r.size:X}")
                continue
            for k, (x, y) in enumerate(zip(tw, cw)):
                ad = r.addr + 4 * k
                if x != y and not words.get(ad):
                    words[ad] = True
                    fixes += 1
        if all(x.match for x in results):
            nw = sum(1 for v in words.values() if v)
            print(f"{a.out}: {len(rows)} function(s) byte-exact, "
                  f"{sum(len(x.relocs) for x in results)} relocations resolved, {nw} instruction(s) as .word")
            obj.unlink(missing_ok=True)
            return 0
        if not fixes:
            for res in results:
                if not res.match:
                    print(D.report(res))
            return 1
    print("gave up after six rounds")
    return 1


if __name__ == "__main__":
    sys.exit(main())
