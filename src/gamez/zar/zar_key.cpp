/* zar_key.cpp -- zar::CKey, a key of the archive's directory: its children by name, read and
 * written with the directory (OVL1 0x25AE00-...). The unit name is ours: zar_search.cpp's fetches
 * and inserts call FindKey and InsertKey out of line, where Read here inlines InsertKey -- the same
 * order as SOCOM 1's (FindKey, Read, Write, InsertKey after its zar_search.cpp fetches). Defined in
 * reverse of their image order (-inline deferred). */

#include <algorithm>
#include <string.h>
#include "zar.h"

namespace zar {

/* a key named so, empty (SOCOM 1's; CZAR's constructor makes the root with "") */
CKey::CKey(const char *name) : m_name((char *)name), m_size(0), m_offset(0)
{
}

CKey::CKey() : m_name(0), m_size(0), m_offset(0)
{
}

/* SOCOM 1's destroyed only its list; SOCOM II's first forgets its children (DeleteChildren,
 * 0x25B580, not yet written) */
CKey::~CKey()
{
    DeleteChildren(false);
}

/* a key named so, skipping the deleted (a size of -1): SOCOM 1's predicate, the struct's name ours */
struct CKeyNamed {
    CKeyNamed(const char *name) : m_name(name) {}
    bool operator()(const CKey *key) const
    {
        return key->m_size != -1 && !strcasecmp(m_name, key->m_name);
    }

    const char *m_name;
};

/* a key's directory record (SOCOM II's, called out of line by WriteDirectory) */
int CKey::RecordSize()
{
    return 16;
}

/* the keys counted, the key itself included, its children's own when deep (SOCOM II's, 0x25B300:
 * WriteDirectory sizes the directory with it on the root, true). Recursive: it self-inlines four
 * levels, each cutting vector<CKey *>'s pieces a level more (the MSL headers round 24, on the
 * image's pointer iterator). The end taken once ahead of the loop; the flag tested `!= false`,
 * so MW hoists the narrowed flag and tests it with beqz -- `deep && ...` hoists `!deep` instead
 * (sltu/xori, bnez), a bool copy or the end in the test change the copies' sizes.
 * OPEN, one call (0x240 both, every instruction the target's): the second copy's end() inlines
 * the raw end and calls the pod's end 0x34D920 where the target calls CKey's raw end 0x257BA0 --
 * one level deeper; `#pragma inline_max_size` 240 to 300 does not move it (120-238: 0x18C), and every
 * spelling with the end in the loop test (as _countKeys' and fixupKey's, byte-exact) is 0x1B8-0x27C;
 * round 15: the end or the iterator assigned rather than initialised 0x19C-0x1A4, the end first 20
 * off (build/scratch/core/v_ck*.py) */
int CKey::CountKeys(bool deep)
{
    int count = 1;

    for (iterator i = begin(), e = end(); deep != false && i != e; ++i)
        count += (*i)->CountKeys(true);
    return count;
}

/* a child key added at the end (SOCOM 1's inserted into a list) */
CKey *CKey::InsertKey(CKey *key)
{
    push_back(key);
    return key;
}

/* the directory record: the name's offset, the data's offset, its size, the child count */
struct KEY_RECORD {
    int name;
    int offset;
    int size;
    int count;
};

bool CKey::Read(CZAR *zar, CIO *io, int base)
{
    /* the file through a copy: numbered after the parameters, it takes s1 where io would take s2
     * (README "MW's saved-register order is a NUMBERING") */
    CIO *file = io;
    KEY_RECORD rec;

    file->fread(&rec, sizeof(rec));
    if (zar->m_root != this) {
        m_name = (char *)(rec.name + base);
        m_offset = rec.offset;
        m_size = rec.size;
    }

    int count = rec.count;

    if (count < 0)
        count = 0;
    if (count)
        reserve(count);
    while (count--) {
        CKey *key = zar->NewKey(0);

        key->Read(zar, file, base);
        InsertKey(key);
    }
    return true;
}

/* a child key by name (SOCOM 1's walked a std::list) */
CKey *CKey::FindKey(const char *name)
{
    if (empty())
        return 0;

    iterator it = std::find_if(begin(), end(), CKeyNamed(name));

    return it == end() ? 0 : *it;
}

}
