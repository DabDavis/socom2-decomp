/* The multiplayer bomb's item on the HUD's map, for its unit and the map's finders. The header's
 * name is ours. */
#ifndef HUD_BOMBMAPITEM_H
#define HUD_BOMBMAPITEM_H

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

#endif
