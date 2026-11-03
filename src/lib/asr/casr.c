/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing -funroll-loops -fno-optimize-sibling-calls */
/* casr: the recogniser's engine API and the engine's callbacks (OVL2 0x4D8398-0x4DF46C, SOCOM 1's
 * casr object at 0x348248 whole: rounds 1 and 2b wrote its tail as cb.c, merged here in round 9c),
 * ScanSoft's speech recogniser, prebuilt with ee-gcc 2.96 at -O2 -G0 -fno-strict-aliasing
 * -funroll-loops (README "The GCC front": casrStoreSegmentation keeps its store above the load of
 * the next node) and -fno-optimize-sibling-calls: casrStartUserWordTraining returns its call
 * through a jal (18s with sibling calls), and the object holds no `j`. Names are SOCOM 1's
 * (s1:exact/align/calls/order), the engine context's fields casr.h's, named by their use. Every
 * entry point checks its handle through an inline returning 6 (null) or 11 (not a live engine):
 * 2.96 if-converts the magic test into `li 11; movz`, and a caller that returns its own constant
 * drops the 6. SOCOM 1's statics casrFreeResult and the NoClearErrorLog pair are inlined into
 * their callers. The functions not yet written are missing; the rest is in image order. */

#include "casr.h"

extern unsigned char _tosControl(void *event, int id, int a, int b);
extern unsigned char _tosErrorLog(void *log, unsigned short code);
extern void *heap_Alloc(void *heap, int size);
extern void heap_Free(void *heap, void *p);
extern int userwordAcknowledge(CASR *c);
extern int userwordStartUserWordTraining(CASR *c, int what, void *a, void *b);
extern int casrEnableGenders(CASR *c, int genders);
extern int casrActivateContext(CASR *c, void *ctx, int a);
extern int func_004D5058(CASR *c, unsigned int *ids, unsigned int n);
extern int casrDoSecondPass(CASR *c);
extern int tosDestruct(CASR *c);
extern int CreateExEvCtxInfo(CASR *c);
extern void FreeCtxInfo(CASR *c);
extern int GetIsolatedWordDpActiveWords(CASR *c, int **words, int *n);
extern int qQueueControl(int queue, int what, int id, int arg);
extern int casrDoSpelling(CASR *c);
extern float expf(float x);

typedef struct IMPORT {
    void *data;
    void *heap;
} IMPORT;

/* a context's accessors (contextv2.c's _ContextV2VirtualTable), as far as the engine calls them */
typedef struct CTXVT CTXVT;
struct CTXVT {
    void *data;                                     /* +0 */
    void *s1[8];
    int (*GetNbrWord)(CTXVT *vt);                   /* +36, slot 9 */
    void *s10[11];
};

extern void FillContextV2VirtualTable(CTXVT *vt);

/* the result the host is handed when nothing was recognised */
typedef struct NORESULT {
    int a;
    int b;
    int reason;                         /* +8: 2 the timeout, else 0 */
} NORESULT;
extern void *memcpy(void *dst, const void *src, unsigned int n);
extern void *qDeQueueOne(struct QUEUE *q, int reader);
extern void qFreeUnusedElemements(struct QUEUE *q);

/* the scheduler's queue (queue.c), as far as the engine reads it */
typedef struct QUEUE {
    void *tos;
    unsigned char id;
    char pad5[3];
    unsigned short elemsize;            /* +8: bytes */
} QUEUE;

/* the language file's 128-byte header, at +8 of the file the activated data holds */
typedef struct LNGHDR {
    int w[32];
} LNGHDR;

typedef struct LNGFILE {
    int pad[2];
    LNGHDR hdr;                         /* +8 */
} LNGFILE;

/* the session data casrExportData hands out: this header, then the engine's own (_tosControl 39) */
typedef struct SESSIONDATA {
    int kind;                           /* +0: 4 */
    int size;                           /* +4: the whole record's bytes */
    int gender[4];                      /* +8: the genders enabled, then the sample format */
    LNGHDR lng;                         /* +24 */
} SESSIONDATA;

/* a recognition result: its items (a word sequence each), each with its words */
typedef struct RESWORD {
    int pad[3];
    void *p;                            /* +12: heap */
} RESWORD;

typedef struct RESITEM {
    int n;                              /* +0: words */
    int pad[4];
    RESWORD *words;                     /* +20: heap */
} RESITEM;

typedef struct RESULT {
    int n;                              /* +0: items */
    RESITEM *items;                     /* +4: heap */
} RESULT;

extern void casrCreateResult(CASR *c, RESULT **r, int frame, int how);
extern int cbIsoWordRecognised(CASR *c, unsigned char a1, unsigned char a2, int arg1, int arg2);
int casrGetParam(CASR *c, int id, unsigned int *value);
int casrSetParam(CASR *c, int id, unsigned int value);
extern int CreateSecondPassContext(CASR *c);
extern int CreateSecondPassContext_ForChinese(CASR *c);
extern int CreateSecondPassTreeContext(CASR *c);
extern int CreateSecondPassTreeContext_ForChinese(CASR *c);
extern void FreeSecondPassContext(CASR *c);
extern int DoLabelStart(CASR *c);
extern int DoLabelStop(CASR *c, const unsigned char *blocks);
extern int SendLabelsToEngine(CASR *c, const unsigned char *blocks, int a);

/* SOCOM 1's casrGetParamNoClearErrorLog and casrSetParamNoClearErrorLog, inlined here: the
 * parameter calls of a second pass keep the caller's error (their own copies are cb.c's rows) */
extern __inline__ int casrGetParamNoClearErrorLog(CASR *c, int id, unsigned int *value)
{
    int err = c->last_error;
    int r = casrGetParam(c, id, value);

    c->last_error = err;
    return r;
}

extern __inline__ int casrSetParamNoClearErrorLog(CASR *c, int id, unsigned int value)
{
    int err = c->last_error;
    int r = casrSetParam(c, id, value);

    c->last_error = err;
    return r;
}
static int casrGetSessionData(CASR *c, void **data, int *size);
static int casrReadLabelInQueue(CASR *c, int n);

/* the casr object's .rodata: the blocks a labelled utterance activates */
static const unsigned char ActiveBlocks_UTTFORMAT_LABELS[3] = { 0x1F, 0x20, 0 };
static const unsigned char ActiveBlocks_UTTFORMAT_LABELS_ISO[2] = { 0x23, 0 };

/* SOCOM 1's static casrFreeResult, inlined into every caller here (no copy is left) */
static __inline__ void casrFreeResult(CASR *c, RESULT *r)
{
    int i, j;
    RESITEM *it;
    RESWORD *w;

    it = r->items;
    for (i = 0; i < r->n; i++) {
        w = it->words;
        for (j = 0; j < it->n; j++) {
            heap_Free(c->heap, w->p);
            w++;
        }
        heap_Free(c->heap, it->words);
        it++;
    }
    if (i != 0)
        heap_Free(c->heap, r->items);
    heap_Free(c->heap, r);
}

/* the end of an utterance: a second pass when parameter 15 asks for one, else the result now.
 * OPEN (9 words): the result variable takes v1 here, v0 in the target (and the demo), so the iso
 * branch's `b` slot gets a move where the target's computes the `== 1`, and the second pass's three
 * stores reorder; tried: the variables in other orders, err an unsigned char, err reused for the
 * callback's result, the return as ==, ?:, if/return, unsigned char (build/scratch/gcc/p/s_dsp*.py). */
int cbDoSecondPassIfEnabled(CASR *c, unsigned char a1, unsigned char a2, int arg1, int arg2)
{
    unsigned int enabled;
    RESULT *r;
    int err;
    int ret;

    if (casrGetParam(c, 15, &enabled) != 0)
        return 1;
    if (enabled) {
        if (_tosControl(c->event, 2, 0, 0) != 0)
            return 1;
        c->sp_arg1 = arg1;
        c->second_pass = 1;
        c->sp_arg2 = arg2;
        err = 0;
    } else {
        if (_tosControl(c->event, 7, 0, 0) != 0)
            return 1;
        if (c->iso_word)
            err = cbIsoWordRecognised(c, a1, a2, arg1, arg2);
        else {
            casrCreateResult(c, &r, arg1, 1);
            ret = c->result_cb(r, c->info);
            if (c->last_state == 0 && ret != 0) {
                c->last_state = ret;
                _tosErrorLog(c->event, 27);
            }
            casrFreeResult(c, r);
            err = 0;
        }
    }
    return err == 1;
}

/* OPEN (38 words, the right size): the target stores the first gender and the format through s+8
 * and the other two genders off s, and loads the language's pointer into a0; every spelling here
 * stores all four through s+8 (a loop, a sub-struct, a pointer for two of them: build/scratch/gcc/p/
 * s_gsd*.py, 30 spellings). The header's destination is a local pointer (`h`): with `s->lng` itself
 * 2.96 folds the data pointer to s+152 and computes the copy's bound once (87s), the demo's shape. */
static int casrGetSessionData(CASR *c, void **data, int *size)
{
    SESSIONDATA *s;
    LNGHDR *h;
    int *g;
    int n;

    n = 0;
    if (_tosControl(c->event, 39, 0, (int)&n) != 0)
        return 1;
    n += sizeof(SESSIONDATA);
    s = heap_Alloc(c->heap, n);
    if (s == 0)
        return 1;
    s->kind = 4;
    s->size = n;
    g = s->gender;
    g[0] = c->gender_on[0];
    g[1] = c->gender_on[1];
    g[2] = c->gender_on[2];
    g[3] = c->uw_state;
    h = &s->lng;
    *h = (*(LNGFILE **)c->data)->hdr;
    n = 0;
    if (_tosControl(c->event, 39, (int)(h + 1), (int)&n) != 0) {
        heap_Free(c->heap, s);
        return 1;
    }
    *size = n + sizeof(SESSIONDATA);
    *data = s;
    return 0;
}

int casrClose(CASR *c)
{
    int err;
    int state;

    c->last_error = 0;
    err = casrCheckHandle(c);
    if (err)
        return err;
    state = c->state;
    if (state != 0 && state != 1 && state != 20 && state != 2)
        return _tosErrorLog(c->event, 7);
    if (c->data != 0) {
        heap_Free(c->heap, c->lng);
        c->lng = 0;
    }
    if (c->ctx != 0) {
        if (_tosControl(c->event, 41, 0, 0) != 0)
            return (unsigned char)c->last_error;
        FreeCtxInfo(c);
    }
    if (c->lab_buf[0] != 0)
        heap_Free(c->heap, c->lab_buf[0]);
    if (c->lab_buf[1] != 0)
        heap_Free(c->heap, c->lab_buf[1]);
    if (c->lab_buf[2] != 0)
        heap_Free(c->heap, c->lab_buf[2]);
    if (c->segtab[0] != 0)
        heap_Free(c->heap, c->segtab[0]);
    heap_Free(c->heap, c->segtab);
    if (tosDestruct(c) != 0)
        return 1;
    c->magic = 0;
    heap_Free(c->heap, c);
    return 0;
}

int casrStart(CASR *c)
{
    int err;

    c->last_error = 0;
    err = casrCheckHandle(c);
    if (err)
        return err;
    if (c->state != 2)
        return _tosErrorLog(c->event, 7);
    c->frames = 0;
    c->lab_pos[0] = c->lab_buf[0];
    c->f672 = 0;
    c->f674 = 0;
    c->second_pass = 0;
    c->f906 = 0;
    c->uw_recs_done = 0;
    if (casrEnableGenders(c, c->gender) != 0)
        return _tosErrorLog(c->event, 20);
    c->uw_state = 4;
    if (c->iso_word) {
        if (_tosControl(c->event, 90, 0, 0) != 0)
            return (unsigned char)c->last_error;
        if (_tosControl(c->event, 8, 0, 0) != 0)
            return (unsigned char)c->last_error;
    } else {
        if (_tosControl(c->event, 87, 0, 0) != 0)
            return (unsigned char)c->last_error;
        if (c->use_dp) {
            if (_tosControl(c->event, 6, 0, 0) != 0)
                return (unsigned char)c->last_error;
        } else {
            if (_tosControl(c->event, 1, 0, 0) != 0)
                return (unsigned char)c->last_error;
        }
    }
    c->f673 = 0;
    if (c->f456 == 0) {
        if (_tosControl(c->event, 2, 0, 0) != 0)
            return (unsigned char)c->last_error;
        c->f673 = 1;
    }
    return (unsigned char)c->last_error;
}

/* the labels the engine queued for training utterance n, appended to its buffer (grown by 2000) */
static int casrReadLabelInQueue(CASR *c, int n)
{
    QUEUE *q;
    int id;
    void *e;
    char *pos, *next, *old, *buf;
    int len;

    q = (QUEUE *)c->queue;
    id = c->queue_id;
    while ((e = qDeQueueOne(q, id)) != 0) {
        pos = c->lab_pos[n];
        next = pos + q->elemsize;
        if (c->lab_end[n] < next) {
            old = c->lab_buf[n];
            len = c->lab_end[n] - old;
            buf = heap_Alloc(c->heap, len + 2000);
            if (buf == 0)
                return _tosErrorLog(c->event, 2);
            if (old != 0) {
                memcpy(buf, old, len);
                heap_Free(c->heap, old);
            }
            pos = buf + (pos - old);
            next = pos + q->elemsize;
            c->lab_buf[n] = buf;
            c->lab_end[n] = buf + len + 2000;
        }
        memcpy(pos, e, q->elemsize);
        c->lab_pos[n] = next;
    }
    qFreeUnusedElemements(q);
    return 0;
}

int casrStop(CASR *c)
{
    int err;
    int state;
    int old;
    int r;

    state = c->state;
    err = casrCheckHandle(c);
    if (err)
        return err;
    c->last_error = 0;
    c->last_state = 0;
    switch (state) {
    case 3:
    case 4:
    case 7:
    case 8:
    case 11:
    case 12:
    case 16:
    case 17:
        break;
    default:
        return _tosErrorLog(c->event, 7);
    }
    if (_tosControl(c->event, 3, 0, 0) != 0)
        return (unsigned char)c->last_error;
    if (casrReadLabelInQueue(c, c->uw_recs_done) != 0)
        return (unsigned char)c->last_error;
    if (c->second_pass && casrDoSecondPass(c) != 0)
        return (unsigned char)c->last_error;
    old = c->train_state;
    if (old != 4) {
        c->train_state = 4;
        r = c->train_cb(old, c->info);
        if (c->last_state == 0 && r != 0) {
            c->last_state = r;
            _tosErrorLog(c->event, 27);
        }
    }
    return (unsigned char)c->last_error;
}

/* the active words: the engine's bitmap (_tosControl 26), one bit a word, high bit first */
int casrGetActiveWords(CASR *c, int **words, int *n)
{
    int err;
    int count = 0;
    int word = 0;
    unsigned char *bits;
    unsigned int nbytes;
    unsigned int i;
    unsigned int mask;
    unsigned char b;
    int *p;

    err = casrCheckHandle(c);
    if (err)
        return err;
    if (words == 0 || n == 0)
        return _tosErrorLog(c->event, CASR_ERR_ARG);
    if (c->iso_word)
        return GetIsolatedWordDpActiveWords(c, words, n);
    c->last_error = 0;
    if (_tosControl(c->event, 26, (int)&bits, (int)&nbytes) != 0)
        return 1;
    for (i = 0; i < nbytes; i++) {
        b = bits[i];
        for (mask = 0x80; mask != 0; mask >>= 1)
            if (b & mask)
                count++;
    }
    if (count != 0) {
        p = heap_Alloc(c->heap, count * sizeof(int));
        if (p == 0)
            return _tosErrorLog(c->event, 1);
    } else
        p = 0;
    *n = count;
    *words = p;
    for (i = 0; i < nbytes; i++) {
        b = bits[i];
        for (mask = 0x80; mask != 0; mask >>= 1) {
            if (b & mask)
                *p++ = word;
            word++;
        }
    }
    return 0;
}

static int InitParamList(CASR *c)
{
    c->p420 = 0;
    c->p424 = 1;
    c->p428 = 30;
    c->p432 = -10;
    c->p436 = -20;
    c->exev = 1;
    if (_tosControl(c->event, 70, c->p444, 0) != 0)
        return 1;
    if (_tosControl(c->event, 71, &c->f456, 0) != 0)
        return 1;
    if (_tosControl(c->event, 72, &c->gender, 0) != 0)
        return 1;
    if (_tosControl(c->event, 73, (int)c->p484, 0) != 0)
        return 1;
    return _tosControl(c->event, 74, &c->p488, 0);
}

int casrGetParam(CASR *c, int id, unsigned int *value)
{
    int err;

    err = casrCheckHandle(c);
    if (err)
        return err;
    if (value == 0)
        return _tosErrorLog(c->event, CASR_ERR_ARG);
    switch (id) {
    case 1:
        *value = c->p460;
        break;
    case 2:
        *value = c->f456;
        break;
    case 3:
        *value = c->p504;
        break;
    case 4:
        *value = c->p464;
        break;
    case 5:
        *value = c->p420 / 100;
        break;
    case 15:
        *value = c->p424;
        break;
    case 18:
        *value = c->exev;
        break;
    case 16:
        *value = c->f1088;
        break;
    case 6:
        *value = c->p488;
        break;
    case 7:
        *value = 50 - c->p484[1];
        break;
    case 8:
        *value = 50 - c->p484[0];
        break;
    case 9:
        *value = c->p468;
        break;
    case 10:
        *value = c->p472;
        break;
    case 17:
        *value = c->p476;
        break;
    case 11:
        *value = c->p444[0];
        break;
    case 13:
        *value = c->p492;
        break;
    case 12:
        *value = c->p444[1];
        break;
    case 14:
        *value = c->gender;
        break;
    default:
        return _tosErrorLog(c->event, 10);
    }
    return 0;
}

/* OPEN (1 word): the last test's branch -- the target's `bnez v0` keeps `li v0,1` in a plain delay
 * slot (the fall-through's `move v0,zero` overwrites it), 2.96 here annuls it (`bnezl`): reorg's view
 * of v0 at the fall-through. SOCOM 1's copy tests a copy (`move v1,v0; bnez v1`). Tried: the return
 * as unsigned char/int/long/short, _tosControl as int/unsigned char/signed char/unsigned short, every
 * error exit a `return 1` or one `goto error`, the else-if and nested forms (build/scratch/gcc/p/
 * s_ues*.py, 30 spellings: 1 to 44s); no switch of 2.96-ee-001003-1 moves it either (round 12c's
 * build/scratch/gcc-r12c/bnez_sweep.sh): a later SCE 2.96's reorg. */
static unsigned char UpdateExevStatus(CASR *c)
{
    if (c->exev != 0 && c->lng->CheckBitField(c->lng, 64)) {
        if (_tosControl(c->event, 100, 0, 0) != 0)
            goto error;
        if (_tosControl(c->event, 86, &c->f1096, 0) != 0)
            goto error;
        if (CreateExEvCtxInfo(c) != 0)
            goto error;
        if (_tosControl(c->event, 102, c->f608, 0) != 0) {
            FreeCtxInfo(c);
            goto error;
        }
    } else if (_tosControl(c->event, 101, 0, 0) != 0)
        goto error;
    return 0;
error:
    return 1;
}

/* the second pass over the labelled utterance: its own context (the Chinese one when the language
 * says so), the labels sent again with the timeout (parameter 1) off, then the context back */
static int casrDoNormalSecondPass(CASR *c)
{
    unsigned int timeout;

    if (c->lng->CheckBitField(c->lng, 8)) {
        if (CreateSecondPassContext_ForChinese(c) != 0)
            return 1;
    } else {
        if (CreateSecondPassContext(c) != 0)
            return 1;
    }
    if (_tosControl(c->event, 36, (int)c->sp_ctx, 0) != 0
        || _tosControl(c->event, 84, (int)&c->p488, 0) != 0
        || _tosControl(c->event, 17, 0, 0) != 0
        || casrGetParamNoClearErrorLog(c, 1, &timeout) != 0
        || casrSetParamNoClearErrorLog(c, 1, 0) != 0
        || DoLabelStart(c) != 0
        || SendLabelsToEngine(c, ActiveBlocks_UTTFORMAT_LABELS, 0) != 0
        || DoLabelStop(c, ActiveBlocks_UTTFORMAT_LABELS) != 0
        || _tosControl(c->event, 21, 0, 0) != 0
        || casrSetParamNoClearErrorLog(c, 1, timeout) != 0
        || _tosControl(c->event, 36, (int)c->run_ctx, 0) != 0
        || _tosControl(c->event, 84, (int)&c->p488, 0) != 0) {
        FreeSecondPassContext(c);
        return 1;
    }
    FreeSecondPassContext(c);
    c->second_pass = 0;
    return 0;
}

/* the same over an isolated word: the tree context, and the iso-word context back after */
static int casrDoIsoWordSecondPass(CASR *c)
{
    unsigned int timeout;

    if (c->lng->CheckBitField(c->lng, 8)) {
        if (CreateSecondPassTreeContext_ForChinese(c) != 0)
            return 1;
    } else {
        if (CreateSecondPassTreeContext(c) != 0)
            return 1;
    }
    if (_tosControl(c->event, 87, 0, 0) != 0
        || _tosControl(c->event, 36, (int)c->sp_ctx, 0) != 0
        || _tosControl(c->event, 84, (int)&c->p488, 0) != 0
        || _tosControl(c->event, 17, 0, 0) != 0
        || casrGetParamNoClearErrorLog(c, 1, &timeout) != 0
        || casrSetParamNoClearErrorLog(c, 1, 0) != 0
        || DoLabelStart(c) != 0
        || SendLabelsToEngine(c, ActiveBlocks_UTTFORMAT_LABELS, 0) != 0
        || DoLabelStop(c, ActiveBlocks_UTTFORMAT_LABELS) != 0
        || _tosControl(c->event, 21, 0, 0) != 0
        || casrSetParamNoClearErrorLog(c, 1, timeout) != 0
        || _tosControl(c->event, 41, 0, 0) != 0
        || _tosControl(c->event, 90, 0, 0) != 0
        || _tosControl(c->event, 36, (int)c->iso_ctx, 0) != 0
        || _tosControl(c->event, 84, (int)&c->p488, 0) != 0) {
        FreeSecondPassContext(c);
        return 1;
    }
    FreeSecondPassContext(c);
    c->second_pass = 0;
    return 0;
}

static int casrInitSampleFormat(CASR *c, unsigned int fmt)
{
    c->uw_state = fmt;
    if (fmt >= 4)
        return 32;
    if (fmt == 0) {
        c->cur = c->slot[0];
        c->f628 = 0;
    } else {
        c->cur = c->slot[3];
        c->f628 = c->iso_word ? ActiveBlocks_UTTFORMAT_LABELS_ISO : ActiveBlocks_UTTFORMAT_LABELS;
    }
    if (fmt != 0) {
        if (_tosControl(c->event, 106, 0, 0) != 0)
            return 1;
    } else {
        if (_tosControl(c->event, 107, 0, 0) != 0)
            return 1;
    }
    if (c->f456 != 0 && fmt != 0) {
        if (_tosControl(c->event, 2, 0, 0) != 0)
            return 1;
    }
    if (c->f456 != 0 && fmt == 0) {
        c->f673 = 0;
        if (_tosControl(c->event, 51, 0, 0) != 0)
            return 1;
    } else {
        c->f673 = 1;
        if (_tosControl(c->event, 52, 0, 0) != 0)
            return 1;
    }
    c->f624 = ((SAMPLEFMT *)c->cur.cb)->frame;
    if (fmt == 2) {
        if (c->lng == 0)
            c->f624--;
        else if ((unsigned short)c->lng->GetNbrCodeBook(c->lng) < 5)
            c->f624--;
    }
    return 0;
}

int casrStartUserWordTraining(CASR *c, void *a, void *b)
{
    int err;
    int state;

    err = casrCheckHandle(c);
    if (err)
        return err;
    if (a == 0 || b == 0)
        return _tosErrorLog(c->event, CASR_ERR_ARG);
    state = c->state;
    if (state != 1 && state != 20 && state != 2)
        return _tosErrorLog(c->event, 7);
    if (c->lng->GetNbrPhenUserWordTraining(c->lng) == 0)
        return _tosErrorLog(c->event, 21);
    if (casrEnableGenders(c, c->gender) != 0)
        return _tosErrorLog(c->event, 20);
    if (c->ctx != 0 && casrActivateContext(c, 0, 0) != 0)
        return (unsigned char)c->last_error;
    c->lab_pos[0] = c->lab_buf[0];
    c->lab_pos[1] = c->lab_buf[1];
    c->lab_pos[2] = c->lab_buf[2];
    c->train_state = 4;
    return userwordStartUserWordTraining(c, 3, a, b);
}

/* OPEN (44 words, 6 short): the right instructions but the row pointer -- the target keeps it a
 * register stepped by 12 per row (`move a1,s1; addiu s1,a1,12`), each row's third pointer stored
 * through the stepped one; 2.96 here splits it into constant offsets of one base. Pre- and
 * post-increment, an index, three explicit rows, a while: all split it (44-119 words). The total
 * is a summing loop: the target multiplies (sum + 6) by 3 and 2, which a single expression lets
 * fold distribute. SOCOM 1's copy is rolled: unsigned counters, the store through `q`. */
void casrInitSegmentation(CASR *c)
{
    short **seg = c->segtab;
    short **q;
    short *p;
    unsigned int i, j;
    int len, total;

    if (seg[0] != 0)
        heap_Free(c->heap, seg[0]);
    total = 0;
    for (i = 0; i < 3; i++)
        total += c->seginfo[i].n + 2;
    p = seg[0] = heap_Alloc(c->heap, total * 3 * sizeof(short));
    q = seg;
    for (i = 0; i < 3; i++) {
        len = c->seginfo[i].n + 2;
        for (j = 0; j < 3; j++) {
            p += len;
            *++q = p;
        }
    }
}

int casrAcknowledge(CASR *c)
{
    int err;
    int ret;
    int old;
    int r;

    err = casrCheckHandle(c);
    if (err)
        return err;
    if (c->state != 15 && c->state != 18 && c->state != 19)
        return _tosErrorLog(c->event, 7);
    ret = userwordAcknowledge(c);
    old = c->train_state;
    if (old != 4) {
        c->train_state = 4;
        r = c->train_cb(old, c->info);
        if (c->last_state == 0 && r != 0) {
            c->last_state = r;
            ret = _tosErrorLog(c->event, 27);
        }
    }
    return ret;
}

int casrExportData(CASR *c, int kind, void **data, int *size)
{
    int err;

    c->last_error = 0;
    err = casrCheckHandle(c);
    if (err)
        return err;
    if (data == 0 || size == 0)
        return _tosErrorLog(c->event, CASR_ERR_ARG);
    if (kind != 4) {
        *data = 0;
        *size = 0;
        return _tosErrorLog(c->event, 12);
    }
    if (c->state != 1 && c->state != 2 && c->state != 20 && c->state != 18) {
        *data = 0;
        *size = 0;
        return _tosErrorLog(c->event, 7);
    }
    return casrGetSessionData(c, data, size) != 0;
}

int casrGainSet(CASR *c, unsigned short gain)
{
    int err;

    err = casrCheckHandle(c);
    if (err)
        return err;
    c->last_error = 0;
    _tosControl(c->event, 53, gain, 0);
    return (unsigned char)c->last_error;
}

int casrGetActivatedData(CASR *c, void **out, int *active)
{
    int err;
    void **p;

    c->last_error = 0;
    err = casrCheckHandle(c);
    if (err)
        return err;
    if (out == 0 || active == 0)
        return _tosErrorLog(c->event, CASR_ERR_ARG);
    *active = c->data != 0;
    if (*active) {
        p = heap_Alloc(c->heap, 8);
        if (p == 0)
            return 1;
        *out = p;
        if (c->data != 0)
            *p = c->data;
        return 0;
    }
    *out = 0;
    return 0;
}

int casrGetUserWordNbrRecsNeeded(CASR *c, int *n)
{
    int err;
    int state;

    err = casrCheckHandle(c);
    if (err)
        return err;
    state = c->state;
    if (state != 1 && state != 15 && state != 17 && state != 16 && state != 18 && state != 19
        && state != 20)
        return _tosErrorLog(c->event, 7);
    if (state == 1)
        *n = c->uw_recs_needed;
    else if (state == 20)
        *n = 0;
    else
        *n = c->uw_recs_needed - c->uw_recs_done;
    return err;
}

/* OPEN (and casrGetLastWarning): the target if-converts the magic test into a flag (li 11,
 * movz 0) and branches on it, after testing the null handle by itself; SOCOM 1's build has this
 * plain guard. The guard, a ?: macro, an inline checker, a single-exit flag: all compile to
 * branches (12 words of 16). */
int casrGetLastError(CASR *c)
{
    int err;

    if (c == 0 || c->magic != CASR_MAGIC)
        return CASR_ERR_HANDLE;
    err = c->last_error;
    c->last_error = 0;
    return err;
}

int casrGetLastWarning(CASR *c)
{
    int err;

    if (c == 0 || c->magic != CASR_MAGIC)
        return CASR_ERR_HANDLE;
    err = c->last_warning;
    c->last_warning = 0;
    return err;
}

int casrGetSignalLevel(CASR *c, unsigned short *level)
{
    if (c == 0)
        return CASR_ERR_NULL;
    if (c->magic != CASR_MAGIC)
        return CASR_ERR_HANDLE;
    if (level == 0)
        return _tosErrorLog(c->event, CASR_ERR_ARG);
    *level = *c->signal;
    return 0;
}

int casrGetSNR(CASR *c, unsigned short *snr)
{
    int err;

    err = casrCheckHandle(c);
    if (err)
        return err;
    if (snr == 0)
        return _tosErrorLog(c->event, CASR_ERR_ARG);
    c->last_error = 0;
    _tosControl(c->event, 5, (int)snr, 0);
    return (unsigned char)c->last_error;
}

int casrGetState(CASR *c, int *state)
{
    if (c == 0)
        return CASR_ERR_NULL;
    if (c->magic != CASR_MAGIC)
        return CASR_ERR_HANDLE;
    if (state == 0)
        return _tosErrorLog(c->event, CASR_ERR_ARG);
    *state = c->state;
    return 0;
}

/* OPEN: the target sets the result 0 before testing ctx, keeps the type it loads in a1 and
 * stores data before heap; the guard and single-exit shapes tried miss by 18-25 words. */
IMPORT *casrImportContext(void *heap, int *ctx)
{
    IMPORT *p = 0;

    if (ctx != 0 && *ctx == 6) {
        p = heap_Alloc(heap, sizeof(IMPORT));
        if (p != 0) {
            p->heap = heap;
            p->data = ctx;
        }
    }
    return p;
}

IMPORT *casrImportData(void *heap, void *data)
{
    IMPORT *p;

    if (data == 0)
        return 0;
    p = heap_Alloc(heap, sizeof(IMPORT));
    if (p == 0)
        return 0;
    p->data = data;
    p->heap = heap;
    return p;
}

/* the words a context may recognise: each id checked against the context's word count */
int casrSetActiveWords(CASR *c, unsigned int *ids, unsigned int n)
{
    int err;
    CTXVT vt;
    unsigned int nwords;
    unsigned int i;

    err = casrCheckHandle(c);
    if (err)
        return err;
    if (ids == 0 && n != 0)
        return _tosErrorLog(c->event, CASR_ERR_ARG);
    if (c->iso_word)
        return func_004D5058(c, ids, n);
    FillContextV2VirtualTable(&vt);
    vt.data = c->ctx->data;
    nwords = vt.GetNbrWord(&vt);
    for (i = 0; i < n; i++)
        if (ids[i] >= nwords)
            return _tosErrorLog(c->event, 25);
    return _tosControl(c->event, 25, n != 0 ? (int)ids : 0, n);
}

short *casrGetSegmStart(CASR *c, int i, int j)
{
    return c->segtab[i * 3 + j];
}

void casrStoreSegmentation(CASR *c, int i, int j, SEGLABEL *l)
{
    short *p = c->segtab[i * 3 + j];

    if (l != 0) {
        do {
            *p++ = l->label;
            l = l->next;
        } while (l != 0);
    } else
        *p = -1;
}

int casrEnableLabelInQueue(CASR *c, unsigned char enable)
{
    if (enable)                         /* the two calls are cross-jumped into one */
        return qQueueControl(c->queue, 5, c->queue_id, 0) == 0;
    return qQueueControl(c->queue, 8, c->queue_id, 0) == 0;
}

/* OPEN: the target tests the result at the join with a branch (bnez; li 1; move 0) where every
 * shape tried (?:, if/return, != 0, a flag variable) compiles to sltu. */
int casrDoSecondPass(CASR *c)
{
    int ok;

    if (c->use_dp)
        ok = casrDoSpelling(c);
    else if (c->iso_word)
        ok = casrDoIsoWordSecondPass(c);
    else
        ok = casrDoNormalSecondPass(c);
    return ok ? 1 : 0;
}

int cbStateHandler(CASR *c, unsigned short state)
{
    int r;

    if (c->state_cb != 0) {
        r = c->state_cb(state, c->frames * 110, c->info);
        if (c->last_state == 0 && r != 0) {
            c->last_state = r;
            _tosErrorLog(c->event, 27);
        }
    }
    return 0;
}

int casrSetParamNoClearErrorLog(CASR *c, int id, unsigned int value)
{
    int err = c->last_error;
    int r = casrSetParam(c, id, value);

    c->last_error = err;
    return r;
}

int casrGetParamNoClearErrorLog(CASR *c, int id, unsigned int *value)
{
    int err = c->last_error;
    int r = casrGetParam(c, id, value);

    c->last_error = err;
    return r;
}

int cbInitArchitectureQueues(CASR *c, unsigned char arch, unsigned char queues)
{
    if (arch == 1 && queues == 4)
        return 0;
    return 1;
}

int cbRegisterInputQueue(CASR *c, int queue, unsigned short id)
{
    c->queue = queue;
    c->queue_id = id;
    return 0;
}

int cbRegisterOutputQueue(CASR *c, int cb)
{
    int *p;

    if (c->slot[0].cb == 0)
        p = &c->slot[0].cb;
    else if (c->slot[1].cb == 0)
        p = &c->slot[1].cb;
    else if (c->slot[2].cb == 0)
        p = &c->slot[2].cb;
    else if (c->slot[3].cb == 0)
        p = &c->slot[3].cb;
    else
        return 1;
    *p = cb;
    return 0;
}

int cbGainRequest(CASR *c, int a1, int a2, int gain)
{
    int err;

    if (c->gain_cb != 0) {
        err = c->gain_cb(gain, c->info);
        if (c->last_state == 0 && err != 0) {
            c->last_state = err;
            _tosErrorLog(c->event, 27);
        }
    }
    return 0;
}

int cbAskCurrentGain(CASR *c)
{
    int err;

    if (c->askgain_cb != 0) {
        err = c->askgain_cb(c->info);
        if (c->last_state == 0 && err != 0) {
            c->last_state = err;
            _tosErrorLog(c->event, 27);
        }
    }
    return 0;
}

/* an abnormal condition: 70 is told the host as 4 */
int cbTOSAbnormalCond(CASR *c, int a1, int a2, int code, int frames)
{
    if (c->abnorm_cb != 0)
        c->last_state = c->abnorm_cb(code == 70 ? 4 : code, frames * 110, c->info);
    return 0;
}

/* a sentence's confidence, 0..10000, from its score against the garbage model's per frame: a
 * logistic either side of a straight line between -26 and 78. SOCOM 1's calls exp (double) where
 * this build calls expf. 2.96's parser lands an ulp below even an exact decimal for the two slope
 * constants (76.9f comes out 0x4299CCCC, the target's), so they are spelled just above the
 * target's floats (0x3DAEFD7F, 0x3CDFE537); x holds the result too (a second variable: 17 words) */
int CalcSentenceConfidence(unsigned int frames, unsigned int score, unsigned int garbage)
{
    float x;

    x = ((float)score - (float)garbage) / (float)frames;
    if (x < -26.0f)
        x = 10000.0f / (expf(x * 0.085444445f + 0.0273309787f) + 1.0f);
    else if (x <= 78.0f)
        x = (91.0f - x) * 76.9f;
    else
        x = 10000.0f / (expf(x * 0.085444445f + -4.467109f) + 1.0f);
    return (int)x;
}

/* the engine's events, passed on to the host's callbacks (a frame is 11 ms: ms = frames * 110 / 10) */
int cbTSDetected(CASR *c, int a1, int a2, int frame)
{
    int err;

    c->f672 = 1;
    err = c->ts_cb(frame * 110, c->info);
    if (c->last_state == 0 && err != 0) {
        c->last_state = err;
        _tosErrorLog(c->event, 27);
    }
    c->f628 = 0;
    return 0;
}

int cbTSAfterStartDetected(CASR *c, int a1, int a2, int frame)
{
    int err;

    frame += *c->frame_base;
    c->f672 = 1;
    err = c->ts_cb(frame * 110, c->info);
    if (c->last_state == 0 && err != 0) {
        c->last_state = err;
        _tosErrorLog(c->event, 27);
    }
    c->f628 = 0;
    return 0;
}

int cbStartDetected(CASR *c, int a1, int a2, int start, int end)
{
    int err;
    int n;

    n = end - start;
    c->f673 = 1;
    err = c->start_cb(n * 110, c->info);
    if (c->last_state == 0 && err != 0) {
        c->last_state = err;
        _tosErrorLog(c->event, 27);
    }
    return 0;
}

int cbTOSRecognised(CASR *c, int a1, int a2, int frame)
{
    RESULT *r;
    int err;

    casrCreateResult(c, &r, frame, 1);
    err = c->result_cb(r, c->info);
    if (c->last_state == 0 && err != 0) {
        c->last_state = err;
        _tosErrorLog(c->event, 27);
    }
    casrFreeResult(c, r);
    return 0;
}

int cbSpellRecognised(CASR *c, int a1, int a2, int arg1)
{
    c->sp_arg1 = arg1;
    return 0;
}

/* nothing recognised: the result's reason is 2 when the utterance outran the timeout (parameter 5) */
int cbNothingRecognised(CASR *c)
{
    NORESULT r;
    int err;

    r.a = 0;
    r.b = 0;
    if (c->p420 != 0 && c->p420 < c->frames)
        r.reason = 2;
    else
        r.reason = 0;
    err = c->result_cb(&r, c->info);
    if (c->last_state == 0 && err != 0) {
        c->last_state = err;
        _tosErrorLog(c->event, 27);
    }
    return 0;
}

int cbSpellNothingRecognised(CASR *c)
{
    NORESULT r;
    int err;

    r.a = 0;
    r.b = 0;
    if (c->p420 != 0 && c->p420 < c->frames)
        r.reason = 2;
    else
        r.reason = 0;
    err = c->result_cb(&r, c->info);
    if (c->last_state == 0 && err != 0) {
        c->last_state = err;
        _tosErrorLog(c->event, 27);
    }
    return 0;
}

int cbDoSecondPass(CASR *c, int a1, int a2, int arg1, int arg2)
{
    c->second_pass = 1;
    c->sp_arg2 = arg2;
    c->sp_arg1 = arg1;
    return 0;
}

int cbNewUserWordUtterance(CASR *c)
{
    c->f674 = 0;
    c->frames = 0;
    c->f906 = 0;
    c->f672 = 0;
    c->uw_state = 4;
    return 0;
}

/* OPEN: the target tests ==1, ==0, ==2 with no range check, its case 0 storing through the
 * join (reorg); a switch tests slti 2 first, the if-chains tried miss by 14-19 words. */
int cbGenderDisable(CASR *c, int a1, int a2, int which, int mode)
{
    c->gender_on[which] = 0;
    switch (mode) {
    case 0:
        c->gender = 1;
        break;
    case 1:
        c->gender = 2;
        break;
    case 2:
        c->gender = 4;
        break;
    default:
        return 1;
    }
    return 0;
}

/* the end-of-backtrace bit (2) of each listed state in a state dictionary, set or cleared; the
 * dictionary's entries are shorts from 4 before the pointer passed. The entry is a pointer of its
 * own (indexing gives the address sum's operands the other way round, 62 words); 2.96 leaves the
 * setting loop rolled where SOCOM 1's compiler unrolled both. */
void Set_EndBackTraceMask_In_StateDict(unsigned char set, unsigned short *dict, int *first,
                                       int *last)
{
    unsigned short *e;
    int *p;

    dict -= 4;
    if (set) {
        for (p = first; p <= last; p++) {
            e = dict + *p;
            *e |= 2;
        }
    } else {
        for (p = first; p <= last; p++) {
            e = dict + *p;
            *e &= ~2;
        }
    }
}

/* a copy of a context's info record (its size at +4), on the given heap */
typedef struct CTXINFO {
    int kind;
    int size;                           /* +4: the record's bytes */
} CTXINFO;

void *CopyContextInfo(void *heap, CTXINFO *info)
{
    void *p;

    p = heap_Alloc(heap, info->size);
    if (p != 0)
        memcpy(p, info, info->size);
    return p;
}
