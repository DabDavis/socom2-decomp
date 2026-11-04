/* zrdr_file.cpp -- CRdrFile, an .rdr held whole: read packed from a file or fetched from a ZAR,
 * its offsets resolved into pointers and back, freed (OVL1 0x32C670-0x32DBFF). The unit name is
 * SOCOM 1's (the press beta's asserts name a zrdr_file.cpp; SOCOM 1's demo holds CRdrFile's
 * functions in this order, ReadPacked to zrdr_free). Defined in reverse of their image order
 * (-inline deferred). The packer keeps a std::map<_zrdr *, _zrdr *> of the arrays copied (a shared
 * array finds its owner's copy there): the map's template tail closes the unit, 0x32D780-0x32DBFF
 * (find_or_insert, destroy, the allocators' pieces, the constructor, insert_node_at). */

#include <string.h>

#include "zrdr.h"

void *_malloc(unsigned int size, const char *file, int line);
void *_calloc(unsigned int n, unsigned int size, const char *file, int line);
void _free(void *p, const char *file, int line);

/* the engine's file, CFileIO, is zutil.h's (through zar.h): ReadPacked calls its fread (slot 0x24) */

/* the packed format's version (1), what a buffer's first halfword must hold */
extern unsigned short data_003DFF58;

/* the resolvers: the tree's pointers made offsets (A) and back (B) */
void _resolveA(_zrdr *rdr, const _zrdr *root, const char *strings);
void _resolveB(_zrdr *rdr, const _zrdr *root, const char *strings);
/* zrdr_freearray (0x335AA0) */
void zrdr_freearray(_zrdr *rdr);

/* a packed buffer's header (ours): the format's version, whether its offsets are resolved, its
 * strings' size, the root's offset; the strings follow */
struct RdrHeader {
    unsigned short version;   // 0x0
    unsigned short flags;   // 0x2: 1, the offsets are resolved
    unsigned int strings;   // 0x4
    int root;   // 0x8
};

/* the arrays' sharing and the values' count (zrdr_pack.cpp) */
void zrdr_compress(_zrdr *rdr);
int zrdr_countarray(_zrdr *rdr);

/* SOCOM 1's; it returns 0 (other units declare it void, which their calls do not see) */
int zrdr_free(CRdrFile *file)
{
    delete file;
    return 0;
}

/* SOCOM 1's: the value cleared (_zrdr's initialiser list: six word read-mask-writes), the string
 * table built for 1024 strings */
CRdrFile::CRdrFile() : m_strings(0, 1024)
{
    m_buffer = 0;
    m_size = 0;
    m_flags = 0;
}

/* SOCOM 1's: the tree freed, or the buffer it lives in */
CRdrFile::~CRdrFile()
{
    if (!m_buffer)
        zrdr_freearray(this);
    else {
        if (!(m_flags & 4))
            _free(m_buffer, 0, 0);
        m_buffer = 0;
        m_size = 0;
    }
    m_flags = 0;
}

/* an array's values copied to dst and the array pointed at the copy, recorded in the map by its old
 * address; a shared array (nofree) pointed at its owner's copy instead. Each item that is an array
 * is copied after them in turn, a string's offset moved by delta; the next free value given back
 * (SOCOM II's grown SOCOM 1's Pack(_zrdr *, _zrdr *, int): the count left and the map new) */
_zrdr *CRdrFile::Pack(_zrdr *rdr, _zrdr *dst, int *remaining, int delta,
                      std::map<_zrdr *, _zrdr *> &copies)
{
    if (rdr->nofree)
        rdr->array = copies[rdr->array];
    else {
        int n = rdr->Count();

        memcpy(dst, rdr->array, n * sizeof(_zrdr));
        copies[rdr->array] = dst;
        rdr->array = dst;
        dst += n;
        *remaining -= n;
        for (int i = 0; i < n; i++) {
            _zrdr *item = rdr->Item(i);

            switch (item->type) {
            case ZRDR_ARRAY:
                dst = Pack(item, dst, remaining, delta, copies);
                break;
            case ZRDR_STRING:
                item->integer += delta;
                break;
            }
        }
    }
    return dst;
}

/* SOCOM 1's _fixup, CSTable::Pack's callback for a file's tree (Pack hands it over): a string's
 * text made the packed table's copy, an array's items in turn. SOCOM II's FindString is a virtual
 * (util_stable.h's slot 0x10); the walk inlines itself twice, the type read straight (the second
 * copy's test stays inline) */
void _fixup(CSTable *table, void *data)
{
    _zrdr *rdr = (_zrdr *)data;

    if (rdr->type == ZRDR_ARRAY) {
        int n = rdr->Count();

        for (int i = 0; i < n; i++)
            _fixup(table, rdr->Item(i));
    } else if (rdr->type == ZRDR_STRING)
        rdr->string = table->FindString(rdr->string);
}

/* SOCOM 1's: the tree packed into a buffer of its own -- its equal arrays shared, its strings packed
 * (each string value pointed at the packed copy), then the header, the strings and the values copied
 * after them, root first, its strings' offsets moved to the new buffer; the old tree freed through a
 * value pointed at it. The buffer's size (SOCOM II's; a packed file's own) */
unsigned int CRdrFile::Pack()
{
    if (m_buffer)
        return m_size;
    std::map<_zrdr *, _zrdr *> copies;

    zrdr_compress(this);
    m_strings.Pack(_fixup, this);
    int n = zrdr_countarray(this) + 1;
    unsigned int offset = m_strings.m_bytes + sizeof(RdrHeader);

    if (offset & 15)
        offset += 16 - (offset & 15);
    unsigned int size = offset + n * sizeof(_zrdr);
    char *buf = (char *)_calloc(size, 1, 0, 0);
    RdrHeader *h = (RdrHeader *)buf;

    h->version = data_003DFF58;
    h->flags = 0;
    h->strings = m_strings.m_bytes;
    h->root = offset;
    int delta = m_strings.Relocate(buf + sizeof(RdrHeader), 0, false);
    _zrdr *old = type == ZRDR_ARRAY ? array : 0;
    _zrdr *root = (_zrdr *)(buf + offset);

    *root = *this;
    int remaining = n - 1;
    Pack(root, root + 1, &remaining, delta, copies);
    integer = root->integer;
    if (m_buffer)
        _free(m_buffer, 0, 0);
    m_buffer = 0;
    m_buffer = (unsigned char *)buf;
    m_size = size;
    _zrdr tree;

    tree.array = old;
    tree.type = ZRDR_ARRAY;
    tree.length = length;
    zrdr_freearray(&tree);
    return m_size;
}

/* SOCOM 1's _resolveA, before _resolveB as there: a tree's pointers made offsets again for packing
 * -- an array's from the tree's root, its items first unless the array lives in the buffer, a
 * string's from the string table. It inlines itself twice, the accessors a level shorter in each
 * copy (the second calls Type(), 0x32CF30, out of line: its copy follows this function) */
void _resolveA(_zrdr *rdr, const _zrdr *root, const char *strings)
{
    switch (rdr->Type()) {
    case ZRDR_ARRAY:
        if (!rdr->nofree) {
            unsigned int n = rdr->Count();

            for (int i = 0; i < n; i++)
                _resolveA(rdr->Item(i), root, strings);
        }
        rdr->integer -= (int)root;
        break;
    case ZRDR_STRING:
        rdr->integer -= (int)strings;
        break;
    }
}

/* SOCOM 1's _resolveB, after Resolve as there: a packed tree's offsets made pointers -- a string's
 * into the string table (a negative offset makes the value the integer 0), an array's into the
 * tree, its items in turn unless the array lives in the buffer (SOCOM II's count in the value's
 * length, its items from 0; SOCOM 1 kept the count in the first item). The array case written
 * first: the compare chain tests the string's type first, as the target's; each offset made a pointer by
 * adding its base to the word (written as pointer sums, MW puts the base first in the addu) and
 * the item index an int (the address recomputed each pass, as the target's) */
void _resolveB(_zrdr *rdr, const _zrdr *root, const char *strings)
{
    switch (rdr->type) {
    case ZRDR_ARRAY:
        rdr->integer += (int)root;
        if (!rdr->nofree) {
            unsigned int n = rdr->Count();

            for (int i = 0; i < n; i++)
                _resolveB(rdr->Item(i), root, strings);
        }
        break;
    case ZRDR_STRING:
        if (rdr->integer >= 0)
            rdr->integer += (int)strings;
        else {
            rdr->integer = 0;
            rdr->type = ZRDR_INTEGER;
        }
        break;
    }
}

/* SOCOM 1's: a packed buffer's offsets made pointers (or, packing, made offsets again), the
 * root's value and count copied into the file's own. The header is read through the const
 * Buffer() (a second load, as the target's), the ternaries' tests reuse the first; root declared
 * ahead of h gives h s0. Before setting the resolved flag the target stores the flags back
 * unchanged (lhu/sh): MW drops every plain spelling of that no-op (`= h->flags`, `&= 0xffff`,
 * `&= ~0x10000`, a bitfield header), a volatile store keeps it -- the original's form is not
 * known */
bool CRdrFile::Resolve(bool pack)
{
    _zrdr *root;
    RdrHeader *h;

    if (!m_buffer)
        return false;
    h = (RdrHeader *)Buffer();
    root = 0;
    if (m_buffer)
        root = (_zrdr *)((char *)h + h->root);
    char *strings = 0;
    if (m_buffer)
        strings = (char *)(h + 1);
    if (pack) {
        _resolveA(root, root, strings);
        h->flags &= ~1;
    } else {
        if (!(h->flags & 1)) {
            _resolveB(root, root, strings);
            *(volatile unsigned short *)&h->flags = h->flags;
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
        unsigned char *table = 0;
        if (m_buffer)
            table = Buffer() + 12;
        m_strings.LoadTable(table, h->strings, false);
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
                ok = zar->FetchLIP(key, &buf);
            else {
                buf = _calloc(size, 1, 0, 0);
                ok = zar->Fetch(key, buf, size, 0);
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
    zar->Insert(key, m_buffer, m_size);
    Resolve(false);
    return key;
}

/* a packed buffer made a file, unless its version is not the reader's (SOCOM II's; the name
 * ours -- SOCOM 1's Load built the file in place) */
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
