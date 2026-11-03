/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing -funroll-loops */
/* subsmp: the trigger's half-band subsampler (OVL2 0x505480-0x505D8C, SOCOM 1's object at
 * 0x370368: Subsampler_Process, Subsampler_CheckImpulseResponse (LOCAL there), Subsampler_Init,
 * Subsampler_Free, Subsampler_Reset), ScanSoft's speech recogniser, prebuilt with ee-gcc 2.96 at
 * -O2 -G0 -fno-strict-aliasing -funroll-loops (README "The GCC front"). Names are SOCOM 1's:
 * Subsampler_Process and the check by the object's order (Subsampler_Init calls the check). The
 * subsampler's fields live in the block that holds it (the trigger's: Init reads the block's
 * profile and logs through it), from +344: a 23-tap half-band filter -- its odd taps zero but the
 * centre -- scaled to 12-bit integers; every second input sample is filtered through a ring of
 * partial-product rows, the others only delayed.
 *
 * Reset and Free are public `__inline__` functions: Init has Reset's body inlined (GCC 2.96 inlines
 * only a body it has seen), and 2.96 defers an inlinable function's own copy to the end of the
 * object, in declaration order -- which gives the image's Init, Reset, Free (SOCOM 1's object,
 * Free then Reset, is Reset alone deferred). The filter table is a local array initialised from
 * .rodata (the block move at Init's top); three of its decimals are spelled an ulp above the
 * target's doubles, which 2.96's parser lands an ulp low (build/scratch/gcc/dconst.py). */
#include "tosblk.h"

typedef struct SUBSAMPLER {
    TOSBLOCK b;
    char pad40[244];
    unsigned int nout;                  /* +284: the output frame's samples */
    char pad288[56];
    int nin;                            /* +344: the input frame's samples (profile 30) */
    short *out;                         /* +348 */
    unsigned int ntaps;                 /* +352 */
    unsigned int n;                     /* +356: taps / 2 + 1 */
    unsigned int len;                   /* +360: taps / 2, the delay line's length */
    int *coef;                          /* +364: the even taps, scaled */
    int *coefend;                       /* +368 */
    int shift;                          /* +372 */
    int shift1;                         /* +376 */
    int *buf;                           /* +380: the delay line */
    int *bufend;                        /* +384 */
    int *pos;                           /* +388 */
    int *mid;                           /* +392 */
    int **rows;                         /* +396: the partial products, a ring of rows */
    int **rowpos;                       /* +400 */
    int **rowsend;                      /* +404 */
    int phase;                          /* +408 */
} SUBSAMPLER;

extern int abs(int);

int Subsampler_Process(SUBSAMPLER *s, short *in)
{
    if (in != 0) {
        short *end = in + s->nin;
        short *out = s->out;
        short *p;

        for (p = in; p < end; p++) {
            int x = *p;

            if (s->phase == 1) {
                *s->pos++ = x << s->shift1;
                if (s->pos >= s->bufend)
                    s->pos = s->buf;
                if (++s->mid >= s->bufend)
                    s->mid = s->buf;
                s->phase = 0;
            } else {
                int acc = 0;
                int *a = *s->rowpos;
                int *b = a + s->n;
                int *c;
                int **rp;
                unsigned int i;

                for (c = s->coef; c < s->coefend; c++) {
                    --b;
                    *a = *b = x * *c;
                    a++;
                }
                if (++s->rowpos >= s->rowsend)
                    s->rowpos = s->rows;
                rp = s->rowpos;
                for (i = 0; i < s->n; i++) {
                    acc += (*rp)[i];
                    if (++rp >= s->rowsend)
                        rp = s->rows;
                }
                acc += *s->mid;
                *out++ = acc >> s->shift;
                s->phase = 1;
            }
        }
    }
    return 0;
}

/* OPEN (49 words, 0x164 against 0x174): every block is the target's but the last two -- the
 * centre tap's test (0.5) and the odd taps' loop (0) share one tail in both builds, but here the
 * centre's copy is cross-jumped into the loop's (`b` into the loop body), where the target keeps
 * the centre's and the loop body jumps back into it (`ld; b tail; addiu 16`), the fail block
 * between them. Tried: guard clauses, the nested if/else with `err = 1; break`, a fail label
 * inside the centre test's then-block (this one), the check written once in a for(;;)/while/do
 * loop (d = *a - 0.5, then d = *a), an index loop (109s). The pairs loop and everything before
 * it match (`*a++ - *b--`: the increments sit before the calls). Two pool constants of 1e-15, as
 * the target's (0x65A4D8, 0x65A4E0). */
static int Subsampler_CheckImpulseResponse(SUBSAMPLER *s, double *h, unsigned int ntaps)
{
    int err = 0;
    double *a, *b;

    if ((ntaps & 1) == 0)
        goto fail;
    s->len = ntaps >> 1;
    s->n = s->len + 1;
    if ((s->len & 1) == 0 || (s->n & 1) == 1)
        goto fail;
    for (a = h, b = h + ntaps - 1; a < b; )
        if (abs(*a++ - *b--) > 1e-15)
            goto fail;
    if (abs(*a - 0.5) > 1e-15) {
fail:
        err = 1;
    } else {
        for (a = h + 1; a < b; a += 2)
            if (abs(*a) > 1e-15)
                goto fail;
    }
    if (err != 0)
        _tosErrorLog(s, 300);
    return err;
}

/* public and inline: Init has its body (GCC 2.96 inlines a body it has seen), and the copy is
 * output after Init */
__inline__ void Subsampler_Reset(SUBSAMPLER *s)
{
    int *p;
    int **pp;

    for (p = s->buf; p < s->bufend; p++)
        *p = 0;
    s->pos = s->buf;
    s->mid = s->buf + (s->len >> 1);
    for (pp = s->rows; pp < s->rowsend; pp++)
        for (p = *pp; p < *pp + s->n; p++)
            *p = 0;
    s->rowpos = s->rows;
    s->phase = 0;
}

int Subsampler_Init(SUBSAMPLER *s)
{
    TOS *tos = s->b.tos;
    int err = 0;
    int **pp;
    double h[23] = {
        -0.0044691990096899955, -2.8698779710312518e-16, 0.011551957698517619,
        3.995664741778653e-16, -0.024138391579451437, -5.525970358269552e-16,
        0.04670208572684477, 6.511092202679241e-16, -0.09517080679036068,
        -7.6770895142885006e-16, 0.3145323335822382, 0.5000000000000008,
        0.3145323335822382, -7.6770895142885006e-16, -0.09517080679036068,
        6.511092202679241e-16, 0.04670208572684477, -5.525970358269552e-16,
        -0.024138391579451437, 3.995664741778653e-16, 0.011551957698517619,
        -2.8698779710312518e-16, -0.0044691990096899955,
    };

    s->ntaps = 23;
    if (Subsampler_CheckImpulseResponse(s, h, 23) != 0) {
        err = 1;
    } else {
        s->nin = (unsigned short)_tosGetProfileU32(s, 30, 110);
        s->out = heap_Calloc(tos->heap, s->nout, 2);
        s->shift = 12;
        s->pos = 0;
        s->mid = 0;
        s->rowpos = 0;
        s->rowsend = 0;
        s->buf = heap_Calloc(tos->heap, s->len, 4);
        s->bufend = s->buf + s->len;
        s->rows = heap_Calloc(tos->heap, s->n, 4);
        s->rowsend = s->rows + s->n;
        if (s->out == 0 || s->buf == 0 || s->rows == 0)
            goto fail;
        for (pp = s->rows; pp < s->rowsend; pp++)
            *pp = heap_Calloc(tos->heap, s->n, 4);
        s->coef = heap_Calloc(tos->heap, s->n >> 1, 4);
        s->coefend = s->coef + (s->n >> 1);
        if (s->coef == 0) {
fail:
            _tosErrorLog(s, 2);
            err = 1;
        } else {
            int *p = s->coef;
            double *hp = h;
            double scale = 1 << s->shift;

            for (; p < s->coefend; p++, hp += 2)
                *p = *hp * scale;
            s->shift1 = s->shift - 1;
            Subsampler_Reset(s);
        }
    }
    return err;
}

__inline__ void Subsampler_Free(SUBSAMPLER *s)
{
    TOS *tos = s->b.tos;
    int **pp;

    if (s->coef != 0)
        heap_Free(tos->heap, s->coef);
    if (s->buf != 0)
        heap_Free(tos->heap, s->buf);
    for (pp = s->rows; pp < s->rowsend; pp++)
        heap_Free(tos->heap, *pp);
    if (s->rows != 0)
        heap_Free(tos->heap, s->rows);
    if (s->out != 0)
        heap_Free(tos->heap, s->out);
}
