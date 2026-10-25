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

/* SOCOM 1's: an intensity's list of sequences made, each weighted alike.
 * OPEN, 8 instructions, registers only: in both push_backs the target holds &size_ in the lower
 * saved register and &data_ in the higher (s0/s2, s1/s2), here the other way round -- include/
 * vector's pod push_back (insert(data_ + size_, ...)) creates them in the other order. One
 * counter for both loops, unsigned and compared as (int)i in the first, gives everything else
 * (an int counter in the first and an unsigned one in the second take an extra register) */
void CSndJukebox::LoadMusicLists(_zrdr *rdr, int mode)
{
    if (!(rdr && mode < 4))
        return;
    unsigned int i;

    for (i = 0; (int)i < (int)rdr->Count(); i++) {
        CSndSequence *seq = new CSndSequence;

        seq->CreateSequence(rdr->Item(i));
        m_playlists[mode].push_back(seq);
    }
    for (i = 0; i < m_playlists[mode].size(); i++)
        m_weights[mode].push_back(1.0f / m_playlists[mode].size());
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

/* SOCOM 1's: an intensity's weights scaled to sum to one, or made equal when they sum to 0.
 * OPEN, registers only (75 instructions differ, the same blocks and size): the target keeps the
 * mode in a2, its offset in v1 and builds each address in a0, here a1, a2 and v1 -- CSndGroup::
 * NormalizeWeights' open register question over m_weights[mode]. Tried: a reference to the
 * mode's vector (0x208), the mode copied to an int (0x210) */
void CSndJukebox::NormalizeWeights(MUSIC_MODE mode)
{
    float total = 0.0f;

    for (int i = 0; i < m_weights[mode].size(); i++)
        total += m_weights[mode][i];
    if (total == 0.0f) {
        float w = 1.0f / m_weights[mode].size();

        for (int i = 0; i < m_weights[mode].size(); i++)
            m_weights[mode][i] = w;
    } else {
        float scale = 1.0f / total;

        for (int i = 0; i < m_weights[mode].size(); i++)
            m_weights[mode][i] *= scale;
    }
}

void CSndJukebox::Start()
{
    m_jukebox_stopped = false;
    ClearQueue();
}
