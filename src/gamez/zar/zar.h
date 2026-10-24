/* zar::CZAR -- the ZAR archive: a tree of named keys over a file, the zar namespace the mangled
 * names'. Shapes SOCOM 1's DWARF where SOCOM II kept them; SOCOM II made the key ring (a key's
 * children, and the archive's stack of open keys) a vector where SOCOM 1's was a std::list,
 * and grew the archive to 0xB8 bytes. The header's name is ours. */
#ifndef ZAR_H
#define ZAR_H

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

/* a vector of keys: a count at 4, the array at 8. Its iterators are classes, != being !(==)
 * (the walks materialise the compare). The walks hold a const_iterator made from begin(): the
 * conversion keeps MW from folding a walk's first test (begin against begin + count) into a
 * test of the count, which an iterator's copy of begin() does not. */
class CKeyRing {
public:
    class iterator {
    public:
        iterator(CKey **p) : m_p(p) {}
        bool operator==(const iterator &o) const { return m_p == o.m_p; }
        bool operator!=(const iterator &o) const { return !(*this == o); }
        iterator &operator++() { ++m_p; return *this; }
        iterator operator-(int n) const { return iterator(m_p - n); }
        CKey *operator*() const { return *m_p; }

        CKey **m_p;
    };

    class const_iterator {
    public:
        const_iterator(CKey *const *p) : m_p(p) {}
        const_iterator(const iterator &i) : m_p(i.m_p) {}
        bool operator==(const const_iterator &o) const { return m_p == o.m_p; }
        bool operator!=(const const_iterator &o) const { return !(*this == o); }
        const_iterator &operator++() { ++m_p; return *this; }
        CKey *operator*() const { return *m_p; }

        CKey *const *m_p;
    };

    iterator begin() { return iterator(m_data); }
    iterator end() { return iterator(m_data + m_size); }
    unsigned int size() const { return m_size; }
    bool empty() const { return m_size == 0; }
    CKey *back() { return *(end() - 1); }
    void pop_back() { m_size--; }
    void push(CKey *key);

    unsigned int m_capacity;   // 0x0
    unsigned int m_size;   // 0x4
    CKey **m_data;   // 0x8
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
