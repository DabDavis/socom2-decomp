/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing -funroll-loops */
/* ctxtree: the recogniser's context tree -- the active words of an isolated-word context as a
 * tree of phoneme nodes (OVL2 0x4D3318-0x4D5B18, SOCOM 1's object at 0x343B58:
 * AllocateNodeAndFreePrevious, func_004D3450 (new in SOCOM II), AddFollowers, intCreateCtxTree, _FreeTree, CalcAccuracyCtxTree,
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
 * (round 22c: the free list tested before it is read, round 6c's shape), intCreateCtxTree (round
 * 36c, with SetUserWordTables, its inline), FreeTree, _FreeTree,
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
    void *fn[7];                        /* +4 */
    int (*GetNbrItem)(struct CTXVT *vt);                /* +32 */
    unsigned int (*GetNbrWord)(struct CTXVT *vt);       /* +36 */
    void *fn10[6];                      /* +40 */
    unsigned short *(*GetpLex)(struct CTXVT *vt);       /* +64 */
    int *(*GetpBeginOfWords)(struct CTXVT *vt);         /* +68 */
    unsigned short *(*GetpWordProp)(struct CTXVT *vt);  /* +72: a word's flags (0x80 inactive, 1 start, 2 stop) */
    unsigned short *(*GetpSyntax)(struct CTXVT *vt);    /* +76 */
    void *fn20;                         /* +80 */
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

/* AddFollowers' walk: the context's tables, the active words' bit array and the stop word's first
 * phoneme */
typedef struct AFREC {
    int nwords;                         /* +0: the words' list so far */
    unsigned short ph;                  /* +4: the stop word's first phoneme */
    short pad6;
    CASR *c;                            /* +8 */
    int *syntax;                        /* +12 */
    int *bow;                           /* +16 */
    unsigned short *lex;                /* +20 */
    unsigned int depth;                 /* +24 */
    unsigned char *bits;                /* +28 */
    unsigned short parts[56];           /* +32: a user word's phonemes (AddFollowers') */
} AFREC;                                /* 0x90 */

int AddFollowers(AFREC *r, TNODE *p, int word, int prev);

extern void heap_Free(void *heap, void *p);
extern void *heap_Realloc(void *heap, void *p, unsigned int size);
extern unsigned char _tosErrorLog(CASRLOG *log, unsigned char code);
extern int _tosControl(CASRLOG *log, int cmd, int arg, void *data);
extern void FillContextV2VirtualTable(CTXVT *vt);
extern void FreeCtxInfo(CASR *c);

int SetIsolatedWordDpActiveWords(CASR *c, unsigned int *ids, unsigned int n);
extern int *fastallo_AllocateMemoryFromChunk(FASTALLO *fa, unsigned short n);
extern void *heap_Alloc(void *heap, unsigned int size);
extern void *memcpy(void *dst, const void *src, unsigned int n);
extern void *memset(void *dst, int c, unsigned int n);

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

/* a user word's states appended to the context's user-word table and split into phonemes of the
 * sizes the tree can hold: the first of up to 8 states, the second of up to 8 more, then of up to
 * 255 each; *parts is their count, each a new entry of the offsets. New in SOCOM II (its AddFollowers
 * is the only caller; SOCOM 1 has neither): written from the code (round 40c). The growth is written
 * out with its literal (`x = x + (c ? 256 : k)`: `+=` pushes the sum into both arms, a variable for
 * the 256 is `slt`); the copy is a do-while under its own test (a `while` is left an entry jump into
 * the loop's middle, "multiple entry points", and is not unrolled); the counter is stepped in a
 * statement of its own. */
int func_004D3450(AFREC *r, unsigned short *p, unsigned short *end, unsigned short *parts)
{
    CASR *c = r->c;
    int err = 0;
    unsigned short n = end - p;
    unsigned short *o, *t;
    int *q;
    int off, last;

    if (c->uw_cap == 0 || c->uw_cap < c->uw_offsets[c->uw_n] + n + 1) {
        c->uw_cap = c->uw_cap + ((n + 1 < 256) ? 256 : n + 1);
        t = heap_Realloc(c->heap, c->uw_states, c->uw_cap * 2);
        if (t == 0) {
            err = 1;
            c->uw_cap = c->uw_cap - ((n + 1 < 256) ? 256 : n + 1);
            goto out;
        }
        c->uw_states = t;
    }
    if (n < 9)
        *parts = 1;
    else if (n < 17)
        *parts = 2;
    else
        *parts = (n - 17) / 255 + 3;
    q = heap_Realloc(c->heap, c->uw_offsets, (c->uw_n + *parts) * 4 + 4);
    if (q == 0) {
        err = 1;
        goto out;
    }
    c->uw_offsets = q;
    *q = 0;
    off = c->uw_offsets[c->uw_n];
    o = (unsigned short *)c->uw_states + off;
    if (p < end) {
        do {
            *o++ = *p++;
        } while (p < end);
    }
    last = off + n;
    c->uw_n++;
    c->uw_offsets[c->uw_n] = n < 9 ? last : off + 8;
    if (n < 9)
        goto out;
    off += 16;
    c->uw_n++;
    c->uw_offsets[c->uw_n] = n < 17 ? last : off;
    if (n < 17)
        goto out;
    n -= 16;
    do {
        off += n < 256 ? n : 255;
        c->uw_n++;
        c->uw_offsets[c->uw_n] = off;
        if (n >= 256)
            n -= 255;
        else
            n = 0;
    } while (n != 0);
out:
    return err;
}

/* a word's entry in the words' list (grown by 1024), its id, -1 when out of memory */
static __inline__ int AddWordToList(AFREC *r, int word, int prev)
{
    int id = -1;
    void *b;

    if (r->nwords == r->c->tree_nwords) {
        r->c->tree_nwords += 1024;
        b = heap_Realloc(r->c->heap, r->c->tree_words, r->c->tree_nwords * 8);
        if (b == 0) {
            r->c->tree_nwords -= 1024;
            goto done;
        }
        r->c->tree_words = b;
    }
    id = r->nwords;
    ((WORDREC *)r->c->tree_words)[id].id = word;
    ((WORDREC *)r->c->tree_words)[r->nwords].next = prev;
    r->nwords++;
done:
    return id;
}

/* a word's followers in the syntax added under a node: a follower with none of its own ends a word
 * (a leaf: at depth 2 the word itself under the stop phoneme, else its entry in the words' list),
 * one with some is added through each of its pronunciations, phoneme by phoneme (user words split by
 * func_004D3450), and recursed into. 1 when too deep or out of memory. SOCOM 1's body with the user
 * words' split (round 40c). The phoneme loop is unrolled by two only at 50 real insns or fewer
 * (loop.c's second pass: MAX_UNROLLED_INSNS / count): `ph` an int reads it with one zero-extending
 * load, an unsigned short with a load and an extension (51, "Loop too big"). Each search's locals are
 * its own (one variable is one pseudo: the target's ph is s2, s2, s1, s3 and its index a3, then a1),
 * `t[n].ph` is the index-first sum `move s1,v1` then copies, and AddWordToList has one exit (its -1
 * kept in s3 and s0, `id` the spilled copy after the test's reload). */
int AddFollowers(AFREC *r, TNODE *p, int word, int prev)
{
    unsigned short nparts;
    int *fp, *fend;
    TNODE *cur;
    int id;
    CASR *c;
    int err;
    int w;
    int *qend;
    int *q;
    unsigned short *s, *send;
    TNODE *node = p;

    c = r->c;
    fp = r->syntax + r->syntax[word];
    fend = r->syntax + r->syntax[word + 1];
    r->depth++;
    cur = node;
    id = -1;
    err = 0;
    if (c->iso_word + 1 < r->depth)
        err = 1;
    else {
        for (; fp < fend; fp++) {
            w = *fp;
            if (r->syntax[w + 1] == r->syntax[w]) {
                p = node;
                if (r->depth == 2) {
                    TNODE *ch, *t;
                    int n, i;
                    int ph;

                    ph = r->ph;
                    ch = p->children;
                    n = p->nchildren;
                    for (i = 0; i < n; i++) {
                        if (ch[i].ph == ph && ch[i].nchildren == 0)
                            break;
                    }
                    if (i < n && (int)ch[i].children == word)
                        continue;
                    t = AllocateNodeAndFreePrevious(c, n, (TNODE **)node);
                    t[n].ph = ph;
                    node->nchildren = n + 1;
                    p = t + n;
                    p->nchildren = 0;
                    p->children = (TNODE *)word;
                } else {
                    TNODE *ch, *t;
                    int n, i;
                    int ph;

                    id = AddWordToList(r, word, prev);
                    if (id == -1) {
                        err = 1;
                        goto out;
                    }
                    n = node->nchildren;
                    ph = r->ph;
                    ch = node->children;
                    for (i = 0; i < n; i++) {
                        if (ch[i].ph == ph && ch[i].nchildren == 0)
                            break;
                    }
                    if (i < n && (int)ch[i].children == -(id + 1))
                        continue;
                    t = AllocateNodeAndFreePrevious(c, n, (TNODE **)node);
                    t[n].ph = ph;
                    node->nchildren = n + 1;
                    p = t + n;
                    p->children = (TNODE *)-(id + 1);
                    p->nchildren = 0;
                }
            } else if (((r->bits[w >> 3] >> (~w & 7)) & 1) != 0) {
                q = r->bow + r->bow[w];
                qend = r->bow + r->bow[w + 1];
                if (r->depth >= 2 && node == cur) {
                    TNODE *ch, *t;
                    int n, i;
                    int ph;

                    id = AddWordToList(r, word, prev);
                    if (id == -1) {
                        err = 1;
                        goto out;
                    }
                    cur = node;
                    n = cur->nchildren;
                    ph = r->ph;
                    ch = cur->children;
                    for (i = 0; i < n; i++) {
                        if (ch[i].ph == ph && ch[i].nchildren != 0)
                            break;
                    }
                    if (i < n)
                        cur = ch + i;
                    else {
                        t = AllocateNodeAndFreePrevious(c, n, (TNODE **)node);
                        t[n].ph = ph;
                        node->nchildren = n + 1;
                        cur = t + n;
                        cur->nchildren = 0;
                    }
                }
                for (; q < qend; q++) {
                    s = r->lex + *q;
                    p = cur;
                    send = r->lex + q[1];
                    if (*s++ != 1) {
                        unsigned short k;

                        r->parts[0] = r->c->uw_n + r->c->lng->GetNbrSpeechUnit(r->c->lng) + 1;
                        if (func_004D3450(r, s, send, &nparts) != 0) {
                            err = 1;
                            goto out;
                        }
                        s = r->parts;
                        send = s + nparts;
                        for (k = 1; k < nparts; k++)
                            s[k] = k + s[0];
                    }
                    if (s < send) {
                        do {
                            TNODE *ch, *t;
                            int n, i;
                            int ph;

                            ph = *s++;
                            n = p->nchildren;
                            ch = p->children;
                            for (i = 0; i < n; i++) {
                                if (ch[i].ph == ph && ch[i].nchildren != 0)
                                    break;
                            }
                            if (i < n)
                                p = ch + i;
                            else {
                                t = AllocateNodeAndFreePrevious(c, n, (TNODE **)p);
                                t[n].ph = ph;
                                p->nchildren = n + 1;
                                p = t + n;
                                p->nchildren = 0;
                            }
                        } while (s < send);
                    }
                    if (AddFollowers(r, p, w, id) != 0) {
                        err = 1;
                        goto out;
                    }
                }
            }
        }
    }
out:
    r->depth--;
    return err;
}

/* the user words' tables kept for the tree (the context's lexicon and its pronunciations' offsets),
 * the old ones given back; 1 when there is no memory */
static __inline__ unsigned char SetUserWordTables(CASR *c, unsigned short *lex, int *offsets)
{
    UWTABLES *uw = heap_Alloc(c->heap, sizeof(UWTABLES));

    if (uw == 0)
        return 1;
    uw->states = lex;
    uw->offsets = offsets;
    if (c->uw_tables != 0)
        heap_Free(c->heap, c->uw_tables);
    c->uw_tables = uw;
    return 0;
}

/* the context's tree: a bit per word, set for the words asked for (all without a list) and for
 * every word the context keeps active (no 0x80 flag); the start word (flag 1) and the stop word
 * (flag 2) found -- a second either ends the scan -- each pronounced from a 1; the root holds the
 * start word's first phoneme and AddFollowers grows the tree from it to the stop word's; with the
 * language's bit 8 the user words' tables kept. 1 on failure. SOCOM 1's body (0x538). Measured
 * (round 36c): the walk record is 0x90 bytes (the target's frame: its spills at +144); a word's
 * pronunciation through a pointer of its own (`q = bow + bow[first]; p = lex + *q;`) -- an address
 * under a dereference is expanded as a sum (EXPAND_SUM: the multiplication first, the target's
 * inner `sll; addu v1,v1,s5`), a pointer value as a plain add (the base first: the outer `addu
 * v1,s5,v0`), so `bow[bow[first]]` gives both index-first (6 words); the record's stores in the
 * order the target has them (reversed, 13 words) */
int intCreateCtxTree(CASR *c, CTXVT *vt, unsigned int *ids, unsigned int n)
{
    unsigned short *syntax, *flags, *lex, *p;
    unsigned int nwords, size, i;
    int *bow;
    unsigned char *bits;
    int first = -1, last = -1;
    int *q;
    TNODE *root;
    AFREC rec;

    syntax = vt->GetpSyntax(vt);
    flags = vt->GetpWordProp(vt);
    nwords = vt->GetNbrWord(vt);
    lex = vt->GetpLex(vt);
    bow = vt->GetpBeginOfWords(vt);
    size = (nwords - 1) / 8 + 1;
    bits = heap_Alloc(c->heap, size);
    if (bits == 0)
        return 1;
    if (ids == 0)
        memset(bits, -1, size);
    else {
        memset(bits, 0, size);
        for (i = 0; i < n; i++)
            bits[ids[i] >> 3] |= 1 << (7 - (ids[i] & 7));
    }
    for (i = 0; i < nwords; i++) {
        if (!(flags[i] & 0x80))
            bits[i >> 3] |= 1 << (7 - (i & 7));
        if (flags[i] & 1) {
            first = i;
            if (last != -1)
                break;
        }
        if (flags[i] & 2) {
            last = i;
            if (first != -1)
                break;
        }
    }
    q = bow + bow[first];
    p = lex + *q;
    if (*p++ != 1) {
        heap_Free(c->heap, bits);
        return 1;
    }
    {
        int **list = &c->fa.free[3];

        if (*list != 0) {
            root = (TNODE *)*list;
            *list = *(int **)root;
        } else
            root = (TNODE *)fastallo_AllocateMemoryFromChunk(&c->fa, 3);
    }
    root->nchildren = 0;
    root->pad4 = 0;
    c->iso_ctx = root;
    root->ph = *p;
    q = bow + bow[last];
    p = lex + *q;
    if (*p++ != 1) {
        heap_Free(c->heap, bits);
        return 1;
    }
    rec.ph = *p;
    rec.c = c;
    rec.syntax = (int *)syntax;
    rec.nwords = 0;
    rec.depth = 0;
    rec.bow = bow;
    rec.lex = lex;
    rec.bits = bits;
    if (AddFollowers(&rec, root, first, -1) != 0) {
        heap_Free(c->heap, bits);
        return 1;
    }
    c->run_ctx = 0;
    c->map = 0;
    if (c->lng->CheckBitField(c->lng, 8) && SetUserWordTables(c, lex, bow + bow[0]) != 0) {
        heap_Free(c->heap, bits);
        return 1;
    }
    heap_Free(c->heap, bits);
    return 0;
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
    c->uw_cap = 0;
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
