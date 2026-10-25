/* zNetLANLobby.cpp -- the LAN games: the second lobby the menus use when they came from the LAN
 * menu (the ComeFromLan valve) -- its own room of games (0x44FFC0), the game info it hosts and
 * broadcasts (0x44FE20), the games other consoles announce, and creating and joining one (new in
 * SOCOM II; the UI lane's CZLANLobby at 0x44FE10 by the UI commands' calls, the unit and every name
 * ours). OVL1 0x303730-0x30460F: after the friends unit (whose template tail ends with the
 * 72-byte elements' erase 0x303490) and before CLobbyFileIO's unit; its static initialiser 0x4024F0
 * builds the room (BriefingRoom's inline constructor) and registers its destructor 0x2E48C0.
 * Its functions read the lobby object through its global name, as the lobby's do theLobby's.
 * Defined in reverse of their image order (-inline deferred). */

#include <string.h>
#include <stdlib.h>
#include "zonline.h"
#include "../znet/znet.h"

/* a valve (zanim.h's CValve, as the other lobby units declare it again) */
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
    void MakeCallbacks(VALVE_STATE state);
    /* the value as an int (the chat unit's: the test's load serves the sum unmasked) */
    int Value() { return m_value; }

    char *m_name;   // 0x0
    unsigned short m_value;   // 0x4
    unsigned short m_flags;   // 0x6
};

/* a network address as the DME builds it (16 bytes) */
struct LANAddress {
    unsigned char m_bytes[16];
};

/* a game another console announced (SOCOM II's LANGame : TransientGame : RoomGame, 0x1D8 bytes:
 * its address and port after the room game; its constructor 0x2E1CE0, the room unit's) */
class LANGame : public RoomGame {
public:
    LANGame(LANAddress *addr, int port);

    LANAddress m_addr;   // 0x1c4
    int m_port;   // 0x1d4
};   // 0x1d8

/* the LAN lobby (the UI lane's name): its room, then the game info it hosts */
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
extern BriefingRoom data_0044FFC0;   /* its room */
extern CValve *data_00450058;   /* the ComeFromLan valve, looked for once */

/* the room unit's statics the game list's filters live in */
extern int data_0044F528;
extern int data_0044F530;
extern int data_0044F538;
extern int data_0044F540;
extern int data_0044F548;
extern int data_0044F550;

/* the mission running (theMission's name, "mpNN", and its rules' record) */
struct LANMissionRules {
    unsigned char _0[0x113];
    unsigned char m_113;   // 0x113
};
extern "C" {
extern char *data_004364FC;
extern LANMissionRules *data_00437CE8;
}

extern "C" {
/* the room's members, called through their placeholders (the room unit's) */
void func_002E2CE0(BriefingRoom *room);   /* (the room's game list updated) */
void func_002E1E10(RoomGame *game, LANGameInfo *info);   /* (a game's info set) */
void func_002E1E00(RoomGame *game, unsigned char seq);
/* the chat unit's */
void func_002F4030(void *msg, int type);   /* a message's byte order converted */
void func_002F7C40(void *rules);   /* the game's rules filled in */
/* the lobby's */
void func_002DF6A0(CZOnlineLobby *lobby);
int func_002EF9F0();   /* (the game's type for the info) */
void func_002F3EF0(void *rules);   /* a type-129 response's tail converted (the chat unit's) */
void func_002F3FF0(void *slots);   /* a free slot looked for (the chat unit's) */
void func_00622AD8(LANAddress *addr);   /* (our own address) */
extern int data_0066A8F0;   /* the game server's connection info (OVL2 bss: its type) */
extern int data_0066A8F4;
extern LANAddress data_0066A8F8;   /* its address */
extern int data_0066A908;   /* its port */
extern int data_0066A924;
/* the DME's and the network's */
void func_00622770(LANAddress *addr, int *ip);   /* (an address from an IP) */
int func_006226C8(int *addr, int client);   /* (a client's address) */
void func_00621EF8(void (*cb)(int, int));   /* (the LAN asked for its games) */
void func_0030CB20(CZNetwork *net, int *addr, int arg);   /* (our game's info sent) */
void func_0030CB90(CZNetwork *net, void (*cb)(LANGameInfo *info));   /* (the game info's callback set) */
}

/* a variable's text when it holds one (the clan and chat units') */
inline char *ZString(_zrdr *z)
{
    return z->type == 3 ? z->string : 0;
}

/* the running mission's number, 0 none */
inline int MissionNumber()
{
    return data_004364FC ? (*data_004364FC ? atoi(data_004364FC + 2) : 0) : 0;
}

/* whether the menus came from the LAN menu */
bool CZLANLobby::IsActive()
{
    bool on = false;

    if (!data_00450058)
        data_00450058 = CValve::GetByName("ComeFromLan");
    if (data_00450058)
        on = data_00450058->m_value != 0;
    return on;
}

/* a game another console announced: kept in the room, new or updated */
void cbLANGameInfo(LANGameInfo *info)
{
    BriefingRoom *room = data_0044FE10.m_room;

    if (room) {
        int ip = info->m_ip;
        LANAddress addr;

        func_00622770(&addr, &ip);
        int port = info->m_port;
        RoomGame *g = room->FindGame(info->m_name);

        if (!g) {
            g = new LANGame(&addr, port);
            room->push_back(g);
        } else {
            memcpy(&((LANGame *)g)->m_addr, &addr, sizeof(addr));
            ((LANGame *)g)->m_port = port;
        }
        g->m_unchecked = 0;
        func_002F4030(info, 0x81);
        RoomGame *game = g;

        if (info) {
            func_002E1E10(game, info);
            func_002F7D90(game->m_19a, info->m_rules);
            if (!game->m_info.m_mission)
                func_002E1E00(game, game->m_missionSeq[0]);
        } else {
            game->m_info.m_ip = -1;
        }
        func_002E2CE0(room);
    }
}

/* the LAN lobby entered: its room in use, the announced games' callback set */
bool CZLANLobby::Connect()
{
    func_002DF6A0(&theLobby);
    func_0030CB90(&theNetwork, cbLANGameInfo);
    m_room = &data_0044FFC0;
    return true;
}

/* the LAN lobby left */
bool CZLANLobby::Disconnect()
{
    theLobby.ReleaseUIResources();
    m_room = 0;
    return true;
}

/* a console asking for our game's info answered */
void cbLANInfoRequest(int client, int arg)
{
    int addr;

    if (!func_006226C8(&addr, client))
        func_0030CB20(&theNetwork, &addr, arg);
}

/* the room's game list asked for again: every game marked unchecked until it answers, the filters
 * cleared, the LAN asked */
bool CZLANLobby::RefreshGameList(C2D *page)
{
    m_room->RemoveEmptyGames();
    m_room->RemoveUncheckedGames();
    func_002E2CE0(m_room);
    for (BriefingRoom::iterator i = m_room->begin(); i != m_room->end(); ++i)
        (*i)->m_unchecked = 1;
    data_0044F528 = 0;
    data_0044F530 = 0;
    data_0044F538 = 65000;
    data_0044F540 = 2;
    data_0044F548 = 2;
    data_0044F550 = 2;
    func_00621EF8(cbLANInfoRequest);
    return true;
}

/* a LAN game hosted: the game server's connection info ours, the game's info made from the
 * menus' valves and variables (the passwords only when their valves say so; the shape the chat
 * unit's CreateNewGameStateHandler has).
 * OPEN, 10 instructions, all in the inlined mission number (GetGameInfo's open): the target
 * branches on the name's first byte with `bnez` to the atoi block and reaches the zero through a
 * `b`; ternaries, if/else chains and early returns tried (try.py, 14 spellings) give `beqz` or a
 * block more */
bool CZLANLobby::CreateGame()
{
    const char *name = "testgame";
    const char *password = "";
    const char *specPassword = password;
    int maxPlayers = 8;
    CValve *passwordValve = CValve::GetByName("PasswordValve");
    CValve *spectators = CValve::GetByName("MaxSpectatorsValve");
    CValve *players = CValve::GetByName("MaxPlayersValve");

    CValve::GetByName("CreateClanAffiliation");
    CUIVariable *var = theLobby.uiv_GameNameVar;

    if (var && var->type == 3)
        name = ZString(var);
    var = theUIVarManager.Add("PlayerGamePassword", 0, UIVL_PERMANENT);
    if (passwordValve && passwordValve->m_value) {
        if (var && var->type == 3)
            password = ZString(var);
    } else {
        *var = "";
    }
    if (players)
        maxPlayers = players->m_value;
    var = theUIVarManager.Add("SpectatorGamePassword", 0, UIVL_PERMANENT);
    if (spectators && spectators->Value()) {
        maxPlayers += spectators->Value();
        if (var && var->type == 3)
            specPassword = ZString(var);
    } else {
        *var = "";
    }
    data_0066A924 = 1;
    data_0066A8F0 = 2;
    data_0066A8F4 = 1;
    LANAddress addr;

    func_00622AD8(&addr);
    memcpy(&data_0066A8F8, &addr, sizeof(addr));
    data_0066A908 = 10075;
    func_002F3FF0(&data_0066A8F0);
    memset(&data_0044FE10.m_info, 0, sizeof(LANGameInfo));
    data_0044FE10.m_info.m_maxPlayers = maxPlayers;
    data_0044FE10.m_info.m_players = 1;
    data_0044FE10.m_info.m_22 = 2;
    data_0044FE10.m_info.m_mission = MissionNumber();
    data_0044FE10.m_info.m_30 = func_002EF9F0();
    int flags = 0;

    if (*password)
        flags |= 1;
    if (*specPassword)
        flags |= 4;
    data_0044FE10.m_info.m_flags = flags;
    if (CValve::Create("mp_locked_game", VALVE_TYPE_1)->m_value)
        data_0044FE10.m_info.m_lock = 3;
    else if (data_00437CE8->m_113 == 1)
        data_0044FE10.m_info.m_lock = 1;
    else
        data_0044FE10.m_info.m_lock = 2;
    data_0044FE10.m_info.m_48 = 2;
    CopyField(data_0044FE10.m_info.m_name, name, sizeof(data_0044FE10.m_info.m_name));
    func_002F7C40(data_0044FE10.m_info.m_rules);
    func_002F3EF0(data_0044FE10.m_info.m_rules);
    return true;
}

/* the game we host as its info goes out: the players and the mission brought up to date.
 * OPEN, 21 instructions (all in the inlined mission number): the target tests the name's first
 * byte with `bnez` to the atoi block placed after the zero arm (and gives the outer null test a
 * zero block of its own); every spelling tried -- the ternary either way round, `== 0`, an if/else
 * chain into a local, early returns, the name in a local -- either puts the atoi block first or
 * adds a block (CreateLANGame's copy of the same inline schedules its zero into the outer test's
 * delay slot) */
void CZLANLobby::GetGameInfo(LANGameInfo *out)
{
    data_0044FE10.m_info.m_players = theNetwork.zNetGetValidGameClientCount();
    data_0044FE10.m_info.m_mission = MissionNumber();
    if (CValve::Create("mp_locked_game", VALVE_TYPE_1)->m_value)
        data_0044FE10.m_info.m_lock = 3;
    else if (data_00437CE8->m_113 == 1)
        data_0044FE10.m_info.m_lock = 1;
    else
        data_0044FE10.m_info.m_lock = 2;
    func_002F7C40(data_0044FE10.m_info.m_rules);
    *out = data_0044FE10.m_info;
}

/* whether a game of the room asks for a password */
bool CZLANLobby::GameHasPassword(const char *game)
{
    RoomGame *g = m_room->FindGame(game);

    if (g && (g->m_info.m_flags & 1))
        return true;
    return false;
}
