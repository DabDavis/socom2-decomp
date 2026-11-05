/* zNetMediusChat.cpp -- the lobby's state machine and its chat: the state handlers the table at
 * 0x2ED460 registers (SOCOM 1's Open registered the same ones, by state) and their Medius
 * callbacks, the player-info pages, the chat queues, the censor, the messages' byte-order
 * converters (SOCOM 1's zNetMedius.cpp's code; the unit and its name are ours: OVL1 from about
 * 0x2ED460, after the ladder unit, to 0x2F6CFF, its static initialiser 0x4020E0 building the two
 * queues 0x44FA20 and 0x44FA50, whose destructor 0x2F5E50 and deque instances close it -- the
 * instances run on past 0x2F62B0 to 0x2F6CF7: 0x2F6340, 0x2F6600, 0x2F6A70 and 0x2F6BC0 are
 * called only from this unit's deque copy constructor 0x2F5F80, its 0x2F6090 and ~__deque_buf
 * 0x2F62B0, so they are its copies, and the lobby unit starts at 0x2F6D00). Every own function is
 * written. The deque instances: push_back 0x2F5290 with init 0x2F5940 and do_assign 0x2F5700, the
 * block buffer from an allocator 0x2F5FF0 and the pair 0x2F6060 byte-exact since the online lane's
 * round 24 (ChatQueueBuffers below, a dead-stripped device). OPEN, the copy chain and the iterators
 * -- the deque's copy constructor 0x2F5F80 (calls begin()/end() const where the target inlines them
 * and calls make_iterator), the circular buffer's 0x2F6090 (0xCC of 0x19C: x.alloc() called, begin()/
 * end() not inlined, the init's iterators not copied twice around the two empty temporaries -- the
 * rdr editor's and the state machine's open shape), both inits 0x2F6340 / 0x2F6600 (calls to the
 * do_assigns the target inlines whole: 0x2F6600 is our 0x444-byte do_assign's size exactly) and the
 * make_iterators 0x2F5D10 / 0x2F6A70 (the buffer's begin() and operator+ inlined in the target) --
 * all the tail's generation order (include/deque's comment; README "THE TAIL'S GENERATION ORDER").
 * Measured in the online lane's round 25 (build/scratch/onl/v_ends.py, v_mkoff.py, untracked), for the
 * copy constructor 0x2F5F80: a dead own function asking for the deque's begin() / end() const, defined
 * LAST (so taken first: the object then emits them at 66/67, ahead of the copy constructor at 99), or
 * ahead of the queues, or copying the deque, leaves both calls (0x60) -- generated first is NOT enough
 * for an out-of-class member to inline into the tail; declared `inline`, begin() / end() take
 * make_iterator in with them (0xC8: the target calls it); `#pragma auto_inline off` around include/
 * deque's make_iterators changes nothing (a template follows the pragma's state at the unit's end).
 * THE UNIT'S LOWER HALF IS zNetMediusStates.cpp since the online lane's round 26 (the state
 * handlers, their callbacks, the player-info pages and the room's player list, OVL1 0x2ED460-
 * 0x2F3EDF, closed by its own template tail: see its comment); this unit is OVL1 0x2F3EE0-0x2F6CFF,
 * the byte-order converters first.
 * Defined in reverse of their image order (-inline deferred). */

#include <stdio.h>
#include <stdlib.h>
#include <algorithm>
#include "zonline.h"
#include "../zmath/zmath.h"
#include "../znet/znet.h"

/* The unit's inline size limit was lowered to 100 in round 10 (`#pragma inline_max_size(100)`):
 * the target's initialiser inlines the chat queues' constructor one level -- its own body, the
 * queue's members called out of line (0x2F5EE0, 0x2F5EC0, 0x2F5ED0, the queue's constructor
 * 0x2F5F50) -- and the round-10 headers expanded it under the default 256. Round 15: with today's
 * deeper include/deque the initialiser and the queue's pieces come out the same at the default,
 * while at 100 the room players' std::sort (0x2F1980, its __sort132 and __selection_sort) calls
 * the iterator's operator* and swap<MediusLobbyWorldPlayerListResponse> out of line -- so the pragma is gone (58 -> 61
 * of the unit's rows, nothing lost). */

/* --- the Medius requests the state handlers make (SOCOM 1's; only what is passed) ----------- */

/* the account logged in or registered (OVL2 bss; SOCOM 1's AccountInfo, 0x44 bytes) */
struct ChatAccountInfo {
    int AccountID;   // 0x0
    char AccountName[32];   // 0x4
    char Password[32];   // 0x24
};

struct MediusSessionBeginRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0x0
    int ConnectionClass;   // 0x18: 1
};   // 0x1c

struct MediusAccountRegistrationRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0x0
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];   // 0x15
    int AccountType;   // 0x28: 1
    char AccountName[32];   // 0x2c
    char Password[32];   // 0x4c
};   // 0x6c

struct MediusAccountLoginRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0x0
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];   // 0x15
    char AccountName[32];   // 0x26
    char Password[32];   // 0x46
};   // 0x66

struct MediusCreateChannelRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0x0
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];   // 0x15
    int ApplicationID;   // 0x28: 10472
    int MaxPlayers;   // 0x2c
    char LobbyName[64];   // 0x30
    char LobbyPassword[32];   // 0x70
    int GenericField;   // 0x90
};   // 0x94

struct MediusJoinChannelRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0x0
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];   // 0x15
    int MediusWorldID;   // 0x28
    char LobbyChannelPassword[32];   // 0x2c
};   // 0x4c

/* how a game is joined (int-sized, as the SDK's enums are: a ternary of two of its values stored
 * is `li 1; movz`, an int's `sltu`) */
enum MediusJoinType {
    MediusJoinAsPlayer = 0,
    MediusJoinAsSpectator = 1,
    MediusJoinType_Extent = 0x7FFFFFFF
};

struct MediusJoinGameRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0x0
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];   // 0x15
    int MediusWorldID;   // 0x28
    MediusJoinType JoinType;   // 0x2c
    char GamePassword[32];   // 0x30
    int GameHostType;   // 0x50
    unsigned char _54[0xc4 - 0x54];
};   // 0xc4

/* a server's addresses (the DME SDK's NetAddressList: a plain struct, copied as words) */
struct NetAddressList {
    int AddressType0;   // 0x0
    char Address0[16];   // 0x4
    int BinaryAddress0;   // 0x14
    int AddressType1;   // 0x18
    char Address1[16];   // 0x1c
    int BinaryAddress1;   // 0x2c
};   // 0x30

/* where a server is and how to talk to it (the DME SDK's NetConnectionInfo, 0x9C bytes: a class
 * with a constructor of its own, so MW copies it member by member -- the addresses as words, the
 * key's array in a loop of eight two-word passes, the two keys bytewise) */
struct NetConnectionInfo {
    NetConnectionInfo() {}

    int Type;   // 0x0
    NetAddressList AddressList;   // 0x4
    int WorldID;   // 0x34
    unsigned int ServerKey[16];   // 0x38
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];   // 0x78
    char AccessKey[MEDIUS_SESSIONKEY_MAXLEN];   // 0x89
};   // 0x9c

/* a world looked for by its name (SOCOM 1's MediusFindWorldByName: 1, a room) */
struct MediusFindWorldByNameRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0x0
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];   // 0x15
    char Name[64];   // 0x26
    int WorldType;   // 0x68
};   // 0x6c

struct MediusFindWorldByNameResponse { MEDIUS_RESPONSE_HEAD };

struct MediusCreateChannelResponse {
    MEDIUS_RESPONSE_HEAD
    int MediusWorldID;   // 0x1c
};
struct MediusJoinChannelResponse {
    MEDIUS_RESPONSE_HEAD
    NetConnectionInfo ConnectInfo;   // 0x1c
};
struct MediusJoinGameResponse {
    MEDIUS_RESPONSE_HEAD
    int GameHostType;   // 0x1c
    NetConnectionInfo ConnectInfo;   // 0x20
};

/* a game created (SOCOM 1's MediusCreateGame, wider: the three generic fields and the host's type
 * SOCOM II's) */
struct MediusCreateGameRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0x0
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];   // 0x15
    int ApplicationID;   // 0x28: 10472
    int MinPlayers;   // 0x2c: 2
    int MaxPlayers;   // 0x30
    int GameLevel;   // 0x34
    char GameName[64];   // 0x38
    char GamePassword[32];   // 0x78
    char SpectatorPassword[32];   // 0x98
    int PlayerSkillLevel;   // 0xb8
    int RulesSet;   // 0xbc
    int GenericField1;   // 0xc0: the room
    int GenericField2;   // 0xc4: 65000
    int GenericField3;   // 0xc8: the clan game's
    int GameHostType;   // 0xcc
};   // 0xd0

struct MediusCreateGameResponse {
    MEDIUS_RESPONSE_HEAD
    int MediusWorldID;   // 0x1c
};

/* a valve: a named 16-bit value (SOCOM 1's CValve; only what is read here) */
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
    static CValve *GetByName(const char *name);
    /* the value as an int (ours: read through it, the test's load serves the sum unmasked) */
    int Value() { return m_value; }
    void MakeCallbacks(VALVE_STATE state);
    /* the value set, the callbacks told when it turns on or off (zanim.h's) */
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
    unsigned short m_value;   // 0x4 (the bit-field's halfword: added to a count with no mask)
    unsigned short m_flags;   // 0x6: the type and the pool bits
};

struct MediusSessionBeginResponse { MEDIUS_RESPONSE_HEAD };
struct MediusVersionServerResponse { MEDIUS_RESPONSE_HEAD };
struct MediusSessionEndResponse { MEDIUS_RESPONSE_HEAD };
struct MediusAccountRegistrationResponse { MEDIUS_RESPONSE_HEAD };
struct MediusAccountLoginResponse {
    MEDIUS_RESPONSE_HEAD
    int AccountID;   // 0x1c
    int AccountType;   // 0x20
    int MediusWorldID;   // 0x24
    NetConnectionInfo ConnectInfo;   // 0x28
};   // 0xc4

/* the console's language and text encoding told to Medius (SOCOM II's) */
struct MediusSetLocalizationParamsRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0x0
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];   // 0x15
    int CharacterEncoding;   // 0x28: 2
    int Language;   // 0x2c
};   // 0x30

/* the console's DNAS id posted (SOCOM II's) */
struct MediusMachineSignaturePost {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0x0
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];   // 0x15
    int _28;   // 0x28
    unsigned char Length;   // 0x2c
    unsigned char Signature[32];   // 0x2d
    unsigned char _4d[0x50 - 0x4d];
};   // 0x50

struct MediusSetLocalizationParamsResponse { MEDIUS_RESPONSE_HEAD };

/* a connection to a server asked for (SOCOM II's: the server's info, then the buffer's size and the
 * callback told the connection's index) */
struct MediusConnectParams {
    NetConnectionInfo ConnectInfo;   // 0x0
    unsigned char _9c[0xf0 - 0x9c];
    int BufferSize;   // 0xf0: 256
    void (*Callback)(int index);   // 0xf4
    int _f8;   // 0xf8
    int _fc;   // 0xfc
};   // 0x100

/* our state told to the lobby server (SOCOM 1's MediusUpdateClientState: 1, joined) */
struct MediusUpdateClientStateRequest {
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];   // 0x0
    int UserAction;   // 0x14
};   // 0x18

/* the game list filtered (SOCOM II's request) */
struct MediusSetGameListFilterRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0x0
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];   // 0x15
    int FilterField;   // 0x28
    int ComparisonOperator;   // 0x2c
    int BaselineValue;   // 0x30
};   // 0x34

struct MediusSetGameListFilterResponse { MEDIUS_RESPONSE_HEAD };

extern "C" {
extern ChatAccountInfo AccountInfo;
extern unsigned int data_0044F9E0;   /* when the active lobby next refreshes the player's info */

MediusErrorCode MediusSessionBegin(MediusSessionBeginRequest *req,
                              void (*cb)(int, int, int, MediusSessionBeginResponse *));   /* MediusSessionBegin */
MediusErrorCode MediusVersionServer(MediusSessionRequest *req,
                              void (*cb)(int, int, int, MediusVersionServerResponse *));   /* MediusVersionServer */
MediusErrorCode MediusSessionEnd(MediusSessionRequest *req,
                              void (*cb)(int, int, int, MediusSessionEndResponse *));   /* MediusSessionEnd */
MediusErrorCode MediusAccountRegistration(MediusAccountRegistrationRequest *req,
                              void (*cb)(int, int, int, MediusAccountRegistrationResponse *));   /* MediusAccountRegistration */
MediusErrorCode MediusAccountLogin(MediusAccountLoginRequest *req,
                              void (*cb)(int, int, int, MediusAccountLoginResponse *));   /* MediusAccountLogin */

void func_002EE910(int, int, int, MediusSessionBeginResponse *);   /* cbSessionBegin */
void func_002EFE00(int, int, int, MediusVersionServerResponse *);   /* cbVersionServer */
void func_002EE7B0(int, int, int, MediusSessionEndResponse *);   /* cbSessionEnd */
MediusErrorCode func_0064AB98(MediusSetLocalizationParamsRequest *req,
                              void (*cb)(int, int, int, MediusSetLocalizationParamsResponse *));   /* MediusSetLocalizationParams */
MediusErrorCode func_00647EC0(MediusMachineSignaturePost *req);   /* the DNAS id posted */
void func_002EF3B0(int, int, int, MediusSetLocalizationParamsResponse *);   /* its callback */
int func_002EF340(unsigned char language);   /* the console's language as Medius's */
extern NetConnectionInfo data_0066A840;   /* the account's lobby server (OVL2 bss) */
extern int data_0066A5E0;   /* the lobby server's connection (OVL2 bss) */
extern float data_0044F8D0;   /* when the lobby server was asked for */
bool func_00300740(int kind, MediusConnectParams *params);   /* a connection asked for (the friends' unit's) */
MediusErrorCode MediusUpdateClientState(MediusUpdateClientStateRequest *req);   /* MediusUpdateClientState */
MediusErrorCode MediusSetGameListFilter(MediusSetGameListFilterRequest *req,
                              void (*cb)(int, int, int, MediusSetGameListFilterResponse *));   /* MediusSetGameListFilter */
void func_002EFE10(int, int, int, MediusSetGameListFilterResponse *);   /* its callback */
void func_002F3FF0(struct ChatSlot *slots);   /* a free slot looked for (below) */
void func_002F7690();   /* (the lobby unit's) */
void func_002F4FC0(int ready);   /* our readiness told the server */
void func_002EF7F0();   /* our stats reported (below) */
int func_002EF590(CValve *locked);   /* the game's world reported, the delay to the next returned (below) */
extern CValve *data_0044F9D0;   /* the game's lock, while it is up */
extern unsigned int data_0044F9D8;   /* when the game's details next go to the lobby */
extern unsigned char data_0066A5F8;   /* the details to go at once */
extern unsigned char data_0044F890[64];   /* the server's key kept */
extern unsigned char data_0066A8E0;   /* logged in */

MediusErrorCode MediusCreateChannel(MediusCreateChannelRequest *req,
                              void (*cb)(int, int, int, MediusCreateChannelResponse *));   /* MediusCreateChannel */
MediusErrorCode MediusJoinChannel(MediusJoinChannelRequest *req,
                              void (*cb)(int, int, int, MediusJoinChannelResponse *));   /* MediusJoinChannel */
MediusErrorCode MediusJoinGame(MediusJoinGameRequest *req,
                              void (*cb)(int, int, int, MediusJoinGameResponse *));   /* MediusJoinGame */
MediusErrorCode MediusFindWorldByName(MediusFindWorldByNameRequest *req,
                              void (*cb)(int, int, int, MediusFindWorldByNameResponse *));   /* MediusFindWorldByName */
void func_002ED9C0(int, int, int, MediusFindWorldByNameResponse *);   /* cbFindRoomByName */
void func_002EE210(int, int, int, MediusFindWorldByNameResponse *);   /* cbFindGameByName */
MediusErrorCode MediusCreateGame(MediusCreateGameRequest *req,
                              void (*cb)(int, int, int, MediusCreateGameResponse *));   /* MediusCreateGame */
void func_002E9840(CValve *valve);   /* the clan game's team set from the valve (the clan unit's) */
int func_002E99F0();   /* the clan game's field (the clan unit's) */

extern NetConnectionInfo data_0066A540;   /* the room's lobby server (OVL2 bss) */
extern NetConnectionInfo data_0066A8F0;   /* the game's server (OVL2 bss) */
extern int LobbyWorldID;   /* the room we are in, -1 none (SOCOM 1's) */
extern int MediusWorldID;   /* the game joined (SOCOM 1's) */
extern unsigned char g_bJoiningRandomGame;
extern float data_004A44A0;   /* (a clock) */
extern float data_0044F9E8;   /* when the room was joined */
int func_002F6D00(CZOnlineLobby *lobby);   /* we join as a spectator */
void func_002F4080(void *msg, int type);   /* a request put into the network's order (empty) */

void func_00300680();   /* the Medius connection closed */
void func_002FA870(CZOnlineLobby *lobby);   /* the player's info asked for (the lobby unit's) */
void func_002FAA80(CZOnlineLobby *lobby);   /* and refreshed */
}

extern BriefingRoomDB data_0044F568;   /* the room list (the rooms' unit's) */
unsigned char GetLanguage();   /* the console's language (util_loc.cpp's) */
const char *GetString(int id);   /* a localised text (util_loc.cpp's) */

void UpdateChatDisplay();   /* the room's chat shown (below) */

/* a variable's text when it holds one (zNetMediusClan.cpp's) */
inline char *ZString(_zrdr *z)
{
    return z->type == 3 ? z->string : 0;
}

/* a pair of 24-byte slots, a word at 4 marking one in use (only what is read here) */
struct ChatSlot {
    int m_0;
    int m_used;   // 0x4
    unsigned char _8[0x18 - 0x8];
};

/* the text filter's answer awaited: the caller's callback and its data (SOCOM 1's Censor kept
 * them likewise) */
typedef void (*CensorCallback)(const char *text, void *data);
/* a chat line's colour: the mission's message type (fts_state.h's MSGTYPE, the same three values --
 * 0 white, 1 yellow, 5 red; a byte enum: assigned with li, compared through andi; the enumerators'
 * names the chat's, ours) */
enum MSGTYPE {
    CHAT_WHITE = 0,
    CHAT_CLAN = 1,
    CHAT_PRIVATE = 5
};

/* the mission's ring of messages for the player (fts_mission.cpp's class: theMission + 0x11B0) */
class CMessageQueue {
public:
    void Enqueue(const char *text, MSGTYPE type, const char *name);
};

/* the chat turned on or off (SOCOM 1's MediusChatToggle; int-sized) */
enum MediusChatToggle {
    MediusChatOn = 0,
    MediusChatOff = 1,
    MediusChatToggle_Extent = 0x7FFFFFFF
};

struct MediusChatToggleRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0x0
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];   // 0x15
    MediusChatToggle ChatToggle;   // 0x28
};   // 0x2c

/* a text to one account */
struct MediusSendDirectMessageRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0x0
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];   // 0x15
    int MessageType;   // 0x28
    int TargetAccountID;   // 0x2c
    char Message[64];   // 0x30
};   // 0x70

/* a player's info asked for (SOCOM 1's MediusGetPlayerInfo) */
struct MediusPlayerInfoRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0x0
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];   // 0x15
    int AccountID;   // 0x28
};   // 0x2c

/* the player's info (the SDK's MediusPlayerInfoResponse: the account's stats as text at 0x48) */
struct MediusPlayerInfoResponse {
    MEDIUS_RESPONSE_HEAD
    char AccountName[32];   // 0x1c
    int ApplicationID;   // 0x3c
    int PlayerStatus;   // 0x40
    int ConnectionClass;   // 0x44
    char Stats[256];   // 0x48
};

/* a chat line (SOCOM 1's MediusChatMessage: 1 private, the target looked up first) */
struct MediusChatMessage {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0x0
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];   // 0x15
    int MessageType;   // 0x28
    int TargetID;   // 0x2c
    char Message[64];   // 0x30
};   // 0x70

/* a player looked up by name (SOCOM 1's MediusFindPlayer: 1, by name) */
struct MediusFindPlayerRequest {
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0x0
    char SessionKey[MEDIUS_SESSIONKEY_MAXLEN];   // 0x15
    int SearchType;   // 0x28
    int ID;   // 0x2c
    char Name[32];   // 0x30
};   // 0x50

/* the player found (the friends unit's layout) */
struct MediusFindPlayerResponse {
    MEDIUS_RESPONSE_HEAD
    int ApplicationID;   // 0x1c
    char ApplicationName[32];   // 0x20
    int ApplicationType;   // 0x40
    int MediusWorldID;   // 0x44
    int AccountID;   // 0x48
    char AccountName[32];   // 0x4c
    char EndOfList;   // 0x6c
};

/* a queue of chat lines (the name ours): four blank lines pushed and popped at the start, so its
 * first block is there before the first line -- the constructor is too tall to inline whole into
 * the initialiser, which calls the queue's members out of line (0x2F5EE0, 0x2F5EC0, 0x2F5ED0) */
class CChatQueue : public std::queue<MediusChatMessage> {
public:
    CChatQueue()
    {
        MediusChatMessage msg;

        for (int i = 0; i < 4; i++)
            push(msg);
        while (!empty())
            pop();
    }
};

/* the unit's two queues, built by its initialiser 0x4020E0, their destructor 0x2F5E50 registered */
CChatQueue data_0044FA20;   /* the public lines waiting for the filter */
CChatQueue data_0044FA50;   /* the private lines waiting for their player */

/* A MODELLING DEVICE, dead-stripped (src/ABSENT.txt; the online lane, round 24): the chat queue's
 * block buffers built in an own function of the unit. An out-of-class template member is inlined
 * into the unit's tail only once an own function has instantiated it (README "Out-of-class template
 * members"), and this unit's tail inlines what the lobby unit's calls: the chat's push_back 0x2F5290
 * zeroes its temporary buffer in place (__cdeque_buf<PCv>() and allocator<PCv>(), called out of line
 * without this), and its __deque_buf(const Allocator &) 0x2F5FF0 builds the rebound buffer temporary
 * in place where the lobby's 0x2E09A0 calls cdeque(const Allocator &) and both allocator
 * constructors, and the pair's constructor 0x2F6060 copies the allocator inline where the lobby's
 * 0x2E0A80 calls its copy. What own code of the original instantiated them is not known (none
 * survives in the image: every own function of the unit is written); this function stands for it --
 * the two buffers __deque_buf's two constructors build (the rebound one, and the default) and the
 * allocator copied -- and nothing calls it (veh_vehicle.cpp's dead-stripped clear is the precedent:
 * README "Tells from completing the MSL headers, round 4"). Measured and NOT enough: requesting the
 * deque's begin() / end() const here does not make the copy constructor 0x2F5F80 inline them, nor
 * requesting the two do_assigns make the inits 0x2F6340 / 0x2F6600 inline them (both stay calls) */
void ChatQueueBuffers(const std::deque<MediusChatMessage> &d)
{
    Metrowerks::cdeque<MediusChatMessage *, std::allocator<MediusChatMessage *> > buf(
        (std::allocator<MediusChatMessage *>(std::allocator<MediusChatMessage>())));
    Metrowerks::cdeque<const void *, std::allocator<const void *> > tmp;
    std::allocator<MediusChatMessage> a(d.alloc());
}

void cbGetPlayerInfo(int, int, int, MediusPlayerInfoResponse *);   /* the player's info shown */

extern "C" {
MediusErrorCode MediusFindPlayer(MediusFindPlayerRequest *req,
                              void (*cb)(int, int, int, MediusFindPlayerResponse *));   /* MediusFindPlayer */
MediusErrorCode MediusGetPlayerInfo(MediusPlayerInfoRequest *req,
                              void (*cb)(int, int, int, MediusPlayerInfoResponse *));   /* MediusGetPlayerInfo */
void func_00300670();   /* the Medius library closed */
void func_0030E3A0(CZNetwork *net, int);   /* and closed (CZNetwork's) */
MediusErrorCode MediusSendChatMessage(MediusSendDirectMessageRequest *req);   /* the direct message sent */
extern char data_00437690[];   /* theMission's message ring (CMission::m_netQueue) */
}

extern "C" {
extern CensorCallback data_0044FA00;
extern void *data_0044FA08;
}

void cbFindChatPlayerID(int, int, int, MediusFindPlayerResponse *resp);   /* the private line's player found */
void cbFilterChat(const char *text, void *data);   /* the public line filtered: sent */

/* a chat line sent: queued, then a private one's player looked up, a public one filtered first
 * (SOCOM 1's SendChat filtered every line; OVL1 0x2F4860). The filter is Censor, inlined */
bool CZOnlineLobby::SendChat(const char *text, const char *to)
{
    if (!m_isopen)
        return false;
    MediusChatMessage msg;

    memset(&msg, 0, sizeof(msg));
    SetSessionKey(msg.SessionKey);
    if (text)
        memcpy(msg.Message, text, sizeof(msg.Message));
    msg.Message[sizeof(msg.Message) - 1] = 0;
    if (to) {
        msg.MessageType = 1;
        msg.TargetID = -1;
        data_0044FA50.push(msg);
        MediusFindPlayerRequest find;

        memset(&find, 0, sizeof(find));
        SetSessionKey(find.SessionKey);
        find.ID = 0;
        find.SearchType = 1;
        if (to)
            memcpy(find.Name, to, sizeof(find.Name));
        CheckMediusError(MediusFindPlayer(&find, cbFindChatPlayerID));
    } else {
        msg.TargetID = -1;
        msg.MessageType = 0;
        data_0044FA20.push(msg);
        Censor(msg.Message, cbFilterChat, &msg);
    }
    return true;
}

/* the online session left: the Medius and DME connections closed, the lobby with them (SOCOM 1's;
 * OVL1 0x2F4240: 'LobbyBailout') */
bool CZOnlineLobby::Bailout()
{
    if (!m_isopen)
        return false;
    if (theNetwork.m_bNetwork) {
        func_00300680();
        func_00300670();
    }
    if (theNetwork.m_network_init) {
        theNetwork.zNetClose();
        func_0030E3A0(&theNetwork, 0);
    }
    Close();
    return true;
}

/* a network error: kept in mp_net_errno, offered to the handler, else told the menus -- and, out
 * of a game and the lobby, the session left (SOCOM 1's; OVL1 0x2F4090) */
void CZOnlineLobby::RecoverFromNetError(int err)
{
    if (!err)
        return;
    bool handled = false;
    CValve::Create("mp_net_errno", VALVE_TYPE_1)->SetValue(err);
    if (m_net_error_handler)
        handled = m_net_error_handler(err);
    if (!handled) {
        if (m_current_state == READY_TO_CONNECT) {
            LobbyEvent("MP_MEDIUS_ERROR", GetString(1067));
        } else if (m_current_state == LOBBY_ACTIVE) {
            LobbyEvent("MP_MEDIUS_ERROR", GetString(1068));
        } else {
            LobbyEvent("MP_MEDIUS_ERROR", GetString(1068));
            theLobby.Bailout();
        }
    }
}

/* a chat message forwarded to us: shown in the active lobby, and anywhere when it is a private or
 * a clan one (SOCOM 1's cbChatMessage, MediusInit's first callback; OVL1 0x2F4EF0) */
void cbChatMessage(int, int, int, MediusChatFwdMessage *msg)
{
    MSGTYPE color = CHAT_WHITE;

    switch (msg->MessageType) {
    case 0:
        color = CHAT_WHITE;
        break;
    case 1:
        color = CHAT_PRIVATE;
        break;
    case 2:
        color = CHAT_CLAN;
        break;
    }
    if (theLobby.m_current_state == LOBBY_ACTIVE || color == CHAT_PRIVATE || color == CHAT_CLAN) {
        char text[1024];
        char *name = msg->OriginatorAccountName;

        sprintf(text, "%s: %s", name, msg->Message);
        ((CMessageQueue *)data_00437690)->Enqueue(text, color, name);
    }
}

/* the chat turned on or off as we enter or leave a game: the request is built, but nothing sends
 * it -- the result handed on is a constant success (SOCOM II's; OVL1 0x2F4FC0). The
 * ready-to-connect handler, zNetMediusStates.cpp's, calls it out of line (its pin, needed while the
 * two were one unit, gone with the split) */
void func_002F4FC0(int ready)
{
    MediusChatToggleRequest req;

    memset(&req, 0, sizeof(req));
    SetSessionKey(req.SessionKey);
    req.ChatToggle = ready ? MediusChatOn : MediusChatOff;
    CheckMediusError(MediusSuccess);
}

/* a text sent straight to an account (SOCOM II's; OVL1 0x2F4D60: the clan unit sends the
 * invitations' texts through it on theLobby) */
extern "C" void func_002F4D60(CZOnlineLobby *lobby, int account, int type, const char *text)
{
    MediusSendDirectMessageRequest req;

    memset(&req, 0, sizeof(req));
    SetSessionKey(req.SessionKey);
    req.TargetAccountID = account;
    if (text)
        memcpy(req.Message, text, sizeof(req.Message));
    req.MessageType = type;
    req.Message[sizeof(req.Message) - 1] = 0;
    CheckMediusError(MediusSendChatMessage(&req));
}

/* the public line filtered: taken off the filter's queue (the block's front() inline) and sent to
 * its target as a direct message -- the text above inlined -- and shown to us unless it was to
 * ourselves, a private line in its own colour (SOCOM 1's cbFilterChat, which SendChat hands the
 * censor as SOCOM 1's did, and which looked the room's players up instead; OVL1 0x2F4BA0) */
void cbFilterChat(const char *text, void *data)
{
    MediusChatMessage msg = data_0044FA20.front();

    data_0044FA20.pop();
    func_002F4D60(&theLobby, msg.TargetID, msg.MessageType, text);
    if (msg.TargetID != AccountInfo.AccountID)
        theGame.GetCurrentState()->DisplayMessage(msg.Message, AccountInfo.AccountName, 0, 0, 0, 1, 0,
                                                  msg.MessageType == 1 ? 5 : 0);
}

/* the private line's player found: the line, taken off the private queue, gets the account as its
 * target and goes through the censor as a public one does -- else it is dropped and the menus told
 * (SOCOM II's; OVL1 0x2F4A20, the callback SendChat's MediusFindPlayer is given; the name ours, by
 * the status it checks, as the friends unit's cbFindBuddyListPlayerID) */
void cbFindChatPlayerID(int, int, int, MediusFindPlayerResponse *resp)
{
    if (CheckMediusCallbackStatus("FindChatPlayerID", resp->StatusCode)) {
        MediusChatMessage msg = data_0044FA50.front();

        data_0044FA50.pop();
        msg.TargetID = resp->AccountID;
        data_0044FA20.push(msg);
        theLobby.Censor(msg.Message, cbFilterChat, 0);
    } else {
        data_0044FA50.pop();
        LobbyEvent("MP_PLAYER_NOT_FOUND", 0);
    }
}

/* the room's chat shown: each line in its colour -- white, grey, a public line's red, a clan line's
 * yellow -- with its sender when it has one, one line of text each (SOCOM 1's cbFilterChat built
 * the list itself; OVL1 0x2F5020: the lobby server's connect callback and the menu state's line
 * insert 0x1F5020 call it; the name ours, as SOCOM 1's UpdateGameListDisplay). The counter is
 * declared first (it takes s2, the room s1) and the line's sender and text are read through the
 * iterator's `*` (through `->` the name's address fills the test's delay slot) */
void UpdateChatDisplay()
{
    if (theLobby.uiv_MediusChatRecvVar) {
        int n;
        CRdrEditor ed;

        ed.push(0);
        BriefingRoom *room = data_0044F568.FindRoom(ZString(theLobby.uiv_LobbyWorldNameVar));

        if (room) {
            n = 1;

            for (std::vector<MediusChatFwdMessage>::iterator i = room->m_chat.begin(); i != room->m_chat.end();
                 ++i, ++n) {
                const char *colour = "#C\xff\xff\xff";
                char text[256];

                switch (i->MessageType) {
                case 0:
                    colour = "#C\x80\x80\x80";
                    break;
                case 1:
                    colour = "#C\x80\x40\x40";
                    break;
                case 2:
                    colour = "#C\x80\x80\x40";
                    break;
                }
                if (i->OriginatorAccountName[0])
                    sprintf(text, "%s %s: %s", colour, (*i).OriginatorAccountName, (*i).Message);
                else
                    sprintf(text, "%s%s", colour, i->Message);
                ed.addstringnode(text);
                if (n < room->m_chat.size())
                    ed.addstringnode("\n");
            }
        }
        ed.pop();
        theLobby.uiv_MediusChatRecvVar->Set(&ed);
    }
}

/* the filtered text handed to the waiting callback, which is then forgotten (SOCOM 1's; OVL1
 * 0x2F4EA0) */
void cbCensor(int a, int b, int c, MediusTextFilterResponse *resp)
{
    if (data_0044FA00)
        data_0044FA00(resp->Text, data_0044FA08);
    data_0044FA00 = 0;
    data_0044FA08 = 0;
}

/* a text sent to Medius's filter, the callback told the result (SOCOM 1's; OVL1 0x2F4DF0) */
void CZOnlineLobby::Censor(const char *text, CensorCallback cb, void *data)
{
    MediusTextFilterRequest req;

    memset(&req, 0, sizeof(req));
    SetSessionKey(req.SessionKey);
    req.TextFilterType = 1;
    if (text)
        memcpy(req.Text, text, sizeof(req.Text));
    data_0044FA00 = cb;
    data_0044FA08 = data;
    CheckMediusError(MediusTextFilter(&req, cbCensor));
}

extern "C" {
void func_002F3EF0(char *rules);   /* a game's rules written out (below) */

/* the first free slot of two looked for, and nothing done with it: the search survives, its
 * result does not (a void leaf: v1 is its only temporary; OVL1 0x2F3FF0). LobbyServerConnect's
 * handler, zNetMediusStates.cpp's, calls it out of line (its pin gone with the split) */
void func_002F3FF0(ChatSlot *slots)
{
    for (int i = 0; i < 2; i++) {
        if (!slots[i].m_used)
            break;
    }
}

/* a game's rules written out as a list of numbers -- the 32 bytes after an 'A' block's five-byte
 * head or a 'B' block's six -- into a buffer nothing reads (a debug print gone; OVL1 0x2F3EF0:
 * the converter below and the LAN unit's JoinGame and CreateGame call it). Open since round 9 (12
 * instructions): the stack wants a number buffer per case, both declared ahead of the text (the
 * first-declared takes the top slot: 'A''s 240, 'B''s 224, the text 48), and each case its own base
 * copy and counter, the counter declared last (base s1, counter s0 in both) */
void func_002F3EF0(char *rules)
{
    char numA[16];
    char numB[16];
    char text[176];

    switch (rules[0]) {
    case 'A': {
        const unsigned char *p = (const unsigned char *)rules;

        sprintf(text, "[");
        for (int i = 0; i < 32; i++) {
            sprintf(numA, "%d, ", p[i + 5]);
            strcat(text, numA);
        }
        strcat(text, "]");
        break;
    }
    case 'B': {
        const unsigned char *p = (const unsigned char *)rules;

        sprintf(text, "[");
        for (int i = 0; i < 32; i++) {
            sprintf(numB, "%d, ", p[i + 6]);
            strcat(text, numB);
        }
        strcat(text, "]");
        break;
    }
    }
}

/* a response put into the host's order: only the type-129 ones carry anything to convert (OVL1
 * 0x2F4030; the request side, 0x2F4080, is empty) */
void func_002F4030(void *msg, int type)
{
    switch (type) {
    case 32:
        break;
    case 33:
        break;
    case 129:
        func_002F3EF0((char *)msg + 140);
        break;
    }
}
}
