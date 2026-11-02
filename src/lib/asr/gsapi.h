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
    char pad8[40];
    void *ctx;                          /* +48: the active context */
    GSRESULT res;                       /* +52 */
    char pad64[12];
    int user;                           /* +76: the host's user data */
    char pad80[12];
    int in_callback;                    /* +92 */
    int f96;
    int f100;                           /* +100 */
};

typedef struct SAFEH {                  /* a safe handle, passed by value */
    int id;
    int check;
} SAFEH;

typedef struct GSAPI {
    void *heap;                         /* +0 */
    int heap_user;                      /* +4 */
    SAFEH handle;                       /* +8: g_lhNullHandle until the engine is open */
    char pad16[8];
    GSCALLBACK result;                  /* +24 */
    GSCALLBACK notify;                  /* +28 */
    int (*read_audio)(void *src, void *buf, int *count);   /* +32 */
    char pad36[56];
    int confidence;                     /* +92: the default rejection threshold */
    char pad96[44];
} GSAPI;

extern GSAPI gGSAPI;

#endif
