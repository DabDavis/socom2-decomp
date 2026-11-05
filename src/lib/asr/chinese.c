/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing -funroll-loops */
/* chinese: the recogniser's second pass for Mandarin -- the context built from the first pass's
 * words with their tones (OVL2 0x4CE5F0-0x4D0BD8, SOCOM 1's object at 0x3404E8: Build_Tree_ForChinese,
 * Translate_Prons_Into_Items_And_Tones, SplitUp_Finals, Swap_Finals,
 * FillOut_Syntax_Statedict_And_Wordprops_ForChinese, FreeSecondPassContext_ForChinese,
 * CreateSecondPassContext_ForChinese, Tone_Of_Final, Calculate_NbrOfStatesInDict_ForChinese,
 * Calculate_NbrOfItemNodes_In_FinalSwappedTranslatedTree; the five before the free and the last
 * two LOCAL there), ScanSoft's speech recogniser, prebuilt with ee-gcc 2.96 at -O2 -G0
 * -fno-strict-aliasing -funroll-loops (README "The GCC front"). The file's name is ours. Written:
 * SplitUp_Finals (round 32c, OPEN), Tone_Of_Final and the two counting walks after it; the rest of
 * the object is not
 * (FreeSecondPassContext_ForChinese stands as a matched draft). The trees' records are named by
 * use.
 *
 * Tone_Of_Final is inlined into Calculate_NbrOfItemNodes_In_FinalSwappedTranslatedTree (its
 * `unsigned char` result zero-extended once at the join, `andi 0xff`, as SOCOM 1's is) while its
 * own copy stays at its place: a GNU `extern __inline__` body above, the real function below
 * (tos.c's pattern). Nothing calls the copy in either build. */

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
