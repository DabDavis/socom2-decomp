/* zsnd_jukebox.cpp -- CSndJukebox, the music: playlists of sequences out of sndjukeboxm.rdr,
 * played by intensity (OVL1 0x348860-0x349A6F). The unit name is SOCOM 1's (its
 * __sinit_zsnd_jukebox.cpp, paired with SOCOM II's 0x4032E0). Defined in reverse of their image
 * order (-inline deferred). */

#include "zsnd.h"

/* the finder (zrdr_main.cpp; zanim.h still calls it by its placeholder) */
_zrdr *zrdr_findtag(const _zrdr *rdr, const char *tag);
#include "../zanim/zanim.h"
#include "../zsys/zsys.h"

class CRdrFile;
void zrdr_free(CRdrFile *file);
_zrdr *zrdr_read(const char *name, const char *file, int line);

/* the playlists and their weights, by intensity: the unit's initialiser 0x4032E0 builds both arrays
 * through __construct_array and registers their array destructors (0x349A50, 0x349A30) */
std::vector<CSndSequence *> CSndJukebox::m_playlists[4];
std::vector<float> CSndJukebox::m_weights[4];

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
        _zrdr *jukebox = zrdr_findtag(rdr, "JUKEBOX");

        if (jukebox) {
            _zrdr *fight = zrdr_findtag(jukebox, "FIGHT_MUSIC");
            _zrdr *medium = zrdr_findtag(jukebox, "MED_FIGHT_MUSIC");
            _zrdr *heavy = zrdr_findtag(jukebox, "HEAVY_FIGHT_MUSIC");

            LoadMusicLists(zrdr_findtag(jukebox, "STEALTH_MUSIC"), 0);
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

/* SOCOM 1's: the music ticked -- a finished sequence or instance let go and the next started, then
 * the next started when the music's state no longer suits the mode (SOCOM II's states: 1 and 2 the
 * fight music, 0 the calm, -1 none). The sequence's verdict is kept in a bool (its `andi`); the
 * mode is read as a signed byte here (`lb`, where SetIntensity and PlayQueued read it `lbu`) --
 * the casts are stand-ins for whatever the original read it through */
void CSndJukebox::Tick(float dT)
{
    if (m_jukebox_stopped)
        return;
    if (m_sequence) {
        bool done = m_sequence->Tick(dT);

        if (done) {
            m_sequence = 0;
            m_currentmode = -1;
            PlayQueued();
        }
    } else if (m_instance && !m_instance->IsPlaying()) {
        m_currentmode = -1;
        CSndInstance::ReleaseSoundInstance(m_instance);
        m_instance = 0;
        PlayQueued();
    } else if (!m_instance) {
        m_currentmode = -1;
        PlayQueued();
    }
    if (m_currentmode == 1 && (signed char)m_mode != MUSIC_MODE_0)
        PlayQueued();
    if ((m_currentmode == -1 || m_currentmode == 0 || (m_currentmode == 2 && m_modeChanged)) &&
        (signed char)m_mode != MUSIC_MODE_0) {
        PlayQueued();
        m_modeChanged = false;
    } else if ((m_currentmode == 2 || m_currentmode == -1) && (signed char)m_mode == MUSIC_MODE_0) {
        FadeOut();
        PlayQueued();
    }
}

/* the queued music played (the heaviest at once, the medium dropped, the rest faded in), else the
 * fight music for a fight mode or the calm music faded in for the calm one (SOCOM II's; SOCOM 1's
 * Tick inlined its own five times, the name ours). The queued mode, a plain char, is compared as
 * a signed byte (lb, slti) and handed to Play unconverted -- no `andi`, which `(MUSIC_MODE)` adds:
 * the reinterpreting read is a stand-in (SOCOM 1's Tick passes it the same way) */
void CSndJukebox::PlayQueued()
{
    if (m_queuedMode > MUSIC_MODE_2) {
        Play(*(MUSIC_MODE *)&m_queuedMode, (CSnd *)m_queued);
        ClearQueue();
    } else if (m_mode > MUSIC_MODE_0)
        Play(MUSIC_MODE_2, 0);
    else if (m_queuedMode > MUSIC_MODE_0) {
        if (m_queuedMode != MUSIC_MODE_2) {
            FadeOut();
            Play(*(MUSIC_MODE *)&m_queuedMode, (CSnd *)m_queued);
            ClearQueue();
        } else
            ClearQueue();
    } else if (m_mode == MUSIC_MODE_0) {
        FadeOut();
        Play(MUSIC_MODE_0, 0);
    }
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

/* SOCOM 1's: music asked for -- what plays faded out first for a higher mode; while anything plays
 * it fades and the request is queued unless a higher one is; else the mode's music started: the
 * calm and medium playlists at random, the sound itself for the fight and heavy modes (SOCOM II's
 * modes 0-3, the mode taken as a signed byte against the state) */
void CSndJukebox::Play(MUSIC_MODE mode, CSnd *sound)
{
    signed char m = mode;

    if (m_currentmode < m)
        FadeOut();
    if (m_sequence) {
        if (!m_sequence->IsFading())
            m_sequence->FadeOut();
        if (m >= m_queuedMode) {
            m_queuedMode = mode;
            m_queued = (int)sound;
        }
        return;
    }
    if (m_instance) {
        if (!m_instance->IsFading())
            m_instance->FadeOut(2.0f);
        if (m >= m_queuedMode) {
            m_queuedMode = mode;
            m_queued = (int)sound;
        }
        return;
    }
    switch (m) {
    case MUSIC_MODE_0:
        if (m_currentmode < 0)
            PlayRandom(MUSIC_MODE_0);
        break;
    case MUSIC_MODE_1:
        if (m_currentmode <= 0) {
            if (m_sequence) {
                m_sequence->Stop();
                m_sequence = 0;
            }
            if (m_instance)
                CSndInstance::ReleaseSoundInstance(m_instance);
            m_instance = sound->PlaySound(false, 0, 255);
            m_currentmode = mode;
        }
        break;
    case MUSIC_MODE_2:
        if (m_currentmode < 2)
            PlayRandom(m_mode);
        break;
    case MUSIC_MODE_3:
        if (m_sequence) {
            m_sequence->Stop();
            m_sequence = 0;
        }
        if (m_instance)
            CSndInstance::ReleaseSoundInstance(m_instance);
        m_instance = sound->PlaySound(false, 0, 255);
        m_currentmode = mode;
        break;
    }
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

/* SOCOM 1's: a sequence of the mode picked at random by weight -- its weight cut to a tenth and the
 * weights normalised again -- and played in place of the instance; SOCOM II's music state is 2 for
 * a fight mode, 0 for the calm one.
 * Byte-exact (the world lane's round 33; open at 11 since round 9) through a MARKED STAND-IN: the
 * target builds &m_playlists[mode], then &...data_ (+8) in s0 for the second read, where MW folds a
 * plain `m_playlists[mode].data_` into `(base + 8) + off`. A no-op conversion of the element's
 * address (PlaylistOf) stops the fold -- the original spelling is unknown (a subclass's conversion to
 * the vector would do the same). Tried before: a reference to the playlist inside the block or
 * before the loop, a pointer, the pod's data() reference, begin() + i, begin().m_p[i] (0x1A4-0x1C8),
 * an inline returning it by reference or pointer (12). The counter an int (unsigned
 * strength-reduces the element address), `sum` declared ahead of `r` ($f21/$f20), `return` (a
 * `break` drops the block's exit, 0x1B8) */
template <class T>
static inline T *PlaylistOf(T *p)
{
    return (T *)(int)p;
}

void CSndJukebox::PlayRandom(MUSIC_MODE mode)
{
    float sum;
    float r = 4.656613e-10f * (float)rand();

    sum = 0.0f;

    for (int i = 0; i < m_weights[mode].size(); i++) {
        sum += m_weights[mode][i];
        if (sum > r) {
            m_weights[mode][i] *= 0.1f;
            NormalizeWeights(mode);
            if (i < m_playlists[mode].size()) {
                (*PlaylistOf(&m_playlists[mode]))[i]->Play();
                if (m_instance)
                    CSndInstance::ReleaseSoundInstance(m_instance);
                m_instance = 0;
                m_sequence = (*PlaylistOf(&m_playlists[mode]))[i];
                if (mode > MUSIC_MODE_0)
                    m_currentmode = 2;
                else if (mode == MUSIC_MODE_0)
                    m_currentmode = 0;
                return;
            }
        }
    }
}

/* SOCOM 1's: an intensity's weights scaled to sum to one, or made equal when they sum to 0. One
 * counter for the three loops, declared at the top (as CSndGroup::NormalizeWeights): a counter per
 * loop colours the mode's 12-byte offset after the unrolled sum's temporaries (75 off -- the world
 * lane's rounds 14 and 21) */
void CSndJukebox::NormalizeWeights(MUSIC_MODE mode)
{
    float total = 0.0f;
    int i;

    for (i = 0; i < m_weights[mode].size(); i++)
        total += m_weights[mode][i];
    if (total == 0.0f) {
        float w = 1.0f / m_weights[mode].size();

        for (i = 0; i < m_weights[mode].size(); i++)
            m_weights[mode][i] = w;
    } else {
        float scale = 1.0f / total;

        for (i = 0; i < m_weights[mode].size(); i++)
            m_weights[mode][i] *= scale;
    }
}

void CSndJukebox::Start()
{
    m_jukebox_stopped = false;
    ClearQueue();
}
