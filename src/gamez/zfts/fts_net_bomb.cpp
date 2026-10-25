/* fts_net_bomb.cpp -- the bomb's network object: CZBombState (SOCOM 1's fts_net_bomb.cpp: the
 * demolition game's one bomb, its state machine and its object, which the session master
 * owns). OVL1 0x2C1970-0x2C2A5F, after fts_net_grenade.cpp and before the blather state's unit.
 * Only some of it so far. Defined in reverse of their image order (-inline deferred). */

#include "../zanim/zanim.h"

/* a network object as CZNetwork's table hands it out (only its data) */
struct NetObject {
    unsigned char _0[0x1c];
    void *m_data;   // 0x1c
};

class CZNetwork {
public:
    NetObject *zNetGetObject(int index);
};

extern CZNetwork theNetwork;

/* the online game (only its bomb valves and its bomb flag at 0xd6) */
class CZNetGame {
public:
    unsigned char _0[0x18];
    CValve *m_pBombValve;   // 0x18: "mp_bomb"
    CValve *m_pBombOwnerValve;   // 0x1c: "bomb_owner"
    unsigned char _20[0xd6 - 0x20];
    unsigned char m_d6;   // 0xd6: the bomb's object known
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

    static void CreateBombHere(CPnt3D *pos);
    static void ChangeBombState();
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

/* a valve's change of state, its callbacks told (zanim.h's CValve has neither the enum nor the
 * setter, so they are spelled here as fts_mission.cpp spells them) */
/* VALVE_STATE and CValve::MakeCallbacks are zanim.h's now; the call still goes by its
 * placeholder */
extern "C" void func_003519F0(CValve *valve, VALVE_STATE state);   /* CValve::MakeCallbacks */

/* CValve::SetValue, inlined in both builds */
inline void SetValveValue(CValve *valve, int value)
{
    VALVE_STATE state = VALVE_SAME;

    if (valve->m_value != value)
        state = value == 0 ? VALVE_OFF : VALVE_ON;
    valve->m_value = value;
    if (state != VALVE_SAME)
        func_003519F0(valve, state);
}

void SendBombStateRequest(int who, unsigned char state, const CPnt3D *pos);

extern "C" {
/* the four fields' ids */
extern int data_00441380;
extern int data_00441388;
extern int data_00441390;
extern int data_00441398;

void func_00620D28();   /* (the DME's: SOCOM 1's NetRegisterObjectStart) */
void func_00620DB0(int *field, int offset, int a, int b, int c, void *type);   /* (NetObjectField) */
void func_00620E98(int *index, const char *name, int size);   /* (NetRegisterStructure) */
void func_0030CE80(CZNetwork *net, int index, void (*creation)(int, int),
                   void (*update)(int, int, int), void (*postupdate)(int, int, int),
                   void (*deletion)(int, int));   /* (CZNetwork::zNetRegisterRemoteObjectCallback) */
void *func_002BD060();   /* (fts_remote.cpp's broadcast schedules) */
void *func_002BD0A0();
bool func_002A72A0(CZNetGame *game);   /* (SOCOM II's: a game type 1 or 2) */
void func_0059DCE0(int state);
int func_0030D670(CZNetwork *net, int *index, int own, int life, unsigned int a, int b,
                  unsigned int c, int d, void *e, const char *name, int structure);   /* (CZNetwork::zNetCreateObject) */
void *memset(void *dst, int c, unsigned int n);
extern int data_004413A0;   /* (the bomb's time) */
}

/* the kind registered: its four fields, their offsets taken off a pointer to a local, as
 * fts_net_grenade.cpp's (OVL1 0x2C26E0) */
void CZBombState::zNetRegisterBomb()
{
    TypeBombObject b;
    TypeBombObject *p = &b;

    func_00620D28();
    func_00620DB0(&data_00441380, (int)&p->pos - (int)p, 12, 13, 1, func_002BD060());
    func_00620DB0(&data_00441388, (int)&p->state - (int)p, 1, 1, 1, func_002BD0A0());
    func_00620DB0(&data_00441390, (int)&p->holder - (int)p, 2, 3, 1, func_002BD0A0());
    func_00620DB0(&data_00441398, (int)&p->defuser - (int)p, 2, 3, 1, func_002BD0A0());
    func_00620E98(&m_NetStructIndexBombObject, "BombState", sizeof(TypeBombObject));
    func_0030CE80(&theNetwork, m_NetStructIndexBombObject, RemoteBombCreationCallback,
                  RemoteBombUpdateCallback, RemoteBombPostUpdateCallback,
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

/* the bomb's object made by the session master: its status restarted from the state it had, the
 * explosion's anim found, the time a placed bomb has left (OVL1 0x2C2260) */
int CZBombState::CreateLocalBomb()
{
    int index;

    if (func_0030D670(&theNetwork, &index, 1, 2, 0, 0, 0, 0, 0, "BombObject",
                      m_NetStructIndexBombObject))
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

/* the bomb's object gone (OVL1 0x2C23A0; SOCOM 1's did nothing) */
void CZBombState::RemoteBombDeletionCallback(int client, int index)
{
    func_0059DCE0(m_BombStatus.CurrentBombState);
}

/* a new state asked of the session master, in an online game (OVL1 0x2C2090) */
void CZBombState::RequestNewBombState(int who, EnumBombState state, const CPnt3D *pos)
{
    if (theMission.m_pNetGame)
        SendBombStateRequest(who, state, pos);
}

/* the bomb dropped here (OVL1 0x2C1BF0) */
void CZBombState::CreateBombHere(CPnt3D *pos)
{
    m_BombStatus.NewBombState = BOMB_GROUNDED;
    m_BombStatus.BombLocation = *pos;
    m_BombStatus.HolderObjectIndex = 0;
    UpdateLocalBomb();
}
