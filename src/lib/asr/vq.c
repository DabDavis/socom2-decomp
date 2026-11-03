/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing -funroll-loops */
/* The recogniser's vector quantiser (OVL2 0x4FBA38-0x4FC240, SOCOM 1's object at 0x367F48:
 * LoadCodeBooks, GetLabel, FreeVq1500, ConstructVq, InitVq1500Process, FreeCodeBooks,
 * IdleVq1500Process, ControlVq1500Process, Vq1500Process -- SOCOM II defines FreeVq1500 last),
 * ScanSoft's speech recogniser, prebuilt with ee-gcc 2.96 at -O2 -G0 -fno-strict-aliasing
 * -funroll-loops (README "The GCC front"). Names are SOCOM 1's (s1:align; GetLabel and the idle
 * process by position and Vq1500Process's call, ControlVq1500Process by position, FreeVq1500 by
 * the call SOCOM 1's ControlVq1500Process makes there). The block turns a feature frame into one
 * label per codebook: the codebooks come from the language (its table's codebook getters, on
 * command 1 with the language), each a first search over `n1` centroids of `dim` components
 * from `compstart` in the frame, then a second over the `nsec` codewords the first's winner
 * lists, each followed by its label. Without codebooks the block idles. */
#include "tosblk.h"

/* the EE's largest float: no infinity, so the all-ones exponent is a number (0x7FFFFFFF) */
#define FLT_MAX 6.80564694e+38f

typedef struct CODEBOOK {
    unsigned char dim;                  /* +0 */
    unsigned char n1;                   /* +1: the first codebook's size */
    unsigned char nsec;                 /* +2: codewords in the second search */
    unsigned char n2;                   /* +3: the second codebook's size */
    unsigned short compstart;           /* +4 */
    float *first;                       /* +8: n1 * dim */
    int *index;                         /* +12: nsec per first codeword, offsets into second */
    float *second;                      /* +16 */
} CODEBOOK;

/* the language's table of accessors (language.c's _LangVirtualTable, copied to its head) */
typedef struct LANG {
    void *pad0[4];
    int (*GetNbrCodeBook)(struct LANG *l);        /* 4 */
    void *pad5[23];
    int (*cdbGetCodeBookDim)(void *cdb);          /* 28 */
    int (*cdbGetFirstCdbSize)(void *cdb);         /* 29 */
    int (*cdbGetNbrInSecSearch)(void *cdb);       /* 30 */
    int (*cdbGetSecondCdbSize)(void *cdb);        /* 31 */
    int (*cdbGetCompStart)(void *cdb);            /* 32 */
    void *pad33[8];
    void *(*GetpCodeBook)(struct LANG *l, unsigned int n);  /* 41 */
    void *pad42[16];
    float *(*cdbGetpFirstCdb)(void *cdb);                   /* 58 */
    int *(*cdbGetpIndexInSecCdb)(void *cdb);                /* 59 */
    float *(*cdbGetpSecondCdb)(void *cdb);                  /* 60 */
} LANG;

typedef struct VQ {
    TOSBLOCK b;
    unsigned char nbooks;               /* +40 */
    CODEBOOK *books;                    /* +44 */
} VQ;

static int LoadCodeBooks(VQ *v, LANG *lang);
static unsigned char GetLabel(CODEBOOK *cb, float *x);
static int InitVq1500Process(VQ *v);
static __inline__ void FreeCodeBooks(TOS *tos, VQ *v);
static void IdleVq1500Process(VQ *v, float **in);
static int ControlVq1500Process(VQ *v, unsigned char cmd, LANG *lang);
static void Vq1500Process(VQ *v, float **in);
static void FreeVq1500(VQ *v);

static int LoadCodeBooks(VQ *v, LANG *lang)
{
    TOS *tos = v->b.tos;
    CODEBOOK *cb;
    void *cdb;
    unsigned int i;

    v->nbooks = lang->GetNbrCodeBook(lang);
    FreeCodeBooks(tos, v);
    v->books = heap_Alloc(tos->heap, v->nbooks * sizeof(CODEBOOK));
    if (v->books == 0)
        return _tosErrorLog(v, 2);
    cb = v->books;
    for (i = 0; i < v->nbooks; i++) {
        cdb = lang->GetpCodeBook(lang, i);
        cb->dim = lang->cdbGetCodeBookDim(cdb);
        cb->n1 = lang->cdbGetFirstCdbSize(cdb);
        cb->nsec = lang->cdbGetNbrInSecSearch(cdb);
        cb->n2 = lang->cdbGetSecondCdbSize(cdb);
        cb->compstart = (unsigned char)lang->cdbGetCompStart(cdb);
        cb->first = lang->cdbGetpFirstCdb(cdb);
        cb->index = lang->cdbGetpIndexInSecCdb(cdb);
        cb->second = lang->cdbGetpSecondCdb(cdb);
        cb++;
    }
    return 0;
}

/* OPEN (0x46C; this shape is the nearest, the wrong size): the target unrolls both outer loops
 * by two (n1 and nsec preconditioned with `& 1`, the second counted down), keeps the label in
 * s0 and FLT_MAX's address in s1 under a frame, hoists x[0] and x + 1, and runs each distance
 * loop with its first step outside and both pointers strength-reduced (x + 2 and c + 2 walked,
 * the `j < dim` test at the loop's top with an empty delay slot, `best < d` at its bottom).
 * The break after the sum is the target's (a `d <= best` loop condition tests at the top);
 * pointer walks for x, a dim local and indexing the codeword all miss by the size. */
static unsigned char GetLabel(CODEBOOK *cb, float *x)
{
    float best = FLT_MAX;
    unsigned char label = 0;
    float d, t;
    float *c;
    int *idx;
    int i, j;

    for (i = 0; i < cb->n1; i++) {
        c = cb->first + i * cb->dim;
        t = x[0] - c[0];
        d = t * t;
        for (j = 1; j < cb->dim; j++) {
            t = x[j] - c[j];
            d += t * t;
            if (best < d)
                break;
        }
        if (d < best) {
            best = d;
            label = i;
        }
    }
    best = FLT_MAX;
    idx = cb->index + cb->nsec * label;
    for (i = 0; i < cb->nsec; i++) {
        c = cb->second + *idx++;
        t = x[0] - *c++;
        d = t * t;
        for (j = 1; j < cb->dim; j++) {
            t = x[j] - *c++;
            d += t * t;
            if (best < d)
                break;
        }
        if (d < best) {
            label = *(unsigned char *)c;
            best = d;
        }
    }
    return label;
}

TOSBLOCK *ConstructVq(TOS *tos, unsigned char id)
{
    return tosBaseBlockConstruct(tos, id, 1, 1, (TOSFUNC)IdleVq1500Process,
                                 (TOSFUNC)InitVq1500Process, (TOSFUNC)ControlVq1500Process,
                                 sizeof(VQ));
}

static int InitVq1500Process(VQ *v)
{
    v->b.inputs[0].size = _tosGetProfileU32(v, 1, 38) * sizeof(float);
    v->b.outputs[0].size = (unsigned char)_tosGetProfileU32(v, 2, 4);
    return 0;
}

static __inline__ void FreeCodeBooks(TOS *tos, VQ *v)
{
    if (v->books != 0) {
        heap_Free(tos->heap, v->books);
        v->books = 0;
    }
}

static void IdleVq1500Process(VQ *v, float **in)
{
}

static int ControlVq1500Process(VQ *v, unsigned char cmd, LANG *lang)
{
    switch (cmd) {
    case 1:
        if (LoadCodeBooks(v, lang) != 0)
            v->b.process = (TOSFUNC)IdleVq1500Process;
        else
            v->b.process = (TOSFUNC)Vq1500Process;
        break;
    case TOS_DESTROY:
        FreeVq1500(v);
        break;
    default:
        return 0;
    }
    return 1;
}

static void Vq1500Process(VQ *v, float **in)
{
    float *frame = in[0];
    unsigned char *out;
    CODEBOOK *cb;
    int i;

    if (frame != 0) {
        out = qEnQueueOne(v->b.outputs[0].q);
        cb = v->books;
        for (i = 0; i < v->nbooks; i++) {
            *out++ = GetLabel(cb, frame + cb->compstart);
            cb++;
        }
    }
}

static void FreeVq1500(VQ *v)
{
    FreeCodeBooks(v->b.tos, v);
    tosBaseBlockDestruct(&v->b);
}
