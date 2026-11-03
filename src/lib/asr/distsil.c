/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing -funroll-loops */
/* distsil: the recogniser's distance and silence block (OVL2 0x4FD4E8-, SOCOM 1's object at
 * 0x369590: ResizeQueues, LoadLanguage, DistCalc, DistCalc4Bit, FreeLanguage, FreeDistCalc,
 * InitDistSilProcess, NormCorrection, DistSilProcess, ControlDistSilProcess,
 * InitCalculateDistances, all LOCAL there), ScanSoft's speech recogniser, prebuilt with ee-gcc
 * 2.96 at -O2 -G0 -fno-strict-aliasing -funroll-loops (README "The GCC front"). Names are SOCOM
 * 1's (s1:align, s1:order, s1:slot); the block's fields are named by their use. ResizeQueues
 * (round 15c): SOCOM 1's body instruction for instruction but the prologue's schedule; round 16c
 * the constructor and the three small functions after it (GetpCachedDistances, GetDistanceOffset,
 * IdleDistCalcProcess). */
#include "tosblk.h"

/* a queue (queue.c), as far as the block reads it: the bytes of an element at +8 */
typedef struct QUEUE {
    char pad0[8];
    unsigned short elemsize;            /* +8 */
} QUEUE;

typedef struct DISTSIL {
    TOSBLOCK b;
    char pad40[8];
    unsigned short ntables;             /* +48: the codebooks' byte tables in use */
    short pad50;
    unsigned char *tables[10];          /* +52 */
    unsigned short *index;              /* +92: ntables codewords per frame */
    int f96;                            /* +96: the distances' offset (GetDistanceOffset) */
    QOUT *dist;                         /* +100: the distances' output */
    int pad104;
    int *f108;                          /* +108: the cached distances (GetpCachedDistances) */
    void *f112;                         /* +112: cleared at construction */
    void *f116;                         /* +116 */
    char pad120[16];
    void *f136;                         /* +136 */
    void *f140;                         /* +140 */
    char pad144[172];
} DISTSIL;                              /* 316 bytes (ConstructDistCalc's) */

/* the profile's reader, as a block: the scheduler and the block's id */
typedef struct TOSLOG {
    TOS *tos;
    unsigned char id;
} TOSLOG;

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

/* (FreeLanguage .. InitCalculateDistances, 0x4FF158-0x500DF8, are not written) */
extern int InitDistSilProcess(DISTSIL *d);
extern int ControlDistSilProcess(DISTSIL *d, unsigned char cmd, void *arg, void *arg2);
static void IdleDistCalcProcess(DISTSIL *d, void **in);

/* the block: no queues when profile 35 is set (the distances come from elsewhere), else one output
 * and two inputs, three with profile 34's cache -- each case setting both counts, the output's
 * first (a single-assignment else if-converts the 3 or 2 into a movz; the other orders put the
 * output's 1 or the input's 3 in the wrong one of the branch's slot and the word before it) */
TOSBLOCK *ConstructDistCalc(TOS *tos, unsigned char id)
{
    TOSLOG log;
    unsigned char nin, nout;
    DISTSIL *d;

    log.tos = tos;
    log.id = id;
    if ((unsigned char)_tosGetProfileU32(&log, 35, 0)) {
        nin = 0;
        nout = 0;
    } else if ((unsigned char)_tosGetProfileU32(&log, 34, 1)) {
        nout = 1;
        nin = 3;
    } else {
        nout = 1;
        nin = 2;
    }
    d = (DISTSIL *)tosBaseBlockConstruct(tos, id, nin, nout, (TOSFUNC)IdleDistCalcProcess,
                                         (TOSFUNC)InitDistSilProcess, (TOSFUNC)ControlDistSilProcess,
                                         sizeof(DISTSIL));
    if (d != 0) {
        d->f116 = 0;
        d->f112 = 0;
        d->f136 = 0;
        d->f140 = 0;
    }
    return &d->b;
}

static int *GetpCachedDistances(DISTSIL *d)
{
    return d->f108;
}

static int GetDistanceOffset(DISTSIL *d)
{
    return d->f96;
}

/* frame n's distance: each table's byte at the frame's codeword, summed, less the offset */
static short CalculateDistance(DISTSIL *d, int n)
{
    unsigned char **t = d->tables;
    unsigned char **end = t + d->ntables;
    unsigned short *idx = d->index + n * d->ntables;
    int sum = 0;

    do
        sum += (*t++)[*idx++];
    while (t < end);
    return sum - d->f96;
}

/* frame n's distance from 4-bit tables (two codewords a byte, the even one in the high nibble):
 * each nibble, kept in the high four bits, summed, less the offset */
static short CalculateDistance4Bit(DISTSIL *d, int n)
{
    unsigned char **t = d->tables;
    unsigned char **end = t + d->ntables;
    unsigned short *idx = d->index + n * d->ntables;
    int sum = 0;
    unsigned short c;

    do {
        c = *idx++;
        sum += ((*t++)[c >> 1] << ((c & 1) << 2)) & 0xF0;
    } while (t < end);
    return sum - d->f96;
}

static void IdleDistCalcProcess(DISTSIL *d, void **in)
{
}
