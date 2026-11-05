/* The claymore's item on the HUD's map, for its unit and the map's finders. The header's
 * name is ours. */
#ifndef HUD_CLAYMOREMAPITEM_H
#define HUD_CLAYMOREMAPITEM_H

#include "../../gamez/ztwod/ztwod.h"
#include "hud_mapitem.h"
#include "hud_newhudmap.h"

/* only what the items read: SOCOM 1's frame end position, 4 later in SOCOM II, and its shooter */
class CZProjectile {
public:
    bool WasDetonated();

    unsigned char _0[0x48];
    CPnt3D m_frameendpos;   // 0x48 (SOCOM 1: 0x44)
    unsigned char _54[0xac - 0x54];
    int m_shooterID;   // 0xac (SOCOM 1: 0x90)
};

/* the node as the map reads it: its matrix (zdb's CNode, the matrix first) */
class CZMapNode {
public:
    CMatrix m_matrix;   // 0x0
};

/* a seal's zoom (zseal.h's ZOOMSTATE, a signed byte under -enum min: only what the map tests) */
enum ZOOMSTATE {
    ZOOMSTATE_UNKNOWN = -1, ZOOMSTATE_3RDPERSON = 0, ZOOMSTATE_1STPERSON = 1, ZOOMSTATE_THERMAL = 2,
    ZOOMSTATE_NVG = 3, ZOOMSTATE_BINOCS = 4
};

/* the kit as the map reads it (zkit's CZKit: the lens colour applied, the heartbeat sensor's bit 15
 * of the flags at 0) */
class CZKit {
public:
    void Apply_LensEffects(bool on, CPnt3D *color);

    unsigned int m_bits0 : 15;
    unsigned int m_heartbeat_enabled : 1;
};

/* CEntity's position, node and team (SOCOM 1's m_team at 0xC4, 4 later in SOCOM II); the seal's zoom
 * and kit (the compass map's lens test reads them) */
class CZSealBody {
public:
    unsigned char _0[0x1c];
    CPnt3D m_posW;   // 0x1c
    CZMapNode *m_node;   // 0x28
    unsigned char _2c[0xc0 - 0x2c];
    class CSealCtrl *m_ctrl;   // 0xc0 (its controller: the full-screen map's tick reads it)
    unsigned char _c4[0xc8 - 0xc4];
    unsigned int m_team;   // 0xc8
    unsigned char _cc[0x200 - 0xcc];
    ZOOMSTATE m_zoomstate;   // 0x200
    unsigned char _201[0x5e0 - 0x201];
    CZKit m_kit;   // 0x5e0
};

/* SOCOM 1's DWARF layout; the tri-fan 4 bytes shorter, so the flag and size sit 4 lower */
class CZClaymoreMapItem : public CZMapItem {
public:
    CZClaymoreMapItem(CZSealBody *owner, CZProjectile *projectile);
    /* Draw declared first: the key function, so the vtable and the implicit destructor come out
     * here while Tick is unwritten (the slots are CZMapItem's either way) */
    virtual void Draw(zdb::CCamera *camera);
    virtual bool Tick(CZNewHudMap *map, float dT);

    CZSealBody *m_owner;   // 0x4
    CZProjectile *m_projectile;   // 0x8
    C2DBitmapPoly m_graphic;   // 0x10
    C2DTriFan m_clippedgraphic;   // 0x200
    bool m_wasdetonated;   // 0x228
    float m_size;   // 0x22c

    static zdb::CTexture *m_claymoretex;
    static float m_U0;
    static float m_V0;
    static float m_U1;
    static float m_V1;
    static zdb::CTexture *m_explosiontex;
    static float m_explosionU0;
    static float m_explosionV0;
    static float m_explosionU1;
    static float m_explosionV1;
};   // 0x230 bytes

#endif
