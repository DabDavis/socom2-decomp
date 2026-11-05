/* seal_unit.cpp -- the squads, CSealUnit (SOCOM 1's unit, __sinit_seal_unit.cpp). OVL2
 * 0x5E6AA0-0x5EF9CF. So far its tail's out-of-line copies (the breach state's destructor, and with
 * its table the inline virtuals and CAiState's destructor), GetUnitByTeam, Create,
 * EscorteeJoinUnit, the constructor (with the orders list's deleter copy), TickAll, the auto-message
 * pair, GetMyPointOutTurnDir and both IsMemberOfUnit; round 14: the order data's GetData and
 * pointer pair, ClearAiStates, UnInit, Notify, PlayRemoteMsg, run_anim (SOCOM 1's can_run_anim
 * inlined), CZSealBody::IsAi's copy, 0x5EAEB0, 0x5EB8A0 and the orders' range erase (0x5EF610);
 * round 15: the destructor (the lists as SOCOM 1's slist/ulist/clist classes) and GetSealCount
 * (0x5ED9B0, open on the MSL's depths: 0x5ED8F0/0x5ED950/0x5ED980 are its iterator copies);
 * round 21: FindRescuedNear (0x5EEDB0, new) and GetMyLookDir (0x5EEF80, SOCOM 1's), the stand-in
 * controller given its real state stack for AiStateIs.
 * round 33: the breach push 0x5EBB50 with CAiState's implicit operator= 0x5EBDC0 and CAiTimer's
 * 0x5EBDA0 after it, and the list assignment's advance_to 0x5EF7F0 and splice 0x5EF950 in the tail.
 * OPEN, 0x1D8 against 0x198: the assignment's do_assign 0x5EF470 (include/list's) -- the target
 * calls the temporary's destructor (0x510110) and inlines the else arm's range erase; here both are
 * inlined (the destructor's out-of-line copy is generated mid-unit, after the constructor, before
 * the tail: taken out with the constructor and destructor, the destructor is inlined one level and
 * both erases are called, 0x170), so the original generated the destructor after do_assign and the
 * erase before it. OPEN, 0x70 against 0x78: init 0x5EF8D0 calls end() out of line (0x50FED0)
 * before each insert, as OVL1's 0x317720 does; here end() is inlined. Defined in reverse of their image
 * order (-inline deferred) where it matters. */

#include <list>
#include <stdio.h>
#include <string.h>

#include "../../ai/ai_states.h"
#include "../zanim/zanim.h"
#include "../znet/znet.h"
#include "../zsnd/zsnd.h"

extern "C" {
/* CSealCtrlAi::GetAiState (OVL2 0x5F6010, seal_aistate.cpp's; zseal.h's CSealCtrlAi does not
 * declare it yet) */
CAiState *GetAiState__11CSealCtrlAiFv(CSealCtrlAi *ai);
}

/* the controller's current state if it is a breach (kind 17), else 0 (OVL2 0x5EA780, after
 * CAiState's constructor; the name ours) */
CAiSBreach *GetBreachState(CSealCtrlAi *ai)
{
    CAiState *s = GetAiState__11CSealCtrlAiFv(ai);

    return s && s->m_state == 17 ? (CAiSBreach *)s : 0;
}

/* what a squad is told of its SEALs (SOCOM 1's UNIT_MESSAGE, a byte; the names ours) */
enum UNIT_MESSAGE { UNIT_MSG_0 = 0, UNIT_MSG_1 = 1, UNIT_MSG_2 = 2, UNIT_MSG_3 = 3, UNIT_MSG_4 = 4 };

/* the squad's formation is ai_states.h's UNIT_FORMATION (the byte at 8) */

/* a squad command (zseal.h's FT_COMMAND, which this unit does not include: the same byte enum) */
enum FT_COMMAND { CMD_UNKNOWN = 0, CMD_ACTION = 1, CMD_LAST = 255 };

/* the SEAL's controller as far as its AI: CSealCtrl's table to slot 0x38, GetSealCtrlAi (zseal.h's
 * CSealCtrl, which this unit does not include; the five slots before it unnamed here) */
class CUnitSealCtrl : public CEntityCtrl {
public:
    /* (zseal.h's CSealCtrl::PlayAiSound: SOCOM II's takes a chance and the AI event the voice makes,
     * and says whether it played) */
    virtual bool PlayAiSound(int sound, float chance, int event);   /* +0x24 */
    virtual void Virtual28();
    virtual bool IsPlayer() const;   /* +0x2C */
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
    /* whether the player drives it (zseal.h's IsPlayer: the controller tested, slot 0x2C called) */
    bool IsPlayer() { return m_control ? ((CUnitSealCtrl *)m_control)->IsPlayer() : false; }
    /* an order given to the SEAL (OVL2 0x590AC0, zseal.h's) */
    void IssueCommand(CZSealBody *from, FT_COMMAND cmd, void *data, bool b);

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
    /* a state pushed, and the current one (zseal.h's, OVL2 0x5F5C00, 0x5F6010) */
    bool PushAiState(AI_STATE state, unsigned int flags);
    CAiState *GetAiState();

    unsigned char _0[0x264];
    unsigned int m_debug : 1;   // 0x264
    unsigned int m_navyseal : 1;   // bit 1
    unsigned int m_bits2 : 14;   // bits 2-15
    /* bits 16 and 17 (zseal.h's m_aibit16/17: a breach begun clears both, the state's bit 13 set
     * from 16 without 17 -- 0x5EBB50) */
    unsigned int m_aibit16 : 1;
    unsigned int m_aibit17 : 1;
    unsigned char _267[0x26c - 0x267];   /* (the bits take three bytes) */
    /* SEAL_AI_PARAMS's two mode bytes at 0x26C/0x26D (zseal.h's m_26c: 4, any target taken) */
    signed char m_26c;   // 0x26c
    signed char m_26d;   // 0x26d
    unsigned char _26e[0x41c - 0x26e];
    /* its states (SEAL_AI_PARAMS's stack: the kinds on it a bit each at 0x42C, m_10) */
    CAiStateStack m_stateStack;   // 0x41c

    /* whether the current state is of a kind (zseal.h's AiStateIs: the stack's const GetState
     * called, the test built whole with its explicit false arm) */
    bool StackNotEmpty() const { return !(m_stateStack.size() == 0); }
    bool AiStateIs(AI_STATE state) const
    {
        return StackNotEmpty() ? (m_stateStack.GetState()->m_state == state ? true : false) : false;
    }
};

/* the AI maps as far as this unit reads them (ai_maps.h's CAiMaps, which this unit does not include):
 * two cells' distance (OVL2 0x52C320, compared signed) */
class CAiMaps {
public:
    unsigned int GetLocationDelta(const CAiMapLoc &a, const CAiMapLoc &b);
    /* an action found by its id, with its cell (ai_maps.h's, OVL2 0x52DAA0) */
    bool GetActionById(unsigned int id, CAiMapLoc &loc, CAiMap::action **pa);
};
extern CAiMaps AiMaps;

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

/* the pursuit state (seal_ai.cpp's, the SEAL lane's: its table 0x6692B0, its timer at 0x58): the squad's
 * private news builds one on the stack as the model of the pushes -- its destructor implicit here, as
 * the target inlines it (CAiState's table stored, the stack's destructor called) */
class CAiSPursue : public CAiState {
public:
    CAiSPursue() : CAiState(AI_STATE_PURSUE) {}
    virtual void OnSignal(SIGNAL sig, CSealCtrlAi *ctrl);
    virtual int Virtual14(CSealCtrlAi *ai, CAiState *sub);
    virtual bool Virtual44(AI_STATE state) { return state == m_state; }
    virtual bool Virtual48(int x, CAiState *other);

    float m_timer;   // 0x58 (SOCOM 1's 0x28)
    signed char m_5c;   // 0x5c
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
    /* the squads made for a single-player mission and for a network game (SOCOM 1's, called where
     * SOCOM 1's are -- CMission::Open by theNetwork.m_bNetwork, ReOpen_Load the first: OVL2
     * 0x5EC6A0, 0x5ECCD0), and a SEAL put in a squad, the one given or else its team's (SOCOM 1's
     * InitalizeSealUnit, the squad argument new: OVL2 0x5EC9F0, between them as in SOCOM 1) */
    static void CreateSinglePlayerUnits();
    static void InitalizeSealUnit(CZSealBody *seal, CSealUnit *unit);
    static void CreateMultiPlayerUnits();
    /* the squad's SEALs' AI states dropped, `all` handed on to each, and with `recurse` the squads
     * under it -- a walk that never runs: its test is against begin() (OVL2 0x5E6C90, new in SOCOM
     * II; the name ours) */
    void ClearAiStates(bool all, bool recurse);
    /* a state of a kind pushed on each of the squad's AI SEALs, when the model state takes the kind,
     * and the new state given the model's steps, clocks and two bits, then handed it (slot 0x48) --
     * with `recurse` the squads under it too, a walk that never runs (its test against begin(), as
     * ClearAiStates'); OVL2 0x5E6AB0, new in SOCOM II: the squad's private news 0x5ED140 calls it;
     * the name ours */
    void PushAiState(AI_STATE state, CAiState *proto, bool recurse);
    /* a SEAL's news: 1 and 2 kept to this squad (0x5ED140), 0, 3 and 4 handed to the top squad's
     * (0x5ED4F0) -- SOCOM 1's PrivateNotify and DistributeNotify by their order (OVL2 0x5ED7B0) */
    void Notify(CZSealBody *seal, UNIT_MESSAGE msg, void *data);
    void DistributeNotify(CZSealBody *seal, UNIT_MESSAGE msg, void *data);
    void PrivateNotify(CZSealBody *seal, UNIT_MESSAGE msg, void *data);
    /* the top squad above this one (zseal.h's inline, the SEALs' round 14 name: its out-of-line copy
     * OVL2 0x561D20, in seal_ai.cpp's range, inlines itself eight times; Notify writes its first
     * level out and calls the copy, so this unit declares it out of line) */
    CSealUnit *GetTop();
    static CSealUnit *GetUnitByTeam(unsigned int team);
    static CSealUnit *Create(unsigned int team);
    /* a SEAL back from the dead: off the dead list and onto the escortees (a team-bit-17 SEAL of
     * squad 17) or the SEALs (SOCOM 1's; OVL2 0x5EDE40) */
    void Revive(CZSealBody *seal);
    /* the SEALs of a team an order can go to: the one the SEAL looks at when it is of the team (and
     * then only it), else the team's among its squad's escortees; then, when `unit` is the team's
     * squad, every SEAL of it -- each added to `out` (OVL2 0x5EA8C0, new in SOCOM II: IssueCommand
     * and AllowCommand ask it for squad 18, `this` and `unit` the same squad -- this one unread;
     * the name ours) */
    void GetTeamSeals(int team, CZSealBody *seal, CSealUnit *unit, std::list<CZSealBody *> *out);
    /* a SEAL taken out of its squad: off its SEALs, escortees and dead, the follow's source handed on
     * to the new leader, the follow flag set again when the squad still follows; the squad updated
     * (SOCOM 1's, static; OVL2 0x5EDF90) */
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
    /* a formation given the squad, slots counted on from an index; the next one returned (OVL2
     * 0x5EB6D0; the name ours) */
    int SetFormation(UNIT_FORMATION f, int index);
    /* an order the network squads take as radio traffic (OVL2 0x5E6E40; the name ours) */
    bool RadioCommand(CZSealBody *seal, FT_COMMAND cmd);
    /* a target handed to the squad's AI SEALs, the SEAL's place in the squad returned (OVL2
     * 0x5ECF90; the name ours) */
    int ShareTarget(CZSealBody *seal, CEntity *target, bool force, int unused);
    /* the SEAL's point-out anim started, taken once: the leader's or the follower's start byte, when
     * the squad is pointing, the AI is a SEAL's and the player stands within 200 and faces away from
     * it (SOCOM 1's, its guarded statics plain data here -- 0x650B10, 0x650B18; OVL2 0x5E7390) */
    bool GetMyPointOutAnimStart(CZSealBody *seal);
    /* a SEAL's look out of a direction (SOCOM 1's; OVL2 0x5EEF80) */
    void GetMyLookDir(CPnt3D *out, CPnt3D *dir, CZSealBody *seal);
    /* a SEAL among the squad's or its escortees, or with `recurse` in a squad under it; and the same
     * asked of a squad by number (SOCOM 1's; OVL2 0x5EDCC0, 0x5EDDD0) */
    bool IsMemberOfUnit(CZSealBody *seal, bool recurse);
    static bool IsMemberOfUnit(unsigned int team, CZSealBody *seal, bool recurse);
    /* the squad's SEALs counted, with those of the squads under it to a depth (OVL2 0x5ED9B0, new in
     * SOCOM II: recursive, inlined into itself twice; the name ours, const by its walk's pieces) */
    int GetSealCount(int depth) const;
    /* the index'th SEAL of the squad, counted on through the squads under it to a depth, or 0
     * (SOCOM 1's GetSealByIndex(unsigned int), the depth new; OVL2 0x5EDBB0) */
    CZSealBody *GetSealByIndex(int index, int depth);
    /* the squad's last SEAL: the last squad under it asked first, then its own SEALs', escortees'
     * and dead's last; and its leader, the first SEAL, else the first squad's (SOCOM 1's GetTail and
     * const GetLeader, before CommandNotifyFromChild in both builds: OVL2 0x5E8850, 0x5E8A90) */
    CZSealBody *GetTail();
    CZSealBody *GetLeader() const;

    class CCompassAnchor *m_compassanchor;   // 0x0
    /* the table at 4: IssueCommand, AllowCommand (0x669650) */
    virtual void IssueCommand(CZSealBody *seal, FT_COMMAND cmd, void *data, bool b);
    virtual bool AllowCommand(CZSealBody *seal, FT_COMMAND cmd, void *data);
    UNIT_FORMATION m_8;   // 0x8 (new in SOCOM II: 1 when made; the name ours -- the enum: passed on
                          // to the formation setter it loads with lb)
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
    /* the rescued AI SEAL near one, while the extraction can run (OVL2 0x5EEDB0) */
    CZSealBody *FindRescuedNear(CZSealBody *seal);
    /* the order's data as a pointer, or 0; and set so (OVL2 0x5E6D60, 0x5E6DD0: the HUD's command
     * menu and the AI's handlers read the target SEAL so; the names ours) */
    void *GetDataPtr();
    bool SetDataPtr(void *p);
    /* the squad's awareness: its SEALs' controllers' aware counts, with those of every squad under it
     * (SOCOM 1's GetAwareCounter by its place and its walks -- the count read off the SEAL's
     * controller here, SOCOM 1's off the SEAL; OVL2 0x5E76D0: the squad's aware sound 0x5EAEB0 asks
     * it) */
    int GetAwareCounter() const;
    /* the farthest any of the squad's SEALs stands from a point, squared, with the squads under it
     * to a depth (OVL2 0x5E7230, new in SOCOM II: AllowCommand asks it twice; the name ours) */
    float GetMaxDistSq(const CPnt3D *pos, int depth);
    /* the squad's centre: its SEALs' positions (only the AI's with `ai`) averaged, with those of the
     * squads under it to a depth weighted by their counts; the count returned (OVL2 0x5E7030, new
     * in SOCOM II -- SOCOM 1's CheckPathProximity's size and place, not its code: AllowCommand asks
     * it; the name ours) */
    float GetCenter(CPnt3D *out, int depth, bool ai);

    void AddSub(CSealUnit *const &unit) { m_sub_units.push_back(unit); }
    unsigned int m_data_size;   // 0x34
    unsigned char m_data[0x20];   // 0x38
    unsigned int m_unit_num;   // 0x58 (SOCOM 1's 0x38)
    unsigned int m_5c;   // 0x5c: the squad's bit, 1 << (number & 31) (new in SOCOM II; the name ours)
    CSealUnit *m_parent;   // 0x60 (SOCOM 1's 0x3c): the squad this one is a part of

    /* a squad put under this one: listed, its parent set and this one's bits added to its own (an
     * inline taking the squad by value -- CreateSinglePlayerUnits re-reads it off the stack after
     * the insert, a copy per call; SOCOM 1 wrote the push and the parent out; the name ours). The
     * push through AddSub, a level deeper: only so do the inserts' temporaries sit under the entity
     * walk's in the frame, as the target's do (a stand-in for whatever level the original had) */
    void AddSubUnit(CSealUnit *unit)
    {
        AddSub(unit);
        unit->m_parent = this;
        unit->m_5c |= m_5c;
    }
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
    unsigned char m_a2;   // 0xa2 (new in SOCOM II: set when the squad is told to follow a SEAL -- 0x5EB580; the name ours)
    unsigned char m_a3;   // 0xa3 (SOCOM 1's m_member_died)
    unsigned char m_a4;   // 0xa4 (SOCOM 1's m_ambush)
    unsigned char _a5[0xa8 - 0xa5];
    CPnt3D m_a8;   // 0xa8: the look a set m_a4 gives every SEAL (GetMyLookDir; SOCOM 1's at 0x80)
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
    /* a squad under this one told of an order (SOCOM 1's): on a follow (4), whether every AI SEAL
     * of this squad and every squad under it follows -- the flag at 0xA0 set -- or some do (0xA1) */
    void CommandNotifyFromChild(CSealUnit *child, CZSealBody *seal, FT_COMMAND cmd, void *data);
    /* the AI's two mode bytes (SEAL_AI_PARAMS 0x26C/0x26D) of the squad's first AI SEAL, else of the
     * last squad under it that has one; whether any had (OVL2 0x5EAA70, new in SOCOM II: the team
     * command menu asks it through 0x5EABA0; the name ours) */
    bool GetAiModes(signed char *a, signed char *b);
    /* a command voiced when the option asks for it (ShouldConfirmVoiceCommands), true when it does
     * not: ConfirmCommand the order itself, a sound named from the squad's two AI modes, the command's
     * name and the item's -- SOCOM 1's ConfirmCommand by that, by its test of the flag first and its
     * place after IssueCommand, its arguments SOCOM II's (OVL2 0x5EABA0, the HUD's menu orders);
     * AcknowledgeCommand the squad's answer, one of three canned sounds (OVL2 0x5EAD70, new in SOCOM
     * II: the HUD's voice-recognition orders; the name ours). Both take the command, a byte the item
     * commands ask and an int no caller sets but 0 */
    bool ConfirmCommand(FT_COMMAND cmd, unsigned char item, int flags);
    bool AcknowledgeCommand(FT_COMMAND cmd, unsigned char item, int flags);
    /* the squad told to follow a SEAL: it made the follow's source, every other SEAL of the squad,
     * the escortees with the first of them, and every squad under it given the follow (4) -- with no
     * SEAL to give it the escortees are seen to at once (0x5E7830); the SEAL handed back (OVL2
     * 0x5EB580, IssueCommand's four calls; new in SOCOM II, the name ours) */
    CZSealBody *FollowSeal(CZSealBody *seal);
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

/* the controller's aware count (CSealCtrl at 0xC0, in zseal.h's padding -- the view and the name
 * ours) */
struct CAwareCtrl {
    unsigned char _0[0xc0];
    int m_aware;   // 0xc0
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

/* (OVL2 0x5EDF90; SOCOM 1's) */
void CSealUnit::SealLeaveUnit(CZSealBody *seal)
{
    CSealUnit *unit = ((CUnitCtrl *)((CEntity *)seal)->m_control)->m_unit;

    if (!unit)
        return;
    bool found = false;

    for (std::list<CZSealBody *>::iterator e = unit->m_seals.end(), i = unit->m_seals.begin(); i != e; ++i)
        if (*i == seal) {
            unit->m_seals.erase(i);
            found = true;
            break;
        }
    for (std::list<CZSealBody *>::iterator e = unit->m_escortees.end(), i = unit->m_escortees.begin(); i != e; ++i)
        if (*i == seal) {
            unit->m_escortees.erase(i);
            found = true;
            break;
        }
    for (std::list<CZSealBody *>::iterator e = unit->m_dead_guys.end(), i = unit->m_dead_guys.begin(); i != e; ++i)
        if (*i == seal) {
            unit->m_dead_guys.erase(i);
            found = true;
            break;
        }
    if (found) {
        if (unit->m_follow_source == seal)
            unit->m_follow_source = unit->m_seals.size() == 0 ? 0 : *unit->m_seals.begin();
        if (unit->m_follow_source)
            if (*unit->m_commands.begin() == 4 || unit->HostEscort())
                unit->m_a0 = 1;
    }
    func_005E7830(unit);
}

/* (OVL2 0x5E7830) the squad's four lists walked and nothing done -- the bodies compiled out: the
 * SEALs, the dead, the escortees, the squads under it. One slist iterator ASSIGNED from each begin()
 * in turn and a ulist one declared after it: an assigned wrapper iterator stays in its stack slot,
 * the base iterator copied in through $f0 (the MSL headers round 15; a fresh iterator per walk, or
 * all four declared at the top, gives other slots) */
extern "C" void func_005E7830(CSealUnit *unit)
{
    CSealUnit::slist::iterator i;

    for (i = unit->m_seals.begin(); i != unit->m_seals.end(); ++i) {
    }
    for (i = unit->m_dead_guys.begin(); i != unit->m_dead_guys.end(); ++i) {
    }
    for (i = unit->m_escortees.begin(); i != unit->m_escortees.end(); ++i) {
    }
    CSealUnit::ulist::iterator u;

    for (u = unit->m_sub_units.begin(); u != unit->m_sub_units.end(); ++u) {
    }
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

/* the player (fts_player.cpp's, OVL1 0x2B3580) */
CZSealBody *ftsGetPlayer();

/* a point's difference, given back by value (inline here: SOCOM II's CPnt3D::operator- const,
 * whose copies zmath.h declares out of line) */
static inline CPnt3D Minus(const CPnt3D &a, const CPnt3D &b)
{
    CPnt3D r;

    a.Sub(&b, &r);
    return r;
}

/* (OVL2 0x5E7390) the result a local: `return true` in place gives `li` and its own block where
 * the target sets the bool with daddiu, its zero in the test's delay slot.
 * OPEN, 16 instructions at the right size, registers and two stores: the target gives `this` s1 (shared
 * after the walk with &d.y) and the start byte's pointer s2, here the other way round (the pointer
 * declared first, a dead first definition, a `self` copy and the bool test in place tried); and it
 * stores d.y's zero (in Normalize's delay slot) and the negated look.z straight to their stack slots
 * while every load of them goes through the saved address registers -- here the stores take the
 * registers too (LenSq/Dot/negate as inline helpers over references or pointers drop the address
 * registers altogether: 0x1E0-0x1E4) */
bool CSealUnit::GetMyPointOutAnimStart(CZSealBody *seal)
{
    bool navy = ((CUnitSealCtrl *)((CEntity *)seal)->m_control)->GetSealCtrlAi()->m_navyseal;

    if (!navy)
        return false;
    unsigned char *start = 0;

    if (m_unit_in_point) {
        std::list<CZSealBody *>::iterator i = m_seals.begin();

        if (*i == seal) {
            if (m_leader_point_data.start_anim)
                start = &m_leader_point_data.start_anim;
        } else if (*++i == seal) {
            if (m_follower_point_data.start_anim)
                start = &m_follower_point_data.start_anim;
        }
    }
    if (!start)
        return false;
    static float maxdist = 40000.0f;
    CPnt3D d = Minus(ftsGetPlayer()->m_posW, seal->m_posW);

    if (d.x * d.x + d.y * d.y + d.z * d.z > maxdist)
        return false;
    d.y = 0.0f;
    d.Normalize();
    CPnt3D look = *(const CPnt3D *)ftsGetPlayer()->m_node->m_matrix.m_matrix[2];

    look.x = -look.x;
    look.y = -look.y;
    look.z = -look.z;
    look.y = 0.0f;
    look.Normalize();
    static float mincos = -0.8f;

    bool ok = false;

    if (look.x * d.x + look.y * d.y + look.z * d.z < mincos) {
        *start = 0;
        ok = true;
    }
    return ok;
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
 * (README "Open, and next", level 6).
 * Measured again (the MSL headers round 15, build/p15/incB, untracked): with include/list's sz() under
 * size() and the wrapper's begin()/end() through the base's, every call this function makes is the
 * target's, copy by copy (sz 0x5E8A80, the base's begin 0x5ED9A0 and the wrapper's constructor 0x5ED980,
 * end 0x5ED8F0, the base's != 0x5ED880 in the first self-copy; size 0x510B80, begin 0x5ED950, the
 * wrapper's != 0x5ED8D0, * 0x5ED940, i++ 0x5ED920 in the second) -- the "third copy" is the raw one, so
 * nothing there inlines. What is left (0x1B0 against 0x1F8) is the outer copy: the target builds
 * begin()'s and end()'s base iterators in stack slots and copies each into the wrapper's through $f0
 * (the by-value parameter kept in memory), then loads the iterator into s0; MW scalarises all of it
 * here. Not it: the iterator assigned from begin() (0x1F4, but in memory through the whole walk), direct
 * initialisation, a while loop, the base iterator's field in a base class (no change).
 * THE WALK IS A CONST ONE (the kit lane, round 45): the pieces 0x5ED880-0x5ED9A0 are not a second copy
 * of DistributeNotify's 0x5ED6E0-0x5ED7A0 but list<CSealUnit *>'s generic_iterator<1> -- the wrapper's
 * constructor 0x5ED980 takes the base's const_iterator BY VALUE, 0x1C, word for word SOCOM 1's
 * list<CEntity *>::generic_iterator<1>(generic_iterator<1>) 0x321240, where DistributeNotify's 0x5ED7A0
 * takes generic_iterator<0> by const reference, and 0x5ED9A0 is the base's begin() const (the same
 * words as OVL2's begin() 0x510100, yet emitted again: another instance). So the function is const (the
 * name ours: the row renamed CFi): with the const walk the outer copy's by-value temporaries are the
 * target's -- 0x1F8, the target's size -- and the unit emits the walk's pieces in the target's order,
 * end() const 0x5ED8F0, begin() const 0x5ED950 and the by-value constructor 0x5ED980 byte-exact.
 * OPEN, 10 instructions, the first self-copy's size() and begin() and the third's element read, all
 * include/list's (the MSL lane's): the target calls sz() 0x5E8A80 and loads the count through it (ours
 * inlines size() whole), calls the base's begin() const 0x5ED9A0 (ours inlines the wrapper's begin()
 * const: tail_ptr() and the base iterator's constructor called), and its const_iterator's operator*
 * 0x5ED940 returns the element BY REFERENCE (`addiu v0,v0,8`, the caller loading it: `lw a0,0(v0)`),
 * where include/list's const_iterator returns it by value (fitted to GetEntity) */
int CSealUnit::GetSealCount(int depth) const
{
    int n = m_seals.size();

    if (depth > 0)
        for (ulist::const_iterator i = m_sub_units.begin(); i != m_sub_units.end(); i++)
            n += (*i)->GetSealCount(depth - 1);
    return n;
}

/* (OVL2 0x5EDBB0: the squad's own SEALs walked to the index, else the squads under it asked in
 * turn, each one's count taken off the index) */
CZSealBody *CSealUnit::GetSealByIndex(int index, int depth)
{
    CZSealBody *seal = 0;

    if (index >= 0 && (unsigned int)index < m_seals.size()) {
        slist::iterator i = m_seals.begin();

        while (index--)
            ++i;
        seal = *i;
    } else if (depth > 0) {
        index -= m_seals.size();
        for (ulist::iterator u = m_sub_units.begin(); !seal && u != m_sub_units.end(); ++u) {
            CSealUnit *unit = *u;

            seal = unit->GetSealByIndex(index, depth - 1);
            if (!seal)
                index -= unit->GetSealCount(depth - 1);
        }
    }
    return seal;
}

extern "C" {
/* the SEAL a SEAL is looking at (seal_env.cpp's, OVL2 0x5AA690: its cached one or its reticule
 * node's) */
CZSealBody *func_005AA690(CZSealBody *seal);
}

/* MSL's back_insert_iterator and copy as far as the squads use them (the target's second walk is
 * copy's: its first iterator a by-value parameter in the frame below the caller's temporaries) */
struct CSealPusher {
    CSealPusher(std::list<CZSealBody *> *l) : c(l) {}
    CSealPusher &operator=(CZSealBody *const &v)
    {
        c->push_back(v);
        return *this;
    }
    CSealPusher &operator*() { return *this; }
    CSealPusher &operator++() { return *this; }

    std::list<CZSealBody *> *c;
};

template <class I, class O>
inline O CopyAll(I first, I last, O result)
{
    for (; first != last; ++first, ++result)
        *result = *first;
    return result;
}

/* (OVL2 0x5EA8C0: the walks' iterators assigned, in memory; the looked-at SEAL in its home, its
 * address handed to push_back; `(1 << team) & m_team`, the mask first in the target's `and`).
 * OPEN, 0x1B8 against 0x1B0: the squad's walk. The target walks it with the iterator assigned from
 * begin() in the frame (two copies, its slot BELOW every push_back temporary), the end in a saved
 * register, the loop top-tested and not rotated, and out->end() rebuilt inside the loop. Written as
 * a plain walk after the early return MW hoists out->end() into a saved register and enters the
 * loop at its bottom test (0x1C8, the first walk's push temporaries a slot lower too); through an
 * inline (a plain walk, or as here MSL's copy over a back-inserter stand-in) the first walk's frame is
 * the target's but the squad's iterator goes to a register; `goto` for the return, a do-while and an
 * assigned iterator inside the copy (0x1D8) tried. Also: the escortees' end and the team mask take
 * s0/s1 the other way round */
void CSealUnit::GetTeamSeals(int team, CZSealBody *seal, CSealUnit *unit, std::list<CZSealBody *> *out)
{
    CZSealBody *s = 0;

    if (seal) {
        s = func_005AA690(seal);
        if (s && ((1 << team) & s->m_team)) {
            out->push_back(s);
            goto done;
        }
        CSealUnit *u = ((CUnitCtrl *)((CEntity *)seal)->m_control)->m_unit;
        std::list<CZSealBody *>::iterator i;

        for (i = u->m_escortees.begin(); i != u->m_escortees.end(); ++i) {
            s = *i;
            if ((1 << team) & s->m_team)
                out->push_back(s);
        }
    }
    if (unit && unit->m_unit_num == team) {
        CopyAll(unit->m_seals.begin(), unit->m_seals.end(), CSealPusher(out));
    }
done:
    ;
}

/* (OVL2 0x5EDE40; SOCOM 1's: the two walks after the dead list's look for the SEAL and do nothing
 * -- compiled-out code. Each walk's end taken into a local ahead of its iterator: the target's
 * iterator in a1, the end in a2.) The SEAL lives in its home (push_back takes its address) and the
 * target reads it there again ahead of the second and third walks (`lw a0,36(sp)` in each loop's
 * preheader): those two walks read it through a `CZSealBody *const *` view of the home -- an
 * expression of its own, which CSE does not merge with the entry's load while loop-invariant motion
 * still hoists it, and the address folds into the load (round 31; a const view, `void **` and a
 * cast through `unsigned int *` all match; an `int *` view does not: 2). Written plainly MW keeps
 * the entry's load through all three walks (0x140 bytes); through a reference local the reloads are
 * the target's but through an address register (`addiu a3,sp,36`); `opt_common_subs off` reloads in
 * the first walk too; a `volatile` view folds like the plain read */
void CSealUnit::Revive(CZSealBody *seal)
{
    if (!seal->m_isAlive)
        return;
    for (std::list<CZSealBody *>::iterator e = m_dead_guys.end(), i = m_dead_guys.begin(); i != e; ++i)
        if (*i == seal) {
            m_dead_guys.erase(i);
            if ((seal->m_team & 0x20000) && m_unit_num == 17)
                m_escortees.push_back(seal);
            else
                m_seals.push_back(seal);
            return;
        }
    for (std::list<CZSealBody *>::iterator e = m_seals.end(), i = m_seals.begin(); i != e; ++i)
        if (*i == *(CZSealBody *const *)&seal)
            return;
    for (std::list<CZSealBody *>::iterator e = m_escortees.end(), i = m_escortees.begin(); i != e; ++i)
        if (*i == *(CZSealBody *const *)&seal)
            return;
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
CSealUnit::CSealUnit(unsigned int team) : m_8(FORMATION_1), m_unit_num(team)
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

/* (OVL2 0x5E8CA0; SOCOM 1's, the AI's follow state (9) read off its stack's kinds where SOCOM 1
 * called FindAiState) */
void CSealUnit::CommandNotifyFromChild(CSealUnit *child, CZSealBody *seal, FT_COMMAND cmd, void *data)
{
    switch (cmd) {
    case 4:
        bool all = true;
        bool some = false;
        slist::iterator e = m_seals.end();

        for (slist::iterator i = m_seals.begin(); i != e; ++i) {
            CZSealBody *s = *i;

            if (s->IsAi()) {
                if (((CUnitSealCtrl *)s->m_control)->GetSealCtrlAi()->m_stateStack.m_10 & (1 << 9))
                    some = true;
                else
                    all = false;
            }
        }
        ulist::iterator ue = m_sub_units.end();

        for (ulist::iterator u = m_sub_units.begin(); u != ue; ++u) {
            CSealUnit *unit = *u;

            if (*unit->m_commands.begin() == 4) {
                if (unit != child)
                    some = true;
            } else
                all = false;
        }
        if (all) {
            some = false;
            m_a0 = 1;
        }
        if (some)
            m_a1 = 1;
        break;
    }
}

/* (OVL2 0x5E8A90: recursive, inlined into itself four times -- SOCOM 1's three -- each copy cutting
 * front() and empty() a level shorter: include/list's accessors, the pieces 0x5E8C40-0x5E8C90 in its
 * tail, empty() and sz() shared with GetTail's 0x5E8A60/0x5E8A80) */
CZSealBody *CSealUnit::GetLeader() const
{
    if (!m_seals.empty())
        return m_seals.front();
    if (!m_sub_units.empty())
        return m_sub_units.front()->GetLeader();
    return 0;
}

/* (OVL2 0x5E8850: recursive, inlined into itself twice; its pieces 0x5E8A40-0x5E8A80 follow it). The
 * squad under it taken into a local first: called with back() as its argument, MW expands that
 * back() a level deeper than the copy's other calls (the target's d1 calls the base's back()) */
CZSealBody *CSealUnit::GetTail()
{
    CZSealBody *t = 0;

    if (!m_sub_units.empty()) {
        CSealUnit *unit = m_sub_units.back();

        t = unit->GetTail();
    }
    if (!t && !m_seals.empty())
        t = m_seals.back();
    if (!t && !m_escortees.empty())
        t = m_escortees.back();
    if (!t && !m_dead_guys.empty())
        t = m_dead_guys.back();
    return t;
}

/* (OVL2 0x5EAA70) */
bool CSealUnit::GetAiModes(signed char *a, signed char *b)
{
    *a = 0;
    *b = 0;
    bool found = false;

    if (m_seals.size() != 0) {
        slist::iterator e = m_seals.end();

        for (slist::iterator i = m_seals.begin(); i != e; ++i) {
            CSealCtrlAi *ai = ((CUnitSealCtrl *)(*i)->m_control)->GetSealCtrlAi();

            if (ai) {
                found = true;
                *a = ai->m_26c;
                *b = ai->m_26d;
                break;
            }
        }
    }
    if (m_sub_units.size() != 0 && !found)
        for (ulist::iterator u = m_sub_units.begin(); u != m_sub_units.end(); ++u) {
            signed char ta, tb;

            /* the first byte tested through an unsigned view (the target's lbu, then lb for the
             * copy; `(bool)ta`, `ta != 0` and an unsigned cast all test the lb load) -- a stand-in
             * spelling */
            (*u)->GetAiModes(&ta, &tb);
            if (*(unsigned char *)&ta) {
                found = true;
                *a = ta;
                *b = tb;
            }
        }
    return found;
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

/* the mission as far as the squads read it: its AI parameters' fireteam command (SOCOM 1's
 * CMission::m_ai_params at 0xB4, AI_PARAMS::fireteam_command at +4 -- the byte SOCOM 1's
 * CreateSinglePlayerUnits reads; SOCOM II's at 0x148, read by this unit alone). fts_state.h's
 * CMission, which this unit does not include, leaves it unnamed: the view ours */
struct CMissionFireteamView {
    unsigned char _0[0x148];
    FT_COMMAND m_fireteam_command;   // 0x148
};
extern CMissionFireteamView theMission;

/* a SEAL's look out of a direction, in the XZ plane: the squad's own when m_a4 is set; following
 * the player, squad 3's leader turned 45 degrees one way and the others 45 the other, the leader
 * of any other squad kept straight; a pair (two SEALs) splits 60 degrees each way; else the
 * direction flattened (OVL2 0x5EEF80; SOCOM 1's GetMyLookDir, its front() a call there) */
void CSealUnit::GetMyLookDir(CPnt3D *out, CPnt3D *dir, CZSealBody *seal)
{
    if (m_a4) {
        *out = m_a8;
        return;
    }
    *out = *dir;
    if (m_follow_source == ftsGetPlayer()) {
        if (m_unit_num == 3) {
            if (m_seals.front() == seal) {
                out->x = 0.7071f * dir->x - 0.7071f * dir->z;
                out->y = 0.0f;
                out->z = 0.7071f * dir->x + 0.7071f * dir->z;
            } else {
                out->x = -dir->z;
                out->y = 0.0f;
                out->z = dir->x;
            }
        } else if (m_seals.front() != seal) {
            out->x = 0.7071f * dir->x + 0.7071f * dir->z;
            out->y = 0.0f;
            out->z = -0.7071f * dir->x + 0.7071f * dir->z;
        }
    } else if (m_seals.size() == 2) {
        if (m_seals.front() == seal) {
            out->x = 0.5f * dir->x - 0.866f * dir->z;
            out->y = 0.0f;
            out->z = 0.866f * dir->x + 0.5f * dir->z;
        } else {
            out->x = 0.5f * dir->x + 0.866f * dir->z;
            out->y = 0.0f;
            out->z = -0.866f * dir->x + 0.5f * dir->z;
        }
    } else {
        out->x = dir->x;
        out->y = 0.0f;
        out->z = dir->z;
    }
}

/* the AI SEAL squad 17 holds in the rescued state within 50 cells of `seal`, while the escortees'
 * extraction anim can run; else 0 (OVL2 0x5EEDB0, new in SOCOM II: IssueCommand and AllowCommand
 * ask it -- the name ours). GetUnitByTeam, can_run_anim and IsAi inlined, the state test the
 * controller's AiStateIs (the stack's const GetState called) */
CZSealBody *CSealUnit::FindRescuedNear(CZSealBody *seal)
{
    if (seal && can_run_anim("escortee_extract")) {
        CSealUnit *unit = GetUnitByTeam(17);

        if (unit) {
            /* the end taken first: the walk's iterator numbered after it, s0 to its s1 */
            slist::iterator e = unit->m_seals.end();

            for (slist::iterator i = unit->m_seals.begin(); i != e; ++i) {
                CZSealBody *s = *i;

                if (s && s->IsAi() && ((CUnitSealCtrl *)s->m_control)->GetSealCtrlAi()->AiStateIs(AI_STATE_RESCUED)
                    && (int)AiMaps.GetLocationDelta(seal->m_mapLocValid, s->m_mapLocValid) < 50)
                    return s;
            }
        }
    }
    return 0;
}


extern "C" {
/* a squad sound by number with a chance (0x5EAF20) and the squad's formation set (0x5EB6D0) */
bool func_005EAF20(CSealUnit *unit, int sound, bool b, float chance);
/* (the sound's answer and its bool argument: the two command voicings use the one and pass the
 * other unextended), and a command that takes an item (seal_cfg.cpp's 0x58DFA0, the SEALs': kinds 2, 9-13 and 16) */
bool func_0058DFA0(unsigned char item);
/* a command's and an item's names (seal_cfg.cpp's 0x58EF80 -- "CMD_" and the name, the prefix skipped
 * here -- and 0x58EB00, the SEALs') */
const char *func_0058EF80(unsigned char cmd);
const char *func_0058EB00(unsigned char item);
}

/* whether the squads answer orders over the radio (SOCOM 1's ShouldConfirmVoiceCommands, gp-30916
 * there: its ConfirmCommand reads it first; SOCOM II's OVL1 data 0x3DF1D0, which UIStoreOptions sets
 * from the ORDER_RESPONSE_CONTROLLER valve -- ConfirmCommand and AcknowledgeCommand read it first) */
extern bool ShouldConfirmVoiceCommands;

/* (OVL2 0x5EB580: the walks' ends taken into locals first -- the end before the iterator in each:
 * the iterators s0/s4, the ends s1/s5, as the target's; tested against end() the two swap) */
CZSealBody *CSealUnit::FollowSeal(CZSealBody *seal)
{
    m_follow_source = seal;
    m_a2 = 1;
    bool escort = !m_escortees.empty();

    slist::iterator end = m_seals.end();

    for (slist::iterator i = m_seals.begin(); i != end; ++i) {
        if (*i != seal)
            (*i)->IssueCommand(seal, (FT_COMMAND)4, 0, true);
        if (escort) {
            for (slist::iterator ee = m_escortees.end(), e = m_escortees.begin(); e != ee; ++e)
                (*e)->IssueCommand(seal, (FT_COMMAND)4, 0, true);
            escort = false;
        }
    }
    if (escort)
        func_005E7830(this);
    ulist::iterator uend = m_sub_units.end();

    for (ulist::iterator u = m_sub_units.begin(); u != uend; ++u)
        (*u)->IssueCommand(seal, (FT_COMMAND)4, 0, true);
    return seal;
}

/* an AI mode as GetAiModes hands it back (zseal.h's AI_MODE, a byte enum: loaded with lb, widened with
 * andi for sprintf) */
enum UNIT_AI_MODE { UNIT_AI_MODE_0 = 0, UNIT_AI_MODE_LAST = 255 };

/* (OVL2 0x5EABA0): the moves (1, 14, 16, 19) sound 55, the item commands (9, 10, 12) sound 12; any
 * other its own sound, "SND_<mode><mode>_<command>" with the item's name after it when there is one
 * (commands 11, 13 and 23 never name one), played at full volume. The result one bool, preset to the
 * option's negation, as AcknowledgeCommand's; the two modes copied out of GetAiModes' bytes into
 * byte-enum locals (lb, widened with andi for sprintf) declared first and assigned second-then-first
 * -- the target's load order with its registers; initialised in place they load first-then-second */
bool CSealUnit::ConfirmCommand(FT_COMMAND cmd, unsigned char item, int flags)
{
    bool r = !ShouldConfirmVoiceCommands;
    bool first = m_unit_num == 1;

    if (!r) {
        switch (cmd) {
        case 1:
        case 16:
        case 19:
        case 14:
            r = func_005EAF20(this, 55, first, 1.0f);
            break;
        case 10:
        case 9:
        case 12:
            r = func_005EAF20(this, 12, first, 1.0f);
            break;
        case 13:
        case 11:
        case 23:
            item = 0;
        default: {
            UNIT_AI_MODE a, b;
            char name[64];

            UNIT_AI_MODE ma, mb;

            GetAiModes((signed char *)&a, (signed char *)&b);
            mb = b;
            ma = a;
            const char *what = func_0058EF80(cmd) + 4;

            if (item == 0)
                sprintf(name, "SND_%d%d_%s", mb, ma, what);
            else
                sprintf(name, "SND_%d%d_%s%s", mb, ma, what, func_0058EB00(item));
            CSnd *snd = CSnd::GetSoundByName(name);

            if (snd) {
                snd->PlaySoundVol(1.0f, 0, 255);
                r = true;
            }
        }
        }
    }
    return r;
}

/* (OVL2 0x5EAD70): none asked -- or an item command with no item -- sound 84; the moves (3-6, 17)
 * 55; any other 56; the squad numbered 1 says so (the sound's third argument, a bool: an int
 * parameter adds an andi). One result, preset to the option's negation: the early answer is that
 * value narrowed (`andi v0,a2,0xff`), each sound's bool narrowed into it; the byte tested `== 0`
 * (re-extended before the branch, as the target; `!item` branches on the register) */
bool CSealUnit::AcknowledgeCommand(FT_COMMAND cmd, unsigned char item, int flags)
{
    bool r = !ShouldConfirmVoiceCommands;
    bool first = m_unit_num == 1;

    if (!r) {
        if (cmd == CMD_UNKNOWN)
            r = func_005EAF20(this, 84, first, 1.0f);
        else if (func_0058DFA0(cmd) && item == 0)
            r = func_005EAF20(this, 84, first, 1.0f);
        else {
            switch (cmd) {
            case 3:
            case 4:
            case 5:
            case 6:
            case 17:
                r = func_005EAF20(this, 55, first, 1.0f);
                break;
            default:
                r = func_005EAF20(this, 56, first, 1.0f);
                break;
            }
        }
    }
    return r;
}

/* the controller's current state if it is a breach (kind 10), else 0 (inline: 0x5EBB50 twice) */
static inline CAiSBreach *GetBreach(CSealCtrlAi *ai)
{
    CAiState *s = ai->GetAiState();

    return s && s->m_state == AI_STATE_BREACH ? (CAiSBreach *)s : 0;
}

/* the bits a breach clears as it is pushed, each through a bool (the target's `andi zero,1`) */
static inline void SetAiBit16(CSealCtrlAi *ai, bool b) { ai->m_aibit16 = b; }
static inline void SetAiBit17(CSealCtrlAi *ai, bool b) { ai->m_aibit17 = b; }

/* a breach pushed on a SEAL's AI and made the squad's prototype (the squad's data pointer: the
 * CSealCtrlAi::OnCommand caller passes it): the state copied whole (CAiSBreach's implicit
 * operator=, inlined a level -- CAiState's 0x5EBDC0 and CAiTimer's 0x5EBDA0 called), the first
 * SEAL made the team's first and last and its valve made, or a later SEAL made the last, its
 * place in the team counted and the valve stepped (OVL2 0x5EBB50; new in SOCOM II, the name ours) */
extern "C" bool func_005EBB50(CAiSBreach *proto, CSealCtrlAi *ai)
{
    bool r = ai->PushAiState(AI_STATE_BREACH, 0);

    if (r) {
    CAiSBreach *s = GetBreach(ai);

    *s = *proto;
    if (s->m_64 == 0) {
        s->m_64 = s->m_68 = ai;
        proto->m_64 = proto->m_68 = ai;
        s->m_70 = CValve::Create(0, 0, VALVE_TEMP);
        proto->m_70 = s->m_70;
        s->m_6c = 0;
        proto->m_6c = 0;
    } else if (s->m_64 == s->m_68) {
        s->m_68 = ai;
        proto->m_68 = ai;
        CAiSBreach *first = GetBreach(s->m_64);

        if (first)
            first->m_68 = ai;
    }
    s->m_6c = proto->m_6c++;
    CValve *v = s->m_70;

    v->SetValue(v->m_value + 1);
    s->SetBit13(ai->m_aibit16 && !ai->m_aibit17);
    SetAiBit16(ai, false);
    SetAiBit17(ai, false);
    s->SetActive(true);
    r = true;
    }
    return r;
}

/* a follower's place in a formation, in map cells (two ints built by a constructor, so the tables
 * below are made on the first call; the name ours) */
struct UnitCellOffset {
    UnitCellOffset(int x, int z) : m_x(x), m_z(z) {}

    int m_x;
    int m_z;
};

/* the first cell of the single file (OVL2 data 0x650B00, holding 1; read only by 0x5EB910 -- the
 * name ours) */
static int s_fileStart = 1;

/* the cell offset of a formation's slot (OVL2 0x5EB910: CAiSFollow::SetFormation's, the follower's
 * m_5c; new in SOCOM II, the name ours): formation 2 a single file straight back from the first cell,
 * 1 and any other a staggered column */
extern "C" void func_005EB910(UNIT_FORMATION formation, int slot, int *cells)
{
    static UnitCellOffset stagger[8] = {
        UnitCellOffset(2, 2), UnitCellOffset(3, -2), UnitCellOffset(4, -3), UnitCellOffset(5, -2),
        UnitCellOffset(6, 3), UnitCellOffset(7, 5), UnitCellOffset(8, -4), UnitCellOffset(9, -5),
    };
    static UnitCellOffset file[8] = {
        UnitCellOffset(s_fileStart, 0), UnitCellOffset(s_fileStart + 1, 0),
        UnitCellOffset(s_fileStart + 2, 0), UnitCellOffset(s_fileStart + 3, 0),
        UnitCellOffset(s_fileStart + 4, 0), UnitCellOffset(s_fileStart + 5, 0),
        UnitCellOffset(s_fileStart + 6, 0), UnitCellOffset(s_fileStart + 7, 0),
    };

    switch (formation) {
    case FORMATION_2:
        cells[0] = file[slot].m_x;
        cells[1] = file[slot].m_z;
        break;
    case FORMATION_1:
        cells[0] = stagger[slot].m_x;
        cells[1] = stagger[slot].m_z;
        break;
    default:
        cells[0] = stagger[slot].m_x;
        cells[1] = stagger[slot].m_z;
        break;
    }
}

/* the formation set to the leader's: 2 inside, else 1, when it differs (OVL2 0x5EB8A0; the
 * formation byte and the name ours) */
extern "C" void func_005EB8A0(CSealUnit *unit)
{
    CZSealBody *leader = unit->GetLeader();

    if (leader) {
        UNIT_FORMATION f;

        if (leader->m_inside ? true : false)
            f = FORMATION_2;
        else
            f = FORMATION_1;
        if (f != unit->m_8)
            unit->SetFormation(f, 0);
    }
}

/* the controller's current state if it is a follow (kind 9), else 0 (GetBreachState's twin, inlined
 * here; the name ours) */
static inline CAiSFollow *GetFollowState(CSealCtrlAi *ai)
{
    CAiState *s = ai->GetAiState();

    return s && s->m_state == 9 ? (CAiSFollow *)s : 0;
}

/* a formation given the squad: each AI SEAL that follows takes its slot in it, counted on from
 * `index` -- the SEALs in order, the escortees after the first of them (seen to at once when the
 * squad has no SEAL, 0x5E7830), then the squads under it in turn; the formation kept at 8 and the
 * flag at 0xA2 cleared; the next slot returned (OVL2 0x5EB6D0, new in SOCOM II: the leader asked for
 * and not used; the name ours -- SOCOM 1's SetFormation took a SEAL and a point). Pinned: the
 * target's callers and its own recursion call it. The formation goes to CAiSFollow::SetFormation
 * unconverted: both take UNIT_FORMATION (round 31: with CAiSFollow's as a signed char MW
 * sign-extends it into a register of its own, 0x1E0; with this one's 0x5EB8A0 does).
 * OPEN, 6 instructions, one placement: the escortees' end taken first (numbered before the SEALs'
 * walk: s6, the target's) is computed at the top, the target's after the walk's guard. Round 43
 * (tools/igreplay.py --target): the squads' walk wanted its end numbered below the iterator --
 * `e = end(), u = begin()` in the for (12 -> 6); and the target's escortees' end is numbered first
 * (f2 < the SEALs' e < i < j: s6, s5, s4, s3) yet placed after the guard. Declared in the inner for
 * (`f2 = end(), j = begin()`) MW hoists it to the guard's far side, the target's place, but numbers it
 * after the SEALs' walk (9: f2 s4, e s6, i s5); declared uninitialised at the top and assigned in the
 * body or the `if (esc)` block the iterator goes through memory (0x1EC, 0x1E4); no f2 at all, 16.
 * Tried before: the three forms of each walk (27: best 14, the SEALs' end first), the escortees' end
 * declared at the top (12) or in the loop body (0x1E0), reassigned in the body (0x1EC), the list
 * through a reference (46), the leader kept, `esc` assigned later or by a ternary, postfix steps */
#pragma auto_inline off
int CSealUnit::SetFormation(UNIT_FORMATION f, int index)
{
    GetLeader();
    slist::iterator f2 = m_escortees.end();
    bool esc = !m_escortees.empty();

    for (slist::iterator e = m_seals.end(), i = m_seals.begin(); i != e; ++i) {
        CSealCtrlAi *ai = ((CUnitSealCtrl *)((CEntity *)*i)->m_control)->GetSealCtrlAi();

        if (ai) {
            CAiSFollow *fs = GetFollowState(ai);

            if (fs)
                fs->SetFormation(f, index++);
        }
        if (esc) {
            for (slist::iterator j = m_escortees.begin(); j != f2; ++j) {
                CSealCtrlAi *ai = ((CUnitSealCtrl *)((CEntity *)*j)->m_control)->GetSealCtrlAi();

                if (ai) {
                    CAiSFollow *fs = GetFollowState(ai);

                    if (fs)
                        fs->SetFormation(f, index++);
                }
            }
            esc = false;
        }
    }
    if (esc)
        func_005E7830(this);
    for (ulist::iterator e = m_sub_units.end(), u = m_sub_units.begin(); u != e; ++u)
        index = (*u)->SetFormation(f, index);
    m_a2 = 0;
    m_8 = f;
    return index;
}
#pragma auto_inline reset

/* (OVL2 0x5E76D0; pinned: -inline auto takes it, and the target's recursion is a call) */
#pragma auto_inline off
/* a point scaled, given back by value (inline here: SOCOM II's CPnt3D::operator*(float) const,
 * whose copies zmath.h declares out of line -- the result copied into the caller's temporary) */
static inline CPnt3D Scaled(const CPnt3D &p, float k)
{
    CPnt3D r;

    p.Scale(k, &r);
    return r;
}

/* (OVL2 0x5E7030) */
float CSealUnit::GetCenter(CPnt3D *out, int depth, bool ai)
{
    float n = 0.0f;
    std::list<CZSealBody *>::iterator e = m_seals.end();
    std::list<CZSealBody *>::iterator i;

    *out = CPnt3D::zero;
    for (i = m_seals.begin(); i != e; ++i) {
        CZSealBody *seal = *i;

        if (!ai || seal->IsAi()) {
            out->Add(&seal->m_posW, out);
            n += 1.0f;
        }
    }
    if (depth > 0) {
        std::list<CSealUnit *>::iterator u;

        for (u = m_sub_units.begin(); u != m_sub_units.end(); ++u) {
            CPnt3D c;
            float k = (*u)->GetCenter(&c, depth - 1, ai);

            *out += Scaled(c, k);
            n += k;
        }
    }
    if (n > 0.0f)
        out->Scale(1.0f / n, out);
    return n;
}

/* (OVL2 0x5E7230: the walks' iterators assigned, in memory -- two copies each) */
float CSealUnit::GetMaxDistSq(const CPnt3D *pos, int depth)
{
    float max = 0.0f;
    std::list<CZSealBody *>::iterator e = m_seals.end();
    std::list<CZSealBody *>::iterator i;

    for (i = m_seals.begin(); i != e; ++i) {
        CPnt3D d;

        pos->Sub(&(*i)->m_posW, &d);
        float l = d.x * d.x + d.y * d.y + d.z * d.z;

        if (l > max)
            max = l;
    }
    if (depth > 0) {
        std::list<CSealUnit *>::iterator u;

        for (u = m_sub_units.begin(); u != m_sub_units.end(); ++u) {
            float l = (*u)->GetMaxDistSq(pos, depth - 1);

            if (l > max)
                max = l;
        }
    }
    return max;
}

int CSealUnit::GetAwareCounter() const
{
    int n = 0;
    slist::const_iterator i;
    ulist::const_iterator u;

    for (i = m_seals.begin(); i != m_seals.end(); ++i)
        n += ((CAwareCtrl *)(*i)->m_control)->m_aware;
    for (u = m_sub_units.begin(); u != m_sub_units.end(); ++u)
        n += (*u)->GetAwareCounter();
    return n;
}
#pragma auto_inline reset

/* a random draw in [min, min + range) (SOCOM 1's Rfloat's operator float without its zero-range test,
 * the 1.0 range through the parameter so its multiply stays -- seal_aifollow.cpp's FollowRand), and
 * whether it falls at or under a chance whose address is taken (the target stores the chance after
 * the 1.0 test, in the draw's block, and keeps its address across rand()) */
static inline float UnitRand(float min, float range)
{
    float r = 4.656613e-10f * rand();

    return min + range * r;
}

static inline bool UnitChance(float chance) { return UnitRand(0.0f, 1.0f) <= *(const float *)&chance; }

/* a random index up to n (0 when n is) */
static inline int UnitPick(int n) { return n == 0 ? 0 : rand() % (n + 1); }

/* the squad's voice (OVL2 0x5EAF20; new in SOCOM II, the name ours): with the chance (1 always),
 * a random SEAL of the squad and the squads under it when `pick`, then the squad's own SEALs in
 * turn, then each squad under it -- the first AI SEAL alive that plays the sound (its controller's
 * PlayAiSound at full chance, no event). Whether one did.
 * OPEN, 0x268 against 0x270, two places, the rest (registers, the walks, the hoisted ends) the
 * target's: (1) the chance -- the target compares $f12 with 1.0, then in the draw's block stores it
 * and keeps its address in s4 across rand() (`swc1 $f12,124(sp); jal rand; addiu s4,sp,124` ...
 * `lwc1 $f0,0(s4)`); the inline's by-value parameter read through `*(const float *)&` gives the
 * store in that block but reloads sp-relative; a reference or pointer inline, a pointer local (a
 * cast or not, before the draw or across a `?:`), SOCOM 1's Rfloat with its range test, a two-level
 * pointer inline all keep the chance in $f20 (0x27C) or reload it sp-relative (0x268); (2) the
 * random index -- the target's zero arm is an empty block (`bnez; move a1,zero; b join; nop`, then an
 * alignment nop before the rand() block), ours fills the b's delay slot with the join's `move a0,s2`:
 * a ternary in or out of an inline, if/else in or out of an inline, early returns, `!n`, `n != 0`, the
 * round-44 self arm `i = n == 0 ? i : ...`, an empty then-branch all fill it */
extern "C" bool func_005EAF20(CSealUnit *unit, int sound, bool pick, float chance)
{
    bool played = false;

    if (chance == 1.0f || UnitChance(chance)) {
        if (pick) {
            int n = unit->GetSealCount(64) - 1;
            CZSealBody *seal = unit->GetSealByIndex(UnitPick(n), 64);

            if (seal && seal->IsAi() && seal->m_isAlive)
                played = ((CUnitSealCtrl *)((CUnitSealCtrl *)seal->m_control)->GetSealCtrlAi())
                             ->PlayAiSound(sound, 1.0f, -1);
        }
        if (!played) {
            for (CSealUnit::slist::iterator i = unit->m_seals.begin(); !played && i != unit->m_seals.end(); ++i) {
                CZSealBody *seal = *i;

                if (seal->IsAi() && seal->m_isAlive)
                    played = ((CUnitSealCtrl *)((CUnitSealCtrl *)seal->m_control)->GetSealCtrlAi())
                                 ->PlayAiSound(sound, 1.0f, -1);
            }
            if (!played)
                for (CSealUnit::ulist::iterator u = unit->m_sub_units.begin(); !played && u != unit->m_sub_units.end(); ++u)
                    played = func_005EAF20(*u, sound, false, 1.0f);
        }
    }
    return played;
}

/* sound 79 when the squad is aware, else 78 (OVL2 0x5EAEB0) */
extern "C" void func_005EAEB0(CSealUnit *unit)
{
    if (unit->GetAwareCounter())
        func_005EAF20(unit, 79, 0, 1.0f);
    else
        func_005EAF20(unit, 78, 0, 1.0f);
}

/* (OVL2 0x5ED7B0; the compare chain tests 2, 1, 0, 4, 3: the cases in reverse) */
void CSealUnit::Notify(CZSealBody *seal, UNIT_MESSAGE msg, void *data)
{
    switch (msg) {
    case UNIT_MSG_3:
    case UNIT_MSG_4:
    case UNIT_MSG_0:
        (m_parent ? m_parent->GetTop() : this)->DistributeNotify(seal, msg, data);
        break;
    case UNIT_MSG_1:
    case UNIT_MSG_2:
        PrivateNotify(seal, msg, data);
        break;
    }
}

/* (OVL2 0x5ED4F0: SOCOM 1's DistributeNotify -- recursive, inlined into itself twice, its iterator
 * pieces 0x5ED6E0-0x5ED7A0 after it). OPEN, 14 instructions, the right size: GetSealCount's question
 * (include/list's depths, the MSL lane's). The outer copy is the target's (the size word, begin and end
 * in s0/s7, the walk raw); the first self-copy too but for its begin(): the target calls the base's
 * begin() (0x510100, by hidden pointer into a frame slot) and then the wrapper iterator's constructor
 * from that base iterator by const reference (0x5ED7A0: `lw v1,0(a1)`), where include/list inlines
 * the base's begin() and calls the base iterator's constructor from the node -- the wrapper's begin()
 * one level deeper, `iterator(base::begin())` (the round-15 measurement: the same shape gives
 * GetSealCount its calls); its end() is one call (0x5ED750, `&end_` straight), its != the base's
 * (0x5ED6E0), * and i++ out of line (0x5ED790; 0x5ED720 copy, 0x5ED700 prefix ++, 0x5ED720). The
 * innermost copy calls empty() (0x5E8A60), begin 0x5ED780, end 0x5ED750, the wrapper's != 0x5ED730,
 * * 0x5ED790 and i++ 0x5ED760 -- its shape `!m_sub_units.empty()` with the walk below, as written */
void CSealUnit::DistributeNotify(CZSealBody *seal, UNIT_MESSAGE msg, void *data)
{
    if (!m_sub_units.empty())
        for (ulist::iterator i = m_sub_units.begin(); i != m_sub_units.end(); i++)
            (*i)->DistributeNotify(seal, msg, data);
    PrivateNotify(seal, msg, data);
}

/* the AI SEAL may point out what it sees: bit 16 set, 17 clear (an inline bool -- the target
 * materialises the `&&` before its branch; the name ours) */
static inline bool PointsOut(CSealCtrlAi *ai) { return ai->m_aibit16 && !ai->m_aibit17; }

/* (OVL2 0x5ED140: SOCOM 1's PrivateNotify -- the compare chain tests 0, 4, 2, 1, 3: the cases in
 * reverse; SOCOM 1's walks of the squad on 0 and 3 gone into ShareTarget) */
void CSealUnit::PrivateNotify(CZSealBody *seal, UNIT_MESSAGE msg, void *data)
{
    switch (msg) {
    case UNIT_MSG_3:
        if (m_a4) {
            m_a4 = 0;
            IssueCommand(0, (FT_COMMAND)3, 0, true);
        }
        break;
    case UNIT_MSG_1:
        m_a3 = 1;
        break;
    case UNIT_MSG_2:
        m_a0 = 1;
        break;
    case UNIT_MSG_4:
        if (data) {
            ClearAiStates(true, false);
            IssueCommand(seal, (FT_COMMAND)3, 0, true);
            ShareTarget(seal, (CEntity *)data, true, 0);
            {
                CAiSPursue pursue;

                pursue.m_timer = -1.0f;
                PushAiState(AI_STATE_PURSUE, &pursue, false);
            }
        }
        break;
    case UNIT_MSG_0:
        if (data) {
            int idx = ShareTarget(seal, (CEntity *)data, false, 0);

            if (idx != -1) {
                CSealCtrlAi *ai = ((CUnitSealCtrl *)seal->m_control)->GetSealCtrlAi();

                if (PointsOut(ai)) {
                    UnitPointData *p = idx == 0 ? &m_leader_point_data : &m_follower_point_data;
                    float d;

                    p->start_anim = 1;
                    p->start_turn = 1;
                    p->in_point = 1;
                    p->timer = 10.0f;
                    d = &seal->m_posW - &((CEntity *)data)->m_posW;
                    p->dirW.x = d;
                    p->dirW.y = d;
                    p->dirW.z = d;
                    p->dirW.Normalize();
                    m_point_ticks = 2;
                }
            }
        }
        break;
    }
}

/* the AI's target (ai_target.cpp's CAiTarget: only its entity here), the test the AI's code makes
 * of it (OVL2 0x60A850: an entity, alive, and an AI's unless it holds bit 8 or 9), its sighting
 * clock (0x60A590) and its setter (0x60A5F0) */
class CAiTarget {
public:
    float GetTimeSinceLastSighting() const;
    void SetTarget(CTarget *t);

    CEntity *m_ent;   // 0x0
};

extern "C" bool func_0060A850(const CAiTarget *t);

/* the AI controller as a squad hands it a target (zseal.h's CSealCtrlAi): its target at 0x67C, the
 * flags word at 0x6A8 (bit 9 lets a held target interrupt) and the virtual SetTarget (+0xB0) -- a
 * view, the 42 slots before it unnamed here */
class CUnitAiView {
public:
    virtual void v08(); virtual void v0c(); virtual void v10(); virtual void v14(); virtual void v18();
    virtual void v1c(); virtual void v20(); virtual void v24(); virtual void v28(); virtual void v2c();
    virtual void v30(); virtual void v34(); virtual void v38(); virtual void v3c(); virtual void v40();
    virtual void v44(); virtual void v48(); virtual void v4c(); virtual void v50(); virtual void v54();
    virtual void v58(); virtual void v5c(); virtual void v60(); virtual void v64(); virtual void v68();
    virtual void v6c(); virtual void v70(); virtual void v74(); virtual void v78(); virtual void v7c();
    virtual void v80(); virtual void v84(); virtual void v88(); virtual void v8c(); virtual void v90();
    virtual void v94(); virtual void v98(); virtual void v9c(); virtual void va0(); virtual void va4();
    virtual void va8(); virtual void vac();
    virtual void SetTarget(CTarget *t);   /* +0xB0 */

    unsigned char _4[0x67c - 4];
    CAiTarget m_target;   // 0x67c
    unsigned char _680[0x6a8 - 0x680];
    unsigned int m_6a8;   // 0x6a8
};

/* a held target that may interrupt (an inline bool, seal_aivehicle.cpp's shape: the `&&`
 * materialised) */
static inline bool TargetEngaged(CUnitAiView *ai)
{
    return func_0060A850(&ai->m_target) && (ai->m_6a8 & 0x200);
}

/* a target handed to the squad's AI SEALs that know it: each one's entry for it marked (bit 8 of
 * the target's flags), the AI's own target refreshed when it is the one, else given it when the AI
 * has none worth keeping or `force` says so; the SEAL's place in the squad returned, -1 when it is
 * not in it (OVL2 0x5ECF90, new in SOCOM II: the squad's private news 0x5ED140 calls it twice,
 * passing a fifth argument, 0, that it never reads; the name ours). The loop's locals numbered by
 * dead first definitions -- the target, the lost flag, then the AI -- and the lost test written
 * out (seal_aivehicle.cpp's TargetLost as an inline numbers its flag after the AI and the target:
 * the three trade s1-s3); the AI's own target tested `!=` with its refresh in the else (`==`
 * lays the refresh out first: 8 bytes short) */
int CSealUnit::ShareTarget(CZSealBody *seal, CEntity *target, bool force, int unused)
{
    int idx = -1;
    int n = 0;

    for (slist::iterator i = m_seals.begin(); i != m_seals.end(); ++i, n++) {
        CZSealBody *s = *i;
        CTarget *t = 0;
        bool lost = false;
        CUnitAiView *ai = (CUnitAiView *)((CUnitSealCtrl *)((CEntity *)s)->m_control)->GetSealCtrlAi();

        if (!ai)
            continue;
        t = ((CEntity *)s)->m_targets.FindTarget(target);

        if (t) {
            t->m_flags |= 0x100;
            if (ai->m_target.m_ent != target) {
                lost = true;
                if (func_0060A850(&ai->m_target)) {
                    if (!TargetEngaged(ai))
                        lost = ai->m_target.GetTimeSinceLastSighting() > 3.0f;
                    else
                        lost = false;
                }
                if (lost || force)
                    ai->SetTarget(t);
            } else
                ai->m_target.SetTarget(t);
        }
        if (s == seal)
            idx = n;
    }
    return idx;
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

/* each squad's update run (an inline both creators end with: the end taken once into a local, the
 * iterator assigned from begin() -- written in each creator, the iterator is numbered ahead of their
 * walks' temporaries and the end is re-built each pass; the name ours) */
static inline void UpdateUnits()
{
    std::list<CSealUnit *>::iterator e = CSealUnit::m_list.end();
    std::list<CSealUnit *>::iterator u;

    for (u = CSealUnit::m_list.begin(); u != e; ++u)
        func_005E7830(*u);
}

/* an entity that is a SEAL (kind 2), or none (seal.cpp's) */
static inline CZSealBody *AsSeal(CEntity *e)
{
    return e && e->m_type == CEntity::ENT_TYPE_SEAL ? (CZSealBody *)e : 0;
}

/* the network game's squads, once: 0 (the player's), 28 (the SEALs'), 17 (the terrorists'), 31,
 * 30, 26, 25 and 24 (the radio channel's); every SEAL into its side's squad by its team bits, the
 * player into squad 0, and each squad's update run (OVL2 0x5ECCD0; SOCOM 1's -- its squads made
 * inline, SOCOM II's through Create). The third arm's ftsGetPlayer() is the target's: a call whose
 * result nothing uses (compiled-out code around it, likely) */
void CSealUnit::CreateMultiPlayerUnits()
{
    if (!m_created_units_test) {
        CSealUnit *unit = Create(0);
        CSealUnit *seals = Create(28);
        CSealUnit *terrorists = Create(17);

        Create(31);
        Create(30);
        Create(26);
        Create(25);
        Create(24);
        m_created_units_test = true;

        CEntityList::const_iterator i;

        for (i = CEntity::GetEntityList().begin(); i != CEntity::GetEntityList().end(); ++i) {
            CZSealBody *seal = AsSeal(*i);

            if (seal) {
                if (seal->m_team & 0x10000000)
                    seals->SealJoinUnit(seal);
                else if (seal->m_team & 0x20000)
                    terrorists->SealJoinUnit(seal);
                else
                    ftsGetPlayer();
            }
        }
        unit->SealJoinUnit(ftsGetPlayer());
        UpdateUnits();
    }
}

/* a SEAL put in a squad: the one given, else its team's -- bit 3 squad 3, bit 2 squad 2, bit 28
 * squad 28, bit 17 squad 17, bit 18 squad 18, else the first of squads 5-15 its bits name, else
 * squad 4 (OVL2 0x5EC9F0; SOCOM 1's InitalizeSealUnit, GetUnitByTeam inlined at each). The team read
 * in place at each test: taken into a local it moves every temporary register */
void CSealUnit::InitalizeSealUnit(CZSealBody *seal, CSealUnit *unit)
{
    if (seal) {
        if (!unit) {
            if (seal->m_team & 8)
                unit = GetUnitByTeam(3);
            else if (seal->m_team & 4)
                unit = GetUnitByTeam(2);
            else if (seal->m_team & 0x10000000)
                unit = GetUnitByTeam(28);
            else if (seal->m_team & 0x20000)
                unit = GetUnitByTeam(17);
            else if (seal->m_team & 0x40000)
                unit = GetUnitByTeam(18);
            else {
                for (unsigned int i = 5; i < 16; i++)
                    if (1 << i & seal->m_team && !unit)
                        unit = GetUnitByTeam(i);
                if (!unit)
                    unit = GetUnitByTeam(4);
            }
        }
        if (unit)
            unit->SealJoinUnit(seal);
    }
}

/* the single-player squads: 1 (Alpha) with 2 and 3 (Bravo's fire teams) under it, 28, 17, 18, 4,
 * 29 and 5-15; every SEAL put in its squad; Alpha told the player's order 6, then the mission's
 * fireteam command -- 5 goes to squad 3 with the data 8 stored, squad 2 told 4 -- the formation set
 * to Alpha's own, and each squad's update run (OVL2 0x5EC6A0; SOCOM 1's, its squads made inline and
 * order 5's 8 passed as the data).
 * OPEN, 11 instructions: the made flag's store. The target loads m_list's first node and stores it to
 * the walk's temporary before it stores the flag; MW here stores the flag first (CreateMultiPlayerUnits,
 * the same store written before its entity walk, gets the target's order). Tried: the flag as 1, through
 * an inline setter, as UpdateUnits' argument, the walk written in place, through a reference, a
 * pointer and a const view of m_list, u declared first, the flag stored after begin() or end() inside
 * the walk's inline. What closed the rest: the order 5 test a one-case switch (`beq; b`), the data an
 * int local (its slot right under the walk's iterator: SetDataPtr's parameter lands lower), the
 * push_back one inline level deeper (AddSub: the inserts' temporaries under the walk's) */
void CSealUnit::CreateSinglePlayerUnits()
{
    CSealUnit *alpha = Create(1);
    CSealUnit *bravo1 = Create(2);
    CSealUnit *bravo2 = Create(3);

    Create(28);
    Create(17);
    Create(18);
    Create(4);
    Create(29);
    alpha->AddSubUnit(bravo1);
    alpha->AddSubUnit(bravo2);
    for (unsigned int n = 5; n < 16; n++)
        Create(n);

    CEntityList::const_iterator i;

    for (i = CEntity::GetEntityList().begin(); i != CEntity::GetEntityList().end(); ++i)
        InitalizeSealUnit(AsSeal(*i), 0);

    CZSealBody *player = ftsGetPlayer();

    alpha->IssueCommand(player, (FT_COMMAND)6, 0, true);

    FT_COMMAND cmd = (FT_COMMAND)theMission.m_fireteam_command;

    switch (cmd) {
    case 5: {
        int data = 8;

        bravo2->SetData(&data, sizeof data);
        bravo2->IssueCommand(player, cmd, (void *)1, true);
        bravo1->IssueCommand(player, (FT_COMMAND)4, 0, true);
        break;
    }
    default:
        alpha->IssueCommand(player, cmd, 0, true);
    }
    alpha->SetFormation(alpha->m_8, 0);
    m_created_units_test = true;
    UpdateUnits();
}

/* (OVL2 0x5E6C90; pinned: at fifteen statements -inline auto takes it, and the target's recursion
 * is a call, not the self-inlined copies MW makes of it) */
#pragma auto_inline off
/* (OVL2 0x5E6AB0, before ClearAiStates in the image) */
void CSealUnit::PushAiState(AI_STATE state, CAiState *proto, bool recurse)
{
    if (proto->Virtual44(state)) {
        std::list<CZSealBody *>::iterator e = m_seals.end();

        for (std::list<CZSealBody *>::iterator i = m_seals.begin(); i != e; ++i) {
            CAiState *s;
            CSealCtrlAi *ai = ((CUnitSealCtrl *)(*i)->m_control)->GetSealCtrlAi();

            if (ai && ai->PushAiState(state, 0)) {
                s = ai->GetAiState();

                if (proto->Virtual44(s->m_state)) {
                    s->m_flags = proto->m_flags;
                    s->m_phase = proto->m_phase;
                    s->m_nextPhase = proto->m_nextPhase;
                    s->m_delay = proto->m_delay;
                    s->m_time = proto->m_time;
                    s->m_flags = proto->m_flags;
                    s->m_bit3 = proto->m_bit3;
                    s->m_bit15 = proto->m_bit15;
                    s->Virtual48((int)ai, proto);
                }
            }
        }
        if (recurse)
            for (std::list<CSealUnit *>::iterator u = m_sub_units.begin(); u != m_sub_units.begin(); ++u)
                (*u)->PushAiState(state, proto, recurse);
    }
}

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

/* the network game (OVL1 bss 0x437CE8, hud_tcmcmds.cpp's CZNetGame): its byte at 0xD2 (the name
 * ours) */
struct CUnitNetGame {
    unsigned char _0[0xd2];
    unsigned char m_d2;   // 0xd2
};
extern CUnitNetGame *data_00437CE8;

extern "C" {
/* whether the game is online (OVL2 0x5750A0, seal_ai.cpp's: theNetwork's second byte) and the voice
 * chat put in a mode (OVL1 0x2C2C20, fts_net_persona.cpp's) */
int func_005750A0(void);
int func_002C2C20(int mode);
}

/* an order the network squads take as radio traffic, online only: squad 26 sends it to everyone and
 * plays it (PlayRemoteMsg inlined), squads 31 and 30 send it to their two sides, squad 24 switches
 * the voice chat -- the four channel orders (31-34) the modes 0-3, the first three kept at 0xC4,
 * the on/off order (35) mode 3 or the kept one -- whether the squad was one of them (OVL2 0x5E6E40,
 * new in SOCOM II: IssueCommand asks it first and stops when it says so; the name ours). An
 * if/else chain (a switch tests every number first); squad 26's id read through a const view, so
 * the inlined PlayRemoteMsg reads it afresh (`lw a0,4(s2)`) -- through the plain pointer MW keeps
 * the field's address in a saved register across the call (4 off) */
bool CSealUnit::RadioCommand(CZSealBody *seal, FT_COMMAND cmd)
{
    bool r = false;

    if (func_005750A0()) {
        if (m_unit_num == 26) {
            r = true;
            SendMessageMessage(((const CEntity *)seal)->m_id, R_SEND_TO_ALL, cmd);
            PlayRemoteMsg(seal, cmd, 0);
        } else if (m_unit_num == 31) {
            r = true;
            SendMessageMessage(((CEntity *)seal)->m_id, R_SEND_TO_3, cmd);
        } else if (m_unit_num == 30) {
            r = true;
            SendMessageMessage(((CEntity *)seal)->m_id, R_SEND_TO_2, cmd);
        } else if (m_unit_num == 24) {
            r = true;
            switch (cmd) {
            case 31:
                func_002C2C20(0);
                m_c4 = 0;
                break;
            case 32:
                func_002C2C20(1);
                m_c4 = 1;
                break;
            case 33:
                func_002C2C20(2);
                m_c4 = 2;
                break;
            case 34:
                func_002C2C20(3);
                break;
            case 35:
                if (data_00437CE8->m_d2)
                    func_002C2C20(3);
                else
                    func_002C2C20(m_c4);
                break;
            case 36:
                break;
            }
        }
    }
    return r;
}

/* a node action as far as this unit reads it (zseal.h's CNodeAction, fts_action.cpp's, which this
 * unit does not include: a word at 8, its type at 0x24, its AI action's id at 0x3C) */
class CNodeAction {
public:
    unsigned char _0[8];
    unsigned int m_8;   // 0x8
    unsigned char _c[0x24 - 0xc];
    unsigned char m_type;   // 0x24 (zseal.h's NODEACTION_TYPE)
    unsigned char _25[0x3c - 0x25];
    unsigned short m_id;   // 0x3c
};

/* whether a SEAL -- the player, or an AI that is a SEAL -- may take a door action from where it
 * stands: the action's word at 8 at least 2, its type 0, the range at least a half, and the AI map's
 * action of its id a door, closed (state 0), facing the SEAL's side (the cosine between the door's
 * direction and the SEAL's from its cell positive) (OVL2 0x5EC4E0, new in SOCOM II: CSealCtrl's
 * OnActionButton and CAiSAction::Tick ask it; the name ours). The SEAL's cell taken by reference
 * first (the target computes its address into s0 right after GetSealCtrlAi), the bit through a
 * ternary (the target's sltu).
 * OPEN, 0x1B0 against 0x1B8: the type and range tests' early returns -- the target jumps from both
 * to the shared `return r` (move v0,s1), MW folds r's false into a `return 0` of their own; and the
 * result, the action and the SEAL's cell take s2/s0/s1 where the target has s1/s2/s0. Tried: one
 * nested if (0x17C), `(bool)` and `!= 0` for the bit (0x1A8), the cell read in place (0x19C), the
 * two tests as one `||` (0x1A0) */
bool CanUseDoorAction(CZSealBody *seal, CNodeAction *a, float range)
{
    bool r = false;

    if (a->m_8 < 2)
        return false;
    static float minRange = 0.5f;

    if (a->m_type)
        return r;
    if (range < minRange)
        return r;
    CSealCtrlAi *ai = ((CUnitSealCtrl *)((CEntity *)seal)->m_control)->GetSealCtrlAi();
    const CAiMapLoc &from = ((CEntity *)seal)->m_mapLocValid;

    if (seal->IsPlayer() || ai && (ai->m_navyseal ? true : false)) {
        CAiMap::action *act = 0;
        CAiMapLoc loc;

        if (AiMaps.GetActionById(a->m_id, loc, &act) && act->GetType() == CAiMap::action::DOOR && !act->m_state
            && CAiMap::GetDirCos(CAiMap::GetClosestDir(from, loc), (CAiMap::DIR)act->m_direction) > 0.0f)
            r = true;
    }
    return r;
}
