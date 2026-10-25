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

#include <eetypes.h>

#include "ztwod.h"
#include "zvid2d.h"

/* libvu0's float-to-fixed copy (MAIN, prebuilt) */
extern "C" void sceVu0FTOI0Vector(int *v0, sceVu0FVECTOR v1);

/* the scratchpad packet and its kick (zsys) */
u_long128 *zSysSprGetPacket(u_long128 **end);
void zSysFifoKick(u_long128 *packet, unsigned int qwc);

/* libgraph's GIF tag (eestruct.h's SCE_GIF_SET_TAG): MW folds the constant fields into one word
 * where every field but the PRIM is constant (C2DLine's), and ORs them in this order otherwise */
#define SCE_GIF_SET_TAG(nloop, eop, pre, prim, flg, nreg)                                       \
    ((unsigned long)(nloop) | ((unsigned long)(eop) << 15) | ((unsigned long)(pre) << 46) |   \
     ((unsigned long)(prim) << 47) | ((unsigned long)(flg) << 58) | ((unsigned long)(nreg) << 60))

/* the texture as the triangle packets it: its GS registers at 0x30 */
namespace zdb {
class CTexture {
public:
    unsigned char _0[0x30];
    sceVu0FVECTOR m_gsregs;   // 0x30
};
}

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

/* the texture's registers, the DMA and VIF tags (sized at the end), the frame-buffer alpha's blend
 * (an A+D write of ALPHA_1) when asked for, a GIF tag (three loops of ST, RGBAQ, XYZ2, the last
 * packet unless the blend is restored after), then the corners: x and y as 12.4 fixed point in the
 * window, y squeezed about the screen's top by the aspect in video mode 1, the 2D depth */
void C2DPoly::MakePacket(zdb::CCamera *cam)
{
    int i;
    u_long128 *packet = zSysSprGetPacket(0);
    sceVu0FVECTOR *p = (sceVu0FVECTOR *)packet;
    sceVu0FVECTOR *tag;
    unsigned int eop;

    if (m_texture) {
        sceVu0CopyVector(*p, m_texture->m_gsregs);
        p++;
    }
    tag = p;
    p++;
    if (m_isFrameAlpha) {
        ((unsigned long *)p)[0] = SCE_GIF_SET_TAG(1, 0, 0, 0, 0, 1);
        ((unsigned long *)p)[1] = 0xe;
        ((unsigned long *)p)[2] = 0x54;
        ((unsigned long *)p)[3] = 0x42;
        eop = 0;
        p += 2;
    } else
        eop = 1;
    ((unsigned long *)p)[0] = SCE_GIF_SET_TAG(3, eop, 1, m_prim, 0, 3);
    ((unsigned long *)p)[1] = 0x512;
    p++;
    for (i = 0; i < 3; i++) {
        sceVu0CopyVector(*p, m_NewUV[i]);
        p++;
        sceVu0FTOI0Vector((int *)p, m_RGB[i]);
        p++;
        ((int *)p)[0] = (int)(16.0f * (m_topx + m_vertex[i][0] * m_fXPixel));
        if (zVid.videoMode == 1)
            ((int *)p)[1] = (int)(16.0f * (zVid.screenTop +
                                           zVid.aspect[1] * (m_vertex[i][1] - zVid.screenTop)));
        else
            ((int *)p)[1] = (int)(16.0f * (m_topy + m_vertex[i][1] * m_fYPixel));
        ((int *)p)[2] = zVid.iMaxZ;
        ((int *)p)[3] = (int)m_vertex[i][3];
        p++;
    }
    if (m_isFrameAlpha) {
        ((unsigned long *)p)[0] = SCE_GIF_SET_TAG(1, 1, 0, 0, 0, 1);
        ((unsigned long *)p)[1] = 0xe;
        ((unsigned long *)p)[2] = 0x44;
        ((unsigned long *)p)[3] = 0x42;
        p += 2;
    }

    int qwc = (u_long128 *)p - (u_long128 *)tag - 1;

    ((unsigned int *)tag)[0] = 0x10000000 | qwc;
    ((unsigned int *)tag)[1] = 0;
    ((unsigned int *)tag)[2] = 0;
    ((unsigned int *)tag)[3] = 0x50000000 | qwc;
    zSysFifoKick(packet, (u_long128 *)p - packet);
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
