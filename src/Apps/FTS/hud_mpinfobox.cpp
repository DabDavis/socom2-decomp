/* hud_mpinfobox.cpp -- the network game's info box (OVL1 0x238420-...): SOCOM II's unit (no SOCOM 1
 * counterpart; the file name is ours), after the ammo counts' and before the radio icons' Ticks:
 * CMPInfoBox's Off, On, Tick, the clean-up CHUD::ReInit_Uninit calls and the re-init
 * ReInit_Init calls, then CWhosTalking's Tick. Only Off, On and the clean-up so far. Defined in
 * reverse of their image order (-inline deferred). */

#include "hud_mpinfobox.h"

/* the strings emptied and the radio's parts hidden */
void CMPInfoBox::UnInit()
{
    Off();
    m_text.SetString(0, true);
    m_whostalking.m_text1.Off();
    m_whostalking.m_text1.SetString(0, true);
    m_whostalking.m_bitmap2.Off();
    m_whostalking.m_text2.Off();
    m_whostalking.m_text2.SetString(0, true);
}

void CMPInfoBox::On()
{
    C2D::On();
    m_bitmap2.On();
    m_healthbar.On();
    m_text.On();
    m_whostalking.On();
}

void CMPInfoBox::Off()
{
    C2D::Off();
    m_bitmap2.Off();
    m_healthbar.Off();
    m_text.Off();
    m_whostalking.Off();
}
