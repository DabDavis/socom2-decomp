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
    EQUIP_END = 254,
    EQUIP_NONE = 255
};

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

    char *m_name;   // 0x0
    char *m_description;   // 0x4
    unsigned char _8[0x30 - 0x8];
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
    /* the name and the description (zwep_weapon.cpp's, out of line: 0x3D2840, 0x3D2830 -- in
     * SOCOM 1's order, GetDescription before GetName) */
    char *GetName() const;
    char *GetDescription() const;
    /* the animations let go of (SOCOM 1's ClearAnims, 0x3D21C0) */
    void ClearAnims();
    /* the id by value (inline: GetWeapon(EQUIP_ITEM) narrows its argument into a register of its
     * own only so; the name is ours) */
    EQUIP_ITEM GetId() const { return m_ID; }

    char *m_name;   // 0x4
    char *m_description;   // 0x8
    unsigned char _c[0x7c - 0xc];
    EQUIP_ITEM m_ID;   // 0x7c (SOCOM 1: 0x60)
    unsigned char _7d[0xc4 - 0x7d];
    CZAmmoList m_LegalAmmoList;   // 0xc4 (SOCOM 1: 0x9c)
};

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
