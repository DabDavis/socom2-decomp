/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing -funroll-loops */
/* shs: the recogniser's sub-harmonic summation and voicing block (OVL2 0x502F50-0x504980, SOCOM 1's
 * object at 0x36E330: spline2, Lin_2_Log, SubHarmonicSummation_and_VUVDetermination,
 * ProcessSHS_VUV, ControlSHS_VUV, CreateFreqWindow, InitSHS_VUV -- all LOCAL there), ScanSoft's
 * speech recogniser, prebuilt with ee-gcc 2.96 at -O2 -G0 -fno-strict-aliasing -funroll-loops
 * (README "The GCC front"). Names are SOCOM 1's (s1:align, the object's order agreeing); the
 * block's fields are named by their use. Only ControlSHS_VUV is written. */
#include "tosblk.h"

typedef struct SHS {
    TOSBLOCK b;
    char pad40[24];
    void *f64;                          /* +64 */
    void *f68;                          /* +68 */
    void **f72;                         /* +72: five tables */
    void *f76;                          /* +76 */
    void *f80;                          /* +80 */
    void *f84;                          /* +84 */
    void *f88;                          /* +88 */
    void *f92;                          /* +92 */
} SHS;

static int ControlSHS_VUV(SHS *s, unsigned char cmd);

/* 255 frees the block's tables; nothing else is a command here. SOCOM 1's body */
static int ControlSHS_VUV(SHS *s, unsigned char cmd)
{
    TOS *tos;

    switch (cmd) {
    case TOS_DESTROY:
        tos = s->b.tos;
        heap_Free(tos->heap, s->f64);
        heap_Free(tos->heap, s->f68);
        heap_Free(tos->heap, s->f72[0]);
        heap_Free(tos->heap, s->f72[1]);
        heap_Free(tos->heap, s->f72[2]);
        heap_Free(tos->heap, s->f72[3]);
        heap_Free(tos->heap, s->f72[4]);
        heap_Free(tos->heap, s->f72);
        heap_Free(tos->heap, s->f76);
        heap_Free(tos->heap, s->f80);
        heap_Free(tos->heap, s->f84);
        heap_Free(tos->heap, s->f88);
        heap_Free(tos->heap, s->f92);
        tosBaseBlockDestruct(&s->b);
        break;
    default:
        return 0;
    }
    return 1;
}
