/* zsnd_csndsequence.cpp -- CSndSequence and its entries: sounds and pauses played one after
 * another (OVL1 0x349A70-0x34A61F). The unit name is SOCOM 1's (its asserts' file name); the
 * sequence's vector of entries closes the unit (insert, the deleter's destructor and clear).
 * Defined in reverse of their image order (-inline deferred). */

#include "zsnd.h"
#include "../zanim/zanim.h"

/* SOCOM 1's zrdr_findstring and zrdr_findreal, not written yet */
extern "C" char *func_0032EE10(_zrdr *rdr, const char *tag);
extern "C" int func_0032EA80(_zrdr *rdr, const char *tag, float *out, int n);

CSndSequence::CSndSequence()
    : m_current(-1), m_name(0), m_fading(false)
{
}

/* OPEN, 0x98 bytes against 0x80: the entries' vector. The target inlines two levels of its
 * destructor (vector's and __vector_imp's this tests) and calls the deleter's (0x34A5A0), which
 * calls clear (0x34A610) out of line -- where nav_package's deleter inlines its clear: SOCOM 1's
 * clear walks the entries backwards, so CSndSequenceEntry has a user-declared (empty) destructor
 * and MSL takes its non-trivial destroy path, which include/vector does not model (an element
 * destructor loop alone, tried, changes nothing here). The name is freed and cleared twice, as
 * the target does */
CSndSequence::~CSndSequence()
{
    if (m_name)
        _free(m_name, 0, 0);
    m_name = 0;
    m_name = 0;
}

void CSndSequence::CreateSequence(_zrdr *rdr)
{
    if (!rdr)
        return;
    char *name = func_0032EE10(rdr, "SEQ_NAME");

    if (name) {
        if (m_name)
            _free(m_name, 0, 0);
        m_name = 0;
        m_name = _strdup(name, 0, 0);
    }
    float pause = 0.0f;

    func_0032EA80(rdr, "PAUSETIME", &pause, 1);
    /* the counter unsigned: as an int, MW strength-reduces the item's address (a register more) */
    for (unsigned int i = 2; (int)i < (int)rdr->Count(); i++) {
        _zrdr *item = rdr->Item(i);
        float delay = 0.0f;
        CSnd *sound = 0;

        func_0032EA80(item, "PAUSETIME", &delay, 1);
        char *sname = func_0032EE10(item, "SNDNAME");

        if (sname)
            sound = CSnd::GetSoundByName(sname);
        m_entries.push_back(CSndSequenceEntry(sound, delay));
    }
}

bool CSndSequence::Tick(float dT)
{
    if (m_done)
        return true;
    int n = m_entries.size();

    /* n first: written m_current >= n, MW materialises the test in v0 */
    if (n <= m_current) {
        m_current = 0;
        m_done = true;
        return true;
    }
    bool next = true;

    if (m_current != -1)
        next = m_entries[m_current].Tick(dT);
    if (next) {
        m_current++;
        if (m_current >= n || m_fading) {
            m_done = true;
            return true;
        }
        m_entries[m_current].Play();
    }
    return false;
}

void CSndSequence::Play()
{
    m_current = -1;
    m_done = false;
    m_fading = false;
}

void CSndSequence::Stop()
{
    if (m_current != -1)
        m_entries[m_current].Stop();
    m_current = -1;
    m_done = true;
    m_fading = false;
}

bool CSndSequence::IsFading()
{
    if (m_current != -1)
        return m_entries[m_current].IsFading();
    return false;
}

void CSndSequence::FadeOut()
{
    if (m_current != -1) {
        m_entries[m_current].FadeOut();
        m_fading = true;
    } else
        Stop();
}

/* the entries' functions are pinned: the sequence's call every one of them */
#pragma auto_inline off

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

#pragma auto_inline reset
