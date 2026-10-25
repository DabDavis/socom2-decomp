/* fts_net_seal.cpp -- the SEALs' network objects: CZSealState (SOCOM 1's fts_net_seal.cpp: the
 * network objects of kind "SealObject", their creation, updates and deletion). OVL1 from 0x2BD380,
 * after fts_remote.cpp, to the static initialiser 0x400650's registered destructor (0x2BFFC0).
 * Only some of it so far. Defined in reverse of their image order (-inline deferred). */

class CZSealBody;

/* a network object as the DME hands it out: its kind's name, and its data (only what this unit
 * touches) */
struct NetObjectData {
    unsigned char _0[0x34];
    unsigned char m_weapons[0x8c - 0x34];   // 0x34: the packed weapon data
    CZSealBody *m_body;   // 0x8c (SOCOM 1's at 0x94)
};

struct NetObject {
    unsigned char _0[0xc];
    union {
        char m_name[0x10];   // 0xc: its kind's name,
        char m_type;   //      its first letter ('S' a SEAL)
    };
    NetObjectData *m_data;   // 0x1c
};

class CZSealState {
public:
    static void *GetPackedWeaponData(CZSealBody *body);
    static CZSealBody *GetBodyFromObjectIndex(int index);
    static void RemoteSealDeletionCallback(int client, int index);
};

/* the SEAL's body (only what this unit touches) */
class CZSealBody {
public:
    static void Destroy(CZSealBody *body);

    unsigned char _0[4];
    int m_4;   // 0x4: its network object (SOCOM 1's m_NetObjectIndex)
    unsigned char _8[0x1c - 0x8];
    float m_posW[3];   // 0x1c: CEntity's position
    unsigned char _28[0xfcf - 0x28];
    unsigned char m_fcf;   // 0xfcf (SOCOM 1's at 0xec7)
    unsigned char _fd0;
    unsigned char m_fd1;   // 0xfd1
};

/* the online game (only the bomb's last place: SOCOM 1's at 0xcc) */
class CZNetGame {
public:
    unsigned char _0[0x104];
    float m_bomb_pos[3];   // 0x104
    unsigned char m_bomb_pos_valid;   // 0x110
};

class CMission {
public:
    unsigned char _0[0x1808];
    CZNetGame *m_pNetGame;   // 0x1808
};

extern CMission theMission;

/* the bomb (SOCOM 1's CZBombState::m_BombStatus: its state, 2 carried, and who carries it) */
struct BombStatus {
    unsigned char m_state;   // 0x0
    unsigned char _1[0x14 - 0x1];
    int m_carrier;   // 0x14: the carrier's network object
    unsigned char _18[0x20 - 0x18];
};

class CZBombState {
public:
    static BombStatus m_BombStatus;
};

/* the network (only its calls here; theNetwork at 0x45A0C0, fts_remote.cpp's head says why) */
class CZNetwork {
public:
    int zNetGetMyGameClientIndex();
    NetObject *zNetGetObject(int index);
};

extern CZNetwork theNetwork;

extern "C" {
int NetGetObject(NetObject **obj, int index);   /* the DME's (prebuilt C) */
int strcmp(const char *a, const char *b);
bool func_002A72A0(CZNetGame *game);   /* (SOCOM II's: a game type 1 or 2) */
}

/* a remote SEAL's object deleted: the bomb's place kept if it carried it, and its body destroyed
 * unless we deleted it ourselves (OVL1 0x2BEF60; SOCOM 1's has the same shape). The bomb's status
 * is taken into a reference local after the game-type test: the target reads the state absolutely
 * and the carrier through the struct's address, made in a1 during the state test (lui a1; addiu a1
 * in the branch's delay slot; lw v1,20(a1)) -- read through the global both are absolute, through
 * a reference declared before the call the address lives in a saved register. */
void CZSealState::RemoteSealDeletionCallback(int client, int index)
{
    if (client == theNetwork.zNetGetMyGameClientIndex())
        return;
    NetObject *obj = theNetwork.zNetGetObject(index);

    if (!obj)
        return;
    if (obj->m_type != 'S')
        return;
    NetObjectData *data = obj->m_data;

    if (!data)
        return;
    CZSealBody *body = data->m_body;

    if (!body)
        return;
    CZNetGame *game = theMission.m_pNetGame;

    if (func_002A72A0(game)) {
        BombStatus &status = CZBombState::m_BombStatus;

        if (status.m_state == 2 && status.m_carrier == index) {
            game->m_bomb_pos[0] = body->m_posW[0];
            game->m_bomb_pos[1] = body->m_posW[1];
            game->m_bomb_pos[2] = body->m_posW[2];
            game->m_bomb_pos_valid = 1;
        }
    }
    body->m_fcf = 0;
    body->m_fd1 = 1;
    if (client >= 0)
        CZSealBody::Destroy(body);
    data->m_body = 0;
}

/* the SEAL a network object carries, if it is a SEAL's (OVL1 0x2BD4D0) */
CZSealBody *CZSealState::GetBodyFromObjectIndex(int index)
{
    NetObject *obj = 0;
    int error = NetGetObject(&obj, index);

    if (!obj || error)
        return 0;
    if (strcmp(obj->m_name, "SealObject"))
        return 0;
    return obj->m_data ? obj->m_data->m_body : 0;
}

/* a SEAL's packed weapon data, out of its network object (OVL1 0x2BD3D0: the index read before the
 * object pointer is cleared, as the target schedules it) */
void *CZSealState::GetPackedWeaponData(CZSealBody *body)
{
    int index = body->m_4;
    NetObject *obj = 0;
    int error = NetGetObject(&obj, index);

    if (!obj || error)
        return 0;
    return obj->m_data->m_weapons;
}
