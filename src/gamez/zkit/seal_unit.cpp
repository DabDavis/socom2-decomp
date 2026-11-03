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

/* a squad command (zseal.h's FT_COMMAND, which this unit does not include: the same byte enum) */
enum FT_COMMAND { CMD_UNKNOWN = 0, CMD_ACTION = 1, CMD_LAST = 255 };

/* the SEAL's controller as far as its AI: CSealCtrl's table to slot 0x38, GetSealCtrlAi (zseal.h's
 * CSealCtrl, which this unit does not include; the five slots before it unnamed here) */
class CUnitSealCtrl : public CEntityCtrl {
public:
    virtual void Virtual24();
    virtual void Virtual28();
    virtual void Virtual2C();
    virtual void Virtual30();
    virtual void Virtual34();
    virtual class CUnitSealAi *GetSealCtrlAi();
};

/* the AI controller's bits at 0x264 (SEAL_AI_PARAMS's, zseal.h's m_debug and m_navyseal) */
class CUnitSealAi {
public:
    unsigned char _0[0x264];
    unsigned int m_debug : 1;   // 0x264
    unsigned int m_navyseal : 1;   // bit 1
};

/* a point-out's state, the leader's and the follower's (SOCOM 1's DWARF: UnitPointData, 0x14 bytes,
 * the same in SOCOM II) */
class UnitPointData {
public:
    unsigned char start_anim;   // 0x0
    unsigned char start_turn;   // 0x1
    unsigned char in_point;   // 0x2
    CPnt3D dirW;   // 0x4
    float timer;   // 0x10
};

/* a squad (SOCOM 1's DWARF: CSealUnit, 0xa4 bytes; SOCOM II's 0xc8: a byte at 8 and 0x24 bytes
 * before the number added, the lists 0x24 on; zseal.h, the SEAL lane's, declares a stub of its own
 * -- this unit does not include it). Only what a written function touches */
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

    /* the SEAL's turn toward a point-out, taken once: the leader's (the first SEAL) or the
     * follower's (the second), when the squad is pointing and the AI is a SEAL's (SOCOM 1's; OVL2
     * 0x5E7580) */
    bool GetMyPointOutTurnDir(CZSealBody *seal, CPnt3D **dir);
    /* a SEAL among the squad's or its escortees, or with `recurse` in a squad under it; and the same
     * asked of a squad by number (SOCOM 1's; OVL2 0x5EDCC0, 0x5EDDD0) */
    bool IsMemberOfUnit(CZSealBody *seal, bool recurse);
    static bool IsMemberOfUnit(unsigned int team, CZSealBody *seal, bool recurse);

    class CCompassAnchor *m_compassanchor;   // 0x0
    /* the table at 4: IssueCommand, AllowCommand (0x669650) */
    virtual void IssueCommand(CZSealBody *seal, FT_COMMAND cmd, void *data, bool b);
    virtual bool AllowCommand(CZSealBody *seal, FT_COMMAND cmd, void *data);
    unsigned char m_8;   // 0x8 (new in SOCOM II: 1 when made; the name ours)
    unsigned char m_unit_in_point;   // 0x9 (SOCOM 1's 0x8)
    UnitPointData m_follower_point_data;   // 0xc
    UnitPointData m_leader_point_data;   // 0x20
    unsigned char _34[0x58 - 0x34];
    unsigned int m_unit_num;   // 0x58 (SOCOM 1's 0x38)
    unsigned int m_5c;   // 0x5c: the squad's bit, 1 << (number & 31) (new in SOCOM II; the name ours)
    CSealUnit *m_parent;   // 0x60 (SOCOM 1's 0x3c): the squad this one is a part of
    std::list<CZSealBody *> m_seals;   // 0x64 (SOCOM 1's 0x40): the squad's SEALs, the leader first
    std::list<CZSealBody *> m_escortees;   // 0x70: the escorted SEALs (SOCOM 1's at 0x4c)
    std::list<CSealUnit *> m_sub_units;   // 0x7c: the squads under this one (SOCOM 1's ulist at 0x58)
    unsigned char _88[0x9c - 0x88];
    unsigned char *m_9c;   // 0x9c: a block whose byte 8 is 4 for ... (SOCOM 1's at 0x78)
    unsigned char m_a0;   // 0xa0: set when an escortee joins so (SOCOM 1's 0x7c)
    unsigned char _a1[0xc0 - 0xa1];
    CValve *m_valve;   // 0xc0: the radio channel's valve, team 24's (SOCOM 1's 0xa0)
    unsigned int m_c4;   // 0xc4 (zeroed with the valve; new in SOCOM II -- the name ours)

    static std::list<CSealUnit *> m_list;   // 0x44D5B0 (OVL1 bss): every squad (SOCOM 1's)
    /* set once the squads are made, cleared when they go (SOCOM 1's; OVL1 bss 0x44D5C0) */
    static bool m_created_units_test;

    /* every top squad ticked, once the squads are made (SOCOM 1's; the scheduler's "UnitTick") and
     * one squad's tick (SOCOM 1's took the scheduler's data too: SOCOM II's callers pass none) */
    static bool TickAll(float dT, void *data);
    void Tick(float dT);
};

bool CSealUnit::m_created_units_test;

/* the squads' radio chatter on or off (SOCOM 1's bgAutoMessagesOn, OVL2 data 0x650B08) */
bool bgAutoMessagesOn = true;

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

/* every squad that is no part of another ticked, once they are made (OVL2 0x5E7FD0; SOCOM 1's,
 * the end taken once and the unit's tick handed no data) */
bool CSealUnit::TickAll(float dT, void *data)
{
    if (m_created_units_test) {
        std::list<CSealUnit *>::iterator e = m_list.end();

        for (std::list<CSealUnit *>::iterator i = m_list.begin(); i != e; ++i)
            if (!(*i)->m_parent)
                (*i)->Tick(dT);
    }
    return false;
}

/* the radio chatter asked about and set (OVL2 0x5E7FC0, 0x5E7FB0; SOCOM 1's, the UI's toggle
 * calls them) */
bool GetPlayAutoMessages()
{
    return bgAutoMessagesOn;
}

void SetPlayAutoMessages(bool on)
{
    bgAutoMessagesOn = on;
}

/* (OVL2 0x5E7580) */
bool CSealUnit::GetMyPointOutTurnDir(CZSealBody *seal, CPnt3D **dir)
{
    bool navy = ((CUnitSealCtrl *)((CEntity *)seal)->m_control)->GetSealCtrlAi()->m_navyseal;

    if (!navy)
        return false;
    if (m_unit_in_point) {
        std::list<CZSealBody *>::iterator i = m_seals.begin();

        if (*i == seal) {
            if (m_leader_point_data.start_turn) {
                *dir = &m_leader_point_data.dirW;
                m_leader_point_data.start_turn = 0;
                return true;
            }
        } else if (*++i == seal) {
            if (m_follower_point_data.start_turn) {
                *dir = &m_follower_point_data.dirW;
                m_follower_point_data.start_turn = 0;
                return true;
            }
        }
    }
    return false;
}

/* (OVL2 0x5EDDD0; GetUnitByTeam inlined) */
bool CSealUnit::IsMemberOfUnit(unsigned int team, CZSealBody *seal, bool recurse)
{
    CSealUnit *unit = GetUnitByTeam(team);

    return unit ? unit->IsMemberOfUnit(seal, recurse) : false;
}

/* the SEALs walked, then the escortees, then the squads under this one (OVL2 0x5EDCC0; SOCOM 1's) */
bool CSealUnit::IsMemberOfUnit(CZSealBody *seal, bool recurse)
{
    bool found = false;

    for (std::list<CZSealBody *>::iterator i = m_seals.begin(); !found && i != m_seals.end(); ++i)
        if (*i == seal)
            found = true;
    if (!found) {
        for (std::list<CZSealBody *>::iterator i = m_escortees.begin(); !found && i != m_escortees.end(); ++i)
            if (*i == seal)
                return true;
        if (!found && recurse)
            for (std::list<CSealUnit *>::iterator u = m_sub_units.begin(); !found && u != m_sub_units.end(); ++u)
                found = (*u)->IsMemberOfUnit(seal, true);
    }
    return found;
}
