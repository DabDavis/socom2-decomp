/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing -funroll-loops */
/* pitchcomb: the recogniser's pitch combiner block (OVL2 0x501A90-, SOCOM 1's object at 0x36CE48:
 * LogPitchProcessing, NormE_Processing, FlushPitchCombiner, ProcessPitchCombiner,
 * PitchCombinerReset, InitPitchCombiner, ConstructPitchCombiner, ControlPitchCombiner -- all but the
 * constructor LOCAL there; SOCOM II moved the control function before the init),
 * ScanSoft's speech recogniser, prebuilt with ee-gcc 2.96 at -O2 -G0 -fno-strict-aliasing
 * -funroll-loops (README "The GCC front"). Names are SOCOM 1's; the block's fields are named by
 * their use. NormE_Processing (round 17c), ControlPitchCombiner (round 22c: SOCOM 1's body,
 * 0x5028F0 -- the constructor's control slot, moved before InitPitchCombiner in SOCOM II),
 * FlushPitchCombiner, ProcessPitchCombiner and ConstructPitchCombiner (round 23c) and
 * LogPitchProcessing (round 24c) are written, PitchCombinerReset OPEN; InitPitchCombiner is not
 * written.
 * The vendor halves the lead (+156) as `p->f156 / 2` (round 24c): the C front end shortens the
 * division to the unsigned char's own type, so its load is another RTL expression than the
 * zero-extended read a compare or a `>> 1` makes -- CSE keeps the two apart, which is the re-read
 * of +156 the flush and the log pass show (round 23c's LEAD_STANDIN, a volatile field, is gone:
 * the field is plain, as the log pass's slot-filled loads of it say it is). */
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
    float f144;                         /* +144: the last frame's difference r60 - r112 */
    float *f148;                        /* +148: the log pass's weights */
    float *weights;                     /* +152: the window's weights */
    unsigned char f156;                 /* +156: r96's and r112's lead, doubled */
    unsigned char nwin;                 /* +157: the window's frames */
    unsigned char nring;                /* +158: the rings' length */
    unsigned char f159;
    unsigned char f160;                 /* +160: frames still to flush */
    unsigned char f161;
    unsigned char f162;
    unsigned char f163;
    unsigned char f164;
    unsigned char fill;                 /* +165: the head of the ring is still to be filled */
    unsigned short f166;                /* +166: the log pass's norm */
    unsigned short norm;                /* +168 */
    char pad170[2];
    float f172;                         /* +172: r112's last frame (the run's mean) */
    float f176;                         /* +176: r96's last frame (the run's weighted mean) */
    char pad180[2];
    unsigned char f182;                 /* +182: set by command 201 */
    unsigned char f183;
    unsigned char f184;
    unsigned char f185;
    unsigned short nvals;               /* +186: the frame's values (in[2]'s, from [1]) */
    float f188;                         /* +188: the output's fifth value when r80's frame is 1 */
    float f192;                         /* +192: else */
} PITCHCOMB;                            /* 196 bytes (ConstructPitchCombiner's) */

float logf(float x);

/* the voiced run's end: when the run's frames reach the lead, r96 takes the run's weighted mean
 * (the +148 weights over the +166 norm) and r112 its mean, from the window start at +76, and a run
 * begun at the lead (+163) back-fills both rings' last half-lead of frames with the frame before;
 * else both rings step on and a run ended at the lead (+164) fills the half-lead with the last
 * means. SOCOM 1's body: the two sums and the two counters (an unsigned char, an unsigned short)
 * apart, each quotient back into its sum (the target divides into the sum's register); the back-
 * fill's `d = s - 1` after the wrap test (reorg's steal from the join duplicates it into the
 * branch's slot and the arm -- two assignments in m2c's reading); r112's start in the else arm
 * if-converted (its end is in a register from the wrap test) where r96's reloads; the back-fill
 * loops re-read +156 each pass (the float stores may alias it: -fno-strict-aliasing).
 * Round 24c, byte-exact: the half-lead bounds are `p->f156 / 2` (see the unit's note: as `>> 1` the
 * fill loops' first bound reuses the entry test's read, which then stays live as a QImode value --
 * 0x384); the second sum's bound is read in the loop's own test, which loop.c hoists (through a
 * local the division after the loop reuses its register); and the two fill blocks' pointers are
 * block-local -- the back-fill's `s` and `d`, the fill's own `s` (one function-wide `s` for both
 * arms outranks nothing: p, the highest priority, takes a1 first and the target's p is a2 -- every
 * grouping of the six pointer roles was scored, build/scratch/gcc/r24c/v_roles.py: the fill's
 * cursor must be a variable of its own, each such grouping matches). */
static void LogPitchProcessing(PITCHCOMB *p)
{
    float *e;
    float *w;
    float sum, sum2;
    unsigned char i;
    unsigned short j;
    unsigned char k;
    unsigned char n;

    if (p->f159 == p->f156) {
        p->f185 = 1;
        e = p->f76;
        w = p->f148;
        sum = 0;
        for (i = 0; i < p->f159; i++) {
            sum += *e++ * *w++;
            if (e == p->r60.end)
                e = p->r60.base;
        }
        sum /= p->f166;
        *p->r96.wr = sum;
        p->f176 = *p->r96.wr++;
        if (p->r96.wr == p->r96.end)
            p->r96.wr = p->r96.base;
        e = p->f76;
        sum2 = 0;
        for (j = 0; j < p->f156; j++) {
            sum2 += *e++;
            if (e == p->r60.end)
                e = p->r60.base;
        }
        sum2 /= p->f156;
        *p->r112.wr = sum2;
        p->f172 = *p->r112.wr++;
        if (p->r112.wr == p->r112.end)
            p->r112.wr = p->r112.base;
        if (p->f163 == 1) {
            float *s;
            float *d;
            s = p->r112.wr - 1;
            if (s < p->r112.base)
                s = p->r112.end - 1;
            d = s - 1;
            if (d < p->r112.base)
                d = p->r112.end - 1;
            for (k = 0; k < p->f156 / 2; k++) {
                *d-- = *s;
                if (d < p->r112.base)
                    d = p->r112.end - 1;
            }
            s = p->r96.wr - 1;
            if (s < p->r96.base)
                s = p->r96.end - 1;
            d = s - 1;
            if (d < p->r96.base)
                d = p->r96.end - 1;
            for (k = 0; k < p->f156 / 2; k++) {
                *d-- = *s;
                if (d < p->r96.base)
                    d = p->r96.end - 1;
            }
            p->f163 = 0;
        }
        p->f159--;
    } else {
        p->r96.wr++;
        if (p->r96.wr == p->r96.end)
            p->r96.wr = p->r96.base;
        p->r112.wr++;
        if (p->r112.wr == p->r112.end)
            p->r112.wr = p->r112.base;
        if (p->f164 == 1) {
            float *s;
            s = p->r112.wr - 1;
            if (s < p->r112.base)
                s = p->r112.end - 1;
            for (k = 0; k < p->f156 / 2; k++) {
                *s++ = p->f172;
                if (s == p->r112.end)
                    s = p->r112.base;
            }
            s = p->r96.wr - 1;
            if (s < p->r96.base)
                s = p->r96.end - 1;
            for (k = 0; k < p->f156 / 2; k++) {
                *s++ = p->f176;
                if (s == p->r96.end)
                    s = p->r96.base;
            }
            p->f164 = 0;
        }
    }
}

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

/* one frame of the flush: the lagging rings (r96, r112: when the frames left reach the lead, padded
 * with their last frame) and the output (padded likewise, through a cursor of its own) caught up once,
 * then a frame out of the rings' read cursors -- the r60 - r112 difference, r96's, the energy's and
 * the output's frames scaled, r80's choice of +188/+192 -- and every read cursor stepped; 1 while
 * frames are left, else the flag at +182 cleared and 0. SOCOM 1's body. Measured: each padding
 * loop's cursor stepped in the store (`*p->r96.wr++ = *s`: the field's address taken once, as the
 * target's `addiu t0,s0,104`), the count an int narrowed at the loop's test (the target's `andi
 * 0xffff` after the wrap test's join), the frame written through `*q++` and its last value one
 * store at the join (a conditional expression).
 * Round 23c: the target -- SOCOM 1's copy too -- reads +156 again for the count after the lead
 * test: the count is `p->f156 / 2` (round 24c; the unit's note -- round 23c's volatile stand-in is
 * gone; as `>> 1` CSE reuses the test's read, 2 words short). The padding blocks' locals
 * are the function's, declared at its top (C89): ONE s, len and i for all three blocks -- i then
 * spans three loops and outranks s in global-alloc (the target's i in a2, s in a3, len in a0 in
 * every block; per-block locals rank s first, 36 words; every order of the five declarations
 * alike); the output block computes its count before s (the nwin load ahead of `s = w - 1`) */
static int FlushPitchCombiner(PITCHCOMB *p)
{
    float *q;
    float *s;
    float *w;
    int len;
    unsigned short i;

    if (p->f160 != 0) {
        if (p->f183 != 0) {
            if (p->f160 >= (unsigned char)(p->f156 - 1)) {
                s = p->r96.wr - 1;
                len = p->f156 / 2;
                if (s < p->r96.base)
                    s = p->r96.end - 1;
                for (i = 0; i < (unsigned short)len; i++) {
                    *p->r96.wr++ = *s;
                    if (p->r96.wr == p->r96.end)
                        p->r96.wr = p->r96.base;
                }
                s = p->r112.wr - 1;
                if (s < p->r112.base)
                    s = p->r112.end - 1;
                for (i = 0; i < (unsigned short)len; i++) {
                    *p->r112.wr++ = *s;
                    if (p->r112.wr == p->r112.end)
                        p->r112.wr = p->r112.base;
                }
            }
            p->f183 = 0;
        }
        if (p->f184 != 0) {
            w = p->out.wr;
            len = p->nwin >> 1;
            s = w - 1;
            if (s < p->out.base)
                s = p->out.end - 1;
            for (i = 0; i < (unsigned short)len; i++) {
                *w++ = *s;
                if (w == p->out.end)
                    w = p->out.base;
            }
            p->f184 = 0;
        }
        p->f144 = *p->r60.rd - *p->r112.rd;
        q = qEnQueueOne(p->b.outputs->q);
        *q++ = p->f144 * 32.0f;
        *q++ = *p->r96.rd * 53.0f;
        *q++ = *p->e.rd * 1.107;
        *q++ = *p->out.rd * 0.832;
        *q = *p->r80.rd == 1.0f ? p->f188 : p->f192;
        p->r60.rd++;
        if (p->r60.rd == p->r60.end)
            p->r60.rd = p->r60.base;
        p->r112.rd++;
        if (p->r112.rd == p->r112.end)
            p->r112.rd = p->r112.base;
        p->r96.rd++;
        if (p->r96.rd == p->r96.end)
            p->r96.rd = p->r96.base;
        p->e.rd++;
        if (p->e.rd == p->e.end)
            p->e.rd = p->e.base;
        p->out.rd++;
        if (p->out.rd == p->out.end)
            p->out.rd = p->out.base;
        p->r80.rd++;
        if (p->r80.rd == p->r80.end)
            p->r80.rd = p->r80.base;
        p->f160--;
        if (p->f160 != 0)
            return 1;
    }
    p->f182 = 0;
    return 0;
}

/* one input frame: the largest of in[2]'s values into the energies' ring, the log of in[0]'s
 * into r60, in[1]'s voicing flag into r80 and the voiced-run bookkeeping (+159 the run's frames,
 * +161 voiced, +162 the last frame's, +163/+164 a run begun/ended at the lead); the log pass once
 * the lead is in, the window's start kept or stepped, the normalisation once a window is in, and
 * once the rings are full a frame out as the flush writes one; a null frame flushes when +182 is
 * set. SOCOM 1's body (logf for its double log). Declared int and returning nothing (the
 * scheduler's TOSFUNC): the null frame's flush is then no sibling call (the target's `jal; nop`
 * under the frame; declared void, 2.96 makes it a `j` -- and the object's constructor does make
 * one, so -fno-optimize-sibling-calls is not the cause). The frame's first value and the scan's
 * pointer in that order (the pointer's addiu ahead of the entry test, the load in its slot), the
 * run's two flags with +161 first (the scheduler issues the last store first) */
static int ProcessPitchCombiner(PITCHCOMB *p, void **in)
{
    float *q;

    if (in[0] != 0) {
        unsigned short n = p->nvals;
        unsigned short i;
        float *f = in[2];
        float max = f[1];
        float *v = f + 2;

        for (i = 1; i < n; i++) {
            if (max < *v)
                max = *v;
            v++;
        }
        *p->e.wr++ = max;
        if (p->e.wr == p->e.end)
            p->e.wr = p->e.base;
        *p->r60.wr++ = logf(*(float *)in[0]);
        if (p->r60.wr == p->r60.end)
            p->r60.wr = p->r60.base;
        *p->r80.wr = *(unsigned char *)in[1];
        p->f160++;
        p->f162 = p->f161;
        if (*p->r80.wr == 1.0f) {
            if (p->f161 == 0) {
                p->f161 = 1;
                p->f163 = 1;
            }
            p->f159++;
        } else if (p->f161 == 1) {
            if (p->f159 == p->f156 - 1)
                p->f164 = 1;
            p->f161 = 0;
            p->f159 = 0;
        }
        p->r80.wr++;
        if (p->r80.wr == p->r80.end)
            p->r80.wr = p->r80.base;
        if (p->f160 >= p->f156)
            LogPitchProcessing(p);
        if (p->f161 == 0)
            p->f76 = p->r60.wr;
        else if (p->f185 == 1) {
            p->f76++;
            if (p->f76 == p->r60.end)
                p->f76 = p->r60.base;
            p->f185 = 0;
        }
        if (p->f160 >= p->nwin)
            NormE_Processing(p);
        if (p->f160 == p->nring) {
            p->f144 = *p->r60.rd - *p->r112.rd;
            q = qEnQueueOne(p->b.outputs->q);
            *q++ = p->f144 * 32.0f;
            *q++ = *p->r96.rd * 53.0f;
            *q++ = *p->e.rd * 1.107;
            *q++ = *p->out.rd * 0.832;
            *q = *p->r80.rd == 1.0f ? p->f188 : p->f192;
            p->r60.rd++;
            if (p->r60.rd == p->r60.end)
                p->r60.rd = p->r60.base;
            p->r112.rd++;
            if (p->r112.rd == p->r112.end)
                p->r112.rd = p->r112.base;
            p->r96.rd++;
            if (p->r96.rd == p->r96.end)
                p->r96.rd = p->r96.base;
            p->e.rd++;
            if (p->e.rd == p->e.end)
                p->e.rd = p->e.base;
            p->out.rd++;
            if (p->out.rd == p->out.end)
                p->out.rd = p->out.base;
            p->r80.rd++;
            if (p->r80.rd == p->r80.end)
                p->r80.rd = p->r80.base;
            p->f160--;
        }
    } else if (p->f182 == 1)
        FlushPitchCombiner(p);
}

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
 * The lead is read once (CSE merges its reads; `/ 2` or `>> 1`, through a local or not, alike).
 * OPEN (round 22c, the right size, 18 words: one store's place): every word is the target's but the
 * last `= 1` flag store, which the target leaves last (in `jr`'s slot) and sched1 here issues before
 * the ring cursors' stores -- the store that is the constant 1's last use (the QImode pseudo's
 * REG_DEAD) outranks the cursor stores, which free their own registers too and then go in source
 * order; whichever of the three 1-flags comes last in the source jumps so (all 36 placements of
 * each, build/scratch/gcc/fd/pc3.py), and a chained assignment, a local `one`, the flags before the
 * cursors or copied from +184 do not stop it (pc4.py).
 * Round 23c, read off cc1's dumps: the store enters sched1 with REG_DEAD on both the 1 and p
 * (weight -2), so under rank_for_schedule's (priority, weight, luid) every source order puts it
 * first; the target's order needs it held back until the other stores issue. `do { p->f183 = 1; }
 * while (0)` does that (its LOOP_BEG note makes the store a scheduling barrier): sched1's output is
 * then the target's, but the barrier stands in sched2 too, where flush_pending_lists gives every
 * load one dependent more and the depend-count tie-break issues each base load before the srl
 * beside it (4 words; with -fno-schedule-insns2 the reset matches, and NormE_Processing and
 * ControlPitchCombiner do not -- sched2 is on). So the vendor's hold is in sched1 only; volatile flags,
 * inlines for the tail or the cursors, a pointer to the flags and the block's own pointer do not
 * give it (build/scratch/gcc/pr1.py). */
static void PitchCombinerReset(PITCHCOMB *p)
{
    int lead;

    ResetRing(&p->r80, p->nring);
    ResetRing(&p->e, p->nring);
    ResetRing(&p->r60, p->nring);
    ResetRing(&p->r112, p->nring);
    ResetRing(&p->r96, p->nring);
    ResetRing(&p->out, p->nring);
    lead = p->f156 >> 1;
    p->r112.wr = p->r112.base + lead;
    p->r96.wr = p->r96.base + lead;
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

/* (InitPitchCombiner 0x502A08 is not written) */
int InitPitchCombiner(PITCHCOMB *p);

TOSBLOCK *ConstructPitchCombiner(TOS *tos, unsigned char id)
{
    return tosBaseBlockConstruct(tos, id, 3, 1, (TOSFUNC)ProcessPitchCombiner,
                                 (TOSFUNC)InitPitchCombiner, (TOSFUNC)ControlPitchCombiner,
                                 sizeof(PITCHCOMB));
}
