/* seal_aivehicle.cpp -- the vehicle state, CAiSUseVehicle (new in SOCOM II; the unit's name ours).
 * OVL2 from 0x60EE00 (its destructor) to 0x60F6BF at least: the destructor, Tick and a helper,
 * then SetTimeout, CanPushAiState and OnSignal. So far only the small ones. Defined in reverse of
 * their image order (-inline deferred). */

#include "../gamez/zkit/zkit.h"

extern "C" {
/* a CZSealBody function (OVL2 0x545EE0, the SEAL lane's range), told 1 as the state ends */
void func_00545EE0(CEntity *seal, int x);
/* the SEAL put into a vehicle entity (OVL2 0x546210, the SEAL lane's range: a byte answer); the
 * look (ClearLook 0x600D40, SetLookDirR 0x600DD0) and RestoreAiMode (0x56D090) are zseal.h's members */
bool func_00546210(CEntity *seal, CEntity *vehicle);
}

/* the SEAL's word at 0xEB8, which the vehicle state tests before letting go (zseal.h does not
 * name it -- the name ours) */
struct CSealVehicleView {
    unsigned char _0[0xeb8];
    void *m_eb8;   // 0xeb8

    /* an inline bool accessor: the caller reloads the SEAL for its next statement */
    bool InVehicle() const { return m_eb8 != 0; }
};

/* the SEAL in a vehicle, read off the controller through a const view: the caller's own read of
 * the SEAL, taken into a local ahead of the test, is then not merged with this one (the kit lane's
 * round 34) */
inline bool InVehicle(CSealCtrlAi *ai)
{
    return ((CSealVehicleView *)((const CSealCtrlAi *)ai)->m_entity)->InVehicle();
}

/* the state's events: reset, a vehicle state left (1, 2) or ended (3) (OVL2 0x60F5E0; byte-exact
 * in the kit lane's round 34, open since round 32: the target loads ai->m_entity twice, the second
 * load in the test's delay slot -- the SEAL a local taken in the case ahead of the test, which reads
 * the SEAL through a const view of the AI; plain reads are merged, a local at the top keeps its own
 * register across the switch) */
/* the controller's look as the vehicle helpers set it (0x7A4: a direction, 0x7B0: its kind) and
 * the SEAL's flag byte at 0x105C (zseal.h keeps those flags in an unsigned int, but the target
 * stores the byte) -- the names ours */
struct CAiCtrlLook {
    unsigned char _0[0x7a4];
    CPnt3D m_lookDir;   // 0x7a4
    unsigned char m_lookKind;   // 0x7b0
};

struct CSealFlagsView {
    unsigned char _0[0x105c];
    unsigned char m_bit0 : 1;   // 0x105c
    unsigned char m_bit1 : 1;
    unsigned char m_aim_point_valid : 1;   // bit 2 (SOCOM 1's)
};

/* the SEAL's aim through a direction, the aim point given up and the look set to it, kind 2 (OVL2
 * 0x60F9F0; unit_nz when none is given; the SEAL's SetAimDirR answers, the name ours) */
extern "C" bool func_0060F9F0(CSealCtrlAi *ai, const CPnt3D *dir)
{
    CEntity *seal = ai->m_entity;

    if (!dir)
        dir = &CPnt3D::unit_nz;
    bool r = ((CZSealBody *)seal)->SetAimDirR(dir, false);

    ((CSealFlagsView *)seal)->m_aim_point_valid = false;
    ((CAiCtrlLook *)ai)->m_lookKind = 2;
    ((CAiCtrlLook *)ai)->m_lookDir = *dir;
    return r;
}

/* a vehicle used: when the entity exists, the vehicle state pushed and its vehicle's id kept
 * (OVL2 0x60F6C0; the name ours) */
extern "C" bool func_0060F6C0(CSealCtrlAi *ai, unsigned int id)
{
    bool ok = false;

    if (CEntity::m_list.GetEntityById(id) && ai->PushAiState(AI_STATE_USEVEHICLE, 0)) {
        CAiState *s = ai->GetAiState();
        CAiSUseVehicle *v = s && s->m_state == AI_STATE_USEVEHICLE ? (CAiSUseVehicle *)s : 0;

        v->m_5c = id;
        ok = true;
    }
    return ok;
}

void CAiSUseVehicle::OnSignal(SIGNAL sig, CSealCtrlAi *ai)
{
    switch (sig) {
    case SIGNAL_4:
        m_5c = CEntity::invalidId;
        SetTimeout(CAiTimer::infinite);
        break;
    case SIGNAL_0:
        break;
    case SIGNAL_1:
    case SIGNAL_2:
        {
            CEntity *seal = ai->m_entity;

            if (InVehicle(ai)) {
                func_00545EE0(seal, 1);
                ai->ClearLook(AIL_10);
                ai->SetDoAiMapCollide(true);
                ai->RestoreAiMode();
            }
        }
        break;
    case SIGNAL_3:
        if (m_phase) {
            m_phaseTime = 0;
            m_phase = 0;
        }
        break;
    }
}

/* (OVL2 0x60F5D0) */
bool CAiSUseVehicle::CanPushAiState(AI_STATE state, CSealCtrlAi *ai)
{
    return true;
}

/* the timeout set and its clock restarted (OVL2 0x60F5C0; the name ours) */
void CAiSUseVehicle::SetTimeout(float t)
{
    m_60 = t;
    m_64 = 0;
}

/* the turret an entity is, its position block's address (0x190 into the block at 0x27C) -- the
 * names ours */
struct CTurretView {
    unsigned char _0[0x27c];
    char *m_27c;   // 0x27c
};

/* the timer's period drawn: the timeout, plus a random part of its range when there is one
 * (Rfloat's conversion on the members themselves; through a cast the target's loads would come
 * off a held address) */
inline float CAiSUseVehicle::Draw() const
{
    float f;

    if (m_64 == 0.0f)
        f = m_60;
    else {
        float r = 4.656613e-10f * rand();

        f = m_60 + m_64 * r;
    }
    return f;
}

/* the AI's target (CAiTarget, ai_target.cpp's: the controller's at 0x67C) -- held (OVL2 0x60A850)
 * and the time since it was last seen (OVL2 0x60A590) */
extern "C" bool func_0060A850(const void *target);
class CAiTarget {
public:
    float GetTimeSinceLastSighting() const;
};

/* a held target that may interrupt (the target's flags' bit 9 at 0x6A8: a materialised `&&`) and
 * no target worth staying for: none held, or one not seen for three seconds (inline bools; the
 * names ours) */
inline bool TargetEngaged(CSealCtrlAi *ai)
{
    return func_0060A850(ai->m_67c) && (ai->m_6a8 & 0x200);
}
inline bool TargetLost(CSealCtrlAi *ai)
{
    bool r = true;

    if (func_0060A850(ai->m_67c)) {
        if (!TargetEngaged(ai))
            r = ((const CAiTarget *)ai->m_67c)->GetTimeSinceLastSighting() > 3.0f;
        else
            r = false;
    }
    return r;
}

/* a turret's flag word (0x3A4: bit 1 keeps an AI off it -- the name ours) */
struct CTurretFlagsView {
    unsigned char _0[0x3a4];
    unsigned int m_3a4;   // 0x3a4
};

/* an entity driven by something (an inline bool: the target materialises the test) */
inline bool Driven(const CEntity *e)
{
    return e->m_control ? true : false;
}

/* an entity's target list, through an inline (an accessor of the entity's in the original, likely:
 * CEntity has none here, zentity.h being the engine core's -- the name ours) */
inline CTargetList *TargetsOf(CEntity *e)
{
    return &e->m_targets;
}

/* a free turret among the SEAL's targets within 500 units taken: in the AI's mode 2, not already
 * in the vehicle state and with no target to stay for, the nearest turret with no driver and its
 * flag clear is used (func_0060F6C0's body inlined) (OVL2 0x60F760: CSealCtrlAi::PreTick calls it
 * each frame; the name ours).
 * THE WALK'S END (the MSL headers round 30, build/p30/av_subs.py x7, untracked): the target holds
 * &m_targets in a0 (set in the entry test's delay slot) and re-reads the size and the data through
 * it every pass (`lw t0,4(a0); lw a2,8(a0)`, the end rebuilt), the entry test reading them off the
 * SEAL -- a loop with no store, so no header end() does it (include/vector's end() comment): the walk
 * runs over a REFERENCE bound through a POINTER LOCAL taken from an inline's result. Without the
 * inline (`lp = &seal->m_targets`), without the pointer local (`l = *TargetsOf(seal)`), walking the
 * pointer itself, or in an inline helper taking the list (the round-48 FreeTurret: 0x228, or with
 * the reference inside it 0x240 and 20 registers off), MW keeps size * 48 or the whole end; found
 * declared first gives the target's a1 (after the list's locals it takes a0). The rest is the
 * target's (TargetLost's else-arm order, Driven's `? true : false` keeping the sltu, the 0x60F6C0
 * body) */
extern "C" void func_0060F760(CSealCtrlAi *ai)
{
    static float range = 250000.0f;
    CEntity *seal = ai->m_entity;

    if (ai->m_26e != 2 || ai->AiStateIs(AI_STATE_USEVEHICLE) || !TargetLost(ai))
        return;
    CEntity *found = 0;
    CTargetList *lp = TargetsOf(seal);
    CTargetList &l = *lp;

    for (CTargetList::iterator i = l.begin(); i != l.end(); ++i) {
        if (!(i->m_distSq <= range))
            break;
        CEntity *e = i->m_ent;
        CEntity *t = e && e->m_type == CEntity::ENT_TYPE_TURRET ? e : 0;

        if (t && !(((CTurretFlagsView *)t)->m_3a4 & 2) && !Driven(t)) {
            found = t;
            break;
        }
    }
    if (found) {
        unsigned int id = found->m_id;

        if (CEntity::m_list.GetEntityById(id) && ai->PushAiState(AI_STATE_USEVEHICLE, 0)) {
            CAiState *s = ai->GetAiState();
            CAiSUseVehicle *v = s && s->m_state == AI_STATE_USEVEHICLE ? (CAiSUseVehicle *)s : 0;

            v->m_5c = id;
        }
    }
}

extern "C" {
/* the maps' traversable cell at a point (zseal.h's stand-in has none) */
bool GetTraversableLocation__7CAiMapsFPC6CPnt3DR9CAiMapLoc(CAiMaps *maps, const CPnt3D *pos, CAiMapLoc *loc);
/* a plan pushed to a cell, coming back to a step (seal_aistate.cpp's) */
bool func_005F0830(CSealCtrlAi *ai, void *loc, CAiState::STEP mode, int kind, CAiState::STEP step);
/* a turret's stick for a look at a point or along a direction (OVL1 0x3A8BD0, 0x3A8910) */
void func_003A8BD0(CEntity *v, const CPnt3D *look, float *a, float *b);
void func_003A8910(CEntity *v, const CPnt3D *look, float *a, float *b);
}

/* the thresholds of the event's distance (a square) and its lift (OVL2 data) */
extern float data_00651150;
extern float data_00651158;
extern float NoExpire__8CAiEvent;

/* the event the AI holds (zseal.h keeps it as bytes: its place and duration -- ai.h's CAiEvent; the view ours), and whether it
 * has run out (inlined over the controller: the event's address taken only after it; the name ours) */
struct CAiVehEvent {
    CPnt3D m_pos;   // 0x0
    float m_c;   // 0xc
    float m_duration;   // 0x10
};

inline bool EventExpired(CSealCtrlAi *ai)
{
    bool r = false;

    if (((CAiVehEvent *)ai->m_event)->m_duration != NoExpire__8CAiEvent && ((CAiVehEvent *)ai->m_event)->m_duration <= 0.0f)
        r = true;
    return r;
}

/* the vehicle's timer run out, and run down: whether it has run out (inlined; never for the infinite period; the names
 * ours) */
inline bool TimerExpired(CAiSUseVehicle *v)
{
    bool r = false;

    if (v->m_68 != CAiTimer::infinite && v->m_68 <= 0.0f)
        r = true;
    return r;
}

inline bool TimerDone(CAiSUseVehicle *v, float dT)
{
    bool r = false;

    if (v->m_68 != CAiTimer::infinite) {
        v->m_68 -= dT;
        r = TimerExpired(v);
    }
    return r;
}

/* the vehicle's frame: to the vehicle (a plan to the cell beside it, back at step 1), boarded, then held until the timer runs
 * out -- the look on the target while one is engaged, else a random look to the front, the event near it stepping off --
 * the turret steered by the look; off the vehicle, then done (OVL2 0x60EE80; new in SOCOM II).
 * OPEN, 8 instructions, registers only: the SEAL (v35) wants s1 and func_0060F9F0's argument (v42) s2, ours the other way
 * round. igreplay: the SEAL numbered exactly between that argument and the inline's own local (42.5), or the argument
 * before the SEAL (34.5). A named pointer local there (declared before the SEAL) is propagated: with func_0060F9F0 its null
 * test stays, with a test-less inline the address folds into the frame; the SEAL through an accessor takes s0 */
int CAiSUseVehicle::Tick(CSealCtrlAi *ai)
{
    STEP step = (STEP)m_phase;
    CEntity *seal = ai->m_entity;

    switch (step) {
    case STEP_0: {
        step = STEP_4;
        CEntity *e = CEntity::m_list.GetEntityById(m_5c);

        if (e && GetTraversableLocation__7CAiMapsFPC6CPnt3DR9CAiMapLoc(&AiMaps, &e->m_posW, &m_58))
            func_005F0830(ai, &m_58, step, 1, step);
        break;
    }
    case STEP_1:
        if (Board(ai))
            step = STEP_2;
        else
            step = STEP_4;
        break;
    case STEP_2:
        if (TimerDone(this, m_dT))
            step = STEP_4;
        if (!TargetEngaged(ai)) {
            if (!TargetLost(ai)) {
                CEntity *t = *(CEntity **)ai->m_67c;
                const CPnt3D *pos = &t->m_posW;

                ai->SetLookPosW((AI_LOOK)7, pos);
                ((CZSealBody *)ai->m_entity)->SetAimPointW(pos, 0, false);
                if (pos) {
                    ((CAiCtrlLook *)ai)->m_lookKind = 1;
                    ((CAiCtrlLook *)ai)->m_lookDir = *pos;
                } else {
                    ((CAiCtrlLook *)ai)->m_lookKind = 2;
                    ((CAiCtrlLook *)ai)->m_lookDir = CPnt3D::unit_nz;
                }
            } else {
                ai->ClearLook((AI_LOOK)7);
                m_time -= m_dT;
                if (m_time <= 0.0f) {
                    float r = 4.656613e-10f * (float)rand();
                    float x = -0.707f + 1.414f * r;
                    CPnt3D dir;

                    dir.x = x;
                    dir.y = 0.0f;
                    dir.z = -sqrtf(1.0f - x * x);
                    func_0060F9F0(ai, &dir);
                    m_time = 3.0f;
                }
                if (!EventExpired(ai)) {
                    CAiVehEvent *ev = (CAiVehEvent *)ai->m_event;
                    CPnt3D d;

                    seal->m_posW.Sub(&ev->m_pos, &d);
                    if (d.x * d.x + d.y * d.y + d.z * d.z <= data_00651150) {
                        if (ev->m_duration < data_00651158)
                            ev->m_duration = data_00651158;
                        step = STEP_3;
                    }
                }
            }
        }
        if (step == STEP_2) {
            CEntity *v = (CEntity *)((CSealVehicleView *)ai->m_entity)->m_eb8;

            v = v && v->m_type == CEntity::ENT_TYPE_TURRET ? v : 0;
            if (v) {
                float a = 0.0f;
                float b = 0.0f;

                switch (((CAiCtrlLook *)ai)->m_lookKind) {
                case 1:
                    func_003A8BD0(v, &((CAiCtrlLook *)ai)->m_lookDir, &a, &b);
                    break;
                case 2:
                    func_003A8910(v, &((CAiCtrlLook *)ai)->m_lookDir, &a, &b);
                    break;
                }
                ai->SetThrottle(2, a);
                ai->SetThrottle(3, b);
            }
        }
        break;
    case STEP_3:
        if (InVehicle(ai)) {
            if (InVehicle(ai)) {
                func_00545EE0(seal, 1);
                ai->ClearLook(AIL_10);
                ai->SetDoAiMapCollide(true);
                ai->RestoreAiMode();
            }
        } else if (EventExpired(ai))
            step = STEP_4;
        break;
    case STEP_4:
        m_active = false;
        break;
    }
    UpdatePhase(step);
    return Result();
}

/* the vehicle boarded: when it exists and the SEAL takes it (0x546210), the look set at its
 * position (or unit_nz), the AI-map collision off, the AI's mode 2, the goal dropped, the timer
 * drawn (OVL2 0x60F470, the Tick's; the name ours) */
bool CAiSUseVehicle::Board(CSealCtrlAi *ai)
{
    CEntity *seal = ai->m_entity;
    bool ok = false;
    CEntity *e = CEntity::m_list.GetEntityById(m_5c);

    if (e) {
        ok = func_00546210(seal, e);
        if (ok) {
            const CPnt3D *pos = &CPnt3D::unit_nz;
            CEntity *t = e && e->m_type == CEntity::ENT_TYPE_TURRET ? e : 0;

            if (t)
                pos = (const CPnt3D *)(((CTurretView *)t)->m_27c + 400);
            ai->SetLookDirR(AIL_10, pos, false);
            ai->SetDoAiMapCollide(false);
            ai->SetAiMode((AI_MODE)2);
            ai->ClearGoal(GOAL_NONE);
            m_68 = Draw();
        }
    }
    return ok;
}

/* (OVL2 0x60EE00) */
CAiSUseVehicle::~CAiSUseVehicle()
{
}
