/* ai_target.cpp -- the AI's target, CAiTarget, and SOCOM 1's ai::CheckLineOfFire: the stretch after
 * seal_airesponse.cpp, OVL2 0x609DF0-0x60BF3F (CAiTarget's constructor at 0x60A940; SEAL_AI_PARAMS's
 * at 0x60BF40 follows, seal_aiparams.cpp). SOCOM 1 had the same functions in the same place, right
 * after the response states' destructors; the unit's name is ours. So far the validity test, a
 * target's worth (0x60B7F0), one getter and, round 29, the visibility update 0x60AFA0 and, round 53,
 * ModifyAimDir (zkit.h included for Rfloat, code-neutral for the rest) and, round 55, the visibility
 * score 0x60AA20 with its two helpers 0x60B120 and 0x60B470 (int_material.h included for the
 * materials). Defined in reverse of their image order (-inline deferred). */

#include "../gamez/zentity/zentity.h"
#include "../gamez/znode/int_material.h"
#include "../gamez/zseal/seal_dipool.h"
#include "../gamez/zkit/zkit.h"

/* the mission's clock, which the target's times are taken on (theMission's float at 0xE0, OVL1
 * 0x4365C0: SOCOM 1's at 0x60 of its theMission, as its GetTimeSinceLastSighting reads it) --
 * a view of the mission, fts_state.h's CMission not reaching it */
struct CMissionVision {
    float m_fog;   // 0x0 (theMission+0x144): the fog's weight in the far bands' score
    unsigned char _4[0x1c - 0x4];
    float m_spot;   // 0x1c: the weight of the lights of kind 4
    float m_indoor;   // 0x20: their share indoors
    float m_shadow;   // 0x24: the shadow's scale
    float m_lit_shadow;   // 0x28: and again where lights of kind 3 reach
};
struct CMissionClock {
    unsigned char _0[0xe0];
    float m_time;   // 0xe0
    unsigned char _e4[0x144 - 0xe4];
    /* the visibility score's light and fog settings (SOCOM II's, read only by 0x60AA20; the names
     * ours) */
    CMissionVision m_vision;   // 0x144
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
    /* an aim direction thrown off by the target's noise, and normalised (OVL2 0x60A000) */
    void ModifyAimDir(CPnt3D *v, float accuracy, float angle);
    /* the target's range (an inline: ModifyAimDir's range is its result temporary, numbered after the
     * noise's -- read as m_target.GetRange() into a local, the local takes $f21 and the noise $f20) */
    float GetRange() { return m_target.GetRange(); }

    union {
        CTargetEntity *m_ent;   // 0x0: the CTarget's entity
        CTarget m_target;   // 0x0 (zentity.h's, 0x30 bytes)
    };
    unsigned int m_aimBits;   // 0x30: bit 0 the target's hostile bit (0x800) kept (SOCOM 1's bits sat at 0x5C)
    CPnt3D m_aim_noise;   // 0x34 (SOCOM 1's at 0x2C: ModifyAimDir adds it to the aim)
    /* 0x40 (new in SOCOM II: set to 0x44's when the entity comes into sight; while it lasts each
     * ModifyAimDir counts it down and aims wide of the target, whatever the accuracy) */
    int m_40;
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
 * its placeholder). IT RETURNS bool (the kit's round 59, asked by the SEALs' round 56, whose 0x562C80
 * declares it int): an int return over this bool local adds `andi v0,s1,0xff` before the epilogue
 * where the target moves s1 straight, and an int local turns `r = true` into `li` where the target
 * has the bool constant's `daddiu`; every caller in the kit's units compiles the same under either
 * declaration (objcmp --by-address: six of seven units the same, this one the only difference) --
 * the callers' `sltu v0,zero,v0` after the call is an `&&` materialising its operand, which a bool
 * return gives too (ShareTarget's TargetEngaged inline; 0x562C80 spelled `bool ok = f(t) && (flags &
 * 0x200);` is its own 17 under bool, as under int with `(bool)`) */
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

/* the cell a step from a cell in a direction reaches and its first layer's cell, and whether a step may
 * end on it; false off the map (inline -- too tall to inline whole: 0x60B470 inlines it one level and
 * calls GetMapId, GetCell and the cell's test out of line; the name ours). The map BY REFERENCE: taken
 * by pointer MW holds the addresses of its two dimensions in saved registers across the loop (s7 and
 * s8 more, 0x394), through the reference it reads them off the map's register as the target does */
static inline bool TraversableNeighbor(const CAiMap &map, const CAiMapLoc &loc, CAiMap::DIR dir, CAiMapLoc &out,
                                       CAiCell *&cell)
{
    int x = loc.x + CAiMap::neighbor[dir][0];
    int y = loc.y + CAiMap::neighbor[dir][1];

    if (x >= 0 && x < map.m_nx && y >= 0 && y < map.m_ny) {
        out.x = x;
        out.y = y;
        out.m_mapid = map.GetMapId();
        cell = map.GetCell(out, 0);
        return cell ? cell->IsTraversable() : false;
    }
    return false;
}

/* the shadow and the indoor share about a cell: the cell's and each neighbour's a step may end on,
 * averaged -- a shadow counted (its bits + 1) / 4, an indoor cell 1; nothing written without the cell
 * (OVL2 0x60B470, new in SOCOM II: the visibility score 0x60AA20 asks it for both ends of the ray; the
 * name unknown). The map's GetCell inlined one level (its pieces called: 0x512140, 0x512120, 0x512100,
 * 0x5120C0, 0x5120B0) */
extern "C" void func_0060B470(const CAiMapLoc &loc, float *shadow, float *inside)
{
    CAiMap *map = AiMaps.GetMapByIndex(loc.m_mapid);

    if (map) {
        float s = 0.0f;
        float in = s;
        CAiCell *cell = map->GetCell(loc, 0);

        if (cell) {
            s += 0.25f * (cell->GetShadow() + 1);
            in += cell->IsInside() ? 1.0f : 0.0f;
            CAiCell *c = 0;
            float n = 1.0f;

            for (int i = 0; i < 8; i++) {
                CAiMapLoc l;

                if (TraversableNeighbor(*map, loc, (CAiMap::DIR)i, l, c)) {
                    s += 0.25f * (c->GetShadow() + 1);
                    in += c->IsInside() ? 1.0f : 0.0f;
                    n += 1.0f;
                }
            }
            if (n > 0.0f) {
                float k = 1.0f / n;

                *shadow = s * k;
                *inside = in * k;
            }
        }
    }
}

/* a blend of two values by a share (inline: its product by the second kept -- 1.0 here, through the
 * parameter MW does not fold it, the target's mula.s / madd.s pair; the name ours) */
static inline float Blend(float a, float b, float t)
{
    return a * t + b * (1.0f - t);
}

/* how lit an entity stands for the eye: the lights of kinds 4 and 3 within a radius, the first weighed
 * down by the indoor share about its cell, all of it darkened by the shadow there (deeper where lights
 * of kind 3 reach), held within the limits (inline: the visibility score 0x60AA20 inlines it twice --
 * a SEAL's once a frame, kept at 0x42C --; its statics, 8-byte-spaced data before 0x60B120's, the
 * radius and the limits; the name ours) */
static inline float EntityLight(CEntity *e)
{
    static float radius = 75.0f;
    static float light_lo = 0.0f;
    static float light_hi = 2.0f;
    const CPnt3D *pos = &e->m_posW;
    float inside = 0.0f;
    float shadow = 1.0f;

    func_0060B470(e->m_mapLoc, &shadow, &inside);
    zdb::CWorld *w = zdb::CWorld::m_world;
    float spot = w->GetLightIntensity(pos, radius, 4);
    float ambient = w->GetLightIntensity(pos, radius, 3);
    const CMissionVision &p = theMission.m_vision;
    float mix = Blend(p.m_indoor, 1.0f, inside);

    if (ambient > 0.0f)
        shadow *= p.m_lit_shadow;
    shadow *= p.m_shadow;
    spot *= p.m_spot;
    float light = (1.0f - shadow) * (ambient + spot * mix);

    _saturate(&light, light_lo, light_hi);
    return light;
}

/* the ray's hits as the walk below leaves them: SOCOM 1's seal_sensors.cpp statics (diNonVolCount
 * 0x4760C0, diStopper 0x4760BC, range 0x508EE0, sorted 0x508F60 there), SOCOM II's 0x66BBC0, 0x66BBC8,
 * 0x66BBD0 and 0x66BC50 -- only 0x60B120 reads or writes them */
static int vis_nonvol_count;
static int vis_stopper;
static float vis_range[32];
static int vis_sorted[32];

/* whether any of the line of sight is left (inline: the walk's test materialises it, then branches on
 * its negation; the name ours) */
static inline bool Positive(float f)
{
    return f > 0.0f;
}

/* a node or one of its parents being a node (inline: materialised, `daddiu 1` on the found arm; the name
 * ours) */
static inline bool NodeUnder(const zdb::CNode *n, const zdb::CNode *root)
{
    const zdb::CNode *c = n;
    bool r = false;

    while (c) {
        if (root == c) {
            r = true;
            break;
        }
        c = c->GetParent();
    }
    return r;
}

/* what is left of the line of sight past the ray's hits, near to far (SOCOM 1's ComputeVisibility's
 * first half, its own function in SOCOM II: OVL2 0x60B120, the visibility score 0x60AA20 calls it with
 * the target's node; the name unknown): each hit's material -- none stops the ray dead -- until the
 * target's own node or one under it; a SEAL's body passes, an opaque material takes its opacity off,
 * a volumetric one a random share by the hit's squared distance (within 50, 300 and 500) */
extern "C" float func_0060B120(void *viewer, zdb::DiIntersect *di, zdb::CNode *node)
{
    static float opacity_lo = 0.0f;
    static float opacity_hi = 1.0f;
    static float near_sq = 2500.0f;
    static float mid_sq = 90000.0f;
    static float far_sq = 250000.0f;
    static float vis_lo = 0.0f;
    static float vis_hi = 1.0f;

    vis_nonvol_count = 0;
    if (!di)
        return 0.0f;
    int n = di->m_Cnt;
    float vis = 1.0f;

    di->SortNearToFar(vis_sorted, vis_range);
    for (int i = 0; Positive(vis) && i < n; i++) {
        int k = vis_sorted[i];
        float d = vis_range[k];

        vis_stopper = k;
        zdb::IntersectStruct *is = &di->m_Intersects[k];
        zdb::CNode *hit = is->m_node;
        CMaterialEx *mat = CMaterialExArray::GetMaterialEx(is->GetMaterial());

        if (node && NodeUnder(hit, node))
            break;
        if (!mat) {
            vis = 0.0f;
            vis_nonvol_count = 1;
            break;
        }
        if (!CZSealBody::GetSeal(hit)) {
            if (!(mat->m_volumetric ? true : false)) {
                vis_nonvol_count++;
                float t = mat->m_opacity;

                _saturate(&t, opacity_lo, opacity_hi);
                vis -= t;
            } else if (d < near_sq)
                vis -= Rfloat(0.5f, 1.0f);
            else if (d < mid_sq)
                vis -= Rfloat(0.25f, 0.65f);
            else if (d < far_sq)
                vis -= Rfloat(0.0f, 0.3f);
        }
    }
    _saturate(&vis, vis_lo, vis_hi);
    return vis;
}

extern "C" {
/* how well a viewer sees a target through its finished ray (OVL2 0x60AA20, this unit's, below:
 * SOCOM 1's CSealCtrl::ComputeVisibility(DiIntersect *, CTarget *) by its place in the same stretch,
 * with the viewer's context (a stack record the OVL1 turret code and the sensors build) first) */
float func_0060AA20(void *viewer, zdb::DiIntersect *di, CTarget *t);
}

/* a request registered with the world (inline: the visibility update tests it, materialised -- the
 * null arm first: `d ? ... : false` lays it out after; the name ours) */
static inline bool DirRegistered(const CDiPool::dir *d)
{
    return d == 0 ? false : d->m_state == 4;
}

/* a registered request's result taken: the request dropped, its DiIntersect handed back, or none
 * (inline; the name ours) */
static inline zdb::DiIntersect *DirTake(CDiPool::dir *d)
{
    if (d && d->m_state == 4) {
        d->m_state = 1;
        return d->m_index >= 0 ? &_diPool.m_pool[d->m_index] : 0;
    }
    return 0;
}

/* how long a request waited, none without one (inline; the name ours) */
static inline float DirAge(const CDiPool::dir *d)
{
    return d ? d->m_age : 0.0f;
}

/* the target's visibility updated from its finished ray (OVL2 0x60AFA0, new in SOCOM II where SOCOM 1's
 * CSealCtrl::SensorTick did it inline -- the OVL1 turret code 0x3A82C0 calls it; the name unknown): with
 * the request registered and its DiIntersect back, how well the viewer sees it (0x60AA20) blended
 * between the limits, scaled by the time the ray waited, added to the visibility, seen at 1, held within
 * the limits; else the visible bit (0x200) as it was */
extern "C" bool func_0060AFA0(void *viewer, CTarget *t)
{
    static float vis_min = -0.15f;
    static float vis_max = 1.15f;
    static float vis_seen = 1.0f;
    bool visible = (t->m_flags & 0x200) != 0;
    CDiPool::dir *d = (CDiPool::dir *)t->m_unk28;

    if (DirRegistered(d)) {
        t->m_unk28 = 0;
        zdb::DiIntersect *di = DirTake(d);

        if (di) {
            float age = DirAge(d);
            float x = func_0060AA20(viewer, di, t);

            x = vis_min * (1.0f - x) + vis_max * x;
            x *= age;
            x += t->m_visibility;
            visible = x >= vis_seen;
            _saturate(&x, vis_min, vis_max);
            t->m_visibility = x;
        }
    }
    return visible;
}

/* how well a viewer perceives a target (ai_perception.cpp's class, declared there in its unit: the
 * same members -- the constructor, SetRange and Compute out of line, the factor setters inline) */
class CAiPerception {
public:
    CAiPerception(unsigned int viewer, unsigned int target);
    void SetRange(float range);
    float Compute();

    unsigned int m_viewer;   // 0x0
    unsigned int m_target;   // 0x4
    unsigned char m_band;   // 0x8
    float m_factor[6];   // 0xc
    unsigned int m_valid;   // 0x24

    void SetValid(unsigned int set, unsigned int clear)
    {
        m_valid &= ~clear;
        m_valid |= set;
    }
    void SetFactor(int i, float v)
    {
        m_factor[i] = v;
        SetValid(1 << i, 0);
    }
};

/* the viewer's record (seal_sensors.cpp's CSensing, the OVL1 turret code's own): its entity and its
 * AI parameters */
struct CSensingView {
    CEntity *m_entity;   // 0x0
    AI_PARAMS *m_params;   // 0x4
};

/* a SEAL's light as the score last worked it out, and the frame it did (zseal.h's CZSealBody pads
 * them, 0x42C-0x433) */
struct CSealLightView {
    unsigned char _0[0x42c];
    float m_light;   // 0x42c
    unsigned int m_light_frame;   // 0x430: zVid's frame number, its low word
};

/* the entity as a SEAL, or 0 (inline: a ternary, the dead-else copy where the result is a new
 * variable; the name ours) */
static inline CZSealBody *SealEntity(CEntity *e)
{
    return e && e->m_type == 2 ? (CZSealBody *)e : 0;
}

/* a SEAL's light, worked out once a frame (inline: its EntityLight a level deeper takes the frame's
 * lower slots, as the target's; the name ours). The SEAL held in a one-member struct, a modelling
 * device: IRO_ScalarizeClassDataMembers makes its member's variable, numbered after every inline's locals
 * and the linearizer's temporaries and before the CSE pass -- where igreplay (rounds 56-57) wanted it */
static inline float SealLight(CZSealBody *seal)
{
    struct { CSealLightView *v; } sl;

    sl.v = (CSealLightView *)seal;
    if (zVid.frameNumber != sl.v->m_light_frame)
        sl.v->m_light = EntityLight(seal);
    sl.v->m_light_frame = zVid.frameNumber;
    return sl.v->m_light;
}

/* how well a viewer sees a target through its finished ray, -1 when nothing of the line is left
 * (SOCOM 1's CSealCtrl::ComputeVisibility by its place, its walk now 0x60B120; OVL2 0x60AA20, the
 * visibility update 0x60AFA0 asks it -- the name unknown): the perception's six factors -- the range
 * (a tenth through item 190), the target's speed, its visibility, its light (a SEAL's once a frame),
 * a SEAL's stance, the maps' line visibility -- scored, the far bands fogged, a target of flag 0x100
 * doubled, all scaled by the viewer's visual sensitivity. Written in the kit's round 55 (unwritten
 * before), from 0x56C to the target's size by four shapes: the indoor blend through an inline
 * (Blend(a, 1.0f, t): MW folds a literal `1.0f *` at once, a constant reaching a parameter keeps the
 * target's mula.s / madd.s), the final `* 1.0f` through a local (`scale`), the SEAL's light through
 * SealLight (its EntityLight a level deeper takes the frame's lower slots: shadow 204 / inside 208 for
 * the SEAL, 216 / 220 for the rest -- inside declared first), the fog's product from a local, the
 * light's position before the 0x60B470 call (the target's delay slot) and `spot *= p.m_spot` (the
 * product in the light's register). */
float func_0060AA20(void *viewer, zdb::DiIntersect *di, CTarget *t)
{
    static float flagged = 2.0f;
    CSensingView *v = (CSensingView *)viewer;
    CEntity *te = t->m_ent;
    CEntity *ve = v->m_entity;
    zdb::CCamera *cam = zdb::CWorld::m_world->m_camera;
    float vis = func_0060B120(viewer, di, te->GetNode());
    float r = -1.0f;
    float scale = 1.0f;
    AI_PARAMS *params = v->m_params;
    CAiPerception p(ve->m_id, te->m_id);

    if (vis > 0.0f) {
        float range = t->GetRange();
        CZSealBody *vs = SealEntity(ve);

        if (vs && vs->m_kit.IsUsingItem((EQUIP_ITEM)190))
            range *= 0.1f;
        p.SetRange(range);
        p.SetFactor(1, te->m_velW.Length());
        p.SetFactor(2, t->m_visibility);
        CZSealBody *ts = SealEntity(te);
        float light;

        if (ts) {
            switch (ts->GetStance(false)) {
            case 2:
                p.SetFactor(4, 0.0f);
                break;
            case 1:
                p.SetFactor(4, 0.5f);
                break;
            default:
                p.SetFactor(4, 1.0f);
                break;
            }
            light = SealLight(ts);
        } else
            light = EntityLight(te);
        p.SetFactor(3, light);
        p.SetFactor(5, AiMaps.ComputeLineVisibility(&di->m_Tail, &di->m_Tip, 1));
        r = p.Compute();
        float fog = theMission.m_vision.m_fog;

        if (fog != 0.0f && p.m_band >= 2)
        {
            float f = cam->GetFogVisibility(&di->m_Tail, &di->m_Tip);

            r *= f * fog;
        }
        if ((t->m_flags & 0x100) && r > 0.0f)
            r *= flagged;
    }
    r *= params->visual_sens;
    r *= scale;
    return r;
}

/* the time since the target was last seen (OVL2 0x60A590; SOCOM 1's, the same 0x14 bytes with the
 * field 0x14 on). Byte-exact in the kit lane's round 34 (open since round 7): the clock through a
 * same-type cast and the field read in place -- the HUD's round-33 rule; every earlier spelling (the
 * clock as a float global, a volatile, a local, an accessor; the field through its accessor; the
 * unit under 3.0b50/b38/b22) coloured the clock $f0 */
float CAiTarget::GetTimeSinceLastSighting() const
{
    return (float)theMission.m_time - m_time_lost_sight;
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

namespace ai {
bool CheckLineOfFire(const CEntity *e, const CTarget *t, float radius);
}

/* the aim thrown off and normalised: while the sighting count lasts it is counted down and the noise
 * drawn wide (a sideways throw of up to 25 and a drop of 12.5 to 25, over the target's range);
 * otherwise the accuracy saturated to [0, 1] -- a perfect aim has no noise, any other a throw of up
 * to 10 each way scaled by the inaccuracy, over the range; then the noise added to the aim (OVL2
 * 0x60A000; SOCOM 1's took the noise from ai::MakeNoise with the angle, which SOCOM II no longer
 * reads -- its random draws are Rfloat's) */
void CAiTarget::ModifyAimDir(CPnt3D *v, float accuracy, float angle)
{
    float range = GetRange();

    if (m_40 <= 0) {
        static float lo = 0.0f;
        static float hi = 1.0f;

        _saturate(&accuracy, lo, hi);
        if (accuracy == 1.0f)
            m_aim_noise = CPnt3D::zero;
        else if (range > 0.0f) {
            static float x = 10.0f;
            static float y = 10.0f;

            m_aim_noise.x = (1.0f - accuracy) * Rfloat(-x, x) / range;
            m_aim_noise.y = (1.0f - accuracy) * Rfloat(-y, y) / range;
            m_aim_noise.z = 0.0f;
        }
    } else {
        m_40--;
        if (range > 0.0f) {
            static float wide_x = 25.0f;
            static float wide_y = 25.0f;

            m_aim_noise.x = Rfloat(-wide_x, wide_x) / range;
            m_aim_noise.y = Rfloat(-wide_y * 0.5f, -wide_y) / range;
            m_aim_noise.z = 0.0f;
        }
    }
    v->Add(&m_aim_noise, v);
    v->Normalize();
}

/* whether the target is the first of the entity's targets met -- false once a live friend (a side
 * in common) stands within the radius of the line from the entity to the target, each place raised
 * by half a man's height (OVL2 0x609DF0; SOCOM 1's, which read the places through GetPosW -- SOCOM
 * II reads them straight, its targets 0x30 bytes). The result declared first and assigned before
 * the walk (initialised there it is zeroed at the top), the list through a reference (its address
 * kept in s1), each target's entity in a local (its sides loaded before the entity's own) */
bool ai::CheckLineOfFire(const CEntity *e, const CTarget *t, float radius)
{
    if (!t || !t->m_ent)
        return true;

    bool clear;
    const CTargetList &targets = e->m_targets;
    static float height = 18.0f;
    static CPnt3D offset(0.0f, 0.5f * height, 0.0f);
    CPnt3D from = e->m_posW + offset;
    CPnt3D to = *TargetPos(t) + offset;

    clear = false;

    for (CTargetList::const_iterator i = targets.begin(); i != targets.end(); ++i) {
        if ((*i).m_ent == t->m_ent) {
            clear = true;
            break;
        }
        CEntity *o = (*i).m_ent;

        if (o->m_isAlive && (o->m_team & e->m_team)) {
            CPnt3D p = o->m_posW + offset;
            float d;

            if (p.DistToVector(&from, &to, &d) && d < radius)
                break;
        }
    }
    return clear;
}
