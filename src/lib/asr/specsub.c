/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing -funroll-loops -ffast-math */
/* specsub: the recogniser's spectral subtraction block (OVL2 0x4F8130-0x4F9DC0, SOCOM 1's object at
 * 0x364AC8: SpecSubPutSession, SpecSubGetSession, SpectralSubtraction, FreeResourcesSpecSub,
 * ControlSpecSub, InitSpecSub, ConstructSpecSub, ProcessSpecSub -- SOCOM 1's order;
 * FreeResourcesSpecSub inlined into ControlSpecSub, no copy left), ScanSoft's speech recogniser,
 * prebuilt with ee-gcc 2.96 at -O2 -G0 -fno-strict-aliasing -funroll-loops -ffast-math (README "The
 * GCC front": SpectralSubtraction's seven max.s are -ffast-math's, round 25c; every function written
 * here compiles the same with and without it, round 26c, so the directive carries the object's
 * flag). Names are SOCOM 1's (the constructor's slots, s1:align, s1:calls); the block's fields
 * are named by their use. A block of the scheduler's (asr/tosblk.h): two inputs, four outputs.
 * Only the functions written so far are here, in image order (SpectralSubtraction, 0x1208, is not
 * written: declared, its row open). The session functions copy the record in field order, the
 * ring's size taken into the count's own variable (`n += s->nring * n`: the target's sum into the
 * count's register). The reset (ControlSpecSub's null session, InitSpecSub's tail) is one text in
 * both: the first loop's count a block-local initialised at its declaration, the count after it
 * the function's own variable (round 27c, below). */
#include "tosblk.h"

typedef struct SPECSUB {
    TOSBLOCK b;
    unsigned short f40;                 /* +40 */
    unsigned short f42;                 /* +42 */
    float *noise;                       /* +44: 2n */
    float *lognoise;                    /* +48: half the log of the noise */
    float *f52;                         /* +52 */
    float *f56;                         /* +56 */
    float *ring;                        /* +60: (nring + 1) frames of n */
    float *f64;                         /* +64 */
    float *ringpos;                     /* +68 */
    float *ringend;                     /* +72 */
    unsigned short nring;               /* +76: profile 3 */
    short pad78;
    float *f80;                         /* +80 */
    unsigned int n;                     /* +84: profile 8 */
    unsigned short f88;                 /* +88: profile 6, less one */
    short pad90;
    float f92;                          /* +92: profile 7 */
    float f96;                          /* +96: profile 4 */
    float f100;
    float f104;                         /* +104: profile 3 */
    float f108;                         /* +108: profile 2 */
    unsigned int nlim;                  /* +112: profile 30, the limits' count */
    unsigned int *lim;                  /* +116: profiles 31-33, each at most n; freed with the block */
    float f120;                         /* +120: -log 11 */
    float f124;                         /* +124: 10/11 */
    float f128;                         /* +128 */
    float f132;                         /* +132: the log of the largest level */
    float f136;
    float f140;
    float f144;
    float f148;
} SPECSUB;                              /* 0x98 bytes (ConstructSpecSub) */

/* a session: the two counters, the level, the four spectra, the ring's two positions, the ring */
typedef struct SPECSUBSESSION {
    int f40;
    int f42;
    float f128;
    float data[1];
} SPECSUBSESSION;

extern float logf(float x);
extern int qQueueControl(void *q, int cmd, unsigned short reader, int arg);
extern void heap_Free(void *heap, void *p);

/* not written yet (their rows open) */
int SpectralSubtraction(SPECSUB *s, unsigned char flag, void *frame, float *out1, float *out0);
static int InitSpecSub(SPECSUB *s);
static void ProcessSpecSub(SPECSUB *s, void **in);

/* a session read back: the spectra copied in, the noise's half log recomputed, the ring's
 * positions from their offsets */
static void SpecSubPutSession(SPECSUB *s, SPECSUBSESSION *ss)
{
    unsigned int n = s->n;
    float *p = ss->data;
    unsigned int i = 0;

    s->f40 = ss->f40;
    s->f42 = ss->f42;
    s->f128 = ss->f128;
    memcpy(s->noise, p, n * 4);
    p += n;
    memcpy(s->f52, p, n * 4);
    p += n;
    memcpy(s->f56, p, n * 4);
    p += n;
    memcpy(s->f80, p, n * 4);
    p += n;
    for (; i < n; i++)
        s->lognoise[i] = logf(s->noise[i]) * 0.5f;
    s->ringpos = s->ring + *(int *)p;
    p++;
    s->ringend = s->ring + *(int *)p;
    n += s->nring * n;
    memcpy(s->ring, p + 1, n * 4);
}

/* the session written out, in the same order */
static void SpecSubGetSession(SPECSUB *s, SPECSUBSESSION *ss)
{
    float *p = ss->data;
    unsigned int n = s->n;

    ss->f40 = s->f40;
    ss->f42 = s->f42;
    ss->f128 = s->f128;
    memcpy(p, s->noise, n * 4);
    p += n;
    memcpy(p, s->f52, n * 4);
    p += n;
    memcpy(p, s->f56, n * 4);
    p += n;
    memcpy(p, s->f80, n * 4);
    p += n;
    *(int *)p = s->ringpos - s->ring;
    p++;
    *(int *)p = s->ringend - s->ring;
    n += s->nring * n;
    memcpy(p + 1, s->ring, n * 4);
}

/* the resources freed with the block (SOCOM 1's FreeResourcesSpecSub, inlined) */
static __inline__ void FreeResourcesSpecSub(SPECSUB *s)
{
    heap_Free(s->b.tos->heap, s->ring);
    heap_Free(s->b.tos->heap, s->lim);
}

/* The reset's first loop bound reads the field again each turn beside a copy taken before the
 * loop (`m + s->n`: the target's two loads of +84, one kept, one reloaded). Round 27c: the copy is
 * a block-local `m` initialised from the field and the count after the loop is ANOTHER variable
 * (the function's `n`). GCSE makes the after-loop read a copy of its reaching register R, copied
 * after the first load (`m = load; R = m`) and after the loop's reload; the rerun of CSE then swaps
 * a load and the copy after it (`R = load; m = R`) when the copy's destination outlives the loaded
 * register (cse.c's REG0/REG1 swap: R becomes its class's first register when its last use is the
 * later one). With one variable for both counts (rounds 15c-26c) the variable lives on past R and
 * keeps the load (`lw a2; move a1,a2`); with the first count a variable of its own, dying in the
 * loop, R takes the load (`lw a1; move a2,a1`, the target's). (`n = s->n; m = n;` is 3 words: the
 * copy propagation folds m back into n.)
 *
 * the block's commands (case bodies in the target's order): 2 and 1 a session in and out (a null session resets the spectra), 3 and 4
 * the second input's queue stopped and started, 255 the block destroyed */
static int ControlSpecSub(SPECSUB *s, unsigned char cmd, char *sess, int *off)
{
    int ret = 1;
    float *p;
    unsigned int n;

    switch (cmd) {
    case 2:
        if (sess != 0) {
            SpecSubPutSession(s, (SPECSUBSESSION *)(sess + *off));
            *off += (s->nring * 4 + 20) * s->n + 20;
        } else {
            float *q;
            unsigned int m = s->n;

            for (q = s->noise; q < s->noise + m + s->n; q++)
                *q = 1.0f;
            n = s->n;
            s->f42 = 0;
            for (p = s->f56; p < s->f56 + n; p++)
                *p = 1.0f;
            for (p = s->f52; p < s->f52 + n; p++)
                *p = 1.0f;
            s->f40 = 0;
            s->f128 = 1.0f;
        }
        break;
    case 1:
        if (sess != 0)
            SpecSubGetSession(s, (SPECSUBSESSION *)(sess + *off));
        *off += (s->nring * 4 + 20) * s->n + 20;
        break;
    case 3:
        ret = qQueueControl(s->b.inputs[1].q, 5, s->b.inputs[1].reader, 0);
        break;
    case 4:
        ret = qQueueControl(s->b.inputs[1].q, 6, s->b.inputs[1].reader, 0);
        break;
    case TOS_DESTROY:
        FreeResourcesSpecSub(s);
        tosBaseBlockDestruct(&s->b);
        break;
    default:
        return 0;
    }
    return ret;
}

/* the profile read: the queues' element sizes (n bins, profile 8), the counts and the levels, one
 * block for the ring ((nring + 1) frames) and the five spectra after it, the constants, the
 * limits (profiles 31-33, each at most n; more than three is error 103), then the spectra reset
 * as the null session resets them. SOCOM 1's body (0x40C in both builds). Round 26c: the float
 * literals land on the target's bits as written (2.96's parse an ulp low: 0x3F4CCCCC, 0x3E4CCCCC,
 * 0x3DCCCCCC, 0x3F68BA2E; 3.7808886e37f is 0x7DE38DD0); the pointers before the constants and the
 * two stores of `buf` ring-first, the constants 0.8, 0.2, 0.1, 0.9 (sched1's tie-breaks: the last
 * store of a value issues first; 120 orders scored, build/scratch/gcc/v_iss3.py); the ring test a
 * guard (the error path falls through); the reset as ControlSpecSub's (round 27c: the first
 * loop's count a block-local initialised from the field, the count after it the function's `n` --
 * any second variable there matches; one variable for both was the 3 words rounds 15c-26c left). */
static int InitSpecSub(SPECSUB *s)
{
    TOS *tos = s->b.tos;
    unsigned int n, i;
    unsigned short k;
    float *buf, *q, *p;

    s->n = n = _tosGetProfileU32(s, 8, 21);
    s->b.inputs[0].size = n * sizeof(float);
    s->b.inputs[1].size = 1;
    s->b.outputs[0].size = 4;
    s->b.outputs[1].size = n * sizeof(float);
    s->b.outputs[2].size = n * sizeof(float);
    s->b.outputs[3].size = 4;
    s->f88 = _tosGetProfileU32(s, 6, 1) - 1;
    s->nring = _tosGetProfileU32(s, 1, 3);
    s->f108 = _tosGetProfileFloat(s, 2, 0.01f);
    s->f104 = _tosGetProfileFloat(s, 3, 1.0f);
    s->f96 = _tosGetProfileFloat(s, 4, 40.0f);
    s->f92 = _tosGetProfileFloat(s, 7, 0.99f);
    buf = heap_Calloc(tos->heap, n * (s->nring + 6), sizeof(float));
    s->ring = buf;
    s->ringpos = buf;
    s->f64 = buf + n * (s->nring + 1);
    s->noise = s->f64;
    s->lognoise = s->noise + n;
    s->f52 = s->lognoise + n;
    s->f56 = s->f52 + n;
    s->f80 = s->f56 + n;
    s->f136 = 0.8f;
    s->f140 = 0.2f;
    s->f144 = 0.1f;
    s->f148 = 0.9f;
    s->f132 = logf(3.7808886e37f);
    s->f128 = 1.0f;
    s->f120 = -logf(11.0f);
    s->f124 = 0.90909091f;
    s->nlim = (unsigned short)_tosGetProfileU32(s, 30, 1);
    s->lim = heap_Calloc(tos->heap, s->nlim, sizeof(unsigned int));
    for (i = 0; i < s->nlim; i++) {
        switch (i) {
        case 0:
            k = _tosGetProfileU32(s, 31, n);
            s->lim[0] = n < k ? n : k;
            break;
        case 1:
            k = _tosGetProfileU32(s, 32, n);
            s->lim[1] = n < k ? n : k;
            break;
        case 2:
            k = _tosGetProfileU32(s, 33, n);
            s->lim[2] = n < k ? n : k;
            break;
        default:
            _tosErrorLog(s, 103);
            return 1;
        }
    }
    if (s->ring == 0)
        return _tosErrorLog(s, 102);
    {
        unsigned int m = s->n;

        for (q = s->noise; q < s->noise + m + s->n; q++)
            *q = 1.0f;
        n = s->n;
        s->f42 = 0;
        for (p = s->f56; p < s->f56 + n; p++)
            *p = 1.0f;
        for (p = s->f52; p < s->f52 + n; p++)
            *p = 1.0f;
        s->f40 = 0;
        s->f128 = 1.0f;
    }
    return 0;
}

TOSBLOCK *ConstructSpecSub(TOS *tos, unsigned char id)
{
    return tosBaseBlockConstruct(tos, id, 2, 4, (TOSFUNC)ProcessSpecSub, (TOSFUNC)InitSpecSub,
                                 (TOSFUNC)ControlSpecSub, sizeof(SPECSUB));
}

/* a frame: the subtraction into the first two outputs, the noise's half log and the log level
 * into the other two */
static void ProcessSpecSub(SPECSUB *s, void **in)
{
    void *frame = in[0];

    if (frame != 0) {
        unsigned char *flag = in[1];
        float *o0 = qEnQueueOne(s->b.outputs[0].q);
        float *o1 = qEnQueueOne(s->b.outputs[1].q);
        float *o2 = qEnQueueOne(s->b.outputs[2].q);
        float *o3 = qEnQueueOne(s->b.outputs[3].q);

        SpectralSubtraction(s, *flag, frame, o1, o0);
        memcpy(o2, s->lognoise, s->n * 4);
        *o3 = logf(s->f128);
    }
}
