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

extern SAFEH g_lhNullHandle;
extern short data_0064F7A0;
extern short data_0064F7A2;
extern void *data_0064F89C;
extern int data_006568F8;

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

extern int RestartEngine(GSENGINE *e);
extern int ProcessAudio(GSENGINE *e);

extern int ContextAPIDeActivate(GSENGINE *e);
extern int ContextActivate(GSENGINE *e, void *ctx);

extern int asrspi_ActivateWords(int engine, int words, int n);
extern int asrspi_DeActivateWords(int engine, int words, int n);
extern int CreateWordIdList(GSENGINE *e, void *ctx);
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
    data_0064F7A0 = 4;
    data_0064F7A2 = 500;
}
