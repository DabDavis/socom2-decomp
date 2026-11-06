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
static int casrEnableGenders(CASR *c, int genders);
extern int casrActivateContext(CASR *c, void *ctx, int a);
extern int SetIsolatedWordDpActiveWords(CASR *c, unsigned int *ids, unsigned int n);
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
    void *s10[5];
    int (*IsSpellingContext)(CTXVT *vt);            /* +60, slot 15 */
    void *s16[4];
    int (*IsIsoWordContext)(CTXVT *vt);             /* +80, slot 20 */
};

/* the context the engine runs (+580), as far as parameter 15 reads it: its state dictionary and
 * a table whose entry n + 2 starts the end states' list, entry n the index of the list's last */
typedef struct RUNCTX {
    int pad0;
    int n;                              /* +4 */
    char pad8[8];
    unsigned short *dict;               /* +16 */
    char pad20[8];
    int *tab;                           /* +28 */
} RUNCTX;

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
 * Round 24c, byte-exact: the second pass's two arguments stored before its flag (8 of the 24
 * orders of the arm's four statements match, build/scratch/gcc/r24c/v_dsp.py; round 9c's order --
 * the flag between them -- was 9 words): the flag's constant 1 then dies before `err = 0`, so err,
 * shared by the three arms, is born in v0, the return register (the GCC front's round-31a rule),
 * and the iso branch's slot computes the `== 1`. */
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
        c->sp_arg2 = arg2;
        c->second_pass = 1;
        err = 0;
    } else {
        if (_tosControl(c->event, 7, 0, 0) != 0)
            return 1;
        if (c->iso_word)
            err = cbIsoWordRecognised(c, a1, a2, arg1, arg2);
        else {
            casrCreateResult(c, &r, arg1, 1);
            ret = c->cb.result_cb(r, c->info);
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

extern void *memset(void *dst, int c, unsigned int n);

/* The command tables are statics of two inline helpers, each its own function: SOCOM 1's local
 * statics are ControlID_Disable.84, ControlID_Enable.88 and ControlID_Disable.89, and SOCOM 1's
 * compiler (a 2.9x/2.95: build/scratch/gcc/lblprobe.py) numbers three hidden names
 * (__FUNCTION__, __PRETTY_FUNCTION__, __func__) at every function's start -- 85 to 87 are a
 * function boundary between the stop table and the switch tables (round 32c; 2.96 numbers
 * statics consecutively, so SOCOM II's image cannot tell). */

/* gender i's recogniser stopped (command 108 + i, its warp value handed over) and its flag set */
static __inline__ unsigned char casrStopGender(CASR *c, unsigned int i, unsigned char on, int value)
{
    static const unsigned char ControlID_Disable[3] = { 108, 109, 110 };

    if (_tosControl(c->event, ControlID_Disable[i], i, value) != 0)
        return 1;
    c->gender_on[i] = on;
    return 0;
}

/* gender i's recogniser switched on or off (command 64 + i or 61 + i) and its flag set */
static __inline__ unsigned char casrSwitchGender(CASR *c, unsigned int i, unsigned char on, int value)
{
    static const unsigned char ControlID_Enable[3] = { 64, 65, 66 };
    static const unsigned char ControlID_Disable[3] = { 61, 62, 63 };
    const unsigned char *ids = on ? ControlID_Enable : ControlID_Disable;

    if (_tosControl(c->event, ids[i], i, value) != 0)
        return 1;
    c->gender_on[i] = on;
    return 0;
}

/* the genders in the mask enabled, the rest disabled: 1 when the mask names none the language's
 * warp factors serve, or (but for all, 0x7FFFFFFF or -1) one they do not; each gender whose state
 * changes switched on or off, and those switched off then stopped. SOCOM 1's body (round 31c).
 * Round 32c closed it: the switch loop is peeled once and unrolled by two (as SOCOM 1's build)
 * because loop.c's second pass counts 34 insns there (3 x 34 >= 100: not unrolled completely) --
 * each loop's flag a variable of its own block, set `on = 0; if (...) on = 1;` (jump.c keeps the
 * set-then-branch one insn longer until if-conversion makes it the sltu); the stop loop's table a
 * static of its helper, so its address is a register plus i (`addu v0,s3,a1`) where an argument
 * constant gives a strength-reduced walker; the block's own flag gives each unrolled copy its
 * own register (s1, s2) where one flag shared with the switch loop lives in s0 throughout.
 * 33 insns at loop.c's second pass unroll the switch loop completely (the 0x3DC of round 31c). */
static int casrEnableGenders(CASR *c, int genders)
{
    unsigned char changed[3];
    unsigned int n, i, mask;
    WARP *warp;

    mask = 0;
    memset(changed, 0, 3);
    if (c->lng == 0)
        return 0;
    i = 0;
    n = c->lng->GetNbrWarpFactors(c->lng);
    warp = c->lng->GetpWarpFactors(c->lng);
    for (i = 0; i < n; i++)
        mask |= warp[i].mask;
    if ((genders & mask) == 0)
        return 1;
    if (genders != 0x7FFFFFFF && genders != -1 && (genders & ~mask) != 0)
        return 1;
    for (i = 0; i < 3; i++) {
        unsigned char on = 0;

        if (warp[i].mask & genders)
            on = 1;
        if (on != c->gender_on[i]) {
            changed[i] = 1;
            if (casrSwitchGender(c, i, on, warp[i].value))
                return 1;
        }
    }
    for (i = 0; i < 3; i++) {
        unsigned char on = 0;

        if (warp[i].mask & genders)
            on = 1;
        if (!on && changed[i])
            if (casrStopGender(c, i, on, warp[i].value))
                return 1;
    }
    return 0;
}

/* the scheduler's callbacks into the engine (tos.c's TOSCB) */
typedef struct TOSCB {
    int (*state)(CASR *c, unsigned short state);
    int (*init_queues)(CASR *c, unsigned char arch, unsigned char queues);
    int (*input_queue)(CASR *c, int queue, unsigned short id);
    int (*output_queue)(CASR *c, int cb);
    void (*error_cb)(CASR *c, int code);
    void (*warning_cb)(CASR *c, int code);
} TOSCB;

extern unsigned char tosInit(CASR *t, void *heap, TOSCB *cb, const void *profiles,
                             const void *procs, const void *writers, const void *readers,
                             void *cmds, unsigned short arch, unsigned short mode);
extern void *heap_Calloc(void *heap, int n, int size);
int cbStateHandler(CASR *c, unsigned short state);
int cbInitArchitectureQueues(CASR *c, unsigned char arch, unsigned char queues);
int cbRegisterInputQueue(CASR *c, int queue, unsigned short id);
int cbRegisterOutputQueue(CASR *c, int cb);
int cbTrain(CASR *c, int a1, int a2, int state);
static int InitParamList(CASR *c);

/* the recogniser's architecture (SOCOM 1's GLOBALs, another object's): the blocks' profiles,
 * the blocks built, the queues each writes and reads, the states' command lists */
extern const unsigned char IniFile[], InitialProcs[], QueueWriters[], QueueReaders[];
extern void *ControlTable[];

/* an engine: the scheduler built over the recogniser's architecture (the context is its own
 * scheduler record and log head), the host's callbacks kept, the default parameters set; null
 * without callbacks or on any failure (the context freed) */
CASR *casrOpen(void *heap, const CASRCBTABLE *cbs, void *user)
{
    TOSCB cb = { cbStateHandler, cbInitArchitectureQueues, cbRegisterInputQueue,
                 cbRegisterOutputQueue, 0, 0 };
    CASR *c;

    if (cbs == 0)
        return 0;
    c = heap_Calloc(heap, 1, sizeof(CASR));
    if (c == 0)
        return 0;
    c->heap = heap;
    c->event->tos = c;
    c->event->block = 0;
    c->cb = *cbs;
    c->info->f0 = 0;
    if (tosInit(c, heap, &cb, IniFile, InitialProcs, QueueWriters, QueueReaders, ControlTable,
                0, 0) != 0)
        goto fail;
    c->train = cbTrain;
    c->train_state = 4;
    c->uw_recs_needed = 3;
    c->uw_recs_done = 0;
    c->segtab = heap_Alloc(c->heap, 40);
    if (c->segtab == 0) {
        c->last_error = 2;
        goto fail;
    }
    c->segtab[0] = 0;
    c->f1052 = 0.0001f;
    if (_tosControl(c->event, 4, (int)&c->signal, 0) != 0)
        goto fail;
    if (_tosControl(c->event, 42, (int)&c->frame_base, 0) != 0)
        goto fail;
    if (InitParamList(c) != 0)
        goto fail;
    c->info->user = user;
    c->magic = CASR_MAGIC;
    c->gender_on[0] = 0;
    c->gender_on[1] = 0;
    c->gender_on[2] = 0;
    return c;
fail:
    heap_Free(c->heap, c);
    return 0;
}

/* the session's data: the record (its kind, its size, the three genders' flags and the sample
 * format, the language file's header) and the engine's own data after it, both sized by command
 * 39. Round 26c, byte-exact: the target stores the first gender and the format through `g` (s+8)
 * and the other two off s, each as it is spelled -- the store order decides the rest: the size
 * before the kind (its reload of `n` then heads the block and the language's pointer takes a0;
 * with the kind first, 38 words, every placement of `g` and `h` alike: build/scratch/gcc/v_gsd2.py).
 * (Round 24c's sweep of the 16 spellings, the kind first, read "one form" from 38 words each.)
 * The header's destination is a local pointer (`h`): with `s->lng` itself 2.96 folds the data
 * pointer to s+152 and computes the copy's bound once (87s), the demo's shape. */
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
    s->size = n;
    s->kind = 4;
    g = s->gender;
    g[0] = c->gender_on[0];
    s->gender[1] = c->gender_on[1];
    s->gender[2] = c->gender_on[2];
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
        r = c->cb.train_cb(old, c->info);
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

/* the extra events' state sent to the engine: on (the language's bit 64) the start, the counter's
 * address, the context info and the info itself, else the stop; 1 on any failure (the context
 * info freed when it was made). Round 24c, byte-exact: the shared failure return is the else-if's
 * own body, its label inside the block (`} else if (...) { error: return 1; } return 0;`) -- with
 * the label after a final `return 0` (round 9c-13c's 30 spellings) the else-if's fall-through
 * shares the then-arm's `return 0` block (its label's one jump is the then-arm's last test: the
 * -dd dump), reorg copies that block's `move v0,zero` into the then-arm's beqzl and redirects it,
 * the label dies, and the else-if's slot is annulled (`bnezl`, 1 word) -- read as resource.c's
 * live-register scan from the block's head, which drops a REG_DEAD register (v0, the call's
 * value) only at the next CODE_LABEL. */
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
    } else if (_tosControl(c->event, 101, 0, 0) != 0) {
error:
        return 1;
    }
    return 0;
}

/* the object's own Set_EndBackTraceMask_In_StateDict (its row is the end of the object), inlined
 * into casrSetParam: an extern inline here, the definition at the end its redefinition (the same
 * body: walking the parameter itself, the inlined copy's walker is set before the set test) */
extern __inline__ void Set_EndBackTraceMask_In_StateDict(unsigned char set, unsigned short *dict,
                                                         int *first, int *last)
{
    unsigned short *e;

    dict -= 4;
    if (set) {
        for (; first <= last; first++) {
            e = dict + *first;
            *e |= 2;
        }
    } else {
        for (; first <= last; first++) {
            e = dict + *first;
            *e &= ~2;
        }
    }
}

/* a parameter set (casrGetParam's ids, the same case order) and sent to the engine in its group's
 * record: 81 the timing group at +456, 84 the group at +488, 83 the two levels, 80 the pair at
 * +444; in the states the engine is idle in (0, 1, 2, 5, 6, 9, 10, 13, 14, 20), else 7. 15 the
 * end-of-backtrace bit of the running context's end states (the language's bit 16 refuses it on,
 * bit 8 asks for the bits), 16 a context activated again (put back on failure), 14 the genders.
 * SOCOM 1's body (0x62C). Round 37c, byte-exact: every case that ends in a call returns the error
 * itself (`return (unsigned char)c->last_error`, the failure exits too) -- jump2 first moves each
 * such return onto the one after the switch (code before the return label), so each case's jump
 * joins that label's chain only as it is processed and the later cases cross-jump into case 1's
 * send (with `break`, every jump is in the chain from the start and case 14's copy is kept, 281
 * words); case 4's failure return keeps its own first send (a label stops the cross-jump). The
 * end states: n a local (GCSE reuses n * 4 for tab[n]), last a statement in the arm (an inline's
 * argument is expanded as a sum, the shift first), and the inline walks its own `first` (the copy
 * comes before the set test, so tab cannot share p's a2: dict a2, tab a3) */
int casrSetParam(CASR *c, int id, unsigned int value)
{
    int err;
    int state;
    LANGVT *lng;
    RUNCTX *rc;
    int *first;
    int *last;
    int n;
    unsigned char on;
    int old;
    int r;
    CTXVT vt;

    err = casrCheckHandle(c);
    if (err)
        return err;
    state = c->state;
    switch (state) {
    case 0:
    case 1:
    case 2:
    case 5:
    case 6:
    case 9:
    case 10:
    case 13:
    case 14:
    case 20:
        break;
    default:
        return _tosErrorLog(c->event, 7);
    }
    c->last_error = 0;
    switch (id) {
    case 1:
        c->p460 = value;
        c->p496 = value;
        if (_tosControl(c->event, 81, (int)&c->f456, 0) != 0)
            return (unsigned char)c->last_error;
        _tosControl(c->event, 84, (int)&c->p488, 0);
        return (unsigned char)c->last_error;
    case 2:
        c->f456 = value;
        _tosControl(c->event, 81, (int)&c->f456, 0);
        return (unsigned char)c->last_error;
    case 3:
        c->p504 = value;
        _tosControl(c->event, 84, (int)&c->p488, 0);
        return (unsigned char)c->last_error;
    case 4:
        c->p464 = value;
        c->p500 = value * 2;
        if (_tosControl(c->event, 81, (int)&c->f456, 0) != 0)
            return (unsigned char)c->last_error;
        _tosControl(c->event, 84, (int)&c->p488, 0);
        return (unsigned char)c->last_error;
    case 5:
        c->p420 = value * 100;
        break;
    case 15:
        lng = c->lng;
        if (lng != 0) {
            if (value != 0 && lng->CheckBitField(lng, 16) != 0)
                return _tosErrorLog(c->event, 28);
            if (lng->CheckBitField(lng, 8) != 0 && (rc = c->run_ctx) != 0) {
                n = rc->n;
                first = rc->tab + n + 2;
                on = rc->dict[*first - 4] & 2;
                if ((value != 0 && on == 0) || (value == 0 && on != 0)) {
                    last = rc->tab + rc->tab[n];
                    Set_EndBackTraceMask_In_StateDict(value, rc->dict, first, last);
                }
            }
        }
        c->p424 = value;
        break;
    case 18:
        c->exev = value;
        if (c->ctx != 0)
            UpdateExevStatus(c);
        return (unsigned char)c->last_error;
    case 16:
        old = c->f1088;
        c->f1088 = value;
        if (c->ctx != 0) {
            FillContextV2VirtualTable(&vt);
            vt.data = c->ctx->data;
            if (vt.IsIsoWordContext(&vt) != 0)
                break;
            if (vt.IsSpellingContext(&vt) != 0)
                break;
            if (c->f1088 != c->iso_word) {
                err = casrActivateContext(c, c->ctx, c->f1092);
                if (err != 0) {
                    c->f1088 = old;
                    r = casrActivateContext(c, c->ctx, c->f1092);
                    if (r != 0)
                        return r;
                    return err;
                }
            }
        }
        break;
    case 6:
        c->p488 = value;
        _tosControl(c->event, 84, (int)&c->p488, 0);
        return (unsigned char)c->last_error;
    case 7:
        c->p484[1] = 50 - value;
        _tosControl(c->event, 83, (int)c->p484, 0);
        return (unsigned char)c->last_error;
    case 8:
        c->p484[0] = 50 - value;
        _tosControl(c->event, 83, (int)c->p484, 0);
        return (unsigned char)c->last_error;
    case 9:
        c->p468 = value;
        _tosControl(c->event, 81, (int)&c->f456, 0);
        return (unsigned char)c->last_error;
    case 10:
        c->p472 = value;
        _tosControl(c->event, 81, (int)&c->f456, 0);
        return (unsigned char)c->last_error;
    case 17:
        c->p476 = value;
        _tosControl(c->event, 81, (int)&c->f456, 0);
        return (unsigned char)c->last_error;
    case 11:
        c->p444[0] = value;
        _tosControl(c->event, 80, (int)c->p444, 0);
        return (unsigned char)c->last_error;
    case 13:
        c->p492 = value;
        _tosControl(c->event, 84, (int)&c->p488, 0);
        return (unsigned char)c->last_error;
    case 12:
        c->p444[1] = value;
        _tosControl(c->event, 80, (int)c->p444, 0);
        return (unsigned char)c->last_error;
    case 14:
        if (state == 0)
            return _tosErrorLog(c->event, 33);
        if (casrEnableGenders(c, value) != 0)
            return _tosErrorLog(c->event, 20);
        c->gender = value;
        if (_tosControl(c->event, 82, (int)&c->gender, 0) != 0)
            return (unsigned char)c->last_error;
        _tosControl(c->event, 38, 0, 0);
        return (unsigned char)c->last_error;
    default:
        return _tosErrorLog(c->event, 10);
    }
    return (unsigned char)c->last_error;
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

/* the segmentation's table: one block of 3 x (each utterance's length + 2) shorts per gender, and
 * for each of the three utterances three pointers into it (one per gender). Round 26c, byte-exact:
 * the table's own pointer `seg` is the running pointer (`*++seg`). The inner loop is unrolled
 * first, its biv remapped to a copy of its value at the row's start (cc1's "Biv initial value
 * remapped": the target's `move a1,s1; addiu s1,a1,12` per row); with a second pointer `q` from
 * `seg`, CSE rewrites every row against the unchanged `seg` and the rows fold into constant offsets
 * of one base (44 words, 6 short -- pre- and post-increment, an index, rows written out, a while
 * all did). The product is assigned back to the total (`total *= 3`): the result is then the
 * total's own pseudo, in a1, with 2t in v0 (`addu a1,v0,a1`); a product in the call's argument
 * ties the result to 2t, the first operand (8 words), `(total + total * 2)` swaps the add's
 * operands (1). SOCOM 1's copy is rolled: unsigned counters, the store through a second
 * pointer. */
void casrInitSegmentation(CASR *c)
{
    short **seg = c->segtab;
    short *p;
    unsigned int i, j;
    int len, total;

    if (seg[0] != 0)
        heap_Free(c->heap, seg[0]);
    total = 0;
    for (i = 0; i < 3; i++)
        total += c->utt[i].n + 2;
    total *= 3;
    p = seg[0] = heap_Alloc(c->heap, total * sizeof(short));
    for (i = 0; i < 3; i++) {
        len = c->utt[i].n + 2;
        for (j = 0; j < 3; j++) {
            p += len;
            *++seg = p;
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
        r = c->cb.train_cb(old, c->info);
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

/* The getters' failure return is written after the success path (round 25c): the target lays the
 * success block first, its jump to the return turned into its own `jr` (the store in its slot), and
 * the two failure tests' shared `return 11` last -- reorg then fills the null test's slot from the
 * fall-through (the magic's lui).
 * Rounds 2b-24c had the guard first (a plain guard, a ?: macro, an inline checker, a single-exit
 * flag): one shared return, each failure branch's slot stealing the `li v0,11` (0x30-0x3C). The
 * magic test is the inline checker's (`li 11; movz` -- its own null test folded into the caller's;
 * round 24c). Four spellings of the layout match (an && guard, nested ifs, a goto, if/else into
 * one result); written as the first. */
int casrGetLastError(CASR *c)
{
    int err;

    if (c != 0 && casrCheckHandle(c) == 0) {
        err = c->last_error;
        c->last_error = 0;
        return err;
    }
    return CASR_ERR_HANDLE;
}

int casrGetLastWarning(CASR *c)
{
    int err;

    if (c != 0 && casrCheckHandle(c) == 0) {
        err = c->last_warning;
        c->last_warning = 0;
        return err;
    }
    return CASR_ERR_HANDLE;
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

/* One guard per test, each its own `return 0` (round 25c, open since round 2b): the two returns
 * share one `move v0,zero`, which the target issues at the entry ("the result 0 before testing
 * ctx"); with one `||` guard 0x68, a single exit 0x64, a result variable 16 words. The heap is
 * stored first in the source (the scheduler reverses the pair). */
IMPORT *casrImportContext(void *heap, int *ctx)
{
    IMPORT *p;

    if (ctx == 0)
        return 0;
    if (*ctx != 6)
        return 0;
    p = heap_Alloc(heap, sizeof(IMPORT));
    if (p != 0) {
        p->heap = heap;
        p->data = ctx;
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
        return SetIsolatedWordDpActiveWords(c, ids, n);
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

/* Each arm tests its own pass's result (round 25c, open since round 1): `if (pass(c) != 0) return
 * 1;` per arm and one `return 0` -- jump2 cross-jumps the three tests into the one at the join, a
 * branch (bnez; li 1; move 0); a result assigned in the arms and tested once (?:, if/return, != 0,
 * a flag variable) is a store-flag, sltu (0x58), and the arms returning their own `!= 0` or `?:`
 * the same. */
int casrDoSecondPass(CASR *c)
{
    if (c->use_dp) {
        if (casrDoSpelling(c) != 0)
            return 1;
    } else if (c->iso_word) {
        if (casrDoIsoWordSecondPass(c) != 0)
            return 1;
    } else if (casrDoNormalSecondPass(c) != 0)
        return 1;
    return 0;
}

int cbStateHandler(CASR *c, unsigned short state)
{
    int r;

    if (c->cb.state_cb != 0) {
        r = c->cb.state_cb(state, c->frames * 110, c->info);
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

    if (c->cb.gain_cb != 0) {
        err = c->cb.gain_cb(gain, c->info);
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

    if (c->cb.askgain_cb != 0) {
        err = c->cb.askgain_cb(c->info);
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
    if (c->cb.abnorm_cb != 0)
        c->last_state = c->cb.abnorm_cb(code == 70 ? 4 : code, frames * 110, c->info);
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
    err = c->cb.ts_cb(frame * 110, c->info);
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
    err = c->cb.ts_cb(frame * 110, c->info);
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
    err = c->cb.start_cb(n * 110, c->info);
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
    err = c->cb.result_cb(r, c->info);
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
    err = c->cb.result_cb(&r, c->info);
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
    err = c->cb.result_cb(&r, c->info);
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

/* the training's events to the host (the scheduler's hook casrOpen installs at +544): the host's
 * first refusal kept and logged, as the other callbacks' */
int cbTrain(CASR *c, int a1, int a2, int state)
{
    int err;

    err = c->cb.train_cb(state, c->info);
    if (c->last_state == 0 && err != 0) {
        c->last_state = err;
        _tosErrorLog(c->event, 27);
    }
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

/* a touch tone: flagged for the acquisition */
int cbDTMFDetected(CASR *c)
{
    c->f674 = 1;
    return 0;
}

/* the gender the engine disabled, and the gender mask that is left: the mode is unsigned (an
 * enum): the switch then tests 1, 0, 2 by equality, as the target does -- an int takes slti 2
 * first (round 1's diagnosis, open until round 13c) */
int cbGenderDisable(CASR *c, int a1, int a2, int which, unsigned int mode)
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

    dict -= 4;
    if (set) {
        for (; first <= last; first++) {
            e = dict + *first;
            *e |= 2;
        }
    } else {
        for (; first <= last; first++) {
            e = dict + *first;
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
