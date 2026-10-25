/* zsnd_queue.cpp -- SOCOM II's sound queues: sounds played one at a time after a delay each,
 * CSndQueue and the headset's CSndVocalQueue over it (OVL1 0x34C560-0x34D17F). Not in SOCOM 1; the
 * class names are the RTTI's, the members' and the unit's names ours. Defined in reverse of their
 * image order (-inline deferred). */

#include "zsnd.h"
#include "../zsys/zsys.h"

/* one queued sound: played when its delay has run out, if it still can be (the RTTI's names) */
class CSndQueueEntry {
public:
    CSndQueueEntry();
    virtual CSndInstance *Play();
    virtual bool CanPlay();
    void SetUsed(bool used);
    void Set(CSnd *sound, float delay, float vol);

    float m_delay;      /* 0x04: counted down by the queue's tick */
    CSnd *m_sound;      /* 0x08 */
    float m_vol;        /* 0x0C */
    bool m_used;        /* 0x10: taken from the free entries */
};   /* 0x14 bytes */

/* a voice's entry: an id to remove it by, and a position to play it at */
class CSndVocalQueueEntry : public CSndQueueEntry {
public:
    CSndVocalQueueEntry() : m_id(-1), m_is3D(false) {}
    virtual CSndInstance *Play();
    virtual bool CanPlay();

    int m_id;           /* 0x14: passed as the play's entity id */
    PNT3D m_pos;        /* 0x18 */
    bool m_is3D;        /* 0x24 */
};   /* 0x28 bytes */

/* the queue: the entries waiting, in order, and the free ones they come from */
class CSndQueue {
public:
    CSndQueue();
    /* declared only: the image holds no destructor (the queue is a global never destroyed), and an
     * implicit one would be emitted with the vocal queue's constructor */
    ~CSndQueue();
    virtual void Stop();
    virtual void OnPlay(CSndQueueEntry *entry);
    virtual void Open();
    virtual void Close();
    void Tick(float dT);
    CSndQueueEntry *GetFreeEntry();
    void PlayNext();
    void DumpQueue();
    void DumpFree();

    std::vector<CSndQueueEntry *> m_queue;  /* 0x04 */
    std::vector<CSndQueueEntry *> m_free;   /* 0x10 */
    CSndInstance *m_instance;               /* 0x1C: the entry playing */
    CSnd *m_sound;                          /* 0x20: its sound */
};   /* 0x24 bytes */

class CSndVocalQueue : public CSndQueue {
public:
    CSndVocalQueue();
    virtual void Stop();
    virtual void OnPlay(CSndQueueEntry *entry);
    virtual void Open();
    void Add(CSnd *sound, int id, const CPnt3D *pos, float delay, float vol);
    void Remove(int id);

    int m_id;           /* 0x24: the playing entry's id, -1 for none */
};   /* 0x28 bytes */

CSndInstance *CSndVocalQueueEntry::Play()
{
    if (m_is3D)
        return m_sound->PlaySound3D((const CPnt3D *)&m_pos, false, m_vol, 0, 0, m_id);
    return m_sound->PlaySoundVol(m_vol, 0, m_id);
}

bool CSndVocalQueueEntry::CanPlay()
{
    if (m_is3D && m_sound) {
        int vol, pan;

        m_sound->CalcVolPan((const CPnt3D *)&m_pos, 1.0f, &vol, &pan);
        if (!vol)
            return false;
    }
    return true;
}

CSndVocalQueue::CSndVocalQueue()
    : m_id(-1)
{
}

void CSndVocalQueue::Open()
{
    if (m_free.size() == 0) {
        for (int i = 0; i < 80; i++) {
            CSndQueueEntry *e = new CSndVocalQueueEntry;

            m_free.push_back(e);
        }
    }
    m_instance = 0;
    m_sound = 0;
}

/* OPEN, 4 instructions: the target tests GetFreeEntry's result in v0, then stores the pushed copy
 * (`sw v0,124(sp)` after the branch, its delay slot a nop) and loads the entry back from it into
 * s1; here MW forwards the copy (`move s1,v0`) or, with `if ((entry = GetFreeEntry()) != 0)`, puts
 * the store in the branch's delay slot (otherwise byte-exact). Tried: the copy as its own local, a
 * pointer reference, casts at every use, the early return, the arms swapped */
void CSndVocalQueue::Add(CSnd *sound, int id, const CPnt3D *pos, float delay, float vol)
{
    CSndQueueEntry *entry = GetFreeEntry();

    if (entry) {
        CSndQueueEntry *kept = entry;
        CSndVocalQueueEntry *e = (CSndVocalQueueEntry *)kept;

        if (!pos)
            e->m_is3D = false;
        else {
            e->m_pos.x = pos->x;
            e->m_pos.y = pos->y;
            e->m_pos.z = pos->z;
            e->m_is3D = true;
        }
        e->Set(sound, delay, vol);
        e->m_id = id;
        m_queue.push_back(kept);
    }
}

void CSndVocalQueue::Stop()
{
    CSndQueue::Stop();
    m_id = -1;
}

void CSndVocalQueue::Remove(int id)
{
    if (m_id == id)
        Stop();
    std::vector<CSndQueueEntry *>::iterator i;

    for (i = m_queue.begin(); i != m_queue.end();) {
        CSndVocalQueueEntry *e = (CSndVocalQueueEntry *)*i;

        if (id == e->m_id) {
            e->SetUsed(false);
            i = m_queue.erase(i);
        } else
            ++i;
    }
}

void CSndVocalQueue::OnPlay(CSndQueueEntry *entry)
{
    m_id = ((CSndVocalQueueEntry *)entry)->m_id;
    DumpQueue();
    DumpFree();
}

/* The functions the target calls out of line from functions defined before them are pinned: the
 * original's auto-inlining reached only functions defined earlier in the unit, which every call in
 * this unit bears out -- CSndQueue::Open (defined after this constructor) inlines it where the
 * vocal entry's inline constructor, inlined into CSndVocalQueue::Open (defined before it), calls
 * it; CSndVocalQueue's constructor, Stop and OnPlay call CSndQueue's later-defined constructor,
 * Stop and walks. Our -inline deferred reaches every function of the unit, so the pins stand in.
 * CSndQueue::Open (0x34CC00) itself is OPEN here, 6 instructions long: with this constructor
 * pinned it calls it as the vocal queue's Open does, where the target inlines it; it is left to
 * its byte-exact draft. */
#pragma auto_inline off
CSndQueueEntry::CSndQueueEntry()
    : m_delay(0), m_sound(0), m_vol(1.0f), m_used(false)
{
}

void CSndQueueEntry::Set(CSnd *sound, float delay, float vol)
{
    m_sound = sound;
    m_vol = vol;
    m_delay = delay;
}

void CSndQueueEntry::SetUsed(bool used)
{
    m_used = used;
}
#pragma auto_inline reset

bool CSndQueueEntry::CanPlay()
{
    return true;
}

CSndInstance *CSndQueueEntry::Play()
{
    return m_sound->PlaySoundVol(m_vol, 0, 255);
}

#pragma auto_inline off
CSndQueue::CSndQueue()
    : m_instance(0), m_sound(0)
{
}
#pragma auto_inline reset

void CSndQueue::Close()
{
    std::vector<CSndQueueEntry *>::iterator i;

    for (i = m_free.begin(); i != m_free.end(); i = m_free.begin()) {
        delete *i;
        m_free.erase(i);
    }
    FreeVector(m_free);
    FreeVector(m_queue);
    m_instance = 0;
    m_sound = 0;
}

void CSndQueue::Tick(float dT)
{
    Headset *h = Headset::ftsGetHeadset();
    std::vector<CSndQueueEntry *>::iterator i = m_queue.begin();

    if (m_instance) {
        if (!m_instance->IsPlaying()) {
            CSndInstance::ReleaseSoundInstance(m_instance);
            m_instance = 0;
            m_sound = 0;
        }
    } else if (i != m_queue.end()) {
        if (!h->IsQueued(m_sound))
            PlayNext();
    }
    for (; i != m_queue.end(); ++i) {
        CSndQueueEntry *e = *i;

        e->m_delay -= dT;
        if (e->m_delay < 0.0f)
            e->m_delay = 0.0f;
    }
}

#pragma auto_inline off
CSndQueueEntry *CSndQueue::GetFreeEntry()
{
    for (std::vector<CSndQueueEntry *>::iterator i = m_free.begin(); i != m_free.end(); ++i) {
        CSndQueueEntry *e = *i;

        if (!e->m_used) {
            e->m_used = true;
            return e;
        }
    }
    DumpQueue();
    return 0;
}

void CSndQueue::Stop()
{
    if (m_instance)
        m_instance->Stop();
    m_instance = 0;
    m_sound = 0;
}
#pragma auto_inline reset

/* OPEN, 17 instructions, stack slots only: the target puts the front iterator `first` at 88(sp),
 * below all four erase temporaries (104, 100 in the loop, 96 and 92 in the two arms); here it takes
 * 104, right after `i` (108), and the temporaries follow. Tried: `first` declared at function
 * scope, assigned rather than initialised, an inline helper taking it by value */
void CSndQueue::PlayNext()
{
    bool found = false;
    std::vector<CSndQueueEntry *>::iterator i;

    for (i = m_queue.begin(); i != m_queue.end() && !found; i = m_queue.begin()) {
        CSndQueueEntry *e = *i;

        if (e->m_delay <= 0.0f) {
            e->m_used = false;
            m_queue.erase(i);
        } else if (!e->CanPlay()) {
            e->m_used = false;
            m_queue.erase(i);
        } else
            found = true;
    }
    if (m_queue.size()) {
        std::vector<CSndQueueEntry *>::iterator first = m_queue.begin();

        m_instance = (*first)->Play();
        m_sound = 0;
        Headset *h = Headset::ftsGetHeadset();

        if (m_instance || (h && h->IsQueued((*first)->m_sound))) {
            OnPlay(*first);
            m_sound = (*first)->m_sound;
            (*first)->m_used = false;
            m_queue.erase(first);
        } else {
            (*first)->m_used = false;
            m_queue.erase(first);
            PlayNext();
        }
    }
}

void CSndQueue::OnPlay(CSndQueueEntry *entry)
{
    DumpQueue();
    DumpFree();
}

#pragma auto_inline off
void CSndQueue::DumpQueue()
{
    for (std::vector<CSndQueueEntry *>::iterator i = m_queue.begin(); i != m_queue.end(); i++)
        ;
}

void CSndQueue::DumpFree()
{
    for (std::vector<CSndQueueEntry *>::iterator i = m_free.begin(); i != m_free.end(); i++)
        ;
}
#pragma auto_inline reset
