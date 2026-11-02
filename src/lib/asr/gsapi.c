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
extern int gsapi_ContextEnableWords(void *ctx, WORDSEL w);
extern int gsapi_ContextDisableWords(void *ctx, WORDSEL w);
extern int gsapi_EngineEnableWords(void *eng, WORDSEL w);
extern int gsapi_EngineDisableWords(void *eng, WORDSEL w);
extern void *memset(void *p, int c, unsigned int n);

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
