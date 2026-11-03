/* anim.cpp -- a SEAL's animation sets: AnimSet, its anims by the item in hand (CAnimsByFP), the
 * per-type records and the blendables (SOCOM 1's unit, __sinit_anim.cpp, linked between
 * seal_weapon.cpp and remote_control.cpp; SOCOM 1's DWARF describes AnimSet and its pieces, no
 * function). OVL2 0x5E0030-0x5E622F, in SOCOM 1's order: AddBlendable, SetMaxMotion, DefineFPAnims,
 * CAnimsByFP's accessors, BaseAnim, InitializeAnimNames, Load, CAnimAuxData::Get, Inherit, the
 * lookups SOCOM II added, AnimId, Load(_zrdr *), ... CBlendable's, the template tail. So far the
 * small accessors and the three name lookups (whose map find and string compares, OVL2's copies
 * 0x58D090, 0x58D250, 0x58D740, the unit's tail emits). Defined in reverse of their image order
 * (-inline deferred). */

#include "zseal.h"

#include <map>
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
    bool Contains(AnimType type);

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
    /* a set closed (SOCOM 1's Close freed its anims; SOCOM II's CloseAll inlines it to nothing --
     * the walk is all that is left) and every set closed (SOCOM 1's; CGame::StopEngine calls it) */
    void Close() {}
    static void CloseAll();

    /* every set (SOCOM 1's AnimSet::List, a pointer vector; OVL1 bss 0x44D430) */
    static std::vector<AnimSet *> list;

    unsigned char _0[0x44];
    std::vector<CAnimTypeRec *> m_types;   // 0x44: a record per anim type, or 0
    unsigned char _50[0x5c - 0x50];
    CAnimsByFP *m_animsbyfp;   // 0x5c (SOCOM 1's m_animgps at 0x60)
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

/* the group holding an anim, of any kind, or 0 (OVL2 0x5E56D0).
 * OPEN, 18 instructions, registers only: the target walks the cursor in t1 and copies it into v0
 * (the group returned) at each pass's top, the early `return 0` a `move v0,zero` in the null test's
 * delay slot; here MW returns the null cursor itself for that 0 and so walks the cursor in v0
 * (the group in t0). Tried: `if (!m_8)` before the cursor, a result local zeroed first and
 * returned, the copy assigned after the count, the group's own pointer for the single test, the
 * pairs' walk after a `continue` -- 16 to 21 instructions off */
ANIM_VARIANT_GROUP *CAnimTypeRec::GetGroup(int anim)
{
    ANIM_VARIANT_GROUP *g = (ANIM_VARIANT_GROUP *)m_8;

    if (!g)
        return 0;
    for (unsigned int i = 0; i < Count(); i++) {
        ANIM_VARIANT_GROUP *group = g;
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

/* an anim type by name, -1 for none or an unknown one (SOCOM 1's FindAnimType: CTPlay::Parse
 * calls it; OVL2 0x5E5000). The test around the lookup, not an early return: `if (!name) return
 * -1;` lays the -1 block out first */
AnimType FindAnimType(const char *name)
{
    if (name) {
        std::map<std::string, int>::iterator i = data_0044C420.find(name);

        if (i != data_0044C420.end())
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

        if (i != data_0044C460.end())
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
 * OPEN, all three, 0xF8 against 0x100: the target extends each type once inside each inlined
 * AnimId (the extension dies in the index) and again from the parameter for the stores; MW keeps
 * the first extension here and reuses it. Two locals for the AnimIds (computed before both `> -1`
 * tests, slti as the target's) and AnimId's if-returns (each arm its own `b`) give the rest; tried
 * int parameters with casts (20 off), `(int)` arguments, the stores written out */
void AnimSet::SetPistolAnim(AnimType rifle, AnimType pistol)
{
    int a = AnimId(rifle);
    int b = AnimId(pistol);

    if (a > -1 && b > -1) {
        m_animsbyfp->SetPistolAnim(rifle, pistol);
        m_types[pistol]->m_base = rifle;
    }
}

void AnimSet::SetRifleFPAnim(AnimType rifle, AnimType fp)
{
    int a = AnimId(rifle);
    int b = AnimId(fp);

    if (a > -1 && b > -1) {
        m_animsbyfp->SetRifleFPAnim(rifle, fp);
        m_types[fp]->m_base = rifle;
    }
}

void AnimSet::SetPistolFPAnim(AnimType rifle, AnimType fp)
{
    int a = AnimId(rifle);
    int b = AnimId(fp);

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
