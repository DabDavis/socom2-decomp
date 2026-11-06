#!/usr/bin/env python3
"""
igsearch -- search a tools/igdump.py record for the numbering, spill picks or interferences that give the wanted registers,
BEFORE any source is written (the AI core's round 37 iganneal.py, igallperm.py, igpicks.py, igedges.py, folded in by the GCC
lane's round 45a). tools/igreplay.py --want/--movers tries every placement exhaustively, or --permute every order: these are for
what that cannot finish, or a question it does not ask. Each subcommand runs igreplay's replay of Metrowerks' colouring only.

    ./tools/igsearch.py anneal  DUMP -f F -r 0 --want a=s0,b=s1 [--movers a,b,c] [--seconds 60] [--seed 37]
    ./tools/igsearch.py allperm DUMP -f F -r 0 --want a=s0,b=s1 --movers a,b,c,d [--seconds 60]
    ./tools/igsearch.py picks   DUMP -f F -r 0 --want a=s0,b=s1 [--edge a-b ...]
    ./tools/igsearch.py edges   DUMP -f F -r 0 --want a=s0,b=s1 [--seconds 200]

anneal: annealing over the order of the movers (default: the wanted nodes) among the slots they hold, scored by how many wanted
registers it gives: the nearest numbering, for ten or more movers (a search of 10! is hours). allperm: EVERY distinct order found
(all of them up to eight movers, restarts and local search above) and THE PRECEDENCES COMMON TO ALL -- what the source's declaration
order must respect, the question to ask before writing a line. picks: every sequence of spill picks over the record, numbering and
interferences as dumped (the pick's float tie-break taken as free): does any give the registers? edges: which added or dropped
interference among the wanted nodes, one then two, does? (`no` to all three means the graph, not its order, is off.)

Selection as igreplay's: -f (a regex on the bare name), --link (on the mangled name), --cls (0 GPRs, the default; 1 FPRs), -r N (the
Nth selected). A node is its index or its variable (igreplay.node_index). rc: 0 a hit, 1 none, 2 stopped by the budget without one.
"""

from __future__ import annotations

import argparse
import itertools
import math
import random
import re
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import igreplay as R


def select(recs: list[dict], func: str | None, link: str | None, cls: int, record: int | None) -> dict:
    """igreplay's selection: the record named, by -f / --link / --cls / -r."""
    rx = re.compile(func) if func else None
    lrx = re.compile(link) if link else None
    sel = [r for r in recs if r['cls'] == cls and (rx is None or rx.search((r.get('proc') or '').split('@0x')[0]))
           and (lrx is None or (r.get('link') and lrx.search(r['link'])))]
    if record is None and len(sel) != 1:
        raise SystemExit(f"{len(sel)} records selected: name one with -r N (N counts the selected, from 0)")
    return sel[record or 0]


def parse_want(rec: dict, text: str) -> dict[int, int]:
    """`a=s0,b=s1`: {node: register number}."""
    return {R.node_index(rec, kv.split('=')[0]): R._rnum(kv.split('=')[1], rec['cls']) for kv in text.split(',')}


def _misses(g: R.Graph, want: dict[int, int], movers: list[int], perm) -> int:
    """How many wanted registers the movers' slots `perm` miss. perm[i]: the slot movers[i] takes."""
    newidx = list(range(len(g.adj)))
    for m, s in zip(movers, perm):
        newidx[m] = s
    colour = g.permuted(newidx).replay()[0]
    return sum(colour[newidx[k]] != v for k, v in want.items())


def _names(g: R.Graph, nodes) -> str:
    return " ".join(g.name(n) or f"v{n}" for n in nodes)


def _slots(rec: dict, want: dict[int, int], movers: list[int] | None) -> tuple[list[int], list[int]]:
    movers = list(movers) if movers else sorted(want)
    bad = [m for m in movers if m < rec['nreal']]
    if bad:
        raise SystemExit(f"a hard register cannot move: {bad}")
    return movers, sorted(movers)


def anneal(rec: dict, want: dict[int, int], movers: list[int] | None = None, seconds: float = 60.0, seed: int = 37,
           steps: int = 4000) -> tuple[int, list[int], int]:
    """(the fewest misses, the movers in slot order at that score, the evaluations made)."""
    movers, slots = _slots(rec, want, movers)
    g = R.Graph(rec)
    rnd = random.Random(seed)
    best = None
    t0 = time.time()
    evals = 0
    while best is None or (best[0] and time.time() - t0 < seconds):
        cur = list(slots)
        rnd.shuffle(cur)
        cs = _misses(g, want, movers, cur)
        evals += 1
        if best is None or cs < best[0]:
            best = (cs, list(cur))
        T = 2.0
        for _ in range(steps if len(cur) > 1 else 0):
            if not cs or time.time() - t0 >= seconds:
                break
            a, b = rnd.sample(range(len(cur)), 2)
            cur[a], cur[b] = cur[b], cur[a]
            s = _misses(g, want, movers, cur)
            evals += 1
            if s <= cs or rnd.random() < math.exp((cs - s) / T):
                cs = s
            else:
                cur[a], cur[b] = cur[b], cur[a]
            T = max(0.05, T * 0.998)
            if cs < best[0]:
                best = (cs, list(cur))
        if len(cur) <= 1:
            break
    return best[0], [m for _, m in sorted(zip(best[1], movers))], evals


def allperm(rec: dict, want: dict[int, int], movers: list[int], seconds: float = 60.0, seed: int = 1,
            exhaustive: int = 8) -> tuple[list[list[int]], list[tuple[int, int]], bool]:
    """(every distinct order found that gives the registers, the movers in slot order; the pairs (a, b) with a before b in all of
    them; whether the search saw every order -- it does up to `exhaustive` movers, inside the budget)."""
    movers, slots = _slots(rec, want, movers)
    g = R.Graph(rec)
    sols: set[tuple] = set()
    t0 = time.time()
    complete = len(movers) <= exhaustive
    if complete:
        for p in itertools.permutations(slots):
            if time.time() - t0 > seconds:
                complete = False
                break
            if _misses(g, want, movers, p) == 0:
                sols.add(tuple(p))
    else:
        rnd = random.Random(seed)
        while time.time() - t0 < seconds:
            cur = list(slots)
            rnd.shuffle(cur)
            cs = _misses(g, want, movers, cur)
            T = 2.0
            for _ in range(3000):
                if time.time() - t0 > seconds:
                    break
                a, b = rnd.sample(range(len(cur)), 2)
                cur[a], cur[b] = cur[b], cur[a]
                s = _misses(g, want, movers, cur)
                if s <= cs or rnd.random() < math.exp((cs - s) / T):
                    cs = s
                else:
                    cur[a], cur[b] = cur[b], cur[a]
                T = max(0.05, T * 0.998)
                if cs == 0:
                    sols.add(tuple(cur))
    orders = [[m for _, m in sorted(zip(p, movers))] for p in sols]
    common = [(a, b) for a, b in itertools.permutations(movers, 2) if orders and all(o.index(a) < o.index(b) for o in orders)]
    return orders, common, complete


def _colours(g: R.Graph, adj, pushed) -> list[int]:
    colour = [-1] * len(adj)
    for i in range(g.nreal):
        colour[i] = i
    vol = set(g.volatile)
    nonvol = list(g.nonvol)
    for node in reversed(pushed):
        mask = set(vol)
        for j in adj[node]:
            c = colour[j]
            if c != -1 and c < g.nreal:
                mask.discard(c)
        if mask:
            colour[node] = min(mask)
        elif nonvol:
            colour[node] = nonvol.pop(0)
            vol.add(colour[node])
    return colour


def picks(rec: dict, want: dict[int, int], edges=(), limit: int = 400000) -> tuple[list[list[int]], int]:
    """(each sequence of spill picks that gives the registers, the complete pick sequences tried), numbering as dumped, `edges` added."""
    g = R.Graph(rec)
    adj = [list(a) for a in g.adj]
    for a, b in edges:
        adj[a].append(b)
        adj[b].append(a)
    n = len(adj)
    flags0 = list(g.flags) + [0] * (n - len(g.flags))
    hits: list[list[int]] = []
    seen: set = set()
    count = [0]

    def sweeps(deg, flags, pushed):
        while True:
            spilled, changed = [], False
            for i in range(g.nreal, n):
                if flags[i] & (R.F_PUSHED | R.F_COAL):
                    continue
                if deg[i] < g.K:
                    for j in adj[i]:
                        deg[j] -= 1
                    flags[i] |= R.F_PUSHED
                    pushed.append(i)
                    changed = True
                else:
                    spilled.append(i)
            if not changed:
                return spilled

    def dfs(deg, flags, pushed, path):
        spilled = sweeps(deg, flags, pushed)
        if not spilled:
            count[0] += 1
            c = _colours(g, adj, pushed)
            if all(c[k] == v for k, v in want.items()):
                hits.append(list(path))
            return
        key = frozenset(pushed)
        if key in seen or count[0] > limit:
            return
        seen.add(key)
        for best in sorted(spilled, reverse=True):
            d2, f2, p2 = list(deg), list(flags), list(pushed)
            for j in adj[best]:
                d2[j] -= 1
            f2[best] |= R.F_PUSHED
            p2.append(best)
            dfs(d2, f2, p2, path + [best])

    sys.setrecursionlimit(max(10000, sys.getrecursionlimit()))
    dfs([len(a) for a in adj], flags0, [], [])
    return hits, count[0]


def edge_search(rec: dict, want: dict[int, int], seconds: float = 200.0) -> tuple[list[int], list[tuple]]:
    """(the wanted nodes missed as dumped, each single or paired added/dropped interference among them that leaves none missed,
    as ((op, a, b), ...), op 'add' or 'drop')."""
    g = R.Graph(rec)
    base = [list(a) for a in g.adj]
    nodes = sorted(want)

    def miss(adj):
        colour = g.replay(adj=adj)[0]
        return [k for k, v in want.items() if colour[k] != v]

    def apply(ops):
        adj = [list(x) for x in base]
        for op, a, b in ops:
            if op == 'add':
                adj[a].append(b)
                adj[b].append(a)
            else:
                adj[a].remove(b)
                adj[b].remove(a)
        return adj

    ops = []
    for a, b in itertools.combinations(nodes, 2):
        if b in base[a]:
            ops.append(('drop', a, b))      # (two wanted in one register interfere: only a drop can give it)
        elif want[a] != want[b]:
            ops.append(('add', a, b))
    found = [(o,) for o in ops if not miss(apply([o]))]
    t0 = time.time()
    for x, y in itertools.combinations(ops, 2):
        if time.time() - t0 > seconds:
            break
        if not miss(apply([x, y])):
            found.append((x, y))
    return miss(base), found


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("command", choices=("anneal", "allperm", "picks", "edges"))
    ap.add_argument("dump", type=Path, help="tools/igdump.py's output")
    ap.add_argument("-f", "--func", help="a regex on the bare name")
    ap.add_argument("--link", help="a regex on the mangled name")
    ap.add_argument("--cls", type=int, default=0, help="0 GPRs (default), 1 FPRs")
    ap.add_argument("-r", "--record", type=int, help="the Nth selected record")
    ap.add_argument("--want", required=True, help="wanted registers, V=reg,...")
    ap.add_argument("--movers", help="the nodes to reorder, comma-separated (anneal: default the wanted)")
    ap.add_argument("--seconds", type=float, default=60.0, help="the budget (edges' pairs default to 60 too)")
    ap.add_argument("--seed", type=int, default=37)
    ap.add_argument("--edge", action="append", default=[], metavar="A-B", help="picks: an interference to add first")
    a = ap.parse_args(argv)
    rec = select(R.load(a.dump), a.func, a.link, a.cls, a.record)
    want = parse_want(rec, a.want)
    movers = [R.node_index(rec, x) for x in a.movers.split(',')] if a.movers else None
    g = R.Graph(rec)
    print(f"{(rec.get('proc') or '?').split('@0x')[0]} cls {rec['cls']}: K {g.K}, {len(g.adj) - g.nreal} virtual")
    if a.command == "anneal":
        score, order, evals = anneal(rec, want, movers, a.seconds, a.seed)
        print(f"{evals} evaluations: {score} of {len(want)} missed at best; the order: {_names(g, order)}")
        return 0 if score == 0 else 2
    if a.command == "allperm":
        if not movers:
            raise SystemExit("allperm needs --movers")
        orders, common, complete = allperm(rec, want, movers, a.seconds, a.seed)
        print(f"{len(orders)} orders give the registers ({'every order tried' if complete else 'a sample: the budget ended first'})")
        for o in orders[:20]:
            print("  ", _names(g, o))
        if common:
            print("always before:", " ".join(f"{g.name(x) or x}<{g.name(y) or y}" for x, y in common))
        return 0 if orders else (1 if complete else 2)
    if a.command == "picks":
        edges = [tuple(R.node_index(rec, y) for y in x.split('-')) for x in a.edge]
        hits, leaves = picks(rec, want, edges)
        print(f"{leaves} pick sequences, {len(hits)} give the registers")
        for h in hits[:12]:
            print("  ", _names(g, h) or "(no pick needed)")
        return 0 if hits else 1
    missed, found = edge_search(rec, want, a.seconds)
    print("missed as dumped:", _names(g, missed) or "none")
    for ops in found[:20]:
        print("  ", "; ".join(f"{op} {g.name(x) or x}-{g.name(y) or y}" for op, x, y in ops))
    return 0 if found or not missed else 1


if __name__ == "__main__":
    sys.exit(main())
