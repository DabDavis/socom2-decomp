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
extern int gsapi_EngineEnableWords(void *eng, WORDSEL w);
extern int gsapi_EngineDisableWords(void *eng, WORDSEL w);
extern void *memset(void *p, int c, unsigned int n);

#define GSAPI_ERR_NULL_CTX  0x80CC0089

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

int gsapi_EngineEnableAllWords(void *eng)
{
    WORDSEL all;

    all.first = -1;
    all.count = 0;
    return gsapi_EngineEnableWords(eng, all);
}

int gsapi_EngineDisableAllWords(void *eng)
{
    WORDSEL all;

    all.first = -1;
    all.count = 0;
    return gsapi_EngineDisableWords(eng, all);
}

int gsapi_NotifySetCallback(GSCALLBACK cb)
{
    if (safeh_HandlesEqual(gGSAPI.handle, g_lhNullHandle))
        return GSAPI_ERR_NOT_OPEN;
    gGSAPI.notify = cb;
    return 0;
}

/* the 0 returned is the handle test's own (2.96 keeps it in v0). The engine accessors between
 * (NotifySetEnergyInterval, EngineSet/GetUserData, 0x4C8490-0x4C8758) are OPEN: SOCOM 1's
 * CheckEngineHandle inlined (an engine in gGSAPI's array, 108 bytes apart, live) comes within 11
 * words -- the target zeroes the result before the tests and swaps the divisor's register; six
 * shapes of the test (a result variable, one condition, early returns, a local base) do not. */
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
