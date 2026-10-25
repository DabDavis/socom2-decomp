/* seal_body.cpp -- the SEAL's body: its actions, stances and motion (SOCOM 1's unit,
 * __sinit_seal_body.cpp; its strings put Action, RestrainBuddy, OKToStand and StanceChanged
 * there). OVL2 from about 0x578990 (RestrainBuddy) to seal_cfg.cpp. Defined in reverse of their
 * image order (-inline deferred). */

#include "zseal.h"

extern "C" {
/* (OVL2 0x58A4D0 and seal.cpp's 0x555170: what the SEAL holds, on a ladder or off it) */
void *func_0058A4D0(CZSealBody *seal, int a);
void *func_00555170(void *v);
}

/* bits of a halfword set or cleared (OVL2 0x58A920; seal_body.cpp's and seal_env.cpp's callers) */
extern "C" unsigned short func_0058A920(unsigned short flags, unsigned short bits, bool on)
{
    flags &= ~bits;
    if (on)
        flags |= bits;
    return flags;
}

/* (OVL2 0x57DC80; CSealCtrl::PreTick calls it twice) */
extern "C" void *func_0057DC80(CZSealBody *seal)
{
    if (seal->m_state == 3)
        return func_0058A4D0(seal, 0);
    return func_00555170((char *)seal + 0x1c0);
}

/* CBody::BodySetState (OVL1 0x28DFB0, the body lane's: the state stored) */
extern "C" void func_0028DFB0(CBody &body, int state);

/* the state set: the controller told of a change first (OVL2 0x58A6B0; SOCOM 1's, virtual in SOCOM
 * II -- CBody's slot 0x14, CZSealBody's 0x90) */
void CZSealBody::BodySetState(int state)
{
    if (m_control && state != m_state)
        ((CSealCtrl *)m_control)->OnStateChange((SEAL_STATE)m_state, (SEAL_STATE)state);
    func_0028DFB0(*this, state);
}

/* the stance's change taken, once (OVL2 0x582500; SOCOM 1's, the false return its own block) */
bool CZSealBody::StanceChanged()
{
    if (m_stancechanged) {
        m_stancechanged = false;
        return true;
    }
    return false;
}
