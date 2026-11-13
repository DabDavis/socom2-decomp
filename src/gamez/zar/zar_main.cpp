/* zar_main.cpp -- zar::CZAR, the archive (OVL1 0x256FD0-0x259EDF, the README's zar units): its
 * file, its stack of open keys and what it fetches. The unit name is SOCOM 1's (the press beta's
 * asserts name it). Defined in reverse of their image order (-inline deferred). Written: the
 * constructor, the destructor, SetFilename, both Closes, CloseKeepDir, Flush, the key stack's six
 * (CloseKey .. GetSize), ReadString, _countKeys and CKey::fixupKey (with their vector<CKey *>
 * pieces 0x257B90-0x257CB0 and 0x257EC0/0x257ED0: the MSL headers round 24), NewKey and
 * FreeDirectory -- whose FreeVectors give the vector<CKey>
 * and vector<CKey *> pieces 0x2573E0-0x25757F right after it, and the key buffer's clear and
 * deleter 0x2599B0/0x259A30 in the tail. Not yet: Open 0x258920, WriteDirectory, the
 * buffer writers below 0x257320, the directory reader 0x258000 and the CKey vector's class insert
 * 0x259250. */

#include "zar.h"
#include "../zutil/util_stable.h"
#include "../zsys/zsys.h"

/* the root key's name and a deleted key's (SOCOM 1's file statics, "ROOT" and "DELETED"; OVL1
 * 0x3DD000 and 0x3DD008, 8 bytes apart as MW 3.0 lays such statics out) */
/* zsys_memory.cpp's calloc (not in zutil.h) */
void *_calloc(unsigned int n, unsigned int size, const char *file, int line);
/* MSL's strtok (MAIN; not in include/string.h) */
extern "C" char *strtok(char *s, const char *delim);

static const char *DEFAULT_ZAR_NAME = "ROOT";
static const char *DELETED_ZAR_NAME = "DELETED";
/* whether the loads are 64-byte aligned (the next file static, 0x3DD010: SOCOM 1's at gp-32348,
 * after DELETED_ZAR_NAME's gp-32352; true; the name ours) */
static bool s_alignLoads = true;

namespace zar {

/* the archive over a file, an own CFileIO when none is given (SOCOM 1's: the members zeroed, the
 * tail cleared and versioned, a string table of 1024, the name set -- SetFilename inlined -- the
 * data padded to 16, a root key named DEFAULT_ZAR_NAME). Byte-exact: the file is a ternary (both
 * builds branch to the allocation and jump over an empty arm, which an `if (!io)` turns round) */
CZAR::CZAR(const char *name, CIO *io)
    : m_pFileAlloc(0), m_pDataFile(0), m_databuffer(0), m_24(0), m_2c(0), m_root(0), m_stable(0),
      m_filename(0), m_bCompress(0)
{
    memset(&m_tail, 0, sizeof(m_tail));
    m_tail.version = 0x20002;
    m_stable = new CSTable(0, 1024);
    m_34 = 0;
    m_data_start = 0;
    SetFilename(name);
    m_pFile = io ? io : (m_pFileAlloc = new CFileIO);
    m_data_padded = 16;
    m_root = new CKey(DEFAULT_ZAR_NAME);
}

/* (SOCOM 1's: closed, the string table deleted, the key buffer freed, the name set back to the
 * default's and then to DELETED_ZAR_NAME, the root deleted -- SOCOM II calls Close out of line) */
CZAR::~CZAR()
{
    Close();
    delete m_stable;
    FreeVector<std::vector<CKey> >(m_key_buffer);
    SetFilename(DEFAULT_ZAR_NAME);
    m_filename = (char *)DELETED_ZAR_NAME;
    delete m_root;
    m_root = 0;
}

/* called out of line by ~CZAR and Open (pinned: MW would inline it) */
#pragma auto_inline off
void CZAR::Close()
{
    if (IsOpen() && (m_pFile->GetMode() & 6))
        Flush();
    Close(true);
    if (m_34) {
        delete m_34;
        m_34 = 0;
    }
}
#pragma auto_inline reset

void CZAR::Close(bool)
{
    if (m_pFileAlloc) {
        m_pFileAlloc->Close();
        delete m_pFileAlloc;
        m_pFileAlloc = 0;
        m_pFile = 0;
    } else if (m_pFile)
        m_pFile->Close();
    if (m_pDataFile) {
        m_pDataFile->Close();
        delete m_pDataFile;
        m_pDataFile = 0;
    }
    FreeDirectory();
}

bool CZAR::Open(const char *name, int dir, unsigned int mode, unsigned int padded, bool data)
{
    bool ok = false;

    if (name) {
        if (!m_pFile)
            m_pFile = m_pFileAlloc = new CFileIO;
        ok = m_pFile->Open(name, mode);
        if (ok) {
            SetFilename(name);
            m_data_padded = padded;
            ok = ReadDirectory(dir, mode);
            if (!ok)
                Close();
            else
                m_tail.appversion = dir;
        }
    }
    if (ok && data && name) {
        char tmp[256];
        char path[256];

        if (!m_pDataFile)
            m_pDataFile = new CFileIO;
        strcpy(path, name);
        strcpy(tmp, strtok(path, "."));
        strcat(tmp, "_tmp.");
        strcat(tmp, strtok(0, "."));
        ok = m_pDataFile->Open(tmp, 60);
    }
    return ok;
}

/* (SOCOM 1's, the same code: NewKey, GetOpenKey and OpenKey inlined) */
CKey *CZAR::CreateKey(const char *name)
{
    CKey *key = 0;

    if (IsOpen()) {
        key = NewKey(name);
        GetOpenKey()->InsertKey(key);
        OpenKey(key);
    }
    return key;
}

bool CZAR::Flush()
{
    if (!IsOpen())
        return false;
    if (m_modified) {
        WriteDirectory();
        m_modified = 0;
    }
    return true;
}

/* the name copied in unless it is the one held: the old copy freed (never the default's own
 * pointer), no name or the default's kept as the default's pointer (SOCOM 1's) */
void CZAR::SetFilename(const char *name)
{
    if (m_filename && m_filename != DEFAULT_ZAR_NAME) {
        if (m_filename == name)
            return;
        _free(m_filename, 0, 0);
        m_filename = 0;
    }
    if (!name || name == DEFAULT_ZAR_NAME)
        m_filename = (char *)DEFAULT_ZAR_NAME;
    else
        m_filename = _strdup(name, 0, 0);
}

/* (SOCOM 1's, the same code) */
void CZAR::CloseKeepDir()
{
    m_pFile->Close();
}

/* a key pushed when there is one: SOCOM 1's (a list's push_front) tests twice too */
inline void CKeyRing::push(CKey *key)
{
    if (key)
        push_back(key);
}

/* SOCOM 1's took the key to close; SOCOM II's pops the innermost, whichever it is */
void CZAR::CloseKey(CKey *key)
{
    if (m_keys.size())
        m_keys.pop_back();
}

CKey *CZAR::OpenKey(const char *name)
{
    CKey *key = IsOpen() ? GetOpenKey()->FindKey(name) : 0;

    return OpenKey(key);
}

CKey *CZAR::FindKey(const char *name)
{
    CKey *key = GetOpenKey();

    return key ? key->FindKey(name) : 0;
}

CKey *CZAR::OpenKey(CKey *key)
{
    if (key)
        m_keys.push(key);
    return key;
}

/* the innermost open key, or the root */
CKey *CZAR::GetOpenKey()
{
    if (m_keys.empty())
        return m_root;
    else
        return m_keys.back();
}

/* SOCOM 1's (0x1D8CD0) */
int CZAR::GetSize(const char *name)
{
    CKey *key = GetOpenKey()->FindKey(name);

    return key ? key->m_size : -1;
}

/* a block allocated 64-aligned when the archive aligns its loads, else plainly (ours) */
static inline void *LoadAlloc(unsigned int size)
{
    return s_alignLoads ? _memalign(64, size, 0, 0) : _malloc(size, 0, 0);
}

/* the directory off the file's head: an empty file gets a fresh tail (versioned, the directory's
 * number kept, to be written); else the tail read and checked, the string table loaded, the key
 * buffer cleared and sized for the keys, the keys read from a buffer of their own, the file sought
 * past the padding -- and the data either located (mode bit 0x20) or loaded whole, the file then
 * swapped for a buffer over it (SOCOM 1's, grown: SOCOM 1's copied a static zero tail, SOCOM II's
 * clears it -- a brace-initialised struct is MW's null-tested byte loop, as func_002642F0's PNT4D;
 * its key buffer's reserve, 0x2597B0, is byte-exact from here).
 * Byte-exact since round 73: the two tests one `ok = seek != -1 && read == sizeof(tail);` (round 72's
 * tell: `go = A && B` keeps A's bool unmasked and puts the andi in the branch's slot, the join unpadded).
 * Was: OPEN, 3 words (the target's size; 47 instructions before the engine core's round 27): the seek's and
 * the read's results reach `ok` through an `andi` where this converts (sltu s0,zero,v0) -- the target
 * computes them in v0, branches on v0, narrows into s0 in the slot -- and the join of the two tests
 * takes no pad nop there. Tried: int, unsigned char, const bool and enum holders, explicit casts,
 * inline bool / int reads (each folded into the assignment: sltu or 8 bytes short), `ok` a byte (the
 * same, 8 short), `&=` (0x520), a negated inline bool (0x518), asm directives around it.
 * Round 27 closed the rest: pad declared ahead of total (total then takes the key bytes' s3, pad the
 * size's s4); the chain `m_pFile = m_pFileAlloc = b` (the stores' order); a conversion on one of the
 * two arguments (a1's load ahead of a2's: `(unsigned int)` on fread's size, `(void *)(unsigned int)` on
 * Open's buffer -- plain loads are set up right to left, a converted one after them or before as the
 * calls show; an inline accessor is folded and changes nothing) */
bool CZAR::ReadDirectory(int dir, unsigned int mode)
{
    bool ok = false;

    if (IsOpen()) {
        TAIL tail = { 0 };

        ok = true;
        if (!m_pFile->GetSize()) {
            m_tail = tail;
            m_tail.version = 0x20002;
            m_tail.appversion = dir;
            m_modified = 1;
            m_pFile->fseek(0, 0);
        } else {
            ok = m_pFile->fseek(0, 0) != -1 && m_pFile->fread(&tail, sizeof(tail)) == sizeof(tail);
            if (ok) {
                ok = tail.version == 0x20002;
                if (ok) {
                    m_tail = tail;

                    int size = m_tail.stable_size;
                    char *buf = (char *)_malloc(size, 0, 0);

                    m_pFile->fread(buf, size);
                    m_stable->Destroy();
                    m_stable->LoadTable(buf, size, true);

                    int base = (int)buf - m_tail.stable_ofs;

                    m_key_buffer.clear();
                    m_key_buffer.reserve(m_tail.key_count);
                    m_root->clear();

                    CBufferIO io;
                    int bytes = m_tail.key_count * 16;
                    void *keys = LoadAlloc(bytes);

                    m_pFile->fread(keys, bytes);
                    io.Open(keys, bytes);
                    ok = m_root->Read(this, &io, base);
                    io.Close();

                    int pad;
                    int total = bytes + (size + 100);
                    int rem = total % m_tail.padded;

                    if (rem)
                        pad = m_tail.padded - rem;
                    else
                        pad = 0;
                    m_pFile->fseek(pad, 1);
                    if (mode & 0x20)
                        m_data_start = pad + total;
                    else {
                        m_data_size = m_tail.offset;
                        if (m_data_size) {
                            if (!m_24 || !m_databuffer)
                                m_databuffer = LoadAlloc(m_data_size);
                            m_data_start = pad + total;
                            m_pFile->fread(m_databuffer, (unsigned int)m_data_size);
                            if (m_pFileAlloc) {
                                m_pFileAlloc->Close();
                                delete m_pFileAlloc;
                                m_pFile = 0;
                                m_pFileAlloc = 0;

                                CBufferIO *b = new CBufferIO;

                                m_pFile = m_pFileAlloc = b;
                                b->Open((void *)(unsigned int)m_databuffer, m_data_size);
                                m_data_start = 0;
                            }
                        }
                    }
                }
            }
        }
    }
    return ok;
}

int CZAR::ReadString(char **str)
{
    int len = 0;
    int n;

    if ((n = m_pFile->fread(&len, 4)) == 4 && len > 0) {
        *str = (char *)_malloc(len + 1, 0, 0);
        memset(*str, 0, len + 1);
        n += m_pFile->fread(*str, len);
    }
    return n;
}

/* the key's name and its children's made the string table's packed copies (SOCOM 1's
 * CKey::fixupKey 0x1D8600, CSTable::Pack's callback: WriteDirectory hands it over with m_root).
 * Recursive, so it self-inlines; the iterator declared, then assigned begin() -- the target's
 * third level calls the iterator's default constructor and implicit assignment (0x257ED0,
 * 0x257EC0), its pieces right after it (the MSL headers round 24, on include/vector's pointer
 * iterator as the image's: round 23's) */
void CKey::fixupKey(CSTable *table, void *p)
{
    CKey *key = (CKey *)p;

    key->m_name = table->FindString(key->m_name);
    CKeyRing::iterator i;

    for (i = key->begin(); i != key->end(); i++)
        fixupKey(table, *i);
}

}

/* the keys under a key, itself included (SOCOM 1's: recursive, WriteDirectory's tail count).
 * The target self-inlines four levels, each cutting vector<CKey *>'s walk a level more -- its
 * second level calls the raw begin and end (0x257CB0, 0x257BA0), the iterator's constructor from
 * the raw pointer BY REFERENCE (0x257CA0, __iter inlined around it) and, inside the inlined ==,
 * base() on each iterator (0x257B90); the third calls __iter (0x257C90), end() and ==; the fourth
 * every piece (begin 0x257C80, end 0x257C30, != 0x257C10, * 0x257C70, ++(int) 0x257C50). The
 * copies are cut by inline_max_size, not the level. Byte-exact on include/vector's pointer
 * iterator as the image has it (the MSL headers round 23 measured it, round 24 adopted it): the
 * raw ends this element type's own, end() four high */
int _countKeys(zar::CKey *key)
{
    int count = 1;

    for (zar::CKeyRing::iterator i = key->begin(); i != key->end(); i++)
        count += _countKeys(*i);
    return count;
}

namespace zar {

/* a fresh key: put in the key buffer when the file is read (its mode's bit 0), the name made a
 * string of the table's -- else allocated on its own (SOCOM 1's, the same code; SOCOM II's
 * nameless key built by CKey(), its temporary destroyed out of line; one result for both paths,
 * as the target's single exit). Its push_backs instantiate the key buffer's class insert
 * (0x259250) and, through CKey's implicit copies, the CKeyRing pod's copy constructor, operator=,
 * init and do_assign (0x259AF0-0x259EDF, byte-exact in the tail). The insert stays OPEN: it calls
 * __copy_backward and __fill_n out of line at -inline level=5 (0x4F8 against 0x558; CKey's
 * assignment chain is one level too tall -- at level 6 it is the target's size, 53 instructions
 * off), inline_max_size changes nothing */
CKey *CZAR::NewKey(const char *name)
{
    CKey *key;

    if (m_pFile->GetMode() & 1) {
        if (name)
            m_key_buffer.push_back(CKey(m_stable->CreateString(name)));
        else
            m_key_buffer.push_back(CKey());
        key = &m_key_buffer.back();
    } else
        key = new CKey(m_stable->CreateString(name));
    return key;
}

/* Byte-exact: the pad through `if (rem) pad = ...; else pad = 0;` -- a ternary's result is a
 * temporary numbered after the buffer, coloured ahead of it (s1, the buffer s2); a named pad
 * declared before the buffer is coloured after it, as the target's (pad s2, buffer s1). The size
 * read in a statement of its own ahead of the calls, the sum then reduced (`%=` on a named
 * local): the remainder's divisor is loaded after the calls as the target's (`x % m` in one
 * expression loads m first, into a saved register). The divisions are signed (div), the fills
 * signed chars (li -81) */
/* the directory written at the file's end: the tail (versioned, the key count, the string table's
 * size and buffer, the padding and the buffer's bytes), the string table, the keys' records, the
 * data padded to m_data_padded with 0xAF -- then the second file's data copied over in 16 MB
 * blocks, or the buffer's -- and the file padded with 0xBD to the size at 0x2C (SOCOM 1's grown:
 * no Securify; it never returns true) */
bool CZAR::WriteDirectory()
{
    if (IsOpen()) {
        m_tail.version = 0x20002;
        m_tail.key_count = _countKeys(m_root);
        m_stable->Pack(CKey::fixupKey, m_root);
        m_tail.padded = m_data_padded;
        m_tail.stable_size = m_stable->m_bytes;
        m_tail.stable_ofs = (int)m_stable->m_buffer;
        m_tail.offset = m_34 ? m_34->m_used : 0;
        m_pFile->fwrite(&m_tail, sizeof(m_tail));
        m_pFile->fwrite(m_stable->m_buffer, m_stable->m_bytes);
        m_root->Write(this);

        int size = m_stable->m_bytes;
        int rem = size + 100 + m_root->CountKeys(true) * CKey::RecordSize();
        int pad;

        rem %= (int)m_data_padded;
        if (rem)
            pad = (int)m_data_padded - rem;
        else
            pad = 0;
        if (pad) {
            char *buf = (char *)_malloc(pad, 0, 0);

            memset(buf, (char)0xAF, pad);
            m_pFile->fwrite(buf, pad);
            if (buf)
                _free(buf, 0, 0);
        }
        if (m_pDataFile) {
            int left = m_pDataFile->fseek(0, 2);
            char *buf = (char *)_calloc(0x1000000, 1, 0, 0);

            m_pDataFile->fseek(0, 0);
            if (left > 0) {
                do {
                    unsigned int n = left;

                    if (n > 0x1000000)
                        n = 0x1000000;
                    m_pDataFile->fread(buf, n);
                    m_pFile->fwrite(buf, n);
                    left -= 0x1000000;
                } while (left > 0);
            }
        } else if (m_34 && m_34->m_data)
            m_pFile->fwrite(m_34->m_data, m_34->m_used);
        if (m_2c) {
            int size = m_stable->m_bytes;
            int used = size + 100 + m_root->CountKeys(true) * CKey::RecordSize();

            if (m_34 && m_34->m_data)
                used += m_34->m_used;

            int pad = m_2c - used;

            if (pad > 0) {
                char *buf = (char *)_malloc(pad, 0, 0);

                memset(buf, (char)0xBD, pad);
                m_pFile->fwrite(buf, pad);
                if (buf)
                    _free(buf, 0, 0);
            }
        }
        m_modified = 0;
    }
    return false;
}

#pragma auto_inline off
void CZAR::FreeDirectory()
{
    if (m_key_buffer.size() == 0)
        m_root->DeleteChildren(true);
    else {
        m_root->DeleteChildren(false);
        m_key_buffer.clear();
        FreeVector<std::vector<CKey> >(m_key_buffer);
    }
    m_keys.clear();
    FreeVector<std::vector<CKey *> >(m_keys);
    m_stable->Destroy();
}
#pragma auto_inline reset

/* The block's size in a CONST local: it survives copy propagation as a named variable numbered
 * before the fresh flag, so the flag is coloured first (a1) and the size after it (a3), as the
 * target's -- read as the member, the size is a CSE temporary numbered after the flag (a1/a2, 13
 * instructions). With a1 taken, both growths as ternaries unpad their joins as the target's.
 * Measured: `!m_data` materialises the flag as the target's sltu/xori/andi (`== 0` gives
 * xor/sltiu); the base taken before `n += size` gives its li/addu pair; `end` declared ahead of `at`
 * gives at s0 and end s1 */
unsigned int CZARBuffer::Write(const void *data, unsigned int size)
{
    const unsigned int cur = m_size;

    if (cur < m_used + size) {
        bool fresh = !m_data;
        unsigned int n = fresh ? 4096 : cur * m_grows + 0x80000;

        n += size;
        if (cur < n) {
            m_data = fresh ? (char *)_calloc(n, 1, 0, 0) : (char *)_realloc(m_data, n, 0, 0);
            m_size = n;
            m_grows++;
        }
    }

    unsigned int end;
    unsigned int at = m_used;

    end = at + size;

    if (at < end) {
        if (m_size < end) {
            m_data = m_data ? (char *)_realloc(m_data, end, 0, 0) : (char *)_calloc(end, 1, 0, 0);
            m_size = end;
            m_grows++;
        }
        m_used = end;
    }
    memcpy(m_data + at, data, size);
    return size;
}

unsigned int CZAR::WriteBuffer(const void *data, unsigned int size)
{
    if (!m_34)
        m_34 = new CZARBuffer;
    return m_34->Write(data, size);
}

}
