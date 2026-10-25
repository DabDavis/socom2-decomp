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
    unsigned char _28[0x70 - 0x28];
    VoiceSlot m_slots[1];   // 0x70
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

extern "C" {
void func_0024F888(void *h, VoiceFrame *f);
extern VoiceRadio *data_0045B718;   /* the radio */
}

extern "C" {

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
