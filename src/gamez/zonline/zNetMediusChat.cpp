/* zNetMediusChat.cpp -- the lobby's chat and its message plumbing: the chat queues, the
 * censor, the messages' byte-order converters (SOCOM 1's zNetMedius.cpp's chat code; the unit and
 * its name are ours: OVL1 from about 0x2ED460, after the ladder unit, to 0x2F628F, its static
 * initialiser 0x4020E0 building the two queues 0x44FA20 and 0x44FA50, whose destructor 0x2F5E50
 * and deque instances close it). Only some of it so far. Defined in reverse of their image order
 * (-inline deferred). */

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

struct MediusCreateChannelResponse { MEDIUS_RESPONSE_HEAD };
struct MediusJoinChannelResponse { MEDIUS_RESPONSE_HEAD };
struct MediusJoinGameResponse { MEDIUS_RESPONSE_HEAD };

/* a room of the room list (SOCOM 1's BriefingRoom: only what is read here) */
struct ChatRoom {
    unsigned char _0[0x2e];
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

struct MediusSessionBeginResponse { MEDIUS_RESPONSE_HEAD };
struct MediusVersionServerResponse { MEDIUS_RESPONSE_HEAD };
struct MediusSessionEndResponse { MEDIUS_RESPONSE_HEAD };
struct MediusAccountRegistrationResponse { MEDIUS_RESPONSE_HEAD };
struct MediusAccountLoginResponse { MEDIUS_RESPONSE_HEAD };

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
void func_002EEC10(int, int, int, MediusAccountLoginResponse *);   /* cbAccountLoginAfterRegister */

MediusErrorCode func_00648840(MediusCreateChannelRequest *req,
                              void (*cb)(int, int, int, MediusCreateChannelResponse *));   /* MediusCreateChannel */
MediusErrorCode func_00648A08(MediusJoinChannelRequest *req,
                              void (*cb)(int, int, int, MediusJoinChannelResponse *));   /* MediusJoinChannel */
MediusErrorCode func_006488F8(MediusJoinGameRequest *req,
                              void (*cb)(int, int, int, MediusJoinGameResponse *));   /* MediusJoinGame */
void func_002ED850(int, int, int, MediusCreateChannelResponse *);   /* cbCreateRoom */
void func_002EE3A0(int, int, int, MediusJoinChannelResponse *);   /* cbJoinRoom */
void func_002EDBA0(int, int, int, MediusJoinGameResponse *);   /* cbJoinGame */

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
        CheckMediusError(func_006488F8(&req, func_002EDBA0));
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
        CheckMediusError(func_00648A08(&req, func_002EE3A0));
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
            CheckMediusError(func_00648840(&req, func_002ED850));
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

/* the account just registered logged in (SOCOM 1's; OVL1 0x2EEB50) */
void cbAccountRegistration(int, int, int, MediusAccountRegistrationResponse *resp)
{
    if (CheckMediusCallbackStatus("AccountRegistration", resp->StatusCode)) {
        MediusAccountLoginRequest req;

        memset(&req, 0, sizeof(req));
        SetSessionKey(req.SessionKey);
        CopyFrom(req.AccountName, AccountInfo.AccountName, sizeof(req.AccountName));
        CopyFrom(req.Password, AccountInfo.Password, sizeof(req.Password));
        CheckMediusError(func_00647F48(&req, func_002EEC10));
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
extern "C" {
extern CensorCallback data_0044FA00;
extern void *data_0044FA08;
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
 * result does not (a void leaf: v1 is its only temporary; OVL1 0x2F3FF0) */
void func_002F3FF0(ChatSlot *slots)
{
    for (int i = 0; i < 2; i++) {
        if (!slots[i].m_used)
            break;
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
