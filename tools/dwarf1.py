#!/usr/bin/env python3
"""
Read the Metrowerks DWARF 1 debug information of an unstripped PS2 ELF (SOCOM 1's 2002
builds carry it): translation units, functions, and class layouts.

    ./tools/dwarf1.py units ELF                 # compile units: source file, address range, function count
    ./tools/dwarf1.py funcs ELF [regex]         # functions: address, size, TU, name, signature
    ./tools/dwarf1.py types ELF [regex]         # class/struct/union layouts as C++, member offsets annotated
    ./tools/dwarf1.py census ELF                # how much the .debug section describes

DWARF 1 (the .debug section) is a flat list of entries: a 4-byte length, a 2-byte tag,
then attributes, each a 2-byte (name << 4 | form) and a value whose size the form fixes.
An entry's children follow it, up to the offset its AT_sibling names. Entries under 8
bytes are padding. Metrowerks adds its own attributes (0x2000+, the mangled name among
them) and fundamental types (0x8000+); unknown ones are kept raw, never guessed.
"""

from __future__ import annotations

import argparse
import re
import struct
import sys
from collections import Counter, defaultdict
from dataclasses import dataclass, field
from pathlib import Path

HERE = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(HERE))
import scdecomp as E

TAGS = {0x00: "padding", 0x01: "array_type", 0x02: "class_type", 0x03: "entry_point",
        0x04: "enumeration_type", 0x05: "formal_parameter", 0x06: "global_subroutine",
        0x07: "global_variable", 0x0A: "label", 0x0B: "lexical_block", 0x0C: "local_variable",
        0x0D: "member", 0x0F: "pointer_type", 0x10: "reference_type", 0x11: "compile_unit",
        0x12: "string_type", 0x13: "structure_type", 0x14: "subroutine", 0x15: "subroutine_type",
        0x16: "typedef", 0x17: "union_type", 0x18: "unspecified_parameters", 0x19: "variant",
        0x1A: "common_block", 0x1B: "common_inclusion", 0x1C: "inheritance",
        0x1D: "inlined_subroutine", 0x1E: "module", 0x1F: "ptr_to_member_type",
        0x20: "set_type", 0x21: "subrange_type", 0x22: "with_stmt"}

# attribute NUMBERS (the 16-bit value is number << 4 | form), DWARF 1.1 figure 16
ATTRS = {0x001: "sibling", 0x002: "location", 0x003: "name", 0x005: "fund_type",
         0x006: "mod_fund_type", 0x007: "user_def_type", 0x008: "mod_u_d_type", 0x009: "ordering",
         0x00A: "subscr_data", 0x00B: "byte_size", 0x00C: "bit_offset", 0x00D: "bit_size",
         0x00F: "element_list", 0x010: "stmt_list", 0x011: "low_pc", 0x012: "high_pc",
         0x013: "language", 0x014: "member", 0x015: "discr", 0x016: "discr_value",
         0x019: "string_length", 0x01A: "common_reference", 0x01B: "comp_dir",
         0x01C: "const_value", 0x01D: "containing_type", 0x01E: "default_value", 0x01F: "friends",
         0x020: "inline", 0x021: "is_optional", 0x022: "lower_bound", 0x023: "program",
         0x024: "private", 0x025: "producer", 0x026: "protected", 0x027: "prototyped",
         0x028: "public", 0x029: "pure_virtual", 0x02A: "return_addr", 0x02B: "specification",
         0x02C: "start_scope", 0x02E: "stride_size", 0x02F: "upper_bound", 0x030: "virtual",
         # Metrowerks (AT_lo_user = 0x2000)
         0x200: "MW_mangled", 0x201: "MW_restore_SP", 0x202: "MW_global_ref",
         0x203: "MW_global_ref_by_name"}

FUND = {0x1: "char", 0x2: "signed char", 0x3: "unsigned char", 0x4: "short", 0x5: "short",
        0x6: "unsigned short", 0x7: "int", 0x8: "int", 0x9: "unsigned int", 0xA: "long",
        0xB: "long", 0xC: "unsigned long", 0xD: "void *", 0xE: "float", 0xF: "double",
        0x10: "long double", 0x11: "complex", 0x12: "double complex", 0x14: "void",
        0x15: "bool", 0x8008: "long long", 0x8208: "unsigned long long"}
MODS = {0x01: "*", 0x02: "&", 0x03: "const", 0x04: "volatile"}


@dataclass
class Die:
    off: int
    tag: int
    attrs: dict
    children: list = field(default_factory=list)
    parent: "Die | None" = None

    @property
    def kind(self) -> str:
        return TAGS.get(self.tag, f"tag_{self.tag:#x}")

    def get(self, name, default=None):
        return self.attrs.get(name, default)

    @property
    def name(self) -> str:
        return self.attrs.get("name", "")


def parse(data: bytes, base: int = 0) -> tuple[list[Die], dict[int, Die]]:
    """All entries in .debug, linked into trees by their sibling pointers."""
    dies, by_off = [], {}
    o = 0
    while o + 4 <= len(data):
        length = struct.unpack_from("<I", data, o)[0]
        if length < 8:
            # padding / end of a child list
            by_off[o + base] = None
            o += max(length, 4)
            continue
        tag = struct.unpack_from("<H", data, o + 4)[0]
        attrs = {}
        p, end = o + 6, o + length
        while p + 2 <= end:
            an = struct.unpack_from("<H", data, p)[0]
            p += 2
            form, aname = an & 0xF, an >> 4
            if form == 1 or form == 2 or form == 6:           # addr, ref, data4
                v = struct.unpack_from("<I", data, p)[0]; p += 4
            elif form == 3:                                    # block2
                n = struct.unpack_from("<H", data, p)[0]; v = data[p + 2:p + 2 + n]; p += 2 + n
            elif form == 4:                                    # block4
                n = struct.unpack_from("<I", data, p)[0]; v = data[p + 4:p + 4 + n]; p += 4 + n
            elif form == 5:                                    # data2
                v = struct.unpack_from("<H", data, p)[0]; p += 2
            elif form == 7:                                    # data8
                v = struct.unpack_from("<Q", data, p)[0]; p += 8
            elif form == 8:                                    # string
                q = data.index(b"\0", p); v = data[p:q].decode("latin-1"); p = q + 1
            else:
                raise ValueError(f"DIE at {o:#x}: unknown form {form} in attribute {an:#06x}")
            attrs[ATTRS.get(aname, f"at_{aname:#x}")] = v
        d = Die(o + base, tag, attrs)
        dies.append(d)
        by_off[o + base] = d
        o += length
    # children: the entries after a parent and before its sibling
    stack: list[tuple[Die, int]] = []
    for d in dies:
        while stack and d.off >= stack[-1][1]:
            stack.pop()
        if stack:
            d.parent = stack[-1][0]
            stack[-1][0].children.append(d)
        sib = d.get("sibling")
        if sib and sib > d.off:
            # a sibling pointer past the next entry means children follow
            stack.append((d, sib))
    return dies, by_off


def location_text(block: bytes) -> str:
    """A DWARF 1 location expression as text: OP_REG n, OP_BASEREG n + const, OP_ADDR."""
    out, p = [], 0
    while p < len(block):
        op = block[p]; p += 1
        if op in (0x01, 0x02):                       # OP_REG, OP_BASEREG
            r = struct.unpack_from("<I", block, p)[0]; p += 4
            out.append(("reg" if op == 1 else "base") + f" {REGNAMES.get(r, r)}")
        elif op == 0x03:                             # OP_ADDR
            out.append(f"addr {struct.unpack_from('<I', block, p)[0]:#x}"); p += 4
        elif op == 0x04:                             # OP_CONST
            out.append(f"{struct.unpack_from('<i', block, p)[0]}"); p += 4
        elif op == 0x05:
            out.append("deref2")
        elif op == 0x06:
            out.append("deref")
        elif op == 0x07:
            out.append("+")
        else:
            out.append(f"op{op:#x}")
    return " ".join(out)


REGNAMES = {i: n for i, n in enumerate(
    "zero at v0 v1 a0 a1 a2 a3 t0 t1 t2 t3 t4 t5 t6 t7 s0 s1 s2 s3 s4 s5 s6 s7 t8 t9 k0 k1 gp sp fp ra".split())}
REGNAMES.update({32 + i: f"f{i}" for i in range(32)})


def location_offset(block: bytes) -> int | None:
    """A member's AT_location: OP_CONST n (0x04 + u32) OP_ADD (0x07) -> n."""
    if len(block) >= 5 and block[0] == 0x04:
        return struct.unpack_from("<I", block, 1)[0]
    return None


class Debug:
    def __init__(self, path: Path):
        self.elf = E.Elf.read(path)
        sec = {s.name: s for s in self.elf.sections}
        if ".debug" not in sec:
            raise SystemExit(f"{path}: no .debug section (stripped?)")
        self.dies, self.by_off = parse(sec[".debug"].data)

    def array_parts(self, t: Die) -> tuple[str, str]:
        """(element type, "[n][m]...") of an array_type, from its AT_subscr_data: a list of
        FMT_FT_C_C items (fund type, low, high: one dimension each) closed by FMT_ET and the
        element type as an embedded attribute."""
        b = t.get("subscr_data", b"")
        p, dims, elem = 0, [], "?"
        while p < len(b):
            fmt = b[p]
            p += 1
            if fmt == 0x0:                                         # FMT_FT_C_C
                lo, hi = struct.unpack_from("<ii", b, p + 2)
                dims.append(hi - lo + 1)
                p += 10
            elif fmt == 0x8:                                       # FMT_ET
                an = struct.unpack_from("<H", b, p)[0]
                p += 2
                form, aname = an & 0xF, ATTRS.get(an >> 4, "")
                if form == 5:
                    v = struct.unpack_from("<H", b, p)[0]; p += 2
                elif form == 2:
                    v = struct.unpack_from("<I", b, p)[0]; p += 4
                elif form == 3:
                    n = struct.unpack_from("<H", b, p)[0]; v = b[p + 2:p + 2 + n]; p += 2 + n
                else:
                    break
                inner = self.by_off.get(v) if aname == "user_def_type" else None
                if inner is not None and inner.tag == 0x01:        # float m[4][4]: an array OF arrays
                    elem, more = self.array_parts(inner)
                    return elem, "".join(f"[{n}]" for n in dims) + more
                elem = self.typename(Die(0, 0, {aname: v}))
            else:
                break                                              # bounds given by location: rare
        return elem, "".join(f"[{n}]" for n in dims)

    def enum_values(self, e: Die) -> list[tuple[str, int]]:
        """AT_element_list: (value, name) pairs, each value as wide as the enum (MW's -enum min
        makes most of them one byte), signed."""
        b, width = e.get("element_list", b""), e.get("byte_size", 4)
        out, p = [], 0
        while p + width <= len(b):
            v = int.from_bytes(b[p:p + width], "little", signed=True)
            q = b.index(b"\0", p + width)
            out.append((b[p + width:q].decode("latin-1"), v))
            p = q + 1
        return out

    def typename(self, d: Die, name: str = "") -> str:
        """C declaration of `name` with this entry's type."""
        if "user_def_type" in d.attrs:
            t = self.by_off.get(d.get("user_def_type"))
            if t is not None and t.tag == 0x01:                    # an array: the dimensions go after the name
                elem, dims = self.array_parts(t)
                return f"{elem} {name}{dims}".rstrip()
        def ref(off):
            t = self.by_off.get(off)
            if t is None:
                return "?"
            return t.name or f"<{t.kind}@{off:#x}>"
        def with_mods(block: bytes, base_size: int, base_of):
            mods = block[:-base_size]
            base = base_of(block[-base_size:])
            out = base
            for m in reversed(mods):          # modifiers apply innermost-last
                s = MODS.get(m, f"mod{m:#x}")
                out = f"{out} {s}" if s in ("const", "volatile") else f"{out}{s}"
            return out
        if "fund_type" in d.attrs:
            t = FUND.get(d.get("fund_type"), f"fund{d.get('fund_type'):#x}")
        elif "user_def_type" in d.attrs:
            t = ref(d.get("user_def_type"))
        elif "mod_fund_type" in d.attrs:
            t = with_mods(d.get("mod_fund_type"), 2,
                          lambda b: FUND.get(struct.unpack("<H", b)[0], f"fund{struct.unpack('<H', b)[0]:#x}"))
        elif "mod_u_d_type" in d.attrs:
            t = with_mods(d.get("mod_u_d_type"), 4, lambda b: ref(struct.unpack("<I", b)[0]))
        else:
            t = "void"
        return f"{t} {name}".rstrip()

    def units(self):
        return [d for d in self.dies if d.tag == 0x11]

    def lines(self, cu: Die) -> list[tuple[int, int]]:
        """(address, source line) for a unit, from .line at its AT_stmt_list: a 4-byte length,
        a 4-byte base address, then (line 4, column 2, address delta 4) entries to line 0."""
        sec = {x.name: x for x in self.elf.sections}.get(".line")
        off = cu.get("stmt_list")
        if sec is None or off is None:
            return []
        data = sec.data
        length, base = struct.unpack_from("<II", data, off)
        out, p = [], off + 8
        while p + 10 <= off + length:
            line, _col, delta = struct.unpack_from("<IHI", data, p)
            p += 10
            if line == 0:
                break
            out.append((base + delta, line))
        return out

    def dump(self, f: Die, cu: Die | None) -> str:
        """A function's signature, parameters, locals (in lexical blocks) and line table."""
        out = [self.signature(f) + f"   // {f.get('low_pc'):#x}-{f.get('high_pc'):#x}"]

        def walk(d: Die, depth: int):
            for c in d.children:
                if c.tag in (0x05, 0x0C):              # formal_parameter, local_variable
                    loc = location_text(c.get("location", b"")) if "location" in c.attrs else ""
                    kind = "param" if c.tag == 0x05 else "local"
                    out.append("    " * depth + f"{kind} {self.typename(c, c.name)};" + (f"   // {loc}" if loc else ""))
                elif c.tag == 0x0B:                    # lexical_block
                    lo, hi = c.get("low_pc"), c.get("high_pc")
                    out.append("    " * depth + "{" + (f"   // {lo:#x}-{hi:#x}" if lo is not None else ""))
                    walk(c, depth + 1)
                    out.append("    " * depth + "}")
        walk(f, 1)
        if cu is not None:
            lo, hi = f.get("low_pc"), f.get("high_pc")
            rows = [(a, l) for a, l in self.lines(cu) if lo <= a < hi]
            if rows:
                out.append("    lines: " + " ".join(f"{a - lo:#x}:{l}" for a, l in rows))
        return "\n".join(out)

    def functions(self):
        for d in self.dies:
            if d.tag in (0x06, 0x14) and "low_pc" in d.attrs:
                cu = d
                while cu is not None and cu.tag != 0x11:
                    cu = cu.parent
                yield d, cu

    def signature(self, f: Die) -> str:
        params = [self.typename(c, c.name) for c in f.children if c.tag == 0x05]
        if any(c.tag == 0x18 for c in f.children):
            params.append("...")
        return f"{self.typename(f, '')} {f.name}({', '.join(params)})"

    def layout(self, t: Die) -> str:
        kw = {0x02: "class", 0x13: "struct", 0x17: "union"}[t.tag]
        bases = [self.typename(c) for c in t.children if c.tag == 0x1C]
        head = f"{kw} {t.name or '<anon>'}" + (f" : {', '.join('public ' + b for b in bases)}" if bases else "")
        lines = [f"{head} {{   // {t.get('byte_size', 0):#x} bytes, DIE {t.off:#x}"]
        for c in t.children:
            if c.tag == 0x0D:
                off = location_offset(c.get("location", b""))
                bits = f" : {c.get('bit_size')}" if "bit_size" in c.attrs else ""
                lines.append(f"    {self.typename(c, c.name)}{bits};" + (f"   // {off:#x}" if off is not None else ""))
            elif c.tag in (0x06, 0x14):
                lines.append(f"    {self.signature(c)};" + (f"   // {c.get('low_pc'):#x}" if "low_pc" in c.attrs else ""))
        lines.append("};")
        return "\n".join(lines)


class HeaderWriter:
    """C++ declarations for a set of types and everything they embed BY VALUE (bases, members,
    array elements), dependencies first; pointed-to types are only forward-declared. One
    definition per name: the largest complete DIE. A template instance (DWARF 1 keeps only
    `vector`, `compressed_pair`, ...) and anything unresolvable becomes an opaque struct of its
    size, so the layout -- every member at its offset -- still compiles exactly."""

    OPAQUE = {"vector", "compressed_pair", "list", "map", "basic_string", "pair", "__tree",
              "__list_imp", "__vector_imp", "__vector_deleter", "__list_deleter", "__vector_pod"}

    def __init__(self, dbg: "Debug"):
        self.dbg = dbg
        self.best: dict[str, Die] = {}
        for d in dbg.dies:
            if d.tag in (0x02, 0x13, 0x17, 0x04) and d.name:
                cur = self.best.get(d.name)
                if cur is None or (d.get("byte_size", 0), len(d.children)) > (cur.get("byte_size", 0), len(cur.children)):
                    self.best[d.name] = d
        self.done: set[str] = set()
        self.fwd: set[str] = set()
        self.out: list[str] = []

    def opaque(self, name: str, size: int) -> str:
        n = f"_opaque_{re.sub(r'[^A-Za-z0-9_]', '_', name)}_{size:x}"
        if n not in self.done:
            self.done.add(n)
            self.out.append(f"struct {n} {{ unsigned char _[{size:#x}]; }};   // {name} (a template instance, or unresolved)\n")
        return n

    def use(self, d: Die) -> str:
        """The type name a member of DIE d's type should be declared with, defining what it embeds."""
        if "user_def_type" in d.attrs:
            t = self.dbg.by_off.get(d.get("user_def_type"))
            if t is not None and t.tag == 0x01:                 # an array: define its element
                elem = self._array_elem(t)
                if elem is not None:
                    self.need(elem)
            elif t is not None:
                return self.need(t)
        if "mod_u_d_type" in d.attrs:
            blk = d.get("mod_u_d_type")
            t = self.dbg.by_off.get(struct.unpack("<I", blk[-4:])[0])
            if t is not None and t.name:
                if blk[:-4] and all(m in (0x01, 0x02) for m in blk[:-4] if m not in (0x03, 0x04)):
                    self.fwd.add(t.name) if t.tag != 0x04 else self.need(t)
                else:
                    self.need(t)
        return ""

    def _array_elem(self, t: Die) -> Die | None:
        b = t.get("subscr_data", b"")
        i = b.find(b"\x08")
        while i >= 0:
            an = struct.unpack_from("<H", b, i + 1)[0] if i + 3 <= len(b) else 0
            if ATTRS.get(an >> 4) == "user_def_type" and an & 0xF == 2:
                return self.dbg.by_off.get(struct.unpack_from("<I", b, i + 3)[0])
            i = b.find(b"\x08", i + 1)
        return None

    exclude = None

    def need(self, t: Die) -> str:
        if t.name and self.exclude is not None and self.exclude.fullmatch(t.name):
            self.done.add(t.name)
            return t.name
        if t.tag == 0x01:
            e = self._array_elem(t)
            return self.need(e) if e is not None else ""
        if t.tag not in (0x02, 0x13, 0x17, 0x04) or not t.name:
            return ""
        # the DIE referenced, when it is complete: two different types can share a name (a
        # `LOD` class and a 4-byte `LOD`), and the largest one is not always the one meant
        complete = t.get("byte_size", 0) and (t.children or t.tag == 0x04)
        best = t if complete else self.best.get(t.name, t)
        if best.name in self.done:
            return best.name
        self.done.add(best.name)
        if best.tag == 0x04:
            vals = self.dbg.enum_values(best)
            self.out.append(f"enum {best.name} {{" + ", ".join(f"{n} = {v}" for n, v in vals) + "};   "
                            f"// {best.get('byte_size')} byte(s)\n")
            return best.name
        if best.name in self.OPAQUE or not best.children:
            self.done.discard(best.name)
            return self.opaque(best.name, best.get("byte_size", 0))
        bases = []
        for c in best.children:
            if c.tag == 0x1C:
                bt = self.dbg.by_off.get(c.get("user_def_type"))
                if bt is not None:
                    bases.append(self.need(bt) or bt.name)
        members = []
        for c in best.children:
            if c.tag == 0x0D:
                self.use(c)
                members.append(c)
        kw = {0x02: "class", 0x13: "struct", 0x17: "union"}[best.tag]
        lines = [f"{kw} {best.name}" + (" : " + ", ".join(f"public {b}" for b in bases) if bases else "") + " {"]
        if kw == "class":
            lines.append("public:")
        # where the members start: past the bases (each base's size, in order)
        at = 0
        for c in best.children:
            if c.tag == 0x1C:
                bt = self.best.get(getattr(self.dbg.by_off.get(c.get("user_def_type")), "name", ""), None)
                at += bt.get("byte_size", 0) if bt is not None else 0
        last_bits = None
        for c in members:
            off = location_offset(c.get("location", b""))
            if off is not None and kw != "union" and off > at and off != last_bits:
                # a hole DWARF 1 does not describe: MW's vtable pointer (after the bases), or padding
                what = "the vtable pointer?" if off - at == 4 and at > 0 and not lines[-1].startswith("    ") else "a hole"
                lines.append(f"    unsigned char _pad{at:x}[{off - at:#x}];   // {at:#x}: {what}")
            if off is not None:
                if "bit_size" in c.attrs:
                    last_bits = off
                    at = max(at, off + c.get("byte_size", 4))
                else:
                    t = self.dbg.by_off.get(c.get("user_def_type")) if "user_def_type" in c.attrs else None
                    size = self._size_of(c)
                    at = max(at, off + size) if size else max(at, off + 4)
            decl = self.dbg.typename(c, c.name)
            t = self.dbg.by_off.get(c.get("user_def_type")) if "user_def_type" in c.attrs else None
            tt = t if (t is not None and t.get("byte_size", 0) and (t.children or t.tag == 0x04)) else \
                (self.best.get(t.name, t) if t is not None else None)
            if t is not None and t.tag != 0x01 and t.name and (t.name in self.OPAQUE or not tt.children) \
                    and tt.tag != 0x04:
                decl = f"{self.opaque(t.name, tt.get('byte_size', 0))} {c.name}"
            if "<subroutine_type" in decl:
                decl = re.sub(r"<subroutine_type@0x[0-9a-f]+>\*", "void *", decl)
            bits = f" : {c.get('bit_size')}" if "bit_size" in c.attrs else ""
            lines.append(f"    {decl}{bits};" + (f"   // {off:#x}" if off is not None else ""))
        lines.append(f"}};   // {best.get('byte_size', 0):#x} bytes\n")
        self.out.append("\n".join(lines))
        return best.name

    def _size_of(self, d: Die) -> int:
        """The byte size of a member's type, when it can be told."""
        if "fund_type" in d.attrs:
            return {0x1: 1, 0x2: 1, 0x3: 1, 0x4: 2, 0x5: 2, 0x6: 2, 0x7: 4, 0x8: 4, 0x9: 4, 0xA: 8, 0xB: 8,
                    0xC: 8, 0xD: 4, 0xE: 4, 0xF: 8, 0x15: 1, 0x8008: 16, 0x8208: 16}.get(d.get("fund_type"), 0)
        if "mod_fund_type" in d.attrs or "mod_u_d_type" in d.attrs:
            blk = d.get("mod_fund_type") or d.get("mod_u_d_type")
            if any(m in (0x01, 0x02) for m in blk[:-2 if "mod_fund_type" in d.attrs else -4]):
                return 4
        if "user_def_type" in d.attrs:
            t = self.dbg.by_off.get(d.get("user_def_type"))
            if t is not None and t.tag == 0x01:
                elem, dims = self.dbg.array_parts(t)
                n = 1
                for k in re.findall(r"\[(\d+)\]", dims):
                    n *= int(k)
                e = self._array_elem(t)
                es = (self.best.get(e.name, e).get("byte_size", 0) if e is not None else
                      {"float": 4, "int": 4, "unsigned int": 4, "char": 1, "unsigned char": 1, "short": 2,
                       "unsigned short": 2, "signed char": 1}.get(elem, 0))
                return n * es
            if t is not None:
                return self.best.get(t.name, t).get("byte_size", 0)
        return 0

    def text(self) -> str:
        # every class reached through a pointer is declared up front: some are defined below,
        # after a class that points at them
        enums = {n for n, d in self.best.items() if d.tag == 0x04}
        fwd = sorted(n for n in self.fwd if n not in enums and not n.startswith("_opaque_"))
        head = "".join(f"class {n};\n" for n in fwd)
        return head + ("\n" if head else "") + "\n".join(self.out)


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("cmd", choices=["units", "funcs", "func", "types", "census", "header"])
    ap.add_argument("elf", type=Path)
    ap.add_argument("pattern", nargs="?")
    ap.add_argument("--tu", help="funcs/func: only functions in units whose file matches this regex")
    ap.add_argument("--exclude", help="header: types an existing header defines (regex over names)")
    a = ap.parse_args(argv)
    dbg = Debug(a.elf)
    rx = re.compile(a.pattern) if a.pattern else None
    if a.cmd == "census":
        c = Counter(d.kind for d in dbg.dies)
        for k, n in c.most_common():
            print(f"{n:>7} {k}")
        print(f"{len(dbg.units())} compile units, {sum(1 for _ in dbg.functions())} functions with an address")
    elif a.cmd == "units":
        per = Counter(cu.off for _, cu in dbg.functions() if cu is not None)
        for cu in dbg.units():
            lo, hi = cu.get("low_pc"), cu.get("high_pc")
            rng = f"{lo:#010x}-{hi:#010x}" if lo is not None and hi is not None else "-" * 21
            print(f"{rng}  {per.get(cu.off, 0):>4} funcs  {cu.name}")
    elif a.cmd in ("funcs", "func"):
        turx = re.compile(a.tu) if a.tu else None
        for f, cu in sorted(dbg.functions(), key=lambda x: x[0].get("low_pc")):
            lo, hi = f.get("low_pc"), f.get("high_pc", 0)
            name = f.get("MW_mangled") or f.name
            if rx and not rx.search(name) and not rx.search(f.name):
                continue
            if turx and not (cu is not None and turx.search(cu.name)):
                continue
            if a.cmd == "func":
                print(dbg.dump(f, cu))
                print()
                continue
            tu = Path(cu.name.replace("\\", "/")).name if cu is not None else "?"
            print(f"{lo:#010x} {hi - lo:#06x} {tu:<24} {name:<50} {dbg.signature(f)}")
    elif a.cmd == "header":
        hw = HeaderWriter(dbg)
        hw.exclude = re.compile(a.exclude) if a.exclude else None
        for name, d in sorted(hw.best.items()):
            if rx and rx.fullmatch(name):
                hw.need(d)
        print(hw.text())
    elif a.cmd == "types":
        seen = set()
        for d in dbg.dies:
            if d.tag == 0x04 and d.name and (not rx or rx.search(d.name)) and (d.name, 0) not in seen:
                seen.add((d.name, 0))
                vals = dbg.enum_values(d)
                print(f"enum {d.name} {{   // {d.get('byte_size')} byte(s)")
                print("".join(f"    {n} = {v},\n" for n, v in vals) + "};\n")
                continue
            if d.tag in (0x02, 0x13, 0x17) and d.name and (not rx or rx.search(d.name)):
                key = (d.name, d.get("byte_size"))
                if key in seen:
                    continue
                seen.add(key)
                print(dbg.layout(d))
                print()
    return 0


if __name__ == "__main__":
    sys.exit(main())
