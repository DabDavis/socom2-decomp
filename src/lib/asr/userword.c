/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing -funroll-loops -fno-optimize-sibling-calls -ffast-math */
/* userword: the recogniser's user-word training (OVL2 0x4E1B28-0x4E42A4, SOCOM 1's object at 0x34FFE0:
 * userwordAcknowledge .. cbDoUserWordTraining), ScanSoft's recogniser, prebuilt with ee-gcc 2.96 at
 * -O2 -G0 -fno-strict-aliasing -funroll-loops (README "The GCC front") and, this object, without
 * sibling calls: DoLabelStop returns casrEnableLabelInQueue's int through a jal, byte-exact only
 * under -fno-optimize-sibling-calls (10 words off without; DoLabelStart likewise) -- and with
 * -ffast-math (round 25c: CalcF2's min.s and FindBestTranscription's max.s/min.s are its; every
 * function here matches with and without). Only the
 * functions written so far are here, in image order; names are SOCOM 1's (s1:align, s1:order),
 * the context casr.h's (the three training utterances' records at +980: round 13c), the rest of
 * its user-word fields by offset. The utterance callbacks index the records through a local index
 * (cbUserWordResultAvailable: every other spelling reloads it or keeps a record pointer), and the
 * accepted ones store the count, the flag, then the state (ccperm: one order fits both).
 * Round 29c: CheckConsistency (DoLabelStart/DoLabelStop inlined from GNU extern inline bodies
 * above it, their real definitions in place) and RejectTrainUtters, SOCOM 1's bodies. Round 30c:
 * DoUserWordTraining, SOCOM 1's body as it stands (the out-parameters at +920/+924 typed in casr.h).
 * OPEN: SendLabelsToEngine (26) and SendPartOfLabelsToEngine (36s), diagnosed in place;
 * FindBestTranscription (round 30c, 0x390 against 0x398) diagnosed in place.
 * Round 31c: DoDurationModeling and ScoreTranscriptions, SOCOM 1's bodies (the object 18 of 21:
 * the scoring result at +944 typed in casr.h, the back pointers' phoneme at SEGLABEL +16).
 *
 * cbDoUserWordTraining's flag test (open since round 4b: `bnezl` here, `bnez` in the target, the
 * return's `move v0,zero` in the slot with a call to ExitUserWordTraining as the fall-through) is
 * byte-exact once ExitUserWordTraining is defined above it: with the callee compiled earlier in
 * the object, reorg leaves the slot unannulled. Not a toolchain question after all (round 12c);
 * tos.c's tosCallControlFunc is the same case (ConstructBlock, its fall-through's callee). */

#include "casr.h"

typedef struct UWDATA {
    int w[9];
} UWDATA;                               /* 36 bytes, kept at +944 */

extern int casrEnableLabelInQueue(CASR *c, unsigned char enable);
extern unsigned char _tosControl(void *event, int id, int a, int b);
extern int ExitUserWordTraining(CASR *c);
extern int DoUserWordTraining(CASR *c);
extern void heap_Free(void *heap, void *p);
extern unsigned char _tosWarningLog(void *log, unsigned short code);

/* the scheduler's queue (queue.c), as far as the labels' sender reads it */
typedef struct QUEUE {
    char pad0[8];
    unsigned short elemsize;            /* +8: bytes */
} QUEUE;

/* the label queue: the fourth output queue the engine registered (cbRegisterOutputQueue) */
#define LABELQ(c)       ((QUEUE *)(c)->slot[3].cb)

extern int qEnQueue(void *q, void *p, int n);
extern int tosRun(CASR *c, const unsigned char *blocks);

/* what command 15 hands the scheduler to start a training: the language's user-word phonemes */
typedef struct UWSETUP {
    int n;                              /* +0 */
    void *phen;                         /* +4 */
    short silence;                      /* +8 */
} UWSETUP;

extern unsigned char _tosErrorLog(void *log, unsigned short code);

/* the host's answer to a training utterance: 1 accepted -- the scheduler told 22 or, with more
 * utterances to come, 10; when the last was rejected in states 19 and 20 the reject commands (35
 * and 21 in 20, then 105); in state 16 with no word yet, 2 as well -- 2 retry (12), 3 cancel
 * (11), anything else an error (17). SOCOM 1's body, round 28c: the count test with the last
 * utterance's case first (`>= needed - 1U`, unsigned as the target's sltu: the 10 path laid out
 * after the 105 test), the cases' bodies in the target's order (3 before 2), the result an
 * `unsigned char` -- _tosControl declared as tos.c defines it, so only the `|=` masks (one andi) */
int userwordAcknowledge(CASR *c, unsigned int mode)
{
    unsigned char ret;

    switch (mode) {
    case 1:
        if (c->uw_recs_done >= c->uw_recs_needed - 1U) {
            ret = _tosControl(c->event, 22, 0, 0);
            if (c->state == 19 || c->state == 20) {
                if (c->rejected != 0) {
                    if (c->state == 20) {
                        if (_tosControl(c->event, 35, 0, 0) != 0)
                            return 1;
                        if (_tosControl(c->event, 21, 0, 0) != 0)
                            return 1;
                    }
                    if (_tosControl(c->event, 105, 0, 0) != 0)
                        return 1;
                }
            }
        } else
            ret = _tosControl(c->event, 10, 0, 0);
        if (c->state == 16 && c->f456 == 0)
            ret |= _tosControl(c->event, 2, 0, 0);
        break;
    case 3:
        ret = _tosControl(c->event, 11, 0, 0);
        break;
    case 2:
        ret = _tosControl(c->event, 12, 0, 0);
        break;
    default:
        ret = _tosErrorLog(c->event, 17);
        break;
    }
    return ret;
}

/* a training started: the scheduler told (14, then 96 and 88 with the language), the counters and
 * the utterances' segmentations reset, the language's user-word phonemes sent (15) with their
 * size, the count kept in a local of its own and stored after the second accessor's call (SOCOM
 * 1's is LOCAL: its caller, userwordStartUserWordTraining, stands as a draft, so this one is
 * public here to be output) */
int InitUserWordTraining(CASR *c)
{
    LANGVT *l = c->lng;
    UWSETUP s;
    unsigned int i;
    int n;

    if (_tosControl(c->event, 14, 0, 0) != 0)
        return 1;
    if (_tosControl(c->event, 96, (int)l, 0) != 0)
        return 1;
    if (_tosControl(c->event, 88, (int)l, 0) != 0)
        return 1;
    c->training = 1;
    c->uw_recs_done = 0;
    for (i = 0; i < c->uw_recs_needed; i++)
        c->utt[i].seg = 0;
    n = l->GetNbrPhenUserWordTraining(l);
    s.phen = l->GetpPhenUserWordTraining(l);
    s.n = n;
    s.silence = c->uw_silence = l->GetUWSilencePhenome(l);
    return _tosControl(c->event, 15, (int)&s, n * 2 + 14) != 0;
}

/* the training over: the utterances' segmentations freed, the scheduler told (35, 21, 16) */
int ExitUserWordTraining(CASR *c)
{
    unsigned int i;

    for (i = 0; i < c->uw_recs_needed; i++)
        if (c->utt[i].seg != 0)
            heap_Free(c->heap, c->utt[i].seg);
    c->train_state = 3;
    if (_tosControl(c->event, 35, 0, 0) != 0)
        return 1;
    if (_tosControl(c->event, 21, 0, 0) != 0)
        return 1;
    if (_tosControl(c->event, 16, 0, 0) != 0)
        return 1;
    c->training = 0;
    return 0;
}

/* every whole label of utterance n written so far into the label queue one at a time, the engine
 * run on each (its result an unsigned char: the target's andi at the one return).
 * OPEN (26 words, the right size; round 15c): the result through an int assigned at each exit
 * gives the target's single `andi v0,a0,0xff` (`return` per exit masks each path, an int
 * initialised at the top lives across the calls in s3, 0xF0 bytes); the division unsigned (the
 * target's divu: the size an unsigned int). What is left: the target loads the buffer into a1 and
 * copies it to p's s0 after the subtraction, here p takes the load (a separate buffer local, 27). */
unsigned char SendLabelsToEngine(CASR *c, const unsigned char *blocks, int n)
{
    char *pos = c->lab_pos[n];
    char *p = c->lab_buf[n];
    unsigned int size = LABELQ(c)->elemsize;
    char *e = p + (pos - p) / size * size;
    int ret;

    if (pos < p)
        p = pos;
    if (pos < e)
        e = pos;
    while (p < e) {
        if (qEnQueue(LABELQ(c), p, 1) == 0) {
            ret = _tosErrorLog(c->event, 1);
            goto out;
        }
        p += LABELQ(c)->elemsize;
        if (tosRun(c, blocks) != 0) {
            ret = 1;
            goto out;
        }
    }
    ret = 0;
out:
    return ret;
}

extern float powf(float x, float y);
extern float expf(float x);

#define MIN(a, b) ((a) < (b) ? (a) : (b))

/* the training utterances' consistency (SOCOM 1's CalcF2, the same slot): of the n x n scores
 * `in`, each row's sums over the other columns; then each utterance's mean over the others of the
 * smaller of the two ratios between their sums, clamped at 1.923 (in double), as
 * exp(-25 (r - 1)^2). The min.s are -ffast-math's (round 25c: the object is built with it -- every
 * function written here matches with and without). */
static void CalcF2(float *out, float *in, unsigned char n)
{
    float m[9];
    unsigned char i, j, k, u, v;
    float sum, a, b;
    double r;

    for (i = 0; i < n; i++)
        for (j = 0; j < n; j++) {
            m[i * n + j] = 0;
            for (k = 0; k < n; k++)
                if (k != j)
                    m[i * n + j] += in[i * n + k];
        }
    for (u = 0; u < n; u++) {
        sum = 0;
        for (v = 0; v < n; v++) {
            if (u == v)
                continue;
            a = m[v * n + u] == 0 ? 0 : m[u * n + u] / m[v * n + u];
            b = m[u * n + u] == 0 ? 0 : m[v * n + u] / m[u * n + u];
            sum += MIN(a, b);
        }
        r = sum / (n - 1);
        if (r > 1.923)
            r = 1.923;
        out[u] = expf(-powf((float)r - 1.0f, 2.0f) * 25.0f);
    }
}

#define MAX(a, b) ((a) > (b) ? (a) : (b))

/* the best of the three utterances' transcriptions: each one's duration score (its own duration
 * against the others', exp(-50 x^2) of the excess ratio over 0.5, clamped to [0, 1.305]), the
 * duration consistency (CalcF2) and its acoustic score (twice its own over the others', less 0.9,
 * floored at -0.923); the sum of the smaller duration term doubled and the acoustic one ranks them,
 * and the one in *best is kept only when its total passes 1.4 and its duration score 0.6. SOCOM 1's
 * body with SOCOM II's float calls (its empty double loop is in the image too). The index's
 * multiplier a variable (`j * n + i`): the target multiplies through a register holding 3 (`li s2,3;
 * mult`, SOCOM 1's too), which a literal 3 never does (synthesised, and `i * 3 + i` folds to i * 4).
 * OPEN (0x390 against 0x398, round 30c): the target holds a second register for the second loop's
 * 3 (s1; one variable here gives one pseudo, s2, for both), keeps the first result row's base in
 * a register (`move s4,sp`), applies the acoustic term's `- 0.9` after its join (`mtc1 zero; sub.s`
 * in the zero arm: here fold distributes it and folds 0 - 0.9) and copies the zeros from fsel's
 * register in fsel, tsel, max order; a per-loop count and the subtraction after the join tried
 * (build/scratch/gcc/v_fbt_*.py) lose 8 more bytes. */
static int FindBestTranscription(CASR *c, float *scores, float *durs, int *best)
{
    float f1[3], f2[3], f3[3];
    float sum, x, t;
    float fsel = 0, tsel = 0, max = 0, min = 3.0f;
    unsigned char i, j, k, m;
    int bi = 0, cur;
    int n = 3;

    for (i = 0; i < 3; i++) {
        sum = 0;
        for (j = 0; j < 3; j++)
            if (j != i)
                sum += durs[j * n + i];
        x = sum == 0 ? 1.305f : MIN(durs[i * n + i] / sum - 0.5f, 1.305f);
        f1[i] = expf(-powf(MAX(x, 0.0f), 2.0f) * 50.0f);
    }
    CalcF2(f2, durs, 3);
    for (i = 0; i < 3; i++) {
        sum = 0;
        for (j = 0; j < 3; j++)
            if (j != i)
                sum += scores[j * n + i];
        x = (sum == 0 ? 0 : 2 * scores[i * n + i] / sum) - 0.9f;
        f3[i] = expf(-powf(MAX(x, -0.923f), 2.0f) * 50.0f);
    }
    for (j = 0; j < 3; j++)
        for (k = 0; k < 3; k++)
            ;
    cur = *best;
    for (m = 0; m < 3; m++) {
        t = (f1[m] < f2[m] ? 2 * f1[m] : 2 * f2[m]) + f3[m];
        if (max < t) {
            max = t;
            bi = m;
        }
        if (t < min)
            min = t;
        if (m == cur) {
            fsel = f1[m];
            tsel = t;
        }
    }
    if (tsel <= 1.4 || fsel <= 0.6)
        *best = bi;
    return 0;
}

extern short *casrGetSegmStart(CASR *c, int i, int j);
extern void casrInitSegmentation(CASR *c);
extern void *heap_Calloc(void *heap, int n, int size);
extern void *memcpy(void *dst, const void *src, unsigned int n);

/* the best transcription's durations modelled: each training utterance's segment starts for it
 * copied (n + 2 a row, the states last to first), each state's duration the least of the
 * utterances' halved (rounded up; the user-word silence one frame), the transcription expanded to
 * one label a frame, and every utterance's segment starts rewritten as that many equal steps
 * through each state. SOCOM 1's body (round 31c), its k loops unrolled by two here where SOCOM 1's
 * build left them: the row length a local (`(n + 2) * 2` folds to `n * 2 + 4`); the utterance
 * counter compared unsigned (the target's beqz/sltu: the count is an int everywhere else); the
 * expansion written through an index (`newseg[idx--]`: a walking pointer makes the body small
 * enough to unroll by four); `total++` above `dur[k] = 1` (the preconditioned copy then stores
 * total's register, the constant 1 there); the two inner counters apart (one counter shared
 * takes other registers); the rewrite's row step in the for's increment after k's, so the
 * duration pointer steps before it. */
static int DoDurationModeling(CASR *c, int best)
{
    int n = c->utt[best].n;
    unsigned short *seg = c->utt[best].seg;
    unsigned short *buf, *p, *q, *out, *newseg;
    unsigned int *dur;
    unsigned int total, min, d, j, m;
    unsigned short i, step, t;
    unsigned int acc;
    int k, idx;
    int len = n + 2;

    buf = heap_Calloc(c->heap, len * c->uw_recs_needed, 2);
    p = buf;
    for (i = 0; i < (unsigned int)c->uw_recs_needed; i++) {
        memcpy(p, casrGetSegmStart(c, best, i), len * 2);
        p += len;
    }
    dur = heap_Calloc(c->heap, n, 4);
    total = 0;
    p = buf;
    for (k = n - 1; k >= 0; k--) {
        q = p++;
        if (seg[k] == (unsigned short)c->uw_silence) {
            total++;
            dur[k] = 1;
        } else {
            min = 10000;
            for (i = 0; i < (unsigned int)c->uw_recs_needed; i++) {
                d = q[0] - q[1];
                if (d < min)
                    min = d;
                q += len;
            }
            dur[k] = (min + 1) / 2;
            total += dur[k];
        }
    }
    newseg = heap_Calloc(c->heap, total, 2);
    idx = total - 1;
    for (k = n - 1; k >= 0; k--)
        for (j = 0; j < dur[k]; j++)
            newseg[idx--] = seg[k];
    p = buf;
    heap_Free(c->heap, c->utt[best].seg);
    c->utt[best].n = total;
    c->utt[best].seg = newseg;
    casrInitSegmentation(c);
    for (i = 0; i < (unsigned int)c->uw_recs_needed; i++) {
        out = casrGetSegmStart(c, best, i);
        for (k = n - 1; k >= 0; k--, p++) {
            t = p[0];
            step = (unsigned short)(p[0] - p[1]) / dur[k];
            acc = 0;
            for (m = 0; m < dur[k]; m++) {
                *out++ = t - acc;
                acc += step;
            }
        }
        for (; k >= -2; k--)
            *out++ = *p++;
    }
    heap_Free(c->heap, dur);
    heap_Free(c->heap, buf);
    return 0;
}

/* the label stop and start (defined at their place below), as GNU extern inlines for the
 * functions between: CheckConsistency holds both bodies */
extern __inline__ int DoLabelStop(CASR *c)
{
    if (_tosControl(c->event, 32, 0, 0) != 0)
        return 1;
    return casrEnableLabelInQueue(c, 1);
}

extern __inline__ int DoLabelStart(CASR *c)
{
    if (casrEnableLabelInQueue(c, 0) != 0)
        return 1;
    return _tosControl(c->event, 31, 0, 0);
}

/* what command 19 hands the scheduler: an utterance's segmentation to score */
typedef struct LABELREQ {
    int n;                              /* +0: the segmentation's length */
    int one;                            /* +4 */
    void *seg;                          /* +8 */
    short *silence;                     /* +12: the user-word silence phoneme */
} LABELREQ;

extern const unsigned char LabelBlockIDs[];

extern void casrStoreSegmentation(CASR *c, int i, int j, SEGLABEL *l);

/* each training utterance's transcription (its user-word silences made the language's silence)
 * scored against every utterance's labels: the scores, per-frame scores and durations kept in
 * the n x n tables, each segmentation stored; the first transcription shorter than the utterance's
 * own is kept, and every later one must repeat it -- else all are rejected (*bad). With the last
 * utterance scored, a repeated one replaces the utterance's own (none: the utterance rejected);
 * otherwise the pass is run once more from the first utterance before the first one's is
 * rejected. The utterance with the least total is *best. SOCOM 1's body (round 31c): the values
 * reload spills declared in the order of their slots (i, least, buf, ret, retried, first, have,
 * same: slots +68 to +96, the pseudos numbered as declared); the size summed straight after
 * lab.one's store (four statements later, loop.c hoists the 1 out of the utterance loop, global-alloc
 * finds it no register and reload folds it into an addiu); the scoring arm first (the skip's
 * `total += score[j]` laid out at the end); the repeat's count through a local (`n = ...; if (cnt
 * == n)`: inside the test jump threading splits the ?: into branches where the target's movn
 * computes it); each loop its own counter and the two list walks their own pointer (one shared
 * variable is one pseudo, one register for all); the replacement copied as `while (k++ < cnt)`
 * (tested on the old value: never unrolled, as the target's is not); the first transcription's
 * pointer read before `first` and `have` are set (sched2 keeps the load ahead of the two spill
 * stores, and reload's 1 for have reuses the dead count's register). */
static int ScoreTranscriptions(CASR *c, float *scores, float *durs, int *best, unsigned char *reject,
                               unsigned char *bad)
{
    LANGVT *l = c->lng;
    int score[3];
    int len[3];
    LABELREQ lab;
    unsigned int i, least;
    unsigned short *buf;
    int ret, retried, first, have, same;
    unsigned int max, total, size;
    int j, k, m, w, cnt, n, eq;
    unsigned short *p, *end;
    unsigned short sil;
    unsigned char err;
    SEGLABEL *q, *r;

    max = 0;
    *bad = 0;
    least = -1;
    buf = 0;
    ret = 0;
    first = 0;
    if (_tosControl(c->event, 89, (int)l, 0) != 0) {
        ret = 1;
        goto out;
    }
    if (_tosControl(c->event, 17, 0, 0) != 0) {
        ret = 1;
        goto out;
    }
    if (_tosControl(c->event, 34, 0, 0) != 0) {
        ret = 1;
        goto out;
    }
    for (i = 0; i < c->uw_recs_needed; i++)
        if (max < c->utt[i].n)
            max = c->utt[i].n;
    buf = heap_Calloc(c->heap, max, 2);
    casrInitSegmentation(c);
    c->uw_silence = l->GetSilencePhenome(l);
    for (i = 0; i < c->uw_recs_needed; i++) {
        total = 0;
        cnt = 0;
        p = c->utt[i].seg;
        end = p + c->utt[i].n;
        sil = l->GetUWSilencePhenome(l);
        have = 0;
        same = 0;
        for (; p < end; p++)
            if (*p == sil)
                *p = l->GetSilencePhenome(l);
        for (j = 0; j < c->uw_recs_needed; j++) {
            score[j] = 0;
            len[j] = 0;
        }
        lab.n = c->utt[i].n;
        lab.one = 1;
        size = (lab.n + lab.one) * 2 + 16;
        lab.seg = c->utt[i].seg;
        lab.silence = &c->uw_silence;
        if (_tosControl(c->event, 23, 1, 0) != 0) {
            ret = 1;
            goto out;
        }
        retried = 0;
        for (j = 0; j < c->uw_recs_needed; j++) {
            if (len[j] != c->utt[i].n) {
                if (_tosControl(c->event, 19, (int)&lab, size) != 0) {
                    ret = 1;
                    goto out;
                }
                err = DoLabelStart(c);
                if (err) {
                    ret = 1;
                    goto out;
                }
                if (SendLabelsToEngine(c, LabelBlockIDs, j) != 0) {
                    ret = 1;
                    goto out;
                }
                err = DoLabelStop(c);
                if (err) {
                    ret = 1;
                    goto out;
                }
                score[j] = c->uw_score;
                total += score[j];
                scores[i * 3 + j] = c->uw_perframe;
                durs[i * 3 + j] = c->uw_nframes;
                casrStoreSegmentation(c, i, j, c->uw_bt);
                len[j] = c->uw_nnodes - 2;
                if (c->uw_nnodes < c->utt[i].n + 2) {
                    if (!have) {
                        cnt = c->uw_nnodes > 1 ? c->uw_nnodes - 2 : 0;
                        q = c->uw_bt;
                        first = j;
                        have = 1;
                        for (k = cnt - 1; k >= 0; k--) {
                            buf[k] = q->word;
                            q = q->next;
                        }
                    } else {
                        n = c->uw_nnodes > 1 ? c->uw_nnodes - 2 : 0;
                        if (cnt == n) {
                            r = c->uw_bt;
                            eq = 1;
                            for (m = cnt - 1; m >= 0 && eq; m--) {
                                eq = r->word == buf[m];
                                r = r->next;
                            }
                        } else
                            eq = 0;
                        if (eq)
                            same = 1;
                        else {
                            for (m = 0; m < c->uw_recs_needed; m++)
                                reject[m] = 1;
                            *bad = 1;
                            goto out;
                        }
                    }
                }
            } else
                total += score[j];
            if (j == c->uw_recs_needed - 1 && have) {
                if (same) {
                    if (cnt == 0) {
                        *bad = 1;
                        reject[i] = 1;
                        total = -1;
                        break;
                    }
                    c->utt[i].n = cnt;
                    p = c->utt[i].seg;
                    w = 0;
                    while (w++ < cnt)
                        *p++ = buf[w];
                    lab.n = c->utt[i].n;
                    size = (lab.n + lab.one) * 2 + 16;
                } else if (_tosControl(c->event, 23, 0, 0) != 0) {
                    ret = 1;
                    goto out;
                }
                if (retried) {
                    *bad = 1;
                    reject[first] = 1;
                    total = -1;
                } else {
                    have = 0;
                    total = 0;
                    same = 0;
                    cnt = 0;
                    retried = 1;
                    j = -1;
                }
            }
        }
        if (total < least) {
            *best = i;
            least = total;
        }
    }
out:
    if (buf != 0)
        heap_Free(c->heap, buf);
    return ret;
}

/* utterance k's segmentation scored against each training utterance's labels: the best score and
 * the second best; above parameter +432 the word is consistent (*ok), and the utterance kept is the
 * best one alone when the second is below parameter +436, else all of them. SOCOM 1's body */
static int CheckConsistency(CASR *c, int k, unsigned char *keep, unsigned char *ok)
{
    LABELREQ lab;
    int i;
    int best, second, besti;
    unsigned int size;
    unsigned char err;

    lab.n = c->utt[k].n;
    lab.one = 1;
    lab.seg = c->utt[k].seg;
    lab.silence = &c->uw_silence;
    size = (lab.n + lab.one) * 2 + 16;
    best = -1000;
    second = -1000;
    besti = -1;
    if (_tosControl(c->event, 23, 0, 0) != 0)
        return 1;
    for (i = 0; i < c->uw_recs_needed; i++) {
        if (_tosControl(c->event, 19, (int)&lab, size) != 0)
            return 1;
        err = DoLabelStart(c);
        if (err)
            return 1;
        if (SendLabelsToEngine(c, LabelBlockIDs, i) != 0)
            return 1;
        err = DoLabelStop(c);
        if (err)
            return 1;
        if (best < c->f960) {
            second = best;
            best = c->f960;
            besti = i;
        } else if (second < c->f960)
            second = c->f960;
    }
    if (c->p432 < best) {
        *ok = 1;
        if (second < c->p436)
            keep[besti] = 1;
        else
            for (i = 0; i < c->uw_recs_needed; i++)
                keep[i] = 1;
    }
    return 0;
}

/* the utterances marked in `reject` dropped -- all of them when two or more are -- their labels'
 * write position reset to the buffer's start, the rest moved down into the freed slots with
 * their label buffers; the count of utterances done lowered by the drops. SOCOM 1's body */
static void RejectTrainUtters(CASR *c, unsigned char *reject)
{
    int i, j, k, n;
    char *t;

    n = 0;
    for (k = 0; k < c->uw_recs_needed; k++)
        n += reject[k];
    if (n >= 2) {
        n = c->uw_recs_needed;
        for (k = 0; k < c->uw_recs_needed; k++)
            reject[k] = 1;
    }
    for (i = 0, j = 0; i < c->uw_recs_needed; i++) {
        if (reject[i] != 0) {
            heap_Free(c->heap, c->utt[i].seg);
            c->utt[i].seg = 0;
            c->utt[i].n = 0;
            c->lab_pos[i] = c->lab_buf[i];
        } else {
            if (j != i) {
                c->utt[j].seg = c->utt[i].seg;
                c->utt[i].seg = 0;
                c->utt[j].n = c->utt[i].n;
                c->utt[i].n = 0;
                t = c->lab_buf[j];
                c->lab_buf[j] = c->lab_buf[i];
                c->lab_buf[i] = t;
                t = c->lab_pos[j];
                c->lab_pos[j] = c->lab_pos[i];
                c->lab_pos[i] = t;
                t = c->lab_end[j];
                c->lab_end[j] = c->lab_end[i];
                c->lab_end[i] = t;
            }
            j++;
        }
    }
    c->rejected = 1;
    c->uw_recs_done -= n;
    c->train_state = 0;
}

extern int TrainInit(CASR *c);
extern void TrainModels(CASR *c, int best);
extern void *heap_Alloc(void *heap, int size);

/* the training once the last utterance is in: every utterance's transcriptions scored against the
 * others (an inconsistent set rejects its utterances, warning 36), the best one chosen, its
 * durations modelled, the models trained on it and checked; the best segmentation handed out
 * through +920/+924. SOCOM 1's body */
int DoUserWordTraining(CASR *c)
{
    float scores[12];
    float durs[12];
    unsigned char reject[16];
    unsigned char bad;
    int best;
    int i;

    best = 0;
    for (i = 0; i < c->uw_recs_needed; i++)
        reject[i] = 0;
    *c->trans_size = 0;
    *c->trans = 0;
    c->rejected = 0;
    if (ScoreTranscriptions(c, scores, durs, &best, reject, &bad) != 0)
        return 1;
    if (bad) {
        c->nstates = 0;
        RejectTrainUtters(c, reject);
        _tosWarningLog(c->event, 36);
        return 0;
    }
    c->nstates = c->utt[best].n;
    if (FindBestTranscription(c, scores, durs, &best) != 0)
        return 1;
    c->nstates = c->utt[best].n;
    if (DoDurationModeling(c, best) != 0)
        return 1;
    c->nstates = c->utt[best].n;
    if (TrainInit(c) != 0)
        return 1;
    TrainModels(c, best);
    CheckConsistency(c, best, reject, &bad);
    if (bad) {
        c->nstates = 0;
        RejectTrainUtters(c, reject);
        _tosWarningLog(c->event, 36);
        return 0;
    }
    *c->trans_size = c->utt[best].n * 2;
    *c->trans = heap_Alloc(c->heap, *c->trans_size);
    memcpy(*c->trans, c->utt[best].seg, *c->trans_size);
    return 0;
}


/* labels [start, end) of utterance n -- clipped to what was written -- into the label queue one at
 * a time, the engine run on each.
 * OPEN (36s): SOCOM 1's order (the step before tosRun: both builds put it in the call's slot). The
 * target's rotated loop keeps its error blocks in place, as a `for` with an increment does (25s,
 * but the step then follows tosRun), and reloads the queue pointer in the preheader where 2.96
 * reuses the first load (t1); the `while`, `for (;;)`, do-while, guard and comparison spellings
 * tried (build/scratch/gcc/uw/v2.txt, v3.txt) keep the error blocks ahead of the loop. */
int SendPartOfLabelsToEngine(CASR *c, const unsigned char *blocks, int n, int start, int end)
{
    char *p = c->lab_buf[n] + start * LABELQ(c)->elemsize;
    char *e = c->lab_buf[n] + end * LABELQ(c)->elemsize;

    if (c->lab_pos[n] < e)
        e = c->lab_pos[n];
    if (c->lab_pos[n] < p)
        p = c->lab_pos[n];
    while (p < e) {
        if (qEnQueue(LABELQ(c), p, 1) == 0)
            return _tosErrorLog(c->event, 1);
        p += LABELQ(c)->elemsize;
        if (tosRun(c, blocks) != 0)
            return 1;
    }
    return 0;
}

/* a training utterance's segmentation, as the engine hands it over */
typedef struct UWRESULT {
    int f0;
    unsigned int frames;                /* +4 */
    int n;                              /* +8: labels */
    short labels[1];                    /* +12 */
} UWRESULT;

/* the utterance's frames and labels kept for the training (the slot's earlier ones freed) */
int cbUserWordResultAvailable(CASR *c, int a1, int a2, UWRESULT *r)
{
    int n = r->n;
    unsigned int i = c->uw_recs_done;

    c->utt[i].frames = r->frames;
    c->utt[i].n = n;
    if (c->utt[i].seg != 0)
        heap_Free(c->heap, c->utt[i].seg);
    if (n != 0) {
        c->utt[i].seg = heap_Alloc(c->heap, n * 2);
        memcpy(c->utt[i].seg, r->labels, n * 2);
    } else
        c->utt[i].seg = 0;
    c->train_state = 1;
    return 0;
}

int cbScoreUserWordResultAvailable(CASR *c, int a1, int a2, UWDATA *pData)
{
    *(UWDATA *)((char *)c + 944) = *pData;
    return 0;
}

int DoLabelStop(CASR *c)
{
    if (_tosControl(c->event, 32, 0, 0) != 0)
        return 1;
    return casrEnableLabelInQueue(c, 1);
}

int DoLabelStart(CASR *c)
{
    if (casrEnableLabelInQueue(c, 0) != 0)
        return 1;
    return _tosControl(c->event, 31, 0, 0);
}

int cbCancelUserWordTraining(CASR *c)
{
    *(int *)((char *)c + 920) = 0;
    *(int *)((char *)c + 924) = 0;
    if (*((unsigned char *)c + 916) != 0 && ExitUserWordTraining(c) != 0)
        return 1;
    return 0;
}

/* the host refused the training utterance: its labels and segmentation dropped */
int cbUtteranceNotAccepted(CASR *c)
{
    c->lab_pos[c->uw_recs_done] = c->lab_buf[c->uw_recs_done];
    heap_Free(c->heap, c->utt[c->uw_recs_done].seg);
    c->utt[c->uw_recs_done].seg = 0;
    c->train_state = 0;
    return 0;
}

/* the host took it: kept, unless it is too short (a warning, and dropped as a refused one) */
int cbUtteranceAccepted(CASR *c)
{
    if (c->utt[c->uw_recs_done].frames < c->p428) {
        _tosWarningLog(c->event, 35);
        c->rejected = 1;
        c->lab_pos[c->uw_recs_done] = c->lab_buf[c->uw_recs_done];
        heap_Free(c->heap, c->utt[c->uw_recs_done].seg);
        c->utt[c->uw_recs_done].seg = 0;
        c->train_state = 0;
        return 0;
    }
    c->uw_recs_done++;
    c->rejected = 0;
    c->train_state = 0;
    return 0;
}

/* the last utterance taken: as cbUtteranceAccepted, the training then due (state 2) */
int cbLastUtteranceAccepted(CASR *c)
{
    if (c->utt[c->uw_recs_done].frames < c->p428) {
        _tosWarningLog(c->event, 35);
        c->rejected = 1;
        c->lab_pos[c->uw_recs_done] = c->lab_buf[c->uw_recs_done];
        heap_Free(c->heap, c->utt[c->uw_recs_done].seg);
        c->utt[c->uw_recs_done].seg = 0;
        c->train_state = 0;
        return 0;
    }
    c->uw_recs_done++;
    c->rejected = 0;
    c->train_state = 2;
    return 0;
}

int cbDoUserWordTraining(CASR *c)
{
    if (DoUserWordTraining(c) != 0)
        return 1;
    if (*((unsigned char *)c + 940) == 0 && ExitUserWordTraining(c) != 0)
        return 1;
    return 0;
}
