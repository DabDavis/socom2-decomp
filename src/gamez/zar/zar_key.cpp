/* zar_key.cpp -- zar::CKey, a key of the archive's directory: its children by name, read and
 * written with the directory (OVL1 0x25AE00-...). The unit name is ours: zar_search.cpp's fetches
 * and inserts call FindKey and InsertKey out of line, where Read here inlines InsertKey -- the same
 * order as SOCOM 1's (FindKey, Read, Write, InsertKey after its zar_search.cpp fetches). Defined in
 * reverse of their image order (-inline deferred). */

#include <algorithm>
#include <string.h>
#include "zar.h"

namespace zar {

/* the children's teardown (0x25B580, SOCOM II's; its flag 0 here), not yet written */
extern "C" void func_0025B580(CKey *key, int flag);

/* a key named so, empty (SOCOM 1's; CZAR's constructor makes the root with "") */
CKey::CKey(const char *name) : m_name((char *)name), m_size(0), m_offset(0)
{
}

/* SOCOM 1's destroyed only its list; SOCOM II's first hands itself to 0x25B580 */
CKey::~CKey()
{
    func_0025B580(this, 0);
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
