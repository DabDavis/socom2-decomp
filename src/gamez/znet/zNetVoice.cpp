/* zNetVoice.cpp -- the voice chat: the radio channels over the DME, the USB headset through
 * Logitech's lgAud library and the speech codec's echo canceller (SOCOM 1's zNetVoice.cpp held
 * CZNetVoice, a much smaller unit; SOCOM II's is OVL1 0x30E490-0x31097F, after zNetDme.cpp and
 * before node_vector.cpp). The classes are unnamed yet: the functions keep their placeholders and
 * the layouts are only what they read. Only some of it so far. Defined in reverse of their image
 * order (-inline deferred). */

#include <string.h>
#include "znet.h"

/* a valve (SOCOM 1's CValve: its inline setter, as every unit has it) */
enum VALVE_STATE {
    VALVE_SAME,
    VALVE_ON,
    VALVE_OFF
};

class CValve {
public:
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

extern "C" {

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

/* the canceller set up, the gain last (OVL1 0x310480) */
void func_00310480(VoiceEcho *e, int gain)
{
    if (e && e->m_handle) {
        func_00252880(e->m_handle, 30);
        func_00252920(e->m_handle, 150);
        func_00252B50(e->m_handle, 0);
        func_00252AB0(e->m_handle, gain);
    }
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
