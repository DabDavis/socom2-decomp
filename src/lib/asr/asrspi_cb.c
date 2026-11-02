/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing -funroll-loops */
/* The recogniser's result and event callbacks to the host (OVL2 0x4C5400-0x4C57F8): ScanSoft's
 * speech recogniser, prebuilt with ee-gcc 2.96 at -O2 -G0 -fno-strict-aliasing -funroll-loops
 * (README "The GCC front"). SOCOM 1's demo carries the same file (its object at 0x336EF0, the
 * same seven functions in the same order); the names are its symbols. Each event goes to the
 * host's notify callback (gGSAPI) with the engine marked as inside a callback. The callbacks
 * return nothing: with an int return the call sets v0 and the constant stored around it moves
 * from v1 to v0. Each returns through a guard (2.96 puts the 0 in the branch's delay slot). */

#include "gsapi.h"

typedef struct WORDREF {
    char pad[8];
    int id;                             /* +8 */
} WORDREF;

typedef struct NBITEM {
    char pad[12];
    WORDREF *word;                      /* +12 */
} NBITEM;

typedef struct NBEST {
    unsigned short nwords;              /* +0 */
    unsigned short pad2;
    int score;                          /* +4 */
    char pad8[12];
    NBITEM *items;                      /* +20 */
} NBEST;

typedef struct RESULT {
    int rejected;                       /* +0 */
    int valid;                          /* +4 */
    NBEST *best;                        /* +8 */
} RESULT;

extern int ContextGetParam(void *ctx, int id, int *value);
extern int ContextGetAction(void *ctx, RESULT *r, int *action);
extern void *func_00243298(void *heap, int size);                  /* the heap's malloc */
extern void heap_Free(void *heap, void *p);
extern int RestartEngine(GSENGINE *e);

/* OPEN: the target keeps two identical notify(e, 0, 0, 0) blocks, one for a null result and one
 * for an invalid one, where 2.96 merges them in every shape tried (if/else-if, gotos to a shared
 * tail, nested ifs; no -O2 sub-flag keeps them apart): 8 words short. The rest -- the rejected
 * path's RestartEngine shared with the flags test -- is as here. */
int asrspi_cbResult(RESULT *r, GSENGINE *e)
{
    int threshold;
    int action = 0;
    int nodefault;
    unsigned short i;

    e->in_callback = 1;
    if (r == 0) {
        if (gGSAPI.notify != 0)
            gGSAPI.notify(e, 0, 0, 0);
    } else if (r->valid == 0) {
        if (gGSAPI.notify != 0)
            gGSAPI.notify(e, 0, 0, 0);
    } else {
        if (e->ctx == 0) {
            e->in_callback = 0;
            return 0;
        }
        nodefault = ContextGetParam(e->ctx, 9, &threshold);
        e->res.rejected = r->rejected;
        e->res.nwords = r->best->nwords;
        e->res.score = r->best->score;
        e->res.ids = func_00243298(gGSAPI.heap, e->res.nwords * sizeof(int));
        for (i = 0; i < e->res.nwords; i++)
            e->res.ids[i] = r->best->items[i].word->id;
        if (nodefault != 0)
            threshold = gGSAPI.confidence;
        if (r->rejected != 0 || r->best->score < threshold) {
            if (gGSAPI.notify != 0)
                gGSAPI.notify(e, 0, &e->res, threshold);
            heap_Free(gGSAPI.heap, e->res.ids);
            if (e->f100 == 0)
                RestartEngine(e);
            e->in_callback = 0;
            return 0;
        }
        ContextGetAction(e->ctx, r, &action);
        gGSAPI.result(e, action, &e->res, e->user);
        heap_Free(gGSAPI.heap, e->res.ids);
    }
    if (e->flags & 2)
        RestartEngine(e);
    e->in_callback = 0;
    return 0;
}

int asrspi_cbAbnorm(int what, int value, GSENGINE *e)
{
    if (gGSAPI.notify == 0)
        return 0;
    e->in_callback = 1;
    gGSAPI.notify(e, 4, (void *)value, what);
    e->in_callback = 0;
    return 0;
}

int asrspi_cbAgc(void)
{
    return 0;
}

int asrspi_cbEnergyLevel(int level, GSENGINE *e)
{
    if (gGSAPI.notify == 0)
        return 0;
    e->in_callback = 1;
    gGSAPI.notify(e, 5, (void *)level, e->user);
    e->in_callback = 0;
    return 0;
}

int asrspi_cbSNRLevel(int level, GSENGINE *e)
{
    if (gGSAPI.notify == 0)
        return 0;
    e->in_callback = 1;
    gGSAPI.notify(e, 6, (void *)level, e->user);
    e->in_callback = 0;
    return 0;
}

int asrspi_cbSpeechDetected(int frame, GSENGINE *e)
{
    if (gGSAPI.notify == 0)
        return 0;
    e->in_callback = 1;
    gGSAPI.notify(e, 7, (void *)frame, e->user);
    e->in_callback = 0;
    return 0;
}

int asrspi_cbTsDetected(int frame, GSENGINE *e)
{
    if (gGSAPI.notify == 0)
        return 0;
    e->in_callback = 1;
    gGSAPI.notify(e, 8, (void *)frame, e->user);
    e->in_callback = 0;
    return 0;
}
