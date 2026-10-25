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

class CZAmmoList;
class CZAnim;
class CSnd;
struct _zrdr;

/* a cartridge type's stats (SOCOM 1's DWARF: 0x24 bytes, m_ID at 0x20; SOCOM II's is 0x40, its
 * reader news 0x40). SOCOM II's members past SOCOM 1's take the names of the rdr fields its ammo
 * reader (0x3CEDB0) hands them: "DisplayName" goes to 0x4, where SOCOM 1's description was,
 * "Description" to a third string at 0x8, then NumProjectilesFired, AccelerationFactor, the
 * Volitile and IgnoreExplosionDI tags, Blowback_Falloff and Blowback_End (squared, by index) and
 * ProximityDistance (squared). The functions are zwep_ammo.cpp's, out of line */
class CZAmmo {
public:
    CZAmmo();
    ~CZAmmo();
    /* the ammunition by name and by id (SOCOM 1's statics, through m_ammolist) */
    static EQUIP_AMMO GetAmmoId(const char *name);
    static CZAmmo *GetAmmo(EQUIP_AMMO id);
    /* SOCOM II's (the names ours, after the rdr fields) */
    void SetIgnoreExplosionDI(bool on);
    bool GetIgnoreExplosionDI();
    void SetVolatile(bool on);
    bool IsVolatile();
    void SetAccelerationFactor(float f);
    float GetAccelerationFactor();
    float GetProximityDistance();
    void SetProximityDistance(float d);
    /* the blowback's scale at a squared distance: 1 inside the falloff, down to 0 at the end */
    float GetBlowbackScale(float d2);
    void SetBlowback(unsigned char which, float d);
    int GetNumProjectilesFired();
    void SetNumProjectilesFired(int n);
    /* SOCOM 1's */
    float GetExplosionRadius();
    float GetExplosionDamage();
    float GetArmorPierce();
    float GetImpactDamage();
    char *GetName();
    char *GetDescription();
    void SetExplosionRadius(float r);
    void SetExplosionDamage(float d);
    void SetArmorPierce(float p);
    void SetStun(float s);
    void SetImpactDamage(float d);
    void SetHitAnim(const char *name);
    /* the third string (SOCOM II's: "Description"; the name ours) */
    void SetLongDescription(char *s);
    void SetDescription(char *s);
    void SetName(char *s);
    /* the id by value (inline: GetAmmo(EQUIP_AMMO) narrows its argument into a register of its
     * own only so; the name is ours) */
    EQUIP_AMMO GetId() const { return m_ID; }

    char *m_name;   // 0x0
    char *m_description;   // 0x4
    char *m_longdescription;   // 0x8 (SOCOM II's)
    float m_impactDamage;   // 0xc (SOCOM 1: 0x8)
    float m_stun;   // 0x10
    float m_pierce;   // 0x14
    float m_explosionDamage;   // 0x18
    float m_explosionRadius;   // 0x1c
    int m_numProjectilesFired;   // 0x20 (SOCOM II's, as the rest up to m_hitanim)
    float m_accelerationFactor;   // 0x24
    unsigned char m_volatile : 1;   // 0x28
    unsigned char m_ignoreExplosionDI : 1;   // 0x28
    CZAnim *m_hitanim;   // 0x2c (SOCOM 1: 0x1c)
    EQUIP_AMMO m_ID;   // 0x30 (SOCOM 1: 0x20)
    float m_blowback[2];   // 0x34 (SOCOM II's, squared)
    float m_proximityDistance;   // 0x3c (squared)

    /* the ammunition (SOCOM 1's static, 12 bytes: 0x4B5310) */
    static CZAmmoList m_ammolist;
};   // 0x40 bytes

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

    /* the decal sets read from the decals rdr's DECAL_SETS list (SOCOM 1's) */
    static void OpenDecals();
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

    /* the range (SOCOM 1's, the float at 0x3C: 0x3D27F0) */
    float GetMaxRange();
    /* the magazine's size and the magazines carried (SOCOM 1's GetAmmoCapacity and GetNumMags,
     * in its order: 0x3D2810 and 0x3D2800; CZKit's magazine code calls them) */
    int GetNumMags();
    int GetAmmoCapacity();

    /* the range the AI fires it at (SOCOM 1's DWARF: 0xc bytes -- a version byte, the custom and
     * hard-minimum flags, the optimal range; SOCOM II's at 0x70 of the weapon). SOCOM II reads the
     * flags a byte at a time, so their container is a byte */
    class AI_PARAMS {
    public:
        /* the weapon entry's "ai_params": the optimal range, when its minimum is the smaller, and
         * the hard minimum's tag; marked custom (new in SOCOM II, 0x3D0290: SOCOM 1 read a ZAR --
         * the name ours) */
        void Read(_zrdr *rdr);
        /* the accuracy at a distance: none at no distance or under a hard minimum, rising to the
         * minimum, full to the maximum and as far again past it, then none (SOCOM 1's answered
         * 1.0; 0x3D01E0) */
        float GetAccuracy(float d) const;
        /* (inline: a constant through the bool parameter is stored as `(b & 1) << n` -- the
         * names ours) */
        void SetCustom(bool b) { m_custom = b; }
        void SetHardMin(bool b) { m_hard_min = b; }

        unsigned char m_version;   // 0x0
        unsigned char m_custom : 1;   // 0x1
        unsigned char m_hard_min : 1;
        float m_min_range;   // 0x4
        float m_max_range;   // 0x8
    };   // 0xc bytes

    /* the three fire sounds by distance, close, medium and far (new in SOCOM II: their names from
     * the weapon entry's FireSoundClose/Med/Far, the sounds looked up by InitializeWeapons; the
     * names ours) */
    void SetFireSoundName(unsigned char i, const char *name);
    char *GetFireSoundName(unsigned char i);
    void SetFireSound(unsigned char i, CSnd *snd);
    /* the sound of the distance from the listener played at the point (0x3D2C50; the name ours) */
    void PlayFireSound(const CPnt3D *pos);
    /* the distances (squared) out to which the close and medium sounds play, from zweapon.rdr's
     * WEAPON_GLOBAL SoundDistanceClose/Med/Far (new in SOCOM II, 0x4B52F0; the names ours) */
    static void SetSoundDistance(unsigned char i, float d);
    static float m_sounddistance[3];
    /* the entry's Timer1 and Timer2 (new in SOCOM II; "never" when not given -- the names ours); the
     * item 152's first comes from the mission (theMission+0x1864) */
    void SetTimer(int i, float t);
    float GetTimer(int i);
    /* in the legal ammo list, by name (SOCOM 1's) */
    bool IsLegalAmmo(CZAmmo *ammo);

    char *m_name;   // 0x4
    char *m_description;   // 0x8
    unsigned char _c[0x24 - 0xc];
    FIREMODE m_maxfiremode;   // 0x24 (SOCOM 1: 0x20)
    unsigned char _25[0x28 - 0x25];
    int m_ammocap;   // 0x28 (SOCOM 1: 0x24)
    int m_nummags;   // 0x2c (SOCOM 1: 0x28)
    unsigned char _30[0x70 - 0x30];
    AI_PARAMS m_ai_params;   // 0x70 (SOCOM 1: 0x54)
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
    unsigned char _d6[0xd8 - 0xd6];
    float m_timer[2];   // 0xd8 (SOCOM II's, as the rest)
    CSnd *m_firesound[3];   // 0xe0
    char *m_firesoundname[3];   // 0xec
    /* the weapon events' callbacks, called and listed (zwep_global.cpp's, SOCOM 1's statics) */
    static void MakeWeaponEventCallbacks(unsigned int event, CZProjectile *proj);
    static void AddWeaponEventCallback(void (*callback)(CZProjectile *, unsigned int, void *),
                                       unsigned int events, void *data);
    static void RemoveWeaponEventCallback(void (*callback)(CZProjectile *, unsigned int, void *));

    /* the lists made and let go of (SOCOM 1's statics, in its order: 0x3D37D0-0x3D39DF) */
    static bool CreateWeaponList();
    static bool DestroyMarkList();
    static bool CreateMarkList();
    static bool DestroyProjectileList();
    static bool CreateProjectileList(int count);

    /* the weapons (SOCOM 1's static, 12 bytes: 0x4B5210) */
    static CZWeaponList m_listWeapons;
    /* the bullet marks (SOCOM 1's static: 0x4B5248; the class is zwep_weapon.cpp's) */
    static class CZMarkList *m_pMarklist;
    /* the projectiles in flight (SOCOM 1's static: 0x4B5220) */
    static CZProjectileList *m_pProjectilelist;
    /* the timer registered for the HUD, by the value it counts (SOCOM 1's static: 0x4B52C0) */
    static float *m_timer_pCurval;
    /* the tracers (SOCOM 1's static, 0x1c bytes: 0x4B5260) */
    static class CFlyoutMachine m_flyout_machine;
    /* the charges set down (SOCOM II's static, 0xc bytes: 0x4B5238, between the projectile list
     * and the mark list; Close clears it, the projectile list takes a charge out of it; the name
     * ours) */
    static class CPlacedExplosiveList m_placedExplosives;
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
class CModel;
class DiIntersect;
struct IntersectStruct;
}
class CPickup;

/* a projectile's state (SOCOM 1's DWARF: a byte enum; SOCOM II tests a seventh, 7, beside them --
 * not named) */
enum PROJECTILE_STATE {
    PROJECTILE_EXPIRED = 0,
    PROJECTILE_FLYOUT = 1,
    PROJECTILE_AT_REST = 2,
    PROJECTILE_TO_BE_DETONATED = 3,
    PROJECTILE_DETONATION_TO_BE_HANDLED = 4,
    PROJECTILE_WAS_DETONATED = 5,
    PROJECTILE_TO_BE_REMOVED = 6
};

/* a grenade's state on the network (SOCOM 1's DWARF: a byte enum) */
enum GRENADE_STATE {
    CREATE = 0,
    NEWPOS = 1,
    DETONATE = 2,
    REMOVE = 3
};

/* how a shot flies (SOCOM 1's DWARF: a byte enum; ONE_FRAME is direct fire) */
enum PROJECTILE_TYPE {
    NORMAL = 0,
    ONE_FRAME = 1,
    ONE_FRAME_SHOTGUN = 2
};

/* the projectile's extension of its node: a weapon's hit on the node handed to the projectile
 * (new in SOCOM II: __vt__17CProjectileNodeEx, 0x4083F0, overriding CNodeEx's first slot; its
 * functions out of line at 0x3D4B80-0x3D4BDF, in a unit after zwep_ammo.cpp). The class needs
 * znode.h's CNodeEx: a unit without it sees a stand-in of its size */
#ifdef ZNODE_H
class CProjectileNodeEx : public zdb::CNodeEx {
public:
    CProjectileNodeEx();
    void SetProjectile(CZProjectile *projectile);
    virtual int OnWeaponHit(zdb::CNode *node, zdb::IntersectStruct *hit, CZProjectile *projectile);

    CZProjectile *m_projectile;   // 0x4
};   // 8 bytes
#else
class CProjectileNodeEx {
public:
    void *m_vtable;   // 0x0
    CZProjectile *m_projectile;   // 0x4
};
#endif

/* a projectile (SOCOM 1's DWARF: 0xa8 bytes, the vtable pointer at 0; SOCOM II's is 0xc4: the node
 * extension inserted at 0x18, a second intersection at 0x30, two words after m_lastVolumetricHit
 * and one after m_currange, the volumetric material moved to 0xc0 -- read off Init and Reset). Its
 * table is SOCOM 1's two slots, PreTick and PostTick (__vt__12CZProjectile, 0x4083D0): no virtual
 * destructor, as in SOCOM 1. zwep_projectile.cpp's */
class CZProjectile {
public:
    /* (0x3CB8A0: the table stored first, the extension built, Init run) */
    CZProjectile();
    ~CZProjectile();
    virtual bool PreTick(float dT);    /* +0x08 */
    virtual bool PostTick(float dT);   /* +0x0C */
    /* the members' first values (new in SOCOM II: SOCOM 1's constructor did it inline; the name
     * ours) */
    void Init();
    void Reset();
    bool WasDetonated();
    /* the state set, unless the game holds it (forced, or a player's or a person-hit projectile
     * passes; SOCOM 1's, 0x3C6F00) */
    void SetProjectileState(PROJECTILE_STATE state, bool force);
    /* SOCOM 1's UpdateGrenade(pos, vel, state) in three (the network grenade's: the names ours) */
    bool UpdateGrenadeState(GRENADE_STATE state);
    bool UpdateGrenadeVelocity(const CPnt3D *vel);
    bool UpdateGrenadePosition(const CPnt3D *pos);
    void ReportGrenadeStatus();
    bool IsInRange(const CPnt3D *pos, float range);
    static float ResolveDamage(float *health, float *armor, float damage, float pierce);
    float GetDamage(const CPnt3D *pos, float scale);
    /* a point inside the blast's cone: from the apex along the direction for the explosion's
     * radius, widening by the angle's tangent (SOCOM II's; GetDamage asks it twice -- the name
     * and the parameters' order ours) */
    bool IsInCone(const CPnt3D *pos, const CPnt3D *apex, const CPnt3D *dir, float angle);
    void SetInstance(zdb::CNode *node);
    /* the node it flies (SOCOM 1's, out of line in zwep_projectile.cpp: 0x3C7AE0) */
    zdb::CNode *GetInstance();
    /* a detonated projectile's hits on a node forgotten (SOCOM II's: the projectile list calls it
     * when a node goes; the name ours) */
    void ForgetHitNode(zdb::CNode *node);
    /* whether a hit counts against the second intersection (0x3C80E0, SOCOM II's; not written,
     * the name ours) */
    bool CheckHit(zdb::DiIntersect *di, zdb::IntersectStruct *hit);
    /* the next hit handed to its node's extension, unless there is no second intersection
     * (SOCOM II's; the name ours) */
    bool HandleNextHit();
    void Detonate();
    /* the detonation's damage dealt once its proximity test has run (SOCOM 1's, 0x3C8330 -- not
     * written) */
    void HandleDelayedDetonate();
    void Cleanup();
    /* the node shown along the frame's travel: a stretched tracer flown every fourth frame when
     * asked, or the shot's own orientation (SOCOM 1's; nothing returned -- no path sets v0) */
    void SetModelOrientation(bool b, const CPnt3D *dir);
    void SetVelW(const CPnt3D *vel);
    /* SOCOM II's: a hit on the projectile itself from another (CProjectileNodeEx hands it on; the
     * name ours) */
    void OnWeaponHit(zdb::IntersectStruct *hit, CZProjectile *projectile);
    /* SOCOM II's position setters (the names ours): both frame ends and the node, then the
     * velocity -- none, or set and flying */
    void SetPosition(const CPnt3D *pos);
    void SetPositionAtRest(const CPnt3D *pos);
    void SetPositionVel(const CPnt3D *pos, const CPnt3D *vel);
    /* SOCOM II's flag accessors (bits 4 and 9 of the word at 4; the names ours) */
    void SetLastHitWasPerson(bool on);
    void SetFlag9(bool on);
    bool GetFlag9();
    /* inline: the bool's `& 1` survives a constant argument, as SetVelW's and Init's stores show
     * (the names ours) */
    void SetDone(bool on) { m_done = on; }
    void SetIsFirstHit(bool on) { m_isfirsthit = on; }
    void SetIsPlayer(bool on) { m_isplayer = on; }
    void SetUseFlyout(bool on) { m_useflyout = on; }
    void SetLastHitWasVolumetric(bool on) { m_lastHitWasVolumetric = on; }
    void SetEcho(bool on) { m_echo = on; }
    void SetFlag7(bool on) { m_b7 = on; }
    void SetFlag8(bool on) { m_b8 = on; }
    void SetFlag17(bool on) { m_b17 = on; }
    void SetFlag18(bool on) { m_b18 = on; }
    void SetFlag19(bool on) { m_b19 = on; }
    void SetFlag20(bool on) { m_b20 = on; }
    /* inline (SOCOM 1's Reset inlines both the same way): the weapon's range and direct fire */
    void SetWeapon(CZWeapon *weapon)
    {
        m_weapon = weapon;
        if (m_weapon) {
            m_currange = m_weapon->GetMaxRange();
            m_shottype = m_weapon->IsDirectFire() ? ONE_FRAME : m_shottype;
        }
    }
    void SetAmmo(CZAmmo *ammo) { m_ammo = ammo; }

    /* SOCOM 1's flags at 0x4 (m_done .. m_echo, bits 0-6); SOCOM II's at bits 7-9 and 17-20 (the
     * explosive's PreTick tests 17 and 18; the names ours) */
    unsigned int m_done : 1;   // 0x4
    unsigned int m_isfirsthit : 1;
    unsigned int m_isplayer : 1;
    unsigned int m_useflyout : 1;
    unsigned int m_lastHitWasPerson : 1;
    unsigned int m_lastHitWasVolumetric : 1;
    unsigned int m_echo : 1;
    unsigned int m_b7 : 1;
    unsigned int m_b8 : 1;
    unsigned int m_b9 : 1;
    /* the next of m_di's hits to hand on (SOCOM II's; the name ours) */
    unsigned int m_hitindex : 7;
    unsigned int m_b17 : 1;
    unsigned int m_b18 : 1;
    unsigned int m_b19 : 1;
    unsigned int m_b20 : 1;
    unsigned int _b21 : 11;
    GRENADE_STATE m_grenadestate;   // 0x8
    /* 1: direct fire (its intersection released to the direct-fire table) */
    PROJECTILE_TYPE m_shottype;   // 0x9
    PROJECTILE_STATE m_state;   // 0xa
    /* SOCOM II's: set when the state becomes PROJECTILE_AT_REST (the name ours) */
    unsigned char m_hasrested;   // 0xb
    CZWeapon *m_weapon;   // 0xc
    CZAmmo *m_ammo;   // 0x10
    CPickup *m_pickup;   // 0x14
    CProjectileNodeEx m_nodeex;   // 0x18 (SOCOM II's)
    zdb::CNode *m_instance;   // 0x20 (SOCOM 1: 0x18)
    zdb::CNode *m_owner;   // 0x24
    /* its intersection and the table's handle (SOCOM 1: 0x20, 0x24), and a second, always
     * direct-fire (SOCOM II's; the names ours) */
    zdb::DiIntersect *m_di;   // 0x28
    int m_di_handle;   // 0x2c
    zdb::DiIntersect *m_di2;   // 0x30
    int m_di2_handle;   // 0x34
    CZAnim *m_animinstance;   // 0x38 (SOCOM 1: 0x28)
    CPnt3D m_startpos;   // 0x3c
    CPnt3D m_framestartpos;   // 0x48 (the explosive's proximity is registered there)
    CPnt3D m_frameendpos;   // 0x54
    CPnt3D m_vel;   // 0x60
    CPnt3D m_lastPlayerHitPos;   // 0x6c
    CPnt3D m_lastVolumetricHit;   // 0x78
    float m_84;   // 0x84 (SOCOM II's, as 0x88)
    float m_88;   // 0x88
    float m_time;   // 0x8c ("never" at first: 9999999; SOCOM 1: 0x74)
    float m_removaltime;   // 0x90
    float m_currange;   // 0x94
    int m_98;   // 0x98 (SOCOM II's)
    float m_heading;   // 0x9c (SOCOM 1: 0x80)
    float m_pitch;   // 0xa0
    float m_roll;   // 0xa4
    /* the world's handle of its proximity registration (SOCOM 1: 0x8c) */
    int m_intersecthandle;   // 0xa8
    int m_shooterID;   // 0xac (SOCOM 1: 0x90)
    int m_object_index;   // 0xb0
    int m_flyout_idx;   // 0xb4
    int m_tracer_count;   // 0xb8
    unsigned char m_remove_ASAP;   // 0xbc
    int m_c0;   // 0xc0 (SOCOM II's: -1 at first)
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

/* the charges set down that go off near a SEAL (new in SOCOM II: a pointer vector of its own,
 * CZWeapon's static at 0x4B5238, which zwep_weapon.cpp's initialiser 0x404C10 builds and registers;
 * the projectile list takes a charge out when it goes, OVL2's proximity test 0x543930 walks it by
 * index and sets off one within its ammo's proximity distance, the kit adds a charge it sets down
 * (0x5BC730). Its functions are out of line at the head of OVL1's last unit, zwep_flyout.cpp; the
 * names ours) */
class CPlacedExplosiveList : public std::vector<PlacedExplosive *> {
public:
    CPlacedExplosiveList();
    ~CPlacedExplosiveList();
    void Clear();
    void Add(PlacedExplosive *explosive);
    void Remove(PlacedExplosive *explosive);
    int Count();
    PlacedExplosive *Get(int i);
};

namespace zdb {
class CModel;
}
class CBBox;

/* the flyouts (zwep_flyout.cpp's: SOCOM 1's classes, moved here for the projectile's tracers) */
/* a tracer in flight: its node (an empty xref the tracer model is set on), its anim, where and how
 * fast (SOCOM 1's class, 0x3c bytes in both; only what the code here touches is named) */
class CFlyout {
public:
    CFlyout();
    ~CFlyout();

    zdb::CNode *m_instance;   // 0x0
    CZAnim *m_anim;   // 0x4
    CPnt3D m_pos;   // 0x8
    CPnt3D m_vel;   // 0x14: the end it flies to (the name SOCOM 1's slot, the use the target's)
    int m_20;   // 0x20: the speed (a float: AcquireFlyout stores it, Tick reads it)
    int m_24;   // 0x24: the distance flown (a float)
    float m_maxdist;   // 0x28: the weapon's range, or 300 (the name ours)
    unsigned char m_done;   // 0x2c (SOCOM 1's Tick releases a flyout whose byte is set)
    unsigned char _2d[0x30 - 0x2d];
    CPnt3D m_30;   // 0x30: zeroed when it is acquired (not named)
};   // 0x3c bytes

/* the tracers: a pool of flyouts, the free ones and the active ones by pointer (SOCOM 1's
 * CFlyoutMachine, 0x1c bytes; CZWeapon's static m_flyout_machine, 0x4B5260) */
class CFlyoutMachine {
public:
    CFlyoutMachine();
    ~CFlyoutMachine();
    void Init(unsigned int count);
    static void HookupReset();
    void ReleaseAllActive();
    bool ReleaseFlyout(CFlyout *flyout, int index);
    /* the done flyouts released, then every active one moved on by its speed: done once past its
     * range, or arrived when the step would overshoot its end (SOCOM 1's) */
    bool Tick(float dT);
    /* a free flyout sent from start to end at a speed, unless the two are within 70 (SOCOM 1's) */
    bool AcquireFlyout(CPnt3D &start, CPnt3D &end, CZAnim *anim, CZWeapon *weapon, float speed,
                       CMatrix *m, CBBox *box);
    /* an active flyout's index, or -1 (inline: ReleaseFlyout's search; the name ours) */
    int FindActive(CFlyout *flyout)
    {
        unsigned int n = m_active.size();

        for (unsigned int i = 0; i < n; i++) {
            if (m_active[i] == flyout)
                return i;
        }
        return -1;
    }
    /* the model the next tracer takes (new in SOCOM II: SetModelOrientation picks the standard,
     * enemy or ally one; the name ours) */
    static void SetTracerModel(zdb::CModel *model);

    CFlyout *m_flyouts;   // 0x0
    std::vector<CFlyout *> m_free;   // 0x4
    std::vector<CFlyout *> m_active;   // 0x10

    /* SOCOM 1's statics: the model in use and the three tracer models */
    static zdb::CModel *pFlyout_Model_Registered;
    static zdb::CModel *pFlyout_Model_Standard;
    static zdb::CModel *pFlyout_Model_Enemy;
    static zdb::CModel *pFlyout_Model_Ally;
    /* SOCOM II's: the enemy's and the ally's tracer animations, looked up only when the flag
     * says so (the names ours) */
    static CZAnim *pFlyout_Anim_Ally;
    static CZAnim *pFlyout_Anim_Enemy;
    static bool m_tracer_anims;
};   // 0x1c bytes

/* the projectiles in flight and the intersections they test with (SOCOM 1's DWARF: 0x208 bytes;
 * the direct- and indirect-fire tables sit where SOCOM 1's do -- its Release*FireDI are
 * byte-identical) */
class CZProjectileList {
public:
    /* (0x3CD740, not written; the destructor 0x3CD6B0 deletes when told: CZWeapon's
     * DestroyProjectileList passes 1) */
    CZProjectileList(int count);
    ~CZProjectileList();
    /* the slots and intersection tables made (SOCOM 1's; 0x3CD480) */
    bool Init();
    /* the next free projectile, given the weapon and the ammunition (SOCOM 1's) */
    CZProjectile *AcquireProjectile(CZWeapon &weapon, CZAmmo &ammo);
    /* a shooter's charges (of one weapon, when given one) and, unless a weapon is given, his
     * projectiles: each set to explode, reported and taken out (SOCOM 1's took the shooter only) */
    void Clear(int shooter, EQUIP_ITEM item);
    bool GetNextIndirectFireDI(zdb::DiIntersect **di, int *handle);
    void ReleaseIndirectFireDI(zdb::DiIntersect **di, int *handle);
    bool GetNextDirectFireDI(zdb::DiIntersect **di, int *handle);
    void ReleaseDirectFireDI(zdb::DiIntersect **di, int *handle);
    void NetRemoveProjectile(CZProjectile *proj);
    int GetNumRemoteExplosives(int shooter);
    /* a charge taken out: its pickup destroyed, unlisted from the charges set down, deleted and
     * erased (SOCOM 1's, without the weapon's name it fetches) */
    void RemoveExplosive(PlacedExplosive *explosive);
    /* a projectile cleaned up and its slot freed, a charge taken out too (SOCOM 1's) */
    void RemoveProjectile(CZProjectile *proj);
    /* whether a projectile is one of the slots' or one of the charges (SOCOM II's; the name ours) */
    bool IsListed(CZProjectile *proj);
    /* the tracers ticked, then every charge's and every slot's projectile up to the last used
     * (SOCOM 1's) */
    void PreTick(float dT);
    /* every charge ticked, those done taken out; every slot's projectile up to the last used
     * ticked, those done freed (SOCOM 1's) */
    void PostTick(float dT);
    /* flag 18 set on every charge and every slot's projectile in use (SOCOM II's: the mission's
     * completion calls it; the name ours) */
    void SetAllFlag18();
    /* a node forgotten by every charge and every slot's projectile in use (SOCOM II's: a SEAL's
     * body going calls it; the name ours, after CZProjectile's) */
    void ForgetHitNode(zdb::CNode *node);
    /* the first free slot's projectile handed out: marked used, the next free found, its node
     * activated and the projectile initialised (SOCOM 1's, without the Init) */
    CZProjectile *GetNextFreeProjectile();
    /* the first free slot after the first free, or -1 (inline: the name ours) */
    int NextFree()
    {
        for (int i = m_firstfree + 1; i < m_size; i++) {
            if (!m_used[i])
                return i;
        }
        return -1;
    }
    /* the last slot in use found again (SOCOM 1's, out of line there; inline in SOCOM II, which
     * keeps no copy) */
    void FindLastUsed()
    {
        int last = -1;

        for (int i = 0; i < m_size; i++) {
            if (m_used[i] == 1)
                last = i;
        }
        m_lastused = last;
    }
    /* a slot freed: the first free moved back to it, the last used found again when it was that
     * (SOCOM 1's, out of line there) */
    void FreeIdx(int i)
    {
        m_used[i] = 0;
        if (i < m_firstfree)
            m_firstfree = i;
        if (i == m_lastused)
            FindLastUsed();
    }
    /* a slot's projectile cleaned up and the slot freed, while the slots exist (the name ours) */
    void Free(int i)
    {
        if (m_used) {
            m_projectiles[i].Cleanup();
            FreeIdx(i);
        }
    }

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
