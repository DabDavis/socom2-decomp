/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing -funroll-loops */
/* The recogniser's delay line (OVL2 0x4E42A8-0x4E4628, SOCOM 1's object at 0x352408: the same
 * four functions in the same order), ScanSoft's speech recogniser, prebuilt with ee-gcc 2.96 at
 * -O2 -G0 -fno-strict-aliasing -funroll-loops (README "The GCC front"). Names are SOCOM 1's
 * (s1:align). The block holds `delay` frames in a ring and passes each frame on that many frames
 * late; a flush drains what it holds. */
#include "tosblk.h"

typedef struct DELAYBLOCK {
    TOSBLOCK b;
    unsigned int size;                  /* +40: delay * framesize */
    unsigned short delay;               /* +44: frames (profile 1) */
    unsigned short framesize;           /* +46: bytes (profile 2) */
    unsigned char *buf;                 /* +48 */
    unsigned char *rp;                  /* +52 */
    unsigned char *wp;                  /* +56 */
    unsigned char *end;                 /* +60 */
    unsigned char filling;              /* +64 */
    unsigned short count;               /* +66: frames held while filling */
    unsigned char flushing;             /* +68 */
    unsigned short flushcount;          /* +70 */
} DELAYBLOCK;

static void ProcessDelayBlock(DELAYBLOCK *d, unsigned char **in);
static int ControlDelayBlock(DELAYBLOCK *d, unsigned char cmd);
static int InitDelayBlock(DELAYBLOCK *d);

static void ProcessDelayBlock(DELAYBLOCK *d, unsigned char **in)
{
    unsigned char *frame = in[0];

    if (frame != 0) {
        if (d->filling == 1) {
            memcpy(d->wp, frame, d->framesize);
            d->wp += d->framesize;
            if (d->wp == d->end)
                d->wp = d->buf;
            if (++d->count == d->delay)
                d->filling = 0;
        } else {
            memcpy(qEnQueueOne(d->b.outputs[0].q), d->rp, d->framesize);
            d->rp += d->framesize;
            if (d->rp == d->end)
                d->rp = d->buf;
            memcpy(d->wp, frame, d->framesize);
            d->wp += d->framesize;
            if (d->wp == d->end)
                d->wp = d->buf;
        }
    } else if (d->flushing == 1) {
        if (d->flushcount == 0)
            d->flushing = 0;
        else {
            memcpy(qEnQueueOne(d->b.outputs[0].q), d->rp, d->framesize);
            d->rp += d->framesize;
            if (d->rp == d->end)
                d->rp = d->buf;
            d->flushcount--;
        }
    }
}

TOSBLOCK *ConstructDelayBlock(TOS *tos, unsigned char id)
{
    return tosBaseBlockConstruct(tos, id, 1, 1, (TOSFUNC)ProcessDelayBlock, (TOSFUNC)InitDelayBlock,
                                 (TOSFUNC)ControlDelayBlock, sizeof(DELAYBLOCK));
}

static int ControlDelayBlock(DELAYBLOCK *d, unsigned char cmd)
{
    unsigned int i;

    switch (cmd) {
    case TOS_FLUSH:
        d->flushing = 1;
        d->flushcount = d->count;
        break;
    case TOS_RESET:
        d->wp = d->buf;
        for (i = 0; i < d->size; i++)
            *d->wp = 0;
        d->filling = 1;
        d->flushcount = 0;
        d->rp = d->buf;
        d->count = 0;
        d->flushing = 0;
        d->wp = d->buf;
        break;
    case TOS_DESTROY:
        heap_Free(d->b.tos->heap, d->buf);
        tosBaseBlockDestruct(&d->b);
        break;
    default:
        return 0;
    }
    return 1;
}

static int InitDelayBlock(DELAYBLOCK *d)
{
    TOS *tos = d->b.tos;

    d->delay = _tosGetProfileU32(d, 1, 1);
    d->framesize = _tosGetProfileU32(d, 2, 4);
    d->size = d->delay * d->framesize;
    d->buf = heap_Calloc(tos->heap, d->delay, d->framesize);
    if (d->buf == 0) {
        _tosErrorLog(d, 2);
        return 1;
    }
    d->end = d->buf + d->size;
    d->wp = d->buf;
    d->rp = d->buf;
    d->b.inputs[0].size = d->framesize;
    d->b.outputs[0].size = d->framesize;
    d->filling = 1;
    d->count = 0;
    d->flushing = 0;
    d->flushcount = 0;
    return 0;
}
