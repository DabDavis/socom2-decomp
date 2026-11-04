/* zNetMediusLobby.cpp -- the lobby's account and game-wide requests: login and logout, the end
 * of game report, the announcements, the mission sequence, banning oneself from a game (SOCOM 1's
 * zNetMedius.cpp; the unit and its name are ours: OVL1 from 0x2F6D00 (the spectator test), after
 * the chat unit's deque instances -- 0x2F6340-0x2F6CF7 are those too, called only from the chat
 * deque's copy constructor and buffer destructor (the online lane, round 11) -- to 0x2FAFBF, before
 * the universe unit; its own copy of "Retrieving messages from server..." (0x3F57C0, the clan
 * unit's is 0x3F4460) says it is not the clan unit). Only some of it so far.
 * Defined in reverse of their image order (-inline deferred). */

#include <string.h>
#include "zonline.h"
#include "../znet/znet.h"

struct MediusGetAnnouncementsRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0x0
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];   // 0x15
    int ApplicationID;   // 0x28
};   // 0x2c

/* the end of game report: the session key first, no message id */
struct MediusEndGameReport {
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];   // 0x0
    int MediusWorldID;   // 0x14
    char GameHostStats[64];   // 0x18
    char GameStats[32];   // 0x58
    int WinningTeam;   // 0x78
    int _7c;
};   // 0x80

struct MediusBanPlayerRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0x0
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];   // 0x15
    int PlayerAccountID;   // 0x28
    int BanTime;   // 0x2c: two hours
    int MediusWorldID;   // 0x30
    int _34;   // 0x34
};   // 0x38

/* an announcement (the SDK's: its id, the text, the list's end) */
struct MediusGetAnnouncementsResponse {
    MEDIUS_RESPONSE_HEAD
    int AnnouncementID;   // 0x1c
    char Announcement[1000];   // 0x20
    char EndOfList;   // 0x408
};
struct MediusBanPlayerResponse { MEDIUS_RESPONSE_HEAD };

/* the account logged in (OVL2 bss; SOCOM 1's AccountInfo) */
struct AccountInfoType {
    int AccountID;   // 0x0
    char AccountName[32];   // 0x4
    char AccountPassword[32];   // 0x24
};

void cbGetAnnouncements(int, int, int, MediusGetAnnouncementsResponse *);
void cbBanSelfFromGame(int, int, int, MediusBanPlayerResponse *);

/* a room (0x44FAB0; ReleaseUIResources empties its games): the unit's one dynamically initialised
 * static, built empty and its destructor registered by the unit's initialiser 0x402250 */
BriefingRoom data_0044FAB0(0);

extern "C" {
extern AccountInfoType AccountInfo;
extern int MediusWorldID;   /* the game joined (SOCOM 1's) */

MediusErrorCode MediusGetAnnouncements(MediusGetAnnouncementsRequest *req,
                                       void (*cb)(int, int, int, MediusGetAnnouncementsResponse *));
MediusErrorCode MediusSendEndGameReport(MediusEndGameReport *req);
MediusErrorCode MediusBanPlayer(MediusBanPlayerRequest *req,
                                void (*cb)(int, int, int, MediusBanPlayerResponse *));
}

/* a valve: a named 16-bit value (SOCOM 1's CValve; only what is read here) */
enum VALVE_TYPE {
    VALVE_TYPE_0,
    VALVE_TYPE_1,
    VALVE_TYPE_2,
    VALVE_TYPE_3
};

enum VALVE_STATE {
    VALVE_SAME,
    VALVE_ON,
    VALVE_OFF
};

class CValve {
public:
    static CValve *Create(const char *name, VALVE_TYPE type);
    /* off (the name ours: the test is materialised, sltu and xori, before the branch -- an
     * inline bool returning the negation) */
    bool IsOff() { return !m_value; }
    void MakeCallbacks(VALVE_STATE state);
    /* the value set, the callbacks told when it turns on or off (zanim.h's, as the chat unit's) */
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
    unsigned int m_value : 16;   // 0x4
    unsigned int m_type : 8;   // 0x4
    unsigned int m_name_pooled : 1;   // 0x4
    unsigned int m_pooled : 1;   // 0x4
    unsigned int m_unused : 6;   // 0x4
};

/* the usage policy asked for (SOCOM II's request, the policy 0) */
struct MediusGetPolicyRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0x0
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];   // 0x15
    int Policy;   // 0x28
};   // 0x2c

/* the game list filtered: a field, how it is compared, the value (SOCOM II's request) */
struct MediusSetGameListFilterRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0x0
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];   // 0x15
    int FilterField;   // 0x28
    int ComparisonOperator;   // 0x2c
    int BaselineValue;   // 0x30
};   // 0x34

/* a page of the game list (no session key; the page and its size as halfwords) */
struct MediusGameListRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0x0
    short PageID;   // 0x16
    short PageSize;   // 0x18
};   // 0x1a

/* the players of the game we are in */
struct MediusGetGamePlayersRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0x0
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];   // 0x15
    int MediusWorldID;   // 0x28
};   // 0x2c

/* a page of the policy (the SDK's: the text, the list's end) */
struct MediusGetPolicyResponse {
    MEDIUS_RESPONSE_HEAD
    char Policy[256];   // 0x1c
    char EndOfList;   // 0x11c
};
struct MediusClearGameListFilterResponse { MEDIUS_RESPONSE_HEAD };
struct MediusSetGameListFilterResponse { MEDIUS_RESPONSE_HEAD };
struct MediusGetGamePlayersResponse {
    MEDIUS_RESPONSE_HEAD
    int AccountID;   // 0x1c
    char AccountName[32];   // 0x20
    char Stats[256];   // 0x40
    int _140;
    char EndOfList;   // 0x144
};


void cbGetPolicy(int, int, int, MediusGetPolicyResponse *);
void cbSetGameListFilter(int, int, int, MediusSetGameListFilterResponse *);
void cbCountGamePlayers(int, int, int, MediusGetGamePlayersResponse *);

extern "C" {
/* a response or request put into the host's order: the PS2 needs nothing, so it is empty
 * (0x2F4080), and the response's converter (0x2F4030) only fixes the type-129 ones */
void func_002F4030(void *msg, int type);
void func_002F4080(void *msg, int type);

MediusErrorCode MediusGetPolicy(MediusGetPolicyRequest *req,
                              void (*cb)(int, int, int, MediusGetPolicyResponse *));
MediusErrorCode MediusSetGameListFilter(MediusSetGameListFilterRequest *req,
                              void (*cb)(int, int, int, MediusSetGameListFilterResponse *));
MediusErrorCode func_0064B008(MediusGameListRequest *req,
                              void (*cb)(int, int, int, MediusGameListResponse *));
MediusErrorCode MediusGetGamePlayers(MediusGetGamePlayersRequest *req,
                                     void (*cb)(int, int, int, MediusGetGamePlayersResponse *));
void func_002F8FD0(int, int, int, MediusGameListResponse *);   /* the random game picked */

/* the game filters the random game is picked by (the rooms' unit's statics, 0x44F528-0x44F550) */
extern int data_0044F528;
extern int data_0044F530;
extern int data_0044F538;
extern int data_0044F548;
extern int data_0044F550;

extern unsigned char data_0044FA88;   /* the game's players being counted */
extern int data_0066A990;   /* the players counted so far */
extern int data_0066A9A0;   /* and when the list ended */
}

/* our stats sent (a text of 256) */
struct MediusUpdateAccountStatsRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0x0
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];   // 0x15
    char Stats[256];   // 0x26
};   // 0x126

/* our ladder figure sent (no session key) */
struct MediusUpdateLadderStatsRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0x0
    int LadderType;   // 0x18
    int Stat;   // 0x1c
    unsigned char _20[0x58 - 0x20];
};   // 0x58

/* a player's info asked for (SOCOM 1's MediusGetPlayerInfo) */
struct MediusPlayerInfoRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0x0
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];   // 0x15
    int AccountID;   // 0x28
};   // 0x2c

/* the player's info (the SDK's MediusPlayerInfoResponse: the account's stats as text at 0x48) */
struct MediusPlayerInfoResponse {
    MEDIUS_RESPONSE_HEAD
    char AccountName[32];   // 0x1c
    int ApplicationID;   // 0x3c
    int PlayerStatus;   // 0x40
    int ConnectionClass;   // 0x44
    char Stats[256];   // 0x48
};
struct MediusUpdateAccountStatsResponse { MEDIUS_RESPONSE_HEAD };
struct MediusUpdateLadderStatsResponse { MEDIUS_RESPONSE_HEAD };

extern "C" {
void func_002FAB80(char *stats);   /* our stats as a text */
extern char data_0066A648[256];   /* our stats' text kept */
extern GamePlayerStats data_0066A750;   /* and unpacked */
extern int data_0066A754;   /* the ladder figure in it */
MediusErrorCode MediusAccountUpdateStats(MediusUpdateAccountStatsRequest *req,
                              void (*cb)(int, int, int, MediusUpdateAccountStatsResponse *));
MediusErrorCode func_0064B248(MediusUpdateLadderStatsRequest *req,
                              void (*cb)(int, int, int, MediusUpdateLadderStatsResponse *));
void func_002FAB70(int, int, int, MediusUpdateAccountStatsResponse *);
void func_002FAB60(int, int, int, MediusUpdateLadderStatsResponse *);
MediusErrorCode MediusGetPlayerInfo(MediusPlayerInfoRequest *req,
                              void (*cb)(int, int, int, MediusPlayerInfoResponse *));   /* MediusGetPlayerInfo */
void func_002FA8F0(int, int, int, MediusPlayerInfoResponse *);   /* our info shown */
}

/* the mission and its net game (only what is read here) */
struct CNetGameLobby {
    unsigned char _0[0x114];
    unsigned char m_state;   // 0x114: 2, in progress
};
struct CMissionLobby {
    unsigned char _0[0x1808];
    CNetGameLobby *m_pNetGame;   // 0x1808
};
extern "C" CMissionLobby theMission;

/* the password shown blank unless it is to be kept secure (SOCOM 1's; OVL1 0x2FAF50) */
void CZOnlineLobby::PasswordSecurity()
{
    CValve *valve = CValve::Create("SecurePassword", VALVE_TYPE_1);

    if (valve->IsOff())
        *theUIVarManager.Add("PLAYERPASSWORD", 0, UIVL_TEMPORARY) = "";
}

/* a variable's text when it holds one (as the clan unit's) */
inline char *ZString(_zrdr *z)
{
    return z->type == 3 ? z->string : 0;
}

/* a variable given an integer, the watchers told when it changes (ours: zui.h's float setter's
 * shape, for an int) */
inline void SetInteger(CUIVariable *var, int n)
{
    if (var->type != 1 || var->integer != n) {
        var->DeleteChildren();
        var->type = 1;
        var->integer = n;
        var->UpdateDependents();
    }
}

extern "C" {
void func_002F3EE0(GamePlayerStats *stats);   /* (the chat unit's: the stats put in order) */
extern char data_0066A7B0[];   /* our home town, in our stats (0x60) */
extern char data_0066A7EF[];   /* our description, in them (0x9F) */
}

/* the clan's record (the clan unit's StoredClanInfo: only what the stats read) */
struct LobbyClanInfo {
    unsigned char _0[0x24];
    char ClanName[32];   // 0x24
    int LeaderAccountID;   // 0x44
    unsigned char _48[0x170 - 0x48];
};

extern "C" {
extern LobbyClanInfo StoredClanInfo;
extern float data_004A44A0;   /* the clock */
extern int data_0044F7F8;   /* our position on the ladder (the ladder unit's) */
extern int data_0044FB48;   /* (the chat unit's: cleared with each record made) */
extern unsigned char data_0044FB50;
}

/* our stats as their record (its 'E' text): the kept stats copied -- the record's implicit
 * assignment, member by member -- with whether we are in a game, the minutes since we logged in,
 * our clan's name and description and whether we lead it, our home town and our ladder position
 * put in; two of the chat unit's counts cleared (SOCOM II's; OVL1 0x2FAB80: the stats sender
 * 0x2FAA80 and the chat unit's player report 0x2EF7F0 call it) */
extern "C" void func_002FAB80(char *stats)
{
    GamePlayerStats *s = (GamePlayerStats *)stats;

    *s = data_0066A750;
    s->m_late = theNetwork.m_bJoined;
    s->m_c = (unsigned int)((data_004A44A0 - theLobby.m_loginTime) / 60.0f);
    strncpy(s->m_40, StoredClanInfo.ClanName, sizeof(s->m_40));
    CUIVariable *var = theUIVarManager.Add("Clan_Description", 0, UIVL_TEMPORARY);

    strncpy(s->m_clan, var->type == 3 ? ZString(var) : "", sizeof(s->m_clan));
    if (AccountInfo.AccountID == StoredClanInfo.LeaderAccountID)
        s->m_2_7 = 1;
    else
        s->m_2_7 = 0;
    var = theUIVarManager.Add("PLAYERHOMETOWN", 0, UIVL_TEMPORARY);
    if (var->type == 3 && ZString(var))
        strncpy(s->m_town, ZString(var), sizeof(s->m_town));
    else
        strncpy(s->m_town, "", sizeof(s->m_town));
    s->m_8 = data_0044F7F8;
    data_0044FB48 = 0;
    data_0044FB50 = 0;
}

/* our stats sent to the server: built into a text, kept, unpacked, and our ladder figure sent too
 * (SOCOM II's; OVL1 0x2FAA80: the active lobby's handler sends them every five minutes; our info's
 * callback 0x2FA8F0 calls it out of line, pinned) */
#pragma auto_inline off
extern "C" void func_002FAA80(CZOnlineLobby *lobby)
{
    if (lobby->m_isopen) {
        char stats[256];
        MediusUpdateAccountStatsRequest req;
        MediusUpdateLadderStatsRequest ladder;

        memset(&req, 0, sizeof(req));
        SetSessionKey(req.SessionKey);
        func_002FAB80(stats);
        memcpy(req.Stats, stats, sizeof(req.Stats));
        if (data_0066A648)
            memcpy(data_0066A648, stats, sizeof(data_0066A648));
        func_002F9AC0(&data_0066A750, stats);
        CheckMediusError(MediusAccountUpdateStats(&req, func_002FAB70));
        memset(&ladder, 0, sizeof(ladder));
        ladder.LadderType = 0;
        ladder.Stat = data_0066A754;
        CheckMediusError(func_0064B248(&ladder, func_002FAB60));
    }
}
#pragma auto_inline reset

extern "C" {
/* our own info come in: our stats unpacked, the home town taken from the menu's variable, our
 * ladder figure and description handed to theirs, and our stats sent back unless the server's
 * are an 'E' record (SOCOM II's; OVL1 0x2FA8F0: func_002FA870's callback) */
void func_002FA8F0(int, int, int, MediusPlayerInfoResponse *resp)
{
    if (CheckMediusCallbackStatus("MediusGetPlayerStats", resp->StatusCode)) {
        func_002F9AC0(&data_0066A750, resp->Stats);
        func_002F3EE0(&data_0066A750);
        CUIVariable *town = theUIVarManager.Add("PLAYERHOMETOWN", 0, UIVL_TEMPORARY);

        if (town->type == 3 && ZString(town))
            strncpy(data_0066A7B0, ZString(town), 63);
        else
            strncpy(data_0066A7B0, "", 63);
        CUIVariable *rating = theUIVarManager.Add("LadderRating", 0, UIVL_TEMPORARY);

        SetInteger(rating, data_0066A754);
        *theUIVarManager.Add("PlayerDescription", 0, UIVL_TEMPORARY) = data_0066A7EF;
        if (resp->Stats[0] != 'E')
            func_002FAA80(&theLobby);
    }
}
}

/* our own info asked for (SOCOM 1's active lobby asked for the stats so; OVL1 0x2FA870) */
extern "C" void func_002FA870(CZOnlineLobby *lobby)
{
    if (lobby->m_isopen) {
        MediusPlayerInfoRequest req;

        memset(&req, 0, sizeof(req));
        SetSessionKey(req.SessionKey);
        req.AccountID = AccountInfo.AccountID;
        CheckMediusError(MediusGetPlayerInfo(&req, func_002FA8F0));
    }
}

extern "C" GamePlayerStats data_0044FB60;   /* a new account's stats (the unit's bss) */

/* a new account's stats made: the 'E' record, a rating of 1000, no side, not counted, the gender
 * the menu's valve holds (SOCOM II's; the name ours: inline -- Login and Register build it twice,
 * once for the copy's test of its source and once for the copy). The valve taken into a local
 * (or tested through IsOff(), or the ternary cast to the enum) numbers the select after the
 * value's load: straight off Create() the two swap a1/a2 */
inline GamePlayerStats *NewAccountStats()
{
    memset(&data_0044FB60, 0, sizeof(data_0044FB60));
    data_0044FB60.m_format = 'E';
    data_0044FB60.m_rating = 1000;
    data_0044FB60.m_side = 0;
    data_0044FB60.m_late = 0;
    CValve *gender = CValve::Create("PLAYERGENDER", VALVE_TYPE_3);

    data_0044FB60.m_gender = gender->m_value ? GENDER_FEMALE : GENDER_MALE;
    return &data_0044FB60;
}

/* a field copied when both ends are there, its source evaluated twice -- a macro, where zonline.h's
 * CopyField is a function (SOCOM 1's Login copied a constant source the same way) */
#define COPY_FIELD(dst, src, n) \
    if ((dst) && (src))         \
    memcpy((dst), (src), (n))

/* logged in as an existing account: the account's name and password kept, our stats' text reset to
 * a new account's, and the login state entered (SOCOM 1's, which pushed the state itself and copied
 * a constant record; OVL1 0x2FA5A0: the UI's LobbyLogin calls it) */
bool CZOnlineLobby::Login(const char *name, const char *password)
{
    if (!m_isopen)
        return false;
    AccountInfo.AccountID = -1;
    CopyField(AccountInfo.AccountName, name, sizeof(AccountInfo.AccountName));
    CopyField(AccountInfo.AccountPassword, password, sizeof(AccountInfo.AccountPassword));
    memset(data_0066A648, 0, sizeof(data_0066A648));
    COPY_FIELD(data_0066A648, NewAccountStats(), sizeof(data_0066A648));
    ChangeState(ACCOUNT_LOGIN);
    return true;
}

/* the session ended: the lobby's queue told to end it (SOCOM 1's, which ran the Medius calls
 * itself; OVL1 0x2FA560) */
bool CZOnlineLobby::Logout()
{
    if (!m_isopen)
        return false;
    ChangeState(END_SESSION);
    return true;
}

/* a new account registered: as Login, the registration state entered (SOCOM 1's; OVL1 0x2FA320: the
 * UI's LobbyRegister calls it) */
bool CZOnlineLobby::Register(const char *name, const char *password)
{
    if (!m_isopen)
        return false;
    AccountInfo.AccountID = -1;
    CopyField(AccountInfo.AccountName, name, sizeof(AccountInfo.AccountName));
    CopyField(AccountInfo.AccountPassword, password, sizeof(AccountInfo.AccountPassword));
    memset(data_0066A648, 0, sizeof(data_0066A648));
    COPY_FIELD(data_0066A648, NewAccountStats(), sizeof(data_0066A648));
    ChangeState(ACCOUNT_REGISTRATION);
    return true;
}

extern "C" {
/* our stats read out: a copy of them, put in order (SOCOM II's; OVL1 0x2F98F0: st_mpexit.cpp's
 * career record and the persona's figures read them; the copy is the record's implicit one,
 * member by member -- zonline.h's GamePlayerStats) */
void func_002F98F0(GamePlayerStats *stats)
{
    *stats = data_0066A750;
    func_002F3EE0(stats);
}

/* our stats replaced: put in order and kept, and sent to the server when asked (SOCOM II's; OVL1
 * 0x2F96F0: st_mpexit.cpp's end of game writes the career record back with 1) */
void func_002F96F0(GamePlayerStats *stats, int send)
{
    func_002F3EE0(stats);
    data_0066A750 = *stats;
    if (send)
        func_002FAA80(&theLobby);
}
}

/* we leave the game for good: banned from it for two hours (SOCOM 1's; OVL1 0x2F95F0) */
void CZOnlineLobby::BanSelfFromGame()
{
    if (m_isopen) {
        MediusBanPlayerRequest req;

        memset(&req, 0, sizeof(req));
        SetSessionKey(req.SessionKey);
        req.BanTime = 120;
        req._34 = 0;
        req.PlayerAccountID = AccountInfo.AccountID;
        req.MediusWorldID = MediusWorldID;
        CheckMediusError(MediusBanPlayer(&req, cbBanSelfFromGame));
    }
}

extern "C" void (*data_0044F3C8)(const char *msg, int arg);   /* the lobby's message box (the UI's) */
extern "C" BriefingRoomDB data_0044F568;   /* the room list (the rooms' unit's) */

/* a game of ours asked for, in the lobby proper: its name made unique to the room ("$~" and the
 * room's name after the player's), refused when the room already has a game so named, the next
 * game's valves reset and the creation state entered (SOCOM 1's, which pushed the state itself;
 * OVL1 0x2F9410: the UI's CreateMPGame calls it) */
bool CZOnlineLobby::CreateGame()
{
    if (!m_isopen)
        return false;
    if (m_current_state != LOBBY_ACTIVE)
        return false;
    std::string name(ZString(uiv_GameNameVar));

    name.append("$~");
    const char *world = ZString(uiv_LobbyWorldNameVar);

    name.append(world);
    *uiv_GameNameVar = name.c_str();
    BriefingRoom *room = data_0044F568.FindRoom(world);

    if (!room || room->FindGame(name.c_str())) {
        if (data_0044F3C8)
            data_0044F3C8("MP_GAME_NAME_EXISTS", 0);
        return false;
    }
    CValve::Create("mp_persistent", VALVE_TYPE_1)->SetValue(0);
    CValve::Create("mp_game_sort_order", VALVE_TYPE_1)->SetValue(127);
    ChangeState(CREATE_NEW_GAME);
    return true;
}

void SetClanTeam(unsigned int team);   /* (the clan unit's) */

extern "C" {
extern int data_0066A998;   /* the game's players when joined */
extern BriefingRoomDB data_0044F568;   /* the room list (the rooms' unit's) */
extern unsigned char g_bJoiningRandomGame;

/* the random game picked from those the game list offered (0x44FAB0): the one with the most room,
 * the lower of the 0x38 figures between equals; its world, clan team, name and player count
 * taken, no password, its room looked up by the game's 0x34 and joined -- the menus told either
 * way -- and the candidates let go (OVL1 0x2F9220: the random game's list callback 0x2F8FD0 calls
 * it at the list's end; the name ours) */
void func_002F9220()
{
    if (!data_0044FAB0.size()) {
        LobbyEvent("MP_MEDIUS_JOIN_GAME_FAILURE", 0);
        return;
    }
    RoomGame *best = *data_0044FAB0.begin();

    /* the postfix step: MW hoists the end into a preheader after the function's last block, the
     * entry test repeated there (the weapons' and the clan unit's tell) */
    for (BriefingRoom::iterator i = data_0044FAB0.begin(); i != data_0044FAB0.end(); i++) {
        RoomGame *g = *i;
        int room = best->m_info.m_maxPlayers - best->m_info.m_22;
        int groom = g->m_info.m_maxPlayers - g->m_info.m_22;

        if (room < groom)
            best = g;
        else if (groom == room && g->m_info.m_38 < best->m_info.m_38)
            best = g;
    }
    MediusWorldID = best->m_info.m_ip;
    SetClanTeam(best->m_info.m_3c);
    *theLobby.uiv_GameNameVar = best->m_info.m_name;
    data_0066A998 = best->m_info.m_players;
    *theUIVarManager.Add("PlayerGamePassword", 0, UIVL_PERMANENT) = "";
    BriefingRoom *room = (BriefingRoom *)((BriefingRoom *)&data_0044F568)->FindGame(best->m_info.m_port);

    if (room) {
        g_bJoiningRandomGame = 1;
        LobbyEvent("MP_RANDOM_GAME_FOUND", 0);
        theLobby.JoinRoom(room->LobbyName);
    } else {
        LobbyEvent("MP_MEDIUS_JOIN_GAME_FAILURE", 0);
    }
    data_0044FAB0.DeleteGames();
}
}

/* the game list's filter dropped (the SDK's request: the filter's id) */
struct MediusClearGameListFilterRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0x0
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];   // 0x15
    unsigned int FilterID;   // 0x28
};   // 0x2c

bool GamePassesFilters(RoomGame *game);   /* (the rooms' unit's) */
bool IsClanTeamAllowed(unsigned int team);   /* (the clan unit's) */

extern "C" {
MediusErrorCode MediusClearGameListFilter(MediusClearGameListFilterRequest *req,
                              void (*cb)(int, int, int, MediusClearGameListFilterResponse *));

/* the random game's filter dropped: nothing to do (OVL1 0x2F9210) */
void func_002F9210(int, int, int, MediusClearGameListFilterResponse *)
{
}

/* a page of the game list for the random game: a game with room for us past its spectators'
 * seats, players but not too many for the menu's filter, passing the room's filters, not locked,
 * no player password, our clan team allowed, our rank's type and missions -- kept as a candidate,
 * any other dropped; at the list's end the game picked and the filter dropped (OVL1 0x2F8FD0: the
 * game list's callback cbSetGameListFilter asks with; the name ours) */
void func_002F8FD0(int, int, int, MediusGameListResponse *resp)
{
    if (CheckMediusCallbackStatus("GetRandomGame::GetGameList", (MediusCallbackStatus)resp->m_18)) {
        if (resp->m_ip > -1) {
            RoomGame *game = new TransientGame(resp);
            GameRules rules;

            func_002F7D90(&rules, (GameRules *)resp->m_rules);
            int room = resp->m_maxPlayers - rules.m_spectators;

            if (game && resp->m_players < room && data_0044F530 < resp->m_players &&
                resp->m_players > 0 && GamePassesFilters(game) && resp->m_lock != 3 &&
                !(resp->m_flags & 1) && IsClanTeamAllowed(resp->m_3c) &&
                game->AllowsType(theLobby.GetRankTier(-1)) && game->HasMissions())
                data_0044FAB0.push_back(game);
            else
                delete game;
        }
        if (resp->m_18c) {
            func_002F9220();
            MediusClearGameListFilterRequest req;

            memset(&req, 0, sizeof(req));
            SetSessionKey(req.SessionKey);
            req.FilterID = 1;
            MediusClearGameListFilter(&req, func_002F9210);
        }
    } else {
        LobbyEvent("MP_MEDIUS_JOIN_GAME_FAILURE", 0);
        MediusClearGameListFilterRequest req;

        memset(&req, 0, sizeof(req));
        SetSessionKey(req.SessionKey);
        req.FilterID = 1;
        MediusClearGameListFilter(&req, func_002F9210);
    }
}
}

/* the random game's filters set from the menu's valves, then the game list asked for, a page
 * of 999 (SOCOM II's; OVL1 0x2F8ED0): the callback of the filter set below */
void cbSetGameListFilter(int, int, int, MediusSetGameListFilterResponse *resp)
{
    CheckMediusCallbackStatus("GetRandomGame::ClearGameListFilter", resp->StatusCode);
    data_0044F528 = CValve::Create("GameTypeFilter", VALVE_TYPE_1)->m_value;
    data_0044F530 = CValve::Create("GamePlayerFilter", VALVE_TYPE_1)->m_value;
    data_0044F538 = CValve::Create("GameSpeedFilter", VALVE_TYPE_1)->m_value;
    data_0044F548 = CValve::Create("FF_FILTER", VALVE_TYPE_1)->m_value;
    data_0044F550 = CValve::Create("LockedFilter", VALVE_TYPE_1)->m_value;
    MediusGameListRequest req;

    memset(&req, 0, sizeof(req));
    req.PageID = 1;
    req.PageSize = 999;
    func_002F4080(&req, 33);
    CheckMediusError(func_0064B008(&req, func_002F8FD0));
}

/* the game list's filter cleared for the random game, a new one set (SOCOM II's; OVL1 0x2F8E50:
 * JoinGame("RANDOM") clears the filter with it) */
void cbClearGameListFilter(int, int, int, MediusClearGameListFilterResponse *resp)
{
    CheckMediusCallbackStatus("GetRandomGame::ClearGameListFilter", resp->StatusCode);
    MediusSetGameListFilterRequest req;

    memset(&req, 0, sizeof(req));
    SetSessionKey(req.SessionKey);
    req.FilterField = 1;
    req.ComparisonOperator = 3;
    req.BaselineValue = 1;
    CheckMediusError(MediusSetGameListFilter(&req, cbSetGameListFilter));
}

void SetDatabase(const char *name);   /* the mission database by name (OVL1 0x27F810, SOCOM 1's) */

extern "C" {
int sprintf(char *s, const char *format, ...);
int func_002F6D00(CZOnlineLobby *lobby);   /* we join as a spectator (above, unwritten) */
bool func_002E34D0(RoomGame *game);   /* the game's friendly fire (its 0x30 flags' bit 0; the room unit's) */
bool func_002E33D0(RoomGame *game);   /* and its respawns (bit 1) */
}

/* the random game asked for: the candidates let go, the game list's filter dropped -- its callback
 * sets the filter the random game is picked by (an inline: its request lies below every local of
 * JoinGame, the editor's included, as an inline's do; the name ours, after the callbacks' status
 * texts "GetRandomGame::...") */
inline void GetRandomGame()
{
    data_0044FAB0.DeleteGames();
    MediusClearGameListFilterRequest req;

    memset(&req, 0, sizeof(req));
    SetSessionKey(req.SessionKey);
    req.FilterID = 10;
    CheckMediusError(MediusClearGameListFilter(&req, cbClearGameListFilter));
}

/* a game of the room joined by its name: "RANDOM" has the game list's filter dropped and a game
 * picked from the list (cbClearGameListFilter goes on); a game the room has sets the menus' valves
 * from its rules -- refused when it has no mission of ours, a spectator joining a game without
 * spectator room taken as joined -- its world, clan team and passwords, takes its mission sequence
 * and the database of the current mission (with no mission, the sequence is printed and the join
 * refused); then the player lists cleared and the join (a world known) or the game's creation
 * entered (SOCOM 1's, 0x574 bytes there; OVL1 0x2F83B0: the UI's JoinMPGame calls it) */
bool CZOnlineLobby::JoinGame(const char *name)
{
    if (!m_isopen)
        return false;
    if (m_current_state != LOBBY_ACTIVE)
        return false;
    if (!strcmp(name, "RANDOM")) {
        GetRandomGame();
        return true;
    }
    BriefingRoom *room = data_0044F568.FindRoom(ZString(uiv_LobbyWorldNameVar));

    if (!room)
        return false;
    data_0066A998 = 0;
    MediusWorldID = -1;
    RoomGame *game = room->FindGame(name);

    if (game) {
        if (!game->HasMissions())
            return false;
        if (func_002F6D00(&theLobby) && !game->m_maxSpectators)
            return true;
        CValve::Create("mp_game_sort_order", VALVE_TYPE_1)->SetValue(game->m_19f[1]);
        CValve::Create("MaxPlayersValve", VALVE_TYPE_1)->SetValue(game->m_info.m_maxPlayers);
        CValve::Create("MaxSpectatorsValve", VALVE_TYPE_1)->SetValue(game->m_maxSpectators);
        CValve::Create("mp_friendly_fire", VALVE_TYPE_1)->SetValue(func_002E34D0(game));
        CValve::Create("mp_allow_respawn", VALVE_TYPE_1)->SetValue(func_002E33D0(game));
        CValve::Create("EQlimit", VALVE_TYPE_1)->SetValue(game->m_19f[0]);
        CValve::Create("mp_max_rounds", VALVE_TYPE_1)->SetValue(game->m_19a[1]);
        CValve::Create("mp_max_round_time", VALVE_TYPE_1)->SetValue(game->m_19a[2] * 60);
        CValve::Create("mp_rank_restrictions", VALVE_TYPE_1)->SetValue(game->m_types);
        data_0066A998 = game->m_info.m_players;
        MediusWorldID = game->m_info.m_ip;
        SetClanTeam(game->m_info.m_3c);
        if (!(game->m_info.m_flags & 1))
            *theUIVarManager.Add("PlayerGamePassword", 0, UIVL_PERMANENT) = "";
        if (!(game->m_info.m_flags & 4))
            *theUIVarManager.Add("SpectatorGamePassword", 0, UIVL_PERMANENT) = "";
        unsigned char *seq = game->GetMissionSequence();

        for (int i = 0; i < 32; i++)
            m_MissionSequence[i] = seq[i];
        m_curr_seq_idx = 0;
        for (int i = 0; i < 31; i++) {
            if (!m_MissionSequence[i + 1]) {
                m_curr_seq_idx = i;
                break;
            }
        }
        if (m_MissionSequence[m_curr_seq_idx] > 0) {
            char db[16];

            sprintf(db, "mp%d", m_MissionSequence[m_curr_seq_idx]);
            SetDatabase(db);
        } else {
            char text[256];

            text[0] = 0;
            for (int i = 0; i < 32; i++)
                sprintf(text, "%s %d", text, m_MissionSequence[i]);
            return false;
        }
    }
    *uiv_GameNameVar = name;
    CUIVariable *seals = theUIVarManager.Add("BSEALLISTVAR", 0, UIVL_PERMANENT);
    CUIVariable *terrs = theUIVarManager.Add("BTERRLISTVAR", 0, UIVL_PERMANENT);
    CUIVariable *specs = theUIVarManager.Add("BSPECLISTVAR", 0, UIVL_PERMANENT);
    CRdrEditor ed;

    ed.push(0);
    ed.pop();
    seals->Set(&ed);
    terrs->Set(&ed);
    specs->Set(&ed);
    if (MediusWorldID > -1) {
        ChangeState(JOIN_GAME);
    } else {
        CValve *password = CValve::Create("PasswordValve", VALVE_TYPE_1);
        CUIVariable *spec = theUIVarManager.Add("SpectatorGamePassword", 0, UIVL_PERMANENT);

        password->SetValue(0);
        CValve::Create("MaxPlayersValve", VALVE_TYPE_1)->SetValue(16);
        if (data_0044FE10.IsActive())
            CValve::Create("MaxSpectatorsValve", VALVE_TYPE_1)->SetValue(4);
        else
            CValve::Create("MaxSpectatorsValve", VALVE_TYPE_1)->SetValue(8);
        *spec = "";
        ChangeState(CREATE_NEW_GAME);
    }
    return true;
}

/* the mission sequence handed out: its index and the 32 missions (SOCOM 1's 12; OVL1 0x2F82E0) */
void CZOnlineLobby::GetMissionSequence(unsigned char *idx, unsigned char *seq)
{
    *idx = m_curr_seq_idx;
    for (int i = 0; i < 32; i++)
        seq[i] = m_MissionSequence[i];
}

/* the mission sequence taken from the host: the missions and the index (SOCOM 1's; OVL1
 * 0x2F8270) */
void CZOnlineLobby::SetMissionSequence(unsigned char idx, unsigned char *seq)
{
    for (int i = 0; i < 32; i++)
        m_MissionSequence[i] = seq[i];
    m_curr_seq_idx = idx;
}

/* the index set to the sequence's last mission, the one before the first empty slot (SOCOM 1's;
 * OVL1 0x2F8230) */
void CZOnlineLobby::ResetMissionSequence()
{
    for (int i = 0; i < 31; i++) {
        if (!m_MissionSequence[i + 1]) {
            m_curr_seq_idx = i;
            return;
        }
    }
}

/* on to the sequence's next mission, back to the first past the last: worked on a copy, kept
 * only when the slot is a mission, the mission returned (SOCOM 1's, over 12; OVL1 0x2F8120).
 * The return is the tell: a void leaf never takes v0 for a temporary, and this one keeps the
 * mission in v0 to the end -- UIAdvanceMPMission compares it with 254 and 255 */
int CZOnlineLobby::AdvanceMissionSequence()
{
    unsigned char seq[32];
    unsigned char idx = m_curr_seq_idx;
    int i;
    int mission;

    for (i = 0; i < 32; i++)
        seq[i] = m_MissionSequence[i];
    idx++;
    if (!seq[idx])
        idx = 0;
    mission = seq[idx];
    if (mission > 0) {
        for (i = 0; i < 32; i++)
            m_MissionSequence[i] = seq[i];
        m_curr_seq_idx = idx;
    }
    return mission;
}

/* the end of the game reported (SOCOM 1's; OVL1 0x2F80C0) */
void CZOnlineLobby::SendEndGameReport()
{
    MediusEndGameReport req;

    SetSessionKey(req.SessionKey);
    req.MediusWorldID = MediusWorldID;
    req.GameHostStats[0] = 0;
    req.GameStats[0] = 0;
    req.WinningTeam = 0;
    CheckMediusError(MediusSendEndGameReport(&req));
}

/* a server's addresses (the DME SDK's NetAddressList: a plain struct, copied as words -- the chat
 * unit's) */
struct NetAddressList {
    int AddressType0;   // 0x0
    char Address0[16];   // 0x4
    int BinaryAddress0;   // 0x14
    int AddressType1;   // 0x18
    char Address1[16];   // 0x1c
    int BinaryAddress1;   // 0x2c
};   // 0x30

/* where a server is and how to talk to it (the DME SDK's NetConnectionInfo, 0x9C bytes, the chat
 * unit's: a class with a constructor of its own, so MW copies it member by member) */
struct NetConnectionInfo {
    NetConnectionInfo() {}

    int Type;   // 0x0
    NetAddressList AddressList;   // 0x4
    int WorldID;   // 0x34
    unsigned int ServerKey[16];   // 0x38
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];   // 0x78
    char AccessKey[MEDIUS_SESSIONKEY_MAXLEN];   // 0x89
};   // 0x9c

extern "C" {
extern NetConnectionInfo data_0066A8F0;   /* the game's server (OVL2 bss; the chat unit's) */
}

/* the game's server handed to the network's connect (SOCOM 1's SetConnectionInfo, which CZNetwork's
 * zNetConnect calls on theLobby as SOCOM 1's does; OVL1 0x2F7FA0) */
void CZOnlineLobby::SetConnectionInfo(NetConnectionInfo *info)
{
    *info = data_0066A8F0;
}

/* a game's rules' older versions (zonline.h's GameRules is 'D'): each lacks the last of the bytes
 * before the sequence ('C' 39, 'B' 38, 'A' 37), and the first, unlettered one held five bytes, its
 * third dropped (the names ours) */
struct GameRulesC {
    char m_version;   // 0x0: 'C'
    unsigned char m_fields[6];   // 0x1
    unsigned char m_missions[32];   // 0x7
};   // 0x27
struct GameRulesB {
    char m_version;   // 0x0: 'B'
    unsigned char m_fields[5];   // 0x1
    unsigned char m_missions[32];   // 0x6
};   // 0x26
struct GameRulesA {
    char m_version;   // 0x0: 'A'
    unsigned char m_fields[4];   // 0x1
    unsigned char m_missions[32];   // 0x5
};   // 0x25

/* a record copied from where the source is (the destination a local, its test is not folded:
 * CopyField's) */
inline void CopyIn(void *dst, const void *src, unsigned int n)
{
    if (src)
        memcpy(dst, src, n);
}

extern "C" {
extern int data_00441458;   /* (the persona state's: the side) */

/* a game's rules read into the current version, an older one brought up a version at a time
 * (OVL1 0x2F7D90: RoomGame::UpdateInfo hands it the game's info) -- the current one's copy
 * returning at the end, after the chain; the version read once (the target keeps it and the 'A'
 * for the unlettered form's bytes) */
GameRules *func_002F7D90(GameRules *dst, const GameRules *src)
{
    char version = src->m_version;

    if (version == 'D') {
        CopyField(dst, src, sizeof(GameRules));
    } else if (version == 'A') {
        GameRulesA a;
        GameRulesB b;

        CopyIn(&a, src, sizeof(a));
        b.m_version = 'B';
        b.m_fields[0] = a.m_fields[0];
        b.m_fields[1] = a.m_fields[1];
        b.m_fields[2] = a.m_fields[2];
        b.m_fields[3] = a.m_fields[3];
        b.m_fields[4] = 0;
        memcpy(b.m_missions, a.m_missions, sizeof(b.m_missions));
        return func_002F7D90(dst, (GameRules *)&b);
    } else if (version == 'B') {
        GameRulesB b;
        GameRulesC c;

        CopyIn(&b, src, sizeof(b));
        c.m_version = 'C';
        c.m_fields[0] = b.m_fields[0];
        c.m_fields[1] = b.m_fields[1];
        c.m_fields[2] = b.m_fields[2];
        c.m_fields[3] = b.m_fields[3];
        c.m_fields[4] = b.m_fields[4];
        c.m_fields[5] = 0;
        memcpy(c.m_missions, b.m_missions, sizeof(c.m_missions));
        return func_002F7D90(dst, (GameRules *)&c);
    } else if (version == 'C') {
        GameRulesC c;
        GameRules d;

        CopyIn(&c, src, sizeof(c));
        d.m_version = 'D';
        d.m_rounds = c.m_fields[0];
        d.m_minutes = c.m_fields[1];
        d.m_side = c.m_fields[2];
        d.m_spectators = c.m_fields[3];
        d.m_eqlimit = c.m_fields[4];
        d.m_sort = c.m_fields[5];
        d.m_rank = 0;
        memcpy(d.m_missions, c.m_missions, sizeof(d.m_missions));
        return func_002F7D90(dst, &d);
    } else {
        const char *old = (const char *)src;
        GameRulesA a;
        /* its bytes read before any is stored (the target loads all three, then stores the
         * five; read in the stores, MW keeps each load beside its store) */
        char f1 = old[1];
        char f3 = old[3];
        char f4 = old[4];

        a.m_version = 'A';
        a.m_fields[0] = version;
        a.m_fields[1] = f1;
        a.m_fields[2] = f3;
        a.m_fields[3] = f4;
        CopyIn(a.m_missions, old + 5, sizeof(a.m_missions));
        return func_002F7D90(dst, (GameRules *)&a);
    }
    return dst;
}

/* the rules of the game we host, from the valves the menus set and the lobby's mission sequence
 * (OVL1 0x2F7C40: the game's details sent to the lobby and the LAN lobby's game info) */
void func_002F7C40(GameRules *r)
{
    r->m_version = 'D';
    r->m_rounds = CValve::Create("mp_max_rounds", VALVE_TYPE_1)->m_value;
    r->m_minutes = CValve::Create("mp_max_round_time", VALVE_TYPE_1)->m_value / 60u;   /* (an unsigned divide) */
    r->m_side = data_00441458;
    r->m_spectators = CValve::Create("MaxSpectatorsValve", VALVE_TYPE_1)->m_value;
    r->m_eqlimit = CValve::Create("EQlimit", VALVE_TYPE_1)->m_value;
    r->m_sort = CValve::Create("mp_game_sort_order", VALVE_TYPE_1)->m_value;
    r->m_rank = CValve::Create("mp_rank_restrictions", VALVE_TYPE_1)->m_value;
    for (int i = 0; i < 32; i++)
        r->m_missions[i] = theLobby.m_MissionSequence[i];
}
}

/* a page of the usage policy: added to the lines kept so far in 'TempUsagePolicy', all of them
 * handed to 'UsagePolicy' with the last page (SOCOM II's, the name ours: GetUsagePolicy's callback,
 * its status 'GetPolicyRequest'; OVL1 0x2F7AC0) */
void cbGetPolicy(int, int, int, MediusGetPolicyResponse *resp)
{
    if (CheckMediusCallbackStatus("GetPolicyRequest", resp->StatusCode)) {
        CRdrEditor ed;
        CUIVariable *var = theUIVarManager.Add("TempUsagePolicy", 0, UIVL_TEMPORARY);

        ed.push(0);
        if (var->Type() == 4) {
            for (int i = 0; i < var->Count(); i++) {
                _zrdr *line = var->Item(i);

                if (line->type == 3)
                    ed.addstringnode(line->type == 3 ? line->string : 0);
            }
        }
        ed.addstringnode(resp->Policy);
        ed.pop();
        if (!resp->EndOfList)
            var->Set(&ed);
        else
            theUIVarManager.Add("UsagePolicy", 0, UIVL_TEMPORARY)->Set(&ed);
    }
}

/* the usage policy asked for, the menu told meanwhile (SOCOM II's; OVL1 0x2F79F0:
 * UIGetUsagePolicy calls it on theLobby; the name ours) */
void CZOnlineLobby::GetUsagePolicy()
{
    if (m_isopen) {
        *theUIVarManager.Add("UsagePolicy", 0, UIVL_TEMPORARY) = "Retrieving Usage Policy.....";
        *theUIVarManager.Add("TempUsagePolicy", 0, UIVL_TEMPORARY) = " ";
        MediusGetPolicyRequest req;

        memset(&req, 0, sizeof(req));
        SetSessionKey(req.SessionKey);
        req.Policy = 0;
        CheckMediusError(MediusGetPolicy(&req, cbGetPolicy));
    }
}

/* an announcement: shown first, a blank line after it, then as many of the lines shown before as
 * fit in 800 characters with it (a line's two characters each besides), the newest kept; on a
 * failure the menu told there are none (SOCOM 1's; OVL1 0x2F77A0) */
void cbGetAnnouncements(int, int, int, MediusGetAnnouncementsResponse *resp)
{
    if (CheckMediusCallbackStatus("MediusGetAnnouncements", resp->StatusCode)) {
        CRdrEditor ed;

        ed.push(0);
        ed.addstringnode(resp->Announcement);
        ed.addstringnode("\n");
        if (theLobby.uiv_MediusMOTD->Type() == 4) {
            int n = theLobby.uiv_MediusMOTD->Count();
            int len = strlen(resp->Announcement);   /* (an int: the unsigned sum swaps the addu) */
            int left = 800 - (len + n * 2);
            int i;

            for (i = theLobby.uiv_MediusMOTD->Count() - 1; i >= 0; i--) {
                _zrdr *line = theLobby.uiv_MediusMOTD->Item(i);

                if (line->type == 3) {
                    left -= strlen(ZString(line));
                    if (left < 0)
                        break;
                }
            }
            for (i++; i < theLobby.uiv_MediusMOTD->Count(); i++) {
                _zrdr *line = theLobby.uiv_MediusMOTD->Item(i);

                if (line->type == 3)
                    ed.addstringnode(ZString(line));
            }
        }
        ed.pop();
        theLobby.uiv_MediusMOTD->Set(&ed);
    } else
        *theLobby.uiv_MediusMOTD = "No server messages.";
}

/* the announcements asked for, the menu told meanwhile (SOCOM 1's; OVL1 0x2F7710) */
void CZOnlineLobby::GetAnnouncements()
{
    if (m_isopen && m_current_state == LOBBY_ACTIVE) {
        *theLobby.uiv_MediusMOTD = "Retrieving messages from server...";
        MediusGetAnnouncementsRequest req;

        memset(&req, 0, sizeof(req));
        SetSessionKey(req.SessionKey);
        req.ApplicationID = 10472;
        CheckMediusError(MediusGetAnnouncements(&req, cbGetAnnouncements));
    }
}

extern "C" {
/* the game's players as the three lists the page shows, built while the answer comes in (the
 * names ours) */
extern CRdrEditor *data_0044FA70;
extern CRdrEditor *data_0044FA78;
extern CRdrEditor *data_0044FA80;
void func_002F7020(int, int, int, MediusGetGamePlayersResponse *);   /* the players shown */
}

/* the players of the game we are in asked for, unless an answer is still being built: the three
 * lists made, each opened, the count started again (SOCOM 1's RefreshGamePlayerList; OVL1
 * 0x2F6EC0) */
bool CZOnlineLobby::RefreshGamePlayerList(C2D *page)
{
    if (!m_isopen)
        return false;
    if (data_0044FA70 || data_0044FA78 || data_0044FA80)
        return true;
    data_0066A990 = 0;
    data_0044FA70 = new CRdrEditor;
    data_0044FA70->push(0);
    data_0044FA78 = new CRdrEditor;
    data_0044FA78->push(0);
    data_0044FA80 = new CRdrEditor;
    data_0044FA80->push(0);
    MediusGetGamePlayersRequest req;

    memset(&req, 0, sizeof(req));
    SetSessionKey(req.SessionKey);
    req.MediusWorldID = MediusWorldID;
    func_002F4080(&req, 32);
    CheckMediusError(MediusGetGamePlayers(&req, func_002F7020));
    return true;
}

/* a page of the game's players counted: those flagged in their stats, the total kept when the
 * list ends (SOCOM II's; OVL1 0x2F6E30) */
void cbCountGamePlayers(int, int, int, MediusGetGamePlayersResponse *resp)
{
    func_002F4030(resp, 32);
    if (CheckMediusCallbackStatus("MediusGetGamePlayers", resp->StatusCode)) {
        GamePlayerStats stats;

        func_002F9AC0(&stats, resp->Stats);
        if (stats.m_late)
            data_0066A990++;
        if (resp->EndOfList) {
            data_0044FA88 = 0;
            data_0066A9A0 = data_0066A990;
        }
    }
}

/* the players of the game in progress counted, once at a time, when late joining is on (SOCOM
 * II's; OVL1 0x2F6D70: a lobby state handler, 0x2EF3C0, calls it; the name ours) */
void CountGamePlayers()
{
    if (!data_0044FA88 && theMission.m_pNetGame->m_state == 2 &&
        CValve::Create("late_joiner", VALVE_TYPE_1)->m_value) {
        MediusGetGamePlayersRequest req;

        data_0066A990 = 0;
        memset(&req, 0, sizeof(req));
        SetSessionKey(req.SessionKey);
        req.MediusWorldID = MediusWorldID;
        func_002F4080(&req, 32);
        if (!CheckMediusError(MediusGetGamePlayers(&req, cbCountGamePlayers)))
            data_0044FA88 = 1;
    }
}
