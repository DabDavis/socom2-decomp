/* hud_healthbar.cpp -- the network game's health bar (OVL1 0x241C30-...): SOCOM II's unit (no SOCOM
 * 1 counterpart; the file name is ours), after the network weapon selector's: CHealthBar's SetTrans
 * (slot 11), SetPos and SetSize, then Update (0x241CC0), the Tick (slot 12, 0x242050), the
 * destructor (0x242300) and the constructor (0x2423D0) -- the HUD lane's range ends at 0x242000,
 * so only the first three so far. Defined in reverse of their image order (-inline deferred). */

#include "hud_mpinfobox.h"

void CHealthBar::SetSize(float w, float h)
{
    m_w = w;
    m_h = h;
}

void CHealthBar::SetPos(float x, float y)
{
    m_x = x;
    m_y = y;
}

void CHealthBar::SetTrans(float trans)
{
    m_bitmap1.SetTrans(trans);
    m_bitmap2.SetTrans(trans);
    m_bitmap3.SetTrans(trans);
}
