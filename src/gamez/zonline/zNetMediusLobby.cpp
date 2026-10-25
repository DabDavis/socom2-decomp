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

/* the mission sequence handed out: its index and the 32 missions (SOCOM 1's 12; OVL1 0x2F82E0) */
void CZOnlineLobby::GetMissionSequence(unsigned char *idx, unsigned char *seq)
{
    *idx = m_curr_seq_idx;
    for (int i = 0; i < 32; i++)
        seq[i] = m_MissionSequence[i];
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
