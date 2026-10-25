/* zNetMediusRoom.cpp -- the lobby's rooms and games: the room and game lists, joining a room,
 * the games' details (SOCOM 1's zNetMedius.cpp's BriefingRoom and RoomGame code; the unit and its
 * name are ours: OVL1 0x2E1CE0-0x2E5FDF, after the lobby's own unit, its static initialiser
 * 0x401E70 building the room list 0x44F568, whose destructor 0x2E5F60 closes it). Only some of it
 * so far. Defined in reverse of their image order (-inline deferred). */

#include "zonline.h"

/* a page of the room list (no session key; the page and its size as halfwords) */
struct MediusChannelListRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0x0
    short PageID;   // 0x16
    short PageSize;   // 0x18
};   // 0x1a

struct MediusChannelListResponse { MEDIUS_RESPONSE_HEAD };

/* a page of the game list (no session key; the page and its size as halfwords) */
struct MediusGameListRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0x0
    short PageID;   // 0x16
    short PageSize;   // 0x18
};   // 0x1a

struct MediusGameListResponse { MEDIUS_RESPONSE_HEAD };

/* a valve: a named 16-bit value (SOCOM 1's CValve; only what is read here) */
enum VALVE_TYPE {
    VALVE_TYPE_0,
    VALVE_TYPE_1
};

class CValve {
public:
    static CValve *Create(const char *name, VALVE_TYPE type);

    char *m_name;   // 0x0
    unsigned short m_value;   // 0x4
    unsigned short m_flags;   // 0x6
};

/* a variable's text when it holds one (zNetMediusClan.cpp's ZString) */
inline char *RoomVarText(_zrdr *z)
{
    return z->type == 3 ? z->string : 0;
}

extern "C" {
extern unsigned char data_0044F568[];   /* the room list */
void func_002E4140(void *rooms);   /* the room list emptied */
MediusErrorCode func_0064B098(MediusChannelListRequest *req,
                              void (*cb)(int, int, int, MediusChannelListResponse *));   /* MediusChannelList */
void func_002E3B00(int, int, int, MediusChannelListResponse *);   /* a room of the list kept */
extern int LobbyWorldID;   /* the room we are in, -1 none (SOCOM 1's) */
/* the game list's filters, from the menu's valves (the unit's statics) */
extern int data_0044F528;
extern int data_0044F530;
extern int data_0044F538;
extern int data_0044F540;
extern int data_0044F548;
extern int data_0044F550;
void func_002E4700(BriefingRoom *room);   /* the room's games emptied */
void func_002E4630(BriefingRoom *room);   /* the room's games kept for the new list */
void func_002F4080(void *msg, int type);   /* a request put into the network's order (empty) */
MediusErrorCode func_0064B008(MediusGameListRequest *req,
                              void (*cb)(int, int, int, MediusGameListResponse *));   /* MediusGameList */
void func_002E2B00(int, int, int, MediusGameListResponse *);   /* a game of the list kept */

void func_002E2920(CZOnlineLobby *lobby, C2D *page);   /* the game list asked for */
void func_002E3AA0(CZOnlineLobby *lobby, C2D *page);   /* the room list asked for */
}

/* a room's games deleted and the list emptied (OVL1 0x2E47D0, among the unit's room-list
 * members): the current room's too, from ReleaseUIResources. Pinned: never inlined */
#pragma auto_inline off
void RoomGameVector::DeleteAll()
{
    std::transform(begin(), end(), begin(), DeleteItem<RoomGame>());
    clear();
}
#pragma auto_inline reset

/* the room list asked for again, in the active lobby (SOCOM 1's; OVL1 0x2E3A30). The page is
 * taken into a local zeroed before the first test (the target's zero in that test's delay slot) */
bool CZOnlineLobby::RefreshRoomList(C2D *page)
{
    C2D *p = 0;

    if (!m_isopen)
        return false;
    if (m_current_state != LOBBY_ACTIVE)
        return false;
    if (page)
        p = page;
    func_002E3AA0(this, p);
    return true;
}

/* the game list asked for again: the room's games emptied, the menu's filters kept, a first page
 * of 256 asked for with every game of the room marked to be checked (SOCOM 1's GetGameList; OVL1
 * 0x2E2920 -- the page it is shown on is not used). RefreshGameList calls it out of line: pinned */
#pragma auto_inline off
void func_002E2920(CZOnlineLobby *lobby, C2D *page)
{
    BriefingRoom *room = ((BriefingRoomDB *)data_0044F568)->FindRoom(RoomVarText(theLobby.uiv_LobbyWorldNameVar));

    if (room)
        func_002E4700(room);
    data_0044F528 = CValve::Create("GameTypeFilter", VALVE_TYPE_1)->m_value;
    data_0044F530 = CValve::Create("GamePlayerFilter", VALVE_TYPE_1)->m_value;
    data_0044F538 = CValve::Create("GameSpeedFilter", VALVE_TYPE_1)->m_value;
    data_0044F540 = CValve::Create("GamePasswordFilter", VALVE_TYPE_1)->m_value;
    data_0044F548 = CValve::Create("FF_FILTER", VALVE_TYPE_1)->m_value;
    data_0044F550 = CValve::Create("LockedFilter", VALVE_TYPE_1)->m_value;
    MediusGameListRequest req;

    memset(&req, 0, sizeof(req));
    req.PageID = 1;
    req.PageSize = 256;
    room = ((BriefingRoomDB *)data_0044F568)->FindRoom(RoomVarText(theLobby.uiv_LobbyWorldNameVar));
    if (room) {
        func_002E4630(room);
        for (RoomGameVector::iterator i = room->m_games.begin(); i != room->m_games.end(); ++i)
            (*i)->m_unchecked = 1;
    }
    func_002F4080(&req, 129);
    CheckMediusError(func_0064B008(&req, func_002E2B00));
}
#pragma auto_inline reset

/* the room list asked for again: the list emptied, then a first page of 999 (SOCOM 1's GetRoomList;
 * OVL1 0x2E3AA0 -- the page it is shown on is not used). RefreshRoomList calls it out of line:
 * pinned */
#pragma auto_inline off
void func_002E3AA0(CZOnlineLobby *lobby, C2D *page)
{
    func_002E4140(data_0044F568);
    MediusChannelListRequest req;

    memset(&req, 0, sizeof(req));
    req.PageID = 1;
    req.PageSize = 999;
    CheckMediusError(func_0064B098(&req, func_002E3B00));
}
#pragma auto_inline reset

/* the room of that name, if the list has one (SOCOM 1's; OVL1 0x2E4470: the chat unit's
 * CreateRoom handler and callback call it; the game list's request calls it out of line: pinned) */
#pragma auto_inline off
BriefingRoom *BriefingRoomDB::FindRoom(const char *name)
{
    BriefingRoom *room = 0;

    if (name) {
        for (iterator i = begin(); i != end(); ++i) {
            if (!strcmp((*i)->m_name, name)) {
                room = *i;
                break;
            }
        }
    }
    return room;
}
#pragma auto_inline reset

/* back to the room we were in, when there is one (SOCOM 1's; OVL1 0x2E3730): `> -1` branches on
 * an slti as the target does, `>= 0` on bltz */
bool CZOnlineLobby::RejoinRoom()
{
    if (!m_isopen)
        return false;
    if (LobbyWorldID > -1)
        ChangeState(LOBBY_ACTIVE);
    return true;
}

/* the game list asked for again (SOCOM 1's; OVL1 0x2E28D0) */
bool CZOnlineLobby::RefreshGameList(C2D *page)
{
    C2D *p = 0;

    if (!m_isopen)
        return false;
    if (page)
        p = page;
    func_002E2920(this, p);
    return true;
}
