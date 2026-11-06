/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing -funroll-loops */
/* spcontext: the recogniser's second-pass context (OVL2 0x4D0BD8-0x4D1C18, SOCOM 1's object at 0x33F590:
 * CreateSecondPassContext, FreeSecondPassContext), ScanSoft's speech recogniser, prebuilt with ee-gcc
 * 2.96 at -O2 -G0 -fno-strict-aliasing -funroll-loops (README "The GCC front"). The file's name is ours.
 *
 * The second pass re-recognises over a context built from the first pass's N best: a tree with a node
 * per pronunciation of each word of each result (common prefixes shared), then the items -- the
 * words' own, and, between two words whose classes the language's transcribed words name, one more for
 * each such word -- with their syntax, states and properties. */

#include "casr.h"

extern void *heap_Alloc(void *heap, unsigned int size);
extern void heap_Free(void *heap, void *p);
extern void *memset(void *dst, int c, unsigned int n);
extern void *memcpy(void *dst, const void *src, unsigned int n);

/* the context the second pass runs and builds (ctxinfo.c's CTXINFO) */
typedef struct SPCTX {
    int f0;
    unsigned int n;                     /* +4 */
    int nstates;                        /* +8 */
    unsigned short *props;              /* +12 */
    unsigned short *dict;               /* +16 */
    unsigned short *wtp;                /* +20 */
    int *bounds;                        /* +24 */
    int *tab;                           /* +28 */
    unsigned char *mask;                /* +32 */
} SPCTX;

extern unsigned char CreateMenuMasks(CASR *c, SPCTX *m, unsigned int n);
void FreeSecondPassContext(CASR *c);

/* the N best as the N-best block sends them (casr.c's) */
typedef struct NBRWORD {
    int state;                          /* +0: the word's end state, an index into the lexicon */
    int score;                          /* +4 */
    int end;                            /* +8 */
    int f12;                            /* +12 */
    struct NBRWORD *next;               /* +16 */
} NBRWORD;

typedef struct NBRESULT {
    int f0;
    int score;                          /* +4 */
    int f8;
    NBRWORD *words;                     /* +12 */
    struct NBRESULT *next;              /* +16 */
} NBRESULT;

/* a word's record for the second pass (ctxinfo.c's) */
typedef struct SPINFO {
    int state;                          /* +0 */
    unsigned short len;                 /* +4 */
    unsigned char first;                /* +6 */
    unsigned char last;                 /* +7 */
} SPINFO;

/* the tree of the N best: a node per pronunciation, each with its list of followers */
typedef struct SPNODE {
    int id;                             /* +0 */
    int pron;                           /* +4 */
    unsigned short link;                /* +8 */
} SPNODE;

typedef struct SPLINK {
    unsigned short node;                /* +0 */
    unsigned short next;                /* +2 */
    unsigned short cross;               /* +4 */
} SPLINK;

/* the states of a transcribed word's pronunciations [b, bend) appended at *po, each state shifted left two
 * and marked 2, the last marked 1, then the item's id in two halves; each pronunciation's first state in
 * *poff. An inline: its pointer arguments are the caller's frame locals, loop.c hoists their addresses
 * (`move t8,sp; addiu t6,sp,4; addiu s6,sp,8`) and its pointer-sum arguments are expanded shift first
 * (round 37c: `addu t0,v1,a1`). `*ptotal`'s sum stays `tot + (n + 2)` because *ptotal is unsigned and the
 * count an int: the conversion keeps the folder from regrouping it. */
static __inline__ void PutCrossWord(unsigned short *lex, unsigned short *b, unsigned short *bend, unsigned int id,
                                    int **poff, unsigned int *ptotal, unsigned short **po)
{
    unsigned short *s = lex + *b;
    int *off = *poff;
    unsigned short *o = *po;

    while (b < bend) {
        unsigned short *e;

        *off++ = *ptotal;
        b++;
        e = lex + *b;
        *ptotal += (e - s) + 2;
        e--;
        while (s < e)
            *o++ = (*s++ << 2) | 2;
        *o++ = (*s++ << 2) | 1;
        o[0] = id;
        o[1] = id >> 16;
        o += 2;
    }
    *po = o;
    *poff = off;
}

/* The tree's search (round 41c) is a `for (;;)` that holds the failure's arm, so its exit test cannot be
 * copied above the loop (duplicate_loop_exit_test refuses a CODE_LABEL in the region: the target's entry
 * `b` to a test at the bottom) and the match block has no barrier at the outer loop's depth to be moved to
 * (find_and_verify_loops: it stays in the loop). The outer `for (;;)` runs to the `break` after the whole
 * chain: the first pass counts more than 124 real insns in it, so its test's constant is not hoisted
 * (`li v0,0xffff; bne s3,v0`, the nop at its top the loop label's alignment). The head's constants are in
 * the order the first scheduling pass needs: the ALU slots after the two loads, a stack store (after reload)
 * never passing a load (no strict aliasing). */
int CreateSecondPassContext(CASR *c)
{
    int *off;
    unsigned int total;
    unsigned short *o;
    SPNODE *nodes;
    SPLINK *links;
    SPCTX *rc;
    int *tab;
    unsigned short *props;
    unsigned short *dict;
    int tab0;
    unsigned int nitems;
    unsigned int nn;
    unsigned int nextid;
    int nstates;
    SPINFO *info;
    int nclasses;
    unsigned short *lex;
    unsigned short *beg;
    int *b;
    unsigned short *p;
    SPCTX *sp;
    NBRESULT *r;
    NBRWORD *w;
    unsigned int npron;
    unsigned int maxpron;
    unsigned short nlinks;
    unsigned short nnodes;
    unsigned int nroots;
    unsigned int prev, first, id, lnk, np, prevnp, i, j;
    int nsyn, nxstates;
    void *tw;
    int *syn;
    unsigned short *wtp;
    WORDMAP *map;
    int *t;
    SPNODE *nd;
    unsigned int size4, size2;
    unsigned short *q;

    npron = 0;
    maxpron = 0;
    nodes = 0;
    links = 0;
    nlinks = 0;
    nnodes = 1;
    nroots = 0;
    rc = c->run_ctx;
    r = *(NBRESULT **)c->sp_arg1;
    tab = rc->tab;
    tab0 = *tab;
    props = rc->props;
    dict = rc->dict;
    nstates = 0;
    info = c->f588;
    nclasses = c->lng->GetNbrSpeechUnitClass(c->lng);
    tw = c->lng->GetpTransWord(c->lng);
    lex = c->lng->GetpTransWordLex(tw);
    beg = c->lng->GetpBeginOfTransWords(tw);
    total = 0;
    if (c->sp_ctx != 0)
        FreeSecondPassContext(c);
    sp = heap_Alloc(c->heap, sizeof(SPCTX));
    if (sp == 0)
        goto fail;
    memset(sp, 0, sizeof(SPCTX));
    c->sp_ctx = sp;
    for (; r != 0; r = r->next) {
        for (w = r->words; w != 0; ) {
            unsigned int x;
            unsigned int n;

            x = dict[w->state + 1] + (dict[w->state + 2] << 16);
            w = w->next;
            n = tab[x + 1] - tab[x];
            npron += n;
            if (maxpron < n)
                maxpron = n;
        }
    }
    nodes = heap_Alloc(c->heap, (npron + 1) * sizeof(SPNODE));
    links = heap_Alloc(c->heap, npron * maxpron * sizeof(SPLINK));
    if (nodes == 0)
        goto fail2;
    if (links == 0)
        goto fail;
    nodes[0].link = -1;
    for (r = *(NBRESULT **)c->sp_arg1; r != 0; r = r->next) {
        w = r->words;
        prev = 0;
        first = 0;
        id = 0;
        if (w != 0) {
            for (;;) {
                if (w != 0) {
                lnk = nodes[prev].link;
                id = dict[w->state + 1] + (dict[w->state + 2] << 16);
                first = tab[id] - tab0;
                for (;;) {
                    unsigned int n;

                    if (lnk == 0xFFFF) {
                        if (prev != 0)
                            prevnp = tab[nodes[prev].id + 1] - tab[nodes[prev].id];
                        else
                            prevnp = 1;
                        break;
                    }
                    n = links[lnk].node;
                    if (nodes[n].pron == first) {
                        prev = n;
                        w = w->next;
                        break;
                    }
                    lnk = links[lnk].next;
                }

                if (lnk != 0xFFFF)
                    continue;
                }
            lnk = nodes[prev].link;
            while (w != 0) {
                np = tab[id + 1] - tab[id];
                for (i = 0; i < np; i++) {
                    for (j = 0; j < prevnp; j++) {
                        links[nlinks + j].node = nnodes + i;
                        if (lnk != 0xFFFF)
                            links[nlinks + j].next = lnk + j;
                        else
                            links[nlinks + j].next = -1;
                    }
                    lnk = nlinks;
                    if (i == np - 1) {
                        for (j = 0; j < prevnp; j++)
                            nodes[prev + j].link = nlinks + j;
                    }
                    nodes[nnodes + i].pron = first + i;
                    nodes[nnodes + i].id = id;
                    nlinks += prevnp;
                }
                prev = nnodes;
                w = w->next;
                nnodes += np;
                prevnp = np;
                if (w == 0)
                    break;
                lnk = 0xFFFF;
                id = dict[w->state + 1] + (dict[w->state + 2] << 16);
                first = tab[id] - tab0;
            }
            for (i = 0; i < prevnp; i++)
                nodes[prev + i].link = -1;
            break;
            }
        }
    }
    for (lnk = nodes[0].link; lnk != 0xFFFF; lnk = links[lnk].next)
        nroots++;
    nitems = nnodes - 1;
    nsyn = nlinks - nroots;
    nxstates = nitems;
    nn = nxstates;
    nextid = nxstates;
    for (i = 1; i <= nn; i++) {
        lnk = nodes[i].link;
        if (lnk != 0) {
            unsigned int last;
            SPINFO *f = &info[nodes[i].pron];

            last = f->last;
            nstates += f->len;
            for (; lnk != 0xFFFF; lnk = links[lnk].next) {
                unsigned short x = last * nclasses + info[nodes[links[lnk].node].pron].first;
                unsigned int e = beg[x + 1];
                unsigned int a = beg[x];
                int n = beg[e] - beg[a];

                if (n != 0) {
                    nstates += n;
                    nxstates += e - a;
                    nitems++;
                    nsyn++;
                } else {
                    x = 0xFFFF;
                }
                links[lnk].cross = x;
            }
        }
    }
    if (CreateMenuMasks(c, sp, nitems) != 0)
        goto fail;
    sp->bounds = heap_Alloc(c->heap, (nitems + nsyn) * 4 + 4);
    if (sp->bounds == 0)
        goto fail;
    b = sp->bounds;
    size4 = nitems * 4;
    size2 = nitems * 2;
    syn = (int *)((char *)b + size4 + 4);
    q = heap_Alloc(c->heap, size2);
    sp->props = q;
    if (q == 0)
        goto fail;
    p = q;
    memset(q, 0, size2);
    sp->wtp = heap_Alloc(c->heap, size2);
    if (sp->wtp == 0)
        goto fail;
    wtp = sp->wtp;
    sp->tab = heap_Alloc(c->heap, (nitems + nxstates) * 4 + 8);
    if (sp->tab == 0)
        goto fail;
    t = sp->tab;
    nstates += nxstates * 2;
    off = (int *)((char *)t + size4 + 4);
    sp->dict = heap_Alloc(c->heap, nstates * 2);
    tab = t;
    if (sp->dict == 0)
        goto fail;
    o = sp->dict;
    c->map = heap_Alloc(c->heap, nitems * 8);
    if (c->map == 0)
        goto fail;
    total = 0;
    map = c->map;
    for (lnk = nodes[0].link; lnk != 0xFFFF; lnk = links[lnk].next)
        q[links[lnk].node - 1] = 1;
    for (i = 1; i <= nn; i++) {
        unsigned int k;
        unsigned int prop;
        unsigned int len;
        SPINFO *f;
        unsigned short *ds;

        nd = &nodes[i];
        k = i - 1;
        prop = props[nd->id];
        lnk = nd->link;
        map->id = nd->id;
        map->word = nd->pron;
        map++;
        *b = syn - sp->bounds;
        b++;
        *off = total;
        *t = off - tab;
        t++;
        off++;
        len = info[nd->pron].len;
        ds = o;
        memcpy(ds, dict + info[nd->pron].state, len * 2);
        ds += len;
        *(ds - 1) |= 1;
        ds[1] = k >> 16;
        ds[0] = k;
        ds += 2;
        o = ds;
        len += 2;
        total += len;
        if (lnk != 0) {
            if (lnk == 0xFFFF)
                *p |= 2;
            else if (*p != 1)
                *p = prop;
            if (*p++ & 0x80)
                *wtp++ = c->wtp;
            else
                *wtp++ = 0;
            for (; lnk != 0xFFFF; lnk = links[lnk].next) {
                if (links[lnk].cross == 0xFFFF)
                    *syn++ = links[lnk].node - 1;
                else
                    *syn++ = nextid++;
            }
        }
    }
    *b = syn - sp->bounds;
    b++;
    nextid = nn;
    *t = off - tab;
    t++;
    for (i = 1; i <= nn; i++) {
        lnk = nodes[i].link;
        if (lnk != 0 && lnk != 0xFFFF) {
            do {
                unsigned int x = links[lnk].cross;

                if (x != 0xFFFF) {
                    map->id = -1;
                    map->word = -1;
                    map++;
                    PutCrossWord(lex, beg + beg[x], beg + beg[x + 1], nextid++, &off, &total, &o);
                    *t = off - tab;
                    t++;
                    *syn++ = links[lnk].node - 1;
                    *b = syn - sp->bounds;
                    b++;
                    *wtp++ = 0;
                }
                lnk = links[lnk].next;
            } while (lnk != 0xFFFF);
        }
    }
    *off++ = total;
    heap_Free(c->heap, nodes);
    heap_Free(c->heap, links);
    sp->f0 = 0;
    sp->n = nitems;
    sp->nstates = nstates;
    return 0;
fail:
    if (nodes != 0)
        heap_Free(c->heap, nodes);
fail2:
    if (links != 0)
        heap_Free(c->heap, links);
    FreeSecondPassContext(c);
    return 1;
}

void FreeSecondPassContext(CASR *c)
{
    SPCTX *sp;

    if (c->map != 0) {
        heap_Free(c->heap, c->map);
        c->map = 0;
    }
    sp = c->sp_ctx;
    if (sp != 0) {
        if (sp->tab != 0)
            heap_Free(c->heap, sp->tab);
        if (((SPCTX *)c->sp_ctx)->dict != 0)
            heap_Free(c->heap, ((SPCTX *)c->sp_ctx)->dict);
        if (((SPCTX *)c->sp_ctx)->bounds != 0)
            heap_Free(c->heap, ((SPCTX *)c->sp_ctx)->bounds);
        if (((SPCTX *)c->sp_ctx)->wtp != 0)
            heap_Free(c->heap, ((SPCTX *)c->sp_ctx)->wtp);
        if (((SPCTX *)c->sp_ctx)->props != 0)
            heap_Free(c->heap, ((SPCTX *)c->sp_ctx)->props);
        if (((SPCTX *)c->sp_ctx)->mask != 0)
            heap_Free(c->heap, ((SPCTX *)c->sp_ctx)->mask);
        heap_Free(c->heap, c->sp_ctx);
        c->sp_ctx = 0;
    }
}
