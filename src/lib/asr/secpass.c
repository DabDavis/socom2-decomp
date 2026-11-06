/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing -funroll-loops */
/* secpass: the recogniser's second-pass tree context (OVL2 0x4D1C18-0x4D3318, SOCOM 1's object at
 * 0x342708: GetSecondPassInfo, putWordInDict (LOCAL), CreateSecondPassTreeContext), ScanSoft's
 * speech recogniser, prebuilt with ee-gcc 2.96 at -O2 -G0 -fno-strict-aliasing -funroll-loops
 * (README "The GCC front"). Names are SOCOM 1's. The language and the context are reached through
 * their virtual tables (language.c's _LangVirtualTable, contextv2.c's CONTEXTV2), slot for slot.
 * Round 27c: GetSecondPassInfo; round 28c: putWordInDict; round 41c: CreateSecondPassTreeContext. */

#include "casr.h"

extern void *heap_Alloc(void *heap, unsigned int size);
extern void heap_Free(void *heap, void *p);
extern void *memset(void *dst, int c, unsigned int n);
extern void FreeSecondPassContext(CASR *c);

typedef struct CTXVT CTXVT;
struct CTXVT {
    void *data;
    void *s1[8];
    unsigned int (*GetNbrWord)(CTXVT *c);           /* +36 */
    void *s10[6];
    unsigned short *(*GetpLex)(CTXVT *c);           /* +64 */
    int *(*GetpBeginOfWords)(CTXVT *c);             /* +68 */
    unsigned short *(*GetpWordProp)(CTXVT *c);      /* +72 */
    void *s19[2];
};

extern void FillContextV2VirtualTable(CTXVT *vt);

typedef struct IMPORT {
    void *data;
    void *heap;
} IMPORT;

/* what the second pass keeps of a word: its states' count and its first and last speech units'
 * classes */
typedef struct SPINFO {
    int f0;
    unsigned short nstates;             /* +4 */
    unsigned char first;                /* +6 */
    unsigned char last;                 /* +7 */
} SPINFO;

/* the context the second pass builds (ctxinfo.c's CTXINFO) */
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

/* a tree context's result: a word, or (negative) ~ the first of a list of words (c->tree_words) */
typedef struct TREEWORD {
    int id;
    int f4;
} TREEWORD;

typedef struct WORDLIST {
    int word;
    int next;
} WORDLIST;

/* spcontext.c's tree of the N best */
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

/* a word's second-pass record from its first pronunciation: a pronunciation of one item (1) is
 * its speech units' states less two each, its ends' classes from the units' first and last
 * entries (the ends left out of the count when there are three or more); any other is its length
 * and the user-word class at both ends. SOCOM 1's body (round 27c): the word's pronunciation
 * table indexed through its own pointer (`w = bow + *bow; w[word]`: the target scales the word
 * first), the item count taken and the length formed before the test (both arms' `n`), the else
 * arm's three stores in the reverse of the target's order (sched1 issues the last of equal stores
 * first: written nstates, first, last they come out last, first, nstates -- 3 words). */
int GetSecondPassInfo(CASR *c, CTXVT *ctx, SPINFO *info, int word)
{
    int *su = c->lng->GetpSpeechUnit(c->lng);
    unsigned char cls = c->lng->GetUserWordSpeechUnitClass(c->lng);
    unsigned short *lex = ctx->GetpLex(ctx);
    int *bow = ctx->GetpBeginOfWords(ctx);
    int *w = bow + *bow;
    unsigned short *p = lex + w[word];
    unsigned short *end = lex + w[word + 1];
    int n;
    unsigned short type;

    type = *p++;
    n = end - p;
    if (type == 1) {
        int k = 0;

        end--;
        info->first = ((unsigned short *)su)[su[*p]];
        info->last = ((unsigned short *)su)[su[*end] + 1];
        if (n >= 3) {
            p++;
            end--;
        }
        for (; p <= end; p++)
            k += su[*p + 1] - su[*p] - 2;
        info->nstates = k;
    } else {
        info->last = cls;
        info->first = cls;
        info->nstates = n;
    }
    return 0;
}

/* a word's states appended to the dictionary at *out: each speech unit's states but its first two
 * (a one-item pronunciation, its ends dropped when three or more) or the pronunciation itself, each
 * shifted left two, the last marked with bit 0, then the word's id in two halves; *count grows by
 * the states and the id. SOCOM 1's body (0x1F4 there, its copies not unrolled), round 28c: the
 * state count before the states' address (the target's sll after the loads, then the subu), a
 * counter of its own per copy loop (one `j` is one pseudo for both: t0 where the target has t1 and
 * a1), the mark through `*(o - 1)` (its address one register for the load and the store: `o[-1]`
 * folds the -2 into both) and the id's halves at o[0], o[1] before one `o += 2`. */
static void putWordInDict(CASR *c, CTXVT *ctx, int word, int *count, unsigned short **out,
                          unsigned int id)
{
    int *su = c->lng->GetpSpeechUnit(c->lng);
    unsigned short *lex = ctx->GetpLex(ctx);
    int *bow = ctx->GetpBeginOfWords(ctx);
    int *w = bow + *bow;
    unsigned short *p = lex + w[word];
    unsigned short *end = lex + w[word + 1];
    unsigned short *o = *out;
    unsigned int n, k, j, i, total = 0;
    unsigned short type;

    type = *p++;
    n = end - p;
    if (type == 1) {
        end--;
        if (n >= 3) {
            p++;
            end--;
        }
        for (; p <= end; p++) {
            unsigned short *s;

            k = su[*p + 1] - su[*p] - 2;
            s = (unsigned short *)su + su[*p] + 2;
            for (j = 0; j < k; j++)
                *o++ = s[j] << 2;
            total += k;
        }
    } else {
        total = n;
        for (i = 0; i < n; i++)
            *o++ = p[i] << 2;
    }
    *(o - 1) |= 1;
    o[0] = id;
    o[1] = id >> 16;
    o += 2;
    *count += total + 2;
    *out = o;
}

/* the states of a transcribed word's pronunciations (spcontext.c's PutCrossWord) */
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

static __inline__ unsigned short IsStartWord(unsigned short f)
{
    return f & 1;
}

static __inline__ unsigned short IsStopWord(unsigned short f)
{
    return f & 2;
}

/* spcontext.c's CreateSecondPassContext over the words of a tree context, each word (or, negative, each list of
 * words: c->tree_words) between the context's start and stop words. SOCOM 1's body, round 41c. What the registers
 * said: the word and its list entries are one variable (`lw v1,0(s6)`, `lw v1,0(v0)`: its copy `sll v1,v1,2` gives the
 * pseudo the preference that takes v1, at every unrolled copy); the properties are indexed (a giv's start is emitted
 * after the hoisted -1: `li a1,-1` first, so reload takes a2 for tab0); so is the path (`addu t6,v0,s7`, index first);
 * the path's walk from its end counts (`path[n - j]`: biv j gone, `addiu a0,a0,-4`); `path[n++] = x` under the known
 * n = 1 is the address hoisted to 204(sp) -- written `path[1]` it is `sw v0,4(s7)`; the walk is a `for` rather than a `while`
 * (a `while` ending in `if (..) break;` is not rotated: expand_end_loop reuses the condjump, and the -1 of the walk's two tests
 * stays apart); the props walker of the first loop is not the items' p (21 refs against 15 flips i and p). */
int CreateSecondPassTreeContext(CASR *c)
{
    CTXVT ctx;
    SPINFO info;
    unsigned int total;
    unsigned short *o;
    int *off;
    SPNODE *nodes;
    SPLINK *links;
    unsigned int nroots;
    unsigned int nitems;
    unsigned int nn;
    unsigned int nextid;
    int nstates;
    int nclasses;
    TREEWORD *words;
    unsigned int nwords;
    int *tbase;
    int tab0;
    unsigned short *lex;
    unsigned short *beg;
    unsigned short *props;
    int start;
    int end;
    int *b;
    SPCTX *sp;
    unsigned int npron;
    unsigned int maxpron;
    unsigned short nlinks;
    unsigned short nnodes;
    TREEWORD *wp;
    void *tw;
    unsigned int nw;
    int *bow;
    unsigned int i, j, k;
    int *path;
    unsigned int prev, first, id, lnk, np, prevnp;
    int nsyn, nxstates;
    int *syn;
    unsigned short *wtp;
    WORDMAP *map;
    int *t;
    unsigned int size4, size2;
    unsigned short *q;
    unsigned short *p;

    maxpron = 0;
    nodes = 0;
    links = 0;
    nlinks = 0;
    nnodes = 1;
    npron = 0;
    nclasses = c->lng->GetNbrSpeechUnitClass(c->lng);
    nroots = 0;
    nstates = 0;
    words = *(TREEWORD **)c->sp_arg1;
    nwords = c->sp_arg2;
    wp = words;
    tw = c->lng->GetpTransWord(c->lng);
    start = -1;
    end = -1;
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
    FillContextV2VirtualTable(&ctx);
    ctx.data = c->ctx->data;
    nw = ctx.GetNbrWord(&ctx);
    props = ctx.GetpWordProp(&ctx);
    bow = ctx.GetpBeginOfWords(&ctx);
    tab0 = *bow;
    for (i = 0; i < nw; i++) {
        unsigned short f = props[i];

        if (IsStartWord(f)) {
            start = i;
            if (end != -1)
                break;
        }
        if (IsStopWord(f)) {
            end = i;
            if (start != -1)
                break;
        }
    }
    for (k = 0; k < nwords; k++, wp++) {
        int f = wp->id;
        unsigned int n;

        if (f < 0) {
            WORDLIST *list = c->tree_words;
            int l;

            for (l = -f - 1; l != -1; ) {
                f = list[l].word;
                l = list[l].next;
                n = bow[f + 1] - bow[f];
                npron += n;
                if (maxpron < n)
                    maxpron = n;
            }
            npron += 2;
        } else {
            n = bow[f + 1] - bow[f];
            if (maxpron < n)
                maxpron = n;
            npron += n + 2;
        }
    }
    nodes = heap_Alloc(c->heap, (npron + 1) * sizeof(SPNODE));
    links = heap_Alloc(c->heap, npron * maxpron * sizeof(SPLINK));
    if (nodes == 0)
        goto fail2;
    if (links == 0)
        goto fail;
    nodes[0].link = -1;
    path = heap_Alloc(c->heap, (c->f1088 + 3) * 4);
    if (path == 0)
        goto fail;
    path[0] = start;
    wp = words;
    for (k = 0; k < nwords; k++, wp++) {
        int x = wp->id;
        unsigned int n;
        unsigned int m;

        prev = 0;
        first = 0;
        id = 0;
        n = 1;
        if (x < 0) {
            WORDLIST *list = c->tree_words;
            int l;

            for (l = -x - 1; l != -1; l = list[l].next)
                n++;
            j = 0;
            for (l = -x - 1; l != -1; l = list[l].next) {
                j++;
                path[n - j] = list[l].word;
            }
        } else {
            path[n++] = x;
        }
        path[n++] = end;
        for (m = 0; m < n; ) {
            id = path[m];
            lnk = nodes[prev].link;
            first = bow[id] - tab0;
            for (;;) {
                unsigned int v;

                if (lnk == 0xFFFF) {
                    if (prev != 0)
                        prevnp = bow[nodes[prev].id + 1] - bow[nodes[prev].id];
                    else
                        prevnp = 1;
                    break;
                }
                v = links[lnk].node;
                if (nodes[v].pron == first) {
                    m++;
                    prev = v;
                    break;
                }
                lnk = links[lnk].next;
            }
            if (lnk == 0xFFFF)
                break;
        }
        lnk = nodes[prev].link;
        if (m < n) {
            for (;;) {
                np = bow[id + 1] - bow[id];
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
                m++;
                nnodes += np;
                prevnp = np;
                if (m == n)
                    break;
                lnk = 0xFFFF;
                id = path[m];
                first = bow[id] - tab0;
            }
        }
        for (i = 0; i < prevnp; i++)
            nodes[prev + i].link = -1;
    }
    heap_Free(c->heap, path);
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

            if (GetSecondPassInfo(c, &ctx, &info, nodes[i].pron) != 0)
                goto fail;
            nstates += info.nstates;
            last = info.last;
            for (; lnk != 0xFFFF; lnk = links[lnk].next) {
                unsigned short x;
                unsigned int e, a;
                int n;

                if (GetSecondPassInfo(c, &ctx, &info, nodes[links[lnk].node].pron) != 0)
                    goto fail;
                x = last * nclasses + info.first;
                e = beg[x + 1];
                a = beg[x];
                n = beg[e] - beg[a];
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
    size2 = nitems * 2;
    size4 = nitems * 4;
    sp->bounds = heap_Alloc(c->heap, (nitems + nsyn) * 4 + 4);
    b = sp->bounds;
    syn = (int *)((char *)b + size4 + 4);
    q = heap_Alloc(c->heap, size2);
    sp->props = q;
    p = q;
    memset(p, 0, size2);
    sp->wtp = heap_Alloc(c->heap, size2);
    wtp = sp->wtp;
    sp->tab = heap_Alloc(c->heap, (nitems + nxstates) * 4 + 8);
    t = sp->tab;
    nstates += nxstates * 2;
    off = (int *)((char *)t + size4 + 4);
    sp->dict = heap_Alloc(c->heap, nstates * 2);
    tbase = t;
    o = sp->dict;
    c->map = heap_Alloc(c->heap, nitems * 8);
    map = c->map;
    total = 0;
    if (CreateMenuMasks(c, sp, nitems) != 0)
        goto fail;
    for (lnk = nodes[0].link; lnk != 0xFFFF; lnk = links[lnk].next)
        q[links[lnk].node - 1] = 1;
    for (i = 1; i <= nn; i++) {
        unsigned int prop;
        unsigned int kk;

        kk = i - 1;
        prop = props[nodes[i].id];
        lnk = nodes[i].link;
        map->id = nodes[i].id;
        map->word = nodes[i].pron;
        map++;
        *b = syn - sp->bounds;
        b++;
        *off = total;
        *t = off - tbase;
        t++;
        off++;
        putWordInDict(c, &ctx, nodes[i].pron, &total, &o, kk);
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
    *t = off - tbase;
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
                    *t = off - tbase;
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
