/* fts_remote.cpp -- the online game's glue: the network's set up and tear down, the DME
 * connection, and the application packets' senders and handlers (SOCOM 1's fts_remote.cpp:
 * its static initialiser builds the broadcast schedules, as SOCOM II's 0x400400 does; OVL1
 * 0x2B7950-0x2BD37F, after the 2D messages' unit and before fts_net_seal.cpp). Defined in
 * reverse of their image order (-inline deferred).
 *
 * The network object: SOCOM II's theNetwork is at 0x45A0C0, not data.txt's 0x45A0B8 -- every
 * sender passes 0x45A0C0 as CZNetwork::zNetSendApplicationMessage's `this` (SOCOM 1's pass
 * theNetwork itself), and the session-master and network flags SOCOM 1 keeps at 8 and 9 are at
 * 0x45A0C0 and 0x45A0C1, so SOCOM II dropped the two team indices ahead of them (the map row
 * has moved). */

#include "../zmath/zmath.h"

/* the network (SOCOM 1's CZNetwork, at SOCOM II's offsets: only what this unit touches) */
class CZNetwork {
public:
    unsigned char m_bSessionMaster;   // 0x0 (SOCOM 1's at 0x8)
    unsigned char m_bNetwork;   // 0x1 (SOCOM 1's at 0x9)
    unsigned char _2[0xf4 - 0x2];
    int m_GameConnectionIndex;   // 0xf4 (SOCOM 1's at 0xd0)

    /* the game connection (an accessor: a sender loads it ahead of an out-of-line call among its
     * arguments, NetSendRequestTeamSwitch's master index, which only an inline explains) */
    int GetGameConnectionIndex() { return m_GameConnectionIndex; }

    void zNetSendApplicationMessage(char a, int conn, int to, int type, int size,
                                    unsigned char *data, bool reliable);
    int zNetGetValidGameClientCount();
    int zNetGetGameSessionMasterClientIndex();
    int zNetGetMyGameClientIndex();
    struct NetObject *zNetGetObject(int index);
};


/* a valve: a named 16-bit value whose changes call back (SOCOM 1's CValve) */
enum VALVE_STATE {
    VALVE_SAME,
    VALVE_ON,
    VALVE_OFF
};

class CValve {
public:
    static CValve *GetByName(const char *name);
    void MakeCallbacks(VALVE_STATE state);

    /* the value set, its callbacks told whether it went on or off (inlined everywhere: SOCOM 1's
     * zRemoteClientJoinsCallback has the same code; the name ours) */
    void SetValue(int value)
    {
        VALVE_STATE state = VALVE_SAME;

        if (m_value != value)
            state = value == 0 ? VALVE_OFF : VALVE_ON;
        m_value = value;
        if (state != VALVE_SAME)
            MakeCallbacks(state);
    }

    char *m_name;   // 0x0
    unsigned int m_value : 16;   // 0x4
    unsigned int m_type : 8;   // 0x4
    unsigned int m_name_pooled : 1;   // 0x4
    unsigned int m_pooled : 1;   // 0x4
    unsigned int m_unused : 6;   // 0x4
};

class CEntityCtrl {
public:
    virtual void _slot0();
    virtual void _slot1();
    virtual void _slot2();
    virtual void _slot3();
    virtual void _slot4();
    virtual void _slot5();
    virtual void _slot6();
    virtual void _slot7();
    virtual void _slot8();
    virtual void _slot9();
    virtual bool IsPlayer();   // 0x30
    virtual bool IsRemote();   // 0x34 (the name ours: a remote SEAL's fire is not counted)
};

class CZSealBody {
public:
    unsigned char _0[4];
    int m_4;   // 0x4: the body's network object (SOCOM 1's m_NetObjectIndex)
    unsigned char _8[0xc0 - 0x8];
    CEntityCtrl *m_ctrl;   // 0xc0
    unsigned char _c4[0x220 - 0xc4];
    unsigned char m_fire_count;   // 0x220 (SOCOM 1's at 0x17c)
    unsigned char _221[0x228 - 0x221];
    float m_fire_y;   // 0x228 (SOCOM 1's at 0x184)
    unsigned char _22c[0xfc8 - 0x22c];
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
    unsigned char _0[0x44];
    CValve *m_pPlayerJoinCountValve;   // 0x44 (SOCOM 1's at 0x40)
    unsigned char _48[0xac - 0x48];
    int m_persona_object_index;   // 0xac (SOCOM 1's at 0x70)
    int m_game_object_index;   // 0xb0 (SOCOM 1's at 0x74)
    unsigned char _b4[0xcf - 0xb4];
    unsigned char m_persona_created;   // 0xcf (SOCOM 1's at 0x97)
    unsigned char _d0[0x111 - 0xd0];
    char m_111;   // 0x111 (SOCOM 1's at 0xd9: m_game_type)
    unsigned char _112;
    unsigned char m_mp_major_game_state;   // 0x113 (SOCOM 1's at 0xdb)
};

class CMission {
public:
    unsigned char _0[0x1c0];
    unsigned char m_messenger[0x1808 - 0x1c0];   // 0x1c0: the mission's message queue (SOCOM 1's at 0x100)
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
    static void UpdateLocalGame(CZNetGame *game);
    static int CreateLocalGame(CZNetGame *game);
};

class CZPersonaState {
public:
    static void ClearPersonaList();
    static void UpdateLocalPersona();
    static int CreateLocalPersona();
};

/* the online lobby (only its Medius connection) */
class CZOnlineLobby {
public:
    int MediusConnectionIndex();
};

/* a map's action (only the bits the action packet sets; SOCOM 1's at 0x2C) */
class CNodeAction {
public:
    static CNodeAction *GetActionById(unsigned int id);

    unsigned char _0[0x38];
    int m_38;   // 0x38
    unsigned char _3c[3];
    unsigned char m_3f_0 : 1;
    unsigned char m_network_log : 1;   // 0x3f, bit 1 (SOCOM 1's m_network_log)
};

class CZAnimMain;
extern CZAnimMain ZAnim;

const char *GetDatabase();

/* an anim's extra data (only its flags: bit 0 an anim the network plays) */
class CAnimAuxData {
public:
    static CAnimAuxData *Get(unsigned int anim);

    bool IsNetworked() const { return m_flags & 1; }   /* (the name ours) */

    unsigned char m_flags;   // 0x0
};
CZSealBody *ftsGetPlayer();
void *_malloc(unsigned int size, const char *file, int line);
void _free(void *p, const char *file, int line);
extern "C" void *memcpy(void *dst, const void *src, unsigned int n);
extern "C" char *strcpy(char *dst, const char *src);

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
extern CZNetwork theNetwork;   /* see the head of the file */
extern unsigned char data_004412D8;
extern CZOnlineLobby theLobby;

void NetFreeObject(int index);
void func_005423E0(int a, float b);
void func_002BD640(CZSealBody *body, int arg);   /* CZSealState::UpdateLocalSeal */
int func_002C7BF0(CZVehicle *vehicle);
int func_002C3B10(int arg);
void func_002F9690(void *lobby, int arg, int mode);
int func_00541CC0(int id);
void func_0059E830(CZSealBody *body, unsigned short who, int id, void *c, int obj);
void func_002C0040(bool on);
void func_002C4290(int me);   /* CZPersonaState::RequestTeamSwitch (SOCOM II's one argument) */
int func_002BAF30(int a, int b, int who, void *data);   /* E3IndexPacketHandler */
int func_002B9BD0(int a, int b, int who, void *data);
int func_002BD420(int who);
int func_00273950(CZAnimMain *zanim, void *data);
void func_00273780(CZAnimMain *zanim, void *data, int who);
void func_00545530(void *list, CZSealBody *body, int arg);
void func_005880E0(CZSealBody *body, short a, unsigned char b, bool c, unsigned short d);
void func_0059E9E0(CZSealBody *body, unsigned char a, void *b, void *c, int d, CPnt3D *pos);
void func_0030D870(CZNetwork *net, void *msg);
void func_00590AC0(CZSealBody *a, CZSealBody *b, unsigned char c, int d, int e);
void func_002B6530(void *queue, const char *text, int a, int b, int c, float d);   /* C2DMessage_Q::Add_Message */
bool func_002C2FA0();
void func_002A47B0(CMission *mission, const char *dialog);
extern int data_00440EC0[];
void func_005B1130(unsigned char a, unsigned short b);
void func_005B1350(CZSealBody *body, unsigned char a, unsigned short b);
bool func_002C2FD0(CZSealBody *body);
int func_002C3920();
void func_002C3A20(int who, bool vote);
void func_002C3550();
int sprintf(char *buf, const char *fmt, ...);
bool func_002C31A0();
bool func_002C3080();
void func_005E7F20(CZSealBody *body, short msg, int arg);
int func_0030D380(CZNetwork *net, void (*connects)(int, int, const char *),
                  void (*disconnects)(int, int, const char *));
void func_002C3330(int arg);
void func_002BC620(int index, int client, const char *name);   /* zRemoteClientConnectsCallback */
void func_002BC530(int index, int client, const char *name);   /* zRemoteClientDisconnectsCallback */
   /* (SOCOM II's: a word per client, cleared when one joins) */

/* the application packets' type ids (SOCOM 1's PacketType<name>) */
extern unsigned int data_00440E20;   /* PacketTypeZAnim */
extern unsigned int data_00440E38;   /* PacketTypeMessage */
extern unsigned int data_00440E60;   /* PacketTypePUAck */
extern unsigned int data_00440E80;   /* PacketTypeVoter */
extern unsigned int data_00440E30;   /* PacketTypeE3Index */
extern unsigned int data_00440E88;   /* (SOCOM II's) */
extern unsigned int data_00440E98;   /* PacketTypeTeamSwitch */
extern unsigned int data_00440EB0;   /* PacketTypeGameSignal */
extern unsigned int data_00440E78;   /* (SOCOM II's: a variable-length packet) */
extern unsigned int data_00440EA8;   /* PacketTypeNewBombState */
extern unsigned int data_00440E70;   /* (SOCOM II's) */
extern unsigned int data_00440E58;   /* PacketTypePUMayI */
extern unsigned int data_00440E68;   /* (SOCOM II's) */
extern unsigned int data_00440E50;   /* PacketTypeAmmoPU */
extern unsigned int data_00440E18;   /* PacketTypePushBlendAnim */
extern unsigned int data_00440EB8;   /* (SOCOM II's: the vote's tally) */
extern unsigned int data_00440E28;   /* PacketTypeAiOrders */
}


/* an int clamped into [lo, hi], saying whether it moved (a template instance closing the unit,
 * its name unknown; OVL1 0x2BD220) */
extern "C" bool func_002BD220(int &v, const int &lo, const int &hi)
{
    if (hi < v) {
        v = hi;
        return true;
    }
    if (v < lo) {
        v = lo;
        return true;
    }
    return false;
}

/* a client joined: its word cleared and the join count's valve set (OVL1 0x2BC490) */
void zRemoteClientJoinsCallback(int index, int client)
{
    if (index != theLobby.MediusConnectionIndex()) {
        data_00440EC0[client] = 0;
        theMission.m_pNetGame->m_pPlayerJoinCountValve->SetValue(
            theNetwork.zNetGetValidGameClientCount());
    }
}

/* a client left: a leave message queued for the network (OVL1 0x2BC400) */
struct NetMessage {
    int kind;   // 0x0
    int client;   // 0x4
    unsigned char _8[0x1c - 0x8];
};

void zRemoteClientLeavesCallback(int index, int client)
{
    if (index != theLobby.MediusConnectionIndex()) {
        NetMessage m = {0};

        m.kind = 2;
        m.client = client;
        func_0030D870(&theNetwork, &m);
    }
}

/* a local SEAL's shot counted, its height kept (OVL1 0x2BBF10) */
void SaveFireMessage(CZSealBody *body, const CPnt3D *pos)
{
    if (!(body->m_ctrl ? body->m_ctrl->IsRemote() : false)) {
        body->m_fire_count++;
        if (body->m_fire_count > 200)
            body->m_fire_count = 0;
        body->m_fire_y = pos->y;
    }
}

/* a blended anim pushed to everyone, for a SEAL the network plays (SOCOM 1's NetSendPushBlend,
 * SOCOM II's with a time; OVL1 0x2BBAF0) */
struct PushBlendRequest {
    short obj;   // 0x0
    short anim;   // 0x2
    short time;   // 0x4
    unsigned char slot;   // 0x6
    unsigned char on;   // 0x7
};

extern "C" void func_002BBAF0(unsigned int obj, unsigned int anim, int slot, bool on, int time)
{
    CZSealBody *body = CZSealState::GetBodyFromObjectIndex(obj);

    if (!(theNetwork.m_bNetwork && !func_002C2FD0(body)))
        return;
    if (CAnimAuxData::Get(anim)->IsNetworked() != false) {
        PushBlendRequest p;

        p.obj = obj;
        p.anim = anim;
        p.time = time;
        p.slot = slot;
        p.on = on;
        theNetwork.zNetSendApplicationMessage(64, theNetwork.GetGameConnectionIndex(), -1,
            data_00440E18, 8, (unsigned char *)&p, false);
    }
}

/* a blended anim pushed on a remote SEAL, in a running game (OVL1 0x2BBA80) */
struct PushBlendPacket {
    unsigned short obj;   // 0x0
    short anim;   // 0x2
    unsigned short time;   // 0x4
    unsigned char slot;   // 0x6
    unsigned char on;   // 0x7
};

int PushBlendAnimPacketHandler(int, int, int, void *data)
{
    CZNetGame *game = theMission.m_pNetGame;

    if (game && game->m_mp_major_game_state == 3) {
        PushBlendPacket *p = (PushBlendPacket *)data;
        CZSealBody *body = CZSealState::GetBodyFromObjectIndex(p->obj);
        bool on = p->on > 0;

        if (body)
            func_005880E0(body, p->anim, p->slot, on, p->time);
    }
    return 8;
}

/* an order for a SEAL the network plays, sent to the session master -- a move order with its
 * place, to the metre (SOCOM 1's NetSendAiOrders; OVL1 0x2BB970) */
struct AiOrdersPacket {
    short obj;   // 0x0
    short target;   // 0x2
    unsigned char order;   // 0x4
    char x;   // 0x5
    char z;   // 0x6
};

extern "C" void func_002BB970(unsigned int obj, unsigned int target, unsigned int order,
                              const CPnt3D *pos)
{
    CZSealBody *body = CZSealState::GetBodyFromObjectIndex(obj);

    if (!(theNetwork.m_bNetwork && !func_002C2FD0(body)))
        return;
    AiOrdersPacket p;

    p.obj = obj;
    p.target = target;
    p.order = order;
    switch ((unsigned char)order) {
    case 4:
        if (pos) {
            p.x = pos->x;
            p.z = pos->z;
        }
        break;
    }
    theNetwork.zNetSendApplicationMessage(64, theNetwork.GetGameConnectionIndex(),
        theNetwork.zNetGetGameSessionMasterClientIndex(), data_00440E28, 8, (unsigned char *)&p,
        false);
}

/* (SOCOM II's: one SEAL acting on another, in a running game; OVL1 0x2BB8E0) */
struct SealPairPacket {
    unsigned short a;   // 0x0
    unsigned short b;   // 0x2
    unsigned char arg;   // 0x4
};

extern "C" int func_002BB8E0(int, int, int, void *data)
{
    CZNetGame *game = theMission.m_pNetGame;

    if (game && game->m_mp_major_game_state == 3) {
        SealPairPacket *p = (SealPairPacket *)data;
        CZSealBody *a = CZSealState::GetBodyFromObjectIndex(p->a);
        CZSealBody *b = CZSealState::GetBodyFromObjectIndex(p->b);

        if (a && b)
            func_00590AC0(b, a, p->arg, 0, 0);
    }
    return 8;
}

/* the ZAnim packet: its body as the caller built it (OVL1 0x2BB880) */
void SendZAnimNetworkPacket(void *data, int size)
{
    if (theNetwork.m_bNetwork)
        theNetwork.zNetSendApplicationMessage(64, theNetwork.GetGameConnectionIndex(), -1,
            data_00440E20, size, (unsigned char *)data, false);
}

/* a ZAnim packet from another client: its size handed back, and played when the game is running
 * (OVL1 0x2BB7F0) */
int ZAnimNetworkPacketHandler(int, int, int who, void *data)
{
    int body = func_002BD420(who);
    int size = func_00273950(&ZAnim, data);
    CZNetGame *game = theMission.m_pNetGame;

    if (game && game->m_mp_major_game_state == 3)
        func_00273780(&ZAnim, data, body);
    return size;
}

/* an action run by a remote SEAL (OVL1 0x2BB770) */
struct ActionAnimPacket {
    unsigned short obj;   // 0x0
    unsigned short action;   // 0x2
};

int ActionAnimPacketHandler(int, int, int, void *data)
{
    ActionAnimPacket *p = (ActionAnimPacket *)data;
    CZSealBody *body = CZSealState::GetBodyFromObjectIndex(p->obj);
    CNodeAction *action = CNodeAction::GetActionById(p->action);

    if (action && body) {
        func_00545530((char *)body + 0x58c, body, action->m_38);
        action->m_network_log = 1;
    }
    return 4;
}

/* the E3 index sent: handled here when it is our own (OVL1 0x2BAEA0) */
struct E3IndexPacket {
    unsigned int index;   // 0x0
    unsigned char flag;   // 0x4
};

void NetSendE3Index(int who, unsigned int index, int flag, bool)
{
    E3IndexPacket p;

    p.index = index;
    p.flag = flag;
    if (who == theNetwork.zNetGetMyGameClientIndex())
        func_002BAF30(0, 0, theNetwork.zNetGetMyGameClientIndex(), &p);
    else
        theNetwork.zNetSendApplicationMessage(64, theNetwork.GetGameConnectionIndex(), who,
            data_00440E30, 8, (unsigned char *)&p, false);
}

/* a team switch asked for: the session master grants its own, a client asks the master
 * (OVL1 0x2BADD0) */
void NetSendRequestTeamSwitch(int team)
{
    if (theNetwork.m_bSessionMaster) {
        int me = theNetwork.zNetGetMyGameClientIndex();

        if (theNetwork.m_bSessionMaster)
            func_002C4290(me);
    } else {
        theNetwork.zNetSendApplicationMessage(64, theNetwork.GetGameConnectionIndex(),
            theNetwork.zNetGetGameSessionMasterClientIndex(), data_00440E98, 4,
            (unsigned char *)&team, false);
    }
}

/* (SOCOM II's: a flag handed on inverted; OVL1 0x2BADA0) */
extern "C" void func_002BADA0()
{
    func_002C0040(!data_004412D8);
}

/* a message from the menu's list to a team or everyone (OVL1 0x2BAD40) */
enum R_SEND_TO {
    R_SEND_TO_3 = -3,
    R_SEND_TO_2,
    R_SEND_TO_ALL
};

struct MessagePacket {
    short who;   // 0x0 (SOCOM 1's a char)
    unsigned short to;   // 0x2: a R_SEND_TO, or the one SEAL's network object
    short msg;   // 0x4
};

void SendMessageMessage(int who, R_SEND_TO to, int msg)
{
    MessagePacket p;

    p.who = who;
    p.to = to;
    p.msg = msg;
    theNetwork.zNetSendApplicationMessage(64, theNetwork.GetGameConnectionIndex(), -1,
        data_00440E38, 6, (unsigned char *)&p, false);
}

/* a menu message from another client, shown when it is for everyone, our team, or us
 * (SOCOM 1's MessagePacketHandler; OVL1 0x2BAC40). The group taken into a local before the body
 * test reproduces the target's lb in the delay slot and its re-extension (dsll32/dsra32 24, the
 * conversion to the signed byte enum); the player taken into a local keeps the halfword's reload
 * after the call (in the comparison itself MW loads the halfword ahead of the call and keeps it in
 * s2), and `p->to == player->m_4` puts the index in v0, the halfword in v1 (the player's index in
 * an int local first swaps them). */
extern "C" int func_002BAC40(int, int, int, void *data)
{
    MessagePacket *p = (MessagePacket *)data;
    CZSealBody *body = CZSealState::GetBodyFromObjectIndex((unsigned short)p->who);
    R_SEND_TO to = (R_SEND_TO)p->to;

    if (body) {
        switch (to) {
        case R_SEND_TO_3:
            if (func_002C31A0())
                func_005E7F20(body, p->msg, 1);
            break;
        case R_SEND_TO_2:
            if (func_002C3080())
                func_005E7F20(body, p->msg, 1);
            break;
        case R_SEND_TO_ALL:
            func_005E7F20(body, p->msg, 1);
            break;
        default: {
            CZSealBody *player = ftsGetPlayer();

            if (p->to == player->m_4)
                func_005E7F20(body, p->msg, 1);
            break;
        }
        }
    }
    return 6;
}

/* an ammo pickup told to everyone (SOCOM 1's SendAmmoPUMessage; OVL1 0x2BA3F0) */
struct AmmoPURequest {
    int id;   // 0x0
    unsigned short a[8];   // 0x4
    unsigned short b[8];   // 0x14
    PNT3D pos;   // 0x24
    unsigned char who;   // 0x30
};

extern "C" void func_002BA3F0(int who, const unsigned short *a, const unsigned short *b, int id,
                              const CPnt3D *pos)
{
    AmmoPURequest p;

    p.who = who & 0xf;
    for (int i = 0; i < 8; i++) {
        p.a[i] = a[i];
        p.b[i] = b[i];
    }
    p.id = id;
    p.pos = *pos;
    theNetwork.zNetSendApplicationMessage(64, theNetwork.GetGameConnectionIndex(), -1,
        data_00440E50, 52, (unsigned char *)&p, false);
}

/* an ammo pickup from another client (OVL1 0x2BA370) */
struct AmmoPUPacket {
    int id;   // 0x0
    unsigned char m_4[0x10];   // 0x4
    unsigned char m_14[0x10];   // 0x14
    float x, y, z;   // 0x24
    unsigned char who;   // 0x30
};

int AmmoPUPacketHandler(int, int, int, void *data)
{
    AmmoPUPacket *p = (AmmoPUPacket *)data;
    CPnt3D pos(p->x, p->y, p->z);
    CZSealBody *body = CZSealState::GetBodyFromObjectIndex(func_00541CC0(p->id));

    if (body)
        func_0059E9E0(body, p->who, p->m_4, p->m_14, p->id, &pos);
    return 52;
}

/* a pickup asked of the item's owner (SOCOM 1's SendMayIPickupMessage, its packet SOCOM II's 24
 * bytes; OVL1 0x2BA2C0) */
struct MayIRequest {
    int id;   // 0x0
    unsigned short ids[8];   // 0x4
    short obj;   // 0x14
    unsigned char who;   // 0x16
};

extern "C" void func_002BA2C0(int who, int id, const unsigned short *ids, int obj, int owner)
{
    MayIRequest p;

    p.who = who & 0xf;
    p.id = id;
    p.obj = obj;
    for (int i = 0; i < 8; i++)
        p.ids[i] = ids[i];
    CZSealBody *body = CZSealState::GetBodyFromObjectIndex(owner);

    if (body)
        theNetwork.zNetSendApplicationMessage(64, theNetwork.GetGameConnectionIndex(), body->m_fc8,
            data_00440E58, 24, (unsigned char *)&p, false);
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
    theNetwork.zNetSendApplicationMessage(64, theNetwork.GetGameConnectionIndex(), -1,
        data_00440E60, 8, (unsigned char *)&p, false);
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
        theNetwork.zNetSendApplicationMessage(64, theNetwork.GetGameConnectionIndex(), body->m_fc8,
            data_00440E80, 4, (unsigned char *)&p, true);
}

/* (SOCOM II's: a vote counted -- the tally told to the player and everyone; OVL1 0x2BA040) */
struct VoteTally {
    int kind;   // 0x0
    int before;   // 0x4
    int after;   // 0x8
};

extern "C" int func_002BA040(int, int, int who, void *data)
{
    VotePacket *p = (VotePacket *)data;
    int before = func_002C3920();

    func_002C3A20(who, p->vote);
    int after = func_002C3920();
    char text[128];

    sprintf(text, "Voting: You have %d votes against you.", after);
    func_002B6530(theMission.m_messenger, text, 2, 0, 0, 0.0f);
    func_002C3550();
    VoteTally m = {0};

    m.kind = 1;
    m.before = before;
    m.after = after;
    theNetwork.zNetSendApplicationMessage(64, theNetwork.GetGameConnectionIndex(), -1,
        data_00440EB8, 12, (unsigned char *)&m, false);
    return 4;
}

/* (SOCOM II's: a SEAL's byte and short sent to the session master -- handled here when we are it;
 * OVL1 0x2B9F60) */
struct SealBytePacket {
    unsigned short obj;   // 0x0
    short a;   // 0x2
    unsigned short b;   // 0x4
};

extern "C" int func_002B9F10(int, int, int, void *data);

extern "C" void func_002B9F60(int obj, int a, int b)
{
    SealBytePacket p = {0};

    p.obj = obj;
    p.a = a;
    p.b = b;
    if (theNetwork.m_bSessionMaster)
        func_002B9F10(0, 0, theNetwork.zNetGetMyGameClientIndex(), &p);
    else
        theNetwork.zNetSendApplicationMessage(64, theNetwork.GetGameConnectionIndex(),
            theNetwork.zNetGetGameSessionMasterClientIndex(), data_00440E68, 6, (unsigned char *)&p,
            false);
}

/* (its handler; OVL1 0x2B9F10) */
extern "C" int func_002B9F10(int, int, int, void *data)
{
    SealBytePacket *p = (SealBytePacket *)data;
    CZSealBody *body = CZSealState::GetBodyFromObjectIndex(p->obj);

    if (body)
        func_005B1350(body, p->a, p->b);
    return 6;
}

/* (SOCOM II's: a SEAL's two shorts sent to its own client -- the player's handled here when we
 * are the session master; OVL1 0x2B9E50) */
struct ShortPairPacket {
    short a;   // 0x0
    unsigned short b;   // 0x2
};

extern "C" int func_002B9E20(int, int, int, void *data);

extern "C" void func_002B9E50(CZSealBody *body, int a, int b)
{
    if (!body)
        return;
    ShortPairPacket p = {0};

    p.a = a;
    p.b = b;
    if (theNetwork.m_bSessionMaster && body == ftsGetPlayer())
        func_002B9E20(0, 0, theNetwork.zNetGetMyGameClientIndex(), &p);
    else
        theNetwork.zNetSendApplicationMessage(64, theNetwork.GetGameConnectionIndex(), body->m_fc8,
            data_00440E70, 4, (unsigned char *)&p, false);
}

/* (its handler; OVL1 0x2B9E20) */
extern "C" int func_002B9E20(int, int, int, void *data)
{
    ShortPairPacket *p = (ShortPairPacket *)data;

    func_005B1130(p->a, p->b);
    return 4;
}

/* (SOCOM II's: the spectators thrown out -- a message, the spectator valve cleared, and a banned
 * client sent to its dialog; OVL1 0x2B9D80) */
extern "C" int func_002B9D80(int, int, int, void *)
{
    func_002B6530(theMission.m_messenger, "Spectators EJECTED from this game.", 2, 0, 0, 0.0f);
    CValve *valve = CValve::GetByName("MaxSpectatorsValve");

    if (valve)
        valve->SetValue(0);
    if (func_002C2FA0())
        func_002A47B0(&theMission, "dlgNetBanned.rdr");
    return 4;
}

/* (SOCOM II's: a vote-like packet to everyone, and handled here too; OVL1 0x2B9B60) */
extern "C" void func_002B9B60(int who, int vote)
{
    VotePacket p;

    p.who = who;
    p.vote = vote;
    theNetwork.zNetSendApplicationMessage(64, theNetwork.GetGameConnectionIndex(), -1,
        data_00440E88, 4, (unsigned char *)&p, true);
    func_002B9BD0(0, 0, theNetwork.zNetGetMyGameClientIndex(), &p);
}

/* (SOCOM II's: a variable-length packet -- its length, three bytes, then the caller's data --
 * built on the heap; OVL1 0x2B9A40) */
extern "C" void func_002B9A40(int to, unsigned char a, unsigned char b, unsigned char c, int size,
                              void *data)
{
    int total = size + 6;
    char *buf = (char *)_malloc(total, 0, 0);

    *(short *)buf = total;
    buf[2] = a;
    buf[3] = b;
    buf[4] = c;
    memcpy(buf + 5, data, size);
    theNetwork.zNetSendApplicationMessage(64, theNetwork.GetGameConnectionIndex(), to,
        data_00440E78, total, (unsigned char *)buf, true);
    _free(buf, 0, 0);
}

/* the bomb's new state asked for (OVL1 0x2B9A00) */
struct BombStatePacket {
    PNT3D pos;   // 0x0
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

/* the bomb's new state asked of the session master -- handled at once when we are it
 * (OVL1 0x2B9910) */
void SendBombStateRequest(int who, unsigned char state, const CPnt3D *pos)
{
    BombStatePacket p = {0};

    p.pos = *pos;
    p.who = who;
    p.state = state;
    if (theNetwork.m_bSessionMaster)
        BombStatePacketHandler(0, 0, theNetwork.zNetGetMyGameClientIndex(), &p);
    else
        theNetwork.zNetSendApplicationMessage(64, theNetwork.GetGameConnectionIndex(),
            theNetwork.zNetGetGameSessionMasterClientIndex(), data_00440EA8, 16,
            (unsigned char *)&p, false);
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
            func_002F9690(&theLobby, func_002C3B10(who), 2);
        break;
    }
    return 12;
}

/* a game signal to one client: the mission's database and the game's type with it
 * (OVL1 0x2B97F0) */
struct GameSignalPacket {
    char database[8];   // 0x0
    int arg;   // 0x8
    unsigned char a;   // 0xc
    unsigned char b;   // 0xd
    unsigned char c;   // 0xe
    char game_type;   // 0xf
    unsigned char flag;   // 0x10
};

void SendGameSignalMessage(int to, unsigned char a, unsigned char b, unsigned char c, int arg, bool)
{
    if (theNetwork.m_bSessionMaster) {
        /* MW zeroes an aggregate initialiser byte by byte, testing the address first */
        GameSignalPacket p = {0};

        p.a = a;
        p.b = b;
        p.c = c;
        p.arg = arg;
        p.flag = data_004412D8;
        p.game_type = theMission.m_pNetGame->m_111;
        strcpy(p.database, GetDatabase());
        theNetwork.zNetSendApplicationMessage(64, theNetwork.GetGameConnectionIndex(), to,
            data_00440EB0, 20, (unsigned char *)&p, false);
    }
}

/* the DME's set up: no session master yet, every client's word cleared, the connect and
 * disconnect callbacks registered (SOCOM 1's DMEinit, much grown; OVL1 0x2B85C0) */
extern "C" bool func_002B85C0()
{
    theNetwork.m_bSessionMaster = false;
    for (int i = 0; i < 24; i++)
        data_00440EC0[i] = 0;
    if (func_0030D380(&theNetwork, func_002BC620, func_002BC530))
        return false;
    func_002C3330(0);
    return true;
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
        CZGameState::UpdateLocalGame(game);
    }
    if (!game->m_persona_created) {
        game->m_persona_object_index = CZPersonaState::CreateLocalPersona();
        game->m_persona_created = true;
        CZPersonaState::UpdateLocalPersona();
    }
}

/* (OVL1 0x2B79F0) */
void ftsNetworkGameUninit()
{
    CZPersonaState::ClearPersonaList();
    func_005423E0(0, 0.0f);
}

/* (SOCOM II's: every network object of kind 'T' handed to OVL2; OVL1 0x2B7950) */
extern "C" void func_002B7950()
{
    for (int i = 0; i < 4096; i++) {
        NetObject *o = theNetwork.zNetGetObject(i);

        if (o) {
            switch (o->m_type) {
            case 'B':
            case 'G':
            case 'K':
            case 'P':
                break;
            case 'T':
                NetFreeObject(i);
                break;
            case 'S':
                break;
            }
        }
    }
}
