/* anim.cpp -- a SEAL's animation sets: AnimSet, its anims by the item in hand (CAnimsByFP), the
 * per-type records and the blendables (SOCOM 1's unit, __sinit_anim.cpp, linked between
 * seal_weapon.cpp and remote_control.cpp; SOCOM 1's DWARF describes AnimSet and its pieces, no
 * function). OVL2 0x5E0030-0x5E622F, in SOCOM 1's order: AddBlendable, SetMaxMotion, DefineFPAnims,
 * CAnimsByFP's accessors, BaseAnim, InitializeAnimNames, Load, CAnimAuxData::Get, Inherit, the
 * lookups SOCOM II added, AnimId, Load(_zrdr *), ... CBlendable's, the template tail. So far the
 * small accessors. Defined in reverse of their image order (-inline deferred). */

#include "zseal.h"

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
    unsigned char m_kind;   // 0x0
    unsigned char m_count;   // 0x1
    unsigned short m_2 : 2;   // 0x2
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
        SetBits2(0);
        m_8 = 0;
    }
    void SetBit0(bool b) { m_bit0 = b; }
    void SetBit1(bool b) { m_bit1 = b; }
    void SetBits2(unsigned int n) { m_bits2 = n; }
    /* the variant groups' count (inline: the walks' loop test re-reads it through it) */
    unsigned int Count() const { return m_count; }
    /* a variant group's index of an anim, and an anim picked from a group by weight (OVL2
     * 0x5E5790, 0x5E5970: not written yet) */
    int GetVariantIndex(int kind, int anim);
    int PickVariant(int kind);

    short m_id;   // 0x0: the anim (AnimId)
    AnimType m_base;   // 0x2: the base type (BaseAnim)
    unsigned char m_count;   // 0x4: variant groups at m_8
    unsigned char m_bit0 : 1;   // 0x5
    unsigned char m_bit1 : 1;
    unsigned char m_bits2 : 2;
    unsigned char m_5_4 : 4;
    unsigned char _6[2];
    void *m_8;   // 0x8: the variant groups
};

/* a SEAL's animations (SOCOM 1's DWARF: AnimSet, 0x84 bytes there) -- only what is written here:
 * SOCOM II's string name is 12 bytes, and its two short vectors became one of records */
class AnimSet {
public:
    AnimType BaseAnim(AnimType type);
    bool TypeBit1(AnimType type);
    int GetVariantIndex(AnimType type, int kind, int anim);
    int PickVariant(AnimType type, int kind);
    short AnimId(AnimType type) const;

    unsigned char _0[0x44];
    std::vector<CAnimTypeRec *> m_types;   // 0x44: a record per anim type, or 0
};

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
short AnimSet::AnimId(AnimType type) const
{
    if (type >= 0 && type < m_types.size()) {
        CAnimTypeRec *rec = m_types[type];

        return rec ? rec->m_id : (short)-1;
    }
    return -1;
}

/* an anim picked from a type's variant group, or -1 (OVL2 0x5E3E10, new in SOCOM II; the name
 * ours) */
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
