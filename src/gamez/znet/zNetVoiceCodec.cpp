/* zNetVoiceCodec.cpp -- the voice's glue to the speech codec and its echo canceller (the prebuilt
 * library's, OVL1 0x24F000-0x257000): the codec pair made and released, a frame encoded or a packet
 * decoded, and the canceller's and the encoder's switches, each a null-tested handle passed on.
 * OVL1 0x3103D0-0x31097F, after zNetVoice.cpp and before node_vector.cpp; the unit and its name are
 * ours. A UNIT OF ITS OWN (the online lane's round 26, tools/unitscan.py): zNetVoice.cpp's four
 * `#pragma auto_inline off` pins (0x3103D0, 0x310480, 0x3106A0, 0x310750 -- each called out of line
 * by its one caller below 0x3103D0) are every pinned pair the unit had, and a cut at 0x3103D0
 * separates all four and no inlined pair; the image's own calls (17) all cross it, none comes
 * back -- split here, both halves score as before with the pins gone. Defined in reverse of their
 * image order (-inline deferred). */

#include <string.h>

extern "C" {
void *calloc(unsigned int n, unsigned int size);
void free(void *p);
void *malloc(unsigned int n);
}

/* the speech codec's echo canceller, a handle (OVL1 0x2527F8-0x252BA0) */
struct VoiceEcho {
    void *m_handle;   // 0x0
};

/* the speech codec's pair: the encoder's and the decoder's handles, each with its frame -- the
 * samples, their count, the packed bytes and their room (56 bytes: VoiceEncoder and VoiceDecoder
 * below are its views; the encoder's preprocessing switches take the pair, its first word the
 * encoder) */
struct CodecFrame {
    short *m_samples;   // 0x0
    unsigned short m_count;   // 0x4
    unsigned char *m_bytes;   // 0x8
    unsigned short m_size;   // 0xc
    unsigned short m_e;   // 0xe
    unsigned char m_10;   // 0x10
    unsigned short m_length;   // 0x12
    unsigned char m_carry;   // 0x14
};   // 0x18
struct VoiceCodec {
    void *m_enc;   // 0x0
    void *m_dec;   // 0x4
    CodecFrame m_encframe;   // 0x8
    CodecFrame m_decframe;   // 0x20
};   // 0x38

/* the speech encoder: its handle, then the frame it works on -- 320 bytes of samples in, the
 * packed bytes out, their count, and whether a byte carries over (the codec's; OVL1 0x24F888) */
struct VoiceFrame {
    void *m_in;   // 0x0
    int m_4;
    unsigned char *m_out;   // 0x8
    unsigned char _c[0x12 - 0xc];
    unsigned short m_length;   // 0x12
    unsigned char m_carry;   // 0x14
};
struct VoiceEncoder {
    void *m_handle;   // 0x0
    int m_4;
    VoiceFrame m_frame;   // 0x8
};

/* the speech decoder: its handle, the samples it hands out and the packed bytes it takes */
struct VoiceDecoder {
    int m_0;
    void *m_handle;   // 0x4
    unsigned char _8[0x20 - 0x8];
    short *m_samples;   // 0x20
    unsigned short m_count;   // 0x24
    unsigned char _26[2];
    void *m_in;   // 0x28
    short m_inlen;   // 0x2c
};

/* the encoder's preprocessing settings, as the radio keeps them (its 0x1084: six floats) */
struct VoiceSettings {
    float m_0;   // 0x0
    float m_4;   // 0x4
    float m_8;   // 0x8
    float m_c;   // 0xc
    float m_10;   // 0x10
    float m_14;   // 0x14
};   // 0x18

/* what the codec says of its frames (only what is read here) */
struct CodecInfo {
    unsigned char _0[0x3c];
    unsigned short m_samples;   // 0x3c: a frame's samples
    unsigned char _3e[0x60 - 0x3e];
    unsigned long m_bits;   // 0x60: a packed frame's bits
    unsigned char _68[0x70 - 0x68];
};

extern "C" {
/* the codec library (OVL1's prebuilt speech codec) */
void func_00256F48(void *(*alloc)(unsigned int n, unsigned int size));   /* (the codec's allocator set) */
void func_00256F68(void (*release)(void *p));   /* (and its release) */
void func_00255A68(CodecInfo *info);   /* (the codec described) */
void func_00255BD0(void **dec);   /* (the decoder made) */
void func_0024F990(void **enc);   /* (the encoder made) */
void func_00255C68(void **dec);   /* (the decoder released) */
void func_0024FA30(void **enc);   /* (the encoder released) */
void func_0024F888(void *h, VoiceFrame *f);   /* (a frame encoded) */
void func_00255B68(void *h, void *frame);   /* (the decoder: a frame decoded) */
void func_00255CF8(void *h, unsigned char *done);   /* (and whether the packet is used up) */
/* the encoder's preprocessing stages: the echo canceller's (0x24F6F8 off, 0x24F6A8 on; its three
 * settings 0x24F518, 0x24F568, 0x24F5B8), the gain's (0x24F838 off, 0x24F608 on with its level)
 * and the noise's (0x24F798 off, 0x24F658 on with its two levels) */
void func_0024F6F8(void *h);
void func_0024F6A8(void *h);
void func_0024F518(void *h, float v);
void func_0024F568(void *h, float v);
void func_0024F5B8(void *h, float v);
void func_0024F838(void *h);
void func_0024F608(void *h, float v);
void func_0024F798(void *h);
void func_0024F658(void *h, float a, float b);
/* the echo canceller's */
void func_002529C0(void *h, short *level);
void func_00252BA0(void *h, short *level);
void func_00252880(void *h, int v);
void func_00252920(void *h, int v);
void func_00252B50(void *h, int v);
void func_00252AB0(void *h, short v);
void func_002527F8(void *h);
void func_00252A60(void *h);
void func_00252748(void *h);
void func_00252A10(void *h);

/* the codec's allocator and release: the C library's (OVL1 0x310970, 0x310960) */
void *func_00310970(unsigned int n, unsigned int size)
{
    return calloc(n, size);
}

void func_00310960(void *p)
{
    free(p);
}

/* the codec pair made: the library given its allocator, the pair cleared, each frame's buffers
 * sized from the codec's description -- the packed bytes rounded up, two over (OVL1 0x310830) */
VoiceCodec *func_00310830()
{
    func_00256F48(func_00310970);
    func_00256F68(func_00310960);
    VoiceCodec *c = (VoiceCodec *)malloc(sizeof(VoiceCodec));

    if (c) {
        CodecInfo info;

        memset(c, 0, sizeof(VoiceCodec));
        c->m_enc = 0;
        c->m_dec = 0;
        func_00255A68(&info);
        int bytes = info.m_bits >> 3;

        if (info.m_bits & 7)
            bytes++;
        c->m_encframe.m_samples = (short *)malloc(info.m_samples * 2);
        c->m_encframe.m_count = info.m_samples;
        c->m_encframe.m_bytes = (unsigned char *)malloc(bytes + 2);
        c->m_encframe.m_size = bytes + 2;
        c->m_encframe.m_e = 0;
        c->m_encframe.m_10 = 0;
        c->m_encframe.m_length = 0;
        c->m_encframe.m_carry = 0;
        c->m_decframe.m_samples = (short *)malloc(info.m_samples * 2);
        c->m_decframe.m_count = info.m_samples;
        c->m_decframe.m_bytes = (unsigned char *)malloc(bytes + 2);
        c->m_decframe.m_size = bytes + 2;
        c->m_decframe.m_e = 0;
        c->m_decframe.m_10 = 0;
        c->m_decframe.m_length = 0;
        c->m_decframe.m_carry = 0;
        func_00255BD0(&c->m_dec);
        func_0024F990(&c->m_enc);
    }
    return c;
}

/* the codec pair released: the decoder and the encoder, each frame's packed bytes then its
 * samples, the pair (OVL1 0x3107D0: the headset's close 0x30FE40 calls it on the radio's codec) */
void func_003107D0(VoiceCodec *c)
{
    if (c) {
        func_00255C68(&c->m_dec);
        func_0024FA30(&c->m_enc);
        free(c->m_encframe.m_bytes);
        free(c->m_encframe.m_samples);
        free(c->m_decframe.m_bytes);
        free(c->m_decframe.m_samples);
        free(c);
    }
}

/* a frame of samples encoded: the packed bytes handed out and their count returned, a carried
 * byte moved to the front (OVL1 0x310750; the record tick 0x30F990 calls it) */
int func_00310750(VoiceEncoder *e, const void *samples, void *out)
{
    memcpy(e->m_frame.m_in, samples, 320);
    func_0024F888(e->m_handle, &e->m_frame);
    int n = e->m_frame.m_length;

    memcpy(out, e->m_frame.m_out, n);
    if (e->m_frame.m_carry > 0)
        e->m_frame.m_out[0] = e->m_frame.m_out[n];
    e->m_frame.m_length = 0;
    return n;
}

/* a packet of 8 bytes decoded frame by frame into the caller's samples (OVL1 0x3106A0). The
 * count returned is the original's slip: it doubles itself (`addu s0,s0,s0`) where the frames'
 * counts were surely meant, so it stays 0 (the packet handler 0x30F7E0 calls it) */
int func_003106A0(VoiceDecoder *d, const void *packet, short *out)
{
    int n = 0;
    unsigned char done;

    memcpy(d->m_in, packet, 8);
    d->m_inlen = 8;
    do {
        func_00255B68(d->m_handle, &d->m_samples);
        if (d->m_count > 0) {
            memcpy(out, d->m_samples, d->m_count * 2);
            n += n;
            out += d->m_count;
        }
        func_00255CF8(d->m_handle, &done);
    } while (!(bool)done);
    return n * 2;
}

/* the encoder's echo canceller's three settings (OVL1 0x310630) */
void func_00310630(VoiceCodec *c, const VoiceSettings *s)
{
    if (c && c->m_enc && s) {
        func_0024F518(c->m_enc, s->m_0);
        func_0024F568(c->m_enc, s->m_4);
        func_0024F5B8(c->m_enc, s->m_8);
    }
}

/* the encoder's echo canceller on (OVL1 0x310600) and off (0x3105D0) */
void func_00310600(VoiceCodec *c)
{
    if (c && c->m_enc)
        func_0024F6A8(c->m_enc);
}

void func_003105D0(VoiceCodec *c)
{
    if (c && c->m_enc)
        func_0024F6F8(c->m_enc);
}

/* the encoder's noise stage on with its two levels (OVL1 0x310590) and off (0x310560) */
void func_00310590(VoiceCodec *c, const VoiceSettings *s)
{
    if (c && c->m_enc && s)
        func_0024F658(c->m_enc, s->m_c, s->m_10);
}

void func_00310560(VoiceCodec *c)
{
    if (c && c->m_enc)
        func_0024F798(c->m_enc);
}

/* the encoder's gain stage on with its level (OVL1 0x310520) and off (0x3104F0) */
void func_00310520(VoiceCodec *c, const VoiceSettings *s)
{
    if (c && c->m_enc && s)
        func_0024F608(c->m_enc, s->m_14);
}

void func_003104F0(VoiceCodec *c)
{
    if (c && c->m_enc)
        func_0024F838(c->m_enc);
}

/* the canceller set up, the gain last (OVL1 0x310480: the voice object's set-up calls it) */
void func_00310480(VoiceEcho *e, int gain)
{
    if (e && e->m_handle) {
        func_00252880(e->m_handle, 30);
        func_00252920(e->m_handle, 150);
        func_00252B50(e->m_handle, 0);
        func_00252AB0(e->m_handle, gain);
    }
}

/* the canceller on or off (OVL1 0x310410) */
void func_00310410(VoiceEcho *e, int on)
{
    if (e && e->m_handle) {
        if (on) {
            func_002527F8(e->m_handle);
            func_00252A60(e->m_handle);
        } else {
            func_00252748(e->m_handle);
            func_00252A10(e->m_handle);
        }
    }
}

/* the canceller's level read (OVL1 0x3103D0; the record tick 0x30F990 calls it) */
int func_003103D0(VoiceEcho *e)
{
    short a;
    short b;

    func_002529C0(e->m_handle, &a);
    func_00252BA0(e->m_handle, &b);
    return b;
}
}
