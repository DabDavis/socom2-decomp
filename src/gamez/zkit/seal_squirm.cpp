/* seal_squirm.cpp -- the squirming hostage's controller, CSealCtrlSquirm (SOCOM 1's unit,
 * __sinit_seal_squirm.cpp). OVL2 0x5FE830-0x5FF1FF, in SOCOM 1's order. Defined in reverse of
 * their image order (-inline deferred). */

#include "zkit.h"

extern "C" {
/* CZSealState::UpdateLocalSeal (SOCOM 1's UpdateLocalSeal__11CZSealStateFP10CZSealBody, called from
 * the same Tick; OVL1 0x2BD640), SOCOM II's taking a further argument */
void func_002BD640(CEntity *seal, int x);
/* CZKit::SetCurItemByIndex and SetCurFireMode (SOCOM 1's, called from the same Initalize; OVL2
 * 0x5C4B10, 0x5C37B0 -- zseal.h's CZKit has no members to call them on yet) */
void func_005C4B10(CZKit *kit, int index);
void func_005C37B0(CZKit *kit, int mode);
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
        func_005C4B10(kit, 0);
        func_005C37B0(kit, 3);
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
