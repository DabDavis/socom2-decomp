/* pickup_main.cpp -- the pickups, CPickup: the weapons, ammunition, grenades and the MP bomb lying
 * in the world, and their list (SOCOM 1's unit, __sinit_pickup_main.cpp; SOCOM 1's order, Close
 * and Open last). OVL2 0x541650-0x542FD7: SetVisibleByUnit first, Open last, seal.cpp right after
 * (0x542E50 is Close, not the heartbeat). The list and the statics are OVL1 data, so this unit
 * defines none of them. Defined in reverse of their image order (-inline deferred). */

#include "zseal.h"

extern "C" {
/* the ammunition's (OVL1, unowned): its rounds tidied, and how many kinds it still holds */
void func_003CFB20(CPickupAmmo *ammo);
int func_003CFCF0(CPickupAmmo *ammo);
}

/* (OVL2 0x542FC0; SOCOM 1's) */
void CPickup::Open(unsigned int bits)
{
    bit_count = bits;
    m_isOpen = true;
}

/* every pickup the owner dropped destroyed (OVL2 0x5421F0; SOCOM 1's compared a byte of the id,
 * SOCOM II's the owner word). The iterator is assigned, so it stays in memory; the end taken
 * once. */
void CPickup::DestroyNetPickups(unsigned int owner)
{
    iterator i, e = m_list.end();

    for (i = m_list.begin(); i != e;) {
        CPickup *p = *i;

        ++i;
        if (owner == p->m_owner)
            Destroy(p);
    }
}

/* whether a weapon of the kind lies within the range of a point (OVL2 0x5420B0; new in SOCOM II,
 * the name ours) */
bool CPickup::IsWeaponNear(unsigned char item, const CPnt3D *pos, float range)
{
    iterator it = GetFirst();

    for (CPickup *p = GetNext(it, pos, range, PU_WEAPON); p; p = GetNext(it, pos, range, PU_WEAPON)) {
        CPickupWeapon *w = p->GetWeapon();

        if (w && item == w->m_equip)
            return true;
    }
    return false;
}

/* the walk's start (OVL2 0x542170; SOCOM 1's) */
CPickup::iterator CPickup::GetFirst()
{
    return m_list.begin();
}

/* the next pickup of a kind (any, for none) within the range of a point, the iterator left past
 * it (OVL2 0x541F10; SOCOM 1's). Out of line: IsWeaponNear calls it, where MW would inline it */
#pragma auto_inline off
CPickup *CPickup::GetNext(iterator &it, const CPnt3D *pos, float range, PU_TYPE type)
{
    if (m_list.size()) {
        iterator e = m_list.end();

        range *= range;
        for (; it != e; ++it) {
            CPickup *p = *it;

            if (type == PU_NONE || p->m_type == type) {
                CPnt3D d;

                p->GetPosW()->Sub(pos, &d);
                if (d.x * d.x + d.y * d.y + d.z * d.z < range) {
                    ++it;
                    return p;
                }
            }
        }
    }
    return 0;
}

#pragma auto_inline reset

/* the next pickup of a kind (any, for none), the iterator left past it (OVL2 0x542030; SOCOM 1's) */
CPickup *CPickup::GetNext(iterator &it, PU_TYPE type)
{
    if (m_list.size()) {
        iterator e = m_list.end();

        for (; it != e; ++it) {
            CPickup *p = *it;

            if (type == PU_NONE || p->m_type == type) {
                ++it;
                return p;
            }
        }
    }
    return 0;
}

/* a pickup by its network id (OVL2 0x541CD0; SOCOM 1's compared m_id alone) */
CPickup *CPickup::FindPickup(unsigned int id)
{
    if (m_list.size()) {
        iterator e = m_list.end();

        for (iterator i = m_list.begin(); i != e; ++i)
            if (id == (*i)->GetPickupNetEntityID())
                return *i;
    }
    return 0;
}

/* (OVL2 0x541CC0) */
unsigned short CPickup::GetEntityIDFromNetID(unsigned int netid)
{
    return netid >> 16;
}

/* the MP bomb's pickup, if one lies about (OVL2 0x541C60; SOCOM 1's) */
CPickup *CPickup::GetMPBomb()
{
    if (m_list.size()) {
        iterator e = m_list.end();

        for (iterator i = m_list.begin(); i != e; ++i)
            if ((*i)->m_type == PU_MPBOMB)
                return *i;
    }
    return 0;
}

/* a pickup whose contents are gone destroyed: ammunition once its last round is taken, a weapon
 * once it has none (OVL2 0x541BB0; SOCOM 1's, the ammunition now a class of OVL1's) */
void CPickup::CleanupAmmoPickup(CPickup *pickup)
{
    switch (pickup->m_type) {
    case PU_AMMO: {
        CPickupAmmo *ammo = pickup->GetAmmo();

        func_003CFB20(ammo);
        if (!func_003CFCF0(ammo))
            Destroy(pickup);
        break;
    }
    case PU_WEAPON:
        if (!pickup->GetWeapon())
            Destroy(pickup);
        break;
    }
}

/* the contents by kind, or 0 (OVL2 0x541AB0, 0x541A90, 0x541A20, 0x5419C0; new in SOCOM II, the
 * names ours) */
CPickupAmmo *CPickup::GetAmmo()
{
    return m_type == PU_AMMO ? (CPickupAmmo *)m_data : 0;
}

CPickupWeapon *CPickup::GetWeapon()
{
    return m_type == PU_WEAPON ? (CPickupWeapon *)m_data : 0;
}

void *CPickup::GetData4567()
{
    return IsType4567() ? m_data : 0;
}

void *CPickup::GetData123()
{
    return IsType123() ? m_data : 0;
}

/* (OVL2 0x5416E0; SOCOM 1's) */
const CPnt3D *CPickup::GetPosW() const
{
    if (m_node)
        return (const CPnt3D *)m_node->m_matrix.m_matrix[3];
    return 0;
}

/* the owner's entity id over the pickup's own (OVL2 0x5416C0) */
unsigned int CPickup::GetPickupNetEntityID()
{
    return m_owner << 16 | m_id;
}

/* (OVL2 0x5416B0, 0x541650; SOCOM 1's) */
bool CPickup::GetVisibleByUnit(unsigned int unit)
{
    return (m_visible_by_units & unit) != 0;
}

void CPickup::SetVisibleByUnit(unsigned int unit)
{
    if (unit == 2 || unit == 4 || unit == 8) {
        m_visible_by_units |= 1;
        m_visible_by_units |= 2;
        m_visible_by_units |= 3;
    } else
        m_visible_by_units |= unit;
}
