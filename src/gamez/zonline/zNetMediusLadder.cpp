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

/* the account logged in (OVL2 bss; SOCOM 1's AccountInfo) */
struct AccountInfoType {
    int AccountID;   // 0x0
    char AccountName[32];   // 0x4
};

void cbGetLadderRanking(int, int, int, MediusLadderPositionResponse *);
void cbGetLadderPosition(int, int, int, MediusLadderPositionResponse *);
void cbGetClanLadderList(MediusErrorCode, MediusErrorCode, MediusErrorCode,
                         MediusGetClanLadderListResponse *);

extern "C" {
extern AccountInfoType AccountInfo;

MediusErrorCode MediusLadderPosition(MediusLadderPositionRequest *req,
                                     void (*cb)(int, int, int, MediusLadderPositionResponse *));
MediusErrorCode func_006467A8(MediusGetClanLadderListRequest *req,
                              void (*cb)(MediusErrorCode, MediusErrorCode, MediusErrorCode,
                                         MediusGetClanLadderListResponse *));   /* MediusGetClanLadderList? */
}

/* The clan ladders' list members, defined first: they follow the unit's own code in the image
 * (0x2EBCF0, 0x2EC060), among the ladders' other members. Pinned: never inlined. */
#pragma auto_inline off

/* the clans deleted and the list emptied (OVL1 0x2EC060) */
void ClanLadderList::DeleteAll()
{
    std::transform(begin(), end(), begin(), DeleteItem<ClanLadderItem>());
    clear();
}

/* a clan kept, unless its position is there, and the lowest position kept (OVL1 0x2EBCF0): the
 * pushed pointer read back from the frame, its address having been taken */
void ClanLadderList::Add(MediusGetClanLadderListResponse *resp)
{
    if (!Find(resp->LadderPosition)) {
        ClanLadderItem *item = new ClanLadderItem(*resp);

        push_back(item);
        if (m_lowest < item->LadderPosition)
            m_lowest = item->LadderPosition;
    }
}

#pragma auto_inline reset

extern "C" {
extern int data_0044F7F8;   /* our position on the ladder (cbGetLadderPosition's) */
extern int data_0044F800;   /* the ladder's positions */
}

/* a ladder position's tier, ours for -1: 4 the top 5 %, 3 to 30 %, 2 to 85 %, 1 the rest (of the
 * positions past the first ten); on a ladder of fewer than 12, 4 for anyone (SOCOM II's; OVL1
 * 0x2EAE20: the ladder callbacks, the game list and CanJoinRoom ask it; the name ours) */
int CZOnlineLobby::GetRankTier(int rank)
{
    int tier = 0;

    if (rank == -1)
        rank = data_0044F7F8;
    if (data_0044F800 > 11) {
        int permille = rank * 1000 / (data_0044F800 - 10);

        if (permille < 51)
            tier = 4;
        else if (permille < 301)
            tier = 3;
        else if (permille < 851)
            tier = 2;
        else
            tier = 1;
    } else if (data_0044F800 > 0)
        tier = 4;
    return tier;
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
    ClanLadderLeaders.DeleteAll();
    MediusGetClanLadderListRequest req;

    memset(&req, 0, sizeof(req));
    strncpy(req.MessageID, "ClanLadderLeaders", MEDIUS_MESSAGEID_MAXLEN);
    req.ClanLadderStatIndex = 0;
    req.SortOrder = 1;
    req.StartPosition = 1;
    req.PageSize = n;
    CheckMediusError(func_006467A8(&req, cbGetClanLadderList));
}
