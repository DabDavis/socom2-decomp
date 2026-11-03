/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing -funroll-loops */
/* specsub: the recogniser's spectral subtraction block (OVL2 0x4F8130-0x4F9DC0, SOCOM 1's object at
 * 0x364AC8: SpecSubPutSession, SpecSubGetSession, SpectralSubtraction, FreeResourcesSpecSub,
 * ControlSpecSub, InitSpecSub, ConstructSpecSub, ProcessSpecSub -- SOCOM 1's order;
 * FreeResourcesSpecSub inlined into ControlSpecSub, no copy left), ScanSoft's speech recogniser,
 * prebuilt with ee-gcc 2.96 at -O2 -G0 -fno-strict-aliasing -funroll-loops (README "The GCC
 * front"). Names are SOCOM 1's (the constructor's slots, s1:align, s1:calls); the block's fields
 * are named by their use. A block of the scheduler's (asr/tosblk.h): two inputs, four outputs.
 * Only the functions written so far are here, in image order (SpectralSubtraction, 0x1208, and
 * InitSpecSub, 0x40C, are not written: declared, their rows open). The session functions copy the
 * record in field order, the ring's size taken into the count's own variable (`n += s->nring * n`:
 * the target's sum into the count's register). */
#include "tosblk.h"

typedef struct SPECSUB {
    TOSBLOCK b;
    unsigned short f40;                 /* +40 */
    unsigned short f42;                 /* +42 */
    float *noise;                       /* +44: 2n */
    float *lognoise;                    /* +48: half the log of the noise */
    float *f52;                         /* +52 */
    float *f56;                         /* +56 */
    float *ring;                        /* +60: (nring + 1) frames of n */
    float *f64;                         /* +64 */
    float *ringpos;                     /* +68 */
    float *ringend;                     /* +72 */
    unsigned short nring;               /* +76: profile 3 */
    short pad78;
    float *f80;                         /* +80 */
    unsigned int n;                     /* +84: profile 8 */
    unsigned short f88;                 /* +88: profile 6, less one */
    short pad90;
    float f92;                          /* +92 */
    float f96;                          /* +96 */
    float f100;
    float f104;                         /* +104 */
    float f108;                         /* +108 */
    int pad112;
    void *f116;                         /* +116: freed with the block */
    int pad120[2];
    float f128;                         /* +128 */
    float f132;                         /* +132 */
    float f136;
    float f140;
    float f144;
    float f148;
} SPECSUB;                              /* 0x98 bytes (ConstructSpecSub) */

/* a session: the two counters, the level, the four spectra, the ring's two positions, the ring */
typedef struct SPECSUBSESSION {
    int f40;
    int f42;
    float f128;
    float data[1];
} SPECSUBSESSION;

extern float logf(float x);
extern int qQueueControl(void *q, int cmd, unsigned short reader, int arg);
extern void heap_Free(void *heap, void *p);

/* not written yet (their rows open) */
int SpectralSubtraction(SPECSUB *s, unsigned char flag, void *frame, float *out1, float *out0);
int InitSpecSub(SPECSUB *s);
static void ProcessSpecSub(SPECSUB *s, void **in);

/* a session read back: the spectra copied in, the noise's half log recomputed, the ring's
 * positions from their offsets */
static void SpecSubPutSession(SPECSUB *s, SPECSUBSESSION *ss)
{
    unsigned int n = s->n;
    float *p = ss->data;
    unsigned int i = 0;

    s->f40 = ss->f40;
    s->f42 = ss->f42;
    s->f128 = ss->f128;
    memcpy(s->noise, p, n * 4);
    p += n;
    memcpy(s->f52, p, n * 4);
    p += n;
    memcpy(s->f56, p, n * 4);
    p += n;
    memcpy(s->f80, p, n * 4);
    p += n;
    for (; i < n; i++)
        s->lognoise[i] = logf(s->noise[i]) * 0.5f;
    s->ringpos = s->ring + *(int *)p;
    p++;
    s->ringend = s->ring + *(int *)p;
    n += s->nring * n;
    memcpy(s->ring, p + 1, n * 4);
}

/* the session written out, in the same order */
static void SpecSubGetSession(SPECSUB *s, SPECSUBSESSION *ss)
{
    float *p = ss->data;
    unsigned int n = s->n;

    ss->f40 = s->f40;
    ss->f42 = s->f42;
    ss->f128 = s->f128;
    memcpy(p, s->noise, n * 4);
    p += n;
    memcpy(p, s->f52, n * 4);
    p += n;
    memcpy(p, s->f56, n * 4);
    p += n;
    memcpy(p, s->f80, n * 4);
    p += n;
    *(int *)p = s->ringpos - s->ring;
    p++;
    *(int *)p = s->ringend - s->ring;
    n += s->nring * n;
    memcpy(p + 1, s->ring, n * 4);
}

/* the resources freed with the block (SOCOM 1's FreeResourcesSpecSub, inlined) */
static __inline__ void FreeResourcesSpecSub(SPECSUB *s)
{
    heap_Free(s->b.tos->heap, s->ring);
    heap_Free(s->b.tos->heap, s->f116);
}

/* OPEN (3 words, the right size): the reset's first loop bound reads the field again each turn
 * beside a local copy taken before the loop (`n + s->n`: the target's two loads of +84, one kept,
 * one reloaded -- the demo's ControlSpecSub has the same pair); its own pointer per loop and `n`
 * taken again after it give every word but the first load's register: the target loads +84 into
 * a1 and copies the local to a2, here the local takes the load and a1 the copy (int, a block-local
 * declaration, the copy taken before the session test, operand orders: 3 to 17 words).
 *
 * the block's commands (case bodies in the target's order): 2 and 1 a session in and out (a null session resets the spectra), 3 and 4
 * the second input's queue stopped and started, 255 the block destroyed */
static int ControlSpecSub(SPECSUB *s, unsigned char cmd, char *sess, int *off)
{
    int ret = 1;
    float *p;
    unsigned int n;

    switch (cmd) {
    case 2:
        if (sess != 0) {
            SpecSubPutSession(s, (SPECSUBSESSION *)(sess + *off));
            *off += (s->nring * 4 + 20) * s->n + 20;
        } else {
            float *q;

            n = s->n;
            for (q = s->noise; q < s->noise + n + s->n; q++)
                *q = 1.0f;
            n = s->n;
            s->f42 = 0;
            for (p = s->f56; p < s->f56 + n; p++)
                *p = 1.0f;
            for (p = s->f52; p < s->f52 + n; p++)
                *p = 1.0f;
            s->f40 = 0;
            s->f128 = 1.0f;
        }
        break;
    case 1:
        if (sess != 0)
            SpecSubGetSession(s, (SPECSUBSESSION *)(sess + *off));
        *off += (s->nring * 4 + 20) * s->n + 20;
        break;
    case 3:
        ret = qQueueControl(s->b.inputs[1].q, 5, s->b.inputs[1].reader, 0);
        break;
    case 4:
        ret = qQueueControl(s->b.inputs[1].q, 6, s->b.inputs[1].reader, 0);
        break;
    case TOS_DESTROY:
        FreeResourcesSpecSub(s);
        tosBaseBlockDestruct(&s->b);
        break;
    default:
        return 0;
    }
    return ret;
}

TOSBLOCK *ConstructSpecSub(TOS *tos, unsigned char id)
{
    return tosBaseBlockConstruct(tos, id, 2, 4, (TOSFUNC)ProcessSpecSub, (TOSFUNC)InitSpecSub,
                                 (TOSFUNC)ControlSpecSub, sizeof(SPECSUB));
}

/* a frame: the subtraction into the first two outputs, the noise's half log and the log level
 * into the other two */
static void ProcessSpecSub(SPECSUB *s, void **in)
{
    void *frame = in[0];

    if (frame != 0) {
        unsigned char *flag = in[1];
        float *o0 = qEnQueueOne(s->b.outputs[0].q);
        float *o1 = qEnQueueOne(s->b.outputs[1].q);
        float *o2 = qEnQueueOne(s->b.outputs[2].q);
        float *o3 = qEnQueueOne(s->b.outputs[3].q);

        SpectralSubtraction(s, *flag, frame, o1, o0);
        memcpy(o2, s->lognoise, s->n * 4);
        *o3 = logf(s->f128);
    }
}
