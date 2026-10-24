/* hud_claymoremapitem.cpp -- a claymore on the HUD's map (OVL1 0x208AD0-0x208FCF): SOCOM 1's unit
 * (its DWARF names it, Z:\dev\Apps\FTS\hud_claymoremapitem.cpp): the implicit destructor, Draw,
 * Tick and the constructor in SOCOM 1's order, with the first map item's inline virtuals' copies
 * (CZMapItem's, linked from here). Tick is not written yet. Defined in reverse of their image
 * order (-inline deferred). */

#include "../../gamez/ztwod/ztwod.h"
#include "hud_mapitem.h"

class CZProjectile;
class CZSealBody;

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
};   // 0x230 bytes

CZClaymoreMapItem::CZClaymoreMapItem(CZSealBody *owner, CZProjectile *projectile)
{
    m_owner = owner;
    m_projectile = projectile;
    m_wasdetonated = false;
    m_size = 3.0f;
}

void CZClaymoreMapItem::Draw(zdb::CCamera *camera)
{
    m_graphic.Draw(camera);
}
