/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing -funroll-loops */
/* The recogniser's contexts (OVL2 0x4C57F8-0x4C60F8): ScanSoft's speech recogniser, prebuilt with
 * ee-gcc 2.96 at -O2 -G0 -fno-strict-aliasing -funroll-loops (README "The GCC front"). SOCOM 1's
 * demo carries the file (its object at 0x3372C8: the same functions in the same order, but the
 * last, gsapi_ErrorGetString, which SOCOM 1 defines in its gsapi object); the names are its
 * symbols (the last's: the body and CReco's calls, where SOCOM 1's reco calls it). A context holds
 * the engine handle, its action table and a lazily allocated parameter block. Round 31c:
 * ContextSetGcdData, SOCOM 1's body -- cases 3 and 4 written apart (one label each: merged, the
 * switch tests a range; apart, 2.96 tests each value and cross-jumping merges the two bodies, the
 * target's two `beql`), the record's type read with its size, before the size test. */

#include "gsapi.h"

#define CTX_NPARAMS     18
#define ERR_NOT_SET     0x80CC0008
#define ERR_NO_ACTIONS  0x80CC0009
#define ERR_NO_MEMORY   0x80CC0086

typedef struct CTXPARAMS {
    int f0;
    int size;                           /* +4: a grammar's record size (84); 0 allocated here */
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

typedef struct GCD GCD;

typedef struct CONTEXT {
    void *engine;                       /* +0 */
    GCD *gcd;                           /* +4: the grammar data (ContextSetGcdData) */
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

typedef struct ERRSTR {
    int code;                           /* the low seven bits of a 0x80CCxxxx code; 127 ends */
    char *string;
} ERRSTR;

extern int asrspi_ActivateCtx(void *engine, int a, int b);
extern int asrspi_ActivateWords(void *engine, void *words, int n);
extern int asrspi_SetParamList(void *engine, PARAM *list, int n);
extern void *func_00243398(void *heap, int count, int size);       /* the heap's calloc */
extern int TranslateGsapiParamId2AsrSpiParamId(int id);
extern void heap_Free(void *heap, void *p);
extern ERRSTR data_00656400[];       /* "Call Succeeded", then {code, message} to 127's */

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

/* a grammar's data: "GCD " and its size, then records -- each a type and its size: 1 the
 * context's parameter block (84 bytes), 2 to 4 its action table (offsets in the file) */
#define GCD_MAGIC       0x20444347      /* "GCD " */
#define ERR_BAD_GCD     0x80CC0003

struct GCD {
    unsigned int magic;                 /* +0 */
    unsigned int size;                  /* +4: bytes, the header included */
};

typedef struct GCDREC {
    unsigned int type;                  /* +0 */
    unsigned int size;                  /* +4 */
} GCDREC;

/* the grammar data taken: the parameter block the context allocated given back, then each of the
 * data's records made the context's parameters or actions (an action table's offsets relocated in
 * place); 0x80CC0003 for data that is not a grammar's. SOCOM 1's body (its record size a byte) */
int ContextSetGcdData(CONTEXT *c, GCD *gcd)
{
    GCDREC *r;
    unsigned int size, n, type;

    if (c->params != 0) {
        if (c->params->size == 0)
            heap_Free(gGSAPI.heap, c->params);
        c->params = 0;
    }
    if (gcd == 0) {
        c->gcd = 0;
        c->actions = 0;
        return 0;
    }
    if (gcd->magic != GCD_MAGIC)
        return ERR_BAD_GCD;
    c->gcd = gcd;
    c->actions = 0;
    size = gcd->size;
    if (size < 9)
        return ERR_BAD_GCD;
    for (r = (GCDREC *)(gcd + 1); (char *)r != (char *)c->gcd + size; r = (GCDREC *)((char *)r + n)) {
        n = r->size;
        type = r->type;
        if (n == 0)
            return ERR_BAD_GCD;
        switch (type) {
        case 1:
            c->params = (CTXPARAMS *)r;
            if (c->params->size != sizeof(CTXPARAMS))
                return ERR_BAD_GCD;
            break;
        case 2:
            c->actions = (ACTIONTAB *)r;
            c->actions->keys = (int *)((char *)c->actions + (int)c->actions->keys);
            break;
        case 3:
            c->actions = (ACTIONTAB *)r;
            c->actions->actions = (int *)((char *)c->actions + (int)c->actions->actions);
            c->actions->keys = (int *)((char *)c->actions + (int)c->actions->keys);
            break;
        case 4:
            c->actions = (ACTIONTAB *)r;
            c->actions->actions = (int *)((char *)c->actions + (int)c->actions->actions);
            c->actions->keys = (int *)((char *)c->actions + (int)c->actions->keys);
            break;
        default:
            return ERR_BAD_GCD;
        }
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

/* SOCOM 1's gsapi_ErrorGetString (its gsapi object's; here at the end of the context object, the
 * extaudio object between): a code's message out of the table that starts with the success
 * string and ends at code 127. A code with bit 7 set (SOCOM II's test) or of another module is
 * asked for again as 0x80CC007F: 2.96 turns the tail recursion into the jump back to the top,
 * the table's address re-set in its delay slot. The recursion last puts its block after the
 * loop's return, as the target has it; the key compared first gives `beq key, code`. */
char *gsapi_ErrorGetString(unsigned int err)
{
    ERRSTR *e = data_00656400;

    if (err == 0)
        return e->string;
    if (!((err >> 7) & 1) && ((err >> 16) & 0x7FFF) == 0xCC) {
        while (e->code != 127) {
            e++;
            if ((err & 0x7F) == e->code)
                return e->string;
        }
        return e->string;
    }
    return gsapi_ErrorGetString(0x80CC007F);
}
