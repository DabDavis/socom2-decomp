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
    /* the SEAL's network data (only its unit's id: SOCOM 1's byte at 0x78) */
    struct TypeSealState {
        unsigned char _0[0x74];
        unsigned short m_unit_id;   // 0x74
    };

    static void UpdateRollandsUnitStuff(CZSealBody *body, TypeSealState *state);
    static int CreateLocalSeal(CZSealBody *body, bool local);
    static void *GetPackedWeaponData(CZSealBody *body);
    static CZSealBody *GetBodyFromObjectIndex(int index);
    static void RemoteSealDeletionCallback(int client, int index);

    static int m_NetStructIndexSealObject;
};

/* the SEALs' units (zseal.h's CSealUnit: SOCOM 1's Create, SealJoinUnit and EscorteeJoinUnit are
 * still placeholders, called below by address) */
class CSealUnit {
public:
    static CSealUnit *GetUnitByTeam(unsigned int team);
};

/* a SEAL's controller (only its unit: SOCOM 1's at 0xdc) */
struct CSealCtrl {
    unsigned char _0[0x10c];
    CSealUnit *m_unit;   // 0x10c
};

/* the SEAL's body (only what this unit touches) */
class CZSealBody {
public:
    static void Destroy(CZSealBody *body);

    unsigned char _0[4];
    int m_4;   // 0x4: its network object (SOCOM 1's m_NetObjectIndex)
    unsigned char _8[0x1c - 0x8];
    float m_posW[3];   // 0x1c: CEntity's position
    unsigned char _28[0xc0 - 0x28];
    CSealCtrl *m_ctrl;   // 0xc0
    unsigned char _c4[4];
    unsigned int m_c8;   // 0xc8: CEntity's flags (0x20000 an escortee: SOCOM 1's at 0xc4)
    unsigned char _cc[0xfc8 - 0xcc];
    int m_fc8;   // 0xfc8: the client that owns it, -1 a remote's (SOCOM 1's at 0xeb8)
    unsigned char _fcc[0xfcf - 0xfcc];
    unsigned char m_fcf;   // 0xfcf (SOCOM 1's at 0xec7)
    unsigned char _fd0;
    unsigned char m_fd1;   // 0xfd1
    unsigned char _fd2[0xfdc - 0xfd2];
    int m_fdc;   // 0xfdc: ten seconds past its creation (SOCOM 1's at 0xec8)
    unsigned char _fe0[0x105d - 0xfe0];
    unsigned char _105d_0 : 6;
    unsigned char m_joined : 1;   // 0x105d, bit 6: set when it joins its unit (SOCOM 1's at 0xf59; the name ours)
};

CZSealBody *ftsGetPlayer();

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
/* the DME's ownership and lifespan of an object (their values are not in SOCOM 1's DWARF) */
enum NetOwnershipStatus {};
enum NetObjectLifespan {};

class CZNetwork {
public:
    int zNetGetLocalTime();
    int zNetGetMyGameClientIndex();
    int zNetCreateObject(int *index, NetOwnershipStatus own, NetObjectLifespan life, unsigned int a,
                         int b, unsigned int c, int d, void *e, char *name, int structure);
    NetObject *zNetGetObject(int index);
};

extern CZNetwork theNetwork;

extern "C" {
int NetGetObject(NetObject **obj, int index);   /* the DME's (prebuilt C) */
int strcmp(const char *a, const char *b);
void *memset(void *dst, int c, unsigned int n);
/* (OVL2; SOCOM 1's CSealUnit::Create, SealJoinUnit and EscorteeJoinUnit at the same calls) */
CSealUnit *func_005EF380(unsigned int team);
void func_005EE250(CSealUnit *unit, CZSealBody *body);
void func_005EE160(CSealUnit *unit, CZSealBody *body);
bool func_002A72A0(CZNetGame *game);   /* (SOCOM II's: a game type 1 or 2) */
}

/* a remote SEAL put in the unit its data names: a SEAL joins its team's unit (made if missing); an
 * escortee goes with the hostages' unit 17 or the side its id names, or joins its own (OVL1 0x2BDEA0;
 * SOCOM 1's, the unit's id now a halfword). The player is fetched into a local of its own block:
 * compared straight (`id == ftsGetPlayer()->m_4 + 32`) MW keeps the id from the tests before in a
 * saved register across the call, which re-colours every register */
void CZSealState::UpdateRollandsUnitStuff(CZSealBody *body, TypeSealState *state)
{
    if (!(body->m_c8 & 0x20000)) {
        CSealUnit *cur = body->m_ctrl->m_unit;
        CSealUnit *unit = CSealUnit::GetUnitByTeam(state->m_unit_id);

        if (!unit)
            unit = func_005EF380(state->m_unit_id);
        if (cur != unit) {
            func_005EE250(unit, body);
            body->m_joined = 1;
        }
    } else if (body->m_c8 & 0x20000) {
        if (state->m_unit_id == 17 || state->m_unit_id == body->m_4 + 32) {
            CSealUnit *cur = body->m_ctrl->m_unit;
            CSealUnit *unit = CSealUnit::GetUnitByTeam(17);

            if (cur != unit)
                func_005EE250(unit, body);
        } else {
            CZSealBody *player = ftsGetPlayer();

            if (state->m_unit_id == player->m_4 + 32) {
                CSealUnit *cur = body->m_ctrl->m_unit;
                CSealUnit *unit = CSealUnit::GetUnitByTeam(0);

                if (cur != unit)
                    func_005EE160(unit, body);
            } else {
                CSealUnit *cur = body->m_ctrl->m_unit;
                CSealUnit *unit = CSealUnit::GetUnitByTeam(state->m_unit_id);

                if (!unit)
                    unit = func_005EF380(state->m_unit_id);
                if (cur != unit)
                    func_005EE160(unit, body);
            }
        }
    }
}

/* a SEAL's object made: kept for the session (a local SEAL's) or the mission, owned by this
 * client or none, its data cleared but for the body (OVL1 0x2BDDA0; SOCOM 1's, the owner now a
 * client index) */
int CZSealState::CreateLocalSeal(CZSealBody *body, bool local)
{
    int index;

    if (theNetwork.zNetCreateObject(&index, (NetOwnershipStatus)1, local ? (NetObjectLifespan)2 : (NetObjectLifespan)3, 0,
                                    0, 0, 1, 0, "SealObject", m_NetStructIndexSealObject))
        return -1;
    NetObject *obj = theNetwork.zNetGetObject(index);

    if (local)
        body->m_fc8 = theNetwork.zNetGetMyGameClientIndex();
    else
        body->m_fc8 = -1;
    body->m_fdc = theNetwork.zNetGetLocalTime() + 10000;
    memset(obj->m_data, 0, sizeof(NetObjectData));
    obj->m_data->m_body = body;
    return index;
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
