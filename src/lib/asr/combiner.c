/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing -funroll-loops */
/* combiner: the recogniser's frame combiner block (OVL2 0x4FCEA8-0x4FD4E8, SOCOM 1's object at
 * 0x368FC0: CombinerProcess, ConstructCombiner, CombinerInit, CombinerControl -- all but the
 * constructor LOCAL there), ScanSoft's speech recogniser, prebuilt with ee-gcc 2.96 at -O2 -G0
 * -fno-strict-aliasing -funroll-loops (README "The GCC front"). Names are SOCOM 1's (the
 * constructor's slots, s1:align); the block's fields are named by their use. A block of the
 * scheduler's (asr/tosblk.h): one input of 4n + 8 bytes, one output; eight rows of 3n + 3 floats
 * in one allocation. CombinerProcess (0x3FC) is not written: declared, its row open. */
#include "tosblk.h"

typedef struct COMBINER {
    TOSBLOCK b;
    unsigned int state;                 /* +40: 0, then 4 .. 7 as frames come */
    unsigned short f44;                 /* +44 */
    unsigned short nfeat;               /* +46: profile 2 */
    unsigned short n;                   /* +48: 3 nfeat + 3 */
    short pad50;
    float *row[8];                      /* +52 */
    float *buf;                         /* +84: the rows' allocation */
    unsigned char started;              /* +88 */
    char pad89[3];
} COMBINER;                             /* 0x5C bytes (ConstructCombiner) */

extern int qCheckInputQueues(QIN *in, int n);
extern void heap_Free(void *heap, void *p);

/* not written yet (its row open) */
int CombinerProcess(COMBINER *c, void **in);
static int CombinerInit(COMBINER *c);
static int CombinerControl(COMBINER *c, unsigned char cmd, int arg);

TOSBLOCK *ConstructCombiner(TOS *tos, unsigned char id)
{
    return tosBaseBlockConstruct(tos, id, 1, 1, (TOSFUNC)CombinerProcess, (TOSFUNC)CombinerInit,
                                 (TOSFUNC)CombinerControl, sizeof(COMBINER));
}

/* the sizes from profile 2 (12 features), the rows allocated; 2 when the allocation fails */
static int CombinerInit(COMBINER *c)
{
    TOS *tos = c->b.tos;
    float *p;
    int i;

    c->nfeat = _tosGetProfileU32(c, 2, 12);
    c->n = c->nfeat * 3 + 3;
    c->b.inputs[0].size = c->nfeat * 4 + 8;
    c->b.outputs[0].size = c->n * 4 - 4;
    c->buf = heap_Calloc(tos->heap, c->n * 8, 4);
    if (c->buf == 0)
        return 2;
    c->f44 = 0;
    p = c->buf;
    for (i = 0; i < 8; i++) {
        c->row[i] = p;
        p += c->n;
    }
    return 0;
}

/* the block's commands (case bodies in the target's order): 2 a frame announced (the first only
 * with its input queue ready), 1 a reset, 255 the block destroyed */
static int CombinerControl(COMBINER *c, unsigned char cmd, int arg)
{
    switch (cmd) {
    case 2:
        if (c->state == 0) {
            if (arg == 0)
                return 1;
            if (qCheckInputQueues(c->b.inputs, 1) == 0)
                return 1;
        }
        c->started = 1;
        if (c->state < 3)
            c->state += 4;
        else
            c->state = 7;
        break;
    case 1:
        c->state = 0;
        memset(c->buf, 0, c->n * 32);
        break;
    case TOS_DESTROY:
        heap_Free(c->b.tos->heap, c->buf);
        tosBaseBlockDestruct(&c->b);
        break;
    default:
        return 0;
    }
    return 1;
}
