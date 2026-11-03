/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* The Medius clan API (OVL2 0x645710-0x6469FC, SOCOM 1's object at 0x3B0EE8, MediusCreateClan
 * first), SCEA's Medius client library, prebuilt with ee-gcc 2.96 at -O2 -G0 (README "The GCC
 * front"). Every request goes the same way: its strings terminated, the library initialised and
 * connected, the caller's callback kept in the request's own slot, the message sent to the lobby
 * server; the result is a MediusErrorCode (medius.h). SOCOM 1's 26 functions are SOCOM II's in the
 * same order, and their callback slots too (SOCOM 1's *CallbackFunctionPtr globals, one per
 * request, in the same order from CreateClan's): the names are SOCOM 1's (s1:order), the five the
 * online lane named by call site among them. Four are SOCOM II's own, after UpdateClanStats:
 * - 0x646708 keeps SOCOM 1's GetClanMemberList slot (the 26th) and its request (the session key
 *   and a clan id): it is SOCOM 1's MediusGetClanMemberList. 0x6468C8, which the lobby's
 *   GetClanRoster calls, is a request with paging and no session key, new here (its slot is the
 *   29th; a 476-byte extra-info answer): MediusGetClanMemberList_ExtraInfo, a predicted name (the
 *   online lane's round 9 verdict moved the names; the integrator applied it);
 * - 0x6467A8 and 0x646838: the clan ladder's list and a clan's position (the online lane's
 *   GetClanLadderList/GetClanLadderLeaders and GetClanRanking call them);
 * - 0x646958: an invitation by account name, answered through InvitePlayerToClan's slot. */

#include "medius.h"

extern int GlobalInitFlag;              /* read as a word here (the API object's is a halfword) */

typedef struct MediusCreateClanRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];
    int ApplicationID;
    char ClanName[MEDIUS_CLANNAME_MAXLEN];
} MediusCreateClanRequest;

typedef struct MediusDisbandClanRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];
    int ClanID;
} MediusDisbandClanRequest;

typedef struct MediusGetClanByNameRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];
    int ApplicationID;
    char ClanName[MEDIUS_CLANNAME_MAXLEN];
} MediusGetClanByNameRequest;

typedef struct MediusGetClanByIDRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];
    int ApplicationID;
    int ClanID;
} MediusGetClanByIDRequest;

typedef struct MediusTransferClanLeadershipRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];
    int NewLeaderAccountID;
    char NewLeaderAccountName[MEDIUS_ACCOUNTNAME_MAXLEN];
} MediusTransferClanLeadershipRequest;

typedef struct MediusAddPlayerToClanRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];
    int ClanInvitationID;
    char Message[MEDIUS_CLANMSG_MAXLEN];
} MediusAddPlayerToClanRequest;

typedef struct MediusRemovePlayerFromClanRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];
    int PlayerAccountID;
    int ClanID;
} MediusRemovePlayerFromClanRequest;

typedef struct MediusInvitePlayerToClanRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];
    int PlayerAccountID;
    char InviteMessage[MEDIUS_CLANMSG_MAXLEN];
} MediusInvitePlayerToClanRequest;

typedef struct MediusCheckMyClanInvitationsRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];
    int Start;
    int PageSize;
} MediusCheckMyClanInvitationsRequest;

typedef struct MediusGetClanInvitationsSentRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];
    int Start;
    int PageSize;
} MediusGetClanInvitationsSentRequest;

typedef struct MediusRespondToClanInvitationRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];
    int ClanInvitationID;
    int Response;
    char Message[MEDIUS_CLANMSG_MAXLEN];
} MediusRespondToClanInvitationRequest;

typedef struct MediusRevokeClanInvitationRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];
    int PlayerAccountID;
} MediusRevokeClanInvitationRequest;

typedef struct MediusGetMyClanMessagesRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];
    int ClanID;
    int Start;
    int PageSize;
} MediusGetMyClanMessagesRequest;

typedef struct MediusSendClanMessageRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];
    char Message[MEDIUS_CLANMSG_MAXLEN];
} MediusSendClanMessageRequest;

typedef struct MediusModifyClanMessageRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];
    int ClanMessageID;
    char Message[MEDIUS_CLANMSG_MAXLEN];
} MediusModifyClanMessageRequest;

typedef struct MediusDeleteClanMessageRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];
    int ClanMessageID;
} MediusDeleteClanMessageRequest;

typedef struct MediusRequestClanTeamChallengeRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];
    int AgainstClanID;
    char Message[MEDIUS_CLANMSG_MAXLEN];
} MediusRequestClanTeamChallengeRequest;

typedef struct MediusRespondToClanTeamChallengeRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];
    int ClanChallengeID;
    int Response;
    char Message[MEDIUS_CLANMSG_MAXLEN];
} MediusRespondToClanTeamChallengeRequest;

typedef struct MediusRevokeClanTeamChallengeRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];
    int ClanChallengeID;
} MediusRevokeClanTeamChallengeRequest;

typedef struct MediusGetClanTeamChallengesRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];
    int ClanID;
    int Status;
    int Start;
    int PageSize;
    int ChallengeType;
} MediusGetClanTeamChallengesRequest;

typedef struct MediusGetMyClansRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];
    int Start;
    int PageSize;
} MediusGetMyClansRequest;

typedef struct MediusConfirmClanTeamChallengeRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];
    int ClanChallengeID;
} MediusConfirmClanTeamChallengeRequest;

typedef struct MediusGetAllClanMessagesRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];
    int Start;
    int PageSize;
} MediusGetAllClanMessagesRequest;

typedef struct MediusGetClanTeamChallengeHistoryRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];
    int ClanID;
    int ThisClanIsChallenger;
    int Start;
    int PageSize;
} MediusGetClanTeamChallengeHistoryRequest;

typedef struct MediusUpdateClanStatsRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];
    int ClanID;
    char Stats[MEDIUS_CLANSTATS_MAXLEN];
} MediusUpdateClanStatsRequest;

typedef struct MediusClanMembersRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];
    int ClanID;
} MediusClanMembersRequest;

typedef struct MediusGetClanLadderListRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
    int ClanLadderStatIndex;
    int SortOrder;
    int StartPosition;
    int PageSize;
} MediusGetClanLadderListRequest;

typedef struct MediusClanLadderPositionRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
    int ClanID;
    int ClanLadderStatIndex;
    int SortOrder;
} MediusClanLadderPositionRequest;

typedef struct MediusGetClanMemberListRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
    int ClanID;
    int Start;
    int PageSize;
} MediusGetClanMemberListRequest;

typedef struct MediusInvitePlayerToClanByNameRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
    char PlayerName[MEDIUS_ACCOUNTNAME_MAXLEN];
    char InviteMessage[MEDIUS_CLANMSG_MAXLEN];
} MediusInvitePlayerToClanByNameRequest;

extern MediusCallback CreateClanCallbackFunctionPtr;
extern int data_00686CB4;
extern MediusCallback DisbandClanCallbackFunctionPtr;
extern int data_00686CBC;
extern MediusCallback GetClanByNameCallbackFunctionPtr;
extern int data_00686CCC;
extern MediusCallback GetClanByIDCallbackFunctionPtr;
extern int data_00686CC4;
extern MediusCallback TransferClanLeadershipCallbackFunctionPtr;
extern int data_00686CD4;
extern MediusCallback AddPlayerToClanCallbackFunctionPtr;
extern int data_00686CDC;
extern MediusCallback RemovePlayerFromClanCallbackFunctionPtr;
extern int data_00686CE4;
extern MediusCallback InvitePlayerToClanCallbackFunctionPtr;
extern int data_00686CEC;
extern MediusCallback CheckMyClanInvitationsCallbackFunctionPtr;
extern int data_00686CF4;
extern MediusCallback GetClanInvitationsSentCallbackFunctionPtr;
extern int data_00686CFC;
extern MediusCallback RespondToClanInvitationCallbackFunctionPtr;
extern int data_00686D04;
extern MediusCallback RevokeClanInvitationCallbackFunctionPtr;
extern int data_00686D0C;
extern MediusCallback GetMyClanMessagesCallbackFunctionPtr;
extern int data_00686D1C;
extern MediusCallback SendClanMessageCallbackFunctionPtr;
extern int data_00686D24;
extern MediusCallback ModifyClanMessageCallbackFunctionPtr;
extern int data_00686D2C;
extern MediusCallback DeleteClanMessageCallbackFunctionPtr;
extern int data_00686D34;
extern MediusCallback RequestClanTeamChallengeCallbackFunctionPtr;
extern int data_00686D14;
extern MediusCallback RespondToClanTeamChallengeCallbackFunctionPtr;
extern int data_00686D3C;
extern MediusCallback RevokeClanTeamChallengeCallbackFunctionPtr;
extern int data_00686D44;
extern MediusCallback GetClanTeamChallengesCallbackFunctionPtr;
extern int data_00686D54;
extern MediusCallback GetMyClansCallbackFunctionPtr;
extern int data_00686D4C;
extern MediusCallback ConfirmClanTeamChallengeCallbackFunctionPtr;
extern int data_00686D5C;
extern MediusCallback GetAllClanMessagesCallbackFunctionPtr;
extern int data_00686D64;
extern MediusCallback GetClanTeamChallengeHistoryCallbackFunctionPtr;
extern int data_00686D6C;
extern MediusCallback UpdateClanStatsCallbackFunctionPtr;
extern int data_00686D74;
extern MediusCallback GetClanMemberListCallbackFunctionPtr;
extern int data_00686D9C;
extern MediusCallback data_00686AC4;
extern int data_00686E00;
extern MediusCallback data_00686AC8;
extern int data_00686E08;
extern MediusCallback data_00686ACC;
extern int data_00686E28;
extern int data_00686E48;

MediusErrorCode MediusCreateClan(MediusCreateClanRequest *request, MediusCallback callback)
{
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    iTerminateString(request->SessionKey, MEDIUS_SESSIONKEY_MAXLEN);
    iTerminateString(request->ClanName, MEDIUS_CLANNAME_MAXLEN);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    CreateClanCallbackFunctionPtr = callback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686CB4,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode MediusDisbandClan(MediusDisbandClanRequest *request, MediusCallback callback)
{
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    iTerminateString(request->SessionKey, MEDIUS_SESSIONKEY_MAXLEN);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    DisbandClanCallbackFunctionPtr = callback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686CBC,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode MediusGetClanByName(MediusGetClanByNameRequest *request, MediusCallback callback)
{
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    iTerminateString(request->SessionKey, MEDIUS_SESSIONKEY_MAXLEN);
    iTerminateString(request->ClanName, MEDIUS_CLANNAME_MAXLEN);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    GetClanByNameCallbackFunctionPtr = callback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686CCC,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode MediusGetClanByID(MediusGetClanByIDRequest *request, MediusCallback callback)
{
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    iTerminateString(request->SessionKey, MEDIUS_SESSIONKEY_MAXLEN);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    GetClanByIDCallbackFunctionPtr = callback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686CC4,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode MediusTransferClanLeadership(MediusTransferClanLeadershipRequest *request, MediusCallback callback)
{
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    iTerminateString(request->SessionKey, MEDIUS_SESSIONKEY_MAXLEN);
    iTerminateString(request->NewLeaderAccountName, MEDIUS_ACCOUNTNAME_MAXLEN);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    TransferClanLeadershipCallbackFunctionPtr = callback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686CD4,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode MediusAddPlayerToClan(MediusAddPlayerToClanRequest *request, MediusCallback callback)
{
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    iTerminateString(request->SessionKey, MEDIUS_SESSIONKEY_MAXLEN);
    iTerminateString(request->Message, MEDIUS_CLANMSG_MAXLEN);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    AddPlayerToClanCallbackFunctionPtr = callback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686CDC,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode MediusRemovePlayerFromClan(MediusRemovePlayerFromClanRequest *request, MediusCallback callback)
{
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    iTerminateString(request->SessionKey, MEDIUS_SESSIONKEY_MAXLEN);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    RemovePlayerFromClanCallbackFunctionPtr = callback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686CE4,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode MediusInvitePlayerToClan(MediusInvitePlayerToClanRequest *request, MediusCallback callback)
{
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    iTerminateString(request->SessionKey, MEDIUS_SESSIONKEY_MAXLEN);
    iTerminateString(request->InviteMessage, MEDIUS_CLANMSG_MAXLEN);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    InvitePlayerToClanCallbackFunctionPtr = callback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686CEC,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode MediusCheckMyClanInvitations(MediusCheckMyClanInvitationsRequest *request, MediusCallback callback)
{
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    iTerminateString(request->SessionKey, MEDIUS_SESSIONKEY_MAXLEN);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    CheckMyClanInvitationsCallbackFunctionPtr = callback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686CF4,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode MediusGetClanInvitationsSent(MediusGetClanInvitationsSentRequest *request, MediusCallback callback)
{
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    iTerminateString(request->SessionKey, MEDIUS_SESSIONKEY_MAXLEN);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    GetClanInvitationsSentCallbackFunctionPtr = callback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686CFC,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode MediusRespondToClanInvitation(MediusRespondToClanInvitationRequest *request, MediusCallback callback)
{
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    iTerminateString(request->SessionKey, MEDIUS_SESSIONKEY_MAXLEN);
    iTerminateString(request->Message, MEDIUS_CLANMSG_MAXLEN);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    RespondToClanInvitationCallbackFunctionPtr = callback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686D04,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode MediusRevokeClanInvitation(MediusRevokeClanInvitationRequest *request, MediusCallback callback)
{
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    iTerminateString(request->SessionKey, MEDIUS_SESSIONKEY_MAXLEN);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    RevokeClanInvitationCallbackFunctionPtr = callback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686D0C,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode MediusGetMyClanMessages(MediusGetMyClanMessagesRequest *request, MediusCallback callback)
{
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    iTerminateString(request->SessionKey, MEDIUS_SESSIONKEY_MAXLEN);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    GetMyClanMessagesCallbackFunctionPtr = callback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686D1C,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode MediusSendClanMessage(MediusSendClanMessageRequest *request, MediusCallback callback)
{
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    iTerminateString(request->SessionKey, MEDIUS_SESSIONKEY_MAXLEN);
    iTerminateString(request->Message, MEDIUS_CLANMSG_MAXLEN);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    SendClanMessageCallbackFunctionPtr = callback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686D24,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode MediusModifyClanMessage(MediusModifyClanMessageRequest *request, MediusCallback callback)
{
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    iTerminateString(request->SessionKey, MEDIUS_SESSIONKEY_MAXLEN);
    iTerminateString(request->Message, MEDIUS_CLANMSG_MAXLEN);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    ModifyClanMessageCallbackFunctionPtr = callback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686D2C,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode MediusDeleteClanMessage(MediusDeleteClanMessageRequest *request, MediusCallback callback)
{
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    iTerminateString(request->SessionKey, MEDIUS_SESSIONKEY_MAXLEN);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    DeleteClanMessageCallbackFunctionPtr = callback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686D34,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode MediusRequestClanTeamChallenge(MediusRequestClanTeamChallengeRequest *request, MediusCallback callback)
{
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    iTerminateString(request->SessionKey, MEDIUS_SESSIONKEY_MAXLEN);
    iTerminateString(request->Message, MEDIUS_CLANMSG_MAXLEN);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    RequestClanTeamChallengeCallbackFunctionPtr = callback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686D14,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode MediusRespondToClanTeamChallenge(MediusRespondToClanTeamChallengeRequest *request, MediusCallback callback)
{
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    iTerminateString(request->SessionKey, MEDIUS_SESSIONKEY_MAXLEN);
    iTerminateString(request->Message, MEDIUS_CLANMSG_MAXLEN);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    RespondToClanTeamChallengeCallbackFunctionPtr = callback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686D3C,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode MediusRevokeClanTeamChallenge(MediusRevokeClanTeamChallengeRequest *request, MediusCallback callback)
{
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    iTerminateString(request->SessionKey, MEDIUS_SESSIONKEY_MAXLEN);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    RevokeClanTeamChallengeCallbackFunctionPtr = callback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686D44,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode MediusGetClanTeamChallenges(MediusGetClanTeamChallengesRequest *request, MediusCallback callback)
{
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    iTerminateString(request->SessionKey, MEDIUS_SESSIONKEY_MAXLEN);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    GetClanTeamChallengesCallbackFunctionPtr = callback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686D54,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode MediusGetMyClans(MediusGetMyClansRequest *request, MediusCallback callback)
{
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    iTerminateString(request->SessionKey, MEDIUS_SESSIONKEY_MAXLEN);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    GetMyClansCallbackFunctionPtr = callback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686D4C,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode MediusConfirmClanTeamChallenge(MediusConfirmClanTeamChallengeRequest *request, MediusCallback callback)
{
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    iTerminateString(request->SessionKey, MEDIUS_SESSIONKEY_MAXLEN);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    ConfirmClanTeamChallengeCallbackFunctionPtr = callback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686D5C,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode MediusGetAllClanMessages(MediusGetAllClanMessagesRequest *request, MediusCallback callback)
{
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    iTerminateString(request->SessionKey, MEDIUS_SESSIONKEY_MAXLEN);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    GetAllClanMessagesCallbackFunctionPtr = callback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686D64,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode MediusGetClanTeamChallengeHistory(MediusGetClanTeamChallengeHistoryRequest *request, MediusCallback callback)
{
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    iTerminateString(request->SessionKey, MEDIUS_SESSIONKEY_MAXLEN);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    GetClanTeamChallengeHistoryCallbackFunctionPtr = callback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686D6C,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode MediusUpdateClanStats(MediusUpdateClanStatsRequest *request, MediusCallback callback)
{
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    iTerminateString(request->SessionKey, MEDIUS_SESSIONKEY_MAXLEN);
    iTerminateString(request->Stats, MEDIUS_CLANSTATS_MAXLEN);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    UpdateClanStatsCallbackFunctionPtr = callback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686D74,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode MediusGetClanMemberList(MediusClanMembersRequest *request, MediusCallback callback)
{
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    iTerminateString(request->SessionKey, MEDIUS_SESSIONKEY_MAXLEN);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    GetClanMemberListCallbackFunctionPtr = callback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686D9C,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode func_006467A8(MediusGetClanLadderListRequest *request, MediusCallback callback)
{
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    data_00686AC4 = callback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686E00,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode func_00646838(MediusClanLadderPositionRequest *request, MediusCallback callback)
{
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    data_00686AC8 = callback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686E08,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode MediusGetClanMemberList_ExtraInfo(MediusGetClanMemberListRequest *request, MediusCallback callback)
{
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    data_00686ACC = callback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686E28,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode func_00646958(MediusInvitePlayerToClanByNameRequest *request, MediusCallback callback)
{
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    iTerminateString(request->InviteMessage, MEDIUS_CLANMSG_MAXLEN);
    iTerminateString(request->PlayerName, MEDIUS_ACCOUNTNAME_MAXLEN);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    InvitePlayerToClanCallbackFunctionPtr = callback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686E48,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

