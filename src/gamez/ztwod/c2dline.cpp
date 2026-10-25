/* c2dline.cpp -- C2DLine (OVL1 0x35F8F0-0x360A1F): SOCOM 1's functions in SOCOM 1's order
 * (GetAssetLibs, the two clip3DLine, Draw2DLine, Draw3DLine, SetClipInfo, FillClipInfo,
 * MakePacket, Draw, SetUV, SetPos, Load, SetTrans, SetColor, the constructor), with SOCOM II's four
 * flag setters (vtable slots 6-9, keeping the GS PRIM bits) before the constructor. SOCOM 1's
 * symbol table gives the class its own unit; the name is ours. Written, all byte-exact: the
 * constructor (the UVs memcpy'd from a local's initialiser), the setters, Load, SetPos, SetUV, Draw
 * and the clip copies; MakePacket OPEN. Not yet: the clippers and line drawers (0x35F9C0-0x36025F)
 * and GetAssetLibs (m2c's draft matches). Defined in reverse of their image order
 * (-inline deferred). */

#include <string.h>
#include <eetypes.h>

#include "ztwod.h"

/* libvu0's float-to-fixed copies (MAIN, prebuilt) */
typedef int sceVu0IVECTOR[4] __attribute__((aligned(16)));
extern "C" void sceVu0FTOI0Vector(sceVu0IVECTOR v0, sceVu0FVECTOR v1);
extern "C" void sceVu0FTOI4Vector(sceVu0IVECTOR v0, sceVu0FVECTOR v1);

/* the scratchpad packet and its kick (zsys) */
u_long128 *zSysSprGetPacket(u_long128 **end);
void zSysFifoKick(u_long128 *packet, unsigned int qwc);

/* the texture as the line packets it: its GS registers at 0x30 */
namespace zdb {
class CTexture {
public:
    unsigned char _0[0x30];
    sceVu0FVECTOR m_gsregs;   // 0x30
};
}

/* the 2D depth the video setup leaves (OVL1 bss, inside zVid: SOCOM II's; no row of its own) */
extern float data_004A44E0;

/* untextured, white then grey and opaque, the UVs the texture's diagonal, and a line's PRIM word
 * (1) with the flags' bits (the vertices' small offsets are SOCOM 1's; the depth SOCOM II's) */
C2DLine::C2DLine()
{
    m_vertex[0][0] = -6.3e-05f;
    m_vertex[0][1] = -4.6e-05f;
    m_vertex[0][2] = data_004A44E0;
    m_vertex[0][3] = 1.0f;
    m_vertex[1][0] = -3e-05f;
    m_vertex[1][1] = -2e-05f;
    m_vertex[1][2] = data_004A44E0;
    m_vertex[1][3] = 1.0f;
    m_x = 0;
    m_y = 0;
    do3D = false;
    m_RGB[0][0] = 255.0f;
    m_RGB[0][1] = 255.0f;
    m_RGB[0][2] = 255.0f;
    m_RGB[0][3] = 100.0f;
    m_RGB[1][0] = 255.0f;
    m_RGB[1][1] = 255.0f;
    m_RGB[1][2] = 255.0f;
    m_RGB[1][3] = 100.0f;
    m_texture = 0;

    float uv[2][4] = {{0.0f, 1.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 1.0f, 0.0f}};

    memcpy(m_NewUV, uv, sizeof(uv));
    SetColor(128.0f, 128.0f, 128.0f);
    SetTrans(128.0f);
    MakePrim(m_prim, 1);
}

/* slot 6: with a texture to draw, TME */
void C2DLine::SetHasTexture(bool on)
{
    m_hasTexture = on;
    if (on && m_texture)
        m_prim |= 0x10;
    else if (m_prim & 0x10)
        m_prim ^= 0x10;
}

/* slot 7: AA1 */
void C2DLine::SetAA(bool on)
{
    m_isAA = on;
    if (on)
        m_prim |= 0x80;
    else if (m_prim & 0x80)
        m_prim ^= 0x80;
}

/* slot 8: ABE */
void C2DLine::SetIsTrans(bool on)
{
    m_isTrans = on;
    if (on)
        m_prim |= 0x40;
    else if (m_prim & 0x40)
        m_prim ^= 0x40;
}

/* slot 9: IIP */
void C2DLine::SetGouraud(bool on)
{
    m_socom2_bit5 = on;
    if (on)
        m_prim |= 0x8;
    else if (m_prim & 0x8)
        m_prim ^= 0x8;
}

void C2DLine::SetColor(float r, float g, float b)
{
    m_RGB[0][0] = r;
    m_RGB[0][1] = g;
    m_RGB[0][2] = b;
    m_RGB[1][0] = r;
    m_RGB[1][1] = g;
    m_RGB[1][2] = b;
}

/* slot 11: both ends' alpha; opaque (128) drops the blending, anything else keeps it (the flag read
 * once, unlike C2DBitmap's) */
void C2DLine::SetTrans(float trans)
{
    m_RGB[0][3] = trans;
    m_RGB[1][3] = trans;

    unsigned char trans_on = m_isTrans;

    if (trans_on && trans == 128.0f) {
        m_isTrans = false;
        if (m_prim & 0x40)
            m_prim ^= 0x40;
    } else if (!trans_on) {
        m_isTrans = true;
        m_prim |= 0x40;
    }
}

/* the texture, then (with one) placed: without one the first-draw flag drops */
void C2DLine::Load(float x1, float y1, float x2, float y2, zdb::CTexture *tex)
{
    m_texture = tex;
    if (!m_texture) {
        if (first)
            first = false;
        return;
    }
    SetPos((int)x1, (int)y1, (int)x2, (int)y2);
}

/* the ends on the screen: scaled by the pixel sizes, moved by the window's corner, at the 2D depth */
void C2DLine::SetPos(int x1, int y1, int x2, int y2)
{
    m_x = x1;
    m_y = y1;
    m_vertex[0][0] = m_topx + x1 * m_fXPixel;
    m_vertex[0][1] = m_topy + y1 * m_fYPixel;
    m_vertex[0][2] = data_004A44E0;
    m_vertex[1][0] = m_topx + x2 * m_fXPixel;
    m_vertex[1][1] = m_topy + y2 * m_fYPixel;
    m_vertex[1][2] = data_004A44E0;
}

/* v counts down the texture */
void C2DLine::SetUV(float u1, float v1, float u2, float v2)
{
    m_NewUV[0][0] = u1;
    m_NewUV[0][1] = -v1;
    m_NewUV[1][0] = u2;
    m_NewUV[1][1] = -v2;
}

/* while on and textured: packeted, then the children */
void C2DLine::Draw(zdb::CCamera *cam)
{
    if (IsOn() != false && m_texture) {
        MakePacket(cam);
        C2D::Draw(cam);
    }
}

/* the texture's registers, a GIF tag (two loops of ST, RGBAQ, XYZ2 under the PRIM word), then
 * both ends, kicked.
 * OPEN, 3 instructions short (0x10C against 0x118): MW folds the packet pointer's steps into
 * constant offsets from one register (a0 = s0 + 48, + 64, ...); the target advances s0 by 32 after
 * each pair of quadwords, as C2DBitmap::MakePacket does (0x3643B0) -- some form of the steps MW
 * cannot fold (&p[n], p = p + 2, a pair struct, a by-reference inline all fold or grow). A p++
 * statement after each call gets the stepping by 32, but paired one call off (the tag's step
 * merged into the first pair's) and the last step folded into the kick's size */
void C2DLine::MakePacket(zdb::CCamera *cam)
{
    u_long128 *packet = zSysSprGetPacket(0);
    u_long128 *p = packet;

    if (m_texture) {
        sceVu0CopyVector((float *)p, m_texture->m_gsregs);
        p++;
    }
    ((unsigned int *)p)[0] = 0x10000007;
    ((unsigned int *)p)[1] = 0;
    ((unsigned int *)p)[2] = 0;
    ((unsigned int *)p)[3] = 0x50000007;
    ((unsigned long *)p)[2] = 0x3000400000008002 | ((unsigned long)m_prim << 47);
    ((unsigned long *)p)[3] = 0x512;
    p += 2;
    sceVu0CopyVector((float *)p, m_NewUV[0]);
    sceVu0FTOI0Vector((int *)(p + 1), m_RGB[0]);
    p += 2;
    sceVu0FTOI4Vector((int *)p, m_vertex[0]);
    sceVu0CopyVector((float *)(p + 1), m_NewUV[1]);
    p += 2;
    sceVu0FTOI0Vector((int *)p, m_RGB[1]);
    sceVu0FTOI4Vector((int *)(p + 1), m_vertex[1]);
    p += 2;
    zSysFifoKick(packet, p - packet);
}

void C2DLine::FillClipInfo(float (*vertex)[4], float (*rgb)[4], float (*uv)[4])
{
    for (int i = 0; i < 2; i++) {
        vertex[i][0] = m_vertex[i][0];
        vertex[i][1] = m_vertex[i][1];
        vertex[i][2] = m_vertex[i][2];
        vertex[i][3] = m_vertex[i][3];
        rgb[i][0] = m_RGB[i][0];
        rgb[i][1] = m_RGB[i][1];
        rgb[i][2] = m_RGB[i][2];
        rgb[i][3] = m_RGB[i][3];
        uv[i][0] = m_NewUV[i][0];
        uv[i][1] = m_NewUV[i][1];
        uv[i][2] = m_NewUV[i][2];
        uv[i][3] = m_NewUV[i][3];
    }
}

void C2DLine::SetClipInfo(float (*vertex)[4], float (*rgb)[4], float (*uv)[4])
{
    for (int i = 0; i < 2; i++) {
        m_vertex[i][0] = vertex[i][0];
        m_vertex[i][1] = vertex[i][1];
        m_vertex[i][2] = vertex[i][2];
        m_vertex[i][3] = vertex[i][3];
        m_RGB[i][0] = rgb[i][0];
        m_RGB[i][1] = rgb[i][1];
        m_RGB[i][2] = rgb[i][2];
        m_RGB[i][3] = rgb[i][3];
        m_NewUV[i][0] = uv[i][0];
        m_NewUV[i][1] = uv[i][1];
        m_NewUV[i][2] = uv[i][2];
        m_NewUV[i][3] = uv[i][3];
    }
}
