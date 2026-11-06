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
into it: the optimiser's and code generation's temporaries) or `at`. The `@no_name@`/`@temp_ptr@`
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
            name = (objname(fobj) or '?').split('@0x')[0]
            inl = dict(inline=name, **site())
            link = linkname(fobj, name)
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
            else:                                   # another maker: its call of GetUniqueName
                o = dict(at=f'{ret - 5:#x}')
            temps[name] = o
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
            state['proc'] = objname(fobj)
            state['link'] = linkname(fobj, state['proc'])
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

    gdb.execute('set pagination off')
    gdb.execute('set confirm off')
    gdb.execute('break thunk_kernel32_CreateFileA')
    gdb.execute('run')
    gdb.execute('delete')
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
             timeout: int = 600) -> tuple[list[dict], Path]:
        """Compile `unit` under gdb and return its colourings (and the file they were written to).
        `compiler`/`flags` default to the unit's directive; `includes` to sccheck's [include/, src/]."""
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
        a = ap.parse_args(argv)
        if a.read:
            recs, out = read(a.read), a.read
        elif a.unit:
            try:
                recs, out = dump(a.unit, a.out, flags=a.flags.split() if a.flags else None)
            except RuntimeError as e:
                print(e, file=sys.stderr)
                return 1
        else:
            ap.error("a unit, or --read DUMP")
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
