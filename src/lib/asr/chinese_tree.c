/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing -funroll-loops */
/* chinese_tree: the Mandarin second pass over a tree context, chinese.c's twin (OVL2
 * 0x4D5B18-0x4D8398, SOCOM 1's object at 0x345CD0: Build_Tree_ForChinese_TreeCtx,
 * Translate_Prons_Into_Items_And_Tones_TreeCtx, SplitUp_Finals_TreeCtx, Swap_Finals_TreeCtx,
 * FillOut_Syntax_Statedict_And_Wordprops_ForChinese_TreeCtx, CreateSecondPassTreeContext_ForChinese,
 * Tone_Of_Final_TreeCtx, Calculate_NbrOfStatesInDict_ForChinese_TreeCtx,
 * Calculate_NbrOfItemNodes_In_FinalSwappedTranslatedTree_TreeCtx,
 * FreeSecondPassContext_ForChinese_TreeCtx), ScanSoft's speech recogniser, prebuilt with ee-gcc
 * 2.96 at -O2 -G0 -fno-strict-aliasing -funroll-loops (README "The GCC front"). The file's name is
 * ours. Written: SplitUp_Finals_TreeCtx (round 36c, OPEN as chinese.c's), Tone_Of_Final_TreeCtx
 * and the two counting walks after it, chinese.c's three
 * under this object's names (the same bodies; Tone_Of_Final_TreeCtx inlined into the second walk
 * as there); FillOut_Syntax_Statedict_And_Wordprops_ForChinese_TreeCtx (round 40c);
 * FreeSecondPassContext_ForChinese_TreeCtx, CreateSecondPassTreeContext_ForChinese (round 41c), with the walks as chinese.c's
 * deferred inlines. */

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
    struct TREEITEM *tree;              /* +4 */
    unsigned int *treelinks;            /* +8 */
    DICTITEM *items;                    /* +12 */
    DICTLINK *links;                    /* +16 */
    DICTITEM *newitems;                 /* +20 */
    DICTLINK *newlinks;                 /* +24 */
    unsigned short *lex;                /* +28 */
    int *begin;                         /* +32 */
    int *su;                            /* +36 */
    unsigned short *table;              /* +40 */
    unsigned short na;                  /* +44 */
    unsigned short nb;                  /* +46 */
    unsigned short ntones;              /* +48 */
} FSTREE;

__inline__ unsigned char Tone_Of_Final_TreeCtx(unsigned short final, unsigned short n)
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

typedef struct TREEITEM {
    int id;
    int pron;                           /* +4 */
    unsigned short link;                /* +8 */
} TREEITEM;

typedef struct TREEWORD {
    int id;
    int f4;
} TREEWORD;

typedef struct TREELINK {
    unsigned short node;
    unsigned short next;
} TREELINK;

/* the menu's sentences as a tree of pronunciations: start, then the sentence's word, then end, each
 * with its prefix shared when already in the tree; a pronunciation count multiplies down a chain.
 * SOCOM 1's LOCAL (round 41c). */
static void Build_Tree_ForChinese_TreeCtx(TREEITEM *items, TREELINK *links, int start, int end, int *bow,
                                          unsigned int nwords, TREEWORD *words, unsigned int *pnlinks,
                                          unsigned int *pnnodes)
{
    int bow0 = bow[0];
    unsigned int nlinks = *pnlinks;
    unsigned int nnodes = *pnnodes;
    unsigned int wi;
    TREEWORD *wp = words;
    unsigned int prevnp;

    for (wi = 0; wi < nwords; wi++, wp++) {
        unsigned int prev = 0;
        unsigned int lnk = items[0].link;
        int w = wp->id;
        int first = bow[start] - bow0;
        unsigned int np, i, j, k;

        for (;;) {
            unsigned int n;

            if (lnk == 0xFFFF) {
                if (prev != 0)
                    prevnp = bow[items[prev].id + 1] - bow[items[prev].id];
                else
                    prevnp = 1;
                break;
            }
            n = links[lnk].node;
            if (items[n].pron == first) {
                prev = n;
                break;
            }
            lnk = links[lnk].next;
        }
        if (lnk == 0xFFFF) {
            unsigned int ns = bow[start + 1] - bow[start];

            lnk = items[prev].link;
            for (i = 0; i < ns; i++) {
                for (j = 0; j < prevnp; j++) {
                    links[nlinks + j].node = nnodes + i;
                    if (lnk != 0xFFFF)
                        links[nlinks + j].next = lnk + j;
                    else
                        links[nlinks + j].next = -1;
                }
                lnk = nlinks;
                if (i == ns - 1) {
                    for (j = 0; j < prevnp; j++)
                        items[prev + j].link = nlinks + j;
                }
                items[nnodes + i].pron = first + i;
                nlinks = (unsigned short)(nlinks + prevnp);
                items[nnodes + i].id = start;
            }
            prev = nnodes;
            nnodes = (unsigned short)(nnodes + ns);
            prevnp = ns;
            lnk = 0xFFFF;
        } else {
            lnk = items[prev].link;
        }
        first = bow[w] - bow0;
        if (lnk != 0xFFFF) {
            for (;;) {
                unsigned int n;

                if (lnk == 0xFFFF) {
                    if (prev != 0)
                        prevnp = bow[items[prev].id + 1] - bow[items[prev].id];
                    else
                        prevnp = 1;
                    break;
                }
                n = links[lnk].node;
                if (items[n].pron == first) {
                    prev = n;
                    break;
                }
                lnk = links[lnk].next;
            }
            lnk = items[prev].link;
        }
        for (k = 0; k < 2; k++) {
            unsigned int t;

            np = bow[w + 1] - bow[w];
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
                    items[j * np + nnodes + i].id = w;
                }
            }
            prev = nnodes;
            w = end;
            t = np * prevnp;
            nnodes = (unsigned short)(nnodes + t);
            prevnp = t;
            first = bow[w] - bow0;
            lnk = 0xFFFF;
        }
        for (i = 0; i < prevnp; i++)
            items[prev + i].link = -1;
    }
    *pnlinks = nlinks;
    *pnnodes = nnodes;
}

/* the pronunciation tree Build_Tree_ForChinese_TreeCtx makes (12-byte nodes, 4-byte links), the item
 * tree it is translated into (DICTITEM / DICTLINK), and the lexicon's tables, as the translation
 * reads them */
typedef struct PRNODE {
    int f0;                             /* +0 */
    int pron;                           /* +4 */
    unsigned short child;               /* +8: the first link, 0xFFFF none */
    unsigned short pad10;
} PRNODE;

typedef struct PRLINK {
    unsigned short node;                /* +0 */
    unsigned short next;                /* +2: 0xFFFF the last */
} PRLINK;

typedef struct TPCTX {
    int pad0;
    PRNODE *nodes;                      /* +4 */
    PRLINK *links;                      /* +8 */
    DICTITEM *items;                    /* +12 */
    DICTLINK *ilinks;                   /* +16 */
    int pad20[2];
    unsigned short *lex;                /* +28: each pronunciation's phonemes, its kind first */
    int *begin;                         /* +32: where each pronunciation starts */
    int *tones;                         /* +36: an index per phoneme into the table as halfwords */
} TPCTX;

/* the pronunciation tree's node translated into items, one per phoneme after the first word of
 * its pronunciation (each linked to the next; the last carries the node's word and pronunciation
 * and, at a leaf, no link), then its children in turn, linked in order; node 0 is the root item.
 * SOCOM 1's body (its _TreeCtx copy; chinese.c's row, 0x4F8, is a longer form, unwritten).
 * OPEN (round 37c, written: 116 words, two short (0x364) -- 161 with the kind read inside the loop;
 * read once through the walker's first post-increment, the target's lhu-then-addiu): registers --
 * the end, end - 1 and node * 12 sit one t-register below the target's (its node stays in t1 from
 * the entry, node * 2 in s1 for the child's reload after the loop), the leaf flag's sltiu is
 * scheduled before the loop test (the target's sits in that branch's slot), and the leaf arm's
 * `bnel` for the last phoneme is a `bne` here (the two missing words: the target reloads *nitems in
 * that arm). Not tried: `t->items[(*nitems)++]` forms. */
static void Translate_Prons_Into_Items_And_Tones_TreeCtx(unsigned int node, unsigned int *nitems,
                                                         unsigned int *nlinks, TPCTX *t)
{
    unsigned short *first, *p, *end;
    unsigned int c;
    unsigned int prev;
    int leaf;
    int kind;

    if (node == 0) {
        c = t->nodes[0].child;
        t->items[0].kind = 0;
        t->items[0].tone = 0xFFFF;
        t->items[0].child = 0;
        t->ilinks[0].node = 1;
    } else {
        p = t->lex + t->begin[t->nodes[node].pron];
        end = t->lex + t->begin[t->nodes[node].pron + 1];
        leaf = t->nodes[node].child == 0xFFFF;
        kind = *p++ == 1;
        for (; p < end; p++) {
            t->items[*nitems].kind = kind;
            if (p == end - 1) {
                t->items[*nitems].f4 = t->nodes[node].f0;
                t->items[*nitems].f8 = t->nodes[node].pron;
            } else {
                t->items[*nitems].f4 = -4;
                t->items[*nitems].f8 = -4;
            }
            t->items[*nitems].pron = *p;
            t->items[*nitems].tone = ((unsigned short *)t->tones)[t->tones[*p] + 1];
            if (leaf && p == end - 1) {
                t->items[*nitems].child = 0xFFFF;
                (*nitems)++;
            } else {
                t->items[*nitems].child = *nlinks;
                (*nitems)++;
                t->ilinks[*nlinks].node = *nitems;
                t->ilinks[*nlinks].next = 0xFFFF;
                (*nlinks)++;
            }
        }
        if (!leaf)
            (*nlinks)--;
        c = t->nodes[node].child;
    }
    while (c != 0xFFFF) {
        prev = (*nlinks)++;
        Translate_Prons_Into_Items_And_Tones_TreeCtx(t->links[c].node, nitems, nlinks, t);
        c = t->links[c].next;
        if (c == 0xFFFF)
            t->ilinks[prev].next = c;
        else {
            t->ilinks[prev].next = *nlinks;
            t->ilinks[*nlinks].node = *nitems;
        }
    }
}

/* chinese.c's SplitUp_Finals under this object's name, word for word the same code (round 36c:
 * the two rows' 299 instructions agree but for the recursion's target). OPEN as SplitUp_Finals is
 * (126 words, the right size: PRE reuses the walk's node * 4 for the tone test; chinese.c's note) */
static unsigned int SplitUp_Finals_TreeCtx(FSTREE *t, unsigned int node, unsigned int *nitems,
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
    } else if (Tone_Of_Final_TreeCtx(t->items[node].tone, t->ntones)) {
        last = *nlinks - 1;
        while (c != 0xFFFF) {
            COPYITEM(t, *nitems, node);
            t->newitems[(*nitems)++].child = *nlinks;
            t->newlinks[*nlinks].node = *nitems;
            t->newlinks[(*nlinks)++].next = 0xFFFF;
            SplitUp_Finals_TreeCtx(t, t->links[c].node, nitems, nlinks);
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
            r = SplitUp_Finals_TreeCtx(t, t->links[c].node, nitems, nlinks);
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

/* one word's states out of its item: a single state or the pronunciation's own, the last one
 * marked, then the word's index in two halves */
static __inline__ void FillOut_ItemStates_TreeCtx(DICTITEM *it, int *su, unsigned int k, int *ptotal, unsigned short **po)
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

/* chinese.c's body for the tree context: the word test is -4 alone (round 40c) */
static int FillOut_Syntax_Statedict_And_Wordprops_ForChinese_TreeCtx(CASR *c, DICTITEM *items, DICTLINK *links,
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
        FillOut_ItemStates_TreeCtx(&items[i], su, k, &total, &o);
        ch = items[i].child;
        if (ch != 0) {
            if (ch == 0xFFFF)
                *p |= 2;
            else if (*p != 1 && items[i].f8 != -4)
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

/* the states a subtree's words take (chinese.c's) */
static __inline__ void Calculate_NbrOfStatesInDict_ForChinese_TreeCtx(unsigned int node, int *count, DICTTREE *t)
{
    unsigned int c;

    if (node != 0) {
        if (t->items[node].kind == 0)
            (*count)++;
        else
            *count += t->begin[t->items[node].pron + 1] - t->begin[t->items[node].pron] - 2;
    }
    for (c = t->items[node].child; c != 0xFFFF; c = t->links[c].next)
        Calculate_NbrOfStatesInDict_ForChinese_TreeCtx(t->links[c].node, count, t);
}

/* the item nodes the final swap adds (chinese.c's) */
static __inline__ void Calculate_NbrOfItemNodes_In_FinalSwappedTranslatedTree_TreeCtx(unsigned int node, FSTREE *t,
                                                                                      int *count)
{
    unsigned int c = t->items[node].child;

    if (c != 0xFFFF) {
        if (t->links[c].next != 0xFFFF && Tone_Of_Final_TreeCtx(t->items[node].tone, t->ntones)) {
            for (c = t->links[c].next; c != 0xFFFF; c = t->links[c].next)
                (*count)++;
            c = t->items[node].child;
        }
        for (; c != 0xFFFF; c = t->links[c].next)
            Calculate_NbrOfItemNodes_In_FinalSwappedTranslatedTree_TreeCtx(t->links[c].node, t, count);
    }
}

/* the free stays above the builder (chinese.c's note); SOCOM 1 writes it last */
void FreeSecondPassContext_ForChinese_TreeCtx(CASR *c)
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
        heap_Free(c->heap, c->sp_ctx);
        c->sp_ctx = 0;
    }
}

typedef struct TREEUW {
    unsigned short *lex;
    int *begin;
} TREEUW;

typedef struct CTXVT CTXVT;
struct CTXVT {
    void *data;
    void *s1[8];
    unsigned int (*GetNbrWord)(CTXVT *c);           /* +36 */
    void *s10[7];
    int *(*GetpBeginOfWords)(CTXVT *c);             /* +68 */
    unsigned short *(*GetpWordProp)(CTXVT *c);      /* +72 */
    void *s19[2];
};

extern void FillContextV2VirtualTable(CTXVT *vt);

typedef struct IMPORT {
    void *data;
    void *heap;
} IMPORT;

extern int Swap_Finals_TreeCtx(unsigned short final, unsigned int prev, unsigned int node, FSTREE *t);

static __inline__ unsigned short IsStartWord(unsigned short f)
{
    return f & 1;
}

static __inline__ unsigned short IsStopWord(unsigned short f)
{
    return f & 2;
}

/* does any start word's pronunciation hold a phoneme whose tone is valid */
static __inline__ unsigned char StartWordsHaveTones(const unsigned int nw, unsigned short *const props, TREEUW *const uw, FSTREE *const t)
{
    unsigned short ntones = t->ntones;
    unsigned int i;
    int *su = t->su;
    unsigned short *lex = uw->lex;
    int *begin = uw->begin;

    for (i = 0; i < nw; i++) {
        if (IsStartWord(props[i])) {
            unsigned short *p = lex + begin[i];
            unsigned short *e = lex + begin[i + 1];

            for (; p < e; p++) {
                if (Tone_Of_Final_TreeCtx(((unsigned short *)su)[su[*p] + 1], ntones))
                    return 1;
            }
        }
    }
    return 0;
}

/* the Mandarin context over a tree context: chinese.c's CreateSecondPassContext_ForChinese, the words read from the
 * context's result; the finals split only when a start word's pronunciation carries a tone. SOCOM 1's body (round 41c).
 * What the code said beyond chinese.c's: n2 holds the start word first (one pseudo: its refs take s7, the end stays in the frame);
 * the tone scan's parameters are const, so the word count is read where it stands (TREE_READONLY in expand_inline_function: s8, and
 * SOCOM 1's reload of the same spill slot); every failure has its own tail (cross-jumped after reload: c's refs against t's).
 * OPEN (2 words, the right size): `addiu t3,v0,-1` and `move s7,zero` swapped in the second count's head -- the target's first
 * scheduling pass issues the zero after the sum (the sum waits for the load's latency), so another ALU insn holds its second slot at the
 * load's next cycle; every order of the block's statements gives this or worse, and the count as an inline changes the size. */
int CreateSecondPassTreeContext_ForChinese(CASR *c)
{
    CTXVT ctx;
    unsigned int maxp;
    unsigned int nnodes;
    unsigned int nlinks;
    unsigned int maxn;
    unsigned int nitems;
    unsigned int nlinks2;
    int nitems2;
    unsigned int nnew;
    unsigned int nnewlinks;
    int nstates;
    int nroots;
    unsigned int n3;
    SPCTX *sp;
    TREEUW *uw;
    int end;
    unsigned short *props;
    LANGVT *lng;
    unsigned short *dims;
    TREEWORD *words;
    unsigned int nwords;
    FSTREE *t;
    unsigned int total;
    unsigned int nw;
    int *bow;
    unsigned int m;
    unsigned int *pm1, *pm2;
    int n2;
    unsigned int n4;
    TREEITEM *items;
    unsigned int *links;
    DICTITEM *ditems;
    DICTLINK *dlinks;
    DICTITEM *ni;
    DICTLINK *nl;
    unsigned int l;
    unsigned int k;

    nroots = 0;
    n2 = -1;
    maxp = 0;
    nlinks = 0;
    end = -1;
    nnodes = 1;
    nstates = 0;
    nitems = 1;
    maxn = 0;
    nlinks2 = 0;
    nitems2 = 0;
    nnew = 0;
    nnewlinks = 0;
    lng = c->lng;
    dims = lng->GetpDimensionsOfToneConversionMatrix(lng);
    uw = c->uw_tables;
    words = *(TREEWORD **)c->sp_arg1;
    nwords = c->sp_arg2;
    if (c->sp_ctx != 0)
        FreeSecondPassContext_ForChinese_TreeCtx(c);
    sp = heap_Alloc(c->heap, sizeof(SPCTX));
    if (sp == 0) {
        FreeSecondPassContext_ForChinese_TreeCtx(c);
        return 1;
    }
    memset(sp, 0, sizeof(SPCTX));
    c->sp_ctx = sp;
    t = heap_Alloc(c->heap, sizeof(FSTREE));
    if (t == 0) {
        FreeSecondPassContext_ForChinese_TreeCtx(c);
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
    FillContextV2VirtualTable(&ctx);
    ctx.data = c->ctx->data;
    nw = ctx.GetNbrWord(&ctx);
    props = ctx.GetpWordProp(&ctx);
    bow = ctx.GetpBeginOfWords(&ctx);
    {
        unsigned int i;

        for (i = 0; i < nw; i++) {
            unsigned short f = props[i];

            if (IsStartWord(f)) {
                n2 = i;
                if (end != -1)
                    break;
            }
            if (IsStopWord(f)) {
                end = i;
                if (n2 != -1)
                    break;
            }
        }
    }
    {
        TREEWORD *wp = words;
        unsigned int j;

        pm1 = &maxp;
        m = *pm1;
        total = 0;
        for (j = 0; j < nwords; j++) {
            int x = wp->id;
            unsigned int n;

            wp++;
            n = bow[x + 1] - bow[x];
            if (m < n)
                m = n;
            total += n + 2;
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
        heap_Free(c->heap, t);
FreeSecondPassContext_ForChinese_TreeCtx(c);
return 1;
    }
    items[0].link = -1;
    Build_Tree_ForChinese_TreeCtx(items, (TREELINK *)links, n2, end, bow, nwords, words, &nlinks, &nnodes);
    for (l = items[0].link; l != 0xFFFF; l = ((unsigned short *)&links[l])[1])
        nroots++;
    {
        unsigned int nw2;
        unsigned short *lex;
        int *begin;
        unsigned int i;
        unsigned int e;
        unsigned int m2;

        k = items[0].link;
        nw2 = nnodes - 1;
        n2 = 0;
        e = 0;
        lex = uw->lex;
        begin = uw->begin;
        while (k != 0xFFFF) {
            k = ((unsigned short *)&links[k])[1];
            e++;
        }
        pm2 = &maxn;
        m2 = e;
        for (i = 1; i <= nw2; i++) {
            int pron = items[i].pron;
            unsigned short *p = lex + begin[pron];
            unsigned short *q = lex + begin[pron + 1];

            p++;
            n2 += q - p;
            e = 0;
            k = items[i].link;
            while (k != 0xFFFF) {
                k = ((unsigned short *)&links[k])[1];
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
        heap_Free(c->heap, t);
FreeSecondPassContext_ForChinese_TreeCtx(c);
return 1;
    }
    Translate_Prons_Into_Items_And_Tones_TreeCtx(0, &nitems, &nlinks2, (TPCTX *)t);
    heap_Free(c->heap, items);
    heap_Free(c->heap, links);
    n4 = n2;
    if (StartWordsHaveTones(nw, props, uw, t)) {
        nitems2 = n4;
        Calculate_NbrOfItemNodes_In_FinalSwappedTranslatedTree_TreeCtx(0, t, &nitems2);
        t->newitems = heap_Alloc(c->heap, maxn * ((n4 + 1) * sizeof(DICTITEM)));
        ni = t->newitems;
        t->newlinks = heap_Alloc(c->heap, sizeof(DICTLINK) * (maxn * n3) * maxn);
        nl = t->newlinks;
        if (ni == 0 || nl == 0) {
            heap_Free(c->heap, ditems);
            heap_Free(c->heap, dlinks);
            if (ni != 0)
                heap_Free(c->heap, ni);
            if (nl != 0)
                heap_Free(c->heap, nl);
            heap_Free(c->heap, t);
FreeSecondPassContext_ForChinese_TreeCtx(c);
return 1;
        }
        SplitUp_Finals_TreeCtx(t, 0, &nnew, &nnewlinks);
        heap_Free(c->heap, ditems);
        heap_Free(c->heap, dlinks);
    } else {
        t->newitems = ditems;
        ni = ditems;
        t->newlinks = dlinks;
        nl = dlinks;
        nitems2 = n4;
    }
    if (Swap_Finals_TreeCtx(0xFFFF, 0, 0, t) != 0)
        goto fail2;
    Calculate_NbrOfStatesInDict_ForChinese_TreeCtx(0, &nstates, (DICTTREE *)t);
    nstates += nitems2 * 2;
    if (FillOut_Syntax_Statedict_And_Wordprops_ForChinese_TreeCtx(c, ni, nl, nstates, nitems2,
                                                                  nlinks2 - nroots + (nitems2 - n4) + 1, sp) != 0) {
fail2:
        heap_Free(c->heap, ni);
        heap_Free(c->heap, nl);
freet:
        heap_Free(c->heap, t);
fail:
        FreeSecondPassContext_ForChinese_TreeCtx(c);
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
