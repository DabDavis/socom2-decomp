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

/* every key of the open one: opened, sought, fetched in place and handed to func, which takes it
 * when it answers more than nothing and no more than the key's size; the keys taken counted (SOCOM
 * 1's FetchAll -- the same walk, its end taken once; SOCOM 1's called FetchLIP out of line and
 * returned whether any were taken). The fetch is FetchLIP's body written with the seek's test in
 * the if (inlined, FetchLIP itself -- its `ok = seek == offset; if (ok)`, which its own row needs --
 * keeps the compare materialised, 8 bytes long) and the key's offset read at each use: one shared
 * temporary, numbered after the scalarised iterator, so it colours first and takes s0 as the
 * target's (a named offset local is numbered before the iterator and swaps the pair: 15 off) */
int CZAR::FetchAll(int (*func)(CZAR &zar, const char *name, void *buf, int size, void *data), void *data)
{
    int count = 0;
    CKey *open = GetOpenKey();
    CKeyRing::iterator e = open->end();

    for (CKeyRing::iterator i = open->begin(); i != e; ++i) {
        CKey *key = *i;
        bool ok = false;

        if (IsOpen() && OpenKey(key)) {
            ok = m_pFile->fseek(key->m_offset, 0) != -1;
            if (ok) {
                void *buf = 0;
                CBufferIO *io = dynamic_cast<CBufferIO *>(m_pFile);

                if (IsOpen() && OpenKey(key)) {
                    if (key->m_size) {
                        if (m_pFile->fseek(key->m_offset, 0) == key->m_offset)
                            io->fread(key->m_size, &buf);
                    }
                    CloseKey(key);
                }
                int n = func(*this, key->m_name, buf, key->m_size, data);

                ok = n > 0 && n <= key->m_size;
            }
            CloseKey(key);
        }
        if (ok)
            count++;
    }
    return count;
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

/* a key's data appended: to the second file at its end, or to the buffer at 0x34 when there is
 * none; the key given the data's place and size, the data padded with 0xAF to m_data_padded
 * (SOCOM II's: SOCOM 1's encrypted the data into the one file) */
bool CZAR::Insert(CKey *key, const void *data, int size)
{
    bool ok = false;

    if (data && size > 0) {
        int pos = m_pDataFile ? m_pDataFile->fseek(0, 2) : 0;

        ok = pos != -1;
        if (ok) {
            int at = 0;

            if (m_pDataFile)
                ok = m_pDataFile->fwrite(data, size) == size;
            else {
                if (m_34)
                    at = m_34->m_used;
                ok = WriteBuffer(data, size) == size;
            }
            if (ok) {
                key->m_offset = m_pDataFile ? pos : at;
                key->m_size = size;

                unsigned int rem = size % m_data_padded;
                unsigned int pad;

                if (rem)
                    pad = m_data_padded - rem;
                else
                    pad = 0;
                if (pad) {
                    char *buf = (char *)_malloc(pad, 0, 0);

                    memset(buf, (char)0xAF, pad);
                    if (m_pDataFile)
                        m_pDataFile->fwrite(buf, pad);
                    else
                        WriteBuffer(buf, pad);
                    if (buf)
                        _free(buf, 0, 0);
                }
            }
        }
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
