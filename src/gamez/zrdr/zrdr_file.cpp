/* zrdr_file.cpp -- CRdrFile, an .rdr held whole: read packed from a file or fetched from a ZAR,
 * its offsets resolved into pointers and back, freed (OVL1 0x32C670-0x32DBFF). The unit name is
 * SOCOM 1's (the press beta's asserts name a zrdr_file.cpp; SOCOM 1's demo holds CRdrFile's
 * functions in this order, ReadPacked to zrdr_free). Defined in reverse of their image order
 * (-inline deferred). Not written yet: the resolvers and packers, which recurse and inline
 * themselves (0x32CB90, 0x32CCB0, Pack at 0x32CF40, 0x32D1B0, 0x32D3C0, 0x32D840), and the
 * template pieces after zrdr_free. */

#include "zrdr.h"

void *_malloc(unsigned int size, const char *file, int line);
void *_calloc(unsigned int n, unsigned int size, const char *file, int line);
void _free(void *p, const char *file, int line);

/* the engine's file (SOCOM 1's CFileIO): the slots up to fread, the one ReadPacked calls */
class CFileIO : public CIO {
public:
    virtual int GetSize() const;
    virtual unsigned int GetMode() const;
    virtual int ftell() const;
    virtual int fseek(int offset, int whence);
    virtual int fread(void *buf, int size);
};

/* the packed format's version (1), what a buffer's first halfword must hold */
extern unsigned short data_003DFF58;

/* zar::CZAR's fetches and insert, not named yet: a key's data into a buffer it allocates, into a
 * buffer given, and a buffer written as a key's data */
extern "C" bool func_00259EE0(zar::CZAR *zar, zar::CKey *key, void **buf);
extern "C" bool func_0025A000(zar::CZAR *zar, zar::CKey *key, void *buf, int size, unsigned int a);
extern "C" void func_0025A120(zar::CZAR *zar, zar::CKey *key, const void *buf, int size);
/* the resolvers (not written yet): the tree's offsets into pointers (A) and back (B) */
extern "C" void func_0032CCB0(_zrdr *rdr, _zrdr *root, char *strings);
extern "C" void func_0032CB90(_zrdr *rdr, _zrdr *root, char *strings);
/* zrdr_freearray (0x335AA0) */
extern "C" void func_00335AA0(_zrdr *rdr);

/* SOCOM 1's; it returns 0 (other units declare it void, which their calls do not see) */
int zrdr_free(CRdrFile *file)
{
    delete file;
    return 0;
}

/* SOCOM 1's: the value cleared, the string table built for 1024 strings.
 * OPEN, 16 instructions: the target clears the value's six bitfields with word read-modify-
 * writes (lw, and, or with `andi zero,mask`, sw) where _zrdr's constructor here compiles to
 * byte stores; SOCOM 1's constructor did the same word writes for its two flags while its Clone
 * wrote bytes, so it is the constructor's form, not the fields' declaration -- not found yet
 * (default arguments and an inline setter with parameters compile to the same bytes) */
CRdrFile::CRdrFile()
{
    func_0039FBD0(&m_strings, 0, 1024);
    m_buffer = 0;
    m_size = 0;
    m_flags = 0;
}

/* SOCOM 1's: the tree freed, or the buffer it lives in */
CRdrFile::~CRdrFile()
{
    if (!m_buffer)
        func_00335AA0(this);
    else {
        if (!(m_flags & 4))
            _free(m_buffer, 0, 0);
        m_buffer = 0;
        m_size = 0;
    }
    m_flags = 0;
    func_0039FB30(&m_strings, -1);
}

/* a packed buffer's header (ours): the format's version, whether its offsets are resolved, its
 * strings' size, the root's offset; the strings follow */
struct RdrHeader {
    unsigned short version;   // 0x0
    unsigned short flags;   // 0x2: 1, the offsets are resolved
    unsigned int strings;   // 0x4
    int root;   // 0x8
};

/* SOCOM 1's: a packed buffer's offsets made pointers (or, packing, made offsets again), the
 * root's value and count copied into the file's own.
 * OPEN, 6 instructions: the target tests m_buffer as first loaded (v1) at each ternary but takes
 * the header from a second load (s0) -- here one load serves both (a local copy, the Root()
 * accessor: the same); and before setting the resolved flag it stores the flags word back
 * unchanged (lhu/sh), a no-op MW drops however it is spelled here (`&= ~0x10000`, a bitfield
 * header -- byte stores) */
bool CRdrFile::Resolve(bool pack)
{
    if (!m_buffer)
        return false;
    RdrHeader *h = (RdrHeader *)m_buffer;
    _zrdr *root = m_buffer ? (_zrdr *)((char *)h + h->root) : 0;
    char *strings = m_buffer ? (char *)(h + 1) : 0;
    if (pack) {
        func_0032CCB0(root, root, strings);
        h->flags &= ~1;
    } else {
        if (!(h->flags & 1)) {
            func_0032CB90(root, root, strings);
            h->flags |= 1;
        } else {
            switch (root->type) {
            case ZRDR_ARRAY:
                root->array = root;
                break;
            case ZRDR_STRING:
                root->string = strings;
                break;
            }
        }
        integer = root->integer;
        length = root->length;
        m_strings.LoadTable(m_buffer ? m_buffer + 12 : 0, h->strings, false);
    }
    return true;
}

/* SOCOM 1's (with SOCOM II's flags): a key's data fetched -- into a buffer of the archive's with
 * flag 4 -- and made a file */
CRdrFile *CRdrFile::Load(zar::CZAR *zar, zar::CKey *key, int flags)
{
    CRdrFile *file = 0;

    if (key) {
        int size = key->m_size;
        if (size != -1) {
            void *buf = 0;
            bool ok;
            if (flags & 4)
                ok = func_00259EE0(zar, key, &buf);
            else {
                buf = _calloc(size, 1, 0, 0);
                ok = func_0025A000(zar, key, buf, size, 0);
            }
            if (ok)
                file = Create(buf, size, flags);
        }
    }
    return file;
}

/* SOCOM 1's: the file packed and written as a key's data, then resolved again */
zar::CKey *CRdrFile::Insert(zar::CZAR *zar, zar::CKey *key)
{
    Pack();
    Resolve(true);
    func_0025A120(zar, key, m_buffer, m_size);
    Resolve(false);
    return key;
}

/* a packed buffer made a file, unless its version is not the reader's (SOCOM II's; the name
 * ours -- SOCOM 1's Load built the file in place).
 * OPEN, 16 instructions: the inlined constructor's bitfield writes (CRdrFile's above) */
CRdrFile *CRdrFile::Create(void *buf, unsigned int size, int flags)
{
    CRdrFile *file = 0;

    if (*(unsigned short *)buf == data_003DFF58) {
        file = new CRdrFile;
        file->m_flags = flags;
        file->m_buffer = (unsigned char *)buf;
        file->m_size = size;
        _zrdr *root = file->Root();
        file->type = ZRDR_ARRAY;
        file->integer = 0;
        file->length = root->length;
        if (!file->Resolve(false)) {
            delete file;
            file = 0;
        }
    }
    return file;
}

/* SOCOM 1's: `size` bytes of a packed file read and resolved */
bool CRdrFile::ReadPacked(CFileIO &io, unsigned int size)
{
    bool ok = false;

    if (size) {
        m_buffer = (unsigned char *)_malloc(size, 0, 0);
        io.fread(m_buffer, size);
        m_size = size;
        type = ZRDR_ARRAY;
        integer = 0;
        ok = Resolve(false);
        if (!ok) {
            type = 0;
            integer = 0;
        }
    }
    return ok;
}
