/* seal_env.cpp -- the SEAL in its world: the terrain it can cross, the MP bomb, its animation sets
 * (SOCOM 1's unit, __sinit_seal_env.cpp; the press beta files ReadDeathAndFlinchAnims under it).
 * OVL2 from 0x59D980 (CanClimbWall, right after seal_dyn.cpp's template tail), in SOCOM 1's order.
 * Defined in reverse of their image order (-inline deferred). */

#include "zseal.h"
#include "../zanim/zanim.h"
#include "../zsys/zsys.h"
#include "../zweapon/zweapon.h"
#include <stdio.h>

/* the bomb's state machine (SOCOM 1's CZBombState, OVL1's) -- only the request */
class CZBombState {
public:
    enum EnumBombState { BOMB_STATE_0 = 0, BOMB_STATE_DEFUSING = 4 };

    static void RequestNewBombState(int id, EnumBombState state, const CPnt3D *pos);
};

/* CSequence, the timed sequence a look is, is zbody.h's; the looks themselves, CLook and CBlink, are
 * zseal.h's (SOCOM II's SEAL holds its three) */

extern "C" {
/* the blink's speed and its angle down (OVL1 data; SOCOM 1's were constants, 1.5 and -0.5925) */
extern float data_0044C2DC;
extern float data_0044C2E0;
}

/* a decal a SEAL can leave: its size range and its texture (SOCOM 1's DWARF: CZDecalDef, 0xc
 * bytes) */
class CZDecalDef {
public:
    /* empty and user-declared, as zwep_weapon.cpp's: the vector's reserve (0x5B71B0) and insert
     * (0x5B6CE0) then copy an element member by member (lwc1/swc1, lwc1/swc1, lw/sw) where an
     * implicit one is copied as a block (the MSL headers round 17) */
    CZDecalDef() {}

    float m_decalminsize;   // 0x0
    float m_decalmaxsize;   // 0x4
    zdb::CTexHandle *m_texture;   // 0x8
};

/* the map's mission-compromised valve (OVL1 data 0x436688), the message queue (0x4366A0) and the
 * switch for the online game (0x45A0C1) */
extern "C" {
extern CValve *data_00436688;
extern unsigned char data_004366A0[];
extern unsigned char data_0045A0C1;
}

enum MSGTYPE { MSGTYPE_PLAYER = 0, MSGTYPE_SYSTEM = 1, MSGTYPE_ALERT = 5 };

class C2DMessage_Q {
public:
    void Add_Message(const char *text, MSGTYPE type, bool centered, int time, float scale);
};

/* a graph edge: the teams that have crossed it (graph_tree.cpp's; ai_maps.cpp's view of it) */
class CGEdge {
public:
    /* (an inline, the name ours: `&= ~0` through the parameter stays as a zero extension) */
    void SetMask(unsigned int set, unsigned int clear)
    {
        m_mask &= ~clear;
        m_mask |= set;
    }

    unsigned char _0[0x18];
    unsigned int m_mask;   // 0x18
};

/* the squad's reaction to a player crossing a boundary (OVL2 0x5EB8A0, seal_unit.cpp's) */
extern "C" void func_005EB8A0(CSealUnit *unit);

/* the off-the-map bit of the controller (an inline, the name ours: the value through a bool, the zero
 * then `andi a0,zero,1`) */
static inline void SetOffMap(CSealCtrl *ctrl, bool off) { ctrl->m_bit16 = off; }

/* where the SEAL is on the AI maps: the cell it stands in, kept when it can be walked (SOCOM II's slot
 * 0x48 of its table; OVL2 0x5B7BA0, past this unit's template tail: the unit's first definition) */
void CZSealBody::UpdateAiMapLoc()
{
    bool found;
    bool moved;
    CAiMapLoc valid(m_mapLocValid);
    CAiMapLoc prev(m_mapLoc);
    CAiMapLoc loc;
    CSealCtrl *ctrl = (CSealCtrl *)m_control;

    moved = false;
    found = AiMaps.GetLocation(&m_posW, loc);

    if (found) {
        m_mapLoc = loc;
        moved = valid.x != loc.x || valid.y != loc.y;
        CAiMap *map = AiMaps.GetMap(loc);

        if (map ? map->IsTraversable(loc) : false) {
            if (moved || m_bit14) {
                if (m_control ? ((CSealCtrl *)m_control)->IsAi() : false)
                    ((CSealCtrl *)m_control)->GetSealCtrlAi()->m_624 = valid;
                m_mapLocValid = loc;
                if (m_snake_path) {
                    static unsigned int k_search = 5;

                    m_snakePath.AddSnakePoint(loc, k_search, m_bit14 ? 8 : 0);
                }
                if (m_isAlive)
                    AiMaps.SetOccupied(valid, loc, m_id);
            }
            m_504 = m_posW;
            SetOffMap(ctrl, false);
        } else {
            static int k_x = 0;
            static int k_y = 0;
            int one = 1;
            int dx = m_mapLocValid.x - loc.x;
            int dy = m_mapLocValid.y - loc.y;

            ctrl->m_bit16 = (_close_enough(dx, k_x, one) && _close_enough(dy, k_y, one)) ? 0 : 1;
        }
    } else
        SetOffMap(ctrl, true);
    if (found && IsPlayer() && !data_0045A0C1) {
        unsigned int before = AiMaps.GetBoundary(prev);
        unsigned char now = AiMaps.GetBoundary(loc);
        bool changed = (unsigned char)before != now;
        CSealUnit *unit = ((CSealCtrl *)m_control)->m_unit;

        if (unit)
            func_005EB8A0(unit);
        if (moved && prev.m_mapid != loc.m_mapid) {
            CGEdge *edge = AiMaps.FindLinkEdge(prev, loc);

            if (edge)
                edge->SetMask(m_team, 0);
        }
        switch (now) {
        case 2:
            if (changed) {
                const char *line0 = "Mission was compromised.  You have";
                const char *line1 = "left the designated mission area.";

                CSealCtrlAi::StopAiSound((AI_SOUND)1);
                CSealCtrlAi::PlayAiSound((AI_SOUND)0);
                data_00436688->SetValue(1);
                C2DMessage_Q *q = (C2DMessage_Q *)data_004366A0;

                if (q) {
                    q->Add_Message(line0, MSGTYPE_PLAYER, true, 0, 0.0f);
                    q->Add_Message(line1, MSGTYPE_PLAYER, true, 0, 0.0f);
                }
            }
            break;
        case 1:
            if (changed) {
                const char *line0 = "WARNING!";
                const char *line1 = "Leaving designated mission area!";
                const char *line2 = "In danger of compromising the mission.";

                CSealCtrlAi::StopAiSound((AI_SOUND)0);
                CSealCtrlAi::PlayAiSound((AI_SOUND)1);
                C2DMessage_Q *q = (C2DMessage_Q *)data_004366A0;

                if (q) {
                    q->Add_Message(line0, MSGTYPE_SYSTEM, true, 0, 0.0f);
                    q->Add_Message(line1, MSGTYPE_SYSTEM, true, 0, 0.0f);
                    q->Add_Message(line2, MSGTYPE_SYSTEM, true, 0, 0.0f);
                }
            }
            break;
        case 0:
            if (changed) {
                CSealCtrlAi::StopAiSound((AI_SOUND)0);
                CSealCtrlAi::StopAiSound((AI_SOUND)1);
            }
            break;
        }
    }
}

extern "C" int rand(void);
zdb::CNode *GetDecalNode(zdb::CNode *node);
/* (SOCOM 1's GetVisualNode__FPQ23zdb5CNode, called where SOCOM 1's HandleBloodDrip calls it; OVL1
 * 0x3D0FD0, unowned) */
zdb::CNode *GetVisualNode(zdb::CNode *node);
/* (zrdr.h's: a block's tag) */
_zrdr *zrdr_findtag(const _zrdr *rdr, const char *tag);

#include <iostream>

/* the unit's statics, in the order its initialiser (__sinit_seal_env.cpp, OVL2 0x668C10) builds them
 * after <iostream>'s two objects -- SOCOM 1's file statics by its symbol table, less the string
 * table, the decal defs and the look's spine tables SOCOM 1's initialiser builds besides (the SEALs,
 * round 40): the four facing cosines, 8 bytes apart in OVL2 data from 0x65F0C8 (SOCOM 1's
 * FLASHBANG_DOT, RESTRAINT_DOT, KNIFE_DOT and CQB_DOT: 40, 85, 25 and 65 degrees -- the first's name
 * is seal_control.cpp's FLASHBANG_DOT's in the map, so it keeps its placeholder; nothing reads it),
 * the matrix stack that owns nothing (OVL2 bss 0x66B468: SOCOM 1's _stack; its ~CStack copy 0x5B7B20
 * is the unit's), the two intersection sets of 64 (0x66B490, 0x66B4F0: SOCOM 1's di0 and di1), the
 * fallback's two cosines (0x65F0E8, 0x65F0F0: SOCOM 1's FALLBACK_DOT_MIN / MAX, 175 and 355 taken
 * through 180/pi -- degrees made larger, not radians, in both builds; nothing reads them) and the
 * decals' projection, straight down (0x66B540) */
static float data_0065F0C8 = cosf(0.017453292f * 40.0f);
static float RESTRAINT_DOT = cosf(0.017453292f * 85.0f);
static float KNIFE_DOT = cosf(0.017453292f * 25.0f);
static float CQB_DOT = cosf(0.017453292f * 65.0f);
static CStack _stack(false);
static zdb::DiIntersect di0(64);
static zdb::DiIntersect di1(64);
static float FALLBACK_DOT_MIN = cosf(175.0f * 57.295776f);
static float FALLBACK_DOT_MAX = cosf(355.0f * 57.295776f);
static CPnt3D projDirDown(0.0f, -1.0f, 0.0f);

/* a random fraction of one (rand() over 2^31) */
static inline float Rand01()
{
    return 4.656612873e-10f * (float)rand();
}

/* a size in a decal's range (an inline: its arguments are loaded before rand() runs; the range
 * is lost -- (hi - hi) -- in both builds). The sum assigned back over `hi` -- a parameter reused is
 * computed where it is assigned, so HandleBloodDrip has it before GetDecalNode is called (the target's
 * add in that call's delay slot, $f22 never saved: returned as an expression MW sinks it below the
 * call, 4 bytes long) -- and the random value a local first (in place it swaps the mul's operands) */
static inline float RandSize(float lo, float hi)
{
    float r = Rand01();

    hi = lo + r * (hi - hi);
    return hi;
}

/* a material's extension (int_material.cpp's, held in zweapon.h's CMaterialExArray): only the
 * footfall sounds by gait (the names ours) */
struct CMaterialEx {
    char *m_name;   // 0x0 (ReInitDatabaseHandles' footfall anims are named after it)
    unsigned char _4[8 - 4];
    CSnd *m_runsound;   // 0x8
    CSnd *m_walksound;   // 0xc
    CSnd *m_pronesound;   // 0x10
    CSnd *m_fallsound;   // 0x14: a landing's (HandleFallingDamage plays it)
    unsigned char _18[0x24 - 0x18];
    float m_penetration;   // 0x24: PENETRATION (int_material.h's)
    unsigned char _28[0x38 - 0x28];
    float m_foot_step_offset;   // 0x38: FOOT_STEP_OFFSET (int_material.h's: the ground probes add it to a hit)
    /* int_material.h's flags: VOLUMETRIC (bit 0), LIQUID (bit 1) and PICKUP (bit 4) here */
    unsigned int m_volumetric : 1;   // 0x3c
    unsigned int m_liquid : 1;   // bit 1
    unsigned int m_underwater : 1;   // bit 2: UNDERWATER
    unsigned int m_particle : 1;   // bit 3: a particle system's (HandleSmokeAndGas tests it)
    unsigned int m_pickup : 1;   // bit 4: PICKUP
    unsigned int m_flags5 : 27;

    /* (inlines written `? true : false`: the target converts the bit, sltu, before its test --
     * `!= 0`, a plain return or a cast fold it away) */
    bool IsPickup() { return m_pickup ? true : false; }
    bool IsVolumetric() { return m_volumetric ? true : false; }
    bool IsLiquid() { return m_liquid ? true : false; }
    bool IsParticle() const { return m_particle ? true : false; }
};

/* a material's extension, the first one for an index past the table (SOCOM 1's bounds-checked
 * read; the name ours) */
static inline CMaterialEx *MaterialEx(unsigned int i)
{
    if (i >= CMaterialExArray::m_materialex_list.size())
        i = 0;
    return (CMaterialEx *)CMaterialExArray::m_materialex_list[i];
}

/* a footfall's sound at a point, by the material underfoot and the gait -- prone, a creep (each of
 * the three throttles within a half) or a run -- heard flat by the player looking through his own
 * eyes (OVL2 0x5A39B0; SOCOM 1's tested two throttles and played it at the point always) */
void CZSealBody::HandleFootfallSound(CPnt3D *pos)
{
    int mat = m_onMaterial;

    if (mat < (int)CMaterialExArray::m_materialex_list.size()) {
        CMaterialEx *ex = MaterialEx(mat);

        if (ex) {
            CSnd *snd;

            if (GetStance(false) == stateProne)
                snd = ex->m_pronesound;
            else {
                float x = m_control->m_throttle[0];
                float y = m_control->m_throttle[1];
                float z = m_control->m_throttle[2];

                if (x > 0.5f || y > 0.5f || x < -0.5f || y < -0.5f || z > 0.5f || z < -0.5f)
                    snd = ex->m_runsound;
                else
                    snd = ex->m_walksound;
            }
            if (snd) {
                if (IsPlayer() && m_zoomstate > ZOOMSTATE_3RDPERSON) {
                    snd->PlaySound((CSndInstance *)0, 255);
                    return;
                }
                snd->PlaySound3D(pos, 1.0f, 0, 0, 255);
            }
        }
    }
}

/* now and then a wounded SEAL drips blood: offline only, a random decal of the set at random size
 * under it, along its altitude hit (OVL2 0x5A33B0; SOCOM 1's word for word, the timer at 0x214 --
 * its size range is min + r * (max - max), as both builds have it). Byte-exact since the SEALs' round
 * 41 (0x1BC against 0x1B8 since round 13): RandSize assigns its sum back over its `hi` parameter
 * (above). The early return and the zero-first pick (`if (n == 0) def = 0; else ...`) are the
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
        zdb::CNode *node = GetVisualNode(alt->m_node);

        if (node) {
            CPnt3D dir;

            alt->m_norm.Scale(-1.0f, &dir);
            float size = RandSize(def->m_decalminsize, def->m_decalmaxsize);

            node->ApplyDecal(GetDecalNode(alt->m_node), 100.0f, &alt->m_pos, &dir, &projDirDown,
                             def->m_texture, size, 0);
        }
    }
}

extern "C" {
/* the decal textures by material (OVL1 bss: a buffer of handles UninitDatabaseHandles frees) */
extern void *data_0044D550;
}

/* (the footprint, below) */
extern "C" void func_005A3280(CZSealBody *seal, CPnt3D *pos);

extern "C" {
/* the footfalls' anims by material (OVL1 bss 0x44D548: ReInitDatabaseHandles fills it) */
extern void *data_0044D548;
float floorf(float);
void *func_00555170(void *stack);
bool func_005551A0(unsigned short flags, unsigned short mask);
}

/* a step's footfall at a foot's place, when asked of the walk's phase: its anim on the material underfoot,
 * then (out of the state 2 for the left, always for the right) its sound and print (an inline for each foot,
 * as SOCOM 1's; the name ours) */
void CZSealBody::HandleSealStep()
{
    if (!(m_animate_footsteps ? true : false))
        return;
    if (!m_lfoot || !m_rfoot)
        return;

    float t = ((CSealCtrl *)m_control)->m_throttle[2];

    if (m_state != 0 || GetSpeedState() < 2) {
        if (m_state == 1 && !(m_velM.x * m_velM.x + m_velM.y * m_velM.y + m_velM.z * m_velM.z <= 0.25f))
            goto walking;
        if (m_state == 2 && !(m_velM.x * m_velM.x + m_velM.y * m_velM.y + m_velM.z * m_velM.z <= 0.25f))
            goto walking;
        if (m_state == 9 && !(m_velM.x * m_velM.x + m_velM.y * m_velM.y + m_velM.z * m_velM.z <= 0.25f))
            goto walking;
        if (t < 0.1f && !(t <= -0.1f))
            return;
    }
walking:
    if (!func_005551A0(((CZBodyAnimBlend *)func_00555170((char *)this + 0x1c0))->m_flags, 64))
        return;
    t = ((CZBodyAnimBlend *)func_00555170(&m_blends))->m_tfrac;
    t -= floorf(t);
    if (t > 0.0f && t < 0.5f) {
        if (!m_didlstep && !m_airborne) {
            CPnt3D p;

            PartInSpace(m_lfoot, 0, &p, 0);
            m_quat.Apply(&p, &p, 1);
            p = p + m_posW;
            m_didlstep = 1;
            if (m_onMaterial < (int)CMaterialExArray::m_materialex_list.size()) {
                CZAnim *a = ((CZAnim **)data_0044D548)[m_onMaterial];

                if (a)
                    a->ActivateArg(1, 5, &p, 0);
            }
            if (m_state != 2) {
                HandleFootfallSound(&p);
                func_005A3280(this, &p);
            }
        }
    } else if (m_didlstep)
        m_didlstep = 0;
    if (t > 0.5f && t < 1.0f && !m_airborne) {
        if (!m_didrstep) {
            CPnt3D p;

            PartInSpace(m_rfoot, 0, &p, 0);
            m_quat.Apply(&p, &p, 1);
            p = p + m_posW;
            m_didrstep = 1;
            if (m_onMaterial < (int)CMaterialExArray::m_materialex_list.size()) {
                CZAnim *a = ((CZAnim **)data_0044D548)[m_onMaterial];

                if (a)
                    a->ActivateArg(1, 5, &p, 0);
            }
            HandleFootfallSound(&p);
            if (m_state != 2)
                func_005A3280(this, &p);
        }
    } else if (m_didrstep)
        m_didrstep = 0;
}

/* a footprint left at a point: on the polygon the SEAL last stood on, unless its decal node refuses
 * decals, the texture its material names, 3.5 deep, turned as the hips face (OVL2 0x5A3280, just
 * before HandleBloodDrip; new in SOCOM II, the name ours) */
extern "C" void func_005A3280(CZSealBody *seal, CPnt3D *pos)
{
    zdb::CNode *node;
    zdb::CNode *decal;
    zdb::IntersectStruct *alt = &seal->m_last_altitude;

    if (!alt->m_poly)
        return;
    node = GetVisualNode(alt->m_node);
    if (!node)
        return;
    decal = GetDecalNode(alt->m_node);
    if (decal && (decal->m_socom2_a1 & 4))
        return;
    int mat = alt->GetMaterial();

    if (!((zdb::CTexHandle **)data_0044D550)[mat])
        return;
    CPnt3D dir;

    alt->m_norm.Scale(-1.0f, &dir);
    zdb::CNode *hips = seal->m_hips->m_node;
    CMatrix m;

    hips->m_matrix.Multiply(&seal->m_node->m_matrix, &m);
    node->ApplyDecal(decal, 100.0f, pos, &dir, &projDirDown, ((zdb::CTexHandle **)data_0044D550)[mat],
                     3.5f, (CPnt3D *)m.m_matrix[2]);
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
extern "C" {
/* an anim's reference point, or the origin (OVL2 0x58C2F0, seal_cfg.cpp's) */
bool func_0058C2F0(CZSealBody *seal, CZSIScriptCommon *anim, CPnt3D *out);
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

/* (where a hit lands: zseal.h's DAMAGE_LOCATION) */

/* an animation set's anims by the item in hand (anim.cpp's CAnimsByFP: only the setter) */
class CAnimsByFP {
public:
    void SetPistolAnim(AnimType rifle, AnimType pistol);
};

/* a list of anim types (SOCOM 1's vector<AnimType>): SOCOM II's MSL runs a 2-byte element on
 * __vector_pod<unsigned short> (the insert's name, OVL2 0x5B68E0), and an element read back is
 * the signed type again (lh). include/vector's 2-byte pod gives this unit's tail its insert
 * (0x5B68E0, push_back's) and reserve (0x5B6BD0) byte-exact (the MSL headers round 16) */
class CAnimTypeVector : public std::__vector_pod<unsigned short, std::allocator<unsigned short> > {
    typedef std::__vector_pod<unsigned short, std::allocator<unsigned short> > __base;

public:
    void push_back(const AnimType &x) { __base::push_back((const unsigned short &)x); }
    unsigned int size() const { return size_; }
    AnimType &operator[](unsigned int n) { return ((AnimType *)data())[n]; }
};

/* a SEAL's animations (SOCOM 1's DWARF: AnimSet) -- its readers of the damage animations, static:
 * each reads a location's block, its STAND, CROUCH and PRONE lists; the tables they fill, by
 * location and state (SOCOM 1's six static members of 0xD8 bytes, in its order; OVL1 bss
 * 0x44CE70-0x44D3F7 here), the sets, and a set's anims by the item in hand */
class AnimSet {
public:
    static void ReadFlinchAnimations(_zrdr *rdr, const char *name, DAMAGE_LOCATION loc);
    static void ReadDeathAnimations(_zrdr *rdr, const char *name, DAMAGE_LOCATION loc);
    static void ReadTeammateDeathAnimations(_zrdr *rdr, const char *name, DAMAGE_LOCATION loc);
    static void InitDeathAndFlinchAnimations();

    static CAnimTypeVector deathanim[6][3];
    static CAnimTypeVector pistoldeathanim[6][3];
    static CAnimTypeVector teammatedeathanim[6][3];
    static CAnimTypeVector teammatepistoldeathanim[6][3];
    static CAnimTypeVector damageanim[6][3];
    static CAnimTypeVector pistoldamageanim[6][3];
    /* every set (SOCOM 1's AnimSet::List, a pointer vector; OVL1 bss 0x44D430) */
    static std::vector<AnimSet *> list;
    /* a type's motion script with no weapon (anim.cpp's, OVL2 0x5E3D90) */
    CZSIScriptCommon *GetScript(AnimType type);

    unsigned char _0[0x5c];
    CAnimsByFP *m_animsbyfp;   // 0x5c (SOCOM 1's m_animgps at 0x60)
};

char *zrdr_findstring(const _zrdr *rdr, const char *tag);
AnimType FindAnimType(const char *name);

/* a ladder ahead: a probe from the root part's height on the SEAL's spot, raised to the climb's (anim
 * 46's) reference point less one, eleven forward along the node's facing, the SEAL's node switched off
 * for it -- a ladder polygon hit (application flags 2) or not (OVL2 0x5B0EB0; SOCOM 1's, PostCheckDILadder
 * calls it) */
bool CZSealBody::CheckDILadder()
{
    CPnt3D fwd(0.0f, 0.0f, -11.0f);
    const CPnt3D *pos = &m_posW;
    CPnt3D root;
    CPnt3D from;
    CPnt3D to;
    CPnt3D ref;

    root = *(CPnt3D *)GetRootPart()->m_node->m_matrix.m_matrix[3];
    func_0058C2F0(this, m_animset->GetScript((AnimType)46), &ref);
    ref.y -= 1.0f;
    root.y += ref.y;
    from = *pos;
    from.Add(&root, &from);
    TransformVectors(&m_node->m_matrix, &fwd, &to, 1);
    to.Add(&from, &to);
    bool active = GetNode()->m_active;

    GetNode()->SetActive(false);
    zdb::IntersectStruct *hit = zdb::CWorld::m_world->GetIntersect(&from, &to);

    GetNode()->SetActive(active);
    if (!hit || ((unsigned char)hit->m_poly->m_flags.m_appflags & 7) != 2)
        return false;
    return true;
}

/* an array's item by an unsigned index (zrdr.h's Item, which this unit cannot include beside
 * zanim.h's _zrdr: the index unsigned keeps MW from strength-reducing an int counter's walk) */
static inline _zrdr *ItemAt(_zrdr *a, unsigned int i)
{
    return i < a->Count() ? &a->array[i] : 0;
}

extern "C" {
/* one animation of a list read into the location's and the state's tables (its rifle and its
 * pistol type) and the pistol type set as the rifle type's in the first set: a flinch, a death, a
 * teammate's death (OVL2 0x5A3D30, 0x5A3E90, 0x5A3FF0, defined after the readers; SOCOM 1's
 * readers did it inline) */
void func_005A3D30(_zrdr *anim, SEAL_STATE state, DAMAGE_LOCATION loc);
void func_005A3E90(_zrdr *anim, SEAL_STATE state, DAMAGE_LOCATION loc);
void func_005A3FF0(_zrdr *anim, SEAL_STATE state, DAMAGE_LOCATION loc);
}

#define READ_DAMAGE_ANIM(f, rifles, pistols)                                                        \
    void f(_zrdr *anim, SEAL_STATE state, DAMAGE_LOCATION loc)                                      \
    {                                                                                               \
        char *name = zrdr_findstring(anim, "NAME");                                                 \
        char *pistol = zrdr_findstring(anim, "PISTOL_NAME");                                        \
                                                                                                    \
        AnimSet::rifles[loc][state].push_back(FindAnimType(name));                                  \
        AnimSet::pistols[loc][state].push_back(FindAnimType(pistol));                               \
        int last = AnimSet::rifles[loc][state].size() - 1;                                          \
                                                                                                    \
        AnimSet::list[0]->m_animsbyfp->SetPistolAnim(AnimSet::rifles[loc][state][last],              \
                                                     AnimSet::pistols[loc][state][last]);            \
    }

/* room for eight anims in each table's every location and state (OVL2 0x5A46C0, which the set
 * loader 0x5E30F0 calls; SOCOM 1's, between the readers and ReadDeathAndFlinchAnims as here) */
void AnimSet::InitDeathAndFlinchAnimations()
{
    for (int i = 0; i < 6; i++)
        for (int j = 0; j < 3; j++) {
            deathanim[i][j].reserve(8);
            pistoldeathanim[i][j].reserve(8);
            teammatedeathanim[i][j].reserve(8);
            teammatepistoldeathanim[i][j].reserve(8);
            damageanim[i][j].reserve(8);
            pistoldamageanim[i][j].reserve(8);
        }
}

/* a location's teammate deaths: each list's animations by stance (OVL2 0x5A44F0; SOCOM 1's, the
 * reading of each one moved into a helper). The three readers differ only in their helper and in
 * where each stance's counter is declared: MW gives the later-declared of a list, its count and
 * its counter the lower saved register, and the target's registers rotate from stance to stance
 * (the flinches' prone loop declares its counter in the for, the deaths' before the count) */
void AnimSet::ReadTeammateDeathAnimations(_zrdr *rdr, const char *name, DAMAGE_LOCATION loc)
{
    _zrdr *block;

    if ((block = zrdr_findtag(rdr, name)) == 0)
        return;
    int i;
    int n;
    _zrdr *stand = zrdr_findtag(block, "STAND");

    n = stand->Count();
    for (i = 0; i < n; i++)
        func_005A3FF0(ItemAt(stand, i), stateStand, loc);
    _zrdr *crouch = zrdr_findtag(block, "CROUCH");
    int j;
    int nc = crouch->Count();

    for (j = 0; j < nc; j++)
        func_005A3FF0(ItemAt(crouch, j), stateCrouch, loc);
    _zrdr *prone = zrdr_findtag(block, "PRONE");
    int k;
    int np = prone->Count();

    for (k = 0; k < np; k++)
        func_005A3FF0(ItemAt(prone, k), stateProne, loc);
}

/* a location's deaths (OVL2 0x5A4320; SOCOM 1's, as the teammates') */
void AnimSet::ReadDeathAnimations(_zrdr *rdr, const char *name, DAMAGE_LOCATION loc)
{
    _zrdr *block;

    if ((block = zrdr_findtag(rdr, name)) == 0)
        return;
    int i;
    int n;
    _zrdr *stand = zrdr_findtag(block, "STAND");

    n = stand->Count();
    for (i = 0; i < n; i++)
        func_005A3E90(ItemAt(stand, i), stateStand, loc);
    _zrdr *crouch = zrdr_findtag(block, "CROUCH");
    int j;
    int nc = crouch->Count();

    for (j = 0; j < nc; j++)
        func_005A3E90(ItemAt(crouch, j), stateCrouch, loc);
    _zrdr *prone = zrdr_findtag(block, "PRONE");
    int k;
    int np = prone->Count();

    for (k = 0; k < np; k++)
        func_005A3E90(ItemAt(prone, k), stateProne, loc);
}

/* a location's flinches (OVL2 0x5A4150; SOCOM 1's, as the teammates') */
void AnimSet::ReadFlinchAnimations(_zrdr *rdr, const char *name, DAMAGE_LOCATION loc)
{
    _zrdr *block;

    if ((block = zrdr_findtag(rdr, name)) == 0)
        return;
    int i;
    int n;
    _zrdr *stand = zrdr_findtag(block, "STAND");

    n = stand->Count();
    for (i = 0; i < n; i++)
        func_005A3D30(ItemAt(stand, i), stateStand, loc);
    _zrdr *crouch = zrdr_findtag(block, "CROUCH");
    int j;
    int nc = crouch->Count();

    for (j = 0; j < nc; j++)
        func_005A3D30(ItemAt(crouch, j), stateCrouch, loc);
    _zrdr *prone = zrdr_findtag(block, "PRONE");
    int np = prone->Count();

    for (int k = 0; k < np; k++)
        func_005A3D30(ItemAt(prone, k), stateProne, loc);
}

/* (short enough to auto-inline into the readers, which call them: pinned) */
#pragma auto_inline off
extern "C" {
READ_DAMAGE_ANIM(func_005A3FF0, teammatedeathanim, teammatepistoldeathanim)
READ_DAMAGE_ANIM(func_005A3E90, deathanim, pistoldeathanim)
READ_DAMAGE_ANIM(func_005A3D30, damageanim, pistoldamageanim)
}
#pragma auto_inline reset

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
/* (0x588BC0 pushes an anim on the body -- an anim type, the state, a flag and a kind; 0x58A920 sets
 * bits of a blend's flags; 0x588AB0 tells the network of the push; ClearToBase, 0x58A4D0, is
 * seal_body.cpp's) */
void *func_00588BC0(CZSealBody *seal, AnimType type, int state, int flag, int kind);
unsigned short func_0058A920(unsigned short flags, unsigned short bits, bool on);
void func_00588AB0(CZSealBody *seal, int time, unsigned int anim);
void *func_00555170(void *stack);
}

extern "C" {
/* the SEAL off its ladder, and its hold on a ladder let go (OVL2 0x59D750, 0x5B14E0: this unit's,
 * defined below -- the deaths inline them) */
void func_0059D750(CZSealBody *seal);
void func_005B14E0(CZSealBody::CSealClimb *climb, CZSealBody *seal);
/* the death's end, once its anim has run (OVL2 0x5A0040, below), the overlay's anims let
 * go (OVL2 0x5781C0, the unclaimed stretch before seal_body.cpp) and 0x5A5DA0 (not written yet) */
void func_005A0040(CZSealBody *seal);
void func_005781C0(CZSealBody *seal);
void func_005A5DA0(CZSealBody *seal);
/* the anim at the end of the death (AnimTypes' static short at OVL1 0x3DEED8) and the network's
 * push of anim 34 (OVL2 0x588B40, seal_body.cpp's) */
extern short data_003DEED8;
void func_00588B40(CZSealBody *seal, int time);
}

/* a death's callback, at the anim's end: the death finished (OVL2 0x5A2680, new in SOCOM II: the
 * two deaths below hang it on their blends; the name ours) */
extern "C" void func_005A2680(void *body, void *data)
{
    func_005A0040((CZSealBody *)body);
}

/* a dying SEAL's anim: the blend's representative anim's length, the callback hung on a short one
 * (the names ours) */
static inline void HangDeathCallback(CZBodyAnimBlend *b)
{
    float t = b->Representative()->m_anim->m_34;

    if (t < 1.0f) {
        b->m_callback.m_func = func_005A2680;
        b->m_callback.m_tfrac = t;
        b->m_callback.m_userdata = 0;
    }
}

extern "C" {
/* (OVL2 0x5A2390, 0x5A2490, 0x5A21B0, below; 0x539E90 and 0x545EE0, this unit's declarations further on) */
CZBodyAnimBlend *func_005A2390(CZSealBody *seal, AnimType type);
CZBodyAnimBlend *func_005A2490(CZSealBody *seal, AnimType type);
CZBodyAnimBlend *func_005A21B0(CZSealBody *seal);
CPnt3D *func_00539E90(CPnt3D *to, const CPnt3D *from);
bool func_00588030(CZSealBody *seal);
AnimType func_005A0950(CZSealBody *seal, DAMAGE_LOCATION loc, SEAL_STATE state);
/* the sets of the two zombies' anims a hit does not show (OVL1 bss) */
extern class AnimSet *data_0044D400;
extern class AnimSet *data_0044D410;
extern short data_003DEE08;
extern short data_003DEE20;
extern short data_003DEE48;
extern short data_003DEE40;
extern short data_003DEE18;
extern short data_003DEE30;
extern short data_003DEE38;
extern short data_003DEDB0;
extern short data_003DEDB8;
extern short data_003DEF70;
extern short data_003DEDF0;
extern short data_003DEDF8;
extern short data_003DEE00;
}

/* a blend's homing reset through CPnt3D's out-of-line assignment (an inline, the name ours: CHoming::Reset's
 * order) */
static inline void ResetHoming(CHoming *h)
{
    h->m_blend = 0;
    func_00539E90(&h->m_refpt_m, &CPnt3D::zero);
    func_00539E90(&h->m_norm_w, &CPnt3D::zero);
    func_00539E90(&h->m_int_w, &CPnt3D::zero);
    h->m_1c = 0;
    h->m_2c = 0;
    func_00539E90(&h->m_refvec_m, &CPnt3D::zero);
    func_00539E90(&h->m_target_refpt_m, &CPnt3D::zero);
}

/* the anim a hit shows (SOCOM 1's WeaponHitAnim; OVL2 0x5A2690, grown): none named and dead: a death anim; a
 * carried body's homing let go; in the fall's anims only the state, a corpse's twitch while it can; a new death
 * by its anim; a living flinch unless a hold, a reload or its anim sets forbid */
void CZSealBody::WeaponHitAnim(AnimType anim, SEAL_STATE state)
{
    if ((short)anim == -1) {
        if ((unsigned char)state == 8 || !m_isAlive) {
            bool b = false;

            anim = FindRandomDeathAnim((DAMAGE_LOCATION)3, &b);
            state = (SEAL_STATE)8;
            if ((short)anim == -1)
                return;
        } else
            return;
    }
    if (m_carry) {
        if (TopBlend()->m_homing.m_blend) {
            ResetHoming(&TopBlend()->m_homing);
            if (m_carry) {
                m_carry->m_carry = 0;
                m_carry = 0;
            }
        } else if (m_carry->TopBlend()->m_homing.m_blend) {
            ResetHoming(&m_carry->TopBlend()->m_homing);
            if (m_carry) {
                m_carry->m_carry = 0;
                m_carry = 0;
            }
        }
    }
    CZBodyAnimBlend *hit;
    CZBodyAnimBlend *top = (CZBodyAnimBlend *)func_00555170(&m_blends);
    unsigned char topstate = top->m_state;

    hit = 0;

    if (func_00588030(this)) {
        if ((unsigned char)state == 8 || !m_isAlive)
            top->m_state = 8;
        return;
    }
    unsigned char alive = m_isAlive;

    if (!alive && m_state == 8)
        return;
    if (!alive && (unsigned char)state != 8) {
        bool b = false;

        state = (SEAL_STATE)8;
        anim = FindRandomDeathAnim((DAMAGE_LOCATION)3, &b);
    }
    if (m_state == 9) {
        if ((unsigned char)state != 8 || topstate == 8)
            return;
        if (m_carry)
            DropBody();
    } else if (IsCurrentBlend((AnimType)data_003DEE08) || IsCurrentBlend((AnimType)data_003DEE20)) {
        if ((unsigned char)state != 8)
            return;
        if (m_carry)
            DropBody();
    } else if (IsCurrentBlend((AnimType)data_003DEE48)) {
        if ((unsigned char)state != 8)
            return;
        CZSealBody *carry = m_carry;

        if (carry) {
            carry->DetachBody();
            carry->PopBlend();
        }
    } else if (IsCurrentBlend((AnimType)data_003DEE40)) {
        if ((unsigned char)state != 8)
            return;
        CZSealBody *carry = m_carry;

        if (carry) {
            carry->DropBody();
            carry->ClearToBase(false);
        }
        return;
    } else if (m_state == 10 || IsCurrentBlend((AnimType)data_003DEE18) || IsCurrentBlend((AnimType)data_003DEE30) ||
               IsCurrentBlend((AnimType)data_003DEE38))
        return;
    if (topstate == 8) {
        if (data_0045A0C1)
            return;
        if (IsCurrentBlend((AnimType)data_003DEDB0) || IsCurrentBlend((AnimType)data_003DEDB8))
            return;
        float t = top->m_tfrac;

        if (!(t <= 0.35f) || t < 0.05f || !(m_14d0 < 3))
            return;
        if (GetRootPart()->m_pos.y < 5.0f)
            anim = func_005A0950(this, (DAMAGE_LOCATION)3, (SEAL_STATE)2);
        if ((short)anim == -1) {
            bool b = false;

            anim = FindRandomDeathAnim((DAMAGE_LOCATION)3, &b);
        }
        func_005A2490(this, anim);
        m_14d0++;
        return;
    }
    if ((unsigned char)state == 8) {
        if (m_state == 5)
            func_005A21B0(this);
        else {
            if (GetRootPart()->m_pos.y < 5.0f)
                anim = func_005A0950(this, (DAMAGE_LOCATION)3, (SEAL_STATE)2);
            if (IsCurrentBlend((AnimType)data_003DEF70)) {
                if (top->m_callback.m_func)
                    top->m_callback.m_func(this, top->m_callback.m_userdata);
                top->m_callback.Clear();
            }
            if ((short)anim == -1) {
                bool b = false;

                anim = FindRandomDeathAnim((DAMAGE_LOCATION)3, &b);
            }
            func_005A2490(this, anim);
        }
        m_14d0++;
        return;
    }
    if (IsChangingWeapons(true))
        return;
    if (IsCurrentBlend((AnimType)data_003DEDF0) || IsCurrentBlend((AnimType)data_003DEDF8) || IsCurrentBlend((AnimType)data_003DEE00))
        return;
    if (m_state == 5 || m_state == 9 || m_animset == data_0044D400 || m_animset == data_0044D410)
        return;
    if (func_005551A0(top->m_flags, 64)) {
        if (IsCurrentBlend((AnimType)1) || IsCurrentBlend((AnimType)18) || IsCurrentBlend((AnimType)26))
            hit = func_005A2390(this, anim);
    } else
        hit = func_005A2390(this, anim);
    if (hit)
        hit->m_flags = func_0058A920(hit->m_flags, 2, true);
}

/* a death by an anim type: the overlay's anims and 0x5A5DA0 first, the SEAL off its ladder and its
 * hold let go, unstunned, the blends cleared to the base and the anim pushed dead (state 8, kind 13)
 * with the death's callback, the push told to the network as event 14 (OVL2 0x5A2490, new in SOCOM
 * II; the name unknown). 0x5B14E0 inlined, its ladder lookup's load order the hold's unsigned index
 * (round 34, below). */
#pragma auto_inline off
extern "C" CZBodyAnimBlend *func_005A2490(CZSealBody *seal, AnimType type)
{
    func_005781C0(seal);
    func_005A5DA0(seal);
    func_0059D750(seal);
    func_005B14E0(&seal->m_climb, seal);
    seal->SetAmStunned(false);
    seal->ClearToBase(false);
    CZBodyAnimBlend *b = (CZBodyAnimBlend *)func_00588BC0(seal, type, stateDead, 0, 13);

    HangDeathCallback(b);
    func_00588AB0(seal, 14, type);
    return b;
}
#pragma auto_inline reset

/* an anim pushed on a living SEAL, kind 13 with the blend's bits 2 and 1 set -- refused when its top
 * blend or itself is dead, or while it holds the entity at 0xEB8 -- and the push told to the network
 * as event 15 (OVL2 0x5A2390, new in SOCOM II: 0x5880E0 calls it; the name unknown) */
extern "C" CZBodyAnimBlend *func_005A2390(CZSealBody *seal, AnimType type)
{
    if (!seal->m_isAlive || ((CZBodyAnimBlend *)func_00555170((char *)seal + 0x1c0))->m_state == stateDead ||
        seal->m_state == stateDead)
        return 0;
    if (seal->m_eb8)
        return 0;
    seal->ClearToBase(false);
    CZBodyAnimBlend *b = (CZBodyAnimBlend *)func_00588BC0(seal, type, seal->m_state, 0, 13);

    b->m_flags &= ~4;
    b->m_flags |= 4;
    b->m_flags = func_0058A920(b->m_flags, 2, true);
    func_00588AB0(seal, 15, type);
    return b;
}

/* the death that ends a fall: the SEAL off its ladder, its hold dropped, unstunned, the blends
 * cleared to the base and AnimTypes' 0x3DEED8 pushed dead with the death's callback; bit 9 set and
 * 10 cleared, the kept velocity zeroed, the altitude where it stands, the push of anim 34 told to the
 * network (OVL2 0x5A21B0, new in SOCOM II; the name unknown) */
#pragma auto_inline off
extern "C" CZBodyAnimBlend *func_005A21B0(CZSealBody *seal)
{
    func_0059D750(seal);
    seal->m_climb.m_state = 0;
    seal->m_climb.m_active = 0;
    seal->SetAmStunned(false);
    seal->ClearToBase(false);
    CZBodyAnimBlend *b = (CZBodyAnimBlend *)func_00588BC0(seal, (AnimType)data_003DEED8, stateDead, 0, 13);

    HangDeathCallback(b);
    seal->SetBit9(true);
    seal->SetBit10(false);
    seal->m_1350 = CPnt3D::zero;
    seal->m_alt = seal->m_posW.y;
    func_00588B40(seal, 21);
    return b;
}
#pragma auto_inline reset

/* an entity that is a SEAL (kind 2), or none (the inline cast HandleWeaponEvent's walk and ReportWeaponHitDamage show) */
static inline CZSealBody *AsSeal(CEntity *e)
{
    return e && e->m_type == (CEntity::TYPE)2 ? (CZSealBody *)e : 0;
}

/* the hit sent to the network (OVL1 0x2BBDA0, fts_remote.cpp's) */
void SendHitMessage(int shooter, int victim, int part, float damage, float f, bool flag, unsigned char c);

/* online, a hit on the SEAL reported: sent on when a SEAL shot, the hit counted for both when it
 * counts, and a first head shot -- not a grenade's (items 121 and 171) -- credited to the shooter
 * (OVL2 0x5A1FF0; SOCOM 1's ReportWeaponHitDamage, after RemoteWeaponDamageVictim in both builds --
 * SOCOM II passes the projectile first and spares grenades the head shot) */
void CZSealBody::ReportWeaponHitDamage(CZProjectile *proj, int shooter_id, int victim_id, DAMAGE_LOCATION where,
                                       float damage, float pierce, bool hit, unsigned char weapon)
{
    if (theNetwork.m_9) {
        CEntity *e = CEntity::m_list.GetEntityById(shooter_id);
        CZSealBody *shooter = 0;

        if (e)
            shooter = AsSeal(e);
        if (shooter)
            SendHitMessage(shooter_id, victim_id, where, damage, pierce, hit, weapon);
        if (hit) {
            if (shooter)
                shooter->m_stats.Hit(*shooter, *this);
            m_stats.WasHit(*this);
        }
        bool grenade = false;

        if (proj->m_weapon) {
            switch (proj->m_weapon->GetIDType()) {
            case 171:
            case 121:
                grenade = true;
            }
        }
        if (where == 0 && m_stats.m_count[1] <= 0 && !grenade) {
            if (shooter)
                shooter->m_stats.HeadShot(*shooter);
            m_stats.m_count[1]++;
        }
    }
}

extern "C" void func_005B6490(zdb::CNode *node, CBBox *box);

/* the bounds of a node tree as it is drawn, its own matrix the start: the unit matrix pushed on the
 * matrix stack, the tree's box taken, popped again (OVL2 0x5B67F0, new in SOCOM II; the name unknown) */
extern "C" CBBox *func_005B67F0(zdb::CNode *node, CBBox *box)
{
    _stack.Push(&CMatrix::identity);
    func_005B6490(node, box);
    _stack.Pop();
    return box;
}

/* a box's eight corners, the minimum second and the maximum last (an inline past inline_max_size:
 * expanded one level, its CPnt3D constructors and assignments called out of line -- OVL2's copies
 * 0x5414F0 / 0x539E90; the box a reference to the caller's copy, declared after the corners: by value
 * or taken straight from the node the frame or the size differ; the name ours) */
static inline void BoxCorners(const CBBox &b, CPnt3D *c)
{
    c[0] = CPnt3D(b.m_min.x, b.m_min.y, b.m_max.z);
    c[1] = b.m_min;
    c[2] = CPnt3D(b.m_max.x, b.m_min.y, b.m_min.z);
    c[3] = CPnt3D(b.m_max.x, b.m_min.y, b.m_max.z);
    c[4] = CPnt3D(b.m_min.x, b.m_max.y, b.m_max.z);
    c[5] = CPnt3D(b.m_min.x, b.m_max.y, b.m_min.z);
    c[6] = CPnt3D(b.m_max.x, b.m_max.y, b.m_min.z);
    c[7] = b.m_max;
}

/* an active node with visuals grown into a box: its matrix multiplied onto the stack (pushed), each
 * active child with visuals taken in turn, then -- a node holding visuals -- its own box's eight
 * corners through the stack's top and into the box, the stack popped (OVL2 0x5B6490, new in SOCOM II;
 * 0x5B67F0 and itself call it; the name unknown). `if (!a || !b) return;` gives the target's bnez into
 * the body and the `b` to the exit; the children through a reference local keep the vector's address
 * saved for the loop's end; each corner through a reference (or a pointer, or an inline min/max
 * helper) loads the box's coordinate first, `c[i].x <= box->m_min.x` straight loads the corner's */
extern "C" void func_005B6490(zdb::CNode *node, CBBox *box)
{
    if (!node->m_active || !node->m_hasVisuals)
        return;
    _stack.Multiply(&node->m_matrix, true);
    zdb::CNodeVector &kids = node->m_child;

    for (zdb::CNodeVector::iterator it = kids.begin(); it != kids.end(); ++it) {
        zdb::CNode *child = *it;

        if (child->m_hasVisuals && child->m_active)
            func_005B6490(child, box);
    }
    if (node->m_visual.size()) {
        CPnt3D c[8];
        CBBox b = node->m_bbox;

        BoxCorners(b, c);
        CStack::m_top->Transform(c, 8);
        for (int i = 0; i < 8; i++) {
            CPnt3D &p = c[i];

            box->m_min.x = p.x <= box->m_min.x ? p.x : box->m_min.x;
            box->m_min.y = p.y <= box->m_min.y ? p.y : box->m_min.y;
            box->m_min.z = p.z <= box->m_min.z ? p.z : box->m_min.z;
            box->m_max.x = p.x >= box->m_max.x ? p.x : box->m_max.x;
            box->m_max.y = p.y >= box->m_max.y ? p.y : box->m_max.y;
            box->m_max.z = p.z >= box->m_max.z ? p.z : box->m_max.z;
        }
    }
    _stack.Pop();
}

/* whether a hit is one a shot goes through: a material known, not under a polygon of camera type 1,
 * neither volumetric nor liquid, and passing under 0.98 of a shot or a pickup's */
static inline bool ShootThrough(zdb::IntersectStruct *is)
{
    CMaterialEx *mat = is ? MaterialEx(is->GetMaterial()) : 0;

    if (!mat || !is)
        return false;
    if (is->m_poly && (unsigned char)is->m_poly->m_flags.m_cameratype == 1)
        return false;
    if (mat->IsVolumetric() || mat->IsLiquid() || mat->m_penetration >= 0.98f && !mat->IsPickup())
        return false;
    return true;
}

/* the nearest hit a shot goes through on the way from one point to another, the SEAL's node switched
 * off for the probe -- or none (OVL2 0x5B6240, new in SOCOM II: 0x5AA6E0 calls it; the name unknown) */
extern "C" zdb::IntersectStruct *func_005B6240(CZSealBody *seal, const CPnt3D *from, const CPnt3D *to)
{
    /* (the node's dead first definition numbers it ahead of the world, which the target loads first
     * and keeps in s0) */
    zdb::CNode *node = 0;
    zdb::CWorld *world = zdb::CWorld::m_world;

    node = seal->GetNode();
    bool active = node->m_active;

    node->SetActive(false);
    zdb::DiIntersect *di = world->GetIntersects(from, to);

    node->SetActive(active);
    int n = di ? di->m_Cnt : 0;
    float best = 0.0f;
    zdb::IntersectStruct *hit = 0;

    for (int i = 0; i < n; i++) {
        zdb::IntersectStruct *is = &di->m_Intersects[i];

        if (ShootThrough(is)) {
            CPnt3D v;

            is->m_pos.Sub(from, &v);
            float d = v.x * v.x + v.y * v.y + v.z * v.z;

            if (!hit || d < best) {
                hit = is;
                best = d;
            }
        }
    }
    return hit;
}

extern "C" {
/* the SEALs' pool of ray intersections, its count and a third word (OVL1 bss; new in SOCOM II) */
extern zdb::DiIntersect **data_0044D588;
extern int data_0044D590;
extern int data_0044D598;
}

/* (the lowest a probe starts from, OVL2 bss 0x66AAB8: a variable) */
extern "C" float data_0066AAB8;

/* the body's bits as bools with a block each (inlines; the names ours) */
static inline bool HoldsNoProbe(const CZSealBody *seal)
{
    if (seal->m_1060_bit11)
        return true;
    return false;
}

static inline bool WantsProbe(const CZSealBody *seal)
{
    if (seal->m_bit8)
        return true;
    return false;
}

static inline bool DrivenByPlayer(const CZSealBody *seal)
{
    if (seal->m_control)
        return ((CSealCtrl *)seal->m_control)->IsPlayer();
    return false;
}

static inline void SetEntityBit8(CEntity *e, bool on)
{
    e->m_bit8 = on;
}

static inline void SetAltitudeCharacters(zdb::DiIntersect *di, bool on);

/* the ground probe queued for the frame, for a SEAL asked to or a player's: from the gunner's place
 * (held at the lowest start), lifted by the root's place -- only its height when the state is 3 -- a
 * pool entry made when all are taken, the region's mask and the characters' flag set (OVL2 0x5B0840, new
 * in SOCOM II: PrePostTick calls it, SOCOM 1's PreCheckDIDown inlined at its head; the name unknown) */
extern "C" void func_005B0840(CZSealBody *seal)
{
    seal->m_down_di = -999;
    if (HoldsNoProbe(seal))
        return;
    if (!WantsProbe(seal) && !DrivenByPlayer(seal))
        return;
    SetEntityBit8(seal, false);

    CPnt3D pos = seal->m_gunner_pos;

    if (data_0066AAB8 > seal->m_gunner_pos.y)
        pos.y = data_0066AAB8;

    zdb::CNode *root = seal->GetRootPart()->m_node;
    CPnt3D rootpos;
    CPnt3D d;

    rootpos.x = root->m_matrix.m_matrix[3][0];
    rootpos.y = root->m_matrix.m_matrix[3][1];
    rootpos.z = root->m_matrix.m_matrix[3][2];

    TransformVectors(&seal->m_node->m_matrix, &rootpos, &d, 1);
    if (seal->m_state == 3)
        pos.y += d.y;
    else
        pos.Add(&d, &pos);

    zdb::DiIntersect *di;

    if (data_0044D598 == data_0044D590) {
        di = new zdb::DiIntersect(32);
        if (!di)
            return;
        di->m_Type = 1;
        data_0044D588 = (zdb::DiIntersect **)_realloc(data_0044D588, (data_0044D590 + 1) * 4, 0, 0);
        if (!data_0044D588)
            return;
        data_0044D588[data_0044D590] = di;
        data_0044D590++;
    }
    seal->m_down_di = data_0044D598;
    di = data_0044D588[seal->m_down_di];
    data_0044D598++;
    di->m_Tail = pos;
    di->SetMask(seal->m_last_altitude.GetRegion());
    if (seal->m_altitude_characters)
        SetAltitudeCharacters(di, true);
    else
        SetAltitudeCharacters(di, false);
    di->m_Node = seal->GetNode();
}

extern "C" {
/* the alert a lost ground raises, when the switch is on (OVL1 data 0x3DF1C8, a byte: no row; OVL2
 * 0x5582C0, seal_ai.cpp's) and the landing's test (OVL2 0x59AD30, seal_create.cpp's) */
extern unsigned char data_003DF1C8;
extern float data_004365C0;
bool func_005582C0(CAiStamp *a, CSealCtrlAi *ai, const CPnt3D *pos, float amount);
bool func_0059AD30(CZSealBody *seal, const CPnt3D *pos);
}

/* the ground probe's answer once the batch has run: a hit taken as the altitude (a SEAL's only its point),
 * the height set on a landing, the material's height added; no hit, with the switch on, an alert and the
 * SEAL back on the last ground; else that ground kept where the SEAL stands, lowered by its material's
 * height (OVL2 0x5B0420, new in SOCOM II: PostTick calls it; the name unknown) */
extern "C" void func_005B0420(CZSealBody *seal)
{
    if (seal->m_1060_bit11 ? true : false)
        return;
    if (seal->m_down_di >= 0) {
        zdb::DiIntersect *di = data_0044D588[seal->m_down_di];
        zdb::IntersectStruct *hit = seal->SealProcessAltitude(di, &di->m_Tail, 1.0f);

        if (hit) {
            func_0059AD30(seal, &hit->m_pos);
            if (seal->m_landed) {
                CPnt3D p = seal->m_node->GetPosition();
                float &y = p.y;

                seal->SetLanded(false);
                y = hit->m_pos.y;
                y += MaterialEx(hit->GetMaterial())->m_foot_step_offset;
                seal->m_node->SetPosition(((const CPnt3D &)p).x, ((const CPnt3D &)p).y, ((const CPnt3D &)p).z);
                seal->m_alt = y;
                if (seal->m_state != 2)
                    seal->m_1100.m_node = 0;
            }
            if (CZSealBody::GetSeal(hit->m_node))
                seal->m_last_altitude.m_pos = hit->m_pos;
            else
                seal->m_last_altitude = *hit;
            seal->m_420 = data_004365C0;
            seal->UpdatePosW();
        } else if (data_003DF1C8) {
            CSealCtrlAi *ai = ((CSealCtrl *)seal->m_control)->GetSealCtrlAi();

            if (ai)
                func_005582C0(&ai->m_stamp, ai, (const CPnt3D *)seal->m_node->m_matrix.m_matrix[3], 0.0333f);
            seal->SetBit9(false);
            seal->SetBit10(true);
            seal->m_1340 = 0.0f;
            seal->m_node->SetPosition(seal->m_last_altitude.m_pos.x, seal->m_last_altitude.m_pos.y, seal->m_last_altitude.m_pos.z);
            seal->UpdatePosW();
            seal->m_1100.m_node = 0;
        } else {
            CPnt3D norm(seal->m_last_altitude.m_norm);
            float y = seal->m_last_altitude.m_pos.y;

            seal->m_last_altitude.m_pos = CPnt3D::zero;
            seal->m_last_altitude.m_norm = CPnt3D::zero;
            seal->m_last_altitude.m_poly = 0;
            seal->m_last_altitude.m_node = 0;
            seal->m_last_altitude.m_pos = *(const CPnt3D *)seal->m_node->m_matrix.m_matrix[3];
            seal->m_last_altitude.m_norm = norm;
            seal->m_last_altitude.m_pos.y = y;
            seal->m_node->SetPosition(seal->m_last_altitude.m_pos.x, seal->m_last_altitude.m_pos.y, seal->m_last_altitude.m_pos.z);

            CMaterialEx *mat = MaterialEx(seal->m_last_altitude.GetMaterial());

            if (mat) {
                CPnt3D d;

                d.x = d.y = d.z = mat->m_foot_step_offset;
                seal->m_last_altitude.m_pos.Sub(&d, &seal->m_last_altitude.m_pos);
            }
        }
        seal->m_down_di = -1;
    }
    seal->SetLanded(false);
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
/* (a pickup taken offline is CZSealBody::PickupItem, OVL2 0x59F0C0, one online
 * CZSealBody::CanPickupNetItem, OVL2 0x59E830) told to the network as another console's (OVL1
 * 0x2BA2C0, the net code's) */
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
            seal->CanPickupNetItem(1, pickup->GetPickupNetEntityID(), (unsigned short *)ids, id);
        else
            func_002BA2C0(1, pickup->GetPickupNetEntityID(), ids, id, owner);
    } else
        seal->PickupItem(pickup);
}

extern "C" {
/* the body-part ids of the rifle and the pistol (OVL1 data, read as variables) */
extern int data_003DF148;
extern int data_003DF150;
/* a third id, cleared as either weapon is taken (OVL1 data, read as a variable) */
extern int data_003DF160;
}

/* The weapons moved between the hand and where they are worn: the part's node taken from its parent's
 * node and given to the new parent's, the part made the body's rifle or pistol (none when worn), its
 * live bit cleared, and a worn weapon's gear part made (SOCOM 1's ParentPistolTo*Callback pair, anim
 * callbacks taking (data, arg) -- 0x588BC0 hands them to the anims by address -- keep the part's
 * member through a reference; the direct forms 0x5A7480 and 0x5A7690, which 0x5807D0 calls, reload it
 * through the SEAL). OVL2 0x5A73C0-0x5A77DF, the rifle's first. */

/* a callback's data as the SEAL (an inline: cast straight in the body, the SEAL is numbered after the
 * part's reference and takes the other saved register) */
static inline CZSealBody *SealOfData(void *data)
{
    return (CZSealBody *)data;
}

/* the rifle slung on the back, as gear (OVL2 0x5A73C0, new in SOCOM II: a callback; the name unknown) */
extern "C" void func_005A73C0(void *data, void *arg)
{
    CZSealBody *seal = SealOfData(data);
    CZBodyPart *&rifle = seal->m_rifle;

    if (rifle->m_node) {
        rifle->m_parent->m_node->DeleteChild(rifle->m_node);
        rifle->m_parent = seal->m_spinelo;
        rifle->m_parent->m_node->AddChild(rifle->m_node);
        seal->SetBodyPart(data_003DF148, 0);
        rifle->SetActive(false);
    }
    seal->SetPartAsGear(seal->m_item_gear[1].m_part, seal->m_item_gear[1].m_gear);
}

/* the rifle taken into the right hand (OVL2 0x5A7480, new in SOCOM II: 0x5807D0 calls it) */
extern "C" void func_005A7480(CZSealBody *seal)
{
    if (seal->m_rifle->m_node) {
        seal->m_rifle->m_parent->m_node->DeleteChild(seal->m_rifle->m_node);
        seal->m_rifle->m_parent = seal->m_rhand;
        seal->m_rifle->m_parent->m_node->AddChild(seal->m_rifle->m_node);
        seal->SetBodyPart(data_003DF148, seal->m_rifle);
        seal->m_rifle->SetActive(false);
    }
}

/* the rifle taken into the right hand, as a callback (OVL2 0x5A7520, new in SOCOM II) */
extern "C" void func_005A7520(void *data, void *arg)
{
    CZSealBody *seal = SealOfData(data);
    CZBodyPart *&rifle = seal->m_rifle;

    if (rifle->m_node) {
        rifle->m_parent->m_node->DeleteChild(rifle->m_node);
        rifle->m_parent = seal->m_rhand;
        rifle->m_parent->m_node->AddChild(rifle->m_node);
        seal->SetBodyPart(data_003DF148, rifle);
        rifle->SetActive(false);
    }
}

/* the pistol holstered on the right thigh, as gear (OVL2 0x5A75D0; SOCOM 1's) */
void ParentPistolToRThighAsGearCallback(void *data, void *arg)
{
    CZSealBody *seal = SealOfData(data);
    CZBodyPart *&pistol = seal->m_pistol;

    if (pistol->m_node) {
        pistol->m_parent->m_node->DeleteChild(pistol->m_node);
        pistol->m_parent = seal->m_rthigh;
        pistol->m_parent->m_node->AddChild(pistol->m_node);
        seal->SetBodyPart(data_003DF150, 0);
        pistol->SetActive(false);
    }
    seal->SetPartAsGear(seal->m_item_gear[2].m_part, seal->m_item_gear[2].m_gear);
}

/* the pistol taken into the right hand (OVL2 0x5A7690, new in SOCOM II: 0x5807D0 calls it) */
extern "C" void func_005A7690(CZSealBody *seal)
{
    if (seal->m_pistol->m_node) {
        seal->m_pistol->m_parent->m_node->DeleteChild(seal->m_pistol->m_node);
        seal->m_pistol->m_parent = seal->m_rhand;
        seal->m_pistol->m_parent->m_node->AddChild(seal->m_pistol->m_node);
        seal->SetBodyPart(data_003DF150, seal->m_pistol);
        seal->m_pistol->SetActive(false);
    }
}

/* the pistol taken into the right hand, as a callback (OVL2 0x5A7730; SOCOM 1's) */
void ParentPistolToRHandCallback(void *data, void *arg)
{
    CZSealBody *seal = SealOfData(data);
    CZBodyPart *&pistol = seal->m_pistol;

    if (pistol->m_node) {
        pistol->m_parent->m_node->DeleteChild(pistol->m_node);
        pistol->m_parent = seal->m_rhand;
        pistol->m_parent->m_node->AddChild(pistol->m_node);
        seal->SetBodyPart(data_003DF150, pistol);
        pistol->SetActive(false);
    }
}

/* the pistol taken: the kit's pistol chosen, the pistol into the right hand when asked, the rifle
 * slung on the back -- still the body's rifle --, the third part none, the pistol the item in hand
 * (OVL2 0x5A7260; SOCOM 1's ChangeToPistolMode, its kit call ReParentToPistol -- OVL2 0x5C6DA0 --
 * in the same place, after ChangeToRifleMode as here) */
#pragma auto_inline off
void CZSealBody::ChangeToPistolMode(bool hand)
{
    m_kit.ReParentToPistol();
    if (hand)
        func_005A7690(this);
    if (m_rifle->m_node) {
        m_rifle->m_parent->m_node->DeleteChild(m_rifle->m_node);
        m_rifle->m_parent = m_spinelo;
        m_rifle->m_parent->m_node->AddChild(m_rifle->m_node);
        SetBodyPart(data_003DF148, m_rifle);
        m_rifle->SetActive(false);
    }
    SetBodyPart(data_003DF160, 0);
    m_item = ITEM_PISTOL;
}
#pragma auto_inline reset

/* the rifle taken: the kit's rifle chosen, the rifle into the right hand, the pistol holstered as
 * gear when asked -- the callback's work written through the SEAL, which the target reloads (the
 * callback inlined keeps the member's address instead) --, the third part none, the rifle the item
 * in hand (OVL2 0x5A70F0; SOCOM 1's ChangeToRifleMode, its kit call ReParentToRifle -- OVL2
 * 0x5C6A40 -- in the same place) */
#pragma auto_inline off
void CZSealBody::ChangeToRifleMode(bool holster)
{
    m_kit.ReParentToRifle();
    func_005A7480(this);
    if (holster) {
        if (m_pistol->m_node) {
            m_pistol->m_parent->m_node->DeleteChild(m_pistol->m_node);
            m_pistol->m_parent = m_rthigh;
            m_pistol->m_parent->m_node->AddChild(m_pistol->m_node);
            SetBodyPart(data_003DF150, 0);
            m_pistol->SetActive(false);
        }
        SetPartAsGear(m_item_gear[2].m_part, m_item_gear[2].m_gear);
    }
    SetBodyPart(data_003DF160, 0);
    m_item = ITEM_RIFLE;
}
#pragma auto_inline reset

extern "C" {
/* the blend on top of the body's stack (seal.cpp's tail copy, OVL2 0x555170) */
void *func_00555170(void *stack);
/* the weapon changes' anims (OVL1 data, an anim type's id each: four blends, three overlays) */
extern short data_003DF0E0;
extern short data_003DF0E8;
extern short data_003DF108;
extern short data_003DF110;
extern short data_003DF0F0;
extern short data_003DF0F8;
}

extern "C" {
/* the anim set a throwable binds (OVL1 bss 0x44D408: seal_body.cpp's 0x580E80 tests the SEAL's set
 * against it) */
extern AnimSet *data_0044D408;
}

/* whether the rifle is the weapon in the right hand -- the pistol there says not, neither says it is
 * (an inline: the three arms' bool assignments, the names ours) */
static inline bool RifleInHand(CZSealBody *seal)
{
    if (seal->m_rifle->m_parent == seal->m_rhand)
        return true;
    if (seal->m_pistol->m_parent == seal->m_rhand)
        return false;
    return true;
}

/* whether a weapon change leaves the rifle: the item asked for says so, else the last item, else it
 * does (an inline returning one constant from several arms, held in the compare's register -- the
 * name ours) */
static inline unsigned char ToRifle(CZSealBody *seal, SEAL_ITEM_BYTE item)
{
    if (item == ITEM_RIFLE)
        return 1;
    if (item == ITEM_PISTOL)
        return 0;
    if (seal->m_lastitem == ITEM_RIFLE)
        return 1;
    if (seal->m_lastitem == ITEM_PISTOL)
        return 0;
    return 1;
}

/* a weapon put away for an item: the blend for putting away the pistol, or the rifle, pushed -- a
 * throwables' set its own two --, that weapon no longer the body's, the item in hand, the aim and
 * fire points looked up again (OVL2 0x5A6B70, new in SOCOM II: SOCOM 1's ChangeWeapon did it inline;
 * the name unknown) */
extern "C" bool func_005A6B70(CZSealBody *seal, SEAL_ITEM_BYTE item)
{
    if (ToRifle(seal, item)) {
        if (seal->m_animset != data_0044D408)
            func_00588BC0(seal, (AnimType)data_003DF0F0, 0, 0, 0);
        else
            func_00588BC0(seal, (AnimType)data_003DF0E0, 0, 1, 0);
        seal->SetBodyPart(data_003DF150, 0);
    } else {
        if (seal->m_animset != data_0044D408)
            func_00588BC0(seal, (AnimType)data_003DF0F8, 0, 0, 0);
        else
            func_00588BC0(seal, (AnimType)data_003DF0E8, 0, 1, 0);
        seal->SetBodyPart(data_003DF148, 0);
    }
    seal->m_item = item;
    seal->m_curr_aimpt = seal->GetFirepointNode("aimpoint");
    seal->m_curr_firept = seal->GetFirepointNode("firepoint");
    return true;
}

/* a throwable taken: the throwables' anim set bound (anim 18, crouched), the blend for putting away
 * the rifle or the pistol in hand pushed, the item kept as the last and none in hand (4), the
 * throwable's part into the right hand, the aim and fire points looked up again (OVL2 0x5A6D30, new
 * in SOCOM II: SOCOM 1's ChangeWeapon did it inline; the name unknown) */
extern "C" bool func_005A6D30(CZSealBody *seal, SEAL_ITEM_BYTE item)
{
    seal->BindAnimSet(data_0044D408, (AnimType)18, stateCrouch, false);
    if (RifleInHand(seal))
        func_00588BC0(seal, (AnimType)data_003DF0E0, 1, 0, 0);
    else
        func_00588BC0(seal, (AnimType)data_003DF0E8, 1, 0, 0);
    seal->m_lastitem = item;
    seal->m_item = ITEM_DONE;
    CZBodyPart *part = seal->m_item_gear[4].m_part;

    if (part && part->m_node) {
        part->m_parent->m_node->DeleteChild(part->m_node);
        part->m_parent = seal->m_rhand;
        part->m_parent->m_node->AddChild(part->m_node);
        part->SetActive(false);
    }
    seal->m_curr_aimpt = seal->GetFirepointNode("aimpoint");
    seal->m_curr_firept = seal->GetFirepointNode("firepoint");
    return true;
}

extern "C" {
/* (seal_body.cpp's OVL2 0x587F90, 0x587F50 and 0x587F10: tests of the SEAL's anim state -- the
 * first refuses the fire, the other two with the item 151 in hand) and whether a blend's fraction is
 * past a point, from the far end when reversed (OVL2 0x58A8C0, its draft), and a halfword's bits
 * tested (seal.cpp's tail copy, OVL2 0x5551A0) */
bool func_00587F90(CZSealBody *seal);
bool func_00587F50(CZSealBody *seal);
bool func_00587F10(CZSealBody *seal);
bool func_0058A8C0(bool reversed, float t, float at);
bool func_005551A0(unsigned short flags, unsigned short bits);
}

/* (the stab's, restraint's and CQB attack's tests, below) */
static inline bool IsStabbing(CZSealBody *seal);
static inline bool IsRestraining(CZSealBody *seal);
static inline bool IsCQBAttacking(CZSealBody *seal);

/* whether the SEAL may fire: standing, crouched, prone, peeking or in the air; not zooming; the top
 * blend not in state 8; not throwing (the start included), reloading, changing weapons (the kit
 * asked when told), stabbing, restraining or in a CQB attack; not carrying; 0x587F90's test failed;
 * no overlay playing, no body carried, not stunned; not holding item 151 in either of the two anim
 * states; and -- when timing is asked -- the top blend's last anim past its script's fire point (from
 * the far end for a reversed blend) (OVL2 0x5A7DE0; SOCOM 1's CanFire, after IsThrowing in both builds,
 * its two flags SOCOM II's: CanChangeItem asks it first, FireWhenReady once). The top through
 * TopBlend() recomputes this+0x1c0 at each fetch as the target does (written func_00555170((char *)this
 * + 0x1c0) MW folds it with IsThrowing's into a saved register: one more saved, 12 bytes long); the
 * reversed flag declared before the fraction gives the target's andi before Begin (after it, the
 * fraction is loaded ahead of the call and the andi sinks into Begin's delay slot) */
bool CZSealBody::CanFire(bool kit, bool timing)
{
    CZBodyAnimBlend *top = TopBlend();

    switch (m_state) {
    case stateStand:
    case stateCrouch:
    case stateProne:
    case statePeek:
    case stateInAir:
        break;
    default:
        return false;
    }
    if (m_iszooming)
        return false;
    if (top->m_state == 8)
        return false;
    if (IsThrowing(false))
        return false;
    if (IsReloading())
        return false;
    if (IsChangingWeapons(kit))
        return false;
    if (IsStabbing(this))
        return false;
    if (IsRestraining(this))
        return false;
    if (IsCQBAttacking(this))
        return false;
    if (m_state == stateCarrying)
        return false;
    if (func_00587F90(this))
        return false;
    if (m_overlay.m_anim)
        return false;
    if (m_carry)
        return false;
    if (m_am_stunned)
        return false;
    if ((func_00587F50(this) || func_00587F10(this)) && m_kit.GetCurItem()->GetIDType() == 151)
        return false;
    if (!timing)
        return true;
    bool r = false;
    bool reversed = func_005551A0(top->m_flags, 0x20);
    float t = top->m_tfrac;

    for (ACTIVE_ANIM *a = top->m_anims.Begin(); a; a = a->m_overlay)
        r = func_0058A8C0(reversed, t, a->m_anim->m_2c);
    return r;
}

/* whether the top blend is a throw's, the start's two too unless told not (OVL2 0x5A7D10; SOCOM 1's
 * IsThrowing, before CanFire in both builds, the same tests in the same order -- CrouchThrowGrenade
 * tested twice in both -- which names the two pistol tosses: the SEALs, round 44) */
bool CZSealBody::IsThrowing(bool started)
{
    short type = ((CZBodyAnimBlend *)func_00555170((char *)this + 0x1c0))->m_animType;

    if (type == AnimTypes::ThrowGrenade || type == AnimTypes::CrouchThrowGrenade ||
        type == AnimTypes::ProneThrowGrenade || type == AnimTypes::TossGrenade ||
        type == AnimTypes::PistolTossGrenade || type == AnimTypes::ProneTossGrenade ||
        type == AnimTypes::PistolProneTossGrenade || type == AnimTypes::CrouchThrowGrenade ||
        (!started && (type == AnimTypes::TossGrenadePeekRight || type == AnimTypes::TossGrenadePeekLeft)))
        return true;
    return false;
}

extern "C" {
/* the reloads' anims SOCOM II adds (OVL1 data, anim types: the pistols' and the kind-81 weapon's,
 * past SOCOM 1's AnimTypes -- SOCOM 1's eight reloads and RifleM203Reload are zseal.h's AnimTypes
 * members) */
extern short data_003DF098;
extern short data_003DF0A0;
extern short data_003DF0A8;
extern short data_003DF0B8;
extern short data_003DF0C0;
extern short data_003DF0C8;
/* whether an anim type is anywhere on the blend stack (seal_body.cpp's OVL2 0x58A470) */
bool func_0058A470(CZSealBody *seal, AnimType type);
}

extern "C" {
/* the reloads SOCOM II adds (OVL1 data, anim types beside data_003DF098..0x3DF0C8) and the throwables'
 * set's reload (0x3DF0D8, its timed call the kit's ShowProjectile: OVL2 0x5A78C0) */
extern short data_003DF0B0;
extern short data_003DF0D0;
extern short data_003DF0D8;
void func_005A78C0(void *body, void *data);
/* (OVL2 0x5B4340: the state test of a kind, below) */
bool func_005B4340(CZSealBody *seal, unsigned char kind);
}

/* a reload's blend pushed: the throwables' set its own; else by the stance the blends rest in -- a
 * launcher's, an item of kind 81 when asked, item 84 or a pistol-kind (101) item but 104-106, the rifle,
 * else the pistol, each moving or still -- in the air only a launcher's; the stack cleared to its base
 * first when still in the state it rests in, the blend's rate set to the weapon's reload time when
 * still (OVL2 0x5A82E0; SOCOM 1's Reload, after CanFire in both builds) */
bool CZSealBody::Reload(bool b)
{
    CZFTSWeapon *item = m_kit.GetCurItem();
    EQUIP_ITEM kind = EQUIP_NONE;
    EQUIP_ITEM id = EQUIP_NONE;

    if (item) {
        kind = item->GetIDType();
        id = item->m_ID;
    }
    TopBlend();
    bool rifle = m_item == ITEM_RIFLE;

    if (!func_005B4340(this, 3))
        return false;
    bool moving = m_velM.x * m_velM.x + m_velM.y * m_velM.y + m_velM.z * m_velM.z > 400.0f;
    bool launcher = true;

    /* the item in hand in an M203's or a grenade launcher's fire mode (the kit's CurWeaponIsM203Mode
     * 0x5BDA00 and CurWeaponIsGrenadeLauncherMode 0x5BD6D0, tail calls on its selected item) */
    if (!m_kit.CurWeaponIsM203Mode() && !m_kit.CurWeaponIsGrenadeLauncherMode())
        launcher = false;
    bool pistolkind = false;

    if (kind == 101 && (unsigned int)(id - 104) > 2)
        pistolkind = true;
    SEAL_STATE state = (SEAL_STATE)m_state;
    SEAL_STATE base = (SEAL_STATE)PeekBaseState();
    short anim;

    if (m_animset == data_0044D408)
        anim = data_003DF0D8;
    else {
        switch (base) {
        case stateStand:
            if (launcher) {
                if (moving)
                    anim = AnimTypes::MovingRifleReload;
                else
                    anim = AnimTypes::RifleM203Reload;
            } else if (b && kind == 81) {
                if (moving)
                    anim = data_003DF0D0;
                else
                    anim = data_003DF0B8;
            } else if (id == 84 || pistolkind) {
                if (moving)
                    anim = data_003DF0B0;
                else
                    anim = data_003DF098;
            } else if (rifle) {
                if (moving)
                    anim = AnimTypes::MovingRifleReload;
                else
                    anim = AnimTypes::RifleReload;
            } else {
                if (moving)
                    anim = AnimTypes::MovingPistolReload;
                else
                    anim = AnimTypes::PistolReload;
            }
            break;
        case stateCrouch:
            if (launcher) {
                if (moving)
                    anim = AnimTypes::MovingRifleReload;
                else
                    anim = AnimTypes::RifleM203Reload;
            } else if (b && kind == 81) {
                if (moving)
                    anim = data_003DF0D0;
                else
                    anim = data_003DF0C0;
            } else if (id == 84 || pistolkind) {
                if (moving)
                    anim = data_003DF0B0;
                else
                    anim = data_003DF0A0;
            } else if (rifle) {
                if (moving)
                    anim = AnimTypes::MovingRifleReload;
                else
                    anim = AnimTypes::RifleCrouchReload;
            } else {
                if (moving)
                    anim = AnimTypes::MovingPistolReload;
                else
                    anim = AnimTypes::PistolCrouchReload;
            }
            break;
        case stateProne:
            if (launcher)
                anim = AnimTypes::RifleM203Reload;
            else if (b && kind == 81)
                anim = data_003DF0C8;
            else if (id == 84 || pistolkind)
                anim = data_003DF0A8;
            else if (rifle)
                anim = AnimTypes::RifleProneReload;
            else
                anim = AnimTypes::PistolProneReload;
            break;
        case stateInAir:
            if (launcher)
                anim = AnimTypes::MovingRifleReload;
            else
                return true;
            break;
        default:
            return true;
        }
    }
    if (anim != -1) {
        if (!moving && state == base)
            ClearToBase(false);
        CZBodyAnimBlend *blend = (CZBodyAnimBlend *)func_00588BC0(this, (AnimType)anim, state, 0, 0);

        if (blend && anim == data_003DF0D8) {
            blend->m_callback.m_func = func_005A78C0;
            blend->m_callback.m_tfrac = 0.5f;
            blend->m_callback.m_userdata = 0;
        }
        if (!moving) {
            ACTIVE_ANIM *rep = blend->Representative();
            float t = item->m_reloadtime;

            if (!(t <= 0.0f))
                rep->m_script_wt = rep->m_anim->m_10 / t;
        }
    }
    return true;
}

/* whether a reload is under way: the kit's fire delay, a reload's anim on top -- or anywhere on the
 * blend stack -- or a moving reload's overlay playing (OVL2 0x5A7AB0; SOCOM 1's IsReloading, between
 * IsChangingWeapons and IsThrowing in both builds: two Top calls, the first unread; SOCOM II asks the
 * kit and the whole stack too, and six reloads more) */
bool CZSealBody::IsReloading()
{
    if (m_kit.IsFireDelayed())
        return true;
    TopBlend();
    short type = TopBlend()->m_animType;

    if (type == AnimTypes::RifleReload || type == AnimTypes::PistolReload || type == AnimTypes::RifleCrouchReload || type == AnimTypes::PistolCrouchReload ||
        type == AnimTypes::RifleProneReload || type == AnimTypes::PistolProneReload || type == AnimTypes::RifleM203Reload || type == data_003DF0B8 ||
        type == data_003DF0C0 || type == data_003DF0C8 || type == data_003DF098 || type == data_003DF0A0 ||
        type == data_003DF0A8 || func_0058A470(this, (AnimType)AnimTypes::RifleReload) ||
        func_0058A470(this, (AnimType)AnimTypes::PistolReload) || func_0058A470(this, (AnimType)AnimTypes::RifleCrouchReload) ||
        func_0058A470(this, (AnimType)AnimTypes::PistolCrouchReload) || func_0058A470(this, (AnimType)AnimTypes::RifleProneReload) ||
        func_0058A470(this, (AnimType)AnimTypes::PistolProneReload) || func_0058A470(this, (AnimType)AnimTypes::RifleM203Reload) ||
        func_0058A470(this, (AnimType)data_003DF0B8) || func_0058A470(this, (AnimType)data_003DF0C0) ||
        func_0058A470(this, (AnimType)data_003DF0C8) || func_0058A470(this, (AnimType)data_003DF098) ||
        func_0058A470(this, (AnimType)data_003DF0A0) || func_0058A470(this, (AnimType)data_003DF0A8))
        return true;
    ACTIVE_ANIM *a = m_overlay.m_anim;

    if (a && a->m_anim) {
        if (a->m_id == AnimTypes::MovingRifleReload || a->m_id == AnimTypes::MovingPistolReload)
            return true;
        return false;
    }
    return false;
}

/* whether a weapon change is under way: the kit's asked item when told to count it, a change's blend
 * on top, or its overlay running (OVL2 0x5A7980; SOCOM 1's IsChangingWeapons, first of the stretch
 * IsChangingWeapons, IsReloading, IsThrowing, CanFire in both builds -- SOCOM II's takes the flag and
 * the kit test, its top blend fetched and left unread) */
bool CZSealBody::IsChangingWeapons(bool kit)
{
    if (kit && m_kit.m_requested_item)
        return true;
    func_00555170((char *)this + 0x1c0);
    if (IsCurrentBlend((AnimType)53) || IsCurrentBlend((AnimType)54) || IsCurrentBlend((AnimType)55) ||
        IsCurrentBlend((AnimType)data_003DF0E0) || IsCurrentBlend((AnimType)data_003DF0E8) ||
        IsCurrentBlend((AnimType)data_003DF108) || IsCurrentBlend((AnimType)data_003DF110))
        return true;
    ACTIVE_ANIM *a = m_overlay.m_anim;

    if (a && a->m_anim && (a->m_id == AnimTypes::MovingRifle2Pistol || a->m_id == data_003DF0F0 || a->m_id == data_003DF0F8))
        return true;
    return false;
}

/* whether the first-person camera stays off: never without a blend; hanging, or (each when asked)
 * the top blend in state 33 or the anims 36 or 35 playing (OVL2 0x5A77E0, new in SOCOM II: OVL1's
 * GetFirstPersonCamPos asks it with 1, 1, 1; the names ours) */
extern "C" bool func_005A77E0(CZSealBody *seal, int state33, int anim36, int anim35)
{
    CZBodyAnimBlend *top = (CZBodyAnimBlend *)func_00555170((char *)seal + 0x1c0);

    /* (an if and two returns: the ternary, a bool local or an int return add the andi 0xff) */
    if (top)
        return seal->m_state == stateHang || (state33 && top->m_state == 33) ||
               (anim36 && seal->IsCurrentBlend((AnimType)36)) ||
               (anim35 && seal->IsCurrentBlend((AnimType)35));
    return false;
}

/* bit 0 of 0x21C set (a setter: a constant true is `daddiu 1`, a false `(0 & 1)`) */
static inline void SetBit21c0(CZSealBody *seal, bool on)
{
    seal->m_useCachedReticuleSeal = on;
}

/* the kit as the shot's check sees it: the reticule's blocked bit (3) and the aim's assist (16), the
 * point aimed at, two values zeroed, the blocked reticule's size and place on the screen (kit.h's
 * CZKit; zseal.h has only its first word) */
struct CKitShootView {
    unsigned int m_bits0 : 3;
    unsigned int m_retblocked : 1;   // bit 3
    unsigned int m_bits4 : 12;
    unsigned int m_autoaimed : 1;   // bit 16 (kit.h's m_bit16)
    unsigned int m_bits17 : 15;
    unsigned char _4[0xc - 0x4];
    CPnt3D m_aimpt;   // 0xc
    float m_18;   // 0x18
    float m_1c;   // 0x1c
    unsigned char _20[0x860 - 0x20];
    float m_retblockedsize;   // 0x860
    float m_retblockedX;   // 0x864
    float m_retblockedY;   // 0x868
};

static inline CKitShootView *KitView(CZSealBody *seal)
{
    return (CKitShootView *)&seal->m_kit;
}

extern unsigned char _AutoAim;

extern "C" {
CZSealBody *func_005AB1C0(CZSealBody *seal, CPnt3D *out, CPnt3D *from);
CPnt3D *func_005414F0(CPnt3D *p, float x, float y, float z);
void __ml__6CPnt3DCFf(CPnt3D *ret, const CPnt3D *p, float s);
void __pl__6CPnt3DCFRC6CPnt3D(CPnt3D *ret, const CPnt3D *p, const CPnt3D *b);
void __mi__6CPnt3DCFRC6CPnt3D(CPnt3D *ret, const CPnt3D *p, const CPnt3D *b);
signed char GetCurReticule__5CZKitFv(CZKit *kit);
}

namespace zdb {
void SetCameraCollideFlagRequired(bool on);
}

static inline CNodeAction *ActionOf(zdb::CNode *node);

/* a random value in [-1, 1], its square keeping the sign (seal_control.cpp's) */
static inline float ShootSignedSquareRand()
{
    float r = 9.313226e-10f * (float)(rand() - 0x3fffffff);

    if (r < 0.0f)
        r = -(r * r);
    else
        r = r * r;
    return r;
}

/* the aim thrown off by a target's accuracy, scaled by its distance from the eye (seal_control.cpp's
 * PerturbAimVector about the line of sight; an inline, its locals after the caller's) */
static inline void PerturbAim(CPnt3D *aim, const CPnt3D *eye, const CAccuracy *acc)
{
    CPnt3D dir;
    CPnt3D up;
    CPnt3D side;
    CPnt3D vert;
    CPnt3D t1;
    CPnt3D t2;
    CPnt3D t4;
    CPnt3D t3;

    const CAccuracy *a = acc;

    __mi__6CPnt3DCFRC6CPnt3D(&dir, aim, eye);
    float dist = dir.Normalize();

    func_005414F0(&up, 0.0f, 1.0f, 0.0f);
    func_00516900(&side);
    func_00516900(&vert);
    dir.Cross(&up, &side, false);
    side.Cross(&dir, &vert, false);
    float r = ShootSignedSquareRand();

    __ml__6CPnt3DCFf(&t1, &side, dist * (a->m_factorX + a->m_factor * r));
    func_00539E90(&side, &t1);
    r = ShootSignedSquareRand();
    __ml__6CPnt3DCFf(&t2, &vert, dist * (a->m_factorY + a->m_factor * r));
    func_00539E90(&vert, &t2);
    __pl__6CPnt3DCFRC6CPnt3D(&t3, aim, &side);
    __pl__6CPnt3DCFRC6CPnt3D(&t4, &t3, &vert);
    func_00539E90(aim, &t4);
}

/* the world's hits along a ray, the SEAL's own node switched off and the camera's collision asked for
 * (an inline, the name ours) */
static inline zdb::DiIntersect *IntersectsPast(CZSealBody *seal, const CPnt3D *from, const CPnt3D *to)
{
    zdb::SetCameraCollideFlagRequired(true);
    zdb::CNode *node;
    zdb::CWorld *world = zdb::CWorld::m_world;

    node = seal->GetNode();
    bool active = node->m_active;

    node->SetActive(false);
    zdb::DiIntersect *di = world->GetIntersects(from, to);

    node->SetActive(active);
    zdb::SetCameraCollideFlagRequired(false);
    return di;
}

static inline void ClearHit(zdb::IntersectStruct *is)
{
    is->m_pos = CPnt3D::zero;
    is->m_norm = CPnt3D::zero;
    is->m_poly = 0;
    is->m_node = 0;
}

static inline bool Near(const CPnt3D &a, float x, float y, float z)
{
    if (!(fabsf(a.x - x) <= 0.008f))
        return false;
    if (!(fabsf(a.y - y) <= 0.008f))
        return false;
    if (!(fabsf(a.z - z) <= 0.008f))
        return false;
    return true;
}

/* the shot's check along the view: the nearest hit a shot goes through and the nearest hit whose node
 * takes an action, out to the weapon's range (at least 600, at most the camera's far plane); the aim's
 * assist onto a target's place thrown off by its accuracy; the reticule's hit and action's hit kept;
 * then, the weapon up, whether something lies in the way between the fire point and the aim (OVL2
 * 0x5AA6E0; SOCOM 1's CheckDIShoot, after GetReticuleDi in both builds) */
void CZSealBody::CheckDIShoot(CPnt3D *from, CMatrix *mat)
{
    if ((m_control ? ((CSealCtrl *)m_control)->IsAi() : false) || IsRemote())
        return;
    m_kit.GetCurItemFireClass();
    float range = 600.0f;
    CZFTSWeapon *w = m_kit.GetCurItem();
    float r = 0.0f;

    if (w)
        r = ((CZWeapon *)w)->GetMaxRange();

    if (r > 600.0f)
        range = r;
    float far = zdb::CWorld::m_world->m_camera->m_mid_plane;

    if (range > far)
        range = far;
    CPnt3D to;
    CPnt3D end(0.0f, 0.0f, -range);
    bool aimed = false;

    mat->Transform(&end, &to, 1);
    zdb::IntersectStruct *hit = 0;
    zdb::IntersectStruct *act = 0;

    zdb::DiIntersect *di = IntersectsPast(this, from, &to);
    int n = di->m_Cnt;
    float best = (range + 1.0f) * (range + 1.0f);
    float bestAct = best;

    for (int i = 0; i < n; i++) {
        zdb::CNode *nd;
        zdb::IntersectStruct *is = &di->m_Intersects[i];
        CPnt3D v;

        if (ShootThrough(is)) {
            is->m_pos.Sub(from, &v);
            float d = v.x * v.x + v.y * v.y + v.z * v.z;

            if (!hit || d < best) {
                hit = is;
                best = d;
                if (d < bestAct) {
                    bestAct = d;
                    act = 0;
                }
            }
        } else if (nd = is->m_node, ActionOf(nd)) {
            is->m_pos.Sub(from, &v);
            float d = v.x * v.x + v.y * v.y + v.z * v.z;

            if (d < bestAct) {
                act = is;
                bestAct = d;
            }
        }
    }

    float sx, sy, sz;
    bool autoaimed = false;

    if (!data_0045A0C1 && !KitView(this)->m_retblocked && _AutoAim && GetCurReticule__5CZKitFv(&m_kit) != 4) {
        CPnt3D aim;
        CZSealBody *target = func_005AB1C0(this, &aim, &hit->m_pos);
        CTarget *t = target ? m_targets.FindTarget(target) : 0;

        if (t && (t->m_flags & 0x200)) {
            CPnt3D eye;

            GetNodePos(m_curr_firept, eye, false);
            PerturbAim(&aim, &eye, &target->m_accuracy);
            SetAimPointW(&aim, 0, false);
            aimed = true;
            /* (the words read through a reference: the x kept for the copy, y and z read again) */
            const CPnt3D &r = aim;

            sx = r.x;
            hit = 0;
            sy = r.y;
            autoaimed = aimed;
            sz = r.z;
            KitView(this)->m_aimpt = aim;
        }
    }
    KitView(this)->m_autoaimed = autoaimed;
    if (hit) {
        SetAimPointW(&hit->m_pos, &hit->m_norm, false);
        m_last_reticule = *hit;
        aimed = true;
        sx = hit->m_pos.x;
        sy = hit->m_pos.y;
        sz = hit->m_pos.z;
    } else if (!aimed) {
        SetAimPointW(&to, 0, false);
        ClearHit(&m_last_reticule);
    }
    if (act)
        m_last_reticule_action = *act;
    else
        ClearHit(&m_last_reticule_action);
    if (m_wep_seq.LinearWeight() == 0.0f) {
        KitView(this)->m_retblocked = false;
        return;
    }
    KitView(this)->m_18 = 0.0f;
    KitView(this)->m_1c = 0.0f;
    CPnt3D ap;

    bool ok = GetAP(ap);

    if (!ok)
        return;
    CPnt3D tgt = m_1458;

    if (m_eb8)
        m_eb8->Virtual60(&ap);
    zdb::IntersectStruct *block = func_005B6240(this, &ap, &tgt);
    CZFTSWeapon *cur = m_kit.GetCurItem();

    if (!autoaimed && block && cur && cur->m_ID != 190 &&
        ((unsigned char)GetWeaponState() == 1 || m_weapon_state == 18) && !(aimed && (!aimed || Near(block->m_pos, sx, sy, sz)))) {
        KitView(this)->m_retblocked = true;
        SetAimPointW(&block->m_pos, &block->m_norm, true);
        CPnt3D scr;

        bool on = zdb::CWorld::m_world->m_camera->GetScreenPos(&block->m_pos, &scr);

        if (on) {
            float m = scr.y;

            if (!(scr.x < scr.y))
                m = scr.x;

            KitView(this)->m_retblockedX = scr.x;
            KitView(this)->m_retblockedY = scr.y;
            KitView(this)->m_retblockedsize = m;
        }
    } else
        KitView(this)->m_retblocked = false;
}

/* the hit under the reticule (OVL2 0x5AA6D0: SOCOM 1's GetReticuleDi, `this + 0x340` there -- its
 * m_last_reticule; between GetReticuleSeal and CheckDIShoot in both builds, and called where SOCOM 1's
 * DoRangeText calls it, ahead of the range's Sub and sprintf, in the HUD's 0x216770) */
zdb::IntersectStruct *CZSealBody::GetReticuleDi()
{
    return &m_last_reticule;
}

/* the SEAL under the reticule: the one kept while the cache is on, else the hit node's (OVL2
 * 0x5AA690: SOCOM 1's GetReticuleSeal, the same 0x34 bytes, after CacheReticuleSeal in both builds) */
CZSealBody *CZSealBody::GetReticuleSeal()
{
    if (m_useCachedReticuleSeal)
        return m_cachedReticuleSeal;
    return GetSeal(m_last_reticule.m_node);
}

/* the toggle the radio menu flips (0x235300, 0x235440): on, bit 0 of 0x21C set and the SEAL of the
 * node at 0x3DC kept; off, both cleared (OVL2 0x5AA620, new in SOCOM II; the names ours) */
void CZSealBody::CacheReticuleSeal(bool on)
{
    if (on) {
        SetBit21c0(this, true);
        m_cachedReticuleSeal = CZSealBody::GetSeal(m_last_reticule.m_node);
    } else {
        SetBit21c0(this, false);
        m_cachedReticuleSeal = 0;
    }
}

/* whether the fire point's offset is set: any of its fields off zero (a macro: the zero is a local,
 * because MW puts a float literal first in `c.eq.s` whichever way the test is written while a
 * variable keeps the field first; and the offset is picked by a ternary in place, each arm its own
 * block -- through an inline function, a pointer result or an if/else, MW presets the field's address
 * and drops its block) */
#define FIREPOS_SET(seal, zero) \
    ((seal)->m_curr_firepos.x != (zero) || (seal)->m_curr_firepos.y != (zero) || (seal)->m_curr_firepos.z != (zero))

/* the pickup under the reticule: a weapon within 20 of the hit, when the hit's material is a pickup's
 * -- the next nearest handed back too when asked -- or online the MP bomb there, whichever lies
 * nearer the SEAL's fire point, that within 20 of it (OVL2 0x5A9C70: TickPickups 0x593DA0 asks it
 * before 0x5A9F40; new in SOCOM II, the name ours) */
CPickup *CZSealBody::GetReticuleWeapon(CPickup **secondary)
{
    zdb::IntersectStruct *hit = &m_last_reticule;

    if (!hit)
        return 0;
    CMaterialEx *mat = MaterialEx(hit->GetMaterial());

    if (!mat || !mat->IsPickup())
        return 0;
    float d = 9999.0f;
    bool full = !m_kit.HasPickupSlot();
    float zero = 0.0f;
    CPnt3D pos = m_posW + (FIREPOS_SET(this, zero) ? m_curr_firepos : CPnt3D::zero);
    CPickup *other = 0;
    CPickup *p = CPickup::GetClosest(&hit->m_pos, 20.0f, CPickup::PU_WEAPON, secondary ? &other : 0, full);

    if (p) {
        CPnt3D v;

        pos.Sub(p->GetPosW(), &v);
        d = v.x * v.x + v.y * v.y + v.z * v.z;
    }
    if (other && secondary) {
        CPnt3D v;

        pos.Sub(other->GetPosW(), &v);
        d = v.x * v.x + v.y * v.y + v.z * v.z;
        if (d < 400.0f)
            *secondary = other;
    }
    if (theNetwork.m_9) {
        CPickup *bomb = CPickup::GetClosest(&hit->m_pos, 20.0f, CPickup::PU_MPBOMB, 0, true);

        if (bomb) {
            CPnt3D v;

            pos.Sub(bomb->GetPosW(), &v);
            float b = v.x * v.x + v.y * v.y + v.z * v.z;

            if (b < d) {
                d = b;
                p = bomb;
            }
        }
    }
    if (!(d <= 400.0f))
        p = 0;
    return p;
}

/* the weapon lying nearest the SEAL's fire point within 20, if any -- the next nearest handed back
 * too when asked and within 20 -- a weapon the kit has no slot for offered only as that (OVL2
 * 0x5A9F40: TickPickups 0x593DA0 asks it; new in SOCOM II, the name ours). The distance is one
 * variable: the second weapon's replaces the first's before the last test, as the target's $f20 */
CPickup *CZSealBody::GetProximityWeapon(CPickup **secondary)
{
    float d = 9999.0f;
    bool full = !m_kit.HasPickupSlot();
    float zero = 0.0f;
    CPnt3D pos = m_posW + (FIREPOS_SET(this, zero) ? m_curr_firepos : CPnt3D::zero);
    CPickup *other = 0;
    CPickup *p = CPickup::GetClosest(&pos, 20.0f, CPickup::PU_WEAPON, secondary ? &other : 0, full);

    if (p) {
        CPnt3D v;

        pos.Sub(p->GetPosW(), &v);
        d = v.x * v.x + v.y * v.y + v.z * v.z;
    }
    if (other && secondary) {
        CPnt3D v;

        pos.Sub(other->GetPosW(), &v);
        d = v.x * v.x + v.y * v.y + v.z * v.z;
        if (d < 400.0f)
            *secondary = other;
    }
    if (!(d <= 400.0f))
        p = 0;
    return p;
}

/* online, the MP bomb lying within 20 of the SEAL's fire point, if any (OVL2 0x5AA110: TickPickups
 * 0x593DA0 asks it; new in SOCOM II, the name ours) */
CPickup *CZSealBody::GetProximityMPBomb()
{
    if (!theNetwork.m_9)
        return 0;
    float zero = 0.0f;
    CPnt3D pos = m_posW + (FIREPOS_SET(this, zero) ? m_curr_firepos : CPnt3D::zero);
    CPickup *bomb = CPickup::GetClosest(&pos, 20.0f, CPickup::PU_MPBOMB, 0, true);

    if (bomb) {
        CPnt3D d;

        pos.Sub(bomb->GetPosW(), &d);
        if (d.x * d.x + d.y * d.y + d.z * d.z < 400.0f)
            return bomb;
    }
    return 0;
}

extern "C" {
/* a node's extension and its parent (zdb::CNode's inline accessors: their out-of-line copies follow
 * CNodeAction::GetAction, OVL2 0x5AA610 and 0x5AA600 -- the drafts') */
zdb::CNodeEx *func_005AA610(zdb::CNode *node);
zdb::CNode *func_005AA600(zdb::CNode *node);
}

/* the action under the reticule: the node of the action's hit, else of the reticule's, offering one
 * itself or up its tree -- none when its valve reads 99 or it takes no action; and when asked, only
 * within its range across of the SEAL's fire point and (unless -1) its range up (OVL2 0x5AA240; SOCOM
 * 1's GetReticuleAction, after the pickup tests in both builds: GetAction(node, true) inlined one level,
 * the accessors and the recursion called; SOCOM 1's built the node's matrix, SOCOM II's only makes one) */
/* the action a node offers, up its tree (CNodeAction::GetAction(node, true) as the image inlines it into
 * its caller: one level, its accessors and its recursion called -- an inline so its variables number
 * after the caller's, as the inlined original's did) */
static inline CNodeAction *ActionOf(zdb::CNode *node)
{
    CNodeAction *a = 0;

    if (node) {
        a = dynamic_cast<CNodeAction *>(func_005AA610(node));
        if (!a)
            a = CNodeAction::GetAction(func_005AA600(node), true);
    }
    return a;
}

CNodeAction *CZSealBody::GetReticuleAction(bool b)
{
    zdb::CNode *node = m_last_reticule_action.m_node ? m_last_reticule_action.m_node : m_last_reticule.m_node;
    CNodeAction *action = ActionOf(node);
    CNodeAction *r = 0;

    if (action && action->m_valve->m_value != 99 && action->m_type != NODEACTION_NONE) {
        if (b) {
            CMatrix m;
            float zero = 0.0f;
            CPnt3D pos = m_posW + (FIREPOS_SET(this, zero) ? m_curr_firepos : CPnt3D::zero);
            CPnt3D at((m_last_reticule_action.m_node ? &m_last_reticule_action : &m_last_reticule)->m_pos);
            CPnt3D d = at - pos;

            at.y = 0.0f;
            float dy = d.y * d.y;

            if (d.x * d.x + d.z * d.z < action->m_range * action->m_range) {
                if (action->m_20 == -1.0f)
                    r = action;
                else if (dy < action->m_20 * action->m_20)
                    r = action;
            }
        } else
            r = action;
    }
    return r;
}

/* the head's place and its look's direction in the world, each when asked (OVL2 0x5A3070; SOCOM 1's
 * GetHeadParams, after CheckForRegionTrigger in both builds -- SOCOM 1's filled both always) */
void CZSealBody::GetHeadParams(CPnt3D *pos, CPnt3D *dir)
{
    if (pos) {
        PartInSpace(m_head, 0, pos, 0);
        m_node->m_matrix.Transform(pos, 1);
    }
    if (dir)
        TransformVectors(&m_node->m_matrix, &m_look.m_curr_dir, dir, 1);
}

/* the head's look's direction in the world: through the node's rotation (OVL2 0x5AF500; SOCOM 1's,
 * the look held) */
CPnt3D *CZSealBody::GetLookDirW(CPnt3D *out) const
{
    TransformVectors(&m_node->m_matrix, &m_look.m_curr_dir, out, 1);
    return out;
}

/* the head turned forward by a pitch, or not at all (OVL2 0x5AF470; SOCOM 1's RotateHead, after
 * GetLookQuat in both builds: the app camera's first- and third-person transitions and the SEAL's
 * constructor call it, setting only $f12 -- SOCOM 1's second float, unread there too, is gone) */
void CZSealBody::RotateHead(float pitch)
{
    if (pitch != 0.0f) {
        CQuat q;

        q.MakeXRot(pitch);
        m_fwdlook = q;
    } else
        m_fwdlook = CQuat::identity;
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

extern "C" {
/* the blend tests of seal_body.cpp: anim types 8 or 9 on top, 10 or 11, and a type of the set's (OVL2
 * 0x587F50, 0x587F10, 0x57DF10) */
bool func_00587F50(CZSealBody *seal);
bool func_00587F10(CZSealBody *seal);
bool func_0057DF10(CZSealBody *seal);
/* the farthest a SEAL may be moving and still take a pickup (OVL2 data: 32) */
extern float data_006508A0;
}

/* whether the SEAL may take a weapon up now: alive, on the ground, in neither blend pair, moving
 * slower than the limit, working nothing, holding no class-4 item, not in the set's blend, and
 * free to fire (OVL2 0x5A81D0, new in SOCOM II: CSealCtrl::TickPickups asks it; the name unknown) */
extern "C" bool func_005A81D0(CZSealBody *seal)
{
    if (!seal->m_isAlive)
        return false;
    if (seal->m_state == stateInAir || func_00587F50(seal) || func_00587F10(seal))
        return false;
    const CPnt3D &v = seal->m_velM;

    if (!(v.x * v.x + v.y * v.y + v.z * v.z <= data_006508A0 * data_006508A0))
        return false;
    if (seal->m_eb8 || seal->m_kit.GetCurItemFireClass() == CZKit::FIRE_CLASS_145)
        return false;
    if (func_0057DF10(seal))
        return false;
    return seal->CanFire(true, false);
}

/* a point turned round into another (an inline: written into an out-parameter, the input's loads
 * fold into sp offsets while the stores keep their field-address registers -- the AI core's
 * negate(&p, &p) in ai_path.cpp; the name ours) */
static inline void NegateInto(const CPnt3D *in, CPnt3D *out)
{
    out->x = -in->x;
    out->y = -in->y;
    out->z = -in->z;
}

extern "C" {
/* (OVL2 0x5B4340, not written yet: a test of the SEAL's state against a kind, below) */
bool func_005B4340(CZSealBody *seal, unsigned char kind);
/* the restrained anim set (OVL1 bss, below) */
extern AnimSet *data_0044D400;
/* a halfword's bits tested (seal.cpp's tail copy, OVL2 0x5551A0) */
bool func_005551A0(unsigned short flags, unsigned short bits);
}

/* whether an entity takes no fire (an inline, as zseal.h's IsNavySeal: its bool is converted,
 * `sltu`, before the branch -- the bit returned bare is tested as it is; the name ours) */
static inline bool NoShoot(const CEntity *e)
{
    return e->m_noshoot ? true : false;
}

/* whether the SEAL may knife a victim: the victim there, alive and of another team, neither restrained
 * nor in the restrained set; the state test of kind 6 passed, no stab's anim on top, no scope up, the
 * victim not one that takes no fire, the SEAL not prone, an AI victim not aware of a direction or more;
 * the victim ahead of the SEAL's fire point within the knife's cosine of its facing, flattened, and the
 * SEAL behind the victim (OVL2 0x5A98B0; SOCOM 1's CanUseKnife, after CanUseCQB in both builds, the
 * same tests -- SOCOM 1's measured from the SEAL's position) */
bool CZSealBody::CanUseKnife(CZSealBody *victim)
{
    if (!victim || !victim->m_isAlive || (victim->m_team & m_team))
        return false;
    if (victim->m_restrained || victim->m_animset == data_0044D400)
        return false;
    if (!func_005B4340(this, 6))
        return false;
    if (IsCurrentBlend((AnimType)AnimTypes::Stab) || IsCurrentBlend((AnimType)AnimTypes::PistolStab) ||
        IsCurrentBlend((AnimType)AnimTypes::Stabbed))
        return false;
    if (m_kit.IsUsingScope())
        return false;
    if (NoShoot(victim))
        return false;
    CSealCtrlAi *ai = victim->GetSealCtrlAi();

    if (GetStance(true) == stateProne)
        return false;
    if (ai) {
        AI_AWARE aware = ai->GetAiAware();

        if (aware == AIA_DIRECTION || aware == AIA_SPECIFIC)
            return false;
    }
    float zero = 0.0f;
    const CPnt3D &pos = m_posW + (FIREPOS_SET(this, zero) ? m_curr_firepos : CPnt3D::zero);
    const CPnt3D *vpos = (const CPnt3D *)victim->m_node->m_matrix.m_matrix[3];
    CPnt3D fwd(*(const CPnt3D *)m_node->m_matrix.m_matrix[2]);
    CPnt3D d = *vpos - pos;

    NegateInto(&fwd, &fwd);
    fwd.y = 0.0f;
    d.y = 0.0f;
    fwd.Normalize();
    d.Normalize();
    if (!(Dot(d, fwd) <= KNIFE_DOT)) {
        CPnt3D vfwd(*(const CPnt3D *)victim->m_node->m_matrix.m_matrix[2]);
        CPnt3D back = pos - *vpos;

        NegateInto(&vfwd, &vfwd);
        vfwd.y = 0.0f;
        back.y = 0.0f;
        if (Dot(back, vfwd) < 0.0f)
            return true;
    }
    return false;
}

/* whether the SEAL may take a victim in a CQB attack: the victim there, alive and of another team; the
 * SEAL's top blend without flag 2; the state test of kind 7 passed; neither SEAL in a CQB anim (the
 * attack's two or the victim's falls); the victim not one that takes no fire; an item in hand, not
 * none; no scope up, neither prone; and the victim within 30 of the SEAL's fire point, ahead within the
 * CQB attack's cosine of its facing, flattened (OVL2 0x5A9500; SOCOM 1's CanUseCQB, after
 * FindDeathAnimTypeCQB in both builds; the AI's sensors and the CQB attack state ask it too) */
bool CZSealBody::CanUseCQB(CZSealBody *victim)
{
    if (!victim || !victim->m_isAlive || (victim->m_team & m_team))
        return false;
    CZBodyAnimBlend *top = TopBlend();

    if (top && func_005551A0(top->m_flags, 2))
        return false;
    if (!func_005B4340(this, 7))
        return false;
    if (IsCurrentBlend((AnimType)AnimTypes::CQB) || IsCurrentBlend((AnimType)AnimTypes::PistolCQB) ||
        IsCurrentBlend((AnimType)AnimTypes::CQBVictimFallBackwards) || IsCurrentBlend((AnimType)AnimTypes::CQBVictimFallForwards))
        return false;
    if (victim->IsCurrentBlend((AnimType)AnimTypes::CQB) || victim->IsCurrentBlend((AnimType)AnimTypes::PistolCQB) ||
        victim->IsCurrentBlend((AnimType)AnimTypes::CQBVictimFallBackwards) || victim->IsCurrentBlend((AnimType)AnimTypes::CQBVictimFallForwards))
        return false;
    if (NoShoot(victim))
        return false;
    CZKit *kit = &m_kit;
    CZFTSWeapon *item = kit->GetCurItem();

    if (!item)
        return false;
    if (item->m_ID == EQUIP_NONE)
        return false;
    if (kit->IsUsingScope())
        return false;
    if (GetStance(true) == stateProne)
        return false;
    if (victim->GetStance(true) == stateProne)
        return false;
    float zero = 0.0f;
    CPnt3D pos = m_posW + (FIREPOS_SET(this, zero) ? m_curr_firepos : CPnt3D::zero);
    const CPnt3D *vpos = (const CPnt3D *)victim->m_node->m_matrix.m_matrix[3];
    CPnt3D fwd(*(const CPnt3D *)m_node->m_matrix.m_matrix[2]);
    CPnt3D d = *vpos - pos;

    NegateInto(&fwd, &fwd);
    fwd.y = 0.0f;
    d.y = 0.0f;
    fwd.Normalize();
    if (d.Normalize() <= 30.0f && !(Dot(d, fwd) <= CQB_DOT))
        return true;
    return false;
}

/* the death a CQB attack gives its victim: from behind or in front, by the attacker's offset against
 * the victim's facing, flattened (OVL2 0x5A9420; SOCOM 1's FindDeathAnimTypeCQB, after CanUseRestraint
 * in both builds, the same arithmetic).
 * OPEN, 10 instructions (the size right since round 23): the target loads both nodes before the
 * facing's three stores (the last in the call's delay slot); MW here loads this node after them. The
 * negation is NegateInto(&fwd, &fwd) -- the out-parameter form folds the loads into sp offsets and keeps
 * only &fwd.y in a register, as the target. Tried since: the nodes in locals (either order), the
 * positions in pointer locals, the offset computed first, both points assigned, GetNode(). Before: the
 * offset computed first, the facing assigned after it, y through a pointer, Negate(fwd.y). Round 39:
 * CanUseRestraint/CanUseCQB/CanUseKnife share the shape and matched with the Sub's `this` (the victim's
 * position) a pointer local declared before the facing -- here the Sub's `this` is this SEAL's own
 * position, and that local (pointer, reference, node pointer, either order, an inline helper taking
 * the facing by value and both points by reference) still loads after the stores: 10 every time.
 * Round 56 (igdump --pcode): the pre-colouring code already has this node's load after the facing's
 * stores, so its register falls free (v0) -- the target's v1/a3 need the load among the facing's
 * loads; this SEAL's position first through a pointer, a reference pair, both nodes in locals (either
 * order) and the facing assigned after the pointer: 10 each (build/scratch/seal-r56/v_cqb.py). SOCOM 1's
 * own code (0x299900) has the target's order with the facing and the Sub's slots swapped. */
AnimType CZSealBody::FindDeathAnimTypeCQB(CZSealBody *victim)
{
    CPnt3D fwd(*(const CPnt3D *)victim->m_node->m_matrix.m_matrix[2]);
    CPnt3D d = *(const CPnt3D *)m_node->m_matrix.m_matrix[3] - *(const CPnt3D *)victim->m_node->m_matrix.m_matrix[3];

    NegateInto(&fwd, &fwd);
    fwd.y = 0.0f;
    d.y = 0.0f;
    if (Dot(d, fwd) < 0.0f)
        return AnimTypes::CQBVictimFallForwards;
    return AnimTypes::CQBVictimFallBackwards;
}

extern "C" {
/* (OVL2 0x5B4340, defined below: a test of the SEAL's state against a kind -- CanTraverse and
 * PreTick ask it too; the name unknown) */
bool func_005B4340(CZSealBody *seal, unsigned char kind);
}

extern "C" {
/* the anim set the zoom refuses (OVL1 bss) */
extern AnimSet *data_0044D3F8;
/* whether a carried body's homing runs: one carried and the top blend's homing set (seal_body.cpp's
 * range, OVL2 0x579EA0: its draft); the top blend's test of the blends a hand-held item allows
 * (seal.cpp's OVL2 0x54F6E0) */
bool func_00579EA0(CZSealBody *seal);
bool func_0054F6E0(CZSealBody *seal, bool blend, bool mount, bool anims);
/* (OVL1 0x2C2FD0: whether a SEAL's hits go unfelt, below) */
bool func_002C2FD0(CZSealBody *seal);
}

/* whether the top blend is a stab's, a restraint's or a CQB attack's (inlines: each fetches the top
 * blend again, the bool materialised -- the names ours) */
static inline bool IsStabbing(CZSealBody *seal)
{
    short type = seal->TopBlend()->m_animType;

    if (type == AnimTypes::PistolStab || type == AnimTypes::Stab)
        return true;
    return false;
}

static inline bool IsRestraining(CZSealBody *seal)
{
    short type = seal->TopBlend()->m_animType;

    if (type == AnimTypes::ApplyRestraints || type == AnimTypes::PistolApplyRestraints)
        return true;
    return false;
}

static inline bool IsCQBAttacking(CZSealBody *seal)
{
    short type = seal->TopBlend()->m_animType;

    if (type == AnimTypes::CQB || type == AnimTypes::PistolCQB)
        return true;
    return false;
}

extern "C" {
/* an anim the restraint refuses on top (OVL1 data, an anim type) */
extern short data_003DEF70;
/* (the cosines the restraint's, the knife's and the CQB attack's facings must pass are the unit's
 * statics RESTRAINT_DOT, KNIFE_DOT and CQB_DOT, at the top) */
/* a halfword's bits tested (seal.cpp's tail copy, OVL2 0x5551A0) and whether the AI's SEAL may be
 * taken hostage (seal_ai.cpp's OVL2 0x555B80, the third argument always 0) */
bool func_005551A0(unsigned short flags, unsigned short bits);
bool func_00555B80(CSealCtrlAi *ai, int kind, int flag);
}

/* whether the SEAL may restrain a victim: the victim there, alive and restrainable; the state test of
 * kind 8 passed; neither restraint anim (the victim's or the captor's two) nor the anim it refuses on
 * top; an AI victim neither stunned nor refusing the hostage state; the victim's top blend without
 * flag 2; no scope up, not prone; and the victim ahead of the SEAL's fire point within the
 * restraint's cosine of its facing, flattened (OVL2 0x5A90B0; SOCOM 1's CanUseRestraint, before
 * FindDeathAnimTypeCQB in both builds -- SOCOM 1's took the victim from behind or surrendering after
 * the facing test) */
bool CZSealBody::CanUseRestraint(CZSealBody *victim)
{
    if (!victim || !victim->m_isAlive)
        return false;
    if (!victim->m_restrainable)
        return false;
    if (!func_005B4340(this, 8))
        return false;
    if (IsCurrentBlend((AnimType)AnimTypes::ReceiveRestraints) ||
        IsCurrentBlend((AnimType)AnimTypes::ApplyRestraints) || IsCurrentBlend((AnimType)AnimTypes::PistolApplyRestraints))
        return false;
    if (IsCurrentBlend((AnimType)data_003DEF70))
        return false;
    if (victim->m_control ? ((CSealCtrl *)victim->m_control)->IsAi() : false) {
        CSealCtrlAi *ai = victim->GetSealCtrlAi();

        if (ai->AiStateIs(AI_STATE_STUNRESPONSE) || !func_00555B80(ai, 3, 0))
            return false;
    }
    CZBodyAnimBlend *top = victim->TopBlend();

    if (top && func_005551A0(top->m_flags, 2))
        return false;
    if (m_kit.IsUsingScope())
        return false;
    if (GetStance(true) == stateProne)
        return false;
    float zero = 0.0f;
    CPnt3D pos = m_posW + (FIREPOS_SET(this, zero) ? m_curr_firepos : CPnt3D::zero);
    const CPnt3D *vpos = (const CPnt3D *)victim->m_node->m_matrix.m_matrix[3];
    CPnt3D fwd(*(const CPnt3D *)m_node->m_matrix.m_matrix[2]);
    CPnt3D d = *vpos - pos;

    NegateInto(&fwd, &fwd);
    fwd.y = 0.0f;
    d.y = 0.0f;
    fwd.Normalize();
    d.Normalize();
    if (!(Dot(d, fwd) <= RESTRAINT_DOT))
        return true;
    return false;
}

/* whether the SEAL may zoom its view: alive; online, its hits felt; standing, crouched, prone,
 * peeking or in the air (carrying, or anything else, not), its top blend not in state 8; not
 * throwing, changing weapons, stabbing, restraining or in a CQB attack; no overlay playing, the
 * top blend's item test failed, not in the anim set it refuses, and no carried body homing (OVL2
 * 0x5A8DA0; SOCOM 1's CanZoom, after CanChangeItem in both builds -- SOCOM II's tests from the
 * overlay on are its own, the homing asked twice) */
bool CZSealBody::CanZoom()
{
    if (!m_isAlive)
        return false;
    if (theNetwork.m_9 && func_002C2FD0(this))
        return false;
    CZBodyAnimBlend *top = TopBlend();

    switch (m_state) {
    case stateCarrying:
        return false;
    case stateStand:
    case stateCrouch:
    case stateProne:
    case statePeek:
    case stateInAir:
        break;
    default:
        return false;
    }
    if (top->m_state == 8)
        return false;
    if (IsThrowing(false))
        return false;
    if (IsChangingWeapons(true))
        return false;
    if (IsStabbing(this))
        return false;
    if (IsRestraining(this))
        return false;
    if (IsCQBAttacking(this))
        return false;
    if (m_overlay.m_anim)
        return false;
    if (func_0054F6E0(this, true, true, true))
        return false;
    if (m_animset == data_0044D3F8)
        return false;
    if (func_00579EA0(this))
        return false;
    if (func_00579EA0(this))
        return false;
    return true;
}

/* whether the SEAL may change what it holds: alive, its hands free (CanFire), the state test passed,
 * not in the air, working no turret, its trigger not held (OVL2 0x5A8CB0; SOCOM 1's, which tested the
 * first two and the air) */
bool CZSealBody::CanChangeItem()
{
    if (!m_isAlive)
        return false;
    if (!CanFire(true, true))
        return false;
    if (!func_005B4340(this, 4))
        return false;
    if (m_airborne || m_state == stateInAir)
        return false;
    if (m_eb8)
        return false;
    if (m_control && ((CSealCtrl *)m_control)->m_trigger == TRIGGER_DOWN)
        return false;
    return true;
}

extern "C" {
/* the anim sets of a SEAL held: the restrained one (seal.cpp's 0x5463F0 and the AnimSet view's
 * 0x58C600 keep it), and a third the carry refuses beside the throwables' (OVL1 bss) */
extern AnimSet *data_0044D400;
extern AnimSet *data_0044D410;
}

/* whether the SEAL may pick a body up: the body there, dead, neither carrying nor carried, not
 * restrained nor in the restrained set; the SEAL alive, not stunned, in neither the throwables' set
 * nor the third, the body's node showing whole, no scope up, neither peeking nor its top blend in
 * state 3, neither carrying nor carried, holding no body, not prone, and the body within 300 (the
 * squared distance) of its fire point (OVL2 0x5A89C0; SOCOM 1's CanUseBodyCarry, after AimWeapon in
 * both builds -- SOCOM 1's measured from the SEAL's position, with fewer tests) */
bool CZSealBody::CanUseBodyCarry(CZSealBody *victim)
{
    if (!victim || victim->m_isAlive || victim->m_state == stateCarrying || victim->m_state == stateCarried)
        return false;
    if (victim->m_restrained || victim->m_animset == data_0044D400)
        return false;
    if (!m_isAlive)
        return false;
    if (m_am_stunned)
        return false;
    if (m_animset == data_0044D408)
        return false;
    if (m_animset == data_0044D410)
        return false;
    zdb::CNode *node = victim->GetNode();

    if (!node->m_active || node->m_Opacity < 1.0f)
        return false;
    if (m_kit.IsUsingScope())
        return false;
    if (m_state == statePeek || ((CZBodyAnimBlend *)func_00555170((char *)this + 0x1c0))->m_state == statePeek)
        return false;
    if (m_state == stateCarrying || m_state == stateCarried)
        return false;
    if (m_carry)
        return false;
    if (GetStance(true) == stateProne)
        return false;
    float zero = 0.0f;
    CPnt3D pos = m_posW + (FIREPOS_SET(this, zero) ? m_curr_firepos : CPnt3D::zero);
    CPnt3D d = pos - victim->m_posW;

    if (!(d.x * d.x + d.y * d.y + d.z * d.z <= 300.0f))
        return false;
    return true;
}

/* an entity that is a turret (kind 3), or none (the inline cast 0x5A88C0 shows: zentity.h's CTurret
 * is only declared here) */
static inline CTurret *AsTurret(CEntity *e)
{
    return e && e->m_type == CEntity::ENT_TYPE_TURRET ? (CTurret *)e : 0;
}

/* whether the SEAL may work the entity its controller holds (action 19): carrying no body, no scope
 * or binoculars up, the state test of kind 10 passed, and the entity agreeing -- CEntity's slot 0x5C,
 * handed the SEAL and the entity again when it is a turret (OVL2 0x5A88C0, new in SOCOM II:
 * CSealCtrl::PerformProximityActions asks it; the name unknown) */
extern "C" bool func_005A88C0(CZSealBody *seal)
{
    if (seal->m_carry)
        return false;

    bool r = false;
    bool scoped = false;

    if (seal->m_kit.IsUsingScope() || seal->m_kit.IsUsingBinocs())
        scoped = true;
    if (func_005B4340(seal, 10) && !scoped) {
        CEntity *e = seal->m_control ? ((CSealCtrl *)seal->m_control)->m_16c : 0;

        if (e && e->Virtual5C(seal, AsTurret(e)))
            r = true;
    }
    return r;
}

/* the aim along the item in hand: its gear part's x axis, or straight ahead without one (OVL2
 * 0x5A8790; SOCOM 1's AimWeapon, before CanUseBodyCarry in both builds -- SOCOM 1's took the rifle
 * and the pistol themselves, SOCOM II's the item's part out of the table, item 4 too) */
void CZSealBody::AimWeapon()
{
    switch (m_item) {
    case 1:
    case 2:
    case 4: {
        CPnt3D pos;
        CQuat q;

        PartInSpace(m_item_gear[m_item].m_part, 0, &pos, &q);
        q.Apply(&CPnt3D::unit_x, &m_aim_dir, 1);
        break;
    }
    default:
        m_aim_dir = CPnt3D::unit_nz;
    }
}

/* (CZWeapon::RemoveWeaponEventCallback is zweapon.h's, through zchar.h) */
extern "C" {
/* the decal handles' buffers and counts (OVL1 bss; SOCOM 1's freed one buffer and zeroed one count) */
extern void *data_0044D550;
extern void *data_0044D548;
extern int data_0044D508;
extern int data_0044D510;
extern int data_0044D518;
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

/* (zdb::CWorld::GetTexHandle, znode.h's: InitDecalHandles calls OVL2's copy, 0x5A4F70, right after
 * it -- an inline's out-of-line copy in the original; st_core.cpp defines the function, out of line,
 * and a definition here would put this unit's span in OVL1) */

extern "C" {
/* the decals rdr (OVL1 bss, the weapons' 0x437D00; a material's index by name is
 * CMaterialExArray::ResolveMaterialIndex, zweapon.h's: OVL1 0x2DE9E0) */
extern _zrdr *data_00437D00;
}
/* (zrdr.h's, which this unit cannot include beside zanim.h's _zrdr) */
bool zrdr_findreal(const _zrdr *rdr, const char *tag, float *out, int n);

/* the decal textures: the footsteps' by material, the blood drips' with their size ranges (OVL2
 * 0x5A4CF0; SOCOM 1's, the arrays SOCOM II's). Each tag found in the test, assigned there (the
 * target tests v0 with the copy in the delay slot), the locals declared first in tools/perm.py's
 * order, the counter and the item one variable each for both loops. Round 36, read off MW's colouring
 * (igdump): the steps tag's variable REUSED for the blood tag -- a variable reused for an unrelated
 * second purpose is split into a new web numbered after the others (the counter's second web too), so
 * the blood tag is coloured first (s0) -- and the blood count a separate local declared before `tex`
 * (s5); the shared count's second web would be numbered last instead and take s0 */
void CZSealBody::InitDecalHandles()
{
    char *texture;
    int i;
    _zrdr *steps;
    char *material;
    int n;
    int nblood;
    _zrdr *rdr;
    zdb::CTexHandle *tex;
    _zrdr *item;
    int index;

    rdr = data_00437D00;
    data_0044D550 = _calloc(4, CMaterialExArray::m_materialex_list.size(), 0, 0);
    if ((steps = zrdr_findtag(rdr, "FOOTSTEP_DECALS")) != 0) {
        n = steps->Count();
        for (i = 0; i < n; i++) {
            item = ItemAt(steps, i);
            material = zrdr_findstring(item, "MATERIALNAME");
            texture = zrdr_findstring(item, "TEXTURENAME");
            index = CMaterialExArray::ResolveMaterialIndex(material);
            ((zdb::CTexHandle **)data_0044D550)[index] = zdb::CWorld::GetTexHandle(texture);
        }
    }
    if ((steps = zrdr_findtag(rdr, "BLOOD_DRIP_DECALS")) != 0) {
        CZDecalDef def;

        def.m_decalminsize = 0;
        def.m_decalmaxsize = 0;
        def.m_texture = 0;
        nblood = steps->Count();
        m_blooddriptexhandles.reserve(nblood);
        for (i = 0; i < nblood; i++) {
            item = ItemAt(steps, i);
            tex = zdb::CWorld::GetTexHandle(zrdr_findstring(item, "TEXTURENAME"));
            if (tex) {
                float min = 0.0f;
                float max = 0.0f;

                zrdr_findreal(item, "MIN_SIZE", &min, 1);
                zrdr_findreal(item, "MAX_SIZE", &max, 1);
                def.m_decalminsize = min;
                def.m_decalmaxsize = max;
                def.m_texture = tex;
                m_blooddriptexhandles.push_back(def);
            }
        }
    }
}

/* (SOCOM 1's InitDatabaseHandles: the handles taken again, OVL2 0x5A4CE0) */
void CZSealBody::InitDatabaseHandles()
{
    ReInitDatabaseHandles();
}

extern "C" {
/* the ripple anims, by gait (OVL1 bss: SOCOM 1's m_bigrippleanim and m_smallrippleanim, three each),
 * the fall into water, the drops (SOCOM 1's m_dropanim and two more), the CQB and body-fall sounds
 * (SOCOM 1's m_cqb_sound, m_body_fall_sound), and the ladder's slide with its instance and the
 * goggles' and the holster's sounds (new in SOCOM II; the names ours) */
extern CZAnim *data_0044D520[3];
extern CZAnim *data_0044D530[3];
extern CZAnim *data_0044D540;
extern CSnd *data_0044D578;
extern CSnd *data_0044D580;
extern CSnd *data_0044CE20;
extern CSndInstance *data_0044CE28;
extern CSnd *data_0044CE30;
extern CSnd *data_0044CE38;
extern CSnd *data_0044CE40;
extern CSnd *data_0044CE48;
}

/* the database's handles taken: the hand signals, the weapon events' callback, the ripple, fall and
 * drop anims, a footfall anim per material ("seal_footfall_" and its name), the sounds -- the
 * heartbeat's reopened (OVL2 0x5A49F0; SOCOM 1's ReInitDatabaseHandles, the footfall loop as SOCOM
 * 1's -- the satchel's drop anim stored over the foot bomb's, as SOCOM II's code has it) */
void CZSealBody::ReInitDatabaseHandles()
{
    int i;   /* (declared first: the count takes the lower saved register) */

    InitHandSignals();
    CZWeapon::AddWeaponEventCallback(HandleWeaponEvent, 1, 0);
    int n = CMaterialExArray::m_materialex_list.size();

    data_0044D520[0] = ZAnim.GetAnim("big_ripple_anim", 0);
    data_0044D520[1] = ZAnim.GetAnim("big_ripple_anim_walk", 0);
    data_0044D520[2] = ZAnim.GetAnim("big_ripple_anim_run", 0);
    data_0044D530[0] = ZAnim.GetAnim("small_ripple_anim", 0);
    data_0044D530[1] = ZAnim.GetAnim("small_ripple_anim_walk", 0);
    data_0044D530[2] = ZAnim.GetAnim("small_ripple_anim_run", 0);
    data_0044D540 = ZAnim.GetAnim("seal_fall_in_water", 0);
    data_0044D508 = (int)ZAnim.GetAnim("drop_item", 0);
    data_0044D510 = (int)ZAnim.GetAnim("drop_footbomb", 0);
    data_0044D510 = (int)ZAnim.GetAnim("drop_satchel", 0);
    data_0044D548 = _calloc(4, n, 0, 0);
    for (i = 0; i < n; i++) {
        ((CZAnim **)data_0044D548)[i] = 0;
        CMaterialEx *ex = MaterialEx(i);

        if (ex) {
            char name[64];

            sprintf(name, "seal_footfall_%s", ex->m_name);
            ((CZAnim **)data_0044D548)[i] = ZAnim.GetAnim(name, 0);
        }
    }
    data_0044D578 = CSnd::GetSoundByName(".RIFLE_BUTT");
    data_0044D580 = CSnd::GetSoundByName(".BONE_BRK_1");
    CloseHeartbeatSounds();
    OpenHeartbeatSounds();
    data_0044CE20 = CSnd::GetSoundByName("~LADDER_SLIDE");
    data_0044CE28 = 0;
    data_0044CE30 = CSnd::GetSoundByName(".NV_GOGGLES_ON");
    data_0044CE38 = CSnd::GetSoundByName(".NV_GOGGLES_OFF");
    data_0044CE40 = CSnd::GetSoundByName(".GUNTOHOLSTER");
    data_0044CE48 = CSnd::GetSoundByName(".GUNTOBACK");
}

/* the database's handles let go: the buffers freed, the blood decals' vector emptied, then ReInit's
 * (Uninit_ReInitDatabaseHandles, inlined) (OVL2 0x5A3BB0; SOCOM 1's with two more counts). The
 * vector goes through FreeVector (zsys.h), inlined one level: the class vector's pieces follow this
 * function out of line in reverse of their first use (0x5A3C60 the temporary's ~vector, 0x5A3CC0 the
 * deleter's swap, 0x5A3D00 clear(), 0x5A3D10 the temporary's constructor) -- and, generated in the
 * unit's first round, the deleter's clear and destructor go ahead of InitDecalHandles' reserve and
 * insert in the template tail, which then inline the temporary's destructor with its clear as the
 * target's 0x5B71B0 and 0x5B6CE0 do (the MSL headers round 17: seven rows byte-exact) */
void CZSealBody::UninitDatabaseHandles()
{
    if (data_0044D550)
        _free(data_0044D550, 0, 0);
    data_0044D550 = 0;
    FreeVector(m_blooddriptexhandles);
    Uninit_ReInitDatabaseHandles();
}

/* ReInitDatabaseHandles' handles let go: the drop anims' words cleared, the footfall anims' buffer
 * freed, the weapon-event callback removed (OVL2 0x5A3B50; SOCOM 1's Uninit_ReInitDatabaseHandles,
 * the integrator's lead from the game states' round 28: CMission::ReOpen_Unload1 calls it where SOCOM
 * 1's does -- SOCOM II clears three words where SOCOM 1 cleared one, and frees without a file and line) */
void CZSealBody::Uninit_ReInitDatabaseHandles()
{
    data_0044D508 = 0;
    data_0044D510 = 0;
    data_0044D518 = 0;
    if (data_0044D548)
        _free(data_0044D548, 0, 0);
    data_0044D548 = 0;
    CZWeapon::RemoveWeaponEventCallback(HandleWeaponEvent);
}

/* the six values' average weighted by their rates, the first counted three times, for a living SEAL
 * (OVL2 0x5A56D0, new in SOCOM II; the names ours) */
extern "C" float func_005A56D0(CZSealBody *seal)
{
    float sum = 0.0f;
    float rate = 0.0f;

    if (seal->m_isAlive) {
        sum += 3.0f * seal->m_ffc[0];
        rate += 3.0f * seal->m_1014[0];
        sum += seal->m_ffc[3];
        rate += seal->m_1014[3];
        sum += seal->m_ffc[1];
        rate += seal->m_1014[1];
        sum += seal->m_ffc[2];
        rate += seal->m_1014[2];
        sum += seal->m_ffc[4];
        rate += seal->m_1014[4];
        sum += seal->m_ffc[5];
        rate += seal->m_1014[5];
        /* (a ternary: the target's division padded with nops either side; an if/else is not) */
        sum = rate != 0.0f ? sum / rate : 0.0f;
    }
    return sum;
}

/* the health taken from the six locations' values: dead when the head's or the body's has run out
 * or the SEAL was dead already -- a bullet in the head then a head shot -- the health zeroed with the
 * death's kind and, for a SEAL alive till now, a death anim played (unless remote, a fall or a
 * grenade) and the HUD's hit flag set; else the health their weighted average, and when that fell a
 * damage anim played in the SEAL's stance (the same exceptions) and the flag set (OVL2 0x5A54D0;
 * SOCOM 1's UpdateHealth, its five health pairs six values now, the drops gone) */
void CZSealBody::UpdateHealth(DAMAGE_LOCATION where, SEAL_DEATH_TYPE type)
{
    float old = m_health;
    bool alive = m_isAlive;
    bool dead = false;

    /* (the flag compared == false: its andi before the branch) */
    if (m_ffc[0] <= 0.0f || m_ffc[3] <= 0.0f || alive == false) {
        dead = true;
        if (where == 0 && type == DEATH_BULLET)
            type = DEATH_HEADSHOT;
    }
    if (dead) {
        SetHealth(0.0f, type);
        if (alive) {
            if (!IsRemote() && type != DEATH_FALLING && type != DEATH_GRENADE) {
                bool b;

                WeaponHitAnim(FindRandomDeathAnim(where, &b), stateDead);
            }
            SetHudHitFlag();
        }
    } else {
        SetHealth(func_005A56D0(this), DEATH_NONE);
        if (m_health < old) {
            if (!IsRemote() && type != DEATH_FALLING && type != DEATH_GRENADE) {
                bool b;
                AnimType anim = FindRandomDamageAnim(where, &b);

                WeaponHitAnim(anim, GetStance(false));
            }
            SetHudHitFlag();
        }
    }
}

/* one of the six values set to its rate times a factor, none below zero, and when it changed started
 * -- value 3 with kind 5, value 0 with 3, any other with 0 (OVL2 0x5A5790, new in SOCOM II; the names
 * ours. The case tests run in reverse source order) */
extern "C" void func_005A5790(CZSealBody *seal, unsigned char i, float f)
{
    float v = seal->m_1014[i] * f;
    float *p = &seal->m_ffc[i];

    if (v != *p) {
        *p = v;
        if (v < 0.0f)
            *p = 0.0f;
        switch (i) {
        case 3:
            seal->UpdateHealth((DAMAGE_LOCATION)i, DEATH_BULLET);
            break;
        case 0:
            seal->UpdateHealth((DAMAGE_LOCATION)i, DEATH_HEADSHOT);
            break;
        default:
            seal->UpdateHealth((DAMAGE_LOCATION)i, DEATH_NONE);
            break;
        }
    }
}

extern "C" {
/* (OVL1 0x2C2FD0, the network game's: whether a SEAL's hits go unfelt -- the player's by a valve,
 * another's by its client's flag; the name unknown) */
bool func_002C2FD0(CZSealBody *seal);
/* two of seal_body.cpp's (OVL2 0x57E770, 0x585FF0; not written) */
void func_0057E770(CZSealBody *seal);
void func_00585FF0(CZSealBody *seal);
}

/* a weapon's hit: online, none when the SEAL's hits go unfelt and none from a team-mate without
 * friendly fire; none from a grenade (item 121) not gone off; a living SEAL's controller told, then
 * -- unless an AI navy SEAL's own team's grenades or flashbangs (121, 126) while it is in state 10,
 * or anyone's item 159, from itself or a team-mate -- the adrenaline raised, the hit kept and the
 * damage taken; a dead one, unless remote, thrown by a death anim; the shooter's id kept (OVL2
 * 0x5AC670; SOCOM 1's OnWeaponHit, CSealEx::OnWeaponHit tail-calls it, with the grenade and the
 * state-10 tests and the two calls after) */
int CZSealBody::OnWeaponHit(zdb::IntersectStruct *hit, CZProjectile *proj)
{
    if (theNetwork.m_9 && func_002C2FD0(this))
        return 0;
    m_lastshooter_id = proj->m_shooterID;
    if (theNetwork.m_9) {
        CZSealBody *shooter = AsSeal(CEntity::m_list.GetEntityById(m_lastshooter_id));

        if (m_lastshooter_id && !m_allow_friendly_fire && shooter && (shooter->m_team & m_team) &&
            !(m_team & 0x20000) && this != shooter)
            return 0;
    }
    if (proj && proj->m_weapon && proj->m_weapon->GetIDType() == 121 && !proj->WasDetonated())
        return 0;
    if (m_isAlive && GetNode()->m_active) {
        ((CSealCtrl *)m_control)->OnWeaponHit(hit, proj);
        CZWeapon *w = proj ? proj->m_weapon : 0;
        EQUIP_ITEM item = w ? w->m_ID : (EQUIP_ITEM)255;
        CSealCtrlAi *ai = ((CSealCtrl *)m_control)->GetSealCtrlAi();

        if (ai) {
            if (ai->IsNavySeal() && (item == 121 || item == 126) && (ai->m_stateStack.m_10 & (1 << 10))) {
                CEntity *e = CEntity::m_list.GetEntityById(proj->m_shooterID);

                if (e && (e == this || (m_team & e->m_team)))
                    return 0;
            } else if (item == 159) {
                CEntity *e = CEntity::m_list.GetEntityById(proj->m_shooterID);

                if (e && (e == this || (m_team & e->m_team)))
                    return 0;
            }
        }
        AdrenalineIncr(0.99f);
        if (hit) {
            m_wephit.m_pos = hit->m_pos;
            m_wephit.m_norm = hit->m_norm;
            m_wephit.m_node = hit->m_node;
        } else
            m_wephit.m_node = 0;
        HandleDamage(proj);
    } else if (!IsRemote()) {
        bool b;

        WeaponHitAnim(FindRandomDeathAnim((DAMAGE_LOCATION)3, &b), stateDead);
    }
    func_0057E770(this);
    func_00585FF0(this);
    return 0;
}

/* the last weapon hit forgotten, the blend stack's top taken (OVL2 0x5AC660; SOCOM 1's HandleWepHit
 * word for word, the hit held now) */
void CZSealBody::HandleWepHit()
{
    m_wephit.m_node = 0;
    TopBlend();
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

/* a cross product through VU0 (an inline, the name ours) */
static inline void CrossVU0(const CPnt3D &a, const CPnt3D &b, CPnt3D *out)
{
    sceVu0FVECTOR c;
    sceVu0FVECTOR va;
    sceVu0FVECTOR vb;

    va[0] = a.x;
    va[1] = a.y;
    va[2] = a.z;
    va[3] = 1.0f;
    vb[0] = b.x;
    vb[1] = b.y;
    vb[2] = b.z;
    vb[3] = 1.0f;
    sceVu0OuterProduct(c, va, vb);
    out->x = c[0];
    out->y = c[1];
    out->z = c[2];
}

/* a point turned round in place through two references (an inline: the loads keep the fields' address
 * registers, the stores fold to sp offsets -- NegateInto's the other way; the name ours) */
static inline void NegRef(const CPnt3D &in, CPnt3D &out)
{
    out.x = -in.x;
    out.y = -in.y;
    out.z = -in.z;
}

/* the anim set the SEALs share (OVL1 bss 0x44D408) */
extern AnimSet *data_0044D408;

/* the spine bent toward the aim: the aim's direction taken into the body's frame -- from the aim
 * point, or the fire point, to the frame's aim (m_1464), which a player's fire point too near a
 * wall puts back to the last frame's or pulls in front of him; an AI with no valid aim point, or a
 * remote SEAL, bends to the goal as it is -- then the low and high spine turned toward it by 0.1
 * and 0.4 of the weapon's weight (OVL2 0x5ACA70; SOCOM 1's PitchAnim, its second half the same,
 * its first new; Tick_0 hands it the aim's goal) */
void CZSealBody::PitchAnim(CPnt3D *goal)
{
    if (CBody::m_state == 2)
        return;
    float w = m_wep_seq.Weight();

    if (m_animset == data_0044D408)
        w = 1.0f;
    if (w <= 0.0f)
        return;
    CPnt3D dir = *goal;
    CQuat inv = m_quat;

    Negate(inv.vec.x);
    Negate(inv.vec.y);
    Negate(inv.vec.z);
    if (!((m_control ? ((CSealCtrl *)m_control)->IsAi() : false) && !m_aim_point_valid) && !IsRemote()) {
        CPnt3D ap;
        CPnt3D fp;
        bool got = GetAP(ap);
        bool gotfp = false;

        if (!got) {
            if (m_kit.CurWeaponIsM203Mode())
                gotfp = GetFirepointPos(fp, "firepoint_203");
            else
                gotfp = GetFP(fp);
            if (gotfp)
                ap = fp;
        }
        if (got || gotfp) {
            CPnt3D at = m_1464;

            if (IsPlayer() && m_curr_firept) {
                CPnt3D d = m_posW;

                d.y += m_root->m_pos.y;
                d.Sub(&at, &d);
                float dot = Dot(*(const CPnt3D *)m_node->m_matrix.m_matrix[2], d);
                bool ahead = dot > 1.0f;

                if (Dot(d, d) < 900.0f && ahead) {
                    CPnt3D fire = m_curr_firepos;

                    fire.Add(&m_posW, &fire);
                    fire.Sub(&at, &fire);
                    NegRef(fire, fire);
                    unsigned char norm = m_have_aim_norm;

                    if (norm && !(m_1470.y < theCharacterDynamics.m_maxSlopeCos)) {
                        if (!(Dot(*(const CPnt3D *)m_node->m_matrix.m_matrix[2], fire) <= 0.7f)) {
                            m_1464 = m_prev_1464;
                            at = m_prev_1464;
                        }
                    } else if (norm && m_1470.y < theCharacterDynamics.m_maxSlopeCos) {
                        CPnt3D back = *(const CPnt3D *)m_node->m_matrix.m_matrix[2];

                        NegRef(back, back);
                        float reach = Dot(m_curr_firepos, back);

                        reach *= 1.06f;

                        back.Scale(reach, &back);
                        if (!(reach <= dot)) {
                            m_1464 = m_prev_1464;
                            at = m_prev_1464;
                        }
                        d = m_posW;
                        d.y += m_root->m_pos.y;
                        d.Sub(&at, &d);
                        if (!(reach <= Dot(*(const CPnt3D *)m_node->m_matrix.m_matrix[2], d))) {
                            back.Add(&m_posW, &back);
                            back.Add(&m_root->m_pos, &back);
                            m_1464 = back;
                            at = back;
                            m_258_0 = false;
                        }
                    }
                }
            }
            dir = at - ap;
            inv.Apply(&dir, &dir, 1);
            dir.Normalize();
        }
        if (Dot(dir, CPnt3D::unit_nz) <= 0.0f && (m_control ? ((CSealCtrl *)m_control)->IsAi() : false))
            dir = *goal;
    }

    CQuat lo;
    CQuat hi;

    PartInSpace(m_spinelo, 0, 0, &lo);
    Negate(lo.vec.x);
    Negate(lo.vec.y);
    Negate(lo.vec.z);
    PartInSpace(m_spinehi, 0, 0, &hi);
    Negate(hi.vec.x);
    Negate(hi.vec.y);
    Negate(hi.vec.z);
    CPnt3D lofwd;
    CPnt3D hifwd;

    lo.Apply(&CPnt3D::unit_nz, &lofwd, 1);
    hi.Apply(&CPnt3D::unit_nz, &hifwd, 1);
    CQuat lorot = m_spinelo->m_rot;
    CQuat hirot = m_spinehi->m_rot;
    CPnt3D d;
    CPnt3D axis;
    CPnt3D turn;
    CQuat q;
    CQuat m;
    CQuat m2;

    lo.Apply(&dir, &d, 1);
    CrossVU0(lofwd, d, &axis);
    axis.Scale(0.1f * w, &turn);
    q.Exp(&turn);
    lorot.Mul(&q, &m);
    m_spinelo->m_rot = m;
    hi.Apply(&dir, &d, 1);
    CrossVU0(hifwd, d, &axis);
    axis.Scale(0.4f * w, &turn);
    q.Exp(&turn);
    hirot.Mul(&q, &m2);
    m_spinehi->m_rot = m2;
}

extern "C" {
/* the kill credited to its shooter (OVL2 0x547AA0, seal.cpp's) and the death's kind kept, the victim
 * struck down (OVL2 0x547A90, seal.cpp's: the word at 0xFC0, then 0x547860) */
void func_00547AA0(CZSealBody *seal, unsigned int id, bool b);
void func_00547A90(CZSealBody *seal, int kind);
}

CZSealBody *ftsGetPlayer();

/* a landing's damage by its speed: by the dynamics' three landing speeds none, light, hard or a fall
 * that hurts (none in states 8 and 10), the ground's landing sound -- and the bones' break on the two
 * hardest -- played at the SEAL; then, for this console's SEAL, the adrenaline raised by the middle
 * two, the six values cut by the share of the speed past the first, the health updated as a fall's
 * and a death this made the player's kill, its kind 253; the severity returned (OVL2 0x5AC1F0; SOCOM
 * 1's HandleFallingDamage, the damage scaled now, a sound for every severity, the kill credited) */
LANDING_TYPE CZSealBody::HandleFallingDamage(float speed)
{
    /* (the grade declared first, uninitialised: it takes s2, the alive flag s1) */
    LANDING_TYPE sev;
    bool alive = m_isAlive;
    CSnd *snd = MaterialEx(m_last_altitude.GetMaterial())->m_fallsound;
    float frac;

    if (m_state == 8 || m_state == 10)
        frac = 0.0f;
    else {
        float lo = theCharacterDynamics.m_landSpeed[0];
        float hi = theCharacterDynamics.m_landSpeed[2];

        if (speed <= lo)
            frac = 0.0f;
        else if (speed >= hi)
            frac = 1.0f;
        else
            frac = (speed - lo) / (hi - lo);
    }
    if (m_state == 8 || m_state == 10)
        sev = LANDING_NONE;
    else if (speed <= theCharacterDynamics.m_landSpeed[0])
        sev = LANDING_NONE;
    else if (speed <= theCharacterDynamics.m_landSpeed[1])
        sev = LANDING_SOFT;
    else if (speed < theCharacterDynamics.m_landSpeed[2])
        sev = LANDING_HARD;
    else
        sev = LANDING_HURT;
    switch (sev) {
    case LANDING_NONE:
        if (snd)
            snd->PlaySound3D(&m_posW, true, 1.0f, 0, 0, 255);
        break;
    case LANDING_SOFT:
        if (snd)
            snd->PlaySound3D(&m_posW, true, 1.0f, 0, 0, 255);
        break;
    case LANDING_HARD:
        if (data_0044D580)
            data_0044D580->PlaySound3D(&m_posW, true, 1.0f, 0, 0, 255);
        break;
    case LANDING_HURT:
        if (snd)
            snd->PlaySound3D(&m_posW, true, 1.0f, 0, 0, 255);
        if (data_0044D580)
            data_0044D580->PlaySound3D(&m_posW, true, 1.0f, 0, 0, 255);
        break;
    }
    if (IsRemote())
        return LANDING_NONE;
    switch (sev) {
    case LANDING_SOFT:
        AdrenalineIncr(0.33f);
        break;
    case LANDING_HARD:
        AdrenalineIncr(0.66f);
        break;
    case LANDING_NONE:
        break;
    case LANDING_HURT:
        break;
    }
    for (int i = 0; i < 6; i++) {
        m_ffc[i] -= m_1014[i] * frac;
        if (m_ffc[i] < 0.0f)
            m_ffc[i] = 0.0f;
    }
    UpdateHealth((DAMAGE_LOCATION)3, DEATH_FALLING);
    if (alive && !m_isAlive && frac > 0.0f) {
        func_00547AA0(this, ftsGetPlayer()->m_id, false);
        func_00547A90(this, 253);
    }
    return sev;
}

/* an explosion's damage: online, nothing for another console's SEAL; the projectile's bit 1
 * cleared; with no check running, one when it hurts here or is weapon 123 -- a delayed intersection
 * from the projectile's frame start to the head, the SEAL's own node ignored (OVL2 0x5AC070; SOCOM
 * 1's HandleDamageExplosive, called from the same place in HandleDamage, without its fling and
 * remade intersection) */
void CZSealBody::HandleDamageExplosive(CZProjectile *proj, CZBodyPart *part, DAMAGE_LOCATION *where)
{
    if (theNetwork.m_9 && IsRemote())
        return;
    proj->SetIsFirstHit(false);
    if (m_projectileintersecthandle == -1) {
        if (proj->GetDamage(&m_posW, 0.0f) > 0.0f || proj->m_weapon->m_ID == 123) {
            CPnt3D head;

            m_tempprojectile = proj;
            PartInSpace(m_head, 0, &head, 0);
            m_quat.Apply(&head, &head, 1);
            head = head + m_posW;
            m_projectileintersecthandle = zdb::CWorld::m_world->RegIntersectDelayed(m_projectiledi,
                &proj->m_framestartpos, &head, -1);
            /* (the world taken first: its load stays ahead of GetNode, in s0, as SOCOM 1's) */
            zdb::CWorld *world = zdb::CWorld::m_world;

            world->RegIgnoreNodeDelayed(m_projectiledi, GetNode());
        }
    }
}

extern "C" {
/* a location's damage taken from a projectile (OVL2 0x5A5A80, new in SOCOM II: SOCOM 1's DamageHead,
 * DamageBody and the limbs' in one, the location first), a damage and an armour pierce dealt to a
 * location (OVL2 0x5A5830), the pellets of a shot from a point that reach the SEAL (OVL2 0x5A1620)
 * and the blowback from a point (OVL2 0x57ED10, seal_body.cpp's) -- the third not written; the names
 * unknown */
bool func_005A5A80(CZSealBody *seal, DAMAGE_LOCATION where, CZProjectile *proj);
bool func_005A5830(CZSealBody *seal, DAMAGE_LOCATION where, float damage, float pierce);
int func_005A1620(CZSealBody *seal, const CPnt3D *from);
bool func_0057ED10(CZSealBody *seal, const CPnt3D *p, float a, float b);
}

/* one pellet's damage, at a location picked at random by the odds -- the body when none is picked
 * (an inline: 0x5A0E70 and HandleDamageNonExplosive share its tables, OVL2 data 0x6508D0 and
 * 0x6508E0; the name ours) */
static inline void PelletDamage(CZSealBody *seal, CZProjectile *proj, DAMAGE_LOCATION &where)
{
    static const DAMAGE_LOCATION locs[6] = {
        (DAMAGE_LOCATION)0, (DAMAGE_LOCATION)3, (DAMAGE_LOCATION)2,
        (DAMAGE_LOCATION)1, (DAMAGE_LOCATION)5, (DAMAGE_LOCATION)4
    };
    static const float odds[6] = { 0.3f, 0.6f, 0.7f, 0.8f, 0.9f, 1.0f };
    float damage = proj->GetDamage(&seal->m_posW, 0.0f);
    float pierce = proj->m_ammo->GetArmorPierce();
    float r = Rand01();

    where = (DAMAGE_LOCATION)3;
    for (int i = 0; i < 6; i++) {
        if (r < odds[i]) {
            where = locs[i];
            break;
        }
    }
    func_005A5830(seal, where, damage, pierce);
}

/* a bullet's damage on a part: online only a player's projectile counts; not twice from the last
 * grenade; a shotgun's (kind 81) pellets each at a random location -- online the body taken whole --
 * unless the last projectile was one too; else the part's location, handed back; then, offline or
 * for this console's SEAL, a shotgun's blowback from its shooter (OVL2 0x5ABBC0; SOCOM 1's
 * HandleDamageNonExplosive with its six Damage calls one, the shotgun and the blowback new) */
void CZSealBody::HandleDamageNonExplosive(CZProjectile *proj, CZBodyPart *part, DAMAGE_LOCATION *where)
{
    /* (the bit as a bool, compared: SOCOM 1's sltu too) */
    if (theNetwork.m_9 && (bool)proj->m_isplayer == false)
        return;
    EQUIP_ITEM type = proj->m_weapon->GetIDType();

    if (m_lastprojectile == proj && (type == 171 || type == 121))
        return;
    if (proj->m_weapon->GetIDType() == 81) {
        if (!m_lastprojectile || m_lastprojectile->m_weapon->GetIDType() != 81) {
            if (theNetwork.m_9)
                func_005A5A80(this, (DAMAGE_LOCATION)3, proj);
            else {
                DAMAGE_LOCATION loc = (DAMAGE_LOCATION)6;
                int n = func_005A1620(this, &proj->m_framestartpos);

                for (int i = 0; i < n; i++)
                    PelletDamage(this, proj, loc);
                if (loc != 6)
                    UpdateHealth(loc, DEATH_BULLET);
            }
            m_lastprojectile = proj;
        }
    } else if (part == m_head || part == m_neck) {
        func_005A5A80(this, (DAMAGE_LOCATION)0, proj);
        *where = (DAMAGE_LOCATION)0;
        m_lastprojectile = proj;
    } else if (part == m_lbicep || part == m_lforearm) {
        *where = (DAMAGE_LOCATION)2;
        func_005A5A80(this, (DAMAGE_LOCATION)2, proj);
    } else if (part == m_rbicep || part == m_rforearm) {
        *where = (DAMAGE_LOCATION)1;
        func_005A5A80(this, (DAMAGE_LOCATION)1, proj);
    } else if (part == m_spinehi || part == m_spinelo || part == m_hips) {
        *where = (DAMAGE_LOCATION)3;
        func_005A5A80(this, (DAMAGE_LOCATION)3, proj);
        m_lastprojectile = proj;
    } else if (part == m_lthigh || part == m_lcalf) {
        *where = (DAMAGE_LOCATION)5;
        func_005A5A80(this, (DAMAGE_LOCATION)5, proj);
    } else if (part == m_rthigh || part == m_rcalf) {
        *where = (DAMAGE_LOCATION)4;
        func_005A5A80(this, (DAMAGE_LOCATION)4, proj);
    }
    CZWeapon *weapon = proj->m_weapon;
    CZAmmo *ammo = proj->m_ammo;

    if (theNetwork.m_9 && IsRemote())
        return;
    if (!ammo)
        return;
    if (!weapon)
        return;
    if (weapon->GetIDType() != 81)
        return;
    float damage = proj->GetDamage(&m_posW, 0.0f);
    CEntity *e = CEntity::m_list.GetEntityById(proj->m_shooterID);
    CZSealBody *shooter = 0;

    if (e)
        shooter = AsSeal(e);
    if (!shooter)
        return;
    CPnt3D d = shooter->m_posW - m_posW;

    func_0057ED10(this, &shooter->m_posW, damage * ammo->GetBlowbackScale(d.x * d.x + d.y * d.y + d.z * d.z), 100.0f);
}

extern "C" {
/* (the kill's credit, 0x547AA0, takes whether the weapon was one of five: the id kept, then 0x547860
 * and 0x59EE20 -- ExecuteAction_CQBAttack inlines it) the five items (OVL1 0x3C5AC0,
 * zwep_ftsweapon.cpp's) */
bool func_003C5AC0(EQUIP_ITEM id);
}

/* a projectile's damage: none from no ammunition or from the explosive already taken (a detonated
 * one with damage is kept); the part the last weapon hit names takes it, explosive when the
 * ammunition has a radius and went off; the health updated -- online, only for a hit no SEAL of
 * another console scored -- and a death this made credited to the shooter, its kind the weapon's
 * (OVL2 0x5AB850; SOCOM 1's HandleDamage, the location now handed back by address, the projectile's
 * detonation asked as well, and the kill's credit through 0x547AA0) */
void CZSealBody::HandleDamage(CZProjectile *proj)
{
    bool alive = m_isAlive;

    if (!(proj && proj->m_ammo && proj != m_lastprojectile))
        return;
    if (proj->m_ammo->GetExplosionDamage() > 0.0f && proj->WasDetonated())
        m_lastprojectile = proj;
    std::vector<CZBodyPart *> parts(m_body_parts);
    zdb::IntersectStruct *hit = m_wephit.m_node ? &m_wephit : 0;
    DAMAGE_LOCATION where = (DAMAGE_LOCATION)0;
    bool searching = true;

    for (std::vector<CZBodyPart *>::iterator it = parts.begin(); searching && it != parts.end(); ++it) {
        CZBodyPart *part = *it;

        if (part && part->m_id >= 0 && part->m_node == hit->m_node) {
            if (proj->m_ammo->GetExplosionRadius() > 0.0f && proj->WasDetonated())
                HandleDamageExplosive(proj, part, &where);
            else
                HandleDamageNonExplosive(proj, part, &where);
            searching = false;
        }
    }
    if (theNetwork.m_9) {
        unsigned int id = proj->m_shooterID;
        CEntity *e = CEntity::m_list.GetEntityById(id);
        CZSealBody *shooter = 0;

        if (e)
            shooter = AsSeal(e);
        if (!id || (shooter && !shooter->IsRemote()))
            UpdateHealth(where, DEATH_BULLET);
    } else
        UpdateHealth(where, DEATH_BULLET);
    if (alive && !m_isAlive) {
        unsigned int id = proj->m_shooterID;
        CEntity *e = CEntity::m_list.GetEntityById(id);
        CZSealBody *killer = 0;

        if (e)
            killer = AsSeal(e);
        if (killer) {
            ((CSealCtrl *)killer->m_control)->NotifyTargetDown(this);
            bool b = false;

            if (func_003C5AC0(proj->m_weapon->m_ID))
                b = true;
            func_00547AA0(this, id, b);
        } else
            m_stats.Death(*this);
    }
}

extern "C" {
/* the blast's stun by the weapon (OVL2 0x5A5250, below); the blowback a blast's damage gives (OVL1
 * 0x3C5930, zwep_ftsweapon.cpp's) */
void func_005A5250(CZSealBody *seal, EQUIP_ITEM item, const CPnt3D *from, const float *scale);
float func_003C5930(float damage);
int func_005A18B0(CZSealBody *seal, CZProjectile *proj);
}

/* (the people's material, SOCOM 1's zwep::personMaterialId: OVL1 data 0x3E14F8) */
namespace zwep {
extern int personMaterialId;
}

static inline float LenSq(const CPnt3D &v)
{
    return v.x * v.x + v.y * v.y + v.z * v.z;
}

static inline bool NoNode(const zdb::CNode *p)
{
    return !(p != 0);
}

/* the explosive's sight from the head answered, once the delayed ray has run: the hits walked from the
 * blast outward past the SEAL's own nodes and whatever lets a blast through; something solid stops it unless
 * the shot has bit 9. The blast's stun by its reach, a player's ringing ears (not for the shotguns' 81-84),
 * the explosive's hits by the odds at random locations, the blowback, the health, a death credited
 * (OVL2 0x5A0E70: SOCOM 1's HandleGrenadeExplosion, which took its stuns one by one) */
void CZSealBody::HandleGrenadeExplosion()
{
    if (!m_tempprojectile)
        return;
    if (m_tempprojectile->m_state == 7) {
        m_tempprojectile = 0;
        m_projectileintersecthandle = -1;
        return;
    }

    bool alive = m_isAlive ? true : false;

    zdb::CWorld::m_world->ClearIgnoreNodeDelayed(m_projectiledi);

    bool done = false;
    int i;
    zdb::IntersectStruct *best = 0;
    bool blocked = false;
    int n = m_projectiledi->m_Cnt;

    do {
        CPnt3D hd;
        float bestd = 0.0f;
        float last;

        if (!best)
            last = bestd;
        else {
            CPnt3D d;

            best->m_pos.Sub(&m_projectiledi->m_Tail, &d);
            last = d.x * d.x + d.y * d.y + d.z * d.z;
        }
        best = 0;
        for (i = 0; i < n; i++) {
            bool mine;
            zdb::CNode *p;
            zdb::IntersectStruct *h = &m_projectiledi->m_Intersects[i];

            if (h->m_node) {
                p = h->m_node->m_parent;
                mine = false;
                while (!NoNode(p) && !mine) {
                    if (p == GetNode())
                        mine = true;
                    p = p->m_parent;
                }
                if (mine)
                    continue;
            }

            h->m_pos.Sub(&m_projectiledi->m_Tail, &hd);

            float dd = hd.x * hd.x + hd.y * hd.y + hd.z * hd.z;

            if (dd > last && (!best || dd < bestd)) {
                best = h;
                bestd = dd;
            }
        }
        if (best) {
            CMaterialEx *m = MaterialEx(best->GetMaterial());

            if (m && !(m->m_particle ? true : false) && !(m->m_liquid ? true : false) && !(m->m_pickup ? true : false) &&
                zwep::personMaterialId != best->GetMaterial()) {
                blocked = true;
                done = blocked;
            }
        } else
            done = true;
    } while (!done);
    if (!blocked || m_tempprojectile->GetFlag9()) {
        bool ears;
        int n;
        CZProjectile *proj;
        bool b;
        const CPnt3D *from;
        int k;
        CZAmmo *ammo;
        unsigned int shooter;
        EQUIP_ITEM id;

        from = &m_tempprojectile->m_framestartpos;
        id = m_tempprojectile->m_weapon->m_ID;
        ears = false;
        if (IsPlayer()) {
            switch (id) {
            case 84:
            case 83:
            case 82:
            case 81:
                break;
            default:
                ears = true;
                break;
            }
        }

        float scale = 1.0f;
        float radius = m_tempprojectile->m_ammo ? m_tempprojectile->m_ammo->GetExplosionRadius() : 0.0f;

        if (radius > 0.0f) {
            float r2 = radius * radius;
            CPnt3D d;

            d = m_posW;
            d.Sub(from, &d);

            float dd = LenSq(d);

            if (r2 > dd)
                scale = 1.0f - dd / r2;
            else
                scale = 0.0f;
        }
        if (scale > 0.0f) {
            func_005A5250(this, id, from, &scale);
            if (ears) {
                CSnd *snd = CSnd::GetSoundByName(".RINGING_EARS");

                if (snd)
                    snd->PlaySound(true, 0, 255);
                CSnd::SetGroupVolTemp(5.0f, 0.35f, 0, 2, 1, 3, 5, 6, -1);
            }

            DAMAGE_LOCATION where;

            n = func_005A18B0(this, m_tempprojectile);
            for (k = 0; k < n; k++)
                PelletDamage(this, m_tempprojectile, where);
            if (n > 0 && !(theNetwork.m_9 && IsRemote())) {
                proj = m_tempprojectile;

                float damage = proj->GetDamage(&m_posW, 0.0f);

                if (damage > 0.0f) {
                    ammo = proj->m_ammo;

                    float reach;

                    if (ammo) {
                        ammo->GetExplosionDamage();
                        reach = ammo->GetExplosionRadius();
                    } else
                        reach = 30.0f;
                    func_0057ED10(this, &proj->m_framestartpos, func_003C5930(damage), reach);
                }
            }
            UpdateHealth(where, (SEAL_DEATH_TYPE)4);

            shooter = m_tempprojectile->m_shooterID;

            CEntity *e = CEntity::m_list.GetEntityById(shooter);
            CZSealBody *killer = 0;

            if (e)
                killer = AsSeal(e);
            if (alive && !m_isAlive) {
                if (killer)
                    ((CSealCtrl *)killer->m_control)->NotifyTargetDown(this);

                b = false;
                if (func_003C5AC0(m_tempprojectile->m_weapon->m_ID))
                    b = true;
                func_00547AA0(this, shooter, b);
                func_00547A90(this, id);
            }
        }
    }
    m_tempprojectile = 0;
    m_projectileintersecthandle = -1;
    if (!IsRemote())
        func_0057E770(this);
}

/* a remote hit taken: the stats told, a first head shot counted; a shotgun's pellets each at a random
 * location by its own ammunition, the location's health then updated; else the location's damage; for this
 * SEAL's own a shotgun's blowback; the health updated -- a death's kill credited, its kind 255 when it lives
 * (OVL2 0x5A1B80: SOCOM 1's RemoteWeaponDamageVictim, the shotgun new, its Damage calls one) */
/* (CZWeapon::GetIDType by its mangled name, through a byte: the argument's read of the spilled parameter
 * is an lbu then -- the enum's a lb, and a cast in the call a common subexpression of its own in a slot of
 * its own; a device, the original's parameter not told apart from the DWARF's unsigned char) */
extern "C" EQUIP_ITEM GetIDType__8CZWeaponF10EQUIP_ITEM(unsigned char id);
static inline EQUIP_ITEM GetIDTypeU(unsigned char id) { return GetIDType__8CZWeaponF10EQUIP_ITEM(id); }
void CZSealBody::RemoteWeaponDamageVictim(DAMAGE_LOCATION where, float damage, float pierce, CZSealBody *victim,
                                          CZSealBody *shooter, unsigned char weapon, bool hit)
{
    EQUIP_ITEM type = GetIDTypeU(weapon);

    if (!shooter || !victim)
        return;

    CSealStats *vs = &victim->m_stats;
    CSealStats *ss = &shooter->m_stats;
    CZAmmo *ammo = 0;

    if (type == 81)
        ammo = CZWeapon::m_listWeapons.GetWeapon((EQUIP_ITEM)weapon)->GetDefaultAmmo();
    if (hit) {
        ss->Hit(*shooter, *victim);
        vs->WasHit(*victim);
    }

    bool alive = m_isAlive ? true : false;
    bool grenade = false;
    bool shotgun = false;

    switch (type) {
    case 171:
    case 121:
        grenade = true;
        break;
    case 81:
        shotgun = true;
        break;
    }
    if (where == 0 && victim->m_isAlive && !grenade && !shotgun) {
        vs->m_count[1]++;
        func_005A5830(this, where, damage, pierce);
        ss->HeadShot(*shooter);
    } else if (shotgun) {
        if (damage > 0.0f) {
            static const DAMAGE_LOCATION locs[6] = {
                (DAMAGE_LOCATION)0, (DAMAGE_LOCATION)3, (DAMAGE_LOCATION)2,
                (DAMAGE_LOCATION)1, (DAMAGE_LOCATION)5, (DAMAGE_LOCATION)4
            };
            static const float odds[6] = { 0.3f, 0.6f, 0.7f, 0.8f, 0.9f, 1.0f };
            DAMAGE_LOCATION loc = (DAMAGE_LOCATION)6;
            int n = func_005A1620(this, &shooter->m_posW);

            for (int i = 0; i < n; i++) {
                float d = ammo->GetImpactDamage();
                float a = ammo->GetArmorPierce();
                float r = Rand01();

                loc = (DAMAGE_LOCATION)3;
                for (int k = 0; k < 6; k++) {
                    if (r < odds[k]) {
                        loc = locs[k];
                        break;
                    }
                }
                func_005A5830(this, loc, d, a);
            }
            if (loc != 6)
                UpdateHealth(loc, DEATH_BULLET);
        }
    } else
        func_005A5830(this, where, damage, pierce);
    if (victim == this && type == 81) {
        CPnt3D d = shooter->m_posW - m_posW;

        func_0057ED10(this, &shooter->m_posW, damage * ammo->GetBlowbackScale(d.x * d.x + d.y * d.y + d.z * d.z), 100.0f);
    }
    if (victim == this) {
        UpdateHealth(where, DEATH_BULLET);
        if (alive && !m_isAlive) {
            func_00547A90(this, weapon);
            if (!IsRemote()) {
                bool b = false;

                if (func_003C5AC0((EQUIP_ITEM)weapon))
                    b = true;
                func_00547AA0(this, shooter->m_id, b);
            }
        }
        if (m_isAlive) {
            func_00547A90(this, -1);
            func_00547AA0(this, 255, false);
        }
        func_0057E770(this);
    } else
        UpdateHealth(where, DEATH_BULLET);
}

/* the SEAL the aim's assist takes: among the targets flagged 0x200 the nearest to the screen's centre
 * (its height halved) of the living, awake enemies the AI drives -- not bit 17's -- within the assist's
 * circle, wider inside 85, past 40 and short of the fog's last fifth, not behind a point given or out
 * of the weapon's range; its target's place handed back, no assist through a scope (OVL2 0x5AB1C0, new in
 * SOCOM II: CheckDIShoot asks it; the name unknown) */
static inline CZSealBody *TargetSeal(CEntity *e)
{
    return e && e->m_type == CEntity::ENT_TYPE_SEAL ? (CZSealBody *)e : 0;
}

static inline bool TargetIsAi(CZSealBody *seal)
{
    return seal->m_control ? ((CSealCtrl *)seal->m_control)->IsAi() : false;
}

static inline bool TargetAwake(const CSealCtrlAi *ai)
{
    return !(ai->m_aibit9 || ai->m_aibit8);
}

static inline zdb::CCamera *WorldCamera()
{
    return zdb::CWorld::m_world->m_camera;
}

extern "C" CZSealBody *func_005AB1C0(CZSealBody *seal, CPnt3D *out, CPnt3D *from)
{
    CZSealBody *best = 0;

    if (seal->m_kit.IsUsingScope())
        return 0;

    float bestd = 100000000.0f;
    CTargetList &targets = seal->m_targets;

    for (CTargetList::iterator i = targets.begin(); i != targets.end(); ++i) {
        CZSealBody *s = TargetSeal(i->m_ent);

        if (!s || !(i->m_flags & 0x200) || !TargetIsAi(s) || !s->m_isAlive)
            continue;
        if ((s->m_team & seal->m_team) || (s->m_team & 0x20000))
            continue;
        if (!TargetAwake(s->GetSealCtrlAi()))
            continue;

        CPnt3D tp;
        CPnt3D sp;

        s->GetTargetPosW(&tp);
        bool on = WorldCamera()->GetScreenPos(&tp, &sp);

        if (!on)
            continue;
        sp.y *= 0.5f;

        float d2 = sp.x * sp.x + sp.y * sp.y;

        if (!(d2 < 10000.0f))
            continue;

        float z = WorldCamera()->GetZ(&tp);
        zdb::CCamera *cam = WorldCamera();
        float limit = cam->m_fog_far;

        limit -= 0.2f * (limit - cam->m_fog_near);
        if (z <= 40.0f || !(z < limit))
            continue;

        float r2 = 900.0f;

        if (z < 85.0f)
            r2 += 9100.0f * (0.022222223f * (85.0f - z));
        if (!(d2 < r2))
            continue;

        bool blocked;

        if (from)
            blocked = cam->GetZ(from) < z;
        else {
            CZFTSWeapon *w = seal->m_kit.GetCurItem();
            float range = 100000000.0f;

            if (w)
                range = ((CZWeapon *)w)->GetMaxRange();
            blocked = range < z;
        }
        if (blocked || !(d2 < bestd))
            continue;
        bestd = d2;
        best = s;
        *out = tp;
    }
    return best;
}

/* two rays' hits at once, through the unit's two intersection sets (di0, di1) run as one batch on the
 * world from the entity's node: for each ray the first hit that is not volumetric, not liquid, and
 * not fully penetrable unless a pickup -- each later one is weighed against a nearest-so-far that is
 * never kept (0.0, in the build: so the first wins) -- every hit's distance taken from the FIRST ray's
 * tail (OVL2 0x5AB590, seal_env.cpp's range between HandleDamage and SOCOM 1's GetCharacterHit; new in
 * SOCOM II, its one caller 0x54D620; the name unknown) */
extern "C" void func_005AB590(CEntity *ent, zdb::IntersectStruct **hit0, const CPnt3D *tail0,
                              const CPnt3D *tip0, zdb::IntersectStruct **hit1, const CPnt3D *tail1,
                              const CPnt3D *tip1)
{
    zdb::DiIntersect *dis[2];

    dis[0] = &di0;
    dis[1] = &di1;
    di0.m_Type = 2;
    di0.m_Tail = *tail0;
    di0.m_Tip = *tip0;
    di1.m_Type = 2;
    di1.m_Tail = *tail1;
    di1.m_Tip = *tip1;
    zdb::CNode *node = ent->GetNode();

    di0.m_Node = node;
    di1.m_Node = node;
    zdb::CWorld::m_world->GetIntersectBatch(dis, 2);
    for (int r = 0; r < 2; r++) {
        zdb::DiIntersect *di = dis[r];
        bool found = false;
        float best = 0.0f;
        int n = di->m_Cnt;

        for (int i = 0; i < n; i++) {
            zdb::IntersectStruct *is = &di->m_Intersects[i];
            CPnt3D v;

            is->m_pos.Sub(tail0, &v);
            float d = v.x * v.x + v.y * v.y + v.z * v.z;
            CMaterialEx *mat = MaterialEx(is->GetMaterial());

            if (mat) {
                if (mat->IsVolumetric() || mat->IsLiquid())
                    continue;
                if (mat->m_penetration == 1.0f && !mat->IsPickup())
                    continue;
            }
            if (!found || d < best) {
                found = true;
                if (r == 0)
                    *hit0 = is;
                else
                    *hit1 = is;
            }
        }
    }
}

/* a shotgun shot's pellet ranges, squared: near offline, near online, far online (OVL2 data, 8 bytes
 * apart: a static pool, read with lwc1) */
static float s_pelletNear = 2500.0f;
static float s_pelletNearOnline = 6400.0f;
static float s_pelletFarOnline = 22500.0f;

/* a random value between two (an inline: the 1.0 range through its parameters keeps the multiply) */
static inline float RandBetween(float lo, float hi)
{
    return lo + (hi - lo) * Rand01();
}

/* how many of a projectile's pellets reach the SEAL, offline: 8 within 30, and 1-7 at random by the
 * same spread; for any weapon but the shotgun (81) one fewer crouched and three fewer prone, none
 * below zero; past 30 the count falls with the square of the distance, its fraction kept at random
 * (OVL2 0x5A18B0, new in SOCOM II beside 0x5A1620, written after it: 0x5A0E70 calls it; the name
 * unknown) */
extern "C" int func_005A18B0(CZSealBody *seal, CZProjectile *proj)
{
    CPnt3D d = seal->m_posW;

    *(PNT3D *)&d = d - proj->m_framestartpos;
    int n = 0;
    float dist = d.x * d.x + d.y * d.y + d.z * d.z;

    if (dist < 900.0f)
        n += 8;
    float r = Rand01();

    if (r < 0.05f)
        n += 1;
    else if (r < 0.15f)
        n += 2;
    else if (r < 0.5f)
        n += 4;
    else if (r < 0.85f)
        n += 5;
    else if (r < 0.95f)
        n += 6;
    else if (r <= 1.0f)
        n += 7;
    if (proj->m_weapon->GetIDType() != 81) {
        switch (seal->GetStance(false)) {
        case stateProne:
            n -= 3;
            break;
        case stateCrouch:
            n -= 1;
            break;
        }
    }
    if (n < 0)
        n = 0;
    if (n > 0 && !(dist <= 900.0f)) {
        float f = 900.0f * (float)n / dist;

        n = (int)f;
        f -= (float)n;
        if (RandBetween(0.0f, 1.0f) <= f)
            n++;
    }
    return n;
}

/* how many of a shotgun shot's pellets from a point reach the SEAL: 8 within the near range (online's
 * wider), 4 more online within the far, and 1-7 at random by a fixed spread; past the near range the
 * count falls with the square of the distance, its fraction kept at random (OVL2 0x5A1620, new in SOCOM
 * II: RemoteWeaponDamageVictim and 0x5ABBC0 call it; the name unknown).
 * Byte-exact (the SEALs, round 41; 0x294 against 0x284 since round 40) by three things: the operator's
 * return slot assigned back over the position's copy THROUGH ITS PNT3D BASE, `*(PNT3D *)&d = d -
 * *from` -- a C struct's assignment, three loads then three stores at fixed offsets, where CPnt3D's
 * own (and `(PNT3D &)d`, an inline `*p = *p - b`, member stores) go through two address registers and
 * every initialised spelling elides the copy (0x274); the network flag read into an unsigned char (a
 * bool local normalises it, sltu); the ratio assigned back over `dist` -- a reused variable is a web
 * numbered after the named locals, so the count's conversion still colours first (f0) with the
 * multiply's operands in the target's order (a ratio local, an inline divide or `n * (range / dist)`
 * swap f0/f1, `s * (float)n` the operands) */
extern "C" int func_005A1620(CZSealBody *seal, const CPnt3D *from)
{
    CPnt3D d = seal->m_posW;

    *(PNT3D *)&d = d - *from;
    int n = 0;
    float dist = d.x * d.x + d.y * d.y + d.z * d.z;
    unsigned char online = theNetwork.m_9;
    float range = online ? s_pelletNearOnline : s_pelletNear;

    if (dist < range)
        n += 8;
    else if (online && dist < s_pelletFarOnline)
        n += 4;
    float r = Rand01();

    if (r < 0.05f)
        n += 1;
    else if (r < 0.15f)
        n += 2;
    else if (r < 0.5f)
        n += 4;
    else if (r < 0.85f)
        n += 5;
    else if (r < 0.95f)
        n += 6;
    else if (r <= 1.0f)
        n += 7;
    if (n > 0 && !(dist <= range)) {
        dist = range / dist;
        float f = (float)n * dist;

        n = (int)f;
        f -= (float)n;
        if (RandBetween(0.0f, 1.0f) <= f)
            n++;
    }
    return n;
}

extern "C" {
/* (OVL2 0x5A60D0, below: a stance change, a kind and a flag) */
void func_005A60D0(CZSealBody *seal, unsigned char item, unsigned char mode);
}

/* a callback made, and made and cleared once (inlines, the names ours: the call's own test, through a
 * const pointer, reloads the function the caller's test read) */
static inline void CallBack(const ANIM_CALLBACK *cb, void *body)
{
    if (cb->m_func)
        cb->m_func(body, cb->m_userdata);
}

static inline void CallOnce(ANIM_CALLBACK &cb, void *body)
{
    if (cb.m_func) {
        CallBack(&cb, body);
        cb.Clear();
    }
}

extern "C" void func_0057D5E0(CZSealBody *seal, SEAL_ITEM_BYTE item);
extern AnimSet *data_0044D408;

extern "C" {
/* the swap's anims' first (OVL1 data 0x3DEC68: AnimTypes', read as a variable) and the changes to and from
 * a launcher (OVL2 0x5A6B70, 0x5A6D30, below) */
extern short data_003DEC68;
void func_005A60D0(CZSealBody *seal, unsigned char item, unsigned char mode);
}

/* the aim and fire points found again (an inline: three times here; the name ours) */
static inline void FindFirepoints(CZSealBody *seal)
{
    seal->m_curr_aimpt = seal->GetFirepointNode("aimpoint");
    seal->m_curr_firept = seal->GetFirepointNode("firepoint");
}

/* an item in hand changed, from one to another, or at once the kit's parts put right: a launcher's change
 * apart; a change neither end of which is a hand's, or one already in the hand, the parts saved and eased;
 * otherwise the swap's anim by the stance -- stood or crouched and moving the walking swap -- or, with no
 * anim to play, the change made at once (OVL2 0x5A64C0, new in SOCOM II: CZSealBody::Tick and CZKit::PreTick
 * call it; the name unknown) */
extern "C" bool func_005A64C0(CZSealBody *seal, SEAL_ITEM_BYTE from, SEAL_ITEM_BYTE to, bool now)
{
    func_00555170(&seal->m_blends);
    if (now) {
        if (seal->m_item == 1)
            seal->m_kit.ReParentToRifle();
        else if (seal->m_item == 2)
            seal->m_kit.ReParentToPistol();
        else if (seal->m_rifle->m_parent == seal->m_rhand)
            seal->m_kit.ReParentToRifle();
        else if (seal->m_pistol->m_parent == seal->m_rhand)
            seal->m_kit.ReParentToPistol();
        FindFirepoints(seal);
        return false;
    }

    bool fromHand = from == ITEM_RIFLE || from == ITEM_PISTOL;
    bool toHand = to == ITEM_RIFLE || to == ITEM_PISTOL;

    if (from == 4) {
        if (to == 4)
            return true;
        bool r = func_005A6B70(seal, to);

        return r;
    }
    if (to == 4) {
        if (!fromHand) {
            if (seal->m_rifle->m_parent == seal->m_rhand)
                from = ITEM_RIFLE;
            else if (seal->m_pistol->m_parent == seal->m_rhand)
                from = ITEM_PISTOL;
            else
                from = ITEM_RIFLE;
        }
        bool r = func_005A6D30(seal, from);

        return r;
    }

    bool same = false;

    if (fromHand && !toHand)
        same = true;
    if (!fromHand && !toHand)
        same = true;
    if (!fromHand) {
        if (to == 1) {
            if (seal->m_rifle->m_parent == seal->m_rhand) {
                seal->m_item = 1;
                same = true;
            } else
                from = ITEM_PISTOL;
        } else if (to == 2) {
            if (seal->m_pistol->m_parent == seal->m_rhand) {
                seal->m_item = 2;
                same = true;
            } else
                from = ITEM_RIFLE;
        }
    }
    if (same) {
        seal->SaveParts();
        seal->m_interp1 = 0.3f;
        seal->m_interp2 = 0.3f;
        return true;
    }

    bool moving = seal->m_velM.x * seal->m_velM.x + seal->m_velM.y * seal->m_velM.y + seal->m_velM.z * seal->m_velM.z > 400.0f;

    switch (to) {
    case 1:
        if (from != 1 && from != 3) {
            switch (seal->m_state) {
            case 0:
                if (moving)
                    func_00588BC0(seal, (AnimType)data_003DEC68, 0, 0, 0);
                else {
                    seal->ChangeToPistolMode(false);
                    func_00588BC0(seal, (AnimType)53, 0, 1, 0);
                }
                break;
            case 1:
                if (moving)
                    func_00588BC0(seal, (AnimType)data_003DEC68, 1, 0, 0);
                else {
                    seal->ChangeToPistolMode(false);
                    func_00588BC0(seal, (AnimType)54, 1, 1, 0);
                }
                break;
            case 2:
                seal->ChangeToPistolMode(false);
                func_00588BC0(seal, (AnimType)55, 2, 1, 0);
                break;
            default:
                seal->ChangeToRifleMode(true);
                func_005A60D0(seal, 1, 0);
                break;
            }
        } else {
            seal->ChangeToRifleMode(true);
            func_005A60D0(seal, 1, 0);
        }
        break;
    case 2:
        if (from != 2 && from != 3) {
            switch (seal->m_state) {
            case 0:
                if (moving)
                    func_00588BC0(seal, (AnimType)data_003DEC68, 0, 0, 0);
                else {
                    seal->ChangeToPistolMode(false);
                    func_0057D5E0(seal, (SEAL_ITEM_BYTE)2);
                    func_00588BC0(seal, (AnimType)53, 0, 0, 0);
                }
                break;
            case 1:
                if (moving)
                    func_00588BC0(seal, (AnimType)data_003DEC68, 1, 0, 0);
                else {
                    seal->ChangeToPistolMode(false);
                    func_0057D5E0(seal, (SEAL_ITEM_BYTE)2);
                    func_00588BC0(seal, (AnimType)54, 1, 0, 0);
                }
                break;
            case 2:
                seal->ChangeToPistolMode(false);
                func_0057D5E0(seal, (SEAL_ITEM_BYTE)2);
                func_00588BC0(seal, (AnimType)55, 2, 0, 0);
                break;
            default:
                seal->ChangeToPistolMode(true);
                func_0057D5E0(seal, (SEAL_ITEM_BYTE)2);
                func_005A60D0(seal, 2, 0);
                break;
            }
        } else {
            seal->ChangeToPistolMode(true);
            func_005A60D0(seal, 2, 0);
        }
        break;
    default:
        if (from == 1) {
            seal->ChangeToRifleMode(true);
            func_005A60D0(seal, 1, 0);
        } else if (from == 2) {
            seal->ChangeToPistolMode(true);
            func_0057D5E0(seal, (SEAL_ITEM_BYTE)2);
            func_005A60D0(seal, 2, 0);
        }
        seal->m_item = to;
        break;
    }
    FindFirepoints(seal);
    return true;
}

/* the launcher's part back on the spine and set as gear (an inline, four times in 0x5A60D0, once in
 * PickupItem; the name ours) */
static inline void StowLauncher(CZSealBody *seal)
{
    CZBodyPart *p = seal->m_item_gear[4].m_part;

    if (p && p->m_node) {
        p->m_parent->m_node->DeleteChild(p->m_node);
        p->m_parent = seal->m_spinehi;
        p->m_parent->m_node->AddChild(p->m_node);
        p->SetActive(false);
    }
    seal->SetPartAsGear(seal->m_item_gear[4].m_part, seal->m_item_gear[4].m_gear);
}

/* a weapon change made at once: to a launcher (mode 4) out of the launcher's anim set, the item readied
 * -- the given, else the last -- and the launcher stowed; otherwise, to the pistol, the rifle stowed on the
 * lower spine as gear; to the rifle, its mode set, the item readied and the pistol as gear; for 4 both as
 * gear -- then the aim and fire points found again (OVL2 0x5A60D0, new in SOCOM II; the name unknown) */
extern "C" void func_005A60D0(CZSealBody *seal, unsigned char item, unsigned char mode)
{
    if (mode == 4) {
        if (seal->m_animset == data_0044D408)
            seal->BindAnimSet(seal->m_default_animset, false);
        seal->m_desiredState = (SEAL_STATE)0;
        if (item == 2) {
            func_0057D5E0(seal, (SEAL_ITEM_BYTE)2);
            StowLauncher(seal);
        } else if (item == 1) {
            func_0057D5E0(seal, (SEAL_ITEM_BYTE)1);
            StowLauncher(seal);
        } else if (seal->m_lastitem == 2) {
            func_0057D5E0(seal, (SEAL_ITEM_BYTE)2);
            StowLauncher(seal);
        } else if (seal->m_lastitem == 1) {
            func_0057D5E0(seal, (SEAL_ITEM_BYTE)1);
            StowLauncher(seal);
        }
    } else if (item == 2) {
        CZBodyPart *rifle = seal->m_rifle;

        if (rifle->m_node) {
            rifle->m_parent->m_node->DeleteChild(rifle->m_node);
            seal->m_rifle->m_parent = seal->m_spinelo;
            seal->m_rifle->m_parent->m_node->AddChild(seal->m_rifle->m_node);
            seal->m_rifle->SetActive(false);
        }
        seal->SetPartAsGear(seal->m_item_gear[1].m_part, seal->m_item_gear[1].m_gear);
    } else if (item == 1) {
        seal->ChangeToRifleMode(false);
        func_0057D5E0(seal, (SEAL_ITEM_BYTE)1);
        seal->SetPartAsGear(seal->m_item_gear[2].m_part, seal->m_item_gear[2].m_gear);
    } else if (item == 4) {
        seal->SetPartAsGear(seal->m_item_gear[2].m_part, seal->m_item_gear[2].m_gear);
        seal->SetPartAsGear(seal->m_item_gear[1].m_part, seal->m_item_gear[1].m_gear);
    }
    seal->m_curr_aimpt = seal->GetFirepointNode("aimpoint");
    seal->m_curr_firept = seal->GetFirepointNode("firepoint");
}

/* a weapon change cut short: with none of the change's blends on top, the change's overlay -- the
 * rifle to the pistol, or either way of 0x3DF0F0 / 0x3DF0F8 -- has its callback made and the stance
 * change it was heading for (0x5A60D0) made at once, then goes back to the pool; with one on top its
 * callback is made and the change made by the blend's flag 0x20 (OVL2 0x5A5DA0, new in SOCOM II:
 * before a death and a weapon's drop; the name unknown). The blend's path first, ended by a `return`
 * (its last if/else's join keeps its own branch out), the overlay's block ended by one too (the
 * target's branch to the very next instruction); the second top fetch through &m_blends (the same
 * address spelt `seal + 0x1c0` twice is computed once, in a third saved register) */
extern "C" void func_005A5DA0(CZSealBody *seal)
{
    if (seal->IsCurrentBlend((AnimType)53) || seal->IsCurrentBlend((AnimType)54) ||
        seal->IsCurrentBlend((AnimType)55) || seal->IsCurrentBlend((AnimType)data_003DF0E0) ||
        seal->IsCurrentBlend((AnimType)data_003DF0E8)) {
        CZBodyAnimBlend *top = (CZBodyAnimBlend *)func_00555170((char *)seal + 0x1c0);

        CallOnce(top->m_callback, seal);
        bool flag = func_005551A0(((CZBodyAnimBlend *)func_00555170(&seal->m_blends))->m_flags, 0x20);

        if (seal->IsCurrentBlend((AnimType)data_003DF0E0) && flag) {
            func_005A60D0(seal, 1, 4);
            return;
        }
        if (seal->IsCurrentBlend((AnimType)data_003DF0E8) && flag) {
            func_005A60D0(seal, 2, 4);
            return;
        }
        if (flag)
            func_005A60D0(seal, 1, 0);
        else
            func_005A60D0(seal, 2, 0);
        return;
    }
    {
        CBlendOverlay &ov = seal->m_overlay;

        if (ov.m_anim && ov.m_anim->m_anim) {
            short id = ov.m_anim->m_id;

            if (id == AnimTypes::MovingRifle2Pistol) {
                CallOnce(ov.m_callback, seal);
                if (ov.m_state == -1)
                    func_005A60D0(seal, 1, 0);
                else
                    func_005A60D0(seal, 2, 0);
            } else if (id == data_003DF0F0) {
                CallOnce(ov.m_callback, seal);
                if (ov.m_state == -1)
                    func_005A60D0(seal, 1, 4);
            } else if (id == data_003DF0F8) {
                CallOnce(ov.m_callback, seal);
                if (ov.m_state == -1)
                    func_005A60D0(seal, 2, 4);
            }
            seal->m_active_anim_pool.PutBack(ov.m_anim);
            ov.Clear();
            return;
        }
    }
}

/* a projectile's first-hit bit (an inline bool accessor on a const pointer: its test reloads the byte
 * for the bit's store, as the target's -- tested in place the load is shared) */
static inline bool FirstHit(const CZProjectile *p)
{
    return p->m_isfirsthit;
}

/* a location's damage taken from a projectile, whether it took (OVL2 0x5A5A80, new in SOCOM II: SOCOM
 * 1's DamageHead, DamageBody and the limbs' in one, the location first; the name unknown). Online
 * the shooter's console decides: a shot from a SEAL another console drives, or from no SEAL, is not
 * taken here; else the hit reported, the first-hit bit cleared and -- unless this SEAL is remote --
 * the damage taken. Offline a first hit is counted for both, the damage taken and, when it killed
 * the SEAL, the kill credited to the shooter (whether the weapon was one of the five). The report's
 * arguments are locals read before its two calls (the weapon's id first -- `lb`, passed on raw), the
 * report and its two returns wrapped in the shooter's test with the `return false` after it, and the
 * offline result declared first (igreplay: it numbers just after the parameters) */
extern "C" bool func_005A5A80(CZSealBody *seal, DAMAGE_LOCATION where, CZProjectile *proj)
{
    bool took;
    bool alive = seal->m_isAlive;

    if (theNetwork.m_9) {
        int id = proj->m_shooterID;
        CEntity *e = CEntity::m_list.GetEntityById(id);
        CZSealBody *shooter = 0;

        if (e)
            shooter = AsSeal(e);
        if (!id || (shooter && !shooter->IsRemote())) {
            EQUIP_ITEM w = proj->m_weapon->m_ID;
            int v = seal->m_id;
            int s = proj->m_shooterID;

            seal->ReportWeaponHitDamage(proj, s, v, where, proj->GetDamage(&seal->m_posW, 0.0f),
                                        proj->m_ammo->GetArmorPierce(), proj->m_isfirsthit, w);
            if (FirstHit(proj))
                proj->SetIsFirstHit(false);
            if (!seal->IsRemote())
                return func_005A5830(seal, where, proj->GetDamage(&seal->m_posW, 0.0f), proj->m_ammo->GetArmorPierce());
            return false;
        }
        return false;
    }
    int id = proj->m_shooterID;
    CEntity *e = CEntity::m_list.GetEntityById(id);
    CZSealBody *shooter = 0;

    if (e)
        shooter = AsSeal(e);
    if (FirstHit(proj)) {
        proj->SetIsFirstHit(false);
        if (shooter)
            shooter->m_stats.Hit(*shooter, *seal);
        seal->m_stats.WasHit(*seal);
    }
    took = func_005A5830(seal, where, proj->GetDamage(&seal->m_posW, 0.0f), proj->m_ammo->GetArmorPierce());

    if (alive && !seal->m_isAlive) {
        bool five = false;

        if (func_003C5AC0(proj->m_weapon->m_ID))
            five = true;
        func_00547AA0(seal, id, five);
    }
    return took;
}

/* the share of a hit an empty location passes on to the body (OVL2 data, 8 bytes apart: a static
 * pool, read with lwc1) */
static float s_passedShare = 0.3f;
static float s_passedPierce = 10.0f;

/* damage at a location, 0-5: taken against the location's value while it lasts -- the body's (3)
 * scaling the four limbs' down to its own share of full -- an empty one but the head's passing 0.3 of
 * it to the body at a pierce of 10; whether a value took it; every value but the totals kept from
 * going below 0 (OVL2 0x5A5830, new in SOCOM II: RemoteWeaponDamageVictim and 0x5A1620's users call
 * it; the name unknown). The body's case tests the value through the location, as the others do (the
 * target indexes it, `m_ffc[3]` is a fixed offset), and takes it at the fixed one */
extern "C" bool func_005A5830(CZSealBody *seal, DAMAGE_LOCATION loc, float damage, float pierce)
{
    bool took = false;

    if (!(damage <= 0.0f)) {
        switch (loc) {
        case 3:
            if (!(seal->m_ffc[loc] <= 0.0f)) {
                took = true;
                CZProjectile::ResolveDamage(&seal->m_ffc[3], &seal->m_102c[3], damage, pierce);
                float share = seal->m_ffc[3] / seal->m_1014[3];

                seal->m_ffc[2] = MIN(seal->m_ffc[2], seal->m_1014[2] * share);
                seal->m_ffc[1] = MIN(seal->m_ffc[1], seal->m_1014[1] * share);
                seal->m_ffc[5] = MIN(seal->m_ffc[5], seal->m_1014[5] * share);
                seal->m_ffc[4] = MIN(seal->m_ffc[4], seal->m_1014[4] * share);
            }
            break;
        case 0:
        case 1:
        case 2:
        case 4:
        case 5:
            if (!(seal->m_ffc[loc] <= 0.0f)) {
                CZProjectile::ResolveDamage(&seal->m_ffc[loc], &seal->m_102c[loc], damage, pierce);
                took = true;
            } else if (loc != 0)
                took = func_005A5830(seal, (DAMAGE_LOCATION)3, damage * s_passedShare, s_passedPierce);
            break;
        }
    }
    if (seal->m_ffc[2] < 0.0f)
        seal->m_ffc[2] = 0.0f;
    if (seal->m_ffc[1] < 0.0f)
        seal->m_ffc[1] = 0.0f;
    if (seal->m_ffc[5] < 0.0f)
        seal->m_ffc[5] = 0.0f;
    if (seal->m_ffc[4] < 0.0f)
        seal->m_ffc[4] = 0.0f;
    if (seal->m_ffc[3] < 0.0f)
        seal->m_ffc[3] = 0.0f;
    if (seal->m_ffc[0] < 0.0f)
        seal->m_ffc[0] = 0.0f;
    return took;
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

extern "C" {
/* the look anims: four sources (OVL2 data: each a table of rotations), what each is built into -- the
 * turns between them (SOCOM 1's dQlookhead and its three fellows) -- and the axes ApplyLookAnim reads
 * (SOCOM 1's Rlookhead and its fellows; OVL1 bss), the head's, then up the spine, and their builder
 * (OVL2 0x5AE980, below; the name unknown) */
extern CPnt3D data_00650820[];
extern CPnt3D data_006507B0[];
extern CPnt3D data_00650740[];
extern CPnt3D data_006506D0[];
extern CQuat data_0044C7A0[];
extern CQuat data_0044C860[];
extern CQuat data_0044C920[];
extern CQuat data_0044C9E0[];
extern CPnt3D data_0044CAA0[];
extern CPnt3D data_0044CB30[];
extern CPnt3D data_0044CBC0[];
extern CPnt3D data_0044CC50[];
void func_005AE980(const CPnt3D *rot, CQuat *dq, CPnt3D *axis);
/* the eyes' limits: up, down and sideways (OVL1 data; SOCOM 1's were constants, 8, -14 and 17
 * degrees) */
extern float data_0044C2D0;
extern float data_0044C2D4;
extern float data_0044C2D8;
}

/* the turn from one look rotation to another, and the axis the unit -z swings about under it (SOCOM 1's
 * ComputeDeltaLookQuat__FR6CPnt3DR6CPnt3DP5CQuatP6CPnt3D, out of line there; inlined in SOCOM II) */
static inline void ComputeDeltaLookQuat(const CPnt3D &from, const CPnt3D &to, CQuat *dq, CPnt3D *axis)
{
    CQuat qa;
    CQuat qb;

    QFromRot(&qa, &from);
    QFromRot(&qb, &to);
    Negate(qa.vec.x);
    Negate(qa.vec.y);
    Negate(qa.vec.z);
    qa.Mul(&qb, dq);
    CPnt3D z = CPnt3D::unit_nz;
    CPnt3D r;

    dq->Apply(&z, &r, 1);
    CrossVU0(z, r, axis);
}

/* one look anim's twelve turns between its rotations and their axes (OVL2 0x5AE980, new in SOCOM II:
 * InitLookAnims calls it for each of its four tables, SOCOM 1's InitLookAnims calling
 * ComputeDeltaLookQuat 48 times in the same order; the name unknown) */
extern "C" void func_005AE980(const CPnt3D *rot, CQuat *dq, CPnt3D *axis)
{
    ComputeDeltaLookQuat(rot[1], rot[2], &dq[0], &axis[0]);
    ComputeDeltaLookQuat(rot[5], rot[2], &dq[1], &axis[1]);
    ComputeDeltaLookQuat(rot[4], rot[5], &dq[2], &axis[2]);
    ComputeDeltaLookQuat(rot[4], rot[1], &dq[3], &axis[3]);
    ComputeDeltaLookQuat(rot[1], rot[0], &dq[4], &axis[4]);
    ComputeDeltaLookQuat(rot[3], rot[0], &dq[5], &axis[5]);
    ComputeDeltaLookQuat(rot[4], rot[3], &dq[6], &axis[6]);
    ComputeDeltaLookQuat(rot[7], rot[8], &dq[7], &axis[7]);
    ComputeDeltaLookQuat(rot[5], rot[8], &dq[8], &axis[8]);
    ComputeDeltaLookQuat(rot[4], rot[7], &dq[9], &axis[9]);
    ComputeDeltaLookQuat(rot[7], rot[6], &dq[10], &axis[10]);
    ComputeDeltaLookQuat(rot[3], rot[6], &dq[11], &axis[11]);
}

/* a look anim's turn for a yaw and a pitch: half of each angle's size about the table's axis for its
 * sign -- the yaw's 2 or 6, the pitch's 3 or 9 -- as quaternions, the pitch's left out when it is
 * close enough to none (OVL2 0x5AE730; SOCOM 1's GetLookQuat__FP6CPnt3DffP5CQuat, the flag held as
 * the call's byte where SOCOM 1 converted it) */
void GetLookQuat(CPnt3D *table, float yaw, float pitch, CQuat *q)
{
    float ayaw = fabsf(yaw);
    float apitch = fabsf(pitch);
    bool flat = _close_enough(pitch, 0.0f, 0.01f);
    CQuat qy, qp;
    CPnt3D v;

    if (yaw >= 0.0f) {
        if (pitch >= 0.0f) {
            table[2].Scale(0.5f * ayaw, &v);
            qy.Exp(&v);
            if (!flat) {
                table[3].Scale(0.5f * apitch, &v);
                qp.Exp(&v);
            }
        } else {
            table[2].Scale(0.5f * ayaw, &v);
            qy.Exp(&v);
            if (!flat) {
                table[9].Scale(0.5f * apitch, &v);
                qp.Exp(&v);
            }
        }
    } else if (pitch >= 0.0f) {
        table[6].Scale(0.5f * ayaw, &v);
        qy.Exp(&v);
        if (!flat) {
            table[3].Scale(0.5f * apitch, &v);
            qp.Exp(&v);
        }
    } else {
        table[6].Scale(0.5f * ayaw, &v);
        qy.Exp(&v);
        if (!flat) {
            table[9].Scale(0.5f * apitch, &v);
            qp.Exp(&v);
        }
    }
    if (flat)
        *q = qy;
    else
        qy.Mul(&qp, q);
}

/* the look anims built, the head's and up the spine (OVL2 0x5AE6A0; SOCOM 1's) */
void CZSealBody::InitLookAnims()
{
    func_005AE980(data_00650820, data_0044C7A0, data_0044CAA0);
    func_005AE980(data_006507B0, data_0044C860, data_0044CB30);
    func_005AE980(data_00650740, data_0044C920, data_0044CBC0);
    func_005AE980(data_006506D0, data_0044C9E0, data_0044CC50);
}

/* the head turned toward a direction when it may turn (OVL2 0x5AE640; SOCOM 1's, the look held) */
bool CZSealBody::SetLookDir(const CPnt3D *dir, float rate)
{
    if (CanHeadLook())
        return m_look.SetLookDir(this, dir, rate);
    return false;
}

/* the eyes turned by a pitch and a yaw, when an eye is drawn: the pitch's limit up or down, the yaw's
 * sideways, the goal kept within the limits off the current direction (OVL2 0x5AE4B0; SOCOM 1's, the
 * constants made globals and the yaw's two equal arms one; SOCOM 1's took the direction third and never
 * read it, SOCOM II's has none -- its one caller, CLook::SetLookDir, sets no a1) */
bool CZSealBody::SetEyeLookDir(float pitch, float yaw, float rate)
{
    if (m_reyeball && m_leyeball &&
        ((m_reyeball->m_node && m_reyeball->m_node->Rendered(2)) ||
         (m_leyeball->m_node && m_leyeball->m_node->Rendered(2)))) {
        float up = data_0044C2D0;
        float down = data_0044C2D4;
        float side = data_0044C2D8;
        /* (SOCOM 1 passed it twice, to SetDir as the start too) */
        const CPnt3D *cur = &m_eyelook.m_curr_dir;
        CPnt3D look;
        CPnt3D goal;
        CQuat q;
        CQuat qx;
        CQuat qy;

        if (pitch >= 0.0f)
            qx.MakeXRot(up * pitch);
        else
            qx.MakeXRot(down * pitch);
        float left = -side;

        qy.MakeYRot(left * yaw);
        qx.Mul(&qy, &q);
        q.Apply(&CPnt3D::unit_nz, &look, 1);
        CalcGoalDir(cur, &look, &goal, up, down, left, side);
        m_eyelook.SetDir(&goal, 1.5f * rate);
        return true;
    }
    return false;
}

/* the look made: the sequence cleared, looking straight ahead (down -z), done (OVL2 0x5AE450; SOCOM
 * 1's, CZSealBody's constructor calls it for its two looks) */
CLook::CLook()
{
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

/* a direction's yaw and pitch (each an arc cosine, kept to 0 at or past 1 and to pi at or past -1,
 * signed by the direction's x and y), the yaw taken round the long way when the direction faces +z
 * (its dot with unit_z over 0.8) and the current direction lies on the other side; within the limits
 * the direction is the goal (true), else the angles clipped to the limits' box are turned into the
 * goal, a YX rotation of -z (false) (OVL2 0x5AE040; SOCOM 1's, the same body: SOCOM II's guards the
 * arc cosine and takes sqrt.s for sqrtf). Byte-exact (round 55, unwritten before): the guard an inline
 * returning its arms (written out in place, each arm stores the angle; as one ternary 8 bytes long), the
 * length tested as a bool (`if (len)`: `c.eq.s len, 0` -- `len != 0.0f` and `0.0f != len` both put the
 * zero first), each limit an if / else-if (`a > hi || a < lo` lays the false arm after the second test) */
static inline float SafeAcos(float c)
{
    if (c >= 1.0f)
        return 0.0f;
    if (c <= -1.0f)
        return 3.1415927f;
    return acosf(c);
}

bool CZSealBody::CalcGoalDir(const CPnt3D *cur, const CPnt3D *dir, CPnt3D *goal, float up, float down,
                             float left, float right)
{
    float len = sqrtf(dir->x * dir->x + dir->z * dir->z);
    float c;
    CPnt3D ang;

    if (len)
        c = -dir->z / len;
    else
        c = 1.0f;
    ang.x = SafeAcos(c);
    if (dir->x < 0.0f)
        ang.x = -ang.x;
    ang.y = SafeAcos(len);
    if (dir->y < 0.0f)
        ang.y = -ang.y;
    ang.z = 0.0f;
    if (Dot(*dir, CPnt3D::unit_z) > 0.8f) {
        if (cur->x > 0.0f && ang.x < 0.0f)
            ang.x += 6.2831855f;
        else if (cur->x < 0.0f && ang.x > 0.0f)
            ang.x -= 6.2831855f;
    }
    bool ok = true;

    if (ang.y > up)
        ok = false;
    else if (ang.y < down)
        ok = false;
    if (ang.x > right)
        ok = false;
    else if (ang.x < left)
        ok = false;
    if (ok) {
        *goal = *dir;
        return true;
    }
    CPnt3D center = CPnt3D::zero;
    CPnt3D clipped;
    CQuat q;

    ClipToBox(center, ang, clipped, up, down, right, left);
    q.MakeYXRot(-clipped.x, clipped.y);
    q.Apply(&CPnt3D::unit_nz, goal, 1);
    return false;
}

extern "C" {
/* the head's limits: up, down and sideways (OVL1 data 0x44C2C4-0x44C2CC), and the reach under which a
 * look is taken whole -- the ground-plane length looking ahead or behind, and the cosine off -z (OVL1
 * data 0x44C2FC-0x44C304; SOCOM 1's were constants) */
extern float data_0044C2C4;
extern float data_0044C2C8;
extern float data_0044C2CC;
extern float data_0044C2FC;
extern float data_0044C300;
extern float data_0044C304;
}

/* the head turned toward a direction (none: straight ahead, taken whole), nothing to do when it is the
 * goal already: a direction far enough off the ground plane's ahead is taken whole, any other goes
 * through CalcGoalDir within the head's limits from the current direction; the look set toward it at
 * the rate and the eyes given its pitch and yaw, each as a fraction of its limit; whether it was taken
 * whole (OVL2 0x5ADC70; SOCOM 1's, the constants made globals and the arc cosines guarded). Byte-exact
 * (round 55, unwritten before): the reach's three globals read into locals ahead of the length (the
 * target loads them before the sqrt, and the length takes the lowest register), the goal copy-constructed
 * inside its if (all three loads first, stored through CalcGoalDir's fourth argument), the pitch and yaw
 * declared bare after the head's limits (assigned where they are computed, the pitch then takes $f20
 * below the limits' $f23-$f21; declared at their assignments it takes $f24, above the rate); SetEyeLookDir
 * given no direction -- the target sets no a1 */
bool CLook::SetLookDir(CZSealBody *seal, const CPnt3D *dir, float rate)
{
    bool r = false;

    if (!dir) {
        r = true;
        dir = &CPnt3D::unit_nz;
    }
    if (*dir == m_goal_dir)
        return true;
    if (!r) {
        bool ok = true;
        float ahead = data_0044C2FC;
        float behind = data_0044C300;
        float cmin = data_0044C304;
        float len = sqrtf(dir->x * dir->x + dir->z * dir->z);
        float c;

        if (len)
            c = -dir->z / len;
        else
            c = 1.0f;
        if (dir->z >= 0.0f) {
            if (len < ahead)
                ok = false;
        } else if (len < behind)
            ok = false;
        if (c < cmin)
            ok = false;
        if (ok)
            r = true;
    }
    float up = data_0044C2C4;
    float down = data_0044C2C8;
    float side = data_0044C2CC;
    float pitch;
    float yaw;

    if (!r) {
        CPnt3D goal = m_curr_dir;

        r = seal->CalcGoalDir(&m_curr_dir, dir, &goal, up, down, -side, side);
        dir = &goal;
    }
    SetDir(dir, rate);
    float len = sqrtf(dir->x * dir->x + dir->z * dir->z);
    float c;

    if (len)
        c = -dir->z / len;
    else
        c = 1.0f;
    pitch = SafeAcos(len);
    yaw = SafeAcos(c);

    if (dir->y < 0.0f)
        pitch = -pitch;
    if (dir->x < 0.0f)
        yaw = -yaw;
    float p;

    if (pitch >= 0.0f)
        p = pitch / up;
    else
        p = -pitch / down;
    seal->SetEyeLookDir(p, yaw / side, rate);
    return r;
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

/* the look aimed at a direction over a time: unless it is the goal already, the start's quaternion
 * taken from straight ahead to where the look points and the time kept; none means done at once,
 * else the turn's angle (the arc of the two directions' dot product, held to [0, pi]) over the time
 * is the step, the direction the goal and the turn's axis (the cross product through VU0, as
 * CPnt3D::Cross) scaled to half the angle -- straight up when the two all but agree (OVL2 0x5AD9D0;
 * SOCOM 1's SetDir without its second quaternion and its start direction: SOCOM II's look turns
 * from where it points). Out of line: Blink, BlinkTick, SetLookDir and SetEyeLookDir call it */
#pragma auto_inline off
void CLook::SetDir(const CPnt3D *dir, float rate)
{
    if (*dir == m_goal_dir)
        return;
    m_quat0.Align(CPnt3D::unit_nz, m_curr_dir);
    m_rate = rate;
    if (rate == 0.0f) {
        m_fracT = 1.0f;
        m_done = 1;
        return;
    }
    m_fracT = 0.0f;
    m_done = 0;
    float d = dir->x * m_curr_dir.x + dir->y * m_curr_dir.y + dir->z * m_curr_dir.z;

    float angle;

    if (d >= 1.0f)
        angle = 0.0f;
    else if (d <= -1.0f)
        angle = 3.1415927f;
    else
        angle = acosf(d);
    m_deltaT = angle / m_rate;
    m_goal_dir = *dir;
    sceVu0FVECTOR r;
    sceVu0FVECTOR va;
    sceVu0FVECTOR vb;

    va[0] = m_curr_dir.x;
    va[1] = m_curr_dir.y;
    va[2] = m_curr_dir.z;
    va[3] = 1.0f;
    vb[0] = dir->x;
    vb[1] = dir->y;
    vb[2] = dir->z;
    vb[3] = 1.0f;
    sceVu0OuterProduct(r, va, vb);
    m_cross.x = r[0];
    m_cross.y = r[1];
    m_cross.z = r[2];
    float len = m_cross.Length();

    if (len < 0.001f) {
        m_cross = CPnt3D::unit_y;
        m_cross.Scale(0.5f * angle, &m_cross);
    } else
        m_cross.Scale(0.5f * angle / len, &m_cross);
}
#pragma auto_inline reset

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
        m_cross.Scale(quadW(m_fracT), &v);
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

/* the head and the three parts above it each turned by its look anim's quaternion for the yaw and
 * the pitch (OVL2 0x5AD5B0; SOCOM 1's, the tables OVL1's) */
void CZSealBody::ApplyLookAnim(float yaw, float pitch)
{
    CZBodyPart *part = m_head;
    CQuat q;
    CQuat r;

    GetLookQuat(data_0044CAA0, yaw, pitch, &q);
    part->m_rot.Mul(&q, &r);
    part->m_rot = r;
    part = part->m_parent;
    GetLookQuat(data_0044CB30, yaw, pitch, &q);
    part->m_rot.Mul(&q, &r);
    part->m_rot = r;
    part = part->m_parent;
    GetLookQuat(data_0044CBC0, yaw, pitch, &q);
    part->m_rot.Mul(&q, &r);
    part->m_rot = r;
    part = part->m_parent;
    GetLookQuat(data_0044CC50, yaw, pitch, &q);
    part->m_rot.Mul(&q, &r);
    part->m_rot = r;
}

/* the head's look's direction (OVL2 0x5AD5A0; SOCOM 1's, the look held) */
const CPnt3D *CZSealBody::GetLookDir() const
{
    return &m_look.m_curr_dir;
}

/* a part's rotation set (an inline taking the quaternion by reference) */
static inline void SetPartRot(CZBodyPart *part, const CQuat &q)
{
    part->m_rot = q;
}

/* the eyeballs turned to the eyes' look (OVL2 0x5AD4D0; SOCOM 1's, the look held) */
void CZSealBody::ApplyEyeLookAnim()
{
    if (m_reyeball && m_leyeball) {
        CQuat q;

        q.Align(CPnt3D::unit_nz, m_eyelook.m_curr_dir);
        SetPartRot(m_reyeball, q);
        SetPartRot(m_leyeball, q);
        m_reyeball->m_node->SetRotation(&m_reyeball->m_rot);
        m_leyeball->m_node->SetRotation(&m_leyeball->m_rot);
    }
}

/* the eyelids turned to the blink (OVL2 0x5AD400; SOCOM 1's, the blink held) */
void CZSealBody::ApplyBlinkAnim()
{
    if (m_reyelid && m_leyelid) {
        CQuat q;

        q.Align(CPnt3D::unit_nz, m_blink.m_curr_dir);
        SetPartRot(m_reyelid, q);
        SetPartRot(m_leyelid, q);
        m_reyelid->m_node->SetRotation(&m_reyelid->m_rot);
        m_leyelid->m_node->SetRotation(&m_leyelid->m_rot);
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

/* a pickup's acknowledgement, one byte (fts_remote.cpp's AckUnion: bit 0 a weapon taken), and its
 * message (OVL1 0x2BA1F0, fts_remote.cpp's) */
union AckUnion {
    struct {
        unsigned char m_weapon : 1;
        unsigned char m_rest : 7;
    } bits;
    /* an ammunition pickup's answer: a bit per entry of its stash (CheckForAmmoPickups; the names
     * ours) */
    struct {
        unsigned char m_0 : 1;
        unsigned char m_1 : 1;
        unsigned char m_2 : 1;
        unsigned char m_3 : 1;
        unsigned char m_4 : 1;
        unsigned char m_5 : 1;
        unsigned char m_6 : 1;
        unsigned char m_7 : 1;
    } ammo;
    unsigned char b;
};

/* an ammunition pickup's contents: the weapons' stash (zwep_weapon.cpp's CZItemStash, three parallel
 * lists -- a weapon, an ammunition and a count per entry -- out of line in OVL1: the constructor
 * 0x3CFF80, Add 0x3CFEF0, Count 0x3CFCF0, Get 0x3CFD70, Clear, SetAnim 0x3CFB10; only what the
 * pickups call) and the pickup's view of it (zseal.h's CPickupAmmo; the name ours). The members
 * typed as the weapons' round 23 read them: a delete runs the implicit destructor, which calls the
 * three members' destructors out of line, last member first (pickup_main.cpp's copies, 0x5428C0,
 * 0x542850, 0x5427E0) */
class CZItemStash {
public:
    CZItemStash();
    int Count();
    void Add(CZWeapon *weapon, CZAmmo *ammo, int rounds);
    bool Get(int i, int *a, int *b, int *c);
    void Clear(int i);
    void SetAnim(CZAnim *anim);

    std::vector<CZWeapon *> m_weapons;   // 0x0
    std::vector<CZAmmo *> m_ammo;   // 0xc
    std::vector<int> m_rounds;   // 0x18
    CZAnim *m_anim;   // 0x24
};

class CPickupAmmo : public CZItemStash {
};

/* the answer's ammunition bit for an entry set (an inline: the chain's tests run 0 to 7, each bit
 * stored as `(on & 1) << i`, the & shared by all; the name ours) */
static inline void SetAckAmmo(AckUnion *ack, int i, bool on)
{
    if (i == 0)
        ack->ammo.m_0 = on;
    else if (i == 1)
        ack->ammo.m_1 = on;
    else if (i == 2)
        ack->ammo.m_2 = on;
    else if (i == 3)
        ack->ammo.m_3 = on;
    else if (i == 4)
        ack->ammo.m_4 = on;
    else if (i == 5)
        ack->ammo.m_5 = on;
    else if (i == 6)
        ack->ammo.m_6 = on;
    else if (i == 7)
        ack->ammo.m_7 = on;
}

extern "C" {
/* an ammunition pickup walked with a label (OVL2 0x541910, pickup_main.cpp's) */
void func_00541910(CPickup *p, const char *label);
}

/* an entry of an ammunition pickup taken: its weapon's rounds into the kit, the entry cleared, the
 * pickup walked (an inline: each of HandlePickupAck's eight answers runs it, every copy with its
 * own three words; the name ours) */
static inline void TakeAmmoEntry(CZSealBody *seal, CPickup *p, int i)
{
    CPickupAmmo *ammo = p->GetAmmo();
    int w = 0;
    int a = 0;
    int n = 0;

    ammo->Get(i, &w, &a, &n);
    seal->m_kit.PickupAmmo((CZFTSWeapon *)w, (CZAmmo *)a, &n);
    ammo->Clear(i);
    func_00541910(p, "pickup");
}

/* each entry of an ammunition pickup with rounds left matched against the items asked for -- an
 * item a weapon in its high byte and an ammunition in its low one -- and the answer's bit for the
 * entry set (OVL2 0x59E580; SOCOM 1's CheckForAmmoPickups, the entries the stash's now) */
void CZSealBody::CheckForAmmoPickups(int count, CPickupAmmo *ammo, unsigned short *items, AckUnion *ack)
{
    for (int i = 0; i < ammo->Count(); i++) {
        int n = 0;
        int w = 0;
        int a = 0;

        ammo->Get(i, &w, &a, &n);
        for (int j = 0; j < count; j++) {
            unsigned short weapon = items[j] >> 8;
            unsigned short round = (unsigned short)(items[j] << 8) >> 8;
            EQUIP_ITEM wid = w ? ((CZWeapon *)w)->m_ID : (EQUIP_ITEM)255;
            EQUIP_AMMO aid = a ? ((CZAmmo *)a)->m_ID : (EQUIP_AMMO)255;

            if (round == aid && weapon == wid && n > 0)
                SetAckAmmo(ack, i, true);
        }
    }
}

/* a console's answer to this SEAL's pickup request: for each bit set, while the count lasts, a weapon
 * pickup (the first bit only) taken or the ammunition entry of the bit taken into the kit; the kit's
 * request finished and the ammunition pickup cleaned up (OVL2 0x59DFB0; SOCOM 1's HandlePickupAck,
 * its eight blocks counting the answers now, the entries the stash's) */
void CZSealBody::HandlePickupAck(int count, int id, AckUnion *ack)
{
    CPickup *p = CPickup::FindPickup(id);

    if (!p)
        return;
    int n = 0;

    if (ack->ammo.m_0 && n < count) {
        switch (p->m_type) {
        case 8:
            PickupItem(p);
            break;
        case 9:
            TakeAmmoEntry(this, p, 0);
            break;
        }
        n++;
    }
    if (ack->ammo.m_1 && n < count) {
        switch (p->m_type) {
        case 8:
            break;
        case 9:
            TakeAmmoEntry(this, p, 1);
            break;
        }
        n++;
    }
    if (ack->ammo.m_2 && n < count) {
        switch (p->m_type) {
        case 8:
            break;
        case 9:
            TakeAmmoEntry(this, p, 2);
            break;
        }
        n++;
    }
    if (ack->ammo.m_3 && n < count) {
        switch (p->m_type) {
        case 8:
            break;
        case 9:
            TakeAmmoEntry(this, p, 3);
            break;
        }
        n++;
    }
    if (ack->ammo.m_4 && n < count) {
        switch (p->m_type) {
        case 8:
            break;
        case 9:
            TakeAmmoEntry(this, p, 4);
            break;
        }
        n++;
    }
    if (ack->ammo.m_5 && n < count) {
        switch (p->m_type) {
        case 8:
            break;
        case 9:
            TakeAmmoEntry(this, p, 5);
            break;
        }
        n++;
    }
    if (ack->ammo.m_6 && n < count) {
        switch (p->m_type) {
        case 8:
            break;
        case 9:
            TakeAmmoEntry(this, p, 6);
            break;
        }
        n++;
    }
    if (ack->ammo.m_7 && n < count) {
        switch (p->m_type) {
        case 8:
            break;
        case 9:
            TakeAmmoEntry(this, p, 7);
            break;
        }
        n++;
    }
    m_kit.PendingPickupHandled();
    CPickup::CleanupAmmoPickup(p);
}

/* (called through its map name with an int first parameter: this caller zero-extends the count at
 * both calls, as it would for an int -- passed to an unsigned short parameter it would go as it is) */
extern "C" void SendPickupAckMessage__Fiii8AckUnion(int who, int id, int obj, AckUnion ack);

/* a console's request for a pickup answered: none found, the pickups dumped and an empty answer
 * sent; else for each of the count, an ammunition pickup's rounds taken by 0x59E580, a weapon's
 * matched against the first item (bit 0 set); the answer sent, and with the asker an entity, its SEAL
 * handed the answer and the ammunition pickup cleaned up (OVL2 0x59E830; SOCOM 1's CanPickupNetItem,
 * after CheckForAmmoPickups in both builds -- SOCOM II finds the pickup by its own id and passes the
 * asker whole) */
void CZSealBody::CanPickupNetItem(unsigned short count, int id, unsigned short *items, int client)
{
    AckUnion ack;
    CPickup *p = CPickup::FindPickup(id);

    ack.b = 0;
    if (!p) {
        CPickup::DebugPickups();
        SendPickupAckMessage__Fiii8AckUnion(count, id, client, ack);
        return;
    }
    for (int i = 0; i < count; i++) {
        if (p->m_type == 9)
            CheckForAmmoPickups(count, p->GetAmmo(), items, &ack);
        else if (p->m_type == 8) {
            /* (the weapon taken first: compared in place, MW loads the item ahead of the call) */
            CZWeapon *w = p->GetWeapon();

            if (*items == w->m_ID)
                ack.bits.m_weapon = true;
        }
    }
    SendPickupAckMessage__Fiii8AckUnion(count, id, client, ack);
    CEntity *e = CEntity::m_list.GetEntityById(client);

    if (e) {
        CZSealBody *seal = AsSeal(e);

        if (e) {
            seal->HandlePickupAck(count, id, &ack);
            CPickup::CleanupAmmoPickup(p);
        }
    }
}

extern "C" {
/* the stats' events (seal.cpp's: a hostage killed, a suicide, a team-mate killed -- the kill is
 * CSealStats::Kill -- and two tallies by a count -- OVL2 0x545390, 0x545C90, 0x545C10, 0x5451A0, 0x5451C0),
 * the network game's side test (OVL1 0x2C30B0, unowned), whether a time of the AI's lies within a
 * span of now (OVL2 0x564CF0, seal_ai.cpp's) and the animation set the stats skip (OVL1 bss
 * 0x44D400, seal.cpp's) */
unsigned short func_00545390(CSealStats *s, const CZSealBody *seal);
void func_00545C90(CSealStats *s, const CZSealBody *seal);
void func_00545C10(CSealStats *s, const CZSealBody *seal);
void func_005451A0(CSealStats *s, int n);
void func_005451C0(CSealStats *s, int n);
int func_002C30B0(const CZSealBody *seal);
bool func_00564CF0(CSealCtrlAi *ai, float t);
extern AnimSet *data_0044D400;
}

/* a kill credited: the killer kept and, a SEAL, given a hostage's kill (online the side's valve
 * stepped), a suicide, a team-mate's or -- unless the victim's animation set is the skipped one --
 * an enemy's, a head shot offline and, against an AI, a kill of note: within ten seconds of its
 * mark when the flag is set, under four otherwise; the victim's death counted (OVL2 0x59EE20; SOCOM
 * 1's HandleKiller, the flag and the AI's tallies new) */
void CZSealBody::HandleKiller(int killer, bool b)
{
    if (m_fc4 == killer || killer == 255)
        return;
    CEntity *e = CEntity::m_list.GetEntityById(killer);

    m_fc4 = killer;
    CZSealBody *k = 0;

    if (e)
        k = AsSeal(e);
    bool hostage = (m_team & 0x20000) != 0;

    if (k) {
        if (hostage) {
            CNetGameState *g = data_00437CE8;

            if (g && theNetwork.m_9) {
                CValve *v = 0;

                if (func_002C30B0(k))
                    v = g->m_3c;
                else if (func_002C31D0(k))
                    v = g->m_38;
                if (v)
                    v->SetValue(v->GetValue() + 1);   /* (CValve's const accessor: the step re-reads the field for SetValue's test) */
            }
            func_00545390(&k->m_stats, k);
        } else if (this == k)
            func_00545C90(&m_stats, this);
        else if (k->m_team & m_team)
            func_00545C10(&k->m_stats, k);
        else if (m_animset != data_0044D400) {
            if (k && !(k->m_team & m_team))
                k->m_stats.Kill(*k);
            if (!theNetwork.m_9 && m_deathType == DEATH_HEADSHOT)
                k->m_stats.HeadShot(*k);
            CSealCtrlAi *ai = ((CSealCtrl *)m_control)->GetSealCtrlAi();

            if (ai) {
                if (b) {
                    if (!func_00564CF0(ai, 10.0f)) {
                        func_005451A0(&k->m_stats, 1);
                        func_005451C0(&k->m_stats, 3);
                    }
                } else if (ai->m_5c4 < 4)
                    func_005451C0(&k->m_stats, 1);
            }
        }
    }
    m_stats.Death(*this);
}

/* another console's dropped weapon made here, with the pickups open, unless this SEAL is a player:
 * the weapon's model instanced at the point and turned, made dynamic and a character node with its
 * textures and polygons, then a network pickup of kind 8 of that id -- a pistol's secondary -- and
 * the drop anim run on its node; with no pickup the node let go (OVL2 0x59EC60; SOCOM 1's
 * CreateNetWeaponPickup, the item and the id handed in now -- WeaponPUPacketHandler's call -- where
 * SOCOM 1's took the item out of the SEAL's own kit) */
void CZSealBody::CreateNetWeaponPickup(unsigned short item, int netid, CPnt3D *pos, CQuat *rot)
{
    if (!CPickup::m_isOpen)
        return;
    if (IsPlayer())
        return;
    CZWeapon *w = CZWeapon::m_listWeapons.GetWeapon((EQUIP_ITEM)item);

    if (!w)
        return;
    zdb::CNode *node = zdb::CNode::CreateInstance(
        (zdb::CModel *)zdb::CWorld::GetModel(w->GetModelName()), pos, rot);

    if (!node)
        return;
    node->SetDynamic(true, true);
    node->m_socom2_a1_bits.bit2 = 1;
    node->m_bumpmap = 1;
    zdb::CWorld::m_world->AddTextureAssetCharacter(zdb::CHARSETCOMMON, node);
    node->SetDescendantDI(true);
    CPickup *p = CPickup::NetworkCreate(netid, (CPickup::PU_TYPE)8, node, w);

    if (p) {
        p->m_secondary = CZKit::GetItemFireClass(w->m_ID) == CZKit::FIRE_CLASS_PISTOL;
        if (data_0044D508)
            ((CZAnim *)data_0044D508)->ActivateArg(1, 3, p->m_node, 0);
    } else {
        zdb::CWorld::m_world->DeleteTextureAssetCharacter(zdb::CHARSETCOMMON, node);
        zdb::CNode::Delete(node);
    }
}

/* another console's dropped ammunition made here, with the pickups open: each of the count's items
 * unpacked into its weapon and ammunition, the weapon's item -- or else the ammunition with its
 * rounds -- let go from this SEAL's kit, and the stash given the weapon, the ammunition and the
 * rounds; an ammunition box instanced at the point, made dynamic and a character node with its
 * textures, then a network pickup of kind 9 of that id holding the stash, the drop anim run on its
 * node and kept by the stash; with no node the stash deleted (OVL2 0x59E9E0; SOCOM 1's
 * CreateNetAmmoPickup -- AmmoPUPacketHandler's call) */
void CZSealBody::CreateNetAmmoPickup(unsigned short count, unsigned short *items, unsigned short *rounds,
                                     int id, CPnt3D *pos)
{
    if (!CPickup::m_isOpen)
        return;
    CZItemStash *stash = new CZItemStash;

    for (int i = 0; i < count; i++) {
        EQUIP_ITEM item;
        EQUIP_AMMO ammo;

        CZWeapon::UnpackItemAndAmmoEnums(items[i], &item, &ammo);
        if (item != EQUIP_NONE) {
            int k = m_kit.GetItemIndex(item);

            if (k != -1)
                m_kit.RemoveItemAndAmmo(k);
        } else {
            int k = m_kit.GetAmmoIndex(ammo, rounds[i]);

            if (k != -1)
                m_kit.RemoveAmmo(k);
        }
        CZWeapon *w = CZWeapon::m_listWeapons.GetWeapon(item);
        CZAmmo *a = CZAmmo::m_ammolist.GetAmmo(ammo);

        stash->Add(w, a, rounds[i]);
    }
    CQuat q;

    q.MakeYXZRot(0.0f, 0.0f, 0.0f);
    zdb::CNode *node =
        zdb::CNode::CreateInstance((zdb::CModel *)zdb::CWorld::GetModel("fiftycal_box"), pos, &q);

    if (!node) {
        delete stash;
        return;
    }
    node->SetDynamic(true, true);
    node->m_socom2_a1_bits.bit2 = 1;
    node->m_bumpmap = 1;
    zdb::CWorld::m_world->AddTextureAssetCharacter(zdb::CHARSETCOMMON, node);
    CPickup *p = CPickup::NetworkCreate(id, CPickup::PU_AMMO, node, stash);

    if (data_0044D508)
        stash->SetAnim(((CZAnim *)data_0044D508)->ActivateArg(1, 3, p->m_node, 0));
}

extern "C" {
/* the MP bomb asked for while the pickups are shut, and where (OVL1 bss 0x44CDF8 and 0x44CDE8: SOCOM 1's
 * pair at the same place, a $gp byte and a point; the names ours) */
extern bool data_0044CDF8;
extern CPnt3D data_0044CDE8;
}

/* the MP bomb's pickup made at a point: with the pickups shut only asked for there; else the satchel's
 * model instanced, named MPFootBomb, put in the world, made dynamic and a character node with its
 * textures, polygons and visuals; unless told not to, a pickup of kind 11 the player owns, and the
 * player's bomb marker made (BOMB_GROUND, type 1) or moved there, its base marker dropped; the node kept
 * as the bomb's (OVL2 0x59DAB0; SOCOM 1's, the owner, the bump-map bit and the marker's move new) */
CPickup *CZSealBody::CreateMPBombPickup(const CPnt3D *pos, bool b)
{
    if (!CPickup::m_isOpen) {
        data_0044CDF8 = true;
        data_0044CDE8 = *pos;
        return 0;
    }
    CQuat q;

    q.MakeYXZRot(0.0f, 0.0f, 0.0f);
    CZWeapon::m_listWeapons.GetWeapon((EQUIP_ITEM)154);
    zdb::CNode *node = zdb::CNode::CreateInstance((zdb::CModel *)zdb::CWorld::GetModel("Satchel"), pos, &q);

    if (!node)
        return 0;
    node->SetName("MPFootBomb");
    zdb::CWorld::m_world->AddChild(node);
    node->SetDynamic(true, true);
    node->m_socom2_a1_bits.bit2 = 1;
    node->m_bumpmap = 1;
    zdb::CWorld::m_world->AddTextureAssetCharacter(zdb::CHARSETCOMMON, node);
    node->SetDescendantDI(true);
    node->SetDescendantVisuals(true);
    CPickup *p = 0;

    if (b == false) {   /* (`!b` tests the register as it came; the target narrows it first) */
        CZSealBody *player = ftsGetPlayer();

        p = CPickup::Create(player->m_id, (CPickup::PU_TYPE)11, node, 0);
        if (!player->m_mpbomb_anchor) {
            CCompassAnchor *a = CCompass::CreateAnchor(*pos, "BOMB_GROUND");

            a->m_type = 1;
            player->CompassAnchor_MPBomb(a);
        } else
            player->m_mpbomb_anchor->m_posW = *pos;
        CCompass::DeleteAnchor(player->m_mpbase_anchor);
        player->CompassAnchor_MPBase(0);
    }
    CPickup::m_mpbombnode = node;
    return p;
}

/* (the kit's ammunition put into a stash, all of it or so many rounds, is CZKit::FillAmmoPickupInstance:
 * SOCOM 1's, OVL2 0x5BA040, the kit lane's -- zseal.h's CZKit declares it) */

/* this SEAL's ammunition dropped, all of it (-1) or so many rounds, with the pickups open and,
 * online, by a player only: the kit's rounds put into a new stash, which -- if any entry has rounds
 * -- goes into an ammunition box at the SEAL's place, turned by its own turn, named, made dynamic
 * and a character node, then a pickup of kind 9 the SEAL owns, the drop anim run on its node and
 * kept by the stash; an empty stash, or one with no node, deleted (OVL2 0x59FD50; SOCOM 1's
 * DropAmmo, the empty test new -- PickupItem and 0x5A0040 call it) */
void CZSealBody::DropAmmo(int rounds)
{
    /* (one test: the two returns share the target's `b` to the epilogue) */
    if (!CPickup::m_isOpen || (theNetwork.m_9 && !IsPlayer()))
        return;
    CZItemStash *stash = new CZItemStash;

    if (rounds == -1)
        m_kit.FillAmmoPickupInstance(stash, -1);
    else
        m_kit.FillAmmoPickupInstance(stash, rounds);
    int i;

    for (i = 0; i < stash->Count(); i++) {
        int w = 0;
        int a = 0;
        int n = 0;

        stash->Get(i, &w, &a, &n);
        if (n)
            break;
    }
    if (i >= stash->Count()) {
        delete stash;
        return;
    }
    CPnt3D off = CPnt3D::zero;
    CPnt3D at;

    m_quat.Apply(&off, &at, 1);
    at.Add(&m_posW, &at);
    CQuat q;

    q.MakeYXZRot(0.0f, 0.0f, 0.0f);
    zdb::CNode *node =
        zdb::CNode::CreateInstance((zdb::CModel *)zdb::CWorld::GetModel("fiftycal_box"), &at, &q);

    if (!node) {
        delete stash;
        return;
    }
    node->SetName("Ammobox");
    node->SetDynamic(true, true);
    node->m_socom2_a1_bits.bit2 = 1;
    node->m_bumpmap = 1;
    zdb::CWorld::m_world->AddTextureAssetCharacter(zdb::CHARSETCOMMON, node);
    CPickup *p = CPickup::Create(m_id, CPickup::PU_AMMO, node, stash);

    if (data_0044D508 && p)
        stash->SetAnim(((CZAnim *)data_0044D508)->ActivateArg(1, 3, p->m_node, 0));
}

/* the ground under a point within a depth (fts_main.cpp's ftsGetAltitudeApprox, OVL1 0x2A2EB0) */
zdb::IntersectStruct *ftsGetAltitudeApprox(const CPnt3D *p, float depth, bool chars);

extern "C" {
/* (OVL1 data: SOCOM 1's AnimTypes::SwapRifle, seal_env.cpp reads it as a variable) */
extern short data_003DEC78;
/* whether the launcher's item may be swapped now (OVL2 0x589900, seal_body.cpp's range) */
bool func_00589900(CZSealBody *seal);
void *func_00588BC0(CZSealBody *seal, AnimType type, int state, int flag, int kind);
}

/* a weapon picked up: the class's item the SEAL holds dropped (a remote's only let go), the node off the
 * pickup and on the part -- an anim of the swap played -- the launcher's rest re-parented to the spine and
 * set as gear, the kit given the node and the weapon, in the slot of the class or a new one, a launcher's
 * ammunition item added, the HUD told, the pickup destroyed (OVL2 0x59F0C0: SOCOM 1's PickupItem, which took
 * the path a remote's took here) */
void CZSealBody::PickupItem(CPickup *pickup)
{
    if (!pickup)
        return;
    if (pickup->m_type != CPickup::PU_WEAPON || !pickup->m_data || !pickup->m_node)
        return;

    bool remote = ((CSealCtrl *)m_control)->IsRemote();
    CZWeapon *weapon = pickup->GetWeapon();
    CZKit::FIRE_CLASS fc = CZKit::GetItemFireClass(weapon->m_ID);
    CPickup *dropped = 0;
    zdb::CNode *parent;
    int index;
    CZBodyPart *part = m_item_gear[fc].m_part;
    CZBodyPart *p;
    int slot;

    parent = 0;
    if (part && part->m_node)
        parent = part->m_node->m_parent;
    index = m_kit.GetFireClassIndex(fc);
    if (!remote && index != -1) {
        DropAmmo(index);
        dropped = DropItem(index, true);
    }

    zdb::CNode *node = pickup->m_node;

    zdb::CWorld::m_world->DeleteTextureAssetCharacter(zdb::CHARSETCOMMON, node);
    pickup->m_node = 0;
    if (node->m_parent)
        node->m_parent->DeleteChild(node);

    short anim = data_003DEC78;

    if (remote && parent) {
        parent->DeleteChild(part->m_node);

        zdb::CNode *old = index != -1 ? (zdb::CNode *)m_kit.GetItemPtr(index) : 0;

        if (old) {
            if (part && old == part->m_node)
                part->m_node = 0;
            zdb::CNode::Delete(old);
        }
        part->m_node = node;
    } else if (dropped) {
        dropped->GetPickupNetEntityID();
        func_00588BC0(this, (AnimType)anim, GetStance(false), 0, 0);
        if (data_0044D508)
            ((CZAnim *)data_0044D508)->ActivateArg(1, 3, dropped->m_node, 0);
    } else if (fc == CZKit::FIRE_CLASS_145 && func_00589900(this)) {
        CZBodyAnimBlend *b = (CZBodyAnimBlend *)func_00588BC0(this, (AnimType)anim, m_state, 0, 0);

        m_rifle->SetActive(false);
        b->m_24 = 0;
    }
    if (parent)
        parent->AddChild(node);
    else
        part->m_node = node;
    switch (fc) {
    case CZKit::FIRE_CLASS_RIFLE:
        node->SetName("rifle");
        break;
    case CZKit::FIRE_CLASS_PISTOL:
        node->SetName("pistol");
        break;
    case CZKit::FIRE_CLASS_145: {
        node->SetName("launcher");
        m_e9c = 0;

        p = m_item_gear[4].m_part;
        if (p && p->m_node) {
            p->m_parent->m_node->DeleteChild(p->m_node);
            p->m_parent = m_spinehi;
            p->m_parent->m_node->AddChild(p->m_node);
            p->SetActive(false);
        }
        SetPartAsGear(m_item_gear[4].m_part, m_item_gear[4].m_gear);
        break;
    }
    }
    if (index != -1) {
        m_kit.ClearItemPtr(index);
        m_kit.SetItemPtr(index, node);
        m_kit.SetItemByIndex(index, pickup->GetWeapon());
    } else {
        bool select = false;
        slot = -1;

        if (fc == CZKit::FIRE_CLASS_145) {
            slot = m_kit.GetPickupSlot(-1);
            if (slot == m_kit.GetSelectedItemNum())
                select = true;
        }
        if (slot == -1)
            slot = m_kit.GetNumItems();
        m_kit.SetItemByIndex(slot, pickup->GetWeapon());
        m_kit.SetItemPtr(slot, node);
        if (select) {
            int cur = 0;

            if (!m_kit.isSelectableItem(0))
                cur = 1;
            m_kit.SetCurItemByIndex(cur);
        }
    }
    if (fc == CZKit::FIRE_CLASS_145) {
        int n = m_kit.GetNumItems();

        m_kit.SetItem(n, weapon->m_ID == 146 ? (EQUIP_ITEM)186 : (EQUIP_ITEM)185);
        m_kit.AddAmmo(n, 1);
    }
    m_kit.m_requested_item = 1;
    m_kit.m_bit13 = 1;
    ((CSealCtrl *)m_control)->m_HudUpdateWeapon = 1;
    CPickup::Destroy(pickup);
}

/* a weapon of the kit dropped as a pickup: refused with pickups shut, or online by a SEAL no player drives;
 * the part of its fire class taken off its parent where it hangs, the item's node let go by the kit and put
 * in the world there, dynamic and a character node, a pickup of kind 8 the SEAL owns made of it, its flag
 * the pistols', the drop anim run unless asked not to (OVL2 0x59F730: SOCOM 1's DropItem, the kit's item
 * pointer calls for its vector's) */
CPickup *CZSealBody::DropItem(int index, bool quiet)
{
    if (!CPickup::m_isOpen || (theNetwork.m_9 && !IsPlayer()))
        return 0;

    CZFTSWeapon *weapon = 0;

    if (m_kit.GetNumItems() < index)
        return 0;
    m_kit.FillWeaponEnum(index, &weapon);
    if (!weapon) {
        delete weapon;
        return 0;
    }

    CPnt3D off = CPnt3D::zero;
    CZBodyPart *part;

    switch (CZKit::GetItemFireClass(((const CZWeapon *&)weapon)->m_ID)) {
    case 1:
        part = m_item_gear[1].m_part;
        break;
    case 2:
        part = m_item_gear[2].m_part;
        break;
    case 4:
        part = m_item_gear[4].m_part;
        break;
    default:
        return 0;
    }
    if (!part || !part->m_node)
        return 0;

    CQuat rot;
    CPnt3D at;
    CQuat q;

    PartInSpace(part, 0, &off, &rot);
    m_quat.Apply(&off, &at, 1);
    at.Add(&m_posW, &at);

    zdb::CNode *parent = part->m_node->m_parent;

    if (!parent)
        return 0;
    parent->DeleteChild(part->m_node);
    m_quat.Mul(&rot, &q);

    zdb::CNode *node = (zdb::CNode *)m_kit.GetItemPtr(index);

    m_kit.ClearItemPtr(index);

    CPickup *p = 0;

    if (node) {
        node->SetPosition((const PNT3D *)&at);
        node->SetRotation((const CQuat *)&q);
        if (part && node == part->m_node)
            part->m_node = 0;
        zdb::CWorld::m_world->AddChild(node);
        node->SetDynamic(true, true);
        node->m_socom2_a1_bits.bit2 = 1;
        node->m_bumpmap = 1;
        zdb::CWorld::m_world->AddTextureAssetCharacter(zdb::CHARSETCOMMON, node);
        node->SetDescendantDI(true);
        node->SetDescendantVisuals(true);
        p = CPickup::Create(m_id, CPickup::PU_WEAPON, node, weapon);
        if (p) {
            p->m_secondary = CZKit::GetItemFireClass(((CZWeapon *)weapon)->GetId()) == 2;
            if (data_0044D508 && !quiet)
                ((CZAnim *)data_0044D508)->ActivateArg(1, 3, p->m_node, 0);
        }
    }
    return p;
}

/* a dead SEAL's satchel charge dropped: the satchel gear hidden; for a SEAL this console runs, holding
 * the charge (item 152) with rounds as a weapon, a satchel model put on the ground under the SEAL's
 * place (DropAmmo's turned offset, looked for from 1 over it), named, made dynamic and a character
 * node, the charge's rounds in a new stash and a pickup of kind 9 the SEAL owns, the drop anim run on
 * its node; the pickup, or none (OVL2 0x59FAC0, new in SOCOM II: CSealCtrlAi::OnDeath calls it; the
 * name unknown -- the anim, data_0044D518, is one ReInitDatabaseHandles never sets) */
extern "C" CPickup *func_0059FAC0(CZSealBody *seal)
{
    CPickup *p = 0;
    zdb::CNode *gear = seal->FindAuxNode("Satchel", false);

    if (gear)
        gear->SetActive(false);
    if (!seal->IsRemote() && seal->m_kit.HasItemWithAmmo((EQUIP_ITEM)152)) {
        CZFTSWeapon *weapon = 0;
        int index = seal->m_kit.GetItemIndex((EQUIP_ITEM)152);

        if (index != -1)
            seal->m_kit.FillWeaponEnum(index, &weapon);
        if (weapon) {
            CPnt3D off = CPnt3D::zero;
            CQuat q;
            CPnt3D at;

            q.MakeYXZRot(0.0f, 0.0f, 0.0f);
            seal->m_quat.Apply(&off, &at, 1);
            at.Add(&seal->m_posW, &at);
            float &y = at.y;

            y += 1.0f;
            zdb::IntersectStruct *hit = ftsGetAltitudeApprox(&at, 8.0f, false);

            if (hit)
                y = hit->m_pos.y;
            zdb::CNode *node =
                zdb::CNode::CreateInstance((zdb::CModel *)zdb::CWorld::GetModel("Satchel"), &at, &q);

            if (node) {
                CZItemStash *stash = new CZItemStash;

                seal->m_kit.FillAmmoPickupInstance(stash, index);
                node->SetName("Satchel");
                node->SetDynamic(true, true);
                node->m_socom2_a1_bits.bit2 = 1;
                node->m_bumpmap = 1;
                zdb::CWorld::m_world->AddTextureAssetCharacter(zdb::CHARSETCOMMON, node);
                p = CPickup::Create(seal->m_id, CPickup::PU_AMMO, node, stash);
                if (p && data_0044D518)
                    stash->SetAnim(((CZAnim *)data_0044D518)->ActivateArg(1, 3, p->m_node, 0));
            }
        }
    }
    return p;
}

extern "C" {
/* (the kit's item at an index let go is CZKit::RemoveItemAndAmmo, zseal.h's; OVL2 0x5B9BB0, the kit
 * lane's) a weapon readied (OVL2 0x57D540, seal_body.cpp's) and the item it takes (OVL1 data
 * 0x3DEB48, a short) */
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
        m_kit.RemoveItemAndAmmo(i);
    CPnt3D pos;
    float &y = pos.y;

    pos.x = m_posW.x;
    pos.y = m_posW.y;
    pos.z = m_posW.z;

    y += 1.0f;
    zdb::IntersectStruct *hit = ftsGetAltitudeApprox(&pos, 8.0f, true);

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
bool CZSealBody::CanClimbLadder()
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

extern "C" {
/* (OVL2 0x5A7DE0 is CZSealBody::CanFire, zseal.h's) the body's current blend (seal.cpp's tail, OVL2
 * 0x555170) */
void *func_00555170(void *stack);
}

/* whether an anim is one a jump cannot start from -- the throws and both peek tosses, IsThrowing's
 * ten tests (zseal.h's AnimTypes, filled when the anims load; an inline bool: the target materialises
 * the test with daddiu/move and branches on it; CrouchThrowGrenade's second test is the source's) */
static inline bool IsNoJumpAnim(short id)
{
    if (id == AnimTypes::ThrowGrenade || id == AnimTypes::CrouchThrowGrenade ||
        id == AnimTypes::ProneThrowGrenade || id == AnimTypes::TossGrenade ||
        id == AnimTypes::PistolTossGrenade || id == AnimTypes::ProneTossGrenade ||
        id == AnimTypes::PistolProneTossGrenade || id == AnimTypes::CrouchThrowGrenade ||
        id == AnimTypes::TossGrenadePeekRight || id == AnimTypes::TossGrenadePeekLeft)
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
    if (!seal->CanFire(true, true))
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
    unsigned char _0[0x18];
    zdb::CNode *m_node;   // 0x18: the ladder's node, its faces the DIs (0x5B1C80; the name ours)
    int m_users;   // 0x1c (the names ours)
    unsigned int m_owner;   // 0x20: the entity id of the SEAL holding it
    int m_index;   // 0x24: the index a SEAL taking it holds (0x5B1130 stores it to m_ladder)

    /* the count read through an accessor: 0x5B1130 tests it, then loads it again to count itself in
     * (the field read straight is reused from the test, 10 off) */
    int GetUsers() const { return m_users; }
};

class CLadderList {
public:
    /* the ladder at an index, or none (SOCOM 1's; OVL1 0x2A9210) */
    CLadderData *GetLadderByIndex(int index);
    /* the ladder at a node (OVL1 0x2A90F0, the states lane's: a flag taken) */
    CLadderData *GetLadderByNode(zdb::CNode *node, int b);
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
 * with the SEAL's m_climb). The states a switch: its chain tests 3, then 1, then branches past. The
 * target loads the ladder list (a0) right after its lui and the index (a1) in the call's delay slot:
 * the hold's index is an UNSIGNED int passed to GetLadderByIndex's int (round 34) -- the conversion
 * moves its load after the list's; as an int MW loads it first and the list in the slot (2 off here
 * and in 0x5A2490, which inlines this, since round 22; every accessor, local and inline order tried
 * then kept the 2). */
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

extern "C" {
/* (OVL1 data: the light's override switch and value, the shadow's inside flag; the falls' anims) */
extern unsigned char data_003DF1B8;
extern float data_003DF1C0;
extern bool data_0066B470;
extern short data_003DEED0;
extern short data_003DEED8;
extern short data_003DEEE8;
extern short data_003DEEF0;
extern short data_003DEEF8;
extern short data_003DEF00;
extern short data_003DEF08;
extern short data_003DEF40;
extern short data_003DEF50;
extern short data_003DEF60;
/* (OVL2: seal_body.cpp's jump and fall steps, seal_fall.cpp's fall against the ground) */
bool func_0057DFE0(CZSealBody *seal);
void *func_0057DF20(CZSealBody *seal);
bool func_0057E130(CZSealBody *seal);
void *func_0057E050(CZSealBody *seal);
bool func_005804E0(CZSealBody *seal);
bool func_00580360(CZSealBody *seal, float t);
void func_0057F310(CZSealBody *seal);
void func_0059B440(CZSealBody *seal, zdb::IntersectStruct *hit, float *speed, bool touch, float alt);
bool func_00587FE0(CZSealBody *seal);
}

/* a fall's landing in one of the two fall anims: the damage taken at once with nothing held, at the
 * ground (the root at most 4 up) the held fall's damage -- the speed when none was kept -- and the
 * fall ended; above it the speed kept, the anim held at 3 and the kept velocity cleared (an inline: the
 * two landings expand it alike; the name ours) */
static inline void FallLanding(CZSealBody *seal, const float &speed)
{
    float held = seal->m_1364;

    if (0.0f == held) {
        seal->HandleFallingDamage(speed);
        func_005804E0(seal);
    } else if (seal->m_root->m_pos.y <= 4.0f) {
        if (held < 0.0f)
            seal->m_1364 = speed;
        if (seal->m_1364 > 0.0f)
            seal->HandleFallingDamage(((const CZSealBody *)seal)->m_1364);
        seal->m_1364 = 0.0f;
        func_005804E0(seal);
    } else {
        if (held < 0.0f)
            seal->m_1364 = speed;
        ACTIVE_ANIM *rep = ((CZBodyAnimBlend *)func_00555170(&seal->m_blends))->Representative();

        rep->m_script_wt = 3.0f;
        seal->m_1350 = CPnt3D::zero;
    }
}

/* the ground probe's result taken: none while the probe is held off; climbing (or in state 4) only the
 * altitude kept and the SEAL on the ground; else the jump's and the fall's waits run down, the SEAL put
 * in the air and stepped against the ground's hit, the throttles stopped in the air; a living SEAL in the
 * air keeps its velocity (a jump's or a fall's step taken), one come down lands -- a fall's anim, a hurt
 * state's drop, the climbs' and the drop's ends, else a landing -- and, with a polygon under it, the
 * material, the shadow's light and the visual's inside flag taken (OVL2 0x5AF930; SOCOM 1's
 * PostCheckDIDown, after HandleLanding: CZSealBody::Tick's three throttles). Byte-exact in round 71
 * (~27 off before): FallLanding's damage call reads the held speed again through a const view (MW kept
 * the compare's f12), its anim hold through a representative local (the 3.0f then stored from a GPR
 * after the calls, not hoisted to f20 ahead of them), the airborne bit into an `unsigned char` (`andi`;
 * a bool `sltu`), the node into a local before the shade's store (the int is stored by `sb` unextended) */
bool CZSealBody::PostCheckDIDown(float &x, float &y, float &z)
{
    if (HoldsNoProbe(this))
        return false;
    func_00555170(&m_blends);
    zdb::IntersectStruct *alt = &m_last_altitude;
    bool climbing = IsCurrentBlend((AnimType)46) || IsCurrentBlend((AnimType)42) ||
                    IsCurrentBlend((AnimType)AnimTypes::ClimbMedium) || IsCurrentBlend((AnimType)35) ||
                    IsCurrentBlend((AnimType)AnimTypes::StepUp) || IsCurrentBlend((AnimType)45);

    if (climbing || m_state == 4) {
        if (alt)
            m_alt = alt->m_pos.y;
        m_airborne = false;
        return false;
    }
    float speed = m_133c;
    float was = m_alt;

    if (m_135c > 0.0f) {
        m_135c -= m_2e0;
        if (m_135c < 0.0f)
            m_135c = 0.0f;
    }
    if (m_1360 > 0.0f) {
        m_1360 -= m_2e0;
        if (m_1360 <= 0.0f)
            m_133c = m_1364;
        m_jumped = true;
    }
    bool grounded = !m_airborne;

    m_airborne = true;
    if (alt && !IsCurrentBlend((AnimType)AnimTypes::ClimbOver))
        func_0059B440(this, alt, &speed, grounded, was);
    if (m_airborne) {
        x = 0.0f;
        z = 0.0f;
        y = 0.0f;
    }
    if (!climbing) {
        if (m_health > 0.0f && m_airborne && !m_carry && !IsCurrentBlend((AnimType)8) &&
            !IsCurrentBlend((AnimType)9) && !IsCurrentBlend((AnimType)data_003DEED0) &&
            !IsCurrentBlend((AnimType)data_003DEED8) && !IsCurrentBlend((AnimType)data_003DEEE8) &&
            !IsCurrentBlend((AnimType)data_003DEEF0) && !IsCurrentBlend((AnimType)AnimTypes::ClimbOver) &&
            !IsCurrentBlend((AnimType)data_003DEF50) && !IsCurrentBlend((AnimType)data_003DEF40) &&
            !IsCurrentBlend((AnimType)AnimTypes::RifleM203Reload)) {
            if (!IsCurrentBlend((AnimType)9) && !IsCurrentBlend((AnimType)data_003DEF08)) {
                switch (m_state) {
                case 3:
                case 10:
                case 9:
                    if (func_0057DFE0(this))
                        func_0057DF20(this);
                    break;
                default:
                    if (func_0057E130(this))
                        func_0057E050(this);
                    break;
                }
                m_1350 = m_velW;
            }
        } else {
            unsigned char air = m_airborne;

            if (!air && !grounded) {
                if (IsCurrentBlend((AnimType)data_003DEED8) || IsCurrentBlend((AnimType)data_003DEED0))
                    FallLanding(this, speed);
                else if (m_state == 9) {
                    HandleFallingDamage(speed);
                    if (!m_isAlive) {
                        DropBody();
                        ClearToBase(false);
                        func_00588BC0(this, (AnimType)data_003DEEE8, 8, 0, 0);
                    }
                } else if (m_state != 10 && !func_00587FE0(this) && !IsCurrentBlend((AnimType)data_003DEEF8) &&
                           !IsCurrentBlend((AnimType)data_003DEF00)) {
                    if (IsCurrentBlend((AnimType)data_003DEF50))
                        func_0057F310(this);
                    else if (IsCurrentBlend((AnimType)AnimTypes::ClimbOver))
                        PopBlend();
                    else if (IsCurrentBlend((AnimType)data_003DEF08))
                        func_00580360(this, speed);
                    else {
                        m_135c = 0.4f;
                        if (m_state != 8 && ((CZBodyAnimBlend *)func_00555170(&m_blends))->m_state != 8 &&
                            !climbing) {
                            m_jumped = false;
                            if (!m_carry) {
                                if (m_velW.x * m_velW.x + m_velW.y * m_velW.y + m_velW.z * m_velW.z < 400.0f)
                                    ClearStack(1, true);
                                else {
                                    if (m_blends.m_top + 1 > 1)
                                        PopBlend();
                                    CQuat inv = m_quat;

                                    Negate(inv.vec.x);
                                    Negate(inv.vec.y);
                                    Negate(inv.vec.z);
                                    inv.Apply(&m_velW, &m_velM, 1);
                                }
                            }
                            HandleLanding(speed);
                        }
                    }
                }
            } else if (!air && grounded) {
                if (IsCurrentBlend((AnimType)data_003DEED8) || IsCurrentBlend((AnimType)data_003DEED0))
                    FallLanding(this, speed);
                if (IsCurrentBlend((AnimType)data_003DEF50)) {
                    ClearStack(1, true);
                    func_00588BC0(this, (AnimType)data_003DEF60, 0, 0, 0);
                }
            }
        }
    }
    if (!alt)
        return false;
    if (!alt->m_poly)
        return false;
    m_onMaterial = alt->GetMaterial();
    if (data_003DF1B8) {
        float light = data_003DF1C0;

        if (light < 0.0f)
            light = AiMaps.GetInterpolatedShadow(m_mapLoc, &m_posW, data_0066B470);
        _saturate(&light, 0.0f, 1.0f);
        int shade = (int)(127.0f * (1.0f - light));
        zdb::CNode *node = GetNode();

        node->m_socom2_a0 = shade;
    } else {
        zdb::CNode *node = GetNode();

        node->m_socom2_a0 = -1;
    }
    if (zdb::CWorld::m_world->m_camera->m_fog_alt_enabled) {
        if (m_last_altitude.m_poly && m_last_altitude.m_poly->m_flags.m_inside) {
            zdb::CVisual *v = GetNode()->GetFirstVisual();

            if (v)
                v->m_bits.m_b19 = false;
        } else {
            zdb::CVisual *v = GetNode()->GetFirstVisual();

            if (v)
                v->m_bits.m_b19 = true;
        }
    }
    return true;
}

extern "C" {
/* (OVL1 data: a time that holds the landings off while above zero; a static 65535, the ladder index of
 * none, read as a float; the three anims a landing picks from; the pace a soft landing keeps an anim
 * under -- SOCOM 1's guarded statics) */
extern float data_004365E8;
extern float data_003F3438;
extern short data_003DEDF0;
extern short data_003DEE00;
extern short data_003DEEE8;
extern float data_003F3428;
void *func_00588BC0(CZSealBody *seal, AnimType type, int state, int flag, int kind);
void func_005781C0(CZSealBody *seal);
void func_005A5DA0(CZSealBody *seal);
}

/* the SEAL off its ladder: the ladder's count of holders down, none held (OVL2 0x59D750's body, inlined
 * here; the name ours) */
static inline void LeaveLadder(CZSealBody *seal)
{
    if (seal->m_ladder != data_003F3438) {
        CLadderData *ladder = theMission.m_ladders->GetLadderByIndex(seal->m_ladder);

        if (ladder)
            ladder->m_users--;
        seal->m_ladder = data_003F3438;
    }
}

/* whether landings are taken (an inline bool: built with its 1 first) */
static inline int LandingsOpen()
{
    return data_004365E8 <= 0.0f;
}

/* a landing taken: its damage; over the hard fall rate an anim by the grade, a hurt one in state 8, a
 * slow step else unless the throttles are all but off; with no body carried the blends cleared to the base
 * and the anim pushed; state 8 lets go of the ladder and the climb (OVL2 0x5AF590: SOCOM 1's HandleLanding,
 * without its two blend tests) */
void CZSealBody::HandleLanding(float speed)
{
    if (LandingsOpen()) {
        AnimType anim;
        SEAL_STATE state = (SEAL_STATE)0;
        LANDING_TYPE landing = HandleFallingDamage(speed);

        if (!m_isAlive)
            landing = LANDING_HURT;
        if (speed > theCharacterDynamics.m_landHardFallRate) {
            switch (landing) {
            case LANDING_NONE:
            case LANDING_SOFT:
                anim = (AnimType)11;
                break;
            case LANDING_HARD:
                anim = 4.656612873e-10f * (float)rand() > 0.5f ? (AnimType)data_003DEE00 : (AnimType)data_003DEDF0;
                break;
            case LANDING_HURT:
                anim = (AnimType)data_003DEEE8;
                state = (SEAL_STATE)8;
                break;
            }
        } else if (m_velW.x * m_velW.x + m_velW.y * m_velW.y + m_velW.z * m_velW.z > 400.0f &&
                   m_244 * m_244 + m_240 * m_240 > data_003F3428 * data_003F3428) {
            anim = animTypeUndefined;
            m_interp1 = 0.4f;
            m_interp2 = 0.4f;
            m_prevx = m_244;
            m_prevz = m_240;
        } else
            anim = (AnimType)10;
        if (!m_carry && anim > animTypeUndefined) {
            ClearToBase(false);
            func_00588BC0(this, anim, state, 0, 0);
            m_21c_bit2 = false;
        }
        if (state != 8)
            BodySetState(state);
        else {
            BodySetState(0);
            func_005781C0(this);
            func_005A5DA0(this);
            LeaveLadder(this);
            func_005B14E0(&m_climb, this);
            SetAmStunned(false);
        }
    }
}

extern "C" {
/* (OVL1 0x2B9E50, the game states': a SEAL's taking of a ladder passed on, with a byte and the index) */
void func_002B9E50(CZSealBody *seal, unsigned char b, int index);
}

/* whether a ladder has no SEAL on it (an inline bool, the ladder looked up again) */
static inline bool LadderFree(int index)
{
    CLadderData *ladder = theMission.m_ladders->GetLadderByIndex(index);

    return ladder ? !(ladder->m_users > 0) : false;
}

/* whether a SEAL may take a ladder: the one it is on, or with none a free one -- an AI's only in the
 * ladder mode (7) with a leg of its climb to go (an inline bool: its `return true` the target's
 * `daddiu 1`) */
static inline bool CanTakeLadder(CZSealBody *seal, int index)
{
    if (seal->m_ladder == index)
        return true;
    if (seal->m_ladder != data_003F3438)
        return false;

    bool free = LadderFree(index);

    if (free) {
        /* (IsAi written as IsPlayer's ternary: the target keeps the call's bool with `move v1,v0`
         * and narrows it once at the join, as SetCurZoomState's IsPlayer does -- zseal.h's if-form
         * narrows it in the call's arm too) */
        if (seal->m_control ? ((CSealCtrl *)seal->m_control)->IsAi() : false) {
            CSealCtrlAi *c = seal->GetSealCtrlAi();

            free = c->m_3f4 == 7 && (c->m_448 & 7) != 0;
        }
    }
    return free;
}

/* a ladder taken by the SEAL when it may and no one holds it: the SEAL its owner, the taking passed on
 * (OVL2 0x5B1350, new in SOCOM II: OVL1's 0x2B9F10 and 0x2B9F60 call it; the name unknown). The byte
 * an int here, narrowed at the call (`andi a1,s2,0xff`).
 * Byte-exact with IsAi written as a ternary in CanTakeLadder (round 24's 1-instruction open). */
extern "C" void func_005B1350(CZSealBody *seal, int b, int index)
{
    CLadderData *ladder = theMission.m_ladders->GetLadderByIndex(index);

    if (ladder) {
        if (CanTakeLadder(seal, index) && ladder->m_owner == CEntity::invalidId) {
            ladder->m_owner = seal->m_id;
            func_002B9E50(seal, b, index);
        }
    }
}

/* the SEAL's ladder, or with none a free one taken: its users counted, the SEAL's index set from the
 * ladder's, the ladder's owner cleared (an inline bool: 0x5B1130's) */
static inline bool TakeLadder(CZSealBody *seal, unsigned int index)
{
    if (seal->m_ladder == index)
        return true;
    if (seal->m_ladder != data_003F3438)
        return false;

    CLadderData *ladder = theMission.m_ladders->GetLadderByIndex(index);

    if (ladder) {
        if (ladder->GetUsers() > 0)
            return false;
        ladder->m_users++;
        seal->m_ladder = ladder->m_index;
        ladder->m_owner = CEntity::invalidId;
        return true;
    }
    return false;
}

/* a ladder's grant passed on: the first SEAL holding the ladder in state 1 under the byte takes it,
 * its hold moved to state 3 when it could (OVL2 0x5B1130, new in SOCOM II: the game states' handler
 * OVL1 0x2B9E20 calls it; the name unknown). The index an int (an unsigned short param narrows it,
 * the target only copies it: s1 for the hold's test, s0 the inline's unsigned copy); the flag set
 * before the cast and tested inside `if (seal)` (a null SEAL steps on past the test); the compares
 * with the parameter on the left (`bne s2,v1`) */
extern "C" void func_005B1130(unsigned char b, int index)
{
    CEntityList::const_iterator it;

    for (it = CEntity::GetEntityList().begin(); it != CEntity::GetEntityList().end(); ++it) {
        bool done = false;
        CEntity *e = *it;
        CZSealBody *seal = e && e->m_type == CEntity::ENT_TYPE_SEAL ? (CZSealBody *)e : 0;

        if (seal) {
            switch (seal->m_climb.m_state) {
            case 1:
                if (b == seal->m_climb.m_active && index == seal->m_climb.m_ladder) {
                    if (TakeLadder(seal, index))
                        seal->m_climb.m_state = 3;
                    done = true;
                }
            }
            if (done)
                break;
        }
    }
}

extern "C" {
/* (OVL2 0x5B1C80, below: the SEAL put on a ladder by its index) */
bool func_005B1C80(CZSealBody *seal, int index);
}

/* the SEAL's hold on a ladder ticked: in state 1 a held ladder the SEAL can no longer take lets the
 * hold go; state 3 takes a held ladder up (0x5B1C80) and lets the hold go; state 2, and any but 0, let
 * it go (OVL2 0x5B1570, new in SOCOM II: PreTick calls it with the SEAL's m_climb; the name unknown).
 * CanTakeLadder inlined (0x5B1350's); the switch's chain tests 3, 2, 1, then 0. The hold's flag is
 * tested by a one-case switch in states 1 and 3 (`beq` to the case over a `b` out): an `if` branches
 * straight past (0x10 short) */
extern "C" void func_005B1570(CZSealBody::CSealClimb *climb, CZSealBody *seal)
{
    switch (climb->m_state) {
    case 0:
        break;
    case 1:
        switch (climb->m_active) {
        case 1:
            if (!CanTakeLadder(seal, climb->m_ladder)) {
                climb->m_state = 0;
                climb->m_active = 0;
            }
        }
        break;
    case 2:
        climb->m_state = 0;
        climb->m_active = 0;
        break;
    case 3:
        switch (climb->m_active) {
        case 1:
            func_005B1C80(seal, climb->m_ladder);
        }
        climb->m_state = 0;
        climb->m_active = 0;
        break;
    default:
        climb->m_state = 0;
        climb->m_active = 0;
        break;
    }
}

extern "C" {
/* CPnt3D's implicit assignment, OVL2's out-of-line copy (OVL1's is 0x20F1F0, CHoming's reset's): the
 * homing's points are set through it, the inline that sets them too tall for it to inline */
CPnt3D *func_00539E90(CPnt3D *to, const CPnt3D *from);
/* (OVL2 0x545EE0, seal.cpp's range: what a carried body's turret entity is told, with 1) */
void func_00545EE0(CZSealBody *seal, int n);
}

/* a blend's homing aimed: the blend, its reference point, the wall's normal and the point it closes
 * on, the reference direction -z and no target point, both counters cleared (an inline member: its
 * `this`, the blend's homing, stays in a saved register as the copies go through the assignment's
 * out-of-line copy -- zbody.h is another lane's, so a stand-in class over CHoming's layout; the names
 * ours) */
class CHomingAim : public CHoming {
public:
    void Aim(CZBodyAnimBlend *b, const CPnt3D *refpt, const CPnt3D *norm, const CPnt3D *intpt)
    {
        m_blend = b;
        func_00539E90(&m_refpt_m, refpt);
        func_00539E90(&m_norm_w, norm);
        func_00539E90(&m_int_w, intpt);
        func_00539E90(&m_refvec_m, &CPnt3D::unit_nz);
        func_00539E90(&m_target_refpt_m, &CPnt3D::zero);
        m_1c = 0;
        m_2c = 0;
    }
};

static inline CHomingAim *HomingOf(CZBodyAnimBlend *b)
{
    return (CHomingAim *)&b->m_homing;
}

/* the homing's turn: the angle between the wall's normal in the SEAL's frame and the reference
 * direction, its rate over the frame about the sign of their cross's y (an inline; the name ours) */
static inline float SafeAcosH(float c)
{
    if (!(c < 1.0f))
        return 0.0f;
    if (c <= -1.0f)
        return 3.1415927f;
    return acosf(c);
}

/* the homing stepped, the first way: the reference point's place in the world measured against the
 * point it closes on, the blend turned at the rate (held within 1.5 pi) that lays its reference
 * direction onto the wall's normal, and moved by the rest in the SEAL's frame over the frame, each
 * part held within 30 -- the rest's length on the flat returned, the dot of the two directions given
 * back when asked (OVL2 0x5B2D20; SOCOM 1's HomingTick's other half, new in SOCOM II: CZSealBody's
 * Tick_0 calls it for a blend flagged 64; the name ours) */
/* a vector turned by a node's rotation in place (an inline over TransformVectors: its VU0 temporaries a
 * level deeper, under the caller's own; the name ours) */
static inline void RotateBy(const zdb::CNode *node, CPnt3D *v)
{
    TransformVectors(&node->m_matrix, v, v, 1);
}

extern "C" float func_005B2D20(CZSealBody *seal, float *cosOut)
{
    CPnt3D f;
    CPnt3D e;
    CPnt3D g;
    CPnt3D nrm;
    CZBodyAnimBlend *top = (CZBodyAnimBlend *)func_00555170(&seal->m_blends);
    CHoming *h = &top->m_homing;

    if (!h)
        return 999999.0f;
    float t = top->m_tfrac;

    top->Representative()->m_anim->TrueLoopTime();
    if (func_005551A0(top->m_flags, 32))
        t = 1.0f - t;
    CPnt3D a = h->m_int_w;

    a.Sub(&seal->m_posW, &a);
    CPnt3D b = seal->GetRootPart()->m_pos;

    RotateBy(seal->m_node, &b);
    a.Sub(&b, &a);
    CQuat inv = seal->m_quat;

    inv.vec.x = -inv.vec.x;
    inv.vec.y = -inv.vec.y;
    inv.vec.z = -inv.vec.z;
    inv.Apply(&h->m_norm_w, &nrm, 1);
    CQuat q = CQuat::identity;
    CPnt3D *ref = &h->m_refvec_m;
    float c = nrm.x * ref->x + nrm.y * ref->y + nrm.z * ref->z;

    if (cosOut)
        *cosOut = c;
    float ang = SafeAcosH(c);
    CPnt3D rot;

    rot.x = 0.0f;
    rot.y = ang / seal->m_2e0;
    if (!(rot.y <= 4.712389f))
        rot.y = 4.712389f;
    else if (rot.y < -4.712389f)
        rot.y = -4.712389f;
    rot.z = 0.0f;
    CPnt3D axis;

    CrossVU0(*ref, nrm, &axis);
    if (axis.y < 0.0f)
        rot.y = -rot.y;
    rot.Scale(axis.Length(), &rot);
    top->m_drot = rot;
    CPnt3D half;
    CQuat al;

    rot.Scale(-0.5f * seal->m_2e0, &half);
    q.Exp(&half);
    al.Align(*ref, nrm);
    CPnt3D p;
    CPnt3D p2;

    al.Apply(&h->m_refpt_m, &p, 1);
    TransformVectors(&seal->m_node->m_matrix, &p, &p2, 1);
    quadW(t);
    a.Sub(&p2, &e);
    f = e;

    inv.Apply(&f, &g, 1);
    float len = sqrtf(e.x * e.x + e.z * e.z);

    g.x /= seal->m_2e0;
    g.y /= seal->m_2e0;
    g.z /= seal->m_2e0;

    if (!(g.x <= 30.0f))
        g.x = 30.0f;
    else if (g.x < -30.0f)
        g.x = -30.0f;
    if (!(g.y <= 30.0f))
        g.y = 30.0f;
    else if (g.y < -30.0f)
        g.y = -30.0f;
    if (!(g.z <= 30.0f))
        g.z = 30.0f;
    else if (g.z < -30.0f)
        g.z = -30.0f;
    top->m_dpos = g;
    top->m_homing.m_1c++;
    return len;
}

/* the homing stepped: the blend turned at the rate that lays its reference direction onto the wall's
 * normal, and moved by the share quadW gives of what is left between its reference point, turned and
 * placed in the world, and the point it closes on -- the step's length on the flat returned, the dot
 * of the two directions given back when asked (OVL2 0x5B3340; SOCOM 1's HomingTick's body, the
 * position now the root's, its fly test 44/46; CZSealBody's Tick_0 calls it with no out; the name
 * ours) */
extern "C" float func_005B3340(CZSealBody *seal, float *cosOut)
{
    CPnt3D d2;
    CPnt3D d;
    CPnt3D d3;
    CZBodyAnimBlend *top = (CZBodyAnimBlend *)func_00555170(&seal->m_blends);
    CHoming *h = &top->m_homing;

    if (!h)
        return 999999.0f;
    float t = top->m_tfrac;

    if (func_005551A0(top->m_flags, 32))
        t = 1.0f - t;
    CQuat inv = seal->m_quat;

    inv.vec.x = -inv.vec.x;
    inv.vec.y = -inv.vec.y;
    inv.vec.z = -inv.vec.z;
    CPnt3D nrm;

    inv.Apply(&h->m_norm_w, &nrm, 1);
    top->Representative()->m_anim->TrueLoopTime();
    CPnt3D *ref = &h->m_refvec_m;
    float c = nrm.x * ref->x + nrm.y * ref->y + nrm.z * ref->z;

    if (cosOut)
        *cosOut = c;
    float ang = SafeAcosH(c);
    CPnt3D rot;

    rot.x = 0.0f;
    rot.y = ang / seal->m_2e0;
    if (!(rot.y <= 6.2831855f))
        rot.y = 6.2831855f;
    else if (rot.y < -6.2831855f)
        rot.y = -6.2831855f;
    rot.z = 0.0f;
    CPnt3D axis;

    CrossVU0(*ref, nrm, &axis);
    if (axis.y < 0.0f)
        rot.y = -rot.y;
    rot.Scale(axis.Length(), &rot);
    top->m_drot = rot;
    CPnt3D half;
    CQuat q;
    CQuat al;

    rot.Scale(-0.5f * seal->m_2e0, &half);
    q.Exp(&half);
    al.Align(*ref, nrm);
    CPnt3D p;
    CPnt3D p2;

    al.Apply(&h->m_refpt_m, &p, 1);
    TransformVectors(&seal->m_node->m_matrix, &p, &p2, 1);
    p2.x = -p2.x;
    p2.y = -p2.y;
    p2.z = -p2.z;
    CPnt3D r = h->m_int_w;

    p2.Add(&r, &p2);
    CPnt3D w;

    CZBodyPart *root = seal->m_root;

    seal->m_node->m_matrix.Transform(&root->m_pos, &w, 1);
    p2.Sub(&w, &d);
    float s = quadW(t);

    s = sqrtf(s);
    d.Scale(s, &d);
    q.Apply(&d, &d2, 1);
    inv.Apply(&d2, &d3, 1);
    if (seal->IsCurrentBlend((AnimType)44) ||
        (seal->IsCurrentBlend((AnimType)46) &&
         func_005551A0(((CZBodyAnimBlend *)func_00555170(&seal->m_blends))->m_flags, 32)))
        d3.y = 0.0f;
    float len = sqrtf(d.x * d.x + d.z * d.z);

    d3.Scale(1.0f / seal->m_2e0, &d3);
    top->m_dpos = d3;
    CPnt3D v = seal->m_velM;

    v.Scale(seal->m_2e0, &v);
    h->m_refpt_m.Sub(&v, &h->m_refpt_m);
    top->m_homing.m_1c++;
    return len;
}

/* a point brought onto a ladder's run between its two ends, on the flat: within 3 of one end only, put
 * 3 from that end along the run; within 3 of both -- or asked for the middle -- put at the middle; false,
 * left as it is, when it is near neither (OVL2 0x5B3A60, new in SOCOM II: 0x5B2620 calls it three times,
 * the SEAL and its ladder passed unread; the names ours). The run's direction declared ahead of the
 * offset (CPnt3D locals: the later-declared at the lower slot); the middle assigned, not initialised
 * (a copy-initialised local is stored loads-first through its address, an assignment member by member
 * at fixed offsets) */
extern "C" bool func_005B3A60(CZSealBody *seal, const CPnt3D *ends, CPnt3D *pos, void *ladder, bool mid)
{
    if (!mid) {
        CPnt3D w;
        CPnt3D v;

        v = ends[0];
        v.Sub(pos, &v);
        v.y = 0.0f;
        float d0 = v.Length();

        v = ends[1];
        v.Sub(pos, &v);
        v.y = 0.0f;
        float d1 = v.Length();

        if (!(d0 < 3.0f) && !(d1 < 3.0f))
            return false;

        w = ends[1];
        w.Sub(&ends[0], &w);
        w.y = 0.0f;
        w.Normalize();
        if (d0 < 3.0f && !(d1 < 3.0f)) {
            v = ends[0];
            w.Scale(3.0f, &w);
            v.Add(&w, pos);
            return true;
        }
        if (!(d0 < 3.0f) && d1 < 3.0f) {
            v = ends[1];
            w.Scale(3.0f, &w);
            v.Sub(&w, pos);
            return true;
        }
    }

    CPnt3D m;

    m = ends[0];
    m.Add(&ends[1], &m);
    m.Scale(0.5f, pos);
    return true;
}

extern "C" {
/* the ladder's tests, the taking told to the others and the climb's place (OVL2 0x57F7D0, 0x57F9C0,
 * seal_body.cpp's; OVL1 0x2B9F60, fts_remote.cpp's; OVL2 0x5B3A60, 0x5B1130, this unit's) */
bool func_0057F7D0(CZSealBody *seal);
bool func_0057F9C0(CZSealBody *seal);
void func_002B9F60(int obj, int a, int b);
}

/* whether the SEAL has the climbs' anims (an inline: the `&&` of plain `!= 0` tests a value MW builds in v0
 * with no join label -- an inline per test takes a saved register a step, `if (ok) ok = ...` pads its joins;
 * the name ours) */
static inline bool HasClimbs(CZSealBody *seal)
{
    return seal->m_animset->GetScript((AnimType)33) != 0 && seal->m_animset->GetScript((AnimType)44) != 0 && seal->m_animset->GetScript((AnimType)46) != 0 && seal->m_animset->GetScript((AnimType)AnimTypes::ClimbMedium) != 0 && seal->m_animset->GetScript((AnimType)42) != 0 && seal->m_animset->GetScript((AnimType)AnimTypes::StepUp) != 0;
}

/* whether an AI may take a ladder at a node (0x5B1350's test by node: holding none, the ladder free, in the
 * climb's mode with a leg of the climb's kinds; an inline, the name ours) */
static inline bool LadderNodeFree(CZSealBody *seal, zdb::IntersectStruct *hit)
{
    if (seal->m_ladder != data_003F3438)
        return false;

    CLadderData *ladder = theMission.m_ladders->GetLadderByNode(hit->m_node, 1);
    bool free = ladder ? !(ladder->m_users > 0) : false;

    if (free) {
        if (seal->m_control ? ((CSealCtrl *)seal->m_control)->IsAi() : false) {
            CSealCtrlAi *c = seal->GetSealCtrlAi();

            free = c->m_3f4 == 7 && (c->m_448 & 7) != 0;
        }
    }
    return free;
}

/* the wall ahead taken: by the face's kind a climb, a step over or a ladder -- refused when the anims are
 * missing, in states 9 and 10, when facing off it or out of the edge's points -- the climb placed on the
 * edge's point and aimed; a ladder only asked for, told to the others online (OVL2 0x5B2620: SOCOM 1's
 * ImpactFront, its faces' kinds and its checks as they stand).
 * OPEN, 24 instructions, all of them frame offsets -- every instruction the target's but a 16-byte slot at
 * sp+176, between hi and edge, that no instruction touches (frame 256 against 240). No source gives a slot
 * with no code: an unused CPnt3D, a dead copy, `(void)&x`, a dead pointer or reference to it, a class with an
 * empty constructor and destructor, a by-value inline parameter ignored, or unreachable code using it
 * (build/scratch/seal-r58/v_620*.py). Otherwise written to the word: the x of d read twice by Length and Dot
 * where y and z share their address temporaries */
bool CZSealBody::ImpactFront(zdb::IntersectStruct *hit, CPnt3D *pos)
{
    if (!HasClimbs(this))
        return false;
    if (m_state == 9 || m_state == 10)
        return false;

    CPnt3D d = hit->m_pos;
    CPnt3D at = hit->m_pos;
    const CPnt3D *n = &hit->m_norm;

    d.Sub(&m_2bc, &d);
    d.y = 0.0f;
    d.x = -d.x;
    d.y = -d.y;
    d.z = -d.z;
    *pos = m_2bc;

    unsigned char kind = (unsigned char)hit->m_poly->m_flags.m_appflags & 7;
    float len = d.Length();
    float dot = Dot(d, *n) / len;
    CPnt3D lo, hi;

    if (!hit->GetIntersectedEdgePoints(&hi, &lo))
        return false;
    if (kind == 2) {
        if (TargetIsAi(this) && !LadderNodeFree(this, hit))
            return false;
        if (hi.y - lo.y <= 15.0f) {
            if (fabsf(dot) < 0.02f)
                return false;
        } else if (dot < 0.02f)
            return false;
    } else if (dot < 0.3f)
        return false;
    d.x = -d.x;
    d.y = -d.y;
    d.z = -d.z;

    CPnt3D edge;

    if (!hit->GetIntersectedEdgePointsY(&edge))
        return false;

    float h = hi.y + MaterialEx(hit->GetMaterial())->m_foot_step_offset - m_posW.y;

    switch (kind) {
    case 4:
        func_005B3A60(this, &edge, &at, pos, false);
        if (h > 5.0f) {
            if (h <= 32.0f)
                Climb(h);
            else
                return false;
        }
        break;
    case 1:
        func_005B3A60(this, &edge, &at, pos, true);
        Climb(h);
        break;
    case 5:
        func_005B3A60(this, &edge, &at, pos, false);
        ClimbOver(h);
        break;
    case 2: {
        bool up;

        if (hi.y - lo.y <= 15.0f) {
            if (!func_0057F7D0(this))
                return false;
            up = true;
        } else {
            if (!func_0057F9C0(this))
                return false;
            up = false;
        }

        CLadderData *ladder = theMission.m_ladders->GetLadderByNode(hit->m_node, 1);
        int index = ladder->m_index;

        bool held = m_climb.m_state == 1 && m_climb.m_active == 1 && index == m_climb.m_ladder;

        if (!held) {
            if (theNetwork.m_9) {
                m_climb.m_active = 1;
                m_climb.m_ladder = index;
                m_climb.m_state = 1;
                m_climb.m_up = up;
                func_002B9F60(m_id, 1, index);
            } else if (!(ladder->m_users > 0)) {
                m_climb.m_active = 1;
                m_climb.m_ladder = index;
                m_climb.m_state = 1;
                m_climb.m_up = up;
                func_005B1130(1, index);
            }
        }
        return false;
    }
    case 3:
        func_005B3A60(this, &edge, &at, pos, false);
        Climb(h);
        break;
    default:
        return false;
    }
    AlignForClimb(false, at, hi, (CPnt3D *)n);
    return true;
}

extern "C" {
/* the wall ahead's hit (OVL2 0x54E430, seal.cpp's) and the hold dropped (OVL2 0x5B3890, below) */
zdb::IntersectStruct *func_0054E430(CZSealBody *seal);
void func_005B3890(CZSealBody *seal, int drop_now);
}

/* whether one of the climbs' anims is on top (an inline: each test a call, the join one bool; the name ours) */
static inline bool InClimbAnim(CZSealBody *seal)
{
    if (seal->IsCurrentBlend((AnimType)42) || seal->IsCurrentBlend((AnimType)AnimTypes::ClimbMedium) ||
        seal->IsCurrentBlend((AnimType)33) || seal->IsCurrentBlend((AnimType)AnimTypes::ClimbOver) ||
        seal->IsCurrentBlend((AnimType)AnimTypes::StepUp) || seal->IsCurrentBlend((AnimType)44))
        return true;
    return false;
}

/* whether the SEAL can take the wall it holds by a kind: the anims there, the action allowed, a ladder free,
 * no climb playing, facing it -- by a small limit on a ladder, by the edge's drop; the ladder ahead of it --
 * then by the kind: a step short enough one way, within 10 the impact taken and the hold dropped, one up to
 * 32 otherwise (OVL2 0x5B3CE0: SOCOM 1's CanTraverse, its faces' kinds grown) */
bool CZSealBody::CanTraverse(CDIType type)
{
    zdb::IntersectStruct *hit = m_1090.m_node ? (zdb::IntersectStruct *)&m_1090 : 0;

    if (!hit)
        return false;

    unsigned char kind = (unsigned char)hit->m_poly->m_flags.m_appflags & 7;

    if (kind != (unsigned char)type)
        return false;
    if (!HasClimbs(this))
        return false;
    if (!func_005B4340(this, 2))
        return false;
    if (kind == 2 && !LadderNodeFree(this, hit))
        return false;
    func_00555170(&m_blends);
    if (InClimbAnim(this))
        return false;

    CPnt3D d = *(const CPnt3D *)m_node->m_matrix.m_matrix[2];

    d.y = 0.0f;

    float len = d.Length();
    float dot = Dot(d, hit->m_norm) / len;

    if (kind == 2) {
        CPnt3D lo, hi;

        if (!hit->GetIntersectedEdgePoints(&hi, &lo))
            return false;
        if (hi.y - lo.y <= 15.0f) {
            if (fabsf(dot) < 0.02f)
                return false;
        } else if (dot < 0.02f)
            return false;

        CPnt3D e = m_posW;

        e.Sub(&hit->m_pos, &e);
        e.y = 0.0f;
        if (Dot(e, hit->m_norm) <= 0.0f)
            return false;
    } else if (dot < 0.3f)
        return false;
    hit = func_0054E430(this);
    switch (kind) {
    case 4: {
        CPnt3D lo, hi;

        if (!hit->GetIntersectedEdgePoints(&hi, &lo))
            return false;

        float h = hi.y + MaterialEx(hit->GetMaterial())->m_foot_step_offset - m_posW.y;

        if (h <= 5.0f)
            return true;
        if (h <= 10.0f) {
            CPnt3D p = m_posW;

            ImpactFront(m_1090.m_node ? (zdb::IntersectStruct *)&m_1090 : 0, &p);
            func_005B3890(this, 1);
            return false;
        }
        return h <= 32.0f;
    }
    case 5:
        return true;
    case 2:
    case 1:
    case 3:
        return true;
    }
    return false;
}


extern "C" void func_005B3890(CZSealBody *seal, int drop_now);

static inline void SetActionJump(CZSealBody *seal, bool on)
{
    seal->m_action_jump = on;
}

/* whether an edge's drop is short (an int inline: the comparison built with its 1 first, masked at the
 * caller's test; the name ours) */
static inline int ShortDrop(const CPnt3D &hi, const CPnt3D &lo)
{
    return hi.y - lo.y <= 15.0f;
}

/* (a body's controller remote, each answer its own block; the name ours) */
static inline bool DrivenRemotely(const CZSealBody *seal)
{
    if (seal->m_control)
        return ((CSealCtrl *)seal->m_control)->IsRemote();
    return false;
}

/* the wall's hit the SEAL holds run into: refused when the jump's action is up (cleared), with no hit held,
 * when moving off it; facing it by more than the limit -- on a climbable face (kind 2) by a small limit,
 * either way round when the edge's drop is short -- the impact taken, the hold dropped on any other, the
 * two throttles and the velocities zeroed; always false (OVL2 0x5B4B40, new in SOCOM II: Tick calls it
 * twice; the name unknown) */
extern "C" bool func_005B4B40(CZSealBody *seal, float *a, float *b)
{
    if (seal->m_action_jump) {
        SetActionJump(seal, false);
        return false;
    }

    zdb::IntersectStruct *hit = seal->m_1090.m_node ? (zdb::IntersectStruct *)&seal->m_1090 : 0;

    if (!hit)
        return false;

    CPnt3D d = hit->m_pos;

    d.Sub(&seal->m_2bc, &d);
    d.y = 0.0f;

    float len = sqrtf(d.x * d.x + d.y * d.y + d.z * d.z);

    d.Scale(-1.0f, &d);
    if ((d.x * hit->m_norm.x + d.y * hit->m_norm.y + d.z * hit->m_norm.z) / len < 0.3f)
        return false;
    TransformVectors(&seal->m_node->m_matrix, &CPnt3D::unit_z, &d, 1);

    float dot = d.x * hit->m_norm.x + d.y * hit->m_norm.y + d.z * hit->m_norm.z;

    if (((unsigned char)hit->m_poly->m_flags.m_appflags & 7) == 2) {
        CPnt3D lo, hi;

        if (!hit->GetIntersectedEdgePoints(&hi, &lo))
            return false;
        if ((bool)ShortDrop(hi, lo)) {
            if (fabsf(dot) < 0.02f)
                return false;
        } else if (dot < 0.02f)
            return false;
    } else if (dot < 0.3f)
        return false;
    if (!DrivenRemotely(seal)) {
        CPnt3D pos = seal->m_posW;

        seal->ImpactFront(hit, &pos);
    }
    if (((unsigned char)hit->m_poly->m_flags.m_appflags & 7) != 2)
        func_005B3890(seal, 1);
    *a = 0.0f;
    *b = 0.0f;
    seal->m_velM = CPnt3D::zero;
    seal->m_1350 = CPnt3D::zero;
    return false;
}

extern "C" {
extern short data_003DEAE0;
extern short data_003DEAE8;
extern short data_003DEF08;
extern AnimSet *data_0044D3F8;
extern AnimSet *data_0044D410;
bool func_00588030(CZSealBody *seal);
AnimType func_005A0950(CZSealBody *seal, DAMAGE_LOCATION loc, SEAL_STATE state);
bool func_00587FE0(CZSealBody *seal);
bool func_00587F90(CZSealBody *seal);
bool func_00587F50(CZSealBody *seal);
bool func_00587F10(CZSealBody *seal);
bool func_0057DF10(CZSealBody *seal);
bool func_0057B510(CZSealBody *seal);
}

/* the blend on top (an inline over the stack's Top, OVL2 0x555170; the name ours) */
static inline CZBodyAnimBlend *TopBlend(CZSealBody *seal)
{
    return (CZBodyAnimBlend *)func_00555170(&seal->m_blends);
}

/* whether an anim is a throw's or a toss's, CQB's, a stab's or a restraint's (inlines over the anim types'
 * rows; the names ours) */
static inline bool IsThrowAnim(short t)
{
    if (t == AnimTypes::ThrowGrenade || t == AnimTypes::CrouchThrowGrenade || t == AnimTypes::ProneThrowGrenade ||
        t == AnimTypes::TossGrenade || t == AnimTypes::PistolTossGrenade || t == AnimTypes::ProneTossGrenade ||
        t == AnimTypes::PistolProneTossGrenade || t == AnimTypes::CrouchThrowGrenade)
        return true;
    return false;
}

static inline bool IsCQBAnim(short t)
{
    if (t == AnimTypes::CQB || t == AnimTypes::PistolCQB)
        return true;
    return false;
}

static inline bool IsStabAnim(short t)
{
    if (t == AnimTypes::PistolStab || t == AnimTypes::Stab)
        return true;
    return false;
}

static inline bool IsRestrainAnim(short t)
{
    if (t == AnimTypes::ApplyRestraints || t == AnimTypes::PistolApplyRestraints)
        return true;
    return false;
}

static inline bool BlendFlag4(const CZBodyAnimBlend *b)
{
    return (b->m_flags & 4) ? true : false;
}

/* whether the SEAL may start an action of a kind: alive, out of the state 8, the anim tests for each kind
 * (OVL2 0x5B4340, new in SOCOM II: the reload, the change of an item, the restraint, the CQB and the knife
 * ask it, with the kit's Reload and the controller's PreTick; the name unknown) */
extern "C" bool func_005B4340(CZSealBody *seal, unsigned char kind)
{
    if (!seal->m_isAlive)
        return false;
    if (seal->m_state == 8)
        return false;
    if (func_00588030(seal) || func_00587FE0(seal))
        return false;
    if (seal->IsCurrentBlend((AnimType)data_003DEAE0))
        return false;
    if (seal->m_state == 5 || TopBlend(seal)->m_state == 5)
        return false;
    if (seal->IsCurrentBlend((AnimType)data_003DEF08))
        return false;
    if (kind == 1) {
        switch (seal->m_state) {
        case 0:
        case 1:
        case 2:
        case 3:
            break;
        default:
            return false;
        }
        if (seal->m_carry)
            return false;

        CZBodyAnimBlend *top = TopBlend(seal);

        if (!top)
            return false;
        if (seal->m_state != 3 && !seal->CanInterrupt())
            return false;
        if ((seal->m_airborne ? true : false) || func_00587F50(seal) || func_00587F10(seal))
            return false;
        if (seal->IsCurrentBlend((AnimType)data_003DEAE8))
            return false;
        if (IsThrowAnim(TopBlend(seal)->m_animType))
            return false;
        if (func_0057DF10(seal))
            return false;
        switch (top->m_state) {
        case 7:
        case 5:
        case 4:
            return false;
        }
        return true;
    }
    if (kind == 2 || kind == 3 || kind == 4 || kind == 5 || kind == 6 || kind == 7 || kind == 8) {
        if (seal->m_state == 8 || TopBlend(seal)->m_state == 8)
            return false;
        if (seal->m_state == 9 || seal->m_state == 10 || seal->m_carry)
            return false;
        if (seal->IsChangingWeapons(true))
            return false;
        if (seal->IsReloading())
            return false;
        if (IsCQBAnim(TopBlend(seal)->m_animType))
            return false;
        if (IsStabAnim(TopBlend(seal)->m_animType))
            return false;
        if (IsRestrainAnim(TopBlend(seal)->m_animType))
            return false;
        if (seal->m_am_stunned)
            return false;
        if (seal->m_animset == data_0044D3F8)
            return false;
    }
    if (kind == 3 || kind == 4 || kind == 5 || kind == 6 || kind == 7 || kind == 8) {
        if (seal->m_state == 4 || TopBlend(seal)->m_state == 4)
            return false;
    }
    if (kind == 2 || kind == 6 || kind == 7 || kind == 8 || kind == 11) {
        if (seal->m_state == 2)
            return false;
        if (seal->m_animset == data_0044D408)
            return false;
        if (seal->m_animset == data_0044D410)
            return false;
    }
    if (kind == 4) {
        if (seal->m_state == 3 || TopBlend(seal)->m_state == 3)
            return false;
        if (seal->m_airborne || seal->m_state == 6)
            return false;
        if (func_00587F50(seal) || func_00587F10(seal))
            return false;
        if (!(seal->m_1340 <= 0.001f))
            return false;
    }
    if (kind == 3) {
        if (seal->IsCurrentBlend((AnimType)46))
            return false;
        if (BlendFlag4(TopBlend(seal)) || func_00588030(seal) || func_00587FE0(seal) || func_00587F90(seal))
            return false;
        if (!seal->m_kit.CurWeaponIsM203Mode() && (seal->m_airborne || seal->m_state == 6))
            return false;
        if ((func_00587F50(seal) || func_00587F10(seal)) && !seal->m_kit.CurWeaponIsM203Mode())
            return false;
        if (func_0057B510(seal))
            return false;
    }
    if (kind == 7) {
        if (func_00587F50(seal) || func_00587F10(seal))
            return false;
    }
    if (kind == 10) {
        if (seal->m_animset == data_0044D408)
            return false;
        if (seal->m_bit9)
            return false;
    }
    return true;
}

/* the wall's hit the SEAL holds kept or dropped: dropped when asked, or when the SEAL is more than 24
 * from it on the flat, stands behind its face or faces away from it -- and, on a climbable face
 * (application flags 2), when its edge's drop is within 15 but the edge's foot above the SEAL by more
 * than 10, or for a SEAL of bit 9; kept as it is when the face's edges cannot be found (OVL2 0x5B3890,
 * new in SOCOM II: the stance setters, the ladder's leave and the states call it; the name unknown).
 * The edge points declared lower first (the target's upper one at the lower slot).
 * OPEN, 16 instructions (the size right): the flattening store -- the target stores d.y = 0 with
 * `sw zero,100(sp)` in the reach test's delay slot and loads d.x twice (f2 squared in place, f7 for
 * the face's dot), d.z once; MW here stores it early through `addiu a0,sp,100` and keeps d.x and d.z
 * in f3/f7 for both. Tried: zmath.h's Dot for either or both products (the zero is then reloaded, 0x1D0
 * and more), a pointer Dot, the reach in a local, 24*24, the flag declared first, the zero an int, a
 * PNT3D view, a Flatten inline, the normal by reference. */
extern "C" void func_005B3890(CZSealBody *seal, int drop_now)
{
    zdb::IntersectStruct *hit = seal->m_1090.m_node ? (zdb::IntersectStruct *)&seal->m_1090 : 0;

    if (!hit || drop_now) {
        seal->m_1090.m_node = 0;
        return;
    }

    CPnt3D d;
    bool drop = false;

    seal->m_posW.Sub(&hit->m_pos, &d);
    d.y = 0.0f;
    if (d.x * d.x + d.y * d.y + d.z * d.z > 576.0f)
        drop = true;
    if (d.x * hit->m_norm.x + d.y * hit->m_norm.y + d.z * hit->m_norm.z <= 0.0f)
        drop = true;

    const CMatrix &m = seal->m_node->m_matrix;

    if (m.m_matrix[2][0] * hit->m_norm.x + m.m_matrix[2][1] * hit->m_norm.y + m.m_matrix[2][2] * hit->m_norm.z <= 0.0f)
        drop = true;

    unsigned char kind = (unsigned char)hit->m_poly->m_flags.m_appflags & 7;

    if (!drop && kind == 2) {
        CPnt3D lo, hi;

        if (!hit->GetIntersectedEdgePoints(&hi, &lo))
            return;
        if (hi.y - lo.y <= 15.0f && !(lo.y <= seal->m_posW.y + 10.0f))
            drop = true;
        if (seal->m_bit9)
            drop = true;
    }
    if (drop)
        seal->m_1090.m_node = 0;
}

/* a body carried: the top blend homed onto a point, along the flat reverse of a direction, its target
 * the reference point itself; a carried body working an entity told so with 1 (OVL2 0x5B1750, new in
 * SOCOM II: RestrainBuddy, StabBuddy and 0x579A30 call it; the name unknown). The direction is turned
 * round by NegateInto(&d, &d): the in-place pointer form gives the y and z their own address registers
 * and renumbers every saved register (55 words off) */
extern "C" void func_005B1750(CZSealBody *seal, const CPnt3D *refpt, const CPnt3D *intpt, const CPnt3D *dir)
{
    if (seal->m_carry) {
        CZBodyAnimBlend *top = (CZBodyAnimBlend *)func_00555170((char *)seal + 0x1c0);
        CPnt3D d = *dir;

        d.y = 0.0f;
        d.Normalize();
        NegateInto(&d, &d);
        HomingOf(top)->Aim(top, refpt, &d, intpt);
        top->m_homing.m_target_refpt_m = *refpt;
        if (seal->m_carry->m_eb8)
            func_00545EE0(seal->m_carry, 1);
    }
}

extern "C" {
/* the anim a SEAL climbing down a ladder turns by (OVL1 data, an AnimType read as a short) */
extern short data_003DEAE0;
/* how near two of a rung's heights must be to be its two ends (OVL2 data) */
extern float data_006506B8;
bool func_0052EB60(const float &a, const float &b, const float &eps);
/* (OVL2 0x57F690 and 0x57F880, seal_body.cpp's: the ladder anims pushed; 0x5B1890, below) */
CZBodyAnimBlend *func_0057F690(CZSealBody *seal);
CZBodyAnimBlend *func_0057F880(CZSealBody *seal);
CZBodyAnimBlend *func_005B1890(CZSealBody *seal);
}

/* the cross product of two points through VU0, normalised (CPnt3D::Cross's body with its flag set, an
 * inline here; the name ours) */
static inline void CrossNormVU0(const CPnt3D *a, const CPnt3D *b, CPnt3D *out)
{
    sceVu0FVECTOR r;
    sceVu0FVECTOR va;
    sceVu0FVECTOR vb;

    va[0] = a->x;
    va[1] = a->y;
    va[2] = a->z;
    va[3] = 1.0f;
    vb[0] = b->x;
    vb[1] = b->y;
    vb[2] = b->z;
    vb[3] = 1.0f;
    sceVu0OuterProduct(r, va, vb);
    sceVu0Normalize(r, r);
    out->x = r[0];
    out->y = r[1];
    out->z = r[2];
}

/* one point less another, by value (an inline over CPnt3D::Sub; the name ours) */
static inline CPnt3D Less(const CPnt3D &a, const CPnt3D &b)
{
    CPnt3D r;

    a.Sub(&b, &r);
    return r;
}

/* the SEAL put on a ladder by its index: none, and the SEAL is off every ladder (false); else the face of
 * the ladder's node whose height (over 15 or not) suits the climb's way -- up a short face, down a tall
 * one -- found, its rung (the two ends of a level edge) taken: up, the ladder anim's goal set on the
 * rung's middle and the SEAL turned onto the face at the anim's rate; down, the climb-down anim homed
 * onto it; then the ladder taken (OVL2 0x5B1C80, new in SOCOM II: 0x5B1570 and 0x5880E0 call it; the name
 * ours). The DI walk's iterator ASSIGNED (`it = begin()`: a temporary, the iterator and the postfix ++'s two
 * copies all in memory); the face's three points as references declared together (the third's address
 * held in s7 across the first Sub); the named points declared in the target's slot order, the edges
 * initialised from a by-value inline; the turn's rate computed into a local before the three stores
 * (a CPnt3D temporary adds a slot); the down-climb's height as 15.6f + y (ref.y is not forwarded) */
extern "C" bool func_005B1C80(CZSealBody *seal, int index)
{
    CLadderData *ladder = theMission.m_ladders->GetLadderByIndex(index);

    if (!ladder) {
        func_0059D750(seal);
        return false;
    }
    CPnt3D fwd;
    CPnt3D norm;
    CPnt3D lo;
    CPnt3D hi;
    zdb::CNode *node = ladder->m_node;
    CMatrix m;

    node->BuildMTW(m);
    zdb::CDIVector &dis = node->m_di;

    TransformVectors(&seal->m_node->m_matrix, &CPnt3D::unit_nz, &fwd, 1);
    unsigned char up = seal->m_climb.m_up;
    bool found = false;
    zdb::CDIVector::iterator it;

    for (it = dis.begin(); it != dis.end(); it++) {
        zdb::CDI *di = *it;
        const CPnt3D &a = *(CPnt3D *)&di->m_pnts[0];
        const CPnt3D &b = *(CPnt3D *)&di->m_pnts[1];
        const CPnt3D &c = *(CPnt3D *)&di->m_pnts[2];
        float top = -99999.0f;
        float bot = 99999.0f;
        CPnt3D n;
        CPnt3D e1 = Less(b, a);
        CPnt3D e2 = Less(c, a);
        CPnt3D pts[4];

        CrossNormVU0(&e1, &e2, &n);
        TransformVectors(&m, &n, &norm, 1);
        for (unsigned int i = 0; i < di->m_flags.m_ptcount; i++) {
            CPnt3D q = *(CPnt3D *)&di->m_pnts[i];

            m.Transform(&q, &pts[i], 1);
            if (pts[i].y > top)
                top = pts[i].y;
            if (pts[i].y < bot)
                bot = pts[i].y;
            if (i != 0 && func_0052EB60(pts[i].y, pts[i - 1].y, data_006506B8)) {
                lo = pts[i - 1];
                hi = pts[i];
            }
        }
        bool low = top - bot <= 15.0f;

        if ((low && up) || (!low && !up)) {
            found = true;
            break;
        }
    }
    if (!found) {
        func_0059D750(seal);
        return false;
    }
    CPnt3D mid;

    mid.x = 0.5f * lo.x + 0.5f * hi.x;
    mid.y = 0.5f * lo.y + 0.5f * hi.y;
    mid.z = 0.5f * lo.z + 0.5f * hi.z;
    if (up) {
        CZBodyAnimBlend *b = func_0057F690(seal);
        REFPT_INFO &info = b->m_refpt_info;

        info.m_goal_pos = mid;
        info.m_goal_norm = norm;
        info.m_valid = 1;
        CQuat inv = seal->m_quat;
        CPnt3D d;

        inv.vec.x = -inv.vec.x;
        inv.vec.y = -inv.vec.y;
        inv.vec.z = -inv.vec.z;
        inv.Apply(&norm, &d, 1);
        float c = d.x * CPnt3D::unit_nz.x + d.y * CPnt3D::unit_nz.y + d.z * CPnt3D::unit_nz.z;

        if (c < 0.0f) {
            float a = SafeAcosH(c);

            if (d.x < 0.0f)
                a = 6.2831855f - a;
            CZBodyAnimBlend *t = (CZBodyAnimBlend *)func_00588BC0(seal, (AnimType)data_003DEAE0, 0, 0, 13);
            CZSIScriptCommon *s = t->Representative()->m_anim;

            float rate = -a / s->LoopTime();

            seal->m_velR.x = 0.0f;
            seal->m_velR.y = rate;
            seal->m_velR.z = 0.0f;
        } else
            func_005B1890(seal);
    } else {
        CZBodyAnimBlend *b = func_0057F880(seal);
        CPnt3D ref;

        func_0058C2F0(seal, b->Representative()->m_anim, &ref);
        ref.y = 15.6f;
        mid.y = 15.6f + seal->m_posW.y;
        NegateInto(&norm, &norm);
        HomingOf(b)->Aim(b, &ref, &norm, &mid);
        CPnt3D z = CPnt3D::zero;

        b->m_homing.m_target_refpt_m = z;
    }
    TakeLadder(seal, index);
    return true;
}

extern "C" {
/* an anim's reference height when it has one, else 0 (OVL2 0x58C2B0, seal_cfg.cpp's range: its draft
 * matches) */
float func_0058C2B0(CZSealBody *seal, AnimType type);
}

/* a climb's homing aimed at a wall: the point it closes on taken to the given height, the anim's own
 * reference point -- for ClimbMedium mixed toward the anim's reference height by the top blend's
 * first weight once it blends two, one higher for StepUp -- and the wall's normal turned about, the
 * target offset by the climb (11.78 up and 2.42 back for anim 33, 0.29 down and 2.42 back for
 * ClimbOver); none unless one of the climbs (or anim 42) is current, nor when told it is done (OVL2
 * 0x5B1A10, SOCOM 1's AlignForClimb, called once from ImpactFront 0x5B2620 in both builds -- the
 * round 48's name). The wall's normal copied in and negated in place at the end, the offset
 * assigned to the homing's target after the aim */
void CZSealBody::AlignForClimb(bool done, CPnt3D &intpt, CPnt3D &at, CPnt3D *wall)
{
    if (done)
        return;
    CZBodyAnimBlend *top = (CZBodyAnimBlend *)func_00555170((char *)this + 0x1c0);
    CPnt3D norm = *wall;
    CPnt3D ref;

    func_0058C2F0(this, top->Representative()->m_anim, &ref);
    CPnt3D off = CPnt3D::zero;

    intpt.y = at.y;
    if (IsCurrentBlend((AnimType)33)) {
        off.y = 11.78f;
        off.z = -2.42f;
    } else if (IsCurrentBlend((AnimType)AnimTypes::ClimbMedium)) {
        if (top->CountAnims() > 1) {
            float w = top->m_anims.Begin()->m_wt;

            ref.y = (1.0f - w) * func_0058C2B0(this, (AnimType)AnimTypes::ClimbMedium) + w * ref.y;
        }
    } else if (IsCurrentBlend((AnimType)AnimTypes::ClimbOver)) {
        off.y = -0.29f;
        off.z = -2.42f;
    } else if (IsCurrentBlend((AnimType)AnimTypes::StepUp))
        ref.y += 1.0f;
    else if (!IsCurrentBlend((AnimType)42))
        return;
    NegateInto(&norm, &norm);
    HomingOf(top)->Aim(top, &ref, &norm, &intpt);
    top->m_homing.m_target_refpt_m = off;
}

/* the top blend homed onto its own goal: its goal marked unreached, the reference point a fixed offset
 * lowered by 11.487 (0x4137CAC0: 11.4869995) less the goal's height over the SEAL, half a unit up
 * again, the goal's normal turned about, the reference direction +z (OVL2 0x5B1890, new in SOCOM II:
 * 0x589AA0 calls it; the name unknown). The goal's info through a reference (its address kept for the
 * homing's normal), the goal assigned field by field and its height taken before the offset is built */
extern "C" CZBodyAnimBlend *func_005B1890(CZSealBody *seal)
{
    CZBodyAnimBlend *top = (CZBodyAnimBlend *)func_00555170((char *)seal + 0x1c0);
    REFPT_INFO &info = top->m_refpt_info;

    info.m_valid = 0;
    CPnt3D goal;
    goal = info.m_goal_pos;
    float h = goal.y - seal->m_posW.y;
    CPnt3D off(0.347f, -1.412f, 3.2f);

    goal.y += off.y;
    off.y -= 11.4869995f;
    off.y += h;
    off.y += 0.5f;
    NegateInto(&info.m_goal_norm, &info.m_goal_norm);
    HomingOf(top)->Aim(top, &off, &info.m_goal_norm, &goal);
    top->m_homing.m_refvec_m = CPnt3D::unit_z;
    return top;
}

/* whether the SEAL is on a given ladder (OVL2 0x59D6C0, new in SOCOM II; 0x56BD10 asks it) */
extern "C" bool func_0059D6C0(CZSealBody *seal, CLadderData *ladder)
{
    if (!ladder || seal->m_ladder == data_003F3438)
        return false;
    return theMission.m_ladders->GetLadderByIndex(seal->m_ladder) == ladder;
}

/* a hit's stun, by the item that hit: 123 two of kind 0, 125 (tear gas) three of kind 1, 122 (smoke)
 * and 127 two of kind 3, anything else five of kind 4, scaled when a scale is given -- HandleStunDamage
 * inlined in each (SOCOM 1's HandleStunFromHit by its place before HandleStunDamage and its tests of
 * the items, in its order; OVL2 0x5A5250: SOCOM II's takes the scale, SOCOM 1's listed the kind-4
 * items) */
extern "C" void func_005A5250(CZSealBody *seal, EQUIP_ITEM item, const CPnt3D *from, const float *scale)
{
    switch (item) {
    case 123:
        seal->HandleStunDamage((STUN_TYPE)0, 2.0f, from);
        break;
    case 125:
        seal->HandleStunDamage((STUN_TYPE)1, 3.0f, from);
        break;
    case 122:
    case 127:
        seal->HandleStunDamage((STUN_TYPE)3, 2.0f, from);
        break;
    default: {
        float amount = 5.0f;

        if (scale)
            amount *= *scale;
        seal->HandleStunDamage((STUN_TYPE)4, amount, from);
    }
    }
}

/* a kind's stun recovered: the stunned bit cleared, and a kind-5 stun leaving twenty of kind 6 (SOCOM
 * 1's HandleStunRecovery, its other cases' additions gone -- their table slots kept; SOCOM II inlines
 * it everywhere) */
inline void CZSealBody::HandleStunRecovery(STUN_TYPE type)
{
    SetAmStunned(false);
    switch (type) {
    case 0:
        break;
    case 1:
        break;
    case 2:
        break;
    case 3:
        break;
    case 4:
        break;
    case 5:
        HandleStunDamage((STUN_TYPE)6, 20.0f, 0);
        break;
    case 6:
        break;
    }
}

/* the stuns run down at the character's recovery rate, the SEAL stunned while any is left, each kind
 * recovered as it reaches zero; not stunned, the bit cleared (OVL2 0x5A50A0; SOCOM 1's, the else
 * new) */
void CZSealBody::TickStun(float dT)
{
    if (m_stunned) {
        m_stunned = 0;
        for (int i = 0; i < 7; i++) {
            bool was = m_stunval[i] > 0.0f;

            if (m_stunval[i] > 0.0f && m_charType) {
                m_stunval[i] -= (float)m_charType->m_recovery_factor * dT;
                if (m_stunval[i] > 0.0f)
                    m_stunned = 1;
            }
            if (was && m_stunval[i] <= 0.0f)
                HandleStunRecovery((STUN_TYPE)i);
        }
    } else
        SetAmStunned(false);
}

/* in tear gas (item 125): its stun taken and the flag set (OVL2 0x5A0B60; 0x5B5050 calls it) */
extern "C" void func_005A0B60(CZSealBody *seal)
{
    func_005A5250(seal, (EQUIP_ITEM)125, 0, 0);
    seal->SetInGas(true);
}

/* in smoke (item 122): the same with the smoke flag (OVL2 0x5A0BB0; 0x5B5050 calls it) */
extern "C" void func_005A0BB0(CZSealBody *seal)
{
    func_005A5250(seal, (EQUIP_ITEM)122, 0, 0);
    seal->SetInSmoke(true);
}

/* a death's anim for a location and a stance, one of the table's at random: a SEAL on the player's
 * team (the player himself aside) dies by the teammates' table -- its location 3's anim for the stance,
 * then for standing, when that one is none -- anyone else by the deaths'; the deaths' location 3
 * standing anim when all else is none (OVL2 0x5A0950, new in SOCOM II: FindRandomDeathAnim hands it the
 * stance it settled on, the falling death and five more ask it -- the name unknown) */
/* an entity's team bits (an inline: the player's are loaded first, as two inline calls are expanded
 * left to right -- the plain loads in either order load the SEAL's first) */
static inline unsigned int TeamOf(const CEntity *e)
{
    return e->m_team;
}

extern "C" AnimType func_005A0950(CZSealBody *seal, DAMAGE_LOCATION loc, SEAL_STATE state)
{
    short anim = -1;

    if (!seal->IsPlayer() && (TeamOf(ftsGetPlayer()) & TeamOf(seal))) {
        int n = AnimSet::teammatedeathanim[loc][state].size();

        if (n > 0) {
            anim = AnimSet::teammatedeathanim[loc][state][n == 1 ? 0 : rand() % n];
            if (anim == -1)
                anim = AnimSet::teammatedeathanim[3][state][0];
            if (anim == -1)
                anim = AnimSet::teammatedeathanim[3][0][0];
        }
    } else {
        int n = AnimSet::deathanim[loc][state].size();

        if (n > 0) {
            anim = AnimSet::deathanim[loc][state][n == 1 ? 0 : rand() % n];
            if (anim == -1)
                anim = AnimSet::deathanim[3][state][0];
        }
    }
    if (anim == -1)
        anim = AnimSet::deathanim[3][0][0];
    return (AnimType)anim;
}

extern "C" {
/* the state of the topmost blend in state 0, 1 or 2, else 0 (seal_body.cpp's, OVL2 0x57DBB0: SOCOM 1's
 * BaseState) and a halfword's bits tested (seal.cpp's tail copy, OVL2 0x5551A0) */
unsigned char func_0057DBB0(CZSealBody *seal);
bool func_005551A0(unsigned short flags, unsigned short bits);
}

/* a death's anim for where the SEAL was hit: the stance it dies in -- dead already, the blends' base
 * state with the body put in state 8; carried or carrying, standing; any other but a stand, a crouch or
 * prone, the base state; stunned, prone -- and, past half of a stance change (17, 19, 25: before half,
 * reversed, by the top blend's flag 0x20), the stance it is going to; then one of that stance's at random
 * (OVL2 0x5A0700; SOCOM 1's, its pick moved out into 0x5A0950) */
AnimType CZSealBody::FindRandomDeathAnim(DAMAGE_LOCATION where, bool *b)
{
    *b = false;
    SEAL_STATE state = GetStance(false);

    switch (state) {
    case stateStand:
    case stateCrouch:
    case stateProne:
        break;
    case stateCarrying:
    case stateCarried:
        state = stateStand;
        break;
    case stateDead:
        state = (SEAL_STATE)func_0057DBB0(this);
        BodySetState(8);
        break;
    default:
        state = (SEAL_STATE)func_0057DBB0(this);
        break;
    }
    if (m_am_stunned)
        state = stateProne;
    if (IsCurrentBlend((AnimType)17) || IsCurrentBlend((AnimType)19) || IsCurrentBlend((AnimType)25)) {
        float t = ((CZBodyAnimBlend *)func_00555170(&m_blends))->m_tfrac;
        bool late = false;
        bool back = func_005551A0(((CZBodyAnimBlend *)func_00555170(&m_blends))->m_flags, 32);

        if (back) {
            if (t > 0.5f)
                late = true;
        } else if (t < 0.5f)
            late = true;
        if (late) {
            if (IsCurrentBlend((AnimType)17)) {
                if (back)
                    state = stateCrouch;
                else
                    state = stateStand;
            }
            if (IsCurrentBlend((AnimType)19)) {
                if (back)
                    state = stateProne;
                else
                    state = stateCrouch;
            }
            if (IsCurrentBlend((AnimType)25)) {
                if (back)
                    state = stateProne;
                else
                    state = stateStand;
            }
        }
    }
    return func_005A0950(this, where, state);
}

extern "C" {
/* three hurt anims a pick moves off when the flag is set (OVL1 data, the AnimTypes statics two to four
 * before Stab -- SOCOM 1's there are DeathStandChest03 and its pistol pair: not named, SOCOM II's
 * order changed) and the flag (OVL1 0x45A0C1, seal_ai.cpp's) */
extern short data_003DEE00;
extern short data_003DEDF8;
extern short data_003DEDF0;
extern unsigned char data_0045A0C1;
}

/* a hurt anim for where the SEAL was hit: the stance as for a death (idle, carrying and carried count as
 * standing), past half of a stance change the stance it is going to, stunned prone -- the state kept
 * as the desired one unless the body is idle, dead, carrying or carried; then one of that stance's at
 * random, none when there are none -- with the flag set, a pick of the three anims above moves on to
 * the next of the others, up to eight, one of them at random (OVL2 0x5A0270; SOCOM 1's, the move-on
 * new). The body's state test one unsigned range (`sltiu at` branched on: a switch over the four
 * gives a compare chain and costs the first switch its delay-slot fills, 0x40 long); `count > 1`
 * branches on `slti at` where `>= 2` computes into v0; the control's IsRemote called, its result
 * unused (a compiled-out print's test, likely) */
AnimType CZSealBody::FindRandomDamageAnim(DAMAGE_LOCATION where, bool *b)
{
    *b = false;
    SEAL_STATE state = GetStance(false);

    switch (state) {
    case stateStand:
    case stateCrouch:
    case stateProne:
        break;
    case stateIdle:
    case stateCarrying:
    case stateCarried:
        state = stateStand;
        break;
    case stateDead:
        state = (SEAL_STATE)func_0057DBB0(this);
        BodySetState(8);
        break;
    default:
        state = (SEAL_STATE)func_0057DBB0(this);
        break;
    }
    if (IsCurrentBlend((AnimType)17) || IsCurrentBlend((AnimType)19) || IsCurrentBlend((AnimType)25)) {
        float t = ((CZBodyAnimBlend *)func_00555170(&m_blends))->m_tfrac;
        bool late = false;
        bool back = func_005551A0(((CZBodyAnimBlend *)func_00555170(&m_blends))->m_flags, 32);

        if (back) {
            if (t > 0.5f)
                late = true;
        } else if (t < 0.5f)
            late = true;
        if (late) {
            if (IsCurrentBlend((AnimType)17)) {
                if (back)
                    state = stateCrouch;
                else
                    state = stateStand;
            }
            if (IsCurrentBlend((AnimType)19)) {
                if (back)
                    state = stateProne;
                else
                    state = stateCrouch;
            }
            if (IsCurrentBlend((AnimType)25)) {
                if (back)
                    state = stateProne;
                else
                    state = stateStand;
            }
        }
    }
    if (m_am_stunned)
        state = stateProne;
    if ((unsigned int)(m_state - stateIdle) > stateCarried - stateIdle)
        m_desiredState = state;

    int n = AnimSet::damageanim[where][state].size();

    if (n <= 0)
        return (AnimType)-1;

    AnimType anim;

    if (n == 1)
        anim = AnimSet::damageanim[where][state][0];
    else {
        int r = rand() % n;

        anim = AnimSet::damageanim[where][state][r];
        if (data_0045A0C1) {
            if (m_control)
                ((CSealCtrl *)m_control)->IsRemote();
            if (anim == data_003DEE00 || anim == data_003DEDF8 || anim == data_003DEDF0) {
                int picks[8];
                int count = 0;

                for (int i = (r + 1) % n; i != r; i = (i + 1) % n) {
                    anim = AnimSet::damageanim[where][state][i];
                    if (anim != data_003DEE00 && anim != data_003DEDF8 && anim != data_003DEDF0) {
                        picks[count++] = i;
                        if (count >= 8)
                            break;
                    }
                }
                if (count == 1)
                    r = picks[0];
                else if (count > 1)
                    r = picks[rand() % count];
                if (count > 0)
                    anim = AnimSet::damageanim[where][state][r];
            }
        }
    }
    return anim;
}

/* the death's end, once its anim has run: unless the SEAL is of team 2 or remote -- or, online, one
 * whose hits go unfelt, a ghost or in a game whose flag 0xDC is set -- its ammunition dropped, then its
 * primary weapon and its pistol (offline only where no such weapon lies within 50), then, online, its
 * class-145 item when that holds item 185's or 186's rounds (OVL2 0x5A0040, new in SOCOM II: the death's
 * callbacks call it -- the name unknown) */
extern "C" void func_005A0040(CZSealBody *seal)
{
    if (seal->m_team & 2)
        return;

    bool drop = true;

    if (seal->IsRemote())
        drop = false;
    if (theNetwork.m_9 && (func_002C2FD0(seal) || seal->m_deathType == DEATH_GHOST || data_00437CE8->m_dc))
        drop = false;
    if (!drop)
        return;

    int primary = seal->m_kit.GetPrimaryIndex();
    int secondary = seal->m_kit.GetSecondaryIndex();
    int launcher = theNetwork.m_9 ? seal->m_kit.GetClass145Index() : -1;
    bool ammo = false;

    if (launcher != -1) {
        if (seal->m_kit.HasItem((EQUIP_ITEM)185))
            ammo = seal->m_kit.ItemHasAmmo((EQUIP_ITEM)185);
        else if (seal->m_kit.HasItem((EQUIP_ITEM)186))
            ammo = seal->m_kit.ItemHasAmmo((EQUIP_ITEM)186);
    }
    seal->DropAmmo(-1);
    if (primary != -1 && (theNetwork.m_9 || !CPickup::IsWeaponNear(seal->m_kit.GetPrimaryEnum(), &seal->m_posW, 50.0f)))
        seal->DropItem(primary, false);
    if (secondary != -1 &&
        (theNetwork.m_9 || !CPickup::IsWeaponNear(seal->m_kit.GetSecondaryEnum(), &seal->m_posW, 50.0f)))
        seal->DropItem(secondary, false);
    if (ammo)
        seal->DropItem(launcher, false);
}

extern "C" {
/* a ZAnim let go (OVL1 0x2707C0, unowned: seal_ai.cpp's reading) */
void func_002707C0(CZAnim *anim, bool now);
}

/* a ripple anim stopped and forgotten when it runs (an inline, the name ours) */
static inline void StopRipple(CZAnim *&anim)
{
    if (anim && anim->Running()) {
        func_002707C0(anim, false);
        anim = 0;
    }
}

extern "C" {
/* (OVL1 data: how fast the water's depth slows the SEAL, and the least it slows it to) */
extern float data_0044C35C;
extern float data_0044C358;
}

/* the share of a throttle the ground's slope leaves along an axis: the cosine's complement to the
 * ground's normal when the axis leans into it, none past straight against it (an inline, the name ours) */
static inline float SlopeShare(const CPnt3D &norm, const CPnt3D &v)
{
    float d = Dot(norm, v);

    if (d <= -1.0f)
        return 0.0f;
    if (d < 0.0f)
        return sqrtf(1.0f - d * d);
    return 1.0f;
}

/* the throttles slowed by the water's depth (down to its least) and turned down on a slope, each by its
 * axis -- the body's x for the first, its -z for the second, each flipped for a negative throttle --
 * squared; none in the air (OVL2 0x5B56C0, between HandleWater and SealProcessAltitude where SOCOM 1 had
 * GetTerrainMotionFactor, which returned one factor; the name unknown) */
extern "C" void func_005B56C0(CZSealBody *seal, float *z, float *x)
{
    CPnt3D v;

    if (seal->m_airborne)
        return;
    float f = 1.0f;

    if (seal->m_waterdepth > 0.0f) {
        f -= seal->m_waterdepth * data_0044C35C;
        if (f < data_0044C358)
            f = data_0044C358;
    }
    if (f < 0.0f || f > 1.0f)
        f = 1.0f;
    bool zs = _close_enough(*z, 0.0f, data_003F3428);

    if (!zs) {
        float s = 1.0f;

        if (*z > 0.0f) {
            TransformVectors(&seal->m_node->m_matrix, &CPnt3D::unit_x, &v, 1);
            s = SlopeShare(seal->m_last_altitude.m_norm, v);
        } else if (*z < 0.0f) {
            CPnt3D a = CPnt3D::unit_x;
            Negate(a.x);
            Negate(a.y);
            Negate(a.z);
            TransformVectors(&seal->m_node->m_matrix, &a, &v, 1);
            s = SlopeShare(seal->m_last_altitude.m_norm, v);
        }
        s *= s;
        if (s > 1.0f)
            s = 1.0f;
        *z *= s;
    }
    bool xs = _close_enough(*x, 0.0f, data_003F3428);

    if (!xs) {
        float s = 1.0f;

        if (*x > 0.0f) {
            TransformVectors(&seal->m_node->m_matrix, &CPnt3D::unit_nz, &v, 1);
            s = SlopeShare(seal->m_last_altitude.m_norm, v);
        } else if (*x < 0.0f) {
            TransformVectors(&seal->m_node->m_matrix, &CPnt3D::unit_z, &v, 1);
            s = SlopeShare(seal->m_last_altitude.m_norm, v);
        }
        s *= s;
        if (s > 1.0f)
            s = 1.0f;
        *x *= s;
    }
    *z *= f;
    *x *= f;
}

/* a hit's water entered: the SEAL's box against the surface -- wading (the surface between its feet
 * and its top) the big ripples by the gait, the small ones stopped, and a fall's splash on landing;
 * just under the surface (within 10 over its top) the small ripples, the big ones stopped; the depth
 * kept, the in-water flag set while in it, both ripples stopped once dead (SOCOM 1's HandleWater, the
 * gait an index now -- none for a SEAL at rest, which is not in water -- and the splash new; OVL2
 * 0x5B52B0: SealProcessAltitude asks it after HandleSmokeAndGas, with whether the SEAL was in water
 * before) */
void CZSealBody::HandleWater(zdb::IntersectStruct *hit, const CMaterialEx *mat, const CPnt3D *pos, bool wasin)
{
    const CBBox *box = GetNode()->GetBBox();
    float top = box->m_max.y + m_posW.y;
    float bottom = box->m_min.y + m_posW.y;
    float water = hit->m_pos.y;
    int gait;

    switch (GetSpeedState()) {
    case 1:
        gait = 0;
        break;
    case 2:
        gait = 1;
        break;
    case 3:
        gait = 2;
        break;
    default:
        gait = -1;
        break;
    }
    bool moving = gait != -1;

    m_waterdepth = water - bottom;
    if (top < bottom) {
        float t = top;

        top = bottom;
        bottom = t;
    }
    if (hit)
        m_wateranimnode.SetPosition(hit->m_pos.x, hit->m_pos.y, hit->m_pos.z);
    if (moving) {
        if (water > bottom && water < top) {
            StopRipple(m_belowwateranim);
            if (data_0044D520[gait] && (!m_inwateranim || !m_inwateranim->Running()) && m_isAlive)
                m_inwateranim = data_0044D520[gait]->ActivateArg(1, 3, &m_wateranimnode, 0);
            if (m_airborne && !wasin && data_0044D540)
                data_0044D540->ActivateArg(1, 5, &hit->m_pos, 0);
            m_inwater = true;
        } else if (water > top && water < 10.0f + top) {
            StopRipple(m_inwateranim);
            if (data_0044D530[gait] && (!m_belowwateranim || !m_belowwateranim->Running()) && m_isAlive)
                m_belowwateranim = data_0044D530[gait]->ActivateArg(1, 3, &m_wateranimnode, 0);
            m_inwater = true;
        } else if (water > top)
            m_inwater = true;
        else
            m_inwater = false;
    } else
        m_inwater = false;
    if (!m_isAlive) {
        StopRipple(m_belowwateranim);
        StopRipple(m_inwateranim);
    }
}

/* a particle effect's node extension (SOCOM 1's DWARF: CParticleEx : CNodeEx, its emitter at 4; its
 * virtuals all CNodeEx's inline ones, so the dynamic_cast below makes the unit emit OVL2's copy of its
 * RTTI, 0x65F0C0) */
class CParticleEx : public zdb::CNodeEx {
public:
    Particle::Source *m_emitter;   // 0x4
};

/* whether a value lies in a range, and a point in a box's x/y and in the whole box (inlines returning
 * through a result: the target's `daddiu 1` per test, each level's result zeroed ahead of the tests
 * it holds). The bounds by reference: the target loads the upper one only after the lower test
 * (by value both load at the call); `!(v < lo)`, as `v >= lo` materialises the test */
static inline bool InRange(float v, const float &lo, const float &hi)
{
    bool r = false;

    if (!(v < lo) && v <= hi)
        r = true;
    return r;
}

static inline bool InBoxXY(const CBBox &b, const CPnt3D &p)
{
    bool r = false;

    if (InRange(p.x, b.m_min.x, b.m_max.x) && InRange(p.y, b.m_min.y, b.m_max.y))
        r = true;
    return r;
}

static inline bool InBox(const CBBox &b, const CPnt3D &p)
{
    bool r = false;

    if (InBoxXY(b, p) && InRange(p.z, b.m_min.z, b.m_max.z))
        r = true;
    return r;
}

/* a hit's particle cloud entered: a point inside the cloud node's box in the world takes the tear gas
 * (cloud type 0) or the smoke (type 1) (SOCOM 1's HandleSmokeAndGas, its box test and its stuns now
 * helpers; OVL2 0x5B5050: SealProcessAltitude 0x5B5D40 asks it for each hit, then HandleWater, as
 * SOCOM 1's) */
void CZSealBody::HandleSmokeAndGas(zdb::IntersectStruct *hit, const CMaterialEx *mat, const CPnt3D *pos)
{
    if (mat->IsParticle()) {
        zdb::CNode *node = hit->m_node;
        CBBox box;

        node->m_matrix.Transform(&node->GetBBox()->m_min, &box.m_min, 2);
        box.Normalize();
        CParticleEx *ex = node ? dynamic_cast<CParticleEx *>(node->m_nodeEx) : 0;
        Particle::Source *src = ex->m_emitter;

        if (src) {
            if (src->m_cloudtype == 0) {
                if (InBox(box, *pos))
                    func_005A0B60(this);
            } else if (src->m_cloudtype == 1) {
                if (InBox(box, *pos))
                    func_005A0BB0(this);
            }
        }
    }
}

/* the SEAL the application camera shows (camera.h's CAppCamera, as much as this unit reads) */
class CAppCamera {
public:
    virtual void Tick(float dT);
    virtual bool AttachTo(CZSealBody *prey);

    unsigned char _4[0xbc - 0x4];
    CZSealBody *m_bc;   // 0xbc
};

extern CAppCamera *appCamera;

/* a hit polygon's bit 31 as a bool, none for no polygon (an inline, the name ours) */
static inline bool PolyBit31(const zdb::IntersectStruct *is)
{
    return is->m_poly ? (bool)is->m_poly->m_flags.m_bit31 : false;
}

/* the hits under a point handled: the volumetric ones kept and their smoke or gas taken, the water
 * handled, any other hit -- a SEAL's only when it is down (2) or dead (8), and none of them for a
 * player -- a candidate for the ground: the highest at or under the point's height plus the given
 * one, else the lowest; the ripples stopped when the SEAL left the water, the ground dropped when it
 * lies over 20 above the node, and for the camera's SEAL its regions updated, the node's bit 6 set
 * by the polygon's bit 31 and a SEAL underfoot marked (SOCOM 1's SealProcessAltitude, SOCOM II's
 * player test, the water's flag passed on and the node's flags new; OVL2 0x5B5D40: SealGetAltitude's
 * tail).
 * Byte-exact (round 47; 0x454 against 0x45C in round 46): the 8 bytes were the camera's block, not
 * the early return -- the target loads the world's camera into a saved register ahead of the first
 * GetRegion call, a local of its own (`w = global; w->g(call())`); then the saved registers (this s3,
 * best s2, lowest s1, the counter s0, the hit s4, the material s5): `tools/igreplay.py --want` with
 * the hit as the only mover put it after the material's temporary (136 of 146 placements, none
 * among the named locals), so the hit is an inline's result -- the element's address through
 * GetIntersect below merges into a temporary numbered after every named local; and the last two
 * (the in-water byte's store scheduled ahead of `best = 0`, `add.s $f20,$f0,$f20`): best declared
 * after the water's reset, the height plus the point's a float local of its own (`f = pos->y + f`,
 * `f += pos->y` and `f = f + pos->y` all give `$f20,$f20,$f0`) */
/* the hit at an index (an inline: its result is the loop's hit) */
static inline zdb::IntersectStruct *GetIntersect(zdb::DiIntersect *di, int i)
{
    return &di->m_Intersects[i];
}

zdb::IntersectStruct *CZSealBody::SealProcessAltitude(zdb::DiIntersect *di, const CPnt3D *pos, float f)
{
    m_volumetricAltitudes.clear();
    if (m_isingas)
        SetInGas(false);
    if (m_isinsmoke)
        SetInSmoke(false);
    if (!di)
        return 0;
    bool wasin = m_inwater;

    m_inwater = false;
    m_waterdepth = 0.0f;
    zdb::IntersectStruct *best = 0;
    zdb::IntersectStruct *lowest = 0;

    if (di->m_Cnt) {
        float top = pos->y + f;

        for (int i = 0; i < di->m_Cnt; i++) {
            zdb::IntersectStruct *hit = GetIntersect(di, i);
            CMaterialEx *mat = MaterialEx(hit->GetMaterial());

            if (mat && mat->IsVolumetric()) {
                m_volumetricAltitudes.push_back(*hit);
                HandleSmokeAndGas(hit, mat, pos);
            } else if (mat && mat->IsLiquid())
                HandleWater(hit, mat, pos, wasin);
            else {
                CZSealBody *seal = GetSeal(hit->m_node);

                if (seal) {
                    if (IsPlayer())
                        continue;
                    if (seal->m_state != 2 && seal->m_state != 8)
                        continue;
                }
                float y = hit->m_pos.y;

                if (y <= top && (!best || !(y < best->m_pos.y)))
                    best = hit;
                if (!lowest || !(lowest->m_pos.y < y))
                    lowest = hit;
            }
        }
    }
    if (!best)
        best = lowest;
    if (wasin && !m_inwater) {
        StopRipple(m_belowwateranim);
        StopRipple(m_inwateranim);
    }
    if (best && !(best->m_pos.y <= 20.0f + m_node->m_matrix.m_matrix[3][1]))
        best = 0;
    if (best) {
        if (appCamera->m_bc == this) {
            zdb::CCamera *cam = zdb::CWorld::m_world->m_camera;

            cam->SetPlayerCanSeeRegions(best->GetRegion());
            CheckForRegionTrigger(best->GetRegion());
        }
        bool b = PolyBit31(best);

        GetNode()->m_socom2_a1_bits.bit6 = b;
        if (GetSeal(best->m_node))
            m_bit8 = true;
    }
    return best;
}
/* (zdb::CWorld::GetAltitudes, OVL1 0x31DEA0: SOCOM II's takes no height -- none of its five callers
 * sets $f12 -- znode.h's since the zdb lane dropped the float; called by name) */

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
    hits = world->GetAltitudes(pos);
    SetAltitudeCharacters(di, chars);
    node->SetActive(active);
    SealProcessAltitude(hits, pos, f);
}
