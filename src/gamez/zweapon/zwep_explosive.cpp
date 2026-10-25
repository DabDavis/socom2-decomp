/* zwep_explosive.cpp -- PlacedExplosive, a charge set down (SOCOM 1's zwep_explosive.cpp, by its
 * static initialiser and its functions' order): OVL1 0x3C51A0-0x3C588F, after zwep_list.cpp's
 * CZAmmoList and before zwep_ftsweapon.cpp, as SOCOM 1's 0x315420-0x315757 follows CZAmmoList's
 * constructor and precedes CZFTSWeapon's GetZoomRange. SOCOM II adds three setters at its head (the
 * image's first), its table is SOCOM 1's two slots (__vt__15PlacedExplosive, 0x4083A0: PreTick
 * 0x3C5210, PostTick 0x3C5310). Defined in reverse of their image order (-inline deferred). */

#include "zweapon.h"
#include "../znode/znode.h"

using namespace zdb;

/* the matrix the identity, the time "never", the words and the flag cleared (SOCOM 1's set the
 * flags; the matrix is its member's constructor's identity, set again) */
PlacedExplosive::PlacedExplosive()
{
    m_detonate = false;
    m_8c = 9999999.0f;
    m_c8 = 0;
    m_cc = 0;
    m_matrix.SetIdentity();
}

/* its textures' character dropped and its intersections given back (SOCOM 1's, with the second
 * intersection SOCOM II's) */
PlacedExplosive::~PlacedExplosive()
{
    CWorld::m_world->DeleteTextureAssetCharacter(CHARSETCOMMON, m_instance);
    if (m_shottype == 1) {
        if (m_di)
            CZWeapon::m_pProjectilelist->ReleaseDirectFireDI(&m_di, &m_di_handle);
    } else if (m_di)
        CZWeapon::m_pProjectilelist->ReleaseIndirectFireDI(&m_di, &m_di_handle);
    if (m_di2)
        CZWeapon::m_pProjectilelist->ReleaseDirectFireDI(&m_di2, &m_di2_handle);
}

/* an intersection's flags: bit 19 marks it registered for the delayed proximity test (a view of
 * zdb::DiIntersect's first word, which znode.h does not lay out; the names ours) */
struct DI_FLAGS {
    unsigned int _b0 : 19;
    unsigned int m_delayed : 1;
    unsigned int _b20 : 12;
};

/* the projectile's detonation's handling (0x3C7EE0, not named) */
extern "C" void func_003C7EE0(CZProjectile *proj);

/* to be detonated: its proximity registered with the world at the blast's radius, and the
 * detonation handed on; detonated, handled once (SOCOM 1's had only the first; SOCOM II's skips
 * either while both of its two new flags are set) */
bool PlacedExplosive::PreTick(float dT)
{
    unsigned char held = m_b18;

    if (held && m_b17)
        return true;
    if (m_state == PROJECTILE_TO_BE_DETONATED) {
        if (m_di) {
            ((DI_FLAGS *)m_di)->m_delayed = 1;
            CWorld *world = CWorld::m_world;
            float radius = m_ammo->GetExplosionRadius();

            m_intersecthandle = world->RegProximityDelayed(m_di, &m_framestartpos, radius, (unsigned int)-1);
        }
        SetProjectileState(PROJECTILE_DETONATION_TO_BE_HANDLED, true);
        return true;
    }
    if (m_state == PROJECTILE_WAS_DETONATED && !held) {
        func_003C7EE0(this);
        return true;
    }
    return true;
}

/* set off from afar (SOCOM 1's) */
void PlacedExplosive::RemoteDetonate()
{
    m_detonate = true;
}

void PlacedExplosive::SetWord(int n)
{
    m_cc = n;
}

void PlacedExplosive::SetMatrix(int n, const CMatrix *m)
{
    m_c8 = n;
    m_matrix = m ? *m : CMatrix::identity;
}

void PlacedExplosive::SetRemote(bool remote)
{
    m_remote = remote;
}

bool PlacedExplosive::IsRemote()
{
    return m_remote;
}

void PlacedExplosive::SetFlag(unsigned char flag)
{
    m_flag = flag;
}
