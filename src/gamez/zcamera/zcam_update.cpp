/* zcam_update.cpp -- zdb::CCamera's per-frame update: the directional fog's colour, the altitude
 * fog's VU vectors, the update itself and the fog visibility test (OVL1 0x293EC0-0x294A2C). The unit
 * name is SOCOM 1's (its DWARF puts UpdateDirectionalFog, Update and GetFogVisibility here; SOCOM II
 * adds the VU vectors between the first two). Defined in reverse of their image order
 * (-inline deferred). */

#include "zcamera.h"

namespace zdb {

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
