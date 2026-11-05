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

/* the marks left by hits (zwep_weapon.cpp's class, its layout there: only the call here) */
class CZMarkList {
public:
    void AddNewMark(zdb::IntersectStruct *is, CZProjectile *proj, float size);
};

/* something the AI notices (ai.h's CAiEvent: its fields and the pool's two calls) */
class CAiEvent {
public:
    static CAiEvent *Acquire();
    static void Create(CAiEvent *e);

    CPnt3D m_pos;   // 0x0
    float m_rangeSq;   // 0xc
    float m_duration;   // 0x10
    int m_owner;   // 0x14
    unsigned int m_id : 16;   // 0x18
    unsigned int m_visible : 1;   // 0x1a
    unsigned int m_type : 15;   // AIE_EXPLOSION is 8
};

/* an animation (zanim.h's class: the call only) */
class CZAnim {
public:
    CZAnim *ActivateArg(int type, ...);
};

/* MSL's, in double (MAIN's copy) */
extern "C" double tan(double x);

/* an animation stopped (0x2707C0, not named) */
extern "C" void func_002707C0(CZAnim *anim, int b);

/* the tracers' speed (0x3E1570, initialised data; not named) */
extern "C" float data_003E1570;

/* the weapon's impact radius (0x3D27D0, m_impactradius) and arming distance (0x3D2030, the float
 * at 0x68: an impact nearer the start bounces as a dud) -- zwep_weapon.cpp's getters, not named */
extern "C" float func_003D27D0(CZWeapon *weapon);
extern "C" float func_003D2030(CZWeapon *weapon);

/* a dud's velocity scale (0x3E1510, initialised data; not named) */
extern "C" float data_003E1510;

/* the person material's index (0x3E14F8; SOCOM 1's zwep::personMaterialId): a shot does not come
 * to rest on a person */
namespace zwep {
extern int personMaterialId;
}

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
    m_78 = 0;
    m_restnode = 0;
    m_98 = 0;
    SetFlag9(false);
    SetFlag20(false);
    m_c0 = -1;
    m_hasrested = 0;
}

/* made ready to fly: a launcher's fire animation started on the node (SOCOM II's), the
 * intersection taken from the direct- or the indirect-fire table and two flags cleared (SOCOM 1's
 * whole body), a volatile round's node given its first child's collision pieces and its extension,
 * and the node's pieces made its descendants' and its box rebuilt from them (SOCOM 1's
 * PreFireProjectile, 0x3CAF60: CZWeapon::Fire's set-up, SOCOM II's 0x3CB1A0 calls it) */
void CZProjectile::PreFireProjectile()
{
    EQUIP_ITEM type = CZWeapon::GetIDType(m_weapon->m_ID);

    if (type == 121 || type == 171 || type == 185 || type == 230) {
        CZAnim *anim = m_weapon->GetFireAnim(false);
        CZAnim *inst = 0;

        if (anim && CZWeapon::m_allowFireAnimations)
            inst = anim->ActivateArg(1, 3, m_instance, 0);
        m_animinstance = inst;
    }
    if (m_shottype == ONE_FRAME)
        CZWeapon::m_pProjectilelist->GetNextDirectFireDI(&m_di, &m_di_handle);
    else
        CZWeapon::m_pProjectilelist->GetNextIndirectFireDI(&m_di, &m_di_handle);
    SetFlag17(false);
    SetFlag18(false);
    if (m_ammo->IsVolatile()) {
        CNode *first = 0;
        CNodeVector children = m_instance->m_child;

        if (children.size())
            first = children[0];
        if (first) {
            CDIVector &src = first->m_di;
            CDIVector dis = src;

            for (int i = 0; i < dis.size(); i++)
                m_instance->AddDI(dis[i]);
        }
        m_instance->SetNodeEx(&m_nodeex, false);
    }
    m_instance->SetDescendantDI(true);
    m_instance->UpdateBBoxFromDI(true);
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

/* no node (inline: the ancestor walk's test is materialised, sltu/xori, before its branch -- as
 * zwep_explosive.cpp's) */
static inline bool NoNode(CNode *node)
{
    return !node;
}

/* an item tested against one id (ours: the weapons' round 24) -- the id handed over BY VALUE, so
 * the field is copied as a byte enum (`lb`) and narrowed at each test (`andi 0xff`, again after a
 * join), each compare its own inline call so MW folds no two consecutive ids into a range test:
 * HandleIntersections' and GetDamage's item tests. The field compared straight, through GetId()
 * or a local, is one `lbu` with 172/173 folded */
static inline bool IsItem(EQUIP_ITEM id, int item)
{
    return id == item;
}

/* the frame's hits walked nearest first, beyond the last one dealt with and within the range,
 * skipping any on a node under the shooter's: none left, a miss; else, unless the material stops
 * nothing (a penetration of 1), a bounce for the launchers' and satchels' classes and five items
 * (two of them only once at rest), an impact for the rest -- each saying whether the shot is spent
 * (0 and 4), stopped (1) or goes on to the next hit; the end point is the frame's end (SOCOM 1's
 * HandleIntersections, 0x3C9B70: SOCOM II's skips the hits while the delay at 0x98 runs, drops the
 * shotgun's impact, and ends the walk on 4 too). The shapes the target fixed: the hit count an
 * if/else on `m_98 > 0` (its zero in the branch's delay slot and the result's clear duplicated into
 * both arms); `closest` given a dead first definition at the top and `i` declared there (MW numbers
 * a variable at its first definition: closest s4, i s3, n s2 as the target's), the loop's point at
 * function scope (its y/z addresses hoisted into s6/s7, the frame's order) and the last hit's its own;
 * the done flag spilled to the frame (sp+176) as the target's.
 * OPEN, 0x438 against 0x440, 2 instructions: the item tests -- the target copies the weapon's id
 * with `lb a0` and narrows it (`andi v1,a0,0xff`) ahead of the 172/173/178/185 compares and again
 * after the 185 arm's join, compares unfolded; here MW reads one `lbu` and folds 172/173 into a range
 * test. GetDamage's (2) is the same miss. Tried: GetId() inline, a local (single-assigned, with a dead
 * default, assigned in the condition, through a comma `(id = .., false)`), a goto and a flag form
 * (the local's own statement after the class tests), an inline predicate taking the id (by value;
 * with the rest node), signed-char views of the field. Round 23: `(unsigned char)(signed char &)w->m_ID`
 * (the HUD's round-39 reinterpreting reference -- MW folds the pair back to one lbu), `(EQUIP_ITEM)
 * w->GetId()` (the kit's round-32 self-cast) and the field read straight all stay 0x438; the bounce/
 * impact choice as a ternary into `r` (the target narrows the result once after the join, here each
 * arm narrows) is 0x4B0; inline bool predicates taking the id by value -- EQUIP_ITEM, int or unsigned
 * char, one predicate or a launcher/rest pair, `return` or if/return-true -- materialise (0x450-0x480).
 * SOCOM 1's (0x3193F8) is `lbu` with 172/173 unfolded too */
bool CZProjectile::HandleIntersections(float dT)
{
    if (!m_di || m_state == PROJECTILE_AT_REST)
        return false;

    IntersectStruct *closest = 0;
    int i;
    int n;

    if (m_98 > 0.0f)
        n = 0;
    else
        n = m_di->m_Cnt;

    bool result = false;

    closest = 0;
    bool done = false;
    CPnt3D pos;
    CPnt3D d;

    do {
        float mindist;
        float range = m_currange * m_currange;

        mindist = 0.0f;
        float lastdist;

        if (!closest)
            lastdist = mindist;
        else {
            CPnt3D e;

            closest->m_pos.Sub(&m_di->m_Tail, &e);
            lastdist = e.x * e.x + e.y * e.y + e.z * e.z;
        }
        closest = 0;
        for (i = 0; i < n; i++) {
            IntersectStruct *is = &m_di->m_Intersects[i];
            CNode *node = is->m_node;

            if (node) {
                bool found = false;

                for (CNode *p = node->m_parent; !NoNode(p) && !found; p = p->m_parent)
                    if (p == m_owner)
                        found = true;
                if (found)
                    continue;
            }

            is->m_pos.Sub(&m_di->m_Tail, &d);

            float dist = d.x * d.x + d.y * d.y + d.z * d.z;

            if (!(dist <= lastdist)) {
                if (!closest || dist < mindist) {
                    if (dist < range) {
                        closest = is;
                        mindist = dist;
                    }
                }
            }
        }
        if (closest) {
            CMaterialEx *ex = CMaterialExArray::GetMaterialEx(closest->GetMaterial());
            float pen = 0.0f;

            if (ex)
                pen = ex->m_penetration;
            if (pen != 1.0f) {
                CZFTSWeapon *w = dynamic_cast<CZFTSWeapon *>(m_weapon);
                PROJECTILE_HIT r;

                if (w && (w->GetIDType() == 121 || w->GetIDType() == 151 || IsItem(w->GetId(), 172) ||
                          IsItem(w->GetId(), 173) || IsItem(w->GetId(), 178) ||
                          (IsItem(w->GetId(), 185) && m_restnode) || (IsItem(w->GetId(), 186) && m_restnode)))
                    r = HandleBounce(closest, dT, &pos);
                else
                    r = HandleImpact(closest, dT, mindist, &pos);
                if (r == 0 || r == 4) {
                    result = true;
                    done = true;
                } else if (r == 1) {
                    done = true;
                    result = false;
                } else {
                    result = false;
                    done = false;
                }
            }
        } else {
            if (DoMiss(dT, &pos) == 3)
                result = false;
            else
                result = true;
            done = true;
        }
    } while (!done);
    m_frameendpos = pos;
    CZWeapon::MakeWeaponEventCallbacks(1, this);
    return result;
}

/* the fuse and the removal timer run out (SOCOM 1's HandleTimers, 0x3C99A0: a network game's copy of
 * another machine's shot -- neither the player's nor a person hit's -- leaves them to its owner,
 * the test written out each time (an inline bool predicate materialises its value); the fuse out, it is to be detonated -- done, and with the removal time out too
 * flagged (bit 17) unless remote; the removal time out once done, removed (7) unless remote and
 * no longer done. SOCOM 1's called the explosive's anim and returned true on a removal).
 * SOCOM 1's shape: a result local (SOCOM 1's `move v0,zero` at the top), the fuse's no-removal arm
 * returning false itself, the removal arm falling through to `return r`, and the done arm ASSIGNING
 * the result -- SOCOM 1's `= true` (daddiu) is `= false` in SOCOM II, which keeps the variable and so
 * the target's early `v0 = 0` and its two padded labels (round 21: the result declared after the
 * network test or numbered there; at the top, or without the done arm's assignment, 8 bytes short) */
bool CZProjectile::HandleTimers()
{
    if (theNetwork.m_bNetwork && !m_isplayer && !m_lastHitWasPerson)
        return false;
    bool r = false;

    if (m_time <= 0.0f && m_time != 9999999.0f && !m_done) {
        m_state = PROJECTILE_TO_BE_DETONATED;
        m_grenadestate = DETONATE;
        if (m_removaltime <= 0.0f && m_removaltime != 9999999.0f) {
            SetDone(true);
            if (!theNetwork.m_bNetwork || m_isplayer || m_lastHitWasPerson)
                SetFlag17(true);
        } else {
            SetDone(true);
            return false;
        }
    } else if (m_done && m_removaltime <= 0.0f && m_removaltime != 9999999.0f) {
        if (!theNetwork.m_bNetwork || m_isplayer || m_lastHitWasPerson)
            m_state = (PROJECTILE_STATE)7;
        SetDone(false);
        m_grenadestate = REMOVE;
        r = false;
    }
    return r;
}

/* no hit this frame (SOCOM 1's DoMiss, 0x318D70 there, with SOCOM II's grenade tumble): a thrown
 * type-121 weapon's projectile not at rest turns about its node's three axes -- slowly, by how far
 * its rotation's y axis is from upright, when it barely moved; fastest over 15 units; in between
 * by the distance -- each angle kept under a turn; the point it reached handed back (its frame's
 * start once it has rested, the velocity then cleared); past the weapon's range the shot is spent
 * (4) off the network, removed (7) and missed (3) on it; within it, missed (3). The quaternion
 * goes to the node's pointer form through a cast: `&q` itself lets MW fold the inline's null test
 * (the target and SOCOM 1 keep it; a pointer local taken after MakeYXZRot keeps it too) */
PROJECTILE_HIT CZProjectile::DoMiss(float dT, CPnt3D *pos)
{
    CPnt3D d = m_frameendpos - m_framestartpos;
    float dist = d.Length();
    CZFTSWeapon *w = dynamic_cast<CZFTSWeapon *>(m_weapon);

    if (w && w->GetIDType() == 121 && m_state != PROJECTILE_AT_REST) {
        float roll = 0.0f;
        float pitch = 0.0f;
        float heading = 0.0f;

        if (dist != 0.0f && !m_hasrested) {
            if (dist < 0.5f) {
                CPnt3D rot = CPnt3D::zero;

                if (m_instance)
                    m_instance->GetRotation(&rot);
                float up = Dot(rot, CPnt3D(0.0f, 1.0f, 0.0f));

                if (fabs(up) > 0.2f) {
                    roll = 0.02f * (1.0f - up);
                    pitch = 0.4f * (1.0f - up);
                    heading = 0.1f * (1.0f - up);
                }
            } else if (dist > 15.0f) {
                roll = 50.0f;
                pitch = 2.5f;
                heading = 10.0f;
            } else {
                float t = (dist - 0.5f) / 15.0f;

                roll = 50.0f - 49.5f * t;
                pitch = 2.5f - 2.0f * t;
                heading = 10.0f - 9.5f * t;
            }
        }
        m_roll += 3.1415927f * roll * dT;
        if (m_roll > 6.2831855f)
            m_roll -= 6.2831855f;
        m_pitch += 3.1415927f * pitch * dT;
        if (m_pitch > 6.2831855f)
            m_pitch -= 6.2831855f;
        m_heading += 3.1415927f * heading * dT;
        if (m_heading > 6.2831855f)
            m_heading -= 6.2831855f;
        CQuat q;

        q.MakeYXZRot(m_heading, m_pitch, m_roll);
        if (m_instance)
            m_instance->SetRotation((const CQuat *)&q);
    }
    if (m_hasrested) {
        *pos = m_framestartpos;
        m_vel = CPnt3D::zero;
    } else
        *pos = m_frameendpos;
    if (dist > m_weapon->GetMaxRange()) {
        if (theNetwork.m_bNetwork) {
            SetProjectileState((PROJECTILE_STATE)7, false);
            return HIT_MISSED;
        }
        return HIT_SPENT_4;
    }
    return HIT_MISSED;
}

/* a bounce off the hit (SOCOM 1's HandleBounce, its time unread): a liquid or a material the shot
 * passes (penetration 0.99 or more) plays the weapon's hit anim there, damps the velocity by the
 * material's elasticity and lets the shot go on just past the hit (2). Anything else stops it (1):
 * the node it rests on kept unless a person, the velocity reflected off the hit (none once rested)
 * and damped -- by three quarters more on its first hit -- the hit anim played for a fast one
 * not under water; a slow one coming to rest when the bounce turned it upward (a person undamps it
 * instead), the point handed back just off the hit along its normal, or its frame's start once it
 * had rested already. Either way the hit node's extension is told. The target's shapes: the
 * material's index unsigned (GetMaterialEx's range test on it, the copy in the delay slot); the
 * hit point handed to ActivateArg BY VALUE through its `...` (MW copies it to a temporary and
 * passes the address: a named copy's slot sits above the temporaries, the target's among them);
 * `down` initialised by a ternary (numbered after the inline's temporary that holds the material,
 * so it takes s3 and the material s4 -- a plain bool is numbered first and swaps them); the
 * point off the hit a named local (its initializer's operator+ and operator* temporaries
 * allocated first, as the target's frame has them -- assigned straight into *pos they follow) */
PROJECTILE_HIT CZProjectile::HandleBounce(IntersectStruct *hit, float dT, CPnt3D *pos)
{
    unsigned int mat = hit->GetMaterial();
    CMaterialEx *ex = CMaterialExArray::GetMaterialEx(mat);
    PROJECTILE_HIT r;

    if (ex && ((bool)ex->m_liquid != false || !(ex->m_penetration < 0.99f))) {
        CZAnim *anim = m_weapon->GetHitAnim(mat);

        if (anim && CZWeapon::m_allowImpactAnimations)
            anim->ActivateArg(1, 5, hit->m_pos, 0);
        m_vel = m_vel * ex->m_elasticity;
        *pos = hit->m_pos + m_vel * 0.0001f;
        r = HIT_GOES_ON;
    } else {
        if (hit->m_node && zwep::personMaterialId != hit->GetMaterial())
            m_restnode = hit->m_node;
        bool down = m_vel.y < 0.0f ? true : false;
        CPnt3D v = m_hasrested ? CPnt3D::zero : m_vel.Reflect(&hit->m_norm);
        float e = 0.0f;

        if (ex)
            e = ex->m_elasticity;
        if (m_isfirsthit) {
            m_vel = v * (0.75f * e);
            SetIsFirstHit(false);
        } else
            m_vel = v * e;
        float speed = m_vel.Length();
        CZAnim *anim = m_weapon->GetHitAnim(mat);

        if (anim && (bool)ex->m_underwater == false && CZWeapon::m_allowImpactAnimations &&
            speed > 10.0f)
            anim->ActivateArg(1, 5, hit->m_pos, 0);
        unsigned char rested = m_hasrested;

        if (speed < 5.0f && m_state != PROJECTILE_AT_REST) {
            if (zwep::personMaterialId != hit->GetMaterial()) {
                if (down && m_vel.y > 0.0f) {
                    m_vel.x = m_vel.y = m_vel.z = 0.0f;
                    SetProjectileState(PROJECTILE_AT_REST, false);
                }
            } else
                m_vel = m_vel * (1.0f / e);
        }
        if (!m_hasrested || !rested) {
            CPnt3D n = hit->m_norm;

            n.Normalize();
            CPnt3D off = hit->m_pos + n * 0.1f;

            *pos = off;
        } else
            *pos = m_framestartpos;
        r = HIT_STOPPED;
    }
    if (hit->m_node && hit->m_node->m_nodeEx)
        hit->m_node->m_nodeEx->OnWeaponHit(hit->m_node, hit, this);
    return r;
}

/* an impact (SOCOM 1's HandleImpact, its shotgun branch gone): the point handed back is the hit's; a
 * person is taken up to the node below its tree's root, a second hit on the same one counted as
 * already dealt with (no mark, event or anim), a new one remembered with the hit point and the
 * echo flag. Out of range the shot is spent (4); else the hit node's extension is told, the mark
 * left, a noise the AI hears raised where the weapon's impact radius scaled by the material
 * reaches past a unit; a weapon with an arming distance not yet flown bounces, damped, as a dud;
 * a player's (or person-hit) explosive shot detonates on anything but a liquid or a pickup; the
 * flight anim is stopped, the hit anims played (a grenade launcher's next material's in a
 * liquid), a grenade launcher's shot out of a liquid spent at once (4); else the range is cut by
 * the material's penetration (the ammunition's armour pierce adds a tenth of it) and the shot
 * goes on (2) while still in range. The target's float shapes: the impact radius taken from the call
 * and scaled in place (`r = f(); r *= mod;` -- the product in one expression swaps the operands),
 * the explosion radius tested for truth (`&& GetExplosionRadius() &&`: a `!= 0.0f` compare puts the
 * zero first), the cut range scaled in place by the penetration */
PROJECTILE_HIT CZProjectile::HandleImpact(IntersectStruct *hit, float dT, float dist, CPnt3D *pos)
{
    *pos = hit->m_pos;
    CNode *node = hit->m_node;
    unsigned int mat = hit->GetMaterial();
    CMaterialEx *ex = CMaterialExArray::GetMaterialEx(mat);
    bool same = false;

    if (node && mat == zwep::personMaterialId) {
        CNode *parent = node->m_parent;
        CNode *next = parent->m_parent;

        while (next) {
            node = parent;
            parent = next;
            next = next->m_parent;
        }
        if (m_78 == node)
            same = true;
        else {
            m_78 = node;
            SetEcho(true);
            m_lastPlayerHitPos = hit->m_pos;
        }
    }
    CZWeapon *weapon = m_weapon;

    if (dist > m_currange * m_currange)
        return HIT_SPENT_4;
    CPnt3D hitpos = hit->m_pos;

    if (node && node->m_nodeEx)
        node->m_nodeEx->OnWeaponHit(node, hit, this);
    if (!same)
        CZWeapon::m_pMarklist->AddNewMark(hit, this, 1.0f);
    if (weapon && !same) {
        float r = func_003D27D0(m_weapon);

        r *= ex->m_impact_radius_mod;

        if (r > 1.0f && !m_echo) {
            CAiEvent *e = CAiEvent::Acquire();

            e->m_pos = hitpos;
            e->m_rangeSq = r * r;
            e->m_duration = 0.1f;
            e->m_type = 6;
            e->m_owner = m_shooterID;
            CAiEvent::Create(e);
        }
    }
    if (weapon && func_003D2030(weapon) > 0.0f) {
        CPnt3D d = *pos;

        d = d - m_startpos;
        float len = d.Length();

        if (len < func_003D2030(weapon)) {
            m_vel = m_vel * data_003E1510;
            m_time = 9999999.0f;
            SetDone(true);
            return HandleBounce(hit, 0.0f, pos);
        }
    }
    float pen = 0.0f;

    if (ex)
        pen = ex->m_penetration;
    if ((m_isplayer || m_lastHitWasPerson) && m_ammo->GetExplosionRadius() && ex &&
        (bool)ex->m_liquid == false && (bool)ex->m_pickup == false) {
        m_state = PROJECTILE_TO_BE_DETONATED;
        m_grenadestate = DETONATE;
        m_currange = 0.0f;
    }
    if (m_animinstance) {
        func_002707C0(m_animinstance, 0);
        m_animinstance = 0;
    }
    CZAnim *anim = 0;
    CZFTSWeapon *fw = dynamic_cast<CZFTSWeapon *>(m_weapon);

    if (fw->IsGrenadeLauncherAmmo() && (bool)ex->m_liquid != false)
        mat++;
    if (mat < CMaterialExArray::m_materialex_list.size())
        anim = m_weapon->GetHitAnim(mat);
    if (!same && anim && CZWeapon::m_allowImpactAnimations)
        anim->ActivateArg(1, 5, &hitpos, 7, &hit->m_norm, 6, &m_vel, 3, node, 0);
    CZAnim *hitanim = m_ammo->m_hitanim;

    if (!same && hitanim && CZWeapon::m_allowImpactAnimations)
        hitanim->ActivateArg(1, 5, &hitpos, 0);
    if (fw->IsGrenadeLauncherAmmo() && (bool)ex->m_liquid == false) {
        m_removaltime = 0.0f;
        return HIT_SPENT_4;
    }
    float range = m_currange;
    float nr = range + range * (0.1f * m_ammo->GetArmorPierce());

    nr *= pen;

    if (nr < range)
        m_currange = nr;
    if (dist > m_currange * m_currange)
        return HIT_SPENT_4;
    return HIT_GOES_ON;
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

/* the detonation's next hit on a node extension no earlier hit reached, a delayed ray registered
 * from the explosion's point to the centre of its node's box, the second intersection taken from
 * the direct-fire table for it and told to hit characters; true once no hit is left (the second
 * intersection given back) or none can be had (SOCOM II's, 0x3C7EE0: both PreTicks call it ahead
 * of HandleNextHit; the name ours) */
bool CZProjectile::RegisterNextHit()
{
    while (m_di && m_hitindex < m_di->m_Cnt && m_hitindex < 127) {
        /* the index a local of its own, numbered ahead of the extension (in the bitfield's place
         * the index takes another register), and the backward search guarded by it: the
         * target's `blez i` ahead of the loop's bottom-tested entry */
        int i = m_hitindex;

        IntersectStruct *hit = &m_di->m_Intersects[i];
        CNode *node = hit->m_node;

        if (node && node->m_nodeEx) {
            CNodeEx *ex = node->m_nodeEx;
            bool first = true;

            if (i > 0) {
                for (int j = i - 1; j >= 0 && first; j--) {
                    CNode *n = m_di->m_Intersects[j].m_node;

                    if (n && n->m_nodeEx == ex)
                        first = false;
                }
            }
            if (first) {
                CPnt3D center;

                if (hit->m_node) {
                    hit->m_node->GetBBox()->GetCenter(&center);
                    hit->m_node->TransformToWorld(center);
                }
                if (!m_di2 && !CZWeapon::m_pProjectilelist->GetNextDirectFireDI(&m_di2, &m_di2_handle))
                    return true;
                CWorld::m_world->RegIntersectDelayed(m_di2, &m_framestartpos, &center, -1);
                m_di2->m_Node = hit->m_node;
                m_di2->SetCharacterHits(true);
                return false;
            }
        }
        m_hitindex++;
    }
    if (m_di2) {
        CZWeapon::m_pProjectilelist->ReleaseDirectFireDI(&m_di2, &m_di2_handle);
        m_di2 = 0;
        m_di2_handle = -1;
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

/* the detonation: its mark on the ground under it (SOCOM 1's HandleDetonateExplosion, which then
 * handed each hit to its node's extension -- SOCOM II's leaves that to HandleNextHit), detonated,
 * the hit index back at the first, and the hits reordered, pass after pass until none moves: each
 * plain hit's place taken by a later instance's (0x3C7AF0; HandleDelayedDetonate calls it). The
 * shapes the target fixed: the node tests written out, each with its doubled null test (inline
 * bool predicates materialise their values); the flags set ahead of the three copies, each copy
 * indexing the array afresh; `j <= i` (the compare in at; `!(i < j)` materialises it);
 * `again = false` ahead of the search flag; the two di tests nested ifs (their exits share the
 * loop's); and the pass loop a `while (again)` -- as a do-while the loop optimiser numbers the
 * two strength-reduced offsets the other way round (s0/s1 swapped) */
void CZProjectile::HandleDetonateExplosion()
{
    if (!m_di)
        return;

    IntersectStruct *ground = CWorld::m_world->GetAltitude(&m_framestartpos, 10.0f);

    if (ground) {
        ground->GetRegion();
        CZWeapon::m_pMarklist->AddNewMark(ground, this, 1.0f);
    }

    DiIntersect *di = m_di;

    if (di) {
        m_state = PROJECTILE_WAS_DETONATED;
        m_hitindex = 0;
        if (di->m_Cnt > 0) {
            bool again = true;
            int i = 0;
            int j = 0;

            while (again) {
                if (i < di->m_Cnt - 1) {
                    IntersectStruct *a = &di->m_Intersects[i];

                    if (a->m_node && a->m_node &&
                        (!(a->m_node->m_socom2_a1 & 4) || a->m_node->m_socom2_a1_bits.bit3)) {
                        again = false;
                        bool searching = true;

                        if (j <= i)
                            j = i + 1;
                        for (; searching && j < di->m_Cnt; j++) {
                            if (di->m_Intersects[j].m_node && di->m_Intersects[j].m_node &&
                                (di->m_Intersects[j].m_node->m_socom2_a1 & 4) &&
                                !a->m_node->m_socom2_a1_bits.bit3) {
                                IntersectStruct t;

                                t.m_poly = 0;
                                t.m_node = 0;
                                again = true;
                                searching = false;
                                memcpy(&t, &di->m_Intersects[i], sizeof(IntersectStruct));
                                memcpy(&di->m_Intersects[i], &di->m_Intersects[j], sizeof(IntersectStruct));
                                memcpy(&di->m_Intersects[j], &t, sizeof(IntersectStruct));
                            }
                        }
                    }
                    i++;
                } else
                    again = false;
            }
        }
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

/* the detonation dealt with once its proximity has been registered: detonated; the blast's point
 * kept for the camera's shake unless the weapon is one of twelve; a noise the AI hears within the
 * weapon's sound radius; the weapon's default animation for nine weapons, else the material's
 * under the point (a flag set over water) or the default; the explosion handled when it has a
 * radius, the pickup destroyed, the claymore's (and 158's) node switched off, 158's removal timer
 * set; detonated, the point lifted (SOCOM 1's, 0x3C8330: its noise took the impact radius through
 * a call, its anim list five weapons long). The twelve are a switch (a compare chain in reverse
 * case order, no delay slot filled; written as `!=` tests MW fills them), the nine an `||` chain
 * (MW folds 127/128 and 172/173 into range tests); the underwater bit is tested as a bool
 * (`(bool)b != false` keeps the target's sltu) and the explosion setter copies through a local
 * pointer (zweapon.h's SetNewExplosion: through the parameter the offset folds into the loads) */
void CZProjectile::HandleDelayedDetonate()
{
    m_state = PROJECTILE_WAS_DETONATED;

    EQUIP_ITEM id = m_weapon->m_ID;

    switch (id) {
    case 122:
    case 125:
    case 123:
    case 127:
    case 128:
    case 159:
    case 172:
    case 173:
    case 174:
    case 177:
    case 178:
    case 182:
        break;
    default:
        SetNewExplosion(&m_framestartpos);
        break;
    }

    float r = m_weapon->m_soundradius;

    if (r > 1.0f) {
        CAiEvent *e = CAiEvent::Acquire();

        e->m_pos = m_framestartpos;
        e->m_rangeSq = r * r;
        e->m_duration = 0.5f;
        e->m_type = 8;
        e->m_owner = m_shooterID;
        CAiEvent::Create(e);
    }
    if (id == 122 || id == 127 || id == 128 || id == 172 || id == 173 || id == 178 || id == 182 ||
        id == 125 || id == CLAYMORE) {
        CZAnim *anim = m_weapon->GetDefaultSpecialAnim();

        if (anim && CZWeapon::m_allowImpactAnimations)
            anim->ActivateArg(1, 3, m_instance, 0);
    } else {
        IntersectStruct *hit = CWorld::m_world->GetAltitude(&m_framestartpos, 0.5f);
        CMaterialEx *ex = 0;
        int mat = 0;

        if (hit) {
            mat = hit->GetMaterial();
            ex = CMaterialExArray::GetMaterialEx(mat);
        }

        CZAnim *anim = 0;

        if (ex) {
            if ((bool)ex->m_underwater != false)
                SetFlag19(true);
            if (!(hit->m_pos.y <= m_framestartpos.y - 10.0f))
                anim = m_weapon->GetSpecialMaterialAnim(mat);
            if (!anim)
                anim = m_weapon->GetDefaultSpecialAnim();
        } else
            anim = m_weapon->GetDefaultSpecialAnim();
        if (anim && CZWeapon::m_allowImpactAnimations)
            anim->ActivateArg(1, 5, &m_framestartpos, 0);
    }
    if (m_ammo->GetExplosionRadius() > 0.0f)
        HandleDetonateExplosion();
    if (m_pickup) {
        CPickup::Destroy(m_pickup);
        m_pickup = 0;
    }
    if (m_weapon->m_ID == CLAYMORE || m_weapon->m_ID == 158)
        m_instance->SetActive(false);
    if (m_weapon->m_ID == 158)
        m_removaltime = m_weapon->GetTimer(1);
    m_state = PROJECTILE_WAS_DETONATED;
    m_framestartpos.y += 1.0f;
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
 * BYTE-EXACT since the weapons' round 26, by MW's float colouring (igdump/igreplay: the nodes
 * colour in descending number, each taking the lowest free FPR): the product's only neighbours
 * are the 1.0 and the damage, so no numbering alone gives it the target's f1 -- it must share a
 * node with the 2.0 (live beside the difference's f0), and the result with it: an in-place local
 * `k` (2.0, `*=`, `k = one - k`). As a named local k is numbered below the temporaries, so the
 * 1.0 must be a named local too, first defined between r and k (`one`): numbered r < one < k,
 * the difference colours f0, k f1, one f2, r f1 -- the target. `one` and `k` are stand-ins for
 * whatever the original named them (k as a temporary, or the 1.0 a literal, is 9 off: r and
 * the 1.0 swap). HISTORY: the falloff's 2.0f * (r -
 * 0.5f) -- the target puts the product and the difference in f1 (the 2.0's register, r's before
 * it), here f0 (SOCOM 1's colouring is ours). The id (2) is closed: each test an IsItem() call
 * over GetId() by value (the byte-enum copy `lb`, narrowed once for both tests, and the label pad
 * before the 159 test with it). Round 24 for (1): an in-place inline (`v *= s; return v;`, either
 * argument order, Falloff(v, r) returning `1 - v`, the difference assigned back to v) and a k
 * local (2.0 then `*=`, or the product then `k = 1 - k`, or `1 - 2k`) all put the product in the
 * 2.0's register but swap r's and 1.0's (9 off); OneMinus(x), Falloff(r), Fall(float &d, r) and
 * `r = 1 - 2(r - .5)` stay 3-4 off. Round 17: the impact arm is the target's -- `over` (dist - eff) a variable of its own,
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
 * first: 0x4D8), `r -= 0.5f`, `t = r - 0.5f` and `k = 2.0f * (r - 0.5f)` locals change nothing.
 * Round 21 for (2), by the first-definition rule: a dead `id = EQUIP_NONE` at the top or just ahead,
 * the id declared at the top uninitialised, read twice, through an int or a signed-char view --
 * all fold to lbu; assigned in both arms of the detonation test (a join) it is 0x4DC. Round 25,
 * MW's float colouring read off igdump (class 1: build/scratch/wep/igf.py, igcode.py): the nodes
 * colour in descending number, each the lowest FPR free, so the target's f1 product needs the 2.0,
 * the product and the difference's complement ONE node numbered between the 1.0 temporary (v66) and
 * the difference (v71) -- a temporary coalesced at the 2.0's number; a k local or an inline's
 * variable is numbered below the 1.0 and colours after it (r and 1.0 swap: 9 off); r as a
 * temporary (`dist / rad` written out) 5 off, `1.0f < r` 7 off */
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
        float one = 1.0f;

        if (r > one)
            damage = 0.0f;
        else if (r > 0.5f) {
            float k = 2.0f;

            k *= r - 0.5f;
            k = one - k;
            damage *= k;
        }
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

    if (IsItem(m_weapon->GetId(), CLAYMORE)) {
        CPnt3D apex;

        apex = *(const CPnt3D *)m_instance->m_matrix.m_matrix[3];
        CPnt3D dir(0.0f, 0.0f, 1.0f);

        const float *z = m_instance->m_matrix.m_matrix[2];

        dir.x = -z[0];
        dir.y = -z[1];
        dir.z = -z[2];
        if (!IsInCone(pos, &apex, &dir, 1.4726216f))
            damage /= 32.0f;
    } else if (IsItem(m_weapon->GetId(), EQUIP_ITEM_159)) {
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
 * object read through a reference (0x180), the test written out (0x164), r * tan(angle). Round 26
 * (build/scratch/wep/v_cone1.py): a user-declared destructor on CONE keeps it in the frame (0x188:
 * its field addresses then held in registers, the frame larger); a user constructor alone changes
 * nothing (0x168) */
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
