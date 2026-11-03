/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing -funroll-loops */
/* gsapi: the recogniser's host API (OVL2 0x4C61A8-0x4C8E68): ScanSoft's speech recogniser,
 * prebuilt with ee-gcc 2.96 at -O2 -G0 -fno-strict-aliasing -funroll-loops (README "The GCC
 * front"). SOCOM 1's demo carries the file (its object at 0x337BC8, InitDefaultParams first);
 * the names are its symbols, by position and by the calls each wrapper makes. The API is
 * guarded by the engine's safe handle (gGSAPI+8), compared by value with g_lhNullHandle. */

#include "gsapi.h"

#define GSAPI_ERR           0x80CC0000
#define GSAPI_ERR_NOT_OPEN  0x80CC0001

typedef struct WORDSEL {
    int first;                          /* -1: every word */
    int count;
} WORDSEL;

typedef struct PARAM {                  /* an asrspi parameter list's entry */
    int id;
    int value;
} PARAM;

/* the asrspi engine's callbacks (asrspi_cb.c's object defines both tables: SOCOM 1's
 * AsrSpiSignalCallBacks and AsrSpiRecogCallBacks) */
typedef struct SIGNALCB {
    void *abnorm;                       /* +0 */
    void *speech_detected;              /* +4 */
    void *ts_detected;                  /* +8 */
    void *agc;                          /* +12 */
    void *energy_level;                 /* +16 */
    void *snr_level;                    /* +20 */
} SIGNALCB;

typedef struct RECOGCB {
    void *result;
    SIGNALCB *signal;
} RECOGCB;

/* a loaded context, as far as the engine calls read it */
typedef struct GSCONTEXT {
    char pad[32];
    unsigned int nwords;                /* +32: the active words */
    unsigned int *words;                /* +36: their ids (gGSAPI.heap) */
} GSCONTEXT;

extern SAFEH g_lhNullHandle;
extern SIGNALCB AsrSpiSignalCallBacks;
extern RECOGCB AsrSpiRecogCallBacks;
extern unsigned short MAXRECOGNIZERS;
extern unsigned short DEFAULTENERGYINTERVAL;
extern void *data_0064F89C;
extern int data_006568F8;

/* SOCOM 1's gsapi object's .rodata: the version string (LOCAL) and the parameter-id map (GLOBAL) */
static const char szGSAPIVersion[] = "Game Speech API V2.02.01 for Sony PlayStation2";
const int TranslateParamTable[18] = {
    0, 1, 3, 17, 6, 7, 8, 9, 10, 255, 13, 14, 15, 16, 4, 11, 12, 18
};

extern int asrspi_GetState(int engine, int *state);
extern int asrspi_Acquisition(int engine, short *samples, int count, int *done);
extern int asrspi_GetActiveWords(int engine, unsigned int *ids, unsigned int *count);
extern int asrspi_Start(int engine, int interval, RECOGCB *cb, GSENGINE *user);
extern int asrspi_Stop(int engine);
extern int asrspi_CloseEngine(int engine);
extern int asrspi_SetParamList(int engine, PARAM *list, int n);
extern int asrspi_GetVersion(unsigned int *version);
extern int asrspi_cbEnergyLevel();
extern int asrspi_cbSNRLevel();
extern int SidGetSamples(void *sid, short **samples, int n);
extern int SidGetSamplesLeft(void *sid);
extern int SidAdvanceBuffer(void *sid, int n);
extern unsigned short SidIsRecording(void *sid);
extern int SidStop(void *sid);
extern int SidClose(void *heap, void *sid);
extern int ExtAudioGetSamples(void *src, short **samples, int n);
extern int ExtAudioGetSamplesLeft(void *src);
extern int ExtAudioStop(void *src);
extern int ExtAudioClose(void *heap, void *src);
extern int heap_Open(void **heap, int user);
extern int heap_Close(void *heap);
extern void *heap_Alloc(void *heap, unsigned int size);
extern void *heap_Calloc(void *heap, unsigned int n, unsigned int size);
extern void *heap_Realloc(void *heap, void *p, unsigned int size);
extern int rsrc_Open(void *heap, int a1, int a2, int a3, SAFEH *out);
extern int rsrc_Close(SAFEH h);
extern int StartListening(GSENGINE *e);
extern int StopListening(GSENGINE *e);
extern int gsapi_LanguageUnLoad(void);
extern int gsapi_EngineStop(GSENGINE *e);
extern int gsapi_EngineClose(GSENGINE *e);
extern int printf(const char *fmt, ...);
extern int sprintf(char *buf, const char *fmt, ...);
extern char *strncpy(char *dst, const char *src, unsigned int n);
extern unsigned int strlen(const char *s);

extern int safeh_HandlesEqual(SAFEH a, SAFEH b);
extern int ContextSetGcdData(void *ctx, void *data);
extern int ContextSetWrdData(void *ctx, void *data);
extern int gsapi_ContextEnableWords(void *ctx, WORDSEL w);
extern int gsapi_ContextDisableWords(void *ctx, WORDSEL w);
extern void *memset(void *p, int c, unsigned int n);

#define GSAPI_ERR_NULL_CTX  0x80CC0089
#define GSAPI_ERR_NOT_LOADED 0x80CC000E
#define GSAPI_ERR_NO_CONTEXT 0x80CC000D
#define GSAPI_ERR_NULL_ARG   0x80CC0083
#define GSAPI_ERR_BAD_MODE   0x80CC0007

#define GSAPI_ERR_NO_MEMORY  0x80CC0086
#define GSAPI_ERR_SAMPLES    0x80CC0006

extern int RestartEngine(GSENGINE *e);

extern int ContextAPIDeActivate(GSENGINE *e);
extern int ContextActivate(GSENGINE *e, void *ctx);

extern int asrspi_ActivateWords(int engine, int words, int n);
extern int asrspi_DeActivateWords(int engine, int words, int n);
extern int ContextDeActivate(GSENGINE *e);
extern int ContextUnLoad(void *ctx);
extern void heap_Free(void *heap, void *p);

/* SOCOM 1's CheckEngineHandle, inlined into every engine call in SOCOM II (its out-of-line copy is
 * gone): an engine is a live slot of gGSAPI's array. The base is read first and the result starts
 * at 0 -- the order that schedules as the target's (an early-return or one-expression form keeps
 * a 0 per failing test, or loads the base after the null test). */
static __inline__ int CheckEngineHandle(GSENGINE *e)
{
    GSENGINE *base = gGSAPI.engines;
    int ok = 0;

    if (e != 0 && e >= base && ((char *)e - (char *)base) % sizeof(GSENGINE) == 0
        && e <= base + gGSAPI.nengines)
        ok = e->f0 != 0;
    return ok;
}

/* the engines' default parameters, set when a language is loaded */
static void InitDefaultParams(void)
{
    gGSAPI.f52 = 0x1FFFF;
    gGSAPI.params[0] = 1;
    gGSAPI.params[1] = 1;
    gGSAPI.params[2] = 0;
    gGSAPI.params[3] = 50;
    gGSAPI.params[4] = 60;
    gGSAPI.params[5] = 1;
    gGSAPI.params[6] = 1;
    gGSAPI.params[7] = 0;
    gGSAPI.params[8] = 0x7FFFFFFF;
    gGSAPI.params[9] = 5000;
    gGSAPI.params[10] = 200;
    gGSAPI.params[11] = 300;
    gGSAPI.params[12] = 40;
    gGSAPI.params[13] = 40;
    gGSAPI.params[14] = 1;
    gGSAPI.params[15] = -7200;
    gGSAPI.params[16] = 0;
    gGSAPI.params[17] = 0;
}

/* one server tick of an engine: its audio fed to the recogniser while a buffer's worth waits */
static int ProcessAudio(GSENGINE *e)
{
    short *samples = 0;
    int done = 0;
    int n;
    int left;

    gGSAPI.status = asrspi_GetState(e->f0, &e->state);
    if (gGSAPI.status != 0)
        return gGSAPI.status;
    switch (e->listening) {
    case 0:
        break;
    case 1:
        do {
            if (gGSAPI.read_audio != 0)
                n = ExtAudioGetSamples(e, &samples, 110);
            else
                n = SidGetSamples(e->audio, &samples, 110);
            if (n > 0) {
                if ((e->flags & 4) && gGSAPI.notify != 0)
                    gGSAPI.notify(e, 1, samples, n * 2);
                gGSAPI.status = asrspi_Acquisition(e->f0, samples, n * 2, &done);
                if (gGSAPI.status < 0)
                    return gGSAPI.status;
                if (gGSAPI.read_audio == 0)
                    SidAdvanceBuffer(e->audio, n);
                if (done != 0)
                    StopListening(e);
            } else if (n < 0) {
                printf("Error retrieving samples\n");
                return GSAPI_ERR_SAMPLES;
            }
            if (gGSAPI.read_audio != 0)
                left = ExtAudioGetSamplesLeft(e);
            else
                left = SidGetSamplesLeft(e->audio);
        } while (left >= 110);
        if (e->f96 == 1) {
            RestartEngine(e);
            e->f96 = 0;
        }
        break;
    }
    return 0;
}

/* the context's copy of the engine's active word ids */
int CreateWordIdList(GSENGINE *e, GSCONTEXT *ctx)
{
    unsigned int n;
    int err;

    err = asrspi_GetActiveWords(e->f0, 0, &n);
    if (err < 0)
        return err;
    if (ctx->words != 0)
        ctx->words = heap_Realloc(gGSAPI.heap, ctx->words, n * 4);
    else
        ctx->words = heap_Alloc(gGSAPI.heap, n * 4);
    if (ctx->words == 0)
        return GSAPI_ERR_NO_MEMORY;
    err = asrspi_GetActiveWords(e->f0, ctx->words, &n);
    if (err < 0) {
        heap_Free(gGSAPI.heap, ctx->words);
        ctx->words = 0;
    }
    ctx->nwords = n;
    return err;
}

int gsapi_Close(void)
{
    GSENGINE *e;
    int i;

    if (safeh_HandlesEqual(gGSAPI.handle, g_lhNullHandle))
        return GSAPI_ERR_NOT_OPEN;
    for (i = 0; i < gGSAPI.nengines; i++) {
        e = &gGSAPI.engines[i];
        if (e->f0 != 0)
            gGSAPI.status = gsapi_EngineClose(e);
    }
    gsapi_LanguageUnLoad();
    heap_Free(gGSAPI.heap, gGSAPI.engines);
    gGSAPI.status = rsrc_Close(gGSAPI.handle);
    if (gGSAPI.status != 0)
        return gGSAPI.status;
    gGSAPI.handle = g_lhNullHandle;
    heap_Close(gGSAPI.heap);
    gGSAPI.heap = 0;
    return 0;
}

/* a context made the engine's active one: the engine's own loaded data is unloaded first */
int gsapi_ContextActivate(GSENGINE *e, void *ctx)
{
    if (safeh_HandlesEqual(gGSAPI.handle, g_lhNullHandle))
        return GSAPI_ERR_NOT_OPEN;
    if (!CheckEngineHandle(e))
        return GSAPI_ERR_NULL_CTX;
    if (ctx == 0)
        return GSAPI_ERR_NULL_CTX;
    gGSAPI.status = ContextAPIDeActivate(e);
    if (gGSAPI.status < 0)
        return gGSAPI.status;
    if (e->f8 != 0) {
        gGSAPI.status = ContextUnLoad(&e->f8);
        if (gGSAPI.status < 0)
            return gGSAPI.status;
        memset(&e->f8, 0, 40);
    }
    return gGSAPI.status = ContextActivate(e, ctx);
}

int gsapi_ContextDeActivate(GSENGINE *e)
{
    if (safeh_HandlesEqual(gGSAPI.handle, g_lhNullHandle))
        return GSAPI_ERR_NOT_OPEN;
    if (!CheckEngineHandle(e))
        return GSAPI_ERR_NULL_CTX;
    gGSAPI.status = ContextDeActivate(e);
    if (gGSAPI.status < 0)
        return gGSAPI.status;
    if (e->f8 != 0) {
        gGSAPI.status = ContextUnLoad(&e->f8);
        if (gGSAPI.status < 0)
            return gGSAPI.status;
        memset(&e->f8, 0, 40);
    }
    return gGSAPI.status;
}

/* a context's words, in every engine the context is loaded into; the last such engine's word
 * list is rebuilt */
int gsapi_ContextDisableWords(void *ctx, WORDSEL w)
{
    GSENGINE *e = 0;
    GSENGINE *p;
    int i;

    if (safeh_HandlesEqual(gGSAPI.handle, g_lhNullHandle))
        return GSAPI_ERR_NOT_OPEN;
    if (ctx == 0)
        return GSAPI_ERR_NULL_CTX;
    for (i = 0; i < gGSAPI.nengines; i++) {
        p = &gGSAPI.engines[i];
        if (p->loaded == ctx) {
            e = p;
            gGSAPI.status = asrspi_DeActivateWords(e->f0, w.count, w.first);
            if (gGSAPI.status < 0)
                return gGSAPI.status;
        }
    }
    if (e == 0)
        return GSAPI_ERR_NOT_LOADED;
    return gGSAPI.status = CreateWordIdList(e, ctx);
}

int gsapi_ContextEnableWords(void *ctx, WORDSEL w)
{
    GSENGINE *e = 0;
    GSENGINE *p;
    int i;

    if (safeh_HandlesEqual(gGSAPI.handle, g_lhNullHandle))
        return GSAPI_ERR_NOT_OPEN;
    if (ctx == 0)
        return GSAPI_ERR_NULL_CTX;
    for (i = 0; i < gGSAPI.nengines; i++) {
        p = &gGSAPI.engines[i];
        if (p->loaded == ctx) {
            e = p;
            gGSAPI.status = asrspi_ActivateWords(e->f0, w.count, w.first);
            if (gGSAPI.status < 0)
                return gGSAPI.status;
        }
    }
    if (e == 0)
        return GSAPI_ERR_NOT_LOADED;
    return gGSAPI.status = CreateWordIdList(e, ctx);
}

/* a context is taken out of every engine it is loaded into, unloaded and freed */
int gsapi_ContextUnLoad(void *ctx)
{
    GSENGINE *e;
    int i;

    if (safeh_HandlesEqual(gGSAPI.handle, g_lhNullHandle))
        return GSAPI_ERR_NOT_OPEN;
    if (ctx == 0)
        return GSAPI_ERR_NULL_CTX;
    for (i = 0; i < gGSAPI.nengines; i++) {
        e = &gGSAPI.engines[i];
        if (e->loaded == ctx) {
            gGSAPI.status = ContextDeActivate(e);
            if (gGSAPI.status < 0)
                return gGSAPI.status;
        }
    }
    gGSAPI.status = ContextUnLoad(ctx);
    heap_Free(gGSAPI.heap, ctx);
    gGSAPI.ncontexts--;
    return gGSAPI.status;
}

int gsapi_EngineClose(GSENGINE *e)
{
    if (safeh_HandlesEqual(gGSAPI.handle, g_lhNullHandle))
        return GSAPI_ERR_NOT_OPEN;
    if (!CheckEngineHandle(e))
        return GSAPI_ERR_NULL_CTX;
    e->closing = 1;
    gsapi_EngineStop(e);
    if (gGSAPI.read_audio != 0)
        ExtAudioClose(gGSAPI.heap, e->audio);
    else {
        if (SidIsRecording(e->audio))
            SidStop(e->audio);
        SidClose(gGSAPI.heap, e->audio);
    }
    ContextDeActivate(e);
    if (e->f8 != 0) {
        ContextUnLoad(&e->f8);
        memset(&e->f8, 0, 40);
    }
    gGSAPI.status = asrspi_CloseEngine(e->f0);
    if (gGSAPI.status == 0) {
        e->f0 = 0;
        gGSAPI.nopen--;
    }
    return gGSAPI.status;
}

int gsapi_EngineStart(GSENGINE *e)
{
    int err;

    if (safeh_HandlesEqual(gGSAPI.handle, g_lhNullHandle))
        return GSAPI_ERR_NOT_OPEN;
    if (!CheckEngineHandle(e))
        return GSAPI_ERR_NULL_CTX;
    e->f100 = 0;
    if (e->in_callback != 0) {
        e->f96 = 1;
        err = 0;
    } else {
        if (e->energy_interval != 0) {
            AsrSpiSignalCallBacks.energy_level = asrspi_cbEnergyLevel;
            AsrSpiSignalCallBacks.snr_level = asrspi_cbSNRLevel;
        } else {
            AsrSpiSignalCallBacks.energy_level = 0;
            AsrSpiSignalCallBacks.snr_level = 0;
        }
        gGSAPI.status = asrspi_Start(e->f0, e->energy_interval, &AsrSpiRecogCallBacks, e);
        if (gGSAPI.status < 0) {
            err = gGSAPI.status;
        } else {
            gGSAPI.status = StartListening(e);
            err = gGSAPI.status;
        }
    }
    if (err >= 0)
        e->listening = 1;
    return err;
}

/* OPEN (2 words longer): GCSE's PRE takes the e->audio load the stop's test makes as available
 * and keeps it in a register for the external-audio branch (`move v1,a0`, the reload only on the
 * path through the notify call); the target reloads it at the join. Byte-exact as written under
 * -fno-expensive-optimizations, which loses gsapi_EngineStart and gsapi_GetVersion, so not the
 * unit's flag; a local copy, the stop flag stored after the load, an inlined notify helper and
 * five arrangements of the tests (build/scratch/gcc/r10c/gsapi.c, -DW=0..8) do not give it. */
int gsapi_EngineStop(GSENGINE *e)
{
    int err;

    if (safeh_HandlesEqual(gGSAPI.handle, g_lhNullHandle))
        return GSAPI_ERR_NOT_OPEN;
    if (!CheckEngineHandle(e))
        return GSAPI_ERR_NULL_CTX;
    e->f100 = 1;
    if (e->audio != 0) {
        if (gGSAPI.read_audio != 0) {
            if (gGSAPI.notify != 0)
                gGSAPI.notify(e, 3, 0, e->user);
            err = ExtAudioStop(e->audio);
        } else if (SidIsRecording(e->audio)) {
            if (gGSAPI.notify != 0)
                gGSAPI.notify(e, 3, 0, e->user);
            err = SidStop(e->audio);
        } else
            err = 0;
    } else
        err = 0;
    gGSAPI.status = err;
    if (gGSAPI.status != 0)
        return gGSAPI.status;
    gGSAPI.status = asrspi_GetState(e->f0, &e->state);
    if (gGSAPI.status != 0)
        return gGSAPI.status;
    if (e->state == 3)
        gGSAPI.status = asrspi_Stop(e->f0);
    if (gGSAPI.status >= 0)
        e->listening = 0;
    return gGSAPI.status;
}

int gsapi_Init(GSCALLBACK result, int user)
{
    int err;

    if (result == 0)
        return GSAPI_ERR_NULL_ARG;
    if (!safeh_HandlesEqual(gGSAPI.handle, g_lhNullHandle))
        return GSAPI_ERR;
    gGSAPI.result = result;
    gGSAPI.user = user;
    gGSAPI.nopen = 0;
    gGSAPI.notify = 0;
    gGSAPI.read_audio = 0;
    gGSAPI.nengines = MAXRECOGNIZERS;
    gGSAPI.ncontexts = 0;
    err = heap_Open(&gGSAPI.heap, gGSAPI.heap_user);
    if (err < 0)
        return err;
    gGSAPI.engines = heap_Calloc(gGSAPI.heap, MAXRECOGNIZERS, sizeof(GSENGINE));
    if (gGSAPI.engines == 0)
        return GSAPI_ERR_NO_MEMORY;
    err = rsrc_Open(gGSAPI.heap, 0, 0, 0, &gGSAPI.handle);
    if (err < 0)
        return err;
    return 0;
}

/* the API's version, then the engine's: "  [ASRSPI major.minor.build]" when 30 bytes are left */
int gsapi_GetVersion(char *buf, int size)
{
    unsigned int v;
    int n;

    if (safeh_HandlesEqual(gGSAPI.handle, g_lhNullHandle))
        return GSAPI_ERR_NOT_OPEN;
    if (buf == 0)
        return GSAPI_ERR_NULL_ARG;
    strncpy(buf, szGSAPIVersion, size);
    buf[size - 1] = 0;
    n = strlen(szGSAPIVersion);
    size -= n;
    buf += n;
    if (size < 30)
        return 0;
    asrspi_GetVersion(&v);
    sprintf(buf, "  [ASRSPI %d.%02d.%04d]", v >> 24, (v >> 16) & 0xFF, v & 0xFFFF);
    return 0;
}

/* a context's grammar and word data handed on (2.96 makes the call a sibling call) */
int gsapi_ContextSetGcdData(void *ctx, void *data)
{
    if (safeh_HandlesEqual(gGSAPI.handle, g_lhNullHandle))
        return GSAPI_ERR_NOT_OPEN;
    if (ctx == 0)
        return GSAPI_ERR_NULL_CTX;
    return ContextSetGcdData(ctx, data);
}

int gsapi_ContextSetWrdData(void *ctx, void *data)
{
    if (safeh_HandlesEqual(gGSAPI.handle, g_lhNullHandle))
        return GSAPI_ERR_NOT_OPEN;
    if (ctx == 0)
        return GSAPI_ERR_NULL_CTX;
    return ContextSetWrdData(ctx, data);
}

int gsapi_ContextEnableAllWords(void *ctx)
{
    WORDSEL all;

    all.first = -1;
    all.count = 0;
    return gsapi_ContextEnableWords(ctx, all);
}

int gsapi_ContextDisableAllWords(void *ctx)
{
    WORDSEL all;

    all.first = -1;
    all.count = 0;
    return gsapi_ContextDisableWords(ctx, all);
}

int gsapi_EngineEnableWords(GSENGINE *e, WORDSEL w)
{
    if (safeh_HandlesEqual(gGSAPI.handle, g_lhNullHandle))
        return GSAPI_ERR_NOT_OPEN;
    if (!CheckEngineHandle(e))
        return GSAPI_ERR_NULL_CTX;
    if (e->f8 == 0)
        return GSAPI_ERR_NO_CONTEXT;
    return gGSAPI.status = asrspi_ActivateWords(e->f0, w.count, w.first);
}

int gsapi_EngineDisableWords(GSENGINE *e, WORDSEL w)
{
    if (safeh_HandlesEqual(gGSAPI.handle, g_lhNullHandle))
        return GSAPI_ERR_NOT_OPEN;
    if (!CheckEngineHandle(e))
        return GSAPI_ERR_NULL_CTX;
    if (e->f8 == 0)
        return GSAPI_ERR_NO_CONTEXT;
    return gGSAPI.status = asrspi_DeActivateWords(e->f0, w.count, w.first);
}

int gsapi_EngineEnableAllWords(GSENGINE *eng)
{
    WORDSEL all;

    all.first = -1;
    all.count = 0;
    return gsapi_EngineEnableWords(eng, all);
}

int gsapi_EngineDisableAllWords(GSENGINE *eng)
{
    WORDSEL all;

    all.first = -1;
    all.count = 0;
    return gsapi_EngineDisableWords(eng, all);
}

int gsapi_EngineSetMode(GSENGINE *e, int mode)
{
    if (safeh_HandlesEqual(gGSAPI.handle, g_lhNullHandle))
        return GSAPI_ERR_NOT_OPEN;
    if (!CheckEngineHandle(e))
        return GSAPI_ERR_NULL_CTX;
    if (mode == 0)
        return GSAPI_ERR_BAD_MODE;
    if ((mode & 3) == 3)
        return GSAPI_ERR_BAD_MODE;
    e->flags = mode;
    return 0;
}

/* the server's tick: each engine restarted when it asked to be, then fed its audio */
void gsapi_ExecServer(void)
{
    GSENGINE *e;
    int i;

    if (safeh_HandlesEqual(gGSAPI.handle, g_lhNullHandle))
        return;
    for (i = 0; i < gGSAPI.nengines; i++) {
        e = &gGSAPI.engines[i];
        if (e->f96 == 1) {
            RestartEngine(e);
            e->f96 = 0;
        }
        gGSAPI.status = ProcessAudio(e);
    }
}

int gsapi_NotifySetCallback(GSCALLBACK cb)
{
    if (safeh_HandlesEqual(gGSAPI.handle, g_lhNullHandle))
        return GSAPI_ERR_NOT_OPEN;
    gGSAPI.notify = cb;
    return 0;
}

/* OPEN, and gsapi_EngineSetUserData (35 words, one short): the same inlined check matches in
 * five other calls, but here the target zeroes the result a second time after the base test
 * and holds 108 in v0 and the difference in v1 (the round-3b diagnosis); the guard, an if
 * around the store and a result variable (build/scratch/gcc/spec_ei.py) do not give it. */
int gsapi_NotifySetEnergyInterval(GSENGINE *e, int interval)
{
    if (safeh_HandlesEqual(gGSAPI.handle, g_lhNullHandle))
        return GSAPI_ERR_NOT_OPEN;
    if (!CheckEngineHandle(e))
        return GSAPI_ERR_NULL_CTX;
    e->energy_interval = interval;
    return 0;
}

int gsapi_EngineSetUserData(GSENGINE *e, int data)
{
    if (safeh_HandlesEqual(gGSAPI.handle, g_lhNullHandle))
        return GSAPI_ERR_NOT_OPEN;
    if (!CheckEngineHandle(e))
        return GSAPI_ERR_NULL_CTX;
    e->user = data;
    return 0;
}

int gsapi_EngineGetUserData(GSENGINE *e, int *data)
{
    if (safeh_HandlesEqual(gGSAPI.handle, g_lhNullHandle))
        return GSAPI_ERR_NOT_OPEN;
    if (!CheckEngineHandle(e))
        return GSAPI_ERR_NULL_CTX;
    if (data == 0)
        return GSAPI_ERR_NULL_ARG;
    *data = e->user;
    return 0;
}

/* the 0 returned is the handle test's own (2.96 keeps it in v0) */
int gsapi_GetHeapHandle(void **heap)
{
    if (safeh_HandlesEqual(gGSAPI.handle, g_lhNullHandle))
        return GSAPI_ERR_NOT_OPEN;
    *heap = gGSAPI.heap;
    return 0;
}

int gsapi_SetHeapUserData(int data)
{
    if (!safeh_HandlesEqual(gGSAPI.handle, g_lhNullHandle))
        return GSAPI_ERR;
    gGSAPI.heap_user = data;
    return 0;
}

int gsapi_UseExternalAudio(int (*read)(void *src, void *buf, int *count))
{
    if (safeh_HandlesEqual(gGSAPI.handle, g_lhNullHandle))
        return GSAPI_ERR_NOT_OPEN;
    gGSAPI.read_audio = read;
    return 0;
}

void func_004C8CB8(void)
{
    memset(&gGSAPI, 0, sizeof(gGSAPI));
    memset(data_0064F89C, 0, data_006568F8);
    MAXRECOGNIZERS = 4;
    DEFAULTENERGYINTERVAL = 500;
}
