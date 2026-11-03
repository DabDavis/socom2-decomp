/* seal_weapon.cpp -- the SEAL's weapon handling: the fire point, WeaponsTick, the weapon state and
 * the wait on it (SOCOM 1's CZSealBody functions in this order, right before AnimSet's; OVL2 from
 * ~0x5DF370 to ~0x5E0023: GetFP/GetAP, GetFirepointNode, WeaponsTick, SetWeaponState, WepWaitOn/Off,
 * GetWeaponState; SOCOM II's 0x5DF4E0, 0x5DF600 and 0x5DF8B0 are SOCOM 1's SetAimPointW, CalcAimGoal
 * and SetAimDirR). The unit's name is ours. So far CalcAimGoal (round 14), SetAimDirR and
 * GetWeaponState; round 15 UpdateAimParams and SetAimPointW. Open: 0x5DFAD0 is SOCOM 1's
 * GetFirepointNode(const char *), CZSealBody's override of CEntity's slot +0x64 (zentity.h names
 * it since the SEAL lane's round 14; UpdateAimParams calls it through the real slot). Defined in
 * reverse of their image order (-inline deferred). */

#include "zseal.h"

/* the aim's point and goal (SOCOM 1's at 0x10A0 and 0x10AC, here 0x3A0 on: zseal.h keeps the
 * body's tail as bytes -- the view and the names ours) */
struct CSealAimView {
    unsigned char _0[0x1440];
    CPnt3D m_aim_from;   // 0x1440
    CPnt3D m_aim_goal;   // 0x144c
    /* SOCOM 1's m_aim_point, m_reticle_pt and m_aim_norm (0x10b8-0x10e3 there) */
    CPnt3D m_aim_point;   // 0x1458
    CPnt3D m_reticle_pt;   // 0x1464
    CPnt3D m_aim_norm;   // 0x1470
};

/* the aim goal's flag (bit 3 of the byte at 0x105F, read and written a byte at a time: zseal.h's
 * bits there are unsigned int -- the view and the name ours), set through an inline taking a bool
 * (`(b & 1) << 3` even for false) */
struct CSealAimBits {
    unsigned char _0[0x105f];
    unsigned char _bits0 : 3;
    unsigned char m_aim_goal_valid : 1;   // 0x105f bit 3
};

inline void SetAimGoalValid(CZSealBody *seal, bool b)
{
    ((CSealAimBits *)seal)->m_aim_goal_valid = b;
}

/* the SEAL's state at 0x174 (CBody's short) and, in state 3, the base blend's (OVL2 0x57DC20,
 * seal_body.cpp's: SOCOM 1's PeekBaseState) */
extern "C" unsigned char func_0057DC20(CZSealBody *seal);

/* a goal for the aim along a direction: the direction itself when it lies inside the stance's
 * limits (their cosines: pitch up or down by the direction's y, and yaw), else what CalcGoalDir
 * makes of it within the limits themselves; standing, crouched and state 6 share the upright
 * limits, prone (2) has its own, any other state has none (OVL2 0x5DF600; SOCOM 1's CalcAimGoal,
 * the limits theCharacterDynamics' as there, the length's sqrt inline) */
bool CZSealBody::CalcAimGoal(const CPnt3D *from, const CPnt3D *dir, CPnt3D *goal)
{
    SEAL_STATE state = (SEAL_STATE)m_state;
    float maxPitch, minPitch, maxYaw;

    if (m_state == 3)
        state = (SEAL_STATE)func_0057DC20(this);

    switch (state) {
    case 0:
    case 1:
    case 6:
        maxPitch = theCharacterDynamics.m_cosMaxAimPitch;
        minPitch = theCharacterDynamics.m_cosMinAimPitch;
        maxYaw = theCharacterDynamics.m_cosMaxAimYaw;
        break;
    case 2:
        maxPitch = theCharacterDynamics.m_cosProneMaxAimPitch;
        minPitch = theCharacterDynamics.m_cosProneMinAimPitch;
        maxYaw = theCharacterDynamics.m_cosProneMaxAimYaw;
        break;
    default:
        SetAimGoalValid(this, false);
        return false;
    }

    bool ok = true;
    float len = sqrtf(dir->x * dir->x + dir->z * dir->z);
    /* (tested bare: `len != 0.0f` compares 0 against len, the target len against 0) */
    float yaw = len ? -dir->z / len : 1.0f;

    if (dir->y >= 0.0f) {
        if (len < maxPitch)
            ok = false;
    } else if (len < minPitch)
        ok = false;
    if (yaw < maxYaw)
        ok = false;
    if (ok) {
        *goal = *dir;
        SetAimGoalValid(this, true);
        return true;
    }

    float up, down, side;

    switch (state) {
    case 0:
    case 1:
    case 6:
        up = theCharacterDynamics.m_maxAimPitch;
        down = theCharacterDynamics.m_minAimPitch;
        side = theCharacterDynamics.m_maxAimYaw;
        break;
    case 2:
        up = theCharacterDynamics.m_proneMaxAimPitch;
        down = theCharacterDynamics.m_proneMinAimPitch;
        side = theCharacterDynamics.m_proneMaxAimYaw;
        break;
    default:
        SetAimGoalValid(this, false);
        return false;
    }

    bool r = CalcGoalDir(from, dir, goal, up, down, -side, side);

    SetAimGoalValid(this, r);
    return r;
}

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

extern "C" {
/* the aim's direction brought up to date (SOCOM 1's UpdateAimDir__10CZSealBodyFb; OVL2 0x5DEEC0,
 * unwritten) */
bool func_005DEEC0(CZSealBody *seal, bool keep);
}

/* the aim at a point: the point kept, and the reticle's too unless asked to keep it, the normal
 * when there is one; with none the aim falls back on the unit -z from the aim's origin (OVL2
 * 0x5DF4E0; SOCOM 1's SetAimPointW, the fields 0x3A0 on) */
bool CZSealBody::SetAimPointW(const CPnt3D *aimpt, const CPnt3D *aimnorm, bool keep)
{
    CSealAimView *v = (CSealAimView *)this;

    if (aimpt) {
        v->m_aim_point = *aimpt;
        m_aim_point_valid = true;
        if (!keep)
            v->m_reticle_pt = *aimpt;
        if (aimnorm) {
            v->m_aim_norm = *aimnorm;
            m_have_aim_norm = true;
        } else
            m_have_aim_norm = false;
    } else {
        m_have_aim_norm = false;
        m_aim_point_valid = false;
        CalcAimGoal(&v->m_aim_from, &CPnt3D::unit_nz, &v->m_aim_goal);
        return false;
    }
    return func_005DEEC0(this, keep);
}

/* the aim and fire points and their positions (SOCOM 1's m_curr_aimpos, m_curr_aimpt,
 * m_curr_firepos and m_curr_firept, 0x1100-0x111F there, 0x3A0 on here: zseal.h keeps the body's
 * tail as bytes -- the view ours) */
struct CSealAimPoints {
    unsigned char _0[0x14a0];
    CPnt3D m_curr_aimpos;   // 0x14a0
    zdb::CNode *m_curr_aimpt;   // 0x14ac
    CPnt3D m_curr_firepos;   // 0x14b0
    zdb::CNode *m_curr_firept;   // 0x14bc
};

extern "C" {
/* a node's position (SOCOM 1's GetNodePos__10CZSealBodyFPCQ23zdb5CNodeR6CPnt3Db; OVL2 0x5DF930,
 * this unit's, unwritten) */
bool func_005DF930(CZSealBody *seal, const zdb::CNode *node, CPnt3D *pos, bool b);
}

/* the points found once by name through the fire-point lookup, then both positions taken (OVL2
 * 0x5DF450; SOCOM 1's UpdateAimParams, its lookups inlined there, a virtual call here) */
void CZSealBody::UpdateAimParams()
{
    CSealAimPoints *a = (CSealAimPoints *)this;

    if (!a->m_curr_aimpt)
        a->m_curr_aimpt = GetFirepointNode("aimpoint");
    if (!a->m_curr_firept)
        a->m_curr_firept = GetFirepointNode("firepoint");
    func_005DF930(this, a->m_curr_aimpt, &a->m_curr_aimpos, false);
    func_005DF930(this, a->m_curr_firept, &a->m_curr_firepos, false);
}

/* the weapon's state without its bit 4 (OVL2 0x5E0010; SOCOM 1's, the same 0x14 bytes with the
 * field 0x120 on) */
WEAPON_STATE CZSealBody::GetWeaponState() const
{
    return (WEAPON_STATE)(m_weapon_state & ~0x10);
}
