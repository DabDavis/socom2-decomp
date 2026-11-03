/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing -funroll-loops */
/* chinese: the recogniser's second pass for Mandarin -- the context built from the first pass's
 * words with their tones (OVL2 0x4CE5F0-0x4D0BD8, SOCOM 1's object at 0x3404E8: Build_Tree_ForChinese,
 * Translate_Prons_Into_Items_And_Tones, SplitUp_Finals, Swap_Finals,
 * FillOut_Syntax_Statedict_And_Wordprops_ForChinese, FreeSecondPassContext_ForChinese,
 * CreateSecondPassContext_ForChinese, Tone_Of_Final, Calculate_NbrOfStatesInDict_ForChinese,
 * Calculate_NbrOfItemNodes_In_FinalSwappedTranslatedTree; the five before the free and the last
 * two LOCAL there), ScanSoft's speech recogniser, prebuilt with ee-gcc 2.96 at -O2 -G0
 * -fno-strict-aliasing -funroll-loops (README "The GCC front"). The file's name is ours. Written:
 * Tone_Of_Final and the two counting walks after it; the rest of the object is not
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
    char pad2[10];
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

/* the final-swapped translated tree: the same items and links at +12 and +16, the tones' count at
 * +48 */
typedef struct FSTREE {
    char pad0[12];
    DICTITEM *items;                    /* +12 */
    DICTLINK *links;                    /* +16 */
    char pad20[28];
    unsigned short ntones;              /* +48 */
} FSTREE;

extern __inline__ unsigned char Tone_Of_Final(unsigned short final, unsigned short n)
{
    if (final != 0 && final < n)
        return 1;
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
