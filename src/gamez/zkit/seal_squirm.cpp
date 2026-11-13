/* seal_squirm.cpp -- the squirming hostage's controller, CSealCtrlSquirm (SOCOM 1's unit,
 * __sinit_seal_squirm.cpp). OVL2 0x5FE830-0x5FF1FF, in SOCOM 1's order. Defined in reverse of
 * their image order (-inline deferred). */

#include "zkit.h"
#include "../zinput/zinput.h"

extern "C" {
/* CZSealState::UpdateLocalSeal (SOCOM 1's UpdateLocalSeal__11CZSealStateFP10CZSealBody, called from
 * the same Tick; OVL1 0x2BD640), SOCOM II's taking a further argument */
void func_002BD640(CEntity *seal, int x);
/* CZKit::SetCurItemByIndex and SetCurFireMode (SOCOM 1's, called from the same Initalize; OVL2
 * 0x5C4B10, 0x5C37B0 -- by their mangled names: zseal.h's CZKit has no members to call them on yet) */
void SetCurItemByIndex__5CZKitFi(CZKit *kit, int index);
void SetCurFireMode__5CZKitF8FIREMODE(CZKit *kit, int mode);
/* CZKit::CurMagHasAmmo and Reload (kit.h's; OVL2 0x5BD530, 0x5C2A90) */
unsigned char CurMagHasAmmo__5CZKitFv(CZKit *kit);
bool Reload__5CZKitFv(CZKit *kit);
/* the pad's pitch stick (SOCOM 1's CCtrlrConfigs::GetPitchStickPosDilated; OVL1 0x2C6280) */
float func_002C6280(CPad *pad);
}

/* (OVL2 0x5FF0F0): the trigger built with its default times (its constructor inlined one level:
 * Rfloat(float) twice, the two range temporaries and SetTimes out of line), the timer with none
 * (CAiTimer(float) inlined whole) */
CSealCtrlSquirm::CSealCtrlSquirm() : m_250(0), m_254(0), m_timer(0.0f)
{
}

/* (OVL2 0x5FF090) */
CSealCtrlSquirm::~CSealCtrlSquirm()
{
}

/* the hostage's start: the kit's first item, fire mode 3, and the squirm's rhythm (OVL2
 * 0x5FEF60; byte-exact once Rfloat's conversion took its random factor into a local of its own) */
void CSealCtrlSquirm::Initalize()
{
    m_254 = 1;
    m_timer.m_min = 8.0f;
    m_timer.m_range = 0.0f;
    CZKit *kit = &((CZSealBody *)m_entity)->m_kit;

    if (kit) {
        SetCurItemByIndex__5CZKitFi(kit, 0);
        SetCurFireMode__5CZKitF8FIREMODE(kit, 3);
    }
    m_trigger.m_dn_time = 5.0f;
    m_trigger.m_up_time = 1.0f;
    m_trigger.m_marker = m_trigger.m_up_time;
    m_trigger.m_timer = m_trigger.m_marker + m_trigger.m_dn_time;
}

/* the controller's empty reactions (OVL2 0x5FEF50 back to 0x5FEF10) */
void CSealCtrlSquirm::OnWeaponHit(zdb::IntersectStruct *hit, CZProjectile *proj)
{
}

void CSealCtrlSquirm::OnStunDamage(STUN_TYPE type, float amount, const CPnt3D *from)
{
}

void CSealCtrlSquirm::OnSetHealth(float health, float old)
{
}

void CSealCtrlSquirm::OnDeath()
{
}

void CSealCtrlSquirm::ResetToInitialState()
{
}

/* a pad by its index, none past the last (SOCOM 1's inline over CInput's table) */
static inline CPad *GetPad(unsigned int n)
{
    return n < CInput::NUM_PADS ? CInput::m_pads[n] : 0;
}

/* the hostage's frame: a pad's button cycles the squirm, another hands the SEAL to a plain
 * controller; the aim held straight ahead, the trigger pulled on the trigger's rhythm, the stance
 * changed at random on the timer's, a weapon run dry swapped for the next; then the throttles of
 * the squirm (OVL2 0x5FE880; SOCOM 1's PreTick, 0x2E24F0 in the demo). Byte-exact (the kit's
 * round 63, unwritten before): the hand-over re-reads the entity into `seal` itself (a local of its
 * own takes another saved register); the trigger's state taken into a bool before the decrement,
 * SetTrigger after it (the store in the call's slot); the sine and cosine scaled in place after
 * both calls, the angle a local */
bool CSealCtrlSquirm::PreTick(float dT)
{
    CZSealBody *seal = (CZSealBody *)m_entity;
    CPad *pad = GetPad(m_padid);

    if (pad) {
        if (pad->m_state[9] == 1)
            m_254 = (m_254 + 1) % 3;
        if (pad->m_state[12] == 2 && pad->m_state[6] == 1) {
            CSealCtrl *ctrl = new CSealCtrl;

            seal = (CZSealBody *)m_entity;
            seal->SetController(ctrl);
            ctrl->m_entity = seal;
            return true;
        }
        static float k_pitch = 1.0f;

        TickAimPitch(dT, k_pitch * func_002C6280(pad));
    }
    seal->SetAimDirR(&CPnt3D::unit_nz, false);
    seal->SetWeaponState((WEAPON_STATE)1);
    bool up = m_trigger.m_timer > m_trigger.m_marker;

    m_trigger.m_timer -= dT;
    SetTrigger(up);
    if (m_trigger.m_timer <= 0.0f) {
        m_trigger.m_marker = m_trigger.m_up_time;
        m_trigger.m_timer = m_trigger.m_marker + m_trigger.m_dn_time;
    }

    bool fire = false;

    if (m_timer.m_timer != CAiTimer::infinite) {
        m_timer.m_timer -= dT;
        fire = m_timer.m_timer != CAiTimer::infinite && m_timer.m_timer <= 0.0f;
        if (fire)
            m_timer.m_timer = m_timer.Period();
    }
    if (fire) {
        switch (rand() % 3) {
        case 0:
            seal->SetStance(stateStand);
            break;
        case 1:
            seal->SetStance(stateCrouch);
            break;
        case 2:
            seal->SetStance(stateProne);
            break;
        }
    }

    CZKit *kit = &seal->m_kit;

    if (kit && !CurMagHasAmmo__5CZKitFv(kit) && !Reload__5CZKitFv(kit)) {
        static int s_item;

        m_254 = 0;
        SetCurItemByIndex__5CZKitFi(kit, ++s_item);
        m_trigger.SetTimes(0.2f, 0.1f);
        m_trigger.m_marker = m_trigger.m_up_time;
        m_trigger.m_timer = m_trigger.m_marker + m_trigger.m_dn_time;
    }
    m_250 += dT;
    switch (m_254) {
    case 0:
        SetThrottle(0, 0.0f);
        SetThrottle(2, 0.0f);
        SetThrottle(1, 0.0f);
        break;
    case 1:
        SetThrottle(0, 0.2f);
        SetThrottle(2, 0.2f);
        SetThrottle(1, 0.0f);
        break;
    case 2: {
        static float k_speed = 0.5f;
        float a = m_250;
        float s = sinf(a);
        float c = cosf(a);

        s *= k_speed;
        c *= k_speed;
        SetThrottle(0, s);
        SetThrottle(2, c);
        break;
    }
    }
    return true;
}

/* the SEAL's state sent on (OVL2 0x5FE850) */
bool CSealCtrlSquirm::Tick(float dT)
{
    func_002BD640(m_entity, 0);
    return true;
}

/* (OVL2 0x5FE840, 0x5FE830) */
bool CSealCtrlSquirm::PostMortemPreTick(float dT)
{
    return true;
}

void CSealCtrlSquirm::OnActionButton()
{
}
