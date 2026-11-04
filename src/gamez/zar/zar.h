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

/* the archive's directory tail, at the file's end (SOCOM 1's DWARF: 0x60 bytes; SOCOM II's 0x64 --
 * CZAR() versions it 0x20002 where SOCOM 1's set 0x20001). SOCOM II's new word is the padding at
 * 0x10, ahead of the array: ReadDirectory's copy of a tail moves five words, the array as a block,
 * then four (round 12; SOCOM 1's fields after it each a word later) */
struct TAIL {
    int flags;   // 0x0
    int key_count;   // 0x4
    int stable_size;   // 0x8
    int stable_ofs;   // 0xc
    int padded;   // 0x10: SOCOM II's (the name ours): the data's padding, m_data_padded when written
    int reserved[16];   // 0x14 (SOCOM 1: 0x10)
    int offset;   // 0x54 (SOCOM 1: 0x50): the data's size, the buffer's used bytes when written
    int crc;   // 0x58 (SOCOM 1: 0x54)
    int appversion;   // 0x5c (SOCOM 1: 0x58): the directory number Open is given
    int version;   // 0x60 (SOCOM 1: 0x5c)
};   // 0x64 bytes

class CKey : public CKeyRing {
public:
    /* an empty key, no name (SOCOM 1's; NewKey pushes one for a nameless key) */
    CKey();
    CKey(const char *name);
    /* out of line (0x25BB50): Insert(const char *, ...) deletes a key it could not write through it */
    ~CKey();
    CKey *FindKey(const char *name);
    /* the key and its children off the directory: a 16-byte record each, the name an offset
     * into the string table at base (SOCOM 1's) */
    bool Read(CZAR *zar, CIO *io, int base);
    CKey *InsertKey(CKey *key);
    /* the key's record and its children's written through the archive's file (SOCOM 1's;
     * 0x25B030, WriteDirectory calls it on the root as SOCOM 1's does) */
    void Write(CZAR *zar);
    /* the keys counted, the key itself included, the children's own when deep (SOCOM II's,
     * 0x25B300: WriteDirectory sizes the directory with it on the root, true; the name ours) */
    int CountKeys(bool deep);
    /* a key's directory record, 16 bytes (SOCOM II's, 0x25B2F0, called out of line; the name ours) */
    static int RecordSize();
    /* the names made the string table's packed copies (SOCOM 1's; CSTable::Pack's callback) */
    static void fixupKey(CSTable *table, void *key);

    /* the children forgotten, each deleted too when asked (SOCOM II's, 0x25B580: FreeDirectory
     * deletes them when the key buffer holds none of the keys -- the keys then their own
     * allocations; the name ours) */
    void DeleteChildren(bool del);

    char* m_name;   // 0xc
    int m_size;   // 0x10
    int m_offset;   // 0x14
};   // 0x18 bytes

/* the archive's key buffer: the directory's keys held by value, 24 bytes each (SOCOM 1's CKeyVec,
 * a vector<CKey>: ~CZAR destroys it three this-tests deep, its FreeVector's temporary two, as
 * SOCOM II's FreeVector over the base std::vector gives) */
class CKeyVec : public std::vector<CKey> {
};

/* SOCOM II's: a 16-byte growing byte buffer the archive owns at 0x34, made by its first write
 * (Close frees its block and zeroes its words before deleting it; the names ours) */
struct CZARBuffer {
    CZARBuffer() : m_data(0), m_size(0), m_used(0), m_grows(0) {}
    ~CZARBuffer()
    {
        if (m_data)
            _free(m_data, 0, 0);
        m_data = 0;
        m_size = 0;
        m_used = 0;
        m_grows = 0;
    }
    /* the bytes appended, the block grown first when they do not fit -- 4 KB past them the first
     * time, then by half a megabyte more than the block times its growths (0x257050) */
    unsigned int Write(const void *data, unsigned int size);

    char *m_data;   // 0x0
    unsigned int m_size;   // 0x4: the block's bytes
    unsigned int m_used;   // 0x8
    unsigned int m_grows;   // 0xc: how often the block grew
};   // 0x10 bytes

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
    /* every key of the open one fetched in place and handed to func -- the archive, the key's
     * name, its data and size, and data: the keys func took counted (SOCOM 1's FetchAll, 0x25A390;
     * SOCOM 1's said only whether there were any) */
    int FetchAll(int (*func)(CZAR &zar, const char *name, void *buf, int size, void *data), void *data);
    bool Fetch(CKey *key, void *buf, int size, unsigned int flags);
    bool Fetch(const char *name, void *buf, int size);
    bool Fetch(const char *name, char **str);
    bool Fetch(const char *name, float *value);
    bool Fetch(const char *name, unsigned int *value);
    bool Fetch(const char *name, int *value);
    bool Fetch(const char *name, bool *value);
    /* a fresh key, named so (SOCOM 1's) */
    CKey *NewKey(const char *name);
    /* a new key under the open one, and opened -- none unless the file is open (SOCOM 1's) */
    CKey *CreateKey(const char *name);
    /* a key's size by name in the open key, -1 when it has none */
    int GetSize(const char *name);
    CKey *GetOpenKey();
    CKey *OpenKey(CKey *key);
    CKey *OpenKey(const char *name);
    CKey *FindKey(const char *name);
    void CloseKey(CKey *key);
    /* the directory written if the file is open for writing, then closed (Close(bool)) and the
     * buffer at 0x34 deleted (SOCOM 1's, grown) */
    void Close();
    /* the files closed, the second deleted, the directory freed (SOCOM 1's Close(bool): SOCOM II's
     * callers pass true, its body never reads the flag -- as SOCOM 1's) */
    void Close(bool);
    /* bytes appended to the buffer at 0x34, made on the first write (SOCOM II's, 0x256FD0: Insert
     * writes a key's data through it; the name ours) */
    unsigned int WriteBuffer(const void *data, unsigned int size);
    /* the directory forgotten: the root's children (deleted when the key buffer holds none of
     * the keys), the key buffer, the open keys, the string table (SOCOM II's, 0x257320: the
     * closes' common tail; the name ours) */
    void FreeDirectory();
    /* the file closed, the directory kept (SOCOM 1's: the file's Close, a tail call) */
    void CloseKeepDir();
    /* the archive's file name: a copy of the name given, or DEFAULT_ZAR_NAME's own pointer for
     * none (SOCOM 1's) */
    void SetFilename(const char *name);
    /* the in-memory data buffer handed to the caller (0x257F90): the CBufferIO under m_pFile
     * releases it and the archive forgets it (the word at 0x1C, the byte at 0x24) -- 0 when none */
    void *ReleaseDataBuffer();
    /* a string off the file at its position: its length, then a fresh zeroed copy of its
     * characters (CIO::fwrite(const char *)'s record read back; the bytes read; SOCOM II's -- the
     * name ours: CZSIObject::Read reads the object's name with it) */
    int ReadString(char **str);
    /* the directory written at the file's end (SOCOM 1's; 0x257590) */
    bool WriteDirectory();
    /* the directory written if it changed, while the file is open: whether it is (SOCOM II's --
     * CSaveManager::Save calls it; the name ours) */
    bool Flush();
    /* the archive opened over the named file: the directory read (dir and mode handed on), the
     * data padded so; with data, a second file "<name>_tmp.<ext>" opened too (SOCOM 1's
     * Open(name, int, unsigned, unsigned) with a fifth argument -- every caller passes 0, its type
     * a guess) */
    bool Open(const char *name, int dir, unsigned int mode, unsigned int padded, bool data);
    /* the directory off the file (SOCOM 1's; 0x258000) */
    bool ReadDirectory(int dir, unsigned int mode);
    /* the file reopened unless it still is (0x258B40, SOCOM 1's ReOpen; both callers pass 1, 1) */
    bool ReOpen(int, unsigned int);
    bool IsOpen() { return m_pFile ? m_pFile->IsOpen() : false; }

    CKeyRing m_keys;   // 0x4: the open keys, innermost last
    CIO* m_pFileAlloc;   // 0x10
    CIO* m_pFile;   // 0x14
    CIO* m_pDataFile;   // 0x18 (SOCOM 1: m_databuffer): SOCOM II's second file, Open's name with
                        // another extension, a CFileIO the archive owns (the name ours)
    void* m_databuffer;   // 0x1c (SOCOM 1: m_databuffer_size): what ReleaseDataBuffer hands over
    int m_data_size;   // 0x20: SOCOM II's (the name ours): the buffer's size the bone loaders take with it
    unsigned char m_24;   // 0x24: SOCOM II's (ReleaseDataBuffer clears it)
    unsigned char m_modified;   // 0x25 (SOCOM 1: 0x20): the directory to be written (0x258BD0)
    unsigned int m_data_padded;   // 0x28 (SOCOM 1: 0x24)
    int m_2c;   // 0x2c: SOCOM II's
    int m_data_start;   // 0x30 (new in SOCOM II, the name ours): the file offset the keys' offsets count from
    CZARBuffer* m_34;   // 0x34: SOCOM II's (Close deletes it)
    TAIL m_tail;   // 0x38 (SOCOM 1: 0x28)
    CKey* m_root;   // 0x9c (SOCOM 1: 0x88)
    CSTable* m_stable;   // 0xa0 (SOCOM 1: 0x8c)
    char* m_filename;   // 0xa4 (SOCOM 1: 0x90): CRdrArchive's walks compare their names with it
    CKeyVec m_key_buffer;   // 0xa8 (SOCOM 1: 0x94)
    unsigned char m_bCompress;   // 0xb4
    unsigned char m_bSecure;   // 0xb5
};   // 0xb8 bytes (OpenZAR's new; SOCOM 1's 0xA4)

}

#endif
