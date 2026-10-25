/* seal_cfg.cpp -- the SEAL's configuration: its gear, its animation set's limits (SOCOM 1's unit,
 * __sinit_seal_cfg.cpp; its strings put SetupGear there, GetMaxXMotion and GetMaxZMotion after
 * it). OVL2 before seal_command.cpp (0x58FBE0). Defined in reverse of their image order
 * (-inline deferred). */

#include "zseal.h"

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
    ANIMSET_PARAMS m_aparams;   // 0x0
    unsigned char _34[0x5c - 0x34];
    CAnimsByFP *m_animsbyfp;   // 0x5c (SOCOM 1's at 0x60)
};

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
