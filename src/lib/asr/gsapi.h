/* gsapi.h: the ScanSoft recogniser's host API (gsapi_*), as far as the hand-written units use
 * it: the global state gGSAPI (data.txt, 0x64F7A8) and the engine handle the callbacks receive.
 * Fields are named by their use; offsets from the code. */
#ifndef GSAPI_H
#define GSAPI_H

typedef struct GSENGINE GSENGINE;

typedef void (*GSCALLBACK)(GSENGINE *e, int event, void *data, int user);

typedef struct GSRESULT {
    int rejected;                       /* +0 */
    unsigned short nwords;              /* +4 */
    unsigned short score;               /* +6 */
    int *ids;                           /* +8 */
} GSRESULT;

struct GSENGINE {
    int f0;                             /* +0: the asrspi engine; 0 while the slot is free */
    int flags;                          /* +4: bit 1 restarts the engine after a result, bit 2
                                         * hands the host every buffer of samples */
    int f8;                             /* +8: the loaded context data (40 bytes) */
    char pad12[36];
    void *ctx;                          /* +48: the active context */
    GSRESULT res;                       /* +52 */
    int listening;                      /* +64: 1 from a started engine's start to its stop */
    int state;                          /* +68: asrspi_GetState's */
    void *loaded;                       /* +72: the context loaded into the engine */
    int user;                           /* +76: the host's user data */
    int energy_interval;                /* +80 */
    void *audio;                        /* +84: the microphone (sid) or the external source */
    int closing;                        /* +88 */
    int in_callback;                    /* +92 */
    int f96;                            /* +96: a restart asked for inside a callback */
    int f100;                           /* +100: 1 once stopped, 0 once started */
    int f104;
};                                      /* 108 bytes, gGSAPI.engines' element */

typedef struct SAFEH {                  /* a safe handle, passed by value */
    int id;
    int check;
} SAFEH;

typedef struct GSAPI {
    void *heap;                         /* +0 */
    int heap_user;                      /* +4 */
    SAFEH handle;                       /* +8: g_lhNullHandle until the engine is open */
    int status;                         /* +16: the last call's result */
    int user;                           /* +20: gsapi_Init's */
    GSCALLBACK result;                  /* +24 */
    GSCALLBACK notify;                  /* +28 */
    int (*read_audio)(void *src, void *buf, int *count);   /* +32 */
    void *language;                     /* +36: the loaded language's data */
    void *langbuf;                      /* +40: its buffer, when a 16-byte header precedes it */
    int f44;
    int f48;
    int f52;                            /* +52: 0x1FFFF by default */
    int params[18];                     /* +56: the engines' defaults, by gsapi parameter id
                                         * (TranslateParamTable maps them to asrspi's);
                                         * params[9], gsapi's own, the rejection threshold */
    GSENGINE *engines;                  /* +128 */
    unsigned short nopen;               /* +132: engines open */
    unsigned short nengines;            /* +134: MAXRECOGNIZERS */
    int ncontexts;                      /* +136 */
} GSAPI;                                /* 0x8C */

extern GSAPI gGSAPI;

/* SOCOM 1's CheckEngineHandle, a GNU `extern __inline__` body here: inlined into every engine call
 * of gsapi.c and its neighbour gsapi2.c, while gsapi.c keeps SOCOM 1's real function (uncalled)
 * -- 2.96 outputs a redefined inline at the end of its file, which is where it stands (0x4C8C38,
 * after StopListening). An engine is a live slot of gGSAPI's array. The base is read first and
 * the result starts at 0 -- the order that schedules as the target's (an early-return or
 * one-expression form keeps a 0 per failing test, or loads the base after the null test). */
extern __inline__ int CheckEngineHandle(GSENGINE *e)
{
    GSENGINE *base = gGSAPI.engines;
    int ok = 0;

    if (e != 0 && e >= base && ((char *)e - (char *)base) % sizeof(GSENGINE) == 0
        && e <= base + gGSAPI.nengines)
        ok = e->f0 != 0;
    return ok;
}

#endif
