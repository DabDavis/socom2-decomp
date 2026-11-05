/* zNetMediusChat.cpp -- the lobby's state machine and its chat: the state handlers the table at
 * 0x2ED460 registers (SOCOM 1's Open registered the same ones, by state) and their Medius
 * callbacks, the player-info pages, the chat queues, the censor, the messages' byte-order
 * converters (SOCOM 1's zNetMedius.cpp's code; the unit and its name are ours: OVL1 from about
 * 0x2ED460, after the ladder unit, to 0x2F6CFF, its static initialiser 0x4020E0 building the two
 * queues 0x44FA20 and 0x44FA50, whose destructor 0x2F5E50 and deque instances close it -- the
 * instances run on past 0x2F62B0 to 0x2F6CF7: 0x2F6340, 0x2F6600, 0x2F6A70 and 0x2F6BC0 are
 * called only from this unit's deque copy constructor 0x2F5F80, its 0x2F6090 and ~__deque_buf
 * 0x2F62B0, so they are its copies, and the lobby unit starts at 0x2F6D00). Still unwritten: the
 * game's world report 0x2EF590 and details 0x2EF7F0, the player-info callback 0x2F0020 (cbGetPlayerInfo) and the
 * player lists 0x2F0EA0-0x2F3B90 (class-vector walks and their sort) and the deque instances.
 * Defined in reverse of their image order (-inline deferred). */

#include <stdio.h>
#include <stdlib.h>
#include <algorithm>
#include "zonline.h"
#include "../zmath/zmath.h"
#include "../znet/znet.h"

/* The unit's inline size limit was lowered to 100 in round 10 (`#pragma inline_max_size(100)`):
 * the target's initialiser inlines the chat queues' constructor one level -- its own body, the
 * queue's members called out of line (0x2F5EE0, 0x2F5EC0, 0x2F5ED0, the queue's constructor
 * 0x2F5F50) -- and the round-10 headers expanded it under the default 256. Round 15: with today's
 * deeper include/deque the initialiser and the queue's pieces come out the same at the default,
 * while at 100 the room players' std::sort (0x2F1980, its __sort132 and __selection_sort) calls
 * the iterator's operator* and swap<MediusLobbyWorldPlayerListResponse> out of line -- so the pragma is gone (58 -> 61
 * of the unit's rows, nothing lost). */

/* --- the Medius requests the state handlers make (SOCOM 1's; only what is passed) ----------- */

/* the account logged in or registered (OVL2 bss; SOCOM 1's AccountInfo, 0x44 bytes) */
struct ChatAccountInfo {
    int AccountID;   // 0x0
    char AccountName[32];   // 0x4
    char Password[32];   // 0x24
};

struct MediusSessionBeginRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0x0
    int ConnectionClass;   // 0x18: 1
};   // 0x1c

struct MediusAccountRegistrationRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0x0
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];   // 0x15
    int AccountType;   // 0x28: 1
    char AccountName[32];   // 0x2c
    char Password[32];   // 0x4c
};   // 0x6c

struct MediusAccountLoginRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0x0
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];   // 0x15
    char AccountName[32];   // 0x26
    char Password[32];   // 0x46
};   // 0x66

struct MediusCreateChannelRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0x0
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];   // 0x15
    int ApplicationID;   // 0x28: 10472
    int MaxPlayers;   // 0x2c
    char LobbyName[64];   // 0x30
    char LobbyPassword[32];   // 0x70
    int GenericField;   // 0x90
};   // 0x94

struct MediusJoinChannelRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0x0
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];   // 0x15
    int MediusWorldID;   // 0x28
    char LobbyChannelPassword[32];   // 0x2c
};   // 0x4c

/* how a game is joined (int-sized, as the SDK's enums are: a ternary of two of its values stored
 * is `li 1; movz`, an int's `sltu`) */
enum MediusJoinType {
    MediusJoinAsPlayer = 0,
    MediusJoinAsSpectator = 1,
    MediusJoinType_Extent = 0x7FFFFFFF
};

struct MediusJoinGameRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0x0
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];   // 0x15
    int MediusWorldID;   // 0x28
    MediusJoinType JoinType;   // 0x2c
    char GamePassword[32];   // 0x30
    int GameHostType;   // 0x50
    unsigned char _54[0xc4 - 0x54];
};   // 0xc4

/* a server's addresses (the DME SDK's NetAddressList: a plain struct, copied as words) */
struct NetAddressList {
    int AddressType0;   // 0x0
    char Address0[16];   // 0x4
    int BinaryAddress0;   // 0x14
    int AddressType1;   // 0x18
    char Address1[16];   // 0x1c
    int BinaryAddress1;   // 0x2c
};   // 0x30

/* where a server is and how to talk to it (the DME SDK's NetConnectionInfo, 0x9C bytes: a class
 * with a constructor of its own, so MW copies it member by member -- the addresses as words, the
 * key's array in a loop of eight two-word passes, the two keys bytewise) */
struct NetConnectionInfo {
    NetConnectionInfo() {}

    int Type;   // 0x0
    NetAddressList AddressList;   // 0x4
    int WorldID;   // 0x34
    unsigned int ServerKey[16];   // 0x38
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];   // 0x78
    char AccessKey[MEDIUS_SESSIONKEY_MAXLEN];   // 0x89
};   // 0x9c

/* a world looked for by its name (SOCOM 1's MediusFindWorldByName: 1, a room) */
struct MediusFindWorldByNameRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0x0
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];   // 0x15
    char Name[64];   // 0x26
    int WorldType;   // 0x68
};   // 0x6c

struct MediusFindWorldByNameResponse { MEDIUS_RESPONSE_HEAD };

struct MediusCreateChannelResponse {
    MEDIUS_RESPONSE_HEAD
    int MediusWorldID;   // 0x1c
};
struct MediusJoinChannelResponse {
    MEDIUS_RESPONSE_HEAD
    NetConnectionInfo ConnectInfo;   // 0x1c
};
struct MediusJoinGameResponse {
    MEDIUS_RESPONSE_HEAD
    int GameHostType;   // 0x1c
    NetConnectionInfo ConnectInfo;   // 0x20
};

/* a game created (SOCOM 1's MediusCreateGame, wider: the three generic fields and the host's type
 * SOCOM II's) */
struct MediusCreateGameRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0x0
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];   // 0x15
    int ApplicationID;   // 0x28: 10472
    int MinPlayers;   // 0x2c: 2
    int MaxPlayers;   // 0x30
    int GameLevel;   // 0x34
    char GameName[64];   // 0x38
    char GamePassword[32];   // 0x78
    char SpectatorPassword[32];   // 0x98
    int PlayerSkillLevel;   // 0xb8
    int RulesSet;   // 0xbc
    int GenericField1;   // 0xc0: the room
    int GenericField2;   // 0xc4: 65000
    int GenericField3;   // 0xc8: the clan game's
    int GameHostType;   // 0xcc
};   // 0xd0

struct MediusCreateGameResponse {
    MEDIUS_RESPONSE_HEAD
    int MediusWorldID;   // 0x1c
};

/* a valve: a named 16-bit value (SOCOM 1's CValve; only what is read here) */
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
    static CValve *GetByName(const char *name);
    /* the value as an int (ours: read through it, the test's load serves the sum unmasked) */
    int Value() { return m_value; }
    void MakeCallbacks(VALVE_STATE state);
    /* the value set, the callbacks told when it turns on or off (zanim.h's) */
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
    unsigned short m_value;   // 0x4 (the bit-field's halfword: added to a count with no mask)
    unsigned short m_flags;   // 0x6: the type and the pool bits
};

struct MediusSessionBeginResponse { MEDIUS_RESPONSE_HEAD };
struct MediusVersionServerResponse { MEDIUS_RESPONSE_HEAD };
struct MediusSessionEndResponse { MEDIUS_RESPONSE_HEAD };
struct MediusAccountRegistrationResponse { MEDIUS_RESPONSE_HEAD };
struct MediusAccountLoginResponse {
    MEDIUS_RESPONSE_HEAD
    int AccountID;   // 0x1c
    int AccountType;   // 0x20
    int MediusWorldID;   // 0x24
    NetConnectionInfo ConnectInfo;   // 0x28
};   // 0xc4

/* the console's language and text encoding told to Medius (SOCOM II's) */
struct MediusSetLocalizationParamsRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0x0
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];   // 0x15
    int CharacterEncoding;   // 0x28: 2
    int Language;   // 0x2c
};   // 0x30

/* the console's DNAS id posted (SOCOM II's) */
struct MediusMachineSignaturePost {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0x0
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];   // 0x15
    int _28;   // 0x28
    unsigned char Length;   // 0x2c
    unsigned char Signature[32];   // 0x2d
    unsigned char _4d[0x50 - 0x4d];
};   // 0x50

struct MediusSetLocalizationParamsResponse { MEDIUS_RESPONSE_HEAD };

/* a connection to a server asked for (SOCOM II's: the server's info, then the buffer's size and the
 * callback told the connection's index) */
struct MediusConnectParams {
    NetConnectionInfo ConnectInfo;   // 0x0
    unsigned char _9c[0xf0 - 0x9c];
    int BufferSize;   // 0xf0: 256
    void (*Callback)(int index);   // 0xf4
    int _f8;   // 0xf8
    int _fc;   // 0xfc
};   // 0x100

/* our state told to the lobby server (SOCOM 1's MediusUpdateClientState: 1, joined) */
struct MediusUpdateClientStateRequest {
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];   // 0x0
    int UserAction;   // 0x14
};   // 0x18

/* the game list filtered (SOCOM II's request) */
struct MediusSetGameListFilterRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0x0
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];   // 0x15
    int FilterField;   // 0x28
    int ComparisonOperator;   // 0x2c
    int BaselineValue;   // 0x30
};   // 0x34

struct MediusSetGameListFilterResponse { MEDIUS_RESPONSE_HEAD };

extern "C" {
extern ChatAccountInfo AccountInfo;
extern unsigned int data_0044F9E0;   /* when the active lobby next refreshes the player's info */

MediusErrorCode MediusSessionBegin(MediusSessionBeginRequest *req,
                              void (*cb)(int, int, int, MediusSessionBeginResponse *));   /* MediusSessionBegin */
MediusErrorCode MediusVersionServer(MediusSessionRequest *req,
                              void (*cb)(int, int, int, MediusVersionServerResponse *));   /* MediusVersionServer */
MediusErrorCode MediusSessionEnd(MediusSessionRequest *req,
                              void (*cb)(int, int, int, MediusSessionEndResponse *));   /* MediusSessionEnd */
MediusErrorCode MediusAccountRegistration(MediusAccountRegistrationRequest *req,
                              void (*cb)(int, int, int, MediusAccountRegistrationResponse *));   /* MediusAccountRegistration */
MediusErrorCode MediusAccountLogin(MediusAccountLoginRequest *req,
                              void (*cb)(int, int, int, MediusAccountLoginResponse *));   /* MediusAccountLogin */

void func_002EE910(int, int, int, MediusSessionBeginResponse *);   /* cbSessionBegin */
void func_002EFE00(int, int, int, MediusVersionServerResponse *);   /* cbVersionServer */
void func_002EE7B0(int, int, int, MediusSessionEndResponse *);   /* cbSessionEnd */
MediusErrorCode func_0064AB98(MediusSetLocalizationParamsRequest *req,
                              void (*cb)(int, int, int, MediusSetLocalizationParamsResponse *));   /* MediusSetLocalizationParams */
MediusErrorCode func_00647EC0(MediusMachineSignaturePost *req);   /* the DNAS id posted */
void func_002EF3B0(int, int, int, MediusSetLocalizationParamsResponse *);   /* its callback */
int func_002EF340(unsigned char language);   /* the console's language as Medius's */
extern NetConnectionInfo data_0066A840;   /* the account's lobby server (OVL2 bss) */
extern int data_0066A5E0;   /* the lobby server's connection (OVL2 bss) */
extern float data_0044F8D0;   /* when the lobby server was asked for */
bool func_00300740(int kind, MediusConnectParams *params);   /* a connection asked for (the friends' unit's) */
MediusErrorCode MediusUpdateClientState(MediusUpdateClientStateRequest *req);   /* MediusUpdateClientState */
MediusErrorCode MediusSetGameListFilter(MediusSetGameListFilterRequest *req,
                              void (*cb)(int, int, int, MediusSetGameListFilterResponse *));   /* MediusSetGameListFilter */
void func_002EFE10(int, int, int, MediusSetGameListFilterResponse *);   /* its callback */
void func_002F3FF0(struct ChatSlot *slots);   /* a free slot looked for (below) */
void func_002F7690();   /* (the lobby unit's) */
void func_002F4FC0(int ready);   /* our readiness told the server */
void func_002EF7F0();   /* our stats reported (below) */
int func_002EF590(CValve *locked);   /* the game's world reported, the delay to the next returned (below) */
extern CValve *data_0044F9D0;   /* the game's lock, while it is up */
extern unsigned int data_0044F9D8;   /* when the game's details next go to the lobby */
extern unsigned char data_0066A5F8;   /* the details to go at once */
extern unsigned char data_0044F890[64];   /* the server's key kept */
extern unsigned char data_0066A8E0;   /* logged in */

MediusErrorCode MediusCreateChannel(MediusCreateChannelRequest *req,
                              void (*cb)(int, int, int, MediusCreateChannelResponse *));   /* MediusCreateChannel */
MediusErrorCode MediusJoinChannel(MediusJoinChannelRequest *req,
                              void (*cb)(int, int, int, MediusJoinChannelResponse *));   /* MediusJoinChannel */
MediusErrorCode MediusJoinGame(MediusJoinGameRequest *req,
                              void (*cb)(int, int, int, MediusJoinGameResponse *));   /* MediusJoinGame */
MediusErrorCode MediusFindWorldByName(MediusFindWorldByNameRequest *req,
                              void (*cb)(int, int, int, MediusFindWorldByNameResponse *));   /* MediusFindWorldByName */
void func_002ED9C0(int, int, int, MediusFindWorldByNameResponse *);   /* cbFindRoomByName */
void func_002EE210(int, int, int, MediusFindWorldByNameResponse *);   /* cbFindGameByName */
MediusErrorCode MediusCreateGame(MediusCreateGameRequest *req,
                              void (*cb)(int, int, int, MediusCreateGameResponse *));   /* MediusCreateGame */
void func_002E9840(CValve *valve);   /* the clan game's team set from the valve (the clan unit's) */
int func_002E99F0();   /* the clan game's field (the clan unit's) */

extern NetConnectionInfo data_0066A540;   /* the room's lobby server (OVL2 bss) */
extern NetConnectionInfo data_0066A8F0;   /* the game's server (OVL2 bss) */
extern int LobbyWorldID;   /* the room we are in, -1 none (SOCOM 1's) */
extern int MediusWorldID;   /* the game joined (SOCOM 1's) */
extern unsigned char g_bJoiningRandomGame;
extern float data_004A44A0;   /* (a clock) */
extern float data_0044F9E8;   /* when the room was joined */
int func_002F6D00(CZOnlineLobby *lobby);   /* we join as a spectator */
void func_002F4080(void *msg, int type);   /* a request put into the network's order (empty) */

void func_00300680();   /* the Medius connection closed */
void func_002FA870(CZOnlineLobby *lobby);   /* the player's info asked for (the lobby unit's) */
void func_002FAA80(CZOnlineLobby *lobby);   /* and refreshed */
}

extern BriefingRoomDB data_0044F568;   /* the room list (the rooms' unit's) */
unsigned char GetLanguage();   /* the console's language (util_loc.cpp's) */
const char *GetString(int id);   /* a localised text (util_loc.cpp's) */

void UpdateChatDisplay();   /* the room's chat shown (below) */

/* the lobby's state changed: pushed onto its queue (SOCOM 1 pushed inline; OVL1 0x2EFE20) */
void CZOnlineLobby::ChangeState(LOBBY_STATE state)
{
    m_state_change_fifo.push(state);
}

/* --- the state handlers (SOCOM 1's; the table the lobby's 0x2ED460 fills, by state) ---------- */

/* a variable's text when it holds one (zNetMediusClan.cpp's) */
inline char *ZString(_zrdr *z)
{
    return z->type == 3 ? z->string : 0;
}

/* a field copied from a global's, the source's address tested as SetSessionKey's is (the
 * request's own address is not: CopyField's test of it stays for a member array; the name ours) */
inline void CopyFrom(char *dst, const char *src, unsigned int n)
{
    if (src)
        memcpy(dst, src, n);
}

/* the room joined: its lobby server kept and connected to, or back to the lobby (SOCOM 1's; OVL1
 * 0x2EE3A0) */
void cbJoinRoom(int, int, int, MediusJoinChannelResponse *resp)
{
    if (CheckMediusCallbackStatus("JoinRoom", resp->StatusCode)) {
        data_0066A540 = resp->ConnectInfo;
        theLobby.ChangeState(LOBBY_SERVER_CONNECT);
    } else {
        theLobby.ChangeState(LOBBY_ACTIVE);
        LobbyEvent("MP_MEDIUS_JOIN_ROOM_FAILURE",
                   "Cannot join room. It may be full. Please try another.");
    }
}

/* the game created and joined; a persistent game's name taken, the game of that name looked for
 * (SOCOM 1's; OVL1 0x2EE090) */
void cbCreateGame(int, int, int, MediusCreateGameResponse *resp)
{
    if (CheckMediusCallbackStatus("CreateGame", resp->StatusCode)) {
        MediusWorldID = resp->MediusWorldID;
        theLobby.ChangeState(JOIN_GAME);
    } else if (CValve::Create("mp_persistent", VALVE_TYPE_1)->m_value) {
        MediusFindWorldByNameRequest req;

        memset(&req, 0, sizeof(req));
        SetSessionKey(req.SessionKey);
        CUIVariable *var = theLobby.uiv_GameNameVar;
        if (ZString(var))
            memcpy(req.Name, ZString(var), sizeof(req.Name));
        req.WorldType = 0;
        CheckMediusError(MediusFindWorldByName(&req, func_002EE210));
    } else {
        if (resp->StatusCode == (MediusCallbackStatus)-975)
            LobbyEvent("MP_GAME_NAME_EXISTS", 0);
        else
            LobbyEvent("MP_MEDIUS_JOIN_GAME_FAILURE", 0);
        theLobby.ChangeState(LOBBY_ACTIVE);
    }
}

/* CREATE_NEW_GAME: the game the menu describes created -- its name, the passwords the valves ask
 * for, the players and spectators allowed (SOCOM 1's; OVL1 0x2EDDA0) */
void CreateNewGameStateHandler(LOBBY_STATE_FLAG flag)
{
    switch (flag) {
    case REGISTER:
        break;
    case ENTRY: {
        const char *name = "testgame";
        const char *password = "";
        const char *specPassword = password;
        int maxPlayers = 8;
        CValve *pwValve = CValve::GetByName("PasswordValve");
        CValve *specValve = CValve::GetByName("MaxSpectatorsValve");
        CValve *maxValve = CValve::GetByName("MaxPlayersValve");
        CValve *clanValve = CValve::GetByName("CreateClanAffiliation");
        CUIVariable *var = theLobby.uiv_GameNameVar;

        if (var && var->type == 3)
            name = ZString(var);
        var = theUIVarManager.Add("PlayerGamePassword", 0, UIVL_PERMANENT);
        if (pwValve && pwValve->m_value) {
            if (var && var->type == 3)
                password = ZString(var);
        } else {
            *var = "";
        }
        if (maxValve)
            maxPlayers = maxValve->m_value;
        var = theUIVarManager.Add("SpectatorGamePassword", 0, UIVL_PERMANENT);
        if (specValve && specValve->Value()) {
            maxPlayers += specValve->Value();
            if (var && var->type == 3)
                specPassword = ZString(var);
        } else {
            *var = "";
        }
        if (clanValve)
            func_002E9840(clanValve);
        MediusCreateGameRequest req;

        memset(&req, 0, sizeof(req));
        SetSessionKey(req.SessionKey);
        req.ApplicationID = 10472;
        req.MinPlayers = 2;
        req.MaxPlayers = maxPlayers;
        req.GameLevel = 0;
        if (name)
            memcpy(req.GameName, name, sizeof(req.GameName));
        if (password)
            memcpy(req.GamePassword, password, sizeof(req.GamePassword));
        if (specPassword)
            memcpy(req.SpectatorPassword, specPassword, sizeof(req.SpectatorPassword));
        req.PlayerSkillLevel = 0;
        req.RulesSet = 0;
        req.GenericField1 = LobbyWorldID;
        req.GenericField2 = 65000;
        req.GenericField3 = func_002E99F0();
        req.GameHostType = 0;
        CheckMediusError(MediusCreateGame(&req, cbCreateGame));
        break;
    }
    case TICK:
        break;
    case EXIT:
        break;
    case UNREGISTER:
        break;
    }
}

/* the game joined: its server kept and connected to, or back to the lobby with the reason
 * (SOCOM 1's; OVL1 0x2EDBA0) */
void cbJoinGame(int, int, int, MediusJoinGameResponse *resp)
{
    if (CheckMediusCallbackStatus("JoinGame", resp->StatusCode)) {
        data_0066A8F0 = resp->ConnectInfo;
        theLobby.ChangeState(READY_TO_CONNECT);
    } else {
        theLobby.ChangeState(LOBBY_ACTIVE);
        if (resp->StatusCode == (MediusCallbackStatus)-978)
            LobbyEvent("MP_INVALID_PASSWORD", 0);
        else if (resp->StatusCode == (MediusCallbackStatus)-972)
            LobbyEvent("MP_BANNED_FROM_GAME", 0);
        else
            LobbyEvent("MP_MEDIUS_JOIN_GAME_FAILURE", 0);
    }
}

/* JOIN_GAME: the game joined, as a spectator or a player, with the password the menu holds
 * (SOCOM 1's; OVL1 0x2EDA60) */
void JoinGameStateHandler(LOBBY_STATE_FLAG flag)
{
    switch (flag) {
    case REGISTER:
        break;
    case ENTRY: {
        MediusJoinGameRequest req;

        memset(&req, 0, sizeof(req));
        SetSessionKey(req.SessionKey);
        req.MediusWorldID = MediusWorldID;
        req.GameHostType = 0;
        req.JoinType = func_002F6D00(&theLobby) ? MediusJoinAsSpectator : MediusJoinAsPlayer;
        CUIVariable *var = theUIVarManager.Add(func_002F6D00(&theLobby) ? "SpectatorGamePassword"
                                                                         : "PlayerGamePassword",
                                               0, UIVL_PERMANENT);
        if (ZString(var))
            memcpy(req.GamePassword, ZString(var), sizeof(req.GamePassword));
        CheckMediusError(MediusJoinGame(&req, cbJoinGame));
        g_bJoiningRandomGame = 0;
        break;
    }
    case TICK:
        break;
    case EXIT:
        break;
    case UNREGISTER:
        break;
    }
}

/* JOIN_ROOM: the game list emptied and the room joined (SOCOM 1's; OVL1 0x2EE290) */
void JoinRoomStateHandler(LOBBY_STATE_FLAG flag)
{
    switch (flag) {
    case REGISTER:
        break;
    case ENTRY: {
        MediusJoinChannelRequest req;
        CRdrEditor ed;

        ed.push(0);
        ed.pop();
        theLobby.uiv_GameList->Set(&ed);
        memset(&req, 0, sizeof(req));
        SetSessionKey(req.SessionKey);
        req.MediusWorldID = LobbyWorldID;
        memcpy(req.LobbyChannelPassword, "", sizeof(req.LobbyChannelPassword));
        func_002F4080(&req, 45);
        data_0044F9E8 = data_004A44A0;
        CheckMediusError(MediusJoinChannel(&req, cbJoinRoom));
        break;
    }
    case TICK:
        break;
    case EXIT:
        break;
    case UNREGISTER:
        break;
    }
}

/* the room created and joined; when its name is taken, the room of that name looked for instead
 * (SOCOM 1's; OVL1 0x2ED850) */
void cbCreateRoom(int, int, int, MediusCreateChannelResponse *resp)
{
    if (CheckMediusCallbackStatus("CreateRoom", resp->StatusCode)) {
        LobbyWorldID = resp->MediusWorldID;
        BriefingRoom *room = data_0044F568.FindRoom(ZString(theLobby.uiv_LobbyWorldNameVar));

        if (room)
            room->MediusWorldID = resp->MediusWorldID;
        theLobby.ChangeState(JOIN_ROOM);
    } else if (resp->StatusCode == (MediusCallbackStatus)-974) {
        MediusFindWorldByNameRequest req;

        memset(&req, 0, sizeof(req));
        SetSessionKey(req.SessionKey);
        CUIVariable *var = theLobby.uiv_LobbyWorldNameVar;
        if (ZString(var))
            memcpy(req.Name, ZString(var), sizeof(req.Name));
        req.WorldType = 1;
        CheckMediusError(MediusFindWorldByName(&req, func_002ED9C0));
    } else {
        theLobby.ChangeState(LOBBY_ACTIVE);
        LobbyEvent("MP_MEDIUS_JOIN_ROOM_FAILURE", "Cannot join this room. Please try another.");
    }
}

/* CREATE_ROOM: the room the menu names created from the room list's (SOCOM 1's; OVL1 0x2ED740) */
void CreateRoomStateHandler(LOBBY_STATE_FLAG flag)
{
    switch (flag) {
    case REGISTER:
        break;
    case ENTRY: {
        BriefingRoom *room = data_0044F568.FindRoom(ZString(theLobby.uiv_LobbyWorldNameVar));

        if (room) {
            MediusCreateChannelRequest req;

            memset(&req, 0, sizeof(req));
            SetSessionKey(req.SessionKey);
            req.ApplicationID = 10472;
            req.MaxPlayers = room->MaxPlayers;
            CopyFrom(req.LobbyName, room->LobbyName, sizeof(req.LobbyName));
            CopyFrom(req.LobbyPassword, room->m_password, sizeof(req.LobbyPassword));
            CheckMediusError(MediusCreateChannel(&req, cbCreateRoom));
        }
        break;
    }
    case TICK:
        break;
    case EXIT:
        break;
    case UNREGISTER:
        break;
    }
}

void cbAccountRegistration(int, int, int, MediusAccountRegistrationResponse *resp);

unsigned char *GetDNASUniqueID(int *size);
void CountGamePlayers();   /* the players of the game in progress counted (the lobby unit's) */

/* the lobby server connected: our state told it, then to the lobby or the random game, the game
 * list's filter set, the version asked (SOCOM 1's; OVL1 0x2EFC80) */
void cbLobbyServerConnect(int index)
{
    if (index != -1) {
        MediusUpdateClientStateRequest upd;
        MediusSetGameListFilterRequest filt;
        MediusSessionRequest ver;

        data_0066A5E0 = index;
        SetSessionKey(upd.SessionKey);
        upd.UserAction = 1;
        CheckMediusError(MediusUpdateClientState(&upd));
        if (!g_bJoiningRandomGame) {
            theLobby.ChangeState(LOBBY_ACTIVE);
            LobbyEvent("MP_JOIN_ROOM", 0);
        } else {
            theLobby.ChangeState(JOIN_GAME);
            g_bJoiningRandomGame = 0;
        }
        memset(&filt, 0, sizeof(filt));
        SetSessionKey(filt.SessionKey);
        filt.FilterField = 10;
        CheckMediusError(MediusSetGameListFilter(&filt, func_002EFE10));
        memset(&ver, 0, sizeof(ver));
        SetSessionKey(ver.SessionKey);
        CheckMediusError(MediusVersionServer(&ver, func_002EFE00));
        if (theLobby.uiv_MediusChatRecvVar)
            UpdateChatDisplay();
    } else {
        theLobby.ChangeState(CONNECT_ERROR);
    }
}

/* the game's rules as flags: respawns allowed 2, friendly fire 1 (SOCOM II's; OVL1 0x2EF9F0) */
extern "C" int func_002EF9F0()
{
    int flags = 0;
    CValve *respawn = CValve::GetByName("mp_allow_respawn");
    CValve *ff = CValve::Create("mp_friendly_fire", VALVE_TYPE_1);

    if (respawn->m_value)
        flags |= 2;
    if (ff->m_value)
        flags |= 1;
    return flags;
}

/* a player's report to the server: our stats' record in the game joined (the SDK's; 0x118 bytes) */
struct MediusPlayerReport {
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];   // 0x0
    int MediusWorldID;   // 0x14
    char Stats[256];   // 0x18
};   // 0x118

extern "C" {
MediusErrorCode MediusSendPlayerReport(MediusPlayerReport *rep);   /* MediusSendPlayerReport */
bool func_002C31A0();   /* we play on the game's first side (the states lane's) */
bool func_002C3080();   /* or on its second (the states lane's) */
void func_002FAB80(char *stats);   /* our stats as a text (the lobby unit's) */
extern char data_0066A648[256];   /* our stats' text kept */
extern GamePlayerStats data_0066A750;   /* and unpacked (OVL2 bss) */
extern GamePlayerStats data_0044F8E0;   /* the stats last reported */
extern int data_0044F9C8;   /* and when */
}

/* a game world's report to the server (the SDK's MediusWorldReport; 0x180 bytes) */
struct MediusWorldReport {
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];   // 0x0
    int MediusWorldID;   // 0x14
    int PlayerCount;   // 0x18
    char GameName[64];   // 0x1c
    char GameStats[256];   // 0x5c: the game's rules
    int MinPlayers;   // 0x15c
    int MaxPlayers;   // 0x160
    int GameLevel;   // 0x164: the mission's number
    int PlayerSkillLevel;   // 0x168
    int RulesSet;   // 0x16c
    int GenericField1;   // 0x170
    int GenericField2;   // 0x174
    int GenericField3;   // 0x178
    int WorldStatus;   // 0x17c: 3 locked, 1 staging, 2 active
};   // 0x180

/* the mission running (theMission's name, "mpNN", and its rules' record: the LAN lobby's) */
struct LANMissionRules {
    unsigned char _0[0x113];
    unsigned char m_113;   // 0x113
};

extern "C" {
MediusErrorCode MediusSendWorldReport(MediusWorldReport *rep);   /* MediusSendWorldReport */
void func_00621F90(int conn, int clients, void (*cb)(int, int));   /* (the DME's NetPing) */
void func_002EFA60(int, int);   /* its callback */
void func_002F7C40(void *rules);   /* the game's rules filled in (the lobby unit's) */
int func_002E99F0();   /* (the clan unit's) */
extern int data_0066A838;   /* our performance figure */
extern char *data_004364FC;
extern LANMissionRules *data_00437CE8;
}

/* the running mission's number, 0 none (the LAN lobby's inline: the && ternary over the pointer in a
 * local -- round 18, zNetLANLobby.cpp's comment) */
inline int MissionNumber()
{
    const char *p = data_004364FC;

    return p && *p ? atoi(p + 2) : 0;
}

/* the game's world reported to the server, the time to the next report returned -- 25 seconds,
 * 5 with no mission: its name (the lobby's game-name variable, else "testgame"), rules, players'
 * and spectators' room, the rules' flags and mission, our three figures and its state, the game's
 * clients pinged (SOCOM 1's report, inlined in its READY_TO_CONNECT tick; OVL1 0x2EF590). Byte-exact
 * since round 18: the mission number is ONE zero block reached from both tests (the && ternary over
 * the pointer in a local, MissionNumber above), which the nested ternaries and if-chains of rounds
 * 13-17 (34 spellings) never gave. The report's GenericField1 is LobbyWorldID (0x66A538: the source
 * said 0x66A5C8 until round 15, a wrong reference the instruction count did not show) */
extern "C" int func_002EF590(CValve *locked)
{
    MediusWorldReport rep;
    int delay = 25000;
    const char *name = "testgame";
    int maxPlayers = 8;

    memset(&rep, 0, sizeof(rep));
    SetSessionKey(rep.SessionKey);
    CValve *players = CValve::GetByName("MaxPlayersValve");
    CValve *spectators = CValve::GetByName("MaxSpectatorsValve");
    CUIVariable *var = theLobby.uiv_GameNameVar;

    if (var && var->type == 3)
        name = ZString(var);
    if (players)
        maxPlayers = players->m_value;
    if (spectators)
        maxPlayers += spectators->m_value;
    func_002F7C40(rep.GameStats);
    SetSessionKey(rep.SessionKey);
    rep.MediusWorldID = MediusWorldID;
    rep.PlayerCount = 0;
    if (name)
        memcpy(rep.GameName, name, sizeof(rep.GameName));
    rep.MinPlayers = 2;
    rep.MaxPlayers = maxPlayers;
    rep.RulesSet = func_002EF9F0();
    rep.PlayerSkillLevel = 0;
    rep.GameLevel = MissionNumber();
    if (!rep.GameLevel)
        delay = 5000;
    rep.GenericField1 = LobbyWorldID;
    rep.GenericField2 = data_0066A838;
    rep.GenericField3 = func_002E99F0();
    if (locked->m_value)
        rep.WorldStatus = 3;
    else if (data_00437CE8->m_113 == 1)
        rep.WorldStatus = 1;
    else
        rep.WorldStatus = 2;
    func_00621F90(theNetwork.m_GameConnectionIndex, 0xFFFF, func_002EFA60);
    MediusSendWorldReport(&rep);
    return delay;
}

/* our stats reported to the server for the game we are in: our side put in them first (0 the
 * first, 1 the second, 2 none), the report sent when they changed -- the time played, two counts
 * and byte 1's low bits not counted -- or five minutes after the last (SOCOM II's; OVL1 0x2EF7F0:
 * the READY_TO_CONNECT tick's) */
extern "C" void func_002EF7F0()
{
    int now = theNetwork.zNetGetLocalTime();

    if (func_002C31A0())
        data_0066A750.m_side = STATS_SEALS;
    else if (func_002C3080())
        data_0066A750.m_side = STATS_TERRORISTS;
    else
        data_0066A750.m_side = STATS_SPECTATOR;
    func_002FAB80(data_0066A648);
    MediusPlayerReport rep;

    memset(&rep, 0, sizeof(rep));
    SetSessionKey(rep.SessionKey);
    rep.MediusWorldID = MediusWorldID;
    if (data_0066A648)
        memcpy(rep.Stats, data_0066A648, sizeof(rep.Stats));
    bool changed = false;
    GamePlayerStats old = data_0044F8E0;
    GamePlayerStats *stats = (GamePlayerStats *)rep.Stats;

    old.m_c = stats->m_c;
    old.m_time = stats->m_time;
    old.m_1 = stats->m_1;
    if (memcmp(stats, &old, sizeof(old)))
        changed = true;
    if (changed || data_0044F9C8 + 300000 < now) {
        MediusSendPlayerReport(&rep);
        memcpy(&data_0044F8E0, stats, sizeof(data_0044F8E0));
        data_0044F9C8 = now;
    }
}

/* READY_TO_CONNECT: the game's lobby state -- set up on entry, its details sent to the lobby
 * from time to time, torn down on exit (SOCOM 1's; OVL1 0x2EF3C0) */
void LobbyReadyToConnectStateHandler(LOBBY_STATE_FLAG flag)
{
    switch (flag) {
    case REGISTER:
        break;
    case ENTRY:
        func_002F7690();
        data_0044F9D0 = CValve::Create("mp_locked_game", VALVE_TYPE_1);
        theLobby.ReleaseUIResources();
        func_002F4FC0(0);
        LobbyEvent("MP_MEDIUS_READY_TO_CONNECT", 0);
        break;
    case TICK: {
        int delay = 25000;

        if (theNetwork.zNetGetLocalTime() > data_0044F9D8 || data_0066A5F8) {
            func_002EF7F0();
            CountGamePlayers();
            if (theNetwork.m_bSessionMaster)
                delay = func_002EF590(data_0044F9D0);
            data_0066A5F8 = 0;
            data_0044F9D8 = theNetwork.zNetGetLocalTime() + delay;
        }
        break;
    }
    case EXIT: {
        MediusUpdateClientStateRequest upd;

        theLobby.CreateUIResources();
        SetSessionKey(upd.SessionKey);
        upd.UserAction = 2;
        CheckMediusError(MediusUpdateClientState(&upd));
        func_002F4FC0(1);
        data_0044F9D0 = 0;
        CValve::Create("mp_persistent", VALVE_TYPE_1)->SetValue(1);
        break;
    }
    case UNREGISTER:
        break;
    }
}

/* LOBBY_SERVER_CONNECT: the room's lobby server connected to (SOCOM 1's; OVL1 0x2EFA90) */
void LobbyServerConnectStateHandler(LOBBY_STATE_FLAG flag)
{
    switch (flag) {
    case REGISTER:
        break;
    case ENTRY: {
        MediusConnectParams params;

        func_00300680();
        memset(&params, 0, sizeof(params));
        func_002F3FF0((ChatSlot *)&data_0066A540);
        params.ConnectInfo = data_0066A540;
        params.Callback = cbLobbyServerConnect;
        params.BufferSize = 256;
        params._f8 = 0;
        params._fc = 0;
        data_0044F8D0 = data_004A44A0;
        if (!func_00300740(1, &params))
            theLobby.ChangeState(CONNECT_ERROR);
        break;
    }
    case TICK:
        break;
    case EXIT:
        break;
    case UNREGISTER:
        break;
    }
}

/* the account logged in: its lobby server kept for the room, the menus told otherwise (SOCOM
 * 1's; OVL1 0x2EEFA0) */
void cbAccountLogin(int, int, int, MediusAccountLoginResponse *resp)
{
    if (CheckMediusCallbackStatus("AccountLogin", resp->StatusCode)) {
        AccountInfo.AccountID = resp->AccountID;
        data_0066A840 = resp->ConnectInfo;
        data_0066A540 = data_0066A840;
        CopyField(data_0044F890, resp->ConnectInfo.ServerKey, sizeof(data_0044F890));
        theLobby.ChangeState(LOBBY_SERVER_CONNECT);
        data_0066A8E0 = 1;
        CZOnlineLobby::PasswordSecurity();
        theLobby.m_loginTime = data_004A44A0;
    } else {
        switch (resp->StatusCode) {
        case (MediusCallbackStatus)-978:
            LobbyEvent("MP_INVALID_PASSWORD", 0);
            break;
        case (MediusCallbackStatus)-997:
            LobbyEvent("MP_PLAYER_LOGGED_IN", 0);
            break;
        case (MediusCallbackStatus)-998:
            theLobby.ChangeState(LOGIN_FAILURE);
            break;
        default:
            LobbyEvent("MP_MEDIUS_ERROR", "Login Failure");
            break;
        }
    }
}

/* ACCOUNT_LOGIN: the language told, the console's DNAS id posted, the account logged in (SOCOM
 * 1's, which only logged in; OVL1 0x2EEDE0) */
void AccountLoginStateHandler(LOBBY_STATE_FLAG flag)
{
    switch (flag) {
    case REGISTER:
        break;
    case ENTRY: {
        int len;
        MediusAccountLoginRequest login;
        MediusMachineSignaturePost post;
        MediusSetLocalizationParamsRequest loc;

        memset(&loc, 0, sizeof(loc));
        SetSessionKey(loc.SessionKey);
        loc.CharacterEncoding = 2;
        loc.Language = func_002EF340(GetLanguage());
        CheckMediusError(func_0064AB98(&loc, func_002EF3B0));
        unsigned char *id = GetDNASUniqueID(&len);
        if (id && len > 0 && len < 33) {
            memset(&post, 0, sizeof(post));
            memset(&post, 0, sizeof(post));
            SetSessionKey(post.SessionKey);
            post._28 = 0;
            post.Length = len;
            memcpy(post.Signature, id, len);
            func_002F4080(&post, 138);
            CheckMediusError(func_00647EC0(&post));
        }
        memset(&login, 0, sizeof(login));
        SetSessionKey(login.SessionKey);
        CopyFrom(login.AccountName, AccountInfo.AccountName, sizeof(login.AccountName));
        CopyFrom(login.Password, AccountInfo.Password, sizeof(login.Password));
        CheckMediusError(MediusAccountLogin(&login, cbAccountLogin));
        break;
    }
    case TICK:
        break;
    case EXIT:
        break;
    case UNREGISTER:
        break;
    }
}

/* the account just registered logged in: its lobby server kept (SOCOM 1's; OVL1 0x2EEC10) */
void cbAccountLoginAfterRegister(int, int, int, MediusAccountLoginResponse *resp)
{
    if (CheckMediusCallbackStatus("AccountLoginAfterRegister", resp->StatusCode)) {
        AccountInfo.AccountID = resp->AccountID;
        data_0066A540 = resp->ConnectInfo;
        CopyField(data_0044F890, resp->ConnectInfo.ServerKey, sizeof(data_0044F890));
        data_0066A8E0 = 1;
        theLobby.ChangeState(LOBBY_SERVER_CONNECT);
        CZOnlineLobby::PasswordSecurity();
    } else {
        theLobby.ChangeState(REGISTRATION_FAILURE);
    }
}

/* the account just registered logged in (SOCOM 1's; OVL1 0x2EEB50) */
void cbAccountRegistration(int, int, int, MediusAccountRegistrationResponse *resp)
{
    if (CheckMediusCallbackStatus("AccountRegistration", resp->StatusCode)) {
        MediusAccountLoginRequest req;

        memset(&req, 0, sizeof(req));
        SetSessionKey(req.SessionKey);
        CopyFrom(req.AccountName, AccountInfo.AccountName, sizeof(req.AccountName));
        CopyFrom(req.Password, AccountInfo.Password, sizeof(req.Password));
        CheckMediusError(MediusAccountLogin(&req, cbAccountLoginAfterRegister));
    } else {
        theLobby.ChangeState(REGISTRATION_FAILURE);
    }
}

/* the account's name passed the filter: registered (SOCOM 1's; OVL1 0x2EEA70) */
void cbFilterAcctName(int, int, int, MediusTextFilterResponse *resp)
{
    if (resp->Result >= 0) {
        MediusAccountRegistrationRequest req;

        memset(&req, 0, sizeof(req));
        SetSessionKey(req.SessionKey);
        CopyFrom(req.AccountName, AccountInfo.AccountName, sizeof(req.AccountName));
        CopyFrom(req.Password, AccountInfo.Password, sizeof(req.Password));
        req.AccountType = 1;
        CheckMediusError(MediusAccountRegistration(&req, cbAccountRegistration));
    } else {
        LobbyEvent("MP_FILTER_FAILURE", 0);
        theLobby.ChangeState(REGISTRATION_FAILURE);
    }
}

/* ACCOUNT_REGISTRATION: the account's name filtered first (SOCOM 1's; OVL1 0x2EE9B0) */
void AccountRegistrationStateHandler(LOBBY_STATE_FLAG flag)
{
    switch (flag) {
    case REGISTER:
        break;
    case ENTRY: {
        MediusTextFilterRequest req;

        memset(&req, 0, sizeof(req));
        SetSessionKey(req.SessionKey);
        req.TextFilterType = 0;
        CopyFrom(req.Text, AccountInfo.AccountName, sizeof(AccountInfo.AccountName));
        CheckMediusError(MediusTextFilter(&req, cbFilterAcctName));
        break;
    }
    case TICK:
        break;
    case EXIT:
        break;
    case UNREGISTER:
        break;
    }
}

/* SESSION_BEGIN: a session begun, unless one is, and the server's version asked (SOCOM 1's;
 * OVL1 0x2EE830) */
void SessionBeginStateHandler(LOBBY_STATE_FLAG flag)
{
    switch (flag) {
    case REGISTER:
        break;
    case ENTRY:
        if (!SessionKey[0]) {
            MediusSessionBeginRequest begin;
            MediusSessionRequest req;

            memcpy(begin.MessageID, "SessionBegin", MEDIUS_MESSAGEID_MAXLEN);
            begin.ConnectionClass = 1;
            CheckMediusError(MediusSessionBegin(&begin, func_002EE910));
            memset(&req, 0, sizeof(req));
            SetSessionKey(req.SessionKey);
            CheckMediusError(MediusVersionServer(&req, func_002EFE00));
        }
        break;
    case TICK:
        break;
    case EXIT:
        break;
    case UNREGISTER:
        break;
    }
}

/* END_SESSION: the session ended, or, with none, back to the root (SOCOM 1's, which also
 * disconnected; OVL1 0x2EE6E0) */
void EndSessionStateHandler(LOBBY_STATE_FLAG flag)
{
    switch (flag) {
    case REGISTER:
        break;
    case ENTRY:
        if (SessionKey[0]) {
            MediusSessionRequest req;

            memset(&req, 0, sizeof(req));
            SetSessionKey(req.SessionKey);
            CheckMediusError(MediusSessionEnd(&req, func_002EE7B0));
        } else {
            theLobby.ChangeState(ROOT_LOBBY_STATE);
        }
        break;
    case TICK:
        break;
    case EXIT:
        break;
    case UNREGISTER:
        break;
    }
}

/* LOBBY_ACTIVE: the player's info asked for on entry, then every five minutes (SOCOM 1's, which
 * asked for the stats and the policy; OVL1 0x2EE540) */
void LobbyActiveStateHandler(LOBBY_STATE_FLAG flag)
{
    switch (flag) {
    case REGISTER:
        break;
    case ENTRY:
        func_002FA870(&theLobby);
        data_0044F9E0 = theNetwork.zNetGetLocalTime() + 120000;
        break;
    case TICK:
        if (theNetwork.zNetGetLocalTime() > data_0044F9E0) {
            func_002FAA80(&theLobby);
            data_0044F9E0 = theNetwork.zNetGetLocalTime() + 300000;
        }
        break;
    case EXIT:
        break;
    case UNREGISTER:
        break;
    }
}

/* CONNECT_ERROR: the connection closed and the menus told (SOCOM 1's was empty; OVL1 0x2EE610) */
void ConnectErrorStateHandler(LOBBY_STATE_FLAG flag)
{
    switch (flag) {
    case REGISTER:
        break;
    case ENTRY:
        func_00300680();
        LobbyEvent("MP_MEDIUS_ERROR", GetString(450));
        break;
    case TICK:
        break;
    case EXIT:
        break;
    case UNREGISTER:
        break;
    }
}

/* a pair of 24-byte slots, a word at 4 marking one in use (only what is read here) */
struct ChatSlot {
    int m_0;
    int m_used;   // 0x4
    unsigned char _8[0x18 - 0x8];
};

/* the text filter's answer awaited: the caller's callback and its data (SOCOM 1's Censor kept
 * them likewise) */
typedef void (*CensorCallback)(const char *text, void *data);
/* a chat line's colour: the mission's message type (fts_state.h's MSGTYPE, the same three values --
 * 0 white, 1 yellow, 5 red; a byte enum: assigned with li, compared through andi; the enumerators'
 * names the chat's, ours) */
enum MSGTYPE {
    CHAT_WHITE = 0,
    CHAT_CLAN = 1,
    CHAT_PRIVATE = 5
};

/* the mission's ring of messages for the player (fts_mission.cpp's class: theMission + 0x11B0) */
class CMessageQueue {
public:
    void Enqueue(const char *text, MSGTYPE type, const char *name);
};

/* the chat turned on or off (SOCOM 1's MediusChatToggle; int-sized) */
enum MediusChatToggle {
    MediusChatOn = 0,
    MediusChatOff = 1,
    MediusChatToggle_Extent = 0x7FFFFFFF
};

struct MediusChatToggleRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0x0
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];   // 0x15
    MediusChatToggle ChatToggle;   // 0x28
};   // 0x2c

/* a text to one account */
struct MediusSendDirectMessageRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0x0
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];   // 0x15
    int MessageType;   // 0x28
    int TargetAccountID;   // 0x2c
    char Message[64];   // 0x30
};   // 0x70

/* a player's info asked for (SOCOM 1's MediusGetPlayerInfo) */
struct MediusPlayerInfoRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0x0
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];   // 0x15
    int AccountID;   // 0x28
};   // 0x2c

/* the player's info (the SDK's MediusPlayerInfoResponse: the account's stats as text at 0x48) */
struct MediusPlayerInfoResponse {
    MEDIUS_RESPONSE_HEAD
    char AccountName[32];   // 0x1c
    int ApplicationID;   // 0x3c
    int PlayerStatus;   // 0x40
    int ConnectionClass;   // 0x44
    char Stats[256];   // 0x48
};

/* a chat line (SOCOM 1's MediusChatMessage: 1 private, the target looked up first) */
struct MediusChatMessage {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0x0
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];   // 0x15
    int MessageType;   // 0x28
    int TargetID;   // 0x2c
    char Message[64];   // 0x30
};   // 0x70

/* a player looked up by name (SOCOM 1's MediusFindPlayer: 1, by name) */
struct MediusFindPlayerRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0x0
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];   // 0x15
    int SearchType;   // 0x28
    int ID;   // 0x2c
    char Name[32];   // 0x30
};   // 0x50

/* the player found (the friends unit's layout) */
struct MediusFindPlayerResponse {
    MEDIUS_RESPONSE_HEAD
    int ApplicationID;   // 0x1c
    char ApplicationName[32];   // 0x20
    int ApplicationType;   // 0x40
    int MediusWorldID;   // 0x44
    int AccountID;   // 0x48
    char AccountName[32];   // 0x4c
    char EndOfList;   // 0x6c
};

/* a queue of chat lines (the name ours): four blank lines pushed and popped at the start, so its
 * first block is there before the first line -- the constructor is too tall to inline whole into
 * the initialiser, which calls the queue's members out of line (0x2F5EE0, 0x2F5EC0, 0x2F5ED0) */
class CChatQueue : public std::queue<MediusChatMessage> {
public:
    CChatQueue()
    {
        MediusChatMessage msg;

        for (int i = 0; i < 4; i++)
            push(msg);
        while (!empty())
            pop();
    }
};

/* the unit's two queues, built by its initialiser 0x4020E0, their destructor 0x2F5E50 registered */
CChatQueue data_0044FA20;   /* the public lines waiting for the filter */
CChatQueue data_0044FA50;   /* the private lines waiting for their player */

void cbGetPlayerInfo(int, int, int, MediusPlayerInfoResponse *);   /* the player's info shown */

extern "C" {
MediusErrorCode MediusFindPlayer(MediusFindPlayerRequest *req,
                              void (*cb)(int, int, int, MediusFindPlayerResponse *));   /* MediusFindPlayer */
MediusErrorCode MediusGetPlayerInfo(MediusPlayerInfoRequest *req,
                              void (*cb)(int, int, int, MediusPlayerInfoResponse *));   /* MediusGetPlayerInfo */
void func_00300670();   /* the Medius library closed */
void func_0030D770(CZNetwork *net);   /* the DME left (CZNetwork's) */
void func_0030E3A0(CZNetwork *net, int);   /* and closed (CZNetwork's) */
MediusErrorCode MediusSendChatMessage(MediusSendDirectMessageRequest *req);   /* the direct message sent */
extern char data_00437690[];   /* theMission's message ring (CMission::m_netQueue) */
}

extern "C" {
extern CensorCallback data_0044FA00;
extern void *data_0044FA08;
}

void cbFindChatPlayerID(int, int, int, MediusFindPlayerResponse *resp);   /* the private line's player found */
void cbFilterChat(const char *text, void *data);   /* the public line filtered: sent */

/* a chat line sent: queued, then a private one's player looked up, a public one filtered first
 * (SOCOM 1's SendChat filtered every line; OVL1 0x2F4860). The filter is Censor, inlined */
bool CZOnlineLobby::SendChat(const char *text, const char *to)
{
    if (!m_isopen)
        return false;
    MediusChatMessage msg;

    memset(&msg, 0, sizeof(msg));
    SetSessionKey(msg.SessionKey);
    if (text)
        memcpy(msg.Message, text, sizeof(msg.Message));
    msg.Message[sizeof(msg.Message) - 1] = 0;
    if (to) {
        msg.MessageType = 1;
        msg.TargetID = -1;
        data_0044FA50.push(msg);
        MediusFindPlayerRequest find;

        memset(&find, 0, sizeof(find));
        SetSessionKey(find.SessionKey);
        find.ID = 0;
        find.SearchType = 1;
        if (to)
            memcpy(find.Name, to, sizeof(find.Name));
        CheckMediusError(MediusFindPlayer(&find, cbFindChatPlayerID));
    } else {
        msg.TargetID = -1;
        msg.MessageType = 0;
        data_0044FA20.push(msg);
        Censor(msg.Message, cbFilterChat, &msg);
    }
    return true;
}

/* the room's players in name order (the sort's comparator; OVL1 0x2F1950, the name ours) */
static bool PlayerNameLess(MediusLobbyWorldPlayerListResponse &a, MediusLobbyWorldPlayerListResponse &b)
{
    return strcmp(a.AccountName, b.AccountName) <= 0;
}

/* the room's players shown: sorted by name, each a row of its account id, its name, its state --
 * "Disconnecting", "Logging In", "In Lobby" in the lobby's colour or the game it is in -- and its
 * clan's tag (MessageID: "[tag]", or 1 in its first byte for none) in the tag colour, handed to
 * the room player list (SOCOM 1's UpdateRoomPlayerListDisplay, grown: the sort and the tags SOCOM
 * II's; OVL1 0x2F1670: the players' pages and their infos call it;
 * std::sort's instance follows it, 0x2F1980-0x2F3EEF with its two helpers) */
void UpdateRoomPlayerListDisplay()
{
    BriefingRoom *room = data_0044F568.FindRoom(ZString(theLobby.uiv_LobbyWorldNameVar));

    if (room)
        std::sort(room->m_players.begin(), room->m_players.end(), PlayerNameLess);
    PNT3D tagColor;
    PNT3D lobbyColor;

    tagColor.x = 128.0f;
    tagColor.y = 64.0f;
    tagColor.z = 32.0f;
    lobbyColor.x = 91.0f;
    lobbyColor.y = 72.0f;
    lobbyColor.z = 36.0f;
    CRdrEditor ed;

    ed.push(0);
    if (room) {
        for (std::vector<MediusLobbyWorldPlayerListResponse>::iterator i = room->m_players.begin(); i != room->m_players.end(); ++i) {
            char id[32];

            ed.push(0);
            sprintf(id, "%d", (*i).AccountID);
            ed.addstringnode(id);
            ed.addstringnode((*i).AccountName);
            switch ((*i).OnlineState.ConnectStatus) {
            case 0:
                ed.addstringnode("Disconnecting");
                break;
            case 1:
                ed.addstringnode("Logging In");
                break;
            case 2:
                ed.push(0);
                ed.addSTRING("VALUE", "In Lobby");
                ed.addPNT3D("COLOR", lobbyColor);
                ed.pop();
                break;
            case 3:
                ed.addstringnode((*i).OnlineState.GameName);
                break;
            }
            ed.push(0);
            if ((*i).MessageID[0] != 1)
                ed.addSTRING("VALUE", (*i).MessageID);
            else
                ed.addSTRING("VALUE", "");
            ed.addPNT3D("COLOR", tagColor);
            ed.pop();
            ed.pop();
        }
    }
    ed.pop();
    theLobby.uiv_RoomPlayerList->Set(&ed);
}

/* the room's players asked for (the SDK's MediusLobbyWorldPlayerListRequest: no session key) */
struct MediusLobbyWorldPlayerListRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0x0
    int MediusWorldID;   // 0x18
};   // 0x1c
extern "C" {
extern unsigned char data_0044F9F0;   /* the room's player list being asked for */
extern float data_0044F9F8;   /* when it may next be asked */
MediusErrorCode func_0064AF78(MediusLobbyWorldPlayerListRequest *req,
                              void (*cb)(int, int, int, MediusLobbyWorldPlayerListResponse *));
}
void cbRoomPlayerList(int, int, int, MediusLobbyWorldPlayerListResponse *);   /* a page of them (below) */

/* a player's whereabouts as the page's update copies them -- member by member (three words, then
 * each 64-byte name in a two-byte loop): the copy of a class with a user-declared constructor, where
 * the SDK's MediusPlayerOnlineState is a plain struct copied as words (a layout-identical stand-in,
 * the name ours) */
struct PlayerWhereabouts {
    PlayerWhereabouts() {}

    int ConnectStatus;   // 0x0
    int MediusLobbyWorldID;   // 0x4
    int MediusGameWorldID;   // 0x8
    char LobbyName[64];   // 0xc
    char GameName[64];   // 0x4c
};   // 0x8c

/* a page of the room's players come in: a player already listed has its whereabouts brought up to
 * date, a new one is added; on the last page the players not seen again (state 0, cleared by the
 * request) are dropped and the list shown; a failed answer shows the list as it is -- either way
 * the next request may go (SOCOM 1's cbRoomPlayerList, which only added; OVL1 0x2F13F0: SOCOM 1's
 * mangled name, cbRoomPlayerList__FiiiP34MediusLobbyWorldPlayerListResponse, names the record).
 * BYTE-EXACT since the online lane's round 22 (open since round 16 at 34): the last page's walk has
 * a second iterator, `next`, assigned with i from begin() and never address-taken -- the register
 * (v0) the target keeps for the entry test and the step -- erase's result or the step stored into i
 * once at the arms' join, i itself read for the dereference, erase's copy and the bottom test
 * (FriendInvitationVector::Remove 0x301CF0, the same walk with the step ahead of the test). On one
 * iterator (`i->`, `(*i).`, an initialised or for-scoped iterator, `i++`, `i = i + 1`, a ternary of
 * the two arms, `!(i == end())`) MW reloaded i for the test and stepped the dereference's load */
void cbRoomPlayerList(int, int, int, MediusLobbyWorldPlayerListResponse *resp)
{
    if (resp->StatusCode == 0) {
        BriefingRoom *room = data_0044F568.FindRoom(ZString(theLobby.uiv_LobbyWorldNameVar));

        if (!room)
            return;
        bool found = false;

        for (std::vector<MediusLobbyWorldPlayerListResponse>::iterator i = room->m_players.begin(); i != room->m_players.end(); ++i) {
            if (resp->AccountID == (*i).AccountID) {
                *(PlayerWhereabouts *)&(*i).OnlineState = *(PlayerWhereabouts *)&resp->OnlineState;
                found = true;
                break;
            }
        }
        if (!found)
            room->m_players.push_back(*resp);
        if (resp->EndOfList) {
            std::vector<MediusLobbyWorldPlayerListResponse>::iterator i;
            std::vector<MediusLobbyWorldPlayerListResponse>::iterator next;

            i = next = room->m_players.begin();
            while (i != room->m_players.end()) {
                if (i->OnlineState.ConnectStatus == 0)
                    next = room->m_players.erase(i);
                else
                    ++next;
                i = next;
            }
            UpdateRoomPlayerListDisplay();
            data_0044F9F0 = 0;
        }
    } else {
        UpdateRoomPlayerListDisplay();
        data_0044F9F0 = 0;
    }
}

extern "C" {
/* the room's players asked for again, at most every ten seconds and one answer at a time: each
 * player's state cleared, the page told it is waiting while it is empty (SOCOM II's; OVL1
 * 0x2F1240: 0x2F11F0, 'RefreshRoomPlayerList', calls it) */
void func_002F1240(C2D *page)
{
    if (data_0044F9F0)
        return;
    float now = data_004A44A0;

    if (now < data_0044F9F8)
        return;
    data_0044F9F8 = 10.0f + now;
    BriefingRoom *room = data_0044F568.FindRoom(ZString(theLobby.uiv_LobbyWorldNameVar));

    if (!room)
        return;
    data_0044F9F0 = 1;
    for (std::vector<MediusLobbyWorldPlayerListResponse>::iterator i = room->m_players.begin(); i != room->m_players.end(); ++i)
        (*i).OnlineState.ConnectStatus = 0;
    if (!theLobby.uiv_RoomPlayerList->Count()) {
        CRdrEditor ed;

        ed.push(0);
        ed.addstringnode("Retrieving player list from server...");
        ed.pop();
        theLobby.uiv_RoomPlayerList->Set(&ed);
    }
    MediusLobbyWorldPlayerListRequest req;

    memset(&req, 0, sizeof(req));
    req.MediusWorldID = LobbyWorldID;
    CheckMediusError(func_0064AF78(&req, cbRoomPlayerList));
}
}


/* a room player's info come in (the 'PlayerListInfo' requests 0x2F0EA0 makes, the account in the
 * message id): the player found in the room we are in, its clan's tag shown in its record's
 * first bytes when it has a clan and the players shown again -- else the record marked (SOCOM
 * II's; OVL1 0x2F10B0) */
extern "C" void func_002F10B0(int, int, int, MediusPlayerInfoResponse *resp)
{
    if (CheckMediusCallbackStatus("PlayerListInfo", resp->StatusCode)) {
        BriefingRoom *room = data_0044F568.FindRoom(ZString(theLobby.uiv_LobbyWorldNameVar));
        GamePlayerStats stats;

        UnpackPlayerStats(&stats, resp->Stats);
        if (room) {
            int id = atoi(resp->MessageID);

            for (std::vector<MediusLobbyWorldPlayerListResponse>::iterator i = room->m_players.begin();
                 i != room->m_players.end(); ++i) {
                if (id == (*i).AccountID) {
                    if (stats.m_clan[0]) {
                        char tag[32];

                        sprintf(tag, "[%s]", stats.m_clan);
                        CopyTo((*i).MessageID, tag, MEDIUS_MESSAGEID_MAXLEN);
                        UpdateRoomPlayerListDisplay();
                    } else {
                        (*i).MessageID[0] = 1;
                    }
                    /* a return, not a break: the nested ifs keep their join, a `b` to the
                     * epilogue (AddFriend's tell); a break is threaded through */
                    return;
                }
            }
        }
    }
}

/* the room player list's box: zui_2d.cpp's CUIListBox, only what is read here (its height, a row's
 * height, the first row shown) -- the class is the 2D family's and no header declares it, so this
 * unit's view of it is a layout-only one */
class CUIListBox {
public:
    /* a row's text (zui_2d.cpp's, 0x374B50; the HUD's round 41 named it) */
    const char *GetItemText(int idx);

    unsigned char _0[0x78];
    int m_height;   // 0x78
    unsigned char _7c[0xac - 0x7c];
    int m_rowheight;   // 0xac (zui_2d.cpp's m_lineHeight)
    unsigned char _b0[0xb8 - 0xb0];
    int m_top;   // 0xb8 (zui_2d.cpp's m_scroll)
};

extern "C" {

/* the info of the room's players the page shows asked for, each one not yet known (its record's
 * first byte still clear) by the account id its row holds -- the request's message id carries the
 * id back (SOCOM II's; OVL1 0x2F0EA0: UIRefreshRoomPlayerList calls it on theLobby) */
bool func_002F0EA0(CZOnlineLobby *lobby, C2D *list)
{
    CUIListBox *page = 0;

    if (!lobby->m_isopen)
        return false;
    if (list)
        page = (CUIListBox *)list;
    if (page) {
        /* the room and the first row numbered first; the bound `top + rows` is the loop
         * optimiser's hoisted temporary, numbered after SetSessionKey's hoisted address, so it
         * takes s0 (a named bound, or rows += top, is numbered early) */
        BriefingRoom *room;
        int top;
        int rows = page->m_height / page->m_rowheight;

        top = page->m_top;
        room = data_0044F568.FindRoom(ZString(theLobby.uiv_LobbyWorldNameVar));
        if (room) {
            for (int row = top; row <= top + rows; row++) {
                const char *s = page->GetItemText(row);

                if (s && s[0]) {
                    int id = atoi(s);

                    for (std::vector<MediusLobbyWorldPlayerListResponse>::iterator i = room->m_players.begin();
                         i != room->m_players.end(); ++i) {
                        if (id == (*i).AccountID && !(*i).MessageID[0]) {
                            MediusPlayerInfoRequest req;

                            memset(&req, 0, sizeof(req));
                            SetSessionKey(req.SessionKey);
                            sprintf(req.MessageID, "%d", id);
                            req.AccountID = id;
                            MediusGetPlayerInfo(&req, func_002F10B0);
                        }
                    }
                }
            }
        }
    }
    return true;
}
}

extern "C" {
void func_002F3EE0(GamePlayerStats *stats);   /* the stats put in order (the unit's: an 8-byte row) */
}

/* a player's info come in, made the menu's three pages: the name (with the clan's tag), the
 * whereabouts and the ladder figures; the online state from the list the request's message id names
 * (the ladder, the clan, the friends, else the room's players); a failure says so (SOCOM II's; OVL1
 * 0x2F0020: MediusGetPlayerInfo's callback, func_002EFE50 asks for it -- named as SOCOM 1's callbacks
 * are, after the request: its own MediusGetPlayerInfo callback is cbGetPlayerStats, our stats). Round 18, byte-exact: the
 * clan's and the friend's whereabouts read through a reference local (the target keeps the member's
 * address, c + 0x140, in a register for the copy's two arrays: written straight the offsets fold, as
 * the room player's and the ladder's do); the room player looked up if-assign (the zero in the null
 * test's delay slot: a ternary branches over it) */
void cbGetPlayerInfo(int, int, int, MediusPlayerInfoResponse *resp)
{
    PNT3D color;
    PlayerWhereabouts where;
    char text[128];
    GamePlayerStats stats;
    char name[256];

    color.x = 91.0f;
    color.y = 72.0f;
    color.z = 36.0f;
    CUIVariable *info = theUIVarManager.Add("PLAYER_INFO_VAR1", 0, UIVL_TEMPORARY);
    CUIVariable *figures = theUIVarManager.Add("PLAYER_INFO_VAR2", 0, UIVL_TEMPORARY);
    CUIVariable *desc = theUIVarManager.Add("PLAYER_INFO_DESCRIPTION", 0, UIVL_TEMPORARY);

    if (resp->StatusCode == 0) {
        memset(&where, 0, sizeof(where));
        UnpackPlayerStats(&stats, resp->Stats);
        if (!strcmp(resp->MessageID, "LADDER")) {
            LadderItem *l = LadderLeaders.Find(resp->AccountName);

            if (l) {
                where = *(PlayerWhereabouts *)&l->m_info;
                stats.m_8 = l->m_position;
            }
        } else if (!strcmp(resp->MessageID, "CLAN")) {
            ClanMemberItem *c = ClanMemberVec.Find(resp->AccountName);

            if (c) {
                const MediusPlayerOnlineState &s = c->OnlineState;

                where = *(PlayerWhereabouts *)&s;
                stats.m_8 = c->LadderPosition;
            }
        } else if (!strcmp(resp->MessageID, "FRIEND")) {
            FriendItem *f = FriendVec.Find(resp->AccountName);

            if (f) {
                const MediusPlayerOnlineState &s = f->OnlineState;

                where = *(PlayerWhereabouts *)&s;
            }
        } else {
            BriefingRoom *room = data_0044F568.FindRoom(ZString(theLobby.uiv_LobbyWorldNameVar));
            MediusLobbyWorldPlayerListResponse *p = 0;

            if (room)
                p = room->FindPlayer(resp->AccountName);

            if (p)
                where = *(PlayerWhereabouts *)&p->OnlineState;
        }
        func_002F3EE0(&stats);
        if (stats.m_clan[0])
            sprintf(name, "#C\x80@ [%s] #CSH$%s", stats.m_clan, resp->AccountName);
        else
            sprintf(name, "%s", resp->AccountName);
        *theUIVarManager.Add("PLAYER_INFO_NAME_VAR", 0, UIVL_TEMPORARY) = name;
        *theUIVarManager.Add("PLAYER_INFO_CHAT_NAME_VAR", 0, UIVL_TEMPORARY) = resp->AccountName;

        CRdrEditor page;
        CRdrEditor numbers;
        CRdrEditor about;

        page.push(0);
        page.push(0);
        page.addstringnode(" ");
        page.addstringnode(" ");
        page.pop();
        page.push(0);
        page.addstringnode(" ");
        page.addstringnode(" ");
        page.addintnode(theLobby.GetRankLevel(stats.m_8));
        page.pop();
        page.push(0);
        page.addstringnode(" ");
        page.addstringnode(" ");
        page.pop();
        page.push(0);
        sprintf(text, "%s %s", stats.m_40, stats.m_2_7 ? "(Leader)" : "");
        page.addstringnode(text);
        page.addstringnode(" ");
        page.pop();
        page.push(0);
        sprintf(text, "%d", stats.m_8);
        page.addstringnode("LADDER POSITION:");
        page.addstringnode(text);
        page.pop();
        page.push(0);
        sprintf(text, "%d", stats.m_rating);
        page.addstringnode("SCORE:");
        page.addstringnode(text);
        page.pop();
        page.push(0);
        sprintf(text, "%d", stats.m_games);
        page.addstringnode("TOTAL # OF GAMES:");
        page.addstringnode(text);
        page.pop();
        page.push(0);
        sprintf(text, "%d", stats.m_14);
        page.addstringnode("ROUNDS WON:");
        page.addstringnode(text);
        page.pop();
        page.push(0);
        sprintf(text, "%d", stats.m_18);
        page.addstringnode("ROUNDS LOST:");
        page.addstringnode(text);
        page.pop();
        page.push(0);
        page.addstringnode(" ");
        page.addstringnode(" ");
        page.pop();
        page.push(0);
        page.addstringnode(" ");
        page.addstringnode(" ");
        page.pop();
        page.push(0);
        page.addstringnode(" ");
        page.addstringnode(" ");
        page.pop();
        page.push(0);
        sprintf(text, "%s", stats.m_town);
        page.addstringnode("HOMETOWN:");
        page.addstringnode(text);
        page.pop();
        if (where.ConnectStatus) {
            page.push(0);
            sprintf(text, "%d min.", stats.m_c);
            page.addstringnode("ONLINE:");
            page.addstringnode(text);
            page.pop();
            page.push(0);
            if (!strncmp(where.LobbyName, "Medius", 6))
                sprintf(text, "MAIN MENU");
            else
                sprintf(text, "%s", where.LobbyName);
            page.addstringnode("BRIEFING ROOM:");
            page.addstringnode(text);
            page.pop();
            page.push(0);
            page.addstringnode("GAME:");
            switch (where.ConnectStatus) {
            case 0:
                page.addstringnode("Disconnecting");
                break;
            case 1:
                page.addstringnode("Logging In");
                break;
            case 2:
                page.push(0);
                page.addSTRING("VALUE", "In Lobby");
                page.addPNT3D("COLOR", color);
                page.pop();
                break;
            case 3:
                page.addstringnode(where.GameName);
                break;
            default:
                page.addstringnode(" ");
                break;
            }
            page.pop();
        } else {
            page.push(0);
            page.addstringnode("NOT CURRENTLY LOGGED IN\n");
            page.addstringnode(" ");
            page.pop();
        }
        page.pop();
        info->Set(&page);
        about.push(0);
        about.push(0);
        sprintf(text, "%s", stats.m_desc);
        about.addstringnode("DESCRIPTION:\n");
        about.addstringnode(text);
        about.pop();
        about.pop();
        desc->Set(&about);
        numbers.push(0);
        numbers.push(0);
        sprintf(text, "%d", stats.m_kills);
        numbers.addstringnode("KILLS:");
        numbers.addstringnode(text);
        numbers.pop();
        numbers.push(0);
        sprintf(text, "%d", stats.m_deaths);
        numbers.addstringnode("DEATHS:");
        numbers.addstringnode(text);
        numbers.pop();
        numbers.push(0);
        sprintf(text, "%d", stats.m_2c);
        numbers.addstringnode("SUICIDES:");
        numbers.addstringnode(text);
        numbers.pop();
        numbers.push(0);
        sprintf(text, "%d", stats.m_2e);
        numbers.addstringnode("BOMBS PLANTED:");
        numbers.addstringnode(text);
        numbers.pop();
        numbers.push(0);
        sprintf(text, "%d", stats.m_30);
        numbers.addstringnode("BOMBS DEFUSED:");
        numbers.addstringnode(text);
        numbers.pop();
        numbers.push(0);
        sprintf(text, "%d", stats.m_24);
        numbers.addstringnode("HOSTAGES RESCUED:");
        numbers.addstringnode(text);
        numbers.pop();
        numbers.push(0);
        sprintf(text, "%d", stats.m_26);
        numbers.addstringnode("VIPs RESCUED:");
        numbers.addstringnode(text);
        numbers.pop();
        numbers.push(0);
        sprintf(text, "%d", stats.m_28);
        numbers.addstringnode("VIPs KILLED:");
        numbers.addstringnode(text);
        numbers.pop();
        numbers.push(0);
        sprintf(text, "%d", stats.m_20_22);
        numbers.addstringnode("BONUS OBJECTIVES:");
        numbers.addstringnode(text);
        numbers.pop();
        numbers.push(0);
        sprintf(text, "%d %%", stats.m_1);
        numbers.addstringnode("HIT PERCENTAGE:");
        numbers.addstringnode(text);
        numbers.pop();
        numbers.push(0);
        sprintf(text, "%d", stats.m_32);
        numbers.addstringnode("HEAD SHOTS:");
        numbers.addstringnode(text);
        numbers.pop();
        numbers.push(0);
        numbers.addstringnode(" ");
        numbers.addstringnode(" ");
        numbers.pop();
        numbers.push(0);
        sprintf(text, "%d : %.2d", stats.m_time / 3600, stats.m_time % 3600 / 60);
        numbers.addstringnode("TOTAL PLAYING TIME:");
        numbers.addstringnode(text);
        numbers.pop();
        numbers.pop();
        figures->Set(&numbers);
        return;
    }
    *info = "Could not retrieve player information.";
    *figures = " ";
    *desc = " ";
}

/* a player's info asked for, the menu's fields blanked meanwhile; the request's message id names
 * the menu it is for (SOCOM II's; OVL1 0x2EFE50: UIGetPlayerInfo calls it on theLobby) */
extern "C" void func_002EFE50(CZOnlineLobby *lobby, const char *id, int what)
{
    *theUIVarManager.Add("PLAYER_INFO_VAR1", 0, UIVL_TEMPORARY) = "Retrieving Player Info";
    *theUIVarManager.Add("PLAYER_INFO_VAR2", 0, UIVL_TEMPORARY) = " ";
    *theUIVarManager.Add("PLAYER_INFO_DESCRIPTION", 0, UIVL_TEMPORARY) = " ";
    MediusPlayerInfoRequest req;

    memset(&req, 0, sizeof(req));
    SetSessionKey(req.SessionKey);
    req.AccountID = atoi(id);
    switch (what) {
    case 0:
        memcpy(req.MessageID, "BRIEFING", MEDIUS_MESSAGEID_MAXLEN);
        break;
    case 1:
        memcpy(req.MessageID, "CLAN", MEDIUS_MESSAGEID_MAXLEN);
        break;
    case 2:
        memcpy(req.MessageID, "FRIEND", MEDIUS_MESSAGEID_MAXLEN);
        break;
    case 3:
        memcpy(req.MessageID, "LADDER", MEDIUS_MESSAGEID_MAXLEN);
        break;
    default:
        memcpy(req.MessageID, "BRIEFING", MEDIUS_MESSAGEID_MAXLEN);
        break;
    }
    MediusGetPlayerInfo(&req, cbGetPlayerInfo);
}

/* the online session left: the Medius and DME connections closed, the lobby with them (SOCOM 1's;
 * OVL1 0x2F4240: 'LobbyBailout') */
bool CZOnlineLobby::Bailout()
{
    if (!m_isopen)
        return false;
    if (theNetwork.m_bNetwork) {
        func_00300680();
        func_00300670();
    }
    if (theNetwork.m_network_init) {
        func_0030D770(&theNetwork);
        func_0030E3A0(&theNetwork, 0);
    }
    Close();
    return true;
}

/* a network error: kept in mp_net_errno, offered to the handler, else told the menus -- and, out
 * of a game and the lobby, the session left (SOCOM 1's; OVL1 0x2F4090) */
void CZOnlineLobby::RecoverFromNetError(int err)
{
    if (!err)
        return;
    bool handled = false;
    CValve::Create("mp_net_errno", VALVE_TYPE_1)->SetValue(err);
    if (m_net_error_handler)
        handled = m_net_error_handler(err);
    if (!handled) {
        if (m_current_state == READY_TO_CONNECT) {
            LobbyEvent("MP_MEDIUS_ERROR", GetString(1067));
        } else if (m_current_state == LOBBY_ACTIVE) {
            LobbyEvent("MP_MEDIUS_ERROR", GetString(1068));
        } else {
            LobbyEvent("MP_MEDIUS_ERROR", GetString(1068));
            theLobby.Bailout();
        }
    }
}

/* a chat message forwarded to us: shown in the active lobby, and anywhere when it is a private or
 * a clan one (SOCOM 1's cbChatMessage, MediusInit's first callback; OVL1 0x2F4EF0) */
void cbChatMessage(int, int, int, MediusChatFwdMessage *msg)
{
    MSGTYPE color = CHAT_WHITE;

    switch (msg->MessageType) {
    case 0:
        color = CHAT_WHITE;
        break;
    case 1:
        color = CHAT_PRIVATE;
        break;
    case 2:
        color = CHAT_CLAN;
        break;
    }
    if (theLobby.m_current_state == LOBBY_ACTIVE || color == CHAT_PRIVATE || color == CHAT_CLAN) {
        char text[1024];
        char *name = msg->OriginatorAccountName;

        sprintf(text, "%s: %s", name, msg->Message);
        ((CMessageQueue *)data_00437690)->Enqueue(text, color, name);
    }
}

/* the chat turned on or off as we enter or leave a game: the request is built, but nothing sends
 * it -- the result handed on is a constant success (SOCOM II's; OVL1 0x2F4FC0). The
 * ready-to-connect handler calls it out of line: pinned */
#pragma auto_inline off
void func_002F4FC0(int ready)
{
    MediusChatToggleRequest req;

    memset(&req, 0, sizeof(req));
    SetSessionKey(req.SessionKey);
    req.ChatToggle = ready ? MediusChatOn : MediusChatOff;
    CheckMediusError(MediusSuccess);
}
#pragma auto_inline reset

/* a text sent straight to an account (SOCOM II's; OVL1 0x2F4D60: the clan unit sends the
 * invitations' texts through it on theLobby) */
extern "C" void func_002F4D60(CZOnlineLobby *lobby, int account, int type, const char *text)
{
    MediusSendDirectMessageRequest req;

    memset(&req, 0, sizeof(req));
    SetSessionKey(req.SessionKey);
    req.TargetAccountID = account;
    if (text)
        memcpy(req.Message, text, sizeof(req.Message));
    req.MessageType = type;
    req.Message[sizeof(req.Message) - 1] = 0;
    CheckMediusError(MediusSendChatMessage(&req));
}

/* the public line filtered: taken off the filter's queue (the block's front() inline) and sent to
 * its target as a direct message -- the text above inlined -- and shown to us unless it was to
 * ourselves, a private line in its own colour (SOCOM 1's cbFilterChat, which SendChat hands the
 * censor as SOCOM 1's did, and which looked the room's players up instead; OVL1 0x2F4BA0) */
void cbFilterChat(const char *text, void *data)
{
    MediusChatMessage msg = data_0044FA20.front();

    data_0044FA20.pop();
    func_002F4D60(&theLobby, msg.TargetID, msg.MessageType, text);
    if (msg.TargetID != AccountInfo.AccountID)
        theGame.GetCurrentState()->DisplayMessage(msg.Message, AccountInfo.AccountName, 0, 0, 0, 1, 0,
                                                  msg.MessageType == 1 ? 5 : 0);
}

/* the private line's player found: the line, taken off the private queue, gets the account as its
 * target and goes through the censor as a public one does -- else it is dropped and the menus told
 * (SOCOM II's; OVL1 0x2F4A20, the callback SendChat's MediusFindPlayer is given; the name ours, by
 * the status it checks, as the friends unit's cbFindBuddyListPlayerID) */
void cbFindChatPlayerID(int, int, int, MediusFindPlayerResponse *resp)
{
    if (CheckMediusCallbackStatus("FindChatPlayerID", resp->StatusCode)) {
        MediusChatMessage msg = data_0044FA50.front();

        data_0044FA50.pop();
        msg.TargetID = resp->AccountID;
        data_0044FA20.push(msg);
        theLobby.Censor(msg.Message, cbFilterChat, 0);
    } else {
        data_0044FA50.pop();
        LobbyEvent("MP_PLAYER_NOT_FOUND", 0);
    }
}

/* the room's chat shown: each line in its colour -- white, grey, a public line's red, a clan line's
 * yellow -- with its sender when it has one, one line of text each (SOCOM 1's cbFilterChat built
 * the list itself; OVL1 0x2F5020: the lobby server's connect callback and the menu state's line
 * insert 0x1F5020 call it; the name ours, as SOCOM 1's UpdateGameListDisplay). The counter is
 * declared first (it takes s2, the room s1) and the line's sender and text are read through the
 * iterator's `*` (through `->` the name's address fills the test's delay slot) */
void UpdateChatDisplay()
{
    if (theLobby.uiv_MediusChatRecvVar) {
        int n;
        CRdrEditor ed;

        ed.push(0);
        BriefingRoom *room = data_0044F568.FindRoom(ZString(theLobby.uiv_LobbyWorldNameVar));

        if (room) {
            n = 1;

            for (std::vector<MediusChatFwdMessage>::iterator i = room->m_chat.begin(); i != room->m_chat.end();
                 ++i, ++n) {
                const char *colour = "#C\xff\xff\xff";
                char text[256];

                switch (i->MessageType) {
                case 0:
                    colour = "#C\x80\x80\x80";
                    break;
                case 1:
                    colour = "#C\x80\x40\x40";
                    break;
                case 2:
                    colour = "#C\x80\x80\x40";
                    break;
                }
                if (i->OriginatorAccountName[0])
                    sprintf(text, "%s %s: %s", colour, (*i).OriginatorAccountName, (*i).Message);
                else
                    sprintf(text, "%s%s", colour, i->Message);
                ed.addstringnode(text);
                if (n < room->m_chat.size())
                    ed.addstringnode("\n");
            }
        }
        ed.pop();
        theLobby.uiv_MediusChatRecvVar->Set(&ed);
    }
}

/* the filtered text handed to the waiting callback, which is then forgotten (SOCOM 1's; OVL1
 * 0x2F4EA0) */
void cbCensor(int a, int b, int c, MediusTextFilterResponse *resp)
{
    if (data_0044FA00)
        data_0044FA00(resp->Text, data_0044FA08);
    data_0044FA00 = 0;
    data_0044FA08 = 0;
}

/* a text sent to Medius's filter, the callback told the result (SOCOM 1's; OVL1 0x2F4DF0) */
void CZOnlineLobby::Censor(const char *text, CensorCallback cb, void *data)
{
    MediusTextFilterRequest req;

    memset(&req, 0, sizeof(req));
    SetSessionKey(req.SessionKey);
    req.TextFilterType = 1;
    if (text)
        memcpy(req.Text, text, sizeof(req.Text));
    data_0044FA00 = cb;
    data_0044FA08 = data;
    CheckMediusError(MediusTextFilter(&req, cbCensor));
}

extern "C" {
void func_002F3EF0(char *rules);   /* a game's rules written out (below) */

/* the first free slot of two looked for, and nothing done with it: the search survives, its
 * result does not (a void leaf: v1 is its only temporary; OVL1 0x2F3FF0). LobbyServerConnect's
 * handler calls it out of line: pinned */
#pragma auto_inline off
void func_002F3FF0(ChatSlot *slots)
{
    for (int i = 0; i < 2; i++) {
        if (!slots[i].m_used)
            break;
    }
}
#pragma auto_inline reset

/* a game's rules written out as a list of numbers -- the 32 bytes after an 'A' block's five-byte
 * head or a 'B' block's six -- into a buffer nothing reads (a debug print gone; OVL1 0x2F3EF0:
 * the converter below and the LAN unit's JoinGame and CreateGame call it). Open since round 9 (12
 * instructions): the stack wants a number buffer per case, both declared ahead of the text (the
 * first-declared takes the top slot: 'A''s 240, 'B''s 224, the text 48), and each case its own base
 * copy and counter, the counter declared last (base s1, counter s0 in both) */
void func_002F3EF0(char *rules)
{
    char numA[16];
    char numB[16];
    char text[176];

    switch (rules[0]) {
    case 'A': {
        const unsigned char *p = (const unsigned char *)rules;

        sprintf(text, "[");
        for (int i = 0; i < 32; i++) {
            sprintf(numA, "%d, ", p[i + 5]);
            strcat(text, numA);
        }
        strcat(text, "]");
        break;
    }
    case 'B': {
        const unsigned char *p = (const unsigned char *)rules;

        sprintf(text, "[");
        for (int i = 0; i < 32; i++) {
            sprintf(numB, "%d, ", p[i + 6]);
            strcat(text, numB);
        }
        strcat(text, "]");
        break;
    }
    }
}

/* a response put into the host's order: only the type-129 ones carry anything to convert (OVL1
 * 0x2F4030; the request side, 0x2F4080, is empty) */
void func_002F4030(void *msg, int type)
{
    switch (type) {
    case 32:
        break;
    case 33:
        break;
    case 129:
        func_002F3EF0((char *)msg + 140);
        break;
    }
}
}
