/* hud_ammo.cpp -- the HUD's ammunition counts (OVL1 ...-0x23841B): SOCOM II's unit (no SOCOM 1
 * counterpart; the file name is ours), before the info box's: only the constructor (the unit's
 * last function in the image), the destructor and Reset so far. Defined in reverse of their image order
 * (-inline deferred). */

#include "hud_ammo.h"

/* the counts last shown (the unit's statics, bss 0x4145E0 and data 0x3DCBD8; no rows yet) */
extern int data_004145E0;
extern int data_003DCBD8;

CAmmoDisplay::CAmmoDisplay()
{
    m_socom2_5c0 = 0;
}

/* the cached counts forgotten, as Reset does */
CAmmoDisplay::~CAmmoDisplay()
{
    data_004145E0 = 0;
    data_003DCBD8 = -1;
}

/* both counts wiped and forgotten (CHUD's ReInit_Init and ReInit_Uninit call it; the name ours) */
void CAmmoDisplay::Reset()
{
    m_ammo.ClearAndWipe();
    m_mags.ClearAndWipe();
    data_004145E0 = 0;
    data_003DCBD8 = -1;
}
