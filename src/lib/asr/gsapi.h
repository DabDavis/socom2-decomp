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
    int f0;
    int flags;                          /* +4: bit 1 restarts the engine after a result */
    int f8;                             /* +8: the loaded context data */
    char pad12[36];
    void *ctx;                          /* +48: the active context */
    GSRESULT res;                       /* +52 */
    char pad64[8];
    void *loaded;                       /* +72: the context loaded into the engine */
    int user;                           /* +76: the host's user data */
    int energy_interval;                /* +80 */
    char pad84[8];
    int in_callback;                    /* +92 */
    int f96;
    int f100;                           /* +100 */
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
    int f20;
    GSCALLBACK result;                  /* +24 */
    GSCALLBACK notify;                  /* +28 */
    int (*read_audio)(void *src, void *buf, int *count);   /* +32 */
    char pad36[56];
    int confidence;                     /* +92: the default rejection threshold */
    char pad96[32];
    GSENGINE *engines;                  /* +128 */
    unsigned short f132;
    unsigned short nengines;            /* +134 */
    int ncontexts;                      /* +136 */
} GSAPI;

extern GSAPI gGSAPI;

#endif
