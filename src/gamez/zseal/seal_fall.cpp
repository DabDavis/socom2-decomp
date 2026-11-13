/* seal_fall.cpp -- the SEAL's altitude against the ground: in the air or not, the fall stepped, the
 * root's height followed (new in SOCOM II, the name ours). OVL2 0x59AD30 to 0x59BA7F, between
 * seal_create.cpp's GetSeal (0x59ACD0, SOCOM 1's last of that unit) and seal_dyn.cpp's Load: a unit of
 * its own, as 0x59B440 calls GetSeal out of line where seal_create.cpp's body-part ticks, defined after
 * it, inline it -- under -inline deferred one unit inlines it into every caller, wherever defined.
 * Defined in reverse of their image order (-inline deferred). */

#include "zseal.h"
#include "../zanim/zanim.h"

extern "C" {
/* the top blend's representative flagged (seal_body.cpp's, OVL2 0x57B510), the body's top blend and a
 * halfword's bits (seal.cpp's tail copies, 0x555170 and 0x5551A0) */
bool func_0057B510(CZSealBody *seal);
void *func_00555170(void *stack);
bool func_005551A0(unsigned short flags, unsigned short bits);
/* AnimTypes statics past SOCOM 1's list (OVL1 data 0x3DEF08, 0x3DEF50) */
extern short data_003DEF08;
extern short data_003DEF50;
}

/* the root's height followed: under a flagged anim whose top blend lacks flag 0x20, or anim 46 (an
 * inline: the target builds the answer -- `daddiu 1` / `move zero` -- and then tests it) */
static inline bool FollowsRoot(CZSealBody *seal)
{
    if ((func_0057B510(seal) &&
         !func_005551A0(((CZBodyAnimBlend *)func_00555170(&seal->m_blends))->m_flags, 32)) ||
        seal->IsCurrentBlend((AnimType)46))
        return true;
    return false;
}

/* the SEAL's altitude moved with its root part against a point: under the 0x3DEF08 anim m_fac eased
 * down to 2.2 first (and there the SEAL lands); under the 0x3DEF50 anim the root held at 11.44; always
 * true (OVL2 0x59B870, new in SOCOM II: 0x57A330 calls it with a point of its own; after GetSeal, before
 * seal_dyn.cpp -- the names ours). The height is 0x41370A3E, one ulp above 11.44f (what `2.2f * 5.2`
 * folds to); the helper's answer an if/return (the `||` of a ternary or a plain return materialises
 * the second call's test, sltu/xori) */
extern "C" bool func_0059B870(CZSealBody *seal, const CPnt3D *p)
{
    CZBodyPart *root = seal->GetRootPart();

    if (seal->IsCurrentBlend((AnimType)data_003DEF08)) {
        if (seal->m_1360 > 0.0f)
            return true;
        seal->m_fac += -seal->m_133c * seal->m_2e0;
        if (seal->m_fac < 2.2f)
            seal->m_fac = 2.2f;
        if (seal->m_bit9)
            seal->m_alt += p->y - root->m_pos.y;
        else if (seal->m_fac <= 2.2f) {
            seal->SetAirborne(false);
            seal->m_133c = 0.0f;
        }
    } else if (seal->IsCurrentBlend((AnimType)data_003DEF50))
        root->m_pos.y = 11.440001f;
    else {
        if (seal->m_bit9)
            seal->m_alt += p->y - root->m_pos.y;
        if (FollowsRoot(seal))
            seal->m_alt += p->y - root->m_pos.y;
    }
    return true;
}

extern "C" {
/* whether one of two anim types is the blend on top (seal_body.cpp's, OVL2 0x588030) */
bool func_00588030(CZSealBody *seal);
}

/* a material's extension (int_material.h's CMaterialEx, which zweapon.h's stand-in of the table
 * leaves incomplete -- seal_env.cpp's view, with the fall sound): the sounds and the foot's offset */
struct CMaterialEx {
    char *m_name;   // 0x0
    unsigned char _4[0x18 - 0x4];
    CSnd *m_fallsound;   // 0x18: FALLSOUND
    unsigned char _1c[0x38 - 0x1c];
    float m_foot_step_offset;   // 0x38: FOOT_STEP_OFFSET
};

/* a material's extension, the first one for an index past the table (seal_env.cpp's) */
static inline CMaterialEx *MaterialEx(unsigned int i)
{
    if (i >= CMaterialExArray::m_materialex_list.size())
        i = 0;
    return (CMaterialEx *)CMaterialExArray::m_materialex_list[i];
}

/* the SEAL's fall stepped against the ground's hit: a remote SEAL under the two blends only has its
 * dead top blend put in state 8; else the fall speeds up by gravity (0.8 of it under the 0x3DEF50
 * anim) and the altitude drops by it. Under the 0x3DEF08 anim, once m_1360 has run out, a SEAL above
 * the touch distance gets bit 9; without it the speed is handed in -- the larger kept in m_1364 with
 * bit 10 -- the altitude set to the value given and then to the ground, airborne unless asked to
 * touch. Out of it, bit 9 holds the SEAL (on the ground with bit 10); else on or under the ground the
 * speed is handed back and the SEAL stood on it at its material's foot offset; and with m_1340 past
 * 0.001, or at the ground and not rising, it lands: the ground's normal kept and the polygon's
 * inside flag taken -- not over another SEAL (OVL2 0x59B440, new in SOCOM II, before 0x59B870; the
 * names ours) */
extern "C" void func_0059B440(CZSealBody *seal, zdb::IntersectStruct *hit, float *speed, bool touch,
                              float alt)
{
    bool top;
    CZBodyAnimBlend *blend = (CZBodyAnimBlend *)func_00555170(&seal->m_blends);

    top = func_00588030(seal);

    if (seal->IsRemote() && top) {
        if (!seal->m_isAlive && blend->m_state != 8)
            blend->m_state = 8;
        return;
    }

    float g = theCharacterDynamics.m_gravity;

    if (seal->IsCurrentBlend((AnimType)data_003DEF50))
        g *= 0.8f;
    seal->m_133c += g * seal->m_2e0;
    seal->m_alt -= seal->m_133c * seal->m_2e0;

    float dist = touch ? theCharacterDynamics.m_groundTouchDistance : 0.0f;

    if (seal->IsCurrentBlend((AnimType)data_003DEF08)) {
        if (!(seal->m_1360 <= 0.0f))
            return;
        if (!seal->m_bit10 && !(seal->m_alt <= hit->m_pos.y + dist))
            seal->SetBit9(true);
        if (!seal->m_bit9) {
            if (seal->m_bit10 && !(*speed <= seal->m_1364))
                seal->m_1364 = *speed;
            seal->m_133c = *speed;
            seal->m_alt = alt;
            seal->m_133c += 9.81f * 10.0f * seal->m_2e0;
            seal->SetAirborne(!touch);
            seal->m_alt = hit->m_pos.y;
        }
        return;
    }
    if (seal->m_bit9) {
        if (seal->m_bit10) {
            seal->SetAirborne(false);
            seal->m_alt = hit->m_pos.y;
        }
        return;
    }
    bool below = seal->m_alt <= hit->m_pos.y ? 1 : 0;

    if (below) {
        if (!(seal->m_133c < 0.0f)) {
            *speed = seal->m_133c;
            seal->m_133c = 0.0f;
        }
        seal->m_alt = hit->m_pos.y + MaterialEx(hit->GetMaterial())->m_foot_step_offset;
    }
    if (!(seal->m_1340 <= 0.001f) || (seal->m_alt <= hit->m_pos.y + dist && !(seal->m_133c < 0.0f))) {
        seal->SetAirborne(false);
        if (!CZSealBody::GetSeal(hit->m_node)) {
            seal->m_1344 = hit->m_norm;
            if (hit->m_poly && hit->m_poly->m_flags.m_reserved26)
                seal->m_inside = hit->m_poly->m_flags.m_inside != 0;
        }
    }
}

/* the body's bits set through setters: a constant true is `daddiu 1 << n`, a false `(0 & 1) << n`
 * (the names ours; zseal.h's SetLanded, SetBit9 and the like have the same shape) */
static inline void SetUsedVelY(CZSealBody *seal, bool on) { seal->m_used_vel_y = on; }
static inline void SetFixSkelY(CZSealBody *seal, bool on) { seal->m_fix_skel_y = on; }
static inline void SetBit8(CZSealBody *seal, bool on) { seal->m_bit8 = on; }

/* the SEAL's altitude for the frame: under the 0x3DEF08 anim, or with bit 9, the one it has (landed
 * with bit 9 under the anim). Under a flagged top blend -- not anim 36 past a fifth of the way, nor
 * anim 44 with its blend's flag 0x20 -- the velocity's step from the node's height, the root part's
 * height added once the first time unless the root is followed, the root then held at 11.487 (at most
 * that when followed), the velocity bit set. Else the velocity bit cleared, and a skeleton to fix
 * once: the root's height dropped to the altitude's place under m_fac and the node moved there (the
 * root's drop handed back when it is already lower); landed once on the ground (OVL2 0x59AFD0, new in
 * SOCOM II; the names ours). The height is 0x4137CAC0, one ulp under 11.487f: `1.1487f * 10.0f` folded */
extern "C" float func_0059AFD0(CZSealBody *seal)
{
    if (seal->IsCurrentBlend((AnimType)data_003DEF08)) {
        if (seal->m_bit9)
            seal->SetLanded(true);
        return seal->m_alt;
    }
    if (seal->m_bit9)
        return seal->m_alt;
    if (func_0057B510(seal)) {
        if ((seal->IsCurrentBlend((AnimType)36) &&
             !(((CZBodyAnimBlend *)func_00555170(&seal->m_blends))->m_tfrac <= 0.2f)) ||
            (seal->IsCurrentBlend((AnimType)44) &&
             func_005551A0(((CZBodyAnimBlend *)func_00555170(&seal->m_blends))->m_flags, 32))) {
            SetUsedVelY(seal, false);
            return seal->m_alt;
        }

        float y = seal->m_velM.y * seal->m_2e0;
        CZBodyPart *root = seal->GetRootPart();

        if (!seal->m_used_vel_y) {
            if ((seal->IsCurrentBlend((AnimType)46) &&
                 func_005551A0(((CZBodyAnimBlend *)func_00555170(&seal->m_blends))->m_flags, 32)) ||
                FollowsRoot(seal))
                y += seal->m_posW.y;
            else
                y = root->m_pos.y + (y + seal->m_posW.y) - 1.1487f * 10.0f;
        } else
            y += seal->m_posW.y;
        if (FollowsRoot(seal)) {
            if (!(root->m_pos.y <= 1.1487f * 10.0f))
                root->m_pos.y = 1.1487f * 10.0f;
        } else
            root->m_pos.y = 1.1487f * 10.0f;
        SetUsedVelY(seal, true);
        return y;
    }
    SetUsedVelY(seal, false);
    if (seal->m_fix_skel_y) {
        CZBodyPart *root = seal->GetRootPart();

        SetFixSkelY(seal, false);
        SetBit8(seal, true);

        float d = seal->m_fac - root->m_pos.y;

        if (d < seal->m_alt) {
            root->m_pos.y = seal->m_fac - seal->m_alt;

            CPnt3D saved = seal->GetRootPart()->m_saved_pos;

            saved.y = root->m_pos.y;
            seal->GetRootPart()->m_saved_pos = saved;

            CPnt3D pos = seal->m_posW;

            pos.y = seal->m_alt;
            seal->GetNode()->SetPosition(&pos);
            return seal->m_alt;
        }
        return d;
    }
    if (!seal->m_airborne)
        seal->SetLanded(true);
    return seal->m_alt;
}

extern "C" {
/* the height above the ground a SEAL counts as in the air from (OVL1 bss 0x44C264), the probe for
 * ground under a falling SEAL and the landing's hit taken (seal.cpp's OVL2 0x54A8B0 and 0x5B0C10) */
#define data_0044C264 (*(float *)((char *)&::theCharacterDynamics + 0x14))   /* theCharacterDynamics+0x14 */
bool func_0054A8B0(CZSealBody *seal);
void func_005B0C10(CZSealBody *seal, zdb::IntersectStruct *hit);
}

/* whether the SEAL is in the air over a point: in the 0x3DEF08 anim, or with no air bit yet, at the
 * in-air height above the point it is airborne (the landed bit 8 cleared, and under the anim bit 9 set and 10
 * cleared); else, out of a flagged anim and played here, the ground probed when it is 5 above (3
 * prone); a SEAL with bit 9 and not 10 that sinks under the point while m_1360 has run out is set on
 * it -- bits 10 and 8 set, 9 cleared, the landing taken (OVL2 0x59AD30, new in SOCOM II: 0x5B0420
 * calls it; after GetSeal -- the name unknown) */
extern "C" bool func_0059AD30(CZSealBody *seal, const CPnt3D *pos)
{
    float h = data_0044C264;

    if (seal->IsCurrentBlend((AnimType)data_003DEF08) && !(seal->m_alt < pos->y + h)) {
        seal->SetAirborne(true);
        seal->SetLanded(false);
        seal->SetBit9(true);
        seal->SetBit10(false);
    }
    if (!seal->m_airborne) {
        if (!(seal->m_alt < pos->y + h)) {
            seal->SetAirborne(true);
            seal->SetLanded(false);
            if (!func_0057B510(seal) && !seal->IsRemote())
                func_0054A8B0(seal);
        } else if (!func_0057B510(seal) && !seal->IsRemote()) {
            float d = 5.0f;

            if (seal->m_state == stateProne)
                d = 3.0f;
            if (!(seal->m_alt <= pos->y + d))
                func_0054A8B0(seal);
        }
    }
    if (seal->m_bit9 && !seal->m_bit10) {
        float y = pos->y;

        if (seal->m_alt < y && seal->m_1360 <= 0.0f) {
            seal->m_alt = y;
            seal->SetBit9(false);
            seal->SetBit10(true);
            seal->SetLanded(true);
            func_005B0C10(seal, 0);
        }
    }
    return seal->m_airborne;
}
