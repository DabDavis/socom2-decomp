/* zar::CZAR -- the ZAR archive: a tree of named keys over a file, the zar namespace the mangled
 * names'. Shapes SOCOM 1's DWARF where SOCOM II kept them; SOCOM II made the key ring (a key's
 * children, and the archive's stack of open keys) a vector where SOCOM 1's was a std::list,
 * and grew the archive to 0xB8 bytes. The header's name is ours. */
#ifndef ZAR_H
#define ZAR_H

#include <vector>

/* the engine's file interface, CIO, is zutil.h's (the full class: SOCOM 1's slots and SOCOM II's
 * three async ones) */
#include "../zutil/zutil.h"

class CSTable;

namespace zar {

class CKey;

/* a key's children and the archive's open keys: SOCOM 1's std::list<CKey *>, a vector here (a
 * count at 4, the array at 8) */
class CKeyRing : public std::vector<CKey *> {
public:
    void push(CKey *key);
};   // 0xc bytes

class CZAR;

class CKey : public CKeyRing {
public:
    CKey(const char *name);
    /* out of line (0x25BB50): Insert(const char *, ...) deletes a key it could not write through it */
    ~CKey();
    CKey *FindKey(const char *name);
    /* the key and its children off the directory: a 16-byte record each, the name an offset
     * into the string table at base (SOCOM 1's) */
    bool Read(CZAR *zar, CIO *io, int base);
    CKey *InsertKey(CKey *key);

    char* m_name;   // 0xc
    int m_size;   // 0x10
    int m_offset;   // 0x14
};   // 0x18 bytes

class CZAR {
public:
    CZAR(const char *name, CIO *io);
    virtual ~CZAR();
    /* a key's data, when the key holds at least size bytes (SOCOM 1's flags argument unused) */
    /* a key's data written at the archive's end (0x25A120), and a new key under the open one
     * holding it -- the key, or 0 when the write failed */
    bool Insert(CKey *key, const void *data, int size);
    CKey *Insert(const char *name, const void *data, int size);
    /* a key's data in place, a pointer into the CBufferIO's buffer */
    bool FetchLIP(CKey *key, void **buf);
    bool Fetch(CKey *key, void *buf, int size, unsigned int flags);
    bool Fetch(const char *name, void *buf, int size);
    bool Fetch(const char *name, char **str);
    bool Fetch(const char *name, float *value);
    bool Fetch(const char *name, unsigned int *value);
    bool Fetch(const char *name, int *value);
    bool Fetch(const char *name, bool *value);
    /* a fresh key, named so (SOCOM 1's) */
    CKey *NewKey(const char *name);
    /* a key's size by name in the open key, -1 when it has none */
    int GetSize(const char *name);
    CKey *GetOpenKey();
    CKey *OpenKey(CKey *key);
    CKey *OpenKey(const char *name);
    CKey *FindKey(const char *name);
    void CloseKey(CKey *key);
    void Close();
    /* the file closed, the directory kept (SOCOM 1's: the file's Close, a tail call) */
    void CloseKeepDir();
    /* the archive's file name: a copy of the name given, or DEFAULT_ZAR_NAME's own pointer for
     * none (SOCOM 1's) */
    void SetFilename(const char *name);
    /* the in-memory data buffer handed to the caller (0x257F90): the CBufferIO under m_pFile
     * releases it and the archive forgets it (the word at 0x1C, the byte at 0x24) -- 0 when none */
    void *ReleaseDataBuffer();
    /* the file reopened unless it still is (0x258B40, SOCOM 1's ReOpen; both callers pass 1, 1) */
    bool ReOpen(int, unsigned int);
    bool IsOpen() { return m_pFile ? m_pFile->IsOpen() : false; }

    CKeyRing m_keys;   // 0x4: the open keys, innermost last
    CIO* m_pFileAlloc;   // 0x10
    CIO* m_pFile;   // 0x14
    unsigned char _18[0x30 - 0x18];   // 0x18: SOCOM 1's m_databuffer .. m_data_padded, and new fields
    int m_data_start;   // 0x30 (new in SOCOM II, the name ours): the file offset the keys' offsets count from
    unsigned char _34[0x9C - 0x34];   // 0x34: a new field, then SOCOM 1's m_tail (grown by 4)
    CKey* m_root;   // 0x9c (SOCOM 1: 0x88)
    CSTable* m_stable;   // 0xa0 (SOCOM 1: 0x8c)
    char* m_filename;   // 0xa4 (SOCOM 1: 0x90): CRdrArchive's walks compare their names with it
    unsigned char _a8[0xB8 - 0xA8];   // 0xa8: SOCOM 1's m_key_buffer .. m_bSecure
};   // 0xb8 bytes (OpenZAR's new; SOCOM 1's 0xA4)

}

#endif
