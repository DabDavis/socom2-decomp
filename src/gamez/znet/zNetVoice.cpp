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
    unsigned int m_4;   // 0x4: the time it is held until (the master's tick compares it unsigned)
};
/* the radio -- the voice object fts_mission.cpp's CMission::Init makes (0x10DC bytes, 0x3102E0
 * its constructor) and keeps at data_0045B718. SOCOM 1's zNetVoice.cpp held CZNetVoice, so the
 * class keeps that name (a prediction: SOCOM II's object is new); VoiceRadio is this unit's
 * older spelling, kept as a typedef for the placeholders */
struct CZNetVoice {
    /* the radio's two message types registered with the network (0x310210) */
    void zNetRegisterVoice();

    unsigned char m_state;   // 0x0: 1 idle, 2 a slot asked for, 3 talking, 4 closing
    unsigned char _1[0x24 - 1];
    int m_slot;   // 0x24
    int m_channel;   // 0x28
    int m_team;   // 0x2c
    int m_talklen;   // 0x30: how long a talk may last (VoiceChat's m_port: the set-up's argument)
    int m_talktime;   // 0x34: how long a slot is held from the time it is given (VoiceChat's
                      //   m_port2: the set-up's argument + 5000)
    int m_handle;   // 0x38: the headset's (VoiceDevice's), -1 none
    unsigned char _3c[0x44 - 0x3c];
    int m_hint;   // 0x44: the headset's enumeration hint (1 a change seen, 2 handled)
    unsigned char m_recording;   // 0x48: the headset recording (VoiceDevice's m_48)
    unsigned char _49;
    unsigned char m_quiet;   // 0x4a: set, the talk key served; cleared (a channel change), the slot given back
    unsigned char _4b;
    unsigned char m_open;   // 0x4c: the set-up done (VoiceChat's)
    unsigned char _4d[0x54 - 0x4d];
    unsigned char m_status[8];   // 0x54: our channel's state, as the DME reports it
    int m_5c;   // 0x5c
    /* the two message ids the network hands back at registration, unsigned (the radio tick's sends
     * load them ahead of the connection only so: the online lane's round 22) */
    unsigned int m_radio_msg;   // 0x60: the "Radio" message
    unsigned int m_msg;   // 0x64: the radio's application message
    unsigned int m_retry;   // 0x68: when a slot is asked for again
    unsigned int m_until;   // 0x6c: when the talk (or its closing) ends
    VoiceSlot m_slots[12];   // 0x70
    unsigned char *m_tone;   // 0xd0: the open and close tone's samples (data_003DF540 at rest)
    short m_tonebuf[2000];   // 0xd4: the tone as the headset plays it
    unsigned char _1074[0x1078 - 0x1074];
    int m_usecodec;   // 0x1078 (VoiceChat's)
};
typedef CZNetVoice VoiceRadio;
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
 * (the DME's; CZNetwork::zNetRegisterMessage, zNetDme.cpp's, registers it) */
void func_00620BD8();
void func_00620C60(void *msg, void *field, int size, int count);
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
int lgAudRead(int handle, int mode, void *buf, int *len);   /* (lgAud: read) */
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

    /* opened (a const view: the read's test and its call load the handle each, no address shared) */
    bool IsOpen() const { return m_handle != -1; }
    int Read(void *buf, int len);
    void SetRecordingVolume(int vol);
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
 * counts were surely meant, so it stays 0. The packet handler 0x30F7E0, its one caller, calls it
 * out of line: pinned */
#pragma auto_inline off
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
#pragma auto_inline reset

/* the radio's two messages registered: "Radio", three bytes, and "Channel Status", the twelve
 * slots' clients as halfwords (OVL1 0x310210) */
void CZNetVoice::zNetRegisterVoice()
{
    unsigned char radio[3];

    func_00620BD8();
    func_00620C60(radio, &radio[0], 1, 1);
    func_00620C60(radio, &radio[1], 1, 1);
    func_00620C60(radio, &radio[2], 1, 1);
    theNetwork.zNetRegisterMessage(&m_radio_msg, func_0030E8E0, "Radio");
    short status[12];

    func_00620BD8();
    func_00620C60(status, status, 2, 12);
    theNetwork.zNetRegisterMessage(&m_msg, (NetMessageCallback)func_0030EB70, "Channel Status");
    m_state = 1;
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
 * byte moved to the front (OVL1 0x310750; pinned: its one caller, 0x30F990, calls it) */
#pragma auto_inline off
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
#pragma auto_inline reset

/* the channel status sent to everyone: the twelve slots' clients as halfwords (inlined in the
 * "Radio" handler's two master cases; the name ours) */
inline void SendChannelStatus(VoiceRadio *r)
{
    short data[12];

    for (int i = 0; i < 12; i++)
        data[i] = r->m_slots[i].m_client;
    theNetwork.zNetSendApplicationMessage(64, theNetwork.m_GameConnectionIndex, -1, r->m_msg, 24,
                                          (unsigned char *)data, false);
}

extern "C" {
int func_00623AE8(int conn, unsigned int slot, void *status);   /* (the DME: our channel's state read;
                                                                 * the slot unsigned: the radio tick loads it
                                                                 * ahead of the connection only so) */
int func_00623D00(int conn, int *talk);   /* (the DME: the talk key held) */
int func_00623C58(int conn, int codec);   /* (the DME: the talk over) */
int lgAudEnumHint(int *hint);   /* (lgAud: the devices' change hint, and its async form) */
int lgAudAEnumHint(int *hint);
int lgAudASync(int wait);
int lgAudClose(int handle);
int lgAudStartRecording(int handle);
int lgAudStopRecording(int handle);
extern unsigned char data_003DF540[];   /* the radio's open and close tone, at rest */
extern int data_0045C290;   /* the ticks without a headset */
}

/* our channel's state as the DME has it (the name ours) */
inline int GetChannelState(int slot, void *status)
{
    return func_00623AE8(theNetwork.m_GameConnectionIndex, slot, status);
}

/* our request to the session master as a "Radio" message (us, the slot, 1 asked for or 2 given
 * back): handled here when we are the master or there is none, sent to the master when it is
 * another (inlined four times in the radio's tick, a message of its own each; the name ours) */
inline void SendRadioRequest(VoiceRadio *r, unsigned char req)
{
    int master = theNetwork.zNetGetGameSessionMasterClientIndex();
    int slot = r->m_slot;
    unsigned char msg[3];

    msg[0] = theNetwork.zNetGetMyGameClientIndex();
    msg[1] = slot;
    msg[2] = req;
    if (master == theNetwork.zNetGetMyGameClientIndex() || master == -1)
        func_0030E8E0(0, 0, master, msg);
    if (master != theNetwork.zNetGetMyGameClientIndex())
        theNetwork.zNetSendApplicationMessage(64, theNetwork.m_GameConnectionIndex, master, r->m_radio_msg, 3, msg,
                                              false);
}

/* the slot we hold given back (the name ours) */
inline void SendRadioRelease(VoiceRadio *r, int slot)
{
    int master = theNetwork.zNetGetGameSessionMasterClientIndex();
    unsigned char msg[3];

    msg[0] = theNetwork.zNetGetMyGameClientIndex();
    msg[1] = slot;
    msg[2] = 2;
    if (master == theNetwork.zNetGetMyGameClientIndex() || master == -1)
        func_0030E8E0(0, 0, master, msg);
    if (master != theNetwork.zNetGetMyGameClientIndex())
        theNetwork.zNetSendApplicationMessage(64, theNetwork.m_GameConnectionIndex, master, r->m_radio_msg, 3, msg,
                                              false);
}

/* one of the slots ours (the name ours) */
inline bool HoldsSlot(VoiceRadio *r)
{
    for (int i = 0; i < 12; i++)
        if (r->m_slots[i].m_client == theNetwork.zNetGetMyGameClientIndex())
            return true;
    return false;
}

/* and which (the name ours) */
inline bool FindSlot(VoiceRadio *r, int &slot)
{
    for (int i = 0; i < 12; i++)
        if (r->m_slots[i].m_client == theNetwork.zNetGetMyGameClientIndex()) {
            slot = i;
            return true;
        }
    return false;
}

/* a slot held by another client (the name ours). The slot reached as the radio stepped by i
 * slots, its client then at +0x70: the target keeps 8i + r across the call and reads 0x70(s0)
 * twice (a stand-in spelling -- `r->m_slots[i]` read twice keeps the whole address instead). The
 * step is integer arithmetic with the SHIFT WRITTEN FIRST, `(i << 3) + (unsigned int)r` (the
 * target's `addu s0,v0,s3`, online round 23; README "`(n << 4) + (unsigned int)p`"): `i * 8`
 * first, `(unsigned int)r + (i << 3)`, char * or VoiceSlot * steps, `&((VoiceSlot *)r)[i]` keep
 * r first; the shift or the base in a local of its own match too */
inline bool SlotTaken(VoiceRadio *r, int i)
{
    VoiceRadio *p = (VoiceRadio *)((i << 3) + (unsigned int)r);
    bool b = p->m_slots[0].m_client != 256;

    if (b)
        b = p->m_slots[0].m_client != theNetwork.zNetGetMyGameClientIndex();
    return b;
}

/* the headset's recording started, 0 when it is (the name ours) */
inline int StartRecording(VoiceRadio *r)
{
    if (r->m_handle == -1)
        return 0x80000000;
    if (r->m_recording == 1)
        return 0;
    int err = lgAudStartRecording(r->m_handle);

    if (err == 0)
        r->m_recording = 1;
    else
        err = 0x80000000;
    return err;
}

/* and stopped (the name ours) */
inline void StopRecording(VoiceRadio *r)
{
    if (r->m_recording && r->m_handle != -1) {
        lgAudStopRecording(r->m_handle);
        r->m_recording = 0;
    }
}

/* the radio's tick: the headset looked for again when the devices change (closed and opened
 * afresh); while it is there, the talk key asks the session master for our channel's slot (1),
 * a slot given plays the opening tone and starts the recording (2), the talk ends when the slot
 * goes or its time is up -- the closing tone, the slot given back (3) -- and the radio rests a
 * while (4); a channel change gives the slot back at once. The master frees the slots whose time
 * is up, telling everyone. The state returned (OVL1 0x30EC10; 0x30F990 ticks it).
 * Byte-exact since the online lane's round 23 (open since round 19): the last instruction was
 * SlotTaken's step, `(i << 3) + (unsigned int)r` (its comment). Round 22, 13 -> 1: THE TWELVE LOAD SWAPS WERE
 * SIGNEDNESS -- an argument loaded as one type and passed as the other is loaded ahead of the
 * connection global (the target's order), one passed as its own type after it: the radio's two
 * message ids are `unsigned int` (zNetSendApplicationMessage takes an int; the five sends right) and
 * the DME's channel read takes the slot `unsigned int` (func_00623AE8's pair right). Found by
 * `*(volatile int *)&r->m_radio_msg` and `*(unsigned int *)&...` each fixing the sends where a
 * statement order (the id and the conn in locals, either order) changed nothing. Left: SlotTaken's
 * `addu s0,v0,s3` (8i + r) against our r + 8i -- the stand-in below in every spelling (the step by
 * pointer or integer arithmetic either way round, an unsigned index, a slot local) keeps r first; read
 * as `r->m_slots[i]` (a reference, a pointer, the decayed array) it keeps the whole address (0x9E8).
 * The rounds before, for the record:
 * (round 19, first written: every block, call and frame slot the target's,
 * r in s3, slot s2, err s1): (1) the five sends load the connection (a2) ahead of the message id
 * (t0), and func_00623AE8's a0 ahead of a1, where the target loads the other argument first (10 + 2)
 * -- the same SendChannelStatus gives the target's order inside func_0030E8E0; an accessor, a
 * wrapper inline, change nothing; (2) SlotTaken's step adds r + 8i where the target adds 8i + r (1:
 * every spelling of the stand-in alike). SlotTaken itself: `r->m_slots[i]` read twice keeps the
 * whole address (r + 8i + 0x70, the extra addiu and an alignment nop: 0x9E8), a getter CSEs the
 * value across the call, by reference, const views, a VoiceSlot accessor, a member: the same; only
 * the stepped radio (`(VoiceRadio *)(i + (VoiceSlot *)r)`, its slot 0) keeps r + 8i. Round 20 measured
 * (1) on probes (build/scratch/onl/p1-p16.cpp, the lane's worktree): the two loads tie in MW's scheduler
 * and the tie goes to the global's load (the IR's left-to-right order) unless the radio's load ends the
 * radio's live range -- SendRadioRequest standalone (r dead after the send), a send followed by `return
 * 1`, or r reassigned after it (`r = global;`), loads t0 first as the target; with r read after the block (`return r->x`, `g2(r)`) the conn goes
 * first whatever the spelling (r copied into a local, an inline's parameter, a reference, a base-class
 * or member-object inline, a cast, a void * parameter, an inline returning r, the id in a local, a
 * volatile conn: MW propagates every copy before scheduling). func_00623AE8's pair flips another way:
 * after `if (!r->m_open) return 0;` straight ahead of it the conn goes first (every spelling of the
 * test, an inline IsOpen, an else arm, a dead local, an empty inline between), with a call between
 * (`g();`) or the call in an if block (`if (r->m_open) { ... }`, the target's block layout lost) the
 * slot goes first. So the target's radio pointer dies at each send -- a second variable holding it, or
 * a different body around the sends; not found. Settled on the
 * way: the clocks compared `GetLocalTime() > field` (the call first, the field loaded after it),
 * the master asked for inside the request before the slot is read, the tone loops `*s++` / `*s--`
 * (MW unrolls them in pairs, the target's), StartRecording's result kept, its zero arm first */
unsigned char func_0030EC10(VoiceRadio *r)
{
    if (!r->m_open)
        return 0;
    GetChannelState(r->m_slot, r->m_status);
    int slot = 0;
    int err = 0;

    if (r->m_recording || SlotTaken(r, r->m_slot))
        err = lgAudEnumHint(&r->m_hint);
    else {
        lgAudASync(0);
        if (r->m_hint != 1)
            err = lgAudAEnumHint(&r->m_hint);
    }
    if (!err && r->m_hint == 1) {
        r->m_hint = 2;
        if (r->m_handle != -1)
            lgAudClose(r->m_handle);
        func_0030FEF0((VoiceDevice *)r);
    }
    if (r->m_handle != -1) {
        data_0045C290 = 0;
        if (r->m_quiet) {
            int talk;

            func_00623D00(theNetwork.m_GameConnectionIndex, &talk);
            if (r->m_state == 1 && talk) {
                r->m_state = 2;
                SendRadioRequest(r, 1);
                r->m_retry = theNetwork.zNetGetLocalTime() + 1000;
            } else if (r->m_state == 2) {
                if (HoldsSlot(r)) {
                    unsigned char *s = r->m_tone;

                    for (int i = 0; i < 2000; i++)
                        r->m_tonebuf[i] = *s++ << 8;
                    func_0030F5F0((VoicePlayer *)r, r->m_tonebuf, 4000);
                    if (!StartRecording(r)) {
                        r->m_state = 3;
                        r->m_until = r->m_talklen + theNetwork.zNetGetLocalTime();
                    }
                } else if (theNetwork.zNetGetLocalTime() > r->m_retry) {
                    SendRadioRequest(r, 1);
                    r->m_retry = theNetwork.zNetGetLocalTime() + 1000;
                }
            } else if (r->m_state == 3) {
                if (!HoldsSlot(r) || theNetwork.zNetGetLocalTime() > r->m_until) {
                    func_00623C58(theNetwork.m_GameConnectionIndex, r->m_usecodec);
                    StopRecording(r);
                    unsigned char *s = r->m_tone + 2000;

                    for (int i = 0; i < 2000; i++)
                        r->m_tonebuf[i] = *s-- << 8;
                    func_0030F5F0((VoicePlayer *)r, r->m_tonebuf, 4000);
                    r->m_tone = data_003DF540;
                    SendRadioRequest(r, 2);
                    r->m_until = r->m_talklen + theNetwork.zNetGetLocalTime();
                    r->m_state = 4;
                }
            } else if (r->m_state == 4 && theNetwork.zNetGetLocalTime() > r->m_until)
                r->m_state = 1;
        } else {
            if (FindSlot(r, slot)) {
                SendRadioRelease(r, slot);
                r->m_slots[slot].m_client = 256;
            }
            if (r->m_state == 3) {
                func_00623C58(theNetwork.m_GameConnectionIndex, r->m_usecodec);
                StopRecording(r);
                unsigned char *s = r->m_tone + 2000;

                for (int i = 0; i < 2000; i++)
                    r->m_tonebuf[i] = *s-- << 8;
                func_0030F5F0((VoicePlayer *)r, r->m_tonebuf, 4000);
                r->m_tone = data_003DF540;
                r->m_state = 1;
            } else if (r->m_state == 2)
                r->m_state = 1;
        }
    } else
        data_0045C290++;
    if (theNetwork.m_bSessionMaster) {
        for (int i = 0; i < 12; i++) {
            if (r->m_slots[i].m_client != 256 && theNetwork.zNetGetTime() > r->m_slots[i].m_4) {
                r->m_slots[i].m_client = 256;
                r->m_slots[i].m_4 = 0;
                SendChannelStatus(r);
            }
        }
    }
    return r->m_state;
}

/* the channel status packet: the twelve slots' clients, the bytes read returned (OVL1 0x30EB70:
 * registered as "Channel Status" with the radio's messages) */
int func_0030EB70(int, int, int, short *data)
{
    for (int i = 0; i < 12; i++)
        data_0045B718->m_slots[i].m_client = data[i];
    return 24;
}

/* a "Radio" message (three bytes: a client, a slot, the request): the session master gives a free
 * slot to the client asking for it (1) and takes one back from the client holding it (2), telling
 * everyone the channel status each time; a client is told a slot is given (3) or free again (4).
 * A slot given is held for the radio's talk time from now; the bytes read returned (OVL1 0x30E8E0,
 * registered as "Radio" by 0x310210) */
int func_0030E8E0(int, int, int client, void *data)
{
    VoiceRadio *r;
    unsigned char *msg = (unsigned char *)data;

    switch (msg[2]) {
    case 1:
        if (theNetwork.m_bSessionMaster && data_0045B718->m_slots[msg[1]].m_client == 256) {
            data_0045B718->m_slots[msg[1]].m_client = client;
            r = data_0045B718;
            r->m_slots[msg[1]].m_4 = r->m_talktime + theNetwork.zNetGetTime();
            SendChannelStatus(data_0045B718);
        }
        break;
    case 3:
        data_0045B718->m_slots[msg[1]].m_client = msg[0];
        r = data_0045B718;
        r->m_slots[msg[1]].m_4 = r->m_talktime + theNetwork.zNetGetTime();
        break;
    case 2:
        if (theNetwork.m_bSessionMaster && data_0045B718->m_slots[msg[1]].m_client == msg[0]) {
            data_0045B718->m_slots[msg[1]].m_client = 256;
            SendChannelStatus(data_0045B718);
        }
        break;
    case 4:
        data_0045B718->m_slots[msg[1]].m_client = 256;
        break;
    }
    return 3;
}

/* the canceller's level read (OVL1 0x3103D0; pinned: its one caller, 0x30F990, calls it) */
#pragma auto_inline off
int func_003103D0(VoiceEcho *e)
{
    short a;
    short b;

    func_002529C0(e->m_handle, &a);
    func_00252BA0(e->m_handle, &b);
    return b;
}
#pragma auto_inline reset

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
 * the codec, the mute table; only what its set-up and the packet handler touch) */
struct VoiceChat {
    unsigned char m_state;   // 0x0
    unsigned char m_hearing;   // 0x1: a packet of our channel being played
    unsigned char _2[0x8 - 0x2];
    int m_talker;   // 0x8: the client last heard
    unsigned char _c[0x24 - 0xc];
    int m_slot;   // 0x24: our channel
    unsigned char _28[0x30 - 0x28];
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
    bool m_mute[24];   // 0x10a4: a client's voice not played

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
    /* a client muted, or no client (ours) */
    bool IsMuted(int i)
    {
        if (i >= 0 && i < 24)
            return m_mute[i];
        return true;
    }
};

/* a voice packet as the DME hands it over: raw samples or the codec's 8-byte frames, the bytes
 * taken handed back (the names ours) */
struct VoicePacket {
    int m_type;   // 0x0: 1, the codec's frames
    int m_client;   // 0x4
    const unsigned char *m_data;   // 0x8
    unsigned int m_length;   // 0xc
    int m_taken;   // 0x10
};

/* the voice object, read through its global at each use (the target reloads it after every
 * store and call) */
inline VoiceChat *Voice() { return (VoiceChat *)data_0045B718; }

extern "C" {
extern int data_0045BD80;   /* the decoded samples' bytes waiting */
extern unsigned char data_0045BD90[];   /* and the samples */
int func_0030F5F0(VoicePlayer *p, const short *in, int len);
int func_003106A0(VoiceDecoder *d, const void *packet, short *out);

/* a buffer of our voice to send: the format (the object's m_usecodec), the bytes and their room,
 * and the bytes handed back (the names ours) */
struct VoiceRecord {
    int m_type;   // 0x0: 1, the codec's frames
    unsigned char *m_buf;   // 0x4
    int m_len;   // 0x8
    int m_got;   // 0xc
};

/* the headset's samples read into a buffer, the bytes read returned (none with no device or when
 * it is not recording; the name ours) */
inline int VoiceDevice::Read(void *buf, int len)
{
    int n = 0;

    if (!IsOpen())
        return 0;
    if (m_48 && len > 0 && buf) {
        n = len;
        if (lgAudRead(m_handle, 1, buf, &n))
            n = 0;
    }
    return n;
}

/* the recording volume set, held within 20-100: a member, `this` taken after the argument (the
 * name ours) */
inline void VoiceDevice::SetRecordingVolume(int vol)
{
    if (vol < 20)
        vol = 20;
    if (vol > 100)
        vol = 100;
    lgAudSetRecordingVolume(m_handle, 0, vol);
    m_recvol = vol;
}

extern "C" {
unsigned char func_0030EC10(VoiceRadio *r);   /* (the radio's own tick) */
int func_00310750(VoiceEncoder *e, const void *samples, void *out);
int func_003103D0(VoiceEcho *e);
extern int data_0045B870;   /* the frames sent, counted */
extern int data_0045B878;   /* the samples' bytes gathered */
extern unsigned char data_0045B880[];   /* and the samples: four frames of 320 bytes */

/* our voice read for sending: the radio ticked first; when the radio is talking, the codec's way
 * gathers four frames of samples (1280 bytes) and encodes them into the buffer, the recording
 * level turned down or up a step every fifth time the canceller hears it too loud or too soft;
 * raw samples are read straight into the buffer (OVL1 0x30F990) */
void func_0030F990(VoiceRecord *rec)
{
    if (rec) {
        rec->m_got = 0;
        rec->m_type = Voice()->m_usecodec;
        func_0030EC10((VoiceRadio *)Voice());
        if (Voice()->m_state == 3) {
            if (rec->m_type == 1) {
                if (Voice()->m_codec) {
                    int n = 0;
                    unsigned char *out = rec->m_buf;

                    if (data_0045B878 < 1280)
                        data_0045B878 += ((VoiceDevice *)Voice())->Read(data_0045B880 + data_0045B878,
                                                                        1280 - data_0045B878);
                    if (data_0045B878 >= 1280) {
                        for (int i = 0; i < 4; i++)
                            n += func_00310750((VoiceEncoder *)Voice()->m_codec, data_0045B880 + i * 320,
                                               out + n);
                        data_0045B878 = 0;
                        if (data_0045B870++ % 5 == 0) {
                            int level = func_003103D0((VoiceEcho *)Voice()->m_codec);

                            /* the new volume a local of the caller: numbered ahead of the inlined
                             * setter's `this`, it takes the lower saved register */
                            if (level < 0) {
                                int vol = Voice()->m_recvol - 5;

                                ((VoiceDevice *)Voice())->SetRecordingVolume(vol);
                            } else if (level > 0) {
                                int vol = Voice()->m_recvol + 5;

                                ((VoiceDevice *)Voice())->SetRecordingVolume(vol);
                            }
                        }
                        rec->m_got = n;
                    }
                }
            } else
                rec->m_got = ((VoiceDevice *)Voice())->Read(rec->m_buf, rec->m_len);
        }
    }
}
}

/* a voice packet heard: from a client on our channel and not muted, the codec's frames decoded
 * and played one by one, raw samples played as they are; the talker kept (OVL1 0x30F7E0, the
 * callback 0x30FC70 registers) */
void func_0030F7E0(VoicePacket *p)
{
    if (p && p->m_length) {
        VoiceChannelInfo info;

        if (func_00623BA0(theNetwork.m_GameConnectionIndex, p->m_client, &info) ||
            info.m_channel != Voice()->m_slot)
            return;
        Voice()->m_hearing = 1;
        if (Voice()->IsMuted(p->m_client)) {
            p->m_taken = p->m_length;
            return;
        }
        if (p->m_type == 1) {
            if (!Voice()->m_codec) {
                Voice()->m_hearing = 0;
                return;
            }
            unsigned int i = 0;
            const unsigned char *data = p->m_data;

            /* the frame counted before it is played: the target's `i += 8` sits in the play
             * call's delay slot */
            for (; i < p->m_length;) {
                func_003106A0((VoiceDecoder *)Voice()->m_codec, data + i,
                              (short *)(data_0045BD90 + data_0045BD80));
                data_0045BD80 += 320;
                i += 8;
                func_0030F5F0((VoicePlayer *)Voice(), (short *)data_0045BD90, data_0045BD80);
                data_0045BD80 = 0;
            }
            p->m_taken = i;
        } else {
            p->m_taken = func_0030F5F0((VoicePlayer *)Voice(), (const short *)p->m_data, p->m_length);
        }
        Voice()->m_talker = p->m_client;
    }
}
}

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
