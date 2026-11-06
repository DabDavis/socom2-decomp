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

/* SOCOM 1's: a channel's sound queued in the last free entry (none free, none queued); SOCOM II
 * plays nothing unless the headset is idle or the sound a dialog or bit 6 one (true, unqueued).
 * A mode-2 or mode-3 sound drops the channel's older entries (SOCOM 1's Remove, 0x34AAC0) */
bool Headset::Queue(unsigned int id, CSnd *sound, int vol)
{
    int idx = -1;

    if (m_free.size() == 0)
        return false;
    if (!m_b && sound->m_mode != 3 && !sound->m_socom2_bit6)
        return true;
    idx = m_free.back();
    if (idx == -1)
        return false;
    m_free.erase(m_free.end() - 1);
    m_entries[idx * 3] = id;
    m_entries[idx * 3 + 1] = (unsigned int)sound;
    m_entries[idx * 3 + 2] = vol;
    m_queue.push_back(idx);
    if (sound->m_mode == 2)
        Remove(id, true, false);
    else if (sound->m_mode == 3)
        Remove(id, true, true);
    return true;
}

/* SOCOM 1's: the first queued entry whose channel the skip list (a count, then pointers to the
 * channels) does not hold -- its channel, sound and volume given back, the entry taken off the
 * queue and its index freed; false when none is left. SOCOM II stops at the first such entry */
bool Headset::DeQueue(int **skip, unsigned int *id, CSnd **sound, int *vol)
{
    bool found = false;
    std::vector<int>::iterator i;

    for (i = m_queue.begin(); i != m_queue.end() && !found;) {
        bool skipped = false;

        /* the count re-read in the loop test (hoisted by MW: a temporary numbered after the
         * walk's, as the target's registers need; a named local swaps three of them) */
        if (skip) {
            if (*skip[0] > 0) {
                for (int k = 1; k <= *skip[0]; k++) {
                    if (*skip[k] == (int)m_entries[*i * 3]) {
                        skipped = true;
                        break;
                    }
                }
            }
        }
        if (!skipped) {
            int idx = *i;

            *id = m_entries[idx * 3];
            *sound = (CSnd *)m_entries[idx * 3 + 1];
            *vol = m_entries[idx * 3 + 2];
            i = m_queue.erase(i);
            m_free.push_back(idx);
            found = true;
        }
    }
    return found;
}

void Headset::StaticSoundSysCallback(unsigned int handle, unsigned long user_data)
{
    if ((Headset *)user_data)
        ((Headset *)user_data)->SoundSysCallback(handle);
}

/* the stream's start or stop acknowledged. A refused start (case 0, a handle) falls back to state
 * 1, as a refused stop does: the target stores the chain's last constant, 1, there (an `m_state =
 * 2` reads as the case constants trading registers). Case 1 has a block of its own (a return) */
void Headset::SoundSysCallback(unsigned int handle)
{
    switch (m_state) {
    case 0:
        if (handle == 0) {
            snd_DoExternCall(0x12C4E67A, 7, 0, 0, 0, 0, 0);
            theHeadsetState = 2;
            m_state = 4;
        } else
            m_state = 1;
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

/* ConnectedTick's: the stream's file name (a static buffer), the headset's volume scale, the VAG
 * index's lookup and its archives' sectors (zsnd_subtitles.cpp's CSubtitleTable 0x48DC30's
 * 0x34D480 / 0x34D470 / 0x34D460, as CSnd::StreamSoundVolPan's; names not known) */
extern char data_0049E1A0[];
extern float data_003E0090;
class CSubtitleTable;
extern CSubtitleTable data_0048DC30;
enum VAG_WHERE { VAG_NONE = 0, VAG_INDEX = 1, VAG_STORE = 2 };   /* a byte (-enum min); names ours */
extern "C" VAG_WHERE func_0034D480(CSubtitleTable *table, const char *name, unsigned int *offset,
                                   int *size);
extern "C" int func_0034D470(CSubtitleTable *table);
extern "C" int func_0034D460(CSubtitleTable *table);

/* DisconnectedTick's: the stream's file name and the headset's volume scale when it is found
 * (static; names not known) */
extern char data_0049E160[];
extern float data_003E0088;

/* SOCOM 1's: while no headset is connected, the voice of a stopped request let go (state 4) and a
 * request's speaker voice stopped when a stop is wanted, or let go once it has finished; once idle
 * the next queued sound dequeued and the device looked for -- found, the headset is set up (its
 * stream opened unless online) and connected, and the sound's .VAG streamed to it; not found, the
 * sound played through the speakers on a voice of its own (SOCOM II's: SOCOM 1 found it in the
 * bank archive). A sound neither dialog nor bit 6 is refused while the headset is not free */
void Headset::DisconnectedTick(float dT)
{
    bool ready = false;

    if (m_state == 4) {
        m_34 = 0;
        m_instance = 0;
        m_3c = -1;
        m_state = 3;
    }
    if (m_state == 3)
        ready = true;
    else if (m_state == 1) {
        if (m_stop && m_34) {
            m_34->Stop();
            CSndInstance::ReleaseSoundInstance(m_34);
            m_34 = 0;
            m_stop = 0;
            m_state = 3;
        } else if (m_34 && !m_34->IsPlaying()) {
            CSndInstance::ReleaseSoundInstance(m_34);
            m_34 = 0;
            m_state = 3;
        }
    }
    if (ready && DeQueue(0, &m_c, &m_sound, &m_vol)) {
        int device;
        int format;
        unsigned char channels;
        bool found = FindDevice(&device, &format, &channels);

        if (found) {
            SetDevice(device, format, channels);
            if (theNetwork.m_bNetwork)
                m_state = 3;
            else {
                snd_DoExternCall(0x12C4E67A, 0, 0xF000, 0x400, 0x5622, (m_8 << 16) | 16, m_0);
                m_state = 3;
            }
            theHeadsetState = 1;
        }
        if (!m_b && m_sound && m_sound->m_mode != 3 && !m_sound->m_socom2_bit6) {
            m_state = 3;
            m_34 = 0;
            return;
        }
        if (!found) {
            m_34 = CSndInstance::AcquireSoundInstance(-1, m_sound);
            if (m_34) {
                if (!m_sound->StreamSoundVolPan(m_vol, 0, m_34, 0)) {
                    m_34->m_handle = 0;
                    CSndInstance::ReleaseSoundInstance(m_34);
                    m_34 = 0;
                    m_state = 3;
                    return;
                }
                m_state = 1;
                if (m_34) {
                    if (m_sound && m_sound->m_socom2_bit4) {
                        m_instance = m_sound;
                        m_3c = m_c;
                    }
                } else {
                    m_instance = 0;
                    m_3c = -1;
                }
            }
        } else {
            unsigned int offset = 0;
            int size = 0;

            sprintf(data_0049E160, "%s.VAG", m_sound->m_20);
            CSubtitleTable *table = &data_0048DC30;
            int where = func_0034D480(table, data_0049E160, &offset, &size);
            int loc = 0;

            if (where == VAG_INDEX) {
                loc = (offset >> 11) + func_0034D470(table);
                offset &= 0x7FF;
            } else if (where == VAG_STORE && func_0034D460(table)) {
                loc = (offset >> 11) + func_0034D460(table);
                offset &= 0x7FF;
            }
            if (where && loc) {
                m_vol = (int)(m_vol * data_003E0088);
                snd_DoExternCall_CB(0x12C4E67A, 1, loc, offset, m_vol, 2, 0, StaticSoundSysCallback,
                                    (unsigned long)this);
                m_state = 0;
            }
        }
    }
}

/* SOCOM 1's: while the headset is connected, a stream request (state 1) sent -- a start, or a
 * stop when one is wanted -- and once idle (state 3) the next queued sound dequeued: one neither
 * dialog nor bit 6 refused while the headset is not free (idle again), else its .VAG found in the
 * VAG indexes and streamed at the headset's volume scale (SOCOM II's: SOCOM 1 found it in the bank
 * archive), a voice-over sound remembered with its entry */
void Headset::ConnectedTick(float dT)
{
    bool ready = false;

    if (m_state == 3)
        ready = true;
    else if (m_state == 1) {
        if (m_stop) {
            snd_DoExternCall_A(0x12C4E67A, 6, 0, 0, 0, 0, 0);
            m_state = 3;
            m_stop = 0;
        } else {
            snd_DoExternCall_CB(0x12C4E67A, 2, 0, 0, 0, 0, 0, StaticSoundSysCallback, (unsigned long)this);
            m_state = 2;
        }
    }
    if (ready && DeQueue(0, &m_c, &m_sound, &m_vol)) {
        if (!m_b && m_sound && m_sound->m_mode != 3 && !m_sound->m_socom2_bit6) {
            m_state = 3;
            m_34 = 0;
            return;
        }
        unsigned int offset = 0;
        int size = 0;

        sprintf(data_0049E1A0, "%s.VAG", m_sound->m_20);
        CSubtitleTable *table = &data_0048DC30;
        int loc = 0;
        int where = func_0034D480(table, data_0049E1A0, &offset, &size);

        if (where == VAG_INDEX) {
            loc = (offset >> 11) + func_0034D470(table);
            offset &= 0x7FF;
        } else if (where == VAG_STORE && func_0034D460(table)) {
            loc = (offset >> 11) + func_0034D460(table);
            offset &= 0x7FF;
        }
        if (where && loc) {
            m_vol = (int)(m_vol * data_003E0090);
            if (m_sound) {
                if (m_sound && m_sound->m_socom2_bit4) {
                    m_instance = m_sound;
                    m_3c = m_c;
                }
            } else {
                m_instance = 0;
                m_3c = -1;
            }
            snd_DoExternCall_CB(0x12C4E67A, 1, loc, offset, m_vol, 7, 0, StaticSoundSysCallback,
                                (unsigned long)this);
            m_state = 0;
        }
    }
}

#include "../zsys/zsys.h"

/* SOCOM 1's: a channel's queued entries dropped (their indices back to the free ones) -- with `a`
 * those whose sound is in mode 1, with `b` those in mode 2 -- and the stream stopped if it is the
 * channel's, its sound in such a mode, and its request not yet done (Queue calls it) */
void Headset::Remove(int id, bool a, bool b)
{
    std::vector<int>::iterator i;

    for (i = m_queue.begin(); i != m_queue.end();) {
        int idx = *i;
        unsigned int *e = &m_entries[idx * 3];
        bool erased = false;

        if (id == e[0]) {
            if (a && ((CSnd *)e[1])->m_mode == 1) {
                i = m_queue.erase(i);
                m_free.push_back(idx);
                erased = true;
            } else if (b && ((CSnd *)e[1])->m_mode == 2) {
                i = m_queue.erase(i);
                m_free.push_back(idx);
                erased = true;
            }
        }
        if (!erased)
            ++i;
    }
    if (m_c == id && ((a && m_sound && m_sound->m_mode == 1) || (b && m_sound && m_sound->m_mode == 2)) &&
        (m_state == 0 || m_state == 1 || m_state == 2))
        m_stop = 1;
}

/* the stream told to stop unless its sound is a dialog's (mode 3) or flagged bit 6, the flushed
 * mark cleared (SOCOM II's, unreferenced in the image: no call, no address taken -- kept, as the
 * linker kept it; the name ours) */
void Headset::Interrupt()
{
    m_b = 0;
    if (m_sound && m_sound->m_mode != 3 && !m_sound->m_socom2_bit6)
        m_stop = 1;
}

/* SOCOM 1's: every queued entry of this channel and sound dropped (its index back to the free
 * ones), and the stream stopped if it is this one and its request is not yet done */
void Headset::StopSound(int channel, CSnd *sound)
{
    std::vector<int>::iterator i;

    for (i = m_queue.begin(); i != m_queue.end();) {
        int idx = *i;
        unsigned int *e = &m_entries[idx * 3];
        bool erased = false;

        if (channel == e[0] && (CSnd *)e[1] == sound) {
            i = m_queue.erase(i);
            m_free.push_back(idx);
            erased = true;
        }
        if (!erased)
            ++i;
    }
    if (m_c == channel && m_sound == sound && (m_state == 0 || m_state == 1 || m_state == 2))
        m_stop = 1;
}

/* a group's members asked first (recursing); then the queued entries (each entry three words, its
 * sound the second) and the sound streaming while its request is not yet done */
bool Headset::IsQueued(CSnd *sound)
{
    if (!sound)
        return false;
    int n = sound->GetNumSounds();

    for (int i = 0; i < n; i++) {
        CSnd *member = sound->GetSound(i);

        if (member && IsQueued(member))
            return true;
    }
    for (std::vector<int>::iterator i = m_queue.begin(); i != m_queue.end(); ++i)
        if ((CSnd *)m_entries[*i * 3 + 1] == sound)
            return true;
    if (m_sound == sound && (m_state == 0 || m_state == 1 || m_state == 2))
        return true;
    return false;
}

/* the stream stopped and the queue emptied (SOCOM II's; name ours: CCinematicState::Init calls it).
 * Byte-exact since the map named its four callees: FreeVector's pieces are vector<int>'s
 * out-of-line copies linked from CMissionObjectiveCam's unit at 0x2C7350 (the constructor),
 * 0x2C7340 (clear), 0x2C7300 (swap) and 0x2C7290 (the destructor). The queue is a vector<int>: the
 * constructor pushes an int by reference with no temporary. Its one caller, CCinematicState::Init,
 * passes true (li a1,1 in the jal's delay slot) to a flag the body never reads (round 27: the row
 * Flush__7HeadsetFb, was Fv) */
void Headset::Flush(bool)
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
