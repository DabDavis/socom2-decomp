/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing -funroll-loops */
/* nbestdp: the recogniser's N-best dynamic programming block (OVL2 0x4EB5E8-0x4F0CB8, SOCOM 1's
 * object at 0x358DF0: CollectDummyMemory, InitNBestDynProgProcess, InitDynprogContext,
 * CleanUpParBTProc, PrunDynProg, the two DynProgAccNBest passes, CopyParBT, BacktraceWordsParBT,
 * FindBestBacktraces, FindBestRecognised, ExpandGarbageArrays, NBestDynProgProcess,
 * ControlNBestDynProgProcess, ConstructNBestDynProg, ExtendBacktraceStack, ScoreCompare,
 * IdleProcess, all but the constructor and FindBestRecognised LOCAL there), ScanSoft's speech
 * recogniser, prebuilt with ee-gcc 2.96 at -O2 -G0 -fno-strict-aliasing -funroll-loops (README
 * "The GCC front"). Names are SOCOM 1's. Only the functions written so far are here, in image
 * order: ExtendBacktraceStack. The backtrace stack is a list of three-word records from the
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
    char pad0[84];
    FASTALLO fa;                        /* +84 */
} DPTOS;

/* a backtrace stack record (three words) */
typedef struct BTSTACK {
    struct BTSTACK *prev;               /* +0 */
    struct BTSTACK *next;               /* +4 */
    int entry;                          /* +8 */
} BTSTACK;

typedef struct NBESTDP {
    DPTOS *tos;                         /* +0: the block's head (tosblk.h) */
    char pad4[84];
    BTSTACK *stack;                     /* +88: the stack's first record */
} NBESTDP;

extern int *fastallo_AllocateMemoryFromChunk(FASTALLO *fa, unsigned short n);

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
