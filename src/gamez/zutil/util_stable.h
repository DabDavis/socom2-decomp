/* util_stable.h -- the engine's string table CSTable (util_stable.cpp). SOCOM 1's DWARF puts the
 * class in zutil.h; it is kept apart here while zrdr.h (the rdr lane's) carries its own 0x20-byte
 * stand-in of the same name, which a definition in zutil.h would collide with (zrdr.h includes
 * zar.h, which includes zutil.h). A unit includes this or zrdr.h, never both.
 *
 * SOCOM 1's CSTable (DWARF: a vector of the strings, then the owner flag, the buffer, its bytes
 * used and its size, 0x1C bytes) with a vtable SOCOM II added after the vector, at 0xC: Clear,
 * Destroy and FindString made virtual, and two hooks of its own -- told of each string created
 * (empty in both tables) and asked for room when the table is full (CSTable's returns 0;
 * CUIVarSTable's collects the UI variables' garbage and creates the string in the manager's
 * current table). The destructor is not virtual: the table has no slot for it. */
#ifndef UTIL_STABLE_H
#define UTIL_STABLE_H

#include <vector>
#include "zutil.h"

class CSTable : public std::vector<char *> {
public:
    CSTable(unsigned int bytes, unsigned int strings);
    ~CSTable();
    virtual void Clear();                                  // 0x08
    virtual void Destroy();                                // 0x0C
    virtual char *FindString(const char *str);             // 0x10: not const (SOCOM 1's was)
    /* (the names ours) */
    virtual void OnCreateString(char *str) {}              // 0x14
    virtual char *OnTableFull(const char *str);            // 0x18
    char *CreateString(const char *str);
    unsigned int Pack(void (*fn)(CSTable *, void *), void *data);
    int Relocate(void *buf, unsigned int reserve, bool owner);
    void LoadTable(void *buf, unsigned int bytes, bool owner);
    /* the strings are packed in a buffer (ours: Clear, Destroy and Relocate test it and load the
     * buffer again) */
    bool IsPacked() const { return m_buffer != 0; }
    /* the buffer let go of, freed if asked (SOCOM 1's out of line; inlined in both of SOCOM II's
     * callers and absent) */
    void ReleaseBuffer(bool free)
    {
        if (free) {
            if (m_buffer)
                _free(m_buffer, 0, 0);
            m_buffer = 0;
        }
        m_buffer = 0;
    }

    bool m_owner;              // 0x10: the buffer is the table's to free
    char *m_buffer;            // 0x14: the strings packed, or 0 (each string its own allocation)
    unsigned int m_bytes;      // 0x18: the buffer's bytes used
    unsigned int m_reserve;    // 0x1c: the buffer's size
};   // 0x20 bytes

#endif
