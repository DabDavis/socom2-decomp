/* zsnd_headset.cpp -- Headset, the USB headset's voice stream (OVL1 0x34A620-...). SOCOM 1 links
 * its functions after CSndSequence's; the unit name is ours. Defined in reverse of their image
 * order (-inline deferred). */

#include "zsnd.h"

Headset *Headset::theHeadset;

/* the headset made (SOCOM II's; CSnd::Init calls it). It returns the headset: only so does MW keep
 * new's result in v0 for the store, as the target does (void, it is moved to a0 first) */
Headset *Headset::Create()
{
    theHeadset = new Headset;
    return theHeadset;
}

/* the headset, looked for once (SOCOM 1's: SOCOM II's finds the device instead of making it --
 * Create made it already) */
Headset *Headset::ftsGetHeadset()
{
    if (theHeadsetState == 1)
        return theHeadset;
    if (theHeadsetState == 2)
        return theHeadset;
    unsigned char channels;
    int device;
    int format;
    bool found = FindDevice(&device, &format, &channels);
    Headset *h = theHeadset;

    if (!h)
        return 0;
    if (found) {
        theHeadsetState = 1;
        h->SetDevice(device, format, channels);
        return theHeadset;
    }
    theHeadsetState = 2;
    h->m_state = 3;
    return theHeadset;
}

/* the Logitech USB audio library's device description (the fields read; the layout ours) */
struct lgAudFormat {
    unsigned char channels;     /* 0x0 */
    unsigned char bits;         /* 0x1 */
    unsigned short minRate;     /* 0x2 */
    unsigned short maxRate;     /* 0x4 */
};

struct lgAudDeviceDesc {
    char m_0[98];
    unsigned char numFormats;   /* 0x62 */
    char m_63;
    lgAudFormat formats[39];    /* 0x64 */
    char m_14e[2];
};   /* 0x150 bytes (the frame's) */

extern "C" int lgAudEnumerate(int index, lgAudDeviceDesc *desc);

class CZNetwork {
public:
    unsigned char m_bSessionMaster; /* 0x0 */
    unsigned char m_bNetwork;       /* 0x1 */
};
extern CZNetwork theNetwork;

/* the first USB audio device with a 16-bit mono format at 22050 Hz (SOCOM II's; name ours): never
 * in a network game */
bool Headset::FindDevice(int *device, int *format, unsigned char *channels)
{
    if (theNetwork.m_bNetwork)
        return false;
    lgAudDeviceDesc desc;

    for (int index = 0; lgAudEnumerate(index, &desc) == 0; index++) {
        for (int i = 0; i < desc.numFormats; i++) {
            if (desc.formats[i].bits == 16 && desc.formats[i].minRate <= 22050 &&
                desc.formats[i].maxRate >= 22050 && desc.formats[i].channels == 1) {
                *device = index;
                *format = i;
                *channels = desc.formats[i].channels;
                return true;
            }
        }
    }
    return false;
}

/* the headset's stream module started on the device found (SOCOM II's; name ours): 989snd's extern
 * call 0, a 0xF000-byte buffer at 22050 Hz -- never in a network game */
void Headset::InitStream()
{
    if (theNetwork.m_bNetwork) {
        m_state = 3;
        theHeadsetState = 2;
        return;
    }
    snd_DoExternCall(0x12C4E67A, 0, 0xF000, 1024, 22050, (m_8 << 16) | 0x10, m_0);
    m_state = 3;
}

Headset::Headset()
{
    m_instance = 0;
    m_3c = -1;
    m_entries = new unsigned int[60];
    for (int i = 0; i < 20; i++)
        m_free.push_back(i);
    m_34 = 0;
    m_stop = 0;
    m_b = 1;
    m_0 = 0;
    m_4 = 0;
    m_8 = 0;
    m_state = 3;
    m_c = 0;
    m_sound = 0;
    m_vol = 0;
}

/* the stream request and the headset state reset (SOCOM II's; CSnd::Close calls it) */
void Headset::Reset()
{
    Headset *h = theHeadset;

    if (h) {
        h->m_34 = 0;
        h->m_stop = 0;
        h->m_b = 1;
        h->m_0 = 0;
        h->m_4 = 0;
        h->m_8 = 0;
        h->m_state = 3;
        h->m_c = 0;
        h->m_sound = 0;
        h->m_vol = 0;
    }
    theHeadsetState = 0;
}

void Headset::ResetStream()
{
    Headset *h = theHeadset;

    if (h) {
        h->m_instance = 0;
        h->m_3c = -1;
    }
}

void Headset::StaticSoundSysCallback(unsigned int handle, unsigned long user_data)
{
    if ((Headset *)user_data)
        ((Headset *)user_data)->SoundSysCallback(handle);
}

/* OPEN, 4 instructions: the case chain's constants 3 and 2 sit in a0 and v1 in the target (each
 * reused by a case's store), in v1 and a0 here. Case 1 has a block of its own (a return) */
void Headset::SoundSysCallback(unsigned int handle)
{
    switch (m_state) {
    case 0:
        if (handle == 0) {
            snd_DoExternCall(0x12C4E67A, 7, 0, 0, 0, 0, 0);
            theHeadsetState = 2;
            m_state = 4;
        } else
            m_state = 2;
        break;
    case 1:
        return;
    case 2:
        if (handle == 0) {
            m_state = 3;
            m_stop = 0;
        } else
            m_state = 1;
        break;
    case 3:
        break;
    }
}

void Headset::Tick(float dT)
{
    if (theHeadsetState == 2)
        DisconnectedTick(dT);
    else if (theHeadsetState == 1)
        ConnectedTick(dT);
}

#include "../zsys/zsys.h"

/* the stream stopped and the queue emptied (SOCOM II's; name ours: CCinematicState::Init calls it).
 * Byte-exact since the map named its four callees: FreeVector's pieces are vector<int>'s
 * out-of-line copies linked from CMissionObjectiveCam's unit at 0x2C7350 (the constructor),
 * 0x2C7340 (clear), 0x2C7300 (swap) and 0x2C7290 (the destructor). The queue is a vector<int>: the
 * constructor pushes an int by reference with no temporary */
void Headset::Flush()
{
    if (theHeadsetState == 1 && m_state != 3) {
        snd_DoExternCall(0x12C4E67A, 6, 0, 0, 0, 0, 0);
        m_state = 3;
        m_stop = 0;
    }
    while (DeQueue(0, &m_c, &m_sound, &m_vol))
        ;
    FreeVector(m_queue);
    m_b = 1;
}

void Headset::Pause()
{
    if (theHeadsetState == 1)
        snd_DoExternCall_A(0x12C4E67A, 4, 0, 0, 0, 0, 0);
}

void Headset::Resume()
{
    if (theHeadsetState == 1)
        snd_DoExternCall_A(0x12C4E67A, 5, 0, 0, 0, 0, 0);
}
