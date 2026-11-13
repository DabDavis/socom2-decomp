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

typedef struct WORDLIST {               /* a context's word strings (WrdCreateWordList's) */
    unsigned int n;
    char **words;
} WORDLIST;

typedef struct CTXPARAMS {              /* a context's parameters (context.c's) */
    int f0;
    int f4;
    unsigned int set;                   /* +8: a bit per parameter */
    int value[18];                      /* +12 */
} CTXPARAMS;

/* a context: 40 bytes, gsapi_ContextLoadBuffer's or an engine's own at +8 */
typedef struct GSCONTEXT {
    void *data;                         /* +0 */
    int f4;
    void *f8;                           /* +8 */
    CTXPARAMS *params;                  /* +12 */
    void *actions;                      /* +16 */
    int f20;
    int f24;
    WORDLIST *wordlist;                 /* +28 */
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
extern int ContextSetParam(void *ctx, int id, int value);
extern int ContextGetParam(void *ctx, int id, int *value);
extern int ContextActivateParams(void *dst, void *ctx);
extern int ContextSetCtxData(void *ctx, void *data);
extern int ExtAudioInit(void *heap, void **audio);
extern int ExtAudioStart(void *src);
extern int SidInit(void *heap, int mic, void **sid);
extern int SidStart(void *sid);
extern int asrspi_OpenEngine(SAFEH rsrc, int mode, GSENGINE *e);
extern int asrspi_ActivateLng(int engine, void *language);
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
#define GSAPI_ERR_NO_LANGUAGE 0x80CC0002
#define GSAPI_ERR_NO_ENGINE  0x80CC0005
#define GSAPI_ERR_NO_WORDS   0x80CC000A
#define GSAPI_ERR_BAD_WORD   0x80CC000B
#define GSAPI_ERR_BAD_PARAM  0x80CC000C
#define GSAPI_ERR_BAD_VALUE  0x80CC000F
#define GSAPI_ERR_NO_WORDLIST 0x80CC0080

extern int RestartEngine(GSENGINE *e);

extern int ContextAPIDeActivate(GSENGINE *e);
extern int ContextActivate(GSENGINE *e, void *ctx);

extern int asrspi_ActivateWords(int engine, int words, int n);
extern int asrspi_DeActivateWords(int engine, int words, int n);
extern int ContextDeActivate(GSENGINE *e);
extern int ContextUnLoad(void *ctx);
extern void heap_Free(void *heap, void *p);

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

/* an engine opened in the first free slot, on the microphone `mic` or the host's audio */
int gsapi_EngineOpen(int mic, GSENGINE **out)
{
    GSENGINE *e = 0;
    int i;

    if (safeh_HandlesEqual(gGSAPI.handle, g_lhNullHandle))
        return GSAPI_ERR_NOT_OPEN;
    if (out == 0)
        return GSAPI_ERR_NULL_ARG;
    if (gGSAPI.language == 0)
        return GSAPI_ERR_NO_LANGUAGE;
    if (gGSAPI.nopen == gGSAPI.nengines)
        return GSAPI_ERR_NO_ENGINE;
    for (i = 0; i < gGSAPI.nengines; i++) {
        e = &gGSAPI.engines[i];
        if (e->f0 == 0)
            break;
    }
    e->closing = 0;
    e->flags = 1;
    e->in_callback = 0;
    e->f96 = 0;
    e->f104 = 0;
    e->ctx = 0;
    e->energy_interval = DEFAULTENERGYINTERVAL;
    e->listening = 0;
    e->loaded = 0;
    e->user = 0;
    if (gGSAPI.read_audio != 0)
        gGSAPI.status = ExtAudioInit(gGSAPI.heap, &e->audio);
    else
        gGSAPI.status = SidInit(gGSAPI.heap, mic, &e->audio);
    if (gGSAPI.status < 0)
        return gGSAPI.status;
    memset(&e->f8, 0, 40);
    gGSAPI.status = asrspi_OpenEngine(gGSAPI.handle, 0, e);
    if (gGSAPI.status >= 0) {
        gGSAPI.nopen++;
        gGSAPI.status = asrspi_ActivateLng(e->f0, gGSAPI.language);
        *out = e;
    }
    return gGSAPI.status;
}

int gsapi_EngineSetParam(GSENGINE *e, unsigned int id, int value)
{
    if (safeh_HandlesEqual(gGSAPI.handle, g_lhNullHandle))
        return GSAPI_ERR_NOT_OPEN;
    if (!CheckEngineHandle(e))
        return GSAPI_ERR_NULL_CTX;
    if (id >= 18)
        return GSAPI_ERR_BAD_PARAM;
    if (id == 8 && value < 0)
        return GSAPI_ERR_BAD_VALUE;
    if (e->f8 == 0)
        return GSAPI_ERR_NO_CONTEXT;
    gGSAPI.status = ContextSetParam(&e->f8, id, value);
    if (gGSAPI.status != 0)
        return gGSAPI.status;
    return gGSAPI.status = ContextActivateParams(e, &e->f8);
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

/* a language file, with or without its 16-byte header (74 18 09 00); the engines' defaults then
 * taken from a parameter block -- read through a pointer the vendor never sets: the target loads
 * the set mask from address 8 and the values from 12 on (the block's +8 and +12). The values are
 * read through their own pointer, set from the block after the mask and stepped every pass, set
 * or not (GCC round 82; open since 10c, 37 words: indexed `p->value[i]` the 12 folds into the
 * loads' displacement, the target's `li a3,12` and loads at 0(a3) are this biv's). */
int gsapi_LanguageLoadBuffer(unsigned char *buf, void *params)
{
    CTXPARAMS *p = 0;
    unsigned int *v;
    unsigned int set;
    int i;

    if (safeh_HandlesEqual(gGSAPI.handle, g_lhNullHandle))
        return GSAPI_ERR_NOT_OPEN;
    if (buf == 0)
        return GSAPI_ERR_NULL_ARG;
    gsapi_LanguageUnLoad();
    if (buf[0] == 0x74 && buf[1] == 0x18 && buf[2] == 0x09 && buf[3] == 0) {
        gGSAPI.langbuf = buf;
        gGSAPI.language = buf + 16;
    } else {
        gGSAPI.language = buf;
        gGSAPI.langbuf = 0;
    }
    InitDefaultParams();
    if (params != 0) {
        set = p->set;
        v = (unsigned int *)p->value;
        for (i = 0; i < 18; i++) {
            if (set & 1)
                gGSAPI.params[i] = *v;
            v++;
            set >>= 1;
        }
    }
    return 0;
}

/* the engines are walked and nothing is done with them (the loop is the vendor's, empty) */
int gsapi_LanguageUnLoad(void)
{
    int i;

    if (safeh_HandlesEqual(gGSAPI.handle, g_lhNullHandle))
        return GSAPI_ERR_NOT_OPEN;
    if (gGSAPI.language == 0)
        return GSAPI_ERR_NO_LANGUAGE;
    for (i = 0; i < gGSAPI.nengines; i++)
        ;
    gGSAPI.language = 0;
    gGSAPI.langbuf = 0;
    return 0;
}

/* a context's data, grammar and words made the engine's own and activated */
int gsapi_EngineSetImmediateContext(GSENGINE *e, void *ctxdata, void *gcd, void *wrd)
{
    int err;

    if (safeh_HandlesEqual(gGSAPI.handle, g_lhNullHandle))
        return GSAPI_ERR_NOT_OPEN;
    if (!CheckEngineHandle(e))
        return GSAPI_ERR_NULL_CTX;
    if (ctxdata == 0)
        return GSAPI_ERR_NULL_ARG;
    err = ContextAPIDeActivate(e);
    if (err >= 0) {
        err = ContextSetCtxData(&e->f8, ctxdata);
        if (err >= 0) {
            err = ContextSetGcdData(&e->f8, gcd);
            if (err >= 0) {
                err = ContextSetWrdData(&e->f8, wrd);
                if (err >= 0)
                    err = ContextActivate(e, &e->f8);
            }
        }
    }
    return err;
}

/* a context allocated on the API's heap and loaded from its data */
int gsapi_ContextLoadBuffer(void *data, GSCONTEXT **out)
{
    GSCONTEXT *ctx;

    if (safeh_HandlesEqual(gGSAPI.handle, g_lhNullHandle))
        return GSAPI_ERR_NOT_OPEN;
    if (data == 0)
        return GSAPI_ERR_NULL_ARG;
    if (out == 0)
        return GSAPI_ERR_NULL_ARG;
    ctx = heap_Alloc(gGSAPI.heap, sizeof(GSCONTEXT));
    if (ctx == 0)
        return GSAPI_ERR_NO_MEMORY;
    memset(ctx, 0, sizeof(GSCONTEXT));
    gGSAPI.status = ContextSetCtxData(ctx, data);
    if (gGSAPI.status < 0) {
        heap_Free(gGSAPI.heap, ctx);
        return gGSAPI.status;
    }
    gGSAPI.ncontexts++;
    *out = ctx;
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

/* a parameter of a loaded context, re-applied to every engine the context is loaded into */
int gsapi_ContextSetParam(void *ctx, unsigned int id, int value)
{
    GSENGINE *e;
    int i;

    if (safeh_HandlesEqual(gGSAPI.handle, g_lhNullHandle))
        return GSAPI_ERR_NOT_OPEN;
    if (ctx == 0)
        return GSAPI_ERR_NULL_CTX;
    if (id >= 18)
        return GSAPI_ERR_BAD_PARAM;
    if (id == 8 && value < 0)
        return GSAPI_ERR_BAD_VALUE;
    gGSAPI.status = ContextSetParam(ctx, id, value);
    if (gGSAPI.status != 0)
        return gGSAPI.status;
    for (i = 0; i < gGSAPI.nengines; i++) {
        e = &gGSAPI.engines[i];
        if (e->loaded == ctx)
            ContextActivateParams(e, ctx);
    }
    return 0;
}

int gsapi_ContextGetParam(void *ctx, unsigned int id, int *value)
{
    if (safeh_HandlesEqual(gGSAPI.handle, g_lhNullHandle))
        return GSAPI_ERR_NOT_OPEN;
    if (ctx == 0)
        return GSAPI_ERR_NULL_CTX;
    if (value == 0)
        return GSAPI_ERR_NULL_ARG;
    if (id >= 18)
        return GSAPI_ERR_BAD_PARAM;
    return ContextGetParam(ctx, id, value);
}

int gsapi_EngineGetParam(GSENGINE *e, unsigned int id, int *value)
{
    if (safeh_HandlesEqual(gGSAPI.handle, g_lhNullHandle))
        return GSAPI_ERR_NOT_OPEN;
    if (!CheckEngineHandle(e))
        return GSAPI_ERR_NULL_CTX;
    if (value == 0)
        return GSAPI_ERR_NULL_ARG;
    if (id >= 18)
        return GSAPI_ERR_BAD_PARAM;
    if (e->f8 == 0)
        return GSAPI_ERR_NO_CONTEXT;
    return ContextGetParam(&e->f8, id, value);
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

/* a word's string by its id, out of the active context's word list (SOCOM II's; CReco prints the
 * recognised words with it) */
int func_004C82F0(GSENGINE *e, unsigned int id, char **word)
{
    GSCONTEXT *ctx;
    WORDLIST *wl;

    if (safeh_HandlesEqual(gGSAPI.handle, g_lhNullHandle))
        return GSAPI_ERR_NOT_OPEN;
    if (!CheckEngineHandle(e))
        return GSAPI_ERR_NULL_CTX;
    if (word == 0)
        return GSAPI_ERR_NULL_ARG;
    ctx = e->ctx;
    if (ctx->f8 == 0)
        return GSAPI_ERR_NO_WORDS;
    wl = ctx->wordlist;
    if (wl == 0)
        return GSAPI_ERR_NO_WORDLIST;
    if (id > wl->n)
        return GSAPI_ERR_BAD_WORD;
    *word = wl->words[id];
    return 0;
}

int gsapi_NotifySetCallback(GSCALLBACK cb)
{
    if (safeh_HandlesEqual(gGSAPI.handle, g_lhNullHandle))
        return GSAPI_ERR_NOT_OPEN;
    gGSAPI.notify = cb;
    return 0;
}

/* SOCOM 1's CheckEngineHandle, the real function, nothing calling it. It redefines gsapi.h's
 * extern inline here, and 2.96 does two things with that: it outputs the function at the end of
 * the file (deferred, as an inline's out-of-line copy is), where the target holds it after
 * StopListening; and it inlines THIS body into the functions after it -- the early-return form,
 * which zeroes the result again after the base test (NotifySetEnergyInterval and
 * EngineSetUserData, open since round 3b on exactly that), where every function before it has the
 * extern inline's single-result form. */
int CheckEngineHandle(GSENGINE *e)
{
    GSENGINE *base = gGSAPI.engines;

    if (e == 0)
        return 0;
    if (e < base)
        return 0;
    if (((char *)e - (char *)base) % sizeof(GSENGINE) != 0)
        return 0;
    if (e > base + gGSAPI.nengines)
        return 0;
    return e->f0 != 0;
}

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

int gsapi_GetNumContextsLoaded(void)
{
    return gGSAPI.ncontexts;
}

/* the last call's result (SOCOM II's) */
int func_004C8890(void)
{
    return gGSAPI.status;
}

/* the defaults gsapi_LanguageLoadBuffer set, made the engine's (ContextActivate's) */
int ActivateDefaultParams(GSENGINE *e)
{
    PARAM list[18];
    int n = 0;
    unsigned int i;

    for (i = 0; i < 18; i++) {
        if (TranslateParamTable[i] != 255) {
            list[n].id = TranslateParamTable[i];
            list[n].value = gGSAPI.params[i];
            n++;
        }
    }
    return asrspi_SetParamList(e->f0, list, n);
}

/* an engine started again after a result (gsapi_ExecServer's, ProcessAudio's); inside a callback
 * only marked, unless it was stopped meanwhile */
int RestartEngine(GSENGINE *e)
{
    if (e->in_callback != 0) {
        if (e->f100 == 0)
            e->f96 = 1;
        return 0;
    }
    if (e->energy_interval != 0) {
        AsrSpiSignalCallBacks.energy_level = asrspi_cbEnergyLevel;
        AsrSpiSignalCallBacks.snr_level = asrspi_cbSNRLevel;
    } else {
        AsrSpiSignalCallBacks.energy_level = 0;
        AsrSpiSignalCallBacks.snr_level = 0;
    }
    gGSAPI.status = asrspi_Start(e->f0, e->energy_interval, &AsrSpiRecogCallBacks, e);
    if (gGSAPI.status < 0)
        return gGSAPI.status;
    return gGSAPI.status = StartListening(e);
}

int TranslateGsapiParamId2AsrSpiParamId(int id)
{
    return TranslateParamTable[id];
}

/* the engine's audio started, the host told first */
int StartListening(GSENGINE *e)
{
    if (e->audio != 0) {
        if (gGSAPI.read_audio != 0) {
            if (gGSAPI.notify != 0)
                gGSAPI.notify(e, 2, 0, e->user);
            return ExtAudioStart(e->audio);
        }
        if (!SidIsRecording(e->audio)) {
            if (gGSAPI.notify != 0)
                gGSAPI.notify(e, 2, 0, e->user);
            return SidStart(e->audio);
        }
    }
    return 0;
}

int StopListening(GSENGINE *e)
{
    if (e->audio != 0) {
        if (gGSAPI.read_audio != 0) {
            if (gGSAPI.notify != 0)
                gGSAPI.notify(e, 3, 0, e->user);
            return ExtAudioStop(e->audio);
        }
        if (SidIsRecording(e->audio)) {
            if (gGSAPI.notify != 0)
                gGSAPI.notify(e, 3, 0, e->user);
            return SidStop(e->audio);
        }
    }
    return 0;
}
