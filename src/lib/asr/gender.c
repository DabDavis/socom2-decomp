/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing -funroll-loops -ffast-math */
/* gender: the recogniser's gender classifier block (OVL2 0x4F4950-0x4F5F20, SOCOM 1's object at
 * 0x361878: ControlGender, GenderDataCompute, GenderSelection, InitGender, ProcessGender,
 * ConstructGender, ResetGender, all but the constructor LOCAL there), ScanSoft's speech
 * recogniser, prebuilt with ee-gcc 2.96 at -O2 -G0 -fno-strict-aliasing -funroll-loops (README
 * "The GCC front"). Names are SOCOM 1's. The block takes four inputs (a frame's level and three
 * frames of features) and gives one output; its data (0x94 bytes with the scheduler's record) is
 * named by use. The object is built with -ffast-math
 * (GenderDataCompute's sqrt.s and folded first step; the other six compile the same either way).
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
    float *(*GetpFirstCdb)(void *cdb);                              /* 58 */
    int *(*GetpIndexInSecCdb)(void *cdb);                           /* 59 */
    float *(*GetpSecondCdb)(void *cdb);                             /* 60 */
} LANG;

/* a warp factor as the language lists them: the genders it belongs to first (8 bytes) */
typedef struct WARP {
    unsigned int mask;
    int f4;
} WARP;

/* the block's state (ProcessGender switches on it: an enum, compared unsigned -- the target's
 * 1, 0, 2 equality chain, where an int takes slti): the ring filling with no gender chosen, a
 * gender chosen (or fixed), and the ring draining (ours) */
typedef enum GENDERSTATE {
    GENDER_FILL,
    GENDER_CHOSEN,
    GENDER_DRAIN
} GENDERSTATE;

typedef struct GENDER {
    TOSBLOCK b;
    GENDERSTATE state;                  /* +40 */
    int mask;                           /* +44: the genders allowed (command 8), the top bit
                                         * for an automatic choice */
    unsigned short frames;              /* +48: the fixed gender + 1, or the frames seen */
    unsigned char fixed;                /* +50: the mask allows one gender only */
    char pad51;
    float *start;                       /* +52: the frame ring */
    float *end;                         /* +56 */
    float *pos;                         /* +60 */
    float *last;                        /* +64 */
    float *scores;                      /* +68: one per model */
    int count;                          /* +72: the frames in the ring */
    float f76;
    int f80;
    float f84;
    unsigned char dim;                  /* +88: the codebook's (command 1) */
    unsigned char first_size;           /* +89 */
    unsigned char nbr_in_sec;           /* +90 */
    unsigned char second_size;          /* +91 */
    short comp_start;                   /* +92 */
    char pad94[2];
    float *first;                       /* +96 */
    int *index_in_sec;                  /* +100 */
    float *second;                      /* +104 */
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

/* the EE's largest float (no infinity): 0x7FFFFFFF through 2.96's parser */
#define FLT_MAX 6.80564694e+38f

extern int qQueueControl(void *q, unsigned char cmd, unsigned short reader, void *arg);
extern unsigned char _tosControl(void *log, unsigned char cmd, int arg, int n);
extern float sqrtf(float x);

static void ProcessGender(GENDER *g, unsigned char **in);
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
        if (g->count != 0 || arg != 0) {
            if (g->frames == 0)
                g->f84 = g->f136 + 1.0f;
            g->state = 1;
        }
        break;
    case 6:
        if (arg != 0) {
            unsigned char *p = (unsigned char *)arg + *pos;
            unsigned int i;

            g->state = *(int *)p;
            p += 4;
            g->frames = *(unsigned short *)p;
            p += 4;
            for (i = 0; i < g->nmodels; i++) {
                g->scores[i] = *(float *)p;
                p += 4;
            }
            g->count = *(int *)p;
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

            *(int *)p = g->state;
            p += 4;
            *(int *)p = g->frames;
            p += 4;
            for (i = 0; i < g->nmodels; i++) {
                *(float *)p = g->scores[i];
                p += 4;
            }
            *(int *)p = g->count;
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
            if (g->state == 0)
                g->state = 1;
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

/* the frame's distance to each enabled model's codebook (the language's two-stage search: the
 * nearest first-stage codeword, then its second-stage candidates), its root scaled by the level
 * and averaged into the model's score over at most f132 frames. The object is built with
 * -ffast-math: sqrtf is a bare sqrt.s (no errno test) and each first-stage distance loop's first
 * step stores t * t over the 0 it starts from (362 words off without the flag).
 * The second stage walks the frame with a pointer of its own (q) and the codeword with the first
 * stage's c: the frame walker is then live inside one stage's codeword loop only, so the unroller
 * gives each unrolled copy its own pseudo, and global allocation takes the codeword walker first
 * (a2, the frame walker a3 -- the target's). With one frame walker for both stages it outranks c
 * (204 references to 152) and the two swap registers in both stages (93 words); with new pointers
 * for both walkers in the second stage, too. Measured besides: the count through its field
 * (`g->f80++`, then `1.0f / g->f80` -- a local keeps it in v1), a block-local `t` (65 words: a
 * function-level one takes another FPR for t * t), the index table read as `*idx` with `idx++`
 * at the iteration's end (idx[k] and `*idx++` move the count-down loop's registers). */
static void GenderDataCompute(GENDER *g, float *frame, float level)
{
    unsigned int i;
    float *x, *end, *p, *c, *q;
    int *idx;
    float best, d, rate;
    int k, label;

    g->f76 += level;
    g->f84 += level;
    if (g->f80 < g->f132)
        g->f80++;
    rate = 1.0f / g->f80;
    x = frame + g->f116;
    for (i = 0; i < g->nmodels; i++) {
        if (g->enabled[i + 1]) {
            end = x + g->n - g->f116;
            label = -1;
            best = FLT_MAX;
            c = g->first;
            for (k = 0; k < g->first_size; k++) {
                for (d = 0, p = x; p < end; p++, c++) {
                    float t = *p - *c;
                    d += t * t;
                    if (best < d)
                        break;
                }
                if (p == end) {
                    best = d;
                    label = k;
                } else
                    c += end - p;
            }
            if (g->nbr_in_sec != 0) {
                best = FLT_MAX;
                idx = g->index_in_sec + g->nbr_in_sec * label;
                for (k = 0; k < g->nbr_in_sec; k++) {
                    c = g->second + *idx;
                    d = 0;
                    q = x;
                    while (q < end) {
                        float t = *q++ - *c++;
                        d += t * t;
                        if (best < d)
                            break;
                    }
                    if (q == end)
                        best = d;
                    idx++;
                }
            }
            g->scores[i] += rate * (sqrtf(best) * level - g->scores[i]);
        }
        x += g->n;
    }
}

/* the enabled model with the lowest score (1-based); with the choice automatic (the mask's top
 * bit) and the evidence enough, the other models are disabled and the choice fixed. The table is
 * SOCOM 1's function-local ControlID_Disable.27 (the same three bytes). `best` is incremented in
 * place on both paths (the target's `addiu s2,s2,1` in three branch slots: `best + 1` is another
 * register, 21-40 words); the fixed path's three stores in one of two orders of six (ccperm). */
static unsigned short GenderSelection(GENDER *g)
{
    static const unsigned char ControlID_Disable[] = { 13, 14, 15 };
    unsigned int i, best = 0;
    float min = FLT_MAX;

    for (i = 0; i < g->nmodels; i++)
        if (g->enabled[i + 1] && g->scores[i] < min) {
            min = g->scores[i];
            best = i;
        }
    if (g->f140 < g->f76 && g->f76 > 300.0f && g->mask < 0) {
        for (i = 0; i < g->nmodels; i++)
            if (i != best)
                _tosControl(g, ControlID_Disable[i], i, best);
        best++;
        switch (best) {
        case 1:
            g->mask = 1;
            break;
        case 2:
            g->mask = 2;
            break;
        case 3:
            g->mask = 4;
            break;
        default:
            return 0;
        }
        g->fixed = 1;
        g->f84 = 0;
        g->state = 1;
        return best;
    }
    best++;
    g->f84 = 0;
    return best;
}

static __inline__ void ResetGender(GENDER *g)
{
    unsigned int i;

    g->state = 0;
    if (g->fixed != 0)
        g->state = 1;
    else
        g->frames = 0;
    g->last = g->start;
    g->pos = g->start;
    for (i = 0; i < g->nmodels; i++)
        g->scores[i] = 0;
    g->count = 0;
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

/* the oldest frame in the ring out to the output queue, as the selected (or fixed) gender's
 * features; the selection is redone first while no gender is chosen, or the evidence since the
 * last choice is enough (ours: SOCOM 1's ProcessGender holds it inlined three times too) */
static __inline__ void OutputFrame(GENDER *g)
{
    float *out;

    if (g->count != 0) {
        out = qEnQueueOne(g->b.outputs[0].q);
        if (out != 0) {
            if (g->fixed == 0 && (g->state == GENDER_FILL || g->f136 < g->f84))
                g->frames = GenderSelection(g);
            memcpy(out, g->pos + (g->frames - 1) * g->n, g->n * 4);
            g->pos += g->framesize;
            if (g->pos == g->end)
                g->pos = g->start;
            g->count--;
        }
    }
}

/* a frame in: each enabled model's features into the ring (a disabled model's slot is skipped),
 * and the scores updated when the level is above the floor and the gender is not fixed (ours).
 * The write position is read before the input test (the target's load in that branch's slot)
 * and the level into a local before the fixed test (in the slot of that one: the first in[0]
 * load serves it when the copy loop is skipped); the new position is a conditional
 * expression (a copy, a0, where an `if` updates p in place). */
static __inline__ void InputFrame(GENDER *g, unsigned char **in)
{
    float *p = g->last;
    unsigned int i;
    float level;

    if (in[0] != 0) {
        for (i = 1; i <= g->nmodels; i++) {
            if (g->enabled[i])
                memcpy(p, in[i], g->n * 4);
            p += g->n;
        }
        level = *(float *)in[0];
        if (g->fixed == 0 && level > 0.001f)
            GenderDataCompute(g, g->last, level);
        g->count++;
        g->last = p == g->end ? g->start : p;
    }
}

/* the ring fills while no gender is chosen (a frame out only once it is full), drains and
 * refills while one is, then drains what is left. The chosen state tests the input at the call
 * as well (both builds load the write position after that test there, before it in the other
 * two: the inline's own test folds away behind it) -- SOCOM 1's ProcessGender has the same
 * three placements. */
static void ProcessGender(GENDER *g, unsigned char **in)
{
    switch (g->state) {
    case GENDER_FILL:
        if (g->count == g->nframes)
            OutputFrame(g);
        if (g->count < g->nframes)
            InputFrame(g, in);
        if (g->f140 < g->f76)
            g->state = GENDER_CHOSEN;
        break;
    case GENDER_CHOSEN:
        OutputFrame(g);
        if (in[0] != 0)
            InputFrame(g, in);
        if (g->count == 0)
            g->state = GENDER_DRAIN;
        break;
    case GENDER_DRAIN:
        if (g->count < g->nframes)
            InputFrame(g, in);
        OutputFrame(g);
        break;
    }
}

TOSBLOCK *ConstructGender(TOS *tos, unsigned char id)
{
    return tosBaseBlockConstruct(tos, id, 4, 1, (TOSFUNC)ProcessGender, (TOSFUNC)InitGender,
                                 (TOSFUNC)ControlGender, sizeof(GENDER));
}


