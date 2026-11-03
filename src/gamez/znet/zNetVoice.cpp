/* zNetVoice.cpp -- the voice chat: the radio channels over the DME, the USB headset through
 * Logitech's lgAud library and the speech codec's echo canceller (SOCOM 1's zNetVoice.cpp held
 * CZNetVoice, a much smaller unit; SOCOM II's is OVL1 0x30E490-0x31097F, after zNetDme.cpp and
 * before node_vector.cpp). The classes are unnamed yet: the functions keep their placeholders and
 * the layouts are only what they read. Only some of it so far. Defined in reverse of their image
 * order (-inline deferred). */

#include <string.h>
#include "znet.h"
#include "../zui/zui.h"
#include "../zmath/zmath.h"

/* a valve (SOCOM 1's CValve: its inline setter, as every unit has it) */
enum VALVE_STATE {
    VALVE_SAME,
    VALVE_ON,
    VALVE_OFF
};

class CValve {
public:
    static CValve *GetByName(const char *name);
    void MakeCallbacks(VALVE_STATE state);

    /* the bool overload: the value promoted at each use (the talk flag's) */
    void SetValue(bool value)
    {
        VALVE_STATE state = VALVE_SAME;

        if (m_value != value)
            state = value == 0 ? VALVE_OFF : VALVE_ON;
        m_value = value;
        if (state != VALVE_SAME)
            MakeCallbacks(state);
    }

    char *m_name;   // 0x0
    unsigned int m_value : 16;   // 0x4
    unsigned int m_type : 8;
    unsigned int m_name_pooled : 1;
    unsigned int m_pooled : 1;
    unsigned int m_unused : 6;
};

/* the speech codec's echo canceller, a handle (the prebuilt library's, OVL1 0x2527F8-0x252BA0) */
struct VoiceEcho {
    void *m_handle;   // 0x0
};

extern "C" {
void func_002529C0(void *h, short *level);
void func_00252BA0(void *h, short *level);
void func_00252880(void *h, int v);
void func_00252920(void *h, int v);
void func_00252B50(void *h, int v);
void func_00252AB0(void *h, short v);
}

/* the radio's channel slots (only what is read here) */
struct VoiceSlot {
    int m_client;   // 0x0: 256, none
    int m_4;
};
struct VoiceRadio {
    unsigned char m_state;   // 0x0: 3, talking
    unsigned char _1[0x24 - 1];
    int m_slot;   // 0x24
    int m_channel;   // 0x28
    int m_team;   // 0x2c
    unsigned char _30[0x4a - 0x30];
    unsigned char m_quiet;   // 0x4a
    unsigned char _4b[0x60 - 0x4b];
    int m_radio_msg;   // 0x60: the "Radio" message
    int m_msg;   // 0x64: the radio's application message
    unsigned char _68[0x70 - 0x68];
    VoiceSlot m_slots[12];   // 0x70
};
struct VoiceTalk {
    unsigned char m_state;   // 0x0
    unsigned char _1[0x1074 - 1];
    CValve *m_valve;   // 0x1074
};

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

/* a client's radio channel as the DME has it */
struct VoiceChannelInfo {
    int m_0;
    int m_4;
    int m_on;   // 0x8
    int m_channel;   // 0xc
};

extern "C" {
void func_0024F888(void *h, VoiceFrame *f);
extern VoiceRadio *data_0045B718;   /* the radio */
int func_00623BA0(int conn, int client, VoiceChannelInfo *info);   /* (the DME: a client's channel) */
int func_00623998(int conn, int channel);   /* (the DME: our channel set) */
}

/* the headset's processing switches and the canceller's handle (only what is read here) */
struct VoiceHeadset {
    unsigned char _0[0x4e];
    unsigned char m_bypass;   // 0x4e: the processing off
    unsigned char m_noagc;   // 0x4f
    unsigned char m_nonoise;   // 0x50
    unsigned char _51[0x107c - 0x51];
    void *m_echo;   // 0x107c
    unsigned char _1080[4];
    unsigned char m_settings[4];   // 0x1084
};

extern "C" {
/* the echo canceller's switches (OVL1 0x3104F0-0x310630, byte-exact drafts) */
void func_003105D0(void *h);
void func_00310600(void *h);
void func_00310630(void *h, void *settings);
void func_003104F0(void *h);
void func_00310520(void *h, void *settings);
void func_00310560(void *h);
void func_00310590(void *h, void *settings);

/* an application message type: its fields described, then registered by name with a handler
 * (the DME's; func_0030CFF0 is zNetDme.cpp's wrapper, a draft) */
void func_00620BD8();
void func_00620C60(void *msg, void *field, int size, int count);
int func_0030CFF0(CZNetwork *net, int *id, int (*handler)(int, int, int, void *), const char *name);
int func_0030E8E0(int, int, int, void *);   /* the "Radio" message's handler */
int func_0030EB70(int, int, int, short *);
}

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

extern "C" {
void func_00255B68(void *h, void *frame);   /* (the decoder: a frame decoded) */
void func_00255CF8(void *h, unsigned char *done);   /* (and whether the packet is used up) */
}

/* the headset's playback: the device's handle and whether it is playing */
struct VoicePlayer {
    unsigned char _0[0x38];
    int m_handle;   // 0x38: -1, none
    unsigned char _3c[0x49 - 0x3c];
    unsigned char m_playing;   // 0x49
};

extern "C" {
int lgAudStartPlayback(int handle);   /* (lgAud: the playback started) */
int lgAudWrite(int handle, int mode, const void *buf, int *len);   /* (lgAud: written) */
}

/* the playback started once (the name ours) */
inline void StartPlayback(VoicePlayer *p)
{
    if (p->m_handle != -1 && !p->m_playing && !lgAudStartPlayback(p->m_handle))
        p->m_playing = 1;
}

/* the Logitech USB audio library's open parameters -- the mode, then the recording and the
 * playback formats, 14 bytes: the stack slot of the headset's open (0x30FEF0) matched only
 * without reco.cpp's two bytes of padding -- and the device list's entry */
struct lgAudFormat {
    unsigned char channels;   // 0x0
    unsigned char bits;   // 0x1
    unsigned short rate;   // 0x2
    unsigned short bufferms;   // 0x4
};
struct lgAudOpenParam {
    unsigned char mode;   // 0x0
    unsigned char _1;
    lgAudFormat rec;   // 0x2
    lgAudFormat play;   // 0x8
};   // 0xe
struct lgAudDeviceDesc {
    unsigned char _0[0x150];
};

/* the headset as a device: its state, its handle and volumes, the streams' switches, and each
 * stream's format (recording [0], playback [1]; only what is read here) */
struct VoiceDevice {
    unsigned char m_state;   // 0x0: 1, opened
    unsigned char _1[0x38 - 1];
    int m_handle;   // 0x38: -1, none
    int m_recvol;   // 0x3c
    int m_playvol;   // 0x40
    unsigned char _44[0x48 - 0x44];
    unsigned char m_48;   // 0x48
    unsigned char m_playing;   // 0x49
    unsigned char m_recording;   // 0x4a
    unsigned char _4b[0x10bc - 0x4b];
    int m_channels[2];   // 0x10bc
    int m_bits[2];   // 0x10c4
    int m_rate[2];   // 0x10cc
    int m_ms[2];   // 0x10d4
};

extern "C" {
int lgAudEnumerate(int index, lgAudDeviceDesc *desc);
int lgAudOpen(int index, lgAudOpenParam *param, int *handle);
int lgAudSetPlaybackVolume(int handle, int channel, unsigned char volume);
int lgAudSetRecordingVolume(int handle, int channel, unsigned char volume);
extern lgAudDeviceDesc data_0045B720;   /* the first device's description */
}

extern "C" {

/* the headset opened: the first device enumerated and opened in both directions with the
 * object's formats, the playback volume taken from the 'headset_controller' setting (0 to 1, as
 * a percentage held within 0-100), both volumes set (OVL1 0x30FEF0) */
int func_0030FEF0(VoiceDevice *d)
{
    d->m_recording = 0;
    d->m_48 = 0;
    d->m_playing = 0;
    d->m_state = 1;
    d->m_handle = -1;
    if (!lgAudEnumerate(0, &data_0045B720)) {
        lgAudOpenParam param;

        param.mode = 3;
        param.play.channels = d->m_channels[1];
        param.play.bits = d->m_bits[1];
        param.play.rate = d->m_rate[1];
        param.play.bufferms = d->m_ms[1];
        param.rec.channels = d->m_channels[0];
        param.rec.bits = d->m_bits[0];
        param.rec.rate = d->m_rate[0];
        param.rec.bufferms = d->m_ms[0];
        if (!lgAudOpen(0, &param, &d->m_handle)) {
            CUIVariable *var = theUIVarManager.Find("headset_controller");

            if (var && var->type == 2) {
                float v = 100.0f * var->Real();

                _saturate(&v, 0.0f, 100.0f);
                d->m_playvol = (int)v;
            }
            lgAudSetPlaybackVolume(d->m_handle, 0, d->m_playvol);
            lgAudSetRecordingVolume(d->m_handle, 0, d->m_recvol);
            return 1;
        }
        return 0;
    }
    return 0;
}

/* samples written to the headset 1 KB at a time, the bytes taken returned (OVL1 0x30F6E0) */
int func_0030F6E0(VoicePlayer *p, const unsigned char *buf, int len)
{
    int total = 0;
    int n = 0;

    if (p->m_handle == -1)
        return 0;
    if (!p->m_playing)
        StartPlayback(p);
    if (p->m_playing) {
        while (len > 0) {
            n = len;
            if (len > 1024)
                n = 1024;
            lgAudWrite(p->m_handle, 1, buf, &n);
            if (n <= 0)
                break;
            len -= n;
            buf += n;
            total += n;
        }
    }
    return total;
}

/* mono samples written as stereo, 256 at a time (OVL1 0x30F5F0). The samples are unsigned (lhu),
 * the halfword pair a chained assignment (the second stored first). Open since round 4 (2
 * instructions): the target copies the sample pointer after the loop's guard (`blez s3`), so the
 * guard is written out with the pointer and the count inside it and the loop a do-while -- a
 * plain while left MW its rotated guard below the pointer's copy */
int func_0030F5F0(VoicePlayer *p, const short *in, int len)
{
    unsigned char buf[1024] = {0};
    int total = 0;

    if (len > 0) {
        const unsigned short *s = (const unsigned short *)in;
        int done = 0;

        do {
            int i = 0;
            unsigned short *q = (unsigned short *)buf;

            while (i < 256 && len - done > 0) {
                q[0] = q[1] = *s;
                i++;
                done += 2;
                s++;
                q += 2;
            }
            total += func_0030F6E0(p, buf, i * 4);
        } while (len - done > 0);
    }
    return total;
}

/* a packet of 8 bytes decoded frame by frame into the caller's samples (OVL1 0x3106A0). The
 * count returned is the original's slip: it doubles itself (`addu s0,s0,s0`) where the frames'
 * counts were surely meant, so it stays 0 */
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

/* the radio's two messages registered: "Radio", three bytes, and "Channel Status", the twelve
 * slots' clients as halfwords (OVL1 0x310210) */
void func_00310210(VoiceRadio *r)
{
    unsigned char radio[3];

    func_00620BD8();
    func_00620C60(radio, &radio[0], 1, 1);
    func_00620C60(radio, &radio[1], 1, 1);
    func_00620C60(radio, &radio[2], 1, 1);
    func_0030CFF0(&theNetwork, &r->m_radio_msg, func_0030E8E0, "Radio");
    short status[12];

    func_00620BD8();
    func_00620C60(status, status, 2, 12);
    func_0030CFF0(&theNetwork, &r->m_msg, (int (*)(int, int, int, void *))func_0030EB70,
                  "Channel Status");
    r->m_state = 1;
}

/* the headset's processing switched on or off: when it changes, the echo canceller's, gain's and
 * noise stages follow, each kept on where its own flag says (OVL1 0x30E5A0) */
void func_0030E5A0(VoiceHeadset *h, bool bypass)
{
    if (bypass == h->m_bypass) {
        if (bypass)
            func_003105D0(h->m_echo);
        else {
            func_00310600(h->m_echo);
            func_00310630(h->m_echo, h->m_settings);
        }
        bool agc = bypass || h->m_noagc;

        if (agc)
            func_003104F0(h->m_echo);
        else
            func_00310520(h->m_echo, h->m_settings);
        bool noise = bypass || h->m_nonoise;

        if (noise)
            func_00310560(h->m_echo);
        else
            func_00310590(h->m_echo, h->m_settings);
        h->m_bypass = !bypass;
    }
}

/* our channel set to a team's (the team and the squad ORed), unless the DME has us on it already
 * (OVL1 0x30FD90's twin; OVL1 0x30FCD0) */
int func_0030FCD0(VoiceRadio *r, int team, int squad)
{
    r->m_team = team;
    int conn = theNetwork.m_GameConnectionIndex;
    VoiceChannelInfo info;

    func_00623BA0(conn, theNetwork.zNetGetMyGameClientIndex(), &info);
    int channel = team | squad;

    if ((info.m_channel != channel && info.m_on) || !info.m_on) {
        r->m_quiet = 0;
        if (!func_00623998(theNetwork.m_GameConnectionIndex, channel))
            r->m_slot = r->m_channel = channel;
    }
    return channel;
}

/* our channel set, unless the DME has us on it already (OVL1 0x30FD90) */
int func_0030FD90(VoiceRadio *r, int channel)
{
    int conn = theNetwork.m_GameConnectionIndex;
    VoiceChannelInfo info;

    func_00623BA0(conn, theNetwork.zNetGetMyGameClientIndex(), &info);
    if ((info.m_channel != channel && info.m_on) || !info.m_on) {
        r->m_quiet = 0;
        if (!func_00623998(theNetwork.m_GameConnectionIndex, channel))
            r->m_slot = r->m_channel = channel;
    }
    return channel;
}

/* the master's channel slots cleared and sent to everyone (OVL1 0x30E7F0) */
void func_0030E7F0(VoiceRadio *r)
{
    if (theNetwork.m_bSessionMaster) {
        for (int i = 0; i < 12; i++) {
            r->m_slots[i].m_client = 256;
            r->m_slots[i].m_4 = 0;
        }
        short data[12];

        for (int j = 0; j < 12; j++)
            data[j] = r->m_slots[j].m_client;
        theNetwork.zNetSendApplicationMessage(64, theNetwork.m_GameConnectionIndex, -1, r->m_msg, 24,
                                              (unsigned char *)data, false);
    }
}

/* our channel set to the all-hands one, 11 (OVL1 0x30E490) */
void func_0030E490(VoiceRadio *r)
{
    int conn = theNetwork.m_GameConnectionIndex;
    VoiceChannelInfo info;

    func_00623BA0(conn, theNetwork.zNetGetMyGameClientIndex(), &info);
    if ((info.m_channel != 11 && info.m_on) || !info.m_on) {
        r->m_quiet = 0;
        if (!func_00623998(theNetwork.m_GameConnectionIndex, 11))
            r->m_slot = r->m_channel = 11;
    }
    r->m_quiet = 0;
}

/* a frame of samples encoded: the packed bytes handed out and their count returned, a carried
 * byte moved to the front (OVL1 0x310750) */
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

/* the channel status packet: the twelve slots' clients, the bytes read returned (OVL1 0x30EB70:
 * registered as "Channel Status" with the radio's messages) */
int func_0030EB70(int, int, int, short *data)
{
    for (int i = 0; i < 12; i++)
        data_0045B718->m_slots[i].m_client = data[i];
    return 24;
}

/* the canceller's level read (OVL1 0x3103D0) */
short func_003103D0(VoiceEcho *e)
{
    short a;
    short b;

    func_002529C0(e->m_handle, &a);
    func_00252BA0(e->m_handle, &b);
    return b;
}

/* the canceller set up, the gain last (OVL1 0x310480: the voice object's set-up, its one caller,
 * calls it out of line, pinned) */
#pragma auto_inline off
void func_00310480(VoiceEcho *e, int gain)
{
    if (e && e->m_handle) {
        func_00252880(e->m_handle, 30);
        func_00252920(e->m_handle, 150);
        func_00252B50(e->m_handle, 0);
        func_00252AB0(e->m_handle, gain);
    }
}
#pragma auto_inline reset

}

/* the speech codec's pair: the encoder's and the decoder's handles, each with its frame -- the
 * samples, their count, the packed bytes and their room (56 bytes: VoiceEcho, VoiceEncoder and
 * VoiceDecoder are its views) */
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

/* what the codec says of its frames (only what is read here) */
struct CodecInfo {
    unsigned char _0[0x3c];
    unsigned short m_samples;   // 0x3c: a frame's samples
    unsigned char _3e[0x60 - 0x3e];
    unsigned long m_bits;   // 0x60: a packed frame's bits
    unsigned char _68[0x70 - 0x68];
};

extern "C" {
void func_00256F48(void *(*alloc)(unsigned int n, unsigned int size));   /* (the codec's allocator set) */
void func_00256F68(void (*release)(void *p));   /* (and its release) */
void func_00255A68(CodecInfo *info);   /* (the codec described) */
void func_00255BD0(void **dec);   /* (the decoder made) */
void func_0024F990(void **enc);   /* (the encoder made) */
void func_00310630(void *h, void *settings);   /* (the canceller's settings) */
void func_00310410(VoiceEcho *e, int on);   /* (the canceller on or off) */
void *calloc(unsigned int n, unsigned int size);
void free(void *p);
void *malloc(unsigned int n);

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
}

/* the voice object itself (0x45B718's: the radio's slots, the headset's switches and volumes,
 * the codec, the mute table; only what its set-up writes) */
struct VoiceChat {
    unsigned char m_state;   // 0x0
    unsigned char _1[0x30 - 1];
    int m_port;   // 0x30
    int m_port2;   // 0x34: the port + 5000
    unsigned char _38[0x3c - 0x38];
    int m_recvol;   // 0x3c
    int m_playvol;   // 0x40
    int m_44;   // 0x44
    unsigned char _48[0x4b - 0x48];
    unsigned char m_4b;   // 0x4b
    unsigned char m_open;   // 0x4c: the set-up done
    unsigned char _4d;
    unsigned char m_bypass;   // 0x4e
    unsigned char m_noagc;   // 0x4f
    unsigned char m_nonoise;   // 0x50
    unsigned char _51[0x5c - 0x51];
    int m_5c;   // 0x5c
    unsigned char _60[0x70 - 0x60];
    VoiceSlot m_slots[12];   // 0x70
    unsigned char _d0[0x1074 - 0xd0];
    CValve *m_valve;   // 0x1074: 'can_talk'
    int m_usecodec;   // 0x1078
    VoiceCodec *m_codec;   // 0x107c
    unsigned char _1080[4];
    float m_settings[6];   // 0x1084: the canceller's
    int m_109c;   // 0x109c
    int m_gain;   // 0x10a0
    unsigned char m_mute[24];   // 0x10a4: a client's voice not played

    /* a client muted or not, when it is one (ours) */
    void SetMute(int i, bool on)
    {
        if (i >= 0 && i < 24)
            m_mute[i] = on;
    }
    /* the canceller's gain set (ours) */
    void SetGain(int gain)
    {
        m_gain = gain;
        func_00310480((VoiceEcho *)m_codec, gain);
    }
};

extern "C" {

/* the voice object set up for a game on a port: the twelve slots free, the volumes and switches
 * at their defaults, nobody muted, the talk valve off, the codec made with the canceller's
 * settings cleared when it is wanted, then the headset opened (OVL1 0x310080: DMEJoinGameServer
 * calls it on 0x45B718) */
void func_00310080(VoiceChat *v, int port)
{
    for (int i = 0; i < 12; i++) {
        v->m_slots[i].m_client = 256;
        v->m_slots[i].m_4 = 0;
    }
    v->m_109c = -1;
    v->m_44 = 2;
    v->m_4b = 1;
    v->m_recvol = 50;
    v->m_playvol = 60;
    v->m_port = port;
    v->m_port2 = port + 5000;
    v->m_5c = 0;
    v->m_nonoise = 0;
    v->m_noagc = 0;
    v->m_usecodec = 1;
    for (int j = 0; j < 24; j++)
        v->SetMute(j, false);
    v->m_valve = CValve::GetByName("can_talk");
    if (v->m_valve)
        v->m_valve->SetValue(false);
    if (v->m_usecodec == 1) {
        v->m_codec = 0;
        v->m_codec = func_00310830();
        for (int k = 0; k < 6; k++)
            v->m_settings[k] = 0;
        func_00310630(v->m_codec, v->m_settings);
        func_00310410((VoiceEcho *)v->m_codec, 0);
        v->SetGain(v->m_gain);
        v->m_bypass = 1;
    }
    func_0030FEF0((VoiceDevice *)v);
    v->m_open = 1;
}

/* whether the current slot holds another client (OVL1 0x30E790) */
bool func_0030E790(VoiceRadio *r)
{
    return r->m_slots[r->m_slot].m_client != 256 &&
           r->m_slots[r->m_slot].m_client != theNetwork.zNetGetMyGameClientIndex();
}

/* talking, told to the valve (OVL1 0x30E710) */
bool func_0030E710(VoiceTalk *t)
{
    bool talking = t->m_state == 3;

    if (t->m_valve)
        t->m_valve->SetValue(talking);
    return talking;
}

}
