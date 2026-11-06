/* c2dbitmap_poly.cpp -- C2DBitmapPoly, a bitmap drawn as two C2DPolys (OVL1 0x357E90-0x35973F):
 * SOCOM 1's unit (its name from a local class's mangling, C2DWidePolyline's CPnt2D in
 * c2dbitmap_poly_cpp), its functions in SOCOM 1's order -- C2DWidePolyline::SetSegments,
 * GetAssetLibs, the two Rotate_Zaxis (on the unit's static r, 0x3E0300), the three flips, SetUseFrameBufferAlpha, GetTrans, SetTrans,
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

/* the rotation both Rotate_Zaxis write and turn the corners by (SOCOM 1's static `r`, 0x43A860;
 * the identity until the first turn) */
static float r[2][2] = { { 1.0f, 0.0f }, { 0.0f, 1.0f } };

/* half the difference added to the larger (the midpoint when p is the smaller; the target's code
 * either way). By reference: the second is read first and numbered below the first, as the target
 * holds them (by value, MW reads the arguments last first and numbers them in order) */
static inline float Mid(const float &p, const float &q)
{
    float d = p - q;

    d *= 0.5f;
    if (p > q)
        d += p;
    else
        d += q;
    return d;
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

/* turned by the angle (degrees) about the centre of the corners' extent (SOCOM 1's, 0x2E4 there):
 * the four distinct corners moved to the origin, r set and applied, moved back, and the second
 * triangle's two shared corners copied. The turns are written out on the function's own x, y, nx
 * and ny (y declared first): an inline turn per corner computes y' before x' whatever its order,
 * where the target computes x' first for the first two corners; the cosine stored to r[1][1] and
 * then copied to r[0][0] (a chained store reloads r[1][1] for the first corner's y'), and the
 * radians left a temporary (a named local is numbered below the centres, the inline's variables,
 * and takes $f22 where the target's $f20) */
void C2DBitmapPoly::Rotate_Zaxis(float s)
{
    float (*a)[4] = m_tris[0].m_vertex;
    float (*b)[4] = m_tris[1].m_vertex;
    float cx = Mid(m_tris[0].m_vertex[1][0], m_tris[0].m_vertex[2][0]);
    float cy = Mid(a[0][1], a[1][1]);
    float y, x, nx, ny;

    a[0][0] -= cx;
    a[0][1] -= cy;
    a[1][0] -= cx;
    a[1][1] -= cy;
    a[2][0] -= cx;
    a[2][1] -= cy;
    b[2][0] -= cx;
    b[2][1] -= cy;
    r[1][0] = sinf(0.017453292f * s);
    r[1][1] = cosf(0.017453292f * s);
    r[0][0] = r[1][1];
    r[0][1] = -r[1][0];
    y = a[0][1];
    x = a[0][0];
    nx = x * r[0][0] + y * r[0][1];
    ny = x * r[1][0] + y * r[1][1];
    a[0][0] = nx;
    a[0][1] = ny;
    y = a[1][1];
    x = a[1][0];
    nx = x * r[0][0] + y * r[0][1];
    ny = x * r[1][0] + y * r[1][1];
    a[1][0] = nx;
    a[1][1] = ny;
    y = a[2][1];
    x = a[2][0];
    nx = x * r[0][0] + y * r[0][1];
    ny = x * r[1][0] + y * r[1][1];
    a[2][0] = nx;
    a[2][1] = ny;
    y = b[2][1];
    x = b[2][0];
    nx = x * r[0][0] + y * r[0][1];
    ny = x * r[1][0] + y * r[1][1];
    b[2][0] = nx;
    b[2][1] = ny;
    a[0][0] += cx;
    a[0][1] += cy;
    a[1][0] += cx;
    a[1][1] += cy;
    a[2][0] += cx;
    a[2][1] += cy;
    b[2][0] += cx;
    b[2][1] += cy;
    CopyRow(b[0], a[0]);
    CopyRow(b[1], a[2]);
}

/* turned by a sine and cosine about the same centre (SOCOM 1's, 0x294 there) */
void C2DBitmapPoly::Rotate_Zaxis(float s, float c)
{
    float (*a)[4] = m_tris[0].m_vertex;
    float (*b)[4] = m_tris[1].m_vertex;
    float cx = Mid(m_tris[0].m_vertex[1][0], m_tris[0].m_vertex[2][0]);
    float cy = Mid(a[0][1], a[1][1]);
    float y, x, nx, ny;

    a[0][0] -= cx;
    a[0][1] -= cy;
    a[1][0] -= cx;
    a[1][1] -= cy;
    a[2][0] -= cx;
    a[2][1] -= cy;
    b[2][0] -= cx;
    b[2][1] -= cy;
    r[1][0] = s;
    r[0][1] = -s;
    r[0][0] = r[1][1] = c;
    y = a[0][1];
    x = a[0][0];
    nx = x * r[0][0] + y * r[0][1];
    ny = x * r[1][0] + y * r[1][1];
    a[0][0] = nx;
    a[0][1] = ny;
    y = a[1][1];
    x = a[1][0];
    nx = x * r[0][0] + y * r[0][1];
    ny = x * r[1][0] + y * r[1][1];
    a[1][0] = nx;
    a[1][1] = ny;
    y = a[2][1];
    x = a[2][0];
    nx = x * r[0][0] + y * r[0][1];
    ny = x * r[1][0] + y * r[1][1];
    a[2][0] = nx;
    a[2][1] = ny;
    y = b[2][1];
    x = b[2][0];
    nx = x * r[0][0] + y * r[0][1];
    ny = x * r[1][0] + y * r[1][1];
    b[2][0] = nx;
    b[2][1] = ny;
    a[0][0] += cx;
    a[0][1] += cy;
    a[1][0] += cx;
    a[1][1] += cy;
    a[2][0] += cx;
    a[2][1] += cy;
    b[2][0] += cx;
    b[2][1] += cy;
    CopyRow(b[0], a[0]);
    CopyRow(b[1], a[2]);
}

/* the texture's library, once, then the children's: the test through ztwod.h's AddAssetLib (inlined
 * one level: the library vector's begin, end, find, == and push_back called out of line, their copies
 * following, 0x358670-0x358728 in reverse of first use -- all five C2D GetAssetLibs call these, this
 * unit's being linked first), the walk C2D's inline (the HUD's round 46: its end node taken after
 * the first node, in this's register) */
void C2DBitmapPoly::GetAssetLibs(CLibList &liblist)
{
    if (m_texture) {
        zdb::CAssetLib *lib;

        if ((lib = m_texture->m_assetlib) != 0)
            AddAssetLib(liblist, lib);
    }
    C2D::GetAssetLibs(liblist);
}

/* a point walk (SOCOM 1's CPtItr: hud_linemap.cpp's line walk derives from it): the step, the point,
 * whether it is still on the line */
class CPtItr {
public:
    virtual void Advance() = 0;
    virtual PNT2D GetPt() const = 0;
    virtual bool IsValid() const = 0;
};

namespace {

/* a screen point (SOCOM 1's CPnt2D, local to the unit) */
class CPnt2D : public PNT2D {
public:
    CPnt2D() {}
    CPnt2D(const PNT2D &p) : PNT2D(p) {}
    CPnt2D(float ax, float ay)
    {
        x = ax;
        y = ay;
    }
    CPnt2D &operator=(const CPnt2D &o)
    {
        x = o.x;
        y = o.y;
        return *this;
    }
    CPnt2D operator+(const CPnt2D &b) const { return CPnt2D(x + b.x, y + b.y); }
    CPnt2D operator-(const CPnt2D &b) const { return CPnt2D(x - b.x, y - b.y); }
    CPnt2D operator*(float k) const { return CPnt2D(x * k, y * k); }
};

/* a line's segment and its side offset */
class CSegment {
public:
    CSegment(const CPnt2D &a, const CPnt2D &b) : m_a(a), m_b(b) {}

    CPnt2D m_a;
    CPnt2D m_b;
    CPnt2D m_side;
};

}

/* the wide line: a bitmap polygon for each stretch of the point walk (SOCOM 1's SetSegments), its
 * corners the line's points offset to either side, mitred where two segments meet */
void C2DWidePolyline::SetSegments(CPtItr *itr, float width)
{
    if (!itr->IsValid())
        return;
    CPnt2D ptPrev(itr->GetPt());

    itr->Advance();
    if (!itr->IsValid())
        return;
    const CPnt2D &t = itr->GetPt();
    CPnt2D ptCur(t);
    float halfWidth = 0.5f * width;
    float halfSq = 0.25f * (width * width);
    CSegment seg(ptPrev, t);

    {
        float dx = ptCur.x - ptPrev.x;
        float dy = ptCur.y - ptPrev.y;
        float k = halfWidth / sqrtf(dx * dx + dy * dy);

        seg.m_side.x = dy * k;
        seg.m_side.y = dx * -k;
    }
    CPnt2D ptL = seg.m_a + seg.m_side;
    CPnt2D ptR = seg.m_a - seg.m_side;
    CPnt2D ptNextL, ptNextR;

    for (;;) {
        itr->Advance();
        if (!itr->IsValid()) {
            ptNextL = seg.m_b + seg.m_side;
            ptNextR = seg.m_b - seg.m_side;
        } else {
            const CPnt2D &t2 = itr->GetPt();
            CPnt2D ptNext(t2);
            CSegment next(ptCur, t2);
            {
                float dx = ptNext.x - ptCur.x;
                float dy = ptNext.y - ptCur.y;
                float k = halfWidth / sqrtf(dx * dx + dy * dy);

                next.m_side.x = dy * k;
                next.m_side.y = dx * -k;
            }
            CPnt2D mid = ((next.m_a + next.m_side) + (seg.m_b + seg.m_side)) * 0.5f;
            CPnt2D d = mid - ptCur;
            float k = halfSq / (d.x * d.x + d.y * d.y);

            ptNextL = ptCur + d * k;
            ptNextR = ptCur - d * k;
            ptCur = ptNext;
            seg = next;
        }
        C2DBitmapPoly *poly = new C2DBitmapPoly;

        poly->SetHasTexture(false);
        push_back(poly);
        SetCorners(poly, ptNextL, ptL, ptR, ptNextR);
        if (!itr->IsValid())
            return;
        ptL = ptNextL;
        ptR = ptNextR;
    }
}
