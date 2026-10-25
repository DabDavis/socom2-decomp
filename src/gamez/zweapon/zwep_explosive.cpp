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
    m_time = 9999999.0f;
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

/* the network's flags (znet.h's CZNetwork: only the one tested) */
class CZNetwork {
public:
    unsigned char m_bSessionMaster;   // 0x0
    unsigned char m_bNetwork;   // 0x1
};
extern CZNetwork theNetwork;

/* the AI maps (OVL2's AiMaps): a point's cell, and the shadow's darkness there (0 lit .. 1 dark;
 * zanim's shadow-light handler reads it the same way) -- not named */
class CAiMaps;
extern CAiMaps AiMaps;
extern "C" int func_0052DC70(CAiMaps *maps, const CPnt3D *pos, unsigned int *loc);
extern "C" float func_0052CE10(CAiMaps *maps, unsigned int *loc, const CPnt3D *pos, int a);

/* no node (inline: the attachment walk's test is materialised, sltu/xori, before its branch) */
static inline bool NoNode(CNode *node)
{
    return !node;
}

/* switched on (inline: the bit converted to a bool, sltu, before its test compared with false;
 * the walk's own variable gives the loop the preheader that the stored true is hoisted into) */
static inline bool NodeActive(CNode *node)
{
    return node->m_active;
}

/* SOCOM 1's PostTick with SOCOM II's head: a charge in flight stuck to a node (the word at 0xcc --
 * satchel and C4 alike, 151 and 153) goes off once any node above it is switched off; the
 * instance's shadow level follows the AI map's darkness at its frame end; a charge held by a
 * parent node (0xc8) follows its matrix. Then SOCOM 1's states, renumbered (7 is SOCOM II's
 * removed, 6 to be removed, 5 detonated), each detonated one handed its next hit first */
bool PlacedExplosive::PostTick(float dT)
{
    if (m_state == PROJECTILE_FLYOUT) {
        CNode *node = (CNode *)m_cc;

        if (node && !m_detonate && m_weapon) {
            EQUIP_ITEM id = m_weapon->GetId();

            if (id == 153 || id == 151) {
                for (CNode *n = node; !NoNode(n) && !m_detonate; n = n->m_parent)
                    if (NodeActive(n) == false)
                        m_detonate = true;
            }
        }
    }
    unsigned int loc;

    if (func_0052DC70(&AiMaps, &m_frameendpos, &loc)) {
        float light = func_0052CE10(&AiMaps, &loc, &m_frameendpos, 0);

        _saturate(&light, 0.0f, 1.0f);
        m_instance->m_socom2_a0 = (int)(127.0f * (1.0f - light));
    }
    if (m_c8 && m_instance) {
        CMatrix m(m_matrix);

        if (((CNode *)m_c8)->BuildMTW(m)) {
            m_instance->SetMatrix(&m);
            m_framestartpos = *(CPnt3D *)m_instance->m_matrix.m_matrix[3];
        }
    }
    PROJECTILE_STATE state = m_state;
    unsigned char held;

    if (state == 7 || ((held = m_b18) != 0 && m_b17)) {
        if (theNetwork.m_bNetwork) {
            m_grenadestate = REMOVE;
            ReportGrenadeStatus();
        }
        return true;
    }
    if (state == PROJECTILE_WAS_DETONATED) {
        if (held || HandleNextHit()) {
            if (m_b17)
                SetProjectileState((PROJECTILE_STATE)7, true);
            else
                SetProjectileState(PROJECTILE_TO_BE_REMOVED, true);
        }
        return false;
    }
    if (state == PROJECTILE_DETONATION_TO_BE_HANDLED) {
        HandleDelayedDetonate();
        return false;
    }
    if (state == PROJECTILE_TO_BE_REMOVED) {
        if (!theNetwork.m_bNetwork || m_isplayer) {
            if (m_removaltime != 9999999.0f)
                m_removaltime -= dT;
        }
        if (m_removaltime <= 0.0f || m_removaltime == 9999999.0f)
            SetProjectileState((PROJECTILE_STATE)7, false);
        else if (m_b17)
            SetProjectileState((PROJECTILE_STATE)7, true);
        return false;
    }
    if (m_time != 9999999.0f) {
        if (m_isplayer || !theNetwork.m_bNetwork)
            m_time -= dT;
        if (m_time <= 0.0f && !m_remote && !m_flag)
            m_detonate = true;
    }
    if (m_detonate) {
        Detonate();
        m_grenadestate = DETONATE;
        ReportGrenadeStatus();
        return false;
    }
    return false;
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
