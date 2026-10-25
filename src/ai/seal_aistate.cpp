/* seal_aistate.cpp -- the AI states' core: CQB attack, unknown, suspended, breach, script, goto,
 * pickup (SOCOM 1's unit, __sinit_seal_aistate.cpp, with its static CAiSUnknown; SOCOM 1 filed it
 * beside the other seal_*.cpp under gamez/zseal, this tree keeps the AI states under src/ai).
 * OVL2 0x5EF9D0-0x5F8BFF: the states' destructors and the inline virtuals their tables pull in,
 * CAiStateStack's out-of-line members, CSealCtrlAi::GetAiState, the base's slot 0x0C and the
 * helpers the response states' Ticks call, the CQB/unknown/pickup Ticks and five OnSignals
 * (round 3); CAiState::Create (0x5F6550, every state's constructor inlined one level), Reset's
 * copy (0x5F6D40), CAiState::Signal (0x5F6FA0, Reset inlined whole), the stack's constructor and
 * its Pop (round 4). The stack's other members (0x5F7470-0x5F8270: SOCOM 1's PushAiState,
 * SetAiState, PopToAiState, StateTick ... moved onto CAiStateStack, the AI passed) and the
 * controller's wrappers after StateTick (0x5F5B10-0x5F5CB0) are next. */

#include "../gamez/zseal/zseal.h"
#include "../gamez/zanim/zanim.h"

#include <algorithm>

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

/* the object's id told an event: 1 drops it, 6 sets the flags' bit 2; the AI's stack bit 0 set
 * either way (OVL2 0x5F06B0) */
void CAiSAction::Virtual0C(int id, int event, CSealCtrlAi *ai)
{
    if (id == m_84) {
        switch ((unsigned char)event) {
        case 1:
            m_84 = 0;
            break;
        case 6:
            SetFlags(4, 0);
            break;
        }
        ((CAiCtrlStack *)ai)->m_stack.SetFlags(1, 0);
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

/* the plan's route handed to the controller (OVL2 0x573110, the SEAL lane's row) and given back
 * to the AI maps (0x52C310, the AI core's) */
extern "C" void func_00573110(CSealCtrlAi *ai, void *path);
extern "C" void func_0052C310(CAiMaps *maps, void *path);

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
    unsigned char _70[0x74 - 0x70];
    CAiMapLoc m_74;   // 0x74
    CAiMapLoc m_78;   // 0x78
    unsigned char _7c[0x9c - 0x7c];   // 0x9c bytes
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
 * stack emptied (0x5F7920, a byte-exact draft), a squad's member (0x5EDBB0, seal_unit.cpp's,
 * unwritten), and the controller's calls (0x56D890, 0x5631B0, 0x570F90: the SEAL lane's rows) */
void func_005F6350(CAiState *s, CSealCtrlAi *ai, CAiState::SIGNAL sig);
void func_005F7920(CAiStateStack *stack, CSealCtrlAi *ai);
CEntity *func_005EDBB0(void *unit, int a, int b);
void func_0056D890(CSealCtrlAi *ai, int x);
void func_005631B0(CSealCtrlAi *ai, int a, int b, float f);
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

/* a state of a stack told a signal (OVL2 0x5F6FA0; SOCOM 1's CAiState::Signal, moved onto the
 * stack): entering (0) resets it to its own kind, sends it the reset (4) and starts its stack's
 * clock; leaving (1) empties its own stack; then, for the controller's own stack, the SEAL (and,
 * squad mode 2, the squad's leader) marked, the trigger let go and the per-signal goals dropped;
 * the state's OnSignal last */
void CAiState::Signal(CAiStateStack *stack, CAiState *state, SIGNAL sig, CSealCtrlAi *ai)
{
    CEntity *seal = ai ? ai->m_entity : 0;
    bool own = &((CAiCtrlStack *)ai)->m_stack == stack;

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
        func_005F7920(&state->m_stack, ai);
        func_005F6190(state, ai);
        break;
    case SIGNAL_2:
        func_005F6350(state, ai, sig);
        func_0056D890(ai, 0);
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
            CEntity *leader = func_005EDBB0(((CAiCtrl10C *)ai)->m_10c, 1, 0);

            if (leader)
                ((CSealSignalBits *)leader)->m_signalled = true;
        }
        ai->SetTrigger(false);
        ai->SetTrigger(false);
        func_005631B0(ai, 2, 1, 0.0f);
        switch (sig) {
        case SIGNAL_4:
            break;
        case SIGNAL_0:
            ai->SetAiMode((AI_MODE)1);
            func_00570F90(ai, 1.0f);
            func_0056D890(ai, 0);
            ai->ClearGoal(GOAL_NONE);
            break;
        case SIGNAL_1:
            ai->ClearGoal(GOAL_NONE);
            func_0056D890(ai, 0);
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
 * OPEN, 10 instructions, one register: before the erase the target takes &end_.next_ (this + 8)
 * into s3 and reads the new front through it after the call; here the front is reloaded from
 * 8(this) and a nop follows. Tried: the const GetState, erase(begin()) for pop_front, the deleted
 * front in a local, the ternary on front() -- all the same 10; round 5: the erase's iterator built
 * from a `node_base *const &` to the link (znode.h's PtrListFirst shape), GetState reading the
 * link through a reference -- the same 10 (the reference form likely belongs in include/list's
 * begin(), the other lane's) */
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
 * remove_copy_if, 0x5F8AF0 and 0x5F8C40, inline it -- the name ours).
 * OPEN, both instances (include/algorithm's, the other lane's this round): the target's inlined
 * find_if reads the functor from remove_if's parameter slot at every step (`lbu a1,104(sp)`, the
 * functor's kind first in the bne), where include/algorithm's find_if takes its copy -- a byte
 * copied to a fresh slot and held in s0 -- so remove_if is 0x124 against 0x110; MSL's find_if
 * passing the predicate on without a copy (or by reference) should close both */
struct CAiStateDeleteKind {
    CAiStateDeleteKind(AI_STATE state) : m_state(state) {}
    bool operator()(CAiState *s) const
    {
        if (s->m_state == m_state) {
            delete s;
            return true;
        }
        return false;
    }

    AI_STATE m_state;
};

/* every state of a kind deleted and taken off; whether there was one (OVL2 0x5F81E0) */
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

/* the state of a kind, or 0 (OVL2 0x5F8270; SOCOM 1's CSealCtrlAi::FindAiState) */
CAiState *CAiStateStack::Find(AI_STATE state) const
{
    const_iterator i;

    for (i = begin(); i != end(); ++i)
        if ((*i)->m_state == state)
            return *i;
    return 0;
}

/* a new state of a kind pushed: the current one told it is covered (2), the new one entered (0)
 * and, when it has a script response and the covered state is the script, the response run on
 * it (the new state's bit 3 then set) (OVL2 0x5F7EE0; SOCOM 1's CSealCtrlAi::PushAiState) */
bool CAiStateStack::Push(CSealCtrlAi *ai, AI_STATE state, bool b)
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

/* a new state of a kind in the current one's place: the current one told it is left (1) and
 * deleted, the new one entered (0) (OVL2 0x5F7D80; SOCOM 1's CSealCtrlAi::SetAiState) */
bool CAiStateStack::Set(CSealCtrlAi *ai, AI_STATE state, bool b)
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

/* popped down to a state, it too when `inclusive`; whether it was found (OVL2 0x5F7A40) */
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

/* the current state popped if it is of a kind; the kind on top after (OVL2 0x5F7990; SOCOM 1's
 * CSealCtrlAi::PopAiState(AI_STATE)).
 * OPEN, 0xA4 against 0xAC: the target materialises Is()'s `!(size() == 0) && GetState()->m_state
 * == state` whole -- xor/sltiu/xori with no andi, the second operand's sltiu, a `b` to the join
 * past the false arm's `move v0,zero`, then one beqz -- where MW narrows the first operand (andi)
 * and branches on the second. Tried: Is() returning through a bool local, `size() != 0`, the
 * ternary, an inline Empty(), Is() as a plain auto-inlined member, the caller testing `!= false`
 * / `== true` / a bool local / a ternary / a result local (all 0xA4 or worse) */
AI_STATE CAiStateStack::Pop(CSealCtrlAi *ai, AI_STATE state)
{
    if (Is(state))
        return Pop(ai);
    return GetStateEnum();
}

/* the stack's constructor (OVL2 0x5F8520): out of line, as its destructor is */
CAiStateStack::CAiStateStack() : m_c(-1), m_10(0), m_14(0)
{
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

/* every `period` seconds of the controller's state clock, to a tenth (OVL2 0x5F6270).
 * OPEN, 0x48 against 0x44: the target builds 10.0 before it loads the clock and multiplies the
 * period first, with no load-delay nop; here the clock's load comes first. Tried: the inline
 * StateTimer, the products in either order, a shared constant local, the divisor in a local. */
extern "C" bool func_005F6270(CAiState *s, float period, CSealCtrlAi *ai)
{
    return (int)(10.0f * ((CAiCtrlClock *)ai)->m_stateTimer) % (int)(10.0f * period) == 0;
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
