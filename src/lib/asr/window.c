/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing -funroll-loops -ffast-math */
/* window: the recogniser's windowing block (OVL2 0x4FC240-0x4FCEA8, SOCOM 1's object at 0x3684C0:
 * WindowFlush, ProcessWindow, InitWindow, ConstructWindow, ControlWindow, all but the constructor
 * LOCAL there), ScanSoft's speech recogniser, prebuilt with ee-gcc 2.96 at -O2 -G0
 * -fno-strict-aliasing -funroll-loops (README "The GCC front") -- and this object with
 * -ffast-math: InitWindow's Hamming loop is unrolled by two, and the copy the unroller peels for
 * i = 0 calls cosf((float)0) with no multiply or divide -- 0 * 2pi / (n - 1) folded to 0, which
 * GCC does only when it need not honour IEEE (without the flag the peeled copy keeps dpmul and
 * dpdiv: 8 words longer); nothing else here changes with it. Names are SOCOM 1's (the process
 * and init rows by the constructor's slots). The input's first differences go into a ring of
 * three frames; each frame out is the Hamming window over the last half-step of the frame
 * before, this frame and the next half-step. Round 31c: WindowFlush (0x4FC240, which ProcessWindow
 * calls when a flush is pending), SOCOM 1's body as it stands -- the object whole (the frame's
 * first and last samples are signed: the flush reads them with lh). */
#include "tosblk.h"

typedef struct WINDOW {
    TOSBLOCK b;
    float *buf;                         /* +40: three frames of differences, a ring */
    float *rd;                          /* +44: the frame being written */
    float *end;                         /* +48 */
    unsigned short len;                 /* +52: the window's length (profile 2) */
    unsigned short step;                /* +54: the frame step (profile 1) */
    char pad56[2];
    unsigned short half;                /* +58: (len - step) / 2 */
    short nbufs;                        /* +60 */
    char pad62[2];
    float prev;                         /* +64: the last input sample, for the difference */
    unsigned char started;              /* +68 */
    unsigned char active;               /* +69 */
    short first;                        /* +70: the frame's first input sample */
    short last;                         /* +72: its last */
    unsigned char flush;                /* +74: a flush asked for before the start */
    char pad75;
    float *coefs;                       /* +76: the Hamming window */
} WINDOW;                               /* 0x50 */

static void WindowFlush(WINDOW *w);
static void ProcessWindow(WINDOW *w, short **in);
static int InitWindow(WINDOW *w);
static int ControlWindow(WINDOW *w, unsigned char cmd);
extern float cosf(float);
extern void *memcpy(void *dst, const void *src, unsigned int n);

/* a flush: one more frame out of what the ring holds -- the frame before the one being written
 * copied into it, its first difference the step from the last frame's last sample back to its
 * first -- windowed out as ProcessWindow does; the block active again. SOCOM 1's body */
static void WindowFlush(WINDOW *w)
{
    float *coef = w->coefs;
    unsigned short m = w->step;
    float *src = w->rd;
    float *out = qEnQueueOne(w->b.outputs[0].q);
    float *q, *e;

    src -= m;
    if (src < w->buf)
        src = w->end - m;
    memcpy(w->rd, src, m * sizeof(float));
    *w->rd = (float)w->first - (float)w->last;
    w->rd += m;
    if (w->rd == w->end)
        w->rd = w->buf;
    q = w->rd + m - w->half;
    for (e = coef + w->half; coef < e; )
        *out++ = *coef++ * *q++;
    if (q == w->end)
        q = w->buf;
    for (e = coef + m; coef < e; )
        *out++ = *coef++ * *q++;
    if (q == w->end)
        q = w->buf;
    for (e = coef + w->half; coef < e; )
        *out++ = *coef++ * *q++;
    w->flush = 0;
    w->active = 1;
}

/* one frame: the input's first differences go into the ring; the first frame after a start is
 * only stored (its first difference zeroed, the frame doubled); then the window over the last
 * half-step of the previous frame, this frame and the next half-step goes out */
static void ProcessWindow(WINDOW *w, short **in)
{
    short *p = *in;

    if (p != 0) {
        unsigned short m = w->step;
        float *coef = w->coefs;
        float *q = w->rd;
        float prev = w->prev;
        float x;
        int i;

        w->first = *p;
        for (i = 0; i < m; i++) {
            x = *p++;
            *q++ = x - prev;
            prev = x;
        }
        w->last = p[-1];
        w->prev = x;
        if (q == w->end)
            q = w->buf;
        if (w->started) {
            float *r = w->rd;

            *r = 0;
            memcpy(q, r, m * sizeof(float));
            q += m;
            if (q == w->end)
                q = w->buf;
            w->rd = q;
            w->started = 0;
            w->active = 0;
        } else {
            float *out = qEnQueueOne(w->b.outputs[0].q);
            float *e;

            w->rd = q;
            q += m - w->half;
            for (e = coef + w->half; coef < e; )
                *out++ = *coef++ * *q++;
            if (q == w->end)
                q = w->buf;
            for (e = coef + m; coef < e; )
                *out++ = *coef++ * *q++;
            if (q == w->end)
                q = w->buf;
            for (e = coef + w->half; coef < e; )
                *out++ = *coef++ * *q++;
        }
    }
    if (w->flush)
        WindowFlush(w);
}

static int InitWindow(WINDOW *w)
{
    TOS *tos = w->b.tos;
    unsigned short m, n;
    int i;

    m = w->step = _tosGetProfileU32(w, 1, 110);
    n = w->len = _tosGetProfileU32(w, 2, 330);
    w->half = (n - m) / 2;
    if (n < m || 3 * m < n)
        return 1;
    w->b.inputs[0].size = 2 * m;
    w->b.outputs[0].size = n * 4;
    w->nbufs = 3;
    w->buf = heap_Calloc(tos->heap, w->step * 3, 4);
    w->coefs = heap_Calloc(tos->heap, n, 4);
    if (w->buf == 0 || w->coefs == 0) {
        _tosErrorLog(w, 2);
        return 1;
    }
    w->rd = w->buf;
    w->end = w->buf + 3 * m;
    for (i = 0; i < n; i++)
        w->coefs[i] = 0.54f - 0.46f * cosf((float)i * 6.283185308 / (float)(n - 1));
    w->flush = 0;
    w->prev = 0;
    w->started = 1;
    w->active = 1;
    return 0;
}

TOSBLOCK *ConstructWindow(TOS *tos, unsigned char id)
{
    return tosBaseBlockConstruct(tos, id, 1, 1, (TOSFUNC)ProcessWindow, (TOSFUNC)InitWindow,
                                 (TOSFUNC)ControlWindow, sizeof(WINDOW));
}

static int ControlWindow(WINDOW *w, unsigned char cmd)
{
    switch (cmd) {
    case 1:
        w->started = 1;
        w->active = 1;
        w->prev = 0;
        break;
    case 2:
        if (w->active == 0)
            w->flush = 1;
        break;
    case TOS_DESTROY:
        heap_Free(w->b.tos->heap, w->buf);
        heap_Free(w->b.tos->heap, w->coefs);
        tosBaseBlockDestruct(&w->b);
        return 1;
    default:
        return 0;
    }
    return 1;
}
