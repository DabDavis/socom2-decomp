/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing -funroll-loops -ffast-math -fno-inline */
/* triggerlr: the recogniser's speech trigger block (OVL2 0x505D90-0x506D94, SOCOM 1's object at
 * 0x370C90: TriggerLR_FindSpeech, ProcessTriggerLR, TriggerLR_Reset, ControlTriggerLR,
 * InitTriggerLR -- all LOCAL there -- and ConstructTriggerLR; SOCOM II has no row for
 * TriggerLR_Reset, whose statements its control function holds), ScanSoft's speech recogniser,
 * prebuilt with ee-gcc 2.96 at -O2 -G0 -fno-strict-aliasing -funroll-loops -ffast-math
 * -fno-inline (README "The GCC front"): the speech search's max.s are -ffast-math's (without it
 * `a < b ? b : a` is a compare and a branch-likely move); -fno-inline is round 26c's (the input-queue
 * commands below). Names are SOCOM 1's (s1:align, the demo's object order); the block's
 * fields are named by their use, the voicing measure's (+276 on) being voicing.c's VOICING and the
 * histogram's (+232 on) slhisto.c's HISTOHOST. SOCOM II adds a field at +64 (the speech's voiced
 * flag, which gates command 202): SOCOM 1's fields from +64 on are 4 lower. Round 24c wrote
 * ConstructTriggerLR (over its draft) and ProcessTriggerLR; round 25c the speech search, the
 * control (SOCOM 1's TriggerLR_Reset in it, and two session commands SOCOM 1 does not have) and
 * the init; round 26c closed ProcessTriggerLR (the object whole). */
#include "tosblk.h"

typedef struct TRIGGERLR {
    TOSBLOCK b;
    unsigned int nframes;               /* +40: the input frames counted */
    unsigned int start;                 /* +44: the speech's first frame, nframes - lead */
    int split;                          /* +48: report the start as command 203 (else 200); profile 31 */
    int restart;                        /* +52: the reset restarts the inputs (param 0) */
    int keep;                           /* +56: keep the inputs running at the start (param 1) */
    int armed;                          /* +60: the end not yet reported */
    int voiced;                         /* +64: SOCOM II's: a voiced frame seen since the speech began */
    unsigned int f68;                   /* +68: f72 / 10; profile 17 */
    unsigned int f72;                   /* +72: param 2 */
    unsigned int maxlead;               /* +76: the lead's ceiling; profile 4 */
    unsigned int f80;                   /* +80: frames under the threshold */
    unsigned int f84;                   /* +84: profile 3 */
    unsigned int f88;                   /* +88: f92 / 10; profile 1 */
    unsigned int f92;                   /* +92: param 4 */
    unsigned int f96;                   /* +96: frames over the threshold */
    unsigned int f100;                  /* +100: voiced frames in a row */
    unsigned int f104;                  /* +104: profile 2 */
    unsigned int lead;                  /* +108: the frames seen before the speech, up to maxlead */
    int f112;                           /* +112: param 5, hundredths of a dB; profile 5 */
    float f116;                         /* +116: the level's floor */
    float f120;                         /* +120: log profile 8 */
    float f124;                         /* +124: log profile 9 */
    float f128;                         /* +128: log profile 10 */
    float f132;                         /* +132: the peak of in[2] */
    float f136;                         /* +136: |profile 6| dB as a natural log */
    float f140;                         /* +140 */
    int state;                          /* +144: 0 before the speech, 1 in it, 2 its end reported */
    int found;                          /* +148: the search's state (3: speech) */
    float f152;                         /* +152: the histogram's quantile; profile 15 */
    float f156;                         /* +156: the peak of in[3] */
    float f160;                         /* +160: the histogram's lower quantile */
    unsigned int f164;                  /* +164: profile 16 */
    int f168;                           /* +168 */
    int f172;                           /* +172: the histogram holds values */
    int f176;                           /* +176 */
    int f180;                           /* +180 */
    int f184;                           /* +184 */
    float *f188;                        /* +188: three voicing maxima, a ring */
    float *f192;                        /* +192: its end */
    float *f196;                        /* +196: its cursor */
    float f200;                         /* +200 */
    unsigned int f204;                  /* +204: param 3; profile 7 */
    unsigned int f208;                  /* +208: profile 18 */
    unsigned int f212;                  /* +212: profile 19 */
    int f216;                           /* +216: profile 20 */
    int f220;                           /* +220: profile 21 */
    float f224;                         /* +224: profile 22 */
    float f228;                         /* +228: profile 23 */
    char pad232[112];
    int ssnin;                          /* +344: the subsampler's input frame (subsmp.c) */
    char pad348[64];
} TRIGGERLR;                            /* 412 bytes (ConstructTriggerLR's) */

extern int Voicing_AddSignal(void *v, short *in);
extern float Voicing_GetMaxVoicing(void *v);
extern void Voicing_MaintainNoiseEner(void *v);
extern void Voicing_Reset(void *v);
extern void Voicing_Free(void *v);
extern int InitVoicing(void *v);
extern void SlidingHisto_NewItem(void *h, float x);
extern int SlidingHisto_Init(void *h);
extern float SlidingHisto_LowerQuantile(void *h, float q);
extern void SlidingHisto_Clear(void *h);
extern void SlidingHisto_Free(void *h);
extern int func_00504E48(void *h);
extern void func_00504E58(void *h, void *s);
extern void func_00504F70(void *h, void *s);
extern unsigned char qQueueControl(void *q, unsigned char cmd, int arg, int n);
extern unsigned char _tosControl(void *log, unsigned char cmd, int arg, int n);
extern float logf(float);
extern int abs(int);

/* the scheduler's input-queue commands as GNU extern inlines -- tos.c's own (which inlines them into
 * tosBaseBlockControl and defines their real copies at their place), so the vendor's scheduler
 * header -- and this object is the only block that calls them. Under -fno-inline they are never
 * inlined here, and no copy is emitted, but cc1 still compiles each body for inlining, and 2.96
 * marks a function it has compiled nothrow (TREE_NOTHROW, set where a deferred inline is saved),
 * so every call to them carries `REG_EH_REGION 0`: ProcessTriggerLR's keep test then keeps its
 * stolen `li v0,1` unannulled before the disable call (`bnez`, the target's; `bnezl` with the
 * functions only declared -- round 25c's one word). Measured (round 26c): without -fno-inline the
 * calls are inlined (0x1CC); a body the inliner refuses (varargs) is "only declared" and gives no
 * note; with all three bodies or the disable alone, the object is byte-exact. */
extern __inline__ unsigned char tosBaseBlockStopInputQueues(TOSBLOCK *b)
{
    unsigned char err = 0;
    unsigned int i;

    for (i = 0; i < b->ninputs; i++)
        err |= !qQueueControl(b->inputs[i].q, 6, b->inputs[i].reader, 0);
    return err;
}

extern __inline__ unsigned char tosBaseBlockDisableInputQueues(TOSBLOCK *b)
{
    unsigned char err = 0;
    unsigned int i;

    for (i = 0; i < b->ninputs; i++)
        if (b->inputs[i].q != 0)
            err |= !qQueueControl(b->inputs[i].q, 8, b->inputs[i].reader, 0);
    return err;
}

extern __inline__ unsigned char tosBaseBlockStartInputQueues(TOSBLOCK *b, int n)
{
    unsigned char err = 0;
    unsigned char cmd = n ? 7 : 5;
    unsigned int i;

    for (i = 0; i < b->ninputs; i++)
        if (b->inputs[i].q != 0)
            err |= !qQueueControl(b->inputs[i].q, cmd, b->inputs[i].reader, n);
    return err;
}

#define MAX(a, b) ((a) > (b) ? (a) : (b))

/* the median of three */
#define MEDIAN3(x, m) do { \
        float a_ = (x)[0], b_ = (x)[1], lo_, hi_; \
        if (a_ < b_) { lo_ = a_; hi_ = b_; } else { lo_ = b_; hi_ = a_; } \
        if (hi_ < (x)[2]) (m) = hi_; else if (lo_ < (x)[2]) (m) = (x)[2]; else (m) = lo_; \
    } while (0)

/* the speech search, one frame: in[1] the energies (its [3] the level), in[2] and in[3] two
 * measures of it; a state machine over `found` -- 0 silence, 1 a rise, 2 its fall, 3 speech, 4 the
 * speech's fall -- with the voicing maxima's median and the level's histogram. SOCOM 1's body
 * (its ring of maxima starts at -10000, SOCOM II's at -2^31), plus SOCOM II's voiced flag and the
 * command 202 it gates. Measured (round 25c): the level's MAX after the two measures' reads
 * (sched1's order; with it first, p->f116's load overlaps p->f136's in local-alloc and takes f1:
 * 7 words; 720 orders of the head, ccperm), thr's difference an operand of its own (through thr
 * itself, 2 words); each `MAX(x, field)` is the target's max.s operand order (rs the field); a
 * pair of constant flag stores is written in the reverse of the target's order (the last store of
 * a constant, its REG_DEAD, issues first: found/f96, f168/f172, found/armed, found/f80), operand
 * 0 of a compare loads first (`lead > f84`, `f132 > f128`), and the median reads its third value
 * in the tests, after the first pair's join (read into a local with the pair, it loads before the
 * first compare and a nop takes its place at the join). */
static int TriggerLR_FindSpeech(TRIGGERLR *p, float **in)
{
    int ret = 0;
    float lev, v2, v3, e0, thr, med;

    lev = in[1][3] + p->f136;
    v2 = *in[2];
    v3 = *in[3];
    lev = MAX(lev, p->f116);
    e0 = in[1][0];
    if (p->f172 == 0)
        p->f160 = lev + p->f140;
    thr = MAX(p->f160 - p->f140, lev);
    switch (p->found) {
    case 0:
        if (p->lead > p->f84)
            p->lead = p->f84;
        if (p->f124 < v2) {
            p->found = 1;
            p->f96 = 1;
            p->f168 = 0;
            p->f176 = 0;
            p->f180 = 0;
            p->voiced = 0;
            p->f80 = 0;
            p->f100 = thr <= e0;
            p->f156 = v3;
            p->f132 = v2;
            if (p->state == 0) {
                p->f188[0] = -2147483648.0f;
                p->f188[1] = -2147483648.0f;
                p->f188[2] = -2147483648.0f;
                p->f196 = p->f188;
                if (thr <= e0)
                    *p->f196 = Voicing_GetMaxVoicing(p);
                p->f184 = 0;
            }
        } else {
            if (p->armed != 0 && p->keep != 0)
                p->f80++;
            if (p->state == 0)
                Voicing_MaintainNoiseEner(p);
        }
        break;
    case 1:
        if (p->f184 == 0) {
            p->f196++;
            if (p->f196 >= p->f192)
                p->f196 = p->f188;
            *p->f196 = -2147483648.0f;
        }
        if (p->f120 < v2) {
            p->f156 = MAX(v3, p->f156);
            p->f132 = MAX(v2, p->f132);
            p->f96++;
            if (thr <= e0) {
                p->voiced = 1;
                p->f100++;
            } else
                p->f100 = 0;
            p->f180 = p->f180 != 0 || p->f100 >= p->f104;
            if (thr <= e0 && p->f184 == 0) {
                *p->f196 = Voicing_GetMaxVoicing(p);
                MEDIAN3(p->f188, med);
                if (p->f200 < med)
                    p->f184 = 1;
            }
            p->f176 = p->f176 != 0 || (p->f96 >= p->f88 && p->f132 > p->f128 && lev <= p->f156);
            if (p->f96 >= p->f164 && p->f176 != 0) {
                SlidingHisto_NewItem(p, p->f156);
                p->f168 = 1;
                p->f172 = 1;
                p->f160 = SlidingHisto_LowerQuantile(p, p->f152);
                p->f96 = p->f88;
                p->f156 = v3;
            }
            if (p->f176 != 0 && p->f180 != 0 && p->f184 != 0) {
                p->found = 3;
                p->armed = 1;
            }
        } else {
            p->found = 2;
            p->f80 = 1;
        }
        break;
    case 2:
        if (p->f184 == 0) {
            p->f196++;
            if (p->f196 >= p->f192)
                p->f196 = p->f188;
            *p->f196 = -2147483648.0f;
        }
        if (p->f124 < v2) {
            p->f132 = v2;
            p->f156 = MAX(v3, p->f156);
            p->found = 1;
            p->f96 = 1;
            p->f176 = 0;
            p->f180 = 0;
            p->f100 = thr <= e0;
            if (thr <= e0 && p->f184 == 0) {
                *p->f196 = Voicing_GetMaxVoicing(p);
                MEDIAN3(p->f188, med);
                if (p->f200 < med)
                    p->f184 = 1;
            }
        } else {
            p->f80++;
            if (p->f80 > p->f84) {
                p->found = 0;
                if (p->voiced != 0 && _tosControl(p, 202, 70, p->nframes) != 0)
                    ret = 1;
                else if (p->f168 == 0 && p->f176 != 0) {
                    SlidingHisto_NewItem(p, p->f156);
                    p->f168 = 1;
                    p->f172 = 1;
                    p->f160 = SlidingHisto_LowerQuantile(p, p->f152);
                }
            }
        }
        break;
    case 3:
        if (p->f120 < v2) {
            p->f96++;
            p->f156 = MAX(v3, p->f156);
            if (p->f96 >= p->f164) {
                SlidingHisto_NewItem(p, p->f156);
                p->f168 = 1;
                p->f172 = 1;
                p->f160 = SlidingHisto_LowerQuantile(p, p->f152);
                p->f96 = 0;
                p->f156 = v3;
            }
        } else {
            p->found = 4;
            p->f80 = 1;
        }
        break;
    case 4:
        if (p->f124 < v2) {
            p->found = 3;
            p->f96++;
            p->f156 = MAX(v3, p->f156);
        } else {
            p->f80++;
            if (p->f80 > p->f84) {
                p->found = 0;
                if (p->f168 == 0) {
                    SlidingHisto_NewItem(p, p->f156);
                    p->f168 = 1;
                    p->f172 = 1;
                    p->f160 = SlidingHisto_LowerQuantile(p, p->f152);
                }
            }
        }
        break;
    }
    return ret;
}

/* one frame of the four inputs: its signal into the voicing measure while no speech is found (the
 * lead counted up to its ceiling), then the speech search; at the speech's start the start frame
 * reported (command 203 or 200, after stopping the first input unless it is kept -- the inputs
 * then disabled, else the state 1), and in the speech the end reported once (command 201, the
 * state 2). SOCOM 1's body, void as SOCOM 1's (whose compile makes the disable call a sibling
 * call; 2.96 keeps it a `jal` in its nested arm, measured -- the target's); the end test
 * `f80 > f68` (operand 0 expanded first: the target loads +80 first). Round 24c had it int (falling off the end, its
 * `(clobber (reg v0))` kept the two state stores from cross-jumping: 0x194); void they merge as
 * the target's. The keep test's `bnez` (its stolen `li v0,1` unannulled before the disable call)
 * is the disable call's nothrow note: round 25c's last word, closed in round 26c by the extern
 * inline above and -fno-inline (the note's mechanism: round 25c, build/scratch/gcc-r25c/ann/). */
static void ProcessTriggerLR(TRIGGERLR *p, void **in)
{
    if (in[0] != 0) {
        p->nframes++;
        if (p->state == 0) {
            if (Voicing_AddSignal(p, in[0]) != 0)
                return;
            p->lead++;
            if (p->lead > p->maxlead)
                p->lead = p->maxlead;
        }
        if (TriggerLR_FindSpeech(p, (float **)in) != 0)
            return;
        if (p->state == 0) {
            if (p->found == 3) {
                p->start = p->nframes - p->lead;
                if (p->keep == 0 && qQueueControl(p->b.inputs->q, 8, p->b.inputs->reader, 0) == 0)
                    return;
                if (p->split != 0) {
                    if (_tosControl(p, 203, p->lead, p->nframes) != 0)
                        return;
                } else if (_tosControl(p, 200, p->lead, p->nframes) != 0)
                    return;
                if (p->keep == 0)
                    tosBaseBlockDisableInputQueues(&p->b);
                else
                    p->state = 1;
            }
        } else if (p->state == 1) {
            if (p->armed != 0 && p->f80 > p->f68) {
                p->armed = 0;
                if (_tosControl(p, 201, p->nframes, 0) == 0)
                    p->state = 2;
            }
        }
    }
}

/* the histogram and the voicing measure cleared (a null session) -- a statement macro: the object
 * is built with -fno-inline, so a function would be called; a plain block (a `do { } while (0)`
 * is a loop to the scheduler, its notes a barrier: 19 words) */
#define TRIGGERLR_CLEAR_SESSION(p) { \
        SlidingHisto_Clear(p); \
        Voicing_Reset(p); \
        (p)->f172 = 0; \
    }

/* the commands: 100 the reset (the inputs restarted, or the first three stopped when the inputs
 * are kept, else all disabled), 101 the start frame's address, 102 and 103 the six parameters
 * out and in, 104 the start's report, 105 and 106 a session in and out (the histogram flag and
 * the histogram's ring after it), 255 the block destroyed. The reset's first statements are SOCOM
 * 1's TriggerLR_Reset (the counters and the search's state cleared, the voicing measure reset),
 * written in place: under -fno-inline a helper would stay a call, and SOCOM II has no row for it.
 * The session in reads its offset before the clears (round 25c: the offset and `arg` live apart
 * across them, the sum formed after -- the target's two saved registers). */
static int ControlTriggerLR(TRIGGERLR *p, unsigned char cmd, void *arg, unsigned int *off)
{
    int err;
    float t;
    char *s;
    unsigned int o;

    switch (cmd) {
    case TOS_RESET:
        p->nframes = 0;
        p->start = 0;
        p->lead = 0;
        p->f96 = 0;
        p->f80 = 0;
        p->armed = 0;
        p->f184 = 0;
        p->found = 0;
        Voicing_Reset(p);
        if (p->restart != 0) {
            p->state = 0;
            if (tosBaseBlockStartInputQueues(&p->b, 0) != 0)
                return 0;
        } else if (p->keep != 0) {
            p->state = 1;
            err = qQueueControl(p->b.inputs[1].q, 5, p->b.inputs[1].reader, 0) == 0;
            err |= qQueueControl(p->b.inputs[2].q, 5, p->b.inputs[2].reader, 0) == 0;
            err |= qQueueControl(p->b.inputs[3].q, 5, p->b.inputs[3].reader, 0) == 0;
            if (err)
                return 0;
        } else {
            p->state = 2;
            if (tosBaseBlockDisableInputQueues(&p->b) != 0)
                return 0;
        }
        break;
    case 103:
        p->restart = ((int *)arg)[0];
        p->keep = ((int *)arg)[1];
        p->f204 = ((unsigned int *)arg)[3];
        t = (float)(p->f204 - p->f208) / (float)(p->f212 - p->f208);
        p->f140 = (p->f216 + (p->f220 - p->f216) * t) * 0.2302585f;
        p->f200 = logf(p->f224 + (p->f228 - p->f224) * t);
        p->f72 = ((unsigned int *)arg)[2];
        p->f92 = ((unsigned int *)arg)[4];
        p->f68 = p->f72 / 10;
        p->f88 = p->f92 / 10;
        p->f112 = ((int *)arg)[5];
        p->f116 = p->f112 * 0.23025851f * 0.01f + 27.37f;
        break;
    case 102:
        ((int *)arg)[0] = p->restart;
        ((int *)arg)[1] = p->keep;
        ((unsigned int *)arg)[2] = p->f72;
        ((unsigned int *)arg)[3] = p->f204;
        ((unsigned int *)arg)[4] = p->f92;
        ((int *)arg)[5] = p->f112;
        break;
    case 101:
        *(unsigned int **)arg = &p->start;
        break;
    case 104:
        p->split = (int)arg;
        break;
    case 105:
        if (arg != 0) {
            o = *off;
            TRIGGERLR_CLEAR_SESSION(p);
            p->f172 = *(int *)((char *)arg + o);
            func_00504E58(p, (char *)arg + o + 4);
            if (p->f172 != 0)
                p->f160 = SlidingHisto_LowerQuantile(p, p->f152);
            *off += func_00504E48(p) + 4;
        } else
            TRIGGERLR_CLEAR_SESSION(p);
        break;
    case 106:
        if (arg != 0) {
            s = (char *)arg + *off;
            *(int *)s = p->f172;
            func_00504F70(p, s + 4);
        }
        *off += func_00504E48(p) + 4;
        break;
    case TOS_DESTROY: {
        TOS *tos = p->b.tos;

        if (p->f188 != 0)
            heap_Free(tos->heap, p->f188);
        SlidingHisto_Free(p);
        Voicing_Free(p);
        tosBaseBlockDestruct(&p->b);
        break;
    }
    default:
        return 0;
    }
    return 1;
}

/* the profile read: the level's floor and thresholds (logs), the voicing threshold interpolated
 * over the parameters' range (profiles 7, 18-23), the histogram, a ring of three voicing maxima,
 * the voicing measure, the counts */
static int InitTriggerLR(TRIGGERLR *p)
{
    TOS *tos = p->b.tos;
    int ret = 0;
    float t;

    p->b.inputs[2].size = 4;
    p->b.inputs[3].size = 4;
    p->b.inputs[1].size = 24;
    p->f88 = _tosGetProfileU32(p, 1, 6);
    p->f92 = p->f88 * 10;
    p->f104 = _tosGetProfileU32(p, 2, 3);
    p->f84 = _tosGetProfileU32(p, 3, 15);
    p->maxlead = _tosGetProfileU32(p, 4, 45);
    p->f112 = _tosGetProfileU32(p, 5, -7200);
    p->split = _tosGetProfileU32(p, 31, 0);
    p->f116 = p->f112 * 0.23025851f * 0.01f + 27.37f;
    p->f120 = logf(_tosGetProfileFloat(p, 8, 1.0f));
    p->f124 = logf(_tosGetProfileFloat(p, 9, 5.0f));
    p->f128 = logf(_tosGetProfileFloat(p, 10, 100.0f));
    p->f136 = abs(_tosGetProfileU32(p, 6, 17)) * 0.2302585f;
    p->f208 = _tosGetProfileU32(p, 18, 0);
    p->f212 = _tosGetProfileU32(p, 19, 100);
    p->f216 = _tosGetProfileU32(p, 20, 5);
    p->f220 = _tosGetProfileU32(p, 21, 25);
    p->f224 = _tosGetProfileFloat(p, 22, 0.75f);
    p->f228 = _tosGetProfileFloat(p, 23, 0.65f);
    p->f204 = _tosGetProfileU32(p, 7, 50);
    t = (float)(p->f204 - p->f208) / (float)(p->f212 - p->f208);
    p->f140 = (p->f216 + (p->f220 - p->f216) * t) * 0.2302585f;
    p->f200 = logf(p->f224 + (p->f228 - p->f224) * t);
    p->f152 = _tosGetProfileFloat(p, 15, 0.5f);
    p->f164 = _tosGetProfileU32(p, 16, 100);
    p->f172 = 0;
    if (SlidingHisto_Init(p) != 0)
        ret = 1;
    else {
        p->f188 = heap_Calloc(tos->heap, 3, sizeof(float));
        if (p->f188 == 0) {
            _tosErrorLog(p, 2);
            ret = 1;
        } else {
            p->f196 = p->f188;
            p->f192 = p->f188 + 3;
            if (InitVoicing(p) != 0)
                ret = 1;
            else {
                p->b.inputs[0].size = p->ssnin * sizeof(short);
                p->restart = 1;
                p->keep = 1;
                p->f68 = _tosGetProfileU32(p, 17, 50);
                p->f72 = p->f68 * 10;
            }
        }
    }
    return ret;
}

TOSBLOCK *ConstructTriggerLR(TOS *tos, unsigned char id)
{
    return tosBaseBlockConstruct(tos, id, 4, 0, (TOSFUNC)ProcessTriggerLR, (TOSFUNC)InitTriggerLR,
                                 (TOSFUNC)ControlTriggerLR, sizeof(TRIGGERLR));
}
