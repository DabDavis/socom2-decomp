/* zNetMediusClan.cpp -- the lobby's clans: the member list, the invitations sent and received,
 * the leader's messages, and the Medius requests and callbacks that keep them (SOCOM 1's
 * zNetMedius.cpp's clan code; the unit and its name are ours). OVL1 0x2E5FE0-0x2EA5BF: after the
 * room list's unit (its vector's destructor 0x2E5F60 closes that one) and before the ladder's
 * (0x2EA5C0); its static initialiser 0x401EB0 builds the four vectors (0x44F700, 0x44F720,
 * 0x44F740, 0x44F760) whose destructors close it (0x2EA3C0-0x2EA5BF). Only some of it so far.
 * Defined in reverse of their image order (-inline deferred). */

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
    int GetAccountID() { return AccountID; }

    unsigned char _0[0x1c];
    int AccountID;   // 0x1c
};

void cbGetClanMemberList(MediusErrorCode, MediusErrorCode, MediusErrorCode, MediusGetClanMemberListResponse *);
void cbDisbandClan(MediusErrorCode, MediusErrorCode, MediusErrorCode, MediusDisbandClanResponse *);
void cbGetClanInvitationsSent(MediusErrorCode, MediusErrorCode, MediusErrorCode, MediusGetClanInvitationsSentResponse *);
void cbCheckMyClanInvitations(MediusErrorCode, MediusErrorCode, MediusErrorCode, MediusCheckMyClanInvitationsResponse *);
void cbRespondToClanInvitation(MediusErrorCode, MediusErrorCode, MediusErrorCode, MediusRespondToClanInvitationResponse *);
void cbQuitClan(MediusErrorCode, MediusErrorCode, MediusErrorCode, MediusRemovePlayerFromClanResponse *);

extern "C" {
extern AccountInfoType AccountInfo;
extern StoredClanInfoType StoredClanInfo;
extern int PlayerClanID;   /* -1 outside a clan */
extern LobbyList InvitationVec;   /* the invitations received */
extern LobbyList ClanInvitationVec;   /* the invitations sent */
extern LobbyList ClanMemberVec;


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
