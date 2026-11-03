/* seal_handsig.cpp -- the SEALs' hand signals: each signal's anim for each stance, looked up by name
 * once and played on the body (SOCOM 1's unit, __sinit_seal_handsig.cpp: DoHandSignal,
 * InitHandSignals and CHandSignal(), between seal_ailines.cpp and the scan's unit). OVL2
 * 0x600290-0x60054F, its static initialiser 0x6690C0 (the signals' array built through
 * __construct_array with CHandSignal()'s out-of-line copy). Defined in reverse of their image order
 * (-inline deferred). */

#include <stdio.h>

#include "zseal.h"

/* an anim type by name (anim.cpp's) */
AnimType FindAnimType(const char *name);

extern "C" {
/* the body's top blend (OVL2 0x555170: RTStack<CZBodyAnimBlend>::Top's OVL2 copy, unnamed in the
 * map), a flag test on a blend's flags (0x5551A0) and the anim pushed on the body (0x588BC0: an anim
 * type, the state, a flag and a kind -- SOCOM 1's PushBlend) */
void *func_00555170(void *stack);
bool func_005551A0(unsigned short flags, unsigned short bit);
void *func_00588BC0(CZSealBody *seal, AnimType type, int state, int flag, int kind);
}

/* a signal's anims: standing, crouched, prone (SOCOM 1's CHandSignal, three ints; SOCOM II's
 * shorts) -- none until InitHandSignals finds them */
class CHandSignal {
public:
    CHandSignal();

    short m_anim[3];   // 0x0
};

/* (OVL2 0x600530: out of line, the array's initialiser hands it to __construct_array) */
CHandSignal::CHandSignal()
{
    m_anim[0] = m_anim[1] = m_anim[2] = -1;
}

/* every signal's anims (SOCOM 1's m_handSignal, a static of the unit; OVL2 bss 0x66BB20) */
static CHandSignal m_handSignal[HANDSIGNAL_COUNT];

/* the signals' anim names (OVL2 data 0x650D80) */
static const char *s_signalName[HANDSIGNAL_COUNT] = {
    "null", "follow", "holdposition", "fireatwill", "deploy", "Point"
};

/* a signal's name (inline; the name ours) */
static inline const char *SignalName(SEAL_HANDSIGNAL s)
{
    return s_signalName[s];
}

/* each signal's anims looked up by name, standing, crouched and prone -- the point one the same
 * anim standing and crouched, none prone (OVL2 0x600440; SOCOM 1's, written out signal by signal
 * there, a loop here; it reads no member) */
void CZSealBody::InitHandSignals()
{
    char name[256];

    for (int i = HANDSIGNAL_FOLLOW; i < HANDSIGNAL_COUNT; i++) {
        const char *s = SignalName((SEAL_HANDSIGNAL)i);
        CHandSignal *h = &m_handSignal[i];

        if (i != HANDSIGNAL_POINT) {
            sprintf(name, "Signal %s", s);
            h->m_anim[0] = FindAnimType(name);
            sprintf(name, "Crouch signal %s", s);
            h->m_anim[1] = FindAnimType(name);
            sprintf(name, "Prone signal %s", s);
            h->m_anim[2] = FindAnimType(name);
        } else {
            h->m_anim[1] = h->m_anim[0] = FindAnimType(s);
            h->m_anim[2] = -1;
        }
    }
}

/* a signal played: its anim for the SEAL's stance pushed unless it is already on top -- none while
 * the top blend refuses (its flag 0x40), the SEAL dead, carrying, hanging or on a ladder; the anim,
 * or -1 (OVL2 0x600290; SOCOM 1's, the top blend's flag test and the carry test new) */
AnimType CZSealBody::DoHandSignal(SEAL_HANDSIGNAL sig)
{
    CZBodyAnimBlend *top = (CZBodyAnimBlend *)func_00555170(&m_blends);

    if (!func_005551A0(top->m_flags, 0x40))
        return animTypeUndefined;
    short state = m_state;

    if (state == stateDead || top->m_state == stateDead)
        return animTypeUndefined;
    if (state == stateCarrying)
        return animTypeUndefined;
    if (m_carry)
        return animTypeUndefined;
    if (state == stateHang || top->m_state == stateHang)
        return animTypeUndefined;
    if (state == stateClimbLadder || top->m_state == stateClimbLadder)
        return animTypeUndefined;
    CHandSignal *h = &m_handSignal[sig];
    SEAL_STATE stance = (SEAL_STATE)state;
    int i;

    switch (stance) {
    case stateStand:
        i = 0;
        break;
    case stateCrouch:
        i = 1;
        break;
    case stateProne:
        i = 2;
        break;
    default:
        return animTypeUndefined;
    }
    AnimType a = (AnimType)h->m_anim[i];

    if (a != animTypeUndefined &&
        a != ((CZBodyAnimBlend *)func_00555170((char *)this + 0x1c0))->m_animType)
        func_00588BC0(this, a, stance, 0, 0);
    return a;
}
