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

/* whether the SEAL counts: not a remote SEAL with bit 9 of 0x1060 set (OVL2 0x57DFE0, new in SOCOM
 * II; 0x5AF930 asks it) */
extern "C" bool func_0057DFE0(CZSealBody *seal)
{
    if (seal->IsRemote() && seal->m_bit9)
        return false;
    return true;
}

/* a table of three-word entries with three words of state (the names ours) */
struct CTripleTable {
    unsigned int m_count;   // 0x0
    int m_4;   // 0x4
    int m_8;   // 0x8
    int m_c;   // 0xc
    struct Entry {
        int m_0, m_4, m_8;
    } *m_items;   // 0x10
};

/* the table cleared: its state zeroed, every entry zeroed (OVL2 0x58A950, new in SOCOM II; three of
 * the body's functions call it) */
extern "C" void func_0058A950(CTripleTable *t)
{
    t->m_4 = 0;
    t->m_8 = 0;
    t->m_c = 0;
    CTripleTable::Entry *e = t->m_items;

    for (unsigned int i = 0; i < t->m_count; i++, e++) {
        e->m_0 = 0;
        e->m_4 = 0;
        e->m_8 = 0;
    }
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
