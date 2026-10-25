/* zwep_projlist.cpp -- CZProjectileList, the projectiles in flight: OVL1 0x3CB8F0?-0x3CDA2F, after
 * zwep_projectile.cpp's CZProjectile and before ZWepReader, as SOCOM 1's (0x31A4E0-0x31B91F in the
 * demo). SOCOM 1 gives the unit no static initialiser and no assert, so the file name is ours.
 * Only the intersection tables so far. Defined in reverse of their image order. */

#include "../znode/node_world.h"
#include "zweapon.h"
#include "../zseal/zseal.h"

using zdb::DiIntersect;

/* the first free slot's projectile: none when every slot is taken; the last used moved up to it,
 * the slot marked, the next free found, its node activated and the projectile initialised
 * (SOCOM 1's activated it only).
 * OPEN, 2 instructions short: the target holds the slot in a0, the search's counter in a2 and its
 * cursor in a1 (this: a2, a1, a0) and pads its first early return with a second nop; the search
 * as an inline returning the slot or -1 (four spellings) or written out with a goto, and the
 * counter declared ahead of the slot, do not move them */
CZProjectile *CZProjectileList::GetNextFreeProjectile()
{
    int idx = m_firstfree;

    if (idx == -1)
        return 0;
    if (!(idx < m_size))
        return 0;
    if (m_lastused < idx)
        m_lastused = idx;
    m_used[idx] = 1;
    m_firstfree = NextFree();
    m_projectiles[idx].GetInstance()->SetActive(true);
    m_projectiles[idx].Init();
    return &m_projectiles[idx];
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
