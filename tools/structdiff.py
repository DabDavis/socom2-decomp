#!/usr/bin/env python3
"""
structdiff -- one function against its target with what register allocation and the link change
normalised away, so only STRUCTURAL differences show (the game states' round 35 structdiff.py and
mydiff.py, promoted by the GCC lane's round 31a).

    ./tools/structdiff.py src/lib/sdk/mpeg_dec.c dmaRefImage            # registers normalised
    ./tools/structdiff.py src/lib/sdk/mpeg_dec.c dmaRefImage --regs     # registers kept (mydiff.py)
    ./tools/structdiff.py src/Apps/FTS/x.cpp F__Fv --target 0x2C6290 -n 3
    ./tools/structdiff.py build/scratch/gcc/probe.c f --unit src/lib/sdk/libscf.c   # a probe as the unit

SRC is built as sccheck builds it -- its `sc:` directive (toolchain, flags, `as=`), its statics
qualified `NAME@UNIT` -- unless --compiler/--flags override it, as scdiff.py's do (scdiff's options,
the same names and meanings; --unit is ccsweep's and ccdump's: a probe compiled under that unit's
directive, its statics qualified as that unit's). SYMBOL is compared with its row in the unit's
overlay (the copy an `ovl-copy:` mark names when the named row lies in another), or with --target's
(a map name or an address).

Normalised on BOTH sides (the instruction text objdump prints, operand by operand):
  - the allocatable registers (v0-v1, a0-a7, t0-t9, s0-s8 as `r`; the FPRs as `$f`) -- unless
    --regs; zero, at, gp, sp, k0/k1 and ra keep their names (a role, not a choice);
  - every relocated immediate: the candidate's by its own relocation table (R_MIPS_HI16, LO16,
    GPREL16, LITERAL), the target's -- which has none -- by what a link writes: a `lui` of an image
    address's high half (0x10-0x71), the offsets through that register until it is written, and
    every $gp offset; the candidate gets the target's rule too (a constant lui that looks like an
    address is masked on both sides alike);
  - branch targets, and a `j` inside the function (they move with any change of length);
  - a call (j/jal outside the function) by its callee's NAME: the candidate's relocation resolved to
    the map's row the way the scorer resolves it (a placeholder, a respelled name, a GNU static, a
    `.text`+N callee), the target's destination by the map -- so calling another function IS a
    structural difference, which masking the field would hide.
Kept as they are: the opcodes, every other immediate (a field offset, a constant, a frame slot, a
shift count) and the order.

Each differing hunk is printed with the real text, the candidate's relocations named
(`%lo(.rodata+0x40)`, `jal sceSifCallRpc`), then one line: `target N, candidate M; structural
differences K` -- and, when the two agree in structure but not in their words, how many instructions
differ in their registers alone: a register-only miss (README "The GCC front": ccdump's -f
priorities are the tool for those). rc 0 when nothing structural differs (with --regs, nothing at
all).
"""

from __future__ import annotations

import argparse
import difflib
import os
import re
import struct
import sys
from dataclasses import dataclass
from pathlib import Path

HERE = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(HERE))
import scdecomp as E
import scdiff as D
import sccheck as C

# objdump's names for the registers an allocator chooses; the rest name a role
ALLOC_GPR = re.compile(r"\b(?:v[01]|a[0-7]|t[0-9]|s[0-8]|fp)\b")
FPR = re.compile(r"\$f\d+\b")
GPR_NAME = re.compile(r"^(?:zero|at|v[01]|a[0-7]|t[0-9]|s[0-8]|k[01]|gp|sp|fp|ra)$")
MEM = re.compile(r"^(?P<pre>.*,)(?P<off>-?(?:0x[0-9a-f]+|\d+))\((?P<base>\w+)\)$")
IMM16 = re.compile(r"^(?P<pre>(?:d?addiu) \w+,(?P<base>\w+),)(?P<off>-?\d+)$")
LUI = re.compile(r"^lui (?P<rt>\w+),(?P<imm>0x[0-9a-f]+|\d+)$")
# the image's own address space: MAIN 0x100000.., OVL1 0x1E7000.., OVL2 0x4C5380..0x71FFFF
HI_LO, HI_HI = 0x10, 0x71
NO_DEST = ("sb", "sh", "sw", "sd", "sq", "swl", "swr", "sdl", "sdr", "swc1", "sdc1", "sqc2", "sc", "scd",
           "mtc1", "dmtc1", "ctc1", "mtc0", "ctc2", "qmtc2", "mthi", "mtlo", "mthi1", "mtlo1", "mtsa",
           "jr", "jalr", "j", "jal", "nop", "sync", "sync.l", "sync.p", "syscall", "break", "cache",
           "pref", "teq", "tne", "eret", "mult", "multu", "div", "divu", "mult1", "multu1", "div1",
           "divu1", "madd", "maddu", "madd1", "maddu1")


# --- building and finding the function ----------------------------------------------------------

def under(p: Path, root: Path) -> bool:
    try:
        p.resolve().relative_to(root.resolve())
        return True
    except ValueError:
        return False


def build(src: Path, compiler: str | None = None, flags: str | None = None, includes=(),
          unit: Path | None = None, tag: str = "structdiff") -> Path:
    """`src` built as sccheck builds it (or under `unit`'s directive and qualifiers), into an object
    named after its path and the process (concurrent users never share one)."""
    dunit = unit or src
    opts = C.tu_options(dunit)
    bad = C.directive_problem(dunit)
    if bad and not (compiler and "toolchain" in bad):     # --compiler overrides the unit's own
        raise SystemExit(bad)
    cc = compiler or opts.get("compiler", D.COMPILER)
    fl = flags if flags is not None else opts.get("flags")
    name = str(src.resolve()).strip("/").replace("/", "_")
    obj = HERE / "build" / tag / f"{name}.{os.getpid()}.o"
    D.compile_obj(src, obj, cc, fl.split() if fl else None,
                  [HERE / "include", C.SRC] + [Path(i) for i in includes] + [src.parent])
    D.qualify_statics(obj, dunit, E.symbols(), C.SRC if under(dunit, C.SRC) else None)
    if "as" in opts:
        old, new = opts["as"].split(None, 1)
        C.rename_symbol(obj, old, new)
    return obj


def target_row(st, obj: Path, symbol: str, target: str | None, overlay: str | None):
    """The row SYMBOL is compared with: --target's (a name or an address), else its scoring row."""
    if target:
        if re.fullmatch(r"(0x)?[0-9a-fA-F]{5,8}", target):
            row = st.at_addr(int(target, 16))
            if row is None:
                raise SystemExit(f"--target {target}: no row starts there")
            return row
        return st.lookup(target)
    try:
        return C.scoring_row(st, symbol, C.unit_overlay(E.Elf.read(obj), st, overlay))
    except KeyError:
        if E.is_placeholder(symbol) and st.at_addr(int(symbol[-8:], 16)) is not None:
            return st.at_addr(int(symbol[-8:], 16))   # a placeholder its row has since renamed
        raise SystemExit(f"{symbol}: no row in the map (give --target)")


# --- the candidate's relocations, named as the scorer reads them --------------------------------

def _s16(v: int) -> int:
    return v - 0x10000 if v & 0x8000 else v


def reloc_names(elf: E.Elf, fsym: E.ElfSymbol, cand: bytes, relocs: list[E.Reloc], taddr: int,
                st) -> dict[int, tuple[str, str, int | None, int | None]]:
    """offset -> (relocation type, the label it names, the map address it resolves to or None, the
    offset into this function it lands on or None). A named symbol by its name (and addend); a GNU
    section symbol -- every TU-local reference -- by the label the object has at that offset (a
    static, `_data_table@sifcmd`), else `.rodata+0x40`; a `.text`+N callee by the function there.
    The address is the scorer's own resolution (scdiff._sym_address: the row, an `ovl-copy:` in
    this overlay, a placeholder's)."""
    gnu = D.is_gnu(elf)
    partner = D.gnu_hi_partners(elf, fsym, cand) if gnu else {}
    his: dict[int, int] = {}           # a %hi's addend is its first %lo's (GNU as writes it just before)
    for l, h in sorted(partner.items()):
        his.setdefault(h, l)
    word = lambda off: struct.unpack_from("<I", cand, off)[0] if off + 4 <= len(cand) else 0
    out = {}
    for r in relocs:
        w = word(r.offset)
        if r.addend is not None:
            add = r.addend
        elif r.type == "R_MIPS_26":
            add = (w & 0x03FFFFFF) << 2
        elif r.type == "R_MIPS_LO16":
            add = _s16(w & 0xFFFF) + ((word(partner[r.offset]) & 0xFFFF) << 16 if r.offset in partner else 0)
        elif r.type == "R_MIPS_HI16":
            add = ((w & 0xFFFF) << 16) + (_s16(word(his[r.offset]) & 0xFFFF) if r.offset in his else 0)
        elif r.type in ("R_MIPS_GPREL16", "R_MIPS_LITERAL"):
            add = _s16(w & 0xFFFF)
        elif r.type == "R_MIPS_32":
            add = w
        else:
            add = 0
        s = r.sym
        addr = None
        own = None                       # a jump into this function itself
        if s.shndx == fsym.shndx and r.type in ("R_MIPS_26", "R_MIPS_PC16"):
            at = (s.value if s.type != "SECTION" else 0) + add
            if fsym.value <= at < fsym.value + fsym.size:
                own = at - fsym.value
        if s.type == "SECTION" or not s.name:
            sec = elf.sections[s.shndx] if 0 < s.shndx < len(elf.sections) else None
            sname = sec.name if sec else "?"
            if r.type == "R_MIPS_26" and gnu and s.shndx == fsym.shndx and add == fsym.value:
                label = fsym.name                          # (a recursive call)
                addr = D._sym_address(st, label, taddr)
            else:
                # the object's own label there (a function, a static the data map may name), else
                # the sized object holding the offset
                # (a function before a data label before a bare one; never GCC's object markers)
                rank = {"FUNC": 0, "OBJECT": 1, "NOTYPE": 2}
                labels = sorted((x for x in elf.symbols if x.shndx == s.shndx and x.name
                                 and x.type in rank and x.name not in ("gcc2_compiled.", "__gnu_compiled_c")),
                                key=lambda x: rank[x.type])
                lab = next((x for x in labels if x.value == add), None)
                inner = 0
                if lab is None:
                    lab = next((x for x in labels if x.size and x.value <= add < x.value + x.size), None)
                    inner = add - lab.value if lab is not None else 0
                label = lab.name if lab else (f"{sname}+0x{add:x}" if add else sname)
                if lab is not None:
                    a0 = D._sym_address(st, label, taddr)
                    addr = None if a0 is None else (a0 + inner) & 0xFFFFFFFF
                    label += f"+0x{inner:x}" if inner else ""
        else:
            label = s.name
            addr = D._sym_address(st, s.name, taddr)
            if r.type != "R_MIPS_26" and add:
                label += f"{add:+#x}"
                addr = None if addr is None else (addr + add) & 0xFFFFFFFF
        out[r.offset] = (r.type, label, addr, own)
    return out


# --- normalising one side -------------------------------------------------------------------------

def _dest(mn: str, ops: list[str]) -> str | None:
    if mn in NO_DEST or mn.startswith("b") or not ops:
        return None
    return ops[0] if GPR_NAME.match(ops[0]) else None


def _regs(tok: str) -> str:
    mn, sp, rest = tok.partition(" ")
    return mn + sp + FPR.sub("$f", ALLOC_GPR.sub("r", rest))


@dataclass
class Insn:
    keep: str          # the token, registers kept
    disp: str          # what is printed: the text, the candidate's relocations named
    loose: str         # the token with every offset and lui immediate masked (the second pass's)
    marked: bool       # an immediate masked as the link's (%hi/%lo/%gp_rel), by relocation or rule

    def tok(self, regs: bool) -> str:
        return self.keep if regs else _regs(self.keep)

    def ltok(self, regs: bool) -> str:
        return self.loose if regs else _regs(self.loose)


def normalise(rows, base: int, size: int, st, relnames: dict | None = None) -> list[Insn]:
    """One side's [(address, word, text)] as Insns: `relnames` (the candidate's reloc_names,
    offsets from `base`), or None for the target, read by the link's rule alone."""
    out: list[Insn] = []
    hi: set[str] = set()                   # registers holding a %hi
    armed = due = False                    # a call clobbers the temporaries after its delay slot
    for addr, w, text in rows:
        if due:
            hi = {r for r in hi if re.fullmatch(r"s[0-8]|fp", r)}
        due, armed = armed, False
        off = addr - base
        rel = relnames.get(off) if relnames is not None else None
        mn, _, rest = text.partition(" ")
        ops = rest.split(",") if rest else []
        tok = disp = loose = text
        marked = False
        op = w >> 26
        if op in (2, 3):                                   # j / jal
            if rel is not None:
                dest = rel[2]
                inside = rel[3] is not None
                disp = f"{mn} +0x{rel[3]:x}" if inside and mn == "j" else f"{mn} {rel[1]}"
                if dest is not None:
                    row = st.at_addr(dest)
                    callee = row.name if row is not None else f"0x{dest:x}"
                else:
                    callee = rel[1]
            else:
                dest = ((addr + 4) & 0xF0000000) | ((w & 0x03FFFFFF) << 2)
                inside = base <= dest < base + size
                row = st.at_addr(dest)
                callee = row.name if row is not None else f"0x{dest:x}"
                disp = f"{text}  <{callee}>" if row is not None else text
            tok = loose = f"{mn} L" if inside and mn == "j" else f"{mn} {callee}"
            armed = mn == "jal"
            out.append(Insn(tok, disp, loose, False))
            continue
        if mn.startswith("b") and mn != "break" and ops and re.fullmatch(r"0x[0-9a-f]+", ops[-1]):
            tok = loose = f"{mn} {','.join(ops[:-1] + ['L'])}"
        elif (m := LUI.match(text)) is not None:
            imm = int(m.group("imm"), 0)
            loose = f"lui {m.group('rt')},?"
            if (rel is not None and rel[0] == "R_MIPS_HI16") or HI_LO <= imm <= HI_HI:
                tok, marked = f"lui {m.group('rt')},%hi", True
                if rel is not None:
                    disp = f"lui {m.group('rt')},%hi({rel[1]})"
                out.append(Insn(tok, disp, loose, marked))
                hi.add(m.group("rt"))
                continue
        else:
            mm = MEM.match(text)
            m = mm or IMM16.match(text)
            if m is not None:
                tail = f"({m.group('base')})" if mm else ""
                if m.group("base") != "sp":
                    loose = f"{m.group('pre')}?{tail}"
                kind = None
                if rel is not None and rel[0] in ("R_MIPS_LO16", "R_MIPS_GPREL16", "R_MIPS_LITERAL"):
                    kind = "%lo" if rel[0] == "R_MIPS_LO16" else "%gp_rel"
                elif m.group("base") == "gp":
                    kind = "%gp_rel"
                elif m.group("base") in hi:
                    kind = "%lo"
                if kind is not None:
                    tok, marked = f"{m.group('pre')}{kind}{tail}", True
                    if rel is not None:
                        disp = f"{m.group('pre')}{kind}({rel[1]}){tail}"
        d = _dest(mn, ops)
        if d is not None:
            hi.discard(d)
        armed = armed or mn == "jalr"
        if rel is not None and disp == text:
            disp = f"{text}  <{rel[0]} {rel[1]}>"
        out.append(Insn(tok, disp, loose, marked))
    return out


# --- the comparison -------------------------------------------------------------------------------

def compare(obj: Path, symbol: str, row, st, img) -> tuple[list[Insn], list[Insn]]:
    """The target's Insns and the candidate's (SYMBOL in `obj` against `row`)."""
    elf = E.Elf.read(obj)
    cand, relocs, fsym = elf.function(symbol)
    names = reloc_names(elf, fsym, cand, relocs, row.addr, st)
    t = normalise(E.disasm(img.read(row.addr, row.size), row.addr), row.addr, row.size, st, None)
    c = normalise(E.disasm(cand, row.addr), row.addr, len(cand), st, names)
    return t, c


def align(t: list[Insn], c: list[Insn], regs: bool) -> tuple[list[str], list[str], list]:
    """The two token lists and difflib's alignment of them. The target's link is read by a rule
    (a lui of an address and the registers it reaches, in address order), which a lui in another
    block can defeat where the candidate's relocation is certain; so a second pass: inside each
    differing run, an instruction pair the alignment of the LOOSE tokens (every offset masked)
    pairs, one side's immediate masked as the link's, is the same instruction -- both take the
    loose token -- and the alignment is made again."""
    a, b = [x.tok(regs) for x in t], [x.tok(regs) for x in c]
    loose = difflib.SequenceMatcher(None, [x.ltok(regs) for x in t], [x.ltok(regs) for x in c],
                                    autojunk=False)
    for tag, i1, i2, j1, j2 in loose.get_opcodes():
        if tag != "equal":
            continue
        for i, j in zip(range(i1, i2), range(j1, j2)):
            if a[i] != b[j] and (t[i].marked or c[j].marked):
                a[i] = b[j] = t[i].ltok(regs)
    sm = difflib.SequenceMatcher(None, a, b, autojunk=False)
    return a, b, sm.get_opcodes()


def differences(t: list[Insn], c: list[Insn], regs: bool = False):
    """(differences -- structural, or with `regs` any --, the instructions the alignment pairs that
    differ in their registers alone, the alignment's opcodes)."""
    a, b, ops = align(t, c, regs)
    n = sum(max(i2 - i1, j2 - j1) for tag, i1, i2, j1, j2 in ops if tag != "equal")
    nreg = 0
    if not regs:
        for tag, i1, i2, j1, j2 in ops:
            if tag != "equal":
                continue
            for i, j in zip(range(i1, i2), range(j1, j2)):
                # a pair the second pass made equal is compared loose (its immediates masked)
                second = a[i] == t[i].ltok(False) and a[i] != t[i].tok(False)
                if (t[i].loose != c[j].loose) if second else (t[i].keep != c[j].keep):
                    nreg += 1
    return n, nreg, ops


def report(t: list[Insn], c: list[Insn], row, context: int = 2, regs: bool = False) -> tuple[str, int, int]:
    n, nreg, ops = differences(t, c, regs)
    lines = []
    for tag, i1, i2, j1, j2 in ops:
        if tag == "equal":
            continue
        lines.append(f"@@ target +0x{4 * i1:x} (0x{row.addr + 4 * i1:x}) / candidate +0x{4 * j1:x} {tag}")
        lines += [f"   {t[k].disp}" for k in range(max(0, i1 - context), i1)]
        lines += [f"-T {t[k].disp}" for k in range(i1, i2)]
        lines += [f"+C {c[k].disp}" for k in range(j1, j2)]
        lines += [f"   {t[k].disp}" for k in range(i2, min(len(t), i2 + context))]
    what = "differences" if regs else "structural differences"
    summary = f"target {len(t)}, candidate {len(c)}; {what} {n}"
    if not regs and nreg:
        summary += f"; {nreg} instruction(s) differ in their registers alone"
    lines.append(summary)
    return "\n".join(lines), n, nreg


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("src", type=Path)
    ap.add_argument("symbol", help="the function's name in the object (mangled for C++)")
    ap.add_argument("--target", help="map name or address to diff against (default: its scoring row)")
    ap.add_argument("--compiler", help="toolchain (default: the unit's `sc: compiler=`)")
    ap.add_argument("--flags", help="compiler flags (default: the unit's `sc: flags=`)")
    ap.add_argument("--unit", type=Path, help="compile SRC under this unit's directive, its statics as its")
    ap.add_argument("-I", dest="includes", action="append", type=Path, default=[])
    ap.add_argument("--regs", action="store_true", help="keep the registers: only the link is normalised")
    ap.add_argument("-n", "--context", type=int, default=2, help="lines of context (default 2)")
    ap.add_argument("--obj", type=Path, help="an object already built (a --build-dir's): read, not compiled")
    a = ap.parse_args(argv)
    img, st = E.image(), E.symbols()
    obj = a.obj or build(a.src, a.compiler, a.flags, a.includes, a.unit)
    try:
        row = target_row(st, obj, a.symbol, a.target, C.tu_options(a.unit or a.src).get("overlay"))
        t, c = compare(obj, a.symbol, row, st, img)
    finally:
        if not a.obj:
            obj.unlink(missing_ok=True)
    text, n, _ = report(t, c, row, a.context, a.regs)
    print(f"{a.symbol} against {row.name} 0x{row.addr:08X} (0x{row.size:X} B)")
    print(text)
    return 0 if n == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
