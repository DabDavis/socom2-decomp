/* util_loc.cpp -- the localised string tables (OVL1 0x3A3100-0x3A54FF): LOC.rdr's tables per language
 * (RUN\LOCALE\<language>, DATA\COMMON\ZRDR\LOCRDR\<language>\<table>LOC.rdr), the language choice
 * (LanguageChoice, from the system's when it is unset) and the lookups by id and by name. SOCOM 1
 * has none of it: the file's name and every name here are ours. Defined in reverse of their image
 * order (-inline deferred).
 *
 * OPEN, not written yet: everything but the table's lookup by id -- the manager's lookups
 * (0x3A3100, 0x3A32F0), the loaders (0x3A3490, 0x3A3900, 0x3A3BE0), the teardown 0x3A3740, the
 * language's (0x3A40E0, 0x3A44B0), the by-name lookup 0x3A41F0, the sort 0x3A4560 and the template
 * tail (the 12-byte entries' vector: insert 0x3A4D10, reserve 0x3A5220, clear 0x3A5400, the
 * destructor 0x3A5450, the sort's helpers 0x3A5500, 0x3A57E0). */

/* a localised string: its id, then its text and how it is shown (ours: 12 bytes, the id a halfword
 * at 0) */
struct LOCENTRY {
    unsigned short m_id;   // 0x0
    unsigned char _2[0xa];
};   // 0xc bytes

/* the entries of one table (ours; the vector's words at 0x24) */
struct LOCENTRIES {
    unsigned int m_capacity;   // 0x24
    unsigned int m_size;   // 0x28
    LOCENTRY *m_data;   // 0x2c

    unsigned int size() const { return m_size; }
    LOCENTRY &operator[](unsigned int i) { return m_data[i]; }
};

/* one table of localised strings (ours) */
class CLocTable {
public:
    LOCENTRY *Find(int id);

    int m_maxId;   // 0x0
    unsigned char _4[0x24 - 0x4];
    LOCENTRIES m_entries;   // 0x24
    unsigned char _30[0x70 - 0x30];
    LOCENTRY *m_last;   // 0x70: the entry found last
};

/* the entry with this id: the one found last, the first when the id is not past it, else the
 * entries walked (the walk's find kept).
 * OPEN, 2 instructions long (0xC0 of 0xB8): the target loads the data pointer into v0 for the first
 * entry's test and the `return first` path, and walks a copy of it (`move a2,v0` in the size test's
 * delay slot); here the pointer lands in a2 and the first-entry return moves it to v0. The block
 * order (the first-entry test branching straight to the shared return, i = 0 in its delay slot)
 * needed the walk nested in the test with i declared first; an unsigned walk index read through an
 * operator[] keeps the per-pass element address; `id == x` puts the parameter first in each bne.
 * Tried: a pointer local for the first entry (before or after i), the test inverted, `id <=`. */
LOCENTRY *CLocTable::Find(int id)
{
    if (id <= 0 || id > m_maxId)
        return 0;
    if (m_last && id == m_last->m_id)
        return m_last;
    int i = 0;

    if (m_entries[0].m_id < id) {
        for (; i < m_entries.size(); i++) {
            LOCENTRY *e = &m_entries[i];

            if (id == e->m_id) {
                m_last = e;
                return e;
            }
        }
        return 0;
    }
    return &m_entries[0];
}
