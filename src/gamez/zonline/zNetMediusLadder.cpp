/* zNetMediusLadder.cpp -- the lobby's ladders: the player's ranking and position, the clan
 * ladder's leaders and list (SOCOM 1's zNetMedius.cpp's ladder code; the unit and its name are
 * ours: OVL1 0x2EA5C0 to about 0x2ED45F, after the clan unit; its static initialiser 0x401FA0
 * builds the four ladder lists 0x44F7B0, 0x44F7E0, 0x44F820 and 0x44F850, whose destructors
 * 0x2ED360 and 0x2ED3E0 close it). Only some of it so far. Defined in reverse of their image
 * order (-inline deferred). */

#include <string.h>
#include <stdio.h>
#include "zonline.h"
#include "../zmath/zmath.h"

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

struct MediusLadderPositionResponse {
    MEDIUS_RESPONSE_HEAD
    int LadderPosition;   // 0x1c
    int TotalRankings;   // 0x20
};

/* a page of the player ladder (no session key; the message id names the list: TheBest, TheRest) */
struct MediusLadderListRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0x0
    int LadderStatIndex;   // 0x18
    int SortOrder;   // 0x1c
    int StartPosition;   // 0x20
    int PageSize;   // 0x24
};   // 0x28

struct MediusLadderListResponse;

/* a valve (zanim.h's CValve, as the other lobby units declare it again) */
enum VALVE_TYPE {
    VALVE_TYPE_0,
    VALVE_TYPE_1
};

enum VALVE_STATE {
    VALVE_SAME,
    VALVE_ON,
    VALVE_OFF
};

class CValve {
public:
    static CValve *Create(const char *name, VALVE_TYPE type);
    void MakeCallbacks(VALVE_STATE state);
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
    unsigned short m_value;   // 0x4
    unsigned short m_flags;   // 0x6
};

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
void func_002F4080(void *msg, int type);   /* a request put into the network's order (empty) */

MediusErrorCode MediusLadderPosition(MediusLadderPositionRequest *req,
                                     void (*cb)(int, int, int, MediusLadderPositionResponse *));
MediusErrorCode func_006467A8(MediusGetClanLadderListRequest *req,
                              void (*cb)(MediusErrorCode, MediusErrorCode, MediusErrorCode,
                                         MediusGetClanLadderListResponse *));   /* MediusGetClanLadderList? */
}

extern "C" {
extern int data_0044F7F8;   /* our position on the ladder (cbGetLadderPosition's) */
extern int data_0044F800;   /* the ladder's positions */
extern unsigned char data_0044F880;   /* a clan ladder list asked for and not yet shown */
extern int data_0044F868;   /* our clan's position */
extern int data_0044F870;   /* the clan ladder's positions */
}

/* the clan's record (the clan unit's StoredClanInfo) */
struct LadderClanInfo {
    unsigned char _0[0x1c];
    int ClanID;   // 0x1c
};
extern "C" LadderClanInfo StoredClanInfo;

/* a UI variable set to an integer, its watchers told when it changes (as zui.h's float
 * assignment; the name ours) */
inline void SetVarInt(CUIVariable *var, int i)
{
    if (var->type != 1 || var->integer != i) {
        var->DeleteChildren();
        var->type = 1;
        var->integer = i;
        var->UpdateDependents();
    }
}

/* our clan's position on the clan ladder asked for (SOCOM II's request: no session key) */
struct MediusClanLadderPositionRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0x0
    int ClanID;   // 0x18
    int ClanLadderStatIndex;   // 0x1c
    int SortOrder;   // 0x20
};   // 0x24

struct MediusClanLadderPositionResponse {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0x0
    int LadderPosition;   // 0x18
    int _1c;   // 0x1c
    MediusCallbackStatus StatusCode;   // 0x20
};

/* a ladder's size asked for (SOCOM II's request, the message id naming it) */
struct MediusGetTotalRankingsRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0x0
    int LadderType;   // 0x18: 1, the clans
};   // 0x1c

struct MediusGetTotalRankingsResponse {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0x0
    MediusCallbackStatus StatusCode;   // 0x18
    int Total;   // 0x1c
};

extern "C" {
extern int data_0044F800;   /* the player ladder's positions (GetRankTier's) */
MediusErrorCode func_00646838(MediusClanLadderPositionRequest *req,
                              void (*cb)(int, int, int, MediusClanLadderPositionResponse *));   /* the clan's ladder position */
MediusErrorCode func_0064AEE8(MediusGetTotalRankingsRequest *req,
                              void (*cb)(int, int, int, MediusGetTotalRankingsResponse *));   /* a ladder's size */
}

/* our clan's position come: kept, shown, made the list's lowest, and the list around it asked for
 * when the request was GetSurrounding's (SOCOM II's; OVL1 0x2EAC70; the name ours) */
void cbGetClanLadderPosition(int, int, int, MediusClanLadderPositionResponse *resp)
{
    if (CheckMediusCallbackStatus("GetClanLadderRanking", resp->StatusCode)) {
        CUIVariable *var = theUIVarManager.Add("ClanLadderRanking", 0, UIVL_TEMPORARY);

        data_0044F868 = resp->LadderPosition;
        SetVarInt(var, data_0044F868);
        ClanLadderRanking.m_lowest = data_0044F868;
        if (!strcasecmp(resp->MessageID, "GetSurrounding")) {
            data_0044F880 = 0;
            theLobby.GetClanLadderList(data_0044F868 - 2 <= 0 ? 1 : data_0044F868, 7);
        }
    }
}

/* a ladder's size come: the players' kept, or the clans' kept and shown (SOCOM II's; OVL1
 * 0x2EB1B0; the name ours) */
void cbGetTotalRankings(int, int, int, MediusGetTotalRankingsResponse *resp)
{
    if (CheckMediusCallbackStatus("GetTotalRankings", resp->StatusCode)) {
        if (!strcmp(resp->MessageID, "PlayerTotal")) {
            data_0044F800 = resp->Total;
            LadderLeaders.m_10 = data_0044F800;
        } else if (!strcmp(resp->MessageID, "ClanTotal")) {
            data_0044F870 = resp->Total;
            SetVarInt(theUIVarManager.Add("TotalClans", 0, UIVL_TEMPORARY), data_0044F870);
            ClanLadderRanking.m_lowest = data_0044F870;
        }
    }
}

/* our clan's ranking asked for -- its position (n: and the clans around it), or, outside a clan,
 * the ladder's top -- and the clans' count (SOCOM II's; OVL1 0x2EAB80) */
void CZOnlineLobby::GetClanRanking(int n)
{
    if (IsMemberOfClan()) {
        MediusClanLadderPositionRequest req;

        memset(&req, 0, sizeof(req));
        if (n)
            strncpy(req.MessageID, "GetSurrounding", MEDIUS_MESSAGEID_MAXLEN);
        req.ClanID = StoredClanInfo.ClanID;
        req.ClanLadderStatIndex = 0;
        req.SortOrder = 1;
        CheckMediusError(func_00646838(&req, cbGetClanLadderPosition));
    } else {
        data_0044F868 = 0;
        theLobby.GetClanLadderList(1, 7);
    }
    MediusGetTotalRankingsRequest total;

    memset(&total, 0, sizeof(total));
    memcpy(total.MessageID, "ClanTotal", MEDIUS_MESSAGEID_MAXLEN);
    total.LadderType = 1;
    CheckMediusError(func_0064AEE8(&total, cbGetTotalRankings));
}

/* the clan ladder's order (std::sort's comparator, empty, by value) */
struct ClanLadderLess {
    ClanLadderLess() {}
};
extern "C" void func_002EC7E0(ClanLadderList::iterator first, ClanLadderList::iterator last,
                              ClanLadderLess less);   /* std::sort's instance */

void UpdateClanLadderLeaders();
void UpdateClanLadderList();

/* the clan ladder's answer: a clan of the leaders or of the list around us kept, the list shown
 * once the last has come (SOCOM II's; OVL1 0x2EA730) */
void cbGetClanLadderList(MediusErrorCode, MediusErrorCode, MediusErrorCode,
                         MediusGetClanLadderListResponse *resp)
{
    if (CheckMediusCallbackStatus("GetClanLadderList", resp->StatusCode)) {
        if (!strcasecmp(resp->MessageID, "ClanLadderLeaders")) {
            ClanLadderLeaders.Add(resp);
            if (resp->EndOfList)
                UpdateClanLadderLeaders();
        } else if (!strcasecmp(resp->MessageID, "ClanLadderList")) {
            ClanLadderRanking.Add(resp);
            if (resp->EndOfList) {
                UpdateClanLadderList();
                data_0044F880 = 0;
            }
        }
    } else
        data_0044F880 = 0;
}

/* the clan ladder's leaders sorted and handed to the menus: each one's position and name
 * (SOCOM II's; OVL1 0x2EA800) */
void UpdateClanLadderLeaders()
{
    CRdrEditor ed;

    func_002EC7E0(ClanLadderLeaders.begin(), ClanLadderLeaders.end(), ClanLadderLess());
    ed.push(0);
    for (ClanLadderList::iterator i = ClanLadderLeaders.begin(); i != ClanLadderLeaders.end(); ++i) {
        char pos[32];

        sprintf(pos, "%d", (*i)->LadderPosition);
        ed.push(0);
        ed.addstringnode(pos);
        ed.addstringnode((*i)->ClanName);
        ed.pop();
    }
    ed.pop();
    theLobby.uiv_ClanLadderLeaders->Set(&ed);
}

/* the clan ladder around us sorted and handed to the menus: each one's position, name and tier,
 * our clan's in colour; the menus told (SOCOM II's; OVL1 0x2EA930) */
void UpdateClanLadderList()
{
    CRdrEditor ed;

    func_002EC7E0(ClanLadderRanking.begin(), ClanLadderRanking.end(), ClanLadderLess());
    ed.push(0);
    for (ClanLadderList::iterator i = ClanLadderRanking.begin(); i != ClanLadderRanking.end(); ++i) {
        char pos[32];

        sprintf(pos, "%d", (*i)->LadderPosition);
        int tier = theLobby.GetClanRankTier((*i)->LadderPosition);

        ed.push(0);
        if (StoredClanInfo.ClanID == (*i)->ClanID) {
            PNT3D color;

            color.x = 91.0f;
            color.y = 72.0f;
            color.z = 36.0f;
            ed.push(0);
            ed.addSTRING("VALUE", pos);
            ed.addPNT3D("COLOR", color);
            ed.pop();
            ed.push(0);
            ed.addSTRING("VALUE", (*i)->ClanName);
            ed.addPNT3D("COLOR", color);
            ed.pop();
            ed.addintnode(tier);
        } else {
            ed.addstringnode(pos);
            ed.addstringnode((*i)->ClanName);
            ed.addintnode(tier);
        }
        ed.pop();
    }
    ed.pop();
    theLobby.uiv_ClanLadderRanking->Set(&ed);
    LobbyEvent("ON_LADDER_REFRESH", 0);
}

/* the clan ladder around a position asked for, twice n clans from it, unless one is on its way or
 * the list already reaches it (SOCOM II's; OVL1 0x2EA5C0: the ladder pages, GetClanRanking) */
void CZOnlineLobby::GetClanLadderList(int start, int n)
{
    if (data_0044F880)
        return;
    if (!((start < ClanLadderRanking.m_lowest || !ClanLadderRanking.size()) && start > 0))
        return;
    data_0044F880 = 1;
    MediusGetClanLadderListRequest req;

    memset(&req, 0, sizeof(req));
    strncpy(req.MessageID, "ClanLadderList", MEDIUS_MESSAGEID_MAXLEN);
    req.SortOrder = 1;
    req.ClanLadderStatIndex = 0;
    ClanLadderRanking.DeleteAll();
    ClanLadderRanking.m_10 = start;
    req.StartPosition = start;
    req.PageSize = n * 2;
    func_002F4080(&req, 123);
    CheckMediusError(func_006467A8(&req, cbGetClanLadderList));
}

/* a clan ladder position's tier, ours for -1: 4 the top 5 %, 3 to 30 %, 2 to 85 %, 1 the rest
 * (SOCOM II's; OVL1 0x2EAD80; the name ours) */
int CZOnlineLobby::GetClanRankTier(int rank)
{
    int tier = 0;

    if (rank == -1)
        rank = data_0044F868;
    if (data_0044F870 > 0) {
        int permille = rank * 1000 / data_0044F870;

        if (permille < 51)
            tier = 4;
        else if (permille < 301)
            tier = 3;
        else if (permille < 851)
            tier = 2;
        else
            tier = 1;
    }
    return tier;
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

extern "C" {
extern unsigned char data_0044F878;   /* a player ladder page asked for and not yet come */
void func_002EC140(LadderList *list);   /* a player ladder's items deleted (LadderList::DeleteAll) */
void func_002EB6A0(int, int, int, MediusLadderListResponse *);   /* the pages' callback */
MediusErrorCode func_0064B128(MediusLadderListRequest *req,
                              void (*cb)(int, int, int, MediusLadderListResponse *));   /* MediusLadderList */
}

/* the player ladder's order (std::sort's comparator, empty, by value) */
struct LadderLess {
    LadderLess() {}
};
extern "C" void func_002EC260(LadderList::iterator first, LadderList::iterator last,
                              LadderLess less);   /* std::sort's instance */

/* the player ladder's best sorted and handed to the menus: each one's position, name and clan
 * (SOCOM II's; OVL1 0x2EB7A0; the name ours).
 * OPEN, 0x164 against 0x15C: the target reads each name's words off the item (0x8C/0x8D/0x94,
 * 0x98/0x99/0xA0) where include/string's inline c_str() keeps the string's own address in a
 * register (item + 0x98, then +1, +8) -- the MSL string's c_str(), or a different member type */
void UpdateLadderBestDisplay()
{
    CRdrEditor ed;

    func_002EC260(LadderRanking.begin(), LadderRanking.end(), LadderLess());
    ed.push(0);
    for (LadderList::iterator i = LadderRanking.begin(); i != LadderRanking.end(); ++i) {
        char pos[32];

        sprintf(pos, "%d", (*i)->m_position);
        ed.push(0);
        ed.addstringnode(pos);
        ed.addstringnode((*i)->m_name.c_str());
        ed.addstringnode((*i)->m_clan.c_str());
        ed.pop();
    }
    ed.pop();
    theLobby.uiv_MediusLadderTop10->Set(&ed);
}

/* the player ladder's best n asked for (SOCOM II's; OVL1 0x2EB610; the name ours) */
void CZOnlineLobby::GetLadderBest(int n)
{
    func_002EC140(&LadderRanking);
    MediusLadderListRequest req;

    memset(&req, 0, sizeof(req));
    memcpy(req.MessageID, "TheBest", MEDIUS_MESSAGEID_MAXLEN);
    req.SortOrder = 1;
    req.StartPosition = 1;
    req.PageSize = n;
    req.LadderStatIndex = 0;
    func_002F4080(&req, 131);
    CheckMediusError(func_0064B128(&req, func_002EB6A0));
}

/* the player ladder around a position asked for, twice n players from it, unless a page is on its
 * way or the list already reaches it (SOCOM II's; OVL1 0x2EB510; the name ours) */
void CZOnlineLobby::GetLadderList(int start, int n)
{
    if (data_0044F878)
        return;
    if (!((start < LadderLeaders.m_10 || !LadderLeaders.size()) && start > 0))
        return;
    int first = start;

    if (start <= 0)
        first = 1;

    func_002EC140(&LadderLeaders);
    LadderLeaders.m_c = first;
    MediusLadderListRequest req;

    memset(&req, 0, sizeof(req));
    memcpy(req.MessageID, "TheRest", MEDIUS_MESSAGEID_MAXLEN);
    req.PageSize = n * 2;
    req.StartPosition = first;
    req.SortOrder = 1;
    req.LadderStatIndex = 0;
    func_002F4080(&req, 131);
    MediusErrorCode err = func_0064B128(&req, func_002EB6A0);

    data_0044F878 = 1;
    CheckMediusError(err);
}

/* our position on the ladder come: kept, shown, the list around it asked for, the medal and the
 * rank restriction's valves set (SOCOM 1's; OVL1 0x2EB340) */
void cbGetLadderPosition(int, int, int, MediusLadderPositionResponse *resp)
{
    if (CheckMediusCallbackStatus("GetLadderPosition", resp->StatusCode)) {
        data_0044F7F8 = resp->LadderPosition;
        data_0044F800 = resp->TotalRankings;
        LadderLeaders.m_10 = data_0044F800;
        SetVarInt(theUIVarManager.Add("LadderRanking", 0, UIVL_TEMPORARY), data_0044F7F8);
        data_0044F878 = 0;
        theLobby.GetLadderList(data_0044F7F8 - 2 <= 0 ? 1 : data_0044F7F8 - 2, 7);
        CValve::Create("LadderMedalOfMerit", VALVE_TYPE_1)->SetValue(theLobby.GetRankLevel(-1));
        CValve::Create("LadderRankRestriction", VALVE_TYPE_1)->SetValue(theLobby.GetRankTier(-1));
    } else
        theLobby.GetLadderList(1, 7);
}

/* our ranking come: kept and shown, the medal and the rank restriction's valves set, the menus
 * told (SOCOM 1's; OVL1 0x2EB010) */
void cbGetLadderRanking(int, int, int, MediusLadderPositionResponse *resp)
{
    if (CheckMediusCallbackStatus("GetLadderRanking", resp->StatusCode)) {
        data_0044F7F8 = resp->LadderPosition;
        data_0044F800 = resp->TotalRankings;
        LadderLeaders.m_10 = data_0044F800;
        SetVarInt(theUIVarManager.Add("LadderRanking", 0, UIVL_TEMPORARY), data_0044F7F8);
        CValve::Create("LadderMedalOfMerit", VALVE_TYPE_1)->SetValue(theLobby.GetRankLevel(-1));
        CValve::Create("LadderRankRestriction", VALVE_TYPE_1)->SetValue(theLobby.GetRankTier(-1));
        LobbyEvent("ON_LADDER_REFRESH", 0);
    }
}

/* a ladder position's rank, ours for -1: 5 the first ten, then GetRankTier's tiers; on a ladder of
 * fewer than 12, 5 for anyone (SOCOM II's; OVL1 0x2EAEE0; the name ours) */
int CZOnlineLobby::GetRankLevel(int rank)
{
    int level = 0;

    if (rank == -1)
        rank = data_0044F7F8;
    if (data_0044F800 > 11) {
        int permille = rank * 1000 / (data_0044F800 - 10);

        if (rank < 11)
            level = 5;
        else if (permille < 51)
            level = 4;
        else if (permille < 301)
            level = 3;
        else if (permille < 851)
            level = 2;
        else
            level = 1;
    } else if (data_0044F800 > 0)
        level = 5;
    return level;
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
