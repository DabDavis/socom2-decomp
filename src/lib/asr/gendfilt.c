/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing -funroll-loops */
/* The recogniser's gender filter (OVL2 0x5011B8-0x501418, SOCOM 1's object at 0x36C5F0:
 * ProcessGenderFilter, ConstructGenderFilter, ControlGenderFilter, InitGenderFilter -- SOCOM II
 * defines the constructor first), ScanSoft's speech recogniser, prebuilt with ee-gcc 2.96 at -O2
 * -G0 -fno-strict-aliasing -funroll-loops (README "The GCC front"). Names are SOCOM 1's
 * (s1:align; ProcessGenderFilter by the constructor's process slot and position). The block
 * has one input per gender model (profile 1, three by default) and passes on the first frame
 * any of them delivers. The constructor reads its profile before the block exists, through a
 * log record of its own (the scheduler and the id), so it keeps a frame and makes no sibling
 * call. */
#include "tosblk.h"

typedef struct GENDERFILTER {
    TOSBLOCK b;
    unsigned int ninputs;               /* +40: profile 1 */
    unsigned short framesize;           /* +44: profile 2 */
} GENDERFILTER;

typedef struct TOSLOG {
    TOS *tos;
    unsigned char id;
} TOSLOG;

static void ProcessGenderFilter(GENDERFILTER *g, unsigned char **in, int n);
static int ControlGenderFilter(GENDERFILTER *g, unsigned char cmd);
static int InitGenderFilter(GENDERFILTER *g);

TOSBLOCK *ConstructGenderFilter(TOS *tos, unsigned char id)
{
    TOSLOG log;

    log.tos = tos;
    log.id = id;
    return tosBaseBlockConstruct(tos, id, _tosGetProfileU32(&log, 1, 3), 1,
                                 (TOSFUNC)ProcessGenderFilter, (TOSFUNC)InitGenderFilter,
                                 (TOSFUNC)ControlGenderFilter, sizeof(GENDERFILTER));
}

static void ProcessGenderFilter(GENDERFILTER *g, unsigned char **in, int n)
{
    unsigned char *frame = in[0];
    unsigned char *out;
    int i;

    for (i = 1; frame == 0 && i < n; i++)
        frame = in[i];
    if (frame != 0) {
        out = qEnQueueOne(g->b.outputs[0].q);
        if (out != 0)
            memcpy(out, frame, g->framesize);
    }
}

static int ControlGenderFilter(GENDERFILTER *g, unsigned char cmd)
{
    if (cmd != TOS_DESTROY)
        return 0;
    tosBaseBlockDestruct(&g->b);
    return 1;
}

static int InitGenderFilter(GENDERFILTER *g)
{
    unsigned int i;

    g->ninputs = (unsigned short)_tosGetProfileU32(g, 1, 3);
    g->framesize = _tosGetProfileU32(g, 2, 4);
    for (i = 0; i < g->ninputs; i++)
        g->b.inputs[i].size = g->framesize;
    g->b.outputs[0].size = g->framesize;
    return 0;
}
