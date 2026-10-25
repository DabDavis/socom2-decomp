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
    unsigned char _2;
    unsigned char m_bJoined;   // 0x3: the game server joined (SOCOM II's)
    unsigned char _4[0xf4 - 0x4];
    int m_GameConnectionIndex;   // 0xf4 (SOCOM 1's at 0xd0)
    int _f8;
    int m_master_index;   // 0xfc: the session master's client

    /* the game connection (an accessor: a sender loads it ahead of an out-of-line call among its
     * arguments, NetSendRequestTeamSwitch's master index, which only an inline explains) */
    int GetGameConnectionIndex() { return m_GameConnectionIndex; }

    void zNetSendApplicationMessage(char a, int conn, int to, int type, int size,
                                    unsigned char *data, bool reliable);
    int zNetGetValidGameClientCount();
    int zNetGetGameSessionMasterClientIndex();
    int zNetGetMyGameClientIndex();
    int zNetGetTime();
    int zNetJoin(void (*joins)(int, int), void (*leaves)(int, int));
    struct NetObject *zNetGetObject(int index);

    /* an action the network put off to its next update (only what zRemoteDelayedAction reads:
     * its kind, a byte, and the client) */
    struct TypeRTimeFixStruct {
        unsigned char m_kind;   // 0x0
        int m_client;   // 0x4
    };
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

/* a SEAL's hit location (SOCOM 1's DAMAGE_LOCATION; its enumerators' names not needed here) */
enum DAMAGE_LOCATION {
    DAMAGE_LOCATION_0 = 0,
    DAMAGE_LOCATION_15 = 15
};

/* a node: only its name (znode.h's CNode, m_name at 0x90) */
struct CNodeName {
    unsigned char _0[0x90];
    char *m_name;   // 0x90
};

class CZSealBody {
public:
    unsigned char _0[4];
    int m_4;   // 0x4: the body's network object (SOCOM 1's m_NetObjectIndex)
    unsigned char _8[0x28 - 0x8];
    CNodeName *m_node;   // 0x28 (CEntity's)
    unsigned char _2c[0xc0 - 0x2c];
    CEntityCtrl *m_ctrl;   // 0xc0
    unsigned char _c4[0x220 - 0xc4];
    unsigned char m_fire_count;   // 0x220 (SOCOM 1's at 0x17c)
    unsigned char _221[0x228 - 0x221];
    float m_fire_y;   // 0x228 (SOCOM 1's at 0x184)
    unsigned char _22c[0xfc8 - 0x22c];
    int m_fc8;   // 0xfc8: the client it belongs to (SOCOM 1's at 0xeb8)
    unsigned char _fcc[0xfd4 - 0xfcc];
    int m_fd4;   // 0xfd4: when it was last hit (the network's time)
    float m_fd8;   // 0xfd8: a copy of 0x1044 at the hit
    unsigned char _fdc[0x1044 - 0xfdc];
    float m_1044;   // 0x1044

    void RemoteWeaponDamageNonVictim(DAMAGE_LOCATION where, CZSealBody *victim, CZSealBody *shooter,
                                     unsigned char c, bool flag);
};

class CZVehicle {
public:
    unsigned char _0[4];
    int m_4;   // 0x4: its network object
};

class CUIVariable;

/* the online game (SOCOM 1's CZNetGame: only what this unit touches, at SOCOM II's offsets) */
class CZNetGame {
public:
    unsigned char _0[0x44];
    CValve *m_pPlayerJoinCountValve;   // 0x44 (SOCOM 1's at 0x40)
    unsigned char _48[0x6c - 0x48];
    CValve *m_6c;   // 0x6c: set from each action packet (SOCOM II's)
    unsigned char _70[0x94 - 0x70];
    CUIVariable *m_chat;   // 0x94: the chat's history (SOCOM II's)
    unsigned char _98[0xac - 0x98];
    int m_persona_object_index;   // 0xac (SOCOM 1's at 0x70)
    int m_game_object_index;   // 0xb0 (SOCOM 1's at 0x74)
    unsigned char _b4[0xcc - 0xb4];
    bool m_late_joiner;   // 0xcc (SOCOM 1's at 0x94)
    unsigned char _cd[0xcf - 0xcd];
    unsigned char m_persona_created;   // 0xcf (SOCOM 1's at 0x97)
    unsigned char _d0[0xda - 0xd0];
    unsigned char m_da;   // 0xda: set while held actions are handled at once (SOCOM II's)
    unsigned char _db[0xde - 0xdb];
    unsigned char m_de;   // 0xde: no hits sent while set (SOCOM II's)
    unsigned char _df[0x111 - 0xdf];
    char m_111;   // 0x111 (SOCOM 1's at 0xd9: m_game_type)
    unsigned char _112;
    unsigned char m_mp_major_game_state;   // 0x113 (SOCOM 1's at 0xdb)

    bool LateJoiner(bool late);
};

class CMission {
public:
    unsigned char _0[0x1ac];
    struct CMissionInfo *m_info;   // 0x1ac: the mission's info (its halfword at 4 the game mode)
    unsigned char _1b0[0x1c0 - 0x1b0];
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
    void Censor(const char *text, void (*send)(const char *, void *), void *arg);
    bool IsIgnored(const char *name);
};

/* a map's action (only the bits the action packet sets; SOCOM 1's at 0x2C) */
class CNodeAction {
public:
    static CNodeAction *GetActionById(unsigned int id);
    void ExecuteAction(CZSealBody *body, unsigned int arg);

    /* (the name ours: the action senders test the bit materialised) */
    bool IsNetworkLogged() const { return m_network_log != 0; }

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
extern "C" unsigned int strlen(const char *s);

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
void func_002C34F0(int value);
struct VotePacket;
int func_002B9BD0(int a, int b, int who, VotePacket *p);
void func_002C2F50(int client, bool ready);
int func_002C2EF0();
bool func_002C2E30();
extern unsigned int data_00440E90;   /* (SOCOM II's: the spectators ejected) */
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
/* what the DME's connect and disconnect callbacks are handed (SOCOM 1's callbacks took the
 * connection, the client and its name as arguments; SOCOM II's read the first two through a
 * pointer) */
struct ClientConnectArgs {
    int m_index;   // 0x0: the connection
    int m_client;   // 0x4
    int _8[2];
    int m_10;   // 0x10: the client's word (DMEJoinGameServer hands each waiting one on)
};

int func_0030D380(CZNetwork *net, void (*connects)(ClientConnectArgs *),
                  void (*disconnects)(ClientConnectArgs *));
void func_002C3330(int arg);
void func_002BC620(ClientConnectArgs *args);   /* zRemoteClientConnectsCallback */
void func_002BC530(ClientConnectArgs *args);   /* zRemoteClientDisconnectsCallback */
extern unsigned char data_00441580[24];   /* (fts_net_persona.cpp's pending slots, a byte per client) */
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


extern "C" double fabs(double x);

/* a float field's change tested: the object's (a0's index) data present, a change of more than 4
 * sent; no change not; a smaller one only while the data's two bytes at 0x7a and 0x7c are both
 * zero (SOCOM II's; nothing in either overlay takes its address. OVL1 0x2BD270) */
struct NetFieldCompare {
    int m_index;   // 0x0: the object
    unsigned char _4[0x10 - 0x4];
    float *m_new;   // 0x10
    float *m_old;   // 0x14
};

struct NetObjectData {
    unsigned char _0[0x1c];
    signed char *m_data;   // 0x1c
};

extern "C" bool func_002BD270(NetFieldCompare *cmp)
{
    float *a = cmp->m_new;
    float *b = cmp->m_old;
    NetObjectData *obj = (NetObjectData *)theNetwork.zNetGetObject(cmp->m_index);

    if (!obj)
        return false;
    signed char *data = obj->m_data;

    if (!data)
        return false;
    float d = fabs(*a - *b);

    if (d > 4.0f)
        return true;
    if (d != 0.0f && data[0x7a] == 0.0f && data[0x7c] == 0.0f)
        return true;
    return false;
}

/* a client disconnected: its word and its pending slot cleared; in a game already under way,
 * fewer than three clients left with no word set abandons it, and otherwise the game's object
 * is updated (SOCOM 1's zRemoteClientDisconnectsCallback, grown; OVL1 0x2BC530) */
extern "C" void func_002BC530(ClientConnectArgs *args)
{
    if (args->m_index != theLobby.MediusConnectionIndex()) {
        int count = 0;

        data_00440EC0[args->m_client] = 0;
        data_00441580[args->m_client] = 0;
        CZNetGame *game = theMission.m_pNetGame;

        for (int i = 0; i < 24; i++) {
            if (data_00440EC0[i])
                count++;
        }
        if (game) {
            if (game->m_late_joiner) {
                if (theNetwork.zNetGetValidGameClientCount() - count < 3)
                    func_002A47B0(&theMission, "dlgNetAbandoned.rdr");
            } else
                CZGameState::UpdateLocalGame(game);
        }
    }
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

void zDoClientLeavesCallback(int client);
bool DMEJoinGameServer();
void ftsNetworkGameInit();

/* what the network put off, done (SOCOM 1's; OVL1 0x2BBFA0): a client gone; the game server
 * joined -- SOCOM II's join is DMEJoinGameServer, where SOCOM 1 joined here -- and a late joiner's
 * flag cleared; the game's and the persona's objects made (ftsNetworkGameInit, inlined) */
void zRemoteDelayedAction(CZNetwork::TypeRTimeFixStruct *fix)
{
    switch (fix->m_kind) {
    case 2:
        zDoClientLeavesCallback(fix->m_client);
        break;
    case 4:
        DMEJoinGameServer();
        if (theMission.m_pNetGame->m_late_joiner)
            theMission.m_pNetGame->LateJoiner(false);
        break;
    case 5:
        ftsNetworkGameInit();
        break;
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

/* a hit told to everyone: the shooter, the victim, a byte, the damage and the second value (times
 * 20) as bytes, the body part in four bits and the flag beside it, each number saturated (SOCOM
 * 1's, the packet two bytes longer); the victim's hit time and value kept (OVL1 0x2BBDA0) */
struct HitPacket {
    unsigned short shooter;   // 0x0
    unsigned short victim;   // 0x2
    unsigned char c;   // 0x4
    unsigned char damage;   // 0x5
    unsigned char m_6;   // 0x6
    unsigned char part : 4;   // 0x7
    unsigned char flag : 1;   // 0x7
};

extern "C" {
extern unsigned int data_00440E00;   /* PacketTypeHit */
}

void SendHitMessage(int shooter, int victim, int part, float damage, float f, bool flag,
                    unsigned char c)
{
    if (theMission.m_pNetGame->m_de)
        return;
    int d;
    int g;
    HitPacket p;

    p.shooter = shooter;
    p.victim = victim;
    p.c = c;
    d = (int)damage;
    _saturate(&d, 0, 255);
    p.damage = d;
    g = (int)(20.0f * f);
    _saturate(&g, 0, 255);
    p.m_6 = g;
    _saturate(&part, 0, 15);
    p.part = part;
    p.flag = flag;
    theNetwork.zNetSendApplicationMessage(64, theNetwork.GetGameConnectionIndex(), -1,
        data_00440E00, 8, (unsigned char *)&p, false);
    CZSealBody *body = CZSealState::GetBodyFromObjectIndex(victim);

    if (body) {
        body->m_fd8 = body->m_1044;
        body->m_fd4 = theNetwork.zNetGetTime();
    }
}

/* the mission's info: its game mode (1 and 2 take no hits from the network) */
struct CMissionInfo {
    unsigned int _0;
    unsigned short m_mode;   // 0x4
};

extern "C" void func_005A1B80(CZSealBody *body, DAMAGE_LOCATION where, CZSealBody *victim,
                              CZSealBody *shooter, float damage, float f, unsigned char c,
                              bool flag);   /* (the victim's own damage, OVL2) */

/* a hit: in a game under way, outside game modes 1 and 2, the victim takes it -- its own client's
 * damage for a local victim, the remote one's for a remote victim (SOCOM 1's; OVL1 0x2BBBD0).
 * OPEN, 11 instructions, registers only. The mode test matches with the mode passed to the inline as
 * an `int` (round 13: the parameter takes the loaded halfword, which frees v0 for the constant 1 and
 * a0 for the bool; read inside the inline -- straight, through a mode local, as a member of the
 * info, or as an unsigned short parameter -- they land in a0/a1, and round 12's shapes, a switch,
 * early returns, bool locals and `||`, all miss). Left: the bitfield byte's v0/v1 trade places with
 * the 0.05 constant's (6: the target loads the constant ahead of the byte and extracts the flag
 * ahead of the part; tried the four locals in six orders, the product cast, swapped, split and
 * through a constant local, the bitfields as unsigned int, bool and signed char, the flag `!= 0`
 * and as a ternary, the part through an int or unsigned char local -- none moves them); and the
 * OVL2 damage call (0x5A1B80, still a placeholder, so called as a C function) sets a0 before a3
 * and t1 where the target -- a member call, `this` loaded last -- sets it after them (5).
 * 0x5A1B80 is SOCOM 1's RemoteWeaponDamageVictim(DAMAGE_LOCATION, float, float, CZSealBody *,
 * CZSealBody *, unsigned char, bool) (right after NonVictim in both builds, the same registers):
 * measured with the row renamed and the member called, the call's moves match and only the
 * bitfield's 6 differ. The member call waits on that OVL2 row's name (the SEAL lane's) */
/* whether the mission's mode takes no hits (the name ours: the handler's test materialised) */
inline bool NoHitMode(int mode)
{
    bool b = true;

    if (mode != 1 && mode != 2)
        b = false;
    return b;
}

int HitPacketHandler(int, int, int, void *data)
{
    if (!NoHitMode(theMission.m_info->m_mode)) {
        CZNetGame *game = theMission.m_pNetGame;

        if (game && game->m_mp_major_game_state == 3) {
            HitPacket *p = (HitPacket *)data;
            CZSealBody *victim = CZSealState::GetBodyFromObjectIndex(p->victim);
            CZSealBody *shooter = CZSealState::GetBodyFromObjectIndex(p->shooter);
            float damage = p->damage;
            float f = 0.05f * p->m_6;
            DAMAGE_LOCATION where = (DAMAGE_LOCATION)p->part;
            bool flag = p->flag;

            if (victim && shooter) {
                if (!(victim->m_ctrl ? victim->m_ctrl->IsRemote() : false))
                    func_005A1B80(victim, where, victim, shooter, damage, f, p->c, flag);
                else
                    victim->RemoteWeaponDamageNonVictim(where, victim, shooter, p->c, flag);
            }
        }
    }
    return 8;
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

void NetSendAiOrders(unsigned int obj, unsigned int target, unsigned int order, void *data)
{
    const CPnt3D *pos = (const CPnt3D *)data;
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

/* an action packet (SOCOM II's, the names ours): the SEAL's object, the action, the argument the
 * action runs with, and whether the action's valve goes on */
struct HeldActionPacket {
    unsigned short obj;   // 0x0
    unsigned short action;   // 0x2
    unsigned short m_4;   // 0x4
    unsigned short arg;   // 0x6
    unsigned short on;   // 0x8
};

/* the actions held back while the game's are not live: up to 16 action packets and a flag each
 * (SOCOM II's; the names ours) */
extern "C" {
extern HeldActionPacket data_00440F40[16];
extern unsigned char data_00440FE0[16];
extern unsigned int data_00440E08;   /* (SOCOM II's: the action packet) */
extern unsigned int data_00440E10;   /* PacketTypeActionAnim (the 4-byte packet) */
}

/* the action's anim, for an action whose anim the network plays and that has not yet been sent:
 * sent to every client and played here (SOCOM 1's NetSendActionAnim, sent and handled at once;
 * the name ours) */
inline void SendActionAnim(int obj, int id)
{
    CNodeAction *action = CNodeAction::GetActionById(id);

    if (!action || action->IsNetworkLogged() != false || !action->m_38)
        return;
    ActionAnimPacket p = {0};

    p.obj = obj;
    p.action = id;
    theNetwork.zNetSendApplicationMessage(64, theNetwork.GetGameConnectionIndex(), -1,
        data_00440E10, 4, (unsigned char *)&p, false);
    ActionAnimPacketHandler(0, 0, 0, &p);
}

/* an action sent, to one client or (-1) to all; the session master playing it to all also sends
 * and plays its anim (SOCOM II's; OVL1 0x2BB5B0) */
extern "C" void func_002BB5B0(int obj, int id, unsigned char c, int arg, int who)
{
    if (!theNetwork.m_bNetwork)
        return;
    HeldActionPacket p = {0};

    p.arg = arg;
    p.m_4 = c;
    p.obj = obj;
    p.action = id;
    if (who != -1)
        p.on = 1;
    theNetwork.zNetSendApplicationMessage(64, theNetwork.GetGameConnectionIndex(), who,
        data_00440E08, 10, (unsigned char *)&p, false);
    if (theNetwork.m_bSessionMaster && who == -1)
        SendActionAnim(obj, id);
}

/* an action packet: held while the game's held actions are not being handled (the game's flag at
 * 0xDA clear), else, in a game under way, the action's valve set, the action marked and run, and
 * its anim sent by the session master (SOCOM II's; OVL1 0x2BB310) */
extern "C" int func_002BB310(int, int, int, void *data)
{
    HeldActionPacket *p = (HeldActionPacket *)data;
    CZSealBody *body;
    CNodeAction *action;
    CZNetGame *game = theMission.m_pNetGame;

    if (game && !game->m_da) {
        int i = p->action;

        if (i >= 0 && i < 16) {
            data_00440F40[i] = *p;
            data_00440FE0[i] = 1;
        }
    } else if (game && game->m_mp_major_game_state == 3) {
        body = CZSealState::GetBodyFromObjectIndex(p->obj);
        action = CNodeAction::GetActionById(p->action);

        game->m_6c->SetValue(p->on);
        if (p->on && action)
            action->m_network_log = 1;
        if (action)
            action->ExecuteAction(body, p->arg);
        if (theNetwork.m_bSessionMaster)
            SendActionAnim(p->obj, p->action);
    }
    return 10;
}

/* the held actions dropped and the game's flag at 0xDA cleared (OVL1 0x2BB1D0) */
extern "C" void func_002BB1D0()
{
    HeldActionPacket empty = {0};

    for (int i = 0; i < 16; i++) {
        data_00440F40[i] = empty;
        data_00440FE0[i] = 0;
    }
    theMission.m_pNetGame->m_da = 0;
}

/* the game's flag at 0xDA set and the held actions handled, each then dropped (OVL1 0x2BB0C0) */
extern "C" void func_002BB0C0()
{
    HeldActionPacket empty = {0};

    theMission.m_pNetGame->m_da = 1;
    for (int i = 0; i < 16; i++) {
        if (data_00440FE0[i]) {
            func_002BB310(0, 0, 0, &data_00440F40[i]);
            data_00440F40[i] = empty;
            data_00440FE0[i] = 0;
        }
    }
}

/* the E3 index packet: SOCOM II's index 4 is an ejection, its flag the reason, shown through the
 * lobby's UI event (MP_EJECT_SPECTATOR); any other index is the client's new index and flag, the
 * persona then updated (SOCOM 1's E3IndexPacketHandler, grown; OVL1 0x2BAF30). The sixth reason's
 * format is the original's "5s" */
struct E3IndexPacket {
    unsigned int index;   // 0x0
    unsigned char flag;   // 0x4
};

extern "C" void (*data_0044F3C8)(const char *event, const char *arg);   /* (the lobby's UI event) */

int E3IndexPacketHandler(int, int, int, void *data)
{
    E3IndexPacket *p = (E3IndexPacket *)data;

    if (theMission.m_pNetGame) {
        if (p->index == 4) {
            char text[48];

            switch (p->flag) {
            case 0:
                sprintf(text, "%s", "General Protection Fault");
                break;
            case 1:
                sprintf(text, "%s", "Password Error");
                break;
            case 2:
                sprintf(text, "%s", "Too many spectators");
                break;
            case 3:
                sprintf(text, "%s", "Too many players");
                break;
            case 4:
                sprintf(text, "%s", "Closed clan game");
                break;
            case 5:
                sprintf(text, "5s", "Team assignment error");
                break;
            default:
                sprintf(text, "%s", "Totally unknown error");
                break;
            }
            if (data_0044F3C8)
                data_0044F3C8("MP_EJECT_SPECTATOR", text);
        } else {
            func_002C3330(p->index);
            func_002C34F0(p->flag);
            CZPersonaState::UpdateLocalPersona();
        }
    }
    return 8;
}

/* the E3 index sent: handled here when it is our own (OVL1 0x2BAEA0) */
void NetSendE3Index(int who, unsigned int index, int flag)
{
    E3IndexPacket p;

    p.index = index;
    p.flag = flag;
    if (who == theNetwork.zNetGetMyGameClientIndex())
        E3IndexPacketHandler(0, 0, theNetwork.zNetGetMyGameClientIndex(), &p);
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

/* a chat line as it is sent (SOCOM II's: its size, the channel -- 0 everyone, 1 and 2 the two
 * teams' tests, 3 the lobby's flag -- and the text) */
struct ChatPacket {
    unsigned short size;   // 0x0
    unsigned short channel;   // 0x2
    char text[4];   // 0x4
};

extern "C" {
extern unsigned int data_00440E40;   /* PacketTypeChat */
const char *func_002C3A70(int client);   /* (a client's name, fts_net_persona.cpp's) */
}

/* a ZRDR value as the chat's history reads it (zrdr.h's _zrdr: the type, an array's count, the
 * value; the accessors as zrdr.h's) */
struct ChatRdr {
    unsigned int type : 8;   // 0x0
    unsigned int flags : 8;   // 0x0
    unsigned int length : 16;   // 0x0
    union {
        char *string;
        ChatRdr *array;
    };   // 0x4

    bool IsArray() const { return type == 4; }
    bool IsString() const { return type == 3; }
    unsigned int Count() const { return type == 4 ? length : 0; }
    ChatRdr *Item(unsigned int i) const { return i < Count() ? &array[i] : 0; }
    const char *String() const { return type == 3 ? string : 0; }
};

/* the chat's history: a UI variable holding an array of strings */
class CUIVariable : public ChatRdr {
public:
    void Set(struct _zrdr *value);
};

/* zrdr.h's editor: only what the chat builds its history with (0x4C bytes) */
class CRdrEditor {
public:
    CRdrEditor();
    ~CRdrEditor();
    bool push(const char *tag);
    bool pop();
    bool addstringnode(const char *s);

    unsigned char _0[0x4c];
};

/* a chat line received: dropped when it is for a team we are not on, for the lobby's flag we do
 * not hold, empty, or from an ignored player; else the history's last 20 lines and the new one
 * (its sender's name, Team or All) made the chat variable's new value (SOCOM 1's
 * ChatPacketHandler, its history in a UI variable; OVL1 0x2BA9D0) */
int ChatPacketHandler(int, int, int who, void *data)
{
    ChatPacket *p = (ChatPacket *)data;
    CZNetGame *game = theMission.m_pNetGame;

    if (game) {
        bool show = true;

        if (p->channel == 1 && !func_002C31A0())
            show = false;
        else if (p->channel == 2 && !func_002C3080())
            show = false;
        else if (p->channel == 3 && !func_002C2FA0())
            show = false;
        else if (p->text[0] == 0)
            show = false;
        const char *name = func_002C3A70(who);

        if (name && theLobby.IsIgnored(name))
            show = false;
        if (show) {
            CRdrEditor ed;
            char text[144];

            ed.push(0);
            if (game->m_chat->IsArray()) {
                int n = game->m_chat->Count();

                for (int i = game->m_chat->Count() - 20; i < n; i++) {
                    ChatRdr *line = game->m_chat->Item(i);

                    if (line && line->IsString())
                        ed.addstringnode(line->String());
                }
            }
            if (!name)
                name = "Unknown";
            if (p->channel)
                sprintf(text, "#C@\x80@%s (Team): %s\n", name, p->text);
            else
                sprintf(text, "#C\x80\x80\x80%s (All): %s\n", name, p->text);
            ed.addstringnode(text);
            ed.pop();
            game->m_chat->Set((struct _zrdr *)&ed);
        }
    }
    return p->size;
}

/* a chat line sent, to one client when arg names one, else to all, on the channel the
 * ChatRecipient valve picks, and shown here (SOCOM 1's, the packet SOCOM II's; OVL1 0x2BA870) */
void AuxSendChatMsg(const char *text, void *arg)
{
    CValve *valve = CValve::GetByName("ChatRecipient");
    int size = strlen(text) + 6;
    ChatPacket *p = (ChatPacket *)_malloc(size, 0, 0);

    p->size = size;
    p->channel = 0;
    if (valve && valve->m_value) {
        if (func_002C31A0())
            p->channel = 1;
        else if (func_002C3080())
            p->channel = 2;
        else if (func_002C2FA0())
            p->channel = 3;
    }
    strcpy(p->text, text);
    theNetwork.zNetSendApplicationMessage(64, theNetwork.GetGameConnectionIndex(),
        arg ? *(int *)arg : -1, data_00440E40, size, (unsigned char *)p, false);
    ChatPacketHandler(0, 0, theNetwork.zNetGetMyGameClientIndex(), p);
    _free(p, 0, 0);
}

/* a chat line, censored first (SOCOM 1's; OVL1 0x2BA850) */
void SendChatMsg(const char *text)
{
    theLobby.Censor(text, AuxSendChatMsg, 0);
}

/* a weapon pickup told to everyone: its id, its orientation as three 10-bit angles (the Euler
 * angles' y, x and z at bits 10, 20 and 0), its position and the client (SOCOM 1's
 * SendWeaponPUMessage and WeaponPUPacketHandler, the orientation packed into a word where SOCOM 1
 * sent the quaternion) */
struct WeaponPUPacket {
    int id;   // 0x0
    unsigned int angles;   // 0x4
    PNT3D pos;   // 0x8
    unsigned short who;   // 0x14
};

extern "C" {
extern unsigned int data_00440E48;   /* PacketTypeWeaponPU */
void func_0059EC60(CZSealBody *body, unsigned short who, int id, CPnt3D *pos, CQuat *q);
}

/* an angle in [-pi, pi] as 10 bits (the name ours) */
inline int PackAngle(float a)
{
    float t = 0.15915494f * (3.1415927f + a);

    if (!(t <= 1.0f))
        t = 1.0f;
    else if (t < 0.0f)
        t = 0.0f;
    return (int)(1023.0f * t);
}

/* (OVL1 0x2BA660) */
extern "C" void func_002BA660(unsigned short who, int id, const CPnt3D *pos, const CQuat *q)
{
    CPnt3D euler;
    WeaponPUPacket p;

    p.who = who;
    p.id = id;
    p.pos.x = pos->x;
    p.pos.y = pos->y;
    p.pos.z = pos->z;
    q->ToEuler(&euler);
    int ax = PackAngle(euler.x) << 20;
    int ay = PackAngle(euler.y) << 10;
    int az = PackAngle(euler.z);

    p.angles = ax | ay | az;
    theNetwork.zNetSendApplicationMessage(64, theNetwork.GetGameConnectionIndex(), -1,
        data_00440E48, 24, (unsigned char *)&p, false);
}

/* (OVL1 0x2BA4E0) */
int WeaponPUPacketHandler(int, int, int, void *data)
{
    WeaponPUPacket *p = (WeaponPUPacket *)data;
    CPnt3D pos(p->pos.x, p->pos.y, p->pos.z);
    CQuat q;
    float ry = 0.0061419215f * (float)((p->angles >> 10) & 0x3ff) - 3.1415927f;
    float rx = 0.0061419215f * (float)((p->angles >> 20) & 0x3ff) - 3.1415927f;
    float rz = 0.0061419215f * (float)(p->angles & 0x3ff) - 3.1415927f;

    q.MakeYXZRot(ry, rx, rz);
    CZSealBody *body = CZSealState::GetBodyFromObjectIndex(func_00541CC0(p->id));

    if (body)
        func_0059EC60(body, p->who, p->id, &pos, &q);
    return 24;
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
    unsigned short who;   // 0x0
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

/* a vote on the spectators: the voter's flag set and the votes against counted and shown; the
 * session master, once enough are against, sends the count to everyone and throws the spectators
 * out here too (func_002B9D80, inlined; SOCOM II's, SOCOM 1's VoterPacketHandler's place; OVL1
 * 0x2B9BD0). func_002B9B60 calls it out of line: pinned */
#pragma auto_inline off
extern "C" int func_002B9BD0(int, int, int who, VotePacket *p)
{
    int count;
    CZSealBody *body = CZSealState::GetBodyFromObjectIndex(p->who);

    if (body) {
        func_002C2F50(who, p->vote);
        int votes = func_002C2EF0();
        char text[128];

        if (p->vote)
            sprintf(text, " %s voting against spectators: %d votes against.", body->m_node->m_name,
                    votes);
        else
            sprintf(text, " %s voting for spectators: %d votes against.", body->m_node->m_name,
                    votes);
        func_002B6530(theMission.m_messenger, text, 2, 0, 0, 0.0f);
        if (theNetwork.m_bSessionMaster && func_002C2E30()) {
            count = votes;
            theNetwork.zNetSendApplicationMessage(64, theNetwork.GetGameConnectionIndex(), -1,
                data_00440E90, 4, (unsigned char *)&count, true);
            func_002B9D80(0, 0, 0, &count);
        }
    }
    return 4;
}
#pragma auto_inline reset

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

extern "C" {
extern ClientConnectArgs data_00440F20;   /* (the arguments handed to the connect callback) */
extern void *data_0045B718;   /* (zNetVoice.cpp's object) */
void func_00310080(void *voice, int ms);
void func_0030FD90(void *voice, int arg);
}

/* the game server joined, once: the game's and the persona's objects forgotten, every client's
 * word cleared, the DME joined; the session master hands each client already waiting to the
 * connect callback; the master's index kept and the voice set up (SOCOM 1's DMEJoinGameServer,
 * with the join SOCOM 1's zRemoteDelayedAction made; OVL1 0x2B83E0) */
bool DMEJoinGameServer()
{
    if (theNetwork.m_bJoined)
        return true;
    theMission.m_pNetGame->m_game_object_index = -1;
    theMission.m_pNetGame->m_persona_object_index = -1;
    theMission.m_pNetGame->m_persona_created = 0;
    for (int i = 0; i < 24; i++)
        data_00440EC0[i] = 0;
    if (theNetwork.zNetJoin(zRemoteClientJoinsCallback, zRemoteClientLeavesCallback))
        return false;
    if (theNetwork.zNetGetGameSessionMasterClientIndex() == theNetwork.zNetGetMyGameClientIndex()) {
        theNetwork.m_bSessionMaster = 1;
        for (int i = 0; i < 24; i++) {
            if (data_00440EC0[i]) {
                data_00440F20.m_client = i;
                data_00440F20.m_10 = data_00440EC0[i];
                func_002BC620(&data_00440F20);
                data_00440EC0[i] = 0;
            }
        }
    } else
        theNetwork.m_bSessionMaster = 0;
    theNetwork.m_master_index = theNetwork.zNetGetGameSessionMasterClientIndex();
    func_00310080(data_0045B718, 10000);
    func_0030FD90(data_0045B718, 11);
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
