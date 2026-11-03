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

/* the name every unnamed texture shares (by address: SOCOM 1's gp-relative static) */
extern char *data_003E02D8;

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

/* SOCOM 1's: every field cleared, a 16-bit bilinear texture of no size, the name copied. The
 * fields from the name on are initialised ahead of the mip list (the target's stores before its
 * three words), TEXTURE_PARAMS' in the body */
CTexture::CTexture(const char *name)
    : m_name(data_003E02D8), m_buffer(0), m_palette(0), m_gifSelect(0), m_vuSelect(0), m_format(0),
      m_palID(0), m_htex_count(0), m_gsWords(0)
{
    m_texelBitSize = 16;
    m_height = 0;
    m_width = 0;
    m_gsaddr = 0;
    m_selectQwc = 0;
    m_pal_offset = 0;
    SetTransparent(false);
    SetPalettized(false);
    SetIsMipChild(false);
    SetBumpmap(false);
    SetBilinear(true);
    SetTransp1Bit(false);
    SetDynamic(false);
    SetContext(false);
    m_AssetLib = 0;
    SetName(name);
    m_loadimage = 0;
    m_TEX0 = 0;
    m_MIP1 = 0;
    m_MIP2 = 0;
}

/* SOCOM 1's: the name and the image buffer freed, the mip list torn down. The name goes through
 * SetName(0), inlined -- its strdup arm left in place behind an unconditional branch -- then the
 * default again (SOCOM 1's does the same two stores) */
CTexture::~CTexture()
{
    SetName(0);
    m_name = data_003E02D8;
    if (m_loadimage)
        _free(m_loadimage, 0, 0);
    m_loadimage = 0;
}

/* SOCOM 1's: the old name freed unless it is the shared default */
void CTexture::SetName(const char *name)
{
    if (m_name && m_name != data_003E02D8)
        _free(m_name, 0, 0);
    m_name = name ? _strdup(name, 0, 0) : data_003E02D8;
}

/* translucent and blended: neither bump-mapped nor 1-bit (SOCOM II's; the name ours -- the
 * particles' draw asks it) */
bool CTexture::IsAlphaBlended()
{
    return m_transparent && !m_bumpmap && !m_transp_1bit;
}

/* SOCOM 1's: the texture's GS memory in 64-word blocks, rounded up -- a narrow low-depth texture
 * takes twice its texels */
unsigned int CTexture::Compute64WordMemoryUsage()
{
    unsigned int n = m_width * m_height;

    if (m_texelBitSize < 9 && m_height * 2 < m_width && m_texelBitSize * m_width < 2048)
        n *= 2;
    switch (m_texelBitSize) {
    case 4:
        n >>= 3;
        break;
    case 8:
        n >>= 2;
        break;
    case 16:
        if (m_palettized)
            n >>= 2;
        else
            n >>= 1;
        break;
    case 24:
    case 32:
        if (m_palettized)
            n >>= 2;
        else if (m_transp_1bit)
            n >>= 1;
        break;
    }
    if (n & 0x3f)
        n += 64 - (n & 0x3f);
    return n >> 6;
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
