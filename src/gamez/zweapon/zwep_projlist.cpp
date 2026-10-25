/* zwep_projlist.cpp -- CZProjectileList, the projectiles in flight: OVL1 0x3CB8F0?-0x3CDA2F, after
 * zwep_projectile.cpp's CZProjectile and before ZWepReader, as SOCOM 1's (0x31A4E0-0x31B91F in the
 * demo). SOCOM 1 gives the unit no static initialiser and no assert, so the file name is ours.
 * Only the intersection tables so far. Defined in reverse of their image order. */

#include "../znode/node_world.h"
#include "zweapon.h"

using zdb::DiIntersect;

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
