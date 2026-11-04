/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing -funroll-loops */
/* pitchcomb: the recogniser's pitch combiner block (OVL2 0x501A90-, SOCOM 1's object at 0x36CE48:
 * LogPitchProcessing, NormE_Processing, FlushPitchCombiner, ProcessPitchCombiner,
 * PitchCombinerReset, InitPitchCombiner, ConstructPitchCombiner, ControlPitchCombiner -- all but the
 * constructor LOCAL there; SOCOM II moved the control function before the init),
 * ScanSoft's speech recogniser, prebuilt with ee-gcc 2.96 at -O2 -G0 -fno-strict-aliasing
 * -funroll-loops (README "The GCC front"). Names are SOCOM 1's; the block's fields are named by
 * their use. NormE_Processing (round 17c), PitchCombinerReset (OPEN) and ControlPitchCombiner
 * (round 22c: SOCOM 1's body, 0x5028F0 -- the constructor's control slot, moved before
 * InitPitchCombiner in SOCOM II) are written. */
#include "tosblk.h"

/* a ring of frames: its start, read and write cursors and end */
typedef struct RING {
    float *base;                        /* +0 */
    float *rd;                          /* +4 */
    float *wr;                          /* +8 */
    float *end;                         /* +12 */
} RING;

typedef struct PITCHCOMB {
    TOSBLOCK b;
    RING e;                             /* +40: the energies' ring */
    float *rd;                          /* +56: the window's first frame in it */
    RING r60;                           /* +60 */
    float *f76;                         /* +76: r60's read cursor at the reset */
    RING r80;                           /* +80 */
    RING r96;                           /* +96 */
    RING r112;                          /* +112 */
    RING out;                           /* +128: the normalised energies' ring */
    int f144;                           /* +144 */
    void *f148;                         /* +148 */
    float *weights;                     /* +152: the window's weights */
    unsigned char f156;                 /* +156: r96's and r112's lead, doubled */
    unsigned char nwin;                 /* +157: the window's frames */
    unsigned char nring;                /* +158: the rings' length */
    unsigned char f159;
    unsigned char f160;
    unsigned char f161;
    unsigned char f162;
    unsigned char f163;
    unsigned char f164;
    unsigned char fill;                 /* +165: the head of the ring is still to be filled */
    char pad166[2];
    unsigned short norm;                /* +168 */
    char pad170[2];
    int f172;                           /* +172 */
    char pad176[6];
    unsigned char f182;                 /* +182: set by command 201 */
    unsigned char f183;
    unsigned char f184;
    unsigned char f185;
    char pad186[10];
} PITCHCOMB;                            /* 196 bytes (ConstructPitchCombiner's) */

/* (LogPitchProcessing 0x501A90 is not written) */

/* the window's weighted energy over the norm into the output ring, the window moved on a frame;
 * at the start the first half-window of frames takes the first value. SOCOM 1's body: the index
 * zeroed at its declaration (the prologue's `move a3,zero`) and the sum loop's own test the only
 * one; the quotient back into the sum (the target divides into $f2); the output stored as
 * `*p->wr++` (one expression: the field's address taken once, the target's `addiu a1,a0,136` --
 * SOCOM 1's store and increment were two statements); the fill's value with the start-of-ring
 * case first (the other arm in the branch-likely slot) */
static void NormE_Processing(PITCHCOMB *p)
{
    unsigned char i = 0;
    float sum = 0;
    float *rd = p->rd;
    float *w;
    float *q;
    float last;

    w = p->weights;
    for (; i < p->nwin; i++) {
        sum += *rd++ * *w++;
        if (rd == p->e.end)
            rd = p->e.base;
    }
    sum /= p->norm;
    *p->out.wr++ = sum;
    if (p->out.wr == p->out.end)
        p->out.wr = p->out.base;
    p->rd++;
    if (p->rd == p->e.end)
        p->rd = p->e.base;
    if (p->fill) {
        if (p->out.wr == p->out.base)
            last = p->out.end[-1];
        else
            last = p->out.wr[-1];
        q = p->out.base;
        for (i = 0; i < p->nwin >> 1; i++)
            *q++ = last;
        p->fill = 0;
    }
}

/* (FlushPitchCombiner and ProcessPitchCombiner, 0x501F38-0x502658, are not written) */

/* a ring emptied: its cursors at its start, its end n frames on, its frames zeroed. The length an
 * int narrowed into a short of its own for the loop (the target's `andi 0xffff` of the loaded byte,
 * which both builds keep; as an unsigned short parameter the conversion folds away, 10 words short) */
static __inline__ void ResetRing(RING *r, int len)
{
    float *b = r->base;
    unsigned short n = len;
    unsigned short i;

    r->wr = b;
    r->end = b + len;
    r->rd = b;
    for (i = 0; i < n; i++)
        r->base[i] = 0;
}

/* the block's state reset: its six rings emptied, the two lagging rings' write cursors half
 * their lead on and the output's half a window, the window at the energies' read cursor, the
 * flags cleared. SOCOM 1's body, each ring's base read once (SOCOM 1's reloads it for the end).
 * Defined above ControlPitchCombiner, whose command 200 calls it: only so is the control's
 * default `move v0,zero` left out of the `beq`'s slot (round 13c's definition-order rule; with the
 * reset only declared, 3 words).
 * OPEN (round 22c, the right size, 18 words: one store's place): every word is the target's but the
 * last `= 1` flag store, which the target leaves last (in `jr`'s slot) and sched1 here issues before
 * the ring cursors' stores -- the store that is the constant 1's last use (the QImode pseudo's
 * REG_DEAD) outranks the cursor stores, which free their own registers too and then go in source
 * order; whichever of the three 1-flags comes last in the source jumps so (all 36 placements of
 * each, build/scratch/gcc/fd/pc3.py), and a chained assignment, a local `one`, the flags before the
 * cursors or copied from +184 do not stop it (pc4.py) */
static void PitchCombinerReset(PITCHCOMB *p)
{
    ResetRing(&p->r80, p->nring);
    ResetRing(&p->e, p->nring);
    ResetRing(&p->r60, p->nring);
    ResetRing(&p->r112, p->nring);
    ResetRing(&p->r96, p->nring);
    ResetRing(&p->out, p->nring);
    p->r112.wr = p->r112.base + (p->f156 >> 1);
    p->r96.wr = p->r96.base + (p->f156 >> 1);
    p->out.wr = p->out.base + (p->nwin >> 1);
    p->f76 = p->r60.rd;
    p->rd = p->e.rd;
    p->f184 = 1;
    p->f185 = 0;
    p->f144 = 0;
    p->f172 = 0;
    p->f159 = 0;
    p->f161 = 0;
    p->f162 = 0;
    p->f160 = 0;
    p->f163 = 0;
    p->f164 = 0;
    p->fill = 1;
    p->f182 = 0;
    p->f183 = 1;
}

/* the block's commands: 200 resets it, 201 sets the flag at +182, 255 frees its eight buffers and
 * destroys it; each answers 1, any other 0. SOCOM 1's body; the cases break to one `return 1`
 * (round 10c's control shape) */
static int ControlPitchCombiner(PITCHCOMB *p, unsigned char cmd)
{
    switch (cmd) {
    case 200:
        PitchCombinerReset(p);
        break;
    case 201:
        p->f182 = 1;
        break;
    case TOS_DESTROY:
        heap_Free(p->b.tos->heap, p->e.base);
        heap_Free(p->b.tos->heap, p->r60.base);
        heap_Free(p->b.tos->heap, p->r80.base);
        heap_Free(p->b.tos->heap, p->r96.base);
        heap_Free(p->b.tos->heap, p->r112.base);
        heap_Free(p->b.tos->heap, p->out.base);
        heap_Free(p->b.tos->heap, p->f148);
        heap_Free(p->b.tos->heap, p->weights);
        tosBaseBlockDestruct(&p->b);
        break;
    default:
        return 0;
    }
    return 1;
}
