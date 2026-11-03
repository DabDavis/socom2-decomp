/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing -funroll-loops -fno-optimize-sibling-calls */
/* userword: the recogniser's user-word training (OVL2 0x4E1B28-0x4E42A4, SOCOM 1's object at 0x34FFE0:
 * userwordAcknowledge .. cbDoUserWordTraining), ScanSoft's recogniser, prebuilt with ee-gcc 2.96 at
 * -O2 -G0 -fno-strict-aliasing -funroll-loops (README "The GCC front") and, this object, without
 * sibling calls: DoLabelStop returns casrEnableLabelInQueue's int through a jal, byte-exact only
 * under -fno-optimize-sibling-calls (10 words off without; DoLabelStart likewise). Only the
 * functions written so far are here, in image order; names are SOCOM 1's (s1:align, s1:order),
 * the context casr.h's (the three training utterances' records at +980: round 13c), the rest of
 * its user-word fields by offset. The utterance callbacks index the records through a local index
 * (cbUserWordResultAvailable: every other spelling reloads it or keeps a record pointer), and the
 * accepted ones store the count, the flag, then the state (ccperm: one order fits both).
 * OPEN: SendPartOfLabelsToEngine (36s, diagnosed in place), cbDoUserWordTraining (1 word; 10 before round
 * 11c, when the flag test and the last call were separate guards and 2.96 folded the last into
 * sltu: as one `&&` the final `return 0` is a join and keeps its branch): the flag test is `bnezl`
 * here and `bnez` in the target -- reorg takes the return's `move v0,zero` into the slot and
 * annuls it, though the fall-through is a call that sets v0. tosCallControlFunc (tos.c) and
 * UpdateExevStatus (casr.c) miss by the same word; round 12c toggled every optimisation switch of
 * 2.96-ee-001003-1 on all three and none moves it -- a later SCE 2.96's reorg (README). */

#include "casr.h"

typedef struct UWDATA {
    int w[9];
} UWDATA;                               /* 36 bytes, kept at +944 */

extern int casrEnableLabelInQueue(CASR *c, unsigned char enable);
extern int _tosControl(void *event, int id, int a, int b);
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

extern void *heap_Alloc(void *heap, int size);
extern void *memcpy(void *dst, const void *src, unsigned int n);

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
