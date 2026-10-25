/* zwep_list.cpp -- the weapon and ammunition lists, CZWeaponList and CZAmmoList: OVL1
 * 0x3C46A0-0x3C519F, after vis_main.cpp's tail and before zwep_explosive.cpp's PlacedExplosive,
 * as SOCOM 1's (0x314690-0x31541F in the demo, ahead of its PlacedExplosive). SOCOM 1 gives the
 * unit no static initialiser and no assert, so the file name is ours. One unit: GetWeaponName
 * inlines GetWeapon(EQUIP_ITEM). Defined in reverse of their image order (-inline deferred). */

#include <string.h>

#include "zweapon.h"

/* ---- CZAmmoList (0x3C4E80-0x3C519F) ---- */

CZAmmoList::CZAmmoList()
{
}

/* the ammunition deleted with the list.
 * OPEN, 4 bytes short (its draft matches, so it is compiled out): the target keeps &end_.next_ in
 * a saved register for clear()'s and the deleter's begin() (lw v0,0(s2)); every walk tried --
 * the wrapper's, the base deleter's, the end node through a reference -- reads it as 8(this) */
#if 0
CZAmmoList::~CZAmmoList()
{
    iterator e = end();

    for (iterator it = begin(); it != e; ++it) {
        if (*it)
            delete *it;
    }
    clear();
}
#endif

void CZAmmoList::AddAmmo(CZAmmo *ammo)
{
    if (ammo)
        push_back(ammo);
}

/* a name's ammo; none for no name.
 * OPEN, 3 instructions: the target re-reads the element off the iterator after strcmp; here the
 * reference operator* keeps its address in a saved register (s2) across the calls -- the base
 * deleter's iterator, a const walk (four iterator copies, the target has two) and a local copy
 * of the element all miss. Pinned: GetAmmoId calls it */
#pragma auto_inline off
CZAmmo *CZAmmoList::GetAmmo(const char *name)
{
    iterator it;

    if (!name)
        return 0;
    it = begin();
    for (iterator e = end(); it != e; ++it) {
        if (!strcmp(name, (*it)->GetName()))
            return *it;
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

/* the weapons deleted with the list.
 * OPEN, 4 bytes short (its draft matches, so it is compiled out): as ~CZAmmoList, the target
 * keeps &end_.next_ in a saved register (s2) for the two erases' begin() */
#if 0
CZWeaponList::~CZWeaponList()
{
    iterator e = end();

    for (iterator it = begin(); it != e; ++it) {
        if (*it)
            delete *it;
    }
    clear();
}
#endif

void CZWeaponList::AddWeapon(CZWeapon *weapon)
{
    if (weapon)
        push_back(weapon);
}

/* OPEN, 4 bytes long, as GetAmmo(const char *): the element's address kept across the calls.
 * Pinned: GetWeaponId calls it */
#pragma auto_inline off
CZWeapon *CZWeaponList::GetWeapon(const char *name)
{
    iterator it;

    it = begin();
    for (iterator e = end(); it != e; ++it) {
        if (!strcmp(name, (*it)->GetName()))
            return *it;
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
