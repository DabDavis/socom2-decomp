/* c2dline.cpp -- C2DLine (OVL1 0x35F8F0-0x360A1F): SOCOM 1's functions in SOCOM 1's order
 * (GetAssetLibs, the two clip3DLine, Draw2DLine, Draw3DLine, SetClipInfo, FillClipInfo,
 * MakePacket, Draw, SetUV, SetPos, Load, SetTrans, SetColor, the constructor), with SOCOM II's four
 * flag setters (vtable slots 6-9, keeping the GS PRIM bits) before the constructor. SOCOM 1's
 * symbol table gives the class its own unit; the name is ours. Written, all byte-exact: the
 * constructor (the UVs memcpy'd from a local's initialiser), the setters, Load, SetPos, SetUV, Draw
 * and the clip copies, the three-plane-pair clip3DLine; MakePacket and the one-plane clip3DLine
 * OPEN. Not yet: the line drawers (0x35FCE0-0x36025F) and GetAssetLibs (m2c's draft matches). Defined in reverse of their image order
 * (-inline deferred). */

#include <string.h>
#include <eetypes.h>

#include "ztwod.h"
#include "zvid2d.h"

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

/* untextured, white then grey and opaque, the UVs the texture's diagonal, and a line's PRIM word
 * (1) with the flags' bits (the vertices' small offsets are SOCOM 1's; the depth, zVid's maxZ, SOCOM II's) */
C2DLine::C2DLine()
{
    m_vertex[0][0] = -6.3e-05f;
    m_vertex[0][1] = -4.6e-05f;
    m_vertex[0][2] = zVid.maxZ;
    m_vertex[0][3] = 1.0f;
    m_vertex[1][0] = -3e-05f;
    m_vertex[1][1] = -2e-05f;
    m_vertex[1][2] = zVid.maxZ;
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
    m_vertex[0][2] = zVid.maxZ;
    m_vertex[1][0] = m_topx + x2 * m_fXPixel;
    m_vertex[1][1] = m_topy + y2 * m_fYPixel;
    m_vertex[1][2] = zVid.maxZ;
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

/* the texture's registers, the DMA and VIF tags (seven quadwords, DIRECT), a GIF tag (two loops of
 * ST, RGBAQ, XYZ2 under the PRIM word), then both ends, kicked. The cursor is a vector pointer, not
 * the packet's type: through the conversion MW keeps it apart from the packet (with one type, MW
 * passes the packet's register for the texture's copy); each quadword is its own step, which MW
 * pairs into the target's steps of 32 */
void C2DLine::MakePacket(zdb::CCamera *cam)
{
    u_long128 *packet = zSysSprGetPacket(0);
    sceVu0FVECTOR *p = (sceVu0FVECTOR *)packet;

    if (m_texture) {
        sceVu0CopyVector(*p, m_texture->m_gsregs);
        p++;
    }
    ((unsigned int *)p)[0] = 0x10000007;
    ((unsigned int *)p)[1] = 0;
    ((unsigned int *)p)[2] = 0;
    ((unsigned int *)p)[3] = 0x50000007;
    p++;
    ((unsigned long *)p)[0] = 0x3000400000008002 | ((unsigned long)m_prim << 47);
    ((unsigned long *)p)[1] = 0x512;
    p++;
    sceVu0CopyVector(*p, m_NewUV[0]);
    p++;
    sceVu0FTOI0Vector((int *)p, m_RGB[0]);
    p++;
    sceVu0FTOI4Vector((int *)p, m_vertex[0]);
    p++;
    sceVu0CopyVector(*p, m_NewUV[1]);
    p++;
    sceVu0FTOI0Vector((int *)p, m_RGB[1]);
    p++;
    sceVu0FTOI4Vector((int *)p, m_vertex[1]);
    p++;
    zSysFifoKick(packet, (u_long128 *)p - packet);
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

/* libvu0's subtraction (MAIN, prebuilt) and a point's difference through it, by value as zmath's
 * CPnt4D operators are (the result copied out of its temporary) */
extern "C" void sceVu0SubVector(sceVu0FVECTOR v0, sceVu0FVECTOR v1, sceVu0FVECTOR v2);

static inline CPnt4D operator-(const CPnt4D &a, const CPnt4D &b)
{
    CPnt4D r;

    sceVu0SubVector((float *)&r, (float *)&a, (float *)&b);
    return r;
}

/* a segment clipped against one plane (a point on it and its normal): both ends in front, kept;
 * both behind, rejected; else the end behind is moved onto the plane.
 * OPEN, 24 instructions, all stack slots: the target's six CPnt4D temporaries of the two moves are
 * grouped by operator (both differences' at 96 and 112, both products' at 128 and 144, both sums'
 * at 160 and 176; the copies above them in evaluation order), where named locals take them in
 * reverse of declaration. The one-expression move, `*p1 = *p0 + (*p1 - *p0) * t`, gives the
 * temporaries' order but nests the three by-value operators past the inline depth (CPnt4D's
 * constructors called out of line, a free or a member operator- alike): SOCOM II's operators
 * must be a level shallower than zmath.h's. `t` declared first gives the target's $f20/$f21 */
#pragma auto_inline off
int clip3DLine(CPnt4D *point, CPnt4D *normal, CPnt4D *p0, CPnt4D *p1)
{
    float t;
    CPnt4D v0 = *p0 - *point;
    CPnt4D v1 = *p1 - *point;
    float d0 = sceVu0InnerProduct((float *)&v0, (float *)normal);
    float d1 = sceVu0InnerProduct((float *)&v1, (float *)normal);

    if (d0 > 0.0f && d1 > 0.0f)
        return true;
    if (d0 <= 0.0f && d1 <= 0.0f)
        return false;
    t = d0 / (d0 - d1);
    if (d0 > 0.0f) {
        CPnt4D d = *p1 - *p0;
        CPnt4D s = d * t;
        CPnt4D r = *p0 + s;

        *p1 = r;
    } else {
        CPnt4D d = *p1 - *p0;
        CPnt4D s = d * t;
        CPnt4D r = *p0 + s;

        *p0 = r;
    }
    return true;
}
#pragma auto_inline reset

/* a segment clipped against the view's planes: the second against the first's point, then the
 * other five against the first's (SOCOM 1's, the planes a CPnt4D array). Both clippers return int
 * and the last answer is kept in a local: returned straight, MW lays the last test out the other
 * way round; a bool local adds an andi */
int clip3DLine(CPnt4D *planes, CPnt4D *p0, CPnt4D *p1)
{
    int ok = 0;

    if (!planes)
        return false;
    if (!clip3DLine(&planes[1], &planes[2], p0, p1))
        return false;
    if (!clip3DLine(&planes[0], &planes[3], p0, p1))
        return false;
    if (!clip3DLine(&planes[0], &planes[4], p0, p1))
        return false;
    if (!clip3DLine(&planes[0], &planes[5], p0, p1))
        return false;
    ok = clip3DLine(&planes[0], &planes[6], p0, p1);
    return ok;
}
