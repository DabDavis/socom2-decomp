/* c2dbitmap.cpp -- C2DBitmap (OVL1 0x3641F0-0x364D5F): SOCOM 1's functions in SOCOM 1's order
 * (GetAssetLibs, TickFade, Reset, MakePacket, the two SetPos, the two Load, SetUV, GetTrans,
 * SetTrans, SetColor, the two Draw, the constructor), with SOCOM II's own among them: a colour
 * from a point and its getter, and the four flag setters that keep the GS PRIM bits (vtable slots
 * 6-9). The unit's name is ours (SOCOM 1's symbol table groups C2DBitmap's RTTI with C2D::Open and
 * Init, which SOCOM II moved to ztwod.cpp); it ends where C2D's begins. Written, all byte-exact:
 * everything but GetAssetLibs, Reset and SetUV (m2c's drafts match) and MakePacket. Defined in
 * reverse of their image order (-inline deferred). */

#include <eetypes.h>

#include "ztwod.h"

/* the texture a handle holds, as the bitmap reads it: its size two halfwords at 0 */
namespace zdb {
class CTexture {
public:
    unsigned short m_width;   // 0x0
    unsigned short m_height;   // 0x2
};

class CTexHandle {
public:
    unsigned char _0[0xc];
    CTexture *m_texture;   // 0xc
};

/* the texture relocation managers (SOCOM 1's; the slots as CBatchRelocator's vtable 0x406C10 holds
 * them): a batch relocator is only its vtable pointer, all its state the texture manager's */
class CTextureRelocMgr {
public:
    virtual void AddToBatch(CTexHandle *tex) const;
    virtual void EndBatchReloc() const;
    virtual void PreGetSprPacket(CTexHandle *tex);
    virtual void StartBatchReloc() const;
    virtual void TakeDrawPacket(u_long128 *packet, unsigned int qwc);
    virtual void ClearAll();
};

class CBatchRelocator : public CTextureRelocMgr {
public:
    virtual void AddToBatch(CTexHandle *tex) const;
    virtual void EndBatchReloc() const;
    virtual void StartBatchReloc() const;
    virtual void TakeDrawPacket(u_long128 *packet, unsigned int qwc);
    virtual void ClearAll();
};
}

/* a texture asked for and not there: the first-draw flag dropped instead of drawing (SOCOM 1's test,
 * in both Loads and both Draws, written out each time: an inline bool would materialise) */
#define TEXTURE_MISSING(h) (m_hasTexture && !((h) ? (h)->m_texture : 0))

/* untextured until loaded, the UVs a hundredth in from the edges, white then grey, opaque, and a
 * sprite's PRIM word (6) with the flags' bits */
C2DBitmap::C2DBitmap()
{
    m_pTexHandle = 0;
    m_iWidth = 0;
    m_iHeight = 0;
    m_x = 0;
    m_y = 0;
    m_NewUV[0][0] = 0.01f;
    m_NewUV[0][1] = -0.01f;
    m_NewUV[0][2] = 0.0f;
    m_NewUV[0][3] = 0.0f;
    m_NewUV[1][0] = 0.99f;
    m_NewUV[1][1] = -0.99f;
    m_NewUV[1][2] = 1.0f;
    m_NewUV[1][3] = 0.0f;
    m_RGB[0][0] = 255.0f;
    m_RGB[0][1] = 255.0f;
    m_RGB[0][2] = 255.0f;
    m_RGB[0][3] = 100.0f;
    m_RGB[1][0] = m_RGB[0][0];
    m_RGB[1][1] = m_RGB[0][1];
    m_RGB[1][2] = m_RGB[0][2];
    m_RGB[1][3] = m_RGB[0][3];
    SetColor(128.0f, 128.0f, 128.0f);
    SetTrans(128.0f);
    MakePrim(m_prim, 6);
}

/* slot 6: with a texture to draw, TME */
void C2DBitmap::SetHasTexture(bool on)
{
    m_hasTexture = on;
    if (on && m_pTexHandle)
        m_prim |= 0x10;
    else if (m_prim & 0x10)
        m_prim ^= 0x10;
}

/* slot 7: AA1 */
void C2DBitmap::SetAA(bool on)
{
    m_isAA = on;
    if (on)
        m_prim |= 0x80;
    else if (m_prim & 0x80)
        m_prim ^= 0x80;
}

/* slot 8: ABE */
void C2DBitmap::SetIsTrans(bool on)
{
    m_isTrans = on;
    if (on)
        m_prim |= 0x40;
    else if (m_prim & 0x40)
        m_prim ^= 0x40;
}

/* slot 9: IIP */
void C2DBitmap::SetGouraud(bool on)
{
    m_socom2_bit5 = on;
    if (on)
        m_prim |= 0x8;
    else if (m_prim & 0x8)
        m_prim ^= 0x8;
}

/* slot 1: on, packeted (unless its texture is missing) through a batch relocator of its own, then
 * the children */
void C2DBitmap::Draw(zdb::CCamera *cam)
{
    zdb::CBatchRelocator reloc;

    if (IsOn() != false) {
        if (TEXTURE_MISSING(m_pTexHandle)) {
            if (first)
                first = false;
        } else
            MakePacket(cam, &reloc, 0);
        C2D::Draw(cam);
    }
}

/* slot 2: the same through the caller's matrix and relocator, with the 2D camera */
void C2DBitmap::Draw(const CMatrix &m, zdb::CTextureRelocMgr *mgr)
{
    zdb::CCamera *cam = m_p2dcamera;

    if (IsOn() != false) {
        if (TEXTURE_MISSING(m_pTexHandle)) {
            if (first)
                first = false;
        } else
            MakePacket(cam, mgr, &m);
        C2D::Draw(cam);
    }
}

/* both rows' colour (SOCOM 1's) */
void C2DBitmap::SetColor(float r, float g, float b)
{
    m_RGB[0][0] = r;
    m_RGB[0][1] = g;
    m_RGB[0][2] = b;
    m_RGB[1][0] = r;
    m_RGB[1][1] = g;
    m_RGB[1][2] = b;
}

/* both rows' colour from a point, through the floats' (inlined: the components loaded first,
 * last to first; SOCOM II's, the name ours) */
void C2DBitmap::SetColor(const CPnt3D &color)
{
    SetColor(color.x, color.y, color.z);
}

/* the first row's colour (SOCOM II's; the name ours, as C2DPoly's) */
float *C2DBitmap::GetColor()
{
    return m_RGB[0];
}

/* slot 11: both rows' alpha; opaque (128) drops the blending, anything else keeps it */
void C2DBitmap::SetTrans(float trans)
{
    m_RGB[0][3] = trans;
    m_RGB[1][3] = trans;
    if (m_isTrans && trans == 128.0f) {
        m_isTrans = false;
        if (m_prim & 0x40)
            m_prim ^= 0x40;
    }
    if (!m_isTrans) {
        m_isTrans = true;
        m_prim |= 0x40;
    }
}

float C2DBitmap::GetTrans()
{
    return m_RGB[0][3];
}

/* SOCOM 1's Load at a point: the handle, then placed as big as its texture (SetPos's, inlined) */
void C2DBitmap::Load(float x, float y, zdb::CTexHandle *tex)
{
    m_pTexHandle = tex;
    if (TEXTURE_MISSING(tex)) {
        if (first)
            first = false;
        return;
    }
    SetPos((int)x, (int)y);
}

/* SOCOM 1's Load by the corners */
void C2DBitmap::Load(float x1, float y1, float x2, float y2, zdb::CTexHandle *tex)
{
    m_pTexHandle = tex;
    m_x = (int)x1;
    m_y = (int)y1;
    m_iWidth = (int)x2 - (int)x1;
    m_iHeight = (int)y2 - (int)y1;
    if (TEXTURE_MISSING(tex)) {
        if (first)
            first = false;
        return;
    }
}

/* placed at the point, as big as its texture (none: not moved) */
void C2DBitmap::SetPos(int x, int y)
{
    if (m_pTexHandle ? m_pTexHandle->m_texture : 0) {
        m_x = x;
        m_y = y;
        m_iWidth = m_pTexHandle->m_texture->m_width;
        m_iHeight = m_pTexHandle->m_texture->m_height;
    }
}

/* placed by its corners */
void C2DBitmap::SetPos(int x1, int y1, int x2, int y2)
{
    m_x = x1;
    m_y = y1;
    m_iWidth = x2 - x1;
    m_iHeight = y2 - y1;
}

/* while fading, the alpha stepped towards the limit (no lower than 0, no higher than 128) */
void C2DBitmap::TickFade()
{
    if (m_fade_enable) {
        float trans = m_RGB[0][3];

        trans += m_fade_dx;

        if (m_fade_dx < 0.0f) {
            if (trans < m_fade_limit)
                trans = m_fade_limit;
            else if (trans < 0.0f)
                trans = 0.0f;
        } else {
            if (trans > m_fade_limit)
                trans = m_fade_limit;
            else if (trans > 128.0f)
                trans = 128.0f;
        }
        SetTrans(trans);
    }
}
