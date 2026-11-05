#!/usr/bin/env python3
"""
igreplay -- Metrowerks 3.0b52's simplify/colour replayed offline on a tools/igdump.py record, so a
numbering can be tried before any source is written: move a virtual register to another index and
see which registers every variable then takes.

    ./tools/igreplay.py build/ig/graph.jsonl                          # every record: does it replay?
    ./tools/igreplay.py build/ig/graph.jsonl -f Read -r 0 --watch i,node,e
    ./tools/igreplay.py DUMP -f Read -r 0 --watch 87,41 --move 87=40.5 --move 41=43.5
    ./tools/igreplay.py DUMP -f Read -r 0 --watch 87 --drop 87-37     # an interference removed
    ./tools/igreplay.py DUMP -f Read -r 0 --want 33=v1,34=a1 --movers 33,34   # which numbering gives it
    ./tools/igreplay.py DUMP -f Read -r 0 --nb 87                     # a node's neighbours and sweeps

The allocator is the decompiled PowerPC backend's (doldecomp/mweppc Coloring.c; README "Tells from
completing the MSL headers, round 10/11"): virtual registers are numbered arguments, then named
locals in declaration order, then front-end temporaries (`@N`), then code-generation temporaries;
simplify sweeps them in ascending order, pushing every node of degree below K (32 less the class's
reserved registers: 25 GPRs -- zero, at, k0, k1, gp, sp, ra -- and all 32 FPRs) and lowering its
neighbours' degrees, until a sweep pushes nothing; then the spill pick -- the first node of least
`spillCost / degree` (as a float) in descending index order (SpillCode.c: a read adds its block's loop
weight, twice for a register not rematerialisable) -- is pushed and the sweeps resume. Colour pops
the stack: each node takes the lowest free volatile register (GPRs v0, v1, a0-a3, t0-t7, t8, t9; FPRs
f0-f19), else the next nonvolatile one (s0-s7 then fp; f20-f31), which every later node may then take
too. So a node pushed after a higher-indexed one is DEFERRED (degree K or more, or freed by a spill
pick) and coloured ahead of the rest: MW 3.0's "first-declared local gets the highest saved
register" and every one-off register tell. Checked: every GPR and FPR record replays exactly (the
colouring order and every colour) -- the MSL lane's probes and st_core.cpp (round 11), and GCC round
32a's canary.

--move V=POS renumbers node V to sit at POS (a half index between two others: `87=40.5`; every
other node keeps its relative order) -- the game states' round-37 relabel.py, which found CGraph::Read
(0x2D0C90: one counter reused across both loops AND the first node a named local -- each alone 5 or 11
off); --drop A-B removes one interference; --want/--movers tries every placement of the movers (a half
step between existing indices: the states' brute.py) and lists those giving the wanted registers;
--nb V lists V's virtual neighbours with their degrees, colours, colouring positions and the sweep
each was pushed in (the AI core's round-28 nbdeg.py, from the replay itself). A node is named by its
index (`87`, `v87`) or by its variable when one node has that name.
"""

from __future__ import annotations

import argparse
import copy
import itertools
import json
import re
import struct
import sys
from pathlib import Path

REGS = ['zero', 'at', 'v0', 'v1', 'a0', 'a1', 'a2', 'a3', 't0', 't1', 't2', 't3', 't4', 't5', 't6', 't7',
        's0', 's1', 's2', 's3', 's4', 's5', 's6', 's7', 't8', 't9', 'k0', 'k1', 'gp', 'sp', 'fp', 'ra']
F_SPILLED, F_PUSHED, F_COAL, F_INTO = 1, 2, 4, 8
GPR_RESERVED = [0, 1, 26, 27, 28, 29, 31]       # a record from before igdump read the class's own
# MW's volatile registers, lowest first, and its nonvolatile handout order, per class
VOLATILE = {0: [2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 24, 25], 1: list(range(0, 20))}
NONVOL = {0: [16, 17, 18, 19, 20, 21, 22, 23, 30], 1: list(range(20, 32))}


def f32(x: float) -> float:
    return struct.unpack('<f', struct.pack('<f', x))[0]


def rname(c: int, cls: int = 0) -> str:
    if not 0 <= c < 32:
        return f'?{c}'
    return REGS[c] if cls == 0 else f'f{c}'


class Graph:
    """One colouring record as MW's simplify/colour sees it."""

    def __init__(self, rec: dict, K: int | None = None):
        self.rec = rec
        self.cls = rec['cls']
        self.nreal = rec['nreal']
        self.nused = rec['nused']
        self.nodes = rec['nodes']
        self.adj = [list(n['nb']) for n in self.nodes]
        self.flags = [n['flags'] or 0 for n in self.nodes]
        self.names = {int(k): (v or '').split('@0x')[0] for k, v in rec['names'].items()}
        res = rec.get('reserved')
        if res is None:                 # an empty list is the FPRs' own (none reserved), not "unknown"
            res = GPR_RESERVED if self.cls == 0 else []
        self.K = K if K is not None else 32 - len(res)
        self.order_rec = rec['order']
        self.costs = rec.get('costs')
        self.use_recorded = False
        self.nused_before = rec['nused']
        self.volatile = [r for r in VOLATILE.get(self.cls, VOLATILE[0]) if r not in res]
        self.nonvol = [r for r in NONVOL.get(self.cls, NONVOL[0]) if r not in res]
        self.sweep_of: dict[int, str] = {}

    def name(self, i: int) -> str:
        return self.names.get(i, '') or ''

    def replay(self, spill_pick=None, adj=None, K=None, nused=None):
        """(colour list, colouring order, deferred set). spill_pick(candidates, degrees) chooses the
        spill candidate when simplification blocks (default: Coloring.c's least cost / degree, or the
        recorded push order's next when the record carries no costs). Each node's sweep is left in
        `sweep_of` (`3`, or `3*` for a spill pick)."""
        adj = adj if adj is not None else self.adj
        K = K if K is not None else self.K
        nused = nused if nused is not None else len(adj)
        deg = [len(a) for a in adj]
        flags = list(self.flags) + [0] * (nused - len(self.flags))
        pushed = []
        deferred = set()
        rank = {n: k for k, n in enumerate(reversed(self.order_rec))}
        self.sweep_of = {}
        nsweep = 0

        def sweep():
            nonlocal nsweep
            while True:
                nsweep += 1
                spilled = []
                changed = False
                for i in range(self.nreal, nused):
                    if flags[i] & (F_PUSHED | F_COAL):
                        continue
                    if deg[i] < K:
                        for j in adj[i]:
                            deg[j] -= 1
                        flags[i] |= F_PUSHED
                        pushed.append(i)
                        self.sweep_of[i] = str(nsweep)
                        changed = True
                    else:
                        spilled.append(i)
                if not changed:
                    return spilled

        spilled = sweep()
        while spilled:
            if spill_pick:
                best = spill_pick(spilled, deg)
            elif self.costs is not None and not self.use_recorded:
                # Coloring.c: the list is in descending index order (prepended in an ascending
                # sweep); the first node of least spillCost/degree (float) wins
                best = None
                bs = None
                for n in sorted(spilled, reverse=True):
                    s = float('inf') if n >= self.nused_before else f32(self.costs[n]) / f32(deg[n])
                    s = f32(s) if s != float('inf') else s
                    if bs is None or s < bs:
                        best, bs = n, s
            else:
                # the recorded push order's earliest among the candidates
                best = min(spilled, key=lambda n: rank.get(n, 1 << 30))
            for j in adj[best]:
                deg[j] -= 1
            flags[best] |= F_PUSHED
            pushed.append(best)
            deferred.add(best)
            self.sweep_of[best] = f'{nsweep}*'
            spilled = sweep()
        order = list(reversed(pushed))
        colour = [-1] * nused
        for i in range(self.nreal):
            colour[i] = i
        vol = set(self.volatile)
        nonvol = list(self.nonvol)
        for node in order:
            mask = set(vol)
            for j in adj[node]:
                c = colour[j]
                if c != -1 and c < 32:
                    mask.discard(c)
            if mask:
                colour[node] = min(mask)
            elif nonvol:
                r = nonvol.pop(0)
                colour[node] = r
                vol.add(r)
            else:
                colour[node] = -1       # spilled: MW records -1 (and fl & 1) for it too
        return colour, order, deferred

    def check(self):
        """(the nodes whose replayed colour is not the recorded one, whether the order is the same).
        Coalesced nodes are skipped: they take their representative's colour."""
        colour, order, deferred = self.replay()
        bad = []
        for n in self.nodes[self.nreal:]:
            i = n['i']
            if self.flags[i] & F_COAL:
                continue
            if colour[i] != n['color']:
                bad.append((i, self.name(i), rname(colour[i], self.cls), rname(n['color'], self.cls)))
        return bad, order == self.order_rec


def relabel(rec: dict, moves: dict[int, float]) -> tuple[dict, dict[int, int]]:
    """`rec` with virtual registers renumbered: moves {old index: new position (a float)}, every other
    node keeping its relative order. Returns the new record and {old index: new index}."""
    n = len(rec['nodes'])
    nreal = rec['nreal']
    keys = {i: float(i) for i in range(n)}
    keys.update(moves)
    virt = sorted(range(nreal, n), key=lambda i: keys[i])
    newidx = {i: i for i in range(nreal)}
    for k, i in enumerate(virt):
        newidx[i] = nreal + k
    rec = copy.deepcopy(rec)
    nodes = [None] * n
    for nd in rec['nodes']:
        i = nd['i']
        nd2 = dict(nd)
        nd2['i'] = newidx[i]
        nd2['nb'] = [newidx[j] for j in nd['nb']]
        nodes[newidx[i]] = nd2
    rec['nodes'] = nodes
    for key in ('costs', 'remat'):
        if rec.get(key):
            v = [0] * n
            for i in range(n):
                v[newidx[i]] = rec[key][i]
            rec[key] = v
    rec['order'] = [newidx[i] for i in rec['order']]
    rec['names'] = {str(newidx[int(k)]): v for k, v in rec['names'].items()}
    return rec, newidx


def replay_moved(rec: dict, moves: dict[int, float], drop=()) -> tuple[list[int], dict[int, int], Graph]:
    """Replay `rec` renumbered by `moves`, the interferences in `drop` (pairs of old indices) removed:
    (colours by NEW index, {old: new}, the graph)."""
    rec2, newidx = relabel(rec, moves)
    g = Graph(rec2)
    for a, b in drop:
        a, b = newidx[a], newidx[b]
        g.adj[a] = [x for x in g.adj[a] if x != b]
        g.adj[b] = [x for x in g.adj[b] if x != a]
    colour, _, _ = g.replay()
    return colour, newidx, g


def brute(rec: dict, want: dict[int, str], movers: list[int], limit: int | None = None) -> list[tuple]:
    """Every placement of `movers` (each at a half step between existing indices) whose replay gives
    each node of `want` its register: the positions, one tuple per hit."""
    lo, hi = rec['nreal'], len(rec['nodes'])
    spots = [x + 0.5 for x in range(lo - 1, hi)]
    hits = []
    for combo in itertools.product(spots, repeat=len(movers)):
        colour, newidx, g = replay_moved(rec, dict(zip(movers, combo)))
        if all(rname(colour[newidx[k]], rec['cls']) == v for k, v in want.items()):
            hits.append(combo)
            if limit and len(hits) >= limit:
                break
    return hits


def neighbours(rec: dict, v: int) -> list[str]:
    """V's virtual neighbours: index, initial degree, colour, colouring position, the replay's sweep."""
    g = Graph(rec)
    g.replay()
    cls = rec['cls']
    nodes = {n['i']: n for n in rec['nodes']}
    pos = {n: q for q, n in enumerate(rec['order'])}
    n = nodes[v]
    hw = [x for x in n['nb'] if x < rec['nreal']]
    out = [f"v{v} {g.name(v)} deg0={n['deg0']} hard={len(hw)} sweep={g.sweep_of.get(v)} "
           f"colour={rname(n['color'], cls)} pos={pos.get(v, '-')}"]
    for x in sorted(y for y in n['nb'] if y >= rec['nreal']):
        m = nodes[x]
        out.append(f"  v{x:<4} deg0={m['deg0']!s:<4} colour={rname(m['color'], cls):5} pos={pos.get(x, '-')!s:<4} "
                   f"sweep={g.sweep_of.get(x)!s:<4} {g.name(x)}")
    return out


# --- the target's registers (the MSL lane's round-11 igmap.py and igtarget.py; the kit's round 42) ---
# Each PCode instruction of a record (pre-rewrite, every operand a virtual or hard register) is
# matched to the nearest unused instruction of the candidate's final code with the same mnemonic and
# the same registers under the record's own colouring; the target's instruction at that index then
# names each virtual register's register in the TARGET (the most frequent, where uses disagree).
OPNAMES = json.loads((Path(__file__).resolve().parent / "igopnames.json").read_text())
OPNAMES.update({'1042': 'lw*', '1025': 'la*', '1112': 'entry*', '1120': 'nop*', '976': 'br*', '982': 'br*'})
PSEUDO = {'1042': 'lw', '1025': 'addiu', '1076': 'sw', '1078': 'swc1', '1043': 'lwc1', '966': 'sltu',
          '968': 'sltiu', '976': 'sltu', '982': 'sltu', '974': 'sltu', '1041': 'lb', '1040': 'lbu',
          '1074': 'sb', '1044': 'lh', '1045': 'lhu', '1075': 'sh'}


def _regs_of(text: str) -> tuple[str, list[str]]:
    parts = text.split(None, 1)
    if len(parts) < 2:
        return (parts[0] if parts else ''), []
    return parts[0], [t for t in re.split(r'[,()\s]+', parts[1]) if t in REGS]


def _pc_sig(ins, colour):
    op, args = ins
    mn = PSEUDO.get(str(op), OPNAMES.get(str(op), str(op)))
    regs, vregs = [], []
    for a in args:
        if a.startswith('r'):
            n = int(a[1:])
            c = n if n < 32 else colour[n]
            regs.append(REGS[c] if 0 <= c < 32 else '?')
            vregs.append(n)
    if str(op) in ('966', '976', '982', '974', '968'):
        regs, vregs = ['at'] + regs, [1] + vregs
    if mn == 'dmove':
        mn = 'move'
    if mn in ('beq', 'bne') and len(regs) == 2 and regs[1] == 'zero':
        mn, regs, vregs = mn + 'z', regs[:1], vregs[:1]
    return mn, regs, vregs


def tmap(rec: dict, ours: list[str], targ: list[str], window: int = 24):
    """({virtual register: its register in the target}, {vreg: disagreeing counts}, PCode unmatched),
    from a GPR record and the candidate's and the target's final code (instruction texts)."""
    colour = [n['color'] for n in rec['nodes']]
    fin = [_regs_of(t) for t in ours]
    tfin = [_regs_of(t) for t in targ]
    used, seen, pos, unmatched = set(), {}, 0, 0
    for ins in rec['code']:
        if isinstance(ins, str):
            continue
        mn, regs, vregs = _pc_sig(ins, colour)
        if not any(v >= 32 for v in vregs):
            continue
        best = None
        for d in range(window):
            for k in (pos + d, pos - d):
                if 0 <= k < len(fin) and k not in used and fin[k] == (mn, regs):
                    best = k
                    break
            if best is not None:
                break
        if best is None:
            unmatched += 1
            continue
        used.add(best)
        pos = best + 1
        tm, tr = tfin[best]
        if tm != mn or len(tr) != len(regs):
            continue
        for v, r in zip(vregs, tr):
            if v >= 32:
                seen.setdefault(v, {}).setdefault(r, 0)
                seen[v][r] += 1
    out, conflicts = {}, {}
    for v, d in seen.items():
        out[v] = REGS.index(max(d, key=d.get))
        if len(d) > 1:
            conflicts[v] = d
    return out, conflicts, unmatched


def target_colours(rec: dict, unit: Path, func: str, img=None):
    """tmap for `func` (a map name the unit defines): the unit built as sccheck builds it (its
    directive, include/ and src/), its final code and the target row's aligned with the record."""
    here = Path(__file__).resolve().parent.parent
    sys.path.insert(0, str(here))
    import scdecomp as E
    import scdiff as D
    import sccheck
    st, img = E.symbols(), img or E.image()
    _, obj, err = sccheck.build_one(Path(unit).resolve(), here / "build" / "ig" / "obj")
    if obj is None:
        raise SystemExit(f"{unit}: {err.strip().splitlines()[-1] if err.strip() else 'did not build'}")
    sym = st.get(func)
    code, rel, _ = E.Elf.read(obj).function(func)
    ours = [t for _, _, t in E.disasm(D.mask(code, rel), sym.addr)]
    targ = [t for _, _, t in E.disasm(img.read(sym.addr, sym.size), sym.addr)]
    if len(ours) != len(targ):
        print(f"  {func}: {len(ours)} instructions against the target's {len(targ)} -- the alignment is approximate")
    return tmap(rec, ours, targ)


def load(path) -> list[dict]:
    return [json.loads(ln) for ln in open(path)]


def node_index(rec: dict, key: str) -> int:
    """A node by its index (`87`, `v87`) or by the variable it holds, when only one node holds it."""
    if re.fullmatch(r'v?\d+', key):
        return int(key.lstrip('v'))
    hits = [int(k) for k, v in rec['names'].items() if (v or '').split('@0x')[0] == key]
    if len(hits) != 1:
        raise SystemExit(f"{key!r}: {'no node' if not hits else f'{len(hits)} nodes'} hold that variable"
                         + (f" ({', '.join(f'v{h}' for h in sorted(hits))})" if hits else ""))
    return hits[0]


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("dump", type=Path, help="tools/igdump.py's output")
    ap.add_argument("-f", "--func", help="only the records of the functions matching this regex")
    ap.add_argument("--cls", type=int, default=-1, help="register class: 0 GPR, 1 FPR, -1 both (default)")
    ap.add_argument("-r", "--record", type=int, help="the Nth selected record (with --move/--want/--nb)")
    ap.add_argument("--watch", help="nodes whose colours to print (indices or variables, comma-separated)")
    ap.add_argument("--move", action="append", default=[], metavar="V=POS", help="renumber V to POS (e.g. 87=40.5)")
    ap.add_argument("--drop", action="append", default=[], metavar="A-B", help="remove the interference A-B")
    ap.add_argument("--want", help="wanted registers, V=reg,... (with --movers)")
    ap.add_argument("--movers", help="the nodes --want may renumber, comma-separated")
    ap.add_argument("--nb", help="list this node's neighbours")
    ap.add_argument("--target", nargs=2, metavar=("UNIT", "FUNC"),
                    help="read each virtual register's register in the target (FUNC a map name UNIT defines); "
                         "with --movers, the saved registers it gives are the --want")
    a = ap.parse_args(argv)
    recs = load(a.dump)
    rx = re.compile(a.func) if a.func else None
    sel = [(k, r) for k, r in enumerate(recs)
           if (a.cls < 0 or r['cls'] == a.cls) and r['cls'] in VOLATILE
           and (rx is None or rx.search((r.get('proc') or '').split('@0x')[0]))]
    single = a.move or a.drop or a.want or a.nb or a.watch or a.target
    if not single:
        bad_recs = 0
        for k, r in sel:
            g = Graph(r)
            bad, same = g.check()
            bad_recs += bool(bad) or not same
            print(f"[{k}] {(r.get('proc') or '?').split('@0x')[0]} cls {r['cls']}: {r['nused'] - r['nreal']} virtual, "
                  f"K {g.K}: order {'same' if same else 'DIFFERS'}, colour mismatches {len(bad)} {bad[:6] if bad else ''}")
        print(f"{len(sel) - bad_recs} of {len(sel)} records replay exactly")
        return 1 if bad_recs else 0
    if a.record is None and len(sel) != 1:
        raise SystemExit(f"{len(sel)} records selected: name one with -r N (N counts the selected, from 0)")
    k, rec = sel[a.record or 0]
    cls = rec['cls']
    print(f"[{k}] {(rec.get('proc') or '?').split('@0x')[0]} cls {cls}: {rec['nused'] - rec['nreal']} virtual")
    if a.nb:
        print("\n".join(neighbours(rec, node_index(rec, a.nb))))
        return 0
    want = {}
    if a.target:
        if cls != 0:
            raise SystemExit("--target reads the GPR class's PCode (igdump records it for class 0 only)")
        tcol, conflicts, unmatched = target_colours(rec, Path(a.target[0]), a.target[1])
        g0 = Graph(rec)
        differ = 0
        for v in sorted(tcol):
            ours = rec['nodes'][v]['color']
            if tcol[v] != ours:
                differ += 1
                print(f"  v{v:<4} {g0.name(v)[:20]:20} ours {rname(ours):5} target {rname(tcol[v]):5}"
                      + (f"  (uses disagree: {conflicts[v]})" if v in conflicts else ""))
            if 16 <= tcol[v] <= 23 or tcol[v] == 30 or 16 <= ours <= 23 or ours == 30:
                want[v] = rname(tcol[v])
        print(f"  {len(tcol)} virtual registers mapped, {differ} differ, {len(conflicts)} with disagreeing "
              f"uses, {unmatched} PCode instructions unmatched")
        if not a.movers:
            return 0
    if a.want or want:
        if a.want:
            want = {node_index(rec, kv.split('=')[0]): kv.split('=')[1] for kv in a.want.split(',')}
        movers = [node_index(rec, x) for x in (a.movers or '').split(',') if x]
        if not movers:
            raise SystemExit("--want needs --movers")
        hits = brute(rec, want, movers)
        print(f"{len(hits)} placements of {', '.join(f'v{m}' for m in movers)} give "
              + ", ".join(f"v{k}={v}" for k, v in want.items()))
        for h in hits[:60]:
            print("  ", "  ".join(f"v{m}->{p}" for m, p in zip(movers, h)))
        return 0
    watch = [node_index(rec, x) for x in a.watch.split(',')] if a.watch else []
    moves = {}
    for m in a.move:
        v, p = m.split('=')
        moves[node_index(rec, v)] = float(p)
    drop = []
    for d in a.drop:
        x, y = d.split('-')
        drop.append((node_index(rec, x), node_index(rec, y)))
    g0 = Graph(rec)
    for tag, mv, dr in (("as recorded", {}, ()), ("replayed", {}, ()), ("moved", moves, drop)):
        if tag == "moved" and not (moves or drop):
            continue
        if tag == "as recorded":
            cols = {w: rec['nodes'][w]['color'] for w in watch}
        else:
            colour, newidx, _ = replay_moved(rec, mv, dr)
            cols = {w: colour[newidx[w]] for w in watch}
        print(f"  {tag:12} " + "  ".join(f"v{w}{('=' + g0.name(w)) if g0.name(w) else ''}:{rname(c, cls)}"
                                          for w, c in cols.items()))
    return 0


if __name__ == "__main__":
    sys.exit(main())
