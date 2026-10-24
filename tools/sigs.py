#!/usr/bin/env python3
"""
Prototypes for the functions a draft calls, so m2c reads the right argument registers.

    ./tools/sigs.py 0x1E7EF0 GetNode__6CZAnimFib ...     # print what it would declare

m2c sees a call's arguments only in the registers the caller writes before it: an argument
passed straight through (a destructor handing `this` on in a0 and writing only a1 = 0) is
invisible, and its draft calls `f(0)` -- MW then puts the 0 in a0. Given `f(s32, s32)` in a
context file, m2c writes `f(arg0, 0)`. Two sources, in order:

  * the mangled name, which lists every declared parameter -- including the ones the callee
    never reads, which callers still load. MW does NOT mark a static member function (its
    `ProcExpr__16CZAnimExpressionFPf` is static), so whether a member takes `this` is read
    off the code: does it read the register one past its declared integer parameters?
  * the callee's code: the argument registers live at its entry (a0-a3, t0-t3; f12-f19),
    by backward dataflow over its blocks, a call reading what its own callee reads and
    clobbering the caller-saved set. A parameter nobody reads is invisible here.

Integer-class parameters are declared s32 (or their mangled width), pointers `void *` (m2c
casts to what the context says; automatch turns every `void *` into `char *` for MW), floats
f32. A parameter this cannot classify (a double, a class by value, a 128-bit
quadword, a member pointer) makes the whole prototype unknown, and m2c guesses as before.
"""

from __future__ import annotations

import re
import struct
import sys
from functools import lru_cache
from pathlib import Path

HERE = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(HERE))
import scdecomp as E

ARG_GPRS = (4, 5, 6, 7, 8, 9, 10, 11)                  # a0-a3, t0-t3 (EABI)
ARG_FPRS = tuple(range(12, 20))                        # f12-f19
CLOBBER_GPR = {1, 2, 3, *range(4, 16), 24, 25, 31}
CLOBBER_FPR = set(range(0, 20))


def uses_defs(w: int):
    """(gprs read, gprs written, fprs read, fprs written) of one R5900 instruction."""
    op, rs, rt, rd, sa, fn = w >> 26, (w >> 21) & 31, (w >> 16) & 31, (w >> 11) & 31, (w >> 6) & 31, w & 63
    R, W, FR, FW = set(), set(), set(), set()
    if op == 0:
        if fn in (0, 2, 3, 0x38, 0x3A, 0x3B, 0x3C, 0x3E, 0x3F):
            R, W = {rt}, {rd}                          # shifts by an immediate
        elif fn == 8:
            R = {rs}                                   # jr
        elif fn == 9:
            R, W = {rs}, {rd}                          # jalr
        elif fn in (0x0A, 0x0B):
            R, W = {rs, rt, rd}, {rd}                  # movz/movn keep rd otherwise
        elif fn in (0x0C, 0x0D, 0x0F):
            pass
        elif fn in (0x10, 0x12, 0x28):
            W = {rd}                                   # mfhi/mflo/mfsa
        elif fn in (0x11, 0x13, 0x29):
            R = {rs}
        elif fn in (0x1A, 0x1B):
            R = {rs, rt}                               # div/divu
        else:
            R, W = {rs, rt}, {rd}
    elif op == 1:
        R = {rs}
        if rt in (0x10, 0x11, 0x12, 0x13):
            W = {31}
    elif op == 3:
        W = {31}
    elif op in (4, 5, 0x14, 0x15):
        R = {rs, rt}
    elif op in (6, 7, 0x16, 0x17):
        R = {rs}
    elif op in (8, 9, 10, 11, 12, 13, 14, 0x18, 0x19):
        R, W = {rs}, {rt}
    elif op == 15:
        W = {rt}
    elif op in (0x20, 0x21, 0x23, 0x24, 0x25, 0x27, 0x37, 0x1E):
        R, W = {rs}, {rt}
    elif op in (0x22, 0x26, 0x1A, 0x1B):
        R, W = {rs, rt}, {rt}                          # lwl/lwr/ldl/ldr merge into rt
    elif op in (0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x3F, 0x1F):
        R = {rs, rt}
    elif op == 0x31:
        R, FW = {rs}, {rt}                             # lwc1
    elif op == 0x39:
        R, FR = {rs}, {rt}                             # swc1
    elif op in (0x36, 0x3E, 0x2F, 0x33):
        R = {rs}                                       # lqc2/sqc2/cache/pref
    elif op == 0x11:
        if rs == 0:
            FR, W = {rd}, {rt}                         # mfc1
        elif rs == 4:
            R, FW = {rt}, {rd}                         # mtc1
        elif rs == 2:
            W = {rt}
        elif rs == 6:
            R = {rt}
        elif rs == 16:
            ft, fs, fd = rt, rd, sa
            if fn == 4:
                FR, FW = {ft}, {fd}                    # sqrt.s fd, ft
            elif fn in (5, 6, 7, 0x24):
                FR, FW = {fs}, {fd}                    # abs/mov/neg/cvt.w.s
            elif fn in (0x18, 0x19, 0x1A, 0x1B, 0x1E, 0x1F) or fn >= 0x30:
                FR = {fs, ft}                          # into ACC, or a compare
            else:
                FR, FW = {fs, ft}, {fd}
        elif rs == 20:
            FR, FW = {rd}, {sa}                        # cvt.s.w
    elif op == 0x12:
        if rs in (1, 2):
            W = {rt}                                   # qmfc2/cfc2
        elif rs in (5, 6):
            R = {rt}                                   # qmtc2/ctc2
    elif op == 0x1C:
        R, W = {rs, rt}, {rd}                          # MMI, near enough
    R.discard(0)
    W.discard(0)
    return R, W, FR, FW


def _is_branch(w: int) -> bool:
    op, rs, rt = w >> 26, (w >> 21) & 31, (w >> 16) & 31
    return (op in (4, 5, 6, 7, 0x14, 0x15, 0x16, 0x17) or (op == 1 and rt in (0, 1, 2, 3, 0x10, 0x11, 0x12, 0x13))
            or (op in (0x11, 0x12) and rs == 8))


def _target(w: int, va: int) -> int:
    off = w & 0xFFFF
    return va + 4 + ((off - 0x10000 if off & 0x8000 else off) << 2)


class Live:
    """Argument registers live at a function's entry, by backward dataflow."""

    def __init__(self, img: E.Image, st: E.SymbolTable):
        self.img, self.st = img, st
        self.memo: dict[int, tuple[frozenset, frozenset]] = {}
        self.busy: set[int] = set()

    def args(self, addr: int) -> tuple[frozenset, frozenset]:
        if addr in self.memo:
            return self.memo[addr]
        sym = self.st.at_addr(addr)
        if sym is None or addr in self.busy or not self.img.is_code(addr):
            return frozenset(), frozenset()
        self.busy.add(addr)
        try:
            g, f = self._analyse(sym)
        finally:
            self.busy.discard(addr)
        res = (frozenset(g & set(ARG_GPRS)), frozenset(f & set(ARG_FPRS)))
        self.memo[addr] = res
        return res

    def _analyse(self, sym) -> tuple[set, set]:
        n = sym.size // 4
        ws = struct.unpack(f"<{n}I", self.img.read(sym.addr, n * 4))
        lo, hi = sym.addr, sym.addr + n * 4
        starts = {0}
        for i, w in enumerate(ws):
            op = w >> 26
            if _is_branch(w):
                t = _target(w, lo + 4 * i)
                if lo <= t < hi:
                    starts.add((t - lo) // 4)
                starts.add(i + 2)
            elif op == 2 or (op == 0 and (w & 63) == 8):
                starts.add(i + 2)
        starts = sorted(s for s in starts if s < n)
        blocks = [(s, (starts[k + 1] if k + 1 < len(starts) else n)) for k, s in enumerate(starts)]
        index = {s: k for k, (s, _) in enumerate(blocks)}
        succ, exit_live = [], []
        for s, e in blocks:
            out, live = [], (set(), set())
            last = e - 1
            br = e - 2 if e - 2 >= s else None              # a control transfer sits before its delay slot
            w = ws[br] if br is not None else None
            if w is not None and _is_branch(w):
                t = (_target(w, lo + 4 * br) - lo) // 4
                if t in index:
                    out.append(index[t])
                if e in index:
                    out.append(index[e])                     # not taken
            elif w is not None and w >> 26 == 2:
                t = ((lo + 4 * br) & 0xF0000000) | ((w & 0x03FFFFFF) << 2)
                if lo <= t < hi and (t - lo) // 4 in index:
                    out.append(index[(t - lo) // 4])
                else:
                    live = tuple(set(x) for x in self.args(t))   # a tail call passes its args on
            elif w is not None and w >> 26 == 0 and (w & 63) == 8:
                if (w >> 21) & 31 != 31:
                    out.extend(range(len(blocks)))           # a jump table: anywhere
            elif e in index:
                out.append(index[e])
            succ.append(out)
            exit_live.append(live)
        live_in = [(set(), set()) for _ in blocks]
        changed = True
        while changed:
            changed = False
            for k in range(len(blocks) - 1, -1, -1):
                s, e = blocks[k]
                g, f = set(exit_live[k][0]), set(exit_live[k][1])
                for j in succ[k]:
                    g |= live_in[j][0]
                    f |= live_in[j][1]
                for i in range(e - 1, s - 1, -1):
                    w = ws[i]
                    if i - 1 >= s and ws[i - 1] >> 26 == 3:      # the delay slot of a jal: the callee runs after it
                        cg, cf = self.args(((lo + 4 * (i - 1)) & 0xF0000000) | ((ws[i - 1] & 0x03FFFFFF) << 2))
                        g = (g - CLOBBER_GPR) | set(cg)
                        f = (f - CLOBBER_FPR) | set(cf)
                    elif i - 1 >= s and ws[i - 1] >> 26 == 0 and (ws[i - 1] & 63) == 9:
                        g -= CLOBBER_GPR                     # jalr: an unknown callee
                        f -= CLOBBER_FPR
                    R, W, FR, FW = uses_defs(w)
                    g = (g - W) | R
                    f = (f - FW) | FR
                if (g, f) != live_in[k]:
                    live_in[k] = (g, f)
                    changed = True
        return live_in[0] if blocks else (set(), set())


# --- the mangled parameter list --------------------------------------------------------

BUILTIN = {"v": "void", "b": "u8", "c": "s8", "s": "s16", "i": "s32", "l": "s64", "x": "s64",
           "f": "f32", "w": "u16"}
UNSIGNED = {"c": "u8", "s": "u16", "i": "u32", "l": "u64", "x": "u64"}


def _name(s: str, i: int) -> tuple[str, int] | None:
    m = re.match(r"\d+", s[i:])
    if not m:
        return None
    n = int(m.group())
    j = i + len(m.group())
    return s[j:j + n], j + n


def _qual(s: str, i: int) -> tuple[str, int] | None:
    if s[i:i + 1] == "Q":
        k = int(s[i + 1])
        i += 2
        parts = []
        for _ in range(k):
            r = _name(s, i)
            if r is None:
                return None
            parts.append(r[0])
            i = r[1]
        return "::".join(parts), i
    return _name(s, i)


def _type(s: str, i: int) -> tuple[str, int] | None:
    """-> (C type or 'ABORT', next index)"""
    ptr = False
    unsigned = signed = False
    while i < len(s) and s[i] in "CVUSPRA":
        c = s[i]
        if c in "PR":
            ptr = True
        elif c == "U":
            unsigned = True
        elif c == "S":
            signed = True
        elif c == "A":
            m = re.match(r"A\d+_", s[i:])
            if not m:
                return None
            ptr = True
            i += len(m.group()) - 1
        i += 1
    if i >= len(s):
        return None
    c = s[i]
    if c == "F":                                        # a function type (behind a pointer)
        depth, i = 0, i + 1
        while i < len(s):
            r = _type(s, i)
            if r is None:
                return None
            i = r[1]
            if i < len(s) and s[i] == "_":
                r = _type(s, i + 1)
                return ("void *", r[1]) if r else None
        return None
    if c in BUILTIN:
        t = BUILTIN[c]
        if unsigned and c in UNSIGNED:
            t = UNSIGNED[c]
        elif signed and c == "c":
            t = "s8"
        return ("void *" if ptr else t), i + 1
    if c.isdigit() or c == "Q":
        r = _qual(s, i)
        if r is None:
            return None
        # a class by value is an enum here as often as not (-enum min): an int
        return ("void *" if ptr else "s32"), r[1]
    if c in "dr1M":
        return ("void *" if ptr else "ABORT"), i + 1 if c != "M" else len(s)
    return None


def demangle(name: str) -> tuple[bool, list[str]] | None:
    """(is a member, [C type of each declared parameter]) or None."""
    if "<" in name or "@" in name:
        return None
    m = re.match(r"^(__ct__|__dt__)(?=[0-9Q])", name) or \
        re.match(r"^([A-Za-z_][A-Za-z0-9_]*?)__(?=[0-9QF])", name)
    if not m:
        return None
    i = m.end()
    member = False
    if name[i] != "F":
        r = _qual(name, i)
        if r is None:
            return None
        member, i = True, r[1]
        if name[i:i + 1] == "C":
            i += 1
    if name[i:i + 1] != "F":
        return None
    i += 1
    params = []
    while i < len(name):
        c = name[i]
        if c == "T":                                     # repeat parameter n (1-based)
            mm = re.match(r"T(\d)", name[i:])
            if not mm or int(mm.group(1)) - 1 >= len(params):
                return None
            params.append(params[int(mm.group(1)) - 1])
            i += 2
            continue
        if c == "N":                                     # N<count><index>: repeat
            mm = re.match(r"N(\d)(\d)", name[i:])
            if not mm or int(mm.group(2)) - 1 >= len(params):
                return None
            params.extend([params[int(mm.group(2)) - 1]] * int(mm.group(1)))
            i += 3
            continue
        if c == "e":
            return None                                  # varargs
        r = _type(name, i)
        if r is None:
            return None
        t, i = r
        if t == "ABORT":
            return None
        params.append(t)
    if params == ["void"]:
        params = []
    if m.group(1) == "__dt__":
        params = ["s16"]                                 # MW's destructor takes a delete flag
    if m.group(1) in ("__ct__", "__dt__"):
        member = True
    return member, params


def prototype(addr: int, name: str, live: Live) -> list[str] | None:
    """The C parameter types to declare for the function at addr, or None for unknown."""
    g, f = live.args(addr)
    d = demangle(name)
    if d is not None:
        member, params = d
        if member:
            ints = sum(1 for p in params if p != "f32")
            # MW does not mark a static member: it has `this` if it reads the register past
            # its declared integer parameters, or reads nothing that says otherwise
            static = ints < len(ARG_GPRS) and g and ARG_GPRS[ints] not in g and \
                max(ARG_GPRS.index(r) for r in g) < ints
            if not static:
                params = ["void *"] + params
        return params
    if not E.is_placeholder(name):
        return None
    ni = max((ARG_GPRS.index(r) + 1 for r in g), default=0)
    nf = max((ARG_FPRS.index(r) + 1 for r in f), default=0)
    return ["s32"] * ni + ["f32"] * nf


def main(argv=None) -> int:
    img, st = E.image(), E.symbols()
    live = Live(img, st)
    for x in (argv or sys.argv[1:]):
        s = st.at_addr(int(x, 16)) if x.startswith("0x") else st.get(x)
        g, f = live.args(s.addr)
        print(f"{s.addr:#x} {s.name}: live {sorted(g)} {sorted(f)} -> {prototype(s.addr, s.name, live)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
