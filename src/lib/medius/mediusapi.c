/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* The Medius client API (OVL2 0x646A00-0x64B3F8, SOCOM 1's object at 0x3B20E8:
 * MediusSetNatServiceAddress .. MediusGetTotalChannels), SCEA's Medius client library ("Medius
 * Client Library Version 1.50.0013"), prebuilt with ee-gcc 2.96 at -O2 -G0 (README "The GCC
 * front"). The object defines the library's state -- GlobalInitFlag a halfword, as SOCOM 1's
 * symbol is (2 bytes), and the requests' callback slots (0x6561C0-0x6562E0) -- and its requests
 * all go one way (as the clan API's, mediusclan.c): strings terminated, initialised and connected,
 * the callback kept in the request's slot, the message sent to the lobby server. SOCOM 1's slots
 * (*CallbackFunctionPtr, data 0x43B958-0x43BA20) are SOCOM II's in the same order with SOCOM II's
 * new requests inserted (three after GetPolicy, one after ChatToggleFilter, the rest after
 * GetTotalChannels), and so are its functions: each name is SOCOM 1's by position and by slot
 * (symbols.txt says which), or the online lane's by call site. One of the online lane's names sits
 * on a request new in SOCOM II where SOCOM 1's function is another row: MediusLadderPosition
 * (0x64B1B8; SOCOM 1's, by position and slot, is 0x64A018) -- the map keeps it, the game's source
 * calls it by that name (mediusclan.c has the same case). Requests SOCOM 1 does not have keep
 * placeholders; their request types
 * are named after them, laid out as far as the code shows (strings terminated, sizeof sent). Only
 * the functions written so far are here, in image order. */

#include "medius.h"

extern short GlobalInitFlag;

typedef struct NetAddress {
    int AddressType;
    char Address[16];
    unsigned short Port;
} NetAddress;                           /* 24 bytes */

typedef struct NetAddressList {
    NetAddress aAddressList[2];
} NetAddressList;                       /* 48 bytes */

extern NetAddress gstNatServiceAddress;
extern NetAddressList gstClientAddressList;
extern int data_006562F8;               /* the message counter */

extern int iCreateAndAllocateMessageID(char *key, int counter, int type, char *out);

/* the universe's information: its status list and news, each with its own callback (the online
 * lane's zNetMediusUniverse.cpp passes cbGetUniverseNews and cbGetUniverseStatusList to
 * 0x64AD98); the store order is the one that schedules as the target does (ccperm, 1 of 120) */
typedef struct MediusGetUniverseInformationRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
    int InfoType;
    int CharacterEncoding;
    int Language;
} MediusGetUniverseInformationRequest;

extern int data_006561AC;               /* the information asked for */
extern int data_006561B0;
extern int data_006561B4;
extern MediusCallback data_006562AC;    /* the news */
extern MediusCallback data_006562B0;
extern MediusCallback data_006562B8;    /* the status list */
extern int data_00686F04;
extern int data_00686F10;

typedef struct MediusVersionServerRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];
} MediusVersionServerRequest;

typedef struct MediusUpdateClientStateRequest {
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];
    int UserAction;
} MediusUpdateClientStateRequest;

typedef struct MediusSessionBeginRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
    int ConnectionClass;
} MediusSessionBeginRequest;

typedef struct MediusSessionEndRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];
} MediusSessionEndRequest;

typedef struct MediusSessionPost166 {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];
    char Data38[128];
} MediusSessionPost166;

typedef struct MediusSessionPost80 {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];
    char Data38[42];
} MediusSessionPost80;

typedef struct MediusAccountLoginRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];
    char AccountName[MEDIUS_ACCOUNTNAME_MAXLEN];
    char Password[MEDIUS_PASSWORD_MAXLEN];
} MediusAccountLoginRequest;

typedef struct MediusAccountLogoutRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];
} MediusAccountLogoutRequest;

typedef struct MediusAccountGetProfileRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];
} MediusAccountGetProfileRequest;

typedef struct MediusAccountRegistrationRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];
    int AccountType;
    char AccountName[MEDIUS_ACCOUNTNAME_MAXLEN];
    char Password[MEDIUS_PASSWORD_MAXLEN];
} MediusAccountRegistrationRequest;

typedef struct MediusAccountUpdatePasswordRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];
    char String38[32];
    char String70[32];
    char Data102[330];
} MediusAccountUpdatePasswordRequest;

typedef struct MediusAccountUpdateStatsRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];
    char Stats[MEDIUS_ACCOUNTSTATS_MAXLEN];
} MediusAccountUpdateStatsRequest;

typedef struct MediusAccountUpdateProfileRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];
    char Data38[6];
    char String44[32];
    char String76[32];
    char String108[32];
    char String140[32];
    char String172[32];
    char String204[32];
    char String236[32];
    char String268[3];
    char String271[32];
    char String303[32];
    char String335[16];
    char String351[80];
    char Data431[1];
} MediusAccountUpdateProfileRequest;

typedef struct MediusAccountDeleteRequest {
    char Data0[70];
} MediusAccountDeleteRequest;

typedef struct MediusAnonymousLoginRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];
    char SessionDisplayName[MEDIUS_ACCOUNTNAME_MAXLEN];
    char SessionDisplayStats[MEDIUS_ACCOUNTSTATS_MAXLEN];
} MediusAnonymousLoginRequest;

typedef struct MediusCreateGameRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];
    char Data38[18];
    char String56[64];
    char String120[32];
    char String152[32];
    char Data184[24];
} MediusCreateGameRequest;

typedef struct MediusTextFilterRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];
    int TextFilterType;
    char Text[64];
} MediusTextFilterRequest;

typedef struct MediusCreateChannelRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];
    char Data38[10];
    char String48[64];
    char String112[32];
    char Data144[4];
} MediusCreateChannelRequest;

typedef struct MediusJoinChannelRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];
    int MediusWorldID;
    char LobbyChannelPassword[32];
} MediusJoinChannelRequest;

typedef struct MediusGetGamesRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];
    char Data38[10];
} MediusGetGamesRequest;

typedef struct MediusGetChannelsRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];
    char Data38[4];
} MediusGetChannelsRequest;

typedef struct MediusGetLobbyPlayersRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];
    char Data38[6];
} MediusGetLobbyPlayersRequest;

typedef struct MediusGetGamePlayersRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];
    int MediusWorldID;
} MediusGetGamePlayersRequest;

typedef struct MediusGetPlayerInfoRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];
    int AccountID;
} MediusGetPlayerInfoRequest;

typedef struct MediusGetGameInfoRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];
    int MediusWorldID;
} MediusGetGameInfoRequest;

typedef struct MediusGetChannelInfoRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];
    int MediusWorldID;
} MediusGetChannelInfoRequest;

typedef struct MediusFindPlayerRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];
    int SearchType;
    int ID;
    char Name[MEDIUS_ACCOUNTNAME_MAXLEN];
} MediusFindPlayerRequest;

typedef struct MediusFindWorldByNameRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];
    char Name[64];
    int WorldType;
} MediusFindWorldByNameRequest;

typedef struct MediusChatMessage {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];
    int MessageType;
    int TargetID;
    char Message[64];
} MediusChatMessage;

typedef struct MediusGetBuddyListRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];
} MediusGetBuddyListRequest;

typedef struct MediusAddToBuddyListRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];
    int AccountID;
} MediusAddToBuddyListRequest;

typedef struct MediusRemoveFromBuddyListRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];
    int AccountID;
} MediusRemoveFromBuddyListRequest;

typedef struct MediusBuddyGetPermissionRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];
    char Data38[10];
} MediusBuddyGetPermissionRequest;

typedef struct MediusBuddyAddConfirmationRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
    char Data21[7];
    char String28[17];
    char Data45[11];
} MediusBuddyAddConfirmationRequest;

typedef struct MediusGetPolicyRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];
} MediusGetPolicyRequest;

typedef struct MediusRequest_006494A0 {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];
    char Data38[6];
} MediusRequest_006494A0;

typedef struct MediusRequest_00649540 {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];
    char Data38[6];
} MediusRequest_00649540;

typedef struct MediusRequest_006495E0 {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];
    char Data38[6];
} MediusRequest_006495E0;

typedef struct MediusPlayerReport {
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];
    char Data17[263];
} MediusPlayerReport;

typedef struct MediusWorldReport {
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];
    char Data17[11];
    char String28[64];
    char Data92[292];
} MediusWorldReport;

typedef struct MediusEndGameReport {
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];
    char Data17[7];
    char String24[64];
    char String88[32];
    char Data120[4];
} MediusEndGameReport;

typedef struct MediusGetAnnouncementsRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];
    char Data38[6];
} MediusGetAnnouncementsRequest;

typedef struct MediusGetAllAnnouncementsRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];
    char Data38[6];
} MediusGetAllAnnouncementsRequest;

typedef struct MediusSetGameListFilterRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];
    int FilterField;
    int ComparisonOperator;
    int BaselineValue;
} MediusSetGameListFilterRequest;

typedef struct MediusClearGameListFilterRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];
    char Data38[6];
} MediusClearGameListFilterRequest;

typedef struct MediusGetGameListFilterRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];
} MediusGetGameListFilterRequest;

typedef struct MediusGetMyIPRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];
} MediusGetMyIPRequest;

typedef struct MediusGetWorldSecurityLevelRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];
    char Data38[10];
} MediusGetWorldSecurityLevelRequest;

typedef struct MediusBanPlayerRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];
    char Data38[18];
} MediusBanPlayerRequest;

typedef struct MediusAccountGetIDRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];
    char AccountName[MEDIUS_ACCOUNTNAME_MAXLEN];
} MediusAccountGetIDRequest;

typedef struct MediusGetLocationsRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];
} MediusGetLocationsRequest;

typedef struct MediusPickLocationRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];
    char Data38[6];
} MediusPickLocationRequest;

typedef struct MediusLadderPositionRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];
    char Data38[18];
} MediusLadderPositionRequest;

typedef struct MediusLadderListRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];
    char Data38[10];
} MediusLadderListRequest;

typedef struct MediusChatToggleRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];
    char Data38[22];
} MediusChatToggleRequest;

typedef struct MediusRequest_0064A1F8 {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];
    char Data38[6];
} MediusRequest_0064A1F8;

typedef struct MediusGetTotalGamesRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];
    char Data38[6];
} MediusGetTotalGamesRequest;

typedef struct MediusGetTotalChannelsRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];
    char Data38[6];
} MediusGetTotalChannelsRequest;

typedef struct MediusRequest_0064AA58 {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];
    char Data38[6];
} MediusRequest_0064AA58;

typedef struct MediusRequest_0064AAF8 {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];
    char Data38[6];
} MediusRequest_0064AAF8;

typedef struct MediusRequest_0064AB98 {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];
    char Data38[10];
} MediusRequest_0064AB98;

typedef struct MediusRequest_0064AC38 {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];
    char Data38[10];
} MediusRequest_0064AC38;

typedef struct MediusRequest_0064AE58 {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
} MediusRequest_0064AE58;

typedef struct MediusRequest_0064AEE8 {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
    char Data21[7];
} MediusRequest_0064AEE8;

typedef struct MediusRequest_0064AF78 {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
    char Data21[7];
} MediusRequest_0064AF78;

typedef struct MediusRequest_0064B008 {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
    char Data21[5];
} MediusRequest_0064B008;

typedef struct MediusRequest_0064B098 {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
    char Data21[5];
} MediusRequest_0064B098;

typedef struct MediusRequest_0064B128 {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
    char Data21[19];
} MediusRequest_0064B128;

typedef struct MediusLadderPositionExRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
    char Data21[15];
} MediusLadderPositionExRequest;

typedef struct MediusRequest_0064B248 {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
    char Data21[67];
} MediusRequest_0064B248;

typedef struct MediusRequest_0064B2D8 {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
    char Data21[7];
} MediusRequest_0064B2D8;

typedef struct MediusRequest_0064B368 {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
} MediusRequest_0064B368;

extern MediusCallback data_00656260;
#define VersionServerCallbackFunctionPtr data_00656260
extern int data_00686CAC;
extern int data_00686C7C;
extern MediusCallback data_006561C0;
#define SessionBeginCallbackFunctionPtr data_006561C0
extern int data_00686B34;
extern MediusCallback data_006561C4;
#define SessionEndCallbackFunctionPtr data_006561C4
extern int data_00686B3C;
extern int data_00686B44;
extern int data_00686B48;
extern MediusCallback data_006561CC;
#define AccountLoginCallbackFunctionPtr data_006561CC
extern int data_00686B54;
extern MediusCallback data_006561D0;
#define AccountLogoutCallbackFunctionPtr data_006561D0
extern int data_00686B5C;
extern MediusCallback data_006561E4;
#define AccountGetProfileCallbackFunctionPtr data_006561E4
extern int data_00686B6C;
extern MediusCallback data_006561D4;
#define AccountRegistrationCallbackFunctionPtr data_006561D4
extern int data_00686B64;
extern MediusCallback data_006561DC;
#define AccountUpdatePasswordCallbackFunctionPtr data_006561DC
extern int data_00686B7C;
extern MediusCallback data_006561E0;
#define AccountUpdateStatsCallbackFunctionPtr data_006561E0
extern int data_00686B84;
extern MediusCallback data_006561D8;
#define AccountUpdateProfileCallbackFunctionPtr data_006561D8
extern int data_00686B74;
extern MediusCallback data_006561E8;
#define AccountDeleteCallbackFunctionPtr data_006561E8
extern int data_00686B8C;
extern MediusCallback data_006561EC;
#define AnonymousLoginCallbackFunctionPtr data_006561EC
extern int data_00686B94;
extern MediusCallback data_006561F8;
#define CreateGameCallbackFunctionPtr data_006561F8
extern int data_00686BA0;
extern MediusCallback data_00656288;
#define MediusTextFilterCallbackFunctionPtr data_00656288
extern int data_00686DC4;
extern MediusCallback data_00656200;
#define CreateChannelCallbackFunctionPtr data_00656200
extern int data_00686BB0;
extern MediusCallback data_00656208;
#define JoinChannelCallbackFunctionPtr data_00656208
extern int data_00686BC8;
extern MediusCallback data_0065620C;
#define GameListCallbackFunctionPtr data_0065620C
extern int data_00686BD8;
extern MediusCallback data_00656210;
#define ChannelListCallbackFunctionPtr data_00656210
extern int data_00686BE0;
extern MediusCallback data_006561F0;
#define LobbyWorldPlayerListCallbackFunctionPtr data_006561F0
extern int data_00686BE8;
extern MediusCallback data_006561F4;
#define GameWorldPlayerListCallbackFunctionPtr data_006561F4
extern int data_00686BF0;
extern MediusCallback data_00656214;
#define PlayerInfoCallbackFunctionPtr data_00656214
extern int data_00686BF8;
extern MediusCallback data_00656218;
#define GameInfoCallbackFunctionPtr data_00656218
extern int data_00686C00;
extern MediusCallback data_0065621C;
#define ChannelInfoCallbackFunctionPtr data_0065621C
extern int data_00686C08;
extern MediusCallback data_00656224;
#define FindPlayerCallbackFunctionPtr data_00656224
extern int data_00686C18;
extern MediusCallback data_00656220;
#define FindWorldByNameCallbackFunctionPtr data_00656220
extern int data_00686C10;
extern int data_00686C20;
extern MediusCallback data_0065622C;
#define GetBuddyListCallbackFunctionPtr data_0065622C
extern int data_00686C28;
extern MediusCallback data_00656230;
#define AddToBuddyListCallbackFunctionPtr data_00656230
extern int data_00686C30;
extern MediusCallback data_00656234;
#define RemoveFromBuddyListCallbackFunctionPtr data_00656234
extern int data_00686C48;
extern MediusCallback data_0065623C;
#define BuddyGetPermissionCallbackFunctionPtr data_0065623C
extern int data_00686C50;
extern int data_00686C40;
extern MediusCallback data_00656240;
#define GetPolicyCallbackFunctionPtr data_00656240
extern int data_00686C5C;
extern MediusCallback data_00656244;
extern int data_00686C64;
extern MediusCallback data_00656248;
extern int data_00686C6C;
extern MediusCallback data_0065624C;
extern int data_00686C74;
extern int data_00686B28;
extern int data_00686B24;
extern int data_00686B2C;
extern MediusCallback data_00656250;
#define GetAnnouncementsCallbackFunctionPtr data_00656250
extern int data_00686C80;
extern int data_00686C84;
extern MediusCallback data_00656254;
#define SetGameListFilterCallbackFunctionPtr data_00656254
extern int data_00686C8C;
extern MediusCallback data_00656258;
#define ClearGameListFilterCallbackFunctionPtr data_00656258
extern int data_00686C94;
extern MediusCallback data_0065625C;
#define GetGameListFilterCallbackFunctionPtr data_0065625C
extern int data_00686C9C;
extern MediusCallback data_00656264;
#define GetMyIPCallbackFunctionPtr data_00656264
extern int data_00686CA4;
extern MediusCallback data_00656268;
#define GetWorldSecurityLevelCallbackFunctionPtr data_00656268
extern int data_00686D7C;
extern MediusCallback data_0065626C;
#define BanPlayerCallbackFunctionPtr data_0065626C
extern int data_00686D84;
extern MediusCallback data_006561C8;
#define AccountGetIDCallbackFunctionPtr data_006561C8
extern int data_00686B4C;
extern MediusCallback data_00656270;
#define GetLocationsCallbackFunctionPtr data_00656270
extern int data_00686D8C;
extern MediusCallback data_00656274;
#define PickLocationCallbackFunctionPtr data_00656274
extern int data_00686D94;
extern MediusCallback data_00656278;
#define MediusLadderPositionCallbackFunctionPtr data_00656278
extern int data_00686DA4;
extern MediusCallback data_0065627C;
#define MediusLadderListCallbackFunctionPtr data_0065627C
extern int data_00686DAC;
extern MediusCallback data_00656280;
#define MediusChatToggleCallbackFunctionPtr data_00656280
extern int data_00686DB4;
extern MediusCallback data_00656284;
extern int data_00686DBC;
extern MediusCallback data_00656290;
#define MediusGetTotalGamesCallbackFunctionPtr data_00656290
extern int data_00686DD0;
extern MediusCallback data_00656294;
#define MediusGetTotalChannelsCallbackFunctionPtr data_00656294
extern int data_00686DD8;
extern MediusCallback data_00656298;
extern int data_00686DE0;
extern MediusCallback data_0065629C;
extern int data_00686DE8;
extern MediusCallback data_006562A0;
extern int data_00686DF0;
extern MediusCallback data_006562A4;
extern int data_00686DF8;
extern MediusCallback data_006562BC;
extern int data_00686E10;
extern MediusCallback data_006562C0;
extern int data_00686E18;
extern MediusCallback data_006562C4;
extern int data_00686E20;
extern MediusCallback data_006562C8;
extern int data_00686E30;
extern MediusCallback data_006562CC;
extern int data_00686E38;
extern MediusCallback data_006562D0;
extern int data_00686E40;
extern MediusCallback data_006562D4;
extern int data_00686E5C;
extern MediusCallback data_006562D8;
extern int data_00686E4C;
extern MediusCallback data_006562DC;
extern int data_00686E54;
extern MediusCallback data_006562E0;
extern int data_00686E94;

void MediusSetNatServiceAddress(NetAddress addr)
{
    gstNatServiceAddress = addr;
}

void MediusGetNatServiceAddress(NetAddress *addr)
{
    *addr = gstNatServiceAddress;
}

void MediusGetClientAddrList(NetAddressList *list)
{
    *list = gstClientAddressList;
}

MediusErrorCode MediusVersionServer(MediusVersionServerRequest *request, MediusCallback callback)
{
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    VersionServerCallbackFunctionPtr = callback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686CAC,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode MediusUpdateClientState(MediusUpdateClientStateRequest *request)
{
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    iTerminateString(request->SessionKey, MEDIUS_SESSIONKEY_MAXLEN);
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686C7C,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode MediusSessionBegin(MediusSessionBeginRequest *request, MediusCallback callback)
{
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    SessionBeginCallbackFunctionPtr = callback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686B34,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode MediusSessionEnd(MediusSessionEndRequest *request, MediusCallback callback)
{
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    iTerminateString(request->SessionKey, MEDIUS_SESSIONKEY_MAXLEN);
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    SessionEndCallbackFunctionPtr = callback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686B3C,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode func_00647E38(MediusSessionPost166 *request)
{
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    iTerminateString(request->SessionKey, MEDIUS_SESSIONKEY_MAXLEN);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686B44,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode func_00647EC0(MediusSessionPost80 *request)
{
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    iTerminateString(request->SessionKey, MEDIUS_SESSIONKEY_MAXLEN);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 4, data_00686B48,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode MediusAccountLogin(MediusAccountLoginRequest *request, MediusCallback callback)
{
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    iTerminateString(request->SessionKey, MEDIUS_SESSIONKEY_MAXLEN);
    iTerminateString(request->AccountName, MEDIUS_ACCOUNTNAME_MAXLEN);
    iTerminateString(request->Password, MEDIUS_PASSWORD_MAXLEN);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    AccountLoginCallbackFunctionPtr = callback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686B54,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode MediusAccountLogout(MediusAccountLogoutRequest *request, MediusCallback callback)
{
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    iTerminateString(request->SessionKey, MEDIUS_SESSIONKEY_MAXLEN);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    AccountLogoutCallbackFunctionPtr = callback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686B5C,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode MediusAccountGetProfile(MediusAccountGetProfileRequest *request, MediusCallback callback)
{
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    iTerminateString(request->SessionKey, MEDIUS_SESSIONKEY_MAXLEN);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    AccountGetProfileCallbackFunctionPtr = callback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686B6C,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode MediusAccountRegistration(MediusAccountRegistrationRequest *request, MediusCallback callback)
{
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    iTerminateString(request->SessionKey, MEDIUS_SESSIONKEY_MAXLEN);
    iTerminateString(request->AccountName, MEDIUS_ACCOUNTNAME_MAXLEN);
    iTerminateString(request->Password, MEDIUS_PASSWORD_MAXLEN);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    AccountRegistrationCallbackFunctionPtr = callback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686B64,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode MediusAccountUpdatePassword(MediusAccountUpdatePasswordRequest *request, MediusCallback callback)
{
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    iTerminateString(request->SessionKey, MEDIUS_SESSIONKEY_MAXLEN);
    iTerminateString(request->String38, 32);
    iTerminateString(request->String70, 32);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    AccountUpdatePasswordCallbackFunctionPtr = callback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686B7C,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode MediusAccountUpdateStats(MediusAccountUpdateStatsRequest *request, MediusCallback callback)
{
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    iTerminateString(request->SessionKey, MEDIUS_SESSIONKEY_MAXLEN);
    iTerminateString(request->Stats, MEDIUS_ACCOUNTSTATS_MAXLEN);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    AccountUpdateStatsCallbackFunctionPtr = callback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686B84,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode MediusAccountUpdateProfile(MediusAccountUpdateProfileRequest *request, MediusCallback callback)
{
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    iTerminateString(request->SessionKey, MEDIUS_SESSIONKEY_MAXLEN);
    iTerminateString(request->String44, 32);
    iTerminateString(request->String76, 32);
    iTerminateString(request->String108, 32);
    iTerminateString(request->String140, 32);
    iTerminateString(request->String172, 32);
    iTerminateString(request->String204, 32);
    iTerminateString(request->String236, 32);
    iTerminateString(request->String268, 3);
    iTerminateString(request->String271, 32);
    iTerminateString(request->String303, 32);
    iTerminateString(request->String335, 16);
    iTerminateString(request->String351, 80);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    AccountUpdateProfileCallbackFunctionPtr = callback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686B74,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode MediusAccountDelete(MediusAccountDeleteRequest *request, MediusCallback callback)
{
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    AccountDeleteCallbackFunctionPtr = callback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686B8C,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode MediusAnonymousLogin(MediusAnonymousLoginRequest *request, MediusCallback callback)
{
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    iTerminateString(request->SessionKey, MEDIUS_SESSIONKEY_MAXLEN);
    iTerminateString(request->SessionDisplayName, MEDIUS_ACCOUNTNAME_MAXLEN);
    iTerminateString(request->SessionDisplayStats, MEDIUS_ACCOUNTSTATS_MAXLEN);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    AnonymousLoginCallbackFunctionPtr = callback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686B94,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode MediusCreateGame(MediusCreateGameRequest *request, MediusCallback callback)
{
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    iTerminateString(request->SessionKey, MEDIUS_SESSIONKEY_MAXLEN);
    iTerminateString(request->String56, 64);
    iTerminateString(request->String120, 32);
    iTerminateString(request->String152, 32);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    CreateGameCallbackFunctionPtr = callback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686BA0,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode MediusTextFilter(MediusTextFilterRequest *request, MediusCallback callback)
{
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    iTerminateString(request->SessionKey, MEDIUS_SESSIONKEY_MAXLEN);
    iTerminateString(request->Text, 64);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    MediusTextFilterCallbackFunctionPtr = callback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686DC4,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode MediusCreateChannel(MediusCreateChannelRequest *request, MediusCallback callback)
{
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    iTerminateString(request->SessionKey, MEDIUS_SESSIONKEY_MAXLEN);
    iTerminateString(request->String48, 64);
    iTerminateString(request->String112, 32);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    CreateChannelCallbackFunctionPtr = callback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 4, data_00686BB0,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode MediusJoinChannel(MediusJoinChannelRequest *request, MediusCallback callback)
{
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    iTerminateString(request->SessionKey, MEDIUS_SESSIONKEY_MAXLEN);
    iTerminateString(request->LobbyChannelPassword, 32);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    JoinChannelCallbackFunctionPtr = callback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686BC8,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode MediusGetGames(MediusGetGamesRequest *request, MediusCallback callback)
{
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    iTerminateString(request->SessionKey, MEDIUS_SESSIONKEY_MAXLEN);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    GameListCallbackFunctionPtr = callback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686BD8,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode MediusGetChannels(MediusGetChannelsRequest *request, MediusCallback callback)
{
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    iTerminateString(request->SessionKey, MEDIUS_SESSIONKEY_MAXLEN);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    ChannelListCallbackFunctionPtr = callback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686BE0,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode MediusGetLobbyPlayers(MediusGetLobbyPlayersRequest *request, MediusCallback callback)
{
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    iTerminateString(request->SessionKey, MEDIUS_SESSIONKEY_MAXLEN);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    LobbyWorldPlayerListCallbackFunctionPtr = callback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686BE8,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode MediusGetGamePlayers(MediusGetGamePlayersRequest *request, MediusCallback callback)
{
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    iTerminateString(request->SessionKey, MEDIUS_SESSIONKEY_MAXLEN);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    GameWorldPlayerListCallbackFunctionPtr = callback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686BF0,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode MediusGetPlayerInfo(MediusGetPlayerInfoRequest *request, MediusCallback callback)
{
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    iTerminateString(request->SessionKey, MEDIUS_SESSIONKEY_MAXLEN);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    PlayerInfoCallbackFunctionPtr = callback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686BF8,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode MediusGetGameInfo(MediusGetGameInfoRequest *request, MediusCallback callback)
{
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    iTerminateString(request->SessionKey, MEDIUS_SESSIONKEY_MAXLEN);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    GameInfoCallbackFunctionPtr = callback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686C00,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode MediusGetChannelInfo(MediusGetChannelInfoRequest *request, MediusCallback callback)
{
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    iTerminateString(request->SessionKey, MEDIUS_SESSIONKEY_MAXLEN);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    ChannelInfoCallbackFunctionPtr = callback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686C08,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode MediusFindPlayer(MediusFindPlayerRequest *request, MediusCallback callback)
{
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    iTerminateString(request->SessionKey, MEDIUS_SESSIONKEY_MAXLEN);
    iTerminateString(request->Name, MEDIUS_ACCOUNTNAME_MAXLEN);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    FindPlayerCallbackFunctionPtr = callback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686C18,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode MediusFindWorldByName(MediusFindWorldByNameRequest *request, MediusCallback callback)
{
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    iTerminateString(request->SessionKey, MEDIUS_SESSIONKEY_MAXLEN);
    iTerminateString(request->Name, 64);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    FindWorldByNameCallbackFunctionPtr = callback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686C10,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode MediusSendChatMessage(MediusChatMessage *request)
{
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    iTerminateString(request->SessionKey, MEDIUS_SESSIONKEY_MAXLEN);
    iTerminateString(request->Message, 64);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686C20,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode MediusGetBuddyList(MediusGetBuddyListRequest *request, MediusCallback callback)
{
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    iTerminateString(request->SessionKey, MEDIUS_SESSIONKEY_MAXLEN);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    GetBuddyListCallbackFunctionPtr = callback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686C28,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode MediusAddToBuddyList(MediusAddToBuddyListRequest *request, MediusCallback callback)
{
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    iTerminateString(request->SessionKey, MEDIUS_SESSIONKEY_MAXLEN);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    AddToBuddyListCallbackFunctionPtr = callback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686C30,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode MediusRemoveFromBuddyList(MediusRemoveFromBuddyListRequest *request, MediusCallback callback)
{
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    iTerminateString(request->SessionKey, MEDIUS_SESSIONKEY_MAXLEN);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    RemoveFromBuddyListCallbackFunctionPtr = callback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686C48,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode MediusBuddyGetPermission(MediusBuddyGetPermissionRequest *request, MediusCallback callback)
{
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    iTerminateString(request->SessionKey, MEDIUS_SESSIONKEY_MAXLEN);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    BuddyGetPermissionCallbackFunctionPtr = callback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 4, data_00686C50,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode MediusBuddyAddConfirmation(MediusBuddyAddConfirmationRequest *request)
{
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    iTerminateString(request->String28, MEDIUS_SESSIONKEY_MAXLEN);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 4, data_00686C40,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode MediusGetPolicy(MediusGetPolicyRequest *request, MediusCallback callback)
{
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    iTerminateString(request->SessionKey, MEDIUS_SESSIONKEY_MAXLEN);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    GetPolicyCallbackFunctionPtr = callback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686C5C,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode func_006494A0(MediusRequest_006494A0 *request, MediusCallback callback)
{
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    iTerminateString(request->SessionKey, MEDIUS_SESSIONKEY_MAXLEN);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    data_00656244 = callback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686C64,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode func_00649540(MediusRequest_00649540 *request, MediusCallback callback)
{
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    iTerminateString(request->SessionKey, MEDIUS_SESSIONKEY_MAXLEN);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    data_00656248 = callback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686C6C,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode func_006495E0(MediusRequest_006495E0 *request, MediusCallback callback)
{
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    iTerminateString(request->SessionKey, MEDIUS_SESSIONKEY_MAXLEN);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    data_0065624C = callback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686C74,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode MediusSendPlayerReport(MediusPlayerReport *request)
{
    iTerminateString(request->SessionKey, MEDIUS_SESSIONKEY_MAXLEN);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686B28,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode MediusSendWorldReport(MediusWorldReport *request)
{
    iTerminateString(request->SessionKey, MEDIUS_SESSIONKEY_MAXLEN);
    iTerminateString(request->String28, 64);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686B24,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode MediusSendEndGameReport(MediusEndGameReport *request)
{
    iTerminateString(request->SessionKey, MEDIUS_SESSIONKEY_MAXLEN);
    iTerminateString(request->String24, 64);
    iTerminateString(request->String88, 32);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686B2C,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode MediusGetAnnouncements(MediusGetAnnouncementsRequest *request, MediusCallback callback)
{
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    iTerminateString(request->SessionKey, MEDIUS_SESSIONKEY_MAXLEN);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    GetAnnouncementsCallbackFunctionPtr = callback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686C80,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode MediusGetAllAnnouncements(MediusGetAllAnnouncementsRequest *request, MediusCallback callback)
{
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    iTerminateString(request->SessionKey, MEDIUS_SESSIONKEY_MAXLEN);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    GetAnnouncementsCallbackFunctionPtr = callback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686C84,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode MediusSetGameListFilter(MediusSetGameListFilterRequest *request, MediusCallback callback)
{
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    iTerminateString(request->SessionKey, MEDIUS_SESSIONKEY_MAXLEN);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    SetGameListFilterCallbackFunctionPtr = callback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686C8C,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode MediusClearGameListFilter(MediusClearGameListFilterRequest *request, MediusCallback callback)
{
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    iTerminateString(request->SessionKey, MEDIUS_SESSIONKEY_MAXLEN);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    ClearGameListFilterCallbackFunctionPtr = callback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686C94,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode MediusGetGameListFilter(MediusGetGameListFilterRequest *request, MediusCallback callback)
{
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    iTerminateString(request->SessionKey, MEDIUS_SESSIONKEY_MAXLEN);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    GetGameListFilterCallbackFunctionPtr = callback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686C9C,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

int MediusCreateMessageID(char *key, int type, char *out)
{
    iTerminateString(key, MEDIUS_SESSIONKEY_MAXLEN);
    iTerminateString(out, MEDIUS_MESSAGEID_MAXLEN);
    iCreateAndAllocateMessageID(key, data_006562F8, type, out);
    if (data_006562F8 >= 999)
        data_006562F8 = 0;
    else
        data_006562F8++;
    return 0;
}

MediusErrorCode MediusGetMyIP(MediusGetMyIPRequest *request, MediusCallback callback)
{
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    iTerminateString(request->SessionKey, MEDIUS_SESSIONKEY_MAXLEN);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    GetMyIPCallbackFunctionPtr = callback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686CA4,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode MediusGetWorldSecurityLevel(MediusGetWorldSecurityLevelRequest *request, MediusCallback callback)
{
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    iTerminateString(request->SessionKey, MEDIUS_SESSIONKEY_MAXLEN);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    GetWorldSecurityLevelCallbackFunctionPtr = callback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686D7C,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode MediusBanPlayer(MediusBanPlayerRequest *request, MediusCallback callback)
{
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    iTerminateString(request->SessionKey, MEDIUS_SESSIONKEY_MAXLEN);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    BanPlayerCallbackFunctionPtr = callback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686D84,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode MediusAccountGetID(MediusAccountGetIDRequest *request, MediusCallback callback)
{
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    iTerminateString(request->SessionKey, MEDIUS_SESSIONKEY_MAXLEN);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    AccountGetIDCallbackFunctionPtr = callback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686B4C,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode MediusGetLocations(MediusGetLocationsRequest *request, MediusCallback callback)
{
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    iTerminateString(request->SessionKey, MEDIUS_SESSIONKEY_MAXLEN);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    GetLocationsCallbackFunctionPtr = callback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686D8C,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode MediusPickLocation(MediusPickLocationRequest *request, MediusCallback callback)
{
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    iTerminateString(request->SessionKey, MEDIUS_SESSIONKEY_MAXLEN);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    PickLocationCallbackFunctionPtr = callback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686D94,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode func_0064A018(MediusLadderPositionRequest *request, MediusCallback callback)
{
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    iTerminateString(request->SessionKey, MEDIUS_SESSIONKEY_MAXLEN);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    MediusLadderPositionCallbackFunctionPtr = callback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686DA4,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode MediusLadderList(MediusLadderListRequest *request, MediusCallback callback)
{
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    iTerminateString(request->SessionKey, MEDIUS_SESSIONKEY_MAXLEN);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    MediusLadderListCallbackFunctionPtr = callback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686DAC,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode MediusChatToggleFilter(MediusChatToggleRequest *request, MediusCallback callback)
{
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    iTerminateString(request->SessionKey, MEDIUS_SESSIONKEY_MAXLEN);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    MediusChatToggleCallbackFunctionPtr = callback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686DB4,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode func_0064A1F8(MediusRequest_0064A1F8 *request, MediusCallback callback)
{
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    iTerminateString(request->SessionKey, MEDIUS_SESSIONKEY_MAXLEN);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    data_00656284 = callback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686DBC,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode MediusGetTotalGames(MediusGetTotalGamesRequest *request, MediusCallback callback)
{
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    iTerminateString(request->SessionKey, MEDIUS_SESSIONKEY_MAXLEN);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    MediusGetTotalGamesCallbackFunctionPtr = callback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686DD0,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode MediusGetTotalChannels(MediusGetTotalChannelsRequest *request, MediusCallback callback)
{
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    iTerminateString(request->SessionKey, MEDIUS_SESSIONKEY_MAXLEN);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    MediusGetTotalChannelsCallbackFunctionPtr = callback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686DD8,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode func_0064AA58(MediusRequest_0064AA58 *request, MediusCallback callback)
{
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    iTerminateString(request->SessionKey, MEDIUS_SESSIONKEY_MAXLEN);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    data_00656298 = callback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686DE0,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode func_0064AAF8(MediusRequest_0064AAF8 *request, MediusCallback callback)
{
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    iTerminateString(request->SessionKey, MEDIUS_SESSIONKEY_MAXLEN);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    data_0065629C = callback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686DE8,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode func_0064AB98(MediusRequest_0064AB98 *request, MediusCallback callback)
{
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    iTerminateString(request->SessionKey, MEDIUS_SESSIONKEY_MAXLEN);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    data_006562A0 = callback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686DF0,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode func_0064AC38(MediusRequest_0064AC38 *request, MediusCallback callback)
{
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    iTerminateString(request->SessionKey, MEDIUS_SESSIONKEY_MAXLEN);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    data_006562A4 = callback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686DF8,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode func_0064ACD8(MediusGetUniverseInformationRequest *request, MediusCallback newsCallback,
                              MediusCallback listCallback)
{
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    data_006561B0 = 0;
    data_006561AC = request->InfoType;
    data_006561B4 = 1;
    data_006562AC = newsCallback;
    data_006562B0 = listCallback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686F04,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode func_0064AD98(MediusGetUniverseInformationRequest *request, MediusCallback newsCallback,
                              MediusCallback statusListCallback)
{
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    data_006561B0 = 0;
    data_006561AC = request->InfoType;
    data_006561B4 = 1;
    data_006562AC = newsCallback;
    data_006562B8 = statusListCallback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 4, data_00686F10,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode func_0064AE58(MediusRequest_0064AE58 *request, MediusCallback callback)
{
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    data_006562BC = callback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686E10,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode func_0064AEE8(MediusRequest_0064AEE8 *request, MediusCallback callback)
{
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    data_006562C0 = callback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686E18,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode func_0064AF78(MediusRequest_0064AF78 *request, MediusCallback callback)
{
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    data_006562C4 = callback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686E20,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode func_0064B008(MediusRequest_0064B008 *request, MediusCallback callback)
{
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    data_006562C8 = callback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686E30,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode func_0064B098(MediusRequest_0064B098 *request, MediusCallback callback)
{
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    data_006562CC = callback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686E38,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode func_0064B128(MediusRequest_0064B128 *request, MediusCallback callback)
{
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    data_006562D0 = callback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686E40,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode MediusLadderPosition(MediusLadderPositionExRequest *request, MediusCallback callback)
{
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    data_006562D4 = callback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686E5C,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode func_0064B248(MediusRequest_0064B248 *request, MediusCallback callback)
{
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    data_006562D8 = callback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686E4C,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode func_0064B2D8(MediusRequest_0064B2D8 *request, MediusCallback callback)
{
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    data_006562DC = callback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686E54,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode func_0064B368(MediusRequest_0064B368 *request, MediusCallback callback)
{
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    data_006562E0 = callback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 4, data_00686E94,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}
