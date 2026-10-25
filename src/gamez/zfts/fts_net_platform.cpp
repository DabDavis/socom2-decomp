/* fts_net_platform.cpp -- the turrets' network objects: CZPlatformState (new in SOCOM II, after
 * the pattern of SOCOM 1's fts_net_*.cpp units: objects of kind "TurretObject", registered as the
 * structure "PlatformState" -- the class and unit names ours, by that structure's name as SOCOM
 * 1's CZSealState registers "SealState"). A turret's aim, ammunition and gunner are sent by the
 * session master's client and played on everyone else's; a turret object that arrives before the
 * mission is ready waits in a list. OVL1 0x2C7910-0x2C875F, after fts_missioncam.cpp's
 * CMissionCam: its static initialiser 0x400770 builds the waiting list (reserving 20) and
 * registers its destructor 0x2C86E0; the list's vector instances (insert 0x2C80A0, the deleter's
 * clear 0x2C84E0 and destructor 0x2C84F0, reserve 0x2C8560, the list's destructor 0x2C86E0) close
 * it, as fts_net_seal.cpp's remote guy list closes that unit. Defined in reverse of their image
 * order (-inline deferred). */

#include <vector>

#include "../zmath/zmath.h"

/* a network object as CZNetwork's table hands it out (its kind a letter, 'T' a turret's) */
struct NetObject {
    unsigned char _0[4];
    int m_owner;   // 0x4: the client controlling it, -1 for none
    unsigned char _8[0xc - 0x8];
    char m_type;   // 0xc
    unsigned char _d[0x1c - 0xd];
    void *m_data;   // 0x1c
};

/* the DME's ownership and lifespan of an object (as fts_net_grenade.cpp has them) */
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
    NetObject *zNetGetObject(int index);
    unsigned int zNetGetTime();
};

extern CZNetwork theNetwork;

/* the mission (only its readiness for remote objects: SOCOM II's byte after the online game) */
class CMission {
public:
    unsigned char _0[0x180c];
    unsigned char m_180c;   // 0x180c: the world loaded, remote objects can be made
};

extern CMission theMission;

namespace zdb {
class CNode;

/* every node in the world (znode.h's: only the index of a node) */
class CNodeUniverse {
public:
    int GetIndex(CNode *node);
};

extern CNodeUniverse *NodeUniverse;
}

class CZSealBody;

/* an entity (zentity.h's CEntity: only what this unit touches) */
class CEntity {
public:
    virtual ~CEntity();
    virtual const CMatrix *GetPosW() const;   // +0xC
    zdb::CNode *GetNode();

    unsigned int m_id;   // 0x4: its network object
};

/* a turret (zveh.h's CTurret: only what this unit touches) */
class CTurret : public CEntity {
public:
    static CTurret *Create(int index);

    unsigned char _8[0x2a8 - 0x8];
    int m_ammo;   // 0x2a8
    unsigned char _2ac[0x3b0 - 0x2ac];
    float m_3b0;   // 0x3b0: the aim's first angle
    float m_3b4;   // 0x3b4: its target, from the network
    float m_3b8;   // 0x3b8: the second angle
    float m_3bc;   // 0x3bc: its target
};

CZSealBody *ftsGetPlayer();

extern "C" {
void *memcpy(void *dst, const void *src, unsigned int n);

void NetRegisterObjectStart();   /* (the DME's) */
void NetObjectField(int *field, int offset, int a, int b, int c, void *type);
void NetRegisterStructure(int *index, const char *name, int size);
void *func_002BD1A0();   /* (fts_remote.cpp's broadcast schedules) */
void *func_002BD0E0();

CZSealBody *func_002BD420(int client);   /* (fts_net_seal.cpp's: a client's SEAL) */
void func_005463C0(CZSealBody *body, CTurret *turret);   /* (OVL2: the SEAL takes the turret) */
void func_00545EE0(CZSealBody *body, CTurret *turret);   /* (OVL2: and leaves it) */
void func_0030D010(CZNetwork *net, int index);   /* (CZNetwork's: the object's fields sent) */
void func_0030D620(CZNetwork *net, int index);
void func_0030D000(CZNetwork *net, int index);

/* the six fields' ids */
extern int data_00441600;
extern int data_00441608;
extern int data_00441610;
extern int data_00441618;
extern int data_00441620;
extern int data_00441628;

extern unsigned int data_004415D8;   /* (when the local turret is next sent) */
}

class CZPlatformState {
public:
    /* a turret object that came before the mission was ready: its client and object. Its empty
     * constructor is what makes MW copy it word by word (lw/sw) in the vector's insert, as the
     * target does: without one the insert's three copy loops go through the FPRs (lwc1/swc1) --
     * the reserve and the push_back compile alike either way */
    struct TypePendingPlatform {
        TypePendingPlatform() {}

        int client;   // 0x0
        int index;   // 0x4
    };

    class CPendingList : public std::vector<TypePendingPlatform> {
    public:
        CPendingList() { reserve(20); }
    };

    /* the turret's network data: 28 bytes */
    struct TypePlatformObject {
        float m_yaw;   // 0x0: the turret's 0x3b0, played into 0x3b4
        float m_pitch;   // 0x4: its 0x3b8, played into 0x3bc
        int m_node;   // 0x8: the turret's node, its index in the universe
        unsigned short m_c;   // 0xc
        unsigned short m_e;   // 0xe
        unsigned short m_ammo;   // 0x10
        CZSealBody *m_body;   // 0x14: the SEAL on it
        CTurret *m_turret;   // 0x18
    };

    static bool CreatePendingPlatforms();
    static void ClearPendingPlatforms();
    static void SendPlatform(CTurret *turret);
    static void UpdateLocalPlatform(CTurret *turret, bool now);
    static int CreateLocalPlatform(CTurret *turret);
    static void RemotePlatformDeletionCallback(int client, int index);
    static void RemotePlatformPostUpdateCallback(int client, int index, int field);
    static void RemotePlatformUpdateCallback(int client, int index, int field);
    static void RemotePlatformCreationCallback(int client, int index);
    static void zNetRegisterPlatform();

    static int m_NetStructIndexPlatformObject;
    static CPendingList m_PendingList;
};

CZPlatformState::CPendingList CZPlatformState::m_PendingList;

/* the kind registered: its six fields, their offsets taken off a local (OVL1 0x2C7F30) */
void CZPlatformState::zNetRegisterPlatform()
{
    TypePlatformObject t;
    TypePlatformObject *p = &t;

    NetRegisterObjectStart();
    NetObjectField(&data_00441600, (int)&p->m_yaw - (int)p, 4, 6, 1, func_002BD1A0());
    NetObjectField(&data_00441608, (int)&p->m_pitch - (int)p, 4, 6, 1, func_002BD1A0());
    NetObjectField(&data_00441610, (int)&p->m_node - (int)p, 4, 4, 1, func_002BD0E0());
    NetObjectField(&data_00441618, (int)&p->m_c - (int)p, 2, 3, 1, func_002BD0E0());
    NetObjectField(&data_00441620, (int)&p->m_e - (int)p, 2, 3, 1, func_002BD0E0());
    NetObjectField(&data_00441628, (int)&p->m_ammo - (int)p, 2, 3, 1, func_002BD0E0());
    NetRegisterStructure(&m_NetStructIndexPlatformObject, "PlatformState", sizeof(TypePlatformObject));
    theNetwork.zNetRegisterRemoteObjectCallback(m_NetStructIndexPlatformObject,
                                                RemotePlatformCreationCallback,
                                                RemotePlatformUpdateCallback,
                                                RemotePlatformPostUpdateCallback,
                                                RemotePlatformDeletionCallback);
}

/* a remote turret appears: before the mission is ready it waits in the list, else the turret on
 * its node is made, and the SEAL its owner's client plays put on it (OVL1 0x2C7E60) */
void CZPlatformState::RemotePlatformCreationCallback(int client, int index)
{
    NetObject *obj = theNetwork.zNetGetObject(index);

    if (obj) {
        TypePlatformObject *data = (TypePlatformObject *)obj->m_data;

        if (data) {
            if (!theMission.m_180c) {
                TypePendingPlatform p;

                p.client = client;
                p.index = index;
                m_PendingList.push_back(p);
                data->m_turret = 0;
            } else {
                data_004415D8 = 0;
                data->m_body = func_002BD420(obj->m_owner);
                data->m_turret = CTurret::Create(data->m_node);
                if (data->m_turret)
                    data->m_turret->m_id = index;
            }
        }
    }
}

/* a remote turret's fields played: its two aim targets and its ammunition, and its gunner (the
 * owner's SEAL taking it, or the last one leaving it) -- the waiting turrets made first (OVL1
 * 0x2C7D00) */
void CZPlatformState::RemotePlatformUpdateCallback(int client, int index, int field)
{
    if (m_PendingList.size() && !CreatePendingPlatforms())
        return;
    NetObject *obj = theNetwork.zNetGetObject(index);

    if (!obj)
        return;
    TypePlatformObject *data = (TypePlatformObject *)obj->m_data;

    if (!data)
        return;
    if (!data->m_turret)
        return;
    if (field == data_00441600 || field == -1)
        data->m_turret->m_3b4 = data->m_yaw;
    if (field == data_00441608 || field == -1)
        data->m_turret->m_3bc = data->m_pitch;
    if (field == data_00441628 || field == -1)
        data->m_turret->m_ammo = data->m_ammo;
    if (field == -2 || field == -1) {
        if (obj->m_owner >= 0) {
            CZSealBody *body = func_002BD420(obj->m_owner);

            if (body) {
                data->m_body = body;
                func_005463C0(body, data->m_turret);
            }
        } else if (obj->m_owner < 0) {
            if (data->m_body) {
                func_00545EE0(data->m_body, 0);
                data->m_body = 0;
            }
        }
    }
}

/* nothing after an update (OVL1 0x2C7CF0) */
void CZPlatformState::RemotePlatformPostUpdateCallback(int, int, int)
{
}

/* nothing when one goes (OVL1 0x2C7CE0) */
void CZPlatformState::RemotePlatformDeletionCallback(int, int)
{
}

/* our turret's network object made: its node's index, its ammunition (OVL1 0x2C7BF0) */
int CZPlatformState::CreateLocalPlatform(CTurret *turret)
{
    int index;

    if (theNetwork.zNetCreateObject(&index, (NetOwnershipStatus)2, (NetObjectLifespan)3, 0, 0, 0,
                                    0, 0, "TurretObject", m_NetStructIndexPlatformObject))
        return -1;
    NetObject *obj = theNetwork.zNetGetObject(index);

    if (!obj)
        return -1;
    TypePlatformObject *data = (TypePlatformObject *)obj->m_data;

    data->m_node = zdb::NodeUniverse->GetIndex(turret->GetNode());
    data->m_yaw = 0.0f;
    data->m_pitch = 0.0f;
    data->m_c = 0;
    data->m_e = 0;
    data->m_ammo = turret->m_ammo;
    data->m_body = 0;
    data->m_turret = turret;
    return index;
}

/* our turret's data, while the player is its gunner (the name ours: the target's three early
 * zeroes are an inline's returns -- the type test's and the null object's straight to the caller's
 * test, the gunner's through a block of its own) */
inline CZPlatformState::TypePlatformObject *GetLocalPlatform(int index)
{
    NetObject *obj = theNetwork.zNetGetObject(index);

    if (!obj)
        return 0;
    if (obj->m_type != 'T')
        return 0;
    CZPlatformState::TypePlatformObject *data = (CZPlatformState::TypePlatformObject *)obj->m_data;

    if (!data || data->m_body != ftsGetPlayer())
        return 0;
    return data;
}

/* our turret's object filled from it, when the player is its gunner: its aim, and its ammunition
 * when sent now, which also flags it changed and sends it -- the turret's angles read off its
 * matrix and not used (OVL1 0x2C7A60) */
void CZPlatformState::UpdateLocalPlatform(CTurret *turret, bool now)
{
    int index = turret->m_id;
    TypePlatformObject *data = GetLocalPlatform(index);

    if (data) {
        CPnt3D euler;

        turret->GetPosW()->ToEuler(&euler);
        TypePlatformObject t = {0};

        t.m_node = data->m_node;
        t.m_yaw = turret->m_3b0;
        t.m_pitch = turret->m_3b8;
        t.m_c = data->m_c;
        t.m_e = data->m_e;
        if (now)
            t.m_ammo = turret->m_ammo;
        else
            t.m_ammo = data->m_ammo;
        t.m_body = data->m_body;
        t.m_turret = data->m_turret;
        memcpy(data, &t, sizeof(TypePlatformObject));
        if (now) {
            func_0030D620(&theNetwork, index);
            func_0030D000(&theNetwork, index);
        }
    }
}

/* our turret sent, at most every two seconds (OVL1 0x2C79F0) */
void CZPlatformState::SendPlatform(CTurret *turret)
{
    if (turret) {
        unsigned int now = theNetwork.zNetGetTime();

        if (now >= data_004415D8) {
            func_0030D010(&theNetwork, turret->m_id);
            data_004415D8 = now + 2000;
        }
    }
}

/* the waiting turrets forgotten (OVL1 0x2C79E0) */
void CZPlatformState::ClearPendingPlatforms()
{
    m_PendingList.clear();
}

/* the waiting turrets made, once the mission is ready (OVL1 0x2C7910) */
#pragma auto_inline off
bool CZPlatformState::CreatePendingPlatforms()
{
    if (!theMission.m_180c)
        return false;
    if (!m_PendingList.size())
        return true;
    for (CPendingList::iterator i = m_PendingList.begin(); i != m_PendingList.end(); ++i)
        RemotePlatformCreationCallback((*i).client, (*i).index);
    m_PendingList.clear();
    return true;
}
#pragma auto_inline reset
