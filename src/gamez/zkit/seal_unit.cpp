/* seal_unit.cpp -- the squads, CSealUnit (SOCOM 1's unit, __sinit_seal_unit.cpp). OVL2
 * 0x5E6AA0-0x5EF9CF. So far its tail's out-of-line copies (the breach state's destructor, and with
 * its table the inline virtuals and CAiState's destructor), GetUnitByTeam, Create,
 * EscorteeJoinUnit, the constructor (with the orders list's deleter copy), TickAll, the auto-message
 * pair, GetMyPointOutTurnDir and both IsMemberOfUnit; round 14: the order data's GetData and
 * pointer pair, ClearAiStates, UnInit, Notify, PlayRemoteMsg, run_anim (SOCOM 1's can_run_anim
 * inlined), CZSealBody::IsAi's copy, 0x5EAEB0, 0x5EB8A0 and the orders' range erase (0x5EF610);
 * round 15: the destructor (the lists as SOCOM 1's slist/ulist/clist classes) and GetSealCount
 * (0x5ED9B0, open on the MSL's depths: 0x5ED8F0/0x5ED950/0x5ED980 are its iterator copies).
 * Open: 0x5EBDC0 is CAiState's implicit operator= (0x5EBB50 assigns a breach state to the one it
 * pushed: the vptr skipped, m_time, the stack -- the list's assignment, then its words at 0xC-0x1B
 * copied as one aggregate through the FPU, lwc1/swc1, m_result with lw/sw -- then CAiState's
 * members, m_loc an aggregate too). The list assignment is MSL's: x.end(), then
 * min(size(), x.size()) through references, the element at that index found from the nearer end
 * (0x5EF7F0, an advance either way), and the assign of [first, mid, last) (0x5EF470: past mid a
 * temporary list built on this one's allocator (0x5EF9B0) and init'ed (0x5EF8D0), the overlap
 * copied, the temporary spliced on (0x5EF950) and destroyed (0x510110); else the overlap copied
 * and the rest erased inline). include/list has none of it. Defined in reverse of their image
 * order (-inline deferred) where it matters. */

#include <list>
#include <string.h>

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

/* what a squad is told of its SEALs (SOCOM 1's UNIT_MESSAGE, a byte; the names ours) */
enum UNIT_MESSAGE { UNIT_MSG_0 = 0, UNIT_MSG_1 = 1, UNIT_MSG_2 = 2, UNIT_MSG_3 = 3, UNIT_MSG_4 = 4 };

/* a squad command (zseal.h's FT_COMMAND, which this unit does not include: the same byte enum) */
enum FT_COMMAND { CMD_UNKNOWN = 0, CMD_ACTION = 1, CMD_LAST = 255 };

/* the SEAL's controller as far as its AI: CSealCtrl's table to slot 0x38, GetSealCtrlAi (zseal.h's
 * CSealCtrl, which this unit does not include; the five slots before it unnamed here) */
class CUnitSealCtrl : public CEntityCtrl {
public:
    virtual void Virtual24();
    virtual void Virtual28();
    virtual void Virtual2C();
    virtual bool IsAi() const;   /* +0x30 */
    virtual void Virtual34();
    virtual class CSealCtrlAi *GetSealCtrlAi();
};

/* the SEAL as far as this unit reads it (zseal.h's CZSealBody, which this unit does not include:
 * CEntity first, at 0) */
class CZSealBody : public CEntity {
public:
    /* whether the AI drives it: the controller tested, IsAi called (the shape of zseal.h's IsPlayer
     * and IsRemote; the name ours) -- an inline whose out-of-line copy follows IssueCommand (OVL2
     * 0x5EA880), defined here until IssueCommand is written */
    bool IsAi();

    unsigned char _170[0x1060 - sizeof(CEntity)];
    unsigned int m_do_drop_weapons : 1;   // 0x1060 (zseal.h's bits)
    unsigned int m_fix_skel_y : 1;
    unsigned int m_inside : 1;   // bit 2
};

/* the AI controller as far as this unit reads it (zseal.h's CSealCtrlAi, which this unit does not
 * include): its states dropped (SOCOM 1's ClearAiState, OVL2 0x5F5F20) and its bits at 0x264
 * (SEAL_AI_PARAMS's m_debug and m_navyseal) */
class CSealCtrlAi {
public:
    bool ClearAiState(bool all);

    unsigned char _0[0x264];
    unsigned int m_debug : 1;   // 0x264
    unsigned int m_navyseal : 1;   // bit 1
};

/* a point-out's state, the leader's and the follower's (SOCOM 1's DWARF: UnitPointData, 0x14 bytes,
 * the same in SOCOM II) */
class UnitPointData {
public:
    /* (inline: the squad's constructor stores the bytes, the direction and the timer) */
    UnitPointData() : start_anim(0), in_point(0), dirW(CPnt3D::unit_nz), timer(0.0f) {}

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
    /* a squad of a number (SOCOM 1's; OVL2 0x5EE6F0: 200 bytes) and its end (OVL2 0x5EE4D0: the
     * compass anchor deleted, the lists emptied -- not virtual, UnInit calls it directly) */
    CSealUnit(unsigned int team);
    ~CSealUnit();
    /* every squad deleted, the made flag cleared (SOCOM 1's; OVL2 0x5ECEF0) */
    static void UnInit();
    /* the squad's SEALs' AI states dropped, `all` handed on to each, and with `recurse` the squads
     * under it -- a walk that never runs: its test is against begin() (OVL2 0x5E6C90, new in SOCOM
     * II; the name ours) */
    void ClearAiStates(bool all, bool recurse);
    /* a SEAL's news: 1 and 2 kept to this squad (0x5ED140), 0, 3 and 4 handed to the top squad's
     * (0x5ED4F0) -- SOCOM 1's PrivateNotify and DistributeNotify by their order (OVL2 0x5ED7B0) */
    void Notify(CZSealBody *seal, UNIT_MESSAGE msg, void *data);
    /* the top squad above this one (zseal.h's inline, the SEALs' round 14 name: its out-of-line copy
     * OVL2 0x561D20, in seal_ai.cpp's range, inlines itself eight times; Notify writes its first
     * level out and calls the copy, so this unit declares it out of line) */
    CSealUnit *GetTop();
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
    /* the squad's SEALs counted, with those of the squads under it to a depth (OVL2 0x5ED9B0, new in
     * SOCOM II: recursive, inlined into itself twice; the name ours) */
    int GetSealCount(int depth);

    class CCompassAnchor *m_compassanchor;   // 0x0
    /* the table at 4: IssueCommand, AllowCommand (0x669650) */
    virtual void IssueCommand(CZSealBody *seal, FT_COMMAND cmd, void *data, bool b);
    virtual bool AllowCommand(CZSealBody *seal, FT_COMMAND cmd, void *data);
    unsigned char m_8;   // 0x8 (new in SOCOM II: 1 when made; the name ours)
    unsigned char m_unit_in_point;   // 0x9 (SOCOM 1's 0x8)
    UnitPointData m_follower_point_data;   // 0xc
    UnitPointData m_leader_point_data;   // 0x20
    /* the squad's order data: up to 32 bytes and how many (new in SOCOM II, the 0x24 bytes before
     * the number; the names ours) -- copied in, or cleared when there are none; copied out only at
     * the size they were stored with (OVL2 0x5EF1D0: the AI's command handlers ask it; inline in
     * the pointer's getter and setter) */
    bool SetData(const void *data, unsigned int size)
    {
        bool ok = false;

        if (data) {
            memcpy(m_data, data, size);
            ok = true;
            m_data_size = size;
        } else {
            memset(m_data, 0, sizeof m_data);
            m_data_size = 0;
        }
        return ok;
    }
    bool GetData(void *data, unsigned int size);
    /* the order's data as a pointer, or 0; and set so (OVL2 0x5E6D60, 0x5E6DD0: the HUD's command
     * menu and the AI's handlers read the target SEAL so; the names ours) */
    void *GetDataPtr();
    bool SetDataPtr(void *p);

    unsigned int m_data_size;   // 0x34
    unsigned char m_data[0x20];   // 0x38
    unsigned int m_unit_num;   // 0x58 (SOCOM 1's 0x38)
    unsigned int m_5c;   // 0x5c: the squad's bit, 1 << (number & 31) (new in SOCOM II; the name ours)
    CSealUnit *m_parent;   // 0x60 (SOCOM 1's 0x3c): the squad this one is a part of
    /* SOCOM 1's DWARF: the squad's three list types, classes over std::list (each adds a this-tested
     * level to its implicit destructor: the destructor's four tests per list) */
    class slist : public std::list<CZSealBody *> {
    };
    class ulist : public std::list<CSealUnit *> {
    };
    class clist : public std::list<FT_COMMAND> {
    };

    slist m_seals;   // 0x64 (SOCOM 1's 0x40): the squad's SEALs, the leader first
    slist m_escortees;   // 0x70: the escorted SEALs (SOCOM 1's at 0x4c)
    ulist m_sub_units;   // 0x7c: the squads under this one (SOCOM 1's at 0x58)
    slist m_dead_guys;   // 0x88 (SOCOM 1's 0x64)
    clist m_commands;   // 0x94 (SOCOM 1's at 0x70): the squad's orders
    unsigned char m_a0;   // 0xa0: set when an escortee joins so (SOCOM 1's m_issue_unit_follow, 0x7c)
    unsigned char m_a1;   // 0xa1 (SOCOM 1's m_issue_sub_unit_follow)
    unsigned char _a2;
    unsigned char m_a3;   // 0xa3 (SOCOM 1's m_member_died)
    unsigned char m_a4;   // 0xa4 (SOCOM 1's m_ambush)
    unsigned char _a5[0xb4 - 0xa5];
    CZSealBody *m_follow_source;   // 0xb4 (SOCOM 1's 0x8c)
    unsigned char _b8[0xbc - 0xb8];
    int m_point_ticks;   // 0xbc (SOCOM 1's 0x9c)
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
    if (*m_commands.begin() == 4 || HostEscort())
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

/* (OVL2 0x5ED9B0: the walk's postfix step and its iterator's out-of-line pieces in the inner
 * copies -- 0x5ED880-0x5ED9A0 before it, as CGTree::FindNode's and Destroy's precede them).
 * OPEN, 0x1A8 against 0x1F8: the shape is right (two self-inlined copies, the third a call with
 * depth - 3; postfix i++), the MSL under it too shallow -- include/list is the MSL lane's. The
 * target's wrapper begin()/end() are `iterator(base::begin())`/`iterator(base::end())` (the
 * outer copy passes the base iterator by value, lwc1/swc1 through the frame; the second copy calls
 * the base's begin() 0x5ED9A0 and the wrapper's constructor 0x5ED980), and its size() is two levels
 * (the second copy calls 0x5E8A80, a const reference to size_ -- SOCOM 1's compressed_pair first()
 * -- the third size() itself, 0x510B80). Tried in a scratch include/list (reverted): both changes
 * move the size and begin() calls to the target's copies, but then the third copy inlines the
 * wrapper's != one level more than the target (0x1B0); the real depths are the MSL lane's work
 * (README "Open, and next", level 6) */
int CSealUnit::GetSealCount(int depth)
{
    int n = m_seals.size();

    if (depth > 0)
        for (ulist::iterator i = m_sub_units.begin(); i != m_sub_units.end(); i++)
            n += (*i)->GetSealCount(depth - 1);
    return n;
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

/* (OVL2 0x5EE6F0; SOCOM 1's, the lists built out of line, the team's bit new) */
CSealUnit::CSealUnit(unsigned int team) : m_8(1), m_unit_num(team)
{
    m_parent = 0;
    m_a0 = 0;
    m_a1 = 0;
    m_a3 = 0;
    m_follow_source = 0;
    m_a4 = 0;
    m_unit_in_point = 0;
    m_point_ticks = 0;
    m_compassanchor = 0;
    m_5c = 1 << (team & 0x1f);
}

/* (OVL2 0x5EE4D0; after the constructor in the source: the compass anchor deleted, the squads
 * under it and the SEALs cleared, then the five lists' implicit destructors) */
CSealUnit::~CSealUnit()
{
    if (m_compassanchor) {
        CCompass::DeleteAnchor(m_compassanchor);
        m_compassanchor = 0;
    }
    m_sub_units.clear();
    m_seals.clear();
}

/* (OVL2 0x5EA880) */
bool CZSealBody::IsAi()
{
    return m_control ? ((CUnitSealCtrl *)m_control)->IsAi() : false;
}

/* a menu message sent to the other consoles (fts_remote.cpp's, OVL1 0x2BAD40: who, to whom -- -1
 * everyone -- and the message) */
enum R_SEND_TO { R_SEND_TO_3 = -3, R_SEND_TO_2, R_SEND_TO_ALL };
void SendMessageMessage(int who, R_SEND_TO to, int msg);

extern "C" {
/* a squad message played (OVL2 0x5E7960, unwritten: SOCOM 1's PlayRemoteMsg's body) */
void func_005E7960(CEntity *e, int msg);
}

/* a squad message played for a SEAL (SOCOM 1's PlayRemoteMsg(CEntity *, int): the same callers --
 * the message packets' handler four times, OnDeath three, CZKit::Fire twice, CSealCtrlAi::OnCommand
 * -- SOCOM II adding the third word, 1 when the network delivered it; else message 43 is sent on to
 * everyone first (OVL2 0x5E7F20). A switch: its case block sits first behind the beq */
void PlayRemoteMsg(CEntity *e, int msg, int remote)
{
    if (!e)
        return;
    switch (msg) {
    case 43:
        if (remote != 1)
            SendMessageMessage(e->m_id, R_SEND_TO_ALL, msg);
        func_005E7960(e, msg);
        break;
    default:
        func_005E7960(e, msg);
    }
}

/* the anim of a name when it can run: running (state 1), or in state 5 once the flag is down
 * (SOCOM 1's can_run_anim__FPCc, its guarded static now plain data -- OVL2 0x650AE0, 1; inlined
 * in its one caller, no copy of its own: `static inline` -- static alone still emits a copy, inline
 * alone gives the static outside linkage, compared by name instead of content) */
static inline CZAnim *can_run_anim(const char *name)
{
    static bool s_flag = true;
    CZAnim *anim = ZAnim.GetAnim(name, 0);

    if (anim && (anim->m_state == 1 || !s_flag && anim->m_state == 5))
        return anim;
    return 0;
}

/* an anim of a name run, handed the argument when there is one; whether it could (OVL2 0x5EF220,
 * new in SOCOM II: IssueCommand runs the squads' anims so; the name ours) */
bool run_anim(const char *name, void *arg)
{
    CZAnim *anim = can_run_anim(name);

    if (anim) {
        if (arg)
            anim->ActivateArg(1, 4, arg, 0);
        else
            anim->Activate(0);
        return true;
    }
    return false;
}

/* (OVL2 0x5EF1D0) */
bool CSealUnit::GetData(void *data, unsigned int size)
{
    bool ok = false;

    if (data && size && m_data_size == size) {
        memcpy(data, m_data, size);
        ok = true;
    }
    return ok;
}

extern "C" {
/* the squad's leader (OVL2 0x5E8A90, unwritten: SOCOM 1's GetLeader by position), its awareness
 * (0x5E76D0: SOCOM 1's GetAwareCounter by position), a squad sound by number with a chance (0x5EAF20)
 * and the squad's formation set (0x5EB6D0) */
CZSealBody *func_005E8A90(CSealUnit *unit);
int func_005E76D0(CSealUnit *unit);
void func_005EAF20(CSealUnit *unit, int sound, int b, float chance);
}

/* the squad's formation (the byte at 8: 1 when made; the names ours) */
enum UNIT_FORMATION { FORMATION_0 = 0, FORMATION_1 = 1, FORMATION_2 = 2, FORMATION_LAST = 255 };

extern "C" {
void func_005EB6D0(CSealUnit *unit, UNIT_FORMATION f, int b);
}

/* the formation set to the leader's: 2 inside, else 1, when it differs (OVL2 0x5EB8A0; the
 * formation byte and the name ours) */
extern "C" void func_005EB8A0(CSealUnit *unit)
{
    CZSealBody *leader = func_005E8A90(unit);

    if (leader) {
        UNIT_FORMATION f;

        if (leader->m_inside ? true : false)
            f = FORMATION_2;
        else
            f = FORMATION_1;
        if (f != unit->m_8)
            func_005EB6D0(unit, f, 0);
    }
}

/* sound 79 when the squad is aware, else 78 (OVL2 0x5EAEB0) */
extern "C" void func_005EAEB0(CSealUnit *unit)
{
    if (func_005E76D0(unit))
        func_005EAF20(unit, 79, 0, 1.0f);
    else
        func_005EAF20(unit, 78, 0, 1.0f);
}

extern "C" {
/* the news kept or handed out (0x5ED140, 0x5ED4F0; unwritten) */
void func_005ED140(CSealUnit *unit, CZSealBody *seal, UNIT_MESSAGE msg, void *data);
void func_005ED4F0(CSealUnit *unit, CZSealBody *seal, UNIT_MESSAGE msg, void *data);
}

/* (OVL2 0x5ED7B0; the compare chain tests 2, 1, 0, 4, 3: the cases in reverse) */
void CSealUnit::Notify(CZSealBody *seal, UNIT_MESSAGE msg, void *data)
{
    switch (msg) {
    case UNIT_MSG_3:
    case UNIT_MSG_4:
    case UNIT_MSG_0:
        func_005ED4F0(m_parent ? m_parent->GetTop() : this, seal, msg, data);
        break;
    case UNIT_MSG_1:
    case UNIT_MSG_2:
        func_005ED140(this, seal, msg, data);
        break;
    }
}

/* (OVL2 0x5ECEF0; SOCOM 1's, the list emptied from the front) */
void CSealUnit::UnInit()
{
    std::list<CSealUnit *>::iterator i = m_list.begin();
    std::list<CSealUnit *>::iterator e = m_list.end();

    while (i != e) {
        CSealUnit *unit = *i;

        i = m_list.erase(i);
        delete unit;
    }
    m_created_units_test = false;
}

/* (OVL2 0x5E6C90; pinned: at fifteen statements -inline auto takes it, and the target's recursion
 * is a call, not the self-inlined copies MW makes of it) */
#pragma auto_inline off
void CSealUnit::ClearAiStates(bool all, bool recurse)
{
    std::list<CZSealBody *>::iterator e = m_seals.end();

    for (std::list<CZSealBody *>::iterator i = m_seals.begin(); i != e; ++i) {
        CSealCtrlAi *ai = ((CUnitSealCtrl *)(*i)->m_control)->GetSealCtrlAi();

        if (ai)
            ai->ClearAiState(all);
    }
    if (recurse)
        for (std::list<CSealUnit *>::iterator u = m_sub_units.begin(); u != m_sub_units.begin(); ++u)
            (*u)->ClearAiStates(all, recurse);
}
#pragma auto_inline reset

/* (OVL2 0x5E6DD0) */
bool CSealUnit::SetDataPtr(void *p)
{
    return SetData(&p, 4);
}

/* (OVL2 0x5E6D60) */
void *CSealUnit::GetDataPtr()
{
    void *p = 0;
    void *r = 0;

    if (GetData(&p, 4))
        r = p;
    return r;
}
