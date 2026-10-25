/* seal_aifollow.cpp -- the follow state, CAiSFollow (SOCOM 1's unit, __sinit_seal_aifollow.cpp).
 * OVL2 0x5FADF0-0x5FE1EF: the destructor first, as SOCOM 1's. So far only the destructor and the
 * new slot +0x3C. Defined in reverse of their image order (-inline deferred). */

#include "ai_states.h"

extern "C" float data_00650C38;   /* the follow's close range (a variable: read with lwc1) */

/* (OVL2 0x5FDD00) */
bool CAiSFollow::Virtual3C()
{
    return m_f4 < data_00650C38 && (m_6c & 8) != 0;
}

/* a record whose first byte's low five bits are a kind (the follow's Tick and 0x5FD790 test it;
 * the names ours) */
struct CAiFollowKind {
    unsigned char m_kind : 5;
};

/* whether the kind is 2, 4, 5 or 12 (OVL2 0x5FC550; the name ours): a bit test written out -- MW
 * lowers a switch over those cases to a compare chain -- true through a bool local (`andi v0,a0,
 * 0xff` from the shift's 1) */
extern "C" bool func_005FC550(const CAiFollowKind *k)
{
    bool r = false;
    unsigned int t = k->m_kind - 2;

    if (t < 11 && ((1 << t) & 0x40d))
        r = true;
    return r;
}

/* (OVL2 0x5FADF0) */
CAiSFollow::~CAiSFollow()
{
}
