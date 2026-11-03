/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing -funroll-loops */
/* voicing: the trigger's voicing measure (OVL2 0x506D98-0x508258, SOCOM 1's voicing object at
 * 0x371BD8), ScanSoft's recogniser, prebuilt with ee-gcc 2.96 at -O2 -G0 -fno-strict-aliasing
 * -funroll-loops (README "The GCC front"). SOCOM 1's six functions in another order here:
 * Voicing_GetMaxVoicing (not yet written), InitVoicing, Voicing_MaintainNoiseEner,
 * Voicing_AddSignal (named by their one caller each, as SOCOM 1's), Voicing_Reset, Voicing_Free.
 * The voicing fields share the trigger block's record with the subsampler's (subsmp.c: the
 * subsampler's output frame is this record's `frame`, its fields from +344); SOCOM II's sit 4
 * bytes above SOCOM 1's. Voicing_Reset and Voicing_Free are public `__inline__` functions defined
 * first: InitVoicing has Reset's body inlined, and GCC 2.96 defers an inlinable function's own
 * copy to the end of the object in declaration order -- the image's Reset, then Free, after
 * Voicing_AddSignal (subsmp.c's Reset and Free, the same). */
#include "tosblk.h"

/* the subsampler's record, as far as the voicing code reads it */
typedef struct SUBSAMPLER {
    TOS *tos;                           /* +0 */
} SUBSAMPLER;

typedef struct VOICING {
    SUBSAMPLER ss;                      /* +0 */
    char pad4[272];
    unsigned int minlag;                /* +276: 5 ms at half the rate (profile 24) */
    unsigned int maxlag;                /* +280: 14 ms (profile 25) */
    unsigned int frame;                 /* +284: 10 ms (profile 26), the subsampler's output */
    unsigned int len;                   /* +288: maxlag + frame */
    short *nbuf;                        /* +292: a ring of shorts */
    short *npos;                        /* +296 */
    short *nend;                        /* +300 */
    unsigned int *ebuf;                 /* +304: the samples' energies, a ring beside nbuf */
    unsigned int *epos;                 /* +308 */
    unsigned int *eend;                 /* +312 */
    float *hpos;                        /* +316: the frames' log energies, a ring */
    float *hbuf;                        /* +320 */
    float *hend;                        /* +324 */
    unsigned int nframes;               /* +328: the history's frames, up to hsize */
    unsigned int hsize;                 /* +332 */
    float thresh;                       /* +336: profile 28, dB as a natural log */
    float floor;                        /* +340: profile 29 */
    int ssnin;                          /* +344: the subsampler's input frame (subsmp.c) */
    short *ssout;                       /* +348: and its output frame, `frame` samples */
} VOICING;

extern void Subsampler_Reset(VOICING *v);
extern void Subsampler_Free(VOICING *v);

__inline__ void Voicing_Reset(VOICING *v)
{
    for (v->npos = v->nbuf; v->npos < v->nend; v->npos++)
        *v->npos = 0;
    v->npos = v->nbuf;
    for (v->epos = v->ebuf; v->epos < v->eend; v->epos++)
        *v->epos = 0;
    v->epos = v->ebuf;
    v->hpos = v->hbuf;
    v->nframes = 0;
    Subsampler_Reset(v);
}

__inline__ void Voicing_Free(VOICING *v)
{
    TOS *tos = v->ss.tos;

    if (v->nbuf != 0)
        heap_Free(tos->heap, v->nbuf);
    if (v->ebuf != 0)
        heap_Free(tos->heap, v->ebuf);
    if (v->hbuf != 0)
        heap_Free(tos->heap, v->hbuf);
    Subsampler_Free(v);
}

extern int Subsampler_Init(VOICING *v);
extern float floorf(float);
extern float ceilf(float);

int InitVoicing(VOICING *v)
{
    TOS *tos;
    float minlag, maxlag, frame, rate, floor;

    v->minlag = v->maxlag = v->frame = 0;
    v->nbuf = v->npos = v->nend = 0;
    v->ebuf = v->epos = v->eend = 0;
    v->hpos = v->hbuf = v->hend = 0;
    v->nframes = v->hsize = 0;
    tos = v->ss.tos;
    minlag = _tosGetProfileFloat(v, 24, 0.005f);
    maxlag = _tosGetProfileFloat(v, 25, 0.014f);
    frame = _tosGetProfileFloat(v, 26, 0.01f);
    rate = _tosGetProfileFloat(v, 27, 11000.0f);
    floor = _tosGetProfileFloat(v, 29, -1000.0f);
    v->thresh = _tosGetProfileFloat(v, 28, 6.0f) * 0.2302585f;
    v->floor = floor;
    v->minlag = floorf(minlag * rate * 0.5f);
    v->maxlag = ceilf(maxlag * rate * 0.5f);
    v->frame = ceilf(frame * rate * 0.5f);
    v->len = v->maxlag + v->frame;
    v->nbuf = heap_Calloc(tos->heap, v->len, 2);
    v->npos = v->nbuf;
    v->nend = v->nbuf + v->len;
    v->ebuf = heap_Calloc(tos->heap, v->len, 4);
    v->epos = v->ebuf;
    v->eend = v->ebuf + v->len;
    v->hsize = 16;
    v->hbuf = heap_Calloc(tos->heap, 16, 4);
    v->hend = v->hbuf + v->hsize;
    if (v->nbuf == 0 || v->ebuf == 0 || v->hbuf == 0) {
        _tosErrorLog(v, 2);
        return 1;
    }
    Voicing_Reset(v);
    return Subsampler_Init(v) != 0;
}

extern float logf(float);

/* the last frame's energy, summed backwards through the ring, into the history as a log */
void Voicing_MaintainNoiseEner(VOICING *v)
{
    float sum = 0;
    unsigned int *q = v->epos - 1;
    unsigned int *buf = v->ebuf;
    unsigned int i;

    if (q < buf)
        q += v->len;
    for (i = 0; i < v->frame; i++) {
        sum += *q--;
        if (q < buf)
            q += v->len;
    }
    *v->hpos++ = logf(sum);
    if (v->hpos >= v->hend)
        v->hpos = v->hbuf;
    if (v->nframes < v->hsize)
        v->nframes++;
}

extern int Subsampler_Process(VOICING *v, short *in);

/* one input frame: subsampled, then each sample and its energy into the two rings */
int Voicing_AddSignal(VOICING *v, short *in)
{
    int err = 0;
    short *p = v->ssout;
    short *end = p + v->frame;

    if (Subsampler_Process(v, in) != 0) {
        err = 1;
    } else {
        for (; p < end; p++) {
            short x = *p;

            *v->npos++ = x;
            *v->epos++ = x * x;
            if (v->npos >= v->nend) {
                v->npos = v->nbuf;
                v->epos = v->ebuf;
            }
        }
    }
    return err;
}
