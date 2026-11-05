/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing -funroll-loops -ffast-math */
/* mel: the recogniser's mel filter-bank block (OVL2 0x4F6110-0x4F6CE0, SOCOM 1's object at 0x362D10:
 * MelInitBands, ConstructMel, ProcessMel, ControlMel, InitMel -- all but the constructor LOCAL
 * there), ScanSoft's speech recogniser, prebuilt with ee-gcc 2.96 at -O2 -G0 -fno-strict-aliasing
 * -funroll-loops -ffast-math (README "The GCC front": MelInitBands' min.s is -ffast-math's, round
 * 25c; every function written here compiles the same with and without it, round 26c, so the
 * directive carries the object's flag). Names are SOCOM 1's. Command 1 rebuilds the bands for
 * the sample rate its fourth argument carries as a float's bits; 255 destroys. A band is a run of
 * weights over the spectrum: its weights (+44), its first bin (+48) and its count (+52), one entry
 * per band. In image order; MelInitBands (0x808, its min.s -ffast-math's: README round 25c) is not
 * written. Round 26c wrote ConstructMel and InitMel (over their drafts) and ProcessMel. */
#include "tosblk.h"

typedef struct MEL {
    TOSBLOCK b;
    unsigned int nbands;                /* +40 */
    float **bands;                      /* +44: each band's weights */
    unsigned int *first;                /* +48: each band's first bin */
    unsigned int *count;                /* +52: each band's weights */
    float rate;                         /* +56: the sample rate (MelInitBands') */
} MEL;                                  /* 60 (ConstructMel's) */

int MelInitBands(MEL *m, float rate);
static void ProcessMel(MEL *m, void **in);
static int ControlMel(MEL *m, unsigned char cmd, void *arg, int arg2);
static int InitMel(MEL *m);

TOSBLOCK *ConstructMel(TOS *tos, unsigned char id)
{
    return tosBaseBlockConstruct(tos, id, 1, 1, (TOSFUNC)ProcessMel, (TOSFUNC)InitMel,
                                 (TOSFUNC)ControlMel, sizeof(MEL));
}

/* one frame of the spectrum: each band's weighted sum of its bins, from 10 (SOCOM 1's body; the
 * sum kept in the output frame, which the weights may alias) */
static void ProcessMel(MEL *m, void **in)
{
    float *x = in[0];
    float *out, *w, *end, *s;
    unsigned int i;

    if (x != 0) {
        out = qEnQueueOne(m->b.outputs[0].q);
        for (i = 0; i < m->nbands; i++) {
            *out = 10.0f;
            w = m->bands[i];
            end = w + m->count[i];
            s = x + m->first[i];
            for (; w < end; w++, s++)
                *out += *w * *s;
            out++;
        }
    }
}

/* the bands' tables freed (no name of the demo's: inlined in both commands) */
static __inline__ void FreeMelBands(MEL *m)
{
    TOS *tos = m->b.tos;
    unsigned int i;

    for (i = 0; i < m->nbands; i++)
        heap_Free(tos->heap, m->bands[i]);
    heap_Free(tos->heap, m->bands);
    heap_Free(tos->heap, m->first);
    heap_Free(tos->heap, m->count);
    m->bands = 0;
}

/* command 1's rate is the fourth argument's bits read as a float through a pointer: the pointer
 * keeps arg2 in its stack slot, so the float goes from there straight into $f12 (`*(float *)&arg2`
 * in the call, a union or a float local: the subreg 2.96's ADDRESSOF leaves goes through $f0) */
static int ControlMel(MEL *m, unsigned char cmd, void *arg, int arg2)
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

/* the queues' element sizes: the spectrum's bins (profile 3) and the bands (profile 4) */
static int InitMel(MEL *m)
{
    unsigned short nbins = _tosGetProfileU32(m, 3, 0);
    unsigned int nbands = _tosGetProfileU32(m, 4, 21);

    m->b.inputs[0].size = nbins * sizeof(float);
    m->b.outputs[0].size = nbands * sizeof(float);
    return 0;
}
