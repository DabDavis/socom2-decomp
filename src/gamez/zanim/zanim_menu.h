/* zanim_menu.h -- what the menu's UI commands (zanim_menu.cpp) work on: the 2D anim command record,
 * the UI command table, and the classes the handlers call -- declarations only, each as much as
 * the handlers use. Layouts are SOCOM 1's DWARF where it has the class, moved to SOCOM II's offsets
 * as the handlers read them; methods SOCOM II's map has not named yet are called through their
 * placeholders (the C functions at the end, `this` first). The header's name is ours. */
#ifndef ZANIM_MENU_H
#define ZANIM_MENU_H

#include "zanim.h"
#include "../ztwod/ztwod.h"

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

/* ---- the classes the handlers call ---- */

/* a UI variable (SOCOM 1's DWARF: CUIVariable : _zrdr, 0x1C bytes) */
enum UIVAR_LONGEVITY {
    UIVAR_TEMPORARY = 0,   /* (the values' names ours: the handlers pass 1 and 2) */
    UIVAR_SESSION = 1,
    UIVAR_PERMANENT = 2
};

class CUIVariable : public _zrdr {
public:
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
    int m_58;   // 0x58: UnInitNetwork clears it
    unsigned char _5c[0x60 - 0x5c];
    unsigned char m_current_state;   // 0x60: SOCOM 1's LOBBY_STATE
    unsigned char _61[0x98 - 0x61];
};
extern CZOnlineLobby theLobby;

/* the network (SOCOM 1's CZNetwork, less the two team indices SOCOM II dropped ahead of the flags) */
class CZNetwork {
public:
    int zNetGetValidGameClientCount();

    bool m_bSessionMaster;   // 0x0 (SOCOM 1's at 8)
    bool m_bNetwork;   // 0x1
    unsigned char _2[0x108 - 0x2];
    bool m_bMediusEnabled;   // 0x108 (SOCOM 1's at 0xE4)
};
extern CZNetwork theNetwork;

/* the online game (SOCOM 1's CZNetGame: only what the handlers read) */
class CZNetGame {
public:
    unsigned char _0[0xac];
    int m_persona_object_index;   // 0xac (SOCOM 1's at 0x70)
    unsigned char _b0[0xcc - 0xb0];
    unsigned char m_late_joiner;   // 0xcc (SOCOM 1's at 0x94)
};

/* the mission (only what the handlers read) */
class CRdrFile;

class CMission {
public:
    void CreateNetGame();

    unsigned char _0[0x1808];
    CZNetGame *m_pNetGame;   // 0x1808 (SOCOM 1's at 0x584)
    unsigned char _180c[0x1824 - 0x180c];
    CRdrFile *m_fontsRdr;   // 0x1824 (SOCOM 1's at 0x5A0)
};
extern CMission theMission;

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
    unsigned char _10[0x20 - 0x10];
    int m_port;   // 0x20
    int m_slot;   // 0x24 (SOCOM 1's MC_ChSlot sets it)
};
extern CMemCard theMemCard;

/* the game and its load state (only what the handlers call) */
class CLoadState {
public:
    void AbortLoad(const char *name);
};

class COurGame {
public:
    unsigned char _0[0xec];
    CLoadState m_loadState;   // 0xec (SOCOM 1's at 0xE0)
};
extern COurGame theGame;

zdb::CNode *GetMenuNode();

/* the menu's screen stack (SOCOM 1's CMenuStack, 0xA34 bytes there, 0xE44 here): the history of
 * screens, kept as names in string slots taken from a free list, and the pop-up (SOCOM II's offsets
 * read off AddToHistory and KillPopUp; the members' names ours) */
class CMenuStack {
public:
    void KillPopUp();
    void CleanPopup();
    void AddToHistory(const char *name);
    void PopUpDialog(const char *name);
    void FlushHistory(bool all);

    unsigned char _0[0x828];
    std::vector<char *> m_history;   // 0x828 (SOCOM 1's at 0x418)
    unsigned char _834[0xe34 - 0x834];
    std::vector<char *> m_free;   // 0xe34 (SOCOM 1's at 0xA24): the unused name slots
    void *m_popup;   // 0xe40 (SOCOM 1's at 0xA30)
};
CMenuStack *GetMenuStack();

/* the network calls the handlers make */
void NetSendRequestTeamSwitch(int persona);
void SendChatMsg(const char *text);

/* text the user types, sent to a callback with whom it is for (SOCOM II's; 0x281D40) */
typedef void (*UITextCallback)(const char *text, const char *to);

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

/* SOCOM II's: whether the player is a terrorist, whether the player was banned */
bool func_002C31A0();
bool func_002C3550();
void func_002241C0();
}

#endif
