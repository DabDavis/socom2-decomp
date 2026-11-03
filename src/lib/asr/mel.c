/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing -funroll-loops */
/* mel: the recogniser's mel filter-bank block (OVL2 0x4F6110-0x4F6CE0, SOCOM 1's object at 0x362D10:
 * MelInitBands, ConstructMel, ProcessMel, ControlMel, InitMel -- all but the constructor LOCAL
 * there), ScanSoft's speech recogniser, prebuilt with ee-gcc 2.96 at -O2 -G0 -fno-strict-aliasing
 * -funroll-loops (README "The GCC front"). Names are SOCOM 1's. Command 1 rebuilds the bands for
 * the sample rate its fourth argument carries as a float's bits; 255 destroys. MelInitBands
 * (0x808) and ProcessMel (0x4F6958) are not written; ConstructMel and InitMel stand as their
 * drafts. */
#include "tosblk.h"

typedef struct MEL {
    TOSBLOCK b;
    unsigned int nbands;                /* +40 */
    float **bands;                      /* +44: each band's weights */
    void *f48;                          /* +48 */
    void *f52;                          /* +52 */
    int pad56;
} MEL;                                  /* 60 (ConstructMel's) */

int MelInitBands(MEL *m, float rate);

/* the bands' tables freed (no name of the demo's: inlined in both commands) */
static __inline__ void FreeMelBands(MEL *m)
{
    TOS *tos = m->b.tos;
    unsigned int i;

    for (i = 0; i < m->nbands; i++)
        heap_Free(tos->heap, m->bands[i]);
    heap_Free(tos->heap, m->bands);
    heap_Free(tos->heap, m->f48);
    heap_Free(tos->heap, m->f52);
    m->bands = 0;
}

/* command 1's rate is the fourth argument's bits read as a float through a pointer: the pointer
 * keeps arg2 in its stack slot, so the float goes from there straight into $f12 (`*(float *)&arg2`
 * in the call, a union or a float local: the subreg 2.96's ADDRESSOF leaves goes through $f0) */
int ControlMel(MEL *m, unsigned char cmd, void *arg, int arg2)
{
    float *rate = (float *)&arg2;
    int ret = 1;

    switch (cmd) {
    case 1:
        if (m->bands != 0)
            FreeMelBands(m);
        ret = MelInitBands(m, *rate) == 0;
        break;
    case 255:
        if (m->bands != 0)
            FreeMelBands(m);
        tosBaseBlockDestruct(&m->b);
        break;
    default:
        ret = 0;
    }
    return ret;
}
