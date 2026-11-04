/* zar_main.cpp -- zar::CZAR, the archive (OVL1 0x256FD0-0x259EDF, the README's zar units): its
 * file, its stack of open keys and what it fetches. The unit name is SOCOM 1's (the press beta's
 * asserts name it). Defined in reverse of their image order (-inline deferred). Written: the
 * constructor, the destructor, SetFilename, both Closes, CloseKeepDir, Flush, the key stack's six
 * (CloseKey .. GetSize), ReadString, NewKey and FreeDirectory -- whose FreeVectors give the vector<CKey>
 * and vector<CKey *> pieces 0x2573E0-0x25757F right after it, and the key buffer's clear and
 * deleter 0x2599B0/0x259A30 in the tail. Not yet: Open 0x258920, WriteDirectory, the
 * buffer writers below 0x257320, the directory reader 0x258000 and the CKey vector's class insert
 * 0x259250. */

#include "zar.h"
#include "../zutil/util_stable.h"
#include "../zsys/zsys.h"

/* the root key's name and a deleted key's (SOCOM 1's file statics, "ROOT" and "DELETED"; OVL1
 * 0x3DD000 and 0x3DD008, 8 bytes apart as MW 3.0 lays such statics out) */
/* MSL's strtok (MAIN; not in include/string.h) */
extern "C" char *strtok(char *s, const char *delim);

static const char *DEFAULT_ZAR_NAME = "ROOT";
static const char *DELETED_ZAR_NAME = "DELETED";

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
                m_tail._5c = dir;
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

}
