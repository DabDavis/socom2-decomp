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

/* the records with the hash of a name: its two words given back and true; else the table walked
 * (its dump compiled out) and false.
 * OPEN, 9 instructions, registers only: the target reads the record's three words before the test
 * (hash, then the second, then the first in the branch's delay slot -- read into locals first, as
 * here, gives the right size; through the record by reference it is 8 bytes longer, a by-value
 * record copy 0x38 longer) and keeps the iterator in a3, the second word in a1, the first in a2,
 * comparing `bne record, hash`; every order of the three locals and of the iterator's declaration
 * tried leaves the iterator in a1 (build/scratch/wld/v_find*.py) */
bool CSubtitleTable::Find(const char *name, int *a, int *b)
{
    unsigned int hash = func_003A59C0(name);

    for (std::vector<CSubtitleEntry>::iterator i = m_entries.begin(); i != m_entries.end(); ++i) {
        unsigned int h = (*i).m_hash;
        int y = (*i).m_4;
        int x = (*i).m_0;

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
void CSubtitleTable::Dump()
{
    for (std::vector<CSubtitleEntry>::iterator i = m_entries.begin(); i != m_entries.end(); i++)
        ;
}
#pragma auto_inline reset

/* both vectors empty, the flag clear, then room for 10 records and 11500 lines */
CSubtitleTable::CSubtitleTable() : m_20(0)
{
    m_entries.reserve(10);
    m_lines.reserve(11500);
}
