/* zar_search.cpp -- zar::CZAR's fetches by key and by name. The unit name is SOCOM 1's: its
 * Fetch(const char *, char **) passes "zar_search.cpp" to _calloc where its CZAR constructor
 * passes "zar_main.cpp" -- and here GetOpenKey, OpenKey(CKey *) and CloseKey are called out of
 * line, where zar_main.cpp's OpenKey(const char *) inlines OpenKey(CKey *). Defined in reverse of
 * their image order (-inline deferred). */

#include "zar.h"
#include "../zsys/zsys.h"

namespace zar {

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

/* a new key under the open one, if its data could be written (SOCOM 1's) */
CKey *CZAR::Insert(const char *name, const void *data, int size)
{
    CKey *key = NewKey(name);
    CKey *open = GetOpenKey();

    if (Insert(key, data, size))
        open->InsertKey(key);
    else {
        delete key;
        key = 0;
    }
    return key;
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
