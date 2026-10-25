/* zNetMediusRoom.cpp -- the lobby's rooms and games: the room and game lists, joining a room,
 * the games' details (SOCOM 1's zNetMedius.cpp's BriefingRoom and RoomGame code; the unit and its
 * name are ours: OVL1 0x2E1CE0-0x2E5FDF, after the lobby's own unit, its static initialiser
 * 0x401E70 building the room list 0x44F568, whose destructor 0x2E5F60 closes it). Only some of it
 * so far. Defined in reverse of their image order (-inline deferred). */

#include "zonline.h"
#include "../zsys/zsys.h"

void _free(void *p, const char *file, int line);

/* a page of the room list (no session key; the page and its size as halfwords) */
struct MediusChannelListRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0x0
    short PageID;   // 0x16
    short PageSize;   // 0x18
};   // 0x1a

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
extern BriefingRoomDB data_0044F568;   /* the room list */
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
void func_002F4080(void *msg, int type);   /* a request put into the network's order (empty) */
MediusErrorCode func_0064B008(MediusGameListRequest *req,
                              void (*cb)(int, int, int, MediusGameListResponse *));   /* MediusGameList */
void func_002E2B00(int, int, int, MediusGameListResponse *);   /* a game of the list kept */

void func_002E2920(CZOnlineLobby *lobby, C2D *page);   /* the game list asked for */
void func_002E3AA0(CZOnlineLobby *lobby, C2D *page);   /* the room list asked for */
}

/* a room torn down: its games deleted, the password freed (SOCOM 1's; OVL1 0x2E48C0, the LAN
 * room's initialiser registers it). Byte-exact but for one callee's name: the chat vector's
 * deleter destructor it calls is an earlier unit's copy, OVL1 0x1F5C30, still a placeholder outside
 * this lane -- once that row is __dt__Q23std82__vector_deleter<20MediusChatFwdMessage,Q23std33
 * allocator<20MediusChatFwdMessage>>Fv, 0x2E48C0 takes this name and the draft goes. Pinned:
 * DeleteRoom calls it out of line */
#pragma auto_inline off
BriefingRoom::~BriefingRoom()
{
    DeleteGames();
    if (m_password)
        _free(m_password, 0, 0);
    m_password = 0;
}
#pragma auto_inline reset

/* the room's password (empty in SOCOM II; OVL1 0x2E48B0: the constructor calls it with "") */
void BriefingRoom::SetPassword(const char *password)
{
}

/* a game forgotten when nobody is in it (0x2E4700's remove_if predicate; OVL1 0x2E4790) */
bool RemoveEmptyGame(RoomGame *g)
{
    bool empty = *(unsigned short *)&g->m_info[0x20] == 0;

    if (empty)
        delete g;
    return empty;
}

/* the games nobody is in dropped (OVL1 0x2E4700: the game list's request calls it out of line,
 * pinned).
 * OPEN, 20 instructions, the same 0x84 bytes: include/vector's erase/end question, as
 * CCharacterType::CleanupTemporary's (OVL2 0x53DF50) -- the target computes end() once, stores it
 * for remove_if's last (48(sp)) and for erase's (60(sp)), and copies the latter into erase's
 * parameter (40(sp)) after the call; this copies erase's last into its parameter before
 * evaluating remove_if. An end in a local passed to both is 0x70 (no copy at all) */
#pragma auto_inline off
void BriefingRoom::RemoveEmptyGames()
{
    erase(std::remove_if(begin(), end(), RemoveEmptyGame), end());
}
#pragma auto_inline reset

/* a game forgotten when the new list did not report it (0x2E4630's predicate; OVL1 0x2E46C0) */
bool RemoveUncheckedGame(RoomGame *g)
{
    bool unchecked = g->m_unchecked;

    if (unchecked)
        delete g;
    return unchecked;
}

/* the games the new list did not report dropped (OVL1 0x2E4630: pinned, as RemoveEmptyGames).
 * OPEN, 20 instructions: RemoveEmptyGames' (see there) */
#pragma auto_inline off
void BriefingRoom::RemoveUncheckedGames()
{
    erase(std::remove_if(begin(), end(), RemoveUncheckedGame), end());
}
#pragma auto_inline reset

/* the room's game by name, if it has one (SOCOM 1's BriefingRoom::FindGame; OVL1 0x2E4570) */
RoomGame *BriefingRoom::FindGame(const char *name)
{
    RoomGame *game = 0;

    if (name) {
        for (iterator i = begin(); i != end(); ++i) {
            if (!strcmp((char *)&(*i)->m_info[0x4c], name)) {
                game = *i;
                break;
            }
        }
    }
    return game;
}

/* the room's game by its mission number (OVL1 0x2E4410) */
RoomGame *BriefingRoom::FindGame(int mission)
{
    RoomGame *game = 0;

    for (iterator i = begin(); i != end(); ++i) {
        if (mission == *(int *)&(*i)->m_info[0x28]) {
            game = *i;
            break;
        }
    }
    return game;
}

/* a room of the channel list's added (OVL1 0x2E4240) */
void BriefingRoomDB::Add(MediusChannelListResponse *resp)
{
    BriefingRoom *room = 0;

    room = new BriefingRoom(resp);
    push_back(room);
}

/* a room deleted and its slot cleared: DeleteAll hands it to std::transform by address, which
 * makes it a direct call (OVL1 0x2E4220) */
extern "C" void func_002E48C0(BriefingRoom *room, int flag);   /* ~BriefingRoom (see there) */

#pragma auto_inline off
BriefingRoom *DeleteRoom(BriefingRoom *room)
{
    func_002E48C0(room, 1);
    return 0;
}
#pragma auto_inline reset

/* the rooms deleted and the list emptied (OVL1 0x2E4140: the room list's request and Release call
 * it out of line, pinned) */
#pragma auto_inline off
void BriefingRoomDB::DeleteAll()
{
    std::transform(begin(), end(), begin(), DeleteRoom);
    clear();
}
#pragma auto_inline reset

/* the rooms deleted and the list's storage given up (OVL1 0x2E4010: ReleaseUIResources calls it).
 * zsys.h's FreeVector on the base vector: six high, inlined one level, its four pieces called out
 * of line (their copies follow, 0x2E4060-0x2E4120) */
void BriefingRoomDB::Release()
{
    DeleteAll();
    FreeVector<std::vector<BriefingRoom *> >(*this);
}

/* the room list's order: by the rooms' sort key, then the names' lengths, then the names (OVL1
 * 0x2E3F80) */
bool RoomLess(BriefingRoom *a, BriefingRoom *b)
{
    unsigned int ka = a->GenericField >> 16;
    unsigned int kb = b->GenericField >> 16;

    if (ka != kb)
        return ka < kb;
    unsigned int la = strlen(a->LobbyName);
    unsigned int lb = strlen(b->LobbyName);

    if (la != lb)
        return la < lb;
    int c = strcasecmp(a->LobbyName, b->LobbyName);

    return c < 0;
}

/* a player of the room by name (for find_if; the name ours) */
struct RoomPlayerNamed {
    RoomPlayerNamed(const char *name) : m_name(name) {}
    bool operator()(const RoomPlayer &p) const { return !strcmp(p.AccountName, m_name); }

    const char *m_name;
};

/* the room's player of that name, if it holds one (OVL1 0x2E3610: the player-info callback
 * 0x2F0020 asks it) */
RoomPlayer *BriefingRoom::FindPlayer(const char *name)
{
    RoomPlayer *player = 0;
    std::vector<RoomPlayer>::iterator i = std::find_if(m_players.begin(), m_players.end(),
                                                       RoomPlayerNamed(name));

    if (i != m_players.end())
        player = &*i;
    return player;
}

/* the room of that name lets us in: its game types allow our rank's (SOCOM II's; OVL1 0x2E34E0,
 * UICanJoinRoom asks it of theLobby) */
bool CZOnlineLobby::CanJoinRoom(const char *name)
{
    bool ok = false;
    BriefingRoom *room = data_0044F568.Find(name);

    if (room)
        ok = room->AllowsType(GetRankTier(-1));
    return ok;
}

/* the mission list (the states lane's fts_missionlist.cpp, data 0x441650) */
class CMultiMission;
class CMultiMissionList {
public:
    CMultiMission *Find(int id, bool multi);
};
extern "C" CMultiMissionList data_00441650;

/* every mission of the game's sequence known here (OVL1 0x2E33E0: JoinGame asks it) */
bool RoomGame::HasMissions()
{
    for (int i = 0; i < 32 && GetMissionSequence()[i]; i++)
        if (!data_00441650.Find(GetMissionSequence()[i], true))
            return false;
    return true;
}

/* a room's game types allow this one, or the room names none (OVL1 0x2E3490) */
bool RoomGame::AllowsType(int type)
{
    bool r = true;

    if (m_types) {
        if (m_types & (1 << (type - 1)))
            r = true;
        else
            r = false;
    }
    return r;
}

/* a room's games deleted and the list emptied (OVL1 0x2E47D0, among the unit's room-list
 * members): the current room's too, from ReleaseUIResources. Pinned: never inlined */
#pragma auto_inline off
void BriefingRoom::DeleteGames()
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
    BriefingRoom *room = data_0044F568.FindRoom(RoomVarText(theLobby.uiv_LobbyWorldNameVar));

    if (room)
        room->RemoveEmptyGames();
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
    room = data_0044F568.FindRoom(RoomVarText(theLobby.uiv_LobbyWorldNameVar));
    if (room) {
        room->RemoveUncheckedGames();
        for (BriefingRoom::iterator i = room->begin(); i != room->end(); ++i)
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
    data_0044F568.DeleteAll();
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
            if (!strcmp((*i)->LobbyName, name)) {
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
