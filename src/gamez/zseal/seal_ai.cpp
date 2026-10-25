/* seal_ai.cpp -- the AI's controller, CSealCtrlAi, and its states (SOCOM 1's unit,
 * __sinit_seal_ai.cpp; SOCOM 1's order: the states' small virtuals and CAiSRescued first, the
 * controller's constructor and the template tail -- ai::_max, ai::_min, _close_enough<i> -- last).
 * OVL2 0x5552C0-0x575CE7. Defined in reverse of their image order (-inline deferred). */

#include "zseal.h"
#include "../zsnd/zsnd.h"

/* the states this unit defines (SOCOM 1's; only what their destructors need -- none of them has
 * a member with a destructor of its own, so each destructor is the base's, inlined) */
class CAiSRescued : public CAiState {
public:
    virtual ~CAiSRescued();
};

class CAiSPursue : public CAiState {
public:
    virtual void OnSignal(SIGNAL sig, CSealCtrlAi *ctrl);
    virtual ~CAiSPursue();

    float m_timer;   // 0x58 (SOCOM 1's 0x28)
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
 * OPEN, 0x64 against 0x6C: the target tests the stack's size with `bnez` to the call, the zero in
 * the delay slot and a `b` to the join (the empty arm laid out first); the ternary either way
 * round, an if-assign and an inline accessor (early return or a result local) all lay the call
 * first and fall through. */
extern "C" bool func_00559000(CSealCtrlAi *ai)
{
    CAiState *s = ai->m_41c ? func_005F8330(&ai->m_41c) : 0;

    if (s)
        return IsKind2Flag80(s);
    return false;
}

/* the state at 0x4EC: 4 with the word at 0x508 set (an inline: the test materialised) */
static inline bool IsState4Held(CSealCtrlAi *ai)
{
    return ai->m_508 != 0 && ai->m_4ec == 4;
}

/* free: in none of states 1-3 nor a held 4 (OVL2 0x5629B0).
 * OPEN, 0x68 against 0x58: the target materialises `m_508 != 0` as `xor; sltiu; xori` (MW's
 * `!(x == 0)`) and folds states 1-3 into one `sltiu at,3`; here `sltu` and only 1-2 fold, 3 tested
 * apart. Tried: `!(m_508 == 0)`, a bool local, a ternary, the whole test one `||` chain. */
extern "C" bool func_005629B0(CSealCtrlAi *ai)
{
    if (IsState4Held(ai))
        return false;
    if (ai->m_4ec == 1 || ai->m_4ec == 2 || ai->m_4ec == 3)
        return false;
    return true;
}

/* the byte at 0x4ED, or 0 in states 1-3 or a held 4 (OVL2 0x562960).
 * OPEN, 0x58 against 0x50: the target folds `t == 1 || t == 2 || t == 3` into one `(t - 1) < 3`
 * (sltiu at); MW folds 1-2 into a range and tests 3 apart. Tried the tests reversed, `t >= 1 && t
 * <= 3`, `(unsigned char)(t - 1) < 3`. */
extern "C" signed char func_00562960(CSealCtrlAi *ai)
{
    if (ai->m_4ec == 1 || ai->m_4ec == 2 || ai->m_4ec == 3)
        return 0;
    if (ai->m_508 && ai->m_4ec == 4)
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

/* (OVL2 0x5621C0) */
extern "C" void *func_005621C0(CSealCtrlAi *ai, unsigned char i)
{
    if (i < 3)
        return ai->m_750[i];
    return 0;
}

/* the AI's state brought up to date, its byte at 0x5BC returned (OVL2 0x5558A0; the AI states'
 * OnSignal call it) */
extern "C" signed char func_005558A0(CSealCtrlAi *ai)
{
    func_00555420(ai, (char *)ai + 0x46c, (char *)ai + 0x514);
    return ai->m_5bc;
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
