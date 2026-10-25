/* The online lobby: CZOnlineLobby (SOCOM 1's zNetMedius.cpp, whose DWARF the demo does not carry;
 * the class layout is from the units that include it, 0x98 bytes in SOCOM 1), its Medius requests
 * and callbacks. SOCOM II splits SOCOM 1's one unit into several (OVL1 0x2DF560-0x306670, each
 * with its own static initialiser) and moves SOCOM 1's uiv_* statics into the class: the object is
 * 0xF8 bytes, and the callbacks, free functions, reach its members through theLobby. The header's
 * name is ours. */
#ifndef ZONLINE_H
#define ZONLINE_H

#include <string.h>
#include <vector>
#include <algorithm>
#include "../zui/zui.h"

/* a field copied when both ends are there (the items' constructors test both addresses, as SOCOM
 * 1's do; a local array's test folds away -- TransferLeadership, CreateClan. The name ours) */
inline void CopyField(void *dst, const void *src, unsigned int n)
{
    if (dst && src)
        memcpy(dst, src, n);
}

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

struct MediusDeleteClanMessageRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0x0
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];   // 0x15
    int ClanMessageID;   // 0x28
};   // 0x2c

/* the leader's messages (SOCOM II's request; the API at 0x646520, unnamed) */
struct MediusGetAllClanMessagesRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0x0
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];   // 0x15
    int Start;   // 0x28
    int PageSize;   // 0x2c
};   // 0x30

struct MediusGetMyClanMessagesRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0x0
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];   // 0x15
    int ClanID;   // 0x28
    int Start;   // 0x2c
    int PageSize;   // 0x30
};   // 0x34

struct MediusSendClanMessageRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0x0
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];   // 0x15
    char Message[200];   // 0x26
};   // 0xee

struct MediusTextFilterRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0x0
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];   // 0x15
    int TextFilterType;   // 0x28
    char Text[64];   // 0x2c
};   // 0x6c

/* a response's head: its status follows the message id */
#define MEDIUS_RESPONSE_HEAD                                                                        \
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   /* 0x0 */                                           \
    MediusCallbackStatus StatusCode;   /* 0x18 */

struct MediusTransferClanLeadershipRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0x0
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];   // 0x15
    int NewLeaderAccountID;   // 0x28
    char NewLeaderAccountName[32];   // 0x2c
};   // 0x4c

/* a player's whereabouts (the SDK's MediusPlayerOnlineState: 0x8c bytes, copied as 35 words) */
struct MediusPlayerOnlineState {
    int ConnectStatus;   // 0x0
    int MediusLobbyWorldID;   // 0x4
    int MediusGameWorldID;   // 0x8
    char LobbyName[64];   // 0xc
    char GameName[64];   // 0x4c
};   // 0x8c

/* the responses the lobby keeps whole (a member: its list's Add copies it, as a class of its own:
 * MW copies a class with a user-declared constructor member by member -- the arrays in loops, the
 * nested struct as words -- where a plain struct is copied as one block of words; the constructor
 * is ours, the copy the target's) */
struct MediusGetClanMemberListResponse {
    MediusGetClanMemberListResponse() {}

    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0x0
    MediusCallbackStatus StatusCode;   // 0x18
    int AccountID;   // 0x1c
    char AccountName[32];   // 0x20
    char Stats[256];   // 0x40
    MediusPlayerOnlineState OnlineState;   // 0x140
    int LadderStat;   // 0x1cc
    unsigned int LadderPosition;   // 0x1d0
    unsigned int TotalRankings;   // 0x1d4
    char EndOfList;   // 0x1d8
};   // 0x1dc

/* a leader's message */
struct MediusGetAllClanMessagesResponse {
    MediusGetAllClanMessagesResponse() {}

    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0x0
    MediusCallbackStatus StatusCode;   // 0x18
    int ClanMessageID;   // 0x1c
    char Message[200];   // 0x20
    int Status;   // 0xe8
    char EndOfList;   // 0xec
};   // 0xf0

struct MediusDisbandClanResponse { MEDIUS_RESPONSE_HEAD };
struct MediusRemovePlayerFromClanResponse { MEDIUS_RESPONSE_HEAD };
struct MediusTransferClanLeadershipResponse { MEDIUS_RESPONSE_HEAD };
/* an invitation to a clan received */
struct MediusCheckMyClanInvitationsResponse {
    MEDIUS_RESPONSE_HEAD
    int ClanInvitationID;   // 0x1c
    int ClanID;   // 0x20
    int ResponseStatus;   // 0x24: 0-2 kept
    char Message[200];   // 0x28
    int LeaderAccountID;   // 0xf0
    char LeaderAccountName[32];   // 0xf4
    char EndOfList;   // 0x114
};
/* an invitation the clan sent */
struct MediusGetClanInvitationsSentResponse {
    MEDIUS_RESPONSE_HEAD
    int AccountID;   // 0x1c
    char AccountName[32];   // 0x20
    char Message[200];   // 0x40
    int ResponseStatus;   // 0x108
    char EndOfList;   // 0x10c
};
struct MediusRespondToClanInvitationResponse { MEDIUS_RESPONSE_HEAD };
struct MediusDeleteClanMessageResponse { MEDIUS_RESPONSE_HEAD };
struct MediusGetMyClanMessagesResponse { MEDIUS_RESPONSE_HEAD };
struct MediusSendClanMessageResponse { MEDIUS_RESPONSE_HEAD };
/* the filter's answer: the text handed back, its status after it */
struct MediusTextFilterResponse {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0x0
    char Text[64];   // 0x15
    int Result;   // 0x58: negative, refused
};

struct MediusCreateClanRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0x0
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];   // 0x15
    int ApplicationID;   // 0x28
    char ClanName[32];   // 0x2c
};   // 0x4c

struct MediusCreateClanResponse { MEDIUS_RESPONSE_HEAD };

typedef void (*MediusCreateClanCallback)(MediusErrorCode, MediusErrorCode, MediusErrorCode,
                                         MediusCreateClanResponse *);
extern "C" MediusErrorCode MediusCreateClan(MediusCreateClanRequest *req, MediusCreateClanCallback cb);

typedef void (*MediusDeleteClanMessageCallback)(MediusErrorCode, MediusErrorCode, MediusErrorCode,
                                                MediusDeleteClanMessageResponse *);
typedef void (*MediusGetAllClanMessagesCallback)(MediusErrorCode, MediusErrorCode, MediusErrorCode,
                                                 MediusGetAllClanMessagesResponse *);
typedef void (*MediusGetMyClanMessagesCallback)(MediusErrorCode, MediusErrorCode, MediusErrorCode,
                                                MediusGetMyClanMessagesResponse *);
typedef void (*MediusSendClanMessageCallback)(MediusErrorCode, MediusErrorCode, MediusErrorCode,
                                              MediusSendClanMessageResponse *);
typedef void (*MediusTextFilterCallback)(int, int, int, MediusTextFilterResponse *);
typedef void (*MediusTransferClanLeadershipCallback)(MediusErrorCode, MediusErrorCode,
                                                     MediusErrorCode,
                                                     MediusTransferClanLeadershipResponse *);

extern "C" {
MediusErrorCode MediusTransferClanLeadership(MediusTransferClanLeadershipRequest *req,
                                             MediusTransferClanLeadershipCallback cb);
void func_006460B0(MediusDeleteClanMessageRequest *req, MediusDeleteClanMessageCallback cb);   /* MediusDeleteClanMessage? */
MediusErrorCode func_00646520(MediusGetAllClanMessagesRequest *req, MediusGetAllClanMessagesCallback cb);
MediusErrorCode MediusGetMyClanMessages(MediusGetMyClanMessagesRequest *req,
                                        MediusGetMyClanMessagesCallback cb);
MediusErrorCode MediusSendClanMessage(MediusSendClanMessageRequest *req,
                                      MediusSendClanMessageCallback cb);
MediusErrorCode MediusTextFilter(MediusTextFilterRequest *req, MediusTextFilterCallback cb);
}

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

/* --- the lobby's lists -------------------------------------------------------------------- */

/* the lobby keeps its lists in vectors of pointers that own their items (SOCOM 1's were plain
 * vectors, emptied in place): each list's destructor deletes its items through DeleteAll (the
 * clan unit's 0x2EA3C0-0x2EA5BF), and every list also has an Add that skips an item already there
 * and a Find by id. The classes and member names are ours. */

/* an item deleted and its slot cleared, for std::transform (the items' destructors are trivial:
 * the delete calls operator delete with no null test) */
template <class T>
struct DeleteItem {
    DeleteItem() {}
    T *operator()(T *p) const
    {
        delete p;
        return 0;
    }
};

/* a list emptied and its storage given up: cleared, then traded for an empty one's. Not inline: a
 * template's instance per list follows its first caller, ReleaseUIResources (0x2DFA50-0x2DFD50,
 * in reverse of their first use), with the whole body inlined in it; the name ours */
template <class T>
void ReleaseVector(std::vector<T *> &v)
{
    std::vector<T *> tmp;

    v.clear();
    v.swap(tmp);
}

/* a pointer list's end built straight from its storage: a walk that re-reads the end after a call
 * inlines whole one level down only so -- through include/vector's end() (three high) an inlined
 * find calls end() and __iter out of line inside its caller (RevokeInvite, AddFriend), where the
 * target inlines everything (ours) */
#define LOBBY_LIST_LAST() \
    iterator Last() { return iterator((elem_t *)this->data_ + this->size_); }

/* the invitations received (SOCOM 1's InvitationItem, its constructor from the response) */
struct InvitationItem {
    InvitationItem(MediusCheckMyClanInvitationsResponse *r)
    {
        ClanInvitationID = r->ClanInvitationID;
        ClanID = r->ClanID;
        CopyField(Message, r->Message, sizeof(Message));
        LeaderAccountID = r->LeaderAccountID;
        CopyField(LeaderAccountName, r->LeaderAccountName, sizeof(LeaderAccountName));
        ResponseStatus = r->ResponseStatus;
    }

    int ClanInvitationID;   // 0x0
    int ClanID;   // 0x4
    int ResponseStatus;   // 0x8
    char Message[200];   // 0xc
    int LeaderAccountID;   // 0xd4
    char LeaderAccountName[32];   // 0xd8
};   // 0xf8

/* the invitations sent (SOCOM 1's ClanInvitationItem, its constructor from the response) */
struct ClanInvitationItem {
    ClanInvitationItem(MediusGetClanInvitationsSentResponse *r)
    {
        AccountID = r->AccountID;
        CopyField(Message, r->Message, sizeof(Message));
        CopyField(AccountName, r->AccountName, sizeof(AccountName));
        ResponseStatus = r->ResponseStatus;
    }

    int AccountID;   // 0x0
    char AccountName[32];   // 0x4
    int ResponseStatus;   // 0x24
    char Message[200];   // 0x28
};   // 0xf0

/* a member of the clan and a leader's message: the responses kept whole */
typedef MediusGetClanMemberListResponse ClanMemberItem;
typedef MediusGetAllClanMessagesResponse ClanMessageItem;

/* The finds by id walk with the end taken once (`i = begin(), e = end()`): with `i != end()` the
 * inlined Find is a level too tall inside Add, which then calls end() out of line. */
class InvitationVector : public std::vector<InvitationItem *> {
public:
    ~InvitationVector() { DeleteAll(); }
    InvitationItem *Find(int id)
    {
        for (iterator i = begin(), e = end(); i != e; ++i)
            if (id == (*i)->ClanInvitationID)
                return *i;
        return 0;
    }
    void Add(MediusCheckMyClanInvitationsResponse *resp);
    void DeleteAll();
};

class ClanInvitationVector : public std::vector<ClanInvitationItem *> {
public:
    LOBBY_LIST_LAST()
    ~ClanInvitationVector() { DeleteAll(); }
    ClanInvitationItem *Find(int id)
    {
        for (iterator i = begin(), e = end(); i != e; ++i)
            if (id == (*i)->AccountID)
                return *i;
        return 0;
    }
    /* the invited account's number by name, -1 none (the end through Last: see there) */
    int FindAccount(const char *name)
    {
        for (iterator i = begin(); i != Last(); ++i)
            if (!strcasecmp((*i)->AccountName, name))
                return (*i)->AccountID;
        return -1;
    }
    void Add(MediusGetClanInvitationsSentResponse *resp);
    void DeleteAll();
};

class ClanMemberVector : public std::vector<ClanMemberItem *> {
public:
    ~ClanMemberVector() { DeleteAll(); }
    ClanMemberItem *Find(int id)
    {
        for (iterator i = begin(), e = end(); i != e; ++i)
            if (id == (*i)->AccountID)
                return *i;
        return 0;
    }
    ClanMemberItem *Find(const char *name);
    void Add(MediusGetClanMemberListResponse *resp);
    void DeleteAll();
};

class ClanMessageVector : public std::vector<ClanMessageItem *> {
public:
    ~ClanMessageVector() { DeleteAll(); }
    ClanMessageItem *Find(int id)
    {
        for (iterator i = begin(), e = end(); i != e; ++i)
            if (id == (*i)->ClanMessageID)
                return *i;
        return 0;
    }
    void Add(MediusGetAllClanMessagesResponse *resp);
    void DeleteAll();
};

/* the clan unit's lists (its initialiser 0x401EB0 builds them) */
extern InvitationVector InvitationVec;   /* the invitations received */
extern ClanInvitationVector ClanInvitationVec;   /* the invitations sent */
extern ClanMemberVector ClanMemberVec;
extern ClanMessageVector ClanMessageVec;   /* the leader's messages (SOCOM II's; the name ours) */

/* a friend, the buddy list's response kept whole (0xD0 bytes; SOCOM II's) */
struct MediusGetBuddyList_ExtraInfoResponse {
    MediusGetBuddyList_ExtraInfoResponse() {}

    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0x0
    MediusCallbackStatus StatusCode;   // 0x18
    int AccountID;   // 0x1c
    char AccountName[32];   // 0x20
    MediusPlayerOnlineState OnlineState;   // 0x40
    char EndOfList;   // 0xcc
};   // 0xd0
typedef MediusGetBuddyList_ExtraInfoResponse FriendItem;

/* the friends unit's friends (0x44FDC0, its initialiser 0x402420) */
class FriendVector : public std::vector<FriendItem *> {
public:
    LOBBY_LIST_LAST()
    ~FriendVector() { DeleteAll(); }
    FriendItem *Find(int id)
    {
        for (iterator i = begin(), e = end(); i != e; ++i)
            if (id == (*i)->AccountID)
                return *i;
        return 0;
    }
    FriendItem *Find(const char *name);
    void Add(MediusGetBuddyList_ExtraInfoResponse *resp);
    void DeleteAll();
};

extern FriendVector FriendVec;

/* a clan on the clan ladder, the response kept whole (0x48 bytes; SOCOM II's) */
struct MediusGetClanLadderListResponse {
    MediusGetClanLadderListResponse() {}

    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0x0
    MediusCallbackStatus StatusCode;   // 0x18
    char ClanName[32];   // 0x1c
    int LadderPosition;   // 0x3c: the list's key, the highest kept
    int LadderStat;   // 0x40
    char EndOfList;   // 0x44
};   // 0x48
typedef MediusGetClanLadderListResponse ClanLadderItem;

/* the ladder unit's clan ladders (0x44F820 the leaders, 0x44F850 the list around us; its
 * initialiser 0x401FA0 zeroes them and sets +0x10 to 2): the clans and the lowest position seen */
class ClanLadderList : public std::vector<ClanLadderItem *> {
public:
    ~ClanLadderList() { DeleteAll(); }
    ClanLadderItem *Find(int pos)
    {
        for (iterator i = begin(), e = end(); i != e; ++i)
            if (pos == (*i)->LadderPosition)
                return *i;
        return 0;
    }
    void Add(MediusGetClanLadderListResponse *resp);
    void DeleteAll();

    int m_lowest;   // 0xc
    int m_10;   // 0x10: 2
};   // 0x14

extern ClanLadderList ClanLadderLeaders;   /* 0x44F820 */
extern ClanLadderList ClanLadderRanking;   /* 0x44F850 */

/* a player on the ladder (SOCOM 1's LadderItem; its two strings make its delete inline their
 * destructors, 0x2EC140) and the player ladders (0x44F7B0, 0x44F7E0) */
struct LadderItem;

class LadderList : public std::vector<LadderItem *> {
public:
    int m_c;   // 0xc: 2
    int m_10;   // 0x10
};   // 0x14

extern LadderList LadderLeaders;   /* 0x44F7B0 */
extern LadderList LadderRanking;   /* 0x44F7E0 */

/* a game in a room (SOCOM 1's RoomGame: a table, but not a virtual destructor -- its delete calls
 * operator delete straight) */
class RoomGame {
public:
    virtual bool IsPersistent();
};

class RoomGameVector : public std::vector<RoomGame *> {
public:
    void DeleteAll();
};

/* a room (SOCOM 1's BriefingRoom, 0x98 bytes in SOCOM II): its games first */
class BriefingRoom {
public:
    RoomGameVector m_games;   // 0x0
    unsigned char _c[0x98 - 0xc];
};   // 0x98

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
    void CreateClan(const char *name);
    void TransferLeadership(const char *name);
    void ChangeState(LOBBY_STATE state);
    bool Logout();
    void BanSelfFromGame();
    void GetMissionSequence(unsigned char *idx, unsigned char *seq);
    void SetMissionSequence(unsigned char idx, unsigned char *seq);
    void ResetMissionSequence();
    int AdvanceMissionSequence();
    void SendEndGameReport();
    void GetAnnouncements();
    void GetUsagePolicy();
    void PasswordSecurity();
    bool RemovePlayerFromClan(const char *name);
    bool SendClanMessage(const char *text);
    void GetClanMessages();
    void GetLeaderMessages();
    void DeleteClanMessage(int id);

    void *DMEInit();
    void RunStateHandler(LOBBY_STATE state, LOBBY_STATE_FLAG flag);
    bool RefreshRoomList(C2D *page);
    bool RejoinRoom();
    bool RefreshGameList(C2D *page);
    void GetLadderInfo();
    void GetLadderRanking();
    void GetClanLadderLeaders(int n);
    void GetFriendsList();
    void RemoveFriend(int id);
    void GetFriendInvitations();
    void GetIgnoreList();
    void AddFriendDirectly(int id);
    void RejectFriend(int id);
    void ConfirmFriend(int id);
    void AddToIgnore(const char *name);
    void RemoveFromIgnore(int id);
    void AddFriend(const char *name);
    bool RevokeInvite(const char *name);
    void UpdateClanStats();
    bool GetClan();
    bool IsIgnored(const char *name);

    /* open and in the lobby proper (the name ours: GetFriendInvitations materialises the && before
     * it branches, an inline bool's shape) */
    bool IsActive() { return m_isopen && m_current_state == LOBBY_ACTIVE; }

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

/* an event told to the menus, when they listen: through theLobby, from members and callbacks
 * alike (the name ours) */
inline void LobbyEvent(const char *event, const char *arg)
{
    if (theLobby.m_event_handler)
        theLobby.m_event_handler(event, arg);
}

/* a Medius call's result looked at, and a response's status (SOCOM II's; the names ours) */
bool CheckMediusError(MediusErrorCode err);
bool CheckMediusCallbackStatus(const char *what, MediusCallbackStatus status);
const char *LookupMediusCallbackStatus(MediusCallbackStatus status);

/* the UI variables' setters (unnamed yet: CUIVariable::operator=(const char *) and
 * CUIVarManager::Add) */
extern "C" {
void func_003862A0(CUIVariable *var, const char *text);
}

#endif
