/* hud_claymoremapitem.cpp -- a claymore on the HUD's map (OVL1 0x208AD0-0x208FCF): SOCOM 1's unit
 * (its DWARF names it, Z:\dev\Apps\FTS\hud_claymoremapitem.cpp): the implicit destructor, Draw,
 * Tick and the constructor in SOCOM 1's order, with the first map item's inline virtuals' copies
 * (CZMapItem's, linked from here). Tick is not written yet. Defined in reverse of their image
 * order (-inline deferred). */

#include "../../gamez/ztwod/ztwod.h"
#include "hud_mapitem.h"
#include "hud_newhudmap.h"

/* only what the item reads: SOCOM 1's frame end position, 4 later in SOCOM II */
class CZProjectile {
public:
    bool WasDetonated();

    unsigned char _0[0x48];
    CPnt3D m_frameendpos;   // 0x48 (SOCOM 1: 0x44)
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

zdb::CTexture *CZClaymoreMapItem::m_claymoretex;
float CZClaymoreMapItem::m_U0;
float CZClaymoreMapItem::m_V0;
float CZClaymoreMapItem::m_U1 = 1.0f;
float CZClaymoreMapItem::m_V1 = 1.0f;
zdb::CTexture *CZClaymoreMapItem::m_explosiontex;
float CZClaymoreMapItem::m_explosionU0;
float CZClaymoreMapItem::m_explosionV0;
float CZClaymoreMapItem::m_explosionU1;
float CZClaymoreMapItem::m_explosionV1;

CZClaymoreMapItem::CZClaymoreMapItem(CZSealBody *owner, CZProjectile *projectile)
{
    m_owner = owner;
    m_projectile = projectile;
    m_wasdetonated = false;
    m_size = 3.0f;
}

/* the claymore shown to its owner's team while it waits, where the map puts its projectile, and
 * blown up in place once detonated (a growing burst; true once it has grown past 30).
 * OPEN (2 instructions, the same in CZMPBombMapItem::Tick): the target sums the grown size into
 * $f0, stores it and copies it to $f23 for the four CalcDrawPos calls; MW sums straight into $f23
 * for every form tried -- the compound assignment, a separate sum and store, the size declared
 * outside the branch, an inline helper taking it by value -- and the one instruction it saves
 * moves the epilogue's alignment nop */
bool CZClaymoreMapItem::Tick(CZNewHudMap *map, float dT)
{
    CZSealBody *viewer = map->m_viewer;

    if (!viewer) {
        if (m_graphic.IsOn())
            m_graphic.Off();
        return false;
    }
    if (!(m_owner->m_team & viewer->m_team)) {
        if (m_graphic.IsOn())
            m_graphic.Off();
        return false;
    }
    if (!m_wasdetonated) {
        if (!m_graphic.IsOn())
            m_graphic.On();
        if (m_projectile && m_projectile->WasDetonated())
            m_wasdetonated = true;
        CPnt3D pos = m_projectile->m_frameendpos;

        map->m_matrix.Transform(&pos, 1);
        float size = m_size;

        m_graphic.Load(pos.x - m_size, map->CalcDrawPos(pos.y - size), pos.x - size,
                       map->CalcDrawPos(pos.y + size), pos.x + size, map->CalcDrawPos(pos.y + size),
                       pos.x + size, map->CalcDrawPos(pos.y - size), m_claymoretex);
        m_graphic.SetUV(m_U0, m_V0, m_U1, m_V1);
        return false;
    }
    CPnt3D pos = m_projectile->m_frameendpos;

    map->m_matrix.Transform(&pos, 1);
    m_size += 10.0f * dT;
    float size = m_size;

    m_graphic.Load(pos.x - m_size, map->CalcDrawPos(pos.y - size), pos.x - size,
                   map->CalcDrawPos(pos.y + size), pos.x + size, map->CalcDrawPos(pos.y + size),
                   pos.x + size, map->CalcDrawPos(pos.y - size), m_explosiontex);
    m_graphic.SetUV(m_explosionU0, m_explosionV0, m_explosionU1, m_explosionV1);
    return m_size > 30.0f;
}

void CZClaymoreMapItem::Draw(zdb::CCamera *camera)
{
    m_graphic.Draw(camera);
}
