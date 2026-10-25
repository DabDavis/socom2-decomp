/* zsnd_jukebox.cpp -- CSndJukebox, the music: playlists of sequences out of sndjukeboxm.rdr,
 * played by intensity (OVL1 0x348860-0x349A6F). The unit name is SOCOM 1's (its
 * __sinit_zsnd_jukebox.cpp, paired with SOCOM II's 0x4032E0). Defined in reverse of their image
 * order (-inline deferred). */

#include "zsnd.h"
#include "../zanim/zanim.h"
#include "../zsys/zsys.h"

class CRdrFile;
void zrdr_free(CRdrFile *file);
_zrdr *zrdr_read(const char *name, const char *file, int line);

/* SOCOM 1's: the playlists freed, the music state reset */
void CSndJukebox::Cleanup()
{
    for (int i = 0; i < 4; i++) {
        if (m_playlists[i].size()) {
            for (std::vector<CSndSequence *>::iterator s = m_playlists[i].begin();
                 s != m_playlists[i].end(); ++s)
                delete *s;
            FreeVector(m_playlists[i]);
        }
        FreeVector(m_weights[i]);
    }
    m_instance = 0;
    m_currentmode = -1;
    m_jukebox_stopped = true;
    m_sequence = 0;
    m_playlists_loaded = false;
}

/* SOCOM 1's: sndjukeboxm.rdr's JUKEBOX record read into the four intensities' lists (SOCOM 1 had
 * two) */
void CSndJukebox::LoadPlaylists(const char *name)
{
    if (m_playlists_loaded == 1)
        Cleanup();
    _zrdr *rdr = zrdr_read(name, 0, 0);

    if (rdr) {
        _zrdr *jukebox = func_0032F0D0(rdr, "JUKEBOX");

        if (jukebox) {
            _zrdr *fight = func_0032F0D0(jukebox, "FIGHT_MUSIC");
            _zrdr *medium = func_0032F0D0(jukebox, "MED_FIGHT_MUSIC");
            _zrdr *heavy = func_0032F0D0(jukebox, "HEAVY_FIGHT_MUSIC");

            LoadMusicLists(func_0032F0D0(jukebox, "STEALTH_MUSIC"), 0);
            LoadMusicLists(fight, 1);
            LoadMusicLists(medium, 2);
            LoadMusicLists(heavy, 3);
        }
    }
    if (rdr)
        zrdr_free((CRdrFile *)rdr);
}

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
    m_jukebox_stopped = false;
    ClearQueue();
}
