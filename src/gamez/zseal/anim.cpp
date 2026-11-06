/* anim.cpp -- a SEAL's animation sets: AnimSet, its anims by the item in hand (CAnimsByFP), the
 * per-type records and the blendables (SOCOM 1's unit, __sinit_anim.cpp, linked between
 * seal_weapon.cpp and remote_control.cpp; SOCOM 1's DWARF describes AnimSet and its pieces, no
 * function). OVL2 0x5E0030-0x5E622F, in SOCOM 1's order: AddBlendable, SetMaxMotion, DefineFPAnims,
 * CAnimsByFP's accessors, BaseAnim, InitializeAnimNames, Load, CAnimAuxData::Get, Inherit, the
 * lookups SOCOM II added, AnimId, Load(_zrdr *), ... CBlendable's, the template tail. So far the
 * small accessors and the three name lookups (whose map find and string compares, OVL2's copies
 * 0x58D090, 0x58D250, 0x58D740, the unit's tail emits); round 52 AddBlendable and SetMaxMotion,
 * the unit's first two (CBlendable's AddBlendable and MaxTrans1 inlined). Defined in reverse of
 * their image order (-inline deferred). */

#include "zseal.h"
#include "../zrdr/zrdr.h"

#include <map>
#include <string.h>
#include <string>
#include <vector>

/* an anim type's variants by the item in hand (SOCOM 1's DWARF: ANIMTYPE_GROUP, 8 bytes) */
struct ANIMTYPE_GROUP {
    AnimType m_rifle_type;   // 0x0
    AnimType m_pistol_type;   // 0x2
    AnimType m_rifle_fp_type;   // 0x4
    AnimType m_pistol_fp_type;   // 0x6
};

/* an animation set's anims by the item in hand (SOCOM 1's DWARF: CAnimsByFP, a group per anim
 * type -- 312 of them in SOCOM II, 300 in SOCOM 1) */
class CAnimsByFP {
public:
    enum { MAX_TYPES = 312 };

    /* (inline: DefineFPAnims expands it -- the groups allocated without a file or line) */
    CAnimsByFP()
    {
        m_animgroups = (ANIMTYPE_GROUP *)_malloc(MAX_TYPES * sizeof(ANIMTYPE_GROUP), 0, 0);
        Clear();
    }
    /* (inline: AnimSet's destructor expands it -- the groups freed without a file or line) */
    ~CAnimsByFP() { _free(m_animgroups, 0, 0); }
    /* every group's anims none (SOCOM 1's Clear) */
    void Clear()
    {
        ANIMTYPE_GROUP *g = m_animgroups;

        for (int i = 0; i < MAX_TYPES; i++, g++) {
            g->m_rifle_type = animTypeUndefined;
            g->m_pistol_type = animTypeUndefined;
            g->m_rifle_fp_type = animTypeUndefined;
            g->m_pistol_fp_type = animTypeUndefined;
        }
    }
    void SetPistolAnim(AnimType rifle, AnimType pistol);
    /* a rifle anim type's first-person variants set (inline: AnimSet's setters expand them) */
    void SetRifleFPAnim(AnimType rifle, AnimType fp)
    {
        m_animgroups[rifle].m_rifle_type = rifle;
        m_animgroups[rifle].m_rifle_fp_type = fp;
    }
    void SetPistolFPAnim(AnimType rifle, AnimType fp)
    {
        m_animgroups[rifle].m_rifle_type = rifle;
        m_animgroups[rifle].m_pistol_fp_type = fp;
    }
    AnimType PistolFPAnim(AnimType type) const;
    AnimType RifleFPAnim(AnimType type) const;
    AnimType PistolAnim(AnimType type) const;

    ANIMTYPE_GROUP *m_animgroups;   // 0x0
};

/* a blend's anims, four slots (SOCOM 1's DWARF: CBlendable, BLENDABLE_ANIM) */
struct BLENDABLE_ANIM {
    float m_trans0;   // 0x0
    float m_trans1;   // 0x4
    AnimType m_animtype;   // 0x8 (-1 ends the slots in use)
};

class CBlendable {
public:
    /* (inline: AnimSet's constructor and destructor expand them, each its own slots cleared) */
    CBlendable() { Clear(); }
    ~CBlendable() { Clear(); }
    /* every slot zeroed, none in use (a pointer stepped with the count: the target's two stores
     * from the blend's base, then two from it 0x18 on -- indexed, MW folds all four) */
    void Clear()
    {
        memset(m_blendable, 0, sizeof(m_blendable));
        BLENDABLE_ANIM *b = m_blendable;

        for (int i = 0; i < 4; i++, b++)
            b->m_animtype = animTypeUndefined;
    }
    bool Contains(AnimType type);
    /* the fastest anim's second speed, over the slots up to the first unused (SOCOM 1's MaxTrans1,
     * out of line there, 0x2CD640; inline here: SetMaxMotion expands it eight times) */
    float MaxTrans1()
    {
        float max = 0.0f;
        BLENDABLE_ANIM *b = m_blendable;

        for (int i = 0; i < 4; i++, b++) {
            if (b->m_animtype == animTypeUndefined)
                break;
            if (b->m_trans1 > max)
                max = b->m_trans1;
        }
        return max;
    }

    /* a type's speeds set in its slot, or in the first free one (SOCOM 1's AddBlendable, which filled
     * only a free slot; inline here: AnimSet::AddBlendable expands it in each case) */
    void AddBlendable(float t0, float t1, AnimType type)
    {
        BLENDABLE_ANIM *b = m_blendable;

        for (int i = 0; i < 4; i++, b++) {
            if (b->m_animtype == (short)type) {
                b->m_trans0 = t0;
                b->m_trans1 = t1;
                return;
            }
            if (b->m_animtype == animTypeUndefined) {
                b->m_trans0 = t0;
                b->m_trans1 = t1;
                b->m_animtype = type;
                return;
            }
        }
    }

    BLENDABLE_ANIM m_blendable[4];   // 0x0
};

/* a record's variant group (new in SOCOM II; the names ours): its kind, its count and, for fewer
 * than two, its one anim in the halfword's upper 14 bits -- else `count` pairs follow it */
struct ANIM_VARIANT {
    int m_anim;   // 0x0
    float m_weight;   // 0x4: its share of the draw
};

struct ANIM_VARIANT_GROUP {
    void SetState(unsigned int s) { m_state = s; }

    unsigned char m_kind;   // 0x0
    unsigned char m_count;   // 0x1
    unsigned short m_state : 2;   // 0x2: a state the record's SetState/ClearState mark it with
    unsigned short m_anim : 14;
};

/* the cursor over a group's pairs (the walks step the group pointer itself) */
#define Pair(g) ((ANIM_VARIANT *)(g))
#define Next(g) ((ANIM_VARIANT_GROUP *)(Pair(g) + 1))

/* an anim type's record in a set (new in SOCOM II: SOCOM 1's set kept two short vectors, the ids
 * and the base types; SOCOM II keeps a record per type, 12 bytes, from a pool of 600 whose new[]
 * takes this constructor and destructor; the names ours). Its variants: m_count groups packed at
 * m_8, each a header {kind, count, an anim in the halfword's upper 14 bits} and, for two or more,
 * that many {anim, weight} pairs after it */
class CAnimTypeRec {
public:
    CAnimTypeRec();
    ~CAnimTypeRec();
    /* the record made empty (inline: the constructor and the destructor both expand it) */
    void Clear()
    {
        m_id = -1;
        m_base = animTypeUndefined;
        SetBit0(false);
        SetBit1(false);
        m_count = 0;
        SetState(0);
        m_8 = 0;
    }
    void SetBit0(bool b) { m_bit0 = b; }
    void SetBit1(bool b) { m_bit1 = b; }
    void SetState(unsigned int s) { m_state = s; }
    /* (tested through it, the byte is loaded again for the store after) */
    unsigned int GetState() const { return m_state; }
    /* the variant groups' count (inline: some walks' loop test re-reads it through it) */
    unsigned int Count() const { return m_count; }
    /* the groups holding an anim marked with a state, whether one was (OVL2 0x5E5500); every
     * group of a state cleared, how many were (0x5E5600); the group holding an anim, or 0
     * (0x5E56D0); an anim's index in the group of a kind (0x5E5790); the anim at an index of a
     * kind's group (0x5E5880); an anim of a kind drawn by weight (0x5E5970) */
    int MarkState(int state, int anim);
    int ClearState(int state);
    /* the same over every record in use, the pool made first (OVL2 0x5E5B10, 0x5E5BF0:
     * CScriptArchive::Register and Unregister call them) */
    static int MarkStateAll(int state, int anim);
    /* a record from the pool, the pool made first, or 0 when it is used up (inline: the loader expands it twice) */
    static CAnimTypeRec *Alloc()
    {
        if (m_used >= 600)
            return 0;
        if (!m_pool)
            m_pool = new CAnimTypeRec[600];
        return &m_pool[m_used++];
    }
    static void ClearStateAll(int state);
    ANIM_VARIANT_GROUP *GetGroup(int anim);
    int GetVariantIndex(int kind, int anim);
    int GetVariant(int kind, int index);
    int PickVariant(int kind);

    short m_id;   // 0x0: the anim (AnimId)
    AnimType m_base;   // 0x2: the base type (BaseAnim)
    unsigned char m_count;   // 0x4: variant groups at m_8
    unsigned char m_bit0 : 1;   // 0x5
    unsigned char m_bit1 : 1;
    unsigned char m_state : 2;   // the state for a record without groups
    unsigned char m_5_4 : 4;
    unsigned char _6[2];
    void *m_8;   // 0x8: the variant groups

    /* the records, 600 made at once (OVL1 bss 0x44C790), and how many are in use (0x44C798) --
     * the loader hands them out; the names ours */
    static CAnimTypeRec *m_pool;
    static unsigned int m_used;
};

/* a set's top speeds (SOCOM 1's DWARF: ANIMSET_PARAMS, thirteen floats; seal_cfg.cpp's view) */
struct ANIMSET_PARAMS {
    float m_stand_max_fwd;   // 0x0
    float m_stand_max_back;   // 0x4
    float m_stand_max_rt;   // 0x8
    float m_stand_max_left;   // 0xc
    float m_crouch_max_fwd;   // 0x10
    float m_crouch_max_back;   // 0x14
    float m_crouch_max_rt;   // 0x18
    float m_crouch_max_left;   // 0x1c
    float m_prone_max_fwd;   // 0x20
    float m_prone_max_back;   // 0x24
    float m_prone_max_rt;   // 0x28
    float m_prone_max_left;   // 0x2c
    float m_ladder_updn;   // 0x30
};

/* a SEAL's animations (SOCOM 1's DWARF: AnimSet, 0x84 bytes there) -- only what is written here:
 * SOCOM II's string name is 12 bytes, and its two short vectors became one of records */
class AnimSet {
public:
    AnimType BaseAnim(AnimType type);
    bool TypeBit1(AnimType type);
    int GetVariantIndex(AnimType type, int kind, int anim);
    int PickVariant(AnimType type, int kind);
    /* a type's motion script: a variant of a kind drawn when the type has groups (the archive its
     * group's state names), else its own anim's (the record's state naming the archive), or 0 --
     * and the same for kind 0 (OVL2 0x5E3C20, 0x5E3D90: the SEAL's tick, the ladder and the strafe
     * lookups call the second; new in SOCOM II, the names ours) */
    CZSIScriptCommon *GetScript(AnimType type, int kind);
    CZSIScriptCommon *GetScript(AnimType type);
    /* the script of a type's variant by kind and index, or 0 (OVL2 0x5E3B20: seal_body.cpp's
     * 0x57DCC0 and 0x5880E0 call it; new in SOCOM II, the name ours) */
    CZSIScriptCommon *GetVariantScript(AnimType type, int kind, int index);
    int AnimId(AnimType type) const;
    /* a rifle anim type's variant set, both types' anims there: the group's (CAnimsByFP's) and the
     * variant's base type (OVL2 0x5E1CF0, 0x5E1BF0, 0x5E1AF0; DefineFPAnims calls them -- new in
     * SOCOM II, the names ours) */
    void SetPistolAnim(AnimType rifle, AnimType pistol);
    void SetRifleFPAnim(AnimType rifle, AnimType fp);
    void SetPistolFPAnim(AnimType rifle, AnimType fp);
    /* the set's top speeds, standing and crouched, from its move blends (SOCOM 1's; OVL2 0x5E0450) */
    void SetMaxMotion();
    /* an anim's speeds put in the move blend of its type (SOCOM 1's; OVL2 0x5E0030) */
    void AddBlendable(int id, float t0, float t1);
    /* a set closed (SOCOM 1's Close freed its anims; SOCOM II's CloseAll inlines it to nothing --
     * the walk is all that is left) and every set closed (SOCOM 1's; CGame::StopEngine calls it) */
    void Close() {}
    static void CloseAll();
    /* a set's anims taken from another (SOCOM 1's; OVL2 0x5E3740) */
    void Inherit(AnimSet *base);
    /* every set read from animset.rdr, again over the same sets for reload (SOCOM 1's Load; OVL2
     * 0x5E30F0) and one set from its record (SOCOM 1's Load(_zrdr *); OVL2 0x5E3FF0) */
    static bool Load(bool reload);
    bool Load(_zrdr *rdr, bool reload);
    /* the rifle anim types' variants (SOCOM 1's DefineFPAnims; OVL2 0x5E0690) */
    void DefineFPAnims();
    /* a type's record's bit 1 set, when it has one (inline: DefineFPAnims expands it) */
    void SetTypeBit1(AnimType type)
    {
        if (type >= 0 && type < m_types.size()) {
            CAnimTypeRec *rec = m_types[type];

            if (rec)
                rec->SetBit1(true);
        }
    }
    /* (seal_env.cpp's: the death and flinch tables; SOCOM 1's) */
    static void InitDeathAndFlinchAnimations();
    static void ReadDeathAndFlinchAnims();
    static std::vector<AnimType> deathanim[6][3];
    /* the sets' array, new AnimSet[n] (OVL1 bss 0x44D418; the name ours -- SOCOM 1 new'd each) */
    static AnimSet *m_sets;
    /* the record list sized, the new ones null (inline: the loader's) */
    void ResizeTypes(unsigned int n)
    {
        CAnimTypeRec *none = 0;

        m_types.resize(n, (const unsigned int &)none);
    }
    /* a type's record made and filed, the list grown first to reach it (inline: the loader expands
     * it for a type's anim and for its modes) */
    static void NewRec(AnimSet *set, short anim, int type, bool reload)
    {
        set = set;
        if (!reload) {
            int g = (AnimType)type;

            while (g >= set->m_types.size())
                set->m_types.push_back(0);
        }
        int t = (AnimType)type;
        CAnimTypeRec *rec = CAnimTypeRec::Alloc();

        if (rec) {
            rec->m_id = anim;
            rec->m_base = (AnimType)type;
            rec->SetBit0(false);
            rec->m_count = 1;
            set->m_types[t] = rec;
        }
    }
    /* the set's name (out of line in the SEALs' units, OVL2 0x578940; the name ours) -- five inline
     * levels high: the depth the target's Find needs (kit round 38, fitted here, not read off the
     * image -- no function calls a piece below GetName or c_str): with Find's whole tree over five,
     * 0x5E3940 inlines Find one level and calls every piece, as the target does; the chain is
     * GetName's or c_str's in the original (include/string's c_str is two levels: the MSL lane's) */
    std::string &Name0() { return m_name; }
    std::string &Name1() { return Name0(); }
    std::string &Name2() { return Name1(); }
    std::string &Name3() { return Name2(); }
    std::string &GetName() { return Name3(); }
    /* a set by its name in the list, or 0 (SOCOM 1's Find; inlined one level in 0x5E3940, the list's
     * iterator pieces called -- OVL2 0x5788C0-0x578980, the SEALs' copies) */
    static AnimSet *Find(const char *name)
    {
        std::vector<AnimSet *>::iterator i;

        for (i = list.begin(); i != list.end(); i++)
            if (!strcmp(name, (*i)->GetName().c_str()))
                return *i;
        return 0;
    }

    /* every set (SOCOM 1's AnimSet::List, a pointer vector; OVL1 bss 0x44D430) */
    static std::vector<AnimSet *> list;

    AnimSet();
    ~AnimSet();

    ANIMSET_PARAMS m_aparams;   // 0x0
    bool m_ready;   // 0x34
    std::string m_name;   // 0x38
    std::vector<CAnimTypeRec *> m_types;   // 0x44: a record per anim type, or 0
    std::vector<std::string> m_included;   // 0x50 (SOCOM 1's, at 0x54 there)
    CAnimsByFP *m_animsbyfp;   // 0x5c (SOCOM 1's m_animgps at 0x60)
    /* the move blends (SOCOM 1's pointers at 0x64-0x80, held whole in SOCOM II) */
    CBlendable m_move_anims;   // 0x60
    CBlendable m_moveback_anims;   // 0x90
    CBlendable m_strafeL_anims;   // 0xc0
    CBlendable m_strafeR_anims;   // 0xf0
    CBlendable m_crouchmove_anims;   // 0x120
    CBlendable m_crouchmoveback_anims;   // 0x150
    CBlendable m_crouchstrafeL_anims;   // 0x180
    CBlendable m_crouchstrafeR_anims;   // 0x1b0
};

/* every record in use cleared of a state (OVL2 0x5E5BF0; the count kept but not returned) */
void CAnimTypeRec::ClearStateAll(int state)
{
    int count = 0;

    if (!m_pool)
        m_pool = new CAnimTypeRec[600];
    CAnimTypeRec *rec = m_pool;

    for (unsigned int i = 0; i < m_used; i++, rec++)
        count += rec->ClearState(state);
}

/* the records in use holding an anim marked with a state, how many were (OVL2 0x5E5B10) */
int CAnimTypeRec::MarkStateAll(int state, int anim)
{
    int count = 0;

    if (!m_pool)
        m_pool = new CAnimTypeRec[600];
    CAnimTypeRec *rec = m_pool;

    for (unsigned int i = 0; i < m_used; i++, rec++)
        count += rec->MarkState(state, anim);
    return count;
}

/* whether a blend holds an anim type: its slots up to the first unused (SOCOM 1's; OVL2 0x5E5AB0) */
bool CBlendable::Contains(AnimType type)
{
    for (int i = 0; i < 4; i++) {
        /* read into a local: compared straight, MW puts the parameter first in the bne */
        AnimType t = m_blendable[i].m_animtype;

        if (t == animTypeUndefined)
            return false;
        if (t == type)
            return true;
    }
    return false;
}

/* an anim of a kind: the group's one anim, or one drawn by weight (the last when the draw passes
 * them all); the record's own anim when it has no groups, -1 for no group of the kind (OVL2
 * 0x5E5970) */
int CAnimTypeRec::PickVariant(int kind)
{
    ANIM_VARIANT_GROUP *g = (ANIM_VARIANT_GROUP *)m_8;

    if (!g)
        return m_id;
    for (unsigned int i = 0; i < Count(); i++) {
        int n = g->m_count;

        if (g->m_kind == kind) {
            if (n < 2)
                return g->m_anim;
            float sum = 0.0f;

            g++;
            float r = (float)rand() * 4.656613e-10f;

            for (unsigned int j = 0; j < n; j++, g = Next(g)) {
                sum += Pair(g)->m_weight;
                if (!(r <= sum))
                    continue;
                return Pair(g)->m_anim;
            }
            return Pair(g)[-1].m_anim;
        }
        if (n < 2)
            g = (ANIM_VARIANT_GROUP *)((int *)g + 1);
        else
            g = (ANIM_VARIANT_GROUP *)((int *)g + (n * 2 + 1));
    }
    return -1;
}

/* the anim at an index of a kind's group, -1 past its end; without groups the record's own anim
 * at index 0 (OVL2 0x5E5880) */
int CAnimTypeRec::GetVariant(int kind, int index)
{
    ANIM_VARIANT_GROUP *g = (ANIM_VARIANT_GROUP *)m_8;

    if (!g)
        return index == 0 ? m_id : -1;
    for (unsigned int i = 0; i < Count(); i++) {
        int n = g->m_count;

        if (g->m_kind == kind) {
            if (n < 2)
                return index == 0 ? g->m_anim : -1;
            g++;
            for (unsigned int j = 0; j < n; j++, g = Next(g))
                if (j == index)
                    return Pair(g)->m_anim;
            return -1;
        }
        if (n < 2)
            g = (ANIM_VARIANT_GROUP *)((int *)g + 1);
        else
            g = (ANIM_VARIANT_GROUP *)((int *)g + (n * 2 + 1));
    }
    return -1;
}

/* an anim's place in the group of a kind (0 for a group of one), -1 when it is not there (OVL2
 * 0x5E5790) */
int CAnimTypeRec::GetVariantIndex(int kind, int anim)
{
    ANIM_VARIANT_GROUP *g = (ANIM_VARIANT_GROUP *)m_8;

    if (!g)
        return -1;
    for (unsigned int i = 0; i < Count(); i++) {
        int n = g->m_count;

        if (g->m_kind == kind) {
            if (n < 2 && g->m_anim == anim)
                return 0;
            /* one cursor: the target walks the pairs in the group pointer's own register (a
             * pointer of their own takes another) */
            g++;
            for (unsigned int j = 0; j < n; j++, g = Next(g))
                if (Pair(g)->m_anim == anim)
                    return j;
            return -1;
        }
        if (n < 2)
            g = (ANIM_VARIANT_GROUP *)((int *)g + 1);
        else
            g = (ANIM_VARIANT_GROUP *)((int *)g + (n * 2 + 1));
    }
    return -1;
}

/* the group holding an anim, of any kind, or 0 (OVL2 0x5E56D0). A const walk, the group cast back
 * at each pass's top (round 43): a plain copy `group = g` is propagated into the single test's
 * `return group` (g unchanged there), so MW returns the cursor and coalesces it with v0 -- 18 off;
 * any conversion on the copy (const either side, a void * or int cast) keeps the target's cursor in
 * t1 and the group in v0 */
ANIM_VARIANT_GROUP *CAnimTypeRec::GetGroup(int anim)
{
    const ANIM_VARIANT_GROUP *g = (const ANIM_VARIANT_GROUP *)m_8;

    if (!g)
        return 0;
    for (unsigned int i = 0; i < Count(); i++) {
        ANIM_VARIANT_GROUP *group = (ANIM_VARIANT_GROUP *)g;
        int n = g->m_count;

        if (n < 2) {
            if (group->m_anim == anim)
                return group;
            g++;
        } else {
            g++;
            for (unsigned int j = 0; j < n; j++, g = Next(g))
                if (Pair(g)->m_anim == anim)
                    return group;
        }
    }
    return 0;
}

/* every group of a state cleared (the record's own state when it has none), how many were (OVL2
 * 0x5E5600) */
int CAnimTypeRec::ClearState(int state)
{
    ANIM_VARIANT_GROUP *g = (ANIM_VARIANT_GROUP *)m_8;
    int count = 0;

    if (!g) {
        if (state == GetState()) {
            SetState(0);
            count++;
        }
        return count;
    }
    for (unsigned int i = 0; i < Count(); i++) {
        int n = g->m_count;

        if (g->m_state == state) {
            g->SetState(0);
            count++;
        }
        if (n < 2)
            g = (ANIM_VARIANT_GROUP *)((int *)g + 1);
        else
            g = (ANIM_VARIANT_GROUP *)((int *)g + (n * 2 + 1));
    }
    return count;
}

/* the group holding an anim marked with a state (the record's own state for its own anim when it
 * has no groups -- true either way then), whether one was (OVL2 0x5E5500) */
int CAnimTypeRec::MarkState(int state, int anim)
{
    ANIM_VARIANT_GROUP *g = (ANIM_VARIANT_GROUP *)m_8;

    if (!g) {
        if (anim == m_id)
            SetState(state);
        return true;
    }
    for (unsigned int i = 0; i < m_count; i++) {
        ANIM_VARIANT_GROUP *group = g;
        int n = g->m_count;

        if (n < 2) {
            if (g->m_anim == anim) {
                g->SetState(state);
                return true;
            }
            g++;
        } else {
            g++;
            for (unsigned int j = 0; j < n; j++, g = Next(g))
                if (Pair(g)->m_anim == anim) {
                    group->SetState(state);
                    return true;
                }
        }
    }
    return false;
}

/* (OVL2 0x5E5350: the loader's new AnimSet[n], 0x1E0 bytes each, passes it to
 * __construct_new_array) */
AnimSet::AnimSet() : m_ready(false)
{
    m_animsbyfp = 0;
}

/* (OVL2 0x5E50E0: new[]'s destructor, the anims by item deleted, then the members' -- the blends
 * cleared again, the name list, the records, the name) */
AnimSet::~AnimSet()
{
    delete m_animsbyfp;
    m_animsbyfp = 0;
}

/* every set closed (SOCOM 1's CloseAll; OVL2 0x5E5070) */
void AnimSet::CloseAll()
{
    /* postfix: the class iterator's temporary gives the target's rotated loop (++i does not) */
    for (std::vector<AnimSet *>::iterator i = list.begin(); i != list.end(); i++)
        (*i)->Close();
}

/* the anim names' maps (OVL1 bss; an OVL1 initialiser, 0x400870, builds them): a name's anim type
 * (FindAnimType's, as SOCOM 1's newNamesMap) and a name's variant kind (0x5E4F20's) */
extern std::map<std::string, int> data_0044C420;
extern std::map<std::string, int> data_0044C460;
extern int numAnimTypes;
extern int loadCount;
extern int data_0066B808;
extern const char *data_00661060[];
extern std::map<std::string, int> data_0044C440;
extern char data_0044C470[];

/* an anim type by name, -1 for none or an unknown one (SOCOM 1's FindAnimType: CTPlay::Parse
 * calls it; OVL2 0x5E5000). The test around the lookup, not an early return: `if (!name) return
 * -1;` lays the -1 block out first */
AnimType FindAnimType(const char *name)
{
    if (name) {
        std::map<std::string, int>::iterator i = data_0044C420.find(name);
        std::map<std::string, int>::iterator e = data_0044C420.end();

        if (i != e)
            return (AnimType)(*i).second;
    }
    return animTypeUndefined;
}

/* the same lookup, word for word (OVL2 0x5E4F90): InitializeAnimNames (0x5E1DF0) calls it for each
 * of its ~240 anim type globals, where SOCOM 1's inlined FindAnimType's find 157 times (the name
 * ours) */
AnimType LookupAnimType(const char *name)
{
    if (name) {
        std::map<std::string, int>::iterator i = data_0044C420.find(name);

        if (i != data_0044C420.end())
            return (AnimType)(*i).second;
    }
    return animTypeUndefined;
}

/* a variant kind by name ("ready", "suspicious", "conversation", ...), -1 for none (OVL2 0x5E4F20;
 * seal_anim.cpp's 0x574C30 keeps five; new in SOCOM II, the name ours) */
int FindAnimKind(const char *name)
{
    if (name) {
        std::map<std::string, int>::iterator i = data_0044C460.find(name);
        std::map<std::string, int>::iterator e = data_0044C460.end();

        if (i != e)
            return (*i).second;
    }
    return -1;
}

/* (OVL2 0x5E3F80, after the destructor: the pool's element constructor) */
CAnimTypeRec::CAnimTypeRec()
{
    Clear();
}

/* (OVL2 0x5E3EE0) */
CAnimTypeRec::~CAnimTypeRec()
{
    Clear();
}

/* a type's anim, or -1 (SOCOM 1's AnimId, read from the type's record; OVL2 0x5E3E80) */
int AnimSet::AnimId(AnimType type) const
{
    if (type >= 0 && type < m_types.size()) {
        CAnimTypeRec *rec = m_types[type];

        if (rec)
            return rec->m_id;
        return -1;
    }
    return -1;
}

/* an anim picked from a type's variant group, or -1 (OVL2 0x5E3E10, new in SOCOM II; the name
 * ours) */
/* (OVL2 0x5E3B20: the compare chain tests 2, then 1) */
CZSIScriptCommon *AnimSet::GetVariantScript(AnimType type, int kind, int index)
{
    if (type >= 0 && type < m_types.size()) {
        CAnimTypeRec *rec = m_types[type];

        if (rec && rec->m_8) {
            int v = rec->GetVariant(kind, index);

            if (v >= 0) {
                switch (rec->GetGroup(v)->m_state) {
                case 1:
                    return zBoneArchive()->m_byRdr[v];
                case 2:
                    return zBoneMapArchive()->m_byRdr[v];
                default:
                    return 0;
                }
            }
        }
    }
    return 0;
}

/* a set's included sets inherited, each found by name in the list (OVL2 0x5E3940: SOCOM 1's Inherit
 * held the walk inline; the name unknown). The target inlines Find one level -- every call in it
 * (the iterator's default constructor, begin, operator=, end, !=, *, GetName, c_str, postfix ++:
 * the SEALs' copies 0x5788C0-0x578980) left a call -- so Find's whole inline tree is taller than
 * five there: GetName's five levels (above) make it so (four levels are 0x184 since the MSL headers'
 * round 24, 0x154 before it). CloseAll (0x5E5070) inlines the list's begin, end, != and ++ whole,
 * so those are at most four high in the original */
extern "C" void func_005E3940(AnimSet *set)
{
    for (std::vector<std::string>::iterator i = set->m_included.begin(); i != set->m_included.end(); i++) {
        AnimSet *base = AnimSet::Find((*i).c_str());

        if (base)
            set->Inherit(base);
    }
}

/* a set's anims taken from another (SOCOM 1's Inherit; OVL2 0x5E3740): the other's own includes
 * first while this one is not ready, then each of its type records in use (a non-negative anim)
 * put at its type's place here -- the record list grown with nulls to reach it -- and its anims by
 * the item in hand copied over where set; this set ready after. The record's dead first definition
 * numbers it before the list's reference (the target's s3 to its s2: the reference keeps the list's
 * address in a register for each pass's end()); `> -1` gives the target's slti, `>= 0` a bltz;
 * this set's groups taken first, the other's after (the later-declared pointer the lower register) */
void AnimSet::Inherit(AnimSet *base)
{
    if (this == base)
        return;
    if (!m_ready)
        func_005E3940(base);
    unsigned int i = 0;
    CAnimTypeRec *rec = 0;
    std::vector<CAnimTypeRec *> &types = base->m_types;

    for (std::vector<CAnimTypeRec *>::iterator r = types.begin(); r != types.end(); r++, i++) {
        rec = *r;

        if (rec && rec->m_id > -1) {
            while (i >= m_types.size())
                m_types.push_back(0);
            m_types[i] = rec;
        }
    }
    CAnimsByFP *fp = m_animsbyfp;
    ANIMTYPE_GROUP *g = base->m_animsbyfp->m_animgroups;

    for (int k = 0; k < CAnimsByFP::MAX_TYPES; k++, g++) {
        if (g->m_pistol_type > -1)
            fp->SetPistolAnim(g->m_rifle_type, g->m_pistol_type);
        if (g->m_rifle_fp_type > -1)
            fp->SetRifleFPAnim(g->m_rifle_type, g->m_rifle_fp_type);
        if (g->m_pistol_fp_type > -1)
            fp->SetPistolFPAnim(g->m_rifle_type, g->m_pistol_fp_type);
    }
    m_ready = true;
}

/* an anim's extra data (SOCOM 1's CAnimAuxData, one per anim type; only its first bit, cleared) */
class CAnimAuxData {
public:
    CAnimAuxData() : m_bit0(false) {}

    unsigned int m_bit0 : 1;

    /* (inline: the loaders expand it) */
    static void Add() { list.push_back(new CAnimAuxData); }

    static std::vector<CAnimAuxData *> list;
};

void InitializeAnimNames();

static inline void AddAnimKind(const char *name)
{
    std::string s(name);

    data_0044C460[s] = data_0066B808;
    data_0066B808++;
}

enum RDR_READ { RDR_ARCHIVED, RDR_FROM_DISC };

/* the sets the SEAL looks for by name (OVL1 bss; SOCOM 1's four anim set pointers) */
extern AnimSet *data_0044D3F8;
extern AnimSet *data_0044D400;
extern AnimSet *data_0044D408;
extern AnimSet *data_0044D410;
/* where the next variant group goes (OVL1 data 0x3DEA08, into a buffer at 0x44C470) */
extern ANIM_VARIANT_GROUP *s_variantCursor;

/* a set from its record: its name and includes, then each anim type's record -- the anim, or the
 * variant groups of its "modes" (SOCOM 1's Load(_zrdr *) over per-type records; OVL2 0x5E3FF0).
 * OPEN, 7 instructions: only the network_log bit's store -- the target has the value in a0 and the
 * index and the element in a1, ours the reverse: the value (generated first, reading the call's v0) must
 * colour before them, so numbered after them, and every spelling of the store and its holders keeps it
 * below (v_lz7, v_lz8, v_lz9: inlines, Get, named const locals, field types). Measured devices, marked:
 * NewRec's `set = set;` -- a parameter the callee modifies gets its copy (CInline_SetupArgsExpression: only
 * an unmodified one is substituted), the target's s1 --, the one-member structs c and pw -- scalarised,
 * numbered after the linearizer's `?:` temporaries, the target's a0 over the item's a1; name and tag reused
 * (IRO_SplitLifetimes' late webs, the two word slots below every spilled temporary) */
bool AnimSet::Load(_zrdr *rdr, bool reload)
{
    _zrdr *name = zrdr_findtag(rdr, "anim_set_name");

    if (name) {
    if (!reload) {
        m_ready = false;
        m_name = name->Item(0)->String();
        if (!strcmp(GetName().c_str(), "defusebomb anim set"))
            data_0044D3F8 = this;
        else if (!strcmp(GetName().c_str(), "Restrained anim set"))
            data_0044D400 = this;
        else if (!strcmp(GetName().c_str(), "launcher anim set"))
            data_0044D408 = this;
        else if (!strcmp(GetName().c_str(), "turret anim set"))
            data_0044D410 = this;
        int i;
        _zrdr *inc = zrdr_flat_findtag_startidx(rdr, "include", 0);

        if (inc && inc->IsArray()) {
            for (i = 0; i < inc->Count(); i++)
                if (inc->Item(i)->IsString())
                    m_included.push_back(std::string(inc->Item(i)->String()));
        }
    }
    name = zrdr_findtag(rdr, "anim_set_anims");

    if (name) {
        unsigned int n = name->Count();

        ResizeTypes(n);
        for (unsigned int i = 0; i < n; i++) {
            _zrdr *item = name->Item(i);
            int anim = -1;
            const char *tname = item->Item(1)->Item(0)->String();
            int type = FindAnimType(tname);

            if (type < 0 && !reload) {
                type = numAnimTypes++;
                data_0044C420[std::string(tname)] = type;
                CAnimAuxData::Add();
            }
            _zrdr *tag = zrdr_findtag(item, "anim_name");

            if (tag) {
                int id = data_0044C440[std::string(tag->Item(0)->String())];

                if (id > 0)
                    anim = id;
                NewRec(this, anim, type, reload);
            }
            tag = zrdr_findtag(item, "modes");

            if (tag) {
                NewRec(this, -1, type, reload);
                CAnimTypeRec *r = m_types[type];
                unsigned int nmodes = tag->Count();

                r->m_count = nmodes;
                struct { ANIM_VARIANT_GROUP *v; } c;

                c.v = s_variantCursor;

                if (nmodes == 1) {
                    if (tag->Item(0)->Item(1)->IsArray())
                        r->m_8 = c.v;
                    else
                        r->m_8 = 0;
                } else
                    r->m_8 = c.v;
                for (unsigned int m = 0; m < nmodes; m++) {
                    _zrdr *mode = tag->Item(m);
                    const char *kname = mode->Item(0)->String();
                    int kind = FindAnimKind(kname);

                    if (kind < 0) {
                        kind = data_0066B808++;
                        data_0044C460[std::string(kname)] = kind;
                    }
                    if (mode->Item(1)->IsArray()) {
                        unsigned int cnt = mode->Count() - 1;
                        float sum = 0.0f;

                        s_variantCursor->m_kind = kind;
                        s_variantCursor->m_count = cnt;
                        s_variantCursor->SetState(0);
                        s_variantCursor->m_anim = -1;
                        s_variantCursor++;
                        for (unsigned int j = 1; j <= cnt; j++) {
                            _zrdr *p = mode->Item(j);
                            int id = data_0044C440[std::string(p->Item(0)->String())];

                            if (id > 0)
                                anim = id;
                            if (r->m_id < 0)
                                r->m_id = anim;
                            *(int *)s_variantCursor = anim;
                            s_variantCursor++;
                            struct { float *v; } pw;

                            pw.v = (float *)s_variantCursor;
                            float w = p->Item(1)->Real();

                            *(int *)pw.v = *(int *)&w;
                            s_variantCursor++;
                            sum += w;
                        }
                        _close_enough(sum, 1.0f, 0.001f);
                    } else {
                        int id = data_0044C440[std::string(mode->Item(1)->String())];

                        if (id > 0)
                            anim = id;
                        if (r->m_id < 0)
                            r->m_id = anim;
                        if (nmodes == 1)
                            break;
                        {
                            s_variantCursor->m_kind = kind;
                            s_variantCursor->m_count = 1;
                            s_variantCursor->SetState(0);
                            s_variantCursor->m_anim = anim;
                            s_variantCursor++;
                        }
                    }
                }
            }
            _zrdr *logged = zrdr_findtag(item, "network_log");

            if (logged)
                CAnimAuxData::list[type]->m_bit0 = logged != 0;
            if (zrdr_findtag(item, "looped"))
                m_types[type]->SetBit0(true);
        }
    }
    if (!reload)
        list.push_back(this);
    return true;
    }
    return false;
}

/* every set read: the names' maps filled, the sets' records read, the transitions' speeds added and
 * the variants, the death tables and the included sets resolved (SOCOM 1's Load; OVL2 0x5E30F0) */
bool AnimSet::Load(bool reload)
{
    unsigned int nanims;

    numAnimTypes = 137;
    loadCount++;
    if (!reload) {
        data_0044C420.clear();
        data_0044C460.clear();
        data_0066B808 = 0;
        AddAnimKind("default");
        int n = numAnimTypes;

        if (n) {
            CAnimAuxData::list.reserve(n);
            while (n--)
                CAnimAuxData::list.push_back(new CAnimAuxData);
        }
    }
    for (int i = 0; i < numAnimTypes; i++)
        data_0044C420[std::string(data_00661060[i])] = i;
    data_0044C440.clear();
    _zrdr *rdr = zrdr_read("motion.rdr", "data/common/zrdr", 0);
    _zrdr *anims = zrdr_findtag(rdr, "animations");
    nanims = anims->Count();

    for (unsigned int j = 0; j < nanims; j++) {
        _zrdr *name = zrdr_findtag(anims->Item(j), "anim_name");

        data_0044C440[std::string(name->Item(0)->String())] = j;
    }
    zrdr_free((CRdrFile *)rdr);
    if (!reload) {
        list.clear();
        if (list.capacity() < 32)
            list.reserve(32);
    }
    rdr = zrdr_read("animset.rdr", "data/common/zrdr", reload ? RDR_FROM_DISC : RDR_ARCHIVED);
    bool ok = false;

    if (rdr) {
        unsigned int k;
        unsigned int nsets = rdr->Count();

        if (!reload)
            m_sets = new AnimSet[nsets];
        s_variantCursor = (ANIM_VARIANT_GROUP *)data_0044C470;
        for (k = 0; k < nsets; k++)
            m_sets[k].Load(rdr->Item(k), reload);
        zrdr_free((CRdrFile *)rdr);
        ok = true;
    }
    data_0044C440.clear();
    InitializeAnimNames();
    CScriptArchive *archive = zBoneArchive();

    for (std::vector<AnimSet *>::iterator a = list.begin(); a != list.end(); a++) {
        AnimSet *set = *a;

        for (std::vector<MOTION_TRANSITION *>::iterator t = archive->m_transitions.begin();
             t != archive->m_transitions.end(); t++)
            set->AddBlendable((*t)->m_index, (*t)->m_speedA, (*t)->m_speedB);
    }
    if (!reload)
        archive->Register();
    if (reload)
        return ok;
    for (std::vector<AnimSet *>::iterator b = list.begin(); b != list.end(); b++)
        (*b)->DefineFPAnims();
    InitDeathAndFlinchAnimations();
    if (deathanim[0][0].empty())
        ReadDeathAndFlinchAnims();
    for (std::vector<AnimSet *>::iterator c = list.begin(); c != list.end(); c++)
        func_005E3940(*c);
    return ok;
}

/* (OVL2 0x5E3D90) */
CZSIScriptCommon *AnimSet::GetScript(AnimType type)
{
    return GetScript(type, 0);
}

/* (OVL2 0x5E3C20: each switch's compare chain tests 2, then 1 -- the cases in reverse) */
CZSIScriptCommon *AnimSet::GetScript(AnimType type, int kind)
{
    CZSIScriptCommon *script = 0;

    if (type >= 0 && type < m_types.size()) {
        CAnimTypeRec *rec = m_types[type];

        if (rec) {
            if (rec->m_8) {
                int v = rec->PickVariant(kind);

                if (v >= 0) {
                    switch (rec->GetGroup(v)->m_state) {
                    case 1:
                        script = zBoneArchive()->m_byRdr[v];
                        break;
                    case 2:
                        script = zBoneMapArchive()->m_byRdr[v];
                        break;
                    }
                }
            }
            if (!script) {
                int id = rec->m_id;

                if (id >= 0) {
                    switch (rec->GetState()) {
                    case 1:
                        script = zBoneArchive()->m_byRdr[id];
                        break;
                    case 2:
                        script = zBoneMapArchive()->m_byRdr[id];
                        break;
                    }
                }
            }
        }
    }
    return script;
}

int AnimSet::PickVariant(AnimType type, int kind)
{
    if (type >= 0 && type < m_types.size()) {
        CAnimTypeRec *rec = m_types[type];

        return rec ? rec->PickVariant(kind) : -1;
    }
    return -1;
}

/* an anim's index in a type's variant group, or -1 (OVL2 0x5E3DA0, new in SOCOM II; the name ours) */
int AnimSet::GetVariantIndex(AnimType type, int kind, int anim)
{
    if (type >= 0 && type < m_types.size()) {
        CAnimTypeRec *rec = m_types[type];

        return rec ? rec->GetVariantIndex(kind, anim) : -1;
    }
    return -1;
}

/* whether a type's record has its bit 1 set (OVL2 0x5E3AC0, new in SOCOM II; the name ours) */
bool AnimSet::TypeBit1(AnimType type)
{
    if (type >= 0 && type < m_types.size()) {
        CAnimTypeRec *rec = m_types[type];

        if (rec)
            return rec->m_bit1 ? true : false;
    }
    return false;
}

/* a rifle anim type's pistol variant (OVL2 0x5E1CF0), first-person rifle variant (0x5E1BF0) and
 * first-person pistol variant (0x5E1AF0) set, when both have anims: the group's types and the
 * variant's base.
 * The target extends each type once inside each inlined AnimId (the extension dies in the index)
 * and again from the parameter for the stores: AnimId's argument cast to its own type
 * (`AnimId((AnimType)rifle)`, a macro's spelling, likely) makes it an expression of its own, which
 * MW does not share with the stores' -- written plainly, MW kept the first extension and reused it
 * (0xF8 against 0x100; a local copy of the parameter does as well as the cast). Two locals for the
 * AnimIds (computed before both `> -1` tests, slti as the target's) and AnimId's if-returns (each
 * arm its own `b`) give the rest */
void AnimSet::SetPistolAnim(AnimType rifle, AnimType pistol)
{
    int a = AnimId((AnimType)rifle);
    int b = AnimId((AnimType)pistol);

    if (a > -1 && b > -1) {
        m_animsbyfp->SetPistolAnim(rifle, pistol);
        m_types[pistol]->m_base = rifle;
    }
}

void AnimSet::SetRifleFPAnim(AnimType rifle, AnimType fp)
{
    int a = AnimId((AnimType)rifle);
    int b = AnimId((AnimType)fp);

    if (a > -1 && b > -1) {
        m_animsbyfp->SetRifleFPAnim(rifle, fp);
        m_types[fp]->m_base = rifle;
    }
}

void AnimSet::SetPistolFPAnim(AnimType rifle, AnimType fp)
{
    int a = AnimId((AnimType)rifle);
    int b = AnimId((AnimType)fp);

    if (a > -1 && b > -1) {
        m_animsbyfp->SetPistolFPAnim(rifle, fp);
        m_types[fp]->m_base = rifle;
    }
}

/* a type's base anim type, or -1 (SOCOM 1's BaseAnim, read from the type's record; OVL2 0x5E1A90) */
AnimType AnimSet::BaseAnim(AnimType type)
{
    if (type >= 0 && type < m_types.size()) {
        CAnimTypeRec *rec = m_types[type];

        return rec ? rec->m_base : animTypeUndefined;
    }
    return animTypeUndefined;
}

/* the anim of a type: its pistol variant (OVL2 0x5E1A50), its rifle first-person one (0x5E1A10) and
 * its pistol first-person one (0x5E19D0), -1 out of range (SOCOM 1's) */
AnimType CAnimsByFP::PistolAnim(AnimType type) const
{
    if (type >= MAX_TYPES || type < 0)
        return animTypeUndefined;
    return m_animgroups[type].m_pistol_type;
}

AnimType CAnimsByFP::RifleFPAnim(AnimType type) const
{
    if (type >= MAX_TYPES || type < 0)
        return animTypeUndefined;
    return m_animgroups[type].m_rifle_fp_type;
}

AnimType CAnimsByFP::PistolFPAnim(AnimType type) const
{
    if (type >= MAX_TYPES || type < 0)
        return animTypeUndefined;
    return m_animgroups[type].m_pistol_fp_type;
}

/* a rifle anim type's pistol variant set (OVL2 0x5E19A0; SOCOM 1's, byte for byte) */
void CAnimsByFP::SetPistolAnim(AnimType rifle, AnimType pistol)
{
    m_animgroups[rifle].m_rifle_type = rifle;
    m_animgroups[rifle].m_pistol_type = pistol;
}


extern short data_003DEA10;
extern short data_003DEA18;
extern short data_003DEA20;
extern short data_003DEA28;
extern short data_003DEA30;
extern short data_003DEA38;
extern short data_003DEA40;
extern short data_003DEA48;
extern short data_003DEA50;
extern short data_003DEA58;
extern short data_003DEA60;
extern short data_003DEA68;
extern short data_003DEA70;
extern short data_003DEA78;
extern short data_003DEA80;
extern short data_003DEA88;
extern short data_003DEA90;
extern short data_003DEA98;
extern short data_003DEAA0;
extern short data_003DEAA8;
extern short data_003DEAB0;
extern short data_003DEAB8;
extern short data_003DEAC8;
extern short data_003DEAD0;
extern short data_003DEB00;
extern short data_003DEB08;
extern short data_003DEB10;
extern short data_003DEB18;
extern short data_003DEB28;
extern short data_003DEB38;
extern short data_003DEB48;
extern short data_003DEB50;
extern short data_003DEB98;
extern short data_003DEBA0;
extern short data_003DEBB0;
extern short data_003DEBC0;
extern short data_003DEBD0;
extern short data_003DEC18;
extern short data_003DEC20;
extern short data_003DEC28;
extern short data_003DEC30;
extern short data_003DEC38;
extern short data_003DEC40;
extern short data_003DEC58;
extern short data_003DEC60;
extern short data_003DEC88;
extern short data_003DEC98;
extern short data_003DECA8;
extern short data_003DECB8;
extern short data_003DECC0;
extern short data_003DECC8;
extern short data_003DECD0;
extern short data_003DECD8;
extern short data_003DECE0;
extern short data_003DECE8;
extern short data_003DECF0;
extern short data_003DECF8;
extern short data_003DED00;
extern short data_003DED08;
extern short data_003DED10;
extern short data_003DED18;
extern short data_003DED20;
extern short data_003DED28;
extern short data_003DED30;
extern short data_003DED38;
extern short data_003DED40;
extern short data_003DED48;
extern short data_003DED50;
extern short data_003DED58;
extern short data_003DED60;
extern short data_003DED68;
extern short data_003DED70;
extern short data_003DED78;
extern short data_003DED80;
extern short data_003DED88;
extern short data_003DED90;
extern short data_003DED98;
extern short data_003DEDA0;
extern short data_003DEDA8;
extern short data_003DEE70;
extern short data_003DEE90;
extern short data_003DEEA8;
extern short data_003DEEC0;
extern short data_003DEF10;
extern short data_003DEF18;
extern short data_003DEF20;
extern short data_003DEF28;
extern short data_003DEF30;
extern short data_003DEF38;
extern short data_003DEF40;
extern short data_003DEF50;
extern short data_003DEF60;

/* every anim type's variants: the groups made all -1, then the pistol, first-person rifle and
 * first-person pistol anims of the rifle types (SOCOM 1's DefineFPAnims, which inlined all three
 * setters over its 142 types; here the setters are calls), and the records of the types whose bit 1
 * is set (OVL2 0x5E0690) */
void AnimSet::DefineFPAnims()
{
    m_animsbyfp = new CAnimsByFP;
    SetPistolAnim((AnimType)1, (AnimType)83);
    SetPistolAnim((AnimType)18, (AnimType)84);
    SetPistolAnim((AnimType)26, (AnimType)85);
    SetPistolAnim((AnimType)34, (AnimType)86);
    SetPistolAnim((AnimType)45, (AnimType)87);
    SetPistolAnim((AnimType)39, (AnimType)115);
    SetPistolAnim((AnimType)2, (AnimType)88);
    SetPistolAnim((AnimType)3, (AnimType)89);
    SetPistolAnim((AnimType)4, (AnimType)90);
    SetPistolAnim((AnimType)5, (AnimType)91);
    SetPistolAnim((AnimType)6, (AnimType)92);
    SetPistolAnim((AnimType)7, (AnimType)93);
    SetPistolAnim((AnimType)12, (AnimType)94);
    SetPistolAnim((AnimType)13, (AnimType)95);
    SetPistolAnim((AnimType)14, (AnimType)94);
    SetPistolAnim((AnimType)15, (AnimType)95);
    SetPistolAnim((AnimType)16, (AnimType)98);
    SetPistolAnim((AnimType)17, (AnimType)99);
    SetPistolAnim((AnimType)19, (AnimType)100);
    SetPistolAnim((AnimType)20, (AnimType)101);
    SetPistolAnim((AnimType)21, (AnimType)102);
    SetPistolAnim((AnimType)22, (AnimType)103);
    SetPistolAnim((AnimType)23, (AnimType)103);
    SetPistolAnim((AnimType)AnimTypes::CrouchStrafeLeft, (AnimType)data_003DECB8);
    SetPistolAnim((AnimType)24, (AnimType)104);
    SetPistolAnim((AnimType)27, (AnimType)105);
    SetPistolAnim((AnimType)29, (AnimType)106);
    SetPistolAnim((AnimType)30, (AnimType)107);
    SetPistolAnim((AnimType)31, (AnimType)108);
    SetPistolAnim((AnimType)32, (AnimType)109);
    SetPistolAnim((AnimType)data_003DEB00, (AnimType)data_003DEB10);
    SetPistolAnim((AnimType)data_003DEB08, (AnimType)data_003DEB18);
    SetPistolAnim((AnimType)AnimTypes::TossGrenadePeekRight, (AnimType)data_003DEB28);
    SetPistolAnim((AnimType)AnimTypes::TossGrenadePeekLeft, (AnimType)data_003DEB38);
    SetPistolAnim((AnimType)33, (AnimType)110);
    SetPistolAnim((AnimType)35, (AnimType)111);
    SetPistolAnim((AnimType)36, (AnimType)112);
    SetPistolAnim((AnimType)37, (AnimType)113);
    SetPistolAnim((AnimType)38, (AnimType)114);
    SetPistolAnim((AnimType)40, (AnimType)116);
    SetPistolAnim((AnimType)41, (AnimType)117);
    SetPistolAnim((AnimType)42, (AnimType)118);
    SetPistolAnim((AnimType)43, (AnimType)119);
    SetPistolAnim((AnimType)44, (AnimType)120);
    SetPistolAnim((AnimType)46, (AnimType)121);
    SetPistolAnim((AnimType)47, (AnimType)122);
    SetPistolAnim((AnimType)48, (AnimType)123);
    SetPistolAnim((AnimType)49, (AnimType)124);
    SetPistolAnim((AnimType)56, (AnimType)126);
    SetPistolAnim((AnimType)57, (AnimType)127);
    SetPistolAnim((AnimType)58, (AnimType)128);
    SetPistolAnim((AnimType)59, (AnimType)129);
    SetPistolAnim((AnimType)61, (AnimType)130);
    SetPistolAnim((AnimType)62, (AnimType)131);
    SetPistolAnim((AnimType)63, (AnimType)132);
    SetPistolAnim((AnimType)64, (AnimType)133);
    SetPistolAnim((AnimType)65, (AnimType)134);
    SetPistolAnim((AnimType)66, (AnimType)135);
    SetPistolAnim((AnimType)67, (AnimType)136);
    SetPistolAnim((AnimType)AnimTypes::ThrowGrenade, (AnimType)data_003DEBD0);
    SetPistolAnim((AnimType)AnimTypes::CrouchThrowGrenade, (AnimType)data_003DEBC0);
    SetPistolAnim((AnimType)AnimTypes::ProneThrowGrenade, (AnimType)data_003DEBB0);
    SetPistolAnim((AnimType)AnimTypes::TossGrenade, (AnimType)AnimTypes::PistolTossGrenade);
    SetPistolAnim((AnimType)AnimTypes::CrouchThrowGrenade, (AnimType)data_003DEBC0);
    SetPistolAnim((AnimType)AnimTypes::ProneTossGrenade, (AnimType)AnimTypes::PistolProneTossGrenade);
    SetPistolAnim((AnimType)data_003DEC18, (AnimType)data_003DEC20);
    SetPistolAnim((AnimType)data_003DEC28, (AnimType)data_003DEC30);
    SetPistolAnim((AnimType)data_003DEC38, (AnimType)data_003DEC40);
    SetPistolAnim((AnimType)AnimTypes::Stand2Prone, (AnimType)AnimTypes::PistolStand2Prone);
    SetPistolAnim((AnimType)data_003DEC58, (AnimType)data_003DEC60);
    SetPistolAnim((AnimType)data_003DECF0, (AnimType)data_003DECF8);
    SetPistolAnim((AnimType)data_003DED00, (AnimType)data_003DED08);
    SetPistolAnim((AnimType)data_003DED10, (AnimType)data_003DED18);
    SetPistolAnim((AnimType)data_003DED20, (AnimType)data_003DED28);
    SetPistolAnim((AnimType)data_003DED30, (AnimType)data_003DED38);
    SetPistolAnim((AnimType)data_003DED40, (AnimType)data_003DED48);
    SetPistolAnim((AnimType)data_003DED50, (AnimType)data_003DED58);
    SetPistolAnim((AnimType)data_003DED60, (AnimType)data_003DED68);
    SetPistolAnim((AnimType)data_003DED70, (AnimType)data_003DED78);
    SetPistolAnim((AnimType)data_003DED80, (AnimType)data_003DED88);
    SetPistolAnim((AnimType)data_003DED90, (AnimType)data_003DED98);
    SetPistolAnim((AnimType)data_003DEDA0, (AnimType)data_003DEDA8);
    SetPistolAnim((AnimType)AnimTypes::Stab, (AnimType)AnimTypes::PistolStab);
    SetPistolAnim((AnimType)AnimTypes::CQB, (AnimType)AnimTypes::PistolCQB);
    SetPistolAnim((AnimType)data_003DEB48, (AnimType)data_003DEB50);
    SetPistolAnim((AnimType)AnimTypes::RifleReload, (AnimType)AnimTypes::PistolReload);
    SetPistolAnim((AnimType)AnimTypes::RifleCrouchReload, (AnimType)AnimTypes::PistolCrouchReload);
    SetPistolAnim((AnimType)AnimTypes::RifleProneReload, (AnimType)AnimTypes::PistolProneReload);
    SetPistolAnim((AnimType)data_003DEBA0, (AnimType)data_003DEB98);
    SetPistolAnim((AnimType)data_003DECC0, (AnimType)data_003DECC8);
    SetPistolAnim((AnimType)data_003DECD0, (AnimType)data_003DECD8);
    SetPistolAnim((AnimType)data_003DECE0, (AnimType)data_003DECE8);
    SetPistolAnim((AnimType)AnimTypes::StepUp, (AnimType)data_003DEC88);
    SetPistolAnim((AnimType)AnimTypes::ClimbMedium, (AnimType)data_003DEC98);
    SetPistolAnim((AnimType)AnimTypes::ClimbOver, (AnimType)data_003DECA8);
    SetPistolAnim((AnimType)AnimTypes::BuddyCarryStand, (AnimType)data_003DEE70);
    SetPistolAnim((AnimType)AnimTypes::BuddyCarryWalk, (AnimType)data_003DEE90);
    SetPistolAnim((AnimType)AnimTypes::BuddyCarryPickup, (AnimType)data_003DEEA8);
    SetPistolAnim((AnimType)AnimTypes::BuddyCarryStep, (AnimType)data_003DEEC0);
    SetPistolAnim((AnimType)data_003DEF10, (AnimType)data_003DEF18);
    SetPistolAnim((AnimType)data_003DEF20, (AnimType)data_003DEF28);
    SetPistolAnim((AnimType)data_003DEF30, (AnimType)data_003DEF38);
    SetRifleFPAnim((AnimType)1, (AnimType)data_003DEA10);
    SetRifleFPAnim((AnimType)18, (AnimType)data_003DEA18);
    SetRifleFPAnim((AnimType)26, (AnimType)data_003DEA20);
    SetRifleFPAnim((AnimType)2, (AnimType)data_003DEA30);
    SetRifleFPAnim((AnimType)3, (AnimType)data_003DEA38);
    SetRifleFPAnim((AnimType)4, (AnimType)data_003DEA40);
    SetRifleFPAnim((AnimType)5, (AnimType)data_003DEA48);
    SetRifleFPAnim((AnimType)6, (AnimType)data_003DEA50);
    SetRifleFPAnim((AnimType)16, (AnimType)data_003DEA60);
    SetRifleFPAnim((AnimType)20, (AnimType)data_003DEA28);
    SetRifleFPAnim((AnimType)24, (AnimType)data_003DEA58);
    SetRifleFPAnim((AnimType)21, (AnimType)data_003DEAC8);
    SetPistolFPAnim((AnimType)1, (AnimType)data_003DEA68);
    SetPistolFPAnim((AnimType)18, (AnimType)data_003DEA70);
    SetPistolFPAnim((AnimType)26, (AnimType)data_003DEA78);
    SetPistolFPAnim((AnimType)2, (AnimType)data_003DEA88);
    SetPistolFPAnim((AnimType)3, (AnimType)data_003DEA90);
    SetPistolFPAnim((AnimType)4, (AnimType)data_003DEA98);
    SetPistolFPAnim((AnimType)5, (AnimType)data_003DEAA0);
    SetPistolFPAnim((AnimType)6, (AnimType)data_003DEAA8);
    SetPistolFPAnim((AnimType)16, (AnimType)data_003DEAB8);
    SetPistolFPAnim((AnimType)20, (AnimType)data_003DEA80);
    SetPistolFPAnim((AnimType)24, (AnimType)data_003DEAB0);
    SetPistolFPAnim((AnimType)21, (AnimType)data_003DEAD0);
    SetPistolFPAnim((AnimType)12, (AnimType)94);
    SetPistolFPAnim((AnimType)13, (AnimType)95);
    SetPistolFPAnim((AnimType)14, (AnimType)94);
    SetPistolFPAnim((AnimType)15, (AnimType)95);
    SetPistolFPAnim((AnimType)66, (AnimType)135);
    SetPistolFPAnim((AnimType)67, (AnimType)136);
    SetTypeBit1((AnimType)1);
    SetTypeBit1((AnimType)18);
    SetTypeBit1((AnimType)26);
    SetTypeBit1((AnimType)34);
    SetTypeBit1((AnimType)16);
    SetTypeBit1((AnimType)45);
    SetTypeBit1((AnimType)39);
    SetTypeBit1((AnimType)33);
    SetTypeBit1((AnimType)35);
    SetTypeBit1((AnimType)AnimTypes::RifleReload);
    SetTypeBit1((AnimType)AnimTypes::RifleCrouchReload);
    SetTypeBit1((AnimType)AnimTypes::RifleProneReload);
    SetTypeBit1((AnimType)data_003DEBA0);
    SetTypeBit1((AnimType)AnimTypes::Stab);
    SetTypeBit1((AnimType)AnimTypes::CQB);
    SetTypeBit1((AnimType)AnimTypes::ApplyRestraints);
    SetTypeBit1((AnimType)AnimTypes::BuddyCarryStand);
    SetTypeBit1((AnimType)AnimTypes::BuddyCarryWalk);
    SetTypeBit1((AnimType)AnimTypes::BuddyCarryPickup);
    SetTypeBit1((AnimType)AnimTypes::BuddyCarryStep);
    SetTypeBit1((AnimType)AnimTypes::TossGrenadePeekRight);
    SetTypeBit1((AnimType)AnimTypes::TossGrenadePeekLeft);
    SetTypeBit1((AnimType)data_003DEF40);
    SetTypeBit1((AnimType)data_003DEF50);
    SetTypeBit1((AnimType)data_003DEF60);
    SetTypeBit1((AnimType)data_003DEF10);
    SetTypeBit1((AnimType)data_003DEF20);
    SetTypeBit1((AnimType)data_003DEF30);
}

/* the set's top speeds: each move blend's fastest anim, standing then crouched, the prone and ladder
 * speeds left to the body (SOCOM 1's SetMaxMotion, its blends held whole here: MaxTrans1 inlined
 * eight times; OVL2 0x5E0450 -- CZSealBody::SetMaxMotion calls it first) */
void AnimSet::SetMaxMotion()
{
    m_aparams.m_stand_max_fwd = m_move_anims.MaxTrans1();
    m_aparams.m_stand_max_back = m_moveback_anims.MaxTrans1();
    m_aparams.m_stand_max_rt = m_strafeL_anims.MaxTrans1();
    m_aparams.m_stand_max_left = m_strafeR_anims.MaxTrans1();
    m_aparams.m_crouch_max_fwd = m_crouchmove_anims.MaxTrans1();
    m_aparams.m_crouch_max_back = m_crouchmoveback_anims.MaxTrans1();
    m_aparams.m_crouch_max_rt = m_crouchstrafeL_anims.MaxTrans1();
    m_aparams.m_crouch_max_left = m_crouchstrafeR_anims.MaxTrans1();
}

/* an anim's speeds, scaled by ten, put in the move blend its type belongs to: walking and running
 * forward, back, right and left, crouched forward, back and right -- crouched left by the anim
 * type's variable, SOCOM 1's AnimTypes::CrouchStrafeLeft (SOCOM 1's AddBlendable, the same cases,
 * its blends held whole here; OVL2 0x5E0030) */
void AnimSet::AddBlendable(int id, float t0, float t1)
{
    AnimType type;
    int n = 0;

    for (std::vector<CAnimTypeRec *>::iterator i = m_types.begin(); i != m_types.end(); i++, n++) {
        if (*i && (*i)->m_id == (short)id) {
            type = (AnimType)n;
            goto found;
        }
    }
    type = animTypeUndefined;
found:

    if (type == animTypeUndefined)
        return;
    t0 *= 10.0f;
    t1 *= 10.0f;
    switch (type) {
    case 2:   /* SOCOM 1's Sneak, Skitter, Run */
    case 3:
    case 4:
        m_move_anims.AddBlendable(t0, t1, type);
        break;
    case 5:   /* SneakBack, SkitterBack */
    case 6:
        m_moveback_anims.AddBlendable(t0, t1, type);
        break;
    case 12:   /* StrafeRight, StrafeRightFast, RunR */
    case 14:
    case 66:
        m_strafeR_anims.AddBlendable(t0, t1, type);
        break;
    case 13:   /* StrafeLeft, StrafeLeftFast, RunL */
    case 15:
    case 67:
        m_strafeL_anims.AddBlendable(t0, t1, type);
        break;
    case 20:   /* CrouchWalk */
        m_crouchmove_anims.AddBlendable(t0, t1, type);
        break;
    case 22:   /* CrouchStrafeR, CrouchStrafeRFast */
    case 23:
        m_crouchstrafeR_anims.AddBlendable(t0, t1, type);
        break;
    case 24:   /* CrouchWalkBack */
        m_crouchmoveback_anims.AddBlendable(t0, t1, type);
        break;
    default:
        if (type == AnimTypes::CrouchStrafeLeft)
            m_crouchstrafeL_anims.AddBlendable(t0, t1, type);
        break;
    }
}
