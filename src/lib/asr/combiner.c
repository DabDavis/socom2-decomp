/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing -funroll-loops */
/* combiner: the recogniser's frame combiner block (OVL2 0x4FCEA8-0x4FD4E8, SOCOM 1's object at
 * 0x368FC0: CombinerProcess, ConstructCombiner, CombinerInit, CombinerControl -- all but the
 * constructor LOCAL there), ScanSoft's speech recogniser, prebuilt with ee-gcc 2.96 at -O2 -G0
 * -fno-strict-aliasing -funroll-loops (README "The GCC front"). Names are SOCOM 1's (the
 * constructor's slots, s1:align); the block's fields are named by their use. A block of the
 * scheduler's (asr/tosblk.h): one input of 4n + 8 bytes, one output; eight rows of 3n + 3 floats
 * in one allocation. CombinerProcess (0x3FC) written in round 33c, OPEN (registers only). */
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

static void CombinerProcess(COMBINER *c, float **in);
static int CombinerInit(COMBINER *c);
static int CombinerControl(COMBINER *c, unsigned char cmd, int arg);

extern void *qEnQueueOne(void *q);

/* a frame (the block's process): the frame (its first value and nfeat features after a skipped
 * word) kept in the ring's current row, the deltas of the row two frames back from the rows round
 * it -- (2 x[t] + x[t-1] - x[t-3] - 2 x[t-4]), the first value's scaled 0.2, the features' 0.375
 * -- and the accelerations of the row before (delta[t-2] - delta[t-4]); while flushing (+88) the
 * frames still held counted down. With a row ready (state 4 on), the row three frames back sent:
 * whole from state 7, from state 6 with its accelerations zeroed, at state 4 or 5 with its deltas
 * zeroed too; the ring then turned. SOCOM 1's LOCAL, written from SOCOM II's code (round 33c):
 * SOCOM II adds an empty loop over the row's values after the send (kept: 2.96 leaves an empty
 * loop's counter in place, unrolled by four). The first value read once (its delta from the row
 * it was just stored in) and the first row's pointers walked a value at a time (the target's
 * `addiu a1,v0,4` and `addiu a0,a0,8`: indexing them is 2 words short). OPEN (round 33c, the
 * right size, 127 words): registers only -- the target allocates p3, d, p4, p1 to a0-a3 and x, cur,
 * k, q to t1-t4, here d, p4, p3, p1 a0, a1, a3, t3 and x t4; the head's index sums are scheduled
 * h & 7 first there. Five declaration orders give the same 127. */
static void CombinerProcess(COMBINER *c, float **in)
{
    float *x = in[0];
    float *src = 0;
    void *q = c->b.outputs[0].q;
    float *cur, *p1, *p2, *p3, *p4, *d, *out;
    unsigned short h;
    int k;

    if (x != 0) {
        h = c->f44;
        cur = c->row[h & 7];
        p1 = c->row[(h + 7) & 7];
        p2 = c->row[(h + 6) & 7];
        p3 = c->row[(h + 5) & 7];
        p4 = c->row[(h + 4) & 7];
        c->state++;
        src = p3 + 1;
        *cur = *x;
        d = p2 + 1;
        *d = (2 * *cur + *p1 - *p3 - 2 * *p4) * 0.2f;
        p3 += 2;
        p4++;
        *p3 = *d - *p4;
        x += 2;
        cur += 3;
        p1 += 3;
        p3++;
        p4 += 2;
        d += c->nfeat + 2;
        for (k = 0; k < c->nfeat; k++) {
            *cur++ = *x;
            *d++ = (2 * *x++ + *p1++ - *p3++ - 2 * *p4++) * 0.375f;
        }
        p3 += c->nfeat;
        d -= c->nfeat;
        for (k = 0; k < c->nfeat; k++)
            *p3++ = *d++ - *p4++;
    }
    if (c->started) {
        if (--c->state == 4)
            c->started = 0;
        src = c->row[(c->f44 + 5) & 7] + 1;
    }
    if (src != 0) {
        if (c->state >= 4) {
            out = qEnQueueOne(q);
            if (c->state >= 7) {
                for (k = 0; k < c->n - 1; k++)
                    *out++ = *src++;
            } else if (c->state == 6) {
                *out++ = *src;
                src += 2;
                *out++ = 0;
                for (k = 0; k < c->nfeat * 2; k++)
                    *out++ = *src++;
                for (k = 0; k < c->nfeat; k++)
                    *out++ = 0;
            } else {
                *out++ = 0;
                *out++ = 0;
                src += 2;
                for (k = 0; k < c->nfeat; k++)
                    *out++ = *src++;
                for (k = 0; k < c->nfeat * 2; k++)
                    *out++ = 0;
            }
            for (k = 1; k < c->n; k++)
                ;
        }
        c->f44 = (c->f44 + 1) & 7;
    }
}

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
