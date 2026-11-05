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
 * and 0x3B5F20 is VuBatchProgram's command list around it (byte-exact since round 29, with
 * VuBatchProgram_Decals 0x3B41D0: SOCOM 1's two, the object header become a flag or dropped,
 * written with the VuCommand inline on the first build). The statics keep
 * their data_ placeholders, each commented with SOCOM 1's name where its use pairs it. Still
 * open here: VuUpdate (0xCC4: its tail calls even 8-byte helpers out of line, the out-of-line
 * copies at 0x3B5250-0x3B52C0 following it, so its inline budget runs out -- likely
 * inline_max_total_size), the deque copy chain's init
 * 0x3B7E30, do_assign 0x3B8000 and default constructor 0x3B7FA0 (the rest of the chain is
 * byte-exact since the MSL headers' round 18: m_stack_vid, below). Round 20: the unit's
 * initialiser, the light tints (0x3B7170, 0x3B76B0, 0x3B78D0) and the header's lights (0x3B6870)
 * byte-exact. Round 25: the lights' packet 0x3B6B20 byte-exact (the by-value 4D copy is a CPnt4D
 * temporary handed to an inline by reference) and AttachDecal 0x3B3800 written (open, 8 bytes). */

#include <stack>
#include "znode.h"
#include "../zsys/zsys_public.h"

/* the z buffer's write switch (zvid_init.cpp's) */
void zVid_ZTestOnWriteSet(bool on);

namespace zdb {

/* a decal's add-on point (SOCOM 1's zdb::AddonPoint by its constructor's name, 0x3B7CF0, which
 * SOCOM 1 builds the same way; no DWARF, the members' names ours): a position from the zero point,
 * two halfwords and two zeroed points with a word after each -- all assigned in the body: the zero
 * point ASSIGNED is copied a field at a time through one FPR, each field's address its own lui (the
 * target's); copy-constructed in the initializer list its address is taken into a register once */
struct AddonPoint {
    AddonPoint()
    {
        m_pos = CPnt3D::zero;
        m_c = 0;
        m_e = -1;
        m_10 = CPnt3D(0.0f, 0.0f, 0.0f);
        m_20 = CPnt3D(0.0f, 0.0f, 0.0f);
        m_1c = 0;
        m_2c = 0;
    }

    CPnt3D m_pos;   // 0x0
    short m_c;   // 0xc
    short m_e;   // 0xe: -1
    CPnt3D m_10;   // 0x10
    int m_1c;   // 0x1c
    CPnt3D m_20;   // 0x20
    int m_2c;   // 0x2c
};   // 0x30 bytes

/* THE UNIT'S STATICS, in __sinit_vis_vu.cpp's order (0x404570, 0x470 bytes: it builds each in its
 * definition order, the stores of each run scheduled together) -- SOCOM 1's __sinit_vis_main.cpp
 * builds the same statics bar SOCOM II's new colour tables, its _StartingMatrix and visData (SOCOM
 * II's CVisData is vis_data.cpp's) */

/* the custom global lights, four of tag_GlobalLight's two unit matrices (0x4B4820) */
tag_GlobalLight CVisual::m_customGlobalLight[4];

/* the free view ids (0x4B4A40): a temporary deque<void *> default-built (0x3B7FA0), copied in
 * (0x3B7DB0) and torn down to ~__deque_buf with 0, the stack's destructor registered (0x3B7D50).
 * Defined here, the unit's tail emits the stack's chain (the MSL headers round 18): ~deque<PCv>
 * 0x3B7C90 (one this-tested level, then ~__deque_buf: the pointer deque's base, not CVisBase), the
 * deque<void *> copy 0x3B7DB0 with its begin/end 0x3B7F40/0x3B7EE0, byte-exact. A plain
 * std::stack<void *>: 0x3B7D50, the destructor the initialiser registers, is ~stack<Pv> inlining
 * ~deque<Pv> and calling ~deque<PCv> (a wrapper class registered its own destructor). OPEN here:
 * deque<void *>() 0x3B7FA0 (0x30 vs 0x28: the default constructors' middle level 0x334120) and
 * init 0x3B7E30 (0x88 vs 0xB0: its iterators copied once more in the target, around two empty
 * converters) */
std::stack<void *> CVisual::m_stack_vid;

/* the base fog colour (0x4B4AB0; CSaveLoad::Load sets its w, SeeFarther reads it) */
CPnt4D CVisual::m_basefog_color(1.0f, 1.0f, 1.0f, 0.0f);

/* the local lights (0x4B4B00, 0x4B4C40, 0x4B4C68) */
CVisual::LOCAL_LIGHT CVisual::localLightBuf[10];
CLight *CVisual::localLightPtr[10];
int CVisual::m_local_light_cnt;

/* the light maps' tints (0x4B4C70, 0x4B4CB0, 0x4B4CF0), the lens colours' product (0x4B4D30), the
 * lens colours (0x4B4D40) and the added colour (0x4B4D80) */
CPnt4D CVisual::m_lightTint[4] = {
    CPnt4D(0.33f, 0.33f, 0.33f, 0.0f), CPnt4D(0.33f, 0.33f, 0.33f, 0.0f),
    CPnt4D(0.33f, 0.33f, 0.33f, 0.0f), CPnt4D(0.33f, 0.33f, 0.33f, 0.0f)
};
CPnt4D CVisual::m_lightTintTarget[4] = {
    CPnt4D(0.33f, 0.33f, 0.33f, 0.0f), CPnt4D(0.33f, 0.33f, 0.33f, 0.0f),
    CPnt4D(0.33f, 0.33f, 0.33f, 0.0f), CPnt4D(0.33f, 0.33f, 0.33f, 0.0f)
};
CPnt4D CVisual::m_lightTintRate[4] = {
    CPnt4D(1.0f, 1.0f, 1.0f, 0.0f), CPnt4D(1.0f, 1.0f, 1.0f, 0.0f),
    CPnt4D(1.0f, 1.0f, 1.0f, 0.0f), CPnt4D(1.0f, 1.0f, 1.0f, 0.0f)
};
CPnt4D CVisual::m_lensScale(1.0f, 1.0f, 1.0f, 0.5f);
CPnt4D CVisual::m_lensColor[4] = {
    CPnt4D(1.0f, 1.0f, 1.0f, 0.0f), CPnt4D(1.0f, 1.0f, 1.0f, 0.0f),
    CPnt4D(1.0f, 1.0f, 1.0f, 0.0f), CPnt4D(1.0f, 1.0f, 1.0f, 0.0f)
};
CPnt3D CVisual::m_addColor(0.0f, 0.0f, 0.0f);

/* the specular, light, shadow and projected maps' lists (0x4B4DA8, 0x4B4DE0, 0x4B4E08, 0x4B4E30) */
CLightMapVector CVisual::m_specMapList;
CLightMapVector CVisual::m_lightMapList;
CLightMapVector CVisual::m_shadowMapList;
CProjectedMapVector CVisual::m_projectedMapList;

/* the world-to-model matrix (0x4B4E70, a unit matrix) */
CMatrix CVisual::m_WorldToModel;

}

/* the fog plane and vector without the camera's alternate fog (SOCOM 1's file statics, outside zdb:
 * the demo's symbols are the bare NoAltFogBottomV/NoAltFogVectorV; const, so MW puts them in .data,
 * 0x3FC200 -- VuObjectSetup transforms the pair together) */
static const CPnt4D NoAltFogBottomV(0.0f, -100000000.0f, 0.0f, 1.0f);
static const CPnt4D NoAltFogVectorV(0.0f, 1.0f, 0.0f, 0.0f);

/* the projected map's colour (0x4B4EE0) */
CPnt4D zdb::CVisual::m_projectedColor(128.0f, 128.0f, 128.0f, 240.0f);

/* the decal builder's add-on points (SOCOM 1's file static decaldata, outside zdb as the demo's
 * symbol is: 0x4B4EF0) */
static zdb::AddonPoint decaldata[6];

namespace zdb {

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
class CDecal;

/* a decal's pool (vis_decal.cpp's class: only the call here) */
class CDecalPool {
public:
    CDecal *Get(bool b);
};

class CDecal {
public:
    static void PutPermDecal(CDecal *d);
    static void PutTempDecal(CDecal *d);
    /* a decal taken from the permanent or the temporary pool (inline: the names ours, after the
     * Put pair -- AttachDecal's two arms each an inlined call, which keeps the target's branch: the
     * delay slot filled from the permanent arm, the temporary arm's own lui and its padded join) */
    static CDecal *GetPermDecal() { return m_permPool.Get(false); }
    static CDecal *GetTempDecal() { return m_tempPool.Get(true); }

    CDecal *m_next;   // 0x0
    CVisual *m_visual;   // 0x4
    unsigned char _8[0xc - 0x8];
    unsigned short m_view;   // 0xc
    unsigned short m_temp;   // 0xe: 1 for a temporary decal

    static CDecalPool m_tempPool;
    static CDecalPool m_permPool;
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

/* the tint's switch and its time left (0x4B4A68, 0x4B4A78: 0x3B7170 steps the tints while the switch
 * is up and the time runs) */
extern "C" unsigned char data_004B4A68;
extern "C" float data_004B4A78;

/* libvu0's difference (MAIN 0x1BFE38; libvu0.h does not declare it) */
extern "C" void sceVu0SubVector(sceVu0FVECTOR v0, sceVu0FVECTOR v1, sceVu0FVECTOR v2);

/* a 4D point's difference by value through VU0, its result copied out of the temporary as
 * operator* and operator+'s are (zmath.h's CPnt4D has no operator-: this unit's own, inline) */
static inline CPnt4D operator-(const CPnt4D &a, const CPnt4D &b)
{
    CPnt4D r;
    sceVu0SubVector((float *)&r, (float *)&a, (float *)&b);
    return r;
}

/* every channel's tint set toward a colour over a time (0x3B78D0; ours): the colour's rgb to a
 * third, its alpha thrice, kept in $f20-$f23 across the walk; at once (every tint and target the
 * colour) for no time, else each target the colour and each rate the step from the tint over the
 * time. The inverse of the time is a named local: numbered before the scalarised colour's fields it
 * takes $f24, the colour $f20-$f23 (x, w, z, y) -- `* (1.0f / time)` hoisted is a temporary numbered
 * among them (32 instructions of FPRs) */
void CVisual::SetLightTints(PNT4D color, float time)
{
    data_004B4A68 = 1;
    data_004B4A78 = time;
    CPnt4D c;

    c.x = color.x * 0.33f;
    c.y = color.y * 0.33f;
    c.z = color.z * 0.33f;
    c.w = color.w * (1.0f / 0.33f);
    if (time == 0.0f) {
        for (int i = 0; i < 4; i++) {
            m_lightTintTarget[i] = c;
            m_lightTint[i] = c;
        }
    } else {
        float inv = 1.0f / time;

        for (int i = 0; i < 4; i++) {
            m_lightTintTarget[i] = c;
            m_lightTintRate[i] = (m_lightTintTarget[i] - m_lightTint[i]) * inv;
        }
    }
}

/* one channel's tint set toward a colour over a time (0x3B76B0; ours): the colour's rgb to a third,
 * its alpha thrice (the constructor's arguments evaluated right to left: w's product first); at once
 * for no time, else the target the colour and the rate the step from the tint over the time */
void CVisual::SetLightTint(PNT4D color, int i, float time)
{
    data_004B4A68 = 1;
    data_004B4A78 = time;
    CPnt4D c(color.x * 0.33f, color.y * 0.33f, color.z * 0.33f, color.w * (1.0f / 0.33f));

    if (time == 0.0f) {
        m_lightTintTarget[i] = c;
        m_lightTint[i] = c;
    } else {
        m_lightTintTarget[i] = c;
        m_lightTintRate[i] = (m_lightTintTarget[i] - m_lightTint[i]) * (1.0f / time);
    }
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

/* the frame's colour step (0x3B7170; new in SOCOM II, the name ours -- CPipe::RenderWorld calls it
 * with the frame's time): the lens colours multiplied into the lens scale (w summed, plus 0.5), and
 * while the tint switch is up -- kept up only while a target or a tint is off the default (0.33,
 * 0.33, 0.33, 0) -- the tints stepped by their rates over the time, set to their targets once the
 * time runs out. Each later loop has a counter of its own (declared in its `for`): one counter
 * reused across the loops swaps each walk's counter and pointer registers (the flag walks' a1/a2,
 * the step's s0/s2: 35 instructions); the first's `i++` in the copy's index keeps the target's
 * `move a1,zero; addiu a1,a1,1` and its entry test */
void CVisual::UpdateLightTints(float dT)
{
    int i = 0;

    m_lensScale = m_lensColor[i++];
    for (; i < 4; i++) {
        m_lensScale.x *= m_lensColor[i].x;
        m_lensScale.y *= m_lensColor[i].y;
        m_lensScale.z *= m_lensColor[i].z;
        m_lensScale.w += m_lensColor[i].w;
    }
    m_lensScale.w += 0.5f;
    if (data_004B4A68) {
        data_004B4A68 = 0;
        for (int j = 0; j < 4 && !data_004B4A68; j++)
            if (m_lightTintTarget[j].x != 0.33f || m_lightTintTarget[j].y != 0.33f ||
                m_lightTintTarget[j].z != 0.33f || m_lightTintTarget[j].w != 0.0f)
                data_004B4A68 = 1;
        for (int j = 0; j < 4 && !data_004B4A68; j++)
            if (m_lightTint[j].x != 0.33f || m_lightTint[j].y != 0.33f || m_lightTint[j].z != 0.33f ||
                m_lightTint[j].w != 0.0f)
                data_004B4A68 = 1;
        if (data_004B4A68) {
            data_004B4A78 -= dT;
            for (int k = 0; k < 4; k++) {
                CPnt4D rate = m_lightTintRate[k];
                CPnt4D step = rate * dT;

                sceVu0AddVector((float *)&m_lightTint[k], (float *)&m_lightTint[k], (float *)&step);
            }
            if (data_004B4A78 <= 0.0f) {
                data_004B4A78 = 0.0f;
                for (int m = 0; m < 4; m++)
                    m_lightTint[m] = m_lightTintTarget[m];
            }
        }
    }
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
    CPnt4D *l = localLightBuf[m_local_light_cnt];

    l[0].x = pos->x;
    l[0].y = pos->y;
    l[0].z = pos->z;
    l[0].w = light->m_maxRangeSq;
    float min2 = light->m_minRange * light->m_minRange;

    l[1].x = light->m_diffuse.x;
    l[1].y = light->m_diffuse.y;
    l[1].z = light->m_diffuse.z;
    l[1].w = 1.0f / (light->m_maxRangeSq - min2);
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
        CVisual::m_applyLocalLights = !node->m_clutter;
    } else {
        m_lightingEnable = false;
        CVisual::m_applyLocalLights = false;
    }
}

/* a 4D point put at the cursor, the quadword after it returned (ours, inline) */
static inline _word128 *Put4D(_word128 *p, const CPnt4D &v)
{
    *(CPnt4D *)p = v;
    return p + 1;
}

/* the lights' packet's statics (new in SOCOM II, not named): the packets' flavour and the light
 * scale 0x3B6B20 keeps */
extern "C" unsigned int data_004B4EB0;
extern "C" float data_004B4A90;
extern "C" _word128 *data_004B4AD8;
extern "C" _word128 *data_004B4AE0;
extern "C" _word128 *func_003B6870(_word128 *q, CNode *node);

/* a node's lights sent (new in SOCOM II, not named: CPipe's renderers call it with the node, 1 and
 * 1.0): the flavour and the scale kept, a packet begun as 0x3B6820's (its stores in another order),
 * 0x3B6870's header; while lit, the light scale from the scene's level (or the head's first word
 * cleared) and, with the local lights on, each local light's two quadwords put in and their count
 * at the head's second word; then VuExecuteState0 and the packet kicked. Each quadword is a
 * CPnt4D temporary built from the light (its four loads, then four stores through its address)
 * and handed to Put4D by reference (the copy out a field at a time, the temporary's field
 * addresses hoisted out of the loop), the cursor threaded through Put4D's return (two +16 steps,
 * not one +32), the light walked by a pointer set up ahead of the loop */
extern "C" void func_003B6B20(CNode *node, int n, float scale)
{
    data_004B4EB0 = n;
    data_004B4A90 = scale;

    _word128 *p = (_word128 *)zSysSprGetPacket((u_long128 **)&CSPR::m_sprPacketEnd);

    CSPR::m_sprPacket = p;
    data_004B4AE0 = p;
    data_004B4AD8 = p + 1;
    data_004B4AD8->u128 = 0;
    p = func_003B6870(data_004B4AD8 + 1, node);

    bool lit = CVisual::m_lightingEnable;

    if (lit && CVisual::m_customGlobalLightIdx != -1)
        m_GlobalLightScale__Q23zdb7CVisual = (1.0f / 127.0f) * CVisual::m_customGlobalLightIdx;
    else
        data_004B4AD8->u32[0] = 0;
    if (lit && CVisual::m_applyLocalLights && CVisual::m_local_light_cnt) {
        CPnt4D *l = CVisual::localLightBuf[0];

        for (unsigned int i = 0; i < (unsigned int)CVisual::m_local_light_cnt; i++) {
            p = Put4D(p, CPnt4D(l[0]));
            p = Put4D(p, CPnt4D(l[1]));
            l += 2;
        }
        data_004B4AD8->u32[1] = CVisual::m_local_light_cnt * 2;
    }
    CSPR::Kick(CVisual::VuExecuteState0(p), true);
}

/* the camera's distance to a point (zcamera.h's CCamera::GetZ: its _word128 clashes with
 * zsys_public.h's here, so the member is declared by its mangled name) and the header's statics:
 * the far tints' fade switch (0x4B4A70), the light scale 0x3B6B20 keeps (0x4B4A90), the packet's
 * head cursor (0x4B4AD8) */
extern "C" float GetZ__Q23zdb7CCameraFP6CPnt3D(CCamera *camera, CPnt3D *pos);
extern "C" unsigned char data_004B4A70;
extern "C" float data_004B4A90;
extern "C" _word128 *data_004B4AD8;

/* the packet's head flag the lights set (ours): bit 0 of the head's word 3 */
struct VU_HEAD_LIGHTS {
    unsigned char _0[12];
    unsigned char m_lights : 1;
    unsigned char m_b1 : 7;
};

/* the object header's lights (0x3B6870; new in SOCOM II, not named -- SOCOM 1's VuObjectSetup's
 * second half): the camera's seven light points into model space at q[4], the model-to-world matrix
 * at q[0]; while the tint switch is up, the node's tint channel (its 2-bit mode) at q[12] -- channels
 * 2 and 3 faded from channel 0 by the camera's distance (all of it within 500, none past 800) while
 * 0x4B4A70 is up; the added colour and the light scale at q[11], the packet's head flagged; the
 * quadword after the header returned */
extern "C" _word128 *func_003B6870(_word128 *q, CNode *node)
{
    CVisual::m_WorldToModel.Transform((CPnt4D *)((char *)CVisual::m_camera + 0x400), (CPnt4D *)&q[4], 7);
    sceVu0CopyMatrix((float (*)[4])q, data_004B4E60->m_matrix);
    if (data_004B4A68) {
        int ch = node->m_socom2_mode;

        if (ch < 2) {
            *(CPnt4D *)&q[12] = CVisual::m_lightTint[ch];
        } else if (data_004B4A70) {
            float z = GetZ__Q23zdb7CCameraFP6CPnt3D(CVisual::m_camera, (CPnt3D *)data_004B4E60->m_matrix[3]);
            float f;

            if (!(z < 800.0f))
                f = 0.0f;
            else if (z <= 500.0f)
                f = 1.0f;
            else
                f = (1.0f / 300.0f) * (800.0f - z);
            *(CPnt4D *)&q[12] = CVisual::m_lightTint[0] + (CVisual::m_lightTint[ch] - CVisual::m_lightTint[0]) * f;
        } else {
            *(CPnt4D *)&q[12] = CVisual::m_lightTint[ch];
        }
    }
    VecSet(q[11].f32, CVisual::m_addColor.x, CVisual::m_addColor.y, CVisual::m_addColor.z, data_004B4A90);
    data_004B4AD8->u32[3] = 0;
    ((VU_HEAD_LIGHTS *)data_004B4AD8)->m_lights = true;
    return q + 13;
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
            fog = (CPnt4D *)&NoAltFogBottomV;
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

/* the detail layers' switch (SOCOM II's; VuUpdate reads it too: not named) */
extern "C" unsigned char data_004B4ED8;

/* the visual's VU program (SOCOM 1's VuBatchProgram, 0x30BDA0 in the demo: the same command tags in
 * the same three branches by the word at 0x1C -- 3 with the scratchpad's end set 57 quadwords on,
 * the opening 82/102 or 112/104 by the type and flag bit 17, 6 for bit 3, then by 0x4B4EB0 the extra
 * pass 100, the fog, lit, local-light and closing commands with the map passes after them; else 72,
 * 4, 12 ... 44, 78 -- and 66 last; SOCOM II counts the VU contexts in 0x4B4AF0, reads the lit bit
 * with the lighting switch, and passes a flag where SOCOM 1 took the object header and tested a
 * static: VuUpdate's four calls hand it whether any light, shadow, specular or projected map is on,
 * and use the result): the command count stored at the head's third word, the packet closed */
_word128 *CVisual::VuBatchProgram(_word128 *p, bool multipass, tag_DETAIL_PARAMS *detail)
{
    _word128 *start = p;
    bool extra = detail && data_004B4ED8 || multipass;
    bool lit = m_lightingEnable && m_bits.m_b4;

    if (m_1c == 3) {
        data_004B4AF0 = 0;
        data_004B4AE8 = p + 57;
        if (m_utype == 1) {
            p = VuCommand(p, 82, 0, 1);
            p = VuCommand(p, 102, 0, 1);
            data_004B4AF0 += 2;
        } else {
            if (m_bits.m_b17)
                p = VuCommand(p, 112, 0, 1);
            else
                p = VuCommand(p, 104, 0, 1);
            data_004B4AF0++;
        }
        if (m_flags & 8) {
            p = VuCommand(p, 6, 0, 1);
            data_004B4AF0++;
        }
        if (data_004B4EB0) {
            if (extra) {
                p = VuCommand(p, 100, 0, 1);
                data_004B4AF0++;
            }
            p = VuCommand(p, 8, 0, 1);
            if (m_fogEnable)
                p = VuCommand(p, 16, data_004B4AF0, 1);
            if (lit) {
                p = VuCommand(p, 84, data_004B4AF0, 1);
                if (m_applyLocalLights && m_local_light_cnt)
                    p = VuCommand(p, 32, data_004B4AF0, m_local_light_cnt);
                p = VuCommand(p, 24, data_004B4AF0, 1);
            }
            if (data_004B4A68)
                p = VuCommand(p, 92, data_004B4AF0, 1);
            p = VuCommand(p, 40, data_004B4AF0, 1);
            if (multipass || detail)
                p = VuMapPasses(p, detail);
        } else {
            p = VuCommand(p, 2, 0, 1);
            data_004B4AF0++;
            p = VuCommand(p, 10, data_004B4AF0, 1);
            if (extra)
                p = VuCommand(p, 100, 0, 1);
            if (m_fogEnable)
                p = VuCommand(p, 18, data_004B4AF0, 1);
            if (lit) {
                p = VuCommand(p, 86, data_004B4AF0, 1);
                if (m_applyLocalLights && m_local_light_cnt)
                    p = VuCommand(p, 34, data_004B4AF0, m_local_light_cnt);
                p = VuCommand(p, 26, data_004B4AF0, 1);
            }
            if (data_004B4A68)
                p = VuCommand(p, 94, data_004B4AF0, 1);
            p = VuCommand(p, 42, data_004B4AF0, 1);
            if (multipass || detail)
                p = VuMapPasses(p, detail);
            p = VuCommand(p, 76, data_004B4AF0, 1);
        }
    } else {
        data_004B4AF0 = 0;
        p = VuCommand(p, 72, 0, 1);
        data_004B4AF0++;
        p = VuCommand(p, 4, 0, 1);
        data_004B4AF0++;
        p = VuCommand(p, 12, data_004B4AF0, 1);
        if (m_fogEnable)
            p = VuCommand(p, 20, data_004B4AF0, 1);
        if (lit) {
            p = VuCommand(p, 88, data_004B4AF0, 1);
            p = VuCommand(p, 28, data_004B4AF0, 1);
        }
        if (data_004B4A68)
            p = VuCommand(p, 96, data_004B4AF0, 1);
        p = VuCommand(p, 44, data_004B4AF0, 1);
        p = VuCommand(p, 78, data_004B4AF0, 1);
    }
    p = VuCommand(p, 66, data_004B4AF0, 1);
    data_004B4AD8->u32[2] = p - start;
    return VuExecuteState0(p);
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
 * bool local (its sltu). Byte-exact since the MSL headers round 24 adopted the image's pointer
 * iterator (end() through the raw end and the pod's end): open until then 5 instructions off on
 * the loop test's end() -- the size and the iterator's word coloured the other way round, which
 * no spelling of the walk moved (end() != it, !(it == end()), ++it, a while loop) */
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

/* the decals' VU program (SOCOM 1's VuBatchProgram_Decals, 0x30C9C0 in the demo: the same command
 * tags in the same order -- 82 and 102 or 70 by the visual's type, 6 for flag bit 3, then by the
 * map statics' switch the lit, fog, local-light and closing commands -- SOCOM II's counting the VU
 * contexts in 0x4B4AF0 and reading the lit bit with the lighting switch; SOCOM II drops the object
 * header SOCOM 1 took: VuUpdate's two calls pass the visual and the packet and keep the result): the
 * command count stored at the head's third word, the packet closed */
_word128 *CVisual::VuBatchProgram_Decals(_word128 *p)
{
    bool lit = m_lightingEnable && m_bits.m_b4;

    data_004B4AF0 = 0;
    _word128 *start = p;

    if (m_utype == 1) {
        p = VuCommand(p, 82, 0, 1);
        p = VuCommand(p, 102, 0, 1);
        data_004B4AF0 += 2;
    } else {
        p = VuCommand(p, 70, 0, 1);
        data_004B4AF0++;
    }
    if (m_flags & 8) {
        p = VuCommand(p, 6, 0, 1);
        data_004B4AF0++;
    }
    if (data_004B4EB0) {
        p = VuCommand(p, 8, 0, 1);
        if (m_fogEnable)
            p = VuCommand(p, 16, data_004B4AF0, 1);
        if (lit) {
            p = VuCommand(p, 84, data_004B4AF0, 1);
            if (m_applyLocalLights && m_local_light_cnt)
                p = VuCommand(p, 32, data_004B4AF0, m_local_light_cnt);
            p = VuCommand(p, 24, data_004B4AF0, 1);
        }
        if (data_004B4A68)
            p = VuCommand(p, 92, data_004B4AF0, 1);
        p = VuCommand(p, 40, data_004B4AF0, 1);
    } else {
        p = VuCommand(p, 2, 0, 1);
        data_004B4AF0++;
        p = VuCommand(p, 10, data_004B4AF0, 1);
        if (m_fogEnable)
            p = VuCommand(p, 18, data_004B4AF0, 1);
        if (lit) {
            p = VuCommand(p, 86, data_004B4AF0, 1);
            if (m_applyLocalLights && m_local_light_cnt)
                p = VuCommand(p, 34, data_004B4AF0, m_local_light_cnt);
            p = VuCommand(p, 26, data_004B4AF0, 1);
        }
        if (data_004B4A68)
            p = VuCommand(p, 94, data_004B4AF0, 1);
        p = VuCommand(p, 42, data_004B4AF0, 1);
        p = VuCommand(p, 76, data_004B4AF0, 1);
    }
    p = VuCommand(p, 66, data_004B4AF0, 1);
    data_004B4AD8->u32[2] = p - start;
    return VuExecuteState0(p);
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
 * of n/m/inv/vid and the id as short/int/unsigned all give the same two (build/scratch/wep/v_adr*.py).
 * Round 24 (CTurret::Fire's no-op-conversion close does not carry over): the direct call over the
 * rows or the CMatrix casts, a source or a destination pointer local, void-pointer casts, the id as
 * an if/else or a zero then an if -- the same two (or 4 short). Round 31 (the HUD's round-55 call
 * arguments and the no-op-conversion rule): the copy through CMatrix's conversion operators on
 * either or both arguments, the source cast straight from the CMatrix pointer (also through void *
 * and unsigned int), local inline copies taking (dst, src), (src, dst), references or the rows --
 * the same two (build/scratch/wep/v_adr*.py) */
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

/* a decal set up (vis_decal.cpp's 0x3BECA0, not written: SOCOM 1's CDecal::AddDecal, SOCOM II's
 * taking the flags, the texture, the points and the new word, then the visual and the view) */
extern "C" void func_003BECA0(CDecal *d, float size, unsigned int flags, CTexHandle *tex,
                              AddonPoint **points, int extra, CVisual *visual, unsigned int vid);

/* a decal taken from the permanent pool (a negative size) or the temporary one and set up, then
 * appended to the view's list -- whose end is found first (SOCOM 1's, SOCOM II finding the end
 * before taking the decal, the decal's link cleared before it is set up). The end's walk is a
 * `while (last)` with the link's test a break (the target's entry test, the link's test and the
 * bottom `bnez` on the new end); the set-up's float comes first in its prototype (the target
 * moves the size into f12 first, in the null test's delay slot); the list is a reference to the
 * pod's data_ (its address spilled to sp+176, reloaded for the store).
 * BYTE-EXACT since the weapons' round 31: each pool's Get through an inline of its own (GetPermDecal /
 * GetTempDecal, CDecal's statics beside the Put pair; a free inline per pool, or one taking the pool
 * by reference, match as well; one inline holding the whole if/else stays 0x144) -- an inlined call
 * in each arm keeps the arms' first instructions where the target has them. Before:
 * OPEN, 8 bytes short (written in the weapons' round 25): the pool's branch -- the target fills
 * the bc1f's delay slot from the fall-through (the permanent pool's lui) and keeps the temporary
 * arm's own lui, its `li a1,1` in the call's delay slot, then pads the join; here MW hoists the
 * temporary arm's lui into the slot. A ternary, the arms inverted (`!(size < 0)`, `size >= 0`),
 * int arguments, pool pointer locals, a bool variable and the decal declared at the top all give
 * the same 0x144 */
void CVisual::AttachDecal(unsigned int vid, float size, unsigned int flags, AddonPoint **points,
                          CTexHandle *tex, int extra)
{
    unsigned int *&list = m_24.data();
    CDecal *last = (CDecal *)list[vid];

    while (last) {
        CDecal *next = last->m_next;

        if (!next)
            break;
        last = next;
    }

    CDecal *d;

    if (size < 0.0f)
        d = CDecal::GetPermDecal();
    else
        d = CDecal::GetTempDecal();
    if (d) {
        d->m_next = 0;
        func_003BECA0(d, size, flags, tex, points, extra, this, vid);
        if (last)
            last->m_next = d;
        else
            list[vid] = (unsigned int)d;
        FlushCache(0);
    }
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
