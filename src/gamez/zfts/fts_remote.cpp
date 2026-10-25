/* fts_remote.cpp -- the online game's glue: the network's set up and tear down, the DME
 * connection, and the application packets' senders and handlers (SOCOM 1's fts_remote.cpp:
 * its static initialiser builds the broadcast schedules, as SOCOM II's 0x400400 does; OVL1
 * 0x2B7950-0x2BD37F, after the 2D messages' unit and before fts_net_seal.cpp). Defined in
 * reverse of their image order (-inline deferred).
 *
 * The network object: SOCOM II's theNetwork is at 0x45A0C0, not data.txt's 0x45A0B8 -- every
 * sender passes 0x45A0C0 as CZNetwork::zNetSendApplicationMessage's `this` (SOCOM 1's pass
 * theNetwork itself), and the session-master and network flags SOCOM 1 keeps at 8 and 9 are at
 * 0x45A0C0 and 0x45A0C1, so SOCOM II dropped the two team indices ahead of them. Until the map
 * row moves, this unit reaches it by its address. */

#include "../zmath/zmath.h"

/* the network (SOCOM 1's CZNetwork, at SOCOM II's offsets: only what this unit touches) */
class CZNetwork {
public:
    unsigned char m_bSessionMaster;   // 0x0 (SOCOM 1's at 0x8)
    unsigned char m_bNetwork;   // 0x1 (SOCOM 1's at 0x9)
    unsigned char _2[0xf4 - 0x2];
    int m_GameConnectionIndex;   // 0xf4 (SOCOM 1's at 0xd0)
};


class CZSealBody {
public:
    unsigned char _0[4];
    int m_4;   // 0x4: the body's network object (SOCOM 1's m_NetObjectIndex)
    unsigned char _8[0xfc8 - 0x8];
    int m_fc8;   // 0xfc8: the client it belongs to (SOCOM 1's at 0xeb8)
};

class CZVehicle {
public:
    unsigned char _0[4];
    int m_4;   // 0x4: its network object
};

/* the online game (SOCOM 1's CZNetGame: only what this unit touches, at SOCOM II's offsets) */
class CZNetGame {
public:
    unsigned char _0[0xac];
    int m_persona_object_index;   // 0xac (SOCOM 1's at 0x70)
    int m_game_object_index;   // 0xb0 (SOCOM 1's at 0x74)
    unsigned char _b4[0xcf - 0xb4];
    unsigned char m_persona_created;   // 0xcf (SOCOM 1's at 0x97)
};

class CMission {
public:
    unsigned char _0[0x1808];
    CZNetGame *m_pNetGame;   // 0x1808 (SOCOM 1's at 0x584)
};

extern CMission theMission;

class CZSealState {
public:
    static CZSealBody *GetBodyFromObjectIndex(int index);
    static int CreateLocalSeal(CZSealBody *body, bool remote);
};

class CZGameState {
public:
    static int CreateLocalGame(CZNetGame *game);
};

class CZBombState {
public:
    static void HandleBombStateRequest(int who, unsigned char state, CPnt3D *pos);
};

/* a network object as CZNetwork's table hands it out (its kind a letter: 'S' a SEAL, 'T' ...) */
struct NetObject {
    unsigned char _0[0xc];
    char m_type;   // 0xc
};

/* unnamed yet (SOCOM 1's names in the comments) */
extern "C" {
extern CZNetwork data_0045A0C0;   /* theNetwork (see the head of the file) */
extern unsigned char data_004412D8;
extern char theLobby[];

void func_0030CEF0(CZNetwork *net, char a, int conn, int to, int type, int size, void *data,
                   bool reliable);   /* CZNetwork::zNetSendApplicationMessage */
NetObject *func_0030D630(CZNetwork *net, int index);
void func_00623470(int index);
void func_002C2BB0();   /* CZPersonaState::ClearPersonaList */
void func_005423E0(int a, float b);
void func_002C00F0(CZNetGame *game);   /* CZGameState::UpdateLocalGame */
int func_002C5450();   /* CZPersonaState::CreateLocalPersona */
void func_002C5360();   /* CZPersonaState::UpdateLocalPersona */
void func_002BD640(CZSealBody *body, int arg);   /* CZSealState::UpdateLocalSeal */
int func_002C7BF0(CZVehicle *vehicle);
int func_002C3B10(int arg);
void func_002F9690(void *lobby, int arg, int mode);
int func_00541CC0(int id);
void func_0059E830(CZSealBody *body, unsigned short who, int id, void *c, int obj);
void func_002C0040(bool on);

/* the application packets' type ids (SOCOM 1's PacketType<name>) */
extern unsigned int data_00440E20;   /* PacketTypeZAnim */
extern unsigned int data_00440E38;   /* PacketTypeMessage */
extern unsigned int data_00440E60;   /* PacketTypePUAck */
extern unsigned int data_00440E80;   /* PacketTypeVoter */
}

#define theNetwork data_0045A0C0

/* the ZAnim packet: its body as the caller built it (OVL1 0x2BB880) */
void SendZAnimNetworkPacket(void *data, int size)
{
    if (theNetwork.m_bNetwork)
        func_0030CEF0(&theNetwork, 64, theNetwork.m_GameConnectionIndex, -1, data_00440E20, size,
                      data, false);
}

/* a message from the menu's list to a team or everyone (OVL1 0x2BAD40) */
enum R_SEND_TO {
    R_SEND_TO_ALL
};

struct MessagePacket {
    short who;   // 0x0 (SOCOM 1's a char)
    short to;   // 0x2
    short msg;   // 0x4
};

void SendMessageMessage(int who, R_SEND_TO to, int msg)
{
    MessagePacket p;

    p.who = who;
    p.to = to;
    p.msg = msg;
    func_0030CEF0(&theNetwork, 64, theNetwork.m_GameConnectionIndex, -1, data_00440E38, 6, &p,
                  false);
}

/* (SOCOM II's: a flag handed on inverted; OVL1 0x2BADA0) */
extern "C" void func_002BADA0()
{
    func_002C0040(!data_004412D8);
}

/* the answer to a pickup request (OVL1 0x2BA260) */
struct MayIPacket {
    int m_0;   // 0x0
    unsigned char m_4[0x10];   // 0x4
    unsigned short m_14;   // 0x14
    unsigned char m_16;   // 0x16
};

int PickupMayIPacketHandler(int, int, int, void *data)
{
    MayIPacket *p = (MayIPacket *)data;
    CZSealBody *body = CZSealState::GetBodyFromObjectIndex((unsigned short)func_00541CC0(p->m_0));

    if (body)
        func_0059E830(body, p->m_16, p->m_0, p->m_4, p->m_14);
    return 24;
}

/* the pickup's acknowledgement (OVL1 0x2BA1F0): SOCOM II's packet is 8 bytes, the id first */
union AckUnion {
    unsigned char b;
};

struct PickupAckPacket {
    int id;   // 0x0
    unsigned short obj;   // 0x4
    unsigned char who;   // 0x6
    AckUnion ack;   // 0x7
};

void SendPickupAckMessage(unsigned short who, int id, int obj, AckUnion ack)
{
    PickupAckPacket p;

    p.who = who;
    p.id = id;
    p.obj = obj;
    p.ack = ack;
    func_0030CEF0(&theNetwork, 64, theNetwork.m_GameConnectionIndex, -1, data_00440E60, 8, &p,
                  false);
}

/* a vote, sent to the voter's own client (OVL1 0x2BA140) */
struct VotePacket {
    short who;   // 0x0
    unsigned char vote;   // 0x2
};

void SendVoteMessage(int who, int voter, bool vote)
{
    VotePacket p;

    p.who = who;
    p.vote = vote;
    CZSealBody *body = CZSealState::GetBodyFromObjectIndex(voter);

    if (body)
        func_0030CEF0(&theNetwork, 64, theNetwork.m_GameConnectionIndex, body->m_fc8,
                      data_00440E80, 4, &p, true);
}

/* the bomb's new state asked for (OVL1 0x2B9A00) */
struct BombStatePacket {
    CPnt3D pos;   // 0x0
    unsigned short who;   // 0xc
    unsigned char state;   // 0xe
};

int BombStatePacketHandler(int, int, int, void *data)
{
    BombStatePacket *p = (BombStatePacket *)data;
    CPnt3D pos(p->pos.x, p->pos.y, p->pos.z);

    CZBombState::HandleBombStateRequest(p->who, p->state, &pos);
    return 16;
}

/* (SOCOM II's: a lobby notice when both halves of a packet agree; OVL1 0x2B98B0) */
struct LobbyPacket {
    int kind;   // 0x0
    int a;   // 0x4
    int b;   // 0x8
};

extern "C" int func_002B98B0(int, int, int who, void *data)
{
    LobbyPacket *p = (LobbyPacket *)data;

    switch (p->kind) {
    case 1:
        if (p->a == p->b)
            func_002F9690(theLobby, func_002C3B10(who), 2);
        break;
    }
    return 12;
}

/* the local SEAL's network object (SOCOM II's piece of SOCOM 1's ftsNetworkSealInit; also called
 * from fts_mission.cpp; OVL1 0x2B7CF0) */
extern "C" void func_002B7CF0(void *, CZSealBody *body)
{
    if (!(theNetwork.m_bNetwork && theNetwork.m_bSessionMaster))
        return;
    body->m_4 = CZSealState::CreateLocalSeal(body, false);
    func_002BD640(body, 0);
}

/* a vehicle's network object (SOCOM II's; OVL1 0x2B7C90) */
extern "C" void func_002B7C90(CZVehicle *vehicle)
{
    if (!(theNetwork.m_bNetwork && theNetwork.m_bSessionMaster && vehicle))
        return;
    vehicle->m_4 = func_002C7BF0(vehicle);
}

/* the game's and the persona's network objects made (OVL1 0x2B7A20) */
void ftsNetworkGameInit()
{
    CZNetGame *game = theMission.m_pNetGame;

    if (theNetwork.m_bSessionMaster) {
        game->m_game_object_index = CZGameState::CreateLocalGame(game);
        func_002C00F0(game);
    }
    if (!game->m_persona_created) {
        game->m_persona_object_index = func_002C5450();
        game->m_persona_created = true;
        func_002C5360();
    }
}

/* (OVL1 0x2B79F0) */
void ftsNetworkGameUninit()
{
    func_002C2BB0();
    func_005423E0(0, 0.0f);
}

/* (SOCOM II's: every network object of kind 'T' handed to OVL2; OVL1 0x2B7950) */
extern "C" void func_002B7950()
{
    for (int i = 0; i < 4096; i++) {
        NetObject *o = func_0030D630(&theNetwork, i);

        if (o) {
            switch (o->m_type) {
            case 'B':
            case 'G':
            case 'K':
            case 'P':
                break;
            case 'T':
                func_00623470(i);
                break;
            case 'S':
                break;
            }
        }
    }
}
