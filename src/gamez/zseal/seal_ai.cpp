/* seal_ai.cpp -- the AI's controller, CSealCtrlAi, and its states (SOCOM 1's unit,
 * __sinit_seal_ai.cpp; SOCOM 1's order: the states' small virtuals and CAiSRescued first, the
 * controller's constructor and the template tail -- ai::_max, ai::_min, _close_enough<i> -- last;
 * SOCOM II's tail ends with the animate state's list instances, 0x575CF0-0x576117: the timed calls'
 * range erase, single erase and insert, the spans' range erase and insert -- used by seal_ai.cpp's
 * functions alone, and seal_anim.cpp's first row, 0x576120, is SOCOM 1's ~CSealAnim's job).
 * OVL2 0x5552C0-0x576117. Defined in reverse of their image order (-inline deferred). */

#include "zseal.h"
#include "../../ai/ai.h"
#include "../zsnd/zsnd.h"
#include "../zanim/zanim.h"
#include "../zweapon/zweapon.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

/* The unit inlines to depth 8, not the flags' 5 (a pragma acts through its state at the unit's end):
 * CSealUnit::GetTop's out-of-line copy (0x561D20, after 0x561B90) inlines itself eight times, and a
 * recursive inline's self-copies are the depth -- 7 gives seven, 9 nine, and no shape of the getter
 * gives eight at 5. Every function of the unit matched at 5 holds at 8; so does every unit including
 * zseal.h, where depth 8 also matches veh_vehicle.cpp's copy of zdb::CNode::GetBBox (0x262550, the
 * other self-recursive inline, open at 5) -- the original's level may be 8 throughout, with MSL's
 * real depths. */
#pragma inline_depth(8)

/* the states this unit defines (SOCOM 1's; only what their destructors need -- none of them has
 * a member with a destructor of its own, so each destructor is the base's, inlined) */
class CAiSRescued : public CAiState {
public:
    virtual void OnSignal(SIGNAL sig, CSealCtrlAi *ctrl);
    virtual ~CAiSRescued();

    unsigned int m_58;   // 0x58 (SOCOM 1's 0x28)
    unsigned int m_5c;   // 0x5c
    unsigned int m_60;   // 0x60
};

/* the base's phase and its timer (CAiState's words at 0x34 and 0x38: ai_states.h names the first) */
struct CAiStatePhase {
    unsigned char _0[0x34];
    unsigned int m_phase;   // 0x34
    unsigned int m_phase_timer;   // 0x38
};

/* the phase set, its timer restarted when it changes (an inline both rescued and action states
 * show) */
static inline void SetPhase(CAiState *s, unsigned int phase)
{
    CAiStatePhase *p = (CAiStatePhase *)s;

    if (p->m_phase != phase) {
        p->m_phase_timer = 0;
        p->m_phase = phase;
    }
}

class CAiSPursue : public CAiState {
public:
    virtual void OnSignal(SIGNAL sig, CSealCtrlAi *ctrl);
    /* slots 0x14, 0x44 and 0x48 (new in SOCOM II; the signatures ai_states.h's) */
    virtual int Virtual14(CSealCtrlAi *ai, CAiState *sub);
    virtual bool Virtual44(AI_STATE state) { return state == m_state; }
    virtual bool Virtual48(int x, CAiState *other);
    virtual ~CAiSPursue();

    float m_timer;   // 0x58 (SOCOM 1's 0x28)
    signed char m_5c;   // 0x5c (the name ours)
};

class CAiSAnimate;

/* the animate state's timed calls (new in SOCOM II; the names ours): at a time of the anim, a call
 * told the state, the controller and a word -- Tick erases each once it answers true -- and over a
 * span of it, the same call every tick inside the span (0x55AC20 pushes 0x55C740, 0x55C690, 0x55C480
 * and 0x55C5E0 with the state itself as the word) */
typedef bool (*ANIM_CALL)(CAiSAnimate *state, CSealCtrlAi *ai, unsigned int data);

struct ANIM_TIMED_CALL {
    float m_time;   // 0x0
    ANIM_CALL m_func;   // 0x4
    unsigned int m_data;   // 0x8
};

struct ANIM_SPAN_CALL {
    float m_start;   // 0x0
    float m_end;   // 0x4
    ANIM_CALL m_func;   // 0x8
    unsigned int m_data;   // 0xc
};

/* the two lists (classes over std::list: their implicit destructors are four this-tested levels) */
class CAnimTimedCallList : public std::list<ANIM_TIMED_CALL> {
};

class CAnimSpanCallList : public std::list<ANIM_SPAN_CALL> {
};

class CAiSAnimate : public CAiState {
public:
    virtual void OnSignal(SIGNAL sig, CSealCtrlAi *ctrl);
    /* (0x5759A0: the two lists destroyed, each through its own range erase -- 0x575CF0 and
     * 0x575F40, the unit's template tail) */
    virtual ~CAiSAnimate();

    unsigned char _58[0x5a - 0x58];
    unsigned char m_5a;   // 0x5a (SOCOM 1's 0x28)
    float m_5c;   // 0x5c (SOCOM 1's 0x30)
    unsigned int m_60;   // 0x60 (SOCOM 1's 0x8)
    CAnimTimedCallList m_timed;   // 0x64
    CAnimSpanCallList m_spans;   // 0x70
};

class CAiSGrenade : public CAiState {
public:
    virtual void OnSignal(SIGNAL sig, CSealCtrlAi *ctrl);
    virtual int Virtual14(CSealCtrlAi *ai, CAiState *sub);
    virtual ~CAiSGrenade();

    /* (the names ours: what OnSignal's reset clears) */
    unsigned int m_58;   // 0x58
    unsigned int m_5c;   // 0x5c
    unsigned char m_60;   // 0x60
    unsigned char _61[0x6c - 0x61];
    unsigned char m_6c;   // 0x6c
    unsigned char _6d[0x7c - 0x6d];
    CAiMapLoc m_7c;   // 0x7c
    CAiMapLoc m_80;   // 0x80
    unsigned char _84[0xa0 - 0x84];
    /* the throw's velocity in the SEAL's frame (0x55C480 turns it by the body's matrix; the name
     * ours) */
    CPnt3D m_vel;   // 0xa0
};

class CAiSDeploy : public CAiState {
public:
    virtual ~CAiSDeploy();
};

/* (OVL2 0x543480, seal.cpp's: the hostage's compass anchor deleted, as SOCOM 1's OnSignal did here
 * with CompassAnchor_Hostage) */
extern "C" void func_00543480(CZSealBody *seal);

/* the rescued state told: signal 4 clears its three words; signal 0 drops the hostage's anchor
 * and, online, moves to phase 2 with the SEAL made invincible (OVL2 0x5594B0; SOCOM 1's) */
void CAiSRescued::OnSignal(SIGNAL sig, CSealCtrlAi *ai)
{
    CZSealBody *seal = ai ? (CZSealBody *)ai->m_entity : 0;

    switch (sig) {
    case SIGNAL_4:
        m_58 = 0;
        m_5c = 0;
        m_60 = 0;
        break;
    case SIGNAL_0:
        func_00543480(seal);
        if (theNetwork.m_9) {
            SetPhase(this, 2);
            seal->m_invincible = true;
        }
        break;
    case SIGNAL_1:
        break;
    case SIGNAL_2:
        break;
    case SIGNAL_3:
        break;
    }
}

/* the entities of kind 4 counted at a mission's start (OVL2 bss 0x66AFE8) */
extern "C" int data_0066AFE8;
/* a range the AI's mode 1 starts with (OVL2 data 0x64FF88, read as a variable) */
extern "C" float data_0064FF88;
CZSealBody *ftsGetPlayer();

/* an entity of kind 4, or none (the inline cast the count shows) */
static inline CEntity *AsKind4(CEntity *e)
{
    return e && e->m_type == (CEntity::TYPE)4 ? e : 0;
}

/* the node's own anims bound: <node>_start and <node>_ambient (OVL2 0x562AD0; SOCOM 1's bound
 * more of them) */
bool CSealCtrlAi::SetupAnimations()
{
    char name[1024];
    CEntity *e = m_entity;

    sprintf(name, "%s_%s", e->m_node->m_name, "start");
    m_anims[0] = ZAnim.GetAnim(name, 0);
    sprintf(name, "%s_%s", e->m_node->m_name, "ambient");
    m_anims[2] = ZAnim.GetAnim(name, 0);
    return true;
}

/* the mission begun: the animations bound, the two standing tasks begun (the first unless a Navy
 * SEAL), and in mode 1 the player taken as the one to follow, with the entities of kind 4
 * counted (OVL2 0x573BD0; SOCOM 1's had the two tasks only) */
void CSealCtrlAi::OnMissionStart()
{
    SetupAnimations();
    if (!IsNavySeal())
        ActivateEx((SEAL_TASK)1, true, true, GetSelect(), 3, m_entity->GetNode(), 0);
    ActivateEx((SEAL_TASK)3, false, true, GetSelect(), 3, m_entity->GetNode(), 0);
    if (m_26c == 1) {
        m_664 = ftsGetPlayer();
        m_660 = 1;
        m_674 = data_0064FF88;
        m_678 = m_674 * m_674;
        for (CEntityList::iterator e = CEntity::m_list.end(), i = CEntity::m_list.begin(); i != e; ++i) {
            if (AsKind4(*i))
                data_0066AFE8++;
        }
    }
}

extern "C" {
float tanf(float x);
}

/* a spray of fire timed: refused in mode 2, with nothing in hand, or when the item cannot fire fully
 * automatic; the weapon raised, the aim straight ahead through SetAimDirR (seal_weapon.cpp's, OVL2
 * 0x5DF8B0, as SOCOM 1's SetSprayTimer calls it) (OVL2 0x565AE0; SOCOM 1's SetSprayTimer, its mode and
 * null tests new) */
void CSealCtrlAi::SetSprayTimer(float t)
{
    CZSealBody *seal = (CZSealBody *)m_entity;
    CZFTSWeapon *item = seal->m_kit.GetCurItem();

    if (m_26e == 2 || !item || item->m_maxfiremode < 3)
        return;
    seal->SetWeaponState((WEAPON_STATE)1);
    seal->SetAimDirR(&CPnt3D::unit_nz, false);
    m_spraytimer = t;
}

extern "C" {
/* the kit's fire mode set (CZKit::SetCurFireMode, the kit lane's; OVL2 0x5C37B0 -- by its address:
 * FIREMODE is zweapon.h's, which zseal.h's kit stand-in cannot see) and the target's word at 0x44
 * copied to 0x40 when asked (OVL2 0x60A2D0, the AI states' range) */
bool func_005C37B0(CZKit *kit, FIREMODE mode);
void func_0060A2D0(void *target, bool on);
}

/* the spray kept up while its timer runs: none for a Navy SEAL; with the item in hand empty the timer
 * stops, else it runs down by the tick, the item fires fully automatic with the trigger held, and the
 * timer stops at 0; whether it fired (OVL2 0x5659E0, new in SOCOM II between AttackTick and SetSprayTimer:
 * AttackTick and PreTick call it; the name unknown). The ammunition's negation is a bool local: the
 * target's sltu, xori and andi before the branch (tested in the `if`, `!` or `== false` branch on the
 * call's v0) */
extern "C" bool func_005659E0(CSealCtrlAi *ai, float dT)
{
    bool r = false;

    if (!ai->IsNavySeal() && ai->m_spraytimer > 0.0f) {
        CZKit *kit = &((CZSealBody *)ai->m_entity)->m_kit;

        bool empty = !kit->CurItemHasAmmo();

        if (empty)
            ai->m_spraytimer = 0.0f;
        else {
            ai->m_spraytimer -= dT;
            ai->m_3d0 = 0;
            func_0060A2D0(ai->m_67c, true);
            func_005C37B0(kit, AUTOFIRE);
            ai->SetTrigger(true, 0.0f);
            if (ai->m_spraytimer <= 0.0f)
                ai->m_spraytimer = 0.0f;
            r = true;
        }
    }
    return r;
}

/* the AI's target (SOCOM 1's CAiTarget at 0x67C, ai_target.cpp's; only its flags: SEAL_AI_PARAMS's
 * m_6a8) -- whether it has one (OVL2 0x60A850, the AI states' range) */
struct CAiTargetFlags {
    unsigned char _0[0x2c];
    unsigned int m_flags;   // 0x2c
};

extern "C" bool func_0060A850(void *target);

/* whether the current state is of kind 2 with flag 0x20 (0x559000's shape) */
static inline bool IsKind2Flag20(CAiState *s)
{
    bool r = false;

    AI_STATE st = s->m_state;

    if (st == (AI_STATE)2 && (s->m_flags & 0x20))
        r = true;
    return r;
}

static inline bool InKind2Flag20(const CAiStateStack &st)
{
    CAiState *s;

    if (st.size() == 0)
        s = 0;
    else
        s = st.GetState();

    return s ? IsKind2Flag20(s) : false;
}

/* whether a state of kind 8 on the stack has flag 0x100 (the stack's kinds tested first) */
static inline bool HasKind8Flag100(const CAiStateStack &st)
{
    bool r = false;

    if (st.m_10 & (1 << 8)) {
        CAiState *p = st.Find((AI_STATE)8);

        r = p && (p->m_flags & 0x100);
    }
    return r;
}

/* whether the AI holds its fire at its target: none without one; with one, while the current state is
 * of kind 2 with flag 0x20, a kind 8 state on the stack has flag 0x100, or a kind 10 is on it -- else
 * by the target's flag 0x800 (OVL2 0x5658D0, new in SOCOM II: AttackTick asks it; the name unknown).
 * The tests take the stack by reference: handed the controller, MW keeps the stack's address in a saved
 * register for both calls, where the target recomputes it for each */
extern "C" bool func_005658D0(CSealCtrlAi *ai)
{
    bool r = false;

    if (func_0060A850(ai->m_67c)) {
        CAiTargetFlags *t = (CAiTargetFlags *)ai->m_67c;

        if (InKind2Flag20(ai->m_stateStack) || HasKind8Flag100(ai->m_stateStack) || (ai->m_stateStack.m_10 & (1 << 10)))
            return true;
        r = (t->m_flags & 0x800) != 0;
    }
    return r;
}

/* an item given the SEAL with its rounds: added at the kit's end unless it holds one, the rounds set
 * -- none for 0, else topped up to the count; whether it holds the item (OVL2 0x5657C0, new in SOCOM II:
 * CAiSDeploy::Tick, 0x565110 and 0x573130 call it; the name unknown). The ammunition and magazine
 * queries' answers are never read (their calls stay); the kit's pointer is declared after the other
 * locals, numbered last as the target's s2 (declared first it takes s5 and shifts the rest) */
extern "C" bool func_005657C0(CSealCtrlAi *ai, EQUIP_ITEM item, int rounds)
{
    int index;
    bool held;
    bool full;
    bool ok;
    CZKit *kit = &((CZSealBody *)ai->m_entity)->m_kit;

    index = kit->GetItemIndex(item);
    held = index != -1;
    full = held && kit->ItemHasAmmo(index);
    ok = true;

    if (!held) {
        index = kit->GetNumItems();
        ok = kit->SetItem(index, item);
    }
    if (ok) {
        int mags = kit->GetNumMags(index);

        if (rounds == 0)
            kit->SetItemAmmo(index, 0);
        else if (kit->GetAmmoTotal(index) < rounds)
            kit->SetItemAmmo(index, 0, rounds, 1);
    }
    return ok;
}

/* the aim's error cone set, as its tangent (OVL2 0x574BA0; SOCOM 1's, after PlayAiSound in both
 * builds -- CCharacterType::Open calls it) */
void CSealCtrlAi::SetAiAccuracyAngle(float angle)
{
    aiAccuracyAngle = tanf(angle);
}

/* a loop started unless it plays already (OVL2 0x574B30; SOCOM 1's) */
void CSealCtrlAi::PlayAiSound(AI_SOUND sound)
{
    CSndInstance *&inst = aiSoundInstance[sound];

    if (!inst) {
        CSnd *snd = aiSoundHandle[sound];

        if (snd)
            inst = snd->PlaySound(0, 255);
    }
}

/* a loop stopped (OVL2 0x574AE0; SOCOM 1's) */
void CSealCtrlAi::StopAiSound(AI_SOUND sound)
{
    CSndInstance *&inst = aiSoundInstance[sound];

    if (inst) {
        inst->Stop();
        inst = 0;
    }
}

/* the box put in order, axis by axis (OVL2 0x56AC70; SOCOM 1's, the same 0x68 bytes) */
void CBBox::Normalize()
{
    if (m_min.x > m_max.x) {
        float t = m_min.x;

        m_min.x = m_max.x;
        m_max.x = t;
    }
    if (m_min.y > m_max.y) {
        float t = m_min.y;

        m_min.y = m_max.y;
        m_max.y = t;
    }
    if (m_min.z > m_max.z) {
        float t = m_min.z;

        m_min.z = m_max.z;
        m_max.z = t;
    }
}

/* (OVL2 data: 42, read as a variable) */
extern "C" float data_0064FE48;

/* a range and its goal set together unless the range stands at 42 already (an inline: 0x570D90
 * expands it twice) */
static inline void SetRange6F4(CSealCtrlAi *ai, float v)
{
    if (data_0064FE48 != ai->m_6f4) {
        ai->m_6f8 = v;
        ai->m_6f4 = v;
    }
}

/* the range made 42 when asked to and the SEAL may shoot, else the fallback range (OVL2 0x570D90;
 * new in SOCOM II, its name unknown -- CAiSSurrender::Tick calls it) */
static inline bool NoShoot(const CEntity *e)
{
    return e->m_noshoot ? true : false;
}

extern "C" void func_00570D90(CSealCtrlAi *ai, bool on)
{
    CEntity *e = ai->m_entity;

    if (on && !NoShoot(e))
        SetRange6F4(ai, data_0064FE48);
    else
        SetRange6F4(ai, ai->m_d8);
}

/* the valve of a name, or none (OVL2 0x574730; SOCOM 1's, without its fourth valve and the alias) */
SEAL_VALVE CSealCtrlAi::get_valve(const char *name)
{
    if (name) {
        for (int i = 0; i < 3; i++)
            if (!strcmp(name, get_valve((SEAL_VALVE)i)))
                return (SEAL_VALVE)i;
    }
    return SEAL_VALVE_NONE;
}

/* the controller unmade: its stack's states deleted (CAiStateStack::Clear, out of line), then the
 * bases -- SEAL_AI_PARAMS's destructor inlined behind the conversion's null test of `this + 0x250`,
 * its members destroyed in reverse order, then CSealCtrl's called with 0 (OVL2 0x573E60; SOCOM 1's
 * popped and deleted the states in place) */
CSealCtrlAi::~CSealCtrlAi()
{
    m_stateStack.Clear();
}

/* a valve set, its callbacks told whether it went on or off (CValve::SetValue, inlined in both
 * builds; the name zanim_menu.h's) */
static inline void SetValveValue(CValve *valve, int value)
{
    VALVE_STATE state = VALVE_SAME;

    if (valve->m_value != value)
        state = value == 0 ? VALVE_OFF : VALVE_ON;
    valve->m_value = value;
    if (state != VALVE_SAME)
        valve->MakeCallbacks(state);
}

/* the valves' names built in (OVL2 bss 0x66B110) */
extern "C" char data_0066B110[];

/* the AI made ready: a valve per kind, named for the body's node, the alive valve opened, the mode
 * 1 (OVL2 0x573D20; SOCOM 1's, with three valves) */
void CSealCtrlAi::Initalize()
{
    CEntity *e = m_entity;

    for (int i = 0; i < 3; i++) {
        sprintf(data_0066B110, "%s_%s", e->m_node->GetName(), get_valve((SEAL_VALVE)i));
        m_valve[i] = CValve::Create(data_0066B110, 0, (VALVE_TYPE)2);
    }
    CValve *alive = m_valve[SEAL_VALVE_ALIVE];

    if (alive)
        SetValveValue(alive, 1);
    m_3f4 = (AI_MODE)1;
}

/* how far an event's kind carries (SOCOM 1's static CAiEvent::GetEventRange, ai.h's class lacks it:
 * OVL2 0x50F4C0 by its address) */
extern "C" float func_0050F4C0(CAiEvent::EVENT event);
/* the voice's reach scale (OVL2 data 0x650380, read as a variable) */
extern "C" float data_00650380;

/* the voice's AI event made where the SEAL stands, unless it has none; the event returned (OVL2
 * 0x55FEE0; PlayAiSound calls it) */
extern "C" int func_0055FEE0(CSealCtrl *ctrl, CAiEvent::EVENT event)
{
    int r = 0;

    if (event != CAiEvent::AIE_NONE) {
        float range = func_0050F4C0(event);

        r = ctrl->CreateAiEvent(CAiEvent::AIE_VOICE, data_00650380, range, false, &ctrl->m_entity->m_posW, event);
    }
    return r;
}

/* the states' destructors: their own table stored, then CAiState's inlined -- its table, and the
 * stack at 8 destroyed out of line (OVL2 0x575B50, 0x575AB0, 0x5758D0, 0x5552C0). Every byte
 * matches; the one call resolves once the AI-states lane's 0x5F8490 is named
 * __dt__13CAiStateStackFv (verified with that name aliased: build/scratch/seal/adiff.py). */
CAiSDeploy::~CAiSDeploy()
{
}

CAiSGrenade::~CAiSGrenade()
{
}

/* (OVL2 0x5759A0: its two lists' implicit destructors inlined whole, each erase called out of line --
 * the unit's template tail's 0x575F40 and 0x575CF0 -- then the base's) */
CAiSAnimate::~CAiSAnimate()
{
}

CAiSPursue::~CAiSPursue()
{
}

CAiSRescued::~CAiSRescued()
{
}

extern "C" {
/* the table of a stance's figures, a row of eight per setting (OVL2 data 0x64FE60), and the row
 * in use (OVL2 bss 0x66AFD8) */
extern float data_0064FE60[][8];
extern int data_0066AFD8;
/* (OVL2 0x555420, not written yet: the AI's state brought up to date from two of its blocks) */
void func_00555420(CSealCtrlAi *ai, void *a, void *b);
}

extern "C" {
/* the stack's current state (OVL2 0x5F8330, the AI-states lane's: CAiStateStack's) */
CAiState *func_005F8330(void *stack);
}

/* a state of kind 2 with flag 0x80 (the answer through a local, as the target's daddiu shows) */
static inline bool IsKind2Flag80(CAiState *s)
{
    bool r = false;

    AI_STATE st = s->m_state;

    if (st == (AI_STATE)2 && (s->m_flags & 0x80))
        r = true;
    return r;
}

/* whether the AI's current state is of kind 2 with flag 0x80 (OVL2 0x559000). The stack's test is the
 * target's as an if/else (the zero arm first: a ternary or an if-assign lays the call first); the
 * inline's kind read into a local keeps the flag test a branch (compared straight, MW if-converts it
 * with xori/sltiu/sltu), and the return's ternary gives the result's `andi 0xff` */
extern "C" bool func_00559000(CSealCtrlAi *ai)
{
    CAiState *s;

    if (ai->m_stateStack.size() == 0)
        s = 0;
    else
        s = func_005F8330(&ai->m_stateStack);

    return s ? IsKind2Flag80(s) : false;
}

/* the SEAL teleported to a cell's centre, facing a direction -- or as it faces for none (8): whether the
 * cell has a centre (OVL2 0x562A10, new in SOCOM II: 0x5DB8F0 calls it twice; the name unknown) */
extern "C" bool func_00562A10(CSealCtrlAi *ai, const CAiMapLoc &loc, CAiMap::DIR dir)
{
    bool r = false;
    CEntity *e = ai->m_entity;
    CPnt3D center;

    if (AiMaps.GetCellCenter(loc, &center)) {
        CQuat q = e->m_quat;

        if (dir != CAiMap::DIR_COUNT)
            CAiMap::GetDirQuat(dir, q, true);
        e->TeleportTo(&center, &q);
        r = true;
    }
    return r;
}

/* the state at 0x4EC: 4 with the word at 0x508 set (an inline: the test materialised, the word
 * as MW's `!(x == 0)`) */
static inline bool IsState4Held(CSealCtrlAi *ai)
{
    return !(ai->m_path.m_9c == 0) && ai->m_path.m_state == 4;
}

/* free: in none of states 1-3 nor a held 4 (OVL2 0x5629B0). States 1-3 are one unsigned range
 * test written out, as the target's `(t - 1) < 3`: three `==` joined by `||` fold only two */
extern "C" bool func_005629B0(CSealCtrlAi *ai)
{
    if (IsState4Held(ai) || (unsigned int)(ai->m_path.m_state - 1) < 3)
        return false;
    return true;
}

/* the byte at 0x4ED, or 0 in states 1-3 or a held 4 (OVL2 0x562960; the range and the inline
 * as 0x5629B0's) */
extern "C" signed char func_00562960(CSealCtrlAi *ai)
{
    if ((unsigned int)(ai->m_path.m_state - 1) < 3)
        return 0;
    if (IsState4Held(ai))
        return 0;
    return ai->m_path.m_status;
}

extern "C" {
/* theNetwork's online byte (OVL1 bss 0x45A0C1, theNetwork + 1: SOCOM 1's network flag at 9) */
extern unsigned char data_0045A0C1;
}

/* the planner a new route takes: a Navy SEAL in state 8 with flag 1 or 0x100 the stealthy one, else the
 * default; an enemy the basic one, or -- offline and restrained -- the restrained one, its bit 17 cleared
 * (OVL2 0x5627E0: SOCOM 1's GetDefaultRouteType, between GetDefaultRouteBias and ClearRoute in both
 * builds; SOCOM 1's tested the state's kind alone and had no restrained case). The body is taken before
 * the first test (the target loads it there); the bit cleared before the kind is set (the other order
 * swaps the insert's registers) */
CAiPath::PATH_TYPE CSealCtrlAi::GetDefaultRouteType()
{
    CZSealBody *seal = (CZSealBody *)m_entity;
    CAiPath::PATH_TYPE t = CAiPath::PATH_DEFAULT;

    if (IsNavySeal()) {
        if (AiStateIs((AI_STATE)8)) {
            CAiState *s = GetAiState();

            if (s->m_flags & 1 || s->m_flags & 0x100)
                t = CAiPath::PATH_STEALTH;
        }
    } else if (!data_0045A0C1 && seal->m_restrained) {
        m_bit17 = false;
        t = CAiPath::PATH_RESTRAINED;
    } else
        t = CAiPath::PATH_BASIC;
    return t;
}

extern "C" {
/* the maps' path plan registered from a cell through a list of cells into a path (SOCOM 1's
 * CAiMaps::RegisterPathPlan by its call here; OVL2 0x52C5C0, the AI core's) and a path's planner set,
 * whether it took (SOCOM 1's CAiPath::SetPathType; OVL2 0x524CD0, the AI core's) */
bool func_0052C5C0(CAiMaps *maps, const CAiMapLoc &start, const CAiMapLoc *locs, int count,
                   CAiPathBytes *path);
bool func_00524CD0(CAiPathBytes *path, CAiPath::PATH_TYPE type);
}

/* whether the current state is of kind 8 with flag 1 (0x559000's shape, kind 2 and flag 0x80 there) */
static inline bool IsKind8Flag1(CAiState *s)
{
    bool r = false;

    AI_STATE st = s->m_state;

    if (st == (AI_STATE)8 && (s->m_flags & 1))
        r = true;
    return r;
}

static inline bool InKind8Flag1(const CAiStateStack &st)
{
    CAiState *s;

    if (st.size() == 0)
        s = 0;
    else
        s = st.GetState();

    return s ? IsKind8Flag1(s) : false;
}

/* the bias a new route's planner takes: 1, whatever the state (SOCOM 1's GetDefaultRouteBias, two
 * statics initialised and unread; inline in SOCOM II -- both SetRoutes test the Navy SEAL's states and
 * keep only the calls). The tests' answer is a bool never read: returned straight, the 1 folds into the
 * caller's store (returned from each arm, MW holds it in $f20 across the planner's call); the second
 * test's kind is not in the image, its compare dead */
inline float CSealCtrlAi::GetDefaultRouteBias()
{
    if (IsNavySeal()) {
        bool b = InKind8Flag1(m_stateStack) || AiStateIs((AI_STATE)8);
    }
    return 1.0f;
}

/* a route planned from the SEAL's last good cell through a list of cells: once registered the AI moves
 * (mode 3), the planner set with its bias, the search limit and the tolerance cleared (OVL2 0x562200:
 * SOCOM 1's SetRoute(const CAiMapLoc *, int), its failure's debug prints gone) */
bool CSealCtrlAi::SetRoute(const CAiMapLoc *locs, int count)
{
    CAiMapLoc start(m_entity->m_mapLocValid);
    bool ok = func_0052C5C0(&AiMaps, start, locs, count, &m_path);

    if (ok) {
        SetAiMode((AI_MODE)3);

        float bias = GetDefaultRouteBias();

        if (func_00524CD0(&m_path, GetDefaultRouteType()))
            m_path.m_type_bias = bias;
        m_path.m_search_limit = 0;
        m_pathTolerance = 0.0f;
    }
    return ok;
}

/* the five bands of an alertness (OVL2 data 0x65C700: 0.05, 0.4, 0.6, 0.8, 1) */
extern "C" float data_0065C700[5];

/* the band a value lies in, the last for any above (OVL2 0x560AC0) */
static inline bool InBand(float x, int band)
{
    return x < data_0065C700[band];
}

extern "C" unsigned char func_00560AC0(float x)
{
    for (int i = 0; i <= 4; i++)
        if (InBand(x, i))
            return i;
    return 4;
}

/* the band of the float at 0xB4 (OVL2 0x560A60; the player map's blip and the AI states ask) */
struct CAlertness {
    unsigned char _0[0xb4];
    float m_alert;   // 0xb4
};

extern "C" unsigned char func_00560A60(CAlertness *a)
{
    for (int i = 0; i <= 4; i++)
        if (InBand(a->m_alert, i))
            return i;
    return 4;
}

extern "C" {
/* (OVL2 0x571150, not written yet: the sound made, or not, by the stance -- 11 for the SEAL's own) */
bool func_00571150(CSealCtrlAi *ai, short sound, unsigned char stance);
/* (OVL2 data: how long a sound holds before another) */
extern float data_0064FE50;
/* (OVL1 data: the game's clock, as this unit reads it) */
extern float data_004365C0;
}

/* an AI sound made if its priority reaches the controller's alertness band (0x560A60 inlined); once
 * made, remembered with when the next may come and the band (OVL2 0x571010, new in SOCOM II: the
 * event and shot-at states call it, with a priority 2 and a stance 11; the name unknown).
 * OPEN, 0xE0 against 0xEC: the target narrows the priority's low byte into a1 at entry (`andi
 * a1,a2,0xff`), keeps the whole word in s0 for the store `(pri & ~0xFF) | band`, takes the band
 * from the inline's v0 into s3 (`andi s3,v0,0xff`) and branches on `slt at,a1,s3` -- the word in a
 * register throughout; the struct here is homed on the stack (sw a2, lbu). Tried: the priority as an
 * unsigned int with the masks written out (0xF4: the band in s0, the low byte taken late), its low
 * byte in a local first, the band an int, the made flag declared first, the test as `band <=
 * level`, the struct's level a plain byte (0x100). */
extern "C" bool func_00571010(CSealCtrlAi *ai, short sound, AI_SOUND_PRIORITY pri, unsigned char stance)
{
    unsigned char band = func_00560A60((CAlertness *)ai);
    bool made = false;

    if (pri.m_level >= band) {
        made = func_00571150(ai, sound, stance);
        if (made) {
            ai->m_sound = sound;
            ai->m_sound_time = data_0064FE50 + data_004365C0;
            ai->m_sound_priority = pri;
            ai->m_sound_priority.m_level = band;
        }
    }
    return made;
}

/* a figure of the current row (OVL2 0x5750D0; before sealai::get_stance) */
extern "C" float func_005750D0(unsigned char i)
{
    return data_0064FE60[data_0066AFD8][i];
}

/* whether an entity may be taken as a target: any in mode 4, else one of no team of ours (OVL2
 * 0x572D30) */
extern "C" bool func_00572D30(CSealCtrlAi *ai, CEntity *e)
{
    CEntity *me = ai->m_entity;

    return ai->m_26c == 4 || (e && !(e->m_team & me->m_team));
}

/* a valve, or none past the three (OVL2 0x5621C0; SOCOM 1's, which had four) */
CValve *CSealCtrlAi::GetValve(SEAL_VALVE v) const
{
    if (v < 3)
        return m_valve[v];
    return 0;
}

/* whether the SEAL may be taken hostage (state 23 pushed with no flags): for kind 3 a restrainable one,
 * or online one of team bit 17; otherwise one of team bit 17 already holding the state or free to take
 * it (OVL2 0x555B80, new in SOCOM II: 0x5784A0 and 0x5A90B0 ask it; the name unknown). A switch (the
 * case's block first behind its beq, a `b` to the default) assigning one bool, masked once at the join:
 * returned from each case, the andi goes */
extern "C" bool func_00555B80(CSealCtrlAi *ai, int kind)
{
    CZSealBody *seal = (CZSealBody *)ai->m_entity;

    bool r;

    switch (kind) {
    case 3:
        r = (seal->m_restrainable || (data_0045A0C1 && (seal->m_team & 0x20000))) && ai->CanPushAiState(AI_STATE_HOSTAGE, 0);
        break;
    default:
        r = (seal->m_team & 0x20000) && ((ai->m_stateStack.m_10 & (1 << AI_STATE_HOSTAGE)) || ai->CanPushAiState(AI_STATE_HOSTAGE, 0));
        break;
    }
    return r;
}

/* the AI's state brought up to date, the route's state returned (OVL2 0x5558A0; the AI states'
 * OnSignal call it) */
extern "C" AI_ROUTE func_005558A0(CSealCtrlAi *ai)
{
    func_00555420(ai, &ai->m_path, &ai->m_route.m_path);
    return ai->m_route.m_state;
}

/* (OVL2 0x527310, the AI-core lane's: a CAiPath's plan aborted and its lists cleared) */
extern "C" bool func_00527310(void *path);

/* the route reset, and its state set: a running one (1 or 2) is ended first (OVL2 0x5557B0; the
 * path-recovery state calls it) */
extern "C" void func_005557B0(CSealCtrlAi *ai, AI_ROUTE route)
{
    func_00527310(&ai->m_route.m_path);
    switch (ai->m_route.m_state) {
    case ROUTE_1:
    case ROUTE_2:
        ai->m_route.m_state = ROUTE_NONE;
        break;
    }
    ai->m_route.m_state = route;
}

/* the route reset; a running one ends as 2 or none (OVL2 0x555820; the script and goto states'
 * signals call it) */
extern "C" void func_00555820(CSealCtrlAi *ai, bool keep)
{
    func_00527310(&ai->m_route.m_path);
    switch (ai->m_route.m_state) {
    case ROUTE_1:
    case ROUTE_2:
        ai->m_route.m_state = keep ? ROUTE_2 : ROUTE_NONE;
        break;
    }
}

extern "C" {
/* a path started over from a cell of a map (OVL2 0x526810, the AI core's: the route's legs begun at the
 * SEAL's cell) and a path's plan copied from another, whether it held one (OVL2 0x527FD0, the AI core's) */
void func_00526810(CAiPathBytes *path, const CAiMapLoc &loc, CAiMap *map, int a, int b);
bool func_00527FD0(CAiPathBytes *path, const CAiPathBytes *from);
}

/* a path's plan under way (states 1-3: ai.h's CAiPath::IsPlanning) and one held complete (state 4 with
 * points: IsState4Held's test on a path) */
static inline bool PathPlanning(const CAiPathBytes *p)
{
    return (unsigned int)(p->m_state - 1) < 3;
}

static inline bool PathHeld(const CAiPathBytes *p)
{
    return !(p->m_9c == 0) && p->m_state == 4;
}

/* a route made from a path: cleared, then begun at the SEAL's last good cell -- as a route still planning
 * (2) while the path plans, or (1) with the path's plan copied once it is held; whether it was made
 * (OVL2 0x5556A0, new in SOCOM II: 0x5558D0 and 0x55E3B0 call it; the name unknown) */
extern "C" bool func_005556A0(CSealCtrlAi *ai, const CAiPathBytes *path, CAiRouteBytes *route)
{
    CEntity *e = ai->m_entity;

    func_00527310(&route->m_path);
    route->m_state = ROUTE_NONE;

    const CAiMapLoc &loc = e->m_mapLocValid;

    route->m_state = ROUTE_NONE;

    bool r = false;

    if (PathPlanning(path)) {
        func_00526810(&route->m_path, loc, AiMaps.GetMapByIndex(loc.m_mapid), 0, 1);
        route->m_state = ROUTE_2;
        r = true;
    } else if (PathHeld(path)) {
        func_00527FD0(&route->m_path, path);
        func_00526810(&route->m_path, loc, AiMaps.GetMapByIndex(loc.m_mapid), 0, 1);
        route->m_state = ROUTE_1;
        r = true;
    }
    return r;
}

/* a table of two rows of eighteen, the row picked by whether the word at 0xC is empty (OVL2
 * 0x558AA0; 0x558640 reads five of them) */
struct CAiRowTable {
    unsigned char _0[0xc];
    void *m_c;   // 0xc
    unsigned char _10[0x18 - 0x10];
    void *m_rows[2][18];   // 0x18

    bool IsEmpty() { return !m_c; }
};

extern "C" void *func_00558AA0(CAiRowTable *t, unsigned char i)
{
    return t->m_rows[t->IsEmpty()][i];
}

/* (OVL2 0x60A850, unowned: a test of the AI's object at 0x67C) */
extern "C" bool func_0060A850(void *p);

/* the AI's target held, with its bit 9 (an inline bool, the kit lane's TargetHeld: the target
 * materialises the `&&`) */
static inline bool TargetHeld(CSealCtrlAi *ai)
{
    return func_0060A850(ai->m_67c) && (ai->m_6a8 & 0x200);
}

/* (OVL2 0x56D090) */
void CSealCtrlAi::RestoreAiMode()
{
    SetAiMode(m_3f5);
    m_3f5 = (AI_MODE)1;
}

/* the AI's turn rate (OVL2 data 0x64FD98, 2.0: read with lwc1, so a variable; the name ours) */
extern "C" float data_0064FD98;

/* the turn toward a goal direction in the ground plane, at most `max`: none inside the dead zone,
 * proportional to the cross product (clamped) within 20 degrees, the whole rate by its sign beyond,
 * the dot product handed back (SOCOM 1's ComputeTurnThrottle, its body with the rate a parameter:
 * both controllers' copies share its statics, OVL2 data 0x650110, 0x650118, 0x650120 -- 1.0, cos 20
 * degrees, 1.25, 8 bytes apart as SOCOM 1's guarded ones; SOCOM 1's were 1.0, cos 30 and 1.75 with
 * the rate 1.0). `max` is homed on the stack: a reference to it is passed on */
static inline float ComputeTurnThrottle(const CPnt3D *dir, const CPnt3D *goal, float *dot, float max)
{
    static float dead = 1.0f;
    static float cone = 0.9396899938583374f;
    static float gain = 1.25f;
    float turn;

    *dot = dir->x * goal->x + dir->z * goal->z;
    turn = 0.0f;
    if (*dot > dead)
        turn = 0.0f;
    else if (*dot > cone) {
        turn = dir->z * goal->x - dir->x * goal->z;
        turn *= gain;
        _saturate(&turn, -max, max);
    } else
        turn = max * (dir->z * goal->x - dir->x * goal->z >= 0.0f ? 1.0f : -1.0f);
    return turn;
}

/* the alertness's band as a level (0x560A60's walk inlined, its 4 an enumerator: `li`, where the
 * unsigned char's is `daddiu`; the names ours) */
enum ALERT_BAND { ALERT_BAND_0 = 0, ALERT_BAND_1 = 1, ALERT_BAND_2 = 2, ALERT_BAND_3 = 3, ALERT_BAND_4 = 4 };

static inline ALERT_BAND AlertBand(float x)
{
    for (int i = 0; i <= 4; i++)
        if (InBand(x, i))
            return (ALERT_BAND)i;
    return ALERT_BAND_4;
}

/* the AI's turn: at its own rate, slowed by the speed (between 0.35 and the whole, OVL2 data
 * 0x650100/0x650108 -- this function's statics) when the alertness is low or the look deploys,
 * unless a Navy SEAL in mode 6 (OVL2 0x56C340: slot 0x9C; the bands are a switch, its chain ending
 * on a `b`) */
float CSealCtrlAi::ComputeTurnThrottle(const CPnt3D *dir, const CPnt3D *goal, float *dot)
{
    static float slowest = 0.35f;
    static float fastest = 1.0f;
    float max = data_0064FD98;

    if (!(m_3f4 == 6 && IsNavySeal())) {
        bool slow = false;

        switch (AlertBand(((CAlertness *)this)->m_alert)) {
        case ALERT_BAND_0:
        case ALERT_BAND_1:
            slow = true;
        }
        if (m_look_mode == 9)
            slow = true;
        if (slow)
            max *= slowest * (1.0f - m_speed) + fastest * m_speed;
    }
    return ::ComputeTurnThrottle(dir, goal, dot, max);
}

/* (OVL2 0x56C220: slot 0x9C -- SOCOM 1's non-virtual CSealCtrl::ComputeTurnThrottle, virtual in
 * SOCOM II, at a rate of 1.5) */
float CSealCtrl::ComputeTurnThrottle(const CPnt3D *dir, const CPnt3D *goal, float *dot)
{
    return ::ComputeTurnThrottle(dir, goal, dot, 1.5f);
}

static inline bool InRegion(const CAiMap::region *r, const CEntity *e)
{
    return r->Contains(e->m_mapLoc);
}

/* whether a live entity of a team in the mask stands in a map region: the entity list walked by a
 * const view, its iterator assigned (OVL2 0x56D530, new in SOCOM II: 0x56D2B0 and CTRegion::Tick
 * call it; the name unknown).
 * OPEN, 12 instructions (the size right): the target holds the entity in t2, takes &e->m_mapLoc
 * into a2 right after reading its map byte and reuses t2 for Contains's `in` (`move t2,zero` in the
 * map test's delay slot); here the entity is t3 and the address is taken in that slot. Calling
 * region::Contains straight folds the address into the loads and drops the chain's join pad (0x168);
 * through this wrapper taking the entity both come back. Tried: the entity declared before the walk
 * or outside the loop, the wrapper taking the cell by reference or pointer, a reference or pointer
 * local to the cell, the region by reference, Contains written out with the cell's pointer first,
 * `in` returned through a local. */
extern "C" bool func_0056D530(const CAiMap::region *r, unsigned int team)
{
    bool found = false;

    if (r && team) {
        const CEntityList &list = CEntity::m_list;
        CEntityList::const_iterator i;

        for (i = list.begin(); !found && i != list.end(); ++i) {
            CEntity *e = *i;

            if (e && e->m_isAlive && (team & e->m_team))
                found = InRegion(r, e);
        }
    }
    return found;
}


/* the leg to walk: with a planned path's points, from the cell to the path's last point -- both
 * centres flat on the ground, the direction and length between them, the leg's words cleared; else
 * just the cell (the SEAL's own when none is given) and the deploying look dropped (OVL2 0x56C080,
 * new in SOCOM II: SetAiMode, PathFollowTick and 0x56C010's pop of a point call it; the name
 * unknown) */
extern "C" void func_0056C080(CSealCtrlAi *ai, CAiPath *path, const CAiMapLoc *loc)
{
    CEntity *e = ai->m_entity;

    ai->m_614 = 0;
    if (!path->m_points.size() || !path->HasPoints() || path->IsPlanning()) {
        ai->m_60c = 0;
        loc = loc->x == CAiMapLoc::invalid.x && loc->y == CAiMapLoc::invalid.y ? &e->m_mapLocValid : loc;
        ai->m_leg_cell = *loc;
        ai->ClearLook(AIL_DEPLOY);
    } else {
        ai->m_leg_cell = *loc;
        AiMaps.GetCellCenter(*loc, &ai->m_leg_from);
        AiMaps.GetCellCenter(path->m_points.back().m_loc, &ai->m_leg_to);
        ai->m_leg_to.y = 0.0f;
        ai->m_leg_from.y = 0.0f;
        ai->m_leg_dir = ai->m_leg_to - ai->m_leg_from;
        ai->m_leg_len = ai->m_leg_dir.Normalize();
        ai->m_608 = 0;
        ai->m_60c = 0;
        ai->m_614 = 0;
        ai->m_610 = 0;
    }
}

/* the controller's SEAL (an inline: the kit's test reads m_entity again through it) */
static inline CZSealBody *SealOf(CSealCtrlAi *ai)
{
    return (CZSealBody *)ai->m_entity;
}

/* bit 7 set or cleared -- never set with bit 1 -- its byte and block reset as it is set, the range
 * put back to its goal as it is cleared; the bit returned (OVL2 0x561E20, new in SOCOM II:
 * CAiSNoAmmo::Tick sets it with -1; the name unknown) */
extern "C" bool func_00561E20(CSealCtrlAi *ai, bool on, signed char c)
{
    bool cleared = !on && ai->m_aibit7;

    if (ai->m_navyseal)
        on = false;
    ai->m_aibit7 = on;
    if (ai->m_aibit7) {
        memset(ai->m_3e4, 0, sizeof(ai->m_3e4));
        ai->m_3e0 = c;
    } else if (cleared)
        ai->m_6f4 = ai->m_6f8;
    return ai->m_aibit7;
}

/* (OVL2 0x56D890).
 * OPEN, 9 instructions: the target loads the team straight into a0 (`lw a0,4(a0); lw a0,0xC8(a0)`)
 * and reads m_entity again for the kit's test after the two team tests; MW here keeps the entity in
 * a1 across both (and moves `on` to s0 first). Tried: the kit through this inline, the team in a
 * local, every test through the inline, the tests unparenthesised. */
void CSealCtrlAi::SetAiBit2(bool on)
{
    if ((m_entity->m_team & 0x20000) || (m_entity->m_team & 0x4000000) || !SealOf(this)->m_kit.GetCurItem())
        on = false;
    m_aibit2 = on;
}

extern "C" {
/* the flag the stance is asked with (OVL2 data, a byte passed as it is: a bool) */
extern bool data_006502D0;
}

/* (OVL2 0x562FC0) */
bool CSealCtrlAi::InStance(SEAL_STANCE stance, bool fromState)
{
    bool r;

    if (!fromState) {
        r = m_stance == stance;
        return r;
    } else
        r = stance == CZSealBody::GetStanceFromState(SealOf(this)->GetStance(data_006502D0), m_stance);
    return r;
}

/* (OVL2 0x567840) */
void CSealCtrlAi::StopThrottles(bool keep)
{
    SetThrottle(1, 0.0f);
    SetThrottle(2, 0.0f);
    SetThrottle(0, 0.0f);
    SetThrottle(3, 0.0f);
    if (!keep) {
        m_throttle[1] = 0.0f;
        m_throttle[2] = 0.0f;
        m_throttle[0] = 0.0f;
        m_throttle[3] = 0.0f;
    }
}

/* (OVL2 0x563100) */
bool CSealCtrlAi::SetStanceDelayed(SEAL_STANCE stance, float delay)
{
    if (stance != m_stance_asked) {
        if (delay == -1.0f) {
            float r = 4.656612873e-10f * (float)rand();

            delay = 0.25f + (1.2f - 0.25f) * r;
        }
        m_stance_asked = stance;
        m_280 = delay;
    }
    return true;
}

/* (OVL2 0x563050) */
bool CSealCtrlAi::ChangeStanceDelayed(SEAL_STANCE from, SEAL_STANCE to, float delay)
{
    if (m_stance == from || m_stance_asked == from) {
        if (delay == -1.0f) {
            float r = 4.656612873e-10f * (float)rand();

            delay = 0.25f + (1.2f - 0.25f) * r;
        }
        m_stance_asked = to;
        m_280 = delay;
    }
    return true;
}

/* the mission (theMission, OVL1 bss 0x4364E0) -- only its clock (remote_control.cpp's reading), and
 * the team valves refreshed and the fade to the mission's end (SOCOM 1's; OVL1 0x2B23A0, 0x2A97C0) */
class CMission {
public:
    void RefreshTeamValves();
    void FadeToEndMission(float time, int how);

    unsigned char _0[0xe0];
    float m_time;   // 0xe0
};

extern CMission theMission;

extern "C" {
/* what the anger cools by each tick (OVL2 bss 0x66B0F8, read as a variable: CreepTick reads it too),
 * and the anger past which the squad turns (OVL2 data 0x64FE08, 190.0f, read as a variable) */
extern float data_0066B0F8;
extern float data_0064FE08;
/* a squad's sound (OVL2 0x5EAF20, the kit lane's seal_unit.cpp) */
void func_005EAF20(CSealUnit *unit, int sound, int b, float chance);
}

/* the player turned on: offline, by a Navy SEAL of the player's team angered past 190 (or told to: both
 * callers say not) -- the player's top squad given commands 5 and 3, the player put out of every team,
 * the team valves refreshed, the squad told (message 4), its sound 82, and the mission faded out over
 * ten seconds; short of that the anger cools while it is not below zero (OVL2 0x561B90, new in SOCOM
 * II: CSealCtrlAi::Tick and OnDeath call it after raising 0x400; the name unknown) */
extern "C" void func_00561B90(CSealCtrlAi *ai, bool now)
{
    CZSealBody *player;
    CZSealBody *seal = (CZSealBody *)ai->m_entity;

    player = ftsGetPlayer();
    if (theNetwork.m_9)
        return;
    if (!ai->IsNavySeal())
        return;
    if (!(player->m_team & seal->m_team))
        return;
    if (now || ai->m_400 > data_0064FE08) {
        CSealUnit *unit = ((CSealCtrl *)player->m_control)->m_unit->GetTop();

        unit->IssueCommand(player, (FT_COMMAND)5, 0, true);
        unit->IssueCommand(player, (FT_COMMAND)3, 0, true);
        player->SetTeamMask(0);
        player->JoinTeam(0);
        theMission.RefreshTeamValves();
        unit->Notify(seal, (UNIT_MESSAGE)4, player);
        func_005EAF20(unit, 82, 1, 1.0f);
        theMission.FadeToEndMission(10.0f, 0);
    } else if (ai->m_400 >= 0.0f)
        ai->m_400 -= data_0066B0F8;
}

extern "C" {
/* whether the SEAL can take a stance where it stands (OVL2 0x562C80, not written yet: an int the
 * caller makes a bool) */
int func_00562C80(CSealCtrlAi *ai, SEAL_STANCE stance);
}

/* a stance taken: nothing to do when it is the stance already; else the body's state for it -- prone
 * falling back to a crouch where the SEAL cannot lie down -- then the stance kept, the asked one
 * dropped and the time stamped, and the body sent to the state (OVL2 0x562B70; SOCOM 1's
 * DoSetStance by its call, the body new) */
bool CSealCtrlAi::DoSetStance(SEAL_STANCE stance)
{
    bool r = false;
    int state = -1;
    CZSealBody *seal = (CZSealBody *)m_entity;

    m_280 = 0;
    if (m_stance == stance)
        r = true;
    if (!r) {
        bool blocked = !func_00562C80(this, stance);

        r = true;
        switch (stance) {
        case STANCE_STAND:
            state = stateStand;
            break;
        case STANCE_CROUCH:
            state = stateCrouch;
            break;
        case STANCE_PRONE:
            /* (an if/else: the target's not-blocked arm falls through, the crouch laid after it) */
            if (!blocked)
                state = stateProne;
            else {
                stance = STANCE_CROUCH;
                state = stateCrouch;
            }
            break;
        }
    }
    if (r) {
        m_stance = stance;
        m_stance_asked = STANCE_NONE;
        m_280 = 0;
        m_stance_delay = theMission.m_time;
        if (state != -1 && state != seal->m_desiredState)
            seal->m_desiredState = (SEAL_STATE)state;
    }
    return r;
}

/* (OVL2 0x5631A0) */
bool CSealCtrlAi::SetStance(SEAL_STANCE stance)
{
    m_stance_asked = stance;
    m_280 = 0;
    return DoSetStance(stance);
}

/* (OVL2 0x5631B0) */
void CSealCtrlAi::SetDesiredStance(SEAL_STANCE stance, float delay, bool force)
{
    if (stance == STANCE_PRONE || TargetHeld(this) || force) {
        ((CZSealBody *)m_entity)->m_desiredStance = stance;
        m_stance_timer.Set(delay);
    }
}

extern "C" {
/* the side a found cell faces from its direction (OVL2 0x55FA90, not written yet) */
unsigned char func_0055FA90(CAiMap *map, const CAiMapLoc &loc, CAiMap::DIR dir);
}

/* a cell to look from, and one to take cover in, near the SEAL's last good cell (its map's
 * FindLookLoc and FindCoverLoc): the side it faces handed back, 2 when none is found (OVL2 0x55F7F0
 * and 0x55F940, new in SOCOM II: CAiSPursue::Tick calls both; the names unknown).
 * OPEN, both 27 instructions, registers only (the code and its size right): the target numbers the
 * result lowest and the map highest -- map s0, radius s1, side s2, out s3, pos s4, found s5 -- where
 * MW here gives the map s5 and shifts the parameters down one. Tried: the flag declared first, every
 * local declared first and assigned after, the call made in the test. Its being numbered below the
 * parameters says the flag is not a plain local: an inline (the two finders' common body, the
 * finder its only difference?) is the next thing to try */
extern "C" bool func_0055F940(CSealCtrlAi *ai, const CPnt3D *pos, CAiMapLoc &out, unsigned char *side,
                              int radius)
{
    CAiMapLoc loc(ai->m_entity->m_mapLocValid);
    CAiMap *map = AiMaps.GetMapByIndex(loc.m_mapid);
    bool found = false;
    CAiMap::DIR dir = CAiMap::DIR_COUNT;

    *side = 2;
    if (map) {
        found = map->FindCoverLoc(loc, pos, out, dir, radius);
        if (found)
            *side = func_0055FA90(map, out, dir);
    }
    return found;
}

extern "C" bool func_0055F7F0(CSealCtrlAi *ai, const CPnt3D *pos, CAiMapLoc &out, unsigned char *side,
                              int radius)
{
    CAiMapLoc loc(ai->m_entity->m_mapLocValid);
    CAiMap *map = AiMaps.GetMapByIndex(loc.m_mapid);
    bool found = false;
    CAiMap::DIR dir = CAiMap::DIR_COUNT;

    *side = 2;
    if (map) {
        found = map->FindLookLoc(loc, pos, out, dir, radius);
        if (found)
            *side = func_0055FA90(map, out, dir);
    }
    return found;
}

/* CAiSPursue's slot 0x14 (new in SOCOM II): the pursuit stops unless the AI's object says so,
 * and the Tick's result (OVL2 0x55A760; an int, handed the sub-stack's state, as ai_states.h's slot
 * 0x14 -- the kit lane, round 6). ai_states.h's Result() gives the target's `movz` and the
 * byte enum's `andi` -- the earlier spelling as a status computed here was the miss */
int CAiSPursue::Virtual14(CSealCtrlAi *ai, CAiState *sub)
{
    if (!func_0060A850(ai->m_67c))
        m_active = false;
    return Result();
}

/* CAiSPursue's slot 0x48 (new in SOCOM II): the timer and the byte taken over from another
 * pursuit (OVL2 0x55A7D0) */
static inline CAiSPursue *AsPursue(CAiState *s)
{
    return s && s->m_state == (AI_STATE)12 ? (CAiSPursue *)s : 0;
}

bool CAiSPursue::Virtual48(int x, CAiState *other)
{
    CAiSPursue *o = AsPursue(other);

    m_timer = o->m_timer;
    m_5c = o->m_5c;
    return true;
}

/* the seven weapon ids of the AI's kinds (OVL2 data 0x66AD00) */
extern "C" int data_0066AD00[7];

/* the kinds (a byte enum; the names ours: rows 2 and 3 are both kind 1) */
enum AI_WEAPON_KIND {
    WEAPON_KIND_0 = 0, WEAPON_KIND_1 = 1, WEAPON_KIND_2 = 2, WEAPON_KIND_3 = 3, WEAPON_KIND_4 = 4,
    WEAPON_KIND_5 = 5, WEAPON_KIND_6 = 6
};

/* the kind of the weapon the body holds: its row in the table, 2 and 3 both kind 1, none 0 (OVL2
 * 0x560540) */
/* OPEN, 0x70 against 0x60: the target sets the result to 0 first, takes the row into v0 and tests
 * it against 2 then 3 (`bne 3` straight to the exit), returning 1 for both; MW here tests 3 apart
 * and branches over a join. Tried: the switch after the loop on a result local, returning from
 * inside the loop, the conversion as an inline switch. */
#pragma auto_inline off
extern "C" AI_WEAPON_KIND func_00560540(CSealCtrlAi *ai)
{
    for (int i = 0; i < 7; i++) {
        if (((CZSealBody *)ai->m_entity)->m_weapon_id == data_0066AD00[i]) {
            AI_WEAPON_KIND kind = (AI_WEAPON_KIND)i;

            switch (kind) {
            case WEAPON_KIND_3:
            case WEAPON_KIND_2:
                return WEAPON_KIND_1;
            }
            return kind;
        }
    }
    return WEAPON_KIND_0;
}
#pragma auto_inline reset

/* (OVL2 0x5605A0, not written yet: a weapon of a kind chosen) */
extern "C" bool func_005605A0(CSealCtrlAi *ai, int a);

/* a weapon chosen; the kind held before it kept once it is (OVL2 0x5604D0) */
extern "C" bool func_005604D0(CSealCtrlAi *ai, int a)
{
    bool r = false;
    AI_WEAPON_KIND kind = func_00560540(ai);

    if (func_005605A0(ai, a)) {
        ai->m_weapon_kind = kind;
        r = true;
    }
    return r;
}

extern "C" int rand(void);

/* a random value in a range (SOCOM 1's Rfloat: the minimum and the span; its inline operator's
 * out-of-line copy at OVL2 0x560960, after 0x5605A0 its first caller, as SOCOM 1's 0x25ABA0 sits
 * after CAiSPursue::Tick) */
class Rfloat {
public:
    Rfloat(float min, float range) : m_min(min), m_range(range) {}
    operator float() const;

    float m_min;   // 0x0
    float m_range;   // 0x4
};

/* a state's bit 2 as a bool (an inline: the byte read again and the test materialised, apart from
 * the running bit's) */
static inline bool StateBit2(const CAiState *s)
{
    return s->m_bit2 ? true : false;
}

/* a running state's nod, once: the SEAL looks at its leader -- the player when it leads itself -- a
 * moment, and says so (sound 56, or 55 when asked), the state's bit 2 set (OVL2 0x560340, new in
 * SOCOM II: the deploy, action, breach, goto and follow states' Ticks call it; the names ours) */
extern "C" void func_00560340(CSealCtrlAi *ai, CAiState *state, bool b)
{
    if (!state || !state->m_active || StateBit2(state))
        return;
    ftsGetPlayer();
    CEntity *leader = ai->m_unit->GetLeader();

    /* (the ternary: an if leaves the join's label aligned with a nop) */
    leader = leader == ai->m_entity ? ftsGetPlayer() : leader;
    if (leader) {
        CPnt3D pos;

        if (ai->SetLookPosW((AI_LOOK)8, leader->GetTargetPosW(&pos)))
            ai->SetLookTimer(Rfloat(0.5f, 1.0f));
        if (!b)
            ai->PlayAiSound((CHARACTER_SOUND)56, 1.0f, -1);
        else
            ai->PlayAiSound((CHARACTER_SOUND)55, 1.0f, -1);
        state->SetBit2(true);
    }
}

/* a sound played for an item, surely and with no AI event: the one given, or for none (-1) the item's
 * own -- 41 to 47 for items 128, 127, 151, 152, 121, 122 and 123, else 40 (OVL2 0x560280, new in SOCOM
 * II: CAiSDeploy::Tick calls it; the name unknown -- the cases written so, their tests run reversed) */
extern "C" void func_00560280(CSealCtrl *ctrl, unsigned char item, CHARACTER_SOUND sound)
{
    if (sound == -1) {
        sound = (CHARACTER_SOUND)40;
        switch (item) {
        case 128:
            sound = (CHARACTER_SOUND)41;
            break;
        case 127:
            sound = (CHARACTER_SOUND)42;
            break;
        case 151:
            sound = (CHARACTER_SOUND)43;
            break;
        case 152:
            sound = (CHARACTER_SOUND)44;
            break;
        case 121:
            sound = (CHARACTER_SOUND)45;
            break;
        case 122:
            sound = (CHARACTER_SOUND)46;
            break;
        case 123:
            sound = (CHARACTER_SOUND)47;
            break;
        }
    }
    ctrl->PlayAiSound(sound, 1.0f, -1);
}

/* the random part through a local: written as one expression the join loses its pad and an
 * inlined copy (0x5608B0, PlayAiSound) its registers */
Rfloat::operator float() const
{
    if (m_range == 0.0f)
        return m_min;
    float r = 4.656613e-10f * (float)rand();

    return m_min + m_range * r;
}

/* whether a chance comes up (OVL2 0x5608B0; 0x5605A0 calls it) */
extern "C" bool func_005608B0(const float &chance)
{
    return Rfloat(0.0f, 1.0f) <= chance;
}

/* a direction to look in from a map spot (OVL2 0x559580, not written: its arguments are what
 * 0x567690 passes) */
extern "C" CAiMap::DIR func_00559580(const CAiMapLoc *loc, int a, unsigned int mask, int b, int n, int c);

/* the AI's glance around before it creeps: while it has a target or acts the countdown is held at 0;
 * else run down, and on expiry restarted at a random 10 to 15 (a guarded function-local Rfloat,
 * 0x66B208/0x66B210) and -- with no look of a higher mode -- a normal look set toward a direction the
 * map gives the SEAL's spot; then the creep (OVL2 0x567690, new in SOCOM II: PreTick and 0x56AF50
 * call it; the name unknown) */
/* whether a look of a mode may be set: none of a higher mode holds (an inline, the name ours) */
static inline bool LookAllowed(CSealCtrlAi *ai, AI_LOOK look)
{
    return look >= ai->m_look_mode;
}

extern "C" void func_00567690(CSealCtrlAi *ai, float dT, bool b)
{
    CEntity *e = ai->m_entity;
    static Rfloat s_wait(10.0f, 5.0f);

    if (!func_0060A850(ai->m_67c) && !ai->AiStateIs(AI_STATE_ACTION)) {
        ai->m_3fc -= dT;
        if (ai->m_3fc < 0.0f) {
            ai->m_3fc = s_wait;
            if (LookAllowed(ai, AIL_NORMAL)) {
                if (ai->SetLookDirW(AIL_NORMAL, func_00559580(&e->m_mapLocValid, 0, 0x40000, 0, 20, 0)))
                    ai->ModifyLookFlags(1, 2);
            }
        }
    } else
        ai->m_3fc = 0.0f;
    ai->CreepTick(dT, b);
}

/* what the AI's line walk carries to its callback (0x56A2E0) and hands back: the walk stopped, an
 * entity met and where, the follow's restraint, a distance, the target cell, the way back and the
 * controller (new in SOCOM II; the layout read off 0x56A130 and the callback, the names ours) */
struct SEAL_LINEWALK {
    unsigned char m_blocked;   // 0x0
    unsigned char _1;
    unsigned char m_met;   // 0x2: an entity in the way
    unsigned char _3[9 - 3];
    bool m_restrained;   // 0x9
    unsigned char _a[0xc - 0xa];
    CAiMapLoc m_metloc;   // 0xc
    float m_dist;   // 0x10
    float m_scale;   // 0x14
    CAiMapLoc m_to;   // 0x18
    CPnt3D m_back;   // 0x1c: the SEAL's facing, reversed
    unsigned char _28[4];
    CSealCtrlAi *m_ai;   // 0x2c
};

extern "C" {
bool func_0056A2E0(const MAP_LINEWALK_STRUCT &s, void *data);
CEntity *func_00569AA0(CEntity *e, const CAiMapLoc *loc);
}

/* whether the AI can walk the line from its SEAL's cell to another of the same map: the walk's
 * data filled -- the follow state offline also carrying the SEAL's restraint -- the line walked in
 * steps of 0.2, an entity met in the way forgotten when none stands there or it is the SEAL itself
 * (OVL2 0x56A130, new in SOCOM II: CreepTick, 0x567900 and CAiSAction::Tick call it; the name
 * unknown) */
extern "C" bool func_0056A130(CSealCtrlAi *ai, const CAiMapLoc &to, SEAL_LINEWALK *w, float dist)
{
    CEntity *e = ai->m_entity;
    const CAiMapLoc &from = e->m_mapLoc;

    if (to.m_mapid != from.m_mapid)
        return false;
    CAiMap *map = AiMaps.GetMapByIndex(from.m_mapid);
    const CMatrix &m = e->m_node->m_matrix;

    w->m_ai = ai;
    w->m_back.x = -m.m_matrix[2][0];
    w->m_back.y = -m.m_matrix[2][1];
    w->m_back.z = -m.m_matrix[2][2];
    w->m_to = to;
    w->m_dist = dist;
    w->m_scale = 1.0f;
    if (ai->AiStateIs(AI_STATE_FOLLOW) && !theNetwork.m_9)
        w->m_restrained = ((CZSealBody *)e)->m_restrained;
    map->WalkLine(from, to, func_0056A2E0, w, 0.2f);
    if (w->m_met) {
        CEntity *met = func_00569AA0(e, &w->m_metloc);

        if (!met || met == e)
            w->m_met = false;
    }
    return !(w->m_blocked || w->m_met);
}

/* the box around eight points -- a box's corners (OVL2 0x56ACE0, new in SOCOM II: 0x56A8E0 calls it;
 * the name unknown) */
/* a bound lowered or raised to a value (inlines, the names ours: the value, the later argument, is
 * read before the bound -- the ternary written in place reads the bound first) */
static inline void LowerTo(float &bound, float v)
{
    bound = v <= bound ? v : bound;
}

static inline void RaiseTo(float &bound, float v)
{
    bound = v >= bound ? v : bound;
}

extern "C" void func_0056ACE0(CBBox *box, const CPnt3D *pts)
{
    box->m_min = pts[0];
    box->m_max = pts[0];
    for (int i = 1; i < 8; i++) {
        const CPnt3D *p = &pts[i];

        LowerTo(box->m_min.x, p->x);
        LowerTo(box->m_min.y, p->y);
        LowerTo(box->m_min.z, p->z);
        RaiseTo(box->m_max.x, p->x);
        RaiseTo(box->m_max.y, p->y);
        RaiseTo(box->m_max.z, p->z);
    }
}

extern "C" {
void func_00570F90(CSealCtrlAi *ai, float t);
void func_005B3890(CEntity *seal, int x);
}

/* the AI put back on its feet off a ladder: out of mode 7 -- the mode before it restored -- into
 * mode 3, the climb's words cleared, the move goal dropped, 0x570F90 asked with 1 and the SEAL's
 * 0x5B3890 with 1 (OVL2 0x56BEF0, new in SOCOM II: 0x56AF50 calls it three times; the name
 * unknown) */
extern "C" void func_0056BEF0(CSealCtrlAi *ai)
{
    CEntity *e = ai->m_entity;

    if (ai->m_3f4 == 7) {
        if (ai->m_3f4 == 7)
            ai->RestoreAiMode();
        ai->SetAiMode((AI_MODE)3);
    }
    ai->m_448 &= 0xffff0000;
    ai->m_440 = 0;
    ai->m_444 = 0;
    ai->ClearGoal(GOAL_MOVE);
    func_00570F90(ai, 1.0f);
    func_005B3890(e, 1);
}

extern "C" {
/* the mission's vehicles, counted (OVL2 bss: OnMissionStart sets it), the reach within which a target
 * vehicle is looked at, squared, and the nearness asked of its box (OVL2 data, 250000 and 5: read
 * with lwc1, variables; the names ours) */
extern int data_0066AFE8;
extern float data_00650170;
extern float data_00650168;
bool func_0056A8E0(CEntity *vehicle, const CPnt3D *pos, float near);
void func_00569550(CSealCtrlAi *ai, unsigned int id);
}

/* the entity as a vehicle (type 4, CVehicle's), if it is one (an inline, the name ours) */
static inline CEntity *AsVehicle(CEntity *e)
{
    return e && e->m_type == 4 ? e : 0;
}

/* the vehicle a spot -- the SEAL's own by default -- is at: the SEAL's targets walked nearest first
 * while within reach, the first vehicle whose box the spot is near; told to 0x569550 when asked (OVL2
 * 0x56A760, new in SOCOM II: PreTick calls it five times, the line walk's callback once; the name
 * unknown) */
extern "C" unsigned int func_0056A760(CSealCtrlAi *ai, const CPnt3D *pos, bool tell)
{
    unsigned int id = CEntity::invalidId;
    CEntity *e = ai->m_entity;

    if (data_0066AFE8 > 0) {
        if (!pos)
            pos = &e->m_posW;
        CTargetList *list = &e->m_targets;

        for (CTargetList::iterator i = list->begin(); i != list->end(); ++i) {
            if (!(i->m_distSq <= data_00650170))
                break;
            CEntity *v = AsVehicle(i->m_ent);

            if (v && func_0056A8E0(v, pos, data_00650168)) {
                id = v->m_id;
                break;
            }
        }
        if (tell && id != CEntity::invalidId)
            func_00569550(ai, id);
    }
    return id;
}

extern "C" {
/* a switch 0x572880 clears when its question fails (OVL2 bss; the name ours) */
extern unsigned char data_0066AFE0;
}

/* the state as a hold (kind 2), if it is one (an inline, the name ours) */
static inline CAiState *AsHold(CAiState *s)
{
    return s && s->m_state == AI_STATE_HOLD ? s : 0;
}

/* whether a player SEAL hit is the one team 2's second member holds for: the switch on, the SEAL a
 * player's, that member the AI's, its state a hold with flag 0x80 -- the AI and the state given
 * back, else the switch, the AI and the state cleared (OVL2 0x572880, new in SOCOM II: OnWeaponHit
 * asks it; the name unknown) */
extern "C" bool func_00572880(CZSealBody *seal, CSealCtrlAi **ai, CAiState **state)
{
    bool r = false;

    if (data_0066AFE0 && seal && seal->IsPlayer()) {
        CSealUnit *unit = CSealUnit::GetUnitByTeam(2);

        if (unit) {
            CZSealBody *s = unit->GetSealByIndex(1, 0);

            if (s && (s->m_control ? ((CSealCtrl *)s->m_control)->IsAi() : false)) {
                *ai = s->GetSealCtrlAi();
                *state = AsHold((*ai)->GetAiState());
                if (*state && ((*state)->m_flags & 0x80))
                    r = true;
            }
        }
    }
    if (!r) {
        data_0066AFE0 = 0;
        *ai = 0;
        *state = 0;
    }
    return r;
}

extern "C" {
/* whether the AI's task anims may run (OVL1 data, a byte; the name ours) */
extern unsigned char data_003DF188;
}

/* the state as a script (kind 1), if it is one (an inline, the name ours) */
static inline CAiSScript *AsScript(CAiState *s)
{
    return s && s->m_state == AI_STATE_SCRIPT ? (CAiSScript *)s : 0;
}

/* a task's anim activated with the arguments that follow, the copy kept four slots on (SOCOM II's
 * anims and copies, four each from 0x450 -- the task counted from 1); when asked and it ran, a
 * script state pushed that plays the anim, marked running; whether it ran (OVL2 0x561F00; SOCOM 1's
 * ActivateEx, variadic: a3-t3 stored to the frame, the flag too, its address taken by va_start) */
bool CSealCtrlAi::ActivateEx(SEAL_TASK task, bool push, ...)
{
    if (!data_003DF188)
        return false;
    va_list ap;

    va_start(ap, push);
    CZAnim *active;
    CZAnim *anim = m_anims[task - 1];

    active = 0;
    if (anim)
        active = anim->ActivateArgV(ap);
    m_anims[task + 3] = active;
    if (active && push && PushAiState(AI_STATE_SCRIPT, 1)) {
        CAiSScript *s = AsScript(GetAiState());

        s->m_58 = (unsigned int)anim;
        s->m_active = true;
    }
    return active != 0;
}

/* the target's entity, held in the word at 0x67C (an inline, the name ours) */
static inline CEntity *TargetOf(CSealCtrlAi *ai)
{
    return *(CEntity **)ai->m_67c;
}

extern "C" bool func_0055E670(CSealCtrlAi *ai, AI_FIREPARAMS *p);

/* a shot asked for by the fire parameters (flags 0x4000000 and 0x40000) taken: flag 4 with a target
 * aims at the target's spot and speed first; either way the aim (0x55E670) fires when ready, and
 * once done -- when asked -- the request is cleared with the look of mode 11; whether one was asked
 * (OVL2 0x55EAA0, new in SOCOM II: 0x563E10, 0x5B8920 and 0x5C1970 call it; the name unknown) */
extern "C" bool func_0055EAA0(CSealCtrlAi *ai, AI_FIREPARAMS *p, bool clear)
{
    bool r = false;

    if ((p->m_flags & 0x4000000) && (p->m_flags & 0x40000)) {
        r = true;
        if ((p->m_flags & 4) && func_0060A850(ai->m_67c)) {
            p->m_targetPos = TargetOf(ai)->m_posW;
            p->m_targetVel = TargetOf(ai)->m_velW;
            if (func_0055E670(ai, p))
                ai->FireWhenReady(*p, 0.0f);
        } else if (func_0055E670(ai, p))
            ai->FireWhenReady(*p, 0.0f);
        if (clear) {
            p->m_flags &= ~0x4000000;
            ai->ClearLook(AIL_11);
        }
    }
    return r;
}

/* the fire parameters aimed straight at the target's spot: the look along the line to it, the aim
 * point the spot, the model's aim down -z, flag 0x100000 on and 0x800000 off, the distance kept
 * (OVL2 0x55EBE0, new in SOCOM II: 0x55F0A0 picks it by flag 0x80000; the name unknown) */
extern "C" bool func_0055EBE0(CSealCtrlAi *ai, AI_FIREPARAMS *p)
{
    CPnt3D *target = &p->m_targetPos;
    CPnt3D dir = *target - ai->m_entity->m_posW;
    float dist = dir.Normalize();

    p->m_aimPointW = *target;
    p->m_lookDirW = dir;
    p->m_aimDirM = CPnt3D::unit_nz;
    p->m_flags &= ~0x800000;
    p->m_flags |= 0x100000;
    p->m_dist = dist;
    return true;
}

/* whether the body holds a weapon of the kind: kind 1 is rows 2 and 3 (OVL2 0x5609F0).
 * OPEN, 15 instructions: the target materialises each row's test (xor, sltiu) and branches on the
 * first, the entity in a2; MW here compares with beq and reuses the 1 the kind test left in v0.
 * Tried: inline bool helpers (the `||` one too), a result local, a bool local for the `||`. */
extern "C" bool func_005609F0(CSealCtrlAi *ai, unsigned char kind)
{
    CZSealBody *seal = (CZSealBody *)ai->m_entity;

    if (kind == 1)
        return data_0066AD00[2] == seal->m_weapon_id || data_0066AD00[3] == seal->m_weapon_id;
    return data_0066AD00[kind] == seal->m_weapon_id;
}

/* the three items AllowCommand asks about (OVL2 0x5664F0; the names ours) */
extern "C" bool func_005664F0(unsigned char item)
{
    switch (item) {
    case 152:
    case 153:
    case 151:
        return true;
    }
    return false;
}

/* the slide stepped: the AI's throttles and the body's stopped first; done sliding, the AI's mode
 * goes back to 1 (OVL2 0x566CF0, slot 0x8C) */
bool CSealCtrlAi::SlideTick(float dT)
{
    SetThrottle(1, 0.0f);
    SetThrottle(2, 0.0f);
    SetThrottle(0, 0.0f);
    SetThrottle(3, 0.0f);
    m_throttle[1] = 0.0f;
    m_throttle[2] = 0.0f;
    m_throttle[0] = 0.0f;
    m_throttle[3] = 0.0f;
    bool slid = CSealCtrl::SlideTick(dT);

    if (!slid)
        SetAiMode((AI_MODE)1);
    return slid;
}

/* the AI-map collision bit, kept off in mode 7 and stored only when it changes (OVL2 0x557EE0,
 * slot 0xB4) */
void CSealCtrlAi::SetDoAiMapCollide(bool on)
{
    if (m_3f4 == 7)
        on = false;
    if (m_do_aimap_collide != on)
        m_do_aimap_collide = on;
}

/* the sign of a value, as one (OVL2 0x557D90; the inline's copy after its caller, 0x557190) */
extern "C" float func_00557D90(float x)
{
    return x >= 0.0f ? 1.0f : -1.0f;
}

/* CZAnim::Stop, which SOCOM II gave a flag (zanim.h's declaration) */
extern "C" void func_002707C0(CZAnim *anim, bool now);

extern "C" {
extern float data_004365C0;   /* (OVL1 data: the delay a new stance waits, read as a variable) */
/* (OVL2 data: how long after its time at 0x40C the controller stays busy) */
extern float data_00650000;
}

/* what 0x572240 is handed: a table whose slot 0x30 fills a point by an amount, and a word at 4 (the AI
 * states' 0x5F8D40 and 0x5F95D0 pass it; a stand-in, the names ours) */
class CAiAlert {
public:
    virtual void V08();
    virtual void V0C();
    virtual void V10();
    virtual void V14();
    virtual void V18();
    virtual void V1C();
    virtual void V20();
    virtual void V24();
    virtual void V28();
    virtual void V2C();
    virtual void Fill(CPnt3D *p, float amount);   /* +0x30 */

    int m_4;   // 0x4
};

extern "C" {
/* the amount for none given, the range's factor, and the range that stands (OVL2 data 0x64FDD8,
 * 0x64FDF0, 0x64FE48, read as variables) */
extern float data_0064FDD8;
extern float data_0064FDF0;
extern float data_0064FE48;
/* (OVL2 0x60E7A0 and 0x60D280, unowned: OnDeath and OnStunDamage make the same pair of calls) */
void *func_0060E7A0(CSealCtrlAi *ai);
void func_0060D280(void *p);
}

/* the range narrowed by a factor unless it stands (an inline: the factor, its argument, is loaded
 * ahead of the test, and the range read again for the product) */
static inline void ScaleRange(CSealCtrlAi *ai, float k)
{
    if (data_0064FE48 != ai->m_6f4)
        ai->m_6f4 *= k;
}

/* an alert taken in: its amount (a default for none) summed, its point filled, its word kept, the range
 * narrowed unless it stands, then the pair OnDeath makes (OVL2 0x572240, new in SOCOM II; the name
 * unknown).
 * OPEN, 0xBC against 0xC4: through the inline the factor loads ahead of the test and the compare is the
 * target's (constant first), but the target reads the range again for the product (`lwc1; mul.s
 * $f0,$f0,$f2`) and pads the block, where MW here reuses the compare's load. Tried: the product as
 * `= m * k`, `= k * m`, through a pointer, a local copy, the test through an accessor or a cast
 * address, the test negated, the factor and the test written out in the function. */
extern "C" void func_00572240(CSealCtrlAi *ai, CAiAlert *alert, float amount)
{
    if (alert) {
        if (amount == 0.0f)
            amount = data_0064FDD8;
        ai->m_718 += amount;
        alert->Fill(&ai->m_724, amount);
        ai->m_730 = alert->m_4;
        ScaleRange(ai, data_0064FDF0);

        void *p = func_0060E7A0(ai);

        if (p)
            func_0060D280(p);
    }
}

/* whether the controller is busy: never a navy SEAL's; within a while of its time at 0x40C, or in
 * state 28 (OVL2 0x572190, new in SOCOM II: 0x5D4770, ai::PARAM's tick, asks it; the name unknown).
 * The stack test is AiStateIs (zseal.h): the target's `Is()` built whole with its explicit false arm,
 * through the const GetState (0x5F8330) -- open on that shape until round 18 */
extern "C" bool func_00572190(CSealCtrlAi *ai)
{
    bool r = false;

    if (!ai->IsNavySeal()) {
        if (data_004365C0 - ai->m_40c < data_00650000)
            r = true;
        else if (ai->AiStateIs((AI_STATE)28))
            r = true;
    }
    return r;
}

/* a stance changed with the state: taken as the goal after the delay, unless either end has none
 * (OVL2 0x571420; SOCOM 1's picked the delay per stance). Both stances come from the AI-states
 * lane's 0x5FC910, SOCOM 1's GetStanceFromState by its two calls here. */
void CSealCtrlAi::OnStateChange(SEAL_STATE from, SEAL_STATE to)
{
    SEAL_STANCE was = CZSealBody::GetStanceFromState(from, STANCE_NONE);
    SEAL_STANCE now = CZSealBody::GetStanceFromState(to, STANCE_NONE);

    if (was != STANCE_NONE && now != STANCE_NONE && now != m_stance) {
        m_stance = now;
        m_stance_delay = data_004365C0;
    }
}

/* the goal cleared: of the given kind, or whatever is set; the position and direction dropped and
 * the goal's timer stopped (OVL2 0x56DCA0; SOCOM 1's, SetGoalPos inlined -- its test written
 * `m_goal_type > type`, which alone puts the inlined copy's `slt at` in this function's delay slot). */
void CSealCtrl::ClearGoal(GOAL_TYPE type)
{
    if (type == GOAL_NONE)
        type = m_goal_type;
    if (m_goal_type == type) {
        SetGoalPos(type, 0, 0.0f);
        SetGoalDir(type, 0, false);
        m_goal_timer.m_timer = CAiTimer::infinite;
        m_goal_timer.m_min = CAiTimer::infinite;
        m_goal_timer.m_range = 0.0f;
    }
}

/* a map cell's centre as the goal (OVL2 0x56DBA0, new in SOCOM II: the point's SetGoalPos
 * inlined) */
bool CSealCtrl::SetGoalPos(GOAL_TYPE type, const CAiMapLoc &loc, float tolerance)
{
    bool set = false;
    CPnt3D center;

    if (AiMaps.GetCellCenter(loc, &center))
        set = SetGoalPos(type, &center, tolerance);
    return set;
}

/* a point as the goal, unless a goal of a higher kind is set; none clears the position, and the
 * kind with it once no direction is left (OVL2 0x56DAA0; SOCOM 1's) */
bool CSealCtrl::SetGoalPos(GOAL_TYPE type, const CPnt3D *pos, float tolerance)
{
    /* the member first: inlined (ClearGoal, SetGoalDir) the test then branches on `slt at` from
     * the delay slot before it, where `type < m_goal_type` computes it into v0 after a nop */
    if (m_goal_type > type)
        return false;
    if (pos) {
        m_goal_pos = *pos;
        m_goal_tolerance = tolerance;
        m_goal_pos_valid = true;
        if (m_goal_dir_valid && m_goal_type != type)
            SetGoalDir(m_goal_type, 0, false);
        m_goal_type = type;
    } else {
        m_goal_pos_valid = false;
        if (!m_goal_dir_valid)
            m_goal_type = GOAL_NONE;
    }
    return true;
}

/* a direction as the goal, unless a goal of a higher kind is set: flattened, and made a unit
 * vector unless it came flat and asked to stay so; a position of another kind cleared (SetGoalPos
 * inlined), the facing's dot product with it returned -- none clears the direction, and the kind
 * once no position is left (OVL2 0x56D920; SOCOM 1's, its sqrtf now sqrt.s).
 * OPEN, 8 instructions (the size right): the target reads x again for the scaling after the sqrt
 * (`lwc1 $f0,0x174(a0)` after `mtc1 1.0`), x and z loaded into f1/f2, x*x in the mula, the root in
 * f2; MW here keeps the x it loaded for the length in a register across the sqrt and the test, as
 * the matched CPnt3D::Normalize does in the target (so sqrt.s is no barrier). The sum is written
 * z first, which gets the rest (`x*x + z*z`, SOCOM 1's order, is 18 off: x in f2 kept). Tried: the
 * components by value into a length inline, the scaling or the whole normalisation through an
 * inline taking the point by pointer, a scalar copy of the point, `len = 1.0f / len`, the two
 * scalings each `*= 1.0f / len`. The three-float constructor gives the stores (z loaded first) and
 * m_matrix's elements read straight give the dot's operand order (through operator() they swap) */
float CSealCtrl::SetGoalDir(GOAL_TYPE type, const CPnt3D *dir, bool flat)
{
    float dot = 1.0f;

    if (type < m_goal_type)
        return dot;
    if (dir) {
        m_goal_dir = CPnt3D(dir->x, 0.0f, dir->z);
        if (!flat || dir->y != 0.0f) {
            float len = sqrtf(m_goal_dir.z * m_goal_dir.z + m_goal_dir.x * m_goal_dir.x);

            if (len != 0.0f) {
                float inv = 1.0f / len;

                m_goal_dir.x *= inv;
                m_goal_dir.z *= inv;
            }
        }
        m_goal_dir_valid = true;
        {
            const CMatrix &m = m_entity->m_node->m_matrix;

            dot = -m.m_matrix[2][0] * m_goal_dir.x + -m.m_matrix[2][2] * m_goal_dir.z;
        }
        if (m_goal_pos_valid && m_goal_type != type)
            SetGoalPos(m_goal_type, 0, 0.0f);
        m_goal_type = type;
    } else {
        m_goal_dir_valid = false;
        if (!m_goal_pos_valid)
            m_goal_type = GOAL_NONE;
    }
    return dot;
}

/* the destructor of the network's controller (OVL2 0x56F460: CSealCtrl's called as a base) */
CRemoteCtrl::~CRemoteCtrl()
{
}

/* the mission over: the states dropped, the task's anim stopped, the AI's mode reset (OVL2
 * 0x573B80, SOCOM 1's with CZAnim::Stop's new flag). Every byte matches; ClearAiState resolves
 * once the AI-states lane's 0x5F5F20 is named ClearAiState__11CSealCtrlAiFb (SOCOM 1's call at
 * the same place; verified aliased, build/scratch/seal/adiff.py). */
void CSealCtrlAi::OnMissionEnd()
{
    ClearAiState(true);
    if (m_task_anim) {
        func_002707C0(m_task_anim, false);
        m_task_anim = 0;
    }
    SetAiMode((AI_MODE)2);
}

/* the throttles the AI asks for (OVL2 0x5678F0, slot 0x20: CEntityCtrl's SetThrottle) */
void CSealCtrlAi::SetThrottle(unsigned int axis, float value)
{
    m_throttle_goal[axis] = value;
}

/* the signals (OVL2 0x55ABC0, 0x55A8D0; SOCOM 1's): signal 4 resets the state, the others are
 * taken and dropped -- the compare chain tests 3, 2, 1, 0, then 4, so 4 is written first */
void CAiSAnimate::OnSignal(SIGNAL sig, CSealCtrlAi *ctrl)
{
    switch (sig) {
    case SIGNAL_4:
        m_60 = 0;
        m_5c = 1.0f;
        m_5a = 0;
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

/* the grenade the state holds: its pickup's projectile, while the pickup is still the one the state
 * took (the network id at 0x5C) -- else none (the names ours) */
static inline CZProjectile *GrenadeOf(CAiSGrenade *g)
{
    CPickup *p = (CPickup *)g->m_58;
    CZProjectile *proj = p ? (CZProjectile *)p->m_data : 0;

    if (!proj || !p || g->m_5c != p->GetPickupNetEntityID())
        return 0;
    return proj;
}

/* the animation's timed call that throws the grenade from where it is (its instance's position),
 * with the state's velocity turned into the world, the thrower's node its owner (OVL2 0x55C740) */
extern "C" bool func_0055C740(CAiSAnimate *state, CSealCtrlAi *ai, unsigned int data)
{
    CAiSGrenade *g = (CAiSGrenade *)data;
    CZProjectile *proj = GrenadeOf(g);

    if (proj) {
        CPnt3D vel;

        TransformVectors(&SealOf(ai)->m_node->m_matrix, &g->m_vel, &vel, 1);
        proj->SetPositionVel((CPnt3D *)proj->GetInstance()->m_matrix.m_matrix[3], &vel);
        proj->m_owner = ai->m_entity->GetNode();
    }
    return true;
}

/* the animation's timed call that throws: the grenade from the right hand with the state's
 * velocity turned into the world, the thrower's node its owner (OVL2 0x55C480) */
extern "C" bool func_0055C480(CAiSAnimate *state, CSealCtrlAi *ai, unsigned int data)
{
    CAiSGrenade *g = (CAiSGrenade *)data;
    CZProjectile *proj = GrenadeOf(g);

    if (proj) {
        CZSealBody *seal = SealOf(ai);
        CZBodyPart *hand = seal->m_rhand;
        CMatrix m;
        CPnt3D vel;

        m.SetIdentity();
        hand->m_node->BuildMTW(m);
        TransformVectors(&seal->m_node->m_matrix, &g->m_vel, &vel, 1);
        CPnt3D *pos = (CPnt3D *)m.m_matrix[3];

        proj->SetPositionVel(pos, &vel);
        proj->m_owner = ai->m_entity->GetNode();
    }
    return true;
}

/* the animation's timed calls that put the grenade in the right hand: moving, and at rest (OVL2
 * 0x55C5E0 and 0x55C690; 0x55AC20 pushes them with the grenade state as the word) */
extern "C" bool func_0055C690(CAiSAnimate *state, CSealCtrlAi *ai, unsigned int data)
{
    CZProjectile *proj = GrenadeOf((CAiSGrenade *)data);

    if (proj) {
        CZBodyPart *hand = SealOf(ai)->m_rhand;
        CMatrix m;

        m.SetIdentity();
        hand->m_node->BuildMTW(m);
        proj->SetPositionAtRest((CPnt3D *)m.m_matrix[3]);
    }
    return true;
}

extern "C" bool func_0055C5E0(CAiSAnimate *state, CSealCtrlAi *ai, unsigned int data)
{
    CZProjectile *proj = GrenadeOf((CAiSGrenade *)data);

    if (proj) {
        CZBodyPart *hand = SealOf(ai)->m_rhand;
        CMatrix m;

        m.SetIdentity();
        hand->m_node->BuildMTW(m);
        proj->SetPosition((CPnt3D *)m.m_matrix[3]);
    }
    return true;
}

extern "C" {
/* (OVL2 0x57E540, seal_body.cpp's span: a Navy SEAL's call when its grenade is about to go off) */
void func_0057E540(CZSealBody *seal);
}

/* CAiSGrenade's slot 0x14 (OVL2 0x55C330, new in SOCOM II): done once its grenade is gone; while
 * held, the path's first cell kept as the target cell, and a Navy SEAL told when the fuse has under
 * half a second left */
int CAiSGrenade::Virtual14(CSealCtrlAi *ai, CAiState *sub)
{
    CZSealBody *seal = SealOf(ai);
    CZProjectile *proj = GrenadeOf(this);

    if (!proj)
        m_active = false;
    else {
        CAiPath *path = (CAiPath *)&ai->m_path;

        if (path->HasPoints())
            m_80 = path->m_points.begin()->m_loc;
        if (proj->m_time < 0.5f && ai->IsNavySeal())
            func_0057E540(seal);
    }
    return Result();
}

/* (OVL2 0x55CCD0, new in SOCOM II's form: SOCOM 1's cleared one byte on signal 0) signal 4 resets
 * the state, its two cells made the invalid one; signal 0 clears the byte at 0x60; signal 1 has a
 * prone SEAL (or one about to go prone) crouch after a random delay -- ChangeStanceDelayed inlined */
void CAiSGrenade::OnSignal(SIGNAL sig, CSealCtrlAi *ctrl)
{
    switch (sig) {
    case SIGNAL_4:
        m_60 = 0;
        m_58 = 0;
        m_5c = 0;
        m_6c = 0;
        m_7c = CAiMapLoc::invalid;
        m_80 = m_7c;
        break;
    case SIGNAL_0:
        m_60 = 0;
        break;
    case SIGNAL_1:
        ctrl->ChangeStanceDelayed(STANCE_PRONE, STANCE_CROUCH, -1.0f);
        break;
    case SIGNAL_2:
        break;
    case SIGNAL_3:
        break;
    }
}

void CAiSPursue::OnSignal(SIGNAL sig, CSealCtrlAi *ctrl)
{
    switch (sig) {
    case SIGNAL_4:
        m_timer = -1.0f;
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

/* hurt: the SEAL's other sounds cut off and the pain cry, always (OVL2 0x571AB0; SOCOM 1's with
 * SOCOM II's extra arguments). The entity is read ahead of the test, as both builds do: only a
 * local taken first gives that `lw` its place before the branch. */
void CSealCtrlAi::OnSetHealth(float health, float old)
{
    CEntity *e = m_entity;

    if (old > health) {
        CSndInstance::Remove(e->m_id, true, true, false);
        PlayAiSound((CHARACTER_SOUND)59, 2.0f, -1);
    }
}

/* the AI's names (SOCOM 1's namespace sealai) */
namespace sealai {
/* a stance's name (inline everywhere in SOCOM II: no out-of-line copy follows get_stance(name),
 * where SOCOM 1's sat) */
inline const char *get_stance(SEAL_STANCE stance)
{
    switch (stance) {
    case STANCE_STAND:
        return "stand";
    case STANCE_CROUCH:
        return "crouch";
    case STANCE_PRONE:
        return "prone";
    case STANCE_IDLE:
        return "idle";
    }
    return "none";
}

SEAL_STANCE get_stance(const char *name);
const char *get_aiseq(unsigned int i);
}

/* the sequence an alertness band starts (OVL2 0x575210; SOCOM 1's) */
const char *sealai::get_aiseq(unsigned int i)
{
    switch (i) {
    case 0:
        return "on_aware0";
    case 1:
        return "on_aware1";
    case 2:
        return "on_aware2";
    case 3:
        return "on_aware3";
    case 4:
        return "on_aware4";
    }
    return "on_awareX";
}

/* the stance of a name, or none (OVL2 0x575100; SOCOM 1's) */
SEAL_STANCE sealai::get_stance(const char *name)
{
    for (int i = 0; name && i < 4; i++)
        if (!strcasecmp(name, get_stance((SEAL_STANCE)i)))
            return (SEAL_STANCE)i;
    return STANCE_NONE;
}

namespace sealai {
const char *get_state(AI_STATE state);
AI_STATE get_state(const char *name);
}

/* the state of a name, or 0 (OVL2 0x575410; SOCOM 1's: the thirty names through get_state(AI_STATE),
 * OVL2 0x575490) */
AI_STATE sealai::get_state(const char *name)
{
    if (name)
        for (int i = 0; i < 30; i++) {
            AI_STATE state = (AI_STATE)i;

            if (!strcasecmp(get_state(state), name))
                return state;
        }
    return AI_STATE_UNKNOWN;
}

/* a flag's name (inline: no out-of-line copy; the names the strings', the function ours) */
static inline const char *get_flag(unsigned char flag)
{
    switch (flag) {
    case 1:
        return "alignbody";
    case 2:
        return "nobodyrot";
    case 4:
        return "scan";
    case 8:
        return "randomscan";
    case 16:
        return "weaponup";
    case 32:
        return "weaponready";
    case 64:
        return "headbob";
    case 128:
        return "modified";
    }
    return "unknown";
}

/* the flag of a name, or none (OVL2 0x5752B0, new in SOCOM II: between get_aiseq and
 * get_state(name); 0x5D79F0 asks it twice) */
extern "C" unsigned char func_005752B0(const char *name)
{
    if (name)
        for (int i = 0; i < 8; i++) {
            unsigned char flag = 1 << i;

            if (!strcasecmp(name, get_flag(flag)))
                return flag;
        }
    return 0;
}

/* a state's name (OVL2 0x575490; SOCOM 1's, SOCOM II's thirty states in its case order) */
const char *sealai::get_state(AI_STATE state)
{
    switch (state) {
    case 0:
        return "unknown";
    case 11:
        return "action";
    case 13:
        return "animate";
    case 3:
        return "avoid";
    case 10:
        return "breach";
    case 14:
        return "cqb";
    case 4:
        return "deploy";
    case 25:
        return "event";
    case 15:
        return "flee";
    case 9:
        return "follow";
    case 8:
        return "goto";
    case 27:
        return "grenade";
    case 23:
        return "hostage";
    case 2:
        return "hold";
    case 5:
        return "lasing";
    case 21:
        return "noammo";
    case 24:
        return "plan";
    case 16:
        return "pathreco";
    case 22:
        return "pickup";
    case 12:
        return "pursue";
    case 20:
        return "reload";
    case 17:
        return "rescued";
    case 19:
        return "shotat";
    case 1:
        return "script";
    case 28:
        return "stunned";
    case 6:
        return "surrender";
    case 18:
        return "suspended";
    case 26:
        return "targeting";
    case 7:
        return "vehicle";
    case 29:
        return "rush";
    default:
        return "undef";

    }
}

/* a role's name (inline: no out-of-line copy; the strings' names, the function ours) */
static inline const char *get_role(unsigned char role)
{
    switch (role) {
    case 0:
        return "unknown";
    case 1:
        return "sniper";
    case 2:
        return "gunner";
    }
    return "error";
}

/* the role of a name, or none (OVL2 0x5756B0, new in SOCOM II: after get_state(AI_STATE)) */
extern "C" unsigned char func_005756B0(const char *name)
{
    for (int i = 0; name && i < 3; i++)
        if (!strcmp(name, get_role(i)))
            return i;
    return 0;
}

namespace sealai {
/* a mode's name (inline everywhere in SOCOM II; the cases in the order its table's blocks lie) */
inline const char *get_mode(AI_MODE mode)
{
    switch (mode) {
    case 1:
        return "idle";
    case 2:
        return "hold";
    case 4:
        return "creep";
    case 5:
        return "slide";
    case 6:
        return "follow";
    case 3:
        return "path";
    case 0:
        return "sleep";
    case 7:
        return "climb";
    case 8:
        return "test";
    case 9:
        break;
    }
    return "error";
}

AI_MODE get_mode(const char *name);
}

/* the mode of a name, or idle (OVL2 0x575790; SOCOM 1's, with nine modes) */
AI_MODE sealai::get_mode(const char *name)
{
    for (int i = 0; name && i < 9; i++)
        if (!strcmp(name, get_mode((AI_MODE)i)))
            return (AI_MODE)i;
    return (AI_MODE)1;
}

/* a walk over map cells: its map, the cell it starts from, the cell at hand and its centre (0x28),
 * and what the walk gathers -- the kind it looks for, a point, a found flag and cell, a count (the
 * names ours) */
struct CCellWalk {
    void *m_map;   // 0x0
    CAiMapLoc m_start;   // 0x4
    unsigned char _8[0xc - 0x8];
    CAiMapLoc m_loc;   // 0xc
    unsigned char _10[0x28 - 0x10];
    CPnt3D m_center;   // 0x28
};

struct CCellGather {
    unsigned char m_kind;   // 0x0
    CPnt3D m_pos;   // 0x4
    unsigned char m_found;   // 0x10
    CAiMapLoc m_loc;   // 0x14
    int m_count;   // 0x18
};

extern "C" {
/* (OVL2 0x511F80 and 0x52D280, the AI core's: a map's cell at a location, and a cell's kind) */
void *func_00511F80(void *map, CAiMapLoc *loc, int a);
int func_0052D280(void *cell);
}

/* a cell's kind, none off the map (an inline returning a byte: each return narrows on its own) */
static inline unsigned char CellKind(void *map, CAiMapLoc *loc)
{
    void *cell = func_00511F80(map, loc, 0);

    if (cell)
        return func_0052D280(cell);
    return 0;
}

/* a cell walk's step: a cell not of the kind sought takes the point and counts (its cell kept at the
 * fourth); one of the kind marks the start found once; the walk goes on while the kinds differ (OVL2
 * 0x556280, new in SOCOM II; a callback).
 * OPEN, 2 instructions: the target narrows the kind into v1 in each arm (`andi v1,v0,0xff` before the
 * then-arm's branch, past the else's own); MW here narrows the call's result into v0 and branches to
 * the else's narrowing. Tried: the kind as an if/else, a ternary (with and without a cast), zero
 * first then the call, the inline returning a byte or an int, the cast inside the compare. */
extern "C" bool func_00556280(CCellWalk *w, CCellGather *g)
{
    bool same = false;
    unsigned char kind = CellKind(w->m_map, &w->m_loc);

    if (g->m_kind == kind)
        same = true;
    if (!same) {
        g->m_pos = w->m_center;
        if (g->m_count == 3) {
            g->m_found = 1;
            g->m_loc = w->m_loc;
        }
        g->m_count++;
    } else if (!g->m_found) {
        g->m_loc = w->m_start;
        g->m_found = 1;
    }
    return !same;
}

/* an alert's level record (the names ours: 0x558030, 0x5582C0 and 0x558440 keep one each) */
struct CAlertLevel {
    float m_time;   // 0x0: when the level was raised, on the game's clock
    float m_amount;   // 0x4
    unsigned char m_level;   // 0x8
    unsigned char _9[0xc - 0x9];
    int m_c;   // 0xc: cleared on any change
    unsigned char _10[0x14 - 0x10];
    AI_MODE m_mode;   // 0x14: the AI's mode at the change, 1 without an AI
};

extern "C" {
/* how fast a level decays (OVL2 data 0x6504C0, read as a variable) */
extern float data_006504C0;
}

/* a level raised: the same one adds to the amount (2), a higher one takes over (1); level 0 decays
 * the amount and drops the level at zero (1); any change clears the word at 0xC (OVL2 0x557F60, new
 * in SOCOM II; the name unknown) */
/* what a raise did (a byte enum: assigned with li, its test narrowed; the names ours) */
enum ALERT_CHANGE { ALERT_NONE = 0, ALERT_SET = 1, ALERT_ADDED = 2 };

extern "C" ALERT_CHANGE func_00557F60(CAlertLevel *a, CSealCtrlAi *ai, unsigned char level, float amount)
{
    ALERT_CHANGE r = ALERT_NONE;

    if (a->m_level == level) {
        a->m_amount += amount;
        a->m_mode = ai ? ai->m_3f4 : (AI_MODE)1;
        r = ALERT_ADDED;
    } else if (level == 0) {
        a->m_amount -= amount * data_006504C0;
        if (a->m_amount <= 0.0f) {
            a->m_amount = 0.0f;
            r = ALERT_SET;
            a->m_level = 0;
        }
    } else if (a->m_level < level) {
        a->m_level = level;
        a->m_amount = amount;
        a->m_time = data_004365C0;
        a->m_mode = ai ? ai->m_3f4 : (AI_MODE)1;
        r = ALERT_SET;
    }
    if (r != ALERT_NONE)
        a->m_c = 0;
    return r;
}

/* whether a map's cell (its first layer's) lies inside: CAiMap::GetCell inlined one level, its pieces
 * the calls ai.h names (OVL2 0x5561C0, new in SOCOM II: the grenade state's Tick, 0x565460 and the
 * AI states call it; the name unknown) */
extern "C" bool func_005561C0(const CAiMap *map, const CAiMapLoc &loc)
{
    CAiCell *cell = map->GetCell(loc, 0);

    return cell ? cell->IsInside() : false;
}

extern "C" {
/* (OVL2 0x52D950, the AI core's: the AI maps' cell at a location) */
void *func_0052D950(CAiMaps *maps, const CAiMapLoc *loc);
}

/* whether a location is the invalid cell (an inline bool: the target materialises the `&&`) */
static inline bool IsInvalidLoc(const CAiMapLoc *loc)
{
    return loc->x == CAiMapLoc::invalid.x && loc->y == CAiMapLoc::invalid.y;
}

/* the cell the AI heads for, with a count (14 for none): the invalid cell clears both and succeeds,
 * any other must be on the maps (OVL2 0x56D450, new in SOCOM II; the AI states' 0x5D4770 calls it) */
extern "C" bool func_0056D450(CSealCtrlAi *ctrl, const CAiMapLoc *loc, int n)
{
    if (!n)
        n = 14;
    bool ok = false;

    if (IsInvalidLoc(loc)) {
        ctrl->m_274 = 0;
        ok = true;
        ctrl->m_270 = 0;
    } else {
        ctrl->m_270 = 0;
        ctrl->m_274 = func_0052D950(&AiMaps, loc);
        if (ctrl->m_274) {
            ctrl->m_270 = n;
            ok = true;
        }
    }
    return ok;
}

/* a box's eight corners (the name ours) */
struct CBoxCorners {
    CPnt3D m_c[8];
};

extern "C" {
/* OVL2's out-of-line copies of CPnt3D's three-float constructor and its implicit copy constructor
 * (0x5414F0, 0x539E90) */
CPnt3D *func_005414F0(CPnt3D *p, float x, float y, float z);
CPnt3D *func_00539E90(CPnt3D *p, const CPnt3D *src);
}

/* a box's eight corners, each a temporary point copied in -- the lows and highs mixed, the two
 * plain corners copied straight from the box (OVL2 0x56AE40, new in SOCOM II; 0x56A8E0 calls it) */
extern "C" CBoxCorners *func_0056AE40(CBoxCorners *c, const CBBox *b)
{
    CPnt3D t0, t1, t2, t3, t4, t5;

    func_00539E90(&c->m_c[0], func_005414F0(&t0, b->m_min.x, b->m_min.y, b->m_max.z));
    func_00539E90(&c->m_c[1], &b->m_min);
    func_00539E90(&c->m_c[2], func_005414F0(&t1, b->m_max.x, b->m_min.y, b->m_min.z));
    func_00539E90(&c->m_c[3], func_005414F0(&t2, b->m_max.x, b->m_min.y, b->m_max.z));
    func_00539E90(&c->m_c[4], func_005414F0(&t3, b->m_min.x, b->m_max.y, b->m_max.z));
    func_00539E90(&c->m_c[5], func_005414F0(&t4, b->m_min.x, b->m_max.y, b->m_min.z));
    func_00539E90(&c->m_c[6], func_005414F0(&t5, b->m_max.x, b->m_max.y, b->m_min.z));
    func_00539E90(&c->m_c[7], &b->m_max);
    return c;
}

extern "C" {
/* (OVL2 0x5712F0, seal_ai.cpp's: whether the controller's SEAL should act now; and 0x589AA0, the
 * body's: the act) */
int func_005712F0(CSealCtrlAi *ctrl);
void func_00589AA0(CZSealBody *seal);
}

/* the controller's SEAL made to act when it should, saying whether it did (OVL2 0x571100, new in
 * SOCOM II; the AI states' 0x60DFC0 asks it) */
extern "C" bool func_00571100(CSealCtrlAi *ctrl)
{
    bool acted = false;   /* declared first: the SEAL takes s0, the flag s1 */
    CZSealBody *seal = (CZSealBody *)ctrl->m_entity;

    if (func_005712F0(ctrl)) {
        func_00589AA0(seal);
        acted = true;
    }
    return acted;
}

/* CEntityCtrl::SetThrottle's out-of-line copy in OVL2 (OVL1's is 0x2A26E0, ent_ctrl.cpp's, byte for
 * byte the same: m_throttle[axis] = value): each overlay links its own copy of an inline virtual, and
 * this one sits after CSealCtrl::CreepTick in seal_ai.cpp's span (OVL2 0x567340) */
extern "C" void func_00567340(CEntityCtrl *ctrl, unsigned int axis, float value)
{
    ctrl->m_throttle[axis] = value;
}

/* the template tail's other two instances, explicit until their callers are written: ai::_min over
 * floats (OVL2 0x575C80: CSealCtrl::CreepTick 0x566DC0 and 0x567900 call it) and _close_enough over
 * ints (OVL2 0x575CB0: 0x567900 calls it twice, as UpdateAiMapLoc does) -- _max<f> (0x575C50), the
 * tail's first, comes with seal_control.cpp's caller */
template const float &ai::_min<float>(const float &, const float &);
template bool _close_enough<int>(const int &, const int &, const int &);
