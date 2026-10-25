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
extern void *data_0045B718;
extern char data_00437E30[];               /* the career stats */
void func_0030E6E0(void *p, int arg);
void func_0030E490(void *p);
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

namespace {

bool UISetControllerConfig(C2DAnimCmdHdr *hdr, float *dT)
{
    func_002C5D60(ArgText(ArgName(hdr)));
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

/* OPEN, 2 instructions (0x1C0 of 0x1C8): the target inlines FlushHistory whole (with all true:
 * the pop-up's test without the flag's, then CleanPopup's), the history's flush and the vectors'
 * pieces with it. Here the chain is one level too tall at this depth: the base's end() inside back()
 * is called out of line (FlushHistory itself matches, one level up). Written out instead (the
 * entries loop, m_history.Flush(), the pop-up's delete, CleanPopup()) everything inlines and one
 * instruction is left: the history's member destruction reads m_free's data_ through its this,
 * lw a0,8(v0), where MW folds it into lw a0,0xE3C(s2) (m_used's, destroyed after, is folded in the
 * target too) -- not the history's base class (measured: m_used as CMenuHistory's base gives the same
 * fold). include/vector's pointer back() and push_back a level lower would likely settle the first
 * form */
CMenuStack::~CMenuStack()
{
    FlushHistory(true);
    CleanPopup();
}

void CMenuStack::AddToHistory(const char *name)
{
    char *slot = m_history.m_free.back();

    m_history.m_free.pop_back();
    m_history.m_used.PtrVec::insert(m_history.m_used.PtrVec::end(), 1, (unsigned int &)slot);
    strcpy(slot, name);
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

static char s_savename[64];

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
