/* seal_aifollow.cpp -- the follow state, CAiSFollow (SOCOM 1's unit, __sinit_seal_aifollow.cpp).
 * OVL2 0x5FADF0-0x5FE1EF: the destructor first, as SOCOM 1's. So far only the destructor and the
 * new slot +0x3C. Defined in reverse of their image order (-inline deferred). The kit's round 53 adds
 * the wedge test 0x5FE1F0 (OPEN) and its edge test 0x5FE4C0 (declared) here, between the follow and
 * seal_squirm.cpp's CSealCtrlSquirm (0x5FE830 on): their unit is unproven. */

#include "../gamez/zseal/zseal.h"

extern "C" float data_00650C38;   /* the follow's close range (a variable: read with lwc1) */
extern "C" float data_00650C28;   /* the follow's range on entry, 15.0 (a variable: read with lwc1) */

/* a cell's size in the world (OVL2 data 0x64FC60, 10.0: read as a variable) */
extern "C" float data_0064FC60;

/* a formation's slot as an offset in cells (OVL2 0x5EB910, seal_unit.cpp's range: the formation, the
 * slot, the offset written) */
extern "C" void func_005EB910(signed char formation, int slot, int *cells);

/* (field by field: the three-float constructor interleaves the two terms' products, the target
 * computes one after the other) */
inline void CAiSFollow::UpdateOffsetLength()
{
    CPnt3D v;

    v.x = m_5c[1] * data_0064FC60;
    v.y = 0.0f;
    v.z = -m_5c[0] * data_0064FC60;
    m_64 = v.Length();
}

/* the AI's route as the follow state reads it (zseal.h's CAiPathBytes at 0x46C: the two flag words;
 * the names ours) */
struct FOLLOW_PATH {
    /* `clear` cleared and `set` set in the word at 0x20 (inline: `&= ~0` stays a zero extension) */
    void SetFlags(unsigned int set, unsigned int clear)
    {
        m_20 &= ~clear;
        m_20 |= set;
    }

    unsigned char _0[0x1c];
    unsigned int m_1c;   // 0x1c
    unsigned int m_20;   // 0x20
};

extern "C" {
/* the AI's goals dropped (OVL2 0x5628F0, the SEALs' row), a path's plan aborted and its template
 * set (OVL2 0x5107B0, 0x527300: the AI core's) and a stance tested (OVL2 0x562FC0, the SEALs') */
void func_005628F0(CSealCtrlAi *ai);
void AbortPathPlan__2aiFP7CAiPath(FOLLOW_PATH *path);
void SetTemplatePath__7CAiPathFP7CAiPath(FOLLOW_PATH *path, FOLLOW_PATH *t);
bool InStance__11CSealCtrlAiF11SEAL_STANCEb(CSealCtrlAi *ai, SEAL_STANCE stance, bool b);
}

extern "C" {
/* a segment against a triangle's edge, in the XZ plane (OVL2 0x5FE4C0, unwritten: the edge's two
 * ends, then the segment's -- what it adds to 0x5FE1F0's count) */
int func_005FE4C0(const CPnt3D *e0, const CPnt3D *e1, const CPnt3D *s0, const CPnt3D *s1);
}

/* whether a segment crosses a wedge laid along a direction from a point: the wedge's apex `back`
 * behind, its far edge `len` ahead and `len * spread` to either side, the segment's ends taken
 * relative to the point; the edge tests summed until three agree (OVL2 0x5FE1F0, new in SOCOM II:
 * 0x5FD150 asks it with the follower's view; the name and the unit ours -- after the follow state,
 * before seal_squirm.cpp).
 * OPEN, 0x2BC against 0x2C8: every call, temporary and frame slot is the target's but one saved
 * register short -- the target keeps &tri[1], &tri[2] and &seg[1] in s1, s0 and s2 from the copy
 * into each (its x stored through the register, y and z sp-relative) to the edge tests, where MW
 * here recomputes each address at its call (`addiu`), saving s0-s3 only (frame 0x1E0 against
 * 0x1F0). Tried: pointer locals (declared at the top they sink to the calls, 0x2D8; declared at
 * each copy 0x2CC), reference locals (every store through the register, 0x2CC / 0x2D8), `tri + 1`
 * arguments (as it is), the edge tests as a loop (0x2C4) */
extern "C" bool func_005FE1F0(const CPnt3D *origin, const CPnt3D *dir, const CPnt3D *a, const CPnt3D *b,
                               float len, float back, float spread)
{
    CPnt3D side(-dir->z, 0.0f, dir->x);
    CPnt3D tri[3];
    CPnt3D seg[2];

    tri[0] = *dir * -back;
    float w = len * spread;

    tri[1] = *dir * len + side * w;
    tri[2] = *dir * len - side * w;
    seg[0] = *a - *origin;
    seg[1] = *b - *origin;

    int n = func_005FE4C0(&tri[0], &tri[1], &seg[0], &seg[1]);

    if (n < 3) {
        n += func_005FE4C0(&tri[1], &tri[2], &seg[0], &seg[1]);
        if (n < 3)
            n += func_005FE4C0(&tri[2], &tri[0], &seg[0], &seg[1]);
    }
    return n >= 3;
}

/* a follow's events: on entry mode 6, the goals dropped, the route marked and the AI-map collision
 * off, then the defaults -- the range 15, formation 1 slot 0 (SetFormation inlined), the flags
 * cleared, 0x76 set, the flag word 8; leaving (1) the route aborted and its template dropped, the
 * collision back, a prone SEAL crouched, the master let go and kept; 2 the route unmarked, mode 1;
 * 3 mode 6 again, the route marked, the step back to 0 (OVL2 0x5FDF80; SOCOM 1's entry and exit
 * reworked) */
void CAiSFollow::OnSignal(SIGNAL sig, CSealCtrlAi *ai)
{
    FOLLOW_PATH *path = ai ? (FOLLOW_PATH *)&ai->m_path : 0;

    switch (sig) {
    case SIGNAL_0:
        ai->SetAiMode((AI_MODE)6);
        func_005628F0(ai);
        path->SetFlags(1, 0);
        ai->SetDoAiMapCollide(false);
        m_68 = data_00650C28;
        SetFormation((UNIT_FORMATION)1, 0);
        m_78 = 0;
        m_77 = 0;
        m_75 = 0;
        m_7a = 0;
        m_79 = 0;
        m_76 = 1;
        m_84 = 0;
        m_80 = 0;
        m_7d = 0;
        m_7c = 0;
        m_6c = 8;
        break;
    case SIGNAL_1:
        AbortPathPlan__2aiFP7CAiPath(path);
        path->m_20 &= ~1;
        SetTemplatePath__7CAiPathFP7CAiPath(path, 0);
        path->m_1c &= ~8;
        ai->SetDoAiMapCollide(true);
        if (InStance__11CSealCtrlAiF11SEAL_STANCEb(ai, (SEAL_STANCE)2, false))
            ai->SetStance((SEAL_STANCE)1);
        ai->SetFollowMaster(0);
        m_master = ai->m_follow_master;
        break;
    case SIGNAL_2:
        path->m_20 &= ~1;
        ai->SetDoAiMapCollide(true);
        ai->SetAiMode((AI_MODE)1);
        m_master = 0;
        break;
    case SIGNAL_3:
        ai->SetAiMode((AI_MODE)6);
        path->SetFlags(1, 0);
        UpdatePhase(0);
        ai->SetDoAiMapCollide(false);
        m_master = 0;
        break;
    case SIGNAL_4:
        break;
    case SIGNAL_7:
        break;
    }
}

/* the remote console's word at 0xC of its +4 set (OVL2 0x5E6AA0, a named draft in seal_unit.cpp's
 * range; the name ours) */
extern "C" bool func_005E6AA0(CEntityCtrl *rc);

/* (OVL2 0x5FCC40; SOCOM 1's, the followers counted where SOCOM 1 tested the current flag, and a
 * remote SEAL's console asked) */
void CZSealBody::InvalidateFollowParams()
{
    if (m_followers <= 0) {
        m_saved_formation_dir_is_valid = 0;
        if (!IsPlayer()) {
            bool keep = false;

            if (IsRemote()) {
                CEntityCtrl *rc = m_control->GetRemoteControl();

                keep = rc ? !func_005E6AA0(rc) : false;
            }
            if (!keep)
                SetSnakePathUpdate(false, 50);
        }
    }
    m_follow_params_are_current = 0;
}

/* a SEAL's follow parameters (SOCOM 1's CZSealBody fields 0x158/0x15c and 0x420-0x459, here
 * 0x518/0x51c and 0x4dc-0x515: zseal.h keeps most of them as bytes -- the view ours, SOCOM 1's
 * names) */
struct CFollowParamsView {
    unsigned char _0[0x4dc];
    CPnt3D m_look_dir;   // 0x4dc
    CPnt3D m_formation_dir;   // 0x4e8
    CPnt3D m_saved_formation_dir;   // 0x4f4
    unsigned char _500[0x510 - 0x500];
    int m_formation_clock;   // 0x510
    unsigned char m_follow_params_are_current;   // 0x514
    unsigned char m_saved_formation_dir_is_valid;   // 0x515
    unsigned char _516[0x518 - 0x516];
    int m_MaxFollowerDistance;   // 0x518
    int m_MaxFollowerDistanceNext;   // 0x51c
};

/* the follow parameters brought up to date once a frame: the followers' distance rolled over, the
 * snake path's update started, and the formation's direction -- the SEAL's flat heading, or its flat
 * velocity once that has pointed within the threshold of the heading for TimeThresh milliseconds,
 * or the saved one (SOCOM 1's UpdateFollowParams with its function-local statics, guarded there and
 * plain data here: the thresholds in .data, the three running values in .bss; OVL2 0x5FC970 --
 * CAiSFollow::Tick calls it twice).
 * OPEN, 0x2C0 against 0x2C8 (the kit, round 52; every instruction in its place but the frame and
 * the registers): the target computes &dir.y and &dir.z once at the top (`addiu s3,sp,148` /
 * `addiu s2,sp,152`) and keeps them in saved registers past dir.Normalize() to every later read of
 * dir (the dot, the formation stores, the look) -- two more saved registers, 160 bytes; here MW's CSE
 * of those addresses dies at the first call (a1/v1 before it, `116(sp)`/`120(sp)` folded after),
 * as the target's own &vel.y does (v0 before the call, s1 after: right here). Field by field copies
 * and dir declared before vel give every other word (a struct copy loads all three first); tried
 * without effect: the negation, the copy or the flattening through inline helpers taking a
 * reference or a pointer, a reference or a pointer local for dir, the row as a CPnt3D pointer.
 * SOCOM 1's build keeps all four addresses (vel's too) from the top */
void CZSealBody::UpdateFollowParams(float dT)
{
    static float VelThreshSquared = 800.0f;
    static float cant;
    static float DotThresh = 0.82f;
    static float velsquared;
    static float dotVal;
    static int TimeThresh = 2000;
    CFollowParamsView *v = (CFollowParamsView *)this;

    if (v->m_follow_params_are_current)
        return;
    v->m_MaxFollowerDistance = v->m_MaxFollowerDistanceNext;
    v->m_MaxFollowerDistanceNext = 0;
    SetSnakePathUpdate(true, 75);
    CPnt3D dir;
    CPnt3D vel;

    vel.x = m_velW.x;
    vel.y = m_velW.y;
    vel.z = m_velW.z;
    vel.y = 0.0f;
    const float *row = m_node->m_matrix.m_matrix[2];

    dir.x = row[0];
    dir.y = row[1];
    dir.z = row[2];
    dir.x = -dir.x;
    dir.y = -dir.y;
    dir.z = -dir.z;
    dir.y = 0.0f;
    dir.Normalize();
    velsquared = Dot(m_velW, m_velW);
    bool keep = true;

    if (Dot(vel, vel) > VelThreshSquared) {
        cant = velsquared;
        vel.Normalize();
        dotVal = Dot(dir, vel);
        if (dotVal > DotThresh) {
            v->m_formation_clock += (int)(1000.0f * dT);
            if (v->m_formation_clock > TimeThresh) {
                vel.Normalize();
                keep = false;
                v->m_formation_dir = vel;
                v->m_saved_formation_dir = vel;
            }
        } else
            v->m_formation_clock = 0;
    } else
        v->m_formation_clock = 0;
    if (keep) {
        if (v->m_saved_formation_dir_is_valid)
            v->m_formation_dir = v->m_saved_formation_dir;
        else {
            v->m_formation_dir = dir;
            v->m_saved_formation_dir = dir;
        }
    }
    velsquared = cant;
    cant = dotVal;
    v->m_look_dir = dir;
    v->m_saved_formation_dir_is_valid = 1;
    v->m_follow_params_are_current = 1;
}

/* a direction to look in from a map spot, toward a point (OVL2 0x559580, the SEAL lane's range, not
 * written: seal_ai.cpp passes 0 for the point) */
extern "C" CAiMap::DIR func_00559580(const CAiMapLoc *loc, const CPnt3D *dir, unsigned int mask, int b, int n,
    int c);

/* whether a look of a mode may be set: none of a higher mode holds (seal_ai.cpp's inline; the name
 * ours) */
static inline bool LookAllowed(CSealCtrlAi *ai, AI_LOOK look)
{
    return look >= ai->m_look_mode;
}

/* whether the AI drives a SEAL, as a ternary (IsPlayer's shape: the call's result narrowed at the
 * join -- zseal.h's IsAi presets a bool; the name ours) */
static inline bool DrivenByAi(CZSealBody *seal)
{
    return seal->m_control ? ((CSealCtrl *)seal->m_control)->IsAi() : false;
}

/* a hold with its flags' bit 5 set (an inline bool, seal_airesponse.cpp's HoldBroken shape: the
 * self-assigning else arm keeps the flags test a branch, the kit lane's round 33; the name ours) */
static inline bool HoldBit5(const CAiState *s)
{
    bool r = false;

    if (s->m_state == AI_STATE_HOLD) {
        if (s->m_flags & 0x20)
            r = true;
        else
            r = r;
    }
    return r;
}

/* the follower's look while the follow hides, set once (flag 2): an escortee looks along the
 * follow's direction at 0xE8; a follower of an AI master holding with its bit 5 at the hold's point;
 * else the squad's look out of the direction at 0xB0, through the map (OVL2 0x5FDAD0, new in SOCOM
 * II: HideCheck's one call; the name ours). Byte-exact in the kit lane's round 34 with three
 * shapes: the team test read through a const view of the AI (the target loads m_entity twice --
 * once for the test, once into `self`; a plain read is merged), the master's AI taken into a const
 * local for the stack's ternary (read through a cast at each use, the call block's first word is
 * moved into the size test's delay slot where the target copies it and leaves the original), and
 * the ternary IsAi (zseal.h's presets a bool) */
bool CAiSFollow::SetHideLook(CSealCtrlAi *ai)
{
    bool done = (m_6c & 2) != 0;

    if (!done && LookAllowed(ai, AIL_NORMAL)) {
        CZSealBody *self = (CZSealBody *)ai->m_entity;
        bool set = false;

        if (((const CSealCtrlAi *)ai)->m_entity->m_team & 0x20000)
            set = ai->SetLookDirW(AIL_NORMAL, (const CPnt3D *)m_e8, true);
        else if (DrivenByAi(m_master)) {
            CSealCtrlAi *mai = m_master->GetSealCtrlAi();
            const CSealCtrlAi *cmai = mai;
            const CAiState *s = cmai->m_stateStack.size() == 0 ? 0 : cmai->m_stateStack.GetState();

            if (s ? HoldBit5(s) : false) {
                CAiState *h = mai->GetAiState();
                CAiSHold *hold = h && h->m_state == AI_STATE_HOLD ? (CAiSHold *)h : 0;

                if (hold && hold->m_active)
                    set = ai->SetLookPosW(AIL_CORPSE, &hold->m_pos);
            }
        } else {
            CPnt3D dir;

            ((CSealCtrl *)self->m_control)->m_unit->GetMyLookDir(&dir, (CPnt3D *)m_b0, self);
            set = ai->SetLookDirW(AIL_NORMAL, func_00559580(&self->m_mapLocValid, &dir, 0x40000, 0, 10, 0));
        }
        if (set) {
            ai->ModifyLookFlags(5, 0);
            m_6c |= 2;
            done = true;
        }
    }
    return done;
}

/* two cells' x and y distances summed (CAiPlanner::dist's shape, unscaled; the name ours) */
static inline int FollowCellDist(const CAiMapLoc &a, const CAiMapLoc &b)
{
    int dx = a.x > b.x ? a.x - b.x : b.x - a.x;
    int dy = a.y > b.y ? a.y - b.y : b.y - a.y;

    return dx + dy;
}

/* (OVL2 0x5FC7D0: the SEAL's offset from the follow's point computed and never read) */
bool CAiSFollow::HideCheck(CSealCtrlAi *ai, bool b, bool hide, char *out)
{
    bool r = hide;
    CPnt3D d;

    ((const CPnt3D *)m_88)->Sub(&ai->m_entity->m_posW, &d);
    if (m_79 && !hide)
        m_79 = 0;
    if (!m_79 && hide)
        m_79 = 1;
    if (hide && !b) {
        if (FollowCellDist(*(const CAiMapLoc *)&m_c0, *(const CAiMapLoc *)&m_c8) > m_c4) {
            *out = 0;
            r = false;
        }
    }
    if (r)
        SetHideLook(ai);
    return r;
}

/* (OVL2 0x5FCD30) */
CZSealBody *CSealCtrlAi::GetFollowMaster(bool recurse)
{
    CZSealBody *master = m_follow_master;

    if (master && recurse) {
        CSealCtrlAi *ai = master->GetSealCtrlAi();

        if (ai && ai != this) {
            CZSealBody *top = ai->GetFollowMaster(recurse);

            if (top)
                master = top;
        }
    }
    return master;
}

/* (OVL2 0x5FCF10) */
bool CSealCtrlAi::SetFollowMaster(CZSealBody *master)
{
    bool ok = false;
    CZSealBody *self = (CZSealBody *)m_entity;

    if (m_follow_master) {
        m_follow_master->RemoveFollower();
        m_follow_master = 0;
    }
    if (!master || self != master && master->m_isAlive) {
        m_follow_master = master;
        if (m_follow_master) {
            m_follow_master->AddFollower();
            m_follow_master_id = m_follow_master->m_id;
        }
        ok = true;
    }
    if (!m_follow_master) {
        CAiState *s = FindAiState(AI_STATE_FOLLOW);
        CAiSFollow *f = s && s->m_state == AI_STATE_FOLLOW ? (CAiSFollow *)s : 0;

        if (f) {
            f->m_active = 0;
            f->m_master = 0;
        }
    }
    return ok;
}

/* (OVL2 0x5FD0E0; CTState::Tick calls it) */
void CAiSFollow::SetOffset(const int *cells)
{
    m_5c[0] = cells[0];
    m_5c[1] = cells[1];
    UpdateOffsetLength();
}

/* (OVL2 0x5FD050; CSealCtrlAi::OnCommand and the squad's formation, 0x5EB6D0, call it) */
void CAiSFollow::SetFormation(UNIT_FORMATION formation, int slot)
{
    m_70 = slot;
    m_74 = formation;
    func_005EB910(m_74, m_70, m_5c);
    UpdateOffsetLength();
}

extern "C" {
/* the AI's check of an entity at a distance (OVL2 0x5FD150, this unit's, not written: the AI, the
 * entity, a flag, the distance; a bool) */
bool func_005FD150(CSealCtrlAi *ai, CEntity *e, bool b, float dist);
}

/* the AI's current state if it is a follow, else 0 (inline; the name ours) */
static inline const CAiSFollow *FollowState(const CSealCtrlAi *ai)
{
    const CAiState *s = ai->GetAiState();

    return s && s->m_state == AI_STATE_FOLLOW ? (const CAiSFollow *)s : 0;
}

/* the AI's mode, the follow's own when following (mode 6): 4, 3 or 2 by its two flags at 0x7C and
 * 0x7D (inline in 0x5FD620; the name ours) */
static inline AI_MODE FollowMode(const CSealCtrlAi *ai)
{
    AI_MODE m = ai->m_3f4;

    if (m == 6) {
        const CAiSFollow *f = FollowState(ai);

        if (f) {
            if (f->m_7c)
                m = (AI_MODE)4;
            else if (f->m_7d)
                m = (AI_MODE)3;
            else
                m = (AI_MODE)2;
        }
    }
    return m;
}

/* a path with points, planned (CAiPath::HasPoints as a preset bool: the emptiness test a materialised
 * inline bool, the state's under it with round 33's self-assigning else arm; the names ours) */
static inline bool PathNotEmpty(const CAiPath *p)
{
    return !(p->m_points.size() == 0);
}

static inline bool PathDone(const CAiPath *p)
{
    bool r = false;

    if (PathNotEmpty(p)) {
        if (p->m_state == CAiPath::COMPLETE)
            r = true;
        else
            r = r;
    }
    return r;
}

/* where a mode sends the AI, the point and its cell: modes 4 and 5 the goal (when one is set), 3
 * the path's last point (a planned path's), 6 the follow's own -- 4 or 3 by its two flags, asked
 * again; whether there is one (OVL2 0x5FD460, new in SOCOM II: 0x5FD620's call; the name unknown).
 * The cases in source order 6, 3, 4/5 (the tests run in reverse); the path through a const local
 * (the size read once for the test and the index, the state and the points through the pointer --
 * a plain pointer folds every field into the AI's offsets and reloads the size), the last point
 * tested for null in an if of its own (a ternary or an inline returning it keeps a zero arm) --
 * byte-exact in the kit lane's round 34 */
extern "C" bool func_005FD460(CSealCtrlAi *ai, CPnt3D *pos, CAiMapLoc *loc, AI_MODE mode)
{
    bool r = false;

    switch (mode) {
    case 6: {
        CAiState *s = ai->GetAiState();
        CAiSFollow *f = s && s->m_state == AI_STATE_FOLLOW ? (CAiSFollow *)s : 0;

        if (f) {
            if (f->m_7c)
                r = func_005FD460(ai, pos, loc, (AI_MODE)4);
            else if (f->m_7d)
                r = func_005FD460(ai, pos, loc, (AI_MODE)3);
        }
        break;
    }
    case 3: {
        const CAiPath *path = (const CAiPath *)&ai->m_path;

        if (PathDone(path)) {
            const CAiMapLoc *last = &path->m_points[path->m_points.size() - 1].m_loc;

            if (last && AiMaps.GetCellCenter(*last, pos)) {
                *loc = *last;
                r = true;
            }
        }
        break;
    }
    case 4:
    case 5:
        if (ai->m_goal_pos_valid) {
            *pos = ai->m_goal_pos;
            r = AiMaps.GetLocation(pos, *loc);
        }
        break;
    }
    return r;
}

/* (OVL2 0x5FD6F0; 0x556370 calls it -- the name unknown) */
extern "C" AI_MODE func_005FD6F0(const CSealCtrlAi *ai)
{
    return FollowMode(ai);
}

/* the follow step in the AI's mode (OVL2 0x5FD620; 0x556370 calls it -- the name unknown) */
extern "C" void func_005FD620(CSealCtrlAi *ai, CPnt3D *pos, CAiMapLoc *loc)
{
    func_005FD460(ai, pos, loc, FollowMode(ai));
}

/* the AI's check of an entity (0x5FD150) at the entity's distance from the SEAL; true when there is
 * none (OVL2 0x5FD3E0: CSealCtrlAi::PreTick hands it the player; the name unknown) -- the SEAL taken
 * into a local first: the target loads it ahead of the entity's null test */
extern "C" bool func_005FD3E0(CSealCtrlAi *ai, CEntity *e)
{
    bool r = true;
    CEntity *me = ((CEntityCtrl *)ai)->m_entity;

    if (e) {
        CPnt3D d = me->m_posW - e->m_posW;

        r = func_005FD150(ai, e, true, d.Normalize());
    }
    return r;
}

/* a SEAL's AI closing in on its goal: its bit 13 and its state's +0x3C, a bool through a local
 * (inline in CheckMaster; the name ours) */
static inline bool ClosingIn(CZSealBody *seal)
{
    bool r = false;
    CSealCtrlAi *ai = seal->GetSealCtrlAi();

    if (ai->m_aibit13 && ai->GetAiState()->Virtual3C(ai))
        r = true;
    return r;
}

/* (OVL2 0x5FDD40; CAiSFollow::Tick's first call -- the master tested alive, IsAi through the local, the
 * AI through m_master again after that call).
 * OPEN, 0x178 against 0x180: the shape is the target's (the result one variable, zeroed in the early
 * arm's delay slot; UpdatePhase(0) inlined; the inline's `r = false ... r = true` kept as daddiu and
 * narrowed into `close` with andi), but (1) the target materialises both bit tests before branching
 * (`sltu v0,zero,v0` after the navy SEAL bit and after bit 13 -- accessors, bool locals, `!= 0` and
 * casts all fold away here) and (2) numbers the master's AI between the result and `close` (s1; this
 * s3, the result s2, close s0) where ours gives it s3. Tried (build/scratch/kit/v_cm*.py, the kit
 * lane's round 29): the bit-13 test in the caller (the if-conversion makes it sltu/andi), an inline
 * on the AI or on the SEAL, `return a && b`, `if (...) return true; return false;`, nested ifs, the
 * inline's locals in both orders, the result preset or assigned in an else; round 30 (the shape that
 * closed GetFiremodeGraphic), the `&&`s as ternary conditions, in the caller, the inline or both:
 * 0x164-0x190. Round 35: the navy SEAL bit through zseal.h's IsNavySeal (`? true : false`) gives the
 * target's size, 0x180, the bit materialised (sltu) before its branch as the target's; 31
 * instructions left, the registers of (2) and the bit-13 test's materialisation (through a `? true :
 * false` accessor too it overshoots to 0x188; a plain inline returning the bit or its negation, 0x178
 * or 0x188) */
bool CAiSFollow::CheckMaster(CSealCtrlAi *ai)
{
    bool alive;
    CZSealBody *master = ai->m_follow_master;

    if (!master)
        alive = false;
    else {
        if (master != m_master)
            UpdatePhase(0);
        m_master = master;
        alive = master->m_isAlive;
        if (alive) {
            bool close = false;

            if (ai->IsNavySeal() && master->IsAi())
                close = ClosingIn(m_master);
            SetFlag(8, close);
        } else
            ai->SetFollowMaster(0);
    }
    return alive;
}

/* (OVL2 0x5FDD00) */
bool CAiSFollow::Virtual3C(CSealCtrlAi *ai)
{
    return m_f4 < data_00650C38 && (m_6c & 8) != 0;
}

/* the follower's stance kept with its master's (OVL2 0x5FC590: SOCOM 1's
 * CSealCtrlAi::HandleFollowTickStance(bool, bool, float) by its place, right before the hide check
 * in both builds -- a member of the state here, the float gone; CAiSFollow::Tick calls it; the
 * names ours). Nothing to do but stand when the entity's flag 0x20000 is set; else the master's
 * own stance (stand, crouch or prone, anything else the follower's) is the one to take -- asked to
 * (`match`) or moving (`moving`): crouching for a player master when the follower stands and
 * moves, then a duck where the AI finds one (after two to three seconds), else the master's; asked
 * neither, crouching under a player or a Navy SEAL's AI, standing otherwise. A stance that differs
 * is set, delayed */
/* a random time, the minimum plus a random part of the range (SOCOM 1's Rfloat's operator float
 * without its zero-range test: the 1.0 range's multiply stays -- written as a literal product the
 * front end folds it away; the name ours) */
static inline float FollowRand(float min, float range)
{
    float r = 4.656612873e-10f * (float)rand();

    return min + range * r;
}

void CAiSFollow::StanceCheck(CSealCtrlAi *ai, bool match, bool moving)
{
    SEAL_STANCE cur = ai->m_stance;
    float delay = -1.0f;
    SEAL_STANCE stance = cur;

    if (ai->m_entity->m_team & 0x20000)
        stance = (SEAL_STANCE)0;
    else {
        CZSealBody *master = m_master;
        SEAL_STANCE want;

        switch (master->GetStance(false)) {
        case 0:
            want = (SEAL_STANCE)0;
            break;
        case 1:
            want = (SEAL_STANCE)1;
            break;
        case 2:
            want = (SEAL_STANCE)2;
            break;
        default:
            want = cur;
            break;
        }
        if (match || moving) {
            if (moving && cur == 0) {
                if (master->IsPlayer())
                    stance = (SEAL_STANCE)1;
                else
                    stance = want;
            }
            if (ai->GetDuckStance(master, stance))
                delay = FollowRand(2.0f, 1.0f);
            else if (want != 0)
                stance = want;
        } else if (master->IsPlayer() || (ai->m_navyseal ? true : false))
            stance = (SEAL_STANCE)1;
        else
            stance = (SEAL_STANCE)0;
    }
    if (stance != cur)
        ai->SetStanceDelayed(stance, delay);
}

/* a record whose first byte's low five bits are a kind (the follow's Tick and 0x5FD790 test it;
 * the names ours) */
struct CAiFollowKind {
    unsigned char m_kind : 5;
};

/* whether the kind is 2, 4, 5 or 12 (OVL2 0x5FC550; the name ours): a bit test written out -- MW
 * lowers a switch over those cases to a compare chain -- true through a bool local (`andi v0,a0,
 * 0xff` from the shift's 1) */
extern "C" bool func_005FC550(const CAiFollowKind *k)
{
    bool r = false;
    unsigned int t = k->m_kind - 2;

    if (t < 11 && ((1 << t) & 0x40d))
        r = true;
    return r;
}

/* (OVL2 0x5FADF0) */
CAiSFollow::~CAiSFollow()
{
}
