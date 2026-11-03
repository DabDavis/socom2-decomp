/* zNetMediusClan.cpp -- the lobby's clans: the member list, the invitations sent and received,
 * the leader's messages, and the Medius requests and callbacks that keep them (SOCOM 1's
 * zNetMedius.cpp's clan code; the unit and its name are ours). OVL1 0x2E5FE0-0x2EA5BF: after the
 * room list's unit (its vector's destructor 0x2E5F60 closes that one) and before the ladder's
 * (0x2EA5C0); its static initialiser 0x401EB0 builds the four vectors (0x44F700, 0x44F720,
 * 0x44F740, 0x44F760) whose destructors close it (0x2EA3C0-0x2EA5BF). Only some of it so far.
 * Defined in reverse of their image order (-inline deferred). */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "zonline.h"

/* the account logged in (OVL2 bss, as SessionKey; SOCOM 1's AccountInfo) */
struct AccountInfoType {
    int AccountID;   // 0x0
    char AccountName[32];   // 0x4
};

/* the clan's record (SOCOM 1's StoredClanInfo, the offsets alike) */
struct StoredClanInfoType {
    unsigned char _0[0x1c];
    int ClanID;   // 0x1c
    unsigned char _20[4];
    char ClanName[32];   // 0x24
    int LeaderAccountID;   // 0x44
    char LeaderAccountName[32];   // 0x48
    char Stats[256];   // 0x68: the icon (three digits), the ranking stat (twelve), the description
    unsigned char _168[0x170 - 0x168];
};   // 0x170: a MediusGetMyClansResponse kept

void UpdateInvitations();   /* the received invitations shown (SOCOM 1's) */
void cbTransferClanLeadership(MediusErrorCode, MediusErrorCode, MediusErrorCode,
                              MediusTransferClanLeadershipResponse *);
void cbRemovePlayerFromClan(MediusErrorCode, MediusErrorCode, MediusErrorCode,
                            MediusRemovePlayerFromClanResponse *);

void cbGetClanMemberList(MediusErrorCode, MediusErrorCode, MediusErrorCode, MediusGetClanMemberListResponse *);
void cbDisbandClan(MediusErrorCode, MediusErrorCode, MediusErrorCode, MediusDisbandClanResponse *);
void cbGetClanInvitationsSent(MediusErrorCode, MediusErrorCode, MediusErrorCode, MediusGetClanInvitationsSentResponse *);
void cbCheckMyClanInvitations(MediusErrorCode, MediusErrorCode, MediusErrorCode, MediusCheckMyClanInvitationsResponse *);
void cbRespondToClanInvitation(MediusErrorCode, MediusErrorCode, MediusErrorCode, MediusRespondToClanInvitationResponse *);
void cbQuitClan(MediusErrorCode, MediusErrorCode, MediusErrorCode, MediusRemovePlayerFromClanResponse *);
void cbDeleteClanMessage(MediusErrorCode, MediusErrorCode, MediusErrorCode, MediusDeleteClanMessageResponse *);
void cbGetLeaderMessages(MediusErrorCode, MediusErrorCode, MediusErrorCode, MediusGetAllClanMessagesResponse *);
void cbGetMyClanMessages(MediusErrorCode, MediusErrorCode, MediusErrorCode, MediusGetMyClanMessagesResponse *);
void cbSendClanMessage(MediusErrorCode, MediusErrorCode, MediusErrorCode, MediusSendClanMessageResponse *);
void cbFilterClanName(int, int, int, MediusTextFilterResponse *);
void cbCreateClan(MediusErrorCode, MediusErrorCode, MediusErrorCode, MediusCreateClanResponse *);

struct MediusRevokeClanInvitationRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0x0
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];   // 0x15
    int PlayerAccountID;   // 0x28
};   // 0x2c

struct MediusUpdateClanStatsRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0x0
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];   // 0x15
    int ClanID;   // 0x28
    char Stats[256];   // 0x2c
};   // 0x12c

struct MediusGetMyClansRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0x0
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];   // 0x15
    int Start;   // 0x28
    int PageSize;   // 0x2c
};   // 0x30

/* an invitation to the clan sent: the account and a message */
struct MediusInvitePlayerToClanRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0x0
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];   // 0x15
    int PlayerAccountID;   // 0x28
    char InviteMessage[200];   // 0x2c
};   // 0xf4

/* an account's number: the status after it (as the friends unit's) */
struct MediusAccountGetIDResponse {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0x0
    int AccountID;   // 0x18
    MediusCallbackStatus StatusCode;   // 0x1c
};

struct MediusInvitePlayerToClanResponse { MEDIUS_RESPONSE_HEAD };
void cbInvitePlayerToClanInvitation(MediusErrorCode, MediusErrorCode, MediusErrorCode,
                                    MediusInvitePlayerToClanResponse *);

struct MediusRevokeClanInvitationResponse { MEDIUS_RESPONSE_HEAD };
struct MediusUpdateClanStatsResponse { MEDIUS_RESPONSE_HEAD };
/* our clan (the SDK's response, kept whole in StoredClanInfo's layout) */
struct MediusGetMyClansResponse {
    MEDIUS_RESPONSE_HEAD
    int ClanID;   // 0x1c
    int ApplicationID;   // 0x20
    char ClanName[32];   // 0x24
    int LeaderAccountID;   // 0x44
    char LeaderAccountName[32];   // 0x48
    char Stats[256];   // 0x68
    int Status;   // 0x168
    char EndOfList;   // 0x16c
};

void cbRevokePlayerInvitation(MediusErrorCode, MediusErrorCode, MediusErrorCode,
                              MediusRevokeClanInvitationResponse *);
void cbUpdateClanStats(MediusErrorCode, MediusErrorCode, MediusErrorCode,
                       MediusUpdateClanStatsResponse *);
void cbGetMyClans(MediusErrorCode, MediusErrorCode, MediusErrorCode, MediusGetMyClansResponse *);

/* a variable's text when it holds one: String's own test made again (the target tests the type
 * twice; of six spellings only the early return around the ternary leaves the second zero in the
 * second test's delay slot) */
inline char *ZString(_zrdr *z)
{
    return z->type == 3 ? z->string : 0;
}

inline char *VarText(_zrdr *z)
{
    if (z->type == 3)
        return ZString(z);
    return 0;
}

extern "C" {
extern int data_0044F770;   /* the clan's ranking stat (UpdateClanRanking sets it) */
extern int data_0044F798;   /* the invitation's progress: 2 sent, 3 no such player */
void func_002F4D60(CZOnlineLobby *lobby, int account, int n, const char *text);   /* told the player */
MediusErrorCode MediusInvitePlayerToClan(MediusInvitePlayerToClanRequest *req,
    void (*cb)(MediusErrorCode, MediusErrorCode, MediusErrorCode, MediusInvitePlayerToClanResponse *));
extern int data_0044F788;   /* the clan asked for: 0 not yet, 1 waiting, 2 answered */
extern bool data_0044F790;   /* in a clan, or about to be */
extern char data_0066A790;   /* (OVL2) */
MediusErrorCode MediusRevokeClanInvitation(MediusRevokeClanInvitationRequest *req,
    void (*cb)(MediusErrorCode, MediusErrorCode, MediusErrorCode, MediusRevokeClanInvitationResponse *));
MediusErrorCode MediusUpdateClanStats(MediusUpdateClanStatsRequest *req,
    void (*cb)(MediusErrorCode, MediusErrorCode, MediusErrorCode, MediusUpdateClanStatsResponse *));
MediusErrorCode MediusGetMyClans(MediusGetMyClansRequest *req,
    void (*cb)(MediusErrorCode, MediusErrorCode, MediusErrorCode, MediusGetMyClansResponse *));
extern AccountInfoType AccountInfo;
extern StoredClanInfoType StoredClanInfo;
extern int PlayerClanID;   /* -1 outside a clan */
extern int RemovePlayerFromClanID;   /* the member being put out */
}

void UpdateClanRoster();   /* the roster's UI variable rebuilt */

/* A clan game's team: the clan's id in the low 28 bits, the side (1-4) as 0xA-0xD in the top four
 * (0x2E99F0 builds it back). The helpers sit after the lists' members in the image, 0x2E9820-0x2E9B3F;
 * the names ours. */
extern "C" {
extern unsigned int data_0044F778;   /* the clan whose team we are on */
extern unsigned char data_0044F780;   /* our side, 0 none */
}

/* a team's side (an int: a byte would make each case's constant a daddiu) */
inline int ClanTeamSide(unsigned int team)
{
    switch (team & 0xF0000000) {
    case 0xA0000000:
        return 1;
    case 0xB0000000:
        return 2;
    case 0xC0000000:
        return 3;
    case 0xD0000000:
        return 4;
    }
    return 0;
}

/* the team kept (JoinGame and 0x2F9220; OVL1 0x2E9AB0) */
void SetClanTeam(unsigned int team)
{
    data_0044F778 = team & 0x0FFFFFFF;
    data_0044F780 = ClanTeamSide(team);
}

/* a team we may join: any but side 3 of another clan (0x2F8C60, 0x2F8FD0; OVL1 0x2E9940) */
bool IsClanTeamAllowed(unsigned int team)
{
    unsigned int id = team & 0x0FFFFFFF;

    if ((unsigned char)ClanTeamSide(team) == 3 && id != StoredClanInfo.ClanID)
        return false;
    return true;
}

/* The lists' members, defined first: they close the unit's own code in the image (0x2E9030-0x2E981F),
 * ahead of the clan-team helpers and the member sort. Never inlined in the target -- their callers
 * call them, the destructors too: pinned. */
#pragma auto_inline off

/* the members deleted and the list emptied (OVL1 0x2E9740) */
void ClanMemberVector::DeleteAll()
{
    std::transform(begin(), end(), begin(), DeleteItem<ClanMemberItem>());
    clear();
}

/* a member kept, unless already there: the response copied whole (OVL1 0x2E9590) */
void ClanMemberVector::Add(MediusGetClanMemberListResponse *resp)
{
    if (!Find(resp->AccountID))
        push_back(new ClanMemberItem(*resp));
}

/* the member by name, or by the account number the name spells (OVL1 0x2E9460) */
ClanMemberItem *ClanMemberVector::Find(const char *name)
{
    if (!name || !name[0])
        return 0;
    for (iterator i = begin(); i != end(); ++i)
        if (!strcasecmp((*i)->AccountName, name))
            return *i;
    return Find(atoi(name));
}

/* the invitations received deleted (OVL1 0x2E9380) */
void InvitationVector::DeleteAll()
{
    std::transform(begin(), end(), begin(), DeleteItem<InvitationItem>());
    clear();
}

/* an invitation received kept, unless already there (OVL1 0x2E9240) */
void InvitationVector::Add(MediusCheckMyClanInvitationsResponse *resp)
{
    if (!Find(resp->ClanInvitationID))
        push_back(new InvitationItem(resp));
}

/* the invitations sent deleted (OVL1 0x2E9160) */
void ClanInvitationVector::DeleteAll()
{
    std::transform(begin(), end(), begin(), DeleteItem<ClanInvitationItem>());
    clear();
}

/* an invitation sent kept, unless already there (OVL1 0x2E9030) */
void ClanInvitationVector::Add(MediusGetClanInvitationsSentResponse *resp)
{
    if (!Find(resp->AccountID))
        push_back(new ClanInvitationItem(resp));
}

#pragma auto_inline reset

/* the clan left (SOCOM 1's; OVL1 0x2E8EF0) */
void CZOnlineLobby::QuitClan()
{
    if (m_isopen) {
        MediusRemovePlayerFromClanRequest req;

        memset(&req, 0, sizeof(req));
        SetSessionKey(req.SessionKey);
        req.PlayerAccountID = AccountInfo.AccountID;
        req.ClanID = PlayerClanID;
        CheckMediusError(MediusRemovePlayerFromClan(&req,
            cbQuitClan));
    }
}

extern "C" void func_002FAA80(CZOnlineLobby *lobby);   /* the player's info refreshed (the lobby unit's) */

/* our clan: kept -- its id, leader (the menu told when we became it), name and stats -- its name and
 * description shown, its roster and messages asked for; none, and the clan forgotten, its roster
 * emptied (the menu told it was disbanded when we were in one). Our name shown with the clan's tag,
 * and the request answered (SOCOM 1's; OVL1 0x2E8BD0: GetClan's callback) */
void cbGetMyClans(MediusErrorCode, MediusErrorCode, MediusErrorCode, MediusGetMyClansResponse *resp)
{
    char icon[32];
    char text[256];
    char *tag;

    if (CheckMediusCallbackStatus("GetMyClans", resp->StatusCode)) {
        if (StoredClanInfo.LeaderAccountID != -1 && StoredClanInfo.LeaderAccountID != AccountInfo.AccountID &&
            resp->LeaderAccountID == AccountInfo.AccountID)
            LobbyEvent("MP_MEDIUS_BECAME_LEADER", 0);
        PlayerClanID = resp->ClanID;
        StoredClanInfo.ClanID = PlayerClanID;
        StoredClanInfo.LeaderAccountID = resp->LeaderAccountID;
        strncpy(StoredClanInfo.ClanName, resp->ClanName, sizeof(StoredClanInfo.ClanName));
        CopyField(StoredClanInfo.Stats, resp->Stats, sizeof(StoredClanInfo.Stats));
        func_003862A0(theLobby.uiv_ClanNameVar, StoredClanInfo.ClanName);
        func_003862A0(theLobby.uiv_ClanDescription, &StoredClanInfo.Stats[15]);
        if (StoredClanInfo.Stats)
            memcpy(icon, StoredClanInfo.Stats, 3);
        icon[3] = 0;
        sprintf(icon, "clan_icon_%.2d.tif", atoi(icon));
        theLobby.GetClanRoster();
        func_003862A0(theLobby.uiv_ClanMessages, "Retrieving messages from server...");
        MediusGetMyClanMessagesRequest req;

        memset(&req, 0, sizeof(req));
        SetSessionKey(req.SessionKey);
        req.ClanID = StoredClanInfo.ClanID;
        req.Start = 1;
        req.PageSize = 999;
        CheckMediusError(MediusGetMyClanMessages(&req, cbGetMyClanMessages));
        tag = VarText(theLobby.uiv_ClanDescription);
    } else {
        PlayerClanID = -1;
        StoredClanInfo.ClanID = -1;
        StoredClanInfo.LeaderAccountID = -1;
        memset(StoredClanInfo.ClanName, 0, sizeof(StoredClanInfo.ClanName));
        func_003862A0(theLobby.uiv_ClanNameVar, "");
        func_003862A0(theLobby.uiv_ClanDescription, "");
        if (data_0044F790) {
            LobbyEvent("MP_CLAN_DISBANDED", 0);
            func_002FAA80(&theLobby);
        }
        ClanMemberVec.DeleteAll();
        UpdateClanRoster();
        tag = 0;
    }
    CUIVariable *var = theUIVarManager.Add("PLAYER_NAME_AND_TAG", 0, UIVL_PERMANENT);

    if (tag && tag[0])
        sprintf(text, "#C\x80@ [%s] #C\x80\x80\x80%s", tag, AccountInfo.AccountName);
    else
        sprintf(text, "%s", AccountInfo.AccountName);
    func_003862A0(var, text);
    data_0044F788 = 2;
}

/* our clan asked for, once (SOCOM 1's; OVL1 0x2E8A90): false while the answer is awaited, true
 * outside the active lobby or once it came; the clan's name and description blanked meanwhile */
bool CZOnlineLobby::GetClan()
{
    if (!m_isopen)
        return true;
    if (m_current_state != LOBBY_ACTIVE)
        return true;
    if (data_0044F788 == 1)
        return false;
    if (data_0044F788 == 2) {
        data_0044F788 = 0;
        return true;
    }
    func_003862A0(uiv_ClanNameVar, "");
    func_003862A0(uiv_ClanDescription, "");
    if (data_0044F788 == 0) {
        data_0044F788 = 1;
        data_0044F790 = PlayerClanID != -1 || data_0066A790 != 0;
        MediusGetMyClansRequest req;

        memset(&req, 0, sizeof(req));
        SetSessionKey(req.SessionKey);
        req.Start = 1;
        req.PageSize = 999;
        CheckMediusError(MediusGetMyClans(&req, cbGetMyClans));
    }
    return false;
}

/* an invitation accepted (SOCOM 1's; OVL1 0x2E89F0) */
void CZOnlineLobby::AcceptClanInvitation(int id)
{
    MediusRespondToClanInvitationRequest req;

    memset(&req, 0, sizeof(req));
    SetSessionKey(req.SessionKey);
    req.Response = 1;
    req.ClanInvitationID = id;
    CheckMediusError(MediusRespondToClanInvitation(&req,
        cbRespondToClanInvitation));
}

/* an invitation declined (SOCOM 1's; OVL1 0x2E8980) */
void CZOnlineLobby::DeclineClanInvitation(int id)
{
    MediusRespondToClanInvitationRequest req;

    memset(&req, 0, sizeof(req));
    SetSessionKey(req.SessionKey);
    req.Response = 2;
    req.ClanInvitationID = id;
    CheckMediusError(MediusRespondToClanInvitation(&req,
        cbRespondToClanInvitation));
}

/* the invitations received handed to the menus: each one's number, its clan's leader and our
 * answer, or a line saying there are none (SOCOM 1's; OVL1 0x2E8780) */
void UpdateInvitations()
{
    CRdrEditor ed;
    char text[128];

    if (InvitationVec.size()) {
        ed.push(0);
        for (InvitationVector::iterator i = InvitationVec.begin(); i != InvitationVec.end(); ++i) {
            ed.push(0);
            sprintf(text, "%d", (*i)->ClanInvitationID);
            ed.addstringnode(text);
            ed.addstringnode((*i)->Message);
            switch ((*i)->ResponseStatus) {
            case 0:
                sprintf(text, "Undecided");
                break;
            case 1:
                sprintf(text, "Accepted");
                break;
            case 2:
                sprintf(text, "Declined");
                break;
            default:
                sprintf(text, "Unknown");
                break;
            }
            ed.addstringnode(text);
            ed.pop();
        }
        ed.pop();
    } else {
        ed.push(0);
        ed.push(0);
        ed.addstringnode(" ");
        ed.addstringnode("No Invitations");
        ed.addstringnode(" ");
        ed.pop();
        ed.pop();
    }
    theLobby.uiv_MediusMyInvites->Set(&ed);
}

/* any invitations received (SOCOM 1's; OVL1 0x2E8770) */
bool CZOnlineLobby::HasClanInvitations()
{
    return InvitationVec.size() != 0;
}

/* an invitation received kept; at the list's end the menu told and the list shown (SOCOM 1's;
 * OVL1 0x2E86C0) */
void cbCheckMyClanInvitations(MediusErrorCode, MediusErrorCode, MediusErrorCode,
                              MediusCheckMyClanInvitationsResponse *resp)
{
    if (CheckMediusCallbackStatus("CheckMyClanInvitations", resp->StatusCode)) {
        /* (MW folds the first two tests into one unsigned compare) */
        if (resp->ResponseStatus == 0 || resp->ResponseStatus == 1 || resp->ResponseStatus == 2)
            InvitationVec.Add(resp);
        if (resp->EndOfList && InvitationVec.size()) {
            LobbyEvent("MP_MEDIUS_HAS_INVITES", 0);
            UpdateInvitations();
        }
    } else {
        UpdateInvitations();
    }
}

/* the invitations received asked for again (SOCOM 1's; OVL1 0x2E8650) */
void CZOnlineLobby::CheckClanInvitations()
{
    InvitationVec.DeleteAll();
    MediusCheckMyClanInvitationsRequest req;

    memset(&req, 0, sizeof(req));
    SetSessionKey(req.SessionKey);
    req.PageSize = 999;
    CheckMediusError(MediusCheckMyClanInvitations(&req,
        cbCheckMyClanInvitations));
}

/* a message of our clan's: added to the lines the menu shows, after a blank one; on a failure the
 * menu told there are none (SOCOM 1's; OVL1 0x2E84E0) */
void cbGetMyClanMessages(MediusErrorCode, MediusErrorCode, MediusErrorCode,
                         MediusGetMyClanMessagesResponse *resp)
{
    if (CheckMediusCallbackStatus("GetMyClanMessages", resp->StatusCode)) {
        CRdrEditor ed;

        ed.push(0);
        if (theLobby.uiv_ClanMessages->Type() == 4) {
            for (int i = 0; i < theLobby.uiv_ClanMessages->Count(); i++) {
                _zrdr *line = theLobby.uiv_ClanMessages->Item(i);

                if (line->type == 3)
                    ed.addstringnode(ZString(line));
            }
        }
        ed.addstringnode("\n");
        ed.addstringnode(resp->Message);
        ed.pop();
        theLobby.uiv_ClanMessages->Set(&ed);
    } else
        func_003862A0(theLobby.uiv_ClanMessages, "No clan messages.");
}

/* the clan's messages asked for, the menu told meanwhile (SOCOM II's; OVL1 0x2E8460) */
void CZOnlineLobby::GetClanMessages()
{
    func_003862A0(uiv_ClanMessages, "Retrieving messages from server...");
    MediusGetMyClanMessagesRequest req;

    memset(&req, 0, sizeof(req));
    SetSessionKey(req.SessionKey);
    req.ClanID = StoredClanInfo.ClanID;
    req.Start = 1;
    req.PageSize = 999;
    CheckMediusError(MediusGetMyClanMessages(&req, cbGetMyClanMessages));
}

/* the message sent: the leader's list asked for again, or the failure spelled out for no one
 * (SOCOM 1's; OVL1 0x2E83F0) */
void cbSendClanMessage(MediusErrorCode, MediusErrorCode, MediusErrorCode,
                       MediusSendClanMessageResponse *resp)
{
    char text[1000];

    if (CheckMediusCallbackStatus("SendClanMessage", resp->StatusCode))
        theLobby.GetLeaderMessages();
    else
        sprintf(text, "Failed To Send Message. Error -> %s", LookupMediusCallbackStatus(resp->StatusCode));
}

/* a message to the clan: an empty one refused with an event (SOCOM 1's, the censor gone; OVL1
 * 0x2E8310) */
bool CZOnlineLobby::SendClanMessage(const char *text)
{
    if (!m_isopen)
        return true;
    if (m_current_state != LOBBY_ACTIVE)
        return true;
    if (!text || !text[0]) {
        LobbyEvent("CLAN_MESSAGE_EMPTY", 0);
        return true;
    }
    MediusSendClanMessageRequest req;

    memset(&req, 0, sizeof(req));
    SetSessionKey(req.SessionKey);
    strncpy(req.Message, text, sizeof(req.Message));
    CheckMediusError(MediusSendClanMessage(&req, cbSendClanMessage));
    return true;
}

/* the leadership handed over, or not (SOCOM 1's; OVL1 0x2E82A0) */
void cbTransferClanLeadership(MediusErrorCode, MediusErrorCode, MediusErrorCode,
                              MediusTransferClanLeadershipResponse *resp)
{
    if (CheckMediusCallbackStatus("TransferClanLeadership", resp->StatusCode))
        LobbyEvent("MP_LEADERSHIP_TRANSFER_SUCCESS", 0);
    else
        LobbyEvent("MP_PLAYER_NOT_FOUND", 0);
}

/* the leadership handed to the named member (SOCOM 1's; OVL1 0x2E81B0) */
void CZOnlineLobby::TransferLeadership(const char *name)
{
    if (!name || !name[0]) {
        LobbyEvent("MP_PLAYER_NOT_FOUND", 0);
        return;
    }
    ClanMemberItem *m = ClanMemberVec.Find(name);

    if (!m) {
        LobbyEvent("MP_PLAYER_NOT_FOUND", 0);
        return;
    }
    MediusTransferClanLeadershipRequest req;

    memset(&req, 0, sizeof(req));
    SetSessionKey(req.SessionKey);
    req.NewLeaderAccountID = m->AccountID;
    if (m->AccountName)
        memcpy(req.NewLeaderAccountName, m->AccountName, 32);
    CheckMediusError(MediusTransferClanLeadership(&req, cbTransferClanLeadership));
}

extern "C" {
unsigned char func_002C2FA0();   /* we watch the game (the states lane's: theLobby's spectator test) */
bool func_002C31A0();   /* we play on the game's first side (the states lane's) */
}

/* on the side of a clan game our clan may take: any side in a game of no clan or of side 4, and
 * when watching; else side 1 is the game's clan's and the others its challengers' (SOCOM 1's, which
 * read the side off the game's flags; OVL1 0x2E80C0: UINotOnCorrectClanTeam's) */
bool CZOnlineLobby::OnCorrectClanTeam()
{
    int ours = StoredClanInfo.ClanID;
    unsigned int team = data_0044F778;

    if (!team)
        return true;
    unsigned char side = data_0044F780;

    if (side == 4)
        return true;
    bool challenger = side != 1;
    bool own = ours == team;

    if (func_002C2FA0())
        return true;
    bool first = func_002C31A0();

    if (own && challenger != first)
        return false;
    if (!own && challenger == first)
        return false;
    return true;
}

/* the clan's ranking stat (the request and its answer named after the status its callback checks,
 * 'MediusUpdateClanRanking'; OVL2 0x64B248, a request SOCOM II added to the SDK; the layout as far
 * as the lobby fills it) */
struct MediusUpdateClanRankingRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0x0
    int StatIndex;   // 0x18: 1
    int Stat;   // 0x1c
    unsigned char _20[0x58 - 0x20];
};   // 0x58

struct MediusUpdateClanRankingResponse { MEDIUS_RESPONSE_HEAD };

extern "C" MediusErrorCode func_0064B248(MediusUpdateClanRankingRequest *req,
    void (*cb)(MediusErrorCode, MediusErrorCode, MediusErrorCode, MediusUpdateClanRankingResponse *));

/* the clan's ranking stat sent: the answer only looked at (SOCOM II's; OVL1 0x2E80B0) */
void cbUpdateClanRanking(MediusErrorCode, MediusErrorCode, MediusErrorCode,
                         MediusUpdateClanRankingResponse *resp)
{
    CheckMediusCallbackStatus("MediusUpdateClanRanking", resp->StatusCode);
}

/* the clan's members ranked, the highest ladder stat first (its std::sort instance closes the
 * unit's own code, 0x2E9B40) */
struct ClanMemberOrder {
    ClanMemberOrder() {}
    bool operator()(ClanMemberItem *a, ClanMemberItem *b) const { return a->LadderStat > b->LadderStat; }
};

/* the clan's ranking stat: with eight members or more, the eight best's ladder stats averaged, else
 * 0 -- kept for UpdateClanStats' text and sent (SOCOM II's, the name ours after the status its
 * callback checks; OVL1 0x2E7F50: UpdateClanStats and cbCreateClan call it on theLobby) */
void CZOnlineLobby::UpdateClanRanking()
{
    data_0044F770 = 0;
    if (ClanMemberVec.size() >= 8) {
        int n = 8;

        std::sort(ClanMemberVec.begin(), ClanMemberVec.end(), ClanMemberOrder());
        /* the postfix step makes MW hoist the end into a preheader after the loop, the entry test
         * repeated there (the weapons' tell); the stat added as unsigned puts *i in v1, the sum's
         * load in v0 (an int addend swaps them) */
        for (ClanMemberVector::iterator i = ClanMemberVec.begin(); i != ClanMemberVec.end(); i++)
            if (n-- > 0)
                data_0044F770 += (unsigned int)(*i)->LadderStat;
        data_0044F770 /= 8;
    }
    MediusUpdateClanRankingRequest req;

    memset(&req, 0, sizeof(req));
    req.StatIndex = 1;
    req.Stat = data_0044F770;
    CheckMediusError(func_0064B248(&req, cbUpdateClanRanking));
}

/* the clan's stats kept: our name shown with the clan's tag (SOCOM 1's, which did nothing; OVL1
 * 0x2E7E70) */
void cbUpdateClanStats(MediusErrorCode, MediusErrorCode, MediusErrorCode,
                       MediusUpdateClanStatsResponse *resp)
{
    char text[256];

    if (CheckMediusCallbackStatus("MediusUpdateClanStats", resp->StatusCode)) {
        char *tag = VarText(theLobby.uiv_ClanDescription);
        CUIVariable *var = theUIVarManager.Add("PLAYER_NAME_AND_TAG", 0, UIVL_PERMANENT);

        if (tag && tag[0])
            sprintf(text, "#C\x80@ [%s] #C\x80\x80\x80%s", tag, AccountInfo.AccountName);
        else
            sprintf(text, "%s", AccountInfo.AccountName);
        func_003862A0(var, text);
    }
}

/* the clan's stats sent: its icon and description packed (SOCOM 1's; OVL1 0x2E7D70), then the
 * members' asked for */
void CZOnlineLobby::UpdateClanStats()
{
    char stats[256];
    char *desc = VarText(theUIVarManager.Add("Clan_Description", 0, UIVL_TEMPORARY));
    MediusUpdateClanStatsRequest req;

    sprintf(stats, "clan_icon_%.2d.tif", 0);
    sprintf(stats, "%.3d%.12d%s", 0, data_0044F770, desc);
    memset(&req, 0, sizeof(req));
    SetSessionKey(req.SessionKey);
    memcpy(req.Stats, stats, sizeof(req.Stats));
    req.ClanID = StoredClanInfo.ClanID;
    CheckMediusError(MediusUpdateClanStats(&req, cbUpdateClanStats));
    UpdateClanRanking();
}

/* the clan created: its id kept, we its leader, the menu told, its name taken from the menu's and
 * its stats sent as UpdateClanStats sends them; on a failure the menu told and the names put back
 * (SOCOM 1's; OVL1 0x2E7B80) */
void cbCreateClan(MediusErrorCode, MediusErrorCode, MediusErrorCode, MediusCreateClanResponse *resp)
{
    if (CheckMediusCallbackStatus("MediusCreateClan", resp->StatusCode)) {
        PlayerClanID = resp->ClanID;
        StoredClanInfo.LeaderAccountID = AccountInfo.AccountID;
        StoredClanInfo.ClanID = PlayerClanID;
        LobbyEvent("MP_CREATE_CLAN_SUCCESS", 0);
        strncpy(StoredClanInfo.ClanName, ZString(theLobby.uiv_ClanNameVar), sizeof(StoredClanInfo.ClanName));
        func_003862A0(theLobby.uiv_ClanDescription, "");
        char stats[256];
        char *desc = VarText(theUIVarManager.Add("Clan_Description", 0, UIVL_TEMPORARY));
        MediusUpdateClanStatsRequest req;

        sprintf(stats, "clan_icon_%.2d.tif", 0);
        sprintf(stats, "%.3d%.12d%s", 0, data_0044F770, desc);
        memset(&req, 0, sizeof(req));
        SetSessionKey(req.SessionKey);
        memcpy(req.Stats, stats, sizeof(req.Stats));
        req.ClanID = StoredClanInfo.ClanID;
        CheckMediusError(MediusUpdateClanStats(&req, cbUpdateClanStats));
        theLobby.UpdateClanRanking();
    } else {
        LobbyEvent("MP_CREATE_CLAN_FAILURE", 0);
        func_003862A0(theLobby.uiv_ClanNameVar, "Unaffiliated");
        func_003862A0(theLobby.uiv_ClanDescription, "");
    }
}

/* the player to invite found: invited, with a message he is also told (SOCOM 1's; OVL1 0x2E7850) */
void cbInvitePlayerToClanGetID(int, int, int, MediusAccountGetIDResponse *resp)
{
    MediusInvitePlayerToClanRequest req;
    char text[1024];

    if (CheckMediusCallbackStatus("MediusGetPlayerForInvitation", resp->StatusCode)) {
        memset(&req, 0, sizeof(req));
        SetSessionKey(req.SessionKey);
        req.PlayerAccountID = resp->AccountID;
        sprintf(text, "Clan %s invites you to join.", StoredClanInfo.ClanName);
        strncpy(req.InviteMessage, text, sizeof(req.InviteMessage));
        CheckMediusError(MediusInvitePlayerToClan(&req, cbInvitePlayerToClanInvitation));
        data_0044F798 = 2;
        func_002F4D60(&theLobby, resp->AccountID, 1, text);
        return;
    }
    LobbyEvent("MP_PLAYER_NOT_FOUND", 0);
    data_0044F798 = 3;
}

/* an account's number asked for by name (as the friends unit's) */
struct MediusAccountGetIDRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0x0
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];   // 0x15
    char AccountName[32];   // 0x26
};   // 0x46

extern "C" MediusErrorCode MediusAccountGetID(MediusAccountGetIDRequest *req,
                                              void (*cb)(int, int, int, MediusAccountGetIDResponse *));

/* the named player invited to the clan: his account asked for first, cbInvitePlayerToClanGetID
 * inviting him; a member, ourselves or one invited already is refused with the menu told. False
 * outside the lobby and once a failed search is told (data_0044F798 3, forgotten then); true while
 * one is under way (SOCOM 1's; OVL1 0x2E7600: UIInvitePlayerToClan's) */
bool CZOnlineLobby::InvitePlayerToClan(const char *name)
{
    if (!m_isopen)
        return false;
    if (m_current_state != LOBBY_ACTIVE)
        return false;
    if (data_0044F798 == 3) {
        data_0044F798 = 0;
        return false;
    }
    if (data_0044F798 == 0) {
        if (!name || !name[0]) {
            LobbyEvent("MP_PLAYER_NOT_FOUND", 0);
            return true;
        }
        if (ClanMemberVec.Find(name)) {
            if (!strcasecmp(name, AccountInfo.AccountName))
                LobbyEvent("MP_PLAYER_IS_LEADER", 0);
            else
                LobbyEvent("MP_PLAYER_ALREADY_MEMBER", 0);
            return true;
        }
        if (ClanInvitationVec.Find(name)) {
            LobbyEvent("MP_PLAYER_ALREADY_INVITED", 0);
            return true;
        }
        MediusAccountGetIDRequest req;

        memset(&req, 0, sizeof(req));
        SetSessionKey(req.SessionKey);
        strncpy(req.AccountName, name, sizeof(req.AccountName));
        CheckMediusError(MediusAccountGetID(&req, cbInvitePlayerToClanGetID));
        data_0044F798 = 1;
    }
    return true;
}

/* the clan's name back from the filter: the clan created under it, or the menu told and the
 * name put back (SOCOM 1's; OVL1 0x2E7AA0) */
void cbFilterClanName(int, int, int, MediusTextFilterResponse *resp)
{
    if (resp->Result >= 0) {
        func_003862A0(theLobby.uiv_ClanNameVar, resp->Text);
        MediusCreateClanRequest req;

        memset(&req, 0, sizeof(req));
        SetSessionKey(req.SessionKey);
        strncpy(req.ClanName, resp->Text, sizeof(req.ClanName));
        CheckMediusError(MediusCreateClan(&req, cbCreateClan));
    } else {
        LobbyEvent("MP_FILTER_FAILURE", 0);
        func_003862A0(theLobby.uiv_ClanNameVar, "Unaffiliated");
        func_003862A0(theLobby.uiv_ClanDescription, " ");
    }
}

/* the clan's name filtered first; cbFilterClanName creates it (SOCOM 1's; OVL1 0x2E7A10) */
void CZOnlineLobby::CreateClan(const char *name)
{
    if (m_isopen && m_current_state == LOBBY_ACTIVE) {
        MediusTextFilterRequest req;

        memset(&req, 0, sizeof(req));
        SetSessionKey(req.SessionKey);
        req.TextFilterType = 0;
        if (name)
            memcpy(req.Text, name, 32);
        CheckMediusError(MediusTextFilter(&req, cbFilterClanName));
    }
}

/* an invitation sent taken back, by the invited player's name (SOCOM 1's; OVL1 0x2E72A0) */
bool CZOnlineLobby::RevokeInvite(const char *name)
{
    if (!m_isopen)
        return false;
    if (m_current_state != LOBBY_ACTIVE)
        return false;
    if (ClanInvitationVec.size() == 0)
        return true;
    int id = ClanInvitationVec.FindAccount(name);

    if (id == -1) {
        LobbyEvent("MP_PLAYER_NOT_FOUND", 0);
        return true;
    }
    MediusRevokeClanInvitationRequest req;

    memset(&req, 0, sizeof(req));
    SetSessionKey(req.SessionKey);
    req.PlayerAccountID = id;
    CheckMediusError(MediusRevokeClanInvitation(&req, cbRevokePlayerInvitation));
    return true;
}

/* the named member put out of the clan -- not ourselves (SOCOM 1's; OVL1 0x2E7450) */
bool CZOnlineLobby::RemovePlayerFromClan(const char *name)
{
    if (!m_isopen)
        return false;
    if (m_current_state != LOBBY_ACTIVE)
        return false;
    ClanMemberItem *m = ClanMemberVec.Find(name);

    if (m) {
        if (AccountInfo.AccountID == m->AccountID) {
            LobbyEvent("MP_PLAYER_IS_LEADER", 0);
        } else {
            MediusRemovePlayerFromClanRequest req;

            memset(&req, 0, sizeof(req));
            SetSessionKey(req.SessionKey);
            req.PlayerAccountID = m->AccountID;
            req.ClanID = PlayerClanID;
            RemovePlayerFromClanID = m->AccountID;
            CheckMediusError(MediusRemovePlayerFromClan(&req, cbRemovePlayerFromClan));
        }
    } else {
        LobbyEvent("MP_PLAYER_NOT_FOUND", 0);
    }
    return true;
}

/* any invitations sent (SOCOM 1's; OVL1 0x2E7290) */
bool CZOnlineLobby::ClanHasInvitations()
{
    return ClanInvitationVec.size() != 0;
}

struct MediusAddPlayerToClanResponse { MEDIUS_RESPONSE_HEAD };

/* an accepted invitation's player added: the answer only looked at (SOCOM 1's; OVL1 0x2E7280) */
void cbAddPlayerToClan(MediusErrorCode, MediusErrorCode, MediusErrorCode, MediusAddPlayerToClanResponse *resp)
{
    CheckMediusCallbackStatus("MediusAddPlayerToClan", resp->StatusCode);
}

/* an accepted invitation taken back, so it is not answered twice: the answer only looked at (SOCOM
 * 1's; OVL1 0x2E7270) */
void cbClanInviteRevokeHack(MediusErrorCode, MediusErrorCode, MediusErrorCode,
                            MediusRevokeClanInvitationResponse *resp)
{
    CheckMediusCallbackStatus("ClanInviteRevokeHack", resp->StatusCode);
}

/* the invitations sent handed to the menus: each one's player and answer, or a line saying there
 * are none (SOCOM 1's; OVL1 0x2E70B0) */
void ClanUpdateInvitations()
{
    CRdrEditor ed;

    if (ClanInvitationVec.size()) {
        ed.push(0);
        for (ClanInvitationVector::iterator i = ClanInvitationVec.begin(); i != ClanInvitationVec.end(); ++i) {
            ed.push(0);
            ed.addstringnode((*i)->AccountName);
            switch ((*i)->ResponseStatus) {
            case 0:
                ed.addstringnode("Undecided");
                break;
            case 2:
                ed.addstringnode("Declined");
                break;
            case 1:
                ed.addstringnode("Accepted");
                break;
            default:
                ed.addstringnode("Unknown");
                break;
            }
            ed.pop();
        }
        ed.pop();
    } else {
        ed.push(0);
        ed.push(0);
        ed.addstringnode("No Invitations Sent");
        ed.addstringnode(" ");
        ed.pop();
        ed.pop();
    }
    theLobby.uiv_ClanInvitations->Set(&ed);
}

/* the clan's request to add a player who accepted (SOCOM 1's MediusAddPlayerToClanRequest) */
struct MediusAddPlayerToClanRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0x0
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];   // 0x15
    int PlayerAccountID;   // 0x28
    char Message[200];   // 0x2c
};   // 0xf4

extern "C" MediusErrorCode MediusAddPlayerToClan(MediusAddPlayerToClanRequest *req,
    void (*cb)(MediusErrorCode, MediusErrorCode, MediusErrorCode, MediusAddPlayerToClanResponse *));

/* an invitation sent, as the server has it: an accepted one's player added to the clan (welcomed in
 * the clan's message) and the invitation taken back; any but a revoked one kept, and the menu told
 * at the list's end, or on a failure (SOCOM 1's; OVL1 0x2E6F50) */
void cbGetClanInvitationsSent(MediusErrorCode, MediusErrorCode, MediusErrorCode,
                              MediusGetClanInvitationsSentResponse *resp)
{
    if (CheckMediusCallbackStatus("MediusGetClanInvitationsSent", resp->StatusCode)) {
        if (resp->ResponseStatus == 1) {
            /* (the requests' and the text's slots are the target's in this order of declaration) */
            MediusRevokeClanInvitationRequest rev;
            char text[200];
            MediusAddPlayerToClanRequest add;
            int id = resp->AccountID;

            memset(&add, 0, sizeof(add));
            SetSessionKey(add.SessionKey);
            add.PlayerAccountID = id;
            sprintf(text, "Everyon Welcome %s to the clan", resp->AccountName);
            memcpy(add.Message, text, sizeof(add.Message));
            CheckMediusError(MediusAddPlayerToClan(&add, cbAddPlayerToClan));
            memset(&rev, 0, sizeof(rev));
            SetSessionKey(rev.SessionKey);
            rev.PlayerAccountID = resp->AccountID;
            CheckMediusError(MediusRevokeClanInvitation(&rev, cbClanInviteRevokeHack));
        }
        if (resp->ResponseStatus == 0 || resp->ResponseStatus == 2 || resp->ResponseStatus == 1)
            ClanInvitationVec.Add(resp);
        if (resp->EndOfList)
            ClanUpdateInvitations();
    } else
        ClanUpdateInvitations();
}

/* the invitations sent asked for again (SOCOM 1's; OVL1 0x2E6EA0) */
bool CZOnlineLobby::GetInvitedPlayers()
{
    if (!m_isopen)
        return false;
    if (m_current_state != LOBBY_ACTIVE)
        return false;
    ClanInvitationVec.DeleteAll();
    MediusGetClanInvitationsSentRequest req;

    memset(&req, 0, sizeof(req));
    SetSessionKey(req.SessionKey);
    req.PageSize = 999;
    CheckMediusError(MediusGetClanInvitationsSent(&req,
        cbGetClanInvitationsSent));
    return true;
}

/* a UI variable given an integer, its watchers told when it changes (as zui.h's float setter; the
 * name ours) */
inline void SetVarInt(CUIVariable *var, int i)
{
    if (var->type != 1 || var->integer != i) {
        var->DeleteChildren();
        var->type = 1;
        var->integer = i;
        var->UpdateDependents();
    }
}

/* the members handed to the menus, best first: each one's account (as VALUE), name, whether online,
 * description, ladder position and rank, the leader marked, and where he is (the main menu, a room
 * and a game, or nothing offline); the clan's rating -- its eight best members' average, as
 * UpdateClanRanking's -- kept and shown (SOCOM II's, the name ours; OVL1 0x2E6AF0). The member's account
 * compared and its stat added as unsigned give the target's registers (int operands swap v1 and a0,
 * as in UpdateClanRanking); the online mark is a switch, its zero case laid out first */
void UpdateClanRoster()
{
    CRdrEditor ed;
    char id[32];
    char rank[64];

    data_0044F770 = 0;
    int n = 8;

    std::sort(ClanMemberVec.begin(), ClanMemberVec.end(), ClanMemberOrder());
    ed.push(0);
    for (ClanMemberVector::iterator i = ClanMemberVec.begin(); i != ClanMemberVec.end(); ++i) {
        bool leader = StoredClanInfo.LeaderAccountID == (unsigned int)(*i)->AccountID;
        bool counted = false;

        if (ClanMemberVec.size() >= 8 && n-- > 0)
            counted = true;
        ed.push(0);
        ed.push(0);
        sprintf(id, "%d", (*i)->AccountID);
        ed.addSTRING("VALUE", id);
        ed.pop();
        ed.addstringnode((*i)->AccountName);
        const char *mark;

        switch ((*i)->OnlineState.ConnectStatus) {
        case 0:
            mark = "\xaa";
            break;
        default:
            mark = "\xa7";
            break;
        }
        ed.addstringnode(mark);
        ed.addstringnode(&(*i)->Stats[0x9f]);
        sprintf(rank, "%d", (*i)->LadderPosition);
        ed.addstringnode(rank);
        if (leader)
            ed.addstringnode("(Leader)");
        else
            ed.addstringnode(" ");
        ed.addintnode(theLobby.GetRankLevel((*i)->LadderPosition));
        if ((*i)->OnlineState.ConnectStatus) {
            if (!strncmp((*i)->OnlineState.LobbyName, "Medius", 6)) {
                ed.addstringnode("MAIN MENU");
                ed.addstringnode(" ");
            } else {
                ed.addstringnode((*i)->OnlineState.LobbyName);
                ed.addstringnode((*i)->OnlineState.GameName);
            }
        } else {
            ed.addstringnode("");
            ed.addstringnode("");
        }
        ed.pop();
        if (counted)
            data_0044F770 += (unsigned int)(*i)->LadderStat;
    }
    data_0044F770 /= 8;
    ed.pop();
    theLobby.uiv_ClanRoster->Set(&ed);
    SetVarInt(theUIVarManager.Add("ClanRating", 0, UIVL_TEMPORARY), data_0044F770);
}

/* the member list asked for again, the old one shown emptied (SOCOM 1's; OVL1 0x2E69F0). Called,
 * not inlined, by cbGetMyClans: pinned */
#pragma auto_inline off
bool CZOnlineLobby::GetClanRoster()
{
    if (!m_isopen)
        return false;
    if (m_current_state != LOBBY_ACTIVE)
        return false;
    ClanMemberVec.DeleteAll();
    UpdateClanRoster();
    MediusGetClanMemberListRequest req;

    memset(&req, 0, sizeof(req));
    req.PageSize = 1;
    req.ClanID = PlayerClanID;
    req.Start = 0;
    CheckMediusError(MediusGetClanMemberList(&req,
        cbGetClanMemberList));
    return true;
}
#pragma auto_inline reset

/* the clan gone: its name, description, messages and members cleared, our membership with them;
 * or the failure told with its reason (SOCOM 1's; OVL1 0x2E68E0) */
void cbDisbandClan(MediusErrorCode, MediusErrorCode, MediusErrorCode,
                   MediusDisbandClanResponse *resp)
{
    char text[1024];

    if (CheckMediusCallbackStatus("MediusDisbandClan", resp->StatusCode)) {
        func_003862A0(theLobby.uiv_ClanNameVar, "");
        func_003862A0(theLobby.uiv_ClanDescription, " ");
        func_003862A0(theLobby.uiv_ClanMessages, "No clan messages.");
        ClanMemberVec.DeleteAll();
        UpdateClanRoster();
        PlayerClanID = -1;
        StoredClanInfo.ClanID = -1;
        StoredClanInfo.LeaderAccountID = -1;
        memset(StoredClanInfo.ClanName, 0, sizeof(StoredClanInfo.ClanName));
        LobbyEvent("MP_DISBAND_CLAN_SUCCESS", 0);
    } else {
        sprintf(text, "Clan Disband Error: %s", LookupMediusCallbackStatus(resp->StatusCode));
        LobbyEvent("MP_DISBAND_CLAN_FAILURE", text);
    }
}

/* the clan disbanded (SOCOM 1's; OVL1 0x2E6850) */
void CZOnlineLobby::DisbandClan()
{
    if (m_isopen && m_current_state == LOBBY_ACTIVE) {
        MediusDisbandClanRequest req;

        memset(&req, 0, sizeof(req));
        SetSessionKey(req.SessionKey);
        req.ClanID = StoredClanInfo.ClanID;
        CheckMediusError(MediusDisbandClan(&req,
            cbDisbandClan));
    }
}

/* in a clan (SOCOM 1's; OVL1 0x2E6830) */
bool CZOnlineLobby::IsMemberOfClan()
{
    return PlayerClanID != -1;
}

/* the named member -- or we, when none is -- the clan's leader (SOCOM 1's; OVL1 0x2E67D0).
 * OPEN, 1 instruction: the member's compare is `xor v0,v0,v1` in the target (the leader first, the
 * member's id loaded into v1), `xor v0,v1,v0` here; written leader-first the member's id lands in v0
 * instead and the loads trade registers. Tried: either operand order, an accessor, a local id, a
 * ternary, a bool local, if/else; and (round 5) the member's arm through an inline taking the id,
 * `IsClanLeader(m->AccountID)`: `leader == id` inside gives the target's xor order with the loads
 * swapped (the id in v0), `id == leader` the target's loads with the xor reversed. */
bool CZOnlineLobby::IsLeaderOfClan(const char *name)
{
    ClanMemberItem *m = ClanMemberVec.Find(name);

    if (m)
        return m->AccountID == StoredClanInfo.LeaderAccountID;
    return AccountInfo.AccountID == StoredClanInfo.LeaderAccountID;
}

/* the leader's messages' list (its two members sit here, after their first user, where the other
 * lists' close the unit: pinned like theirs) */
#pragma auto_inline off

/* the messages deleted (OVL1 0x2E66F0) */
void ClanMessageVector::DeleteAll()
{
    std::transform(begin(), end(), begin(), DeleteItem<ClanMessageItem>());
    clear();
}

/* a message kept, unless already there: the response copied whole (OVL1 0x2E65B0) */
void ClanMessageVector::Add(MediusGetAllClanMessagesResponse *resp)
{
    if (!resp)
        return;
    if (!Find(resp->ClanMessageID))
        push_back(new ClanMessageItem(*resp));
}

#pragma auto_inline reset

/* the leader's messages handed to the menus, numbered from 1 -- each one's id, text and number --
 * with an empty one (id -1) to write next while there are fewer than five (SOCOM II's, the name
 * ours after the UI variable it fills; OVL1 0x2E6400: cbGetLeaderMessages calls it) */
void UpdateLeaderMessages()
{
    CRdrEditor ed;
    char text[512];

    ed.push(0);
    int n = 1;   /* (after the push: declared ahead of it, MW sets it in the push's delay slot) */

    for (ClanMessageVector::iterator i = ClanMessageVec.begin(); i != ClanMessageVec.end(); ++i) {
        ClanMessageItem *m = *i;

        ed.push(0);
        sprintf(text, "%d", m->ClanMessageID);
        ed.addstringnode(text);
        ed.addstringnode(m->Message);
        sprintf(text, "Message %d:", n);
        ed.addstringnode(text);
        ed.pop();
        n++;
    }
    if (ClanMessageVec.size() < 5) {
        ed.push(0);
        ed.addstringnode("-1");
        ed.addstringnode(" ");
        sprintf(text, "Message %d:", n);
        ed.addstringnode(text);
        ed.pop();
    }
    ed.pop();
    theUIVarManager.Add("ClanLeaderMessages", 0, UIVL_TEMPORARY)->Set(&ed);
}

/* the leader's messages emptied and asked for (SOCOM II's; OVL1 0x2E6310). Never inlined in the
 * target, though three callbacks of this unit call it: pinned */
#pragma auto_inline off
void CZOnlineLobby::GetLeaderMessages()
{
    ClanMessageVec.DeleteAll();
    MediusGetAllClanMessagesRequest req;

    memset(&req, 0, sizeof(req));
    SetSessionKey(req.SessionKey);
    req.Start = 1;
    req.PageSize = 999;
    CheckMediusError(MediusGetAllClanMessages(&req, cbGetLeaderMessages));
}
#pragma auto_inline reset

/* a leader's message rewritten (the SDK's request: its id and the text) */
struct MediusModifyClanMessageRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0x0
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];   // 0x15
    int ClanMessageID;   // 0x28
    char Message[200];   // 0x2c
};   // 0xf4

struct MediusModifyClanMessageResponse { MEDIUS_RESPONSE_HEAD };

void cbModifyClanMessage(MediusErrorCode, MediusErrorCode, MediusErrorCode, MediusModifyClanMessageResponse *);
extern "C" MediusErrorCode MediusModifyClanMessage(MediusModifyClanMessageRequest *req,
    void (*cb)(MediusErrorCode, MediusErrorCode, MediusErrorCode, MediusModifyClanMessageResponse *));

/* a leader's message written: a new one (id -1) sent as a clan message (SendClanMessage, inlined),
 * a kept one's text replaced -- the API's result not looked at, a success handed to the check, as
 * DeleteClanMessage's; an empty text or an unknown id told (SOCOM II's; OVL1 0x2E60C0: the UI
 * command 'ModifyClanMessage' 0x27A0E0 calls it on theLobby) */
void CZOnlineLobby::ModifyClanMessage(int id, const char *text)
{
    if (text && text[0]) {
        if (id == -1) {
            SendClanMessage(text);
        } else {
            ClanMessageItem *m = ClanMessageVec.Find(id);

            if (m) {
                MediusModifyClanMessageRequest req;

                memset(&req, 0, sizeof(req));
                SetSessionKey(req.SessionKey);
                req.ClanMessageID = id;
                if (text)
                    memcpy(req.Message, text, sizeof(req.Message));
                MediusModifyClanMessage(&req, cbModifyClanMessage);
                CheckMediusError(MediusSuccess);
            } else
                LobbyEvent("CLAN_MESSAGE_ID_ERR", 0);
        }
    } else
        LobbyEvent("CLAN_MESSAGE_EMPTY", 0);
}

/* a message deleted (SOCOM II's; OVL1 0x2E6010): the API's result not looked at, a success
 * handed to the check instead */
void CZOnlineLobby::DeleteClanMessage(int id)
{
    if (id != -1) {
        MediusDeleteClanMessageRequest req;

        memset(&req, 0, sizeof(req));
        SetSessionKey(req.SessionKey);
        req.ClanMessageID = id;
        MediusDeleteClanMessage(&req, cbDeleteClanMessage);
        CheckMediusError(MediusSuccess);
    }
}
