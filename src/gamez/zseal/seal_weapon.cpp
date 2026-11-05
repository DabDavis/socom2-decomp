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

/* (in state 3 the base blend's state: CZSealBody::PeekBaseState, OVL2 0x57DC20, seal_body.cpp's --
 * zseal.h's member) */

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
        state = (SEAL_STATE)PeekBaseState();

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


/* the points found once by name through the fire-point lookup, then both positions taken (OVL2
 * 0x5DF450; SOCOM 1's UpdateAimParams, its lookups inlined there, a virtual call here) */
void CZSealBody::UpdateAimParams()
{
    CSealAimPoints *a = (CSealAimPoints *)this;

    if (!a->m_curr_aimpt)
        a->m_curr_aimpt = GetFirepointNode("aimpoint");
    if (!a->m_curr_firept)
        a->m_curr_firept = GetFirepointNode("firepoint");
    GetNodePos(a->m_curr_aimpt, a->m_curr_aimpos, false);
    GetNodePos(a->m_curr_firept, a->m_curr_firepos, false);
}

/* the item's part worn as gear for a fire class (SOCOM 1's BodyWepPart, out of line there and by the
 * item type; inlined here, the table read once and tested twice -- the name ours) */
static inline CZBodyPart *WepPart(CZSealBody *seal, int fc)
{
    return seal->m_item_gear[fc].m_part;
}

/* the node a weapon's point hangs on: the named child of the part the item in hand is worn as, or the
 * part's own node with no name (OVL2 0x5DFAD0; SOCOM 1's GetFirepointNode, by the fire class here, and
 * virtual: CEntity's slot +0x64) */
zdb::CNode *CZSealBody::GetFirepointNode(const char *name)
{
    unsigned char fc = m_kit.GetCurItemFireClass();

    if (m_item_gear[fc].m_part) {
        CZBodyPart *part = WepPart(this, fc);

        if (part) {
            if (name && part->m_node)
                return part->m_node->FindChild(name, false);
            return part->m_node;
        }
    }
    return 0;
}

/* where a node of the item in hand lies: the item worn by the fire class -- with none in hand, the
 * rifle's or the pistol's, whichever the right hand holds -- the node's translation taken as the
 * offset from its part, the result in the world or only turned into the world's axes (OVL2
 * 0x5DF930; SOCOM 1's GetNodePos, the hand test SOCOM II's). The class an int (`li` for 1 and 2, no
 * narrowing at the index), the transform inside the table test (its failure goes straight to the
 * return). The hand test reads the part's parent through an inline accessor on the left of `==`: the
 * target loads the rifle (0x350) into v1 and the right hand into v0 and compares `bne v1,v0` -- the
 * accessor's result a temporary numbered after the hand's load (round 42; written in place, `parent ==
 * hand` gave 7 differences, `hand == parent` 5, the accessor on the right 2) */
static inline CZBodyPart *PartParent(const CZBodyPart *p)
{
    return p->m_parent;
}

bool CZSealBody::GetNodePos(const zdb::CNode *node, CPnt3D &pos, bool world)
{
    if (!node)
        return false;
    CPnt3D p;
    int fc = m_kit.GetCurItemFireClass();

    if (fc == 0) {
        if (PartParent(m_rifle) == m_rhand)
            fc = 1;
        else if (PartParent(m_pistol) == m_rhand)
            fc = 2;
    }
    bool ok = false;

    if (m_item_gear[fc].m_part) {
        CZBodyPart *part = WepPart(this, fc);

        if (part && part->m_node) {
            PartInSpace(part, (const CPnt3D *)node->m_matrix.m_matrix[3], &p, 0);
            ok = true;
        }
        if (ok) {
            if (world)
                m_node->m_matrix.Transform(&p, &pos, 1);
            else
                TransformVectors(&m_node->m_matrix, &p, &pos, 1);
        }
    }
    return ok;
}

/* where a named point of the item in hand lies in the world: the part's own place with no name or no
 * such child, the child's translation as the offset with one (OVL2 0x5DFB70; SOCOM 1's
 * GetFirepointPos, by the fire class here, the world transform a call where SOCOM 1 inlined it) */
bool CZSealBody::GetFirepointPos(CPnt3D &pos, const char *name)
{
    CPnt3D p;
    bool ok;
    unsigned char fc = m_kit.GetCurItemFireClass();

    ok = false;
    if (m_item_gear[fc].m_part) {
        const CPnt3D *off = 0;
        CZBodyPart *part = WepPart(this, fc);

        ok = true;
        if (!part || !part->m_node)
            ok = false;
        if (ok) {
            if (name) {
                zdb::CNode *n = part->m_node->FindChild(name, false);

                if (n)
                    off = (const CPnt3D *)n->m_matrix.m_matrix[3];
            }
            PartInSpace(part, off, &p, 0);
        }
    }
    if (ok)
        m_node->m_matrix.Transform(&p, &pos, 1);
    return ok;
}

/* the weapon's timing and its sequence (SOCOM 1's 0xDD0-0xDDF and its m_weapon_seq pointer at 0xF88:
 * the times 276 bytes on here, the sequence held at 0x1160 -- zseal.h keeps them as bytes; the view
 * and the names ours) */
struct CSealWepView {
    unsigned char _0[0x258];
    unsigned char m_258 : 1;   // 0x258 bit 0: SOCOM II's, tested before a draw starts (the name ours)
    unsigned char _259[0xee4 - 0x259];
    float m_wep_upT;   // 0xee4
    float m_wep_holdT;   // 0xee8
    float m_wep_downT;   // 0xeec
    float m_wep_lenT;   // 0xef0
    unsigned char _ef4[0x1160 - 0xef4];
    CSequence m_wep_seq;   // 0x1160

};

/* a weapon's id (zweapon.h's CZWeapon::m_ID at 0x7C, a byte enum -- zseal.h declares the item only by
 * name; the view ours) */
struct CSealWepIdView {
    unsigned char _0[0x7c];
    EQUIP_ITEM m_ID;   // 0x7c
};

/* the weapon's state set: drawing (1) starts the sequence from rest unless item 190 is in hand, or
 * clears the busy bit once past the hold; putting away (2) runs the sequence back from where it is
 * (SOCOM 1's SetWeaponState, its launcher 182 here 190, and SOCOM II's bit at 0x258; OVL2 0x5DFE30) */
void CZSealBody::SetWeaponState(WEAPON_STATE state)
{
    CSealWepView *v = (CSealWepView *)this;

    if (state == GetWeaponState())
        return;
    float t = v->m_wep_seq.m_currT;

    if (t > 0.0f)
        t = v->m_wep_lenT - t;
    switch (state) {
    case 16:
        return;
    case 4:
        return;
    case 1:
        if (v->m_258) {
            m_weapon_state = 17;
            if (t == 0.0f) {
                if (m_kit.GetCurItem() && ((CSealWepIdView *)m_kit.GetCurItem())->m_ID != 190)
                    v->m_wep_seq.Start(v->m_wep_upT, v->m_wep_holdT, v->m_wep_downT);
            } else if (!(t < v->m_wep_upT) && t < v->m_wep_upT + v->m_wep_holdT)
                m_weapon_state &= ~0x10;
        }
        break;
    case 2:
        {
            float w = 0.99f < v->m_wep_seq.LinearWeight() ? 0.99f : v->m_wep_seq.LinearWeight();

            v->m_wep_seq.m_currT = w * v->m_wep_downT;
        }
        m_weapon_state = 18;
        break;
    }
}

/* the weapon state machine stepped (OVL2 0x5DFC80; SOCOM 1's WeaponsTick, the sequence held): with
 * the busy bit, a draw holds until the sequence passes its hold and a put-away runs out; without it,
 * a finished draw or put-away turns into the other's state; then the aim's direction, the aim and the
 * kit's frame -- but in state 16, which returns at once. The state read once into the byte enum
 * (GetWeaponState inlined loads the word again for the bit test).
 * OPEN, 5 instructions, the draw's hold test alone (case 1): the target loads the hold time (0xEE8)
 * into $f2, then the down time into $f0, the sequence's time into $f1, and sums `add.s $f0,$f0,$f2`
 * (down + hold, the sum in the down's register); every spelling here gives the down $f1 and the time
 * $f0 -- `hold + down` or `down + hold` in place (CSE'd for the store) or in a local, an inline
 * returning the sum on the view, inline accessors for both, a two-float inline taking (down, hold)
 * (its arguments evaluated right to left), the comparison reversed (`>=`: c.lt.s). SOCOM 1's (0x2C02C0)
 * loads the down first and sums into the hold's register: SOCOM II's source changed here. Round 52,
 * MW's colouring replayed (tools/igreplay.py on the FPR record, --want 33=f0,34=f2,35=f0,37=f1): every
 * placement giving the target's four is `sum < hold < currT < down` -- the sum a front-end temporary
 * (the store's CSE) and the three loads numbered hold, currT, down, where every spelling here loads
 * hold, down, then currT (the sum's operands first); hold in a named local (propagated: 6), the sum
 * through an inline accessor on the view, `down + hold` or `hold + down`, in a local or twice, or the
 * test reversed (5-7) do not reorder them */
void CZSealBody::WeaponsTick(float dT)
{
    CSealWepView *v = (CSealWepView *)this;
    WEAPON_STATE ws = (WEAPON_STATE)(m_weapon_state & ~0x10);

    if (m_weapon_state & 0x10) {
        switch (ws) {
        case 16:
            return;
        case 4:
            break;
        case 1:
            if (v->m_wep_seq.m_currT <= v->m_wep_holdT + v->m_wep_downT) {
                v->m_wep_seq.m_currT = v->m_wep_holdT + v->m_wep_downT;
                m_weapon_state &= ~0x10;
            } else
                v->m_wep_seq.Tick(dT);
            break;
        case 2:
            v->m_wep_seq.Tick(dT);
            if (v->m_wep_seq.m_currT <= 0.0f) {
                m_weapon_state &= ~0x10;
                v->m_wep_seq.Clear();
            }
            break;
        }
    } else {
        switch (ws) {
        case 1:
            if (v->m_wep_seq.Weight() != 1.0f) {
                m_weapon_state = 2;
                SetWeaponState((WEAPON_STATE)1);
            }
            break;
        case 2:
            if (v->m_wep_seq.Weight() != 0.0f) {
                m_weapon_state = 1;
                SetWeaponState((WEAPON_STATE)2);
            }
            break;
        }
    }
    func_005DEEC0(this, false);
    AimWeapon();
    m_kit.PreTick(dT);
}

/* the weapon's state without its bit 4 (OVL2 0x5E0010; SOCOM 1's, the same 0x14 bytes with the
 * field 0x120 on) */
WEAPON_STATE CZSealBody::GetWeaponState() const
{
    return (WEAPON_STATE)(m_weapon_state & ~0x10);
}
