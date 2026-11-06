/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing -funroll-loops */
/* ctxinfo: the recogniser's context-information object (OVL2 0x4CCE90-0x4CE5F0, SOCOM 1's object at
 * 0x33E418: CreateSecondPassInfo (LOCAL), CreateExEvCtxInfo, CreateCtxInfo, FreeCtxInfo,
 * CalcAccuracyCtxInfo, CreateMenuMasks -- the same six in the same order), ScanSoft's speech
 * recogniser, prebuilt with ee-gcc 2.96 at -O2 -G0 -fno-strict-aliasing -funroll-loops (README "The
 * GCC front"). Names are SOCOM 1's (s1:calls, the object's order agreeing). FreeCtxInfo stands as its
 * matched draft. */
#include "casr.h"

void *heap_Alloc(void *heap, unsigned int size);
extern void heap_Free(void *heap, void *p);
extern unsigned char _tosErrorLog(void *log, unsigned short code);
extern void *memset(void *dst, int c, unsigned int n);
extern int casrGetParam(CASR *c, int id, unsigned int *value);

/* a context's accessors (contextv2.c's _ContextV2VirtualTable) */
typedef struct CTXVT CTXVT;
struct CTXVT {
    void *data;                                     /* +0 */
    void *s1[8];
    unsigned int (*GetNbrWord)(CTXVT *vt);          /* +36, slot 9 */
    unsigned int (*GetNbrPron)(CTXVT *vt);          /* +40, slot 10 */
    void *s11[4];
    int (*IsSpellingContext)(CTXVT *vt);            /* +60, slot 15 */
    unsigned short *(*GetpLex)(CTXVT *vt);          /* +64, slot 16 */
    int *(*GetpBeginOfWords)(CTXVT *vt);            /* +68, slot 17 */
    unsigned short *(*GetpWordProp)(CTXVT *vt);     /* +72, slot 18 */
    unsigned int *(*GetpSyntax)(CTXVT *vt);         /* +76, slot 19 */
};

/* the user words' tables (ctxtree.c's) */
typedef struct UWTABLES {
    void *states;
    int *offsets;
} UWTABLES;

/* a word's record for the second pass: its score, its length and the speech-unit classes at its ends */
typedef struct SPINFO {
    int score;                          /* +0 */
    short len;                          /* +4 */
    unsigned char first;                /* +6 */
    unsigned char last;                 /* +7 */
} SPINFO;

/* each word's record: a pronunciation of kind 1 is a phoneme list -- its first and last phoneme's units
 * give the classes, the first's states (less two, when there are three or more) join the score, and the
 * inner phonemes' states the length; any other kind is the user word's class throughout. SOCOM 1's body. */
int CreateSecondPassInfo(CASR *c, unsigned short *lex, int *bow, int *scores, unsigned int n)
{
    int *su = c->lng->GetpSpeechUnit(c->lng);
    unsigned char cls = c->lng->GetUserWordSpeechUnitClass(c->lng);
    SPINFO *info = heap_Alloc(c->heap, n * sizeof(SPINFO));
    unsigned int i;

    if (info == 0)
        return 1;
    c->f588 = info;
    for (i = 0; i < n; i++) {
        unsigned short *p = lex + bow[i];
        unsigned short *e = lex + bow[i + 1];
        unsigned int kind = *p++;
        int len = e - p;

        if (kind == 1) {
            int adj = 0;
            int sum = 0;

            e--;
            info->first = *(unsigned char *)((short *)su + su[*p]);
            info->last = ((unsigned char *)((short *)su + su[*e]))[2];
            if (len >= 3) {
                unsigned int x = *p++;

                e--;
                adj = su[x + 1] - su[x] - 2;
            }
            info->score = *scores++ + adj;
            for (; p <= e; p++) {
                unsigned int x = *p;

                sum += (unsigned short)((unsigned short)(((unsigned short *)su)[x * 2 + 2] - ((unsigned short *)su)[x * 2]) - 2);
            }
            info->len = sum;
        } else {
            info->score = *scores++;
            info->len = len;
            info->last = cls;
            info->first = cls;
        }
        info++;
    }
    return 0;
}

/* the extra events' rejection word as the engine runs it (casr +608): the states of each of its pronunciations
 * in a row, the silence at both ends. SOCOM 1's body. */
typedef struct EXEVINFO {
    int nstates;                        /* +0 */
    unsigned short *states;             /* +4 */
    unsigned short *starts;             /* +8: each pronunciation's first state, n + 1 */
    int n;                              /* +12 */
    int penalty;                        /* +16 */
    int path_penalty;                   /* +20 */
} EXEVINFO;

extern void FreeCtxInfo(CASR *c);

#define EXEV(c) ((EXEVINFO *)(c)->f608)

int CreateExEvCtxInfo(CASR *c)
{
    void *ev;
    int word, sil;
    int *bow, *p, *pe, *pb;
    unsigned short *items, *q, *qe;
    int *su;
    int total = 0;
    int n = 0;
    unsigned short *w, *st, *base;

    ev = c->lng->GetpExtraEventContext(c->lng);
    word = c->lng->exevGetRejection_NBS_WordIndex(ev);
    sil = c->lng->exevGetExtraEventSilencePhenome(ev);
    bow = c->lng->exevGetpBeginOfWords(ev);
    items = c->lng->exevGetpBeginOfItems(ev);
    pb = bow + bow[word];
    pe = bow + bow[word + 1];
    su = c->lng->GetpSpeechUnit(c->lng);
    p = pb;
    c->f608 = (int)heap_Alloc(c->heap, sizeof(EXEVINFO));
    if (c->f608 == 0)
        goto fail;
    while (p < pe) {
        q = items + *p;
        p++;
        qe = items + *p;
        for (; q < qe; q++)
            total += ((unsigned short *)su)[*q * 2 + 2] - su[*q] - 2;
        n++;
    }
    total += 2;
    EXEV(c)->starts = heap_Alloc(c->heap, n * 2 + 4);
    if (EXEV(c)->starts == 0)
        goto fail;
    EXEV(c)->states = heap_Alloc(c->heap, total * 2);
    if (EXEV(c)->states == 0)
        goto fail;
    base = EXEV(c)->states;
    w = EXEV(c)->starts;
    *w++ = 0;
    st = base;
    *st++ = sil;
    for (p = pb; p < pe; ) {
        *w++ = st - base;
        q = items + *p;
        p++;
        qe = items + *p;
        while (q < qe) {
            unsigned int x = *q++;
            unsigned short *a = (unsigned short *)su + su[x] + 2;
            unsigned short *b = (unsigned short *)su + su[x + 1];

            for (; a < b; a++)
                *st++ = *a;
        }
    }
    *st = sil;
    *w = st - base;
    EXEV(c)->n = n;
    EXEV(c)->penalty = c->lng->exevGetRejection_NBS_penalty(ev);
    EXEV(c)->path_penalty = c->lng->exevGetRejection_NBS_path_penalty(ev);
    EXEV(c)->nstates = total;
    return 0;
fail:
    FreeCtxInfo(c);
    return 1;
}

/* the context the engine runs (casr +580), and the spelling pass's (+584): its counts, its state
 * dictionary -- each pronunciation's states, the last marked, then the end mark and the word in two
 * halves -- with each word's first entry and each pronunciation's end */
typedef struct CTXINFO {
    int f0;                             /* +0: 1 */
    unsigned int n;                     /* +4: words */
    int nstates;                        /* +8 */
    unsigned short *props;              /* +12 */
    unsigned short *dict;               /* +16 */
    unsigned short *wtp;                /* +20: each word's penalty */
    unsigned int *bounds;               /* +24 */
    int *tab;                           /* +28 */
    unsigned char *mask;                /* +32 */
} CTXINFO;

/* a mask of n entries, all set: inlined into CreateCtxInfo (the extern-inline trick: the row is the
 * object's last) */
extern __inline__ unsigned char CreateMenuMasks(CASR *c, CTXINFO *m, unsigned int n)
{
    unsigned int size = ((n - 1) >> 3) + 1;
    unsigned char *mask;
    unsigned int i;

    mask = heap_Alloc(c->heap, size);
    m->mask = mask;
    if (mask == 0)
        return 1;
    for (i = 0; i < size - 1; i++)
        mask[i] = 0xFF;
    mask[i] = 0xFF << (-(n & 7) & 7);
    return 0;
}

/* the user words' tables kept with the context (ctxtree.c's SetUserWordTables) */
static __inline__ unsigned char SetUserWordTables(CASR *c, unsigned short *lex, int *offsets)
{
    UWTABLES *uw = heap_Alloc(c->heap, sizeof(UWTABLES));

    if (uw == 0) {
        _tosErrorLog(c->event, 1);
        return 1;
    }
    uw->states = lex;
    uw->offsets = offsets;
    if (c->uw_tables != 0)
        heap_Free(c->heap, c->uw_tables);
    c->uw_tables = uw;
    return 0;
}

/* the context's run-time form: the state dictionary, its offsets, the words' penalties and mask, the
 * second-pass records (or the user words' tables under the language's bit 8); a spelling context a
 * second record with no penalties and every word active. A failed record's allocation returns 0 (the
 * known zero in v0), the rest are freed and return 1. SOCOM 1's body (round 40c), measured:
 * - vt, the mark, the pronunciation count, the words' table, the size and the flag are spilled, in that
 *   order of declaration; the state count is three statements (35 weighted refs against 31: it takes
 *   s4 from the properties) and pr is `prons + 1` then pr++ (17 refs: it takes s1 from nph, and the
 *   last call-crossing register from vt);
 * - w is one variable for the penalties' table and the spelling copy's: it crosses GetpWordProp's call,
 *   which is why the second is `move s0,v0` with a copy for the walker;
 * - each loop's end pointer is its own variable (one `e` for the count and the fill: t2 for a2);
 * - the tab pointer is its own walker (`t4++`), bow[i + 1] is indexed (its giv's init sits inside the
 *   n != 0 guard), the end mark is `*o++` and the word's halves `o[0], o[1]` before `o += 2`;
 * - one `return 0` after the spelling arm's join: reorg steals its `move v0,zero` into the arm's `b`,
 *   and the else's store into the branch-likely's slot;
 * - CreateMenuMasks and SetUserWordTables are inlines (the former by the extern-inline trick, the row
 *   being the object's last), `andi v0,v0,0xff` their unsigned char at the test. */
int CreateCtxInfo(CASR *c, CTXVT *vt)
{
    unsigned int param;
    unsigned short mark;
    unsigned int nprons;
    int *bow;
    int size;
    unsigned short flag = 0;
    int nph = 0;
    int nst = 0;
    int *su;
    unsigned int n;
    unsigned short *lex;
    int *prons;
    int npr;
    int *pr;
    unsigned short *p;
    unsigned int i;
    unsigned short *dict, *o;
    int *tab, *t4, *t5;
    CTXINFO *ci;
    unsigned short *prop;
    int size2;
    unsigned short *w;
    unsigned int *syn;

    mark = (c->lng->GetSilPhenome(c->lng) << 2) | 1;
    su = c->lng->GetpSpeechUnit(c->lng);
    n = vt->GetNbrWord(vt);
    size = n * 4;
    npr = n + 1;
    nprons = vt->GetNbrPron(vt);
    lex = vt->GetpLex(vt);
    bow = vt->GetpBeginOfWords(vt);
    prons = bow + n;
    pr = prons + 1;
    if (casrGetParam(c, 15, &param) != 0)
        return 1;
    if (param != 0)
        flag = c->lng->CheckBitField(c->lng, 8) ? 2 : 0;
    ci = heap_Alloc(c->heap, sizeof(CTXINFO));
    c->run_ctx = ci;
    if (ci == 0)
        return 0;
    c->map = 0;
    p = lex;
    for (i = 1; i <= nprons; i++) {
        unsigned short *e = lex + prons[i + 1];

        if (*p++ == 1) {
            nph += e - p;
            for (; p < e; p++)
                nst += su[*p + 1] - su[*p];
        } else {
            nst += e - p;
            p = e;
        }
    }
    nst -= nph * 2;
    nst += nprons;
    nst += nprons * 2;
    ((CTXINFO *)c->run_ctx)->tab = heap_Alloc(c->heap, (n + nprons) * 4 + 8);
    if (((CTXINFO *)c->run_ctx)->tab == 0)
        goto fail;
    ((CTXINFO *)c->run_ctx)->dict = heap_Alloc(c->heap, nst * 2);
    dict = ((CTXINFO *)c->run_ctx)->dict;
    if (dict == 0)
        goto fail;
    t4 = ((CTXINFO *)c->run_ctx)->tab;
    o = dict;
    p = lex;
    *t4 = npr;
    pr++;
    tab = t4 + n;
    t4++;
    tab[1] = 0;
    t5 = tab + 2;
    for (i = 0; i < n; i++) {
        int *pe = bow + bow[i + 1];

        while (pr <= pe) {
            unsigned short type = *p++;
            unsigned short *e = lex + *pr;

            if (type == 1) {
                for (; p < e; ) {
                    unsigned int x = *p++;
                    unsigned short *a = (unsigned short *)su + su[x] + 2;
                    unsigned short *b = (unsigned short *)su + su[x + 1];

                    for (; a < b; a++)
                        *o++ = *a << 2;
                }
            } else {
                for (; p < e; p++)
                    *o++ = *p << 2;
            }
            *(o - 1) |= flag;
            pr++;
            npr++;
            *o++ = mark;
            o[0] = i;
            o[1] = i >> 16;
            o += 2;
            *t5++ = o - dict;
        }
        *t4++ = npr;
    }
    size2 = n * 2;
    syn = vt->GetpSyntax(vt);
    w = heap_Alloc(c->heap, size2);
    ((CTXINFO *)c->run_ctx)->wtp = w;
    if (w == 0)
        goto fail;
    prop = vt->GetpWordProp(vt);
    for (i = 0; i < n; i++)
        w[i] = (prop[i] & 0x80) ? c->wtp : 0;
    if (CreateMenuMasks(c, c->run_ctx, n) != 0)
        goto fail;
    ((CTXINFO *)c->run_ctx)->nstates = nst;
    ((CTXINFO *)c->run_ctx)->n = n;
    ((CTXINFO *)c->run_ctx)->bounds = syn;
    ((CTXINFO *)c->run_ctx)->props = prop;
    ((CTXINFO *)c->run_ctx)->f0 = 1;
    if (c->lng->CheckBitField(c->lng, 8) != 0) {
        if (SetUserWordTables(c, lex, bow + *bow) != 0)
            goto fail;
    } else {
        if (CreateSecondPassInfo(c, lex, bow + *bow, ((CTXINFO *)c->run_ctx)->tab + n + 1, nprons) != 0)
            goto fail;
    }
    if (vt->IsSpellingContext(vt) != 0) {
        unsigned short *t2;
        unsigned int *t3;

        c->f584 = heap_Alloc(c->heap, sizeof(CTXINFO));
        if (c->f584 == 0)
            goto fail;
        *(CTXINFO *)c->f584 = *(CTXINFO *)c->run_ctx;
        w = heap_Alloc(c->heap, size2);
        if (w == 0)
            goto fail;
        ((CTXINFO *)c->f584)->wtp = w;
        for (i = 0; i < n; i++)
            w[i] = 0;
        t2 = heap_Alloc(c->heap, size2);
        if (t2 == 0)
            goto fail;
        ((CTXINFO *)c->f584)->props = t2;
        for (i = 0; i < n; i++) {
            if (prop[i] & 0x80)
                t2[i] = 0x83;
            else
                t2[i] = 0;
        }
        t3 = heap_Alloc(c->heap, size + 4);
        if (t3 == 0)
            goto fail;
        ((CTXINFO *)c->f584)->bounds = t3;
        memset(t3, 0, size + 4);
    } else
        c->f584 = 0;
    return 0;
fail:
    FreeCtxInfo(c);
    return 1;
}

/* the second pass's accuracy window: six times the longest word, no less than 50 times the mean
 * word length, no less than 300 -- 1500 when the engine runs the DP. SOCOM 1's body: the mean
 * taken before the test (the target divides first), one result returned at the end (guard clauses
 * set v0 at each exit; the target keeps it in t1 to a final move), and the word count a variable
 * taken right after `end` -- combine then folds `(p + n) - p` to the target's `(n << 2) >> 2`,
 * where a subtraction written after the `*end` load (which uses end in between) stays `subu` */
unsigned int CalcAccuracyCtxInfo(CASR *c)
{
    CTXINFO *ci = c->run_ctx;
    unsigned int *p = ci->bounds;
    unsigned int *end = p + ci->n;
    unsigned int nw = end - p;
    unsigned int max = 0;
    unsigned int mean, acc;

    mean = (*end - *p) * 50 / nw;
    if (c->use_dp != 0)
        acc = 1500;
    else {
        for (; p < end; p++)
            if (max < p[1] - p[0])
                max = p[1] - p[0];
        acc = max * 6;
        if (acc < mean)
            acc = mean;
        if (acc < 300)
            acc = 300;
    }
    return acc;
}

/* a mask of n entries, all set: SOCOM 1's body (its fill loop unrolled by four here), the fill's
 * bound `size - 1` -- the unroller copies it (the target's `move t2,s1`; a bound variable of its
 * own is used as it stands) */
unsigned char CreateMenuMasks(CASR *c, CTXINFO *m, unsigned int n)
{
    unsigned int size = ((n - 1) >> 3) + 1;
    unsigned char *mask;
    unsigned int i;

    mask = heap_Alloc(c->heap, size);
    m->mask = mask;
    if (mask == 0)
        return 1;
    for (i = 0; i < size - 1; i++)
        mask[i] = 0xFF;
    mask[i] = 0xFF << (-(n & 7) & 7);
    return 0;
}
