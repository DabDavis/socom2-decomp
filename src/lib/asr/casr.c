/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing -funroll-loops -fno-optimize-sibling-calls */
/* casr: the recogniser's engine API (OVL2 0x4D8398-0x4DE490, the head of SOCOM 1's casr object at
 * 0x348248; its tail, casrGetLastError .. cbGenderDisable, is cb.c), ScanSoft's speech recogniser,
 * prebuilt with ee-gcc 2.96 at -O2 -G0 -fno-strict-aliasing -funroll-loops (README "The GCC
 * front") and -fno-optimize-sibling-calls: casrStartUserWordTraining returns its call through a
 * jal (18s with sibling calls), and the object -- 0x4D8398-0x4DF2C8 -- holds no `j`. Names are SOCOM 1's (s1:align/calls/order), the engine context's fields casr.h's, named
 * by their use. Every entry point checks its handle through an inline returning 6 (null) or 11
 * (not a live engine): 2.96 if-converts the magic test into `li 11; movz`, and a caller that
 * returns its own constant drops the 6. Only the functions written so far are here, in image
 * order. */

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
extern void FillContextV2VirtualTable(void *vt);
extern int casrDoSecondPass(CASR *c);
extern int tosDestruct(CASR *c);
extern int CreateExEvCtxInfo(CASR *c);
extern void FreeCtxInfo(CASR *c);
extern int GetIsolatedWordDpActiveWords(CASR *c, int **words, int *n);
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

static int casrGetSessionData(CASR *c, void **data, int *size);
static int casrReadLabelInQueue(CASR *c, int n);

/* the casr object's .rodata: the blocks a labelled utterance activates */
static const unsigned char ActiveBlocks_UTTFORMAT_LABELS[3] = { 0x1F, 0x20, 0 };
static const unsigned char ActiveBlocks_UTTFORMAT_LABELS_ISO[2] = { 0x23, 0 };

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
    if (_tosControl(c->event, 73, &c->p484, 0) != 0)
        return 1;
    return _tosControl(c->event, 74, &c->p488, 0);
}

/* OPEN (1 word): the last test's branch -- the target's `bnez v0` keeps `li v0,1` in a plain delay
 * slot (the fall-through's `move v0,zero` overwrites it), 2.96 here annuls it (`bnezl`): reorg's view
 * of v0 at the fall-through. SOCOM 1's copy tests a copy (`move v1,v0; bnez v1`). Tried: the return
 * as unsigned char/int/long/short, _tosControl as int/unsigned char/signed char/unsigned short, every
 * error exit a `return 1` or one `goto error`, the else-if and nested forms (build/scratch/gcc/p/
 * s_ues*.py, 30 spellings: 1 to 44s). */
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
