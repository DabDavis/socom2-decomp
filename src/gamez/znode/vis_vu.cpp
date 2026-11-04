/* vis_vu.cpp -- zdb::CVisual's decals and VU code: OVL1 0x3B36F0?-0x3B7FDF, after zvid_init.cpp's
 * stretch and before vis_mesh.cpp, holding what SOCOM 1's vis_main.cpp held ahead of CVisual's
 * reading (DereferenceDecals .. Init, 0x3092B0-0x30E2FF in the demo) -- SOCOM II links this half
 * apart from the reading and texture selects (vis_main.cpp here, 0x3C3570-), and which half kept
 * the name is not settled: the file name is ours. Evidence that this half is SOCOM 1's vis_main.cpp
 * (round 10, not acted on): its tail holds CVisual::m_stack_vid's deque copy-constructor chain
 * (0x3B7DB0 and its pieces 0x3B7E30-0x3B83D0 -- SOCOM 1's deque<unsigned int> copy constructor
 * 0x30E4B0 closes its vis_main.cpp the same way), and the initialiser that builds that static
 * (0x404570, its call to 0x3B7DB0) sits between zvid_init.cpp's (0x404490, round 17) and the decal
 * pools' in the ctors table, as this unit sits between zvid_init.cpp and vis_mesh.cpp. Round 17:
 * 0x404500, between the two, is a third unit's <iostream> pair (its objects at 0x4B45F0, beside
 * this unit's statics; zvid_init.cpp's at 0x4A4160, beside its own) -- a unit between
 * zvid_init.cpp and this one, perhaps the unwritten 0x3B1BF0-0x3B36EF stretch. Only some of its
 * functions so far; the rest stay placeholders. Defined in reverse of their image order (-inline
 * deferred).
 *
 * Round 13 (the VU chain): SOCOM 1's VuObjectSetup/VuBatchProgram/VuShadowMap/VuLightMap/
 * VuDetailMap are rearranged in SOCOM II into passes that each write a command quadword (the VU
 * program's tag, the context count 0x4B4AF0, a word) and take the packet's cursor by address,
 * failing when the scratchpad's room (m_sprEndOfMP) runs out; VuMapPasses (0x3B5B90) runs them
 * and 0x3B5F20 (open, not written) is VuBatchProgram's command list around it. The statics keep
 * their data_ placeholders, each commented with SOCOM 1's name where its use pairs it. Still
 * open here: VuUpdate (0xCC4: its tail calls even 8-byte helpers out of line, the out-of-line
 * copies at 0x3B5250-0x3B52C0 following it, so its inline budget runs out -- likely
 * inline_max_total_size), 0x3B5F20, 0x3B41D0 (the decal program), 0x3B6870/0x3B6B20 (the local
 * lights' setup), 0x3B7170/0x3B76B0/0x3B78D0 (the lens colours), the deque copy chain. */

#include <stack>
#include "znode.h"
#include "../zsys/zsys_public.h"

/* the z buffer's write switch (zvid_init.cpp's) */
void zVid_ZTestOnWriteSet(bool on);

namespace zdb {

/* the free view ids' stack (znode.h's CVisual::m_stack_vid; zrdr.h's own std::stack keeps <stack>
 * out of znode.h) */
class CVidStack : public std::stack<void *> {
};

/* the view id in use, the stack's top -- a `break` if the stack is empty (the wep lane, round 13:
 * SOCOM 1's GetChainData and VuUpdate test the size and break the same way ahead of the top; the
 * name ours). The trap is MW's break and nop intrinsics, two instructions: inlined one level
 * (GetChainData: empty() and top() called) the target pads `break; nop; nop`, whole (its own copy,
 * 0x3B52C0) `break; nop` -- a lone __I_break leaves no nop in the first. It is six levels high
 * through std::stack<void *>'s top(), so a caller inlines it one level; its out-of-line copy
 * follows VuUpdate, its one caller there */
inline unsigned int CVisual::GetVid()
{
    if (m_stack_vid.empty()) {
        __I_break(0);
        __I_nop();
    }
    return (unsigned int)m_stack_vid.top();
}

/* a decal on a visual: the link and the pool it goes back to (vis_decal.cpp's class; SOCOM 1's,
 * no DWARF -- the names ours) */
class CDecal {
public:
    static void PutPermDecal(CDecal *d);
    static void PutTempDecal(CDecal *d);

    CDecal *m_next;   // 0x0
    CVisual *m_visual;   // 0x4
    unsigned char _8[0xc - 0x8];
    unsigned short m_view;   // 0xc
    unsigned short m_temp;   // 0xe: 1 for a temporary decal
};

/* the end of the scratchpad's room for a packet, the VU context count, the world-to-model matrix
 * and the model-to-world one (SOCOM 1's m_sprEndOfMP, m_WorldToModel and m_ModelToWorld by their
 * use; the count is SOCOM II's) */
extern "C" _word128 *data_004B4AE8;
extern "C" unsigned int data_004B4AF0;
extern "C" CMatrix data_004B4E70;
extern "C" CMatrix *data_004B4E60;
/* a command quadword for the VU program: its tag, the context count and a word, the quadword after
 * it given back (ours, inline: VuMapPasses steps its cursor through it, unreloaded; the
 * callers load the count before the first store, as an inline's arguments are) */
static inline _word128 *VuCommand(_word128 *p, unsigned int tag, unsigned int count, unsigned int n)
{
    p->u32[0] = tag;
    p->u32[1] = count;
    p->u32[2] = n;
    return p + 1;
}

/* whether the scratchpad's packet has room for n quadwords past the cursor (ours, inline: the
 * shadow and light passes test it materialised, `!(>=)` twice negated) */
static inline bool VuRoom(_word128 *p, unsigned int n)
{
    return !(p + n >= data_004B4AE8);
}

/* the local lights (SOCOM 1's statics) */
CVisual::LOCAL_LIGHT CVisual::localLightBuf[10];
CLight *CVisual::localLightPtr[10];
int CVisual::m_local_light_cnt;

/* whether the band draws at the squared distance: not outside it, fully between its fades, and
 * faded by the opacity within one (SOCOM 1's) */
bool CVisual::DrawLOD(CLOD_band *band, float d, float *opacity)
{
    if (d <= band->m_minRangeNearSq || !(d < band->m_maxRangeFarSq))
        return false;
    if (!band->m_minFade && !band->m_maxFade)
        return true;
    if (!(d < band->m_minRangeFarSq) && d <= band->m_maxRangeNearSq)
        return true;
    if (band->m_minFade && d <= band->m_minRangeFarSq) {
        *opacity *= band->m_minInvDeltaRangeSq * (d - band->m_minRangeNearSq);
        return true;
    }
    if (band->m_maxFade && !(d < band->m_maxRangeNearSq)) {
        *opacity *= band->m_maxInvDeltaRangeSq * (band->m_maxRangeFarSq - d);
        return true;
    }
    return true;
}

/* an entry of two static tables of four floats, by value (new in SOCOM II, not named: the particles'
 * Draw functions read the first, 0x4B4C70, CameraZoom::LoadLensFx the second, 0x4B4CB0). The named
 * local is the target's: built on the stack and copied out, where a returned temporary is built in
 * the return slot */
extern "C" CPnt4D data_004B4C70[];
extern "C" CPnt4D data_004B4CB0[];

extern "C" CPnt4D func_003B7620(int i)
{
    CPnt4D t(data_004B4CB0[i].x, data_004B4CB0[i].y, data_004B4CB0[i].z, data_004B4CB0[i].w);

    return t;
}

extern "C" CPnt4D func_003B7590(int i)
{
    CPnt4D t(data_004B4C70[i].x, data_004B4C70[i].y, data_004B4C70[i].z, data_004B4C70[i].w);

    return t;
}

/* the alpha switch: an int flag, the z buffer's writes off while it is on (SOCOM 1's AlphaEnable --
 * there the flag itself went to the z-write switch; the pipe's CAlpha::Render and the core and
 * cinematic states call it) */
extern "C" int data_004B4A20;

void CVisual::AlphaEnable(bool on)
{
    data_004B4A20 = on;
    zVid_ZTestOnWriteSet(!on);
}

/* the landmark switch: its flag set, the lighting the other way (SOCOM 1's LandmarkEnable, the same
 * code; the pipe's renderers call it) */
extern "C" unsigned char data_004B4A58;

void CVisual::LandmarkEnable(bool on)
{
    data_004B4A58 = on;
    m_lightingEnable = !on;
}

/* a light added to the local lights: its position, its squared range, its colour and the inverse
 * of its falloff, the light listed and counted (SOCOM 1's, the same code: the minimum range's square
 * is taken ahead of the colour's copy, the maximum's reloaded after it) */
void CVisual::AddLocalLight(CLight *light, CPnt3D *pos)
{
    LOCAL_LIGHT *l = &localLightBuf[m_local_light_cnt];

    l->m_pos[0] = pos->x;
    l->m_pos[1] = pos->y;
    l->m_pos[2] = pos->z;
    l->m_rangeSq = light->m_maxRangeSq;
    float min2 = light->m_minRange * light->m_minRange;

    l->m_color[0] = light->m_diffuse.x;
    l->m_color[1] = light->m_diffuse.y;
    l->m_color[2] = light->m_diffuse.z;
    l->m_invFalloff = 1.0f / (light->m_maxRangeSq - min2);
    localLightPtr[m_local_light_cnt] = light;
    m_local_light_cnt++;
}

/* the texture selects' DMA references (SOCOM 1's file statics _savedTS, the cursor, into m_savedTS:
 * VuUpdate points it at 0x4B4610) */
extern "C" _word128 *data_003E1440;

/* a texture select saved for the VU code: a DMA reference to the texture's select written to the
 * cursor through the uncached mirror, unpacking at the select's offset in the packet; unless the
 * blend word is the default one or already the select's, a second reference with the blend
 * register's write (SOCOM 1's, the same code) */
void CVisual::SaveTextureSelect(_word128 *select, CTexture *tex, unsigned long blend)
{
    int n = select - CSPR::m_sprPacket - 1;
    _word128 *ts = (_word128 *)((unsigned int)data_003E1440 | 0x20000000);
    unsigned long *alpha = &((_word128 *)((_word128 *)&tex->m_dmaRef)->u32[1])[1].u64[0];

    ts[0].u32[0] = 0x30000001;
    ts[0].u32[1] = ((_word128 *)&tex->m_dmaRef)->u32[1] + 48;
    ts[0].u32[2] = 0x01000101;
    ts[0].u32[3] = (n + 3) | 0x6C018000;
    data_003E1440++;
    if (blend != 0x4000000000000000 && *alpha != blend) {
        ts[1].u32[0] = 0x10000001;
        ts[1].u32[1] = 0;
        ts[1].u32[2] = 0x01000101;
        ts[1].u32[3] = (n + 1) | 0x6C018000;
        ts[2].u64[0] = blend;
        ts[2].u64[1] = 0x42;
        data_003E1440 += 2;
    }
}

/* the VU chain's statics (SOCOM 1's CVisual statics by their use: m_dmaChain, m_dmaQwc,
 * m_dmaSubfaceChain, m_dmaSubfaceQwc, m_applyDetailTexture; the subface's offset is new) */
extern "C" _word128 *data_004B4AC8;
extern "C" unsigned int data_004B4AD0;
extern "C" _word128 *data_004B4EC0;
extern "C" unsigned int data_004B4EC8;
extern "C" unsigned int data_004B4ED0;
extern "C" unsigned char data_004B4A88;

/* whether the detail layers draw: the first layer's length cuts the chain and the first layer
 * reaching the subface cuts that (SOCOM 1's, inlined in GetChainData: the result's `true` an
 * assignment) */
static inline bool DetailChain(CVisual *v)
{
    CVisual::DETAIL *d = v->m_details;

    bool r;

    if (d && data_004B4A88 && (v->m_flags & 0x4000)) {
        r = true;
        CVisual::DETAIL *e = d + v->m_numDetails;

        if (d->m_count8 < data_004B4AD0) {
            data_004B4AD0 = d->m_count8;
            d++;
        }
        if (data_004B4EC8) {
            for (; d < e; d++) {
                if (d->m_count8 >= data_004B4ED0) {
                    data_004B4EC8 = d->m_count8 - data_004B4ED0;
                    break;
                }
            }
        }
    } else
        r = false;
    return r;
}

/* the view's chain found: its head quadword gives the chain's length and its subface part (split
 * off the length), and whether the detail layers draw (SOCOM 1's, with the subface's offset kept) */
bool CVisual::GetChainData()
{
    _word128 *head = (_word128 *)m_lodMesh[GetVid()];

    data_004B4AC8 = head + 1;
    data_004B4AD0 = head->u16[0];
    data_004B4EC8 = head->u16[7];
    if (data_004B4EC8) {
        data_004B4ED0 = head->u16[6];
        data_004B4EC0 = data_004B4AC8 + data_004B4ED0;
        data_004B4AD0 -= data_004B4EC8;
    } else
        data_004B4EC0 = 0;
    return DetailChain(this);
}

/* a node's render set-up: the model-to-world matrix kept and its inverse taken; lighting on for a
 * node or atom that asks for it (unless 0x4B4A60 is set), the light scale from the atom's signed
 * byte at 0xA0 (negative: full), and 0x3E1458 on unless the node is clutter (SOCOM 1's NodeSetup,
 * its fourth argument gone: no SOCOM II caller sets a3). The scale's byte in a variable of its own
 * pads the else label (MW's 8-byte label padding) */
extern "C" CMatrix *data_004B4E60;
extern "C" CMatrix data_004B4E70;
extern "C" unsigned char data_004B4A60;
extern "C" bool data_003E1458;
extern "C" float m_GlobalLightScale__Q23zdb7CVisual;

void CVisual::NodeSetup(CNode *atom, CNode *node, CMatrix *m)
{
    data_004B4E60 = m;
    Vu0MatrixInverseWithUniformScale(&data_004B4E70, m);
    if (!data_004B4A60 && (atom->m_dynamic_motion || node->m_dynamic_light)) {
        m_lightingEnable = true;
        signed char s = atom->m_socom2_a0;

        if (s < 0)
            m_GlobalLightScale__Q23zdb7CVisual = 1.0f;
        else
            m_GlobalLightScale__Q23zdb7CVisual = (1.0f / 127.0f) * s;
        data_003E1458 = !node->m_clutter;
    } else {
        m_lightingEnable = false;
        data_003E1458 = false;
    }
}

/* the object header's statics: its second quadword (0x4B4D30, 0x3B5A10's scale and offset), the
 * clip word (SOCOM 1's m_fovClipWord), the light scales (SOCOM 1's m_GlobalLightScale and
 * m_AmbientLightScale), the fog switch and the landmark flag (SOCOM 1's m_fogEnable,
 * m_landmarkFlag), the default fog plane and vector (SOCOM 1's NoAltFogBottomV/NoAltFogVectorV)
 * and the packet's cursor quadword (0x3B6820's) */
extern "C" _word128 data_004B4D30;
extern "C" unsigned int data_004B4E58;
extern "C" float m_GlobalLightScale__Q23zdb7CVisual;
extern "C" float data_003E1480;
extern "C" unsigned char data_003E1468;
extern "C" unsigned char data_004B4A58;
extern "C" CPnt4D data_003FC200[2];
extern "C" _word128 *data_004B4AD8;

/* the camera's alternate-fog switch (CCamera's, the top bit of its byte 335: zcamera.h's _word128
 * clashes with zsys_public.h's here, so the camera is read through this; the name ours) */
struct CAMERA_FOG {
    unsigned char _0[335];
    unsigned char _b0 : 7;
    unsigned char m_altFog : 1;
};

/* the packet's head flag set (ours): bit 3 of the head's word 3 */
struct VU_HEAD_FLAGS {
    unsigned char _0[12];
    unsigned char m_b0 : 3;
    unsigned char m_kick : 1;
    unsigned char m_b4 : 4;
};

/* the object header: the clip word, the light scales, the fog plane and vector in model space with
 * their range (the landmarks' own, at full range), the model-to-world matrix's place left after it;
 * the packet's head word cleared and flagged (SOCOM 1's VuObjectSetup's first part: the matrices
 * are 0x3B5F20's now) */
_word128 *CVisual::VuObjectSetup(_word128 *p)
{
    p[1] = data_004B4D30;
    if (m_1c == -2)
        p->u32[0] = 63;
    else
        p->u32[0] = data_004B4E58 >> 8;
    if (m_flags & 0x40)
        p->f32[1] = m_GlobalLightScale__Q23zdb7CVisual;
    else
        p->f32[1] = 0;
    if (m_flags & 0x20)
        p->f32[2] = data_003E1480;
    else
        p->f32[2] = 0;
    if (data_003E1468) {
        CPnt4D *fog;

        if (data_004B4A58)
            fog = (CPnt4D *)((char *)m_camera + 1280);
        else if (((CAMERA_FOG *)m_camera)->m_altFog && m_bits.m_b19)
            fog = (CPnt4D *)((char *)m_camera + 1248);
        else
            fog = data_003FC200;
        data_004B4E70.Transform(fog, (CPnt4D *)&p[2], 2);
        if (data_004B4A58) {
            p[2].f32[3] = 255.0f;
            p[3].f32[3] = 0;
        } else {
            p[2].f32[3] = *(float *)((char *)m_camera + 304);
            p[3].f32[3] = *(float *)((char *)m_camera + 308);
        }
    }
    data_004B4AD8->u32[3] = 0;
    ((VU_HEAD_FLAGS *)data_004B4AD8)->m_kick = 1;
    return p + 4;
}

/* the saved texture selects' references (SOCOM 1's m_savedTS, 32 quadwords) and the packet's head
 * (0x3B6820 keeps the packet's start there) */
extern "C" _word128 data_004B4610[32];
extern "C" _word128 *data_004B4AE0;   // (declared again below with the cursor)

/* the packet closed: a flush when no select was saved, the head quadword's DMA tag and unpack set
 * from the packet's length, then the saved selects' references copied in after it with a flush (SOCOM
 * 1's VuExecuteState0, its head the static now) */
_word128 *CVisual::VuExecuteState0(_word128 *p)
{
    _word128 *start = p;

    if (data_003E1440 == data_004B4610) {
        p->u32[0] = 0x15000000;
        p->u32[1] = 0;
        p->u32[2] = 0;
        p->u32[3] = 0;
        p++;
    }

    int head = start - CSPR::m_sprPacket - 1;
    int len = p - CSPR::m_sprPacket - 1;

    data_004B4AE0->u32[0] = len | 0x10000000;
    data_004B4AE0->u32[1] = 0;
    data_004B4AE0->u32[2] = 0x01000101;
    data_004B4AE0->u32[3] = (head << 16) | 0x6C008000;
    if (data_004B4610 < data_003E1440) {
        p = CSPR::Copy(p, data_004B4610, data_003E1440 - data_004B4610);
        data_003E1440 = data_004B4610;
        p->u32[0] = 0x10000000;
        p->u32[1] = 0;
        p->u32[2] = 0x15000000;
        p->u32[3] = 0;
        p++;
    }
    return p;
}

/* the VU packet's statics (new in SOCOM II, not named): the cursor and the packet's start in the
 * scratchpad, and the scale and offset 0x3B5A10 applies (0x4B4AD8, 0x4B4AE0, 0x4B4D30) */
extern "C" _word128 *data_004B4AD8;
extern "C" _word128 *data_004B4AE0;
extern "C" _word128 data_004B4D30;

/* a scratchpad packet begun: taken from the system, its start kept and its first quadword cleared
 * past the tag, the cursor after it returned (0x3B4580 calls it; new in SOCOM II, not named) */
extern "C" _word128 *func_003B6820()
{
    _word128 *p = (_word128 *)zSysSprGetPacket((u_long128 **)&CSPR::m_sprPacketEnd);

    CSPR::m_sprPacket = p;
    data_004B4AD8 = p + 1;
    data_004B4AE0 = p;
    data_004B4AD8->u128 = 0;
    return data_004B4AD8 + 1;
}

/* the projected map's colour (SOCOM II's, not named) */
extern "C" float data_004B4EE0[4];

/* a projected map: its projection, its node and its texture (SOCOM II's; the names ours) */
struct CProjectedMap {
    CMatrix m_projection;   // 0x0
    unsigned char _40[0x4c - 0x40];
    CNode *m_node;   // 0x4c
    CTexture *m_tex;   // 0x50
};

/* a frame whose third row is the direction, built about the up vector (zmath_main.cpp's, not named) */
extern "C" void func_00307700(sceVu0FVECTOR zd, sceVu0FVECTOR p, sceVu0FMATRIX out);

/* the material band in use (SOCOM 1's m_mtl_band by its use: VuUpdate clears it first, then points it
 * at the world's material of the visual's) */
extern "C" CMaterial_band *data_004B4AA0;

/* the passes' switches: the packets' flavour, the specular map's (SOCOM 1's m_specMapGlobal), the
 * counts of projected, shadow and light maps (SOCOM 1's m_shadowMapCnt, m_lightMapCnt; the
 * projected maps' SOCOM II's) and the specular texture when the band has none (SOCOM 1's
 * m_specMapTexture) */
extern "C" unsigned int data_004B4EB0;
extern "C" unsigned char data_004B4DC0;
extern "C" unsigned int data_004B4E40;
extern "C" unsigned int data_004B4E18;
extern "C" unsigned int data_004B4DF0;
extern "C" CTexHandle *data_004B4D90;

/* the specular map's pass: a command, the band's texture select (or the default specular texture's)
 * saved with the default blend, and the setup (ours, inline: 0x3B5B90 inlines both of its calls,
 * the `true` an assignment) */
inline bool CVisual::VuSpecularMap(_word128 **cursor, unsigned int tag)
{
    _word128 *p = *cursor;

    if (!(p + 12 < data_004B4AE8))
        return false;
    VuCommand(p, tag, data_004B4AF0, 1);

    CTexHandle *h = data_004B4AA0->m_tex;
    _word128 *q = p + 1;

    if (!h)
        h = data_004B4D90;
    h->m_texture->CopyTextureSelect(q);
    SaveTextureSelect(q, h->m_texture, 0x4000000000000000);

    CMaterial_band *band = data_004B4AA0;
    CPnt4D color;

    color.x = band->m_color.x;
    color.y = band->m_color.y;
    color.z = band->m_color.z;
    color.w = band->m_color.w;

    VuSpecularSetup(q, &color, &band->m_rgb, band->m_opacity);
    *cursor = q + 11;
    return true;
}

/* the map passes of a packet: the detail layer's, then a command opening the passes, the specular,
 * projected, shadow and light maps' as each is wanted, a command closing them -- the first that
 * runs out of room ends them; two flavours by the static, their tags two apart (SOCOM II's, from
 * SOCOM 1's VuBatchProgram's middle: the name ours) */
_word128 *CVisual::VuMapPasses(_word128 *p, tag_DETAIL_PARAMS *detail)
{
    if (data_004B4EB0) {
        if (detail && !VuDetailMap(&p, 48, detail))
            goto done;
        p = VuCommand(p, 114, data_004B4AF0, 1);
        if (data_004B4DC0 && !VuSpecularMap(&p, 52))
            goto done;
        if (data_004B4E40 && !VuProjectedMap(&p, 118))
            goto done;
        if (data_004B4E18 && !VuShadowMap(&p, 60))
            goto done;
        if (data_004B4DF0)
            VuLightMap(&p, 56);
    } else {
        if (detail && !VuDetailMap(&p, 50, detail))
            goto done;
        p = VuCommand(p, 114, data_004B4AF0, 1);
        if (data_004B4DC0 && !VuSpecularMap(&p, 54))
            goto done;
        if (data_004B4E40 && !VuProjectedMap(&p, 120))
            goto done;
        if (data_004B4E18 && !VuShadowMap(&p, 62))
            goto done;
        if (data_004B4DF0)
            VuLightMap(&p, 58);
    }
done:
    p = VuCommand(p, 116, data_004B4AF0, 1);
    return p;
}

/* the visual's squared distance to the camera and the detail textures' address offset (SOCOM 1's
 * m_rangeSqdToCamera by its use; the offset SOCOM II's) */
extern "C" float data_004B4A98;
extern "C" unsigned int data_004B4EB8;

/* a texture select's TEX0 base pointer, the low 12 bits of its halfword (ours) */
struct TBP_HALF {
    unsigned short m_tbp : 12;
    unsigned short m_rest : 4;
};

/* the detail layer's pass: per texture, while the visual is within the layer's range, a command,
 * the select (its first halfword the entry's when it has one, its base offset by the static when
 * set) and the entry's float (SOCOM 1's VuDetailMap, made a pass per texture) */
bool CVisual::VuDetailMap(_word128 **cursor, unsigned int tag, tag_DETAIL_PARAMS *detail)
{
    _word128 *q = *cursor;

    for (unsigned int i = 0; i < detail->m_count; i++) {
        if (data_004B4A98 <= detail->m_rangeSq) {
            if (!(q + 9 < data_004B4AE8))
                return false;

            _word128 *p = *cursor;

            q = p + 1;
            VuCommand(p, tag, data_004B4AF0, 1);

            tag_DETAIL_PARAMS::ENTRY *e = &detail->m_data[i];

            e->m_tex->CopyTextureSelect(q);
            if (e->m_select)
                q[1].u16[0] = e->m_select;
            if (data_004B4EB8)
                ((TBP_HALF *)&q[2].u16[2])->m_tbp += (unsigned short)data_004B4EB8;
            q[7].f32[0] = e->m_8;
            q += 8;
            *cursor = q;
        }
    }
    return true;
}

/* a point scaled by the static's xyz and offset by its w, in VU0 (the zdb world's 0x339DE0 calls
 * it; new in SOCOM II, not named) */
extern "C" asm void func_003B5A10(_word128 *in, _word128 *out)
{
    la v1, data_004B4D30
    lqc2 vf7, 0(a0)
    lqc2 vf4, 0(v1)
    vmul.xyz vf7, vf7, vf4
    vaddw.xyz vf7, vf7, vf4
    jr ra
    sqc2 vf7, 0(a1)
}

/* a point through a matrix's rotation first when the flag (0x4B4A68) is up, then scaled and offset
 * as 0x3B5A10 does (new in SOCOM II, not named). The address's `la` sits in the branch's delay
 * slot, so its lui lands there as the target's does */
extern "C" unsigned char data_004B4A68;

extern "C" asm void func_003B59A0(_word128 *in, _word128 *m, _word128 *out)
{
    .set noreorder
    lbu v1, data_004B4A68
    beqz v1, plain
    la v1, data_004B4D30
    lqc2 vf7, 0(a0)
    lqc2 vf4, 0(a1)
    lqc2 vf5, 0(v1)
    vmulax.xyz ACC, vf4, vf7x
    vmadday.xyz ACC, vf4, vf7y
    vmaddaz.xyz ACC, vf4, vf7z
    vmaddz.xyz vf7, vf4, vf4z
    vmul.xyz vf7, vf7, vf5
    vaddw.xyz vf7, vf7, vf5w
    b done
    sqc2 vf7, 0(a2)
plain:
    la v1, data_004B4D30
    lqc2 vf7, 0(a0)
    lqc2 vf4, 0(v1)
    vmul.xyz vf7, vf7, vf4
    vaddw.xyz vf7, vf7, vf4w
    sqc2 vf7, 0(a2)
    nop
done:
    jr ra
    nop
}

/* the projected map's pass: the first map's texture select, its position in model space, its
 * projection into model space and the colour (SOCOM II's; the name ours). Pinned: fifteen statements
 * by MW's count, -inline auto would take it into VuMapPasses, which calls it */
#pragma auto_inline off
bool CVisual::VuProjectedMap(_word128 **cursor, unsigned int tag)
{
    _word128 *p = *cursor;

    if (!(p + 13 < data_004B4AE8))
        return false;

    _word128 *q = p + 1;

    VuCommand(p, tag, data_004B4AF0, 1);

    CProjectedMap *map = (CProjectedMap *)m_projectedMapList[0];

    map->m_tex->CopyTextureSelect(q);
    data_004B4E70.Transform((CPnt4D *)map->m_node->m_matrix.m_matrix[3], (CPnt4D *)&q[10], 1);
    sceVu0MulMatrix(*(sceVu0FMATRIX *)&q[6], map->m_projection.m_matrix, data_004B4E60->m_matrix);
    q[11].f32[0] = data_004B4EE0[0];
    q[11].f32[1] = data_004B4EE0[1];
    q[11].f32[2] = data_004B4EE0[2];
    q[11].f32[3] = data_004B4EE0[3];
    *cursor = q + 12;
    return true;
}
#pragma auto_inline reset

/* the shadow maps' pass: per map a command, its texture select, its position and direction in
 * model space, its projection into model space, its range, falloff, colour and opacity (SOCOM 1's
 * VuShadowMap, made a pass per map) */
bool CVisual::VuShadowMap(_word128 **cursor, unsigned int tag)
{
    _word128 *q = *cursor;

    for (std::vector<CLightMap *>::iterator it = m_shadowMapList.begin(); it != m_shadowMapList.end(); ++it) {
        if (!VuRoom(q, 15))
            return false;

        _word128 *p = *cursor;

        q = p + 1;
        VuCommand(p, tag, data_004B4AF0, 1);

        CLightMap *map = *it;

        map->m_pTex->CopyTextureSelect(q);
        data_004B4E70.Transform((CPnt4D *)map->m_matrix.m_matrix[3], (CPnt4D *)&q[10], 1);
        data_004B4E70.Transform(&map->m_dir, (CPnt4D *)&q[11], 1);
        sceVu0MulMatrix(*(sceVu0FMATRIX *)&q[6], map->m_worldToUV.m_matrix, data_004B4E60->m_matrix);
        q[12].f32[2] = map->m_maxRange;
        q[12].f32[3] = map->m_invDeltaRange;
        q[13].f32[0] = map->m_diffuse.x;
        q[13].f32[1] = map->m_diffuse.y;
        q[13].f32[2] = map->m_diffuse.z;
        q[13].f32[3] = map->m_opacity;
        q += 14;
        *cursor = q;
    }
    return true;
}

/* the colour tables (0x3B7590/0x3B7620 read them by value) and the flag that tints the light maps
 * through them (SOCOM II's, not named) */
extern "C" CPnt4D data_004B4C70[];
extern "C" unsigned char data_004B4A68;

/* the light maps' pass: per map a command (its word 1 for the first, which carries the first map's
 * texture select and blend), its position in model space, its falloff, range, colour and opacity,
 * the colour tinted through a table when the flag is up (SOCOM 1's VuLightMap, made a pass per
 * map). The flag is an int (`li`, stored unmasked), the iterator assigned from begin() (its
 * temporary), the blend word a local (loaded ahead of the texture for the call), the type test a
 * bool local (its sltu). OPEN, 5 instructions: the loop test's end() -- the target colours the
 * size v1 and the iterator's word a0, ours the other way round (end() != it, !(it == end()), ++it
 * and a while loop do not move it) */
bool CVisual::VuLightMap(_word128 **cursor, unsigned int tag)
{
    if (!(*cursor + 10 < data_004B4AE8))
        return false;

    int first = 1;
    std::vector<CLightMap *>::iterator it;

    for (it = m_lightMapList.begin(); it != m_lightMapList.end(); it++) {
        _word128 *p = *cursor;

        if (!(p + 3 < data_004B4AE8))
            return false;
        _word128 *q1 = p + 1;

        VuCommand(p, tag, data_004B4AF0, first);
        *cursor = q1;
        if (first) {
            (*it)->m_pTex->CopyTextureSelect(*cursor);
            (*cursor)[1].u64[0] = (*it)->m_blendmode.m_alphaWORD;
            unsigned long blend = (*it)->m_blendmode.m_alphaWORD;

            SaveTextureSelect(*cursor, (*it)->m_pTex, blend);
            *cursor += 6;
            first = 0;
        }

        CLightMap *map = *it;
        _word128 *q = *cursor;

        data_004B4E70.Transform((CPnt4D *)map->m_matrix.m_matrix[3], (CPnt4D *)q, 1);
        q[1].f32[1] = map->m_invMaxRange;
        q[1].f32[2] = map->m_maxRange;
        q[1].f32[3] = map->m_invDeltaRange;
        q[2].f32[0] = map->m_diffuse.x;
        q[2].f32[1] = map->m_diffuse.y;
        q[2].f32[2] = map->m_diffuse.z;
        q[2].f32[3] = map->m_opacity;
        if (data_004B4A68) {
            bool mesh = GetType() == 1;
            CPnt4D *m = !mesh ? &data_004B4C70[0] : &data_004B4C70[2];
            _word128 *c = &q[2];

            asm volatile {
                lqc2 vf7, 0(c)
                lqc2 vf4, 0(m)
                vmulax.xyz ACC, vf4, vf7x
                vmadday.xyz ACC, vf4, vf7y
                vmaddaz.xyz ACC, vf4, vf7z
                vmaddz.xyz vf7, vf4, vf4z
                sqc2 vf7, 0(c)
            }
        }
        q += 3;
        *cursor = q;
    }
    return true;
}

/* the specular map's setup: a frame looking along the material's direction, into model space; the
 * band's intensity and falloff; the camera's position in model space; the colour and opacity (SOCOM
 * II's: 0x3B5B90 calls it with the band's colour and opacity -- the name ours) */
void CVisual::VuSpecularSetup(_word128 *q, CPnt4D *dir, CPnt3D *rgb, float opacity)
{
    CPnt4D up = CPnt4D::unit_w;
    CMatrix look;
    CMatrix m;

    func_00307700((float *)dir, (float *)&up, look.m_matrix);
    sceVu0MulMatrix(m.m_matrix, look.m_matrix, data_004B4E60->m_matrix);
    sceVu0CopyMatrix(*(sceVu0FMATRIX *)&q[6], m.m_matrix);
    q[10].f32[0] = data_004B4AA0->m_intensity;
    q[10].f32[1] = data_004B4AA0->m_falloff;
    q[10].f32[3] = data_004B4AA0->m_invFalloff;

    CPnt3D cam = *(CPnt3D *)((CNode *)(void *)m_camera)->m_matrix.m_matrix[3];
    CPnt4D p(cam.x, cam.y, cam.z, 1.0f);

    sceVu0ApplyMatrix((float *)&p, data_004B4E70.m_matrix, (float *)&p);
    q[6].f32[3] = p.x;
    q[7].f32[3] = p.y;
    q[8].f32[3] = p.z;
    q[9].f32[0] = rgb->x;
    q[9].f32[1] = rgb->y;
    q[9].f32[2] = rgb->z;
    q[9].f32[3] = opacity;
}

/* the quadwords left in the scratchpad's packet past a cursor (0x3B4580's; not named) */
extern "C" int func_003B5270(_word128 *p)
{
    return ((int)CSPR::m_sprPacketEnd - (int)p) / 16;
}

/* a quadword copied (0x3B4580's; not named) */
extern "C" _word128 *func_003B5260(_word128 *to, _word128 *from)
{
    *to = *from;
    return to;
}

/* the decal builder (0x3B3AB0, not written): the visual, the node's view id, the size, the point,
 * the normal in the model's frame, the UV projection and the texture */
extern "C" void func_003B3AB0(CVisual *v, int vid, float size, CPnt3D *pos, CPnt3D *normal,
                              CMatrix *uv, CTexHandle *tex);

/* a decal projected onto the visual: the normal brought into the model's frame through the
 * world-to-model inverse, the projection composed with the model-to-world matrix, the node's view
 * id (none without a node) handed to the builder (SOCOM 1's, SOCOM II's builder taking the
 * matrix). OPEN, 2 instructions: the copy's argument set-up -- the target (and SOCOM 1's) sets a1
 * (the matrix) before the call and &inv in its delay slot, here &inv goes first and a1 fills the
 * slot; the direct call, the copy assignment, casts, a pointer local, the four declaration orders
 * of n/m/inv/vid and the id as short/int/unsigned all give the same two (build/scratch/wep/v_adr*.py) */
void CVisual::ApplyDecalRender(CMatrix *mtw, CNode *node, float size, CPnt3D *pos, CPnt3D *normal,
                               CTexHandle *tex, CMatrix *uv)
{
    CPnt3D n;
    CMatrix m;
    CMatrix inv;
    int vid = node ? node->m_vid : 0;

    inv = *mtw;
    Vu0MatrixInverseWithUniformScale(&inv, &inv);
    TransformVectors(&inv, normal, &n, 1);
    sceVu0MulMatrix(m.m_matrix, uv->m_matrix, mtw->m_matrix);
    func_003B3AB0(this, vid, size, pos, &n, &m, tex);
}

/* the decals of one view let go of, each back to the pool its kind says, and the view's list
 * emptied -- none on a visual of another type (SOCOM 1's, the same code). The list's slot is read
 * and cleared through the pod's data() reference, &data_ kept in a saved register across the
 * pools' calls (the pointer vector's operator[] reads data_ straight: `this` would be kept) */
void CVisual::DereferenceDecals(unsigned int vid)
{
    if (m_type == 0 && m_24.size()) {
        CDecal *d = (CDecal *)m_24.data()[vid];

        if (d) {
            while (d) {
                if (d->m_temp == 1)
                    CDecal::PutTempDecal(d);
                else
                    CDecal::PutPermDecal(d);
                d = d->m_next;
            }
            m_24.data()[vid] = 0;
        }
    }
}

}
