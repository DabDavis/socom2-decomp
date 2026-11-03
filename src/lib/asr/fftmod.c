/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing -funroll-loops */
/* fftmod: the recogniser's FFT block (OVL2 ..0x4E8AE8, SOCOM 1's object at 0x355A38:
 * CalcAmplitudeSpec, ProcessFFTMod, InitFFTMod, ConstructFFTMod, ControlFFTMod, all but the
 * constructor LOCAL there), ScanSoft's speech recogniser, prebuilt with ee-gcc 2.96 at -O2 -G0
 * -fno-strict-aliasing -funroll-loops (README "The GCC front"). Names are SOCOM 1's (the process
 * and init rows by the constructor's slots and the object's order). Profile 7 adds a second
 * output, which the control's command 1 hands out. Written: InitFFTMod (the bin range from the
 * profile's two frequencies: a separate variable for the clamped top bin is what gives the
 * target's movn), the constructor and the control; ProcessFFTMod (and the two rows before it,
 * 0x4E7AB8 and 0x4E7C78, which it calls) not yet. */
#include "tosblk.h"

typedef struct TOSLOG {
    TOS *tos;
    unsigned char id;
} TOSLOG;

typedef struct FFTMOD {
    TOSBLOCK b;
    unsigned int n;                     /* +40: the input frame's samples (profile 6) */
    unsigned int size;                  /* +44: the FFT's length (profile 4) */
    unsigned int lo;                    /* +48: the first bin kept (profile 1, Hz) */
    unsigned int hi;                    /* +52: the last (profile 2, Hz) */
    float floor;                        /* +56: profile 5 */
    unsigned char owns;                 /* +60: the buffer is the block's own (n < size) */
    char pad61[3];
    int pad;                            /* +64: the bytes of zeros after the frame */
    void *buf;                          /* +68 */
    short out2;                         /* +72: the second output, handed out by command 1 */
    unsigned char second;               /* +74: profile 7 */
    char pad75;
} FFTMOD;                               /* 76 */

extern void ProcessFFTMod(FFTMOD *m, unsigned char **in, int n);
static int InitFFTMod(FFTMOD *m);
static int ControlFFTMod(FFTMOD *m, unsigned char cmd, void **arg);
extern float floorf(float);

static int InitFFTMod(FFTMOD *m)
{
    TOS *tos = m->b.tos;
    short rate = _tosGetProfileU32(m, 3, 5500);
    unsigned int size, n, lo, hi, top;

    m->size = size = (unsigned short)_tosGetProfileU32(m, 4, 512);
    m->n = n = (unsigned short)_tosGetProfileU32(m, 6, 512);
    if (size < n)
        return 1;
    if (n < size) {
        m->owns = 1;
        m->pad = (size - n) * 4;
        m->buf = heap_Alloc(tos->heap, size * 4);
    }
    m->b.inputs[0].size = m->n * 4;
    if (m->second == 0) {
        m->b.outputs[0].size = (size / 2 + 1) * 4;
    } else {
        m->b.outputs[0].size = 4;
        m->b.outputs[1].size = (size / 2 + 1) * 4;
    }
    lo = floorf(m->size * _tosGetProfileFloat(m, 1, 300.0f) / (2.0f * rate));
    if (lo >= m->size / 2 - 1)
        lo = m->size / 2 - 2;
    m->lo = lo;
    hi = floorf(m->size * _tosGetProfileFloat(m, 2, 4500.0f) / (2.0f * rate) + 0.9999f);
    top = hi < m->size / 2 - 1 ? hi : m->size / 2 - 2;
    m->hi = m->lo < top ? top : m->lo;
    m->floor = _tosGetProfileFloat(m, 5, 0.0f);
    return 0;
}

TOSBLOCK *ConstructFFTMod(TOS *tos, unsigned char id)
{
    TOSLOG log;
    unsigned char second;
    FFTMOD *m;

    log.tos = tos;
    log.id = id;
    second = _tosGetProfileU32(&log, 7, 1);
    m = (FFTMOD *)tosBaseBlockConstruct(tos, id, 1, second ? 2 : 1, (TOSFUNC)ProcessFFTMod,
                                        (TOSFUNC)InitFFTMod, (TOSFUNC)ControlFFTMod,
                                        sizeof(FFTMOD));
    m->second = second;
    return &m->b;
}

static int ControlFFTMod(FFTMOD *m, unsigned char cmd, void **arg)
{
    TOS *tos = m->b.tos;

    switch (cmd) {
    case 1:
        if (m->second == 0) {
            _tosErrorLog(m, 102);
            return 1;
        }
        *arg = &m->out2;
        break;
    case TOS_DESTROY:
        if (m->owns != 0)
            heap_Free(tos->heap, m->buf);
        tosBaseBlockDestruct(&m->b);
        break;
    default:
        return 0;
    }
    return 1;
}
