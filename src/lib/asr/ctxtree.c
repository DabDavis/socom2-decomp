/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing -funroll-loops */
/* ctxtree: the recogniser's context tree -- the active words of an isolated-word context as a
 * tree of phoneme nodes (OVL2 0x4D3318-0x4D5B18, SOCOM 1's object at 0x343B58:
 * AllocateNodeAndFreePrevious, AddFollowers, intCreateCtxTree, _FreeTree, CalcAccuracyCtxTree,
 * GetIsolatedWordDpActiveWords, CreateCtxTree, FreeTree, SetIsolatedWordDpActiveWords, CalcNrNodes,
 * CalcNrNodes10, _CountWordsInTree, _GetWordsInTree, CompareIDs, CreateActiveWordsCtxTree), ScanSoft's
 * speech recogniser, prebuilt with ee-gcc 2.96 at -O2 -G0 -fno-strict-aliasing -funroll-loops
 * (README "The GCC front"). Names are SOCOM 1's, each by its body and a call site (README "The
 * recogniser and Medius by hand", round 18c).
 *
 * SOCOM II's object inlines: SetIsolatedWordDpActiveWords holds FreeTree's and
 * CreateActiveWordsCtxTree's bodies, and FreeTree _FreeTree's first level (its children's loop,
 * calling _FreeTree for theirs); CalcNrNodes10 holds
 * CalcNrNodes' first level at a budget of 12 (aNodesFr[12] read directly -- SOCOM 1's at 14);
 * GetIsolatedWordDpActiveWords holds the first levels of _CountWordsInTree and _GetWordsInTree. 2.96
 * inlines only a body it has seen and defers an inline's own copy to the object's end, in
 * declaration order -- the image's FreeTree, _FreeTree, CalcNrNodes, _CountWordsInTree,
 * _GetWordsInTree, CompareIDs, CreateActiveWordsCtxTree after SetIsolatedWordDpActiveWords,
 * CreateCtxTree and CalcAccuracyCtxTree (CompareIDs, qsort's comparator, is deferred as well: its
 * copy follows _GetWordsInTree's). Only the functions written so far are here: AllocateNodeAndFreePrevious
 * (round 22c: the free list tested before it is read, round 6c's shape), FreeTree, _FreeTree,
 * CalcNrNodes, CompareIDs and CreateActiveWordsCtxTree as the inlines they are, CalcNrNodes10,
 * SetIsolatedWordDpActiveWords, CreateCtxTree, CalcAccuracyCtxTree, and _CountWordsInTree and
 * _GetWordsInTree as plain statics until GetIsolatedWordDpActiveWords, their inliner, is written (a
 * static inline with no caller has no copy). CreateCtxTree and CreateActiveWordsCtxTree were in
 * ctxutil.c until round 19c. */

#include "casr.h"

/* a word list's entry: the word, and the next entry's index (-1 ends the list) */
typedef struct WORDREC {
    int id;                             /* +0 */
    int next;                           /* +4 */
} WORDREC;

/* the context's accessors as contextv2.c fills them (21 words: the data pointer, then 20) */
typedef struct CTXVT {
    void *data;
    void *fn[20];
} CTXVT;

/* the activated context as the engine holds it (casr.h's IMPORT: its data first) */
struct IMPORT {
    void *data;
};

/* the user words' tables, as the scheduler's command 36 takes them with the tree */
typedef struct UWTABLES {
    void *states;
    int *offsets;
} UWTABLES;

extern void heap_Free(void *heap, void *p);
extern unsigned char _tosErrorLog(CASRLOG *log, unsigned char code);
extern int _tosControl(CASRLOG *log, int cmd, int arg, void *data);
extern void FillContextV2VirtualTable(CTXVT *vt);
extern int intCreateCtxTree(CASR *c, CTXVT *vt, unsigned int *ids, unsigned int n);
extern void FreeCtxInfo(CASR *c);

int SetIsolatedWordDpActiveWords(CASR *c, unsigned int *ids, unsigned int n);
extern int *fastallo_AllocateMemoryFromChunk(FASTALLO *fa, unsigned short n);
extern void *heap_Alloc(void *heap, unsigned int size);
extern void *memcpy(void *dst, const void *src, unsigned int n);

/* a node's children grown by one: n + 1 records (three words each) from the free list of their size
 * in words -- or a new chunk -- while n is below 8, else from the heap; the n old ones copied in and
 * given back, to the free list of theirs while fewer than 9, else to the heap (as _FreeTree gives a
 * node's back); the new record's word and child count cleared. SOCOM 1's body (round 22c) */
TNODE *AllocateNodeAndFreePrevious(CASR *c, int n, TNODE **children)
{
    TNODE *t;

    if (n < 8) {
        unsigned int k = (unsigned int)(n * 12 + 15) >> 2;
        int **list = &c->fa.free[k];

        if (*list != 0) {
            t = (TNODE *)*list;
            *list = *(int **)t;
        } else
            t = (TNODE *)fastallo_AllocateMemoryFromChunk(&c->fa, k);
    } else
        t = heap_Alloc(c->tos_heap, n * 12 + 12);
    if (t == 0)
        return 0;
    t[n].nchildren = 0;
    t[n].pad4 = 0;
    if (n != 0) {
        TNODE *old = *children;

        memcpy(t, old, n * 12);
        if (n < 9) {
            int **list = &c->fa.free[n * 3];

            *(int **)old = *list;
            *list = (int *)old;
        } else
            heap_Free(c->tos_heap, old);
    }
    *children = t;
    return t;
}

int CalcAccuracyCtxTree(CASR *c);
void FreeTree(CASR *c);
static void _FreeTree(CASR *c, TNODE *n);
static float CalcNrNodes(int *tab, int *uwtab, unsigned short n, TNODE *node, int budget);
static float CalcNrNodes10(int *tab, int *uwtab, unsigned short n, TNODE *node, float best,
                           unsigned int *maxc);
static int _CountWordsInTree(WORDREC *tab, TNODE *n);
static void _GetWordsInTree(WORDREC *tab, TNODE *n, int **out);
int CompareIDs(const void *a, const void *b);
int CreateActiveWordsCtxTree(CASR *c, unsigned int *ids, unsigned int n);
long CreateCtxTree(CASR *c, CTXVT *vt);

/* each budget level's weight (SOCOM 1's aNodesFr: 13 floats) */
extern float aNodesFr[13];

/* a node's children back to the allocator, depth first: each child's subtree, then the array --
 * to the free list of its size in words (three a node) when it has fewer than 9, else to the heap */
static __inline__ void _FreeTree(CASR *c, TNODE *n)
{
    unsigned short nc = n->nchildren;
    unsigned int i;

    for (i = 0; i < nc; i++)
        _FreeTree(c, &n->children[i]);
    if (nc != 0) {
        if (nc < 9) {
            int **list = &c->fa.free[nc * 3];

            *(int **)n->children = *list;
            *list = (int *)n->children;
        } else
            heap_Free(c->tos_heap, n->children);
    }
}

/* the context tree: its nodes, the root (a node, three words), its word lists and the user words'
 * tables */
__inline__ void FreeTree(CASR *c)
{
    int **list;

    _FreeTree(c, c->iso_ctx);
    list = &c->fa.free[3];
    *(int **)c->iso_ctx = *list;
    *list = (int *)c->iso_ctx;
    c->iso_ctx = 0;
    if (c->tree_words != 0) {
        heap_Free(c->heap, c->tree_words);
        c->tree_words = 0;
        c->tree_nwords = 0;
    }
    if (c->uw_states != 0) {
        heap_Free(c->heap, c->uw_states);
        c->uw_states = 0;
    }
    c->f1128 = 0;
    if (c->uw_offsets != 0) {
        heap_Free(c->heap, c->uw_offsets);
        c->uw_offsets = 0;
    }
    c->uw_n = 0;
}

/* a subtree's weight within a budget of states: the node's phoneme takes its states off the budget
 * (a speech unit's less its two boundary states, a user-word phoneme's all), and while any is left
 * each child adds its own */
static __inline__ float CalcNrNodes(int *tab, int *uwtab, unsigned short n, TNODE *node, int budget)
{
    float sum = aNodesFr[budget];
    TNODE *ch, *end;
    int k;

    if (node->ph < n)
        k = tab[node->ph + 1] - tab[node->ph] - 2;
    else {
        unsigned short u = node->ph - n - 1;

        k = uwtab[u + 1] - uwtab[u];
    }
    budget -= k;
    if (budget > 0) {
        ch = node->children;
        end = ch + node->nchildren;
        for (; ch < end; ch++)
            sum += CalcNrNodes(tab, uwtab, n, ch, budget);
    }
    return sum;
}

/* the tree's weight: the largest of its nodes' weights within 12 states, taken at every node with
 * more children than a tenth of the most yet seen (that maximum kept at *maxc) -- CalcNrNodes' body
 * inlined there, each node's children after it. The child count is compared through its integer
 * promotion (`lhu`, litodp; a cast to double converts the unsigned short itself: `lh` and a 65536
 * fixup), and re-read after the store to *maxc (the target's reload: the store may alias it);
 * 0.1 lands an ulp low through 2.96's parser, on the target's 0x3FB9999999999999 */
static float CalcNrNodes10(int *tab, int *uwtab, unsigned short n, TNODE *node, float best,
                           unsigned int *maxc)
{
    TNODE *ch, *end;

    if (node->nchildren > *maxc * 0.1) {
        float sum;

        if (*maxc < node->nchildren)
            *maxc = node->nchildren;
        sum = CalcNrNodes(tab, uwtab, n, node, 12);
        if (best < sum)
            best = sum;
    }
    ch = node->children;
    end = ch + node->nchildren;
    for (; ch < end; ch++)
        best = CalcNrNodes10(tab, uwtab, n, ch, best, maxc);
    return best;
}

/* the tree of the activated context's words: a table of the context accessors on the stack, over
 * the activated context's data -- an `__inline__`, so SetIsolatedWordDpActiveWords holds its body
 * (the table then a block of the inline's frame, above the caller's own locals) and its own copy
 * comes out at the object's end; a call, not a sibling one (the table is a stack slot) */
__inline__ int CreateActiveWordsCtxTree(CASR *c, unsigned int *ids, unsigned int n)
{
    CTXVT vt;

    FillContextV2VirtualTable(&vt);
    vt.data = c->ctx->data;
    return intCreateCtxTree(c, &vt, ids, n);
}

/* a new list of active words for the isolated-word context: the old tree freed, a new one built
 * from the context and handed to the scheduler with the user words' tables (a null list stands for
 * none: the handle is passed for it -- SOCOM 1's movz too). The tree test is positive, its error
 * last (round 14b's rule: the target's `beqz` to the end). The accessor table is the inlined
 * CreateActiveWordsCtxTree's: integrate.c gives the inline's frame its block where the call is
 * expanded, above the tables' pair (sp+16 against sp+0), and each `&vt` is the inline's own
 * address, formed at each call (round 18c's OPEN: with the table a local of this function, CSE
 * kept the two `&vt` arguments in one saved register across FillContextV2VirtualTable). */
int SetIsolatedWordDpActiveWords(CASR *c, unsigned int *ids, unsigned int n)
{
    UWTABLES uw;
    unsigned char err;

    if (c->iso_ctx != 0) {
        if (_tosControl(c->event, 41, 0, 0) != 0)
            return c->last_error;
        FreeTree(c);
        if (n == 0)
            ids = (unsigned int *)c;
        err = CreateActiveWordsCtxTree(c, ids, n);
        if (err != 0)
            return err;
        uw.states = c->uw_states;
        uw.offsets = c->uw_offsets;
        if (_tosControl(c->event, 36, (int)c->iso_ctx, &uw) != 0) {
            FreeCtxInfo(c);
            return c->last_error;
        }
        return 0;
    }
    return _tosErrorLog(c->event, 4);
}

/* a tree over the given accessor table (no words chosen); no sibling call -- 2.96 declines one where
 * the return types differ (as mtxConstruct) */
long CreateCtxTree(CASR *c, CTXVT *vt)
{
    return intCreateCtxTree(c, vt, 0, 0);
}

/* the tree's accuracy: its weighted node count (CalcNrNodes10 over the speech units and the user
 * words' phonemes), rounded, past a constant */
int CalcAccuracyCtxTree(CASR *c)
{
    unsigned int maxc = 0;
    TNODE *root = c->iso_ctx;
    int *tab;
    int n;

    tab = c->lng->GetpSpeechUnit(c->lng);
    n = c->lng->GetNbrSpeechUnit(c->lng);
    return (unsigned int)(CalcNrNodes10(tab, c->uw_offsets, n, root, 0.0f, &maxc) + 0.5f) + 135;
}

/* the words under a node: a leaf holds a word (its index) or a list of them (~ its first entry).
 * The children's sum returns inside its positive test: so 2.96 unrolls the loop by four, as the
 * target (an if/else falling into one `return count` leaves it rolled -- _GetWordsInTree's shape);
 * the list is walked by an index of its own, so combine folds the entry test `~w != -1` into the
 * target's `beqz w` with the `nor` in its slot (reusing w keeps the compare with -1) */
static int _CountWordsInTree(WORDREC *tab, TNODE *n)
{
    int count = 0;
    unsigned short nc = n->nchildren;
    unsigned int i;

    if (nc != 0) {
        for (i = 0; i < nc; i++)
            count += _CountWordsInTree(tab, &n->children[i]);
        return count;
    } else {
        int w = (int)n->children;
        int j, k;

        if (w >= 0)
            return 1;
        k = 0;
        for (j = ~w; j != -1; j = tab[j].next)
            k++;
        return k;
    }
}

/* ... and the words themselves, appended at *out: the children's loop falls into the arms' join,
 * and 2.96 leaves it rolled, as the target's (SOCOM 1's compiler unrolled it) */
static void _GetWordsInTree(WORDREC *tab, TNODE *n, int **out)
{
    unsigned short nc = n->nchildren;
    unsigned int i;

    if (nc != 0) {
        for (i = 0; i < nc; i++)
            _GetWordsInTree(tab, &n->children[i], out);
    } else {
        int w = (int)n->children;
        int j;

        if (w >= 0)
            *(*out)++ = w;
        else
            for (j = ~w; j != -1; j = tab[j].next)
                *(*out)++ = tab[j].id;
    }
}

/* qsort's order for the active words' ids (GetIsolatedWordDpActiveWords sorts them) */
__inline__ int CompareIDs(const void *a, const void *b)
{
    return *(const int *)a - *(const int *)b;
}
