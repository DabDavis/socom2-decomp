/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing -funroll-loops */
/* The recogniser's undersampler (OVL2 0x4F3330-0x4F35A0, SOCOM 1's object at 0x360258: the same
 * four functions in the same order, all but the constructor static there), ScanSoft's speech
 * recogniser, prebuilt with ee-gcc 2.96 at -O2 -G0 -fno-strict-aliasing -funroll-loops (README
 * "The GCC front"). Names are SOCOM 1's (s1:align; ProcessUndersampler and InitUndersampler by
 * the constructor's slots and position). A frame of `nsamples` 16-bit samples goes out as one
 * int per `factor` samples, their sum; the factor is the input rate over the output rate
 * (profiles 2 and 3), and the frame must hold a whole number of groups. The rates are unsigned
 * shorts, so their quotient is an unsigned division (2.96 shortens it); the frame's length is
 * an int, a signed one. The output size is taken into a local before the input size is stored:
 * the test's division then gives the quotient too, and only its zero trap stays in the success
 * path (a store between would reload the factor and divide again). */
#include "tosblk.h"

typedef struct UNDERSAMPLER {
    TOSBLOCK b;
    unsigned short nsamples;            /* +40: profile 1 */
    unsigned char factor;               /* +42 */
} UNDERSAMPLER;

static void ProcessUndersampler(UNDERSAMPLER *u, short **in);
static int ControlUndersampler(UNDERSAMPLER *u, unsigned char cmd);
static int InitUndersampler(UNDERSAMPLER *u);

TOSBLOCK *ConstructUndersampler(TOS *tos, unsigned char id)
{
    return tosBaseBlockConstruct(tos, id, 1, 1, (TOSFUNC)ProcessUndersampler,
                                 (TOSFUNC)InitUndersampler, (TOSFUNC)ControlUndersampler,
                                 sizeof(UNDERSAMPLER));
}

static void ProcessUndersampler(UNDERSAMPLER *u, short **in)
{
    short *frame = in[0];
    unsigned char factor = u->factor;
    short *p, *end, *group;
    int *out;
    int sum;

    if (frame != 0) {
        out = qEnQueueOne(u->b.outputs[0].q);
        end = frame + u->nsamples;
        for (p = frame; p < end; ) {
            group = p + factor;
            for (sum = *p++; p < group; p++)
                sum += *p;
            *out++ = sum;
        }
    }
}

static int ControlUndersampler(UNDERSAMPLER *u, unsigned char cmd)
{
    if (cmd != TOS_DESTROY)
        return 0;
    tosBaseBlockDestruct(&u->b);
    return 1;
}

static int InitUndersampler(UNDERSAMPLER *u)
{
    int n, outsize;
    unsigned short rate, outrate;

    n = u->nsamples = _tosGetProfileU32(u, 1, 110);
    rate = _tosGetProfileU32(u, 2, 11000);
    outrate = _tosGetProfileU32(u, 3, 2000);
    u->factor = rate / outrate;
    if (n % u->factor != 0) {
        _tosErrorLog(u, 100);
        return 1;
    }
    outsize = n / u->factor * sizeof(int);
    u->b.inputs[0].size = n * sizeof(short);
    u->b.outputs[0].size = outsize;
    return 0;
}
