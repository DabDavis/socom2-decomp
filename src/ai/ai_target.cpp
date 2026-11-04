/* ai_target.cpp -- the AI's target, CAiTarget, and SOCOM 1's ai::CheckLineOfFire: the stretch after
 * seal_airesponse.cpp, OVL2 0x609DF0-0x60BF3F (CAiTarget's constructor at 0x60A940; SEAL_AI_PARAMS's
 * at 0x60BF40 follows, seal_aiparams.cpp). SOCOM 1 had the same functions in the same place, right
 * after the response states' destructors; the unit's name is ours. So far the validity test, a
 * target's worth (0x60B7F0) and one getter. Defined in reverse of their image order (-inline
 * deferred). */

#include "../gamez/zentity/zentity.h"
#include "../gamez/zseal/seal_dipool.h"

/* the mission's clock, which the target's times are taken on (theMission's float at 0xE0, OVL1
 * 0x4365C0: SOCOM 1's at 0x60 of its theMission, as its GetTimeSinceLastSighting reads it) --
 * a view of the mission, fts_state.h's CMission not reaching it */
struct CMissionClock {
    unsigned char _0[0xe0];
    float m_time;   // 0xe0
};
extern CMissionClock theMission;

/* the target's entity as the validity test reads it (a view: zentity.h's CEntity and zseal.h's
 * controller classes are not included here) -- its type (2 a SEAL), its controller, the alive bit
 * (0xE1 bit 4) -- and the controller's two virtuals it asks, IsAi (+0x30) and GetSealCtrlAi (+0x38),
 * and the AI word's bits 8 and 9 (0x264, read a byte at a time: the byte at 0x265) */
struct CTargetAi {
    unsigned char _0[0x265];
    unsigned char m_aibit8 : 1;   // 0x265 bit 0 (the word at 0x264's bit 8)
    unsigned char m_aibit9 : 1;   // bit 1
};

struct CTargetCtrl {
    virtual void Virtual08();
    virtual void Virtual0C();
    virtual void Virtual10();
    virtual void Virtual14();
    virtual void Virtual18();
    virtual void Virtual1C();
    virtual void Virtual20();
    virtual void Virtual24();
    virtual void Virtual28();
    virtual void Virtual2C();
    virtual bool IsAi() const;   // +0x30
    virtual void Virtual34();
    virtual CTargetAi *GetSealCtrlAi();   // +0x38
};

struct CTargetEntity {
    unsigned char _0[0x10];
    unsigned char m_type;   // 0x10 (2 a SEAL)
    unsigned char _11[0x1c - 0x11];
    CPnt3D m_posW;   // 0x1c (zentity.h's CEntity::m_posW)
    unsigned char _28[0x38 - 0x28];
    CPnt3D m_velW;   // 0x38 (CEntity::m_velW)
    unsigned char _44[0xc0 - 0x44];
    CTargetCtrl *m_control;   // 0xc0
    unsigned char _c4[0xc8 - 0xc4];
    unsigned int m_team;   // 0xc8 (CEntity::m_team: the sides it is on, a bit each)
    unsigned char _cc[0xe1 - 0xcc];
    unsigned char _e1 : 4;
    unsigned char m_alive : 1;   // 0xe1 bit 4

    /* an AI drives it (as zseal.h's CZSealBody::IsPlayer: the controller tested, then asked) */
    bool IsAi() { return m_control ? m_control->IsAi() : false; }
};

/* the entity as a SEAL, or 0 (an inline: the entity tested again) */
static inline CTargetEntity *SealOf(CTargetEntity *e)
{
    return e && e->m_type == 2 ? e : 0;
}

/* SOCOM 1's DWARF: CAiTarget, 0x64 bytes; SOCOM II's is 0x78 (its CTarget 8 bytes longer, two
 * counters added at 0x40) -- only what a written function touches */
class CAiTarget {
public:
    CAiTarget();
    float GetTimeSinceLastSighting() const;
    /* the time it was last seen (an inline: expanded ahead of the subtraction, its load first) */
    float GetTimeLostSight() const { return m_time_lost_sight; }

    /* the entity's last known place and velocity taken, and the time it came into or went out of
     * sight (OVL2 0x60A2F0; SOCOM 1's OnSetVisible -- the validity test, inlined, where SOCOM 1's
     * tested the entity and its alive bit; the position read in place, not through GetPosW) */
    void OnSetVisible(bool visible);
    /* the target given up (set to none) when it no longer counts or is on the entity's side
     * (OVL2 0x60A460; SOCOM 1's Tick: its validity test inlined, the team words at 0xC8 ANDed) */
    void Tick(CEntity *self, float dT);
    /* the target taken (OVL2 0x60A5F0; SOCOM 1's SetTarget without its threat argument: the
     * change test 0x60A6A0, the times and the last place) */
    void SetTarget(CTarget *t);

    union {
        CTargetEntity *m_ent;   // 0x0: the CTarget's entity
        CTarget m_target;   // 0x0 (zentity.h's, 0x30 bytes)
    };
    unsigned int m_aimBits;   // 0x30: bit 0 the target's hostile bit (0x800) kept (SOCOM 1's bits sat at 0x5C)
    unsigned char _34[0x40 - 0x34];
    int m_40;   // 0x40 (new in SOCOM II: set to 0x44's when the entity comes into sight)
    int m_44;   // 0x44
    unsigned char _48[0x4c - 0x48];
    float m_time_acquired;   // 0x4c (SOCOM 1's at 0x38)
    float m_time_visible;   // 0x50 (SOCOM 1's at 0x3C)
    float m_time_lost_sight;   // 0x54 (SOCOM 1's at 0x40)
    CPnt3D m_last_pos;   // 0x58 (SOCOM 1's at 0x44)
    CPnt3D m_last_vel;   // 0x64 (SOCOM 1's at 0x50)
    unsigned char _70[0x78 - 0x70];
};

/* the "target_pretick" task (CCoreState::Init adds it at 4.8, after the entities' pre-post-tick):
 * the visibility pool's rays registered with the world for its delayed intersection pass (OVL2
 * 0x60BC40, new in SOCOM II: CDiPool::PreTick inlined one level, its callees out of line -- the
 * request list's pieces seal_sensors.cpp emits, the world accessor 0x60BF30 and the DiIntersect
 * setter 0x60BF10, whose copies follow; the name unknown) */
extern "C" bool func_0060BC40(float dT, void *data)
{
    _diPool.PreTick(dT);
    return false;
}

/* the "target_post_tick" task (CCoreState::Init adds it at 10.6, after the route planner): the pool's
 * requests settled after the world's pass (OVL2 0x60B8F0, new in SOCOM II: CDiPool::PostTick inlined
 * one level -- its local list's constructor and destructor, the erase, the iterator's copy and
 * prefix step and the request test out of line after it, 0x60BAF0-0x60BC37; the name unknown) */
extern "C" bool func_0060B8F0(float dT, void *data)
{
    _diPool.PostTick();
    return false;
}

/* whether the target still counts: an entity, alive, and -- when it is a SEAL an AI drives -- that AI
 * with neither bit 8 nor bit 9 of its word at 0x264 set (OVL2 0x60A850, new in SOCOM II: some fifty
 * callers pass it the CAiTarget at controller+0x67C; where SOCOM 1's CAiSPursue::Tick tested the
 * target's entity and its alive bit inline, SOCOM II calls this. The integrator's suggested name,
 * CAiTarget::IsValid, is unproven -- SOCOM 1 has no such out-of-line member -- so the row keeps
 * its placeholder) */
extern "C" bool func_0060A850(const CAiTarget *t)
{
    bool r = false;
    CTargetEntity *e = t->m_ent;

    if (e && e->m_alive) {
        r = true;
        CTargetEntity *seal = SealOf(e);

        if (seal && seal->IsAi()) {
            CTargetAi *ai = seal->m_control->GetSealCtrlAi();

            if (ai)
                r = !(ai->m_aibit9 || ai->m_aibit8);
        }
    }
    return r;
}

/* what a target is worth to an entity, 0 out of range: an enemy 0.1, 0.3 more for flag 0x400 and 0.3
 * more when the vector to it lies along the target's own facing (its node's third row), halved when
 * the vector drops more than 20 below; a friend -1 -- all scaled down by the squared distance, to
 * nothing at 1000 (OVL2 0x60B7F0, new in SOCOM II: CSealCtrlAi's 0x5C9B30 calls it; the name unknown).
 * The constants are function statics (8-byte-spaced data, read as variables: -1 is loaded, not
 * built); the entity taken before the range test (the target loads it in the branch's delay slot),
 * the vector through a pointer, and the squared distance read twice in place -- a local for it
 * swaps its float register with the vector's y */
extern "C" float func_0060B7F0(const CEntity *self, const CTarget *t)
{
    static float maxDistSq = 1000000.0f;
    static float distScale = 0.000001f;
    static float enemy = 0.1f;
    static float flagged = 0.3f;
    static float facing = 0.3f;
    static float ally = -1.0f;
    static float below = -20.0f;
    static float belowScale = 0.5f;
    float r = 0.0f;
    CEntity *e = t->m_ent;

    if (t->m_distSq < maxDistSq) {
        if (e->IsEnemy(self)) {
            const CPnt3D *v = &t->m_vec;

            r = enemy;
            if (t->m_flags & 0x400)
                r += flagged;
            const float *fwd = e->m_node->m_matrix.m_matrix[2];

            if (v->x * fwd[0] + v->y * fwd[1] + v->z * fwd[2] > 0.0f)
                r += facing;
            if (v->y < below)
                r *= belowScale;
        } else
            r = ally;
        r *= 1.0f - distScale * t->m_distSq;
    }
    return r;
}

/* the time since the target was last seen (OVL2 0x60A590; SOCOM 1's, the same 0x14 bytes with the
 * field 0x14 on).
 * OPEN, 3 instructions: the target loads the field into $f0 first and the clock into $f1 after
 * (`sub.s $f0,$f1,$f0`); every spelling tried loads the clock first -- the clock as a float
 * global, a volatile, theMission's member, through an inline accessor or a local; the field
 * through a local, an inline accessor; a negated reverse difference (0x18 bytes). Round 22, all 3 off:
 * both in named locals in either order, `d -= field`, the plain member, both through accessors, a
 * static inline Since(t) / Elapsed(now, then) by value and by reference, and the unit built by
 * 3.0b50, 3.0b38 and 3.0b22-020926 through an `sc: compiler=` line -- MW loads the field first in
 * every form, but always colours the clock $f0. Round 28: MW's float colouring (the igdump tools)
 * has three vregs, coloured highest-numbered first -- the result, then the clock (f0), then the field
 * (f1): the field is numbered before the clock in every spelling, the clock through a static inline
 * MissionNow() with the field read in place or through its accessor and a Since(t) taking the field
 * by value included (all 3 off) */
float CAiTarget::GetTimeSinceLastSighting() const
{
    return theMission.m_time - GetTimeLostSight();
}

/* (OVL2 0x60A2F0; SOCOM 1's, its aim bits gone and the counter's copy new) */
void CAiTarget::OnSetVisible(bool visible)
{
    if (func_0060A850(this)) {
        m_last_pos = m_ent->m_posW;
        m_last_vel = m_ent->m_velW;
        if (visible) {
            m_time_visible = theMission.m_time;
            m_40 = m_44;
        } else
            m_time_lost_sight = theMission.m_time;
    }
}

/* the target's entity's place, none without one (an inline: SOCOM 1's SetTarget read GetPosW) */
static inline const CPnt3D *TargetPos(const CTarget *t)
{
    return t->m_ent ? &((CTargetEntity *)t->m_ent)->m_posW : 0;
}

/* two targets the same: one target, or both with the same entity (SOCOM 1's compare_target, an
 * inline here) */
static inline bool SameTarget(const CTarget *a, const CTarget *b)
{
    if (a == b)
        return true;
    if (!a || !b)
        return false;
    if (a->m_ent == b->m_ent)
        return true;
    return false;
}

/* whether the target changed, taken: copied from the one given (none clears the entity alone, as
 * SOCOM 1's CTarget assignment from a pointer did); a new one keeps its hostile bit (0x800) in the
 * word at 0x30's bit 0 and is lost to sight now, the same one gets the bit back and is lost to sight
 * when it stops being visible (bit 9) (OVL2 0x60A6A0; SOCOM 1's UpdateTarget's place, the name
 * unknown -- SetTarget calls it) */
extern "C" bool func_0060A6A0(CAiTarget *self, CTarget *t)
{
    bool changed = !SameTarget(t, &self->m_target);
    bool visible = (self->m_target.m_flags & 0x200) != 0;

    if (!t)
        self->m_target.m_ent = 0;
    else {
        self->m_target.m_ent = t->m_ent;
        self->m_target.m_vec = t->m_vec;
        self->m_target.m_distSq = t->m_distSq;
        self->m_target.m_dist = t->m_dist;
        self->m_target.m_visibility = t->m_visibility;
        self->m_target.m_aware = t->m_aware;
        self->m_target.m_unk28 = t->m_unk28;
        self->m_target.m_unk20 = t->m_unk20;
        self->m_target.m_unk24 = t->m_unk24;
        self->m_target.m_flags = t->m_flags;
    }
    if (t) {
        self->m_last_pos = *TargetPos(t);
        if (changed) {
            if (t->m_flags & 0x800)
                self->m_aimBits |= 1;
            else
                self->m_aimBits &= ~1;
            self->m_time_lost_sight = theMission.m_time;
        } else {
            if (self->m_aimBits & 1)
                t->m_flags |= 0x800;
            else
                t->m_flags &= ~0x800;
            if (visible && !(self->m_target.m_flags & 0x200))
                self->m_time_lost_sight = theMission.m_time;
        }
    }
    return changed;
}

/* (OVL2 0x60A5F0: Tick calls it out of line -- pinned, its ten statements would inline) */
#pragma auto_inline off
void CAiTarget::SetTarget(CTarget *t)
{
    bool changed = func_0060A6A0(this, t);

    if (changed) {
        m_time_acquired = theMission.m_time;
        m_time_visible = theMission.m_time;
    }
    if (t) {
        m_last_pos = *TargetPos(t);
        m_time_lost_sight = theMission.m_time;
    }
    if (changed)
        m_40 = m_44;
}
#pragma auto_inline reset

/* (OVL2 0x60A460) */
void CAiTarget::Tick(CEntity *self, float dT)
{
    if (!func_0060A850(this) || (((CTargetEntity *)m_ent)->m_team & ((CTargetEntity *)self)->m_team))
        SetTarget(0);
}
