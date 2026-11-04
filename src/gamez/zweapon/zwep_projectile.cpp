/* zwep_projectile.cpp -- CZProjectile, a shot or a thrown thing in flight (SOCOM 1's
 * zwep_projectile.cpp, by its static initialiser's place in the link): OVL1 0x3C6980-0x3CB8EF,
 * after zwep_global.cpp's template tail and before CZProjectileList (zwep_projlist.cpp), as SOCOM
 * 1's CZProjectile (0x317190-0x31A4DF in the demo) follows its flyout machine -- which SOCOM II
 * moved to the end of the overlay. SOCOM II opens the unit with five functions of its own (the hit
 * on the projectile, a flag setter, three position setters), splits SOCOM 1's UpdateGrenade in
 * three and adds an Init the constructor calls. Only the small and medium functions so far: the
 * rest stay placeholders. Defined in reverse of their image order (-inline deferred). */

#include "../znode/znode.h"
#include "../znode/node_world.h"
#include "../znode/int_material.h"
#include "zweapon.h"
#include "../zsys/zsys.h"

using namespace zdb;

/* the network's flags (znet.h's CZNetwork: only the two it tests) */
class CZNetwork {
public:
    unsigned char m_bSessionMaster;   // 0x0
    unsigned char m_bNetwork;   // 0x1
};
extern CZNetwork theNetwork;

/* the network grenade (fts_net_grenade.cpp's) */
class CZGrenadeState {
public:
    static void UpdateLocalGrenade(CZProjectile *proj);
};

/* a pickup (OVL2's) */
class CPickup {
public:
    static void Destroy(CPickup *pickup);
};

/* MSL's, in double (MAIN's copy) */
extern "C" double tan(double x);

/* an animation stopped (0x2707C0, not named) */
extern "C" void func_002707C0(CZAnim *anim, int b);

/* the tracers' speed (0x3E1570, initialised data; not named) */
extern "C" float data_003E1570;

/* SOCOM 1's tracer pool: a static nothing reads in SOCOM II, still built and torn down by the
 * initialiser (its destructor is the copy at 0x2CD710, linked from an earlier unit) */
class CZTracerPool {
public:
    static std::vector<zdb::CModel *> m_tracerpool;
};

/* the unit's statics, in the initialiser's order (__sinit_zwep_projectile.cpp, 0x404BB0: the point
 * copied from CPnt3D::zero, the pool's three words zeroed and its destructor registered) */
CPnt3D CZProjectile::m_explosionPos = CPnt3D::zero;
bool CZProjectile::m_newExplosion;
std::vector<zdb::CModel *> CZTracerPool::m_tracerpool;

/* ---- the members ---- */

/* the extension built, the members set, the extension pointed at the projectile */
CZProjectile::CZProjectile()
{
    m_owner = 0;
    Init();
    m_nodeex.SetProjectile(this);
}

/* cleaned up, the node deleted, the anim stopped and the pickup destroyed (SOCOM 1's, with the
 * second intersection SOCOM II's) */
CZProjectile::~CZProjectile()
{
    Cleanup();
    if (m_instance)
        m_instance = CNode::Delete(m_instance);
    m_di = 0;
    m_di2 = 0;
    m_weapon = 0;
    m_ammo = 0;
    m_owner = 0;
    if (m_animinstance) {
        func_002707C0(m_animinstance, 0);
        m_animinstance = 0;
    }
    if (m_pickup) {
        CPickup::Destroy(m_pickup);
        m_pickup = 0;
    }
}

/* every member's first value (SOCOM 1's constructor's stores, in its order, with SOCOM II's
 * members among them) */
/* (the bit tested for the tracer is bit 5 of the flags, SOCOM 1's bit 3; the flyout machine taken
 * between the count's two tests, as the target loads it; the diagonals set matrix by matrix) */
void CZProjectile::SetModelOrientation(bool b, const CPnt3D *dir)
{
    if (!m_instance)
        return;
    m_instance->SetActive(true);
    CPnt3D d = m_frameendpos - m_framestartpos;
    float len = d.Length();

    d.Normalize();
    CMatrix m(m_instance->m_matrix);
    CMatrix scale;
    CMatrix stretch;

    if (b) {
        len *= 0.1f;
        scale.SetIdentity();
        stretch = scale;
        stretch.m_matrix[0][0] = len;
        stretch.m_matrix[1][1] = len;
        stretch.m_matrix[2][2] = len;
        scale.m_matrix[0][0] = 7.5f;
        scale.m_matrix[1][1] = 7.5f;
        scale.m_matrix[2][2] = 7.5f;
    }
    CMatrix rot;
    CPnt3D fwd(1.0f, 0.0f, 0.0f);
    CQuat q;

    q.Align(fwd, d);
    q.ToMatrix(&rot);
    CBBox box;

    if (b) {
        sceVu0MulMatrix(scale.m_matrix, rot.m_matrix, scale.m_matrix);
        sceVu0MulMatrix(stretch.m_matrix, rot.m_matrix, stretch.m_matrix);
        m_instance->SetMatrix(&stretch);
        box = *m_instance->GetBBox();
        box.m_max.x = len;
        box.m_max.y = 0.1f;
        box.m_max.z = 0.1f;
    }
    if (b && m_lastHitWasVolumetric) {
        if (m_tracer_count) {
            CFlyoutMachine *machine = &CZWeapon::m_flyout_machine;

            if (!(m_tracer_count % 4)) {
                if (m_flyout_idx == 0)
                    CFlyoutMachine::SetTracerModel(CFlyoutMachine::pFlyout_Model_Ally);
                else if (m_flyout_idx == 1)
                    CFlyoutMachine::SetTracerModel(CFlyoutMachine::pFlyout_Model_Enemy);
                machine->AcquireFlyout(m_framestartpos, m_frameendpos, 0, m_weapon, data_003E1570,
                                       &scale, &box);
                CFlyoutMachine::SetTracerModel(CFlyoutMachine::pFlyout_Model_Standard);
                m_tracer_count = 0;
            }
        }
    } else {
        EQUIP_ITEM type = m_weapon->GetIDType();

        if (m_weapon->IsDirectFire() || type == 171 || type == 185 || type == 230)
            m_instance->SetMatrix(&rot);
    }
    m_instance->SetPosition(m_framestartpos.x, m_framestartpos.y, m_framestartpos.z);
}

void CZProjectile::Init()
{
    m_heading = m_pitch = m_roll = 0.0f;
    m_time = 9999999.0f;
    m_pickup = 0;
    SetDone(false);
    m_intersecthandle = -1;
    SetFlag19(false);
    m_pickup = 0;
    m_di = 0;
    m_di2 = 0;
    m_animinstance = 0;
    m_shottype = NORMAL;
    m_state = PROJECTILE_EXPIRED;
    m_currange = 0.0f;
    m_shooterID = 0;
    SetIsFirstHit(true);
    SetUseFlyout(false);
    SetIsPlayer(false);
    SetLastHitWasPerson(false);
    SetEcho(false);
    SetFlag7(false);
    m_weapon = 0;
    m_ammo = 0;
    SetLastHitWasVolumetric(false);
    m_flyout_idx = 1;
    m_tracer_count = 0;
    SetFlag8(false);
    SetFlag17(false);
    SetFlag18(false);
    m_di_handle = -1;
    m_di2_handle = -1;
    m_lastPlayerHitPos.x = 0.0f;
    m_lastPlayerHitPos.y = 0.0f;
    m_lastPlayerHitPos.z = 0.0f;
    m_lastVolumetricHit.x = 0.0f;
    m_lastVolumetricHit.y = 0.0f;
    m_98 = 0;
    SetFlag9(false);
    SetFlag20(false);
    m_c0 = -1;
    m_hasrested = 0;
}

/* its velocity: none, at rest; else flying and not done (SOCOM 1's) */
void CZProjectile::SetVelW(const CPnt3D *vel)
{
    if (!vel) {
        m_state = PROJECTILE_AT_REST;
        m_vel = CPnt3D::zero;
        return;
    }
    m_state = PROJECTILE_FLYOUT;
    m_vel = *vel;
    SetDone(false);
    m_intersecthandle = -1;
}

/* the fuse and the removal timer run out (SOCOM 1's HandleTimers, 0x3C99A0: a network game's copy of
 * another machine's shot -- neither the player's nor a person hit's -- leaves them to its owner,
 * the test written out each time (an inline bool predicate materialises its value); the fuse out, it is to be detonated -- done, and with the removal time out too
 * flagged (bit 17) unless remote; the removal time out once done, removed (7) unless remote and
 * no longer done. SOCOM 1's called the explosive's anim and returned true on a removal).
 * OPEN, 2 instructions: 0x1C8 against 0x1D0 -- the same blocks and the same reuse of the first
 * network load in the last test (a reload after the stores, as the target's), but the target sets
 * v0 = 0 in the fuse test's delay slot (its temporaries move to v1/a1/a2) and pads two more labels
 * (ahead of the flag-17 store and of the else arm's SetDone). Tried: a bool/int result variable
 * (top, after the test, returned everywhere), an else-if chain with one return, the m_done test
 * nested, the timers or the "never" constant in locals, the network flag in a bool/uchar/int local
 * (a bool used first and last gives the size, 92 off: MW then reuses it in the middle) */
bool CZProjectile::HandleTimers()
{
    if (theNetwork.m_bNetwork && !m_isplayer && !m_lastHitWasPerson)
        return false;
    if (m_time <= 0.0f && m_time != 9999999.0f && !m_done) {
        m_state = PROJECTILE_TO_BE_DETONATED;
        m_grenadestate = DETONATE;
        if (m_removaltime <= 0.0f && m_removaltime != 9999999.0f) {
            SetDone(true);
            if (!theNetwork.m_bNetwork || m_isplayer || m_lastHitWasPerson)
                SetFlag17(true);
        } else
            SetDone(true);
        return false;
    }
    if (m_done && m_removaltime <= 0.0f && m_removaltime != 9999999.0f) {
        if (!theNetwork.m_bNetwork || m_isplayer || m_lastHitWasPerson)
            m_state = (PROJECTILE_STATE)7;
        SetDone(false);
        m_grenadestate = REMOVE;
    }
    return false;
}

/* (a material's resistance to a blast is int_material.h's CMaterialEx::m_penetration, PENETRATION) */

/* whether an explosion reaches past the other intersection's hits: none counts for a flag-9
 * projectile, nor a hit on the node (or node extension) of the given one; a material that stops
 * blasts outright ends it, the others shrink the reach until it falls short of the shot's length
 * (SOCOM II's; the name CheckHit is HandleNextHit's) */
bool CZProjectile::CheckHit(DiIntersect *di, IntersectStruct *hit)
{
    if (m_b9)
        return true;

    bool measured = false;
    float len;
    float r = m_ammo->GetExplosionRadius();
    float reach = r * m_ammo->GetExplosionRadius();

    if (di->m_Cnt > 0) {
        for (int i = 0; i < di->m_Cnt; i++) {
            IntersectStruct *is = &di->m_Intersects[i];

            if (!is)
                continue;
            if (hit && hit->m_node) {
                if (hit->m_node == is->m_node)
                    continue;
                if (hit->m_node->m_nodeEx && hit->m_node->m_nodeEx == is->m_node->m_nodeEx)
                    continue;
            }

            unsigned int mat = is->GetMaterial();

            if (mat >= CMaterialExArray::m_materialex_list.size())
                mat = 0;

            CMaterialEx *ex = CMaterialExArray::m_materialex_list[mat];

            if (!ex)
                continue;

            float resist = ex->m_penetration;

            if (resist < 0.0001f)
                return false;
            if (!measured) {
                measured = true;

                CPnt3D d = di->m_Tip - di->m_Tail;

                len = d.x * d.x + d.y * d.y + d.z * d.z;
            }
            reach *= resist;
            if (reach < len)
                return false;
        }
    }
    return true;
}

/* the next hit, if it counts, handed to its node's extension -- an instance's to the node it
 * instances -- and the index moved on; none without both intersections */
bool CZProjectile::HandleNextHit()
{
    if (m_di && m_di2) {
        if (CheckHit(m_di2, &m_di->m_Intersects[m_hitindex])) {
            IntersectStruct *hit = &m_di->m_Intersects[m_hitindex];
            CNode *node = hit->m_node;

            if (node && (node->m_socom2_a1 & 4) && !node->m_socom2_a1_bits.bit3)
                hit->m_node = node->GetChildren()[0];
            if (hit->m_node && hit->m_node->m_nodeEx)
                hit->m_node->m_nodeEx->OnWeaponHit(hit->m_node, hit, this);
        }
        m_hitindex++;
        return false;
    }
    return true;
}

/* the node cleared from every hit of a detonated projectile's intersection */
void CZProjectile::ForgetHitNode(zdb::CNode *node)
{
    if (!(m_state == PROJECTILE_WAS_DETONATED && m_di->m_Cnt > 0))
        return;
    for (int i = 0; i < m_di->m_Cnt; i++) {
        IntersectStruct *hit = &m_di->m_Intersects[i];

        if (hit && hit->m_node == node)
            hit->m_node = 0;
    }
}

/* the timer given up if it is this one's, the intersections released, the node emptied and taken
 * out of the world, the pickup destroyed and the anim stopped (SOCOM 1's, with the second
 * intersection SOCOM II's) */
void CZProjectile::Cleanup()
{
    if (&m_time == CZWeapon::m_timer_pCurval)
        CZWeapon::ClearTimer(0);
    if (m_di) {
        if (m_shottype == ONE_FRAME)
            CZWeapon::m_pProjectilelist->ReleaseDirectFireDI(&m_di, &m_di_handle);
        else
            CZWeapon::m_pProjectilelist->ReleaseIndirectFireDI(&m_di, &m_di_handle);
        m_di = 0;
        m_di_handle = -1;
    }
    if (m_di2) {
        CZWeapon::m_pProjectilelist->ReleaseDirectFireDI(&m_di2, &m_di2_handle);
        m_di2 = 0;
        m_di2_handle = -1;
    }
    if (m_instance) {
        m_instance->SetNodeEx(0, true);
        m_instance->DeleteChildren();
        m_instance->SetActive(false);
        if (m_weapon && !m_weapon->IsDirectFire())
            CWorld::m_world->DeleteTextureAssetCharacter(CHARSETCOMMON, m_instance);
        if (m_instance->m_parent)
            m_instance->m_parent->DeleteChild(m_instance);
        FreeVector<std::vector<CNode *> >(m_instance->GetChildren());
        m_instance->SetModel(0);
        m_instance->DeleteDI();
    }
    if (m_pickup) {
        CPickup::Destroy(m_pickup);
        m_pickup = 0;
    }
    if (m_animinstance) {
        func_002707C0(m_animinstance, 0);
        m_animinstance = 0;
    }
}

void CZProjectile::Detonate()
{
    m_state = PROJECTILE_TO_BE_DETONATED;
}

bool CZProjectile::GetFlag9()
{
    return m_b9 != 0;
}

void CZProjectile::SetFlag9(bool on)
{
    m_b9 = on;
}

zdb::CNode *CZProjectile::GetInstance()
{
    return m_instance;
}

void CZProjectile::SetInstance(zdb::CNode *node)
{
    m_instance = node;
}

/* a cone by its length and its radius there, a start radius and the length's inverse (the name
 * ours) */
struct CONE {
    float m_length;   // 0x0
    float m_radius;   // 0x4
    float m_start;   // 0x8
    float m_invlength;   // 0xc

    bool Contains(const CPnt3D &d, float along)
    {
        if (along <= 0.0f)
            return false;
        if (!(along <= m_length))
            return false;
        float w = m_start + along * m_invlength * m_radius;

        if (Dot(d, d) - along * along <= w * w)
            return true;
        return false;
    }
};

/* the damage at a point: a detonation's falling off past half its radius to nothing at the
 * radius, an impact's from the weapon's effective range to nothing at its maximum; none when it
 * comes to nothing or under the minimum, else scaled for FTS (SOCOM 1's, with the weapon's bonus
 * SOCOM II's). SOCOM II's items 153 and 159 take cones besides: 153's damage a 32nd outside an
 * 84.375-degree cone about its node's -z axis, 159's none outside a 45-degree cone about its
 * node's rotated z axis, nor at all off the network. The node's position is assigned from its
 * matrix row field by field, one register (as SOCOM 1's): GetPosition()'s by-value copy stores
 * through a pointer; the -z axis through a row pointer keeps the node in one register; IsInCone
 * is pinned (called out of line).
 * OPEN, 0x4C8 against 0x4D0, 3 instructions: (1) the falloff's 2.0f * (r - 0.5f) -- the target
 * puts the product in f1 (the 2.0's register), here f0 (SOCOM 1's colouring is ours); (2) the
 * id -- the target copies the byte enum with `lb` and narrows it (`andi 0xff`) once for both
 * compares, here one `lbu`; the target's label pad before the 159 test (`b; nop; nop`) follows
 * from (2). Round 17: the impact arm is the target's -- `over` (dist - eff) a variable of its own,
 * declared ahead of `span` and assigned after it (the subtraction in the span test's delay slot,
 * span in f1 and over in f2); declared after span the two FPRs swap, inline in the division it
 * sinks below the branch. For (2): a byte enum held in a real variable is `lb` plus `andi` at the
 * compare (an id declared and initialised at the top: `lb s0` there, `andi v1,s0,0xff` at the
 * test -- CZProjectileList::Clear's `w ? w->m_ID : EQUIP_NONE` join is the same pair), so the
 * target's id is a variable MW does not fold into the compares; a local, an assigned local, an
 * assignment in the condition, a const, int, char or signed-char-cast local, a weapon local,
 * GetId(), and a switch on the local or the field (lbu, a compare chain 8 bytes longer) all
 * fold. Tried for (1): `damage = damage * (...)`, the product reversed, `/ 0.5f`, brackets, the
 * product, the difference or the factor in a variable, `r` and `dist` declared apart, a radius
 * variable. Round 18: the switch with its cases in either order (159 first: 125 off; the claymore
 * first: 0x4D8), `r -= 0.5f`, `t = r - 0.5f` and `k = 2.0f * (r - 0.5f)` locals change nothing */
float CZProjectile::GetDamage(const CPnt3D *pos, float minimum)
{
    float damage;

    if (WasDetonated()) {
        damage = m_ammo->GetExplosionDamage();
        damage += m_weapon->GetDamageBonus();
        CPnt3D d;

        d = *(const CPnt3D *)m_instance->m_matrix.m_matrix[3];

        d = d - *pos;
        float dist = d.Length();
        float r = dist / m_ammo->GetExplosionRadius();

        if (r > 1.0f)
            damage = 0.0f;
        else if (r > 0.5f)
            damage *= 1.0f - 2.0f * (r - 0.5f);
    } else {
        damage = m_ammo->GetImpactDamage();
        damage += m_weapon->GetDamageBonus();
        CPnt3D d = CPnt3D::zero;

        d = *(const CPnt3D *)m_instance->m_matrix.m_matrix[3];
        d = d - *pos;
        float dist = d.Length();
        float eff = m_weapon->GetEffectiveRange();

        if (dist > eff) {
            float over;
            float span = m_weapon->GetMaxRange() - eff;

            over = dist - eff;

            if (span <= 0.0f)
                damage = 0.0f;
            else
                damage *= 1.0f - over / span;
        }
    }
    if (damage && !(damage < minimum))
        damage = CZFTSWeapon::ResolveFTSDamage(damage);
    else
        damage = 0.0f;

    EQUIP_ITEM id = m_weapon->m_ID;

    if (id == CLAYMORE) {
        CPnt3D apex;

        apex = *(const CPnt3D *)m_instance->m_matrix.m_matrix[3];
        CPnt3D dir(0.0f, 0.0f, 1.0f);

        const float *z = m_instance->m_matrix.m_matrix[2];

        dir.x = -z[0];
        dir.y = -z[1];
        dir.z = -z[2];
        if (!IsInCone(pos, &apex, &dir, 1.4726216f))
            damage /= 32.0f;
    } else if (id == EQUIP_ITEM_159) {
        if (!theNetwork.m_bNetwork)
            damage = 0.0f;
        else {
            CPnt3D apex;

            apex = *(const CPnt3D *)m_instance->m_matrix.m_matrix[3];
            CPnt3D dir(0.0f, 0.0f, 1.0f);
            CPnt3D rot = CPnt3D::zero;

            m_instance->GetRotation(&rot);
            CMatrix m;

            m.SetRotate(&rot);
            TransformVectors(&m, &dir, &dir, 1);
            if (!IsInCone(pos, &apex, &dir, 0.7853982f))
                damage = 0.0f;
        }
    }
    return damage;
}

/* the damage a hit does at a point: the ammunition's impact damage (its explosion's once the
 * projectile has gone off) and the weapon's bonus, nothing when that is below the minimum, else
 * the damage at the point, resolved through the health and armour (SOCOM 1's, without the bonus) */
float CZProjectile::GetDamage(const CPnt3D *pos, float *health, float *armor, float minimum)
{
    float damage = m_ammo->GetImpactDamage();

    damage += m_weapon->GetDamageBonus();
    if (WasDetonated()) {
        damage = m_ammo->GetExplosionDamage();
        damage += m_weapon->GetDamageBonus();
    }
    float d = 0.0f;

    if (!(damage < minimum)) {
        d = GetDamage(pos, minimum);
        ResolveDamage(health, armor, d, m_ammo->GetArmorPierce());
    }
    return d;
}

/* OPEN, 0x168 against 0x17C: the target keeps the cone in its frame (sp+112..124, each field
 * stored where it is computed and reloaded by the test) and the projection at sp+96, with &d and
 * its y and z addresses in registers; MW scalarises this CONE whole. Tried: the projection by
 * const reference (0x168), Contains through a CONE pointer (0x174), the cone as a volatile
 * object read through a reference (0x180), the test written out (0x164), r * tan(angle) */
/* (pinned: GetDamage calls it out of line) */
#pragma auto_inline off
bool CZProjectile::IsInCone(const CPnt3D *pos, const CPnt3D *apex, const CPnt3D *dir, float angle)
{
    float r = m_ammo->GetExplosionRadius();
    CONE cone;

    cone.m_radius = tan(angle) * r;
    cone.m_start = 0.0f;
    cone.m_length = r;
    cone.m_invlength = 1.0f / r;
    CPnt3D d;

    pos->Sub(apex, &d);
    float along = Dot(*dir, d);

    return cone.Contains(d, along);
}
#pragma auto_inline reset

/* the damage done to a health through its armour: the armour takes the piercing's share and a
 * quarter of what it stopped (SOCOM 1's, the piercing turned around: 10 pierces nothing) */
float CZProjectile::ResolveDamage(float *health, float *armor, float damage, float pierce)
{
    float d = damage - *armor * ((10.0f - pierce) / 10.0f);

    if (d <= 0.0f)
        d = 0.0f;
    *armor -= (damage - d) / 4.0f;
    if (*armor <= 0.0f)
        *armor = 0.0f;
    *health -= d;
    if (*health <= 0.0f)
        *health = 0.0f;
    return *health;
}

/* within a range of where the frame started (SOCOM 1's) */
bool CZProjectile::IsInRange(const CPnt3D *pos, float range)
{
    CPnt3D d = *pos;

    d = d - m_framestartpos;
    return !(Dot(d, d) > range * range);
}

/* a thrown grenade's state to the network (SOCOM 1's, the person-hit test SOCOM II's) */
void CZProjectile::ReportGrenadeStatus()
{
    if (!m_isplayer || m_shottype == ONE_FRAME || m_shottype == ONE_FRAME_SHOTGUN || m_lastHitWasPerson)
        return;
    CZGrenadeState::UpdateLocalGrenade(this);
}

/* a remote grenade's place: both frame ends, the model turned to it (SOCOM 1's UpdateGrenade's
 * first part) */
bool CZProjectile::UpdateGrenadePosition(const CPnt3D *pos)
{
    m_framestartpos = *pos;
    m_frameendpos = *pos;
    if (m_shottype == ONE_FRAME_SHOTGUN)
        SetModelOrientation(true, pos);
    else if (m_shottype == ONE_FRAME)
        SetModelOrientation(true, pos);
    else
        SetModelOrientation(false, pos);
    return false;
}

/* a remote grenade's velocity: none puts a flying one at rest */
bool CZProjectile::UpdateGrenadeVelocity(const CPnt3D *vel)
{
    m_vel = *vel;
    if (vel->x == 0.0f && vel->y == 0.0f && vel->z == 0.0f
        && (m_state == PROJECTILE_FLYOUT || m_state == PROJECTILE_TO_BE_REMOVED))
        m_state = PROJECTILE_AT_REST;
    return false;
}

/* a remote grenade's state: removed (or never armed) says so; detonated, it detonates here, the
 * charges' node hidden (SOCOM 1's UpdateGrenade's last part) */
bool CZProjectile::UpdateGrenadeState(GRENADE_STATE state)
{
    if (state == REMOVE || (!m_weapon && !m_ammo))
        return true;
    if (state == DETONATE) {
        if (m_state != PROJECTILE_TO_BE_DETONATED && m_state != PROJECTILE_DETONATION_TO_BE_HANDLED
            && m_state != PROJECTILE_TO_BE_REMOVED && m_state != 7 && m_state != PROJECTILE_WAS_DETONATED) {
            m_state = PROJECTILE_TO_BE_DETONATED;
            /* (a switch: its chain tests the cases last to first, 185 first in the image) */
            switch (m_weapon->m_ID) {
            case 172:
            case 153:
            case 158:
            case 186:
            case 185:
                m_instance->SetActive(false);
                break;
            }
        }
    }
    return false;
}

/* the state set unless the network holds it: forced, a player's, or a person-hit projectile
 * passes; at rest marks it (SOCOM 1's, the person-hit test and the mark SOCOM II's) */
void CZProjectile::SetProjectileState(PROJECTILE_STATE state, bool force)
{
    if (!theNetwork.m_bNetwork || force || m_isplayer || m_lastHitWasPerson) {
        m_state = state;
        if (state == PROJECTILE_AT_REST)
            m_hasrested = 1;
    }
}

bool CZProjectile::WasDetonated()
{
    if (m_state == PROJECTILE_TO_BE_REMOVED || m_state == PROJECTILE_WAS_DETONATED)
        return true;
    return false;
}

/* back to the start: the pickup destroyed, the flags and times reset, the positions zeroed, the
 * model let go of, the weapon's and the ammo's state read again (SOCOM 1's, with SOCOM II's
 * members) */
void CZProjectile::Reset()
{
    if (m_pickup) {
        CPickup::Destroy(m_pickup);
        m_pickup = 0;
    }
    m_heading = 0.0f;
    m_pitch = 0.0f;
    m_roll = 0.0f;
    SetDone(false);
    m_intersecthandle = -1;
    m_animinstance = 0;
    m_shottype = NORMAL;
    m_state = PROJECTILE_EXPIRED;
    m_currange = 0.0f;
    m_shooterID = 0;
    m_98 = 0;
    SetFlag9(false);
    SetIsFirstHit(true);
    SetUseFlyout(false);
    SetIsPlayer(false);
    SetLastHitWasPerson(false);
    SetEcho(false);
    SetFlag7(false);
    m_hasrested = 0;
    m_time = 9999999.0f;
    m_removaltime = 9999999.0f;
    m_framestartpos = CPnt3D::zero;
    m_frameendpos = CPnt3D::zero;
    m_startpos = CPnt3D::zero;
    m_instance->SetModel(0);
    if (m_weapon)
        SetWeapon(m_weapon);
    if (m_ammo)
        SetAmmo(m_ammo);
}

/* placed with no velocity */
void CZProjectile::SetPositionAtRest(const CPnt3D *pos)
{
    SetPosition(pos);
    m_vel = CPnt3D::zero;
}

/* both frame ends and the node */
void CZProjectile::SetPosition(const CPnt3D *pos)
{
    m_frameendpos = *pos;
    m_framestartpos = m_frameendpos;
    m_instance->SetPosition(pos);
}

/* placed and set flying */
void CZProjectile::SetPositionVel(const CPnt3D *pos, const CPnt3D *vel)
{
    SetPosition(pos);
    SetVelW(vel);
}

void CZProjectile::SetLastHitWasPerson(bool on)
{
    m_lastHitWasPerson = on;
}

/* hit by another projectile's blast: detonated, unless it already was or the network holds it */
void CZProjectile::OnWeaponHit(zdb::IntersectStruct *hit, CZProjectile *projectile)
{
    if (projectile->GetDamage(&m_frameendpos, 1.0f)) {
        if (!WasDetonated() && (!theNetwork.m_bNetwork || m_useflyout)) {
            m_state = PROJECTILE_TO_BE_DETONATED;
            m_grenadestate = DETONATE;
            if (theNetwork.m_bNetwork && m_useflyout)
                ReportGrenadeStatus();
        }
    }
}

/* the last detonation's point, once: the flag cleared as it is handed out */
CPnt3D *CZProjectile::GetNewExplosion()
{
    CPnt3D *pos = 0;

    if (m_newExplosion) {
        m_newExplosion = false;
        pos = &m_explosionPos;
    }
    return pos;
}

/* the last detonation forgotten (the HUD's re-init) */
void CZProjectile::ClearNewExplosion()
{
    m_newExplosion = false;
}
