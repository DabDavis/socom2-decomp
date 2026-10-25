/* seal_airesponse.cpp -- the states an AI SEAL falls into in response: hostage, avoid, rush, path
 * recovery, flee, target, event, stun response, hold, surrender, no ammo, reload, shot at (SOCOM
 * 1's unit, __sinit_seal_airesponse.cpp -- its CAiSRestrained and CAiSCorpse are gone, CAiSAvoid
 * moved in from seal_aistate.cpp, CAiSHostage is here). OVL2 0x601880-0x609DEF: the states'
 * destructors and the inline virtuals their tables pull in, the NoAmmo/Flee/Reload Ticks and seven
 * OnSignals (round 3). Defined in reverse of their image order (-inline deferred). */

#include "../gamez/zkit/zkit.h"

/* a chance taken: a random float in [0, 1) against the odds (an inline, seal_control.cpp's shape:
 * the odds by value, read through a reference -- the target stores them and reads them back) */
inline bool Chance(float odds)
{
    const float &c = odds;
    Rfloat r;

    r.m_min = 0.0f;
    r.m_range = 1.0f;
    return r <= c;
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
/* (OVL2 0x558590: the SEAL lane's row, on the controller's object at 0x324; 0x563050, a goal
 * set with a float) */
void func_00558590(void *obj);
void func_00563050(CSealCtrlAi *ai, int a, int b, float f);
/* a look or a goal at a point (OVL2 0x561110, the SEAL lane's row) */
void func_00561110(CSealCtrlAi *ai, int kind, const CPnt3D *pos, float t);
/* (OVL2 0x5628F0, 0x5B3890: the SEAL lane's rows -- the first on the controller, the second on
 * the SEAL) and a byte of the SEAL lane's (OVL2 bss 0x66AFE0, cleared by a hold) */
void func_005628F0(CSealCtrlAi *ai);
void func_005B3890(CEntity *seal, int x);
extern unsigned char data_0066AFE0;
/* the AI's mode, a byte (OVL2 0x560A60); an order given (0x571010) with a target this unit
 * picks (0x609530); the stance set after a delay (0x563100); a CSealCtrl call taking a signed
 * byte (0x600D40, this lane's range, unwritten) -- the SEAL lane's rows but 0x609530/0x600D40 */
unsigned char func_00560A60(CSealCtrlAi *ai);
void func_00571010(CSealCtrlAi *ai, void *target, int a, int kind);
void *func_00609530(int x);
void func_00563100(CSealCtrlAi *ai, signed char stance, float delay);
void func_00600D40(CSealCtrlAi *ai, int x);
/* (OVL2 0x567840, 0x570D60: the SEAL lane's rows -- a stun's controller calls, the second
 * taking a float) */
void func_00567840(CSealCtrlAi *ai, int x);
void func_00570D60(CSealCtrlAi *ai, int x, float f);
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

            *(unsigned int *)&m_pickupLoc = *(unsigned int *)&seal->m_mapLocValid;
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
    UpdatePhase(phase);
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

/* the controller's word at 0x718 (zseal.h does not name it -- the name ours) */
struct CAiCtrl718 {
    unsigned char _0[0x718];
    unsigned int m_718;   // 0x718
};

/* a random float: a minimum and a range (zkit.h's Rfloat, built in place) */
inline float Random(float min, float range)
{
    Rfloat r;

    r.m_min = min;
    r.m_range = range;
    return r;
}

/* shot at: the stance noted and a short random wait on entry, an order (kind 11) in the AI's
 * first two modes; on leaving for a sub-state the stance back after a random delay (OVL2 0x6093F0;
 * SOCOM 1's) */
void CAiSShotAt::OnSignal(SIGNAL sig, CSealCtrlAi *ai)
{
    switch (sig) {
    case SIGNAL_4:
        break;
    case SIGNAL_0:
        m_stance = ai->m_stance;
        m_time = Random(0.05f, 0.2f);
        if (func_00560A60(ai) < 2)
            func_00571010(ai, func_00609530(0), 2, 11);
        break;
    case SIGNAL_1:
        func_00563100(ai, m_stance, Random(0.25f, 0.25f));
        func_00600D40(ai, 6);
        ((CAiCtrl718 *)ai)->m_718 = 0;
        break;
    case SIGNAL_2:
        break;
    case SIGNAL_3:
        break;
    }
}

/* the controller's bit 17 at 0x170 (zseal.h's m_bit17), set through a bool: an inline setter */
inline void SetBit17(CSealCtrlAi *ai, bool on)
{
    ai->m_bit17 = on;
}

/* an event's events: reset (its points cleared, its clock to -1); on entry the AI's mode 4 and
 * the flags' bits 4 and 5; a sub-state hands it a look at one of its points (bit 8, bit 0 picking)
 * or the AI's own (bit 7); done on the way out unless bit 6 asked for more (OVL2 0x605A80;
 * SOCOM 1's) */
void CAiSEvent::OnSignal(SIGNAL sig, CSealCtrlAi *ai)
{
    switch (sig) {
    case SIGNAL_4:
        m_rnd_min = 1.0f;
        m_rnd_range = 2.0f;
        m_80 = 0.5f;
        m_58 = 0;
        m_pos = CPnt3D::zero;
        m_pos2 = CPnt3D::zero;
        m_8e = 0;
        m_88 = 0;
        m_8c = 0;
        m_8d = 0;
        m_time = -1.0f;
        break;
    case SIGNAL_0:
        ai->SetAiMode((AI_MODE)4);
        SetFlags(0x30, 0);
        break;
    case SIGNAL_1:
        if (m_flags & 0x100) {
            func_00561110(ai, 3, (m_flags & 1) ? &m_pos2 : &m_pos, -1.0f);
        } else if (m_flags & 0x80)
            func_00561110(ai, 2, 0, -1.0f);
        func_00600D40(ai, 5);
        break;
    case SIGNAL_2:
        break;
    case SIGNAL_3:
        m_active = (m_flags & 0x40) != 0;
        SetFlags(0, 0x40);
        break;
    }
}

/* the hold's signed-byte count (a file static: its Tick reads it too; OVL2 data 0x650E10) */
static signed char s_holdCount = 3;

/* a hold's events: reset; entering and leaving, the AI's goals dropped and the bit off; a sub-state
 * (1, 2) puts a goal back and the bit on (OVL2 0x607650; SOCOM 1's) */
void CAiSHold::OnSignal(SIGNAL sig, CSealCtrlAi *ai)
{
    CEntity *seal = ai ? ai->m_entity : 0;

    switch (sig) {
    case SIGNAL_4:
        m_58 = -1;
        *(unsigned int *)&m_cell = *(unsigned int *)&CAiMapLoc::invalid;
        m_78 = 0;
        m_70 = 0;
        m_6c = 0;
        break;
    case SIGNAL_0:
        func_005628F0(ai);
        ai->ClearGoal(GOAL_NONE);
        func_005B3890(seal, 1);
        SetBit17(ai, false);
        break;
    case SIGNAL_1:
        func_00563050(ai, 2, 1, -1.0f);
        func_00600D40(ai, s_holdCount);
        if (m_flags & 0x80)
            data_0066AFE0 = 0;
        ai->m_bit17 = true;
        func_005B3890(seal, 1);
        break;
    case SIGNAL_2:
        func_00563050(ai, 2, 1, -1.0f);
        ai->m_bit17 = true;
        break;
    case SIGNAL_3:
        func_005628F0(ai);
        ai->ClearGoal(GOAL_NONE);
        SetBit17(ai, false);
        if (m_flags & 0x20)
            UpdatePhase(0);
        break;
    }
}

/* the path recovery's tries (a file static: its Tick reads it too; OVL2 data 0x651000) */
static int s_pathTries = 10;

/* a path recovery's events: reset its tries; on entry the AI's mode 4 and a goal, the bit off;
 * the bit on again on leaving for a sub-state; done at the end (OVL2 0x604490; SOCOM 1's) */
void CAiSPathRecover::OnSignal(SIGNAL sig, CSealCtrlAi *ai)
{
    switch (sig) {
    case SIGNAL_4:
        m_tries = s_pathTries;
        break;
    case SIGNAL_0:
        ai->SetAiMode((AI_MODE)4);
        func_00567840(ai, 0);
        func_00558590((char *)ai + 0x324);
        func_00563050(ai, 2, 0, 0.1f);
        SetBit17(ai, false);
        m_58 = 0;
        m_time = 0.0f;
        break;
    case SIGNAL_1:
        SetBit17(ai, true);
        break;
    case SIGNAL_2:
        break;
    case SIGNAL_3:
        m_active = false;
        break;
    }
}

/* an avoidance's events: reset; up from prone on entry and on the way out; its goal dropped when
 * a sub-state takes over (1) or it is left (2, done) (OVL2 0x602D10; SOCOM 1's) */
void CAiSAvoid::OnSignal(SIGNAL sig, CSealCtrlAi *ai)
{
    switch (sig) {
    case SIGNAL_4:
        m_58 = 0;
        break;
    case SIGNAL_0:
        if (ai->m_stance == STANCE_PRONE)
            func_005631A0(ai, 1);
        break;
    case SIGNAL_2:
        ai->ClearGoal(GOAL_AVOID);
        func_00558590((char *)ai + 0x324);
        m_active = false;
        break;
    case SIGNAL_1:
        ai->ClearGoal(GOAL_AVOID);
        func_00558590((char *)ai + 0x324);
        break;
    case SIGNAL_3:
        if (ai->m_stance == STANCE_PRONE)
            func_005631A0(ai, 1);
        UpdatePhase(0);
        break;
    }
}

/* a float of the controller's (CSealCtrl at 0xD8; zseal.h does not name it -- the name ours) */
struct CAiCtrlD8 {
    unsigned char _0[0xd8];
    float m_d8;   // 0xd8
};

/* a stun response's events: reset; on entry a random wait under 0.75 and the AI's mode 2; done
 * on the way out (OVL2 0x605F90; SOCOM 1's) */
void CAiSStunResponse::OnSignal(SIGNAL sig, CSealCtrlAi *ai)
{
    switch (sig) {
    case SIGNAL_4:
        m_wait = 0.0f;
        m_5c = -1;
        m_64 = 0;
        m_time = 0.0f;
        break;
    case SIGNAL_0: {
        Rfloat r;

        r.m_min = 0.0f;
        r.m_range = 0.75f;
        m_wait = r;
        func_00567840(ai, 0);
        func_00570D60(ai, 0, 0.0f);
        ai->SetAiMode((AI_MODE)2);
        break;
    }
    case SIGNAL_1:
        func_00570D60(ai, 0, ((CAiCtrlD8 *)ai)->m_d8);
        break;
    case SIGNAL_2:
    case SIGNAL_3:
        m_active = false;
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
 * 0x6085C0; SOCOM 1's reloaded itself) */
AI_RESULT CAiSReload::Tick(float dT, CSealCtrlAi *ai)
{
    CEntity *seal = ai->m_entity;
    unsigned int phase = m_phase;

    switch (phase) {
    case 0:
        SetWeaponState__10CZSealBodyF12WEAPON_STATE(seal, 2);
        m_stance = ai->m_stance;
        if (Chance(0.1f))
            SetFlags(1, 0);
        else
            SetFlags(0, 1);
        if (Chance(0.25f))
            SetFlags(2, 0);
        else
            SetFlags(0, 2);
        /* the target object's address, spelled apart from its entity's load: taken as
         * &...->m_target MW keeps it in a saved register for both */
        if (func_0060A850((char *)ai + 0x67c)) {
            CEntity *t = ((CAiCtrlTarget *)ai)->m_target;

            if (func_005F0B30(ai, t ? &t->m_posW : 0, 5, -1, -1))
                SetFlags(4, 0);
        }
        phase = 1;
        break;
    case 1:
    case 2:
        Virtual14(ai, 0);
        phase = m_phase;
        break;
    }
    UpdatePhase(phase);
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
        else if (func_00529EE0(&AiMaps, cell, (unsigned int *)&m_cell)) {
            step = STEP_2;
            func_005F0830(ai, (unsigned int *)&m_cell, 4, 1, 2);
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
    UpdatePhase(step);
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
