/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing -funroll-loops -ffast-math */
/* fftmod: the recogniser's FFT block (OVL2 0x4E7C78-0x4E8AE8, SOCOM 1's object at 0x355A38:
 * CalcAmplitudeSpec, ProcessFFTMod, InitFFTMod, ConstructFFTMod, ControlFFTMod, all but the
 * constructor LOCAL there), ScanSoft's speech recogniser, prebuilt with ee-gcc 2.96 at -O2 -G0
 * -fno-strict-aliasing -funroll-loops -ffast-math (README "The GCC front"; the object's sqrtf is a
 * bare sqrt.s, only under -ffast-math -- the twelfth such object, round 33c; the other four rows
 * compile the same either way). Names are SOCOM 1's (the process and init rows by the
 * constructor's slots and the object's order). Profile 7 adds a second output, which the
 * control's command 1 hands out. Written: InitFFTMod (the bin range from the profile's two
 * frequencies: a separate variable for the clamped top bin is what gives the target's movn), the
 * constructor and the control; CalcAmplitudeSpec and ProcessFFTMod (round 33c: the object whole,
 * 5 of 5). realfft (0x4E7AB8), which the process calls first, is fht.c's. */
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

void ProcessFFTMod(FFTMOD *m, float **in, int n);
extern void realfft(unsigned int n, float *x);
extern int qEnQueue(void *q, char *data, unsigned int n);
extern float logf(float x);
extern float sqrtf(float x);
static int InitFFTMod(FFTMOD *m);
static int ControlFFTMod(FFTMOD *m, unsigned char cmd, void **arg);
extern float floorf(float);

/* the power spectrum of the real FFT x (realfft's packing: bin i's real part at i, its imaginary
 * part at size - i) into out, bins 0 to size / 2, and the band's energy -- the bins lo to hi summed
 * onto the profile's floor -- returned. SOCOM 1's LOCAL (a magnitude spectrum there, with no
 * energy), written from SOCOM II's code (round 33c). The output pointer is the walker and the
 * band's first-bin test reads a copy taken before it moves (the target's `move a3,t0`; a walker of
 * its own leaves the parameter in a2 and swaps the registers: 64 words) */
static float CalcAmplitudeSpec(FFTMOD *m, float *x, float *out)
{
    float *re = x;
    float *im = x + m->size - 1;
    float *first = out;
    float *lo = out + m->lo;
    float *hi = out + m->hi;
    float *half = out + m->size / 2;
    float e = m->floor;
    float v;

    *out++ = *re * *re;
    re++;
    if (first == lo)
        e += *lo++;
    while (out < lo) {
        *out++ = *re * *re + *im * *im;
        re++;
        im--;
    }
    while (out <= hi) {
        v = *re * *re + *im * *im;
        *out++ = v;
        e += v;
        re++;
        im--;
    }
    while (out < half) {
        *out++ = *re * *re + *im * *im;
        re++;
        im--;
    }
    *out = *re * *re;
    return e;
}

/* the magnitude spectrum of the real FFT x into out, bins 0 to size / 2 (SOCOM 1's
 * CalcAmplitudeSpec, its sqrt now sqrtf: a bare sqrt.s under -ffast-math); inlined into the
 * process, the only caller. The name is ours. */
static __inline__ void CalcMagnitudeSpec(FFTMOD *m, float *x, float *out)
{
    float *re = x;
    float *im = x + m->size - 1;
    float *o = out;
    float *lo = out + m->lo;
    float *hi = out + m->hi;
    float *half = out + m->size / 2;

    *o++ = sqrtf(*re * *re);
    re++;
    if (out == lo)
        lo++;
    while (o < lo) {
        *o++ = sqrtf(*re * *re + *im * *im);
        re++;
        im--;
    }
    while (o <= hi) {
        *o++ = sqrtf(*re * *re + *im * *im);
        re++;
        im--;
    }
    while (o < half) {
        *o++ = sqrtf(*re * *re + *im * *im);
        re++;
        im--;
    }
    *o = sqrtf(*re * *re);
}

/* a frame (the block's process): the input copied into the block's buffer and padded with zeros
 * when the FFT is longer than the frame (a null input, nothing); the real FFT taken, then with the
 * second output the power spectrum and the band's log energy (queued on the first output), the
 * energy's level in hundredths of a dB above 27.37 clamped to 1800 and held as a peak decaying by
 * 64000/65536 a frame; without it the magnitude spectrum. SOCOM 1's LOCAL (with no energy there),
 * written from SOCOM II's code (round 33c) */
void ProcessFFTMod(FFTMOD *m, float **in, int n)
{
    float *x, *out;
    float e;
    short level;

    if (m->owns != 0) {
        if (in[0] != 0) {
            memcpy(m->buf, in[0], m->b.inputs[0].size);
            x = m->buf;
            memset(x + m->n, 0, m->pad);
        } else
            x = 0;
    } else
        x = in[0];
    if (x == 0)
        return;
    if (m->second)
        out = qEnQueueOne(m->b.outputs[1].q);
    else
        out = qEnQueueOne(m->b.outputs[0].q);
    realfft(m->size, x);
    if (m->second) {
        e = logf(CalcAmplitudeSpec(m, x, out));
        qEnQueue(m->b.outputs[0].q, (char *)&e, 1);
        level = (e - 27.37f) * 4.3429448f * 100.0;
        if (level > 1800)
            level = 1800;
        if (m->out2 < level)
            m->out2 = level;
        else
            m->out2 = ((m->out2 + 7200) * 64000 >> 16) - 7200;
    } else
        CalcMagnitudeSpec(m, x, out);
}

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
