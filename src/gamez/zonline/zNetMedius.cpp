/* zNetMedius.cpp -- the online lobby itself: theLobby, its construction, Init/UnInit, Open and
 * Close, the state machine's queue and handlers (SOCOM 1's zNetMedius.cpp, whose name the demo's
 * __sinit_zNetMedius.cpp gives; it held the whole lobby, which SOCOM II splits). OVL1
 * 0x2DF560-0x2E1CDF: after int_material.cpp (the zdb-world lane's, to 0x2DF554), its static
 * initialiser 0x401E30 constructing theLobby (0x2E0710) and registering its destructor (0x2E0630);
 * the queue's deque instances (0x2E07B0-0x2E1BB0) close it, before the rooms' unit (0x2E1CE0).
 * Only some of it so far. Defined in reverse of their image order (-inline deferred). */

#include "zonline.h"

extern "C" {
extern int myConnectionIndex;   /* the Medius connection (SOCOM 1's) */
extern unsigned char g_bJoiningRandomGame;
extern unsigned char data_0044F510;   /* the DME set up */
extern unsigned char data_0044F490[0x40];   /* the DME's state (handed to OVL2) */
extern unsigned char data_0044F4D0[0x40];

void func_00300680();   /* the Medius connection closed */
void func_006305C8(void *a, void *b);   /* (OVL2: the DME's set up) */

extern unsigned char data_0044F568[];   /* the room list (the rooms' unit's) */
extern BriefingRoom data_0044FAB0;   /* a room (the lobby unit's) */
extern BriefingRoom *data_0044FE10;   /* the room we are in, if any */
void func_002E4010(void *rooms);   /* the room list released */
void func_002EC140(LadderList *list);   /* a player ladder deleted (LadderList::DeleteAll) */
void func_002E81A0();   /* the clan team forgotten (the clan unit's) */
void func_00305640();   /* (the unit after the friends') */
}

/* the DME set up, once (SOCOM II's; OVL1 0x2E05D0) */
void *CZOnlineLobby::DMEInit()
{
    if (!data_0044F510 && theLobby.m_a8) {
        func_006305C8(data_0044F490, data_0044F4D0);
        data_0044F510 = 1;
    }
    return data_0044F490;
}

/* the lobby's lists given back: each deleted and its storage freed, the UI variables forgotten,
 * the room we were in emptied (SOCOM 1's, which also freed the rooms' and games' own storage;
 * OVL1 0x2DF8E0). Its ReleaseVector instances follow it, in reverse of their first use here. The
 * UI variables cleared are those Init creates but the lobby's names (0xc0, 0xc4) and the clan's
 * (0xec-0xf4) */
void CZOnlineLobby::ReleaseUIResources()
{
    func_002E4010(data_0044F568);
    func_002EC140(&LadderLeaders);
    ReleaseVector(LadderLeaders);
    func_002EC140(&LadderRanking);
    ReleaseVector(LadderRanking);
    ClanLadderRanking.DeleteAll();
    ReleaseVector(ClanLadderRanking);
    ClanLadderLeaders.DeleteAll();
    ReleaseVector(ClanLadderLeaders);
    InvitationVec.DeleteAll();
    ReleaseVector(InvitationVec);
    ClanInvitationVec.DeleteAll();
    ReleaseVector(ClanInvitationVec);
    ClanMemberVec.DeleteAll();
    ReleaseVector(ClanMemberVec);
    data_0044FAB0.m_games.DeleteAll();
    ReleaseVector(data_0044FAB0.m_games);
    FriendVec.DeleteAll();
    ReleaseVector(FriendVec);
    func_002E81A0();
    func_00305640();
    uiv_GameList = 0;
    uiv_RoomList = 0;
    uiv_RoomPlayerList = 0;
    uiv_MediusChatRecvVar = 0;
    uiv_MediusMOTD = 0;
    uiv_ClanMessages = 0;
    uiv_MediusMyInvites = 0;
    uiv_ClanInvitations = 0;
    uiv_ClanRoster = 0;
    uiv_MediusLadderTop10 = 0;
    uiv_MediusLadderRanking = 0;
    uiv_ClanLadderLeaders = 0;
    uiv_ClanLadderRanking = 0;
    g_bJoiningRandomGame = false;
    BriefingRoom *room = data_0044FE10;

    if (room) {
        room->m_games.DeleteAll();
        ReleaseVector(room->m_games);
    }
}

/* the state's handler called with the flag, when the state has one (SOCOM II's; OVL1 0x2DFE10) */
void CZOnlineLobby::RunStateHandler(LOBBY_STATE state, LOBBY_STATE_FLAG flag)
{
    if (m_state_handler[state])
        m_state_handler[state](flag);
}

/* the lobby closed: the connection dropped and its lists let go (SOCOM 1's, which only let them
 * go; OVL1 0x2DFDD0) */
void CZOnlineLobby::Close()
{
    if (m_isopen) {
        m_isopen = false;
        func_00300680();
        ReleaseUIResources();
    }
}

/* the lobby shut down with the engine (SOCOM 1's; OVL1 0x2DF650): Close inlined, its test
 * repeated */
void CZOnlineLobby::UnInit()
{
    if (m_isopen)
        Close();
    m_isinit = false;
    g_bJoiningRandomGame = false;
}

/* the Medius connection's index (SOCOM 1's; OVL1 0x2DF640) */
int CZOnlineLobby::MediusConnectionIndex()
{
    return myConnectionIndex;
}
