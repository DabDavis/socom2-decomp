/* zwep_list.cpp -- the weapon and ammunition lists, CZWeaponList and CZAmmoList: OVL1
 * 0x3C46A0-0x3C519F, after vis_main.cpp's tail and before zwep_explosive.cpp's PlacedExplosive,
 * as SOCOM 1's (0x314690-0x31541F in the demo, ahead of its PlacedExplosive). SOCOM 1 gives the
 * unit no static initialiser and no assert, so the file name is ours. One unit: GetWeaponName
 * inlines GetWeapon(EQUIP_ITEM). Defined in reverse of their image order (-inline deferred). */

#include <stdio.h>
#include <string.h>

#include "zweapon.h"
#include "../zsnd/zsnd.h"

/* what InitializeWeapons looks up: the world's textures (znode.h's: only the lookup and the
 * handle's texture), the animations (zanim.h's CZAnimMain: only the lookup), a decal set by name
 * (zwep_weapon.cpp's class: only the lookup) and a material's extension by index (int_material.cpp's
 * CMaterialExArray::GetWeaponAnimMaterialEx, SOCOM 1's static: its name first) */
namespace zdb {
class CTexHandle {
public:
    char *m_name;   // 0x0
    int m_count;   // 0x4
    unsigned int m_gsAddr;   // 0x8
    CTexture *m_texture;   // 0xc
};
class CWorld {
public:
    static CTexHandle *GetTexHandle(const char *name);
};
}
class CZAnimMain {
public:
    CZAnim *GetAnim(const char *name, const char *set_name);
};
extern CZAnimMain ZAnim;
class CZDecalSet {
public:
    static CZDecalSet *GetDecalSetByName(const char *name);
};
struct MATERIALEX {
    char *m_name;   // 0x0
};
/* (zweapon.h declares CMaterialExArray::GetWeaponAnimMaterialEx) */

/* ---- CZAmmoList (0x3C4E80-0x3C519F) ---- */

CZAmmoList::CZAmmoList()
{
}

/* the ammunition deleted with the list: &end_.next_ kept in a saved register for clear()'s and the
 * deleter's begin() (lw v0,0(s2)), as include/list's begin() reads the first node through a
 * reference to the sentinel */
CZAmmoList::~CZAmmoList()
{
    iterator e = end();

    for (iterator it = begin(); it != e; ++it) {
        if (*it)
            delete *it;
    }
    clear();
}

void CZAmmoList::AddAmmo(CZAmmo *ammo)
{
    if (ammo)
        push_back(ammo);
}

/* a name's ammo; none for no name. The element read through the base iterator and cast out by
 * value (`*it.i_`), so the target's re-read of it off the iterator after strcmp: the wrapper's
 * reference operator* keeps its address in a saved register across the calls (and no one form of
 * that operator serves both this and CGNode::FindEdge's reuse -- include/list). Pinned: GetAmmoId
 * calls it */
#pragma auto_inline off
CZAmmo *CZAmmoList::GetAmmo(const char *name)
{
    iterator it;

    if (!name)
        return 0;
    it = begin();
    for (iterator e = end(); it != e; ++it) {
        if (!strcmp(name, ((CZAmmo *)*it.i_)->GetName()))
            return (CZAmmo *)*it.i_;
    }
    return 0;
}
#pragma auto_inline reset

CZAmmo *CZAmmoList::GetAmmo(int index)
{
    iterator e = end();
    int i = 0;

    for (iterator it = begin(); it != e; ++it, ++i) {
        if (i == index)
            return *it;
    }
    return 0;
}

/* begin() assigned, then end() into a local (the iterator in memory, the end in a0) */
CZAmmo *CZAmmoList::GetAmmo(EQUIP_AMMO id)
{
    iterator it;

    it = begin();
    for (iterator e = end(); it != e; ++it) {
        if ((*it)->GetId() == id)
            return *it;
    }
    return 0;
}

EQUIP_AMMO CZAmmoList::GetAmmoId(const char *name)
{
    CZAmmo *ammo = GetAmmo(name);

    if (ammo)
        return ammo->m_ID;
    return AMMO_NONE;
}

/* ---- CZWeaponList (0x3C46A0-0x3C4E7F) ---- */

CZWeaponList::CZWeaponList()
{
}

/* the weapons deleted with the list, as ~CZAmmoList */
CZWeaponList::~CZWeaponList()
{
    iterator e = end();

    for (iterator it = begin(); it != e; ++it) {
        if (*it)
            delete *it;
    }
    clear();
}

void CZWeaponList::AddWeapon(CZWeapon *weapon)
{
    if (weapon)
        push_back(weapon);
}

/* the element cast out by value, as GetAmmo(const char *). Pinned: GetWeaponId calls it */
#pragma auto_inline off
CZWeapon *CZWeaponList::GetWeapon(const char *name)
{
    iterator it;

    it = begin();
    for (iterator e = end(); it != e; ++it) {
        if (!strcmp(name, ((CZWeapon *)*it.i_)->GetName()))
            return (CZWeapon *)*it.i_;
    }
    return 0;
}
#pragma auto_inline reset

EQUIP_ITEM CZWeaponList::GetWeaponId(const char *name)
{
    CZWeapon *weapon = GetWeapon(name);

    if (weapon)
        return weapon->m_ID;
    return EQUIP_NONE;
}

/* SOCOM 1's shape: the weapon found, then its string or a default (the find inlined) */
char *CZWeaponList::GetWeaponName(EQUIP_ITEM id)
{
    CZWeapon *weapon = Find(id);

    if (weapon)
        return weapon->GetDescription();
    return "Unknown Weapon";
}

/* begin() then end() (out of line; the inlined find in GetWeaponName takes end() first) */
CZWeapon *CZWeaponList::GetWeapon(EQUIP_ITEM id)
{
    for (iterator it = begin(), e = end(); it != e; ++it) {
        if ((*it)->GetId() == id)
            return *it;
    }
    return 0;
}

/* the handle's texture, none without a handle (SOCOM 1's shape: the null test repeated) */
static inline zdb::CTexture *HandleTexture(zdb::CTexHandle *h)
{
    return h ? h->m_texture : 0;
}

/* every weapon's sounds, decal set, icon and animations looked up by the names its entry gave
 * (SOCOM 1's; SOCOM II adds the passing rocket's sound, the three fire sounds, the reload sound's
 * second name and the zoomed thermal fire animation). The fire sounds' index is narrowed into a
 * byte variable of its own (the target's `andi s3,s4,0xff` once a pass, kept across the three
 * calls); the fire and default special animations are `a = 0; if (name) a = ...` (the zero in the
 * test's delay slot; a ternary branches round it); the walk tests `it != end()` (the end hoisted
 * into this's register, as GetWeaponIndex's).
 * OPEN, the right size (0x3AC), 51 instructions -- registers only, the walk's three: the target
 * colours the iterator s0, this and then the end s1 (`addiu s1,s1,4` in place) and the weapon s2;
 * here the weapon s0, the iterator s2 and this s1, the end recomputed into v1 at each test (with
 * `e = end()` in the for: this s0, the end s1, the iterator s2). The weapon or the iterators
 * declared at the top, before the loop or in the for, postfix steps, `!(it == end())`, `end() !=
 * it`, the weapon cast out through `it.i_` and a while walk move none of it */
void CZWeaponList::InitializeWeapons()
{
    CZWeapon::SetBulletPassSound(CSnd::GetSoundByName(".BUL_PASSING"));
    CZWeapon::SetRocketPassSound(CSnd::GetSoundByName(".ROCKET_BY"));
    for (iterator it = begin(); it != end(); ++it) {
        CZWeapon *w = *it;

        w->m_decalSet = CZDecalSet::GetDecalSetByName(w->GetDecalSetName());
        if (w->m_reloadsoundname)
            w->m_reloadsound = CSnd::GetSoundByName(w->m_reloadsoundname);
        if (w->m_socom2_a4)
            w->m_socom2_a0 = (unsigned int)CSnd::GetSoundByName((const char *)w->m_socom2_a4);
        for (int i = 0; i < 3; i++) {
            unsigned char c = i;

            if (w->GetFireSoundName(c))
                w->SetFireSound(c, CSnd::GetSoundByName(w->GetFireSoundName(c)));
        }
        if (w->GetIconTextureName()) {
            zdb::CTexHandle *h = zdb::CWorld::GetTexHandle(w->GetIconTextureName());

            if (h && HandleTexture(h))
                w->m_icon = h;
        }
        w->AllocateHitAnimMem();
        w->AllocateSpecialMaterialAnimMem();

        char *fire = w->GetFireAnimName();
        CZAnim *anim = 0;

        if (fire)
            anim = ZAnim.GetAnim(fire, 0);

        if (anim)
            w->SetFireAnim(anim);
        if (fire) {
            char name[32];

            sprintf(name, "%s_zoom", fire);
            CZAnim *zoom = ZAnim.GetAnim(name, 0);

            if (zoom)
                w->SetZoomedFireAnim(zoom);
        }
        if (fire) {
            char name[32];

            sprintf(name, "%s_zoom_TT", fire);
            CZAnim *zoom = ZAnim.GetAnim(name, 0);

            if (zoom)
                w->SetZoomedFireAnimTT(zoom);
        }

        char *hit = w->GetHitAnimName();

        if (hit) {
            unsigned int n = CMaterialExArray::m_materialex_list.size();

            for (unsigned int i = 0; i < n; i++) {
                MATERIALEX *m = CMaterialExArray::GetWeaponAnimMaterialEx(i);

                if (m) {
                    char name[32];

                    sprintf(name, "%s_%s", hit, m->m_name);
                    CZAnim *a = ZAnim.GetAnim(name, 0);

                    if (a)
                        w->SetHitAnim(i, a);
                    else
                        w->SetHitAnim(i, 0);
                }
            }
        }

        char *def = w->GetDefaultSpecialAnimName();
        CZAnim *special = 0;

        if (def)
            special = ZAnim.GetAnim(def, 0);

        if (special)
            w->SetDefaultSpecialAnim(special);

        char *mat = w->GetSpecialMaterialAnimName();

        if (mat) {
            unsigned int n = CMaterialExArray::m_materialex_list.size();

            for (unsigned int i = 0; i < n; i++) {
                MATERIALEX *m = CMaterialExArray::GetWeaponAnimMaterialEx(i);

                if (m) {
                    char name[32];

                    sprintf(name, "%s_%s", mat, m->m_name);
                    CZAnim *a = ZAnim.GetAnim(name, 0);

                    if (a)
                        w->SetSpecialMaterialAnim(i, a);
                    else
                        w->SetSpecialMaterialAnim(i, 0);
                }
            }
        }
    }
}

CZWeapon *CZWeaponList::GetWeaponByIndex(int index)
{
    iterator e = end();
    int i = 0;

    for (iterator it = begin(); it != e; ++it, ++i) {
        if (i == index)
            return *it;
    }
    return 0;
}

int CZWeaponList::GetWeaponIndex(CZWeapon *weapon)
{
    int i = 0;

    for (iterator it = begin(); it != end(); ++it, ++i) {
        if (*it == weapon)
            return i;
    }
    return -1;
}

void CZWeaponList::ClearAnimations()
{
    iterator e = end();

    for (iterator it = begin(); it != e; ++it)
        (*it)->ClearAnims();
}
