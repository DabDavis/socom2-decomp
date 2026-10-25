/* The online lobby: CZOnlineLobby (SOCOM 1's zNetMedius.cpp, whose DWARF the demo does not carry;
 * the class layout is from the units that include it, 0x98 bytes in SOCOM 1), its Medius requests
 * and callbacks. SOCOM II splits SOCOM 1's one unit into several (OVL1 0x2DF560-0x306670, each
 * with its own static initialiser) and moves SOCOM 1's uiv_* statics into the class: the object is
 * 0xF8 bytes, and the callbacks, free functions, reach its members through theLobby. The header's
 * name is ours. */
#ifndef ZONLINE_H
#define ZONLINE_H

/* --- the Medius SDK's types (Sony's prebuilt library in OVL2; only what the lobby passes) ---- */

/* an API call's result: 0 on success (int-sized: its values run past a byte) */
enum MediusErrorCode {
    MediusSuccess = 0,
    MediusErrorCode_Extent = 0x7FFFFFFF
};

/* a response's status (int-sized likewise) */
enum MediusCallbackStatus {
    MediusCallbackSuccess = 0,
    MediusCallbackStatus_Extent = 0x7FFFFFFF
};

#define MEDIUS_MESSAGEID_MAXLEN 21
#define MEDIUS_SESSIONKEY_MAXLEN 17

/* the session key every request carries (OVL2 bss: SOCOM II keeps the Medius state there) */
extern char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];

/* a request's session key filled in: inlined everywhere, the null test of the array kept (SOCOM 1
 * has the same `beqz` on its own SessionKey; the name ours) */
inline void SetSessionKey(char *dst)
{
    if (SessionKey)
        memcpy(dst, SessionKey, MEDIUS_SESSIONKEY_MAXLEN);
}

/* the common head of a session request */
struct MediusSessionRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0x0
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];   // 0x15
};

struct MediusDisbandClanRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0x0
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];   // 0x15
    int ClanID;   // 0x28
};   // 0x2c

struct MediusRemovePlayerFromClanRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0x0
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];   // 0x15
    int PlayerAccountID;   // 0x28
    int ClanID;   // 0x2c
};   // 0x30

struct MediusCheckMyClanInvitationsRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0x0
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];   // 0x15
    int Start;   // 0x28
    int PageSize;   // 0x2c
};   // 0x30

struct MediusGetClanInvitationsSentRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0x0
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];   // 0x15
    int Start;   // 0x28
    int PageSize;   // 0x2c
};   // 0x30

struct MediusRespondToClanInvitationRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0x0
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];   // 0x15
    int ClanInvitationID;   // 0x28
    int Response;   // 0x2c: 1 accepted, 2 declined
    char Message[200];   // 0x30
};   // 0xf8

/* SOCOM II's member list request carries no session key */
struct MediusGetClanMemberListRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0x0
    int ClanID;   // 0x18
    int Start;   // 0x1c
    int PageSize;   // 0x20
};   // 0x24

struct MediusDisbandClanResponse;
struct MediusGetClanMemberListResponse;
struct MediusRemovePlayerFromClanResponse;
struct MediusCheckMyClanInvitationsResponse;
struct MediusGetClanInvitationsSentResponse;
struct MediusRespondToClanInvitationResponse;

typedef void (*MediusDisbandClanCallback)(MediusErrorCode, MediusErrorCode, MediusErrorCode,
                                          MediusDisbandClanResponse *);
typedef void (*MediusRemovePlayerFromClanCallback)(MediusErrorCode, MediusErrorCode,
                                                   MediusErrorCode,
                                                   MediusRemovePlayerFromClanResponse *);
typedef void (*MediusCheckMyClanInvitationsCallback)(MediusErrorCode, MediusErrorCode,
                                                     MediusErrorCode,
                                                     MediusCheckMyClanInvitationsResponse *);
typedef void (*MediusGetClanInvitationsSentCallback)(MediusErrorCode, MediusErrorCode,
                                                     MediusErrorCode,
                                                     MediusGetClanInvitationsSentResponse *);
typedef void (*MediusRespondToClanInvitationCallback)(MediusErrorCode, MediusErrorCode,
                                                      MediusErrorCode,
                                                      MediusRespondToClanInvitationResponse *);
typedef void (*MediusGetClanMemberListCallback)(MediusErrorCode, MediusErrorCode, MediusErrorCode,
                                                MediusGetClanMemberListResponse *);

extern "C" {
MediusErrorCode MediusDisbandClan(MediusDisbandClanRequest *req, MediusDisbandClanCallback cb);
MediusErrorCode MediusRemovePlayerFromClan(MediusRemovePlayerFromClanRequest *req,
                                           MediusRemovePlayerFromClanCallback cb);
MediusErrorCode MediusCheckMyClanInvitations(MediusCheckMyClanInvitationsRequest *req,
                                             MediusCheckMyClanInvitationsCallback cb);
MediusErrorCode MediusGetClanInvitationsSent(MediusGetClanInvitationsSentRequest *req,
                                             MediusGetClanInvitationsSentCallback cb);
MediusErrorCode MediusRespondToClanInvitation(MediusRespondToClanInvitationRequest *req,
                                              MediusRespondToClanInvitationCallback cb);
MediusErrorCode MediusGetClanMemberList(MediusGetClanMemberListRequest *req,
                                        MediusGetClanMemberListCallback cb);
}

/* --- the lobby --------------------------------------------------------------------------- */

/* the lobby's states (SOCOM 1's DWARF; a byte) */
enum LOBBY_STATE {
    ROOT_LOBBY_STATE = 0,
    CONNECT_SUCCESS = 1,
    CONNECT_ERROR = 2,
    ACCOUNT_REGISTRATION = 3,
    USER_REGISTRATION = 4,
    SCREENNAME_REGISTRATION = 5,
    REGISTRATION_FAILURE = 6,
    ACCOUNT_LOGIN = 7,
    USER_LOGIN = 8,
    SCREENNAME_LOGIN = 9,
    LOGIN_FAILURE = 10,
    LOBBY_ACTIVE = 11,
    SESSION_BEGIN = 12,
    CREATE_NEW_GAME = 13,
    CREATE_FAILURE = 14,
    JOIN_GAME = 15,
    LOBBY_SERVER_CONNECT = 16,
    READY_TO_CONNECT = 17,
    END_SESSION = 18,
    JOIN_ROOM = 19,
    CREATE_ROOM = 20,
    RETRIEVE_CLAN_INFO = 21,
    MAX_LOBBY_STATES = 22
};

enum LOBBY_STATE_FLAG {
    REGISTER = 0,
    ENTRY = 1,
    TICK = 2,
    EXIT = 3,
    UNREGISTER = 4
};

class CValve;
class CUIVariable;
class C2D;

typedef void (*LobbyStateHandler)(LOBBY_STATE_FLAG flag);
typedef void (*LobbyEventHandler)(const char *event, const char *arg);

class CZOnlineLobby {
public:
    bool Init();
    void UnInit();
    void Close();
    void ReleaseUIResources();
    int MediusConnectionIndex();
    void SyncGameClock(int t);

    bool IsMemberOfClan();
    bool IsLeaderOfClan(const char *name);
    void DisbandClan();
    bool GetClanRoster();
    bool GetInvitedPlayers();
    void QuitClan();
    void CheckClanInvitations();
    void AcceptClanInvitation(int id);
    void DeclineClanInvitation(int id);
    bool ClanHasInvitations();
    bool HasClanInvitations();

    /* the lobby open (SOCOM 1's m_isopen) and its state */
    bool IsOpen() { return m_isopen; }

    LobbyStateHandler m_state_handler[MAX_LOBBY_STATES];   // 0x0
    LobbyEventHandler m_event_handler;   // 0x58
    void *m_net_error_handler;   // 0x5c
    LOBBY_STATE m_current_state;   // 0x60
    unsigned int m_state_change_fifo[6];   // 0x64: a queue<LOBBY_STATE> (SOCOM 1's 0x1c)
    unsigned char m_isinit;   // 0x7c
    unsigned char m_isopen;   // 0x7d
    CValve *socom_online_state;   // 0x80
    unsigned char m_MissionSequence[32];   // 0x84 (SOCOM 1's 12)
    int m_curr_seq_idx;   // 0xa4
    unsigned char m_a8;   // 0xa8: set by the constructor (SOCOM II's)
    unsigned char _a9[3];
    int m_ac;   // 0xac
    /* SOCOM 1's uiv_* statics, members now */
    CUIVariable *uiv_GameList;   // 0xb0
    CUIVariable *uiv_RoomList;   // 0xb4
    CUIVariable *uiv_RoomPlayerList;   // 0xb8
    CUIVariable *uiv_MediusChatRecvVar;   // 0xbc
    CUIVariable *uiv_LobbyWorldNameVar;   // 0xc0
    CUIVariable *uiv_GameNameVar;   // 0xc4
    CUIVariable *uiv_MediusMOTD;   // 0xc8
    CUIVariable *uiv_ClanMessages;   // 0xcc (SOCOM II's)
    CUIVariable *uiv_MediusMyInvites;   // 0xd0
    CUIVariable *uiv_ClanInvitations;   // 0xd4
    CUIVariable *uiv_ClanRoster;   // 0xd8
    CUIVariable *uiv_MediusLadderTop10;   // 0xdc
    CUIVariable *uiv_MediusLadderRanking;   // 0xe0
    CUIVariable *uiv_ClanLadderLeaders;   // 0xe4 (SOCOM II's)
    CUIVariable *uiv_ClanLadderRanking;   // 0xe8 (SOCOM II's)
    CUIVariable *uiv_GameClock;   // 0xec
    CUIVariable *uiv_ClanNameVar;   // 0xf0
    CUIVariable *uiv_ClanDescription;   // 0xf4
};   // 0xf8

extern CZOnlineLobby theLobby;

/* a Medius call's result looked at, and a response's status (SOCOM II's; the names ours) */
bool CheckMediusError(MediusErrorCode err);
bool CheckMediusCallbackStatus(const char *what, MediusCallbackStatus status);

#endif
