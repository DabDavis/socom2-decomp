/* zsnd_jukebox.cpp -- CSndJukebox, the music: playlists of sequences out of sndjukeboxm.rdr,
 * played by intensity (OVL1 0x348860-0x349A6F). The unit name is SOCOM 1's (its
 * __sinit_zsnd_jukebox.cpp, paired with SOCOM II's 0x4032E0). Defined in reverse of their image
 * order (-inline deferred). */

#include "zsnd.h"

void CSndJukebox::FadeOut()
{
    if (m_sequence) {
        if (!m_sequence->IsFading())
            m_sequence->FadeOut();
        return;
    }
    if (m_instance) {
        if (!m_instance->IsFading())
            m_instance->FadeOut(2.0f);
        return;
    }
}

void CSndJukebox::ClearQueue()
{
    m_queued = 0;
    m_queuedMode = -1;
}

/* the intensity stepped by a level: under 1 calm, up to 1 under 3, up to 2 under 5, 3 above
 * (SOCOM II's; name ours) */
void CSndJukebox::SetIntensity(float level)
{
    MUSIC_MODE old = m_mode;

    if (level < 1.0f)
        m_mode = MUSIC_MODE_0;
    else if (level < 3.0f && m_mode < MUSIC_MODE_1)
        m_mode = MUSIC_MODE_1;
    else if (level >= 3.0f && level < 5.0f && m_mode < MUSIC_MODE_2)
        m_mode = MUSIC_MODE_2;
    else if (level >= 5.0f)
        m_mode = MUSIC_MODE_3;
    m_modeChanged = m_mode != old;
}

void CSndJukebox::Start()
{
    m_inFight = false;
    ClearQueue();
}
