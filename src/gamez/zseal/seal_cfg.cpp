/* seal_cfg.cpp -- the SEAL's configuration: its gear, its animation set's limits (SOCOM 1's unit,
 * __sinit_seal_cfg.cpp; its strings put SetupGear there, GetMaxXMotion and GetMaxZMotion after
 * it). OVL2 before seal_command.cpp (0x58FBE0). Defined in reverse of their image order
 * (-inline deferred). */

#include "zseal.h"

/* a blend's anims (anim.cpp's CBlendable, the kit lane's: only the test here) */
class CBlendable {
public:
    bool Contains(AnimType type);
};

/* the animation set's per-weapon tables (anim.cpp's CAnimsByFP, the kit lane's: only the pistol's
 * lookup) and the set's pointer to them at 0x5C (a view of AnimSet; the name ours) */
class CAnimsByFP {
public:
    AnimType PistolAnim(AnimType type) const;
};

struct AnimSetByFP {
    unsigned char _0[0x5c];
    CAnimsByFP *m_byfp;   // 0x5c
};

/* a name-to-id map (MSL's map<string, int>: a tree node's links, then the pair; OVL1 bss 0x44AF60, its
 * header node at +4). OVL2 calls its find and its operator[]'s insert out of line (0x58D090, 0x58CEF0
 * -- the string-keyed tree is not modelled in include/tree), as it does the string's constructor and
 * destructor (0x58CE70, 0x53FAD0: ovl-copy rows) */
struct CNameIdNode {
    unsigned char _0[0xc];
    std::string m_key;   // 0xc
    int m_id;   // 0x18
};

struct CNameIdPair {
    std::string first;   // 0x0
    int second;   // 0xc
};

extern "C" {
extern unsigned char data_0044AF60[];
/* whether the map is filled (OVL1 bss 0x44AF70), and the next id handed out (OVL1 data 0x3DE930) */
extern bool data_0044AF70;
extern int data_003DE930;
/* the map filled (OVL2 0x58CC30, not written yet) */
void func_0058CC30();
void func_0058D090(CNameIdNode **it, void *map, const std::string &key);
CNameIdPair *func_0058CEF0(void *map, const std::string &key);
}

extern "C" {
/* the next body part id handed out (OVL1 data 0x3DE928) */
extern unsigned int data_003DE928;
}

/* a body part's id by its name: the map filled once, the name found -- or given the next id while
 * the parts' object slots last, the last four kept back, else -1 (OVL2 0x58CB30, new in SOCOM II:
 * BodyAddAnim, NewBodyPart, ReadMotionRdr and CCharacterType::Open ask it; the names ours) */
/* (zbody.h declares it, as an int) */
extern "C" int func_0058CB30(const char *name)
{
    CNameIdNode *it;

    if (!data_0044AF70)
        func_0058CC30();
    func_0058D090(&it, data_0044AF60, std::string(name));
    if (it != (CNameIdNode *)(data_0044AF60 + 4))
        return it->m_id;

    unsigned int id = data_003DE928;

    if (id >= siNumBodyObjects - 4)
        return -1;
    func_0058CEF0(data_0044AF60, std::string(name))->second = id;
    data_003DE928++;
    return id;
}

/* a name's id: the map filled once, the name found -- or given the next id (OVL2 0x58CA60, new in
 * SOCOM II; the names ours) */
extern "C" int func_0058CA60(const char *name)
{
    CNameIdNode *it;

    if (!data_0044AF70)
        func_0058CC30();
    func_0058D090(&it, data_0044AF60, std::string(name));
    if (it != (CNameIdNode *)(data_0044AF60 + 4))
        return it->m_id;

    int id = data_003DE930;

    func_0058CEF0(data_0044AF60, std::string(name))->second = id;
    data_003DE930++;
    return id;
}

/* an anim type for a kind of hold: 1 as it is, 2 the pistol's own (the type itself when the set has
 * none), any other kind as it is (OVL2 0x58C9E0, new in SOCOM II; the names ours) */
extern "C" AnimType func_0058C9E0(CZSealBody *seal, AnimType type, unsigned char kind)
{
    AnimType r;

    switch (kind) {
    case 1:
        r = type;
        break;
    case 2:
        r = ((AnimSetByFP *)seal->m_animset)->m_byfp->PistolAnim(type);
        break;
    default:
        return type;
    }
    if (r < 0)
        return type;
    return r;
}

/* every anim of a list that a blendable holds played at a rate scaled (OVL2 0x58BAB0, new in SOCOM
 * II; its first argument unread, the names ours -- the rate read ahead of the test) */
extern "C" void func_0058BAB0(CZSealBody *seal, CActiveAnimList *list, CBlendable *blendable, float scale)
{
    /* (RTList<ACTIVE_ANIM>'s Begin and Next: this unit emits the instances, OVL2's copies 0x5780E0
     * and 0x578110 in seal_anim.cpp's span) */
    for (ACTIVE_ANIM *a = list->Begin(); a; a = list->Next()) {
        float rate = a->m_playback;

        if (blendable->Contains((AnimType)a->m_id))
            a->m_playback = scale * rate;
    }
}

/* two lookups over one byte kind (OVL2 0x58DEB0, 0x58DF20, new in SOCOM II: the HUD's radio menu
 * at 0x2329F0/0x232BC0 asks both in turn; the names ours): 2, 16 and the group 9-13 each give their
 * own value, any other kind 0. Each case's compare runs in reverse source order */
extern "C" int func_0058DF20(unsigned char k)
{
    int r = 0;

    switch (k) {
    case 2:
        r = 3;
        break;
    case 16:
        r = 17;
        break;
    case 10:
    case 11:
    case 9:
    case 13:
    case 12:
        r = 21;
        break;
    }
    return r;
}

extern "C" int func_0058DEB0(unsigned char k)
{
    int r = 0;

    switch (k) {
    case 2:
        r = 16;
        break;
    case 16:
        r = 20;
        break;
    case 10:
    case 11:
    case 9:
    case 13:
    case 12:
        r = 32;
        break;
    }
    return r;
}

/* an animation set's speeds (SOCOM 1's DWARF: ANIMSET_PARAMS, 0x34 bytes) */
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

/* a piece of gear a character wears, CCharacterGear, is zseal.h's */

/* (OVL1 0x2DE9E0: a material's index by name, -1 unknown; node_util.cpp's DI setters) */
extern "C" int func_002DE9E0(const char *name);
void SetDIBBox(zdb::CNode *node, const CBBox *bbox, bool b);
void SetMaterial(zdb::CNode *node, int material);

/* every bone's node (the parts past the last four object slots left out) given a box DI of the
 * PERSON material, and the upper spine's box capped at 3.01 across (OVL2 0x58B960: SOCOM 1's
 * AddDIPolys, after the AddItems in both builds -- SOCOM 1 skipped the rifle, pistol, grenade and
 * weapon parts by pointer, SOCOM II by id; the cap is new) */
void CZSealBody::AddDIPolys()
{
    std::vector<CZBodyPart *> &parts = m_body_parts;
    int mat = func_002DE9E0("PERSON");

    for (std::vector<CZBodyPart *>::iterator i = parts.begin(); i != parts.end(); ++i) {
        CZBodyPart *p = *i;

        if (p && p->m_id < siNumBodyObjects - 4) {
            zdb::CNode *n = p->m_node;

            SetDIBBox(n, &n->m_bbox, false);
            SetMaterial(n, mat);
        }
    }
    if (m_spinehi) {
        CBBox *box = (CBBox *)m_spinehi->m_node->GetBBox();

        if (box->m_max.x > 3.01f)
            box->m_max.x = 3.01f;
    }
}

/* a gear's model attached to the part the gear names, given an id by its name, named and kept out
 * of the animation (OVL2 0x58B790: SOCOM 1's AddItem(CCharacterGear *), the name's id SOCOM II's own
 * 0x58CA60 inlined where SOCOM 1 called BodyId, the part looked up with its bounds tested) */
bool CZSealBody::AddItem(CCharacterGear *gear)
{
    zdb::CNode *node = gear->CreateInstance(gear->m_name);

    if (!node)
        return false;
    int id = gear->m_partid;
    CZBodyPart *parent;

    if (id < m_body_parts.size() && id >= 0)
        parent = m_body_parts[id];
    else
        parent = 0;

    if (!parent)
        return false;
    CZBodyPart *part = AttachAuxNode(node, parent, &gear->m_offset, &gear->m_rotation);

    part->m_id = func_0058CA60(gear->m_name);
    part->m_node->SetName(gear->m_name);
    part->SetActive(false);
    m_obj_anims.Add(part, gear);
    return true;
}

/* a bare part added: made, attached to none, given an id by its name and kept out of the animation
 * (OVL2 0x58B630: SOCOM 1's AddItem(const char *), the id as the gear's) */
bool CZSealBody::AddItem(const char *name)
{
    CZBodyPart *part = NewBodyPart(0, 0);

    AttachAuxPart(part, 0);
    part->m_id = func_0058CA60(name);
    part->SetActive(false);
    m_obj_anims.Add(part, 0);
    return true;
}

/* the rifle, pistol and launcher found among the made gears, and the launcher's part added to the
 * anims' parts (OVL2 0x58B0F0: SOCOM 1's SetupGear, by its place after SetPartAsGear -- SOCOM 1's found
 * the rifle and the pistol alone) */
void CZSealBody::SetupGear()
{
    m_item_gear[1].m_gear = FindGear("rifle");
    m_item_gear[2].m_gear = FindGear("pistol");
    m_item_gear[4].m_gear = FindGear("launcher");
    m_obj_anims.Add(m_item_gear[4].m_part, m_item_gear[4].m_gear);
}

/* a body part made a gear's: placed and turned as the gear says, oriented on its node, and taken
 * off the body's table of parts (OVL2 0x58B040; SOCOM 1's with the null tests, and CBody::SetBodyPart
 * where SOCOM 1's cleared the table's entry by hand) */
void CZSealBody::SetPartAsGear(CZBodyPart *part, CCharacterGear *gear)
{
    if (!part)
        return;
    if (gear) {
        CQuat q;

        part->m_pos = gear->m_offset;
        part->m_rot = *QFromRot(&q, &gear->m_rotation);
    }
    if (part->m_node)
        part->Orient();
    SetBodyPart(part->m_id, 0);
}

/* an animation set's anims by the item in hand (SOCOM 1's CAnimsByFP: an anim type's pistol and
 * rifle variants) -- only its two lookups (OVL2 0x5E1A10 and 0x5E19D0, the kit lane's range: the
 * shorts at 4 and 6 of an anim type's 8-byte record, -1 past the 312 types) */
class CAnimsByFP;

extern "C" {
AnimType func_005E1A10(CAnimsByFP *anims, AnimType type);
AnimType func_005E19D0(CAnimsByFP *anims, AnimType type);
}

/* a SEAL's animations (SOCOM 1's DWARF: AnimSet, its speeds first) -- only those here */
class AnimSet {
public:
    /* a type's motion script with no weapon (anim.cpp's GetScript(type), OVL2 0x5E3D90: the 3-argument
     * GetScript tail-called with kind 0 -- the kit lane's round 19) */
    CZSIScriptCommon *GetScript(AnimType type);
    /* a type's base type, and whether it carries bit 1 (anim.cpp's, OVL2 0x5E1A90 and 0x5E3AC0) */
    AnimType BaseAnim(AnimType type);
    bool TypeBit1(AnimType type);

    ANIMSET_PARAMS m_aparams;   // 0x0
    unsigned char _34[0x5c - 0x34];
    CAnimsByFP *m_animsbyfp;   // 0x5c (SOCOM 1's at 0x60)
};

extern "C" {
/* the set's own speeds read (SOCOM 1's AnimSet::SetMaxMotion; OVL2 0x5E0450, anim.cpp's) */
void func_005E0450(AnimSet *set);
}

/* an anim type for the item held: the rifle's base type, the pistol's own where the set marks the
 * type, the type itself when either has none; for any other item the base blend's anim type picks
 * (1 the rifle, else the pistol), with no base blend the item kept at 0xF78 (the rifle unless that is
 * the rifle or the pistol) -- OVL2 0x58C820, which 0x58C960 tail-calls with the item in hand; new in
 * SOCOM II, the names ours */
extern "C" AnimType func_0058C820(CZSealBody *seal, AnimType type, SEAL_ITEM_BYTE item)
{
    AnimType r = animTypeUndefined;

    if (item != ITEM_RIFLE && item != ITEM_PISTOL) {
        CZBodyAnimBlend *base = seal->m_blends.m_stack[0];

        if (!base) {
            switch (seal->m_lastitem) {
            case ITEM_PISTOL:
            case ITEM_RIFLE:
                item = seal->m_lastitem;
                break;
            default:
                item = ITEM_RIFLE;
            }
        } else
            item = base->m_animType == 1 ? ITEM_RIFLE : ITEM_PISTOL;
    }
    switch (item) {
    case ITEM_RIFLE: {
        AnimType b = seal->m_animset->BaseAnim(type);

        if (b >= 0)
            return b;
        break;
    }
    case ITEM_PISTOL:
        if (seal->m_animset->TypeBit1(type))
            r = seal->m_animset->m_animsbyfp->PistolAnim(type);
        if (r >= 0)
            return r;
        break;
    }
    return type;
}

extern "C" {
/* (seal_body.cpp's: a blend pushed for an anim type in a state, OVL2 0x588BC0; and the body's top
 * blend, 0x555170 -- RTStack<CZBodyAnimBlend>::Top's OVL2 copy) */
void *func_00588BC0(CZSealBody *seal, AnimType type, int state, int flag, int kind);
void *func_00555170(void *v);
}

/* a new anim set bound, unless none or the bound one without the force flag: a carried body let go
 * (both bodies cleared to their base in the carry states 10 and 9), the blend overlay and the stack
 * cleared, the set swapped in with the old one kept, the weapon id zeroed and the speeds set, the
 * type's blend pushed in the state at full weight and the parts saved (OVL2 0x58C350: SOCOM 1's
 * BindAnimSet(AnimSet *), the order of the carry tests and the calls the same) */
void CZSealBody::BindAnimSet(AnimSet *set, AnimType type, SEAL_STATE state, bool force)
{
    if (!set)
        return;
    if (set == m_animset && !force)
        return;
    if (m_carry) {
        if (m_root->m_parent) {
            if (m_state == 10) {
                m_carry->ClearToBase(false);
                ClearToBase(false);
            }
            DetachBody();
        } else {
            if (m_state == 9) {
                m_carry->ClearToBase(false);
                ClearToBase(false);
            }
            m_carry->DetachBody();
        }
    }
    ClearBlendOverlay();
    ClearStack(0, true);
    m_default_animset = m_animset;
    m_animset = set;
    m_weapon_id = 0;
    SetMaxMotion();
    func_00588BC0(this, type, state, 0, 0);
    ((CZBodyAnimBlend *)func_00555170(&m_blends))->SetRepresentativeWt(1.0f);
    m_desiredState = state;
    SaveParts();
}

/* a script's factor at 0x1C, read by value (SetMaxMotion loads it ahead of the scale's call) */
static inline float Factor(CZSIScriptCommon *a)
{
    return a->m_1c;
}

/* the anim set's speeds set, then the prone crawl's forward and back, its two strafes and the ladder's
 * from their scripts' distance units scaled by the factor, 0 for an anim the set lacks (OVL2 0x58BCB0:
 * SOCOM 1's SetMaxMotion, after GetMaxZMotion in both builds -- SOCOM 1's took the body's anims'
 * maximum velocities).
 * OPEN, 4 instructions (the size right): in the two strafe lookups the target loads the AnimTypes
 * static (a1) before the set (a0, in the call's delay slot), as a right-to-left argument order would;
 * MW loads the set first here. `a` declared ahead of `set` gives their saved registers and the factor
 * through the by-value accessor its $f20 (both needed); tried: the static through a const reference or
 * a short cast, the set in a block-local, an inline member wrapper and a real AnimSet member call (the
 * same order), the product's operands swapped. Round 18, with 0x5E3D90 named AnimSet::GetScript(type)
 * (the kit lane's round 19 predicted the member call would load the type first): spelled as the member
 * call, still 4 -- and so are the type in a block-local (AnimType, short, int), through an inline
 * (`AsType(short)`, `RStrafe()`), the call wrapped in an inline (`Id(...)`, a store helper taking the
 * result) or reached through a static inline taking the body. BlendedClimb's identical call (nested in
 * GetActiveAnim's argument list, after a branch) does load the type first: the order is the scheduler's,
 * set by the block's context, not by the call's spelling */
void CZSealBody::SetMaxMotion()
{
    CZSIScriptCommon *a;

    func_005E0450(m_animset);
    AnimSet *set = m_animset;

    a = set->GetScript((AnimType)27);

    if (a) {
        set->m_aparams.m_prone_max_fwd = a->DistanceScale() * Factor(a);
        set->m_aparams.m_prone_max_back = a->DistanceScale() * Factor(a);
    } else {
        set->m_aparams.m_prone_max_fwd = 0.0f;
        set->m_aparams.m_prone_max_back = 0.0f;
    }
    a = m_animset->GetScript((AnimType)AnimTypes::ProneRStrafe);
    if (a)
        set->m_aparams.m_prone_max_rt = a->DistanceScale() * Factor(a);
    else
        set->m_aparams.m_prone_max_rt = 0.0f;
    a = m_animset->GetScript((AnimType)AnimTypes::ProneLStrafe);
    if (a)
        set->m_aparams.m_prone_max_left = a->DistanceScale() * Factor(a);
    else
        set->m_aparams.m_prone_max_left = 0.0f;
    a = m_animset->GetScript((AnimType)45);
    if (a)
        set->m_aparams.m_ladder_updn = a->DistanceScale() * Factor(a);
    else
        set->m_aparams.m_ladder_updn = 0.0f;
}

extern "C" {
int strcasecmp(const char *a, const char *b);
/* two tables of 72 names each, a byte value's name (OVL2 0x58ED60 and 0x58EF80, string switches) */
const char *func_0058ED60(unsigned char v);
const char *func_0058EF80(unsigned char v);
}

/* a name's value in the first and the second table, 0 for none (OVL2 0x58E020, 0x58E0A0, new in
 * SOCOM II; CCharacterType::InitMultiplayerMessages reads the multiplayer messages through them). The
 * byte value a local declared after the counter: it takes s0, the counter s1 */
extern "C" unsigned char func_0058E020(const char *name)
{
    if (name)
        for (int i = 0; i < 72; i++) {
            unsigned char v = i;

            if (!strcasecmp(func_0058ED60(v), name))
                return v;
        }
    return 0;
}

extern "C" unsigned char func_0058E0A0(const char *name)
{
    if (name)
        for (int i = 0; i < 72; i++) {
            unsigned char v = i;

            if (!strcasecmp(func_0058EF80(v), name))
                return v;
        }
    return 0;
}

/* an anim's reference point when it has one, else the origin (OVL2 0x58C2F0, SOCOM 1's RefPoint by
 * role -- SOCOM 1's searched a list of points; SOCOM II reads the script's own -- so the name stays
 * a placeholder; CheckDILadder and three more call it) */
extern "C" bool func_0058C2F0(CZSealBody *seal, CZSIScriptCommon *anim, CPnt3D *out)
{
    if (anim->m_4b_0) {
        *out = anim->m_4c;
        return true;
    }
    *out = CPnt3D::zero;
    return false;
}

/* a cell search's query and its answer (0x58F230 passes this callback; the names ours) */
struct CCellQuery {
    unsigned char _0[0xc];
    CAiMapLoc m_loc;   // 0xc
    int m_10;   // 0x10
};

struct CCellAnswer {
    bool m_found;   // 0x0
    CAiMapLoc m_loc;   // 0x4
};

extern "C" {
/* (OVL2 0x58FA70: whether the cell can be taken) */
int func_0058FA70(CAiMapLoc *loc, int n);
}

/* whether a cell is on a map: either coordinate off the invalid cell's (an inline bool: the target
 * materialises the `||`) */
static inline bool IsMapped(const CAiMapLoc &loc)
{
    return loc.x != CAiMapLoc::invalid.x || loc.y != CAiMapLoc::invalid.y;
}

/* a cell search's step: a cell taken ends it, found; otherwise the answer is whether the cell it
 * holds is on a map, and the search goes on (OVL2 0x58F8A0, new in SOCOM II: a callback 0x58F230
 * passes by address) */
extern "C" bool func_0058F8A0(CCellQuery *q, CCellAnswer *a)
{
    bool done = true;

    if (!func_0058FA70(&q->m_loc, q->m_10)) {
        a->m_found = IsMapped(a->m_loc);
        done = false;
    } else {
        a->m_loc = q->m_loc;
        a->m_found = true;
    }
    return done;
}

/* a slot for an item kind: 3 for kind 2, 17 for kind 16, 21 for kinds 9-13, none otherwise -- asked
 * only as a bool (OVL2 0x58DFA0, new in SOCOM II; OVL1's 0x22FF30 and 0x230050 and the kit lane's
 * 0x5EAD70 call it). The shared cases are listed in reverse of the target's test order */
enum ITEM_SLOT { ITEM_SLOT_NONE = 0, ITEM_SLOT_3 = 3, ITEM_SLOT_17 = 17, ITEM_SLOT_21 = 21 };

extern "C" bool func_0058DFA0(unsigned char item)
{
    ITEM_SLOT slot = ITEM_SLOT_NONE;

    switch (item) {
    case 2:
        slot = ITEM_SLOT_3;
        break;
    case 16:
        slot = ITEM_SLOT_17;
        break;
    case 10:
    case 11:
    case 9:
    case 13:
    case 12:
        slot = ITEM_SLOT_21;
        break;
    }
    return slot != ITEM_SLOT_NONE;
}

/* an order's name for the HUD (OVL2 0x58E950, a switch over 24 orders; its draft matches) */
extern "C" const char *func_0058E950(unsigned char cmd);

/* an order's name, regroup's spelled apart (OVL2 0x58E900; new in SOCOM II, its name unknown --
 * OVL1's HUD calls it) */
extern "C" const char *func_0058E900(unsigned char cmd)
{
    switch (cmd) {
    case 7:
        return "RE GROUP";
    default:
        return func_0058E950(cmd);
    }
}

/* an anim type for the item in hand: item 1's and item 2's variants, any other the type itself (OVL2
 * 0x58C970, new in SOCOM II; 0x57A330 asks it).
 * OPEN, 2 instructions: the target's default arm is `b epilogue; move v0,a1` -- a branch to the very
 * next instruction -- where MW here lets it fall through (`move v0,a1; nop`). Tried: the return after
 * the switch, a dead return after it, a result local with breaks, the default first, case 2 first, an
 * if/else-if chain (0x54 bytes), a copy of the type in a local. */
extern "C" AnimType func_0058C970(CZSealBody *seal, AnimType type, unsigned char item)
{
    switch (item) {
    case 1:
        return func_005E1A10(seal->m_animset->m_animsbyfp, type);
    case 2:
        return func_005E19D0(seal->m_animset->m_animsbyfp, type);
    default:
        return type;
    }
}

/* the forward and backward limits for the state: standing, crouched, prone or on a ladder (OVL2
 * 0x58BC00, 0x58BB50; SOCOM 1's, the state now CBody's halfword) */
void CZSealBody::GetMaxZMotion(float &a, float &b)
{
    AnimSet *set = m_animset;

    switch (m_state) {
    case stateStand:
        a = set->m_aparams.m_stand_max_fwd;
        b = set->m_aparams.m_stand_max_back;
        break;
    case stateCrouch:
        a = set->m_aparams.m_crouch_max_fwd;
        b = set->m_aparams.m_crouch_max_back;
        break;
    case stateProne:
        a = set->m_aparams.m_prone_max_fwd;
        b = set->m_aparams.m_prone_max_back;
        break;
    case stateClimbLadder:
        a = set->m_aparams.m_ladder_updn;
        b = set->m_aparams.m_ladder_updn;
        break;
    default:
        a = 0.0f;
        b = 0.0f;
        break;
    }
}

void CZSealBody::GetMaxXMotion(float &a, float &b)
{
    AnimSet *set = m_animset;

    switch (m_state) {
    case stateStand:
        a = set->m_aparams.m_stand_max_left;
        b = set->m_aparams.m_stand_max_rt;
        break;
    case stateCrouch:
        a = set->m_aparams.m_crouch_max_left;
        b = set->m_aparams.m_crouch_max_rt;
        break;
    case stateProne:
        a = set->m_aparams.m_prone_max_left;
        b = set->m_aparams.m_prone_max_rt;
        break;
    case stateClimbLadder:
        a = set->m_aparams.m_ladder_updn;
        b = set->m_aparams.m_ladder_updn;
        break;
    default:
        a = 0.0f;
        b = 0.0f;
        break;
    }
}

/* the parts deleted and the list's storage given up (OVL2 0x58AB30: SOCOM 1's ~CObjAnims, seal_cfg.cpp's
 * first row in both builds; the SEAL's destructor calls it. FreeVector's pieces follow it, called out of
 * line: the empty vector 0x58ACA0, clear 0x58AC90, swap 0x58AC50, and the pod vector's destructor --
 * seal.cpp's copy, 0x555040) */
CObjAnims::~CObjAnims()
{
    for (std::vector<CObjAnim *>::iterator i = m_obj_anims.begin(); i != m_obj_anims.end(); ++i)
        delete *i;
    FreeVector(m_obj_anims);
}
