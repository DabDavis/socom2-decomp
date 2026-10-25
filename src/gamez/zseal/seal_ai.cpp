/* seal_ai.cpp -- the AI's controller, CSealCtrlAi, and its states (SOCOM 1's unit,
 * __sinit_seal_ai.cpp; SOCOM 1's order: the states' small virtuals and CAiSRescued first, the
 * controller's constructor and the template tail -- ai::_max, ai::_min, _close_enough<i> -- last).
 * OVL2 0x5552C0-0x575CE7. Defined in reverse of their image order (-inline deferred). */

#include "zseal.h"
#include "../../ai/ai.h"
#include "../zsnd/zsnd.h"
#include "../zanim/zanim.h"
#include <stdio.h>
#include <string.h>

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
    virtual AI_RESULT Virtual14(CSealCtrlAi *ai, int x);
    virtual bool Virtual44(AI_STATE state) { return state == m_state; }
    virtual bool Virtual48(int x, CAiState *other);
    virtual ~CAiSPursue();

    float m_timer;   // 0x58 (SOCOM 1's 0x28)
    signed char m_5c;   // 0x5c (the name ours)
};

class CAiSAnimate : public CAiState {
public:
    virtual void OnSignal(SIGNAL sig, CSealCtrlAi *ctrl);
    /* (0x5759A0, matched as a draft: two lists at 0x64 and 0x70 destroyed, each with its own
     * erase -- declared so this unit emits no implicit one) */
    virtual ~CAiSAnimate();

    unsigned char _58[0x5a - 0x58];
    unsigned char m_5a;   // 0x5a (SOCOM 1's 0x28)
    float m_5c;   // 0x5c (SOCOM 1's 0x30)
    unsigned int m_60;   // 0x60 (SOCOM 1's 0x8)
};

class CAiSGrenade : public CAiState {
public:
    virtual ~CAiSGrenade();
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
    m_3f4 = 1;
}

/* how far an event's kind carries (SOCOM 1's static CAiEvent::GetEventRange, ai.h's class lacks it:
 * OVL2 0x50F4C0 by its address) */
extern "C" float func_0050F4C0(CAiEvent::EVENT event);
/* CSealCtrl::CreateAiEvent (SOCOM 1's CreateAiEvent__9CSealCtrlFQ28CAiEvent5EVENTffbPC6CPnt3DUi by
 * its order and its call here; OVL2 0x572FA0, called by its address) */
extern "C" int func_00572FA0(CSealCtrl *ctrl, CAiEvent::EVENT event, float a, float b, bool c,
                             const CPnt3D *pos, unsigned int d);
/* the voice's reach scale (OVL2 data 0x650380, read as a variable) */
extern "C" float data_00650380;

/* the voice's AI event made where the SEAL stands, unless it has none; the event returned (OVL2
 * 0x55FEE0; PlayAiSound calls it) */
extern "C" int func_0055FEE0(CSealCtrl *ctrl, CAiEvent::EVENT event)
{
    int r = 0;

    if (event != CAiEvent::AIE_NONE) {
        float range = func_0050F4C0(event);

        r = func_00572FA0(ctrl, CAiEvent::AIE_VOICE, data_00650380, range, false, &ctrl->m_entity->m_posW, event);
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

    if (s->m_state == (AI_STATE)2 && (s->m_flags & 0x80))
        r = true;
    return r;
}

/* whether the AI's current state is of kind 2 with flag 0x80 (OVL2 0x559000).
 * OPEN, 11 instructions: the stack's test is the target's as an if/else (the zero arm first: a
 * ternary or an if-assign lays the call first), but MW if-converts the inlined flag test (xori,
 * sltiu, sltu) where the target branches and assigns the true with daddiu. Tried: the test written
 * out in the function, a single `&&` chain with a result local. */
extern "C" bool func_00559000(CSealCtrlAi *ai)
{
    CAiState *s;

    if (ai->m_41c == 0)
        s = 0;
    else
        s = func_005F8330(&ai->m_41c);

    if (s)
        return IsKind2Flag80(s);
    return false;
}

/* the state at 0x4EC: 4 with the word at 0x508 set (an inline: the test materialised, the word
 * as MW's `!(x == 0)`) */
static inline bool IsState4Held(CSealCtrlAi *ai)
{
    return !(ai->m_508 == 0) && ai->m_4ec == 4;
}

/* free: in none of states 1-3 nor a held 4 (OVL2 0x5629B0). States 1-3 are one unsigned range
 * test written out, as the target's `(t - 1) < 3`: three `==` joined by `||` fold only two */
extern "C" bool func_005629B0(CSealCtrlAi *ai)
{
    if (IsState4Held(ai) || (unsigned int)(ai->m_4ec - 1) < 3)
        return false;
    return true;
}

/* the byte at 0x4ED, or 0 in states 1-3 or a held 4 (OVL2 0x562960; the range and the inline
 * as 0x5629B0's) */
extern "C" signed char func_00562960(CSealCtrlAi *ai)
{
    if ((unsigned int)(ai->m_4ec - 1) < 3)
        return 0;
    if (IsState4Held(ai))
        return 0;
    return ai->m_4ed;
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

/* the AI's state brought up to date, the route's state returned (OVL2 0x5558A0; the AI states'
 * OnSignal call it) */
extern "C" AI_ROUTE func_005558A0(CSealCtrlAi *ai)
{
    func_00555420(ai, (char *)ai + 0x46c, ai->m_path);
    return ai->m_route;
}

/* (OVL2 0x527310, the AI-core lane's: a CAiPath's plan aborted and its lists cleared) */
extern "C" bool func_00527310(void *path);

/* the route reset, and its state set: a running one (1 or 2) is ended first (OVL2 0x5557B0; the
 * path-recovery state calls it) */
extern "C" void func_005557B0(CSealCtrlAi *ai, AI_ROUTE route)
{
    func_00527310(ai->m_path);
    switch (ai->m_route) {
    case ROUTE_1:
    case ROUTE_2:
        ai->m_route = ROUTE_NONE;
        break;
    }
    ai->m_route = route;
}

/* the route reset; a running one ends as 2 or none (OVL2 0x555820; the script and goto states'
 * signals call it) */
extern "C" void func_00555820(CSealCtrlAi *ai, bool keep)
{
    func_00527310(ai->m_path);
    switch (ai->m_route) {
    case ROUTE_1:
    case ROUTE_2:
        ai->m_route = keep ? ROUTE_2 : ROUTE_NONE;
        break;
    }
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

/* CAiSPursue's slot 0x14 (new in SOCOM II): the pursuit stops unless the AI's object says so,
 * and the Tick's result (OVL2 0x55A760). ai_states.h's Result() gives the target's `movz` and the
 * byte enum's `andi` -- the earlier spelling as a status computed here was the miss */
AI_RESULT CAiSPursue::Virtual14(CSealCtrlAi *ai, int x)
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
 * the goal's timer stopped (OVL2 0x56DCA0; SOCOM 1's, SetGoalPos inlined).
 * OPEN, 2 instructions (0xB0 against 0xA8): the target puts the inlined `type < m_goal_type` test
 * in the `m_goal_type == type` branch's delay slot as `slt at` and branches on it; here MW
 * computes it into v0 after a nop and leaves a nop before the call's join. The same with the
 * position's clearing written out (`if (type >= m_goal_type) {...}`), `(void)`, or an early
 * return for the outer test -- the out-of-line SetGoalPos, the same test, matches. */
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
    if (type < m_goal_type)
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
