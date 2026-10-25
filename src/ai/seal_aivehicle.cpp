/* seal_aivehicle.cpp -- the vehicle state, CAiSUseVehicle (new in SOCOM II; the unit's name ours).
 * OVL2 from 0x60EE00 (its destructor) to 0x60F6BF at least: the destructor, Tick and a helper,
 * then SetTimeout, CanPushAiState and OnSignal. So far only the small ones. Defined in reverse of
 * their image order (-inline deferred). */

#include "../gamez/zkit/zkit.h"

extern "C" {
/* a CZSealBody function (OVL2 0x545EE0, the SEAL lane's range), told 1 as the state ends */
void func_00545EE0(CEntity *seal, int x);
/* a CSealCtrl function taking a signed byte (OVL2 0x600D40, unclaimed), told 10 as the state
 * ends */
void func_00600D40(CSealCtrlAi *ai, int x);
/* a CSealCtrlAi function (OVL2 0x56D090, the SEAL lane's range) */
void func_0056D090(CSealCtrlAi *ai);
/* the SEAL aimed along a direction (OVL2 0x5DF8B0, unclaimed: a byte answer) */
bool func_005DF8B0(CEntity *seal, const CPnt3D *dir, int x);
/* the SEAL put into a vehicle entity (OVL2 0x546210, the SEAL lane's range: a byte answer) and a
 * look of a kind at a point (OVL2 0x600DD0, this lane's range, unwritten) */
bool func_00546210(CEntity *seal, CEntity *vehicle);
void func_00600DD0(CSealCtrlAi *ai, int kind, const CPnt3D *pos, int x);
}

/* the SEAL's word at 0xEB8, which the vehicle state tests before letting go (zseal.h does not
 * name it -- the name ours) */
struct CSealVehicleView {
    unsigned char _0[0xeb8];
    void *m_eb8;   // 0xeb8

    /* an inline bool accessor: the caller reloads the SEAL for its next statement */
    bool InVehicle() const { return m_eb8 != 0; }
};

/* the SEAL in a vehicle, read off the controller (the target reloads the SEAL after the test, as
 * an inline accessor's load is not reused) */
inline bool InVehicle(CSealCtrlAi *ai)
{
    return ((CSealVehicleView *)ai->m_entity)->InVehicle();
}

/* the state's events: reset, a vehicle state left (1, 2) or ended (3) (OVL2 0x60F5E0).
 * OPEN, 4 instructions: the target loads ai->m_entity again for the first call (lw a0,4(s0) in
 * the test's delay slot) where MW here reuses the test's load (lw a0 first, li a1,1 in the delay
 * slot) -- written out, through an inline bool accessor on the SEAL or on the controller, the
 * same; the call may be a member call on the SEAL (its row, 0x545EE0, the SEAL lane's, unnamed) */
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
 * 0x60F9F0; unit_nz when none is given; the SEAL's 0x5DF8B0 answers, the name ours) */
extern "C" bool func_0060F9F0(CSealCtrlAi *ai, const CPnt3D *dir)
{
    CEntity *seal = ai->m_entity;

    if (!dir)
        dir = &CPnt3D::unit_nz;
    bool r = func_005DF8B0(seal, dir, 0);

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
        if (InVehicle(ai)) {
            func_00545EE0(ai->m_entity, 1);
            func_00600D40(ai, 10);
            ai->SetDoAiMapCollide(true);
            func_0056D090(ai);
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
            func_00600DD0(ai, 10, pos, 0);
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
