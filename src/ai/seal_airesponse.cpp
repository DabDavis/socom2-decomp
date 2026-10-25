/* seal_airesponse.cpp -- the states an AI SEAL falls into in response: hostage, avoid, rush, path
 * recovery, flee, target, event, stun response, hold, surrender, no ammo, reload, shot at (SOCOM
 * 1's unit, __sinit_seal_airesponse.cpp -- its CAiSRestrained and CAiSCorpse are gone, CAiSAvoid
 * moved in from seal_aistate.cpp, CAiSHostage is here). OVL2 0x601880-0x609DEF: the states'
 * destructors and the inline virtuals their tables pull in, the NoAmmo/Flee/Reload Ticks and seven
 * OnSignals (round 3). Defined in reverse of their image order (-inline deferred). */

#include <string.h>

#include "../gamez/zkit/zkit.h"
#include "../gamez/zsnd/zsnd.h"

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

/* a SEAL of the player's side (SEAL_AI_PARAMS's bit 1 at 0x264, SOCOM 1's m_navyseal; an inline
 * bool accessor: its callers keep the sltu) */
inline bool NavySeal(CSealCtrlAi *ai)
{
    return ai->m_navyseal;
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
enum ORDER_KIND { ORDER_KIND_0 = 0, ORDER_KIND_LAST = 255 };
void func_00571010(CSealCtrlAi *ai, void *target, int a, ORDER_KIND kind);
void *func_00609530(int x);
void func_00563100(CSealCtrlAi *ai, signed char stance, float delay);
void func_00600D40(CSealCtrlAi *ai, int x);
/* (OVL2 0x567840, 0x570D60: the SEAL lane's rows -- a stun's controller calls, the second
 * taking a float) */
void func_00567840(CSealCtrlAi *ai, int x);
/* the kit's reload (OVL2 0x5C2A90, seal_kit.cpp's range, unwritten: called on the SEAL's kit at
 * 0x5E0 by the reload state's sub-step) */
void func_005C2A90(void *kit);
/* a SEAL's check of a kind, a byte (OVL2 0x58A720, the SEAL lane's row) */
ORDER_KIND func_0058A720(CEntity *seal, int x);
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

/* (OVL2 0x609810) its members destroyed by hand, as ai_states.h keeps them as bytes: the vector
 * first (std::vector<CAiMapLoc>'s deleter destructor, 0x515200, behind ~vector's and
 * ~__vector_imp's this-tests), then the route (~CAiPath, 0x5281B0) */
CAiSPathRecover::~CAiSPathRecover()
{
    unsigned char *v = m_vec;

    if (v)
        if (v)
            if (v)
                func_00515200(v, 0);
    func_005281B0(m_path, -1);
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
int CAiSNoAmmo::Tick(CSealCtrlAi *ai)
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

/* a target's sub-step: over once the AI's target (CAiTarget at the controller's 0x67C) no longer
 * holds (OVL2 0x604BF0) */
int CAiSTarget::Virtual14(CSealCtrlAi *ai, CAiState *sub)
{
    if (!func_0060A850((char *)ai + 0x67c))
        SetActive(false);
    return Result();
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
            func_00571010(ai, func_00609530(0), 2, (ORDER_KIND)11);
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

/* the controller's byte at 0x32C (zseal.h does not name it -- the name ours) */
struct CAiCtrl32C {
    unsigned char _0[0x32c];
    unsigned char m_32c;   // 0x32c
};

/* the shot-at state's limit (a function-local static in the original: 8-byte-spaced data) */
static float s_shotAtTime = 15.0f;

/* shot at, the sub-step: over after its time; before, once the AI's byte at 0x32C is set, the
 * sub-state ended and step 3 taken (OVL2 0x609340).
 * OPEN, 0x9C against 0xA4: the target zero-extends the byte it tests (`lbu; dsll32 v0,v0,0;
 * dsrl32 v0,v0,0; beqz`) -- an unsigned int widened to 64 bits; an unsigned char, an 8-bit
 * bitfield in a char, int or long container, `(unsigned long)` and `& ~0u` all branch on the lbu
 * result directly */
int CAiSShotAt::Virtual14(CSealCtrlAi *ai, CAiState *sub)
{
    if (!(m_phaseTime < s_shotAtTime))
        SetActive(false);
    else if (((CAiCtrl32C *)ai)->m_32c) {
        sub->SetActive(false);
        UpdatePhase(3);
    }
    return Result();
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
        m_name[0] = 0;
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

/* the event's name, up to sixteen characters, or none (OVL2 0x605A10) */
void CAiSEvent::SetName(const char *name)
{
    if (name)
        strncpy(m_name, name, 16);
    else
        m_name[0] = 0;
}

/* the controller's word at 0x6A8 and float at 0x718 as the event state reads them (zseal.h does
 * not name them -- the names ours) */
struct CAiCtrlEvent {
    unsigned char _0[0x6a8];
    unsigned int m_6a8;   // 0x6a8: bit 9 lets a held target interrupt an event
    unsigned char _6ac[0x718 - 0x6ac];
    float m_718;   // 0x718
};

/* the AI's target held, and the AI's bit 9 at 0x6A8 set (an inline bool: the target materialises
 * the `&&`) */
inline bool TargetHeld(CSealCtrlAi *ai)
{
    return func_0060A850((char *)ai + 0x67c) && (((CAiCtrlEvent *)ai)->m_6a8 & 0x200);
}

/* a state's flags tested through an inline (the target loads the word again for it) */
inline bool HasFlags(const CAiState *s, unsigned int bits)
{
    return s->m_flags & bits;
}

/* the event's sub-states dropped and step 8 taken, the step it left kept in m_8d (inlined twice
 * in its sub-step; the name ours) */
inline void CAiSEventInterrupt(CAiSEvent *s, CSealCtrlAi *ai)
{
    s->m_stack.PopAll(ai);
    if (s->m_phase != 8) {
        s->m_8d = s->m_phase;
        s->UpdatePhase(8);
    }
}

/* the next step: 1 after 0; after 1, 2 with the flags' bit 0 or 3 with bit 1; after 2, 3 with bit
 * 1; else 8 -- kept, with the delay, and taken at once or after the wait (7) (OVL2 0x605950) */
void CAiSEvent::NextStep(STEP *step, float delay)
{
    STEP next = STEP_8;

    switch (*step) {
    case 0:
        next = STEP_1;
        break;
    case 1:
        if (m_flags & 1)
            next = STEP_2;
        else if (m_flags & 2)
            next = STEP_3;
        break;
    case 2:
        if (m_flags & 2)
            next = STEP_3;
        break;
    }
    m_84 = delay;
    m_8c = next;
    *step = delay == 0.0f ? next : STEP_7;
}

/* an event, the sub-step: nothing while a step-0 or unrecorded event waits (the flags' bit 2);
 * interrupted by a held target (bit 4, with the AI's bit 9 at 0x6A8) or by the AI's value at
 * 0x718 passing 1 (bit 5) (OVL2 0x605800) */
int CAiSEvent::Virtual14(CSealCtrlAi *ai, CAiState *sub)
{
    unsigned int flags = m_flags;

    if (flags & 4) {
        bool waiting = m_phase == 0 || m_8d == 0;

        if (waiting)
            goto done;
    }
    if ((flags & 0x10) && TargetHeld(ai))
        CAiSEventInterrupt(this, ai);
    else if (HasFlags(this, 0x20) && ((CAiCtrlEvent *)ai)->m_718 > 1.0f)
        CAiSEventInterrupt(this, ai);
done:
    return Result();
}

/* a callout: the sound played at a volume, and with `order` the order of kind 0 given when the
 * SEAL's 0x58A720 check (with 1) fails (OVL2 0x605770: the AI's 0x5CA470 and the stack's
 * 0x5F8D40 call it; the name unknown) */
extern "C" void func_00605770(CSealCtrlAi *ai, bool order, CHARACTER_SOUND sound, float vol)
{
    CEntity *seal = ai->m_entity;

    ai->PlayAiSound(sound, vol, -1);
    if (order) {
        ORDER_KIND r = func_0058A720(seal, 1);

        if (r == ORDER_KIND_0)
            func_00571010(ai, func_00609530(0), 2, r);
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

/* a hold with its flags' bit 5 or 7 set (an inline bool: its true is daddiu, the result narrowed) */
inline bool HoldBroken(const CAiState *s)
{
    if (s->m_state == AI_STATE_HOLD && (s->m_flags & 0xa0))
        return true;
    return false;
}

/* whether the AI's current state is a hold with its flags' bit 5 or 7 set (OVL2 0x607830: the SEAL
 * lane's 0x558AE0 and 0x563E10 call it; the name ours).
 * OPEN, 0x7C against 0x6C: the target keeps the hold test in a1 -- zero in the state compare's
 * delay slot, `daddiu a1,zero,1` for true, `b; andi v0,a1,0xff` at the join, and a separate
 * `move v0,zero` for no state. The ternary over the inline (here) puts the andi on a block of its
 * own; `if (s) return HoldBroken(s); return false;` is 0x68, the true in v0 and no andi; a bool
 * local `r = false; if (a && b) r = true;` (inline or not, nested ifs too) is if-converted into
 * sltu; `? true : false` and `HoldBroken(s) != false` miss further. */
extern "C" bool func_00607830(CSealCtrlAi *ai)
{
    const CAiState *s = ai->m_stateStack.size() == 0 ? 0 : ai->m_stateStack.GetState();

    return s ? HoldBroken(s) : false;
}

/* the response states' callouts, looked up by name once the sounds are open: forty-three cue
 * names and their sounds (OVL2 data 0x650E20; the table's and the functions' names ours) */
struct CAiCallout {
    const char *m_name;
    CSnd *m_snd;
};

static CAiCallout s_callouts[43] = {
    { "^CS_0222", 0 }, { "^CS_0226", 0 }, { "^CS_0292", 0 }, { "^*CS_0275", 0 },
    { "^*CS_0276", 0 }, { "^*CS_0278", 0 }, { "^CS_0270", 0 }, { "^CS_0283", 0 },
    { "^*CS_0285", 0 }, { "^CS_0286", 0 }, { "^CS_0266", 0 }, { "^CS_0265", 0 },
    { "^CS_0264", 0 }, { "^CS_0263", 0 }, { "^CS_0262", 0 }, { "^CS_0261", 0 },
    { "^CS_0260", 0 }, { "^CS_1291", 0 }, { "^CS_1292", 0 }, { "^CS_1293", 0 },
    { "^CS_1294", 0 }, { "^CS_1295", 0 }, { "^CS_1296", 0 }, { "^CS_1297", 0 },
    { "^CS_1298", 0 }, { "^CS_1299", 0 }, { "^CS_1300", 0 }, { "^CS_1301", 0 },
    { "^CS_1302", 0 }, { "^CS_1303", 0 }, { "^CS_1304", 0 }, { "^CS_1305", 0 },
    { "^CS_0268", 0 }, { "^CS_0269", 0 }, { "^CS_0282", 0 }, { "^CS_0281", 0 },
    { "^CS_1309", 0 }, { "^CS_1307", 0 }, { "^CS_1310", 0 }, { "^CS_1308", 0 },
    { "^CS_0288", 0 }, { "^CS_0295", 0 }, { "^CS_0308", 0 },
};

/* each callout's sound looked up and put in mode 2 (OVL2 0x6075C0: the SEAL lane's 0x574C30
 * calls it) */
extern "C" void func_006075C0()
{
    for (int i = 0; i < 43; i++) {
        s_callouts[i].m_snd = s_callouts[i].m_name ? CSnd::GetSoundByName(s_callouts[i].m_name) : 0;
        if (s_callouts[i].m_snd)
            s_callouts[i].m_snd->m_mode = 2;
    }
}

/* the sounds forgotten (OVL2 0x607560: the SEAL lane's 0x574BD0 calls it; each entry through a
 * pointer local -- `s_callouts[i].m_snd = 0` folds the field's offset into the remainder pass's
 * address) */
extern "C" void func_00607560()
{
    for (int i = 0; i < 43; i++) {
        CAiCallout *c = &s_callouts[i];

        c->m_snd = 0;
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

/* the reload's wait (a function-local static in the original: 8-byte-spaced data) */
static float s_reloadWait = 0.5f;

/* the reload, the sub-step: a shout first when the flags' bit 0 asks (then cleared), the kit
 * reloaded, a stance -- crouched with bit 2, else kept or (bit 1) changed -- then a short wait
 * (OVL2 0x608840) */
int CAiSReload::Virtual14(CSealCtrlAi *ai, CAiState *sub)
{
    unsigned int phase = m_phase;
    CEntity *seal = ai->m_entity;

    switch (phase) {
    case 1:
        if (m_flags & 1) {
            ai->PlayAiSound((CHARACTER_SOUND)68, 1.0f, -1);
            SetFlags(0, 1);
        }
        func_005C2A90((char *)seal + 0x5e0);
        if (m_flags & 4)
            func_00563050(ai, 2, 1, -1.0f);
        else
            func_00563050(ai, 0, (m_flags & 2) ? 2 : 1, -1.0f);
        m_time = s_reloadWait;
        phase = 2;
        break;
    case 2:
        m_time -= m_dT;
        if (m_time <= 0.0f)
            SetActive(false);
        break;
    }
    UpdatePhase(phase);
    return Result();
}

/* a reload: weapon down, the stance kept, a crouch one time in ten and a move one in four, a look
 * at the target when there is one (the flags' bits 0-2), then the sub-states' turn (OVL2
 * 0x6085C0; SOCOM 1's reloaded itself) */
int CAiSReload::Tick(CSealCtrlAi *ai)
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
int CAiSFlee::Tick(CSealCtrlAi *ai)
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
        ai->m_stateStack.SetFlags(1, 0);
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

/* the hostage's captor: its id and team, the flags' bit 0 when asked (OVL2 0x601E50) */
void CAiSHostage::SetCaptor(CEntity *e, bool flag)
{
    if (flag)
        SetFlags(1, 0);
    if (e) {
        m_5c = e->m_id;
        m_58 = e->m_team;
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
