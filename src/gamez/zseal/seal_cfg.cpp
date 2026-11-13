/* seal_cfg.cpp -- the SEAL's configuration: its gear, its animation set's limits (SOCOM 1's unit,
 * __sinit_seal_cfg.cpp; its strings put SetupGear there, GetMaxXMotion and GetMaxZMotion after
 * it). OVL2 0x58AB30 (~CObjAnims) to its template tail's end, 0x58DBAF -- the orders' names and
 * cell searches after it are seal_orders.cpp's. Defined in reverse of their image order (-inline
 * deferred). */

#include "zseal.h"
#include <map>

/* a blend's anims (anim.cpp's CBlendable, the kit lane's: only the test here) and its four speed
 * bands, each an anim and the speeds it covers, the first unused band's type -1 (MixAnims reads
 * them; the names ours) */
class CBlendable {
public:
    bool Contains(AnimType type);

    struct BAND {
        float m_min;   // 0x0
        float m_max;   // 0x4
        short m_type;   // 0x8
    } m_band[4];
};

/* the animation set's per-weapon tables (anim.cpp's CAnimsByFP, the kit lane's: only the pistol's
 * lookup) and the set's pointer to them at 0x5C (a view of AnimSet; the name ours) */
class CAnimsByFP {
public:
    AnimType PistolAnim(AnimType type) const;
    /* an anim type's rifle and pistol variants (OVL2 0x5E1A10, 0x5E19D0, anim.cpp's: the shorts at 4 and
     * 6 of the type's 8-byte record, -1 past the 312 types) */
    AnimType RifleFPAnim(AnimType type) const;
    AnimType PistolFPAnim(AnimType type) const;
};

struct AnimSetByFP {
    unsigned char _0[0x5c];
    CAnimsByFP *m_byfp;   // 0x5c
};

/* the body parts' names to their ids (SOCOM 1's seal_cfg.cpp's bodyPartNames_map, a map<string, int>:
 * OVL1 bss 0x44AF60, fts_ovl2statics.cpp's). This unit's tail emits OVL2's copies of the tree's find
 * 0x58D090, find_or_insert 0x58CEF0 (operator[]'s half) and insert_node_at 0x58D2B0 -- the material
 * unit's word for word (the MSL headers' round 22: include/map as it stands) -- as it does the string's
 * constructor and destructor (0x58CE70, 0x53FAD0: ovl-copy rows) */
extern std::map<std::string, int> bodyPartNames_map;

extern "C" {
/* whether the map is filled (OVL1 bss 0x44AF70), and the next id handed out (OVL1 data 0x3DE930) */
extern bool data_0044AF70;
extern int data_003DE930;
/* the map filled (OVL2 0x58CC30, below) */
void func_0058CC30();
}

extern "C" {
/* the next body part id handed out (OVL1 data 0x3DE928) */
extern unsigned int data_003DE928;
}

extern "C" {
/* the ids of the parts the map names first (OVL1 data 0x3DF140-0x3DF168, 8 bytes apart): the root's,
 * then from the top of the parts' object slots the four kept back and the last -- the rifle's, the
 * pistol's, the grenade's, the weapon's and rifle_out's (the names ours) */
extern int data_003DF140;
extern int data_003DF148;
extern int data_003DF150;
extern int data_003DF158;
extern int data_003DF160;
extern int data_003DF168;
}

/* the SEALs' dynamics, one for all (OVL2 0x58CE60: SOCOM 1's GetDynamics, its seal_cfg.cpp's last
 * function as here -- defined first, before the unit's template tail; no function of the unit calls it) */
CharacterDynamics *CZSealBody::GetDynamics()
{
    return &theCharacterDynamics;
}

/* the body parts' names filled in once: no part handed out yet, skel_root the first, the four kept
 * back and rifle_out at the top of the slots; the next id after them (OVL2 0x58CC30, new in SOCOM II:
 * 0x58CB30 and 0x58CA60 call it -- the name ours) */
extern "C" void func_0058CC30()
{
    if (data_0044AF70)
        return;
    data_003DE928 = 0;
    data_003DF140 = 0;
    data_003DE930 = siNumBodyObjects;
    bodyPartNames_map[std::string("skel_root")] = data_003DF140;
    data_003DE928++;
    data_003DF148 = siNumBodyObjects - 4;
    bodyPartNames_map[std::string("rifle")] = data_003DF148;
    data_003DF150 = siNumBodyObjects - 3;
    bodyPartNames_map[std::string("pistol")] = data_003DF150;
    data_003DF158 = siNumBodyObjects - 2;
    bodyPartNames_map[std::string("grenade")] = data_003DF158;
    data_003DF160 = siNumBodyObjects - 1;
    bodyPartNames_map[std::string("weapon")] = data_003DF160;
    data_003DF168 = siNumBodyObjects;
    bodyPartNames_map[std::string("rifle_out")] = data_003DF168;
    data_003DE930++;
    data_0044AF70 = true;
}

/* a body part's id by its name: the map filled once, the name found -- or given the next id while
 * the parts' object slots last, the last four kept back, else -1 (OVL2 0x58CB30, new in SOCOM II:
 * BodyAddAnim, NewBodyPart, ReadMotionRdr and CCharacterType::Open ask it; the names ours) */
/* (zbody.h declares it, as an int) */
extern "C" int func_0058CB30(const char *name)
{
    if (!data_0044AF70)
        func_0058CC30();
    std::map<std::string, int>::iterator it = bodyPartNames_map.find(std::string(name));

    if (it != bodyPartNames_map.end())
        return (*it).second;

    unsigned int id = data_003DE928;

    if (id >= siNumBodyObjects - 4)
        return -1;
    bodyPartNames_map[std::string(name)] = id;
    data_003DE928++;
    return id;
}

/* the body parts' names forgotten (SOCOM 1's ClearBodyPartNames_Map, its demo's 0x280E80, between
 * BodyWepPart and BodyId as here): nothing calls it in SOCOM II, so the linker stripped it
 * (src/ABSENT.txt) -- but the map's clear it instantiates is this unit's tail's, and survives: OVL2
 * 0x58D150 with its destroy 0x58D450 and the node pieces 0x58D6D0-0x58D737, which anim.cpp's 0x5E30F0
 * calls on the anim sets' two maps (an earlier unit's copy, as ai_events.cpp's stripped Clear leaves
 * its range erase) */
void ClearBodyPartNames_Map()
{
    bodyPartNames_map.clear();
    data_0044AF70 = false;
}

/* a name's id: the map filled once, the name found -- or given the next id (OVL2 0x58CA60, new in
 * SOCOM II; the names ours). The end taken into a local: compared straight (`it != map.end()`), the
 * end() an argument of the iterator's != sits a level deeper, and both AddItems -- which inline this --
 * call the tree's tail() out of line (8 bytes each; the out-of-line copy matches either way) */
extern "C" int func_0058CA60(const char *name)
{
    if (!data_0044AF70)
        func_0058CC30();
    std::map<std::string, int>::iterator it = bodyPartNames_map.find(std::string(name));
    std::map<std::string, int>::iterator e = bodyPartNames_map.end();

    if (it != e)
        return (*it).second;

    int id = data_003DE930;

    bodyPartNames_map[std::string(name)] = id;
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

/* (a material's index by name, -1 unknown, is CMaterialExArray::ResolveMaterialIndex, zweapon.h's:
 * OVL1 0x2DE9E0; node_util.cpp's DI setters) */
void SetDIBBox(zdb::CNode *node, const CBBox *bbox, bool b);
void SetMaterial(zdb::CNode *node, int material);

/* every bone's node (the parts past the last four object slots left out) given a box DI of the
 * PERSON material, and the upper spine's box capped at 3.01 across (OVL2 0x58B960: SOCOM 1's
 * AddDIPolys, after the AddItems in both builds -- SOCOM 1 skipped the rifle, pistol, grenade and
 * weapon parts by pointer, SOCOM II by id; the cap is new) */
void CZSealBody::AddDIPolys()
{
    std::vector<CZBodyPart *> &parts = m_body_parts;
    int mat = CMaterialExArray::ResolveMaterialIndex("PERSON");

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

/* the eyes and their lids: each aux node found made a part, on the head, with its name's id; none, no part
 * (SOCOM 1's SetupEyes, which hung them on the node's own part; OVL2 0x58B1A0, between SetupGear and the
 * AddItems in both builds) */
void CZSealBody::SetupEyes()
{
    CNode *node = FindAuxNode("right_eyeball", true);

    if (node) {
        m_reyeball = NewBodyPart(0, 0);
        m_reyeball->m_node = node;
        m_reyeball->m_parent = m_head;
        m_reyeball->m_id = func_0058CA60("right_eyeball");
    } else
        m_reyeball = 0;
    node = FindAuxNode("left_eyeball", true);
    if (node) {
        m_leyeball = NewBodyPart(0, 0);
        m_leyeball->m_node = node;
        m_leyeball->m_parent = m_head;
        m_leyeball->m_id = func_0058CA60("left_eyeball");
    } else
        m_leyeball = 0;
    node = FindAuxNode("right_lid", true);
    if (node) {
        m_reyelid = NewBodyPart(0, 0);
        m_reyelid->m_node = node;
        m_reyelid->m_parent = m_head;
        m_reyelid->m_id = func_0058CA60("right_lid");
    } else
        m_reyelid = 0;
    node = FindAuxNode("left_lid", true);
    if (node) {
        m_leyelid = NewBodyPart(0, 0);
        m_leyelid->m_node = node;
        m_leyelid->m_parent = m_head;
        m_leyelid->m_id = func_0058CA60("left_lid");
    } else
        m_leyelid = 0;
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

/* a part's object in the anims' parts, by its id, or none (SOCOM 1's GetObjAnim, out of line there) */
static inline CObjAnim *FindObjAnim(std::vector<CObjAnim *> &v, int id)
{
    typedef std::vector<CObjAnim *>::iterator iter;
    iter i((CObjAnim **)v.data());
    iter e((CObjAnim **)v.data() + v.size());

    for (; i != e; ++i) {
        CObjAnim *o = *i;

        if (id == o->m_part->m_id)
            return o;
    }
    return 0;
}

/* a part's gear, if it has an object (an inline) */
static inline CCharacterGear *FindObjGear(std::vector<CObjAnim *> &v, CZBodyPart *part)
{
    if (part) {
        const int id = part->m_id;
        CObjAnim *oo = FindObjAnim(v, id);

        if (oo)
            return oo->m_gear;
        else
            return 0;
    } else
        return 0;
}

/* a body part by id, none out of range; a script's object of a body part, none out of range (inlines) */
static inline CZBodyPart *BodyPartOf(CZSealBody *seal, int i)
{
    if (i < seal->m_body_parts.size() && i >= 0)
        return seal->m_body_parts[i];
    return 0;
}

static inline CZSIObject *ObjectOf(CZSIScriptCommon *common, int i)
{
    if (i >= 0 && i < common->m_objects_for_body.size())
        return common->m_objects_for_body[i];
    return 0;
}

/* an anim's objects past the body's own (the aux parts, four short of the body's objects) given to the
 * table of parts: an object's part in, the part it replaces made a gear's; no object, the part made a
 * gear's and its slot emptied (SOCOM 1's ConfigureAuxPartAnim, on the common; OVL2 0x58ACC0, after
 * ~CObjAnims and before SetPartAsGear in both builds -- PushBlend and PopBlend call it in both) */
void CZSealBody::ConfigureAuxPartAnim(CZSIScriptCommon *common)
{
    if (!common)
        return;
    CZBodyPart *obj;
    for (int i = common->m_nBodyObjects; i < siNumBodyObjects - 4; i++) {
        CZBodyPart *part = BodyPartOf(this, i);
        CCharacterGear *gear = FindObjGear(m_obj_anims.m_obj_anims, part);

        if (ObjectOf(common, i)) {
            int id = ObjectOf(common, i)->m_body_part_id;
            CObjAnim *o = FindObjAnim(m_obj_anims.m_obj_anims, id);

            if (o)
                obj = o->m_part;
            else
                obj = 0;
            if (part && part != obj)
                SetPartAsGear(part, gear);
            SetBodyPart(i, obj);
        } else {
            if (part && gear)
                SetPartAsGear(part, gear);
            SetBodyPart(i, 0);
        }
    }
}

/* (an animation set's anims by the item in hand -- SOCOM 1's CAnimsByFP: its two lookups declared
 * above, called by name) */

/* a SEAL's animations (SOCOM 1's DWARF: AnimSet, its speeds first) -- only those here */
class AnimSet {
public:
    /* a type's motion script with no weapon (anim.cpp's GetScript(type), OVL2 0x5E3D90: the 3-argument
     * GetScript tail-called with kind 0 -- the kit lane's round 19) */
    CZSIScriptCommon *GetScript(AnimType type);
    CZSIScriptCommon *GetScript(AnimType type, int kind);
    /* a type's base type, and whether it carries bit 1 (anim.cpp's, OVL2 0x5E1A90 and 0x5E3AC0) */
    AnimType BaseAnim(AnimType type);
    bool TypeBit1(AnimType type);
    /* the set's own speeds read (SOCOM 1's AnimSet::SetMaxMotion; OVL2 0x5E0450, anim.cpp's) */
    void SetMaxMotion();

    /* the set's name (out of line in the SEALs' units, OVL2 0x578940) and a set by its name in the
     * list, or 0 (SOCOM 1's Find, anim.cpp's: inlined one level in 0x58C600, the list's iterator pieces
     * called -- OVL2 0x5788C0-0x578980, seal_body.cpp's copies). GetName is five inline levels high:
     * FITTED, not read off the image (the kit's round 38 in anim.cpp, the same chain) -- the depth
     * the target's Find needs, so that its whole tree is over five and 0x58C600 inlines Find one
     * level and calls every piece, as the target does; no function calls a piece below GetName or
     * c_str, so the chain is GetName's or c_str's in the original */
    std::string &Name0() { return m_name; }
    std::string &Name1() { return Name0(); }
    std::string &Name2() { return Name1(); }
    std::string &Name3() { return Name2(); }
    std::string &GetName() { return Name3(); }
    static AnimSet *Find(const char *name)
    {
        std::vector<AnimSet *>::iterator i;

        for (i = list.begin(); i != list.end(); i++)
            if (!strcmp(name, (*i)->GetName().c_str()))
                return *i;
        return 0;
    }

    /* every set (SOCOM 1's AnimSet::List; OVL1 bss 0x44D430) */
    static std::vector<AnimSet *> list;

    ANIMSET_PARAMS m_aparams;   // 0x0
    unsigned char _34[0x38 - 0x34];
    std::string m_name;   // 0x38
    unsigned char _44[0x5c - 0x44];
    CAnimsByFP *m_animsbyfp;   // 0x5c (SOCOM 1's at 0x60)
};

extern "C" {
/* a SEAL posed by one anim alone (seal_anim.cpp's, OVL2 0x577DF0) */
void func_00577DF0(CZSealBody *seal, ACTIVE_ANIM *anim);
void *func_00555170(void *v);
int strcmp(const char *a, const char *b);
}

/* the body readied: unless asked to keep them, every anim of the common archive and of the map's added
 * to the body; the type's first anim set found by name and bound standing with its type-1 blend, kept
 * as the default too; unless asked, the SEAL posed by the top blend's first anim and the parts saved
 * (OVL2 0x58C600, new in SOCOM II: the constructor calls it -- the name unknown). The target inlines
 * Find ONE level -- every call in it (the iterator's default constructor, begin, operator=, end, !=,
 * *, GetName, c_str, postfix ++) left a call to seal_body.cpp's copies 0x5788C0-0x578980, as in
 * 0x5784A0 and anim.cpp's 0x5E3940: byte-exact with GetName five high (the view above, fitted), the
 * loops' locals as they stand (the first archive, then its counter, then its count; the second's
 * count declared first, bare -- each loop's registers rotate: build/scratch/seal-r37/v_58c600.py, 1
 * of 10 orders) */
extern "C" bool func_0058C600(CZSealBody *seal, bool keep)
{
    if (!keep) {
        zBoneSetCurArchive(zBoneArchive());
        CScriptArchive *a = zBoneCurArchive();
        int i;
        int n = a->m_byRdr.size();

        for (i = 0; i < n; i++)
            seal->BodyAddAnim(a, i);
        int nm;
        zBoneSetCurArchive(zBoneMapArchive());
        CScriptArchive *m = zBoneCurArchive();
        nm = m->m_byRdr.size();

        for (int j = 0; j < nm; j++)
            seal->BodyAddAnim(m, j);
    }
    zBoneSetCurArchive(0);
    AnimSet *set = AnimSet::Find(seal->m_charType->m_anim_sets[0]);

    seal->BindAnimSet(set, (AnimType)1, stateStand, false);
    seal->m_530 = set;
    seal->m_default_animset = set;
    if (!keep) {
        ACTIVE_ANIM *first = ((CZBodyAnimBlend *)func_00555170(&seal->m_blends))->m_anims.Begin();

        seal->m_interp1 = 0.0f;
        seal->m_interp2 = 0.0f;
        func_00577DF0(seal, first);
        seal->SaveParts();
    }
    return true;
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

/* a new anim set bound in the standing state with its type-1 blend (OVL2 0x58C4A0: SOCOM 1's
 * BindAnimSet(AnimSet *), SOCOM II's force flag appended -- SOCOM 1's five calls from the AI states,
 * CAiSScript::OnSignal's three and CAiSSurrender's Tick and OnSignal, are SOCOM II's calls to this
 * row; the full binder below is SOCOM 1's body with the type and the state added) */
void CZSealBody::BindAnimSet(AnimSet *set, bool force)
{
    BindAnimSet(set, (AnimType)1, (SEAL_STATE)0, force);
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

/* the anims of the speed bands a speed falls in mixed into a list (OVL2 0x58BDF0: SOCOM 1's MixAnims,
 * 0x4C8 there, by its place after SetMaxMotion and its body; SOCOM II's has no flag): each band the
 * speed (100 times its magnitude, by the band's first script's scale) lies in takes its anim -- the top
 * blend's own, taken off its list, else a new one of the set's script for the weapon -- played at the
 * speed's rate; none, the nearest band's end anim at the edge's rate (negated for a reversed speed);
 * two, weighed between them by where the speed lies, the unused one put back. The anims taken
 * (RTList<ACTIVE_ANIM>::Remove: its OVL2 copy 0x58D190 in this unit's tail, byte-exact).
 * OPEN, 6 instructions (the MW lane's round 81: 245 to 6, every register and spill slot the target's):
 * the blendable test `m_type > -1 ? blendable : 0` (round 80's tell: the target's `slti; movn`, where
 * `< 0 ? 0 :` branches); the anim id an `int id` (FindAnim's argument, kept in s6 and stored to m_id --
 * the extra saved register that spills the list to 252), its pool's blend taken first; the edge types
 * AnimType, the max's copied from the min's (as shorts each copy took a sign-extension and maxType a
 * 2-byte slot: an AnimType assigned a constant spills to a 16-byte one, 224 and 208); maxEnd declared
 * ahead of minEnd and the band's type declared outside the walk (the FPR and GPR numbering); the first
 * script's factor in a float local. Left: the frame (304 against 272: 32 bytes above `taken` that
 * SOCOM 1's 256-byte frame has too -- `taken[12]` gives it, not committed), the first GetScript's
 * argument read ahead of the set (the target reads the set first) and the pool's address one slot
 * earlier. Round 33: the edge types set through an inline taking a short reference, or through `*&`,
 * change nothing (MW folds both); the blendable test as an if-assign loses 6 */
int CZSealBody::MixAnims(CActiveAnimList *list, CBlendable *blendable, float scale, float wt)
{
    if (wt <= 0.0f)
        return 0;
    blendable = blendable->m_band[0].m_type > -1 ? blendable : 0;
    if (!blendable)
        return 0;
    CBlendable::BAND *band = blendable->m_band;
    int n = 0;
    float speed = fabsf(scale);
    float lo = 100.0f;
    float hi = 0.0f;
    AnimType minType, maxType;

    minType = (AnimType)-1;
    maxType = minType;
    float maxEnd = hi;
    float minEnd = lo;
    ACTIVE_ANIM *taken[4];
    float f = m_animset->GetScript((AnimType)band->m_type)->m_1c;
    float s = 100.0f * speed * f;
    CActiveAnimPool *pool = &m_active_anim_pool;

    AnimType type;

    for (int i = 0; i < 4; i++, band++) {
        type = (AnimType)band->m_type;

        if (type == -1)
            break;
        CZSIScriptCommon *script = m_animset->GetScript((AnimType)type);
        float min = band->m_min;
        float max = band->m_max;

        if (min < minEnd) {
            minEnd = min;
            minType = type;
        }
        if (max > maxEnd) {
            maxEnd = max;
            maxType = type;
        }
        if (min <= s && s <= max) {
            CZBodyAnimBlend *blend = (CZBodyAnimBlend *)func_00555170(&m_blends);
            int id = type;
            ACTIVE_ANIM *a = blend->FindAnim(id);

            if (a) {
                ((CZBodyAnimBlend *)func_00555170(&m_blends))->m_anims.Remove(a);
                taken[n++] = a;
                a->ClearOverlays(this);
            } else {
                CZSIScriptCommon *sc = m_animset->GetScript((AnimType)type, m_weapon_id);

                if (!sc)
                    sc = m_animset->GetScript((AnimType)type, 0);
                if (sc)
                    a = pool->GetActiveAnim(sc, -1);
                a->m_id = id;
                taken[n++] = a;
            }
            a->SetPlayback(scale * script->m_1c);
            if (max < lo)
                lo = max;
            if (min > hi)
                hi = min;
        }
    }
    if (n == 0) {
        if (s < minEnd) {
            if (speed != scale)
                minEnd = -minEnd;
            ACTIVE_ANIM *a = pool->GetActiveAnim(m_animset->GetScript((AnimType)minType), -1);

            taken[n++] = a;
            a->SetPlayback(minEnd / 100.0f);
        } else if (s > maxEnd) {
            if (speed != scale)
                maxEnd = -maxEnd;
            ACTIVE_ANIM *a = pool->GetActiveAnim(m_animset->GetScript((AnimType)maxType), -1);

            taken[n++] = a;
            a->SetPlayback(maxEnd / 100.0f);
        }
    }
    if (n <= 0)
        return 0;
    if (wt < 1.0f)
        RenormAnims(list, 1.0f - wt);
    if (n == 2) {
        float t = (s - hi) / (lo - hi);

        if (t >= 1.0f) {
            PushAnim(list, taken[1], wt);
            pool->PutBack(taken[0]);
        } else if (t > 0.0f) {
            PushAnim(list, taken[0], (1.0f - t) * wt);
            PushAnim(list, taken[1], t * wt);
        } else {
            PushAnim(list, taken[0], wt);
            pool->PutBack(taken[1]);
        }
    } else
        PushAnim(list, taken[0], wt);
    return n;
}

/* the anim set's speeds set, then the prone crawl's forward and back, its two strafes and the ladder's
 * from their scripts' distance units scaled by the factor, 0 for an anim the set lacks (OVL2 0x58BCB0:
 * SOCOM 1's SetMaxMotion, after GetMaxZMotion in both builds -- SOCOM 1's took the body's anims'
 * maximum velocities). `a` declared ahead of `set` gives their saved registers and the factor through
 * the by-value accessor its $f20. In the two strafe lookups the target loads the AnimTypes static (a1)
 * before the set (a0, in the call's delay slot): the two statics are AnimTypes themselves, passed with
 * no conversion (round 34) -- as shorts cast to the enum, MW loads the set first (4 off since round 17,
 * whatever the call's spelling); an argument needing a conversion is evaluated after `this`, one
 * needing none before it (0x5B14E0's unsigned index the other way round). */
void CZSealBody::SetMaxMotion()
{
    CZSIScriptCommon *a;

    m_animset->SetMaxMotion();
    AnimSet *set = m_animset;

    a = set->GetScript((AnimType)27);

    if (a) {
        set->m_aparams.m_prone_max_fwd = a->DistanceScale() * Factor(a);
        set->m_aparams.m_prone_max_back = a->DistanceScale() * Factor(a);
    } else {
        set->m_aparams.m_prone_max_fwd = 0.0f;
        set->m_aparams.m_prone_max_back = 0.0f;
    }
    a = m_animset->GetScript(AnimTypes::ProneRStrafe);
    if (a)
        set->m_aparams.m_prone_max_rt = a->DistanceScale() * Factor(a);
    else
        set->m_aparams.m_prone_max_rt = 0.0f;
    a = m_animset->GetScript(AnimTypes::ProneLStrafe);
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

/* an anim type for the item in hand: item 1's and item 2's variants, any other the type itself (OVL2
 * 0x58C970, new in SOCOM II; 0x57A330 asks it). Its neighbour 0x58C9E0's shape (round 34): the two
 * calls into a result local, broken out to `return r`, the default returning the type straight -- the
 * empty `return r` block between the default's branch and the epilogue is what leaves the target's
 * `b epilogue; move v0,a1` a branch to the very next instruction (every case returning straight lets
 * the default fall through, 2 off). */
extern "C" AnimType func_0058C970(CZSealBody *seal, AnimType type, unsigned char item)
{
    AnimType r;

    switch (item) {
    case 1:
        r = seal->m_animset->m_animsbyfp->RifleFPAnim(type);
        break;
    case 2:
        r = seal->m_animset->m_animsbyfp->PistolFPAnim(type);
        break;
    default:
        return type;
    }
    return r;
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
