/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing -funroll-loops */
/* nbestdp: the recogniser's N-best dynamic programming block (OVL2 0x4EB5E8-0x4F0CB8, SOCOM 1's
 * object at 0x358DF0: CollectDummyMemory, InitNBestDynProgProcess, InitDynprogContext,
 * CleanUpParBTProc, PrunDynProg, the two DynProgAccNBest passes, CopyParBT, BacktraceWordsParBT,
 * FindBestBacktraces, FindBestRecognised, ExpandGarbageArrays, NBestDynProgProcess,
 * ControlNBestDynProgProcess, ConstructNBestDynProg, ExtendBacktraceStack, ScoreCompare,
 * IdleProcess, all but the constructor and FindBestRecognised LOCAL there), ScanSoft's speech
 * recogniser, prebuilt with ee-gcc 2.96 at -O2 -G0 -fno-strict-aliasing -funroll-loops (README
 * "The GCC front"). Names are SOCOM 1's. Only the functions written so far are here, in image
 * order: CollectDummyMemory (round 20c), InitNBestDynProgProcess and PrunDynProg (round 19c; PrunDynProg closed in 20c), FindBestRecognised (round
 * 17c), ExtendBacktraceStack, ScoreCompare (NAME@nbestdp in the map: the iso-word object has its
 * own). SOCOM II's rows hold SOCOM 1's in order: 0x4EB5E8 is CollectDummyMemory's position,
 * 0x4EB7D8 InitViterbi's, 0x4EBCB0 InitDynprogContext's, 0x4EC130 new. The backtrace stack is a list of three-word records from the
 * scheduler's small-block allocator (fastallo.c), a recycled one first. */

/* the scheduler's small-block allocator, as fastallo.c keeps it */
typedef struct FASTALLO {
    void *heap;                         /* +0 */
    unsigned short chunk_size;          /* +4 */
    unsigned short left;                /* +6 */
    int nchunks;                        /* +8 */
    int **chunks;                       /* +12 */
    int *next;                          /* +16 */
    int *free[65];                      /* +20: a free list per size in words */
} FASTALLO;

/* the scheduler, as far as the DP reads it */
typedef struct DPTOS {
    char pad0[80];
    void *heap;                         /* +80 */
    FASTALLO fa;                        /* +84 */
} DPTOS;

/* a backtrace stack record (three words) */
typedef struct BTSTACK {
    struct BTSTACK *prev;               /* +0 */
    struct BTSTACK *next;               /* +4 */
    int entry;                          /* +8 */
} BTSTACK;

/* a backtrace record the tokens share: its count of users at +12 */
typedef struct BTREC {
    int pad0[3];
    unsigned short users;               /* +12 */
} BTREC;

/* a token on the active lists at +112 (seven words) and +116 (five) */
typedef struct NBTOKEN {
    struct NBTOKEN *next;               /* +0 */
    int pad4[2];
    BTREC *bt;                          /* +12 */
} NBTOKEN;

/* a link of a word record's list (two words) */
typedef struct NBLINK {
    struct NBLINK *next;                /* +0 */
} NBLINK;

/* a word record on the list at +120 (five words): its backtrace and a list of links */
typedef struct NBWORD {
    struct NBWORD *next;                /* +0 */
    int pad4;
    BTREC *bt;                          /* +8 */
    int pad12;
    NBLINK *links;                      /* +16 */
} NBWORD;

/* a recognised word's record in a result (five words), chained at +16 */
typedef struct NBRWORD {
    int pad0[4];
    struct NBRWORD *next;               /* +16 */
} NBRWORD;

/* a recognised word sequence, chained best first */
typedef struct NBRESULT {
    char pad0[12];
    NBRWORD *words;                     /* +12 */
    struct NBRESULT *next;              /* +16 */
} NBRESULT;

/* a backtrace's head as the N-best ranks it */
typedef struct NBTRACE {
    int pad0;
    short score;                        /* +4 */
} NBTRACE;

/* a block's reader in an input queue (tosblk.h's QIN) */
typedef struct NBQIN {
    void *q;
    unsigned short reader;              /* +4 */
    unsigned short size;                /* +6 */
} NBQIN;

typedef struct NBESTDP {
    DPTOS *tos;                         /* +0: the block's head (tosblk.h) */
    char pad4[8];
    NBQIN *inputs;                      /* +12 */
    char pad16[72];
    BTSTACK *stack;                     /* +88: the stack's first record */
    NBRESULT *results;                  /* +92 */
    char pad96[12];
    unsigned int frame;                 /* +108: the frames so far */
    NBTOKEN *tok7;                      /* +112 */
    NBTOKEN *tok5;                      /* +116 */
    NBWORD *words;                      /* +120 */
    char pad124[48];
    int f172;                           /* +172 */
    unsigned short f176;                /* +176 */
    char pad178[6];
    unsigned int nresults;              /* +184: the sequences asked for (profile 7) */
    char pad188[8];
    unsigned int beam;                  /* +196: profile 6 */
    unsigned int beam10;                /* +200: ten times it */
    char pad204[4];
    unsigned int maxframes;             /* +208: profile 9 */
    unsigned char pruning;              /* +212 */
    char pad213;
    short level;                        /* +214 */
    short maxlevel;                     /* +216: profile 11 */
    char pad218[6];
    unsigned int rate;                  /* +224 */
    unsigned int f228;                  /* +228: profile 10's fraction, in 128ths */
    unsigned int hist[5];               /* +232 */
    unsigned int hist2[5];              /* +252 */
    unsigned char nhist;                /* +272 */
    char pad273[7];
    short f280;                         /* +280: profile 14 */
} NBESTDP;

extern int *fastallo_AllocateMemoryFromChunk(FASTALLO *fa, unsigned short n);
extern unsigned int _tosGetProfileU32(void *b, unsigned short key, unsigned int def);
extern float _tosGetProfileFloat(void *b, unsigned short key, float def);

extern unsigned int FindBestBacktraces(NBESTDP *d, NBTRACE ***bt);
extern void CleanUpParBTProc(NBESTDP *d, BTREC *bt);
extern NBRESULT *BacktraceWordsParBT(NBESTDP *d, NBTRACE *bt);
extern void heap_Free(void *heap, void *p);

/* a record given back to the scheduler's free list of its size, its first word the link. The
 * list's address is a pointer of the push's own (`fl_`): loop.c hoists each as an invariant of its
 * own -- the target's `addiu s3,s6,132` in each preheader, and in the results' two loops a second
 * register copied from the first (`move t1,t0`) -- where the free list spelled
 * as a field is addressed off tos each time (`lw v0,132(s4)`), and a pointer declared at the top
 * of a loop's body takes another priority in global-alloc (the word records' list and their copy
 * swap s3/s4) */
#define FREE_REC(tos, k, p) { int **fl_ = &(tos)->fa.free[k]; *(int **)(p) = *fl_; *fl_ = (int *)(p); }

/* the pass's memory given back: every token, word record and result to the scheduler's free lists
 * by size (seven words, five, two), a backtrace losing its last user cleaned up. SOCOM 1's body is
 * the middle two lists (0x118 bytes), with a scheduler pointer of its own (read again after the
 * first list: PRE reloads it on that loop's exit, the target's s5); SOCOM II adds the seven-word
 * tokens before them and the results after (0x1EC), through the pointer read at the top (s6).
 * The record is copied before its backtrace's test (a saved register across the call) */

static void CollectDummyMemory(NBESTDP *d)
{
    DPTOS *tos = d->tos;
    NBTOKEN *t, *pt;
    NBWORD *w, *pw;
    NBRESULT *r, *pr;

    for (t = d->tok7; t != 0; ) {
        pt = t;
        if (t->bt != 0 && --t->bt->users == 0)
            CleanUpParBTProc(d, t->bt);
        t = t->next;
        FREE_REC(tos, 7, pt);
    }
    d->frame = 0;
    d->tok7 = 0;
    {
        DPTOS *tos = d->tos;

        for (t = d->tok5; t != 0; ) {
            pt = t;
            if (t->bt != 0 && --t->bt->users == 0)
                CleanUpParBTProc(d, t->bt);
            t = t->next;
            FREE_REC(tos, 5, pt);
        }
        for (w = d->words; w != 0; ) {
            NBLINK *l = w->links;

            pw = w;
            if (w->bt != 0 && --w->bt->users == 0)
                CleanUpParBTProc(d, w->bt);
            while (l != 0) {
                NBLINK *pl = l;

                l = l->next;
                FREE_REC(tos, 2, pl);
            }
            w = w->next;
            FREE_REC(tos, 5, pw);
        }
        d->tok5 = 0;
        d->words = 0;
    }
    for (r = d->results; r != 0; ) {
        NBRWORD *rw = r->words;

        pr = r;
        while (rw != 0) {
            NBRWORD *prw = rw;

            rw = rw->next;
            FREE_REC(tos, 5, prw);
        }
        r = r->next;
        FREE_REC(tos, 5, pr);
    }
    d->results = 0;
}

/* the block's profile: SOCOM 1's body, its fraction converted inline where SOCOM 1 called fptoui
 * (0.2f lands an ulp low through 2.96's parser, on the target's 0x3E4CCCCC); the inputs' frame
 * sizes last */
static int InitNBestDynProgProcess(NBESTDP *d)
{
    d->f172 = 0;
    d->beam = _tosGetProfileU32(d, 6, 50);
    d->beam10 = d->beam * 10;
    d->nresults = _tosGetProfileU32(d, 7, 10);
    d->maxframes = _tosGetProfileU32(d, 9, 300);
    d->f228 = _tosGetProfileFloat(d, 10, 0.2f) * 128.0f;
    d->maxlevel = _tosGetProfileU32(d, 11, 5500);
    d->f280 = _tosGetProfileU32(d, 14, 0);
    d->results = 0;
    d->inputs[1].size = _tosGetProfileU32(d, 15, 4);
    d->inputs[0].size = 1;
    return 0;
}

/* the pruning level's update, once the pass has run a while (or has seen 11 ...): the rate of the
 * last five frames' sums, a saturating fixed-point product, the level moved by it within the
 * maximum and not below a third of where it was; then this frame into the five-entry history.
 * SOCOM 1's body (0x1F4 in both builds). Measured: the flag's store before the flag (`d->pruning
 * = 1; pruning = 1;` -- the store takes a constant of its own, the target's v0), the ring index an
 * int (no `andi` after its increment), `x < 0` and `x > MAX` two tests (one `||` folds into an
 * unsigned compare), the sum an int, the rate read into x itself before the product (`x =
 * d->rate; ... x = diff * (d->f228 * x)`: round 19c's 17 words, registers only, were global-alloc
 * giving x -- a dozen references over the clamps, allocated first -- the first free register, a0;
 * with the rate's load in x's own live range x conflicts with the f228 load local-alloc put in a0
 * and takes a1, the old level then a0: the target's, byte-exact. A rate local of its own leaves x
 * free of a0, whatever the order). */
static void PrunDynProg(NBESTDP *d)
{
    unsigned char pruning = d->pruning;

    if (pruning) {
        unsigned int s1 = 0, s2 = 1;
        int i, diff, x, t;
        short old;

        for (i = 0; i < 5; i++) {
            s1 += d->hist[i];
            s2 += d->hist2[i];
        }
        if (s1 > 0x1FFFFFF)
            d->rate = s1 / (s2 >> 7);
        else
            d->rate = (s1 << 7) / s2;
        x = d->rate;
        diff = d->maxframes - d->frame;
        x = diff * (d->f228 * x);
        if (diff > 0) {
            if (x < 0)
                x = 0x1FFFC000;
            else if (x > 0x1FFFC000)
                x = 0x1FFFC000;
        } else {
            if (x > 0)
                x = -0x1FFFC000;
            else if (x < -0x1FFFC000)
                x = -0x1FFFC000;
        }
        old = d->level;
        t = (x >> 14) + old;
        if (t > d->maxlevel)
            d->level = d->maxlevel;
        else
            d->level = t;
        if (d->level < old / 3)
            d->level = old / 3;
    } else if (d->f176 >= 11 || (d->maxframes >> 2) < d->frame) {
        d->pruning = 1;
        pruning = 1;
    }
    if (pruning) {
        int n = d->nhist;

        d->hist[n] = (d->frame * (unsigned short)d->level) >> 3;
        d->hist2[n] = (d->frame * d->frame) >> 3;
        n++;
        if (n == 5)
            n = 0;
        d->nhist = n;
    }
}

/* the N best word sequences: the best backtraces (sorted, best first) made relative to the best,
 * each turn's best backtraced into a result and moved back past the ones now better (its trace
 * advanced to the next path), the results chained at +92; 1 when there is one. SOCOM 1's body
 * (the same 0x1C4): the count set before the scores are made relative (the target's early `li
 * s4,1`), the moved trace's score in a local (the shifting stores may alias it: reloaded each pass
 * otherwise), and the turns a `for` with its test alone -- a `while` there gets its exit test
 * copied to the top (8 bytes short), the target jumps to the test at the bottom */
int FindBestRecognised(NBESTDP *d)
{
    NBTRACE **bt;
    NBTRACE *top;
    NBRESULT *first, *r;
    unsigned int n, i, j, k;
    short best, score;
    int ret = 0;

    n = FindBestBacktraces(d, &bt);
    if (n != 0) {
        k = 1;
        best = bt[0]->score;
        for (i = 0; i < n; i++)
            bt[i]->score -= best;
        first = BacktraceWordsParBT(d, bt[0]);
        r = first;
        if (first != 0) {
            for (; k < d->nresults; ) {
                top = bt[0];
                score = top->score;
                for (j = 1; j < n && bt[j]->score < score; j++)
                    bt[j - 1] = bt[j];
                bt[j - 1] = top;
                k++;
                r->next = BacktraceWordsParBT(d, bt[0]);
                r = r->next;
                if (r == 0)
                    break;
            }
        }
        if (r != 0)
            r->next = 0;
        d->results = first;
        heap_Free(d->tos->heap, bt);
        if (first != 0)
            ret = 1;
    } else
        d->results = 0;
    return ret;
}

/* a record pushed after `top` (the first one when top is null) */
static void ExtendBacktraceStack(NBESTDP *d, BTSTACK *top)
{
    DPTOS *tos = d->tos;
    int **list = &tos->fa.free[3];
    BTSTACK *e;

    if (*list != 0) {
        e = (BTSTACK *)*list;
        *list = *(int **)e;
    } else
        e = (BTSTACK *)fastallo_AllocateMemoryFromChunk(&tos->fa, 3);
    if (top != 0) {
        top->next = e;
        e->prev = top;
    } else {
        d->stack = e;
        e->prev = 0;
    }
    e->next = 0;
}

/* the qsort comparator of the backtraces (pointers to them): ascending scores */
typedef struct BTSCORE {
    void *f0;
    short score;                        /* +4 */
} BTSCORE;

static int ScoreCompare(BTSCORE **a, BTSCORE **b)
{
    if ((*a)->score > (*b)->score)
        return 1;
    if ((*a)->score < (*b)->score)
        return -1;
    return 0;
}
