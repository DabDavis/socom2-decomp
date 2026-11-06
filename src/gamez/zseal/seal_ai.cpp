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

/* THE UNIT'S DYNAMICALLY INITIALISED STATICS, in the order its initialiser builds them
 * (__sinit_seal_ai.cpp, OVL2 0x6687E0): the five slices (zseal.h's CAiSlice) */
CAiSlice ai_slice0;
CAiSlice ai_slice1;
CAiSlice ai_slice2;
CAiSlice ai_slice3;
CAiSlice ai_slice4;

/* the states this unit defines (SOCOM 1's; only what their destructors need -- none of them has
 * a member with a destructor of its own, so each destructor is the base's, inlined) */
class CAiSRescued : public CAiState {
public:
    virtual void OnSignal(SIGNAL sig, CSealCtrlAi *ctrl);
    virtual int Tick(CSealCtrlAi *ai);
    virtual ~CAiSRescued();
    void SetExtractValve(CValve *valve);

    /* 0x58 (SOCOM 1's 0x28): the extraction region the SEAL was rescued into -- led to the cell its
     * name gives once the valve opens (PreTick's, from GetRegionTypeContainer(1, ...)) */
    CAiMap::region *m_58;
    unsigned int m_5c;   // 0x5c
    CValve *m_60;   // 0x60: the extraction valve (SetExtractValve's)
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
    /* (a constructor of its own: the list's insert, 0x575E30, copies the value member by member --
     * the time through an FPR, the call and the word through GPRs -- where a plain struct is one
     * block through the FPRs) */
    ANIM_TIMED_CALL(float time, ANIM_CALL func, unsigned int data) : m_time(time), m_func(func), m_data(data) {}

    float m_time;   // 0x0
    ANIM_CALL m_func;   // 0x4
    unsigned int m_data;   // 0x8
};

struct ANIM_SPAN_CALL {
    ANIM_SPAN_CALL(float start, float end, ANIM_CALL func, unsigned int data)
        : m_start(start), m_end(end), m_func(func), m_data(data)
    {
    }

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

    virtual int Tick(CSealCtrlAi *ai);

    AnimType m_anim;   // 0x58: the anim played, or -1 (SOCOM 1's at 0x2c)
    unsigned char m_5a;   // 0x5a (SOCOM 1's 0x28): the state the anim's blend is pushed in
    float m_5c;   // 0x5c (SOCOM 1's 0x30)
    /* how long the anim may hold the state when flag 1 says so, on the state stack's clock (SOCOM
     * 1's 0x8 -- a countdown there) */
    float m_60;   // 0x60
    CAnimTimedCallList m_timed;   // 0x64
    CAnimSpanCallList m_spans;   // 0x70
};

class CAiSGrenade : public CAiState {
public:
    virtual void OnSignal(SIGNAL sig, CSealCtrlAi *ctrl);
    virtual int Virtual14(CSealCtrlAi *ai, CAiState *sub);
    /* slots 0x2C and 0x3C of its table, both false (OVL2 0x575B30, 0x575B40: the tail's stubs) */
    virtual bool CanPursue(CSealCtrlAi *ai) const { return false; }
    virtual bool Virtual3C(CSealCtrlAi *ai) { return false; }
    virtual ~CAiSGrenade();

    virtual int Tick(CSealCtrlAi *ai);

    /* (the names ours: what OnSignal's reset clears) */
    unsigned int m_58;   // 0x58: the grenade's pickup
    unsigned int m_5c;   // 0x5c: its network id when taken
    unsigned char m_60;   // 0x60
    unsigned char _61[0x6c - 0x61];
    /* where the grenade will land, once 0x55CDC0 has found it, and its cell (Tick; the names ours) */
    unsigned char m_6c;   // 0x6c
    unsigned char _6d[0x70 - 0x6d];
    CPnt3D m_70;   // 0x70
    CAiMapLoc m_7c;   // 0x7c
    CAiMapLoc m_80;   // 0x80: the cell the dodge's plan heads for
    /* the step after the run to the grenade: 4 the kick, 5 the throw back, 6 none (compared with
     * lbu; Tick's copies to its step reload it with lb -- open there; the name ours) */
    unsigned char m_84;   // 0x84
    unsigned char _85[0xa0 - 0x85];
    /* the throw's velocity in the SEAL's frame (0x55C480 turns it by the body's matrix; the name
     * ours) */
    CPnt3D m_vel;   // 0xa0
};

class CAiSDeploy : public CAiState {
public:
    virtual void OnSignal(SIGNAL sig, CSealCtrlAi *ai);
    /* slots 0x24 and 0x3C of its table, both false (OVL2 0x575BE0, 0x575BD0: the tail's stubs) */
    virtual bool CanAttack() const { return false; }
    virtual bool Virtual3C(CSealCtrlAi *ai) { return false; }
    /* slot 0x1C: no state pushed over a deploy (OVL2 0x55E660, out of line: mid-unit, after
     * OnSignal) */
    virtual bool CanPushAiState(AI_STATE state, CSealCtrlAi *ai);
    virtual ~CAiSDeploy();

    /* the point the deploy is made at and the aim it is made with (the grenade's thrower 0x564D20 sets
     * both; the names ours) */
    CPnt3D m_58;   // 0x58
    CPnt3D m_64;   // 0x64
    /* what signal 4 resets (the names ours; 0x564D20 puts the item at 0x70) */
    unsigned int m_70;   // 0x70
    CAiMapLoc m_74;   // 0x74: a cell, the invalid one when reset
    unsigned char _78[0x7c - 0x78];
    unsigned char m_7c;   // 0x7c: 2 when reset
    unsigned char _7d[0x80 - 0x7d];
    unsigned int m_80;   // 0x80
    unsigned int m_84;   // 0x84
    float m_88;   // 0x88: a wait 0x564D20 sets for a launcher's round (the name ours)
    unsigned int m_8c;   // 0x8c
    float m_timer;   // 0x90: a weapon timer (CZWeapon::ClearTimer clears it)
    unsigned int m_94;   // 0x94
    unsigned int m_98;   // 0x98
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

/* the extraction valve given: kept, and flag 1 set with one, cleared without (OVL2 0x559470; SOCOM 1's
 * SetExtractValve, between Tick and OnSignal in both builds -- SOCOM II's also marks the flags dirty) */
void CAiSRescued::SetExtractValve(CValve *valve)
{
    m_60 = valve;
    SetFlags(valve != 0, !(valve != 0));
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
/* (the kit's fire mode set is CZKit::SetCurFireMode, zseal.h's; OVL2 0x5C37B0) the target's word
 * at 0x44 copied to 0x40 when asked (OVL2 0x60A2D0, the AI states' range) */
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
            ai->m_fire.m_spread = 0;
            func_0060A2D0(ai->m_67c, true);
            kit->SetCurFireMode(AUTOFIRE);
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
    CEntity *m_entity;   // 0x0 (Dump compares its targets' with it)
    unsigned char _4[0x2c - 0x4];
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

/* the entity as a SEAL, if it is one (an inline, the name ours) */
static inline CZSealBody *AsSeal(CEntity *e)
{
    return e && e->m_type == CEntity::ENT_TYPE_SEAL ? (CZSealBody *)e : 0;
}

/* the controller's dump: its SEAL's teams written into a line, then each target's SEAL asked for
 * its AI and marked when it is this AI's target -- nothing printed (OVL2 0x570E00, slot 0x68;
 * SOCOM 1's walked the targets and asked their AIs alone) */
void CSealCtrl::Dump()
{
    char line[128];
    char buf[128];
    CEntity *e = m_entity;

    sprintf(line, "  Team: ");
    for (int i = 0; i < 32; i++) {
        if ((1 << i) & e->m_team) {
            sprintf(buf, "%d ", i);
            strcat(line, buf);
        }
    }
    CTargetList *list = &e->m_targets;
    CSealCtrlAi *ai = GetSealCtrlAi();

    for (CTargetList::iterator t = list->begin(); t != list->end(); ++t) {
        CZSealBody *seal = AsSeal(t->m_ent);

        if (seal)
            seal->GetSealCtrlAi();
        if (ai && ((CAiTargetFlags *)ai->m_67c)->m_entity == t->m_ent)
            sprintf(buf, "* ");
        else
            sprintf(buf, "  ");
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
        SetRange6F4(ai, ai->m_aiParams.courage);
}

extern "C" {
/* (OVL2 0x572190, below: whether the controller is busy -- auto-inlined into the grenade choice) */
bool func_00572190(CSealCtrlAi *ai);
/* (OVL2 0x565460: whether a grenade of the item reaches the point -- unwritten) */
bool func_00565460(CSealCtrlAi *ai, EQUIP_ITEM item, const CPnt3D *pos);
/* (OVL2 0x564D20: the grenade thrown at the point -- unwritten; CSealCtrlAi::OnCommand and the
 * breach's Tick call it too) */
bool func_00564D20(CSealCtrlAi *ai, EQUIP_ITEM item, const CPnt3D *target, const CPnt3D *from,
    unsigned int flags);
/* (OVL2 data: how long the AI waits after a grenade before choosing another, read as a variable) */
extern float data_0064FF80;
/* (OVL1 data: the game's clock, as this unit reads it) */
extern float data_004365C0;
}

/* the grenades the AI chooses by its target's range: the item and the squared ranges it covers (OVL2
 * data 0x650210, eight: 121 and 126 from 50 to 600, the others from 100 to 600; the names ours) */
struct AI_GRENADE_RANGE {
    EQUIP_ITEM m_item;   // 0x0
    float m_min;   // 0x4
    float m_max;   // 0x8
};
extern "C" AI_GRENADE_RANGE data_00650210[8];

/* an entity's position, or none (an inline: the target's point copied out of it) */
static inline const CPnt3D *EntityPosW(const CEntity *e)
{
    return e ? &e->m_posW : 0;
}

/* whether the SEAL holds the item with the rounds asked; asked a negative count, the item given it
 * with that many (0x5657C0) */
static inline bool HasGrenadeRounds(CSealCtrlAi *ai, EQUIP_ITEM item, int count)
{
    CZKit *kit = &((CZSealBody *)ai->m_entity)->m_kit;

    bool ok;

    if (count >= 0) {
        ok = kit->HasItem(item);
        if (ok)
            ok = kit->GetAmmoTotal(kit->GetItemIndex(item)) >= count;
    } else
        ok = func_005657C0(ai, item, -count);
    return ok;
}

/* a grenade chosen and thrown at the target: not within its wait of the last (0x770) nor while the
 * controller is busy (0x572190); for no item, none at a team mate, item 123 at an entity not to be shot,
 * none by a navy SEAL, else the first of the table's whose ranges hold the target's, the SEAL holding
 * it with a round (flag 0x20: given one) and its throw reaching the point; whether it was thrown (OVL2
 * 0x565110, new in SOCOM II: 0x563E10 and the sensors' tick call it; the name unknown). Byte-exact (the
 * SEALs' round 57, unwritten before) with: 0x572190 auto-inlined (its first `r = true` the reused 1 of
 * the count's movz, `andi s1,v1`); the rounds test with ONE exit, each arm assigning `ok` (a bool call's
 * `&&` operand is `sltu`, its assignment `andi`; two returns add an `andi` at the join); the SEAL declared
 * before the wait (else the clock is loaded ahead of 0x770); the table's item an EQUIP_ITEM field read
 * straight into `item` -- `lb`, kept unnormalised (a signed char field cast to EQUIP_ITEM gives `lbu`) */
extern "C" bool func_00565110(CSealCtrlAi *ai, EQUIP_ITEM item, CTarget *target, unsigned int flags)
{
    bool r = false;
    CEntity *seal = ai->m_entity;
    float since = data_004365C0 - ai->m_770;
    CPnt3D pos = *EntityPosW(target->m_ent);
    int count = (flags & 0x20) ? -1 : 1;

    if (since > data_0064FF80 && !func_00572190(ai)) {
        bool ok = false;

        if (item == EQUIP_NONE) {
            CEntity *e = target->m_ent;

            if (!(e->m_team & seal->m_team)) {
                if (NoShoot(e))
                    item = (EQUIP_ITEM)123;
                else if (!ai->IsNavySeal()) {
                    float d = target->m_distSq;

                    for (int i = 0; i < 8; i++) {
                        if (d >= data_00650210[i].m_min && d <= data_00650210[i].m_max) {
                            item = data_00650210[i].m_item;
                            if (HasGrenadeRounds(ai, item, count) && func_00565460(ai, item, &pos)) {
                                ok = true;
                                break;
                            }
                            item = EQUIP_NONE;
                        }
                    }
                }
            }
        } else if (HasGrenadeRounds(ai, item, count) && func_00565460(ai, item, &pos))
            ok = true;
        if (ok)
            r = func_00564D20(ai, item, &pos, 0, 0);
    }
    return r;
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

extern "C" {
/* the stack's bits brought up to date (OVL2 0x5F7470, seal_aistate.cpp's: the stack, the AI, a flag) */
void func_005F7470(CAiStateStack *stack, CSealCtrlAi *ai, int x);
}

/* the AI's controller made: the bases (CSealCtrl's and SEAL_AI_PARAMS's constructors, out of line),
 * then no debug, no group, full speed, the leg's cells invalid, the flags cleared but bit 12, the view
 * cone a quarter turn wide and 1000 long, the alertness nil, mode 1, the valves and both anim tables
 * cleared, the stack's bits brought up to date, the stances standing (OVL2 0x574080; SOCOM 1's
 * constructor, which built SEAL_AI_PARAMS's members in place -- SOCOM II's 0x60BF40 does) */
CSealCtrlAi::CSealCtrlAi()
{
    m_debug = false;
    SetGroup(0);
    m_speed = 1.0f;
    m_61c = m_620 = CAiMapLoc::invalid;
    m_774 = CPnt3D::zero;
    m_770 = 0;
    m_aibit2 = false;
    m_aibit3 = false;
    m_aibit5 = false;
    m_aibit6 = false;
    m_aibit7 = false;
    m_aibit13 = false;
    m_aibit8 = false;
    m_aibit9 = false;
    m_nosnooze = false;
    m_aibit11 = false;
    m_aibit12 = true;
    m_aibit15 = false;
    m_spraytimer = 0.0f;
    m_5c8 = 0.0f;
    m_awareTimer = 0.0f;
    m_awarePending = (AI_AWARE)0;
    m_5c4 = 0;
    m_5c0 = -100.0f;
    SetViewFOV(0.7853982f);
    SetViewRange(1000.0f);
    m_aware.m_value = 0.0f;
    m_aware.m_goal = 0.0f;
    m_3f4 = (AI_MODE)1;
    m_700 = 1.0f;
    m_704 = 1.0f;
    m_70c = 1.0f;
    m_710 = 1.0f;
    m_follow_master = 0;
    m_follow_master_id = CEntity::invalidId;
    m_padid = 1;
    m_6f0 = 0.0f;
    m_scan_limits = 1.0471976f;
    memset(m_valve, 0, sizeof m_valve);
    memset(_44c, 0, 16);
    memset(&m_anims[3], 0, 16);
    func_005F7470(&m_stateStack, this, 1);
    m_stance_asked = STANCE_STAND;
    m_stance = STANCE_STAND;
    m_stance_delay = 0.0f;
    m_280 = 0.0f;
    m_pathTolerance = 0.0f;
    m_724 = CPnt3D::zero;
    m_730 = CEntity::invalidId;
    m_71c = 0;
    m_6ec = 1.0f;
    m_400 = 0.0f;
    m_404 = 0.0f;
    m_26c = 0;
    m_26d = 0;
    m_780 = m_78c = m_798 = CPnt3D::zero;
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

/* a squad mate's radio valve set -- the AI's own by its call sign at 0x26C (1 Able 2, 2 Bravo 1, 3
 * Bravo 2), or with no AI all three (OVL2 0x574EF0, new in SOCOM II: OnDeath closes the AI's,
 * 0x573130 opens it as it takes its call sign, 0x574C30 closes all three; the name unknown) */
extern "C" void func_00574EF0(CSealCtrlAi *ai, int value)
{
    if (!ai) {
        CValve *v;

        v = CValve::GetByName("snd_able2");
        if (v)
            SetValveValue(v, value);
        v = CValve::GetByName("snd_bravo1");
        if (v)
            SetValveValue(v, value);
        /* (the last test on v0, the copy in its delay slot) */
        if ((v = CValve::GetByName("snd_bravo2")) != 0)
            SetValveValue(v, value);
    } else {
        CValve *v = 0;

        switch (ai->m_26c) {
        case 1:
            v = CValve::GetByName("snd_able2");
            break;
        case 2:
            v = CValve::GetByName("snd_bravo1");
            break;
        case 3:
            v = CValve::GetByName("snd_bravo2");
            break;
        }
        if (v)
            SetValveValue(v, value);
    }
}

/* the scripted groups (ai_group.cpp's class: only its statics here) */
class CAiGroup {
public:
    static void DeleteAll();
    static void ResetIds();
    /* the groups' cameras ended when asked (OVL2 0x60E280) */
    static void EndCameras(bool end);
    /* the group's act broken off (OVL2 0x60D280) */
    void Abort();
};

/* whether the AI drives a SEAL (IsPlayer's ternary shape: the bool narrowed in each arm here) */
static inline bool AiDriven(CZSealBody *seal)
{
    return seal->m_control ? ((CSealCtrl *)seal->m_control)->IsAi() : false;
}

extern "C" {
/* (OVL2 0x5F55E0, the AI states' lane: the AI's alarm raised over a time) */
void func_005F55E0(CSealCtrlAi *ai, float t);
}

/* the AI switched on or off: each SEAL the AI drives has its throttles let go; switched off, the
 * suspended state (18) pushed, its flags 0x10 -- and 1 without the alarm -- set and running, the alarm
 * raised over a thirtieth without it; switched on, that state popped and the body's parts oriented.
 * The groups' cameras ended as the AI now stands (OVL2 0x574830: SOCOM 1's SetAiEnabled, which
 * zeroed the throttles itself, passed a bool to the push and had no alarm or cameras; the list
 * walked through a const view, as seal.cpp's 0x545D80) */
void CSealCtrlAi::SetAiEnabled(bool on, bool alarm)
{
    if (aiEnabled == on)
        return;
    aiEnabled = on;

    const CEntityList &list = CEntity::m_list;

    for (CEntityList::const_iterator i = list.begin(); i != list.end(); ++i) {
        CZSealBody *seal = AsSeal(*i);

        if (seal && AiDriven(seal)) {
            CSealCtrlAi *ai = seal->GetSealCtrlAi();

            ai->StopThrottles(false);
            if (!on) {
                if (ai->PushAiState((AI_STATE)18, 1)) {
                    CAiState *st = ai->GetAiState();
                    unsigned int flags;

                    st = st && st->m_state == 18 ? st : 0;
                    flags = 0x10;
                    if (!alarm)
                        flags |= 1;
                    st->SetFlags(flags, 0);
                    st->SetActive(true);
                    if (!alarm)
                        func_005F55E0(ai, 0.0333f);
                }
            } else {
                ai->PopAiState((AI_STATE)18);
                seal->OrientParts();
            }
        }
    }
    CAiGroup::EndCameras(aiEnabled);
}

/* (declared again below where their users are: the doze count, the task anims' switch, the row) */
namespace ai {
extern int snooze_count;
}

extern "C" {
extern unsigned char data_003DF188;
extern int data_0066AFD8;
/* the callouts' sounds looked up and forgotten (OVL2 0x6075C0, 0x607560: the AI states' seal_airesponse.cpp) */
void func_006075C0();
void func_00607560();
/* a doze's start (SOCOM 1's ai::snooze_time, -3 there; OVL2 bss 0x66AFF0) and two flags cleared with
 * it (OVL2 bss 0x66B008, 0x66AFE0; the names ours) */
extern float data_0066AFF0;
extern unsigned char data_0066B008;
extern unsigned char data_0066AFE0;
/* a time per character sound, twice (OVL2 bss 0x66AD20 and 0x66AE80, 85 floats each: when each sound
 * last played and the least gap before it plays again -- PlayAiSound reads both; the second's fields
 * set after both are cleared) and the idle anims' kinds (OVL2 bss 0x66AD00) -- the names ours */
extern float data_0066AD20[85];
extern float data_0066AE80[85];
extern float data_0066AE94;
extern float data_0066AF6C;
extern float data_0066AF70;
extern float data_0066AF74;
extern float data_0066AF80;
extern float data_0066AF88;
extern float data_0066AF9C;
extern float data_0066AFA0;
extern float data_0066AFA8;
extern float data_0066AFAC;
extern int data_0066B048;
extern int data_0066B050;
extern int data_0066AFE8;
extern int data_0066AD00[7];
}

/* the anim kind of a name (anim.cpp's, the kit lane's: C++ linkage) */
int FindAnimKind(const char *name);

/* the AI's handles let go: the loops' sounds and instances, the callouts' sounds, the request pool, the
 * groups (OVL2 0x574BD0; SOCOM 1's ReleaseDatabaseHandles, the integrator's lead from the kit's round
 * 25: SOCOM 1's two memsets and CloseDiPool, SOCOM II's callouts and groups around them) */
void CSealCtrlAi::ReleaseDatabaseHandles()
{
    memset(aiSoundHandle, 0, sizeof aiSoundHandle);
    memset(aiSoundInstance, 0, sizeof aiSoundInstance);
    func_00607560();
    CloseDiPool();
    CAiGroup::DeleteAll();
}

/* the AI's handles taken: the loops' sounds, the callouts', the task anims allowed, the request pool
 * opened, the doze count and time reset, the difficulty off its valve (within 0 to 7, else 1), the
 * squad's radio valves closed, the tuning cleared and set, the slices reset, the idle anims' kinds
 * looked up and the groups' ids (OVL2 0x574C30; SOCOM 1's AcquireDatabaseHandles, the integrator's
 * lead from the kit's round 25: SOCOM 1's two GetSoundByName and OpenDiPool, then its doze reset) */
void CSealCtrlAi::AcquireDatabaseHandles()
{
    aiSoundHandle[0] = CSnd::GetSoundByName("SND_STREAM_ABORT");
    aiSoundHandle[1] = CSnd::GetSoundByName("STREAMING_BOUNDRY_WARNING");
    func_006075C0();
    data_003DF188 = 1;
    OpenDiPool();
    ai::snooze_count = 0;
    data_0066AFF0 = -2.0f;
    data_0066B008 = 0;
    data_0066AFE0 = 0;

    CValve *v = CValve::GetByName("ai_difficulty");

    if (v) {
        data_0066AFD8 = v->m_value;
        _saturate(&data_0066AFD8, 0, 7);
    } else {
        data_0066AFD8 = 1;
    }
    func_00574EF0(0, 0);
    memset(data_0066AD20, 0, sizeof data_0066AD20);
    memset(data_0066AE80, 0, sizeof data_0066AE80);
    data_0066AF70 = 1.5f;
    data_0066AF74 = 1.5f;
    data_0066AF6C = 1.25f;
    data_0066AE94 = 0.15f;
    data_0066AF9C = 3.5f;
    data_0066AFA0 = 2.5f;
    data_0066AFAC = 3.0f;
    data_0066AFA8 = 5.0f;
    data_0066B050 = 4;
    data_0066AF88 = 3.0f;
    data_0066AF80 = 3.0f;
    ai_slice0.Reset();
    ai_slice1.Reset();
    ai_slice4.Reset();
    data_0066B048 = 0;
    data_0066AD00[0] = 0;
    data_0066AD00[2] = FindAnimKind("bored02");
    data_0066AD00[3] = FindAnimKind("bored03");
    data_0066AD00[1] = data_0066AD00[2];
    data_0066AD00[4] = FindAnimKind("suspicious");
    data_0066AD00[5] = FindAnimKind("ready");
    data_0066AD00[6] = FindAnimKind("conversation");
    CAiGroup::ResetIds();
    data_0066AFE8 = 0;
}

/* (how far an event's kind carries: SOCOM 1's static CAiEvent::GetEventRange, OVL2 0x50F4C0 -- ai.h's) */
/* the voice's reach scale (OVL2 data 0x650380, read as a variable) */
extern "C" float data_00650380;

/* the voice's AI event made where the SEAL stands, unless it has none; the event returned (OVL2
 * 0x55FEE0; PlayAiSound calls it) */
extern "C" CAiEvent *func_0055FEE0(CSealCtrl *ctrl, CAiEvent::EVENT event)
{
    CAiEvent *r = 0;

    if (event != CAiEvent::AIE_NONE) {
        float range = CAiEvent::GetEventRange(event);

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

extern "C" {
/* (below: the route walked on along a path, and the AI's state brought up to date, the route's
 * state returned -- OVL2 0x5556A0, 0x5558A0) */
bool func_005556A0(CSealCtrlAi *ai, const CAiPathBytes *path, CAiRouteBytes *route);
AI_ROUTE func_005558A0(CSealCtrlAi *ai);
/* (the kit's fire mode and its current item by index are zseal.h's CZKit::SetCurFireMode and
 * SetCurItemByIndex, OVL2 0x5C37B0 and 0x5C4B10) */
}

/* the fire's aim dropped: the two aim flags off, its time restarted (an inline both FireWhenReady and
 * the deploy's signals show; the name ours) */
static inline void DropAim(AI_FIREPARAMS &p)
{
    p.m_flags &= ~0x4200000;
    p.m_timeout = 0.0f;
}

/* the deploy told: signal 4 resets it; 1 starts it -- the kit's first item on full auto, the look of
 * mode 11 cleared, the SEAL to go prone at once, the aim dropped, the weapon to state 2 and the
 * timer cleared; 2 stops it -- in the first three steps the route walked on (0x5556A0), in steps 3-5
 * the step back to 0, in step 8 the timer cleared and the step back to 0, past them the state no
 * longer running -- and drops the aim and the look; 3 ends a route under way: brought up to date
 * (the step back to 0 when it is a route of kind 2), its path cleared and a running one ended (OVL2
 * 0x55E3B0: SOCOM 1's OnSignal, its signal 1 alone -- the stance, the timer and signals 2 and 3 new) */
static inline int PhaseOf(const CAiState *s)
{
    return s->m_phase;
}

void CAiSDeploy::OnSignal(SIGNAL sig, CSealCtrlAi *ai)
{
    CZSealBody *seal = ai ? (CZSealBody *)ai->m_entity : 0;

    switch (sig) {
    case SIGNAL_4:
        m_80 = 0;
        m_84 = 0;
        m_98 = 0;
        m_94 = 0;
        m_70 = 0;
        m_timer = 0.0f;
        m_8c = 0;
        *(unsigned int *)&m_74 = *(unsigned int *)&CAiMapLoc::invalid;
        m_7c = 2;
        break;
    case SIGNAL_0:
        break;
    case SIGNAL_1: {
        CZKit *kit = &seal->m_kit;

        kit->SetCurItemByIndex(0);
        kit->SetCurFireMode((FIREMODE)2);
        ai->ClearLook(AIL_11);
        ai->SetDesiredStance(STANCE_PRONE, 0.0f, false);
        DropAim(ai->m_fire);
        seal->SetWeaponState((WEAPON_STATE)2);
        CZWeapon::ClearTimer(&m_timer);
        break;
    }
    case SIGNAL_2: {
        int step = PhaseOf(this);

        if (step < 3)
            func_005556A0(ai, &ai->m_path, &ai->m_route);
        else if (step < 6)
            UpdatePhase(0);
        else if (step == 8) {
            CZWeapon::ClearTimer(&m_timer);
            UpdatePhase(0);
        } else
            SetActive(false);
        DropAim(ai->m_fire);
        ai->ClearLook(AIL_11);
        break;
    }
    case SIGNAL_3:
        if (ai->m_route.m_state != ROUTE_NONE) {
            if (func_005558A0(ai) == ROUTE_2)
                UpdatePhase(0);
            ((CAiPath *)&ai->m_route.m_path)->Clear();
            switch (ai->m_route.m_state) {
            case ROUTE_1:
            case ROUTE_2:
                ai->m_route.m_state = ROUTE_NONE;
                break;
            }
        }
        break;
    }
}

/* (OVL2 0x55E660: slot 0x1C of __vt__10CAiSDeploy) */
bool CAiSDeploy::CanPushAiState(AI_STATE state, CSealCtrlAi *ai)
{
    return false;
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
/* (OVL2 0x555420, below: a route brought up to date from the AI's path) */
bool func_00555420(CSealCtrlAi *ai, CAiPathBytes *path, CAiRouteBytes *route);
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
        s = ((const CAiStateStack &)ai->m_stateStack).GetState();

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
    bool ok = AiMaps.RegisterPathPlan(start, locs, count, (CAiPath *)&m_path);

    if (ok) {
        SetAiMode((AI_MODE)3);

        float bias = GetDefaultRouteBias();

        if (((CAiPath *)&m_path)->SetPathType(GetDefaultRouteType()))
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

/* (OVL2 0x560A60: SOCOM 1's GetAiAware() const -- the band an enum: `li 4` where it is inlined) */
AI_AWARE CSealCtrlAi::GetAiAware() const
{
    for (int i = 0; i <= 4; i++)
        if (InBand(m_aware.m_value, i))
            return (AI_AWARE)i;
    return AIA_SPECIFIC;
}

/* the bands' floors (OVL2 data 0x65C6E0: 0, 0.05, 0.4, 0.6, 0.8) */
extern "C" float data_0065C6E0[5];

/* the alertness's value and goal set by the flags -- 1 the value, 2 the goal, 4 the goal added to
 * the given one first -- each held within [0, 1], the band the value left kept and a change of band
 * noted; past GENERAL the goal is kept at GENERAL's floor (OVL2 0x5612F0: SOCOM 1's SetAiAware(float,
 * unsigned int), the sleeper's test, the dead SEAL's zero and the bands new) */
void CSealCtrlAi::SetAiAware(float value, unsigned int flags)
{
    if (IsSleeping() && IsAiBit15())
        return;

    AI_AWARE old = GetAiAware();

    if (!m_entity->m_isAlive)
        value = 0.0f;
    if (flags & 1) {
        float v = value;

        if (flags & 4)
            v += m_aware.m_goal;
        _saturate(&v, 0.0f, 1.0f);
        m_5c4 = GetAiAware();
        m_aware.m_value = v;
    }
    if (flags & 2) {
        float v = value;

        if (flags & 4)
            v += m_aware.m_goal;
        _saturate(&v, 0.0f, 1.0f);
        m_aware.m_goal = v;
    }

    AI_AWARE now = GetAiAware();

    if (old != now)
        m_5c8 = 0.0f;
    if (now >= AIA_GENERAL && m_aware.m_goal < data_0065C700[AIA_GENERAL])
        m_aware.m_goal = data_0065C6E0[AIA_GENERAL];
}

/* the alertness raised to a band's top (less a thousandth): a dead SEAL's to none; at DIRECTION with
 * a point, an event of kind 10 made there unless already SPECIFIC; a time of -1 asks the difficulty's
 * delay for GENERAL and above (none when that band already waits); a delay, for any but a Navy SEAL,
 * leaves the band waiting with it, otherwise the value is raised at once and nothing waits (OVL2
 * 0x561110: SOCOM 1's SetAiAware(AI_AWARE, const CPnt3D *), its event kind 2 for 1.5 seconds, the
 * time new) */
void CSealCtrlAi::SetAiAware(AI_AWARE aware, const CPnt3D *pos, float time)
{
    if (IsSleeping() && IsAiBit15())
        return;
    if (!m_entity->m_isAlive)
        aware = AIA_UNAWARE;

    float level = data_0065C700[aware] - 0.001f;

    if (pos && aware == AIA_DIRECTION) {
        if (GetAiAware() < AIA_SPECIFIC)
            SetAiEvent((CAiEvent::EVENT)10, 3.0f, pos);
    }
    if (time == -1.0f && aware >= AIA_GENERAL) {
        if (m_awarePending == aware)
            return;
        time = data_0064FE60[data_0066AFD8][5];
    }
    if (time > 0.0f && !IsNavySeal()) {
        m_awareTimer = time;
        m_awarePending = aware;
        return;
    }
    if (level > m_aware.m_value)
        SetAiAware(level, 1);
    m_awareTimer = 0.0f;
    m_awarePending = AIA_UNAWARE;
}

extern "C" {
/* (OVL2 0x571150, below: the anim played, or not, in the state -- 11 for the SEAL's own) */
bool func_00571150(CSealCtrlAi *ai, short sound, SEAL_STATE stance);
/* (OVL2 data: how long a sound holds before another) */
extern float data_0064FE50;
/* (OVL1 data: the game's clock, as this unit reads it) */
extern float data_004365C0;
}

/* an AI sound made if its priority reaches the controller's alertness band (0x560A60 inlined); once
 * made, remembered with when the next may come and the band (OVL2 0x571010, new in SOCOM II: the
 * event and shot-at states call it, with a priority 2 and a stance 11; the name unknown).
 * The priority is a plain word (a struct by value is homed on the stack, sw a2/lbu): its low byte
 * taken first into an INT local (`andi a1,a2,0xff` at entry -- an unsigned char local takes it late
 * and moves the band to s0, an unsigned int compares with sltu), the word kept in s0 for the store
 * `(pri & ~0xFF) | band`; the test written `band <= level` branches on `slt at` (`level >= band`
 * materialises into v1). The second parameter stays a short: as an AnimType MW narrows it again,
 * 8 bytes longer (the kit's callers pass func_00609530's AnimType, the C linkage hides it). */
extern "C" bool func_00571010(CSealCtrlAi *ai, short sound, unsigned int pri, unsigned char stance)
{
    int level = pri & 0xFF;
    unsigned char band = ai->GetAiAware();
    bool made = false;

    if (band <= level) {
        made = func_00571150(ai, sound, (SEAL_STATE)stance);
        if (made) {
            ai->m_sound = sound;
            ai->m_sound_time = data_0064FE50 + data_004365C0;
            *(unsigned int *)&ai->m_sound_priority = (pri & ~0xFF) | band;
        }
    }
    return made;
}

/* a figure of the current row (OVL2 0x5750D0; before sealai::get_stance) */
extern "C" float func_005750D0(unsigned char i)
{
    return data_0064FE60[data_0066AFD8][i];
}

/* an AI event of the SEAL's made, no sooner than its kind's rate after the last of the kind: for the
 * duration, heard within the range, seen or not, at the point given or the SEAL's own, the word
 * handed on, the SEAL its owner; the event, or none (OVL2 0x572FA0: SOCOM 1's CreateAiEvent, which
 * answered a bool, took the SEAL's position through a virtual and never met a null event) */
CAiEvent *CSealCtrl::CreateAiEvent(CAiEvent::EVENT event, float duration, float range, bool visible,
    const CPnt3D *pos, unsigned int userdata)
{
    float now = data_004365C0;
    float elapsed = now - m_event_time[event];
    float rate = CAiEvent::GetEventRate(event);
    CAiEvent *e = 0;

    if (elapsed > rate) {
        m_event_time[event] = now;
        CEntity *owner = m_entity;

        e = CAiEvent::Acquire();
        if (e) {
            e->m_type = event;
            e->m_duration = duration;
            e->m_rangeSq = range * range;
            e->m_visible = visible;
            const CPnt3D *p = pos ? pos : &owner->m_posW;

            e->m_pos = *p;
            e->m_userdata = userdata;
            e->m_owner = owner->m_id;
            CAiEvent::Create(e);
        }
    }
    return e;
}

/* an event kind's bit (an inline: through it the base mask is a guarded static, as the target's) */
static inline unsigned int EventBit(CAiEvent::EVENT e)
{
    return 1 << e;
}

/* neither of SOCOM 1's m_snoozing (bit 9) and m_sleeping (bit 8) set (an inline bool). The target
 * materialises it before its branch: bit 9 as a bool (sltu), bit 8 only when that is clear, then a
 * bare xori -- an int holding the `||` gives exactly that; `!(a || b)` builds 1/0 constants, a bool
 * local adds a second sltu at the join, bool accessors cost 8 bytes */
static inline bool AiAwake(const CSealCtrlAi *ai)
{
    int r = ai->m_aibit9 != 0;

    if (!r)
        r = ai->m_aibit8 != 0;
    return r ^ 1;
}

/* the kinds of event the AI hears: all but the unknown, less the sleeper's when it sleeps or its team
 * has bit 26, less the teams' of bits 28 and 17, and only the deaf set's complement for a team of bit
 * 1 -- none online (OVL2 0x572D70: SOCOM 1's UpdateEventMask, by its order and CTTeam::Tick's calls --
 * the join and the leave -- where SOCOM 1's calls it; SOCOM II starts from a fourth static and tests
 * the team's bit 1 first) */
void CSealCtrlAi::UpdateEventMask()
{
    static unsigned int all = ~EventBit(CAiEvent::AIE_UNKNOWN);
    static unsigned int asleep = CAiEvent::MakeMaskEx(CAiEvent::AIE_UNKNOWN, CAiEvent::AIE_GENERIC,
        CAiEvent::AIE_POINT, CAiEvent::AIE_ENTITY, CAiEvent::AIE_VOICE, CAiEvent::AIE_CORPSE,
        CAiEvent::AIE_DOOR, CAiEvent::AIE_HELPME, CAiEvent::AIE_ALARM, CAiEvent::AIE_LIGHTSOFF,
        CAiEvent::AIE_NONE);
    static unsigned int unaware = CAiEvent::MakeMaskEx(CAiEvent::AIE_UNKNOWN, CAiEvent::AIE_GENERIC,
        CAiEvent::AIE_POINT, CAiEvent::AIE_ENTITY, CAiEvent::AIE_CORPSE, CAiEvent::AIE_HELPME,
        CAiEvent::AIE_ALARM, CAiEvent::AIE_LIGHTSOFF, CAiEvent::AIE_EXPLOSION, CAiEvent::AIE_NONE);
    static unsigned int deaf = CAiEvent::MakeMaskEx(CAiEvent::AIE_UNKNOWN, CAiEvent::AIE_GENERIC,
        CAiEvent::AIE_ALARM, CAiEvent::AIE_LIGHTSOFF, CAiEvent::AIE_CORPSE, CAiEvent::AIE_POINT,
        CAiEvent::AIE_DOOR, CAiEvent::AIE_NONE);
    CEntity *e = m_entity;

    m_eventMask = all;
    if (e->m_team & 2)
        m_eventMask &= ~deaf;
    else {
        if (!AiAwake(this) || (e->m_team & 0x4000000))
            m_eventMask &= ~asleep;
        if ((e->m_team & 0x10000000) || (e->m_team & 0x20000))
            m_eventMask &= ~unaware;
    }
    if (theNetwork.m_9)
        m_eventMask = 0;
}

/* whether an entity may be taken as a target: any in mode 4, else one of no team of ours (OVL2
 * 0x572D30) */
extern "C" bool func_00572D30(CSealCtrlAi *ai, CEntity *e)
{
    CEntity *me = ai->m_entity;

    return ai->m_26c == 4 || (e && !(e->m_team & me->m_team));
}

extern "C" {
/* an alert from an entity taken in (OVL2 0x572240, below: auto-inlined into OnWeaponEvent) */
void func_00572240(CSealCtrlAi *ai, CEntity *alert, float amount);
/* how near a player's shot must pass to be called out, and the alert's factor (OVL2 data 0x64FFF8,
 * 70, as SOCOM 1's guarded static; 0x64FDE0, 2) */
extern float data_0064FFF8;
extern float data_0064FDE0;
}

/* a weapon of the grenades' 121-128, and of the explosives' 151-153 (inlines, the names ours: each
 * test's bool through the null test's own, the target masks at the second's join) */
static inline bool IsGrenadeId(EQUIP_ITEM id)
{
    switch (id) {
    case 121:
    case 122:
    case 123:
    case 124:
    case 125:
    case 126:
    case 127:
    case 128:
        return true;
    default:
        return false;
    }
}

static inline bool IsGrenade(CZWeapon *w)
{
    return w ? IsGrenadeId(w->m_ID) : false;
}

static inline bool IsExplosiveId(EQUIP_ITEM id)
{
    switch (id) {
    case 152:
    case 153:
    case 151:
        return true;
    default:
        return false;
    }
}

static inline bool IsExplosive(CZWeapon *w)
{
    return w ? IsExplosiveId(w->m_ID) : false;
}

/* a projectile's flight heard: awake and alive, a shooter to take as a target whose line passes near
 * -- the player's own shot within 70 called out as an event of 9 -- within the hearing range an alert
 * by how near, unless a grenade's or an explosive's, and the shooter's target flagged (SOCOM 1's
 * OnWeaponEvent, 0x31C bytes: SOCOM II's takes the alert from 0x572240, new, for SOCOM 1's point,
 * event and alertness; OVL2 0x5729F0) */
void CSealCtrlAi::OnWeaponEvent(CZProjectile *proj, unsigned int event, void *data)
{
    CZSealBody *player;
    CEntity *e = m_entity;
    int id;
    bool a = !(m_aibit9 || m_aibit8);

    if (!a || !e->m_isAlive)
        return;
    float dist = 999999.0f;
    const CPnt3D *pos = &e->m_posW;
    const CPnt3D *from = &proj->m_framestartpos;
    const CPnt3D *to = &proj->m_frameendpos;
    CEntity *shooter = CEntity::m_list.GetEntityById(proj->m_shooterID);
    bool taken = func_00572D30(this, shooter);

    id = proj->m_shooterID;
    player = ftsGetPlayer();
    if (taken && pos->DistToVector(from, to, &dist)) {
        if (dist < data_0064FFF8 && id == player->m_id)
            ((CSealCtrl *)player->m_control)->CreateAiEvent((CAiEvent::EVENT)9, 0.5f, 1500.0f, false, &player->m_posW, e->m_id);
        if (dist < m_aiParams.thip_range) {
            if (!IsGrenade(proj->m_weapon) && !IsExplosive(proj->m_weapon))
                func_00572240(this, shooter, data_0064FDE0 * (1.0f - dist / m_aiParams.thip_range));
            CTarget *t = m_entity->m_targets.FindTarget(shooter);

            if (t)
                t->m_flags |= 0x400;
        }
    }
}

/* whether a map action is a door (an inline bool: the test's type written in the `if` loses the pad at
 * the type's join, its false arm then filling the branch's delay slot; the name ours) */
static inline bool ActionIsDoor(const CAiMap::action *a)
{
    return a->GetType() == 1;
}

/* what the AI is at, as the team names show it: by its state's kind and the state's flags, a follower
 * of an AI leader as the leader is (OVL2 0x5743C0, new in SOCOM II: CTeamNames::Tick asks it; the name
 * unknown). The cases written so, their tests run reversed */
extern "C" const char *func_005743C0(CSealCtrlAi *ai)
{
    CEntity *e = ai->m_entity;
    AI_STATE kind = ai->GetJumpFlag();
    CZSealBody *leader = ai->m_unit->GetLeader();
    bool lead = e == leader;
    bool leaderAi = leader->m_control ? ((CSealCtrl *)leader->m_control)->IsAi() : false;
    CAiState *s = ai->GetAiState();
    const char *r = 0;

    switch (kind) {
    case 0:
        r = "HOLDING";
        break;
    case 2:
        if (s->m_flags & 0x20)
            r = "COVERING";
        else if (s->m_flags & 0x80)
            r = "OVERWATCH";
        else
            r = "HOLDING";
        break;
    case 8:
        if (s->m_flags & 8)
            r = "LEADING";
        else if (s->m_flags & 0x10)
            r = "ATTACKING";
        else if (s->m_flags & 0x100)
            r = "CLEARING";
        else if (s->m_flags & 0x200)
            r = "REGROUPING";
        else
            r = "TRAVELING";
        break;
    case 4:
        r = "DEPLOYING";
        break;
    case 10:
        if (s->m_phase < 8)
            r = "BREACHING";
        else
            r = "CLEARING";
        break;
    case 9:
        if (!lead && leaderAi)
            r = func_005743C0(((CSealCtrl *)leader->m_control)->GetSealCtrlAi());
        else
            r = "FOLLOWING";
        break;
    case 5:
        r = "LASING";
        break;
    case 12:
    case 26:
        r = "ENGAGING";
        break;
    case 11: {
        CAiSAction *a = s && s->m_state == AI_STATE_ACTION ? (CAiSAction *)s : 0;
        CNodeAction *na = a ? a->m_58 : 0;

        if (na) {
            CAiMapLoc loc;
            CAiMap::action *act = 0;

            if (AiMaps.GetActionById(na->m_id, loc, &act) && ActionIsDoor(act)) {
                if (a->m_5c == 1)
                    r = "OPENING";
                else
                    r = "CLOSING";
            }
        }
        if (!r)
            r = "WORKING";
        break;
    }
    }
    return r;
}

/* the AI's anger raised by a time's worth, when the switch is on (an inline: OVL2 data 0x64FDF8, a
 * byte, and 0x64FE00, 60 a second; the name ours) */
extern "C" {
extern unsigned char data_0064FDF8;
extern float data_0064FE00;
extern float data_0064FDD8;
/* the noise a shot's point takes when it is no target's, the anger's rate (OVL2 data 0x64FDB0;
 * 0x64FE20, the range's factor at 0x70C) */
extern PNT2D data_0064FDB0;
extern float data_0064FE20;
/* the held team's response to a hit (OVL2 0x606DB0, seal_airesponse.cpp's) and whether the player hit
 * is the one held for (OVL2 0x572880, below) */
void func_00606DB0(CAiState *s, CSealCtrlAi *ai, CEntity *e);
bool func_00572880(CZSealBody *seal, CSealCtrlAi **ai, CAiState **state);
}

static inline void AddAnger(CSealCtrlAi *ai, float t)
{
    if (data_0064FDF8)
        ai->m_400 += data_0064FE00 * t;
}

/* a projectile's hit taken: the shooter's team told, an enemy's shot heard as a voice's event and its
 * point (the shooter's, or behind the shot) noised, an alert from the shooter unless a grenade's or an
 * explosive's; a player's own shot angers a Navy SEAL unless it is a grenade's -- bit 22 set for a
 * detonation; the time kept, nothing at all done twice in a frame (OVL2 0x572330: SOCOM 1's OnWeaponHit
 * grown) */
void CSealCtrlAi::OnWeaponHit(zdb::IntersectStruct *hit, CZProjectile *proj)
{
    CZSealBody *seal = (CZSealBody *)m_entity;
    CZSealBody *shooter = AsSeal(CEntity::m_list.GetEntityById(proj->m_shooterID));
    bool enemy = func_00572D30(this, shooter);

    if ((float)data_004365C0 - m_40c == 0.0f)
        return;
    if (shooter && shooter->IsPlayer()) {
        if (data_0066AFE0) {
            CAiState *state = 0;
            CSealCtrlAi *ai = 0;

            if (func_00572880(shooter, &ai, &state))
                func_00606DB0(state, ai, seal);
        } else if (enemy) {
            CSealUnit *unit = ((CSealCtrl *)shooter->m_control)->m_unit;

            if (unit) {
                unsigned int bit = unit->m_mask;
                bool seen = (((const CEntity *)seal)->m_UnitsSeenBy & bit) != 0;

                if (!seen)
                    seal->m_UnitsSeenBy |= bit;
                if (!seen)
                    unit->Notify(shooter, UNIT_MESSAGE_0, seal);
            }
        }
    }
    if (enemy) {
        func_0055FEE0(this, (CAiEvent::EVENT)2);

        CPnt3D p;

        if (shooter) {
            CTarget *t = m_entity->m_targets.FindTarget(shooter);

            p = shooter->m_posW;
            if (t)
                t->m_flags |= 0x400;
        } else
            proj->m_vel.Scale(-100.0f, &p);
        if (((CAiTargetFlags *)m_67c)->m_entity != shooter)
            ai::AddNoise(&p, data_0064FDB0);
        m_70c *= data_0064FE20;
        if (!IsGrenade(proj->m_weapon) && !IsExplosive(proj->m_weapon))
            func_00572240(this, shooter, data_0064FDD8);
    } else if (shooter && shooter->IsPlayer()) {
        CZWeapon *w = proj->m_weapon;
        EQUIP_ITEM item = w ? w->m_ID : (EQUIP_ITEM)255;

        if (proj != seal->m_lastprojectile) {
            if (proj->WasDetonated()) {
                switch (item) {
                case 159:
                case 127:
                case 128:
                case 122:
                case 123:
                    break;
                default:
                    m_aibit22 = 1;
                    break;
                }
            } else {
                switch (item) {
                case 159:
                case 151:
                case 127:
                case 128:
                case 122:
                case 123:
                case 121:
                    break;
                default:
                    AddAnger(this, 1.0f);
                    break;
                }
            }
        }
    }
    m_40c = data_004365C0;
}

/* a valve, or none past the three (OVL2 0x5621C0; SOCOM 1's, which had four) */
CValve *CSealCtrlAi::GetValve(SEAL_VALVE v) const
{
    if (v < 3)
        return m_valve[v];
    return 0;
}

extern "C" {
/* the stack's bits brought up to date (OVL2 0x5F7470, seal_aistate.cpp's: the stack, the AI, a flag) */
void func_005F7470(CAiStateStack *stack, CSealCtrlAi *ai, int x);
}

/* an anim's signal taken: 1, 2, 3 and 6 handed to every state of the stack, the top last-first --
 * through the stack's reverse iterator (include/list's rbegin()/rend(), the MSL headers' round 22: the
 * end pair's second() its one call, 0x5621B0); 4 the suspended state pushed, kept only when it is the
 * state on top, its flags set and marked running; 5 the suspended state popped; then the stack's bits
 * brought up to date (OVL2 0x562030: SOCOM 1's AnimSignal, whose 0x404 bytes walked the stack forward
 * for its signals 1 and 2).
 * OPEN, 0x17C against 0x180 (0x174 before the kit's round 35 retyped slot 0xC to the caller's (CZAnim *,
 * AI_SIGNAL, CSealCtrlAi *): the target hands the signal on as it came, `move s0,a2`, the andi only the
 * switch's), on (2) this unit's inline_depth(8) -- the MSL lane's probe at 5 calls the end
 * pair's second() (0x5621B0) as the target does at each rbegin()/rend(); at 8 MW inlines it, holding
 * the sentinel's next in a fourth saved register (the frame 96 bytes against 80). The probe's own
 * last difference (the end pair's address kept, not rebuilt per call) sits behind that */
void CSealCtrlAi::AnimSignal(CZAnim *anim, AI_SIGNAL sig)
{
    switch (sig) {
    case 1:
    case 2:
    case 3:
    case 6: {
        CAiStateStack::reverse_iterator i;

        for (i = m_stateStack.rbegin(); i != m_stateStack.rend(); ++i)
            (*i)->Virtual0C(anim, sig, this);
        break;
    }
    case 4:
        if (PushAiState((AI_STATE)18, 1)) {
            CAiState *s = GetAiState();

            s = s && s->m_state == 18 ? s : 0;

            s->SetFlags(7, 0);
            s->m_active = true;
        }
        break;
    case 5:
        PopAiState((AI_STATE)18);
        break;
    }
    func_005F7470(&m_stateStack, this, 1);
}

/* whether the SEAL may be taken hostage (state 23 pushed with no flags): for kind 3 a restrainable one,
 * or online one of team bit 17; otherwise one of team bit 17 already holding the state or free to take
 * it (an inline, the name ours: 0x555B80 is it out of line, 0x555900 inlines it -- generated ahead of
 * 0x555B80, it cannot inline that). A switch (the case's block first behind its beq, a `b` to the
 * default) assigning one bool, masked once at the join: returned from each case, the andi goes */
static inline bool CanTakeHostage(CSealCtrlAi *ai, int kind)
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

/* (OVL2 0x555B80, new in SOCOM II: 0x5784A0 and 0x5A90B0 ask it; the name unknown) */
extern "C" bool func_00555B80(CSealCtrlAi *ai, int kind)
{
    return CanTakeHostage(ai, kind);
}

/* the SEAL taken hostage when it may be: the states popped down to the hostage state, the alert one
 * (6) or the base one -- else cleared --, then the hostage state found on top or pushed, its step set
 * to the kind with no delay pending and the given flags added; the state returned, or none (OVL2
 * 0x555900, new in SOCOM II: 0x5784A0 calls it -- the name unknown) */
extern "C" CAiState *func_00555900(CSealCtrlAi *ai, int kind, unsigned int flags)
{
    CAiState *s = 0;

    if (CanTakeHostage(ai, kind)) {
        if (ai->m_stateStack.m_10 & (1 << AI_STATE_HOSTAGE))
            ai->PopToAiState(AI_STATE_HOSTAGE, false);
        else if (ai->m_stateStack.m_10 & (1 << 6))
            ai->PopToAiState((AI_STATE)6, false);
        else if (!ai->PopToAiState((AI_STATE)1, false))
            ai->ClearAiState(true);
        if (ai->AiStateIs(AI_STATE_HOSTAGE) || ai->PushAiState(AI_STATE_HOSTAGE, 0)) {
            CAiState *st = ai->GetAiState();

            s = st && st->m_state == AI_STATE_HOSTAGE ? st : 0;
            s->UpdatePhase(kind);
            s->SetFlags(0, 0x40000000);
            s->m_delay = 0.0f;
            s->m_nextPhase = s->m_phase;
            if (flags)
                s->SetFlags(flags, 0);
        }
    }
    return s;
}

/* the AI's state brought up to date, the route's state returned (OVL2 0x5558A0; the AI states'
 * OnSignal call it) */
extern "C" AI_ROUTE func_005558A0(CSealCtrlAi *ai)
{
    func_00555420(ai, &ai->m_path, &ai->m_route);
    return ai->m_route.m_state;
}


/* the route reset, and its state set: a running one (1 or 2) is ended first (OVL2 0x5557B0; the
 * path-recovery state calls it) */
extern "C" void func_005557B0(CSealCtrlAi *ai, AI_ROUTE route)
{
    ((CAiPath *)&ai->m_route.m_path)->Clear();
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
    ((CAiPath *)&ai->m_route.m_path)->Clear();
    switch (ai->m_route.m_state) {
    case ROUTE_1:
    case ROUTE_2:
        ai->m_route.m_state = keep ? ROUTE_2 : ROUTE_NONE;
        break;
    }
}

/* a path started over from a cell of a map: CAiPath::InsertPathLoc(loc, map, 0, true) (OVL2 0x526810,
 * the AI core's: the route's legs begun at the SEAL's cell); a path's plan is copied from another by
 * CAiPath::CopyPlan (OVL2 0x527FD0, ai.h's) */

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

    ((CAiPath *)&route->m_path)->Clear();
    route->m_state = ROUTE_NONE;

    const CAiMapLoc &loc = e->m_mapLocValid;

    route->m_state = ROUTE_NONE;

    bool r = false;

    if (PathPlanning(path)) {
        ((CAiPath *)&route->m_path)->InsertPathLoc(loc, AiMaps.GetMapByIndex(loc.m_mapid), 0, true);
        route->m_state = ROUTE_2;
        r = true;
    } else if (PathHeld(path)) {
        ((CAiPath *)&route->m_path)->CopyPlan((const CAiPath *)path);
        ((CAiPath *)&route->m_path)->InsertPathLoc(loc, AiMaps.GetMapByIndex(loc.m_mapid), 0, true);
        route->m_state = ROUTE_1;
        r = true;
    }
    return r;
}

/* a route brought up to date: one asked to start (3) runs (1); a running one whose plan is held is
 * handed to the AI's path -- as it is when it ends on the SEAL's cell, with that cell put at its head
 * when the line from it to the last point is clear, else from the first point back from the end the
 * line reaches (none past a flagged one), the points after it dropped and the path smoothed, the
 * route left planning (2) when no point is reached; a running route's path is cleared either way;
 * whether the route was running or planning (OVL2 0x555420, new in SOCOM II: 0x5558A0 and
 * CAiSDeploy::OnSignal call it; the name unknown).
 * OPEN, 14 structural differences and 6 register-only (159 instructions against 157; round 48): the
 * held test. The target materialises the count's test (`xor/sltiu/xori` -- an inline's `!empty()`,
 * as here), branches on the state (`li 4; bne`), and sets its result `0` in the first test's delay
 * slot and `andi a2,a0,0xff` (the switch's 1) when both pass, keeping the count in a1 for the last
 * point's index. Every shape tried gives one of two: `r = false; if (A && B) r = true;` with A a
 * materialised inline is if-converted to `r = A ? B : 0` (B by xori/sltiu, 17 differences), and
 * written plain (A a branch on the count) it keeps the target's init-and-andi but drops A's
 * materialisation and reloads the count (14); `return true; / return false;` (here) gives two
 * assignment arms (14). Tried too: a bool local (folded), HasPoints(), nested ifs, `(bool)size`,
 * `== true` / `!= false`, the path cast once ahead of the test. The rest follows the target: the
 * SEAL's cell named after the last point (a named `loc` ahead of `i` takes s1 -- a temporary's s0 in
 * the target -- through the accessor it becomes the CSE temporary), the point compare an inline with
 * the point's field on the left (the target loads its y first), size() by value (count()'s reference
 * kept &size_ in a sixth saved register) */
static inline bool HasAny(const CAiPath *p)
{
    return !p->m_points.empty();
}

static inline bool RouteHeld(const CAiPath *p)
{
    if (HasAny(p) && p->m_state == CAiPath::COMPLETE)
        return true;
    return false;
}

/* whether two cells are the same place, the map aside (an inline bool: the target materialises each
 * compare; the name ours -- 0x569AA0's family and 0x555420 ask it) */
static inline bool SameCell(const CAiMapLoc &a, const CAiMapLoc &b)
{
    return a.x == b.x && a.y == b.y;
}

extern "C" bool func_00555420(CSealCtrlAi *ai, CAiPathBytes *path, CAiRouteBytes *route)
{
    CZSealBody *seal = (CZSealBody *)ai->m_entity;
    bool r = false;

    switch (route->m_state) {
    case ROUTE_NONE:
        break;
    case ROUTE_3:
        route->m_state = ROUTE_1;
        r = true;
        break;
    case ROUTE_2:
        r = true;
        break;
    case ROUTE_1:
        if (RouteHeld((CAiPath *)&route->m_path)) {
            CAiPath *rp = (CAiPath *)&route->m_path;
            const CAiPath::point &last = rp->m_points[rp->m_points.size() - 1];
            const CAiMapLoc &loc = seal->MapLocValid();

            r = true;
            if (SameCell(last.m_loc, loc))
                ((CAiPath *)path)->CopyPlan(rp);
            else if (AiMaps.CanTravelLine(loc, last.m_loc, 0)) {
                ((CAiPath *)path)->CopyPlan(rp);
                rp->InsertPathLoc(loc, AiMaps.GetMapByIndex(loc.m_mapid), 0, true);
            } else {
                route->m_state = ROUTE_2;
                for (int i = 1; i < rp->m_points.size(); i++) {
                    const CAiPath::point &p = rp->m_points[rp->m_points.size() - i - 1];

                    if (p.m_flags & 3)
                        break;
                    if (AiMaps.CanTravelLine(loc, p.m_loc, 0)) {
                        while (i-- > 0)
                            rp->PopPoint(0);
                        route->m_state = ROUTE_1;
                        ((CAiPath *)path)->CopyPlan(rp);
                        ((CAiPath *)path)->SmoothPath(false);
                        break;
                    }
                }
            }
        }
        ((CAiPath *)&route->m_path)->Clear();
        break;
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

/* the SEALs dozing (SOCOM 1's ai::snooze_count, a $gp word there; OVL2 bss 0x66AFF8) */
namespace ai {
extern int snooze_count;
}

/* a doze begun or ended: never for a sleeper or a SEAL that may not doze; begun, the snooze state
 * pushed and set running, the SEAL dropped from the player checks and counted; ended, the state
 * popped; either way the stamp restarted (OVL2 0x56D6A0: SOCOM 1's SetSnoozing, which pushed with a
 * bool, never tested the state for null, kept the entity's bits and the stamp alone) */
/* the sleep bits as bools (inline accessors: the target turns each into a bool, sltu, before its
 * branch, and reloads the byte for them; read in place the tests reuse one load and branch on the
 * bits). In SetSnoozing the state on top is kept only when of the doze's kind through `s = s &&
 * s->m_state == 18 ? s : 0;` -- an if, or a cast inline, fills two delay slots the target leaves
 * empty; the first compare is written `bit == on` there and `on == bit` in SetSleeping (MW loads a
 * comparison's right operand first: the target loads `on` first in the one, the bit in the other) */
static inline bool Sleeping(const CSealCtrlAi *ai)
{
    return ai->m_aibit8 ? true : false;
}

static inline bool Snoozing(const CSealCtrlAi *ai)
{
    return ai->m_aibit9 ? true : false;
}

static inline bool NoSnooze(const CSealCtrlAi *ai)
{
    return ai->m_nosnooze ? true : false;
}

void CSealCtrlAi::SetSnoozing(bool on)
{
    CEntity *e = m_entity;

    if (m_aibit9 == on || Sleeping(this) || NoSnooze(this))
        return;
    if (on) {
        if (PushAiState((AI_STATE)18, 1)) {
            CAiState *s = GetAiState();

            s = s && s->m_state == 18 ? s : 0;

            s->SetFlags(3, 0);
            s->m_active = true;
        }
        e->m_check_player_collision = false;
        e->m_get_new_altitude = false;
        ai::snooze_count++;
        m_aibit9 = true;
    } else {
        PopAiState((AI_STATE)18);
        e->m_check_player_collision = true;
        e->m_get_new_altitude = true;
        m_aibit9 = false;
        ai::snooze_count--;
    }
    m_stamp.Reset();
}

/* asleep or woken: a dozing SEAL is woken to sleep, never to wake; asleep, the node hidden and the SEAL
 * out of the target lists and the player checks, the stack told signal 5; woken, all back, the mode 1
 * and signal 6; then the stamp restarted and the events heard worked out again (OVL2 0x56D0C0: SOCOM
 * 1's SetSleeping, which kept the entity's bits, signalled nothing and printed the alert) */
void CSealCtrlAi::SetSleeping(bool on)
{
    CEntity *e = m_entity;

    if (on == m_aibit8)
        return;
    if (Snoozing(this)) {
        if (on)
            SetSnoozing(false);
        else
            return;
    }
    if (on) {
        e->GetNode()->SetActive(false);
        e->SetUpdateTargetList(false);
        e->SetIncludeInTargetList(false);
        e->m_check_player_collision = false;
        e->m_get_new_altitude = false;
        m_aibit8 = true;
        CAiState::Signal(&m_stateStack, (CAiState::SIGNAL)5, this);
    } else if (!on) {
        e->GetNode()->SetActive(true);
        e->SetUpdateTargetList(true);
        e->SetIncludeInTargetList(true);
        e->m_check_player_collision = true;
        e->m_get_new_altitude = true;
        SetAiMode((AI_MODE)1);
        m_aibit8 = false;
        CAiState::Signal(&m_stateStack, (CAiState::SIGNAL)6, this);
    }
    m_stamp.Reset();
    UpdateEventMask();
}

/* (OVL2 0x56D090) */
void CSealCtrlAi::RestoreAiMode()
{
    SetAiMode(m_3f5);
    m_3f5 = (AI_MODE)1;
}

extern "C" {
/* the time the mode 1 glance starts at (OVL2 0x650098), the SEAL's top test (0x54F6E0, below), the path's
 * deploy (0x56C080, below) */
extern float data_00650098;
bool func_0054F6E0(CZSealBody *seal, bool a, bool b, bool c);
void func_0056C080(CSealCtrlAi *ai, CAiPath *path, const CAiMapLoc *loc);
}

/* the four throttles put to rest through their slot (an inline, one order everywhere) */
static inline void RestThrottles(CSealCtrlAi *ai)
{
    ai->SetThrottle(1, 0.0f);
    ai->SetThrottle(2, 0.0f);
    ai->SetThrottle(0, 0.0f);
    ai->SetThrottle(3, 0.0f);
}

/* the throttles' values zeroed too (the order the stores keep) */
static inline void ZeroThrottles(CSealCtrlAi *ai)
{
    RestThrottles(ai);
    ai->m_throttle[1] = 0.0f;
    ai->m_throttle[2] = 0.0f;
    ai->m_throttle[0] = 0.0f;
    ai->m_throttle[3] = 0.0f;
}

/* a mode set: a held AI stays 6, one on top of something or staying on it 7; leaving 6 for a held AI and leaving
 * 7 while still on top are refused; leaving 7 puts the AI-map collision back, and for 3 the path's next point
 * deployed; entering each mode rests the throttles or sets its own (OVL2 0x56CBF0; SOCOM 1's with the top test,
 * modes 7 and the path's point new).
 * OPEN, 6 instructions and the branch offsets past them (0x18 short): leaving mode 3 the target evaluates a test
 * whose result nothing uses -- the held 4 materialised, then `(state - 1) < 3` with its `sltiu at; beqz at`, an
 * empty arm `b J; nop` (SOCOM 1's had a body there). A dead bool, an empty inline or `if (0)` in the arm drop the
 * second term (here, everything else byte-exact). `int x = (a || b) ? 1 : 0` keeps it at the right size but MW then
 * stops putting `li` and the stack's address in the delay slots before them, all through the function (102 off);
 * `{}` or `(void)0` do the same and drop the term. */
void CSealCtrlAi::SetAiMode(AI_MODE mode)
{
    AI_MODE old;
    CZSealBody *seal = (CZSealBody *)m_entity;

    old = m_3f4;
    if (mode != 7 && AiStateIs((AI_STATE)9))
        mode = (AI_MODE)6;
    else if (func_0054F6E0(seal, true, true, true))
        mode = (AI_MODE)7;
    else if (old == 7 && m_440)
        mode = (AI_MODE)7;
    if (old == mode)
        return;
    switch (old) {
    case 6:
        if (mode != 7 && AiStateIs((AI_STATE)9))
            return;
        break;
    case 3:
        {
            bool on = false;

            if (IsState4Held(this) || (unsigned int)(m_path.m_state - 1) < 3)
                on = true;
        }
        break;
    case 7:
        if (func_0054F6E0(seal, true, true, true))
            return;
        SetDoAiMapCollide((m_448 & 0x80000) != 0);
        m_bit17 = 1;
        if ((m_448 & 0x100000) && mode == 3) {
            CEntity *e = m_entity;
            CAiMapLoc loc;
            CAiPath *path = (CAiPath *)&m_path;

            if (!path->PopPoint(&loc))
                loc = e->m_mapLocValid;
            func_0056C080(this, path, &loc);
        }
        break;
    }
    switch (mode) {
    case 6:
        ZeroThrottles(this);
        break;
    case 2:
        ZeroThrottles(this);
        break;
    case 3:
        func_0056C080(this, (CAiPath *)&m_path, &CAiMapLoc::invalid);
        break;
    case 1:
        RestThrottles(this);
        m_3fc = data_00650098;
        break;
    case 7:
        m_3fc = 0.0f;
        if (m_do_aimap_collide)
            m_448 |= 0x80000;
        else
            m_448 &= ~0x80000;
        m_448 |= 0x200000;
        SetDoAiMapCollide(false);
        break;
    }
    m_3f4 = mode;
    m_3f8 = 0;
    m_3f5 = old;
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

/* the blend stack's top (seal.cpp's stand-in for RTStack<CZBodyAnimBlend>::Top's OVL2 copy) */
extern "C" void *func_00555170(void *stack);

/* whether the AI's anim of a kind still plays on top of the body's blends: never for none (-1); the
 * one the AI made (0x75C) is dropped -- made none, and false -- once its time runs out or, when its
 * priority says so (bit 8), once the alertness's band passes its level (OVL2 0x5712F0, new in SOCOM
 * II: 0x571100, 0x571150 and the AI states' ticks ask it; the name unknown). The level is an int of
 * the word's low byte, taken before the band (its `andi` ahead of the walk), the band AlertBand's
 * enum (`li 4`). The time test reads the clock through a same-type cast (round 34, the HUD's round 33
 * rule): `(float)now > t` loads the clock (0x4365C0) into $f1 ahead of the anim's time, `c.le.s
 * $f1,$f0; bc1t` as the target; plain, every spelling loads the time first (3 off). */
extern "C" bool func_005712F0(CSealCtrlAi *ai, short anim)
{
    if (anim == -1)
        return false;

    CZBodyAnimBlend *top = (CZBodyAnimBlend *)func_00555170((char *)ai->m_entity + 0x1c0);
    bool r = top ? anim == top->m_animType : false;

    if (r && anim == ai->m_sound) {
        if ((float)data_004365C0 > ai->m_sound_time) {
            ai->m_sound = -1;
            r = false;
        } else {
            /* (the priority's word read once: its bit 8 tested, its low byte the level) */
            unsigned int pri = *(unsigned int *)&ai->m_sound_priority;

            if (pri & 0x100) {
                int level = pri & 0xff;

                if (level < AlertBand(((CAlertness *)ai)->m_alert)) {
                    ai->m_sound = -1;
                    r = false;
                }
            }
        }
    }
    return r;
}

extern "C" {
/* the anims the SEAL may be held in (OVL1 data: AnimTypes statics 8 bytes apart -- by SOCOM 1's
 * order, after its DefuseBombEnd pair, likely Ladder2Slide and PistolLadder2Slide) */
extern short data_003DEED0;
extern short data_003DEED8;
/* (OVL2 0x54F6E0, seal.cpp's span: a test of the body's top blend, asked with three trues) */
bool func_0054F6E0(CZSealBody *seal, bool a, bool b, bool c);
/* (seal_body.cpp's: a blend pushed for an anim type in a state, OVL2 0x588BC0) */
void *func_00588BC0(CZSealBody *seal, AnimType type, int state, int flag, int kind);
}

/* an anim the AI plays: the sound's anim forgotten first; the state 11 the SEAL's own stance; never
 * while held in either of two anims, while the top blend says so, by a dead SEAL, for none, or when
 * it already plays -- the three CQB falls (76, 79, 80) pushed in the state 8 with the SEAL killed
 * (DEATH_CQB), any other after the body cleared to its base, in the state; whether it was played
 * (OVL2 0x571150, new in SOCOM II: 0x571010 and the AI states call it; the name unknown) */
extern "C" bool func_00571150(CSealCtrlAi *ai, short anim, SEAL_STATE state)
{
    bool r = false;
    CZSealBody *seal = (CZSealBody *)ai->m_entity;

    ai->m_sound = -1;
    if (state == stateDone)
        state = seal->GetStance(true);

    bool held = seal->IsCurrentBlend((AnimType)data_003DEED0) || seal->IsCurrentBlend((AnimType)data_003DEED8);
    bool top = func_0054F6E0(seal, true, true, true);

    if (seal->m_isAlive && !top && !held) {
        if (anim != -1 && !func_005712F0(ai, anim)) {
            switch (anim) {
            case 79:
            case 80:
            case 76:
                func_00588BC0(seal, (AnimType)anim, 8, 0, 0);
                seal->SetHealth(0.0f, DEATH_CQB);
                r = true;
                break;
            default:
                seal->ClearToBase(false);
                func_00588BC0(seal, (AnimType)anim, state, 0, 0);
                r = true;
                break;
            }
        }
    }
    return r;
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

/* the sleep check's turn (OVL2 0x56D2B0, new in SOCOM II: the fifth frame slice, by the entity's id):
 * a SEAL with a region and a team falls asleep when no live one of the team stands in it, and wakes
 * when one does; one without them, on its turn, widens the window by one (the name unknown) */
extern "C" void func_0056D2B0(CSealCtrlAi *ai)
{
    int id = ai->m_entity->m_id;

    if (!ai->m_270 || !ai->m_274) {
        if (ai_slice4.Has(id))
            ai_slice4.SetSize(ai_slice4.m_size + 1, ai_slice4.m_total);
    } else if (ai_slice4.Has(id)) {
        ai_slice4.m_count++;
        bool in = func_0056D530((const CAiMap::region *)ai->m_274, ai->m_270);

        ai->SetSleeping(!in);
    }
}


/* the leg to walk: with a planned path's points, from the cell to the path's last point -- both
 * centres flat on the ground, the direction and length between them, the leg's words cleared; else
 * just the cell (the SEAL's own when none is given) and the deploying look dropped (OVL2 0x56C080,
 * new in SOCOM II: SetAiMode, PathFollowTick and 0x56C010's pop of a point call it; the name
 * unknown) */
extern "C" void func_0056C080(CSealCtrlAi *ai, CAiPath *path, const CAiMapLoc *loc)
{
    CEntity *e = ai->m_entity;

    ai->m_strafe = 0.0f;
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
        ai->m_leg_done = 0.0f;
        ai->m_60c = 0;
        ai->m_strafe = 0.0f;
        ai->m_strafe_time = 0.0f;
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

/* (OVL2 0x56D890). Byte-exact since round 33 (the kit's round-34 measurement): both team tests read
 * the team through a const view of the controller -- the target loads the entity and its team straight
 * into a0 for each test and reads m_entity again for the kit's, where plain reads let MW keep the
 * entity in a1 across all three (round 32's 9 instructions) */
void CSealCtrlAi::SetAiBit2(bool on)
{
    if ((((const CSealCtrlAi *)this)->m_entity->m_team & 0x20000) ||
        (((const CSealCtrlAi *)this)->m_entity->m_team & 0x4000000) || !SealOf(this)->m_kit.GetCurItem())
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

/* a ladder and the mission's list of them (seal_env.cpp's stand-ins: SOCOM 1's CLadderData and
 * CLadderList, its GetLadderByIndex OVL1 0x2A9210) */
class CLadderData {
public:
    unsigned char _0[0x1c];
    int m_users;   // 0x1c: the SEALs on it
    unsigned char _20[0x28 - 0x20];
};

class CLadderList {
public:
    CLadderData *GetLadderByIndex(int index);
};

/* the mission (theMission, OVL1 bss 0x4364E0) -- only its clock (remote_control.cpp's reading), its
 * ladders (seal_env.cpp's reading), and the team valves refreshed and the fade to the mission's end
 * (SOCOM 1's; OVL1 0x2B23A0, 0x2A97C0) */
class CMission {
public:
    void RefreshTeamValves();
    void FadeToEndMission(float time, int how);
    /* an entity put back in the mission's pool (SOCOM 1's; OVL1 0x2B2310) */
    void RecycleEntity(CEntity *e);

    unsigned char _0[0xe0];
    float m_time;   // 0xe0
    unsigned char _e4[0x1804 - 0xe4];
    CLadderList *m_ladders;   // 0x1804
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
/* whether the SEAL can take a stance where it stands (OVL2 0x562C80, defined below: an int the
 * caller makes a bool) */
bool func_00562C80(CSealCtrlAi *ai, SEAL_STANCE stance);
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
/* the AI states' range (the kit lane's): the aim's direction in the SEAL's frame set, or for none
 * the reference direction (0x60F9F0, seal_aivehicle.cpp's), the aim's point set (0x60FA90) -- each
 * whether the body took it -- and the aim's cosine, 1 in a turret (0x60F9B0) */
bool func_0060F9F0(CSealCtrlAi *ai, const CPnt3D *dir);
bool func_0060FA90(CSealCtrlAi *ai, const CPnt3D *point);
float func_0060F9B0(CSealCtrlAi *ai);
}

namespace ai {
/* a clear line from the SEAL to its target, within a slack (SOCOM 1's; OVL2 0x609DF0, ai_target.cpp's) */
int CheckLineOfFire(const CEntity *e, const CTarget *target, float slack);
}

/* the shot taken once it may be: the time run up; under 0x2000000 the look turned onto the fire's
 * direction (11) and no shot until the body faces within acos 0.92 of it; the body able to fire and,
 * under 0x800000, a clear line to the target (8 of slack); then the aim set -- in the SEAL's frame
 * (0x100000) or at a point (0x400000), else the two aim flags dropped and the time restarted -- and,
 * under 0x1000000, the weapon raised and the shot only once the aim is within acos 0.98 with the
 * weapon's state 1; under 0x40 a delay run down first (OVL2 0x563BD0: SOCOM 1's FireWhenReady, its
 * flags a bit higher, the static cosine 0.92 where SOCOM 1's was 0.98, the slack 8 where SOCOM 1's
 * was 10, the aim through the AI states' helpers, and the delay new). What the target fixes: the
 * fire test one `&&`/`||` expression, CanFire's bool taken through `(int)` (the target's andi before
 * the sltu: unconverted, MW gives the sltu alone), the aim's two flag tests through an inline (one load
 * for both, the drop's own load after them, as the target has it -- tested in place, MW reuses the
 * first load for the drop and fills no delay slot), `dot > 0.98f` (bc1f, the 1 in its delay slot) */
static inline unsigned int Flag(const AI_FIREPARAMS &q, unsigned int f)
{
    return q.m_flags & f;
}

bool CSealCtrlAi::FireWhenReady(AI_FIREPARAMS &p, float dT)
{
    CZSealBody *seal = (CZSealBody *)m_entity;
    static float s_cos = 0.92f;
    static float s_slack = 8.0f;

    p.m_timeout += dT;
    if (p.m_flags & 0x2000000) {
        if (m_look_mode != AIL_11)
            SetLookDirW(AIL_11, &p.m_lookDirW, true);

        const CMatrix &m = seal->m_node->m_matrix;

        if (-m.m_matrix[2][0] * p.m_lookDirW.x + -m.m_matrix[2][1] * p.m_lookDirW.y +
            -m.m_matrix[2][2] * p.m_lookDirW.z < s_cos)
            return false;
    }

    bool fire = (int)seal->CanFire(true, true) &&
                (!(p.m_flags & 0x800000) || ai::CheckLineOfFire(seal, (const CTarget *)m_67c, s_slack));

    if (fire) {
        if (Flag(p, 0x100000))
            fire = func_0060F9F0(this, &p.m_aimDirM);
        else if (Flag(p, 0x400000))
            fire = func_0060FA90(this, &p.m_aimPointW);
        else {
            p.m_flags &= ~0x4200000;
            p.m_timeout = 0.0f;
            fire = false;
        }
        if (fire && (p.m_flags & 0x1000000)) {
            seal->SetWeaponState((WEAPON_STATE)1);
            fire = func_0060F9B0(this) > 0.98f && seal->m_weapon_state == 1;
        }
        if (fire && (p.m_flags & 0x40)) {
            p.m_delay -= dT;
            fire = p.m_delay <= 0.0f;
        }
    }
    return fire;
}

/* two bytes of OVL2 bss either of which keeps the duck off (debug switches? the names unknown) */
extern "C" unsigned char data_0066B040;
extern "C" unsigned char data_0066B038;

/* whether to duck for the squad's leader -- or the leader given: a Navy SEAL's, within the reach
 * (100 in the CQB state 9, else 75) and in front of the leader's facing (a cosine of 0.9), the stance
 * taken a crouch when the leader's stands, prone otherwise (OVL2 0x5632A0: SOCOM 1's GetDuckStance,
 * after SetStance in both builds -- SOCOM II adds the switches, the Navy SEAL's test and the CQB
 * reach, and reads the positions and the leader's matrix straight where SOCOM 1 called virtuals) */
bool CSealCtrlAi::GetDuckStance(CZSealBody *leader, SEAL_STANCE &stance) const
{
    static float reach_cqb = 100.0f;
    static float reach = 75.0f;
    static float facing = 0.9f;
    bool ok = false;
    CEntity *e = m_entity;

    if (!data_0066B040 && !data_0066B038 && IsNavySeal()) {
        if (!leader)
            leader = m_unit->GetTop()->GetLeader();
        if (leader) {
            CPnt3D d;

            e->m_posW.Sub(&leader->m_posW, &d);

            float dist = d.Normalize();

            if (dist < (AiStateIs((AI_STATE)9) ? reach_cqb : reach)) {
                const CMatrix &m = leader->m_node->m_matrix;

                if (-m.m_matrix[2][0] * d.x + -m.m_matrix[2][1] * d.y + -m.m_matrix[2][2] * d.z > facing) {
                    if (CZSealBody::GetStanceFromState(leader->GetStance(false), stance) == STANCE_STAND)
                        stance = STANCE_CROUCH;
                    else
                        stance = STANCE_PRONE;
                    ok = true;
                }
            }
        }
    }
    return ok;
}

extern "C" {
/* the side a found cell faces from its direction (OVL2 0x55FA90, not written yet) */
unsigned char func_0055FA90(CAiMap *map, const CAiMapLoc &loc, CAiMap::DIR dir);
}

/* a cell to look from, and one to take cover in, near the SEAL's last good cell (its map's
 * FindLookLoc and FindCoverLoc): the side it faces handed back, 2 when none is found (OVL2 0x55F7F0
 * and 0x55F940, new in SOCOM II: CAiSPursue::Tick calls both; the names unknown). Round 36, read off
 * MW's colouring (igdump): the flag defined after the side's store -- every spill cost is 0 here, so
 * the order is the simplify sweeps': defined at its declaration the flag has 26 neighbours and the
 * map leaves in the second sweep ahead of the parameters (s5); defined after the store it has 24, it
 * leaves in the second sweep instead and the map with the parameters in the third, last -- map s0,
 * radius s1, side s2, out s3, pos s4, found s5, as the target */
extern "C" bool func_0055F940(CSealCtrlAi *ai, const CPnt3D *pos, CAiMapLoc &out, unsigned char *side,
                              int radius)
{
    CAiMapLoc loc(ai->m_entity->m_mapLocValid);
    CAiMap *map = AiMaps.GetMapByIndex(loc.m_mapid);
    bool found;
    CAiMap::DIR dir = CAiMap::DIR_COUNT;

    *side = 2;
    found = false;
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
    bool found;
    CAiMap::DIR dir = CAiMap::DIR_COUNT;

    *side = 2;
    found = false;
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
/* (round 36: the result preset and the loop left by `break`, 2 and 3 joined by `||` -- the target's
 * `bne 3` straight to the exit; the weapon id an `int` local, defined after the counter's bare
 * declaration so it is numbered above it: the id a1, the counter a2, the table cursor a0) */
#pragma auto_inline off
extern "C" AI_WEAPON_KIND func_00560540(CSealCtrlAi *ai)
{
    AI_WEAPON_KIND kind = WEAPON_KIND_0;
    int i;
    int id = ((CZSealBody *)ai->m_entity)->m_weapon_id;

    for (i = 0; i < 7; i++) {
        if (id == data_0066AD00[i]) {
            kind = (AI_WEAPON_KIND)i;
            if (kind == WEAPON_KIND_2 || kind == WEAPON_KIND_3)
                kind = WEAPON_KIND_1;
            break;
        }
    }
    return kind;
}
#pragma auto_inline reset

/* (OVL2 0x5605A0, not written yet: a weapon of a kind chosen) */
/* OPEN, unwritten (OVL2 0x5605A0, 0x2F4 bytes; round 59 parked it, nothing here depends on it): `bool F(ai, kind)`:
 * seal = 0x560950(ai) [ai->m_entity]; dead reads 0x560940(seal) [seal->m_id] and 0x560930(ai) [(signed char)ai->m_26c];
 * if (!InGroup || kind == 6) { if (kind == 1) F(chance ? 2 : 3) else { w = data_0066AD00[kind]; if (w != 0x5608A0(seal))
 * { r = 0x58C5D0(seal, w); 0x571000(ai) } } } return r. 0x560920 is a one-float class's constructor and 0x5608B0 its
 * `operator bool`; Rfloat's true constructor is (min, max). `r = F(ai, c ? 2 : 3)` has the right selects but expands the
 * second level's operator bool whole (CInline's second walk: 0x344 vs 0x2F4); `kind = c ? 2 : 3; r = F(ai, kind)` has its
 * Rfloat calls and literal but regenerates the select (0x31C); if/else passes the 15-statement auto limit (`inline`
 * escapes it, then the constructor is called at level 1 and 0x5604D0 inlines F); the level 0's IsKind is one level too
 * deep in all three. Written, it lost 0x5604D0, 0x5609F0 and 0x567690, so it stays out: 0x570F90 and 0x571000 are
 * structurally exact from source, 0x5609F0 as real recursion 1 off. */
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
            ai->PlayAiSound((CHARACTER_SOUND)56, 1.0f, CAiEvent::AIE_NONE);
        else
            ai->PlayAiSound((CHARACTER_SOUND)55, 1.0f, CAiEvent::AIE_NONE);
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
    ctrl->PlayAiSound(sound, 1.0f, CAiEvent::AIE_NONE);
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

extern "C" {
/* whether the SEAL is in a turret (its body's 0xEB8; OVL2 0x60F9A0) and the aim's point set, or for
 * none dropped (OVL2 0x60FA90) -- the AI states' range */
bool func_0060F9A0(CSealCtrlAi *ai);
bool func_0060FA90(CSealCtrlAi *ai, const CPnt3D *point);
}

/* an Rfloat's roll read through a float pointer (its min, then its range): the target recomputes each
 * field's address off the controller across rand(), which this view gives -- the class's operator, a
 * `const Rfloat &` helper or the members written out keep the range's address in a saved register
 * (16 off) or lose the join (0x10 short) */
static inline float RollTime(const float *r)
{
    if (r[1] == 0.0f)
        return r[0];
    float x = 4.656613e-10f * (float)rand();

    return r[0] + r[1] * x;
}

/* an attack let go: out of a turret the weapon put in the state given and the aim's point dropped,
 * the two times rolled again -- the first's, then the first's plus the second's -- the shooting goal
 * cleared and the trigger released (OVL2 0x565B80, new in SOCOM II after SetSprayTimer: AttackTick
 * calls it four times, CAiGroup's 0x60D22C with 2; the name unknown) */
extern "C" void func_00565B80(CSealCtrlAi *ai, WEAPON_STATE state)
{
    CZSealBody *seal = (CZSealBody *)ai->m_entity;

    if (!func_0060F9A0(ai)) {
        seal->SetWeaponState(state);
        func_0060FA90(ai, 0);
    }
    ai->m_748 = RollTime(ai->m_734);
    ai->m_744 = ai->m_748 + RollTime(ai->m_73c);
    ai->ClearGoal(GOAL_SHOOT);
    ai->SetTrigger(false);
}

/* whether a chance comes up (OVL2 0x5608B0; 0x5605A0 calls it) */
extern "C" bool func_005608B0(const float &chance)
{
    return Rfloat(0.0f, 1.0f) <= chance;
}

/* a random value between two bounds (an inline over Rfloat: through it the bounds' registers are the
 * target's -- the Rfloat built in place swaps them; the name ours) */
static inline float RandRange(float lo, float hi)
{
    return Rfloat(lo, hi - lo);
}

/* whether a chance comes up, the chance by value read through a reference (seal_control.cpp's
 * inline: the target stores the chance in a stack slot and reads it back after rand()) */
static inline bool Chance(float chance)
{
    const float &c = chance;

    return Rfloat(0.0f, 1.0f) <= c;
}

/* a sound of the SEAL's character played (SOCOM 1's PlayAiSound(CHARACTER_SOUND), which always played
 * it; OVL2 0x55FF60, slot 0x24 of CSealCtrlAi's vtable): not again within the sound's gap since it last
 * played, then only if the chance comes up; sound 80 only while the AI runs, out of 0x60A850's pursuit
 * and in state 2 or 9 at least 10 s into its step; a dead SEAL's only for sounds 60 and 61. A Navy
 * SEAL's plays from no point and makes its AI event only for event 1, any other SEAL's from where it
 * stands (CSealCtrl's 0x5978A0); the time kept. Whether it played */
bool CSealCtrlAi::PlayAiSound(CHARACTER_SOUND sound, float chance, CAiEvent::EVENT event)
{
    bool played = false;
    CCharacterType *ch;
    float *last;
    bool ok;
    CZSealBody *seal = (CZSealBody *)m_entity;

    if (sound != CHRSND_NONE) {
        ch = seal->GetCharacter();
        last = &data_0066AD20[sound];
        ok = true;
        if (data_004365C0 - *last < data_0066AE80[sound])
            ok = false;

        if (ok && chance < 1.0f)
            ok = Chance(chance);
        if (sound == 80) {
            ok = false;
            if (aiEnabled && !func_0060A850(m_67c) && (AiStateIs((AI_STATE)2) || AiStateIs((AI_STATE)9)))
                ok = GetAiState()->m_phaseTime >= 10.0f;
        }
        bool dying = sound == 60 || sound == 61;

        if (ok && (seal->m_isAlive || dying)) {
            unsigned int id = seal->m_id;
            bool send = event != CAiEvent::AIE_NONE;

            if (m_navyseal) {
                played = ch->PlayAt(sound, id, 0);
                send = event == CAiEvent::AIE_GENERIC;
            } else
                played = ch->PlayAt(sound, id, &seal->m_posW);
            if (send)
                func_0055FEE0(this, event);
            *last = data_004365C0;
        }
    }
    return played;
}

/* neither of SOCOM 1's m_snoozing and m_sleeping set, as a bool -- the `||` MW's own (its join no
 * aligned label: the target's in 0x569AA0 and 0x569C60, where AiAwake's if pads one; the name ours) */
static inline bool AiAwakeNow(const CSealCtrlAi *ai)
{
    return !(ai->m_aibit9 || ai->m_aibit8);
}

/* the nearest SEAL among an entity's targets within a range: alive, awake when the AI drives it, on a
 * team the mask meets or not as asked, and with the flag one whose target bit 9 is set -- the range
 * handed back as the target's. The targets are kept nearest first, so the first past the range ends
 * the walk (OVL2 0x569C60, new in SOCOM II: CAiSHostage::Tick and CAiSSurrender::Tick ask it, and
 * 0x569C50 for the entity's own enemies; the names ours) */
extern "C" CEntity *func_00569C60(CEntity *e, float *range, unsigned int mask, bool member, bool flagged)
{
    CTargetList &targets = e->m_targets;
    float r2 = *range * *range;

    for (CTargetList::iterator i = targets.begin(); i != targets.end(); ++i) {
        CZSealBody *seal = AsSeal(i->m_ent);

        if (!seal || !seal->m_isAlive)
            continue;
        if (AiDriven(seal) && !AiAwakeNow(seal->GetSealCtrlAi()))
            continue;
        if (member != ((mask & seal->m_team) != 0))
            continue;
        if (flagged && !(i->m_flags & 0x200))
            continue;
        if (!(i->m_distSq < r2))
            break;
        *range = i->GetRange();
        return seal;
    }
    return 0;
}

/* an entity's kind by reference (an inline, the name ours: read in a loop through it, MW hoists the
 * kind's address into the preheader) */
static inline const CEntity::TYPE &TypeOf(const CEntity *e)
{
    return e->m_type;
}

/* a turret's flag word (zveh.h's CTurret::m_flags at 0x3A4; only that) and bit 4 clear (an inline
 * returning the negation: materialised, sltu then xori, before the branch) */
struct CTurretFlags {
    bool NoBit10() const { return !(m_flags & 0x10); }

    unsigned char _0[0x3a4];
    unsigned int m_flags;   // 0x3a4
};

/* the entity as a turret, if it is one (an inline, the name ours) */
static inline CTurretFlags *AsTurretFlags(CEntity *e)
{
    return e ? (TypeOf(e) == CEntity::ENT_TYPE_TURRET ? (CTurretFlags *)e : 0) : 0;
}

/* the AI's bit 16 set without bit 17 (an inline, the name ours) */
static inline bool AiBit16(CSealCtrlAi *ai) { return ai->m_aibit16 ? true : false; }
static inline bool AiBit17(CSealCtrlAi *ai) { return ai->m_aibit17 ? true : false; }
static inline bool Bit16Only(CSealCtrlAi *ai)
{
    bool r = AiBit16(ai);

    if (r)
        r = !AiBit17(ai);
    return r;
}
static inline bool NotBit16Only(CSealCtrlAi *ai)
{
    return !(AiBit16(ai) && !AiBit17(ai));
}

/* whether the SEAL may lie down where it stands (inline helpers, the names ours): its cell holds an
 * action, is inside (for a Navy SEAL), or is of soil 7, 6, 11 or 2 -- each step a bool of its own */
static inline bool CellBarsProne(const CAiCell *cell, const CSealCtrlAi *ai)
{
    bool r5 = true;
    bool r4 = true;
    bool r3 = true;
    bool r2 = true;
    bool r1 = true;

    if (!cell->m_action) {
        bool r0 = false;

        if (cell->m_inside && ai->IsNavySeal())
            r0 = true;
        if (!r0)
            r1 = false;
    }
    if (!r1 && cell->m_soil != 7)
        r2 = false;
    if (!r2 && cell->m_soil != 6)
        r3 = false;
    if (!r3 && cell->m_soil != 11)
        r4 = false;
    if (!r4 && cell->m_soil != 2)
        r5 = false;
    return r5;
}

/* whether the AI is no Navy SEAL (the negation an inline's: materialised before the branch) */
static inline bool NotNavySeal(const CSealCtrlAi *ai)
{
    return !ai->IsNavySeal();
}

/* whether the SEAL can take a stance where it stands: standing and crouching always; prone unless the
 * target held (bit 9) is not under bit 16 without 17 and stands high above or below it or within 250,
 * the cell bars it, or the SEAL is hurt below a quarter and no Navy SEAL -- any of those overruled when
 * a state of kind 27 is on the stack, or a restrained SEAL is not in a kind 9 (OVL2 0x562C80, new in
 * SOCOM II: DoSetStance and 0x591520 ask it -- 0x591520 masks the result as a bool; the name unknown;
 * written the round 56). Each test a bool of its own, the cell's ors CellBarsProne's nested bools,
 * the state test AiStateIs's explicit false arm through a bool local (an inline negation, `!` in the
 * `&&` or a bool local in StackIs fold it into branches), 0x60A850 a bool (round 58: 0x60A850 returns
 * bool in the row's own unit, the test `bool ok = func_0060A850(t) && (t->m_flags & 0x200)`), `exposed`
 * declared bare ahead of the SEAL (igreplay --movers: numbered before it, s1 and s0 as the target).
 * OPEN, 17 instructions -- ONE WORD: the bit-16-without-17 test's merge gets an `andi v0,0xff` the
 * target lacks (the other 16 are branch offsets it shifts, until the 8-byte pad after `exposed = true`
 * absorbs it). The andi is MW converting the `!` of the bit-17 bool into the && value (igdump: op1087
 * into the temporary, in the bit-17 block) -- a bool, int, unsigned or unsigned char variable, the
 * helper or the caller, `!(x != 0)`, `x ? false : true`, `^ true`, a `!= 0` accessor, `!x ? true :
 * false`, `(bool)!x` and `x != true` all keep it;
 * `== false` drops it but compiles to xor/sltiu; `(b16 ? !b17 : false) == false` moves it to the merge
 * and drops the first sltu (7); the target's `sltu; xori` reaches the branch clean
 * (build/scratch/seal-r56/v_c80*.py) */
extern "C" bool func_00562C80(CSealCtrlAi *ai, SEAL_STANCE stance)
{
    bool exposed;
    CZSealBody *seal = (CZSealBody *)ai->m_entity;
    bool r = false;

    switch (stance) {
    case STANCE_STAND:
    case STANCE_CROUCH:
        r = true;
        break;
    case STANCE_PRONE: {
        CTarget *t = (CTarget *)ai->m_67c;
        exposed = false;
        bool ok = func_0060A850(t) && (t->m_flags & 0x200);

        if (ok) {
            if (!Bit16Only(ai)) {
                if (t->m_vec.y * t->m_vec.y / t->m_distSq > 0.07f)
                    exposed = true;
                if (t->m_distSq < 62500.0f)
                    exposed = true;
            }
        }
        bool barred = false;
        CAiCell *cell = AiMaps.GetCell(seal->m_mapLocValid);

        if (cell)
            barred = CellBarsProne(cell, ai);
        bool held = true;

        if (!(ai->m_stateStack.m_10 & (1 << 27))) {
            bool e = false;

            bool out = !ai->AiStateIs((AI_STATE)9);

            if (out && seal->m_restrained)
                e = true;
            if (!e)
                held = false;
        }
        bool weak = false;

        if (seal->m_health < 0.25f && NotNavySeal(ai))
            weak = true;
        if ((!exposed && !barred && !weak) || held)
            r = true;
        else
            r = false;
        break;
    }
    }
    return r;
}

/* whether a turret the AI drives may take targets: unless the AI has bit 16 without 17, flagged 2,
 * not 0x10, and 0x20 (an inline, each step a bool of its own; the names ours) */
static inline bool TurretTakesTargets(CSealCtrlAi *ai, const CTurretFlags *t)
{
    bool r = false;
    bool b = false;
    bool a = false;

    if (NotBit16Only(ai) && (t->m_flags & 2))
        a = true;
    if (a && t->NoBit10())
        b = true;
    if (b && (t->m_flags & 0x20))
        r = true;
    return r;
}

/* whether the AI's entity takes targets: a SEAL always, kind 4 never, a turret as above (an inline,
 * the name ours) */
static inline bool TakesTargets(CSealCtrlAi *ai, CEntity *e)
{
    bool r = false;

    if (e) {
        switch (TypeOf(e)) {
        case CEntity::ENT_TYPE_TURRET:
            r = TurretTakesTargets(ai, AsTurretFlags(e));
            break;
        case 4:
            r = false;
            break;
        default:
            r = true;
            break;
        }
    }
    return r;
}

/* the nearest enemy SEAL among the AI's entity's targets within a range of a point: alive, awake
 * when the AI drives it, on no team the entity is on, while the entity takes targets. The targets
 * are kept nearest first, so the first past the range ends the walk (OVL2 0x569E40, new in SOCOM
 * II: 0x561550 and CAiSGoto::Tick ask it; the names ours).
 * OPEN, 23 instructions (the size right; round 50 -- 24 with round 49's `r2 = range; r2 *= r2`, whose
 * second operand MW propagates back to $f12: the parameter squared in place, `range *= range`, gives
 * the target's `mul.s $f20,$f20,$f20`). Right: the kind read through a
 * reference-returning inline (TypeOf: its address hoisted into s0, the kind reused by the turret
 * test), the AI taken non-const (a const one hoists the bits' address 0x266 too), kind 4's
 * `r = false` (its own `b` block). Left: (1) the turret test's `e && kind == 3 ? e : 0` -- the target
 * branches `beqz e` with the zero in the delay slot, then `beq` to the true arm; the plain ternary
 * (as seal.cpp's AsTurret) makes MW hoist `e != 0` into a saved register and materialise the `&&`,
 * the nested ternary used here branches `bne` past the true arm, if/return and a result local are
 * worse (R1-R4, F1-F5 in build/scratch/seal-r49/v_569e40*.py); (2) the AI's bit test: the target
 * computes `bit16 && !bit17` as a value in the last operand's register (`sltu; beqz; ... xori`) and
 * negates it with one `xori` at the join -- every inline form here either folds the negation into
 * the branches (the bits read in place), sets a bool with `daddiu 1` (an `&&` of accessor calls) or
 * renormalises a bool local (`andi 0xff; sltu`) before the `xori`. Round 50 (build/scratch/seal-r50/
 * v_569e40*.py, 56 variants): the target's turret test is a SINGLE-CASE SWITCH's layout (`beq` to the
 * case, `b` to the default, the case's move in the beq's delay slot) with the null test branching to
 * the default's `move a0,zero` -- `r = 0; if (e) switch (TypeOf(e)) { case 3: r = e; }` gives the beq/b
 * but MW drops both zero moves (a0 already 0 from TakesTargets' `r = false`): 17 with the bit test as
 * `!(AiBit16(ai) ? !AiBit17(ai) : AiBit16(ai))` (its false arm then still a `move`), 18 as Bit16Only;
 * `default: r = 0`, an else, or returns in the cases 18-20 or the size off */
extern "C" CZSealBody *func_00569E40(CSealCtrlAi *ai, const CPnt3D *pos, float range)
{
    CEntity *e = ai->m_entity;

    range *= range;
    CTargetList &targets = e->m_targets;

    for (CTargetList::iterator i = targets.begin(); i != targets.end(); ++i) {
        CZSealBody *seal = AsSeal(i->m_ent);

        if (!seal || !seal->m_isAlive)
            continue;
        if (AiDriven(seal) && !AiAwakeNow(seal->GetSealCtrlAi()))
            continue;
        if (seal->m_team & e->m_team)
            continue;
        if (!TakesTargets(ai, e))
            continue;

        CPnt3D d;

        pos->Sub(&seal->m_posW, &d);
        if (!(d.x * d.x + d.y * d.y + d.z * d.z < range))
            break;
        return seal;
    }
    return 0;
}

/* the nearest enemy SEAL among an entity's targets within a range (OVL2 0x569C50: 0x569C60 for a
 * team the entity's own does not meet) */
extern "C" CEntity *func_00569C50(CEntity *e, float *range, bool flagged)
{
    return func_00569C60(e, range, e->m_team, false, flagged);
}

/* the SEAL among an entity's targets standing in a cell: alive, and when the AI drives it awake
 * (OVL2 0x569AA0, new in SOCOM II: 0x56A130's line walk and 0x567900 ask it for what they met; the
 * name ours). The SEAL's cell through CZSealBody::MapLocValid(): the member accessor gives the
 * target's `addiu a0,s1,232` for the y read, the field read straight or a static accessor folds it.
 * Both callers call it out of line (short enough for -inline auto here: pinned) */
#pragma auto_inline off
extern "C" CEntity *func_00569AA0(CEntity *e, const CAiMapLoc *loc)
{
    CTargetList &targets = e->m_targets;

    for (CTargetList::iterator i = targets.begin(); i != targets.end(); ++i) {
        CZSealBody *seal = AsSeal(i->m_ent);

        if (!seal || !seal->m_isAlive)
            continue;
        if (AiDriven(seal) && !AiAwakeNow(seal->GetSealCtrlAi()))
            continue;
        if (SameCell(seal->MapLocValid(), *loc))
            return seal;
    }
    return 0;
}
#pragma auto_inline reset

extern "C" {
/* the side step's limits (OVL2 data: the leg's length left under which it stops, the time between
 * picks, and its least and greatest size) */
extern float data_00650180;
extern float data_00650188;
extern float data_00650190;
extern float data_00650198;
}

/* the side step off the leg being walked: none near the leg's end; picked again once the time since
 * the last pick passes its limit -- from none, the greatest either way at even chances; else even
 * chances of none or a random size the other way -- the time restarted (OVL2 0x5698A0, new in SOCOM
 * II: 0x567900 calls it; the name ours) */
extern "C" float func_005698A0(CSealCtrlAi *ai)
{
    if (!(ai->m_leg_len - ai->m_leg_done <= data_00650180)) {
        if (!(ai->m_strafe_time <= data_00650188)) {
            float side = ai->m_strafe >= 0.0f ? 1.0f : -1.0f;

            if (ai->m_strafe == 0.0f)
                ai->m_strafe = data_00650198 * (Chance(0.5f) ? -1.0f : 1.0f);
            else
                ai->m_strafe = Chance(0.5f) ? 0.0f : -side * RandRange(data_00650190, data_00650198);
            ai->m_strafe_time = 0.0f;
        }
    } else
        ai->m_strafe = 0.0f;
    return ai->m_strafe;
}

/* a direction to look in from a map spot (OVL2 0x559580, not written: its arguments are what
 * 0x567690 passes) */
extern "C" CAiMap::DIR func_00559580(const CAiMapLoc *loc, const CPnt3D *dir, unsigned int mask, CAiMapLoc *out,
                                      int n, int *got);

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
    /* (CreepTick's inline constructor: every member but the met cell, the scale and the way back) */
    SEAL_LINEWALK()
        : m_blocked(0), _1(0), m_met(0), m_metid(CEntity::invalidId), m_8(false), m_restrained(false),
          m_dist(30.0f), m_to(CAiMapLoc::invalid), m_28(0), m_ai(0), m_30(true)
    {
    }

    unsigned char m_blocked;   // 0x0
    unsigned char _1;
    unsigned char m_met;   // 0x2: an entity in the way
    unsigned int m_metid;   // 0x4: CEntity::invalidId at first
    bool m_8;   // 0x8
    bool m_restrained;   // 0x9
    CAiMapLoc m_metloc;   // 0xc
    float m_dist;   // 0x10
    float m_scale;   // 0x14
    CAiMapLoc m_to;   // 0x18
    CPnt3D m_back;   // 0x1c: the SEAL's facing, reversed
    unsigned int m_28;   // 0x28
    CSealCtrlAi *m_ai;   // 0x2c
    bool m_30;   // 0x30: true at first
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

/* the AI's creep stepped (OVL2 0x567350, slot 0x88; new in SOCOM II, SOCOM 1's AI used the base's):
 * with no goal the throttles stopped, the requests too (StopThrottles inlined), and done. Else the
 * creep's bit 4 at 0x264 set; asked to walk, with a goal point and a goal kind other than 6, the line
 * to the point's cell walked (0x56A130, a reach of 7.5) -- a cell off the map or a walk refused drops
 * the walk and the goal point, unless the AI is in mode 7 and met no one. Then the base's creep; the
 * goal gone after it, the AI out of mode 4 (to 1) and the goal cleared -- ClearGoal(m_goal_type): the
 * target keeps the GOAL_NONE test's dead andi with no branch. Byte-exact (round 51) */
void CSealCtrlAi::CreepTick(float dT, bool b)
{
    bool has = (bool)m_goal_dir_valid || (bool)m_goal_pos_valid;

    if (!has) {
        StopThrottles(false);
        return;
    }
    m_aibit4 = true;
    if (b && m_goal_pos_valid && m_goal_type != GOAL_RECOVER) {
        SEAL_LINEWALK w;
        CAiMapLoc loc;

        if (!(AiMaps.GetLocation(&m_goal_pos, loc) ? func_0056A130(this, loc, &w, 7.5f) : false)
            && (m_3f4 != 7 || w.m_met)) {
            b = false;
            m_goal_pos_valid = false;
        }
    }
    CSealCtrl::CreepTick(dT, b);
    has = (bool)m_goal_dir_valid || (bool)m_goal_pos_valid;
    if (!has) {
        if (m_3f4 == 4)
            SetAiMode((AI_MODE)1);
        ClearGoal(m_goal_type);
    }
}

/* the game's camera (camera.h's CAppCamera, OVL1 bss 0x415FF0) -- only the SEAL it shows */
class CAppCamera {
public:
    virtual void Tick(float dT);

    unsigned char _4[0xbc - 0x4];
    CZSealBody *m_bc;   // 0xbc: the SEAL the camera shows
};

extern CAppCamera *appCamera;

extern "C" {
/* the dead AI's times: the corpse's wait before it fades (+0xC) and, with respawns left, before it
 * fades out to respawn (+0x10) (OVL1 data 0x436624, read as a table through a pointer local -- the
 * target keeps its address in s0, a sixth saved register; the names ours) */
extern float data_00436624[5];
/* the dead SEAL's network state sent (OVL1 0x2BD640, seal_control.cpp's caller's) */
void func_002BD640(CEntity *e, int n);
}

/* a node's opacity moved toward a value by 0.75 a second, held within 0..value rising and value..1
 * falling (the pooled bounds 0x64FD88 / 0x64FD90 are 0.0f and 1.0f), the node shown while it is not
 * transparent (an inline, the name ours: the value a by-value parameter whose address _saturate takes).
 * The tests read the opacity's own value (`cur`) and the arms a second local copied from it (`op`, the
 * one whose address _saturate takes): MW then reloads `op` from its slot in each arm, as the target
 * does -- one variable for both propagates the first load into the arms (the SEALs' round 53) */
static inline void FadeToward(zdb::CNode *node, float target, float dT)
{
    float cur = node->m_Opacity;
    float op = cur;

    if (cur != target) {
        float step = 0.75f * dT;

        if (!(target <= cur)) {
            op += step;
            _saturate(&op, 0.0f, target);
        } else {
            op -= step;
            _saturate(&op, target, 1.0f);
        }
        node->m_Opacity = op;
    }
    node->SetActive(op != 0.0f);
}

/* the dead AI's tick (OVL2 0x56EE80, slot 0x14; SOCOM 1's slot alike): with the AI running, out of
 * a group and bit 5 clear -- with no respawn left and the node shown, once the corpse is out of the
 * camera's sight for the table's +0xC (or already fading) the node faded out, its AI event's kind
 * cleared, and the faded node hidden and the entity recycled; with respawns left the time run up --
 * a hidden node asks for the respawn and counts one down, a shown one fades after +0x10. Online the
 * state sent. Byte-exact (round 53): the locals declared bare at the top in the order node, table,
 * flag, params, entity and assigned entity first -- `igreplay --movers` put the params after the
 * flag and the entity after the params (the target: the entity s5, the node s1, the params s3, the
 * table s0, the shown flag s4, this s2); the camera's wait `fading = m_6f0 >= times[3]` (bc1f, where
 * `!(m_6f0 < times[3])` materialises an xori); FadeToward's two locals (above) */
bool CSealCtrlAi::PostMortemPreTick(float dT)
{
    zdb::CNode *node;
    const float *times;
    bool active;
    AI_PARAMS *params;
    CEntity *e;

    e = m_entity;
    node = e->GetNode();
    params = &m_aiParams;
    times = data_00436624;
    active = node->m_active;

    if (!m_aibit5 && aiEnabled && !InGroup()) {
        int n = params->respawn;

        if (n == 0) {
            if (active) {
                bool recycle = false;
                bool fading = node->m_Opacity < 1.0f;

                if (appCamera->m_bc && !fading) {
                    m_6f0 += dT;
                    fading = m_6f0 >= times[3];
                    if (fading)
                        e->SetIncludeInTargetList(false);
                }
                if (fading) {
                    FadeToward(node, 0.0f, dT);
                    CAiEvent *ev = ((CZSealBody *)e)->m_aiEvent;

                    recycle = !node->m_active;
                    if (ev)
                        ev->m_type = 0;
                }
                if (recycle) {
                    node->SetActive(false);
                    theMission.RecycleEntity(e);
                }
            }
        } else {
            m_6f0 += dT;
            if (!active) {
                ((CZSealBody *)e)->m_needs_respawn = 1;
                params->respawn = n - 1;
            } else if (!(m_6f0 < times[4])) {
                FadeToward(node, 0.0f, dT);
                CAiEvent *ev = ((CZSealBody *)e)->m_aiEvent;

                if (ev)
                    ev->m_type = 0;
            }
        }
    }
    if (theNetwork.m_9)
        func_002BD640(e, 0);
    return true;
}

/* a box's eight corners (the name ours) */
struct CBoxCorners {
    CPnt3D m_c[8];
};

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

/* a value within two bounds, as the box tests ask it (inlines, each answer its own bool: the target's
 * ladder of three zeroed bools; the names ours) */
static inline bool InRange(const float &v, const float &lo, const float &hi)
{
    bool r = false;

    if (v >= lo && v <= hi)
        r = true;
    return r;
}

static inline bool InXY(const CBBox &b, const CPnt3D *p)
{
    bool r = false;

    if (InRange(p->x, b.m_min.x, b.m_max.x) && InRange(p->y, b.m_min.y, b.m_max.y))
        r = true;
    return r;
}

static inline bool InBox(const CBBox &b, const CPnt3D *p)
{
    bool r = false;

    if (InXY(b, p) && InRange(p->z, b.m_min.z, b.m_max.z))
        r = true;
    return r;
}

extern "C" CBoxCorners *func_0056AE40(CBoxCorners *c, const CBBox *b);
extern "C" CPnt3D *func_0056AE10(const CMatrix *m, CPnt3D *pts);
/* OVL2's copy of zdb::CNode::GetBBox, called out of line here as in the target (at this unit's
 * inline_depth(8) the inline would expand) */
extern "C" const CBBox *func_00541510(zdb::CNode *node);

/* the lesser and the greater of two floats, by reference, and a box and two corners' union into a box
 * (inlines, the names ours: the operands' trees are the point -- the first box's reads CSE with the
 * test after it, the stores and the second corners' reads stand alone, the x's read twice as an offset-0
 * member is) */
static inline float MinRef(const float &a, const float &b) { return b <= a ? b : a; }
static inline float MaxRef(const float &a, const float &b) { return b >= a ? b : a; }

static inline void BoxUnion(CBBox &o, const CBBox &a, const CPnt3D &lo, const CPnt3D &hi)
{
    o.m_min.x = MinRef(a.m_min.x, lo.x);
    o.m_min.y = MinRef(a.m_min.y, lo.y);
    o.m_min.z = MinRef(a.m_min.z, lo.z);
    o.m_max.x = MaxRef(a.m_max.x, hi.x);
    o.m_max.y = MaxRef(a.m_max.y, hi.y);
    o.m_max.z = MaxRef(a.m_max.z, hi.z);
}

/* whether a point lies in a vehicle's box as its node stands now or, given a time, as it moves through
 * that time (OVL2 0x56A8E0, new in SOCOM II: 0x56A760 asks it; the name unknown) */
extern "C" bool func_0056A8E0(CEntity *vehicle, const CPnt3D *pos, float near)
{
    zdb::CNode *node = vehicle ? vehicle->GetNode() : 0;
    bool r = false;

    if (node) {
        const CBBox *bb = func_00541510(node);
        const CMatrix *m = (const CMatrix *)vehicle->m_node;
        CBBox box;
        CBBox swept;
        CBoxCorners c;

        func_0056AE40(&c, bb);
        func_0056AE10(m, c.m_c);
        func_0056ACE0(&box, c.m_c);
        r = InBox(box, pos);
        if (!r && near > 0.0f) {
            swept = box;

            CPnt3D t = vehicle->m_velW * near;

            swept.m_min.Add(&t, &swept.m_min);
            swept.m_max.Add(&t, &swept.m_max);
            BoxUnion(swept, swept, box.m_min, box.m_max);
            r = InBox(swept, pos);
        }
    }
    return r;
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
/* whether the SEAL is on the ladder (OVL2 0x59D6C0, seal_env.cpp's span: its index converted and
 * looked up) */
bool func_0059D6C0(CZSealBody *seal, CLadderData *ladder);
}

/* the ladder's index the AI's climbing leg names, 0xFFFF for none, and the ladder (inlines, each a
 * ternary in place, its false arm a block of its own: the index is computed whole ahead of the
 * lookup's test, as an argument of the second inline -- called inside its true arm it adds a third
 * test) */
static inline int LegLadderIndex(const CSealCtrlAi *ai)
{
    return ai->m_444 ? ai->m_444->m_ladder : 0xFFFF;
}

static inline CLadderData *LegLadder(const CSealCtrlAi *ai, int index)
{
    return ai->m_444 ? theMission.m_ladders->GetLadderByIndex(index) : 0;
}

/* whether the SEAL must wait at a ladder: not when it is on it already; when someone uses it, or an
 * AI SEAL of a lower id climbs it (mode 7) -- the list walked by its ids, stopping at the first SEAL
 * of an id not below its own (OVL2 0x56BD10, new in SOCOM II: 0x56AF50 asks it; the name unknown) */
extern "C" bool func_0056BD10(CZSealBody *seal, CLadderData *ladder)
{
    bool r = false;

    /* (the redundant false keeps the arm a block of its own, a branch to the shared return: `return
     * r;` returns a constant zero, an empty arm or a goto inverts the test) */
    if (func_0059D6C0(seal, ladder))
        r = false;
    else if (ladder->m_users > 0)
        r = true;
    else {
        const CEntityList &list = CEntity::m_list;

        for (CEntityList::const_iterator i = list.begin(); i != list.end(); ++i) {
            CZSealBody *other = AsSeal(*i);

            if (other && (other->m_control ? ((CSealCtrl *)other->m_control)->IsAi() : false)) {
                if (!(other->m_id < seal->m_id))
                    break;

                CSealCtrlAi *ai = other->GetSealCtrlAi();

                if (ai->m_3f4 == 7 && LegLadder(ai, LegLadderIndex(ai)) == ladder) {
                    r = true;
                    break;
                }
            }
        }
    }
    return r;
}

extern "C" {
/* the mission's vehicles, counted (OVL2 bss: OnMissionStart sets it), the reach within which a target
 * vehicle is looked at, squared, and the nearness asked of its box (OVL2 data, 250000 and 5: read
 * with lwc1, variables; the names ours) */
extern int data_0066AFE8;
extern float data_00650170;
extern float data_00650168;
bool func_0056A8E0(CEntity *vehicle, const CPnt3D *pos, float near);
bool func_00569550(CSealCtrlAi *ai, unsigned int id);
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
/* (OVL2 data: how long the AI's dodge of a vehicle lasts, read as a variable) */
extern float data_006501A0;
}

/* whether the current state is of kind 2 with flag 0x400 (InKind2Flag20's shape) */
static inline bool IsKind2Flag400(CAiState *s)
{
    bool r = false;

    AI_STATE st = s->m_state;

    if (st == (AI_STATE)2 && (s->m_flags & 0x400))
        r = true;
    return r;
}

static inline bool InKind2Flag400(const CAiStateStack &st)
{
    CAiState *s;

    if (st.size() == 0)
        s = 0;
    else
        s = st.GetState();

    return s ? IsKind2Flag400(s) : false;
}

/* the state of kind 2 on top, or none (an inline: the state's kind tested) */
static inline CAiState *AsKind2(CAiState *s)
{
    return s && s->m_state == (AI_STATE)2 ? s : 0;
}

/* a vehicle by its id dodged: alive and moving, and not already dodged (a kind-2 state with flag
 * 0x400 on top), a cell found away along its heading from the SEAL's (0x559580, mask 0xD0004, ten
 * cells), the kind-2 state pushed with flags 0x404 and 4 and its time, and the cell made the avoid
 * goal (0x570F90 told with 1 when it is); whether it was dodged (OVL2 0x569550, new in SOCOM II:
 * 0x56A760 and 0x567900 call it; the name ours) */
extern "C" bool func_00569550(CSealCtrlAi *ai, unsigned int id)
{
    bool r = false;
    const CAiMapLoc *at = &ai->m_entity->m_mapLocValid;
    CEntity *v = AsVehicle(CEntity::m_list.GetEntityById(id));

    if (v && v->m_isAlive) {
        CPnt3D dir = v->m_velW;

        if (dir.Normalize() > 0.0f && !InKind2Flag400(ai->m_stateStack)) {
            CAiMapLoc loc = CAiMapLoc::invalid;

            func_00559580(at, &dir, 0xD0004, &loc, 10, 0);
            if (ai->PushAiState((AI_STATE)2, 3)) {
                CAiState *st = AsKind2(ai->GetAiState());

                st->SetFlags(0x404, 0);
                st->m_time = data_006501A0;
                st->SetFlags(4, 0);
                if (ai->SetGoalPos(GOAL_AVOID, loc, 5.0f))
                    func_00570F90(ai, 1.0f);
            }
            r = true;
        }
    }
    return r;
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

/* the AI's weapon hints (the kit lane's kit.h CAiWepHint, completed here with the static this unit
 * calls: a weapon's accuracy at a distance -- OVL2 0x5C9340) */
class CAiWepHint {
public:
    /* (the defaults, an inline: SetTarget 0x566540 builds one in place -- seal_tasks.cpp's fields; the
     * range a float) */
    CAiWepHint(float range)
    {
        SetMode(0);
        m_2 = 2;
        m_auto = 1;
        m_burst = 1;
        SetBit8(false);
        m_range = 0.0f;
        SetHasWeapon(false);
        SetHigh(0);
        m_range = range;
        SetBit8(true);
    }
    void SetMode(unsigned int m) { m_mode = m; }
    void SetBit8(bool b) { m_bit8 = b; }
    void SetHasWeapon(bool b) { m_hasWeapon = b; }
    void SetHigh(unsigned int n) { m_high = n; }
    static float GetWeaponAccuracy(CZWeapon *w, float d, bool b);

    unsigned int m_mode : 2;   // bits 0-1
    unsigned int m_2 : 2;   // bits 2-3
    unsigned int m_auto : 2;   // bits 4-5
    unsigned int m_burst : 2;   // bits 6-7
    unsigned int m_bit8 : 1;   // bit 8
    unsigned int m_hasWeapon : 1;   // bit 9
    unsigned int m_weapon : 4;   // bits 10-13
    unsigned int m_high : 18;   // bits 14-31
    float m_range;   // 0x4
};

/* the AI's target as the AI holds it (SOCOM 1's CAiTarget at 0x67C, ai_target.cpp's) */
class CAiTarget {
public:
    void SetTarget(CTarget *t);
};

/* a squad's sound (OVL2 0x5EAF20, the kit lane's seal_unit.cpp) */
extern "C" void func_005EAF20(CSealUnit *unit, int sound, int b, float chance);

/* the entity as a turret and whether the AI may attack an entity: a SEAL always, kind 4 never, a turret
 * as above (SOCOM 1's CanAttack, an inline in SOCOM II; the names ours) */
static inline CTurretFlags *AsTurret(CEntity *e)
{
    CTurretFlags *t = (CTurretFlags *)e;

    if (!e || e->m_type != CEntity::ENT_TYPE_TURRET)
        t = 0;
    return t;
}

static inline bool NotBit17(CSealCtrlAi *ai) { return !AiBit17(ai); }
static inline bool NB(CSealCtrlAi *ai) { int r = ai->m_aibit16 != 0; if (r) r = (ai->m_aibit17 != 0) ^ 1; return r ^ 1; }
static inline bool TT(CSealCtrlAi *ai, const CTurretFlags *t)
{
    bool r = false;
    bool b = false;
    bool a = false;

    if (NB(ai) && (t->m_flags & 2))
        a = true;
    if (a && t->NoBit10())
        b = true;
    if (b && (t->m_flags & 0x20))
        r = true;
    return r;
}

static inline bool CanAttack(CSealCtrlAi *ai, CEntity *e)
{
    bool r = false;

    if (e) {
        switch (e->m_type) {
        case CEntity::ENT_TYPE_TURRET:
            r = TT(ai, AsTurret(e));
            break;
        case 4:
            r = false;
            break;
        default:
            r = true;
            break;
        }
    }
    return r;
}

/* a target's entity, if any (an inline, the name ours) */
static inline CEntity *TargetEnt(const CTarget *t)
{
    return t ? t->m_ent : 0;
}

/* a target given to the AI, unless the same one or not taken: its target set and bit 17 cleared; with
 * bit 2 a weapon chosen for the target's range -- a hint's mode 1 under bit 16 without 17 -- and without
 * it, once, an enemy's squad's sound for a player's SEAL (OVL2 0x566540, slot 0xB0: SOCOM 1's SetTarget,
 * SOCOM II's CanAttack inline).
 * OPEN, 0x3B0 against 0x3A8 -- every instruction the target's but: (1) AsTurret's join takes an 8-byte pad
 * the target lacks (its `move t0,zero` runs straight on: `if` pads, every ternary spelling folds into a
 * materialised sltu / movz; two words then shift two more pads); (2) the mode: the target's `li v0,1` comes
 * from the block's top (a logical temporary, live through the hint's stores: no block temporary may take v0),
 * `int mode = 1` declared ahead of the hint gets the same shape in a0 -- the node is deferred in the target
 * (degree over K, coloured first), and `(bit16 && !bit17)` written as a value materialises with sltu instead
 * (build/scratch/seal-r58/v_540*.py). */
void CSealCtrlAi::SetTarget(CTarget *target)
{
    if (target ? ((CAiTargetFlags *)m_67c)->m_entity == target->m_ent : !((CAiTargetFlags *)m_67c)->m_entity)
        return;
    CEntity *e = TargetEnt(target);

    if (e ? CanAttack(this, e) : true) {
        ((CAiTarget *)m_67c)->SetTarget(target);
        m_aibit17 = false;
        CEntity *t = TargetEnt(target);

        if (m_navyseal) {
            float range = target ? target->GetRange() : m_6ec;
            int mode = 1;
            CAiWepHint hint(range);

            if (!m_aibit16 || m_aibit17)
                mode = 0;
            hint.m_mode = mode;
            ChooseWeapon(hint);
        } else if (!data_0066B008 && t && (t->m_team & 2)) {
            CZSealBody *s = AsSeal(t);
            CSealUnit *unit = s ? ((CSealCtrl *)s->m_control)->m_unit : 0;

            if (unit)
                func_005EAF20(unit, 63, 0, 1.0f);
            data_0066B008 = 1;
        }
    }
}

extern "C" {
/* the fire parameters aimed straight at the target's spot (OVL2 0x55EBE0, below) and shot (OVL2
 * 0x55ECC0, not written yet; the names unknown) */
bool func_0055EBE0(CSealCtrlAi *ai, AI_FIREPARAMS *p);
bool func_0055ECC0(CSealCtrlAi *ai, AI_FIREPARAMS *p);
}

/* a vector's length on the ground plane (an inline reading through a reference: the x read again
 * for the scaling after it) */
static inline float FlatLength(const CPnt3D &v)
{
    return sqrtf(v.x * v.x + v.z * v.z);
}

/* a fire parameters' flags changed: some cleared, some set (an inline: two read-modify-writes, `&= ~0`
 * a bare zero extension; the name ours) */
static inline void ModifyFireFlags(AI_FIREPARAMS *p, unsigned int set, unsigned int clear)
{
    p->m_flags &= ~clear;
    p->m_flags |= set;
}

/* the weapon a launcher (items 121-128: a switch, its jump table), a rocket (151, 153, 152) or an
 * automatic (171-179, 143, 142, a switch written ascending: its compare chain descends) -- the rocket
 * none for no weapon (inlines, the names ours) */
static inline bool LauncherId(EQUIP_ITEM id)
{
    switch ((unsigned char)id) {
    case 121:
    case 122:
    case 123:
    case 124:
    case 125:
    case 126:
    case 127:
    case 128:
        return true;
    }
    return false;
}

static inline bool RocketId(unsigned char id)
{
    switch (id) {
    case 152:
    case 153:
    case 151:
        return true;
    }
    return false;
}

static inline bool IsRocket(const CZWeapon *w)
{
    return w ? RocketId((unsigned char)w->m_ID) : false;
}

static inline bool IsAutomatic(EQUIP_ITEM id)
{
    switch ((unsigned char)id) {
    case 142:
    case 143:
    case 171:
    case 172:
    case 173:
    case 174:
    case 175:
    case 176:
    case 177:
    case 178:
    case 179:
        return true;
    }
    return false;
}

extern "C" {
/* (OVL2 0x58F6E0, seal_orders.cpp's: a cell near a point, along a line from another) */
bool func_0058F6E0(const CPnt3D *from, const CPnt3D *to, CAiMapLoc *out, bool along);
/* (OVL2 0x555C50: a throw found from a point to another -- its landing point and cell; unwritten) */
bool func_00555C50(CSealCtrlAi *ai, const CPnt3D *from, const CPnt3D *to, CPnt3D *pos, CAiMapLoc *loc);
/* (OVL2 data: the wait a launcher's round is given, read as a variable) */
extern float data_0064FE38;
}

/* the deploy state on top, or none (an inline: the state's kind tested) */
static inline CAiSDeploy *AsDeploy(CAiState *s)
{
    return s && s->m_state == AI_STATE_DEPLOY ? (CAiSDeploy *)s : 0;
}

/* a grenade of the item deployed at a point from another (the SEAL's own when none): a rocket's cell
 * along the line (0x58F6E0), else a throw (0x555C50, flag 0x40) or the cell short of the point --
 * levelled with the line for all but a launcher's round; with a cell and an item the deploy state
 * pushed and given the point, a zero aim, the cell, the item and its wait -- 15 for a rocket, 5 else,
 * a launcher's round's 121 its own wait and flag 0x10 but for a navy SEAL -- and the grenade's time
 * taken; whether it was (OVL2 0x564D20, new in SOCOM II: the grenade choice 0x565110, OnCommand and
 * the breach's Tick call it; the name ours). Byte-exact (the SEALs' round 57, unwritten before) with:
 * the point declared ahead of the aim (the later-declared takes the lower slot); the throw's test
 * negated, its true arm last (laid out after the cell's path); the item compared and switched on as
 * `(unsigned char)item` -- the one expression RocketId's and LauncherId's conversions share, kept in s0
 * (a plain `item != EQUIP_NONE` normalises it again and swaps the item's and the flags' registers; a
 * plain `switch (item)` shares the 0x70 store's zero extension) */
extern "C" bool func_00564D20(CSealCtrlAi *ai, EQUIP_ITEM item, const CPnt3D *target, const CPnt3D *from,
    unsigned int flags)
{
    bool r = false;
    CEntity *seal = ai->m_entity;
    CPnt3D pos;
    CPnt3D aim = CPnt3D::zero;
    CAiMapLoc loc = CAiMapLoc::invalid;

    if (!from)
        from = &seal->m_posW;
    if (RocketId(item)) {
        if (func_0058F6E0(target, from, &loc, true))
            r = AiMaps.GetCellCenter(loc, &pos) != 0;
    } else if (!func_00555C50(ai, from, target, &pos, &loc)) {
        if (func_0058F6E0(target, from, &loc, false)) {
            r = AiMaps.GetCellCenter(loc, &pos) != 0;
            if (!LauncherId(item)) {
                CPnt3D out;
                float t;

                if (pos.DistToVector(from, target, &out, &t))
                    pos.y = out.y;
            }
        }
    } else {
        r = true;
        flags |= 0x40;
    }
    if (r && (unsigned char)item != EQUIP_NONE && ai->PushAiState(AI_STATE_DEPLOY, 0)) {
        CAiSDeploy *st = AsDeploy(ai->GetAiState());

        st->SetFlags(flags, 0);
        st->m_58 = pos;
        st->m_64 = aim;
        st->m_74 = loc;
        st->m_70 = item;
        switch ((unsigned char)item) {
        case 151:
        case 152:
        case 153:
            st->m_time = 15.0f;
            break;
        case 121:
            st->m_time = 5.0f;
            if (!ai->IsNavySeal()) {
                st->m_88 = data_0064FE38;
                st->SetFlags(0x10, 0);
            }
            break;
        default:
            st->m_time = 5.0f;
            break;
        }
        ai->m_770 = data_004365C0;
    } else
        r = false;
    return r;
}

/* the fire parameters set up from the kit for a target -- its spot and its velocity (none: still) --
 * unless flag 0x4000000 holds them: reset, the kit's item and ammunition taken, the flags given with
 * 0x10000; a launcher thrown (0x60000) and a rocket aimed straight (0x80000), an automatic on a fire
 * mode from 4 up sprayed (0x20000), the throw and the spray after half a second (flag 0x40) (OVL2
 * 0x55F3B0, new in SOCOM II: CAiSDeploy::Tick and 0x563E10 call it; the name unknown) */
extern "C" bool func_0055F3B0(AI_FIREPARAMS *p, CZKit *kit, const CPnt3D *pos, const CPnt3D *vel,
                              unsigned int flags)
{
    if (p->m_flags & 0x4000000)
        return false;
    p->reset();
    p->m_weapon = kit->GetCurItem();
    p->m_ammo = kit->GetCurAmmo();
    p->m_targetPos = *pos;
    p->m_targetVel = vel ? *vel : CPnt3D::zero;
    p->m_flags = flags | 0x10000;
    p->m_weapon->GetIDType();
    /* (the id read once, signed, kept for the last test: lb, then each test narrows it; a test of a
     * mode `> 3` -- `>= 4` materialises the compare) */
    EQUIP_ITEM id = p->m_weapon->m_ID;

    if (p->m_weapon ? LauncherId(id) : false) {
        ModifyFireFlags(p, 0x60000, 0);
        p->m_delay = 0.5f;
        ModifyFireFlags(p, 0x40, 0);
    } else if (IsRocket(p->m_weapon))
        ModifyFireFlags(p, 0x80000, 0);
    else if (IsAutomatic(id) && kit->GetCurFireMode() > 3) {
        ModifyFireFlags(p, 0x20000, 0);
        p->m_delay = 0.5f;
        ModifyFireFlags(p, 0x40, 0);
    }
    return true;
}

/* the fire parameters worked out, unless flag 0x4000000 holds them: thrown by 0x55E670 (flag 0x40000)
 * or aimed straight by 0x55EBE0 (flag 0x80000) -- the trigger let go twice -- else shot by 0x55ECC0;
 * once worked out, under flag 0x2000000 the look flattened onto the ground, flag 0x200000 set and the
 * wait cleared, and with a weapon the spread from its accuracy at the distance (1 in state 4, held to
 * [0.25, 1]) times the row's first figure (1.5 for a Navy SEAL; halved against a valid target whose
 * SEAL is in state 2) times the AI's accuracy factor, held to [0, 1]; whether flag 0x200000 is on
 * (OVL2 0x55F0A0, new in SOCOM II: CAiSDeploy::Tick and 0x563E10 call it; the name unknown).
 * Byte-exact (round 49): 0x55EBE0 pinned out of line; the look's ground length through a reference
 * inline (x read again for the scaling); the Navy SEAL bit an `unsigned char` local (andi 0xff),
 * tested again as `(bool)navy == false` (the sltu in both arms of the ternary before it); the target
 * pointer taken first and tested after the validity call */
extern "C" bool func_0055F0A0(CSealCtrlAi *ai, AI_FIREPARAMS *p)
{
    unsigned int flags = p->m_flags;

    if (!(flags & 0x4000000)) {
        bool done;

        if (flags & 0x40000) {
            done = func_0055E670(ai, p);
            ai->SetTrigger(false);
            ai->SetTrigger(false);
        } else if (flags & 0x80000) {
            done = func_0055EBE0(ai, p);
            ai->SetTrigger(false);
            ai->SetTrigger(false);
        } else
            done = func_0055ECC0(ai, p);
        if (done) {
            if ((p->m_flags & 0x2000000) && p->m_lookDirW.y != 0.0f) {
                p->m_lookDirW.y = 0.0f;

                float len = FlatLength(p->m_lookDirW);

                if (len != 0.0f) {
                    float inv = 1.0f / len;

                    p->m_lookDirW.x *= inv;
                    p->m_lookDirW.z *= inv;
                }
            }
            ModifyFireFlags(p, 0x200000, 0);
            p->m_timeout = 0.0f;

            CZWeapon *w = (CZWeapon *)p->m_weapon;

            if (w) {
                float dist = p->m_dist;
                float acc;

                if (ai->AiStateIs((AI_STATE)4))
                    acc = 1.0f;
                else
                    acc = CAiWepHint::GetWeaponAccuracy(w, dist, false);
                _saturate(&acc, 0.25f, 1.0f);

                unsigned char navy = ai->m_navyseal;
                float f = navy ? 1.5f : data_0064FE60[data_0066AFD8][0];

                if ((bool)navy == false) {
                    static float against_down = 0.5f;
                    void *t = ai->m_67c;

                    if (func_0060A850(t) && t) {
                        CZSealBody *seal = AsSeal(*(CEntity **)t);

                        if (seal && seal->m_state == 2)
                            f *= against_down;
                    }
                }

                float spread = f * (acc * ai->GetAccuracyFactor());

                _saturate(&spread, 0.0f, 1.0f);
                p->m_spread = spread;
            }
        }
    }
    return (p->m_flags & 0x200000) != 0;
}

/* the fire parameters aimed straight at the target's spot: the look along the line to it, the aim
 * point the spot, the model's aim down -z, flag 0x100000 on and 0x800000 off, the distance kept
 * (OVL2 0x55EBE0, new in SOCOM II: 0x55F0A0 picks it by flag 0x80000 and calls it out of line --
 * short enough for -inline auto there: pinned; the name unknown) */
#pragma auto_inline off
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
#pragma auto_inline reset

/* whether the body holds a weapon of the kind: kind 1 is rows 2 and 3 (OVL2 0x5609F0; round 36: a
 * result local set in an if/else, kind 1's rows through an inline -- and the other kind's row read
 * into an `int` local first: MW evaluates `data[k] == seal->m_weapon_id` id first whichever way it
 * is written, so the id took a register the narrowed kind still held; the element first leaves the
 * id the kind's v1 and the seal a2, as the target's) */
/* kind 1's two rows tested as a value, the second only when the first fails: the inline's bool
 * narrowed into the caller's at the join (the target's `andi` in the `b`'s delay slot) */
static inline bool IsKind1(CZSealBody *seal)
{
    bool r = data_0066AD00[2] == seal->m_weapon_id;

    if (!r)
        r = data_0066AD00[3] == seal->m_weapon_id;
    return r;
}
extern "C" bool func_005609F0(CSealCtrlAi *ai, unsigned char kind)
{
    CZSealBody *seal = (CZSealBody *)ai->m_entity;
    bool r;

    if (kind == 1)
        r = IsKind1(seal);
    else {
        int w = data_0066AD00[kind];

        r = w == seal->m_weapon_id;
    }
    return r;
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

extern "C" {
/* one over the tick (SOCOM 1's _dTI, a static of its unit: OVL2 bss 0x66B100 -- SlideTick sets it,
 * CreepTick, 0x567900 and CSealCtrlAi::PreTick read it) */
extern float data_0066B100;
}

/* the goal's countdown run down, true once it has (an infinite one never runs: the turret's
 * TickTimer, veh_turret.cpp) */
static inline bool GoalExpired(const CAiTimer &t)
{
    return t.m_timer != CAiTimer::infinite && t.m_timer <= 0.0f;
}

/* a vector's length on the ground plane */
static inline float LengthXZ(const CPnt3D &v)
{
    return sqrtf(v.x * v.x + v.z * v.z);
}

static inline bool TickGoalTimer(CAiTimer &t, float dT)
{
    bool r = false;

    if (t.m_timer != CAiTimer::infinite) {
        t.m_timer -= dT;
        r = GoalExpired(t);
    }
    return r;
}

/* zero a small object's bytes (an inline: the target's byte loop, with the address's null test) */

static inline float LenXZ(const CPnt3D &v)
{
    return sqrtf(v.x * v.x + v.z * v.z);
}

extern "C" {
extern float data_00650128;
extern float data_0065012C;
extern float data_00650130;
extern float data_00650134;
extern float data_0064FDA0;
extern float data_006501D8;
extern float data_006501E0;
extern float data_006501E8;
}

static inline float DotP(const CPnt3D &a, const CPnt3D &b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

/* the creep toward the goals: the goal's timer run down, an expired goal cleared; the facing laid flat; a
 * goal position walked at -- the throttles a critically damped pull along and across, held within the speed
 * (the side's to 1), the point dropped once within its tolerance; a goal direction turned to by the slot
 * 0x9C's throttle, while at the point or an AI in mode 7, and dropped when reached; the three throttles set
 * (OVL2 0x566DC0, slot 0x88: SOCOM 1's CreepTick, the flag new) */
void CSealCtrl::CreepTick(float dT, bool walk)
{
    float dot;
    float hi;
    float x;
    float lo;
    float top;
    float z;
    float vz;

    data_0066B100 = 1.0f / dT;

    bool has = (bool)m_goal_dir_valid || (bool)m_goal_pos_valid;

    if (!has)
        return;
    if (TickGoalTimer(m_goal_timer, dT)) {
        ClearGoal(m_goal_type);
        return;
    }
    const CMatrix *m = &m_entity->m_node->m_matrix;
    CPnt3D dir(-m->m_matrix[2][0], 0.0f, -m->m_matrix[2][2]);

    dir.Normalize();

    CPnt3D right(-dir.z, 0.0f, dir.x);
    CPnt3D delta(0.0f, 0.0f, 0.0f);
    float t[3] = { 0.0f };
    bool arrived = true;

    if (m_goal_pos_valid && walk) {
        m_goal_pos.SubXZ(&m_entity->m_posW, &delta);

        float dist = LenXZ(delta);

        if (dist != 0.0f) {
            float inv = 1.0f / dist;

            delta.x *= inv;
            delta.z *= inv;
        }

        float along = DotP(dir, delta);
        float across = DotP(right, delta);

        if (dist < data_006501D8)
            arrived = false;

        float vx = m_throttle[0];
        static float rest0 = data_00650130;

        float speed = m_speed;
        float goal = 0.0f;

        x = rest0 * goal - (data_00650130 * (-dist * along) + data_00650134 * vx);
        x *= data_0066B0F8;
        x += vx;
        hi = speed;
        lo = -speed;
        _saturate(&x, lo, hi);
        t[0] = x;

        vz = m_throttle[1];

        static float rest1 = data_00650128;

        top = ai::_min((float)m_speed, data_0064FDA0);
        z = rest1 * goal - (data_00650128 * (-dist * across) + data_0065012C * vz);
        z *= data_0066B0F8;
        z += vz;
        _saturate(&z, -top, top);
        t[1] = z;
        t[2] = 0.0f;
        if (dist <= m_goal_tolerance) {
            m_goal_pos_valid = false;
            arrived = true;
        }
    }

    CSealCtrlAi *ai = GetSealCtrlAi();
    bool turn = !m_goal_pos_valid || arrived || (ai && ai->m_3f4 == 7);
    dot = 0.0f;
    if (m_goal_dir_valid && turn) {
        t[2] = ComputeTurnThrottle(&dir, &m_goal_dir, &dot);
        if (_close_enough(t[2], data_006501E0, data_006501E8))
            m_goal_dir_valid = false;
    }
    SetThrottle(0, t[0]);
    SetThrottle(1, t[1]);
    SetThrottle(2, t[2]);
}


/* the slide toward the goal stepped: the share of what is left this tick (all of it once the goal's
 * time is up), the place's share as the world fudge velocity, the turn's as the rotation's; the goal
 * cleared when it is reached (OVL2 0x566940; SOCOM 1's, its SetGoalPos call now ClearGoal inlined,
 * returning whether the SEAL still slides). Byte-exact (the SEALs, round 44): the dead `sliding =
 * false` ahead of `ent` numbers it first (s3 over ent's s2); the goal bits turned to bools into a
 * local (the target branches on the || value in v0); the time left read through a const view (a
 * second load, the tick's own first); the length and the dot through reference inlines and the side
 * through a scalarised `perp` (a zero register, not a folded literal) keep only &facing.z and
 * &delta.z in saved registers; a bool into a float converts as unsigned; the turn's product taken
 * first, then times _dTI */
bool CSealCtrl::SlideTick(float dT)
{
    bool sliding = false;
    data_0066B100 = 1.0f / dT;
    CEntity *ent = m_entity;

    bool has = (bool)m_goal_dir_valid || (bool)m_goal_pos_valid;

    if (!has)
        return false;

    sliding = true;
    float left = ((const CSealCtrl *)this)->m_goal_timer.m_timer;
    float scale;

    if (TickGoalTimer(m_goal_timer, dT)) {
        scale = 1.0f;
        sliding = false;
    } else if (left < dT) {
        scale = 1.0f;
        sliding = false;
    } else
        scale = dT / left;

    const CMatrix &m = m_entity->m_node->m_matrix;
    CPnt3D facing(-m.m_matrix[2][0], 0.0f, -m.m_matrix[2][2]);

    facing.Normalize();

    CPnt3D delta(0.0f, 0.0f, 0.0f);

    if (m_goal_pos_valid) {
        m_goal_pos.SubXZ(&ent->m_posW, &delta);
        float dist = LengthXZ(delta);

        if (0.0f != dist) {
            float inv = 1.0f / dist;

            delta.x *= inv;
            delta.z *= inv;
        }
        delta.Scale(data_0066B100 * (dist * scale), &m_fudge_velW);
        if (1.0f == scale)
            m_goal_pos_valid = false;
    }
    if (m_goal_dir_valid) {
        float dot = Dot(facing, m_goal_dir);
        CPnt3D perp(-facing.z, 0.0f, facing.x);
        float side = Dot(m_goal_dir, perp);
        float right = side >= 0.0f;

        if (dot < 1.0f) {
            float a = acosf(dot);
            float turn = a * scale;

            m_fudge_velR.y = turn * data_0066B100 * (right ? -1.0f : 1.0f);
        }
    }
    if (!sliding)
        ClearGoal(m_goal_type);
    return sliding;
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

/* a mover: where it is, its heading flattened, the heading's length and the room it keeps (the
 * class and its names ours: 0x557190 builds one for each of its six tests) */
struct CAiMover {
    CAiMover(const CPnt3D &pos, const PNT3D &vel, float radius)
        : m_pos(pos), m_dir(*(const CPnt3D *)&vel), m_radius(radius)
    {
        m_dir.y = 0.0f;
        m_len = m_dir.Normalize();
    }

    CPnt3D m_pos;   // 0x0
    CPnt3D m_dir;   // 0xc
    float m_len;   // 0x18
    float m_radius;   // 0x1c
};

/* whether a mover's path ahead passes within reach of a point, flat on the ground: the turn away
 * handed back -- the way across the path, or turned about by the side the point lies on. An inline
 * past inline_max_size, expanded one level: its own body only, its vector pieces called out of line
 * (their copies 0x557D90-0x557E90 after 0x557190, in reverse first use; the names ours). The mover's
 * fields come in as references (the round 33 fix): their addresses, bound as the inline's
 * parameters, are what the mover's own stores and the Normalize go through -- the target's s1/s2 held
 * from before each mover is built */
static inline bool AvoidPoint(const CPnt3D &pos, const CPnt3D &dir, const float &radius, const CPnt3D &pt,
                              CPnt3D &out, bool turn)
{
    static float s_reach = 20.0f;
    static float s_side = 0.45f;
    bool found = false;
    float reach = s_reach;
    CPnt3D side(-dir.z, 0.0f, dir.x);
    CPnt3D p(pt.x, 0.0f, pt.z);
    CPnt3D q(pos.x, 0.0f, pos.z);
    CPnt3D d = p - q;

    d.y = 0.0f;
    d.Normalize();
    float dot = Dot(d, side);
    CPnt3D ahead = q + dir * radius;
    CPnt3D aside = q + side * (s_side * (radius * func_00557D90(dot)));

    aside.y = 0.0f;
    ahead.y = 0.0f;
    CPnt3D g = ahead - aside;
    CPnt3D h(-g.z, 0.0f, g.x);
    CPnt3D nearest;
    float dist;

    if (p.DistToVector(&aside, &ahead, &nearest, &dist) && dist < reach) {
        if (turn)
            out = h * func_00557D90(dot);
        else
            out = g;
        found = true;
    }
    return found;
}

/* the AI's way past another entity (OVL2 0x557190, new in SOCOM II: 0x556370 calls it; the name
 * unknown): with the player, both moving answers true; else whichever moves is tested against the
 * other's place -- the AI's own move turned across the path, the other's turned about -- and a turn
 * found is scaled and handed to the fudge velocity. With an AI both moving the same way the faster
 * slows (the forward throttle by 0.95), else the same tests as with the player.
 * The copies it leaves out of line match: CPnt3D's +, * and - (0x557DC0, 0x557E10, 0x557E90 -- the
 * round 24 names) and the sign, 0x557D90 (the stand-in below).
 * Byte-exact since round 33: the slowing product its own statement (`t *= s_slow;` -- the target's
 * `mul.s $f20` then `mov.s $f12`, the two instructions round 32 was short), and AvoidPoint taking the
 * mover's fields by reference: the two addresses held across the Normalize add the neighbours that
 * keep the AI deferred past the other four (igdump: all five over K), so it colours first, s0 */
extern "C" bool func_00557190(CSealCtrlAi *ai, CEntity *other)
{
    static float s_still = 25.0f;
    static float s_room = 20.0f;
    static float s_slow = 0.95f;
    static float s_scale = 1.0f;
    CEntity *e = ai->m_entity;
    CPnt3D myVel(e->m_velW);
    CPnt3D otherVel(other->m_velW);
    CPnt3D myPos(e->m_posW);
    CPnt3D otherPos(other->m_posW);
    bool result = false;
    bool meMoving = ai::is_moving(&myVel, s_still);
    bool otherMoving = ai::is_moving(&otherVel, s_still);
    static float s_radius = s_room;

    if (other->m_control ? ((CSealCtrl *)other->m_control)->IsPlayer() : false) {
        if (meMoving && otherMoving)
            result = true;
        else if (meMoving) {
            CAiMover m(myPos, myVel, s_radius);
            bool found = AvoidPoint(m.m_pos, m.m_dir, m.m_radius, otherPos, myVel, false);

            result = found;
            if (found) {
                myVel.Scale(s_scale, &myVel);
                ai->SetFudgeVelW(&myVel);
                result = false;
            }
        } else if (otherMoving) {
            CAiMover m(otherPos, otherVel, s_radius);
            bool found = AvoidPoint(m.m_pos, m.m_dir, m.m_radius, myPos, myVel, true);

            result = found;
            if (found) {
                myVel.Scale(s_scale, &myVel);
                ai->SetFudgeVelW(&myVel);
            }
        }
    } else if (other->m_control ? ((CSealCtrl *)other->m_control)->IsAi() : false) {
        ((CSealCtrl *)other->m_control)->GetSealCtrlAi();
        if (meMoving && otherMoving) {
            CPnt3D d = otherPos - myPos;

            if (Dot(myVel, otherVel) > 0.0f) {
                if (Dot(d, myVel) > 0.0f) {
                    float t = ai->m_throttle[0];

                    if (t > fabsf(other->m_control->m_throttle[0])) {
                        t *= s_slow;
                        ai->SetThrottle(0, t);
                    }
                }
            } else {
                CAiMover m(otherPos, otherVel, s_radius);
                bool found = AvoidPoint(m.m_pos, m.m_dir, m.m_radius, myPos, myVel, true);

                result = found;
                if (found) {
                    myVel.Scale(s_scale, &myVel);
                    ai->SetFudgeVelW(&myVel);
                }
            }
        } else if (meMoving) {
            CAiMover m(myPos, myVel, s_radius);
            bool found = AvoidPoint(m.m_pos, m.m_dir, m.m_radius, otherPos, myVel, false);

            result = found;
            if (found) {
                myVel.Scale(s_scale, &myVel);
                ai->SetFudgeVelW(&myVel);
            }
        } else if (otherMoving) {
            CAiMover m(otherPos, otherVel, s_radius);
            bool found = AvoidPoint(m.m_pos, m.m_dir, m.m_radius, myPos, myVel, true);

            result = found;
            if (found) {
                myVel.Scale(s_scale, &myVel);
                ai->SetFudgeVelW(&myVel);
            }
        }
    }
    return result;
}

/* the cell a step in a direction reaches, and whether it may be stepped on: on the map, with a
 * traversable first-layer cell (an inline, the name ours: GetNeighbor's body with the cell's test) */
static inline bool TraversableNeighbor(CAiMap *map, const CAiMapLoc &loc, CAiMap::DIR dir, CAiMapLoc &out)
{
    int x = loc.x + CAiMap::neighbor[dir][0];
    int y = loc.y + CAiMap::neighbor[dir][1];

    if (x >= 0 && x < map->m_nx && y >= 0 && y < map->m_ny) {
        out.x = x;
        out.y = y;
        out.m_mapid = map->GetMapId();

        CAiCell *cell = map->GetCell(out, 0);

        return cell ? cell->IsTraversable() : false;
    }
    return false;
}

/* whether a soil hides who stands on it (CAiCell::IsConcealingSoil's test on a soil already read:
 * MW lowers the four compares to a bit test, 0x40D over soil - 2) */
static inline bool ConcealingSoil(unsigned int soil)
{
    return soil == 2 || soil == 4 || soil == 5 || soil == 12;
}

/* a cell made soil 13 -- one of soil 1, 13 or a concealing soil, with no link, when asked -- and,
 * given a direction, the traversable cell a step that way marked too (OVL2 0x556EE0, new in SOCOM II:
 * 0x556370 calls it twice, it calls itself for the step; the names ours) */
extern "C" void func_00556EE0(CAiMap *map, const CAiMapLoc &loc, const CPnt3D *dir, bool mark)
{
    if (!map)
        map = AiMaps.GetMapByIndex(loc.m_mapid);

    CAiCell *cell = map ? map->GetCell(loc, 0) : 0;

    if (cell) {
        if (mark && !cell->HasLink()) {
            unsigned int soil = cell->m_soil;

            if (soil == 1 || soil == 13 || ConcealingSoil(soil))
                cell->m_soil = 13;
        }
        if (dir) {
            CAiMapLoc n;

            if (TraversableNeighbor(map, loc, CAiMap::GetClosestDir(dir), n))
                func_00556EE0(map, n, 0, true);
        }
    }
}

/* CZAnim::Stop, which SOCOM II gave a flag (zanim.h's declaration) */
extern "C" void func_002707C0(CZAnim *anim, bool now);

extern "C" {
extern float data_004365C0;   /* (OVL1 data: the delay a new stance waits, read as a variable) */
/* (OVL2 data: how long after its time at 0x40C the controller stays busy) */
extern float data_00650000;
}

extern "C" {
/* the amount for none given, the range's factor, and the range that stands (OVL2 data 0x64FDD8,
 * 0x64FDF0, 0x64FE48, read as variables) */
extern float data_0064FDD8;
extern float data_0064FDF0;
extern float data_0064FE48;
}

/* the range narrowed by a factor unless it stands (an inline: the factor, its argument, is loaded
 * ahead of the test; the test reads the range through a const view, so the product reads it again --
 * plain reads MW merges) */
static inline void ScaleRange(CSealCtrlAi *ai, float k)
{
    if (data_0064FE48 != ((const CSealCtrlAi *)ai)->m_6f4)
        ai->m_6f4 *= k;
}

/* an alert taken in: its amount (a default for none) summed, its point filled, its word kept, the range
 * narrowed unless it stands, then the pair OnDeath makes (OVL2 0x572240, new in SOCOM II; the name
 * unknown). Byte-exact since round 33: the range's second load (`lwc1; mul.s $f0,$f0,$f2`) is the
 * const view's in ScaleRange -- the kit's round-34 rule for two loads of one member with nothing
 * between */
extern "C" void func_00572240(CSealCtrlAi *ai, CEntity *alert, float amount)
{
    if (alert) {
        if (amount == 0.0f)
            amount = data_0064FDD8;
        ai->m_718 += amount;
        alert->GetTargetPosW(&ai->m_724);
        ai->m_730 = alert->m_id;
        ScaleRange(ai, data_0064FDF0);

        CAiGroup *g = ai->GetGroup();

        if (g)
            g->Abort();
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
 * once no position is left (OVL2 0x56D920; SOCOM 1's, its sqrtf now sqrt.s). Byte-exact since round
 * 33: the target reads x again for its scaling after the sqrt (`lwc1 $f0,0x174(a0)` after `mtc1 1.0`)
 * -- x scaled through a const view of the controller, the kit's round-34 rule, where a plain read lets
 * MW keep the length's x across the sqrt; with it the sum is SOCOM 1's order, x*x first (round 32's 8
 * instructions, its z-first sum a fit to the merged load). The three-float constructor gives the
 * stores (z loaded first) and m_matrix's elements read straight give the dot's operand order (through
 * operator() they swap) */
float CSealCtrl::SetGoalDir(GOAL_TYPE type, const CPnt3D *dir, bool flat)
{
    float dot = 1.0f;

    if (type < m_goal_type)
        return dot;
    if (dir) {
        m_goal_dir = CPnt3D(dir->x, 0.0f, dir->z);
        if (!flat || dir->y != 0.0f) {
            float len = sqrtf(m_goal_dir.x * m_goal_dir.x + m_goal_dir.z * m_goal_dir.z);

            if (len != 0.0f) {
                float inv = 1.0f / len;

                m_goal_dir.x = ((const CSealCtrl *)this)->m_goal_dir.x * inv;
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

/* the state the other console sends for its SEAL, as a word's bits (seal.cpp's SEAL_NET_STATE read
 * whole: a code -- 17 a kill to credit -- the entity it names, the node's fade in fifteenths, the
 * restraint, a friend's compass marker; the names ours) */
struct REMOTE_NET_STATE {
    unsigned int m_code : 8;   // 0x0
    unsigned int m_id : 16;   // 0x1
    unsigned int m_fade : 4;   // 0x3
    unsigned int m_restrained : 1;
    unsigned int m_friend : 1;
    unsigned int m_bit6 : 1;
    unsigned int m_bit7 : 1;
};

extern "C" {
/* a rescue credited to a SEAL's stats -- its counter 14 and three points to its score (OVL2 0x545480,
 * seal.cpp's span: the SEAL in a1, which both callers hand it) */
void func_00545480(CSealStats *stats, CZSealBody *seal);
}

/* the AI's post-tick (OVL2 0x56F970, slot 0x3C: SOCOM 1's AI had no override): the sightings ticked
 * while bit 19 is set; the stance kept up to the SEAL's state, an asked stance taken once its delay runs
 * out; the desired-stance timer's expiry asks the SEAL for stance 2; a follower whose team has bit 17,
 * once it stands in a region of type 1, rescued by its master -- the rescued state pushed with the
 * region and the master's id, and the rescue credited to the master (0x545480) -- the master forgotten
 * when it is not a SEAL; the first time the alertness is DIRECTION with 8 s in its band (SPECIFIC with
 * 5 s) the player's counter 16 goes up by 2 (by 3). Byte-exact (round 51): the bit's test a
 * branch-built bool (`m_aibit19 ? true : false`: sltu, then beqz -- the bit tested in place, a (bool)
 * cast or a bool local each 4 off) */
bool CSealCtrlAi::PostTick(float dT)
{
    CZSealBody *seal = (CZSealBody *)m_entity;

    if (m_aibit19 ? true : false)
        SensorTick(dT);
    m_stance = CZSealBody::GetStanceFromState((SEAL_STATE)seal->m_state, m_stance);
    if (m_stance != m_stance_asked && m_stance_asked != -1) {
        m_280 -= dT;
        if (m_280 < 0.0f)
            DoSetStance(m_stance_asked);
    }
    if (TickGoalTimer(m_stance_timer, dT))
        seal->m_desiredStance = (SEAL_STANCE)2;
    if (m_follow_master_id != CEntity::invalidId && (seal->m_team & 0x20000)) {
        CAiMap::region *region = AiMaps.GetRegionTypeContainer((CAiMap::region::TYPE)1, seal->m_mapLoc);

        if (region) {
            CZSealBody *master = AsSeal(CEntity::m_list.GetEntityById(m_follow_master_id));

            if (master) {
                if (SetAiState(AI_STATE_RESCUED, 0)) {
                    CAiState *st = GetAiState();
                    CAiSRescued *r = st && st->m_state == AI_STATE_RESCUED ? (CAiSRescued *)st : 0;

                    r->m_58 = region;
                    r->m_5c = m_follow_master_id;
                    r->SetActive(true);
                    func_00545480(&master->m_stats, master);
                }
            } else
                m_follow_master_id = CEntity::invalidId;
        }
    }
    if (!m_aibit18) {
        if (GetAiAware() == AIA_DIRECTION && !(m_5c8 < 8.0f)) {
            m_aibit18 = true;
            ftsGetPlayer()->m_stats.m_count[16] += 2;
        } else if (GetAiAware() == AIA_SPECIFIC && !(m_5c8 < 5.0f)) {
            m_aibit18 = true;
            ftsGetPlayer()->m_stats.m_count[16] += 3;
        }
    }
    return true;
}

extern "C" float floorf(float);

/* a node's fade in fifteenths: 15 times the faded part when it is shown, else none (seal.cpp's
 * helper: the target tests the shown flag again inside it) */
static inline unsigned int FadeLevel(bool active, float opacity)
{
    return active ? (unsigned int)floorf(15.0f * (1.0f - opacity)) : 0;
}

/* this console's state for the others, as a word's bits (OVL2 0x56F6C0, slot 0x6C: SOCOM 1's
 * CSealCtrlAi::GetAiNetState -- the state's kind, through slot 0xA4 now where SOCOM 1 inlined its
 * GetAiStateEnum; the id the kind names: the master followed, the rescuer, else the master's id
 * where SOCOM 1 sent none; the fade, the restraint and the friend's marker). The switch tests the
 * call's value (`andi v1,v0,0xff`): `switch (st.m_code)` after the store re-reads the byte (lbu),
 * and a local of any type (AI_STATE, unsigned char, int) or the assignment as the switch's value
 * matches, under slot 0xA4 typed bool or AI_STATE alike -- this code cannot tell the slot's type */
unsigned int CSealCtrlAi::GetAiNetState() const
{
    zdb::CNode *node;
    CZSealBody *seal = (CZSealBody *)m_entity;
    REMOTE_NET_STATE st;
    unsigned int zero = 0;
    unsigned int state;

    memcpy(&st, &zero, sizeof(st));
    node = seal->GetNode();
    switch (st.m_code = GetJumpFlag()) {
    case 9: {
        const CAiState *s = GetAiState();

        st.m_id = m_follow_master ? m_follow_master->m_id : 0;
        break;
    }
    case 17: {
        const CAiState *s = GetAiState();
        const CAiSRescued *r = s && s->m_state == AI_STATE_RESCUED ? (const CAiSRescued *)s : 0;

        st.m_id = r->m_5c;
        break;
    }
    default:
        st.m_id = m_follow_master_id;
        break;
    }
    bool active = node->m_active;
    float opacity = node->m_Opacity;

    if (active && opacity > 0.0f)
        st.m_fade = FadeLevel(active, opacity);
    else
        st.m_fade = 15;
    st.m_restrained = seal->m_restrained ? true : false;
    st.m_friend = seal->m_isAlive && (seal->m_team & 0x20000);
    memcpy(&state, &st, sizeof(st));
    return state;
}

/* the restraint taken from the state when it changes (an inline: both bits made bools) */
static inline void SetRestrained(CZSealBody *seal, bool on)
{
    if (on != (bool)seal->m_restrained)
        seal->m_restrained = on;
}

/* the other console's state taken: kept, the node faded and shown by it, the friend's marker made or
 * dropped, a new kill credited to the SEAL it names, the restraint (OVL2 0x56F4C0: SOCOM 1's
 * CRemoteCtrl::SetAiNetState, its hostage anchor and its credit to the player SOCOM II's marker and
 * stats). The registers want the node declared ahead of the SEAL and the state copied into a local
 * between the two memcpys (the copy numbered last: s0; the parameter's own address taken instead
 * homes it on the stack) */
void CRemoteCtrl::SetAiNetState(unsigned int state)
{
    zdb::CNode *node;
    CZSealBody *seal = (CZSealBody *)m_entity;

    node = seal->GetNode();
    REMOTE_NET_STATE old;
    REMOTE_NET_STATE cur;
    unsigned int prev = GetAiNetState();
    unsigned int now;

    memcpy(&old, &prev, sizeof(old));
    now = state;
    memcpy(&cur, &now, sizeof(cur));
    m_ai_net_state = state;
    node->m_Opacity = 1.0f - cur.m_fade / 15.0f;
    node->SetActive(cur.m_fade != 15);
    if (seal->m_isAlive && (cur.m_friend ? true : false))
        seal->CreateCompassAnchor(4, 0);
    else
        func_00543480(seal);
    if (old.m_code != cur.m_code) {
        switch (cur.m_code) {
        case 17: {
            CZSealBody *killer = AsSeal(CEntity::m_list.GetEntityById(cur.m_id));

            if (killer)
                func_00545480(&killer->m_stats, killer);
        }
        }
    }
    SetRestrained(seal, cur.m_restrained);
}

extern "C" {
/* (OVL2 0x5EC9F0, seal_unit.cpp's: zseal.h's CSealUnit does not declare it) */
void InitalizeSealUnit__9CSealUnitFP10CZSealBodyP9CSealUnit(CZSealBody *seal, CSealUnit *unit);
}

/* the remote SEAL taken over by the AI: its controller's network state read, the old controller
 * deleted, a new AI's set up in the squad -- the followed one's squad kept for a follower whose squad
 * still has SEALs -- and then, by the state's code, the follow order given to the SEAL it named (9),
 * the rescued state pushed with the rescuer's id (17), or the id kept as the master's (OVL2
 * 0x56F210: SOCOM 1's CZSealBody::AcquireRemoteAi, the squad, the deletion and the default new).
 * The registers want the code, the squad and the unit declared at the top in that order (the six
 * orders give 2 to 12 off; declared where defined, 37), the squad set to the unit and then cleared
 * (the ternary duplicates the block), and the unit stored into the new AI before the squad's set-up
 * (the store in that call's delay slot) */
void CZSealBody::AcquireRemoteAi(CZSealBody *seal)
{
    unsigned int code;
    CSealUnit *squad;
    CSealUnit *unit;
    CSealCtrl *ctrl = (CSealCtrl *)seal->m_control;
    CSealCtrl *remote = (CSealCtrl *)ctrl->GetRemoteControl();

    if (!remote)
        return;

    REMOTE_NET_STATE st;
    unsigned int state = remote->GetAiNetState();

    memcpy(&st, &state, sizeof(st));

    unit = ctrl->m_unit;
    code = st.m_code;
    squad = unit;

    if (code != 9 || !unit->GetSealCount(0))
        squad = 0;

    seal->SetController(0);
    if (remote)
        delete remote;

    CSealCtrlAi *ai = new CSealCtrlAi;

    seal->SetController(ai);
    ai->m_unit = unit;
    InitalizeSealUnit__9CSealUnitFP10CZSealBodyP9CSealUnit(seal, squad);
    ai->Initalize();
    ai->InitAiBrain();
    ai->Dump();
    switch (code) {
    case 9: {
        CZSealBody *leader = AsSeal(CEntity::m_list.GetEntityById(st.m_id));

        if (leader)
            seal->IssueCommand(leader, (FT_COMMAND)4, 0, false);
        break;
    }
    case 17:
        if (ai->PushAiState(AI_STATE_RESCUED, 0)) {
            CAiState *s = ai->GetAiState();
            CAiSRescued *r = s && s->m_state == AI_STATE_RESCUED ? (CAiSRescued *)s : 0;

            r->m_5c = st.m_id;
            r->SetActive(true);
        }
        break;
    default:
        ai->m_follow_master_id = st.m_id;
        break;
    }
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

/* the anim played out (OVL2 0x55A920, slot 0x10: SOCOM 1's CAiSAnimate::Tick without its dT, the
 * blend pushed through 0x588BC0 in the state's own stance, and SOCOM II's timed and span calls):
 * steps 0 and 1 push the anim unless it is on top already, step 2 runs the timed calls the anim's
 * fraction has reached -- each erased once it answers true, the step done when one answers false --
 * and every span call the fraction is inside, then ends once the anim is off the top or, under flag
 * 1, the state has held its time; done, the state stops running. The shapes the target fixes: the
 * timed list's front read through the CONST begin() (front() and erase(begin()) both through the
 * sentinel's reference make MW keep &end_.next_ in a saved register and renumber s0-s5); the top
 * blend's anim type through an inline (loaded ahead of m_anim, compared second) where steps 0/1 and
 * the hold test read it, plainly in step 2; the call's bool tested `== false` (its `andi 0xff`); the
 * step defined before the SEAL; the span walk's end a declared local (s5); and the hold test as
 * `if (step != STEP_3) {...} else step = STEP_3;` -- the arm that sets the 3 again (`move s2,v0`,
 * the compare's constant) laid last */
static inline short TypeOf(const CZBodyAnimBlend *b)
{
    return b->m_animType;
}

int CAiSAnimate::Tick(CSealCtrlAi *ai)
{
    STEP step = (STEP)m_phase;
    CZSealBody *seal = (CZSealBody *)ai->m_entity;

    if (m_anim == -1)
        step = STEP_2;

    CZBodyAnimBlend *top = (CZBodyAnimBlend *)func_00555170((char *)seal + 0x1c0);

    switch (step) {
    case STEP_0:
    case STEP_1:
        if (!top || m_anim != TypeOf(top)) {
            func_00588BC0(seal, m_anim, m_5a, 0, 0);
            step = STEP_2;
        } else
            step = STEP_3;
        break;
    case STEP_2:
        if (!top || m_anim != top->m_animType) {
            step = STEP_3;
            break;
        }
        {
            float t = top->m_tfrac;

            while (m_timed.size() != 0) {
                const ANIM_TIMED_CALL &c = *((const CAnimTimedCallList &)m_timed).begin();

                if (t < c.m_time)
                    break;
                if (c.m_func(this, ai, c.m_data) == false) {
                    step = STEP_3;
                    break;
                }
                m_timed.erase(m_timed.begin());
            }
            CAnimSpanCallList::iterator e = m_spans.end();

            for (CAnimSpanCallList::iterator i = m_spans.begin(); i != e; ++i) {
                ANIM_SPAN_CALL &c = *i;

                if (!(t < c.m_start) && t < c.m_end)
                    c.m_func(this, ai, c.m_data);
            }
        }
        if (step != STEP_3) {
            if ((m_flags & 1) && (m_anim != TypeOf(top) || !(ai->m_stateStack.m_timer < m_60)))
                step = STEP_3;
        } else
            step = STEP_3;
        break;
    }
    if (step == STEP_3)
        SetActive(false);
    UpdatePhase(step);
    return Result();
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

extern "C" {
/* the grenade's landing point solved from its flight, whether found (OVL2 0x55CDC0, not written
 * yet) and the dodge's look and sound (0x55C870, not written yet); a path is planned to a cell by the
 * AI core's CAiPath::InsertPathLoc(loc, map) 0x526C10 */
bool func_0055CDC0(CZProjectile *proj, CPnt3D *pos, float *fuse);
void func_0055C870(CAiSGrenade *g, CSealCtrlAi *ai);
/* (below: a map's cell inside; the AI states' range: a plan pushed to a cell) */
bool func_005561C0(const CAiMap *map, const CAiMapLoc &loc);
bool func_005F0830(CSealCtrlAi *ai, void *loc, unsigned int mode, int kind, int step);
}

/* an anim type by its name (anim.cpp's, OVL2 0x5E5000) */
AnimType FindAnimType(const char *name);

/* the AI's path dropped: its plan aborted, emptied, the default planner kind set and the search
 * limit cleared -- whether it could be emptied (an inline: the grenade state's Tick, three times;
 * the name ours) */
static inline bool ResetPath(CSealCtrlAi *ai)
{
    ai::AbortPathPlan((CAiPath *)&ai->m_path);
    if (((CAiPath *)&ai->m_path)->Clear()) {
        if (((CAiPath *)&ai->m_path)->SetPathType((CAiPath::PATH_TYPE)0))
            ai->m_path.m_type_bias = 1.0f;
        ai->m_path.m_search_limit = 0;
        return true;
    }
    return false;
}

/* the path neither holding points nor planning (an inline bool: the target materialises it) */
static inline bool PathIdle(CSealCtrlAi *ai)
{
    if (((CAiPath *)&ai->m_path)->HasPoints() || ((CAiPath *)&ai->m_path)->IsPlanning())
        return false;
    return true;
}

extern "C" {
/* the AI sent to a cell (OVL2 0x5623B0, not written yet; the name unknown) */
void func_005623B0(CSealCtrlAi *ai, const CAiMapLoc *loc);
}

/* the rescued SEAL's tick (OVL2 0x559070, slot 0x10; SOCOM 1's slot alike): step 0 joins team 17's
 * squad (a SEAL of team bit 17 not in it yet); 1 waits for flag 1 (an extraction valve given), 3 for
 * the valve to reach 2; 4 sets 3 seconds, drops the path and sends the AI to its destination's
 * cell; 5 counts the time down while the path is free, then opens the valve (3); 6 suspends the AI.
 * Step 2 fades the SEAL's node out and, offline, puts the AI to sleep once it is hidden. Byte-exact
 * (round 53): FadeToward's tests read one local and its arms a second copied from it, and the rising
 * arm's lower bound is 0.0f (the pooled constant 0x64FD88 is zero -- the scorer's CONTENT-MISMATCH on
 * the -1.0f had been read as noise) */
int CAiSRescued::Tick(CSealCtrlAi *ai)
{
    unsigned int step = m_phase;
    CZSealBody *seal = (CZSealBody *)ai->m_entity;

    switch (step) {
    case 0:
        if (seal->m_team & 0x20000) {
            CSealUnit *unit = CSealUnit::GetUnitByTeam(17);

            if (unit != ai->m_unit)
                unit->SealJoinUnit(seal);
        }
        step = 1;
        break;
    case 1:
        if (m_flags & 1)
            step = 3;
        break;
    case 2: {
        zdb::CNode *node = seal->GetNode();

        FadeToward(node, 0.0f, m_dT);
        if (!theNetwork.m_9 && !node->m_active) {
            SetActive(false);
            ai->SetSleeping(true);
        }
        break;
    }
    case 3:
        if (m_60->GetValue() >= 2)
            step = 4;
        break;
    case 4:
        m_time = 3.0f;
        step = 5;
        ResetPath(ai);
        if (m_58) {
            CAiMapLoc loc;

            if (AiMaps.GetLocation(m_58->m_name, loc))
                func_005623B0(ai, &loc);
        }
        break;
    case 5:
        if (func_005629B0(ai)) {
            m_time -= m_dT;
            if (m_time <= 0.0f) {
                SetValveValue(m_60, 3);
                step = 6;
            }
        }
        break;
    case 6:
        if (ai->PushAiState((AI_STATE)18, 0)) {
            CAiState *s = ai->GetAiState();

            s = s && s->m_state == 18 ? s : 0;
            s->SetFlags(0xe, 0);
            s->SetActive(true);
        }
        break;
    }
    UpdatePhase(step);
    return Result();
}

/* a point turned about (the three loaded, negated and stored: a value built by the three-float
 * constructor) */
static inline CPnt3D Neg(const CPnt3D &p)
{
    return CPnt3D(-p.x, -p.y, -p.z);
}

/* the plan the dodge pushed (its own stack's top, a plan state) told to hurry, and a Navy SEAL's
 * to preempt (an inline: three times in Tick; the name ours) */
static inline void HurryPlan(CAiSGrenade *g, CSealCtrlAi *ai)
{
    CAiState *top = g->m_stack.GetState();
    CAiSPlan *plan = top ? (top && top->m_state == AI_STATE_PLAN ? (CAiSPlan *)top : 0) : 0;

    plan->m_12c = 1.0f;
    if (ai->IsNavySeal())
        *(float *)&plan->m_path[0x18] = ai::s_preemptPriority;
}

/* whether a cell's map holds it (the map's own test, none for no map), and whether it can be
 * walked (the names ours) */
static inline bool CellInside(const CAiMapLoc &loc)
{
    CAiMap *map = AiMaps.GetMap(loc);

    return map ? func_005561C0(map, loc) : false;
}

static inline bool CellTraversable(const CAiMapLoc &loc)
{
    CAiMap *map = AiMaps.GetMap(loc);

    return map ? map->IsTraversable(loc) : false;
}

extern "C" {
/* (OVL2 data: the share of a grenade's blast radius the AI keeps its own clear of, read as a variable) */
extern float data_00650200;
}

/* two team masks that do not meet (an inline returning the negation, taking the masks by value: used as
 * the whole test it is materialised -- sltu, xori -- and each mask loaded in the loop; over the two
 * entities MW keeps the SEAL's mask address hoisted and the saved registers move) */
static inline bool TeamsApart(unsigned int a, unsigned int b)
{
    return !((a & b) != 0);
}

/* a value squared, the expression written twice (a macro: the target calls the radius twice) */
#define AI_SQUARE(x) ((x) * (x))

/* whether a grenade of the item may be thrown at the point: item 123 always; a fragmentation kind
 * (121, 124, 126, 142, 143, 171, 174-176, 179) only from outside the map's cells (0x5561C0), the point
 * beyond a share of its blast's radius and no living team mate the SEAL tracks -- awake when the AI
 * drives it -- within that reach of the point (OVL2 0x565460, new in SOCOM II: the grenade choice
 * 0x565110 asks it for each candidate; the name ours). Byte-exact (the SEALs' round 57, unwritten before)
 * with: the cases listed 123 first (the chain runs in reverse); the map's location a named reference
 * (its address kept in the item's saved register for both of CellInside's calls); `r = true` ahead of
 * the targets' reference; the team test TeamsApart over the two masks by value */
extern "C" bool func_00565460(CSealCtrlAi *ai, EQUIP_ITEM item, const CPnt3D *pos)
{
    bool r = false;
    CEntity *seal = ai->m_entity;
    CZWeapon *w = CZWeapon::m_listWeapons.GetWeapon(item);

    if (w) {
        CZAmmo *ammo = w->GetDefaultAmmo();

        if (ammo) {
            switch (item) {
            case 123:
                r = true;
                break;
            case 121: case 124: case 126: case 142: case 143:
            case 171: case 174: case 175: case 176: case 179:
                const CAiMapLoc &loc = seal->m_mapLocValid;

                if (!CellInside(loc)) {
                    float reach = AI_SQUARE(data_00650200 * ammo->GetExplosionRadius());
                    CPnt3D d;

                    seal->m_posW.Sub(pos, &d);
                    if (d.x * d.x + d.y * d.y + d.z * d.z > reach) {
                        r = true;
                        CTargetList &targets = seal->m_targets;

                        for (CTargetList::iterator i = targets.begin(); i != targets.end(); ++i) {
                            CZSealBody *s = AsSeal(i->m_ent);

                            if (!s || !s->m_isAlive)
                                continue;
                            if (AiDriven(s) && !AiAwakeNow(s->GetSealCtrlAi()))
                                continue;
                            if (TeamsApart(s->m_team, seal->m_team))
                                continue;

                            CPnt3D d2;

                            pos->Sub(&s->m_posW, &d2);
                            if (d2.x * d2.x + d2.y * d2.y + d2.z * d2.z < reach) {
                                r = false;
                                break;
                            }
                        }
                    }
                }
                break;
            }
        }
    }
    return r;
}

/* the grenade state's tick (OVL2 0x55AC20, slot 0x10; SOCOM 1's Tick__11CAiSGrenadeFfP11CSealCtrlAi
 * without its dT, 0x191C there). With its grenade gone the state waits a random while -- shorter
 * for an untrained SEAL -- and ends. Else the grenade's landing point is found once; step 0 alerts
 * the AI to it and, when it may (a launcher's round not yet taken, outside the map, its owner no
 * friend, time left over the run to it), runs to it for a kick (4) or a throw back (5), else dodges
 * (3); step 2 runs, and once there pushes step 4 or 5, or gives up; step 3 pushes a plan away from
 * the blast; 4 and 5 push the animate state with its timed calls (the lists' inserts, 0x575E30 and
 * 0x576000, this function's alone); 6 goes prone or crouches by the blast's radius; 9 ends.
 * The data at 0x650428-0x650470 (8 apart) are the function-local statics below in their order, the
 * guarded 0x66B2B0/0x66B2B8 s_flee and the bss byte 0x66B2C0 s_none_friendly; KickGrenade is
 * SOCOM 1's AnimTypes::KickGrenade by position, "Return throw grenade" the string at 0x65C830.
 * OPEN, the size right (0x1710, 1476 instructions; 127 differ by alignment, round 32): the code is
 * the target's but for registers and frame slots -- the spilled locals' slots run the other way
 * (the target: training 176, launcher 192, pos 208, ammo 224, close 255, cells 256, self 272; MW
 * here close 191, cells 208, self 224, pos 240, ammo 256, training 272) and the CPnt3D temporaries
 * sit 48 lower (the first Sub's at 384 against 432); the owner's null arm goes through v0 (`move
 * v0,zero` in the test's delay slot, `move s8,v0` at the join -- the target `move s8,zero` in an
 * arm of its own; if/else, the ternary, a zeroed first definition and the arms swapped alike); the
 * Chance arm's 5 is not hoisted into the bc1f's delay slot (the if/else, the ternary and an
 * initialised `next` all miss); the dodge's negation interleaves its loads in the target (written
 * field by field it does, but then MW addresses away.y through a register: +8 bytes); and the
 * copies `step = m_84` reload the byte with lb where the compares read it with lbu (a signed char
 * field gives lb compares; unsigned char, a union or a signed cast all reuse the compare's lbu).
 * Fixed on the way (MW's numbering): step declared before the pickup (s2/s1 as the target's), the
 * plan cell declared after the count (608/612), the untrained wait a ternary, self after fuse */
int CAiSGrenade::Tick(CSealCtrlAi *ai)
{
    static float s_pick_time = 0.26f;
    static float s_throw_time = 0.76f;
    static float s_kick_time = 0.4f;
    static float s_fuse_margin = 1.5f;
    static float s_timer_slack = 3.3f;
    static float s_close_sq = 2500.0f;
    static bool s_throw_back = true;
    static float s_kick_range = 120.0f;
    static int s_cell_slack = 2;
    static float s_cover_radius = 10.0f;
    static bool s_none_friendly;

    STEP step = (STEP)m_phase;
    CPickup *p = (CPickup *)m_58;
    CEntity *seal = ai->m_entity;
    CZProjectile *proj = p ? (CZProjectile *)p->m_data : 0;
    CEntity *owner;

    if (proj)
        owner = CEntity::m_list.GetEntityById(proj->m_shooterID);
    else
        owner = 0;
    float *training = &ai->m_aiParams.training;
    bool navy = ai->IsNavySeal();
    float hurry = 2.0f * (1.0f - *training);

    if (navy || (seal->m_team & 0x20000))
        hurry = 0.0f;
    if (!p || m_5c != p->GetPickupNetEntityID() || !proj || !owner) {
        if (!m_60) {
            m_60 = 1;
            float h = navy ? 0.0f : 2.0f * (1.0f - *training);
            if (step == STEP_8)
                m_time = RandRange(1.0f, 1.8f + h);
            else
                m_time = RandRange(2.1f, 2.9f + 1.5f * h);
        } else {
            m_time -= m_dT;
            if (m_time < 0.0f)
                SetActive(false);
        }
        return Result();
    }

    CZAmmo *ammo = proj->m_ammo;
    CPnt3D *pos = &seal->m_posW;
    CAiMapLoc *loc = &seal->m_mapLocValid;
    CPnt3D away = proj->m_framestartpos - *pos;

    away.y = 0.0f;
    float dist = away.Normalize();
    float fuse = proj->m_time;
    bool self = seal == owner;
    bool friendly = (owner->m_team & seal->m_team) != 0;
    float radius = ammo ? ammo->GetExplosionRadius() : 200.0f;
    const AI_PARAMS &params = ai->m_aiParams;
    static float s_flee = 1.3f + s_fuse_margin;

    p->m_bits.m_vec[seal->m_index >> 5] |= 1 << (seal->m_index & 31);
    float spare = fuse - s_fuse_margin;
    float k;

    if (friendly)
        k = 1.2f;
    else
        k = 0.5f * (1.0f - params.training) + 1.2f * params.training;
    int cells = (int)(0.1f * (radius * k));

    if (!m_6c) {
        m_6c = func_0055CDC0(proj, &m_70, &fuse);
        if (m_6c)
            AiMaps.GetLocation(&m_70, m_7c);
    }

    switch (step) {
    case STEP_0:
        if (m_6c) {
            if (m_6c)
                ai->SetAiAware(AIA_DIRECTION, &m_70, -1.0f);
            bool thrown = p->m_locked;
            bool launcher = proj->m_weapon->GetIDType() == (EQUIP_ITEM)121;
            bool close = false;
            STEP next;

            ai->ChangeStanceDelayed(STANCE_PRONE, STANCE_STAND, -1.0f);
            if (self) {
                CPnt3D d = m_70 - ai->m_774;

                if (Dot(d, d) <= s_close_sq)
                    close = false;
            }
            if (m_7c != CAiMapLoc::invalid) {
                CPnt3D d = m_70 - *pos;
                float len = d.Length();
                float t = len / ai->GetSpeedFromThrottle(0.25f, 0);

                if (s_throw_back && launcher && !thrown && !CellInside(m_7c) &&
                    (!friendly || s_none_friendly || close) && !(fuse - t <= spare + hurry) &&
                    !(seal->m_team & 2) && !(seal->m_team & 0x20000) && !(seal->m_team & 0x40000) &&
                    !(seal->m_team & 0x10000000) && !data_0045A0C1 && AiMaps.CanTravelLine(*loc, m_7c, 0)) {
                    func_00570F90(ai, 1.0f);
                    if (*training > 0.8f) {
                        if (len > s_kick_range)
                            next = STEP_4;
                        else
                            next = STEP_5;
                    } else {
                        if (Chance(0.5f))
                            next = STEP_4;
                        else
                            next = STEP_5;
                    }
                    p->SetLocked(true);
                    ai->PlayAiSound((CHARACTER_SOUND)52, 1.0f, CAiEvent::AIE_NONE);
                    func_0055C870(this, ai);
                    if (ResetPath(ai) && ((CAiPath *)&ai->m_path)->InsertPathLoc(m_7c, 0)) {
                        ai->SetAiMode((AI_MODE)3);
                        step = STEP_2;
                    } else {
                        step = STEP_3;
                        next = STEP_6;
                        p->SetLocked(false);
                    }
                } else {
                    if (navy && thrown) {
                        step = STEP_6;
                        SetFlags(1, 0);
                    } else
                        step = STEP_3;
                    next = STEP_6;
                }
            } else {
                step = STEP_3;
                next = STEP_6;
            }
            m_84 = next;
        } else {
            ai->ChangeStanceDelayed(STANCE_PRONE, STANCE_STAND, -1.0f);
            step = STEP_3;
        }
        if (step != STEP_3)
            break;
    case STEP_3: {
        bool found = false;
        CPnt3D dir;
        int got;
        CAiMapLoc cell;
        CAiMap::DIR side;

        ai->ChangeStanceDelayed(STANCE_PRONE, STANCE_STAND, -1.0f);
        step = STEP_6;
        if (m_6c) {
            dir = *pos - m_70;
            dir.y = 0.0f;
            dir.Normalize();
        } else {
            dir = Neg(away);
            CPnt3D vel = proj->m_vel;

            vel.Normalize();
            CPnt3D t = vel * Dot(dir, vel);

            dir.Sub(&t, &dir);
            dir.y = 0.0f;
            dir.Normalize();
        }
        got = 0;
        unsigned int mask = 0x10000;

        if (!func_005664F0(proj->m_weapon->m_ID))
            mask |= 0x20000;
        func_00559580(loc, &dir, mask, &cell, cells + s_cell_slack, &got);
        if (CellTraversable(cell) && got >= cells - s_cell_slack && func_005F0830(ai, &cell, 12, -1, -1)) {
            HurryPlan(this, ai);
            found = true;
            m_80 = cell;
        }
        if (!found) {
            CAiMap *map = AiMaps.GetMapByIndex(loc->m_mapid);

            if (map && map->FindCoverLoc(*loc, &m_70, cell, side, (int)s_cover_radius) &&
                func_005F0830(ai, &cell, 4, -1, -1)) {
                HurryPlan(this, ai);
                found = true;
                m_80 = cell;
            }
        }
        if (!found) {
            if (CellTraversable(cell) && func_005F0830(ai, &cell, 12, -1, -1)) {
                HurryPlan(this, ai);
                found = true;
                m_80 = cell;
            }
        }
        if (!found) {
            step = STEP_6;
            ai->PlayAiSound((CHARACTER_SOUND)53, 1.0f, CAiEvent::AIE_NONE);
        }
        break;
    }
    case STEP_2: {
        bool throwing = m_84 == 4 || m_84 == 5;

        func_00570F90(ai, 1.0f);
        if (navy && throwing && (ai->m_stamp.m_8 & 5)) {
            m_84 = 6;
            p->SetLocked(false);
            step = STEP_3;
            ResetPath(ai);
            if (!data_0045A0C1) {
                CZProjectile *g = GrenadeOf(this);

                if (g)
                    g->m_time += 1.0f;
            }
        } else {
            if (PathIdle(ai)) {
                CEntity *o = proj ? CEntity::m_list.GetEntityById(proj->m_shooterID) : 0;

                if (navy && o && (m_84 == 4 || m_84 == 5)) {
                    CPnt3D d = o->m_posW - seal->m_posW;

                    d = Neg(d);
                    if (ai->SetLookDirW((AI_LOOK)3, &d, true)) {
                        ai->ModifyLookFlags(1, 0);
                        ai->SetLookTimer(3.0f);
                    }
                }
                step = (STEP)m_84;
            } else {
                if (!((CAiPath *)&ai->m_path)->IsPlanning() && throwing && navy && fuse < s_flee) {
                    m_84 = 6;
                    p->SetLocked(false);
                    step = STEP_3;
                    ResetPath(ai);
                    if (!data_0045A0C1) {
                        CZProjectile *g = GrenadeOf(this);

                        if (g)
                            g->m_time += 1.0f;
                    }
                }
                hurry = navy || hurry <= 0.2f ? 0.2f : hurry;
                if (ai->m_stateStack.m_timer > s_timer_slack + hurry || (navy && proj->m_time < 0.3f)) {
                    if (m_84 == 6)
                        step = (STEP)m_84;
                }
            }
        }
        break;
    }
    case STEP_4:
        step = STEP_6;
        if (ai->PushAiState(AI_STATE_ANIMATE, 0)) {
            CAiState *s = ai->GetAiState();
            CAiSAnimate *a = s && s->m_state == AI_STATE_ANIMATE ? (CAiSAnimate *)s : 0;

            a->m_anim = (AnimType)AnimTypes::KickGrenade;
            a->m_5a = 0;
            ANIM_TIMED_CALL kick(s_kick_time, func_0055C740, (unsigned int)this);

            a->m_timed.push_back(kick);
            a->SetActive(true);
        }
        break;
    case STEP_5:
        step = STEP_6;
        if (ai->PushAiState(AI_STATE_ANIMATE, 0)) {
            CAiState *s = ai->GetAiState();
            CAiSAnimate *a = s && s->m_state == AI_STATE_ANIMATE ? (CAiSAnimate *)s : 0;

            a->m_anim = FindAnimType("Return throw grenade");
            a->m_5a = 0;
            ANIM_TIMED_CALL pick(s_pick_time, func_0055C690, (unsigned int)this);

            a->m_timed.push_back(pick);
            ANIM_TIMED_CALL thr(s_throw_time, func_0055C480, (unsigned int)this);

            a->m_timed.push_back(thr);
            ANIM_SPAN_CALL hold(s_pick_time, s_throw_time, func_0055C5E0, (unsigned int)this);

            a->m_spans.push_back(hold);
            a->SetActive(true);
        }
        break;
    case STEP_6:
        if (dist >= radius)
            step = STEP_8;
        else
            step = STEP_7;
        if (navy && step == STEP_7 && !p->m_locked && (m_flags & 1)) {
            SetFlags(0, 1);
            if (proj->m_time > 1.0f)
                step = STEP_3;
        }
        break;
    case STEP_7:
        ai->SetStanceDelayed(STANCE_PRONE, -1.0f);
        break;
    case STEP_8:
        ai->SetStanceDelayed(STANCE_CROUCH, -1.0f);
        break;
    case STEP_9:
        SetActive(false);
        break;
    }
    UpdatePhase(step);
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

/* the squad (an inline accessor: read through it the squad's register comes after the AsSeal inline's result,
 * as OnDeath's target has it -- written `unit = m_unit` the two swap, s0 and s1; the name ours) */
static inline CSealUnit *UnitOf(CSealCtrl *c)
{
    return c->m_unit;
}

extern "C" {
/* the chance of a dying burst for a SEAL at least alert, the anger's chance and the rage it stops at
 * (OVL2 data; the names unknown); the pickup a Navy SEAL's satchel drops (OVL2 0x59FAC0, seal_env.cpp's) */
extern float data_00650018;
extern float data_0064FE10;
extern float data_0064FE18;
class CPickup *func_0059FAC0(CZSealBody *seal);
}

/* dead: whatever it did stopped -- the alive valve shut, the three others closed, its group aborted, the
 * throttles zeroed, its sounds cut -- its node's bit 22 set (SOCOM 1's m_NOTUSED), the cry by how it died, the killer
 * of its own team's anger raised; the squad told, the AI state cleared, the target and the map's bit let
 * go; an alert SEAL's dying burst on a chance, a Navy SEAL's satchel dropped, the events forgotten
 * (OVL2 0x5714B0, slot 0x18; SOCOM 1's OnDeath grown) */
void CSealCtrlAi::OnDeath()
{
    zdb::CNode *node;
    CZSealBody *seal = (CZSealBody *)m_entity;
    CZSealBody *killer;
    CSealUnit *unit;

    node = seal->GetNode();
    killer = AsSeal(CEntity::m_list.GetEntityById(seal->m_lastshooter_id));
    unit = UnitOf(this);

    CValve *valve = m_valve[SEAL_VALVE_ALIVE];

    if (valve)
        SetValveValue(valve, 0);
    func_00574EF0(this, 0);

    CAiGroup *group = GetGroup();

    if (group)
        group->Abort();
    SetThrottle(1, 0.0f);
    SetThrottle(2, 0.0f);
    SetThrottle(0, 0.0f);
    SetThrottle(3, 0.0f);
    m_throttle[1] = 0.0f;
    m_throttle[2] = 0.0f;
    m_throttle[0] = 0.0f;
    m_throttle[3] = 0.0f;
    CSndInstance::Remove(seal->m_id, true, true, true);
    node->m_NOTUSED = true;

    SEAL_DEATH_TYPE type = seal->m_deathType;

    static Rfloat s_burst(1.5f, 1.5f);
    float chance = 0.0f;

    switch (type) {
    case DEATH_KNIFE_KILL:
    case DEATH_CQB:
        PlayAiSound((CHARACTER_SOUND)61, 1.0f, CAiEvent::AIE_NONE);
        break;
    case DEATH_HEADSHOT:
        PlayAiSound((CHARACTER_SOUND)61, 1.0f, (CAiEvent::EVENT)3);
        break;
    case DEATH_GHOST:
        break;
    case DEATH_FALLING:
        if (seal->m_aiEvent)
            seal->m_aiEvent->m_type = 0;
        break;
    default:
        PlayAiSound((CHARACTER_SOUND)60, 1.0f, (CAiEvent::EVENT)4);
        if (AlertBand(((CAlertness *)this)->m_alert) >= ALERT_BAND_2)
            chance = data_00650018;
        break;
    }
    if (killer && (seal->m_team & killer->m_team) && killer->IsPlayer()) {
        float t = data_0064FE10;

        if (m_408 < data_0064FE18)
            t *= m_408;
        if (data_0064FDF8) {
            if (!(t <= 1.0f))
                PlayAiSound((CHARACTER_SOUND)81, 1.0f, CAiEvent::AIE_NONE);
            m_400 += data_0064FE00 * t;
        }
        func_00561B90(this, false);
    }
    if (unit)
        unit->Notify(seal, UNIT_MESSAGE_DEATH, 0);
    ClearAiState(true);
    SetTarget(0);
    SetAiAware(0.0f, 3);
    seal->m_restrained = false;
    if (m_aiParams.respawn)
        seal->SetIncludeInTargetList(false);
    if (!(chance <= 0.0f) && Chance(chance)) {
        float t = s_burst;

        SetSprayTimer(t);
    }
    if (IsNavySeal())
        func_0059FAC0(seal);
    ((CAiEventQ *)m_eventQueue)->clear();
    CleanupDiPool(seal);
    seal->SetUpdateTargetList(false);
    SetDoAiMapCollide(false);
}

/* hurt: the SEAL's other sounds cut off and the pain cry, always (OVL2 0x571AB0; SOCOM 1's with
 * SOCOM II's extra arguments). The entity is read ahead of the test, as both builds do: only a
 * local taken first gives that `lw` its place before the branch. */
void CSealCtrlAi::OnSetHealth(float health, float old)
{
    CEntity *e = m_entity;

    if (old > health) {
        CSndInstance::Remove(e->m_id, true, true, false);
        PlayAiSound((CHARACTER_SOUND)59, 2.0f, CAiEvent::AIE_NONE);
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
/* (OVL2 0x511F80 and 0x52D280, the AI core's: a map's cell at a location, CAiMap::GetCell, and
 * whether the cell is inside, CAiCell::IsInside -- a bool: ai.h's inline, which the target calls out of
 * line here. Kept by their addresses (the round-35 respell): both are ai.h's in-class inlines, so
 * the member spellings would inline them where the target calls) */
void *func_00511F80(void *map, CAiMapLoc *loc, int a);
bool func_0052D280(void *cell);
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
 * 0x556280, new in SOCOM II; a callback). The target narrows the kind into v1 in each arm (`andi
 * v1,v0,0xff` in the then-arm's delay slot, past the else's own): IsInside returns a bool (round 34) --
 * declared int, MW narrows the call's result into v0 and branches to the else's narrowing (2 off). */
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

/* the stamp restarted: the mission's time, its fields cleared (OVL2 0x558590; the sleep setters
 * inline it, the states and the group call it) */
void CAiStamp::Reset()
{
    m_8 = 0;
    m_4 = 0;
    m_time = data_004365C0;
    m_1c = 0;
    m_20 = 0;
}

/* an alert's level record (the names ours: 0x558030, 0x5582C0 and 0x558440 keep one each) -- the
 * AI's stamp (zseal.h's CAiStamp at 0x324, whose Reset restarts it) with what each keeps of its
 * cause: the cell, the entity and its flag (0x558440), the point (0x5582C0) */
struct CAlertLevel {
    float m_time;   // 0x0: when the level was raised, on the game's clock
    float m_amount;   // 0x4
    unsigned char m_level;   // 0x8
    unsigned char _9[0xc - 0x9];
    int m_c;   // 0xc: cleared on any change
    unsigned char _10[0x14 - 0x10];
    AI_MODE m_mode;   // 0x14: the AI's mode at the change, 1 without an AI
    unsigned char _15[0x18 - 0x15];
    CAiMapLoc m_loc;   // 0x18: the cause's cell
    CZSealBody *m_who;   // 0x1c: the SEAL that caused it (CAiStamp's m_1c)
    unsigned char m_avoid;   // 0x20: whether the SEAL is one to avoid (CAiStamp's m_20)
    unsigned char _21[0x24 - 0x21];
    CPnt3D m_pos;   // 0x24: the cause's point

    /* restarted (CAiStamp::Reset, this unit's own function, inlined) */
    void Reset() { ((CAiStamp *)this)->Reset(); }
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

/* a point's distance from another flat on the ground, squared (an inline: the difference built by the
 * three-float constructor, its y a zero -- the target subtracts x first, as only it or locals declared
 * z first give) */
static inline float FlatDistSq(const CPnt3D *a, const CPnt3D *b)
{
    CPnt3D d(a->x - b->x, 0.0f, a->z - b->z);

    return d.x * d.x + d.y * d.y + d.z * d.z;
}

/* (OVL2 data 0x6504A0: how far, squared, a noise may move before it counts as a new one -- 100,
 * read as a variable) */
extern "C" float data_006504A0;

/* a noise heard at a point (level 4): a first one keeps the point and its cell; more of it add up
 * unless it moved ten or more away, which restarts the record on the new point; whether the record
 * now holds an untaken level 4 (OVL2 0x5582C0, new in SOCOM II: 0x5582B0 tail-calls it, 0x5B0420
 * calls it; the name unknown) */
extern "C" bool func_005582C0(CAlertLevel *a, CSealCtrlAi *ai, const CPnt3D *pos, float amount)
{
    switch (func_00557F60(a, ai, 4, amount)) {
    case ALERT_NONE:
        break;
    case ALERT_SET:
        a->m_pos = *pos;
        AiMaps.GetLocation(&a->m_pos, a->m_loc);
        break;
    case ALERT_ADDED:
        if (!(FlatDistSq(&a->m_pos, pos) < data_006504A0)) {
            a->Reset();
            func_00557F60(a, ai, 4, amount);
            a->m_pos = *pos;
            AiMaps.GetLocation(&a->m_pos, a->m_loc);
        }
        break;
    }
    return a->m_c == 0 && a->m_level == 4;
}

extern "C" {
/* (OVL2 data 0x6504A8 and 0x6504B0, ints read as variables: how many changes a level-2 record keeps
 * its cell through, and how many cells away a new one must be to restart it) */
extern int data_006504A8;
extern int data_006504B0;
}

/* how many cells apart two cells are, across and along (an inline: x compared unsigned, as the
 * fields are, y as ints -- the target's sltu / slt; `a > b ?` loads b first) */
static inline int CellSteps(const CAiMapLoc &a, const CAiMapLoc &b)
{
    unsigned int dx = a.x > b.x ? a.x - b.x : b.x - a.x;
    int dy = (int)a.y > (int)b.y ? (int)a.y - (int)b.y : (int)b.y - (int)a.y;

    return dx + dy;
}

/* a disturbance at a cell (level 2): a first one keeps the cell and the flag; more of it with the
 * flag newly set take the amount back off (the change undone), the flag newly cleared restarts the
 * record on the cell, set again only restarts it, and unflagged at another cell -- after enough
 * changes, or far enough -- restarts it there; whether the record now holds an untaken level 2 (OVL2
 * 0x558030, new in SOCOM II beside 0x5582C0 and 0x558440; the name unknown) */
extern "C" bool func_00558030(CAlertLevel *a, CSealCtrlAi *ai, const CAiMapLoc *loc, bool flag, float amount)
{
    int c = a->m_c;

    switch (func_00557F60(a, ai, 2, amount)) {
    case ALERT_NONE:
        break;
    case ALERT_SET:
        a->m_loc = *loc;
        a->m_avoid = flag;
        break;
    case ALERT_ADDED:
        if (flag && !a->m_avoid) {
            a->m_amount -= amount;
            a->m_c = c;
        } else if (!flag && a->m_avoid) {
            a->Reset();
            func_00557F60(a, ai, 2, amount);
            a->m_loc = *loc;
            a->m_avoid = false;
        } else if (flag) {
            a->Reset();
        } else if (a->m_loc != *loc) {
            if (a->m_c >= data_006504A8 || CellSteps(a->m_loc, *loc) >= data_006504B0) {
                a->Reset();
                func_00557F60(a, ai, 2, amount);
                a->m_loc = *loc;
                a->m_avoid = flag;
            }
        }
        break;
    }
    return a->m_c == 0 && a->m_level == 2;
}

/* whether one SEAL should keep away from another within a range (seal_airesponse.cpp's), and the
 * range here (OVL2 data 0x651028: 25, five squared -- read as a variable) */
bool ShouldAvoid(CZSealBody *a, CZSealBody *b, float d2);
extern "C" float data_00651028;

/* another SEAL seen (level 1), unless it is the AI's own or, asked to, one it need not avoid within
 * five: a first sighting keeps its cell, the SEAL and the flag; more of the same SEAL add up, another
 * restarts the record on it; whether the record now holds an untaken level 1 (OVL2 0x558440, new in
 * SOCOM II: 0x54F7E0 calls it four times, 0x567900 once; the name unknown) */
extern "C" bool func_00558440(CAlertLevel *a, CSealCtrlAi *ai, CZSealBody *seal, bool avoid, float amount)
{
    if (seal && seal != ai->m_entity) {
        if (!avoid || ShouldAvoid((CZSealBody *)ai->m_entity, seal, data_00651028)) {
            switch (func_00557F60(a, ai, 1, amount)) {
            case ALERT_NONE:
                break;
            case ALERT_SET:
                a->m_loc = seal->m_mapLocValid;
                a->m_who = seal;
                a->m_avoid = avoid;
                break;
            case ALERT_ADDED:
                if (a->m_who != seal) {
                    a->Reset();
                    a->m_loc = seal->m_mapLocValid;
                    a->m_who = seal;
                    a->m_amount = amount;
                    a->m_avoid = avoid;
                } else if (!avoid) {
                    a->m_avoid = false;
                }
                break;
            }
        }
    }
    return a->m_c == 0 && a->m_level == 1;
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
        ctrl->m_274 = AiMaps.GetRegion(*loc);
        if (ctrl->m_274) {
            ctrl->m_270 = n;
            ok = true;
        }
    }
    return ok;
}

extern "C" {
/* OVL2's out-of-line copies of CPnt3D's three-float constructor and its implicit copy constructor
 * (0x5414F0, 0x539E90) */
CPnt3D *func_005414F0(CPnt3D *p, float x, float y, float z);
CPnt3D *func_00539E90(CPnt3D *p, const CPnt3D *src);
}

/* a box's eight corners, each a temporary point copied in -- the lows and highs mixed, the two
 * plain corners copied straight from the box (OVL2 0x56AE40, new in SOCOM II; 0x56A8E0 calls it, out
 * of line: pinned, the unit's inline_depth(8) would take it) */
#pragma auto_inline off
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
#pragma auto_inline reset

/* the AI's anim of a kind stopped when it plays on top of the body's blends (0x5712F0), saying whether
 * it was (OVL2 0x571100, new in SOCOM II; CAiGroup::act_anim::Stop, 0x60DFC0, asks it with the act's
 * anim -- the kind passed through in a1) */
extern "C" bool func_00571100(CSealCtrlAi *ctrl, short anim)
{
    bool acted = false;   /* declared first: the SEAL takes s0, the flag s1 */
    CZSealBody *seal = (CZSealBody *)ctrl->m_entity;

    if (func_005712F0(ctrl, anim)) {
        seal->PopBlend();
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

/* the template tail's other instance, explicit until its callers are written: _close_enough over ints
 * (OVL2 0x575CB0: 0x567900 calls it twice, as UpdateAiMapLoc does) -- _max<f> (0x575C50), the tail's first,
 * comes with seal_control.cpp's caller, _min<f> (0x575C80) with CSealCtrl::CreepTick's: an explicit
 * instantiation in the unit makes it auto-inline at the call */
template bool _close_enough<int>(const int &, const int &, const int &);
