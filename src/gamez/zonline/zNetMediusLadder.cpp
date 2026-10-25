/* zNetMediusLadder.cpp -- the lobby's ladders: the player's ranking and position, the clan
 * ladder's leaders and list (SOCOM 1's zNetMedius.cpp's ladder code; the unit and its name are
 * ours: OVL1 0x2EA5C0 to about 0x2ED45F, after the clan unit; its static initialiser 0x401FA0
 * builds the four ladder lists 0x44F7B0, 0x44F7E0, 0x44F820 and 0x44F850, whose destructors
 * 0x2ED360 and 0x2ED3E0 close it). Only some of it so far. Defined in reverse of their image
 * order (-inline deferred). */

#include <string.h>
#include "zonline.h"

/* a ladder position request: no session key, the account at 0x18 (SOCOM 1's
 * MediusLadderPositionRequest) */
struct MediusLadderPositionRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0x0
    int AccountID;   // 0x18
    int LadderStatIndex;   // 0x1c
    int SortOrder;   // 0x20
};   // 0x24

/* a clan ladder request, its message id naming the list it fills */
struct MediusGetClanLadderListRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0x0
    int ClanLadderStatIndex;   // 0x18
    int SortOrder;   // 0x1c
    int StartPosition;   // 0x20
    int PageSize;   // 0x24
};   // 0x28

struct MediusLadderPositionResponse { MEDIUS_RESPONSE_HEAD };
struct MediusGetClanLadderListResponse { MEDIUS_RESPONSE_HEAD };

/* the account logged in (OVL2 bss; SOCOM 1's AccountInfo) */
struct AccountInfoType {
    int AccountID;   // 0x0
    char AccountName[32];   // 0x4
};

/* a ladder's list (only what is passed on) */
struct LadderList;

void cbGetLadderRanking(int, int, int, MediusLadderPositionResponse *);
void cbGetLadderPosition(int, int, int, MediusLadderPositionResponse *);
void cbGetClanLadderList(MediusErrorCode, MediusErrorCode, MediusErrorCode,
                         MediusGetClanLadderListResponse *);

extern "C" {
extern AccountInfoType AccountInfo;
extern LadderList data_0044F820;   /* the clan ladder's leaders */

void func_002EC060(LadderList *list);   /* a clan ladder list emptied */
MediusErrorCode MediusLadderPosition(MediusLadderPositionRequest *req,
                                     void (*cb)(int, int, int, MediusLadderPositionResponse *));
MediusErrorCode func_006467A8(MediusGetClanLadderListRequest *req,
                              void (*cb)(MediusErrorCode, MediusErrorCode, MediusErrorCode,
                                         MediusGetClanLadderListResponse *));   /* MediusGetClanLadderList? */
}

/* our position on the ladder, in the active lobby (SOCOM 1's; OVL1 0x2EB2C0) */
void CZOnlineLobby::GetLadderInfo()
{
    if (m_isopen && m_current_state == LOBBY_ACTIVE) {
        MediusLadderPositionRequest req;

        memset(&req, 0, sizeof(req));
        req.SortOrder = 1;
        req.AccountID = AccountInfo.AccountID;
        req.LadderStatIndex = 0;
        CheckMediusError(MediusLadderPosition(&req, cbGetLadderPosition));
    }
}

/* our ranking (SOCOM 1's; OVL1 0x2EAFB0) */
void CZOnlineLobby::GetLadderRanking()
{
    MediusLadderPositionRequest req;

    memset(&req, 0, sizeof(req));
    req.SortOrder = 1;
    req.AccountID = AccountInfo.AccountID;
    req.LadderStatIndex = 0;
    CheckMediusError(MediusLadderPosition(&req, cbGetLadderRanking));
}

/* the clan ladder's leaders emptied and the first n asked for (SOCOM II's; OVL1 0x2EA6B0) */
void CZOnlineLobby::GetClanLadderLeaders(int n)
{
    func_002EC060(&data_0044F820);
    MediusGetClanLadderListRequest req;

    memset(&req, 0, sizeof(req));
    strncpy(req.MessageID, "ClanLadderLeaders", MEDIUS_MESSAGEID_MAXLEN);
    req.ClanLadderStatIndex = 0;
    req.SortOrder = 1;
    req.StartPosition = 1;
    req.PageSize = n;
    CheckMediusError(func_006467A8(&req, cbGetClanLadderList));
}
