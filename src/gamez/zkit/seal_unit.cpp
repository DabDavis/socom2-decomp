/* seal_unit.cpp -- the squads, CSealUnit (SOCOM 1's unit, __sinit_seal_unit.cpp). OVL2
 * 0x5E6AA0-0x5EF9CF. So far its tail's out-of-line copies (the breach state's destructor, and with
 * its table the inline virtuals and CAiState's destructor), GetUnitByTeam, Create and
 * EscorteeJoinUnit. Defined in reverse of their image order (-inline deferred). */

#include <list>

#include "../../ai/ai_states.h"
#include "../zanim/zanim.h"
#include "../znet/znet.h"

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
    /* a squad of a number (SOCOM 1's; OVL2 0x5EE6F0, not written yet: 200 bytes) */
    CSealUnit(unsigned int team);
    static CSealUnit *GetUnitByTeam(unsigned int team);
    static CSealUnit *Create(unsigned int team);
    /* a SEAL taken out of its squad (SOCOM 1's, static; OVL2 0x5EDF90, not written yet) */
    static void SealLeaveUnit(CZSealBody *seal);
    void SealJoinUnit(CZSealBody *seal);
    void EscorteeJoinUnit(CZSealBody *seal);
    /* this squad's escort taken up on the session master of a network game: a squad number outside
     * the single-player squads' 1-32, then the network's two bytes (an inline bool through one
     * local -- the `||` materialised with its true set first, the `&&` re-assigned to it; the name
     * ours) */
    bool HostEscort() const
    {
        bool open = m_unit_num == 0 || m_unit_num > 32;

        if (open)
            open = theNetwork.m_bNetwork && theNetwork.m_bSessionMaster;
        return open;
    }

    unsigned char _0[0x58];
    unsigned int m_unit_num;   // 0x58 (SOCOM 1's 0x38)
    unsigned char _5c[0x70 - 0x5c];
    std::list<CZSealBody *> m_escortees;   // 0x70: the escorted SEALs (SOCOM 1's at 0x4c)
    unsigned char _7c[0x9c - 0x7c];
    unsigned char *m_9c;   // 0x9c: a block whose byte 8 is 4 for ... (SOCOM 1's at 0x78)
    unsigned char m_a0;   // 0xa0: set when an escortee joins so (SOCOM 1's 0x7c)
    unsigned char _a1[0xc0 - 0xa1];
    CValve *m_valve;   // 0xc0: the radio channel's valve, team 24's (SOCOM 1's 0xa0)
    unsigned int m_c4;   // 0xc4 (zeroed with the valve; new in SOCOM II -- the name ours)

    static std::list<CSealUnit *> m_list;   // 0x44D5B0 (OVL1 bss): every squad (SOCOM 1's)
};

extern "C" {
/* a squad's update after a member joins (OVL2 0x5E7830, unwritten; new in SOCOM II) */
void func_005E7830(CSealUnit *unit);
}

/* the controller's squad word (CSealCtrl at 0x10C; zseal.h does not name it -- the view ours) */
struct CUnitCtrl {
    unsigned char _0[0x10c];
    CSealUnit *m_unit;   // 0x10c
};

/* an escortee taken in: out of its old squad, then the leader's squad itself when this is squad
 * 17, else onto the escort list, its controller pointed here and the flag at 0xA0 set for ... (OVL2
 * 0x5EE160; SOCOM 1's, which recursed on a sub-squad) */
void CSealUnit::EscorteeJoinUnit(CZSealBody *seal)
{
    SealLeaveUnit(seal);
    if (m_unit_num == 17) {
        SealJoinUnit(seal);
        return;
    }
    m_escortees.push_back(seal);
    ((CUnitCtrl *)((CEntity *)seal)->m_control)->m_unit = this;
    if (m_9c[8] == 4 || HostEscort())
        m_a0 = 1;
    func_005E7830(this);
}

/* the squad of a number, made and listed when there is none; team 24's gets the radio channel's
 * valve (OVL2 0x5EF380; SOCOM 1's, the word after the valve zeroed with it) */
CSealUnit *CSealUnit::Create(unsigned int team)
{
    CSealUnit *unit = GetUnitByTeam(team);

    if (!unit) {
        unit = new CSealUnit(team);
        m_list.push_back(unit);
    }
    if (team == 24) {
        unit->m_valve = CValve::Create("mp_radio_channel_valve", VALVE_PERM);
        unit->m_c4 = 0;
    } else
        unit->m_valve = 0;
    return unit;
}

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
