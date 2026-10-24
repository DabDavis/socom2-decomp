/* hud_newhudmap.cpp -- the HUD's map (OVL1 0x208FD0-0x20CFxx): SOCOM 1's unit (its DWARF names it,
 * Z:\dev\Apps\FTS\hud_newhudmap.cpp). Only five small functions so far, in SOCOM 1's order.
 * CZNewHudMap's members to its matrix are SOCOM 1's, 4 lower (C2D's); after it SOCOM II
 * reordered them -- the circle clipper, the centre and the full-screen flag right after the matrix,
 * where SOCOM 1 kept its two background bitmaps -- and only those are placed. Defined in reverse
 * of their image order (-inline deferred). */

#include "../../gamez/ztwod/ztwod.h"

class CZSealBody;

class CCircleClip {
public:
    /* the circle placed at once: its centre and radius computed before any is stored (the name
     * is ours) */
    void Set(float cx, float cy, float radius)
    {
        m_Cx = cx;
        m_Cy = cy;
        m_Radius = radius;
    }

    float m_Cx;   // 0x0
    float m_Cy;   // 0x4
    float m_rCx;   // 0x8
    float m_rCy;   // 0xc
    float m_Radius;   // 0x10
};

class CZNewHudMap : public C2D {
public:
    float CalcDrawPos(float y) const;
    void SetWorldWidth(float width);
    void SetMapScreenPos(float x1, float y1, float x2, float y2);
    void ReInit();
    float GetZoomOut() const;
    void SwitchToCompassMap();
    /* the world window, its width (no narrower than the minimum) and centre at once: the width's
     * setter passes the centre back in, read before the clamp (the name is ours) */
    void SetWorldView(float width, float cx, float cy);

    unsigned char m_itemlist[0xc];   // 0x18: SOCOM 1's `list m_itemlist`
    CZSealBody *m_viewer;   // 0x24
    float m_screenx1;   // 0x28
    float m_screeny1;   // 0x2c
    float m_screenx2;   // 0x30
    float m_screeny2;   // 0x34
    CPnt3D m_world1;   // 0x38
    CPnt3D m_world2;   // 0x44
    CPnt3D m_world3;   // 0x50
    CPnt3D m_world4;   // 0x5c
    float m_screencenterx;   // 0x68
    float m_screencentery;   // 0x6c
    float m_width;   // 0x70
    float m_worldcenterx;   // 0x74
    float m_worldcentery;   // 0x78
    float m_scale;   // 0x7c
    float m_radar_distance;   // 0x80
    CMatrix m_matrix;   // 0x90
    CCircleClip *m_circleclipper;   // 0xd0
    float m_centerx;   // 0xd4
    float m_centerz;   // 0xd8
    bool m_isFullScreen;   // 0xdc
};

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
