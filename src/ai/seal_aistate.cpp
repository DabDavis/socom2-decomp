/* sc: compiler=mwcps2-3.0b50-030527 */
/* (the unit's build: 3.0b50, not the game's 3.0b52 -- the breach side test 0x5F3270 is b50's own Dot form, shared y-product
 * and all, and no b52 spelling of 72 reproduces it; nothing else in the unit changes under b50: the integrator, 2026-11-04) */
/* seal_aistate.cpp -- the AI states' core: CQB attack, unknown, suspended, breach, script, goto,
 * pickup (SOCOM 1's unit, __sinit_seal_aistate.cpp, with its static CAiSUnknown; SOCOM 1 filed it
 * beside the other seal_*.cpp under gamez/zseal, this tree keeps the AI states under src/ai).
 * OVL2 0x5EF9D0-0x5F8BFF: the states' destructors and the inline virtuals their tables pull in,
 * CAiStateStack's out-of-line members, CSealCtrlAi::GetAiState, the base's slot 0x0C and the
 * helpers the response states' Ticks call, the CQB/unknown/pickup Ticks and five OnSignals
 * (round 3); CAiState::Create (0x5F6550, every state's constructor inlined one level), Reset's
 * copy (0x5F6D40), CAiState::Signal (0x5F6FA0, Reset inlined whole), the stack's constructor and
 * its Pop (round 4); round 5: SOCOM 1's CSealCtrlAi stack calls moved onto CAiStateStack, the AI
 * passed (Find, Remove, Push, Set, PopTo x2, Pop(AI_STATE), PopAll, the stack's Tick -- 0x5F7920
 * to 0x5F8270), ClearAiState, the states' hold on the controller's settings (0x5F6350), slot
 * 0x34's two overrides. Still open: the flags' update (0x5F7470, 0x4B0 bytes), CanPushAiState
 * (0x5F5CD0) and StateTick (0x5F55E0); the controller's wrappers (0x5F5BA0-0x5F5CB0) are named
 * drafts until zseal.h declares them. */

#include "../gamez/zseal/zseal.h"
#include "../gamez/zanim/zanim.h"

#include <algorithm>

extern "C" {
/* (CSealCtrlAi::RestoreAiMode, OVL2 0x56D090, zseal.h's: called twice on leaving the unknown state) */
}

/* the controller's state clock: its stack's (SEAL_AI_PARAMS's stack at 0x41C, the clock 0x18 into
 * it, 0x434 -- SOCOM 1's m_stateTimer at 0x330), read through an inline */
inline float StateTimer(CSealCtrlAi *ai)
{
    return ai->m_stateStack.m_timer;
}

/* the controller's word at 0x10C (CSealCtrl's: its squad; zseal.h does not name it) */
struct CAiCtrl10C {
    unsigned char _0[0x10c];
    void *m_10c;   // 0x10c
};

/* a SEAL of the player's side (an inline bool accessor: its callers keep the sltu) */
inline bool NavySeal(CSealCtrlAi *ai)
{
    return ai->m_navyseal;
}

/* a word of masks changed: `clear` cleared, `set` set (inlined with constants; the controller's
 * word at 0x488 -- zseal.h does not name it; the name ours) */
struct CAiCtrl488 {
    unsigned char _0[0x488];
    unsigned int m_488;   // 0x488
};

inline void SetMask488(CSealCtrlAi *ai, unsigned int set, unsigned int clear)
{
    ((CAiCtrl488 *)ai)->m_488 &= ~clear;
    ((CAiCtrl488 *)ai)->m_488 |= set;
}

/* the state an empty stack answers with (SOCOM 1's _unknown, 0x509870: this unit's static; OVL2 bss
 * 0x66B990), built by the unit's initialiser __sinit_seal_aistate.cpp (OVL2 0x668F60: CAiState(0)
 * called, the table stored, ~CAiSUnknown 0x5F8800 registered -- SOCOM 1's less its CSTable, its
 * Reset inlined in CAiState's constructor) */
CAiSUnknown _unknown;

extern "C" {
/* the controller's helpers the goto's events call (OVL2 0x555810, 0x555820, 0x5558A0, 0x5558D0
 * -- the SEAL lane's rows), the stance set after a delay (0x563100) and a squad's call on the
 * controller's word at 0x10C (0x5E7660, seal_unit.cpp's, a byte-exact draft) */
bool func_00555810(CSealCtrlAi *ai);
void func_00555820(CSealCtrlAi *ai, int x);
unsigned char func_005558A0(CSealCtrlAi *ai);
void func_005558D0(CSealCtrlAi *ai);
void func_005E7660(void *unit);
/* CZSealBody's weapon state (named; CSealCtrl::ClearLook, OVL2 0x600D40, is zseal.h's) */
void SetWeaponState__10CZSealBodyF12WEAPON_STATE(CEntity *seal, int state);
/* CPickup's and the AI maps' (OVL1 0x5416C0 named; OVL2 0x52DC70, 0x529A30, the AI core's rows:
 * CAiMaps::GetLocation and GetTraversableLocation, by their mangled names) */
unsigned int GetPickupNetEntityID__7CPickupFv(CPickup *p);
bool GetLocation__7CAiMapsFPC6CPnt3DR9CAiMapLoc(CAiMaps *maps, const CPnt3D *pos, void *loc);
bool GetTraversableLocation__7CAiMapsFPC6CPnt3DR9CAiMapLoc(CAiMaps *maps, const CPnt3D *pos, void *loc);
/* the goto pushed to a cell (OVL2 0x5F0830, this unit's: the AI, the cell, a mode, a kind and
 * the step to come back to -- the reading ours) */
bool func_005F0830(CSealCtrlAi *ai, void *loc, unsigned int mode, int kind, int step);
void PickupObject__10CZSealBodyFP7CPickup(CEntity *seal, CPickup *pickup);
/* (CSealCtrlAi::SetStance, SOCOM 1's, called from the same Ticks; OVL2 0x5631A0: zseal.h's) */
/* CZSealBody::ExecuteAction_CQBAttack (named; zseal.h's CZSealBody does not declare it) */
void ExecuteAction_CQBAttack__10CZSealBodyFP10CZSealBody(CEntity *seal, CEntity *target);
}

/* the "action" state's answers (OVL2 0x5F07E0 back to 0x5F07A0): no attacking, scanning or
 * pushing a state of kind 3 while its object is held; kind 25 never */
bool CAiSAction::CanPushAiState(AI_STATE state, CSealCtrlAi *ai)
{
    bool ok = true;

    switch (state) {
    case 3:
        ok = m_84 == 0;
        break;
    case 25:
        ok = false;
        break;
    }
    return ok;
}

bool CAiSAction::CanScan() const
{
    return m_84 == 0;
}

bool CAiSAction::CanPursue(CSealCtrlAi *ai) const
{
    return false;
}

bool CAiSAction::CanAttack() const
{
    return m_84 == 0;
}

/* the object's id told an event: 1 drops it, 6 sets the flags' bit 2; the AI's stack bit 0 set
 * either way (OVL2 0x5F06B0) */
void CAiSAction::Virtual0C(CZAnim *anim, AI_SIGNAL sig, CSealCtrlAi *ai)
{
    if ((int)anim == m_84) {
        switch (sig) {
        case 1:
            m_84 = 0;
            break;
        case 6:
            SetFlags(4, 0);
            break;
        }
        ai->m_stateStack.SetFlags(1, 0);
    }
}

extern "C" {
/* an anim stopped (OVL1 0x2707C0, zanim's: SOCOM II's Stop with a flag) -- the object's holder id
 * is dropped by zseal.h's CNodeAction::Release (OVL1 0x2B49D0) and the SEAL's anim set bound
 * through zseal.h's BindAnimSet(AnimSet *, bool), OVL2 0x58C4A0 */
void func_002707C0(CZAnim *anim, bool now);
}

/* the AI's bit 0 at 0x264 set through a bool (zseal.h's m_debug: the `andi zero,1` form; the name
 * ours) */
inline void SetAiBit0(CSealCtrlAi *ai, bool b)
{
    ai->m_debug = b;
}

/* the "action" state told: a reset clears its object, point, anims and cells; on entry the SEAL's
 * anim set is kept and the AI's bit 0 cleared; leaving (1, the object released first, or 2) a held
 * anim is stopped, the stack marked, the anim set bound back and the step put to 0 -- or 9 past the
 * wait (7) unless its bit 2 -- the stack marked again; 3 marks it and releases the object (OVL2
 * 0x5F0470; SOCOM 1's reset only, the rest new). The step's two compares read through a const view
 * share one load, and UpdatePhase reloads it (read in place, MW reuses the compare's register; a
 * switch over the step makes a jump table) */
void CAiSAction::OnSignal(SIGNAL sig, CSealCtrlAi *ai)
{
    CEntity *seal = ai ? ai->m_entity : 0;

    switch (sig) {
    case SIGNAL_4:
        m_58 = 0;
        m_60 = CPnt3D::zero;
        m_84 = 0;
        m_animset = 0;
        m_6c = 0;
        *(unsigned int *)&m_70 = *(unsigned int *)&CAiMapLoc::invalid;
        *(unsigned int *)&m_74 = *(unsigned int *)&m_70;
        m_78 = 0;
        break;
    case SIGNAL_0:
        m_animset = ((CZSealBody *)seal)->m_animset;
        SetAiBit0(ai, false);
        break;
    case SIGNAL_1:
        if (m_58)
            m_58->Release(seal->m_id);
    case SIGNAL_2:
        if (m_84 && (m_flags & 4)) {
            func_002707C0((CZAnim *)m_84, false);
            m_84 = 0;
            m_78 = 0;
        }
        ai->m_stateStack.SetFlags(1, 0);
        ((CZSealBody *)seal)->BindAnimSet(m_animset, false);
        if (((const CAiState *)this)->m_phase < 7)
            UpdatePhase(0);
        else if (((const CAiState *)this)->m_phase == 7) {
            if (m_flags & 4)
                UpdatePhase(0);
        } else
            UpdatePhase(9);
        ai->m_stateStack.SetFlags(1, 0);
        break;
    case SIGNAL_3:
        ai->m_stateStack.SetFlags(1, 0);
        if (m_58)
            m_58->Release(ai->m_entity->m_id);
        break;
    }
}

/* an event's kind as the states read it (ai.h's CAiEvent::m_type, the halfword at 0x1A, bits
 * 1-15: this unit cannot include ai.h, whose CAiMaps zseal.h's stand-in rivals) and the kinds
 * they test, a signed byte as ai.h's EVENT is (the names ai.h's) */
enum AI_EVENT_KIND { AIEK_NONE = -1, AIEK_ENTITY = 3, AIEK_WEPHIT = 6, AIEK_WEPFIRE = 7, AIEK_LAST = 127 };

struct CAiEventKind {
    unsigned char _0[0x18];
    unsigned int m_id : 16;   // 0x18
    unsigned int m_visible : 1;   // 0x1a
    unsigned int m_type : 15;
};

/* +0x34: an event taken up while the phase is under 6, else a friend's sighting or weapon hit
 * (OVL2 0x5F0740) */
bool CAiSAction::Virtual34(CSealCtrlAi *ai, CAiEvent *e, CZSealBody *seal, bool enemy)
{
    bool r = false;

    if (m_phase < 6)
        r = true;
    else if (!enemy) {
        switch ((AI_EVENT_KIND)((CAiEventKind *)e)->m_type) {
        case AIEK_WEPHIT:
        case AIEK_ENTITY:
            r = true;
            break;
        }
    }
    return r;
}

/* +0x38: the state's phase word below 6 (the name ours) */
bool CAiSAction::Virtual38(CSealCtrlAi *ai)
{
    return m_phase < 6;
}

/* the AI map's action as the action state reads it (ai.h's CAiMap::action, which this unit cannot
 * include: its kind, none once marked invalid -- GetType) and the valve an action holds (its word at
 * 4, read as a halfword: on or off) -- the views ours */
struct CActionView {
    CAiMapLoc m_loc;   // 0x0
    unsigned int m_direction : 4;   // 0x4
    unsigned int m_actionId : 16;
    unsigned int m_state : 3;
    unsigned int m_type : 5;
    unsigned int m_locExValid : 1;
    unsigned int m_offset : 4;   // 0x8
    unsigned int m_type_invalid : 1;

    enum TYPE { NONE = 0, DOOR = 1, TYPE_MAX = 31 };

    TYPE GetType() const { return m_type_invalid == 0 ? (TYPE)m_type : NONE; }
};

struct CActionValveView {
    unsigned char _0[4];
    unsigned short m_4;   // 0x4
};

extern "C" {
/* the AI maps' action of an id with its cell, a traversable cell near one, and a map's own test
 * (ai_maps.h's / ai.h's: OVL2 0x52DAA0, 0x529300, 0x52A090 -- by their names, as this unit cannot
 * include ai.h) */
bool GetActionById__7CAiMapsFUiR9CAiMapLocPPQ26CAiMap6action(CAiMaps *maps, unsigned int id, CAiMapLoc &loc,
                                                             CActionView **pa);
bool FindTraversable__7CAiMapsFRC9CAiMapLocR9CAiMapLoc(CAiMaps *maps, const CAiMapLoc &loc, CAiMapLoc &out);
bool IsTraversable__6CAiMapCFRC9CAiMapLoc(const CAiMap *map, const CAiMapLoc &loc);
}

/* whether a cell can be walked, none for no map (seal_ai.cpp's CellTraversable) */
static inline bool ActionCellTraversable(const CAiMapLoc &loc)
{
    CAiMap *map = AiMaps.GetMap(loc);

    return map ? IsTraversable__6CAiMapCFRC9CAiMapLoc(map, loc) : false;
}

/* the action state given its node action and mode (-1: the action's valve says): the action taken
 * for the SEAL (flag 8 set when another holds it), its AI map action found with its cell -- a
 * traversable cell near it when the cell is not -- and its kind kept (flag 16 for kinds 2 and 1);
 * without its AI map action, or a cell to stand in, the state ends (step 9) (OVL2 0x5F0180, before
 * GetStatus: new in SOCOM II, the name ours). The user's id read first (the target loads it ahead of the
 * stores), the kind through the typed GetType (its andi) and tested in a switch (`beq` to the set for
 * each, `b` past it; an `||` falls into the set) */
extern "C" void func_005F0180(CAiSAction *s, CSealCtrlAi *ai, CNodeAction *na, int mode)
{
    unsigned int id = ai->m_entity->m_id;

    s->m_58 = na;
    s->m_5c = mode;
    if (!na)
        return;
    bool ok = true;

    if (!na->Acquire(id))
        s->SetFlags(8, 0);
    CActionView *act = 0;
    CActionValveView *valve = (CActionValveView *)na->m_valve;

    if (!GetActionById__7CAiMapsFUiR9CAiMapLocPPQ26CAiMap6action(&AiMaps, na->m_id, s->m_70, &act))
        ok = false;
    else {
        if (s->m_5c == -1)
            s->m_5c = !(bool)valve->m_4;
        *(unsigned int *)&s->m_74 = *(unsigned int *)&s->m_70;
        if (!ActionCellTraversable(s->m_70) && !FindTraversable__7CAiMapsFRC9CAiMapLocR9CAiMapLoc(&AiMaps, s->m_70, s->m_74))
            ok = false;
        s->m_7c = act->GetType();
        switch (s->m_7c) {
        case 1:
        case 2:
            s->SetFlags(16, 0);
            break;
        }
    }
    if (!ok) {
        s->UpdatePhase(9);
        s->m_active = false;
    }
}

/* what the AI's line walk carries and hands back (seal_ai.cpp's SEAL_LINEWALK, 0x56A130's: a copy of
 * its layout and inline constructor; the names seal_ai.cpp's) */
struct SEAL_LINEWALK {
    SEAL_LINEWALK()
        : m_blocked(0), _1(0), m_met(0), m_metid(CEntity::invalidId), m_8(false), m_restrained(false),
          m_dist(30.0f), m_to(CAiMapLoc::invalid), m_28(0), m_ai(0), m_30(true)
    {
    }

    unsigned char m_blocked;   // 0x0
    unsigned char _1;
    unsigned char m_met;   // 0x2: an entity in the way
    unsigned int m_metid;   // 0x4
    bool m_8;   // 0x8
    bool m_restrained;   // 0x9
    CAiMapLoc m_metloc;   // 0xc
    float m_dist;   // 0x10
    float m_scale;   // 0x14
    CAiMapLoc m_to;   // 0x18
    CPnt3D m_back;   // 0x1c
    float m_28;   // 0x28
    CSealCtrlAi *m_ai;   // 0x2c
    bool m_30;   // 0x30
};

extern "C" {
/* the controller's: the state's hold on its settings (0x560340), its stop within a range (0x5621F0),
 * whether its route ran out (0x5629B0), its look's agreement (0x600CF0), the action's end reached
 * (0x56C010), its line walk (0x56A130, seal_ai.cpp's) -- the SEAL lane's rows -- and the AI maps'
 * action on a cell (ai_maps.h's, OVL2 0x52D8B0, by its name) */
void func_00560340(CSealCtrlAi *ai, CAiState *state, bool b);
void func_005621F0(CSealCtrlAi *ai, float range);
bool func_005629B0(CSealCtrlAi *ai);
float func_00600CF0(CSealCtrlAi *ai);
void func_0056C010(CSealCtrlAi *ai);
bool func_0056A130(CSealCtrlAi *ai, const CAiMapLoc &to, SEAL_LINEWALK *w, float dist);
CAiMap::action *GetCellAction__7CAiMapsFRC9CAiMapLoc(CAiMaps *maps, const CAiMapLoc &loc);
}

/* the door's anim index for a SEAL at an action (seal_unit.cpp's, OVL2 0x5EC4E0: unsigned, as
 * seal_control.cpp declares it -- passed on with no narrowing) */
unsigned int CanUseDoorAction(CZSealBody *seal, CNodeAction *action, float f);

/* two points differ when any coordinate does (inline: materialised x, then y, then z) */
static inline bool operator!=(const CPnt3D &a, const CPnt3D &b)
{
    return a.x != b.x || a.y != b.y || a.z != b.z;
}

/* the action's tries at its cell, and the look's agreement it waits on (OVL2 data) */
extern int data_00650BE0;
extern float data_00650BE8;

/* the action's frame (OVL2 0x5EFA50; SOCOM 1's grown): over once the state is off, has no action
 * or its valve stands as wanted; a point to step to first (0, 1); the route to the action's cell
 * (2, 3), its look (4, 5), the action taken (6) and waited on (7); 8 the end, 9 a failure */
int CAiSAction::Tick(CSealCtrlAi *ai)
{
    STEP step;
    CZSealBody *seal = (CZSealBody *)ai->m_entity;

    if (!m_active || !m_58)
        return m_done = AI_DONE;
    CValve *valve = m_58->m_valve;

    step = (STEP)m_phase;
    if (!valve || m_5c == ((CActionValveView *)valve)->m_4)
        return m_done = AI_DONE;
    switch (step) {
    case STEP_0:
        step = (m_flags & 2) ? STEP_6 : STEP_2;
        if (m_60 != CPnt3D::zero) {
            ai->SetGoalPos(GOAL_MOVE, &m_60, 1.0f);
            ai->m_goal_timer.m_timer = 0.25f;
            ai->m_goal_timer.m_min = 0.25f;
            ai->m_goal_timer.m_range = 0.0f;
            ai->SetAiMode((AI_MODE)5);
            step = STEP_1;
        }
        break;
    case STEP_1: {
        bool has = (bool)ai->m_goal_dir_valid || (bool)ai->m_goal_pos_valid;

        if (!has || ai->m_goal_type != GOAL_MOVE)
            step = (m_flags & 2) ? STEP_6 : STEP_2;
        break;
    }
    case STEP_2:
        if (m_6c < data_00650BE0 && ai->SetRoute(m_74)) {
            unsigned char st = ai->m_path.m_state;

            step = (st == 1 || st == 2 || st == 3) ? STEP_3 : STEP_4;
            func_00560340(ai, this, true);
            if (m_70 == m_74)
                func_005621F0(ai, 10.0f);
        } else
            step = STEP_9;
        break;
    case STEP_3: {
        CAiPathBytes *path = &ai->m_path;
        unsigned char st = path->m_state;

        if (!(st == 1 || st == 2 || st == 3)) {
            bool ok = !(path->m_9c == 0) && st == 4;

            if (ok) {
                ai->SetAiMode((AI_MODE)3);
                step = STEP_4;
                ((CAiPath *)path)->PrintPath("dbg_path");
            } else
                step = STEP_9;
        }
        break;
    }
    case STEP_4:
        if (func_005629B0(ai)) {
            step = STEP_6;
            if (!(m_flags & 0x10)) {
                bool ok = false;
                CAiMap::action *act = GetCellAction__7CAiMapsFRC9CAiMapLoc(&AiMaps, m_70);

                if (act) {
                    CAiMap::DIR dir = (CAiMap::DIR)act->m_direction;

                    if (dir != 8)
                        ok = ai->SetLookDirW((AI_LOOK)3, dir);
                    else
                        ok = ai->SetLookPosW((AI_LOOK)3, m_70);
                }
                if (ok) {
                    ai->SetLookTimer(0.45f);
                    ai->ModifyLookFlags(1, 0);
                } else
                    step = STEP_5;
            }
        } else if (m_7c == 1) {
            CAiPathBytes *path = &ai->m_path;
            bool ok = !(path->m_9c == 0) && path->m_state == 4;

            if (ok) {
                const CAiMapLoc &last = (*(CAiMapLoc **)&path->_a0)[((int)path->m_9c - 1) * 2];

                if (last == m_70)
                    func_0056C010(ai);
            }
        }
        break;
    case STEP_5:
        if (ai->m_look_mode == 3) {
            if (!(func_00600CF0(ai) < data_00650BE8))
                step = STEP_6;
        } else
            step = STEP_6;
        break;
    case STEP_6:
        ai->SetAiMode((AI_MODE)4);
        if (!(m_flags & 2)) {
            SEAL_LINEWALK w;

            if (func_0056A130(ai, m_74, &w, 2.0f) && w.m_met) {
                m_6c++;
                step = STEP_2;
            }
        }
        if (step == STEP_6) {
            ai->ClearGoal(GOAL_NONE);
            unsigned int idx = 0;

            if (m_flags & 1)
                idx = CanUseDoorAction(seal, m_58, 1.0f);
            if (m_58->ExecuteAction(seal, idx)) {
                step = STEP_7;
                m_84 = (int)m_58->m_10;
                m_78 = m_58->_3e;
            } else
                step = STEP_8;
        }
        break;
    case STEP_7:
        if ((unsigned int)m_78 < m_58->_3e)
            step = STEP_8;
        break;
    }
    if (step == STEP_8)
        SetActive(false);
    else if (step == STEP_9) {
        func_00560340(ai, this, false);
        SetActive(false);
    }
    if (step != m_phase)
        ai->m_stateStack.SetFlags(1, 0);
    UpdatePhase(step);
    return Result();
}

/* a script takes kinds 12 and 26 only when it may be pursued from (OVL2 0x5F40F0; byte-exact in the
 * kit lane's round 33: Pursuable's result local zeroed ahead of the tests, set by the call's test
 * with a self-assigning else arm -- without it MW if-converts the test into an sltu on the call's
 * result, 0x6C; `else r = false` leaves a `b` over the vanished arm) */
bool CAiSScript::CanPushAiState(AI_STATE state, CSealCtrlAi *ai)
{
    bool ok = true;

    switch (state) {
    case 12:
    case 26:
        ok = Pursuable(ai);
        break;
    }
    return ok;
}

/* the script's own sequence told an event: 1 ends it (its id cleared), 2 holds it and 3 lets it
 * go, the byte at 0x5F made 255 either way (OVL2 0x5F3FF0, CAiSScript's slot 0x0C) */
void CAiSScript::Virtual0C(CZAnim *anim, AI_SIGNAL sig, CSealCtrlAi *ai)
{
    if ((unsigned int)anim == m_58) {
        switch (sig) {
        case 1:
            m_58 = 0;
            break;
        case 2:
            SetHold(true);
            m_5f = 255;
            break;
        case 3:
            SetHold(false);
            m_5f = 255;
            break;
        }
    }
}

/* the script's bit 3 set, then the controller's 0x555820 told 0 (OVL2 0x5F3FC0: a CAiSScript
 * member -- `this` in a0 -- unnamed; three of the controller's script functions call it) */
extern "C" void func_005F3FC0(CAiSScript *script, CSealCtrlAi *ai, bool on)
{
    script->m_bit3 = on;
    func_00555820(ai, 0);
}

/* pursued from: in its own AI mode and half a second on (OVL2 0x5F4090; SOCOM 1's 0x6C bytes) */
bool CAiSScript::CanPursue(CSealCtrlAi *ai) const
{
    return m_mode == ai->GetAiAware() && m_60 >= 0.5f;
}

/* a script sequence run on the AI's script state, when that is the current one: whether it began
 * (OVL2 0x5F43B0; the event state's Tick calls it -- a controller helper, unnamed).
 * OPEN, 39 instructions, all Is()'s: the same materialised `&&` as the stack's Pop(ai, AI_STATE)
 * (below) -- and here the target reads the stack's size again for GetAiState after it, where MW
 * keeps the first load; Is() const changes nothing. The rest (GetAiState and the script cast
 * inlined, the call, the andi) is the target's shape */
extern "C" bool func_005F43B0(CSealCtrlAi *ai, const char *name, unsigned int flags)
{
    if (ai->m_stateStack.Is(AI_STATE_SCRIPT)) {
        CAiState *s = ai->GetAiState();
        CAiSScript *script = s && s->m_state == AI_STATE_SCRIPT ? (CAiSScript *)s : 0;

        return script->ActivateSequence(name, flags);
    }
    return false;
}

extern "C" char *strncpy(char *dst, const char *src, unsigned int n);

/* an anim paused (the byte at 2 as a bool: materialised, sltu, then branched on -- zanim.h's CZAnim
 * has no accessor; the name ours) */
static inline bool AnimPaused(const CZAnim *anim)
{
    return anim->m_paused ? true : false;
}

/* the AI's sequence names (SOCOM 1's namespace sealai; OVL2 0x575210, seal_ai.cpp's) */
namespace sealai {
const char *get_aiseq(unsigned int i);
}

/* a response sequence run on the script's anim (m_58: ActivateEx stores it): with a name, while the
 * state runs, unheld (or flags bit 3), when the anim has the sequence -- bit 2 resumes a paused anim
 * first (false when it is not paused) and marks the flags' bit 2; the AI's own sequence paused, this
 * one activated, held by bit 0, the mode cleared (255) by bit 1, the name kept; whether it began
 * (OVL2 0x5F41F0; SOCOM 1's, with SOCOM II's bit 3 and Resume's new argument) */
bool CAiSScript::ActivateSequence(const char *name, unsigned int flags)
{
    bool r = false;

    if (name && m_active && ((flags & 8) || !Held()) && m_58
        && ((CZAnim *)m_58)->SequenceExists(name)) {
        if (flags & 4) {
            if (!AnimPaused((CZAnim *)m_58))
                return false;
            func_002709B0((CZAnim *)m_58, 0);
            SetFlags(4, 0);
        }
        ((CZAnim *)m_58)->PauseSequence(sealai::get_aiseq(m_seq));
        ((CZAnim *)m_58)->ActivateSequence(name);
        SetHold(flags & 1);
        if (flags & 2)
            m_mode = 255;
        strncpy(m_seqName, name, 16);
        r = true;
    }
    return r;
}

/* (OVL2 0x5F4080) */
bool CAiSScript::CanAttack() const
{
    return true;
}

/* an AI's bit 6 as a bool (inline: the `? true : false` keeps the target's sltu; the name ours) */
inline bool AiBit6(CSealCtrlAi *ai)
{
    return ai->m_aibit6 ? true : false;
}

/* a script's events: a reset, an entry, the anim stopped on leaving (unless it runs on past a dead SEAL), the state back on
 * top -- the mode restored, a response sequence that held dropped, the anim resumed, the set back -- or a state pushed
 * above it -- the mode and the set kept, the anim paused -- and its sequence paused or resumed for the anim's own signals
 * (OVL2 0x5F3BA0; SOCOM 1's) */
void CAiSScript::OnSignal(SIGNAL sig, CSealCtrlAi *ai)
{
    CZSealBody *seal = ai ? (CZSealBody *)ai->m_entity : 0;

    switch (sig) {
    case SIGNAL_4:
        m_seqName[0] = 0;
        SetHold(false);
        m_held_once = true;
        m_mode = 255;
        m_seq = 255;
        m_5f = 255;
        m_mode_saved = 1;
        m_58 = 0;
        m_60 = 0.0f;
        m_keep_anim = false;
        m_78 = 0;
        m_animset = 0;
        m_80 = 0;
        m_bit3 = false;
        break;
    case SIGNAL_0:
        SetHold(false);
        m_mode = 255;
        m_seq = 255;
        m_5f = 255;
        m_held_once = true;
        m_60 = 0.0f;
        m_78 = 0;
        m_bit3 = false;
        break;
    case SIGNAL_1:
        if (m_58 && (!m_keep_anim || !seal->m_isAlive))
            func_002707C0((CZAnim *)m_58, false);
        if (!AiBit6(ai))
            seal->BindAnimSet(seal->m_530, false);
        break;
    case SIGNAL_3:
        ai->SetAiMode(*(AI_MODE *)&m_mode_saved);
        if (m_58) {
            if (m_flags & 4) {
                if (m_seqName[0]) {
                    ((CZAnim *)m_58)->SequenceRunning(m_seqName);
                    const char *seq = sealai::get_aiseq(m_seq);

                    m_held_once = false;
                    ((CZAnim *)m_58)->StopSequence(m_seqName);
                    ((CZAnim *)m_58)->ResumeSequence(seq);
                    SetHold(false);
                    m_seqName[0] = 0;
                }
                m_flags |= 0x80000000;
                m_flags &= ~4;
            }
            m_held_once = true;
            m_60 = 0.0f;
            func_002709B0((CZAnim *)m_58, 0);
        }
        if (seal->m_isAlive && m_bit3)
            func_005558A0(ai);
        else
            func_00555820(ai, 0);
        if (m_78) {
            if (seal->m_isAlive)
                seal->BindAnimSet(m_78, false);
            m_78 = 0;
        }
        break;
    case SIGNAL_2:
        *(AI_MODE *)&m_mode_saved = ai->m_3f4;
        if (m_58)
            func_00270BE0((CZAnim *)m_58, 0);
        if (m_bit3)
            func_005558D0(ai);
        else
            func_00555820(ai, 0);
        if (!AiBit6(ai)) {
            m_78 = seal->m_animset;
            seal->BindAnimSet(seal->m_530, false);
        }
        break;
    case 5:
        if (m_58)
            ((CZAnim *)m_58)->PauseSequence(sealai::get_aiseq(m_mode));
        break;
    case 6:
        if (m_58)
            ((CZAnim *)m_58)->ResumeSequence(sealai::get_aiseq(m_mode));
        break;
    }
}

/* the waits before a higher alertness shows (a table of floats by AI_AWARE: OVL2 data) */
extern float data_00650B80[];

/* the response sequence's progress: while it runs, false; once it has run out, the AI's own sequence resumed (the name
 * ours; inlined) */
inline bool ResponseRunning(CAiSScript *s)
{
    return s->m_seqName[0] != 0 && ((CZAnim *)s->m_58)->SequenceRunning(s->m_seqName);
}

inline bool ResponseDone(CAiSScript *s)
{
    if (ResponseRunning(s))
        return false;
    if (s->m_seqName[0] && !((CZAnim *)s->m_58)->SequenceRunning(s->m_seqName)) {
        const char *seq = sealai::get_aiseq(s->m_seq);

        s->m_held_once = false;
        ((CZAnim *)s->m_58)->StopSequence(s->m_seqName);
        ((CZAnim *)s->m_58)->ResumeSequence(seq);
        s->SetHold(false);
        s->m_seqName[0] = 0;
    }
    return true;
}

/* the script's frame: done without an anim or once it stops; else the response sequence first, then the sequence of the
 * AI's alertness -- changed when the alertness has held long enough, a rise from 0 to 1 pausing the sequence (the
 * SEAL's anim set kept), a fall from 1 to 0 resuming it (OVL2 0x5F37C0; SOCOM 1's) */
inline CZSealBody *ScriptSeal(CSealCtrlAi *ai) { return (CZSealBody *)ai->m_entity; }
inline const char *ScriptSeq(int i) { return sealai::get_aiseq(i); }
int CAiSScript::Tick(CSealCtrlAi *ai)
{
    CZAnim *anim = (CZAnim *)m_58;
    CZSealBody *seal = ScriptSeal(ai);

    if (!anim) {
        SetActive(false);
        return Result();
    }
    anim->GetName();
    if (!anim->Running()) {
        SetActive(false);
        return Result();
    }
    if (!ResponseDone(this))
        return Result();
    if (!m_held_once) {
        int mode = m_mode;
        int aware = (unsigned char)ai->GetAiAware();
        float wait = mode < aware ? data_00650B80[aware] : 0.0f;

        if (ai->m_5c8 > wait && mode != aware) {
            if (!m_hold) {
                const char *cur = sealai::get_aiseq(m_mode);
                const char *next = sealai::get_aiseq(aware);

                bool exists = anim->SequenceExists(next);

                if (exists || (mode < 2 && aware > 1)) {
                    if (mode == 0 && aware == 1) {
                        m_animset = seal->m_animset;
                        anim->PauseSequence(cur);
                    } else if (aware == 0 && mode == 1) {
                        anim->ResumeSequence(next);
                        if (m_animset) {
                            seal->BindAnimSet(m_animset, false);
                            m_animset = 0;
                        }
                    } else {
                        anim->StopSequence(cur);
                        anim->ActivateSequence(next);
                        m_bit3 = false;
                        func_00555820(ai, 0);
                    }
                    m_60 = 0.0f;
                    m_seq = aware;
                } else
                    m_60 = 0.5f;
                m_mode = aware;
            } else if (m_5f != aware)
                m_5f = aware;
        } else
            m_60 += m_dT;
    } else
        m_held_once = false;
    return Result();
}

/* the plan's route handed to the controller (OVL2 0x573110, the SEAL lane's row) and given back
 * to the AI maps (0x52C310, the AI core's) */
extern "C" void func_00573110(CSealCtrlAi *ai, void *path);
extern "C" void func_0052C310(CAiMaps *maps, void *path);

/* the plan's goal cell set (OVL2 0x5F1760, a CAiSPlan member: the plan in a0 -- unnamed) */
extern "C" void func_005F1760(CAiSPlan *plan, void *loc);

/* the plan's route begun to its goal cells (its vector at 0x58): a planner it was handed given to
 * its path; with the flag bit 3 the cells taken leg by leg -- one that fails steps the plan to 3
 * and answers 4 -- the step then 1 either way; else the whole run registered with the AI maps from
 * the SEAL's last valid cell, the plan's limits handed on (0x11C the search limit, 0x120 the
 * path's word at 0x90, 0x130 the tolerance when positive) and the step 1, or 3 when it failed. 4
 * with no goals, else 0 or 4 (OVL2 0x5F15A0: CAiSPlan::Tick calls it, a member -- the plan in a0;
 * unnamed). The goals through ai_states.h's member view of the vector, its operator[] taking the data
 * into a const local: a pointer cast of the plan to a view makes MW hoist the data's address out of
 * the loop (the target folds it into each load), the data read straight strength-reduces the index */
extern "C" int func_005F15A0(CAiSPlan *plan, CSealCtrlAi *ai)
{
    int r = 4;
    int n = plan->m_goals.m_size;

    if (n) {
        CEntity *seal = ai->m_entity;

        if (plan->m_10c) {
            ((CAiPath *)plan->m_path)->SetPlanner((CAiPlanner *)plan->m_10c);
            plan->m_10c = 0;
            plan->m_110 = 0;
        }
        if (plan->m_flags & 8) {
            r = 0;
            for (int i = 0; i < n; i++) {
                CAiMap *map = AiMaps.GetMapByIndex(plan->m_goals[i].m_mapid);

                if (!((CAiPath *)plan->m_path)->AppendPathLoc(plan->m_goals[i], map)) {
                    plan->UpdatePhase(3);
                    r = 4;
                    break;
                }
            }
            plan->UpdatePhase(1);
        } else if (AiMaps.RegisterPathPlan(seal->m_mapLocValid, plan->m_goals.m_data, n,
                       (CAiPath *)plan->m_path)) {
            if (plan->m_11c)
                ((CAiPath *)plan->m_path)->m_search_limit = plan->m_11c;
            if (plan->m_120)
                ((CAiPath *)plan->m_path)->m_unk90 = plan->m_120;
            if (plan->m_130 > 0)
                ((CAiPath *)plan->m_path)->m_tolerance = plan->m_130;
            plan->UpdatePhase(1);
            r = 0;
        } else
            plan->UpdatePhase(3);
    }
    return r;
}

extern "C" {
/* the controller's hold on a route's end (OVL2 0x562790), its stop within a range (0x5621F0) and whether its route has
 * run out (0x5629B0): the SEAL lane's rows */
bool func_00562790(CSealCtrlAi *ai, void *path);
void func_005621F0(CSealCtrlAi *ai, float range);
bool func_005629B0(CSealCtrlAi *ai);
void func_00570F90(CSealCtrlAi *ai, float t);
float GetDistance__7CAiMapsFRC9CAiMapLocRC9CAiMapLoc(CAiMaps *maps, const CAiMapLoc *a, const CAiMapLoc *b);
}

/* the margin on a plan's tolerance and the tolerance with none (OVL2 data) */
extern float data_00650BC8;
extern int data_00650BD0;

/* whether the plan's route arrived (inline; the name ours) */
inline bool PlanArrived(CAiSPlan *p)
{
    return !(p->m_100 == 0) && p->m_pathState == 4;
}

/* the state the plan covers stepped on, to the step for success or for failure, when it has one (inline; the names ours) */
inline void PlanSucceeded(CAiSPlan *p)
{
    int k;
    CAiState *s = (CAiState *)p->m_114;

    if (s) {
        k = p->m_124;
        if (k != -1)
            s->UpdatePhase(k);
    }
}

inline void PlanFailed(CAiSPlan *p)
{
    int k;
    CAiState *s = (CAiState *)p->m_114;

    if (s) {
        k = p->m_128;
        if (k != -1)
            s->UpdatePhase(k);
    }
}

/* whether the AI holds the route's end (inline; the name ours) */
inline bool PlanHolds(CAiSPlan *p, CSealCtrlAi *ai)
{
    bool a = !(p->m_100 == 0);

    if (a)
        a = p->m_pathState == 4;
    bool ok = a;

    if (a)
        ok = func_00562790(ai, p->m_path);
    return ok;
}

/* the route's end taken: the plan's block freed, the state it covers stepped; 3 when the AI holds the route's end, else 4
 * (inline; the name ours) */
inline int PlanEnd(CAiSPlan *p, CSealCtrlAi *ai)
{
    int r;

    if (p->m_10c) {
        delete p->m_10c;
        p->m_10c = 0;
    }
    bool ok = PlanHolds(p, ai);

    r = 4;
    if (ok) {
        PlanSucceeded(p);
        r = 3;
    } else
        PlanFailed(p);
    return r;
}

/* the stop's range: 3 within it, else 4, the state the plan covers stepped (inline; the name ours) */
inline int PlanInRange(CAiSPlan *p, CEntity *seal)
{
    const CAiMapLoc *to = PlanArrived(p) ? p->m_104 : &CAiMapLoc::invalid;
    float d = GetDistance__7CAiMapsFRC9CAiMapLocRC9CAiMapLoc(&AiMaps, &seal->m_mapLocValid, to);
    int tol = p->m_130;
    int r;

    tol = tol < 0 ? data_00650BD0 : tol;
    r = 4;
    if (d <= data_00650BC8 * (10.0f * (float)tol)) {
        PlanSucceeded(p);
        r = 3;
    } else
        PlanFailed(p);
    return r;
}

/* a plan's frame: its route begun (step 0), then run; at its end the state it covers stepped and, flag 2, the AI stands
 * guard there -- flag 0 and 1 its bits -- until the stop's range is left (step 2); a failed plan ends with 4 (OVL2
 * 0x5F1160; new in SOCOM II) */
int CAiSPlan::Tick(CSealCtrlAi *ai)
{
    int r;
    STEP step = (STEP)m_phase;
    CEntity *seal = ai->m_entity;

    r = 0;
    switch (step) {
    case STEP_0:
        r = func_005F15A0(this, ai);
        if (!r)
            step = (STEP)m_phase;
        break;
    case STEP_1:
        if (m_pathState != 1 && !(m_pathState == 1 || m_pathState == 2 || m_pathState == 3)) {
            if (PlanArrived(this)) {
                r = PlanEnd(this, ai);
                if (r == 3 && (m_flags & 4)) {
                    ai->SetAiMode((AI_MODE)3);
                    func_00570F90(ai, m_12c);
                    ai->SetAiBit2((m_flags & 1) != 0);
                    ai->m_aibit3 = (m_flags & 2) != 0;
                    func_005621F0(ai, 10.0f * (float)m_130);
                    r = 0;
                    step = STEP_2;
                }
            } else
                r = PlanEnd(this, ai);
        }
        break;
    case STEP_2:
        if (func_005629B0(ai))
            r = PlanInRange(this, seal);
        break;
    case STEP_3:
        r = 4;
        break;
    }
    UpdatePhase(step);
    m_done = r;
    return r;
}

/* the plan's path flags (its CAiPath's word at 0x1C, the plan's 0x80) changed: `clear` cleared,
 * `set` set (inlined with constants, as the controller's own path's at 0x488; the name ours) */
inline void SetPathFlags(CAiSPlan *plan, unsigned int set, unsigned int clear)
{
    *(unsigned int *)&plan->m_path[0x1c] &= ~clear;
    *(unsigned int *)&plan->m_path[0x1c] |= set;
}

/* the step of the search and its first distance (OVL2 data) */
extern float data_00650BD8;

/* the first by-value difference of two points (a struct copy: a double word and a word) */
inline CPnt3D CellSearchDelta(const CPnt3D &a, const CPnt3D *b)
{
    CPnt3D d;

    a.Sub(b, &d);
    return d;
}

/* whether a cell is traversable (CAiMap::IsTraversable, ai_maps.cpp's, inlined with its two calls out of line, OVL2 0x511F80 and
 * 0x511F40 -- a level cut in the target; stand-ins by their mangled names) */
extern "C" {
CAiCell *GetCell__6CAiMapCFRC9CAiMapLoci(CAiMap *map, const CAiMapLoc &loc, int layer);
bool IsTraversable__7CAiCellCFv(CAiCell *cell);
}

inline bool CellSearchFree(CAiMap *map, const CAiMapLoc &loc)
{
    CAiCell *cell = GetCell__6CAiMapCFRC9CAiMapLoci(map, loc, 0);

    return cell ? IsTraversable__7CAiCellCFv(cell) : false;
}

/* a cell for a point: the point's own, on the SEAL's map, else the first traversable cell along the line from it toward the
 * SEAL; whether there is one (OVL2 0x5F0CD0: 0x5F0990 and 0x5F0B30 call it -- the name ours; GetLocation's inline) */
extern "C" bool func_005F0CD0(CSealCtrlAi *ai, const CPnt3D *pos, CAiMapLoc *loc)
{
    CEntity *seal;
    bool r = false;

    seal = ai->m_entity;
    CAiMap *map = AiMaps.GetMapByIndex(seal->m_mapLocValid.m_mapid);

    if (map->GetLocation(pos, *loc))
        r = true;
    else {
        CPnt3D dir = CellSearchDelta(seal->m_posW, pos);
        CPnt3D p = *pos;
        float len = dir.Normalize();

        dir.Scale(data_00650BD8, &dir);
        for (float t = data_00650BD8; t < len; t += data_00650BD8) {
            if (map->GetLocation(&p, *loc) && CellSearchFree(map, *loc)) {
                r = true;
                break;
            }
            p.Add(&dir, &p);
        }
    }
    return r;
}

/* a plan pushed on the current state's own stack, to a cell: its path's bit 5, the cell, the
 * flags `mode`, the state it covers and the kind and step to come back to; whether it was pushed
 * (OVL2 0x5F0830; the readings ours) */
extern "C" bool func_005F0830(CSealCtrlAi *ai, void *loc, unsigned int mode, int kind, int step)
{
    bool r = false;
    CAiState *s = ai->GetAiState();

    if (s->m_stack.Push(ai, AI_STATE_PLAN, 0)) {
        CAiState *top = s->m_stack.GetState();
        CAiSPlan *plan = top ? (top && top->m_state == AI_STATE_PLAN ? (CAiSPlan *)top : 0) : 0;

        SetPathFlags(plan, 0x20, 0);
        func_005F1760(plan, loc);
        plan->SetFlags(mode, 0);
        r = true;
        plan->m_114 = (unsigned int)s;
        plan->m_124 = kind;
        plan->m_128 = step;
    }
    return r;
}

/* a plan's events: reset; its route handed over on entry; on leaving for a sub-state the result
 * (4 when the route failed, or did not arrive while one was wanted) and its block freed (OVL2
 * 0x5F1800) */
void CAiSPlan::OnSignal(SIGNAL sig, CSealCtrlAi *ai)
{
    int result = 3;

    switch (sig) {
    case SIGNAL_4:
        m_10c = 0;
        m_110 = 0;
        m_114 = 0;
        m_128 = -1;
        m_124 = -1;
        m_12c = 1.0f;
        m_130 = -1;
        m_11c = 0;
        m_120 = 0;
        break;
    case SIGNAL_0:
        func_00573110(ai, m_path);
        break;
    case SIGNAL_1: {
        unsigned char st = m_pathState;

        if ((unsigned int)(st - 1) < 3) {
            func_0052C310(&AiMaps, m_path);
            result = 4;
        } else {
            bool arrived = !(m_100 == 0) && st == 4;

            if (!arrived)
                result = 4;
        }
        if (m_10c) {
            delete m_10c;
            m_10c = 0;
        }
        m_done = result;
        break;
    }
    case SIGNAL_2:
        break;
    case SIGNAL_3:
        if (m_10c) {
            delete m_10c;
            m_10c = 0;
            m_110 = 0;
        }
        break;
    }
}

/* the states seal_ai.cpp defines (the SEAL lane's unit declares them for itself): here only what
 * Create builds -- each one's size, its members' constructors and its table (stored, not emitted:
 * the destructors are seal_ai.cpp's) */
class CAiSRescued : public CAiState {
public:
    CAiSRescued() : CAiState(AI_STATE_RESCUED) {}
    virtual ~CAiSRescued();

    unsigned char _58[0x64 - 0x58];
};

class CAiSPursue : public CAiState {
public:
    CAiSPursue() : CAiState(AI_STATE_PURSUE) {}
    virtual ~CAiSPursue();

    unsigned char _58[0x60 - 0x58];
};

class CAiSAnimate : public CAiState {
public:
    /* two lists of types not known yet, built by hand (Create's copies 0x5F6F70, 0x5F6F40) */
    CAiSAnimate() : CAiState(AI_STATE_ANIMATE)
    {
        func_005F6F70(m_64);
        func_005F6F40(m_70);
    }
    virtual ~CAiSAnimate();

    unsigned char _58[0x64 - 0x58];
    unsigned char m_64[0xc];   // 0x64: a list (~CAiSAnimate erases it, 0x575CF0)
    unsigned char m_70[0xc];   // 0x70: a list (0x575F40)
};

class CAiSGrenade : public CAiState {
public:
    CAiSGrenade() : CAiState(AI_STATE_GRENADE) {}
    virtual ~CAiSGrenade();

    unsigned char _58[0x70 - 0x58];
    CPnt3D m_70;   // 0x70
    CAiMapLoc m_7c;   // 0x7c
    CAiMapLoc m_80;   // 0x80
    unsigned char _84[0x88 - 0x84];
    CPnt3D m_88;   // 0x88
    CPnt3D m_94;   // 0x94
    CPnt3D m_a0;   // 0xa0 (0xac bytes)
};

class CAiSDeploy : public CAiState {
public:
    CAiSDeploy() : CAiState(AI_STATE_DEPLOY) {}
    virtual ~CAiSDeploy();

    CPnt3D m_58;   // 0x58
    CPnt3D m_64;   // 0x64
    unsigned int m_70;   // 0x70: the item (seal_ai.cpp's)
    CAiMapLoc m_74;   // 0x74
    CAiMapLoc m_78;   // 0x78
    signed char m_7c;   // 0x7c: the stance (seal_ai.cpp's; the breach's Tick stores the door's side)
    unsigned char _7d[0x88 - 0x7d];
    float m_88;   // 0x88: a wait (seal_ai.cpp's)
    unsigned char _8c[0x9c - 0x8c];   // 0x9c bytes
};

/* a new state of a kind (OVL2 0x5F6550; SOCOM 1's, the cases in the same order -- alphabetical
 * by the state's name -- and a plain CAiState for the kinds it does not know). Every state's
 * constructor is inlined one level (CAiState's expansion is past inline_max_size): CAiState's
 * constructor and the members' are called, the table stored between them */
CAiState *CAiState::Create(AI_STATE state)
{
    CAiState *s;

    switch (state) {
    case AI_STATE_ACTION:
        s = new CAiSAction;
        break;
    case AI_STATE_ANIMATE:
        s = new CAiSAnimate;
        break;
    case AI_STATE_AVOID:
        s = new CAiSAvoid;
        break;
    case AI_STATE_BREACH:
        s = new CAiSBreach;
        break;
    case AI_STATE_CQBATTACK:
        s = new CAiSCQBAttack;
        break;
    case AI_STATE_DEPLOY:
        s = new CAiSDeploy;
        break;
    case AI_STATE_EVENT:
        s = new CAiSEvent;
        break;
    case AI_STATE_FLEE:
        s = new CAiSFlee;
        break;
    case AI_STATE_FOLLOW:
        s = new CAiSFollow;
        break;
    case AI_STATE_GOTO:
        s = new CAiSGoto;
        break;
    case AI_STATE_GRENADE:
        s = new CAiSGrenade;
        break;
    case AI_STATE_HOLD:
        s = new CAiSHold;
        break;
    case AI_STATE_HOSTAGE:
        s = new CAiSHostage;
        break;
    case AI_STATE_NOAMMO:
        s = new CAiSNoAmmo;
        break;
    case AI_STATE_PLAN:
        s = new CAiSPlan;
        break;
    case AI_STATE_PATHRECOVER:
        s = new CAiSPathRecover;
        break;
    case AI_STATE_PICKUP:
        s = new CAiSPickup;
        break;
    case AI_STATE_PURSUE:
        s = new CAiSPursue;
        break;
    case AI_STATE_RELOAD:
        s = new CAiSReload;
        break;
    case AI_STATE_RESCUED:
        s = new CAiSRescued;
        break;
    case AI_STATE_RUSH:
        s = new CAiSRush;
        break;
    case AI_STATE_SCRIPT:
        s = new CAiSScript;
        break;
    case AI_STATE_SHOTAT:
        s = new CAiSShotAt;
        break;
    case AI_STATE_STUNRESPONSE:
        s = new CAiSStunResponse;
        break;
    case AI_STATE_SURRENDER:
        s = new CAiSSurrender;
        break;
    case AI_STATE_SUSPENDED:
        s = new CAiSSuspended;
        break;
    case AI_STATE_TARGET:
        s = new CAiSTarget;
        break;
    case AI_STATE_USEVEHICLE:
        s = new CAiSUseVehicle;
        break;
    default:
        s = new CAiState(state);
        break;
    }
    return s;
}

extern "C" {
/* the state's signal handed on (OVL2 0x5F6350, unwritten: the state, the AI, the signal), the
 * stack emptied (0x5F7920, a byte-exact draft), a squad's member (CSealUnit::GetSealByIndex,
 * 0x5EDBB0: seal_unit.cpp's, by its mangled name), and the controller's calls (0x570F90; SetAiBit2
 * and SetDesiredStance, 0x56D890 and 0x5631B0, are zseal.h's) */
void func_005F6350(CAiState *s, CSealCtrlAi *ai, CAiState::SIGNAL sig);
CEntity *GetSealByIndex__9CSealUnitFii(void *unit, int a, int b);
void func_00570F90(CSealCtrlAi *ai, float t);
/* the last state popped: its result and its kind (OVL1 bss 0x44D628, 0x44D630: the stack's pop
 * stores them, the stop signal tests them -- unnamed) */
extern int data_0044D628;
extern AI_STATE data_0044D630;
void func_005F6190(CAiState *s, CSealCtrlAi *ai);
/* the stack's bits brought up to date (OVL2 0x5F7470, unwritten: the stack, the AI, a flag --
 * SOCOM 1's UpdateStackFlags?) */
void func_005F7470(CAiStateStack *stack, CSealCtrlAi *ai, int x);
}

/* a SEAL's bit the signals set (CZSealBody at 0x105D, bit 1; zseal.h does not name it -- the
 * name ours) */
struct CSealSignalBits {
    unsigned char _0[0x105d];
    unsigned char m_bit0 : 1;
    unsigned char m_signalled : 1;
};

/* the AI's focus: nothing, an entity or a point (the record at 0x660 -- zseal.h keeps it as bytes, the first
 * entity field its m_664; the view and the names ours) and where it stands, copied, or 0 (inlined; the name ours) */
struct CAiFocusView {
    unsigned char m_kind;   // 0x0
    unsigned char _1[3];
    CEntity *m_entity;   // 0x4
    CPnt3D m_point;   // 0x8
};

inline CPnt3D *FocusPos(CAiFocusView *f, CPnt3D *out)
{
    switch (f->m_kind) {
    case 1:
        {
            CEntity *e = f->m_entity;

            if (e) {
                out->x = e->m_posW.x;
                out->y = e->m_posW.y;
                out->z = e->m_posW.z;
                return out;
            }
        }
        return 0;
    case 2:
        out->x = f->m_point.x;
        out->y = f->m_point.y;
        out->z = f->m_point.z;
        return out;
    }
    return 0;
}

/* the kinds a state of a kind lets be pushed on it (inlined; the name ours) */
inline unsigned int StatePushMask(CAiState *s)
{
    unsigned int mask = 0;

    switch (s->m_state) {
    case 0: mask = 0x3FFFFFFE; break;
    case 1: mask = 0x3FFFFBD6; break;
    case 2: mask = 0x3FFFFFFA; break;
    case 3: mask = 0x010448C2; break;
    case 4: mask = 0x19054BC6; break;
    case 5: mask = 0x19044AC6; break;
    case 6: mask = 0x19874A8E; break;
    case 7: mask = 0x188D0040; break;
    case 8: mask = 0x3FFFFEFE; break;
    case 9: mask = 0x1FFFFDFE; break;
    case 10: mask = 0x0FFFFBFE; break;
    case 11: mask = 0x1FFFF7FE; break;
    case 12: mask = 0x3BFFE8F6; break;
    case 13: mask = 0x17FFDFFE; break;
    case 14: mask = 0x11040000; break;
    case 15: mask = 0x190449C6; break;
    case 16: mask = 0x1FFE48C6; break;
    case 17: mask = 0x190C48C6; break;
    case 18: mask = 0x01000000; break;
    case 19: mask = 0x31040800; break;
    case 20: mask = 0x390448C2; break;
    case 21: mask = 0x390448C2; break;
    case 22: mask = 0x390549C2; break;
    case 23: mask = 0x190F4BC0; break;
    case 24: mask = 0x40000; break;
    case 25: mask = 0x399C4942; break;
    case 26: mask = 0x390448C2; break;
    case 27: mask = 0x310468C2; break;
    case 28: mask = 0x21042000; break;
    case 29: mask = 0x1BFDE9DA; break;
    }
    return mask;
}

/* the two slots the update hands the AI to, and takes an int from (a view of the table: ai_states.h declares them
 * without the argument, which every override ignores, as SOCOM 1's names have it) */
class CAiStateFlagSlots {
public:
    virtual void s08();
    virtual void s0c();
    virtual void s10();
    virtual void s14();
    virtual void s18();
    virtual void s1c();
    virtual void s20();
    virtual int CanAttackAi(CSealCtrlAi *ai);   // +0x24
    virtual void s28();
    virtual void s2c();
    virtual void s30();
    virtual void s34();
    virtual void s38();
    virtual void s3c();
    virtual int Virtual40Ai(CSealCtrlAi *ai);   // +0x40
};

/* the stack's bits brought up to date when asked or marked (bit 0): from the bottom state up, bit 1 "may attack" while
 * every state says so, bit 4 while a state with a focus has one (a breach clears it), bit 2 for a held script; the mask
 * of kinds that may be pushed ANDed by each state's, the kinds on the stack listed; a group with cameras allows only the
 * target state; the SEAL (and, squad mode 2, the squad's leader) marked (OVL2 0x5F7470; the name unknown) */
extern "C" void func_005F7470(CAiStateStack *stack, CSealCtrlAi *ai, int x)
{
    if (x || (stack->m_14 & 1)) {
        stack->m_14 = 2;
        stack->m_c = -1;
        stack->m_10 = 0;
        for (CAiStateStack::reverse_iterator i = stack->rbegin(); i != stack->rend(); ++i) {
            CAiState *s = *i;
            AI_STATE kind = s->m_state;

            if (!((CAiStateFlagSlots *)s)->CanAttackAi(ai))
                stack->m_14 &= ~2;
            if (((CAiStateFlagSlots *)s)->Virtual40Ai(ai)) {
                CPnt3D pos;
                CPnt3D *out;
                CAiFocusView *f = (CAiFocusView *)&ai->m_660;

                out = &pos;
                if (FocusPos(f, out))
                    stack->m_14 |= 0x10;
            }
            switch (kind) {
            case AI_STATE_BREACH:
                stack->m_14 &= ~0x10;
                break;
            case AI_STATE_SCRIPT: {
                CAiSScript *script = s && s->m_state == AI_STATE_SCRIPT ? (CAiSScript *)s : 0;

                if (script->Held())
                    stack->m_14 |= 4;
                break;
            }
            }
            stack->m_c &= StatePushMask(s);
            stack->m_10 |= 1 << kind;
        }
        CAiGroup *group = ai->GetGroup();

        if (group && (*(unsigned int *)group & 1))
            stack->m_c = 0x100;
        CEntity *seal = ai->m_entity;

        if (seal) {
            ((CSealSignalBits *)seal)->m_signalled = true;
            if (ai->m_26c == 2 && ((CAiCtrl10C *)ai)->m_10c) {
                CEntity *leader = GetSealByIndex__9CSealUnitFii(((CAiCtrl10C *)ai)->m_10c, 1, 0);

                if (leader)
                    ((CSealSignalBits *)leader)->m_signalled = true;
            }
        }
    }
}

/* a state of a stack told a signal (OVL2 0x5F6FA0; SOCOM 1's CAiState::Signal, moved onto the
 * stack): entering (0) resets it to its own kind, sends it the reset (4) and starts its stack's
 * clock; leaving (1) empties its own stack; then, for the controller's own stack, the SEAL (and,
 * squad mode 2, the squad's leader) marked, the trigger let go and the per-signal goals dropped;
 * the state's OnSignal last */
void CAiState::Signal(CAiStateStack *stack, CAiState *state, SIGNAL sig, CSealCtrlAi *ai)
{
    CEntity *seal = ai ? ai->m_entity : 0;
    bool own = &ai->m_stateStack == stack;

    switch (sig) {
    case SIGNAL_4:
        break;
    case SIGNAL_0:
        state->SetBit0(true);
        state->Reset(state->m_state);
        state->OnSignal(SIGNAL_4, 0);
        state->SetActive(true);
        func_005F6350(state, ai, sig);
        stack->m_timer = 0.0f;
        break;
    case SIGNAL_1:
        func_005F6350(state, ai, sig);
        state->m_stack.PopAll(ai);
        func_005F6190(state, ai);
        break;
    case SIGNAL_2:
        func_005F6350(state, ai, sig);
        ai->SetAiBit2(false);
        break;
    case SIGNAL_3:
        func_005F6350(state, ai, sig);
        if (data_0044D630 == 24 && data_0044D628 == 3)
            ai->SetAiMode((AI_MODE)3);
        break;
    }
    if (own && ai) {
        ((CSealSignalBits *)seal)->m_signalled = true;
        if (ai->m_26c == 2 && ((CAiCtrl10C *)ai)->m_10c) {
            CEntity *leader = GetSealByIndex__9CSealUnitFii(((CAiCtrl10C *)ai)->m_10c, 1, 0);

            if (leader)
                ((CSealSignalBits *)leader)->m_signalled = true;
        }
        ai->SetTrigger(false);
        ai->SetTrigger(false);
        ai->SetDesiredStance((SEAL_STANCE)2, 0.0f, true);
        switch (sig) {
        case SIGNAL_4:
            break;
        case SIGNAL_0:
            ai->SetAiMode((AI_MODE)1);
            func_00570F90(ai, 1.0f);
            ai->SetAiBit2(false);
            ai->ClearGoal(GOAL_NONE);
            break;
        case SIGNAL_1:
            ai->ClearGoal(GOAL_NONE);
            ai->SetAiBit2(false);
            break;
        case SIGNAL_2:
            ai->ClearGoal(GOAL_NONE);
            break;
        case SIGNAL_3:
            ai->ClearGoal(GOAL_NONE);
            break;
        }
    }
    state->OnSignal(sig, ai);
}

/* the current state popped: told it is left (1), noted as the last popped, deleted, and the state
 * under it told it is back on top (3); its kind returned, or 0 (OVL2 0x5F7C40; SOCOM 1's
 * CSealCtrlAi::PopAiState(), whose wrapper 0x5F5CA0 now calls the stack's -- the name ours).
 * The new front is read through the &end_.next_ the erase's begin() computed (s3): GetState()
 * reads its front through the wrapper's begin(), as the erase does (round 6) */
AI_STATE CAiStateStack::Pop(CSealCtrlAi *ai)
{
    AI_STATE kind = AI_STATE_UNKNOWN;

    if (size() != 0) {
        CAiState *s = GetState();
        CAiState *next;

        CAiState::Signal(this, s, CAiState::SIGNAL_1, ai);
        data_0044D630 = s->m_state;
        data_0044D628 = s->m_done;
        next = 0;
        if (size() != 0) {
            delete GetState();
            pop_front();
            next = GetState();
        }
        if (next) {
            kind = next->m_state;
            CAiState::Signal(this, next, CAiState::SIGNAL_3, ai);
        }
        func_005F7470(this, ai, 1);
    }
    return kind;
}

/* a state of a kind deleted, for remove_if (Remove's; the template tail's remove_if and
 * remove_copy_if, 0x5F8AF0 and 0x5F8C40, inline it -- the name ours). The kind read into a local
 * first, then compared on the left (round 31): remove_if's search, which reaches the functor through
 * a reference, then re-reads its byte at every step (`lbu a1,104(sp)`, the virtual destructor's call
 * in the loop kills it) while remove_copy_if's by-value copy is hoisted into s0 -- both instances
 * byte-exact so; read in place, either operand order, through a reference, a `self` pointer or a
 * non-const operator() MW hoists the read in both (0x114 and 0xE8 against 0x110 and 0x100) */
struct CAiStateDeleteKind {
    CAiStateDeleteKind(AI_STATE state) : m_state(state) {}
    bool operator()(CAiState *s) const
    {
        AI_STATE k = m_state;

        if (k == s->m_state) {
            delete s;
            return true;
        }
        return false;
    }

    AI_STATE m_state;
};

/* every state of a kind deleted and taken off; whether there was one (OVL2 0x5F81E0) */
#pragma auto_inline off
bool CAiStateStack::Remove(AI_STATE state)
{
    bool r = false;
    iterator e = end();
    iterator i = std::remove_if(begin(), e, CAiStateDeleteKind(state));

    if (i != e) {
        erase(i, e);
        r = true;
    }
    return r;
}
#pragma auto_inline reset

/* the state of a kind, or 0 (OVL2 0x5F8270; SOCOM 1's CSealCtrlAi::FindAiState). This and the
 * pops below are pinned: the controller's wrappers call them out of line */
#pragma auto_inline off
CAiState *CAiStateStack::Find(AI_STATE state) const
{
    const_iterator i;

    for (i = begin(); i != end(); ++i)
        if ((*i)->m_state == state)
            return *i;
    return 0;
}
#pragma auto_inline reset

/* a new state of a kind pushed: the current one told it is covered (2), the new one entered (0)
 * and, when it has a script response and the covered state is the script, the response run on
 * it (the new state's bit 3 then set) (OVL2 0x5F7EE0; SOCOM 1's CSealCtrlAi::PushAiState). Pinned:
 * fourteen statements make it an auto-inline candidate too big to inline whole, and a function
 * calling such a one is never auto-inlined -- the controller's PushAiState is, into 0x5F51A0 */
#pragma auto_inline off
bool CAiStateStack::Push(CSealCtrlAi *ai, AI_STATE state, unsigned int flags)
{
    CAiState *top = GetState();
    CAiSScript *script = 0;

    if (top) {
        CAiState::Signal(this, top, CAiState::SIGNAL_2, ai);
        script = top && top->m_state == AI_STATE_SCRIPT ? (CAiSScript *)top : 0;
    }
    CAiState *s = PushNew(state);

    CAiState::Signal(this, s, CAiState::SIGNAL_0, ai);
    const char *response = s->GetScriptResponse();

    if (response && script && script->ActivateSequence(response, 7))
        s->SetBit3(true);
    func_005F7470(this, ai, 1);
    return true;
}
#pragma auto_inline reset

/* a new state of a kind in the current one's place: the current one told it is left (1) and
 * deleted, the new one entered (0) (OVL2 0x5F7D80; SOCOM 1's CSealCtrlAi::SetAiState) */
bool CAiStateStack::Set(CSealCtrlAi *ai, AI_STATE state, unsigned int flags)
{
    if (size() == 0) {
        CAiState *s = CAiState::Create(state);

        push_front(s);
        CAiState::Signal(this, s, CAiState::SIGNAL_0, ai);
    } else {
        CAiState::Signal(this, GetState(), CAiState::SIGNAL_1, ai);
        if (size() != 0) {
            delete GetState();
            pop_front();
        }
        CAiState *s = CAiState::Create(state);

        push_front(s);
        CAiState::Signal(this, s, CAiState::SIGNAL_0, ai);
    }
    func_005F7470(this, ai, 1);
    return true;
}

/* popped down to a kind (when the kind is on the stack at all), it too when `inclusive`; whether
 * it was found (OVL2 0x5F7B10; SOCOM 1's CSealCtrlAi::PopToAiState).
 * OPEN, 0x124 against 0x130: Is(), as in Pop(ai, state) below -- the target materialises its
 * whole `&&` (both operands into one register, the false arm a `move v0,zero`, then one test);
 * here MW threads the second operand's test into the branch */
#pragma auto_inline off
bool CAiStateStack::PopTo(CSealCtrlAi *ai, AI_STATE state, bool inclusive)
{
    bool found;

    if (m_10 & (1 << state))
        while (size() != 0 && !Is(state))
            Pop(ai);
    found = Is(state);
    if (found && inclusive)
        Pop(ai);
    return found;
}
#pragma auto_inline reset

/* popped down to a state, it too when `inclusive`; whether it was found (OVL2 0x5F7A40) */
#pragma auto_inline off
bool CAiStateStack::PopTo(CSealCtrlAi *ai, CAiState *state, bool inclusive)
{
    bool found = false;

    while (!found && size() != 0) {
        if (GetState() == state) {
            found = true;
            if (inclusive)
                Pop(ai);
        } else
            Pop(ai);
    }
    return found;
}
#pragma auto_inline reset

/* the current state popped if it is of a kind; the kind on top after (OVL2 0x5F7990; SOCOM 1's
 * CSealCtrlAi::PopAiState(AI_STATE)).
 * OPEN, 0xA4 against 0xAC: the target materialises Is()'s `!(size() == 0) && GetState()->m_state
 * == state` whole -- xor/sltiu/xori with no andi, the second operand's sltiu, a `b` to the join
 * past the false arm's `move v0,zero`, then one beqz -- where MW narrows the first operand (andi)
 * and branches on the second. Tried: Is() returning through a bool local, `size() != 0`, the
 * ternary, an inline Empty(), Is() as a plain auto-inlined member, the caller testing `!= false`
 * / `== true` / a bool local / a ternary / a result local (all 0xA4 or worse). Round 6: Is() over
 * size_ itself, `bool r = A; if (r) r = B; return r;` (with and without Empty()), the if-return,
 * Is() returning int (two andi then), the comparison's operands swapped or cast, its second
 * operand an inline of its own (StateIs/StateOf) or a comma expression, the first an inline
 * NotEmpty() -- 0xA0 to 0xA8, never the target's unnarrowed first operand and explicit false arm.
 * One clue: with the second operand an out-of-line call, MW drops the first operand's andi */
#pragma auto_inline off
AI_STATE CAiStateStack::Pop(CSealCtrlAi *ai, AI_STATE state)
{
    if (Is(state))
        return Pop(ai);
    return GetStateEnum();
}
#pragma auto_inline reset

/* the current state ticked: its own stack first, then (once that is empty and done) its Tick, or
 * else its sub-step (slot 0x14, handed the sub-stack's state); a state done is popped, its result
 * kept; else the stack's clock and the state's step clock run (OVL2 0x5F8030) */
int CAiStateStack::Tick(float dT, CSealCtrlAi *ai)
{
    int result = AI_DONE;

    CAiState::m_dT = dT;
    if (size() != 0) {
        CAiState *s = GetState();
        int r;

        if (s->m_active) {
            if (s->m_stack.Tick(dT, ai) && s->m_stack.size() == 0) {
                r = s->Tick(ai);
                if (r == AI_RUNNING) {
                    func_005F7470(this, ai, (s->m_flags & 0x80000000) != 0);
                    s->SetFlags(0, 0x80000000);
                }
            } else
                r = s->Virtual14(ai, s->m_stack.GetState());
        } else
            r = AI_DONE;
        if (r != AI_RUNNING) {
            Pop(ai);
            m_result = r;
            result = AI_FINISHED;
        } else {
            m_timer += dT;
            s->m_phaseTime += dT;
            s->SetBit0(false);
            result = AI_RUNNING;
        }
    }
    return result;
}

/* every state popped, then the flags brought up to date (OVL2 0x5F7920: its callers here,
 * ClearAiState and Signal, call it out of line) */
#pragma auto_inline off
void CAiStateStack::PopAll(CSealCtrlAi *ai)
{
    if (size() != 0)
        do
            Pop(ai);
        while (size() != 0);
    func_005F7470(this, ai, 1);
}
#pragma auto_inline reset

/* the planner a hiding state hands its path (new in SOCOM II, the name its RTTI's: 0x14 bytes,
 * table 0x6696B0 -- reset, is_goal and dist its own, is_traversable and fcost the base's). Made by
 * 0x5F0990 and 0x5F0B30 with the flag clear, a distance of 5 and a threshold of 0.5; its goal is a
 * cell off the start, far enough from it, from which the line to the threat is seen through at
 * most (or, without the flag, at least) the threshold (the names ours) */
class CAiHidePlanner : public CAiPlanner {
public:
    /* (inline: 0x5F0990 and 0x5F0B30 expand it after `new`, the base's table stored first) */
    CAiHidePlanner(bool hide, int dist, float thresh) : m_hide(hide), m_dist(dist), m_thresh(thresh) {}
    virtual void reset(CAiMap &map, const CAiMapLoc &start, const CAiMapLoc &goal);
    virtual bool is_goal(CAiMap &map, const CAiMapLoc &a, const CAiMapLoc &b);
    virtual int dist(CAiMap &map, const CAiMapLoc &a, const CAiMapLoc &b);

    bool m_hide;   // 0x4
    int m_dist;   // 0x8: the least distance from the start, in cells (0: any)
    float m_thresh;   // 0xc
    CAiMapLoc m_start;   // 0x10: the search's start, kept by reset
};

/* (OVL2 0x5F8560) */
void CAiHidePlanner::reset(CAiMap &, const CAiMapLoc &start, const CAiMapLoc &)
{
    m_start = start;
}

/* the height above both ends the hide planner's line is seen from, 8.0 (OVL2 data among
 * ai_events.cpp's and ai_astar.cpp's statics; the name ours) */
extern float s_hideHeight;

/* two cells' x and y distances summed (CAiPlanner::dist's shape, unscaled; inline, the name ours) */
inline int HideDist(const CAiMapLoc &a, const CAiMapLoc &b)
{
    int dx = a.x > b.x ? a.x - b.x : b.x - a.x;
    int dy = a.y > b.y ? a.y - b.y : b.y - a.y;

    return dx + dy;
}

/* (OVL2 0x5F8570: the line seen from eight units up at both ends -- the height a variable, read from
 * data; the distance an inline, whose x fields MW shares with operator!='s, as the target does).
 * The hide/see compare a ternary: as an if/else (open since written, 0x130) MW filled the m_dist
 * test's delay slot from the fall-through and left the visibility label unpadded -- the UI's round-31
 * if-versus-ternary rule, here in the function's own body (round 72). */
bool CAiHidePlanner::is_goal(CAiMap &map, const CAiMapLoc &a, const CAiMapLoc &b)
{
    bool r = false;

    if (m_start != a && (!m_dist || HideDist(a, m_start) >= m_dist)) {
        float vis = map.ComputeLineVisibility(a, b, 0, s_hideHeight, s_hideHeight);

        r = m_hide ? vis <= m_thresh : vis >= m_thresh;
    }
    return r;
}

/* (OVL2 0x5F86B0: every step alike) */
int CAiHidePlanner::dist(CAiMap &, const CAiMapLoc &, const CAiMapLoc &)
{
    return 0;
}

extern "C" {
/* a cell to hide in from a point or to see it from, found for the controller (OVL2 0x5F0CD0, this
 * unit's, unwritten: both pushes below call it first) */
bool func_005F0CD0(CSealCtrlAi *ai, const CPnt3D *pos, CAiMapLoc *loc);
}

/* a plan pushed on the current state's own stack to a cell from which the point is seen (the hide
 * planner's flag clear) or hidden from (set), five cells off at least, at a threshold of a half:
 * the cell, the planner (its kind 2), the flags `mode`, the state it covers and the kind and step to
 * come back to; whether it was pushed (OVL2 0x5F0B30 and 0x5F0990, alike but for the flag:
 * 0x5F0830's shape with the cell searched for first -- CAiSTarget::Tick picks one, CAiSHostage,
 * CAiSReload and CAiSShotAt call the first, CAiSHold the second; the names unknown) */
extern "C" bool func_005F0B30(CSealCtrlAi *ai, const CPnt3D *pos, unsigned int mode, int kind, int step)
{
    bool r = false;
    CAiMapLoc loc;

    if (func_005F0CD0(ai, pos, &loc)) {
        CAiState *s = ai->GetAiState();

        if (s->m_stack.Push(ai, AI_STATE_PLAN, 0)) {
            CAiState *top = s->m_stack.GetState();
            CAiSPlan *plan = top ? (top && top->m_state == AI_STATE_PLAN ? (CAiSPlan *)top : 0) : 0;

            func_005F1760(plan, &loc);
            plan->m_10c = (char *)new CAiHidePlanner(true, 5, 0.5f);
            plan->m_110 = 2;
            plan->SetFlags(mode, 0);
            r = true;
            plan->m_114 = (unsigned int)s;
            plan->m_124 = kind;
            plan->m_128 = step;
        }
    }
    return r;
}

extern "C" bool func_005F0990(CSealCtrlAi *ai, const CPnt3D *pos, unsigned int mode, int kind, int step)
{
    bool r = false;
    CAiMapLoc loc;

    if (func_005F0CD0(ai, pos, &loc)) {
        CAiState *s = ai->GetAiState();

        if (s->m_stack.Push(ai, AI_STATE_PLAN, 0)) {
            CAiState *top = s->m_stack.GetState();
            CAiSPlan *plan = top ? (top && top->m_state == AI_STATE_PLAN ? (CAiSPlan *)top : 0) : 0;

            func_005F1760(plan, &loc);
            plan->m_10c = (char *)new CAiHidePlanner(false, 5, 0.5f);
            plan->m_110 = 2;
            plan->SetFlags(mode, 0);
            r = true;
            plan->m_114 = (unsigned int)s;
            plan->m_124 = kind;
            plan->m_128 = step;
        }
    }
    return r;
}

/* the stack's constructor (OVL2 0x5F8520): out of line, as its destructor is */
CAiStateStack::CAiStateStack() : m_c(-1), m_10(0), m_14(0)
{
}

/* the controller's settings a covered state keeps: SEAL_AI_PARAMS's debug bit and bit 2 (0x264),
 * its stance (0x278), CSealCtrl's float at 0x1C4 (the one 0x570F90 takes) and the mode at 0x3F4,
 * read as a signed byte here -- zseal.h names neither the float nor a signed mode (the names
 * ours) */
struct CAiCtrlSaved {
    unsigned char _0[0x1c4];
    float m_1c4;   // 0x1c4
    unsigned char _1c8[0x3f4 - 0x1c8];
    signed char m_mode;   // 0x3f4
};

/* the debug bit cleared through a setter taking a bool (the name ours) */
inline void SetDebug(CSealCtrlAi *ai, bool b)
{
    ai->m_debug = b;
}

/* a state's hold on the controller's settings: its sub-state's entry clears bit 14, its leaving
 * the debug bit; covered (2) it keeps the stance, the mode, bit 2, the float and the SEAL's cell,
 * back on top (3) it gives the first four back (OVL2 0x5F6350; the name ours) */
extern "C" void func_005F6350(CAiState *s, CSealCtrlAi *ai, CAiState::SIGNAL sig)
{
    switch (sig) {
    case CAiState::SIGNAL_0:
        s->SetBit14(false);
        break;
    case CAiState::SIGNAL_1:
        SetDebug(ai, false);
        break;
    case CAiState::SIGNAL_2:
        s->SetBit4(ai->m_aibit2);
        s->SetNibble5(ai->m_stance);
        s->SetNibble9(((CAiCtrlSaved *)ai)->m_mode);
        s->m_4c = ((CAiCtrlSaved *)ai)->m_1c4;
        s->m_loc = CAiMapLoc(ai->m_entity->m_mapLocValid);
        break;
    case CAiState::SIGNAL_3:
        ai->SetStanceDelayed((SEAL_STANCE)s->m_nibble5, -1.0f);
        ai->SetAiBit2(s->m_bit4);
        ai->SetAiMode((AI_MODE)s->m_nibble9);
        func_00570F90(ai, s->m_4c);
        break;
    }
}

extern "C" {
/* the AI's weapon kind, and a kind asked for -- whether it took (OVL2 0x560540, 0x5604D0: the
 * SEAL lane's seal_ai.cpp, its AI_WEAPON_KIND a byte) */
unsigned char func_00560540(CSealCtrlAi *ai);
bool func_005604D0(CSealCtrlAi *ai, int kind);
}

/* a weapon kind asked of the AI: whether it took; the kind it had kept in the word at 0x54 when
 * `keep` (0x5F6190 gives it back) (OVL2 0x5F6100; a CAiState member, unnamed) */
extern "C" bool func_005F6100(CAiState *s, CSealCtrlAi *ai, int kind, bool keep)
{
    bool r = false;
    int old = func_00560540(ai);

    if (func_005604D0(ai, kind)) {
        if (keep)
            s->m_54 = old;
        r = true;
    }
    return r;
}

/* (OVL2 0x5605A0, the SEAL lane's row: the controller told a byte) */
extern "C" void func_005605A0(CSealCtrlAi *ai, unsigned char x);

/* CAiState's helpers the response states' Ticks call out of line (each a member -- the state in
 * a0 -- unnamed, so spelled with the placeholder and the state first) */

/* the word at 0x54, when set, handed to the controller and cleared (OVL2 0x5F6190) */
extern "C" void func_005F6190(CAiState *s, CSealCtrlAi *ai)
{
    if (s->m_54 != -1) {
        func_005605A0(ai, s->m_54);
        s->m_54 = -1;
    }
}

/* a delayed step running: true while the delay (the flags' bit 30) lasts, then the step taken
 * (OVL2 0x5F61E0) */
extern "C" bool func_005F61E0(CAiState *s)
{
    bool r = false;

    if (s->m_flags & 0x40000000) {
        r = true;
        s->m_delay -= CAiState::m_dT;
        if (s->m_delay <= 0.0f) {
            s->m_delay = 0.0f;
            s->UpdatePhase(s->m_nextPhase);
            s->SetFlags(0, 0x40000000);
            r = false;
        }
    }
    return r;
}

/* every `period` seconds of the controller's state clock, to a tenth (OVL2 0x5F6270; byte-exact in
 * the kit lane's round 34, open since round 13: the clock read through a same-type cast -- the HUD's
 * round-33 rule -- so MW builds 10.0 and multiplies the period first, as the target does) */
extern "C" bool func_005F6270(CAiState *s, float period, CSealCtrlAi *ai)
{
    return (int)(10.0f * (float)ai->m_stateStack.m_timer) % (int)(10.0f * period) == 0;
}

/* a step taken now, or after a delay (the flags' bit 30 then set); the step the state is on
 * (OVL2 0x5F62C0) */
extern "C" unsigned int func_005F62C0(CAiState *s, unsigned int phase, float delay)
{
    if (delay == 0.0f) {
        s->UpdatePhase(phase);
        s->SetFlags(0, 0x40000000);
    } else {
        s->SetFlags(0x40000000, 0);
        s->m_delay = delay;
        s->m_nextPhase = phase;
    }
    return s->m_phase;
}

/* the controller's state of a kind (SOCOM 1's FindAiState; OVL2 0x5F5FC0, a jump to the stack's
 * walk): Virtual0C calls it out of line */
#pragma auto_inline off
CAiState *CSealCtrlAi::FindAiState(AI_STATE state) const
{
    return m_stateStack.Find(state);
}
#pragma auto_inline reset

/* the script state (kind 1) running sequence `id` and told event 2: this state is done (OVL2
 * 0x5F6050) */
void CAiState::Virtual0C(CZAnim *anim, AI_SIGNAL sig, CSealCtrlAi *ai)
{
    CAiState *s = ai->FindAiState((AI_STATE)1);
    CAiSScript *script = s && s->m_state == 1 ? (CAiSScript *)s : 0;

    if (script && script->m_58 == (unsigned int)anim && sig == 2)
        m_active = false;
}

/* the AI's states dropped: every one popped when asked, or when the SEAL is dead, a Navy SEAL or
 * on the team bit 18 -- whether they were; the flags brought up to date either way (OVL2 0x5F5F20;
 * SOCOM 1's ClearAiState(bool), its loop the stack's PopAll now) */
bool CSealCtrlAi::ClearAiState(bool all)
{
    bool cleared = false;

    if (all || !m_entity->m_isAlive || IsNavySeal() || (m_entity->m_team & 0x40000)) {
        m_stateStack.PopAll(this);
        cleared = true;
    }
    func_005F7470(&m_stateStack, this, 1);
    return cleared;
}

/* the squared distance between two points across the ground, x and z (OVL2 0x5F5AD0: an inline's
 * out-of-line copy after StateTick, 0x5F55E0, its one caller -- unnamed). The height's zero kept
 * as a local (its square folded, the `+ 0` not), the differences declared z first */
#pragma auto_inline off
extern "C" float func_005F5AD0(const CPnt3D *a, const CPnt3D *b)
{
    float dz = a->z - b->z;
    float dy = 0.0f;
    float dx = a->x - b->x;

    return dx * dx + dy * dy + dz * dz;
}
#pragma auto_inline reset

/* an entity's place (OVL2 0x5F5B10: a draft, an accessor's out-of-line copy; the name ours) and the place a kept target
 * names: its entity's for kind 1, its own point for kind 2 (OVL2 0x5F5B20: a draft, the same) */
extern "C" bool ConfirmCommand__9CSealUnitF10FT_COMMANDUci(CSealUnit *unit, int cmd, unsigned char item, int flags);
extern "C" const CPnt3D *func_005F5B10(CZSealBody *seal);
extern "C" void *func_005F5B20(void *target, CPnt3D *out);

/* the anim's byte at 2 as a bool (inline: StateTick's; the name ours) */
inline bool AnimBusy(CZAnim *a)
{
    return ((unsigned char *)a)[2] != 0;
}

/* the follow state's master in a player-led mode (inline: the master read through a reference, kept on the stack; the
 * name ours) */
inline bool ModeMaster(CSealCtrlAi *ai, CZSealBody *&master)
{
    bool r = ai->m_660 == 1;

    if (r)
        master = ai->m_664;
    return r;
}

/* the state stack's frame (OVL2 0x5F55E0; SOCOM 1's StateTick, its pieces moved into the stack): the stack ticked; a
 * finished state's response sequence seen off in the script under it; past its range from the kept target, the stack
 * popped to the first state that answers, and with none left in a player-led mode the SEAL told to follow, the squad
 * told; a kind 3 state taken away when the flag asks.
 * OPEN, 0x4C8 against 0x4E8 (the kit's round 61, its saved registers all the target's: the declarations' order decides the
 * spill pick's tie): the target keeps the master in a frame slot (172: `sw zero`, then direct loads and stores -- every
 * address-taking tried folds, or leaves the address in a register); the Is(3) at the end is the stack's known Is() shape,
 * the materialised first operand and explicit false arm, which fold in this unit (round 58's AiStateIs) */
void CSealCtrlAi::StateTick(float dT)
{
    /* (declared in this order: the tie on the spill pick falls to the highest number, the SEAL, which takes s6) */
    CAiStateStack *stack = &m_stateStack;
    CAiState *s;
    bool running;
    const char *seq;
    CZSealBody *seal = (CZSealBody *)m_entity;

    if (stack->size() == 0)
        return;
    CAiState *top = stack->GetState();

    if (stack->Tick(dT, this) == 0) {
        if ((top->m_bit3 ? true : false) && stack->size() >= 2) {
            s = stack->Find(AI_STATE_SCRIPT);
            CAiSScript *script = s && s->m_state == AI_STATE_SCRIPT ? (CAiSScript *)s : 0;

            if (script) {
                running = false;
                if (script->m_58 && !AnimBusy((CZAnim *)script->m_58)) {
                    running = ResponseRunning(script);
                    if (!running) {
                        if (script->m_seqName[0] && !((CZAnim *)script->m_58)->SequenceRunning(script->m_seqName)) {
                            seq = sealai::get_aiseq(script->m_seq);
                            script->m_held_once = false;
                            ((CZAnim *)script->m_58)->StopSequence(script->m_seqName);
                            ((CZAnim *)script->m_58)->ResumeSequence(seq);
                            script->SetHold(false);
                            script->m_seqName[0] = 0;
                        }
                        func_00270BE0((CZAnim *)script->m_58, 0);
                    }
                }
                if (!running)
                    top->m_bit3 = false;
            }
        }
        CAiStateWord &word = stack->m_14;

        if (word & 0x10) {
            CPnt3D p;
            bool near = true;

            if (!(m_674 <= 0.0f) && func_005F5B20(&m_660, &p))
                near = func_005F5AD0(&p, func_005F5B10(seal)) <= m_678;
            if (!near) {
                std::list<CAiState *>::const_iterator i = ((const CAiStateStack *)stack)->begin();

                for (; i != ((const CAiStateStack *)stack)->end(); ++i)
                    if (((CAiStateFlagSlots *)*i)->Virtual40Ai(this))
                        break;
                if (i != ((const CAiStateStack *)stack)->end()) {
                    PopToAiState(*i, true);
                    /* (its address taken in an unused pointer keeps its frame slot, at no code) */
                    CZSealBody *master = 0;
                    CZSealBody **pm = &master;

                    if (stack->size() == 0 && ModeMaster(this, master)) {
                        CZSealBody *m = master;

                        if (!m || m->m_type != 2)
                            m = 0;
                        SetFollowMaster(m);
                        bool pushed = CanPushAiState(AI_STATE_FOLLOW, 0);

                        if (pushed)
                            pushed = m_stateStack.Push(this, AI_STATE_FOLLOW, 0);
                        if (pushed) {
                            CAiState *t = GetAiState();
                            CAiSFollow *f = t && t->m_state == AI_STATE_FOLLOW ? (CAiSFollow *)t : 0;

                            f->SetActive(true);
                            f->SetBit2(true);
                            if (m_unit)
                                ConfirmCommand__9CSealUnitF10FT_COMMANDUci(m_unit, 7, 0, 0);
                        }
                    }
                } else
                    word &= ~0x10;
            }
        }
    }
    if (m_stateStack.m_10 & 8) {
        if (!AiStateIs((AI_STATE)3) && stack->Remove((AI_STATE)3))
            func_005F7470(&m_stateStack, this, 1);
    }
}

/* the animation a SEAL taking a stun response must not be blending in (OVL1 data 0x3DEE40, read
 * as a variable with lh) */
extern "C" AnimType data_003DEE40;

/* neither of the controller's bits 8 and 9 set (an inline bool returning a negation: the target
 * materialises it, sltu/xori, before its branch; the name ours) */
inline bool AiFree(CSealCtrlAi *ai)
{
    return !(ai->m_aibit9 || ai->m_aibit8);
}

/* whether a state of a kind may be pushed, the flags bending the rules (SOCOM 1's
 * CanPushAiState(AI_STATE), the flags new: 1 lets the current kind be pushed again, 2 any kind the
 * stack's mask at 0xC lacks, 4 past the controller's bits 8 and 9, 8 past the stack's bit 2 that
 * keeps only the urgent kinds); the current state has the last word (OVL2 0x5F5CD0) */
bool CSealCtrlAi::CanPushAiState(AI_STATE state, unsigned int flags)
{
    CEntity *seal = m_entity;

    if (state == AI_STATE_SUSPENDED)
        return true;
    if (!AiFree(this) && !(flags & 4))
        return false;
    if (!seal->m_isAlive)
        return false;
    CAiState *s = GetAiState();
    AI_STATE cur = s->m_state;
    unsigned int mask = m_stateStack.m_c;
    bool ok = (flags & 2) != 0 || (mask & (1 << state)) != 0;

    if (ok && !(flags & 1))
        ok = cur != state;
    if (ok && state != AI_STATE_SUSPENDED) {
        if (!(flags & 8) && (m_stateStack.m_14 & 4)) {
            switch (state) {
            case AI_STATE_SCRIPT:
            case AI_STATE_HOLD:
            case AI_STATE_GOTO:
            case AI_STATE_ACTION:
            case AI_STATE_ANIMATE:
            case AI_STATE_DEPLOY:
            case AI_STATE_PATHRECOVER:
            case AI_STATE_STUNRESPONSE:
            case AI_STATE_SURRENDER:
            case AI_STATE_RELOAD:
            case AI_STATE_NOAMMO:
                ok = true;
                break;
            case AI_STATE_HOSTAGE:
                ok = cur == AI_STATE_SURRENDER;
                break;
            default:
                ok = false;
                break;
            }
        }
        if (state == AI_STATE_CQBATTACK)
            ok = true;
        if (ok && state == AI_STATE_STUNRESPONSE)
            ok = !((CZSealBody *)seal)->IsCurrentBlend(data_003DEE40);
        if (ok)
            ok = s->CanPushAiState(state, this);
    }
    return ok;
}

/* the stack's operations as the controller offers them (SOCOM 1's CSealCtrlAi members, their
 * bodies the stack's now; OVL2 0x5F5CB0 back to 0x5F5BA0) */
AI_STATE CSealCtrlAi::PopAiState(AI_STATE state)
{
    return m_stateStack.Pop(this, state);
}

AI_STATE CSealCtrlAi::PopAiState()
{
    return m_stateStack.Pop(this);
}

bool CSealCtrlAi::PopToAiState(AI_STATE state, bool inclusive)
{
    return m_stateStack.PopTo(this, state, inclusive);
}

#pragma auto_inline off
bool CSealCtrlAi::PopToAiState(CAiState *state, bool inclusive)
{
    return m_stateStack.PopTo(this, state, inclusive);
}
#pragma auto_inline reset

/* pushed, or set in the current one's place, when CanPushAiState allows it (the flags its) */
bool CSealCtrlAi::PushAiState(AI_STATE state, unsigned int flags)
{
    bool r = CanPushAiState(state, flags);

    if (r)
        r = m_stateStack.Push(this, state, flags);
    return r;
}

bool CSealCtrlAi::SetAiState(AI_STATE state, unsigned int flags)
{
    bool r = CanPushAiState(state, flags);

    if (r)
        r = m_stateStack.Set(this, state, flags);
    return r;
}

/* the state on top of the controller's stack, or the unknown state when it is empty (OVL2
 * 0x5F6010, 0x5F5FD0 -- the same code twice; SOCOM 1's tested the top for 0 instead) */
CAiState *CSealCtrlAi::GetAiState()
{
    if (m_stateStack.size() == 0)
        return &_unknown;
    return m_stateStack.GetState();
}

const CAiState *CSealCtrlAi::GetAiState() const
{
    if (m_stateStack.size() == 0)
        return &_unknown;
    return m_stateStack.GetState();
}

/* a stack's front state told a signal, when there is one (OVL2 0x5F7420) */
void CAiState::Signal(CAiStateStack *stack, SIGNAL sig, CSealCtrlAi *ai)
{
    if (stack->size() != 0)
        Signal(stack, stack->GetState(), sig, ai);
}

/* the stack's destructor: its states deleted, then the list's own (OVL2 0x5F8490) */
CAiStateStack::~CAiStateStack()
{
    Clear();
}

/* the current state: the front, or 0 when the stack is empty (OVL2 0x5F8360, 0x5F8330 -- the
 * same code twice, the non-const and the const one). The non-const one reads it through begin()
 * (the sentinel by reference): Pop's GetState after the erase then reuses the &end_.next_ the
 * erase's begin() kept in a saved register */
CAiState *CAiStateStack::GetState()
{
    if (size() == 0)
        return 0;
    return *begin();
}

CAiState *CAiStateStack::GetState() const
{
    if (size() == 0)
        return 0;
    return ((CAiStateStack *)this)->front();
}

/* every state deleted, the stack emptied (OVL2 0x5F8390; ~CSealCtrlAi calls it too).
 * OPEN, 13 instructions, stack slots only: the target walks two const_iterators assigned from
 * the const begin()/end() (the four copies each -- store, bitwise, word, bitwise -- and the walk
 * re-read from memory at every step, as here), but keeps the walking pair BELOW the copies'
 * temporaries (68/64 against 72/76), where every named pair lands above them; MSL's for_each over
 * the const iterators gives the target's order of operations but its parameters' slots too high
 * (14), over the plain iterators a register walk (0xA0). The pair is likely an inlined function's
 * parameters, as for_each's are, with a functor shape not found yet. */
void CAiStateStack::Clear()
{
    const CAiStateStack &s = *this;
    const_iterator i, e;

    e = s.end();
    for (i = s.begin(); i != e; ++i)
        delete *i;
    erase(begin(), end());
}

/* the setters the target passes a bool through to SEAL_AI_PARAMS's flag word (0x264, bits 13 and
 * 17; the names ours) */
inline void SetBit264_13(CSealCtrlAi *ai, bool on)
{
    ai->m_aibit13 = on;
}

inline void SetBit264_17(CSealCtrlAi *ai, bool on)
{
    ai->m_aibit17 = on;
}

/* a breach's events: on entry no goal, the order byte cleared, crouched, the bit off; on leaving
 * for a sub-state its valve to 3 (a live SEAL's), weapon down, the bits set; a reset clears it
 * (OVL2 0x5F3620; SOCOM 1's) */
void CAiSBreach::OnSignal(SIGNAL sig, CSealCtrlAi *ai)
{
    CEntity *seal = ai ? ai->m_entity : 0;

    switch (sig) {
    case SIGNAL_0:
        ai->ClearGoal(GOAL_NONE);
        ai->ClearLook(AIL_ALL);
        ai->SetStance((SEAL_STANCE)1);
        SetBit264_13(ai, false);
        break;
    case SIGNAL_1:
        if (!seal->m_isAlive && m_valve)
            m_valve->SetValue(3);
        SetWeaponState__10CZSealBodyF12WEAPON_STATE(seal, 2);
        ai->m_aibit16 = m_bit13 != 0;
        SetBit264_17(ai, false);
        ai->m_aibit13 = true;
        break;
    case SIGNAL_2:
        break;
    case SIGNAL_3:
        break;
    case SIGNAL_4:
        m_64 = 0;
        m_70 = 0;
        m_74 = 0;
        m_6c = 0;
        m_90 = 0;
        m_78 = 0;
        m_valve = 0;
        break;
    }
}

/* which side of the breach's door the SEAL stands on: the door's side (the reverse one with the
 * flag bit 0) against the flat direction from the SEAL to the breach point -- -1 when the
 * direction's left perpendicular lies within 35 degrees of the side, 1 when its right does within
 * 20, 2 otherwise or for a door with no side (OVL2 0x5F3270: CAiSBreach::Tick calls it twice, a
 * member -- the state in a0; the name unknown). The cosines are function-local statics (lwc1 from
 * 8-byte-spaced data: as literals MW builds them with lui/ori), both dots taken before the tests
 * (the target ends the second in the first test's delay slot).
 * OPEN, 16 instructions at the right size, the dots' float scheduling only: the target computes
 * the y-terms' `0 * v.y` once and adds it into both dots (the first through adda/madd, the second
 * with two add.s, its z-term first); here the second dot's y-term is fused into a madd of its own.
 * Tried: the two points field by field (0x134), the dots written out (0x114), one point reused for
 * both (its x and z reassigned, or negated: 0x134), either dot first, Dot(v, p); round 28: the
 * right point's y taken from the left's (16), the y-term in a shared local with the dots written
 * out (22; the second's z-term first, 17). The 0 is the points' literal, not d.y: d is reloaded
 * after Normalize and its y never is.
 * BYTE-EXACT UNDER 3.0b50 as written (the MAIN runtime lane's round 7, confirmed in round 31 with an
 * `sc: compiler=mwcps2-3.0b50-030527` line): b50 shares the y-product between the dots, b52 fuses
 * the second dot's x and y terms into mula/madd and recomputes it. Round 31 under b52: 72 explicit
 * shared-y-term spellings (either point first, either dot first, the y-term from either point, three
 * groupings of each dot) give 17 or 22 -- and 17 under b50 too, so the target is b50's Dot form,
 * not a written-out one; const points, temporaries bound to references or passed straight to Dot,
 * one declaration for both dots and right.y taken from left.y all stay at 16; d.y as the zero 0x134 */
extern "C" int func_005F3270(CAiSBreach *b, CSealCtrlAi *ai)
{
    int r = 2;
    CAiMap::DIR dir = (CAiMap::DIR)b->m_78->m_direction;

    if (dir != CAiMap::DIR_COUNT) {
        if (b->m_flags & 1)
            dir = (CAiMap::DIR)((dir + 4) % 8);
        CPnt3D v;

        CAiMap::GetDirVec(dir, v);
        CPnt3D d = b->m_58 - ai->m_entity->m_posW;

        d.y = 0.0f;
        d.Normalize();
        CPnt3D left(-d.z, 0.0f, d.x);
        CPnt3D right(d.z, 0.0f, -d.x);
        float l = Dot(left, v);
        float rt = Dot(right, v);
        static float cosLeft = 0.8191f;
        static float cosRight = 0.9395f;

        if (l > cosLeft)
            r = -1;
        else if (rt > cosRight)
            r = 1;
    }
    return r;
}

/* an action's extra cell i, or the invalid cell past the six (the name ours: an unsigned test) */
static inline const CAiMapLoc &BreachLocEx(const CAiMap::action *a, unsigned int i)
{
    return i < 6 ? a->m_locEx[i] : CAiMapLoc::invalid;
}

/* the cell at the middle of the door's three cells on the breach's side -- 0-2 with the flag bit
 * 0, 3-5 without -- its coordinates their averages, its map the last one's (OVL2 0x5F3100:
 * CAiSBreach::Tick calls it, a member -- the state in a0 -- and takes the cell's centre; the name
 * unknown) */
extern "C" void func_005F3100(CAiSBreach *b, CAiMapLoc *out)
{
    float x = 0.0f;
    float y = 0.0f;
    unsigned int mapid = 0;
    int start = (b->m_flags & 1) ? 0 : 3;

    for (int i = 0; i < 3; i++) {
        const CAiMapLoc &loc = BreachLocEx(b->m_78, i + start);

        x += loc.x;
        y += loc.y;
        mapid = loc.m_mapid;
    }
    out->x = (int)(x / 3.0f);
    out->y = (int)(y / 3.0f);
    out->m_mapid = mapid;
}

/* what a breach lets be pushed: no avoiding in its steps 4-9, no grenade in steps 6 and 7 -- a
 * grenade allowed opens its valve (3) -- and no targeting (OVL2 0x5F3520; SOCOM 1's):
 * `ok` passed through two inline selections taking the step as an int (andi at the call, `s > 3` branching on `slti at`
 * where `s >= 4` computes into v0): the avoid arm's `if (c) ok = false; return ok;` narrows at its join; the grenade arm's
 * `r = false; if (s != 6 && s != 7) r = ok;` keeps two beqs where `s == 6 || s == 7` folds into a range test, and
 * MW materialises every `c ? false : ok` (sltu / xori). Propagation off in the body keeps `ok`'s 1 in s0 (round 56) */
inline bool BreachAvoidOk(bool ok, int s)
{
    if (s > 3 && s < 10)
        ok = false;
    return ok;
}

inline bool BreachGrenadeOk(bool ok, int s)
{
    bool r = false;

    if (s != 6 && s != 7)
        r = ok;
    return r;
}

bool CAiSBreach::CanPushAiState(AI_STATE state, CSealCtrlAi *ai)
{
#pragma opt_propagation off
    bool open = false;
    STEP step = (STEP)m_phase;
    bool ok = true;

    switch (state) {
    case AI_STATE_AVOID:
        ok = BreachAvoidOk(ok, step);
        break;
    case AI_STATE_GRENADE:
        ok = open = BreachGrenadeOk(ok, step);
        break;
    case AI_STATE_TARGET:
        ok = false;
        break;
    }
    if (open && m_valve)
        m_valve->SetValue(3);
    return ok;
#pragma opt_propagation reset
}

/* +0x34: a breach takes up only a sighting or a shot (OVL2 0x5F34D0) */
bool CAiSBreach::Virtual34(CSealCtrlAi *ai, CAiEvent *e, CZSealBody *seal, bool enemy)
{
    bool r = false;

    switch ((AI_EVENT_KIND)((CAiEventKind *)e)->m_type) {
    case AIEK_WEPFIRE:
    case AIEK_ENTITY:
        r = true;
        break;
    }
    return r;
}

/* the breach's limits (OVL2 data: the first SEAL's wait for the team, 20, and the charge's time per
 * SEAL, 3) */
extern float data_00650BA8;
extern float data_00650BB0;

extern "C" {
/* (OVL2 0x569C50, seal_ai.cpp's: an enemy within a range of an entity) */
CEntity *func_00569C50(CEntity *e, float *range, bool flagged);
bool func_0060A850(void *target);
void func_005668F0(CSealCtrlAi *ai, CZSealBody *seal);
bool func_00564D20(CSealCtrlAi *ai, int item, const CPnt3D *target, const CPnt3D *from, unsigned int flags);
/* (the AI maps' cell name, OVL2 0x52A770, by its mangled name: zseal.h's CAiMaps does not declare it) */
const char *GetCellNameString__7CAiMapsCFRC9CAiMapLoc(const CAiMaps *maps, const CAiMapLoc &loc);
}

/* the AI's target held, and its bit 9 at 0x6A8 set (an inline bool: the target materialises the `&&`) */
inline bool BreachTargetHeld(CSealCtrlAi *ai)
{
    return func_0060A850(ai->m_67c) && (ai->m_6a8 & 0x200);
}

/* a timer counted down by a frame, its period drawn again once it runs out; whether it did (inline:
 * the frame's time read first) */
inline bool BreachTimerTick(CAiTimer &t, float dT)
{
    bool fire = false;

    if (t.m_timer != CAiTimer::infinite) {
        t.m_timer -= dT;
        fire = t.m_timer != CAiTimer::infinite && t.m_timer <= 0.0f;
        if (fire)
            t.m_timer = t.Period();
    }
    return fire;
}

inline void SetBit264_16(CSealCtrlAi *ai, bool on)
{
    ai->m_aibit16 = on;
}

/* a breach's frame (OVL2 0x5F2120; SOCOM 1's grown): the team checked -- no valve or door ends it,
 * a dead first SEAL or the valve at 3 too; to the door's cell (0, 2, 3), the team waited on (4), the
 * door opened (5) or a charge laid (6, 7), the cell beyond (9), the charge's time (10), through (11),
 * a grenade (12), the hold (13); 14 a failure.
 * OPEN (the AI states' round 66, unwritten before), size exact, 18 instructions, all in step 9's test
 * of the first SEAL's stack: the target materialises Is()'s `&&` without narrowing it (xor/sltiu/xori
 * then beqz, the false arm's `move v0,zero` after the true arm's `b`), here an `andi 0xff` follows
 * each arm. Tried: `!Is`, `Is == false` (both these 0xFE0), `== 0`, `!!`, an int-returning inline or
 * local, the ternary forms (`?:` with `? true : false`, AiStateIs, int arms), `!Empty() &&`, `size()
 * != 0 &&`, GetStateEnum, a pointer to the stack, the phase test folded in with `||`/`&&` -- none
 * drops the andi. Settled: the path hoisted ahead of the switch (s7), `step = 2` ahead of the hold
 * call, the door valve's switch (case 0 then 1), the pushed state's cast assigned twice (`s = s &&
 * ... ? s : 0`: branches, where an inline's `?:` materialises), the action's local declared ahead of
 * the node (it then shares first's s1), SetActive before the step, `<= 9`, the timer's tick an inline
 * taking dT (read first), the two timer stores one each */
int CAiSBreach::Tick(CSealCtrlAi *ai)
{
    CEntity *seal = ai->m_entity;
    STEP step = (STEP)m_phase;

    if (m_active) {
        CSealCtrlAi *first = m_64;
        CSealCtrlAi *last = m_68;

        if (!m_70 || !m_78)
            step = (STEP)14;
        else if (!m_valve)
            m_valve = CValve::Create(GetCellNameString__7CAiMapsCFRC9CAiMapLoc(&AiMaps, m_78->m_loc), 0, VALVE_TEMP);
        if (!first || !first->m_entity->m_isAlive) {
            m_valve->SetValue(3);
            step = (STEP)14;
        } else if (m_valve->GetValue() == 3)
            step = (STEP)14;
        if (!last || !last->m_entity->m_isAlive)
            last = first;
        bool isLast = last == ai;
        bool isFirst = first == ai;
        CAiState *lead = first->FindAiState(AI_STATE_BREACH);

        if (!lead && step < 11)
            step = (STEP)14;
        CAiPathBytes *path = &ai->m_path;

        switch (step) {
        case 0: {
            m_90 = m_70->m_value;
            unsigned int i = (m_flags & 1) ? m_6c + 3 : m_6c;

            if (ai->SetRoute(BreachLocEx(m_78, i))) {
                func_00570F90(ai, 1.0f);
                step = (STEP)2;
                func_00560340(ai, this, true);
                m_valve->SetValue(1);
            } else {
                func_00560340(ai, this, false);
                step = (STEP)14;
            }
            break;
        }
        case 2: {
            unsigned char st = path->m_state;

            if (!(st == 1 || st == 2 || st == 3)) {
                if (((CAiPath *)path)->FindPoint(m_80, 0)) {
                    ((CAiPath *)path)->PrintPath("ai::breach");
                    unsigned int v = m_70->GetValue();

                    if (v)
                        v--;
                    m_70->SetValue(v);
                    m_valve->SetValue(4);
                    step = (STEP)9;
                } else
                    step = (STEP)3;
            }
            break;
        }
        case 3:
            if (func_005629B0(ai)) {
                unsigned int v = m_70->GetValue();

                if (v)
                    v--;
                m_70->SetValue(v);
                ai->SetAiMode((AI_MODE)4);
                if (ai->SetLookPosW((AI_LOOK)3, &m_58)) {
                    ai->ModifyLookFlags(1, 0);
                    ai->SetLookTimer(8.0f);
                }
                step = (STEP)4;
            }
            break;
        case 4:
            m_time += m_dT;
            if (isFirst) {
                if (m_70->GetValue() == 0)
                    step = (STEP)5;
                else if (!(m_time <= data_00650BA8))
                    step = (STEP)14;
            } else if (lead->m_phase >= 5) {
                ai->SetAiMode((AI_MODE)4);
                step = isLast ? (STEP)6 : (STEP)7;
            }
            break;
        case 5: {
            SetBit264_16(ai, false);
            SetBit264_17(ai, false);
            CAiSAction *a;
            CNodeAction *na = (CNodeAction *)m_74;

            if (na && m_78->GetType() == CAiMap::action::DOOR) {
                switch (na->m_valve->GetValue()) {
                case 0:
                    if (ai->PushAiState(AI_STATE_ACTION, 0)) {
                        CAiState *s = ai->GetAiState();

                        s = s && s->m_state == AI_STATE_ACTION ? s : 0;
                        a = (CAiSAction *)s;

                        a->SetFlags(3, 0);
                        a->UpdatePhase(6);
                        func_005F0180(a, ai, na, 1);
                        a->SetActive(true);
                        step = isLast ? (STEP)6 : (STEP)7;
                    } else
                        step = (STEP)14;
                    break;
                case 1:
                    step = isLast ? (STEP)6 : (STEP)7;
                    break;
                }
            } else
                step = isLast ? (STEP)6 : (STEP)7;
            break;
        }
        case 6:
            if (m_valve->GetValue() != 4) {
                if (m_flags & 4) {
                    int side = func_005F3270(this, ai);

                    if (ai->PushAiState(AI_STATE_DEPLOY, 0)) {
                        CAiState *s = ai->GetAiState();

                        s = s && s->m_state == AI_STATE_DEPLOY ? s : 0;
                        CAiSDeploy *d = (CAiSDeploy *)s;

                        d->m_58 = m_58;
                        d->m_64 = CPnt3D(0.0f, 0.0f, 100.0f);
                        d->m_70 = 123;
                        d->SetFlags(0xa4, 0);
                        d->m_7c = side;
                        d->m_88 = 1.0f;
                        d->SetFlags(0x10, 0);
                        d->UpdatePhase(3);
                        d->SetActive(true);
                    }
                } else if (m_flags & 2) {
                    int side = func_005F3270(this, ai);

                    if (ai->PushAiState(AI_STATE_DEPLOY, 0)) {
                        CAiState *s = ai->GetAiState();

                        s = s && s->m_state == AI_STATE_DEPLOY ? s : 0;
                        CAiSDeploy *d = (CAiSDeploy *)s;

                        d->m_58 = m_58;
                        d->m_64 = CPnt3D(0.0f, 0.0f, 100.0f);
                        d->m_70 = 121;
                        d->SetFlags(0xa4, 0);
                        d->m_7c = side;
                        d->m_88 = 1.5f;
                        d->SetFlags(0x10, 0);
                        d->UpdatePhase(3);
                        d->SetActive(true);
                    }
                }
            }
            step = (STEP)7;
            break;
        case 7:
            if (isLast)
                step = (STEP)9;
            else if (!(m_68->m_stateStack.m_10 & 0x10))
                step = (STEP)9;
            break;
        case 9:
            if (!isFirst) {
                ai->SetAiMode((AI_MODE)4);
                if (ai->SetLookPosW((AI_LOOK)3, &m_58)) {
                    ai->ModifyLookFlags(1, 0);
                    ai->SetLookTimer(5.0f);
                }
                if (first->m_stateStack.Is(AI_STATE_BREACH) == false)
                    break;
                if (lead->m_phase <= 9)
                    break;
            }
            {
                unsigned int i = (m_flags & 1) ? m_6c : m_6c + 3;
                CAiMapLoc to(BreachLocEx(m_78, i));

                if (ai->SetRoute(to)) {
                    ai->ClearLook((AI_LOOK)3);
                    step = (STEP)10;
                    m_timer.m_min = 0.5f;
                    m_timer.m_range = 0.5f;
                    m_time = data_00650BB0 * ((float)m_6c / (float)m_90);
                } else
                    step = (STEP)14;
                ((CSealSignalBits *)seal)->m_signalled = true;
            }
            break;
        case 10:
            ai->SetAiMode((AI_MODE)2);
            m_time -= m_dT;
            if (m_time <= 0.0f)
                step = (STEP)11;
            break;
        case 11: {
            float range = 0.5f * CAiEvent::GetEventRange((CAiEvent::EVENT)1);
            CEntity *e = func_00569C50(seal, &range, false);

            if (e) {
                if (!func_0060A850(ai->m_67c))
                    func_005668F0(ai, (CZSealBody *)e);
                if (BreachTimerTick(m_timer, m_dT) && BreachTargetHeld(ai))
                    ai->PlayAiSound((CHARACTER_SOUND)5, 1.0f, (CAiEvent::EVENT)-1);
            }
            if (func_005629B0(ai)) {
                if (!e)
                    step = (STEP)12;
                ai->SetAiMode((AI_MODE)1);
                m_70->SetValue(m_70->m_value + 1);
                m_time = 5.0f;
            } else
                ai->SetAiMode((AI_MODE)3);
            break;
        }
        case 12:
            if (isFirst) {
                CAiMapLoc c;
                CPnt3D p;

                func_005F3100(this, &c);
                if (AiMaps.GetCellCenter(c, &p))
                    func_00564D20(ai, 128, &p, 0, 0);
            }
            step = (STEP)13;
            break;
        case 13:
            m_valve->SetValue(2);
            m_time -= m_dT;
            if (m_time <= 0.0f || m_70->GetValue() >= m_90 || BreachTargetHeld(ai))
                SetActive(false);
            break;
        }
    }
    if (step == 14) {
        UpdatePhase(14);
        m_valve->SetValue(3);
        func_00560340(ai, this, false);
        SetActive(false);
    }
    UpdatePhase(step);
    return Result();
}

/* a goto's events: reset (no goal, the invalid cell); on entry the stance after half a second
 * (not for the player's side) and the controller's mask bit 5; the bit off for a sub-state; at
 * the end the route given up unless arrived (OVL2 0x5F5010; SOCOM 1's) */
void CAiSGoto::OnSignal(SIGNAL sig, CSealCtrlAi *ai)
{
    switch (sig) {
    case SIGNAL_4:
        *(unsigned int *)&m_cell = *(unsigned int *)&CAiMapLoc::invalid;
        m_pos = CPnt3D::zero;
        m_6c = 1.0f;
        m_58 = 0;
        m_74 = 400.0f;
        break;
    case SIGNAL_0:
        if (NavySeal(ai) == false)
            ai->SetStanceDelayed((SEAL_STANCE)0, 0.5f);
        SetMask488(ai, 0x20, 0);
        func_005E7660(((CAiCtrl10C *)ai)->m_10c);
        break;
    case SIGNAL_1:
        SetMask488(ai, 0, 0x20);
        break;
    case SIGNAL_2:
        func_005558D0(ai);
        break;
    case SIGNAL_3:
        if (m_phase != 4 && func_00555810(ai)) {
            if (func_005558A0(ai) == 2)
                UpdatePhase(0);
            func_00555820(ai, 0);
        }
        break;
    }
}

extern "C" {
/* the goto's helpers: the route dropped (OVL2 0x5628F0, SOCOM 1's ClearRoute), the squad's point-out
 * state (0x5E7690, SOCOM 1's CSealUnit::UpdatePointOutState) and its turn and hand signal (0x5E7580,
 * 0x5E7390, seal_unit.cpp's), the pursuit's target set (0x5668F0), a throttle for a speed (0x592910,
 * SOCOM 1's CSealCtrl::GetThrottleFromSpeed) -- by their names or addresses */
void func_005628F0(CSealCtrlAi *ai);
void func_005E7690(CSealUnit *unit);
bool GetMyPointOutTurnDir__9CSealUnitFP10CZSealBodyPP6CPnt3D(CSealUnit *unit, CZSealBody *seal, CPnt3D **dir);
bool GetMyPointOutAnimStart__9CSealUnitFP10CZSealBody(CSealUnit *unit, CZSealBody *seal);
void func_005668F0(CSealCtrlAi *ai, CZSealBody *seal);
CZSealBody *func_00569E40(CSealCtrlAi *ai, const CPnt3D *pos, float range);
float GetThrottleFromSpeed__9CSealCtrlFfi(CSealCtrl *ctrl, float speed, int axis);
void func_00560340(CSealCtrlAi *ai, CAiState *state, bool b);
}
/* the player (zfts's) */
CZSealBody *ftsGetPlayer();
extern "C" {
}

/* the squad's point-out under way (its byte at 9) and its word at 0x70 (views; the names ours) */
struct CAiUnitView {
    unsigned char _0[9];
    unsigned char m_pointing;   // 0x9
    unsigned char _a[0x70 - 0xa];
    int m_70;   // 0x70
};

/* a SEAL's word at 0x518, measured against the followers' limits (a view; the name ours) */
struct CAiSealView518 {
    unsigned char _0[0x518];
    int m_518;   // 0x518
};

/* the goto's limits on the player's distance -- under the first two the calls are said again, past
 * the next three each is said, the last ends the goto -- the throttle's scale and floor, and the
 * followers' limits (OVL2 data) and whether the first two were said (OVL2 bss) */
extern float data_0064FDC0;
extern float data_0064FDC8;
extern float data_0064FDD0;
extern float data_00650B38;
extern float data_00650B40;
extern int data_00650B48;
extern int data_00650B50;
extern int data_00650B58;
extern int data_00650B60;
extern int data_00650B68;
extern int data_00650B70;
extern int data_00650B78;
extern bool data_0066B9E8;
extern bool data_0066B9F0;

/* two cells' x and y distances summed (seal_aifollow.cpp's FollowCellDist; the name ours) */
static inline int GotoCellDist(const CAiMapLoc &a, const CAiMapLoc &b)
{
    int dx = a.x > b.x ? a.x - b.x : b.x - a.x;
    int dy = a.y > b.y ? a.y - b.y : b.y - a.y;

    return dx + dy;
}

/* the route's first point and the SEAL's cell, each through an inline (their returned values are
 * temporaries numbered after the distance's: as named locals the SEAL's cell address and the x
 * distance swap a1/a2; the names ours) */
static inline const CAiMapLoc *GotoPathFirstP(CAiPathBytes *p)
{
    return *(CAiMapLoc **)&p->_a0;
}

static inline const CAiMapLoc *GotoSealCell(CZSealBody *s)
{
    return &s->m_mapLocValid;
}

/* whether the route arrived (inline; the name ours) */
static inline bool GotoArrived(CAiPathBytes *path)
{
    return !(path->m_9c == 0) && path->m_state == 4;
}

/* a goto's frame (OVL2 0x5F4470; SOCOM 1's grown): teleported (flag 0x80); for the squad's point-out
 * (flag 8) the SEAL turns and signals; the speed and stance by flags 2, 4/0x10 and 1; then the route
 * set (0), planned (1), walked (2) -- flag 0x40 done within m_58 cells of the route's end, flag 8
 * keeping pace with the player and calling to him --, a pursuit pushed at the end (4) or the end (3).
 * Byte-exact (the kit's round 64, unwritten before): the pursuit's failure arm first and the pushed
 * state kept by `s = s && s->m_state == K ? s : 0` -- with an if, no block optimisation, and the
 * target's fall-through delay slots (left nops) fill; the step, the two flags and the pursuit's point
 * declared at the top (the registers and the two points' slots); the speed and stance constants
 * OVL2 data, not literals; the cover search's cell a const reference; the arrival `ok` one `&&`
 * (no andi); the distances compared through named locals (as an expression MW computes the right
 * side first); the player's speed into a local before the product (`mul.s f12,f0,f1`) */
int CAiSGoto::Tick(CSealCtrlAi *ai)
{
    STEP step;
    bool slow;
    bool done;
    CPnt3D c4;
    CZSealBody *seal = (CZSealBody *)ai->m_entity;

    if (m_flags & 0x80) {
        CPnt3D c;

        func_005628F0(ai);
        if (AiMaps.GetCellCenter(m_cell, &c))
            seal->TeleportTo(&c, &seal->m_quat);
        return 1;
    }
    step = (STEP)m_phase;

    if (m_flags & 8) {
        CSealUnit *unit = ai->m_unit;

        func_005E7690(unit);
        if (((CAiUnitView *)unit)->m_pointing) {
            CPnt3D *dir = 0;

            if (GetMyPointOutTurnDir__9CSealUnitFP10CZSealBodyPP6CPnt3D(ai->m_unit, seal, &dir) &&
                ai->SetLookDirW((AI_LOOK)3, dir, true)) {
                ai->ModifyLookFlags(1, 0);
                ai->SetLookTimer(10.0f);
            }
            if (GetMyPointOutAnimStart__9CSealUnitFP10CZSealBody(ai->m_unit, seal))
                seal->DoHandSignal((SEAL_HANDSIGNAL)5);
            ai->SetAiMode((AI_MODE)1);
            func_00570F90(ai, 0.0f);
            return 1;
        }
        ai->SetAiMode((AI_MODE)3);
    } else if (m_flags & 2) {
        m_6c = data_0064FDC0;
        ai->SetStanceDelayed((SEAL_STANCE)1, -1.0f);
    } else if ((m_flags & 4) || (m_flags & 0x10)) {
        m_6c = data_0064FDC8;
        ai->SetStanceDelayed((SEAL_STANCE)1, -1.0f);
    } else if (m_flags & 1) {
        m_6c = data_0064FDD0;
        ai->SetStanceDelayed((SEAL_STANCE)2, -1.0f);
    }
    CAiPathBytes *path = &ai->m_path;

    switch (step) {
    case STEP_0:
        ai->SetAiMode((AI_MODE)2);
        if (m_flags & 0x20) {
            CAiMap::DIR dir;
            const CAiMapLoc &here = seal->m_mapLocValid;

            if (!AiMaps.GetMapByIndex(here.m_mapid)->FindCoverLoc(here, &m_pos, m_cell, dir, 5))
                m_cell = CAiMapLoc::invalid;
        }
        if (ai->SetRoute(m_cell)) {
            step = STEP_1;
            func_00560340(ai, this, true);
        } else {
            SetFlags(0, 0x100);
            func_00560340(ai, this, false);
            step = STEP_3;
        }
        break;
    case STEP_1: {
        unsigned char st = path->m_state;

        if (!(st == 1 || st == 2 || st == 3)) {
            bool ok = !(path->m_9c == 0) && st == 4;

            if (ok) {
                func_00560340(ai, this, true);
                ai->SetAiMode((AI_MODE)3);
                func_00570F90(ai, m_6c);
                ((CAiPath *)path)->RebuildLegs(&AiMaps, true);
                step = STEP_2;
            } else {
                SetFlags(0, 0x100);
                func_00560340(ai, this, false);
                step = STEP_3;
            }
        }
        break;
    }
    case STEP_2:
        if (m_flags & 1)
            ai->SetStanceDelayed((SEAL_STANCE)2, -1.0f);
        if ((m_flags & 0x40) && GotoArrived(path)) {
            if ((unsigned int)GotoCellDist(*GotoPathFirstP(path), *GotoSealCell(seal)) <= m_58) {
                func_005628F0(ai);
                step = STEP_3;
            }
        } else if (func_005629B0(ai))
            step = STEP_3;
        else if (m_flags & 8) {
            slow = false;
            done = false;
            CZSealBody *player = ftsGetPlayer();
            CAiMapLoc goal(m_cell);
            CAiMapLoc mine(seal->m_mapLocValid);
            CAiMapLoc his(player->m_mapLocValid);
            int d = GotoCellDist(mine, his);

            int dm = GotoCellDist(goal, mine);
            int dh = GotoCellDist(goal, his);

            if (dm < dh) {
                if (d < data_00650B50)
                    data_0066B9E8 = false;
                if (d < data_00650B60)
                    data_0066B9F0 = false;
                if (data_00650B48 < d) {
                    if (!data_0066B9E8) {
                        ai->PlayAiSound((CHARACTER_SOUND)1, 1.0f, CAiEvent::AIE_NONE);
                        data_0066B9E8 = true;
                    }
                    slow = true;
                } else
                    slow = false;
                if (data_00650B58 < d && !data_0066B9F0) {
                    ai->PlayAiSound((CHARACTER_SOUND)2, 1.0f, CAiEvent::AIE_NONE);
                    data_0066B9F0 = true;
                }
                if (data_00650B68 < d) {
                    ai->PlayAiSound((CHARACTER_SOUND)0, 1.0f, CAiEvent::AIE_NONE);
                    done = true;
                }
            }
            if (!slow) {
                CSealCtrl *pc = (CSealCtrl *)player->m_control;
                float speed = pc->GetSpeedFromThrottle(pc->m_throttle[0], 0);
                float t = GetThrottleFromSpeed__9CSealCtrlFfi(ai, speed * data_00650B38, 0);

                if (t < data_00650B40)
                    t = data_00650B40;
                func_00570F90(ai, t);
            } else
                func_00570F90(ai, 0.0f);
            if (done) {
                SetFlags(0, 0x100);
                func_00560340(ai, this, false);
                step = STEP_3;
            }
        }
        if (seal->m_followers > 0) {
            if (((CAiSealView518 *)seal)->m_518 < (((CAiUnitView *)ai->m_unit)->m_70 ? data_00650B78 : data_00650B70)) {
                if (!(m_flags & 8))
                    func_00570F90(ai, m_6c);
            } else
                func_00570F90(ai, 0.0f);
        }
        break;
    case STEP_4: {
        CZSealBody *target = 0;

        if (AiMaps.GetCellCenter(m_cell, &c4))
            target = func_00569E40(ai, &c4, m_74);
        if (!target || !ai->PushAiState(AI_STATE_PURSUE, 0)) {
            ai->PlayAiSound((CHARACTER_SOUND)3, 1.0f, CAiEvent::AIE_NONE);
            step = STEP_0;
            SetFlags(0, 0x100);
        } else {
            func_005668F0(ai, target);
            CAiState *s = ai->GetAiState();

            s = s && s->m_state == AI_STATE_PURSUE ? s : 0;
            s->SetFlags(2, 4);
            s->SetActive(true);
        }
        break;
    }
    case STEP_3:
        if (m_flags & 0x100)
            step = STEP_4;
        else {
            if (m_flags & 0x400)
                ai->PlayAiSound((CHARACTER_SOUND)83, 1.0f, CAiEvent::AIE_NONE);
            SetActive(false);
        }
        break;
    }
    UpdatePhase(step);
    return Result();
}

/* the pickup gone for, its network id kept to check it still stands, and the flags (OVL2
 * 0x5F5520; the name ours) */
void CAiSPickup::SetPickup(CPickup *p, unsigned int flags)
{
    m_pickup = p;
    m_pickupId = GetPickupNetEntityID__7CPickupFv(p);
    SetFlags(flags, 0);
}

/* the nearest pickup to a point (FindNearestPickup, OVL2 0x6084B0, unwritten: the point, a kind
 * mask, a range) and the range the AI looks within (OVL2 data 0x650B30, read as a variable) */
CPickup *FindNearestPickup(const CPnt3D *pos, unsigned char mask, float range);
extern "C" {
extern float data_00650B30;
}

/* the nearest pickup gone for: the pickup state pushed when allowed and handed the pickup with its
 * flag bit 0; whether it was (OVL2 0x5F51A0; the surrender's Tick calls it -- a controller helper,
 * unnamed: its second argument unread) */
extern "C" bool func_005F51A0(CSealCtrlAi *ai, int x, unsigned int flags)
{
    CPickup *p = FindNearestPickup(&ai->m_entity->m_posW, 255, data_00650B30);
    bool r = false;

    if (p && ai->PushAiState(AI_STATE_PICKUP, flags)) {
        CAiState *s = ai->GetAiState();
        CAiSPickup *pick = s && s->m_state == AI_STATE_PICKUP ? (CAiSPickup *)s : 0;

        pick->SetPickup(p, 1);
        r = true;
    }
    return r;
}

/* a pickup: to a cell beside it (the goto pushed, coming back to step 6), the AI's mode 4, the
 * pickup taken (when the flag's bit 0 asks) and marked seen by this SEAL, then three seconds on
 * (OVL2 0x5F52D0; SOCOM 1's). The bit's low five bits through an enum-typed
 * argument (a modelling device, the game states' round-53 rule: IRO's propagation leaves an enum-typed
 * variable alone, so the argument's temporary stays where the replay of round 59 wanted it, numbered
 * before SetBit's index -- 40 integer spellings over rounds 39-59 stayed at 4 off) */
enum PickBit_ { PICKBIT_MAX = 0x7fffffff };
/* a bit of a CBits set (inlined; zmath.h's CBits has Test and Clear only -- the name ours) */
inline void SetBitE(CBits *bits, unsigned int i, PickBit_ bit)
{
    bits->m_vec[i >> 5] |= 1 << bit;
}
int CAiSPickup::Tick(CSealCtrlAi *ai)
{
    STEP step;
    CPickup *p;
    CEntity *seal = ai->m_entity;

    if (!m_active)
        return AI_DONE;
    p = m_pickup;
    step = (STEP)m_phase;

    if (!p || m_pickupId != GetPickupNetEntityID__7CPickupFv(p))
        return AI_DONE;
    switch (step) {
    case STEP_0: {
        unsigned int loc;

        if (!GetLocation__7CAiMapsFPC6CPnt3DR9CAiMapLoc(&AiMaps, p->GetPosW(), &loc) && !GetTraversableLocation__7CAiMapsFPC6CPnt3DR9CAiMapLoc(&AiMaps, m_pickup->GetPosW(), &loc))
            step = STEP_5;
        else {
            step = STEP_6;
            func_005F0830(ai, &loc, 4, 2, 6);
        }
        break;
    }
    case STEP_2:
        ai->SetAiMode((AI_MODE)4);
        step = STEP_3;
        break;
    case STEP_3:
        if (m_flags & 1)
            PickupObject__10CZSealBodyFP7CPickup(seal, p);
        step = STEP_4;
        SetBitE((CBits *)p->m_bits, seal->m_index, (PickBit_)(seal->m_index & 31));
        m_time = StateTimer(ai) + 3.0f;
        break;
    case STEP_4:
        if (!(StateTimer(ai) < m_time))
            step = STEP_5;
        break;
    case STEP_5:
        m_active = false;
        break;
    case STEP_6:
        m_active = false;
        break;
    }
    UpdatePhase(step);
    return Result();
}

/* a pickup's reset (OVL2 0x5F5590; SOCOM 1's) */
void CAiSPickup::OnSignal(SIGNAL sig, CSealCtrlAi *ai)
{
    switch (sig) {
    case SIGNAL_4:
        m_pickup = 0;
        m_pickupId = 0;
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

/* the unknown state: the AI's mode 8 on entry and exit, 0x56D090 twice otherwise (OVL2 0x5F1BF0;
 * SOCOM 1's was empty) */
void CAiSUnknown::OnSignal(SIGNAL sig, CSealCtrlAi *ai)
{
    switch (sig) {
    case SIGNAL_4:
        break;
    case SIGNAL_0:
    case SIGNAL_3:
        ai->SetAiMode((AI_MODE)8);
        break;
    case SIGNAL_1:
    case SIGNAL_2:
        ai->RestoreAiMode();
        ai->RestoreAiMode();
        break;
    }
}

/* a CQB attack's reset, and its delay on entry (OVL2 0x5F1AD0; SOCOM 1's) */
void CAiSCQBAttack::OnSignal(SIGNAL sig, CSealCtrlAi *ai)
{
    switch (sig) {
    case SIGNAL_4:
        m_target = 0;
        m_5c = 0.0f;
        break;
    case SIGNAL_0:
        m_5c = ai->m_navyseal ? 0.0f : 0.75f;
        break;
    case SIGNAL_1:
        break;
    case SIGNAL_2:
        break;
    case SIGNAL_3:
        break;
    }
}

/* a suspended SEAL told: on entry (0) the AI's throttles stopped and what it is kept -- its node on,
 * its place, its two target-list bits, its anim set; leaving (1) the node put back in the world, the
 * SEAL teleported to its place, the bits given back and the anim set bound again when the flag's bit
 * 4 asks; 3 sets bit 0, a reset (4) forgets the place and the set, 5 drops the node bit, 6 keeps the
 * bits and shuts the node (OVL2 0x5F1E40; SOCOM 1's had 0-4, its node read in each case) */
void CAiSSuspended::OnSignal(SIGNAL sig, CSealCtrlAi *ai)
{
    CEntity *seal = ai ? ai->m_entity : 0;
    zdb::CNode *node = seal ? seal->GetNode() : 0;

    switch (sig) {
    case SIGNAL_0:
        ai->StopThrottles(false);
        m_nodeActive = (bool)node->m_active;
        m_matrix.SetIdentity();
        node->BuildMTW(m_matrix);
        m_includeInTargets = (bool)seal->m_include_in_targetlist;
        m_updateTargets = (bool)seal->m_update_targetlist;
        m_animset = ((CZSealBody *)seal)->m_animset;
        break;
    case SIGNAL_1:
        if (node->m_parent != zdb::CWorld::m_world)
            zdb::CWorld::m_world->AddChild(node);
        seal->TeleportTo(&m_matrix);
        node->SetActive(m_nodeActive);
        seal->SetUpdateTargetList(m_updateTargets);
        seal->SetIncludeInTargetList(m_includeInTargets);
        if (m_animset && (m_flags & 0x10)) {
            ((CZSealBody *)seal)->BindAnimSet(m_animset, false);
            m_animset = 0;
        }
        break;
    case SIGNAL_2:
        break;
    case SIGNAL_3:
        SetBit0(true);
        break;
    case SIGNAL_4:
        m_nodeActive = true;
        m_matrix.SetIdentity();
        m_animset = 0;
        break;
    case (SIGNAL)5:
        m_nodeActive = false;
        break;
    case (SIGNAL)6:
        m_nodeActive = (bool)node->m_active;
        m_includeInTargets = (bool)seal->m_include_in_targetlist;
        m_updateTargets = (bool)seal->m_update_targetlist;
        node->SetActive(false);
        break;
    }
}

/* a suspended SEAL: faded out first when its flag's bit 3 asks (the node's opacity run to 0 at
 * 0.75 a second, then the node shut and the fade dropped), then held out of the world -- its node
 * off (bit 0, unless it faded), its target list no longer updated (bit 1) nor holding it (bit 2)
 * (OVL2 0x5F1C70; SOCOM 1's, which faded in from below as well).
 * Byte-exact in the kit's round 59 (10 off since round 6): the opacity read into a register local
 * that the two tests compare and that initialises the saturated copy -- the tests share the load
 * and the arms re-read the copy from its frame slot, the step built in $f1 over the compared value,
 * as the target and SOCOM 1's; compared on the copy itself MW forwards the load into the arms.
 * The registers, the declaration order (node, seal, fading) and the early m_dT load are the
 * target's. */
int CAiSSuspended::Tick(CSealCtrlAi *ai)
{
    zdb::CNode *node;
    CEntity *seal = ai->m_entity;
    bool fading = (m_flags & 8) != 0;

    node = seal->GetNode();
    if (fading) {
        static float lo = 0.0f;
        static float hi = 1.0f;
        float dT = m_dT;
        float target = 0.0f;
        float cur = node->m_Opacity;
        float o = cur;

        if (cur != target) {
            float step = 0.75f * dT;

            if (target > cur) {
                o += step;
                _saturate(&o, lo, target);
            } else {
                o -= step;
                _saturate(&o, target, hi);
            }
            node->m_Opacity = o;
        }
        node->SetActive(o != 0.0f);
        if (!node->m_active)
            SetFlags(0, 8);
    }
    if (m_bit0) {
        if (!fading && (m_flags & 1))
            node->SetActive(false);
        if (m_flags & 2)
            seal->SetUpdateTargetList(false);
        if (m_flags & 4)
            seal->SetIncludeInTargetList(false);
    }
    return Result();
}

/* the unknown state holds the AI still: its throttles set each frame (OVL2 0x5F1B50; SOCOM 1's was
 * empty) */
int CAiSUnknown::Tick(CSealCtrlAi *ai)
{
    static float x = 1.0f;
    static float z = 1.0f;

    ai->SetThrottle(0, x);
    ai->SetThrottle(1, 0.0f);
    ai->SetThrottle(2, z);
    return Result();
}

/* a CQB attack: the delay run down, the attack (or a second's pause when the target is out of
 * reach), then done once the state clock passes the deadline (OVL2 0x5F1960; SOCOM 1's, the
 * frame's time now the states' static and the result inverted -- true when done) */
int CAiSCQBAttack::Tick(CSealCtrlAi *ai)
{
    CEntity *seal = ai->m_entity;

    if (!m_target)
        return AI_DONE;
    unsigned int phase = m_phase;

    switch (phase) {
    case 0:
        m_5c -= m_dT;
        if (m_5c <= 0.0f)
            phase = 1;
        break;
    case 1:
        ai->SetStance((SEAL_STANCE)0);
        if (((CZSealBody *)seal)->CanUseCQB((CZSealBody *)m_target)) {
            ExecuteAction_CQBAttack__10CZSealBodyFP10CZSealBody(seal, m_target);
            m_time = StateTimer(ai) + 2.5f;
        } else
            m_time = StateTimer(ai) + 1.0f;
        phase = 2;
        break;
    case 2:
        if (!(StateTimer(ai) < m_time))
            m_active = false;
        break;
    }
    UpdatePhase(phase);
    return Result();
}

/* the destructors (OVL2 0x5F8A70, 0x5F89C0, 0x5F8940, 0x5F8880, 0x5F8800, 0x5F8770): each its
 * own table stored, then CAiState's, its sub-state stack destroyed out of line */
CAiSPickup::~CAiSPickup()
{
}

CAiSGoto::~CAiSGoto()
{
}

CAiSScript::~CAiSScript()
{
}

CAiSSuspended::~CAiSSuspended()
{
}

CAiSUnknown::~CAiSUnknown()
{
}

CAiSCQBAttack::~CAiSCQBAttack()
{
}

/* (OVL2 0x5F86C0) its members destroyed by hand, as ai_states.h keeps them as bytes: the route
 * (~CAiPath, 0x5281B0, with -1), then the vector as the CAiMapLocVec it is (its deleter's destructor
 * 0x515200 called with 0 behind the three this-tests of ~vector and ~__vector_imp inlined) */
CAiSPlan::~CAiSPlan()
{
    __dt__7CAiPathFv(m_path, -1);
    ((CAiMapLocVec *)_58)->~CAiMapLocVec();
}

/* (OVL2 0x5EF9D0, the unit's first function) */
CAiSAction::~CAiSAction()
{
}
