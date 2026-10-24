/* zar::CZAR -- the ZAR archive: a tree of named keys over a file, the zar namespace the mangled
 * names'. Shapes SOCOM 1's DWARF where SOCOM II kept them; SOCOM II made the key ring (a key's
 * children, and the archive's stack of open keys) a vector where SOCOM 1's was a std::list,
 * and grew the archive to 0xB8 bytes. The header's name is ours. */
#ifndef ZAR_H
#define ZAR_H

#include <vector>

/* the engine's file interface, abstract: its slots SOCOM 1's (CFileIO's and CMemCardIO's
 * vtables name them), those read so far */
class CIO {
public:
    virtual bool Open(const char *name, unsigned int mode);
    virtual void Close();
    virtual bool IsOpen() const;
};

namespace zar {

class CKey;

/* a key's children and the archive's open keys: SOCOM 1's std::list<CKey *>, a vector here (a
 * count at 4, the array at 8) */
class CKeyRing : public std::vector<CKey *> {
public:
    void push(CKey *key);
};   // 0xc bytes

class CKey : public CKeyRing {
public:
    CKey *FindKey(const char *name);

    char* m_name;   // 0xc
    int m_size;   // 0x10
    int m_offset;   // 0x14
};   // 0x18 bytes

class CZAR {
public:
    CZAR(const char *name, CIO *io);
    virtual ~CZAR();
    bool Fetch(const char *name, void *buf, int size);
    CKey *GetOpenKey();
    CKey *OpenKey(CKey *key);
    CKey *OpenKey(const char *name);
    CKey *FindKey(const char *name);
    void CloseKey(CKey *key);
    bool IsOpen() { return m_pFile ? m_pFile->IsOpen() : false; }

    CKeyRing m_keys;   // 0x4: the open keys, innermost last
    CIO* m_pFileAlloc;   // 0x10
    CIO* m_pFile;   // 0x14
    unsigned char _18[0x9C - 0x18];   // 0x18: SOCOM 1's m_databuffer .. m_tail, grown by 0x14
    CKey* m_root;   // 0x9c (SOCOM 1: 0x88)
    unsigned char _a0[0xB8 - 0xA0];   // 0xa0: SOCOM 1's m_stable .. m_bSecure
};   // 0xb8 bytes (OpenZAR's new; SOCOM 1's 0xA4)

}

#endif
