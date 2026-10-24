/* zar_main.cpp -- zar::CZAR, the archive (OVL1 0x258000-...): its file, its stack of open keys
 * and what it fetches. The unit name is SOCOM 1's (the press beta's asserts name it). Defined in
 * reverse of their image order (-inline deferred). */

#include "zar.h"

/* the ring's insert(position, n, x): MSL's vector<CKey *>, not named */
extern "C" void func_001FE340(zar::CKeyRing *ring, zar::CKey **pos, unsigned int n, zar::CKey *const *x);

namespace zar {

/* a key pushed when there is one: SOCOM 1's (a list's push_front) tests twice too */
inline void CKeyRing::push(CKey *key)
{
    if (key)
        func_001FE340(this, m_data + m_size, 1, &key);
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

}
