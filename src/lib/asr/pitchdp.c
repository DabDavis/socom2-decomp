/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing -funroll-loops */
/* pitchdp: the recogniser's pitch tracker, a dynamic programme over pitch bins (OVL2
 * 0x4F0CB8-0x4F2900, SOCOM 1's object at 0x35E0E8: Actual_DP, BackTracking, PostDP, FlushPitchDP,
 * ProcessPitchDP, ControlPitchDP, InitJumpFactors, InitPitchDP, ConstructPitchDP, all but the
 * constructor LOCAL there), ScanSoft's speech recogniser, prebuilt with ee-gcc 2.96 at -O2 -G0
 * -fno-strict-aliasing -funroll-loops (README "The GCC front"). Names are SOCOM 1's (the object's
 * order, the constructor's slots and the call sites). The block takes one input -- a frame of
 * nbins pitch-bin scores, the bins log-spaced from fmin over bins_per_octave -- into a grid of
 * nbins rows by ncols columns, and once a column block is in, runs the Viterbi pass (Actual_DP:
 * every bin of column t takes the best of the bins within maxjump of it in column t - 1, weighted
 * by the jump factors), backtracks the best path into the pitch track (BackTracking) and writes
 * the track's frequencies to its output queue, then shifts the grid (PostDP). The records'
 * fields are named by use. */
#include "tosblk.h"

/* a grid cell: the frame's score for the bin, the path's score, the bin it came from */
typedef struct CELL {
    float value;                        /* +0 */
    float score;                        /* +4 */
    unsigned int back;                  /* +8 */
} CELL;

typedef struct PITCHDP {
    TOSBLOCK b;
    unsigned short nbins;               /* +40: bins_per_octave * log2(fmax / fmin) */
    unsigned char lookahead;            /* +42: profile 4 (15) */
    unsigned char overlap;              /* +43: profile 5 (6) */
    unsigned char first;                /* +44: the column the next frame goes to, past col */
    char pad45[3];
    unsigned int thresh;                /* +48: profile 6 (10): a bin is extended above max - max / thresh */
    unsigned short maxjump;             /* +52: profile 7 (10), in bins */
    char pad54[2];
    float block;                        /* +56: the column blocks done, from 1 */
    unsigned short f60;                 /* +60: profile 8 (2000) */
    unsigned short ncols;               /* +62: lookahead + overlap * 2 */
    float f64;                          /* +64: profile 9 (0.01): f60 * f64 is the jump factors' width */
    unsigned short len;                 /* +68: the columns a block runs the DP over */
    unsigned short last;                /* +70 */
    unsigned short col;                 /* +72: the columns filled in this block */
    unsigned short nflush;              /* +74: the columns the flush writes */
    unsigned short flushed;             /* +76 */
    unsigned char flushing;             /* +78: command 200 asks for a flush */
    unsigned char unflushed;            /* +79 */
    unsigned char fresh;                /* +80: the grid's first column is not yet seeded */
    char pad81[3];
    CELL *cells;                        /* +84: nbins * ncols */
    CELL **rows;                        /* +88: each bin's row of cells */
    float *freqs;                       /* +92: each bin's frequency */
    float **jf;                         /* +96: the jump factors, nbins by nbins */
    float *track;                       /* +100: the best path's frequencies, one per column */
} PITCHDP;                              /* 0x68 */

extern float logf(float x);
extern float powf(float x, float y);

static void Actual_DP(PITCHDP *p, unsigned int nbins, unsigned int ncols);
static void BackTracking(PITCHDP *p, unsigned int ncols);
static void PostDP(PITCHDP *p);
static int FlushPitchDP(PITCHDP *p);
static void ProcessPitchDP(PITCHDP *p, float **in);
static int ControlPitchDP(PITCHDP *p, unsigned char cmd);
static void InitJumpFactors(PITCHDP *p);
static int InitPitchDP(PITCHDP *p);

/* OPEN (0x430 against 0x404): the loop nest is the target's -- the previous column's
 * maximum over p->cells/p->ncols reloaded per column, the threshold max - max / thresh, the bins
 * within maxjump, the inner loop unrolled by four -- but the target keeps one more value live
 * across the loops (s0-s6 saved, a 64-byte frame; here s0-s5, 48). Round 14c: the target loads
 * jf[j] ONCE per bin (`lw v1,0(t8)`, t8 walking jf) and walks &jf[j][k] and &rows[k] as pointers
 * stepped by 4 (the unrolled copies at 0, 4, 8, 12), where this source reloads jf[j] after every
 * store (no strict aliasing) and indexes by k. `float *f = jf[j];` after hi is set gives the one
 * load (0x420, 264 words against 257) but still indexes f[k] and rows[k]: k as int, unsigned
 * short, j and t as int, and the max loop's `c += stride` (the target's step is the stride
 * local, not p->ncols reloaded) do not bring the strength reduction. */
static void Actual_DP(PITCHDP *p, unsigned int nbins, unsigned int ncols)
{
    unsigned short stride = p->ncols;
    CELL *cells = p->cells;
    unsigned short maxjump = p->maxjump;
    float **jf = p->jf;
    CELL **rows = p->rows;
    unsigned int t, j, k;
    unsigned short i, prev, lo, hi;
    float max, thr, score;
    CELL *c, *src, *dst;

    for (t = 1; t < ncols; t++) {
        prev = t - 1;
        max = 0;
        c = &p->cells[prev];
        for (i = 0; i < p->nbins; i++) {
            if (max < c->score)
                max = c->score;
            c += p->ncols;
        }
        thr = max - max / p->thresh;
        for (j = 0; j < nbins; j++) {
            src = &cells[j * stride + t - 1];
            if (thr < src->score) {
                lo = 0;
                if (j >= maxjump)
                    lo = j - maxjump;
                hi = nbins - 1;
                if (nbins - j >= maxjump)
                    hi = j + maxjump;
                for (k = lo; k < hi; k++) {
                    if (jf[j][k] > 0) {
                        dst = &rows[k][t];
                        score = src->score + jf[j][k] * dst->value;
                        if (dst->score < score) {
                            dst->score = score;
                            dst->back = j;
                        }
                    }
                }
            }
        }
    }
}

/* the best path: the last column's best bin, then each column's bin back through the cells' back
 * links. The column index t is a register-wide variable assigned through `(unsigned short)` (the
 * target's `andi 0xffff` at both assignments): the backtrace's `t--` is then a plain decrement and
 * the loop strength-reduces &track[t] and t's cell offset into pointers stepped by 4 and 12, which
 * an `unsigned short t` (masked at every step) does not give -- 68 words, with best_i and track,
 * the cell pointer and the index's *4 swapping registers besides (an int t compiles the same).
 * The first search's cell pointer is the backtrace's (c = rows[i] + t: a pointer of its own there
 * is 147 words). */
static void BackTracking(PITCHDP *p, unsigned int ncols)
{
    unsigned short nbins = p->nbins;
    float *track = p->track;
    CELL **rows = p->rows;
    float *freqs = p->freqs;
    unsigned int i, best_i;
    unsigned int t;
    float best;
    CELL *c;

    if (ncols > 1) {
        t = (unsigned short)(ncols - 1);
        best = rows[0][t].score;
        track[t] = freqs[0];
        best_i = 0;
        for (i = 1; i < nbins; i++) {
            c = rows[i] + t;
            if (best < c->score) {
                best = c->score;
                track[t] = freqs[i];
                best_i = i;
            }
        }
        c = &rows[best_i][ncols - 1];
        t = (unsigned short)(ncols - 1);
        while (t != 0) {
            t--;
            track[t] = freqs[c->back];
            c = &rows[c->back][t];
        }
    } else {
        track[0] = 0;
        best = rows[0][0].value;
        for (i = 1; i < nbins; i++) {
            if (best < rows[i][0].value) {
                best = rows[i][0].value;
                track[0] = freqs[i];
            }
        }
    }
}

/* the grid's last columns moved to its front for the next block */
static void PostDP(PITCHDP *p)
{
    unsigned short nbins = p->nbins;
    unsigned char lookahead = p->lookahead;
    unsigned char overlap = p->overlap;
    CELL **rows = p->rows;
    unsigned int k, i;

    if (p->block == 1.0) {
        for (k = 0; k < overlap * 2; k++)
            for (i = 0; i < nbins; i++)
                rows[i][k] = rows[i][k + (lookahead - overlap)];
    } else {
        for (k = 0; k < overlap * 2; k++)
            for (i = 0; i < nbins; i++)
                rows[i][k] = rows[i][k + lookahead];
    }
}

/* the input's end: the DP over the columns filled, then their track out a frame a call; 1 while
 * frames remain */
static int FlushPitchDP(PITCHDP *p)
{
    unsigned short nbins = p->nbins;
    int ret = 0;
    unsigned int i;
    CELL **rows;

    if (p->unflushed) {
        if (p->block == 1.0)
            p->nflush = p->col;
        else
            p->nflush = p->overlap + p->col;
        if (p->nflush == 0) {
            p->flushing = 0;
            goto done;
        }
        rows = p->rows;
        for (i = 0; i < nbins; i++)
            (*rows++)->back = 0;
        Actual_DP(p, nbins, p->nflush);
        BackTracking(p, p->nflush);
        if (p->block == 1.0)
            p->overlap = 0;
        p->unflushed = 0;
    }
    *(float *)qEnQueueOne(p->b.outputs[0].q) = p->track[p->overlap + p->flushed];
    p->flushed++;
    if (p->flushed == p->nflush)
        p->flushing = 0;
    else
        ret = 1;
done:
    return ret;
}

/* OPEN (43 words, the right size): the block header is copied to the stack from a second
 * pointer to the record (`TOSBLOCK *b = &p->b`: the target's and SOCOM 1's s6, `move s6,s0` at the
 * entry), the fresh grid's seeding unrolled through `c = *r++` as the target's -- what is left is
 * the s-register choice: the target gives nbins s1, the frame s2, col s3 (and later the track s3,
 * the lookahead s2), here col s1, nbins s2, the frame s3; and p->col is reloaded after the copy
 * into v1 there, into col's s1 here. Tried: the index as first + col (43), col += first (50),
 * first an int (45), the cell through a local (45), the frame walked (50), the declaration orders. */
static void ProcessPitchDP(PITCHDP *p, float **in)
{
    TOSBLOCK *b = &p->b;
    float *x = in[0];
    unsigned short nbins = p->nbins;
    unsigned short len = p->len;

    if (x != 0) {
        CELL **rows = p->rows;
        unsigned short col = p->col;
        unsigned short j;
        unsigned char first;

        if (p->block == 2.0) {
            p->first = p->overlap * 2;
            p->len = p->lookahead + p->overlap * 2;
            p->last = p->lookahead;
        }
        first = p->first;
        for (j = 0; j < nbins; j++)
            rows[j][first + col].value = x[j];
        if (p->col < p->last - 1)
            p->col++;
        else {
            unsigned char n;
            float *track;
            unsigned char off;
            unsigned char i;
            TOSBLOCK blk;

            if (p->fresh) {
                CELL **r = p->rows;
                unsigned int k;

                for (k = 0; k < nbins; k++) {
                    CELL *c = *r++;

                    c->score = c->value;
                    c->back = 0;
                }
                p->fresh = 0;
            }
            Actual_DP(p, nbins, len);
            BackTracking(p, len);
            n = p->lookahead;
            blk = *b;
            track = p->track;
            off = p->overlap;
            if (p->block == 1.0) {
                for (i = 0; i < n; i++)
                    *(float *)qEnQueueOne(blk.outputs[0].q) = track[i];
            } else {
                for (i = 0; i < n; i++)
                    *(float *)qEnQueueOne(blk.outputs[0].q) = track[i + off];
            }
            PostDP(p);
            p->col = 0;
            p->block += 1.0f;
        }
    } else if (p->flushing == 1)
        FlushPitchDP(p);
}

/* 201 restarts the tracker, 200 asks for a flush, 255 destroys the block.
 * Command 201's ten stores are independent, and the first scheduling pass (before register
 * allocation) ranks a store that is the LAST USE of its value's register first (it frees a
 * register: INSN_REG_WEIGHT), then source order. The target's output -- fresh, last, block,
 * flushed, len, col, first, nflush, flushing, unflushed (in the loop branch's slot) -- so
 * needs fresh after unflushed (fresh then frees the 1), last after len (last frees the sum),
 * last before block, and the stores that free nothing in output order: one such source is
 * this one (written as the output order, it was 10 words off: unflushed, block and len freed
 * their registers and went first). */
static int ControlPitchDP(PITCHDP *p, unsigned char cmd)
{
    switch (cmd) {
    case 201: {
        unsigned short ncols = p->ncols;
        unsigned short nbins = p->nbins;
        CELL *cells = p->cells;
        unsigned short i, j;

        p->overlap = _tosGetProfileU32(p, 5, 6);
        p->flushed = 0;
        p->len = p->lookahead + p->overlap;
        p->last = p->lookahead + p->overlap;
        p->block = 1.0f;
        p->col = 0;
        p->first = 0;
        p->nflush = 0;
        p->flushing = 0;
        p->unflushed = 1;
        p->fresh = 1;
        for (i = 0; i < ncols; i++)
            for (j = 0; j < nbins; j++)
                cells[j * ncols + i].score = 0;
        break;
    }
    case 200:
        p->flushing = 1;
        break;
    case 255: {
        CELL *cells = p->cells;
        unsigned short nbins = p->nbins;
        float **jf = p->jf;
        TOS *tos = p->b.tos;
        unsigned short i;

        if (cells != 0) {
            heap_Free(tos->heap, cells);
            heap_Free(tos->heap, p->rows);
            heap_Free(tos->heap, p->freqs);
            heap_Free(tos->heap, p->track);
            for (i = 0; i < nbins; i++)
                heap_Free(tos->heap, jf[i]);
            heap_Free(tos->heap, p->jf);
        }
        tosBaseBlockDestruct(&p->b);
        break;
    }
    default:
        return 0;
    }
    return 1;
}

/* each bin's jump factors: 1 - (df / width)^2 out to where they fall below 0 */
/* OPEN (81 words, the right size): the FP registers only. The target's unrolled blocks hold the
 * two frequencies and d in three registers (f2, f1 -> f0), so width is f3 and the constants f6, f5,
 * f4; here d reuses a frequency's register and everything sits one lower. `d = df; d /= width`
 * (87 -> 81) is the nearest of seven spellings tried (one expression, d = 1 - d*d, d *= d, ...).
 * Only the unrolled copies differ: the preconditioning copies reuse the register in the target
 * too. A d per loop (block-scoped, or a second function-level variable for one loop) is 82. */
static void InitJumpFactors(PITCHDP *p)
{
    float width = p->f60 * p->f64;
    unsigned short nbins = p->nbins;
    float **jf = p->jf;
    float *freqs = p->freqs;
    int i, k;
    float d;

    for (i = 0; i < nbins; i++) {
        jf[i][i] = 1.0f;
        for (k = i - 1; k >= 0; k--) {
            d = freqs[i] - freqs[k];
            d /= width;
            jf[i][k] = 1.0f - d * d;
            if (jf[i][k] < 0) {
                jf[i][k] = 0;
                break;
            }
        }
        for (k = i + 1; k < nbins; k++) {
            d = freqs[k] - freqs[i];
            d /= width;
            jf[i][k] = 1.0f - d * d;
            if (jf[i][k] < 0) {
                jf[i][k] = 0;
                break;
            }
        }
    }
}

/* OPEN (0x3C0 against 0x3C4): the target keeps one more value in a saved register
 * (s0-s4: the queue size bpo * frames survives the fmax and fmin profile calls in s0) and lays the
 * reset stores out as ControlPitchDP's command 201 (another order); the allocations, the test and
 * the bins' loop are the target's shape. Round 14c: the queue size is an int in the target (no
 * `andi 0xffff`, and `mult s0,s0,s1`, frames first: `unsigned int n` gives both, 238 words against
 * 241); the extra saved register is bpo, which the target converts to float only after the fmin
 * call. Here the first scheduling pass hoists `(float)bpo` -- a pseudo-only insn, which moves
 * across calls -- into a free issue slot before the fmax call (the trace: ready from the entry,
 * ranked last, issued at the first cycle with a slot over), so bpo's register dies there and fmax
 * reuses it. Not found what keeps the target's conversion below the fmin call. The reset stores
 * follow ControlPitchDP's rule (a store that frees its value's register first, then source order). */
static int InitPitchDP(PITCHDP *p)
{
    TOS *tos = p->b.tos;
    unsigned short bpo, fmax, fmin;
    unsigned short n;
    unsigned int i;

    bpo = _tosGetProfileU32(p, 1, 50);
    n = (unsigned char)_tosGetProfileU32(p, 10, 5) * bpo;
    fmax = _tosGetProfileU32(p, 2, 500);
    fmin = _tosGetProfileU32(p, 3, 50);
    p->b.inputs[0].size = n * 4;
    p->b.outputs[0].size = 4;
    p->thresh = (unsigned short)_tosGetProfileU32(p, 6, 10);
    p->maxjump = _tosGetProfileU32(p, 7, 10);
    p->f60 = _tosGetProfileU32(p, 8, 2000);
    p->lookahead = _tosGetProfileU32(p, 4, 15);
    p->overlap = _tosGetProfileU32(p, 5, 6);
    p->ncols = p->lookahead + p->overlap * 2;
    p->f64 = _tosGetProfileFloat(p, 9, 0.01f);
    p->block = 1.0f;
    p->fresh = 1;
    p->last = p->lookahead + p->overlap;
    p->len = p->lookahead + p->overlap;
    p->unflushed = 1;
    p->flushed = 0;
    p->col = 0;
    p->flushing = 0;
    p->nflush = 0;
    p->first = 0;
    p->nbins = bpo * logf((float)fmax / fmin) / logf(2.0f);
    p->cells = heap_Calloc(tos->heap, p->nbins * p->ncols, 12);
    p->rows = heap_Calloc(tos->heap, p->nbins, 4);
    p->freqs = heap_Calloc(tos->heap, p->nbins, 4);
    p->jf = heap_Calloc(tos->heap, p->nbins, 4);
    p->track = heap_Calloc(tos->heap, p->lookahead + p->overlap * 2, 4);
    if (p->cells == 0 || p->rows == 0 || p->jf == 0 || p->freqs == 0 || p->track == 0) {
        _tosErrorLog(p, 2);
        return 1;
    }
    for (i = 0; i < p->nbins; i++) {
        p->rows[i] = p->cells + p->ncols * i;
        p->freqs[i] = fmin * powf(2.0f, i / (float)bpo);
        p->jf[i] = heap_Calloc(tos->heap, p->nbins, 4);
        if (p->jf[i] == 0) {
            _tosErrorLog(p, 2);
            return 1;
        }
    }
    InitJumpFactors(p);
    return 0;
}

TOSBLOCK *ConstructPitchDP(TOS *tos, unsigned char id)
{
    return tosBaseBlockConstruct(tos, id, 1, 1, (TOSFUNC)ProcessPitchDP, (TOSFUNC)InitPitchDP,
                                 (TOSFUNC)ControlPitchDP, sizeof(PITCHDP));
}






