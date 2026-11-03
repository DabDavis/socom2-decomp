/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing -funroll-loops */
/* gender: the recogniser's gender classifier block (OVL2 0x4F4950-0x4F5F20, SOCOM 1's object at
 * 0x361878: ControlGender, GenderDataCompute, GenderSelection, InitGender, ProcessGender,
 * ConstructGender, ResetGender, all but the constructor LOCAL there), ScanSoft's speech
 * recogniser, prebuilt with ee-gcc 2.96 at -O2 -G0 -fno-strict-aliasing -funroll-loops (README
 * "The GCC front"). Names are SOCOM 1's. The block takes four inputs (a frame's level and three
 * frames of features) and gives one output; its data (0x94 bytes with the scheduler's record) is
 * named by use. Not yet written: GenderDataCompute, GenderSelection, ProcessGender (declared).
 * SetLanguage is ours: command 1's body, a `static __inline__` returning the scheduler's
 * `unsigned char` error -- the target zero-extends the result once at the join of its two
 * returns (one `andi` the error arm's `b` copies into its slot), which an inlined function's
 * QImode return gives (an `unsigned char` local assigned in both arms does not).
 *
 * ResetGender is a `static __inline__` defined after the control and before the init: GCC 2.96
 * inlines only a body it has already seen, so ControlGender (above it) calls it twice while
 * InitGender (below it) holds its body inlined, and its own copy, deferred as an inline's, comes
 * out at the object's end -- after ConstructGender, as in the image. */
#include "tosblk.h"

/* the language's table of accessors (language.c's _LangVirtualTable, copied to its head); the
 * codebook accessors take the codebook */
typedef struct LANG {
    void *pad0[4];
    int (*GetNbrCodeBook)(struct LANG *l);                          /* 4 */
    void *pad5[23];
    int (*GetCodeBookDim)(void *cdb);                               /* 28 */
    int (*GetFirstCdbSize)(void *cdb);                              /* 29 */
    int (*GetNbrInSecSearch)(void *cdb);                            /* 30 */
    int (*GetSecondCdbSize)(void *cdb);                             /* 31 */
    int (*GetCompStart)(void *cdb);                                 /* 32 */
    void *pad33;
    struct WARP *(*GetpWarpFactors)(struct LANG *l);                /* 34 */
    void *pad35[6];
    void *(*GetpCodeBook)(struct LANG *l, unsigned int n);          /* 41 */
    void *pad42[10];
    int (*CheckBitField)(struct LANG *l, int mask);                 /* 52 */
    void *pad53[5];
    int *(*GetpFirstCdb)(void *cdb);                                /* 58 */
    int *(*GetpIndexInSecCdb)(void *cdb);                           /* 59 */
    int *(*GetpSecondCdb)(void *cdb);                               /* 60 */
} LANG;

/* a warp factor as the language lists them: the genders it belongs to first (8 bytes) */
typedef struct WARP {
    unsigned int mask;
    int f4;
} WARP;

typedef struct GENDER {
    TOSBLOCK b;
    int selected;                       /* +40: 1 when a gender is chosen (or fixed) */
    unsigned int mask;                  /* +44: the genders allowed (command 8) */
    unsigned short frames;              /* +48: the fixed gender + 1, or the frames seen */
    unsigned char fixed;                /* +50: the mask allows one gender only */
    char pad51;
    float *start;                       /* +52: the frame ring */
    float *end;                         /* +56 */
    float *pos;                         /* +60 */
    float *last;                        /* +64 */
    float *scores;                      /* +68: one per model */
    int f72;
    float f76;
    int f80;
    float f84;
    unsigned char dim;                  /* +88: the codebook's (command 1) */
    unsigned char first_size;           /* +89 */
    unsigned char nbr_in_sec;           /* +90 */
    unsigned char second_size;          /* +91 */
    short comp_start;                   /* +92 */
    char pad94[2];
    int *first;                         /* +96 */
    int *index_in_sec;                  /* +100 */
    int *second;                        /* +104 */
    unsigned char *enabled;             /* +108: a flag per model, from 1 */
    unsigned int n;                     /* +112: features a frame (profile 6) */
    unsigned int f116;                  /* +116: profile 5 (1) */
    unsigned int nmodels;               /* +120: the inputs but the first */
    unsigned int nframes;               /* +124: the ring's frames (profile 1, 50) */
    unsigned int framesize;             /* +128: nmodels * n */
    int f132;                           /* +132: profile 2 (600) */
    int f136;                           /* +136: profile 3 (10) */
    int f140;                           /* +140: profile 4 (20) */
    LANG *lng;                          /* +144 */
} GENDER;                               /* 0x94 */

extern int qQueueControl(void *q, unsigned char cmd, unsigned short reader, void *arg);

extern void ProcessGender(GENDER *g, unsigned char **in, int n);
static __inline__ void ResetGender(GENDER *g);

static __inline__ unsigned char SetLanguage(GENDER *g, LANG *lng)
{
    void *cdb;

    if (lng->CheckBitField(lng, 4))
        cdb = lng->GetpCodeBook(lng, lng->GetNbrCodeBook(lng));
    else
        cdb = lng->GetpCodeBook(lng, 1);
    g->dim = lng->GetCodeBookDim(cdb);
    g->first_size = lng->GetFirstCdbSize(cdb);
    g->nbr_in_sec = lng->GetNbrInSecSearch(cdb);
    g->second_size = lng->GetSecondCdbSize(cdb);
    g->comp_start = (unsigned char)lng->GetCompStart(cdb);
    g->first = lng->GetpFirstCdb(cdb);
    g->index_in_sec = lng->GetpIndexInSecCdb(cdb);
    g->second = lng->GetpSecondCdb(cdb);
    if (g->dim > g->n - g->f116)
        return _tosErrorLog(g, 102);
    g->lng = lng;
    return 0;
}

static int ControlGender(GENDER *g, unsigned char cmd, void *arg, int *pos)
{
    TOS *tos = g->b.tos;
    int ret = 1;

    switch (cmd) {
    case 3:
        if (g->f72 != 0 || arg != 0) {
            if (g->frames == 0)
                g->f84 = g->f136 + 1.0f;
            g->selected = 1;
        }
        break;
    case 6:
        if (arg != 0) {
            unsigned char *p = (unsigned char *)arg + *pos;
            unsigned int i;

            g->selected = *(int *)p;
            p += 4;
            g->frames = *(unsigned short *)p;
            p += 4;
            for (i = 0; i < g->nmodels; i++) {
                g->scores[i] = *(float *)p;
                p += 4;
            }
            g->f72 = *(int *)p;
            p += 4;
            g->f76 = *(float *)p;
            p += 4;
            g->f80 = *(int *)p;
            p += 4;
            g->f84 = *(float *)p;
            *pos += g->nmodels * 4 + 24;
        } else
            ResetGender(g);
        break;
    case 7:
        if (arg != 0) {
            unsigned char *p = (unsigned char *)arg + *pos;
            unsigned int i;

            *(int *)p = g->selected;
            p += 4;
            *(int *)p = g->frames;
            p += 4;
            for (i = 0; i < g->nmodels; i++) {
                *(float *)p = g->scores[i];
                p += 4;
            }
            *(int *)p = g->f72;
            p += 4;
            *(float *)p = g->f76;
            p += 4;
            *(int *)p = g->f80;
            p += 4;
            *(float *)p = g->f84;
        }
        *pos += g->nmodels * 4 + 24;
        break;
    case 4:
        ret = qQueueControl(g->b.inputs[0].q, 7, g->b.inputs[0].reader, arg);
        break;
    case 5:
        ret = qQueueControl(g->b.inputs[0].q, 6, g->b.inputs[0].reader, arg);
        break;
    case 8: {
        unsigned int mask = *(unsigned int *)arg;
        WARP *w = g->lng->GetpWarpFactors(g->lng);
        unsigned int count = 0;
        unsigned short sel = 0;
        unsigned int i;
        unsigned char one;

        for (i = 0; i < g->nmodels; i++) {
            if (w[i].mask & mask) {
                count++;
                sel = i;
            }
        }
        one = count == 1;
        if (one != g->fixed)
            ResetGender(g);
        if (one) {
            g->frames = sel + 1;
            if (g->selected == 0)
                g->selected = 1;
        }
        g->fixed = one;
        g->mask = mask;
        break;
    }
    case 9:
        *(unsigned int *)arg = g->mask;
        break;
    case 1:
        ret = SetLanguage(g, arg) == 0;
        break;
    case 2:
        g->enabled[(int)arg + 1] = 1;
        break;
    case 10:
        g->enabled[(int)arg + 1] = 0;
        break;
    case 255:
        heap_Free(tos->heap, g->scores);
        heap_Free(tos->heap, g->start);
        heap_Free(tos->heap, g->enabled);
        tosBaseBlockDestruct(&g->b);
        break;
    default:
        ret = 0;
        break;
    }
    return ret;
}

static __inline__ void ResetGender(GENDER *g)
{
    unsigned int i;

    g->selected = 0;
    if (g->fixed != 0)
        g->selected = 1;
    else
        g->frames = 0;
    g->last = g->start;
    g->pos = g->start;
    for (i = 0; i < g->nmodels; i++)
        g->scores[i] = 0;
    g->f72 = 0;
    g->f76 = 0;
    g->f80 = 0;
    g->f84 = 0;
}

static int InitGender(GENDER *g)
{
    TOS *tos = g->b.tos;

    g->nmodels = g->b.ninputs - 1;
    g->f116 = (unsigned short)_tosGetProfileU32(g, 5, 1);
    g->n = (unsigned short)_tosGetProfileU32(g, 6, 0);
    g->nframes = (unsigned short)_tosGetProfileU32(g, 1, 50);
    g->f132 = (short)_tosGetProfileU32(g, 2, 600);
    g->f136 = (short)_tosGetProfileU32(g, 3, 10);
    g->f140 = (short)_tosGetProfileU32(g, 4, 20);
    g->framesize = g->nmodels * g->n;
    g->start = heap_Calloc(tos->heap, g->nframes * g->framesize, 4);
    g->end = g->start + g->nframes * g->framesize;
    g->scores = heap_Alloc(tos->heap, g->nmodels * 4);
    g->enabled = heap_Alloc(tos->heap, g->nmodels + 1);
    if (g->start == 0 || g->scores == 0 || g->enabled == 0)
        return 1;
    memset(g->enabled, 1, g->nmodels + 1);
    g->b.inputs[0].size = 4;
    g->b.inputs[1].size = g->n * 4;
    g->b.inputs[2].size = g->n * 4;
    g->b.inputs[3].size = g->n * 4;
    g->b.outputs[0].size = g->n * 4;
    g->mask = 0x7FFFFFFF;
    ResetGender(g);
    return 0;
}

TOSBLOCK *ConstructGender(TOS *tos, unsigned char id)
{
    return tosBaseBlockConstruct(tos, id, 4, 1, (TOSFUNC)ProcessGender, (TOSFUNC)InitGender,
                                 (TOSFUNC)ControlGender, sizeof(GENDER));
}
