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
 * as there). */

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
    char pad28[20];
    unsigned short ntones;              /* +48 */
} FSTREE;

extern __inline__ unsigned char Tone_Of_Final_TreeCtx(unsigned short final, unsigned short n)
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
unsigned char Tone_Of_Final_TreeCtx(unsigned short final, unsigned short n)
{
    if (final != 0 && final < n)
        return 1;
    return 0;
}

/* the states a subtree's words take: a single-state item one, a pronunciation its states less
 * the two it shares (SOCOM 1's LOCAL; the order of the sum compiles the same three ways) */
static void Calculate_NbrOfStatesInDict_ForChinese_TreeCtx(unsigned int node, int *count, DICTTREE *t)
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

/* the item nodes the final swap adds: an item with a valid tone and more than one child gets a
 * node for each child after the first (SOCOM 1's LOCAL).
 * OPEN (19 words, the right size): the first child is a local (the target reuses its index and
 * link address after the tone test; re-reading the field is 0x140), but its *8 shares the
 * recursion loop's pseudo here (s0, the target's t0) and the temporaries sit one register off.
 * A variable of its own for the first child is 18 (inside the test) or the wrong size (the
 * recursion loop outside it, or one `&&` test); one for the counting loop 32-45. */
static void Calculate_NbrOfItemNodes_In_FinalSwappedTranslatedTree_TreeCtx(unsigned int node, FSTREE *t,
                                                                   int *count)
{
    unsigned int c = t->items[node].child;

    if (c != 0xFFFF) {
        if (t->links[c].next != 0xFFFF && Tone_Of_Final_TreeCtx(t->items[node].tone, t->ntones)) {
            for (c = t->links[c].next; c != 0xFFFF; c = t->links[c].next)
                (*count)++;
        }
        for (c = t->items[node].child; c != 0xFFFF; c = t->links[c].next)
            Calculate_NbrOfItemNodes_In_FinalSwappedTranslatedTree_TreeCtx(t->links[c].node, t, count);
    }
}
