#!/usr/bin/env python3
"""
igdump -- Metrowerks' own register colouring, read out of the compiler: a unit compiled as sccheck
builds it (its `sc:` directive's flags, include/ and src/ on the path) by mwccps2 3.0b52 running
under gdb, every colouring the allocator does written as one JSON line. The MSL headers' round-10
tool (build/p10/igdump.py, gdb Python) with the AI core's round-24 additions (each node's spill cost
and rematerialisation, each block's loop weight, the procedure's name), promoted by GCC round 32a
from the lanes' copies (states-r32 ... ai-r28 /ig/: the igdump.py every lane carried, each with an
ig.sh hard-wired to one worktree) -- this one resolves everything from its own location, so a lane
runs it from its own worktree.

    ./tools/igdump.py src/gamez/zmath/zmath_pnt3d.cpp                   # every colouring, one line each
    ./tools/igdump.py src/gamez/zmath/zmath_pnt3d.cpp -f '^Interp$'     # and those records' nodes
    ./tools/igdump.py src/Apps/FTS/graph.cpp -f Read --saved            # who holds each saved register
    ./tools/igdump.py --read build/ig/graph.jsonl -f Read --cls 1       # an earlier dump, the FPRs
    ./tools/igdump.py --read build/ig/ai_target.jsonl -f '^func_0060AA20$' --pcode   # its PCode, coloured
    ./tools/igdump.py src/ai/ai_target.cpp -f '^func_0060AA20$' --origins --cls -1  # who made each @N
    ./tools/igdump.py src/ai/ai_target.cpp -f '^func_0060AA20$' --iro --iro-phase 'after IRO_CommonSubs'
                                                                     # mwccps2's own optimiser log

The allocator (README "Tells from completing the MSL headers, round 10"): mwccps2 3.0b52's colouring
driver calls buildinterferencegraph 0x4BC7B0, simplifygraph 0x4C4480 and colorgraph 0x4C47F0 (its
exit 0x4C4962); the graph is an array of 30-byte nodes at [0x5B63C4] (next +0, the variable's Object
+4, its rematerialisation +8, spill cost +0xC, index +0x10, colour +0x12, flags +0x14, degree +0x16,
an edge list +0x1A whose records hold both ends at +0xC/+0xE and each end's next at +4/+8), the
register class at 0x5B7F5D (0 GPR, 1 FPR), the real and used register counts per class at 0x5B6348
and 0x5B6880, the reserved registers per class at 0x5A6A00, the blocks at [0x5B639C] (next +0, first
instruction +0x14, loop weight +0x24; an instruction's opcode +0x28, operand count +0x2A, 22-byte
operands from +0x3C, a register's number at +4). gdb waits for wibo's CreateFileA thunk (the PE is
loaded by then) and breaks at those four addresses -- 3.0b52's alone: a unit built by another
Metrowerks build (or GCC: tools/ccdump.py reads cc1's own dumps) is refused.

One record per colouring (a function is coloured once per class, again after spill code):
`cls`, `nreal` (hard registers, indices below it), `nused`, `proc` (the function's own name, as
`name@0xK`: the word of its Object that held it -- the bare identifier, `Read@0x8`, `__dt@0x8`),
`link` (GCC round 39a: its mangled name, the Object's +0x30, `Read__Q23zdb5CGridFRQ23zdb9CSaveLoad`;
a C function's is its name), `order` (the colouring order: the simplify
stack popped -- reversed, the push order), `nodes` (per node `i`, `flags` and `deg0` and `nb` --
its neighbours -- as buildinterferencegraph left them, `color` and `fl` after colouring: fl & 1 is
spilled, flags & 4 coalesced), `names` (a virtual register's variable, where it has one: a named
local or parameter, MW's `@N` temporaries), `costs` and `remat` (as simplify read them), `reserved`
(the class's unallocatable registers), `code` (the GPR class's pre-rewrite PCode: `B<n> w=<weight>`
block markers and [opcode, operands]), and (GCC round 34a, the weapons' round-27 request) `frame`
(the function's local area's top above sp once its frame is laid out: an Object's sp offset is this
plus its own) and, on a colouring that spilled, `spills` -- the slot pass insertspillcode runs
first (0x5173E0: the spilled nodes in INDEX order, each not rematerialised bumped down the local
area by 0x4A0D30): `base` and `end` (the area's size before and after), `slots` (per node `i`, its
variable's `name`, the `size` and `align` it was given -- a char or bool spilled takes a 16-byte
quadword -- `before` and its `off`set, negative), `remat` (spilled nodes rematerialised: no slot).
A function's records are written once its frame is laid out. tools/igreplay.py replays a record's
simplify/colour offline, and its slots; tools/igvar.py compares source variants by what this records.

GCC round 40a adds two fields (an older dump has neither: dump the unit again). `syms`: the pointers
the PCode's operands carry, named -- a k4 (memory) operand's Object, its second word (a global, a
local, a call's callee: `ig_h`), a k6 operand's label by the block it labels (`B7`); a k3 operand's
immediate is not in the eight bytes `code` keeps (0.5f's lui holds it at the operand's +12), so it is
shown as recorded. `origins`: per node named `@N`, its MAKER (the kit's round 55) -- an inline
expansion as `inline` (the function), `role` (`argument` -- one the copy needs an object for: a
trivial argument is substituted and makes none --, `local`, `result`: a statement-level copy's),
`var` (the inline's own name for it), `line` (the call site: the statement being expanded -- a copy's
statements keep the call's line, so a nested copy has its outer call's), `level` (the inliner's pass:
0 a call in the function's own text, 1 one inside a copy, `within` naming the copies expanded at that
line a level up); any other `@N` as `creator` and `caller` (create_temp_object 0x477270 and the call
into it: the optimiser's and code generation's temporaries) or `at` -- and (GCC round 41a) the `pass`
that asked: `IRO_CommonSubs` (0x5327F8, the kit's s0/s1 holders: a COMMON SUBEXPRESSION, its `expr`
as C -- `&p + 36`, `(x * y) + 7` -- and the IRO linears that `computed` it and `reused` it),
`IRO_SplitLifetimes` (0x533D7E: a reused variable's later def-use web, `var` the variable), the
linearizer's, the loop passes', CInline's serialisation and code generation's by their caller
(`what`: TEMP_MAKERS). A temporary is numbered by its Object's place in the locals -- parameters,
named locals in declaration order, then temporaries in the order made, every pass's after every
inline's -- so a common subexpression or a split web takes a virtual register after all of them.
--iro writes mwccps2's OWN optimiser log (its IRO dumper, compiled in, switched on under gdb):
every phase's flowgraph of -f's functions, each linear by the index `computed`/`reused` cite, the
passes' own lines ("Replacing common sub at 7", "Splitting range for variable: 2"); --iro-phase
prints the phases matching (`--iro-phase 'after IRO_CommonSubs|before IRO_Delinearize'`). The
`@no_name@`/`@temp_ptr@`
hidden parameters are not `@N`s. --pcode prints a record's PCode instruction by instruction, every
operand as the colouring left it (`v45<@6305>=s3`: the virtual register -- its node -- its variable
and the register it took; a GPR record's float operands `fv32<p>=f21`, from its function's FPR
record), then its variables with their makers; --origins lists the makers alone. How the inliner was
read is the dumper's comment (UniqBP and its neighbours).

Every record is named (GCC round 39a, the engine core's round 21): names are read in full -- the
reader took 24 bytes and refused a name that filled them, so a function named in 24 characters or
more had `proc` null (buildOrderedCellAtomList, SetTraversalBoundaryOrdered) and `-f '^$' -r N` was
the only way to select it; a static initialiser's, padded with blanks in its node, was null too
(`__sinit_zmath_pnt3d.cpp`), and zlib's inflate_blocks_sync_point (25) was named `8G%l@0x28`, bytes
of another word the short reader took for a name. `-f` (here and in tools/igreplay.py; tools/igvar.py's PROC) is a regex
SEARCHED in the bare name -- `proc` with its `@0xK` suffix stripped -- so `-f '^Read$'` is one function's records
and `-f Read` every name containing it; `--link` searches the mangled name instead (two `__dt`s apart:
`--link '^__dt__Q23zdb11CNodeVector'`).

Writes build/ig/<stem>.jsonl (or -o); the scratch object is deleted.
"""

from __future__ import annotations

try:                    # inside gdb this file is the dumper; outside, the driver that runs gdb on it
    import gdb
except ImportError:
    gdb = None

REGS = ['zero', 'at', 'v0', 'v1', 'a0', 'a1', 'a2', 'a3', 't0', 't1', 't2', 't3', 't4', 't5', 't6', 't7',
        's0', 's1', 's2', 's3', 's4', 's5', 's6', 's7', 't8', 't9', 'k0', 'k1', 'gp', 'sp', 'fp', 'ra']

if gdb is not None:     # ----------------------------------------------------------- the dumper
    import json
    import os
    import struct

    OUT = open(os.environ.get('IGDUMP', '/dev/stdout'), 'w')
    CLASS = 0x5b7f5d
    NREAL = 0x5b6348
    NUSED = 0x5b6880
    GRAPH = 0x5b63c4
    STRIDE = 30
    state = {}

    def mem(addr, n):
        return bytes(gdb.selected_inferior().read_memory(addr, n))

    def u8(a): return mem(a, 1)[0]
    def u16(a): return struct.unpack('<H', mem(a, 2))[0]
    def s16(a): return struct.unpack('<h', mem(a, 2))[0]
    def u32(a): return struct.unpack('<I', mem(a, 4))[0]

    def snapshot():
        cls = u8(CLASS)
        nreal = u32(NREAL + cls * 4)
        nused = u32(NUSED + cls * 4)
        base = u32(GRAPH)
        nodes = []
        for i in range(nused):
            raw = mem(base + i * STRIDE, STRIDE)
            reg, color, flags, degree = struct.unpack_from('<hhHh', raw, 0x10)
            e = struct.unpack_from('<I', raw, 0x1a)[0]
            nb = []
            guard = 0
            while e and guard < 100000:
                A = s16(e + 0xc)
                B = s16(e + 0xe)
                nb.append(B if reg < B else A)
                e = u32(e + 4) if reg < B else u32(e + 8)
                guard += 1
            nodes.append(dict(i=i, reg=reg, color=color, flags=flags, degree=degree, nb=nb))
        return cls, nreal, nused, base, nodes

    # A name is read in full (GCC round 39a): the reader took 24 bytes and refused a name that filled
    # them, so every function -- or variable -- named in 24 characters or more went unnamed
    # (buildOrderedCellAtomList, SetTraversalBoundaryOrdered: `-f '^$'` was the only way to select
    # them). IGDUMP_NAME_MAX is the canary's negative control (24: the old reader).
    NAME_MAX = int(os.environ.get('IGDUMP_NAME_MAX', '4096'))

    def hashname(p):
        """The name of the HashNameNode at p (NUL-terminated at +0xa), or None: unreadable, empty,
        not printable, or NAME_MAX bytes without a NUL."""
        z = b''
        while len(z) < NAME_MAX:
            n = min(64, NAME_MAX - len(z))
            try:
                t = mem(p + 0xa + len(z), n)
            except Exception:
                return None
            cut = t.split(b'\0')[0]
            z += cut
            if len(cut) < n:
                break
        else:
            return None
        # a static initialiser's name is padded with spaces in its node (`__sinit_zmath_pnt3d.cpp` and
        # some hundred blanks before the NUL), which the reader refused: those records were unnamed too
        z = z.rstrip(b' ')
        if z and all(32 < c < 127 for c in z):
            return z.decode()
        return None

    def objname(obj):
        # an Object's name: each word of its first 0x40 bytes tried as a HashNameNode (name at +0xa)
        if not obj:
            return None
        try:
            raw = mem(obj, 0x40)
        except Exception:
            return None
        for k in range(0, 0x40, 4):
            p = struct.unpack_from('<I', raw, k)[0]
            if 0x400000 <= p < 0xffffffff:
                z = hashname(p)
                if z:
                    return f'{z}@{k:#x}'
        return None

    def linkname(obj, proc):
        # the function's mangled (link) name: its Object's +0x30, a HashNameNode -- the bare name for
        # a C function, NAME__... for C++ (`buildOrderedCellAtomList__Q23zdb5CGridFPC6CPnt3D`); None
        # when the word there is no name that spells this function's
        try:
            p = u32(obj + 0x30)
        except Exception:
            return None
        bare = (proc or '').split('@0x')[0]
        z = hashname(p) if 0x400000 <= p < 0xffffffff else None
        if z and bare and (z == bare or z.startswith((bare + '__', bare + '<'))):   # a template's NAME<...>__F
            return z
        return None

    # GCC round 42a: an extern "C" function's records were `.l@0x4` (the game states' round 51 read them by
    # record index): objname's scan takes the first word that reads as a name, and the word before the
    # Object's own name (+0x8) -- its namespace, +0x4 -- can read as one when that namespace's bytes at
    # +0xa happen to be printable (`.l`: a heap pointer's upper half and a NUL), so the link name, which
    # must spell the name, was refused too. When the scan's name has no link name, the name word +0x8 is
    # read and kept if the link name (+0x30) spells IT -- an extern "C" function's link name IS its name
    # -- or, with no link name made yet (an inline never emitted out of line: its origins' `inline`), if
    # the scan's word lay BEFORE it (+0x0, +0x4: the Object's kind bytes and its namespace, never a name).
    # IGDUMP_NO_LINKNAME=1 is the canary's negative control (the scan alone, as before).
    LINK_FALLBACK = not os.environ.get('IGDUMP_NO_LINKNAME')

    def funcname(fobj):
        """(a function's own name as `name@0xK`, its link name or None)."""
        proc = objname(fobj)
        link = linkname(fobj, proc)
        if link is None and fobj and LINK_FALLBACK:
            try:
                p = u32(fobj + 0x8)
            except Exception:
                return proc, link
            own = hashname(p) if 0x400000 <= p < 0xffffffff else None
            l8 = linkname(fobj, own) if own else None
            if l8:
                return f'{own}@0x8', l8
            if own and (proc is None or proc.endswith(('@0x0', '@0x4'))):
                return f'{own}@0x8', None
        return proc, link

    def pcode(syms=None):
        """The blocks' PCode: `B<n> w=<weight>` markers and [opcode, operands]. With `syms` (a dict, GCC
        round 40a) each pointer an operand carries is named in it: a k4 (memory) operand's Object (its
        second word: a memory access's symbol or local, a call's callee) by its bare name, a k6
        operand's label (its first word) by the block it labels (`B7`: a PCodeLabel's +4 is its
        block). A k3 (immediate) operand's layout is not known -- its value is not in the eight bytes
        recorded -- so nothing in it is read as a pointer."""
        out = []
        blocks = {}
        blk = u32(0x5b639c)
        while blk and len(blocks) < 5000:
            blocks[blk] = len(blocks)
            blk = u32(blk)
        blk = u32(0x5b639c)
        nb = 0
        while blk and nb < 5000:
            out.append(f'B{nb} w={u32(blk + 0x24)}')
            pc = u32(blk + 0x14)
            while pc:
                op = u16(pc + 0x28)
                argc = s16(pc + 0x2a)
                args = []
                for k in range(max(0, min(argc, 8))):
                    raw = mem(pc + 0x3c + k * 0x16, 0x16)
                    kind, rcls = raw[0], raw[1]
                    if kind == 0:
                        r = struct.unpack_from('<h', raw, 4)[0]
                        args.append(('r' if rcls == 0 else f'c{rcls}:') + str(r))
                    else:
                        args.append(f'k{kind}:' + raw[2:10].hex())
                        if syms is not None:
                            w0, w1 = struct.unpack_from('<II', raw, 2)
                            if kind == 6 and w0 and f'{w0:#x}' not in syms:
                                try:
                                    b = blocks.get(u32(w0 + 4))
                                except Exception:
                                    b = None
                                if b is not None:
                                    syms[f'{w0:#x}'] = f'B{b}'
                            elif kind == 4 and w1 and f'{w1:#x}' not in syms:
                                z = objname(w1)
                                if z:
                                    syms[f'{w1:#x}'] = z.split('@0x')[0]
                out.append((op, args))
                pc = u32(pc)
            blk = u32(blk)
            nb += 1
        return out

    # Which expansion made each `@N` (GCC round 40a, the kit's round 55: what stands between 0x60AA20
    # and its last two registers). Every `@N` name is CParser_GetUniqueName's (0x42F620: '@' and the
    # unit's running id, 0x5841AA), and the inliner's variables are CInline_NewLocalObject's (0x466D00,
    # mweppc CInline.c's shape exactly): an expansion -- the expression-level CInline_InlineFunction-
    # Expression 0x4672F0 and the statement-level CInline_ExpandStatements 0x4687F0 alike -- sets up its
    # copy in CInline_SetupArgsExpression 0x466E90 (the function's Object, its CI_FuncData: +0 the
    # variables' count, +2 the arguments', +4 the variables, 0x14 bytes each, the arguments first, each
    # named at +0), whose two NewLocalObject calls hold the variable in ebp: an argument the copy needs
    # an object for (a non-trivial or modified one; a trivial one is substituted and makes none) and
    # each local the copy uses; CInline_ExtractInlineFunction 0x468D20 makes a statement-level copy's
    # RESULT object first (ebp its function). The call site is the statement CInline_ExpandStatement
    # 0x468E20 is expanding (its line at +0x16: the copied statements keep the call's), the level the
    # inliner's pass (cinline_level 0x584E06: 0 a call in the function's own text, 1 a call inside a
    # copy made at level 0 -- MW expands one level a pass, so a nested copy's temporaries are numbered
    # after the direct one's). Every other `@N` records its creator's call (create_temp_object 0x477270:
    # its caller -- the optimiser's and code generation's temporaries). Labels (newlabel 0x47713B)
    # draw from the same ids, which is why a unit's temporaries skip numbers.
    UNIQ_DONE = 0x42f63c        # inside GetUniqueName, its name made (eax): the creator's return at [esp+0x10]
    NEWLOCAL_RET = 0x466d13     # CInline_NewLocalObject's return from GetUniqueName
    SETUP_NEWLOCAL = (0x466ff3, 0x467025)   # CInline_SetupArgsExpression's NewLocalObject calls (ebp: the CI_Var)
    EXTRACT_NEWLOCAL = (0x468da4, 0x468dbf)  # CInline_ExtractInlineFunction's result objects (ebp: the function)
    TEMP_RET = 0x477283         # create_temp_object 0x477270's return from GetUniqueName
    LABEL_RET = 0x47714a        # newlabel 0x47713B's: a label, no variable
    INL_LEVEL = 0x584e06
    temps = {}

    def reg32(r):
        return int(gdb.parse_and_eval('$' + r)) & 0xffffffff

    def site():
        st = dict(state.get('stmt') or {})
        st['level'] = s16(INL_LEVEL)
        return st

    class InlStmtBP(gdb.Breakpoint):            # 0x468E20 CInline_ExpandStatement(Statement *)
        def stop(self):
            s = u32(reg32('rsp') + 4)
            st = dict(line=struct.unpack('<i', mem(s + 0x16, 4))[0])
            f = u32(s + 0x1a)
            if f:
                st['file'] = hashname(f) or f'{f:#x}'
            state['stmt'] = st
            return False

    class InlSetupBP(gdb.Breakpoint):           # 0x466E90 CInline_SetupArgsExpression(func, funcdata, args)
        def stop(self):
            esp = reg32('rsp')
            fobj, data = u32(esp + 4), u32(esp + 8)
            proc, link = funcname(fobj)
            name = (proc or '?').split('@0x')[0]
            inl = dict(inline=name, **site())
            if link and link != name:
                inl['link'] = link
            seen = state.setdefault('expanded', {})
            within = seen.get((inl.get('file'), inl['line'], inl['level'] - 1))
            if within:
                inl['within'] = sorted(set(within))
            seen.setdefault((inl.get('file'), inl['line'], inl['level']), []).append(name)
            state['inl'] = (inl, u32(data + 4), s16(data + 2))
            return False

    class UniqBP(gdb.Breakpoint):               # 0x42F63C: a new `@N`, and who asked for it
        def stop(self):
            esp = reg32('rsp')
            ret = u32(esp + 0x10)
            if ret == LABEL_RET:
                return False
            name = hashname(reg32('rax'))
            if not name:
                return False
            caller = u32(esp + 0x18)
            ebp = reg32('rbp')
            o = None
            if ret == NEWLOCAL_RET and caller in SETUP_NEWLOCAL and state.get('inl'):
                inl, vars_, nargs = state['inl']
                k = (ebp - vars_) // 0x14
                o = dict(inl, role='argument' if 0 <= k < nargs else 'local')
                try:
                    v = hashname(u32(ebp))
                except Exception:
                    v = None
                if v:
                    o['var'] = v
            elif ret == NEWLOCAL_RET and caller in EXTRACT_NEWLOCAL:
                fn = (objname(ebp) or '?').split('@0x')[0]
                o = dict(inline=fn, **site(), role='result')
            elif ret == NEWLOCAL_RET:
                o = dict(creator='0x466d00', caller=f'{caller:#x}')
            elif ret == TEMP_RET:
                o = dict(creator='0x477270', caller=f'{caller:#x}')
                # GCC round 41a: the pass that asked, and what it made the temporary for
                made = state.pop('pass', None)
                if caller in TEMP_MAKERS:
                    o['pass'], o['what'] = TEMP_MAKERS[caller]
                if made and made.get('ret') == caller:
                    o.update((k, v) for k, v in made.items() if k != 'ret')
            else:                                   # another maker: its call of GetUniqueName
                o = dict(at=f'{ret - 5:#x}')
            temps[name] = o
            return False

    # The temporaries no inline made (GCC round 41a; the kit's 0x60AA20, whose s0/s1 holders round 40a
    # traced to create_temp_object called from 0x5327F8): mwccps2's IRO passes, read under gdb (their
    # dump-phase names, IRO_DumpAfterPhase 0x51F5B0's argument after each call in IRO_Optimizer
    # 0x4D4870, and their own IRO_Dump lines).
    #   0x5327F8 -- IRO_CommonSubs 0x532920 (dumped as "IRO_CommonSubs"; it opens with IRO_FindExpressions
    #     0x524B40 (NULL, FALSE), and its last step 0x532850 logs "Replacing common sub at %d"): a
    #     COMMON SUBEXPRESSION. Every linear's expr (+0x18, an IROExpr) of one class shares a member
    #     list (IROExpr +0x1A: [0] the member, [4] the next); its step 0x532770 merges the members'
    #     temporaries (IROExpr +0xA) and, for a class with none yet, makes ONE of the type of the class's
    #     first member's linear (IROExpr +6, its rtype +0x10) -- the break at 0x5327EA, the branch
    #     only a class without an object takes, reads the class there.
    #   0x533D7E -- IRO_SplitLifetimes 0x533E30 (copts.lifetimes, -O3 and up; "IRO_SplitLifetimes"),
    #     its SplitOffRange 0x533D10(VarRecord *var: +4 its Object) logging "Splitting range for
    #     variable: %d": a variable's LATER def-use web given a new temporary, the web holding its
    #     earliest definition keeping the variable (mweppc IROUseDef.c's IRO_SplitLifetimes shape).
    #   every other maker by its caller (TEMP_MAKERS: the linearizer's IroLinearForm.c helpers, the
    #     loop passes', code generation's).
    # Numbering (measured, build/scratch/gcc/r41/p3.cpp): a variable's virtual register follows its
    # Object's place in the function's locals -- the parameters, the named locals in DECLARATION order
    # (not first use), then every temporary in the order it was made: the inliner's (a level at a
    # time), the linearizer's, then the optimiser's passes in their order -- so a common subexpression's
    # temporary comes after every named and inline variable, and a split web's after those too.
    # The IROLinear is mwccps2's (the dumper 0x501F90's reads): +0 the kind (1 operand, 2 one operand,
    # 3 two, 4 three, 6 a call), +1 the ENodeType (its name at [0x59EEAC + 4 * type], filled at run
    # time), +8 the index the IRO log prints, +0x10 rtype, +0x18 expr, +0x24 the operand (an ENode for
    # kind 1) or the left, +0x28 the right (a call's argument array), +0x2C the third (a call's
    # function), +0x26 a call's argument count; an ENode's +0 its type and +0x18 its datum (an
    # EOBJREF's Object, an EINTCONST's value).
    CSE_CLASS = 0x5327ea
    CSE_RET = 0x5327f8
    SPLIT_ENTRY = 0x533d10
    SPLIT_RET = 0x533d7e
    ENODE_NAMES = 0x59eeac
    # every other caller of create_temp_object, by the pass that runs it: `measured` where a probe made
    # one (build/scratch/gcc/r41/p2-p4.cpp), else by the file its function's asserts name (and, for the
    # linearizer, its place among IroLinearForm.c's helpers -- mweppc's order)
    TEMP_MAKERS = {
        0x467c9f: ('CInline', "an inline copy's value serialised into a temporary (CInline_LoadToTemp 0x467C70, "
                              "mweppc's shape: a ?:, && / || or forced value inside a statement-level copy)"),
        0x4feda5: ('IRO_Linearize', "a comma expression's value (AssignCommaRToTemp; measured)"),
        0x4feea4: ('IRO_Linearize', "an argument evaluated ahead of its call (AssignDangerousArgumentToTemp, by its place)"),
        0x4fef75: ('IRO_Linearize', "a logical value's temporary, set to 0 (CreateTempAssignmentToZero, by its place)"),
        0x4ff035: ('IRO_Linearize', "a logical value's temporary, set to 1 (CreateTempAssignmentToOne, by its place)"),
        0x4ff202: ('IRO_Linearize', "a null check's value (GenerateNullcheckExprTempAssignment, by its place)"),
        0x4ff304: ('IRO_Linearize', "a conditional expression's value, `c ? a : b` (GenerateExpr1TempAssignment "
                                    "0x4FF2E0: both its type and the first arm's non-void; measured)"),
        0x4ff3d9: ('IRO_Linearize', "a conditional expression's value made at its second arm (GenerateExpr2TempAssignment 0x4FF3B0)"),
        0x4ff499: ('IRO_Linearize', "an expression-level inline's returned value that loads or calls -- CInline's "
                                    "EFORCELOAD (GenerateForceLoadTempAssignment 0x4FF480; measured: a returned call)"),
        0x530925: ('IRO_LoopInvariants', "a loop-invariant expression hoisted out of its loop (IroLoopInvariants.c; measured)"),
        0x539d6c: ('IRO_StrengthReduction', "an induction variable (IroStrengthReduction.c; measured)"),
        0x53a01d: ('IRO_StrengthReduction', "an induction variable (IroStrengthReduction.c)"),
        0x51c042: ('IRO_LoopUnroller', "the loop unroller's (IroUnrollLoop.c; measured)"),
        0x51be2f: ('IRO_LoopUnroller', "the loop unroller's (IroUnrollLoop.c)"),
        0x51a34e: ('IRO_LoopUnroller', "the loop unroller's (IroUnrollLoop.c)"),
        0x51a1d2: ('IRO_LoopUnroller', "the loop unroller's (IroUnrollLoop.c)"),
        0x519fc3: ('IRO_LoopUnroller', "the loop unroller's (IroUnrollLoop.c)"),
        0x519df5: ('IRO_LoopUnroller', "the loop unroller's (IroUnrollLoop.c)"),
        0x52c0b6: ('IRO_ScalarizeClassDataMembers', "a member scalarised (IroScalarization.c, by its asserts)"),
        0x538aa7: ('IRO_NonRegLoopAccesses', "IroNonRegLoopAccesses.c's (by its asserts)"),
    }
    for _a in (0x536fa3, 0x537548, 0x53756b, 0x537608, 0x53762b):
        TEMP_MAKERS[_a] = ('IRO_RegenerateExpressions', "a rebuilt expression's (IroExprRegeneration.c, by its asserts)")
    for _a in (0x517d6c, 0x517e4b, 0x518165, 0x518180, 0x51868c, 0x518706, 0x51878d, 0x518817, 0x5188db,
               0x518936, 0x5189a5):
        TEMP_MAKERS[_a] = ('code generation', "an inline asm's operand or a spill (GCCInlineAsm.c / Spill.c, by their asserts)")
    for _a in (0x4df541, 0x4dfa68, 0x4dfe4c, 0x4dfe60, 0x4dff80, 0x4e02a3, 0x4e0561, 0x4e0bcf, 0x4e0f78):
        TEMP_MAKERS[_a] = ('CException', "the exception or construction machinery's (CException.c, by its asserts)")
    OP2 = {'EADD': '+', 'ESUB': '-', 'EMUL': '*', 'EDIV': '/', 'EMODULO': '%', 'ESHL': '<<', 'ESHR': '>>',
           'EAND': '&', 'EOR': '|', 'EXOR': '^', 'ELESS': '<', 'EGREATER': '>', 'ELESSEQU': '<=',
           'EGREATEREQU': '>=', 'EEQU': '==', 'ENOTEQU': '!=', 'ELAND': '&&', 'ELOR': '||', 'EASS': '=',
           'ECOMMA': ','}
    OP1 = {'EMONMIN': '-', 'EBINNOT': '~', 'ELOGNOT': '!'}

    def cstr(p, n=48):
        try:
            return mem(p, n).split(b'\0')[0].decode('latin1')
        except Exception:
            return None

    def ename(t):
        try:
            return cstr(u32(ENODE_NAMES + 4 * t)) or f'E{t:#x}'
        except Exception:
            return f'E{t:#x}'

    def enode_text(e):
        """An operand's ENode: an object as its address (`&p`: the IRO's operands are addresses, an
        EINDIRECT of one its value), a constant as its value."""
        if not e:
            return '?'
        t = ename(u8(e))
        if t == 'EOBJREF':
            nm = objname(u32(e + 0x18))
            return '&' + (nm.split('@0x')[0] if nm else '?')
        if t == 'EINTCONST':                    # 128 bits, the most significant word first (the
            w = struct.unpack('<IIII', mem(e + 0x18, 16))   # printer 0x501EF1 passes all four): 7 at +0x24
            v = (w[0] << 96) | (w[1] << 64) | (w[2] << 32) | w[3]
            v = v - (1 << 128) if w[0] & 0x80000000 else v
            return str(v) if -0x10000 < v < 0x10000 else hex(v & 0xffffffffffffffff)
        if t == 'EFLOATCONST':
            return repr(struct.unpack('<d', mem(e + 0x18, 8))[0])
        return t

    def linear_text(lin, depth=0):
        """An IROLinear tree as C: `p + 36`, `x * y`, `*(&a)` read back as `a`."""
        if not lin:
            return '?'
        if depth > 12:
            return '...'
        kind, t = u8(lin), ename(u8(lin + 1))
        if kind == 1:
            return enode_text(u32(lin + 0x24))
        if kind == 2:
            a = linear_text(u32(lin + 0x24), depth + 1)
            if t == 'EINDIRECT':
                if a.startswith('&') and a[1:].replace('_', 'a').replace('@', 'a').isalnum():
                    return a[1:]
                return f'*{a}' if a.startswith('(') and a.endswith(')') else f'*({a})'
            return f'{OP1[t]}{a}' if t in OP1 else f'{t}({a})'
        if kind == 3:
            l, r = linear_text(u32(lin + 0x24), depth + 1), linear_text(u32(lin + 0x28), depth + 1)
            return f'({l} {OP2[t]} {r})' if t in OP2 else f'{t}({l}, {r})'
        if kind == 4:
            return f'{t}(' + ', '.join(linear_text(u32(lin + k), depth + 1) for k in (0x24, 0x28, 0x2c)) + ')'
        if kind == 6:
            n, argv = s16(lin + 0x26), u32(lin + 0x28)
            args = [linear_text(u32(argv + 4 * i), depth + 1) for i in range(max(0, min(n, 16)))]
            f = linear_text(u32(lin + 0x2c), depth + 1)
            return f'{f[1:] if f.startswith("&") else f}({", ".join(args)})'
        return f'<{kind}:{t}>'

    def top(text):
        return text[1:-1] if text.startswith('(') and text.endswith(')') else text

    class CseClassBP(gdb.Breakpoint):           # 0x5327EA: a class of common subexpressions with no object
        # ebp the linear REUSING the value (its expr's +0x1A lists the occurrences that compute it and
        # reach it), edi that list: each computing occurrence will store into the new temporary
        def stop(self):
            members = reg32('rdi')
            idx, first, guard = [], None, members
            while guard and len(idx) < 256:
                e = u32(guard)
                lin = u32(e + 6) if e else 0
                if lin:
                    first = first or lin
                    idx.append(u32(lin + 8))
                guard = u32(guard + 4)
            state['pass'] = dict(ret=CSE_RET, **{'pass': 'IRO_CommonSubs'},
                                 expr=top(linear_text(first)) if first else '?', computed=idx,
                                 reused=[u32(reg32('rbp') + 8)])
            return False

    class CseReuseBP(gdb.Breakpoint):           # 0x532887, IRO_CommonSubs' replacement step 0x532850:
        # esi a reuse being replaced by a read of the temporary its class's first member holds
        # ([[edx] + 0xA]); recorded per temporary, by name (the first reuse is recorded already)
        def stop(self):
            lst = reg32('rdx')
            try:
                nm = (objname(u32(u32(lst) + 0xa)) or '').split('@0x')[0]
            except Exception:
                return False
            o = temps.get(nm)
            if o is not None and o.get('pass') == 'IRO_CommonSubs':
                i = u32(reg32('rsi') + 8)
                if i not in o.setdefault('reused', []):
                    o['reused'].append(i)
            return False

    class LinearBP(gdb.Breakpoint):             # a pass about to make a temporary for one IROLinear:
        # IRO_LoopInvariants' 0x530919 (ebp the IROExpr hoisted: +6 its linear), IRO_StrengthReduction's
        # 0x539D5C (eax its first argument: +6 the induction expression's linear)
        def __init__(self, at, reg, ret, name):
            super().__init__(f'*{at:#x}')
            self.reg, self.ret, self.name = reg, ret, name

        def stop(self):
            try:
                lin = u32(reg32(self.reg) + 6)
                state['pass'] = dict(ret=self.ret, **{'pass': self.name}, expr=top(linear_text(lin)),
                                     computed=[u32(lin + 8)])
            except Exception:
                pass
            return False

    class SplitBP(gdb.Breakpoint):              # 0x533D10 SplitOffRange(VarRecord *var): a later web
        def stop(self):
            var = u32(reg32('rsp') + 4)
            nm = objname(u32(var + 4)) if var else None
            state['pass'] = dict(ret=SPLIT_RET, **{'pass': 'IRO_SplitLifetimes'},
                                 var=nm.split('@0x')[0] if nm else '?')
            return False

    def origins(names):
        """{node: origin} for the record's `@N` variables whose making was seen."""
        out = {}
        for i, nm in names.items():
            bare = (nm or '').split('@0x')[0]
            if bare.startswith('@') and bare in temps:
                out[i] = temps[bare]
        return out

    class BuildBP(gdb.Breakpoint):
        def stop(self):
            esp = int(gdb.parse_and_eval('$rsp')) & 0xffffffff
            fobj = u32(esp + 4)
            state['proc'], state['link'] = funcname(fobj)
            return False

    class SimplifyBP(gdb.Breakpoint):
        def stop(self):
            snap = snapshot()
            state['pre'] = snap
            res = mem(0x5a6a00 + snap[0] * 32, 32)
            state['reserved'] = [k for k in range(32) if res[k]]
            return False

    class ColorEntryBP(gdb.Breakpoint):
        def stop(self):
            esp = int(gdb.parse_and_eval('$rsp')) & 0xffffffff
            head = u32(esp + 4)
            base = u32(GRAPH)
            order = []
            p = head
            while p and len(order) < 100000:
                order.append((p - base) // STRIDE)
                p = u32(p)
            state['order'] = order
            nused = u32(NUSED + u8(CLASS) * 4)
            costs, remat = [], []
            for i in range(nused):
                a = base + i * STRIDE
                remat.append(1 if u32(a + 8) else 0)
                costs.append(struct.unpack('<i', mem(a + 0xc, 4))[0])
            state['costs'] = costs
            state['remat'] = remat
            return False

    def flush():
        for rec in state.pop('pending', []):
            OUT.write(json.dumps(rec) + '\n')
        OUT.flush()

    class ColorExitBP(gdb.Breakpoint):
        def stop(self):
            cls, nreal, nused, base, nodes = snapshot()
            names = {}
            for n in nodes[nreal:]:
                obj = u32(base + n['i'] * STRIDE + 4)
                if obj:
                    names[n['i']] = objname(obj)
            pre = state.get('pre')
            syms = {}
            rec = dict(cls=cls, nreal=nreal, nused=nused, order=state.get('order', []),
                       nodes=[dict(i=n['i'], flags=pre[4][n['i']]['flags'] if pre else None,
                                   deg0=pre[4][n['i']]['degree'] if pre else None,
                                   nb=pre[4][n['i']]['nb'] if pre else n['nb'],
                                   color=n['color'], fl=n['flags']) for n in nodes],
                       names=names, code=pcode(syms), reserved=state.get('reserved'),
                       costs=state.get('costs'), remat=state.get('remat'), proc=state.get('proc'))
            if state.get('link'):
                rec['link'] = state['link']
            # GCC round 40a: the pointers the operands carry, named (--pcode), and the `@N`s' makers
            rec['syms'] = syms
            rec['origins'] = origins(names)
            # held until the function's frame is laid out (or the compile ends): a colouring that
            # spilled is followed by the spilled nodes' stack slots, and the frame's top comes last
            state.setdefault('pending', []).append(rec)
            return False

    # The spill slots (GCC round 34a, the weapons' round-27 request): when a colouring spills,
    # insertspillcode 0x517580 first gives each spilled node its memory -- 0x5173E0 walks the nodes
    # in INDEX order and hands each one not rematerialised (node +8) its variable's Object, or a new
    # temporary's (0x516EE0) for a node without one, to 0x4A0D30, a bump allocator over the local
    # area: size += type size, rounded up to the alignment 0x45CB20 returns (ax at 0x4A0D3B), the
    # Object's offset (+0x30) the negated total (0x5B63A8) -- the frame's top less it, at layout.
    LOCALSIZE = 0x5b63a8

    class SpillEntryBP(gdb.Breakpoint):         # 0x5173E0, the slot pass's entry
        def stop(self):
            state['spill'] = dict(base=u32(LOCALSIZE), slots=[])
            return False

    class SpillAlignBP(gdb.Breakpoint):         # 0x4A0D3B, inside the allocator: ebx the Object, ax its alignment
        def stop(self):
            sp = state.get('spill')
            if sp is None:
                return False
            obj = int(gdb.parse_and_eval('$rbx')) & 0xffffffff
            align = struct.unpack('<h', struct.pack('<H', int(gdb.parse_and_eval('$rax')) & 0xffff))[0]
            sp['slots'].append(dict(obj=obj, align=align, size=u32(u32(obj + 0xc) + 2),
                                    before=u32(LOCALSIZE)))
            return False

    class SpillExitBP(gdb.Breakpoint):          # 0x517578, the slot pass's exit: each Object's offset
        def stop(self):
            sp = state.pop('spill', None)
            rec = (state.get('pending') or [None])[-1]
            if sp is None or rec is None:
                return False
            base = u32(GRAPH)
            nreal, nused = rec['nreal'], rec['nused']
            of_obj = {}
            remat = []
            for i in range(nreal, nused):
                a = base + i * STRIDE
                if not u16(a + 0x14) & 1:
                    continue
                if u32(a + 8):
                    remat.append(i)
                obj = u32(a + 4)
                if obj and obj not in of_obj:
                    of_obj[obj] = i
            slots = []
            for s in sp['slots']:
                off = struct.unpack('<i', mem(s['obj'] + 0x30, 4))[0]
                slots.append(dict(i=of_obj.get(s['obj']), name=objname(s['obj']), size=s['size'],
                                  align=s['align'], before=s['before'], off=off))
            rec['spills'] = dict(base=sp['base'], end=u32(LOCALSIZE), slots=slots, remat=remat)
            return False

    # The frame, once laid out (0x4BE6F0; at 0x4BE8BE [0x5B6D90] is the local area's top above sp:
    # the area rounded to the stack's alignment, plus the outgoing arguments and the saved
    # registers): an Object's offset from sp is this plus its own (negative) +0x30. Every record
    # of the function since the last layout carries it.
    FRAMETOP = 0x5b6d90

    class FrameBP(gdb.Breakpoint):              # 0x4BE8BE (not 0x4BE8B1: gdb steps over a breakpoint
        # out of line, decoding as x86-64, where that absolute byte store reads rip-relative: SIGSEGV)
        def stop(self):
            top = u32(FRAMETOP)
            for rec in state.get('pending', []):
                rec['frame'] = top
            flush()
            return False

    # The optimiser's own log (GCC round 41a: --iro). mwccps2 keeps the IRO's dumper compiled in:
    # IRO_Dump 0x51F6C0 vfprintf's to the log [0x5A3818] while IRO_Log (0x5B7FA7) is set, and
    # IRO_DumpAfterPhase 0x51F5B0 dumps the whole flowgraph -- every linear by its index, kind and
    # operands, `@N` temporaries by name -- after each phase while 0x5A5D4E is set (its option byte).
    # IRO_Setup 0x4D5290 clears IRO_Log, then 0x51F600 opens the log when it is set: `<source>.log`
    # beside the source, its name built in the frame's buffer -- rewritten at 0x51F68F (fopen(buf,
    # "wt") next) to IGDUMP_IRO's path. Each function's turn starts in IRO_Optimizer 0x4D4870 (ebp its
    # Object; at 0x4D48A8 both bytes are set when IGDUMP_IRO_FUNC, searched in its bare name, matches).
    # The code is the same with the log on (the object files compared: README, round 41a).
    IRO_LOG, IRO_PHASES = 0x5b7fa7, 0x5a5d4e

    def poke(addr, data):
        gdb.selected_inferior().write_memory(addr, data)

    class IroInitBP(gdb.Breakpoint):            # 0x51F600 (sub esp,0x100): open the log
        def stop(self):
            poke(IRO_LOG, b'\1')
            return False

    class IroOpenBP(gdb.Breakpoint):            # 0x51F68F: the log's name in the frame's buffer at esp
        def stop(self):
            path = os.environ['IGDUMP_IRO'].encode() + b'\0'
            if len(path) <= 0x100:
                poke(reg32('rsp'), path)
            return False

    class IroFuncBP(gdb.Breakpoint):            # 0x4D48A8 (test ebp,ebp): a function's turn
        def stop(self):
            import re as _re
            f = reg32('rbp')
            nm = (hashname(u32(f + 8)) if f else None) or ''
            rx = os.environ.get('IGDUMP_IRO_FUNC')
            on = b'\1' if (not rx or _re.search(rx, nm)) else b'\0'
            poke(IRO_LOG, on)
            poke(IRO_PHASES, on)
            return False

    gdb.execute('set pagination off')
    gdb.execute('set confirm off')
    gdb.execute('break thunk_kernel32_CreateFileA')
    gdb.execute('run')
    gdb.execute('delete')
    if os.environ.get('IGDUMP_IRO'):
        IroInitBP('*0x51f600')
        IroOpenBP('*0x51f68f')
        IroFuncBP('*0x4d48a8')
    BuildBP('*0x4bc7b0')
    SimplifyBP('*0x4c4480')
    ColorEntryBP('*0x4c47f0')
    ColorExitBP('*0x4c4962')
    SpillEntryBP('*0x5173e0')
    SpillAlignBP('*0x4a0d3b')
    SpillExitBP('*0x517578')
    FrameBP('*0x4be8be')
    if not os.environ.get('IGDUMP_NO_ORIGINS'):     # the canary's negative control: no maker recorded
        InlStmtBP('*0x468e20')
        InlSetupBP('*0x466e90')
        UniqBP('*0x42f63c')
        if not os.environ.get('IGDUMP_NO_PASSES'):  # GCC round 41a's negative control: no pass named
            CseClassBP(f'*{CSE_CLASS:#x}')
            CseReuseBP('*0x532887')
            SplitBP(f'*{SPLIT_ENTRY:#x}')
            LinearBP(0x530919, 'rbp', 0x530925, 'IRO_LoopInvariants')
            LinearBP(0x539d5c, 'rax', 0x539d6c, 'IRO_StrengthReduction')
    gdb.execute('continue')
    flush()
    OUT.close()

else:                   # ----------------------------------------------------------- the driver
    import argparse
    import json
    import os
    import re
    import struct
    import subprocess
    import sys
    from pathlib import Path

    HERE = Path(__file__).resolve().parent.parent
    sys.path.insert(0, str(HERE / "tools"))
    sys.path.insert(0, str(HERE))
    import scdiff as D
    import sccheck

    MW_BUILD = "mwcps2-3.0b52-030722"   # the breakpoints are this build's addresses
    OUT_DIR = HERE / "build" / "ig"

    def unit_build(unit: Path) -> tuple[str, list[str] | None]:
        """The toolchain and flags `unit` is built with: its directive's, else the defaults."""
        opts = sccheck.tu_options(unit)
        return opts.get("compiler", D.COMPILER), (opts["flags"].split() if "flags" in opts else None)

    def refusal(unit: Path, compiler: str) -> str:
        """Why `unit` cannot be dumped ("" when it can)."""
        if unit.suffix == ".s" or D.is_gcc(compiler):
            return f"{unit.name}: a GCC unit ({compiler}): its allocation is cc1's -- tools/ccdump.py"
        if compiler != MW_BUILD:
            return f"{unit.name}: built by {compiler}; igdump's breakpoints are {MW_BUILD}'s addresses"
        if not D.WIBO.is_file() or os.environ.get("SC_WINE"):
            return f"igdump runs the compiler under wibo ({D.WIBO}), not wine"
        return ""

    def dump(unit: Path, out: Path | None = None, *, compiler: str | None = None,
             flags: list[str] | None = None, includes: list[Path] | None = None,
             timeout: int = 600, iro: Path | None = None, iro_func: str | None = None
             ) -> tuple[list[dict], Path]:
        """Compile `unit` under gdb and return its colourings (and the file they were written to).
        `compiler`/`flags` default to the unit's directive; `includes` to sccheck's [include/, src/].
        `iro` (GCC round 41a): also write the optimiser's own log there -- every phase's flowgraph of
        the functions whose bare name `iro_func` (a regex, searched) matches, every function without."""
        unit = Path(unit).resolve()
        if compiler is None:
            compiler, dflags = unit_build(unit)
            flags = dflags if flags is None else flags
        bad = refusal(unit, compiler)
        if bad:
            raise RuntimeError(bad)
        out = Path(out) if out else OUT_DIR / f"{unit.stem}.jsonl"
        out.parent.mkdir(parents=True, exist_ok=True)
        obj = out.parent / f"{out.stem}.{os.getpid()}.o"
        obj.unlink(missing_ok=True)
        out.unlink(missing_ok=True)
        cmd = (["gdb", "-nx", "-batch", "-x", str(Path(__file__).resolve()), "--args"]
               + D.compile_cmd(unit, obj, compiler, flags, includes or [sccheck.INCLUDE, sccheck.SRC]))
        env = dict(os.environ, IGDUMP=str(out), WINEDEBUG="-all")
        env.pop("IGDUMP_IRO", None)
        env.pop("IGDUMP_IRO_FUNC", None)
        if iro is not None:
            iro = Path(iro).resolve()
            iro.parent.mkdir(parents=True, exist_ok=True)
            iro.unlink(missing_ok=True)
            if len(str(iro)) >= 0x100:
                raise RuntimeError(f"igdump: the IRO log's path must be under 256 bytes: {iro}")
            env["IGDUMP_IRO"] = str(iro)
            if iro_func:
                env["IGDUMP_IRO_FUNC"] = iro_func
        try:
            r = subprocess.run(cmd, capture_output=True, text=True, env=env, timeout=timeout)
        except subprocess.TimeoutExpired:
            raise RuntimeError(f"igdump: gdb timed out after {timeout} s: {unit}") from None
        built = obj.exists()
        obj.unlink(missing_ok=True)
        if not built:
            lines = [l for l in (r.stdout + r.stderr).splitlines()
                     if l.strip() and not re.match(r"(Breakpoint \d|\[Inferior|warning: Error disabling)", l)]
            raise RuntimeError(f"igdump: {unit.name} did not build:\n" + "\n".join(lines[-12:]))
        recs = read(out) if out.exists() else []
        if not recs:
            raise RuntimeError(f"igdump: {unit.name} built but no colouring was recorded "
                               f"(gdb's breakpoints missed: not {MW_BUILD}?)")
        return recs, out

    def read(path: Path) -> list[dict]:
        return [json.loads(ln) for ln in open(path)]

    # --- the optimiser's own log (GCC round 41a) ---------------------------------------------------
    def iro_read(path: Path) -> list[dict]:
        """The IRO log as its functions, in the order compiled: per function `name`, `phases` -- a list
        of (phase, its flowgraph's lines) -- and `events`, the passes' own lines between the dumps
        ("Replacing common sub at 7", "Splitting range for variable: 2" and its bit sets), each with
        the phase that follows it."""
        funcs, cur, phase, buf, events = [], None, None, [], []

        def close():
            if cur is not None and phase is not None:
                cur["phases"].append((phase, buf[:]))
        for ln in open(path, errors="replace"):
            ln = ln.rstrip("\n")
            m = re.match(r"Starting function (.*)$", ln)
            if m:
                close()
                cur = dict(name=m.group(1), phases=[], events=[])
                funcs.append(cur)
                phase, buf = None, []
                continue
            m = re.match(r"Dumping function (.*?) (after|before) (.*?) ?$", ln)
            if m and cur is not None:
                close()
                phase, buf = f"{m.group(2)} {m.group(3)}", []
                for e in events:
                    cur["events"].append((e, phase))
                events = []
                continue
            if cur is None:
                continue
            if phase is None or re.match(r"(Replacing common sub|Splitting range|Def set|Use set|All defs|All uses)", ln):
                if ln.strip() and not ln.startswith("---"):
                    events.append(ln.strip())
                continue
            buf.append(ln)
        close()
        if cur is not None:
            for e in events:
                cur["events"].append((e, None))
        return funcs

    def iro_summary(funcs: list[dict], rx: str | None = None, phase_rx: str | None = None) -> list[str]:
        """Per logged function (its bare name matching `rx`): its phases, the passes' events -- each
        common subexpression replaced, each lifetime split -- and the dumps of the phases matching
        `phase_rx`."""
        pat = re.compile(rx) if rx else None
        ppat = re.compile(phase_rx) if phase_rx else None
        out = []
        for f in funcs:
            if pat and not pat.search(f["name"]):
                continue
            cses = [e for e, _ in f["events"] if e.startswith("Replacing common sub")]
            splits = [e for e, _ in f["events"] if e.startswith("Splitting range")]
            out.append(f"{f['name']}: {len(f['phases'])} phases dumped; {len(cses)} common subexpression "
                       f"replacements, {len(splits)} lifetime splits")
            for e, ph in f["events"]:
                if e.startswith(("Replacing", "Splitting")):
                    out.append(f"    {e}  (then {ph})")
            for ph, lines in f["phases"]:
                if ppat and ppat.search(ph):
                    out.append(f"  --- {ph}")
                    out.extend("  " + l for l in lines if l.strip())
        return out

    def proc_name(rec: dict) -> str:
        """The record's function name without igdump's `@0xK` suffix ('' for none)."""
        return (rec.get("proc") or "").split("@0x")[0]

    def select(recs: list[dict], rx: str | None = None, cls: int | None = 0,
               link: str | None = None) -> list[tuple[int, dict]]:
        """(index in the dump, record) of the records whose function matches `rx` (a regex searched in
        the bare name: `proc` with its `@0xK` suffix stripped) and `link` (one searched in the mangled
        name, `link`: a record without one never matches), of class `cls` (None: every class)."""
        pat = re.compile(rx) if rx else None
        lpat = re.compile(link) if link else None
        return [(k, r) for k, r in enumerate(recs)
                if (cls is None or r["cls"] == cls) and (pat is None or pat.search(proc_name(r)))
                and (lpat is None or lpat.search(r.get("link") or "") and r.get("link"))]

    def reg_name(cls: int, c: int) -> str:
        if not 0 <= c < 32:
            return "-" if c < 0 else f"->{c}"
        return REGS[c] if cls == 0 else f"f{c}"

    def var_name(rec: dict, i: int) -> str:
        return (rec["names"].get(str(i)) or "").split("@0x")[0]

    def deferred(rec: dict) -> set[int]:
        """Nodes pushed after a higher-indexed one: not simplify's first ascending sweep's (a node of
        degree K or more, or one a spill pick freed) -- coloured ahead of the rest."""
        out, mx = set(), -1
        for n in reversed(rec["order"]):
            if n < mx:
                out.add(n)
            mx = max(mx, n)
        return out

    def is_saved(cls: int, c: int) -> bool:
        return (16 <= c <= 23 or c == 30) if cls == 0 else 20 <= c <= 31

    def saved(rec: dict) -> tuple[list[str], dict[str, list[str]]]:
        """(the spilled nodes, {saved register: the nodes holding it}), each by its variable's name
        (vN where it has none) -- the summary tools/igvar.py prints per variant."""
        cls, spilled, owners = rec["cls"], [], {}
        for n in rec["nodes"][rec["nreal"]:]:
            nm = var_name(rec, n["i"]) or f"v{n['i']}"
            if n["fl"] & 1:
                spilled.append(nm)
            elif is_saved(cls, n["color"]):
                owners.setdefault(reg_name(cls, n["color"]), []).append(nm)
        return spilled, dict(sorted(owners.items(), key=lambda kv: (len(kv[0]), kv[0])))

    def origin_text(o: dict | None) -> str:
        """An `@N`'s maker (GCC round 40a): `Blend's argument t, inlined at line 17 (level 0)`; a
        temporary another pass made, by its maker and the call into it."""
        if not o:
            return ""
        if "inline" in o:
            what = {"argument": f"argument {o.get('var', '?')}", "local": f"local {o.get('var', '?')}",
                    "result": "result"}.get(o.get("role"), o.get("role", "?"))
            where = f"line {o.get('line', '?')}" + (f" of {o['file']}" if o.get("file") else "")
            within = f", inside {'/'.join(o['within'])}'s copy" if o.get("within") else ""
            return f"{o['inline']}'s {what}, inlined at {where} (level {o.get('level', '?')}{within})"
        if o.get("pass") == "IRO_CommonSubs":           # GCC round 41a: the pass, and what for
            c, r = o.get("computed") or [], o.get("reused") or []
            return (f"IRO_CommonSubs' common subexpression {o.get('expr', '?')} (computed at linear "
                    f"{', '.join(map(str, c)) or '?'}, reused at {', '.join(map(str, r)) or '?'})")
        if o.get("pass") == "IRO_SplitLifetimes":
            return f"IRO_SplitLifetimes: a later web of {o.get('var', '?')} (the earliest definition's web keeps it)"
        if o.get("pass"):
            return (f"{o['pass']}: {o.get('what', '?')}" + (f" -- {o['expr']}" if o.get("expr") else "")
                    + (f" (linear {', '.join(map(str, o['computed']))})" if o.get("computed") else "")
                    + f" (create_temp_object, called from {o.get('caller', '?')})")
        if o.get("creator") == "0x477270":
            return f"a temporary: create_temp_object, called from {o.get('caller', '?')}"
        if o.get("creator"):
            return f"made by {o['creator']}, called from {o.get('caller', '?')}"
        return f"named at {o.get('at', '?')}"

    def origin(rec: dict, i: int) -> dict | None:
        return (rec.get("origins") or {}).get(str(i))

    def show(k: int, rec: dict, every: bool = False) -> list[str]:
        """One record's virtual registers: index, variable, colour, initial degree, position in the
        colouring order (0 first), DEF (deferred), coal / SPILL, neighbour counts (virtual, hard) --
        and (GCC round 40a) an `@N`'s maker: the inline expansion that made it, or the pass.
        Without `every`, only the named ones, the deferred, the saved-register holders and the
        spilled."""
        cls, nreal = rec["cls"], rec["nreal"]
        pos = {n: q for q, n in enumerate(rec["order"])}
        dfr = deferred(rec)
        lines = [f"[{k}] {proc_name(rec) or '?'} cls {cls}: {rec['nused'] - nreal} virtual, "
                 f"{len(rec['order'])} coloured"]
        for n in rec["nodes"][nreal:]:
            i, c = n["i"], n["color"]
            nm = var_name(rec, i)
            spill, coal = n["fl"] & 1, (n["flags"] or 0) & 4
            if not (every or nm or i in dfr or spill or is_saved(cls, c)):
                continue
            virt = sum(1 for x in n["nb"] if x >= nreal)
            o = origin_text(origin(rec, i))
            lines.append(f"  v{i:<4} {nm[:22]:22} {reg_name(cls, c):5} deg0={n['deg0']!s:<3} "
                         f"pos={pos.get(i, '-')!s:<4} {'DEF' if i in dfr else '   '} "
                         f"{'SPILL' if spill else 'coal' if coal else '':5} nb={virt}+{len(n['nb']) - virt}"
                         + (f"  <- {o}" if o else ""))
        return lines

    # --- the PCode view (GCC round 40a, the AI core's round-35 pcode.py folded in) ----------------
    def opname(op, cls: int) -> str:
        """A PCode opcode's name: igreplay's tables (mwccps2's own registrations), a macro starred."""
        import igreplay as R
        o = str(op)
        if o in ("1112", "1113"):       # mwccps2's registrations (0x4AD3DC, 0x4AD3F9): the bounds of the code
            return ".start" if o == "1112" else ".end"
        if o in R.OPNAMES:
            return R.OPNAMES[o]
        if o in R.FOPNAMES:
            return R.FOPNAMES[o] + ("*" if o in ("1043", "1044", "1078", "1079") else "")
        return R.PSEUDO[o] + "*" if o in R.PSEUDO else f"op{o}"

    def fpr_record(recs: list[dict], k: int) -> dict | None:
        """The FPR colouring of record k's function (the first class-1 record after it, before the
        function's records end): what a GPR record's float operands, not yet coloured, take."""
        rec = recs[k]
        for r in recs[k + 1:]:
            if (r.get("proc"), r.get("link")) != (rec.get("proc"), rec.get("link")):
                return None
            if r["cls"] == 1:
                return r
        return None

    def operand(rec: dict, a: str, fpr: dict | None = None) -> str:
        """One PCode operand as the colouring left it: a hard register by name; a virtual one as
        `vN=reg` (the virtual register is its interference-graph node: MW's PCode is renamed as it
        coalesces, so no coalesced node is ever an operand) with its variable `<name>`; a class-1
        operand in a GPR record (not yet coloured) as `fvN`, `=fK` from its function's FPR record;
        a memory operand as its displacement and the Object it names, a label as its block (the
        pointers named by the dump's `syms`: `ig_h`, `B7`); an immediate as recorded (`k3:...`)."""
        cls, nreal = rec["cls"], rec["nreal"]
        syms = rec.get("syms") or {}
        m = re.fullmatch(r"(r|c(\d+):)(-?\d+)", a)
        if m:
            rc = 0 if m.group(1) == "r" else int(m.group(2))
            n = int(m.group(3))
            if rc not in (0, 1):
                return f"c{rc}:{n}"
            if n < 32:
                return REGS[n] if rc == 0 else f"f{n}"
            own = rec if rc == cls else (fpr if rc == 1 else None)
            pre = "v" if rc == cls else ("fv" if rc == 1 else "rv")
            if own is None or n >= len(own["nodes"]):
                return f"{pre}{n}"
            nm = var_name(own, n)
            return f"{pre}{n}" + (f"<{nm}>" if nm else "") + "=" + reg_name(rc, own["nodes"][n]["color"])
        m = re.fullmatch(r"k(\d+):([0-9a-f]{16})", a)
        if not m:
            return a
        kind, raw = int(m.group(1)), bytes.fromhex(m.group(2))
        w0, w1 = struct.unpack("<iI", raw)
        if kind == 6:
            return syms.get(f"{w0 & 0xffffffff:#x}", f"L{w0 & 0xffffffff:#x}")
        if kind == 4:                   # a memory operand: its displacement and the Object it names
            sym = (syms.get(f"{w1:#x}") or f"obj{w1:#x}") if w1 else None
            return (sym + (f"{w0:+d}" if w0 else "")) if sym else str(w0)
        # an immediate's value is NOT in the eight bytes recorded (0.5f's lui holds 0x3F000000 at the
        # operand's +12, `addiu 1` all eight zero): shown as recorded, not decoded
        return a

    def pcode_view(recs: list[dict], k: int) -> list[str]:
        """Record k's PCode, instruction by instruction, every operand coloured (`operand`), each
        instruction numbered as igreplay --target counts them; then the virtual registers it names,
        each `@N` with its maker (the inline expansion: its function, its role in the copy, the call
        site's line and level)."""
        rec = recs[k]
        cls = rec["cls"]
        fpr = fpr_record(recs, k) if cls == 0 else None
        code = rec.get("code") or []
        n_ins = sum(1 for c in code if not isinstance(c, str))
        lines = [f"[{k}] {proc_name(rec) or '?'} cls {cls}: {n_ins} instructions in "
                 f"{sum(1 for c in code if isinstance(c, str))} blocks, {rec['nused'] - rec['nreal']} virtual"]
        if not code:
            lines.append("  (no PCode recorded)")
        used, q = [], 0
        for c in code:
            if isinstance(c, str):
                lines.append(f"  {c}")
                continue
            op, args = c
            ops = [operand(rec, a, fpr) for a in args]
            for a in args:
                m = re.fullmatch(r"(?:r|c1:)(\d+)", a)
                rc = 0 if a.startswith("r") else 1
                if m and rc == cls and int(m.group(1)) >= rec["nreal"] and int(m.group(1)) not in used:
                    used.append(int(m.group(1)))
            lines.append(f"    {q:4} {opname(op, cls):9} {', '.join(ops)}")
            q += 1
        named = [i for i in range(rec["nreal"], rec["nused"]) if var_name(rec, i)]
        if named:
            lines.append("  its variables:")
            for i in named:
                o = origin_text(origin(rec, i))
                lines.append(f"    v{i:<4} {var_name(rec, i)[:22]:22} {reg_name(cls, rec['nodes'][i]['color']):5}"
                             + ("" if i in used else " (no operand)") + (f" {o}" if o else ""))
        return lines

    def main(argv=None) -> int:
        ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
        ap.add_argument("unit", type=Path, nargs="?", help="the unit to compile (or --read a dump)")
        ap.add_argument("-o", "--out", type=Path, help="the dump (default build/ig/<stem>.jsonl)")
        ap.add_argument("--read", type=Path, help="show an earlier dump instead of compiling")
        ap.add_argument("-f", "--func", help="show the records of the functions matching this regex (searched in "
                        "the bare name igdump read, its @0xK suffix stripped: `-f '^Read$'`)")
        ap.add_argument("--link", help="only the records whose MANGLED name matches this regex (`link`: GCC round 39a)")
        ap.add_argument("--cls", type=int, default=0, help="register class: 0 GPR (default), 1 FPR, -1 every")
        ap.add_argument("-a", "--all", action="store_true", help="every virtual register, not only the telling")
        ap.add_argument("--saved", action="store_true", help="per record: the spilled and the saved registers' holders")
        ap.add_argument("--pcode", action="store_true", help="per record: its PCode, each operand coloured (the "
                        "virtual register -- its node -- and the register it took), the variables named with "
                        "each `@N`'s maker (GCC round 40a)")
        ap.add_argument("--origins", action="store_true", help="per record: each `@N` variable's maker -- the "
                        "inline expansion (function, role, call site) or the pass (GCC round 40a)")
        ap.add_argument("--flags", help="compile with these flags, not the unit's")
        ap.add_argument("--iro", nargs="?", const="", metavar="LOG", help="also write the optimiser's own log "
                        "(mwccps2's IRO dumps: every phase's flowgraph, each common subexpression replaced, "
                        "each lifetime split -- GCC round 41a) for -f's functions (every one without -f), "
                        "default build/ig/<stem>.iro.log, and summarise it; with --read, a log to summarise")
        ap.add_argument("--iro-phase", metavar="REGEX", help="with --iro: print the dumps of the phases matching "
                        "(`CommonSubs|Delinearize`: after IRO_CommonSubs, before IRO_Delinearize)")
        a = ap.parse_args(argv)
        iro = None
        if a.iro is not None:               # beside the dump: build/ig/<stem>.iro.log, or -o's / --read's
            beside = a.read or a.out or (OUT_DIR / f"{Path(a.unit).stem}.jsonl" if a.unit else None)
            iro = Path(a.iro) if a.iro else (beside.with_suffix(".iro.log") if beside else None)
        if a.read:
            recs, out = read(a.read), a.read
        elif a.unit:
            try:
                recs, out = dump(a.unit, a.out, flags=a.flags.split() if a.flags else None,
                                 iro=iro, iro_func=a.func)
            except RuntimeError as e:
                print(e, file=sys.stderr)
                return 1
        else:
            ap.error("a unit, or --read DUMP")
        if iro is not None:
            if not iro.is_file():
                print(f"{iro}: no IRO log written", file=sys.stderr)
                return 1
            funcs = iro_read(iro)
            print(f"{iro}: the optimiser's log, {len(funcs)} functions")
            print("\n".join(iro_summary(funcs, a.func, a.iro_phase)))
            if not (a.pcode or a.origins or a.saved or a.all):
                return 0
        cls = None if a.cls < 0 else a.cls
        sel = select(recs, a.func, cls, a.link)
        print(f"{out}: {len(recs)} colourings, {sum(1 for r in recs if r['cls'] == 0)} GPR; "
              f"{len(sel)} selected")
        for k, r in sel:
            if a.pcode:
                print("\n".join(pcode_view(recs, k)))
            elif a.origins:
                print(f"[{k}] {proc_name(r) or '?'} cls {r['cls']}:")
                if "origins" not in r:
                    print("  (an older dump: no maker recorded -- dump the unit again)")
                for i in sorted(int(x) for x in (r.get("origins") or {})):
                    print(f"  v{i:<4} {var_name(r, i)[:22]:22} {reg_name(r['cls'], r['nodes'][i]['color']):5} "
                          f"{origin_text(origin(r, i))}")
            elif a.saved:
                sp, own = saved(r)
                print(f"[{k}] {proc_name(r)} cls {r['cls']}: spilled {' '.join(sp) or '-'}; "
                      + " ".join(f"{reg}={'/'.join(v)}" for reg, v in own.items()))
            elif a.func or a.link or a.all:
                print("\n".join(show(k, r, a.all)))
            else:
                print(f"[{k}] {proc_name(r) or '?'} cls {r['cls']}: {r['nused'] - r['nreal']} virtual")
        return 0

    if __name__ == "__main__":
        sys.exit(main())
