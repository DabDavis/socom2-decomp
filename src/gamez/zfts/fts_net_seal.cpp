/* fts_net_seal.cpp -- the SEALs' network objects: CZSealState (SOCOM 1's fts_net_seal.cpp: the
 * network objects of kind "SealObject", their creation, updates and deletion). OVL1 from 0x2BD380,
 * after fts_remote.cpp, to the static initialiser 0x400650's registered destructor (0x2BFFC0): the
 * remote guy list's vector instances (insert 0x2BF980, its temporary's deleter destructor 0x2BFDD0,
 * reserve 0x2BFE40, the list's destructor 0x2BFFC0) close it. Only some of it so far: not yet
 * UpdateLocalSeal (0x2BD640), IncrementalUpdateRemoteSeal (0x2BE150), zNetRegisterSeal (0x2BF2C0)
 * and the tail. Defined in reverse of their image order (-inline deferred). */

class CZSealBody;

#include <vector>

#include "../zanim/zanim.h"

class CZSealState {
public:
    /* a remote SEAL whose object came before the mission was ready: its client and object (SOCOM
     * 1's list held them newed; SOCOM II's is a vector of them, 8 bytes each -- the name ours).
     * The empty constructor makes MW copy it word by word in the vector's insert (lw/sw, as the
     * target's 0x2BF980; without it the copies go through the FPRs) */
    struct TypeRemoteGuy {
        TypeRemoteGuy() {}

        int client;   // 0x0
        int index;   // 0x4
    };

    /* the list reserves 30 when it is built (the static initialiser 0x400650 inlines this: three
     * words zeroed, then reserve(30) out of line); its implicit destructor (0x2BFFC0) tests `this`
     * four times, the class's own and the vector's three levels */
    class CRemoteGuyList : public std::vector<TypeRemoteGuy> {
    public:
        CRemoteGuyList() { reserve(30); }
    };

    /* the SEAL's network data, 0x90 bytes (SOCOM 1's 0x98): only the packed weapons, the unit's id
     * (SOCOM 1's byte at 0x78) and the body */
    struct TypeSealState {
        int m_0;   // 0x0
        int m_4;   // 0x4
        int m_8;   // 0x8
        unsigned int m_c;   // 0xc: flags handed to CreateRemoteSeal (SOCOM 1's byte at 0x78)
        int m_10;   // 0x10
        char m_14[0x34 - 0x14];   // 0x14
        unsigned char m_weapons[0x64 - 0x34];   // 0x34: the packed weapon data
        unsigned short m_64;   // 0x64
        unsigned short m_66;   // 0x66
        unsigned short m_68;   // 0x68
        unsigned short m_6a;   // 0x6a
        unsigned short m_6c;   // 0x6c
        unsigned short m_6e;   // 0x6e
        unsigned short m_70;   // 0x70
        unsigned short m_72;   // 0x72
        unsigned short m_unit_id;   // 0x74
        unsigned short m_76;   // 0x76
        unsigned short m_chartype;   // 0x78: a custom character type's index, 0 a persona's SEAL (SOCOM 1's byte at 0x7a)
        unsigned char m_7a;   // 0x7a
        unsigned char m_7b;   // 0x7b
        unsigned char m_7c;   // 0x7c
        unsigned char m_7d;   // 0x7d
        unsigned char m_7e;   // 0x7e
        unsigned char m_7f;   // 0x7f
        unsigned char m_80;   // 0x80
        unsigned char m_81;   // 0x81
        unsigned char m_82;   // 0x82
        unsigned char m_83;   // 0x83
        unsigned char m_84;   // 0x84
        unsigned char m_85;   // 0x85
        unsigned char m_86;   // 0x86
        unsigned char m_87;   // 0x87
        unsigned char m_88;   // 0x88
        unsigned char m_89;   // 0x89
        unsigned char m_8a;   // 0x8a
        unsigned char m_8b;   // 0x8b
        CZSealBody *m_body;   // 0x8c (SOCOM 1's at 0x94)
    };

    static void RemoteSealUpdateCallback(int client, int index, int field);
    static void IncrementalUpdateRemoteSeal(int client, int index, int field);
    static void RemoteSealCreationCallback(int client, int index);
    static void RemoteSealPostUpdateCallback(int client, int index, int field);
    static bool UnloadRemoteGuyList();
    static void ClearRemoteGuyList();
    static void UpdateRollandsUnitStuff(CZSealBody *body, TypeSealState *state);
    static int CreateLocalSeal(CZSealBody *body, bool local);
    static void *GetPackedWeaponData(CZSealBody *body);
    static CZSealBody *GetBodyFromObjectIndex(int index);
    static void RemoteSealDeletionCallback(int client, int index);
    static void zNetRegisterSeal();

    static int m_NetStructIndexSealObject;
    static CRemoteGuyList m_RemoteGuyList;
};

typedef CZSealState::TypeSealState NetObjectData;

CZSealState::CRemoteGuyList CZSealState::m_RemoteGuyList;

/* a network object as the DME hands it out: its kind's name, and its data (only what this unit
 * touches) */
struct NetObject {
    unsigned char _0[0xc];
    union {
        char m_name[0x10];   // 0xc: its kind's name,
        char m_type;   //      its first letter ('S' a SEAL)
    };
    NetObjectData *m_data;   // 0x1c
};

/* the SEALs' units (zseal.h's CSealUnit: SOCOM 1's Create, SealJoinUnit and EscorteeJoinUnit are
 * still placeholders, called below by address) */
class CSealUnit {
public:
    static CSealUnit *GetUnitByTeam(unsigned int team);
};

/* a SEAL's controller (only its unit, SOCOM 1's at 0xdc, and the virtual at +0x34 its unit's update
 * waits on) */
class CSealCtrl {
public:
    virtual void Slot08();
    virtual void Slot0C();
    virtual void Slot10();
    virtual void Slot14();
    virtual void Slot18();
    virtual void Slot1C();
    virtual void Slot20();
    virtual void Slot24();
    virtual void Slot28();
    virtual void Slot2C();
    virtual void Slot30();
    virtual bool Slot34();

    unsigned char _4[0x10c - 0x4];
    CSealUnit *m_unit;   // 0x10c
    unsigned char _110[0x1fc - 0x110];
    unsigned char m_1fc[0x210 - 0x1fc];   // 0x1fc (the two members the body's slot +0x3C takes)
    unsigned char m_210[4];   // 0x210
};

/* the SEAL's body (only what this unit touches) */
class CZSealBody {
public:
    static void Destroy(CZSealBody *body);

    /* CEntity's table: only the slot at +0x3C the creation callback calls with two of the
     * controller's members (the names ours) */
    virtual void Slot08();
    virtual void Slot0C();
    virtual void Slot10();
    virtual void Slot14();
    virtual void Slot18();
    virtual void Slot1C();
    virtual void Slot20();
    virtual void Slot24();
    virtual void Slot28();
    virtual void Slot2C();
    virtual void Slot30();
    virtual void Slot34();
    virtual void Slot38();
    virtual void Slot3C(void *a, void *b);

    int m_4;   // 0x4: its network object (SOCOM 1's m_NetObjectIndex)
    unsigned char _8[0x1c - 0x8];
    float m_posW[3];   // 0x1c: CEntity's position
    unsigned char _28[0xc0 - 0x28];
    CSealCtrl *m_ctrl;   // 0xc0
    unsigned char _c4[4];
    unsigned int m_c8;   // 0xc8: CEntity's flags (0x20000 an escortee: SOCOM 1's at 0xc4)
    unsigned char _cc[0xfc8 - 0xcc];
    int m_fc8;   // 0xfc8: the client that owns it, -1 a remote's (SOCOM 1's at 0xeb8)
    unsigned char m_fcc;   // 0xfcc: cleared once its remote object is made (SOCOM 1's at 0xec0)
    unsigned char _fcd[0xfcf - 0xfcd];
    unsigned char m_fcf;   // 0xfcf (SOCOM 1's at 0xec7)
    unsigned char _fd0;
    unsigned char m_fd1;   // 0xfd1
    unsigned char _fd2[2];
    int m_fd4;   // 0xfd4: a network time the lobby is told at (SOCOM II's; the names ours)
    float m_fd8;   // 0xfd8
    int m_fdc;   // 0xfdc: ten seconds past its creation (SOCOM 1's at 0xec8)
    unsigned char _fe0[0x1044 - 0xfe0];
    float m_1044;   // 0x1044
    unsigned char _1048[0x105d - 0x1048];
    unsigned char _105d_0 : 6;
    unsigned char m_joined : 1;   // 0x105d, bit 6: set when it joins its unit (SOCOM 1's at 0xf59; the name ours)
};

CZSealBody *ftsGetPlayer();

/* the online game (only its major state) */
class CZNetGame;

/* the online lobby (its 0x2F9690, which the remote glue calls with a persona too) */
class CZOnlineLobby;

extern CZOnlineLobby theLobby;

/* the online game (only the bomb's last place: SOCOM 1's at 0xcc) */
class CZNetGame {
public:
    unsigned char _0[0x104];
    float m_bomb_pos[3];   // 0x104
    unsigned char m_bomb_pos_valid;   // 0x110
    unsigned char _111[2];
    unsigned char m_mp_major_game_state;   // 0x113
};

/* the mission's custom character types: 0x160-byte records, the name first (the names ours) */
struct CustomCharType {
    char m_name[0x160];
};

struct CustomCharTypeList {
    CustomCharType *Item(unsigned int i) { return i < m_size ? &m_data[i] : 0; }

    unsigned int m_capacity;   // 0x0
    unsigned int m_size;   // 0x4
    CustomCharType *m_data;   // 0x8
};

class CMission {
public:
    void RefreshTeamValves();

    unsigned char _0[0x1808];
    CZNetGame *m_pNetGame;   // 0x1808
    unsigned char m_180c;   // 0x180c: the mission ready for remote SEALs (SOCOM 1's byte at 0x588)
    unsigned char _180d[0x1814 - 0x180d];
    CustomCharTypeList *m_customCharTypes;   // 0x1814
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
    int zNetGetTime();
    int zNetGetLocalTime();
    int zNetGetMyGameClientIndex();
    int zNetCreateObject(int *index, NetOwnershipStatus own, NetObjectLifespan life, unsigned int a,
                         int b, unsigned int c, int d, void *e, char *name, int structure);
    NetObject *zNetGetObject(int index);
    void zNetRegisterRemoteObjectCallback(int index, void (*creation)(int, int),
                                          void (*update)(int, int, int),
                                          void (*postupdate)(int, int, int),
                                          void (*deletion)(int, int));
};

/* a game client as the network keeps it (only its object: 0x1000 until its SEAL's is known) */
struct GameClient {
    unsigned char _0[4];
    unsigned short m_object;   // 0x4
};

extern "C" GameClient *func_0030D5E0(CZNetwork *net, int client);   /* (SOCOM 1's zNetGetGameClient) */

/* CZSealBody::CreateRemoteSeal (OVL2 0x599690) under the map's name, which carries SOCOM 1's
 * signature: SOCOM II's sixth parameter is a word of flags (the body tests it against 0x10020000;
 * the caller loads it with lw), not SOCOM 1's unsigned char -- the row carries SOCOM II's
 * signature now (the SEAL lane's round 6) */
extern "C" CZSealBody *CreateRemoteSeal__10CZSealBodyFiiPCciPcUiPUc(int client, int index, const char *name,
                                                                    int chartype, char *a, unsigned int flags,
                                                                    unsigned char *weapons);

extern CZNetwork theNetwork;

extern "C" {
int NetGetObject(NetObject **obj, int index);   /* the DME's (prebuilt C) */
int strcmp(const char *a, const char *b);
void *memset(void *dst, int c, unsigned int n);
/* (OVL2; SOCOM 1's CSealUnit::Create, SealJoinUnit and EscorteeJoinUnit at the same calls) */
CSealUnit *func_005EF380(unsigned int team);
void func_005EE250(CSealUnit *unit, CZSealBody *body);
void func_005EE160(CSealUnit *unit, CZSealBody *body);
int func_002C3B10(int client);   /* (fts_net_persona.cpp's: a client's persona's word at 4) */
const char *func_002C3A70(int client);   /* (fts_net_persona.cpp's: a client's persona's name) */
int sprintf(char *buf, const char *fmt, ...);
void func_002F9690(CZOnlineLobby *lobby, int a, int mode);
bool func_002A72A0(CZNetGame *game);   /* (SOCOM II's: a game type 1 or 2) */
}

extern "C" {
/* the fields' ids */
extern int data_00441018;
extern int data_00441020;
extern int data_00441028;
extern int data_00441030;
extern int data_00441038;
extern int data_00441040;
extern int data_00441048;
extern int data_00441050;
extern int data_00441058;
extern int data_00441060;
extern int data_00441068;
extern int data_00441070;
extern int data_00441078;
extern int data_00441080;
extern int data_00441088;
extern int data_00441090;
extern int data_00441098;
extern int data_004410A0;
extern int data_004410A8;
extern int data_004410B0;
extern int data_004410B8;
extern int data_004410C0;
extern int data_004410C8;
extern int data_004410D0;
extern int data_004410D8;
extern int data_004410E0;
extern int data_004410E8;
extern int data_004410F0;
extern int data_004410F8;
extern int data_00441100;
extern int data_00441108;
extern int data_00441110;
extern int data_00441118;
extern int data_00441120;
extern int data_00441128;
extern int data_00441130;

void NetRegisterObjectStart();   /* (the DME's) */
void NetObjectField(int *field, int offset, int a, int b, int c, void *type);
void NetRegisterStructure(int *index, const char *name, int size);
void *func_002BD0A0();   /* (fts_remote.cpp's broadcast schedules) */
void *func_002BD0E0();
void *func_002BD120();
void *func_002BD160();
void *func_002BD1E0();
}

/* the kind registered: its fields, their offsets taken off a pointer to a local as
 * fts_net_game.cpp's; the weapons twice (the target computes their offset once for both calls)
 * (OVL1 0x2BF2C0) */
void CZSealState::zNetRegisterSeal()
{
    TypeSealState s;
    TypeSealState *p = &s;

    NetRegisterObjectStart();
    NetObjectField(&data_00441018, (int)&p->m_0 - (int)p, 4, 6, 1, func_002BD1E0());
    NetObjectField(&data_00441020, (int)&p->m_4 - (int)p, 4, 6, 1, func_002BD1E0());
    NetObjectField(&data_00441028, (int)&p->m_8 - (int)p, 4, 6, 1, func_002BD1E0());
    NetObjectField(&data_00441030, (int)&p->m_64 - (int)p, 2, 2, 1, func_002BD160());
    NetObjectField(&data_00441108, (int)&p->m_68 - (int)p, 2, 2, 1, func_002BD0E0());
    NetObjectField(&data_00441110, (int)&p->m_6a - (int)p, 2, 2, 1, func_002BD0E0());
    NetObjectField(&data_00441118, (int)&p->m_6c - (int)p, 2, 2, 1, func_002BD0E0());
    NetObjectField(&data_00441120, (int)&p->m_6e - (int)p, 2, 2, 1, func_002BD0E0());
    NetObjectField(&data_00441128, (int)&p->m_70 - (int)p, 2, 2, 1, func_002BD0E0());
    NetObjectField(&data_00441130, (int)&p->m_72 - (int)p, 2, 2, 1, func_002BD0E0());
    NetObjectField(&data_00441038, (int)&p->m_7a - (int)p, 1, 0, 1, func_002BD160());
    NetObjectField(&data_00441040, (int)&p->m_7b - (int)p, 1, 0, 1, func_002BD160());
    NetObjectField(&data_00441048, (int)&p->m_7c - (int)p, 1, 0, 1, func_002BD160());
    NetObjectField(&data_00441050, (int)&p->m_7e - (int)p, 1, 1, 1, func_002BD160());
    NetObjectField(&data_00441058, (int)&p->m_7f - (int)p, 1, 1, 1, func_002BD160());
    NetObjectField(&data_00441060, (int)&p->m_80 - (int)p, 1, 1, 1, func_002BD160());
    NetObjectField(&data_00441068, (int)&p->m_81 - (int)p, 1, 1, 1, func_002BD160());
    NetObjectField(&data_00441070, (int)&p->m_82 - (int)p, 1, 1, 1, func_002BD120());
    NetObjectField(&data_00441078, (int)&p->m_83 - (int)p, 1, 1, 1, func_002BD120());
    NetObjectField(&data_00441080, (int)&p->m_84 - (int)p, 1, 1, 1, func_002BD120());
    NetObjectField(&data_00441088, (int)&p->m_85 - (int)p, 1, 1, 1, func_002BD120());
    NetObjectField(&data_00441090, (int)&p->m_86 - (int)p, 1, 1, 1, func_002BD120());
    NetObjectField(&data_00441098, (int)&p->m_87 - (int)p, 1, 1, 1, func_002BD120());
    NetObjectField(&data_004410A0, (int)&p->m_88 - (int)p, 1, 1, 1, func_002BD0A0());
    NetObjectField(&data_004410A8, (int)&p->m_76 - (int)p, 2, 3, 1, func_002BD0E0());
    NetObjectField(&data_004410B0, (int)&p->m_89 - (int)p, 1, 1, 1, func_002BD0E0());
    NetObjectField(&data_004410B8, (int)&p->m_8a - (int)p, 1, 1, 1, func_002BD160());
    NetObjectField(&data_004410C0, (int)&p->m_66 - (int)p, 2, 3, 1, func_002BD120());
    NetObjectField(&data_004410D8, (int)&p->m_unit_id - (int)p, 2, 3, 1, func_002BD120());
    NetObjectField(&data_004410C8, (int)&p->m_c - (int)p, 4, 5, 1, func_002BD120());
    NetObjectField(&data_004410D0, (int)&p->m_10 - (int)p, 4, 5, 1, func_002BD120());
    NetObjectField(&data_004410E0, (int)&p->m_7d - (int)p, 1, 0, 1, func_002BD160());
    NetObjectField(&data_004410E8, (int)&p->m_8b - (int)p, 1, 1, 1, func_002BD0A0());
    NetObjectField(&data_004410F0, (int)&p->m_chartype - (int)p, 2, 3, 1, func_002BD120());
    NetObjectField(&data_004410F8, (int)&p->m_14 - (int)p, 1, 1, 32, func_002BD120());
    NetObjectField(&data_00441100, (int)&p->m_weapons - (int)p, 1, 1, 48, func_002BD0A0());
    NetObjectField(&data_00441100, (int)&p->m_weapons - (int)p, 1, 1, 48, func_002BD0A0());
    NetRegisterStructure(&m_NetStructIndexSealObject, "SealState", sizeof(TypeSealState));
    theNetwork.zNetRegisterRemoteObjectCallback(m_NetStructIndexSealObject,
                                                RemoteSealCreationCallback,
                                                RemoteSealUpdateCallback,
                                                RemoteSealPostUpdateCallback,
                                                RemoteSealDeletionCallback);
}

/* after a remote SEAL's update: the lobby told once, in play, when its time comes due, and the SEAL
 * put in its unit once its controller is ready (OVL1 0x2BE020; SOCOM 1's had only the unit).
 * OPEN, 10 instructions, registers only: the client and the hit time trade s2 and s3 (the target's
 * client s2, time s3). The time declared first, uninitialised, fixed the data's and the body's
 * registers (21 -> 10). `!(t - time < 8000)` gives the target's registers exactly but a subu/slti
 * where it has addiu/slt; tried the comparison's operands swapped and reversed, the time
 * unsigned, a local copy of the client (round 13) */
void CZSealState::RemoteSealPostUpdateCallback(int client, int index, int field)
{
    int t;
    NetObject *obj = theNetwork.zNetGetObject(index);

    if (!obj)
        return;
    TypeSealState *data = obj->m_data;
    CZSealBody *body;

    if (!(data && (body = data->m_body)))
        return;
    t = body->m_fd4;

    if (t && theMission.m_pNetGame->m_mp_major_game_state == 3 && !(t < theNetwork.zNetGetTime() + 8000) &&
        body->m_fd8 > 0.0f && !(body->m_1044 < body->m_fd8)) {
        func_002F9690(&theLobby, func_002C3B10(client), 1);
        body->m_fd8 = 1.0f;
        body->m_fd4 = 0;
    }
    CSealCtrl *ctrl = body->m_ctrl;
    bool ready = ctrl ? ctrl->Slot34() : false;

    if (ready)
        UpdateRollandsUnitStuff(body, data);
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

/* a remote SEAL's update: taken up once the SEALs waiting on the mission are made (OVL1 0x2BEE10;
 * SOCOM 1's, UnloadRemoteGuyList inlined) */
void CZSealState::RemoteSealUpdateCallback(int client, int index, int field)
{
    if (!m_RemoteGuyList.size() || UnloadRemoteGuyList())
        IncrementalUpdateRemoteSeal(client, index, field);
}

/* a remote SEAL's object appears: kept on the list until the mission is ready; then its SEAL made,
 * under its custom character type (the type's alive valve set, no owner) or its persona's name, its
 * fields and unit taken up, the team valves refreshed and its client told its object (OVL1
 * 0x2BF070; SOCOM 1's, the custom types and the body's slot added) */
void CZSealState::RemoteSealCreationCallback(int client, int index)
{
    NetObject *obj = theNetwork.zNetGetObject(index);

    if (!obj)
        return;
    TypeSealState *data = obj->m_data;

    if (!theMission.m_180c) {
        TypeRemoteGuy guy;

        guy.client = client;
        guy.index = index;
        m_RemoteGuyList.push_back(guy);
        data->m_body = 0;
        return;
    }
    const char *name = func_002C3A70(client);

    if (!data->m_chartype && !name) {
        data->m_body = 0;
        return;
    }
    int owner = client;

    if (data->m_chartype) {
        char buf[100];

        name = theMission.m_customCharTypes->Item(data->m_chartype)->m_name;
        owner = -1;
        sprintf(buf, "%s_valve_alive", name);
        CValve *valve = CValve::Create(buf, 0, (VALVE_TYPE)2);

        if (valve)
            valve->SetValue(1);
    } else
        name = func_002C3A70(client);
    data->m_body = CreateRemoteSeal__10CZSealBodyFiiPCciPcUiPUc(owner, index, name, data->m_chartype, data->m_14,
                                                                data->m_c, data->m_weapons);
    if (data->m_body) {
        data->m_body->m_fcc = 0;
        IncrementalUpdateRemoteSeal(client, index, -1);
        RemoteSealPostUpdateCallback(client, index, -1);
        theMission.RefreshTeamValves();
        CZSealBody *body = data->m_body;
        CSealCtrl *ctrl = body->m_ctrl;

        if (ctrl)
            body->Slot3C(ctrl->m_1fc, ctrl->m_210);
        GameClient *gc = func_0030D5E0(&theNetwork, client);

        if (gc && gc->m_object == 0x1000)
            gc->m_object = index;
    }
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

/* the waiting remote SEALs forgotten (OVL1 0x2BD630; CLoadState::Init calls it -- SOCOM II's, the
 * name ours) */
void CZSealState::ClearRemoteGuyList()
{
    m_RemoteGuyList.clear();
}

/* the remote SEALs whose objects came before the mission was ready made now, once it is, and the
 * list emptied (OVL1 0x2BD560; SOCOM 1's, called twice by CLoadState::Tick as SOCOM 1's is) */
bool CZSealState::UnloadRemoteGuyList()
{
    if (!theMission.m_180c)
        return false;
    if (!m_RemoteGuyList.size())
        return true;
    for (CRemoteGuyList::iterator i = m_RemoteGuyList.begin(); i != m_RemoteGuyList.end(); ++i)
        RemoteSealCreationCallback((*i).client, (*i).index);
    m_RemoteGuyList.clear();
    return true;
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
