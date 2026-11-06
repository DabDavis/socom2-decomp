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
 * of the object is not (FreeSecondPassContext_ForChinese stands as a matched draft). The trees'
 * records are named by use.
 *
 * Tone_Of_Final is inlined into Calculate_NbrOfItemNodes_In_FinalSwappedTranslatedTree (its
 * `unsigned char` result zero-extended once at the join, `andi 0xff`, as SOCOM 1's is) while its
 * own copy stays at its place: a GNU `extern __inline__` body above, the real function below
 * (tos.c's pattern). Nothing calls the copy in either build. */

#include "casr.h"

extern void *heap_Alloc(void *heap, unsigned int size);
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
    char pad0[12];
    DICTITEM *items;                    /* +12 */
    DICTLINK *links;                    /* +16 */
    DICTITEM *newitems;                 /* +20 */
    DICTLINK *newlinks;                 /* +24 */
    char pad28[12];
    unsigned short *table;              /* +40: the swapped final of (a, tone, pron), na x a + nb x tone + pron */
    unsigned short na;                  /* +44 */
    unsigned short nb;                  /* +46 */
    unsigned short ntones;              /* +48 */
} FSTREE;

extern __inline__ unsigned char Tone_Of_Final(unsigned short final, unsigned short n)
{
    if (final != 0 && final < n)
        return 1;
    return 0;
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
unsigned char Tone_Of_Final(unsigned short final, unsigned short n)
{
    if (final != 0 && final < n)
        return 1;
    return 0;
}

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
static void Calculate_NbrOfStatesInDict_ForChinese(unsigned int node, int *count, DICTTREE *t)
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
 * node for each child after the first (SOCOM 1's LOCAL).
 * OPEN (19 words, the right size): the first child is a local (the target reuses its index and
 * link address after the tone test; re-reading the field is 0x140), but its *8 shares the
 * recursion loop's pseudo here (s0, the target's t0) and the temporaries sit one register off.
 * A variable of its own for the first child is 18 (inside the test) or the wrong size (the
 * recursion loop outside it, or one `&&` test); one for the counting loop 32-45. */
static void Calculate_NbrOfItemNodes_In_FinalSwappedTranslatedTree(unsigned int node, FSTREE *t,
                                                                   int *count)
{
    unsigned int c = t->items[node].child;

    if (c != 0xFFFF) {
        if (t->links[c].next != 0xFFFF && Tone_Of_Final(t->items[node].tone, t->ntones)) {
            for (c = t->links[c].next; c != 0xFFFF; c = t->links[c].next)
                (*count)++;
        }
        for (c = t->items[node].child; c != 0xFFFF; c = t->links[c].next)
            Calculate_NbrOfItemNodes_In_FinalSwappedTranslatedTree(t->links[c].node, t, count);
    }
}
