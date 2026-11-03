/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing -funroll-loops */
/* acne: the recogniser's ACNE block, its level and noise-floor normaliser (OVL2 0x4F35A0-0x4F43DC,
 * SOCOM 1's object at 0x360460: Reset, NormalizeAndEnQ, ProcessAcne, FreeResourcesAcne,
 * ControlAcne, InitAcne, ConstructAcne, all but the constructor LOCAL there -- SOCOM II has no
 * FreeResourcesAcne row), ScanSoft's speech recogniser, prebuilt with ee-gcc 2.96 at -O2 -G0
 * -fno-strict-aliasing -funroll-loops (README "The GCC front"). Names are SOCOM 1's (the object's
 * order and the constructor's slots). The block takes four inputs (two frames of n features, a
 * level and a 24-byte frame record) and writes n + 1 words to its own queue; it keeps a speech
 * mean and a noise floor per feature, seeded from the two 11 kHz tables below and smoothed
 * through the smoothing object (smtConstruct, SOCOM 1's Smoother1600Info11kHz). ProcessAcne is
 * not written (declared). */
#include "tosblk.h"

typedef struct ACNE {
    TOSBLOCK b;
    void *q;                            /* +40: the block's own queue */
    short f44;                          /* +44: profile 2 (100) */
    char pad46[2];
    int state;                          /* +48: 2 after a reset */
    unsigned char f52;                  /* +52 */
    unsigned char smoothed;             /* +53: the speech mean went through the smoother */
    char pad54[2];
    float *mean;                        /* +56: n */
    float *mean0;                       /* +60: n, the first of the three arrays */
    float *f64;                         /* +64: n */
    float f68;                          /* +68: 100000 after a reset */
    float f72;                          /* +72 */
    int f76;
    float f80;                          /* +80: 0.5 after a reset */
    float f84;                          /* +84: the frames since the mean was smoothed */
    int f88;
    int f92;
    unsigned int n;                     /* +96: features a frame (profile 9, 21) */
    float f100;                         /* +100: 10 / f108 */
    int f104;                           /* +104: profile 6 (15) */
    float f108;                         /* +108: profile 5 (50) */
    float f112;                         /* +112: profile 7 (-30) in nepers */
    float f116;                         /* +116: 4 */
    float f120;                         /* +120: profile 3 (-1000) in nepers */
    float f124;                         /* +124: profile 4 (-1000) in nepers */
    char pad128[4];
    float f132;                         /* +132: 5 */
    const float *noise;                 /* +136: g_NoiseFloors11kHz */
    const float *speech;                /* +140: g_SpeechMeanInit11kHz */
    const void *smtinfo;                /* +144: the smoother's tables */
    void *smoother;                     /* +148 */
} ACNE;                                 /* 0x98 */

/* SOCOM 1's LOCAL tables of the same names, 21 features each */
static const float g_SpeechMeanInit11kHz[21] = {
    7.5637, 8.2281, 8.596, 8.6901, 8.7195, 8.7353, 8.7409, 8.7439, 8.7445, 8.7442, 8.7438,
    8.743, 8.7414, 8.739, 8.7338, 8.7239, 8.7083, 8.6777, 8.6297, 8.511, 8.2116,
};

static const float g_NoiseFloors11kHz[21] = {
    -1.6742692, -2.1850982, -2.5215726, -2.7728887, -2.87825, -2.87825, -2.87825, -2.87825,
    -2.87825, -2.87825, -2.87825, -2.87825, -2.87825, -2.87825, -2.87825, -2.87825, -2.87825,
    -2.87825, -2.87825, -2.9869571, -3.0510371,
};

/* the smoothing object's 11 kHz tables (SOCOM 1's Smoother1600Info11kHz: three {spline table,
 * 21, 10} entries) */
extern const char Smoother1600Info11kHz[];

extern void qQueueReset(void *q);
extern void *qQueueConstruct(TOS *tos, unsigned char id, int n);
extern int qQueueInitEx(void *q, unsigned short size, int n, int flags);
extern void *smtConstruct(TOS *tos, unsigned int n, const void *info, float a, float b);
extern int Smoothing(void *smoother, float *x);
extern void *qDeQueueOne(void *q, int reader);
extern void qFreeUnusedElemements(void *q);
extern float HLnOnePlusExpHFloat(float x);
extern float func_004F6000(float x, float y);
extern int qQueueControl(void *q, unsigned char cmd, int arg, int n);
extern void smtDestruct(void *smoother, TOS *tos);    /* SOCOM 1's smtDestruct */

extern void ProcessAcne(ACNE *a, unsigned char **in, int n);

static int Reset(ACNE *a)
{
    qQueueReset(a->q);
    a->state = 2;
    a->f52 = 0;
    if (a->f132 > 0.0f) {
        memcpy(a->mean0, a->speech, a->n * sizeof(float));
        memcpy(a->mean, a->speech, a->n * sizeof(float));
        if (Smoothing(a->smoother, a->mean) != 0)
            return 1;
        a->smoothed = 1;
    } else {
        memset(a->mean0, 0, a->n * sizeof(float));
        a->smoothed = 0;
    }
    memset(a->f64, 0, a->n * sizeof(float));
    a->f68 = 100000.0f;
    a->f72 = a->f132;
    a->f80 = 0.5f;
    a->f76 = 0;
    a->f84 = 0;
    a->f88 = 0;
    a->f92 = 0;
    return 0;
}

/* a frame out of the block's own queue, its features less the speech mean and log-added to the
 * noise floor (or the floor itself before the mean is smoothed), its level first, into the output
 * queue; 0 when the queue was empty or the smoothing failed */
static int NormalizeAndEnQ(ACNE *a)
{
    const float *noise = a->noise;
    unsigned int n = a->n;
    float *in;
    float *out;
    float *p;

    in = qDeQueueOne(a->q, 0);
    if (in == 0)
        return 0;
    if (a->f84 > a->f100) {
        a->f84 = 0;
        memcpy(a->mean, a->mean0, n * sizeof(float));
        if (Smoothing(a->smoother, a->mean) != 0)
            return 0;
        a->smoothed = 1;
    }
    out = qEnQueueOne(a->b.outputs[0].q);
    p = out + 1;
    if (a->smoothed != 0) {
        float *end = p + n;
        float *m;

        memcpy(p, in, n * sizeof(float));
        for (m = a->mean; p < end; p++, m++, noise++) {
            *p -= *m;
            if (*p < *noise)
                *p = *noise + HLnOnePlusExpHFloat(*p - *noise);
            else
                *p = *p + HLnOnePlusExpHFloat(*noise - *p);
        }
    } else
        memcpy(p, noise, n * sizeof(float));
    *out = func_004F6000(in[n] - a->f68, a->f112);
    qFreeUnusedElemements(a->q);
    return 1;
}

/* SOCOM 1's FreeResourcesAcne, inlined into the control (no copy left) */
static __inline__ void FreeResourcesAcne(ACNE *a)
{
    TOS *tos = a->b.tos;

    qQueueControl(a->q, 255, 0, 0);
    heap_Free(tos->heap, a->mean0);
    smtDestruct(a->smoother, tos);
}

/* 2 marks the block (f52); 3 saves the block's state into the session buffer at *pos (or only
 * counts its size), 4 restores it (or, without a buffer, resets the block); 255 destroys it.
 * OPEN (48 words, the right size): every instruction is the target's but the session pointer and
 * the array size take each other's saved register (the target: p in s0, size in s1). Tried: the
 * reset first or in the else, the record read through p or its own base (this one: the base, as
 * the target's lbu 4/8 off a3), size declared at the top, in the block, before or after rec. */
static int ControlAcne(ACNE *a, unsigned char cmd, unsigned char *buf, int *pos)
{
    unsigned char *p;

    switch (cmd) {
    case 2:
        a->f52 = 1;
        break;
    case 4:
        if (buf != 0) {
            unsigned int size = a->n * sizeof(float);
            unsigned char *rec = buf + *pos;

            p = rec + 12;
            a->state = *(int *)rec;
            a->f52 = rec[4];
            a->smoothed = rec[8];
            memcpy(a->mean, p, size);
            p += size;
            memcpy(a->mean0, p, size);
            p += size;
            memcpy(a->f64, p, size);
            p += size;
            a->f68 = *(float *)p;
            p += 4;
            a->f72 = *(float *)p;
            p += 4;
            a->f76 = *(int *)p;
            p += 4;
            a->f80 = *(float *)p;
            p += 4;
            a->f84 = *(float *)p;
            p += 4;
            a->f88 = *(int *)p;
            p += 4;
            a->f92 = *(int *)p;
            *pos += a->n * 3 * sizeof(float) + 40;
        } else if (Reset(a) != 0)
            return 0;
        break;
    case 3:
        if (buf != 0) {
            unsigned int size = a->n * sizeof(float);
            unsigned char *rec = buf + *pos;

            p = rec + 12;
            *(int *)rec = a->state;
            rec[4] = a->f52;
            rec[8] = a->smoothed;
            memcpy(p, a->mean, size);
            p += size;
            memcpy(p, a->mean0, size);
            p += size;
            memcpy(p, a->f64, size);
            p += size;
            *(float *)p = a->f68;
            p += 4;
            *(float *)p = a->f72;
            p += 4;
            *(int *)p = a->f76;
            p += 4;
            *(float *)p = a->f80;
            p += 4;
            *(float *)p = a->f84;
            p += 4;
            *(int *)p = a->f88;
            p += 4;
            *(int *)p = a->f92;
        }
        *pos += a->n * 3 * sizeof(float) + 40;
        break;
    case 255:
        FreeResourcesAcne(a);
        tosBaseBlockDestruct(&a->b);
        break;
    default:
        return 0;
    }
    return 1;
}

static int InitAcne(ACNE *a)
{
    TOS *tos = a->b.tos;
    unsigned short n;

    n = _tosGetProfileU32(a, 9, 21);
    a->n = n;
    a->b.inputs[0].size = n * 4;
    a->b.inputs[1].size = n * 4;
    a->b.inputs[2].size = 4;
    a->b.inputs[3].size = 24;
    a->b.outputs[0].size = n * 4 + 4;
    a->f44 = _tosGetProfileU32(a, 2, 100);
    a->q = qQueueConstruct(tos, 255, 1);
    if (a->q == 0)
        return 1;
    if (qQueueInitEx(a->q, n * 4 + 4, 1, 0) == 0)
        return 1;
    a->mean0 = heap_Calloc(tos->heap, n * 3, 4);
    a->f64 = a->mean0 + n;
    a->mean = a->f64 + n;
    a->f120 = _tosGetProfileFloat(a, 3, -1000.0f) * 0.11513f;
    a->f124 = _tosGetProfileFloat(a, 4, -1000.0f) * 0.11513f;
    a->f116 = 4.0f;
    a->f108 = _tosGetProfileFloat(a, 5, 50.0f);
    a->f100 = 10.0f / a->f108;
    a->f104 = (short)_tosGetProfileU32(a, 6, 15);
    a->f112 = _tosGetProfileFloat(a, 7, -30.0f) * 0.23026f;
    a->f132 = 5.0f;
    a->noise = g_NoiseFloors11kHz;
    a->speech = g_SpeechMeanInit11kHz;
    a->smtinfo = Smoother1600Info11kHz;
    a->smoother = smtConstruct(tos, n, a->smtinfo, _tosGetProfileFloat(a, 8, 2.6f), 0.0f);
    return Reset(a) != 0;
}

TOSBLOCK *ConstructAcne(TOS *tos, unsigned char id)
{
    return tosBaseBlockConstruct(tos, id, 4, 1, (TOSFUNC)ProcessAcne, (TOSFUNC)InitAcne,
                                 (TOSFUNC)ControlAcne, sizeof(ACNE));
}
