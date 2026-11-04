/* zNetMedius.cpp -- the online lobby itself: theLobby, its construction, Init/UnInit, Open and
 * Close, the state machine's queue and handlers (SOCOM 1's zNetMedius.cpp, whose name the demo's
 * __sinit_zNetMedius.cpp gives; it held the whole lobby, which SOCOM II splits). OVL1
 * 0x2DF560-0x2E1CDF: after int_material.cpp (the zdb-world lane's, to 0x2DF554), its static
 * initialiser 0x401E30 constructing theLobby (0x2E0710) and registering its destructor (0x2E0630);
 * the queue's deque instances (0x2E07B0-0x2E1BB0) close it, before the rooms' unit (0x2E1CE0).
 * Only some of it so far. Defined in reverse of their image order (-inline deferred). */

#include <stdio.h>
#include "zonline.h"
#include "../znet/znet.h"

extern "C" {
extern int myConnectionIndex;   /* the Medius connection (SOCOM 1's) */
extern unsigned char g_bJoiningRandomGame;
extern unsigned char data_0044F510;   /* the DME set up */
extern unsigned char data_0044F490[0x40];   /* the DME's state (handed to OVL2) */
extern unsigned char data_0044F4D0[0x40];

void func_00300680();   /* the Medius connection closed */
void KM_GenerateRSAKeyPair(void *pPublic, void *pPrivate);   /* (OVL2 0x6305C8: the DME's RSA key pair,
                                                               * Medius' rt_crypt.c -- the GCC lane's round 18b) */

extern BriefingRoomDB data_0044F568;   /* the room list (the rooms' unit's) */
extern BriefingRoom data_0044FAB0;   /* a room (the lobby unit's) */
void func_002E81A0();   /* the clan team forgotten (the clan unit's) */
void func_002E3AA0(CZOnlineLobby *lobby, C2D *page);   /* the room list asked for (the rooms' unit's) */
}

/* the LAN lobby: zonline.h's CZLANLobby (here only its room, the one we are in, and its test) */

void ReleaseFileTransfers();   /* the Medius file API's transfers forgotten (zNetMediusFile.cpp) */

/* a valve (zanim.h's CValve: its look-up and its setter here) */
enum VALVE_TYPE {
    VALVE_TYPE_0,
    VALVE_TYPE_1
};

enum VALVE_STATE {
    VALVE_SAME,
    VALVE_ON,
    VALVE_OFF
};

class CValve {
public:
    static CValve *Create(const char *name, VALVE_TYPE type);
    void MakeCallbacks(VALVE_STATE state);
    /* the value set, the callbacks told when it turns on or off (the chat unit's, zanim.h's) */
    bool SetValue(int value)
    {
        VALVE_STATE state = VALVE_SAME;

        if (m_value != value)
            state = value == 0 ? VALVE_OFF : VALVE_ON;
        m_value = value;
        if (state != VALVE_SAME) {
            MakeCallbacks(state);
            return true;
        }
        return false;
    }

    char *m_name;   // 0x0
    unsigned short m_value;   // 0x4
    unsigned short m_flags;   // 0x6
};

extern "C" {
extern float data_004A449C;   /* the frame's time (fts_patch.cpp's) */
}

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
        KM_GenerateRSAKeyPair(data_0044F490, data_0044F4D0);
        data_0044F510 = 1;
    }
    return data_0044F490;
}

/* ---- Open: the lobby server's connection asked for ---- */

/* a server's addresses, its key, and where it is (the universe unit's layouts: the info a class
 * with a constructor, so copied member by member; the key a plain struct, as eight two-word
 * passes) */
struct NetAddressList {
    int AddressType0;   // 0x0
    char Address0[16];   // 0x4
    int BinaryAddress0;   // 0x14
    int AddressType1;   // 0x18
    char Address1[16];   // 0x1c
    int BinaryAddress1;   // 0x2c
};   // 0x30

struct NetServerKey {
    unsigned int m_words[16];
};   // 0x40

struct NetConnectionInfo {
    NetConnectionInfo() {}

    int Type;   // 0x0
    NetAddressList AddressList;   // 0x4
    int WorldID;   // 0x34
    NetServerKey ServerKey;   // 0x38
    char SessionKey[17];   // 0x78
    char AccessKey[17];   // 0x89
};   // 0x9c

/* a connection asked for: the server's info, then the buffer's size and the callback told the
 * connection's index */
struct MediusConnectParams {
    NetConnectionInfo ConnectInfo;   // 0x0
    unsigned char _9c[0xf0 - 0x9c];
    int BufferSize;   // 0xf0: 256
    void (*Callback)(int index);   // 0xf4
    int _f8;   // 0xf8
    int _fc;   // 0xfc
};   // 0x100

/* the account logged in (OVL2 bss; SOCOM 1's AccountInfo, its name and password Open sets) */
struct AccountInfoType {
    int AccountID;   // 0x0
    char AccountName[32];   // 0x4
    char AccountPassword[32];   // 0x24
};

extern "C" {
extern int MediusWorldID;   /* the game joined (SOCOM 1's) */
extern int LobbyWorldID;   /* the room we are in (SOCOM 1's) */
extern AccountInfoType AccountInfo;
extern unsigned char data_0066A8E0;   /* logged in (the chat unit's) */
extern unsigned char data_0044F470;
extern char data_0066A9B0[256];   /* the lobby server's host */
extern int data_0066AAB0;   /* and port */
extern GamePlayerStats data_0066A750;   /* our stats (zonline.h's: the side's two bits in byte 2, the
                                           ladder figure at 4, the description at 0x9F) */
extern int data_0066A838;   /* our performance figure, after them (0xE8; SOCOM 1's
                               UniversalGamePerformanceMetric) */
extern NetConnectionInfo data_0066A540;   /* the lobby server (OVL2 bss) */
void func_002ED460(CZOnlineLobby *lobby);   /* the state handlers registered (the chat unit's table) */
MediusErrorCode MediusVersionClient(char *version);
int MediusGetBuildTimeStamp(char *stamp);
void func_0064A750(NetConnectionInfo *info, int type, const char *ip, int port, int flags);   /* (the info built for an address) */
void func_0064F4E0(NetServerKey *key);   /* (the DNAS key) */
bool func_00300740(int kind, MediusConnectParams *params);   /* a connection asked for (the friends' unit's) */
}

/* the lobby server connected: the session begun (OVL1 0x2E0440, the callback Open hands over) */
extern "C" void func_002E0440(int index)
{
    theLobby.ChangeState(SESSION_BEGIN);
}

/* the lobby opened on a server (SOCOM 1's Open, which registered the handlers itself; OVL1
 * 0x2E0030: UILobbyConnect calls it on theLobby with the server's host and port): once, the
 * session's globals reset, the handlers registered by the chat unit's table, the UI variables
 * made, the server looked up and its connection asked for -- the error state entered at once
 * when it cannot be */
bool CZOnlineLobby::Open(const char *host, int port)
{
    if (m_isopen)
        return false;
    m_isopen = true;
    MediusWorldID = -1;
    SessionKey[0] = 0;
    data_0066A8E0 = 0;
    data_0044F470 = 0;
    strncpy(data_0066A9B0, host, 256);
    data_0066AAB0 = port;
    LobbyWorldID = -1;
    data_0066A750.m_rating = 99;
    data_0066A750.m_side = STATS_SEALS;
    sprintf(data_0066A750.m_desc, "Description Not Set");
    data_0066A838 = 65000;
    func_002E9920();
    AccountInfo.AccountID = -1;
    CopyTo(AccountInfo.AccountName, "zname_a", 32);
    CopyTo(AccountInfo.AccountPassword, "zpass_a", 32);
    m_current_state = ROOT_LOBBY_STATE;
    for (int i = 0; i < MAX_LOBBY_STATES; i++)
        m_state_handler[i] = 0;
    while (!m_state_change_fifo.empty())
        m_state_change_fifo.pop();
    func_002ED460(this);
    CreateUIResources();
    char ip[64];
    char server[64];
    char stamp[64];
    char client[64] = {0};
    MediusConnectParams params;
    NetServerKey key;

    theNetwork.zNetLookupHost(host, ip, sizeof(ip));
    MediusVersionClient(server);
    MediusGetBuildTimeStamp(stamp);
    MediusVersionClient(client);
    func_0064A750(&params.ConnectInfo, 0, ip, port, 1);
    params.BufferSize = 256;
    memcpy(&data_0066A540, &params.ConnectInfo, sizeof(NetConnectionInfo));
    if (theLobby.m_a8)
        func_0064F4E0(&key);
    else
        memset(&key, 0, sizeof(key));
    data_0066A540.ServerKey = key;
    params.ConnectInfo = data_0066A540;
    params.Callback = func_002E0440;
    params._f8 = 0;
    params._fc = 0;
    if (!func_00300740(1, &params)) {
        theLobby.ChangeState(CONNECT_ERROR);
        theLobby.Tick();
    }
    return true;
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
    BriefingRoom *room = data_0044FE10.m_room;

    if (room) {
        room->DeleteGames();
        ReleaseVector<RoomGame>(*room);
    }
}

/* the lobby run each frame (SOCOM 1's Tick, zNetUpdate's call; OVL1 0x2DFE50): a state waiting is
 * entered -- the current one's handler told it is left, the next taken off the queue (the block's
 * front() inline, SOCOM 1 called the circular buffer's [] out of line) and told it is entered, the
 * online valve set to it -- else the current one ticked; then the game clock counted down by the
 * frame's time while it is not negative */
/* a variable's number, read as zrdr.h's const Real() reads it (zui.h's Real() is not const: read
 * through a non-const view, MW reuses its type byte for the float setter's test, where the target
 * loads it again -- a const and a non-const access are not one expression to MW's CSE) */
inline float ClockReal(const _zrdr *z)
{
    if (z->type == 2)
        return z->real;
    else if (z->type == 1)
        return (float)z->integer;
    else
        return 0.0f;
}

void CZOnlineLobby::Tick()
{
    if (m_isopen) {
        if (!m_state_change_fifo.empty()) {
            RunStateHandler(m_current_state, EXIT);
            m_current_state = m_state_change_fifo.front();
            m_state_change_fifo.pop();
            RunStateHandler(m_current_state, ENTRY);
            if (socom_online_state)
                socom_online_state->SetValue(m_current_state);
        } else {
            RunStateHandler(m_current_state, TICK);
        }
        if (uiv_GameClock) {
            float t = ClockReal(uiv_GameClock);

            if (!(t < 0.0f)) {
                t -= data_004A449C;
                *uiv_GameClock = t;
            }
        }
    }
}

/* the lobby's UI variables made (SOCOM II's, out of SOCOM 1's Open; OVL1 0x2DF6A0: Open, the LAN
 * lobby's Connect, the ready-to-connect state and UIDisconnectFromGameServer call it; the name ours,
 * after SOCOM 1's ReleaseUIResources): the lists' variables, the chat's an empty list, the messages'
 * empty strings; no game clock; the room list asked for unless on a LAN; the counts cleared */
void CZOnlineLobby::CreateUIResources()
{
    uiv_GameList = theUIVarManager.Add("GAMELISTVAR", 0, UIVL_TEMPORARY);
    uiv_RoomList = theUIVarManager.Add("ROOMLISTVAR", 0, UIVL_TEMPORARY);
    uiv_RoomPlayerList = theUIVarManager.Add("ROOMPLAYERLISTVAR", 0, UIVL_TEMPORARY);
    CRdrEditor ed;

    ed.push(0);
    ed.pop();
    uiv_MediusChatRecvVar = theUIVarManager.Add("MEDIUS_CHAT_RECV_VAR", &ed, UIVL_TEMPORARY);
    uiv_MediusMOTD = theUIVarManager.Add("MEDIUS_MOTD", 0, UIVL_TEMPORARY);
    *uiv_MediusMOTD = "";
    uiv_ClanMessages = theUIVarManager.Add("MEDIUS_CLAN_MESSAGES", 0, UIVL_TEMPORARY);
    *uiv_ClanMessages = "";
    uiv_ClanInvitations = theUIVarManager.Add("INVITED_PLAYERS", 0, UIVL_TEMPORARY);
    uiv_ClanRoster = theUIVarManager.Add("CLAN_ROSTER", 0, UIVL_TEMPORARY);
    uiv_MediusMyInvites = theUIVarManager.Add("MY_CLAN_INVITES", 0, UIVL_TEMPORARY);
    uiv_MediusLadderTop10 = theUIVarManager.Add("LADDER_LEADERS", 0, UIVL_TEMPORARY);
    uiv_MediusLadderRanking = theUIVarManager.Add("LADDER_RANKING", 0, UIVL_TEMPORARY);
    uiv_ClanLadderLeaders = theUIVarManager.Add("CLAN_LADDER_LEADERS", 0, UIVL_TEMPORARY);
    uiv_ClanLadderRanking = theUIVarManager.Add("CLAN_LADDER_RANKING", 0, UIVL_TEMPORARY);
    uiv_GameClock = 0;
    if (!data_0044FE10.IsActive())
        func_002E3AA0(this, 0);
    data_0066A9A0 = 0;
    data_0066A998 = 0;
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
