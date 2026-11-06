/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing -funroll-loops */
/* chinese: the recogniser's second pass for Mandarin -- the context built from the first pass's
 * words with their tones (OVL2 0x4CE5F0-0x4D0BD8, SOCOM 1's object at 0x3404E8: Build_Tree_ForChinese,
 * Translate_Prons_Into_Items_And_Tones, SplitUp_Finals, Swap_Finals,
 * FillOut_Syntax_Statedict_And_Wordprops_ForChinese, FreeSecondPassContext_ForChinese,
 * CreateSecondPassContext_ForChinese, Tone_Of_Final, Calculate_NbrOfStatesInDict_ForChinese,
 * Calculate_NbrOfItemNodes_In_FinalSwappedTranslatedTree; the five before the free and the last
 * two LOCAL there), ScanSoft's speech recogniser, prebuilt with ee-gcc 2.96 at -O2 -G0
 * -fno-strict-aliasing -funroll-loops (README "The GCC front"). The file's name is ours. Written:
 * SplitUp_Finals (round 32c, OPEN), Swap_Finals (round 39c, OPEN), Tone_Of_Final and the two
 * counting walks after it, FillOut_Syntax_Statedict_And_Wordprops_ForChinese (round 40c); the rest
 * of the object is not: Build_Tree_ForChinese and Translate_Prons_Into_Items_And_Tones. Round 41c:
 * FreeSecondPassContext_ForChinese, CreateSecondPassContext_ForChinese. The trees' records are named by use.
 *
 * Tone_Of_Final and the two counting walks are inline functions defined above the builder: both inline into it (the recursion's
 * inner call stays a call) and GCC writes their deferred copies at the end of the object, after the builder -- the image's
 * order, which no other shape gives (their rows closed with it: round 41c). */

#include "casr.h"

extern void *heap_Alloc(void *heap, unsigned int size);
extern void heap_Free(void *heap, void *p);
extern void *memset(void *dst, int c, unsigned int n);

/* the second pass's context (ctxinfo.c's CTXINFO) */
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

/* the dictionary tree the state count walks: items of 20 bytes, each with its first link; links
 * {node, next}, 0xFFFF ending a list; each pronunciation's first state in begin[] */
typedef struct DICTITEM {
    unsigned short kind;                /* +0: 0 a single state */
    short pad2;
    int f4;                             /* +4 */
    int f8;                             /* +8 */
    unsigned short pron;                /* +12 */
    unsigned short tone;                /* +14 */
    unsigned int child;                 /* +16: the first link, 0xFFFF none */
} DICTITEM;

typedef struct DICTLINK {
    unsigned int node;
    unsigned int next;
} DICTLINK;

typedef struct DICTTREE {
    char pad0[20];
    DICTITEM *items;                    /* +20 */
    DICTLINK *links;                    /* +24 */
    char pad28[8];
    int *begin;                         /* +36 */
} DICTTREE;

/* the final-swapped translated tree: the same items and links at +12 and +16, the split tree's at
 * +20 and +24, the tones' count at +48 */
typedef struct FSTREE {
    CASR *c;                            /* +0 */
    struct TREEITEM *tree;              /* +4: the word tree (Build_Tree_ForChinese) */
    struct TREELINK *treelinks;         /* +8 */
    DICTITEM *items;                    /* +12 */
    DICTLINK *links;                    /* +16 */
    DICTITEM *newitems;                 /* +20 */
    DICTLINK *newlinks;                 /* +24 */
    unsigned short *lex;                /* +28 */
    int *begin;                         /* +32 */
    int *su;                            /* +36 */
    unsigned short *table;              /* +40: the swapped final of (a, tone, pron), na x a + nb x tone + pron */
    unsigned short na;                  /* +44 */
    unsigned short nb;                  /* +46 */
    unsigned short ntones;              /* +48 */
} FSTREE;

__inline__ unsigned char Tone_Of_Final(unsigned short final, unsigned short n)
{
    if (final != 0 && final < n)
        return 1;
    return 0;
}

static __inline__ unsigned short IsStartWord(unsigned short f)
{
    return f & 1;
}

typedef struct TREEITEM {
    int id;
    int pron;                           /* +4 */
    unsigned short link;                /* +8 */
} TREEITEM;

typedef struct TREEUW {
    unsigned short *lex;
    int *begin;
} TREEUW;

typedef struct NBRWORD {
    int state;
    int score;
    int end;
    int f12;
    struct NBRWORD *next;
} NBRWORD;

typedef struct NBRESULT {
    int f0;
    int score;
    int f8;
    NBRWORD *words;
    struct NBRESULT *next;
} NBRESULT;

typedef struct TREELINK {
    unsigned short node;
    unsigned short next;
} TREELINK;

/* the N best's words as a tree of pronunciations: a word whose state is marked with bit 0 (a boundary of a phrase, f >> 2 its kind) is
 * skipped when it starts the result or repeats its neighbour's kind, and is otherwise one item of id -2; the prefixes already in
 * the tree are shared. SOCOM 1's LOCAL (round 41c). */
static void Build_Tree_ForChinese(CASR *c, TREEITEM *items, TREELINK *links, unsigned short *dict, int *bow,
                                  int tab0, unsigned int *pnlinks, unsigned int *pnnodes)
{
    NBRESULT *r = *(NBRESULT **)c->sp_arg1;
    unsigned int nlinks = 0;
    unsigned int nnodes = 1;

    for (; r != 0; r = r->next) {
        NBRWORD *w = r->words;
        unsigned int prev = 0;
        int first = 0;
        int id = 0;
        unsigned int lnk, prevnp, np, i, j;
        int e0, e;

        if (w != 0) {
            e0 = w->end;
            do {
                unsigned short *d = dict + w->state;
                unsigned int f;

                e = w->end;
                lnk = items[prev].link;
                f = *d;
                if (IsStartWord(f)) {
                    NBRWORD *nx;
                    unsigned int f2;

                    if (e == e0) {
                        w = w->next;
                        goto next;
                    }
                    f2 = f >> 2;
                    if ((d[-1] >> 2) == f2) {
                        w = w->next;
                        goto next;
                    }
                    nx = w->next;
                    if (nx != 0 && f2 == (dict[nx->state] >> 2)) {
                        w = nx;
                        goto next;
                    }
                    id = -2;
                    first = -2;
                } else {
                    d++;
                    e0 = e;
                    id = d[1] + (d[2] << 16);
                    first = bow[id] - tab0;
                }
                for (; lnk != 0xFFFF; lnk = links[lnk].next) {
                    unsigned int n = links[lnk].node;

                    if (items[n].pron == first) {
                        w = w->next;
                        prev = n;
                        break;
                    }
                }
                if (lnk == 0xFFFF) {
                    if (prev != 0 && items[prev].id != -2)
                        prevnp = bow[items[prev].id + 1] - bow[items[prev].id];
                    else
                        prevnp = 1;
                    break;
                }
            next:
                ;
            } while (w != 0);
            lnk = items[prev].link;
            if (w != 0) {
                for (;;) {
                    if (id == -2)
                        np = 1;
                    else
                        np = bow[id + 1] - bow[id];
                    for (i = 0; i < np; i++) {
                        for (j = 0; j < prevnp; j++) {
                            links[nlinks + j].node = j * np + nnodes + i;
                            if (lnk != 0xFFFF)
                                links[nlinks + j].next = lnk + j;
                            else
                                links[nlinks + j].next = -1;
                        }
                        lnk = nlinks;
                        if (i == np - 1) {
                            for (j = 0; j < prevnp; j++)
                                items[prev + j].link = nlinks + j;
                        }
                        nlinks = (unsigned short)(nlinks + prevnp);
                        for (j = 0; j < prevnp; j++) {
                            items[j * np + nnodes + i].pron = first + i;
                            items[j * np + nnodes + i].id = id;
                        }
                    }
                    prev = nnodes;
                    w = w->next;
                    {
                        unsigned int k = np * prevnp;

                        nnodes = (unsigned short)(nnodes + k);
                        prevnp = k;
                    }
                    if (w == 0)
                        break;
                    {
                        unsigned short *d = dict + w->state;
                        unsigned int f = *d;

                        if (IsStartWord(f)) {
                            NBRWORD *nx;
                            unsigned int f2 = f >> 2;

                            if (w->end != e0 && (d[-1] >> 2) != f2) {
                                nx = w->next;
                                if (nx == 0 || f2 != (dict[nx->state] >> 2))
                                    goto boundary;
                            } else {
                                nx = w->next;
                            }
                            w = nx;
                            if (w == 0)
                                break;
                            d = dict + w->state;
                            e0 = w->end;
                            d++;
                            id = d[1] + (d[2] << 16);
                            first = bow[id] - tab0;
                            goto have;
                        boundary:
                            id = -2;
                            first = -2;
                        } else {
                            e0 = w->end;
                            d++;
                            id = d[1] + (d[2] << 16);
                            first = bow[id] - tab0;
                        }
                    }
                have:
                    lnk = 0xFFFF;
                }
            }
            for (i = 0; i < prevnp; i++)
                items[prev + i].link = -1;
        }
    }
    *pnlinks = nlinks;
    *pnnodes = nnodes;
}

/* the word tree's node translated into items, one per phoneme after the first of its pronunciation (a phrase boundary, id -2,
 * is one item of the silence phoneme), each linked to the next; the last carries the node's word and pronunciation and, at a
 * leaf, no link; then its children in turn, linked in order; node 0 is the root item. SOCOM 1's body (round 41c). */
static void Translate_Prons_Into_Items_And_Tones(unsigned int node, unsigned int *nitems, unsigned int *nlinks, FSTREE *t)
{
    unsigned int c;
    unsigned int prev;

    if (node == 0) {
        c = t->tree[0].link;
        t->items[0].kind = 0;
        t->items[0].tone = -1;
        t->items[0].child = 0;
        t->links[0].node = 1;
    } else {
        int leaf = t->tree[node].link == 0xFFFF;

        if (t->tree[node].pron != -2) {
            unsigned short *p = t->lex + t->begin[t->tree[node].pron];
            unsigned short *end = t->lex + t->begin[t->tree[node].pron + 1];
            unsigned short kind = *p++;

            for (; p < end; p++) {
                t->items[*nitems].kind = kind == 1;
                if (p == end - 1) {
                    t->items[*nitems].f4 = t->tree[node].id;
                    t->items[*nitems].f8 = t->tree[node].pron;
                } else {
                    t->items[*nitems].f4 = -4;
                    t->items[*nitems].f8 = -4;
                }
                t->items[*nitems].pron = *p;
                if (t->items[*nitems].kind == 1)
                    t->items[*nitems].tone = ((unsigned short *)t->su)[t->su[*p] + 1];
                else
                    t->items[*nitems].tone = t->ntones;
                if (leaf && p == end - 1) {
                    t->items[(*nitems)++].child = 0xFFFF;
                } else {
                    t->items[*nitems].child = *nlinks;
                    t->links[*nlinks].node = ++(*nitems);
                    t->links[(*nlinks)++].next = 0xFFFF;
                }
            }
        } else {
            t->items[*nitems].kind = 0;
            t->items[*nitems].f4 = t->tree[node].id;
            t->items[*nitems].f8 = t->tree[node].pron;
            t->items[*nitems].pron = t->c->lng->GetSilPhenome(t->c->lng);
            t->items[*nitems].tone = 0;
            if (leaf) {
                t->items[(*nitems)++].child = 0xFFFF;
            } else {
                t->items[*nitems].child = *nlinks;
                t->links[*nlinks].node = ++(*nitems);
                t->links[(*nlinks)++].next = 0xFFFF;
            }
        }
        if (!leaf)
            (*nlinks)--;
        c = t->tree[node].link;
    }
    while (c != 0xFFFF) {
        node = t->treelinks[c].node;
        prev = (*nlinks)++;
        Translate_Prons_Into_Items_And_Tones(node, nitems, nlinks, t);
        c = t->treelinks[c].next;
        if (c == 0xFFFF) {
            t->links[prev].next = c;
        } else {
            t->links[prev].next = *nlinks;
            t->links[*nlinks].node = *nitems;
        }
    }
}

/* item n copied into the split tree's slot i, its child link apart (the pointers the block's own) */
#define COPYITEM(t, i, n)                                               \
    {                                                                   \
        DICTITEM *d = &(t)->newitems[i];                                \
        DICTITEM *s = &(t)->items[n];                                   \
        d->kind = s->kind;                                              \
        d->f4 = s->f4;                                                  \
        d->f8 = s->f8;                                                  \
        d->pron = s->pron;                                              \
        d->tone = s->tone;                                              \
    }

/* the subtree at node copied into the split tree (nitems and nlinks its counts): a chain of single
 * children copied as it stands; a leaf ends it; an item with a valid tone and several children
 * copied once per child, each copy leading to its child alone, the copies' links chained (the
 * first copy's link returned when no chain led here, for the caller's list); any other item
 * copied once with every child under it, a child's returned link continuing the list. SOCOM 1's
 * LOCAL, written from SOCOM II's code (round 32c); SplitUp_Finals_TreeCtx (0x4D6710) is the same
 * code word for word. Each copy's two pointers are the macro's own block locals (one pair shared by
 * the four copies outlives everything and takes t0/t1: 152 words); each count's bump a
 * post-increment inside its store (one load of the count); the chained links rebuilt as last + 1.
 * OPEN (round 32c, the right size, 126 words): PRE reuses the walk's node * 4 (a0) for the tone
 * test, where the target recomputes it there (`sll v0,a1,2` in the post-loop test's slot) while
 * reusing it on the leaf, split and plain paths alike; one word shorter there, the split loop's
 * label then needs GNU as's alignment nop (`.p2align 3`), and the registers follow (the child t1
 * and the items t0 here, t0 and t1 there; each copy's source and destination a0/a1 swapped). The
 * first loop as while/for/for(;;)/do-while, the tone through a temporary, a local, an inline taking
 * the node or the item, a walked copy of the node: none recomputes it (build/scratch/gcc/r32c). */
static unsigned int SplitUp_Finals(FSTREE *t, unsigned int node, unsigned int *nitems,
                                   unsigned int *nlinks)
{
    unsigned int c, last, r;
    int first = 1;

    c = t->items[node].child;
    while (c != 0xFFFF && t->links[c].next == 0xFFFF) {
        if (first)
            first = 0;
        COPYITEM(t, *nitems, node);
        t->newitems[(*nitems)++].child = *nlinks;
        t->newlinks[*nlinks].node = *nitems;
        t->newlinks[(*nlinks)++].next = 0xFFFF;
        node = t->links[c].node;
        c = t->items[node].child;
    }
    if (c == 0xFFFF) {
        COPYITEM(t, *nitems, node);
        t->newitems[(*nitems)++].child = 0xFFFF;
    } else if (Tone_Of_Final(t->items[node].tone, t->ntones)) {
        last = *nlinks - 1;
        while (c != 0xFFFF) {
            COPYITEM(t, *nitems, node);
            t->newitems[(*nitems)++].child = *nlinks;
            t->newlinks[*nlinks].node = *nitems;
            t->newlinks[(*nlinks)++].next = 0xFFFF;
            SplitUp_Finals(t, t->links[c].node, nitems, nlinks);
            c = t->links[c].next;
            if (c == 0xFFFF) {
                t->newlinks[last].next = c;
                return first ? last : 0;
            }
            t->newlinks[last].next = *nlinks;
            last = *nlinks;
            t->newlinks[last].node = *nitems;
            *nlinks = last + 1;
        }
    } else {
        COPYITEM(t, *nitems, node);
        t->newitems[(*nitems)++].child = *nlinks;
        last = *nlinks;
        t->newlinks[last].node = *nitems;
        *nlinks = last + 1;
        while (c != 0xFFFF) {
            r = SplitUp_Finals(t, t->links[c].node, nitems, nlinks);
            if (r != 0)
                last = r;
            c = t->links[c].next;
            if (c == 0xFFFF)
                t->newlinks[last].next = c;
            else {
                t->newlinks[last].next = *nlinks;
                last = *nlinks;
                t->newlinks[last].node = *nitems;
                *nlinks = last + 1;
            }
        }
    }
    return 0;
}

/* a final's tone is valid: nonzero and below the count (SOCOM II's body; SOCOM 1's returned the
 * compare itself after the zero test) */

/* the split tree's finals swapped for their tone-carrying forms, down the tree: on a chain of single children
 * the pending final is carried along, an item with a valid tone settling the one before it. SOCOM 1's body.
 * OPEN (round 39c, written: 275 words of 280, the layout the target's). Measured: a `while` rather than `for (;;)`
 * (the bottom test on the register of 0xFFFF), the pron read before prev moves in both arms of the pending-final
 * test (the target's branch; one `if` gives xor/movz), the leaf's `if (final == 0xFFFF) final = 0` plain (the
 * target's xori/movz there). Open: the target keeps the head's 0xFFFF in a register of its own (t3, issued early),
 * which moves every t-register up one here; five words of the two table stores' arms. */
int Swap_Finals(unsigned short final, unsigned int prev, unsigned int node, FSTREE *t)
{
    unsigned int c = t->newitems[node].child;
    unsigned short pron = t->newitems[prev].pron;
    unsigned short i;

    while (c != 0xFFFF) {
        {
            if (Tone_Of_Final(t->newitems[node].tone, t->ntones)) {
                if (prev != 0) {
                    t->newitems[prev].pron = t->table[t->na * final + t->nb * t->newitems[node].tone + pron];
                    final = t->newitems[prev].tone;
                    pron = t->newitems[node].pron;
                    prev = node;
                } else {
                    if (final == 0xFFFF) {
                        final = 0;
                        pron = t->newitems[node].pron;
                        prev = node;
                    } else {
                        pron = t->newitems[node].pron;
                        prev = node;
                    }
                }
            } else if (t->newitems[node].kind == 0) {
                if (t->newitems[node].tone == 0) {
                    if (prev != 0)
                        t->newitems[prev].pron = t->table[t->na * final + pron];
                    prev = 0;
                    final = 0;
                } else if (node != 0) {
                    if (prev != 0)
                        t->newitems[prev].pron = t->table[t->na * final + t->nb * t->ntones + pron];
                    final = t->ntones;
                    prev = 0;
                }
            }
            if (t->newlinks[c].next != 0xFFFF)
                break;
            node = t->newlinks[t->newitems[node].child].node;
            c = t->newitems[node].child;
        }
    }
    if (t->newitems[node].child == 0xFFFF) {
        if (Tone_Of_Final(t->newitems[node].tone, t->ntones)) {
            if (prev != 0) {
                t->newitems[prev].pron = t->table[t->na * final + t->nb * t->newitems[node].tone + pron];
                final = t->newitems[prev].tone;
                pron = t->newitems[node].pron;
            } else {
                if (final == 0xFFFF)
                    final = 0;
                pron = t->newitems[node].pron;
            }
            t->newitems[node].pron = t->table[t->na * final + pron];
        } else if (prev != 0) {
            unsigned int tone;

            if (t->newitems[node].kind == 0 && t->newitems[node].tone != 0)
                tone = t->ntones;
            else
                tone = 0;
            t->newitems[prev].pron = t->table[t->na * final + t->nb * tone + pron];
        }
    }
    i = 0;
    if (c != 0xFFFF) {
        do {
            if (i != 0 && prev != 0) {
                final = t->newitems[prev].tone;
                prev = 0;
            }
            i++;
            if (Swap_Finals(final, prev, t->newlinks[c].node, t) != 0)
                return 1;
            c = t->newlinks[c].next;
        } while (c != 0xFFFF);
    }
    return 0;
}

/* one word's states out of its item: a single state or the pronunciation's own, the last one
 * marked, then the word's index in two halves */
static __inline__ void FillOut_ItemStates(DICTITEM *it, int *su, unsigned int k, int *ptotal, unsigned short **po)
{
    unsigned short pron = it->pron;
    unsigned short *q = *po;
    unsigned short cnt = 0;
    int nw;

    if (it->kind == 0) {
        *q++ = pron << 2;
        cnt = 1;
    } else {
        unsigned short *a = (unsigned short *)su + su[pron] + 2;
        unsigned short *e = (unsigned short *)su + su[pron + 1];

        for (; a < e; a++) {
            *q++ = *a << 2;
            cnt++;
        }
    }
    *(q - 1) |= 1;
    q[0] = k;
    q[1] = k >> 16;
    q += 2;
    nw = cnt + 2;
    *ptotal += nw;
    *po = q;
}

/* the second-pass context from the swapped tree: the mask, the words' properties and penalties, the
 * map to the first pass's words, the state dictionary and the two offset tables, one word per item
 * but the root; 1 when an allocation fails. SOCOM 1's body (round 40c). Measured:
 * - total and o live in the frame because their addresses reach an inline (the target's sp+0 and
 *   sp+4 are read directly in the first block and through a hoisted address past the join); a local
 *   takes its slot when its address is parsed, so &total goes first.
 * - items[i] everywhere, not a walker: SOCOM 1 recomputes it after the states, and the extra real
 *   insns loop.c counts (116 with a walker, 122 here) are what leave -4 and -2 "not desirable" while
 *   1 is hoisted (the threshold falls by 3 per register moved: 122 moves one, 124 none, 118 two).
 * - k a local of the loop: passed as `i - 1` it is one insn more (123), and 1 stays in the loop.
 * - `ctx->wtp = heap_Alloc(...); w = ctx->wtp;` stores v0 and copies: a local set first stores its
 *   own register, one ref more; props is that form, the others this one.
 * - `*w++` in both arms, cross-jumped to the one bump: 14 refs put w ahead of syn and t (s3); one
 *   bump after the join is 10 refs (s8). `*p++` in the test puts p's bump in the branch's slot. */
static int FillOut_Syntax_Statedict_And_Wordprops_ForChinese(CASR *c, DICTITEM *items, DICTLINK *links,
                                                             unsigned int nstates, unsigned int n, int nl,
                                                             SPCTX *ctx)
{
    int total;
    unsigned short *o;
    int *tbase;
    int *su;
    int size;
    unsigned short *props;
    unsigned short *p, *w;
    WORDMAP *map;
    int *b, *t;
    int *syn, *off;
    unsigned int ch;
    unsigned int i;
    int msize;
    int fsize;

    total = 0;
    if (CreateMenuMasks(c, ctx, n) != 0)
        return 1;
    size = n * 2;
    msize = n * 8;
    props = heap_Alloc(c->heap, size);
    ctx->props = props;
    p = props;
    fsize = n * 4;
    ctx->wtp = heap_Alloc(c->heap, size);
    w = ctx->wtp;
    c->map = heap_Alloc(c->heap, msize);
    map = c->map;
    ctx->bounds = heap_Alloc(c->heap, (n + nl) * 4 + 4);
    b = ctx->bounds;
    syn = (int *)((char *)b + fsize + 4);
    ctx->tab = heap_Alloc(c->heap, msize + 8);
    t = ctx->tab;
    tbase = t;
    off = (int *)((char *)t + fsize + 4);
    su = c->lng->GetpSpeechUnit(c->lng);
    ctx->dict = heap_Alloc(c->heap, nstates * 2);
    o = ctx->dict;
    if (p == 0 || w == 0 || map == 0 || b == 0 || t == 0 || o == 0)
        return 1;
    memset(p, 0, size);
    for (ch = items[0].child; ch != 0xFFFF; ch = links[ch].next)
        props[links[ch].node - 1] = 1;
    for (i = 1; i <= n; i++) {
        unsigned int k;

        map->id = items[i].f4;
        map->word = items[i].f8;
        map++;
        *b = syn - ctx->bounds;
        b++;
        *off = total;
        *t = off - tbase;
        off++;
        t++;
        k = i - 1;
        FillOut_ItemStates(&items[i], su, k, &total, &o);
        ch = items[i].child;
        if (ch != 0) {
            if (ch == 0xFFFF)
                *p |= 2;
            else if (*p != 1 && items[i].f8 != -4 && items[i].f8 != -2)
                *p |= 0x80;
            if (*p++ & 0x80)
                *w++ = c->wtp;
            else
                *w++ = 0;
            for (; ch != 0xFFFF; ch = links[ch].next)
                *syn++ = links[ch].node - 1;
        }
    }
    *b = syn - ctx->bounds;
    *t = off - tbase;
    return 0;
}

/* the states a subtree's words take: a single-state item one, a pronunciation its states less
 * the two it shares (SOCOM 1's LOCAL; the order of the sum compiles the same three ways) */
static __inline__ void Calculate_NbrOfStatesInDict_ForChinese(unsigned int node, int *count, DICTTREE *t)
{
    unsigned int c;

    if (node != 0) {
        if (t->items[node].kind == 0)
            (*count)++;
        else
            *count += t->begin[t->items[node].pron + 1] - t->begin[t->items[node].pron] - 2;
    }
    for (c = t->items[node].child; c != 0xFFFF; c = t->links[c].next)
        Calculate_NbrOfStatesInDict_ForChinese(t->links[c].node, count, t);
}

/* the item nodes the final swap adds: an item with a valid tone and more than one child gets a
 * node for each child after the first (SOCOM 1's LOCAL). */
static __inline__ void Calculate_NbrOfItemNodes_In_FinalSwappedTranslatedTree(unsigned int node, FSTREE *t,
                                                                              int *count)
{
    unsigned int c = t->items[node].child;

    if (c != 0xFFFF) {
        if (t->links[c].next != 0xFFFF && Tone_Of_Final(t->items[node].tone, t->ntones)) {
            for (c = t->links[c].next; c != 0xFFFF; c = t->links[c].next)
                (*count)++;
            c = t->items[node].child;
        }
        for (; c != 0xFFFF; c = t->links[c].next)
            Calculate_NbrOfItemNodes_In_FinalSwappedTranslatedTree(t->links[c].node, t, count);
    }
}

/* The free stays above the builder: the call to a function defined earlier in the unit has the nothrow note, so
 * reorg can read through it for the registers its target needs (the first failure's `beqz s8; move a0,s8`). */
void FreeSecondPassContext_ForChinese(CASR *c)
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

/* the context for Mandarin: the word tree, its pronunciations translated into items and tones, the finals split up
 * and swapped; SOCOM 1's body (round 41c). What the code said: one variable holds the table's sum and then the total (`move
 * s0,zero` stays after the last call by its anti-dependence on `sw s0,40(s3)`; a value of its own floats up across the calls and
 * collides with the table's s0); its product is assigned back (`mult s0,s0,v0`); the running maximum is a register twin of a
 * frame word, read once before the loops, stored once after them, its address a pointer of its own per maximum (else it is
 * put back in a register; one pointer for both keeps it in a register across cprop); `n3` is read for the first allocation and
 * `n4` for the later ones: without the second copy PRE commons the two sums' shift across the calls (a spilled temporary); the second
 * early failure has its own tail (its refs lift c above t: s2 against s3); `sizeof * (maxn * n3) * maxn` is the target's operand
 * order, and the first count's locals are set in the order the second scheduling pass needs (issue rate two, loads by sched1's order). */
int CreateSecondPassContext_ForChinese(CASR *c)
{
    unsigned int maxp;
    unsigned int nlinks;
    unsigned int nnodes;
    unsigned int maxn;
    unsigned int nitems;
    unsigned int nlinks2;
    int nitems2;
    unsigned int nnew;
    unsigned int nnewlinks;
    int nstates;
    int nroots;
    unsigned int n3;
    SPCTX *rc;
    LANGVT *lng;
    int *bow;
    unsigned short *dict;
    int tab0;
    unsigned short *dims;
    TREEUW *uw;
    SPCTX *sp;
    FSTREE *t;
    unsigned int total;
    NBRESULT *r;
    unsigned int n;
    unsigned int m;
    unsigned int *pm1, *pm2;
    unsigned int n2, n4;
    TREEITEM *items;
    TREELINK *links;
    DICTITEM *ditems;
    DICTLINK *dlinks;
    unsigned int l;

    nroots = 0;
    maxp = 0;
    nlinks = 0;
    nnodes = 1;
    nstates = 0;
    nitems = 1;
    maxn = 0;
    nlinks2 = 0;
    nitems2 = 0;
    nnew = 0;
    nnewlinks = 0;
    rc = c->run_ctx;
    lng = c->lng;
    bow = rc->tab;
    dict = rc->dict;
    tab0 = *bow;
    dims = lng->GetpDimensionsOfToneConversionMatrix(lng);
    uw = c->uw_tables;
    if (c->sp_ctx != 0)
        FreeSecondPassContext_ForChinese(c);
    sp = heap_Alloc(c->heap, sizeof(SPCTX));
    if (sp == 0)
        goto fail;
    memset(sp, 0, sizeof(SPCTX));
    c->sp_ctx = sp;
    t = heap_Alloc(c->heap, sizeof(FSTREE));
    if (t == 0) {
        FreeSecondPassContext_ForChinese(c);
        return 1;
    }
    t->c = c;
    t->lex = uw->lex;
    t->begin = uw->begin;
    t->su = lng->GetpSpeechUnit(lng);
    total = (unsigned int)lng->GetpToneConversionMatrix(lng);
    total -= *lng->GetpOffsetForFinals(lng) * 2;
    t->table = (unsigned short *)total;
    t->na = dims[2];
    t->nb = dims[2] * dims[1];
    t->ntones = lng->GetNbrTones(lng) + 1;
    total = 0;
    n = 0;
    pm1 = &maxp;
    m = *pm1;
    for (r = *(NBRESULT **)c->sp_arg1; r != 0; r = r->next) {
        NBRWORD *w;
        unsigned int mul = 1;

        for (w = r->words; w != 0; w = w->next) {
            unsigned short *d = dict + w->state;

            if (!IsStartWord(*d)) {
                unsigned int id;

                d++;
                id = d[1] + (d[2] << 16);
                n = bow[id + 1] - bow[id];
                if (m < n)
                    m = n;
                total += n * mul;
            } else {
                if (m == 0)
                    m = 1;
                total += mul;
            }
            mul *= n;
        }
    }
    *pm1 = m;
    items = heap_Alloc(c->heap, (total + 1) * sizeof(TREEITEM));
    t->tree = items;
    total *= maxp;
    links = heap_Alloc(c->heap, total * 4);
    t->treelinks = links;
    if (items == 0 || links == 0) {
        if (items != 0)
            heap_Free(c->heap, items);
        if (links != 0)
            heap_Free(c->heap, links);
        goto freet;
    }
    items[0].link = -1;
    Build_Tree_ForChinese(c, items, links, dict, bow, tab0, &nlinks, &nnodes);
    for (l = items[0].link; l != 0xFFFF; l = links[l].next)
        nroots++;
    {
        unsigned int nw;
        unsigned short *lex;
        int *begin;
        unsigned int i;
        unsigned int k;
        unsigned int e;
        unsigned int m2;

        k = items[0].link;
        n2 = 0;
        nw = nnodes - 1;
        e = 0;
        lex = uw->lex;
        begin = uw->begin;
        while (k != 0xFFFF) {
            k = links[k].next;
            e++;
        }
        pm2 = &maxn;
        m2 = e;
        for (i = 1; i <= nw; i++) {
            int pron = items[i].pron;

            if (pron == -2) {
                n2++;
            } else {
                unsigned short *p = lex + begin[pron];
                unsigned short *e = lex + begin[pron + 1];

                p++;
                n2 += e - p;
            }
            e = 0;
            k = items[i].link;
            while (k != 0xFFFF) {
                k = links[k].next;
                e++;
            }
            if (m2 < e)
                m2 = e;
        }
        *pm2 = m2;
    }
    n3 = n2;
    ditems = heap_Alloc(c->heap, (n3 + 1) * sizeof(DICTITEM));
    t->items = ditems;
    dlinks = heap_Alloc(c->heap, n2 * maxn * sizeof(DICTLINK));
    t->links = dlinks;
    if (ditems == 0 || dlinks == 0) {
        heap_Free(c->heap, items);
        heap_Free(c->heap, links);
        if (ditems != 0)
            heap_Free(c->heap, ditems);
        if (dlinks != 0)
            heap_Free(c->heap, dlinks);
        goto freet;
    }
    Translate_Prons_Into_Items_And_Tones(0, &nitems, &nlinks2, t);
    heap_Free(c->heap, items);
    heap_Free(c->heap, links);
    n4 = n2;
    nitems2 = n4;
    Calculate_NbrOfItemNodes_In_FinalSwappedTranslatedTree(0, t, &nitems2);
    {
        DICTITEM *ni = heap_Alloc(c->heap, maxn * ((n4 + 1) * sizeof(DICTITEM)));
        DICTLINK *nl;

        t->newitems = ni;
        nl = heap_Alloc(c->heap, sizeof(DICTLINK) * (maxn * n3) * maxn);
        t->newlinks = nl;
        if (ni == 0 || nl == 0) {
            heap_Free(c->heap, ditems);
            heap_Free(c->heap, dlinks);
            if (ni != 0)
                heap_Free(c->heap, ni);
            if (nl != 0)
                heap_Free(c->heap, nl);
            goto freet;
        }
        SplitUp_Finals(t, 0, &nnew, &nnewlinks);
        heap_Free(c->heap, ditems);
        heap_Free(c->heap, dlinks);
        if (Swap_Finals(0xFFFF, 0, 0, t) != 0)
            goto fail2;
        Calculate_NbrOfStatesInDict_ForChinese(0, &nstates, (DICTTREE *)t);
        nstates += nitems2 * 2;
        if (FillOut_Syntax_Statedict_And_Wordprops_ForChinese(c, ni, nl, nstates, nitems2,
                                                              nlinks2 - nroots + (nitems2 - n4) + 1, sp) != 0) {
fail2:
            heap_Free(c->heap, ni);
            heap_Free(c->heap, nl);
freet:
            heap_Free(c->heap, t);
fail:
            FreeSecondPassContext_ForChinese(c);
            return 1;
        }
        heap_Free(c->heap, ni);
        heap_Free(c->heap, nl);
        heap_Free(c->heap, t);
        sp->n = nitems2;
        sp->f0 = 0;
        sp->nstates = nstates;
        return 0;
    }
}
