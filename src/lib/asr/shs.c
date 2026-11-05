/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing -funroll-loops */
/* shs: the recogniser's sub-harmonic summation and voicing block (OVL2 0x502F50-0x504980, SOCOM 1's
 * object at 0x36E330: spline2, Lin_2_Log, SubHarmonicSummation_and_VUVDetermination,
 * ProcessSHS_VUV, ControlSHS_VUV, CreateFreqWindow, InitSHS_VUV -- all LOCAL there), ScanSoft's
 * speech recogniser, prebuilt with ee-gcc 2.96 at -O2 -G0 -fno-strict-aliasing -funroll-loops
 * (README "The GCC front"). Names are SOCOM 1's (s1:align, the object's order agreeing); the
 * block's fields are named by their use. ControlSHS_VUV and Lin_2_Log (round 27c) are written;
 * round 28c spline2 (0x4C0, SOCOM 1's body). */
#include "tosblk.h"

typedef struct SHS {
    TOSBLOCK b;
    unsigned short nlin;                /* +40: the linear spectrum's bins */
    unsigned short f42;                 /* +42: its bins per unit of frequency */
    unsigned short f44;                 /* +44: the log scale's first frequency */
    unsigned short nlog;                /* +46: the log scale's points per frame (and octave) */
    unsigned char nframes;              /* +48 */
    char pad49[15];
    void *f64;                          /* +64 */
    float *f68;                         /* +68: the spectrum (Lin_2_Log's y) */
    void **f72;                         /* +72: five tables */
    float *f76;                         /* +76: its second derivatives (spline2's) */
    float *f80;                         /* +80: the log-scale spectrum */
    void *f84;                          /* +84 */
    void *f88;                          /* +88 */
    void *f92;                          /* +92 */
} SHS;

static int ControlSHS_VUV(SHS *s, unsigned char cmd);
static void spline2(float *y, float *y2, SHS *s);

/* the spectrum's spline: y2 (n = nlin / 2 + 1 points) from y over unit spacing, the tridiagonal
 * system solved through the block's five tables -- the differences, the eliminated diagonal, its
 * inverses, the right-hand side and the forward sweep -- then back-substituted (the last diagonal
 * is 2 - diag[n - 2], as the vendor's code has it). Round 28c, SOCOM 1's body: four walkers shared
 * by all five loops (one register each, as the target's), counters counting down as written. Where
 * a walker's partner is formed from it (`q = p + 1`) and both step by `*p++`, CSE keeps `p + 4` in
 * a shadow register and the duplicated exit test's back edge is threaded to a label before the
 * loop notes: loop.c ignores the loop ("multiple entry points") -- not unrolled, not reversed, the
 * shadow re-formed from p (`move s1,s4; addiu s4,s1,4`) because p outlives it -- loops 1, 2 and 4;
 * loops 3 and 5 form their walkers apart and are unrolled. The last point's quotient goes through
 * the back-substitution's own walker, copied (`t = y2 + n - 1; r = t; *t-- = ...`: the target's
 * `move s0,a0`). */
static void spline2(float *y, float *y2, SHS *s)
{
    float **tab = (float **)s->f72;
    float *d = tab[0];
    float *diag = tab[1];
    float *inv = tab[2];
    float *rhs = tab[3];
    float *w = tab[4];
    int n = s->nlin / 2 + 1;
    float *p, *q, *r, *t;
    int i;

    p = y;
    q = p + 1;
    r = d + 1;
    for (i = n - 1; i > 0; i--)
        *r++ = *q++ - *p++;
    p = diag;
    q = p + 1;
    r = inv + 1;
    *p = 2.0f;
    for (i = n - 2; i > 0; i--) {
        *r = 1.0 / *p++;
        *q++ = 4.0f - *r++;
    }
    *r = 1.0 / *p;
    *q = 2.0f - *p;
    r = rhs;
    p = d + 1;
    q = d + 2;
    *r++ = *p * 3.0f;
    for (i = n - 2; i > 0; i--)
        *r++ = (*p++ + *q++) * 3.0f;
    *r = *p * 3.0f;
    p = inv + 1;
    q = rhs;
    r = w;
    t = r + 1;
    *r = *q++;
    for (i = n - 1; i > 0; i--)
        *t++ = *q++ - *p++ * *r++;
    p = diag + n - 1;
    q = w + n - 1;
    t = y2 + n - 1;
    r = t;
    *t-- = *q-- / *p--;
    for (i = n - 2; i >= 0; i--)
        *t-- = (*q-- - *r--) / *p--;
}

extern float powf(float x, float y);

/* the spectrum resampled on a log-frequency scale, nframes rows of nlog points, each the cubic
 * spline's value at its frequency (spline2's second derivatives), 0 past the spectrum's half;
 * the frequency steps by 2^(1/nlog). SOCOM 1's body (round 27c): the counts unsigned ints (the
 * conversions' unsigned paths), the bracket's two indices variables of their own (`khi = klo + 1`:
 * the target's separate `(klo + 1) * 4`; `y[n + 1]` folds into an offset of y[n]), the frame's
 * counter and row offset stepped together in the for (`j++, k += nlog`: the target's counter
 * set first), and the output indexed `out[k + i]` -- the strength-reduced pointer's start is the
 * giv's `k * 4 + out` (an `out + k` pointer of the source's adds in the other order). */
static void Lin_2_Log(SHS *s)
{
    unsigned char last_frame = s->nframes - 1;
    unsigned int nlog = s->nlog;
    unsigned int nlin = s->nlin;
    unsigned short last = nlog - 1;
    float *out = s->f80;
    float max = nlin * 0.5;
    float *y = s->f68;
    float *y2 = s->f76;
    float scale, ratio, f;
    unsigned int i, j, k;

    spline2(y, y2, s);
    scale = (float)nlin / s->f42;
    ratio = powf(2.0f, 1.0 / nlog);
    f = s->f44;
    for (j = 0, k = 0; j <= last_frame; j++, k += nlog) {
        for (i = 0; i <= last; i++) {
            float x = f * scale;

            if (x < max) {
                int klo = x;
                int khi = klo + 1;
                float a = x - klo;
                float b = 1.0f - a;
                float d = y[khi] - y[klo];

                out[k + i] = a * y[khi] + b * y[klo] + a * b * ((y2[klo] - d) * b - (y2[khi] - d) * a);
            } else
                out[k + i] = 0.0f;
            f *= ratio;
        }
    }
}

/* 255 frees the block's tables; nothing else is a command here. SOCOM 1's body */
static int ControlSHS_VUV(SHS *s, unsigned char cmd)
{
    TOS *tos;

    switch (cmd) {
    case TOS_DESTROY:
        tos = s->b.tos;
        heap_Free(tos->heap, s->f64);
        heap_Free(tos->heap, s->f68);
        heap_Free(tos->heap, s->f72[0]);
        heap_Free(tos->heap, s->f72[1]);
        heap_Free(tos->heap, s->f72[2]);
        heap_Free(tos->heap, s->f72[3]);
        heap_Free(tos->heap, s->f72[4]);
        heap_Free(tos->heap, s->f72);
        heap_Free(tos->heap, s->f76);
        heap_Free(tos->heap, s->f80);
        heap_Free(tos->heap, s->f84);
        heap_Free(tos->heap, s->f88);
        heap_Free(tos->heap, s->f92);
        tosBaseBlockDestruct(&s->b);
        break;
    default:
        return 0;
    }
    return 1;
}
