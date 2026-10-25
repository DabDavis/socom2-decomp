/* zNetMediusLobby.cpp -- the lobby's account and game-wide requests: login and logout, the end
 * of game report, the announcements, the mission sequence, banning oneself from a game (SOCOM 1's
 * zNetMedius.cpp; the unit and its name are ours: OVL1 from about 0x2F6290, after the chat
 * unit's queues, to about 0x2FB080 -- its own copy of "Retrieving messages from server..."
 * (0x3F57C0, the clan unit's is 0x3F4460) says it is not the clan unit). Only some of it so far.
 * Defined in reverse of their image order (-inline deferred). */

#include <string.h>
#include "zonline.h"

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

struct MediusGetAnnouncementsResponse { MEDIUS_RESPONSE_HEAD };
struct MediusBanPlayerResponse { MEDIUS_RESPONSE_HEAD };

/* the account logged in (OVL2 bss; SOCOM 1's AccountInfo) */
struct AccountInfoType {
    int AccountID;   // 0x0
    char AccountName[32];   // 0x4
};

void cbGetAnnouncements(int, int, int, MediusGetAnnouncementsResponse *);
void cbBanSelfFromGame(int, int, int, MediusBanPlayerResponse *);

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
    VALVE_TYPE_1
};

class CValve {
public:
    static CValve *Create(const char *name, VALVE_TYPE type);
    /* off (the name ours: the test is materialised, sltu and xori, before the branch -- an
     * inline bool returning the negation) */
    bool IsOff() { return !m_value; }

    char *m_name;   // 0x0
    unsigned int m_value : 16;   // 0x4
    unsigned int m_type : 8;   // 0x4
    unsigned int m_name_pooled : 1;   // 0x4
    unsigned int m_pooled : 1;   // 0x4
    unsigned int m_unused : 6;   // 0x4
};

extern "C" {
CUIVariable *func_003857C0(void *mgr, const char *name, void *rdr, int longevity);   /* CUIVarManager::Add */
}

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

struct MediusGetPolicyResponse { MEDIUS_RESPONSE_HEAD };
struct MediusClearGameListFilterResponse { MEDIUS_RESPONSE_HEAD };
struct MediusSetGameListFilterResponse { MEDIUS_RESPONSE_HEAD };
struct MediusGameListResponse { MEDIUS_RESPONSE_HEAD };
struct MediusGetGamePlayersResponse {
    MEDIUS_RESPONSE_HEAD
    int AccountID;   // 0x1c
    char AccountName[32];   // 0x20
    char Stats[256];   // 0x40
    int _140;
    char EndOfList;   // 0x144
};

/* a player's stats unpacked (the stats parser 0x2F9AC0's output: only the flag read here, the
 * layout ours) */
struct GamePlayerStats {
    unsigned int _0 : 15;
    unsigned int m_late : 1;   // byte 1, bit 7: counted
    unsigned char _4[0xf0 - 4];
};

void cbGetPolicy(int, int, int, MediusGetPolicyResponse *);
void cbSetGameListFilter(int, int, int, MediusSetGameListFilterResponse *);
void cbCountGamePlayers(int, int, int, MediusGetGamePlayersResponse *);

extern "C" {
/* a response or request put into the host's order: the PS2 needs nothing, so it is empty
 * (0x2F4080), and the response's converter (0x2F4030) only fixes the type-129 ones */
void func_002F4030(void *msg, int type);
void func_002F4080(void *msg, int type);
void func_002F9AC0(GamePlayerStats *stats, const char *text);   /* a player's stats unpacked */

MediusErrorCode func_006495E0(MediusGetPolicyRequest *req,
                              void (*cb)(int, int, int, MediusGetPolicyResponse *));
MediusErrorCode func_00649960(MediusSetGameListFilterRequest *req,
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

struct MediusPlayerInfoResponse { MEDIUS_RESPONSE_HEAD };
struct MediusUpdateAccountStatsResponse { MEDIUS_RESPONSE_HEAD };
struct MediusUpdateLadderStatsResponse { MEDIUS_RESPONSE_HEAD };

extern "C" {
void func_002FAB80(char *stats);   /* our stats as a text */
extern char data_0066A648[256];   /* our stats' text kept */
extern GamePlayerStats data_0066A750;   /* and unpacked */
extern int data_0066A754;   /* the ladder figure in it */
MediusErrorCode func_006482B0(MediusUpdateAccountStatsRequest *req,
                              void (*cb)(int, int, int, MediusUpdateAccountStatsResponse *));
MediusErrorCode func_0064B248(MediusUpdateLadderStatsRequest *req,
                              void (*cb)(int, int, int, MediusUpdateLadderStatsResponse *));
void func_002FAB70(int, int, int, MediusUpdateAccountStatsResponse *);
void func_002FAB60(int, int, int, MediusUpdateLadderStatsResponse *);
MediusErrorCode func_00648D30(MediusPlayerInfoRequest *req,
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
        func_003862A0(func_003857C0(&theUIVarManager, "PLAYERPASSWORD", 0, 1), "");
}

/* our stats sent to the server: built into a text, kept, unpacked, and our ladder figure sent too
 * (SOCOM II's; OVL1 0x2FAA80: the active lobby's handler sends them every five minutes) */
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
        CheckMediusError(func_006482B0(&req, func_002FAB70));
        memset(&ladder, 0, sizeof(ladder));
        ladder.LadderType = 0;
        ladder.Stat = data_0066A754;
        CheckMediusError(func_0064B248(&ladder, func_002FAB60));
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
        CheckMediusError(func_00648D30(&req, func_002FA8F0));
    }
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
    CheckMediusError(func_00649960(&req, cbSetGameListFilter));
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

/* the usage policy asked for, the menu told meanwhile (SOCOM II's; OVL1 0x2F79F0:
 * UIGetUsagePolicy calls it on theLobby; the name ours) */
void CZOnlineLobby::GetUsagePolicy()
{
    if (m_isopen) {
        func_003862A0(func_003857C0(&theUIVarManager, "UsagePolicy", 0, 1),
                      "Retrieving Usage Policy.....");
        func_003862A0(func_003857C0(&theUIVarManager, "TempUsagePolicy", 0, 1), " ");
        MediusGetPolicyRequest req;

        memset(&req, 0, sizeof(req));
        SetSessionKey(req.SessionKey);
        req.Policy = 0;
        CheckMediusError(func_006495E0(&req, cbGetPolicy));
    }
}

/* the announcements asked for, the menu told meanwhile (SOCOM 1's; OVL1 0x2F7710) */
void CZOnlineLobby::GetAnnouncements()
{
    if (m_isopen && m_current_state == LOBBY_ACTIVE) {
        func_003862A0(theLobby.uiv_MediusMOTD, "Retrieving messages from server...");
        MediusGetAnnouncementsRequest req;

        memset(&req, 0, sizeof(req));
        SetSessionKey(req.SessionKey);
        req.ApplicationID = 10472;
        CheckMediusError(MediusGetAnnouncements(&req, cbGetAnnouncements));
    }
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
