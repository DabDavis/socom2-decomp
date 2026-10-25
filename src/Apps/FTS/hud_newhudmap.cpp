/* hud_newhudmap.cpp -- the HUD's map (OVL1 0x208FD0-0x20CFxx): SOCOM 1's unit (its DWARF names it,
 * Z:\dev\Apps\FTS\hud_newhudmap.cpp). Only five small functions so far, in SOCOM 1's order.
 * CZNewHudMap's members to its matrix are SOCOM 1's, 4 lower (C2D's); after it SOCOM II
 * reordered them -- the circle clipper, the centre and the full-screen flag right after the matrix,
 * where SOCOM 1 kept its two background bitmaps -- and only those are placed. Defined in reverse
 * of their image order (-inline deferred). */

#include "hud_newhudmap.h"

/* the narrowest world window (a variable: read with lwc1, where MW builds a constant) */
static float s_minWorldWidth = 1300.0f;

inline void CZNewHudMap::SetWorldView(float width, float cx, float cy)
{
    if (width < s_minWorldWidth)
        width = s_minWorldWidth;
    m_width = width;
    m_worldcenterx = cx;
    m_worldcentery = cy;
}

float CZNewHudMap::GetZoomOut() const
{
    return 0.000454545458f * m_width;   /* 1/2200 */
}

void CZNewHudMap::ReInit()
{
    if (m_isFullScreen)
        SwitchToCompassMap();
}

/* the map's rectangle on screen and its centre; the compass map's clipping circle follows it */
void CZNewHudMap::SetMapScreenPos(float x1, float y1, float x2, float y2)
{
    m_screenx1 = x1;
    m_screeny1 = y1;
    m_screenx2 = x2;
    m_screeny2 = y2;
    m_screencenterx = x1 + (x2 - x1) / 2.0f;
    m_screencentery = y1 + (y2 - y1) / 2.0f;
    if (!m_isFullScreen)
        m_circleclipper->Set(m_screencenterx + C2D::m_topx, m_screencentery + C2D::m_topy,
                             m_screencentery - m_screeny1);
}

void CZNewHudMap::SetWorldWidth(float width)
{
    SetWorldView(width, m_worldcenterx, m_worldcentery);
}

float CZNewHudMap::CalcDrawPos(float y) const
{
    return m_screencentery + m_scale * (y - m_screencentery);
}
