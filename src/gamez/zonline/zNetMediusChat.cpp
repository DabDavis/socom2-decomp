/* zNetMediusChat.cpp -- the lobby's chat and its message plumbing: the chat queues, the
 * censor, the messages' byte-order converters (SOCOM 1's zNetMedius.cpp's chat code; the unit and
 * its name are ours: OVL1 from about 0x2ED460, after the ladder unit, to 0x2F628F, its static
 * initialiser 0x4020E0 building the two queues 0x44FA20 and 0x44FA50, whose destructor 0x2F5E50
 * and deque instances close it). Only some of it so far. Defined in reverse of their image order
 * (-inline deferred). */

#include <stdio.h>
#include "zonline.h"
#include "../znet/znet.h"

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

/* a room of the room list (SOCOM 1's BriefingRoom: only what is read here) */
struct ChatRoom {
    unsigned char _0[0x28];
    int m_worldID;   // 0x28
    unsigned char _2c[0x2e - 0x2c];
    unsigned short m_maxPlayers;   // 0x2e
    unsigned char _30[0x38 - 0x30];
    char m_name[64];   // 0x38
    unsigned char _78[0x94 - 0x78];
    char *m_password;   // 0x94
};

/* the rdr editor a list is built in (zrdr.h's CRdrEditor, 0x4C bytes, a value at its start; zui.h's
 * _zrdr and zrdr.h's cannot meet in one unit) */
class CRdrEditor : public _zrdr {
public:
    CRdrEditor();
    ~CRdrEditor();
    bool push(const char *tag);
    bool pop();

    unsigned char _8[0x4c - 0x8];
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

MediusErrorCode func_00647CE8(MediusSessionBeginRequest *req,
                              void (*cb)(int, int, int, MediusSessionBeginResponse *));   /* MediusSessionBegin */
MediusErrorCode func_00647548(MediusSessionRequest *req,
                              void (*cb)(int, int, int, MediusVersionServerResponse *));   /* MediusVersionServer */
MediusErrorCode func_00647D88(MediusSessionRequest *req,
                              void (*cb)(int, int, int, MediusSessionEndResponse *));   /* MediusSessionEnd */
MediusErrorCode func_00648140(MediusAccountRegistrationRequest *req,
                              void (*cb)(int, int, int, MediusAccountRegistrationResponse *));   /* MediusAccountRegistration */
MediusErrorCode func_00647F48(MediusAccountLoginRequest *req,
                              void (*cb)(int, int, int, MediusAccountLoginResponse *));   /* MediusAccountLogin */

void func_002EE910(int, int, int, MediusSessionBeginResponse *);   /* cbSessionBegin */
void func_002EFE00(int, int, int, MediusVersionServerResponse *);   /* cbVersionServer */
void func_002EE7B0(int, int, int, MediusSessionEndResponse *);   /* cbSessionEnd */
MediusErrorCode func_0064AB98(MediusSetLocalizationParamsRequest *req,
                              void (*cb)(int, int, int, MediusSetLocalizationParamsResponse *));   /* MediusSetLocalizationParams */
MediusErrorCode func_00647EC0(MediusMachineSignaturePost *req);   /* the DNAS id posted */
void func_002EF3B0(int, int, int, MediusSetLocalizationParamsResponse *);   /* its callback */
int func_002EF340(unsigned char language);   /* the console's language as Medius's */
unsigned char func_003A4460();   /* the console's language */
extern NetConnectionInfo data_0066A840;   /* the account's lobby server (OVL2 bss) */
extern int data_0066A5E0;   /* the lobby server's connection (OVL2 bss) */
extern float data_0044F8D0;   /* when the lobby server was asked for */
int func_00300740(int kind, MediusConnectParams *params);   /* a connection asked for (the friends' unit's) */
MediusErrorCode func_00647C60(MediusUpdateClientStateRequest *req);   /* MediusUpdateClientState */
MediusErrorCode func_00649960(MediusSetGameListFilterRequest *req,
                              void (*cb)(int, int, int, MediusSetGameListFilterResponse *));   /* MediusSetGameListFilter */
void func_002EFE10(int, int, int, MediusSetGameListFilterResponse *);   /* its callback */
void func_002F5020();   /* the chat shown again */
void func_002F3FF0(struct ChatSlot *slots);   /* a free slot looked for (below) */
void func_002F7690();   /* (the lobby unit's) */
void func_002F4FC0(int ready);   /* our readiness told the server */
void func_002EF7F0();   /* the game's details sent to the lobby (below) */
int func_002EF590(CValve *locked);   /* the game's world updated, the delay to the next returned */
void func_002DF6A0(CZOnlineLobby *lobby);   /* the lobby's UI variables made again (the lobby's unit) */
extern CValve *data_0044F9D0;   /* the game's lock, while it is up */
extern unsigned int data_0044F9D8;   /* when the game's details next go to the lobby */
extern unsigned char data_0066A5F8;   /* the details to go at once */
extern unsigned char data_0044F890[64];   /* the server's key kept */
extern unsigned char data_0066A8E0;   /* logged in */

MediusErrorCode func_00648840(MediusCreateChannelRequest *req,
                              void (*cb)(int, int, int, MediusCreateChannelResponse *));   /* MediusCreateChannel */
MediusErrorCode func_00648A08(MediusJoinChannelRequest *req,
                              void (*cb)(int, int, int, MediusJoinChannelResponse *));   /* MediusJoinChannel */
MediusErrorCode func_006488F8(MediusJoinGameRequest *req,
                              void (*cb)(int, int, int, MediusJoinGameResponse *));   /* MediusJoinGame */
MediusErrorCode func_00648FB8(MediusFindWorldByNameRequest *req,
                              void (*cb)(int, int, int, MediusFindWorldByNameResponse *));   /* MediusFindWorldByName */
void func_002ED9C0(int, int, int, MediusFindWorldByNameResponse *);   /* cbFindRoomByName */
void func_002EE210(int, int, int, MediusFindWorldByNameResponse *);   /* cbFindGameByName */
MediusErrorCode func_006485B8(MediusCreateGameRequest *req,
                              void (*cb)(int, int, int, MediusCreateGameResponse *));   /* MediusCreateGame */
void func_002E9840(CValve *valve);   /* the clan game's team set from the valve (the clan unit's) */
int func_002E99F0();   /* the clan game's field (the clan unit's) */

extern NetConnectionInfo data_0066A540;   /* the room's lobby server (OVL2 bss) */
extern NetConnectionInfo data_0066A8F0;   /* the game's server (OVL2 bss) */
extern int LobbyWorldID;   /* the room we are in, -1 none (SOCOM 1's) */
extern int MediusWorldID;   /* the game joined (SOCOM 1's) */
extern unsigned char g_bJoiningRandomGame;
extern unsigned char data_0044F568[];   /* the room list (the rooms' unit's) */
extern float data_004A44A0;   /* (a clock) */
extern float data_0044F9E8;   /* when the room was joined */
ChatRoom *func_002E4470(void *rooms, const char *name);   /* the room list's FindRoom */
int func_002F6D00(CZOnlineLobby *lobby);   /* we join as a spectator */
void func_002F4080(void *msg, int type);   /* a request put into the network's order (empty) */

void func_00300680();   /* the Medius connection closed */
const char *func_00361B40(int id);   /* a localised text */
void func_002FA870(CZOnlineLobby *lobby);   /* the player's info asked for (the lobby unit's) */
void func_002FAA80(CZOnlineLobby *lobby);   /* and refreshed */
}


/* the lobby's state changed: pushed onto its queue (SOCOM 1 pushed inline; OVL1 0x2EFE20) */
extern "C" void func_002E1610(void *fifo, const LOBBY_STATE *state);   /* the queue's push_back */

void CZOnlineLobby::ChangeState(LOBBY_STATE state)
{
    func_002E1610(m_state_change_fifo, &state);
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
        CheckMediusError(func_00648FB8(&req, func_002EE210));
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
        CheckMediusError(func_006485B8(&req, cbCreateGame));
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
        CheckMediusError(func_006488F8(&req, cbJoinGame));
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
        CheckMediusError(func_00648A08(&req, cbJoinRoom));
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
        ChatRoom *room = func_002E4470(data_0044F568, ZString(theLobby.uiv_LobbyWorldNameVar));

        if (room)
            room->m_worldID = resp->MediusWorldID;
        theLobby.ChangeState(JOIN_ROOM);
    } else if (resp->StatusCode == (MediusCallbackStatus)-974) {
        MediusFindWorldByNameRequest req;

        memset(&req, 0, sizeof(req));
        SetSessionKey(req.SessionKey);
        CUIVariable *var = theLobby.uiv_LobbyWorldNameVar;
        if (ZString(var))
            memcpy(req.Name, ZString(var), sizeof(req.Name));
        req.WorldType = 1;
        CheckMediusError(func_00648FB8(&req, func_002ED9C0));
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
        ChatRoom *room = func_002E4470(data_0044F568, ZString(theLobby.uiv_LobbyWorldNameVar));

        if (room) {
            MediusCreateChannelRequest req;

            memset(&req, 0, sizeof(req));
            SetSessionKey(req.SessionKey);
            req.ApplicationID = 10472;
            req.MaxPlayers = room->m_maxPlayers;
            CopyFrom(req.LobbyName, room->m_name, sizeof(req.LobbyName));
            CopyFrom(req.LobbyPassword, room->m_password, sizeof(req.LobbyPassword));
            CheckMediusError(func_00648840(&req, cbCreateRoom));
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
        CheckMediusError(func_00647C60(&upd));
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
        CheckMediusError(func_00649960(&filt, func_002EFE10));
        memset(&ver, 0, sizeof(ver));
        SetSessionKey(ver.SessionKey);
        CheckMediusError(func_00647548(&ver, func_002EFE00));
        if (theLobby.uiv_MediusChatRecvVar)
            func_002F5020();
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

        func_002DF6A0(&theLobby);
        SetSessionKey(upd.SessionKey);
        upd.UserAction = 2;
        CheckMediusError(func_00647C60(&upd));
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
        loc.Language = func_002EF340(func_003A4460());
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
        CheckMediusError(func_00647F48(&login, cbAccountLogin));
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
        CheckMediusError(func_00647F48(&req, cbAccountLoginAfterRegister));
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
        CheckMediusError(func_00648140(&req, cbAccountRegistration));
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
            CheckMediusError(func_00647CE8(&begin, func_002EE910));
            memset(&req, 0, sizeof(req));
            SetSessionKey(req.SessionKey);
            CheckMediusError(func_00647548(&req, func_002EFE00));
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
            CheckMediusError(func_00647D88(&req, func_002EE7B0));
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
        LobbyEvent("MP_MEDIUS_ERROR", func_00361B40(450));
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
/* a chat message forwarded (SOCOM 1's MediusChatFwdMessage) */
struct MediusChatFwdMessage {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0x0
    int OriginatorAccountID;   // 0x18
    char OriginatorAccountName[32];   // 0x1c
    int MessageType;   // 0x3c: 1 private, 2 clan
    char Message[64];   // 0x40
};

/* a chat line's colour (a byte enum: assigned with li, compared through andi; the names ours) */
enum CHAT_COLOR {
    CHAT_WHITE = 0,
    CHAT_CLAN = 1,
    CHAT_PRIVATE = 5
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

extern "C" {
void func_00300670();   /* the Medius library closed */
void func_0030D770(CZNetwork *net);   /* the DME left (CZNetwork's) */
void func_0030E3A0(CZNetwork *net, int);   /* and closed (CZNetwork's) */
MediusErrorCode func_00649060(MediusSendDirectMessageRequest *req);   /* the direct message sent */
void func_002A4050(void *chat, const char *text, CHAT_COLOR color, const char *name);   /* a line added to the chat */
extern char data_00437690[];   /* the chat's window */
}

extern "C" {
extern CensorCallback data_0044FA00;
extern void *data_0044FA08;
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
            LobbyEvent("MP_MEDIUS_ERROR", func_00361B40(1067));
        } else if (m_current_state == LOBBY_ACTIVE) {
            LobbyEvent("MP_MEDIUS_ERROR", func_00361B40(1068));
        } else {
            LobbyEvent("MP_MEDIUS_ERROR", func_00361B40(1068));
            theLobby.Bailout();
        }
    }
}

/* a chat message forwarded to us: shown in the active lobby, and anywhere when it is a private or
 * a clan one (SOCOM 1's cbChatMessage, MediusInit's first callback; OVL1 0x2F4EF0) */
void cbChatMessage(int, int, int, MediusChatFwdMessage *msg)
{
    CHAT_COLOR color = CHAT_WHITE;

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
        func_002A4050(data_00437690, text, color, name);
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
    CheckMediusError(func_00649060(&req));
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
void func_002F3EF0(void *part);   /* a type-129 response's tail converted */

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
