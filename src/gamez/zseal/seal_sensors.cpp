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
/* SOCOM 1's diNonVolCount (its file static, $gp -27440 there): read by the two OnSetTargetVisible as
 * whether a ray's sighting is fresh, never written (OVL2 bss 0x66B758, before the colours) */
static int diNonVolCount;

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
 * CBits::Test indexes first; the name ours). A mask local of an enum type (a modelling device): MW folds
 * an integer mask local read once back into the expression and then evaluates the word's index first
 * (51 spellings over rounds 35-43), but IRO's propagation leaves an enum-typed variable alone (the game
 * states' round 53), so the mask stays numbered first and the AND takes a fresh temporary */
enum SensorMask_ { SENSORMASK_MAX = 0x7fffffff };
inline bool TestSensorBit(const CBits *bits, unsigned int i)
{
    SensorMask_ m = (SensorMask_)(1 << (i & 31));

    return m & bits->m_vec[i >> 5];
}

/* a weapon pickup seen: unless the AI already goes for a pickup or this SEAL has marked it, a Navy
 * SEAL goes for item 152 when its kit has none of it or none with ammunition (the pickup state
 * pushed and handed it, flag 1); any other item is marked seen by this SEAL. Whether it went
 * (OVL2 0x5CB450: 0x5CB870 asks it; new in SOCOM II -- the name ours). `go` declared first and
 * assigned after the early return: the target numbers it before the SEAL (s2 to its s1). */
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
 * Whether it went (OVL2 0x5CB630: 0x5CB450's sibling for the stash, new in SOCOM II -- the name ours) */
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

/* a thrown grenade's pickup seen (OVL2 0x5CAC00), below */
struct CSensorProjView2;
extern "C" bool func_005CAC00(CSealCtrlAi *ai, CPickup *p, CSensorProjView2 *proj);

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
                    found = func_005CAC00(ai, p, (CSensorProjView2 *)proj);
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

/* a random float, a minimum and a range (zkit.h's Rfloat, which this unit does not include: SOCOM 1's
 * operator float inlined) */
class Rfloat {
public:
    Rfloat(float min, float max) : m_min(min), m_range(max - min) {}
    operator float() const
    {
        float f;

        if (m_range == 0.0f)
            f = m_min;
        else {
            float r = 4.656613e-10f * rand();
            f = m_min + m_range * r;
        }
        return f;
    }

    float m_min;   // 0x0
    float m_range;   // 0x4
};

CZSealBody *ftsGetPlayer();

/* the entity as a SEAL, or 0 (a ternary: the zero's arm falls through, the SEAL's joins after it; an
 * inline, the name ours) */
static inline CZSealBody *SensorSeal(CEntity *e)
{
    return e && e->m_type == CEntity::ENT_TYPE_SEAL ? (CZSealBody *)e : 0;
}

/* the AI's own target (ai_target.cpp's CAiTarget, the AI's at 0x67C: only what the sighting asks) and
 * its validity test and update from a target (OVL2 0x60A850, 0x60A6A0 -- ai_target.cpp's, unnamed) */
class CAiTarget {
public:
    float GetTimeSinceLastSighting() const;
    void OnSetVisible(bool visible);
};

extern "C" bool func_0060A850(void *target);
extern "C" void func_0060A6A0(void *target, CTarget *t);

/* the mission's clock, and the time since a mark on it (OVL2 0x5750B0, 0x5750C0: seal_command.cpp's
 * out-of-line copies) */
extern "C" float func_005750B0(void);
extern "C" float func_005750C0(float t);

/* the AI's reaction to a fresh sighting: a sound played and, unless it already crouches, a stance
 * asked (OVL2 0x605770, unwritten; the name unknown) */
extern "C" void func_00605770(CSealCtrlAi *ai, bool stance, CHARACTER_SOUND sound, float chance);

/* the AI's bit 16, and not its bit 17, as bools (inlines, the names ours: the second materialised --
 * the target tests its `xori` -- and both read off one load of the byte). Joined as `b ? NotBit17 : b`:
 * the first's zero tested again at the join, where `&&` jumps past it and `if (b) b = ...` narrows */
static inline bool Bit16(const CSealCtrlAi *ai)
{
    return ai->m_aibit16 ? true : false;
}

static inline bool NotBit17(const CSealCtrlAi *ai)
{
    return !(ai->m_aibit17 ? true : false);
}

/* a chance taken (an inline, seal_airesponse.cpp's Chance's shape: the odds by value, read through a
 * reference -- the target keeps them in the frame and their address in a saved register across rand) */
static inline bool SensorChance(float odds)
{
    const float &c = odds;
    Rfloat r(0.0f, 1.0f);

    return r <= c;
}

/* a target seen or lost by the AI (OVL2 0x5CA470, CSealCtrlAi's slot +0xAC; SOCOM 1's, rewritten): the
 * fresh-sighting bit as the base's; seen anew (not yet bit 9) -- a Navy SEAL calls the enemy out to the
 * player; else one not heard (bit 8) that the AI's own target is not holding, or one unseen for 3
 * seconds, makes the AI react: at once when it is unaware and the target within 250, else one time in
 * four (the alertness then not raised); the callout made; a SEAL enemy's sighting counted, a rushing one
 * (or one armed beyond 250) marked 0x400, the latter announced; the target heard, the AI's event reset,
 * the sighting's time taken, the AI's target told -- then bit 11 once seen for longer than the target
 * delay, visible, the alertness specific. Lost: the AI's target, if this is it, remembered where it was
 * for 1-3 seconds, the alertness by direction; bit 11 cleared after 5 seconds, bit 9 cleared. Every
 * local declared first (params s8, react s3, the AI's target s2 -- only the update's argument -- friendly
 * s6, heard s7, current s1, the SEAL s0; the distance f21 before the alertness f20); the Navy SEAL bit
 * read before the visible test; a rushing AI through zseal.h's AiStateIs; the sound's bits as Bit16's
 * bool `b ? NotBit17(this) : b` */
void CSealCtrlAi::OnSetTargetVisible(CTarget &target, bool visible, zdb::DiIntersect *di)
{
    AI_PARAMS *params;
    bool react;
    CAiTarget *mine;
    bool friendly;
    bool heard;
    bool current;
    CZSealBody *seal;
    float dist;
    float aware;

    params = &m_aiParams;
    mine = (CAiTarget *)m_67c;
    current = *(CEntity **)m_67c == target.m_ent;
    friendly = (target.m_ent->m_team & m_entity->m_team) != 0;
    bool fresh = di ? diNonVolCount > 0 : true;

    if (fresh)
        target.m_flags |= 2;
    else
        target.m_flags &= ~2;
    if (visible) {
        seal = SensorSeal(target.m_ent);
        dist = target.m_distSq;
        aware = 0.0f;
        heard = (target.m_flags & 0x100) != 0;
        bool navy = IsNavySeal();

        if (!(target.m_flags & 0x200)) {
            if (navy)
                EnemyCallout(target.m_ent, ftsGetPlayer());
            else {
                if (!heard) {
                    react = true;
                    if (func_0060A850(m_67c)) {
                        bool held = func_0060A850(m_67c) && (m_6a8 & 0x200);

                        if (!held)
                            react = ((CAiTarget *)m_67c)->GetTimeSinceLastSighting() > 3.0f;
                        else
                            react = false;
                    }
                    if (react) {
                        if (GetAiAware() < AIA_GENERAL && dist < 62500.0f) {
                            aware = -1.0f;
                            func_00605770(this, true, (CHARACTER_SOUND)70, 1.0f);
                        } else if (SensorChance(0.25f)) {
                            aware = 0.0f;
                            func_00605770(this, true, (CHARACTER_SOUND)70, 1.0f);
                        }
                    }
                }
                EnemyCallout(target.m_ent, 0);
            }
            if (seal && !friendly && !heard)
                seal->m_stats.IncrTimesSeen(1);
            if (!(target.m_flags & 0x400) && seal && !friendly) {
                if (dist < 62500.0f) {
                    CSealCtrlAi *ai = ((CSealCtrl *)seal->m_control)->GetSealCtrlAi();

                    if (ai && ai->AiStateIs(AI_STATE_RUSH))
                        target.m_flags |= 0x400;
                } else if (seal->GetWeaponState() == 1) {
                    target.m_flags |= 0x400;
                    bool b = Bit16(this);

                    if (b ? NotBit17(this) : b)
                        PlayAiSound((CHARACTER_SOUND)64, 1.0f, (CAiEvent::EVENT)-1);
                }
            }
            target.m_flags |= 0x100;
            ((CAiEvent *)&m_event)->Initalize();
            target.m_unk20 = func_005750B0();
            if (current)
                ((CAiTarget *)m_67c)->OnSetVisible(true);
        }
        if (!(func_005750C0(target.m_unk20) <= params->target_delay))
            target.m_flags |= 0x800;
        target.m_flags |= 0x200;
        SetAiAware(AIA_SPECIFIC, 0, aware);
        if (func_0060A850(m_67c) && current)
            func_0060A6A0(mine, &target);
    } else {
        if (target.m_flags & 0x200) {
            if (current && func_0060A850(m_67c)) {
                SetAiEvent((CAiEvent::EVENT)2, Rfloat(1.0f, 3.0f), &target.m_ent->m_posW);
                ((CAiTarget *)m_67c)->OnSetVisible(false);
            }
            SetAiAware(AIA_DIRECTION, 0, -1.0f);
        }
        if (!(func_005750C0(target.m_unk20) <= 5.0f))
            target.m_flags &= ~0x800;
        target.m_flags &= ~0x200;
        if (current)
            func_0060A6A0(mine, &target);
    }
}

/* the online test (seal_command.cpp's 0x5750A0) */
extern "C" int func_005750A0(void);

/* an anchor brought back onto the compass: unlocked, then enabled with its arrow (inline; the name
 * ours) */
static inline void ShowAnchor(CCompassAnchor *a)
{
    a->m_locked = false;
    a->SetEnabled(true);
    a->SetArrowEnabled(true);
}

/* a SEAL's compass anchor and an anchor's kind, -1 for none (inlines; the names ours) */
static inline CCompassAnchor *SealAnchor(CZSealBody *seal)
{
    return seal ? seal->m_compass_anchor : 0;
}

static inline int AnchorType(const CCompassAnchor *a)
{
    return a ? (int)a->m_type : -1;
}

/* an entity's anchor (CEntity's, 0xC4), and the entity as a turret or a kind 4 (inlines; the names
 * ours) */
static inline CCompassAnchor *EntityAnchor(CEntity *e)
{
    return e ? e->m_anchor : 0;
}

static inline CEntity *AsKind(CEntity *e, int kind)
{
    return e && e->m_type == kind ? e : 0;
}

/* a target seen or lost: its fresh-sighting bit (1: no ray, or diNonVolCount up) and its visible bit
 * (9) set; a target seen that is no friend told to the squad (message 0); offline, a hostage or
 * escortee SEAL seen (team bits 0x20000, 0x40000) shows its anchor of kind 4 or 5, and a turret's or a
 * kind 4's own anchor is shown (OVL2 0x5CA170; SOCOM 1's set the two bits only) */
void CSealCtrl::OnSetTargetVisible(CTarget &target, bool visible, zdb::DiIntersect *di)
{
    bool fresh = di ? diNonVolCount > 0 : true;

    if (fresh)
        target.m_flags |= 2;
    else
        target.m_flags &= ~2;
    if (visible)
        target.m_flags |= 0x200;
    else
        target.m_flags &= ~0x200;
    if (visible) {
        CEntity *e = target.m_ent;

        if (!(e->m_team & m_entity->m_team))
            m_unit->Notify((CZSealBody *)m_entity, (UNIT_MESSAGE)0, e);
        if (!func_005750A0()) {
            if (e && ((e->m_team & 0x20000) || (e->m_team & 0x40000))) {
                CCompassAnchor *a = SealAnchor((CZSealBody *)AsKind(e, CEntity::ENT_TYPE_SEAL));
                int type = AnchorType(a);

                if (type == 4 || type == 5)
                    ShowAnchor(a);
            }
            CEntity *t = target.m_ent;
            CCompassAnchor *a = EntityAnchor(t);

            if (a) {
                CEntity *turret = AsKind(t, CEntity::ENT_TYPE_TURRET);
                CEntity *kind4 = AsKind(t, 4);

                if (turret || kind4)
                    ShowAnchor(a);
            }
        }
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

/* the views the grenade's sighting needs (zweapon.h's CZProjectile, which zseal.h declares only by name) */
struct CSensorProjView2 {
    unsigned char _0[0x10];
    CZAmmo *m_ammo;   // 0x10
    unsigned char _14[0x48 - 0x14];
    CPnt3D m_pos;   // 0x48
    unsigned char _54[0xac - 0x54];
    unsigned int m_shooterID;   // 0xac
};

/* the grenade state's pickup set (OVL2 0x55CCA0; seal_ai.cpp's), the tables' static floats */
extern "C" void func_0055CCA0(CAiState *grenade, CPickup *p);
extern float data_00650A28;
extern float data_00650A30;
extern float data_00650A38;

/* the SEAL's squad number (CSealCtrl's unit at 0x10C, CSealUnit's m_unit_num 0x58) */
static inline unsigned int UnitNumOf(CSealCtrlAi *ai)
{
    return *(unsigned int *)(*(char **)((char *)ai + 0x10c) + 0x58);
}

/* a point copied, its fields read first (inline; the name ours) */
static inline void CopyPnt(CPnt3D *to, const CPnt3D *from)
{
    float x = from->x;
    float y = from->y;
    float z = from->z;

    to->x = x;
    to->y = y;
    to->z = z;
}

/* a grenade in the view cone (CSensorCone::Contains on the controller's own fields: through a `this` of
 * its own MW keeps the cone's address in a saved register; inline, the name ours) */
static inline bool ConeContains(CSealCtrlAi *c, CSensorProjView2 *proj)
{
    proj->m_pos.Sub((const CPnt3D *)c->m_cone_pos, (CPnt3D *)c->m_cone_vec);
    float dot = c->m_cone_dir[0] * c->m_cone_vec[0] + c->m_cone_dir[1] * c->m_cone_vec[1] + c->m_cone_dir[2] * c->m_cone_vec[2];

    c->m_cone_dot = dot;
    if (dot <= 0.0f || dot > c->m_cone_height)
        return false;
    float r = c->m_cone_point_radius + dot * c->m_cone_inv_height * c->m_cone_radius;

    if (c->m_cone_vec[2] * c->m_cone_vec[2] + (c->m_cone_vec[0] * c->m_cone_vec[0] + c->m_cone_vec[1] * c->m_cone_vec[1]) - dot * dot <= r * r)
        return true;
    return false;
}

/* how far a grenade is, squared (inline: its point below the function's own, as the target's; the name ours) */
static inline float GrenadeDistSq(CEntity *e, CSensorProjView2 *proj)
{
    CPnt3D d;

    proj->m_pos.Sub(&e->m_posW, &d);
    return d.x * d.x + d.y * d.y + d.z * d.z;
}

/* a grenade in range and in line of sight (inline; the name ours) */
static inline bool InReach(CEntity *e, CSensorProjView2 *proj)
{
    CPnt3D d;

    proj->m_pos.Sub(&e->m_posW, &d);
    return !(data_00650A28 <= d.x * d.x + d.y * d.y + d.z * d.z) &&
           !(AiMaps.ComputeLineVisibility(e->m_mapLocValid, &proj->m_pos, 0, 18.0f) <= data_00650A30);
}

/* a thrown grenade's pickup seen: marked seen by this SEAL, the squad told once if it was already known to
 * the unit, or a Navy SEAL or the grenade state not up (whether it reacts at all); the thrower looked up
 * and, when the SEAL is the thrower or has him as a target, whether it saw the throw -- the grenade in
 * its view cone or within reach and in line of sight; its comrades' throws let pass; a callout when an
 * enemy's frag is seen; the grenade state pushed when the grenade is inside its blast radius and, when it
 * should react first, an event state ahead of it (OVL2 0x5CAC00, new in SOCOM II: 0x5CB870 calls it; the
 * name ours). OPEN, the size 0x840 against 0x848: the m_pos2 reads' dead arms are SetEvent's now (`(pos2 ? pos2 :
 * &CPnt3D::zero)->x` with a literal 0: a `b` over `move v0,zero` each). Left: saw in s2 for the target's s6, type in fp
 * for s2, told in s6 for fp (24 declaration orders scored, the best d=72; igreplay --greedy), the range==0 arm's surviving
 * `b; nop` (the block optimization's copy, three forms tried) and the unseen test's first sb one slot early. Measured: a cast of a parameter at an inline's
 * argument (ETYPCON(ai)) makes every field address subable, in saved registers -- hence the array members,
 * the projectile view as the parameter type and CEntity's own m_index; the
 * name ours) */
extern "C" bool func_005CAC00(CSealCtrlAi *ai, CPickup *p, CSensorProjView2 *proj)
{
    bool told;
    bool self;
    bool saw;
    unsigned char type;
    CEntity *e = ai->m_entity;

    type = p->m_type;
    bool react = false;
    bool unseen = true;

    if (TestSensorBit((CBits *)p->m_bits, e->m_index)) {
        unseen = false;
        bool again;

        if (p->GetVisibleByUnit(UnitNumOf(ai)) || ai->IsNavySeal()) {
            if (!(ai->m_stateStack.m_10 & (1 << AI_STATE_GRENADE))) {
                again = true;
                unseen = false;
                react = false;
            } else
                again = false;
        } else if ((type == 1 || type == 2) && !(ai->m_stateStack.m_10 & (1 << AI_STATE_GRENADE))) {
            again = true;
            react = true;
        } else
            again = false;
        if (!again)
            return false;
    }
    p->GetPosW();
    bool go = true;

    if (unseen) {
        CEntity *thrower = CEntity::m_list.GetEntityById(proj->m_shooterID);

        if (thrower) {
            saw = false;
            CTarget *t = ((CEntity *)ai->m_entity)->m_targets.FindTarget(thrower);

            self = thrower == e;
            told = false;
            bool team = (thrower->m_team & e->m_team) != 0;

            if (!self && !t)
                go = false;
            else {
                CZSealBody *seal = thrower && thrower->m_type == 2 ? (CZSealBody *)thrower : 0;
                CSealCtrlAi *tai = seal ? seal->GetSealCtrlAi() : 0;
                bool nav;

                if (tai)
                    nav = tai->IsNavySeal();
                else
                    nav = seal ? seal->IsPlayer() : false;

                if (self || (t->m_flags & 0x200) || ai->IsNavySeal()) {
                    p->SetVisibleByUnit(UnitNumOf(ai));
                    saw = true;
                    told = saw;
                } else {
                    if (!ConeContains(ai, proj)) {
                        if (data_00650A28 <= GrenadeDistSq(e, proj))
                            goto notseen;
                        if (AiMaps.ComputeLineVisibility(e->m_mapLocValid, &proj->m_pos, 0, 18.0f) <= data_00650A30)
                            goto notseen;
                    }
                    p->SetVisibleByUnit(UnitNumOf(ai));
                    saw = true;
                    told = saw;
                notseen:;
                }
                if (!saw && (!team || !nav))
                    go = false;
                if (told && type == 1 && !team)
                    ai->PlayAiSound((CHARACTER_SOUND)51, 1.0f, (CAiEvent::EVENT)-1);
            }
        }
    }
    if (!go)
        return false;
    SetSensorBit((CBits *)p->m_bits, e->m_index);
    if (!(ai->m_stateStack.m_10 & (1 << AI_STATE_GRENADE))) {
        CZAmmo *ammo = proj->m_ammo;
        float r = ammo ? ammo->GetExplosionRadius() : 200.0f;

        if (ai->IsNavySeal() && r < 200.0f)
            r = 200.0f;
        r *= r;
        CPnt3D d;

        CopyPnt(&d, &e->m_posW);
        d.Sub(p->GetPosW(), &d);
        if (!(r <= d.x * d.x + d.y * d.y + d.z * d.z)) {
            ai->PopAiState(AI_STATE_SHOTAT);
            if (ai->PushAiState(AI_STATE_GRENADE, 0)) {
                CAiState *s = ai->GetAiState();
                CAiState *g = s && s->m_state == AI_STATE_GRENADE ? s : 0;

                func_0055CCA0(g, p);
                g->m_active = true;
            }
            if (react && ai->PushAiState(AI_STATE_EVENT, 2)) {
                float delay = 1.5f * (1.2f - ai->m_aiParams.training);

                if (ai->m_navyseal)
                    delay = 0.2f;
                float dist = (d.x * d.x + d.y * d.y + d.z * d.z) / r;
                CAiState *s = ai->GetAiState();
                CAiSEvent *ev = s && s->m_state == AI_STATE_EVENT ? (CAiSEvent *)s : 0;
                const CPnt3D *pos = p->GetPosW();

                ev->SetEvent(1, pos, 0);
                float t = delay * dist;
                float range = 2.0f * t - t;

                ev->SetFlags(4, 0);
                float v = t;

                if (range == 0.0f)
                    v = t;
                else
                    v += range * (4.656613e-10f * (float)rand());
                *(float *)&ev->m_88 = data_00650A38 + v;
                ev->m_rnd_min = data_00650A38;
                ev->m_rnd_range = 0.0f;
                ev->m_active = true;
            }
        }
    }
    return true;
}


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
 * The target adds the window's start to the size it loaded first as `addu a1,v1,a1` (the start's
 * load first among the operands, the sum in the size's register): Next() loads the size into a named
 * local and adds it to the start (`int size0 = m_size; int start = m_start + size0;`), and the function
 * body opens `#pragma opt_propagation off` -- copy propagation would fold the local's load into the
 * sum (the load order and a0/a1 traded, 27 off), and the two-step `start += m_start` the rounds before
 * tried puts the variable first in the add (3 off). Round 56: byte-exact so (the brief's lead: MW
 * substitutes `v = expr` into a single use -- here the substitution is the whole difference) */
extern "C" bool func_005CCA30(float dT, void *data)
{
#pragma opt_propagation off
    static int size0 = 6;
    static int size4 = 5;
    static int size1 = 1;
    int n = CEntity::m_list.size();

    ai_slice0.Next(n, size0);
    ai_slice1.Next(n, size1);
    ai_slice4.Next(n, size4);
    return false;
}
#pragma opt_propagation reset

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
    PlayAiSound(snd, 1.0f, CAiEvent::AIE_UNKNOWN);
}

/* the perception's range bands (ai_perception.cpp's class: only the static setter here) */
class CAiPerception {
public:
    static void SetMaxRange(float range);
};

/* a target's worth to an entity, and how well a viewer sees a target through its finished ray
 * (ai_target.cpp's 0x60B7F0 and 0x60AA20) */
extern "C" float func_0060B7F0(const CEntity *self, const CTarget *t);
extern "C" float func_0060AA20(void *viewer, zdb::DiIntersect *di, CTarget *t);

/* the sensing's limits: the reach the perception's bands stop at, the visibility a ray's sighting
 * is held within and the visibility seen at (OVL2 data, read as variables) */
extern float data_00650A48;
extern float data_00650990;
extern float data_00650998;
extern float data_0065099C;

/* a request registered with the world, its result taken (the request dropped, its DiIntersect handed
 * back) and how long it waited (inlines: ai_target.cpp's 0x60AFA0 has the same three; the names ours) */
static inline bool SensorDirRegistered(const CDiPool::dir *d)
{
    return d == 0 ? false : d->m_state == 4;
}

static inline zdb::DiIntersect *SensorDirTake(CDiPool::dir *d)
{
    if (d && d->m_state == 4) {
        d->m_state = 1;
        return d->m_index >= 0 ? &_diPool.m_pool[d->m_index] : 0;
    }
    return 0;
}

static inline float SensorDirAge(const CDiPool::dir *d)
{
    return d ? d->m_age : 0.0f;
}

/* the entity as a turret, or 0 (a ternary, as SensorSeal; the name ours) */
static inline CSensorTurret *SensorTurret(CEntity *e)
{
    return e && e->m_type == CEntity::ENT_TYPE_TURRET ? (CSensorTurret *)e : 0;
}

/* a turret flagged 2 and not 0x10 (an inline; the name ours): the first bit a named bool, then `r &&`
 * -- the target computes it in v0 and narrows it into the result in the branch's delay slot (an
 * if-assigned bool, a ternary, early returns all land it straight in the result or pad the join) */
static inline bool TurretSensed(const CSensorTurret *t)
{
    bool r = t->Bit2();

    return r && !t->Bit10();
}

/* Perceivable as SensorTick has it: the turret through the ternary cast and TurretSensed (an inline;
 * the name ours) */
static inline bool SensorPerceivable(CEntity *e, bool ai)
{
    bool r = false;

    if (e && e->m_isAlive) {
        if (e->m_type == CEntity::ENT_TYPE_SEAL)
            r = true;
        else if (e->m_type == CEntity::ENT_TYPE_TURRET)
            r = TurretSensed(SensorTurret(e));
        else if (e->m_type == 4)
            r = !ai;
    }
    return r;
}

/* two entities sharing a team, materialised (an inline; the name ours: through it, the address of the
 * SEAL's mask is hoisted out of the walk and spilled, as the target's) */
static inline bool SameTeam(const CEntity *a, const CEntity *b)
{
    return (a->m_team & b->m_team) != 0;
}

/* a side's threat: its largest, scaled up when it outnumbers the other side, plus its mean (an
 * inline; the name ours): the largest copied into the result first and the mean added as a ternary
 * (the target's `b` over the else arm the allocator empties) */
static inline float SideThreat(float most, float sum, int n, int other)
{
    float r = most;
    int k = other < n ? n / (other + 1) + 1 : 1;

    r *= k;
    r += n > 0 ? sum / n : most;
    return r;
}

/* the targets' frame (OVL2 0x5C9B30; SOCOM 1's CSealCtrl::SensorTick, CSealCtrl::PostTick's and the
 * AI's SensorTick's call, rewritten in SOCOM II): the perception's reach set to the view range (1000 at
 * least); each target: one not of a kind there is to perceive is lost; else a seen enemy flagged 0x200
 * counted, its worth (0x60B7F0) summed and the largest kept -- of the enemies seen or heard (0x100,
 * 0x400), of those in sight (the best taken as the target), of the rest -- and a friend's (negative)
 * likewise while the AI is not stunned; a finished ray's visibility (0x60AA20) blended in by the time it
 * waited, held within the limits, seen at the threshold -- a hidden SEAL never. The threat the sides'
 * sums, at least 0. Every local declared first (the saved registers: best s8, targets s7, the entity
 * s6, the counts s4/s3, the target's entity s2, friendly s1, the walk s0 -- the entity declared before
 * the list), the floats assigned after the parameters' address. One of each pair of the target's
 * reads and writes of a field through `(*t).` (spelled alike, MW keeps the field's address in a saved
 * register across the calls; the target reloads off the walk). The friends' sum added in a second
 * statement (in one expression MW works out the friends first) */
void CSealCtrl::SensorTick(float dT)
{
    CTarget *best;
    int nEnemies;
    int nFriends;
    CEntity *e;
    CTargetList *targets;
    CEntity *te;
    bool friendly;
    CTargetList::iterator t;
    float most;
    float sumEnemy;
    float least;
    float sumFriend;
    float seenBest;
    float unseenBest;
    float threat;
    AI_PARAMS *params;

    e = m_entity;
    GetSealCtrlAi();
    m_targetSum = 0;
    params = &m_aiParams;
    most = 0.0f;
    targets = &e->m_targets;
    nEnemies = 0;
    nFriends = 0;
    best = 0;
    sumEnemy = 0.0f;
    least = 0.0f;
    sumFriend = 0.0f;
    seenBest = 0.0f;
    unseenBest = 0.0f;
    CAiPerception::SetMaxRange(ai::_max(data_00650A48, params->visual_range));
    for (t = targets->begin(); t != targets->end(); t++) {
        te = t->m_ent;
        if (!te)
            continue;
        friendly = SameTeam(e, te);
        if (!SensorPerceivable(te, IsAi()))
            OnSetTargetVisible(*t, false, 0);
        else {
            if (((*t).m_flags & 0x200) && !friendly)
                m_targetSum++;
            threat = func_0060B7F0(e, &*t);
            if (!friendly && !(threat < 0.0f)) {
                unsigned int flags = t->m_flags;

                if ((flags & 0x100) || (flags & 0x400)) {
                    nEnemies++;
                    sumEnemy += threat;
                    if (threat > most)
                        most = threat;
                }
                if (flags & 0x200) {
                    if (threat >= seenBest) {
                        seenBest = threat;
                        best = &*t;
                    }
                } else if (threat >= unseenBest)
                    unseenBest = threat;
            }
            if (friendly && threat < 0.0f && IsAi()
                && !(GetSealCtrlAi()->m_stateStack.m_10 & (1 << AI_STATE_STUNRESPONSE))) {
                sumFriend += threat;
                nFriends++;
                if (threat < least)
                    least = threat;
            }
        }
        CDiPool::dir *d = (CDiPool::dir *)t->m_unk28;

        if (SensorDirRegistered(d)) {
            (*t).m_unk28 = 0;
            zdb::DiIntersect *di = SensorDirTake(d);

            if (di) {
                float age = SensorDirAge(d);
                CSensing s;

                s.m_entity = e;
                s.m_params = params;
                s.m_eye_valid = false;
                float x = func_0060AA20(&s, di, &*t);
                float v = x * age + t->m_visibility;

                _saturate(&v, data_00650998, data_0065099C);
                bool seen = v >= data_00650990;

                t->m_aware = x;
                (*t).m_visibility = v;
                CZSealBody *seal = SensorSeal(t->m_ent);

                if (seal && (seal->m_invisible ? true : false))
                    seen = false;
                OnSetTargetVisible(*t, seen, di);
            }
        }
    }
    if (best)
        SetTarget(best);
    float sum = SideThreat(most, sumEnemy, nEnemies, nFriends);

    sum += SideThreat(least, sumFriend, nFriends, nEnemies);
    if (sum < 0.0f)
        sum = 0.0f;
    m_targetThreat.m_goal = sum;
}

/* a request off the free list, set up done for an entity and put on the used list (OVL2 0x5C9830,
 * GetRequest's copy; 0x5CC100 and 0x5CC0D0, unwritten out-of-line copies: the set-up and the list's
 * push_back) */
extern "C" CDiPool::dir *func_005C9830(CDiPool::CDirList *free);
extern "C" void func_005CC100(CDiPool::dir *d, CEntity *e, int x);
extern "C" void func_005CC0D0(CDiPool::CDiList *used, CDiPool::dir **d);

/* the AI's reaction to a target it lost track of, and to a friend's AI (OVL2 0x565110, 0x560B20 --
 * seal_ai.cpp's range, unwritten; the names unknown) */
extern "C" void func_00565110(CSealCtrlAi *ai, int kind, void *target, int x);
extern "C" void func_00560B20(CSealCtrlAi *ai, CSealCtrlAi *other, int x);
/* the glance on the AI's frame slice (OVL2 0x558640, seal_ai.cpp's) and the pickups' sighting (0x5CB870,
 * this unit's) */
extern "C" void func_00558640(CSealCtrlAi *ai);
extern "C" void func_005CB870(CSealCtrlAi *ai);

/* the AI sensing's limits (OVL2 data, read as variables): the wait between reactions to a stale target,
 * the odds of one by the training, the CQB reach, the reach a SEAL counts as near, a Navy SEAL's call
 * reach; and the reaction forced (bss, a byte) */
extern float data_0064FF80;
extern float data_006509F8;
extern float data_00650A00;
extern float data_00650A08;
extern float data_00650A10;
extern "C" unsigned char data_0066B7C8;

/* a state as the CQB attack, or 0 (a ternary, as SensorSeal; the name ours) */
static inline CAiSCQBAttack *AsCQBAttack(CAiState *s)
{
    return s && s->m_state == AI_STATE_CQBATTACK ? (CAiSCQBAttack *)s : 0;
}

/* a done request for an entity taken and queued, and the frame's count of them stepped (inlines, the
 * names ours: two levels, as the target lays the request's slot under the chance's -- in place or one
 * level, above it) */
static inline void Request2(CEntity *e)
{
    CDiPool::dir *d = func_005C9830(&_diPool.m_free);

    func_005CC100(d, e, 0);
    func_005CC0D0(&_diPool.m_used, &d);
}

static inline void Request1(CEntity *e)
{
    Request2(e);
    _diPool.m_34++;
}

/* the AI's sensing frame (OVL2 0x5CBA20; SOCOM 1's CSealCtrlAi::SensorPreTick, CSealCtrlAi::PreTick's
 * call, rewritten; written from the target): a ray requested while the frame's budget lasts and the attack asks for one; a
 * target the AI's own target is not holding and not seen for 3 seconds, after the wait, makes a
 * non-Navy SEAL react (forced, or by the training's odds); the events, the slice's glance and the
 * pickups ticked; the sensing record set; each living target: the nearest enemy's range kept, the AI's
 * own target found; a SEAL in reach marks the SEAL to collide with it and take altitude; an enemy SEAL
 * seen whose ray is not fresh and that is not invincible starts a CQB attack within reach (with the
 * stack's flag 2, not already attacking, when the SEAL can and the push takes), else a Navy SEAL with
 * the no-shoot bit calls out a visible one in reach; a friend's AI is told; an enemy not perceived
 * fades as the base's. The AI's target dropped when it was not among them. A friend's AI through
 * CZSealBody's GetSealCtrlAi (one inline level more: the target's unfilled slot and padded join); the
 * stale-target test as OnSetTargetVisible's; the perception into a bool local (the andi); the
 * distance's limit after the target test's call */
void CSealCtrlAi::SensorPreTick(float dT)
{
    CZSealBody *seal = (CZSealBody *)m_entity;
    bool react;

    if (AttackTick(dT, _diPool.m_8 - _diPool.m_34 > 0))
        Request1(seal);
    react = true;
    if (func_0060A850(m_67c)) {
        bool held = func_0060A850(m_67c) && (m_6a8 & 0x200);

        if (!held)
            react = ((CAiTarget *)m_67c)->GetTimeSinceLastSighting() > 3.0f;
        else
            react = false;
    }
    if (!react && !IsNavySeal() && !(func_005750C0(m_770) <= data_0064FF80)) {
        if (data_0066B7C8 || SensorChance(data_006509F8 * m_aiParams.training))
            func_00565110(this, 255, m_67c, 0);
        m_770 = func_005750B0();
    }
    EventTick(dT);
    func_00558640(this);
    func_005CB870(this);
    _sensing.m_entity = seal;
    _sensing.m_params = &m_aiParams;
    _sensing.m_eye_valid = false;

    CTargetList &targets = seal->m_targets;
    bool have = func_0060A850(m_67c);
    float nearest = 1000000.0f;
    bool found = false;

    for (CTargetList::iterator t = targets.begin(); t != targets.end(); t++) {
        CEntity *te = t->m_ent;

        if (!te || !te->m_isAlive)
            continue;
        bool enemy = !((te->m_team & seal->m_team) != 0);
        float dist = t->m_distSq;

        if (enemy && dist < nearest) {
            nearest = dist;
            m_6ec = t->GetRange();
        }
        if (have && !found)
            found = *(CEntity **)m_67c == te;
        static float hearing = CAiEvent::GetEventRange((CAiEvent::EVENT)0) * CAiEvent::GetEventRange((CAiEvent::EVENT)0);
        CZSealBody *s = SensorSeal(te);
        CSealCtrlAi *ai = s ? s->GetSealCtrlAi() : 0;

        if (s && dist <= data_00650A08) {
            seal->m_check_player_collision = 1;
            seal->m_get_new_altitude = 1;
        }
        if (enemy) {
            if (s && !(seal->m_noshoot ? true : false) && (t->m_flags & 0x200) && !(t->m_flags & 2)
                && !(s->m_invincible ? true : false)) {
                if (dist < data_00650A00 && (m_stateStack.m_14 & 2) && !AiStateIs(AI_STATE_CQBATTACK)
                    && seal->CanUseCQB(s) && PushAiState(AI_STATE_CQBATTACK, 0)) {
                    CAiSCQBAttack *cqb = AsCQBAttack(GetAiState());

                    cqb->m_target = s;
                    cqb->m_active = 1;
                }
            } else if (IsNavySeal() && (seal->m_noshoot ? true : false) && dist <= data_00650A10
                       && (t->m_flags & 0x200))
                PlayAiSound((CHARACTER_SOUND)5, 1.0f, (CAiEvent::EVENT)1);
        } else if (ai)
            func_00560B20(this, ai, 0);
        if (enemy) {
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
    if (have && !found)
        SetTarget(0);
}

/* a target seen to fall: the AI calls it, "tango down", at full chance (OVL2 0x5C9B10; SOCOM 1's
 * played its sound 18 the same way, through the same slot +0x24) */
void CSealCtrlAi::NotifyTargetDown(CZSealBody *seal)
{
    PlayAiSound(CHRSND_TANGO_DOWN, 1.0f, CAiEvent::AIE_NONE);
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
