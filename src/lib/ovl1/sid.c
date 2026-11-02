/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing */
/* The microphone input (OVL1 0x1E7F58-0x1E8530): the voice chat's capture over lgAud (the USB
 * headset), prebuilt with ee-gcc 2.96 at -O2 -G0 -fno-strict-aliasing (README "The GCC front":
 * SidGetSamples reloads the read position after storing through `out`). Names are SOCOM 1's
 * (s1:exact). The capture buffer holds 1760 samples. */

#define SID_SAMPLES 1760

typedef struct SID {
    int f0;
    int f4;
    int handle;                 /* +8: the lgAud stream, -1 when closed */
    short buf[SID_SAMPLES];     /* +12 */
    short count;                /* +3532: samples buffered */
    short f3534;
    short pos;                  /* +3536: the read position */
    short pad;
    int f3540;
} SID;

extern int lgAudASync(int mode);
extern int lgAudClose(int h);
extern void heap_Free(void *heap, void *p);
extern int startrecording(SID *s);
extern int stoprecording(SID *s);
extern int getsamples(SID *s);

void flushbuffer(SID *s)
{
    lgAudASync(0);
    s->count = 0;
    s->f3534 = 0;
    s->pos = 0;
    s->f3540 = -1;
}

int closemic(SID *s)
{
    lgAudASync(0);
    flushbuffer(s);
    if (s->handle != -1) {
        lgAudClose(s->handle);
        s->handle = -1;
    }
    return 0;
}

int SidClose(void *heap, SID *s)
{
    closemic(s);
    heap_Free(heap, s);
    return 0;
}

int SidStart(SID *s)
{
    startrecording(s);
    return 0;
}

int SidStop(SID *s)
{
    stoprecording(s);
    return 0;
}

int SidGetSamples(SID *s, short **out)
{
    getsamples(s);
    *out = &s->buf[s->pos];
    return (s->count < SID_SAMPLES - s->pos) ? s->count : SID_SAMPLES - s->pos;
}

int SidGetSamplesLeft(SID *s)
{
    int left = SID_SAMPLES - s->pos;

    return (s->count < left) ? s->count : left;
}
