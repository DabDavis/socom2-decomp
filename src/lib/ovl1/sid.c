/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing */
/* The microphone input (OVL1 0x1E7F58-0x1E8530): the voice chat's capture over lgAud (the USB
 * headset), prebuilt with ee-gcc 2.96 at -O2 -G0 -fno-strict-aliasing (README "The GCC front":
 * SidGetSamples reloads the read position after storing through `out`). The demo's object 0x3B7288
 * (C), its functions in the same order. Names are SOCOM 1's (s1:exact/callee/calls; SidIsRecording
 * by the demo's 8-byte body at the same place). The capture ring holds 1760 samples (11025 Hz, 16
 * bits, mono); getsamples keeps one asynchronous lgAudARead in flight into it. */

#define SID_SAMPLES 1760

typedef unsigned char u_char;
typedef unsigned short u_short;

typedef struct SID {
    u_short recording;          /* +0 */
    short pad02;
    int hint;                   /* +4: lgAud's device-change hint */
    int handle;                 /* +8: the lgAud stream, -1 when closed */
    short buf[SID_SAMPLES];     /* +12 */
    short count;                /* +3532: samples buffered */
    short wpos;                 /* +3534: the write position */
    short pos;                  /* +3536: the read position */
    short pad;
    int pending;                /* +3540: the read in flight's bytes, -1 when none */
} SID;

typedef struct {
    u_char Mode;                /* 2: record */
    u_char pad01;
    u_char channels;
    u_char bits;
    u_short rate;
    u_short latency;
    u_char pad08[6];
} lgAudOpenParam;

typedef struct {
    int pad[83];
} lgAudDeviceDesc;

extern int lgAudInit(void *alloc, void *dealloc);
extern int lgAudASync(int mode);
extern int lgAudEnumerate(int index, lgAudDeviceDesc *desc);
extern int lgAudOpen(int index, lgAudOpenParam *param, int *handle);
extern int lgAudClose(int h);
extern int lgAudEnumHint(int *hint);
extern int lgAudAEnumHint(int *hint);
extern int lgAudARead(int handle, int channel, void *dest, int *size);
extern int lgAudStartRecording(int handle);
extern int lgAudStopRecording(int handle);
extern int lgAudSetRecordingVolume(int handle, u_char left, u_char right);
extern void *heap_Alloc(void *heap, unsigned int size);
extern void heap_Free(void *heap, void *p);

int getsamples(SID *s);

void flushbuffer(SID *s)
{
    lgAudASync(0);
    s->count = 0;
    s->wpos = 0;
    s->pos = 0;
    s->pending = -1;
}

int openmic(SID *s)
{
    lgAudDeviceDesc desc;
    lgAudOpenParam param;
    int i;

    lgAudASync(0);
    if (s->handle == -1) {
        flushbuffer(s);
        for (i = 0; lgAudEnumerate(i, &desc) == 0; i++) {
            param.Mode = 2;
            param.channels = 1;
            param.bits = 16;
            param.rate = 11025;
            param.latency = 500;
            if (lgAudOpen(i, &param, &s->handle) == 0) {
                lgAudSetRecordingVolume(s->handle, 0, 50);
                break;
            }
        }
        if (s->handle == -1)
            lgAudAEnumHint(&s->hint);
        return 1;
    }
    return 0;
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

int getsamples(SID *s)
{
    if (s->count >= 111)
        return 0;
    if (s->hint != 0 || s->handle == -1) {
        closemic(s);
        openmic(s);
        if (s->handle == -1)
            return 0;
        lgAudEnumHint(&s->hint);
    }
    if (s->pending != -1) {
        if (lgAudASync(1) != 0)
            return 0;
        lgAudEnumHint(&s->hint);
        if (s->hint != 0)
            return 0;
        s->count += s->pending / 2;
        s->wpos += s->pending / 2;
        s->wpos %= SID_SAMPLES;
    }
    s->pending = ((SID_SAMPLES - s->wpos < SID_SAMPLES - s->count) ? SID_SAMPLES - s->wpos
                                                                    : SID_SAMPLES - s->count) * 2;
    lgAudARead(s->handle, 1, &s->buf[s->wpos], &s->pending);
    return 0;
}

int startrecording(SID *s)
{
    lgAudASync(0);
    if (s->handle == -1)
        openmic(s);
    lgAudASync(0);
    if (s->handle != -1)
        lgAudStartRecording(s->handle);
    s->recording = 1;
    getsamples(s);
    return 0;
}

int stoprecording(SID *s)
{
    lgAudASync(0);
    if (s->handle == -1)
        openmic(s);
    lgAudASync(0);
    if (s->handle != -1)
        lgAudStopRecording(s->handle);
    flushbuffer(s);
    s->recording = 0;
    return 0;
}

int SidInit(void *heap, int unused, SID **psid)
{
    SID *s;

    if (lgAudInit(0, 0) != 0)
        return 0x80CC0006;
    s = heap_Alloc(heap, sizeof(SID));
    if (s == 0)
        return 0x80CC0086;
    s->hint = 2;
    s->handle = -1;
    s->recording = 0;
    openmic(s);
    getsamples(s);
    *psid = s;
    return 0;
}

u_short SidIsRecording(SID *s)
{
    return s->recording;
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

int SidAdvanceBuffer(SID *s, short n)
{
    if (s->recording != 0) {
        s->count -= n;
        s->pos += n;
        s->pos %= SID_SAMPLES;
        getsamples(s);
    }
    return 0;
}
