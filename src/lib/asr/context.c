/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing -funroll-loops */
/* The recogniser's contexts (OVL2 0x4C57F8-0x4C60F8): ScanSoft's speech recogniser, prebuilt with
 * ee-gcc 2.96 at -O2 -G0 -fno-strict-aliasing -funroll-loops (README "The GCC front"). SOCOM 1's
 * demo carries the file (its object at 0x3372C8: the same functions in the same order, but the
 * last, the error translator, which is SOCOM II's); the names are its symbols. A context holds
 * the engine handle, its action table and a lazily allocated parameter block. */

#include "gsapi.h"

#define CTX_NPARAMS     18
#define ERR_NOT_SET     0x80CC0008
#define ERR_NO_ACTIONS  0x80CC0009
#define ERR_NO_MEMORY   0x80CC0086

typedef struct CTXPARAMS {
    int f0;
    int f4;
    unsigned int set;                   /* +8: a bit per parameter */
    int value[CTX_NPARAMS];             /* +12 */
} CTXPARAMS;

typedef struct ACTIONTAB {
    unsigned int type;                  /* +0: 2 by index, 3 by word, 4 by word over a score */
    int f4;
    unsigned int n;                     /* +8 */
    int *keys;                          /* +12 */
    int *actions;                       /* +16 */
} ACTIONTAB;

typedef struct WORDS {
    char pad[36];
    void *list;                         /* +36 */
} WORDS;

typedef struct CONTEXT {
    void *engine;                       /* +0 */
    int f4;
    int f8;
    CTXPARAMS *params;                  /* +12 */
    ACTIONTAB *actions;                 /* +16 */
    char pad20[28];
    int active;                         /* +48 */
    char pad52[20];
    int f72;                            /* +72 */
} CONTEXT;

typedef struct WORDREF {
    char pad[8];
    int id;                             /* +8 */
} WORDREF;

typedef struct NBITEM {
    char pad[12];
    WORDREF *word;                      /* +12 */
} NBITEM;

typedef struct NBEST {
    int score;                          /* +0 */
    int f4;
    char pad8[12];
    NBITEM *items;                      /* +20 */
} NBEST;

typedef struct RESULT {
    int rejected;
    int valid;
    NBEST *best;                        /* +8 */
} RESULT;

typedef struct PARAM {
    int id;
    int value;
} PARAM;

typedef struct ERRMAP {
    int code;
    int value;
} ERRMAP;

extern int asrspi_ActivateCtx(void *engine, int a, int b);
extern int asrspi_ActivateWords(void *engine, void *words, int n);
extern int asrspi_SetParamList(void *engine, PARAM *list, int n);
extern void *func_00243398(void *heap, int count, int size);       /* the heap's calloc */
extern int TranslateGsapiParamId2AsrSpiParamId(int id);
extern ERRMAP data_00656400[];

int ContextGetAction(CONTEXT *c, RESULT *r, int *action)
{
    int word;
    unsigned int i;

    *action = 0;
    if (c == 0)
        return 0;
    if (c->actions == 0)
        return ERR_NO_ACTIONS;
    word = r->best->items[1].word->id;
    switch (c->actions->type) {
    case 2:
        if (word < c->actions->n)
            *action = c->actions->keys[word];
        break;
    case 3:
        for (i = 0; i < c->actions->n; i++)
            if (c->actions->keys[i] == word) {
                *action = c->actions->actions[i];
                break;
            }
        break;
    case 4:
        for (i = 0; i < c->actions->n; i++)
            if (c->actions->keys[i * 2] < r->best->score && c->actions->keys[i * 2 + 1] == word)
                *action = c->actions->actions[i];
        break;
    }
    return 0;
}

int ContextAPIDeActivate(CONTEXT *c)
{
    c->active = 0;
    c->f72 = 0;
    return 0;
}

int ContextDeActivate(CONTEXT *c)
{
    int r = asrspi_ActivateCtx(c->engine, 0, 0);

    if (r < 0)
        return r;
    c->active = 0;
    c->f72 = 0;
    return 0;
}

int ContextSetCtxData(CONTEXT *c, void *engine)
{
    c->engine = engine;
    return 0;
}

int ContextSetActiveWords(CONTEXT *c, WORDS *w)
{
    if (w->list == 0)
        return 0;
    return asrspi_ActivateWords(c->engine, w->list, -1);
}

int ContextGetParam(CONTEXT *c, int id, int *value)
{
    CTXPARAMS *p = c->params;

    if (p == 0)
        return ERR_NOT_SET;
    if ((p->set & (1 << id)) == 0)
        return ERR_NOT_SET;
    *value = p->value[id];
    return 0;
}

int ContextSetParam(CONTEXT *c, int id, int value)
{
    if (c->params == 0) {
        c->params = func_00243398(gGSAPI.heap, 1, sizeof(CTXPARAMS));
        if (c->params == 0)
            return ERR_NO_MEMORY;
    }
    c->params->set |= 1 << id;
    c->params->value[id] = value;
    return 0;
}

int ContextActivateParams(CONTEXT *dst, CONTEXT *c)
{
    PARAM list[CTX_NPARAMS];
    int n = 0;
    unsigned int set;
    unsigned int i;
    int id;

    if (c->params == 0)
        return 0;
    set = c->params->set;
    for (i = 0; i < CTX_NPARAMS; i++) {
        if (set & 1) {
            id = TranslateGsapiParamId2AsrSpiParamId(i);
            if (id != 255) {
                list[n].id = id;
                list[n].value = c->params->value[i];
                n++;
            }
        }
        set >>= 1;
    }
    return asrspi_SetParamList(dst->engine, list, n);
}

/* OPEN: the error translator (new since SOCOM 1: a 0x80CCxxxx code to the host's, through the
 * {code, value} table at 0x656400 that ends at code 127). The target re-tests err == 0 after
 * substituting 0x80CC007F for a foreign code (a jump back to the top) and walks the table from
 * its second entry; this first shape is 26 words off and the size differs (0x84 against 0x80). */
int func_004C6078(unsigned int err)
{
    ERRMAP *m = data_00656400;

    if (err != 0) {
        if (((err >> 7) & 1) || ((err >> 16) & 0x7FFF) != 0xCC)
            err = 0x80CC007F;
        if (m->code != 127)
            for (m++; m->code != 127; m++)
                if ((err & 0x7F) == m->code)
                    break;
    }
    return m->value;
}
