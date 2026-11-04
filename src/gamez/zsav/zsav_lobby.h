/* zsav_lobby.h -- the lobby's save (SOCOM II's; every name here ours): a blob of named entries
 * the lobby's modules write and read back, kept in one buffer and transferred whole as a Medius
 * file. Not SOCOM 1's CSaveManager (zsave.h: a ZAR of .rdr keys) -- its manager is a pointer
 * list of modules with a file, a buffer, the buffer's header and entries, and the bytes used:
 *
 *   buffer: Header { version, count } then `count` 40-byte Entry { name[32], offset, size },
 *           then the entries' data, each at its offset from the buffer's start.
 *
 * The manager is the global at 0x4364A0 (built by the memory card unit's initialiser 0x400160,
 * its destructor 0x33ED70 registered there; GetLobbySaveManager() returns it); theAcctDB is its one
 * module. zNetMediusUniverse.cpp (the online lane's round 12) includes this header: CAcctDB derives
 * from CLobbySaveModule and its overrides take what the manager passes. */
#ifndef ZSAV_LOBBY_H
#define ZSAV_LOBBY_H

#include <list>
#include <string>
#include <string.h>
#include "../zutil/zutil.h"

class CLobbySaveModule;

/* a string that is never null (SOCOM 1's zutil.h inline, as fts_state.h carries it): the module's
 * constructor assigns its name through it -- written out there, erase()'s category byte takes the
 * frame slot above the registered pointer's copy, the target's other way round */
class CSaferStr : public std::string {
public:
    CSaferStr() {}
    CSaferStr &operator=(const char *sz)
    {
        if (sz)
            assign(sz);
        else
            erase();
        return *this;
    }
};

/* a file transferred through Medius: zNetMediusFile.cpp's class (0x9C bytes), its constructor
 * inline as the manager's constructor inlines it (the CIO base's vtable stored first) */
struct MediusFile;
class CLobbyFileIO : public CIO {
public:
    CLobbyFileIO() : m_handle(0), m_open(0), m_result(0), m_buf(0), m_size(0) { m_path[0] = 0; }
    bool Open(const char *name, unsigned int mode);
    void Close();
    bool IsOpen() const;
    int GetSize() const;
    unsigned int GetMode() const;
    int ftell() const;
    int fseek(int offset, int whence);
    int fread(void *buf, int size);
    int fread(int size, void **buf);
    int fwrite(const void *buf, int size);
    int freadAsync(void *buf, int size);
    int fwriteAsync(const void *buf, int size);
    bool fsyncAsync(int mode, int *result);
    void fflush();
    const unsigned char *GetChecksum() const;

    MediusFile *m_handle;   // 0x4
    unsigned int m_mode;   // 0x8
    char m_path[0x80];   // 0xc
    unsigned char m_open;   // 0x8c
    int m_result;   // 0x90
    const void *m_buf;   // 0x94
    int m_size;   // 0x98
};   // 0x9c

/* the lobby's save: the modules, the file, the blob (0x2C bytes) */
class CLobbySaveManager : public std::list<CLobbySaveModule *> {
public:
    /* the blob's header and its entries */
    struct Header {
        int m_version;   // 0x0
        int m_count;   // 0x4
    };   // 0x8
    struct Entry {
        char m_name[32];   // 0x0
        int m_offset;   // 0x20
        int m_size;   // 0x24
    };   // 0x28

    CLobbySaveManager();
    ~CLobbySaveManager();

    bool Pending();
    bool Load(const char *name, int version);
    bool Fetch(const char *name, int a, int b);
    bool Save(const char *name, int version, int flags);
    void Close();

    void RegisterModule(CLobbySaveModule *module)
    {
        if (module)
            push_back(module);
    }
    void FreeBuffer()
    {
        if (m_buffer)
            _free(m_buffer, 0, 0);
        m_buffer = 0;
    }
    /* an entry named and its data appended to the blob */
    void Add(const char *name, const char *data, int size)
    {
        Entry *e = &m_entries[m_header->m_count];

        strncpy(e->m_name, name, 32);
        e->m_size = size;
        e->m_offset = m_used;
        m_header->m_count++;
        if (m_buffer)
            m_buffer = (char *)_realloc(m_buffer, m_used + size, 0, 0);
        else
            m_buffer = (char *)_malloc(size, 0, 0);
        memcpy(m_buffer + m_used, data, size);
        m_used += size;
    }

    CSaferStr m_name;   // 0xc
    CLobbyFileIO *m_file;   // 0x18
    char *m_buffer;   // 0x1c
    Header *m_header;   // 0x20
    Entry *m_entries;   // 0x24
    int m_used;   // 0x28
};   // 0x2c


/* a part of the lobby's save: its vtable at 0, its manager, its name; three pure virtuals */
class CLobbySaveModule {
public:
    CLobbySaveModule(const char *name, CLobbySaveManager *manager);
    ~CLobbySaveModule();
    virtual void Save(int flags) = 0;   // 0x08
    virtual void Load(const char *data, int size) = 0;   // 0x0C
    virtual void SetChecksum(const unsigned char *sum) = 0;   // 0x10

    void Write(const char *data, int size);
    /* the name as a C string (ours, inline: through it the inlined c_str() folds the name's
     * offset into the module's, as the manager's Load reads it) */
    const char *GetName() const { return m_name.c_str(); }

    CLobbySaveManager *m_manager;   // 0x4
    CSaferStr m_name;   // 0x8
};   // 0x14

/* the lobby's save manager, the global above (0x2AFBA0, fts_mission.cpp's; the name ours) */
CLobbySaveManager *GetLobbySaveManager();

#endif
