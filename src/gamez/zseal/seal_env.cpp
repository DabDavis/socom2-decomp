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
    CLook();
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
    CBlink();
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

extern "C" {
/* the least value at 0x240 the ladder check waits for (OVL1 data, 0.03: a variable, as SOCOM 1's) */
extern float data_003F3428;
}

/* the ladder check taken only for a SEAL this console drives and past the threshold; otherwise
 * passed (OVL2 0x5B10B0; SOCOM 1's took three float references and tested the first, SOCOM II's
 * reads its own member and passes a remote SEAL) */
bool CZSealBody::PostCheckDILadder()
{
    if (IsRemote())
        return true;
    if (m_240 < data_003F3428)
        return true;
    return CheckDILadder();
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

extern "C" {
/* the SEALs' pool of ray intersections, its count and a third word (OVL1 bss; new in SOCOM II) */
extern zdb::DiIntersect **data_0044D588;
extern int data_0044D590;
extern int data_0044D598;
}

/* the intersection pool let go: each deleted, the array freed, the words cleared (OVL2 0x5B61A0, new
 * in SOCOM II; CCoreState::UnInit, CMPExitState::Init and CMission::ReOpen_Unload1 call it) */
extern "C" void func_005B61A0()
{
    for (int i = 0; i < data_0044D590; i++)
        delete data_0044D588[i];
    if (data_0044D588)
        _free(data_0044D588, 0, 0);
    data_0044D588 = 0;
    data_0044D590 = 0;
    data_0044D598 = 0;
}

extern "C" {
/* a pickup taken offline (OVL2 0x59F0C0), told to the network as this console's own or another's
 * (OVL2 0x59E830; OVL1 0x2BA2C0, the net code's) */
void func_0059F0C0(CZSealBody *seal, CPickup *pickup);
void func_0059E830(CZSealBody *seal, int a, unsigned int netid, short *ids, unsigned int id);
void func_002BA2C0(int a, unsigned int netid, short *ids, unsigned int id, unsigned int owner);
}

/* a weapon pickup taken: offline at once; online the weapon's id sent, as this SEAL's own pickup when
 * it owns it, else for its owner (OVL2 0x59F610, new in SOCOM II; PickupObject calls it) */
extern "C" void func_0059F610(CZSealBody *seal, CPickup *pickup)
{
    if (!seal->m_isAlive)
        return;
    if (theNetwork.m_9) {
        unsigned int owner = pickup->m_owner;
        CZWeapon *w = pickup->GetWeapon();

        if (!w)
            return;
        short ids[8];

        ids[0] = ((unsigned char *)w)[0x7c];
        unsigned int id = seal->m_id;

        if (owner == id)
            func_0059E830(seal, 1, pickup->GetPickupNetEntityID(), ids, id);
        else
            func_002BA2C0(1, pickup->GetPickupNetEntityID(), ids, id, owner);
    } else
        func_0059F0C0(seal, pickup);
}

/* whether the SEAL's timed action is due: set at all, and offline at once, online once the network's
 * clock reaches its time (OVL2 0x5A8850, new in SOCOM II; CSealCtrl::PerformProximityActions asks it) */
extern "C" bool func_005A8850(CZSealBody *seal)
{
    bool due = false;

    if (seal->m_eb8) {
        if (theNetwork.m_9) {
            int t = seal->m_22c;

            if (theNetwork.zNetGetTime() >= t)
                due = true;
        } else
            due = true;
    }
    return due;
}

/* (CZWeapon::RemoveWeaponEventCallback is zweapon.h's, through zchar.h) */
extern "C" {
/* the blood decals' vector emptied the FreeVector way, through the unit's out-of-line copies of the
 * class vector's pieces, which follow this function in reverse of their first use (OVL2 0x5A3D10 the
 * temporary's constructor, 0x5A3D00 clear, 0x5A3CC0 swap, 0x5A3C60 the temporary's destructor) */
void *func_005A3D10(void *v);
void func_005A3D00(void *v);
void func_005A3CC0(void *a, void *b);
void func_005A3C60(void *v, int flags);
/* the decal handles' buffers and counts (OVL1 bss; SOCOM 1's freed one buffer and zeroed one count) */
extern void *data_0044D550;
extern void *data_0044D548;
extern int data_0044D508;
extern int data_0044D510;
extern int data_0044D518;
}

/* an entity that is a SEAL (kind 2), or none (the inline cast the walk shows) */
static inline CZSealBody *AsSeal(CEntity *e)
{
    return e && e->m_type == (CEntity::TYPE)2 ? (CZSealBody *)e : 0;
}

/* a weapon's event handed to every SEAL's controller (OVL2 0x5A4F80; SOCOM 1's
 * HandleWeaponEvent__10CZSealBodyFP12CZProjectileUiPv, the callback InitDatabaseHandles registers
 * and UninitDatabaseHandles removes): the entities walked through a const reference, the iterator
 * assigned from begin() -- the target's four copies -- and end() rebuilt each pass */
void CZSealBody::HandleWeaponEvent(CZProjectile *proj, unsigned int event, void *data)
{
    const CEntityList &list = CEntity::m_list;
    CEntityList::const_iterator i;

    for (i = list.begin(); i != list.end(); ++i) {
        CZSealBody *seal = AsSeal(*i);

        if (seal)
            ((CSealCtrl *)seal->m_control)->OnWeaponEvent(proj, event, data);
    }
}

/* a 12-byte vector temporary (the class vector's three words) */
struct CZDecalDefVec {
    unsigned int m_0;
    unsigned int m_4;
    unsigned int m_8;
};

/* the database's handles let go: the buffers freed, the blood decals' vector emptied, the counts
 * cleared, the weapon-event callback removed (OVL2 0x5A3BB0; SOCOM 1's with two more counts) */
void CZSealBody::UninitDatabaseHandles()
{
    if (data_0044D550)
        _free(data_0044D550, 0, 0);
    data_0044D550 = 0;
    CZDecalDefVec tmp;

    func_005A3D10(&tmp);
    func_005A3D00(&m_blooddriptexhandles);
    func_005A3CC0(&m_blooddriptexhandles, &tmp);
    func_005A3C60(&tmp, -1);
    data_0044D508 = 0;
    data_0044D510 = 0;
    data_0044D518 = 0;
    if (data_0044D548)
        _free(data_0044D548, 0, 0);
    data_0044D548 = 0;
    CZWeapon::RemoveWeaponEventCallback(HandleWeaponEvent);
}

/* six values run down at their rates, none below zero (OVL2 0x5AC620, new in SOCOM II: the arrays at
 * 0xFFC and 0x1014; OVL1's vehicle code, 0x3ACC10, calls it) */
extern "C" void func_005AC620(CZSealBody *seal, float dT)
{
    for (int i = 0; i < 6; i++) {
        seal->m_ffc[i] -= seal->m_1014[i] * dT;
        if (seal->m_ffc[i] < 0.0f)
            seal->m_ffc[i] = 0.0f;
    }
}

/* a stun taken: stunned, the kind's highest amount kept, and the controller told with it (OVL2
 * 0x5A53D0; SOCOM 1's without the recovery's switch -- the kind passed on as it came) */
void CZSealBody::HandleStunDamage(STUN_TYPE type, float amount, const CPnt3D *from)
{
    if (type < 8) {
        m_stunned = 1;
        if (m_stunval[type] < amount)
            m_stunval[type] = amount;
        ((CSealCtrl *)m_control)->OnStunDamage(type, m_stunval[type], from);
    }
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

/* the look made: the sequence cleared, looking straight ahead (down -z), done (OVL2 0x5AE450; SOCOM
 * 1's, CZSealBody's constructor calls it for its two looks) */
CLook::CLook()
{
    Clear();
    m_curr_dir.x = 0.0f;
    m_curr_dir.y = 0.0f;
    m_curr_dir.z = -1.0f;
    m_rate = 0.0f;
    m_done = 1;
    m_goal_dir = m_curr_dir;
}

/* the eyelids made: open, the first blink two to eight seconds off (OVL2 0x5AE3B0; SOCOM 1's, the
 * look's constructor inlined). The random fraction written out (a macro's expansion?): through the
 * Rand01 inline the two constants trade registers */
CBlink::CBlink()
{
    m_org = 0;
    m_nextT = 2.0f + 6.0f * (4.656612873e-10f * (float)rand());
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

extern "C" {
extern CNetGameState *data_00437CE8;
/* (OVL1, unowned: a test of a SEAL against the game, both builds' net code; seal.cpp's too) */
int func_002C31D0(const CZSealBody *seal);
}

/* the MP bomb picked up: the bomb told it is carried -- in game mode 1 only by a SEAL the test
 * passes (OVL2 0x59DEF0; SOCOM 1's HandlePickupMPBomb by its place before EnableMPBombGear and its
 * body, without the test) */
void CZSealBody::HandlePickupMPBomb(CPickup *pickup)
{
    if (data_00437CE8->m_mode == 1 && !func_002C31D0(this))
        return;
    CPnt3D pos(0.0f, 0.0f, 0.0f);

    CZBombState::RequestNewBombState(m_id, (CZBombState::EnumBombState)2, &pos);
}

extern "C" {
/* the kit's item at an index let go (OVL2 0x5B9BB0, the kit lane's), the ground under a point
 * within a depth (OVL1 0x2A2EB0), a weapon readied (OVL2 0x57D540, seal_body.cpp's) and the item it
 * takes (OVL1 data 0x3DEB48, a short) */
void func_005B9BB0(CZKit *kit, int index);
zdb::IntersectStruct *func_002A2EB0(const CPnt3D *p, float depth, int chars);
void func_0057D540(CZSealBody *seal, short item, int x);
extern short data_003DEB48;
}

/* the controller's HUD weapon flag set (an inline: a constant true is `daddiu 16`) */
static inline void SetHudUpdateWeapon(CSealCtrl *ctrl, bool on)
{
    ctrl->m_HudUpdateWeapon = on;
}

/* everything dropped: the MP bomb out of the kit and made a pickup on the ground under the SEAL, the
 * satchel gear hidden, a local SEAL's weapon readied again, the HUD told (OVL2 0x59DDB0; SOCOM 1's).
 * The point copied field by field, its height through a reference (the target keeps &pos.y in a
 * saved register); a copy construction loads all three first */
void CZSealBody::DropAllItems()
{
    int i = m_kit.GetItemIndex((EQUIP_ITEM)154);

    if (i != -1)
        func_005B9BB0(&m_kit, i);
    CPnt3D pos;
    float &y = pos.y;

    pos.x = m_posW.x;
    pos.y = m_posW.y;
    pos.z = m_posW.z;

    y += 1.0f;
    zdb::IntersectStruct *hit = func_002A2EB0(&pos, 8.0f, 1);

    if (hit)
        y = hit->m_pos.y;
    CreateMPBombPickup(&pos, false);
    zdb::CNode *gear = FindAuxNode("Satchel", false);

    if (gear)
        gear->SetActive(false);
    if (!IsRemote())
        func_0057D540(this, data_003DEB48, 0);
    SetHudUpdateWeapon((CSealCtrl *)m_control, true);
}

/* the bomb's defusal handled: nothing, in both builds (OVL2 0x59DA70; SOCOM 1's, 8 bytes) */
void CZSealBody::HandleBombDefuse()
{
}

/* a ladder's polygons crossed (OVL2 0x59DA60; SOCOM 1's tail call with type 2) */
int CZSealBody::CanClimbLadder()
{
    return CanTraverse((CDIType)2);
}

/* whether a hit polygon is one a wall climb takes: the four kinds CanClimbWall asks after, read off
 * its application flags (OVL2 0x59D9F0, new in SOCOM II; seal.cpp's 0x5483D0 calls it thrice) */
extern "C" bool func_0059D9F0(zdb::IntersectStruct *hit)
{
    if (!hit || !hit->m_poly)
        return false;
    /* the flags taken as the kind first (narrowed, andi 0xff), then masked: an unsigned char local
     * lets MW drop the narrowing */
    CDIType type = (CDIType)hit->m_poly->m_flags.m_appflags;
    CDIType t = (CDIType)(type & 7);

    if (t == 3 || t == 4 || t == 1 || t == 5)
        return true;
    return false;
}

/* a wall climbs if any of four polygon kinds can be crossed (OVL2 0x59D980; SOCOM 1's) */
bool CZSealBody::CanClimbWall()
{
    return CanTraverse((CDIType)3) || CanTraverse((CDIType)4) || CanTraverse((CDIType)1) ||
           CanTraverse((CDIType)5);
}

/* the anims a jump cannot start from (OVL1 data: an anim type each, its id the first short,
 * filled when the anims load; SOCOM 1's AnimType was an enum) */
extern "C" {
extern short data_003DEB20;
extern short data_003DEB30;
extern short data_003DEBA8;
extern short data_003DEBB8;
extern short data_003DEBC8;
extern short data_003DEBD8;
extern short data_003DEBE0;
extern short data_003DEBE8;
extern short data_003DEBF0;
/* (OVL2 0x5A7DE0: whether the stance leaves room to jump, two flags; the name ours) */
bool func_005A7DE0(CZSealBody *seal, bool a, bool b);
/* the body's current blend (seal.cpp's tail, OVL2 0x555170) */
void *func_00555170(void *stack);
}

/* whether an anim is one a jump cannot start from (an inline bool: the target materialises the
 * test with daddiu/move and branches on it; the second test of 0x3DEBB8 is the source's) */
static inline bool IsNoJumpAnim(short id)
{
    if (id == data_003DEBC8 || id == data_003DEBB8 || id == data_003DEBA8 || id == data_003DEBE8 ||
        id == data_003DEBF0 || id == data_003DEBD8 || id == data_003DEBE0 || id == data_003DEBB8 ||
        id == data_003DEB20 || id == data_003DEB30)
        return true;
    return false;
}

/* whether the SEAL can jump now: alive, not in one of the anims above, room to jump, on the ground
 * (OVL2 0x59D830, new in SOCOM II; CSealCtrl::PreTick and the kit's 0x5BE9A0 ask it) */
extern "C" bool func_0059D830(CZSealBody *seal)
{
    if (!seal->m_isAlive)
        return false;
    if (IsNoJumpAnim(((CZBodyAnimBlend *)func_00555170((char *)seal + 0x1c0))->m_animType))
        return false;
    if (!func_005A7DE0(seal, true, true))
        return false;
    if (seal->m_airborne || seal->m_state == stateInAir)
        return false;
    return true;
}

/* a ladder: its users counted at 0x1c (SOCOM 1's DWARF: CLadderData, 0x28 bytes -- its normal,
 * box and node; SOCOM II counts the SEALs on it) and the mission's list of them (SOCOM 1's
 * CLadderList : vector<CLadderData>) */
class CLadderData {
public:
    unsigned char _0[0x1c];
    int m_users;   // 0x1c (the names ours)
    unsigned int m_owner;   // 0x20: the entity id of the SEAL holding it
    unsigned char _24[0x28 - 0x24];
};

class CLadderList {
public:
    /* the ladder at an index, or none (SOCOM 1's; OVL1 0x2A9210) */
    CLadderData *GetLadderByIndex(int index);
};

/* the mission (theMission, OVL1 bss 0x4364E0) -- only its ladders (SOCOM 1's at 0x580, beside the
 * net game, which SOCOM II's has at 0x1808) */
class CMission {
public:
    unsigned char _0[0x1804];
    CLadderList *m_ladders;   // 0x1804
};

extern CMission theMission;

extern "C" {
/* no ladder: the index a SEAL off every ladder holds (OVL1 data, 65535.0 -- a float variable, read
 * with lwc1 and converted at run time for both the test and the store) */
extern float data_003F3438;
}

/* the SEAL off its ladder: the ladder's count of users dropped and the index cleared (OVL2
 * 0x59D750, new in SOCOM II; OnDeath, PreTick and the destructor call it) */
extern "C" void func_0059D750(CZSealBody *seal)
{
    if (seal->m_ladder != data_003F3438) {
        CLadderData *ladder = theMission.m_ladders->GetLadderByIndex(seal->m_ladder);

        if (ladder)
            ladder->m_users--;
        seal->m_ladder = data_003F3438;
    }
}

/* the SEAL's hold on its ladder let go: the ladder's owner cleared if it is this SEAL, the hold
 * reset (OVL2 0x5B14E0, new in SOCOM II; OnDeath, the destructor and two more call it after 0x59D750
 * with the SEAL's m_climb). The states a switch: its chain tests 3, then 1, then branches past.
 * OPEN, 2 instructions: the target loads the ladder list (a0) right after its lui and the index (a1)
 * in the call's delay slot; MW here loads the index first and the list in the slot. Tried: the list
 * in a pointer local, a reference local, through an inline accessor, the index in a local, the hold
 * through a reference. */
extern "C" void func_005B14E0(CZSealBody::CSealClimb *climb, CZSealBody *seal)
{
    switch (seal->m_climb.m_state) {
    case 1:
    case 3: {
        CLadderData *ladder = theMission.m_ladders->GetLadderByIndex(seal->m_climb.m_ladder);

        if (ladder && ladder->m_owner == seal->m_id)
            ladder->m_owner = CEntity::invalidId;
        break;
    }
    }
    climb->m_state = 0;
    climb->m_active = 0;
}

/* whether the SEAL is on a given ladder (OVL2 0x59D6C0, new in SOCOM II; 0x56BD10 asks it) */
extern "C" bool func_0059D6C0(CZSealBody *seal, CLadderData *ladder)
{
    if (!ladder || seal->m_ladder == data_003F3438)
        return false;
    return theMission.m_ladders->GetLadderByIndex(seal->m_ladder) == ladder;
}

/* a hit's stun, by the item that hit (SOCOM 1's HandleStunFromHit by its place before
 * HandleStunDamage and its tests of items 122, 123, 125 and 127; OVL2 0x5A5250, not written yet:
 * SOCOM II's takes two more arguments) */
extern "C" void func_005A5250(CZSealBody *seal, int item, const CPnt3D *from, int x);

/* in tear gas (item 125): its stun taken and the flag set (OVL2 0x5A0B60; 0x5B5050 calls it) */
extern "C" void func_005A0B60(CZSealBody *seal)
{
    func_005A5250(seal, 125, 0, 0);
    seal->SetInGas(true);
}

/* in smoke (item 122): the same with the smoke flag (OVL2 0x5A0BB0; 0x5B5050 calls it) */
extern "C" void func_005A0BB0(CZSealBody *seal)
{
    func_005A5250(seal, 122, 0, 0);
    seal->SetInSmoke(true);
}

/* the hits under a point handled (SOCOM 1's SealProcessAltitude; OVL2 0x5B5D40, not written yet) */
extern "C" void func_005B5D40(CZSealBody *seal, zdb::DiIntersect *di, CPnt3D *pos, float f);
/* zdb::CWorld::GetAltitudes (OVL1 0x31DEA0): SOCOM II's takes no height -- none of its five callers
 * sets $f12 -- where znode.h declares SOCOM 1's (p, alt); called by its address until the zdb lane
 * drops the float */
extern "C" zdb::DiIntersect *func_0031DEA0(zdb::CWorld *world, const CPnt3D *p);

/* the inline setter of the altitude query's character flag (the stores `(b & 1) << 1`) */
static inline void SetAltitudeCharacters(zdb::DiIntersect *di, bool on)
{
    di->m_AltitudeCharacters = on;
}

/* the ground under a point: the SEAL's node switched off for the query, characters hit when the
 * SEAL asks, both restored after, the hits handled (OVL2 0x5B4F30; SOCOM 1's, which asked for
 * 10000 units down -- SOCOM II's GetAltitudes takes no height). The declarations' order is
 * tools/perm.py's: every local declared first, uninitialised, the hits apart from the world */
void CZSealBody::SealGetAltitude(CPnt3D *pos, float f)
{
    zdb::CNode *node;
    zdb::CWorld *world;
    zdb::DiIntersect *hits;
    bool active;
    zdb::DiIntersect *di;
    bool chars;

    world = zdb::CWorld::m_world;
    node = GetNode();
    active = node->m_active;
    node->SetActive(false);
    di = world->m_diInt;
    chars = di->m_AltitudeCharacters;
    if (m_altitude_characters)
        SetAltitudeCharacters(di, true);
    hits = func_0031DEA0(world, pos);
    SetAltitudeCharacters(di, chars);
    node->SetActive(active);
    func_005B5D40(this, hits, pos, f);
}
