/* zweapon.h -- the weapons: CZWeapon and CZFTSWeapon, the ammunition (CZAmmo), their lists, the
 * projectiles (CZProjectile, CZProjectileList), the marks and decal sets, the flyouts. SOCOM 1's
 * DWARF has the layouts (its zwep_*.cpp units: zwep_explosive, zwep_global, zwep_projectile,
 * zwep_weapon and zwep_ammo by their static initialisers, zwep_weapon and zwep_ammo by the press
 * beta's asserts); SOCOM II moved most members, so every offset here is SOCOM II's own, read off
 * its code, and only what the matched functions touch is placed. The header's name is ours. */
#ifndef ZWEAPON_H
#define ZWEAPON_H

#include <list>
#include <vector>

#include "../zmath/zmath.h"

/* an item a SEAL carries (SOCOM 1's DWARF: a byte enum, its classes at 0, 30, 50, ... and the
 * values past 127 its heavy weapons, explosives and equipment -- unsigned, with EQUIP_NONE 255:
 * GetWeaponId hands back `li v0,255`, as SOCOM 1's did). Only the bounds are named here */
enum EQUIP_ITEM {
    EQUIP_PISTOL = 0,
    /* the two goggles' ids of their own (GetWeaponId; SOCOM 1's 182 and 184; the names ours) */
    EQUIP_BINOCULARS = 190,
    EQUIP_NVG = 191,
    EQUIP_END = 254,
    EQUIP_NONE = 255
};

/* a weapon's fire modes (SOCOM 1's DWARF: a byte enum) */
enum FIREMODE {
    SAFETY = 0,
    SINGLE = 1,
    BURST = 2,
    AUTOFIRE = 3,
    SPECIAL_MODE = 4,
    NUM_FIREMODES = 5,
    /* the mode in use, as an argument (the kit's code passes 255; the name ours) */
    FIREMODE_CURRENT = 255
};

class CZAnim;
namespace zdb {
class CTexture;
}
class CZWeaponList;
class CZProjectile;
class CZProjectileList;

/* a cartridge type (SOCOM 1's DWARF: a byte enum, AMMO_NONE 255 and AMMO_END 254) */
enum EQUIP_AMMO {
    A_9X19 = 1,
    AMMO_END = 254,
    AMMO_NONE = 255
};

/* a cartridge type's stats (SOCOM 1's DWARF: 0x24 bytes, m_ID at 0x20; SOCOM II's id is at 0x30,
 * the size its reader news 0x34) */
class CZAmmo {
public:
    ~CZAmmo();
    /* the name (zwep_ammo.cpp's, out of line: 0x3D4540) */
    char *GetName();
    /* the id by value (inline: GetAmmo(EQUIP_AMMO) narrows its argument into a register of its
     * own only so; the name is ours) */
    EQUIP_AMMO GetId() const { return m_ID; }

    /* the blast's radius (SOCOM 1's, out of line in zwep_ammo.cpp: 0x3D4500) */
    float GetExplosionRadius();

    char *m_name;   // 0x0
    char *m_description;   // 0x4
    unsigned char _8[0x1c - 0x8];
    float m_explosionRadius;   // 0x1c (the name ours)
    unsigned char _20[0x30 - 0x20];
    EQUIP_AMMO m_ID;   // 0x30
};

/* the ammunition, a pointer list (SOCOM 1's DWARF: CZAmmoList : list); its elements are its own */
class CZAmmoList : public std::list<CZAmmo *> {
public:
    CZAmmoList();
    ~CZAmmoList();
    EQUIP_AMMO GetAmmoId(const char *name);
    CZAmmo *GetAmmo(EQUIP_AMMO id);
    CZAmmo *GetAmmo(int index);
    CZAmmo *GetAmmo(const char *name);
    void AddAmmo(CZAmmo *ammo);
};   // 0xc bytes

/* a weapon (SOCOM 1's DWARF: 0xb0 bytes, the vtable pointer at 0). SOCOM II's is 0xf8 bytes (its
 * constructor clears up to 0xf4) and its table has a second slot (0x3D2D70) */
class CZWeapon {
public:
    CZWeapon();
    virtual ~CZWeapon();
    /* +0x0C, new in SOCOM II (0x3D2D70; not named) */
    virtual int Slot0C();
    /* the item's class (SOCOM 1's two: the static one from the id's range, 0x3D1A60, and the
     * member one handing it the id, 0x3D1E10) */
    static EQUIP_ITEM GetIDType(EQUIP_ITEM id);
    EQUIP_ITEM GetIDType();
    /* the name and the description (zwep_weapon.cpp's, out of line: 0x3D2840, 0x3D2830 -- in
     * SOCOM 1's order, GetDescription before GetName) */
    char *GetName() const;
    char *GetDescription() const;
    /* the animations let go of (SOCOM 1's ClearAnims, 0x3D21C0) */
    void ClearAnims();
    /* the id by value (inline: GetWeapon(EQUIP_ITEM) narrows its argument into a register of its
     * own only so; the name is ours) */
    EQUIP_ITEM GetId() const { return m_ID; }

    /* the name, packed with the ammo into a word for the network (SOCOM 1's statics) */
    static unsigned short PackItemAndAmmoEnums(EQUIP_ITEM item, EQUIP_AMMO ammo);
    static void UnpackItemAndAmmoEnums(unsigned short packed, EQUIP_ITEM *item, EQUIP_AMMO *ammo);
    /* an item's id by name: the two goggles first, then the weapon list (SOCOM 1's static) */
    static EQUIP_ITEM GetWeaponId(const char *name);
    /* a material's hit and special animations, none past the materials' count (SOCOM 1's) */
    CZAnim *GetHitAnim(int material);
    CZAnim *GetSpecialMaterialAnim(int material);
    void SetHasFireMode(FIREMODE mode);
    void SetMaxFireMode(FIREMODE mode);
    bool SetHitAnim(int material, CZAnim *anim);
    bool SetSpecialMaterialAnim(int material, CZAnim *anim);
    static bool IsPrimary(EQUIP_ITEM id);
    bool IsDirectFire();
    EQUIP_AMMO GetDefaultAmmoId();
    CZAmmo *GetDefaultAmmo();

    /* the magazine's size and the magazines carried (SOCOM 1's GetAmmoCapacity and GetNumMags,
     * in its order: 0x3D2810 and 0x3D2800; CZKit's magazine code calls them) */
    int GetNumMags();
    int GetAmmoCapacity();

    char *m_name;   // 0x4
    char *m_description;   // 0x8
    unsigned char _c[0x24 - 0xc];
    FIREMODE m_maxfiremode;   // 0x24 (SOCOM 1: 0x20)
    unsigned char _25[0x28 - 0x25];
    int m_ammocap;   // 0x28 (SOCOM 1: 0x24)
    int m_nummags;   // 0x2c (SOCOM 1: 0x28)
    unsigned char _30[0x7c - 0x30];
    EQUIP_ITEM m_ID;   // 0x7c (SOCOM 1: 0x60)
    unsigned char _7d[0x80 - 0x7d];
    CZAnim **m_hitanim;   // 0x80 (SOCOM 1: 0x64; ClearAnims frees it)
    CZAnim *m_fireanim;   // 0x84
    CZAnim *m_zoomedfireanim;   // 0x88
    CZAnim *m_defaultspecialanim;   // 0x8c
    CZAnim *m_socom2_90;   // 0x90 (SOCOM II's: ClearAnims clears it with the others)
    CZAnim **m_specialmaterialanim;   // 0x94 (SOCOM 1: 0x74)
    unsigned char _98[0xbc - 0x98];
    /* the weapon's icon (the kit lane's: kit.h reads a texture there; the name ours) */
    zdb::CTexture *m_icon;   // 0xbc
    unsigned char _c0[0xc4 - 0xc0];
    CZAmmoList m_LegalAmmoList;   // 0xc4 (SOCOM 1: 0x9c)
    unsigned char m_hasFireModes[5];   // 0xd0 (SOCOM 1: 0xa8)
    unsigned char m_socom2_d5;   // 0xd5
    unsigned char _d6[0xf8 - 0xd6];
    /* the weapon events' callbacks, called and listed (zwep_global.cpp's, SOCOM 1's statics) */
    static void MakeWeaponEventCallbacks(unsigned int event, CZProjectile *proj);
    static void AddWeaponEventCallback(void (*callback)(CZProjectile *, unsigned int, void *),
                                       unsigned int events, void *data);
    static void RemoveWeaponEventCallback(void (*callback)(CZProjectile *, unsigned int, void *));

    /* the weapons (SOCOM 1's static, 12 bytes: 0x4B5210) */
    static CZWeaponList m_listWeapons;
    /* the projectiles in flight (SOCOM 1's static: 0x4B5220) */
    static CZProjectileList *m_pProjectilelist;
};   // 0xf8 bytes (CZFTSWeapon's targetmods start there)

/* the materials' table (int_material.cpp's, SOCOM 1's class): only the extension list, whose size
 * the weapons' per-material animations are bounded by (SOCOM 1's static; 0x44F350 here) */
class CMaterialExArray {
public:
    static std::vector<void *> m_materialex_list;
};

/* a weapon's per-stance reticle and recoil tuning (SOCOM 1's DWARF: 0x4c bytes; SOCOM II's is 0x74,
 * a float inserted at 0x28 and more members after SOCOM 1's -- only what the matched code reads is
 * named, SOCOM 1's names where the order agrees) */
class CTargetMods {
public:
    CTargetMods();
    void SetWiggleReport(float r);
    float GetWiggleReport();
    void SetTarget_DilateUponMovement(float d);

    float m_reticuleknock;   // 0x0
    float m_reticuleknock_return;   // 0x4
    float m_reticuleknock_max;   // 0x8
    float m_distppframe_posx;   // 0xc
    float m_distppframe_posy;   // 0x10
    float m_distlimit_posx;   // 0x14
    float m_distlimit_posy;   // 0x18
    float m_sniper_decayrate;   // 0x1c
    float m_target_dilateuponfire;   // 0x20
    float m_target_dilateuponmovement;   // 0x24
    float m_socom2_28;   // 0x28 (SOCOM II's: the constructor sets 1)
    float m_target_dilateuponmovementSqrt;   // 0x2c (SOCOM 1: 0x28)
    float m_target_constrict;   // 0x30 (SOCOM 1's from here, 4 bytes later)
    float m_target_min;   // 0x34
    float m_target_max;   // 0x38
    float m_firerifle_kickrate;   // 0x3c
    float m_firerifle_kickreturnrate;   // 0x40 (the constructor leaves it)
    float m_firerifle_kickbasedist;   // 0x44
    float m_firerifle_kickrandomdist;   // 0x48
    /* SOCOM II's eight (the constructor's 2, 1, 2, 2000, 1, 1, 2, 250; not named) */
    float m_socom2_4c;   // 0x4c
    float m_socom2_50;   // 0x50
    float m_socom2_54;   // 0x54
    float m_socom2_58;   // 0x58
    float m_socom2_5c;   // 0x5c
    float m_socom2_60;   // 0x60
    float m_socom2_64;   // 0x64
    float m_socom2_68;   // 0x68
    int m_socom2_6c;   // 0x6c (SOCOM II's: the constructor sets 3)
    float m_wiggle_report;   // 0x70 (SOCOM 1: 0x48, its last member as here)
};   // 0x74 bytes

/* what a SEAL's kit holds (SOCOM 1's enum; only its size matters here: CZFTSWeapon stores it as a
 * word) */
enum SEAL_ITEM {
    SEAL_ITEM_FORCE_WORD = 0x10000
};

/* a weapon as the game uses it (SOCOM 1's DWARF: 0x1cc bytes, everything after CZWeapon 0x48 bytes
 * later here, then 0xc0 later from the rumble on; 0x28c bytes) */
class CZFTSWeapon : public CZWeapon {
public:
    CZFTSWeapon();
    /* +0x10, its own (8 bytes, SOCOM 1's) */
    virtual void Recoil();
    float GetZoomRange(unsigned int i);
    CTargetMods *TargetMods(int i);
    float ResolveFTSDamage(float damage);
    bool IsGrenadeLauncherWeapon();
    /* SOCOM 1's (0x3C5D20: CZKit calls it where SOCOM 1's calls IsGrenadeLauncherAmmo) */
    bool IsGrenadeLauncherAmmo();
    /* the grenade launchers or the class 145 (0x3C5D80, new in SOCOM II; the name is ours) */
    bool IsLauncherWeapon();
    bool IsM203Weapon();
    /* the same test of an id (new in SOCOM II, 0x3C5E10; the member one inlines it, its true an
     * assignment there -- daddiu; the name is ours) */
    static bool IsM203Weapon(EQUIP_ITEM id);
    void AddZoommode(float zoom);
    void AccuracyScalar_Max(float s);
    void AccuracyScalar_Min(float s);
    void AccuracyBurstCnt_Max(int n);
    void AccuracyBurstCnt_Min(int n);
    int AccuracyBurstCnt_Max() const;
    int AccuracyBurstCnt_Min() const;

    CTargetMods targetmods[3];   // 0xf8 (SOCOM 1: 0xb0)
    unsigned char m_rumblehigh_enable;   // 0x254
    float m_rumblehigh_time;   // 0x258
    unsigned char m_rumblelow_enable;   // 0x25c
    float m_rumblelow_time;   // 0x260
    float m_rumblelow_power;   // 0x264
    int m_accuracy_burstcnt_min;   // 0x268
    int m_accuracy_burstcnt_max;   // 0x26c
    float m_accuracy_scalar_min;   // 0x270
    float m_accuracy_scalar_max;   // 0x274
    float m_accuracy_slope;   // 0x278
    SEAL_ITEM m_type;   // 0x27c
    std::vector<float> m_zoommode;   // 0x280
};   // 0x28c bytes

namespace zdb {
class CNode;
class DiIntersect;
}

/* a projectile's state (SOCOM 1's DWARF: a byte enum) */
enum PROJECTILE_STATE {
    PROJECTILE_EXPIRED = 0,
    PROJECTILE_FLYOUT = 1,
    PROJECTILE_AT_REST = 2,
    PROJECTILE_TO_BE_DETONATED = 3,
    PROJECTILE_DETONATION_TO_BE_HANDLED = 4,
    PROJECTILE_WAS_DETONATED = 5,
    PROJECTILE_TO_BE_REMOVED = 6
};

/* a projectile (SOCOM 1's DWARF: 0xa8 bytes, the vtable pointer at 0; SOCOM II's is 0xc4 -- only
 * what the matched code reads is placed: the instance 8 bytes on, the shooter's id 0x1c on). Its
 * table is SOCOM 1's two slots, PreTick and PostTick (__vt__12CZProjectile, 0x4083D0): no virtual
 * destructor, as in SOCOM 1 */
class CZProjectile {
public:
    /* (0x3CB8A0: the table stored first, its node extension built at 0x18) */
    CZProjectile();
    ~CZProjectile();
    virtual bool PreTick(float dT);    /* +0x08 */
    virtual bool PostTick(float dT);   /* +0x0C */
    /* the node it flies (SOCOM 1's, out of line in zwep_projectile.cpp: 0x3C7AE0) */
    zdb::CNode *GetInstance();
    /* the state set, unless the game holds it (forced, or a player's or a person-hit projectile
     * passes; SOCOM 1's, 0x3C6F00) */
    void SetProjectileState(PROJECTILE_STATE state, bool force);

    /* SOCOM 1's flags at 0x4 (m_done .. m_echo, bits 0-6); SOCOM II's two at bits 17 and 18 (the
     * explosive's PreTick tests them; the names ours) */
    unsigned int m_done : 1;   // 0x4
    unsigned int m_isfirsthit : 1;
    unsigned int m_isplayer : 1;
    unsigned int m_useflyout : 1;
    unsigned int m_lastHitWasPerson : 1;
    unsigned int m_lastHitWasVolumetric : 1;
    unsigned int m_echo : 1;
    unsigned int _b7 : 10;
    unsigned int m_b17 : 1;
    unsigned int m_b18 : 1;
    unsigned int _b19 : 13;
    unsigned char m_grenadestate;   // 0x8
    /* 1: direct fire (its intersection released to the direct-fire table: SOCOM 1's
     * PROJECTILE_TYPE, a byte) */
    unsigned char m_shottype;   // 0x9
    PROJECTILE_STATE m_state;   // 0xa
    unsigned char _b[0x10 - 0xb];
    CZAmmo *m_ammo;   // 0x10
    unsigned char _14[0x20 - 0x14];
    zdb::CNode *m_instance;   // 0x20 (SOCOM 1: 0x18)
    unsigned char _24[0x28 - 0x24];
    /* its intersection and the table's handle (SOCOM 1: 0x20, 0x24), and a second, always
     * direct-fire (SOCOM II's; the names ours) */
    zdb::DiIntersect *m_di;   // 0x28
    int m_di_handle;   // 0x2c
    zdb::DiIntersect *m_di2;   // 0x30
    int m_di2_handle;   // 0x34
    unsigned char _38[0x48 - 0x38];
    /* where it is (SOCOM 1's m_framestartpos, 0x38: the explosive's proximity is registered
     * there) */
    CPnt3D m_framestartpos;   // 0x48
    unsigned char _54[0x8c - 0x54];
    /* a time, "never" at first (PlacedExplosive's constructor sets 9999999; the name ours) */
    float m_8c;   // 0x8c
    unsigned char _90[0xa8 - 0x90];
    /* the world's handle of its proximity registration (SOCOM 1's m_intersecthandle, 0x8c) */
    int m_intersecthandle;   // 0xa8
    int m_shooterID;   // 0xac (SOCOM 1: 0x90)
    unsigned char _b0[0xc4 - 0xb0];
};   // 0xc4 bytes

/* a charge set down (SOCOM 1's DWARF: CZProjectile and two flags, m_detonate and m_remote; SOCOM
 * II's flags at 0xc4 and 0xc5, a third flag, two words and a matrix after them -- 0x110 bytes; its
 * accessors out of line in zwep_explosive.cpp) */
class PlacedExplosive : public CZProjectile {
public:
    PlacedExplosive();
    ~PlacedExplosive();
    virtual bool PreTick(float dT);
    virtual bool PostTick(float dT);
    /* set off from afar (SOCOM 1's) */
    void RemoteDetonate();
    bool IsRemote();
    void SetRemote(bool remote);
    /* SOCOM II's setters (0x3C51A0, 0x3C51D0, 0x3C5200; the names ours): the third flag, the word
     * at 0xc8 with the matrix (the identity when given none), the word at 0xcc */
    void SetFlag(unsigned char flag);
    void SetMatrix(int n, const CMatrix *m);
    void SetWord(int n);

    /* bools (SOCOM 1's DWARF writes them as unsigned char: IsRemote returns the byte as is) */
    bool m_detonate;   // 0xc4
    bool m_remote;   // 0xc5
    unsigned char m_flag;   // 0xc6
    int m_c8;   // 0xc8
    int m_cc;   // 0xcc
    CMatrix m_matrix;   // 0xd0
};   // 0x110 bytes

/* the projectiles in flight and the intersections they test with (SOCOM 1's DWARF: 0x208 bytes;
 * the direct- and indirect-fire tables sit where SOCOM 1's do -- its Release*FireDI are
 * byte-identical) */
class CZProjectileList {
public:
    bool GetNextIndirectFireDI(zdb::DiIntersect **di, int *handle);
    void ReleaseIndirectFireDI(zdb::DiIntersect **di, int *handle);
    bool GetNextDirectFireDI(zdb::DiIntersect **di, int *handle);
    void ReleaseDirectFireDI(zdb::DiIntersect **di, int *handle);
    void NetRemoveProjectile(CZProjectile *proj);
    int GetNumRemoteExplosives(int shooter);

    CZProjectile *m_projectiles;   // 0x0
    std::vector<PlacedExplosive *> m_explosives;   // 0x4
    unsigned char *m_used;   // 0x10
    int m_firstfree;   // 0x14
    int m_lastused;   // 0x18
    int m_size;   // 0x1c
    zdb::DiIntersect *m_directFireDI[60];   // 0x20
    unsigned char m_directFireDIUsed[60];   // 0x110
    int m_direct_firstfree;   // 0x14c
    zdb::DiIntersect *m_indirectFireDI[30];   // 0x150
    unsigned char m_indirectFireDIUsed[60];   // 0x1c8 (60, as SOCOM 1's, for 30 intersections)
    int m_indirect_firstfree;   // 0x204
};   // 0x208 bytes

/* the weapons, a pointer list (SOCOM 1's, out of line in a unit of its own); its elements are its
 * own */
class CZWeaponList : public std::list<CZWeapon *> {
public:
    CZWeaponList();
    ~CZWeaponList();
    void ClearAnimations();
    void InitializeWeapons();
    /* a weapon's place in the list, or -1 (new in SOCOM II; the name is ours) */
    int GetWeaponIndex(CZWeapon *weapon);
    CZWeapon *GetWeaponByIndex(int index);
    CZWeapon *GetWeapon(EQUIP_ITEM id);
    char *GetWeaponName(EQUIP_ITEM id);
    EQUIP_ITEM GetWeaponId(const char *name);
    CZWeapon *GetWeapon(const char *name);
    /* a weapon listed (new in SOCOM II, as CZAmmoList's AddAmmo; the name is ours) */
    void AddWeapon(CZWeapon *weapon);
    /* GetWeapon(EQUIP_ITEM) as GetWeaponName inlines it: the same walk with end() taken first --
     * no one source gives both the out-of-line copy (begin() first) and this inlined one (the
     * name is ours) */
    CZWeapon *Find(EQUIP_ITEM id)
    {
        iterator e = end();

        for (iterator it = begin(); it != e; ++it) {
            if ((*it)->GetId() == id)
                return *it;
        }
        return 0;
    }
};   // 0xc bytes

#endif
