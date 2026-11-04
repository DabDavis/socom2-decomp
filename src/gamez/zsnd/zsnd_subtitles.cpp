/* zsnd_subtitles.cpp -- the subtitle table (OVL1 0x34D180-0x34E06F, SOCOM II's; the file, class and
 * member names are ours): two vectors of 12-byte records looked up by a string's hash, built by
 * zsnd_csnd.cpp's initialiser (0x402FF0 constructs the global 0x48DC30 through 0x34D500 and
 * registers its implicit destructor 0x346160, zsnd_csnd.cpp's tail); CSnd::Init loads it
 * (0x34D180, a wrapper of 0x34D190 with a null second argument), SetLanguage calls 0x34D190 again.
 * The unit sits between zsnd_vocalqueue.cpp and zsys_sched.cpp, closed by the records' reserve
 * (0x34DED0). Defined in reverse of their image order (-inline deferred).
 * Written so far: the constructor, the lookup and the empty walk it calls. Not written: the loader
 * 0x34D190, 0x34D460/0x34D470/0x34D480, 0x34D5D0, 0x34D740, 0x34DA00, the tail. */

#include <vector>

#include "zsnd.h"
#include "zsnd_subtitles.h"

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
