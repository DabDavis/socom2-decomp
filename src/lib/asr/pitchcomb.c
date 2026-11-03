/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing -funroll-loops */
/* pitchcomb: the recogniser's pitch combiner block (OVL2 0x501A90-, SOCOM 1's object at 0x36CE48:
 * LogPitchProcessing, NormE_Processing, FlushPitchCombiner, ProcessPitchCombiner,
 * PitchCombinerReset, InitPitchCombiner, ConstructPitchCombiner, ControlPitchCombiner -- all but the
 * constructor LOCAL there; SOCOM II has one function more, func_005028F0, after the reset),
 * ScanSoft's speech recogniser, prebuilt with ee-gcc 2.96 at -O2 -G0 -fno-strict-aliasing
 * -funroll-loops (README "The GCC front"). Names are SOCOM 1's; the block's fields are named by
 * their use. Only NormE_Processing is written (byte-exact). */
#include "tosblk.h"

typedef struct PITCHCOMB {
    TOSBLOCK b;
    float *ring;                        /* +40: the energies' ring */
    int pad44[2];
    float *ringend;                     /* +52 */
    float *rd;                          /* +56: the window's first frame in the ring */
    char pad60[68];
    float *out;                         /* +128: the normalised energies' ring */
    int pad132;
    float *wr;                          /* +136: its next frame */
    float *outend;                      /* +140 */
    int pad144[2];
    float *weights;                     /* +152: the window's weights */
    char pad156;
    unsigned char nwin;                 /* +157: the window's frames */
    char pad158[7];
    unsigned char fill;                 /* +165: the head of the ring is still to be filled */
    char pad166[2];
    unsigned short norm;                /* +168 */
} PITCHCOMB;

/* the window's weighted energy over the norm into the output ring, the window moved on a frame;
 * at the start the first half-window of frames takes the first value. SOCOM 1's body: the index
 * zeroed at its declaration (the prologue's `move a3,zero`) and the sum loop's own test the only
 * one; the quotient back into the sum (the target divides into $f2); the output stored as
 * `*p->wr++` (one expression: the field's address taken once, the target's `addiu a1,a0,136` --
 * SOCOM 1's store and increment were two statements); the fill's value with the start-of-ring
 * case first (the other arm in the branch-likely slot) */
static void NormE_Processing(PITCHCOMB *p)
{
    unsigned char i = 0;
    float sum = 0;
    float *rd = p->rd;
    float *w;
    float *q;
    float last;

    w = p->weights;
    for (; i < p->nwin; i++) {
        sum += *rd++ * *w++;
        if (rd == p->ringend)
            rd = p->ring;
    }
    sum /= p->norm;
    *p->wr++ = sum;
    if (p->wr == p->outend)
        p->wr = p->out;
    p->rd++;
    if (p->rd == p->ringend)
        p->rd = p->ring;
    if (p->fill) {
        if (p->wr == p->out)
            last = p->outend[-1];
        else
            last = p->wr[-1];
        q = p->out;
        for (i = 0; i < p->nwin >> 1; i++)
            *q++ = last;
        p->fill = 0;
    }
}
