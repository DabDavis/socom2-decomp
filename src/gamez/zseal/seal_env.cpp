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
    unsigned char _0[8];
    CSnd *m_runsound;   // 0x8
    CSnd *m_walksound;   // 0xc
    CSnd *m_pronesound;   // 0x10
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

/* where a hit lands (SOCOM 1's DAMAGE_LOCATION, a byte: only the name matters here) */
enum DAMAGE_LOCATION { DAMAGE_LOCATION_0 = 0, DAMAGE_LOCATION_LAST = 255 };

/* an animation set's anims by the item in hand (anim.cpp's CAnimsByFP: only the setter) */
class CAnimsByFP {
public:
    void SetPistolAnim(AnimType rifle, AnimType pistol);
};

/* a list of anim types (SOCOM 1's vector<AnimType>): SOCOM II's MSL runs a 2-byte element on
 * __vector_pod<unsigned short> (the insert's name, OVL2 0x5B68E0), and an element read back is
 * the signed type again (lh) */
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
        seal->m_218 = CZSealBody::GetSeal(seal->m_3dc);
    } else {
        SetBit21c0(seal, false);
        seal->m_218 = 0;
    }
}

/* the fire point's offset, none while it is unset (an inline: its three compares with zero) */
static inline const CPnt3D *FireOffset(const CZSealBody *seal)
{
    if (seal->m_curr_firepos.x == 0.0f && seal->m_curr_firepos.y == 0.0f && seal->m_curr_firepos.z == 0.0f)
        return &CPnt3D::zero;
    return &seal->m_curr_firepos;
}

/* online, the MP bomb lying within 20 of the SEAL's fire point, if any (OVL2 0x5AA110: TickPickups
 * 0x593DA0 asks it; new in SOCOM II, the name ours).
 * OPEN, 0x128 against 0x130, ~12 instructions: (1) the target compares each field with zero as
 * `c.eq.s field,zero`, MW here always puts the literal first (`c.eq.s zero,field`) -- `x == 0.0f`,
 * `0.0f == x` and `!=` alike; (2) after the three tests the target branches over the offset's
 * &field block to the &zero block (`b`), the early-return inline here falls into &zero (the `==`
 * forms) or inverts the last test (`!=`); the ternary materialises the &&; (3) the network test's
 * return 0 is its own `b epilogue; move v0,zero` in the target, merged with the last return here */
extern "C" CPickup *func_005AA110(CZSealBody *seal)
{
    if (!theNetwork.m_9)
        return 0;
    CPnt3D pos = seal->m_posW + *FireOffset(seal);
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
/* the CQB deaths, from in front and from behind (OVL1 data, anim types; SOCOM 1's small-data pair) */
extern AnimType data_003DEE30;
extern AnimType data_003DEE38;
}

/* the death a CQB attack gives its victim: from behind or in front, by the attacker's offset against
 * the victim's facing, flattened (OVL2 0x5A9420; SOCOM 1's FindDeathAnimTypeCQB, after CanUseRestraint
 * in both builds, the same arithmetic).
 * OPEN, 0xDC against 0xD8: the target loads both nodes before the facing's three stores (the last in
 * the call's delay slot) and holds only &fwd.y in a register; MW here loads this node after the stores
 * and keeps &fwd.z in a register too. Tried: the nodes in locals first, the offset computed first, the
 * facing assigned after it, y through a pointer, Negate(fwd.y). */
AnimType CZSealBody::FindDeathAnimTypeCQB(CZSealBody *victim)
{
    CPnt3D fwd(*(const CPnt3D *)victim->m_node->m_matrix.m_matrix[2]);
    CPnt3D d = *(const CPnt3D *)m_node->m_matrix.m_matrix[3] - *(const CPnt3D *)victim->m_node->m_matrix.m_matrix[3];

    fwd.x = -fwd.x;
    fwd.y = -fwd.y;
    fwd.z = -fwd.z;
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
