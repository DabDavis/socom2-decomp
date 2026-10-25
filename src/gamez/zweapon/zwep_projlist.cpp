/* zwep_projlist.cpp -- CZProjectileList, the projectiles in flight: OVL1 0x3CB8F0?-0x3CDA2F, after
 * zwep_projectile.cpp's CZProjectile and before ZWepReader, as SOCOM 1's (0x31A4E0-0x31B91F in the
 * demo). SOCOM 1 gives the unit no static initialiser and no assert, so the file name is ours.
 * Only the intersection tables so far. Defined in reverse of their image order. */

#include "../znode/node_world.h"
#include "zweapon.h"
#include "../zseal/zseal.h"

using zdb::DiIntersect;

/* a node forgotten by every charge there is and every projectile in a slot in use up to the last
 * (SOCOM II's: ~CZSealBody calls it; the name ours) */
void CZProjectileList::ForgetHitNode(zdb::CNode *node)
{
    if (m_used && m_projectiles) {
        for (std::vector<PlacedExplosive *>::iterator it = m_explosives.begin(); it != m_explosives.end(); ++it) {
            if (*it)
                (*it)->ForgetHitNode(node);
        }

        int i;
        int last = m_lastused;

        if (last != -1) {
            for (i = 0; i <= last; i++) {
                if (m_used[i] == 1)
                    m_projectiles[i].ForgetHitNode(node);
            }
        }
    }
}

/* flag 18 raised on every charge, then on every projectile in a slot in use up to the last
 * (SOCOM II's: _OnMissionComplete calls it; the name ours) */
void CZProjectileList::SetAllFlag18()
{
    if (m_used && m_projectiles) {
        for (std::vector<PlacedExplosive *>::iterator it = m_explosives.begin(); it != m_explosives.end(); ++it)
            (*it)->SetFlag18(true);

        int i;
        int last = m_lastused;

        if (last != -1) {
            for (i = 0; i <= last; i++) {
                if (m_used[i] == 1)
                    m_projectiles[i].SetFlag18(true);
            }
        }
    }
}

/* the first free slot's projectile: none when every slot is taken; the last used moved up to it,
 * the slot marked, the next free found, its node activated and the projectile initialised
 * (SOCOM 1's activated it only). The slot is declared after the two tests that read it: declared
 * ahead of them, it takes a2 where the target's has a0 (and AcquireProjectile's inlined copy too) */
CZProjectile *CZProjectileList::GetNextFreeProjectile()
{
    if (m_firstfree == -1)
        return 0;
    if (!(m_firstfree < m_size))
        return 0;

    int idx = m_firstfree;

    if (m_lastused < idx)
        m_lastused = idx;
    m_used[idx] = 1;
    m_firstfree = NextFree();
    m_projectiles[idx].GetInstance()->SetActive(true);
    m_projectiles[idx].Init();
    return &m_projectiles[idx];
}

/* constructed for a count of projectiles: no slots yet, the intersection tables empty, the first
 * free the first and none used, then made (SOCOM 1's) */
CZProjectileList::CZProjectileList(int count)
{
    int i;

    m_size = count;
    m_used = 0;
    m_projectiles = 0;
    for (i = 0; i < 60; i++)
        m_directFireDI[i] = 0;
    for (i = 0; i < 30; i++)
        m_indirectFireDI[i] = 0;
    m_firstfree = 0;
    m_lastused = -1;
    Init();
}

/* a shooter's charges, of the given weapon unless it is none, each made to explode, reported and
 * taken out; then, with no weapon given, his projectiles up to the last slot used, each made to
 * explode (state 7), reported and freed (SOCOM 1's took the shooter only).
 * OPEN, 4 instructions short: the weapon test's `andi` sits in the filter branch's delay slot here
 * where the target leaves it a nop and pads after the id compare; the id and charge registers
 * (v1/s2/s3 in the target: a1/s3/s2 here) follow. The tests as `filter && item != id` first, `id ==
 * item`, `filter == false` (an extra saved register) and an unsigned char id do not give it */
void CZProjectileList::Clear(int shooter, EQUIP_ITEM item)
{
    bool filter = item != EQUIP_NONE;

    if (m_used && m_projectiles) {
        std::vector<PlacedExplosive *>::iterator it;

        for (it = m_explosives.begin(); it != m_explosives.end();) {
            PlacedExplosive *e = *it;
            EQUIP_ITEM id = EQUIP_NONE;

            if (e->m_weapon)
                id = e->m_weapon->GetId();
            if (!filter || item == id) {
                if (shooter == e->m_shooterID) {
                    e->m_weapon->GetName();
                    (*it)->m_grenadestate = (GRENADE_STATE)3;
                    (*it)->ReportGrenadeStatus();
                    CPickup::Destroy(e->m_pickup);
                    CZWeapon::m_placedExplosives.Remove(e);
                    delete e;
                    it = m_explosives.erase(it);
                } else {
                    ++it;
                }
            } else {
                ++it;
            }
        }

        int i;
        int last = m_lastused;

        if (last != -1 && !filter) {
            for (i = 0; i <= last; i++) {
                if (m_used[i] == 1 && shooter == m_projectiles[i].m_shooterID) {
                    m_projectiles[i].m_grenadestate = (GRENADE_STATE)3;
                    m_projectiles[i].SetProjectileState((PROJECTILE_STATE)7, false);
                    m_projectiles[i].ReportGrenadeStatus();
                    Free(i);
                }
            }
        }
    }
}

/* the charges ticked, each one done taken out (its weapon's name fetched, its pickup destroyed,
 * unlisted, deleted and erased); then the projectiles up to the last slot used, each one done
 * freed (SOCOM 1's) */
void CZProjectileList::PostTick(float dT)
{
    std::vector<PlacedExplosive *>::iterator it;

    for (it = m_explosives.begin(); it != m_explosives.end();) {
        if ((*it)->PostTick(dT)) {
            PlacedExplosive *e = *it;

            e->m_weapon->GetName();
            CPickup::Destroy(e->m_pickup);
            CZWeapon::m_placedExplosives.Remove(e);
            delete e;
            it = m_explosives.erase(it);
        } else {
            ++it;
        }
    }

    int i;
    int last = m_lastused;

    if (last != -1) {
        for (i = 0; i <= last; i++) {
            if (m_used[i] == 1 && m_projectiles[i].PostTick(dT) == true)
                Free(i);
        }
    }
}

/* the tracers ticked, then the charges, then the projectiles up to the last slot used, each
 * through its table (SOCOM 1's) */
void CZProjectileList::PreTick(float dT)
{
    CZWeapon::m_flyout_machine.Tick(dT);
    for (std::vector<PlacedExplosive *>::iterator it = m_explosives.begin(); it != m_explosives.end(); ++it)
        (*it)->PreTick(dT);

    int i;
    int last = m_lastused;

    if (last != -1) {
        for (i = 0; i <= last; i++)
            m_projectiles[i].PreTick(dT);
    }
}

/* whether a projectile is in a slot in use or among the charges (SOCOM II's: NetRemoveProjectile
 * asks it first; the name ours) */
bool CZProjectileList::IsListed(CZProjectile *proj)
{
    if (!proj)
        return false;
    if (!m_used)
        return false;
    for (int i = 0; i < m_size; i++) {
        if (m_used[i] && &m_projectiles[i] == proj)
            return true;
    }
    for (std::vector<PlacedExplosive *>::iterator it = m_explosives.begin(); it != m_explosives.end(); ++it) {
        if (*it == proj)
            return true;
    }
    return false;
}

/* a projectile removed for the network: one being detonated is only flagged to go (SOCOM II's
 * includes state 6, and asks first whether the projectile is listed at all) */
void CZProjectileList::NetRemoveProjectile(CZProjectile *proj)
{
    if (IsListed(proj)) {
        PROJECTILE_STATE state = proj->m_state;

        if (state == PROJECTILE_TO_BE_DETONATED || state == PROJECTILE_DETONATION_TO_BE_HANDLED ||
            state == PROJECTILE_WAS_DETONATED || state == 6) {
            proj->m_b17 = true;
            return;
        }
        RemoveProjectile(proj);
    }
}

/* a projectile let go of: the slot holding it cleaned up and freed, and a charge taken out of the
 * charges besides (SOCOM 1's tested for a charge with dynamic_cast) */
void CZProjectileList::RemoveProjectile(CZProjectile *proj)
{
    if (proj && m_used) {
        for (int i = 0; i < m_size; i++) {
            if (m_used[i] && &m_projectiles[i] == proj)
                Free(i);
        }

        PlacedExplosive *e = (PlacedExplosive *)proj;

        if (e)
            RemoveExplosive(e);
    }
}

/* a charge taken out of the list: its weapon's name fetched and dropped, its pickup destroyed, the
 * charge unlisted, deleted and erased (SOCOM 1's walked raw pointers and kept no weapon name) */
#pragma auto_inline off
void CZProjectileList::RemoveExplosive(PlacedExplosive *explosive)
{
    if (explosive) {
        std::vector<PlacedExplosive *>::iterator it;

        for (it = m_explosives.begin(); it != m_explosives.end(); ++it) {
            PlacedExplosive *e = *it;

            if (e == explosive) {
                e->m_weapon->GetName();
                CPickup::Destroy(e->m_pickup);
                CZWeapon::m_placedExplosives.Remove(e);
                delete e;
                m_explosives.erase(it);
                break;
            }
        }
    }
}
#pragma auto_inline reset

/* the shooter's remote charges still in the world (SOCOM 1's: remote, his, its node active) */
int CZProjectileList::GetNumRemoteExplosives(int shooter)
{
    int count = 0;

    for (std::vector<PlacedExplosive *>::iterator it = m_explosives.begin(); it != m_explosives.end(); ++it) {
        PlacedExplosive *e = *it;

        if (e->IsRemote() && shooter == e->m_shooterID && e->GetInstance()->m_active)
            count++;
    }
    return count;
}

/* a direct-fire intersection let go of: free again, the first free moved back to it */
void CZProjectileList::ReleaseDirectFireDI(DiIntersect **di, int *handle)
{
    if (m_directFireDI[0]) {
        m_directFireDIUsed[*handle] = 0;
        if (*handle < m_direct_firstfree)
            m_direct_firstfree = *handle;
    }
}

/* the first free direct-fire intersection handed out, all hits enabled, the next free found */
bool CZProjectileList::GetNextDirectFireDI(DiIntersect **di, int *handle)
{
    if (!m_directFireDI[0])
        return false;
    if (m_direct_firstfree >= 60)
        return false;
    if (m_direct_firstfree < 60) {
        *di = m_directFireDI[m_direct_firstfree];
        (*di)->SetMask(0xFFFFFFFF);
        *handle = m_direct_firstfree;
        (*di)->m_socom2_bit19 = 0;
        m_directFireDIUsed[m_direct_firstfree] = 1;
        while (m_direct_firstfree < 60 && m_directFireDIUsed[m_direct_firstfree] == 1)
            m_direct_firstfree++;
    }
    return true;
}

void CZProjectileList::ReleaseIndirectFireDI(DiIntersect **di, int *handle)
{
    if (m_indirectFireDI[0]) {
        m_indirectFireDIUsed[*handle] = 0;
        if (*handle < m_indirect_firstfree)
            m_indirect_firstfree = *handle;
    }
}

bool CZProjectileList::GetNextIndirectFireDI(DiIntersect **di, int *handle)
{
    if (!m_indirectFireDI[0])
        return false;
    if (m_indirect_firstfree >= 30)
        return false;
    if (m_indirect_firstfree < 30) {
        *di = m_indirectFireDI[m_indirect_firstfree];
        *handle = m_indirect_firstfree;
        (*di)->m_socom2_bit19 = 0;
        (*di)->SetMask(0xFFFFFFFF);
        m_indirectFireDIUsed[m_indirect_firstfree] = 1;
        while (m_indirect_firstfree < 30 && m_indirectFireDIUsed[m_indirect_firstfree] == 1)
            m_indirect_firstfree++;
    }
    return true;
}

/* the next free projectile, given the weapon (its range and direct fire) and the ammunition
 * (SOCOM 1's) */
CZProjectile *CZProjectileList::AcquireProjectile(CZWeapon &weapon, CZAmmo &ammo)
{
    CZProjectile *p = GetNextFreeProjectile();

    if (p) {
        p->SetWeapon(&weapon);
        p->SetAmmo(&ammo);
    }
    return p;
}
