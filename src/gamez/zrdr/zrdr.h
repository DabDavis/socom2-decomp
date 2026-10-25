/* zrdr.h -- the engine's text-data reader: the .rdr files every subsystem reads its settings from,
 * parsed into a tree of typed values. SOCOM 1's zrdr (the press beta's asserts name zrdr_main.cpp,
 * zrdr_file.cpp, zrdr_parse.cpp, zrdr_edit.cpp and a zrdr_local.h; its initialisers
 * zrdr_archive.cpp and zrdr_parse.cpp); the header's own name is ours.
 *
 * A value is SOCOM 1's _zrdr (its DWARF: a type byte, two flags, a 16-bit length and a word).
 * SOCOM II changed the array: SOCOM 1 kept the count in an array's first element and its items
 * from index 1; SOCOM II keeps the count in `length` and the items from index 0 -- every finder
 * walks `Count()`/`Item(i)`, the two inline accessors the code expands at every use (an array's
 * count re-tested against its type each time). */
#ifndef ZRDR_H
#define ZRDR_H

#include "../zar/zar.h"

class CSTable;
class CRdrFile;
class CRdrArchiveList;
class CFileIO;
struct PNT2D;
struct PNT3D;
struct IPNT2D;
class Rfloat;
class tag_RECT;

/* a value's type (SOCOM 1's values; the names ours) */
enum {
    ZRDR_INTEGER = 1,
    ZRDR_REAL = 2,
    ZRDR_STRING = 3,
    ZRDR_ARRAY = 4
};

/* a ZRDR value (SOCOM 1's DWARF) */
struct _zrdr {
    unsigned int type : 8;   // 0x0
    unsigned int isclone : 1;   // 0x0: the array is the value's own (Clone's), DeleteChildren frees it
    unsigned int packed : 1;   // 0x0
    unsigned int nofree : 1;   // 0x0: SOCOM II's: the array lives in a packed buffer, zrdr_freearray leaves it (the name ours)
    unsigned int unused : 5;   // 0x0
    unsigned int length : 16;   // 0x0: an array's count
    union {
        float real;
        int integer;
        char *string;
        _zrdr *array;
    };   // 0x4

    /* SOCOM 1's (0x3C there); SOCOM II's clears its new flag too, each field read, masked and
     * written back as MW stores a constant into a bitfield */
    _zrdr()
    {
        type = 0;
        isclone = false;
        packed = false;
        nofree = false;
        unused = 0;
        length = 0;
    }
    ~_zrdr();
    void DeleteChildren();
    void Clone(_zrdr *src, CSTable *strings);
    /* out of line in every unit (0x32C650/0x32C660): the file and pack units call them */
    bool IsString() const;
    bool IsArray() const;

    /* an array's count, its items, a value read as a string or a number (ours, inline) */
    unsigned int Count() const { return type == ZRDR_ARRAY ? length : 0; }
    _zrdr *Item(unsigned int i) const { return i < Count() ? &array[i] : 0; }
    char *String() const { return type == ZRDR_STRING ? string : 0; }
    float Real() const
    {
        if (type == ZRDR_REAL)
            return real;
        else if (type == ZRDR_INTEGER)
            return (float)integer;
        else
            return 0.0f;
    }
    int Int() const
    {
        if (type == ZRDR_INTEGER)
            return integer;
        else if (type == ZRDR_REAL)
            return (int)real;
        else
            return 0;
    }
    bool IsNumber() const { return type == ZRDR_INTEGER || type == ZRDR_REAL; }
};   // 0x8 bytes

/* the finders (zrdr_main.cpp): a tag is a string item, its value the item after it */
_zrdr *zrdr_findtag(const _zrdr *rdr, const char *tag);
_zrdr *zrdr_findtag_startidx(const _zrdr *rdr, const char *tag, int start);
_zrdr *zrdr_flat_findtag_startidx(const _zrdr *rdr, const char *tag, int start);
_zrdr *zrdr_nexttag(const _zrdr *rdr, const char *tag, _zrdr *after);
_zrdr *_zrdr_nexttag(const _zrdr *rdr, const char *tag, int len, _zrdr *after);
char *zrdr_findstring(const _zrdr *rdr, const char *tag);
char *zrdr_flat_findstring(const _zrdr *rdr, const char *tag, int start);
bool zrdr_isitemtype(const _zrdr *rdr, int index, int type);
bool zrdr_findreal(const _zrdr *rdr, const char *tag, float *out, int n);
bool zrdr_findint(const _zrdr *rdr, const char *tag, int *out, int n);
bool zrdr_finduint(const _zrdr *rdr, const char *tag, unsigned int *out, int n);
bool zrdr_findRfloat(const _zrdr *rdr, const char *tag, Rfloat *out);
bool zrdr_findbool(const _zrdr *rdr, const char *tag, bool *out);
bool zrdr_findRECT(const _zrdr *rdr, const char *tag, tag_RECT *out);
bool zrdr_findIPNT2D(const _zrdr *rdr, const char *tag, IPNT2D *out);
bool zrdr_findPNT2D(const _zrdr *rdr, const char *tag, PNT2D *out);
bool zrdr_findPNT3D(const _zrdr *rdr, const char *tag, PNT3D *out);
bool zrdr_findSTRING(const _zrdr *rdr, const char *tag, const char **out);
bool zrdr_toREAL(const _zrdr *rdr, float *out, int n);
bool zrdr_toINT(const _zrdr *rdr, int *out, int n);
bool zrdr_tobool(const _zrdr *rdr, bool *out);
bool zrdr_toRfloat(const _zrdr *rdr, Rfloat *out);
bool zrdr_toRECT(const _zrdr *rdr, tag_RECT *out);
const char *zrdr_findfile(const char *name, const char *path);

/* the archives .rdr files are fetched from before the disc (zrdr_archive.cpp): SOCOM 1's class,
 * a ZAR and its file name; the list of them is the class's (SOCOM 1's m_list) */
class CRdrArchive : public zar::CZAR {
public:
    CRdrArchive(const char *name) : zar::CZAR(name, 0) {}
    virtual ~CRdrArchive();

    static CRdrFile *FindRdr(const char *path, int flags);
    static CRdrArchive *AddArchive(const char *name, const char *path);
    static bool RemoveArchive(const char *name, const char *path);
    static CRdrArchive *Find(const char *name);
    static void CloseAll();
    static void OpenAll();

    static CRdrArchiveList m_list;   // 0x488510
};   // 0xb8 bytes

/* the engine's string table (util_stable.cpp, not written yet): SOCOM 1's CSTable grown by a
 * vtable pointer at 0xC, 0x20 bytes. Its constructor and destructor are still placeholders, so
 * CRdrFile calls them by hand where its member's would run: an inline wrapper adds the null test
 * MW puts around an inlined member destructor */
extern "C" void func_0039FBD0(CSTable *table, unsigned int bytes, unsigned int strings);
extern "C" void func_0039FB30(CSTable *table, int flags);
class CSTable {
public:
    void LoadTable(void *table, unsigned int bytes, bool copy);

    unsigned char _0[0x20];
};   // 0x20 bytes

/* a file read or fetched whole: SOCOM 1's CRdrFile, the tree's root, its strings and the buffer
 * it was read into; SOCOM II added the flags it was read with (0x34 bytes, new'd so) */
class CRdrFile : public _zrdr {
public:
    CRdrFile();
    ~CRdrFile();

    static CRdrFile *Load(zar::CZAR *zar, zar::CKey *key, int flags);
    static CRdrFile *Create(void *buf, unsigned int size, int flags);
    bool ReadPacked(CFileIO &io, unsigned int size);
    zar::CKey *Insert(zar::CZAR *zar, zar::CKey *key);
    bool Resolve(bool pack);
    void Pack();

    /* a packed buffer's tree: its root's offset at 8, its strings from 12 (ours) */
    _zrdr *Root() { return m_buffer ? (_zrdr *)(m_buffer + *(int *)(m_buffer + 8)) : 0; }

    CSTable m_strings;   // 0x08
    unsigned char *m_buffer;   // 0x28
    unsigned int m_size;   // 0x2c
    int m_flags;   // 0x30: 4, the buffer is not the file's to free
};   // 0x34 bytes

/* the files (zrdr_file.cpp, zrdr_parse.cpp) */
_zrdr *zrdr_read(const char *name, const char *path, int flags);
int zrdr_free(CRdrFile *file);

#endif
