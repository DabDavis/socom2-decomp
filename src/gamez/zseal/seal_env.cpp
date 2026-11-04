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

extern "C" int rand(void);
zdb::CNode *GetDecalNode(zdb::CNode *node);
/* (SOCOM 1's GetVisualNode__FPQ23zdb5CNode, called where SOCOM 1's HandleBloodDrip calls it; OVL1
 * 0x3D0FD0, unowned) */
zdb::CNode *GetVisualNode(zdb::CNode *node);
/* (zrdr.h's: a block's tag) */
_zrdr *zrdr_findtag(const _zrdr *rdr, const char *tag);
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

/* a material's extension (int_material.cpp's, held in zweapon.h's CMaterialExArray): only the
 * footfall sounds by gait (the names ours) */
struct CMaterialEx {
    char *m_name;   // 0x0 (ReInitDatabaseHandles' footfall anims are named after it)
    unsigned char _4[8 - 4];
    CSnd *m_runsound;   // 0x8
    CSnd *m_walksound;   // 0xc
    CSnd *m_pronesound;   // 0x10
    unsigned char _14[0x3c - 0x14];
    /* int_material.h's flags: only PICKUP here (bit 4) */
    unsigned int m_flags0 : 4;   // 0x3c
    unsigned int m_pickup : 1;   // bit 4: PICKUP
    unsigned int m_flags5 : 27;

    /* (an inline written `? true : false`: the target converts the bit, sltu, before its test --
     * `!= 0`, a plain return or a cast fold it away) */
    bool IsPickup() { return m_pickup ? true : false; }
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

    unsigned char _0[0x5c];
    CAnimsByFP *m_animsbyfp;   // 0x5c (SOCOM 1's m_animgps at 0x60)
};

char *zrdr_findstring(const _zrdr *rdr, const char *tag);
AnimType FindAnimType(const char *name);

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
/* the death's end, once its anim has run (OVL2 0x5A0040, not written yet), the overlay's anims let
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

/* a death by an anim type: the overlay's anims and 0x5A5DA0 first, the SEAL off its ladder and its
 * hold let go, unstunned, the blends cleared to the base and the anim pushed dead (state 8, kind 13)
 * with the death's callback, the push told to the network as event 14 (OVL2 0x5A2490, new in SOCOM
 * II; the name unknown).
 * OPEN, 2 instructions (the size right): 0x5B14E0's miss exactly, inlined -- in the hold's ladder
 * lookup the target loads the ladder list (a0) right after its lui and the index (a1) in the call's
 * delay slot, MW here the index first and the list in the slot. Fixing 0x5B14E0 fixes this. Tried
 * on both (build/scratch/seal/v_climb*.py): the list through an inline member accessor of CMission,
 * a static inline accessor, a CMission::GetLadder(i) wrapper, a pointer local and an index local, the
 * hold through a pointer or a reference local or an inline returning it; a ternary over the index
 * (`climb == &seal->m_climb ? ... : ...`) matches 0x5B14E0 alone -- the inlined copy folds it -- and
 * is not a real spelling. Round 26 (build/scratch/seal/v_5b14e0.py): the release as an inline taking
 * the index, the SEAL or the list, and a Ladder(list, index) inline in either parameter order -- all
 * still the 2 */
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

extern "C" {
/* the stats' events (seal.cpp's): a hit scored, a hit taken, a head shot (SOCOM 1's CSealStats::Hit,
 * WasHit and HeadShot, by these calls; OVL2 0x5458A0, 0x544CF0, 0x5459C0) */
void func_005458A0(CSealStats *s, const CZSealBody *seal, const CZSealBody *other);
void func_00544CF0(CSealStats *s, const CZSealBody *seal);
void func_005459C0(CSealStats *s, const CZSealBody *seal);
}

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
                func_005458A0(&shooter->m_stats, shooter, this);
            func_00544CF0(&m_stats, this);
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
                func_005459C0(&shooter->m_stats, shooter);
            m_stats.m_count[1]++;
        }
    }
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

extern "C" {
/* the body-part ids of the rifle and the pistol (OVL1 data, read as variables) */
extern int data_003DF148;
extern int data_003DF150;
/* a third id, cleared as either weapon is taken (OVL1 data, read as a variable) and the kit's two
 * selections the takes start with (OVL2 0x5C6A40, 0x5C6DA0: the kit lane's range) */
extern int data_003DF160;
void func_005C6A40(CZKit *kit);
void func_005C6DA0(CZKit *kit);
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
void CZSealBody::ChangeToPistolMode(bool hand)
{
    func_005C6DA0(&m_kit);
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

/* the rifle taken: the kit's rifle chosen, the rifle into the right hand, the pistol holstered as
 * gear when asked -- the callback's work written through the SEAL, which the target reloads (the
 * callback inlined keeps the member's address instead) --, the third part none, the rifle the item
 * in hand (OVL2 0x5A70F0; SOCOM 1's ChangeToRifleMode, its kit call ReParentToRifle -- OVL2
 * 0x5C6A40 -- in the same place) */
void CZSealBody::ChangeToRifleMode(bool holster)
{
    func_005C6A40(&m_kit);
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

extern "C" {
/* the blend on top of the body's stack (seal.cpp's tail copy, OVL2 0x555170) */
void *func_00555170(void *stack);
/* the throws' anims (OVL1 data, an anim type's id each, the first short; SOCOM 1's small-data
 * statics) -- the second is tested twice in both builds -- and the two a throw's start adds (the
 * anims a jump cannot start from, below) */
extern short data_003DEBC8;
extern short data_003DEBB8;
extern short data_003DEBA8;
extern short data_003DEBE8;
extern short data_003DEBF0;
extern short data_003DEBD8;
extern short data_003DEBE0;
extern short data_003DEB20;
extern short data_003DEB30;
/* the weapon changes' anims (OVL1 data, an anim type's id each: four blends, three overlays) */
extern short data_003DF0E0;
extern short data_003DF0E8;
extern short data_003DF108;
extern short data_003DF110;
extern short data_003DEC68;
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

/* whether the top blend is a throw's, the start's two too unless told not (OVL2 0x5A7D10; SOCOM 1's
 * IsThrowing, before CanFire in both builds, the same tests) */
bool CZSealBody::IsThrowing(bool started)
{
    short type = ((CZBodyAnimBlend *)func_00555170((char *)this + 0x1c0))->m_animType;

    if (type == data_003DEBC8 || type == data_003DEBB8 || type == data_003DEBA8 || type == data_003DEBE8 ||
        type == data_003DEBF0 || type == data_003DEBD8 || type == data_003DEBE0 || type == data_003DEBB8 ||
        (!started && (type == data_003DEB20 || type == data_003DEB30)))
        return true;
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

    if (a && a->m_anim && (a->m_id == data_003DEC68 || a->m_id == data_003DF0F0 || a->m_id == data_003DF0F8))
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
    seal->m_21c_bit0 = on;
}

/* the toggle the radio menu flips (0x235300, 0x235440): on, bit 0 of 0x21C set and the SEAL of the
 * node at 0x3DC kept; off, both cleared (OVL2 0x5AA620, new in SOCOM II; the names ours) */
extern "C" void func_005AA620(CZSealBody *seal, bool on)
{
    if (on) {
        SetBit21c0(seal, true);
        seal->m_218 = CZSealBody::GetSeal(seal->m_last_reticule.m_node);
    } else {
        SetBit21c0(seal, false);
        seal->m_218 = 0;
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
extern "C" CPickup *func_005A9C70(CZSealBody *seal, CPickup **secondary)
{
    zdb::IntersectStruct *hit = &seal->m_last_reticule;

    if (!hit)
        return 0;
    CMaterialEx *mat = MaterialEx(hit->GetMaterial());

    if (!mat || !mat->IsPickup())
        return 0;
    float d = 9999.0f;
    bool full = !seal->m_kit.HasPickupSlot();
    float zero = 0.0f;
    CPnt3D pos = seal->m_posW + (FIREPOS_SET(seal, zero) ? seal->m_curr_firepos : CPnt3D::zero);
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
extern "C" CPickup *func_005A9F40(CZSealBody *seal, CPickup **secondary)
{
    float d = 9999.0f;
    bool full = !seal->m_kit.HasPickupSlot();
    float zero = 0.0f;
    CPnt3D pos = seal->m_posW + (FIREPOS_SET(seal, zero) ? seal->m_curr_firepos : CPnt3D::zero);
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
extern "C" CPickup *func_005AA110(CZSealBody *seal)
{
    if (!theNetwork.m_9)
        return 0;
    float zero = 0.0f;
    CPnt3D pos = seal->m_posW + (FIREPOS_SET(seal, zero) ? seal->m_curr_firepos : CPnt3D::zero);
    CPickup *bomb = CPickup::GetClosest(&pos, 20.0f, CPickup::PU_MPBOMB, 0, true);

    if (bomb) {
        CPnt3D d;

        pos.Sub(bomb->GetPosW(), &d);
        if (d.x * d.x + d.y * d.y + d.z * d.z < 400.0f)
            return bomb;
    }
    return 0;
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
/* the CQB deaths, from in front and from behind (OVL1 data, anim types; SOCOM 1's small-data pair) */
extern AnimType data_003DEE30;
extern AnimType data_003DEE38;
}

/* the death a CQB attack gives its victim: from behind or in front, by the attacker's offset against
 * the victim's facing, flattened (OVL2 0x5A9420; SOCOM 1's FindDeathAnimTypeCQB, after CanUseRestraint
 * in both builds, the same arithmetic).
 * OPEN, 10 instructions (the size right since round 23): the target loads both nodes before the
 * facing's three stores (the last in the call's delay slot); MW here loads this node after them. The
 * negation is NegateInto(&fwd, &fwd) -- the out-parameter form folds the loads into sp offsets and keeps
 * only &fwd.y in a register, as the target. Tried since: the nodes in locals (either order), the
 * positions in pointer locals, the offset computed first, both points assigned, GetNode(). Before: the
 * offset computed first, the facing assigned after it, y through a pointer, Negate(fwd.y). */
AnimType CZSealBody::FindDeathAnimTypeCQB(CZSealBody *victim)
{
    CPnt3D fwd(*(const CPnt3D *)victim->m_node->m_matrix.m_matrix[2]);
    CPnt3D d = *(const CPnt3D *)m_node->m_matrix.m_matrix[3] - *(const CPnt3D *)victim->m_node->m_matrix.m_matrix[3];

    NegateInto(&fwd, &fwd);
    fwd.y = 0.0f;
    d.y = 0.0f;
    if (Dot(d, fwd) < 0.0f)
        return data_003DEE38;
    return data_003DEE30;
}

extern "C" {
/* (OVL2 0x5B4340, not written yet: a test of the SEAL's state against a kind -- CanTraverse and
 * PreTick ask it too; the name unknown) */
bool func_005B4340(CZSealBody *seal, int kind);
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
/* the decals rdr (OVL1 bss, the weapons' 0x437D00) and a material's index by name (OVL1 0x2DE9E0,
 * SOCOM 1's CMaterialExArray::ResolveMaterialIndex -- not named) */
extern _zrdr *data_00437D00;
int func_002DE9E0(const char *name);
}
/* (zrdr.h's, which this unit cannot include beside zanim.h's _zrdr) */
bool zrdr_findreal(const _zrdr *rdr, const char *tag, float *out, int n);

/* the decal textures: the footsteps' by material, the blood drips' with their size ranges (OVL2
 * 0x5A4CF0; SOCOM 1's, the arrays SOCOM II's). Each tag found in the test, assigned there (the
 * target tests v0 with the copy in the delay slot), the locals declared first in tools/perm.py's
 * order, the counter, the count and the item one variable each for both loops.
 * OPEN, 10 instructions (the size right, 2 registers): in the second block the target gives the blood
 * tag s0 and the loop's count s5; here they swap (the tag s5, the count's second web s0). The first
 * block is the target's. Tried: perm.py over the declarations (best 10 with the shared counter, 19
 * with separate second-loop variables or a separate count; 1828-2328 steps each), the tag declared in
 * the test or tested apart, the decals rdr's local reused for the blood tag, the steps tag reused. */
void CZSealBody::InitDecalHandles()
{
    char *texture;
    int i;
    _zrdr *steps;
    _zrdr *blood;
    char *material;
    int n;
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
            index = func_002DE9E0(material);
            ((zdb::CTexHandle **)data_0044D550)[index] = zdb::CWorld::GetTexHandle(texture);
        }
    }
    if ((blood = zrdr_findtag(rdr, "BLOOD_DRIP_DECALS")) != 0) {
        CZDecalDef def;

        def.m_decalminsize = 0;
        def.m_decalmaxsize = 0;
        def.m_texture = 0;
        n = blood->Count();
        m_blooddriptexhandles.reserve(n);
        for (i = 0; i < n; i++) {
            item = ItemAt(blood, i);
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

extern "C" {
/* one of the six values started, with a kind (OVL2 0x5A54D0, not written yet) */
void func_005A54D0(CZSealBody *seal, unsigned char i, int kind);
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
            func_005A54D0(seal, i, 5);
            break;
        case 0:
            func_005A54D0(seal, i, 3);
            break;
        default:
            func_005A54D0(seal, i, 0);
            break;
        }
    }
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

extern "C" {
/* the look anims: four sources (OVL2 data), what each is built into and the turn tables ApplyLookAnim
 * reads (OVL1 bss) -- the head's, then up the spine -- and their builder (OVL2 0x5AE980, not written
 * yet; the names unknown) */
extern unsigned char data_00650820[];
extern unsigned char data_006507B0[];
extern unsigned char data_00650740[];
extern unsigned char data_006506D0[];
extern unsigned char data_0044C7A0[];
extern unsigned char data_0044C860[];
extern unsigned char data_0044C920[];
extern unsigned char data_0044C9E0[];
extern CPnt3D data_0044CAA0[];
extern CPnt3D data_0044CB30[];
extern CPnt3D data_0044CBC0[];
extern CPnt3D data_0044CC50[];
void func_005AE980(void *src, void *anim, CPnt3D *table);
/* the eyes' limits: up, down and sideways (OVL1 data; SOCOM 1's were constants, 8, -14 and 17
 * degrees) */
extern float data_0044C2D0;
extern float data_0044C2D4;
extern float data_0044C2D8;
}

/* a look anim's turn for a yaw and a pitch (SOCOM 1's GetLookQuat__FP6CPnt3DffP5CQuat; OVL2 0x5AE730,
 * not written yet) */
void GetLookQuat(CPnt3D *table, float yaw, float pitch, CQuat *q);

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
 * constants made globals and the yaw's two equal arms one; `dir` unread in both builds) */
bool CZSealBody::SetEyeLookDir(float pitch, float yaw, const CPnt3D *dir, float rate)
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

extern "C" {
/* (the kit's item at an index let go is CZKit::RemoveItemAndAmmo, zseal.h's; OVL2 0x5B9BB0, the kit
 * lane's) the ground under a point within a depth (OVL1 0x2A2EB0), a weapon readied (OVL2 0x57D540,
 * seal_body.cpp's) and the item it takes (OVL1 data 0x3DEB48, a short) */
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
        m_kit.RemoveItemAndAmmo(i);
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
/* (OVL2 0x5A7DE0 is CZSealBody::CanFire, zseal.h's) the body's current blend (seal.cpp's tail, OVL2
 * 0x555170) */
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
