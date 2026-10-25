/* ztex_manager.cpp -- zdb's texture manager, its tail (OVL1 0x357000-0x357E8F): the press beta's
 * asserts name the unit (ztex_manager.cpp, the 1.0 beta's `tu:` rows from 0x355840 on); SOCOM 1's
 * functions in SOCOM 1's order (CGSTexBuffer's Hookup, Setup and Add(CTexList *), SetDynTexList,
 * CDynTexList's three Adds, CGSTexBuffer's destructor and constructor, then the template tail).
 * The unit starts in the zdb world's range (from ~0x3557xx); this file holds the HUD and 2D lane's
 * share of it, kept with the 2D family as zui_2d.cpp is until zdb's textures have a directory of
 * their own. Written so far: CDynTexList's two Adds (the handle's byte-exact). Defined in reverse of their image order
 * (-inline deferred). */

#include <algorithm>
#include <string.h>
#include <vector>

namespace zdb {

class CTexPalette;

class CTexPalList;

/* a texture as the lists read it: its name record, its palette and the palette's id (the names ours
 * but m_palette) */
class CTexture {
public:
    bool FindPalette(CTexPalList *list);
    bool HasMipLevel(CTexture *tex);

    unsigned char _0[0x10];
    char **m_name;   // 0x10: the name first
    unsigned char _14[0x1c - 0x14];
    CTexPalette *m_palette;   // 0x1c
    unsigned char _20[0x2a - 0x20];
    unsigned short m_palid;   // 0x2a
    unsigned char _2c[0x5c - 0x2c];
    std::vector<CTexture *> m_mips;   // 0x5c: its mip levels (SOCOM II's)
};

/* a palette as the texture matches it: its name record and id (the names ours) */
class CTexPalette {
public:
    unsigned char _0[0x8];
    char **m_name;   // 0x8
    unsigned char _c[0x10 - 0xc];
    unsigned int m_id;   // 0x10
};

/* a handle on a texture (its texture at 0xc) */
class CTexHandle {
public:
    unsigned char _0[0xc];
    CTexture *m_texture;   // 0xc
};

/* a library's textures and palettes (SOCOM 1's DWARF: two pointer vectors; znode.h's) */
class CTexList : public std::vector<CTexHandle *> {
public:
    CTexHandle *GetHandle(CTexture *tex);
};

class CTexPalList : public std::vector<CTexPalette *> {
public:
    /* whether the palette is listed (ours, inline: the target materialises the test, then branches) */
    bool Exists(CTexPalette *pal) { return std::find(begin(), end(), pal) != end(); }
};

/* a texture buffer's dynamic textures (SOCOM 1's DWARF: CDynTexList, two vectors; znode.h's) */
class CDynTexList {
public:
    void Add(CTexHandle *tex, bool b);
    void Add(CTexPalette *pal, bool b);
    void Add(CDynTexList *list);

    CTexList m_tex_list;   // 0x0
    CTexPalList m_pal_list;   // 0xc
};   // 0x18 bytes

class CAssetLib;

/* a GS texture buffer (SOCOM 1's DWARF: 0x40 bytes) */
class CGSTexBuffer {
public:
    void Add(CTexList *list, bool b);

    char *m_name;   // 0x0
    unsigned char m_checkForOverflow;   // 0x4
    unsigned int m_startAddress;   // 0x8
    unsigned int m_nextAddress;   // 0xc
    unsigned int m_endAddress;   // 0x10
    CAssetLib *m_assetLib;   // 0x14
    void *m_chainp;   // 0x18
    std::vector<unsigned int> m_pktbuf;   // 0x1c
    CDynTexList m_dyntex_list;   // 0x28
};   // 0x40 bytes

}

/* the buffer's chain dropped (SOCOM 1 stored m_chainp = 0 in place; SOCOM II calls this, 0x355A20,
 * out of line -- no name yet) */
extern "C" void func_00355A20(zdb::CGSTexBuffer *buf);

namespace zdb {

/* whether a texture is one of this one's mip levels (CAssetList::FindMipLevel's test; the name
 * ours) */
bool CTexture::HasMipLevel(CTexture *tex)
{
    bool found = false;

    for (int i = 0; i < m_mips.size() && !found; i++)
        found = m_mips[i] == tex;
    return found;
}

/* the texture's palette looked for in a list, by id and name, and hooked up when found (called three
 * times by 0x3571D0; the name ours) */
bool CTexture::FindPalette(CTexPalList *list)
{
    bool found = false;

    for (CTexPalList::iterator i = list->begin(); i != list->end(); ++i) {
        CTexPalette *pal = *i;

        if (m_palid == pal->m_id && !strcmp(*pal->m_name, *m_name)) {
            m_palette = pal;
            found = true;
            break;
        }
    }
    return found;
}

/* a library's textures put on the buffer's dynamic list, each palette after its texture, the chain
 * dropped after each (SOCOM 1's; its palette Add inlined) */
void CGSTexBuffer::Add(CTexList *list, bool b)
{
    unsigned int n = list->size();

    for (unsigned int i = 0; i < n; i++) {
        CTexHandle *tex = (*list)[i];

        m_dyntex_list.Add(tex, b);
        func_00355A20(this);
        if (tex->m_texture->m_palette) {
            m_dyntex_list.Add(tex->m_texture->m_palette, b);
            func_00355A20(this);
        }
    }
}

/* another list's handles and palettes added, each once */
void CDynTexList::Add(CDynTexList *list)
{
    if (list) {
        for (CTexList::iterator i = list->m_tex_list.begin(); i != list->m_tex_list.end(); ++i)
            Add(*i, true);
        for (CTexPalList::iterator i = list->m_pal_list.begin(); i != list->m_pal_list.end(); ++i)
            Add(*i, true);
    }
}

/* a handle listed -- unless b and its texture is already there -- and its texture's palette after
 * it (SOCOM 1's inlined the palette's Add) */
void CDynTexList::Add(CTexHandle *tex, bool b)
{
    if (b && m_tex_list.GetHandle(tex->m_texture))
        return;
    if (m_tex_list.size() == m_tex_list.capacity())
        m_tex_list.reserve(m_tex_list.size() + 1);
    m_tex_list.push_back(tex);
    if (tex->m_texture->m_palette)
        Add(tex->m_texture->m_palette, b);
}

/* a palette listed, unless b and it is already there.
 * OPEN, 9 instructions: the inlined find's begin() and end() take each other's registers (the
 * target loads the size into a3 and builds end() there, the data pointer in t0, and stores end's
 * temporary first); the test through an inline bool (Exists) gives the target's materialised `!=`
 * and the value's early load -- written in the if it is 14 off; `iterator i = find(...); return i
 * != end();` 10, an end() local or `end() != find(...)` change the size */
void CDynTexList::Add(CTexPalette *pal, bool b)
{
    if (b && m_pal_list.Exists(pal))
        return;
    if (m_pal_list.size() == m_pal_list.capacity())
        m_pal_list.reserve(m_pal_list.size() + 1);
    m_pal_list.push_back(pal);
}

}
