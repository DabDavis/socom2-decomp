/* zsnd_csndinstance.cpp -- CSndInstance, one playing voice of a CSnd (OVL1 0x346300-0x347B9F).
 * The unit name is SOCOM 1's (its __sinit_zsnd_csndinstance.cpp, paired with SOCOM II's 0x403230:
 * the list of voices, the pool and a second vector of instances). Defined in reverse of their
 * image order (-inline deferred). */

#include "zsnd.h"

extern "C" void func_00341010();        /* zsnd_csnd.cpp (not named yet) */
extern "C" bool func_00346270();        /* zsnd_csnd.cpp (not named yet) */

std::list<CSndInstance *> CSndInstance::m_list;
CSndInstanceVec CSndInstance::m_pool;
std::vector<CSndInstance *> CSndInstance::m_vec;

void CSndInstance::Cleanup()
{
    m_vec.clear();
}

/* SOCOM 1's DebugSoundList walked the voices to print them; SOCOM II's never steps -- with its
 * report compiled out, the step went too (the element is still read: the report's argument; the loop is MW's short-loop padding) */
void CSndInstance::DebugSoundList()
{
    std::list<CSndInstance *>::iterator e = m_list.end();
    std::list<CSndInstance *>::iterator i = m_list.begin();

    while (i != e) {
        CSndInstance *p = *i;
    }
}

CSndInstance *CSndInstance::AcquireSoundInstance(unsigned int handle, CSnd *parent)
{
    CSndInstance *inst;

    if (m_pool.size() == 0) {
        inst = new CSndInstance(handle, parent);
        m_numAllocated++;
    } else {
        inst = m_pool.back();
        inst->InitInstance(handle, parent);
        m_pool.pop_back();
    }
    CSnd::m_numActive++;
    return inst;
}

/* OPEN, 21 instructions: registers only -- the target reloads the homed instance into a0 and
 * keeps it there, MW here takes t0 (which shifts the loop's registers up one). Tried: the
 * pending case as an inline (TerminateASAP, which removed the reload between the two bit
 * stores), an inline bool test, an iterator loop, a local copy for push_back */
void CSndInstance::ReleaseSoundInstance(CSndInstance *instance)
{
    if (instance->m_handle == -1) {
        instance->TerminateASAP();
        return;
    }
    for (unsigned int i = 0; i < m_pool.size(); i++)
        if (m_pool[i] == instance)
            return;
    if (instance->m_parent->m_isStreamed) {
        CSnd::m_numStreams--;
        if (!func_00346270())
            func_00341010();
    }
    CSnd::m_numActive--;
    if (m_pool.size() == m_pool.capacity())
        m_pool.reserve(m_pool.size() + 16);
    m_pool.push_back(instance);
    RemoveFromList(instance);
}

void CSndInstance::InitInstance(unsigned int handle, CSnd *parent)
{
    m_parent = parent;
    m_handle = handle;
    m_forcedLoop = false;
    m_list.push_back(this);
    m_numInits++;
    m_vol = 1.0f;
    m_pos = CPnt3D::zero;
    m_is3D = false;
    m_fadeTime = 0.0f;
    m_terminateOnFinish = false;
    m_terminateASAP = false;
    m_pauseASAP = false;
    m_continueASAP = false;
    m_socom2_bit6 = false;
    m_c = 0;
}

void CSndInstance::Stop()
{
    if (CSnd::m_isDisabled)
        return;
    if (m_handle == -1) {
        m_terminateASAP = true;
        m_terminateOnFinish = true;
    } else
        snd_StopSound(m_handle);
}

void CSndInstance::SetVol(float vol)
{
    if (CSnd::m_isDisabled)
        return;
    m_vol = vol;
    if (!m_is3D) {
        int i;

        for (i = 0; m_handle == -1 && i < 500; i++)
            snd_FlushSoundCommands();
        if (i < 500)
            snd_SetSoundVolPan(m_handle, CalcVol(vol), -2);
    }
}

void CSndInstance::SetPan(int pan)
{
    if (CSnd::m_isDisabled)
        return;
    while (m_handle == -1)
        snd_FlushSoundCommands();
    snd_SetSoundVolPan(m_handle, 0x7FFFFFFF, pan);
}

void CSndInstance::SetPitch(int pitch)
{
    if (CSnd::m_isDisabled)
        return;
    while (m_handle == -1)
        snd_FlushSoundCommands();
    snd_SetSoundPitch(m_handle, pitch);
}

void CSndInstance::FadeOut(float time)
{
    m_fadeTime = time;
    if (!CSnd::m_isDisabled) {
        int i;

        for (i = 0; m_handle == -1 && i < 500; i++)
            snd_FlushSoundCommands();
        if (i < 500) {
            m_vol = 0.0f;
            snd_AutoVol(m_handle, CalcVol(0.0f), (int)(240.0f * time), 2);
        }
    }
}

bool CSndInstance::IsFading()
{
    return m_fadeTime > 0.0f;
}

void CSndInstance::Set3D(float vol)
{
    m_vol = vol;
    m_is3D = true;
}

void CSndInstance::TerminateOnFinish()
{
    m_terminateOnFinish = true;
}

/* the voice taken off the list of those playing (SOCOM II's, name ours). Pinned:
 * ReleaseSoundInstance calls it */
#pragma auto_inline off
void CSndInstance::RemoveFromList(CSndInstance *instance)
{
    std::list<CSndInstance *>::iterator e = m_list.end();

    for (std::list<CSndInstance *>::iterator i = m_list.begin(); i != e; ++i) {
        if (*i == instance) {
            m_list.erase(i);
            return;
        }
    }
}
#pragma auto_inline reset

void CSndInstance::PlaySoundCallback(unsigned int handle, unsigned long user_data)
{
    *(unsigned int *)user_data = handle;
}
