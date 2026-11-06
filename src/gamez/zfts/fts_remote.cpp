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

#include <list>

#include "../zmath/zmath.h"

struct ClientConnectArgs;

#include "../znet/znet.h"


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

/* a SEAL's command (zseal.h's FT_COMMAND: a byte under -enum min) */
enum FT_COMMAND {
    CMD_UNKNOWN = 0,
    CMD_ACTION = 1,
    CMD_LAST = 255
};

class CQuat;
class CZSealBody {
public:
    /* zentity.h's virtual functions up to the matrix teleport (+0x40): ftsNetworkPlayerInit's */
    virtual ~CZSealBody();                                      /* +0x08 */
    virtual const CMatrix *GetPosW() const;
    virtual void UpdatePosW();
    virtual void PreTick(float dT);
    virtual void Tick(float dT);
    virtual void PostPreTick(float dT);                         /* +0x1C */
    virtual void PrePostTick(float dT);
    virtual void PostTick(float dT);
    virtual void ComputeNextPosition(float dT);
    virtual void UpdateNextPosition();                          /* +0x2C */
    virtual CPnt3D *GetTargetPosW(CPnt3D *out);
    virtual CPnt3D *GetHeadPosW(CPnt3D *out);
    virtual CPnt3D *GetNeckPosW(CPnt3D *out);
    virtual void TeleportTo(const CPnt3D *pos, const CQuat *quat);   /* +0x3C */
    virtual void TeleportTo(const CMatrix *m);                  /* +0x40 */

    int m_4;   // 0x4: the body's network object (SOCOM 1's m_NetObjectIndex)
    unsigned char _8[0x1c - 0x8];
    CPnt3D m_posW;   // 0x1c: where it stands (zentity.h's)
    CNodeName *m_node;   // 0x28 (CEntity's)
    unsigned char _2c[0xc0 - 0x2c];
    CEntityCtrl *m_ctrl;   // 0xc0
    unsigned char _c4[0xc8 - 0xc4];
    unsigned int m_team;   // 0xc8: the team's bits (zentity.h's)
    unsigned char _cc[0x220 - 0xcc];
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

    /* a command given to another SEAL (OVL2's, SOCOM 1's at the same call) */
    void IssueCommand(CZSealBody *who, FT_COMMAND cmd, void *data, bool b);   /* OVL2 0x590AC0 */
    /* whether another is on no team of this one's (zentity.h's CEntity::IsEnemy, its operands turned
     * round: the other's team bits read first) */
    bool IsEnemy(const CZSealBody *e) const { return !(e->m_team & m_team); }
    void RemoteWeaponDamageNonVictim(DAMAGE_LOCATION where, CZSealBody *victim, CZSealBody *shooter,
                                     unsigned char c, bool flag);
    void RemoteWeaponDamageVictim(DAMAGE_LOCATION where, float damage, float f, CZSealBody *victim,
                                  CZSealBody *shooter, unsigned char c, bool flag);
    /* a console's request for a pickup answered (OVL2 0x59E830; SOCOM 1's, the id and the asker
     * ints in SOCOM II) */
    void CanPickupNetItem(unsigned short count, int id, unsigned short *items, int client);
    /* a pickup another client made, made here (OVL2 0x59EC60, 0x59E9E0: zseal.h's) */
    void CreateNetWeaponPickup(unsigned short item, int netid, CPnt3D *pos, CQuat *rot);
    void CreateNetAmmoPickup(unsigned short count, unsigned short *items, unsigned short *rounds, int id,
                             CPnt3D *pos);
    /* a remote SEAL's AI taken over by the new session master (OVL2 0x56F210, SOCOM 1's) */
    static void AcquireRemoteAi(CZSealBody *body);
};

class CZVehicle {
public:
    unsigned char _0[4];
    int m_4;   // 0x4: its network object
};

/* the world's entities (zentity.h's CEntity: only the list and the kind -- 2 a SEAL, 3 a turret) */
class CEntity;

class CEntityList : public std::list<CEntity *> {
};

class CEntity {
public:
    static const CEntityList &GetEntityList() { return m_list; }

    static CEntityList m_list;

    unsigned char _0[0x10];
    unsigned char m_type;   // 0x10
};

/* a turret (veh_turret.cpp's CTurret: only its network object here) */
class CTurret : public CZVehicle {
};

/* the turrets' network objects (fts_net_platform.cpp's) */
class CZPlatformState {
public:
    static void zNetRegisterPlatform();   /* 0x2C8A00 */
    static int CreateLocalPlatform(CTurret *turret);
};

class CUIVariable;

/* the online game (SOCOM 1's CZNetGame: only what this unit touches, at SOCOM II's offsets) */
class CZNetGame {
public:
    enum MP_MAJOR_GAME_STATE {
        MP_MAJOR_0 = 0,
        MP_MAJOR_1 = 1,
        MP_MAJOR_GHOST = 14
    };

    unsigned char _0[0x28];
    CValve *m_28;   // 0x28: mp_join_game (fts_mission.cpp's; SOCOM 1's m_pJoinGameValve at 0x24)
    unsigned char _2c[0x44 - 0x2c];
    CValve *m_pPlayerJoinCountValve;   // 0x44 (SOCOM 1's at 0x40)
    unsigned char _48[0x6c - 0x48];
    CValve *m_6c;   // 0x6c: set from each action packet (SOCOM II's)
    unsigned char _70[0x84 - 0x70];
    /* the teams' lists' UI variables, BSEALLISTVAR, BTERRLISTVAR and BSPECLISTVAR (fts_mission.cpp's
     * m_84[5]; SOCOM 1's m_uiv_*List at 0x50) */
    CUIVariable *m_84;   // 0x84
    CUIVariable *m_88;   // 0x88
    CUIVariable *m_8c;   // 0x8c
    unsigned char _90[0x94 - 0x90];
    CUIVariable *m_chat;   // 0x94: the chat's history (SOCOM II's)
    unsigned char _98[0xac - 0x98];
    int m_persona_object_index;   // 0xac (SOCOM 1's at 0x70)
    int m_game_object_index;   // 0xb0 (SOCOM 1's at 0x74)
    unsigned char _b4[0xcc - 0xb4];
    bool m_late_joiner;   // 0xcc (SOCOM 1's at 0x94)
    unsigned char _cd[0xcf - 0xcd];
    unsigned char m_persona_created;   // 0xcf (SOCOM 1's at 0x97)
    unsigned char _d0;
    unsigned char m_d1;   // 0xd1 (SOCOM 1's m_ghost_ok at 0x99)
    unsigned char m_d2;   // 0xd2: the player's SEAL made anew at its start (SOCOM II's; the name ours)
    unsigned char _d3[0xd9 - 0xd3];
    unsigned char m_d9;   // 0xd9: SOCOM II's (a client collision reported)
    unsigned char m_da;   // 0xda: set while held actions are handled at once (SOCOM II's)
    unsigned char _db[0xdc - 0xdb];
    unsigned char m_respawn;   // 0xdc: respawning on (fts_mission.cpp's)
    unsigned char _dd[0xde - 0xdd];
    unsigned char m_de;   // 0xde: no hits sent while set (SOCOM II's)
    unsigned char _df[0x104 - 0xdf];
    CPnt3D m_104;   // 0x104: where the bomb is (SOCOM 1's at 0xcc; the name ours)
    unsigned char m_110;   // 0x110: a bomb is down there (SOCOM 1's at 0xd8; the name ours)
    char m_111;   // 0x111 (SOCOM 1's at 0xd9: m_game_type)
    unsigned char m_112;   // 0x112 (SOCOM 1's m_net_start_flag at 0xda)
    unsigned char m_mp_major_game_state;   // 0x113 (SOCOM 1's at 0xdb)
    unsigned char m_my_major_game_state;   // 0x114 (SOCOM 1's at 0xdc)

    bool LateJoiner(bool late);
    MP_MAJOR_GAME_STATE MPMajorGameState(MP_MAJOR_GAME_STATE state);   /* 0x2A7490 */
    MP_MAJOR_GAME_STATE MyMajorGameState(MP_MAJOR_GAME_STATE state);   /* 0x2A7420 */
};

class CMission {
public:
    void RefreshTeamValves();   /* 0x2B23A0 */

    unsigned char _0[0x1ac];
    CValve *m_valve_abort;   // 0x1ac: the "mission_abort" valve (fts_mission.cpp's): at 1 or 2 no hits are taken
    unsigned char _1b0[0x1c0 - 0x1b0];
    unsigned char m_messenger[0x1808 - 0x1c0];   // 0x1c0: the mission's message queue (SOCOM 1's at 0x100)
    CZNetGame *m_pNetGame;   // 0x1808 (SOCOM 1's at 0x584)
};

extern CMission theMission;

class CZSealState {
public:
    static void zNetRegisterSeal();   /* 0x2BF2C0 */
    static CZSealBody *GetBodyFromObjectIndex(int index);
    static int CreateLocalSeal(CZSealBody *body, bool remote);
};

class CZGameState {
public:
    static void zNetRegisterGame();   /* 0x2C1120 */
    static void UpdateLocalGame(CZNetGame *game);
    static int CreateLocalGame(CZNetGame *game);
};

class CZPersonaState {
public:
    static void zNetRegisterPersona();
    static void ClearPersonaList();
    static void UpdateLocalPersona();
    static int CreateLocalPersona();
};

/* the online lobby (only its Medius connection) */
class CZOnlineLobby {
public:
    int MediusConnectionIndex();
    void ForceStatusReport();   /* 0x2F8350 */
    void Censor(const char *text, void (*send)(const char *, void *), void *arg);
    bool IsIgnored(const char *name);
    void SyncGameClock(int t);   /* 0x2DF560 */
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

/* the anims (zanim.h's CZAnimMain: only the network send's member) */
class CZAnimMain {
public:
    unsigned char _0[0x4c];
    /* the anims' network send (0x414BFC: CZAnim::Activate calls through it; the engine core's round 20) */
    void (*m_network_activation_callback)(void *data, int size);   // 0x4c
};

extern CZAnimMain ZAnim;

const char *GetDatabase();
void SetDatabase(const char *name);   /* 0x27F810 */

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
    static void zNetRegisterBomb();
    static void HandleBombStateRequest(int who, unsigned char state, CPnt3D *pos);
    static int CreateLocalBomb();   /* 0x2C2260 */
    static void CreateBombHere(CPnt3D *pos);   /* 0x2C1BF0 */
};


/* the pickups' network side (OVL2's CPickup: only its statics here) and a remote message played
 * (OVL2's) */
class CPickup {
public:
    static int GetEntityIDFromNetID(unsigned int id);   /* OVL2 0x541CC0 */
    static void NetTick(const CPnt3D *pos, float dT);   /* OVL2 0x5423E0 */
};

class CEntity;
void PlayRemoteMsg(CEntity *e, int msg, int arg);   /* OVL2 0x5E7F20 */

/* unnamed yet (SOCOM 1's names in the comments) */
extern "C" {
extern unsigned char data_004412D8;
extern CZOnlineLobby theLobby;

void NetFreeObject(int index);
void func_002BD640(CZSealBody *body, int arg);   /* CZSealState::UpdateLocalSeal */
int func_002C3B10(int arg);
void func_002F9690(void *lobby, int arg, int mode);
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
/* what the DME's connect and disconnect callbacks are handed (SOCOM 1's callbacks took the
 * connection, the client and its name as arguments; SOCOM II's read the first two through a
 * pointer) */
struct ClientConnectArgs {
    int m_index;   // 0x0: the connection
    int m_client;   // 0x4
    int _8[2];
    int m_10;   // 0x10: the client's word (DMEJoinGameServer hands each waiting one on)
};

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
 * zero (SOCOM II's; only the two position schedules below take its address. OVL1 0x2BD270) */
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

/* how a schedule decides a field has changed enough to send (SOCOM 1's NetThresholdMethod: its
 * position schedules 2 with a threshold, its flags 1 with none; the names ours) -- znet.h's, which
 * this unit cannot include while its own CZNetwork carries the members znet.h's lacks
 * (GetGameConnectionIndex, zNetGetGameSessionMasterClientIndex, zNetGetObject, the fix's byte) */

/* a field's broadcast schedule (SOCOM 1's CZBroadcastSchedule, 16 bytes here: SOCOM 1's double
 * threshold is a float, or a compare function in its place, and SOCOM II adds a byte) -- znet.h's
 * declaration: both constructors are zNetDme.cpp's (OVL1 0x30E3F0, 0x30E3D0: the online lobby's
 * round 15) */

/* the schedules, online and on the LAN (SOCOM 1's six were CZNetObject's statics: position,
 * rotation, flags, flash, grenade position and velocity -- SOCOM II's eighteen are file statics
 * picked by the getters below; the names ours). Built in this order by the unit's initialiser
 * __sinit_fts_remote.cpp 0x400400 (0x440CE0-0x440DF0) */
static CZBroadcastSchedule sched_pos(100, NET_THRESHOLD_DELTA, 2.0f, 0);
static CZBroadcastSchedule sched_pos_cmp(100, NET_THRESHOLD_DELTA, func_002BD270, 0);
static CZBroadcastSchedule sched_rot(50, NET_THRESHOLD_DELTA, 0.05f, 0);
static CZBroadcastSchedule sched_flags(100, NET_THRESHOLD_ANY, 0.0f, 0);
static CZBroadcastSchedule sched_flash(0, NET_THRESHOLD_ANY, 0.0f, 0);
static CZBroadcastSchedule sched_grenade_pos(50, NET_THRESHOLD_DELTA, 5.0f, 0);
static CZBroadcastSchedule sched_grenade_vel(50, NET_THRESHOLD_DELTA, 0.1f, 0);
static CZBroadcastSchedule sched_slow(2000, NET_THRESHOLD_ANY, 0.0f, 0);
static CZBroadcastSchedule sched_lan_pos(200, NET_THRESHOLD_DELTA, 2.0f, 0);
static CZBroadcastSchedule sched_lan_pos_cmp(200, NET_THRESHOLD_DELTA, func_002BD270, 0);
static CZBroadcastSchedule sched_lan_rot(100, NET_THRESHOLD_DELTA, 0.05f, 0);
static CZBroadcastSchedule sched_lan_flags(200, NET_THRESHOLD_ANY, 0.0f, 0);
static CZBroadcastSchedule sched_lan_flags64(200, NET_THRESHOLD_ANY, 0.0f, 64);
static CZBroadcastSchedule sched_lan_flash(50, NET_THRESHOLD_ANY, 0.0f, 0);
static CZBroadcastSchedule sched_lan_flash80(50, NET_THRESHOLD_ANY, 0.0f, 80);
static CZBroadcastSchedule sched_lan_grenade_pos(100, NET_THRESHOLD_DELTA, 5.0f, 0);
static CZBroadcastSchedule sched_lan_grenade_vel(100, NET_THRESHOLD_DELTA, 0.1f, 0);
static CZBroadcastSchedule sched_lan_slow(2000, NET_THRESHOLD_ANY, 0.0f, 0);

/* the LAN lobby (zNetLANLobby.cpp's): only whether it is up */
class CZLANLobby {
public:
    bool IsActive();   /* 0x3045B0 */
};
extern CZLANLobby data_0044FE10;   /* the LAN lobby */

/* each field's schedule, the LAN's while the LAN lobby is up (SOCOM II's; fts_net_seal.cpp and
 * fts_net_grenade.cpp register their fields with them -- OVL1 0x2BD020-0x2BD21B, unnamed) */
extern "C" CZBroadcastSchedule *func_002BD1E0()
{
    if (data_0044FE10.IsActive())
        return &sched_lan_pos_cmp;
    return &sched_pos_cmp;
}

extern "C" CZBroadcastSchedule *func_002BD1A0()
{
    if (data_0044FE10.IsActive())
        return &sched_lan_rot;
    return &sched_rot;
}

extern "C" CZBroadcastSchedule *func_002BD160()
{
    if (data_0044FE10.IsActive())
        return &sched_lan_flags;
    return &sched_flags;
}

extern "C" CZBroadcastSchedule *func_002BD120()
{
    if (data_0044FE10.IsActive())
        return &sched_lan_flags64;
    return &sched_flags;
}

extern "C" CZBroadcastSchedule *func_002BD0E0()
{
    if (data_0044FE10.IsActive())
        return &sched_lan_flash;
    return &sched_flash;
}

extern "C" CZBroadcastSchedule *func_002BD0A0()
{
    if (data_0044FE10.IsActive())
        return &sched_lan_flash80;
    return &sched_flash;
}

extern "C" CZBroadcastSchedule *func_002BD060()
{
    if (data_0044FE10.IsActive())
        return &sched_lan_grenade_pos;
    return &sched_grenade_pos;
}

extern "C" CZBroadcastSchedule *func_002BD020()
{
    if (data_0044FE10.IsActive())
        return &sched_lan_grenade_vel;
    return &sched_grenade_vel;
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
        theNetwork.AddToFixList((CZNetwork::TypeRTimeFixStruct *)&m);
    }
}

/* a squad (zseal.h's CSealUnit: its first word, then its table -- IssueCommand first; the squads'
 * list, SOCOM 1's m_list, and the number SOCOM 1's DWARF calls m_unit_num, at 0x38 there) */
enum UNIT_MESSAGE { UNIT_MESSAGE_0 = 0, UNIT_MESSAGE_DEATH = 1, UNIT_MESSAGE_2 = 2, UNIT_MESSAGE_LAST = 255 };

class CSealUnit {
public:
    static CSealUnit *GetUnitByTeam(unsigned int team);   /* OVL2 0x5EF2F0 */
    void Notify(CZSealBody *seal, UNIT_MESSAGE msg, void *data);   /* OVL2 0x5ED7B0 */

    unsigned int m_0;   // 0x0
    virtual void IssueCommand(CZSealBody *seal, FT_COMMAND cmd, void *data, bool b);
    unsigned char _8[0x58 - 0x8];
    unsigned int m_unit_num;   // 0x58 (SOCOM 1's at 0x38)

    static std::list<CSealUnit *> m_list;   /* 0x44D5B0 */
};

void ftsNetworkGameInit();

/* the entity as a SEAL, if it is one, into a variable of the caller's (AsSeal through a reference
 * out-parameter, ours: zDoClientLeavesCallback's SEAL is a named local, uninitialised at the top
 * and numbered there -- below the client index and the net game, the target's s2 against the game's
 * s1; AsSeal's result returned by value merges into the inline's temporary, numbered after the
 * caller's locals, and the two trade s1 and s2: the states' round 39, read off igreplay --want) */
inline void GetSeal(CEntity *e, CZSealBody *&s)
{
    s = e && e->m_type == 2 ? (CZSealBody *)e : 0;
}

/* whether a SEAL is a remote one (its controller asked; ours, IsPlayerSeal's twin: the result
 * narrowed in each arm) */
inline bool IsRemoteSeal(CZSealBody *seal)
{
    return seal->m_ctrl ? seal->m_ctrl->IsRemote() : false;
}

extern "C" {
bool func_002A72A0(CZNetGame *game);   /* (fts_mission.cpp's: a game type 1 or 2 -- a bomb game) */
extern ClientConnectArgs data_00440F20;   /* (the arguments handed to the connect callback) */
/* the radio (zNetVoice.cpp's CZNetVoice: only its messages' registration, 0x310210 -- the online
 * lane's round-25 name) */
struct CZNetVoice {
    void zNetRegisterVoice();
};

extern CZNetVoice *data_0045B718;   /* (zNetVoice.cpp's object) */
void func_0030E7F0(void *voice);   /* (SOCOM 1's CZNetVoice::IAmNewSessionMaster) */
}

/* a client gone, put off to here (SOCOM 1's; OVL1 0x2BC080): with a client index of our own, if
 * the session master left and we are the new one, we take over -- the game's and the persona's
 * objects made (ftsNetworkGameInit, inlined), the bomb's in a bomb game, every remote SEAL that
 * was the master's AI taken up, every squad but number 17 told, the lobby's and the voice's
 * reports, the team valves (game states 2-5), the persona sent and each client waiting handed to
 * the connect callback; then a bomb left down made here by the master, and the master's index
 * kept. Byte-exact (round 39): the squads' walk takes its end into a local first (in a saved
 * register, as the target; `!= m_list.end()` each pass rebuilds the address, 8 bytes longer), the
 * entities' walk is round 31's (`!= GetEntityList().end()`), and the SEAL is GetSeal's */
void zDoClientLeavesCallback(int client)
{
    CZSealBody *seal;

    theNetwork.zNetGetGameSessionMasterClientIndex();
    int me = theNetwork.zNetGetMyGameClientIndex();
    CZNetGame *game = theMission.m_pNetGame;

    if (me == 256)
        return;
    if (!theNetwork.m_bSessionMaster && me == theNetwork.zNetGetGameSessionMasterClientIndex()) {
        theNetwork.m_bSessionMaster = 1;
        ftsNetworkGameInit();
        if (func_002A72A0(game))
            CZBombState::CreateLocalBomb();
        CEntityList::const_iterator i;

        for (i = CEntity::GetEntityList().begin(); i != CEntity::GetEntityList().end(); ++i) {
            GetSeal(*i, seal);
            if (seal && IsRemoteSeal(seal) && (seal->m_team & 0x20000))
                CZSealBody::AcquireRemoteAi(seal);
        }
        std::list<CSealUnit *>::iterator u, end = CSealUnit::m_list.end();

        for (u = CSealUnit::m_list.begin(); u != end; ++u) {
            if ((*u)->m_unit_num != 17)
                (*u)->Notify(0, UNIT_MESSAGE_2, 0);
        }
        theLobby.ForceStatusReport();
        func_0030E7F0(data_0045B718);
        if (game->m_mp_major_game_state >= 2 && game->m_mp_major_game_state < 6)
            theMission.RefreshTeamValves();
        CZPersonaState::UpdateLocalPersona();
        for (int c = 0; c < 24; c++) {
            if (data_00440EC0[c]) {
                data_00440F20.m_client = c;
                data_00440F20.m_10 = data_00440EC0[c];
                func_002BC620(&data_00440F20);
                data_00440EC0[c] = 0;
            }
        }
    }
    if (theNetwork.m_bSessionMaster && func_002A72A0(game) && game->m_110)
        CZBombState::CreateBombHere(&game->m_104);
    game->m_110 = 0;
    if (theNetwork.m_sessionmaster_index != theNetwork.zNetGetGameSessionMasterClientIndex())
        theNetwork.m_sessionmaster_index = theNetwork.zNetGetGameSessionMasterClientIndex();
}

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



/* a hit: in a game under way, unless the mission is aborting (its abort valve at 1 or 2), the victim takes it -- its own client's
 * damage for a local victim, the remote one's for a remote victim (SOCOM 1's; OVL1 0x2BBBD0).
 * OPEN, 2 instructions (round 38, from 6: see the end). The mode test matches with the mode passed to the inline as
 * an `int` (round 13: the parameter takes the loaded halfword, which frees v0 for the constant 1 and
 * a0 for the bool; read inside the inline -- straight, through a mode local, as a member of the
 * info, or as an unsigned short parameter -- they land in a0/a1, and round 12's shapes, a switch,
 * early returns, bool locals and `||`, all miss), and the victim's damage is SOCOM 1's member
 * RemoteWeaponDamageVictim (0x5A1B80, named by the SEAL lane: `this` loaded last, the call's moves
 * the target's). Left: the bitfield byte's v0/v1 trade places with the 0.05 constant's (the target
 * loads the constant ahead of the byte and extracts the flag ahead of the part; tried the four
 * locals in six orders, the product cast, swapped, split and through a constant local, the
 * bitfields as unsigned int, bool and signed char, the flag `!= 0` and as a ternary, the part
 * through an int or unsigned char local -- none moves them). Round 36, read off MW's colouring
 * (igdump): the byte is numbered after the constant's lui/ori temporaries because MW generates
 * f's product (the constant last) before the part's load, and the higher number is coloured first
 * (v0). The constant generated after the byte -- `float f = p->m_6;` ahead of the part and flag,
 * `f = 0.05f * f;` after them -- gives exactly the target's GPRs (the constant v0, the byte v1, the
 * flag extracted ahead of the part) but puts the conversion in f's own register ($f20, the product
 * `mul.s $f20,$f20,$f0`) where the target converts into a temporary ($f1) and multiplies constant
 * first; a separate conversion local is forwarded to its single use (56 off), all 24 orders of the
 * four locals as they stand 6-66 off.
 * Round 37 (igdump): the constant's lui/ori temporaries are v66/v67 and the byte v68, coloured
 * highest first -- the byte must be generated before the constant, and MW generates the product's
 * constant after its converted operand, so no single-expression product allows it. Tried: the scale
 * as an inline (by float, unsigned int or unsigned char, through a constant parameter, a named
 * result -- all 6), `v / 20.0f` (33), part and flag in a named copy of a nested bitfield struct
 * (0x1D4, through memory) or of the packet (0x1EC); part and flag defined before the floats move the
 * byte's load and their live ranges, renumbering the saved registers (55-66).
 * Round 38: 6 -> 2. The conversion a local of its own (`s`) with the flag a TERNARY between it and
 * the product: MW's expression propagation (the IR optimiser's, mweppc IroPropagate.c) forwards a
 * single-use local's expression to its use only inside one flow-graph node, and a `?:` (as `&&`/`||`)
 * is linearised into branches -- so `s` stays at its definition ($f1, a temporary), the byte is
 * generated before the constant (v1 against v0) and the flag's select folds back to the target's
 * sltu (SOCOM 1's flag is a select too: `li 1; movz`). Without the ternary `s` is forwarded into the
 * product (56 off); a dead second use, a const or register local, a reference, `(void)s` or an
 * inline's out-parameter forward all the same. Left: the target issues the constant's lui ahead of
 * the byte's lbu, this the lbu first (same registers, same block: a scheduler tie broken by the
 * generation order, which the colouring fixes as byte-first) -- every order of the four statements
 * with the ternary, ten spellings of the flag and five of the product keep it */
/* whether the mission's abort valve's value takes no hits (the name ours: the handler's test
 * materialised) */
inline bool NoHitMode(int mode)
{
    bool b = true;

    if (mode != 1 && mode != 2)
        b = false;
    return b;
}

int HitPacketHandler(int, int, int, void *data)
{
    if (!NoHitMode(theMission.m_valve_abort->m_value)) {
        CZNetGame *game = theMission.m_pNetGame;

        if (game && game->m_mp_major_game_state == 3) {
            HitPacket *p = (HitPacket *)data;
            CZSealBody *victim = CZSealState::GetBodyFromObjectIndex(p->victim);
            CZSealBody *shooter = CZSealState::GetBodyFromObjectIndex(p->shooter);
            float damage = p->damage;
            float s = p->m_6;
            DAMAGE_LOCATION where = (DAMAGE_LOCATION)p->part;
            bool flag = p->flag ? true : false;
            float f = 0.05f * s;

            if (victim && shooter) {
                if (!(victim->m_ctrl ? victim->m_ctrl->IsRemote() : false))
                    victim->RemoteWeaponDamageVictim(where, damage, f, victim, shooter, p->c, flag);
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
            b->IssueCommand(a, (FT_COMMAND)p->arg, 0, false);
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
                PlayRemoteMsg((CEntity *)body, p->msg, 1);
            break;
        case R_SEND_TO_2:
            if (func_002C3080())
                PlayRemoteMsg((CEntity *)body, p->msg, 1);
            break;
        case R_SEND_TO_ALL:
            PlayRemoteMsg((CEntity *)body, p->msg, 1);
            break;
        default: {
            CZSealBody *player = ftsGetPlayer();

            if (p->to == player->m_4)
                PlayRemoteMsg((CEntity *)body, p->msg, 1);
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
    bool addintnode(int i);
    bool addrdrnode(struct _zrdr *rdr);
    bool addPNT3D(const char *tag, const PNT3D &p);
    bool addSTRING(const char *tag, const char *s);

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
    CZSealBody *body = CZSealState::GetBodyFromObjectIndex(CPickup::GetEntityIDFromNetID(p->id));

    if (body)
        body->CreateNetWeaponPickup(p->who, p->id, &pos, &q);
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
    unsigned short m_4[8];   // 0x4
    unsigned short m_14[8];   // 0x14
    float x, y, z;   // 0x24
    unsigned char who;   // 0x30
};

int AmmoPUPacketHandler(int, int, int, void *data)
{
    AmmoPUPacket *p = (AmmoPUPacket *)data;
    CPnt3D pos(p->x, p->y, p->z);
    CZSealBody *body = CZSealState::GetBodyFromObjectIndex(CPickup::GetEntityIDFromNetID(p->id));

    if (body)
        body->CreateNetAmmoPickup(p->who, p->m_4, p->m_14, p->id, &pos);
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
    CZSealBody *body = CZSealState::GetBodyFromObjectIndex((unsigned short)CPickup::GetEntityIDFromNetID(p->m_0));

    if (body)
        body->CanPickupNetItem(p->m_16, p->m_0, (unsigned short *)p->m_4, p->m_14);
    return 24;
}

/* the pickup's acknowledgement (OVL1 0x2BA1F0): SOCOM II's packet is 8 bytes, the id first; the
 * first parameter an int (its caller 0x59E830 zero-extends the count with andi 0xffff before both
 * calls, as MW passes an unsigned short to an int parameter -- the SEALs' round 27) */
union AckUnion {
    unsigned char b;
};

struct PickupAckPacket {
    int id;   // 0x0
    unsigned short obj;   // 0x4
    unsigned char who;   // 0x6
    AckUnion ack;   // 0x7
};

void SendPickupAckMessage(int who, int id, int obj, AckUnion ack)
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
    unsigned char game_type;   // 0xf (the handler loads it lbu)
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

/* the UI's variables (zui.h's CUIVarManager: only its lookup) */
class CUIVarManager {
public:
    CUIVariable *Find(const char *name);   /* 0x385900 */
};
extern CUIVarManager theUIVarManager;

extern "C" {
unsigned int func_002C32E0();   /* (fts_net_persona.cpp's: the terrorists' team mask) */
unsigned int func_002C32C0();   /* (the SEALs' team mask) */
}

/* a game signal from the session master (SOCOM 1's GameSignalPacketHandler, grown; OVL1 0x2B9150):
 * a refusal (b 4) told the lobby with its reason and taken as signal 0; then, with a game, signal 13
 * forces a new database (the late joiner takes it and has the network's fixes 4 and 5 queued, a
 * joiner not yet in the game marks its start flag), 14 sets the ghost flag, 2 and up join: the
 * clock synchronised, a late joiner, every client's word cleared, the game type and database
 * taken, both major states 1, and the team's list (b: 1, 2 or 3) given our name with our clan's
 * tag in its colour; 0 and 1 only take the database and the team; and every one sets the join
 * valve to 10. Byte-exact (the game states' round 47): the team an `int` local (an `unsigned char`
 * one is a promoted temporary numbered after every named local: s1 where the target has s4), the
 * join test `p->a > 1` (the target's fused `slti at`; `>= 2` materialises into v0), the colour three
 * stores (an initialiser block-copies it from .rodata), and an `int` counter -- an `unsigned int` one
 * matching Item()'s parameter is strength-reduced to an offset stepped by 8 */
extern "C" int func_002B9150(int, int, int, void *data)
{
    GameSignalPacket *p = (GameSignalPacket *)data;
    int team = p->b;
    CZNetGame *game = theMission.m_pNetGame;

    if (team == 4) {
        char text[48];

        switch (p->c) {
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
        default:
            sprintf(text, "%s", "Totally unknown error");
            break;
        }
        if (data_0044F3C8)
            data_0044F3C8("MP_EJECT_SPECTATOR", text);
        p->a = 0;
    }
    if (game) {
        if (p->a == 13) {
            if (game->m_late_joiner) {
                if (game->m_my_major_game_state == 1) {
                    game->m_111 = p->game_type;
                    SetDatabase(p->database);
                    NetMessage m = {0};

                    m.kind = 4;
                    theNetwork.AddToFixList((CZNetwork::TypeRTimeFixStruct *)&m);
                    m.kind = 5;
                    theNetwork.AddToFixList((CZNetwork::TypeRTimeFixStruct *)&m);
                } else {
                    game->m_112 = 3;
                }
                if (data_0044F3C8)
                    data_0044F3C8("ON_FORCE_NEW_DB", 0);
            }
        } else if (p->a == 14) {
            game->m_d1 = 1;
        } else if (p->a > 1) {
            theLobby.SyncGameClock(p->arg);
            game->LateJoiner(true);
            if (game->m_d9 && data_0044F3C8)
                data_0044F3C8("MP_MEDIUS_ERROR", "Client Collision Error");
            for (int i = 0; i < 24; i++)
                data_00440EC0[i] = 0;
            game->m_111 = p->game_type;
            SetDatabase(p->database);
            game->MPMajorGameState(CZNetGame::MP_MAJOR_1);
            game->MyMajorGameState(CZNetGame::MP_MAJOR_1);
            CUIVariable *name = theUIVarManager.Find("PLAYERNAMEVAR");
            CUIVariable *clan = theUIVarManager.Find("Clan_Description");
            CUIVariable *list = 0;
            PNT3D color;
            color.x = 128.0f;
            color.y = 64.0f;
            color.z = 32.0f;
            const char *player = name && name->IsString() ? name->String() : "Player";
            const char *desc = clan && clan->IsString() ? clan->String() : 0;
            char tag[32];

            if (desc && *desc)
                sprintf(tag, "[%s]", desc);
            else
                sprintf(tag, "");
            func_002C0040(p->flag != 0);
            if (team == 1) {
                func_002C3330(func_002C32E0());
                list = game->m_84;
            } else if (team == 2) {
                func_002C3330(func_002C32C0());
                list = game->m_88;
            } else if (team == 3) {
                func_002C3330(0x10000);
                list = game->m_8c;
            }
            if (list) {
                CRdrEditor ed;

                ed.push(0);
                if (list->IsArray())
                    for (int i = 0; i < list->Count(); i++)
                        ed.addrdrnode((struct _zrdr *)list->Item(i));
                ed.push(0);
                ed.addstringnode(player);
                ed.addintnode(0);
                ed.addintnode(0);
                ed.push(0);
                ed.addSTRING("VALUE", tag);
                ed.addPNT3D("COLOR", color);
                ed.pop();
                ed.pop();
                ed.pop();
                list->Set((struct _zrdr *)&ed);
            }
            func_002C34F0(p->c);
        } else {
            game->LateJoiner(false);
            SetDatabase(p->database);
            func_002C0040(p->flag != 0);
            if (team == 1)
                func_002C3330(func_002C32E0());
            else if (team == 2)
                func_002C3330(func_002C32C0());
            else if (team == 3)
                func_002C3330(0x10000);
            func_002C34F0(p->c);
        }
        theMission.m_pNetGame->m_28->SetValue(10);
    }
    return 20;
}

/* ---- the network messages' descriptions (SOCOM II's; the structs' names ours, each laid out by its
 * fields' descriptions: offsets, sizes and counts) ---- */

/* "Hit" (OVL1 0x2BBBD0's handler) */
struct NetMsgHit {
    unsigned short m_0;   // 0x0
    unsigned short m_2;   // 0x2
    unsigned char m_4;   // 0x4
    unsigned char m_5;   // 0x5
    unsigned char m_6;   // 0x6
    unsigned char m_7;   // 0x7
};

/* "Anim" (OVL1 0x2BB7F0's handler) */
struct NetMsgAnim {
    int m_0;   // 0x0
    unsigned short m_4;   // 0x4
    unsigned short m_6;   // 0x6
};

/* "Push Blend" (OVL1 0x2BBA80's handler) */
struct NetMsgPushBlend {
    unsigned short m_0;   // 0x0
    unsigned short m_2;   // 0x2
    unsigned short m_4;   // 0x4
    unsigned char m_6;   // 0x6
    unsigned char m_7;   // 0x7
};

/* "Action Anim" (OVL1 0x2BB310's handler) */
struct NetMsgActionAnim {
    unsigned short m_0;   // 0x0
    unsigned short m_2;   // 0x2
    unsigned short m_4;   // 0x4
    unsigned char _6[2];
    unsigned short m_8;   // 0x8
};

/* "Action Points" (OVL1 0x2BB770's handler) */
struct NetMsgActionPoints {
    unsigned short m_0;   // 0x0
    unsigned short m_2;   // 0x2
};

/* "AI Orders" (OVL1 0x2BB8E0's handler) */
struct NetMsgAIOrders {
    unsigned short m_0;   // 0x0
    unsigned char _2[2];
    unsigned char m_4;   // 0x4
};

/* "E3 Index" (OVL1 0x2BAF30's handler) */
struct NetMsgE3Index {
    int m_0;   // 0x0
    unsigned char m_4;   // 0x4
};

/* "Reject" (OVL1 0x2B9D80's handler) */
struct NetMsgReject {
    int m_0;   // 0x0
};

/* "Message" (OVL1 0x2BAC40's handler) */
struct NetMsgMessage {
    unsigned short m_0;   // 0x0
    unsigned char _2[2];
    unsigned short m_4;   // 0x4
};

/* "Chat" (OVL1 0x2BA9D0's handler) */
struct NetMsgChat {
    unsigned short m_0;   // 0x0
    unsigned short m_2;   // 0x2
    unsigned char m_4;   // 0x4
};

/* "Wep PU" (OVL1 0x2BA4E0's handler) */
struct NetMsgWepPU {
    int m_0;   // 0x0
    int m_4;   // 0x4
    int m_8;   // 0x8
    int m_c;   // 0xc
    int m_10;   // 0x10
    unsigned short m_14;   // 0x14
};

/* "Wep Ammo PU" (OVL1 0x2BA370's handler) */
struct NetMsgWepAmmoPU {
    int m_0;   // 0x0
    unsigned short m_4[8];   // 0x4
    unsigned short m_14[8];   // 0x14
    int m_24;   // 0x24
    int m_28;   // 0x28
    int m_2c;   // 0x2c
    unsigned char m_30;   // 0x30
};

/* "PU May I" (OVL1 0x2BA260's handler) */
struct NetMsgPUMayI {
    int m_0;   // 0x0
    unsigned short m_4[8];   // 0x4
    unsigned short m_14;   // 0x14
    unsigned char m_16;   // 0x16
};

/* "PU OK" (OVL1 0x2BA1A0's handler) */
struct NetMsgPUOK {
    int m_0;   // 0x0
    unsigned short m_4;   // 0x4
    unsigned char m_6;   // 0x6
    unsigned char m_7;   // 0x7
};

/* "ClimbAsk" (OVL1 0x2B9F10's handler) */
struct NetMsgClimbAsk {
    unsigned short m_0;   // 0x0
    unsigned short m_2;   // 0x2
    unsigned short m_4;   // 0x4
};

/* "ClimbOK" (OVL1 0x2B9E20's handler) */
struct NetMsgClimbOK {
    unsigned short m_0;   // 0x0
    unsigned short m_2;   // 0x2
};

/* "Names" (OVL1 0x2B9B20's handler) */
struct NetMsgNames {
    unsigned short m_0;   // 0x0
    unsigned char m_2;   // 0x2
    unsigned char m_3;   // 0x3
    unsigned char m_4;   // 0x4
    unsigned char m_5;   // 0x5
};

/* "Voter" (OVL1 0x2BA040's handler) */
struct NetMsgVoter {
    unsigned short m_0;   // 0x0
    unsigned char m_2;   // 0x2
};

/* "Spec Vote" (OVL1 0x2B9BD0's handler) */
struct NetMsgSpecVote {
    unsigned short m_0;   // 0x0
    unsigned char m_2;   // 0x2
};

/* "Team Switch" (OVL1 0x2BAE70's handler) */
struct NetMsgTeamSwitch {
    int m_0;   // 0x0
};

/* "All Switch" (OVL1 0x2BADC0's handler) */
struct NetMsgAllSwitch {
    int m_0;   // 0x0
};

/* "Bomb" (OVL1 0x2B9A00's handler) */
struct NetMsgBomb {
    int m_0;   // 0x0
    int m_4;   // 0x4
    int m_8;   // 0x8
    unsigned short m_c;   // 0xc
    unsigned char m_e;   // 0xe
};

/* "Alert" (OVL1 0x2B98B0's handler) */
struct NetMsgAlert {
    int m_0;   // 0x0
    int m_4;   // 0x4
    int m_8;   // 0x8
};

/* "Game Signal" (OVL1 0x2B9150's handler) */
struct NetMsgGameSignal {
    unsigned char m_0[5];   // 0x0
    unsigned char _5[3];
    int m_8;   // 0x8
    unsigned char m_c;   // 0xc
    unsigned char m_d;   // 0xd
    unsigned char m_e;   // 0xe
    unsigned char m_f;   // 0xf
    unsigned char m_10;   // 0x10 (GameSignalPacket's flag: not described)
};


extern "C" {
void func_00620BD8(void);   /* (the DME's: a message type's description begun) */
void func_00620C60(void *base, void *field, int size, int count);   /* (a field described) */
void func_002C2B20();   /* (fts_net_persona.cpp's registration's twin: the platform's? -- SOCOM II's) */
extern unsigned int data_00440EA0;   /* (SOCOM II's: "All Switch") */
int func_002B9B20(int, int, int, void *);
int func_002BADC0(int, int, int, void *);
int func_002B9150(int, int, int, void *);
}

int PickupAckPacketHandler(int, int, int, void *data);   /* 0x2BA1A0 */
int TeamSwitchPacketHandler(int, int, int, void *data);   /* 0x2BAE70 */

class CZGrenadeState {
public:
    static void zNetRegisterGrenade();   /* 0x2C1630 */
};

/* the network's objects and messages registered, once, before the connection (SOCOM 1's
 * ftsNetworkRegister, grown: the five object kinds and SOCOM II's 0x2C2B20, then 24 message types,
 * each described field by field -- "Wep PU" without its own begin, as the target has it -- and
 * registered with its handler, then the voice's; OVL1 0x2B8660). Every description's struct a local
 * of its own, declared in use order: MW stacks the 4-byte ones on top, then the others by size
 * class */
extern "C" void func_002B8660()
{
    NetMsgHit mHit;
    NetMsgAnim mAnim;
    NetMsgPushBlend mPushBlend;
    NetMsgActionAnim mActionAnim;
    NetMsgActionPoints mActionPoints;
    NetMsgAIOrders mAIOrders;
    NetMsgE3Index mE3Index;
    NetMsgReject mReject;
    NetMsgMessage mMessage;
    NetMsgChat mChat;
    NetMsgWepPU mWepPU;
    NetMsgWepAmmoPU mWepAmmoPU;
    NetMsgPUMayI mPUMayI;
    NetMsgPUOK mPUOK;
    NetMsgClimbAsk mClimbAsk;
    NetMsgClimbOK mClimbOK;
    NetMsgNames mNames;
    NetMsgVoter mVoter;
    NetMsgSpecVote mSpecVote;
    NetMsgTeamSwitch mTeamSwitch;
    NetMsgAllSwitch mAllSwitch;
    NetMsgBomb mBomb;
    NetMsgAlert mAlert;
    NetMsgGameSignal mGameSignal;

    if (theNetwork.m_bConnected)
        return;
    CZSealState::zNetRegisterSeal();
    CZPersonaState::zNetRegisterPersona();
    func_002C2B20();
    CZGrenadeState::zNetRegisterGrenade();
    CZGameState::zNetRegisterGame();
    CZBombState::zNetRegisterBomb();
    CZPlatformState::zNetRegisterPlatform();
    func_00620BD8();
    func_00620C60(&mHit, &mHit.m_0, 2, 1);
    func_00620C60(&mHit, &mHit.m_2, 2, 1);
    func_00620C60(&mHit, &mHit.m_4, 1, 1);
    func_00620C60(&mHit, &mHit.m_5, 1, 1);
    func_00620C60(&mHit, &mHit.m_6, 1, 1);
    func_00620C60(&mHit, &mHit.m_7, 1, 1);
    theNetwork.zNetRegisterMessage(&data_00440E00, HitPacketHandler, "Hit");
    ZAnim.m_network_activation_callback = SendZAnimNetworkPacket;
    func_00620BD8();
    func_00620C60(&mAnim, &mAnim.m_0, 4, 1);
    func_00620C60(&mAnim, &mAnim.m_4, 2, 1);
    func_00620C60(&mAnim, &mAnim.m_6, 2, 1);
    theNetwork.zNetRegisterMessage(&data_00440E20, ZAnimNetworkPacketHandler, "Anim");
    func_00620BD8();
    func_00620C60(&mPushBlend, &mPushBlend.m_0, 2, 1);
    func_00620C60(&mPushBlend, &mPushBlend.m_2, 2, 1);
    func_00620C60(&mPushBlend, &mPushBlend.m_4, 2, 1);
    func_00620C60(&mPushBlend, &mPushBlend.m_6, 1, 1);
    func_00620C60(&mPushBlend, &mPushBlend.m_7, 1, 1);
    theNetwork.zNetRegisterMessage(&data_00440E18, PushBlendAnimPacketHandler, "Push Blend");
    func_00620BD8();
    func_00620C60(&mActionAnim, &mActionAnim.m_0, 2, 1);
    func_00620C60(&mActionAnim, &mActionAnim.m_2, 2, 1);
    func_00620C60(&mActionAnim, &mActionAnim.m_4, 2, 1);
    func_00620C60(&mActionAnim, &mActionAnim.m_8, 2, 1);
    theNetwork.zNetRegisterMessage(&data_00440E08, func_002BB310, "Action Anim");
    func_00620BD8();
    func_00620C60(&mActionPoints, &mActionPoints.m_0, 2, 1);
    func_00620C60(&mActionPoints, &mActionPoints.m_2, 2, 1);
    theNetwork.zNetRegisterMessage(&data_00440E10, ActionAnimPacketHandler, "Action Points");
    func_00620BD8();
    func_00620C60(&mAIOrders, &mAIOrders.m_0, 2, 1);
    func_00620C60(&mAIOrders, &mAIOrders.m_4, 1, 1);
    theNetwork.zNetRegisterMessage(&data_00440E28, func_002BB8E0, "AI Orders");
    func_00620BD8();
    func_00620C60(&mE3Index, &mE3Index.m_0, 4, 1);
    func_00620C60(&mE3Index, &mE3Index.m_4, 1, 1);
    theNetwork.zNetRegisterMessage(&data_00440E30, E3IndexPacketHandler, "E3 Index");
    func_00620BD8();
    func_00620C60(&mReject, &mReject.m_0, 4, 1);
    theNetwork.zNetRegisterMessage(&data_00440E90, func_002B9D80, "Reject");
    func_00620BD8();
    func_00620C60(&mMessage, &mMessage.m_0, 2, 1);
    func_00620C60(&mMessage, &mMessage.m_4, 2, 1);
    theNetwork.zNetRegisterMessage(&data_00440E38, func_002BAC40, "Message");
    func_00620BD8();
    func_00620C60(&mChat, &mChat.m_0, 2, 1);
    func_00620C60(&mChat, &mChat.m_2, 2, 1);
    func_00620C60(&mChat, &mChat.m_4, 1, 1);
    theNetwork.zNetRegisterMessage(&data_00440E40, ChatPacketHandler, "Chat");
    func_00620C60(&mWepPU, &mWepPU.m_14, 2, 1);
    func_00620C60(&mWepPU, &mWepPU.m_0, 4, 1);
    func_00620C60(&mWepPU, &mWepPU.m_8, 4, 1);
    func_00620C60(&mWepPU, &mWepPU.m_c, 4, 1);
    func_00620C60(&mWepPU, &mWepPU.m_10, 4, 1);
    func_00620C60(&mWepPU, &mWepPU.m_4, 4, 1);
    theNetwork.zNetRegisterMessage(&data_00440E48, WeaponPUPacketHandler, "Wep PU");
    func_00620BD8();
    func_00620C60(&mWepAmmoPU, &mWepAmmoPU.m_30, 1, 1);
    func_00620C60(&mWepAmmoPU, &mWepAmmoPU.m_4[0], 16, 8);
    func_00620C60(&mWepAmmoPU, &mWepAmmoPU.m_14[0], 16, 8);
    func_00620C60(&mWepAmmoPU, &mWepAmmoPU.m_0, 4, 1);
    func_00620C60(&mWepAmmoPU, &mWepAmmoPU.m_24, 4, 1);
    func_00620C60(&mWepAmmoPU, &mWepAmmoPU.m_28, 4, 1);
    func_00620C60(&mWepAmmoPU, &mWepAmmoPU.m_2c, 4, 1);
    theNetwork.zNetRegisterMessage(&data_00440E50, AmmoPUPacketHandler, "Wep Ammo PU");
    func_00620BD8();
    func_00620C60(&mPUMayI, &mPUMayI.m_16, 1, 1);
    func_00620C60(&mPUMayI, &mPUMayI.m_0, 4, 1);
    func_00620C60(&mPUMayI, &mPUMayI.m_4[0], 16, 8);
    func_00620C60(&mPUMayI, &mPUMayI.m_14, 2, 1);
    theNetwork.zNetRegisterMessage(&data_00440E58, PickupMayIPacketHandler, "PU May I");
    func_00620BD8();
    func_00620C60(&mPUOK, &mPUOK.m_6, 1, 1);
    func_00620C60(&mPUOK, &mPUOK.m_0, 4, 1);
    func_00620C60(&mPUOK, &mPUOK.m_4, 2, 1);
    func_00620C60(&mPUOK, &mPUOK.m_7, 1, 1);
    theNetwork.zNetRegisterMessage(&data_00440E60, PickupAckPacketHandler, "PU OK");
    func_00620BD8();
    func_00620C60(&mClimbAsk, &mClimbAsk.m_0, 2, 1);
    func_00620C60(&mClimbAsk, &mClimbAsk.m_2, 2, 1);
    func_00620C60(&mClimbAsk, &mClimbAsk.m_4, 2, 1);
    theNetwork.zNetRegisterMessage(&data_00440E68, func_002B9F10, "ClimbAsk");
    func_00620BD8();
    func_00620C60(&mClimbOK, &mClimbOK.m_0, 2, 1);
    func_00620C60(&mClimbOK, &mClimbOK.m_2, 2, 1);
    theNetwork.zNetRegisterMessage(&data_00440E70, func_002B9E20, "ClimbOK");
    func_00620BD8();
    func_00620C60(&mNames, &mNames.m_0, 2, 1);
    func_00620C60(&mNames, &mNames.m_2, 1, 1);
    func_00620C60(&mNames, &mNames.m_3, 1, 1);
    func_00620C60(&mNames, &mNames.m_4, 1, 1);
    func_00620C60(&mNames, &mNames.m_5, 1, 1);
    theNetwork.zNetRegisterMessage(&data_00440E78, func_002B9B20, "Names");
    func_00620BD8();
    func_00620C60(&mVoter, &mVoter.m_0, 2, 1);
    func_00620C60(&mVoter, &mVoter.m_2, 1, 1);
    theNetwork.zNetRegisterMessage(&data_00440E80, func_002BA040, "Voter");
    func_00620BD8();
    func_00620C60(&mSpecVote, &mSpecVote.m_0, 2, 1);
    func_00620C60(&mSpecVote, &mSpecVote.m_2, 1, 1);
    theNetwork.zNetRegisterMessage(&data_00440E88, (int (*)(int, int, int, void *))func_002B9BD0, "Spec Vote");
    func_00620BD8();
    func_00620C60(&mTeamSwitch, &mTeamSwitch.m_0, 4, 1);
    theNetwork.zNetRegisterMessage(&data_00440E98, TeamSwitchPacketHandler, "Team Switch");
    func_00620BD8();
    func_00620C60(&mAllSwitch, &mAllSwitch.m_0, 4, 1);
    theNetwork.zNetRegisterMessage(&data_00440EA0, func_002BADC0, "All Switch");
    func_00620BD8();
    func_00620C60(&mBomb, &mBomb.m_0, 4, 1);
    func_00620C60(&mBomb, &mBomb.m_4, 4, 1);
    func_00620C60(&mBomb, &mBomb.m_8, 4, 1);
    func_00620C60(&mBomb, &mBomb.m_c, 2, 1);
    func_00620C60(&mBomb, &mBomb.m_e, 1, 1);
    theNetwork.zNetRegisterMessage(&data_00440EA8, BombStatePacketHandler, "Bomb");
    func_00620BD8();
    func_00620C60(&mAlert, &mAlert.m_0, 4, 1);
    func_00620C60(&mAlert, &mAlert.m_4, 4, 1);
    func_00620C60(&mAlert, &mAlert.m_8, 4, 1);
    theNetwork.zNetRegisterMessage(&data_00440EB8, func_002B98B0, "Alert");
    func_00620BD8();
    func_00620C60(&mGameSignal, &mGameSignal.m_c, 1, 1);
    func_00620C60(&mGameSignal, &mGameSignal.m_d, 1, 1);
    func_00620C60(&mGameSignal, &mGameSignal.m_e, 1, 1);
    func_00620C60(&mGameSignal, &mGameSignal.m_f, 1, 1);
    func_00620C60(&mGameSignal, &mGameSignal.m_8, 4, 1);
    func_00620C60(&mGameSignal, &mGameSignal.m_0[0], 1, 5);
    theNetwork.zNetRegisterMessage(&data_00440EB0, func_002B9150, "Game Signal");
    data_0045B718->zNetRegisterVoice();
}

/* the DME's set up: no session master yet, every client's word cleared, the connect and
 * disconnect callbacks registered (SOCOM 1's DMEinit, much grown; OVL1 0x2B85C0) */
extern "C" bool func_002B85C0()
{
    theNetwork.m_bSessionMaster = false;
    for (int i = 0; i < 24; i++)
        data_00440EC0[i] = 0;
    if (theNetwork.zNetConnect(func_002BC620, func_002BC530))
        return false;
    func_002C3330(0);
    return true;
}

extern "C" {
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
    theNetwork.m_sessionmaster_index = theNetwork.zNetGetGameSessionMasterClientIndex();
    func_00310080(data_0045B718, 10000);
    func_0030FD90(data_0045B718, 11);
    return true;
}


extern "C" {
bool func_002C30B0(CZSealBody *body);
void func_00543D50(CZSealBody *body);   /* (OVL2) */
}

/* an AI map's cell, a named one and the eight directions (ai.h's CAiMapLoc, CAiMap::name, DIR) */
class CAiMapLoc {
public:
    unsigned int m_word;
};

class CAiMap {
public:
    enum DIR { N = 0, NE = 1, E = 2, SE = 3, S = 4, SW = 5, W = 6, NW = 7, DIR_COUNT = 8 };
    class name {
    public:
        CAiMapLoc m_loc;   // 0x0
        char m_name[16];   // 0x4
        unsigned char m_14;   // 0x14: the direction it faces
    };

    static CPnt3D &GetDirVec(DIR dir, CPnt3D &v);   /* OVL2 0x519130 */
};

/* the AI maps (ai_maps.h's CAiMaps: only the spawn points and the named cells) */
class CAiMaps {
public:
    CAiMap::name *GetCellName(const CAiMapLoc &loc) const;   /* OVL2 0x52A820 */
    bool GetLocation(const char *name, CAiMapLoc &loc);   /* OVL2 0x52DB70 */
    CPnt3D *GetCellCenter(const CAiMapLoc &loc, CPnt3D *center);   /* OVL2 0x52D480 */
    int GetSporeCount(unsigned int id);   /* OVL2 0x52B7E0 */
    bool GetSporeLocation(unsigned int type, unsigned int n, CPnt3D *pos, CPnt3D *dir);   /* OVL2 0x52B5C0 */
};

extern CAiMaps AiMaps;

/* the entity as a SEAL, if it is one (seal.cpp's AsSeal) */
inline CZSealBody *AsSeal(CEntity *e)
{
    return e && e->m_type == 2 ? (CZSealBody *)e : 0;
}

/* the spawn point of a kind farthest from the enemy: of the SEAL's client's share of the points (a
 * 24th of them, from its client's index on), the one whose nearest enemy SEAL is farthest away,
 * squared distances (SOCOM II's; OVL1 0x2B7EE0 -- 0x2B8100, the spawn point's finder, calls it with
 * the kind's point count). The target's shape: the entity list's begin and end taken once ahead of
 * the walk (initialised -- the end then lives in a register, the begin is assigned to the walking
 * iterator each pass, a bitwise copy into its slot), the team test with the SEAL as `this` and the
 * player the inline's parameter (the player's team address hoisted, the player itself dead), the
 * kind an `int` (an `unsigned int` homes it at the entry, the target in the loop's preheader), the
 * counts declared ahead of the best distance (the division scheduled first) and the nearest distance
 * declared at the top, defined in the loop (it numbers below the best: $f21 against $f20) */
extern "C" int func_002B7EE0(CZSealBody *self, int type, int count)
{
    CEntityList::const_iterator i;
    float nearest;
    int per = count / 24;
    int first = per * self->m_fc8;
    int last = first + per;
    int pick = first;
    float best = -1.0f;
    CEntityList::const_iterator start = CEntity::GetEntityList().begin();
    CEntityList::const_iterator end = CEntity::GetEntityList().end();

    for (int k = first; k < last; k++) {
        nearest = -1.0f;
        CPnt3D pos, dir;

        AiMaps.GetSporeLocation(type, k, &pos, &dir);
        for (i = start; i != end; ++i) {
            CZSealBody *seal = AsSeal(*i);

            if (seal && seal->IsEnemy(self)) {
                CPnt3D d;

                pos.Sub(&seal->m_posW, &d);
                float dist = d.x * d.x + d.y * d.y + d.z * d.z;

                if (dist < nearest || nearest == -1.0f)
                    nearest = dist;
            }
        }
        if (!(nearest <= best)) {
            best = nearest;
            pick = k;
        }
    }
    return pick;
}

extern "C" int rand();

/* the network game (OVL1 bss 0x437CE8: hud_ammo.cpp's CNetGameState stand-in) */
extern CZNetGame *data_00437CE8;

/* the cross product of two points made through VU0 and normalised (CPnt3D::Cross(b, out, true), as
 * seal_control.cpp writes it: inlined here; the name ours) */
static inline void CrossNormal(const CPnt3D *a, const CPnt3D *b, CPnt3D *out)
{
    sceVu0FVECTOR r;
    sceVu0FVECTOR va;
    sceVu0FVECTOR vb;

    va[0] = a->x;
    va[1] = a->y;
    va[2] = a->z;
    va[3] = 1.0f;
    vb[0] = b->x;
    vb[1] = b->y;
    vb[2] = b->z;
    vb[3] = 1.0f;
    sceVu0OuterProduct(r, va, vb);
    sceVu0Normalize(r, r);
    out->x = r[0];
    out->y = r[1];
    out->z = r[2];
}

/* a draw below n, an inline of its own: its exit ends the pick's arm on a label MW pads to 8 bytes
 * (the target's nop after the `mfhi`) and leaves the zero arm's `b` its nop -- the draw written in
 * the arm lets MW fill that slot from the join, 8 bytes short (the AI core's round-22 shape, as
 * CAiMaps::GetRandomLoc's _rnd and CVehicleRdrEntry::Randomize's RandomDraw; the name ours) */
static inline int RandDraw(int n)
{
    return rand() % n;
}

/* a SEAL's spawn point and its facing as a matrix (SOCOM II's; OVL1 0x2B8100 -- ftsNetworkPlayerInit
 * and OVL2's RemoteRespawn call it): in a respawning game a point of the kind asked -- for kinds 0
 * and 2 one at random from the SEAL's client's share (a 24th of them), otherwise the one farthest
 * from the enemy (0x2B7EE0) --; a spectator at the map's "spectator" cell, facing its direction;
 * else the point of the client's own index. Found, it is lifted a unit and the matrix's rows made the
 * side (up crossed with the facing), up, the facing and the point.
 * Byte-exact (the states' round 38; open since round 36) from three things together: the random
 * arm's client and share are both over K and coloured highest-index first, so the target's client
 * s0 / share s1 need the share numbered first (round 37, MW's colouring read with igdump); the zero
 * arm laid out first with the draw an inline of its own (RandDraw above: the join padded, the `b`
 * keeping its nop); and the points declared facing, side, up (the frame laid out up 160, side 176,
 * facing 192 -- the only one of the 24 orders of the three and the flag that gives it) */
extern "C" void func_002B8100(CZSealBody *body, CPnt3D *pos, CMatrix *m, int type)
{
    CPnt3D dir;
    CPnt3D side;
    CPnt3D up(0.0f, 1.0f, 0.0f);
    bool found = false;

    if (data_00437CE8 && data_00437CE8->m_respawn) {
        int count = AiMaps.GetSporeCount(type);
        int n;

        if (type == 2 || type == 0) {
            int per = count / 24;
            int client = body->m_fc8;
            int range = per - 1;
            int r = range == 0 ? 0 : RandDraw(range + 1);

            n = r + per * client;
        } else
            n = func_002B7EE0(body, type, count);
        found = AiMaps.GetSporeLocation(type, n, pos, &dir);
    } else if (func_002C2FD0(body)) {
        CAiMapLoc loc;

        if (AiMaps.GetLocation("spectator", loc)) {
            CAiMap::name *cell = AiMaps.GetCellName(loc);

            found = AiMaps.GetCellCenter(loc, pos) != 0;
            CAiMap::GetDirVec((CAiMap::DIR)cell->m_14, dir);
        }
    } else {
        int client = body->m_fc8;

        AiMaps.GetSporeCount(type);
        found = AiMaps.GetSporeLocation(type, client, pos, &dir);
    }
    if (found) {
        pos->y += 1.0f;
        CrossNormal(&up, &dir, &side);
        m->SetRows(&side, &up, &dir, pos);
    }
}

/* the player's SEAL put into a network game: its network object made and set as this console's,
 * moved to its spawn point (SOCOM II's finder where SOCOM 1 asked the AI maps for a spore), given
 * the squad's order 31, made anew at the game's start when the game says so, and sent (SOCOM 1's
 * ftsNetworkPlayerInit, rebuilt; OVL1 0x2B7D60) */
void ftsNetworkPlayerInit()
{
    if (!theNetwork.m_bNetwork)
        return;
    CZSealBody *player = ftsGetPlayer();

    player->m_4 = CZSealState::CreateLocalSeal(player, true);
    theNetwork.zNetSetMyClientObject(player->m_4);
    CPnt3D pos = CPnt3D::zero;
    CMatrix m;

    m.SetIdentity();
    func_002B8100(player, &pos, &m, func_002C30B0(player) ? 2 : 0);
    m.m_matrix[3][0] = pos.x;
    m.m_matrix[3][1] = pos.y;
    m.m_matrix[3][2] = pos.z;
    player->TeleportTo(&m);
    CSealUnit *unit = CSealUnit::GetUnitByTeam(24);

    if (unit)
        unit->IssueCommand(player, (FT_COMMAND)31, 0, false);
    CZNetGame *game = theMission.m_pNetGame;

    if (func_002C2FA0())
        func_00543D50(player);
    else if (game->m_d2) {
        if (game->m_respawn) {
            func_00543D50(player);
            game->m_d2 = 0;
        } else
            func_00543D50(player);
    }
    func_002BD640(player, 1);
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

/* a turret's network object (SOCOM II's; OVL1 0x2B7C90) */
extern "C" void func_002B7C90(CTurret *turret)
{
    if (!(theNetwork.m_bNetwork && theNetwork.m_bSessionMaster && turret))
        return;
    turret->m_4 = CZPlatformState::CreateLocalPlatform(turret);
}

/* the entity as a turret, if it is one (AsSeal's twin, ours) */
inline CTurret *AsTurret(CEntity *e)
{
    return e && e->m_type == 3 ? (CTurret *)e : 0;
}

/* whether a SEAL is a player's (its controller asked; ours) */
inline bool IsPlayerSeal(CZSealBody *seal)
{
    return seal->m_ctrl ? seal->m_ctrl->IsPlayer() : false;
}

extern "C" unsigned char data_0044CE50;   /* (set by OVL2's NetworkPlayerCreate: the objects asked for) */

/* the network objects made when asked (SOCOM II's; OVL1 0x2B7A90): the request cleared, the player's
 * SEAL put in, then every player SEAL's and every turret's local object made by the session master
 * (0x2B7CF0 and 0x2B7C90 inlined), and the mission's team valves refreshed */
extern "C" void func_002B7A90()
{
    if (!data_0044CE50)
        return;
    data_0044CE50 = 0;
    ftsNetworkPlayerInit();
    CEntityList::const_iterator i;

    for (i = CEntity::GetEntityList().begin(); i != CEntity::GetEntityList().end(); ++i) {
        CZSealBody *seal = AsSeal(*i);
        CTurret *turret = AsTurret(*i);

        if (seal) {
            if (IsPlayerSeal(seal))
                func_002B7CF0(0, seal);
        } else if (turret)
            func_002B7C90(turret);
    }
    theMission.RefreshTeamValves();
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
    CPickup::NetTick(0, 0.0f);
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
