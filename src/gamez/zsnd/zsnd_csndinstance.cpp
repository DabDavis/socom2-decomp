/* zsnd_csndinstance.cpp -- CSndInstance, one playing voice of a CSnd (OVL1 0x346300-0x347B9F).
 * The unit name is SOCOM 1's (its __sinit_zsnd_csndinstance.cpp, paired with SOCOM II's 0x403230:
 * the list of voices, the pool and a second vector of instances). Defined in reverse of their
 * image order (-inline deferred). */

#include "zsnd.h"

extern "C" void func_00341010();        /* zsnd_csnd.cpp (not named yet) */
extern "C" void func_00341060();        /* zsnd_csnd.cpp: the group-volume fade restarted */
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
    m_dist = 0;
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

/* SOCOM 1's (a fourth flag added: every mode): the voices of an entity stopped, those of mode 1
 * when efx, of mode 2 when music */
void CSndInstance::Remove(unsigned int entityID, bool efx, bool music, bool all)
{
    if (CSnd::m_isDisabled)
        return;
    std::list<CSndInstance *>::iterator e = m_list.end();

    for (std::list<CSndInstance *>::iterator i = m_list.begin(); i != e; ++i) {
        CSndInstance *inst = *i;

        if (inst->m_entityID == entityID) {
            if ((inst->IsMode(1) && efx) || (inst->IsMode(2) && music) || all)
                inst->Stop();
        }
    }
}

extern "C" void func_0034A640(Headset *h, int entityID);    /* the headset's sounds of an entity
                                                                stopped (SOCOM II's) */

/* SOCOM 1's: an entity's effects stopped when it grows aware -- SOCOM II's calls Remove where
 * SOCOM 1 inlined the walk, and stops the headset's too */
void CSndInstance::AwarenessUpgrade(unsigned int entityID)
{
    if (CSnd::m_isDisabled)
        return;
    Remove(entityID, true, false, false);
    Headset *h = Headset::ftsGetHeadset();

    if (h)
        func_0034A640(h, entityID);
}

extern "C" void snd_StartCaching(void);
extern "C" int snd_SendCache(void);

/* SOCOM 1's: every voice ticked under one batch of commands, the finished ones let go.
 * OPEN, 3 instructions: the end iterator's address is built before snd_StartCaching here (lui
 * ahead of the call, addiu in its delay slot), after it in the target (begin's load in the delay
 * slot). The begin is read before the call in both builds (SOCOM 1 builds its iterator there);
 * declared after begin, the end gets the target's schedule but begin and end trade s0/s1 (the
 * later-defined iterator takes the lower register). Tried: end in the loop test (recomputed each
 * pass), end assigned after the call (copied through memory), begin copied through another
 * iterator, a const end. The arms each step on (three copies of the step, as SOCOM 1's) */
bool CSndInstance::PostTick(float dT, void *data)
{
    if (CSnd::m_isDisabled)
        return false;
    m_vec.clear();
    std::list<CSndInstance *>::iterator e = m_list.end();
    std::list<CSndInstance *>::iterator i = m_list.begin();

    snd_StartCaching();

    /* each arm steps on by itself (SOCOM 1's too: an erasing walk's shape) */
    while (i != e) {
        CSndInstance *inst = *i;

        if (inst) {
            bool done = inst->Tick(dT);

            if (done && inst->m_terminateOnFinish) {
                CSndInstance *kept = inst;

                m_vec.push_back(kept);
                ++i;
            } else
                ++i;
        } else
            ++i;
    }
    snd_SendCache();
    RemoveFromList();
    snd_FlushSoundCommands();
    return false;
}

extern float data_004A449C;             /* the frame's time (fts_patch.cpp's) */
extern "C" int func_00340FA0(float a, float b, float mph);     /* zsnd_csnd.cpp */
extern "C" double floor(double x);
extern float data_003E0078;             /* the Doppler shift's scale (name not known) */

/* SOCOM 1's: a voice's pending commands carried out, its fade run down, then its status asked
 * for -- a 3D voice's with its volume and pan recomputed. SOCOM II adds a Doppler shift: m_dist
 * is the listener distance of the last tick, the shift the parent's flag asks for */
bool CSndInstance::Tick(float dT)
{
    if (CSnd::m_isDisabled)
        return false;
    if (m_handle == -1)
        return false;
    if (m_handle == 0)
        return true;
    if (m_terminateASAP) {
        snd_StopSound(m_handle);
        m_handle = 0;
        return true;
    }
    if (m_pauseASAP) {
        snd_PauseSound(m_handle);
        m_pauseASAP = false;
        return false;
    }
    if (m_continueASAP) {
        snd_ContinueSound(m_handle);
        m_continueASAP = false;
    }
    if (IsFading()) {
        m_fadeTime -= dT;
        if (m_fadeTime <= 0.0f) {
            snd_StopSound(m_handle);
            m_handle = 0;
            return true;
        }
    }
    unsigned int handle = m_handle;

    m_handle = -1;
    if (m_is3D) {
        int vol = 1;
        int pan = -1;
        bool doppler = false;

        if (m_parent->m_socom2_bit8 && m_dist != 0.0f)
            doppler = true;
        float dist = m_parent->CalcVolPan(&m_pos, m_vol, &vol, &pan);
        int pitch = 0;

        if (doppler)
            pitch = func_0033F050((int)floor(data_003E0078 * func_00340FA0(m_dist, dist, data_004A449C)));
        m_dist = dist;
        snd_SetSoundParams_CB(handle, 5, vol, pan, pitch, 0, PlaySoundCallback, (unsigned long)this);
    } else
        snd_SoundIsStillPlaying_CB(handle, PlaySoundCallback, (unsigned long)this);
    return false;
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

/* every voice faded out over a second and a half (SOCOM II's; name ours: a UI command calls it) */
void CSndInstance::FadeOutAllSounds()
{
    if (CSnd::m_isDisabled)
        return;
    std::list<CSndInstance *>::iterator e = m_list.end();

    for (std::list<CSndInstance *>::iterator i = m_list.begin(); i != e; ++i) {
        CSndInstance *inst = *i;

        if (inst)
            inst->FadeOut(1.5f);
    }
}

extern "C" void func_00340270(void);    /* 989snd's command 24: every sound stopped */

/* SOCOM 1's: every voice let go -- SOCOM II stops them all with one command, then zeroes each
 * handle where SOCOM 1 stopped each voice */
void CSndInstance::StopAllSounds()
{
    if (CSnd::m_isDisabled)
        return;
    func_00341060();
    func_00340270();
    std::list<CSndInstance *>::iterator e = m_list.end();

    for (std::list<CSndInstance *>::iterator i = m_list.begin(); i != e; ++i) {
        CSndInstance *inst = *i;

        if (inst) {
            inst->m_handle = 0;
            /* pushed through a copy: only the copy goes to memory, the voice stays in v0 */
            CSndInstance *kept = inst;

            m_vec.push_back(kept);
        }
    }
    RemoveFromList();
    PostTick(0.0f, 0);
}

/* SOCOM 1's: the voices of one mode stopped (the mode now a field of SOCOM II's packed flags) */
void CSndInstance::StopAllSounds(SOUND_MODE mode)
{
    if (CSnd::m_isDisabled)
        return;
    std::list<CSndInstance *>::iterator e = m_list.end();

    for (std::list<CSndInstance *>::iterator i = m_list.begin(); i != e; ++i) {
        CSndInstance *inst = *i;

        if (inst && mode == (SOUND_MODE)inst->m_parent->m_soundmode)
            inst->Stop();
    }
}

/* the voices of one mode paused, and the headset's stream with the dialogue (SOCOM II's; names
 * ours: the mission cameras pause the game's sounds with it). The disabled flag is tested twice */
void CSndInstance::PauseAllSounds(SOUND_MODE mode)
{
    if (CSnd::m_isDisabled)
        return;
    if (CSnd::m_isDisabled)
        return;
    std::list<CSndInstance *>::iterator e = m_list.end();

    for (std::list<CSndInstance *>::iterator i = m_list.begin(); i != e; ++i) {
        CSndInstance *inst = *i;

        if (inst && mode == (SOUND_MODE)inst->m_parent->m_soundmode)
            inst->PauseVoice();
    }
    if (mode == SND_MODE_DIALOG) {
        Headset *h = Headset::ftsGetHeadset();

        if (h)
            h->Pause();
    }
}

/* ... and continued (the cameras' cleanup) */
void CSndInstance::ResumeAllSounds(SOUND_MODE mode)
{
    if (CSnd::m_isDisabled)
        return;
    if (CSnd::m_isDisabled)
        return;
    std::list<CSndInstance *>::iterator e = m_list.end();

    for (std::list<CSndInstance *>::iterator i = m_list.begin(); i != e; ++i) {
        CSndInstance *inst = *i;

        if (inst && mode == (SOUND_MODE)inst->m_parent->m_soundmode)
            inst->ContinueVoice();
    }
    if (mode == SND_MODE_DIALOG) {
        Headset *h = Headset::ftsGetHeadset();

        if (h)
            h->Resume();
    }
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

/* SOCOM 1's: the voices kept in m_vec released and taken off the list of those playing */
void CSndInstance::RemoveFromList()
{
    for (std::vector<CSndInstance *>::iterator v = m_vec.begin(); v != m_vec.end(); ++v) {
        CSndInstance *inst = *v;

        if (inst) {
            ReleaseSoundInstance(inst);
            /* found by hand: std::find compares the other way round (`*first == value`) */
            std::list<CSndInstance *>::iterator i = m_list.begin();
            std::list<CSndInstance *>::iterator e = m_list.end();

            while (i != e && inst != *i)
                ++i;

            if (*i == inst)
                m_list.erase(i);
        }
    }
}

/* SOCOM 1's: the pool and the kept list made room for n voices, the shortfall built */
void CSndInstance::InitInstancePool(int n)
{
    int count = n;

    if (m_pool.capacity() < n) {
        count = n - m_pool.capacity();
        m_pool.reserve(n);
    }
    if (m_vec.capacity() < n) {
        count = n - m_vec.capacity();
        m_vec.reserve(n);
    }
    for (int i = 0; i < count; i++) {
        CSndInstance *inst = new CSndInstance;

        m_numAllocated++;
        m_pool.push_back(inst);
    }
}

void CSndInstance::PlaySoundCallback(unsigned int handle, unsigned long user_data)
{
    *(unsigned int *)user_data = handle;
}
