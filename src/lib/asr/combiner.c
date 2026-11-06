/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing -funroll-loops */
/* combiner: the recogniser's frame combiner block (OVL2 0x4FCEA8-0x4FD4E8, SOCOM 1's object at
 * 0x368FC0: CombinerProcess, ConstructCombiner, CombinerInit, CombinerControl -- all but the
 * constructor LOCAL there), ScanSoft's speech recogniser, prebuilt with ee-gcc 2.96 at -O2 -G0
 * -fno-strict-aliasing -funroll-loops (README "The GCC front"). Names are SOCOM 1's (the
 * constructor's slots, s1:align); the block's fields are named by their use. A block of the
 * scheduler's (asr/tosblk.h): one input of 4n + 8 bytes, one output; eight rows of 3n + 3 floats
 * in one allocation. CombinerProcess (0x3FC) written in round 33c, byte-exact in round 34c. */
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
 * loop's counter in place, unrolled by four). The first row's pointers walked a value at a time
 * (the target's `addiu a1,v0,4` and `addiu a0,a0,8`: indexing them is 2 words short).
 * Byte-exact in round 34c (127 words -> 0), four things at once: (1) the first value read into a
 * local BEFORE the state's increment and stored from it (`v = *x; c->state++; ... *cur = v;`):
 * the target's x[0] load sits above `sw t0,40(s0)`, which no alias analysis lets a float load
 * through a pointer cross without strict aliasing, and with the load first sched1 issues h & 7
 * first and every register falls into the target's (87 -> 3); (2) the output loops and the empty
 * one count in a variable of their own, j (the target's a1, apart from the feature loops' t3) --
 * which keeps the empty loop only together with (3) the ring's turn under an `if (src != 0)` of
 * its own after the send (the target re-tests s1 after the empty loop: src is stepped in the
 * loops, so the test survives, and jump threading sends the first test's null arm straight to
 * the epilogue); (4) `d -= nfeat` written before `p3 += nfeat` (sched1 issues the later of two
 * independent adds first) and the turn's sum narrowed to unsigned char -- the target's `lbu`
 * of the halfword ring index (`& 7` alone keeps `lhu`). */
static void CombinerProcess(COMBINER *c, float **in)
{
    float *x = in[0];
    float *src = 0;
    void *q = c->b.outputs[0].q;
    float *cur, *p1, *p2, *p3, *p4, *d, *out;
    unsigned short h;
    int k, j;
    float v;

    if (x != 0) {
        h = c->f44;
        cur = c->row[h & 7];
        p1 = c->row[(h + 7) & 7];
        p2 = c->row[(h + 6) & 7];
        p3 = c->row[(h + 5) & 7];
        p4 = c->row[(h + 4) & 7];
        v = *x;
        c->state++;
        src = p3 + 1;
        *cur = v;
        d = p2 + 1;
        *d = (2 * v + *p1 - *p3 - 2 * *p4) * 0.2f;
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
        d -= c->nfeat;
        p3 += c->nfeat;
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
                for (j = 0; j < c->n - 1; j++)
                    *out++ = *src++;
            } else if (c->state == 6) {
                *out++ = *src;
                src += 2;
                *out++ = 0;
                for (j = 0; j < c->nfeat * 2; j++)
                    *out++ = *src++;
                for (j = 0; j < c->nfeat; j++)
                    *out++ = 0;
            } else {
                *out++ = 0;
                *out++ = 0;
                src += 2;
                for (j = 0; j < c->nfeat; j++)
                    *out++ = *src++;
                for (j = 0; j < c->nfeat * 2; j++)
                    *out++ = 0;
            }
            for (j = 1; j < c->n; j++)
                ;
        }
    }
    if (src != 0)
        c->f44 = (unsigned char)(c->f44 + 1) & 7;
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
