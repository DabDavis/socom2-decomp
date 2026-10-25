/* zsnd_vocalqueue.cpp -- the headset's voice queue, CSndVocalQueue over CSndQueue (OVL1
 * 0x34CD70-0x34D17F). Its own unit, linked right after zsnd_queue.cpp: every call it makes into
 * CSndQueue and CSndQueueEntry stays a call -- the base constructor, CSndQueue::Stop, the two dump
 * walks, GetFreeEntry, Set, SetUsed, and the entry's constructor inside the vocal entry's inline one
 * -- because -inline auto never inlines across units, while CSndQueue::Open, in zsnd_queue.cpp,
 * inlines that same constructor (README "The two queue units"). No pins. Not in SOCOM 1; the unit's
 * name is ours. Defined in reverse of their image order (-inline deferred). */

#include "zsnd_queue.h"

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
