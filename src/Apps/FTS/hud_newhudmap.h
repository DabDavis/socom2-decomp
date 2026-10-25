/* The HUD's map (CZNewHudMap) and its clipping circle, as far as the map's own small functions and
 * the map items' Ticks reach into them. SOCOM 1's DWARF layout to the matrix, 4 lower (C2D's);
 * after it SOCOM II's order (see hud_newhudmap.cpp). The header's name is ours. */
#ifndef HUD_NEWHUDMAP_H
#define HUD_NEWHUDMAP_H

#include "../../gamez/ztwod/ztwod.h"
#include "hud_mapitem.h"

class CZSealBody;

namespace zdb {
class CWorld;
}

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

class CZClaymoreMapItem;
class CZMPBombMapItem;
class CZProjectile;

class CZNewHudMap : public C2D {
public:
    CZNewHudMap();
    CPnt3D MapPoint(const CPnt3D *p) const;
    CZClaymoreMapItem *FindClaymoreItem(CZProjectile *projectile);
    CZMPBombMapItem *FindMPBombItem();
    void UpdateMPBomb();
    void UpdateExplosives();
    float CalcDrawPos(float y) const;
    void SetWorldWidth(float width);
    void SetMapScreenPos(float x1, float y1, float x2, float y2);
    void ReInit();
    void UninitMap();
    void InitMap(zdb::CWorld *world);
    float GetZoomOut() const;
    void SwitchToCompassMap();
    void CalcNewMatrix();
    /* the world window, its width (no narrower than the minimum) and centre at once: the width's
     * setter passes the centre back in, read before the clamp (the name is ours) */
    void SetWorldView(float width, float cx, float cy);

    std::list<CZMapItem *> m_itemlist;   // 0x18
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
    unsigned char _dd[0x830 - 0xdd];
};   // 0x830 bytes (CHUD's constructor news it)

#endif
