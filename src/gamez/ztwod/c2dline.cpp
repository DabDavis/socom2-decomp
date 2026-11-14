/* c2dline.cpp -- C2DLine (OVL1 0x35F8F0-0x360A1F): SOCOM 1's functions in SOCOM 1's order
 * (GetAssetLibs, the two clip3DLine, Draw2DLine, Draw3DLine, SetClipInfo, FillClipInfo,
 * MakePacket, Draw, SetUV, SetPos, Load, SetTrans, SetColor, the constructor), with SOCOM II's four
 * flag setters (vtable slots 6-9, keeping the GS PRIM bits) before the constructor. SOCOM 1's
 * symbol table gives the class its own unit; the name is ours. Written, all byte-exact: the
 * constructor (the UVs memcpy'd from a local's initialiser), the setters, Load, SetPos, SetUV, Draw
 * and the clip copies, the three-plane-pair clip3DLine; MakePacket and the one-plane clip3DLine
 * OPEN; Draw3DLine (SOCOM II's own body: the HUD's round 40); GetAssetLibs (the HUD's round 46);
 * Draw2DLine 0x35FCE0 (written in round 53, byte-exact since round 58). Defined in reverse of their
 * image order (-inline deferred). */

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

/* the texture as the line packets it: its GS registers at 0x30, the library it came from at 0x10
 * (GetAssetLibs) */
namespace zdb {
class CTexture {
public:
    unsigned char _0[0x10];
    CAssetLib *m_assetlib;   // 0x10
    unsigned char _14[0x30 - 0x14];
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

void C2DLine::SetClipInfo(float (*vertex)[4], float (*rgb)[4], float (*uv)[4], int count)
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

/* the world's camera as the 3D lines reach it (znode.h's CWorld::m_world and m_camera at 0x128, its
 * matrix set's VU block at 0x3F0 -- zcamera.h's -- and the block's world-to-screen matrix at 0x20) */
namespace zdb {
class CFrameSetup {
public:
    unsigned char _0[0x20];
    sceVu0FMATRIX mtxWorldToScreen;   // 0x20
};

class CCamera {
public:
    unsigned char _0[Z_OFS(0x3f0, 0x400)];   /* the host's: zcamera.h's m_mtxSet at 0x300 there */
    CFrameSetup *m_vu;   // 0x3f0 (m_mtxSet.m_vu)
};

class CWorld {
public:
    unsigned char _0[Z_OFS(0x128, 0x134)];   /* the host's: CNode's vptr first, its 16-aligned base after */
    CCamera *m_camera;   // 0x128 (host 0x134)
    static CWorld *m_world;
};
}

/* libvu0's product, element by element, and its perspective transform (MAIN, prebuilt) */
extern "C" void sceVu0MulVector(sceVu0FVECTOR v0, sceVu0FVECTOR v1, sceVu0FVECTOR v2);
extern "C" void sceVu0RotTransPers(sceVu0IVECTOR v0, sceVu0FMATRIX m0, sceVu0FVECTOR v1, int mode);

/* the depth the 3D lines are drawn nearer by (SOCOM 1's static zOffset, 0x4753C8: 2) */
static int zOffset = 2;

/* a line in the world in one packet of its own: the GS's depth test set (TEST_1 through A+D), then
 * the two ends' colours (0..1 scaled to the GS's 255, the alpha's 128) and their points through the
 * camera's world-to-screen matrix, their depths offset by zOffset (SOCOM II's: SOCOM 1 scaled the
 * colours inline and chose the blend by the alphas). The target's three packet registers come from
 * one step per quadword on a cursor of another type than the packet, the DMA tag a third pointer
 * declared first and assigned from the cursor (initialised from the packet it is merged with it),
 * and each end's point pointer a block local (one function-level pointer for both takes s1, the
 * matrix s0: the target's the other way round) */
void Draw3DLine(CPnt4D *p0, CPnt4D *p1, CPnt4D *c0, CPnt4D *c1)
{
    static CPnt4D s_scale(255.0f, 255.0f, 255.0f, 128.0f);
    unsigned int *tag;
    u_long128 *packet = zSysSprGetPacket(0);
    sceVu0FVECTOR *p = (sceVu0FVECTOR *)packet;

    tag = (unsigned int *)p;
    sceVu0FVECTOR *m = zdb::CWorld::m_world->m_camera->m_vu->mtxWorldToScreen;

    p++;
    ((unsigned long *)p)[0] = 0x1024C00000008001;
    ((unsigned long *)p)[1] = 0xE;
    p++;
    ((unsigned long *)p)[0] = 0x5000C;
    ((unsigned long *)p)[1] = 0x47;
    p++;
    ((unsigned long *)p)[0] = 0x2000000000008002;
    ((unsigned long *)p)[1] = 0x41;
    p++;
    sceVu0MulVector((float *)p, (float *)c0, (float *)&s_scale);
    sceVu0FTOI0Vector((int *)p, (float *)p);
    {
        p++;
        int *xyz = (int *)p;

        p0->w = 1.0f;
        sceVu0RotTransPers(xyz, m, (float *)p0, 0);
        xyz[2] += zOffset;
        p++;
    }
    sceVu0MulVector((float *)p, (float *)c1, (float *)&s_scale);
    sceVu0FTOI0Vector((int *)p, (float *)p);
    {
        p++;
        int *xyz = (int *)p;

        p1->w = 1.0f;
        sceVu0RotTransPers(xyz, m, (float *)p1, 0);
        xyz[2] += zOffset;
        p++;
    }

    int qwc = p - (sceVu0FVECTOR *)tag - 1;

    tag[0] = qwc | 0x10000000;
    tag[1] = 0;
    tag[2] = 0x11000000;
    tag[3] = qwc | 0x50000000;
    zSysFifoKick(packet, (u_long128 *)p - packet);
}

/* a line on the screen in one packet of its own (SOCOM 1's Draw2DLine, grown): the two points
 * copied, their y scaled by the pixel aspect (SOCOM II's), fixed on VU0 (x and y 12.4, w whole),
 * their depth the far one and their q zero, moved from the screen's centre to the GS's (SOCOM 1's
 * 2048 + 320 / 224, SOCOM II's the render size's half); then a GIF tag of two loops of RGBAQ and
 * XYZ2 under a gouraud line's PRIM, blended when either end's alpha is not 1, each end's colour
 * scaled to the GS's (as SOCOM 1's, inline) before its point. The VU0 block's four pointers
 * declared fixed-first (the target's a1/a0 for the fixed points, v1/v0 for the copies); the
 * offsets added negated (`+= -(...)`: negu/addu, where `-=` gives subu).
 * The DMA tag's first word a named local declared ahead of the count (round 58, open since round 53:
 * igreplay put the first word's OR just below the count, 43.5 -- as a temporary it was numbered after
 * it and took a1, the count a2, the target the other way round) */
void Draw2DLine(CPnt4D *p0, CPnt4D *p1, CPnt4D *c0, CPnt4D *c1)
{
    unsigned int *tag;
    u_long128 *packet = zSysSprGetPacket(0);
    sceVu0FVECTOR *p = (sceVu0FVECTOR *)packet;
    CPnt4D a = *p0;
    CPnt4D b = *p1;
    sceVu0IVECTOR ia;
    sceVu0IVECTOR ib;

    if (C2D::m_fYPixel != 1.0f) {
        a.y *= C2D::m_fYPixel;
        b.y *= C2D::m_fYPixel;
    }
    {
        int *pa = ia;
        int *pb = ib;
        float *fa = (float *)&a;
        float *fb = (float *)&b;

#if Z_PORT
        /* vftoi4.xy (12.4 fixed point) and vftoi0.w of each end; the z lane is a stale word the asm
         * stores and the code below overwrites (left as it was here) */
        pa[0] = z_vu0_ftoi(fa[0], 4);
        pa[1] = z_vu0_ftoi(fa[1], 4);
        pa[3] = z_vu0_ftoi(fa[3], 0);
        pb[0] = z_vu0_ftoi(fb[0], 4);
        pb[1] = z_vu0_ftoi(fb[1], 4);
        pb[3] = z_vu0_ftoi(fb[3], 0);
#else
        asm volatile {
            lqc2 vf3, 0(fa)
            vftoi4.xy vf4, vf3
            vftoi0.w vf4, vf3
            lqc2 vf5, 0(fb)
            vftoi4.xy vf6, vf5
            vftoi0.w vf6, vf5
            sqc2 vf4, 0(pa)
            sqc2 vf6, 0(pb)
        }
#endif
    }
    ia[2] = 1000000000;
    ib[2] = 1000000000;
    ia[3] = 0;
    ib[3] = 0;
    tag = (unsigned int *)p;
    p++;
    ia[0] += -(((zVid.renderWidth >> 1) + 2048) << 4);
    ia[1] += -(((zVid.renderHeight >> 1) + 2048) << 4);
    ib[0] += -(((zVid.renderWidth >> 1) + 2048) << 4);
    ib[1] += -(((zVid.renderHeight >> 1) + 2048) << 4);

    unsigned int prim = 9;

    if (c0->w != 1.0f || c1->w != 1.0f)
        prim |= 0x40;
    ((unsigned long *)p)[0] = 0x2000400000008002 | ((unsigned long)prim << 47);
    ((unsigned long *)p)[1] = 0x41;
    p++;
    (*p)[0] = 255.0f * c0->x;
    (*p)[1] = 255.0f * c0->y;
    (*p)[2] = 255.0f * c0->z;
    (*p)[3] = 128.0f * c0->w;
    sceVu0FTOI0Vector((int *)p, *p);
    p++;
    sceVu0CopyVector(*p, (float *)ia);
    p++;
    (*p)[0] = 255.0f * c1->x;
    (*p)[1] = 255.0f * c1->y;
    (*p)[2] = 255.0f * c1->z;
    (*p)[3] = 128.0f * c1->w;
    sceVu0FTOI0Vector((int *)p, *p);
    p++;
    sceVu0CopyVector(*p, (float *)ib);
    p++;

    unsigned int cnt;
    int qwc = p - (sceVu0FVECTOR *)tag - 1;

    cnt = qwc | 0x10000000;
    tag[0] = cnt;
    tag[1] = 0;
    tag[2] = 0x11000000;
    tag[3] = qwc | 0x50000000;
    zSysFifoKick(packet, (u_long128 *)p - packet);
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

/* the texture's library, once, then the children's (C2DBitmapPoly's shape: ztwod.h's AddAssetLib
 * inlined one level, C2D's walk) */
void C2DLine::GetAssetLibs(CLibList &liblist)
{
    if (m_texture) {
        zdb::CAssetLib *lib;

        if ((lib = m_texture->m_assetlib) != 0)
            AddAssetLib(liblist, lib);
    }
    C2D::GetAssetLibs(liblist);
}
