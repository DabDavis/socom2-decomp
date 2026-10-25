/* seal_env.cpp -- the SEAL in its world: the terrain it can cross, the MP bomb, its animation sets
 * (SOCOM 1's unit, __sinit_seal_env.cpp; the press beta files ReadDeathAndFlinchAnims under it).
 * OVL2 from 0x59D980 (CanClimbWall, right after seal_dyn.cpp's template tail), in SOCOM 1's order.
 * Defined in reverse of their image order (-inline deferred). */

#include "zseal.h"
#include "../zanim/zanim.h"
#include <stdio.h>

/* the bomb's state machine (SOCOM 1's CZBombState, OVL1's) -- only the request */
class CZBombState {
public:
    enum EnumBombState { BOMB_STATE_0 = 0, BOMB_STATE_DEFUSING = 4 };

    static void RequestNewBombState(int id, EnumBombState state, const CPnt3D *pos);
};

/* CSequence, the timed sequence a look is, is zbody.h's (through zseal.h) */

/* a head's turn toward a direction, eased over a time (SOCOM 1's DWARF: CLook : CSequence, 0x80
 * bytes; SOCOM II's drops a quaternion: m_goal_dir and everything after it 0x10 earlier) */
class CLook : public CSequence {
public:
    void SetDir(const CPnt3D *dir, float rate);
    void LookTick(float dT);

    CPnt3D m_curr_dir;   // 0x10
    float m_rate;   // 0x1c
    CQuat m_quat0;   // 0x20
    CQuat m_quat1;   // 0x30
    CPnt3D m_goal_dir;   // 0x40 (SOCOM 1's 0x50)
    float m_deltaT;   // 0x4c
    CPnt3D m_cross;   // 0x50
    float m_fracT;   // 0x5c
    unsigned char m_done;   // 0x60
    bool IsDone() const { return m_done; }
    unsigned char _61[0x70 - 0x61];   // (SOCOM II's CLook is 0x70: CBlink's m_org at 0x70)
};

/* the eyelids: a look down and back up (SOCOM 1's DWARF: CBlink : CLook) */
class CBlink : public CLook {
public:
    void Blink(int org);
    void BlinkTick(float dT);

    int m_org;   // 0x70 (SOCOM 1's 0x80)
    float m_nextT;   // 0x74
};

extern "C" {
/* SOCOM 1's quadW (OVL1 0x2874F0, unowned): the look's easing curve */
float func_002874F0(float t);
/* the blink's speed and its angle down (OVL1 data; SOCOM 1's were constants, 1.5 and -0.5925) */
extern float data_0044C2DC;
extern float data_0044C2E0;
}

/* a decal a SEAL can leave: its size range and its texture (SOCOM 1's DWARF: CZDecalDef, 0xc
 * bytes) */
class CZDecalDef {
public:
    float m_decalminsize;   // 0x0
    float m_decalmaxsize;   // 0x4
    zdb::CTexHandle *m_texture;   // 0x8
};

extern "C" {
/* GetVisualNode (SOCOM 1's GetVisualNode__FPQ23zdb5CNode, called where SOCOM 1's HandleBloodDrip
 * calls it; OVL1 0x3D0FD0, unowned) */
zdb::CNode *func_003D0FD0(zdb::CNode *node);
int rand(void);
}
zdb::CNode *GetDecalNode(zdb::CNode *node);
/* the decals' projection, straight down (OVL2 bss, data.txt's) */
extern CPnt3D projDirDown;

/* a random fraction of one (rand() over 2^31) */
static inline float Rand01()
{
    return 4.656612873e-10f * (float)rand();
}

/* a size in a decal's range (an inline: its arguments are loaded before rand() runs; the range
 * is lost -- (hi - hi) -- in both builds) */
static inline float RandSize(float lo, float hi)
{
    return lo + Rand01() * (hi - hi);
}

/* now and then a wounded SEAL drips blood: offline only, a random decal of the set at random size
 * under it, along its altitude hit (OVL2 0x5A33B0; SOCOM 1's word for word, the timer at 0x214 --
 * its size range is min + r * (max - max), as both builds have it).
 * OPEN, 0x1BC against 0x1B8 (93 of 110 instructions in place): the target computes the whole size
 * before GetDecalNode is called (the sum in $f20, hi's register, in the call's delay slot) and so
 * saves only $f20/$f21; MW here keeps the random product in $f20 across the call and adds after
 * it, saving $f22 too. Tried: the size in a local, the decal node in a local, the helper's sum
 * through a local, the product written inside the argument with min and max in locals (either
 * order). The early return and the zero-first pick (`if (n == 0) def = 0; else ...`) are the
 * target's. */
void CZSealBody::HandleBloodDrip(float dT)
{
    if (!(m_health < 1.0f) || theNetwork.m_9)
        return;
    if (m_blooddriptimer > 0.0f) {
        m_blooddriptimer -= dT;
        return;
    }
    m_blooddriptimer = 3.0f * Rand01();
    int n = m_blooddriptexhandles.size();
    CZDecalDef *def;

    if (n == 0)
        def = 0;
    else
        def = &m_blooddriptexhandles[n == 1 ? 0 : rand() % n];

    if (def) {
        zdb::IntersectStruct *alt = &m_last_altitude;
        zdb::CNode *node = func_003D0FD0(alt->m_node);

        if (node) {
            CPnt3D dir;

            alt->m_norm.Scale(-1.0f, &dir);
            float size = RandSize(def->m_decalminsize, def->m_decalmaxsize);

            node->ApplyDecal(GetDecalNode(alt->m_node), 100.0f, &alt->m_pos, &dir, &projDirDown,
                             def->m_texture, size, 0);
        }
    }
}

/* a float negated in place (an inline taking it by reference: the target negates the quaternion's
 * y and z through their addresses) */
static inline void Negate(float &f)
{
    f = -f;
}

/* the motion slid along a wall: the wall's normal brought into the SEAL's frame (the inverse of
 * its quaternion), flipped, and the velocity's part along it taken away (OVL2 0x5B0DD0; SOCOM 1's,
 * the velocity now CEntity's m_velM). Its unit is SOCOM 1's DI-collision stretch (CheckDIDown,
 * SetWallHit, Slide, CheckDILadder), kept here beside the look code until its boundary is read */
void CZSealBody::Slide(zdb::IntersectStruct *hit)
{
    CQuat inv = m_quat;

    Negate(inv.vec.x);
    Negate(inv.vec.y);
    Negate(inv.vec.z);
    CPnt3D n;

    inv.Apply(&hit->m_norm, &n, 1);
    n.Scale(-1.0f, &n);
    n.Scale(Dot(n, m_velM), &n);
    m_velM.Sub(&n, &m_velM);
}

/* where a hit lands (SOCOM 1's DAMAGE_LOCATION, a byte: only the name matters here) */
enum DAMAGE_LOCATION { DAMAGE_LOCATION_0 = 0, DAMAGE_LOCATION_LAST = 255 };

/* a SEAL's animations (SOCOM 1's DWARF: AnimSet) -- only its readers of the damage animations,
 * static: each reads a location's block, its STAND, CROUCH and PRONE lists */
class AnimSet {
public:
    static void ReadFlinchAnimations(_zrdr *rdr, const char *name, DAMAGE_LOCATION loc);
    static void ReadDeathAnimations(_zrdr *rdr, const char *name, DAMAGE_LOCATION loc);
    static void ReadTeammateDeathAnimations(_zrdr *rdr, const char *name, DAMAGE_LOCATION loc);
};

/* an array's item by an unsigned index (zrdr.h's Item, which this unit cannot include beside
 * zanim.h's _zrdr: the index unsigned keeps MW from strength-reducing an int counter's walk) */
static inline _zrdr *ItemAt(_zrdr *a, unsigned int i)
{
    return i < a->Count() ? &a->array[i] : 0;
}

extern "C" {
/* one animation of a list read into the location's and the state's tables: a flinch, a death, a
 * teammate's death (OVL2 0x5A3D30, 0x5A3E90, 0x5A3FF0; SOCOM 1's readers did it inline) */
void func_005A3D30(_zrdr *anim, int state, DAMAGE_LOCATION loc);
void func_005A3E90(_zrdr *anim, int state, DAMAGE_LOCATION loc);
void func_005A3FF0(_zrdr *anim, int state, DAMAGE_LOCATION loc);
}

/* a location's teammate deaths: each list's animations by stance (OVL2 0x5A44F0; SOCOM 1's, the
 * reading of each one moved into a helper). The three readers differ only in their helper and in
 * where each stance's counter is declared: MW gives the later-declared of a list, its count and
 * its counter the lower saved register, and the target's registers rotate from stance to stance
 * (the flinches' prone loop declares its counter in the for, the deaths' before the count) */
void AnimSet::ReadTeammateDeathAnimations(_zrdr *rdr, const char *name, DAMAGE_LOCATION loc)
{
    _zrdr *block;

    if ((block = func_0032F0D0(rdr, name)) == 0)
        return;
    int i;
    int n;
    _zrdr *stand = func_0032F0D0(block, "STAND");

    n = stand->Count();
    for (i = 0; i < n; i++)
        func_005A3FF0(ItemAt(stand, i), stateStand, loc);
    _zrdr *crouch = func_0032F0D0(block, "CROUCH");
    int j;
    int nc = crouch->Count();

    for (j = 0; j < nc; j++)
        func_005A3FF0(ItemAt(crouch, j), stateCrouch, loc);
    _zrdr *prone = func_0032F0D0(block, "PRONE");
    int k;
    int np = prone->Count();

    for (k = 0; k < np; k++)
        func_005A3FF0(ItemAt(prone, k), stateProne, loc);
}

/* a location's deaths (OVL2 0x5A4320; SOCOM 1's, as the teammates') */
void AnimSet::ReadDeathAnimations(_zrdr *rdr, const char *name, DAMAGE_LOCATION loc)
{
    _zrdr *block;

    if ((block = func_0032F0D0(rdr, name)) == 0)
        return;
    int i;
    int n;
    _zrdr *stand = func_0032F0D0(block, "STAND");

    n = stand->Count();
    for (i = 0; i < n; i++)
        func_005A3E90(ItemAt(stand, i), stateStand, loc);
    _zrdr *crouch = func_0032F0D0(block, "CROUCH");
    int j;
    int nc = crouch->Count();

    for (j = 0; j < nc; j++)
        func_005A3E90(ItemAt(crouch, j), stateCrouch, loc);
    _zrdr *prone = func_0032F0D0(block, "PRONE");
    int k;
    int np = prone->Count();

    for (k = 0; k < np; k++)
        func_005A3E90(ItemAt(prone, k), stateProne, loc);
}

/* a location's flinches (OVL2 0x5A4150; SOCOM 1's, as the teammates') */
void AnimSet::ReadFlinchAnimations(_zrdr *rdr, const char *name, DAMAGE_LOCATION loc)
{
    _zrdr *block;

    if ((block = func_0032F0D0(rdr, name)) == 0)
        return;
    int i;
    int n;
    _zrdr *stand = func_0032F0D0(block, "STAND");

    n = stand->Count();
    for (i = 0; i < n; i++)
        func_005A3D30(ItemAt(stand, i), stateStand, loc);
    _zrdr *crouch = func_0032F0D0(block, "CROUCH");
    int j;
    int nc = crouch->Count();

    for (j = 0; j < nc; j++)
        func_005A3D30(ItemAt(crouch, j), stateCrouch, loc);
    _zrdr *prone = func_0032F0D0(block, "PRONE");
    int np = prone->Count();

    for (int k = 0; k < np; k++)
        func_005A3D30(ItemAt(prone, k), stateProne, loc);
}

/* the regions the SEAL stands in: on a change each region's OnEnterRegion anim run, region 0's
 * when it is in none (OVL2 0x5A2F90; SOCOM 1's word for word) */
void CZSealBody::CheckForRegionTrigger(unsigned int regions)
{
    if (m_lastregion != regions) {
        for (int i = 0; i < 31; i++) {
            if ((1 << i) & regions) {
                char name[48];

                sprintf(name, "OnEnterRegion%d", i);
                CZAnim *anim = ZAnim.GetAnim(name, 0);

                if (anim)
                    anim->Activate(0);
            }
        }
        if (!regions) {
            CZAnim *anim = ZAnim.GetAnim("OnEnterRegion0", 0);

            if (anim)
                anim->Activate(0);
        }
    }
    m_lastregion = regions;
}

/* the HUD's flags: the hit taken once, the view cone's update taken once, the hit set (OVL2
 * 0x5A5490, 0x5A5450, 0x5A5430; SOCOM 1's word for word, the flags at 0x105C) */
bool CZSealBody::GetHudHitFlag()
{
    if (m_hudwashit) {
        m_hudwashit = false;
        return true;
    }
    return false;
}

bool CZSealBody::HudUpdateViewCone()
{
    if (m_HudUpdateViewCone) {
        m_HudUpdateViewCone = false;
        return true;
    }
    return false;
}

void CZSealBody::SetHudHitFlag()
{
    m_hudwashit = true;
}

/* the blink stepped: while a look runs it is stepped, and a finished look down turns into the look
 * back up; between blinks the wait runs down to nothing (OVL2 0x5AD730; SOCOM 1's, LookTick and
 * Blink inlined -- Blink's angle is dead on the way back up and MW drops its load). The outer test
 * through an inline accessor makes MW load m_done again for the inlined LookTick's, as the target
 * does; written out, the first load is reused. */
void CBlink::BlinkTick(float dT)
{
    if (!IsDone()) {
        LookTick(dT);
        if (m_done) {
            if (m_org == 1) {
                m_org = 0;
                Blink(0);
                m_done = false;
            } else
                m_org = 0;
        }
    } else {
        m_nextT -= dT;
        if (m_nextT < 0.0f)
            m_nextT = 0.0f;
    }
}

/* the look stepped: the fraction run up to its end, the direction the start's quaternion turned
 * by the eased part of the turn (OVL2 0x5AD920; SOCOM 1's) */
void CLook::LookTick(float dT)
{
    if (!m_done) {
        m_fracT += dT / m_deltaT;
        if (m_fracT >= 1.0f) {
            m_fracT = 1.0f;
            m_done = true;
        }
        CPnt3D v;
        m_cross.Scale(func_002874F0(m_fracT), &v);
        CQuat q;
        q.Exp(&v);
        CQuat r;
        m_quat0.Mul(&q, &r);
        r.Apply(&CPnt3D::unit_nz, &m_curr_dir, 1);
    }
}

/* a blink begun once the last look is done: down by the angle, or back to straight ahead (OVL2
 * 0x5AD870; SOCOM 1's with SetDir's start dropped and its constants made globals) */
void CBlink::Blink(int org)
{
    float rate = data_0044C2DC;
    float angle = data_0044C2E0;

    if (m_done && !m_org) {
        if (org == 1) {
            m_org = 1;
            CQuat q;
            q.MakeXRot(angle);
            CPnt3D dir;
            q.Apply(&CPnt3D::unit_nz, &dir, 1);
            SetDir(&dir, rate);
        } else {
            m_org = -1;
            SetDir(&CPnt3D::unit_nz, rate);
        }
    }
}

/* a defusal asked of the bomb, from nowhere in particular (OVL2 0x59DA80; SOCOM 1's) */
void CZSealBody::AttemptBombDefuse()
{
    CPnt3D pos(0.0f, 0.0f, 0.0f);

    CZBombState::RequestNewBombState(m_id, CZBombState::BOMB_STATE_DEFUSING, &pos);
}

/* a wall climbs if any of four polygon kinds can be crossed (OVL2 0x59D980; SOCOM 1's) */
bool CZSealBody::CanClimbWall()
{
    return CanTraverse((CDIType)3) || CanTraverse((CDIType)4) || CanTraverse((CDIType)1) ||
           CanTraverse((CDIType)5);
}
