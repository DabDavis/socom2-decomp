/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing -funroll-loops */
/* nbestdp: the recogniser's N-best dynamic programming block (OVL2 0x4EB5E8-0x4F0CB8, SOCOM 1's
 * object at 0x358DF0: CollectDummyMemory, InitNBestDynProgProcess, InitDynprogContext,
 * CleanUpParBTProc, PrunDynProg, the two DynProgAccNBest passes, CopyParBT, BacktraceWordsParBT,
 * FindBestBacktraces, FindBestRecognised, ExpandGarbageArrays, NBestDynProgProcess,
 * ControlNBestDynProgProcess, ConstructNBestDynProg, ExtendBacktraceStack, ScoreCompare,
 * IdleProcess, all but the constructor and FindBestRecognised LOCAL there), ScanSoft's speech
 * recogniser, prebuilt with ee-gcc 2.96 at -O2 -G0 -fno-strict-aliasing -funroll-loops (README
 * "The GCC front"). Names are SOCOM 1's. Only the functions written so far are here, in image
 * order: FindBestRecognised (round 17c), ExtendBacktraceStack, ScoreCompare (NAME@nbestdp in the
 * map: the iso-word object has its own). The backtrace stack is a list of three-word records from the
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

/* a recognised word sequence, chained best first */
typedef struct NBRESULT {
    char pad0[16];
    struct NBRESULT *next;              /* +16 */
} NBRESULT;

/* a backtrace's head as the N-best ranks it */
typedef struct NBTRACE {
    int pad0;
    short score;                        /* +4 */
} NBTRACE;

typedef struct NBESTDP {
    DPTOS *tos;                         /* +0: the block's head (tosblk.h) */
    char pad4[84];
    BTSTACK *stack;                     /* +88: the stack's first record */
    NBRESULT *results;                  /* +92 */
    char pad96[88];
    unsigned int nresults;              /* +184: the sequences asked for */
} NBESTDP;

extern int *fastallo_AllocateMemoryFromChunk(FASTALLO *fa, unsigned short n);

extern unsigned int FindBestBacktraces(NBESTDP *d, NBTRACE ***bt);
extern NBRESULT *BacktraceWordsParBT(NBESTDP *d, NBTRACE *bt);
extern void heap_Free(void *heap, void *p);

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
