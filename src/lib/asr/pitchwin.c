/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing -funroll-loops */
/* pitchwin: the recogniser's pitch window block (OVL2 0x4F2900-0x4F2F90, SOCOM 1's object at
 * 0x35F828: CreateWindow, InitPitchWindow, ConstructPitchWindow, ProcessPitchWindow,
 * ControlPitchWindow, all but the constructor LOCAL there), ScanSoft's speech recogniser, prebuilt
 * with ee-gcc 2.96 at -O2 -G0 -fno-strict-aliasing -funroll-loops (README "The GCC front"). Names
 * are SOCOM 1's (the object's order and the constructor's slots). The block keeps a ring of the
 * last `len` samples (int) and, for every input frame of `step` samples, writes the ring's last
 * window through a Hamming window (float) to its output queue. */
#include "tosblk.h"

typedef struct PITCHWIN {
    TOSBLOCK b;
    float *win;                         /* +40: the Hamming window, len taps */
    int *buf;                           /* +44: the sample ring, len samples */
    int *rd;                            /* +48: the next window's first frame */
    int *wr;                            /* +52: where the next input frame goes */
    int *end;                           /* +56: buf + len */
    unsigned short nframes;             /* +60: the input frames a window spans */
    unsigned short step;                /* +62: the samples an input frame brings */
    unsigned short len;                 /* +64: the window's samples */
    char pad66[2];
} PITCHWIN;                             /* 0x44 */

#define PI 3.141592653589793

extern float cosf(float x);

static void CreateWindow(float *w, unsigned short rate, unsigned short len);
static int InitPitchWindow(PITCHWIN *w);
static void ProcessPitchWindow(PITCHWIN *w, int **in);
static int ControlPitchWindow(PITCHWIN *w, unsigned char cmd);

/* a Hamming window of len taps at rate samples a second, 1 at its centre */
static void CreateWindow(float *w, unsigned short rate, unsigned short len)
{
    float omega = 2 * PI / (len / (float)rate);
    float step = 1.0f / rate;
    int half = len >> 1;
    float t;
    int i;

    w[half] = 1.0f;
    t = (half - 1) * step;
    for (i = 1; i < half; i++) {
        w[half + i] = w[half - i] = 0.54 - 0.46 * cosf(omega * t);
        t -= step;
    }
    w[0] = 0.54 - 0.46 * cosf(omega * t);
}

static int InitPitchWindow(PITCHWIN *w)
{
    TOS *tos = w->b.tos;
    unsigned short rate, frame, span, len;
    float secs;
    unsigned int n;

    rate = _tosGetProfileU32(w, 1, 2200);
    secs = _tosGetProfileFloat(w, 2, 0.04f);
    frame = _tosGetProfileU32(w, 3, 110);
    span = _tosGetProfileU32(w, 4, 11000);
    w->len = rate * secs + 0.5;
    len = w->len;
    n = secs * span + 0.5;
    if (n % frame != 0) {
        _tosErrorLog(w, 100);
        return 1;
    }
    w->nframes = n / frame;
    w->step = len / w->nframes;
    w->b.inputs[0].size = w->step * 4;
    w->b.outputs[0].size = len * 4;
    w->win = heap_Calloc(tos->heap, len, 4);
    w->buf = heap_Calloc(tos->heap, len, 4);
    if (w->win == 0 || w->buf == 0) {
        _tosErrorLog(w, 2);
        return 1;
    }
    CreateWindow(w->win, rate, len);
    w->rd = w->buf;
    w->end = w->buf + w->len;
    w->wr = w->buf + (w->nframes - 1) * w->step;
    return 0;
}

TOSBLOCK *ConstructPitchWindow(TOS *tos, unsigned char id)
{
    return tosBaseBlockConstruct(tos, id, 1, 1, (TOSFUNC)ProcessPitchWindow,
                                 (TOSFUNC)InitPitchWindow, (TOSFUNC)ControlPitchWindow,
                                 sizeof(PITCHWIN));
}

/* one input frame into the ring, then the ring's last window, windowed, out */
static void ProcessPitchWindow(PITCHWIN *w, int **in)
{
    float *win;
    float *out;
    unsigned short i, j;

    if (*in != 0) {
        memcpy(w->wr, *in, w->step * 4);
        w->wr += w->step;
        if (w->wr == w->end)
            w->wr = w->buf;
        win = w->win;
        out = qEnQueueOne(w->b.outputs[0].q);
        for (i = 0; i < w->nframes; i++) {
            for (j = 0; j < w->step; j++)
                *out++ = *w->rd++ * *win++;
            if (w->rd == w->end)
                w->rd = w->buf;
        }
        w->rd += w->step;
        if (w->rd == w->end)
            w->rd = w->buf;
    }
}

/* 200 clears the ring; 255 destroys the block */
static int ControlPitchWindow(PITCHWIN *w, unsigned char cmd)
{
    unsigned short i;

    switch (cmd) {
    case 200:
        w->wr = w->buf;
        for (i = 0; i < w->len; i++)
            *w->wr++ = 0;
        w->rd = w->buf;
        w->end = w->buf + w->len;
        w->wr = w->buf + (w->nframes - 1) * w->step;
        break;
    case 255:
        heap_Free(w->b.tos->heap, w->win);
        heap_Free(w->b.tos->heap, w->buf);
        tosBaseBlockDestruct(&w->b);
        break;
    default:
        return 0;
    }
    return 1;
}
