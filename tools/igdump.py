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
`name@0xK`: the word of its Object that held it), `order` (the colouring order: the simplify
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
                try:
                    t = mem(p + 0xa, 24)
                except Exception:
                    continue
                z = t.split(b'\0')[0]
                if z and all(32 < c < 127 for c in z) and len(z) < 24:
                    return f'{z.decode()}@{k:#x}'
        return None

    def pcode():
        out = []
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
                out.append((op, args))
                pc = u32(pc)
            blk = u32(blk)
            nb += 1
        return out

    class BuildBP(gdb.Breakpoint):
        def stop(self):
            esp = int(gdb.parse_and_eval('$rsp')) & 0xffffffff
            state['proc'] = objname(u32(esp + 4))
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
            rec = dict(cls=cls, nreal=nreal, nused=nused, order=state.get('order', []),
                       nodes=[dict(i=n['i'], flags=pre[4][n['i']]['flags'] if pre else None,
                                   deg0=pre[4][n['i']]['degree'] if pre else None,
                                   nb=pre[4][n['i']]['nb'] if pre else n['nb'],
                                   color=n['color'], fl=n['flags']) for n in nodes],
                       names=names, code=pcode(), reserved=state.get('reserved'),
                       costs=state.get('costs'), remat=state.get('remat'), proc=state.get('proc'))
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
    gdb.execute('continue')
    flush()
    OUT.close()

else:                   # ----------------------------------------------------------- the driver
    import argparse
    import json
    import os
    import re
    import subprocess
    import sys
    from pathlib import Path

    HERE = Path(__file__).resolve().parent.parent
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

    def select(recs: list[dict], rx: str | None = None, cls: int | None = 0) -> list[tuple[int, dict]]:
        """(index in the dump, record) of the records whose function matches `rx` (a regex on the bare
        name), of class `cls` (None: every class)."""
        pat = re.compile(rx) if rx else None
        return [(k, r) for k, r in enumerate(recs)
                if (cls is None or r["cls"] == cls) and (pat is None or pat.search(proc_name(r)))]

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

    def show(k: int, rec: dict, every: bool = False) -> list[str]:
        """One record's virtual registers: index, variable, colour, initial degree, position in the
        colouring order (0 first), DEF (deferred), coal / SPILL, neighbour counts (virtual, hard).
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
            lines.append(f"  v{i:<4} {nm[:22]:22} {reg_name(cls, c):5} deg0={n['deg0']!s:<3} "
                         f"pos={pos.get(i, '-')!s:<4} {'DEF' if i in dfr else '   '} "
                         f"{'SPILL' if spill else 'coal' if coal else '':5} nb={virt}+{len(n['nb']) - virt}")
        return lines

    def main(argv=None) -> int:
        ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
        ap.add_argument("unit", type=Path, nargs="?", help="the unit to compile (or --read a dump)")
        ap.add_argument("-o", "--out", type=Path, help="the dump (default build/ig/<stem>.jsonl)")
        ap.add_argument("--read", type=Path, help="show an earlier dump instead of compiling")
        ap.add_argument("-f", "--func", help="show the records of the functions matching this regex")
        ap.add_argument("--cls", type=int, default=0, help="register class: 0 GPR (default), 1 FPR, -1 every")
        ap.add_argument("-a", "--all", action="store_true", help="every virtual register, not only the telling")
        ap.add_argument("--saved", action="store_true", help="per record: the spilled and the saved registers' holders")
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
        sel = select(recs, a.func, cls)
        print(f"{out}: {len(recs)} colourings, {sum(1 for r in recs if r['cls'] == 0)} GPR; "
              f"{len(sel)} selected")
        for k, r in sel:
            if a.saved:
                sp, own = saved(r)
                print(f"[{k}] {proc_name(r)} cls {r['cls']}: spilled {' '.join(sp) or '-'}; "
                      + " ".join(f"{reg}={'/'.join(v)}" for reg, v in own.items()))
            elif a.func or a.all:
                print("\n".join(show(k, r, a.all)))
            else:
                print(f"[{k}] {proc_name(r) or '?'} cls {r['cls']}: {r['nused'] - r['nreal']} virtual")
        return 0

    if __name__ == "__main__":
        sys.exit(main())
