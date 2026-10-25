/* zanim_menu.cpp -- the menu's ZAnim commands and the scene and database they work on (OVL1
 * 0x2744F0-...). The unit name is SOCOM 1's: its anonymous-namespace handlers mangle as
 * @unnamed@zanim_menu_cpp@. Defined in reverse of their image order (-inline deferred).
 *
 * The UI commands: a menu's sequence names a command (C2DAnimCmdHdr), zAnimUIMenuTick looks its
 * name up in UICommandTable (0x3DD4D0, 208 records in SOCOM II, 149 in SOCOM 1's) and calls the
 * record's handler with the command and the frame time; the handler returns whether the command is
 * done. Both builds register their handlers in that table by name, so each handler's row is named
 * by the command it is registered under: SOCOM 1's handler name for a command SOCOM 1 registers
 * (UI<command> mostly, some in the anonymous namespace), UI<command> after SOCOM 1's convention for
 * SOCOM II's new ones. The functions above 0x280000 (the equipment and saved-game handlers, the
 * template instances that close the unit) are not written yet. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "zanim_menu.h"
#include "../zsys/zsys.h"

/* the lobby objects the handlers also reach (not named yet) */
class CZLANLobby;
extern "C" {
extern CZLANLobby data_0044FE10;           /* a second lobby: the LAN game's */
bool func_003045B0(CZLANLobby *lan);        /* whether the LAN lobby is the one in use */
void func_003043E0(CZLANLobby *lan);
void func_00303F40(CZLANLobby *lan);
bool func_003039B0(CZLANLobby *lan, const char *game);
bool func_00303730(CZLANLobby *lan, const char *game);
void func_002DFDD0(CZOnlineLobby *lobby);
void func_002DF6A0(CZOnlineLobby *lobby);
void func_002DF8E0(CZOnlineLobby *lobby);
void func_002E3730(CZOnlineLobby *lobby);
void func_002F80C0(CZOnlineLobby *lobby);
bool func_002F9410(CZOnlineLobby *lobby);
void func_002FA560(CZOnlineLobby *lobby);
void func_0030D770(CZNetwork *net);
void func_0030E3A0(CZNetwork *net, int arg);
void func_002A9EF0(CMission *mission);
bool func_002B83E0();                       /* DMEJoinGameServer (SOCOM 1's, by UIJoinGameServer) */
extern int data_00441470;
extern char data_00437E30[];               /* the career stats */
}

/* the online armory (an object at 0x45B718; only the flag VCButton tests) */
struct COnlineArmoryView {
    unsigned char _0[0x4a];
    unsigned char m_4a;   // 0x4a
};

extern "C" {
extern COnlineArmoryView *data_0045B718;
void func_0030E6E0(COnlineArmoryView *p, int arg);
void func_0030E490(COnlineArmoryView *p);
}

class CZGameState {
public:
    static void UpdateLocalGame(CZNetGame *game);
};
void ftsNetworkGameInit();
void ftsNetworkGameUninit();
bool DMEConnectToGameServer();
void DMEDisconnectFromGameServer();

/* the 2D object a command's node draws, if it has one (ours) */
inline C2D *GetCommandC2D(unsigned short node_ref)
{
    C2D *c2d = 0;

    if (node_ref) {
        ZAnim.m_CurAnim->Refresh(false);
        CNode *node = ZAnim.m_CurAnim->GetNode(node_ref, false);

        if (node) {
            CVisualView *visual = (CVisualView *)node->GetFirstVisual();

            if (visual) {
                C2DVisual *v2d = visual->Get2DVisual();

                if (v2d)
                    c2d = v2d->m_c2d;
            }
        }
    }
    return c2d;
}

char database[64];
zdb::CNode *g_menuRootNode;
/* the saved game's name the memory card handlers build (0x414E50) */
static char s_savename[64];

namespace {

bool OnSetCounterSpeedFactor(C2DAnimCmdHdr *hdr, float *dT)
{
    func_0037B990(func_00191478(ArgText(ArgName(hdr))));
    return true;
}

bool OnSetTtySpeedFactor(C2DAnimCmdHdr *hdr, float *dT)
{
    func_0037B980(func_00191478(ArgText(ArgName(hdr))));
    return true;
}

}

bool UIUpdateMpFinalStatsUiVars(C2DAnimCmdHdr *hdr, float *dT)
{
    func_002241C0();
    return true;
}

/* CopyUIVar's parser: the two variables' names (-1 for none). The command's type goes where the
 * other records keep the command's name (SOCOM II's; the name ours) */
_zanim_cmd_hdr *UICopyUIVarParse(const char *name, _zrdr *rdr)
{
    UICopyUIVarCmd *cmd = (UICopyUIVarCmd *)_calloc(1, sizeof(UICopyUIVarCmd), 0, 0);
    const char *s;

    cmd->header.timeless = 1;
    cmd->header.data_size = sizeof(UICopyUIVarCmd);
    cmd->node_ref_index = 0;
    cmd->m_command_index = GetUICommandType(name);
    s = zrdr_findstring(rdr, "From");
    cmd->m_from = s ? ZAnim.m_CurAnim->AddName(s) : -1;
    s = zrdr_findstring(rdr, "To");
    cmd->m_to = s ? ZAnim.m_CurAnim->AddName(s) : -1;
    return (_zanim_cmd_hdr *)cmd;
}

/* SendChatText's and SendMediusChat's parser: the argument and the UI variable holding whom the
 * text is for (SOCOM II's; the name ours) */
_zanim_cmd_hdr *UIChatParse(const char *name, _zrdr *rdr)
{
    UIChatCmd *cmd = (UIChatCmd *)_calloc(1, sizeof(UIChatCmd), 0, 0);
    const char *s;

    cmd->header.timeless = 1;
    cmd->header.data_size = sizeof(UIChatCmd);
    cmd->node_ref_index = 0;
    cmd->m_command_index = GetUICommandType(name);
    s = zrdr_findstring(rdr, "ARGUMENT");
    if (s)
        cmd->SetArgument(s);
    s = zrdr_findstring(rdr, "RECIP_UIVAR");
    if (s && strlen(s) < 32)
        strcpy(cmd->m_recip_uivar, s);
    else
        cmd->m_recip_uivar[0] = 0;
    return (_zanim_cmd_hdr *)cmd;
}

namespace {

bool UISetControllerConfig(C2DAnimCmdHdr *hdr, float *dT)
{
    func_002C5D60(ArgText(ArgName(hdr)));
    return true;
}

/* a saved game loaded from the slot the argument numbers, the loading valve cleared */
bool _LoadSavedGame(C2DAnimCmdHdr *hdr, float *dT)
{
    int slot = atoi(ArgText(ArgName(hdr)));
    CValve *valve = CValve::GetByName("LoadingSavedGame");

    sprintf(s_savename, "SaveGame%d", slot);
    func_003A1E10(&theMemCard, s_savename);
    if (valve)
        SetValveValue(valve, 0);
    return true;
}

}

void SetDatabase(const char *name)
{
    strcpy(database, name);
}

char *GetDatabase()
{
    return database;
}

void RegisterMenuNode(zdb::CNode *node)
{
    g_menuRootNode = node;
}

void UnRegisterMenuNode()
{
    g_menuRootNode = 0;
}

zdb::CNode *GetMenuNode()
{
    return g_menuRootNode;
}


CMenuStack::CMenuStack()
{
    m_top = 0;
    m_count = 0;
    m_popup = 0;
}

/* OPEN, 1 instruction (round 2: the size now right, written as FlushHistory(true) and CleanPopup()):
 * the history's member destruction reads m_free's data_ through the pointer its this-tests use,
 * `addiu v0,s2,3636 ... lw a0,8(v0)`, where MW folds it into lw a0,0xE3C(s2) -- m_used's, destroyed
 * after, is folded in the target too (lw a0,2096(s2)). Round 1: with FlushHistory written out the
 * same instruction was left; m_used as CMenuHistory's base gives the same fold. include/vector's
 * pointer back() and push_back a level lower would likely settle the first form */
CMenuStack::~CMenuStack()
{
    FlushHistory(true);
    CleanPopup();
}

/* the top entry's command done with: its argument's name reset, the entry popped as its command
 * says (SOCOM 1's, the same ten cases in the same order; the name unused in both) */
void CMenuStack::HandleCommand(const char *name)
{
    strcpy(m_entries[m_top].m_argname, DEFAULT_ARGNAME);
    CMenuEntry &entry = m_entries[m_top];

    switch (entry.m_id) {
    case -1:
        break;
    case 0:
        entry.m_id = -1;
        break;
    case 2:
        entry.m_id = -1;
        m_top--;
        break;
    case 1:
        m_top--;
        break;
    case 3:
    case 4:
        entry.m_id = -1;
        m_top--;
        break;
    case 5:
        entry.m_id = -1;
        m_top--;
        break;
    case 6:
    case 7:
    case 8:
        entry.m_id = -1;
        m_top--;
        break;
    }
    m_count--;
}

void CMenuStack::AddToHistory(const char *name)
{
    char *slot = m_history.m_free.back();

    m_history.m_free.pop_back();
    m_history.m_used.PtrVec::insert(m_history.m_used.PtrVec::end(), 1, (unsigned int &)slot);
    strcpy(slot, name);
}

/* the pop-up put on the stack with its argument (the text typed when the name says INPUT_STRING),
 * its node shown and the menu's sound played (SOCOM 1's, the sound SOCOM II's) */
void CMenuStack::PopUpDialog(const char *name)
{
    m_top++;
    std::string arg;

    if (!strcmp(name, "INPUT_STRING"))
        arg = ZAnim.m_CurAnim->m_arg_list->GetCharP(0);
    else
        arg = name;
    m_entries[m_top].m_id = 3;
    strcpy(m_entries[m_top].m_argname, arg.c_str());
    m_count++;
    CNode *node = GetMenuNode()->FindChild("POPUP", true);

    if (!node)
        return;
    node->SetActive(true);
    CSnd *sound = CSnd::GetSoundByName(".METAL");

    if (sound)
        sound->PlaySound(true, 0, 255);
}

/* the mission's reader archive let go of (SOCOM 1's builds the path and does not use it either) */
void Unload_MissionReader()
{
    const char *db = theMission.m_database;

    if (db) {
        char path[1024] = "run/";

        func_0039C580();
        strcat(path, db);
        func_0032C2B0("readerm.zar", db);
    }
}

CRdrFile *FetchMissionFontsRdr()
{
    return theMission.m_fontsRdr;
}

void CMenuStack::CleanPopup()
{
    if (m_popup) {
        delete m_popup;
        m_popup = 0;
    }
}

/* (the base's push_back() and end() stand in for the vector's push_back() and back(): include/
 * vector's pointer back() is three high -- it calls the base's, which calls end() -- and its
 * push_back two. With them the name is fetched out of line (push_back's argument is checked a level
 * deeper), and FlushHistory, ~CMenuStack and FlushMenuHistory all need these pieces inlined whole
 * at their depths, which only the one-level forms give: SOCOM II's back() must be a level lower) */
void CMenuHistory::Flush()
{
    while (m_used.size()) {
        m_free.PtrVec::push_back(*(m_used.PtrVec::end() - 1));
        strcpy((char *)*(m_free.PtrVec::end() - 1), MENUHISTORY_STRPOOL);
        m_used.pop_back();
    }
}

void CMenuStack::FlushHistory(bool all)
{
    for (int i = 0; i < 40; i++)
        strcpy(m_entries[i].m_argname, DEFAULT_ARGNAME);
    m_history.Flush();
    if (m_popup && all) {
        delete m_popup;
        m_popup = 0;
    }
}

void CMenuStack::KillPopUp()
{
    if (m_popup) {
        CNode *node = GetMenuNode()->FindChild("POPUP", true);

        if (node)
            node->SetActive(false);
    }
}

/* (pinned: the stack's constructor calls it, where MW would inline it) */
#pragma auto_inline off
CMenuHistory::CMenuHistory()
{
    m_used.reserve(32);
    m_free.reserve(32);
    for (unsigned int i = 0; i < 32; i++) {
        strcpy(m_pool[i], MENUHISTORY_STRPOOL);
        m_free.push_back(m_pool[i]);
    }
}
#pragma auto_inline reset

/* the mission set by name (or by the UI variable of that name), the background film stopped
 * around the change and put back */
bool UISetMission(C2DAnimCmdHdr *hdr, float *dT)
{
    const char *name = ArgText(ArgName(hdr));
    bool background = func_001F3BE0(&theGame.m_menuState);
    CUIVariable *var = func_00385900(&theUIVarManager, name);

    if (var && var->type == 3)
        name = var->String();
    strcpy(database, name);
    func_002ACC10(&theMission, name);
    if (background)
        func_001F3BD0(&theGame.m_menuState, 0);
    return true;
}

bool UIPopUpDialog(C2DAnimCmdHdr *hdr, float *dT)
{
    GetMenuStack()->PopUpDialog(ArgName(hdr));
    return true;
}

bool UIIsSessionMaster(C2DAnimCmdHdr *hdr, float *dT)
{
    if (theNetwork.m_bNetwork && theNetwork.m_bSessionMaster)
        return true;
    return false;
}

bool UIIsNetwork(C2DAnimCmdHdr *hdr, float *dT)
{
    return theNetwork.m_bNetwork;
}

/* the lobby's events, run as event anims by whichever of the two states with them is on top
 * (SOCOM 1's: then only the menu state's; InitNetwork hands it to the lobby) */
bool UILobbyEventHandler(const char *eventname, const char *argument)
{
    if (theGame.CurrentState() == (CGameState *)&theGame.m_menuState)
        return theGame.m_menuState.RunEventAnim(eventname, argument);
    if (theGame.CurrentState() == (CGameState *)theGame.m_coreState)
        return func_001EB070(theGame.m_coreState, eventname, argument);
    return false;
}

bool UIUnInitNetwork(C2DAnimCmdHdr *hdr, float *dT)
{
    theLobby.m_event_handler = 0;
    func_002DFDD0(&theLobby);
    func_0030D770(&theNetwork);
    func_0030E3A0(&theNetwork, 0);
    return true;
}

bool UIHostLANGame(C2DAnimCmdHdr *hdr, float *dT)
{
    bool ok = false;

    theMission.CreateNetGame();
    if (func_003045B0(&data_0044FE10)) {
        if (DMEConnectToGameServer()) {
            func_002DF8E0(&theLobby);
            ok = true;
        } else {
            func_002A9EF0(&theMission);
        }
    }
    return ok;
}

bool UIConnectToGameServer(C2DAnimCmdHdr *hdr, float *dT)
{
    bool ok;

    theMission.CreateNetGame();
    if (func_002B85C0()) {
        if (func_003045B0(&data_0044FE10))
            func_002DF8E0(&theLobby);
        ok = true;
    } else {
        func_002E3730(&theLobby);
        func_002A9EF0(&theMission);
        ok = false;
    }
    return ok;
}

bool UIDisconnectFromGameServer(C2DAnimCmdHdr *hdr, float *dT)
{
    if (!func_003045B0(&data_0044FE10) && theNetwork.m_bSessionMaster
        && theNetwork.zNetGetValidGameClientCount() == 1)
        func_002F80C0(&theLobby);
    ftsNetworkGameUninit();
    DMEDisconnectFromGameServer();
    func_002E3730(&theLobby);
    func_002A9EF0(&theMission);
    if (func_003045B0(&data_0044FE10))
        func_002DF6A0(&theLobby);
    return true;
}

bool UIJoinGameServer(C2DAnimCmdHdr *hdr, float *dT)
{
    if (!theMission.m_pNetGame->m_late_joiner) {
        if (!func_002B83E0())
            return false;
        if (!theNetwork.m_bSessionMaster && !data_00441470)
            return false;
        ftsNetworkGameInit();
    }
    if (theNetwork.m_bSessionMaster)
        CZGameState::UpdateLocalGame(theMission.m_pNetGame);
    return true;
}

/* CheckJoinMP's last counts: function-local statics in the original (SOCOM 1's had one, its guard
 * beside it), held to their addresses by their placeholders; -1 in data */
extern "C" int data_003DD490;
extern "C" int data_003DD498;

bool UICheckJoinMP(C2DAnimCmdHdr *hdr, float *dT)
{
    int clients = theNetwork.zNetGetValidGameClientCount();

    if (clients != data_003DD490)
        data_003DD490 = clients;
    NetSessionInfo info;

    func_0030CDD0(&theNetwork, &info);
    if (info.m_count != data_003DD498)
        data_003DD498 = info.m_count;
    if (data_003DD490 == data_003DD498 && data_003DD490 == 1) {
        SetValveValue(theMission.m_pNetGame->m_pJoinGameValve, 10);
        theMission.m_pNetGame->m_joined = 1;
    }
    return true;
}

/* the voice chat's button: 1 on (unless already), 0 off -- not for a late joiner */
bool UIVCButton(C2DAnimCmdHdr *hdr, float *dT)
{
    if (theMission.m_pNetGame && !theMission.m_pNetGame->m_late_joiner) {
        const char *text = ArgText(ArgName(hdr));

        if (*text == '1' && !data_0045B718->m_4a)
            func_0030E6E0(data_0045B718, 1);
        else if (*text == '0')
            func_0030E6E0(data_0045B718, 0);
    }
    return true;
}

bool UIIsMemCardInserted(C2DAnimCmdHdr *hdr, float *dT)
{
    return theMemCard.IsInserted();
}

bool UIIsMemCardFormatted(C2DAnimCmdHdr *hdr, float *dT)
{
    return theMemCard.IsFormatted();
}

bool UIIsMemcardSaveOK(C2DAnimCmdHdr *hdr, float *dT)
{
    int error = func_003A1CF0(&theMemCard);

    SetValveValue(CValve::Create("memcard_error_code", (VALVE_TYPE)2), error);
    return error == 0;
}

/* whether the slot the argument numbers holds a real saved game (not the dummies SaveConfig
 * writes) */
bool UIDoesSlotHaveGame(C2DAnimCmdHdr *hdr, float *dT)
{
    int slot = atoi(ArgText(ArgName(hdr)));
    bool has = false;
    MemCardFileInfo info;

    sprintf(s_savename, "SaveGame%d", slot);
    if (func_003A0950(&theMemCard, s_savename)) {
        sprintf(s_savename, "SaveGame%d", slot);
        func_0033D4A0(theMemCard.m_18, s_savename, 12, &info);
        if (info.m_text[0] == 'm' && strcmp(info.m_text, "This game is a dummy game."))
            has = true;
    }
    return has;
}

/* the real saved games counted into the valve the argument names */
bool UIGetNumSavedGames(C2DAnimCmdHdr *hdr, float *dT)
{
    CValve *valve = CValve::Create(ArgName(hdr), (VALVE_TYPE)2);
    int count = 0;
    MemCardFileInfo info;

    for (int i = 0; i < 10; i++) {
        sprintf(s_savename, "SaveGame%d", i);
        if (func_003A0950(&theMemCard, s_savename)) {
            sprintf(s_savename, "SaveGame%d", i);
            func_0033D4A0(theMemCard.m_18, s_savename, 12, &info);
            if (info.m_text[0] == 'm' && strcmp(info.m_text, "This game is a dummy game."))
                count++;
        }
    }
    SetValveValue(valve, count);
    return true;
}

bool UIFormatMemCard(C2DAnimCmdHdr *hdr, float *dT)
{
    return theMemCard.Format();
}

bool UILobbyRegister(C2DAnimCmdHdr *hdr, float *dT)
{
    CUIVariable *name = func_003857C0(&theUIVarManager, "PLAYERNAMEVAR", 0, UIVAR_SESSION);
    CUIVariable *password = func_003857C0(&theUIVarManager, "PLAYERPASSWORD", 0, UIVAR_SESSION);
    const char *n = name->String();
    const char *p = password->String();

    if (n && *n && p && *p)
        return func_002FA320(&theLobby, n, p);
    return false;
}

bool UILobbyLogin(C2DAnimCmdHdr *hdr, float *dT)
{
    CUIVariable *name = func_003857C0(&theUIVarManager, "PLAYERNAMEVAR", 0, UIVAR_SESSION);
    CUIVariable *password = func_003857C0(&theUIVarManager, "PLAYERPASSWORD", 0, UIVAR_SESSION);
    const char *n = name->String();
    const char *p = password->String();

    if (n && *n && p && *p)
        return func_002FA5A0(&theLobby, n, p);
    return false;
}

bool UILobbyLogout(C2DAnimCmdHdr *hdr, float *dT)
{
    bool done = false;

    if (theLobby.m_current_state == 0)
        done = true;
    else if (theLobby.m_current_state != 18)
        func_002FA560(&theLobby);
    return done;
}

bool UILobbyBailout(C2DAnimCmdHdr *hdr, float *dT)
{
    func_002F4240(&theLobby);
    return true;
}

bool UIGetAcctInfo(C2DAnimCmdHdr *hdr, float *dT)
{
    return false;
}

bool UIRefreshGameList(C2DAnimCmdHdr *hdr, float *dT)
{
    if (func_003045B0(&data_0044FE10))
        return func_003042B0(&data_0044FE10, GetCommandC2D(hdr->node_ref_index));
    return func_002E28D0(&theLobby, GetCommandC2D(hdr->node_ref_index));
}

bool UIRefreshRoomList(C2DAnimCmdHdr *hdr, float *dT)
{
    return func_002E3A30(&theLobby, GetCommandC2D(hdr->node_ref_index));
}

bool UIInitializeOnlineArmory(C2DAnimCmdHdr *hdr, float *dT)
{
    if (data_0045B718) {
        func_0030E6E0(data_0045B718, 0);
        func_0030E490(data_0045B718);
    }
    return true;
}

bool UIRefreshGamePlayerList(C2DAnimCmdHdr *hdr, float *dT)
{
    return func_002F6EC0(&theLobby, GetCommandC2D(hdr->node_ref_index));
}

bool UICreateMPGame(C2DAnimCmdHdr *hdr, float *dT)
{
    if (func_003045B0(&data_0044FE10)) {
        func_00303F40(&data_0044FE10);
        return true;
    }
    return func_002F9410(&theLobby);
}

bool UIJoinMPGame(C2DAnimCmdHdr *hdr, float *dT)
{
    const char *name = ArgName(hdr);
    bool ok;

    name = !strcmp(name, "INPUT_STRING") ? ZAnim.m_CurAnim->m_arg_list->GetCharP(0) : name;
    if (func_003045B0(&data_0044FE10))
        ok = func_003039B0(&data_0044FE10, name);
    else
        ok = func_002F83B0(&theLobby, name);
    return ok;
}

/* a clan game's sides swapped: START (when the lobby's game is in state 4) locks the game and
 * reports the status, FINISH ends the swap */
bool UIClanGameSwapSides(C2DAnimCmdHdr *hdr, float *dT)
{
    bool done = true;
    const char *text = ArgText(ArgName(hdr));

    if (text) {
        if (!strcasecmp(text, "START")) {
            if (func_002E99E0() == 4) {
                SetValveValue(CValve::Create("mp_locked_game", (VALVE_TYPE)1), 1);
                func_002F8350(&theLobby);
                done = true;
            } else {
                done = false;
            }
        } else if (!strcasecmp(text, "FINISH")) {
            func_002BADA0();
            done = true;
        }
    }
    return done;
}

bool UICanJoinRoom(C2DAnimCmdHdr *hdr, float *dT)
{
    bool ok = false;
    const char *text = ArgText(ArgName(hdr));

    if (text)
        ok = func_002E34E0(&theLobby, text);
    return ok;
}

bool UIClanGameClosed(C2DAnimCmdHdr *hdr, float *dT)
{
    return func_002F8C60(&theLobby, ArgText(ArgName(hdr)));
}

bool UIGameRejectsRank(C2DAnimCmdHdr *hdr, float *dT)
{
    return func_002F8BC0(&theLobby, ArgText(ArgName(hdr)));
}

bool UIGameHasPassword(C2DAnimCmdHdr *hdr, float *dT)
{
    const char *game = ArgText(ArgName(hdr));

    if (func_003045B0(&data_0044FE10))
        return func_00303730(&data_0044FE10, game);
    return func_002F8DD0(&theLobby, game);
}

bool UIGameHasSpectatorPassword(C2DAnimCmdHdr *hdr, float *dT)
{
    return func_002F8D50(&theLobby, ArgText(ArgName(hdr)));
}

bool UIJoinMPRoom(C2DAnimCmdHdr *hdr, float *dT)
{
    const char *name = ArgName(hdr);
    const char *room;

    if (!strcmp(name, "INPUT_STRING"))
        room = ZAnim.m_CurAnim->m_arg_list->GetCharP(0);
    else if (!strcmp(name, "RANDOM"))
        room = 0;
    else
        room = name;
    return func_002E3790(&theLobby, room);
}

/* SendMediusChat's text, sent to the lobby (SOCOM II's; the name ours) */
static void SendMediusChatCallback(const char *text, const char *to)
{
    func_002F4860(&theLobby, text, to);
}

bool UISendMediusChat(C2DAnimCmdHdr *hdr, float *dT)
{
    func_00281D40(hdr, SendMediusChatCallback, 1);
    return true;
}

bool UIGetMediusAnnouncements(C2DAnimCmdHdr *hdr, float *dT)
{
    func_002F7710(&theLobby);
    return true;
}

bool UIKillPopUp(C2DAnimCmdHdr *hdr, float *dT)
{
    GetMenuStack()->KillPopUp();
    return true;
}

bool UIIsTerrorist(C2DAnimCmdHdr *hdr, float *dT)
{
    return !func_002C31A0();
}

unsigned int g_can_use_switch;

bool UISwitchSides(C2DAnimCmdHdr *hdr, float *dT)
{
    if (func_0030D540(&theNetwork) > g_can_use_switch) {
        NetSendRequestTeamSwitch(theMission.m_pNetGame->m_persona_object_index);
        g_can_use_switch = func_0030D540(&theNetwork) + 5000;
    }
    return true;
}

bool UIForceMPLaunch(C2DAnimCmdHdr *hdr, float *dT)
{
    CZNetGame *game = theMission.m_pNetGame;

    if (!game->m_ten_second_launch) {
        game->TenSecondLaunch(true);
        CZGameState::UpdateLocalGame(game);
    } else if (game->m_ten_second_launch) {
        unsigned int launch = game->m_launch_time;
        unsigned int now = theNetwork.zNetGetTime();

        if (now < launch - 5000) {
            game->TenSecondLaunch(false);
            CZGameState::UpdateLocalGame(game);
        }
    }
    return true;
}

/* SendChatText's text: to one player through the lobby, or to the game (SOCOM II's; the name ours) */
static void SendChatTextCallback(const char *text, const char *to)
{
    if (to)
        func_002F4860(&theLobby, text, to);
    else
        SendChatMsg(text);
}

bool UISendChatText(C2DAnimCmdHdr *hdr, float *dT)
{
    func_00281D40(hdr, SendChatTextCallback, 0);
    return true;
}

unsigned int g_can_use_ready;

bool UIReadyToLoad(C2DAnimCmdHdr *hdr, float *dT)
{
    if (func_0030D540(&theNetwork) > g_can_use_ready) {
        if (theMission.m_pNetGame->m_late_joiner) {
            theMission.m_pNetGame->MyMajorGameState(CZNetGame::MP_MAJOR_LOADING);
            theMission.m_pNetGame->MyMinorGameState(CZNetGame::MP_MINOR_NOTREADY);
        } else {
            CZPersonaState::ToggleReady();
        }
        g_can_use_ready = func_0030D540(&theNetwork) + 5000;
    }
    return true;
}

bool UIFlushMenuHistory(C2DAnimCmdHdr *hdr, float *dT)
{
    GetMenuStack()->FlushHistory(false);
    return true;
}

bool UIClearGameState(C2DAnimCmdHdr *hdr, float *dT)
{
    return true;
}

bool UIToggleAutoMessages(C2DAnimCmdHdr *hdr, float *dT)
{
    CUIVariable *var = func_003857C0(&theUIVarManager, "AUTOMESSAGES", 0, UIVAR_SESSION);

    if (func_005E7FC0()) {
        func_003862A0(var, "OFF");
        func_005E7FB0(0);
    } else {
        func_003862A0(var, "ON");
        func_005E7FB0(1);
    }
    return true;
}

bool UIActivateButton(C2DAnimCmdHdr *hdr, float *dT)
{
    const char *button = ArgText(ArgName(hdr));

    GetMenuStack()->Push(6, button);
    return true;
}

bool UIEnableButton(C2DAnimCmdHdr *hdr, float *dT)
{
    const char *button = ArgText(ArgName(hdr));

    GetMenuStack()->Push(7, button);
    return true;
}

bool UIDisableButton(C2DAnimCmdHdr *hdr, float *dT)
{
    const char *button = ArgText(ArgName(hdr));

    GetMenuStack()->Push(8, button);
    return true;
}

bool UICheckVideoMode(C2DAnimCmdHdr *hdr, float *dT)
{
    const char *mode = ArgText(ArgName(hdr));

    if (mode && *mode) {
        if (!strcasecmp(mode, "NTSC"))
            return zVid.vidMode == 0;
        if (!strcasecmp(mode, "DTV"))
            return zVid.vidMode == 2;
    }
    return true;
}

bool UISwitchProgressiveVideo(C2DAnimCmdHdr *hdr, float *dT)
{
    const char *mode = ArgText(ArgName(hdr));

    if (mode && *mode) {
        if (!strcasecmp(mode, "NTSC"))
            zVid.videoMode = 0;
        else if (!strcasecmp(mode, "DTV"))
            zVid.videoMode = 2;
    }
    return true;
}

bool UISetScreenOrigin(C2DAnimCmdHdr *hdr, float *dT)
{
    int y = CValve::Create("ScreenOriginY", (VALVE_TYPE)1)->m_value;
    int x = CValve::Create("ScreenOriginX", (VALVE_TYPE)1)->m_value;

    if (zVid.videoMode == 2)
        y += 18;
    zVid.pcrtcOffsetX = x - 128;
    zVid.pcrtcOffsetY = y - 128;
    return true;
}

bool UIIsSpaceForSaveGameFile(C2DAnimCmdHdr *hdr, float *dT)
{
    int space = func_003A1210(&theMemCard);

    if (space <= 3000)
        space += func_003A1060(&theMemCard, 0);
    return space >= 3000;
}

bool UIMC_Changed(C2DAnimCmdHdr *hdr, float *dT)
{
    int slot;
    bool changed = func_003A1370(&theMemCard, 1, &slot);

    SetValveValue(CValve::Create("mc_changed_slot", (VALVE_TYPE)1), slot);
    return changed;
}

bool UIMC_ChDir(C2DAnimCmdHdr *hdr, float *dT)
{
    return func_003A07D0(&theMemCard, ArgName(hdr));
}

bool UIMC_MkDir(C2DAnimCmdHdr *hdr, float *dT)
{
    return func_003A0890(&theMemCard, ArgName(hdr));
}

bool UIMC_ChPort(C2DAnimCmdHdr *hdr, float *dT)
{
    int port = atoi(ArgName(hdr));

    if (port >= 0)
        theMemCard.m_port = port;
    theMemCard.SelectPortSlot();
    return true;
}

bool UIMC_ChSlot(C2DAnimCmdHdr *hdr, float *dT)
{
    int slot = atoi(ArgName(hdr));

    if (slot >= 0)
        theMemCard.m_slot = slot;
    theMemCard.SelectPortSlot();
    return true;
}

bool UIMC_Exists(C2DAnimCmdHdr *hdr, float *dT)
{
    return func_003A0950(&theMemCard, ArgName(hdr));
}

bool UIMC_CreateIconFiles(C2DAnimCmdHdr *hdr, float *dT)
{
    theMemCard.CreateIconFiles();
    return true;
}

bool UILoadConfig(C2DAnimCmdHdr *hdr, float *dT)
{
    func_003A1730(&theMemCard, ArgName(hdr));
    CUIVariable *var = func_003857C0(&theUIVarManager, "AUTOMESSAGES", 0, UIVAR_SESSION);

    if (var->type == 3) {
        if (!strcasecmp(var->String(), "ON"))
            func_005E7FB0(0);
        else
            func_005E7FB0(1);
    }
    return true;
}

bool UISaveConfig(C2DAnimCmdHdr *hdr, float *dT)
{
    const char *name = ArgName(hdr);

    func_003A1C60(&theMemCard);
    func_003A17E0(&theMemCard, name, 0);
    for (int i = 0; i < 7; i++) {
        sprintf(s_savename, "SaveGame%d", i);
        if (!func_003A0950(&theMemCard, s_savename)) {
            sprintf(s_savename, "SaveGame%d", i);
            func_003A1EA0(&theMemCard, s_savename, "This game is a dummy game.");
        }
    }
    func_003A1880(&theMemCard);
    return true;
}

bool UIMediusEnabled(C2DAnimCmdHdr *hdr, float *dT)
{
    return theNetwork.m_bMediusEnabled;
}

/* PlayMPEG's parser: the film by ARGUMENT or FILE (or its INDEX), whether it fades, plays in the
 * game and interlaced, the audio stream (a number, a valve or a UI variable), the rectangles and
 * the filtering (SOCOM II's; the name ours) */
_zanim_cmd_hdr *UIPlayMPEGParse(const char *name, _zrdr *rdr)
{
    char *var;
    char buf[800];
    UIPlayMPEGCmd *cmd = (UIPlayMPEGCmd *)_calloc(1, sizeof(UIPlayMPEGCmd), 0, 0);
    const char *s;
    _zrdr *stream;

    cmd->header.timeless = 1;
    cmd->header.data_size = sizeof(UIPlayMPEGCmd);
    s = zrdr_findstring(rdr, "ARGUMENT");
    if (!s)
        s = zrdr_findstring(rdr, "FILE");
    cmd->m_by_index = 0;
    if (!s) {
        s = zrdr_findstring(rdr, "INDEX");
        if (s)
            cmd->m_by_index = 1;
    }
    if (s && *s) {
        strcpy(buf, s);
        cmd->SetArgument(buf);
    }
    cmd->m_fade = true;
    func_0032DC00(rdr, "FADE", &cmd->m_fade);
    cmd->m_ingame = true;
    func_0032DC00(rdr, "INGAME", &cmd->m_ingame);
    cmd->m_interlace = true;
    func_0032DC00(rdr, "INTERLACE", &cmd->m_interlace);
    cmd->m_stream_type = 0;
    cmd->m_stream = 0;
    stream = func_0032F0D0(rdr, "AUDIOSTREAM");
    if (stream) {
        if (stream->type == 1) {
            cmd->m_stream_type = 0;
            cmd->m_stream = stream->Int();
        } else {
            var = 0;
            if (func_0032DF80(stream, "VALVE", &var)) {
                cmd->m_stream_type = 1;
                cmd->m_stream = ZAnim.m_CurAnim->AddName(var);
            } else if (func_0032DF80(stream, "UIVAR", &var)) {
                cmd->m_stream_type = 2;
                cmd->m_stream = ZAnim.m_CurAnim->AddName(var);
            }
        }
    }
    cmd->m_has_dest = func_0032DC40(rdr, "DEST_RECT", cmd->m_dest);
    func_0032DC40(rdr, "SOURCE_RECT", cmd->m_source);
    cmd->m_bilinear = false;
    func_0032DC00(rdr, "BILINEAR", &cmd->m_bilinear);
    return (_zanim_cmd_hdr *)cmd;
}

/* PlayMPEGNoFade's toggle: a function-local static in the original (SOCOM 1's UIPlayMPEG has one,
 * alt), held to its address by its placeholder -- scdiff leaves a local static's object unchecked */
extern "C" bool data_00414EA8;

bool UIPlayMPEGNoFade(C2DAnimCmdHdr *hdr, float *dT)
{
    CZSealBody *player = ftsGetPlayer();

    if (player && !player->m_e1_4)
        return true;
    bool was = data_00414EA8;

    data_00414EA8 = !was;
    if (!was) {
        func_001EDA60(theGame.m_cinematicSubState, ArgName(hdr), 0, 0);
        func_002CF380(&theGame, theGame.m_cinematicSubState, 0);
    }
    return was;
}

bool UIGetClan(C2DAnimCmdHdr *hdr, float *dT)
{
    return theLobby.GetClan();
}

bool UIClearCareerStats(C2DAnimCmdHdr *hdr, float *dT)
{
    func_002A3E20(data_00437E30);
    return true;
}

bool UISetCareerStatsUIVars(C2DAnimCmdHdr *hdr, float *dT)
{
    func_002A3BC0(data_00437E30);
    return true;
}

bool UICreateClan(C2DAnimCmdHdr *hdr, float *dT)
{
    CZAnim *anim = ZAnim.m_CurAnim;
    CUIVariable *var = func_003857C0(&theUIVarManager, "CLANNAMEVAR", 0, UIVAR_SESSION);

    if (var && var->type == 3) {
        func_002E7A10(&theLobby, var->String());
        return true;
    }
    func_002E7A10(&theLobby, anim->GetName(hdr->m_argument_index));
    return true;
}

bool UIDisbandClan(C2DAnimCmdHdr *hdr, float *dT)
{
    func_002E6850(&theLobby);
    return true;
}

bool UIIsMemberOfClan(C2DAnimCmdHdr *hdr, float *dT)
{
    return func_002E6830(&theLobby);
}

bool UIIsLeaderOfClan(C2DAnimCmdHdr *hdr, float *dT)
{
    return func_002E67D0(&theLobby, ArgText(ArgName(hdr)));
}

bool UIRemovePlayerFromClan(C2DAnimCmdHdr *hdr, float *dT)
{
    theLobby.RemovePlayerFromClan(ZAnim.m_CurAnim->GetArgCharP(0));
    return true;
}

bool UICanInviteMorePlayersToClan(C2DAnimCmdHdr *hdr, float *dT)
{
    return func_002E5FE0(&theLobby);
}

bool UIInvitePlayerToClan(C2DAnimCmdHdr *hdr, float *dT)
{
    CUIVariable *var = func_003857C0(&theUIVarManager, "PLAYER_INVITE_NAME", 0, UIVAR_SESSION);

    if (var->type == 3) {
        CSaferStr name;

        if (var->type == 3)
            name = var->String();
        bool ok = func_002E7600(&theLobby, name.c_str());

        return ok;
    }
    return true;
}

bool UIInvitePlayerToClanWithArg(C2DAnimCmdHdr *hdr, float *dT)
{
    return func_002E7600(&theLobby, ArgText(ArgName(hdr)));
}

bool UIVagStreaming(C2DAnimCmdHdr *hdr, float *dT)
{
    const char *text = ArgText(ArgName(hdr));

    if (!strcmp(text, "ON")) {
        data_0048DDA0 = 1;
        CSnd::UIOpen();
    } else if (!strcmp(text, "OFF")) {
        CSnd::Close();
        data_0048DDA0 = 0;
    }
    return true;
}

/* the menu's background film: ON the default, OFF none, else the one named */
bool UIBackgroundMPEG(C2DAnimCmdHdr *hdr, float *dT)
{
    const char *text = ArgText(ArgName(hdr));

    if (!strcmp(text, "ON"))
        func_001F3BD0(&theGame.m_menuState, 0);
    else if (!strcmp(text, "OFF"))
        func_001F3BE0(&theGame.m_menuState);
    else
        func_001F3BD0(&theGame.m_menuState, text);
    return true;
}

bool UIAutoAim(C2DAnimCmdHdr *hdr, float *dT)
{
    const char *text = ArgText(ArgName(hdr));

    if (!strcmp(text, "ON"))
        data_003DF1A0 = 1;
    else if (!strcmp(text, "OFF"))
        data_003DF1A0 = 0;
    return true;
}

bool UIGetInvitedPlayers(C2DAnimCmdHdr *hdr, float *dT)
{
    func_002E6EA0(&theLobby);
    return true;
}

bool UICheckClanInvites(C2DAnimCmdHdr *hdr, float *dT)
{
    func_002E8650(&theLobby);
    return true;
}

bool UIGetClanRoster(C2DAnimCmdHdr *hdr, float *dT)
{
    func_002E69F0(&theLobby);
    return true;
}

bool UIRevokeInvite(C2DAnimCmdHdr *hdr, float *dT)
{
    return func_002E72A0(&theLobby, ArgText(ArgName(hdr)));
}

bool UIGetFriendsList(C2DAnimCmdHdr *hdr, float *dT)
{
    func_00301750(&theLobby);
    return true;
}

bool UIGetFriendInvitations(C2DAnimCmdHdr *hdr, float *dT)
{
    func_003012F0(&theLobby);
    return true;
}

bool UIGetIgnoreList(C2DAnimCmdHdr *hdr, float *dT)
{
    func_00300F00(&theLobby);
    return true;
}

bool UIGetClanRanking(C2DAnimCmdHdr *hdr, float *dT)
{
    func_002EAB80(&theLobby, 0);
    return true;
}

bool UIDeleteClanMessage(C2DAnimCmdHdr *hdr, float *dT)
{
    func_002E6010(&theLobby, atoi(ArgText(ArgName(hdr))));
    return true;
}

bool UIModifyClanMessage(C2DAnimCmdHdr *hdr, float *dT)
{
    int index = atoi(ArgText(ArgName(hdr)));
    CUIVariable *var = func_003857C0(&theUIVarManager, "SEND_CLAN_MESSAGE", 0, UIVAR_SESSION);

    if (var && var->type == 3)
        func_002E60C0(&theLobby, index, var->String());
    return true;
}

bool UIGetLeaderMessages(C2DAnimCmdHdr *hdr, float *dT)
{
    func_002E6310(&theLobby);
    return true;
}

bool UIAddFriend(C2DAnimCmdHdr *hdr, float *dT)
{
    CUIVariable *var = func_003857C0(&theUIVarManager, "PLAYER_FRIEND_NAME", 0, UIVAR_SESSION);

    func_003013C0(&theLobby, var->String());
    return true;
}

bool UIAddFriendDirectly(C2DAnimCmdHdr *hdr, float *dT)
{
    const char *text = ArgText(ArgName(hdr));

    if (text)
        func_00300B70(&theLobby, atoi(text));
    return true;
}

bool UIAddToIgnore(C2DAnimCmdHdr *hdr, float *dT)
{
    CUIVariable *var = func_003857C0(&theUIVarManager, "PLAYER_IGNORE_NAME", 0, UIVAR_SESSION);

    func_00300C50(&theLobby, var->String());
    return true;
}

bool UIConfirmFriend(C2DAnimCmdHdr *hdr, float *dT)
{
    const char *text = ArgText(ArgName(hdr));

    if (text)
        func_00300A70(&theLobby, atoi(text));
    return true;
}

bool UIRejectFriend(C2DAnimCmdHdr *hdr, float *dT)
{
    const char *text = ArgText(ArgName(hdr));

    if (text)
        func_00300AF0(&theLobby, atoi(text));
    return true;
}

bool UIGetGameInfo(C2DAnimCmdHdr *hdr, float *dT)
{
    const char *game = ArgText(ArgName(hdr));

    if (game) {
        if (!strcasecmp(game, "CurrentGame"))
            func_002E24D0(&theLobby);
        else
            func_002E1F20(&theLobby, game);
    }
    return true;
}

/* GetPlayerInfo's parser: the argument and what to ask for (SOCOM II's; the name ours) */
_zanim_cmd_hdr *UIGetPlayerInfoParse(const char *name, _zrdr *rdr)
{
    char buf[128];
    UIPlayerInfoCmd *cmd = (UIPlayerInfoCmd *)_calloc(1, sizeof(UIPlayerInfoCmd), 0, 0);
    const char *s;

    cmd->header.timeless = 1;
    cmd->header.data_size = sizeof(UIPlayerInfoCmd);
    s = zrdr_findstring(rdr, "ARGUMENT");
    if (s && *s) {
        strcpy(buf, s);
        cmd->SetArgument(buf);
    }
    if (func_0032F0D0(rdr, "BRIEFING"))
        cmd->m_what = 0;
    else if (func_0032F0D0(rdr, "CLAN"))
        cmd->m_what = 1;
    else if (func_0032F0D0(rdr, "FRIEND"))
        cmd->m_what = 2;
    else if (func_0032F0D0(rdr, "LADDER"))
        cmd->m_what = 3;
    else
        cmd->m_what = 0;
    return (_zanim_cmd_hdr *)cmd;
}

bool UIGetPlayerInfo(C2DAnimCmdHdr *hdr, float *dT)
{
    const char *name = ArgText(ArgName(hdr));

    if (name)
        func_002EFE50(&theLobby, name, ((UIPlayerInfoCmd *)hdr)->m_what);
    return true;
}

bool UIGetFriendInfo(C2DAnimCmdHdr *hdr, float *dT)
{
    const char *text = ArgText(ArgName(hdr));

    if (text)
        func_00301140(&theLobby, atoi(text));
    return true;
}

bool UIGetClanMemberInfo(C2DAnimCmdHdr *hdr, float *dT)
{
    const char *text = ArgText(ArgName(hdr));

    if (text)
        func_002E6000(&theLobby, text);
    return true;
}

bool UIRemoveFriend(C2DAnimCmdHdr *hdr, float *dT)
{
    const char *text = ArgText(ArgName(hdr));

    if (text)
        func_003016A0(&theLobby, atoi(text));
    return true;
}

bool UIRemoveFromIgnore(C2DAnimCmdHdr *hdr, float *dT)
{
    const char *text = ArgText(ArgName(hdr));

    if (text)
        func_00300FE0(&theLobby, atoi(text));
    return true;
}

bool UISendClanMessage(C2DAnimCmdHdr *hdr, float *dT)
{
    CUIVariable *var = func_003857C0(&theUIVarManager, "SEND_CLAN_MESSAGE", 0, UIVAR_SESSION);

    if (var->type == 3) {
        CSaferStr text;

        if (var->type == 3)
            text = var->String();
        bool ok = func_002E8310(&theLobby, text.c_str());

        return ok;
    }
    return true;
}

bool UIGetUsagePolicy(C2DAnimCmdHdr *hdr, float *dT)
{
    func_002F79F0(&theLobby);
    return true;
}

bool UIGetClanMessages(C2DAnimCmdHdr *hdr, float *dT)
{
    func_002E8460(&theLobby);
    return true;
}

bool UIIsClanChallengeGame(C2DAnimCmdHdr *hdr, float *dT)
{
    return false;
}

/* the next mission of the rotation: the game's launch timed, and the session master picks the
 * mission (the lobby's choice, 254 the one after the current, 255 the first) and tells the game */
bool UIAdvanceMPMission(C2DAnimCmdHdr *hdr, float *dT)
{
    CZNetGame *game = theMission.m_pNetGame;

    if (!(game && theNetwork.m_bNetwork))
        return true;
    int wait = game->m_10->m_value ? 45000 : 30000;

    game->m_c8 = wait + theNetwork.zNetGetTime();
    if (theNetwork.m_bSessionMaster) {
        char name[16];
        char current[16];
        int index = func_002F8120(&theLobby);

        if (index == 254) {
            strncpy(current, database, 16);
            index = func_002C8DB0(data_00441650, atoi(current + 2), 1)->m_index;
        }
        if (index == 255)
            index = func_002C8840(data_00441650, 1)->m_index;
        if (index > 0) {
            sprintf(name, "mp%d", index);
            strcpy(database, name);
        }
        CZGameState::UpdateLocalGame(game);
    }
    func_002ACC10(&theMission, database);
    return true;
}

bool UISelectLocale(C2DAnimCmdHdr *hdr, float *dT)
{
    func_003A44B0(func_003A40E0(ArgText(ArgName(hdr))));
    return true;
}

bool UIHaveBeenBannedFromGame(C2DAnimCmdHdr *hdr, float *dT)
{
    return func_002C3550();
}

bool UIGameStatePopCommand(C2DAnimCmdHdr *hdr, float *dT)
{
    theGame.m_menuState.SetDefaultMenu(ArgText(ArgName(hdr)));
    func_002CF2F0(&theGame, 0);
    func_002CF410(&theGame, &theGame.m_menuState, 0);
    return true;
}

bool UISetDefaultMenuScreen(C2DAnimCmdHdr *hdr, float *dT)
{
    theGame.m_menuState.SetDefaultMenu(ArgText(ArgName(hdr)));
    return true;
}

bool UIBanSelfFromGame(C2DAnimCmdHdr *hdr, float *dT)
{
    func_002F95F0(&theLobby);
    return true;
}

/* the hard disk operation's last result (a byte; the name ours) */
extern "C" unsigned char data_00414EB0;

bool UIQuitClan(C2DAnimCmdHdr *hdr, float *dT)
{
    func_002E8EF0(&theLobby);
    return true;
}

/* GetLadderInfo's parser: which list and which move, the clan ladder's numbered after the player
 * ladder's (SOCOM II's; the name ours) */
_zanim_cmd_hdr *UIGetLadderInfoParse(const char *name, _zrdr *rdr)
{
    UILadderCmd *cmd = (UILadderCmd *)_calloc(1, sizeof(UILadderCmd), 0, 0);

    cmd->header.timeless = 1;
    cmd->header.data_size = sizeof(UILadderCmd);
    if (func_0032F0D0(rdr, "CLAN")) {
        if (func_0032F0D0(rdr, "Top10"))
            cmd->m_op = 7;
        else if (func_0032F0D0(rdr, "LadderPressUp"))
            cmd->m_op = 8;
        else if (func_0032F0D0(rdr, "LadderPressDown"))
            cmd->m_op = 9;
        else if (func_0032F0D0(rdr, "LadderUp"))
            cmd->m_op = 10;
        else if (func_0032F0D0(rdr, "LadderDown"))
            cmd->m_op = 11;
        else
            cmd->m_op = 6;
    } else if (func_0032F0D0(rdr, "Top10")) {
        cmd->m_op = 1;
    } else if (func_0032F0D0(rdr, "LadderPressUp")) {
        cmd->m_op = 2;
    } else if (func_0032F0D0(rdr, "LadderPressDown")) {
        cmd->m_op = 3;
    } else if (func_0032F0D0(rdr, "LadderUp")) {
        cmd->m_op = 4;
    } else if (func_0032F0D0(rdr, "LadderDown")) {
        cmd->m_op = 5;
    } else {
        cmd->m_op = 0;
    }
    return (_zanim_cmd_hdr *)cmd;
}

/* the hard disk operation's result, told to the menu through a valve and the lobby's event handler
 * (HardDriveOperation hands it on; SOCOM II's, the name ours) */
bool HardDriveOperationCallback(int result)
{
    int code;

    data_00414EB0 = result;
    switch (data_00414EB0) {
    case 1:
        code = 1;
        break;
    case 2:
        code = 2;
        break;
    case 3:
        code = 3;
        break;
    default:
        code = 5;
        break;
    }
    SetValveValue(CValve::Create("hard_disk_error_code", (VALVE_TYPE)1), code);
    if (theLobby.m_event_handler)
        theLobby.m_event_handler("HDD_ERROR", 0);
    return true;
}

bool UIAbortLoad(C2DAnimCmdHdr *hdr, float *dT)
{
    theGame.m_loadState.AbortLoad(ArgName(hdr));
    return false;
}

bool UINetCnfInit(C2DAnimCmdHdr *hdr, float *dT)
{
    func_0030C6E0();
    return true;
}

bool UINetCnfUninit(C2DAnimCmdHdr *hdr, float *dT)
{
    func_0030C6A0();
    return true;
}

bool UINetCnfClose(C2DAnimCmdHdr *hdr, float *dT)
{
    NetCnfRdr rdr;

    func_00333AE0(&rdr);
    func_003339B0(&rdr, 0);
    func_00333770(&rdr);
    func_003857C0(&theUIVarManager, "NETCNF_DB", (_zrdr *)&rdr, UIVAR_SESSION);
    strncpy(data_003DF360, "", 256);
    func_00333A20(&rdr, -1);
    return true;
}

/* the valve the argument names set to the index of the configuration in use, among the valid ones.
 * OPEN, 22 instructions: the same code with the loop's registers permuted (as SelectCombo's) */
bool UINetCnfCurrCombo(C2DAnimCmdHdr *hdr, float *dT)
{
    CValve *valve = CValve::Create(ArgText(ArgName(hdr)), (VALVE_TYPE)2);
    int count = data_00457C40.m_count;
    NetCnfEntry *cur = func_001E8890(&data_00457C40);
    int index = 0;

    for (int i = 0; i < count; i++) {
        NetCnfEntry *e = data_00457C40.Entry(i);

        if (e && e->m_type >= 0) {
            if (!strcmp(cur->m_name, e->m_name)) {
                SetValveValue(valve, index);
                break;
            }
            index++;
        }
    }
    return true;
}

/* the configuration the argument's valve indexes (among the valid ones) selected, when no memory
 * card has changed.
 * OPEN, 16 instructions, registers only: the target keeps the count in t0, the walking pointer in
 * t1, i in a2, the index in a3 and the entry in a1; here count a1, walk a2, i t0, index t1, entry
 * a3. Measured without result: i or the index declared outside the loop or the function's top,
 * the count in a local, the entry declared before the loop or assigned in the test, the accessor
 * called three times, the compare's operands swapped */
bool UINetCnfSelectCombo(C2DAnimCmdHdr *hdr, float *dT)
{
    bool ok = false;

    if (!func_003A1370(&theMemCard, 0, 0)) {
        CValve *valve = CValve::Create(ArgText(ArgName(hdr)), (VALVE_TYPE)2);

        if (valve) {
            int index = 0;

            for (int i = 0; i < data_00457C40.m_count; i++) {
                NetCnfEntry *e = data_00457C40.Entry(i);

                if (e && e->m_type >= 0 && index++ == valve->m_value) {
                    if (func_001E8910(data_003DF360, e->m_name, data_004567C0) >= 0)
                        ok = true;
                    break;
                }
            }
        }
    }
    return ok;
}

/* the network interfaces listed into the UI variable the argument names, "vendor/product" each.
 * OPEN, 10 instructions, all stack offsets: the target's frame is 48 bytes larger (0xED0), the
 * text at 0xE00 and the reader at 0xE80 with a hole between the list's end (0xDD0) and the text --
 * a local the original kept a slot for (an unused array is dropped here; a 176-byte text buffer
 * fixes the frame but not the text's offset; the list's 0x240 stride is fixed by the loop) */
bool UINetGetInterfaceList(C2DAnimCmdHdr *hdr, float *dT)
{
    NetCnfRdr rdr;
    char text[128];
    NetInterfaceInfo list[6];
    int n = func_0030BD40(list, 6);
    CUIVariable *var = func_003857C0(&theUIVarManager, ArgText(ArgName(hdr)), 0, UIVAR_SESSION);

    func_00333AE0(&rdr);
    func_003339B0(&rdr, 0);
    for (int i = 0; i < n; i++) {
        sprintf(text, "%s/%s", list[i].m_vendor, list[i].m_product);
        func_00333610(&rdr, text);
    }
    func_00333770(&rdr);
    func_00386460(var, &rdr);
    func_00333A20(&rdr, -1);
    return true;
}

bool UIClanHasInvitations(C2DAnimCmdHdr *hdr, float *dT)
{
    return func_002E7290(&theLobby);
}

bool UIHasClanInvitations(C2DAnimCmdHdr *hdr, float *dT)
{
    return func_002E8770(&theLobby);
}

bool UIStopAllSounds(C2DAnimCmdHdr *hdr, float *dT)
{
    func_00346C00();
    return true;
}

/* the IOP rebooted for what comes next (SOCOM 1's took the mode's name the same way) */
bool UIRebootIOP(C2DAnimCmdHdr *hdr, float *dT)
{
    int mode = 2;
    const char *text = ArgText(ArgName(hdr));

    if (!strcasecmp(text, "Singleplayer"))
        mode = 2;
    else if (!strcasecmp(text, "Multiplayer"))
        mode = 3;
    else if (!strcasecmp(text, "NetConfig"))
        mode = 4;
    else if (!strcasecmp(text, "MainMenu"))
        mode = 5;
    func_0034F9B0(&theIOPManager, mode);
    return true;
}

bool UIFadeAllSounds(C2DAnimCmdHdr *hdr, float *dT)
{
    CSndInstance::FadeOutAllSounds();
    return true;
}

void AuxNetCensorText(const char *text, void *var)
{
    if (var)
        func_003862A0((CUIVariable *)var, text);
}

bool UINetCensorText(C2DAnimCmdHdr *hdr, float *dT)
{
    if (theNetwork.m_bNetwork && !func_003045B0(&data_0044FE10)) {
        const char *name;
        CZAnim *anim = ZAnim.m_CurAnim;

        name = anim->GetName(hdr->m_argument_index);
        if (name && !strcasecmp("$1", name))
            name = anim->m_arg_list->GetCharP(0);
        CUIVariable *var = func_00385900(&theUIVarManager, name);

        if (var && var->type == 3)
            func_002F4DF0(&theLobby, var->String(), AuxNetCensorText, var);
    }
    return true;
}

bool UIForceMediusStatusReport(C2DAnimCmdHdr *hdr, float *dT)
{
    func_002F8350(&theLobby);
    return true;
}

bool UISetPlayerDescription(C2DAnimCmdHdr *hdr, float *dT)
{
    CUIVariable *var = func_003857C0(&theUIVarManager, "PlayerDescription", 0, UIVAR_SESSION);

    if (var && var->type == 3)
        func_002FA7F0(&theLobby, var->String());
    return true;
}

bool UIUpdateClanStats(C2DAnimCmdHdr *hdr, float *dT)
{
    func_002E7D70(&theLobby);
    return true;
}

bool UINotOnCorrectClanTeam(C2DAnimCmdHdr *hdr, float *dT)
{
    return !theLobby.OnCorrectClanTeam();
}

bool UIAcceptInvitation(C2DAnimCmdHdr *hdr, float *dT)
{
    func_002E89F0(&theLobby, atoi(ArgText(ArgName(hdr))));
    return true;
}

bool UIDeclineInvitation(C2DAnimCmdHdr *hdr, float *dT)
{
    func_002E8980(&theLobby, atoi(ArgText(ArgName(hdr))));
    return true;
}

bool UITransferLeadership(C2DAnimCmdHdr *hdr, float *dT)
{
    func_002E81B0(&theLobby, ArgText(ArgName(hdr)));
    return true;
}

bool UIIsSwitchTeamsOK(C2DAnimCmdHdr *hdr, float *dT)
{
    return func_002F8CE0(&theLobby);
}

bool UIPasswordSecurity(C2DAnimCmdHdr *hdr, float *dT)
{
    CZOnlineLobby::PasswordSecurity();
    return true;
}

bool UISysPostInit(C2DAnimCmdHdr *hdr, float *dT)
{
    bool reopen;

    if (CFileCD::m_streamingSndActive) {
        reopen = true;
        CSnd::Close();
    } else {
        reopen = false;
    }
    func_0034F950();
    if (reopen)
        CSnd::UIOpen();
    return true;
}

bool UISysLoadIRX(C2DAnimCmdHdr *hdr, float *dT)
{
    char name[256];

    strncpy(name, ArgText(ArgName(hdr)), 256);
    return true;
}

bool UICheckTeamStatus(C2DAnimCmdHdr *hdr, float *dT)
{
    bool ok = true;

    if (!theMission.m_pNetGame || !theNetwork.m_bSessionMaster && !data_00441470)
        ok = false;
    return ok;
}

bool UISuspendMenuInput(C2DAnimCmdHdr *hdr, float *dT)
{
    func_001F3BB0(&theGame.m_menuState, func_00191478(ArgText(ArgName(hdr))));
    return true;
}

/* waiting for the network: on failure the lobby is reset and the menu told (an error text for
 * MP_MEDIUS_ERROR), the valve 0; once it is up, the valve 1 -- then done either way */
void CSMInitNetwork::WaitForNetwork(unsigned char event)
{
    char text[256];

    switch (event) {
    case 0:
        if (!func_0030DEE0(&theNetwork)) {
            func_002EFE20(&theLobby, 2);
            func_002DFE50(&theLobby);
            sprintf(text, "Network Initialization Error: %s (%d)", "Cannot Initialize Network", 0);
            theGame.m_menuState.RunEventAnim("MP_MEDIUS_ERROR", text);
            SetValveValue(m_valve, 0);
            SetState(&CSimpleStateMachine::Finish);
        }
        break;
    case 1:
        if (func_0030BF20()) {
            SetValveValue(m_valve, 1);
            SetState(&CSimpleStateMachine::Finish);
        }
        break;
    case 2:
        break;
    }
}

/* the network's initialisation started, then waited for */
void CSMInitNetwork::Initialize(unsigned char event)
{
    switch (event) {
    case 0:
        func_0030E290(&theNetwork);
        SetState((SSMState)&CSMInitNetwork::WaitForNetwork);
        break;
    case 1:
        break;
    case 2:
        break;
    }
}

bool CSMInitNetwork::Start()
{
    CSimpleStateMachine::Start();
    SetState((SSMState)&CSMInitNetwork::Initialize);
    return true;
}

/* the machine InitNetwork runs (a function-local static in the original, likely: held to its
 * address by its placeholder) */
extern "C" CSMInitNetwork *data_00414EB8;

/* the network started: the lobby's events handed to the menu, the machine made and run until it
 * is done, the valve the argument names told how it went (SOCOM 1's built no machine).
 * OPEN, 0 instructions: every byte is right; the two calls name CSimpleStateMachine's constructor
 * and destructor, rows outside this lane (0x3A5DD0 __ct__19CSimpleStateMachineFv: the vtable
 * 0x408240 stored, the queue built, the idle state 0x3E07D8 copied in; 0x3A5CC0
 * __dt__19CSimpleStateMachineFv: the same vtable stored first, deleted when asked) -- matches once
 * the map names them (measured with both renamed) */
bool UIInitNetwork(C2DAnimCmdHdr *hdr, float *dT)
{
    bool done = false;

    if (!data_00414EB8) {
        CValve *valve = CValve::Create(ArgName(hdr), (VALVE_TYPE)2);

        theLobby.m_event_handler = UILobbyEventHandler;
        data_00414EB8 = new CSMInitNetwork(valve);
        data_00414EB8->Start();
    } else {
        func_003A5C30(data_00414EB8);
        if (func_003A5C10(data_00414EB8)) {
            delete data_00414EB8;
            data_00414EB8 = 0;
            done = true;
        }
    }
    return done;
}

bool UIIsControllerConnected(C2DAnimCmdHdr *hdr, float *dT)
{
    bool connected = false;
    int port = atoi(ArgText(ArgName(hdr)));
    CPad *pad = (unsigned int)port < CInput::NUM_PADS ? CInput::m_pads[port] : 0;

    if (pad)
        connected = pad->m_dc == 1;
    return connected;
}

bool UIDMEInit(C2DAnimCmdHdr *hdr, float *dT)
{
    CValve *valve = CValve::Create(ArgName(hdr), (VALVE_TYPE)2);

    func_002E05D0(&theLobby);
    if (!func_0030DBB0(&theNetwork)) {
        func_0030E3A0(&theNetwork, 1);
        func_002B8660();
        SetValveValue(valve, 1);
    } else {
        SetValveValue(valve, 0);
    }
    return true;
}

bool UIMediusInit(C2DAnimCmdHdr *hdr, float *dT)
{
    CValve *valve = CValve::Create(ArgName(hdr), (VALVE_TYPE)2);

    SetValveValue(valve, func_00300840());
    return true;
}

/* whether the persona the argument names exists on the lobby server (its host and port the UI
 * variables') */
bool UIMUISPersonaExists(C2DAnimCmdHdr *hdr, float *dT)
{
    const char *name = ArgText(ArgName(hdr));
    const char *host = func_003857C0(&theUIVarManager, "LOBBY_HOST", 0, UIVAR_PERMANENT)->String();
    CUIVariable *port = func_003857C0(&theUIVarManager, "LOBBY_PORT", 0, UIVAR_PERMANENT);

    return func_002FBBF0(data_0044FC60, host, port->Int(), name);
}

bool UIVerifyPatchLevel(C2DAnimCmdHdr *hdr, float *dT)
{
    const char *level = ArgName(hdr);

    level = !strcmp(level, "INPUT_STRING") ? ZAnim.m_CurAnim->m_arg_list->GetCharP(0) : level;
    bool ok = func_002FD6E0(data_0044FC60, level);

    if (!ok) {
        CUIVariable *var = func_003857C0(&theUIVarManager, "PatchServerURL", 0, UIVAR_SESSION);

        func_003862A0(var, func_002FD610(data_0044FC60, level));
    }
    return ok;
}

/* GetMissionInformation's parser: the mission set and the names the mission's fields go to
 * (SOCOM II's; the name ours) */
_zanim_cmd_hdr *UIGetMissionInformationParse(const char *name, _zrdr *rdr)
{
    UIMissionInfoCmd *cmd = (UIMissionInfoCmd *)_calloc(1, sizeof(UIMissionInfoCmd), 0, 0);

    cmd->header.timeless = 1;
    cmd->header.data_size = sizeof(UIMissionInfoCmd);
    if (func_0032F0D0(rdr, "MULTIPLAYER"))
        cmd->m_multiplayer = 1;
    else
        cmd->m_multiplayer = 0;
    cmd->m_name = ZAnim.m_CurAnim->AddName(zrdr_findstring(rdr, "NAME"));
    cmd->m_number = ZAnim.m_CurAnim->AddName(zrdr_findstring(rdr, "NUMBER"));
    cmd->m_index = ZAnim.m_CurAnim->AddName(zrdr_findstring(rdr, "INDEX"));
    return (_zanim_cmd_hdr *)cmd;
}

/* CharacterProfile's parser: what to do with the profile (SOCOM II's; the name ours) */
_zanim_cmd_hdr *UICharacterProfileParse(const char *name, _zrdr *rdr)
{
    UIProfileCmd *cmd = (UIProfileCmd *)_calloc(1, sizeof(UIProfileCmd), 0, 0);

    cmd->header.timeless = 1;
    cmd->header.data_size = sizeof(UIProfileCmd);
    if (func_0032F0D0(rdr, "EXISTS"))
        cmd->m_op = UIPROFILE_EXISTS;
    else if (func_0032F0D0(rdr, "LOAD"))
        cmd->m_op = UIPROFILE_LOAD;
    else if (func_0032F0D0(rdr, "SAVE"))
        cmd->m_op = UIPROFILE_SAVE;
    else if (func_0032F0D0(rdr, "SaveToServer"))
        cmd->m_op = UIPROFILE_SAVE_TO_SERVER;
    else if (func_0032F0D0(rdr, "WaitForServerSaveComplete"))
        cmd->m_op = UIPROFILE_WAIT_FOR_SERVER_SAVE;
    else if (func_0032F0D0(rdr, "LoadFromServer"))
        cmd->m_op = UIPROFILE_LOAD_FROM_SERVER;
    else if (func_0032F0D0(rdr, "WaitForLoadComplete"))
        cmd->m_op = UIPROFILE_WAIT_FOR_LOAD;
    else if (func_0032F0D0(rdr, "DELETE"))
        cmd->m_op = UIPROFILE_DELETE;
    else
        cmd->m_op = UIPROFILE_NONE;
    return (_zanim_cmd_hdr *)cmd;
}

/* GetMUISPersonaInfo's parser: the persona by INDEX or by NAME, the argument the one given (SOCOM
 * II's; the name ours) */
_zanim_cmd_hdr *UIPersonaParse(const char *name, _zrdr *rdr)
{
    UIPersonaCmd *cmd = (UIPersonaCmd *)_calloc(1, sizeof(UIPersonaCmd), 0, 0);
    const char *s;

    cmd->header.timeless = 1;
    cmd->header.data_size = sizeof(UIPersonaCmd);
    s = zrdr_findstring(rdr, "INDEX");
    if (s) {
        cmd->m_by = 1;
    } else {
        s = zrdr_findstring(rdr, "NAME");
        cmd->m_by = 2;
    }
    cmd->m_argument_index = ZAnim.m_CurAnim->AddName(s);
    return (_zanim_cmd_hdr *)cmd;
}

/* CompareValues's parser: the two values, each a name or a UI variable's, and how to compare them
 * (SOCOM II's; the name ours) */
_zanim_cmd_hdr *UICompareValuesParse(const char *name, _zrdr *rdr)
{
    UICompareValuesCmd *cmd = (UICompareValuesCmd *)_calloc(1, sizeof(UICompareValuesCmd), 0, 0);
    const char *s;

    cmd->header.timeless = 1;
    cmd->header.data_size = sizeof(UICompareValuesCmd);
    s = zrdr_findstring(rdr, "VALUE1");
    cmd->m_uivar1 = 0;
    if (!s) {
        s = zrdr_findstring(rdr, "UIVAR1");
        cmd->m_uivar1 = 1;
    }
    if (s)
        cmd->m_value1 = ZAnim.m_CurAnim->AddName(s);
    s = zrdr_findstring(rdr, "VALUE2");
    cmd->m_uivar2 = 0;
    if (!s) {
        s = zrdr_findstring(rdr, "UIVAR2");
        cmd->m_uivar2 = 1;
    }
    if (s)
        cmd->m_value2 = ZAnim.m_CurAnim->AddName(s);
    if (func_0032F0D0(rdr, "CASE_INSENSITIVE"))
        cmd->m_case_insensitive = 1;
    else
        cmd->m_case_insensitive = 0;
    if (func_0032F0D0(rdr, "COMPARE_AFTER_FORMATTING"))
        cmd->m_after_formatting = 1;
    else
        cmd->m_after_formatting = 0;
    return (_zanim_cmd_hdr *)cmd;
}

bool UILobbyDisconnect(C2DAnimCmdHdr *hdr, float *dT)
{
    if (func_003045B0(&data_0044FE10))
        func_003043E0(&data_0044FE10);
    else
        func_002DFDD0(&theLobby);
    return true;
}

extern CMenuStack g_MenuStack;

CMenuStack *GetMenuStack()
{
    return &g_MenuStack;
}

/* the command's type by its name: 0 for none, the count of types for no name at all */
int GetUICommandType(const char *name)
{
    if (!name)
        return 232;
    if (!*name)
        return 0;
    /* (a do-while: the table is never empty -- SOCOM 1's has the same shape) */
    int i = 0;

    do {
        if (!strcasecmp(UICommandTable[i].name, name))
            return UICommandTable[i].id;
    } while (UICommandTable[++i].id);
    return 0;
}

/* a UI command's handler run: the table's own record when the type indexes it, else the record
 * that carries the type; true when there is none (ours) */
inline bool RunUICommand(C2DAnimCmdHdr *hdr, float *dT)
{
    int i = 0;
    int type = hdr->m_command_type;

    if (!type) {
        type = GetUICommandType(ZAnim.m_CurAnim->GetName(hdr->m_command_index));
        hdr->m_command_type = type;
    }
    if (UICommandTable[type].id == type) {
        if (UICommandTable[type].func)
            return UICommandTable[type].func(hdr, dT);
    } else {
        do {
            if (UICommandTable[i].id == type && UICommandTable[i].func)
                return UICommandTable[i].func(hdr, dT);
        } while (UICommandTable[++i].id);
    }
    return true;
}

/* UI_COMMAND's tick: the command goes to its node's 2D object when the node has one, else to the
 * handler its name registers (the names looked up and left unused are SOCOM 1's too: debug output
 * compiled away).
 * OPEN, size 0x2AC of 0x2B4: every instruction is right but the saved registers. The target has
 * handled s2, node_ref s3, anim s4, the C2D s5 -- and the scan's index in s5 too, set to 0 before
 * the type's lookup (in the delay slot of `if (!type)`), so it lives across the lookup's strcasecmp
 * calls. With the lookup in the caller and the index local to RunUICommand's scan, the first half
 * matches exactly and the index is a2 (its loop placed inline, not after the epilogue with a
 * trampoline as the target's); with the lookup moved into the inline and the index declared first
 * (as here) the tail matches and the first half takes anim s2, handled s3, the C2D s4, node_ref s5.
 * Measured without result: the six declarations permuted (tools/perm.py, best 149 from 156), the
 * dispatch written out in the tick with i and type among them (best 188 from 195). Next: the index
 * as a parameter or member the inline's caller owns, or the type's lookup as its own inline */
bool zAnimUIMenuTick(_zanim_cmd_hdr *cmd, float *dT)
{
    C2DAnimCmdHdr *hdr = (C2DAnimCmdHdr *)cmd;
    bool handled = false;
    bool done = true;
    CZAnim *anim = ZAnim.m_CurAnim;

    anim->GetName(hdr->m_command_index);
    unsigned short node_ref = hdr->node_ref_index;

    if (node_ref) {
        C2D *c2d = GetCommandC2D(node_ref);

        if (c2d) {
            handled = c2d->TickZAnimCmd(cmd, dT, &done);
        } else {
            ZAnim.m_CurAnim->GetNode(hdr->node_ref_index, false);
            anim->GetName();
        }
    }
    if (!handled) {
        anim->GetName(hdr->m_argument_index);
        done = RunUICommand(hdr, dT);
    }
    return done;
}

bool zAnimUIAppMenuTick(_zanim_cmd_hdr *hdr, float *dT)
{
    return true;
}

namespace {

/* the selected characters one by one: a bit for each of the 32 in a mask, the weapons list of the
 * character the index is on (SOCOM 1's class; the members' names ours) */
class CSelectedCharsItr {
public:
    void Advance();
    /* (an accessor: the loop's test then reads the index apart from the body, as the target's
     * does -- written out, MW keeps the test's load for the shift) */
    bool Valid() const { return m_index < 32; }

    unsigned int m_mask;   // 0x0
    int m_index;   // 0x4
    void *m_weapons;   // 0x8
};

/* on to the next selected character that has a type (and so its weapons) */
void CSelectedCharsItr::Advance()
{
    for (m_index++; Valid(); m_index++) {
        if ((1 << m_index) & m_mask) {
            CCharacterType *type = func_0053E170(GetCustomCharTypeName(m_index));

            m_weapons = type ? type->m_weapons : 0;
            if (m_weapons)
                break;
        }
    }
}

}
