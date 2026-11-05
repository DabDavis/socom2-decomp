/* zsnd_subtitles.cpp -- the subtitle table (OVL1 0x34D180-0x34E06F, SOCOM II's; the file, class and
 * member names are ours): two vectors of 12-byte records looked up by a string's hash, built by
 * zsnd_csnd.cpp's initialiser (0x402FF0 constructs the global 0x48DC30 through 0x34D500 and
 * registers its implicit destructor 0x346160, zsnd_csnd.cpp's tail); CSnd::Init loads it
 * (0x34D180, a wrapper of 0x34D190 with a null second argument), SetLanguage calls 0x34D190 again.
 * The unit sits between zsnd_vocalqueue.cpp and zsys_sched.cpp, closed by the records' reserve
 * (0x34DED0). Defined in reverse of their image order (-inline deferred).
 * Written so far: the constructor, the lookup and the empty walk it calls, the loader (0x34D190:
 * CSubtitleTable is the VAG location table -- each language's VAGS_<lang>.ZAR and VAGSTORE.ZAR, their
 * files' offsets, sizes and name hashes) and its first call (0x34D180), and since the MSL headers
 * round 24 (include/vector's pointer iterator as the image's) CSubtitleList::Load 0x34D5D0, the
 * next-file walk 0x34D740 it inlines one level (self-inlined three times, each copy calling the key
 * ring's pieces a level higher: the pod's end 0x34D920 at the second) and the records' insert(pos,
 * n, x) 0x34DA00. Not written: 0x34D460/0x34D470/0x34D480 (drafts), the tail. */

#include <stdio.h>
#include <vector>

#include "zsnd.h"
#include "zsnd_subtitles.h"
#include "../zar/zar.h"

/* the string hash the lookups key on (0x3A59C0, an earlier unit's: a tail call into the util
 * unit's hash, not named yet) */
extern "C" unsigned int func_003A59C0(const char *s);

/* the records with the hash of a name: its two words given back and true; else the records
 * walked (their dump compiled out) and false. The cursor is a plain pointer compared as an
 * iterator: a named local, numbered before the record's three words, it takes a3 as the target's
 * (a class iterator is split into a temporary numbered after them, a1 -- round 6's nine words) */
bool CSubtitleList::Find(const char *name, int *a, int *b)
{
    unsigned int hash = func_003A59C0(name);

    for (CSubtitleEntry *e = begin().m_p; iterator(e) != end(); ++e) {
        unsigned int h = e->m_hash;
        int x = e->m_0;
        int y = e->m_4;

        if (h == hash) {
            *a = x;
            *b = y;
            return true;
        }
    }
    Dump();
    return false;
}

/* the open key's next file from it on: the key under it, it stepped past it -- a directory (a
 * negative size) skipped for the one after it; 0 at the end. Recursive, so inlined one level into
 * Load and self-inlined in its own copy (0x34D740, after Load: the MSL headers round 24, on
 * include/vector's pointer iterator as the image's -- each copy cuts the key ring's pieces a
 * level more, the second calling the pod's end 0x34D920 out of line; the name ours) */
inline zar::CKey *NextFile(zar::CKey *key, zar::CKeyRing::iterator &it)
{
    zar::CKey *k = it != key->end() ? *it : 0;

    if (k) {
        it++;
        if (k->m_size < 0)
            return NextFile(key, it);
    }
    return k;
}

void CopyUpper(const char *src, char *dst, int max);

/* the open key's files listed: each one's data offset, size and upper-cased name's hash. The
 * loop tests at its top (`for (;;)`: a while is rotated, its test laid after the body behind an
 * entry jump, 8 bytes short); the offset, size and name declared in that order, so the name
 * (numbered last) takes s0 and the offset s2 (README "MW's saved-register order is a NUMBERING") */
bool CSubtitleList::Load(zar::CZAR *ar)
{
    zar::CKey *key = ar->GetOpenKey();
    zar::CKeyRing::iterator it = key->begin();

    for (;;) {
        zar::CKey *k = NextFile(key, it);

        if (!k)
            break;
        CSubtitleEntry e;
        char name[32];
        int ofs = ar->m_data_start + k->m_offset;
        int size = k->m_size;
        const char *s = k->m_name;

        strcpy(name, s);
        CopyUpper(s, name, 30);
        e.m_0 = ofs;
        e.m_4 = size;
        e.m_hash = func_003A59C0(name);
        push_back(e);
    }
    return true;
}

/* the records walked, the print compiled out (out of line: Find calls it) */
#pragma auto_inline off
void CSubtitleList::Dump()
{
    for (iterator i = begin(); i != end(); i++)
        ;
}
#pragma auto_inline reset

/* both vectors empty, the flag clear, then room for 10 records and 11500 lines */
CSubtitleTable::CSubtitleTable() : m_20(0)
{
    m_entries.reserve(10);
    m_lines.reserve(11500);
}

/* the language's VAG index (VAGS_<lang>.ZAR, English for the two English languages, none for the
 * rest) and VAGSTORE's read: each archive's start sector off the CD's table of contents, then its
 * files listed (the VAG index's into the first list, VAGSTORE's into the second); whether VAGSTORE
 * opened. The flag cleared first, set once a language is chosen; the lower-case name of each file
 * is written and never read (the host's path, its use compiled out?) */
bool CSubtitleTable::Load(LANGUAGE lang)
{
    char path[128];
    char name[128];
    bool ok;

    if (m_20)
        m_20 = 0;
    if (lang == LANG_KOREA) {
        sprintf(path, "RUN\\SOUNDS\\VAGS_KOR.ZAR");
        sprintf(name, "run/sounds/vags_kor.zar");
    } else if (lang == LANG_FRANCE) {
        sprintf(path, "RUN\\SOUNDS\\VAGS_FRN.ZAR");
        sprintf(name, "run/sounds/vags_frn.zar");
    } else if (lang == LANG_GERMANY) {
        sprintf(path, "RUN\\SOUNDS\\VAGS_GER.ZAR");
        sprintf(name, "run/sounds/vags_ger.zar");
    } else if (lang == LANG_SPAIN) {
        sprintf(path, "RUN\\SOUNDS\\VAGS_SPN.ZAR");
        sprintf(name, "run/sounds/vags_spn.zar");
    } else if (lang == LANG_ITALY) {
        sprintf(path, "RUN\\SOUNDS\\VAGS_ITL.ZAR");
        sprintf(name, "run/sounds/vags_itl.zar");
    } else if (lang == LANG_JAPAN) {
        sprintf(path, "RUN\\SOUNDS\\VAGS_JPN.ZAR");
        sprintf(name, "run/sounds/vags_jpn.zar");
    } else if (lang == LANG_STATES || lang == LANG_UK) {
        sprintf(path, "RUN\\SOUNDS\\VAGS_ENG.ZAR");
        sprintf(name, "run/sounds/vags_eng.zar");
    }
    m_20 = 1;
    {
        char cd[128];

        sprintf(cd, "%s;1", path);
        _FILECD *f = CFileCD::Find(cd);
        if (f)
            m_18 = f->startSector;
        zar::CZAR zar(0, 0);
        ok = zar.Open(path, 0, 33, 16, false);
        if (ok) {
            m_entries.Load(&zar);
            zar.Close();
        }
    }
    sprintf(path, "RUN\\SOUNDS\\VAGSTORE.ZAR");
    sprintf(name, "run/sounds/vagstore.zar");
    char cd[128];

    sprintf(cd, "%s;1", path);
    _FILECD *f = CFileCD::Find(cd);
    if (f)
        m_1c = f->startSector;
    else
        m_1c = 0;
    zar::CZAR zar(0, 0);
    ok = zar.Open(path, 0, 33, 16, false);
    if (ok) {
        m_lines.Load(&zar);
        zar.Close();
    }
    return ok;
}

/* the first load: the English index (CSnd::Init calls it; SetLanguage calls Load itself) */
void CSubtitleTable::Init()
{
    Load(LANG_STATES);
}
