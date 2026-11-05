/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing -funroll-loops -ffast-math */
/* mel: the recogniser's mel filter-bank block (OVL2 0x4F6110-0x4F6CE0, SOCOM 1's object at 0x362D10:
 * MelInitBands, ConstructMel, ProcessMel, ControlMel, InitMel -- all but the constructor LOCAL
 * there), ScanSoft's speech recogniser, prebuilt with ee-gcc 2.96 at -O2 -G0 -fno-strict-aliasing
 * -funroll-loops -ffast-math (README "The GCC front": MelInitBands' min.s is -ffast-math's, round
 * 25c; every function written here compiles the same with and without it, round 26c, so the
 * directive carries the object's flag). Names are SOCOM 1's. Command 1 rebuilds the bands for
 * the sample rate its fourth argument carries as a float's bits; 255 destroys. A band is a run of
 * weights over the spectrum: its weights (+44), its first bin (+48) and its count (+52), one entry
 * per band. Round 26c wrote ConstructMel and InitMel (over their drafts) and ProcessMel; round 27c
 * MelInitBands, and the object's order: the block's process, control and init functions are
 * DEFERRED static inlines, defined above MelInitBands. ControlMel's call to MelInitBands carries no
 * nothrow note in the target (REG_EH_REGION 0 changes 73 words of ControlMel: reorg's slots and the
 * free loop's layout), so ControlMel was expanded before MelInitBands was compiled -- yet the image
 * holds MelInitBands first. 2.96 defers a static inline whose address is not yet taken and emits
 * it at the end of the object, in declaration order: so the three are expanded (their calls
 * noted then) before MelInitBands, and written after the constructor that takes their addresses --
 * MelInitBands, ConstructMel, ProcessMel, ControlMel, InitMel, the image's order (with the three
 * plain statics above MelInitBands they would lead the object; below it, ControlMel's call is
 * noted). */
#include "tosblk.h"

typedef struct MEL {
    TOSBLOCK b;
    unsigned int nbands;                /* +40 */
    float **bands;                      /* +44: each band's weights */
    unsigned int *first;                /* +48: each band's first bin */
    unsigned int *count;                /* +52: each band's weights */
    float rate;                         /* +56: the sample rate (MelInitBands') */
} MEL;                                  /* 60 (ConstructMel's) */

extern float logf(float x);
extern float powf(float x, float y);

static void ProcessMel(MEL *m, void **in);
static int ControlMel(MEL *m, unsigned char cmd, void *arg, int arg2);
static int InitMel(MEL *m);
static int MelInitBands(MEL *m, float rate);

/* one frame of the spectrum: each band's weighted sum of its bins, from 10 (SOCOM 1's body; the
 * sum kept in the output frame, which the weights may alias) */
static __inline__ void ProcessMel(MEL *m, void **in)
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
static __inline__ int ControlMel(MEL *m, unsigned char cmd, void *arg, int arg2)
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
static __inline__ int InitMel(MEL *m)
{
    unsigned short nbins = _tosGetProfileU32(m, 3, 0);
    unsigned int nbands = _tosGetProfileU32(m, 4, 21);

    m->b.inputs[0].size = nbins * sizeof(float);
    m->b.outputs[0].size = nbands * sizeof(float);
    return 0;
}

/* the bands for a sample rate: each bin's frequency on the mel scale (linear to 1 kHz, a hundred to
 * the unit, then five units an octave), the band count from the top frequency less the profile's
 * first band, then each band a triangle of weights two units wide round its centre (its count
 * and first bin from the bins inside it, its weights 1 - |c - x|). SOCOM 1's shape (0x880 there:
 * double log and pow, here logf and powf): the scale's double literals land on the target's
 * ulp-low bits as written (0.01, 0.001); the count's two branches each store it (`m->nbands = ...
 * - 0.95` in both arms: cross-jumping merges their tails after CSE, so `-= low` reloads +40; one
 * double assigned in the arms and stored once, CSE forwards the store and the reload is gone);
 * the weights' cursor post-incremented in the store of a `?:` (`*w++ = x < c ? ...`: the address
 * taken before the value, the target's `move a0,a2; addiu a2,a0,4`; an if/else storing in each arm
 * and `w++` after it lays the loop out otherwise); the band test `in` as `&&` of the two double
 * compares (the target's `li 1; movz`). */
static int MelInitBands(MEL *m, float rate)
{
    TOS *tos = m->b.tos;
    float *freq;
    float *center;
    unsigned short low;
    unsigned int i;
    unsigned short nbins;
    float top, step, f, lim, c;
    unsigned int k;

    top = _tosGetProfileFloat(m, 2, 4000.0f);
    low = _tosGetProfileU32(m, 5, 0);
    nbins = _tosGetProfileU32(m, 3, 0);
    m->rate = rate;
    freq = heap_Alloc(tos->heap, nbins * sizeof(float));
    if (freq == 0) {
        _tosErrorLog(m, 100);
        return 1;
    }
    step = top / (unsigned int)(nbins - 1);
    f = 0.0f;
    lim = top < 1000.0f ? top : 1000.0f;
    {
        float *p = freq;

        for (; f < lim; f += step)
            *p++ = f / (rate * 100.0f);
        lim = top + step * 0.5f;
        for (; f < lim; f += step)
            *p++ = logf(f / (rate * 1000.0f)) * 1.4426955f * 5.0f + 10.0f;
    }
    if (top < 1000.0f)
        m->nbands = top * 0.01 - 0.95;
    else
        m->nbands = logf(f * 0.001) * 5.0f * 1.4426955f + 10.0f - 0.95;
    m->nbands -= low;
    if (m->nbands != (unsigned short)_tosGetProfileU32(m, 4, 21))
        return 1;
    m->bands = heap_Calloc(tos->heap, m->nbands, sizeof(float *));
    if (m->bands == 0) {
        _tosErrorLog(m, 100);
        return 1;
    }
    m->first = heap_Calloc(tos->heap, m->nbands, sizeof(unsigned int));
    if (m->first == 0) {
        _tosErrorLog(m, 100);
        heap_Free(tos->heap, m->bands);
        return 1;
    }
    m->count = heap_Calloc(tos->heap, m->nbands, sizeof(unsigned int));
    if (m->count == 0) {
        _tosErrorLog(m, 100);
        heap_Free(tos->heap, m->bands);
        heap_Free(tos->heap, m->first);
        return 1;
    }
    center = heap_Calloc(tos->heap, m->nbands, sizeof(float));
    if (center == 0) {
        _tosErrorLog(m, 100);
        heap_Free(tos->heap, m->bands);
        heap_Free(tos->heap, m->first);
        heap_Free(tos->heap, m->count);
        return 1;
    }
    for (i = 0; i < m->nbands; i++) {
        float *w;
        int in;

        m->count[i] = 0;
        m->first[i] = -1;
        c = i + low + 1;
        if (i + low < 10)
            center[i] = c * 100.0f;
        else
            center[i] = powf(2.0f, c * 0.2f - 2.0) * 1000.0f;
        for (k = 1; k < nbins; k++) {
            in = freq[k] > c - 1.0 && freq[k] < c + 1.0;
            m->count[i] += in;
            if (in && m->first[i] == -1)
                m->first[i] = k;
        }
        m->bands[i] = heap_Calloc(tos->heap, m->count[i], sizeof(float));
        if (m->bands[i] == 0) {
            _tosErrorLog(m, 100);
            heap_Free(tos->heap, center);
            return 1;
        }
        w = m->bands[i];
        for (k = 0; k < m->count[i]; k++) {
            float x = freq[m->first[i] + k];

            *w++ = x < c ? 1.0f - c + x : 1.0f - x + c;
        }
    }
    heap_Free(tos->heap, center);
    heap_Free(tos->heap, freq);
    return 0;
}

TOSBLOCK *ConstructMel(TOS *tos, unsigned char id)
{
    return tosBaseBlockConstruct(tos, id, 1, 1, (TOSFUNC)ProcessMel, (TOSFUNC)InitMel,
                                 (TOSFUNC)ControlMel, sizeof(MEL));
}

