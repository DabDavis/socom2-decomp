/* zcam_update.cpp -- zdb::CCamera's per-frame update: the directional fog's colour, the altitude
 * fog's VU vectors, the update itself and the fog visibility test (OVL1 0x293EC0-0x294A2C). The unit
 * name is SOCOM 1's (its DWARF puts UpdateDirectionalFog, Update and GetFogVisibility here; SOCOM II
 * adds the VU vectors between the first two). Defined in reverse of their image order
 * (-inline deferred). */

#include "zcamera.h"

extern "C" double fabs(double);

/* SOCOM II's: the screen's horizontal shift from its aspect, for whoever reads it (0x4B4EB8,
 * not named yet) */
extern int data_004B4EB8;

namespace zdb {

/* the camera's per-frame update: the frustum (a normal camera's), the VU's frame block -- its
 * tags, the view, clip and screen matrices, the field of view's clip matrix -- the clip planes in
 * the world, and the fog. SOCOM 1's, with SOCOM II's changes: the clip matrices start a twentieth
 * of the near plane out, the field of view's clip widens with the screen offset (in double
 * precision, by fabs) and comes from the render size, not constants, and the altitude fog's VU
 * vectors and the screen shift follow. SOCOM 1's locals.
 * OPEN (0x294070): 13 instructions, all stack offsets -- the near plane point's VU0 temporaries
 * sit in the target as [the three results, then the three copies], each group in source order;
 * here the addition's pair comes first in each group. One expression (no deltaNear) gives the
 * target's order but calls CPnt4D's constructors out of line (the inline levels run out) */
void CCamera::Update(tag_ZCAM_TYPE type)
{
    float nearClip;
    tag_ZCAM_MTX_SET *pSet;
    float maxZ;
    float oneOverRange;
    float mtxViewToFovClip[4][4];
    float tmp_hfov;

    m_type = type;
    nearClip = 0.05f * m_near_plane;
    if (type == ZCAM_NORMAL)
        UpdateFrustum();
    m_worldPos = CPnt3D::zero;
    TransformToWorld(m_worldPos);

    ((unsigned int *)&m_mtxSet.m_vu->dmaTag)[0] = 0x10000018;
    ((unsigned int *)&m_mtxSet.m_vu->dmaTag)[1] = 0;
    ((unsigned int *)&m_mtxSet.m_vu->dmaTag)[2] = 0;
    ((unsigned int *)&m_mtxSet.m_vu->dmaTag)[3] = 0;
    ((unsigned int *)&m_mtxSet.m_vu->vifTag)[0] = 0;
    ((unsigned int *)&m_mtxSet.m_vu->vifTag)[1] = 0x11000000;
    ((unsigned int *)&m_mtxSet.m_vu->vifTag)[2] = 0x01000101;
    ((unsigned int *)&m_mtxSet.m_vu->vifTag)[3] = 0x6C170004;
    m_mtxSet.m_vu->shadowGifTag = 0x3035C00000008000UL;
    ((unsigned long *)&m_mtxSet.m_vu->shadowGifTag)[1] = 0x412;

    pSet = &m_mtxSet;
    maxZ = m_landmark_far_plane;
    sceVu0ViewScreenMatrix(pSet->mtxViewToScreen, m_scrZ, m_screenAspect[0], m_screenAspect[1],
                           m_screenCenter[0] + m_screenOffset[0], m_screenCenter[1] + m_screenOffset[1],
                           m_Zmin, m_Zmax, m_near_plane, maxZ);

    oneOverRange = 1.0f / (maxZ - m_near_plane);
    sceVu0UnitMatrix(pSet->mtxViewToClip);
    pSet->mtxViewToClip[0][0] = m_screenAspect[0] * m_scrZ / m_vecClipVolume[0];
    pSet->mtxViewToClip[1][1] = m_screenAspect[1] * m_scrZ / m_vecClipVolume[0];
    pSet->mtxViewToClip[2][2] = oneOverRange * (maxZ + m_near_plane);
    pSet->mtxViewToClip[2][3] = 1.0f;
    pSet->mtxViewToClip[3][2] = oneOverRange * (-2.0f * maxZ * m_near_plane);
    pSet->mtxViewToClip[3][3] = 0.0f;

    oneOverRange = 1.0f / (m_mid_plane - nearClip);
    sceVu0UnitMatrix(mtxViewToFovClip);
    mtxViewToFovClip[0][0] = m_screenAspect[0] * m_scrZ / (m_vecClipVolume[0] + fabs(m_screenOffset[0]));
    mtxViewToFovClip[1][1] = m_screenAspect[1] * m_scrZ / (m_vecClipVolume[1] + fabs(m_screenOffset[1]));
    mtxViewToFovClip[2][2] = oneOverRange * (m_mid_plane + nearClip);
    mtxViewToFovClip[2][3] = 1.0f;
    mtxViewToFovClip[3][2] = oneOverRange * (-2.0f * m_mid_plane * nearClip);
    mtxViewToFovClip[3][3] = 0.0f;

    sceVu0UnitMatrix(pSet->m_vu->mtxClipToScreen);
    pSet->m_vu->mtxClipToScreen[0][0] = m_screenAspect[0] * m_vecClipVolume[0];
    pSet->m_vu->mtxClipToScreen[1][1] = m_screenAspect[1] * m_vecClipVolume[1];
    pSet->m_vu->mtxClipToScreen[2][2] = -0.5f * (m_Zmax - m_Zmin);
    pSet->m_vu->mtxClipToScreen[3][0] = m_screenCenter[0] + m_screenOffset[0];
    pSet->m_vu->mtxClipToScreen[3][1] = m_screenCenter[1] + m_screenOffset[1];
    pSet->m_vu->mtxClipToScreen[3][2] = 0.5f * (m_Zmax + m_Zmin);
    pSet->m_vu->mtxClipToScreen[3][3] = 1.0f;

    sceVu0CopyVector(pSet->mtxWorldToView[0], m_matrix.m_matrix[0]);
    sceVu0ScaleVector(pSet->mtxWorldToView[1], m_matrix.m_matrix[1], -1.0f);
    sceVu0ScaleVector(pSet->mtxWorldToView[2], m_matrix.m_matrix[2], -1.0f);
    pSet->mtxWorldToView[3][0] = m_matrix(3, 0);
    pSet->mtxWorldToView[3][1] = m_matrix(3, 1);
    pSet->mtxWorldToView[3][2] = m_matrix(3, 2);
    pSet->mtxWorldToView[3][3] = 1.0f;
    sceVu0InversMatrix(pSet->mtxWorldToView, pSet->mtxWorldToView);
    sceVu0MulMatrix(pSet->m_vu->mtxWorldToClip, pSet->mtxViewToClip, pSet->mtxWorldToView);
    sceVu0MulMatrix(pSet->m_vu->mtxWorldToScreen, pSet->mtxViewToScreen, pSet->mtxWorldToView);
    sceVu0MulMatrix(pSet->mtxWorldToClip, mtxViewToFovClip, pSet->mtxWorldToView);

    m_ClipSet.LeftPlaneNormal = CPnt4D(1.0f, 0.0f, 0.0f, 0.0f);
    m_ClipSet.TopPlaneNormal = CPnt4D(0.0f, -1.0f, 0.0f, 0.0f);
    m_ClipSet.NearPlaneNormal = CPnt4D(0.0f, 0.0f, -1.0f, 0.0f);
    tmp_hfov = 0.5f * zVid.renderWidth;
    tmp_hfov = (tmp_hfov + fabs(m_screenOffset[0])) / (m_screenAspect[0] * m_scrZ);
    m_ClipSet.LeftPlaneNormal.Rotate_AxisY(tmp_hfov);
    m_ClipSet.RightPlaneNormal.x = -m_ClipSet.LeftPlaneNormal.x;
    m_ClipSet.RightPlaneNormal.y = m_ClipSet.LeftPlaneNormal.y;
    m_ClipSet.RightPlaneNormal.z = m_ClipSet.LeftPlaneNormal.z;
    m_ClipSet.RightPlaneNormal.w = m_ClipSet.LeftPlaneNormal.w;
    tmp_hfov = 0.5f * zVid.renderHeight;
    tmp_hfov = (tmp_hfov + fabs(m_screenOffset[1])) / (m_screenAspect[1] * m_scrZ);
    m_ClipSet.TopPlaneNormal.Rotate_AxisX(tmp_hfov);
    m_ClipSet.BottomPlaneNormal.x = m_ClipSet.TopPlaneNormal.x;
    m_ClipSet.BottomPlaneNormal.y = -m_ClipSet.TopPlaneNormal.y;
    m_ClipSet.BottomPlaneNormal.z = m_ClipSet.TopPlaneNormal.z;
    m_ClipSet.BottomPlaneNormal.w = m_ClipSet.TopPlaneNormal.w;
    sceVu0ApplyMatrix((float *)&m_ClipSet.LeftPlaneNormal, m_matrix.m_matrix, (float *)&m_ClipSet.LeftPlaneNormal);
    sceVu0ApplyMatrix((float *)&m_ClipSet.RightPlaneNormal, m_matrix.m_matrix, (float *)&m_ClipSet.RightPlaneNormal);
    sceVu0ApplyMatrix((float *)&m_ClipSet.TopPlaneNormal, m_matrix.m_matrix, (float *)&m_ClipSet.TopPlaneNormal);
    sceVu0ApplyMatrix((float *)&m_ClipSet.BottomPlaneNormal, m_matrix.m_matrix, (float *)&m_ClipSet.BottomPlaneNormal);
    sceVu0ApplyMatrix((float *)&m_ClipSet.NearPlaneNormal, m_matrix.m_matrix, (float *)&m_ClipSet.NearPlaneNormal);
    m_ClipSet.ViewPlanePoint = CPnt4D(m_matrix(3, 0), m_matrix(3, 1), m_matrix(3, 2), 1.0f);
    CPnt4D deltaNear = m_ClipSet.NearPlaneNormal * nearClip * 1.0001f;
    m_ClipSet.NearPlanePoint = m_ClipSet.ViewPlanePoint + deltaNear;

    m_fogB = -255.0f / (m_fog_far - m_fog_near);
    m_fogA = 255.0f - m_fog_near * m_fogB;
    pSet->m_vu->fogA[0] = 0.0f;
    pSet->m_vu->fogA[1] = 0.0f;
    pSet->m_vu->fogA[2] = 0.0f;
    pSet->m_vu->fogA[3] = m_fogA;
    pSet->m_vu->fogB[0] = 0.0f;
    pSet->m_vu->fogB[1] = 0.0f;
    pSet->m_vu->fogB[2] = 0.0f;
    pSet->m_vu->fogB[3] = m_fogB;
    if (m_fog_alt_enabled)
        UpdateFogAlt();
    UpdateFogAltVU();
    if (m_directional_fog_enabled)
        UpdateDirectionalFog();
    if (1.0f == m_screenAspect[0])
        data_004B4EB8 = 0;
    else
        data_004B4EB8 = (int)(-3.0f * m_screenAspect[0]);
}

/* how far the fog lets p2 be seen from p1, 0 to 1: the fog's A + B times their distance, held to
 * 0..255, scaled by how far the lower of them stands into the altitude fog. SOCOM 1's locals */
float CCamera::GetFogVisibility(const CPnt3D *p1, const CPnt3D *p2) const
{
    CPnt3D vec = *p2 - *p1;
    float vis = m_fogA + m_fogB * vec.Length();
    float wy;

    vis = MAX(0.0f, vis);
    vis = MIN(255.0f, vis);
    if (m_fog_alt_enabled) {
        if (p2->y < p1->y)
            wy = p2->y;
        else
            wy = p1->y;
        wy -= m_fog_bottom;
        wy *= m_fog_alt[1].y;
        wy = MAX(0.0f, wy);
        wy = MIN(1.0f, wy);
        vis *= wy;
    }
    return (1.0f / 255.0f) * vis;
}

/* the altitude fog's vectors as the VU takes them -- the bottom and 1 / the depth, the fog's A and
 * B in w -- twice over, the altitude pushed out of reach when the altitude fog is off (SOCOM II's;
 * Update and the constructor call it; the name is ours) */
void CCamera::UpdateFogAltVU()
{
    m_vuFogAlt[0] = m_fog_alt[0];
    m_vuFogAlt[1] = m_fog_alt[1];
    m_vuLandmarkFogAlt[0] = m_fog_alt[0];
    m_vuLandmarkFogAlt[1] = m_fog_alt[1];
    if (!m_fog_alt_enabled) {
        m_vuFogAlt[0].y = -10000.0f;
        m_vuFogAlt[1].y = 0.001f;
        m_vuLandmarkFogAlt[0].y = -10000.0f;
        m_vuLandmarkFogAlt[1].y = 0.001f;
    }
    m_vuFogAlt[0].w = m_fogA;
    m_vuFogAlt[1].w = m_fogB;
    m_vuLandmarkFogAlt[0].w = m_fogA;
    m_vuLandmarkFogAlt[1].w = m_fogB;
}

/* the fog's colour from the view direction: the +/-x and +/-z colours the direction faces,
 * weighted by its x and z over their sum. SOCOM 1's locals */
void CCamera::UpdateDirectionalFog()
{
    CPnt3D *Cz;
    CPnt3D *Cx;
    float delta;
    float wx;
    float wz;

    wz = m_matrix.m_matrix[2][2];
    wx = m_matrix.m_matrix[2][0];
    if (wz >= 0.0f) {
        Cz = &m_pz_fog_color;
        delta = wz;
    } else {
        Cz = &m_nz_fog_color;
        delta = wz = -wz;
    }
    if (wx >= 0.0f) {
        delta += wx;
        Cx = &m_px_fog_color;
    } else {
        Cx = &m_nx_fog_color;
        delta -= wx;
        wx = -wx;
    }
    if (delta != 0.0f)
        delta = 1.0f / delta;
    wx *= delta;
    wz *= delta;
    m_fog_color.x = wx * Cx->x + wz * Cz->x;
    m_fog_color.y = wx * Cx->y + wz * Cz->y;
    m_fog_color.z = wx * Cx->z + wz * Cz->z;
}

}
