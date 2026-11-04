/* seal_sensors.cpp -- what an AI SEAL senses: its view cone, the visibility tests, the targets it
 * sees fall (SOCOM 1's unit, __sinit_seal_sensors.cpp: UpdateViewCone, SetViewFOV, ComputeVisibility,
 * RegisterCanSee ... NotifyTargetDown). OVL2 0x5C9400-0x5CDBCF, after seal_kit.cpp's CAiWepHint and
 * before seal_tasks.cpp. So far NotifyTargetDown, a pickup's sighting and the request pool's end:
 * CloseDiPool, OpenDiPool, CleanupDiPool and the free list's members (seal_dipool.h). Defined in
 * reverse of their image order (-inline deferred). */

#include "zseal.h"
#include "seal_dipool.h"

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
 * CBits::Test indexes first; the name ours) */
inline bool TestSensorBit(const CBits *bits, unsigned int i)
{
    return (1 << (i & 31)) & bits->m_vec[i >> 5];
}

/* a weapon pickup seen: unless the AI already goes for a pickup or this SEAL has marked it, a Navy
 * SEAL goes for item 152 when its kit has none of it or none with ammunition (the pickup state
 * pushed and handed it, flag 1); any other item is marked seen by this SEAL. Whether it went
 * (OVL2 0x5CB450: 0x5CB870 asks it; new in SOCOM II -- the name ours). `go` declared first and
 * assigned after the early return: the target numbers it before the SEAL (s2 to its s1).
 * OPEN, 8 instructions, the seen-bit test's registers only: the target masks the index first
 * (`andi a1,v1,0x1f` before `srl v1,v1,5`, the 1 in a0, the words in v0 -- the set below, which
 * matches, has the same allocation); every spelling here shifts first (srl a1, andi in place, the
 * 1 in v0): CBits::Test, the mask first or last, either in a local, the word's address in a local,
 * `(w >> s) & 1` (10) */
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

/* a target seen to fall: the AI calls it, "tango down", at full chance (OVL2 0x5C9B10; SOCOM 1's
 * played its sound 18 the same way, through the same slot +0x24) */
void CSealCtrlAi::NotifyTargetDown(CZSealBody *seal)
{
    PlayAiSound(CHRSND_TANGO_DOWN, 1.0f, -1);
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
