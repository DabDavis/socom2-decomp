/* seal_sensors.cpp -- what an AI SEAL senses: its view cone, the visibility tests, the targets it
 * sees fall (SOCOM 1's unit, __sinit_seal_sensors.cpp: UpdateViewCone, SetViewFOV, ComputeVisibility,
 * RegisterCanSee ... NotifyTargetDown). OVL2 0x5C9400-0x5CDBCF, after seal_kit.cpp's CAiWepHint and
 * before seal_tasks.cpp. So far NotifyTargetDown, a pickup's sighting and the request pool's end:
 * CloseDiPool, OpenDiPool, CleanupDiPool and the free list's members (seal_dipool.h). Defined in
 * reverse of their image order (-inline deferred). */

#include "zseal.h"
#include "seal_dipool.h"

#include <iostream>

/* the unit's statics, in the order its initialiser (__sinit_seal_sensors.cpp, OVL2 0x668E50) builds
 * them after <iostream>'s two objects: SOCOM 1's four debug colours (its symbol table's file statics;
 * OVL2 bss 0x66B760, 0x66B770, 0x66B780, 0x66B790 -- only the initialiser touches them, the drawing
 * compiled out), then SOCOM II's sensing record (0x66B7A0, the names ours): the entity and AI
 * parameters of the controller sensing now -- the sensor ticks set them -- and its eye, worked out on
 * first use through the entity's slot +0x34 (_GetTargetPerception, 0x5CC120); the constructor
 * zeroes the two pointers and the flag */
static CPnt3D _colorViewDir(127.0f, 0.0f, 0.0f);
static CPnt3D _colorEntVisible(0.0f, 127.0f, 0.0f);
static CPnt3D _colorEntNoBlock(0.0f, 0.0f, 127.0f);
static CPnt3D _colorPlayerNoSee(127.0f, 127.0f, 0.0f);

struct CSensing {
    CSensing() : m_entity(0), m_params(0), m_eye_valid(false) {}

    CEntity *m_entity;   // 0x0
    AI_PARAMS *m_params;   // 0x4: the controller's m_aiParams
    CPnt3D m_eye;   // 0x8
    bool m_eye_valid;   // 0x14
};

static CSensing _sensing;

/* what an entity knows of another, or 0 (inline: the ternary inside it keeps the call's result in
 * both arms, `b; move v1,v0` / `move v1,zero`, and pads no label -- written in the caller it pads
 * both, 0x198; the name ours) */
static inline CTarget *TargetOf(CEntity *e, CEntity *of)
{
    return e ? e->m_targets.FindTarget(of) : 0;
}

/* a dying SEAL's requests dropped (CDiPool::Release inlined one level: the list's pieces called out
 * of line, as CloseDiPool's), then each of its targets' entry for it unhooked from its request
 * (SOCOM 1's; OVL2 0x5CC8A0). CTarget's word at 0x28 is the request (zentity.h's m_unk28). The
 * targets through a reference local keep the vector's address in s1; the request's value declared
 * before its address gives the target's a2/a1 (a reference to the field, or the address first, swaps
 * them) */
void CleanupDiPool(CZSealBody *seal)
{
    _diPool.Release(seal);
    CTargetList &targets = seal->m_targets;

    for (CTargetList::iterator t = targets.begin(); t != targets.end(); t++) {
        CTarget *found = TargetOf(t->m_ent, seal);

        if (found) {
            CDiPool::dir *d = (CDiPool::dir *)found->m_unk28;
            CDiPool::dir **p = (CDiPool::dir **)&found->m_unk28;

            if (d)
                d->Drop();
            *p = 0;
        }
    }
}

/* (SOCOM 1's, empty in both; OVL2 0x5CC890) */
void CSealCtrlAi::OpenDiPool()
{
}

/* the pool emptied at the mission's end (SOCOM 1's; OVL2 0x5CC5F0) */
void CSealCtrlAi::CloseDiPool()
{
    _diPool.Close();
}

/* a weapon's item (zweapon.h's CZWeapon::m_ID at 0x7C, a byte enum: copied with lb, compared with
 * lbu -- zseal.h declares CZWeapon only by name) */
struct CSensorWeaponView {
    unsigned char _0[0x7c];
    EQUIP_ITEM m_ID;   // 0x7c
};

/* a bit of a CBits set (inlined; zmath.h's CBits has Test and Clear only -- the name ours) */
inline void SetSensorBit(CBits *bits, unsigned int i)
{
    bits->m_vec[i >> 5] |= 1 << (i & 31);
}

/* and tested, the mask first (the target builds it before the word's address, as the set does --
 * CBits::Test indexes first; the name ours). The mask a local ANDed in place: MW folds a mask local
 * read once back into the expression and then evaluates the word's index first, whatever the order
 * written (13 spellings, inlined helpers for either side included); `m &= word` (or `m = m & word`)
 * keeps the mask first and every register but the result's (the kit's round 35: 8 -> 2) */
inline bool TestSensorBit(const CBits *bits, unsigned int i)
{
    unsigned int m = 1 << (i & 31);

    m &= bits->m_vec[i >> 5];
    return m;
}

/* a weapon pickup seen: unless the AI already goes for a pickup or this SEAL has marked it, a Navy
 * SEAL goes for item 152 when its kit has none of it or none with ammunition (the pickup state
 * pushed and handed it, flag 1); any other item is marked seen by this SEAL. Whether it went
 * (OVL2 0x5CB450: 0x5CB870 asks it; new in SOCOM II -- the name ours). `go` declared first and
 * assigned after the early return: the target numbers it before the SEAL (s2 to its s1).
 * OPEN, 2 instructions, the seen-bit test's AND only: the target's `and v0,a0,v0` puts the result in
 * the word's register, TestSensorBit's `m &= word` in the mask's (`and a0,a0,v0`, the branch on a0).
 * Every other way round MW evaluates the word's index first (8: CBits::Test, the mask first or last,
 * in a local, the word's address in a local, `(w >> s) & 1`, inlined mask or word helpers, a
 * reference to the word); `m = 1; m <<= i & 31; return m & word;` puts the AND in v0 but swaps the 1
 * and the shift amount (4). 0x5CB630 below shares it. Round 39: 23 more shapes (the word or the
 * result a local, a copy of the mask, `!= 0`, a ternary, the shift as its own statement with the
 * amount a local before or after, a dead first definition, the index a local) all stay at 2 or
 * move to 4-10 -- the `m = 1; m <<= s;` forms put the AND in v0 but always trade the 1 and the
 * amount (CAiSPickup::Tick's SetBit has the same pair, its igdump reading in its note). Round 40:
 * the AND inside an inline taking the word and the mask by value (either order, `& `, `!= 0`, a
 * ternary, the word by pointer) gives 8 -- the word's index computed first again. Round 41: `w = m & w`
 * with the word a local (declared first or after the mask, a dead first definition of the mask) gives
 * 10, a copy of `m &= word` returned 2, the shot-at state's mask-parameter inline (TestBits, either
 * argument order) 8. Round 43, MW's colouring replayed (tools/igreplay.py on the `return (1 << (i & 31))
 * & word` variant, `--move` of its andi, 1 and sllv below the word's address chain gives every target
 * register): the target numbers the mask chain first and ANDs into a fresh temporary -- `m &=` keeps the
 * order but writes the mask's own vreg (no numbering splits them), and every fresh-result shape (30
 * more: `% 32` / `/ 32` trees, inline mask helpers, dead, self and `|= 0` definitions, a const or bool
 * local, casts hiding the load, `1UL`) propagates the mask back and computes the word's address first
 * (8); `m = 1; m <<= s` forms keep a fresh result but number the 1 before the amount (4) */
extern "C" bool func_005CB450(CSealCtrlAi *ai, CPickup *p, CZWeapon *w)
{
    bool go;
    CZSealBody *seal = (CZSealBody *)ai->m_entity;

    if (ai->AiStateIs(AI_STATE_PICKUP) || TestSensorBit((CBits *)p->m_bits, seal->m_index))
        return false;
    go = false;
    CZKit *kit = &seal->m_kit;

    if (ai->IsNavySeal() && kit) {
        EQUIP_ITEM item = ((CSensorWeaponView *)w)->m_ID;
        int i = kit->GetItemIndex(item);

        switch (item) {
        case 152:
            if (i == -1 || !kit->ItemHasAmmo(i))
                go = true;
            break;
        default:
            SetSensorBit((CBits *)p->m_bits, seal->m_index);
            break;
        }
    }
    if (go && ai->PushAiState(AI_STATE_PICKUP, 0)) {
        CAiState *s = ai->GetAiState();
        CAiSPickup *pick = s && s->m_state == AI_STATE_PICKUP ? (CAiSPickup *)s : 0;

        pick->SetPickup(p, 1);
    }
    return go;
}

/* an ammunition pickup's contents: the weapons' stash (zwep_weapon.cpp's CZItemStash, three parallel
 * lists of words -- a weapon, an ammunition and a count per entry -- out of line in OVL1; only what
 * the scan calls) and the pickup's view of it (zseal.h's CPickupAmmo; seal_env.cpp's spelling) */
class CZItemStash {
public:
    int Count();
    bool Get(int i, int *a, int *b, int *c);
};

class CPickupAmmo : public CZItemStash {
};

/* an ammunition pickup seen: unless the AI already goes for a pickup or this SEAL has marked it, a
 * Navy SEAL goes for it when it holds item 152 and the kit has none of that or none with ammunition
 * (the pickup state pushed and handed it, flag 1); every other entry marks it seen by this SEAL.
 * Whether it went (OVL2 0x5CB630: 0x5CB450's sibling for the stash, new in SOCOM II -- the name ours).
 * OPEN, 2 instructions: the seen-bit test's AND, 0x5CB450's (TestSensorBit's note) */
extern "C" bool func_005CB630(CSealCtrlAi *ai, CPickup *p, CPickupAmmo *a)
{
    CZSealBody *seal = (CZSealBody *)ai->m_entity;

    if (ai->AiStateIs(AI_STATE_PICKUP) || TestSensorBit((CBits *)p->m_bits, seal->m_index))
        return false;
    bool go = false;
    CZKit *kit = &seal->m_kit;
    int n = a->Count();

    for (int i = 0; i < n; i++) {
        int w = 0;
        int b = 0;
        int c = 0;

        a->Get(i, &w, &b, &c);
        EQUIP_ITEM item = w ? ((CSensorWeaponView *)w)->m_ID : (EQUIP_ITEM)255;

        if (item == 152 && ai->IsNavySeal()) {
            int k = kit->GetItemIndex(item);

            if (k == -1 || !kit->ItemHasAmmo(k))
                go = true;
        } else
            SetSensorBit((CBits *)p->m_bits, seal->m_index);
    }
    if (go && ai->PushAiState(AI_STATE_PICKUP, 0)) {
        CAiState *s = ai->GetAiState();
        CAiSPickup *pick = s && s->m_state == AI_STATE_PICKUP ? (CAiSPickup *)s : 0;

        pick->SetPickup(p, 1);
    }
    return go;
}

/* a thrown grenade's pickup seen (OVL2 0x5CAC00) -- unwritten */
extern "C" bool func_005CAC00(CSealCtrlAi *ai, CPickup *p, CZProjectile *proj);

/* a projectile's fuse (zweapon.h's CZProjectile, which zseal.h declares only by name) */
struct CSensorProjView {
    unsigned char _0[0x8c];
    float m_fuse;   // 0x8c
};

/* a grenade-like pickup's projectile, or none (inline: the null test of the pickup kept) */
static inline CZProjectile *PickupProjectile(CPickup *p)
{
    return p ? (CZProjectile *)p->m_data : 0;
}

/* the pickup scan's view of the SEAL's team */
struct CSensorTeamView {
    unsigned char _0[0xc8];
    unsigned int m_team;   // 0xc8
};

/* the pickups within 200 of the SEAL looked at in turn until one is taken up: a weapon (0x5CB450),
 * ammunition (0x5CB630), or a grenade-like pickup whose fuse has 16 or less to run, when the AI may
 * push the grenade state and -- breaching -- its thrower is no team-mate (0x5CAC00) (OVL2 0x5CB870:
 * the pickup loop SOCOM 1's CSealCtrlAi::SensorPreTick held inline, split out; its one caller
 * 0x5CBA20 -- the name ours). The pickup's dead first definition numbers it before the flag (the
 * target's s3 to its s2); the team read through a view, so MW keeps its address in a saved register
 * across the loop as the target does (s0: a member read in place folds its offset into the load) */
extern "C" void func_005CB870(CSealCtrlAi *ai)
{
    static float range = 200.0f;
    static float fuse = 16.0f;
    CZSealBody *seal = (CZSealBody *)ai->m_entity;
    CPickup::iterator it = CPickup::GetFirst();
    CPickup *p = 0;
    bool found = false;

    while (!found && (p = CPickup::GetNext(it, &seal->m_posW, range, CPickup::PU_NONE)) != 0) {
        switch (p->m_type) {
        case CPickup::PU_TYPE_4:
        case CPickup::PU_TYPE_5:
        case CPickup::PU_TYPE_1:
            if (ai->CanPushAiState(AI_STATE_GRENADE, 0)) {
                CZProjectile *proj = PickupProjectile(p);

                if (proj && ((CSensorProjView *)proj)->m_fuse <= fuse) {
                    if (ai->m_stateStack.m_10 & (1 << AI_STATE_BREACH)) {
                        CEntity *thrower = CEntity::m_list.GetEntityById(p->m_owner);

                        if (thrower && (((CSensorTeamView *)seal)->m_team & thrower->m_team))
                            break;
                    }
                    found = func_005CAC00(ai, p, proj);
                }
            }
            break;
        case CPickup::PU_AMMO: {
            CPickupAmmo *a = p->GetAmmo();

            if (a)
                found = func_005CB630(ai, p, a);
            break;
        }
        case CPickup::PU_WEAPON: {
            CZWeapon *w = p->GetWeapon();

            if (w)
                found = func_005CB450(ai, p, w);
            break;
        }
        }
    }
}

/* how fast an unperceived target's visibility falls (OVL2 data 0x6509E0, read by both
 * SensorPreTicks: a static, the name ours) */
static float _visDecay = -0.15f;

/* a target's visibility (inline: the decay's products take the other operand first) */
static inline float Vis(const CTarget *t)
{
    return t->m_visibility;
}

/* the targets' frame: the sensing record set to this controller (its eye to be worked out again),
 * a point ahead of the SEAL by its velocity worked out and left unused (new in SOCOM II), then each
 * live target within the view range -- the list runs nearest first, so the first beyond it ends the
 * walk -- that the SEAL does not perceive fades: its visibility falls by a rate the visibility sets,
 * its request dropped, its sighting ended (OVL2 0x5CA9F0, CSealCtrl::PreTick calls it; SOCOM 1's
 * SensorPreTick, which saturated a rise instead). The point a two-point local ASSIGNED (its tail
 * and tip: two separate points, or one initialised, drop the operators' return copies -- 0x1E8);
 * the perception into a bool local (the target's andi); the rate a local and the visibility read
 * through Vis(), the products' operands then the target's */
void CSealCtrl::SensorPreTick(float dT)
{
    static float lead = 1.0f;
    CEntity *e = m_entity;
    CTargetList &targets = e->m_targets;

    _sensing.m_entity = e;
    _sensing.m_params = &m_aiParams;
    _sensing.m_eye_valid = false;
    float range = m_visrange.m_value * m_visrange.m_value;
    CPnt3D vel = e->m_velW;
    struct { CPnt3D tail, tip; } ray;
    float speed = vel.Normalize();

    ray.tail = e->m_posW;
    ray.tip = ray.tail + vel * (speed * lead);

    for (CTargetList::iterator t = targets.begin(); t != targets.end(); t++) {
        if (!t->m_ent || !t->m_ent->m_isAlive)
            continue;
        if (!(t->m_distSq <= range))
            return;
        bool seen = _GetTargetPerception(*t);

        if (seen)
            continue;
        float rate = _visDecay * Vis(&*t);

        t->m_aware = rate;
        t->m_visibility = rate * dT + Vis(&*t);
        CDiPool::dir *d = (CDiPool::dir *)t->m_unk28;

        if (d)
            d->Drop();
        t->m_unk28 = 0;
        OnSetTargetVisible(*t, false, 0);
    }
}

/* the view cone as the controller keeps it (SOCOM 1's DWARF: CSealCtrl's m_fov, m_fovtan, m_fovcos,
 * m_view_cone -- a CAiCone, 0x38 bytes -- and m_visrange at 0x64-0xA8, the same offsets in SOCOM II;
 * zseal.h's CSealCtrl pads them: a view) */
struct CSensorCone {
    float m_dot;   // 0x0
    CPnt3D vec;   // 0x4
    float m_height;   // 0x10
    float m_radius;   // 0x14
    float m_point_radius;   // 0x18
    float m_inv_height;   // 0x1c
    CPnt3D m_cone_pos;   // 0x20
    CPnt3D m_cone_dir;   // 0x2c

    /* whether a point lies in the cone (SOCOM 1's CAiCone::ContainsPoint, out of line there and
     * inlined here): the vector to it and its depth along the cone kept */
    bool ContainsPoint(const CPnt3D *p)
    {
        p->Sub(&m_cone_pos, &vec);
        m_dot = m_cone_dir.x * vec.x + m_cone_dir.y * vec.y + m_cone_dir.z * vec.z;
        if (m_dot <= 0.0f || m_dot > m_height)
            return false;
        float r = m_point_radius + m_dot * m_inv_height * m_radius;

        if (vec.z * vec.z + (vec.x * vec.x + vec.y * vec.y) - m_dot * m_dot <= r * r)
            return true;
        return false;
    }
    /* the same with the depth worked out in a local and stored once (_GetTargetPerception's: the
     * caller then reads the stored depth again) */
    bool Contains(const CPnt3D *p)
    {
        p->Sub(&m_cone_pos, &vec);
        float dot = m_cone_dir.x * vec.x + m_cone_dir.y * vec.y + m_cone_dir.z * vec.z;

        m_dot = dot;
        if (dot <= 0.0f || dot > m_height)
            return false;
        float r = m_point_radius + dot * m_inv_height * m_radius;

        if (vec.z * vec.z + (vec.x * vec.x + vec.y * vec.y) - dot * dot <= r * r)
            return true;
        return false;
    }
};

/* (SOCOM 1's CAiDecay, its value read through an inline: the cone's products load it before the
 * other factor) */
struct CSensorDecay {
    float Get() const { return m_value; }

    float m_value;   // 0x0
    float m_goal;   // 0x4
    float m_8;   // 0x8
};

struct CSensorConeCtrl {
    unsigned char _0[0x64];
    float m_fov;   // 0x64
    float m_fovtan;   // 0x68
    float m_fovcos;   // 0x6c
    CSensorCone m_view_cone;   // 0x70
    CSensorDecay m_visrange;   // 0xa8
};

/* the AI dozing (SOCOM 1's m_snoozing, bit 9), and neither dozing nor asleep (the names ours) */
static inline bool IsSnoozing(const CSealCtrlAi *ai)
{
    return ai->m_aibit9;
}

static inline bool IsAwake(const CSealCtrlAi *ai)
{
    return !IsSnoozing(ai) && !ai->IsSleeping();
}

/* the doze bits as bools (seal_ai.cpp's SetSnoozing reads them so: `? true : false`, sltu) */
static inline bool Snoozing(const CSealCtrlAi *ai)
{
    return ai->m_aibit9 ? true : false;
}

static inline bool NoSnooze(const CSealCtrlAi *ai)
{
    return ai->m_nosnooze ? true : false;
}

/* neither dozing nor asleep, the either-or materialised and negated (an inline, the name ours) */
static inline bool NotDozing(const CSealCtrlAi *ai)
{
    bool r = Snoozing(ai);

    if (!r)
        r = ai->IsSleeping();
    return r == false;
}

/* the world's camera and its far plane (zdb::CWorld holds it at 0x128, the plane at 0xEC:
 * seal_control.cpp's view) */
struct CSensorCamera {
    unsigned char _0[0xec];
    float m_far;   // 0xec
};

struct CSensorWorld {
    unsigned char _0[0x128];
    CSensorCamera *m_camera;   // 0x128
};

/* a turret's flag word (zveh.h's CTurret::m_flags at 0x3A4; only that) and its bits 1 and 4 (inlines,
 * the names ours) */
struct CSensorTurret {
    bool Bit2() const { return m_flags & 2; }
    bool Bit10() const { return m_flags & 0x10; }

    unsigned char _0[0x3a4];
    unsigned int m_flags;   // 0x3a4
};

/* the entity as a turret, or as a SEAL, if it is one (inlines, the names ours): the kind tested `!=` with
 * a return in each arm lays out the target's `beq` to the entity's arm after the zero's -- a ternary
 * (`==` or `!=`, MW turns the one into the other) puts the entity's arm first */
static inline CSensorTurret *AsSensorTurret(CEntity *e)
{
    if (e) {
        if (e->m_type != CEntity::ENT_TYPE_TURRET)
            return 0;
        return (CSensorTurret *)e;
    }
    return 0;
}

static inline CZSealBody *AsSensorSeal(CEntity *e)
{
    if (e) {
        if (e->m_type != CEntity::ENT_TYPE_SEAL)
            return 0;
        return (CZSealBody *)e;
    }
    return 0;
}

/* whether an entity is of a kind there is to perceive: a living SEAL; a living turret flagged 2 and
 * not 0x10; a living kind 4 (a vehicle) by a player only (an inline, the name ours) */
static inline bool Perceivable(CEntity *e, bool ai)
{
    bool r = false;

    if (e && e->m_isAlive) {
        if (e->m_type == CEntity::ENT_TYPE_SEAL)
            r = true;
        else if (e->m_type == CEntity::ENT_TYPE_TURRET) {
            CSensorTurret *t = AsSensorTurret(e);

            r = t->Bit2();
            if (r)
                r = !t->Bit10();
        }
        else if (e->m_type == 4)
            r = !ai;
    }
    return r;
}

/* whether the controller perceives a target: one there is, an enemy unless a player looks, of a kind
 * there is to perceive (an AI SEAL dozing or asleep is not), within the view range (the camera's
 * far plane at most); in the view cone, or ahead and moving faster than 50, or -- a Navy SEAL's --
 * behind and nearer than 100. A target perceived gets a ray from the sensing record's eye (the
 * sensing entity's head, worked out on first use) to its neck requested of the DI pool (OVL2
 * 0x5CC120: SOCOM 1's, which kept the speed's square a guarded function-local static and asked no
 * kind). RequestRay is seal_dipool.h's (the turret's twin's): cut to one level, its get and set-up
 * are the OVL2 copies 0x5C9830 / 0x5C97D0, byte-exact from here (`ovl-copy:` of OVL1's 0x3A7E30 /
 * 0x3A7DD0), with the list's push_back 0x5C97A0 and CPnt3D's assignment 0x539E90.
 * OPEN, 230 instructions, at the target's size (0x4D0), the kit's round 51. Right: the team test on
 * the masks in place (IsEnemy() materialises its bool), the kinds an if-else chain 2, 3, 4 (a switch
 * branches `beq` to each case), the two casts' `beq` to the entity's arm after the zero's (the kind
 * tested `!=` with a return in each arm: a ternary, `==` or `!=`, puts the entity's arm first or
 * joins it at an inner label), the cone through a named controller local (through the cast MW keeps
 * `&m_view_cone` in a saved register) with the depth a local stored once (the caller re-reads it),
 * the two thresholds function-local statics (data 0x6509E8 / 0x6509F0: literals are built with
 * lui / ori). Left: (1) the registers -- the target numbers this s5, the target s4, the entity s1,
 * self s0, the result s3, the sighting s2; (2) the casts' join labels are padded here (a nop before
 * the entity's arm and the join: statement labels inside an inline are aligned, the target's are
 * not -- an expression-level form with this block order not found); (3) the turret's first bit
 * lands straight in the result (`sltu a0`) where the target computes it in v0 and narrows it into
 * a0 in the branch's delay slot; (4) the doze test: the target materialises `snoozing ? 1 :
 * sleeping` in v0 and negates it with one xori at the join -- a bool local renormalises it (an extra
 * sltu), `||` sets it with daddiu, `== false` folds into the branch (v_doze*.py: 19 forms; the
 * SEALs' round-49 0x569E40 note has the same pair); (5) the radius's product order with the depth a
 * local (`radius * (depth * inv)`; the member form gives the target's) */
bool CSealCtrl::_GetTargetPerception(CTarget &target)
{
    CEntity *ent = target.m_ent;
    CEntity *self = m_entity;
    bool perceived = false;

    if (!ent)
        return false;
    if (!IsPlayer() && (ent->m_team & self->m_team))
        return false;
    if (!Perceivable(ent, IsAi()))
        return false;
    CZSealBody *seal = AsSensorSeal(ent);

    if (seal) {
        CSealCtrlAi *ai = seal->GetSealCtrlAi();

        if (ai && !NotDozing(ai))
            return false;
    }
    float range = m_visrange.m_value * m_visrange.m_value;
    float dist = target.m_distSq;
    CSensorCamera *cam = ((CSensorWorld *)zdb::CWorld::m_world)->m_camera;

    if (cam) {
        float far = cam->m_far * cam->m_far;

        if (far < range)
            range = far;
    }
    if (!(dist <= range))
        return false;
    static float fastSq = 2500.0f;
    static float nearSq = 10000.0f;
    CPnt3D pos;
    CSensorConeCtrl *cc = (CSensorConeCtrl *)this;

    ent->GetNeckPosW(&pos);
    bool seen = cc->m_view_cone.Contains(&pos);

    if (!seen) {
        if (cc->m_view_cone.m_dot > 0.0f) {
            if (ent->m_velW.x * ent->m_velW.x + ent->m_velW.z * ent->m_velW.z > fastSq)
                seen = true;
        } else if (dist < nearSq && GetSealCtrlAi() && GetSealCtrlAi()->IsNavySeal())
            seen = true;
    }
    if (seen) {
        zdb::CNode *node = ent->GetNode();

        if (!_sensing.m_eye_valid) {
            _sensing.m_entity->GetHeadPosW(&_sensing.m_eye);
            _sensing.m_eye_valid = true;
        }
        RequestRay(target.m_unk28, _sensing.m_eye, pos, self, node, ent);
        perceived = true;
    }
    return perceived;
}

/* the snoozer's counts (SOCOM 1's CSealCtrlAi statics aiActiveCount, aiSleepingCount and
 * aiSnoozingCount -- its $gp words at -28316/-28312/-28308, cleared at its pre-tick's end as here;
 * OVL1 bss 0x44D4B8, 0x44D4C0, 0x44D4C8), its clock (SOCOM 1's ai::snooze_time, $gp -28328 there;
 * OVL2 bss 0x66AFF0), the online test (seal_command.cpp's 0x5750A0) */
namespace ai {
extern float snooze_time;
}

/* the mission's limits on awake AIs (theMission at OVL1 0x4364E0: the words at 0x178 and 0x17C --
 * new in SOCOM II, the names ours) */
class CMission {
public:
    unsigned char _0[0x178];
    int m_ai_active_limit;   // 0x178
    int m_ai_unsnoozable_limit;   // 0x17c
};

extern CMission theMission;

int func_005750A0(void);
CZSealBody *ftsGetPlayer();

/* the scripted groups, ticked once a frame (ai_group.cpp's; OVL2 0x60E590) */
class CAiGroup {
public:
    static void TickAll(float dT);
};

/* an AI and how far it is from the player's party (the snoozer's sorted lists; new in SOCOM II --
 * the names ours) */
struct CSnoozeRec {
    CSnoozeRec(CSealCtrlAi *ai, float dist) : m_ai(ai), m_dist(dist) {}

    CSealCtrlAi *m_ai;   // 0x0
    float m_dist;   // 0x4
};

/* the snoozer's list: the value put in before a position through a member of its own, the
 * position copied on into the base's insert (out of line at OVL2 0x5CD8F0; the class ours, as
 * seal_dipool.h's CDiList -- include/list's list<T> has no insert of its own) */
class CSnoozeList : public std::list<CSnoozeRec> {
public:
    iterator insert(iterator position, const CSnoozeRec &x) { return base_list::insert(position, x); }

private:
    typedef std::__list_imp<CSnoozeRec, std::allocator<CSnoozeRec> > base_list;
};

/* a band around a point: a SEAL inside it (in the XZ radius, between the heights) stays as it is
 * (SOCOM 1's pre-tick tested the band in line; the names ours) */
struct CSnoozeZone {
    CSnoozeZone(float radius_sq, float max_dy, float min_dy)
    {
        m_valid = false;
        m_radius_sq = radius_sq;
        m_max_dy = max_dy;
        m_min_dy = min_dy;
        m_result = 0;
    }
    void SetCenter(const CPnt3D *p)
    {
        if (p) {
            m_center = *p;
            m_valid = true;
        } else
            m_valid = false;
    }
    /* 1 below the band, 2 above it, 3 inside its radius, 4 outside it, 0 with no centre */
    unsigned char Classify(const CPnt3D *pos)
    {
        int r = 0;

        if (m_valid) {
            CPnt3D d;

            pos->SubXZ(&m_center, &d);
            m_dist_sq = d.x * d.x + d.z * d.z;
            m_dy = pos->y - m_center.y;
            if (m_dy < m_min_dy)
                r = 1;
            else if (m_dy > m_max_dy)
                r = 2;
            else if (m_dist_sq < m_radius_sq)
                r = 3;
            else
                r = 4;
        }
        m_result = r;
        return m_result;
    }

    float m_dist_sq;   // 0x0
    float m_dy;   // 0x4
    unsigned char m_result;   // 0x8
    bool m_valid;   // 0x9
    float m_max_dy;   // 0xc
    float m_min_dy;   // 0x10
    float m_radius_sq;   // 0x14
    CPnt3D m_center;   // 0x18
};

/* the XZ distance squared from a point (the name ours) */
static inline float DistSqXZ(const CPnt3D *a, const CPnt3D *b)
{
    CPnt3D d;

    a->SubXZ(b, &d);
    return d.x * d.x + d.z * d.z;
}

/* the nearer of two distances (by value: the inline's copies are taken by address) */
static inline float NearerSq(float a, float b)
{
    return ai::_min(a, b);
}

/* an AI put into a list in order of its distance, the nearest first (the name ours) */
static inline void InsertSorted(CSnoozeList &l, CSealCtrlAi *ai, float d)
{
    for (CSnoozeList::iterator i = l.begin(); i != l.end(); i++) {
        if (!(d < (*i).m_dist))
            continue;
        l.insert(i, CSnoozeRec(ai, d));
        return;
    }
    l.push_back(CSnoozeRec(ai, d));
}

/* the "ai_pre_tick" task (CCoreState::Init adds it at 0.9): the awareness counted again after a
 * pause, then once a second (unless online) the AIs among the player's targets sorted -- the awake
 * ones that may doze and are neither seen by the player nor near him or his squad's leader, the
 * snoozing ones far from both -- and as many woken, nearest first, as the mission's limit allows,
 * the farthest awake ones put to sleep past it; the counts cleared, the groups ticked (SOCOM 1's
 * CSealCtrlAi::PreTick(float, void *), the same statics; SOCOM II sorts by distance; OVL2 0x5CCCD0).
 * Its pieces 0x5CD870-0x5CDBB0 are byte-exact from it (the kit's round 46): the sorted insert is an
 * inline past inline_max_size -- with the list's own insert one wrapper level more than include/list
 * gives it (the class above), 200-250 units without -- so it comes in one level, its list calls out
 * of line as the target's; the snooze tests are the target's with the doze bit read once through a
 * plain inline (andi) and the && materialised (IsAwake); `recount--;` then its own test reloads it.
 * OPEN, the target's size and 743 instructions, 28 short runs apart (registers and frame slots
 * otherwise -- the locals sit 16 bytes lower: the target spills &m_cone_pos (sp+192) and the awake
 * list's &end_.next_ (sp+176), keeps &player->m_targets in s7 and each Classify's point in a register
 * of its own (s6, then s2), where MW here propagates `&seal->m_posW` and keeps the player): the
 * no-snooze test and the else-if's doze test want `? true : false` (sltu) -- written so (NoSnooze,
 * Snoozing) they read the byte through a kept address (5 more runs); the targets reference declared
 * after the cone, before it or through a pointer: 0xB94 */
bool CSealCtrlAi::PreTick(float dT, void *data)
{
    static float snooze_rate = 1.0f;
    static float snoozer_sleep_radius_sq = 160000.0f;
    static float snoozer_wakeup_radius_sq = 40000.0f;
    static float snoozer_min_dy = -100.0f;
    static float snoozer_max_dy = 300.0f;
    static int recount;
    static int unsnoozable;
    int limit = theMission.m_ai_active_limit;

    if (aiEnabled) {
        if (recount > 0) {
            recount--;
            if (recount == 0)
                CEntity::CountAwareness();
        }
        ai::snooze_time += dT;
        if (!(ai::snooze_time <= snooze_rate) && !func_005750A0()) {
            ai::snooze_time = 0.0f;
            unsnoozable = 0;
            CZSealBody *player = ftsGetPlayer();
            CZSealBody *leader = CSealUnit::GetUnitByTeam(3)->GetLeader();
            CSensorConeCtrl *cone = (CSensorConeCtrl *)player->m_control;
            CPnt3D pos = player->m_posW;
            CSnoozeZone near_player(snoozer_sleep_radius_sq, snoozer_max_dy, snoozer_min_dy);

            near_player.SetCenter(&pos);
            const CPnt3D *lpos = leader ? &leader->m_posW : 0;
            CSnoozeZone near_leader(snoozer_sleep_radius_sq, snoozer_max_dy, snoozer_min_dy);

            near_leader.SetCenter(lpos);
            CSnoozeList awake;
            CSnoozeList snoozing;
            int active = 0;
            int queued = 0;
            CTargetList &targets = player->m_targets;

            for (CTargetList::iterator t = targets.begin(); t != targets.end(); t++) {
                CEntity *e = t->m_ent;
                CZSealBody *seal = e && e->m_type == CEntity::ENT_TYPE_SEAL ? (CZSealBody *)e : 0;

                if (!seal)
                    continue;
                if (!(seal->m_control ? ((CSealCtrl *)seal->m_control)->IsAi() : false) || !seal->m_isAlive)
                    continue;
                CSealCtrlAi *ai = seal->GetSealCtrlAi();

                if (IsAwake(ai)) {
                    active++;
                    if (ai->m_nosnooze || ai->m_3f4 == 7 || ai->GetGroup()) {
                        unsnoozable++;
                        continue;
                    }
                    if (t->m_visibility > 0.0f && cone->m_view_cone.ContainsPoint(&seal->m_posW))
                        continue;
                    if (near_player.Classify(&seal->m_posW) == 3)
                        continue;
                    if (near_leader.Classify(&seal->m_posW) == 3)
                        continue;
                    float d = near_leader.m_valid ? NearerSq(near_player.m_dist_sq, near_leader.m_dist_sq)
                                                  : near_player.m_dist_sq;

                    InsertSorted(awake, ai, d);
                    queued++;
                } else if (IsSnoozing(ai)) {
                    if (cone->m_view_cone.ContainsPoint(&seal->m_posW))
                        continue;
                    float d = DistSqXZ(&near_player.m_center, &seal->m_posW);

                    if (near_leader.m_valid) {
                        float dl = DistSqXZ(&near_leader.m_center, &seal->m_posW);

                        d = ai::_min(d, dl);
                    }
                    if (d <= snoozer_wakeup_radius_sq)
                        continue;
                    InsertSorted(snoozing, ai, d);
                }
            }
            if (awake.size() || snoozing.size()) {
                if (theMission.m_ai_unsnoozable_limit < unsnoozable)
                    limit += theMission.m_ai_unsnoozable_limit - unsnoozable;
                int count = active - queued;
                CSnoozeList::iterator a = awake.begin();

                for (CSnoozeList::iterator s = snoozing.begin(); s != snoozing.end(); ++s) {
                    CSealCtrlAi *ai = (*s).m_ai;
                    CSnoozeRec &r = *s;

                    for (; a != awake.end(); a = awake.begin()) {
                        if (!((*a).m_dist < r.m_dist))
                            break;
                        awake.erase(awake.begin());
                        queued--;
                        count++;
                    }
                    if (!(count < limit))
                        break;
                    ai->SetSnoozing(false);
                    if (!Snoozing(ai))
                        count++;
                }
                count += queued;
                std::reverse_iterator<CSnoozeList::iterator> r;

                for (r = std::reverse_iterator<CSnoozeList::iterator>(awake.end());
                     r != std::reverse_iterator<CSnoozeList::iterator>(awake.begin()); ++r) {
                    if (!(limit < count))
                        break;
                    CSealCtrlAi *ai = (*r).m_ai;

                    ai->SetSnoozing(true);
                    if (Snoozing(ai))
                        count--;
                }
            }
            recount = 3;
        }
    }
    aiSnoozingCount = 0;
    aiSleepingCount = 0;
    aiActiveCount = 0;
    CAiGroup::TickAll(dT);
    return false;
}

/* the "ai_posttick" task (CCoreState::Init adds it at 10.7, after target_post_tick): three slices
 * (zseal.h's CAiSlice) moved on over the entity list, by the function's three static sizes
 * (8-byte-spaced data in definition order: 6, 5, 1) (OVL2 0x5CCA30, new in SOCOM II -- SOCOM 1's
 * ai_post_tick was the
 * pool's post-tick, ai_target.cpp's 0x60B8F0 here; the name unknown).
 * OPEN, 3 instructions, one in each slice: the target adds the window's start to the size it loaded
 * first as `addu a1,v1,a1` (the start's load first among the operands, the sum in the size's
 * register), MW writes `addu a1,a1,v1` -- the two-step sum gives every register right but puts the
 * variable first in the add whichever way it is written (`start += m_start`, `start = m_start +
 * start`, `start = start + m_start`, a cast, a const accessor returning it); `m_start + m_size`
 * in one expression (in either order, through named copies of either field, a declaration split
 * from its assignment, an accessor, a wrap helper taking the sum and the count by value, a
 * named count before or after, the parameters swapped) loads the start first and trades the sum's
 * and the count's a0/a1 (27); `m_start += m_size` stores early (0x2C8); the wrap's arms the other
 * way round, `m_total ? x % m_total : 0` or an if/else, 0x2B0 */
extern "C" bool func_005CCA30(float dT, void *data)
{
    static int size0 = 6;
    static int size4 = 5;
    static int size1 = 1;
    int n = CEntity::m_list.size();

    ai_slice0.Next(n, size0);
    ai_slice1.Next(n, size1);
    ai_slice4.Next(n, size4);
    return false;
}

/* the AI's current state a hold with its bit 7 set (OVL2 0x559000, the SEALs' seal_ai.cpp: unnamed) */
extern "C" bool func_00559000(CSealCtrlAi *ai);

/* a squad's bit in the entities' m_UnitsSeenBy (zseal.h's CSealUnit pads it) */
struct CSensorUnitView {
    unsigned char _0[0x5c];
    unsigned int m_seen_bit;   // 0x5c
};

/* an enemy the squad had not seen called out: the squad told, then -- unless the AI holds with bit
 * 7 -- a SEAL's sighting by its place on the player's clock ("high" above 0.15 of the way up), moving
 * when the player's own target for it says so, a plain sighting with no player; nothing for an
 * entity not of kind 2 (OVL2 0x5C9930, called by OnSetTargetVisible with ftsGetPlayer(); SOCOM 1's
 * EnemyCallout took the CTarget and tested the SEAL's team bits). The squad and its bit into locals and
 * the first read of m_UnitsSeenBy through a const view (the target reads the word twice, the squad
 * once: a plain read is merged); the sighting preset and replaced for an entity not of kind 2 (the
 * target's beq takes the 62 in its delay slot, the -1 in the b's) */
void CSealCtrlAi::EnemyCallout(CEntity *e, CEntity *player)
{
    static float moving_speed = 800.0f;
    static float high_y = 0.15f;
    CZSealBody *seal = (CZSealBody *)m_entity;

    if (!e || e == player)
        return;
    CSealUnit *unit = m_unit;
    unsigned int bit = ((CSensorUnitView *)unit)->m_seen_bit;
    bool seen = (((const CEntity *)e)->m_UnitsSeenBy & bit) != 0;

    if (!seen)
        e->m_UnitsSeenBy |= bit;
    if (seen)
        return;
    unit->Notify(seal, UNIT_MESSAGE_0, e);
    if (func_00559000(this))
        return;
    bool moving = ai::is_moving(&e->m_velW, moving_speed);
    CHARACTER_SOUND snd = CHRSND_ENEMY_SIGHTED;

    if (e->m_type != 2)
        snd = CHRSND_NONE;
    else {
        if (player) {
            if (player->m_targets.FindTarget(e)->m_flags & 0x200) {
                snd = moving ? CHRSND_TANGO_MOVING : CHRSND_ENEMY_SIGHTED;
            } else {
                CPnt3D d;
                CQuat q;

                e->m_posW.Sub(&player->m_posW, &d);
                d.Normalize();
                q.w = player->m_quat.w;
                q.vec.x = -player->m_quat.vec.x;
                q.vec.y = -player->m_quat.vec.y;
                q.vec.z = -player->m_quat.vec.z;
                q.Apply(&d, &d, 1);
                bool high = d.y > high_y;

                d.y = 0.0f;
                d.Normalize();
                int clock = ai::get_clock_direction(&d) + 14;

                if (high)
                    clock += 12;
                snd = (CHARACTER_SOUND)clock;
            }
        }
    }
    PlayAiSound(snd, 1.0f, 0);
}

/* a target seen to fall: the AI calls it, "tango down", at full chance (OVL2 0x5C9B10; SOCOM 1's
 * played its sound 18 the same way, through the same slot +0x24) */
void CSealCtrlAi::NotifyTargetDown(CZSealBody *seal)
{
    PlayAiSound(CHRSND_TANGO_DOWN, 1.0f, -1);
}

/* the object at 0x67C a state tests (OVL2 0x60A850, unwritten; seal_tasks.cpp's spelling) */
extern "C" bool func_0060A850(void *target);

/* the sensing frame: the base's, the view's sensitivity (the AI parameters' visual_sens) halved for
 * it while the AI is in a group and put back after, then the object at 0x67C asked (OVL2 0x5C98B0,
 * byte-exact as a draft before: SOCOM 1's CSealCtrlAi::SensorTick, the base's caller there too --
 * its own work SOCOM II moved into the base and the sensor pre-tick). The parameters' address a
 * local taken first, the saved value read in place: the target keeps the one in s0 and reads the
 * other off `this` */
void CSealCtrlAi::SensorTick(float dT)
{
    static float group_sens = 0.5f;
    AI_PARAMS *p = &m_aiParams;
    float sens = m_aiParams.visual_sens;

    if (InGroup())
        p->visual_sens *= group_sens;
    CSealCtrl::SensorTick(dT);
    p->visual_sens = sens;
    func_0060A850(m_67c);
}

/* the application's camera (camera.h's CAppCamera, OVL1 bss 0x415FF0: its zdb camera, whose matrix
 * the node's first member, and the SEAL it shows -- seal.cpp's view) */
class CAppCamera {
public:
    unsigned char _0[0xb4];
    zdb::CCamera *m_camera;   // 0xb4
    zdb::CNode *m_camera_effects;   // 0xb8
    CZSealBody *m_bc;   // 0xbc: the SEAL the camera shows
};

extern CAppCamera *appCamera;

/* the SEAL's aim point (SOCOM 1's m_aim_goal and m_aim_point at 0x10AC/0x10B8, SOCOM II's 0x3A0 on:
 * zseal.h pads them) */
struct CSensorAimView {
    unsigned char _0[0x144c];
    CPnt3D m_aim_goal;   // 0x144c
    CPnt3D m_aim_point;   // 0x1458
};

/* a player's view cone: the camera's when it shows this SEAL (its point and its forward axis), the
 * head's otherwise, turned at the aim point while the SEAL has one; its height the view range
 * stretched by the zoom, its radius the range at the field of view's tangent, narrowed by the zoom
 * (OVL2 0x5C9520, CSealCtrl's slot +0xA8 -- SOCOM 1's took the zoom off the kit and looked from the
 * head alone; the three tuning statics are 8-byte-spaced data at 0x650A78). The range read in place
 * (through CSensorDecay::Get the products take the other operand first), the height defined before
 * the radius: MW numbers them so (radius $f20, height $f21) */
void CSealCtrl::UpdateViewCone()
{
    static float range_scale = 1.0f;
    static float point_radius = 25.0f;
    static float zoom_scale = 1.0f;
    CSensorConeCtrl *c = (CSensorConeCtrl *)this;
    CZSealBody *seal = (CZSealBody *)m_entity;
    float zoom = seal->m_zoom_range;
    float height = c->m_visrange.m_value * zoom;
    float radius = c->m_visrange.m_value * c->m_fovtan / range_scale;
    CPnt3D pos, dir;

    if (zoom > 1.0f)
        radius /= zoom * zoom_scale;
    if (appCamera && appCamera->m_bc == seal) {
        const CMatrix *m = &appCamera->m_camera->m_matrix;

        pos = *(const CPnt3D *)m->m_matrix[3];
        TransformVectors(m, &CPnt3D::unit_nz, &dir, 1);
    } else {
        seal->GetHeadParams(&pos, &dir);
        if (seal->m_aim_point_valid) {
            dir = ((CSensorAimView *)seal)->m_aim_point - pos;
            dir.Normalize();
        }
    }
    c->m_view_cone.m_height = height;
    c->m_view_cone.m_inv_height = 1.0f / height;
    c->m_view_cone.m_radius = radius;
    c->m_view_cone.m_point_radius = point_radius;
    c->m_view_cone.m_cone_pos = pos;
    c->m_view_cone.m_cone_dir = dir;
}

/* the view cone set from the SEAL's head: its point the eye, its direction halfway between the
 * head's and the body's back (the node's third row negated -- new in SOCOM II), its height the view
 * range, its radius the range at the field of view's tangent (OVL2 0x5C9400; SOCOM 1's, whose
 * height a scope's zoom stretched -- SOCOM II keeps the factor at 1) */
void CSealCtrlAi::UpdateViewCone()
{
    CZSealBody *seal = (CZSealBody *)m_entity;
    CSensorConeCtrl *c = (CSensorConeCtrl *)this;
    CPnt3D pos, dir;

    seal->GetHeadParams(&pos, &dir);
    const CMatrix &m = seal->m_node->m_matrix;
    CPnt3D back;

    back.x = -m.m_matrix[2][0];
    back.y = -m.m_matrix[2][1];
    back.z = -m.m_matrix[2][2];
    CPnt3D d = (dir + back) * 0.5f;
    float zoom = 1.0f;

    c->m_view_cone.m_height = zoom * c->m_visrange.Get();
    c->m_view_cone.m_inv_height = 1.0f / c->m_view_cone.m_height;
    c->m_view_cone.m_radius = c->m_fovtan * c->m_visrange.Get();
    c->m_view_cone.m_point_radius = 25.0f;
    c->m_view_cone.m_cone_pos = pos;
    c->m_view_cone.m_cone_dir = d;
}
