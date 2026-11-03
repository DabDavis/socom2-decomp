/* seal_ik.cpp -- SOCOM 1's unit of the name (linked between seal_ex.cpp and seal_kit.cpp: its
 * TurnSetup and FootInSpace there). OVL2 0x5B8270-0x5B82CF, after CSealEx::OnWeaponHit (seal_ex.cpp)
 * and before the kit's first row (seal_kit.cpp, ~0x5B82D0): SOCOM II keeps only TurnSetup, its foot
 * placement gone. */

#include "zseal.h"

/* a turn's set-up: with both feet, the two bits at 0x1060 6 and 7 cleared, 7 first (SOCOM 1's
 * TurnSetup__10CZSealBodyFi in the same place, called likewise from two stance functions passing
 * a second word -- 0x584C60 and 0x586050, as SOCOM 1's InStateCrouch and StandTurning -- the
 * word unread now; OVL2 0x5B8270) */
void CZSealBody::TurnSetup(int anim)
{
    if (!m_lfoot || !m_rfoot)
        return;
    m_bit7 = false;
    m_bit6 = false;
}
