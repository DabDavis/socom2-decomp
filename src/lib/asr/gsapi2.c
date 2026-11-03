/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing -funroll-loops */
/* gsapi2: two host-API functions SOCOM II added after the gsapi object (OVL2 0x4C8CB8-0x4C8E68),
 * ScanSoft's recogniser, prebuilt with ee-gcc 2.96 at -O2 -G0 -fno-strict-aliasing -funroll-loops
 * (README "The GCC front"). Its own object, not gsapi.c's tail: gsapi.c ends at CheckEngineHandle,
 * which 2.96 outputs last there (a redefined extern inline's copy is deferred to the end of its
 * file), and these two follow it; func_004C8D10 inlines gsapi.h's CheckEngineHandle (the
 * single-result form, as a file without the real definition sees it). SOCOM 1 has neither, so the
 * file's name is ours and the functions keep their placeholders. */

#include "gsapi.h"

#define GSAPI_ERR_NOT_OPEN  0x80CC0001
#define GSAPI_ERR_NULL_ARG  0x80CC0083
#define GSAPI_ERR_NULL_CTX  0x80CC0089
#define GSAPI_ERR_BAD_PARAM 0x80CC000C

typedef struct PARAM {                  /* an asrspi parameter list's entry */
    int id;
    int value;
} PARAM;

typedef struct CTXPARAMS {              /* a context's parameters (context.c's) */
    int f0;
    int f4;
    unsigned int set;                   /* +8 */
    int value[18];                      /* +12 */
} CTXPARAMS;

typedef struct GSCONTEXT {              /* a context, as far as its parameters */
    void *data;
    int f4;
    void *f8;
    CTXPARAMS *params;                  /* +12 */
} GSCONTEXT;

extern SAFEH g_lhNullHandle;
extern const int TranslateParamTable[18];
extern unsigned short MAXRECOGNIZERS;
extern unsigned short DEFAULTENERGYINTERVAL;
extern void *data_0064F89C;             /* a 100-byte block's address (0x64F838, after gGSAPI) */
extern int data_006568F8;               /* its size, 100 */

extern int safeh_HandlesEqual(SAFEH a, SAFEH b);
extern int asrspi_GetParamList(int engine, PARAM *list, int n);
extern void *memset(void *p, int c, unsigned int n);

/* the API's statics reset to their link-time values (gGSAPI, the block after it, the two shorts) */
void func_004C8CB8(void)
{
    memset(&gGSAPI, 0, sizeof(gGSAPI));
    memset(data_0064F89C, 0, data_006568F8);
    MAXRECOGNIZERS = 4;
    DEFAULTENERGYINTERVAL = 500;
}

/* an engine's parameter as the engine holds it: gsapi's own threshold (9) from the active
 * context's parameters, any other from asrspi by its translated id. OPEN (11 words, the right
 * size): id and gGSAPI's address take each other's saved register (s1/s2); the test inverted, a
 * switch, the locals' order and the PARAM written whole do not swap them. */
int func_004C8D10(GSENGINE *e, unsigned int id, int *value)
{
    PARAM p;
    int err = 0;

    if (safeh_HandlesEqual(gGSAPI.handle, g_lhNullHandle))
        return GSAPI_ERR_NOT_OPEN;
    if (!CheckEngineHandle(e))
        return GSAPI_ERR_NULL_CTX;
    if (value == 0)
        return GSAPI_ERR_NULL_ARG;
    if (id >= 18)
        return GSAPI_ERR_BAD_PARAM;
    if (id == 9)
        *value = ((GSCONTEXT *)e->ctx)->params->value[9];
    else {
        p.id = TranslateParamTable[id];
        err = asrspi_GetParamList(e->f0, &p, 1);
        *value = p.value;
    }
    return err;
}
