/* zar_search.cpp -- zar::CZAR's fetches by key and by name. The unit name is SOCOM 1's: its
 * Fetch(const char *, char **) passes "zar_search.cpp" to _calloc where its CZAR constructor
 * passes "zar_main.cpp" -- and here GetOpenKey, OpenKey(CKey *) and CloseKey are called out of
 * line, where zar_main.cpp's OpenKey(const char *) inlines OpenKey(CKey *). Defined in reverse of
 * their image order (-inline deferred). */

#include <algorithm>
#include <string.h>
#include "zar.h"
#include "../zsys/zsys.h"

namespace zar {

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

/* the typed fetches: SOCOM 1's call Fetch(CKey *, ...), which SOCOM II's, without the
 * decryption, inline */
bool CZAR::Fetch(const char *name, bool *value)
{
    unsigned char b;
    CKey *open = GetOpenKey();
    bool ok = open ? Fetch(open->FindKey(name), &b, sizeof(b), 0) : 0;

    if (ok)
        *value = b != 0;
    return ok;
}

bool CZAR::Fetch(const char *name, int *value)
{
    CKey *open = GetOpenKey();

    return open ? Fetch(open->FindKey(name), value, sizeof(int), 0) : 0;
}

bool CZAR::Fetch(const char *name, unsigned int *value)
{
    CKey *open = GetOpenKey();

    return open ? Fetch(open->FindKey(name), value, sizeof(unsigned int), 0) : 0;
}

bool CZAR::Fetch(const char *name, float *value)
{
    CKey *open = GetOpenKey();

    return open ? Fetch(open->FindKey(name), value, sizeof(float), 0) : 0;
}

/* a string: the key's bytes in a buffer one longer, zeroed (SOCOM 1's) */
bool CZAR::Fetch(const char *name, char **str)
{
    if (!IsOpen())
        return false;

    bool ok = false;
    int size;

    if ((size = GetSize(name)) > 0) {
        *str = (char *)_calloc(size + 1, 1, 0, 0);
        char *buf = *str;
        CKey *open = GetOpenKey();

        ok = open ? Fetch(open->FindKey(name), buf, size, 0) : 0;
    }
    return ok;
}

bool CZAR::Fetch(const char *name, void *buf, int size)
{
    CKey *open = GetOpenKey();

    return open ? Fetch(open->FindKey(name), buf, size, 0) : 0;
}

/* the key opened, sought and read; false when the key holds fewer than size bytes */
bool CZAR::Fetch(CKey *key, void *buf, int size, unsigned int flags)
{
    bool ok = false;

    if (IsOpen() && OpenKey(key)) {
        if (key->m_size) {
            int offset = key->m_offset;

            ok = m_pFile->fseek(m_data_start + offset, 0) == m_data_start + offset;
            if (ok && size <= key->m_size)
                ok = m_pFile->fread(buf, size) == size;
        }
        CloseKey(key);
    }
    return ok;
}

/* a key's data in place: the archive's file must be a CBufferIO, whose fread(size, &p) hands out
 * a pointer into its buffer (SOCOM 1's; SOCOM II's seeks without the data start) */
bool CZAR::FetchLIP(CKey *key, void **buf)
{
    CBufferIO *io = dynamic_cast<CBufferIO *>(m_pFile);
    bool ok = false;

    if (IsOpen() && OpenKey(key)) {
        if (key->m_size) {
            int offset = key->m_offset;

            ok = m_pFile->fseek(offset, 0) == offset;
            if (ok)
                ok = io->fread(key->m_size, buf);
        }
        CloseKey(key);
    }
    return ok;
}

}
