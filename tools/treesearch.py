#!/usr/bin/env python3
"""treesearch.py SPEC.py: which source trees of a sum give the target's (README "The GCC front", the
recogniser's round 18c: 2.96's fold regroups a sum, so the target's grouping is not the source's).

Every binary tree over the spec's leaves, in every order (30,240 for six), is written as one function
each into batch files, compiled under a GCC directive with cc1's RTL dump (tools/ccdump.py --dump=-dr),
and each function's expanded dataflow is rebuilt from the `.rtl` dump -- what GCC computed, after
fold and before any optimisation pass -- and compared with the wanted one. Promoted from the
recogniser lane's build/scratch/gcc-r18c/lang/tsearch2.py (GCC round 20a): the directive and the
prelude come from the spec, the batches go under build/scratch/gcc/treesearch/.

SPEC (a Python file) defines:
  LEAVES     {name: C expression}            the sum's terms
  ORDER      [names]                         the leaves permuted (default: every leaf)
  OFF        {offset: name}                  word loads off the parameter (offset 0: a bare (mem (reg)))
  PARAM      name                            the parameter's own leaf name (a0's register)
  PROTO      'int *f%d(CDB *cdb)'            the function's type and parameter, %d its number
  WANT       '((a*b)+(c<<2))'                the wanted dataflow, in the dump's spelling (as printed
                                             under `shapes seen` -- run once without a WANT to see them)
and optionally WRAP ('cdb->first + {}': the tree inside an expression), FIXED (more trees, tried
first), ONLY_FIXED, PRELUDE (C text before the functions: the types), PRELUDE_FILE (a file beside
the spec, inlined), DIRECTIVE (default the recogniser's: ee-gcc2.96 -O2 -G0 -fno-strict-aliasing
-funroll-loops), SHOW (how many dataflow shapes to list), BATCH (functions a file, default 1200).

  ./tools/treesearch.py build/scratch/gcc-r18c/lang/t_pm2.py
"""
import itertools, re, subprocess, sys
from pathlib import Path

D = Path(__file__).resolve().parents[1]
spec = Path(sys.argv[1]).resolve()
ns = {}
exec(spec.read_text(), ns)
LEAVES, OFF, PARAM, PROTO, WANT = ns['LEAVES'], ns['OFF'], ns['PARAM'], ns['PROTO'], ns.get('WANT', '')
ORDER = ns.get('ORDER', list(LEAVES))
WRAP = ns.get('WRAP', '{}')
PRELUDE = ns.get('PRELUDE', '')
if ns.get('PRELUDE_FILE'):
    PRELUDE = (spec.parent / ns['PRELUDE_FILE']).read_text() + '\n' + PRELUDE
DIRECTIVE = ns.get('DIRECTIVE', '/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing -funroll-loops */')
B = ns.get('BATCH', 1200)


def trees(items):
    if len(items) == 1:
        yield items[0]
        return
    for i in range(1, len(items)):
        for l in trees(items[:i]):
            for r in trees(items[i:]):
                yield f"({l}+{r})"


cands = ns.get('FIXED', [])[:]
if not ns.get('ONLY_FIXED'):
    for perm in itertools.permutations(ORDER):
        cands.extend(trees(list(perm)))
print(len(cands), "trees", file=sys.stderr)


def to_c(t):
    out = t
    for k in sorted(LEAVES, key=len, reverse=True):
        out = re.sub(r'(?<![\w>])%s(?!\w)' % re.escape(k), '(' + LEAVES[k] + ')', out)
    return WRAP.format(out.replace('+', ' + '))


R = r"\(reg(?:/\w)*:SI (\d+)(?: \w+)?\)"


def parse_rtl(txt):
    """each function's value returned in v0, as an expression over OFF's loads and PARAM"""
    res = {}
    for fm in re.finditer(r";; Function (\w+)(.*?)(?=\n;; Function |\Z)", txt, re.S):
        fn, body = fm.group(1), re.sub(r'\s+', ' ', fm.group(2))
        val = {4: PARAM}
        insns = re.findall(r"\((?:insn|call_insn) \d+ \d+ \d+ (\(.*?\)) -?\d+ \((?:nil|expr_list|insn_list)", body)
        m0 = re.search(r"\(set %s \(reg:SI 4 a0\)\)" % R, body)
        if m0:
            val[int(m0.group(1))] = PARAM
        for _ in range(2):
            for ins in insns:
                m = re.match(r"\(set %s (.*)\)$" % R, ins)
                mp = re.match(r"\(parallel ?\[ ?\(set %s \(mult:SI %s %s\)\)" % (R, R, R), ins)
                if mp:
                    val[int(mp.group(1))] = f"({val.get(int(mp.group(2)), '?')}*{val.get(int(mp.group(3)), '?')})"
                    continue
                if not m:
                    continue
                r, e = int(m.group(1)), m.group(2)
                mm = re.match(r"\(mem(?:/\w)*:SI \(plus:SI %s \(const_int (-?\d+)" % R, e)
                if mm:
                    val[r] = OFF.get(int(mm.group(2)), '?%s' % mm.group(2))
                    continue
                mm = re.match(r"\(mem(?:/\w)*:SI %s" % R, e)
                if mm:
                    val[r] = OFF.get(0, '?0')
                    continue
                mm = re.match(r"%s$" % R, e)
                if mm:
                    val[r] = val.get(int(mm.group(1)), 'r' + mm.group(1))
                    continue
                mm = re.match(r"\((plus|mult|ashift|minus):SI %s (?:%s|\(const_int (-?\d+)[^)]*\))\)$" % (R, R), e)
                if mm:
                    a = val.get(int(mm.group(2)), 'r' + mm.group(2))
                    b = val.get(int(mm.group(3)), 'r' + mm.group(3)) if mm.group(3) else mm.group(4)
                    op = {'plus': '+', 'mult': '*', 'ashift': '<<', 'minus': '-'}[mm.group(1)]
                    val[r] = f"({a}{op}{b})"
        m = re.search(r"\(set \(reg/i:SI 2 v0\) %s\)" % R, body)
        if m:
            res[fn] = val.get(int(m.group(1)), '?')
    return res


work = D / 'build' / 'scratch' / 'gcc' / 'treesearch'
work.mkdir(parents=True, exist_ok=True)
hits, seen = [], {}
for bi in range(0, len(cands), B):
    chunk = cands[bi:bi + B]
    src = work / 'ts_batch.c'
    lines = [DIRECTIVE, PRELUDE]
    for i, t in enumerate(chunk):
        lines.append((PROTO % (bi + i)) + f" {{ return {to_c(t)}; }}")
    src.write_text('\n'.join(lines) + '\n')
    out = work / 'dumps'
    r = subprocess.run([sys.executable, str(D / 'tools' / 'ccdump.py'), str(src), '--dump=-dr', '--out', str(out)],
                       capture_output=True, text=True, cwd=D)
    # 2.96 names its dumps <src>.NN.<pass>, the 2.9 builds <src>.<pass>
    dumps = sorted(out.glob('ts_batch.c*.rtl'))
    if not dumps:
        print(r.stdout[-2000:], r.stderr[-2000:])
        sys.exit(1)
    res = parse_rtl(dumps[0].read_text())
    for i, t in enumerate(chunk):
        e = res.get(f"f{bi + i}", '?')
        seen.setdefault(e, []).append(t)
        if WANT and e.replace(' ', '') == WANT.replace(' ', ''):
            hits.append(t)
if WANT:
    print(f"{len(hits)} source trees give {WANT}")
for h in hits[:2000]:
    print("  ", h, " => ", to_c(h))
if not hits or ns.get('SHOW'):
    print("shapes seen (count, first source):")
    for e, ts in sorted(seen.items(), key=lambda x: -len(x[1]))[:ns.get('SHOW', 12)]:
        print("  ", len(ts), e, "  <=", ts[0])
