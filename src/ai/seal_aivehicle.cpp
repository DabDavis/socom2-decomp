/* seal_aivehicle.cpp -- the vehicle state, CAiSUseVehicle (new in SOCOM II; the unit's name ours).
 * OVL2 from 0x60EE00 (its destructor) to 0x60F6BF at least: the destructor, Tick and a helper,
 * then SetTimeout, CanPushAiState and OnSignal. So far only the small ones. Defined in reverse of
 * their image order (-inline deferred). */

#include "../gamez/zseal/zseal.h"

extern "C" {
/* a CZSealBody function (OVL2 0x545EE0, the SEAL lane's range), told 1 as the state ends */
void func_00545EE0(CEntity *seal, int x);
/* a CSealCtrl function taking a signed byte (OVL2 0x600D40, unclaimed), told 10 as the state
 * ends */
void func_00600D40(CSealCtrlAi *ai, int x);
/* a CSealCtrlAi function (OVL2 0x56D090, the SEAL lane's range) */
void func_0056D090(CSealCtrlAi *ai);
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
        if (m_34) {
            m_38 = 0;
            m_34 = 0;
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

/* (OVL2 0x60EE00) */
CAiSUseVehicle::~CAiSUseVehicle()
{
}
