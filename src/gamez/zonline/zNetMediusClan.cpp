/* zNetMediusClan.cpp -- the lobby's clans: the member list, the invitations sent and received,
 * the leader's messages, and the Medius requests and callbacks that keep them (SOCOM 1's
 * zNetMedius.cpp's clan code; the unit and its name are ours). OVL1 0x2E5FE0-0x2EA5BF: after the
 * room list's unit (its vector's destructor 0x2E5F60 closes that one) and before the ladder's
 * (0x2EA5C0); its static initialiser 0x401EB0 builds the four vectors (0x44F700, 0x44F720,
 * 0x44F740, 0x44F760) whose destructors close it (0x2EA3C0-0x2EA5BF). Only some of it so far.
 * Defined in reverse of their image order (-inline deferred). */

#include <stdio.h>
#include <string.h>
#include "zonline.h"

/* a vector of pointers, as the lobby's lists are (only the size is read here) */
struct LobbyList {
    unsigned int capacity_;   // 0x0
    unsigned int size_;   // 0x4
    void **data_;   // 0x8
};

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
};

/* a member of the clan (only its account) */
struct ClanMemberItem {
    unsigned char _0[0x1c];
    int AccountID;   // 0x1c
    char AccountName[32];   // 0x20
};

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

extern "C" {
extern AccountInfoType AccountInfo;
extern StoredClanInfoType StoredClanInfo;
extern int PlayerClanID;   /* -1 outside a clan */
extern LobbyList InvitationVec;   /* the invitations received */
extern LobbyList ClanInvitationVec;   /* the invitations sent */
extern LobbyList ClanMemberVec;
extern LobbyList data_0044F760;   /* the leader's messages (SOCOM II's) */
extern int RemovePlayerFromClanID;   /* the member being put out */
void func_002E9240(LobbyList *list, MediusCheckMyClanInvitationsResponse *resp);   /* one kept */
void func_002E66F0(LobbyList *list);   /* the leader's messages deleted */

ClanMemberItem *func_002E9460(LobbyList *list, const char *name);   /* the member by name */
void func_002E9380(LobbyList *list);   /* the received invitations deleted */
void func_002E9160(LobbyList *list);   /* the sent invitations deleted */
void func_002E9740(LobbyList *list);   /* the members deleted */
void func_002E6AF0();   /* the roster's UI variable rebuilt */
}

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

/* any invitations received (SOCOM 1's; OVL1 0x2E8770) */
bool CZOnlineLobby::HasClanInvitations()
{
    return InvitationVec.size_ != 0;
}

/* an invitation received kept; at the list's end the menu told and the list shown (SOCOM 1's;
 * OVL1 0x2E86C0) */
void cbCheckMyClanInvitations(MediusErrorCode, MediusErrorCode, MediusErrorCode,
                              MediusCheckMyClanInvitationsResponse *resp)
{
    if (CheckMediusCallbackStatus("CheckMyClanInvitations", resp->StatusCode)) {
        /* (MW folds the first two tests into one unsigned compare) */
        if (resp->ResponseStatus == 0 || resp->ResponseStatus == 1 || resp->ResponseStatus == 2)
            func_002E9240(&InvitationVec, resp);
        if (resp->EndOfList && InvitationVec.size_) {
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
    func_002E9380(&InvitationVec);
    MediusCheckMyClanInvitationsRequest req;

    memset(&req, 0, sizeof(req));
    SetSessionKey(req.SessionKey);
    req.PageSize = 999;
    CheckMediusError(MediusCheckMyClanInvitations(&req,
        cbCheckMyClanInvitations));
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
    ClanMemberItem *m = func_002E9460(&ClanMemberVec, name);

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

/* the named member put out of the clan -- not ourselves (SOCOM 1's; OVL1 0x2E7450) */
bool CZOnlineLobby::RemovePlayerFromClan(const char *name)
{
    if (!m_isopen)
        return false;
    if (m_current_state != LOBBY_ACTIVE)
        return false;
    ClanMemberItem *m = func_002E9460(&ClanMemberVec, name);

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
    return ClanInvitationVec.size_ != 0;
}

/* the invitations sent asked for again (SOCOM 1's; OVL1 0x2E6EA0) */
bool CZOnlineLobby::GetInvitedPlayers()
{
    if (!m_isopen)
        return false;
    if (m_current_state != LOBBY_ACTIVE)
        return false;
    func_002E9160(&ClanInvitationVec);
    MediusGetClanInvitationsSentRequest req;

    memset(&req, 0, sizeof(req));
    SetSessionKey(req.SessionKey);
    req.PageSize = 999;
    CheckMediusError(MediusGetClanInvitationsSent(&req,
        cbGetClanInvitationsSent));
    return true;
}

/* the member list asked for again, the old one shown emptied (SOCOM 1's; OVL1 0x2E69F0) */
bool CZOnlineLobby::GetClanRoster()
{
    if (!m_isopen)
        return false;
    if (m_current_state != LOBBY_ACTIVE)
        return false;
    func_002E9740(&ClanMemberVec);
    func_002E6AF0();
    MediusGetClanMemberListRequest req;

    memset(&req, 0, sizeof(req));
    req.PageSize = 1;
    req.ClanID = PlayerClanID;
    req.Start = 0;
    CheckMediusError(MediusGetClanMemberList(&req,
        cbGetClanMemberList));
    return true;
}

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
        func_002E9740(&ClanMemberVec);
        func_002E6AF0();
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
 * ternary, a bool local, if/else. */
bool CZOnlineLobby::IsLeaderOfClan(const char *name)
{
    ClanMemberItem *m = func_002E9460(&ClanMemberVec, name);

    if (m)
        return m->AccountID == StoredClanInfo.LeaderAccountID;
    return AccountInfo.AccountID == StoredClanInfo.LeaderAccountID;
}

/* the leader's messages emptied and asked for (SOCOM II's; OVL1 0x2E6310). Never inlined in the
 * target, though three callbacks of this unit call it: pinned */
#pragma auto_inline off
void CZOnlineLobby::GetLeaderMessages()
{
    func_002E66F0(&data_0044F760);
    MediusGetAllClanMessagesRequest req;

    memset(&req, 0, sizeof(req));
    SetSessionKey(req.SessionKey);
    req.Start = 1;
    req.PageSize = 999;
    CheckMediusError(func_00646520(&req, cbGetLeaderMessages));
}
#pragma auto_inline reset

/* a message deleted (SOCOM II's; OVL1 0x2E6010): the API's result not looked at, a success
 * handed to the check instead */
void CZOnlineLobby::DeleteClanMessage(int id)
{
    if (id != -1) {
        MediusDeleteClanMessageRequest req;

        memset(&req, 0, sizeof(req));
        SetSessionKey(req.SessionKey);
        req.ClanMessageID = id;
        func_006460B0(&req, cbDeleteClanMessage);
        CheckMediusError(MediusSuccess);
    }
}
