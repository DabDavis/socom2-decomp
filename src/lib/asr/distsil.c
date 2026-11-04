/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing -funroll-loops -ffast-math */
/* distsil: the recogniser's distance and silence block (OVL2 0x4FD4E8-, SOCOM 1's object at
 * 0x369590: ResizeQueues, LoadLanguage, DistCalc, DistCalc4Bit, FreeLanguage, FreeDistCalc,
 * InitDistSilProcess, NormCorrection, DistSilProcess, ControlDistSilProcess,
 * InitCalculateDistances, all LOCAL there), ScanSoft's speech recogniser, prebuilt with ee-gcc
 * 2.96 at -O2 -G0 -fno-strict-aliasing -funroll-loops (README "The GCC front"). Names are SOCOM
 * 1's (s1:align, s1:order, s1:slot); the block's fields are named by their use. ResizeQueues
 * (round 15c): SOCOM 1's body instruction for instruction but the prologue's schedule; round 16c
 * the constructor and the three small functions after it (GetpCachedDistances, GetDistanceOffset,
 * IdleDistCalcProcess); round 21c FreeLanguage, FreeDistCalc (OPEN), NormCorrection -- and the
 * object's -ffast-math, which NormCorrection's sum shows (README "The recogniser and Medius by
 * hand", round 21c). */
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
    unsigned short n112;                /* +144: f112's codebooks */
    unsigned short n116;                /* +146: f116's */
    char pad148[20];
    int f168;                           /* +168 */
    int f172;                           /* +172 */
    void *f176;                         /* +176 */
    void *f180;                         /* +180 */
    int f184;                           /* +184 */
    char pad188[8];
    void *f196[10];                     /* +196 */
    char pad236[56];
    float *f292;                        /* +292: a float per codeword byte */
    float f296;                         /* +296 */
    float f300;                         /* +300 */
    char pad304[8];
    void **f312;                        /* +312: a table whose first word is a table too */
} DISTSIL;                              /* 316 bytes (ConstructDistCalc's) */

/* a codebook of the language's (12 bytes): its two tables */
typedef struct DSCODEBOOK {
    void *data;                         /* +0 */
    int f4;
    void *f8;                           /* +8 */
} DSCODEBOOK;

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

/* a list of codebooks given back: each one's two tables, then the list, its count set to `z`
 * (SOCOM 1's FreeLanguage holds this twice, the scheduler read again for the second). The count's
 * new value is an argument, 0 at both calls: integrate.c copies a constant argument into a pseudo,
 * which reload rematerialises (the target's `move v0,zero; sh v0`; SOCOM 1's compile spilled the
 * first expansion's to the stack and kept the second's in s8) -- `*n = 0` stores $zero */
static __inline__ void FreeCodeBooks(DISTSIL *d, DSCODEBOOK **list, unsigned short *n, int z)
{
    TOS *tos = d->b.tos;
    DSCODEBOOK *c = *list;
    int i;

    for (i = 0; i < *n; i++, c++)
        if (c->data != 0) {
            heap_Free(tos->heap, c->data);
            heap_Free(tos->heap, c->f8);
            c->data = 0;
        }
    if (*n != 0) {
        heap_Free(tos->heap, *list);
        *n = z;
    }
}

/* the language's tables given back: both codebook lists, the ten byte tables, the two caches and
 * the cached distances. SOCOM 1's body (0x248; 0x228 here). The tables' counter is unsigned (the
 * target's `sltiu`, counting up: an int counter is reversed by loop.c) */
static void FreeLanguage(DISTSIL *d)
{
    TOS *tos = d->b.tos;
    unsigned int i;

    FreeCodeBooks(d, (DSCODEBOOK **)&d->f112, &d->n112, 0);
    FreeCodeBooks(d, (DSCODEBOOK **)&d->f116, &d->n116, 0);
    for (i = 0; i < 10; i++)
        if (d->f196[i] != 0) {
            heap_Free(tos->heap, d->f196[i]);
            d->f196[i] = 0;
        }
    if (d->f176 != 0) {
        heap_Free(tos->heap, d->f176);
        d->f176 = 0;
        heap_Free(tos->heap, d->f180);
        d->f180 = 0;
    }
    d->f168 = 0;
    d->f172 = 0;
    d->f184 = 0;
    if (d->f108 != 0) {
        heap_Free(tos->heap, d->f108);
        d->f108 = 0;
    }
}

/* the block's memory given back and the block destroyed: the tables at +312 and +292, both codebook
 * lists, the ten byte tables, the caches and the cached distances (only the last cleared). SOCOM
 * 1's FreeDistCalc's place (0x25C there, 0x224 here), the tables' counter an int (reversed by
 * loop.c: the target's `li s2,9`, against FreeLanguage's unsigned one).
 * OPEN (round 21c, 31 words, the right size, registers only): d and each codebook expansion's
 * scheduler pointer trade s2/s3 -- a tie in global-alloc's priority, d 17 refs over 119 insns and
 * each expansion's pointer 6 over 21, both exactly 4/7 (build/scratch/gcc/prio.py), broken by pseudo
 * number in d's favour; the target's pointers rank first. The ten tables walked by a pointer, a
 * do-while, the caches' frees in one statement leave it; the +312 table through a local is 13 words
 * (build/scratch/gcc/v_fd1.py) */
static int FreeDistCalc(DISTSIL *d)
{
    TOS *tos = d->b.tos;
    int i;

    heap_Free(tos->heap, *d->f312);
    heap_Free(tos->heap, d->f312);
    heap_Free(tos->heap, d->f292);
    FreeCodeBooks(d, (DSCODEBOOK **)&d->f112, &d->n112, 0);
    FreeCodeBooks(d, (DSCODEBOOK **)&d->f116, &d->n116, 0);
    for (i = 0; i < 10; i++)
        if (d->f196[i] != 0)
            heap_Free(tos->heap, d->f196[i]);
    if (d->f176 != 0) {
        heap_Free(tos->heap, d->f176);
        heap_Free(tos->heap, d->f180);
    }
    if (d->f108 != 0) {
        heap_Free(tos->heap, d->f108);
        d->f108 = 0;
    }
    tosBaseBlockDestruct(&d->b);
    return 0;
}

/* (InitDistSilProcess, 0x4FF5A8, is not written) */
extern float logf(float x);

/* a frame's codeword bytes corrected for the frame's level: the log of the bytes' table sum,
 * scaled to the byte range over the table's span (+296 - +300) and rounded, added to every byte,
 * clamped to 0..255. SOCOM 1's body (0x180; SOCOM II calls logf where SOCOM 1 calls log; 0x224
 * unrolled). Measured: the object is built with -ffast-math (the sum's first unrolled copy loads
 * straight into it, `0 + x` folded -- a fifth such object; its other functions compile the same),
 * the rounding compares in double (`x < 0.0`: the target's dpcmp on x converted once), the clamp's
 * upper bound first (both bounds if-converted into the target's movn/movz pair; the lower first
 * keeps a branch) */
static void NormCorrection(DISTSIL *d, QIN *q, unsigned char *b)
{
    float sum = 0;
    float x;
    int i, shift, v;

    for (i = 0; i < q->reader; i++)
        sum += d->f292[b[i]];
    x = logf(sum) * 255.0 / (d->f296 - d->f300);
    shift = (int)(x < 0.0 ? x - 0.5 : x + 0.5);
    for (i = 0; i < q->reader; i++) {
        v = b[i] + shift;
        if (v > 255)
            v = 255;
        else if (v < 0)
            v = 0;
        b[i] = v;
    }
}

/* (DistSilProcess .. InitCalculateDistances, -0x500DF8, are not written but
 * ControlDistSilProcess's prototype) */
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
