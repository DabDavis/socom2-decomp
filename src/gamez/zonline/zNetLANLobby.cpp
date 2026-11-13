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
    /* the value set, the callbacks told when it turns on or off (the lobby unit's, as zanim.h's) */
    bool SetValue(int value)
    {
        VALVE_STATE state = VALVE_SAME;

        if (m_value != value)
            state = value == 0 ? VALVE_OFF : VALVE_ON;
        m_value = value;
        if (state != VALVE_SAME) {
            MakeCallbacks(state);
            return true;
        }
        return false;
    }

    char *m_name;   // 0x0
    unsigned short m_value;   // 0x4
    unsigned short m_flags;   // 0x6
};

/* LANAddress and LANGame are zonline.h's (the room unit builds the game: its constructor 0x2E1CE0) */

/* the LAN lobby is zonline.h's CZLANLobby (data_0044FE10) */
/* its room: the unit's one dynamically initialised static, built empty and its destructor
 * registered by the unit's initialiser 0x4024F0 */
BriefingRoom data_0044FFC0(0);
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
class CMission;
extern CMission theMission;
#define data_004364FC (*(char **)((char *)&::theMission + 0x1C))   /* theMission+0x1C */
#define data_00437CE8 (*(LANMissionRules **)((char *)&::theMission + 0x1808))   /* theMission+0x1808 */
}

extern "C" {
/* the chat unit's */
void func_002F4030(void *msg, int type);   /* a message's byte order converted */
void func_002F7C40(void *rules);   /* the game's rules filled in */
/* the lobby's */
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

/* the running mission's number, 0 none. Read as IR, the three targets (CreateGame's, GetGameInfo's
 * and the chat unit's world report 0x2EF590) are ONE zero block reached from both tests: the
 * inner test `bnez` to atoi, its fall-through a `b` to the zero, the && ternary's own code (round 17's
 * reading). Round 18: it is `p && *p ? atoi(p + 2) : 0` over the pointer IN A LOCAL -- with the global
 * read in place MW materialises the && into a bool and tests that (round 17's "as a ternary the && is
 * materialised"), with the if-forms it folds the bnez/b pair into one beqz; CConsole::FindHandler's
 * `name && h->m_name ? !strcmp(..) : 0` (con_ps2.cpp, 0x29D080) has the same blocks, found by
 * build/scratch/onl/pat_bz.py (matched rows holding an unfolded branch around an empty goto) */
inline int MissionNumber()
{
    const char *p = data_004364FC;

    return p && *p ? atoi(p + 2) : 0;
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
        g->UpdateInfo(info);
        UpdateGameListDisplay(room);
    }
}

/* the LAN lobby entered: its room in use, the announced games' callback set */
bool CZLANLobby::Connect()
{
    theLobby.CreateUIResources();
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
    UpdateGameListDisplay(m_room);
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
 * unit's CreateNewGameStateHandler has; byte-exact since round 18's mission number, above) */
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

void SetDatabase(const char *name);   /* the mission database by name (OVL1 0x27F810, SOCOM 1's) */

extern "C" {
int sprintf(char *s, const char *format, ...);
#define data_0044F434 (*(CUIVariable **)((char *)&::theLobby + 0xC4))   /* theLobby+0xC4 */ /* the game's name as the menus show it */
}

/* a LAN game of the room joined: refused without a mission of ours; the menus' valves set from its
 * rules, a spectator joining a game without spectator room taken as joined; its server's address
 * and port made the connection's, its info ours, the passwords cleared that it does not ask for, its
 * mission sequence taken and the current mission's database set and the game's name shown (with no
 * mission, the sequence is printed and the join refused) -- the online lobby's JoinGame for the LAN
 * (SOCOM II's; OVL1 0x3039B0: the UI's JoinMPGame calls it when the LAN lobby is up; the name ours) */
bool CZLANLobby::JoinGame(const char *name)
{
    RoomGame *game = m_room->FindGame(name);

    if (game) {
        if (!game->HasMissions())
            return false;
        if (game) {
            CValve::Create("MaxPlayersValve", VALVE_TYPE_1)->SetValue(game->m_info.m_maxPlayers);
            CValve::Create("MaxSpectatorsValve", VALVE_TYPE_1)->SetValue(game->m_maxSpectators);
            CValve::Create("mp_friendly_fire", VALVE_TYPE_1)->SetValue(game->IsFriendlyFire());
            CValve::Create("mp_allow_respawn", VALVE_TYPE_1)->SetValue(game->IsRespawnAllowed());
            CValve::Create("EQlimit", VALVE_TYPE_1)->SetValue(game->m_19f[0]);
            CValve::Create("mp_max_rounds", VALVE_TYPE_1)->SetValue(game->m_19a[1]);
            CValve::Create("mp_max_round_time", VALVE_TYPE_1)->SetValue(game->m_19a[2]);
            if (theLobby.JoinAsSpectator() && !game->m_maxSpectators)
                return true;
            data_0066A924 = 1;
            data_0066A8F0 = 2;
            data_0066A8F4 = 1;
            memcpy(&data_0066A8F8, &((LANGame *)game)->m_addr, sizeof(LANAddress));
            data_0066A908 = ((LANGame *)game)->m_port;
            func_002F3FF0(&data_0066A8F0);
            data_0044FE10.m_info = game->m_info;
            if (!(game->m_info.m_flags & 1))
                *theUIVarManager.Add("PlayerGamePassword", 0, UIVL_PERMANENT) = "";
            if (!(game->m_info.m_flags & 4))
                *theUIVarManager.Add("SpectatorGamePassword", 0, UIVL_PERMANENT) = "";
            func_002F3EF0(game->m_19a);
            theLobby.SetMissionSequence(0, game->GetMissionSequence());
            theLobby.ResetMissionSequence();
            unsigned char idx;
            unsigned char seq[32];

            theLobby.GetMissionSequence(&idx, seq);
            if (seq[idx] > 0) {
                char db[15];

                sprintf(db, "mp%d", seq[idx]);
                SetDatabase(db);
            } else {
                char text[256];

                text[0] = 0;
                for (int i = 0; i < 32; i++)
                    sprintf(text, "%s%d", text, seq[i]);
                return false;
            }
            if (data_0044F434 && name)
                *data_0044F434 = name;
        }
    }
    return true;
}

/* the game we host as its info goes out: the players and the mission brought up to date (byte-exact
 * since round 18's mission number: the delay-slot filler puts the zero block's move in the inner `b`
 * here, in the outer test's slot in CreateGame -- one IR) */
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
