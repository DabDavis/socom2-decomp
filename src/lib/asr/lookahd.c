/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing -funroll-loops */
/* The recogniser's look-ahead (OVL2 0x501418-0x501690, SOCOM 1's object at 0x36C840: the same
 * four functions in the same order, all but the constructor static there), ScanSoft's speech
 * recogniser, prebuilt with ee-gcc 2.96 at -O2 -G0 -fno-strict-aliasing -funroll-loops (README
 * "The GCC front"). Names are SOCOM 1's (s1:align). A running sum of the last `len` input values
 * (a ring of floats) is compared with a threshold, and the flag for each frame goes out
 * `lookahead` frames late; a flush emits the flags still owed. */
#include "tosblk.h"

typedef struct LOOKAHEAD {
    TOSBLOCK b;
    unsigned char flushing;             /* +40 */
    int lookahead;                      /* +44: profile 1 */
    int len;                            /* +48: profile 2 */
    float threshold;                    /* +52: profile 3 */
    float *buf;                         /* +56 */
    float *wp;                          /* +60 */
    float *end;                         /* +64 */
    float sum;                          /* +68 */
    int count;                          /* +72 */
} LOOKAHEAD;

static void ProcessLookAhead(LOOKAHEAD *l, float **in);
static int ControlLookAhead(LOOKAHEAD *l, unsigned char cmd);
static int InitLookAhead(LOOKAHEAD *l);

TOSBLOCK *ConstructLookAhead(TOS *tos, unsigned char id)
{
    return tosBaseBlockConstruct(tos, id, 1, 1, (TOSFUNC)ProcessLookAhead, (TOSFUNC)InitLookAhead,
                                 (TOSFUNC)ControlLookAhead, sizeof(LOOKAHEAD));
}

static void ProcessLookAhead(LOOKAHEAD *l, float **in)
{
    float *frame = in[0];
    unsigned char *flag;

    if (frame != 0) {
        l->sum -= *l->wp;
        *l->wp++ = *frame;
        l->sum += *frame;
        if (l->wp == l->end)
            l->wp = l->buf;
        if (l->count == l->lookahead) {
            flag = qEnQueueOne(l->b.outputs[0].q);
            *flag = l->sum > l->threshold;
        } else
            l->count++;
    } else if (l->flushing) {
        while (l->count != 0) {
            flag = qEnQueueOne(l->b.outputs[0].q);
            *flag = l->sum > l->threshold;
            l->count--;
        }
        l->flushing = 0;
    }
}

static int ControlLookAhead(LOOKAHEAD *l, unsigned char cmd)
{
    switch (cmd) {
    case 1:
        l->flushing = 1;
        break;
    case TOS_DESTROY:
        heap_Free(l->b.tos->heap, l->buf);
        tosBaseBlockDestruct(&l->b);
        break;
    default:
        return 0;
    }
    return 1;
}

static int InitLookAhead(LOOKAHEAD *l)
{
    TOS *tos = l->b.tos;

    l->lookahead = (unsigned short)_tosGetProfileU32(l, 1, 0);
    l->len = (unsigned short)_tosGetProfileU32(l, 2, 15);
    l->threshold = _tosGetProfileFloat(l, 3, 1.0f);
    l->b.inputs[0].size = sizeof(float);
    l->b.outputs[0].size = 1;
    l->buf = heap_Calloc(tos->heap, l->len, sizeof(float));
    l->wp = l->buf;
    l->end = l->buf + l->len;
    l->sum = 0;
    l->count = 0;
    return 0;
}
