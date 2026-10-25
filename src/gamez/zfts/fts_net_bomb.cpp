/* fts_net_bomb.cpp -- the bomb's network object: CZBombState (SOCOM 1's fts_net_bomb.cpp: the
 * demolition game's one bomb, its state machine and its object, which the session master
 * owns). OVL1 0x2C1970-0x2C27FF, after fts_net_grenade.cpp and before the blather state's unit.
 * Complete: 13 functions, all byte-exact but HandleBombStateRequest. Defined in reverse of their
 * image order (-inline deferred). */

#include "../zanim/zanim.h"

/* a network object as CZNetwork's table hands it out (only its data) */
struct NetObject {
    unsigned char _0[0x1c];
    void *m_data;   // 0x1c
};

/* the DME's ownership and lifespan of an object (their values are not in SOCOM 1's DWARF, only
 * their names in zNetCreateObject's) */
enum NetOwnershipStatus {};
enum NetObjectLifespan {};

class CZNetwork {
public:
    int zNetGetTime();
    void zNetRegisterRemoteObjectCallback(int index, void (*creation)(int, int),
                                          void (*update)(int, int, int),
                                          void (*postupdate)(int, int, int),
                                          void (*deletion)(int, int));
    int zNetCreateObject(int *index, NetOwnershipStatus own, NetObjectLifespan life, unsigned int a,
                         int b, unsigned int c, int d, void *e, char *name, int structure);
    NetObject *zNetGetObject(int index);

    unsigned char m_bSessionMaster;   // 0x0
};

extern CZNetwork theNetwork;

/* the game's clock (SOCOM 1's CNetClock without its saved maximum and its delta: SOCOM II's
 * SetInitialRead, 0x2A6AC0, stores at 4 and 0x14) */
struct CNetClock {
    unsigned int m_max_game_time;   // 0x0
    unsigned int m_initial_read;   // 0x4
    unsigned int m_time_in_round;   // 0x8
    unsigned int m_game_time;   // 0xc
    unsigned int m_total_time;   // 0x10
    unsigned char m_initial_read_set;   // 0x14
    unsigned char m_clock_running;   // 0x15
};

/* the online game (only its bomb valves, its clock and its bomb flags) */
class CZNetGame {
public:
    unsigned char _0[0x18];
    CValve *m_pBombValve;   // 0x18: "mp_bomb"
    CValve *m_pBombOwnerValve;   // 0x1c: "bomb_owner"
    unsigned char _20[0xd5 - 0x20];
    unsigned char m_d5;   // 0xd5: the placed bomb's clock started (SOCOM 1's at 0x9c)
    unsigned char m_d6;   // 0xd6: the bomb's object known (SOCOM 1's at 0x9d)
    unsigned char _d7[0xe0 - 0xd7];
    CNetClock m_clock;   // 0xe0 (SOCOM 1's at 0xa0)
    unsigned char _f8[0x113 - 0xf8];
    unsigned char m_mp_major_game_state;   // 0x113 (SOCOM 1's at 0xdb)
};

class CMission;

extern "C" int func_002A6AD0(CMission *mission);   /* (the mission's bomb time) */

class CMission {
public:
    unsigned char _0[0x1808];
    CZNetGame *m_pNetGame;   // 0x1808 (SOCOM 1's at 0x584)
};

extern CMission theMission;

class CZAnim;

/* a SEAL's body: only what the bomb's changes reach -- its kit at 0x5E0 (SOCOM 1's at 0x520) and
 * the member at 0x58C the bomb's events go to (zseal.h has the whole of it) */
class CZSealBody {
public:
    void DropAllItems();
    static void CreateMPBombPickup(const CPnt3D *pos, bool placed);

    unsigned char _0[0x58c];
    unsigned char m_58c[0x5e0 - 0x58c];   // 0x58c
    unsigned char m_kit[4];   // 0x5e0: its CZKit
};

/* the pickups' MP bomb node (CPickup::m_mpbombnode, OVL1 bss 0x44D4A0) */
class CPickup {
public:
    static zdb::CNode *m_mpbombnode;
};

/* the SEALs' network objects (fts_net_seal.cpp's) */
class CZSealState {
public:
    static CZSealBody *GetBodyFromObjectIndex(int index);
};

/* the AI's maps (OVL2's AiMaps): a named location's cell and its centre */
class CAiMapLoc {
public:
    unsigned int m_word;
};

class CAiMaps {
public:
    bool GetLocation(const char *name, CAiMapLoc &loc);
    bool GetCellCenter(const CAiMapLoc &loc, CPnt3D *center);
};

extern CAiMaps AiMaps;

class CZBombState {
public:
    /* SOCOM 1's DWARF */
    enum EnumBombState {
        BOMB_UNKNOWN = 0,
        BOMB_GROUNDED = 1,
        BOMB_CAPTURED = 2,
        BOMB_PLACED = 3,
        BOMB_DEFUSED = 4,
        BOMB_DETONATED = 5
    };

    struct TypeBombStatus {
        EnumBombState CurrentBombState;   // 0x0
        EnumBombState NewBombState;   // 0x1
        CPnt3D BombLocation;   // 0x4
        CZAnim *pBombAnimation;   // 0x10
        int HolderObjectIndex;   // 0x14
        int DefuserObjectIndex;   // 0x18
        int BombObjectIndex;   // 0x1c
    };

    static void UninitBomb();
    static void InitBomb();
    static void CreateBombHere(CPnt3D *pos);
    static void ChangeBombState();
    static void HandleBombStateRequest(int who, unsigned char state, CPnt3D *pos);
    static void RequestNewBombState(int who, EnumBombState state, const CPnt3D *pos);
    static void UpdateLocalBomb();
    static void RemoteBombDeletionCallback(int client, int index);
    static void RemoteBombPostUpdateCallback(int client, int index, int field);
    static void RemoteBombUpdateCallback(int client, int index, int field);
    static void RemoteBombCreationCallback(int client, int index);
    static void zNetRegisterBomb();
    static int CreateLocalBomb();

    static TypeBombStatus m_BombStatus;
    static int m_NetStructIndexBombObject;
};

/* the bomb's network data: 20 bytes (SOCOM 1's 16) */
struct TypeBombObject {
    PNT3D pos;   // 0x0
    unsigned short holder;   // 0xc: its holder's network object
    unsigned short defuser;   // 0xe: its defuser's
    unsigned char state;   // 0x10
};

/* CValve::SetValue, inlined in both builds (CValve, VALVE_STATE and MakeCallbacks are zanim.h's) */
inline void SetValveValue(CValve *valve, int value)
{
    VALVE_STATE state = VALVE_SAME;

    if (valve->m_value != value)
        state = value == 0 ? VALVE_OFF : VALVE_ON;
    valve->m_value = value;
    if (state != VALVE_SAME)
        valve->MakeCallbacks(state);
}

void SendBombStateRequest(int who, unsigned char state, const CPnt3D *pos);

extern "C" {
/* the four fields' ids */
extern int data_00441380;
extern int data_00441388;
extern int data_00441390;
extern int data_00441398;

void NetRegisterObjectStart();   /* (the DME's) */
void NetFreeObject(int index);
void NetObjectField(int *field, int offset, int a, int b, int c, void *type);
void NetRegisterStructure(int *index, const char *name, int size);
void *func_002BD060();   /* (fts_remote.cpp's broadcast schedules) */
void *func_002BD0A0();
bool func_002A72A0(CZNetGame *game);   /* (SOCOM II's: a game type 1 or 2) */
void func_0059DCE0(int state);
void *memset(void *dst, int c, unsigned int n);
void *memcpy(void *dst, const void *src, unsigned int n);
void func_002A6AC0(CNetClock *clock, unsigned int time, bool set);   /* (SOCOM 1's CNetClock::SetInitialRead) */
/* the SEAL's side of the bomb (OVL2; SOCOM 1's CZKit::PickupMPBomb, CZKit::HandlePlaceMPBomb and
 * CZSealBody::HandleBombDefuse at the same calls, then the three events SOCOM II adds) */
void func_005BBF10(void *kit);
void func_005BC200(void *kit, const CPnt3D *pos);
void func_0059DA70(CZSealBody *body);
void func_005455D0(void *events, CZSealBody *body);
void func_00545650(void *events, CZSealBody *body, int a);
void func_00545750(void *events, CZSealBody *body, int a);
bool func_002C30B0(CZSealBody *body);   /* (the persona unit's: the owner's side) */
extern int data_004413A0;   /* (the bomb's time) */
}

/* the kind registered: its four fields, their offsets taken off a pointer to a local, as
 * fts_net_grenade.cpp's (OVL1 0x2C26E0) */
void CZBombState::zNetRegisterBomb()
{
    TypeBombObject b;
    TypeBombObject *p = &b;

    NetRegisterObjectStart();
    NetObjectField(&data_00441380, (int)&p->pos - (int)p, 12, 13, 1, func_002BD060());
    NetObjectField(&data_00441388, (int)&p->state - (int)p, 1, 1, 1, func_002BD0A0());
    NetObjectField(&data_00441390, (int)&p->holder - (int)p, 2, 3, 1, func_002BD0A0());
    NetObjectField(&data_00441398, (int)&p->defuser - (int)p, 2, 3, 1, func_002BD0A0());
    NetRegisterStructure(&m_NetStructIndexBombObject, "BombState", sizeof(TypeBombObject));
    theNetwork.zNetRegisterRemoteObjectCallback(m_NetStructIndexBombObject,
                                                RemoteBombCreationCallback,
                                                RemoteBombUpdateCallback,
                                                RemoteBombPostUpdateCallback,
                                                RemoteBombDeletionCallback);
}

/* the bomb's object appears: the two bomb valves made and cleared, the status reset, the
 * explosion's anim found, and the object's fields taken up (OVL1 0x2C2590) */
void CZBombState::RemoteBombCreationCallback(int client, int index)
{
    NetObject *obj = theNetwork.zNetGetObject(index);

    if (obj && obj->m_data) {
        CZNetGame *game = theMission.m_pNetGame;

        if (game) {
            game->m_pBombValve = CValve::Create("mp_bomb", (VALVE_TYPE)1);
            SetValveValue(game->m_pBombValve, 0);
            game->m_pBombOwnerValve = CValve::Create("bomb_owner", (VALVE_TYPE)1);
            SetValveValue(game->m_pBombOwnerValve, 0);
            m_BombStatus.NewBombState = BOMB_UNKNOWN;
            m_BombStatus.CurrentBombState = BOMB_UNKNOWN;
            m_BombStatus.DefuserObjectIndex = 0;
            m_BombStatus.HolderObjectIndex = 0;
            m_BombStatus.BombObjectIndex = index;
            m_BombStatus.pBombAnimation = ZAnim.GetAnim("base_explosion1", 0);
            game->m_d6 = 1;
            RemoteBombUpdateCallback(client, index, -1);
            RemoteBombPostUpdateCallback(client, index, -1);
        }
    }
}

/* a remote update: before the object is known, its creation; then its place, holder and defuser
 * (OVL1 0x2C2450) */
void CZBombState::RemoteBombUpdateCallback(int client, int index, int field)
{
    CZNetGame *game = theMission.m_pNetGame;

    if (!game || !func_002A72A0(game))
        return;
    if (!game->m_d6) {
        RemoteBombCreationCallback(client, index);
        return;
    }
    NetObject *obj = theNetwork.zNetGetObject(index);

    if (obj) {
        TypeBombObject *data = (TypeBombObject *)obj->m_data;

        if (data) {
            if (field == data_00441380 || field == -1)
                m_BombStatus.BombLocation = CPnt3D(data->pos.x, data->pos.y, data->pos.z);
            if (field == data_00441390 || field == -1)
                m_BombStatus.HolderObjectIndex = data->holder;
            if (field == data_00441398 || field == -1)
                m_BombStatus.DefuserObjectIndex = data->defuser;
        }
    }
}

/* after an update, in a bomb game: the state the object carries taken up (OVL1 0x2C23B0; pinned:
 * the creation callback calls it, where -inline auto would take it whole) */
#pragma auto_inline off
void CZBombState::RemoteBombPostUpdateCallback(int client, int index, int field)
{
    CZNetGame *game = theMission.m_pNetGame;

    if (!game || !func_002A72A0(game))
        return;
    if (game->m_d6) {
        NetObject *obj = theNetwork.zNetGetObject(index);

        if (obj) {
            TypeBombObject *data = (TypeBombObject *)obj->m_data;

            if (data) {
                EnumBombState state = (EnumBombState)data->state;

                if (m_BombStatus.CurrentBombState != state) {
                    m_BombStatus.NewBombState = state;
                    ChangeBombState();
                }
            }
        }
    }
}

#pragma auto_inline reset

/* the bomb's object gone (OVL1 0x2C23A0; SOCOM 1's did nothing) */
void CZBombState::RemoteBombDeletionCallback(int client, int index)
{
    func_0059DCE0(m_BombStatus.CurrentBombState);
}

/* the bomb's object made by the session master: its status restarted from the state it had, the
 * explosion's anim found, the time a placed bomb has left (OVL1 0x2C2260) */
int CZBombState::CreateLocalBomb()
{
    int index;

    if (theNetwork.zNetCreateObject(&index, (NetOwnershipStatus)1, (NetObjectLifespan)2, 0, 0, 0,
                                    0, 0, "BombObject", m_NetStructIndexBombObject))
        return -1;
    NetObject *obj = theNetwork.zNetGetObject(index);

    if (!obj)
        return -1;
    memset(obj->m_data, 0, sizeof(TypeBombObject));
    m_BombStatus.BombObjectIndex = index;
    m_BombStatus.pBombAnimation = ZAnim.GetAnim("base_explosion1", 0);
    m_BombStatus.NewBombState = m_BombStatus.CurrentBombState;
    m_BombStatus.CurrentBombState = BOMB_UNKNOWN;
    if (m_BombStatus.NewBombState == BOMB_GROUNDED)
        m_BombStatus.HolderObjectIndex = 0;
    if (m_BombStatus.NewBombState == BOMB_PLACED)
        data_004413A0 = func_002A6AD0((CMission *)&theMission);
    else
        data_004413A0 = 30;
    return index;
}

/* the session master's bomb, in play: a placed bomb whose clock ran out detonated, a state asked
 * for taken up, the clock started once it is placed, and the object's data written (OVL1 0x2C20C0;
 * SOCOM 1's, the clock's time now the mission's) */
void CZBombState::UpdateLocalBomb()
{
    NetObject *obj;
    CZNetGame *game;

    if (!theNetwork.m_bSessionMaster)
        return;
    game = theMission.m_pNetGame;
    if (!(game && func_002A72A0(game) && game->m_mp_major_game_state == 3))
        return;
    if (!game->m_d6)
        return;
    obj = theNetwork.zNetGetObject(m_BombStatus.BombObjectIndex);

    if (!obj)
        return;
    if (m_BombStatus.CurrentBombState == BOMB_PLACED && game->m_clock.m_game_time == 0)
        m_BombStatus.NewBombState = BOMB_DETONATED;
    if (m_BombStatus.CurrentBombState != m_BombStatus.NewBombState)
        ChangeBombState();
    if (m_BombStatus.CurrentBombState == BOMB_PLACED && !game->m_d5) {
        game->m_clock.m_max_game_time = data_004413A0 * 1000;
        func_002A6AC0(&game->m_clock, theNetwork.zNetGetTime(), true);
        game->m_d5 = 1;
    }
    void *data = obj->m_data;
    TypeBombObject b;

    b.state = m_BombStatus.CurrentBombState;
    b.pos.x = m_BombStatus.BombLocation.x;
    b.pos.y = m_BombStatus.BombLocation.y;
    b.pos.z = m_BombStatus.BombLocation.z;
    b.holder = m_BombStatus.HolderObjectIndex;
    b.defuser = m_BombStatus.DefuserObjectIndex;
    memcpy(data, &b, sizeof b);
}

/* a new state asked of the session master, in an online game (OVL1 0x2C2090) */
void CZBombState::RequestNewBombState(int who, EnumBombState state, const CPnt3D *pos)
{
    if (theMission.m_pNetGame)
        SendBombStateRequest(who, state, pos);
}

/* a SEAL's request, on the session master: picked up when grounded, dropped or placed by its
 * holder, defused once placed -- then the bomb updated (OVL1 0x2C1F10; SOCOM 1's).
 * OPEN, 0x180 against 0x178: right but for where the state's copy into s2 lands -- the target (and
 * SOCOM 1's build) sinks `move s2,a1` into GetBodyFromObjectIndex's delay slot, past the session
 * master's test, where MW keeps it in the entry block; the empty slot then shifts every label pad.
 * Tried: the two tests joined (|| and an enclosing &&), the body in a local, the state copied to a
 * local (uchar, int, before and after the test), switch on (EnumBombState) or (int), pos copied */
void CZBombState::HandleBombStateRequest(int who, unsigned char state, CPnt3D *pos)
{
    if (!theNetwork.m_bSessionMaster)
        return;
    if (!CZSealState::GetBodyFromObjectIndex(who))
        return;
    switch (state) {
    case BOMB_GROUNDED:
        if (m_BombStatus.CurrentBombState == BOMB_CAPTURED && m_BombStatus.HolderObjectIndex == who) {
            m_BombStatus.NewBombState = BOMB_GROUNDED;
            m_BombStatus.BombLocation = *pos;
        }
        break;
    case BOMB_CAPTURED:
        if (m_BombStatus.CurrentBombState == BOMB_GROUNDED) {
            m_BombStatus.NewBombState = BOMB_CAPTURED;
            m_BombStatus.HolderObjectIndex = who;
        }
        break;
    case BOMB_PLACED:
        if (m_BombStatus.CurrentBombState == BOMB_CAPTURED && m_BombStatus.HolderObjectIndex == who) {
            m_BombStatus.NewBombState = BOMB_PLACED;
            m_BombStatus.BombLocation = *pos;
        }
        break;
    case BOMB_DEFUSED:
        if (m_BombStatus.CurrentBombState == BOMB_PLACED) {
            m_BombStatus.NewBombState = BOMB_DEFUSED;
            m_BombStatus.DefuserObjectIndex = who;
        }
        break;
    }
    UpdateLocalBomb();
}

/* the state asked for taken up: the SEALs and the pickups told, the bomb's anim run when it goes
 * off, then the two valves set -- the owner's to its side, or cleared while the bomb is on the
 * ground (OVL1 0x2C1C30; SOCOM 1's, SOCOM II's events added) */
void CZBombState::ChangeBombState()
{
    CZNetGame *game = theMission.m_pNetGame;
    CZSealBody *body = 0;

    switch (m_BombStatus.NewBombState) {
    case BOMB_GROUNDED: {
        CZSealBody *holder = CZSealState::GetBodyFromObjectIndex(m_BombStatus.HolderObjectIndex);

        if (holder)
            holder->DropAllItems();
        else
            CZSealBody::CreateMPBombPickup(&m_BombStatus.BombLocation, false);
        body = 0;
        break;
    }
    case BOMB_CAPTURED:
        body = CZSealState::GetBodyFromObjectIndex(m_BombStatus.HolderObjectIndex);
        if (body)
            func_005BBF10(body->m_kit);
        break;
    case BOMB_PLACED:
        body = CZSealState::GetBodyFromObjectIndex(m_BombStatus.HolderObjectIndex);
        if (body) {
            func_005BC200(body->m_kit, &m_BombStatus.BombLocation);
            func_005455D0(body->m_58c, body);
        } else
            CZSealBody::CreateMPBombPickup(&m_BombStatus.BombLocation, true);
        break;
    case BOMB_DEFUSED:
        body = CZSealState::GetBodyFromObjectIndex(m_BombStatus.DefuserObjectIndex);
        if (body) {
            func_0059DA70(body);
            func_00545650(body->m_58c, body, 0);
        }
        if (game)
            game->m_clock.m_clock_running = false;
        break;
    case BOMB_DETONATED: {
        body = CZSealState::GetBodyFromObjectIndex(m_BombStatus.HolderObjectIndex);
        if (body)
            func_00545750(body->m_58c, body, 0);
        zdb::CNode *node = CPickup::m_mpbombnode;

        if (m_BombStatus.pBombAnimation && node) {
            m_BombStatus.pBombAnimation->ActivateArg(1, 3, node, 0);
            node->SetActive(false);
        }
        break;
    }
    }
    m_BombStatus.CurrentBombState = m_BombStatus.NewBombState;
    if (game) {
        SetValveValue(game->m_pBombValve, m_BombStatus.CurrentBombState);
        if (body)
            SetValveValue(game->m_pBombOwnerValve, func_002C30B0(body) ? 2 : 1);
        else if (m_BombStatus.CurrentBombState != BOMB_PLACED && m_BombStatus.CurrentBombState != BOMB_DEFUSED &&
                 m_BombStatus.CurrentBombState != BOMB_DETONATED)
            SetValveValue(game->m_pBombOwnerValve, 0);
    }
}

/* the bomb dropped here (OVL1 0x2C1BF0) */
void CZBombState::CreateBombHere(CPnt3D *pos)
{
    m_BombStatus.NewBombState = BOMB_GROUNDED;
    m_BombStatus.BombLocation = *pos;
    m_BombStatus.HolderObjectIndex = 0;
    UpdateLocalBomb();
}

/* the session master's bomb set up once per game: its status cleared, its object made, the two bomb
 * valves made and cleared, and the bomb grounded at the map's "StartBomb" cell (OVL1 0x2C1A80;
 * SOCOM 1's, its object's creation now CreateLocalBomb) */
void CZBombState::InitBomb()
{
    CZNetGame *game = theMission.m_pNetGame;

    if (!game)
        return;
    if (!theNetwork.m_bSessionMaster)
        return;
    if (game->m_d6)
        return;
    m_BombStatus.NewBombState = BOMB_UNKNOWN;
    m_BombStatus.CurrentBombState = BOMB_UNKNOWN;
    m_BombStatus.DefuserObjectIndex = 0;
    m_BombStatus.HolderObjectIndex = 0;
    m_BombStatus.BombObjectIndex = 0;

    CAiMapLoc loc;
    CPnt3D pos;

    if (AiMaps.GetLocation("StartBomb", loc))
        AiMaps.GetCellCenter(loc, &pos);
    CreateLocalBomb();
    game->m_d6 = 1;
    game->m_d5 = 0;
    game->m_pBombValve = CValve::Create("mp_bomb", (VALVE_TYPE)1);
    SetValveValue(game->m_pBombValve, 0);
    game->m_pBombOwnerValve = CValve::Create("bomb_owner", (VALVE_TYPE)1);
    SetValveValue(game->m_pBombOwnerValve, 0);
    CreateBombHere(&pos);
}

/* the bomb put away at a game's end: its object freed by the session master, the flags, status and
 * both valves cleared (OVL1 0x2C1970; SOCOM 1's) */
void CZBombState::UninitBomb()
{
    CZNetGame *game = theMission.m_pNetGame;

    if (game && func_002A72A0(game) && game->m_d6) {
        if (theNetwork.m_bSessionMaster && m_BombStatus.BombObjectIndex)
            NetFreeObject(m_BombStatus.BombObjectIndex);
        game->m_d6 = 0;
        game->m_d5 = 0;
        m_BombStatus.NewBombState = BOMB_UNKNOWN;
        m_BombStatus.CurrentBombState = BOMB_UNKNOWN;
        m_BombStatus.DefuserObjectIndex = 0;
        m_BombStatus.HolderObjectIndex = 0;
        m_BombStatus.BombObjectIndex = 0;
        if (game->m_pBombValve)
            SetValveValue(game->m_pBombValve, 0);
        if (game->m_pBombOwnerValve)
            SetValveValue(game->m_pBombOwnerValve, 0);
    }
}
