/* seal_airesponse.cpp -- the states an AI SEAL falls into in response: hostage, avoid, rush, path
 * recovery, flee, target, event, stun response, hold, surrender, no ammo, reload, shot at (SOCOM
 * 1's unit, __sinit_seal_airesponse.cpp -- its CAiSRestrained and CAiSCorpse are gone, CAiSAvoid
 * moved in from seal_aistate.cpp, CAiSHostage is here). OVL2 0x601880-0x609DEF. So far only the
 * states' destructors and the inline virtuals their tables pull in. Defined in reverse of their
 * image order (-inline deferred). */

#include "../gamez/zkit/zkit.h"

/* a chance taken: a random float in [0, 1) against the odds (an inline taking them by reference --
 * the target stores the constant and reads it back; the name ours) */
inline bool Chance(const float &odds)
{
    Rfloat r;

    r.m_min = 0.0f;
    r.m_range = 1.0f;
    return r <= odds;
}

/* the controller's bits (CSealCtrlAi at 0x264: SOCOM 1's m_debug word at 0x280 -- bit 1 is
 * m_navyseal; zseal.h does not name them) */
struct CAiCtrlBits {
    unsigned char _0[0x264];
    unsigned char m_debug : 1;
    unsigned char m_navyseal : 1;
};

/* a SEAL of the player's side (an inline bool accessor: its callers keep the sltu) */
inline bool NavySeal(CSealCtrlAi *ai)
{
    return ((CAiCtrlBits *)ai)->m_navyseal;
}

/* the controller's state stack (CSealCtrlAi at 0x41C; zseal.h does not name it) */
struct CAiCtrlStack {
    unsigned char _0[0x41c];
    CAiStateStack m_stack;   // 0x41c
};

inline CAiStateStack &StateStack(CSealCtrlAi *ai)
{
    return ((CAiCtrlStack *)ai)->m_stack;
}

/* a map cell copied field by field (SOCOM 1's written copy constructor, CAiMapLoc(const
 * CAiMapLoc &), whose out-of-line copy is 0x5EA7E0; zentity.h's CAiMapLoc has none -- the name
 * ours) */
struct CAiCell {
    CAiCell(const CAiMapLoc &o) : m_mapid(o.m_mapid), x(o.x), y(o.y) {}

    unsigned int m_mapid : 6;
    unsigned int x : 13;
    unsigned int y : 13;
};

class CZKit;
class CZFTSWeapon;

extern "C" {
/* CZKit's (seal_kit.cpp; kit.h's CZKit cannot meet zseal.h's here) */
CZFTSWeapon *GetItemByIndex__5CZKitCFi(const CZKit *kit, int index);
void *GetAmmoByIndex__5CZKitFi(CZKit *kit, int index);
/* CZSealBody's (named; zseal.h's CZSealBody does not declare them) */
void SetWeaponState__10CZSealBodyF12WEAPON_STATE(CEntity *seal, int state);
void PickupObject__10CZSealBodyFP7CPickup(CEntity *seal, CPickup *pickup);
/* the AI's state-clock hold (OVL2 0x570F90, the SEAL lane's: SOCOM 1 set the goal timer at 0x1BC
 * and saturated it in place) */
void func_00570F90(CSealCtrlAi *ai, float t);
/* CSealCtrlAi::SetStance, SetRoute, SetDoReaction and GetRouteComplete (SOCOM 1's, called from
 * the same Ticks; OVL2 0x5631A0, 0x5623B0, 0x561E20, 0x5629B0 -- the SEAL lane's rows) */
void func_005631A0(CSealCtrlAi *ai, int stance);
bool func_005623B0(CSealCtrlAi *ai, void *loc);
void func_00561E20(CSealCtrlAi *ai, int on, int reaction);
bool func_005629B0(CSealCtrlAi *ai);
/* a traversable cell beside a point (SOCOM 1's free FindTraversableNeighbor, now on the AI maps;
 * OVL2 0x529A30, the AI core's row) */
bool func_00529A30(CAiMaps *maps, const CPnt3D *pos, void *loc);
/* a cell to flee to, and its centre (OVL2 0x52A3C0, 0x529EE0, the AI core's rows) */
void *func_0052A3C0(CAiMaps *maps, CAiCell *from, int a, int b);
bool func_00529EE0(CAiMaps *maps, void *cell, unsigned int *loc);
/* the goto pushed to a cell (OVL2 0x5F0830, seal_aistate.cpp's) */
void func_005F0830(CSealCtrlAi *ai, unsigned int *loc, int mode, int kind, int step);
/* the AI's target held (OVL2 0x60A850, after the response states: CAiTarget's, the target at
 * CSealCtrlAi 0x67C) and a look pushed at a point (OVL2 0x5F0B30, seal_aistate.cpp's) */
bool func_0060A850(void *target);
bool func_005F0B30(CSealCtrlAi *ai, const CPnt3D *pos, int mode, int a, int b);
/* the nearest ammunition for a weapon (SOCOM 1's ai::find_closest_ammo, after the no-ammunition
 * state as there; OVL2 0x6082E0) */
CPickup *func_006082E0(const CPnt3D *pos, float range, CZKit *kit, CZFTSWeapon *w);
}

/* the destructors (OVL2 0x609D60 back to 0x609710, and CAiSHostage's at 0x601880): each its own
 * table stored, then CAiState's, its sub-state stack destroyed out of line */
CAiSShotAt::~CAiSShotAt()
{
}

CAiSReload::~CAiSReload()
{
}

CAiSNoAmmo::~CAiSNoAmmo()
{
}

CAiSSurrender::~CAiSSurrender()
{
}

CAiSHold::~CAiSHold()
{
}

CAiSStunResponse::~CAiSStunResponse()
{
}

CAiSEvent::~CAiSEvent()
{
}

CAiSTarget::~CAiSTarget()
{
}

CAiSFlee::~CAiSFlee()
{
}

CAiSRush::~CAiSRush()
{
}

CAiSAvoid::~CAiSAvoid()
{
}

/* a no-ammunition state's reset (OVL2 0x608290; SOCOM 1's) */
void CAiSNoAmmo::OnSignal(SIGNAL sig, CSealCtrlAi *ai)
{
    switch (sig) {
    case SIGNAL_4:
        m_pickup = 0;
        break;
    case SIGNAL_0:
        break;
    case SIGNAL_1:
        break;
    case SIGNAL_2:
        break;
    case SIGNAL_3:
        break;
    }
}

/* out of ammunition: crouched, weapon down, off to the nearest ammunition for the first item and
 * picking it up -- or, with none to reach, a reaction (not for the player's side) and done (OVL2
 * 0x608060; SOCOM 1's) */
AI_RESULT CAiSNoAmmo::Tick(float dT, CSealCtrlAi *ai)
{
    unsigned int phase;
    bool routed = false;
    CEntity *seal = ai->m_entity;

    phase = m_phase;
    CZKit *kit = (CZKit *)((char *)seal + 0x5e0);
    CZFTSWeapon *w = GetItemByIndex__5CZKitCFi(kit, 0);

    GetAmmoByIndex__5CZKitFi(kit, 0);
    switch (phase) {
    case 0:
        func_00570F90(ai, 1.0f);
        func_005631A0(ai, 1);
        SetWeaponState__10CZSealBodyF12WEAPON_STATE(seal, 2);
        if ((m_pickup = func_006082E0(&seal->m_posW, 5000.0f, kit, w)) != 0) {
            unsigned int loc;

            m_pickupLoc = *(unsigned int *)&seal->m_mapLocValid;
            if (func_00529A30(&AiMaps, m_pickup->GetPosW(), &loc) && func_005623B0(ai, &loc)) {
                phase = 1;
                func_005631A0(ai, 0);
                func_00570F90(ai, 1.0f);
                routed = true;
            }
        }
        if (!routed && NavySeal(ai) == false) {
            func_00561E20(ai, 1, -1);
            m_active = false;
        }
        break;
    case 1:
        if (func_005629B0(ai)) {
            if (m_pickup) {
                PickupObject__10CZSealBodyFP7CPickup(seal, m_pickup);
                m_pickup = 0;
                phase = 2;
            } else
                phase = 2;
        }
        break;
    case 2:
        m_active = false;
        break;
    }
    SetPhase(phase);
    return Result();
}

/* a surrender takes another state only once it is past its fifth phase: any, for an entity of the
 * 0x20000 team bit, else kinds 23 and 28 (OVL2 0x607D10) */
bool CAiSSurrender::CanPushAiState(AI_STATE state, CSealCtrlAi *ai)
{
    bool ok = false;

    if ((int)m_phase >= 5) {
        if (ai->m_entity->m_team & 0x20000)
            ok = true;
        else
            ok = state == 23 || state == 28;
    }
    return ok;
}

/* a target state: the AI's mode 4 on entry, its goal cleared on the way out (OVL2 0x604C60;
 * SOCOM 1's) */
void CAiSTarget::OnSignal(SIGNAL sig, CSealCtrlAi *ai)
{
    switch (sig) {
    case SIGNAL_4:
        break;
    case SIGNAL_0:
        ai->SetAiMode((AI_MODE)4);
        break;
    case SIGNAL_1:
        break;
    case SIGNAL_2:
        break;
    case SIGNAL_3:
        ai->ClearGoal(GOAL_NONE);
        break;
    }
}

/* the controller's target (CSealCtrlAi at 0x67C: SOCOM 1's CAiTarget m_target at 0x4BC, its
 * entity first; zseal.h does not name it) */
struct CAiCtrlTarget {
    unsigned char _0[0x67c];
    CEntity *m_target;   // 0x67c
};

/* a reload: weapon down, the stance kept, a crouch one time in ten and a move one in four, a look
 * at the target when there is one (the flags' bits 0-2), then the sub-states' turn (OVL2
 * 0x6085C0; SOCOM 1's reloaded itself).
 * OPEN, 0x284 against 0x27C: each chance's odds are stored to the frame (0x5C, 0x58) before
 * rand() and read back through a pointer kept in s0, as SOCOM 1's (0x3D8F5C29 at 132(sp)) --
 * here MW folds the odds into the compare (no frame slots, 80 bytes against 96). Tried: the
 * odds as a literal, a named local, through a template, a pointer, a local reference. The
 * compare chain (2, 1, 0), the phase re-read after Virtual14 and the early seal load are the
 * target's. */
AI_RESULT CAiSReload::Tick(float dT, CSealCtrlAi *ai)
{
    CEntity *seal = ai->m_entity;
    unsigned int phase = m_phase;

    switch (phase) {
    case 0: {
        float crouch = 0.1f;
        float move = 0.25f;

        SetWeaponState__10CZSealBodyF12WEAPON_STATE(seal, 2);
        m_stance = ai->m_stance;
        if (Chance(crouch))
            SetFlags(1, 0);
        else
            SetFlags(0, 1);
        if (Chance(move))
            SetFlags(2, 0);
        else
            SetFlags(0, 2);
        if (func_0060A850(&((CAiCtrlTarget *)ai)->m_target)) {
            CEntity *t = ((CAiCtrlTarget *)ai)->m_target;

            if (func_005F0B30(ai, t ? &t->m_posW : 0, 5, -1, -1))
                SetFlags(4, 0);
        }
        phase = 1;
        break;
    }
    case 1:
    case 2:
        Virtual14(ai, 0);
        phase = m_phase;
        break;
    }
    SetPhase(phase);
    return Result();
}

/* a flight: to a cell away from danger (the goto pushed, a shout, crouched), the AI's mode 1 for
 * five seconds, then done (OVL2 0x6045B0; SOCOM 1's ran its own route) */
AI_RESULT CAiSFlee::Tick(float dT, CSealCtrlAi *ai)
{
    STEP step = (STEP)m_phase;
    CAiCell from(ai->m_entity->m_mapLocValid);

    switch (step) {
    case STEP_0: {
        void *cell = func_0052A3C0(&AiMaps, &from, 2, 0);

        if (!cell)
            step = STEP_2;
        else if (func_00529EE0(&AiMaps, cell, &m_cell)) {
            step = STEP_2;
            func_005F0830(ai, &m_cell, 4, 1, 2);
            ai->PlayAiSound((CHARACTER_SOUND)72, 0.75f, -1);
            func_005631A0(ai, 0);
            func_00570F90(ai, 1.0f);
            ai->SetTarget(0);
        } else
            step = STEP_2;
        break;
    }
    case STEP_1: {
        static float wait = 5.0f;

        ai->SetAiMode((AI_MODE)1);
        if (!(m_phaseTime < wait))
            step = STEP_4;
        break;
    }
    case STEP_3:
        step = STEP_1;
        break;
    case STEP_2:
        step = STEP_4;
        break;
    case STEP_4:
        m_active = false;
        break;
    }
    if (step != m_phase)
        StateStack(ai).SetFlags(1, 0);
    SetPhase(step);
    return Result();
}

/* a flight's reset (OVL2 0x604830; SOCOM 1's) */
void CAiSFlee::OnSignal(SIGNAL sig, CSealCtrlAi *ai)
{
    switch (sig) {
    case SIGNAL_4:
        m_60 = 0;
        m_58 = true;
        break;
    case SIGNAL_0:
        break;
    case SIGNAL_1:
        break;
    case SIGNAL_2:
        break;
    case SIGNAL_3:
        break;
    }
}

/* a hostage's reset: no captor (OVL2 0x601F20; SOCOM 1's cleared one word) */
void CAiSHostage::OnSignal(SIGNAL sig, CSealCtrlAi *ai)
{
    switch (sig) {
    case SIGNAL_4:
        m_5c = CEntity::invalidId;
        m_58 = -1;
        break;
    case SIGNAL_0:
        break;
    case SIGNAL_1:
        break;
    case SIGNAL_2:
        break;
    case SIGNAL_3:
        break;
    }
}

/* a hostage takes any state but kind 16, and kind 19 only in its phases 5 and 6 (OVL2
 * 0x601EC0) */
bool CAiSHostage::CanPushAiState(AI_STATE state, CSealCtrlAi *ai)
{
    bool ok = true;
    unsigned char phase = m_phase;

    switch (state) {
    case 19:
        ok = phase == 5 || phase == 6;
        break;
    case 16:
        ok = false;
        break;
    }
    return ok;
}

CAiSHostage::~CAiSHostage()
{
}
