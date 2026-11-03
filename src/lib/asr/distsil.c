/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing -funroll-loops */
/* distsil: the recogniser's distance and silence block (OVL2 0x4FD4E8-, SOCOM 1's object at
 * 0x369590: ResizeQueues, LoadLanguage, DistCalc, DistCalc4Bit, FreeLanguage, FreeDistCalc,
 * InitDistSilProcess, NormCorrection, DistSilProcess, ControlDistSilProcess,
 * InitCalculateDistances, all LOCAL there), ScanSoft's speech recogniser, prebuilt with ee-gcc
 * 2.96 at -O2 -G0 -fno-strict-aliasing -funroll-loops (README "The GCC front"). Names are SOCOM
 * 1's (s1:align); the block's fields are named by their use. Only ResizeQueues is written (round
 * 15c): SOCOM 1's body instruction for instruction but the prologue's schedule. */
#include "tosblk.h"

/* a queue (queue.c), as far as the block reads it: the bytes of an element at +8 */
typedef struct QUEUE {
    char pad0[8];
    unsigned short elemsize;            /* +8 */
} QUEUE;

typedef struct DISTSIL {
    TOSBLOCK b;
    char pad40[60];
    QOUT *dist;                         /* +100: the distances' output */
} DISTSIL;

extern int qQueueReDimension(void *q, unsigned short size);
extern void qDeQueueOne(void *q, unsigned short reader);

/* the queues re-sized for n codewords: the distances' output to n shorts, the third input (when
 * the block has three) to n triples of bytes, emptied of its element; 1 when a re-sizing fails */
static int ResizeQueues(DISTSIL *d, int n)
{
    if (d->b.ninputs != 0 && d->dist->q != 0
        && ((QUEUE *)d->dist->q)->elemsize != n * 2) {
        d->dist->size = n * 2;
        if (qQueueReDimension(d->dist->q, n * 2) == 0)
            return 1;
    }
    if (d->b.ninputs == 3 && d->b.inputs[2].q != 0) {
        if (((QUEUE *)d->b.inputs[2].q)->elemsize != n * 3) {
            d->b.inputs[2].size = n * 3;
            if (qQueueReDimension(d->b.inputs[2].q, n * 3) == 0)
                return 1;
        }
        if (d->b.inputs[2].q != 0)
            qDeQueueOne(d->b.inputs[2].q, d->b.inputs[2].reader);
    }
    return 0;
}
