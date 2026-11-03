/* ztex_main.cpp -- zdb's textures: a library's handle list, the texture itself, its handle and
 * its palette, the batch relocator (OVL1 0x354510-0x355A6F). The unit name is SOCOM 1's (the press
 * beta's asserts put CTexture::Release_HTEX in a ztex_main.cpp); its functions in SOCOM 1's order
 * (CTexList's Free, Append and two GetHandles, CTexture's, CTexHandle's SetTexture/Destroy/Create,
 * CTexPalette's, CBatchRelocator's). Where the unit ends is not settled: the texture manager,
 * ztex_manager.cpp, follows (its tail is src/gamez/ztwod/ztex_manager.cpp, the HUD lane's).
 * Defined in reverse of their image order (-inline deferred). */

#include <algorithm>
#include <string.h>

#include "../znode/node_world.h"

char *_strdup(const char *s, const char *file, int line);

namespace zdb {

/* a palette made with its name: the next palette id its GS address (SOCOM 1's, the same code) */
CTexPalette::CTexPalette(const char *name)
{
    m_name = _strdup(name, 0, 0);
    m_loadimage = 0;
    m_gsaddr = m_palId;
    m_slots = 0;
    m_size = 0;
    m_buffer = 0;
    SetDynamic(false);
    SetUnused(0);
    m_AssetLib = 0;
    m_palId++;
}

/* a handle made on a texture, counted (SOCOM 1's). Never inlined in the target (CTexList::Append
 * calls it, as SOCOM 1's does), though -inline auto would take its three statements: pinned */
#pragma auto_inline off
CTexHandle *CTexHandle::Create(CTexture *tex)
{
    CTexHandle *h = new CTexHandle(tex);

    h->m_count++;
    return h;
}
#pragma auto_inline reset

/* the handle moved to another texture: the old one's handle count given back, the new one's name
 * copied and its count taken (SOCOM 1's) */
void CTexHandle::SetTexture(CTexture *tex)
{
    if (m_texture)
        m_texture->Release_HTEX();
    m_texture = tex;
    if (tex) {
        SetName(tex->m_name);
        tex->m_htex_count++;
    }
}

/* CTexList::GetHandle(const char *)'s test: a handle's texture of that name, any case (ours) */
struct CTexHandleNamed {
    CTexHandleNamed(const char *name) : m_name(name) {}
    /* the two null tests branch, only the comparison is materialised (a `&&` of all three
     * materialises each test) */
    bool operator()(CTexHandle *h) const
    {
        CTexture *tex = h->m_texture;

        if (tex && tex->m_name)
            return strcasecmp(tex->m_name, m_name) == 0;
        return false;
    }

    const char *m_name;
};

/* CTexList::GetHandle(CTexture *)'s test: a handle on that texture (ours) */
struct CTexHandleOf {
    CTexHandleOf(CTexture *tex) : m_tex(tex) {}
    bool operator()(CTexHandle *h) const { return h->m_texture == m_tex; }

    CTexture *m_tex;
};

/* the handle of that texture, or none (SOCOM 1's). Never inlined in the target (Append and
 * CDynTexList::Add call it), though -inline auto would take it: pinned */
#pragma auto_inline off
CTexHandle *CTexList::GetHandle(CTexture *tex)
{
    iterator i = std::find_if(begin(), end(), CTexHandleOf(tex));

    if (i != end())
        return *i;
    return 0;
}
#pragma auto_inline reset

/* the handle of a texture of that name, or none (SOCOM 1's) */
CTexHandle *CTexList::GetHandle(const char *name)
{
    iterator i = std::find_if(begin(), end(), CTexHandleNamed(name));

    if (i != end())
        return *i;
    return 0;
}

/* a texture's handle listed -- with b, the one already listed reused (SOCOM 1's) */
CTexHandle *CTexList::Append(CTexture *tex, bool b)
{
    CTexHandle *h = 0;

    if (b)
        h = GetHandle(tex);
    if (!h) {
        h = CTexHandle::Create(tex);
        push_back(h);
    }
    return h;
}

/* CTexList::Free's operation: a handle destroyed, its slot left null (ours) */
struct CTexHandleDestroy {
    CTexHandleDestroy() {}
    CTexHandle *operator()(CTexHandle *h)
    {
        CTexHandle::Destroy(h);
        return 0;
    }
};

/* every handle let go of, the list emptied (SOCOM 1's: the std::transform of the list onto
 * itself -- two begin()s and an end(), right to left) */
void CTexList::Free()
{
    std::transform(begin(), end(), begin(), CTexHandleDestroy());
    clear();
}

}
