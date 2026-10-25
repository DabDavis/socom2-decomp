/* zsnd_queue.cpp -- SOCOM II's sound queue, CSndQueue, and its entries (OVL1 0x34C560-0x34CD6F);
 * the headset's CSndVocalQueue over it is the next unit, zsnd_vocalqueue.cpp (0x34CD70-0x34D17F).
 * Not in SOCOM 1; the class names are the RTTI's (zsnd_queue.h), the members' and the unit's names
 * ours. Defined in reverse of their image order (-inline deferred).
 *
 * The two units are why every call the vocal queue makes into this one stays a call while this
 * unit's own CSndQueue::Open inlines the entry's constructor: -inline auto never inlines across a
 * unit boundary. Read as one unit, the calls had looked like an inliner reaching only functions
 * defined earlier, which no Metrowerks mode does (README "The two queue units"). */

#include "zsnd_queue.h"

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

bool CSndQueueEntry::CanPlay()
{
    return true;
}

CSndInstance *CSndQueueEntry::Play()
{
    return m_sound->PlaySoundVol(m_vol, 0, 255);
}

CSndQueue::CSndQueue()
    : m_instance(0), m_sound(0)
{
}

void CSndQueue::Open()
{
    if (m_free.size() == 0) {
        for (int i = 0; i < 80; i++) {
            CSndQueueEntry *e = new CSndQueueEntry;

            m_free.push_back(e);
        }
    }
    m_instance = 0;
    m_sound = 0;
}

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

/* The two dumps are the unit's only pins: GetFreeEntry and OnPlay, in this unit, call them out of
 * line. Empty walks here, they were debug prints compiled out, and code compiled to nothing still
 * counts towards -inline auto's statement limit (README "The compiler"), so the original's were
 * too big to auto-inline; the pin stands in for the vanished statements. */
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
