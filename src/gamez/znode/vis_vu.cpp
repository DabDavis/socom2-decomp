/* vis_vu.cpp -- zdb::CVisual's decals and VU code: OVL1 0x3B36F0?-0x3B7FDF, after zvid_init.cpp's
 * stretch and before vis_mesh.cpp, holding what SOCOM 1's vis_main.cpp held ahead of CVisual's
 * reading (DereferenceDecals .. Init, 0x3092B0-0x30E2FF in the demo) -- SOCOM II links this half
 * apart from the reading and texture selects (vis_main.cpp here, 0x3C3570-), and which half kept
 * the name is not settled: the file name is ours. Only some of its functions so far; the rest stay
 * placeholders. Defined in reverse of their image order (-inline deferred). */

#include "znode.h"
#include "../zsys/zsys_public.h"

namespace zdb {

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

/* a level of detail's band of squared distances: drawn between the near minimum and the far
 * maximum, faded in up to the far minimum and out past the near maximum (SOCOM 1's DWARF, 0x20
 * bytes) */
class CLOD_band {
public:
    float m_minRangeNearSq;   // 0x0
    float m_minRangeFarSq;   // 0x4
    float m_minInvDeltaRangeSq;   // 0x8
    float m_maxRangeNearSq;   // 0xc
    float m_maxRangeFarSq;   // 0x10
    float m_maxInvDeltaRangeSq;   // 0x14
    unsigned int m_minFade : 1;   // 0x18
    unsigned int m_maxFade : 1;
    unsigned int m_unused : 30;
    unsigned int m_padding;   // 0x1c
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
