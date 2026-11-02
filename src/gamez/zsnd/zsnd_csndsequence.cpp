/* zsnd_csndsequence.cpp -- CSndSequence: sounds and pauses played one after another (OVL1
 * 0x349C70-0x34A61F). The unit name is SOCOM 1's: the press beta's asserts naming it sit in
 * CreateSequence and the destructor. The sequence's vector of entries closes the unit (insert, the
 * deleter's destructor and clear). Its entries, CSndSequenceEntry, are the unit before it,
 * zsnd_csndsequenceentry.cpp (0x349A70-0x349C6F): every entry function this unit uses is called out
 * of line, as -inline auto does only across units (README "The inlining mode, measured"; one unit
 * needed all six pinned). Defined in reverse of their image order (-inline deferred). */

#include "zsnd.h"
#include "../zanim/zanim.h"

/* SOCOM 1's zrdr_findstring and zrdr_findreal, not written yet */
char *zrdr_findstring(const _zrdr *rdr, const char *tag);
int zrdr_findreal(const _zrdr *rdr, const char *tag, float *out, int n);

CSndSequence::CSndSequence()
    : m_current(-1), m_name(0), m_fading(false)
{
}

/* the name freed and cleared twice, as the target does; the entries' vector inlines two levels
 * of its destructor (vector's and __vector_imp's this tests) and calls the deleter's (0x34A5A0),
 * which include/vector defines out of class. The deleter's own copy still differs: it calls clear
 * (0x34A610) out of line where nav_package's inlines its clear -- SOCOM 1's clear walks the
 * entries backwards, so CSndSequenceEntry has a user-declared (empty) destructor, and MSL's
 * non-trivial destroy path is not modelled yet (the draft of 0x34A5A0 keeps its match) */
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
    char *name = zrdr_findstring(rdr, "SEQ_NAME");

    if (name) {
        if (m_name)
            _free(m_name, 0, 0);
        m_name = 0;
        m_name = _strdup(name, 0, 0);
    }
    float pause = 0.0f;

    zrdr_findreal(rdr, "PAUSETIME", &pause, 1);
    /* the counter unsigned: as an int, MW strength-reduces the item's address (a register more) */
    for (unsigned int i = 2; (int)i < (int)rdr->Count(); i++) {
        _zrdr *item = rdr->Item(i);
        float delay = 0.0f;
        CSnd *sound = 0;

        zrdr_findreal(item, "PAUSETIME", &delay, 1);
        char *sname = zrdr_findstring(item, "SNDNAME");

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
