/* zNetMediusFriends.cpp -- the lobby's buddy lists, new in SOCOM II: friends, the ignore list and
 * the friend invitations, and their Medius requests (the unit and its name are ours: OVL1 about
 * 0x300670-0x30348F; its static initialiser 0x402420 builds the three lists 0x44FDC0, 0x44FDE0 and
 * 0x44FE00, whose destructors 0x303310-0x30348F close it). The Medius buddy calls are unnamed
 * yet: SOCOM 1's lobby made none of them. Only some of it so far. Defined in reverse of their
 * image order (-inline deferred). */

#include <string.h>
#include "zonline.h"

/* a vector of pointers, as the lobby's lists are */
struct LobbyList {
    unsigned int capacity_;   // 0x0
    unsigned int size_;   // 0x4
    void **data_;   // 0x8
};

/* a request bearing only its message id */
struct MediusIDRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0x0
};   // 0x15

/* an account named in a session request */
struct MediusAccountRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0x0
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];   // 0x15
    int AccountID;   // 0x28
};   // 0x2c

/* the ignore list's request: the session's head and no more */
struct MediusGetIgnoreListRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0x0
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];   // 0x15
};   // 0x26

/* the answer to a forwarded buddy request: its status ahead of the session key */
struct MediusBuddyConfirmationResponse {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0x0
    int StatusCode;   // 0x18: 2 accepted, -985 refused
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];   // 0x1c
    int TargetAccountID;   // 0x30
    int AddType;   // 0x34
};   // 0x38

struct MediusBuddyResponse;

typedef void (*MediusBuddyCallback)(int, int, int, MediusBuddyResponse *);

/* an account's number asked for by name */
struct MediusAccountGetIDRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0x0
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];   // 0x15
    char AccountName[32];   // 0x26
};   // 0x46

/* the number: the status after it */
struct MediusAccountGetIDResponse {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0x0
    int AccountID;   // 0x18
    MediusCallbackStatus StatusCode;   // 0x1c
};

/* a request's bare answer */
struct MediusStatusResponse { MEDIUS_RESPONSE_HEAD };

/* someone ignored (0x48 bytes, a vector of them: the class vector's clear out of line, 0x302FC0) */
struct IgnoreItem {
    /* a blank entry (resize's T(): the memset call 0x303250 makes) */
    IgnoreItem() { memset(this, 0, sizeof(*this)); }

    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0x0
    MediusCallbackStatus StatusCode;   // 0x18
    int AccountID;   // 0x1c
    char AccountName[32];   // 0x20
    int _40;
    char EndOfList;   // 0x44
};   // 0x48

class IgnoreVector : public std::vector<IgnoreItem> {
public:
    /* sized to 64 blank entries at the start (the initialiser calls the resize 0x303250 with 64) */
    IgnoreVector() { resize(64); }
    /* emptied first (the destructor's first store, ahead of the bases' three this-tests) */
    ~IgnoreVector() { clear(); }
    /* the ignored by name (the null test inside, though the caller made it) */
    IgnoreItem *Find(const char *name)
    {
        if (name)
            for (iterator i = begin(); i != end(); ++i)
                if (!strcasecmp((*i).AccountName, name))
                    return &*i;
        return 0;
    }
    IgnoreItem *Find(int id)
    {
        for (iterator i = begin(), e = end(); i != e; ++i)
            if (id == (*i).AccountID)
                return &*i;
        return 0;
    }
};

/* a friend's invitation, as the list keeps it (0x40 bytes) */
struct FriendInvitationItem {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0x0
    int AccountID;   // 0x18
    char AccountName[32];   // 0x1c
    int AddType;   // 0x3c
};   // 0x40

/* the invitations: a vector of them (its insert and erase out of line, 0x302AA0 and 0x302850) */
class FriendInvitationVector : public std::vector<FriendInvitationItem> {
public:
    /* room for 64 at the start (the initialiser calls the reserve 0x3030B0 with 64) */
    FriendInvitationVector() { reserve(64); }
    ~FriendInvitationVector() { clear(); }
    FriendInvitationItem *Find(const char *name)
    {
        if (name)
            for (iterator i = begin(); i != end(); ++i)
                if (!strcasecmp((*i).AccountName, name))
                    return &*i;
        return 0;
    }
    FriendInvitationItem *Find(int id)
    {
        for (iterator i = begin(), e = end(); i != e; ++i)
            if (id == (*i).AccountID)
                return &*i;
        return 0;
    }
    void Add(FriendInvitationItem *item);
    void Remove(int id);
};

/* an invitation as it comes: its status ahead of the account */
struct MediusGetBuddyInvitationsResponse {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0x0
    MediusCallbackStatus StatusCode;   // 0x18
    int AccountID;   // 0x1c
    char AccountName[32];   // 0x20
    int AddType;   // 0x40
    char EndOfList;   // 0x44
};

/* a player looked for by name or number */
struct MediusFindPlayerRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0x0
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];   // 0x15
    int SearchType;   // 0x28: 1 by name
    int ID;   // 0x2c
    char Name[32];   // 0x30
};   // 0x50

struct MediusFindPlayerResponse {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0x0
    MediusCallbackStatus StatusCode;   // 0x18
    int ApplicationID;   // 0x1c
    char ApplicationName[32];   // 0x20
    int ApplicationType;   // 0x40
    int MediusWorldID;   // 0x44
    int AccountID;   // 0x48
    char AccountName[32];   // 0x4c
    char EndOfList;   // 0x6c
};

/* a friend asked for: the account and how */
struct MediusAddToBuddyListConfirmationRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0x0
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];   // 0x15
    int TargetAccountID;   // 0x28
    int AddType;   // 0x2c
};   // 0x30

void cbFindBuddyListPlayerID(int, int, int, MediusFindPlayerResponse *resp);
void cbRemoveFromIgnoreList(int, int, int, MediusStatusResponse *resp);
void cbAddToIgnoreList(int, int, int, MediusStatusResponse *resp);
void cbFindIgnoreListPlayerID(int, int, int, MediusAccountGetIDResponse *resp);

struct AccountInfoType {
    int AccountID;   // 0x0
    char AccountName[32];   // 0x4
};

extern "C" {
extern AccountInfoType AccountInfo;
extern IgnoreVector data_0044FDE0;   /* the ignored */
extern FriendInvitationVector data_0044FE00;   /* the friend invitations */
void func_00300BE0(int, int, int, MediusStatusResponse *);   /* the confirmation's callback */
MediusErrorCode MediusFindPlayer(MediusFindPlayerRequest *req,
                              void (*cb)(int, int, int, MediusFindPlayerResponse *));   /* MediusFindPlayer? */
MediusErrorCode MediusBuddyGetPermission(MediusAddToBuddyListConfirmationRequest *req,
                              void (*cb)(int, int, int, MediusStatusResponse *));

MediusErrorCode MediusAccountGetID(MediusAccountGetIDRequest *req,
                                   void (*cb)(int, int, int, MediusAccountGetIDResponse *));
MediusErrorCode MediusAddToIgnoreList(MediusAccountRequest *req,
                              void (*cb)(int, int, int, MediusStatusResponse *));   /* add to the ignored */
MediusErrorCode MediusRemoveFromIgnoreList(MediusAccountRequest *req,
                              void (*cb)(int, int, int, MediusStatusResponse *));   /* remove from them */

}

/* the unit's three lists, in the order its static initialiser (0x402420) builds them and registers
 * their destructors (0x303410, 0x303390, 0x303310: the unit's last functions but the erase) */
FriendVector FriendVec;
IgnoreVector data_0044FDE0;
FriendInvitationVector data_0044FE00;

extern "C" {
void func_003017A0(int, int, int, MediusBuddyResponse *);   /* the callbacks */
void func_00301710(int, int, int, MediusBuddyResponse *);
void func_00300F70(int, int, int, MediusBuddyResponse *);
void func_00300C10(int, int, int, MediusBuddyResponse *);

/* the Medius buddy calls (unnamed) */
MediusErrorCode func_0064AE58(MediusIDRequest *req, MediusBuddyCallback cb);
MediusErrorCode MediusRemoveFromBuddyList(MediusAccountRequest *req, MediusBuddyCallback cb);
MediusErrorCode func_0064B368(MediusIDRequest *req, MediusBuddyCallback cb);
MediusErrorCode MediusGetIgnoreList(MediusGetIgnoreListRequest *req, MediusBuddyCallback cb);
MediusErrorCode MediusAddToBuddyList(MediusAccountRequest *req, MediusBuddyCallback cb);
MediusErrorCode MediusBuddyAddConfirmation(MediusBuddyConfirmationResponse *req);
}

/* The friends' list members, defined first: in the image they follow the buddy code (0x3020A0,
 * 0x302210), ahead of the other two lists' template instances. Pinned: never inlined. */
#pragma auto_inline off

/* the friends deleted and the list emptied (OVL1 0x302210) */
void FriendVector::DeleteAll()
{
    std::transform(begin(), end(), begin(), DeleteItem<FriendItem>());
    clear();
}

/* a friend kept, unless already there: the response copied whole (OVL1 0x3020A0) */
void FriendVector::Add(MediusGetBuddyList_ExtraInfoResponse *resp)
{
    if (!Find(resp->AccountID))
        push_back(new FriendItem(*resp));
}

#pragma auto_inline reset

/* a friend by name, the null test inside (OVL1 0x301C40: out of line for the state handler 0x2F0020,
 * inlined in AddFriend) */
FriendItem *FriendVector::Find(const char *name)
{
    if (name)
        for (iterator i = begin(); i != Last(); ++i)
            if (!strcasecmp((*i)->AccountName, name))
                return *i;
    return 0;
}

/* the named player on the ignore list (the persona code asks it of theLobby; OVL1 0x3008E0) */
bool CZOnlineLobby::IsIgnored(const char *name)
{
    bool ignored = false;

    if (name && name[0])
        ignored = data_0044FDE0.Find(name) != 0;
    return ignored;
}

/* the Medius library's set up: three callbacks and a timeout (SOCOM II's; OVL1 0x300840; in this
 * unit, ahead of the buddy code, by its place) */
struct MediusErrorMessage;

struct MediusInitializeParams {
    void (*cb0)(int, int, int, void *);   // 0x0
    void (*cb1)(int, int, int, void *);   // 0x4
    void (*ErrorCallback)(int, int, int, MediusErrorMessage *);   // 0x8
    int _c;   // 0xc
    int Timeout;   // 0x10
    int _14;
    int _18;
};

void cbErrorMessage(int, int, int, MediusErrorMessage *);

extern "C" {
void func_002F4EF0(int, int, int, void *);
void func_003009C0(int, int, int, void *);
MediusErrorCode func_00647318(MediusInitializeParams *p, int *handle);   /* MediusInitialize? */
}

bool MediusInit()
{
    int handle;
    MediusInitializeParams p;

    p.cb0 = func_002F4EF0;
    p.cb1 = func_003009C0;
    p.ErrorCallback = cbErrorMessage;
    p.Timeout = 30000;
    bool ok = CheckMediusError(func_00647318(&p, &handle));

    return ok;
}

/* the friends emptied and asked for (OVL1 0x301750): the call's result not looked at */
void CZOnlineLobby::GetFriendsList()
{
    FriendVec.DeleteAll();
    MediusIDRequest req;

    memset(&req, 0, sizeof(req));
    func_0064AE58(&req, func_003017A0);
    CheckMediusError(MediusSuccess);
}

/* the player to befriend found: asked to confirm, the menu told (OVL1 0x3015D0) */
void cbFindBuddyListPlayerID(int, int, int, MediusFindPlayerResponse *resp)
{
    if (CheckMediusCallbackStatus("FindBuddyListPlayerID", resp->StatusCode)) {
        int id = resp->AccountID;
        MediusAddToBuddyListConfirmationRequest req;

        memset(&req, 0, sizeof(req));
        SetSessionKey(req.SessionKey);
        req.TargetAccountID = id;
        req.AddType = 0;
        CheckMediusError(MediusBuddyGetPermission(&req, func_00300BE0));
        LobbyEvent("MP_FRIEND_CONFIRMATION_REQUESTED", 0);
    } else {
        LobbyEvent("MP_PLAYER_NOT_FOUND", 0);
    }
}

/* the named player asked to be a friend -- not one already, not invited, not ourselves ('AddFriend'
 * 0x27A050 calls it on theLobby; OVL1 0x3013C0): looked for by name first.
 * OPEN, 3 instructions (0x204 both): the finds match through FriendVector::Last (see zonline.h)
 * and the event is written twice, as the target has it; the first copy's null test branches past
 * a jump block to the epilogue where the target's goes straight there, its call followed by the
 * jump. Tried: the event expanded with the return inside its test, if/else arms throughout. */
void CZOnlineLobby::AddFriend(const char *name)
{
    if (name) {
        FriendItem *f = FriendVec.Find(name);

        if (!f) {
            FriendInvitationItem *inv = data_0044FE00.Find(name);

            if (!inv) {
                if (strcasecmp(name, AccountInfo.AccountName)) {
                    MediusFindPlayerRequest req;

                    memset(&req, 0, sizeof(req));
                    SetSessionKey(req.SessionKey);
                    strncpy(req.Name, name, sizeof(req.Name));
                    req.SearchType = 1;
                    CheckMediusError(MediusFindPlayer(&req, cbFindBuddyListPlayerID));
                    return;
                }
                LobbyEvent("MP_PLAYER_IS_FRIEND", 0);
                return;
            }
        }
    }
    LobbyEvent("MP_PLAYER_IS_FRIEND", 0);
}

/* an invitation received kept (GetFriendInvitations' callback, its status not looked at; OVL1
 * 0x301350) */
void cbGetFriendInvitations(int, int, int, MediusGetBuddyInvitationsResponse *resp)
{
    FriendInvitationItem item;

    if (resp->MessageID)
        memcpy(item.MessageID, resp->MessageID, sizeof(item.MessageID));
    item.AccountID = resp->AccountID;
    if (resp->AccountName)
        memcpy(item.AccountName, resp->AccountName, sizeof(item.AccountName));
    item.AddType = resp->AddType;
    data_0044FE00.Add(&item);
}

/* a friend dropped (OVL1 0x3016A0) */
void CZOnlineLobby::RemoveFriend(int id)
{
    MediusAccountRequest req;

    memset(&req, 0, sizeof(req));
    SetSessionKey(req.SessionKey);
    req.AccountID = id;
    MediusRemoveFromBuddyList(&req, func_00301710);
    CheckMediusError(MediusSuccess);
}

/* the friend invitations asked for, in the active lobby (OVL1 0x3012F0) */
void CZOnlineLobby::GetFriendInvitations()
{
    if (IsActive()) {
        MediusIDRequest req;

        memset(&req, 0, sizeof(req));
        CheckMediusError(func_0064B368(&req, (MediusBuddyCallback)cbGetFriendInvitations));
    }
}

/* the ignored account let go of: the list asked for again (OVL1 0x3010C0; the name ours, after
 * the status it reports) */
void cbRemoveFromIgnoreList(int, int, int, MediusStatusResponse *resp)
{
    if (CheckMediusCallbackStatus("RemoveFromIgnoreList", resp->StatusCode))
        theLobby.GetIgnoreList();
}

/* an ignored account let go of, if it is one ('RemoveFromIgnore' 0x2798E0 calls it on theLobby
 * with atoi of the argument; OVL1 0x300FE0) */
void CZOnlineLobby::RemoveFromIgnore(int id)
{
    if (data_0044FDE0.Find(id)) {
        MediusAccountRequest req;

        memset(&req, 0, sizeof(req));
        SetSessionKey(req.SessionKey);
        req.AccountID = id;
        CheckMediusError(MediusRemoveFromIgnoreList(&req, cbRemoveFromIgnoreList));
    }
}

/* the ignore list emptied and asked for (OVL1 0x300F00) */
void CZOnlineLobby::GetIgnoreList()
{
    data_0044FDE0.clear();
    MediusGetIgnoreListRequest req;

    memset(&req, 0, sizeof(req));
    SetSessionKey(req.SessionKey);
    CheckMediusError(MediusGetIgnoreList(&req, func_00300F70));
}

/* the account ignored: the list asked for again, or the menu told it is full (OVL1 0x300E60) */
void cbAddToIgnoreList(int, int, int, MediusStatusResponse *resp)
{
    if (CheckMediusCallbackStatus("AddToIgnoreList", resp->StatusCode))
        theLobby.GetIgnoreList();
    else
        LobbyEvent("MP_IGNORE_LIST_FULL", 0);
}

/* the account to ignore found: ignored (OVL1 0x300DB0) */
void cbFindIgnoreListPlayerID(int, int, int, MediusAccountGetIDResponse *resp)
{
    if (CheckMediusCallbackStatus("FindIgnoreListPlayerID", resp->StatusCode)) {
        MediusAccountRequest req;

        memset(&req, 0, sizeof(req));
        SetSessionKey(req.SessionKey);
        req.AccountID = resp->AccountID;
        CheckMediusError(MediusAddToIgnoreList(&req, cbAddToIgnoreList));
    } else {
        LobbyEvent("MP_PLAYER_NOT_FOUND", 0);
    }
}

/* the named player ignored, unless already ('AddToIgnore' 0x279F50 calls it on theLobby; OVL1
 * 0x300C50): the account's number asked for first */
void CZOnlineLobby::AddToIgnore(const char *name)
{
    if (name && name[0]) {
        if (!data_0044FDE0.Find(name)) {
            MediusAccountGetIDRequest req;

            memset(&req, 0, sizeof(req));
            SetSessionKey(req.SessionKey);
            strncpy(req.AccountName, name, sizeof(req.AccountName));
            CheckMediusError(MediusAccountGetID(&req, cbFindIgnoreListPlayerID));
            return;
        }
        LobbyEvent("PLAYER_IS_IGNORED", 0);
    }
}

/* an account made a friend at once (OVL1 0x300B70) */
void CZOnlineLobby::AddFriendDirectly(int id)
{
    MediusAccountRequest req;

    memset(&req, 0, sizeof(req));
    SetSessionKey(req.SessionKey);
    req.AccountID = id;
    CheckMediusError(MediusAddToBuddyList(&req, func_00300C10));
}

/* a friend's request refused, and its invitation dropped (OVL1 0x300AF0) */
void CZOnlineLobby::RejectFriend(int id)
{
    MediusBuddyConfirmationResponse req;

    memset(&req, 0, sizeof(req));
    SetSessionKey(req.SessionKey);
    req.StatusCode = -985;
    req.TargetAccountID = id;
    req.AddType = 0;
    CheckMediusError(MediusBuddyAddConfirmation(&req));
    data_0044FE00.Remove(id);
}

/* a friend's request accepted, and its invitation dropped (OVL1 0x300A70) */
void CZOnlineLobby::ConfirmFriend(int id)
{
    MediusBuddyConfirmationResponse req;

    memset(&req, 0, sizeof(req));
    SetSessionKey(req.SessionKey);
    req.StatusCode = 2;
    req.TargetAccountID = id;
    req.AddType = 0;
    CheckMediusError(MediusBuddyAddConfirmation(&req));
    data_0044FE00.Remove(id);
}

extern "C" void func_00301810(FriendInvitationVector *list);   /* the invitations shown again */

/* The invitations' members its callers call out of line. Pinned: never inlined */
#pragma auto_inline off

/* an invitation kept, unless it is empty or its account's is there, and the list shown again
 * (OVL1 0x301DB0): copied by assignment first -- the odd-length id makes it member by member,
 * as the target's copy is (initialised from *item it would be a block copy) */
void FriendInvitationVector::Add(FriendInvitationItem *item)
{
    if (item && item->AccountName[0] && !Find(item->AccountID)) {
        FriendInvitationItem copy;

        copy = *item;

        push_back(copy);
        func_00301810(this);
    }
}

/* every invitation from the account dropped, and the list shown again (OVL1 0x301CF0).
 * OPEN, 35 instructions (0xC4 against 0xBC): the target keeps the assigned iterator's value in v0
 * across the loop -- the guard tests the value it just stored, the step is `addiu v0,v0,64` in the
 * found test's delay slot -- while re-reading the stored iterator for the dereference; MW here
 * reloads it for the guard and steps the reloaded copy behind a `b`. Assigned or initialised,
 * prefix or postfix ++, (*i). or i->, while or for: all the same 35 */
void FriendInvitationVector::Remove(int id)
{
    iterator i;

    i = begin();
    while (i != end()) {
        if (id == (*i).AccountID)
            i = erase(i);
        else
            ++i;
    }
    func_00301810(this);
}

#pragma auto_inline reset

/* a state of the game's stack, as far as the player is told things (Apps/FTS/fts_state.h's
 * CGameState: its first ten slots, DisplayMessage at 0x2C) */
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
                                unsigned char d, unsigned char e, char type);
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

/* an invitation to be a friend received: kept, and the player told (OVL1 0x3009C0: MediusInit's
 * second callback; its response is the invitation's record) */
void func_003009C0(int, int, int, void *data)
{
    FriendInvitationItem *resp = (FriendInvitationItem *)data;
    char text[512];

    data_0044FE00.Add(resp);
    sprintf(text, "%s has invited you to join their friends list.", resp->AccountName);
    theGame.GetCurrentState()->DisplayMessage(text, "", 0, 0, 0, 1, 0, 1);
}

/* --- the Medius connection (the unit's first functions, 0x300670-0x3007FF) ------------------ */

/* a connection's parameters (the chat unit's MediusConnectParams: only the callback read here) */
struct FriendsConnectParams {
    unsigned char _0[0xf4];
    void (*Callback)(int index);   // 0xf4
    int _f8;   // 0xf8
    int _fc;   // 0xfc
};   // 0x100

/* what opening a connection hands back: its index, or -1 and the error */
struct FriendsConnectResult {
    int index;   // 0x0
    int err;   // 0x4
};

extern "C" {
extern int myConnectionIndex;   /* the Medius connection, -1 none (SOCOM 1's) */
extern unsigned char data_0044FDA0;   /* the kind of server it is to: 1 the lobby, 2 the universes' */
extern void (*data_0044FDA8)(int index);   /* the caller's callback, called once connected */
MediusErrorCode func_006479B8();   /* (OVL2: the connection closed) */
MediusErrorCode func_006476E0(FriendsConnectParams *params, FriendsConnectResult *result);   /* (OVL2: one opened) */
void func_002F3FF0(void *params);   /* (the chat unit's) */
void func_00300800(int index);   /* the connection made: the caller's callback called */

/* a connection asked for: the one open closed first (a lobby's through Medius), the caller's
 * callback kept and ours put in its place (OVL1 0x300740: the lobby's and the universes'
 * connect handlers call it) */
bool func_00300740(int kind, FriendsConnectParams *params)
{
    FriendsConnectResult result;

    if (myConnectionIndex != -1) {
        if (data_0044FDA0 == 1)
            CheckMediusError(func_006479B8());
        myConnectionIndex = -1;
    }
    data_0044FDA0 = kind;
    func_002F3FF0(params);
    data_0044FDA8 = params->Callback;
    params->Callback = func_00300800;
    myConnectionIndex = -1;
    return CheckMediusError(func_006476E0(params, &result));
}
}
