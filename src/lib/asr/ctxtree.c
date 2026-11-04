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
 * SOCOM II's object inlines: SetIsolatedWordDpActiveWords holds FreeTree's body, and FreeTree
 * _FreeTree's first level (its children's loop, calling _FreeTree for theirs); GetIsolatedWord-
 * DpActiveWords holds the first levels of _CountWordsInTree and _GetWordsInTree. 2.96 inlines only
 * a body it has seen and defers an inline's own copy to the object's end, in declaration order --
 * the image's FreeTree, _FreeTree, CalcNrNodes, _CountWordsInTree, _GetWordsInTree, CompareIDs,
 * CreateActiveWordsCtxTree after SetIsolatedWordDpActiveWords (CalcNrNodes is called only from
 * CalcNrNodes10, defined before it: an inline never inlined). Only the functions written so far are
 * here: FreeTree and _FreeTree as the inlines they are, SetIsolatedWordDpActiveWords (OPEN),
 * CalcAccuracyCtxTree, CalcNrNodes, _CountWordsInTree and _GetWordsInTree (as plain statics
 * until their inliners are written -- a static inline with no caller has no copy: CalcNrNodes is
 * then a static inline defined after CalcNrNodes10, and the two word walks inlines defined above
 * GetIsolatedWordDpActiveWords); CreateCtxTree and CreateActiveWordsCtxTree are in ctxutil.c. */

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
extern float CalcNrNodes10(int *tab, int *uwtab, unsigned short n, TNODE *node, float best,
                           unsigned int *maxc);

int SetIsolatedWordDpActiveWords(CASR *c, unsigned int *ids, unsigned int n);
int CalcAccuracyCtxTree(CASR *c);
void FreeTree(CASR *c);
static void _FreeTree(CASR *c, TNODE *n);
static float CalcNrNodes(int *tab, int *uwtab, unsigned short n, TNODE *node, int budget);
static int _CountWordsInTree(WORDREC *tab, TNODE *n);
static void _GetWordsInTree(WORDREC *tab, TNODE *n, int **out);

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

/* a new list of active words for the isolated-word context: the old tree freed, a new one built
 * from the context and handed to the scheduler with the user words' tables (a null list stands for
 * none: the handle is passed for it -- SOCOM 1's movz too). The tree test is positive, its error
 * last (round 14b's rule: the target's `beqz` to the end).
 * OPEN (6 words, the frame): the target keeps the tables' pair at sp+0 and the accessor table at
 * sp+16 -- the pair declared first, or the table in the inner block, gives that frame but then the
 * two `&vt` arguments (sibling-call placeholders compute arguments into pseudos) are merged by CSE
 * into a saved register across FillContextV2VirtualTable (`addiu s0,sp,16`, 2 words longer), where
 * the target computes `addiu a0,sp,16` and `addiu a1,sp,16` at each call; -fno-cse-skip-blocks,
 * -fno-cse-follow-jumps and the null-list test between the calls leave it
 * (build/scratch/gcc/ctree/v_siw*.py). As here, with the table first at sp+0, every register is
 * the target's. */
int SetIsolatedWordDpActiveWords(CASR *c, unsigned int *ids, unsigned int n)
{
    CTXVT vt;
    UWTABLES uw;
    unsigned char err;

    if (c->iso_ctx != 0) {
        if (_tosControl(c->event, 41, 0, 0) != 0)
            return c->last_error;
        FreeTree(c);
        if (n == 0)
            ids = (unsigned int *)c;
        FillContextV2VirtualTable(&vt);
        vt.data = c->ctx->data;
        err = intCreateCtxTree(c, &vt, ids, n);
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

/* a subtree's weight within a budget of states: the node's phoneme takes its states off the budget
 * (a speech unit's less its two boundary states, a user-word phoneme's all), and while any is left
 * each child adds its own */
static float CalcNrNodes(int *tab, int *uwtab, unsigned short n, TNODE *node, int budget)
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
