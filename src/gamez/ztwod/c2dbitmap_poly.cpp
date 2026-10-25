/* c2dbitmap_poly.cpp -- C2DBitmapPoly, a bitmap drawn as two C2DPolys (OVL1 0x357E90-0x35973F):
 * SOCOM 1's unit (its name from a local class's mangling, C2DWidePolyline's CPnt2D in
 * c2dbitmap_poly_cpp), its functions in SOCOM 1's order -- C2DWidePolyline::SetSegments,
 * GetAssetLibs, the two Rotate_Zaxis, the three flips, SetUseFrameBufferAlpha, GetTrans, SetTrans,
 * SetColor, the two SetUV, SetCenterPos, SetPos, Draw, Load and the constructor -- with SOCOM II's
 * own among them: a corner's alpha after GetTrans and a rectangle's SetPos before the corners'.
 * The vtable's inline flag setters open it (SetAA and SetGouraud linked from here; the other two
 * copies were linked from the CZPauseTest unit first). Defined in reverse of their image order
 * (-inline deferred). */

#include <algorithm>

#include "ztwod.h"

/* a texture as the bitmap reads it: the asset library it came from */
namespace zdb {
class CTexture {
public:
    unsigned char _0[0x10];
    CAssetLib *m_assetlib;   // 0x10
};
}

/* a row of four floats copied (the flips move whole vertices) */
static inline void CopyRow(float *d, const float *s)
{
    d[0] = s[0];
    d[1] = s[1];
    d[2] = s[2];
    d[3] = s[3];
}

/* all four corners at once, by value (an inline: the callers copy the points into its parameters,
 * the last first) */
static inline void SetCorners(C2DBitmapPoly *poly, PNT2D p1, PNT2D p2, PNT2D p3, PNT2D p4)
{
    poly->m_tris[0].SetPos(p1.x, p1.y, p2.x, p2.y, p3.x, p3.y);
    poly->m_tris[1].SetPos(p1.x, p1.y, p3.x, p3.y, p4.x, p4.y);
}

/* the whole texture, grey, opaque */
C2DBitmapPoly::C2DBitmapPoly()
{
    m_texture = 0;
    m_tris[0].SetUV(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
    m_tris[1].SetUV(0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f);
    m_tris[0].SetColor(128.0f, 128.0f, 128.0f);
    m_tris[1].SetColor(128.0f, 128.0f, 128.0f);
    SetTrans(128.0f);
}

void C2DBitmapPoly::Load(float x1, float y1, float x2, float y2, float x3, float y3, float x4,
                         float y4, zdb::CTexture *tex)
{
    m_texture = tex;
    m_tris[0].Load(x1, y1, x2, y2, x3, y3, tex);
    m_tris[1].Load(x1, y1, x3, y3, x4, y4, tex);
}

/* while on: both triangles, then the children */
void C2DBitmapPoly::Draw(zdb::CCamera *cam)
{
    if (IsOn()) {
        m_tris[0].Draw(cam);
        m_tris[1].Draw(cam);
        C2D::Draw(cam);
    }
}

void C2DBitmapPoly::SetPos(float x1, float y1, float x2, float y2, float x3, float y3, float x4,
                           float y4)
{
    m_tris[0].SetPos(x1, y1, x2, y2, x3, y3);
    m_tris[1].SetPos(x1, y1, x3, y3, x4, y4);
}

void C2DBitmapPoly::SetPos(float x, float y, float w, float h)
{
    PNT2D p1 = {x, y};
    PNT2D p2 = {x, y + h};
    PNT2D p3 = {x + w, y + h};
    PNT2D p4 = {x + w, y};

    SetCorners(this, p1, p2, p3, p4);
}

void C2DBitmapPoly::SetCenterPos(PNT2D center, float w, float h)
{
    float hw = 0.5f * w;
    float hh = 0.5f * h;

    m_tris[0].SetPos(center.x - hw, center.y - hh, center.x - hw, center.y + hh, center.x + hw,
                     center.y + hh);
    m_tris[1].SetPos(center.x - hw, center.y - hh, center.x + hw, center.y + hh, center.x + hw,
                     center.y - hh);
}

void C2DBitmapPoly::SetUV(float u0, float v0, float u1, float v1)
{
    m_tris[0].SetUV(u0, v0, u0, v1, u1, v1);
    m_tris[1].SetUV(u0, v0, u1, v1, u1, v0);
}

void C2DBitmapPoly::SetUV(float u1, float v1, float u2, float v2, float u3, float v3, float u4,
                          float v4)
{
    m_tris[0].SetUV(u1, v1, u2, v2, u3, v3);
    m_tris[1].SetUV(u1, v1, u3, v3, u4, v4);
}

void C2DBitmapPoly::SetColor(float r, float g, float b)
{
    m_tris[0].SetColor(r, g, b);
    m_tris[1].SetColor(r, g, b);
}

/* slot 11: both triangles' alpha, and blended */
void C2DBitmapPoly::SetTrans(float trans)
{
    m_tris[0].SetTrans(trans);
    m_tris[1].SetTrans(trans);
    m_isTrans = true;
}

/* one corner's alpha: the top left and bottom right are in both triangles; blended and Gouraud
 * shaded after */
void C2DBitmapPoly::SetTrans(int corner, float trans)
{
    switch (corner) {
    case 0:
        m_tris[0].SetTrans(0, trans);
        m_tris[1].SetTrans(0, trans);
        break;
    case 1:
        m_tris[0].SetTrans(1, trans);
        break;
    case 2:
        m_tris[0].SetTrans(2, trans);
        m_tris[1].SetTrans(1, trans);
        break;
    default:
        m_tris[1].SetTrans(3, trans);
        break;
    }
    m_isTrans = true;
    m_socom2_bit5 = true;
}

float C2DBitmapPoly::GetTrans()
{
    return m_tris[0].GetTrans();
}

/* slot 20: the flag, and both triangles' */
void C2DBitmapPoly::SetUseFrameBufferAlpha(bool isFrameAlpha)
{
    m_isFrameAlpha = isFrameAlpha;
    m_tris[0].SetUseFrameBufferAlpha(isFrameAlpha);
    m_tris[1].SetUseFrameBufferAlpha(isFrameAlpha);
}

/* mirrored left to right: the corners trade columns (the shared corners carry the swaps) */
void C2DBitmapPoly::FlipImage_Xaxis()
{
    CopyRow(m_tris[0].m_vertex[0], m_tris[0].m_vertex[1]);
    CopyRow(m_tris[0].m_vertex[1], m_tris[1].m_vertex[0]);
    CopyRow(m_tris[1].m_vertex[0], m_tris[0].m_vertex[0]);
    CopyRow(m_tris[1].m_vertex[1], m_tris[1].m_vertex[2]);
    CopyRow(m_tris[1].m_vertex[2], m_tris[0].m_vertex[2]);
    CopyRow(m_tris[0].m_vertex[2], m_tris[1].m_vertex[1]);
}

/* mirrored top to bottom */
void C2DBitmapPoly::FlipImage_Yaxis()
{
    CopyRow(m_tris[1].m_vertex[0], m_tris[1].m_vertex[2]);
    CopyRow(m_tris[1].m_vertex[2], m_tris[0].m_vertex[0]);
    CopyRow(m_tris[0].m_vertex[0], m_tris[1].m_vertex[0]);
    CopyRow(m_tris[0].m_vertex[2], m_tris[0].m_vertex[1]);
    CopyRow(m_tris[0].m_vertex[1], m_tris[1].m_vertex[1]);
    CopyRow(m_tris[1].m_vertex[1], m_tris[0].m_vertex[2]);
}

/* turned half round */
void C2DBitmapPoly::FlipImage_XYaxis()
{
    CopyRow(m_tris[0].m_vertex[0], m_tris[0].m_vertex[2]);
    CopyRow(m_tris[0].m_vertex[2], m_tris[1].m_vertex[0]);
    CopyRow(m_tris[1].m_vertex[1], m_tris[1].m_vertex[0]);
    CopyRow(m_tris[1].m_vertex[0], m_tris[0].m_vertex[0]);
}

/* the texture's library, once, then the children's.
 * OPEN, 0x108 against 0xC8 (m2c's draft matches, so this stays out of the build): the target calls
 * the library vector's begin, end, find, == and push_back out of line (their copies follow it,
 * 0x358670-0x358728, in reverse of first use; all five C2D GetAssetLibs call these same copies,
 * c2dbitmap_poly.cpp's being linked first), where include/vector's pointer vector inlines all five
 * here. Five calls out of line is the shape of an inline helper inlined one level, which needs it
 * five high. Measured with wrapper ladders (build/scratch/hud/p_vh.cpp): begin and end are 3 high,
 * find 2, push_back 2, == 1, and a call in argument position of an inline call counts a level
 * deeper -- so a helper holding this test is 4 high and inlines whole (begin and end one level
 * inside it, 0x160), one wrapper more and the wrapper is called (0x80); a non-inline template is
 * called, an inline one inlines whole, `inline` on this definition changes nothing (0x104). The
 * helper's fifth level is not found yet */
#if 0
void C2DBitmapPoly::GetAssetLibs(CLibList &liblist)
{
    if (m_texture) {
        zdb::CAssetLib *lib = m_texture->m_assetlib;

        if (lib) {
            CLibList::iterator it = std::find(liblist.begin(), liblist.end(), lib);

            if (it == liblist.end())
                liblist.push_back(lib);
        }
    }
    C2D::GetAssetLibs(liblist);
}
#endif
