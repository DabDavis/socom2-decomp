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


TRIPLES = ("mipsee-gcc-c", "mipsee-mwcc-c")   # the second converts about 1 % more of the misses


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
            if bsrc is not None:
                hit = try_one(tmp, bsrc, sym, real, addr, img, st, suffix=".cpp")
                if hit == "hit":
                    bsrc = "/* sc: c++ */\n" + bsrc
                    if real != sym.name:
                        bsrc = f"/* sc: as={sym.name} {real} */\n" + bsrc
                    return "hit", bsrc
                worst = hit if worst == "build" else worst
        # as C++: its member calls made as member calls, its virtual calls as virtual calls
        cpp = cxx_draft(body, sym.name, img, st)
        if cpp is not None:
            hit = try_one(tmp, cpp, sym, real, addr, img, st, suffix=".cpp")
            if hit == "hit":
                cpp = "/* sc: c++ */\n" + cpp          # (emit's marker; sccheck reads `as=` in the first 3 lines)
                if real != sym.name:
                    cpp = f"/* sc: as={sym.name} {real} */\n" + cpp
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
    """`M2C_FIELD(obj, T **, VPTR)` (or SC_MEMBER) -> (obj, VPTR)."""
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
    return None


def _virtualize(body: str):
    """m2c's `M2C_FIELD(M2C_FIELD(obj, T **, VPTR), R (**)(P...), SLOT)(obj, ...)` -> a call
    of slot SLOT on a class whose vtable pointer is at VPTR: (struct text, body) or None."""
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
            if first == obj:
                this, rest = obj, args[1:]
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


def prototypes(draft: str, img, st, own=None) -> str:
    """Declarations of the draft's callees whose parameters tools/sigs.py can tell, each with
    the return type m2c's draft gave it (what the caller does with the result) -- and, given
    `own` (the drafted symbol), the function itself when its mangled name lists its parameters:
    m2c then types them as declared (a bool, a short) instead of by their use."""
    global _live
    import sigs
    if _live is None:
        _live = sigs.Live(img, st)
    out = []
    if own is not None and os.environ.get("SC_OWN_PROTO", "1") == "1":
        params = sigs.prototype(own.addr, own.name, _live) if sigs.demangle(own.name) else None
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
        params = sigs.prototype(s.addr, s.name, _live)
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


_ADDR_SUM = re.compile(r"\(\((\w+) \* (\d+|0x[0-9A-Fa-f]+)\) \+ (\w+)\)")
_ADDR_SUM2 = re.compile(r"\((\w+) \+ \((\w+) \* (\d+|0x[0-9A-Fa-f]+)\)\)")


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
    if not flags:
        return None
    for v in flags:
        body = re.sub(rf"^    s32 {v};$", f"    bool {v};", body, flags=re.M)
        body = re.sub(rf"\b{v} = 1;", f"{v} = true;", body)
        body = re.sub(rf"\b{v} = 0;", f"{v} = false;", body)
    rets = re.findall(r"\breturn ([^;]+);", body)
    if rets and all(r.strip() in flags or r.strip() in ("0", "1") for r in rets):
        body = re.sub(rf"^s32 ({re.escape(name)}\()", r"bool \1", body, flags=re.M)
        body = body.replace("return 1;", "return true;").replace("return 0;", "return false;")
    return PRELUDE + 'extern "C" {\n' + body + "}\n"


def try_one(tmp: Path, src: str, sym, real: str, addr: int, img, st, suffix: str = ".c") -> str:
    (tmp / f"f{suffix}").write_text(src)
    try:
        D.compile_obj(tmp / f"f{suffix}", tmp / "f.o")
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


def _pointerish(term: str, decl: dict[str, str]) -> bool | None:
    """Whether an expression's leading term is a pointer, by the draft's own declarations."""
    t = term.strip()
    while t.startswith("(") and _group(t, 0) == len(t):
        t = t[1:-1].strip()
    if t.startswith("&"):
        return True
    m = re.match(r"^\(([^()]*\*)\)", t)
    if m:
        return True                                   # a cast to a pointer
    if t.startswith("M2C_FIELD("):
        a = _args(t, len("M2C_FIELD"))
        return None if a is None or len(a[0]) != 3 else a[0][1].count("*") > 1
    if t.startswith("("):                              # (a + b) ...: its first term
        end = _group(t, 0)
        return _pointerish(t[:end], decl) if end > 0 else None
    m = re.match(r"^([A-Za-z_]\w*)\s*(.*)$", t)
    if m and not m.group(2).startswith("("):
        ty = decl.get(m.group(1))
        return None if ty is None else "*" in ty
    if re.match(r"^-?(0x[0-9A-Fa-f]+|\d+)\b", t):
        return False
    return None


def _deref_ints(line: str, decl: dict[str, str]) -> str:
    """`*x` / `*(x + ...)` whose operand leads with an int: a word load through an int pointer."""
    out, i = [], 0
    while i < len(line):
        c = line[i]
        prev = line[:i].rstrip()[-1:] if line[:i].strip() else "("
        if c == "*" and (prev in "(,=!&|?:*" or line[:i].rstrip().endswith("return")) and line[i + 1:i + 2] not in "=":
            j = i + 1
            if line[j:j + 1] == "(":
                end = _group(line, j)
                operand = line[j:end] if end > 0 else ""
            else:
                m = re.match(r"[A-Za-z_]\w*", line[j:])
                operand = m.group(0) if m and line[j + len(m.group(0)):j + len(m.group(0)) + 1] != "(" else ""
            if operand and _pointerish(operand, decl) is False:
                out.append("*(s32 *)" + operand)
                i = j + len(operand)
                continue
        out.append(c)
        i += 1
    return "".join(out)


def _unprototype(text: str, names: set[str]) -> str:
    """Declare these functions without parameter types, when each is an int or a pointer."""
    for name in names:
        m = re.search(rf"^(?P<ret>[^\n(;{{}}]*?\b){re.escape(name)}\((?P<params>[^()\n]*)\);", text, re.M)
        if m and m.group("params").strip() not in ("", "void") and not _WIDE.search(m.group("params")):
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
    ap.add_argument("--redo", action="store_true", help="retry functions src/auto already holds")
    ap.add_argument("--addrs", type=Path, help="only these addresses (one hex VA per line)")
    ap.add_argument("--stale", action="store_true",
                    help="redo the drafts a rename left behind (implies --redo --emit, any size)")
    a = ap.parse_args(argv)
    img, st = E.image(), E.symbols()
    if a.stale:
        a.redo, a.emit, a.max_size = True, True, 0x100000
        want_stale = stale(st)
        print(f"{len(want_stale)} drafts spell a name the map no longer has")
    cands = [s.addr for s in st.symbols
             if img.is_code(s.addr) and a.min_size < s.size <= a.max_size
             and img.segment(s.addr).name != "MAIN" and s.addr % 16 == 0]
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
    with ProcessPoolExecutor(a.jobs) as ex:
        for addr, kind, src in ex.map(attempt, sorted(cands)):
            tally[kind] = tally.get(kind, 0) + 1
            if src:
                (OUT / f"{addr:08X}.c").write_text(src)
                hits[addr] = src
    total = sum(tally.values())
    print(f"{total} tried: " + ", ".join(f"{k} {v}" for k, v in sorted(tally.items())))
    print(f"hit rate {100 * tally.get('hit', 0) / max(total, 1):.1f} %  (hits in {OUT})")
    if a.emit:
        emit(hits, img, st)
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


def stale(st: E.SymbolTable) -> set[int]:
    """Drafts that define a name other than their row's, or call/reference a placeholder
    the map renamed since."""
    names = {s.name for s in st.symbols}
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
        if s is None or not (f"{s.name}(" in txt or f" {s.name} */" in txt) or gone:
            out.add(a)
    return out


def hand_defined() -> set[str]:
    """Function names hand-written source defines (anything under src/ but src/auto)."""
    names = set()
    obj_root = HERE / "build" / "obj"
    for o in obj_root.rglob("*.o"):
        if "auto" in o.relative_to(obj_root).parts:
            continue
        try:
            names |= {f.name for f in E.Elf.read(o).functions()}
        except Exception:
            pass
    return names


def emit(hits: dict[int, str], img: E.Image, st: E.SymbolTable) -> None:
    written = 0
    if not hits:
        print("no new hits")
    for addr, src in sorted(hits.items()):
        seg = img.segment(addr).name.lower()
        d = AUTO / seg
        d.mkdir(parents=True, exist_ok=True)
        name = st.at_addr(addr).name
        ext = ".cpp" if "/* sc: c++ */" in src[:200] else ".c"
        for old in d.glob(f"{addr:08X}.c*"):
            old.unlink()
        (d / f"{addr:08X}{ext}").write_text(HEADER.format(name=name) + src)
        written += 1
    stale = 0
    hand = hand_defined()
    for f in auto_files():
        s = st.at_addr(int(f.stem, 16))
        txt = f.read_text()
        defines = (f"{s.name}(" in txt or f" {s.name} */" in txt) if s is not None else False
        if s is None or s.name in hand or not defines:
            f.unlink()                      # gone, hand-written now, or renamed since (re-emitted below)
            stale += 1
    print(f"emitted {written} into {AUTO}; removed {stale} superseded")


if __name__ == "__main__":
    sys.exit(main())
