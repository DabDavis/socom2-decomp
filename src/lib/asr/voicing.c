/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing -funroll-loops */
/* voicing: the trigger's voicing measure (OVL2 0x506D98-0x508258, SOCOM 1's voicing object at
 * 0x371BD8), ScanSoft's recogniser, prebuilt with ee-gcc 2.96 at -O2 -G0 -fno-strict-aliasing
 * -funroll-loops (README "The GCC front"). SOCOM 1's six functions in another order here:
 * Voicing_GetMaxVoicing, InitVoicing, Voicing_MaintainNoiseEner, Voicing_AddSignal (named by
 * their one caller each, as SOCOM 1's), Voicing_Reset, Voicing_Free. A voicing record begins
 * with the subsampler's (Subsampler_Reset and Subsampler_Free take it); SOCOM II's fields sit 4
 * bytes above SOCOM 1's. The functions not yet written are missing; the rest is in image order. */

#include "tosblk.h"

/* the subsampler's record, as far as the voicing code reads it */
typedef struct SUBSAMPLER {
    TOS *tos;                           /* +0 */
    char pad4[288];
} SUBSAMPLER;

typedef struct VOICING {
    SUBSAMPLER ss;                      /* +0 */
    short *nbuf;                        /* +292: a ring of shorts */
    short *npos;                        /* +296 */
    short *nend;                        /* +300 */
    int *ebuf;                          /* +304: a ring of ints */
    int *epos;                          /* +308 */
    int *eend;                          /* +312 */
    void *hpos;                         /* +316 */
    void *hbuf;                         /* +320 */
    int f324;
    int nframes;                        /* +328 */
} VOICING;

extern void Subsampler_Reset(VOICING *v);
extern void Subsampler_Free(VOICING *v);

void Voicing_Reset(VOICING *v)
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

void Voicing_Free(VOICING *v)
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
