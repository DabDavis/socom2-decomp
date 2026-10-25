/* seal_aistate.cpp -- the AI states' core: CQB attack, unknown, suspended, breach, script, goto,
 * pickup (SOCOM 1's unit, __sinit_seal_aistate.cpp, with its static CAiSUnknown; SOCOM 1 filed it
 * beside the other seal_*.cpp under gamez/zseal, this tree keeps the AI states under src/ai).
 * OVL2 0x5F1960-0x5F8BFF. So far only the unit's tail: the states' destructors and the inline
 * virtuals their tables pull in, and CAiStateStack's two out-of-line members. Defined in reverse
 * of their image order (-inline deferred). */

#include "../gamez/zseal/zseal.h"

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

extern "C" {
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

/* a pickup's reset (OVL2 0x5F5590; SOCOM 1's) */
void CAiSPickup::OnSignal(SIGNAL sig, CSealCtrlAi *ai)
{
    switch (sig) {
    case SIGNAL_4:
        m_58 = 0;
        m_5c = 0;
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
        if (!node->m_active) {
            m_flags |= 0x80000000;
            m_flags &= ~8;
        }
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
    SetPhase(phase);
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
