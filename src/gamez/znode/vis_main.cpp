/* vis_main.cpp -- zdb::CVisual, a node's visual: its reading, creation, construction and texture
 * selects (SOCOM 1's vis_main.cpp, by the press beta's asserts: UpdateCentroid and
 * GetTextureSelectBlock_ver2 carry its name): OVL1 0x3C3570?-0x3C469F, after vis_texfx.cpp and
 * before zwep_list.cpp. SOCOM 1's unit also held the VU code (VuUpdate .. Init), which SOCOM II
 * links earlier (0x3B3AB0-0x3B7FDF, with vis_main's static initialiser's statics); which file each
 * half was is not settled. Only some of its functions so far; the rest stay generated drafts or
 * placeholders. Defined in reverse of their image order (-inline deferred). */

#include "znode.h"
#include "../zar/zar.h"
#include "../zsys/zsys_public.h"

using namespace zdb;

/* zar::CZAR::Fetch(const char *, int *), which zar.h does not declare yet (0x25AB70) */
extern "C" bool func_0025AB70(zar::CZAR &zar, const char *name, int *value);

/* a visual of the archive's type, read; none if the read fails (SOCOM 1's) */
CVisual *CVisual::Create(zar::CZAR &zar)
{
    int type = -1;

    func_0025AB70(zar, "vtype", &type);
    CVisual *v = 0;
    switch (type) {
    case -1:
        v = new CVisual;
        break;
    case 0:
        v = new CVisual;
        break;
    case 1:
        v = new CMesh;
        break;
    case 2:
        v = new CSubMesh(0);
        break;
    }
    if (v && !v->Read(zar)) {
        delete v;
        v = 0;
    }
    return v;
}

/* the specular maps' storage given back (SOCOM 1's) */
void CVisual::Flush()
{
    FreeVector(m_specMapList);
}

/* the head's parameters, then the detail layers: their count, the block's size and the block,
 * each layer's data pointer set into the block after the layers (SOCOM 1's) */
bool CVisual::Read(zar::CZAR &zar)
{
    zar.Fetch("vparams", this, 0x18);
    if (m_bits.m_b10)
        m_1c = -2;
    else
        m_1c = 3;
    if (m_details)
        _free(m_details, 0, 0);
    m_details = 0;
    m_numDetails = 0;
    zar.Fetch("detail_cnt", &m_numDetails, 4);
    if (m_numDetails) {
        unsigned int size;

        zar.Fetch("detail_size", &size, 4);
        m_details = (DETAIL *)_malloc(size, 0, 0);
        zar.Fetch("detail_buff", m_details, size);
        DETAIL *d = m_details;
        char *data = (char *)(d + m_numDetails);

        for (unsigned int i = 0; i < m_numDetails; i++, d++) {
            d->m_data = data;
            data += d->m_count * 12;
        }
    }
    return true;
}

/* a texture-select block's head: zeroed, then one flag set (the names ours) */
struct TEXSELECT_HEAD {
    unsigned int m_0;
    unsigned int m_4;
    unsigned int m_8;
    unsigned int m_c0 : 1;
    unsigned int m_ver2 : 1;
    unsigned int m_c2 : 30;
};

_word128 *CVisual::GetTextureSelectBlock_ver2(unsigned int n, _word128 **cursor, _word128 *ref,
                                         _word128 *tag)
{
    _word128 *head = (*cursor)++;

    head->u128 = 0;
    ((TEXSELECT_HEAD *)head)->m_ver2 = 1;
    _word128 *end = *cursor + n;
    end->u32[0] = 0x15000000;
    end->u32[1] = 0;
    end->u32[2] = 0;
    end->u32[3] = 0;
    ref->u32[0] = n | 0x30000000;
    ref->u32[1] = (unsigned int)*cursor;
    ref->u32[2] = 0x11000000;
    ref->u32[3] = n | 0x50000000;
    tag->u32[0] = (n + 2) | 0x30000000;
    tag->u32[1] = (unsigned int)head;
    tag->u32[2] = 0x01000101;
    tag->u32[3] = ((n + 1) << 16) | 0x6C008000;
    return head;
}

CTexture *CVisual::ResolveTextureName(_word128 *names, _word128 *select)
{
    CTexture *tex = 0;
    CTexHandle *h = CWorld::GetTexHandle((char *)names + select->u32[1]);

    if (!h)
        h = CWorld::GetTexHandle("null_xmas.bmp");
    if (h) {
        tex = h->m_texture;
        m_dynTexList.Add(h, true);
        if (tex->m_palette)
            m_dynTexList.Add(tex->m_palette, true);
    }
    if (tex) {
        select->u128 = tex->m_dmaRefVu;
        select->u8[2] = 5;
    }
    return tex;
}
