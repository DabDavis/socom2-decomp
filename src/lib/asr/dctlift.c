/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing -funroll-loops -ffast-math */
/* dctlift: the recogniser's DCT and cepstral lifter block (OVL2 0x4F43E0-0x4F494C, SOCOM 1's
 * object at 0x3613E0: InitDCTLift, ConstructDCTLift, ProcessDCTLift, ControlDCTLift, all but the
 * constructor LOCAL there), ScanSoft's speech recogniser, prebuilt with ee-gcc 2.96 at -O2 -G0
 * -fno-strict-aliasing -funroll-loops (README "The GCC front"). Names are SOCOM 1's, the object's
 * order agreeing with the constructor's slots (process 0x4F4730, init 0x4F43E0). The block takes
 * one input (a frame's level, then the filterbank's log energies) and writes the level and n
 * cepstral coefficients: a DCT-II over the bands from `first`, each basis row weighted by the
 * lifter 1 + 6 sin(pi i / L), the sum scaled by 2 / bands and the first coefficient halved.
 * Integers reach the double-precision arithmetic through `float` (the unsigned halving trick's
 * SFmode target, then fptodp), and pi is spelled twice (two pool entries, 0x6592E0 and 0x6592E8,
 * as in SOCOM 1's .rodata). The object is built with -ffast-math, as window.c's: the DCT's
 * first preconditioned step stores c * p over the 0 it just stored (`0 + x` folded, which 2.96
 * does only without IEEE semantics; 78 words longer without). The coefficient matrix's rows are
 * a variable of their own in the init (the target copies n into another saved register for the
 * allocation: one variable gives one register, 35 words). */
#include "tosblk.h"

#define PI 3.14159265358979323846

typedef struct DCTLIFT {
    TOSBLOCK b;
    unsigned int n;                     /* +40: the coefficients (profile 2, 13) */
    unsigned int nin;                   /* +44: the input's bands (profile 1, 21) */
    unsigned int first;                 /* +48: the first band used (profile 4, 0) */
    float *coef;                        /* +52: n rows of nin - first */
    float scale;                        /* +56: 2 / (nin - first) */
} DCTLIFT;                              /* 0x3C */

extern float sinf(float x);
extern float cosf(float x);

static void ProcessDCTLift(DCTLIFT *d, float **in);
static int ControlDCTLift(DCTLIFT *d, unsigned char cmd);

static int InitDCTLift(DCTLIFT *d)
{
    unsigned int n, m, i, j, nrows;
    short L;
    float w;
    float *c;

    n = (unsigned short)_tosGetProfileU32(d, 2, 13);
    d->n = n;
    nrows = n;
    L = _tosGetProfileU32(d, 3, 16);
    d->nin = (unsigned short)_tosGetProfileU32(d, 1, 21);
    d->first = (unsigned short)_tosGetProfileU32(d, 4, 0);
    m = d->nin - d->first;
    d->b.inputs[0].size = d->nin * 4 + 4;
    d->b.outputs[0].size = n * 4 + 4;
    d->scale = 2.0 / (float)m;
    d->coef = heap_Alloc(d->b.tos->heap, nrows * m * 4);
    c = d->coef;
    for (i = 0; i < n; i++) {
        w = sinf((float)i * PI / (float)L) * 6.0f + 1.0f;
        for (j = 0; j < m; j++)
            *c++ = w * cosf(((float)j + 0.5) * PI * (float)i / (float)m);
    }
    return 0;
}

TOSBLOCK *ConstructDCTLift(TOS *tos, unsigned char id)
{
    return tosBaseBlockConstruct(tos, id, 1, 1, (TOSFUNC)ProcessDCTLift, (TOSFUNC)InitDCTLift,
                                 (TOSFUNC)ControlDCTLift, sizeof(DCTLIFT));
}

static void ProcessDCTLift(DCTLIFT *d, float **in)
{
    float *x = in[0];
    float *out, *end, *to, *p, *c;
    float scale;

    if (x == 0)
        return;
    out = qEnQueueOne(d->b.outputs[0].q);
    c = d->coef;
    scale = d->scale;
    *out++ = *x++;
    end = out + d->n;
    to = x + d->nin;
    x += d->first;
    for (; out < end; out++) {
        *out = 0;
        for (p = x; p < to; p++)
            *out += *c++ * *p;
        *out *= scale;
    }
    *(end - d->n) *= 0.5f;
}

static int ControlDCTLift(DCTLIFT *d, unsigned char cmd)
{
    if (cmd != TOS_DESTROY)
        return 0;
    heap_Free(d->b.tos->heap, d->coef);
    tosBaseBlockDestruct(&d->b);
    return 1;
}
