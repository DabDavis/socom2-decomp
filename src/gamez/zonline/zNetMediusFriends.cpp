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

extern "C" {
extern LobbyList data_0044FDE0;   /* the ignored */
extern LobbyList data_0044FE00;   /* the friend invitations */

void func_00302FC0(LobbyList *list);   /* the ignored deleted */
void func_00301CF0(LobbyList *list, int id);   /* an invitation dropped */

void func_003017A0(int, int, int, MediusBuddyResponse *);   /* the callbacks */
void func_00301710(int, int, int, MediusBuddyResponse *);
void func_00301350(int, int, int, MediusBuddyResponse *);
void func_00300F70(int, int, int, MediusBuddyResponse *);
void func_00300C10(int, int, int, MediusBuddyResponse *);

/* the Medius buddy calls (unnamed) */
MediusErrorCode func_0064AE58(MediusIDRequest *req, MediusBuddyCallback cb);
MediusErrorCode func_00649238(MediusAccountRequest *req, MediusBuddyCallback cb);
MediusErrorCode func_0064B368(MediusIDRequest *req, MediusBuddyCallback cb);
MediusErrorCode func_00649400(MediusGetIgnoreListRequest *req, MediusBuddyCallback cb);
MediusErrorCode func_00649198(MediusAccountRequest *req, MediusBuddyCallback cb);
MediusErrorCode func_00649378(MediusBuddyConfirmationResponse *req);
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

/* a friend dropped (OVL1 0x3016A0) */
void CZOnlineLobby::RemoveFriend(int id)
{
    MediusAccountRequest req;

    memset(&req, 0, sizeof(req));
    SetSessionKey(req.SessionKey);
    req.AccountID = id;
    func_00649238(&req, func_00301710);
    CheckMediusError(MediusSuccess);
}

/* the friend invitations asked for, in the active lobby (OVL1 0x3012F0) */
void CZOnlineLobby::GetFriendInvitations()
{
    if (IsActive()) {
        MediusIDRequest req;

        memset(&req, 0, sizeof(req));
        CheckMediusError(func_0064B368(&req, func_00301350));
    }
}

/* the ignore list emptied and asked for (OVL1 0x300F00) */
void CZOnlineLobby::GetIgnoreList()
{
    func_00302FC0(&data_0044FDE0);
    MediusGetIgnoreListRequest req;

    memset(&req, 0, sizeof(req));
    SetSessionKey(req.SessionKey);
    CheckMediusError(func_00649400(&req, func_00300F70));
}

/* an account made a friend at once (OVL1 0x300B70) */
void CZOnlineLobby::AddFriendDirectly(int id)
{
    MediusAccountRequest req;

    memset(&req, 0, sizeof(req));
    SetSessionKey(req.SessionKey);
    req.AccountID = id;
    CheckMediusError(func_00649198(&req, func_00300C10));
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
    CheckMediusError(func_00649378(&req));
    func_00301CF0(&data_0044FE00, id);
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
    CheckMediusError(func_00649378(&req));
    func_00301CF0(&data_0044FE00, id);
}
