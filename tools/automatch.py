#!/usr/bin/env python3
"""
Try m2c's draft of each function as-is: compile it with the settled toolchain and see
whether it is already byte-exact. An inventory, not source: a hit says the function
is trivially matchable and hands over a correct (if ugly) starting body.

    ./tools/automatch.py --max-size 0x40 --sample 200     # a random sample of small functions
    ./tools/automatch.py --range 0x306000 0x30A000        # every function in a range

Writes build/automatch/<VA>.c for every hit and prints the hit rate. With --emit it
also (re)writes src/auto/<segment>/<VA>.c, one file per function -- m2c's prototypes for
two functions can disagree, so they never share a translation unit. A function that
hand-written source already defines (or whose map row has a real name) is skipped, and
an auto file whose function is no longer a placeholder is deleted: src/auto is the
GENERATED tier, counted apart by sccheck, and hand-written source always supersedes it.
"""

from __future__ import annotations

import argparse
import os
import random
import re
import subprocess
import sys
from concurrent.futures import ProcessPoolExecutor
from pathlib import Path

HERE = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(HERE))
sys.path.insert(0, str(HERE / "tools"))
import scdecomp as E
import scdiff as D
import sccheck
import m2c as M

OUT = HERE / "build" / "automatch"
PRELUDE = """typedef signed char s8; typedef unsigned char u8; typedef short s16; typedef unsigned short u16;
typedef int s32; typedef unsigned int u32; typedef long s64; typedef unsigned long u64;
typedef float f32; typedef double f64; typedef int M2C_UNK; typedef int M2C_UNK32;
#define M2C_FIELD(expr, type, offset) (*(type)((char *)(expr) + (offset)))
#define SC_MEMBER(base, type, offset) (*(type)((base) + (offset)))
#define M2C_ERROR(x) 0
#define NULL 0
"""
# A GCC draft's prelude adds what m2c writes for GNU C and the prelude never defined (GCC round
# 16a: 20 of the lane's 87 build failures): M2C_BITWISE, a register's bits as another type -- the
# union newlib's fdlibm spells GET_FLOAT_WORD/SET_FLOAT_WORD with, which GCC turns into the
# `mfc1`/`mtc1` the code holds -- the 128-bit types (TImode, Sony's u_long128) and the R5900's
# `sync`. MW's drafts keep PRELUDE as it was.
GCC_PRELUDE = PRELUDE + """#define M2C_BITWISE(type, expr) ({ union { __typeof__(expr) _f; type _t; } _u; _u._f = (expr); _u._t; })
#define M2C_SYNC() __asm__ __volatile__ ("sync.l")
typedef int s128 __attribute__((mode(TI))); typedef unsigned int u128 __attribute__((mode(TI)));
"""


TRIPLES = ("mipsee-gcc-c", "mipsee-mwcc-c")   # the second converts about 1 % more of the misses

# The GCC front (README "The GCC front"): the prebuilt libraries, each built with its vendor's
# EE-GCC, identified on its own functions with tools/ccsweep.py. A row in one of these ranges
# -- 8-aligned or not, MAIN included -- is drafted for GCC and compiled with that range's
# toolchains in order (MAIN interleaves Sony's own libraries, built with 2.9-ee-991111, and the
# toolchain's newlib and libm, built with 2.96: the first is the range's own, the second for the
# other's pieces); its draft carries the directive of the one that matched.
# (lo, hi, [(toolchain, flags), ...], what)
# the SDK's own build is dtls13010 (README "The GCC front": libvu0's inline asm, the broken sibling
# call); its C compiles as 991111's but where that sibling call shows, so it goes first. The runtime
# (newlib's libc and libm, libgcc) was built with GCC's CFLAGS_FOR_TARGET, -g -O2 (SCE's patch adds
# -G0): under -g GNU as fills none of the delay slots GCC leaves it (a double's move), cc1's output
# the same (GCC round 23a)
SDK, LIBC = ("ee-gcc2.9-991111-01-dtls13010", "-O2 -G0"), ("ee-gcc2.96", "-O2 -G0 -g")
GCC_RANGES = [
    (0x18DB90, 0x191410, [SDK, LIBC], "MAIN: libcdvd, libdma (Sony's SDK 2.7)"),
    (0x191410, 0x1A13D0, [LIBC, SDK], "MAIN: newlib's libc, libgcc and fp-bit"),
    (0x1A13D0, 0x1AD088, [SDK, LIBC], "MAIN: libgraph, libipu, libkernl"),
    (0x1AD088, 0x1B3970, [LIBC, SDK], "MAIN: newlib's libm"),
    (0x1B3970, 0x1C4CC0, [SDK, LIBC], "MAIN: libmc, libmpeg, libmrpc, libpad2, libscf, libvu0 ..."),
    # (0x1CCD78-0x1CE2A4, "the code after zlib's", is libkernl's .data: the tlbsrc kernel image
    # linked at 0x80075000 and its syscall table, then tlbfunc.c's statics -- no rows since GCC 23a)
    (0x1E7F58, 0x1E8FD4, [("ee-gcc2.96", "-O2 -G0 -fno-strict-aliasing")],
     "OVL1: the microphone input (Sid*), libezcnf/libezctl"),
    # the heap is ScanSoft's (the demo links it inside the recogniser): SearchFor's loop is unrolled
    # by four, byte-exact only with the recogniser's -funroll-loops (src/lib/ovl1/heap.c)
    (0x2430F0, 0x243840, [("ee-gcc2.96", "-O2 -G0 -fno-strict-aliasing -funroll-loops")],
     "OVL1: the recogniser's heap (heap_*)"),
    (0x243840, 0x2484A0, [("ee-gcc2.96", "-O2 -G0")], "OVL1: lgAud, libnetb/inet (and the UDP ring's last two)"),
    # the codec's two libraries were built apart (round 12a): the encoder reaches its scalars and
    # .lit4 floats through $gp (-G8), the decoder reaches the same host callbacks by lui (-G0)
    (0x2484A0, 0x253510, [("ee-gcc2.96", "-O2 -G8")], "OVL1: Nellymoser's encoder (SaseEncVad)"),
    (0x253510, 0x256F48, [("ee-gcc2.96", "-O2 -G0")], "OVL1: Nellymoser's decoder (SaseDec)"),
    (0x256F48, 0x256FD0, [("ee-gcc2.96", "-O2 -G8")], "OVL1: the codec's host-callback setters"),
    (0x4C5400, 0x50DC50, [("ee-gcc2.96", "-O2 -G0 -fno-strict-aliasing -funroll-loops")],
     "OVL2: ScanSoft's recogniser (asrspi, casr)"),
    (0x60FB50, 0x64F720, [("ee-gcc2.96", "-O2 -G0")], "OVL2: Medius, DME and the rt_* libraries"),
]


def gcc_range(addr: int) -> list[tuple[str, str]] | None:
    """The (toolchain, flags) a GCC-built library row is tried with, in order, or None (MW)."""
    for lo, hi, ccs, _ in GCC_RANGES:
        if lo <= addr < hi:
            return ccs
    return None


def gcc_directive(cc: tuple[str, str], rename: str | None = None) -> str:
    """The draft's directive line: its toolchain and flags (and its rename, `as=OLD NEW`)."""
    return f"/* sc: compiler={cc[0]}; flags={cc[1]}" + (f"; as={rename}" if rename else "") + " */\n"


def _on(correction: str) -> bool:
    """Each correction can be switched off (SC_AM_<NAME>=0) to measure it against the rest."""
    return os.environ.get(f"SC_AM_{correction}", "1") != "0"


def attempt(addr: int):
    img, st = E.image(), E.symbols()
    sym = st.at_addr(addr)
    real = sym.name
    if not C_IDENT.match(real):                 # a template instance: draft the placeholder,
        sym = E.Symbol(f"func_{addr:08X}", sym.addr, sym.size, sym.note)   # rename the object
    asm = M.export(img, st, sym)
    tmp = OUT / "work" / f"{addr:08X}"
    tmp.mkdir(parents=True, exist_ok=True)
    (tmp / "f.s").write_text(asm)
    ccs = gcc_range(addr)
    if ccs is not None:
        return attempt_gcc(addr, tmp, sym, real, ccs, img, st)
    worst = "m2c"
    first = {}
    for triple in TRIPLES:
        out = run_m2c(triple, tmp / "f.s")
        if out is None:
            continue
        first[triple] = out
        hit, src = try_draft(tmp, out, sym, real, addr, img, st)
        if hit == "hit":
            return addr, "hit", src
        worst = hit if worst in ("m2c", "build") else worst
    # again, told the callees' prototypes (tools/sigs.py): m2c cannot see an argument passed
    # straight through in its register, nor one the caller loads that the callee ignores
    for triple, out in first.items():
        decls = prototypes(out, img, st, st.at_addr(addr))
        if not decls:
            continue
        (tmp / "ctx.h").write_text(TYPES + decls)
        out2 = run_m2c(triple, tmp / "f.s", tmp / "ctx.h")
        if out2 is None:
            continue
        hit, src = try_draft(tmp, decls + out2, sym, real, addr, img, st)
        if hit == "hit":
            return addr, "hit", src
        worst = hit if worst in ("m2c", "build") else worst
    return addr, worst, None


def attempt_gcc(addr: int, tmp: Path, sym, real: str, ccs: list[tuple[str, str]], img, st):
    """A GCC-built library row: m2c's GNU C draft compiled with its range's toolchains (in order)
    as it stands and in the variants near misses want (`gcc_variants`), then redrafted told its
    callees' prototypes and its own parameters. MW's own corrections -- the member spelling,
    the repair from MW's diagnostics, the bool and C++ stages -- answer MW's quirks and are not
    tried."""
    rename = f"{sym.name} {real}" if real != sym.name else None
    out = run_m2c("mipsee-gcc-c", tmp / "f.s")
    if out is None:
        return addr, "m2c", None
    worst = "build"
    below = stack_below_saves(img, addr, sym.size)

    def tries(draft: str):
        nonlocal worst
        body = fill_params(re.sub(r"\bvoid(\s*\*)", r"char\1", draft), sym.name)
        if _on("GCC_REPAIR"):
            body = gcc_repair(tmp, body, ccs[0]) or body
        for variant in gcc_variants(body, sym.name, below):
            src = (GCC_PRELUDE if _on("GCC_PRELUDE") else PRELUDE) + variant
            for cc in ccs:
                hit = try_one(tmp, src, sym, real, addr, img, st, cc=cc)
                if hit == "hit":
                    return gcc_directive(cc, rename) + src
                worst = hit if worst == "build" else worst
        return None

    src = tries(out)
    if src is None:
        # told the callees' prototypes and its own parameters (by dataflow: a C library name
        # lists none), m2c sees arguments passed straight through (strncpy(arg0 + 0x10, arg1, 64))
        decls = prototypes(out, img, st, st.at_addr(addr), own_dataflow=True)
        if decls:
            (tmp / "ctx.h").write_text(TYPES + decls)
            out2 = run_m2c("mipsee-gcc-c", tmp / "f.s", tmp / "ctx.h")
            if out2 is not None:
                src = tries(decls + out2)
    return (addr, "hit", src) if src is not None else (addr, worst, None)


# GCC refuses less of m2c's GNU C than MW does, but not nothing: a load through an int
# (`*(i * 4 + M2C_FIELD(p, s32 *, 4))`: "invalid type argument of `unary *'") and a call that
# disagrees with the prototype m2c took from another call ("too many/few arguments") were ~420 of
# the ~520 GCC drafts that would not build on 2026-11-02. Each is repaired as MW's are (`_deref_ints`,
# `_unprototype`), from GCC's errors -- never its warnings -- a round at a time.
_GDIAG = re.compile(r"^.*?\bf\.c:(\d+): (?!warning)(.*)$", re.M)


def gcc_build_errors(tmp: Path, src: str, cc: tuple[str, str]) -> dict[int, str] | None:
    (tmp / "f.c").write_text(src)
    try:
        D.compile_obj(tmp / "f.c", tmp / "f.o", cc[0], cc[1].split())
    except RuntimeError as e:
        out: dict[int, str] = {}
        for m in _GDIAG.finditer(str(e)):
            n = int(m.group(1))
            out[n] = (out.get(n, "") + " " + m.group(2).strip()).strip()
        return out or {0: str(e)}
    return None


def gcc_repair(tmp: Path, body: str, cc: tuple[str, str], rounds: int = 3) -> str | None:
    """The draft made to build with GCC, or None (the body unchanged when it already builds).

    A draft left with errors no repair here makes -- an undeclared frame slot, which only
    stack_locals' variant declares -- keeps the repairs made so far (GCC_PARTIAL, GCC round 24a):
    returning None threw them away, and every variant then started from the unrepaired body
    (17 of the lane's 29 build failures were loads through an int this repair rewrites). m2c's
    `saved_reg_<r>` -- a callee-saved register's value at the entry, no variable of the draft's -- is
    declared from GCC's own "undeclared" error (GCC_SAVED_REGS): an int, a float for an FPR."""
    prelude = GCC_PRELUDE if _on("GCC_PRELUDE") else PRELUDE
    base = prelude.count("\n")
    text = body
    for _ in range(rounds):
        errs = gcc_build_errors(tmp, prelude + text, cc)
        if errs is None:
            return text
        lines = text.split("\n")
        decl = _declared(text)
        unproto: set[str] = set()
        saved: set[str] = set()
        for n, msg in sorted((k - base, v) for k, v in errs.items()):
            if not 1 <= n <= len(lines):
                continue
            if "invalid type argument of `unary *'" in msg:
                lines[n - 1] = _deref_ints(lines[n - 1], decl, typed=_on("GCC_TYPED_LOADS"),
                                           nested=_on("GCC_NESTED_LOADS"))
            if "invalid operands to binary -" in msg and _on("GCC_PTRDIFF"):
                lines[n - 1] = _byte_differences(lines[n - 1], decl)
            if "invalid operands to binary +" in msg and _on("GCC_PTRSUM"):
                lines[n - 1] = _byte_sums(lines[n - 1], _with_arrays(text, decl) if _on("GCC_ARRAYS") else decl)
            if "called object is not a function" in msg and _on("GCC_CALLS"):
                lines[n - 1] = _calls_through_ints(lines[n - 1])
            unproto |= set(re.findall(r"too (?:many|few) arguments to function `([A-Za-z_]\w*)'", msg))
            if _on("GCC_SAVED_REGS"):
                saved |= set(re.findall(r"`(saved_reg_\w+)' undeclared", msg))
        new = _declare_saved(_unprototype("\n".join(lines), unproto), saved)
        if new == text:
            break
        text = new
    return text if text != body and _on("GCC_PARTIAL") else None


def _declare_saved(text: str, names: set[str]) -> str:
    """m2c's `saved_reg_<r>` names declared at the top of the draft's function (an FPR's a float)."""
    if not names:
        return text
    m = re.search(r"^[^\n;{}]*\b[A-Za-z_]\w*\([^()]*\)\s*\{\n", text, re.M)
    if m is None:
        return text
    fpr = re.compile(r"saved_reg_f\d")
    decls = "".join("    %s %s;\n" % ("f32" if fpr.match(n) else "s32", n) for n in sorted(names))
    return text[:m.end()] + decls + text[m.end():]


_CAST = re.compile(r"^\((?:const\s+)?(?:unsigned\s+|signed\s+)?(?:struct\s+)?[A-Za-z_]\w*(?:\s*\*)*\)$")


def _operand_left(line: str, i: int) -> int:
    """The start of the operand ending just before line[i] (a binary operator's left side: a
    primary with its postfix calls and subscripts, its prefix casts, `&` and `*`)."""
    j = i
    while j > 0 and line[j - 1] == " ":
        j -= 1
    end = j
    while j > 0 and line[j - 1] in ")]":                 # a group, a call's or a subscript's
        depth, k = 0, j - 1
        while k >= 0:
            if line[k] in ")]":
                depth += 1
            elif line[k] in "([":
                depth -= 1
                if depth == 0:
                    break
            k -= 1
        if k < 0:
            return end
        j = k
    while j > 0 and (line[j - 1].isalnum() or line[j - 1] in "_."):
        j -= 1
    while True:                                          # prefix casts, & and *
        k = j
        while k > 0 and line[k - 1] == " ":
            k -= 1
        if k > 0 and line[k - 1] in "&*" and (k < 2 or not (line[k - 2].isalnum() or line[k - 2] in "_)]")):
            j = k - 1
            continue
        if k > 0 and line[k - 1] == ")":
            depth, m = 0, k - 1
            while m >= 0:
                depth += line[m] == ")"
                depth -= line[m] == "("
                if depth == 0:
                    break
                m -= 1
            if m >= 0 and _CAST.match(line[m:k]):
                j = m
                continue
        return j


def _operand_right(line: str, i: int) -> int:
    """The end of the operand starting at line[i] (a binary operator's right side)."""
    j = i
    while j < len(line) and line[j] == " ":
        j += 1
    while True:                                          # prefix casts and unary operators
        if line[j:j + 1] in ("&", "*", "!", "~") or (line[j:j + 1] == "-" and line[j + 1:j + 2] != "-"):
            j += 1
            continue
        if line[j:j + 1] == "(":
            end = _group(line, j)
            if end > 0 and _CAST.match(line[j:end]):
                j = end
                while j < len(line) and line[j] == " ":
                    j += 1
                continue
        break
    if line[j:j + 1] == "(":
        j = _group(line, j)
        if j < 0:
            return -1
    else:
        m = re.match(r"[A-Za-z_]\w*|0x[0-9A-Fa-f]+|\d+(?:\.\d*)?f?", line[j:])
        if not m:
            return -1
        j += m.end()
    while line[j:j + 1] in ("(", "["):                   # a call's arguments, a subscript
        j = _group(line, j)
        if j < 0:
            return -1
    return j


def _byte_differences(line: str, decl: dict[str, str]) -> str:
    """`a - p` where p is a pointer that a is not compatible with (GCC: "invalid operands to binary
    -"): m2c's `char *` minus an `M2C_UNK *` (`(p - &_iob) >> 4`), meaning the registers' byte
    difference, as `(s32) (a) - (s32) (p)` (round 15a's cheapest repair: 12 of the lane's rows)."""
    for _ in range(8):
        cands = [m.start() for m in re.finditer(r"(?<=[\w)\]]) - (?![-=])", line)]
        done = False
        for k in reversed(cands):
            lo, hi = _operand_left(line, k), _operand_right(line, k + 2)
            if hi < 0 or lo >= k:
                continue
            left, right = line[lo:k].strip(), line[k + 2:hi].strip()
            if left.startswith("(s32) (") or _pointerish(right, decl) is not True:
                continue
            line = line[:lo] + f"((s32) ({left}) - (s32) ({right}))" + line[hi:]
            done = True
            break
        if not done:
            break
    return line


_ARRAY_DECL = re.compile(r"^\s*(?:const\s+)?(?:unsigned\s+)?[A-Za-z_]\w*(?:\s*\*+)?\s*([A-Za-z_]\w*)\[[^\]]*\];", re.M)


def _with_arrays(text: str, decl: dict[str, str]) -> dict[str, str]:
    """`decl` with each array the draft declares typed as the pointer it is in a sum (GCC round 32a:
    m2c's frame `s32 sp[28]` in `M2C_FIELD((sp + var_s3), ...)`, var_s3 a `char *` -- 0x24B2F8's one
    error, a byte sum of the frame and an offset m2c typed a pointer)."""
    out = dict(decl)
    for m in _ARRAY_DECL.finditer(text):
        if m.group(1) in out and "*" not in out[m.group(1)]:
            out[m.group(1)] = out[m.group(1)] + " *"
    return out


def _calls_through_ints(line: str) -> str:
    """`*(x + k)(args)`: m2c's call through a word it loaded (GCC: "called object is not a function";
    libmpeg's _doMC, GCC round 32a) made through a function pointer, `(*(void (**)()) (x + k))(args)`
    -- `int (**)()` where the call's value is used."""
    for _ in range(8):
        done = False
        for m in re.finditer(r"\*\(", line):
            i = m.start()
            if line[i + 2:].startswith(("void (**)()", "int (**)()")):
                continue
            j = _group(line, i + 1)
            if j < 0 or j >= len(line) or line[j] != "(" or re.fullmatch(r"[\w\s]*\*+\s*", line[i + 2:j - 1]):
                continue                                  # not a call, or a cast's load: `*(s32 *)(p)`
            ret = "void" if not line[:i].strip() else "int"
            line = line[:i] + f"(*({ret} (**)()) {line[i + 1:j]})" + line[j:]
            done = True
            break
        if not done:
            break
    return line


def _byte_sums(line: str, decl: dict[str, str]) -> str:
    """`p + q` of two pointers (GCC: "invalid operands to binary +"): m2c's self-relative fixup
    `p->f = p + p->f` (the recogniser's ContextSetGcdData, an offset field turned into a pointer),
    meaning the registers' byte sum, as `(s32) (p) + (s32) (q)` (GCC round 25a)."""
    for _ in range(8):
        cands = [m.start() for m in re.finditer(r"(?<=[\w)\]]) \+ (?![+=])", line)]
        done = False
        for k in reversed(cands):
            lo, hi = _operand_left(line, k), _operand_right(line, k + 2)
            if hi < 0 or lo >= k:
                continue
            left, right = line[lo:k].strip(), line[k + 2:hi].strip()
            if left.startswith("(s32) (") or _pointerish(left, decl) is not True or _pointerish(right, decl) is not True:
                continue
            line = line[:lo] + f"((s32) ({left}) + (s32) ({right}))" + line[hi:]
            done = True
            break
        if not done:
            break
    return line


def stack_below_saves(img, addr: int, size: int) -> int:
    """The bytes of a GCC frame below its register saves (`addiu sp,sp,-F` then `sd/sq/sw r,
    off(sp)`): where its locals live, from sp+0 (the EABI passes no arguments on the stack)."""
    frame, low = 0, None
    for w in E.words(img.read(addr, min(size, 0x40))):
        op, rs, rt = w >> 26, (w >> 21) & 31, (w >> 16) & 31
        if op == 0x09 and rs == 29 and rt == 29 and w & 0x8000:
            frame = 0x10000 - (w & 0xFFFF)
        elif op in (0x2B, 0x3F, 0x1F) and rs == 29 and (16 <= rt <= 23 or rt in (30, 31)):
            low = (w & 0xFFFF) if low is None else min(low, w & 0xFFFF)
    return low if low is not None else frame


def stack_locals(body: str, name: str, below: int) -> str | None:
    """A draft that takes the frame's own address (`f((s32) sp)`, a callee filling a local
    buffer m2c never saw declared), or names a frame slot it never declared, with the buffer
    declared: `sp` as an array the size of the frame below the saves, and each undeclared `spXX`
    slot as its word -- or, when m2c only ever subscripts it (`&sp50[0]`, `(&sp1B0[0])[i]`: an
    array it passes by address), as a pointer to its elements, typed by m2c's own cast of it
    (`(f32 (*)[]) &sp1B0[0]`; GCC round 24a: 9 of the lane's 29 build failures). A slot m2c
    declares as a function pointer (`s32 (*spA0)(...)`) is declared."""
    decl = _declared(body)
    slots = sorted({s for s in re.findall(r"\bsp([0-9A-F]+)\b", body) if f"sp{s}" not in decl
                    and not re.search(rf"\(\*\s*sp{s}\)\s*\(", body)})
    bare = re.search(r"\bsp\b", body)
    if not (bare or (slots and _on("STACK_SLOTS"))) or re.search(r"^\s+\S+\s+\**sp(\[[^\]]*\])?;", body, re.M) or below <= 0:
        return None
    m = re.search(rf"^[^\n]*\b{re.escape(name)}\([^)]*\)\s*\{{\n", body, re.M)
    if m is None:
        return None
    defs = ""
    for s in slots:
        uses = re.findall(rf"\bsp{s}\b(\s*\[)?", body)
        if uses and all(u for u in uses) and _on("STACK_ARRAYS"):
            t = re.search(rf"\(([A-Za-z_]\w*) \(\*\)\[\]\) &sp{s}\[0\]", body)
            defs += f"#define sp{s} (({t.group(1) if t else 's32'} *)((char *)sp + 0x{s}))\n"
        else:
            defs += f"#define sp{s} (*(s32 *)((char *)sp + 0x{s}))\n"
    return defs + body[:m.end()] + f"    s32 sp[{(below + 3) // 4}];\n" + body[m.end():]


def gcc_variants(body: str, name: str, below: int = 0):
    """GCC's own retries, after `variants`: m2c's single exit as the vendors' guard clauses,
    m2c's byte offsets on pointers summed as bytes, a frame buffer m2c left undeclared."""
    for b in variants(body):
        yield b
    seen = set()
    sl = stack_locals(body, name, below) if _on("STACK_LOCALS") else None
    if sl is not None:
        seen.add(sl)
        yield sl
        body = sl
    for f in (guard_returns, return_call, unk_char_ptrs, byte_steps, unk_sums, void_callees):
        if not _on(f.__name__.upper()):
            continue
        v = f(body, name)
        if v is not None and v != body and v not in seen:
            seen.add(v)
            yield v
            g = guard_returns(v, name) if f is not guard_returns else None
            if g is not None and g not in seen:
                seen.add(g)
                yield g


def return_call(body: str, name: str) -> str | None:
    """A void draft ending in a call as returning the call's value: m2c types a function void
    when nothing reads v0 after its last call, but the original handed the value back
    (`return sceCdLayerSearchFile(fp, name, 0);`), and 2.9's tail jump of a void call
    (`j f`, no frame) is not the jal-and-return a value makes."""
    m = re.search(rf"^void ({re.escape(name)}\([^)]*\)\s*\{{\n)", body, re.M)
    if m is None:
        return None
    end = body.find("\n}", m.end())
    if end < 0:
        return None
    lines = body[m.end():end].split("\n")
    k = len(lines) - 1
    while k >= 0 and not lines[k].strip():
        k -= 1
    if k < 0 or not re.fullmatch(r"    [A-Za-z_]\w*\(.*\);", lines[k]):
        return None
    callee = re.match(r"    ([A-Za-z_]\w*)\(", lines[k]).group(1)
    if re.search(rf"^void {re.escape(callee)}\(", body, re.M):
        return None                                   # declared void itself
    lines[k] = "    return " + lines[k][4:]
    out = body[:m.start()] + "s32 " + m.group(1) + "\n".join(lines) + body[end:]
    return re.sub(rf"^void ({re.escape(name)}\([^)]*\);)", r"s32 \1", out, flags=re.M)   # its prototype


def void_callees(body: str, name: str) -> str | None:
    """A callee whose every call discards its value declared void: m2c types an unknown callee
    M2C_UNK (an int), and a call that sets v0 keeps the result register out of the allocation
    after it -- the draft's `lui v1` where the target's is `lui v0` (sceSifExitRpc and _sceFsWaitS,
    byte-exact under 2.9 only with their callees void)."""
    out = body
    for m in re.finditer(r"^((?:M2C_UNK|[su](?:8|16|32)|char \*|void \*) )([A-Za-z_]\w*)(\([^;{]*\);)", body, re.M):
        f = m.group(2)
        if f == name:
            continue
        rest = body[:m.start()] + body[m.end():]
        uses = re.findall(rf"^(.*?)\b{re.escape(f)}\(", rest, re.M)
        if uses and all(re.fullmatch(r"\s+", u) for u in uses):
            out = out.replace(m.group(0), "void " + f + m.group(3), 1)
    return out if out != body else None


def unk_char_ptrs(body: str, name: str) -> str:
    """`M2C_UNK *` as `char *`: m2c steps an unknown pointer in bytes (`var_a0 += 0x334`), which
    C scales by the int M2C_UNK is (func_001BD588)."""
    return re.sub(r"\bM2C_UNK \*", "char *", body)


def byte_steps(body: str, name: str) -> str:
    """`p += N` on a typed pointer local (not char) as a byte step: m2c writes a pointer's
    increment in bytes whatever its type (`s32 *var_s0; ... var_s0 += 4;`, func_0061C5A0)."""
    decl = _declared(body)

    def sub(m):
        t = decl.get(m.group(2), "").replace(" ", "")
        if not t.endswith("*") or t in ("char*", "u8*", "s8*"):
            return m.group(0)
        return f"{m.group(1)}{m.group(2)} = ({t[:-1]} *)((char *){m.group(2)} {m.group(3)} {m.group(4)});"
    return re.sub(r"^(\s+)([A-Za-z_]\w*) ([+-])= (0x[0-9A-Fa-f]+|\d+);$", sub, body, flags=re.M)


def unk_sums(body: str, name: str) -> str:
    """`(i * 4) + &x` on an M2C_UNK x summed as bytes (the other order of `unk_bytes`'s form)."""
    unk = {n for n, t in _declared(body).items() if t.replace(" ", "") == "M2C_UNK"}
    return re.sub(r"\(\(([^()]+)\) \+ &([A-Za-z_]\w*)\)",
                  lambda m: f"((M2C_UNK *)((char *)&{m.group(2)} + ({m.group(1)})))" if m.group(2) in unk
                  else m.group(0), body)


def _blocks(lines: list[str]) -> dict[int, int]:
    """The line that opens the block each `}`-led line closes (brace matching), or -1."""
    stack, opener = [], {}
    for i, l in enumerate(lines):
        s = l.strip()
        for c in s:
            if c == "}":
                opener.setdefault(i, stack.pop() if stack else -1)
            elif c == "{":
                stack.append(i)
    return opener


def guard_returns(body: str, name: str) -> str | None:
    """m2c's single exit -- `v = 2; if (a && b) { ...; v = 0; } return v;` -- as guard clauses:
    `if (!(a)) return 2; if (!(b)) return 2; ...; return 0;`. GCC expands each return straight
    into v0, where a returned local is a pseudo the allocator may put elsewhere (`move v0,v1`
    at the end): the vendors (the rt_* libraries, Nellymoser) wrote guards. None when the
    function is not of that shape."""
    lines = body.split("\n")
    m = re.search(rf"^[^\n]*\b{re.escape(name)}\([^)]*\)\s*\{{$", body, re.M)
    if m is None or "goto " in body:
        return None
    start = body[:m.end()].count("\n") + 1
    end = next((i for i in range(len(lines) - 1, start, -1) if lines[i] == "}"), None)
    if end is None:
        return None
    k = end - 1
    while k > start and not lines[k].strip():
        k -= 1
    rm = re.fullmatch(r"    return ([A-Za-z_]\w*);", lines[k])
    if rm is None:
        return None
    var = rm.group(1)
    # m2c writes a constant result where the scheduler put it (`var = 0; *out = *in;`): moved
    # past the plain statements that follow it in its block, where the source had it
    for i in range(start, k):
        am = re.fullmatch(rf"(\s+){var} = (-?(?:0x[0-9A-Fa-f]+|\d+)U?);", lines[i])
        if not am:
            continue
        ind, j = am.group(1), i + 1
        while j < k and lines[j].startswith(ind) and not lines[j][len(ind):].startswith((" ", "}")) \
                and lines[j].endswith(";") and not re.search(rf"\b{var}\b|[{{}}]|\b(return|goto|break|continue)\b",
                                                             lines[j]):
            j += 1
        if j > i + 1 and lines[j] == ind[:-4] + "}":
            lines = lines[:i] + lines[i + 1:j] + [lines[i]] + lines[j:]
    opener = _blocks(lines)

    def flows_to_end(i: int) -> bool:
        j = i + 1
        while j < k:
            s = lines[j].strip()
            if not s:
                j += 1
                continue
            if s.startswith("}"):
                o = opener.get(j, -1)
                head = lines[o].strip() if o >= 0 else ""
                if not head.startswith(("if ", "if(", "} else", "else")):
                    return False                     # a loop's or a switch's end goes elsewhere
                if s == "}":
                    j += 1
                    continue
                if s.startswith("} else"):           # the branch is done: past the whole chain
                    depth = 1
                    j += 1
                    while j < k and depth:
                        t = lines[j].strip()
                        depth += t.count("{") - t.count("}")
                        j += 1
                    continue
                return False
            return False
        return j == k

    out = list(lines)
    for i in range(start, k):
        am = re.fullmatch(rf"(\s+){var} = (.+);", lines[i])
        if am and flows_to_end(i):
            out[i] = f"{am.group(1)}return {am.group(2)};"
    rest = [i for i in range(start, k) if re.match(rf"\s+{var} = ", out[i])]
    # the guard shape: one remaining `var = C;` at the top level, then statements, then a final if
    if len(rest) != 1 or not out[rest[0]].startswith(f"    {var} = "):
        return "\n".join(out) if out != lines else None
    a = rest[0]
    c = re.fullmatch(rf"    {var} = (.+);", out[a]).group(1)
    j = k - 1
    while j > a and not out[j].strip():
        j -= 1
    o = opener.get(j, -1)
    if out[j] != "    }" or o <= a or not re.fullmatch(r"    if \((.*)\) \{", out[o]):
        return "\n".join(out) if out != lines else None
    cond = re.fullmatch(r"    if \((.*)\) \{", out[o]).group(1)
    terms, depth, last = [], 0, 0
    for p, ch in enumerate(cond):
        depth += ch == "("
        depth -= ch == ")"
        if depth == 0 and cond[p:p + 4] == " && ":
            terms.append(cond[last:p])
            last = p + 4
    terms.append(cond[last:])
    guards = [f"    if (!({t.strip()})) {{\n        return {c};\n    }}" for t in terms]
    inner = [l[4:] if l.startswith("    ") else l for l in out[o + 1:j]]
    new = out[:a] + out[a + 1:o] + guards + inner
    tail = next((l for l in reversed(inner) if l.strip()), "")
    if not tail.strip().startswith("return "):
        new.append(f"    return {c};")
    used = [l for l in new[start:] if re.search(rf"\b{var}\b", l)]
    if any(not re.fullmatch(rf"\s+(?!return\b)[A-Za-z_]\w*(?: [A-Za-z_]\w*)*\s*\**\s*{var};", l) for l in used):
        return "\n".join(out) if out != lines else None     # still read somewhere: not guards
    return "\n".join(new + out[end:])


def run_m2c(triple: str, asm: Path, ctx: Path | None = None) -> str | None:
    cmd = [sys.executable, str(M.M2C), "-t", triple, "--valid-syntax", str(asm)]
    if ctx is not None:
        cmd += ["--context", str(ctx)]
    r = subprocess.run(cmd, capture_output=True, text=True)
    if r.returncode != 0 or "M2C_ERROR" in r.stdout:
        return None
    return r.stdout


def try_draft(tmp: Path, out: str, sym, real: str, addr: int, img, st) -> tuple[str, str | None]:
    # m2c leans on GNU C's arithmetic on void *, which MW rejects; char * means the same
    # byte offsets and compiles to the same code
    body = fill_params(re.sub(r"\bvoid(\s*\*)", r"char\1", out), sym.name)
    worst = "build"
    # a field of a base that is already a char * read without converting it (members), then as
    # m2c's macro has it: every cast on the base -- even (char *) of a char * -- makes MW keep
    # the field's address in a register, where the original's member access addresses it off
    # the base (and the cast moves registers besides)
    forms = [members(body)] if _on("MEMBERS") else []
    forms += [body] if not forms or forms[0] != body else []
    for form in forms:
        for variant in variants(form):
            src = PRELUDE + variant
            hit = try_one(tmp, src, sym, real, addr, img, st)
            if hit == "hit":
                if real != sym.name:
                    src = f"/* sc: as={sym.name} {real} */\n" + src
                return "hit", src
            worst = hit if worst == "build" else worst
    # MW refused every form of it: repaired from its diagnostics (see repair), and again
    if worst == "build" and _on("REPAIR"):
        fixed = repair(tmp, forms[0])
        for variant in variants(fixed) if fixed is not None else ():
            src = PRELUDE + variant
            hit = try_one(tmp, src, sym, real, addr, img, st)
            if hit == "hit":
                if real != sym.name:
                    src = f"/* sc: as={sym.name} {real} */\n" + src
                return "hit", src
            worst = hit if worst == "build" else worst
    trues = any(w >> 26 == 0x19 and (w >> 21) & 31 == 0 and w & 0xFFFF == 1
                for w in E.words(img.read(addr, sym.size)))
    for body in forms:
        # as C++ with real bools, when the target materialises a `true` (daddiu r,zero,1)
        if trues:
            bsrc = bool_draft(body, sym.name)
            bret = bool_returns(bsrc, sym.name) if bsrc is not None and _on("BOOL") else None
            for bsrc in [x for x in (bsrc, bret) if x is not None]:
                hit = try_one(tmp, bsrc, sym, real, addr, img, st, suffix=".cpp")
                if hit == "hit":
                    # one directive line: emit's HEADER takes two and sccheck reads three
                    bsrc = (f"/* sc: c++; as={sym.name} {real} */\n" if real != sym.name
                            else "/* sc: c++ */\n") + bsrc
                    return "hit", bsrc
                worst = hit if worst == "build" else worst
        # as C++: its member calls made as member calls, its virtual calls as virtual calls
        cpp = cxx_draft(body, sym.name, img, st)
        if cpp is not None:
            hit = try_one(tmp, cpp, sym, real, addr, img, st, suffix=".cpp")
            if hit == "hit":
                # emit's marker and the rename on one line (HEADER takes two; sccheck reads three)
                cpp = (f"/* sc: c++; as={sym.name} {real} */\n" if real != sym.name
                       else "/* sc: c++ */\n") + cpp
                return "hit", cpp
            worst = hit if worst == "build" else worst
    return worst, None


def members(body: str) -> str:
    """Each M2C_FIELD whose base is already a char * -- declared one, a char * field, a char *
    plus an int, a call returning one -- as SC_MEMBER, which adds the offset to the base as it
    is. The same length, so nested fields keep their places."""
    decl = {k: v.replace(" ", "") for k, v in _declared(body).items()}
    rets = {m.group("name"): m.group("ret").replace(" ", "")
            for m in re.finditer(r"^(?P<ret>[^\n(;{}]*?)\s*\b(?P<name>[A-Za-z_]\w*)\([^()\n]*\);", body, re.M)}
    out, i = list(body), 0
    while True:
        k = body.find("M2C_FIELD(", i)
        if k < 0:
            break
        a = _args(body, k + len("M2C_FIELD"))
        if a is not None and len(a[0]) == 3 and _char_ptr(a[0][0], decl, rets):
            out[k:k + 9] = list("SC_MEMBER")
        i = k + 1
    return "".join(out)


def _char_ptr(e: str, decl: dict[str, str], rets: dict[str, str]) -> bool:
    e = e.strip()
    while e.startswith("(") and _group(e, 0) == len(e):
        e = e[1:-1].strip()
    if re.fullmatch(r"[A-Za-z_]\w*", e):
        return decl.get(e) == "char*"
    for macro in ("M2C_FIELD(", "SC_MEMBER("):
        if e.startswith(macro):
            a = _args(e, len(macro) - 1)
            return a is not None and a[1] == len(e) and len(a[0]) == 3 and a[0][1].replace(" ", "") == "char**"
    m = re.match(r"^([A-Za-z_]\w*)\(", e)
    if m and _group(e, m.end() - 1) == len(e):
        return rets.get(m.group(1)) == "char*"
    # base + int: the leading term a char *, nothing after it a pointer
    depth = 0
    for j, c in enumerate(e):
        if c in "([":
            depth += 1
        elif c in ")]":
            depth -= 1
        elif depth == 0 and c in "+-" and e[j - 1:j] == " " and e[j + 1:j + 2] == " ":
            rest = e[j + 1:]
            return (_char_ptr(e[:j], decl, rets) and "&" not in rest and "M2C_FIELD" not in rest
                    and "SC_MEMBER" not in rest
                    and not any("*" in decl.get(n, "") for n in re.findall(r"\b[A-Za-z_]\w*\b", rest)))
    return False


_DECL = re.compile(r"^\s*(?:extern\s+)?(?!(?:return|goto|case|else|do|break|continue)\b)"
                   r"(?P<type>(?:const\s+)?(?:unsigned\s+)?[A-Za-z_]\w*(?:\s*\*+)?)\s*"
                   r"(?P<name>[A-Za-z_]\w*)(?:\[[^\]]*\])?;", re.M)


def _declared(text: str) -> dict[str, str]:
    """name -> declared type, for the draft's globals, locals and parameters."""
    out = {m.group("name"): m.group("type") for m in _DECL.finditer(text)}
    for m in re.finditer(r"^[^\n;{}]*?\b[A-Za-z_]\w*\(([^()]*)\)\s*\{", text, re.M):
        for p in m.group(1).split(","):
            pm = re.match(r"^\s*(.*?)\s*\b([A-Za-z_]\w*)\s*$", p)
            if pm and pm.group(1):
                out[pm.group(2)] = pm.group(1)
    return out


def _group(s: str, i: int) -> int:
    """s[i] is an opening bracket: the index past its match (or -1)."""
    depth = 0
    for j in range(i, len(s)):
        if s[j] in "([":
            depth += 1
        elif s[j] in ")]":
            depth -= 1
            if depth == 0:
                return j + 1
    return -1


def cxx_draft(body: str, own: str, img, st) -> str | None:
    """The draft as C++ (tools/cxx.py): each call of a mangled, non-static member function
    rewritten as a member call on a class declared to mangle back to that name, each m2c
    virtual call (a function pointer two loads from the object) made on a declared class
    with the vtable pointer at that offset. None when there is nothing to rewrite."""
    global _live
    import cxx
    import sigs
    if _live is None:
        _live = sigs.Live(img, st)
    decls, n = cxx.Decls(), 0
    for m in list(EXTERN.finditer(body)):
        name = m.group("name")
        if name == own:
            continue
        s = _lookup(st, name)
        f = cxx.parse(s.name) if s is not None else None
        if f is None or not f.scope:
            continue
        proto = sigs.prototype(s.addr, s.name, _live)
        if proto is None or len(proto) != len(f.params) + 1:
            continue                                   # static (or unknown): a plain call is right
        new, k = _member_calls(body, name, f)
        if not k:
            continue
        ret = m.group("ret").strip() or "int"
        decls.add(f, "int" if ret == "M2C_UNK" else ret)
        body = new.replace(m.group(0), "")
        n += k
    v = _virtualize(body)
    if v is not None:
        structs, body = v
        n += 1
    else:
        structs = ""
    if not n:
        return None
    try:
        classes = decls.render()
    except cxx.Unsupported:
        return None
    return PRELUDE + classes + structs + 'extern "C" {\n' + body + "}\n"


def _lookup(st, name: str):
    try:
        return st.get(name)
    except KeyError:
        mm = re.match(r"^func_([0-9A-F]{8})$", name)
        return st.at_addr(int(mm.group(1), 16)) if mm else None


def _args(s: str, i: int):
    """s[i] == '(' -> (the top-level arguments, the index past the matching ')')."""
    depth, start, out = 0, i + 1, []
    for j in range(i, len(s)):
        c = s[j]
        if c in "([{":
            depth += 1
        elif c in ")]}":
            depth -= 1
            if depth == 0:
                last = s[start:j].strip()
                if last or out:
                    out.append(last)
                return out, j + 1
        elif c == "," and depth == 1:
            out.append(s[start:j].strip())
            start = j + 1
    return None


def _member_calls(body: str, name: str, f) -> tuple[str, int]:
    """`name(obj, a, b)` -> `((::C *)(obj))->m(a, b)`; pointers, references and enums cast."""
    out, i, k = [], 0, 0
    cls = "::" + "::".join(f.scope)
    pat = re.compile(rf"\b{re.escape(name)}\(")
    while True:
        m = pat.search(body, i)
        if not m:
            break
        a = _args(body, m.end() - 1)
        if a is None or len(a[0]) != len(f.params) + 1:
            out.append(body[i:m.end()])
            i = m.end()
            continue
        args = []
        for p, x in zip(f.params, a[0][1:]):
            d = p.decl()
            if d.endswith("&"):
                args.append(f"*({d[:-1].strip()} *)({x})")
            elif "*" in d or p.by_value:
                args.append(f"({d})({x})")
            else:
                args.append(x)
        out.append(body[i:m.start()])
        out.append(f"(({cls} *)({a[0][0]}))->{f.name}({', '.join(args)})")
        i = a[1]
        k += 1
    out.append(body[i:])
    return "".join(out), k


_FN = re.compile(r"^(?P<ret>.*?)\s*\(\*\*\)\((?P<params>.*)\)$", re.S)


_FIELD = re.compile(r"\b(M2C_FIELD|SC_MEMBER)\(")


def _vtable_load(txt: str) -> tuple[str, int] | None:
    """`M2C_FIELD(obj, T **, VPTR)` (or SC_MEMBER) -> (obj, VPTR); `*obj` -> (obj, 0)."""
    t = txt.strip()
    m = _FIELD.match(t)
    if m:
        a = _args(t, m.end() - 1)
        if a is None or a[1] != len(t) or len(a[0]) != 3 or a[0][1].count("*") < 2:
            return None
        try:
            return a[0][0], int(a[0][2], 0)
        except ValueError:
            return None
    if t.startswith("*") and not t.startswith("**") and _on("VIRT"):
        return t[1:].strip(), 0
    return None


def _virtualize(body: str):
    """m2c's `M2C_FIELD(M2C_FIELD(obj, T **, VPTR), R (**)(P...), SLOT)(obj, ...)` -> a call
    of slot SLOT on a class whose vtable pointer is at VPTR: (struct text, body) or None.
    The vtable pointer at 0 reads `*obj`; a member object's virtual is called on `obj + K`
    (its vtable pointer VPTR - K into it); and a call m2c gives no arguments passes the
    object on in a0, which m2c cannot see."""
    classes, out, i, n = {}, [], 0, 0
    for fm in _FIELD.finditer(body):
        k = fm.start()
        if k < i:
            continue
        outer = _args(body, fm.end() - 1)
        ok = outer is not None and len(outer[0]) == 3
        if ok:
            (inner_txt, fntype, slot), end = outer
            m = _FN.match(fntype)
            vt = _vtable_load(inner_txt)
            ok = vt is not None and m is not None and body[end:end + 1] == "("
        if ok:
            obj, vptr_off = vt
            call = _args(body, end)
            ok = call is not None
        if ok:
            args = call[0]
            first = args[0].strip() if args else None
            while first and first.startswith("(") and _group(first, 0) == len(first):
                first = first[1:-1].strip()
            sub = re.fullmatch(rf"{re.escape(obj)} \+ (0x[0-9A-Fa-f]+|\d+)", first or "")
            if first == obj:
                this, rest = obj, args[1:]
            elif sub and int(sub.group(1), 0) <= vptr_off and _on("VIRT"):
                this, rest = args[0], args[1:]
                vptr_off -= int(sub.group(1), 0)
            elif not args and _on("VIRT"):
                this, rest = obj, []
            else:
                ok = False
        if ok:
            try:
                slot_off = int(slot, 0)
            except ValueError:
                ok = False
        if not ok or slot_off < 8 or slot_off % 4:
            continue
        params = [x.strip() for x in m.group("params").split(",")][1:]
        classes.setdefault(vptr_off, {}).setdefault(slot_off, (m.group("ret").strip(), params))
        out.append(body[i:k])
        out.append(f"((VC_{vptr_off:X} *)({this}))->v_{slot_off:X}({', '.join(rest)})")
        i = call[1]
        n += 1
    if not n:
        return None
    out.append(body[i:])
    structs = []
    for off, slots in sorted(classes.items()):
        lines = [f"struct VC_{off:X} {{"] + ([f"    char pad[0x{off:X}];"] if off else [])
        for so in range(8, max(slots) + 4, 4):
            if so in slots:
                ret, params = slots[so]
                lines.append(f"    virtual {ret} v_{so:X}({', '.join(x for x in params if x) or 'void'});")
            else:
                lines.append(f"    virtual void v_{so:X}();")
        structs.append("\n".join(lines + ["};"]))
    return "\n".join(structs) + "\n", "".join(out)


TYPES = PRELUDE.split("#define")[0]
EXTERN = re.compile(r"^(?P<ret>[^(\n]*?)\s*\b(?P<name>[A-Za-z_]\w*)\((?P<args>[^)]*)\);\s*/\* extern \*/", re.M)
_live = None


def prototypes(draft: str, img, st, own=None, own_dataflow: bool = False) -> str:
    """Declarations of the draft's callees whose parameters tools/sigs.py can tell, each with
    the return type m2c's draft gave it (what the caller does with the result) -- and, given
    `own` (the drafted symbol), the function itself when its mangled name lists its parameters
    (or, `own_dataflow`, by the argument registers live at its entry -- a GCC library's C names
    list none): m2c then types them as declared (a bool, a short) instead of by their use."""
    global _live
    import sigs
    if _live is None:
        _live = sigs.Live(img, st)
    out = []
    if own is not None and os.environ.get("SC_OWN_PROTO", "1") == "1":
        params = (sigs.prototype(own.addr, own.name, _live, c_names=own_dataflow)
                  if sigs.demangle(own.name) or own_dataflow else None)
        if params is not None and own_dataflow and not sigs.demangle(own.name):
            # dataflow knows how many parameters, m2c their types: m2c's own list (its gaps
            # filled), extended with the integer parameters passed straight through unseen
            own_name = own.name if C_IDENT.match(own.name) else f"func_{own.addr:08X}"
            dm = re.search(rf"\b{re.escape(own_name)}\(([^)]*)\)\s*\{{", fill_params(draft, own_name))
            have = [p.strip() for p in dm.group(1).split(",")] if dm and dm.group(1).strip() not in ("", "void") else []
            if any(not re.search(r"\barg\d$", p) for p in have):
                params = None                          # float or stack parameters: left alone
            else:
                ni = sum(1 for p in params if p != "f32")
                params = [re.sub(r"\s*\barg\d$", "", p) for p in have] + ["s32"] * max(0, ni - len(have))
                if len(params) == len(have):
                    params = None                      # nothing m2c did not already see
        d = re.search(rf"^(?P<ret>[^\n(]*?)\s*\b{re.escape(own.name if C_IDENT.match(own.name) else f'func_{own.addr:08X}')}\(", draft, re.M)
        if params is not None and d:
            name = own.name if C_IDENT.match(own.name) else f"func_{own.addr:08X}"
            args = ", ".join(f"{p} arg{i}" for i, p in enumerate(params)) or "void"
            out.append(f"{d.group('ret').strip()} {name}({args});\n")
    for m in EXTERN.finditer(draft):
        name = m.group("name")
        try:
            s = st.get(name)
        except KeyError:
            s = None
        if s is None:
            mm = re.match(r"^func_([0-9A-F]{8})$", name)
            s = st.at_addr(int(mm.group(1), 16)) if mm else None
        if s is None:
            continue
        params = sigs.prototype(s.addr, s.name, _live, c_names=own_dataflow)
        if params is None:
            continue
        ret = m.group("ret").strip() or "s32"
        args = ", ".join(f"{p} arg{i}" for i, p in enumerate(params)) or "void"
        out.append(f"{ret} {name}({args});\n")
    return "".join(out)


def fill_params(body: str, name: str) -> str:
    """m2c names a parameter by its register (arg0..arg3) and leaves out the unused ones, so
    `f(s32 arg0, s32 arg2)` would pass arg2 in a1: put the skipped ones back, unused."""
    m = re.search(rf"\b{re.escape(name)}\(([^)]*)\)\s*\{{", body)
    if not m or not m.group(1).strip() or m.group(1).strip() == "void":
        return body
    parts = [x.strip() for x in m.group(1).split(",")]
    idx = {}
    for x in parts:
        mm = re.match(r"^(.*?)\barg(\d)$", x)
        if not mm:
            return body                       # floats or stack arguments: leave it alone
        idx[int(mm.group(2))] = x
    full = [idx.get(i, f"s32 arg{i}") for i in range(max(idx) + 1)]
    return body[:m.start(1)] + ", ".join(full) + body[m.end(1):]


def variants(body: str):
    """The draft, then retries each a near-miss survey measured: narrow fields unsigned (a byte
    stored as 0xFF is `li 255` to an unsigned char, `li -1` to a signed one) or signed (`lb`
    against `lbu`); and every `(i * n) + p` address sum the other way round -- MW emits the
    operands of an address addu in the reverse of the source's order, and m2c's choice is
    the opposite of the original's about as often as not (24 drafts missed by that addu)."""
    yield body
    u = re.sub(r"\bs(8|16)\b", r"u\1", body)
    if u != body:
        yield u
    s = re.sub(r"\bu(8|16)\b", r"s\1", body)
    if s != body:
        yield s
    sw = _ADDR_SUM.sub(lambda m: f"({m.group(3)} + ({m.group(1)} * {m.group(2)}))", body)
    sw = _ADDR_SUM2.sub(lambda m: f"(({m.group(2)} * {m.group(3)}) + {m.group(1)})", sw) if sw == body else sw
    if sw != body:
        yield sw
    if _on("NOT"):
        nz = not_zero(body)
        if nz != body:
            yield nz
    if _on("AT"):
        bo = big_offsets(body)
        if bo != body:
            yield bo
    if _on("UNKPTR"):
        ub = unk_bytes(body)
        if ub != body:
            yield ub


_ADDR_SUM = re.compile(r"\(\((\w+) \* (\d+|0x[0-9A-Fa-f]+)\) \+ (\w+)\)")
_ADDR_SUM2 = re.compile(r"\((\w+) \+ \((\w+) \* (\d+|0x[0-9A-Fa-f]+)\)\)")
_EQ0 = re.compile(r" == (?:0|NULL)(?=\s*(?:[),;?:]|&&|\|\|))")


def not_zero(body: str) -> str:
    """Every `x == 0` (or NULL) whose x is a whole operand as `!x`. MW materialises `!x` as
    `sltu r,zero,x; xori r,r,1` and `x == 0` as `xor r,x,zero; sltiu r,r,1`; m2c reads both as
    `== 0`. As a branch's condition they compile alike."""
    out, last = [], 0
    for m in _EQ0.finditer(body):
        j = m.start()
        # the left operand: an identifier, or a bracketed group with the name it calls
        k = j
        if body[k - 1:k] == ")":
            depth = 0
            while k > 0:
                k -= 1
                if body[k] == ")":
                    depth += 1
                elif body[k] == "(":
                    depth -= 1
                    if depth == 0:
                        break
            while k > 0 and (body[k - 1].isalnum() or body[k - 1] == "_"):
                k -= 1
        else:
            while k > 0 and (body[k - 1].isalnum() or body[k - 1] == "_"):
                k -= 1
        if k == j or k < last:
            continue
        before = body[:k].rstrip()
        if not (before.endswith(("(", ",", "!", "&&", "||", "?", ":", "return"))
                or (before.endswith("=") and not before.endswith(("==", "!=", "<=", ">=")))):
            continue                             # part of a wider operand: `a + b == 0`
        out.append(body[last:k])
        out.append(f"!{body[k:j]}")
        last = m.end()
    out.append(body[last:])
    return "".join(out)


_BIG = re.compile(r"(?<![\w.>])([A-Za-z_]\w*) \+ (0x[0-9A-Fa-f]{4,})\b")


def big_offsets(body: str) -> str:
    """`x + 0x1A698` on an int x as a pointer sum: MW builds an offset past 16 bits in $at
    for a pointer, in v0 (the next free temporary) for an int -- a member's address taken
    (`&theHUD->m_letterbox`) where m2c, seeing only the add, typed the base an int."""
    decl = _declared(body)

    def sub(m):
        t = decl.get(m.group(1), "")
        if int(m.group(2), 16) < 0x8000 or not t or "*" in t or t in ("f32", "f64", "s64", "u64"):
            return m.group(0)
        return f"(s32)((char *){m.group(1)} + {m.group(2)})"
    return _BIG.sub(sub, body)


def unk_bytes(body: str) -> str:
    """m2c's offsets from an M2C_UNK's address (`*(&data_X + (i * 4))`) count bytes, where C
    scales them by the int M2C_UNK is to MW (`sll 4` for the target's `sll 2`): summed as bytes."""
    unk = {n for n, t in _declared(body).items() if t.replace(" ", "") == "M2C_UNK"}
    out, i = [], 0
    for m in re.finditer(r"\(&([A-Za-z_]\w*) ([+-]) ", body):
        if m.start() < i or m.group(1) not in unk:
            continue
        end = _group(body, m.start())
        if end < 0:
            continue
        out.append(body[i:m.start()])
        out.append(f"((M2C_UNK *)((char *)&{m.group(1)} {m.group(2)} {body[m.end():end - 1]}))")
        i = end
    out.append(body[i:])
    return "".join(out)


def bool_draft(body: str, name: str) -> str | None:
    """The draft as C++ with its 0/1 locals as bool (and the function's result when it only
    returns those or 0/1): MW's C has no bool, and C++ materialises `true` as `daddiu r,zero,1`
    where an int 1 is `li` -- 11 near misses differed in that word alone."""
    locs = re.findall(r"^    s32 (\w+);$", body, re.M)
    flags = [v for v in locs
             if re.search(rf"\b{v} = [01];", body)
             and not re.search(rf"\b{v} = (?![01];)", body)
             and not re.search(rf"\b{v} (\+|-|\*|/|&|\||\^|<<|>>)=", body)
             and not re.search(rf"\b{v}(\+\+|--)", body)]
    rets = re.findall(r"\breturn ([^;]+);", body)
    # a function that only returns 0 and 1 is a bool one too: MW merges the returns into a
    # value it assigns, and an assigned true is daddiu (func_0027E4C0)
    if not flags and not (_on("BOOL") and rets and all(r.strip() in ("0", "1") for r in rets)):
        return None
    for v in flags:
        body = re.sub(rf"^    s32 {v};$", f"    bool {v};", body, flags=re.M)
        body = re.sub(rf"\b{v} = 1;", f"{v} = true;", body)
        body = re.sub(rf"\b{v} = 0;", f"{v} = false;", body)
        if _on("BOOL"):
            # tested as the bool it is: `flag != 0` narrows it again (andi 0xff) before the branch
            body = re.sub(rf"\b{v} != 0\b", v, body)
            body = re.sub(rf"\b{v} == 0\b", f"!{v}", body)
    if rets and all(r.strip() in flags or r.strip() in ("0", "1") for r in rets):
        body = re.sub(rf"^s32 ({re.escape(name)}\()", r"bool \1", body, flags=re.M)
        body = body.replace("return 1;", "return true;").replace("return 0;", "return false;")
    return PRELUDE + 'extern "C" {\n' + body + "}\n"


def bool_returns(src: str, name: str) -> str | None:
    """A bool draft whose returns become assignments to one bool, returned at the end: MW
    materialises `return true` as `li` but an assigned true as `daddiu` -- the original
    assigned it (or inlined a bool function), as func_0027E4C0 shows."""
    m = re.search(rf"^bool {re.escape(name)}\([^)]*\)\s*\{{\n", src, re.M)
    end = src.rfind("\n}\n}")
    if not m or end < m.end():
        return None
    head, body, tail = src[:m.end()], src[m.end():end], src[end:]
    rets = re.findall(r"\breturn ([^;]+);", body)
    if not rets or not all(r.strip() in ("true", "false") or re.fullmatch(r"\w+", r.strip()) for r in rets):
        return None
    body = re.sub(r"\breturn ([^;]+);", r"{ sc_ret = \1; goto sc_return; }", body)
    return head + "    bool sc_ret;\n" + body + "\nsc_return:\n    return sc_ret;" + tail


def try_one(tmp: Path, src: str, sym, real: str, addr: int, img, st, suffix: str = ".c",
            cc: tuple[str, str] | None = None) -> str:
    (tmp / f"f{suffix}").write_text(src)
    try:
        if cc is None:
            D.compile_obj(tmp / f"f{suffix}", tmp / "f.o")
        else:                                   # a GCC-built library row (GCC_RANGES)
            D.compile_obj(tmp / f"f{suffix}", tmp / "f.o", cc[0], cc[1].split())
        if real != sym.name:
            import sccheck
            sccheck.rename_symbol(tmp / "f.o", sym.name, real)
        res = D.compare(tmp / "f.o", real, st.at_addr(addr), img, st)
    except Exception:
        return "build"
    return "hit" if res.match else "miss"


# --- a draft MW will compile ---------------------------------------------------------------
#
# m2c writes GNU C, where an int passed or assigned as a pointer (and back), a pointer of another
# type, a call that disagrees with the prototype m2c inferred from another call, or a load through
# an int are warnings at worst -- and errors in MW's C. A draft that will not build is repaired
# from MW's own diagnostics and compiled again, each repair a conversion the code makes anyway:
# an explicit cast to the type the implicit conversion names, an extern (or a function pointer)
# declared without parameter types when every parameter is an int or a pointer (the call
# compiles the same: same registers, no promotion), MW's relaxed pointer rules (`mpwc_relax`),
# a word load through an int made through an int pointer. The byte compare decides, as ever.

RELAX = "#pragma mpwc_relax on\n"
_DIAG = re.compile(r"^.*?\bf\.c(?:pp)?:(\d+): (.*)$", re.M)
_WIDE = re.compile(r"\b(f32|f64|s64|u64|float|double|long|s128|u128)\b|\.\.\.")


def diagnostics(msg: str) -> dict[int, str]:
    """MW's errors by line, a message's continuation lines joined."""
    out: dict[int, str] = {}
    for m in _DIAG.finditer(msg):
        n = int(m.group(1))
        out[n] = (out.get(n, "") + " " + m.group(2).strip()).strip()
    return out


def build_errors(tmp: Path, src: str, suffix: str = ".c") -> dict[int, str] | None:
    (tmp / f"f{suffix}").write_text(src)
    try:
        D.compile_obj(tmp / f"f{suffix}", tmp / "f.o")
    except RuntimeError as e:
        return diagnostics(str(e)) or {0: str(e)}
    return None


def repair(tmp: Path, body: str, rounds: int = 4) -> str | None:
    """The draft made to compile (MW's diagnostics fixed a round at a time), or None."""
    text = RELAX + body
    base = PRELUDE.count("\n")
    for _ in range(rounds):
        errs = build_errors(tmp, PRELUDE + text)
        if errs is None:
            return text
        new = _fix(text, {n - base: m for n, m in errs.items()})
        if new == text:
            return None
        text = new
    return None


def _assignment(line: str) -> tuple[str, str] | None:
    """`lhs = rhs;` at the top level -> (lhs, rhs)."""
    s = line.rstrip()
    if not s.endswith(";"):
        return None
    depth = 0
    for i, c in enumerate(s):
        if c in "([":
            depth += 1
        elif c in ")]":
            depth -= 1
        elif c == "=" and depth == 0 and s[i + 1:i + 2] != "=" and s[i - 1:i] not in tuple("=!<>+-*/%&|^"):
            return s[:i].rstrip(), s[i + 1:-1].strip()
    return None


def _pointerish(term: str, decl: dict[str, str], loads: bool = False, nested: bool = False) -> bool | None:
    """Whether an expression's leading term is a pointer, by the draft's own declarations (with
    `loads`, GCC's repair: a leading load through a cast pointer, `*(s32 *)p * 4`, by its type; with
    `nested`, GCC round 25a, any leading load whose pointer's type the draft declares -- `*var_a7 *
    4`, var_a7 an `s32 *`: an int, so `*((*var_a7 * 4) + temp_s0)` is a load through an int)."""
    t = term.strip()
    while t.startswith("(") and _group(t, 0) == len(t):
        t = t[1:-1].strip()
    if t.startswith("&"):
        return True
    m = re.match(r"^\(([^()]*\*)\)", t)
    if m:
        return True                                   # a cast to a pointer
    if re.match(r"^\((?:unsigned\s+|signed\s+)?(?:[su](?:8|16|32|64)|int|short|char|long|M2C_UNK(?:32)?)\)", t):
        return False                                  # a cast to an integer: `*((s32) p | 0x20000000)`
    if loads and (m := re.match(r"^\*\s*\(([^()]*?)\s*\*\)", t)):
        return "*" in m.group(1)                      # a load: a pointer iff its type is one
    if nested and t.startswith("*") and (ty := _term_type(t, decl)) is not None:
        return "*" in ty                              # a load through a declared pointer: its pointee
    if t.startswith("M2C_FIELD("):
        a = _args(t, len("M2C_FIELD"))
        return None if a is None or len(a[0]) != 3 else a[0][1].count("*") > 1
    if t.startswith("("):                              # (a + b) ...: its first term
        end = _group(t, 0)
        return _pointerish(t[:end], decl, loads, nested) if end > 0 else None
    m = re.match(r"^([A-Za-z_]\w*)\s*(.*)$", t)
    if m and not m.group(2).startswith("("):
        ty = decl.get(m.group(1))
        return None if ty is None else "*" in ty
    if re.match(r"^-?(0x[0-9A-Fa-f]+|\d+)\b", t):
        return False
    return None


_SCALAR = r"(?:unsigned\s+|signed\s+)?(?:[su](?:8|16|32)|f32|int|short|char|M2C_UNK(?:32)?)"


def _deref_ints(line: str, decl: dict[str, str], typed: bool = False, nested: bool = False) -> str:
    """`*x` / `*(x + ...)` whose operand leads with an int: a word load through an int pointer.

    `typed` (GCC's repair, GCC_TYPED_LOADS, round 24a): also a load behind m2c's cast of it,
    `(f32) *((i * 4) + p)` -- m2c's type for a load whose pointer it holds as an int, an `lwc1`
    here -- and the load made as that type, or as the type of the variable it is assigned to
    (`temp_f2 = *(var_a6 + arg0)`); an s32 load converted to the float was an `lw` and a
    `cvt.s.w` the target does not have.

    `nested` (GCC_NESTED_LOADS, GCC round 25a: six of OVL2's sixteen GCC drafts that would not
    build): an operand leading with a load through a declared pointer (`*((*var_a7 * 4) + p)`,
    _pointerish's `nested`), an `M2C_FIELD(...)` operand taken whole (`*M2C_FIELD(arg0, u32 *, 0x68)`:
    a pointer held in a u32 field), and a store through one made as the type of the value m2c casts
    for it (`*M2C_FIELD(...) = (f32) ...;` a float's `swc1`, not an int's conversion)."""
    out, i = [], 0
    assigned = re.match(r"^\s*([A-Za-z_]\w*)\s*=[^=]", line)
    store = _assignment(line) if nested else None
    while i < len(line):
        c = line[i]
        before = line[:i].rstrip()
        prev = before[-1:] if before else "("
        cast = re.search(rf"\(({_SCALAR})\)$", before) if typed and prev == ")" else None
        # (`nested`: after a binary operator too -- `x + *(...)` is a load -- but not a `p++ *`)
        unary = prev in "(,=!&|?:*" or (nested and prev in "+-/%<>^~" and before[-2:] not in ("++", "--", "->"))
        if c == "*" and (unary or cast or before.endswith("return")) and line[i + 1:i + 2] not in "=":
            j = i + 1
            if line[j:j + 1] == "(":
                end = _group(line, j)
                operand = line[j:end] if end > 0 else ""
            else:
                m = re.match(r"[A-Za-z_]\w*", line[j:])
                operand = m.group(0) if m and line[j + len(m.group(0)):j + len(m.group(0)) + 1] != "(" else ""
                if nested and m and m.group(0) == "M2C_FIELD" and line[j + 9:j + 10] == "(":
                    end = _group(line, j + 9)
                    operand = line[j:end] if end > 0 else ""
            if operand and _pointerish(operand, decl, loads=typed, nested=nested) is False:
                ty = "s32"
                if cast:
                    ty = cast.group(1)
                elif typed and assigned and prev == "=" and re.fullmatch(_SCALAR, decl.get(assigned.group(1), "")):
                    ty = decl[assigned.group(1)]
                elif (store and not before and store[0].strip() == "*" + operand
                      and (sc := re.match(rf"^\(({_SCALAR})\)", store[1]))):
                    ty = sc.group(1)
                out.append(f"*({ty} *)" + operand)
                i = j + len(operand)
                continue
        out.append(c)
        i += 1
    return "".join(out)


def _term_type(term: str, decl: dict[str, str]) -> str | None:
    """The declared type of an expression's leading term, when the draft says it: a variable's
    declaration, a cast's type, an `M2C_FIELD(p, T *, off)`'s T, a load `*x`'s pointee -- else None."""
    t = term.strip()
    while t.startswith("(") and _group(t, 0) == len(t):
        t = t[1:-1].strip()
    if t.startswith("*"):
        inner = _term_type(t[1:], decl)
        if inner is None or "*" not in inner:
            return None
        k = inner.rindex("*")
        return (inner[:k] + inner[k + 1:]).strip()
    if t.startswith("M2C_FIELD("):
        a = _args(t, len("M2C_FIELD"))
        if a is None or len(a[0]) != 3 or "*" not in a[0][1]:
            return None
        k = a[0][1].rindex("*")
        return (a[0][1][:k] + a[0][1][k + 1:]).strip()
    if (m := re.match(r"^\(([^()]*)\)\s*[A-Za-z_(*]", t)) and (re.fullmatch(_SCALAR, m.group(1).strip())
                                                         or m.group(1).strip().endswith("*")):
        return m.group(1).strip()                     # a cast
    if t.startswith("("):
        end = _group(t, 0)
        return _term_type(t[:end], decl) if end > 0 else None
    m = re.match(r"^([A-Za-z_]\w*)\b\s*(.?)", t)
    if m and m.group(2) != "(":
        return decl.get(m.group(1))
    return None


# C library functions whose real parameters are all ints and pointers, so that a float or a wide
# type in m2c's prototype is m2c's misreading (newlib's `__assert(const char *, int, const char *)`,
# which m2c gave a fourth f32 from one call's stale $f12: two of the codec's drafts, GCC round 24a)
NARROW_LIBC = {"__assert"}


def _unprototype(text: str, names: set[str]) -> str:
    """Declare these functions without parameter types, when each is an int or a pointer."""
    for name in names:
        m = re.search(rf"^(?P<ret>[^\n(;{{}}]*?\b){re.escape(name)}\((?P<params>[^()\n]*)\);", text, re.M)
        if m and m.group("params").strip() not in ("", "void") and (
                not _WIDE.search(m.group("params")) or (name in NARROW_LIBC and _on("GCC_NARROW_LIBC"))):
            text = text[:m.start("params")] + text[m.end("params"):]
    return text


def _fix(text: str, errs: dict[int, str]) -> str:
    lines = text.split("\n")
    decl = _declared(text)
    funcs = {m.group("name") for m in re.finditer(r"^[^\n(;{}]*?\b(?P<name>[A-Za-z_]\w*)\([^()\n]*\);", text, re.M)}
    unproto: set[str] = set()
    for n, msg in sorted(errs.items()):
        if not 1 <= n <= len(lines):
            continue
        line = lines[n - 1]
        called = {m.group(1) for m in re.finditer(r"\b([A-Za-z_]\w*)\(", line)} & funcs
        if "undefined identifier 's128'" in msg or "undefined identifier 'u128'" in msg:
            lines.insert(0, "typedef int s128 __attribute__((mode(TI))); "
                            "typedef unsigned int u128 __attribute__((mode(TI)));")
            return "\n".join(lines)
        if "illegal implicit conversion" in msg or "does not match" in msg:
            unproto |= called
            # a function pointer's parameter types, likewise
            line = re.sub(r"(\(\*\*?\))\(([^()]*)\)",
                          lambda m: m.group(1) + "()" if not _WIDE.search(m.group(2)) else m.group(0), line)
        if "illegal implicit conversion" in msg:
            m = re.search(r"from '[^']*' to '([^'()]*)'", msg)
            to = m.group(1).strip() if m else None
            if to:
                r = re.match(r"^(\s*)return (.+);$", line)
                a = _assignment(line)
                if r:
                    line = f"{r.group(1)}return ({to})({r.group(2)});"
                elif a and not a[1].startswith(f"({to})("):
                    line = f"{a[0]} = ({to})({a[1]});"
        if "pointer/array required" in msg:
            line = _deref_ints(line, decl)
        if "illegal operand" in msg:
            # an int compared with a pointer: the pointer as an int
            def cmp(m):
                l, op, r = m.group(1), m.group(2), m.group(3)
                pl, pr = _pointerish(l, decl), _pointerish(r, decl)
                if pl is False and pr:
                    return f"{l} {op} (s32){r}"
                if pr is False and pl:
                    return f"(s32){l} {op} {r}"
                return m.group(0)
            line = re.sub(r"(&?[A-Za-z_]\w*) (==|!=|<|>|<=|>=) (&?[A-Za-z_]\w*)", cmp, line)
        lines[n - 1] = line
    return _unprototype("\n".join(lines), unproto)


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--max-size", type=lambda s: int(s, 0), default=0x40)
    ap.add_argument("--min-size", type=lambda s: int(s, 0), default=0)
    ap.add_argument("--sample", type=int, default=0)
    ap.add_argument("--range", nargs=2, type=lambda s: int(s, 16))
    ap.add_argument("--seed", type=int, default=1)
    ap.add_argument("--jobs", type=int, default=7)
    ap.add_argument("--emit", action="store_true", help="write hits into src/auto/")
    ap.add_argument("--no-prune", action="store_true",
                    help="with --emit, write the hits and leave every other draft alone (a lane's run over its own range)")
    ap.add_argument("--redo", action="store_true", help="retry functions src/auto already holds")
    ap.add_argument("--addrs", type=Path, help="only these addresses (one hex VA per line)")
    ap.add_argument("--stale", action="store_true",
                    help="redo the drafts a rename left behind (implies --redo --emit, any size)")
    ap.add_argument("--obj-dir", type=Path, action="append", default=[],
                    help="also read hand-written objects here (a lane's sccheck --build-dir), with build/obj")
    a = ap.parse_args(argv)
    OBJ_DIRS[:] = [d.resolve() for d in a.obj_dir]
    img, st = E.image(), E.symbols()
    if a.stale:
        a.redo, a.emit, a.max_size = True, True, 0x100000
        want_stale = stale(st)
        print(f"{len(want_stale)} drafts spell a name the map no longer has")
    # MW's functions are 16-aligned overlay code; a GCC-built library row (GCC_RANGES) is any
    # row in its range, 8-aligned or not, MAIN's SDK included
    cands = [s.addr for s in st.symbols
             if img.is_code(s.addr) and a.min_size < s.size <= a.max_size
             and (gcc_range(s.addr) is not None
                  or (img.segment(s.addr).name != "MAIN" and s.addr % 16 == 0))]
    if a.range:
        cands = [x for x in cands if a.range[0] <= x < a.range[1]]
    if a.addrs:
        want = {int(l, 16) for l in a.addrs.read_text().split()}
        cands = [x for x in cands if x in want]
    if a.stale:
        cands = [x for x in cands if x in want_stale]
    if a.sample and len(cands) > a.sample:
        cands = random.Random(a.seed).sample(cands, a.sample)
    hand = hand_defined()
    have = {int(f.stem, 16) for f in auto_files()} if not a.redo else set()
    cands = [x for x in cands if st.at_addr(x).name not in hand and x not in have]
    OUT.mkdir(parents=True, exist_ok=True)
    tally = {}
    hits = {}
    verdicts = []
    with ProcessPoolExecutor(a.jobs) as ex:
        for addr, kind, src in ex.map(attempt, sorted(cands)):
            tally[kind] = tally.get(kind, 0) + 1
            verdicts.append(f"0x{addr:08X} {kind}\n")
            if src:
                (OUT / f"{addr:08X}.c").write_text(src)
                hits[addr] = src
    total = sum(tally.values())
    (OUT / "verdicts.txt").write_text("".join(verdicts))      # each row's verdict, for the next look
    print(f"{total} tried: " + ", ".join(f"{k} {v}" for k, v in sorted(tally.items())))
    print(f"hit rate {100 * tally.get('hit', 0) / max(total, 1):.1f} %  (hits in {OUT})")
    if a.emit:
        emit(hits, img, st, prune=not a.no_prune)
    return 0


AUTO = HERE / "src" / "auto"
# a generated draft defines the MAP's name -- a placeholder, or a real mangled name when it
# is a valid C identifier (most MW names are: Exp__5CQuatFPC6CPnt3D), so naming a function
# never costs its generated match; template names (<, >, ,) are left to hand-written source
C_IDENT = re.compile(r"^[A-Za-z_][A-Za-z0-9_]*$")
HEADER = """/* GENERATED by tools/automatch.py: m2c's draft of {name}, byte-exact as it stands
 * (every relocation resolved). Replace it with real source; the generator then deletes it. */
"""


def auto_files():
    return [f for f in AUTO.rglob("*") if f.suffix in (".c", ".cpp")]


def map_history_names(root: Path = HERE) -> set[str]:
    """Every name symbols.txt and data.txt have held, read off git's history of the two files (each
    row a commit removed or rewrote: a rename removes the old row). Empty when there is no history
    to read (no git, a shallow clone or an export) -- the placeholder rule below still applies."""
    try:
        out = subprocess.run(["git", "-C", str(root), "log", "--format=", "-p", "-U0", "--no-renames",
                              "--", "symbols.txt", "data.txt"],
                             capture_output=True, text=True, check=True).stdout
    except (OSError, subprocess.CalledProcessError):
        return set()
    return {m.group(3) for m in (E.SymbolTable._LINE.match(l[1:]) for l in out.splitlines()
                                  if l.startswith("-0x")) if m}


# a draft's code with its comments and string literals taken out: the names it spells as code
_COMMENT_OR_STRING = re.compile(r"/\*.*?\*/|//[^\n]*|\"(?:[^\"\\\n]|\\.)*\"", re.S)


def stale(st: E.SymbolTable, history: set[str] | None = None) -> set[int]:
    """Drafts that define a name other than their row's, or call/reference a name the map renamed
    since: a placeholder its row no longer carries, or a real name the map held once and holds no
    longer (`history`, map_history_names() by default) -- the UI's round 20 integration: 0x2808D0's
    draft went on calling SkbOpen__Fv after the row took its signature (SkbOpen__FP13C2DAnimCmdHdrPf),
    a call no row resolves, which only the full check caught (GCC round 28a)."""
    names = {s.name for s in st.symbols}
    current = names | {s.name for s in st.data}
    left = (map_history_names() if history is None else history) - current
    ident = re.compile(r"[A-Za-z_]\w*$")

    def renamed(r: str) -> bool:
        # a placeholder the map renamed -- unless to a name no C draft can spell (a template's
        # `<,>`), which the draft must go on calling by address
        s = st.at_addr(int(r[5:], 16))
        return s is None or bool(ident.match(s.name))

    out = set()
    for f in auto_files():
        a, txt = int(f.stem, 16), f.read_text()
        s = st.at_addr(a)
        own = f"func_{a:08X}"
        gone = any(r not in names and r != own and renamed(r)
                   for r in re.findall(r"\b(func_[0-9A-F]{8})\b", txt))
        # a real name the map no longer has (placeholders are the rule above's: one whose row took
        # a template's name is still called by address); the draft's own row's names are its own
        code = _COMMENT_OR_STRING.sub(" ", txt)
        gone = gone or any(r in left and not E.is_placeholder(r) and r != own
                           for r in set(re.findall(r"\b[A-Za-z_]\w*\b", code)))
        # a template-named row's draft defines its placeholder and renames it in the object
        # (`/* sc: as=OLD NEW */`, last in a GCC draft's directive): stale when NEW is no longer
        # the map's name
        m = re.search(r"/\* sc: (?:[^*\n]*?; )?as=(\S+) (\S+) \*/", txt)
        renamed_away = m is not None and s is not None and m.group(2) != s.name
        if s is None or not (f"{s.name}(" in txt or f" {s.name} */" in txt) or gone or renamed_away:
            out.add(a)
    return out


# objects a lane built with `sccheck.py --build-dir DIR` (--obj-dir): read with build/obj's, so a
# lane's private build tells hand_defined what its own source defines (round 5a's 40 false hits)
OBJ_DIRS: list[Path] = []


def _hand_objects():
    for obj_root in [HERE / "build" / "obj", *OBJ_DIRS]:
        for o in obj_root.rglob("*.o"):
            if "auto" not in o.relative_to(obj_root).parts:
                yield o


def _object_source(o: Path) -> Path | None:
    """The hand-written unit an object of `_hand_objects` was built from (sccheck.object_path's
    inverse: `x.o` from x.c or x.cpp, `x.s.o` from x.s), or None."""
    for obj_root in [HERE / "build" / "obj", *OBJ_DIRS]:
        try:
            rel = o.relative_to(obj_root)
        except ValueError:
            continue
        stem = rel.with_suffix("")
        for cand in ([stem] if stem.suffix == ".s" else [stem.with_suffix(".c"), stem.with_suffix(".cpp")]):
            if (HERE / "src" / cand).is_file():
                return HERE / "src" / cand
    return None


def hand_defined() -> set[str]:
    """Function names hand-written source defines (anything under src/ but src/auto): its own
    definitions, and the inline copies it emits byte-exact. A copy that misses supersedes no
    draft -- the linker keeps a strong definition over every unit's copy, as sccheck scores it
    (zsnd_csndsequence.cpp's ~__vector_deleter against the draft of 0x34A5A0). A definition of a
    name the map gives a row in each overlay (`ovl-copy:`) is a copy's case: the name of the row
    it matches, in the unit's own overlay (GCC round 25a)."""
    names, copies = set(), {}
    img, st = E.image(), E.symbols()
    for o in _hand_objects():
        try:
            elf = E.Elf.read(o)
            for f in elf.functions():
                if sccheck.is_copy(elf, f) or len(st.overlays_of(f.name)) > 1:
                    # a copy, or a name with a row per overlay (an explicit template instance):
                    # it defines the row it matches, as sccheck scores it
                    copies.setdefault(f.name, []).append(o)
                else:
                    names.add(f.name)
        except Exception:
            pass

    def home(o):
        # the overlay the unit is linked in, by sccheck's rule (its `overlay=` directive when its
        # source is found, else the overlay its own functions' rows lie in): its copies are there
        src = _object_source(o)
        overlay = sccheck.tu_options(src).get("overlay") if src is not None else None
        try:
            return sccheck.unit_overlay(E.Elf.read(o), st, overlay)
        except Exception:
            return None

    for name, objs in copies.items():
        if name not in st._by_name:
            continue
        for o in objs:
            # sccheck scores a copy against its own overlay's row when the map lists one there
            # (`ovl-copy:`): a match there supersedes that row's draft, under the row's name
            sym = sccheck.scoring_row(st, name, home(o))
            if sym.name in names:
                continue
            try:
                if D.compare(o, name, sym, img, st).match:
                    names.add(sym.name)
            except Exception:
                pass
    return names


def emit(hits: dict[int, str], img: E.Image, st: E.SymbolTable, prune: bool = True) -> None:
    written = 0
    if not hits:
        print("no new hits")
    for addr, src in sorted(hits.items()):
        seg = img.segment(addr).name.lower()
        d = AUTO / seg
        d.mkdir(parents=True, exist_ok=True)
        name = st.at_addr(addr).name
        ext = ".cpp" if re.search(r"/\* sc: c\+\+[ ;]", src[:300]) else ".c"
        for old in d.glob(f"{addr:08X}.c*"):
            old.unlink()
        (d / f"{addr:08X}{ext}").write_text(HEADER.format(name=name) + src)
        written += 1
    if not prune:
        print(f"emitted {written} into {AUTO} (no pruning)")
        return
    stale = renamed = 0
    hand = hand_defined()
    for f in auto_files():
        a = int(f.stem, 16)
        s = st.at_addr(a)
        txt = f.read_text()
        defines = (f"{s.name}(" in txt or f" {s.name} */" in txt) if s is not None else False
        own = f"func_{a:08X}"
        if s is not None and s.name not in hand and not defines and f"{own}(" in txt:
            # the row was renamed and its re-draft (if one was tried) missed: keep the draft that
            # matched under the placeholder, renamed -- deleting it loses the match (it did, three
            # times: SlotList's and CWeaponSlot's destructors, HEALTH_PARAMS::Parse)
            if C_IDENT.match(s.name):
                txt = re.sub(rf"\b{own}\b", s.name, txt)
            elif re.search(r"/\* sc: compiler=[^\n]*\*/", txt) and "as=" not in txt:
                # a GCC draft's rename joins its directive line (sccheck reads three lines)
                txt = re.sub(r"(/\* sc: compiler=[^\n]*?) \*/", rf"\1; as={own} {s.name} */", txt, count=1)
            elif re.search(r"^/\* sc: c\+\+ \*/$", txt, re.M) and not re.search(r"/\* sc: [^\n]*\bas=", txt):
                # a C++ draft's marker is its third line: the rename joins it (a line inserted above
                # it pushed the marker past the third line, which sccheck refuses -- 0x2B20E0, 2026-11-04)
                txt = re.sub(r"^/\* sc: c\+\+ \*/$", f"/* sc: c++; as={own} {s.name} */", txt, count=1, flags=re.M)
            elif not re.search(r"/\* sc: [^\n]*\bas=", txt):
                txt = txt.replace("*/\n", f"*/\n/* sc: as={own} {s.name} */\n", 1)
            else:
                f.unlink()
                stale += 1
                continue
            f.write_text(txt)
            renamed += 1
            continue
        if s is None or s.name in hand or not defines:
            f.unlink()                      # gone or hand-written now
            stale += 1
    print(f"emitted {written} into {AUTO}; removed {stale} superseded; renamed {renamed} in place")


if __name__ == "__main__":
    sys.exit(main())
