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

/* (OVL2 0x5FADF0) */
CAiSFollow::~CAiSFollow()
{
}
