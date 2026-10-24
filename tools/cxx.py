#!/usr/bin/env python3
"""
C++ declarations that mangle back to a Metrowerks name, so a generated draft can make the
calls the original made: a member call, a virtual call.

    ./tools/cxx.py GetNode__6CZAnimFib BuildMTW__Q23zdb5CNodeFR7CMatrix   # print declarations

Two things m2c's C cannot say, both measured on SOCOM II's own handlers:

  * a MEMBER call evaluates its arguments before it loads `this`; m2c's draft calls the
    mangled name as a C function with `this` first, and MW then loads `this` first --
    zAnimObjectActiveStateTick's draft differs in three instructions until GetNode is
    called as `((CZAnim *)p)->GetNode(n, 0)`, and is byte-exact after.
  * a VIRTUAL call goes through t9 (`lw t9,96(a0); lw t9,8(t9); jalr t9`); a call through a
    function pointer -- all C can write -- goes through v0, in C and C++ alike.

So a draft whose calls need either is rewritten as C++: the mangled member functions it
calls declared in classes named and qualified as the mangling says (MW mangles a nested
class and a namespace alike, so every scope that is not itself a class with members is a
namespace, which can be reopened), and its virtual calls made on a class with the vtable
pointer where the code finds it (MW puts it after the class's own data) and a slot for
every offset up to the one called. The draft's own functions stay `extern "C"`, under the
names the map gives them.

A type this cannot declare faithfully -- a template, a member pointer, a nested type by
value inside a class that is itself declared -- makes `parse` refuse the name, and the call
stays a C call.
"""

from __future__ import annotations

import re
import sys
from dataclasses import dataclass, field


BUILTIN = {"v": "void", "b": "bool", "c": "char", "s": "short", "i": "int", "l": "long",
           "x": "long long", "f": "float", "d": "double", "r": "long double", "w": "wchar_t",
           "1": "u_long128"}
UNSIGNED = {"c": "unsigned char", "s": "unsigned short", "i": "unsigned int", "l": "unsigned long",
            "x": "unsigned long long"}
OPERATORS = re.compile(r"^__(as|pl|mi|ml|dv|md|eq|ne|lt|gt|le|ge|nw|dl|vc|cl|apl|ami|amu|adv|"
                       r"er|or|ad|co|nt|aa|oo|ls|rs|pp|mm|rf|rm|vn|vd|nwa|dla|opv)__")


class Unsupported(Exception):
    pass


@dataclass
class Type:
    text: str                          # C++ spelling, %s where a declarator name would go
    classes: list = field(default_factory=list)       # qualified class paths it names
    by_value: list = field(default_factory=list)      # class paths it takes by value

    def decl(self, name: str = "") -> str:
        return self.text.replace("%s", name).strip()


def _len_name(s: str, i: int) -> tuple[str, int]:
    m = re.match(r"\d+", s[i:])
    if not m:
        raise Unsupported(s)
    n, j = int(m.group()), i + len(m.group())
    if j + n > len(s):
        raise Unsupported(s)
    return s[j:j + n], j + n


def qualified(s: str, i: int) -> tuple[list[str], int]:
    if s[i:i + 1] == "Q":
        k, i, parts = int(s[i + 1]), i + 2, []
        for _ in range(k):
            p, i = _len_name(s, i)
            parts.append(p)
        return parts, i
    p, i = _len_name(s, i)
    return [p], i


def _ptype(s: str, i: int) -> tuple[Type, int]:
    quals = []
    while i < len(s) and s[i] in "CVPRUS":
        quals.append(s[i])
        i += 1
    if i >= len(s):
        raise Unsupported(s)
    c = s[i]
    unsigned = "U" in quals
    signed = "S" in quals
    quals = [q for q in quals if q not in "US"]
    if c == "F":                                       # a function type: F<params>_<ret>
        params, i = _params(s, i + 1, stop="_")
        ret, i = _ptype(s, i + 1)
        inner = "(%s)(" + ", ".join(p.decl() for p in params) + ")" if params else "(%s)(void)"
        base = Type(ret.decl() + " " + inner, ret.classes + [c for p in params for c in p.classes])
        base.by_value = ret.by_value + [b for p in params for b in p.by_value]
    elif c in BUILTIN and not (c == "1" and s[i + 1:i + 2].isdigit()):   # `11CSealCtrlAi` is a name
        t = UNSIGNED[c] if unsigned and c in UNSIGNED else ("signed char" if signed and c == "c" else BUILTIN[c])
        base, i = Type(t + " %s"), i + 1
    elif c.isdigit() or c == "Q":
        path, i = qualified(s, i)
        if any("<" in p for p in path):
            raise Unsupported(s)
        base = Type("::" + "::".join(path) + " %s", [path])
        if not any(q in "PR" for q in quals):
            base.by_value = [path]
    else:
        raise Unsupported(s)                           # member pointers, arrays, ...
    # the qualifiers read outward: `PC6CPnt3D` is a pointer to a const CPnt3D
    for q in reversed(quals):
        if q == "C":
            base = Type(base.text.replace("%s", "const %s", 1) if "(" not in base.text.split("%s")[0]
                        else base.text, base.classes, base.by_value)
        elif q == "V":
            base = Type(base.text.replace("%s", "volatile %s", 1), base.classes, base.by_value)
        elif q in "PR":
            star = "*" if q == "P" else "&"
            if base.text.startswith(("(", )) or "(%s)" in base.text:
                base = Type(base.text.replace("(%s)", f"({star}%s)"), base.classes, base.by_value)
            else:
                base = Type(base.text.replace("%s", f"{star}%s", 1), base.classes, base.by_value)
    return base, i


def _params(s: str, i: int, stop: str = "") -> tuple[list[Type], int]:
    out = []
    while i < len(s) and s[i] != stop[:1]:
        c = s[i]
        if c == "T":
            m = re.match(r"T(\d)", s[i:])
            if not m or int(m.group(1)) > len(out):
                raise Unsupported(s)
            out.append(out[int(m.group(1)) - 1])
            i += 2
        elif c == "N":
            m = re.match(r"N(\d)(\d)", s[i:])
            if not m or int(m.group(2)) > len(out):
                raise Unsupported(s)
            out.extend([out[int(m.group(2)) - 1]] * int(m.group(1)))
            i += 3
        elif c == "e":
            out.append(Type("..."))
            i += 1
        else:
            t, i = _ptype(s, i)
            out.append(t)
    if len(out) == 1 and out[0].text.strip() == "void %s".strip():
        out = []
    if out and out[0].decl() == "void":
        out = []
    return out, i


@dataclass
class Func:
    mangled: str
    scope: list[str]                   # the class path; [] for a free function
    name: str
    const: bool
    params: list[Type]


def parse(mangled: str) -> Func | None:
    """A member or free function's declaration parts, or None when it cannot be declared."""
    if "<" in mangled or "@" in mangled or mangled.startswith(("__ct__", "__dt__")) or OPERATORS.match(mangled):
        return None
    m = re.match(r"^([A-Za-z_][A-Za-z0-9_]*?)__(?=[0-9QF])", mangled)
    if not m:
        return None
    try:
        i = m.end()
        scope, const = [], False
        if mangled[i] != "F":
            scope, i = qualified(mangled, i)
            if mangled[i:i + 1] == "C":
                const, i = True, i + 1
        if mangled[i:i + 1] != "F":
            return None
        params, i = _params(mangled, i + 1)
        if i != len(mangled):
            return None
    except (Unsupported, IndexError, ValueError):
        return None
    return Func(mangled, scope, m.group(1), const, params)


class Decls:
    """The classes a draft's calls need, rendered once."""

    def __init__(self):
        self.methods: dict[tuple, list[str]] = {}     # class path -> member declarations
        self.types: set[tuple] = set()                 # every class path named
        self.enums: set[tuple] = set()                 # class paths taken by value

    def add(self, f: Func, ret: str) -> None:
        path = tuple(f.scope)
        params = ", ".join(p.decl() for p in f.params) or "void"
        self.methods.setdefault(path, []).append(f"{ret} {f.name}({params}){' const' if f.const else ''};")
        self.types.add(path)
        for p in f.params:
            self.types.update(tuple(c) for c in p.classes)
            self.enums.update(tuple(c) for c in p.by_value)

    def render(self) -> str:
        classes = set(self.methods) | self.types
        # a path is a class when it has members or is named as a type; its prefixes are
        # scopes -- namespaces, unless a prefix is itself a class
        for e in self.enums:
            if e in self.methods:
                raise Unsupported(f"{e} is both a class with members and taken by value")
        lines = ["typedef unsigned int u_long128 __attribute__((mode(TI)));"]
        for path in sorted(classes):
            for k in range(1, len(path)):
                if path[:k] in classes:
                    raise Unsupported(f"{path}: nested in a declared class")
        for path in sorted(classes):
            ns, name = path[:-1], path[-1]
            body = ""
            if path in self.enums:
                decl = f"enum {name} {{ {name}__max = 0x7FFFFFFF }};"
            elif path in self.methods:
                decl = None
            else:
                decl = f"struct {name};"
            if decl:
                lines.append("".join(f"namespace {n} {{ " for n in ns) + decl + " }" * len(ns))
        for path in sorted(self.methods):
            ns, name = path[:-1], path[-1]
            members = "\n    ".join(self.methods[path])
            open_ = "".join(f"namespace {n} {{ " for n in ns)
            lines.append(f"{open_}struct {name} {{\n    {members}\n}};{' }' * len(ns)}")
        return "\n".join(lines) + "\n"


def main(argv=None) -> int:
    d = Decls()
    for name in (argv or sys.argv[1:]):
        f = parse(name)
        print(f"{name}: {f}")
        if f and f.scope:
            d.add(f, "int")
    print(d.render())
    return 0


if __name__ == "__main__":
    sys.exit(main())
