/* fts_net_grenade.cpp -- the grenades' network objects: CZGrenadeState (SOCOM 1's
 * fts_net_grenade.cpp: a thrown grenade's position, velocity, state and thrower, created and
 * updated by the thrower's client, flown by everyone else's). OVL1 0x2C1420-0x2C196F, after
 * fts_net_game.cpp and before fts_net_bomb.cpp. SOCOM II updates the three fields one by one
 * (SOCOM 1's update callback made one UpdateGrenade call). Defined in reverse of their image
 * order (-inline deferred). */

#include "../zmath/zmath.h"

extern "C" {
void *memcpy(void *dst, const void *src, unsigned int n);
void *memset(void *dst, int c, unsigned int n);
}

/* a network object as CZNetwork's table hands it out (its kind a letter, 'K' a grenade's) */
struct NetObject {
    unsigned char _0[0xc];
    char m_type;   // 0xc
    unsigned char _d[0x1c - 0xd];
    void *m_data;   // 0x1c
};

/* the DME's ownership and lifespan of an object (their values are not in SOCOM 1's DWARF, only
 * their names in zNetCreateObject's) */
enum NetOwnershipStatus {};
enum NetObjectLifespan {};

class CZNetwork {
public:
    void zNetRegisterRemoteObjectCallback(int index, void (*creation)(int, int),
                                          void (*update)(int, int, int),
                                          void (*postupdate)(int, int, int),
                                          void (*deletion)(int, int));
    int zNetCreateObject(int *index, NetOwnershipStatus own, NetObjectLifespan life, unsigned int a,
                         int b, unsigned int c, int d, void *e, char *name, int structure);
    int zNetGetMyGameClientIndex();
    NetObject *zNetGetObject(int index);

    unsigned char m_bSessionMaster;   // 0x0
    unsigned char m_bNetwork;   // 0x1
};

extern CZNetwork theNetwork;

/* a projectile (SOCOM 1's CZProjectile, at SOCOM II's offsets: only what this unit touches) */
class CZProjectile {
public:
    unsigned char _0[8];
    unsigned char m_grenadestate;   // 0x8 (a GRENADE_STATE)
    unsigned char _9[0x48 - 0x9];
    CPnt3D m_framestartpos;   // 0x48 (SOCOM 1's at 0x38)
    unsigned char _54[0x60 - 0x54];
    CPnt3D m_vel;   // 0x60 (SOCOM 1's at 0x50)
    unsigned char _6c[0xac - 0x6c];
    int m_shooterID;   // 0xac (SOCOM 1's at 0x90)
    int m_object_index;   // 0xb0 (SOCOM 1's at 0x94)
};

class CZProjectileList {
public:
    void NetRemoveProjectile(CZProjectile *proj);
};

extern "C" {
extern CZProjectileList *m_pProjectilelist__8CZWeapon;   /* CZWeapon::m_pProjectilelist */

void func_003C70E0(CZProjectile *proj, const CPnt3D *pos);   /* (SOCOM II's: the position) */
void func_003C7060(CZProjectile *proj, const CPnt3D *vel);   /* (the velocity) */
void func_003C6F60(CZProjectile *proj, int state);   /* (the grenade's state: an int, its byte loaded after the projectile) */
}

/* the kit and the SEAL (only the kit's grenade) */
class CZKit {
public:
    CZProjectile *FireGrenade(const CPnt3D *pos, const CPnt3D *vel);
};

class CZSealBody {
public:
    unsigned char _0[0x5e0];
    unsigned char m_kit[4];   // 0x5e0: the kit (a CZKit)
};

class CZSealState {
public:
    static CZSealBody *GetBodyFromObjectIndex(int index);
};

/* the grenade's network data: 36 bytes */
struct TypeGrenadeObject {
    CPnt3D pos;   // 0x0
    CPnt3D vel;   // 0xc
    short shooter;   // 0x18: the thrower's network object
    unsigned char state;   // 0x1a
    CZProjectile *proj;   // 0x1c: the projectile flying it here
    CZSealBody *body;   // 0x20: its thrower's SEAL
};

class CZGrenadeState {
public:
    static void UpdateLocalGrenade(CZProjectile *proj);
    static int CreateLocalGrenade(CZProjectile *proj);
    static void RemoteGrenadeDeletionCallback(int client, int index);
    static void RemoteGrenadePostUpdateCallback(int client, int index, int field);
    static void RemoteGrenadeUpdateCallback(int client, int index, int field);
    static void RemoteGrenadeCreationCallback(int client, int index);
    static void zNetRegisterGrenade();

    static int m_NetStructIndexGrenadeObject;
};

extern "C" {
/* the four fields' ids */
extern int data_00441358;
extern int data_00441360;
extern int data_00441368;
extern int data_00441370;

void NetRegisterObjectStart();   /* (the DME's) */
void NetObjectField(int *field, int offset, int a, int b, int c, void *type);
void NetRegisterStructure(int *index, const char *name, int size);
void *func_002BD060();   /* (fts_remote.cpp's broadcast schedules) */
void *func_002BD020();
void *func_002BD0A0();
void *func_002BD120();
void NetFreeObject(int index);   /* (the DME's) */
}

/* the kind registered: its four fields, their offsets taken off a local (OVL1 0x2C1850) */
void CZGrenadeState::zNetRegisterGrenade()
{
    TypeGrenadeObject g;
    TypeGrenadeObject *p = &g;

    NetRegisterObjectStart();
    NetObjectField(&data_00441358, (int)&p->pos - (int)p, 12, 13, 1, func_002BD060());
    NetObjectField(&data_00441360, (int)&p->vel - (int)p, 12, 13, 1, func_002BD020());
    NetObjectField(&data_00441368, (int)&p->state - (int)p, 1, 1, 1, func_002BD0A0());
    NetObjectField(&data_00441370, (int)&p->shooter - (int)p, 2, 3, 1, func_002BD120());
    NetRegisterStructure(&m_NetStructIndexGrenadeObject, "GrenadeState", sizeof(TypeGrenadeObject));
    theNetwork.zNetRegisterRemoteObjectCallback(m_NetStructIndexGrenadeObject,
                                                RemoteGrenadeCreationCallback,
                                                RemoteGrenadeUpdateCallback,
                                                RemoteGrenadePostUpdateCallback,
                                                RemoteGrenadeDeletionCallback);
}

/* a remote grenade appears: the thrower's kit throws one here (OVL1 0x2C17B0) */
void CZGrenadeState::RemoteGrenadeCreationCallback(int client, int index)
{
    NetObject *obj = theNetwork.zNetGetObject(index);

    if (obj) {
        TypeGrenadeObject *data = (TypeGrenadeObject *)obj->m_data;

        if (data) {
            data->body = 0;
            data->proj = 0;
            CZSealBody *body = CZSealState::GetBodyFromObjectIndex((unsigned short)data->shooter);

            if (body) {
                CPnt3D pos(data->pos.x, data->pos.y, data->pos.z);
                CPnt3D vel(data->vel.x, data->vel.y, data->vel.z);

                data->body = body;
                data->proj = ((CZKit *)body->m_kit)->FireGrenade(&pos, &vel);
            }
        }
    }
}

/* a remote grenade's fields updated (OVL1 0x2C16A0) */
void CZGrenadeState::RemoteGrenadeUpdateCallback(int client, int index, int field)
{
    NetObject *obj = theNetwork.zNetGetObject(index);

    if (obj) {
        TypeGrenadeObject *data = (TypeGrenadeObject *)obj->m_data;

        if (data && data->proj && data->body && data->proj) {
            if (field == data_00441358 || field == -1) {
                CPnt3D pos(data->pos.x, data->pos.y, data->pos.z);

                func_003C70E0(data->proj, &pos);
            }
            if (field == data_00441360 || field == -1) {
                CPnt3D vel(data->vel.x, data->vel.y, data->vel.z);

                func_003C7060(data->proj, &vel);
            }
            if (field == data_00441368 || field == -1)
                func_003C6F60(data->proj, data->state);
        }
    }
}

/* nothing after an update (OVL1 0x2C1670: its data's load kept, as in fts_net_blather.cpp) */
void CZGrenadeState::RemoteGrenadePostUpdateCallback(int client, int index, int field)
{
    NetObject *obj = theNetwork.zNetGetObject(index);

    if (obj)
        void *data = *(void *volatile *)&obj->m_data;
}

/* a remote grenade gone: its projectile taken out of the list (OVL1 0x2C15E0) */
void CZGrenadeState::RemoteGrenadeDeletionCallback(int client, int index)
{
    NetObject *obj = theNetwork.zNetGetObject(index);

    if (obj->m_type == 'K') {
        TypeGrenadeObject *data = (TypeGrenadeObject *)obj->m_data;

        if (data && client != theNetwork.zNetGetMyGameClientIndex() && data->proj &&
            m_pProjectilelist__8CZWeapon)
            m_pProjectilelist__8CZWeapon->NetRemoveProjectile(data->proj);
    }
}

/* our grenade's network object made (OVL1 0x2C1510) */
int CZGrenadeState::CreateLocalGrenade(CZProjectile *proj)
{
    int index;

    if (theNetwork.zNetCreateObject(&index, (NetOwnershipStatus)1, (NetObjectLifespan)2, 0, 0, 0,
                                    0, 0, "KProjectileObject", m_NetStructIndexGrenadeObject))
        return -1;
    NetObject *obj = theNetwork.zNetGetObject(index);

    if (!obj)
        return -1;
    memset(obj->m_data, 0, sizeof(TypeGrenadeObject));
    ((TypeGrenadeObject *)obj->m_data)->proj = proj;
    return index;
}

/* our grenade's fields sent, or its object freed once it went off (OVL1 0x2C1420: the index an
 * unsigned local -- an int one trades s0 and s1 with the projectile, as SOCOM 1's has them -- and
 * the state the larger of the two, the projectile's read first) */
void CZGrenadeState::UpdateLocalGrenade(CZProjectile *proj)
{
    if (theNetwork.m_bNetwork) {
        unsigned int index = proj->m_object_index;
        NetObject *obj = theNetwork.zNetGetObject(index);

        if (obj) {
            TypeGrenadeObject *data = (TypeGrenadeObject *)obj->m_data;
            TypeGrenadeObject g;

            g.pos = proj->m_framestartpos;
            g.vel = proj->m_vel;
            g.state = proj->m_grenadestate > data->state ? proj->m_grenadestate : data->state;
            g.shooter = proj->m_shooterID;
            if (proj->m_grenadestate == 3)
                NetFreeObject(index);
            else {
                g.proj = data->proj;
                g.body = data->body;
                memcpy(data, &g, sizeof(TypeGrenadeObject));
            }
        }
    }
}
