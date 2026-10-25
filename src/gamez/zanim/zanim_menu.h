/* zanim_menu.h -- what the menu's UI commands (zanim_menu.cpp) work on: the 2D anim command record,
 * the UI command table, and the classes the handlers call -- declarations only, each as much as
 * the handlers use. Layouts are SOCOM 1's DWARF where it has the class, moved to SOCOM II's offsets
 * as the handlers read them; methods SOCOM II's map has not named yet are called through their
 * placeholders (the C functions at the end, `this` first). The header's name is ours. */
#ifndef ZANIM_MENU_H
#define ZANIM_MENU_H

#include <string>

#include "zanim.h"
#include "../ztwod/ztwod.h"
#include "../zinput/zinput.h"

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

/* a command's type by its name (zanim_menu.cpp's) */
int GetUICommandType(const char *name);

/* the reader's lookups the parsers use (SOCOM 1's zrdr_findstring; the others not named yet:
 * a tag's bool, its rectangle, a string in an array) */
char *zrdr_findstring(const _zrdr *rdr, const char *tag);
extern "C" {
bool func_0032DC00(_zrdr *rdr, const char *tag, bool *out);
unsigned char func_0032DC40(_zrdr *rdr, const char *tag, int *out);
bool func_0032DF80(_zrdr *rdr, const char *tag, char **out);
}

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

/* a node's visual as the menu sees it: zdb::CVisual's table up to Get2DVisual, its sixth slot (+0x1C;
 * SOCOM 1's seventh). znode.h's CVisual does not declare it yet, so its out-of-line copy (0x274860,
 * this unit's in both builds: `return 0;`) is still the generated draft's */
class CVisualView {
public:
    unsigned char _0[0x20];
    virtual ~CVisualView();
    virtual void Slot0C();
    virtual void Slot10();
    virtual void Slot14();
    virtual void Slot18();
    virtual C2DVisual *Get2DVisual();
};

/* a UI variable (SOCOM 1's DWARF: CUIVariable : _zrdr, 0x1C bytes) */
enum UIVAR_LONGEVITY {
    UIVAR_TEMPORARY = 0,   /* (the values' names ours: the handlers pass 1 and 2) */
    UIVAR_SESSION = 1,
    UIVAR_PERMANENT = 2
};

class CUIVariable;
extern "C" {
void func_0032F350(_zrdr *value);           /* (a value's storage let go of) */
void func_003863D0(CUIVariable *var);       /* (a variable's change told to what it is attached to) */
}

class CUIVariable : public _zrdr {
public:
    /* a number set, unless it already holds it (ours: ClearUIVar's and the options' inline) */
    void SetInt(int value)
    {
        if (type != 1 || integer != value) {
            func_0032F350(this);
            type = 1;
            integer = value;
            func_003863D0(this);
        }
    }
    void SetReal(float value)
    {
        if (type != 2 || real != value) {
            func_0032F350(this);
            type = 2;
            real = value;
            func_003863D0(this);
        }
    }

    char *m_varname;   // 0x8
    unsigned char m_attached_items[0xc];   // 0xc: SOCOM 1's vector
    UIVAR_LONGEVITY m_longevity;   // 0x18
};

/* the UI variables (SOCOM 1's DWARF: CUIVarManager : vector, CSaveModule, 0x8064 bytes) */
class CUIVarManager {
public:
    unsigned char _0[0x8064];
};
extern CUIVarManager theUIVarManager;

/* the online lobby (SOCOM 1's DWARF: 0x98 bytes; the members the handlers read, at SOCOM II's
 * offsets) */
class CZOnlineLobby {
public:
    int RemovePlayerFromClan(const char *name);
    bool OnCorrectClanTeam();
    bool GetClan();
    static void PasswordSecurity();

    unsigned char _0[0x58];
    bool (*m_event_handler)(const char *eventname, const char *argument);   // 0x58 (SOCOM 1's): InitNetwork sets it
    unsigned char _5c[0x60 - 0x5c];
    unsigned char m_current_state;   // 0x60: SOCOM 1's LOBBY_STATE
    unsigned char _61[0x98 - 0x61];
};
extern CZOnlineLobby theLobby;

/* the network (SOCOM 1's CZNetwork, less the two team indices SOCOM II dropped ahead of the flags) */
class CZNetwork {
public:
    int zNetGetValidGameClientCount();
    unsigned int zNetGetTime();

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

    unsigned char _0[0x10];
    CValve *m_10;   // 0x10: a valve AdvanceMPMission tests (a longer wait when on)
    unsigned char _14[0x28 - 0x14];
    CValve *m_pJoinGameValve;   // 0x28 (SOCOM 1's at 0x24)
    unsigned char _2c[0xac - 0x2c];
    int m_persona_object_index;   // 0xac (SOCOM 1's at 0x70)
    unsigned char _b0[0xc8 - 0xb0];
    unsigned int m_c8;   // 0xc8: when the next mission launches (AdvanceMPMission)
    unsigned char m_late_joiner;   // 0xcc (SOCOM 1's at 0x94)
    unsigned char _cd[0xd8 - 0xcd];
    unsigned char m_ten_second_launch;   // 0xd8 (SOCOM 1's at 0x9F)
    unsigned char m_joined;   // 0xd9: CheckJoinMP sets it (SOCOM II's; the name ours)
    unsigned char _da[0xe4 - 0xda];
    unsigned int m_launch_time;   // 0xe4 (SOCOM 1's clock's at 0xA8)
};

class CZPersonaState {
public:
    static void ToggleReady();
};



/* the mission (only what the handlers read) */
class CRdrFile;

class CMission {
public:
    void CreateNetGame();

    unsigned char _0[0x1c];
    char *m_database;   // 0x1c (SOCOM 1's)
    unsigned char _20[0x1808 - 0x20];
    CZNetGame *m_pNetGame;   // 0x1808 (SOCOM 1's at 0x584)
    unsigned char _180c[0x1824 - 0x180c];
    CRdrFile *m_fontsRdr;   // 0x1824 (SOCOM 1's at 0x5A0)
};
extern CMission theMission;

/* a memory card file's information as the saved-game handlers read it (their 272-byte local; the
 * text from 12, "This game is a dummy game." in SaveConfig's dummies; ours) */
struct MemCardFileInfo {
    unsigned char _0[12];
    char m_text[260];   // 0xc
};

/* the memory card (SOCOM 1's CMemCard, 0x28 bytes; only what the handlers use) */
class CMemCard {
public:
    bool IsInserted();
    bool IsFormatted();
    bool Format();
    void CreateIconFiles();
    /* the port and slot chosen made the current ones (SOCOM II's; the name ours) */
    void SelectPortSlot()
    {
        m_cur_slot = m_slot;
        m_cur_port = m_port;
    }

    unsigned char _0[0x8];
    int m_cur_port;   // 0x8
    int m_cur_slot;   // 0xc
    unsigned char _10[0x18 - 0x10];
    void *m_18;   // 0x18: what the file information is read from (0x33D4A0)
    unsigned char _1c[0x20 - 0x1c];
    int m_port;   // 0x20
    int m_slot;   // 0x24 (SOCOM 1's MC_ChSlot sets it)
};
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

    unsigned char _0[0x30];
    char m_DefaultMenu[64];   // 0x30
    unsigned char _70[0x74 - 0x70];
};

class CGameState;

class COurGame {
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

/* the menu's pop-up (SOCOM 1's CPopUpDialog, a C2D: its table's first slot the destructor) */
class CPopUpDialog : public C2D {
public:
    virtual ~CPopUpDialog();
};

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

    CMenuEntry m_entries[40];   // 0x0 (SOCOM 1's 20)
    int m_count;   // 0x820: entries pushed (the names ours)
    int m_top;   // 0x824: the top entry's index
    CMenuHistory m_history;   // 0x828 (SOCOM 1's at 0x418)
    CPopUpDialog *m_popup;   // 0xe40 (SOCOM 1's at 0xA30)
};
CMenuStack *GetMenuStack();

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
    unsigned char _0[0x2e8];
    unsigned char m_weapons[0xc];   // 0x2e8 (SOCOM 1's CCharacterWeapList at 0x250)
};

const char *GetCustomCharTypeName(int index);

/* ---- the network's start-up: a simple state machine ---- */

/* a state machine whose states are member functions, each told what happens to it: 0 entered, 1
 * ticked, 2 left (SOCOM II's CSimpleStateMachine, by its RTTI at 0x3EF738, its unit at 0x3A5xxx; the
 * members' names ours). It runs on a thread when it has one (at 4); a state change goes through a
 * queue (at 0x14: a deque of the 12-byte member pointers, 42 to a block, 0x2852B0 its push) */
class CSimpleStateMachine;
typedef void (CSimpleStateMachine::*SSMState)(unsigned char event);

extern "C" {
void func_002852B0(void *queue, SSMState state);    /* the state queue's push */
void func_003A5C30(CSimpleStateMachine *sm);        /* (the machine's tick) */
bool func_003A5C10(CSimpleStateMachine *sm);        /* (whether it is done: no thread and the flag) */
}

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

        func_002852B0(m_queue, s);
    }

    void Idle(unsigned char event);     /* 0x3A5CB0: nothing */
    void Finish(unsigned char event);   /* 0x3A5C70: done once entered or ticked */

    unsigned int m_thread;   // 0x4
    SSMState m_state;   // 0x8
    unsigned char m_queue[0x18];   // 0x14
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

/* ---- placeholders: methods and functions not named yet (SOCOM 1's names, by the call SOCOM 1's
 * handler makes at the same place, in the comments) ---- */
extern "C" {
/* CZOnlineLobby */
bool func_002E3790(CZOnlineLobby *lobby, const char *room);         /* JoinRoom */
bool func_002E72A0(CZOnlineLobby *lobby, const char *name);         /* RevokeInvite */
bool func_002E6830(CZOnlineLobby *lobby);                           /* IsMemberOfClan */
void func_002E6850(CZOnlineLobby *lobby);                           /* DisbandClan */
void func_002E69F0(CZOnlineLobby *lobby);                           /* GetClanRoster */
void func_002E6EA0(CZOnlineLobby *lobby);                           /* GetInvitedPlayers */
bool func_002E7290(CZOnlineLobby *lobby);                           /* ClanHasInvitations */
void func_002E7D70(CZOnlineLobby *lobby);                           /* UpdateClanStats */
void func_002E8460(CZOnlineLobby *lobby);                           /* (clan messages) */
void func_002E8650(CZOnlineLobby *lobby);                           /* CheckClanInvitations */
bool func_002E8770(CZOnlineLobby *lobby);                           /* HasClanInvitations */
void func_002E8EF0(CZOnlineLobby *lobby);                           /* QuitClan */
bool func_002E5FE0(CZOnlineLobby *lobby);                           /* (more invitations?) */
void func_002E6310(CZOnlineLobby *lobby);                           /* (leader messages) */
void func_002EAB80(CZOnlineLobby *lobby, int arg);                  /* (clan ranking; mis-named Pop__5CGameFUi) */
void func_002F4240(CZOnlineLobby *lobby);                           /* Bailout */
void func_002F4860(CZOnlineLobby *lobby, const char *text, const char *to);   /* SendChat */
void func_002F7710(CZOnlineLobby *lobby);                           /* GetAnnouncements */
void func_002F79F0(CZOnlineLobby *lobby);                           /* (usage policy) */
void func_002F8350(CZOnlineLobby *lobby);                           /* ForceStatusReport */
bool func_002F8CE0(CZOnlineLobby *lobby);                           /* (switch teams OK) */
void func_002F95F0(CZOnlineLobby *lobby);                           /* BanSelfFromGame */
void func_002FA7F0(CZOnlineLobby *lobby, const char *desc);         /* SetPlayerDescription */
void func_00300C50(CZOnlineLobby *lobby, const char *name);         /* (add to ignore) */
void func_00300F00(CZOnlineLobby *lobby);                           /* (ignore list) */
void func_003012F0(CZOnlineLobby *lobby);                           /* (friend invitations) */
void func_003013C0(CZOnlineLobby *lobby, const char *name);         /* (add friend) */
void func_00301750(CZOnlineLobby *lobby);                           /* (friends list) */

/* CUIVarManager::Add (SOCOM 1's Add__13CUIVarManagerFPCcP5_zrdr15UIVAR_LONGEVITY) */
CUIVariable *func_003857C0(CUIVarManager *vars, const char *name, _zrdr *value, UIVAR_LONGEVITY life);

/* CZNetwork::zNetGetLocalTime (SOCOM 1's) */
unsigned int func_0030D540(CZNetwork *net);

/* the career stats (an object at 0x437E30) */
void func_002A3BC0(void *stats);
void func_002A3E20(void *stats);

/* the sounds' stop (SOCOM 1's StopAllSounds) */
void func_00346C00();

/* NetCnf */
void func_0030C6A0();
void func_0030C6E0();

/* the memory card's calls (SOCOM 1's names: Exists, mkdir, chdir, GetFreeSpace; LoadConfig) */
bool func_003A0950(CMemCard *card, const char *name);
bool func_003A0890(CMemCard *card, const char *name);
bool func_003A07D0(CMemCard *card, const char *name);
int func_003A1210(CMemCard *card);
int func_003A1060(CMemCard *card, int arg);
void func_003A1730(CMemCard *card, const char *name);
void func_003A1C60(CMemCard *card);
void func_003A17E0(CMemCard *card, const char *name, int arg);
void func_003A1880(CMemCard *card);
void func_003A1EA0(CMemCard *card, const char *name, const char *text);

/* a UI variable's text set (SOCOM 1's CUIVariable::Set) */
void func_003862A0(CUIVariable *var, const char *text);

/* the automatic messages' switch, and its state */
void func_005E7FB0(int on);
int func_005E7FC0();

/* the text entry the chat commands open (SOCOM II's) */
void func_00281D40(C2DAnimCmdHdr *hdr, UITextCallback cb, int lobby);

/* more of the lobby (SOCOM 1's names by the calls SOCOM 1's handlers make) */
bool func_002E67D0(CZOnlineLobby *lobby, const char *name);         /* IsLeaderOfClan */
bool func_002E7600(CZOnlineLobby *lobby, const char *name);         /* InvitePlayerToClan */
bool func_002E8310(CZOnlineLobby *lobby, const char *text);         /* SendClanMessage */
void func_002EFE20(CZOnlineLobby *lobby, int state);                /* (the lobby's state set) */
void func_002DFE50(CZOnlineLobby *lobby);                           /* (the lobby reset) */
void func_0030E290(CZNetwork *net);                                 /* (the network's initialisation started) */
bool func_0030DEE0(CZNetwork *net);                                 /* (whether it started) */
bool func_0030BF20();                                               /* (whether the network is up) */
void func_0033D4A0(void *src, const char *name, int what, MemCardFileInfo *info);   /* (a file's information) */
int func_002FBBF0(void *svc, const char *host, int port, const char *name);         /* (whether a persona exists) */
void func_002ACC10(CMission *mission, const char *name);            /* (the mission set by name) */
void func_0034F9B0(CIOPManager *iop, int mode);                     /* CIOPManager::RebootIOP (named: zinput.h does not declare it) */
void func_002E81B0(CZOnlineLobby *lobby, const char *name);         /* TransferLeadership */
bool func_002F8D50(CZOnlineLobby *lobby, const char *name);         /* (game has a spectator password) */
bool func_002F8BC0(CZOnlineLobby *lobby, const char *name);         /* (game rejects the rank) */
bool func_002F8C60(CZOnlineLobby *lobby, const char *name);         /* (clan game closed) */
bool func_002B85C0();                                               /* (connect to the game server) */
int func_003A1CF0(CMemCard *card);                                  /* (the card's error code) */
bool func_003A1370(CMemCard *card, int arg, int *slot);             /* (the card changed) */
void *func_003A40E0(const char *name);                              /* (a locale by name) */
void func_003A44B0(void *locale);                                   /* (the locale selected) */
void func_0034F950();                                               /* zSysPostInit (SOCOM 1's) */
bool func_00300840();                                               /* (Medius initialised) */
void func_002EFE50(CZOnlineLobby *lobby, const char *name, int what);   /* (a player's info) */
void func_002E05D0(CZOnlineLobby *lobby);                           /* (the DME's start) */
bool func_0030DBB0(CZNetwork *net);                                 /* (the DME initialised) */
void func_002B8660();
void func_002E60C0(CZOnlineLobby *lobby, int index, const char *text);   /* (a clan message changed) */
CUIVariable *func_00385900(CUIVarManager *vars, const char *name);  /* CUIVarManager::Find (SOCOM 1's) */
void func_002F4DF0(CZOnlineLobby *lobby, const char *text, void (*done)(const char *, void *), void *data);   /* Censor */
void func_002E8980(CZOnlineLobby *lobby, int index);                /* DeclineClanInvitation */
void func_002E89F0(CZOnlineLobby *lobby, int index);                /* AcceptClanInvitation */
void func_002E6000(CZOnlineLobby *lobby, const char *name);         /* (clan member's info) */
void func_002E6010(CZOnlineLobby *lobby, int index);                /* (clan message deleted) */
void func_00300FE0(CZOnlineLobby *lobby, int index);                /* (removed from ignore) */
void func_003016A0(CZOnlineLobby *lobby, int index);                /* (friend removed) */
void func_00301140(CZOnlineLobby *lobby, int index);                /* (friend's info) */
void func_00300AF0(CZOnlineLobby *lobby, int index);                /* (friend rejected) */
void func_00300A70(CZOnlineLobby *lobby, int index);                /* (friend confirmed) */
void func_00300B70(CZOnlineLobby *lobby, int index);                /* (friend added directly) */
void func_001F3BB0(CMenuState *state, float time);                  /* (the menu's input suspended) */
void func_001F3BD0(CMenuState *state, const char *movie);           /* (the background film set) */
bool func_001F3BE0(CMenuState *state);                              /* (the background film stopped: whether one was playing) */
void func_003A1E10(CMemCard *card, const char *name);               /* (a saved game loaded) */
double func_00191478(const char *s);                                /* atof */
void func_0037B980(float f);                                        /* UiSetTtySpeedFactor (SOCOM 1's) */
void func_0037B990(float f);                                        /* UiSetCounterSpeedFactor */
void func_002C5D60(const char *name);                               /* CCtrlrConfigs::SelectConfig (SOCOM 1's) */
CCharacterType *func_0053E170(const char *name);                    /* CCharacterType::GetByNameMod (SOCOM 1's) */
char *func_0039C580();
bool func_002F6EC0(CZOnlineLobby *lobby, C2D *list);                /* RefreshGamePlayerList */
bool func_002E3A30(CZOnlineLobby *lobby, C2D *list);                /* RefreshRoomList */
bool func_002E34E0(CZOnlineLobby *lobby, const char *room);         /* (can join the room) */
bool func_002F83B0(CZOnlineLobby *lobby, const char *game);         /* JoinGame */
void func_002E7A10(CZOnlineLobby *lobby, const char *name);         /* CreateClan */
bool func_002F8DD0(CZOnlineLobby *lobby, const char *game);         /* GameIsPasswordProtected */
void func_002CF2F0(COurGame *game, unsigned int arg);               /* CGame::Pop (the states lane's note) */
void func_002CF410(COurGame *game, void *state, unsigned int arg);  /* CGame::Switch */
void func_002CF380(COurGame *game, void *state, unsigned int arg);  /* CGame::Push (SOCOM 1's, by UIPlayMPEG) */
void func_001EDA60(void *cine, const char *movie, int a, int b);    /* CCinematicState::SetMovie (SOCOM 1's took the name only) */
void func_002E24D0(CZOnlineLobby *lobby);                           /* (the current game's info) */
void func_002E1F20(CZOnlineLobby *lobby, const char *game);         /* (a game's info) */
bool func_002FA5A0(CZOnlineLobby *lobby, const char *name, const char *password);   /* Login */
bool func_002FA320(CZOnlineLobby *lobby, const char *name, const char *password);   /* Register */
bool func_001EB070(void *core, const char *eventname, const char *argument);       /* (CCoreState's RunEventAnim) */

/* the online services' object at 0x44FC60 (patches, persona, account: not named yet) */
extern char data_0044FC60[];
bool func_002FD6E0(void *svc, const char *level);                   /* (the patch level verified) */
const char *func_002FD610(void *svc, const char *level);            /* (the patch server's URL) */

/* the auto-aim option (a byte, on in data) */
extern unsigned char data_003DF1A0;

/* CSnd::m_vagEnabled (SOCOM 1's static; zsnd.h's CSnd does not declare it yet) */
extern unsigned char data_0048DDA0;

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
 * each a name, whether it is multiplayer and its number (ours) */
struct MPMissionInfo {
    std::string m_name;   // 0x0
    unsigned char m_multiplayer;   // 0xc
    int m_number;   // 0x10
};
extern std::vector<MPMissionInfo *> data_00441650;
MPMissionView *func_002C8DB0(void *missions, int number, int arg);
MPMissionView *func_002C8840(void *missions, int arg);
int func_002F8120(CZOnlineLobby *lobby);
bool func_003042B0(void *lan, C2D *list);
bool func_002E28D0(CZOnlineLobby *lobby, C2D *list);
bool func_002F0EA0(CZOnlineLobby *lobby, C2D *list);
bool func_002F11F0(CZOnlineLobby *lobby, C2D *list);

/* the network configurations found (an object at 0x457C40: the count, then 0x68-byte entries from
 * 0x30, each valid when its first word is not negative, its name at 8; ours) */
struct NetCnfEntry {
    int m_type;   // 0x0
    unsigned char _4[4];
    char m_name[0x60];   // 0x8
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
NetCnfEntry *func_001E8890(NetCnfList *list);                       /* (the configuration in use) */
int func_001E8910(const char *device, const char *name, void *out); /* (a configuration selected) */
unsigned char func_002E99E0();                                      /* (the lobby's game state) */
void func_002BADA0();                                               /* (the side swap finished) */

/* the network configurations as a reader value: an 80-byte object NetCnfClose builds on its stack
 * (its constructor and destructor not named yet, so called through their placeholders) */
struct NetCnfRdr {
    unsigned char _0[80];
};
void func_00333AE0(NetCnfRdr *rdr);                 /* (its constructor) */
void func_00333A20(NetCnfRdr *rdr, int flags);      /* (its destructor) */
void func_003339B0(NetCnfRdr *rdr, int arg);
void func_00333770(NetCnfRdr *rdr);
void func_00333610(NetCnfRdr *rdr, const char *text);
void func_00386460(CUIVariable *var, NetCnfRdr *rdr);
const char *func_002FDA50(void *svc, int server);   /* (a server's message) */
const char *func_002FDAA0(void *svc, int server);   /* (a server's name) */
int func_002FDEC0(void *svc);                       /* (the news' line count) */
const char *func_002FDEA0(void *svc, int line);     /* (a line of the news) */

/* the network configuration's device path ("mc0:" in data) */
extern char data_003DF360[];                                              /* (a path: the game's directory?) */
void func_0032C2B0(const char *archive, const char *name);          /* CRdrArchive::RemoveArchive (SOCOM 1's) */

/* SOCOM II's: whether the player is a terrorist, whether the player was banned */
bool func_002C31A0();
bool func_002C3550();
void func_002241C0();
}

#endif
