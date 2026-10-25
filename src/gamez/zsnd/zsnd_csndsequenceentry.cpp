/* zsnd_csndsequenceentry.cpp -- CSndSequenceEntry, one step of a sound sequence: a sound played to
 * its end, or a pause (OVL1 0x349A70-0x349C6F). Its own unit, linked right before
 * zsnd_csndsequence.cpp: CSndSequence's FadeOut, IsFading, Stop and Tick call every one of these out
 * of line, which -inline auto would not do inside one unit (README "The inlining mode, measured").
 * It closes no template tail and has no initialiser; the jukebox unit before it ends with its two
 * array destructors (0x349A30, 0x349A50). No pins. SOCOM 1 links these functions in the same place
 * but names their file nowhere -- the press beta's zsnd_csndsequence.cpp asserts are in
 * CSndSequence's CreateSequence and destructor -- so this unit's name is ours. Defined in reverse
 * of their image order (-inline deferred). */

#include "zsnd.h"

CSndSequenceEntry::CSndSequenceEntry(CSnd *sound, float pause)
{
    m_sound = sound;
    m_pause = pause;
    m_timer = 0.0f;
}

bool CSndSequenceEntry::Tick(float dT)
{
    if (m_sound) {
        if (!m_instance || !m_instance->IsPlaying()) {
            if (m_instance)
                CSndInstance::ReleaseSoundInstance(m_instance);
            if (m_instance)
                CSndInstance::ReleaseSoundInstance(m_instance);
            m_instance = 0;
            return true;
        }
        return false;
    }
    m_timer += dT;
    if (m_timer > m_pause) {
        m_timer = 0.0f;
        return true;
    }
    return false;
}

void CSndSequenceEntry::Play()
{
    if (m_sound)
        m_instance = m_sound->PlaySound(false, 0, 255);
}

void CSndSequenceEntry::Stop()
{
    if (m_instance) {
        m_instance->Stop();
        CSndInstance::ReleaseSoundInstance(m_instance);
    }
    m_instance = 0;
}

void CSndSequenceEntry::FadeOut()
{
    if (m_instance)
        m_instance->FadeOut(2.0f);
    else
        m_timer = m_pause;
}

bool CSndSequenceEntry::IsFading()
{
    if (m_instance)
        return m_instance->IsFading();
    return false;
}
