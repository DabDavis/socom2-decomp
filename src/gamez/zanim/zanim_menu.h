/* zanim_menu.h -- what the menu's UI commands (zanim_menu.cpp) work on: the 2D anim command record,
 * the UI command table, and the classes the handlers call -- declarations only, each as much as
 * the handlers use. Layouts are SOCOM 1's DWARF where it has the class, moved to SOCOM II's offsets
 * as the handlers read them; methods SOCOM II's map has not named yet are called through their
 * placeholders (the C functions at the end, `this` first). The header's name is ours. */
#ifndef ZANIM_MENU_H
#define ZANIM_MENU_H

#include <string>
#include <queue>

#include "zanim.h"
#include "../ztwod/ztwod.h"
#include "../zinput/zinput.h"
#include "../zui/zui_uivar.h"

/* a string that is never null (SOCOM 1's zutil.h: MSL's basic_string, a null pointer assigned as
 * the empty string -- its operator=, lines 546-551, inlined in the menu's string handlers in both
 * builds. zentity.h and fts_state.h keep stand-ins of it until <string> replaces them) */
class CSaferStr : public std::string {
public:
    void operator=(const char *sz)
    {
        if (sz)
            assign(sz);
        else
            erase();
    }
};

/* A UI command as a menu's ZAnim sequence carries it. SOCOM 1's DWARF: 8 bytes -- the header, the
 * node reference at 4, the command's type at 6 and its argument's name index at 7. SOCOM II's
 * (read off zAnimUIMenuTick and the handlers): the command's own name index at 6, the argument's
 * name index widened to a halfword at 8, and the command's type at 0xA, looked up by name the first
 * time the command runs and kept there */
class C2DAnimCmdHdr {
public:
    _zanim_cmd_hdr header;            // 0x0
    unsigned short node_ref_index;    // 0x4
    unsigned char m_command_index;    // 0x6: the command's name, an index into the anim's names (ours)
    unsigned short m_argument_index;  // 0x8 (SOCOM 1's a byte at 7)
    unsigned char m_command_type;     // 0xa (SOCOM 1's at 6): 0 until GetUICommandType names it
};

/* a UI command whose record a parser builds (the eleven commands with a second function in the
 * table, called with the command's name and its rdr data): the header, then a halfword the parsers
 * zero with the argument's name (SOCOM II's; the names ours) */
struct UIParsedCmd : public C2DAnimCmdHdr {
    /* the argument's name added to the anim's names, if there is one (the parsers'; ours) */
    void SetArgument(const char *name)
    {
        if (name) {
            m_argument_index = ZAnim.m_CurAnim->AddName(name);
            m_c = 0;
        }
    }

    unsigned short m_c;   // 0xc
};

/* GetPlayerInfo's record: the command and what to ask for, read by its parser out of the tags
 * (0..3; SOCOM II's; the names ours) */
struct UIPlayerInfoCmd : public UIParsedCmd {
    int m_what;   // 0x10
};

/* CompareValues's record: two values, each a name (VALUE1/2) or a UI variable's (UIVAR1/2), and
 * how to compare them (SOCOM II's; the names ours) */
struct UICompareValuesCmd : public UIParsedCmd {
    unsigned char m_value1;   // 0x10
    unsigned char m_uivar1;   // 0x11: m_value1 names a UI variable
    unsigned char m_value2;   // 0x12
    unsigned char m_uivar2;   // 0x13
    unsigned char m_case_insensitive;   // 0x14
    unsigned char m_after_formatting;   // 0x15
};

/* GetMUISPersonaInfo's record: the persona by its INDEX (1) or its NAME (2), the argument naming
 * which (SOCOM II's; the names ours) */
struct UIPersonaCmd : public UIParsedCmd {
    unsigned char m_by;   // 0x10
};

/* CharacterProfile's record: what to do with the profile (SOCOM II's; the names ours) */
enum UIPROFILE_OP {
    UIPROFILE_LOAD = 0,
    UIPROFILE_SAVE = 1,
    UIPROFILE_EXISTS = 2,
    UIPROFILE_SAVE_TO_SERVER = 3,
    UIPROFILE_WAIT_FOR_SERVER_SAVE = 4,
    UIPROFILE_LOAD_FROM_SERVER = 5,
    UIPROFILE_WAIT_FOR_LOAD = 6,
    UIPROFILE_DELETE = 7,
    UIPROFILE_NONE = 0xFFFFFF
};

struct UIProfileCmd : public UIParsedCmd {
    UIPROFILE_OP m_op;   // 0x10
};

/* GetMissionInformation's record: the mission set and the names its fields go to (SOCOM II's; the
 * names ours) */
struct UIMissionInfoCmd : public UIParsedCmd {
    unsigned char m_multiplayer;   // 0x10
    int m_name;   // 0x14
    int m_number;   // 0x18
    int m_index;   // 0x1c
};

/* GetLadderInfo's record: which list and which move (1..5, 7..11 for the clan ladder; SOCOM II's;
 * the name ours) */
struct UILadderCmd : public UIParsedCmd {
    int m_op;   // 0x10
};

/* PlayMPEG's record: the film (by name or by index), how it plays, the audio stream as a number or
 * a valve's or UI variable's name, and the rectangles (SOCOM II's; the names ours) */
struct UIPlayMPEGCmd : public UIParsedCmd {
    bool m_fade;   // 0x10
    bool m_ingame;   // 0x11
    bool m_interlace;   // 0x12
    int m_stream_type;   // 0x14: 0 a number, 1 a valve, 2 a UI variable
    int m_stream;   // 0x18
    unsigned char m_has_dest;   // 0x1c
    bool m_bilinear;   // 0x1d
    unsigned char _1e[2];
    int m_source[4];   // 0x20
    int m_dest[4];   // 0x30
    unsigned char m_by_index;   // 0x40
};

/* SendChatText's and SendMediusChat's record: the UI variable the recipient is in (SOCOM II's; the
 * names ours) */
struct UIChatCmd : public UIParsedCmd {
    char m_recip_uivar[32];   // 0x10
};

/* CopyUIVar's record: the variables' names, -1 for none (SOCOM II's; the names ours) */
struct UICopyUIVarCmd : public UIParsedCmd {
    int m_from;   // 0x10
    int m_to;   // 0x14
};

/* the personas listed into PLAYERPERSONALIST (zanim_menu.cpp's) */
void UpdatePersonaList(const char *host, int port);

/* a command's type by its name (zanim_menu.cpp's) */
int GetUICommandType(const char *name);

/* the reader's lookups the parsers use (zrdr.h's: SOCOM 1's zrdr_findstring; a tag's bool, its
 * rectangle, a string in an array -- the rectangle's byte result as the parsers store it) */
struct tag_RECT;
char *zrdr_findstring(const _zrdr *rdr, const char *tag);
bool zrdr_findbool(const _zrdr *rdr, const char *tag, bool *out);
unsigned char zrdr_findRECT(const _zrdr *rdr, const char *tag, tag_RECT *out);
bool zrdr_findSTRING(const _zrdr *rdr, const char *tag, const char **out);

/* a UI command's handler: whether the command is done */
typedef bool (*UICommandFunc)(C2DAnimCmdHdr *hdr, float *dT);

/* the UI commands by name (SOCOM 1's UICommandTable, 12-byte records; SOCOM II's 16: a second
 * function after the handler, set for eleven commands). Ends at a record whose id is 0. The
 * record's names are ours */
struct UICommand {
    int id;                 // 0x0
    const char *name;       // 0x4
    UICommandFunc func;     // 0x8
    void *func2;            // 0xc: SOCOM II's
};
extern UICommand UICommandTable[];

/* the anim the interpreter is running: the handlers' arguments come out of it */
inline CZAnim *CurAnim() { return ZAnim.m_CurAnim; }

/* a command's argument, a name in the anim's table (the handlers' common first line; ours) */
inline const char *ArgName(C2DAnimCmdHdr *hdr)
{
    return ZAnim.m_CurAnim->GetName(hdr->m_argument_index);
}

/* the argument's text: what the user typed when the name says INPUT_STRING, else the name itself
 * (SOCOM II's handlers branch on it where SOCOM 1's materialised the test; ours) */
inline const char *ArgText(const char *name)
{
    return name && !strcmp(name, "INPUT_STRING") ? ZAnim.m_CurAnim->m_arg_list->GetCharP(0) : name;
}

/* a valve set, its callbacks told whether it went on or off (CValve::SetValue, inlined in both
 * builds; the name ours, as fts_mission.cpp spells it) */
inline void SetValveValue(CValve *valve, int value)
{
    VALVE_STATE state = VALVE_SAME;

    if (valve->m_value != value)
        state = value == 0 ? VALVE_OFF : VALVE_ON;
    valve->m_value = value;
    if (state != VALVE_SAME)
        valve->MakeCallbacks(state);
}

/* ---- the classes the handlers call ---- */

/* a 2D object's visual: C2DVisual (SOCOM 1's, a zdb::CVisual made from a C2D), its C2D at 0x68 */
class C2DVisual {
public:
    unsigned char _0[0x68];
    C2D *m_c2d;   // 0x68
};


/* a UI variable (SOCOM 1's DWARF: CUIVariable : _zrdr, 0x1C bytes) */
enum UIVAR_LONGEVITY {
    UIVAR_TEMPORARY = 0,   /* (the values' names ours: the handlers pass 1 and 2) */
    UIVAR_SESSION = 1,
    UIVAR_PERMANENT = 2
};

class CUIVariable : public _zrdr {
public:
    /* a number set, unless it already holds it (ours: ClearUIVar's and the options' inline) */
    void SetInt(int value)
    {
        if (type != 1 || integer != value) {
            DeleteChildren();
            type = 1;
            integer = value;
            UpdateDependents();
        }
    }
    void SetReal(float value)
    {
        if (type != 2 || real != value) {
            DeleteChildren();
            type = 2;
            real = value;
            UpdateDependents();
        }
    }
    /* a string set (zui.h's, 0x3862A0: the lobby callbacks' and the patch downloader's setter) */
    CUIVariable &operator=(const char *str);
    /* (zui.h's, 0x3863D0: the variable's change told to what it is attached to) */
    void UpdateDependents();
    /* a value given to it (zui.h's, 0x386460) */
    void Set(_zrdr *value);

    char *m_varname;   // 0x8
    unsigned char m_attached_items[0xc];   // 0xc: SOCOM 1's vector
    UIVAR_LONGEVITY m_longevity;   // 0x18
};

/* the UI variables (SOCOM 1's DWARF: CUIVarManager : vector, CSaveModule, 0x8064 bytes) */
class CUIVarManager {
public:
    /* a variable made, or found by its name (zui.h's, 0x3857C0) */
    CUIVariable *Add(const char *name, _zrdr *value, UIVAR_LONGEVITY longevity);
    /* a variable found by its name (zui.h's, 0x385900) */
    CUIVariable *Find(const char *name);

    unsigned char _0[0x8064];
};
extern CUIVarManager theUIVarManager;

/* the lobby's states (SOCOM 1's DWARF; a byte: zonline.h's, which this unit does not include) */
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

/* the online lobby (SOCOM 1's DWARF: 0x98 bytes; the members the handlers read, at SOCOM II's
 * offsets; the members the handlers call, by their map rows -- the online lane's names) */
class CZOnlineLobby {
public:
    int RemovePlayerFromClan(const char *name);
    void SetMissionSequence(unsigned char idx, unsigned char *seq);   /* 0x2F8270 */
    void ResetMissionSequence();                                      /* 0x2F8230 */
    void GetLadderInfo();                        /* 0x2EB2C0 (the online lane's names) */
    void GetLadderRanking();                     /* 0x2EAFB0 */
    void GetLadderBest(int n);                   /* 0x2EB610 */
    void GetLadderList(int start, int n);        /* 0x2EB510 */
    int GetLadderStart();                        /* 0x2EB790 */
    int GetClanLadderStart();                    /* 0x2EB780 */
    int GetLadderTotal();                        /* 0x2EB770 */
    int GetClanLadderTotal();                    /* 0x2EB760 */
    void GetClanLadderList(int start, int n);    /* 0x2EA5C0 */
    void GetClanLadderLeaders(int n);            /* 0x2EA6B0 */
    bool Open(const char *host, int port);   /* 0x2E0030 (the online lane's round 12) */
    bool OnCorrectClanTeam();
    bool GetClan();
    static void PasswordSecurity();
    void CreateUIResources();
    void ReleaseUIResources();
    void Close();
    void Tick();
    void DMEInit();
    bool RefreshGameList(C2D *list);
    bool CanJoinRoom(const char *room);
    void GetCurrentGameInfo();              /* 0x2E24D0 (the online lane's round 14) */
    void GetGameInfo(const char *name);     /* 0x2E1F20 (the online lane's round 14) */
    void RejoinRoom();
    bool JoinRoom(const char *room);
    bool RefreshRoomList(C2D *list);
    void DeleteClanMessage(int index);
    void ModifyClanMessage(int index, const char *text);
    void GetLeaderMessages();
    bool IsLeaderOfClan(const char *name);
    bool IsMemberOfClan();
    void DisbandClan();
    void GetClanRoster();
    void GetInvitedPlayers();
    bool ClanHasInvitations();
    bool RevokeInvite(const char *name);
    bool InvitePlayerToClan(const char *name);
    void CreateClan(const char *name);
    void UpdateClanStats();
    void TransferLeadership(const char *name);
    bool SendClanMessage(const char *text);
    void GetClanMessages();
    void CheckClanInvitations();
    bool HasClanInvitations();
    void DeclineClanInvitation(int index);
    void AcceptClanInvitation(int index);
    void QuitClan();
    void GetClanRanking(int arg);
    void ChangeState(LOBBY_STATE state);
    void Bailout();
    void SendChat(const char *text, const char *to);
    void Censor(const char *text, void (*done)(const char *, void *), void *data);
    bool RefreshGamePlayerList(C2D *list);
    void GetAnnouncements();
    void GetUsagePolicy();
    void SendEndGameReport();
    int AdvanceMissionSequence();
    void ForceStatusReport();
    bool JoinGame(const char *game);
    bool GameIsPasswordProtected(const char *game);
    bool CreateGame();
    void BanSelfFromGame();
    bool Register(const char *name, const char *password);
    void Logout();
    bool Login(const char *name, const char *password);
    void SetPlayerDescription(const char *desc);
    void ConfirmFriend(int index);
    void RejectFriend(int index);
    void AddFriendDirectly(int index);
    void AddToIgnore(const char *name);
    void GetIgnoreList();
    void RemoveFromIgnore(int index);
    void GetFriendInfo(int index);
    void GetFriendInvitations();
    void AddFriend(const char *name);
    void RemoveFriend(int index);
    void GetFriendsList();
    bool CanInviteMorePlayersToClan();                  /* 0x2E5FE0 (the online lane's round 27) */
    void GetClanMemberInfo(const char *name);           /* 0x2E6000 (the online lane's round 27) */
    bool GameRejectsRank(const char *name);             /* 0x2F8BC0 (the online lane's round 27) */
    bool ClanGameClosed(const char *name);              /* 0x2F8C60 (the online lane's round 27) */
    bool IsSwitchTeamsOK();                             /* 0x2F8CE0 (the online lane's round 27) */
    bool GameHasSpectatorPassword(const char *name);    /* 0x2F8D50 (the online lane's round 27) */

    unsigned char _0[0x58];
    bool (*m_event_handler)(const char *eventname, const char *argument);   // 0x58 (SOCOM 1's): InitNetwork sets it
    unsigned char _5c[0x60 - 0x5c];
    unsigned char m_current_state;   // 0x60: SOCOM 1's LOBBY_STATE
    unsigned char _61[0x98 - 0x61];
};
extern CZOnlineLobby theLobby;
/* our clan game's side (zNetMediusClan.cpp's 0x2E99E0, which reads it signed; read unsigned here, as
 * zNetMediusLobby.cpp does: the online lane's round 27) */
unsigned char GetClanTeamSide();

/* the network (SOCOM 1's CZNetwork, less the two team indices SOCOM II dropped ahead of the flags) */
class CZNetwork {
public:
    int zNetGetValidGameClientCount();
    unsigned int zNetGetTime();
    unsigned int zNetGetLocalTime();   /* 0x30D540 (SOCOM 1's) */
    bool zNetInitialize();   /* 0x30DBB0 */
    bool zNetLoadIRX();      /* 0x30DEE0 (SOCOM 1's; the online lane's round 14) */
    void zNetClose();        /* 0x30D770 (SOCOM 1's; named by the online lane's round 25) */
    void zNetInit();         /* 0x30E290 (SOCOM 1's; named by the online lane's round 25) */

    bool m_bSessionMaster;   // 0x0 (SOCOM 1's at 8)
    bool m_bNetwork;   // 0x1
    unsigned char _2[0x108 - 0x2];
    bool m_bMediusEnabled;   // 0x108 (SOCOM 1's at 0xE4)
};
extern CZNetwork theNetwork;

/* the online game (SOCOM 1's CZNetGame: only what the handlers read) */
class CZNetGame {
public:
    /* SOCOM 1's DWARF: the states as bytes */
    enum MP_MAJOR_GAME_STATE { MP_MAJOR_UNKNOWN = 0, MP_MAJOR_MENUS = 1, MP_MAJOR_LOADING = 2 };
    enum MP_MINOR_GAME_STATE { MP_MINOR_UNKNOWN = 0, MP_MINOR_NOTREADY = 1, MP_MINOR_READY = 2 };

    void TenSecondLaunch(bool on);
    void MyMajorGameState(MP_MAJOR_GAME_STATE state);
    void MyMinorGameState(MP_MINOR_GAME_STATE state);
    void MPMajorGameState(MP_MAJOR_GAME_STATE state);   /* 0x2A7490 (SOCOM 1's) */

    unsigned char _0[0x10];
    CValve *m_10;   // 0x10: a valve AdvanceMPMission tests (a longer wait when on)
    unsigned char _14[0x28 - 0x14];
    CValve *m_pJoinGameValve;   // 0x28 (SOCOM 1's at 0x24)
    unsigned char _2c[0x30 - 0x2c];
    CValve *m_p45SecondClockValve;   // 0x30 (SOCOM 1's at 0x2C): the seconds to the launch
    CValve *m_pXSecondClockValve;   // 0x34 (SOCOM 1's at 0x30): the seconds of the ten-second launch
    unsigned char _38[0xac - 0x38];
    int m_persona_object_index;   // 0xac (SOCOM 1's at 0x70)
    unsigned char _b0[0xc8 - 0xb0];
    unsigned int m_c8;   // 0xc8: when the next mission launches (AdvanceMPMission)
    unsigned char m_late_joiner;   // 0xcc (SOCOM 1's at 0x94)
    unsigned char _cd[0xd4 - 0xcd];
    unsigned char m_game_over;   // 0xd4 (SOCOM 1's at 0x9B, four before the launch flag in both)
    unsigned char _d5[0xd8 - 0xd5];
    unsigned char m_ten_second_launch;   // 0xd8 (SOCOM 1's at 0x9F)
    unsigned char m_joined;   // 0xd9: CheckJoinMP sets it (SOCOM II's; the name ours)
    unsigned char _da[0xe4 - 0xda];
    unsigned int m_launch_time;   // 0xe4 (SOCOM 1's clock's at 0xA8)
    unsigned char _e8[0x12c - 0xe8];
    char m_chartype[31];   // 0x12c: the character type's name (ChangeCharacterType)
};

class CZPersonaState {
public:
    static void ToggleReady();
    static void UpdateLocalPersona();   /* 0x2C5360 */
    /* a client's persona's name, none without one (0x2C3A70, SOCOM 1's), and its clan tag, "" without
     * one (0x2C3AC0; fts_net_persona.cpp's) */
    static const char *GetPersonaName(int client);
    static const char *GetPersonaClanTag(int client);
};



/* the mission (only what the handlers read) */
class CRdrFile;

/* how the mission stands (SOCOM 1's DWARF: a byte enum) */
enum MISSION_STATE {
    MISSION_UNLOADED = 0,
    MISSION_RUNNING = 1,
    MISSION_SUCCESS = 2,
    MISSION_FAILURE = 3,
    MISSION_ABORTED = 4,
    MISSION_TIMEDOUT = 5
};

class CMission {
public:
    void CreateNetGame();
    void DestroyNetGame();   /* 0x2A9EF0 */
    void PreOpen(const char *database);   /* 0x2ACC10 (fts_mission.cpp's: SOCOM 1's) */

    unsigned char _0[0x18];
    MISSION_STATE m_state;   // 0x18 (SOCOM 1's)
    unsigned char _19[0x1c - 0x19];
    char *m_database;   // 0x1c (SOCOM 1's)
    unsigned char _20[0x1808 - 0x20];
    CZNetGame *m_pNetGame;   // 0x1808 (SOCOM 1's at 0x584)
    unsigned char _180c[0x1824 - 0x180c];
    CRdrFile *m_fontsRdr;   // 0x1824 (SOCOM 1's at 0x5A0)
};
extern CMission theMission;

/* the clock as libcdvd reads it (BCD bytes) */
struct sceCdCLOCK {
    unsigned char stat;
    unsigned char second;
    unsigned char minute;
    unsigned char hour;
    unsigned char pad;
    unsigned char day;
    unsigned char month;
    unsigned char year;
};
extern "C" int sceCdReadClock(sceCdCLOCK *clock);
extern "C" void sceScfGetLocalTimefromRTC(sceCdCLOCK *clock);

/* a memory card file's information as the saved-game handlers read it (their 272-byte local; the
 * text from 12, "This game is a dummy game." in SaveConfig's dummies; ours) */
struct MemCardFileInfo {
    unsigned char _0[12];
    char m_text[260];   // 0xc
};

/* the memory card: zutil.h's CMemCard (zanim.h includes it through zar.h) */
extern CMemCard theMemCard;

/* the game and its load state (only what the handlers call) */
class CLoadState {
public:
    void AbortLoad(const char *name);

    unsigned char _0[0x24];
};

/* the front end's state (fts_state.h has the whole of it): its menu and the menu it opens on */
class CMenuState {
public:
    bool RunEventAnim(const char *eventname, const char *argument);
    /* (SOCOM II's out-of-line copy at 0x1E7B50; the name ours, as fts_state.h spells it) */
    void SetDefaultMenu(const char *name) { strncpy(m_DefaultMenu, name, 64); }

    unsigned char _0[0x20];
    CSaferStr BackSoundString;   // 0x20 (fts_state.h's)
    unsigned char _2c[0x30 - 0x2c];
    char m_DefaultMenu[64];   // 0x30
    unsigned char _70[0x74 - 0x70];
};

class CGameState;

/* the game's state stack (game_main.cpp's CGame: only what the handlers call) */
class CGame {
public:
    void Pop(unsigned int arg);                        /* 0x2CF2F0 */
    void Push(CGameState *state, unsigned int arg);    /* 0x2CF380 */
    void Switch(CGameState *state, unsigned int arg);  /* 0x2CF410 */
};

/* the cinematic sub-state (fts_state.h's CCinematicState: only its film set) */
class CCinematicState {
public:
    void SetMovie(const char *movie, unsigned char a, unsigned char b);   /* 0x1EDA60 */

    unsigned char _0[0x29];
    unsigned char m_29;   // 0x29: interlaced (PlayMPEG's settings, fts_state.h's names)
    unsigned char m_2a;   // 0x2a: in game
    unsigned char _2b;
    int m_2c;   // 0x2c: the audio stream
    unsigned char _30[0x3c - 0x30];
    unsigned char m_3c;   // 0x3c: the rectangles given
    unsigned char m_3d;   // 0x3d: bilinear
    unsigned char _3e[0x40 - 0x3e];
    int m_40[4];   // 0x40: the source rectangle
    int m_50[4];   // 0x50: the destination rectangle
};

class COurGame : public CGame {
public:
    /* the state on top of the stack (SOCOM 1's CGame; the name ours) */
    CGameState *CurrentState()
    {
        if (m_CurrentState >= 16)
            return 0;
        return m_Stack[m_CurrentState];
    }

    unsigned char _0[0xc];
    unsigned int m_CurrentState;   // 0xc
    CGameState *m_TestVariable;   // 0x10
    CGameState *m_Stack[16];   // 0x14
    unsigned char _54[0x78 - 0x54];
    CMenuState m_menuState;   // 0x78 (SOCOM 1's at 0x7C)
    CLoadState m_loadState;   // 0xec (SOCOM 1's at 0xE0)
    unsigned char m_coreState[0xd0];   // 0x110: a CCoreState (fts_state.h)
    unsigned char _1e0[0x234 - 0x1e0];
    unsigned char m_cinematicSubState[0x64];   // 0x234: a CCinematicState (fts_state.h)
};
extern COurGame theGame;

zdb::CNode *GetMenuNode();

/* ---- the pop-up's classes, as GetPopUp builds it and its destructor tears it down (zui_2d.h's,
 * which this header cannot include: its zui.h stand-ins clash with zanim.h's). Each keeps one
 * non-inline virtual, so this unit emits none of their tables (zui_2d.cpp does: the pop-up's at
 * 0x4073A0 sits among its tables, with the destructor's thunk 0x37D110) ---- */

/* a plain bitmap (SOCOM 1's CPlainBmp; zui_2d.h's): its constructor called out of line -- the copy
 * an earlier unit links, 0x20CD50 -- and its destructor inline, ~UIVarMixin and the bitmap's with it */
class CPlainBmp : public C2DBitmap, public UIVarMixin {
public:
    CPlainBmp();
    virtual ~CPlainBmp() {}
    virtual void UpdateUIVar(CUIVariable *var);

    bool m_pulsing;     // 0x88
    float m_period;     // 0x8c
    float m_curtrans;   // 0x90
} __attribute__((aligned(16)));   // 0xa0 bytes

/* text wrapped to a width (SOCOM 1's CWrappedText; zui_2d.h's: a renderable string, 0x250 bytes, its
 * inline constructor's out-of-line copy 0x27F250 -- called here, by name) */
class CWrappedText : public C2DString {
public:
    CWrappedText();
    virtual ~CWrappedText();

#if Z_PORT
    /* the host's size: zui_2d.cpp's class is 0x230 bytes under the host's ABI (tools/odrcheck.py
     * in the port), where this restatement's MW padding made 0x250 -- and moved every member after
     * a CWrappedText (CPopUpDialog's) */
    unsigned char _98[0x230 - sizeof(C2DString)];
#else
    unsigned char _98[0x250 - 0x98];
#endif
} __attribute__((aligned(16)));   // 0x250 bytes

/* a button (zui_2d.h's C2DButton, 0x440 bytes: its constructor 0x377800 and destructor 0x377510
 * out of line) */
class C2DButton {
public:
    C2DButton();
    ~C2DButton();

#if Z_PORT
    unsigned char _0[0x430];   /* the host's size of zui_2d.cpp's class (MW's 0x440) */
#else
    unsigned char _0[0x440];
#endif
} __attribute__((aligned(16)));

/* the menu's pop-up (SOCOM 1's CPopUpDialog: a plain bitmap with its text, a 2D object, two buttons;
 * SOCOM II's 0xBF0 bytes, made by GetPopUp. Its members by its constructor, inlined there, and its
 * destructor, whose out-of-line copy this unit emits (0x27C8D0, after UIFlushMenuHistory: the first
 * function to delete one, through FlushHistory); the rest of the class is zui_2d.cpp's -- the
 * members' names follow its model, the 2D object at 0x314 and the word at 0xBE4 ours) */
class CPopUpDialog : public CPlainBmp {
public:
    CPopUpDialog() : m_32c(0), m_330(0), m_be4(0) { m_text.SetString("", true); }
    virtual ~CPopUpDialog()
    {
        if (m_32c) {
            delete m_32c;
            m_32c = 0;
        }
    }
    virtual void Draw(const CMatrix &m, zdb::CTextureRelocMgr *mgr);
    virtual void Tick(float dT);
    virtual bool HandlePad(const CPad *pad, int port);
    virtual int GetZAnimCmdType(const char *name);

    unsigned char _a0[0xb0 - 0xa0];
    CWrappedText m_text;   // 0xb0
    unsigned char _300[0x314 - 0x300];
    C2D m_314;   // 0x314
    C2D *m_32c;   // 0x32c: drawn instead of it all when set
    CValve *m_330;   // 0x330: the valve the answer is set in
    unsigned char _334[0x340 - 0x334];
    C2DButton m_340;   // 0x340: the first button
    unsigned char _780[0x790 - 0x780];
    C2DButton m_790;   // 0x790: the second
    unsigned char _bd0[0xbe4 - 0xbd0];
    int m_be4;   // 0xbe4
} __attribute__((aligned(16)));   // 0xbf0 bytes

/* the default name of a stack entry and of a history slot (SOCOM 1's DEFAULT_ARGNAME and
 * MENUHISTORY_STRPOOL: "NO_ARG" and "POOL") */
extern const char *DEFAULT_ARGNAME;
extern const char *MENUHISTORY_STRPOOL;

/* a screen on the stack: its id and its argument's name, 52 bytes (ours: the stack's constructor
 * and FlushHistory write them so) */
struct CMenuEntry {
    CMenuEntry()
    {
        m_id = -1;
        strcpy(m_argname, DEFAULT_ARGNAME);
    }

    int m_id;   // 0x0
    char m_argname[48];   // 0x4
};

/* a pointer vector's storage, as MSL keeps it (include/vector) */
typedef std::__vector_pod<unsigned int, std::allocator<unsigned int> > PtrVec;

/* the screens gone back over: names in slots of a pool, a free list of the unused slots (SOCOM 1's
 * CMenuHistory, 0x618 bytes in both builds; its members' names ours) */
class CMenuHistory {
public:
    CMenuHistory();
    /* every name given back to the pool (ours: FlushHistory and the destructor inline it) */
    void Flush();

    std::vector<char *> m_used;   // 0x0
    char m_pool[32][48];   // 0xc
    std::vector<char *> m_free;   // 0x60c
};

/* the menu's screen stack (SOCOM 1's CMenuStack, 0xA34 bytes there, 0xE44 here: twice the
 * entries) */
class CMenuStack {
public:
    CMenuStack();
    ~CMenuStack();
    void KillPopUp();
    void CleanPopup();
    void AddToHistory(const char *name);
    CPopUpDialog *GetPopUp();
    void PopUpDialog(const char *name);
    void FlushHistory(bool all);
    void HandleCommand(const char *name);
    /* an entry put on top: a command for the screen and its argument (the button handlers inline it,
     * both builds; the name ours) */
    void Push(int id, const char *argname)
    {
        m_top++;
        m_entries[m_top].m_id = id;
        strcpy(m_entries[m_top].m_argname, argname);
        m_count++;
    }
    /* whether there is a screen to go back to (ours: SwitchMenu's `&&` materialises it) */
    bool CanGoBack() { return !m_history.m_used.empty(); }
    /* the screen last gone back over, " " with none (SwitchMenu's; ours) */
    const char *Previous()
    {
        return m_history.m_used.empty() ? " " : (const char *)*(m_history.m_used.PtrVec::end() - 1);
    }
    /* back to the screen last gone over: an entry for it put on top, its history slot given back to
     * the pool (SOCOM 1's SwitchMenu wrote it out; SOCOM II's inlines it twice there -- ours) */
    void Back()
    {
        if (!m_history.m_used.empty()) {
            m_top++;
            m_entries[m_top].m_id = 5;
            strcpy(m_entries[m_top].m_argname, Previous());
            if (!m_history.m_used.empty()) {
                char *slot = (char *)*(m_history.m_used.PtrVec::end() - 1);

                m_history.m_used.pop_back();
                m_history.m_free.PtrVec::insert(m_history.m_free.PtrVec::end(), 1, (unsigned int &)slot);
                strcpy(slot, MENUHISTORY_STRPOOL);
            }
            m_count++;
        }
    }

    CMenuEntry m_entries[40];   // 0x0 (SOCOM 1's 20)
    int m_count;   // 0x820: entries pushed (the names ours)
    int m_top;   // 0x824: the top entry's index
    CMenuHistory m_history;   // 0x828 (SOCOM 1's at 0x418)
    CPopUpDialog *m_popup;   // 0xe40 (SOCOM 1's at 0xA30)
};
CMenuStack *GetMenuStack();
extern CMenuStack g_MenuStack;   /* 0x414ED0 (SOCOM 1's) */

/* the video state (SOCOM 1's _zvid_public; only what the handlers read, at SOCOM II's offsets) */
struct ZVidMenuView {
    unsigned char _0[0x74];
    unsigned char videoMode;   // 0x74 (SOCOM 1's at 0x50): 2 is PAL
    unsigned char vidMode;   // 0x75 (SOCOM 1's at 0x51): 0 NTSC, 2 DTV
    unsigned char _76[0x88 - 0x76];
    int pcrtcOffsetX;   // 0x88: the screen's origin (SOCOM 1's pcrtcOffset at 0x5C)
    int pcrtcOffsetY;   // 0x8c
};
extern ZVidMenuView zVid;

/* the player (only the flag PlayMPEGNoFade tests: bit 4 of the byte at 0xE1) */
class CZSealBody {
public:
    unsigned char _0[0xe1];
    unsigned char _e1_0 : 4;
    unsigned char m_e1_4 : 1;   // 0xe1 bit 4
};
CZSealBody *ftsGetPlayer();

/* the network calls the handlers make */
void NetSendRequestTeamSwitch(int persona);
void SendChatMsg(const char *text);

/* text the user types, sent to a callback with whom it is for (SOCOM II's; 0x281D40) */
typedef void (*UITextCallback)(const char *text, const char *to);

/* a character type (SOCOM 1's CCharacterType, 0x2FC bytes; SOCOM II's weapons list at 0x2E8) */
class CCharacterWeapList;

class CCharacterType {
public:
    static CCharacterType *GetByNameMod(const char *name);   /* 0x53E170 (SOCOM 1's) */
    unsigned char _0[0x2e8];
    unsigned char m_weapons[0xc];   // 0x2e8 (SOCOM 1's CCharacterWeapList at 0x250)
    unsigned char _2f4[0x388 - 0x2f4];
    unsigned char m_stats[4];   // 0x388: what ChangeCharacterType reads its figures from
};

const char *GetCustomCharTypeName(int index);

/* ---- the network's start-up: a simple state machine ---- */

/* a state machine whose states are member functions, each told what happens to it: 0 entered, 1
 * ticked, 2 left (SOCOM II's CSimpleStateMachine, by its RTTI at 0x3EF738, its unit at 0x3A5xxx; the
 * members' names ours). It runs on a thread when it has one (at 4); a state change goes through a
 * queue (at 0x14: a std::queue over a deque of the 12-byte member pointers, 42 to a block) whose
 * push_back this unit emits in its template tail, the copy linked (0x2852B0: include/deque's
 * __deque_imp::push_back, byte-exact as it stands -- the UI lane's round 34) */
class CSimpleStateMachine;
typedef void (CSimpleStateMachine::*SSMState)(unsigned char event);

/* the state queue (util_ssm.cpp's: a class of its own around std::queue) */
class CStateQueue : public std::queue<SSMState> {
};

class CSimpleStateMachine {
public:
    CSimpleStateMachine();    /* 0x3A5DD0 */
    ~CSimpleStateMachine();   /* 0x3A5CC0 */
    /* the machine put back in its idle state, not done (the base's copy, 0x3A5AE0) */
    virtual bool Start()
    {
        SetState(&CSimpleStateMachine::Idle);
        m_done = 0;
        return false;
    }
    /* a state change queued: every machine's vtable holds the copy linked from this unit (0x276F40),
     * the first in the link to emit one */
    virtual void SetState(const SSMState &state)
    {
        SSMState s = state;   /* (copied first: passed straight, the reference is handed on) */

        m_queue.push(s);
    }

    void Idle(unsigned char event);     /* 0x3A5CB0: nothing */
    void Finish(unsigned char event);   /* 0x3A5C70: done once entered or ticked */
    /* (util_ssm.cpp's: whether it is done -- no thread and the flag, 0x3A5C10; its tick -- a
     * threaded machine's thread given the processor, else a step, 0x3A5C30) */
    bool IsDone();
    void Tick();

    unsigned int m_thread;   // 0x4
    SSMState m_state;   // 0x8
    CStateQueue m_queue;   // 0x14
    unsigned char m_done;   // 0x2c
    int m_30;   // 0x30
    int m_thread_id;   // 0x34
};

/* the network started as a machine: initialised, then waited for, the command's valve told how it
 * went (SOCOM II's CSMInitNetwork, its vtable 0x4060B0; the members' names ours) */
class CSMInitNetwork : public CSimpleStateMachine {
public:
    CSMInitNetwork(CValve *valve) : m_valve(valve) {}
    virtual bool Start();
    void Initialize(unsigned char event);
    void WaitForNetwork(unsigned char event);

    CValve *m_valve;   // 0x38
};

/* ---- the functions the handlers call, by their map rows (the owners' units define them) ---- */

/* the controller configurations (fts_controller.cpp's CCtrlrConfigs: only the selection) */
class CCtrlrConfigs {
public:
    static void SelectConfig(const char *name);   /* 0x2C5D60 (SOCOM 1's) */
};

/* the reader's archives (zrdr.h's CRdrArchive: only the removal) */
class CRdrArchive {
public:
    static void RemoveArchive(const char *archive, const char *name);   /* 0x32C2B0 (SOCOM 1's) */
};

/* the languages (util_loc.cpp's, the names its own) */
enum LANGUAGE {
    LANG_STATES,
    LANG_UK,
    LANG_FRANCE,
    LANG_GERMANY,
    LANG_SPAIN,
    LANG_ITALY,
    LANG_JAPAN,
    LANG_KOREA,
    LANG_TEST,
    LANG_COUNT,
    LANG_UNSET = 99
};
LANGUAGE LanguageFromName(const char *name);   /* 0x3A40E0 */
/* the game's partition on the hard disk (util_sonyio.cpp's): mounted (0 or an error), its files
 * found, and unmounted (0x39D020) */
int HddMountGame();                                                                  /* 0x39D050 */
int HddFindFiles(const char *pattern, void *out, int size, unsigned int index);     /* 0x39CE80 */
int HddUnmountGame();                                                                /* 0x39D020 */
void SetLanguage(LANGUAGE lang);               /* 0x3A44B0 */

extern "C" int ezNetCnfGetNetcnfifData(const char *device, const char *name, void *out);   /* 0x1E8910 */
struct NetCnfList;
extern "C" int ezNetCnfGetCombinationList(char *fname, int type, NetCnfList *list);   /* 0x1E8740 */
void SetFinalStatsUiVars();          /* 0x2241C0 (st_mpexit.cpp's) */
bool DMEJoinGameServer();            /* 0x2B83E0 (SOCOM 1's, by UIJoinGameServer) */
bool MediusInit();                   /* 0x300840 */
void zSysPostInit();                 /* 0x34F950 (SOCOM 1's) */
void UiSetTtySpeedFactor(float f);       /* 0x37B980 (SOCOM 1's) */
void UiSetCounterSpeedFactor(float f);   /* 0x37B990 */
void SetPlayAutoMessages(bool on);   /* 0x5E7FB0 (seal_unit.cpp's) */
bool GetPlayAutoMessages();          /* 0x5E7FC0 */

/* a reader value built node by node (zrdr.h's CRdrEditor, 80 bytes on the handlers' stacks: a
 * _zrdr first, which the UI variables take; its constructor 0x333AE0 and destructor 0x333A20) */
class CRdrEditor : public _zrdr {
public:
    CRdrEditor();
    ~CRdrEditor();
    void addstringnode(const char *text);   /* 0x333610 */
    void addintnode(int value);             /* 0x333700 */
    void pop();                             /* 0x333770 */
    void addrdrnode(_zrdr *node);           /* 0x333540 */
    void push(const char *name);            /* 0x3339B0 */

    unsigned char _8[80 - 8];
};

/* a side's equipment saved for a mission (zNetMediusUniverse.cpp's AcctLoadout, 14 bytes: the
 * character's skin, the five selections' values, whether it is set) */
struct AcctLoadout {
    AcctLoadout() { m_valid = 0; }

    unsigned char m_skin;   // 0x0
    unsigned short m_primary;   // 0x2
    unsigned short m_secondary;   // 0x4
    unsigned short m_equip1;   // 0x6
    unsigned short m_equip2;   // 0x8
    unsigned short m_equip3;   // 0xa
    unsigned char m_valid;   // 0xc
};

/* the player's account (the online lane's AccountInfoType: only its id) */
struct AccountInfoType {
    int AccountID;   // 0x0
};
extern AccountInfoType AccountInfo;

/* the lobby's save files on the server (zsav_lobby.h's CLobbySaveManager: only what CharacterProfile
 * calls; zsav_lobby.cpp's) */
class CLobbySaveManager {
public:
    bool Pending();                                     /* 0x33E7F0 */
    bool Load(const char *name, int version);           /* 0x33E850 */
    bool Fetch(const char *name, int a, int b);         /* 0x33EA10 */
    bool Save(const char *name, int version, int flags);   /* 0x33EB70 */
};
CLobbySaveManager *GetLobbySaveManager();   /* 0x2AFBA0 */

/* the account database (the online lane's CZUniversInformationServer::CAcctDB: what the handlers ask
 * of it -- the personas on a host, the patch level, the servers, the news) */
/* an account's gender: INT-SIZED, as the online lane's (zNetMediusUniverse.cpp, round 14): the
 * handler's `li 1; movz` is a ternary of its enumerators passed to an enum parameter */
enum ACCT_GENDER { ACCT_MALE = 0, ACCT_FEMALE = 1, ACCT_GENDER_EXTENT = 0x7FFFFFFF };
class CZUniversInformationServer {
public:
    class CAcctDB {
    public:
        int Count(const char *host, int port);                              /* 0x2FB920 */
        int Find(const char *host, int port, const char *name);             /* 0x2FBBF0 */
        void SetAcctInfo(const char *host, int port, const char *name, const char *password,
                         const char *hometown, ACCT_GENDER gender, bool secure);    /* 0x2FD0D0 */
        const char *GetName(const char *host, int port, int i);             /* 0x2FD510 */
        int IndexOf(const char *host, int port, const char *name);          /* 0x2FB9F0 */
        int GetUniverseStatus(const char *name);                            /* 0x2FD7E0 */
        int GetUniversePort(const char *name);                              /* 0x2FD8B0 */
        const char *GetUniverseDNS(const char *name);                       /* 0x2FD980 */
        void Delete(const char *host, int port, int index);                 /* 0x2FBAE0 */
        AcctLoadout *GetLoadout(const char *host, int port, const char *name, const char *mission,
                                int side);                                  /* 0x2FB470 */
        void SetLoadout(const char *host, int port, const char *name, const char *mission,
                        AcctLoadout *loadout, int side);                    /* 0x2FB610 */
        int GetGender(const char *host, int port, int index);               /* 0x2FD110 */
        bool GetSavePassword(const char *host, int port, int index);        /* 0x2FD210 */
        const char *GetTown(const char *host, int port, int index);         /* 0x2FD310 */
        const char *GetPassword(const char *host, int port, int index);     /* 0x2FD410 */
        const char *GetPatchURL(const char *level);                         /* 0x2FD610 */
        bool VerifyPatchLevel(const char *level);                           /* 0x2FD6E0 */
        const char *GetUniverseDescription(unsigned int server);            /* 0x2FDA50 */
        const char *GetUniverseName(unsigned int server);                   /* 0x2FDAA0 */
        const char *GetNewsLine(int line);                                  /* 0x2FDEA0 */
        int GetNewsCount();                                                 /* 0x2FDEC0 */
    };
};

/* the save files' manager (zsave.h's CSaveManager: only a file's header read) */
class CSaveManager {
public:
    class CSaveHeader;
    bool GetHeader(const char *name, int flags, CSaveHeader *header);   /* 0x33D4A0 */
};

/* ---- placeholders: methods and functions not named yet (SOCOM 1's names, by the call SOCOM 1's
 * handler makes at the same place, in the comments) ---- */
extern "C" {
/* the single-player career's stats (fts_mission.cpp's CCareerStats, theMission's at 0x1950: only the
 * two the UI commands call) */
class CCareerStats {
public:
    void Clear();   /* 0x2A3E20 */
    void SetUIVars();   /* 0x2A3BC0 */
};

/* the sounds' stop (SOCOM 1's StopAllSounds) */
void func_00346C00();

/* NetCnf */
void func_0030C6A0();
void func_0030C6E0();


/* the chat commands' body (SOCOM II's; zanim_menu.cpp's, 0x281D40) */
void SendChatLine(C2DAnimCmdHdr *hdr, UITextCallback cb, int lobby);

/* more of the lobby (SOCOM 1's names by the calls SOCOM 1's handlers make) */
bool func_0030BF20();                                               /* (whether the network is up) */
bool func_002B85C0();                                               /* (connect to the game server) */
void func_002EFE50(CZOnlineLobby *lobby, const char *name, int what);   /* (a player's info) */
void func_002B8660();
void func_001F3BB0(CMenuState *state, float time);                  /* (the menu's input suspended) */
void func_001F3BD0(CMenuState *state, const char *movie);           /* (the background film set) */
bool func_001F3BE0(CMenuState *state);                              /* (the background film stopped: whether one was playing) */
char *func_0039C580();
bool func_001EB070(void *core, const char *eventname, const char *argument);       /* (CCoreState's RunEventAnim) */

/* the online services' account database at 0x44FC60 (patches, personas, accounts, news: the
 * online lane's CZUniversInformationServer::CAcctDB, its object not named yet) */
extern CZUniversInformationServer::CAcctDB data_0044FC60;

/* the auto-aim option (a byte, on in data: SOCOM 1's _AutoAim, 0x3DF1A0 -- its CheckDIShoot reads it as
 * SOCOM II's 0x5AAB14 does) */
extern unsigned char _AutoAim;

/* the game's session as the network reports it (func_0030CDD0 fills it; its third word the
 * players joined) */
struct NetSessionInfo {
    int m_0;
    int m_4;
    int m_count;   // 0x8
    int m_c;
};
void func_0030CDD0(CZNetwork *net, NetSessionInfo *info);

/* a network interface as the network lists it (0x240 bytes; two names at 8 and 0x108; ours) */
struct NetInterfaceInfo {
    unsigned char _0[8];
    char m_vendor[256];   // 0x8
    char m_product[256];   // 0x108
    unsigned char _208[0x240 - 0x208];
};
int func_0030BD40(NetInterfaceInfo *list, int max);

/* the multiplayer missions (an object at 0x441650: a mission's record, its index at 0x10) */
struct MPMissionView {
    unsigned char _0[0x10];
    int m_index;   // 0x10
};

/* the missions as GetMissionInformation walks them: the same object seen as a vector of records,
 * each a name, whether it is multiplayer and its number (fts_missionlist.cpp's CMultiMission, only the
 * fields the handlers read) */
struct CMultiMission {
    std::string m_name;   // 0x0
    unsigned char m_multiplayer;   // 0xc
    int m_number;   // 0x10
    unsigned char m_kind;   // 0x14: the game's kind (GetMultiValue's byte; 5 the one with respawns)
    unsigned char _15[0x1c - 0x15];
    unsigned char m_on_hdd;   // 0x1c: read off the hard disk (Purge erases these)
};
/* a mission's description as the hard disk's files hold it (39 bytes; HddFindFiles' copy) */
struct MissionDesc {
    unsigned char m_data[39];
};
extern std::vector<CMultiMission *> data_00441650;
/* the same object as fts_missionlist.cpp's CMultiMissionList (a pointer vector of the missions,
 * OVL1 0x2C8840's this): only its random pick and the mission after a given one */
class CMultiMissionList : public std::vector<CMultiMission *> {
public:
    MPMissionView *GetRandom(bool multi);
    /* the mission after the one of an id and kind (fts_missionlist.cpp's, 0x2C8DB0) */
    MPMissionView *GetNext(int id, bool multi);
    /* a mission's multiplayer kind, by its id (fts_missionlist.cpp's, 0x2C8B80) */
    signed char GetMultiValue(int id);
    /* the hard disk's missions erased (0x2C90B0); a downloaded one entered (0x2C91A0) */
    void Purge();
    CMultiMission *Add(const MissionDesc *desc);
};
bool func_002F0EA0(CZOnlineLobby *lobby, C2D *list);
bool func_002F11F0(CZOnlineLobby *lobby, C2D *list);

/* the network configurations found (an object at 0x457C40: the count, then 0x68-byte entries from
 * 0x30, each valid when its first word is not negative, its name at 8; ours) */
struct NetCnfEntry {
    int m_type;   // 0x0
    unsigned char _4[4];
    char m_name[32];   // 0x8 (Sony's ezNetCnf combination: the combination's name, then the
    char m_ifc[32];    // 0x28   interface's and the device's)
    char m_dev[32];    // 0x48
};

struct NetCnfList {
    /* (an accessor: the handlers test the entry's address for null) */
    NetCnfEntry *Entry(int i) { return &m_entries[i]; }

    int m_count;   // 0x0
    unsigned char _4[0x30 - 0x4];
    NetCnfEntry m_entries[16];   // 0x30
};
extern NetCnfList data_00457C40;
extern char data_004567C0[];
NetCnfEntry *ezNetCnfGetDefault(NetCnfList *list);                       /* (the configuration in use) */
void func_002BADA0();                                               /* (the side swap finished) */


/* the network configuration's device path ("mc0:" in data) */
extern char data_003DF360[];                                              /* (a path: the game's directory?) */

/* SOCOM II's: whether the player is a terrorist, whether the player was banned */
bool func_002C31A0();
bool func_002C3080();
bool func_002C2FA0();
bool func_002C3550();
}

/* whether a mission is its name's newest: the list's Find of its name gives it back (fts_missionlist.cpp's
 * IsNewestVersion 0x2C94F0, GetRandom's pool test) */
bool IsNewestVersion(CMultiMission *m);

#endif
