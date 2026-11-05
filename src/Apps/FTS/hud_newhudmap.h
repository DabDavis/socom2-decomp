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
    CCircleClip() : m_Cx(0), m_Cy(0), m_Radius(0) {}
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

/* the map's objective list (hud_objectives.cpp defines the class): a stand-in, as far as the map
 * reaches into it -- a C2D (SOCOM 1's DWARF), so the map draws it through its table (`lw t9,236`:
 * the list at 0xE0, its vptr at 0xC), 0x730 bytes (its last member a byte at 0x72C, the class
 * 16-aligned). IsPlayingAnimation is called on the list (a0 its address), as SOCOM 1's. The
 * destructor declared, not defined: implicit, a unit including this would emit a copy of it */
class CMissionObjectiveCam;

class CZObjectiveList : public C2D {
public:
    CZObjectiveList();
    virtual ~CZObjectiveList();
    virtual void Draw(zdb::CCamera *cam);
    bool IsPlayingAnimation();
    CMissionObjectiveCam *GetCurrentObjective() const;
    void MissionEndCleanup();

    unsigned char _18[0x730 - 0x18];
};

class CHUD;
class CSnd;

class CZClaymoreMapItem;
class CZMPBombMapItem;
class CZProjectile;

class CZNewHudMap : public C2D {
public:
    CZNewHudMap();
    /* declared, not defined here: its draft (0x20BA00) inlines the real objective list's destructor,
     * which the stand-in above cannot give -- defined, the unit's implicit copy would replace it */
    virtual ~CZNewHudMap();
    virtual void Draw(zdb::CCamera *cam);
    void PostDrawItems(const std::list<CZMapItem *>::iterator &first, zdb::CCamera *cam);
    CPnt3D MapPoint(const CPnt3D *p) const;
    CZClaymoreMapItem *FindClaymoreItem(CZProjectile *projectile);
    CZMPBombMapItem *FindMPBombItem();
    void UpdateMPBomb();
    void UpdateExplosives();
    float CalcDrawPos(float y) const;
    void SetWorldWidth(float width);
    void SetMapScreenPos(float x1, float y1, float x2, float y2);
    void ReInit();
    CZMapItem *RemoveItem(CZMapItem *item);
    void UninitMap();
    void InitMap(zdb::CWorld *world);
    float GetZoomOut() const;
    void FrameObjective();
    void SwitchToCompassMap();
    /* the full-screen map scrolled by the stick (SOCOM 1's; 0x20A120) */
    void HandleScrolling(float fStickX, float fStickY);
    /* the bounds the full-screen map scrolls inside (an inline; the name ours) */
    const CBBox *GetScrollBounds() const;
    void CalcNewMatrix();
    /* the world window, its centre and width (no narrower than the minimum) at once: the width's
     * setter passes the centre back in, read before the clamp (the name is ours) */
    void SetWorldView(float cx, float cy, float width);

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
    unsigned char m_oldmenustate;   // 0xdd (SOCOM 1's MENU_STATE)
    unsigned char _de[0xe0 - 0xde];
    CZObjectiveList m_objectivelist;   // 0xe0 (SOCOM 1's, at 0x4d0)
    CHUD *m_parent;   // 0x810 (SOCOM 1's last three, after the list as there)
    CSnd *m_zoominsound;   // 0x814
    CSnd *m_zoomoutsound;   // 0x818
    int m_socom2_81c;   // 0x81c: SOCOM II's three, zeroed by the constructor
    int m_socom2_820;   // 0x820
    int m_socom2_824;   // 0x824
    unsigned char _828[0x830 - 0x828];
};   // 0x830 bytes (CHUD's constructor news it)

#endif
