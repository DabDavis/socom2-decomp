/* seal_weapon.cpp -- the SEAL's weapon handling: the fire point, WeaponsTick, the weapon state and
 * the wait on it (SOCOM 1's CZSealBody functions in this order, right before AnimSet's; OVL2 from
 * ~0x5DF370 to ~0x5E0023: GetFP/GetAP, Virtual64, WeaponsTick, SetWeaponState, WepWaitOn/Off,
 * GetWeaponState). The unit's name is ours. So far one accessor. Defined in reverse of their image
 * order (-inline deferred). */

#include "zseal.h"

/* the weapon's state without its bit 4 (OVL2 0x5E0010; SOCOM 1's, the same 0x14 bytes with the
 * field 0x120 on) */
WEAPON_STATE CZSealBody::GetWeaponState() const
{
    return (WEAPON_STATE)(m_weapon_state & ~0x10);
}
