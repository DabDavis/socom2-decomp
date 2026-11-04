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

/* someone ignored, as the server answers (0x48 bytes; the name ours) */
struct IgnoreInfo {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0x0
    MediusCallbackStatus StatusCode;   // 0x18
    int AccountID;   // 0x1c
    char AccountName[32];   // 0x20
    int _40;
    char EndOfList;   // 0x44
};   // 0x48
/* the ignore list's entry (a vector of them: the class vector's clear out of line, 0x302FC0): the
 * answer held as a MEMBER -- MW's implicit assignment copies a struct's char arrays member by member
 * but a member of struct type as one block, so the list's erases (0x303490) and insert (0x3022F0)
 * move whole 72-byte blocks, as the target's do (the record itself, or the entry derived from it,
 * copies its arrays bytewise: 0x528 against 0x2A0). Built from an answer by assigning it (Add's copy:
 * member by member, no memset) */
struct IgnoreItem {
    /* a blank entry (resize's T(): the memset call 0x303250 makes) */
    IgnoreItem() { memset(this, 0, sizeof(*this)); }
    IgnoreItem(const IgnoreInfo &info) { m_info = info; }

    IgnoreInfo m_info;   // 0x0
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
                if (!strcasecmp((*i).m_info.AccountName, name))
                    return &*i;
        return 0;
    }
    IgnoreItem *Find(int id)
    {
        for (iterator i = begin(), e = end(); i != e; ++i)
            if (id == (*i).m_info.AccountID)
                return &*i;
        return 0;
    }
    void Add(IgnoreItem *item);
    void UpdateDisplay();
};

/* a friend's invitation, as the server answers (0x40 bytes; the name ours) */
struct FriendInvitationInfo {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0x0
    int AccountID;   // 0x18
    char AccountName[32];   // 0x1c
    int AddType;   // 0x3c
};   // 0x40
/* the invitations' entry: the answer held as a member, so the vector's erase (0x302850) and insert
 * (0x302AA0) move whole 64-byte blocks as the target's do, where a copy of the answer itself (Add's)
 * goes member by member (see IgnoreItem) */
struct FriendInvitationItem {
    FriendInvitationInfo m_info;   // 0x0
};   // 0x40

/* the invitations: a vector of them (its insert and erase out of line, 0x302AA0 and 0x302850; the
 * erase byte-exact, the insert OPEN, 27 instructions -- the class insert's in-place registers,
 * include/vector's open: the end pointer takes a0 where the target's takes v1, as the room players'
 * 0x2F2D90 and the ignore list's 0x3022F0 (38)) */
class FriendInvitationVector : public std::vector<FriendInvitationItem> {
public:
    /* room for 64 at the start (the initialiser calls the reserve 0x3030B0 with 64) */
    FriendInvitationVector() { reserve(64); }
    ~FriendInvitationVector() { clear(); }
    FriendInvitationItem *Find(const char *name)
    {
        if (name)
            for (iterator i = begin(); i != end(); ++i)
                if (!strcasecmp((*i).m_info.AccountName, name))
                    return &*i;
        return 0;
    }
    FriendInvitationItem *Find(int id)
    {
        for (iterator i = begin(), e = end(); i != e; ++i)
            if (id == (*i).m_info.AccountID)
                return &*i;
        return 0;
    }
    void Add(FriendInvitationItem *item);
    void Remove(int id);
    void UpdateDisplay();
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
    void (*cb0)(int, int, int, MediusChatFwdMessage *);   // 0x0: the chat unit's cbChatMessage
    void (*cb1)(int, int, int, void *);   // 0x4
    void (*ErrorCallback)(int, int, int, MediusErrorMessage *);   // 0x8
    int _c;   // 0xc
    int Timeout;   // 0x10
    int _14;
    int _18;
};

void cbErrorMessage(int, int, int, MediusErrorMessage *);
void cbChatMessage(int, int, int, MediusChatFwdMessage *msg);   /* (the chat unit's) */

extern "C" {
void func_003009C0(int, int, int, void *);
MediusErrorCode MediusInitializeBare(MediusInitializeParams *p, int *handle);
}

bool MediusInit()
{
    int handle;
    MediusInitializeParams p;

    p.cb0 = cbChatMessage;
    p.cb1 = func_003009C0;
    p.ErrorCallback = cbErrorMessage;
    p.Timeout = 30000;
    bool ok = CheckMediusError(MediusInitializeBare(&p, &handle));

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
 * 0x27A050 calls it on theLobby; OVL1 0x3013C0): looked for by name first. The finds match
 * through FriendVector::Last (see zonline.h) and the event is written twice, as the target has it:
 * the three tests one `&&` chain and both events else-arms -- with the tests nested and the event
 * followed by a `return`, the first event's null test branched to a jump block instead of straight
 * to the epilogue (open since round 8, 3 instructions) */
void CZOnlineLobby::AddFriend(const char *name)
{
    if (name && !FriendVec.Find(name) && !data_0044FE00.Find(name)) {
        if (strcasecmp(name, AccountInfo.AccountName)) {
            MediusFindPlayerRequest req;

            memset(&req, 0, sizeof(req));
            SetSessionKey(req.SessionKey);
            strncpy(req.Name, name, sizeof(req.Name));
            req.SearchType = 1;
            CheckMediusError(MediusFindPlayer(&req, cbFindBuddyListPlayerID));
        } else
            LobbyEvent("MP_PLAYER_IS_FRIEND", 0);
    } else
        LobbyEvent("MP_PLAYER_IS_FRIEND", 0);
}

/* an invitation received kept (GetFriendInvitations' callback, its status not looked at; OVL1
 * 0x301350) */
void cbGetFriendInvitations(int, int, int, MediusGetBuddyInvitationsResponse *resp)
{
    FriendInvitationItem item;

    if (resp->MessageID)
        memcpy(item.m_info.MessageID, resp->MessageID, sizeof(item.m_info.MessageID));
    item.m_info.AccountID = resp->AccountID;
    if (resp->AccountName)
        memcpy(item.m_info.AccountName, resp->AccountName, sizeof(item.m_info.AccountName));
    item.m_info.AddType = resp->AddType;
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

/* a friend's whereabouts for the menu: the name, and the game and lobby it is in -- the main menu
 * when its lobby is Medius's own -- or blanks for no such friend ('GetFriendInfo' 0x279AB0 calls it
 * on theLobby with atoi of its argument; OVL1 0x301140, the name ours, after the command) */
void CZOnlineLobby::GetFriendInfo(int id)
{
    CUIVariable *name = theUIVarManager.Add("FriendInfoName", 0, UIVL_TEMPORARY);
    CUIVariable *game = theUIVarManager.Add("FriendInfoGame", 0, UIVL_TEMPORARY);
    CUIVariable *lobby = theUIVarManager.Add("FriendInfoLobby", 0, UIVL_TEMPORARY);
    FriendItem *f = FriendVec.Find(id);

    if (f) {
        *name = f->AccountName;
        if (!strncmp(f->OnlineState.LobbyName, "Medius", 6)) {
            *game = "MAIN MENU";
            *lobby = " ";
        } else {
            *game = f->OnlineState.GameName;
            *lobby = f->OnlineState.LobbyName;
        }
    } else {
        *name = "";
        *game = "";
        *lobby = "";
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

/* an account's mark in the lists: the font's online and offline glyphs (the name ours: a switch
 * with only `case 0` and `default` -- the target's beqz to the zero case, a `b` to the other) */
inline const char *OnlineMark(int status)
{
    const char *mark;

    switch (status) {
    case 0:
        mark = "\xaa";
        break;
    default:
        mark = "\xa7";
        break;
    }
    return mark;
}

/* The lists' members their callers call out of line. Pinned: never inlined */
#pragma auto_inline off

/* an account ignored kept, unless its account's is there: copied whole (OVL1 0x301ED0: the ignore
 * list's callback 0x300F70 hands it each response) -- an entry built from the answer, which assigns
 * it member by member as the target does (a copy of the entry itself would be one block). It first
 * asks whether the name is ignored -- IsIgnored inlined, its walk of this very list made -- and does
 * nothing with the answer: both outcomes go on to the test by account */
void IgnoreVector::Add(IgnoreItem *item)
{
    theLobby.IsIgnored(item->m_info.AccountName);
    if (!Find(item->m_info.AccountID)) {
        IgnoreItem copy(item->m_info);

        push_back(copy);
    }
}

/* the friends shown: each its account as text, its name, whether it is online (the font's two
 * marks) and where -- the main menu when its lobby is Medius's own, else the lobby and the game --
 * or two blanks (OVL1 0x301A70: the friends list's callback 0x3017A0 calls it; the name ours) */
void FriendVector::UpdateDisplay()
{
    CRdrEditor ed;
    char text[64];

    ed.push(0);
    for (iterator i = begin(); i != end(); ++i) {
        FriendItem *f = *i;

        sprintf(text, "%d", f->AccountID);
        ed.push(0);
        ed.addstringnode(text);
        ed.addstringnode(f->AccountName);
        ed.addstringnode(OnlineMark(f->OnlineState.ConnectStatus));
        if (f->OnlineState.ConnectStatus) {
            if (!strncmp(f->OnlineState.LobbyName, "Medius", 6)) {
                ed.addstringnode("MAIN MENU");
                ed.addstringnode(" ");
            } else {
                ed.addstringnode(f->OnlineState.LobbyName);
                ed.addstringnode(f->OnlineState.GameName);
            }
        } else {
            ed.addstringnode("");
            ed.addstringnode("");
        }
        ed.pop();
    }
    ed.pop();
    theUIVarManager.Add("FriendListVar", 0, UIVL_TEMPORARY)->Set(&ed);
}

/* the ignored shown: each its account as text, its name and its mark (OVL1 0x301920: the ignore
 * list's callback 0x300F70 calls it; the name ours) */
void IgnoreVector::UpdateDisplay()
{
    CRdrEditor ed;
    char text[64];

    ed.push(0);
    for (iterator i = begin(); i != end(); ++i) {
        sprintf(text, "%d", (*i).m_info.AccountID);
        ed.push(0);
        ed.addstringnode(text);
        ed.addstringnode((*i).m_info.AccountName);
        ed.addstringnode(OnlineMark((*i).m_info._40));
        ed.pop();
    }
    ed.pop();
    theUIVarManager.Add("IgnoreListVar", 0, UIVL_TEMPORARY)->Set(&ed);
}

/* the invitations shown: each its account as text and its name, kept for good (OVL1 0x301810; the
 * name ours) */
void FriendInvitationVector::UpdateDisplay()
{
    CRdrEditor ed;
    char text[64];

    ed.push(0);
    for (iterator i = begin(); i != end(); ++i) {
        sprintf(text, "%d", (*i).m_info.AccountID);
        ed.push(0);
        ed.addstringnode(text);
        ed.addstringnode((*i).m_info.AccountName);
        ed.pop();
    }
    ed.pop();
    theUIVarManager.Add("FriendConfirmationListVar", 0, UIVL_PERMANENT)->Set(&ed);
}

/* an invitation kept, unless it is empty or its account's is there, and the list shown again
 * (OVL1 0x301DB0): the answer copied in by assignment first -- the char arrays make it member by
 * member, as the target's copy is (the entry copied whole, or initialised from the answer, would be
 * one block) */
void FriendInvitationVector::Add(FriendInvitationItem *item)
{
    if (item && item->m_info.AccountName[0] && !Find(item->m_info.AccountID)) {
        FriendInvitationItem copy;

        copy.m_info = item->m_info;

        push_back(copy);
        UpdateDisplay();
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
        if (id == (*i).m_info.AccountID)
            i = erase(i);
        else
            ++i;
    }
    UpdateDisplay();
}

#pragma auto_inline reset

/* an invitation to be a friend received: kept, and the player told (OVL1 0x3009C0: MediusInit's
 * second callback; its response is the invitation's record) */
void func_003009C0(int, int, int, void *data)
{
    FriendInvitationItem *resp = (FriendInvitationItem *)data;
    char text[512];

    data_0044FE00.Add(resp);
    sprintf(text, "%s has invited you to join their friends list.", resp->m_info.AccountName);
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
MediusErrorCode MediusDisconnect();   /* (OVL2: the connection closed) */
MediusErrorCode MediusConnect(FriendsConnectParams *params, FriendsConnectResult *result);   /* (OVL2: one opened) */
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
            CheckMediusError(MediusDisconnect());
        myConnectionIndex = -1;
    }
    data_0044FDA0 = kind;
    func_002F3FF0(params);
    data_0044FDA8 = params->Callback;
    params->Callback = func_00300800;
    myConnectionIndex = -1;
    return CheckMediusError(MediusConnect(params, &result));
}
}

/* a clear of the friend invitations the image no longer holds (no row: a stand-in for a function
 * the link dead-stripped with the copies only it called -- its call made the deleter's clear
 * generated ahead of the deleter's destructor, which the target's 0x303040 inlines; README "Tells
 * from the weapons, round 7": dead-stripped stand-ins) */
void DeadFriendInvitationsClear()
{
    data_0044FE00.clear();
}
