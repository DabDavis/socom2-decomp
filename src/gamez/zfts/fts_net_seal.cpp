/* fts_net_seal.cpp -- the SEALs' network objects: CZSealState (SOCOM 1's fts_net_seal.cpp: the
 * network objects of kind "SealObject", their creation, updates and deletion). OVL1 from 0x2BD380,
 * after fts_remote.cpp, to the static initialiser 0x400650's registered destructor (0x2BFFC0): the
 * remote guy list's vector instances (insert 0x2BF980, its temporary's deleter destructor 0x2BFDD0,
 * reserve 0x2BFE40, the list's destructor 0x2BFFC0) close it. UpdateLocalSeal (0x2BD640) byte-exact
 * since round 48, IncrementalUpdateRemoteSeal (0x2BE150) since round 51; not yet the tail. Defined in
 * reverse of their image order (-inline deferred). */

class CZSealBody;

#include <vector>

#include "../zanim/zanim.h"
#include "../znet/znet.h"

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
        union {
            int m_0;   // 0x0
            float m_x;   // 0x0: the SEAL's position (UpdateLocalSeal's)
        };
        union {
            int m_4;   // 0x4
            float m_y;
        };
        union {
            int m_8;   // 0x8
            float m_z;
        };
        unsigned int m_c;   // 0xc: flags handed to CreateRemoteSeal (SOCOM 1's byte at 0x78)
        int m_10;   // 0x10
        char m_14[0x34 - 0x14];   // 0x14
        unsigned char m_weapons[0x64 - 0x34];   // 0x34: the packed weapon data
        unsigned short m_64;   // 0x64
        unsigned short m_66;   // 0x66
        short m_68;   // 0x68
        short m_6a;   // 0x6a
        short m_6c;   // 0x6c
        short m_6e;   // 0x6e
        short m_70;   // 0x70
        short m_72;   // 0x72
        unsigned short m_unit_id;   // 0x74
        unsigned short m_76;   // 0x76
        unsigned short m_chartype;   // 0x78: a custom character type's index, 0 a persona's SEAL (SOCOM 1's byte at 0x7a)
        signed char m_7a;   // 0x7a
        signed char m_7b;   // 0x7b
        signed char m_7c;   // 0x7c
        signed char m_7d;   // 0x7d
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

/* the SEALs' units (zseal.h's CSealUnit: SOCOM 1's Create, SealJoinUnit and EscorteeJoinUnit are
 * still placeholders, called below by address) */
class CSealUnit {
public:
    static CSealUnit *GetUnitByTeam(unsigned int team);
    /* (OVL2; SOCOM 1's at the same calls) */
    static CSealUnit *Create(unsigned int team);   /* 0x5EF380 */
    void SealJoinUnit(CZSealBody *body);   /* 0x5EE250 */
    void EscorteeJoinUnit(CZSealBody *body);   /* 0x5EE160 */

    unsigned char _0[0x58];
    int m_58;   // 0x58: its number (17 the escortees'; UpdateLocalSeal's)
};

/* a character type (zchar's CCharacterType: its name, its base name and the packed weapons) */
struct CCharacterWeapPacked;
class CCharacterType {
public:
    const char *GetBaseName();   /* 0x53B420 */
    static void KorusGetPackedWeapons(CCharacterWeapPacked *packed);   /* 0x53EE10 */

    char *m_name;   // 0x0
};

/* the kit (zkit's CZKit: only the current item and fire mode) */
enum ITEM_ENUM {
    ITEM_ENUM_0 = 0,
    ITEM_ENUM_255 = 255
};

/* the kit's item and fire-mode ids, a SEAL's weapon state and death kind (byte enums; only their
 * names: the setters' mangled names carry them) */
enum EQUIP_ITEM {
    EQUIP_ITEM_0 = 0,
    EQUIP_ITEM_255 = 255
};
enum FIREMODE {
    FIREMODE_0 = 0,
    FIREMODE_255 = 255
};
enum WEAPON_STATE {
    WEAPON_STATE_0 = 0,
    WEAPON_STATE_255 = 255
};
enum SEAL_DEATH_TYPE {
    SEAL_DEATH_0 = 0,
    SEAL_DEATH_255 = 255
};

class CZKit {
public:
    void *GetCurItem() const;   /* 0x5C3780 */
    ITEM_ENUM GetCurItemEnum() const;   /* 0x5C89D0 */
    signed char GetCurFireMode();   /* 0x5C3790 */
    void SetCurFireMode(FIREMODE mode);   /* 0x5C37B0 */
    bool SetCurItemEnum(EQUIP_ITEM item);   /* 0x5C8A20 */
    unsigned int GetNumItems() const;   /* 0x5BE030 */
    void SetItem(unsigned int index, EQUIP_ITEM item);   /* 0x5BDFB0 */
};

/* a node (zdb::CNode: only the direction the network packet takes its heading from) */
class CSealNode {
public:
    unsigned char _0[0x20];
    CPnt3D m_20;   // 0x20
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
    /* a control's value, -1 to 1, on its axis (IncrementalUpdateRemoteSeal's three; the name ours) */
    virtual void SetControl(float value, int axis);
    virtual void Slot24();
    virtual void Slot28();
    virtual void Slot2C();
    virtual void Slot30();
    virtual bool Slot34();
    virtual void Slot38();
    virtual void Slot3C();
    virtual void Slot40();
    virtual void Slot44();
    virtual void Slot48();
    virtual void Slot4C();
    virtual void Slot50();
    virtual void Slot54();
    virtual void Slot58();
    virtual void Slot5C();
    virtual void Slot60();
    virtual void Slot64();
    virtual void Slot68();
    virtual int Slot6C();   // (the network packet's word at 0x10)
    virtual void Slot70(int word);   // (the packet's word at 0x10 taken up)

    unsigned char _4[0x8 - 0x4];
    float m_8;   // 0x8: three of the controls, -1 to 1 (the packet's bytes at 0x7a-0x7c)
    float m_c;   // 0xc
    float m_10;   // 0x10
    unsigned char _14[0x10c - 0x14];
    CSealUnit *m_unit;   // 0x10c
    unsigned char _110[0x170 - 0x110];
    unsigned char m_170_0 : 2;
    unsigned char m_170_2 : 1;   // 0x170, bit 2 (the packet's byte at 0x81)
    unsigned char m_170_3 : 5;
    unsigned char _171[0x1fc - 0x171];
    CPnt3D m_1fc;   // 0x1fc: the position (the two members the body's slot +0x3C takes)
    unsigned char _208[0x210 - 0x208];
    CQuat m_210;   // 0x210: the heading
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
    virtual void Slot40();
    virtual void Slot44();
    virtual void Slot48();
    virtual void Slot4C();
    virtual void Slot50();
    virtual void Slot54();
    virtual void Slot58();
    virtual void Slot5C();
    virtual void Slot60();
    virtual void Slot64();
    virtual void Slot68();
    virtual void Slot6C();
    virtual void Slot70();
    virtual void Slot74();
    virtual void Slot78();
    virtual void Slot7C();
    virtual void Slot80();
    virtual void Slot84();
    virtual void Slot88();
    virtual void Slot8C();
    virtual void Slot90(int kind);   /* (8 when a remote SEAL's death is taken up) */

    CCharacterType *GetCharacter();   /* OVL2 0x599810 */
    signed char GetWeaponState() const;   /* OVL2 0x5E0010 */
    void SetWeaponState(WEAPON_STATE state);   /* OVL2 0x5DFE30 */
    void SetNetLife(unsigned char life);   /* OVL2 0x544210 */
    void SetHealth(float health, SEAL_DEATH_TYPE type);   /* OVL2 0x5477A0 */

    int m_4;   // 0x4: its network object (SOCOM 1's m_NetObjectIndex)
    unsigned char _8[0xc - 0x8];
    int m_c;   // 0xc: CEntity's m_vehicle_index (the packet's character type)
    unsigned char _10[0x1c - 0x10];
    float m_posW[3];   // 0x1c: CEntity's position
    CSealNode *m_node;   // 0x28: CEntity's node
    unsigned char _2c[0xc0 - 0x2c];
    CSealCtrl *m_ctrl;   // 0xc0
    unsigned char _c4[4];
    unsigned int m_c8;   // 0xc8: CEntity's flags (0x20000 an escortee: SOCOM 1's at 0xc4)
    unsigned char _cc[Z_SEALBODY_OFS(0x220) - 0xcc];
    unsigned char m_220;   // 0x220: the kills the network last gave
    unsigned char _221[Z_SEALBODY_OFS(0x224) - Z_SEALBODY_OFS(0x221)];
    int m_224;   // 0x224: a count the kills move, kept within 1 to 10
    float m_228;   // 0x228: taken by the network packet and cleared
    unsigned char _22c[Z_SEALBODY_OFS(0x374) - Z_SEALBODY_OFS(0x22c)];
    signed char m_desiredState;   // 0x374 (zseal.h's)
    signed char m_desiredStance;   // 0x375
    unsigned char _376[Z_SEALBODY_OFS(0x550) - Z_SEALBODY_OFS(0x376)];
    unsigned short m_550;   // 0x550
    unsigned char _552[Z_SEALBODY_OFS(0x556) - Z_SEALBODY_OFS(0x552)];
    unsigned short m_556;   // 0x556
    unsigned char _558[Z_SEALBODY_OFS(0x580) - Z_SEALBODY_OFS(0x558)];
    short m_580;   // 0x580
    unsigned char _582[Z_SEALBODY_OFS(0x598) - Z_SEALBODY_OFS(0x582)];
    unsigned short m_598;   // 0x598
    unsigned char _59a[Z_SEALBODY_OFS(0x59e) - Z_SEALBODY_OFS(0x59a)];
    unsigned short m_59e;   // 0x59e
    unsigned char _5a0[Z_SEALBODY_OFS(0x5c8) - Z_SEALBODY_OFS(0x5a0)];
    short m_5c8;   // 0x5c8
    unsigned char _5ca[Z_SEALBODY_OFS(0x5e0) - Z_SEALBODY_OFS(0x5ca)];
    CZKit m_kit;   // 0x5e0
    unsigned char _5e1[Z_SEALBODY_OFS(0xf7a) - Z_SEALBODY_OFS(0x5e1)];
    signed char m_netlife;   // 0xf7a (zseal.h's)
    unsigned char _f7b[Z_SEALBODY_OFS(0xfbc) - Z_SEALBODY_OFS(0xf7b)];
    int m_lastkiller_id;   // 0xfbc (zseal.h's)
    int m_fc0;   // 0xfc0: the death's kind (zseal.h's)
    unsigned char _fc4[Z_SEALBODY_OFS(0xfc8) - Z_SEALBODY_OFS(0xfc4)];
    int m_fc8;   // 0xfc8: the client that owns it, -1 a remote's (SOCOM 1's at 0xeb8)
    unsigned char m_fcc;   // 0xfcc: cleared once its remote object is made (SOCOM 1's at 0xec0)
    unsigned char _fcd[Z_SEALBODY_OFS(0xfcf) - Z_SEALBODY_OFS(0xfcd)];
    unsigned char m_fcf;   // 0xfcf (SOCOM 1's at 0xec7)
    unsigned char _fd0;
    unsigned char m_fd1;   // 0xfd1
    unsigned char _fd2[2];
    int m_fd4;   // 0xfd4: a network time the lobby is told at (SOCOM II's; the names ours)
    float m_fd8;   // 0xfd8
    int m_fdc;   // 0xfdc: ten seconds past its creation (SOCOM 1's at 0xec8)
    unsigned char _fe0[Z_SEALBODY_OFS(0xffc) - Z_SEALBODY_OFS(0xfe0)];
    float m_ffc[6];   // 0xffc: six values run down (zseal.h's)
    float m_1014[6];   // 0x1014: and their full values
    unsigned char _102c[Z_SEALBODY_OFS(0x1044) - Z_SEALBODY_OFS(0x102c)];
    float m_1044;   // 0x1044
    unsigned char _1048[Z_SEALBODY_OFS(0x105d) - Z_SEALBODY_OFS(0x1048)];
    unsigned char _105d_0 : 6;
    unsigned char m_joined : 1;   // 0x105d, bit 6: set when it joins its unit (SOCOM 1's at 0xf59; the name ours)
    unsigned char _105e[Z_SEALBODY_OFS(0x144c) - Z_SEALBODY_OFS(0x105e)];
    CPnt3D m_aim_goal;   // 0x144c (zseal.h's)
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

/* the personas (fts_net_persona.cpp's: only a client's persona's name) */
class CZPersonaState {
public:
    static const char *GetPersonaName(int client);   /* 0x2C3A70 */
};

/* a game client as the network keeps it (only its object: 0x1000 until its SEAL's is known) */
struct GameClient {
    unsigned char _0[4];
    unsigned short m_object;   // 0x4
};


/* CZSealBody::CreateRemoteSeal (OVL2 0x599690) under the map's name, which carries SOCOM 1's
 * signature: SOCOM II's sixth parameter is a word of flags (the body tests it against 0x10020000;
 * the caller loads it with lw), not SOCOM 1's unsigned char -- the row carries SOCOM II's
 * signature now (the SEAL lane's round 6) */
extern "C" CZSealBody *CreateRemoteSeal__10CZSealBodyFiiPCciPcUiPUc(int client, int index, const char *name,
                                                                    int chartype, char *a, unsigned int flags,
                                                                    unsigned char *weapons);

extern "C" {
int NetGetObject(NetObject **obj, int index);   /* the DME's (prebuilt C) */
int strcmp(const char *a, const char *b);
void *memset(void *dst, int c, unsigned int n);
int func_002C3B10(int client);   /* (fts_net_persona.cpp's: a client's persona's word at 4) */
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

/* what the remote update calls: the entity's team (ent_main.cpp's), the actions' network sync
 * (fts_action.cpp's), the body's value, word and short setters (OVL2's), the fields' counters (one
 * word per field, 0x441138 on, in the fields' order) */
class CEntity {
public:
    void SetTeamMask(unsigned int mask);   /* 0x29ED10 */
};

class CNodeAction {
public:
    static void SyncActionsMP(int client);   /* 0x2B4440 */
};

/* the controller's word as flags (only its bit 30: the SEAL has died) */
struct NetCtrlFlags {
    unsigned char _0[3];
    unsigned char _3_0 : 6;
    unsigned char m_dead : 1;
    unsigned char _3_7 : 1;
};

extern "C" {
void func_005A5790(CZSealBody *body, float value, int which);
void func_00547AA0(CZSealBody *body, unsigned short value, int arg);
void func_00547A90(CZSealBody *body, unsigned char value);
void func_00544D60(CZSealBody *body, int arg);
void *memcpy(void *dst, const void *src, unsigned int n);
extern int data_00441138;
extern int data_00441140;
extern int data_00441148;
extern int data_00441150;
extern int data_00441158;
extern int data_00441160;
extern int data_00441168;
extern int data_00441170;
extern int data_00441178;
extern int data_00441180;
extern int data_00441188;
extern int data_00441190;
extern int data_00441198;
extern int data_004411A0;
extern int data_004411A8;
extern int data_004411B0;
extern int data_004411B8;
extern int data_004411C0;
extern int data_004411C8;
extern int data_004411D0;
extern int data_004411D8;
extern int data_004411E0;
extern int data_004411E8;
extern int data_004411F0;
extern int data_004411F8;
extern int data_00441200;
extern int data_00441208;
extern int data_00441210;
extern int data_00441218;
extern int data_00441220;
extern int data_00441228;
extern int data_00441230;
extern int data_00441238;
extern int data_00441240;
extern int data_00441248;
extern int data_00441250;
}

/* a remote SEAL's network fields taken up, each that changed (or all, at -1), and counted: its
 * position, axis by axis, and heading on its controller; the controller's three axes; the weapon
 * state, the desired state and stance; six of the body's values; its life, its two words; the fire
 * mode and the item (put in the kit first when it has none); the team; the controller's word, a
 * death it marks taken up while the body still has health (the session master syncing the
 * client's actions); a value saturated; six of the body's shorts; and the kills, the change added
 * to the count, kept within 1 to 10. Four fields are only counted (SOCOM 1's, grown and counted;
 * OVL1 0x2BE150). Byte-exact (the states' round 51, unwritten before) as first written but for five
 * spellings, each measured: the item an int with `(EQUIP_ITEM)item` written in the test and the
 * call (the shared conversion is a temporary made after GetCurItemEnum: the target's sixth saved
 * register, `move s0,s1`); the packet's six shorts signed (lh; a (short) cast at the copy changes
 * nothing); the kills compared `body->m_220 != data->m_8b` (the right operand loaded first) with
 * the store inside the if; the heading's constant written out (1 / 10430.06f folds a unit high);
 * and the flags and the saturated value declared at the top, the controller's word in its block
 * (the frame's three words in that order: declared in the blocks, the word sat above the flags) */
void CZSealState::IncrementalUpdateRemoteSeal(int client, int index, int field)
{
    NetObject *obj = theNetwork.zNetGetObject(index);

    if (!obj)
        return;
    TypeSealState *data = (TypeSealState *)obj->m_data;

    if (!data)
        return;
    CZSealBody *body = data->m_body;
    NetCtrlFlags flags;
    float v;

    if (!body) {
        RemoteSealCreationCallback(client, index);
        return;
    }
    if (field == data_00441018 || field == -1) {
        CPnt3D p = body->m_ctrl->m_1fc;

        p.x = data->m_x;
        body->m_ctrl->m_1fc = p;
        data_00441138++;
    }
    if (field == data_00441020 || field == -1) {
        CPnt3D p = body->m_ctrl->m_1fc;

        p.y = data->m_y;
        body->m_ctrl->m_1fc = p;
        data_00441140++;
    }
    if (field == data_00441028 || field == -1) {
        CPnt3D p = body->m_ctrl->m_1fc;

        p.z = data->m_z;
        body->m_ctrl->m_1fc = p;
        data_00441148++;
    }
    if (field == data_00441030 || field == -1) {
        CQuat q;

        q.MakeYRot((short)data->m_64 * 9.5876698e-05f);
        body->m_ctrl->m_210 = q;
        data_00441150++;
    }
    if (field == data_00441038 || field == -1) {
        body->m_ctrl->SetControl(data->m_7a * (1.0f / 127.0f), 0);
        data_00441158++;
    }
    if (field == data_00441040 || field == -1) {
        body->m_ctrl->SetControl(data->m_7b * (1.0f / 127.0f), 2);
        data_00441160++;
    }
    if (field == data_00441048 || field == -1) {
        body->m_ctrl->SetControl(data->m_7c * (1.0f / 127.0f), 1);
        data_00441168++;
    }
    if (field == data_00441050 || field == -1) {
        body->SetWeaponState((WEAPON_STATE)data->m_7e);
        data_00441170++;
    }
    if (field == data_00441058 || field == -1) {
        body->m_desiredState = data->m_7f;
        data_00441178++;
    }
    if (field == data_00441060 || field == -1) {
        body->m_desiredStance = data->m_80;
        data_00441180++;
    }
    if (field == data_00441068 || field == -1)
        data_00441188++;
    if (field == data_00441070 || field == -1) {
        func_005A5790(body, (unsigned int)data->m_82 * (1.0f / 255.0f), 0);
        data_00441190++;
    }
    if (field == data_00441078 || field == -1) {
        func_005A5790(body, (unsigned int)data->m_83 * (1.0f / 255.0f), 3);
        data_00441198++;
    }
    if (field == data_00441080 || field == -1) {
        func_005A5790(body, (unsigned int)data->m_84 * (1.0f / 255.0f), 5);
        data_004411A0++;
    }
    if (field == data_00441088 || field == -1) {
        func_005A5790(body, (unsigned int)data->m_85 * (1.0f / 255.0f), 4);
        data_004411A8++;
    }
    if (field == data_00441090 || field == -1) {
        func_005A5790(body, (unsigned int)data->m_86 * (1.0f / 255.0f), 2);
        data_004411B0++;
    }
    if (field == data_00441098 || field == -1) {
        func_005A5790(body, (unsigned int)data->m_87 * (1.0f / 255.0f), 1);
        data_004411B8++;
    }
    if (field == data_004410A0 || field == -1) {
        body->SetNetLife(data->m_88);
        data_004411C0++;
    }
    if (field == data_004410A8 || field == -1) {
        func_00547AA0(body, data->m_76, 0);
        data_004411C8++;
    }
    if (field == data_004410B0 || field == -1) {
        func_00547A90(body, data->m_89);
        data_004411D0++;
    }
    if (field == data_004410B8 || field == -1) {
        unsigned char mode = data->m_8a;

        if (mode != (unsigned char)body->m_kit.GetCurFireMode())
            body->m_kit.SetCurFireMode((FIREMODE)mode);
        data_004411D8++;
    }
    if (field == data_004410C0 || field == -1) {
        int item = data->m_66;

        if ((EQUIP_ITEM)item != body->m_kit.GetCurItemEnum()) {
            if (!body->m_kit.SetCurItemEnum((EQUIP_ITEM)item)) {
                body->m_kit.SetItem(body->m_kit.GetNumItems(), (EQUIP_ITEM)data->m_66);
                body->m_kit.SetCurItemEnum((EQUIP_ITEM)data->m_66);
            }
        }
        data_004411E0++;
    }
    if (field == data_004410C8 || field == -1) {
        ((CEntity *)body)->SetTeamMask(data->m_c);
        data_004411E8++;
    }
    if (field == data_004410D0 || field == -1) {
        body->m_ctrl->Slot70(data->m_10);
        int word = body->m_ctrl->Slot6C();

        memcpy(&flags, &word, 4);
        if (flags.m_dead && body->m_1044 > 0.0f) {
            body->SetHealth(0.0f, (SEAL_DEATH_TYPE)0);
            body->Slot90(8);
            if (theNetwork.m_bSessionMaster)
                CNodeAction::SyncActionsMP(client);
        }
        data_004411F0++;
    }
    if (field == data_004410D8 || field == -1)
        data_004411F8++;
    if (field == data_004410F0 || field == -1)
        data_00441210++;
    if (field == data_004410F8 || field == -1)
        data_00441218++;
    if (field == data_00441100 || field == -1)
        data_00441220++;
    if (field == data_004410E0 || field == -1) {
        v = data->m_7d * (1.0f / 128.0f);

        _saturate(&v, -1.0f, 1.0f);
        body->m_228 = v;
        data_00441200++;
    }
    if (field == data_00441108 || field == -1) {
        body->m_59e = data->m_68;
        data_00441228++;
    }
    if (field == data_00441110 || field == -1) {
        body->m_598 = data->m_6a;
        data_00441230++;
    }
    if (field == data_00441118 || field == -1) {
        body->m_5c8 = data->m_6c;
        func_00544D60(body, 0);
        data_00441238++;
    }
    if (field == data_00441120 || field == -1) {
        body->m_556 = data->m_6e;
        data_00441240++;
    }
    if (field == data_00441128 || field == -1) {
        body->m_550 = data->m_70;
        data_00441248++;
    }
    if (field == data_00441130 || field == -1) {
        body->m_580 = data->m_72;
        data_00441250++;
    }
    if (field == data_004410E8 || field == -1) {
        if (body->m_220 != data->m_8b) {
            body->m_224 += data->m_8b - body->m_220;
            if (body->m_224 > 10)
                body->m_224 = 10;
            else if (body->m_224 <= 0)
                body->m_224 = 1;
            body->m_220 = data->m_8b;
        }
        data_00441208++;
    }
}

/* after a remote SEAL's update: the lobby told once, in play, when its time comes due, and the SEAL
 * put in its unit once its controller is ready (OVL1 0x2BE020; SOCOM 1's had only the unit).
 * The time declared first, uninitialised, fixes the data's and the body's registers; the client
 * through a `const` copy declared right after it (round 34, the engine core's round-7 rule: a const
 * local survives copy propagation, where a plain copy -- round 13's -- folds into the parameter).
 * Read off MW's colouring: the client (26 neighbours), the time (25), the data and the body (29)
 * all sit at or over K = 25 until the first sweep has pushed the temporaries, then go in one sweep
 * in index order, so the later-numbered is coloured first -- the copy, numbered after the time,
 * takes s2 and the time s3, as the target's */
void CZSealState::RemoteSealPostUpdateCallback(int client, int index, int field)
{
    int t;
    const int c = client;
    NetObject *obj = theNetwork.zNetGetObject(index);

    if (!obj)
        return;
    TypeSealState *data = (TypeSealState *)obj->m_data;
    CZSealBody *body;

    if (!(data && (body = data->m_body)))
        return;
    t = body->m_fd4;

    if (t && theMission.m_pNetGame->m_mp_major_game_state == 3 && !(t < theNetwork.zNetGetTime() + 8000) &&
        body->m_fd8 > 0.0f && !(body->m_1044 < body->m_fd8)) {
        func_002F9690(&theLobby, func_002C3B10(c), 1);
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
            unit = CSealUnit::Create(state->m_unit_id);
        if (cur != unit) {
            unit->SealJoinUnit(body);
            body->m_joined = 1;
        }
    } else if (body->m_c8 & 0x20000) {
        if (state->m_unit_id == 17 || state->m_unit_id == body->m_4 + 32) {
            CSealUnit *cur = body->m_ctrl->m_unit;
            CSealUnit *unit = CSealUnit::GetUnitByTeam(17);

            if (cur != unit)
                unit->SealJoinUnit(body);
        } else {
            CZSealBody *player = ftsGetPlayer();

            if (state->m_unit_id == player->m_4 + 32) {
                CSealUnit *cur = body->m_ctrl->m_unit;
                CSealUnit *unit = CSealUnit::GetUnitByTeam(0);

                if (cur != unit)
                    unit->EscorteeJoinUnit(body);
            } else {
                CSealUnit *cur = body->m_ctrl->m_unit;
                CSealUnit *unit = CSealUnit::GetUnitByTeam(state->m_unit_id);

                if (!unit)
                    unit = CSealUnit::Create(state->m_unit_id);
                if (cur != unit)
                    unit->EscorteeJoinUnit(body);
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
    ((NetObjectData *)obj->m_data)->m_body = body;
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
    TypeSealState *data = (TypeSealState *)obj->m_data;

    if (!theMission.m_180c) {
        TypeRemoteGuy guy;

        guy.client = client;
        guy.index = index;
        m_RemoteGuyList.push_back(guy);
        data->m_body = 0;
        return;
    }
    const char *name = CZPersonaState::GetPersonaName(client);

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
        name = CZPersonaState::GetPersonaName(client);
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
            body->Slot3C(&ctrl->m_1fc, &ctrl->m_210);
        GameClient *gc = theNetwork.zNetGetGameClient(client);

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
    NetObjectData *data = (NetObjectData *)obj->m_data;

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

extern "C" {
bool func_002C2FD0(CZSealBody *body);   /* (fts_net_persona.cpp's: a SEAL not the network's to send) */
void func_005A1FD0(CZSealBody *body, float *out, int arg);   /* (OVL2: a SEAL's four values out) */
float atan2f(float y, float x);
float ceilf(float x);
char *strcpy(char *dst, const char *src);
void *memcpy(void *dst, const void *src, unsigned int n);
}

/* a local SEAL's state put in its network object (SOCOM 1's CZSealState::UpdateLocalSeal, SOCOM II's
 * with a second argument that sends it even when func_002C2FD0 says no; OVL1 0x2BD640): online only,
 * a packet zeroed and filled -- the character's name (a custom type's or an escortee's own, else its
 * base name), the flags, the controller's word, the unit's number (17 the escortees', an escortee's
 * own unit or the player's, else its object's), the position, the heading as a 16-bit angle, three
 * controls as bytes, the weapon state, the desired state and stance, the controller's flag, six
 * values as fractions of their full ones (bytes, rounded up), the network life, the killer, the
 * death's kind (saturated to a byte), the stats, the character type, the vertical aim, the current
 * item and fire mode, the packed weapons and the body -- then copied into the object. Byte-exact
 * since the game states' round 48 (written in round 47, 2 off): the escortee's unit number falling
 * back on the player's is `id == 0 ? UnitNumber(ftsGetPlayer()) : id` -- the inline's return keeps
 * the then-arm's branch to the join (`b; addiu a0,v0,32`) where `if (!id) id = ...;` falls through
 * onto an alignment nop and `!id ? ... : id` / `id ? id : ...` sink the store into both arms
 * (0x75C). The rest: the entry test one `||` expression with a `return` (the shared `b end` block),
 * the heading converted through `short` (an unsigned short field converts unsigned), the killer
 * stored chained (`p.m_76 = kind = ...`: no reload of the address-taken int), the vertical aim
 * through a float local stored after the clear, the clamps' locals declared ahead in their slots'
 * order */
static inline int UnitNumber(CZSealBody *seal)
{
    return seal->m_4 + 32;
}

extern "C" void func_002BD640(CZSealBody *body, int send)
{
    if (!theNetwork.m_bNetwork || func_002C2FD0(body) && !send)
        return;
    NetObjectData p = {0};
    const char *name = body->GetCharacter()->m_name;

    if (body->m_c8 & 0x10000000 || body->m_c8 & 0x20000)
        strcpy(p.m_14, name);
    else
        strcpy(p.m_14, body->GetCharacter()->GetBaseName());
    p.m_c = body->m_c8;
    p.m_10 = body->m_ctrl->Slot6C();
    CSealCtrl *ctrl = body->m_ctrl;

    if (ctrl->m_unit) {
        int id = ctrl->m_unit->m_58;

        if (id == 17) {
            p.m_unit_id = 17;
        } else if (body->m_c8 & 0x20000) {
            id = id == 0 ? UnitNumber(ftsGetPlayer()) : id;
            p.m_unit_id = id;
        } else {
            p.m_unit_id = UnitNumber(body);
        }
    }
    int kind;
    float c0;
    float c1;
    float c2;
    float f0;
    float f3;
    float f5;
    float f4;
    float f2;
    float f1;

    p.m_x = body->m_posW[0];
    p.m_y = body->m_posW[1];
    p.m_z = body->m_posW[2];
    CPnt3D dir = body->m_node->m_20;

    p.m_64 = (short)(10430.06f * atan2f(dir.x, dir.z));
    c0 = ctrl->m_8;
    _saturate(&c0, -1.0f, 1.0f);
    p.m_7a = 127.0f * c0;
    c1 = ctrl->m_10;
    _saturate(&c1, -1.0f, 1.0f);
    p.m_7b = 127.0f * c1;
    c2 = ctrl->m_c;
    _saturate(&c2, -1.0f, 1.0f);
    p.m_7c = 127.0f * c2;
    p.m_7e = body->GetWeaponState();
    p.m_7f = body->m_desiredState;
    p.m_80 = body->m_desiredStance;
    p.m_81 = body->m_ctrl->m_170_2 != 0;
    f0 = body->m_ffc[0] / body->m_1014[0];
    _saturate(&f0, 0.0f, 1.0f);
    p.m_82 = ceilf(255.0f * f0);
    f3 = body->m_ffc[3] / body->m_1014[3];
    _saturate(&f3, 0.0f, 1.0f);
    p.m_83 = ceilf(255.0f * f3);
    f5 = body->m_ffc[5] / body->m_1014[5];
    _saturate(&f5, 0.0f, 1.0f);
    p.m_84 = ceilf(255.0f * f5);
    f4 = body->m_ffc[4] / body->m_1014[4];
    _saturate(&f4, 0.0f, 1.0f);
    p.m_85 = ceilf(255.0f * f4);
    f2 = body->m_ffc[2] / body->m_1014[2];
    _saturate(&f2, 0.0f, 1.0f);
    p.m_86 = ceilf(255.0f * f2);
    f1 = body->m_ffc[1] / body->m_1014[1];
    _saturate(&f1, 0.0f, 1.0f);
    p.m_87 = ceilf(255.0f * f1);
    p.m_88 = body->m_netlife;
    p.m_76 = kind = body->m_lastkiller_id;
    kind = body->m_fc0;
    _saturate(&kind, 0, 255);
    p.m_89 = kind;
    CPnt3D out;

    func_005A1FD0(body, &out.x, 0);
    p.m_68 = body->m_59e;
    p.m_6a = body->m_598;
    p.m_6c = body->m_5c8;
    p.m_6e = body->m_556;
    p.m_70 = body->m_550;
    p.m_72 = body->m_580;
    p.m_chartype = body->m_c;
    CPnt3D aim = body->m_aim_goal;

    if (body->m_228 == 0.0f)
        body->m_228 = aim.y;
    float vert = 128.0f * body->m_228;

    body->m_228 = 0.0f;
    p.m_7d = vert;
    p.m_8b = body->m_220;
    if (body->m_kit.GetCurItem())
        p.m_66 = (unsigned char)body->m_kit.GetCurItemEnum();
    else
        p.m_66 = 255;
    p.m_8a = body->m_kit.GetCurFireMode();
    CCharacterType::KorusGetPackedWeapons((CCharacterWeapPacked *)p.m_weapons);
    p.m_body = body;
    NetObject *obj = theNetwork.zNetGetObject(body->m_4);

    if (obj)
        memcpy(obj->m_data, &p, sizeof(NetObjectData));
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
    return obj->m_data ? ((NetObjectData *)obj->m_data)->m_body : 0;
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
    return ((NetObjectData *)obj->m_data)->m_weapons;
}
