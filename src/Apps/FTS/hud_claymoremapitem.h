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

/* CEntity's team (SOCOM 1's m_team at 0xC4, 4 later in SOCOM II) */
class CZSealBody {
public:
    unsigned char _0[0xc8];
    unsigned int m_team;   // 0xc8
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
