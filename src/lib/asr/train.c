/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing -funroll-loops */
/* train: the recogniser's user-word model training (OVL2 0x4DF470-0x4E1B28, SOCOM 1's object at
 * 0x34E8E0: TrainSmoothCounts, TrainProbsFromCounts, TrainFree, TrainCountsFromAcq,
 * CalcEntropyMerged4bit), ScanSoft's speech recogniser, prebuilt with ee-gcc 2.96 at -O2 -G0
 * -fno-strict-aliasing -funroll-loops (README "The GCC front"). Names are SOCOM 1's.
 *
 * WHERE IT STARTS: the four rows between the casr object's end (0x4DF46C) and TrainSmoothCounts
 * (0x4DF470, 0x4DF898, 0x4DFD20, 0x4E0050) have no partner in SOCOM 1's casr object -- they are
 * this object's own first functions, SOCOM II's additions: three are called only by
 * TrainSmoothCounts (0x4E0D00) and the fourth only by DoUserWordTraining (the user-word object,
 * which also calls TrainSmoothCounts), and 0x4E0050 ends in a sibling call (`j heap_Free`, as
 * TrainFree does) where the casr object, built with -fno-optimize-sibling-calls, holds no `j`.
 * Only the functions written so far are here, in image order: TrainFree. */

#include "casr.h"

/* a training record: two buffers and the next record */
typedef struct TRAINREC {
    void *buf0;                         /* +0 */
    int f4;
    void *buf8;                         /* +8 */
    int f12;
    struct TRAINREC *next;              /* +16 */
} TRAINREC;

extern void heap_Free(void *heap, void *p);

void TrainFree(CASR *c)
{
    TRAINREC *r, *next;

    for (r = c->trainrecs; r != 0; r = next) {
        next = r->next;
        heap_Free(c->heap, r->buf0);
        heap_Free(c->heap, r->buf8);
        heap_Free(c->heap, r);
    }
    heap_Free(c->heap, c->train1060);
    heap_Free(c->heap, c->train1048);
}
