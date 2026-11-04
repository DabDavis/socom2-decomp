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
#include <string>
#include <algorithm>
#include <queue>
#include "../zui/zui.h"

/* a field copied when both ends are there (the items' constructors test both addresses, as SOCOM
 * 1's do; a local array's test folds away -- TransferLeadership, CreateClan. The name ours) */
inline void CopyField(void *dst, const void *src, unsigned int n)
{
    if (dst && src)
        memcpy(dst, src, n);
}

/* a field copied in, only the destination's address tested (Open's account defaults from
 * literals, the room player's clan tag from a local: CopyField's test of either source is not
 * folded; the name ours) */
inline void CopyTo(char *dst, const char *src, unsigned int n)
{
    if (dst)
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
    int ResponseTime;   // 0x10c (the SDK's; cbGetClanInvitationsSent reads the list's end after it)
    char EndOfList;   // 0x110
};
struct MediusRespondToClanInvitationResponse { MEDIUS_RESPONSE_HEAD };
struct MediusDeleteClanMessageResponse { MEDIUS_RESPONSE_HEAD };
/* a message of our clan's (the SDK's: the clan, the text, the list's end) */
struct MediusGetMyClanMessagesResponse {
    MEDIUS_RESPONSE_HEAD
    int ClanID;   // 0x1c
    char Message[200];   // 0x20
    char EndOfList;   // 0xe8
};
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

struct MediusCreateClanResponse {
    MEDIUS_RESPONSE_HEAD
    int ClanID;   // 0x1c: the new clan's
};

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
MediusErrorCode MediusDeleteClanMessage(MediusDeleteClanMessageRequest *req, MediusDeleteClanMessageCallback cb);
MediusErrorCode MediusGetAllClanMessages(MediusGetAllClanMessagesRequest *req, MediusGetAllClanMessagesCallback cb);
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
MediusErrorCode MediusGetClanMemberList_ExtraInfo(MediusGetClanMemberListRequest *req,
                                        MediusGetClanMemberListCallback cb);
}

/* the rdr editor the menus' lists are built in (zrdr.h's CRdrEditor, 0x4C bytes, a value at its
 * start; zui.h's _zrdr and zrdr.h's cannot meet in one unit, so the lobby declares it again) */
struct PNT3D;

class CRdrEditor : public _zrdr {
public:
    CRdrEditor();
    ~CRdrEditor();
    bool push(const char *tag);
    bool pop();
    bool addintnode(int i);
    bool addstringnode(const char *s);
    bool addPNT3D(const char *tag, const PNT3D &p);
    bool addSTRING(const char *tag, const char *s);

    unsigned char _8[0x4c - 0x8];
};

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
    /* the invitation sent to the named player, 0 none (InvitePlayerToClan's; the end re-read
     * every pass, as the call in the loop makes MW do) */
    ClanInvitationItem *Find(const char *name)
    {
        for (iterator i = begin(); i != end(); ++i)
            if (!strcasecmp((*i)->AccountName, name))
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
    void UpdateDisplay();
};

extern FriendVector FriendVec;

/* a clan on the clan ladder, the response kept whole (0x48 bytes; SOCOM II's) */
struct MediusGetClanLadderListResponse {
    MediusGetClanLadderListResponse() {}

    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0x0
    int ClanID;   // 0x18: our clan's shown in colour
    char ClanName[32];   // 0x1c
    int LadderPosition;   // 0x3c: the list's key, the highest kept
    MediusCallbackStatus StatusCode;   // 0x40 (the callback checks it)
    char EndOfList;   // 0x44
};   // 0x48
typedef MediusGetClanLadderListResponse ClanLadderItem;

/* the ladder unit's clan ladders (0x44F820 the leaders, 0x44F850 the list around us; its
 * initialiser 0x401FA0 zeroes them and sets +0x10 to 2): the clans and the lowest position seen */
class ClanLadderList : public std::vector<ClanLadderItem *> {
public:
    ClanLadderList() { m_10 = 2; m_lowest = 0; }
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
 * destructors, 0x2EC140) and the player ladders (0x44F7B0, 0x44F7E0). Only what the displays read:
 * the names as MSL strings (c_str() short form first) and the position. The names are CSaferStrs
 * (each unit declares the class again: a std::string, its destructor called as a base's) */
class CSaferStr : public std::string {
public:
    CSaferStr() {}
    /* nothing returned: the out-of-line copy (0x22DCC0) keeps no `this` for a result */
    void operator=(const char *sz)
    {
        if (sz)
            assign(sz);
        else
            erase();
    }
};

/* a ladder page's player record, kept whole at the item's head (0x8C bytes; no constructor -- the
 * item's, cut to one level, would call it -- yet its assignment copies member by member, the
 * arrays two bytes a pass: the out-of-line copy 0x2EBFE0 after LadderList::Add, its first caller;
 * the names ours) */
struct LadderPlayerInfo {
    int m_0;   // 0x0
    int m_4;   // 0x4
    int m_8;   // 0x8
    char m_c[64];   // 0xc
    char m_4c[64];   // 0x4c
};   // 0x8c

/* a player's stats unpacked (the stats parser 0x2F9AC0's output: 0xE4 bytes, which it copies whole
 * from an 'E' record, the format letter in byte 0, and converts from the older 'A'-'D', 'N' and 'Q'
 * texts; the names ours). The layout is the one its implicit copy reads, member by member (the
 * odd-length array), as our stats' copies 0x2F96F0/0x2F98F0 do (round 13): a char (lb), byte 1's
 * and byte 2's bit-fields each copied by their group's first type (lbu, though m_side is a signed
 * char: Open's store reads it with lb), byte 3's unsigned int bit-field as an UNALIGNED word
 * (lwr/lwl 3..6, overlapping the next word's copy), eight words, eight unsigned shorts and four
 * char arrays (two bytes a pass where the length is even) */
struct GamePlayerStats {
    char m_format;   // 0x0: 'E'
    unsigned char m_1 : 7;   // 0x1
    unsigned char m_late : 1;   // byte 1, bit 7: counted
    unsigned char m_2 : 5;   // 0x2
    signed char m_side : 2;   // byte 2, bits 5-6
    unsigned char m_2_7 : 1;
    unsigned int m_3 : 8;   // 0x3
    int m_rating;   // 0x4: the ladder figure
    int m_8;   // 0x8
    int m_c;   // 0xc
    unsigned int m_time;   // 0x10: the seconds played (st_mpexit.cpp's)
    int m_14;   // 0x14
    int m_18;   // 0x18
    int m_kills;   // 0x1c
    unsigned int m_games : 22;   // 0x20
    unsigned int m_20_22 : 10;
    unsigned short m_24;   // 0x24
    unsigned short m_26;   // 0x26
    unsigned short m_28;   // 0x28
    unsigned short m_deaths;   // 0x2a
    unsigned short m_2c;   // 0x2c
    unsigned short m_2e;   // 0x2e
    unsigned short m_30;   // 0x30
    unsigned short m_32;   // 0x32
    char m_clan[12];   // 0x34: the clan's tag
    char m_40[32];   // 0x40
    char m_town[63];   // 0x60: the home town
    char m_desc[66];   // 0x9f: the description
};   // 0xe4

/* a page of the player ladder (no session key: the message id names the list, TheBest or
 * TheRest) */
struct MediusLadderListResponse {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0x0
    MediusCallbackStatus StatusCode;   // 0x18
    int LadderPosition;   // 0x1c
    int LadderStat;   // 0x20
    char AccountName[32];   // 0x24
    char AccountStats[256];   // 0x44
    LadderPlayerInfo Info;   // 0x144
    char EndOfList;   // 0x1d0
};

extern "C" void func_002F9AC0(GamePlayerStats *stats, const char *text);   /* a player's stats unpacked */

struct LadderItem {
    /* the page's player made an item: too tall to inline whole, so the names' constructors and
     * assignments are calls (0x1E7B60, 0x22DCC0) and the record's assignment too (0x2EBFE0) */
    LadderItem(MediusLadderListResponse *resp)
    {
        GamePlayerStats stats;

        func_002F9AC0(&stats, resp->AccountStats);
        m_position = resp->LadderPosition;
        m_name = resp->AccountName;
        m_a4 = resp->LadderStat;
        char clan[32] = {0};
        if (stats.m_clan[0])
            sprintf(clan, "[%s]", stats.m_clan);
        m_clan = clan;
        m_info = resp->Info;
    }

    /* the names' texts (the names ours): read through an accessor, the inlined c_str() folds the
     * string's offset into the item's (0x8C/0x8D/0x94); `(*i)->m_name.c_str()` keeps the string's
     * own address in a register (UpdateLadderBestDisplay 8 bytes long -- the MSL headers round 6) */
    const char *GetName() { return m_name.c_str(); }
    const char *GetClan() { return m_clan.c_str(); }

    LadderPlayerInfo m_info;   // 0x0: the page's record
    CSaferStr m_name;   // 0x8c (a base's destructor: 0x2EC140 passes 0)
    CSaferStr m_clan;   // 0x98
    int m_a4;   // 0xa4
    int m_position;   // 0xa8
};

class LadderList : public std::vector<LadderItem *> {
public:
    LadderList() { m_c = 2; m_10 = 0; }
    ~LadderList() { DeleteAll(); }
    LadderItem *Find(int pos)
    {
        for (iterator i = begin(), e = end(); i != e; ++i)
            if (pos == (*i)->m_position)
                return *i;
        return 0;
    }
    LadderItem *Find(const char *name);
    void Add(MediusLadderListResponse *resp);
    void DeleteAll();

    int m_c;   // 0xc: 2
    int m_10;   // 0x10
};   // 0x14

extern LadderList LadderLeaders;   /* 0x44F7B0 */
extern LadderList LadderRanking;   /* 0x44F7E0 */

/* a game's info as the game list reports it, and as a LAN host hosts and announces it (the SDK's
 * MediusGameListResponse, 0x190 bytes: the head of every RoomGame; a class with a constructor of its
 * own, copied member by member -- the id bytewise, the strings two bytes a pass. Its assignment is
 * too big to inline: the copy at 0x2E1E10 follows the game's constructor, its first caller. The
 * struct's name is SOCOM 1's -- cbGameList and RoomGame::UpdateInfo take a MediusGameListResponse *
 * in its symbols -- the field names the LAN unit's, which calls it LANGameInfo) */
struct MediusGameListResponse {
    MediusGameListResponse() {}

    char m_id[21];   // 0x0
    int m_18;   // 0x18: the response's status
    int m_ip;   // 0x1c: the game's world (Medius), the host's address (LAN)
    unsigned short m_players;   // 0x20
    unsigned short m_22;   // 0x22
    unsigned short m_maxPlayers;   // 0x24
    int m_mission;   // 0x28
    int m_2c;   // 0x2c
    int m_30;   // 0x30
    int m_port;   // 0x34
    int m_38;   // 0x38
    int m_3c;   // 0x3c
    int m_flags;   // 0x40: 1 a player password, 4 a spectators' one
    int m_lock;   // 0x44: 3 locked, else 1 or 2
    int m_48;   // 0x48
    char m_name[64];   // 0x4c
    unsigned char m_rules[256];   // 0x8c
    char m_18c;   // 0x18c: the list's last
};   // 0x190
typedef MediusGameListResponse LANGameInfo;

/* a game's rules as its info carries them (at 0x8c; a RoomGame keeps them at 0x19a): a version
 * letter, then the round count and time, the side byte, the spectators, the equipment limit, the
 * sort order and the rank restrictions, then the mission sequence -- 'D', 40 bytes (the lobby
 * unit's builder 0x2F7C40 and reader 0x2F7D90; the names ours) */
struct GameRules {
    char m_version;   // 0x0: 'D'
    unsigned char m_rounds;   // 0x1
    unsigned char m_minutes;   // 0x2
    unsigned char m_side;   // 0x3
    unsigned char m_spectators;   // 0x4
    unsigned char m_eqlimit;   // 0x5
    unsigned char m_sort;   // 0x6
    unsigned char m_rank;   // 0x7
    unsigned char m_missions[32];   // 0x8
};   // 0x28

extern "C" GameRules *func_002F7D90(GameRules *dst, const GameRules *src);   /* a game's rules read, an older version brought up to date (the lobby unit's) */

/* a game in a room (SOCOM 1's RoomGame: a table, but not a virtual destructor -- its delete calls
 * operator delete straight). SOCOM II's keeps the game's 0x190-byte info first and its table after
 * it (the constructor 0x2E1D50 stores the vtable at 0x190 and clears the 400 bytes before it); its
 * one virtual, SOCOM 1's IsPersistent slot, hands back the mission sequence at 0x1A2 (0x2E24C0) */
class RoomGame {
public:
    RoomGame(LANGameInfo *info);
    /* the game's info taken from the list (SOCOM 1's UpdateInfo): too big to inline whole, so the
     * constructor and the game list's callback expand it one level, calling the info's assignment
     * and SetMission out of line */
    void UpdateInfo(LANGameInfo *info)
    {
        if (info) {
            m_info = *info;
            func_002F7D90((GameRules *)m_19a, (GameRules *)info->m_rules);
            if (!m_info.m_mission)
                SetMission(m_missionSeq[0]);
        } else
            m_info.m_ip = -1;
    }
    void SetMission(int mission) { m_info.m_mission = mission; }

    LANGameInfo m_info;   // 0x0

    virtual unsigned char *GetMissionSequence();
    bool AllowsType(int type);
    bool HasMissions();

    int m_194;   // 0x194
    bool m_unchecked;   // 0x198: set for every game as the game list is asked for again
    unsigned char m_199;   // 0x199
    unsigned char m_19a[3];   // 0x19a: what the game's rules hand func_002F7D90
    unsigned char m_spectators;   // 0x19d: of the players, those watching
    unsigned char m_maxSpectators;   // 0x19e
    unsigned char m_19f[2];   // 0x19f
    unsigned char m_types;   // 0x1a1: the game types it allows, a bit each (none: any)
    unsigned char m_missionSeq[0x22];   // 0x1a2
};   // 0x1c4

/* a game the game list reported (SOCOM II's, by its RTTI at 0x4066D0: no virtual of its own) */
class TransientGame : public RoomGame {
public:
    TransientGame(LANGameInfo *info) : RoomGame(info) {}
};

/* a room as the channel list reports it (the Medius SDK's MediusChannelListResponse, 0x70 bytes:
 * copied member by member into the room -- the id bytewise, the name two bytes a pass) */
struct MediusChannelListResponse {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0x0
    MediusCallbackStatus StatusCode;   // 0x18
    int MediusWorldID;   // 0x1c
    unsigned short PlayerCount;   // 0x20
    unsigned short MaxPlayers;   // 0x22
    int GameWorldCount;   // 0x24
    unsigned int GenericField;   // 0x28: the game types the room allows (a bit each, the low four),
                                 // the room list's sort key (the top sixteen)
    char LobbyName[64];   // 0x2c
    char EndOfList;   // 0x6c
};   // 0x70

/* a player in a room (the SDK's MediusLobbyWorldPlayerListResponse, 0xD0 bytes) */
struct RoomPlayer {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0x0
    MediusCallbackStatus StatusCode;   // 0x18
    int AccountID;   // 0x1c
    char AccountName[32];   // 0x20
    MediusPlayerOnlineState OnlineState;   // 0x40
    char EndOfList;   // 0xcc
};   // 0xd0

/* a chat message forwarded (SOCOM 1's MediusChatFwdMessage, 0x80 bytes: a room keeps its chat in
 * a vector of them, as SOCOM 1's BriefingRoom did -- the menu state's 0x1F5020 builds one and
 * inserts it, through the insert 0x1F5640 whose deleter's destructor 0x1F5C30 ~BriefingRoom calls) */
struct MediusChatFwdMessage {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0x0
    int OriginatorAccountID;   // 0x18
    char OriginatorAccountName[32];   // 0x1c
    int MessageType;   // 0x3c: 1 private, 2 clan
    char Message[64];   // 0x40
};   // 0x80

/* a room (SOCOM 1's BriefingRoom, 0x98 bytes in SOCOM II): its games (the base vector itself --
 * ~BriefingRoom tests `this` three times for it, a derived list would be four), then the channel
 * list's record of it (a base at 0xc: the copy converts the room with a null test), its players,
 * the other list and the password. The constructor is inline, too big to inline whole: it calls
 * its bases' and members' constructors out of line (0x2E43F0, 0x2E43D0, 0x2E43B0), the record's
 * copy (0x2E4300) and the password's setter */
class BriefingRoom : public std::vector<RoomGame *>, public MediusChannelListResponse {
public:
    BriefingRoom(MediusChannelListResponse *resp)
    {
        m_password = 0;
        MaxPlayers = 256;
        PlayerCount = 0;
        if (resp)
            Set(resp);
        SetPassword("");
    }
    ~BriefingRoom();
    void Set(MediusChannelListResponse *resp) { *(MediusChannelListResponse *)this = *resp; }
    void SetPassword(const char *password);
    RoomPlayer *FindPlayer(const char *name);
    /* the game types the room allows include this one, or it names none (CanJoinRoom inlines it;
     * RoomGame::AllowsType is the same test of a game's byte) */
    bool AllowsType(int type)
    {
        bool r = true;
        unsigned int types = GenericField & 0xF;

        if (types) {
            if ((1 << (type - 1)) & types)
                r = true;
            else
                r = false;
        }
        return r;
    }
    void DeleteGames();
    RoomGame *FindGame(int mission);
    RoomGame *FindGame(const char *name);
    void RemoveEmptyGames();
    void RemoveUncheckedGames();
    void SortGames();

    std::vector<RoomPlayer> m_players;   // 0x7c
    std::vector<MediusChatFwdMessage> m_chat;   // 0x88
    char *m_password;   // 0x94
};   // 0x98

/* the room's games sorted and handed to the menus (SOCOM 1's; the rooms' unit's, 0x2E2CE0) */
void UpdateGameListDisplay(BriefingRoom *room);

/* the room list (SOCOM 1's BriefingRoomDB: 0x44F568, the rooms' unit's) */
class BriefingRoomDB : public std::vector<BriefingRoom *> {
public:
    /* the rooms deleted with the list (0x2E5F60, which __sinit_zNetMediusRoom.cpp registers) */
    ~BriefingRoomDB() { DeleteAll(); }
    BriefingRoom *FindRoom(const char *name);
    /* FindRoom again, as an inline (CanJoinRoom expands it where the game list's request calls
     * FindRoom out of line) */
    BriefingRoom *Find(const char *name)
    {
        BriefingRoom *room = 0;

        if (name) {
            for (iterator i = begin(); i != end(); ++i) {
                if (!strcmp((*i)->LobbyName, name)) {
                    room = *i;
                    break;
                }
            }
        }
        return room;
    }
    void Add(MediusChannelListResponse *resp);
    void DeleteAll();
    void Release();
};

class C2D;

/* the LAN lobby (the UI lane's name; zNetLANLobby.cpp's, 0x44FE10): its room, then the game info
 * it hosts. Declared here for the units that call it by name (zNetMedius.cpp's room test,
 * zNetDme.cpp's game info and IsActive) */
class CZLANLobby {
public:
    bool GameHasPassword(const char *game);
    bool CreateGame();
    void GetGameInfo(LANGameInfo *out);
    bool RefreshGameList(C2D *page);
    bool Disconnect();
    bool Connect();
    bool IsActive();

    BriefingRoom *m_room;   // 0x0
    unsigned char _4[0x10 - 0x4];
    LANGameInfo m_info;   // 0x10
};   // 0x1a0

extern CZLANLobby data_0044FE10;   /* the LAN lobby */

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

/* the states waiting to be entered (SOCOM 1's queue<LOBBY_STATE>; a class of its own around it,
 * as util_ssm.cpp's CStateQueue: ~CZOnlineLobby tests `this` four times before calling the block
 * buffer's destructor, 0x2E19E0. The name ours) */
class CLobbyStateQueue : public std::queue<LOBBY_STATE> {
};

typedef void (*LobbyStateHandler)(LOBBY_STATE_FLAG flag);
typedef void (*LobbyEventHandler)(const char *event, const char *arg);

class CZOnlineLobby {
public:
    CZOnlineLobby();
    ~CZOnlineLobby();
    bool Init();
    void UnInit();
    bool Open(const char *host, int port);
    void Close();
    void ReleaseUIResources();
    void CreateUIResources();
    int MediusConnectionIndex();
    void SyncGameClock(int t);
    void Tick();

    bool IsMemberOfClan();
    bool IsLeaderOfClan(const char *name);
    bool InvitePlayerToClan(const char *name);
    bool OnCorrectClanTeam();
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
    void SetConnectionInfo(struct NetConnectionInfo *info);
    void GetMissionSequence(unsigned char *idx, unsigned char *seq);
    void SetMissionSequence(unsigned char idx, unsigned char *seq);
    void ResetMissionSequence();
    int AdvanceMissionSequence();
    void SendEndGameReport();
    void GetAnnouncements();
    void GetUsagePolicy();
    static void PasswordSecurity();
    bool RemovePlayerFromClan(const char *name);
    bool SendClanMessage(const char *text);
    void GetClanMessages();
    void GetLeaderMessages();
    void ModifyClanMessage(int id, const char *text);
    void DeleteClanMessage(int id);

    void *DMEInit();
    void RecoverFromNetError(int err);
    bool Bailout();
    void RunStateHandler(LOBBY_STATE state, LOBBY_STATE_FLAG flag);
    bool RefreshRoomList(C2D *page);
    bool RejoinRoom();
    bool JoinRoom(const char *name);
    bool RefreshGameList(C2D *page);
    bool RefreshGamePlayerList(C2D *page);
    void GetLadderInfo();
    void GetLadderRanking();
    int GetRankTier(int rank);
    int GetRankLevel(int rank);
    void GetLadderBest(int n);
    void GetLadderList(int start, int n);
    int GetClanRankTier(int rank);
    void GetClanLadderList(int start, int n);
    void GetClanRanking(int n);
    bool CanJoinRoom(const char *name);
    void GetClanLadderLeaders(int n);
    void GetFriendsList();
    void RemoveFriend(int id);
    void GetFriendInvitations();
    void GetFriendInfo(int id);
    void GetIgnoreList();
    void AddFriendDirectly(int id);
    void RejectFriend(int id);
    void ConfirmFriend(int id);
    void AddToIgnore(const char *name);
    void RemoveFromIgnore(int id);
    void AddFriend(const char *name);
    bool RevokeInvite(const char *name);
    void UpdateClanStats();
    void UpdateClanRanking();
    bool GetClan();
    bool IsIgnored(const char *name);
    void Censor(const char *text, void (*cb)(const char *text, void *data), void *data);
    bool SendChat(const char *text, const char *to);

    /* open and in the lobby proper (the name ours: GetFriendInvitations materialises the && before
     * it branches, an inline bool's shape) */
    bool IsActive() { return m_isopen && m_current_state == LOBBY_ACTIVE; }

    LobbyStateHandler m_state_handler[MAX_LOBBY_STATES];   // 0x0
    LobbyEventHandler m_event_handler;   // 0x58
    bool (*m_net_error_handler)(int err);   // 0x5c: offered a network error first
    LOBBY_STATE m_current_state;   // 0x60
    CLobbyStateQueue m_state_change_fifo;   // 0x64 (SOCOM 1's 0x1c)
    bool m_isinit;   // 0x7c (a bool: Init returns it unconverted)
    unsigned char m_isopen;   // 0x7d
    CValve *socom_online_state;   // 0x80
    unsigned char m_MissionSequence[32];   // 0x84 (SOCOM 1's 12)
    int m_curr_seq_idx;   // 0xa4
    unsigned char m_a8;   // 0xa8: set by the constructor (SOCOM II's)
    unsigned char _a9[3];
    float m_loginTime;   // 0xac: when the account logged in (the clock 0x4A44A0; SOCOM II's)
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

/* a state of the game's stack, as far as the player is told things (Apps/FTS/fts_state.h's
 * CGameState: its first ten slots, DisplayMessage at 0x2C; the friends unit's invitations and the
 * chat's lines are told through it) */
class CMessageState {
public:
    virtual ~CMessageState();
    virtual void PreInit();
    virtual bool Init();
    virtual bool Tick(float dT);
    virtual void PreUnInit();
    virtual void UnInit();
    virtual void OnPush(int userdata);
    virtual void OnPop(int userdata);
    virtual void SetSkipSwap(int swap);
    virtual void DisplayMessage(const char *text, const char *who, int a, int b, int c,
                                int d, unsigned char e, int type);
};

/* the game's stack of states (zgame.h's CGame: the current state's index at 0xC, the stack 0x14) */
struct CMessageGame {
    CMessageState *GetCurrentState() { return m_CurrentState >= 16 ? 0 : m_Stack[m_CurrentState]; }

    unsigned char _0[0xc];
    unsigned int m_CurrentState;   // 0xc
    CMessageState *m_TestVariable;   // 0x10
    CMessageState *m_Stack[16];   // 0x14
};

extern CMessageGame theGame;   /* 0x4084C0 */

#endif
