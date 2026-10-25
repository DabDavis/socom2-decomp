/* c2dpoly.cpp -- C2DTriFan and C2DPoly (OVL1 0x360A20-0x361AFF): SOCOM 1's functions in SOCOM 1's
 * order (the tri-fan's GetAssetLibs, Draw, destructor and constructor; the triangle's MakePacket,
 * Draw, SetPos, SetUV, Load, GetTrans, SetTrans, GetColor, SetColor, constructor and GetAssetLibs;
 * then the triangle vector's template instances), with SOCOM II's own among them: a vertex's alpha
 * after GetTrans, and the four flag setters that keep the GS PRIM bits (vtable slots 6-9) before
 * the constructor. SOCOM 1's symbol table groups the two classes' RTTI in one unit; its name is
 * ours. Written, all byte-exact: C2DPoly's but MakePacket and GetAssetLibs (m2c's draft matches; it
 * calls the library vector's pieces out of line, as C2DBitmapPoly's), and the tri-fan's
 * constructor; the tri-fan's Draw and destructor OPEN on include/vector. Defined in reverse of
 * their image order (-inline deferred). */

#include "ztwod.h"

/* the triangle at 0,0, untextured, white, blended, then grey and opaque, a triangle's PRIM word (3)
 * with the flags' bits (the vertices' small offsets are SOCOM 1's) */
C2DPoly::C2DPoly()
{
    m_vertex[0][0] = -6.3e-05f;
    m_vertex[0][1] = -4.6e-05f;
    m_vertex[0][2] = 0.0001f;
    m_vertex[0][3] = 1.0f;
    m_vertex[1][0] = -3e-05f;
    m_vertex[1][1] = -2e-05f;
    m_vertex[1][2] = 0.0001f;
    m_vertex[1][3] = 1.0f;
    m_vertex[2][0] = -3e-05f;
    m_vertex[2][1] = -2e-05f;
    m_vertex[2][2] = 0.0001f;
    m_vertex[2][3] = 1.0f;
    m_x = 0;
    m_y = 0;
    m_NewUV[0][0] = 0.0f;
    m_NewUV[0][1] = 0.0f;
    m_NewUV[0][2] = 1.0f;
    m_NewUV[0][3] = 0.0f;
    m_NewUV[1][0] = 1.0f;
    m_NewUV[1][1] = 1.0f;
    m_NewUV[1][2] = 1.0f;
    m_NewUV[1][3] = 0.0f;
    m_NewUV[2][0] = 1.0f;
    m_NewUV[2][1] = 1.0f;
    m_NewUV[2][2] = 1.0f;
    m_NewUV[2][3] = 0.0f;
    m_RGB[0][0] = 255.0f;
    m_RGB[0][1] = 255.0f;
    m_RGB[0][2] = 255.0f;
    m_RGB[0][3] = 100.0f;
    m_RGB[1][0] = 255.0f;
    m_RGB[1][1] = 255.0f;
    m_RGB[1][2] = 255.0f;
    m_RGB[1][3] = 100.0f;
    m_RGB[2][0] = 255.0f;
    m_RGB[2][1] = 255.0f;
    m_RGB[2][2] = 255.0f;
    m_RGB[2][3] = 100.0f;
    m_texture = 0;
    C2DPoly::SetIsTrans(true);
    SetColor(128.0f, 128.0f, 128.0f);
    SetTrans(128.0f);
    MakePrim(m_prim, 3);
}

/* slot 6: with a texture to draw, TME */
void C2DPoly::SetHasTexture(bool on)
{
    m_hasTexture = on;
    if (on && m_texture)
        m_prim |= 0x10;
    else if (m_prim & 0x10)
        m_prim ^= 0x10;
}

/* slot 7: AA1 */
void C2DPoly::SetAA(bool on)
{
    m_isAA = on;
    if (on)
        m_prim |= 0x80;
    else if (m_prim & 0x80)
        m_prim ^= 0x80;
}

/* slot 8: ABE */
void C2DPoly::SetIsTrans(bool on)
{
    m_isTrans = on;
    if (on)
        m_prim |= 0x40;
    else if (m_prim & 0x40)
        m_prim ^= 0x40;
}

/* slot 9: IIP */
void C2DPoly::SetGouraud(bool on)
{
    m_socom2_bit5 = on;
    if (on)
        m_prim |= 0x8;
    else if (m_prim & 0x8)
        m_prim ^= 0x8;
}

void C2DPoly::SetColor(float r, float g, float b)
{
    m_RGB[0][0] = r;
    m_RGB[0][1] = g;
    m_RGB[0][2] = b;
    m_RGB[1][0] = r;
    m_RGB[1][1] = g;
    m_RGB[1][2] = b;
    m_RGB[2][0] = r;
    m_RGB[2][1] = g;
    m_RGB[2][2] = b;
}

CPnt3D C2DPoly::GetColor()
{
    CPnt3D color(m_RGB[0][0], m_RGB[0][1], m_RGB[0][2]);

    return color;
}

/* slot 11: every vertex's alpha; opaque (128) drops the blending, anything else keeps it */
void C2DPoly::SetTrans(float trans)
{
    m_RGB[0][3] = trans;
    m_RGB[1][3] = trans;
    m_RGB[2][3] = trans;
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

void C2DPoly::SetTrans(unsigned int vertex, float trans)
{
    m_RGB[vertex < 3 ? vertex : 2][3] = trans;
    if (!m_isTrans) {
        m_isTrans = true;
        m_prim |= 0x40;
    }
    if (!m_socom2_bit5) {
        m_socom2_bit5 = true;
        m_prim |= 0x8;
    }
}

float C2DPoly::GetTrans()
{
    return m_RGB[0][3];
}

/* placed only with a texture */
void C2DPoly::Load(float x0, float y0, float x1, float y1, float x2, float y2, zdb::CTexture *tex)
{
    m_texture = tex;
    if (m_texture)
        SetPos(x0, y0, x1, y1, x2, y2);
}

/* v counts down the texture */
void C2DPoly::SetUV(float u0, float v0, float u1, float v1, float u2, float v2)
{
    m_NewUV[0][0] = u0;
    m_NewUV[0][1] = -v0;
    m_NewUV[1][0] = u1;
    m_NewUV[1][1] = -v1;
    m_NewUV[2][0] = u2;
    m_NewUV[2][1] = -v2;
}

void C2DPoly::SetPos(float x0, float y0, float x1, float y1, float x2, float y2)
{
    m_x = (int)x0;
    m_y = (int)y0;
    m_vertex[0][0] = x0;
    m_vertex[0][1] = y0;
    m_vertex[1][0] = x1;
    m_vertex[1][1] = y1;
    m_vertex[2][0] = x2;
    m_vertex[2][1] = y2;
}

/* while on: packeted, then the children */
void C2DPoly::Draw(zdb::CCamera *cam)
{
    if (IsOn() != false) {
        MakePacket(cam);
        C2D::Draw(cam);
    }
}

/* room for five triangles */
C2DTriFan::C2DTriFan()
{
    m_polylist.reserve(5);
}

/* the triangles cleared, then the vector and C2D torn down.
 * OPEN, 2 instructions short (0x88 against 0x90): the target calls the triangle vector's clear and
 * its deleter's destructor out of line (0x3619A0, 0x361A30) behind two this tests, where
 * include/vector's class vector inlines a level differently -- a vector of a class's destructor
 * depth, include/vector's (the integrator's this round) */
C2DTriFan::~C2DTriFan()
{
    m_polylist.clear();
}

/* while on, every triangle.
 * OPEN, 2 instructions short (0xC0 against 0xC8): MW folds the walk's first test, begin against
 * begin + size * 0xD0, into size == 0 (begin() one level, as include/vector's class vector has
 * it); the target keeps the pointer sum and compares -- a taller begin(), as the pointer vectors'
 * (README: "A one-level begin() folded a walk's first test"). Assigned, while, postfix, !(==), a
 * reference local and raw pointers all miss */
void C2DTriFan::Draw(zdb::CCamera *cam)
{
    if (IsOn()) {
        for (std::vector<C2DPoly>::iterator i = m_polylist.begin(); i != m_polylist.end(); ++i)
            (*i).Draw(cam);
    }
}
