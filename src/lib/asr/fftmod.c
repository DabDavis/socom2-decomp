/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing -funroll-loops */
/* fftmod: the recogniser's FFT block (OVL2 ..0x4E8AE8, SOCOM 1's object at 0x355A38:
 * CalcAmplitudeSpec, ProcessFFTMod, InitFFTMod, ConstructFFTMod, ControlFFTMod, all but the
 * constructor LOCAL there), ScanSoft's speech recogniser, prebuilt with ee-gcc 2.96 at -O2 -G0
 * -fno-strict-aliasing -funroll-loops (README "The GCC front"). Names are SOCOM 1's (the process
 * and init rows by the constructor's slots and the object's order). Profile 7 adds a second
 * output, which the control's command 1 hands out. Written so far: the constructor and the
 * control; the process and init functions it names are declared, not yet defined. */
#include "tosblk.h"

typedef struct TOSLOG {
    TOS *tos;
    unsigned char id;
} TOSLOG;

typedef struct FFTMOD {
    TOSBLOCK b;
    char pad40[20];
    unsigned char owns;                 /* +60: the buffer is the block's own */
    char pad61[7];
    void *buf;                          /* +68 */
    short out2;                         /* +72: the second output, handed out by command 1 */
    unsigned char second;               /* +74: profile 7 */
    char pad75;
} FFTMOD;                               /* 76 */

extern void ProcessFFTMod(FFTMOD *m, unsigned char **in, int n);
extern int InitFFTMod(FFTMOD *m);
static int ControlFFTMod(FFTMOD *m, unsigned char cmd, void **arg);

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
