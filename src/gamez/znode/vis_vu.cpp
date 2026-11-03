/* vis_vu.cpp -- zdb::CVisual's decals and VU code: OVL1 0x3B36F0?-0x3B7FDF, after zvid_init.cpp's
 * stretch and before vis_mesh.cpp, holding what SOCOM 1's vis_main.cpp held ahead of CVisual's
 * reading (DereferenceDecals .. Init, 0x3092B0-0x30E2FF in the demo) -- SOCOM II links this half
 * apart from the reading and texture selects (vis_main.cpp here, 0x3C3570-), and which half kept
 * the name is not settled: the file name is ours. Evidence that this half is SOCOM 1's vis_main.cpp
 * (round 10, not acted on): its tail holds CVisual::m_stack_vid's deque copy-constructor chain
 * (0x3B7DB0 and its pieces 0x3B7E30-0x3B83D0 -- SOCOM 1's deque<unsigned int> copy constructor
 * 0x30E4B0 closes its vis_main.cpp the same way), and the initialiser that builds that static
 * (0x404570, its call to 0x3B7DB0) sits between zvid_init.cpp's (0x404500) and the decal pools' in
 * the ctors table, as this unit sits between zvid_init.cpp and vis_mesh.cpp. Only some of its
 * functions so far; the rest stay placeholders. Defined in reverse of their image order (-inline
 * deferred). */

#include <stack>
#include "znode.h"
#include "../zsys/zsys_public.h"

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

/* the end of the scratchpad's room for a packet, the VU context count, the world-to-model matrix
 * and the model-to-world one (SOCOM 1's m_sprEndOfMP, m_WorldToModel and m_ModelToWorld by their
 * use; the count is SOCOM II's) */
extern "C" _word128 *data_004B4AE8;
extern "C" unsigned int data_004B4AF0;
extern "C" CMatrix data_004B4E70;
extern "C" CMatrix *data_004B4E60;
/* the projected map's colour (SOCOM II's, not named) */
extern "C" float data_004B4EE0[4];

/* a projected map: its projection, its node and its texture (SOCOM II's; the names ours) */
struct CProjectedMap {
    CMatrix m_projection;   // 0x0
    unsigned char _40[0x4c - 0x40];
    CNode *m_node;   // 0x4c
    CTexture *m_tex;   // 0x50
};

/* a command quadword for the VU program: its tag, the context count and a word (ours, inline: the
 * callers load the count before the first store, as an inline's arguments are) */
static inline void VuCommand(_word128 *p, unsigned int tag, unsigned int count, unsigned int n)
{
    p->u32[0] = tag;
    p->u32[1] = count;
    p->u32[2] = n;
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

/* whether the scratchpad's packet has room for n quadwords past the cursor (ours, inline: the
 * shadow and light passes test it materialised, `!(>=)` twice negated) */
static inline bool VuRoom(_word128 *p, unsigned int n)
{
    return !(p + n >= data_004B4AE8);
}

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

/* the projected map's pass: the first map's texture select, its position in model space, its
 * projection into model space and the colour (SOCOM II's; the name ours) */
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
