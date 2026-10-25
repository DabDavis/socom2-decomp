/* hud_bombmapitem.cpp -- the multiplayer bomb on the HUD's map (OVL1 0x20CF40-0x20D38F): SOCOM 1's
 * unit (its DWARF names it, Z:\dev\Apps\FTS\hud_bombmapitem.cpp): the implicit destructor, Draw,
 * Tick and the constructor in SOCOM 1's order. Tick is not written yet. Defined in reverse of
 * their image order (-inline deferred). */

#include "hud_bombmapitem.h"

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
