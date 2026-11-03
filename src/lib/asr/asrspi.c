/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing -funroll-loops */
/* asrspi: the recogniser's service-provider interface (OVL2 0x4C8F40-0x4CCE90, SOCOM 1's object
 * at 0x33A768, ConvErr .. ASRSPICBRESULT), ScanSoft's speech recogniser, prebuilt with ee-gcc
 * 2.96 at -O2 -G0 -fno-strict-aliasing -funroll-loops (README "The GCC front"). Names are SOCOM
 * 1's: the object's functions in the demo's order (s1:order/align/calls), its statics from the
 * demo's symtab (g_StateError, g_APIRecogCallBacks). Only the functions written so far are here,
 * in image order.
 *
 * An engine is a 0xD8-byte block on the recogniser's heap around a casr handle. Every entry
 * point checks the handle (null, then the magic), takes the engine's busy flag under its
 * critical section, checks the state the call is allowed in (g_StateError: the error each state
 * answers with), calls the casr function and turns its error into the API's (ConvErr). Errors
 * are ScanSoft's 0x80MMxxxx codes, MM the module (0x04 here). */

#include "casr.h"

#define ASRSPI_MAGIC        11111

#define ASRSPI_E_HANDLE     0x80040001
#define ASRSPI_E_BUSY       0x80040003
#define ASRSPI_E_NULLPTR    0x80040083

typedef struct CRITSEC CRITSEC;

/* the host's signal callbacks (SOCOM 1's AsrSpiSignalCallBacks: asrspi_cbAbnorm,
 * asrspi_cbSpeechDetected, asrspi_cbTsDetected, asrspi_cbAgc, asrspi_cbEnergyLevel,
 * asrspi_cbSNRLevel), copied into the engine */
typedef struct SIGNALCB {
    int (*abnorm)(int what, int value, int user);       /* +0 */
    int (*speech_detected)(int frame, int user);        /* +4 */
    int (*ts_detected)(int frame, int user);            /* +8 */
    int (*agc)(int gain, int user);                     /* +12 */
    int (*energy_level)(short level, int user);         /* +16 */
    int (*snr_level)(short snr, int user);              /* +20 */
} SIGNALCB;

/* the host's recognition callbacks (SOCOM 1's AsrSpiRecogCallBacks: asrspi_cbResult and the
 * signal callbacks); a user word's training passes its trained callback in the first slot */
typedef struct RECOGCB {
    void *result;                       /* +0 */
    SIGNALCB *signal;                   /* +4 */
} RECOGCB;

typedef int (*USWTRAINED)(void *rec, void *rec_info, int user);

typedef struct CASRRESULT {             /* what the casr engine reports a result with */
    int f0;
    int f4;
    unsigned int status;                /* +8: an enum (unsigned: the switch tests 0 by equality) */
} CASRRESULT;

typedef struct RESULT {                 /* the result the host's callback receives */
    int kind;                           /* +0 */
    int f4;                             /* +4 */
    int f8;                             /* +8 */
} RESULT;

typedef struct CONTEXTV2 {              /* a context's accessors (contextv2.c) */
    int *data;
    int (*CheckDataType)(struct CONTEXTV2 *c);
    int (*GetSize)(struct CONTEXTV2 *c);
    char *(*GetLanguageVersionInfo)(struct CONTEXTV2 *c);
    void *fn4[5];                       /* GetTS .. GetNbrItem */
    unsigned int (*GetNbrWord)(struct CONTEXTV2 *c);
    void *fn10[11];
} CONTEXTV2;

typedef struct LANG {                   /* a language's accessors (language.c) */
    int *data;
    int (*CheckDataType)(struct LANG *l);
    int (*GetSize)(struct LANG *l);
    char *(*GetVersionInfo)(struct LANG *l);
    void *fn[75];
} LANG;

typedef struct ASRSPI {
    int magic;                          /* +0: ASRSPI_MAGIC */
    CASR *casr;                         /* +4 */
    int *ctxdata;                       /* +8: the active context's data */
    void *ctx;                          /* +12: the active context (casrImportContext) */
    void *data;                         /* +16: the active language (casrImportData) */
    int user;                           /* +20: the host's data, passed to its callbacks */
    int start_user;                     /* +24: asrspi_Start's */
    int got_result;                     /* +28: a result or an accepted utterance arrived */
    int trained;                        /* +32 */
    int utt_done;                       /* +36 */
    void *rec;                          /* +40: the user word's recording */
    void *rec_info;                     /* +44 */
    int verdict;                        /* +48 */
    int state;                          /* +52 */
    int busy;                           /* +56 */
    int level_interval;                 /* +60: frames between the level callbacks */
    RESULT result;                      /* +64 */
    SIGNALCB signal;                    /* +76 */
    int (*result_cb)(RESULT *r, int user);      /* +100 */
    USWTRAINED trained_cb;              /* +104 */
    unsigned short gain;                /* +108 */
    int aborting;                       /* +112 */
    int pad116;
    int open_mode;                      /* +120 */
    CONTEXTV2 ctxv2;                    /* +124: the active context's accessors */
    void *heap;                         /* +208 */
    CRITSEC *cs;                        /* +212 */
} ASRSPI;                               /* 0xD8 */

typedef struct PARAM {                  /* a parameter list's entry */
    int id;
    int value;
} PARAM;

typedef struct CASRCB {                 /* what the casr engine hands its callbacks */
    void *f0;
    ASRSPI *engine;                     /* +4: casrOpen's user data */
} CASRCB;

typedef struct CASRCALLBACKS {          /* the callbacks casrOpen takes */
    int (*ts)(int frame, CASRCB *cb);
    int (*startdet)(int frame, CASRCB *cb);
    int (*result)(CASRRESULT *res, CASRCB *cb);
    void *f12;
    int (*train)(int event, CASRCB *cb);
    int (*abnorm)(int event, int value, CASRCB *cb);
    int (*agc)(int gain, CASRCB *cb);
    int (*askgain)(CASRCB *cb);
} CASRCALLBACKS;

extern int critsec_Enter(CRITSEC *cs);
extern int critsec_Leave(CRITSEC *cs);
extern int casrGetLastError(CASR *c);
extern int casrGetLastWarning(CASR *c);
extern int casrGetCallbackErr(CASR *c, int *err);
extern int casrStop(CASR *c);
extern int casrAcknowledge(CASR *c, int what);
extern int casrGetUserWordNbrRecsNeeded(CASR *c, int *n);
extern void *casrImportContext(void *heap, int *ctx);
extern int casrCloseContext(void *ctx);
extern int casrCheckUserWordConfusion(CASR *c, void *ctx, int a2, int a3);
extern int casrSetParam(CASR *c, int id, int value);
extern int casrGetParam(CASR *c, int id, int *value);
extern int casrStartUserWordTraining(CASR *c, void **rec, void **rec_info);
extern void heap_Free(void *heap, void *p);
extern void *memset(void *p, int c, unsigned int n);
extern void *heap_Calloc(void *heap, int count, int size);
extern void *casrOpen(void *heap, const CASRCALLBACKS *callbacks, ASRSPI *e);
extern int casrClose(CASR *c);
extern int casrCloseData(void *data);
extern void *casrImportData(void *heap, int *data);
extern int casrActivateData(CASR *c, void *data);
extern int casrActivateContext(CASR *c, void *ctx, int a2);
extern int casrGetSPIVersion(void);
extern int casrGainSet(CASR *c, unsigned short gain);
extern int casrGetState(CASR *c, int *state);
extern int casrAcquisition(CASR *c, int fmt, char *samples, unsigned int count);
extern int casrGetSignalLevel(CASR *c, short *level);
extern int casrGetSNR(CASR *c, short *snr);
extern int casrStart(CASR *c);
extern int casrExportData(CASR *c, int what, void **data, unsigned int *size);
extern int casrGetActiveWords(CASR *c, int **words, unsigned int *count);
extern int casrSetActiveWords(CASR *c, int *words, unsigned int n);
extern void FillContextV2VirtualTable(CONTEXTV2 *vt);
extern void FillLanguageVirtualTable(LANG *vt);
extern void *heap_Alloc(void *heap, int size);
extern void *memcpy(void *d, const void *s, unsigned int n);

typedef struct SAFEH {                  /* a safe handle, passed by value */
    int id;
    int check;
} SAFEH;

typedef struct RSRC {                   /* the resources InitRsrcFunction hands out */
    int f0;
    void *heap;                         /* +4 */
} RSRC;

extern int InitRsrcFunction(SAFEH h, RSRC **rsrc);

int ASRSPICBTS(int frame, CASRCB *cb);
int ASRSPICBSTARTDET(int frame, CASRCB *cb);
int ASRSPICBRESULT(CASRRESULT *res, CASRCB *cb);
int ASRSPICBTRAIN(int event, CASRCB *cb);
int ASRSPICBABNORM(int event, int value, CASRCB *cb);
int ASRSPICBAGC(int gain, CASRCB *cb);
int ASRSPICBASKCURRENTGAIN(CASRCB *cb);

static const int UswVersionCode = 0x16000001;
static const int g_ApiVersion = 0;
const int _sizeof_asrspi_engine = sizeof(ASRSPI);

/* the error a call answers when the engine's state does not allow it */
static const int g_StateError[6] = {
    0x8004000B, 0x8004000C, 0x8004000D, 0x8004000E, 0x8004000F, 0x80040010
};

/* the entry points' common steps */
#define ENGINE_CHECK(e)                                 \
    if ((e) == 0)                                       \
        return ASRSPI_E_NULLPTR;                        \
    if ((e)->magic != ASRSPI_MAGIC)                     \
        return ASRSPI_E_HANDLE

#define ENGINE_ENTER(e)                                 \
    if ((e)->cs) {                                      \
        critsec_Enter((e)->cs);                         \
        if ((e)->busy) {                                \
            critsec_Leave((e)->cs);                     \
            return ASRSPI_E_BUSY;                       \
        }                                               \
        (e)->busy = 1;                                  \
        critsec_Leave((e)->cs);                         \
    }

/* the state check: 0 when the engine is in one of the states named, else the state's error.
 * An inline per count: its parameters are pseudos the constants are copied into, all before the
 * first test, as the target loads them (a macro's `||` chain folds 1 and 2 into a range test). */
static __inline__ int StateError1(ASRSPI *e, int s1)
{
    if (e->state != s1)
        return g_StateError[e->state];
    return 0;
}

static __inline__ int StateError2(ASRSPI *e, int s1, int s2)
{
    if (e->state != s1 && e->state != s2)
        return g_StateError[e->state];
    return 0;
}

static __inline__ int StateError3(ASRSPI *e, int s1, int s2, int s3)
{
    if (e->state != s1 && e->state != s2 && e->state != s3)
        return g_StateError[e->state];
    return 0;
}

static __inline__ int StateError4(ASRSPI *e, int s1, int s2, int s3, int s4)
{
    if (e->state != s1 && e->state != s2 && e->state != s3 && e->state != s4)
        return g_StateError[e->state];
    return 0;
}

static __inline__ int StateError5(ASRSPI *e, int s1, int s2, int s3, int s4, int s5)
{
    if (e->state != s1 && e->state != s2 && e->state != s3 && e->state != s4 && e->state != s5)
        return g_StateError[e->state];
    return 0;
}

#define ENGINE_LEAVE(e)                                 \
    if ((e)->cs) {                                      \
        critsec_Enter((e)->cs);                         \
        (e)->busy = 0;                                  \
        critsec_Leave((e)->cs);                         \
    }

int ConvErr(ASRSPI *e, int casrErr)
{
    int err;

    switch (casrErr) {
    case 1:  err = 0x8004001A; break;
    case 4:  err = 0x8004001D; break;
    case 5:  err = 0x8004001E; break;
    case 6:  err = 0x8004001F; break;
    case 7:  err = g_StateError[e->state]; break;
    case 8:  err = 0x80040021; break;
    case 9:  err = 0x80040022; break;
    case 10: err = 0x80040014; break;
    case 11: err = 0x80040001; break;
    case 12: err = 0x8004000A; break;
    case 13: err = 0x80040024; break;
    case 15: err = 0x80040026; break;
    case 17: err = 0x80040028; break;
    case 19: err = 0x80040015; break;
    case 20: err = 0x80040029; break;
    case 21: err = 0x8004002A; break;
    case 18: err = 0x8004002B; break;
    case 22: err = 0x8004002C; break;
    case 25: err = 0x80040018; break;
    case 27: casrGetCallbackErr(e->casr, &err); break;
    case 28: err = 0x8004002D; break;
    case 29: err = 0x80040008; break;
    case 30: err = 0x80040009; break;
    case 31: err = 0x8004000A; break;
    case 32: err = 0x80040004; break;
    case 33: err = 0x80040013; break;
    case 34: err = 0x80040034; break;
    case 39: err = 0x8004003A; break;
    case 35: err = 0x00040035; break;
    case 36: err = 0x00040036; break;
    case 37: err = 0x80040038; break;
    case 38: err = 0x80040039; break;
    default: err = 0x8004002E; break;
    }
    return err;
}

int ASRSPICBTRAIN(int event, CASRCB *cb)
{
    ASRSPI *e = cb->engine;
    int err = 0;

    switch (event) {
    case 0:
        err = casrAcknowledge(e->casr, 1);
        e->utt_done = 1;
        break;
    case 1:
        if (e->aborting)
            err = casrAcknowledge(e->casr, 3);
        else
            e->got_result = 1;
        break;
    case 2:
        err = casrAcknowledge(e->casr, 1);
        if (err)
            err = casrGetLastError(e->casr);
        if (err == 27)
            casrGetCallbackErr(e->casr, &err);
        else if (err)
            err = ConvErr(e, err);
        e->utt_done = 1;
        break;
    case 3:
        e->trained = 1;
        if (e->trained_cb && !e->aborting)
            err = e->trained_cb(e->rec, e->rec_info, e->user);
        break;
    case 4:
        break;
    }
    return err;
}

/* the callbacks casrOpen registers for every engine */
static const CASRCALLBACKS g_APIRecogCallBacks = {
    ASRSPICBTS, ASRSPICBSTARTDET, ASRSPICBRESULT, 0,
    ASRSPICBTRAIN, ASRSPICBABNORM, ASRSPICBAGC, ASRSPICBASKCURRENTGAIN,
};

int asrspi_UswStartRecording(ASRSPI *e, int interval, RECOGCB *cb, int user, int setparams)
{
    int err;

    if (cb == 0)
        return ASRSPI_E_NULLPTR;
    if (interval == 0 && cb->signal && (cb->signal->energy_level || cb->signal->snr_level))
        return 0x80040016;
    ENGINE_CHECK(e);
    ENGINE_ENTER(e);
    err = StateError3(e, 1, 6, 2);
    if (err) {
        ENGINE_LEAVE(e);
        return err;
    }
    e->got_result = 0;
    e->trained = 0;
    if (e->state == 6 && e->rec)
        heap_Free(e->heap, e->rec);
    e->level_interval = interval;
    e->rec = 0;
    e->rec_info = 0;
    if (cb->signal)
        e->signal = *cb->signal;
    else
        memset(&e->signal, 0, sizeof e->signal);
    e->trained_cb = cb->result;
    e->user = user;
    if (setparams && (casrSetParam(e->casr, 6, 300) || casrSetParam(e->casr, 4, 300)
                      || casrSetParam(e->casr, 8, 50) || casrSetParam(e->casr, 7, 50))
        || casrStartUserWordTraining(e->casr, &e->rec, &e->rec_info)) {
        err = ConvErr(e, casrGetLastError(e->casr));
        ENGINE_LEAVE(e);
        return err;
    }
    e->state = 4;
    ENGINE_LEAVE(e);
    return 0;
}

int asrspi_UswContinue(ASRSPI *e, int accept, int *trained)
{
    int err;

    if (trained == 0)
        return ASRSPI_E_NULLPTR;
    ENGINE_CHECK(e);
    ENGINE_ENTER(e);
    e->got_result = 0;
    err = StateError1(e, 5);
    if (err) {
        ENGINE_LEAVE(e);
        return err;
    }
    e->utt_done = 0;
    e->verdict = 0;
    if (accept == 1) {
        err = casrAcknowledge(e->casr, 1);
        if (err == 0) {
            err = casrGetLastWarning(e->casr);
            if (err == 35) {
                err = 0;
                e->verdict = 2;
            } else if (err == 36) {
                err = 0;
                e->verdict = 3;
            }
        }
    } else {
        err = casrAcknowledge(e->casr, 2);
        e->verdict = 1;
    }
    *trained = e->trained;
    if (e->trained)
        e->state = 6;
    else
        e->state = 4;
    ENGINE_LEAVE(e);
    if (err)
        return ConvErr(e, casrGetLastError(e->casr));
    return 0;
}

int asrspi_UswCheckConfusion(ASRSPI *e, int *ctx, int a2, int a3)
{
    int err;
    void *c;

    ENGINE_CHECK(e);
    ENGINE_ENTER(e);
    err = StateError1(e, 6);
    if (err == 0) {
        if (ctx == 0) {
            ENGINE_LEAVE(e);
            return 0x8004002B;
        }
        if (ctx[1] == 24 && ctx[0] != 2) {
            ENGINE_LEAVE(e);
            return 0x80040026;
        }
        c = casrImportContext(e->heap, ctx);
        if (c == 0) {
            ENGINE_LEAVE(e);
            return 0x80040009;
        }
        if (casrCheckUserWordConfusion(e->casr, c, a2, a3) != 0) {
            err = ConvErr(e, casrGetLastError(e->casr));
            casrCloseContext(c);
        } else if (casrCloseContext(c) != 0)
            err = ConvErr(e, casrGetLastError(e->casr));
    }
    ENGINE_LEAVE(e);
    return err;
}

int asrspi_UswStopRecording(ASRSPI *e)
{
    int err;

    ENGINE_CHECK(e);
    ENGINE_ENTER(e);
    err = StateError1(e, 4);
    if (err) {
        ENGINE_LEAVE(e);
        return err;
    }
    if (casrStop(e->casr) != 0) {
        ENGINE_LEAVE(e);
        return ConvErr(e, casrGetLastError(e->casr));
    }
    e->state = 5;
    ENGINE_LEAVE(e);
    return 0;
}

int asrspi_UswAbort(ASRSPI *e)
{
    int err;

    ENGINE_CHECK(e);
    ENGINE_ENTER(e);
    err = StateError2(e, 5, 4);
    if (err) {
        ENGINE_LEAVE(e);
        return err;
    }
    e->aborting = 1;
    if (e->state == 5) {
        if (casrAcknowledge(e->casr, 3) != 0) {
            ENGINE_LEAVE(e);
            return ConvErr(e, casrGetLastError(e->casr));
        }
    } else if (casrStop(e->casr) != 0) {
        ENGINE_LEAVE(e);
        return ConvErr(e, casrGetLastError(e->casr));
    }
    e->state = 1;
    e->aborting = 0;
    ENGINE_LEAVE(e);
    return 0;
}

int asrspi_UswGetNbrRecsNeeded(ASRSPI *e, int *n)
{
    int err;

    ENGINE_CHECK(e);
    ENGINE_ENTER(e);
    err = StateError5(e, 1, 2, 5, 4, 6);
    if (err == 0 && casrGetUserWordNbrRecsNeeded(e->casr, n) != 0)
        err = ConvErr(e, casrGetLastError(e->casr));
    ENGINE_LEAVE(e);
    return err;
}

int asrspi_OpenEngine(SAFEH rsrc, int mode, ASRSPI **out)
{
    RSRC *r;
    ASRSPI *e;
    CASR *c;
    int err;

    if (out == 0)
        return ASRSPI_E_NULLPTR;
    err = InitRsrcFunction(rsrc, &r);
    if (err < 0)
        return err;
    e = heap_Calloc(r->heap, 1, sizeof(ASRSPI));
    if (e == 0)
        return 0x8004002F;
    memset(e, 0, sizeof(ASRSPI));
    e->heap = r->heap;
    if (mode != 0 && mode != 2 && mode != 4) {
        heap_Free(e->heap, e);
        return 0x80040004;
    }
    c = casrOpen(r->heap, &g_APIRecogCallBacks, e);
    if (c == 0) {
        heap_Free(e->heap, e);
        return 0x80040086;
    }
    e->casr = c;
    e->magic = ASRSPI_MAGIC;
    e->open_mode = mode;
    e->state = 0;
    e->verdict = 0;
    FillContextV2VirtualTable(&e->ctxv2);
    e->gain = 0xBFFF;
    *out = e;
    return 0;
}

int asrspi_CloseEngine(ASRSPI *e)
{
    int err;

    ENGINE_CHECK(e);
    ENGINE_ENTER(e);
    err = StateError4(e, 0, 1, 6, 2);
    if (err) {
        ENGINE_LEAVE(e);
        return err;
    }
    if (e->state == 6 && e->rec)
        heap_Free(e->heap, e->rec);
    e->rec = 0;
    e->rec_info = 0;
    if (casrClose(e->casr) != 0) {
        ENGINE_LEAVE(e);
        return ConvErr(e, casrGetLastError(e->casr));
    }
    if ((e->ctx && casrCloseContext(e->ctx)) || (e->data && casrCloseData(e->data))) {
        ENGINE_LEAVE(e);
        return 0x80040086;
    }
    e->magic = 0;
    heap_Free(e->heap, e);
    return 0;
}

int asrspi_ActivateLng(ASRSPI *e, int *lng)
{
    int err;
    void *data;

    ENGINE_CHECK(e);
    ENGINE_ENTER(e);
    err = StateError4(e, 0, 1, 6, 2);
    if (err) {
        ENGINE_LEAVE(e);
        return err;
    }
    if (e->state == 6 && e->rec)
        heap_Free(e->heap, e->rec);
    e->rec = 0;
    data = 0;
    e->rec_info = 0;
    if (lng) {
        if (*lng != 0x1003 && *lng != 0x1004) {
            ENGINE_LEAVE(e);
            return 0x80040008;
        }
        data = casrImportData(e->heap, lng);
        if (data == 0) {
            ENGINE_LEAVE(e);
            return 0x80040086;
        }
    }
    if (casrActivateData(e->casr, data) != 0) {
        err = ConvErr(e, casrGetLastError(e->casr));
        if (err == 0x8004000A)
            err = 0x80040008;
        casrCloseData(data);
        ENGINE_LEAVE(e);
        return err;
    }
    if (e->ctx && casrCloseContext(e->ctx)) {
        ENGINE_LEAVE(e);
        return 0x80040086;
    }
    e->ctx = 0;
    if (e->data && casrCloseData(e->data)) {
        ENGINE_LEAVE(e);
        return 0x80040086;
    }
    e->data = data;
    if (lng)
        e->state = 1;
    else
        e->state = 0;
    ENGINE_LEAVE(e);
    return 0;
}

int asrspi_ActivateCtx(ASRSPI *e, int *ctxdata, int a2)
{
    int state_err, err;
    void *c;

    ENGINE_CHECK(e);
    ENGINE_ENTER(e);
    state_err = StateError3(e, 1, 6, 2);
    if (state_err) {
        ENGINE_LEAVE(e);
        return state_err;
    }
    if (e->state == 6 && e->rec)
        heap_Free(e->heap, e->rec);
    e->rec = 0;
    e->rec_info = 0;
    if (ctxdata) {
        if (ctxdata[1] == 24 && ctxdata[0] != 2) {
            ENGINE_LEAVE(e);
            return 0x80040026;
        }
        c = casrImportContext(e->heap, ctxdata);
        if (c == 0) {
            ENGINE_LEAVE(e);
            return 0x80040009;
        }
    } else
        c = 0;
    if (casrActivateContext(e->casr, c, a2) != 0) {
        err = ConvErr(e, casrGetLastError(e->casr));
        casrCloseContext(c);
        ENGINE_LEAVE(e);
        return err;
    }
    if (e->ctx)
        casrCloseContext(e->ctx);
    e->ctx = c;
    e->ctxdata = ctxdata;
    e->ctxv2.data = ctxdata;
    if (ctxdata)
        e->state = 2;
    else
        e->state = 1;
    ENGINE_LEAVE(e);
    return 0;
}

/* the API's parameter ids, as the casr engine's */
static __inline__ int TranslateParamId(int id, int *casr_id)
{
    switch (id) {
    case 0:  *casr_id = 1; break;
    case 1:  *casr_id = 2; break;
    case 2:  *casr_id = 3; break;
    case 3:  *casr_id = 5; break;
    case 17: *casr_id = 9; break;
    case 6:  *casr_id = 10; break;
    case 11: *casr_id = 17; break;
    case 7:  *casr_id = 13; break;
    case 8:  *casr_id = 12; break;
    case 9:  *casr_id = 11; break;
    case 4:  *casr_id = 15; break;
    case 18: *casr_id = 18; break;
    case 10: *casr_id = 14; break;
    case 12: *casr_id = 16; break;
    case 15: *casr_id = 7; break;
    case 16: *casr_id = 8; break;
    case 14: *casr_id = 6; break;
    case 13: *casr_id = 4; break;
    default: return 0x80040014;
    }
    return 0;
}

int asrspi_SetParamList(ASRSPI *e, PARAM *list, unsigned int n)
{
    int err, id;
    unsigned int i;

    if (list == 0)
        return ASRSPI_E_NULLPTR;
    ENGINE_CHECK(e);
    ENGINE_ENTER(e);
    err = StateError4(e, 0, 1, 6, 2);
    if (err) {
        ENGINE_LEAVE(e);
        return err;
    }
    for (i = 0; i < n; i++) {
        err = TranslateParamId(list[i].id, &id);
        if (err) {
            ENGINE_LEAVE(e);
            return err;
        }
        if (id == 14 && e->state == 0)
            return 0x80040013;
    }
    for (i = 0; i < n; i++) {
        TranslateParamId(list[i].id, &id);
        if (casrSetParam(e->casr, id, list[i].value) != 0) {
            ENGINE_LEAVE(e);
            return ConvErr(e, casrGetLastError(e->casr));
        }
    }
    ENGINE_LEAVE(e);
    return 0;
}

int asrspi_GetParamList(ASRSPI *e, PARAM *list, unsigned int n)
{
    int err, id;
    unsigned int i;

    if (list == 0)
        return ASRSPI_E_NULLPTR;
    ENGINE_CHECK(e);
    ENGINE_ENTER(e);
    for (i = 0; i < n; i++) {
        err = TranslateParamId(list[i].id, &id);
        if (err) {
            ENGINE_LEAVE(e);
            return err;
        }
        if (casrGetParam(e->casr, id, &list[i].value) != 0) {
            ENGINE_LEAVE(e);
            return ConvErr(e, casrGetLastError(e->casr));
        }
    }
    ENGINE_LEAVE(e);
    return 0;
}

int asrspi_GetActiveWords(ASRSPI *e, int *ids, unsigned int *count)
{
    int *words;
    unsigned int n;
    int err;

    if (count == 0)
        return ASRSPI_E_NULLPTR;
    ENGINE_CHECK(e);
    ENGINE_ENTER(e);
    err = StateError2(e, 2, 3);
    if (err) {
        ENGINE_LEAVE(e);
        return err;
    }
    if (casrGetActiveWords(e->casr, &words, &n) != 0) {
        ENGINE_LEAVE(e);
        return ConvErr(e, casrGetLastError(e->casr));
    }
    if (ids) {
        if (*count < n) {
            heap_Free(e->heap, words);
            ENGINE_LEAVE(e);
            return 0x80040085;
        }
        memcpy(ids, words, n * 4);
    }
    *count = n;
    heap_Free(e->heap, words);
    ENGINE_LEAVE(e);
    return 0;
}

/* the index of a word in the active list, -1 when it is not there */
static __inline__ unsigned int FindWord(int *words, unsigned int count, unsigned int id)
{
    unsigned int k;

    for (k = 0; k < count; k++)
        if (words[k] == id)
            return k;
    return (unsigned int)-1;
}

int asrspi_DeActivateWords(ASRSPI *e, unsigned int *ids, unsigned int n)
{
    int *words;
    unsigned int count;
    unsigned int nbr, i, j, m;
    int *p, *q;
    int err;

    if (ids == 0 && n != (unsigned int)-1)
        return ASRSPI_E_NULLPTR;
    ENGINE_CHECK(e);
    ENGINE_ENTER(e);
    err = StateError1(e, 2);
    if (err) {
        ENGINE_LEAVE(e);
        return err;
    }
    if (n == (unsigned int)-1) {
        if (casrSetActiveWords(e->casr, 0, 0) != 0) {
            ENGINE_LEAVE(e);
            return ConvErr(e, casrGetLastError(e->casr));
        }
        ENGINE_LEAVE(e);
        return 0;
    }
    if (casrGetActiveWords(e->casr, &words, &count) != 0) {
        ENGINE_LEAVE(e);
        return ConvErr(e, casrGetLastError(e->casr));
    }
    nbr = e->ctxv2.GetNbrWord(&e->ctxv2);
    for (i = 0; i < n; i++) {
        if (ids[i] >= nbr) {
            heap_Free(e->heap, words);
            ENGINE_LEAVE(e);
            return 0x80040018;
        }
        j = FindWord(words, count, ids[i]);
        if (j != (unsigned int)-1)
            words[j] = -1;
    }
    q = words;
    p = words;
    for (i = 0, m = 0; i < count; i++) {
        if (q != p)
            *p = *q;
        q++;
        if (*p != -1) {
            p++;
            m++;
        }
    }
    if (casrSetActiveWords(e->casr, words, m) != 0) {
        heap_Free(e->heap, words);
        ENGINE_LEAVE(e);
        return ConvErr(e, casrGetLastError(e->casr));
    }
    heap_Free(e->heap, words);
    ENGINE_LEAVE(e);
    return 0;
}

int asrspi_Start(ASRSPI *e, int interval, RECOGCB *cb, int user)
{
    int err;

    if (cb == 0)
        return ASRSPI_E_NULLPTR;
    if (interval == 0 && cb->signal && (cb->signal->energy_level || cb->signal->snr_level))
        return 0x80040016;
    ENGINE_CHECK(e);
    ENGINE_ENTER(e);
    err = StateError1(e, 2);
    if (err) {
        ENGINE_LEAVE(e);
        return err;
    }
    if (interval && cb->signal && e->open_mode != 0 && e->open_mode != 1
        && (cb->signal->energy_level || cb->signal->snr_level))
        return 0x80040017;
    e->level_interval = interval;
    e->got_result = 0;
    e->user = user;
    e->start_user = user;
    if (cb->signal)
        e->signal = *cb->signal;
    else
        memset(&e->signal, 0, sizeof e->signal);
    e->result_cb = cb->result;
    if (casrStart(e->casr) != 0) {
        ENGINE_LEAVE(e);
        return ConvErr(e, casrGetLastError(e->casr));
    }
    e->state = 3;
    ENGINE_LEAVE(e);
    return 0;
}

int asrspi_Stop(ASRSPI *e)
{
    int err, state;

    ENGINE_CHECK(e);
    ENGINE_ENTER(e);
    err = StateError1(e, 3);
    if (err) {
        ENGINE_LEAVE(e);
        return err;
    }
    if (casrStop(e->casr) != 0) {
        casrGetState(e->casr, &state);
        if (state == 2)
            e->state = state;
        ENGINE_LEAVE(e);
        return ConvErr(e, casrGetLastError(e->casr));
    }
    e->state = 2;
    ENGINE_LEAVE(e);
    return 0;
}

int asrspi_GainSet(ASRSPI *e, unsigned short gain)
{
    ENGINE_CHECK(e);
    ENGINE_ENTER(e);
    if (casrGainSet(e->casr, gain) != 0) {
        ENGINE_LEAVE(e);
        return ConvErr(e, casrGetLastError(e->casr));
    }
    ENGINE_LEAVE(e);
    return 0;
}

/* the host's sample format, as the casr engine's */
static __inline__ int ConvSampleFormat(unsigned int mode, int *fmt)
{
    switch (mode) {
    case 0: *fmt = 0; break;
    case 2: *fmt = 1; break;
    case 4: *fmt = 2; break;
    case 6: *fmt = 4; break;
    default: return 0x80040004;
    }
    return 0;
}

/* feed the samples to the engine; with a level callback (and 8- or 16-bit samples) in chunks of
 * the level interval, the signal level and SNR reported after each */
/* the loop is the vendor's goto: a for or while loop rotates its exit test or inverts the
 * last-chunk test (20 words), and returning 0 from the top takes v1 for the flag */
static int intAcquisition(ASRSPI *e, unsigned int mode, char *samples, unsigned int count)
{
    int fmt, err, last;
    unsigned int chunk, n, left;
    char *p;
    short level, snr;

    err = ConvSampleFormat(mode, &fmt);
    if (err)
        return err;
    if ((e->signal.energy_level || e->signal.snr_level) && mode < 2) {
        left = count;
        p = samples;
        last = 0;
        if (mode == 0)
            chunk = e->level_interval * 22;
        else
            chunk = e->level_interval * 16;
    again:
        if (e->got_result)
            goto done;
        n = chunk;
        if (chunk >= left) {
            n = left;
            last = 1;
        }
        if (casrAcquisition(e->casr, fmt, p, n) != 0
            || casrGetSignalLevel(e->casr, &level) != 0 || casrGetSNR(e->casr, &snr) != 0)
            return ConvErr(e, casrGetLastError(e->casr));
        if (e->signal.snr_level && (err = e->signal.snr_level(snr, e->user)) != 0)
            return err;
        if (e->signal.energy_level && (err = e->signal.energy_level(level, e->user)) != 0)
            return err;
        left -= n;
        p += chunk;
        if (!last)
            goto again;
    } else if (casrAcquisition(e->casr, fmt, samples, count) != 0)
        return ConvErr(e, casrGetLastError(e->casr));
done:
    return 0;
}

int asrspi_Acquisition(ASRSPI *e, char *samples, unsigned int count, int *done)
{
    int err;

    if (done == 0 || samples == 0)
        return ASRSPI_E_NULLPTR;
    *done = 0;
    ENGINE_CHECK(e);
    ENGINE_ENTER(e);
    err = StateError1(e, 3);
    if (err) {
        ENGINE_LEAVE(e);
        return err;
    }
    err = intAcquisition(e, e->open_mode, samples, count);
    *done = e->got_result;
    if (e->got_result)
        e->state = 2;
    if (err) {
        ENGINE_LEAVE(e);
        return err;
    }
    ENGINE_LEAVE(e);
    return 0;
}

int asrspi_UswAcquisition(ASRSPI *e, char *samples, unsigned int count, int *done)
{
    int err;

    if (done == 0 || samples == 0)
        return ASRSPI_E_NULLPTR;
    ENGINE_CHECK(e);
    ENGINE_ENTER(e);
    err = StateError1(e, 4);
    if (err) {
        ENGINE_LEAVE(e);
        return err;
    }
    err = intAcquisition(e, e->open_mode, samples, count);
    *done = e->got_result;
    if (e->got_result)
        e->state = 5;
    if (err) {
        ENGINE_LEAVE(e);
        return err;
    }
    ENGINE_LEAVE(e);
    return 0;
}

int asrspi_EnableAGC(ASRSPI *e, unsigned short gain)
{
    ENGINE_CHECK(e);
    ENGINE_ENTER(e);
    e->gain = gain;
    if (casrSetParam(e->casr, 11, 1) != 0) {
        ENGINE_LEAVE(e);
        return ConvErr(e, casrGetLastError(e->casr));
    }
    ENGINE_LEAVE(e);
    return 0;
}

int asrspi_DisableAGC(ASRSPI *e)
{
    ENGINE_CHECK(e);
    ENGINE_ENTER(e);
    if (casrSetParam(e->casr, 11, 0) != 0) {
        ENGINE_LEAVE(e);
        return ConvErr(e, casrGetLastError(e->casr));
    }
    ENGINE_LEAVE(e);
    return 0;
}

int asrspi_ExportSessionData(ASRSPI *e, int what, void *buf, unsigned int *size)
{
    void *data;
    unsigned int len;

    ENGINE_CHECK(e);
    ENGINE_ENTER(e);
    if (size == 0) {
        ENGINE_LEAVE(e);
        return ASRSPI_E_NULLPTR;
    }
    if (buf == 0)
        *size = 0;
    if (casrExportData(e->casr, 4, &data, &len) != 0) {
        ENGINE_LEAVE(e);
        return ConvErr(e, casrGetLastError(e->casr));
    }
    if (*size) {
        if (*size < len) {
            heap_Free(e->heap, data);
            ENGINE_LEAVE(e);
            return 0x80040085;
        }
        memcpy(buf, data, len);
    }
    *size = len;
    ENGINE_LEAVE(e);
    heap_Free(e->heap, data);
    return 0;
}

int asrspi_ActivateSessionData(ASRSPI *e, int *sess)
{
    struct {
        int type;
        int size;
        char pad[136];
    } def;
    int *p;
    void *data;
    int err;

    ENGINE_CHECK(e);
    ENGINE_ENTER(e);
    err = StateError3(e, 1, 6, 2);
    if (err) {
        ENGINE_LEAVE(e);
        return err;
    }
    if (sess == 0) {
        def.size = 8;
        def.type = 4;
        p = &def.type;
    } else {
        p = sess;
        if (*p != 4) {
            ENGINE_LEAVE(e);
            return 0x8004000A;
        }
    }
    data = casrImportData(e->heap, p);
    if (data == 0 || casrActivateData(e->casr, data) != 0 || casrCloseData(data) != 0) {
        ENGINE_LEAVE(e);
        return ConvErr(e, casrGetLastError(e->casr));
    }
    ENGINE_LEAVE(e);
    return 0;
}

int asrspi_GetLngGenders(int *lng, int *genders)
{
    return 0x80040081;
}

int asrspi_GetLngVersion(int *lng, char *version)
{
    LANG l;
    int err = 0;

    if (lng == 0 || version == 0)
        err = ASRSPI_E_NULLPTR;
    else {
        FillLanguageVirtualTable(&l);
        l.data = lng;
        if (!l.CheckDataType(&l))
            err = 0x80040008;
        else
            memcpy(version, l.GetVersionInfo(&l), 128);
    }
    return err;
}

int asrspi_GetCtxVersion(int *ctx, char *version)
{
    CONTEXTV2 c;
    int err = 0;

    if (ctx == 0 || version == 0)
        err = ASRSPI_E_NULLPTR;
    else {
        FillContextV2VirtualTable(&c);
        c.data = ctx;
        if (!c.CheckDataType(&c))
            err = 0x80040009;
        else
            memcpy(version, c.GetLanguageVersionInfo(&c), 128);
    }
    return err;
}

int asrspi_GetVersion(int *version)
{
    if (version == 0)
        return ASRSPI_E_NULLPTR;
    *version = casrGetSPIVersion();
    return 0;
}

int asrspi_GetUswVersion(int *version)
{
    *version = UswVersionCode;
    return 0;
}

int asrspi_GetState(ASRSPI *e, int *state)
{
    ENGINE_CHECK(e);
    if (state == 0)
        return ASRSPI_E_NULLPTR;
    *state = e->state;
    return 0;
}

int asrspi_UswGetRecordingInfo(ASRSPI *e, int *verdict)
{
    int err;

    ENGINE_CHECK(e);
    ENGINE_ENTER(e);
    err = StateError2(e, 4, 6);
    if (err) {
        ENGINE_LEAVE(e);
        return err;
    }
    *verdict = e->verdict;
    ENGINE_LEAVE(e);
    return 0;
}

void *_engineMalloc(ASRSPI *e, int size)
{
    return heap_Alloc(e->heap, size);
}

void *_engineCalloc(ASRSPI *e, int count, int size)
{
    return heap_Calloc(e->heap, count, size);
}

void _engineFree(ASRSPI *e, void *p)
{
    if (p)
        heap_Free(e->heap, p);
}

/* the casr engine's abnormal conditions, as the host's (the same numbers) */
static __inline__ void ConvAbnorm(int event, int *what)
{
    switch (event) {
    case 0: *what = 0; break;
    case 1: *what = 1; break;
    case 2: *what = 2; break;
    case 3: *what = 3; break;
    case 4: *what = 4; break;
    case 5: *what = 5; break;
    }
}

int ASRSPICBABNORM(int event, int value, CASRCB *cb)
{
    ASRSPI *e = cb->engine;
    int what;

    ConvAbnorm(event, &what);
    if (e->signal.abnorm)
        return e->signal.abnorm(what, value, e->user);
    return 0;
}

int ASRSPICBAGC(int gain, CASRCB *cb)
{
    ASRSPI *e = cb->engine;

    if (e->signal.agc)
        return e->signal.agc((unsigned short)gain, e->user);
    return 0;
}

int ASRSPICBASKCURRENTGAIN(CASRCB *cb)
{
    ASRSPI *e = cb->engine;

    return casrGainSet(e->casr, e->gain);
}

int ASRSPICBTS(int frame, CASRCB *cb)
{
    ASRSPI *e = cb->engine;
    int ret = 0;

    if (e->signal.ts_detected)
        ret = e->signal.ts_detected(frame, e->user);
    return ret;
}

int ASRSPICBSTARTDET(int frame, CASRCB *cb)
{
    ASRSPI *e = cb->engine;
    int ret = 0;

    if (e->signal.speech_detected)
        ret = e->signal.speech_detected(frame, e->user);
    return ret;
}

/* the casr engine's result status, as the host's result kind */
static __inline__ int ConvResultKind(unsigned int status, int *kind)
{
    switch (status) {
    case 1: *kind = 1; break;
    case 0: *kind = 0; break;
    case 2: *kind = 2; break;
    case 4: *kind = 3; break;
    default: return 0x8004002E;
    }
    return 0;
}

int ASRSPICBRESULT(CASRRESULT *res, CASRCB *cb)
{
    ASRSPI *e = cb->engine;
    int kind, err;

    e->got_result = 1;
    err = ConvResultKind(res->status, &kind);
    if (err)
        return err;
    e->result.kind = kind;
    e->result.f4 = res->f0;
    e->result.f8 = res->f4;
    if (e->result_cb)
        return e->result_cb(&e->result, e->user);
    return 0;
}
