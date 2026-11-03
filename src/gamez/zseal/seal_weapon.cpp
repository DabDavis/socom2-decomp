/* seal_weapon.cpp -- the SEAL's weapon handling: the fire point, WeaponsTick, the weapon state and
 * the wait on it (SOCOM 1's CZSealBody functions in this order, right before AnimSet's; OVL2 from
 * ~0x5DF370 to ~0x5E0023: GetFP/GetAP, Virtual64, WeaponsTick, SetWeaponState, WepWaitOn/Off,
 * GetWeaponState). The unit's name is ours. So far one accessor. Defined in reverse of their image
 * order (-inline deferred). */

#include "zseal.h"

/* the aim's point and goal (SOCOM 1's at 0x10A0 and 0x10AC, here 0x3A0 on: zseal.h keeps the
 * body's tail as bytes -- the view and the names ours) */
struct CSealAimView {
    unsigned char _0[0x1440];
    CPnt3D m_aim_from;   // 0x1440
    CPnt3D m_aim_goal;   // 0x144c
};

/* the aim set along a direction from its point, and whether a goal came of it: when none did and
 * `reset` is asked, the goal is the unit -z (OVL2 0x5DF8B0; SOCOM 1's SetAimDirR, the same code with
 * the fields 0x3A0 on -- the bool it returns stays in v0, masked in place). The arguments are
 * spelled as offsets so that MW does not keep the goal's address for the stores */
bool CZSealBody::SetAimDirR(const CPnt3D *dir, bool reset)
{
    bool ok = CalcAimGoal((CPnt3D *)((char *)this + 0x1440), dir, (CPnt3D *)((char *)this + 0x144c));

    if (!ok && reset)
        ((CSealAimView *)this)->m_aim_goal = CPnt3D::unit_nz;
    return ok;
}

/* the weapon's state without its bit 4 (OVL2 0x5E0010; SOCOM 1's, the same 0x14 bytes with the
 * field 0x120 on) */
WEAPON_STATE CZSealBody::GetWeaponState() const
{
    return (WEAPON_STATE)(m_weapon_state & ~0x10);
}
