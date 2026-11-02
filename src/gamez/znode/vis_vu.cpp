/* vis_vu.cpp -- zdb::CVisual's decals and VU code: OVL1 0x3B36F0?-0x3B7FDF, after zvid_init.cpp's
 * stretch and before vis_mesh.cpp, holding what SOCOM 1's vis_main.cpp held ahead of CVisual's
 * reading (DereferenceDecals .. Init, 0x3092B0-0x30E2FF in the demo) -- SOCOM II links this half
 * apart from the reading and texture selects (vis_main.cpp here, 0x3C3570-), and which half kept
 * the name is not settled: the file name is ours. Only some of its functions so far; the rest stay
 * placeholders. Defined in reverse of their image order (-inline deferred). */

#include "znode.h"

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
