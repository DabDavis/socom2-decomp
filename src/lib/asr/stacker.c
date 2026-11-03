/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing -funroll-loops */
/* The recogniser's stacker (OVL2 0x4F2F90-0x4F3330, SOCOM 1's object at 0x35FEC8: InitStacker,
 * ConstructStacker, ProcessStacker, ControlStacker, all but the constructor static there),
 * ScanSoft's speech recogniser, prebuilt with ee-gcc 2.96 at -O2 -G0 -fno-strict-aliasing
 * -funroll-loops (README "The GCC front"). Names are SOCOM 1's (s1:align; ProcessStacker by
 * the constructor's slot and position). The block's output frame is its inputs' frames side by
 * side (two inputs: profile 1 sizes the first, profile 2 the second); an input switched off
 * (command 100/101, back on with 150/151) keeps its place in the frame, unwritten. Nothing goes
 * out until some input delivers. */
#include "tosblk.h"

typedef struct STACKER {
    TOSBLOCK b;
    char pad40[4];
    unsigned short ninputs;             /* +44: profile 3 */
    unsigned short *sizes;              /* +48: each input's bytes */
    unsigned char *enabled;             /* +52 */
} STACKER;

static int InitStacker(STACKER *s);
static void ProcessStacker(STACKER *s, unsigned char **in, int n);
static int ControlStacker(STACKER *s, unsigned char cmd);

static int InitStacker(STACKER *s)
{
    TOS *tos = s->b.tos;
    unsigned short i = 0;

    s->ninputs = _tosGetProfileU32(s, 3, 2);
    s->sizes = heap_Calloc(tos->heap, s->ninputs, sizeof(unsigned short));
    s->enabled = heap_Calloc(tos->heap, s->ninputs, 1);
    if (s->sizes == 0 || s->enabled == 0) {
        _tosErrorLog(s, 2);
        return 1;
    }
    for (; i < s->ninputs; i++) {
        switch (i) {
        case 0:
            s->sizes[0] = _tosGetProfileU32(s, 1, 152);
            break;
        case 1:
            s->sizes[1] = _tosGetProfileU32(s, 2, 20);
            break;
        default:
            _tosErrorLog(s, 200);
            return 1;
        }
        s->enabled[i] = 1;
    }
    for (i = 0; i < s->ninputs; i++) {
        s->b.inputs[i].size = s->sizes[i];
        s->b.outputs[0].size += s->b.inputs[i].size;
    }
    return 0;
}

TOSBLOCK *ConstructStacker(TOS *tos, unsigned char id)
{
    return tosBaseBlockConstruct(tos, id, 2, 1, (TOSFUNC)ProcessStacker, (TOSFUNC)InitStacker,
                                 (TOSFUNC)ControlStacker, sizeof(STACKER));
}

static void ProcessStacker(STACKER *s, unsigned char **in, int n)
{
    unsigned short i;
    unsigned char *out;

    for (i = 0; i < n; i++)
        if (in[i] != 0)
            break;
    if (i != n) {
        out = qEnQueueOne(s->b.outputs[0].q);
        for (i = 0; i < n; i++) {
            if (s->enabled[i])
                memcpy(out, in[i], s->sizes[i]);
            out += s->sizes[i];
        }
    }
}

static int ControlStacker(STACKER *s, unsigned char cmd)
{
    switch (cmd) {
    case 100:
        s->enabled[0] = 0;
        break;
    case 101:
        s->enabled[1] = 0;
        break;
    case 150:
        s->enabled[0] = 1;
        break;
    case 151:
        s->enabled[1] = 1;
        break;
    case TOS_DESTROY:
        heap_Free(s->b.tos->heap, s->sizes);
        heap_Free(s->b.tos->heap, s->enabled);
        tosBaseBlockDestruct(&s->b);
        break;
    default:
        return 0;
    }
    return 1;
}
