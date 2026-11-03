/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing -funroll-loops */
/* vad: the recogniser's voice-activity block (OVL2 0x4F9DC0-0x4FBA38, SOCOM 1's object at
 * 0x366410: ChangeMike, TestChangeMike, CheckForAbnormalConditions, VadGetSession, VadPutSession,
 * VadFastTrackProcess, ProcessVad, ControlVad, InitVad, ConstructVad -- all but the constructor
 * LOCAL there, the same order here),
 * ScanSoft's speech recogniser, prebuilt with ee-gcc 2.96 at -O2 -G0 -fno-strict-aliasing
 * -funroll-loops (README "The GCC front"). Names are SOCOM 1's. The constructor reads two
 * profile entries through a log record of its own: 45, the inputs, and 37, a flag that adds a
 * second output and that the block keeps. Written so far: the session pair (round 15c: the
 * state copied field by field, the arrays by loops 2.96 unrolls by ten, the two ring positions
 * kept as unsigned indices) and the constructor; the rest are declared, not yet defined. */
#include "tosblk.h"

typedef struct TOSLOG {
    TOS *tos;
    unsigned char id;
} TOSLOG;

typedef struct VAD {
    TOSBLOCK b;
    unsigned char f40;                  /* +40 */
    char pad41[3];
    float f44[6];                       /* +44 */
    int pad68;
    unsigned short f72[4];              /* +72 */
    float f80;                          /* +80 */
    float hist[20];                     /* +84 */
    int pad164;
    float *histpos;                     /* +168: in hist */
    short f172[150];                    /* +172 */
    char pad472[600];
    int f1072;                          /* +1072 */
    short f1076[40];                    /* +1076 */
    int pad1156;
    short *pos1160;                     /* +1160: in f1076 */
    unsigned char second;               /* +1164: profile 37 */
    char pad1165[91];
} VAD;                                  /* 1256 */

/* a session: the block's state field by field, its two ring positions as indices */
typedef struct VADSESSION {
    unsigned char f40;                  /* +0 */
    char pad1[3];
    float f44[6];                       /* +4 */
    unsigned short f72[4];              /* +28 */
    float f80;                          /* +36 */
    float hist[20];                     /* +40 */
    unsigned short histidx;             /* +120 */
    short f172[150];                    /* +122 */
    int f1072;                          /* +424 */
    short f1076[40];                    /* +428 */
    unsigned short idx1160;             /* +508 */
} VADSESSION;

static void VadGetSession(VAD *v, VADSESSION *ss)
{
    int i;

    ss->f40 = v->f40;
    ss->f44[0] = v->f44[0];
    ss->f44[1] = v->f44[1];
    ss->f44[2] = v->f44[2];
    ss->f44[3] = v->f44[3];
    ss->f44[4] = v->f44[4];
    ss->f44[5] = v->f44[5];
    ss->f72[0] = v->f72[0];
    ss->f72[1] = v->f72[1];
    ss->f72[2] = v->f72[2];
    ss->f72[3] = v->f72[3];
    ss->f80 = v->f80;
    for (i = 0; i < 20; i++)
        ss->hist[i] = v->hist[i];
    ss->histidx = v->histpos - v->hist;
    for (i = 0; i < 150; i++)
        ss->f172[i] = v->f172[i];
    ss->f1072 = v->f1072;
    for (i = 0; i < 40; i++)
        ss->f1076[i] = v->f1076[i];
    ss->idx1160 = v->pos1160 - v->f1076;
}

static void VadPutSession(VAD *v, VADSESSION *ss)
{
    int i;

    v->f40 = ss->f40;
    v->f44[0] = ss->f44[0];
    v->f44[1] = ss->f44[1];
    v->f44[2] = ss->f44[2];
    v->f44[3] = ss->f44[3];
    v->f44[4] = ss->f44[4];
    v->f44[5] = ss->f44[5];
    v->f72[0] = ss->f72[0];
    v->f72[1] = ss->f72[1];
    v->f72[2] = ss->f72[2];
    v->f72[3] = ss->f72[3];
    v->f80 = ss->f80;
    for (i = 0; i < 20; i++)
        v->hist[i] = ss->hist[i];
    v->histpos = &v->hist[ss->histidx];
    for (i = 0; i < 150; i++)
        v->f172[i] = ss->f172[i];
    v->f1072 = ss->f1072;
    for (i = 0; i < 40; i++)
        v->f1076[i] = ss->f1076[i];
    v->pos1160 = &v->f1076[ss->idx1160];
}

extern void ProcessVad(VAD *v, unsigned char **in, int n);
extern int ControlVad(VAD *v, unsigned char cmd);
extern int InitVad(VAD *v);

TOSBLOCK *ConstructVad(TOS *tos, unsigned char id)
{
    TOSLOG log;
    unsigned char second;
    VAD *v;

    log.tos = tos;
    log.id = id;
    second = _tosGetProfileU32(&log, 37, 0);
    v = (VAD *)tosBaseBlockConstruct(tos, id, _tosGetProfileU32(&log, 45, 1), second ? 2 : 1,
                                     (TOSFUNC)ProcessVad, (TOSFUNC)InitVad, (TOSFUNC)ControlVad,
                                     sizeof(VAD));
    v->second = second;
    return &v->b;
}
