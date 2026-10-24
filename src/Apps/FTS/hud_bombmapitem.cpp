/* hud_bombmapitem.cpp -- the multiplayer bomb on the HUD's map (OVL1 0x20CF40-0x20D38F): SOCOM 1's
 * unit (its DWARF names it, Z:\dev\Apps\FTS\hud_bombmapitem.cpp): the implicit destructor, Draw,
 * Tick and the constructor in SOCOM 1's order. Tick is not written yet. Defined in reverse of
 * their image order (-inline deferred). */

#include "../../gamez/ztwod/ztwod.h"
#include "hud_mapitem.h"

namespace zdb {
class CNode;
}

/* SOCOM 1's DWARF layout; the tri-fan 4 bytes shorter */
class CZMPBombMapItem : public CZMapItem {
public:
    CZMPBombMapItem(zdb::CNode *node);
    /* Draw declared first: the key function, so the vtable and the implicit destructor come out
     * here while Tick is unwritten (the slots are CZMapItem's either way) */
    virtual void Draw(zdb::CCamera *camera);
    virtual bool Tick(CZNewHudMap *map, float dT);

    C2DBitmapPoly m_graphic;   // 0x10
    C2DTriFan m_clippedgraphic;   // 0x200
    bool m_wasdetonated;   // 0x228
    float m_size;   // 0x22c
    zdb::CNode *m_node;   // 0x230
};   // 0x240 bytes

/* the bomb's node kept, a red bitmap */
CZMPBombMapItem::CZMPBombMapItem(zdb::CNode *node)
{
    m_node = node;
    m_wasdetonated = false;
    m_size = 3.0f;
    m_graphic.SetColor(255.0f, 0.0f, 0.0f);
}

void CZMPBombMapItem::Draw(zdb::CCamera *camera)
{
    m_graphic.Draw(camera);
}
