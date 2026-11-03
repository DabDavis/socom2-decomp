/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing -funroll-loops */
/* The recogniser's stationarity test (OVL2 0x504FB0-0x505480, SOCOM 1's object at 0x36FF88:
 * ProcessStationarity, InitStationarity, ConstructStationarity, ControlStationarity, the same
 * order; all but the constructor static there), ScanSoft's speech recogniser, prebuilt with
 * ee-gcc 2.96 at -O2 -G0 -fno-strict-aliasing -funroll-loops (README "The GCC front"). Names are
 * SOCOM 1's (s1:align; ProcessStationarity by the constructor's slot and position). The first
 * input's values fill a window of 2 * half + 1 (profile 1); the second input is delayed by half
 * frames and goes out as it is while the window's spread stays under the threshold (profile 2),
 * a tenth of it otherwise. A flush (command 1) drains the delay. */
#include "tosblk.h"

typedef struct STATIONARITY {
    TOSBLOCK b;
    int flushing;                       /* +40 */
    unsigned int count;                 /* +44 */
    unsigned int half;                  /* +48 */
    float threshold;                    /* +52 */
    float *buf;                         /* +56: the window */
    float *end;                         /* +60 */
    float *wp;                          /* +64 */
    float *dbuf;                        /* +68: the delay line */
    float *dend;                        /* +72 */
    float *dp;                          /* +76 */
} STATIONARITY;

static void ProcessStationarity(STATIONARITY *s, float **in);
static int InitStationarity(STATIONARITY *s);
static int ControlStationarity(STATIONARITY *s, unsigned char cmd);

/* OPEN (50 words, the right size): the registers of the first block -- the target keeps the
 * input pointer in v1 and the count in a2, the fill loop's pointer in v1 beside the buffer in a0
 * (here a0, v1 and a copy in a1), and so moves s into s0 in the null test's delay slot. The
 * window's bound as a local and the extremes from *p are the target's (without them the size
 * misses); a frame local, the input read into p, every order of the integer locals and four
 * spellings of the extremes' start leave it at 50. */
static void ProcessStationarity(STATIONARITY *s, float **in)
{
    float v = 0;
    float x, delayed, mn, mx;
    float *p, *out, *end;
    int stationary;

    if (in[0] != 0) {
        x = *in[0];
        v = *in[1];
        if (s->count == 0) {
            for (p = s->buf; p < s->end; p++)
                *p = x;
            s->wp = s->buf;
        } else {
            *s->wp++ = x;
            if (s->wp == s->end)
                s->wp = s->buf;
        }
        s->count++;
    } else if (s->flushing == 0)
        return;
    delayed = *s->dp;
    *s->dp++ = v;
    if (s->dp == s->dend)
        s->dp = s->dbuf;
    if (s->flushing && s->count == 0)
        s->flushing = 0;
    if (s->count > s->half || s->flushing) {
        out = qEnQueueOne(s->b.outputs[0].q);
        s->count--;
        if (delayed > 0) {
            p = s->buf;
            end = s->end;
            mx = mn = *p;
            for (p++; p < end; p++) {
                if (*p < mn)
                    mn = *p;
                if (mx < *p)
                    mx = *p;
            }
            stationary = mx - mn < s->threshold;
            if (!stationary)
                delayed *= 0.1f;
        }
        *out = delayed;
    }
}

static int InitStationarity(STATIONARITY *s)
{
    TOS *tos = s->b.tos;
    unsigned int i;

    s->b.inputs[0].size = 24;
    s->b.inputs[1].size = sizeof(float);
    s->b.outputs[0].size = sizeof(float);
    s->half = (unsigned char)_tosGetProfileU32(s, 1, 2);
    s->threshold = _tosGetProfileFloat(s, 2, 2.0f);
    s->buf = heap_Calloc(tos->heap, s->half * 2 + 1, sizeof(float));
    s->dbuf = heap_Calloc(tos->heap, s->half, sizeof(float));
    if (s->buf == 0 || s->dbuf == 0)
        return _tosErrorLog(s, 2);
    for (i = 0; i < s->half * 2 + 1; i++)
        s->buf[i] = 0;
    s->end = s->buf + s->half * 2 + 1;
    s->wp = s->buf;
    s->dend = s->dbuf + s->half;
    s->dp = s->dbuf;
    return 0;
}

TOSBLOCK *ConstructStationarity(TOS *tos, unsigned char id)
{
    return tosBaseBlockConstruct(tos, id, 2, 1, (TOSFUNC)ProcessStationarity,
                                 (TOSFUNC)InitStationarity, (TOSFUNC)ControlStationarity,
                                 sizeof(STATIONARITY));
}

static int ControlStationarity(STATIONARITY *s, unsigned char cmd)
{
    TOS *tos = s->b.tos;

    switch (cmd) {
    case 1:
        s->flushing = 1;
        break;
    case TOS_DESTROY:
        heap_Free(tos->heap, s->buf);
        heap_Free(tos->heap, s->dbuf);
        tosBaseBlockDestruct(&s->b);
        break;
    default:
        return 0;
    }
    return 1;
}
