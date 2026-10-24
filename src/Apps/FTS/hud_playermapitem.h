/* A seal on the HUD's map: its blip and view cone, clipped to the map's circle, and its name
 * (CZPlayerMapItem). SOCOM 1's DWARF layout moved for SOCOM II: the tri-fans are 4 bytes shorter
 * each (C2D's), so everything after them sits 0x10 lower; the statics are SOCOM 1's. The header's
 * name is ours. */
#ifndef HUD_PLAYERMAPITEM_H
#define HUD_PLAYERMAPITEM_H

#include "../../gamez/ztwod/ztwod.h"
#include "hud_mapitem.h"

class CEntity;
class CZSealBody;

class CZPlayerMapItem : public CZMapItem {
public:
    CZPlayerMapItem(CEntity *entity);
    virtual bool Tick(CZNewHudMap *map, float dT);
    virtual void Draw(zdb::CCamera *camera);
    virtual void PostDraw(zdb::CCamera *camera);
    virtual void Clip(CZNewHudMap *map);
    void SetViewCone(CZNewHudMap *map, const CPnt3D *dir, const CPnt3D *pos, unsigned int color,
                     float length, float width);
    void TickSealCone(CZNewHudMap *map);
    unsigned int GetBlipID(CZSealBody *viewer);
    static void DrawAlphaBravo(zdb::CCamera *camera);
    static void MissionEndCleanup();
    static void UnInitStaticItems();
    static void InitStaticItems();

    CEntity *m_entity;   // 0x4
    C2DBitmapPoly m_blip;   // 0x10
    C2DBitmapPoly m_cone;   // 0x200
    C2DTriFan m_clippedblip[2];   // 0x3f0
    C2DTriFan m_clippedcone[2];   // 0x440
    bool m_wasalive;   // 0x490
    zdb::CTexture *m_myconetex;   // 0x494
    zdb::CTexture *m_mybliptex;   // 0x498
    CPnt3D m_oldrot;   // 0x49c
    CPnt3D m_oldpos;   // 0x4a8
    float m_fadeval;   // 0x4b4
    bool m_fadein;   // 0x4b8
    C2DString m_namestring;   // 0x4bc

    static zdb::CTexture *m_conetex;
    static zdb::CTexture *m_alivebliptex;
    static zdb::CTexture *m_deadbliptex;
    static C2DString m_ablestring;
    static C2DString m_bravostring;
};   // 0x560 bytes

#endif
