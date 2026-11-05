/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing -funroll-loops -ffast-math */
/* shs: the recogniser's sub-harmonic summation and voicing block (OVL2 0x502F50-0x504980, SOCOM 1's
 * object at 0x36E330: spline2, Lin_2_Log, SubHarmonicSummation_and_VUVDetermination,
 * ProcessSHS_VUV, ControlSHS_VUV, CreateFreqWindow, InitSHS_VUV -- all LOCAL there), ScanSoft's
 * speech recogniser, prebuilt with ee-gcc 2.96 at -O2 -G0 -fno-strict-aliasing -funroll-loops
 * (README "The GCC front") and -ffast-math (round 29c: CreateFreqWindow's peeled `0 - c` is a
 * neg.s and both log2s multiply by ln 2's reciprocal -- 2.96 does neither with IEEE semantics; the
 * object's other functions compile the same with and without it: the eleventh -ffast-math object).
 * Names are SOCOM 1's (s1:align, the object's order agreeing); the block's fields are named by
 * their use. ControlSHS_VUV and Lin_2_Log (round 27c) are written; round 28c spline2 (0x4C0,
 * SOCOM 1's body); round 29c CreateFreqWindow, InitSHS_VUV, ConstructSHS_VUV (over its draft),
 * SubHarmonicSummation_and_VUVDetermination (byte-exact in round 30c) and ProcessSHS_VUV (OPEN,
 * diagnosed in place). */
#include "tosblk.h"

typedef struct SHS {
    TOSBLOCK b;
    unsigned short nlin;                /* +40: the linear spectrum's bins */
    unsigned short f42;                 /* +42: its bins per unit of frequency */
    unsigned short f44;                 /* +44: the log scale's first frequency */
    unsigned short nlog;                /* +46: the log scale's points per frame (and octave) */
    unsigned char nframes;              /* +48 */
    unsigned char nharm;                /* +49: the harmonics summed */
    char pad50[2];
    float f52;                          /* +52 */
    unsigned int f56;                   /* +56 */
    unsigned short f60;                 /* +60: nframes * nlog */
    char pad62[2];
    float *f64;                         /* +64: the spectrum's peaks (ProcessSHS_VUV's) */
    float *f68;                         /* +68: the spectrum (Lin_2_Log's y) */
    void **f72;                         /* +72: five tables */
    float *f76;                         /* +76: its second derivatives (spline2's) */
    float *f80;                         /* +80: the log-scale spectrum */
    float *f84;                         /* +84: the sub-harmonic sum's input */
    float *f88;                         /* +88: the voicing window (CreateFreqWindow's) */
    int *f92;                           /* +92: each harmonic's offset on the log scale */
} SHS;                                  /* 96 (ConstructSHS_VUV's) */

static void ProcessSHS_VUV(SHS *s, void **in);
static int ControlSHS_VUV(SHS *s, unsigned char cmd);
static int InitSHS_VUV(SHS *s);
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

/* the sub-harmonic sum: the log spectrum (+84) copied into the output frame, each harmonic's shifted
 * copy added at a weight falling by f52 a harmonic; the frame is voiced (1 into the second output)
 * when its peak over the input's second word reaches f56. SOCOM 1's body (0x1F4 there: its loops
 * rolled), round 29c; byte-exact in round 30c. Measured: a walker and a source pointer of their own
 * per loop (one shared pointer crosses the second qEnQueueOne and takes s0 for all three loops); the
 * harmonic's offset read and stepped at the top (`k = *off++`); the copy, harmonic and peak ends
 * three variables (the target's a3, t1, a1). The harmonic end is computed in a temporary set twice
 * and copied (`t = out + n; t -= k; hend = t;`): set twice, t is no loop.c movable, so `out + n*4`
 * stays in the outer loop (`addu v1,t0,t5`, SOCOM 1's compile too), and the copy gives the end a
 * pseudo of its own (combine folds `t -= k` into it: `subu t1,v1,v0`) -- `hend = out + n - k` hoists
 * the sum, `hend = out + n; hend -= k` keeps it in hend's register (round 29c's 38 words). The
 * copy's source and the harmonic table read after the copy end (`q = s->f84; off = s->f92;`: sched1
 * issues the f84 load first, as the target); `x = in[0]; x++;` loads in[0] straight into x's
 * register (the target's `lw s2,0(s2)`; `in[0] + 1` loads into v0 first); the peak's ratio tested
 * as an expression (`max / *x < s->f56`: the quotient a pseudo of its own, $f0). */
static void SubHarmonicSummation_and_VUVDetermination(SHS *s, void **in)
{
    float r = s->f52;
    float h = r;
    float *out = qEnQueueOne(s->b.outputs[0].q);
    int n = s->f60;
    int *off, *offend;
    float *src;
    float *p, *q, *d, *q2, *end, *hend, *t, *mend;
    float max, *x;
    int k;
    unsigned char *flag;

    end = out + n;
    q = s->f84;
    off = s->f92;
    offend = off + s->nharm;
    for (d = out; d < end; )
        *d++ = *q++;
    src = s->f84;
    for (; off < offend; ) {
        k = *off++;
        t = out + n;
        t -= k;
        hend = t;
        q2 = src + k;
        for (d = out; d < hend; d++, q2++)
            *d += h * *q2;
        h *= r;
    }
    p = out;
    x = in[0];
    x++;
    flag = qEnQueueOne(s->b.outputs[1].q);
    max = *p;
    mend = p + s->f60;
    for (p++; p < mend; p++)
        if (max < *p)
            max = *p;
    if (max / *x < s->f56)
        *flag = 0;
    else
        *flag = 1;
}

/* one frame: the spectrum's (in[1]) peaks kept with their two neighbours each side in f64, the rest
 * zeroed, from the bin of f44 Hz; f64 smoothed (1 2 1) into f68, resampled on the log scale
 * (Lin_2_Log), windowed into f84 (0 where the log spectrum is not positive), and summed (a sibling
 * call). SOCOM 1's body, round 29c. Measured: a pointer of its own per loop (one shared walker
 * crosses the smoothing's libcalls and takes a saved register in every loop); the zeroed count
 * `n = nlin / 2 + 1` a variable (inline, fold distributes the * 4 over the + 1); the smoothing as
 * `*(x - 1) + *x + *x + *(x + 1)` -- the address of x - 1 a register, which the unroller's per-copy
 * pointers then share (round 28c's `*(p - 1)`; `x[-1]` folds the offset, `2 * x[0]` regroups); a
 * peak's last neighbour guarded by `p != pend`: jump threading merges that test with the loop's
 * (the target -- SOCOM 1's too -- exits after four copies when p is the last bin).
 * OPEN (0x638 against 0x640, 29 structural): the target's peak loop is entered by a jump over its
 * exit test, which sits above the body (the not-a-peak path branches back to it; p + 1 is formed
 * into the copy's register and moved into p there), where every loop shape tried -- do-while on
 * p++, for (;;) with a break, gotos into the body, while (1) -- keeps the test below the body; and
 * the zeroing loop's unrolled preconditioning annuls two slots the target leaves plain. */
static void ProcessSHS_VUV(SHS *s, void **in)
{
    float *spec, *peaks, *z, *zend, *p, *pend, *q, *y, *yend, *x, *o, *oend, *w;
    int i, n;
    float *r, *t;

    if (in[0] == 0)
        return;
    peaks = s->f64;
    n = s->nlin / 2 + 1;
    for (z = peaks, zend = z + n; z < zend; )
        *z++ = 0.0f;
    spec = in[1];
    pend = spec + s->nlin / 2 - 1;
    p = spec + (int)(s->f44 / ((float)s->f42 / s->nlin));
    do {
        if (*(p - 1) <= *p && *(p + 1) <= *p) {
            q = peaks + (p - spec);
            q[-2] = p[-2];
            q[-1] = *(p - 1);
            *q++ = *p;
            r = p + 1;
            *q++ = *r++;
            if (p != pend)
                *q = *r;
        }
    } while (p++ != pend);
    y = s->f68;
    *y = 0.0f;
    x = s->f64 + 1;
    for (yend = y + s->nlin / 2, y++; y < yend; y++, x++)
        *y = (*(x - 1) + *x + *x + *(x + 1)) * 0.25;
    *y = (*(x - 1) + *x) * 0.5;
    Lin_2_Log(s);
    o = s->f84;
    oend = o + s->f60;
    x = s->f80;
    w = s->f88;
    for (; o < oend; o++, x++, w++)
        *o = *x > 0.0f ? *x * *w : 0.0f;
    SubHarmonicSummation_and_VUVDetermination(s, in);
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

extern float logf(float x);
extern float atanf(float x);

/* the voicing window over the log scale's points: an arctangent step centred on the point of 65 Hz
 * (c = nlog * log2(65 / f44)), 0 below its foot. Round 29c, SOCOM 1's body with SOCOM II's float
 * calls: -ffast-math's (the peeled first step's `0 - c` folded to neg.s, the division by ln 2 a
 * product with its reciprocal -- 0.69314718f lands an ulp low in 2.96's parser, the reciprocal on
 * the target's 0x3FB8AA3C); the count an int converted after the logf call, the divisor `(float)nlog`
 * explicitly (its fptodp, hoisted as a loop invariant, reuses that conversion: through a float local
 * the conversion is made at the top and held in $f20) */
static void CreateFreqWindow(SHS *s)
{
    int i;
    unsigned short nlog = s->nlog;
    float *w = s->f88;
    unsigned short n = s->f60;
    float c = nlog * logf(65.0 / (float)s->f44) / 0.69314718f;

    for (i = 0; i < n; i++) {
        w[i] = atanf((i - c) * 3.0 / (float)nlog) * 0.3183098861837907 + 0.5;
        if (w[i] < 0.0f)
            w[i] = 0.0f;
    }
}

/* the block's profile -- 1: the spectrum's bins (65), 2: 2200, 3: the log scale's first frequency
 * (50), 4: its points (50), 5: the frames (5), 8: 5000, 6: the harmonics (15), 7: 0.84 -- its queues'
 * element sizes, its tables, the window and the harmonics' offsets on the log scale */
static int InitSHS_VUV(SHS *s)
{
    TOS *tos = s->b.tos;
    unsigned short nbins;
    int i, n, nlog;
    int *h;

    nbins = _tosGetProfileU32(s, 1, 65);
    s->nlin = (nbins - 1) * 2;
    s->f42 = _tosGetProfileU32(s, 2, 2200);
    s->f44 = _tosGetProfileU32(s, 3, 50);
    s->nlog = _tosGetProfileU32(s, 4, 50);
    s->nframes = _tosGetProfileU32(s, 5, 5);
    s->f60 = s->nframes * s->nlog;
    s->f56 = _tosGetProfileU32(s, 8, 5000);
    s->b.inputs[0].size = 24;
    s->b.inputs[1].size = nbins * 4;
    s->b.outputs[0].size = s->f60 * 4;
    s->b.outputs[1].size = 1;
    s->f64 = heap_Calloc(tos->heap, nbins, 4);
    s->f68 = heap_Calloc(tos->heap, nbins, 4);
    s->f72 = heap_Calloc(tos->heap, 5, 4);
    if (s->f64 == 0 || s->f68 == 0 || s->f72 == 0) {
        _tosErrorLog(s, 2);
        return 1;
    }
    s->f72[0] = heap_Calloc(tos->heap, nbins, 4);
    s->f72[1] = heap_Calloc(tos->heap, nbins, 4);
    s->f72[2] = heap_Calloc(tos->heap, nbins, 4);
    s->f72[3] = heap_Calloc(tos->heap, nbins, 4);
    s->f72[4] = heap_Calloc(tos->heap, nbins, 4);
    s->f76 = heap_Calloc(tos->heap, nbins, 4);
    s->f80 = heap_Calloc(tos->heap, s->f60, 4);
    s->f84 = heap_Calloc(tos->heap, s->f60, 4);
    s->f88 = heap_Calloc(tos->heap, s->f60, 4);
    CreateFreqWindow(s);
    s->nharm = _tosGetProfileU32(s, 6, 15);
    s->f92 = heap_Calloc(tos->heap, s->nharm, 4);
    n = s->nharm;
    nlog = s->nlog;
    h = s->f92;
    for (i = 0; i < n; i++)
        h[i] = nlog * logf(i + 2.0) / 0.69314718f + 0.5;
    s->f52 = _tosGetProfileFloat(s, 7, 0.84f);
    if (s->f72[0] == 0 || s->f72[1] == 0 || s->f72[2] == 0 || s->f72[3] == 0 || s->f72[4] == 0
        || s->f76 == 0 || s->f80 == 0 || s->f84 == 0 || s->f88 == 0 || s->f92 == 0) {
        _tosErrorLog(s, 2);
        return 1;
    }
    return 0;
}

TOSBLOCK *ConstructSHS_VUV(TOS *tos, unsigned char id)
{
    return tosBaseBlockConstruct(tos, id, 2, 2, (TOSFUNC)ProcessSHS_VUV, (TOSFUNC)InitSHS_VUV,
                                 (TOSFUNC)ControlSHS_VUV, sizeof(SHS));
}
