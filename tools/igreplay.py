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
    ./tools/igreplay.py DUMP --link '^__dt__Q23zdb11CNodeVector' -r 0 --watch 33   # by the mangled name

-f is a regex SEARCHED in the function's bare name -- igdump's `proc` with its `@0xK` suffix stripped
(`Read@0x8` is `Read`): `-f '^Read$'` selects one function's records, `-f Read` every name containing
it; --link searches the mangled name instead (igdump's `link`, GCC round 39a: two `__dt`s told apart).
-r N counts the records selected. Every record igdump writes is named since GCC round 39a; in an
older dump a function named in 24 characters or more has none (its records only `-f '^$'` selects).

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

--target UNIT FUNC (GCC round 33a, the lanes' requests) scores the colouring the replay gives: with
--move/--drop the MOVED one (as recorded, a move could not be checked against the shipped code), and
it lists every node whose uses disagree in the target even where the most frequent register is ours
(one line of it was the kit's 0x5CB450 diagnosis). --move/--drop are the base a --want/--movers search
starts from. A search says how many placements it will try, prints its progress to stderr every
--progress seconds (default 5) and stops at its budget, --timeout seconds (default 300; 0: none) or
--max-iters placements, saying so (rc 2: the hits are of the part searched) -- the weapons' round-26
search on HandleBounce ran 28 minutes. A placement is replayed on the graph renumbered in place, not on
a deep copy of the record (relabel(), kept for its callers): the same colours, ~20x faster.

    ./tools/igreplay.py DUMP -f F -r 0 --target UNIT F_MANGLED --move 41=87.5   # the move against the target
    ./tools/igreplay.py DUMP -f F -r 0 --target UNIT F_MANGLED --movers 41,87 --timeout 60

FPR records (GCC round 35a, the AI core's LeadTargetG and the lanes' `$f20,$f0,$f20` operand misses):
A dump igdump made with --compiler (a unit's colouring under another build than its directive's: GCC round 44a, the
kit's round 59) is said to be one on this tool's output, a NOTE line naming both builds, and --target then builds its
candidate with the dump's own build; --compiler BUILD names another, and says so.

--target reads a float node's register as it reads a GPR's -- the candidate's instructions at the
node's uses, the target's at the same indices -- and --move/--want/--movers score FPR records as GPR
ones (the saved registers f20-f31, the volatile f0-f19; `--want len=f21`). igdump records an FPR
colouring's PCode from this round (before, the GPR class's alone: re-dump an older unit). The GPRs are
coloured and rewritten before the FPRs, so an FPR record's GPR operands are the hard registers; its
float operations are named from mwccps2's own opcode registrations (FOPNAMES).

    ./tools/igreplay.py DUMP -f '^Exp$' --cls 1 -r 1 --target src/gamez/zmath/zmath_main.cpp Exp__5CQuatFPC6CPnt3D

The spill slots (GCC round 34a, the weapons' round-27 request): a record igdump wrote with `spills`
and `frame` also replays which stack slot each spilled node takes -- the slot pass gives them in
INDEX order, each bumped down the local area by its size and rounded to its alignment -- so a
--move shows the slots and the frame it gives as well as the colours, and --target reads each
spilled node's slot in the target (the candidate's instructions addressing it, the target's at the
same indices). The every-record mode checks each record's slots as it checks its colours.

    ./tools/igreplay.py DUMP -f '^Fire$' -r 0 --move 45=40.5 --target src/gamez/zweapon/zwep_weapon.cpp Fire__8CZWeapon...
      as recorded  v34=ammo 4/4 -212 (sp 236); v41=useflyout 16/16 -240 (sp 208); v45=@5712 4/4 -244 (sp 204); ... frame top 448
      moved        v34=ammo 4/4 -212 (sp 220); v45=@5712 4/4 -216 (sp 216); v41=useflyout 16/16 -240 (sp 192); ... frame top 432
      3 of 3 spilled nodes' slots are the target's under --move/--drop; frame top 432
"""

from __future__ import annotations

import argparse
import copy
import itertools
import json
import math
import re
import struct
import sys
import time
from dataclasses import dataclass, field
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

    def permuted(self, newidx, drop=()) -> 'Graph':
        """This graph renumbered (newidx[old] = new) with the interferences in `drop` (pairs of NEW
        indices) removed: what Graph(relabel(rec)) holds for the replay -- adjacency, flags, costs, the
        recorded order, names -- built without copying the record (its PCode, every node's dict)."""
        n = len(self.adj)
        g = copy.copy(self)
        adj = [None] * n
        flags = [0] * n
        for i in range(n):
            ni = newidx[i]
            adj[ni] = [newidx[j] for j in self.adj[i]]
            flags[ni] = self.flags[i]
        for a, b in drop:
            adj[a] = [x for x in adj[a] if x != b]
            adj[b] = [x for x in adj[b] if x != a]
        g.adj, g.flags = adj, flags
        if self.costs:
            costs = [0] * n
            for i in range(n):
                costs[newidx[i]] = self.costs[i]
            g.costs = costs
        g.order_rec = [newidx[i] for i in self.order_rec]
        g.names = {newidx[i]: v for i, v in self.names.items()}
        g.nodes = None                  # the record's nodes are not renumbered: check() is the record's
        g.sweep_of = {}
        return g


def _newidx(rec: dict, moves: dict[int, float]) -> list[int]:
    """newidx[old] = new: the virtual registers in the order of their keys -- each its own index, the
    moved ones at their positions (a float) -- the hard registers as they are. relabel()'s rule."""
    n = len(rec['nodes'])
    nreal = rec['nreal']
    keys = [float(i) for i in range(n)]
    for i, p in moves.items():
        keys[i] = p
    virt = sorted(range(nreal, n), key=keys.__getitem__)
    newidx = list(range(n))
    for k, i in enumerate(virt):
        newidx[i] = nreal + k
    return newidx


def relabel(rec: dict, moves: dict[int, float]) -> tuple[dict, dict[int, int]]:
    """`rec` with virtual registers renumbered: moves {old index: new position (a float)}, every other
    node keeping its relative order. Returns the new record and {old index: new index}. (The replay
    uses Graph.permuted and _newidx, the same rule without the copy; the canary holds them equal.)"""
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


def replay_relabelled(rec: dict, moves: dict[int, float], drop=()) -> tuple[list[int], dict[int, int], Graph]:
    """replay_moved as round 32a wrote it -- the record deep-copied by relabel() and a Graph built on
    the copy. Kept as the reference the canary holds replay_moved's faster path to."""
    rec2, newidx = relabel(rec, moves)
    g = Graph(rec2)
    for a, b in drop:
        a, b = newidx[a], newidx[b]
        g.adj[a] = [x for x in g.adj[a] if x != b]
        g.adj[b] = [x for x in g.adj[b] if x != a]
    colour, _, _ = g.replay()
    return colour, newidx, g


def replay_moved(rec: dict, moves: dict[int, float], drop=(), graph: Graph | None = None
                 ) -> tuple[list[int], dict[int, int], Graph]:
    """Replay `rec` renumbered by `moves`, the interferences in `drop` (pairs of old indices) removed:
    (colours by NEW index, {old: new}, the graph). `graph` is Graph(rec) when the caller has it (a
    search builds it once); the renumbered graph is Graph.permuted's, not a copy of the record."""
    newidx = _newidx(rec, moves)
    g = (graph or Graph(rec)).permuted(newidx, [(newidx[a], newidx[b]) for a, b in drop])
    colour, _, _ = g.replay()
    return colour, dict(enumerate(newidx)), g


@dataclass
class Search:
    """A --want/--movers search's result: the hits (each a tuple of the movers' positions), how many
    placements were replayed of how many, why it stopped short (None: it searched them all; 'limit':
    the hits wanted were found; 'max-iters' or 'timeout': the budget ran out) and its time."""
    hits: list = field(default_factory=list)
    tried: int = 0
    total: int = 0
    stopped: str | None = None
    elapsed: float = 0.0


def mover_slots(movers: list[int], base: dict[int, float] | None = None) -> list[float]:
    """Each mover's own slot, as a --permute search permutes them: its --move position in `base`, else
    its index (old indices; a slot is a key _newidx sorts by, so the movers trading slots trade places
    and every other node keeps its own)."""
    base = base or {}
    return [float(base.get(m, m)) for m in movers]


def placements(rec: dict, movers: list[int], base: dict[int, float] | None = None,
               permute: bool = False) -> tuple[int, 'itertools.product | itertools.permutations']:
    """(how many, the placements) a search tries: every half step between existing indices for each
    mover (the product), or -- `permute`, the engine core's round 24 -- only the orders of the movers
    among their own slots (k! of them, the identity first)."""
    if permute:
        if len(set(movers)) != len(movers):
            raise ValueError(f"--permute: a mover named twice ({movers})")
        return math.factorial(len(movers)), itertools.permutations(mover_slots(movers, base))
    lo, hi = rec['nreal'], len(rec['nodes'])
    spots = [x + 0.5 for x in range(lo - 1, hi)]
    return len(spots) ** len(movers), itertools.product(spots, repeat=len(movers))


def search(rec: dict, want: dict[int, str], movers: list[int], limit: int | None = None,
           base: dict[int, float] | None = None, drop=(), max_iters: int | None = None,
           timeout: float | None = None, progress=None, every: float = 5.0, permute: bool = False) -> Search:
    """Every placement of `movers` (each at a half step between existing indices), the nodes `base`
    moves held at their positions and `drop`'s interferences removed, whose replay gives each node of
    `want` its register -- until `limit` hits, `max_iters` placements or `timeout` seconds. progress(
    tried, total, hits, elapsed) is called every `every` seconds. `permute` (GCC round 42a, the engine
    core's round 24: SetCDIBBox's eight common subexpressions are ~5e18 placements) tries only the
    movers' orders among their own slots -- mover_slots(), k! of them -- each hit the slots in the
    movers' order, a --move for every mover (`--move 41=44 --move 44=41`)."""
    total, combos = placements(rec, movers, base, permute)
    res = Search(total=total)
    g0 = Graph(rec)
    cls = rec['cls']
    wanted = list(want.items())
    base = dict(base or {})
    t0 = time.monotonic()
    tick = t0 + every
    for combo in combos:
        if max_iters is not None and res.tried >= max_iters:
            res.stopped = 'max-iters'
            break
        now = time.monotonic()
        if timeout is not None and now - t0 >= timeout:
            res.stopped = 'timeout'
            break
        if progress is not None and now >= tick:
            progress(res.tried, res.total, len(res.hits), now - t0)
            tick = now + every
        moves = dict(base)
        moves.update(zip(movers, combo))
        colour, newidx, _ = replay_moved(rec, moves, drop, graph=g0)
        res.tried += 1
        if all(rname(colour[newidx[k]], cls) == v for k, v in wanted):
            res.hits.append(combo)
            if limit and len(res.hits) >= limit:
                res.stopped = 'limit'
                break
    res.elapsed = time.monotonic() - t0
    return res


def brute(rec: dict, want: dict[int, str], movers: list[int], limit: int | None = None, **budget) -> list[tuple]:
    """Every placement of `movers` (each at a half step between existing indices) whose replay gives
    each node of `want` its register: the positions, one tuple per hit (search()'s hits; `budget` its
    base, drop, max_iters, timeout, progress)."""
    return search(rec, want, movers, limit=limit, **budget).hits


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
          '968': 'sltiu', '976': 'sltu', '982': 'sltu', '974': 'sltu', '1037': 'lb', '1030': 'lbu',
          '1074': 'sb', '1038': 'lh', '1039': 'lhu', '1075': 'sh'}
# The byte and halfword loads (GCC round 36a): mwccps2's registrations name 1030 lbu, 1037 lb, 1038 lh
# and 1039 lhu -- what a fixture's `signed char`/`unsigned char`/`short`/`unsigned short` loads record.
# Rounds 32a-35a read 1040/1041/1044/1045 as lbu/lb/lh/lhu: those are ll, lld, l.s and lwc2 (none of
# them in any lane's GPR record), so no byte or halfword load was ever matched and its register was
# never read in the target.
# The FPR class (GCC round 35a). mwccps2 registers every opcode at start-up -- `push "name"; push
# <number>; call` -- and the 900 pairs read off the compiler agree with all 309 of igopnames.json;
# these are the float ones it lacks: 1110 `fcopy` (a float register move, assembled as mov.s), the
# R5900's accumulator operations 436-444, rsqrt.s; and the macro loads and stores (1043 lwc1, 1044
# l.s, 1078 swc1, 1079 s.s), whose base is r0 when the address is a symbol's -- assembled through a
# `lui`'s register, so that base matches any register ('*').
FOPNAMES = {'1110': 'mov.s', '436': 'adda.s', '437': 'madd.s', '438': 'madda.s', '439': 'max.s',
            '440': 'min.s', '441': 'msub.s', '442': 'msuba.s', '443': 'mula.s', '444': 'suba.s',
            '292': 'rsqrt.s', '1043': 'lwc1', '1044': 'lwc1', '1078': 'swc1', '1079': 'swc1'}
FREG = re.compile(r'\$f(\d+)$')


def _regs_of(text: str, cls: int = 0) -> tuple[str, list[str]]:
    """(mnemonic, registers in operand order): the GPRs, and for the FPR class (cls 1) the FPRs too,
    spelled fN (objdump's `$f20` is f20)."""
    parts = text.split(None, 1)
    if len(parts) < 2:
        return (parts[0] if parts else ''), []
    toks = re.split(r'[,()\s]+', parts[1])
    if cls != 1:
        return parts[0], [t for t in toks if t in REGS]
    return parts[0], [('f' + FREG.match(t).group(1)) if FREG.match(t) else t
                      for t in toks if t in REGS or FREG.match(t)]


def _pc_sig_fpr(ins, colour):
    """An FPR record's PCode instruction: (mnemonic, its registers under the record's colouring,
    each register's node -- an FPR operand's number, -1 for a GPR). The GPRs are coloured first and
    rewritten before the FPRs are (no FPR record holds a virtual GPR: zmath_pnt3d.cpp's twelve), so
    every `rN` is the hard register N; `c1:N` is an FPR, virtual from 32; `c2:N` (the condition flag,
    the accumulator) is implicit in the code's text and left out."""
    op, args = ins
    o = str(op)
    mn = FOPNAMES.get(o) or PSEUDO.get(o) or OPNAMES.get(o, o)
    regs, vregs = [], []
    for a in args:
        if a.startswith('r'):
            n = int(a[1:])
            regs.append(REGS[n] if 0 <= n < 32 else '?')
            vregs.append(-1)
        elif a.startswith('c1:'):
            n = int(a[3:])
            c = n if n < 32 else colour[n]
            regs.append(f'f{c}' if 0 <= c < 32 else '?')
            vregs.append(n)
    if mn in ('lwc1', 'swc1') and len(regs) == 2 and regs[1] == 'zero':
        regs[1] = '*'                   # a symbol's address: the base is the lui's register
    return mn, regs, vregs


def _sig_eq(have: tuple[str, list[str]], mn: str, regs: list[str]) -> bool:
    """Whether a final-code instruction (mnemonic, registers) is the PCode's, '*' matching any one."""
    if have[0] != mn or len(have[1]) != len(regs):
        return False
    return all(w == '*' or w == h for h, w in zip(have[1], regs))


def _pc_sig(ins, colour, cls: int = 0):
    if cls == 1:
        return _pc_sig_fpr(ins, colour)
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


def _rnum(name: str, cls: int = 0) -> int:
    """A register's number from its name in the code: a GPR's, or for the FPR class fN's N."""
    if cls == 1:
        return int(name[1:]) if re.fullmatch(r'f\d+', name) else -1
    return REGS.index(name)


DISP = re.compile(r',(-?\d+)\(')


def _pc_disp(ins) -> int | None:
    """A PCode memory access's displacement (GCC round 36a): its `k4:` operand is two little-endian
    words, the displacement and the Object a symbol's or a local's access names -- known before the
    final code only where that Object is none (a pointer and a constant offset: `lh t1,42(a3)` is
    `k4:2a00000000000000`); None otherwise (a %lo, a frame offset)."""
    for a in ins[1]:
        if a.startswith('k4:') and len(a) == 19:
            disp, obj = struct.unpack('<iI', bytes.fromhex(a[3:]))
            return disp if obj == 0 else None
    return None


def _disp_of(text: str) -> int | None:
    m = DISP.search(text)
    return int(m.group(1)) if m else None


def tmap(rec: dict, ours: list[str], targ: list[str], window: int = 24, disp: bool = True):
    """({virtual register: its register in the target}, {vreg: disagreeing counts}, PCode unmatched),
    from a record -- GPR or (GCC round 35a) FPR -- and the candidate's and the target's final code
    (instruction texts): each PCode instruction holding one of the class's virtual registers is
    matched to the nearest unused instruction of the candidate's with its mnemonic and registers, and
    the target's instruction at that index names the node's register there. With `disp` (GCC round
    36a; the default), a memory access whose displacement the PCode knows (_pc_disp) takes the
    nearest such instruction with that displacement first, the nearest without only if none is in
    the window: an unrolled copy's identical `lh t1,N(a3)` -- each a different field, scheduled out of
    the PCode's order -- otherwise walk the alignment into the wrong copy (c2dfont.cpp's vector
    erase: 41 of its 68 registers lost, once round 36a's table matched its halfword loads)."""
    cls = rec['cls']
    colour = [n['color'] for n in rec['nodes']]
    fin = [_regs_of(t, cls) for t in ours]
    tfin = [_regs_of(t, cls) for t in targ]
    fdisp = [_disp_of(t) for t in ours] if disp else None
    used, seen, pos, unmatched = set(), {}, 0, 0
    for ins in rec['code']:
        if isinstance(ins, str):
            continue
        mn, regs, vregs = _pc_sig(ins, colour, cls)
        if not any(v >= 32 for v in vregs):
            continue
        want = _pc_disp(ins) if disp else None
        best = None
        for exact in ((True, False) if want is not None else (False,)):
            for d in range(window):
                for k in (pos + d, pos - d):
                    if 0 <= k < len(fin) and k not in used and (not exact or fdisp[k] == want) and (
                            fin[k] == (mn, regs) if cls != 1 else _sig_eq(fin[k], mn, regs)):
                        best = k
                        break
                if best is not None:
                    break
            if best is not None:
                break
        if best is None:
            unmatched += 1
            continue
        used.add(best)
        pos = best + 1
        if best >= len(tfin):           # a candidate longer than the target (GCC round 36a: an
            continue                    # IndexError before): nothing there to read
        tm, tr = tfin[best]
        if tm != mn or len(tr) != len(regs):
            continue
        for v, r in zip(vregs, tr):
            if v >= 32:
                seen.setdefault(v, {}).setdefault(r, 0)
                seen[v][r] += 1
    out, conflicts = {}, {}
    for v, d in seen.items():
        out[v] = _rnum(max(d, key=d.get), cls)
        if len(d) > 1:
            conflicts[v] = d
    return out, conflicts, unmatched


def final_code(unit: Path, func: str, img=None, compiler: str | None = None) -> tuple[list[str], list[str]]:
    """(the candidate's final code, the target row's), instruction texts: `func` (a map name the unit
    defines) built as sccheck builds the unit (its directive, include/ and src/) -- under `compiler`
    in place of its directive's toolchain when given (--compiler, GCC round 44a)."""
    here = Path(__file__).resolve().parent.parent
    sys.path.insert(0, str(here))
    import scdecomp as E
    import scdiff as D
    import sccheck
    st, img = E.symbols(), img or E.image()
    _, obj, err = sccheck.build_one(Path(unit).resolve(), here / "build" / "ig" / "obj", compiler=compiler)
    if obj is None:
        raise SystemExit(f"{unit}: {err.strip().splitlines()[-1] if err.strip() else 'did not build'}")
    sym = st.get(func)
    code, rel, _ = E.Elf.read(obj).function(func)
    ours = [t for _, _, t in E.disasm(D.mask(code, rel), sym.addr)]
    targ = [t for _, _, t in E.disasm(img.read(sym.addr, sym.size), sym.addr)]
    if len(ours) != len(targ):
        print(f"  {func}: {len(ours)} instructions against the target's {len(targ)} -- the alignment is approximate")
    return ours, targ


def target_colours(rec: dict, unit: Path, func: str, img=None, code=None):
    """tmap for `func` (a map name the unit defines): the unit built as sccheck builds it (its
    directive, include/ and src/), its final code and the target row's aligned with the record
    (`code`: final_code()'s pair, when the caller has it)."""
    ours, targ = code or final_code(unit, func, img)
    return tmap(rec, ours, targ)


# --- the stack slots of the spilled nodes (GCC round 34a: the weapons' round-27 request) ------------
# igdump records, for a colouring that spilled, the slot pass's work (`spills`: the local area's size
# before it, `base`, and per slot the node, its Object's size and alignment and its offset) and, per
# function, the local area's top above sp once the frame is laid out (`frame`). The slot pass
# (insertspillcode's first step, 0x5173E0) walks the nodes in INDEX order and gives every spilled one
# that is not rematerialised a slot by bumping the area: size added, rounded up to the alignment, the
# offset the negated total. So a renumbering moves the slots as well as the colours: CZWeapon::Fire
# (0x3D2D70) spills ammo (v34, 4 bytes), useflyout (v41, a 16-byte quadword) and GetDefaultAmmo's
# result (v45, 4): in index order the two words fall apart (-212, -240, -244: 244 bytes, a 448-byte
# frame); numbered ammo, result, flag -- the target's 220, 216, 192 -- they share a quadword (240,
# 432). A node with no recorded slot that spills only in a replay is sized as its class's word (4).

SP_RE = re.compile(r'(-?\d+)\(sp\)')


def _align_up(x: int, a: int) -> int:
    return (x + a - 1) & ~(a - 1) if a > 0 else x


def stack_slots(rec: dict, colour: list[int] | None = None, newidx=None, later_end: int | None = None) -> dict | None:
    """The slots the slot pass gives the nodes the colouring spills: `colour` by NEW index and
    `newidx` (old -> new) a replay's (default: the record's own colouring). None when the record
    carries no slot information (igdump before round 34a, or a colouring that spilled nothing).
    {'slots': [(old index, name, size, align, offset, sp offset or None, assumed size)], 'base', 'end',
    'frame' (the local area's top above sp, None unless igdump recorded the function's)}; `later_end`,
    the local area's size after the function's last spill pass when that is a later one, keeps the
    later allocations in the frame."""
    sp = rec.get('spills')
    if not sp:
        return None
    n = len(rec['nodes'])
    nreal = rec['nreal']
    if newidx is None:
        newidx = list(range(n))
    known = {s['i']: (s['size'], s['align']) for s in sp['slots'] if s.get('i') is not None}
    remat = rec.get('remat') or [0] * n
    flags = [nd['flags'] or 0 for nd in rec['nodes']]
    if colour is None:                  # the record's own: MW's spilled flag
        spilled = [i for i in range(nreal, n) if rec['nodes'][i]['fl'] & F_SPILLED and not remat[i]]
    else:
        spilled = [i for i in range(nreal, n)
                   if colour[newidx[i]] == -1 and not flags[i] & F_COAL and not remat[i]]
    names = {int(k): (v or '').split('@0x')[0] for k, v in rec['names'].items()}
    top = sp['base']
    slots = []
    for i in sorted(spilled, key=lambda i: newidx[i]):
        size, align = known.get(i, (4, 4))
        top = _align_up(top + size, align)
        slots.append([i, names.get(i, ''), size, align, -top, None, i not in known])
    frame = rec.get('frame')
    if frame is not None:
        last = later_end if later_end is not None else sp['end']
        extra = frame - _align_up(last, 16)
        frame = _align_up(last + top - sp['end'], 16) + extra
        for s in slots:
            s[5] = frame + s[4]
    return dict(slots=[tuple(s) for s in slots], base=sp['base'], end=top, frame=frame)


def slots_line(res: dict) -> str:
    out = []
    for i, name, size, align, off, spoff, assumed in res['slots']:
        out.append(f"v{i}{('=' + name) if name else ''} {size}/{align}{'?' if assumed else ''} {off}"
                   + (f" (sp {spoff})" if spoff is not None else ""))
    return ("; ".join(out) or "none") + f"; locals {res['base']} -> {res['end']}" + (
        f", frame top {res['frame']}" if res['frame'] is not None else "")


def check_slots(rec: dict) -> bool | None:
    """Whether the slot model gives the record's own slots (None: the record carries none)."""
    sp = rec.get('spills')
    if not sp:
        return None
    got = stack_slots(rec)
    want = [(s['i'], s['size'], s['align'], s['off']) for s in sp['slots']]
    return [(i, size, align, off) for i, _, size, align, off, _, _ in got['slots']] == want \
        and got['end'] == sp['end']


def target_slots(res: dict, ours: list[str], targ: list[str]) -> dict[int, dict[int, int]]:
    """{node: {the target's sp offset: uses}}: every instruction of the candidate's final code that
    addresses a node's slot (sp + its offset, as `stack_slots` of the RECORD's colouring gives it)
    and the target's at the same index -- an alignment by index, so read it where the two have the
    same length (Fire: 31 instructions differ in their offsets alone)."""
    out: dict[int, dict[int, int]] = {}
    at = {s[5]: s[0] for s in res['slots'] if s[5] is not None}
    for k, text in enumerate(ours):
        m = SP_RE.search(text)
        if not m or int(m.group(1)) not in at or k >= len(targ):
            continue
        t = SP_RE.search(targ[k])
        if t and text.split()[0] == targ[k].split()[0]:
            v = at[int(m.group(1))]
            out.setdefault(v, {}).setdefault(int(t.group(1)), 0)
            out[v][int(t.group(1))] += 1
    return out


def target_rows(tcol: dict, conflicts: dict, colour) -> list[tuple]:
    """[(v, our register, the target's, the target's disagreeing uses or None)] for every mapped node
    whose register (`colour[v]`) is not the target's most frequent one, AND every node whose uses
    disagree in the target even where that most frequent register is ours -- one use elsewhere is a
    value the target keeps in two registers (the kit's round 43: 0x5CB450's diagnosis was that line)."""
    return [(v, colour[v], tcol[v], conflicts.get(v)) for v in sorted(tcol)
            if tcol[v] != colour[v] or v in conflicts]


def is_saved(c: int, cls: int = 0) -> bool:
    """A callee-saved register: s0-s7 and fp, or for the FPR class f20-f31."""
    return 20 <= c <= 31 if cls == 1 else (16 <= c <= 23 or c == 30)


def target_slot_report(rec: dict, code, moves=None, drop=(), late: int | None = None
                       ) -> tuple[list[str], int, int]:
    """--target's slot lines: each spilled node's slot from sp -- the record's, or with `moves`/`drop`
    the replay's -- against the target's (target_slots: the candidate's instructions at the record's
    slots, the target's at the same indices); and (agreeing, mapped)."""
    own = stack_slots(rec, later_end=late)
    tsl = target_slots(own, *code)
    mine = own
    if moves or drop:
        moved, newidx, _ = replay_moved(rec, moves or {}, drop)
        mine = stack_slots(rec, moved, newidx, later_end=late)
    sp_of = {s[0]: s[5] for s in mine['slots']}
    g = Graph(rec)
    lines, agree = [], 0
    for v in sorted(tsl, key=lambda v: -(sp_of.get(v) or 0)):
        best = max(tsl[v], key=tsl[v].get)
        agree += best == sp_of.get(v)
        lines.append(f"  slot v{v:<4} {g.name(v)[:20]:20} {'moved' if mine is not own else 'ours '} sp "
                     f"{sp_of.get(v)!s:5} target sp {best}" + (f"  (uses: {tsl[v]})" if len(tsl[v]) > 1 else ""))
    lines.append(f"  {agree} of {len(tsl)} spilled nodes' slots are the target's"
                 + (" under --move/--drop" if mine is not own else "")
                 + (f"; frame top {mine['frame']}" if mine['frame'] is not None else ""))
    return lines, agree, len(tsl)


def target_report(rec: dict, tcol: dict, conflicts: dict, unmatched: int, moves=None, drop=()
                  ) -> tuple[list[str], dict[int, str], int]:
    """--target's lines, the saved registers a --movers search then wants, and how many nodes differ:
    each mapped node scored against the colouring the record holds -- or, with `moves`/`drop` (old
    indices), the colouring their replay gives, so a move is checked against the shipped code."""
    g0 = Graph(rec)
    cls = rec['cls']
    recorded = [n['color'] for n in rec['nodes']]
    colour = recorded
    if moves or drop:
        moved, newidx, _ = replay_moved(rec, moves or {}, drop, graph=g0)
        colour = [moved[newidx[v]] for v in range(len(recorded))]
    lines, want = [], {}
    rows = target_rows(tcol, conflicts, colour)
    for v, ours, targ, uses in rows:
        if ours != targ:
            lines.append(f"  v{v:<4} {g0.name(v)[:20]:20} ours {rname(ours, cls):5} target {rname(targ, cls):5}"
                         + (f"  (uses disagree: {uses})" if uses else ""))
        else:
            lines.append(f"  v{v:<4} {g0.name(v)[:20]:20} ours {rname(ours, cls):5} target {rname(targ, cls):5}"
                         f"  uses disagree: {uses} -- the most frequent is ours")
    for v in sorted(tcol):
        if is_saved(tcol[v], cls) or is_saved(colour[v], cls):
            want[v] = rname(tcol[v], cls)
    differ = sum(1 for _, o, t, _ in rows if o != t)
    agree = sum(1 for _, o, t, u in rows if o == t and u)
    tail = ""
    if colour is not recorded:
        differ0 = sum(1 for v in tcol if tcol[v] != recorded[v])
        tail = f"; scored against the colouring --move/--drop give (as recorded: {differ0} differ)"
    lines.append(f"  {len(tcol)} virtual registers mapped, {differ} differ, {len(conflicts)} with disagreeing "
                 f"uses ({agree} of them where the most frequent is ours), {unmatched} PCode instructions "
                 f"unmatched{tail}")
    return lines, want, differ


def load(path) -> list[dict]:
    return [json.loads(ln) for ln in open(path)]


def build_note(recs: list[dict]) -> str | None:
    """The line that says a dump came from another build than its unit's (igdump --compiler, GCC round
    44a); None for a dump of the unit's own build."""
    r = next((r for r in recs if r.get('built_by')), None)
    if r is None:
        return None
    return (f"NOTE: this dump came from {r['built_by']}, not the unit's own {r['unit_compiler']} (igdump "
            f"--compiler): its colourings are that build's, not the build the unit is scored under")


def target_compiler(recs: list[dict], unit: Path, override: str | None) -> tuple[str | None, str | None]:
    """(the toolchain --target's candidate is built by, in place of the unit's directive -- None: its
    own -- and the line saying so). --compiler, else the build the dump's records name, since the
    candidate's code is aligned with the record's PCode: a candidate from another build than the
    dump's misaligns wherever the two builds' code differs."""
    here = Path(__file__).resolve().parent.parent
    sys.path.insert(0, str(here))
    import scdiff as D
    import sccheck
    own = sccheck.tu_options(Path(unit).resolve()).get('compiler', D.COMPILER)
    made = next((r['built_by'] for r in recs if r.get('built_by')), None)
    use = override or made or own
    other = f" (the dump's own build, {made}, is another: the alignment is approximate)" if made and made != use else ""
    if use == own:
        return None, f"NOTE: --target's candidate built by the unit's own {own}{other}" if other else None
    return use, f"NOTE: --target's candidate built by {use}, not the unit's own {own}{other}"


def later_end(recs: list[dict], k: int) -> int | None:
    """The local area's size after the last spill pass of record k's function (its records follow it in
    the dump with the same procedure and frame), for stack_slots' `later_end`; None if k's is the last."""
    rec, last = recs[k], None
    for r in recs[k + 1:]:
        if (r.get('proc') != rec.get('proc') or r.get('link') != rec.get('link')
                or r.get('frame') != rec.get('frame')):
            break
        if r.get('spills'):
            last = r['spills']['end']
    return last


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
    ap.add_argument("-f", "--func", help="only the records of the functions matching this regex, searched in the "
                    "bare name igdump read with its @0xK suffix stripped (`-f '^Read$'`; every record is named "
                    "since GCC round 39a's igdump -- an older dump's 24-character-or-longer names are '')")
    ap.add_argument("--link", help="only the records whose MANGLED name (igdump's `link`, GCC round 39a) matches "
                    "this regex")
    ap.add_argument("--cls", type=int, default=-1, help="register class: 0 GPR, 1 FPR, -1 both (default)")
    ap.add_argument("-r", "--record", type=int, help="the Nth selected record (with --move/--want/--nb)")
    ap.add_argument("--watch", help="nodes whose colours to print (indices or variables, comma-separated)")
    ap.add_argument("--move", action="append", default=[], metavar="V=POS", help="renumber V to POS (e.g. 87=40.5)")
    ap.add_argument("--drop", action="append", default=[], metavar="A-B", help="remove the interference A-B")
    ap.add_argument("--want", help="wanted registers, V=reg,... (with --movers)")
    ap.add_argument("--movers", help="the nodes --want may renumber, comma-separated")
    ap.add_argument("--permute", action="store_true",
                    help="with --movers: try only the movers' orders among their own slots (k! placements; GCC "
                         "round 42a) instead of every half step for each")
    ap.add_argument("--nb", help="list this node's neighbours")
    ap.add_argument("--target", nargs=2, metavar=("UNIT", "FUNC"),
                    help="read each virtual register's register in the target (FUNC a map name UNIT defines) and "
                         "score the colouring --move/--drop give against it; with --movers, the saved registers "
                         "it gives are the --want")
    ap.add_argument("--compiler", metavar="BUILD",
                    help="build --target's candidate with this toolchain, not the one the unit's directive names "
                         "(default: the build the dump's records name, igdump --compiler; GCC round 44a); the "
                         "output says which build the dump and the candidate came from")
    ap.add_argument("--timeout", type=float, default=300.0,
                    help="a --want/--movers search's budget in seconds (default 300; 0: none)")
    ap.add_argument("--max-iters", type=int, help="a search's budget in placements replayed")
    ap.add_argument("--progress", type=float, default=5.0,
                    help="seconds between a search's progress lines on stderr (default 5; 0: none)")
    a = ap.parse_args(argv)
    recs = load(a.dump)
    note = build_note(recs)
    if note:
        print(note)
    rx = re.compile(a.func) if a.func else None
    lrx = re.compile(a.link) if a.link else None
    sel = [(k, r) for k, r in enumerate(recs)
           if (a.cls < 0 or r['cls'] == a.cls) and r['cls'] in VOLATILE
           and (rx is None or rx.search((r.get('proc') or '').split('@0x')[0]))
           and (lrx is None or (r.get('link') and lrx.search(r['link'])))]
    if a.permute and not (a.movers and (a.want or a.target)):
        raise SystemExit("--permute needs --movers, and --want or --target")
    single = a.move or a.drop or a.want or a.nb or a.watch or a.target
    if not single:
        bad_recs = nslots = 0
        for k, r in sel:
            g = Graph(r)
            bad, same = g.check()
            slots = check_slots(r)
            nslots += slots is not None
            bad_recs += bool(bad) or not same or slots is False
            print(f"[{k}] {(r.get('proc') or '?').split('@0x')[0]} cls {r['cls']}: {r['nused'] - r['nreal']} virtual, "
                  f"K {g.K}: order {'same' if same else 'DIFFERS'}, colour mismatches {len(bad)} {bad[:6] if bad else ''}"
                  + ("" if slots is None else f", stack slots {'same' if slots else 'DIFFER'}"))
        print(f"{len(sel) - bad_recs} of {len(sel)} records replay exactly"
              + (f" ({nslots} with spill slots, each checked)" if nslots else ""))
        return 1 if bad_recs else 0
    if a.record is None and len(sel) != 1:
        raise SystemExit(f"{len(sel)} records selected: name one with -r N (N counts the selected, from 0)")
    k, rec = sel[a.record or 0]
    cls = rec['cls']
    print(f"[{k}] {(rec.get('proc') or '?').split('@0x')[0]} cls {cls}: {rec['nused'] - rec['nreal']} virtual")
    if a.nb:
        print("\n".join(neighbours(rec, node_index(rec, a.nb))))
        return 0
    moves = {}
    for m in a.move:
        v, p = m.split('=')
        moves[node_index(rec, v)] = float(p)
    drop = []
    for d in a.drop:
        x, y = d.split('-')
        drop.append((node_index(rec, x), node_index(rec, y)))
    want = {}
    late = later_end(recs, k)
    if rec.get('spills'):
        print("  stack slots (the slot pass: each spilled node in index order, bumped down the local area):")
        print(f"    as recorded  {slots_line(stack_slots(rec, later_end=late))}")
        if moves or drop:
            moved, newidx, _ = replay_moved(rec, moves, drop)
            print(f"    moved        {slots_line(stack_slots(rec, moved, newidx, later_end=late))}")
    if a.target:
        if not any(not isinstance(ins, str) for ins in rec.get('code') or ()):
            raise SystemExit(f"record [{k}] holds no PCode: igdump recorded the GPR class's alone before GCC "
                             "round 35a -- dump the unit again for its FPR records' PCode")
        built, note = target_compiler(recs, Path(a.target[0]), a.compiler)
        if note:
            print(note)
        code = final_code(Path(a.target[0]), a.target[1], compiler=built)
        tcol, conflicts, unmatched = target_colours(rec, Path(a.target[0]), a.target[1], code=code)
        lines, want, _ = target_report(rec, tcol, conflicts, unmatched, moves, drop)
        print("\n".join(lines))
        if rec.get('spills'):
            print("\n".join(target_slot_report(rec, code, moves, drop, late)[0]))
        if not a.movers and not a.watch:
            return 0
    if a.want or (want and a.movers):
        if a.want:
            want = {node_index(rec, kv.split('=')[0]): kv.split('=')[1] for kv in a.want.split(',')}
        movers = [node_index(rec, x) for x in (a.movers or '').split(',') if x]
        if not movers:
            raise SystemExit("--want needs --movers")
        if a.permute and len(set(movers)) != len(movers):
            raise SystemExit(f"--permute: a mover named twice ({', '.join(f'v{m}' for m in movers)})")
        total = placements(rec, movers, moves, a.permute)[0]
        budget = []
        if a.timeout:
            budget.append(f"{a.timeout:g} s")
        if a.max_iters is not None:
            budget.append(f"{a.max_iters:,} placements")
        print(f"searching {total:,} placements of {', '.join(f'v{m}' for m in movers)}"
              + (" -- their orders among their own slots, "
                 + " ".join(f"{p:g}" for p in mover_slots(movers, moves)) if a.permute else "")
              + (f" from --move/--drop's numbering" if moves or drop else "")
              + f" (budget: {' or '.join(budget) if budget else 'none'})", flush=True)

        def progress(tried, total, hits, elapsed):
            left = (total - tried) * elapsed / tried if tried else 0
            print(f"  searched {tried:,} of {total:,} placements ({100 * tried / total:.1f} %), {hits} hits, "
                  f"{elapsed:.0f} s, ~{left / 60:.1f} min to go", file=sys.stderr, flush=True)
        res = search(rec, want, movers, base=moves, drop=drop, max_iters=a.max_iters,
                     timeout=a.timeout or None, progress=progress if a.progress > 0 else None,
                     every=a.progress or 5.0, permute=a.permute)
        print(f"{len(res.hits)} placements of {', '.join(f'v{m}' for m in movers)} give "
              + ", ".join(f"v{k}={v}" for k, v in want.items()))
        for h in res.hits[:60]:
            # a permutation's slots are whole indices (or a --move's positions): `v41->44`, a --move each
            print("  ", "  ".join(f"v{m}->{p:g}" if a.permute else f"v{m}->{p}" for m, p in zip(movers, h)))
        if res.stopped in ('timeout', 'max-iters'):
            print(f"STOPPED by the {'--timeout' if res.stopped == 'timeout' else '--max-iters'} budget after "
                  f"{res.tried:,} of {res.total:,} placements ({res.elapsed:.0f} s): the hits are of the part searched")
            return 2
        return 0
    watch = [node_index(rec, x) for x in a.watch.split(',')] if a.watch else []
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
