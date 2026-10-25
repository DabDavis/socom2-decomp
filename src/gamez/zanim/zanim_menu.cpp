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

/* a pointer vector's storage, as MSL keeps it (include/vector) */
typedef std::__vector_pod<unsigned int, std::allocator<unsigned int> > PtrVec;

void CMenuStack::AddToHistory(const char *name)
{
    char *slot = m_free.back();

    m_free.pop_back();
    m_history.PtrVec::insert(m_history.PtrVec::end(), 1, (unsigned int &)slot);
    strcpy(slot, name);
}

CRdrFile *FetchMissionFontsRdr()
{
    return theMission.m_fontsRdr;
}

void CMenuStack::KillPopUp()
{
    if (m_popup) {
        CNode *node = GetMenuNode()->FindChild("POPUP", true);

        if (node)
            node->SetActive(false);
    }
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

bool UIUnInitNetwork(C2DAnimCmdHdr *hdr, float *dT)
{
    theLobby.m_58 = 0;
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

bool UIInitializeOnlineArmory(C2DAnimCmdHdr *hdr, float *dT)
{
    if (data_0045B718) {
        func_0030E6E0(data_0045B718, 0);
        func_0030E490(data_0045B718);
    }
    return true;
}

bool UICreateMPGame(C2DAnimCmdHdr *hdr, float *dT)
{
    if (func_003045B0(&data_0044FE10)) {
        func_00303F40(&data_0044FE10);
        return true;
    }
    return func_002F9410(&theLobby);
}

bool UIClanGameClosed(C2DAnimCmdHdr *hdr, float *dT)
{
    return func_002F8C60(&theLobby, ArgText(ArgName(hdr)));
}

bool UIGameRejectsRank(C2DAnimCmdHdr *hdr, float *dT)
{
    return func_002F8BC0(&theLobby, ArgText(ArgName(hdr)));
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

bool UIQuitClan(C2DAnimCmdHdr *hdr, float *dT)
{
    func_002E8EF0(&theLobby);
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

bool zAnimUIAppMenuTick(_zanim_cmd_hdr *hdr, float *dT)
{
    return true;
}
