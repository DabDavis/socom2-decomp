/* zar_main.cpp -- zar::CZAR, the archive (OVL1 0x258000-...): its file, its stack of open keys
 * and what it fetches. The unit name is SOCOM 1's (the press beta's asserts name it). Defined in
 * reverse of their image order (-inline deferred). */

#include "zar.h"

/* the root key's name and a deleted key's (SOCOM 1's file statics, "ROOT" and "DELETED"; OVL1
 * 0x3DD000 and 0x3DD008, 8 bytes apart as MW 3.0 lays such statics out) */
static const char *DEFAULT_ZAR_NAME = "ROOT";
static const char *DELETED_ZAR_NAME = "DELETED";

namespace zar {

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

}
