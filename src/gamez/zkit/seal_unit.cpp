/* seal_unit.cpp -- the squads, CSealUnit (SOCOM 1's unit, __sinit_seal_unit.cpp). OVL2
 * 0x5E6AA0-0x5EF9CF. So far only its tail's out-of-line copies: the breach state's destructor
 * (and with its table the inline virtuals and CAiState's destructor). Defined in reverse of their
 * image order (-inline deferred). */

#include <list>

#include "../../ai/ai_states.h"

extern "C" {
/* CSealCtrlAi::GetAiState (OVL2 0x5F6010, seal_aistate.cpp's; zseal.h's CSealCtrlAi does not
 * declare it yet) */
CAiState *func_005F6010(CSealCtrlAi *ai);
}

/* the controller's current state if it is a breach (kind 17), else 0 (OVL2 0x5EA780, after
 * CAiState's constructor; the name ours) */
CAiSBreach *GetBreachState(CSealCtrlAi *ai)
{
    CAiState *s = func_005F6010(ai);

    return s && s->m_state == 17 ? (CAiSBreach *)s : 0;
}

/* a squad (SOCOM 1's DWARF: CSealUnit, 0xa4 bytes; zseal.h, the SEAL lane's, declares a stub of
 * its own -- this unit does not include it). Only what a written function touches */
class CSealUnit {
public:
    static CSealUnit *GetUnitByTeam(unsigned int team);

    unsigned char _0[0x58];
    unsigned int m_unit_num;   // 0x58 (SOCOM 1's 0x38)

    static std::list<CSealUnit *> m_list;   // 0x44D5B0 (OVL1 bss): every squad (SOCOM 1's)
};

/* the squad of a number (OVL2 0x5EF2F0; SOCOM 1's) */
CSealUnit *CSealUnit::GetUnitByTeam(unsigned int team)
{
    std::list<CSealUnit *>::iterator e = m_list.end();

    for (std::list<CSealUnit *>::iterator i = m_list.begin(); i != e; ++i)
        if (team == (*i)->m_unit_num)
            return *i;
    return 0;
}

/* (OVL2 0x5EA4D0; CAiState's destructor follows it in, 0x5EA450) */
CAiSBreach::~CAiSBreach()
{
}
