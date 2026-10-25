/* seal_aistate.cpp -- the AI states' core: CQB attack, unknown, suspended, breach, script, goto,
 * pickup (SOCOM 1's unit, __sinit_seal_aistate.cpp, with its static CAiSUnknown; SOCOM 1 filed it
 * beside the other seal_*.cpp under gamez/zseal, this tree keeps the AI states under src/ai).
 * OVL2 0x5F1960-0x5F8BFF. So far only the unit's tail: the states' destructors and the inline
 * virtuals their tables pull in, and CAiStateStack's two out-of-line members. Defined in reverse
 * of their image order (-inline deferred). */

#include "../gamez/zseal/zseal.h"
#include "../gamez/zanim/zanim.h"

extern "C" {
/* a CSealCtrlAi function (OVL2 0x56D090, the SEAL lane's range), called twice on leaving the
 * unknown state */
void func_0056D090(CSealCtrlAi *ai);
/* the AI's mode, a byte (OVL2 0x560A60, the SEAL lane's range) */
unsigned char func_00560A60(CSealCtrlAi *ai);
}

/* the AI controller's bit the CQB attack's delay reads (CSealCtrlAi at 0x264, bit 1; zseal.h does
 * not name it -- the names ours) */
struct CAiCtrlBits {
    unsigned char _0[0x264];
    unsigned char m_bit0 : 1;
    unsigned char m_nodelay : 1;
};

/* the controller's state clock: its stack's (CSealCtrlAi's stack at 0x41C, the clock 0x18 into
 * it -- SOCOM 1's m_stateTimer at 0x330; zseal.h does not name it) */
struct CAiCtrlClock {
    unsigned char _0[0x434];
    float m_stateTimer;   // 0x434
};

inline float StateTimer(CSealCtrlAi *ai)
{
    return ((CAiCtrlClock *)ai)->m_stateTimer;
}

/* the controller's state stack (CSealCtrlAi at 0x41C; zseal.h keeps a word there) */
struct CAiCtrlStack {
    unsigned char _0[0x41c];
    CAiStateStack m_stack;   // 0x41c
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

/* the state an empty stack answers with (SOCOM 1's _unknown, 0x509870: this unit's static,
 * built by its initialiser; OVL2 bss 0x66B990 -- declared here, not defined, so this unit emits
 * no initialiser yet) */
extern CAiSUnknown _unknown;

extern "C" {
/* the controller's helpers the goto's events call (OVL2 0x555810, 0x555820, 0x5558A0, 0x5558D0
 * -- the SEAL lane's rows), the stance set after a delay (0x563100) and a squad's call on the
 * controller's word at 0x10C (0x5E7660, seal_unit.cpp's, a byte-exact draft) */
bool func_00555810(CSealCtrlAi *ai);
void func_00555820(CSealCtrlAi *ai, int x);
unsigned char func_005558A0(CSealCtrlAi *ai);
void func_005558D0(CSealCtrlAi *ai);
void func_00563100(CSealCtrlAi *ai, signed char stance, float delay);
void func_005E7660(void *unit);
/* a CSealCtrl call taking a signed byte (OVL2 0x600D40, unwritten) and CZSealBody's weapon state
 * (named) */
void func_00600D40(CSealCtrlAi *ai, int x);
void SetWeaponState__10CZSealBodyF12WEAPON_STATE(CEntity *seal, int state);
/* CPickup's and the AI maps' (OVL1 0x5416C0 named; OVL2 0x52DC70, 0x529A30, the AI core's rows:
 * a cell for a point, and SOCOM 1's free FindTraversableNeighbor) */
unsigned int GetPickupNetEntityID__7CPickupFv(CPickup *p);
bool func_0052DC70(CAiMaps *maps, const CPnt3D *pos, void *loc);
bool func_00529A30(CAiMaps *maps, const CPnt3D *pos, void *loc);
/* the goto pushed to a cell (OVL2 0x5F0830, this unit's: the AI, the cell, a mode, a kind and
 * the step to come back to -- the reading ours) */
void func_005F0830(CSealCtrlAi *ai, void *loc, int mode, int kind, int step);
void PickupObject__10CZSealBodyFP7CPickup(CEntity *seal, CPickup *pickup);
/* CSealCtrlAi::SetStance (SOCOM 1's SetStance__11CSealCtrlAiF11SEAL_STANCE, called from the same
 * Ticks; OVL2 0x5631A0, the SEAL lane's row) */
void func_005631A0(CSealCtrlAi *ai, int stance);
/* CZSealBody::CanUseCQB (SOCOM 1's CanUseCQB__10CZSealBodyFP10CZSealBody, called from the same
 * CQB Tick; OVL2 0x5A9500, the SEAL lane's row) */
bool func_005A9500(CEntity *seal, CEntity *target);
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

/* +0x38: the state's phase word below 6 (the name ours) */
bool CAiSAction::Virtual38()
{
    return m_phase < 6;
}

/* a script takes kinds 12 and 26 only when it may be pursued from (OVL2 0x5F40F0).
 * OPEN, 0x84 against 0x80: the target keeps the inlined test's result in s0, zeroed in the delay
 * slot of the !m_hold branch (materialised, sltu/xori, as NotHeld() gives it) and set to 1 after
 * the virtual CanPursue, then copies it with andi; here the inline's return true/false lands in
 * v0 at its own labels -- a result local assigned false first is if-converted into an sltu on
 * the call's result instead (0x6C), with == true or != false 0x74 */
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
    return m_mode == func_00560A60(ai) && m_60 >= 0.5f;
}

/* (OVL2 0x5F4080) */
bool CAiSScript::CanAttack() const
{
    return true;
}

/* the controller's state of a kind (SOCOM 1's FindAiState; OVL2 0x5F5FC0, a byte-exact draft:
 * zseal.h's CSealCtrlAi does not declare it) */
extern "C" CAiState *FindAiState__11CSealCtrlAiCF8AI_STATE(const CSealCtrlAi *ai, AI_STATE state);

/* the script state (kind 1) running sequence `id` and told event 2: this state is done (OVL2
 * 0x5F6050) */
void CAiState::Virtual0C(int id, int event, CSealCtrlAi *ai)
{
    CAiState *s = FindAiState__11CSealCtrlAiCF8AI_STATE(ai, (AI_STATE)1);
    CAiSScript *script = s && s->m_state == 1 ? (CAiSScript *)s : 0;

    if (script && script->m_58 == id && (unsigned char)event == 2)
        m_active = false;
}

/* the state on top of the controller's stack, or the unknown state when it is empty (OVL2
 * 0x5F6010, 0x5F5FD0 -- the same code twice; SOCOM 1's tested the top for 0 instead) */
CAiState *CSealCtrlAi::GetAiState()
{
    if (((CAiCtrlStack *)this)->m_stack.size() == 0)
        return &_unknown;
    return ((CAiCtrlStack *)this)->m_stack.GetState();
}

const CAiState *CSealCtrlAi::GetAiState() const
{
    if (((const CAiCtrlStack *)this)->m_stack.size() == 0)
        return &_unknown;
    return ((CAiCtrlStack *)this)->m_stack.GetState();
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
 * same code twice, the non-const and the const one) */
CAiState *CAiStateStack::GetState()
{
    if (size() == 0)
        return 0;
    return front();
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

/* the controller's bit word at 0x264 (SOCOM 1's 0x280: zseal.h names bits 0 and 1 only; the
 * names ours) and the setters the target passes a bool through */
struct CAiCtrl264 {
    unsigned char _0[0x264];
    unsigned int m_b0 : 13;
    unsigned int m_bit13 : 1;   // 0x265 bit 5
    unsigned int m_b14 : 2;
    unsigned int m_bit16 : 1;   // 0x266 bit 0
    unsigned int m_bit17 : 1;   // 0x266 bit 1
    unsigned int m_b18 : 14;
};

inline void SetBit264_13(CSealCtrlAi *ai, bool on)
{
    ((CAiCtrl264 *)ai)->m_bit13 = on;
}

inline void SetBit264_17(CSealCtrlAi *ai, bool on)
{
    ((CAiCtrl264 *)ai)->m_bit17 = on;
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
        func_00600D40(ai, -1);
        func_005631A0(ai, 1);
        SetBit264_13(ai, false);
        break;
    case SIGNAL_1:
        if (!seal->m_isAlive && m_valve)
            m_valve->SetValue(3);
        SetWeaponState__10CZSealBodyF12WEAPON_STATE(seal, 2);
        ((CAiCtrl264 *)ai)->m_bit16 = m_bit13 != 0;
        SetBit264_17(ai, false);
        ((CAiCtrl264 *)ai)->m_bit13 = true;
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

/* the controller's word at 0x10C (CSealCtrl's; zseal.h does not name it) */
struct CAiCtrl10C {
    unsigned char _0[0x10c];
    void *m_10c;   // 0x10c
};

/* a goto's events: reset (no goal, the invalid cell); on entry the stance after half a second
 * (not for the player's side) and the controller's mask bit 5; the bit off for a sub-state; at
 * the end the route given up unless arrived (OVL2 0x5F5010; SOCOM 1's) */
void CAiSGoto::OnSignal(SIGNAL sig, CSealCtrlAi *ai)
{
    switch (sig) {
    case SIGNAL_4:
        m_cell = *(unsigned int *)&CAiMapLoc::invalid;
        m_pos = CPnt3D::zero;
        m_6c = 1.0f;
        m_58 = 0;
        m_74 = 400.0f;
        break;
    case SIGNAL_0:
        if (NavySeal(ai) == false)
            func_00563100(ai, 0, 0.5f);
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

/* a bit of a CBits set (inlined; zmath.h's CBits has Test and Clear only -- the name ours) */
inline void SetBit(CBits *bits, unsigned int i)
{
    bits->m_vec[i >> 5] |= 1 << (i & 31);
}

/* the pickup gone for, its network id kept to check it still stands, and the flags (OVL2
 * 0x5F5520; the name ours) */
void CAiSPickup::SetPickup(CPickup *p, unsigned int flags)
{
    m_pickup = p;
    m_pickupId = GetPickupNetEntityID__7CPickupFv(p);
    SetFlags(flags, 0);
}

/* a pickup: to a cell beside it (the goto pushed, coming back to step 6), the AI's mode 4, the
 * pickup taken (when the flag's bit 0 asks) and marked seen by this SEAL, then three seconds on
 * (OVL2 0x5F52D0; SOCOM 1's).
 * OPEN, 4 instructions, two registers in the bit's set: the target holds the SEAL's index in a1
 * and its low five bits in a2, here the other way round. Tried: the inline's parameters swapped,
 * a CBits member, a nested inline on the pickup, the mask computed first, `/ 32` and `% 32`, an
 * int index, the step assigned before, after or around it. Everything else is the target's. */
AI_RESULT CAiSPickup::Tick(float dT, CSealCtrlAi *ai)
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

        if (!func_0052DC70(&AiMaps, p->GetPosW(), &loc) && !func_00529A30(&AiMaps, m_pickup->GetPosW(), &loc))
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
        SetBit((CBits *)p->m_bits, seal->m_index);
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
        func_0056D090(ai);
        func_0056D090(ai);
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
        m_5c = ((CAiCtrlBits *)ai)->m_nodelay ? 0.0f : 0.75f;
        break;
    case SIGNAL_1:
        break;
    case SIGNAL_2:
        break;
    case SIGNAL_3:
        break;
    }
}

/* a suspended SEAL: faded out first when its flag's bit 3 asks (the node's opacity run to 0 at
 * 0.75 a second, then the node shut and the fade dropped), then held out of the world -- its node
 * off (bit 0, unless it faded), its target list no longer updated (bit 1) nor holding it (bit 2)
 * (OVL2 0x5F1C70; SOCOM 1's, which faded in from below as well).
 * OPEN, 10 instructions, the fade's two arms only: the target computes the step into $f1 (over
 * the opacity it just compared) and re-reads the opacity from its frame slot in each arm, as
 * SOCOM 1's does; here MW keeps the compared value in $f1 and builds the step in $f0. Tried: the
 * step inline in each arm or as a local (before, inside, uninitialised ahead), the arms through
 * a reference or `*&o`, the compare on node->m_Opacity (reloads it). The registers, the
 * declaration order (node, seal, fading) and the early m_dT load are the target's. */
AI_RESULT CAiSSuspended::Tick(float dT, CSealCtrlAi *ai)
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
        float o = node->m_Opacity;

        if (o != target) {
            float step = 0.75f * dT;

            if (target > o) {
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
AI_RESULT CAiSUnknown::Tick(float dT, CSealCtrlAi *ai)
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
AI_RESULT CAiSCQBAttack::Tick(float dT, CSealCtrlAi *ai)
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
        func_005631A0(ai, 0);
        if (func_005A9500(seal, m_target)) {
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

/* (OVL2 0x5EF9D0, the unit's first function) */
CAiSAction::~CAiSAction()
{
}
