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

typedef struct USWPARAMS {              /* a user word's training parameters (24 bytes) */
    int f0[4];
    int f16;                            /* +16 */
    int f20;                            /* +20 */
} USWPARAMS;

typedef int (*USWTRAINED)(void *rec, void *rec_info, int user);

typedef struct USWCB {                  /* asrspi_UswStartRecording's callbacks */
    USWTRAINED trained;                 /* +0: the word is trained: its recording */
    USWPARAMS *params;                  /* +4 */
} USWCB;

typedef struct ASRSPI {
    int magic;                          /* +0: ASRSPI_MAGIC */
    CASR *casr;                         /* +4 */
    int pad8[3];
    int user;                           /* +20: the host's data, passed to its callbacks */
    int pad24;
    int utt_accepted;                   /* +28: the training's events (ASRSPICBTRAIN) */
    int trained;                        /* +32 */
    int utt_done;                       /* +36 */
    void *rec;                          /* +40: the user word's recording */
    void *rec_info;                     /* +44 */
    int verdict;                        /* +48 */
    int state;                          /* +52 */
    int busy;                           /* +56 */
    int mode;                           /* +60 */
    int pad64[3];
    USWPARAMS params;                   /* +76 */
    int (*agc)(int gain, int user);     /* +100 */
    USWTRAINED trained_cb;              /* +104 */
    unsigned short gain;                /* +108 */
    int aborting;                       /* +112 */
    char pad116[92];
    void *heap;                         /* +208 */
    CRITSEC *cs;                        /* +212 */
} ASRSPI;                               /* 0xD8 */

typedef struct CASRCB {                 /* what the casr engine hands its callbacks */
    void *f0;
    ASRSPI *engine;                     /* +4: casrOpen's user data */
} CASRCB;

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
extern int casrStartUserWordTraining(CASR *c, void **rec, void **rec_info);
extern void heap_Free(void *heap, void *p);
extern void *memset(void *p, int c, unsigned int n);

static const long UswVersionCode = 0x16000001;
static const long g_ApiVersion = 0;
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
            e->utt_accepted = 1;
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

int asrspi_UswStartRecording(ASRSPI *e, int mode, USWCB *cb, int user, int setparams)
{
    int err;

    if (cb == 0)
        return ASRSPI_E_NULLPTR;
    if (mode == 0 && cb->params && (cb->params->f16 || cb->params->f20))
        return 0x80040016;
    ENGINE_CHECK(e);
    ENGINE_ENTER(e);
    err = StateError3(e, 1, 6, 2);
    if (err) {
        ENGINE_LEAVE(e);
        return err;
    }
    e->utt_accepted = 0;
    e->trained = 0;
    if (e->state == 6 && e->rec)
        heap_Free(e->heap, e->rec);
    e->mode = mode;
    e->rec = 0;
    e->rec_info = 0;
    if (cb->params)
        e->params = *cb->params;
    else
        memset(&e->params, 0, sizeof e->params);
    e->trained_cb = cb->trained;
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
    e->utt_accepted = 0;
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

int asrspi_GetState(ASRSPI *e, int *state)
{
    ENGINE_CHECK(e);
    if (state == 0)
        return ASRSPI_E_NULLPTR;
    *state = e->state;
    return 0;
}
