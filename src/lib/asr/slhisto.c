/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing -funroll-loops */
/* The recogniser's sliding histogram (OVL2 0x5049C0-0x504FB0, SOCOM 1's object at 0x36FB28:
 * SlidingHisto_NewItem, _Init, _Free, _LowerQuantile, _Clear -- SOCOM II defines _Free after
 * _Clear and adds three session functions after it), ScanSoft's speech recogniser, prebuilt with
 * ee-gcc 2.96 at -O2 -G0 -fno-strict-aliasing -funroll-loops (README "The GCC front"). Names are
 * SOCOM 1's (s1:align; SlidingHisto_Free by body: the bins and the history freed, as SOCOM 1's);
 * the session functions are new in SOCOM II and keep placeholders. The histogram lives in its
 * owner's block (from +0xE8; the block's head gives the scheduler for the heap and the
 * profile): `nbins` bins over [min, min + range] (profiles 12, 13, 14) count the last `histlen`
 * values (profile 11), each remembered by its bin in a ring so it can leave again. A session is
 * the ring's position (a long) and the ring. */
#include "tosblk.h"

typedef struct HISTOHOST {
    TOS *tos;                           /* +0: the owner block's head */
    char pad4[228];
    float min;                          /* +232 */
    float range;                        /* +236 */
    int nbins;                          /* +240 */
    int lastbin;                        /* +244 */
    float width;                        /* +248 */
    int *bins;                          /* +252 */
    unsigned int count;                 /* +256 */
    int histlen;                        /* +260 */
    int *hist;                          /* +264: each value's bin, -1 when empty */
    int *hend;                          /* +268 */
    int *pos;                           /* +272 */
} HISTOHOST;

typedef struct HISTOSESSION {
    long pos;
    int hist[1];
} HISTOSESSION;

float floorf(float x);
float ceilf(float x);

void SlidingHisto_NewItem(HISTOHOST *h, float x)
{
    int bin;
    int old;

    if (x > h->min + h->range)
        bin = h->lastbin;
    else if (x < h->min)
        bin = 0;
    else
        bin = floorf((x - h->min + h->width * 0.5f) / h->width);
    h->bins[bin]++;
    h->count++;
    old = *h->pos;
    *h->pos = bin;
    h->pos++;
    if (h->pos == h->hend)
        h->pos = h->hist;
    if (old >= 0) {
        h->bins[old]--;
        h->count--;
    }
}

int SlidingHisto_Init(HISTOHOST *h)
{
    TOS *tos = h->tos;
    int err = 0;
    float min, max;
    int *p, *q;

    min = _tosGetProfileFloat(h, 12, 6.5f);
    max = _tosGetProfileFloat(h, 13, 27.0f);
    h->min = min;
    h->range = max - min;
    h->nbins = _tosGetProfileU32(h, 14, 150);
    h->bins = heap_Calloc(tos->heap, h->nbins, sizeof(int));
    h->lastbin = h->nbins - 1;
    h->width = h->range / h->lastbin;
    h->histlen = _tosGetProfileU32(h, 11, 20);
    h->hist = heap_Calloc(tos->heap, h->histlen, sizeof(int));
    h->pos = h->hist;
    h->hend = h->hist + h->histlen;
    if (h->bins == 0 || h->hist == 0) {
        _tosErrorLog(h, 2);
        err = 1;
    } else {
        p = h->bins + h->nbins;
        while (h->bins < p)
            *--p = 0;
        h->count = 0;
        for (q = h->hist; q < h->hend; q++)
            *q = -1;
    }
    return err;
}

float SlidingHisto_LowerQuantile(HISTOHOST *h, float q)
{
    unsigned int sum = 0;
    int *p = h->bins;
    unsigned int n;

    n = ceilf(q * h->count + 0.001);
    do
        sum += *p++;
    while (sum < n);
    return (p - h->bins - 1) * h->width + h->min;
}

void SlidingHisto_Clear(HISTOHOST *h)
{
    int *p, *q;

    p = h->bins + h->nbins;
    while (h->bins < p)
        *--p = 0;
    h->count = 0;
    for (q = h->hist; q < h->hend; q++)
        *q = -1;
}

void SlidingHisto_Free(HISTOHOST *h)
{
    TOS *tos = h->tos;

    if (h->bins != 0)
        heap_Free(tos->heap, h->bins);
    if (h->hist != 0)
        heap_Free(tos->heap, h->hist);
}

/* the session's size */
int func_00504E48(HISTOHOST *h)
{
    return h->histlen * sizeof(int) + 8;
}

/* a session restored: the ring copied back and the bins counted again */
void func_00504E58(HISTOHOST *h, HISTOSESSION *s)
{
    int *p, *q, *r;

    p = h->bins + h->nbins;
    while (h->bins < p)
        *--p = 0;
    h->count = 0;
    for (q = h->hist; q < h->hend; q++)
        *q = -1;
    h->pos = h->hist + s->pos;
    memcpy(h->hist, s->hist, h->histlen * sizeof(int));
    for (r = h->hist; r < h->hend; r++)
        if (*r >= 0) {
            h->bins[*r]++;
            h->count++;
        }
}

/* a session saved */
void func_00504F70(HISTOHOST *h, HISTOSESSION *s)
{
    s->pos = h->pos - h->hist;
    memcpy(s->hist, h->hist, h->histlen * sizeof(int));
}
