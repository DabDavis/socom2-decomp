/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing -funroll-loops -ffast-math */
/* distsil: the recogniser's distance and silence block (OVL2 0x4FD4E8-, SOCOM 1's object at
 * 0x369590: ResizeQueues, LoadLanguage, DistCalc, DistCalc4Bit, FreeLanguage, FreeDistCalc,
 * InitDistSilProcess, NormCorrection, DistSilProcess, ControlDistSilProcess,
 * InitCalculateDistances, all LOCAL there), ScanSoft's speech recogniser, prebuilt with ee-gcc
 * 2.96 at -O2 -G0 -fno-strict-aliasing -funroll-loops (README "The GCC front"). Names are SOCOM
 * 1's (s1:align, s1:order, s1:slot); the block's fields are named by their use. ResizeQueues
 * (round 15c): SOCOM 1's body instruction for instruction but the prologue's schedule; round 16c
 * the constructor and the three small functions after it (GetpCachedDistances, GetDistanceOffset,
 * IdleDistCalcProcess); round 21c FreeLanguage, FreeDistCalc, NormCorrection -- and the
 * object's -ffast-math, which NormCorrection's sum shows (README "The recogniser and Medius by
 * hand", round 21c); round 22c FreeDistCalc byte-exact over a marked stand-in insn; round 34c
 * DistSilProcess (0x4FFCE8, named by SOCOM 1's object order), func_004FD5C8 and LoadLanguage, with
 * the fields they read; InitDistSilProcess written (OPEN, 3 words), byte-exact in round 35c (an
 * intermediate in v: the constant's register by expand_preferences). */
#include "tosblk.h"

/* a queue (queue.c), as far as the block reads it: the bytes of an element at +8 */
typedef struct QUEUE {
    char pad0[8];
    unsigned short elemsize;            /* +8 */
} QUEUE;

/* a codebook of the language's (12 bytes): its codeword table, the codewords' count, and a byte per
 * codeword counting the frames that picked it */
typedef struct DSCODEBOOK {
    unsigned char **data;               /* +0: a byte table per codeword */
    unsigned short n;                   /* +4 */
    unsigned short m;                   /* +6: the bytes a codeword's table takes */
    unsigned char *f8;                  /* +8: n counts */
} DSCODEBOOK;

/* the language's frames (+128, pointed to by +140): a row of codewords per frame */
typedef struct DSFRAMES {
    unsigned short *data;
    unsigned short n;
    unsigned short m;
} DSFRAMES;

typedef struct DISTSIL {
    TOSBLOCK b;
    short *f40;                         /* +40: the third input's frame */
    short *f44;                         /* +44: f158 shorts into it */
    unsigned short ntables;             /* +48: the codebooks' byte tables in use */
    short pad50;
    unsigned char *tables[10];          /* +52 */
    unsigned short *index;              /* +92: ntables codewords per frame */
    int f96;                            /* +96: the distances' offset (GetDistanceOffset) */
    QOUT *dist;                         /* +100: the distances' output */
    void (*f104)(struct DISTSIL *d, void *out, int n, unsigned short sum); /* +104 */
    int *f108;                          /* +108: the cached distances (GetpCachedDistances) */
    void *f112;                         /* +112: cleared at construction */
    void *f116;                         /* +116 */
    char pad120[8];
    DSFRAMES frames;                    /* +128: ntables codewords per frame */
    DSCODEBOOK *f136;                   /* +136: the codebooks in use */
    DSFRAMES *f140;                     /* +140: &frames */
    unsigned short n112;                /* +144: f112's codebooks */
    unsigned short n116;                /* +146: f116's */
    unsigned short ncb;                 /* +148: f136's */
    unsigned short f150;                /* +150 */
    short f152;                         /* +152: profile 30, added to a value with bit 1 */
    short f154;                         /* +154: profile 31, added to a value with bit 0 */
    unsigned short f156;                /* +156 */
    unsigned short f158;                /* +158 */
    unsigned short f160;                /* +160 */
    unsigned short f162;                /* +162 */
    short f164;                         /* +164: -f160 */
    short pad166;
    void *f168;                         /* +168 */
    unsigned short *f172;               /* +172: f160 + 1 offsets into f184 */
    short *f176;                        /* +176: f156 shorts */
    short *f180;                        /* +180: f184's values decoded */
    short *f184;                        /* +184 */
    char pad188[8];
    unsigned char *f196[10];            /* +196: a codeword byte per codebook */
    unsigned short f236;                /* +236: the frame f128 is read at */
    char pad238[42];
    unsigned int f280;                  /* +280: silent frames, modulo f308 */
    unsigned int f284;                  /* +284: f308-frame periods, up to 20 */
    unsigned char f288;                 /* +288 */
    unsigned char f289;                 /* +289: the first input read */
    unsigned char f290;                 /* +290 */
    char pad291;
    float *f292;                        /* +292: a float per codeword byte */
    float f296;                         /* +296 */
    float f300;                         /* +300 */
    float f304;                         /* +304 */
    unsigned int f308;                  /* +308: the period */
    unsigned char **f312;               /* +312: a byte table per count */
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

extern void func_004FE6C8();

/* each codebook's bytes at the frame's codeword (f128's row f236) copied into its row at +196; with
 * the 4-bit distances (+104 the 4-bit pass, 0x4FE6C8) two codewords share a byte, the odd one in
 * the low nibble, each nibble scaled by 17 to the byte range. No SOCOM 1 counterpart (its object
 * goes from ResizeQueues to LoadLanguage); unnamed. LoadLanguage (+0x56C) and ControlDistSilProcess
 * (+0x3C0) call it. Round 34c, byte-exact as first written. */
static void func_004FD5C8(DISTSIL *d)
{
    DSCODEBOOK *cb;
    unsigned char **data;
    unsigned short c, n;
    unsigned int k, j;
    int mask, shift, mult;

    for (k = 0; k < d->ntables; k++) {
        mask = 255;
        shift = 0;
        mult = 1;
        c = d->frames.data[d->f236 * d->ntables + k];
        cb = d->f116;
        data = cb[k].data;
        if (d->f104 == (void *)func_004FE6C8) {
            if (c & 1)
                mask = 15;
            else
                shift = 4;
            mult = 17;
            c >>= 1;
        }
        n = cb[k].n;
        for (j = 0; j < n; j++)
            d->f196[k][j] = ((data[j][c] & mask) >> shift) * mult;
    }
}

/* the language's interface (the engine's language object), as far as LoadLanguage calls it */
typedef struct DSLANG {
    char pad0[16];
    int (*f16)(struct DSLANG *l);                   /* +16: the codebooks */
    int (*f20)(struct DSLANG *l);                   /* +20 */
    int (*f24)(struct DSLANG *l);                   /* +24 */
    int pad28;
    int (*f32)(struct DSLANG *l);                   /* +32 */
    int (*f36)(struct DSLANG *l);                   /* +36 */
    int pad40;
    int (*f44)(struct DSLANG *l);                   /* +44 */
    int pad48;
    int (*f52)(struct DSLANG *l);                   /* +52 */
    char pad56[16];
    int (*f72)(struct DSLANG *l);                   /* +72 */
    char pad76[48];
    int (*f124)(void *cb);                          /* +124: a codebook's codewords */
    int pad128;
    unsigned int (*f132)(void *cb);                 /* +132: a codeword's bytes */
    int pad136;
    unsigned short *(*f140)(struct DSLANG *l);      /* +140 */
    char pad144[8];
    void *(*f152)(struct DSLANG *l);                /* +152 */
    unsigned short *(*f156)(struct DSLANG *l);      /* +156 */
    short *(*f160)(struct DSLANG *l);               /* +160 */
    void *(*f164)(struct DSLANG *l, int k);         /* +164: codebook k */
    char pad168[40];
    int (*f208)(struct DSLANG *l, int what);        /* +208: 1, the 4-bit tables */
    char pad212[32];
    unsigned char *(*f244)(void *cb);               /* +244: a codebook's tables */
} DSLANG;

extern void func_004FDE08();

/* codebook cb's tables at tab, n codewords of m bytes; 2 logged when its allocations failed */
static __inline__ unsigned char SetCodeBook(DISTSIL *d, DSCODEBOOK *cb, unsigned char *tab,
                                            unsigned short n, unsigned short m)
{
    TOS *tos = d->b.tos;
    int i;

    if (cb->data != 0) {
        heap_Free(tos->heap, cb->data);
        heap_Free(tos->heap, cb->f8);
    }
    cb->n = n;
    cb->m = m;
    cb->data = heap_Alloc(tos->heap, n * 4);
    cb->f8 = heap_Calloc(tos->heap, n, 1);
    if (cb->data == 0 || cb->f8 == 0)
        return _tosErrorLog(d, 2);
    for (i = 0; i < n; i++) {
        cb->data[i] = tab;
        tab += m;
    }
    return 0;
}

/* the codebook list made n long, every codebook's tables given back first; the list returned */
static __inline__ DSCODEBOOK *SizeCodeBooks(DISTSIL *d, DSCODEBOOK **list, unsigned short *cnt,
                                            unsigned short n)
{
    TOS *tos = d->b.tos;
    DSCODEBOOK *c = *list;
    int i;

    for (i = 0; i < *cnt; i++, c++)
        if (c->data != 0) {
            heap_Free(tos->heap, c->data);
            heap_Free(tos->heap, c->f8);
            c->data = 0;
        }
    if (*cnt != n) {
        if (*cnt != 0)
            heap_Free(tos->heap, *list);
        if (n != 0)
            *list = heap_Calloc(tos->heap, n, 12);
        *cnt = n;
    }
    return *list;
}

/* the language loaded into the block (s1:align, SOCOM 1's LOCAL LoadLanguage, 0x65C): its sizes
 * read through its interface, the packed values at +184 decoded (each value >> 2, plus +154 with
 * bit 0 and +152 with bit 1), the frames' table at +128, the codebook list sized, each codebook's
 * codeword tables (two codewords a byte with the 4-bit tables) and its row at +196 allocated, the
 * queues re-sized, the distance pass chosen (0x4FE6C8 for the 4-bit tables, else 0x4FDE08), the
 * frame's bytes copied, the index set at frame -f160, the cache at +108 allocated (2 logged when
 * it fails). Written from SOCOM II's code in round 34c, byte-exact: each count a variable of its
 * own (one `n` for the frames', the list's and a codebook's counts is one pseudo across thirteen
 * calls, and it takes the interface's s4); the list's size an inline returning the list (the null
 * test reads through its pointer, s7); a codebook's tables an inline returning the error log's
 * byte; the codeword size computed into an unsigned int and narrowed at the inline's call (the
 * target's single `andi` after the join); the frames' fields stored through a pointer after all
 * three calls; the f44 value in a variable of its own; the decode's accumulator an int (a short one
 * re-extends each sum, 0x614; the value read as an int too, 0x71C); the ncb/f136 stores
 * written f136 first (sched1). */
static int LoadLanguage(DISTSIL *d, DSLANG *l)
{
    int is4bit;
    TOS *tos = d->b.tos;
    short *s, *t, *end;
    short v;
    int x, w;
    unsigned short nf, mf, ncb, n;
    DSFRAMES *f;
    unsigned short *data;
    unsigned int mm;
    unsigned int k;
    void *cbl;
    unsigned char *tab;

    is4bit = l->f208(l, 1);
    d->index = 0;
    d->f236 = l->f32(l);
    d->f156 = l->f36(l);
    d->f158 = l->f24(l);
    d->f162 = l->f52(l);
    w = l->f44(l);
    d->f160 = w;
    d->f164 = -w;
    d->f168 = l->f152(l);
    d->f172 = l->f156(l);
    d->f184 = l->f160(l);
    if (d->f176 != 0) {
        heap_Free(tos->heap, d->f176);
        heap_Free(tos->heap, d->f180);
    }
    d->f176 = heap_Alloc(tos->heap, d->f156 * 2);
    d->f180 = heap_Alloc(tos->heap, (d->f172[d->f160] - d->f172[0]) * 2);
    t = d->f180;
    s = d->f184;
    end = s + d->f172[d->f160] - d->f172[0];
    for (; s < end; s++) {
        v = *s;
        x = v >> 2;
        if (v & 1)
            x += d->f154;
        if (v & 2)
            x += d->f152;
        *t++ = x;
    }
    d->f150 = l->f72(l);
    nf = l->f20(l);
    mf = l->f16(l);
    data = l->f140(l);
    f = &d->frames;
    f->n = nf;
    f->m = mf;
    f->data = data;
    d->f140 = f;
    d->ntables = ncb = l->f16(l);
    if (SizeCodeBooks(d, (DSCODEBOOK **)&d->f116, &d->n116, ncb) == 0)
        return 1;
    for (k = 0; k < d->ntables; k++) {
        cbl = l->f164(l, k);
        n = l->f124(cbl);
        if (is4bit)
            mm = (l->f132(cbl) + 1) >> 1;
        else
            mm = l->f132(cbl);
        tab = l->f244(cbl);
        if (SetCodeBook(d, (DSCODEBOOK *)d->f116 + k, tab, n, mm))
            return 1;
        if (d->f196[k] != 0)
            heap_Free(tos->heap, d->f196[k]);
        d->f196[k] = heap_Alloc(tos->heap, l->f124(cbl));
    }
    d->f136 = d->f116;
    d->ncb = d->n116;
    if (ResizeQueues(d, l->f24(l)) != 0)
        return 1;
    if (is4bit)
        d->f104 = (void *)func_004FE6C8;
    else
        d->f104 = (void *)func_004FDE08;
    func_004FD5C8(d);
    d->index = d->f140->data + d->f164 * d->ntables;
    if (d->f108 != 0)
        heap_Free(tos->heap, d->f108);
    d->f108 = heap_Alloc(tos->heap, d->f158 * 2);
    if (d->f108 == 0)
        return _tosErrorLog(d, 2);
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
 * Round 21c's 31 words (registers only) were a tie in global-alloc's priority: d 17 refs over 119
 * insns and each codebook expansion's scheduler pointer 6 over 21, both 5714 (`tools/ccdump.py -f`),
 * and a tie goes to the lower pseudo -- d, the parameter, always numbered first, so no renumbering
 * can give the target's order (the pointers s2, d s3). The target's d lives 120 insns or more (5666):
 * one RTL insn more inside d's life and outside both expansions' -- after the second, or anywhere
 * past the codebook frees -- gives the target byte for byte (round 22c). FREEDIST_STANDIN is that
 * insn, a STAND-IN: an empty asm emits nothing and counts in the live length. What the vendor had
 * there is not known: a result variable, a split `TOSBLOCK *` parameter, a duplicated tail for
 * cross-jumping, a guard, a goto, a label, a do-while(0), the inline's parameters in any order or
 * as `const`, its tos passed in or loaded later (19: then it outranks the block's own tos) -- each
 * leaves the tie (build/scratch/gcc/fd/v1-v6.py) */
#define FREEDIST_STANDIN() __asm__ __volatile__ ("")
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
    FREEDIST_STANDIN();
    tosBaseBlockDestruct(&d->b);
    return 0;
}

extern float logf(float x);
extern float expf(float x);

/* the block's init (s1:align: SOCOM 1's LOCAL InitDistSilProcess, 0x524, the same body but its exp):
 * the queues sized from profiles 32 and 33 (the output's n33 shorts, the third input's n33 triples);
 * the period (19, as an unsigned short); the level scale from 0 down to -9.22306 in 255 steps, its
 * linear values (expf) at +292; per count up to the period, a byte per level -- the level after
 * adding count x profile 18's step to its linear value, rescaled to the 255 steps below the top of
 * the scale and rounded, clamped to 0..255 -- the rows in one allocation (1 when it fails); the
 * counters cleared, profile 20 at +288. Written in round 34c from SOCOM II's code; byte-exact in
 * round 35c: v holds the log less lmax first, and `v - d->f296` -- fptodp's argument, set straight
 * into $f12 with v its first operand -- gives v's pseudo a $f12 preference, which global.c's
 * expand_preferences hands the float-to-unsigned conversion's 2^31 constant where both die into
 * `v - 2^31`: the constant's $f12 (one expression, or a separate temporary, gives $f3 -- the first
 * call-clobbered register no neighbour holds). */
static int InitDistSilProcess(DISTSIL *d)
{
    TOS *tos = d->b.tos;
    unsigned short n32, n33;
    unsigned int i, j;
    unsigned char *p;
    float lmax, x, v;

    d->f290 = 1;
    n32 = _tosGetProfileU32(d, 32, 4);
    n33 = _tosGetProfileU32(d, 33, 1);
    d->f150 = 0xFFFF;
    d->f152 = _tosGetProfileU32(d, 30, 0);
    d->f154 = _tosGetProfileU32(d, 31, 0);
    if (d->b.ninputs != 0) {
        d->dist = d->b.outputs;
        d->b.inputs[1].size = n32;
        d->b.inputs[0].size = 1;
        d->dist->size = n33 * 2;
        if (d->b.ninputs == 3)
            d->b.inputs[2].size = n33 * 3;
    }
    d->f308 = (unsigned short)_tosGetProfileU32(d, 19, 4);
    d->f296 = 0;
    d->f300 = -9.22306f;
    d->f292 = heap_Alloc(tos->heap, 1024);
    for (i = 0; i < 256; i++)
        d->f292[i] = expf(d->f296 - i * (d->f296 - d->f300) * (1.0 / 255));
    d->f304 = _tosGetProfileFloat(d, 18, 0.002f);
    d->f312 = heap_Alloc(tos->heap, d->f308 * 4 + 4);
    p = heap_Alloc(tos->heap, d->f308 * 256 + 256);
    if (p == 0)
        return 1;
    for (i = 0; i <= d->f308; i++) {
        d->f312[i] = p;
        p += 256;
    }
    lmax = logf(d->f308 * d->f304 + 1.0);
    for (j = 0; j <= d->f308; j++) {
        x = j * d->f304;
        for (i = 0; i < 256; i++) {
            v = logf(expf(d->f296 - (d->f296 - d->f300) * i * (1.0 / 255)) + x) - lmax;
            v = (v - d->f296) * -255.0 / (d->f296 - d->f300) + 0.5;
            if (v < 0)
                v = 0;
            else if (v > 255)
                v = 255;
            d->f312[j][i] = v;
        }
    }
    d->f280 = 0;
    d->f284 = 0;
    d->f288 = _tosGetProfileU32(d, 20, 0);
    d->f289 = 1;
    d->f108 = 0;
    return 0;
}

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

extern void *qEnQueueOne(void *q);

/* a frame (the block's process): the codewords (the second input, a byte per codebook) and the
 * third input's frame kept; with the first input read, a silent frame (its flag clear) counted
 * unless +288 is set, every f308th starting a period; per codebook the frame's table taken and,
 * at a period's start, its codeword counts folded into the codeword bytes and cleared (each byte
 * renormalised once 20 periods have passed); the bytes at the frame's codewords summed and,
 * with the third input present, sent through +104 with the output's element. SOCOM 1's LOCAL
 * (0x280), written from SOCOM II's code (round 34c), byte-exact: the third input read once into a
 * local (two reads through `in` reload it after the first store: no strict aliasing); the flag
 * loaded into sil before `1 - sil` (the target's `lbu s6,0(v1)`: sil's own register); the table of
 * tables read into a local before the fold's loop (SOCOM 1's too). */
static void DistSilProcess(DISTSIL *d, unsigned char **in)
{
    unsigned char *cw;
    unsigned char sil;
    unsigned char norm;
    DSCODEBOOK *cb;
    unsigned char *b;
    unsigned char **tbl;
    unsigned int sum;
    short *f;
    int i, j;

    if (in[1] == 0)
        return;
    cw = in[1];
    sil = 0;
    f = (short *)in[2];
    d->f40 = f;
    d->f44 = f + d->f158;
    if (d->f289) {
        sil = *in[0];
        sil = 1 - sil;
    }
    if (d->f288)
        sil = 0;
    norm = 0;
    if (sil) {
        if (++d->f280 % d->f308 == 0) {
            norm = 1;
            d->f280 = 0;
            d->f284++;
        }
    }
    sum = 0;
    cb = d->f136;
    for (i = 0; i < d->ncb; i++) {
        b = d->f196[i];
        if (sil)
            cb->f8[*cw]++;
        d->tables[i] = cb->data[*cw];
        if (norm) {
            tbl = d->f312;
            for (j = 0; j < cb->n; j++) {
                b[j] = tbl[cb->f8[j]][b[j]];
                cb->f8[j] = 0;
            }
            if (d->f284 >= 20)
                NormCorrection(d, (QIN *)cb, b);
        }
        sum += b[*cw];
        cw++;
        cb++;
    }
    if (d->f40 != 0)
        d->f104(d, qEnQueueOne(d->dist->q), i, sum);
    if (d->f284 >= 20)
        d->f284 = 0;
}

/* (ControlDistSilProcess .. InitCalculateDistances, -0x500DF8, are not written but
 * ControlDistSilProcess's prototype) */
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
