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

extern BriefingRoomDB data_0044F568;   /* the room list (the rooms' unit's) */
extern BriefingRoom data_0044FAB0;   /* a room (the lobby unit's) */
extern BriefingRoom *data_0044FE10;   /* the room we are in, if any */
void func_002E81A0();   /* the clan team forgotten (the clan unit's) */
}

void ReleaseFileTransfers();   /* the Medius file API's transfers forgotten (zNetMediusFile.cpp) */

/* a valve (zanim.h's CValve: only its look-up here) */
enum VALVE_TYPE {
    VALVE_TYPE_0,
    VALVE_TYPE_1
};

class CValve {
public:
    static CValve *Create(const char *name, VALVE_TYPE type);
};

/* the clan's record (the clan unit's StoredClanInfo: Init clears the clan, its leader and name) */
struct LobbyClanInfo {
    unsigned char _0[0x1c];
    int ClanID;   // 0x1c
    unsigned char _20[4];
    char ClanName[32];   // 0x24
    int LeaderAccountID;   // 0x44
    unsigned char _48[0x170 - 0x48];
};

extern "C" {
extern LobbyClanInfo StoredClanInfo;
extern int PlayerClanID;   /* -1 outside a clan */
extern int data_0066A9A0;   /* the game's players counted (the lobby unit's) */
extern int data_0066A990;
extern int data_0066A998;
extern int data_0044F518;
extern unsigned char data_0066A5F8;   /* the details to go at once (the chat unit's) */
void func_002E9920();   /* the clan team forgotten (the clan unit's) */
}

/* the lobby itself (0x44F370): the unit's one dynamically initialised static -- its initialiser
 * 0x401E30 constructs it and registers the destructor 0x2E0630 */
CZOnlineLobby theLobby;

/* the lobby built (SOCOM II's; OVL1 0x2E0710): the event and error handlers cleared ahead of the
 * queue, which is built as std::queue builds it -- from a temporary deque (0x2E0D90) copied
 * (0x2E07B0), the temporary's block buffer destroyed (0x2E19E0) -- then the flags, the mission
 * sequence cleared and no mission current */
CZOnlineLobby::CZOnlineLobby() : m_event_handler(0), m_net_error_handler(0)
{
    m_isinit = false;
    m_isopen = false;
    m_a8 = 1;
    for (int i = 0; i < 32; i++)
        m_MissionSequence[i] = 0;
    m_curr_seq_idx = -1;
}

/* the lobby torn down (SOCOM II's; OVL1 0x2E0630): closed and shut down as UnInit does, Close
 * inlined into both, then the queue's four levels */
CZOnlineLobby::~CZOnlineLobby()
{
    if (m_isopen)
        Close();
    if (m_isinit)
        UnInit();
}

/* the lobby started with the engine (SOCOM 1's; OVL1 0x2E0450): the DME set up, then once the
 * lobby's UI variables made, the clan forgotten, the online-state valve looked up and the queue
 * primed -- a state pushed and popped */
bool CZOnlineLobby::Init()
{
    DMEInit();
    if (!m_isinit) {
        m_isinit = true;
        g_bJoiningRandomGame = false;
        uiv_LobbyWorldNameVar = theUIVarManager.Add("MEDIUS_LOBBY_NAME", 0, UIVL_PERMANENT);
        uiv_GameNameVar = theUIVarManager.Add("GAMENAMEVAR", 0, UIVL_PERMANENT);
        uiv_ClanNameVar = theUIVarManager.Add("CLANNAMEVAR", 0, UIVL_PERMANENT);
        uiv_ClanDescription = theUIVarManager.Add("Clan_Description", 0, UIVL_PERMANENT);
        data_0066A9A0 = 0;
        data_0066A990 = 0;
        data_0066A998 = 0;
        data_0044F518 = 0;
        func_002E9920();
        PlayerClanID = -1;
        StoredClanInfo.ClanID = -1;
        StoredClanInfo.LeaderAccountID = -1;
        memset(StoredClanInfo.ClanName, 0, sizeof(StoredClanInfo.ClanName));
        socom_online_state = CValve::Create("socom_online_state", VALVE_TYPE_1);
        data_0066A5F8 = 0;
        m_state_change_fifo.push(ROOT_LOBBY_STATE);
        m_state_change_fifo.pop();
    }
    return m_isinit;
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
    data_0044F568.Release();
    LadderLeaders.DeleteAll();
    ReleaseVector(LadderLeaders);
    LadderRanking.DeleteAll();
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
    data_0044FAB0.DeleteGames();
    ReleaseVector<RoomGame>(data_0044FAB0);
    FriendVec.DeleteAll();
    ReleaseVector(FriendVec);
    func_002E81A0();
    ReleaseFileTransfers();
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
        room->DeleteGames();
        ReleaseVector<RoomGame>(*room);
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
