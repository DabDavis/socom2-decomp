/* seal_body.cpp -- the SEAL's body: its actions, stances and motion (SOCOM 1's unit,
 * __sinit_seal_body.cpp; its strings put Action, RestrainBuddy, OKToStand and StanceChanged
 * there). OVL2 from about 0x578990 (RestrainBuddy) to seal_cfg.cpp. Defined in reverse of their
 * image order (-inline deferred). */

#include "zseal.h"

/* an animation set (anim.cpp's, the kit lane's, which declares the whole class: only the members
 * called here -- their rows named, OVL2 0x5E3B20-0x5E3E80) */
class AnimSet {
public:
    int AnimId(AnimType type) const;
    CZSIScriptCommon *GetVariantScript(AnimType type, int kind, int index);
    CZSIScriptCommon *GetScript(AnimType type);
    CZSIScriptCommon *GetScript(AnimType type, int kind);
    int GetVariantIndex(AnimType type, int kind, int anim);
    AnimType BaseAnim(AnimType type);
    /* the set's name (out of line here, OVL2 0x578940: this unit emits Find's pieces first, right
     * after 0x5784A0) -- five inline levels high: FITTED, not read off the image (the kit's round 38
     * in anim.cpp, the same chain), the depth the target's Find needs so that its whole tree is over
     * five and 0x5784A0 inlines Find one level, calling every piece, as the target does */
    std::string &Name0() { return m_name; }
    std::string &Name1() { return Name0(); }
    std::string &Name2() { return Name1(); }
    std::string &Name3() { return Name2(); }
    std::string &GetName() { return Name3(); }
    /* a set by its name in the list, or 0 (SOCOM 1's Find, anim.cpp's: its iterator pieces called --
     * OVL2 0x5788C0-0x578980, this unit's copies) */
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

    unsigned char _0[0x38];
    std::string m_name;   // 0x38
};

/* THE UNIT'S STATICS, in the order its initialiser builds them (__sinit_seal_body.cpp, OVL2 0x668940):
 * three offsets new in SOCOM II (OVL2 data 0x65D008, 0x65D018, 0x65D028: the crouched pickup's,
 * StabBuddy's and RestrainBuddy's, each read by that function alone -- the names ours), the climbs'
 * anim list (SOCOM 1's newanims, a CActiveAnimList of eight where SOCOM 1's held twelve: OVL2 bss
 * 0x66B2F0, its destructor registered with the record at 0x66B2E0), then SOCOM 1's ten throw offsets
 * (0x65D038-0x65D0D3, the same values, read by the throw's anim choice 0x57FA70). SOCOM 1's CSTable
 * and its float copied through $gp are gone. */
CPnt3D pickup_offset(5.27f, -4.22f, -3.26f);
CPnt3D stab_offset(-2.11f, 0.64f, -8.9f);
CPnt3D restrain_offset(4.49f, 0.0f, -4.0f);
CActiveAnimList newanims(8);
CPnt3D stand_throw(3.343816f, 19.34775f, 3.913763f);
CPnt3D crouch_throw(2.450365f, 6.39872f, 6.219564f);
CPnt3D prone_throw(4.523735f, 1.312385f, -3.574992f);
CPnt3D stand_toss(4.673006f, 8.821998f, -8.002712f);
CPnt3D crouch_toss(2.630568f, 6.323799f, 6.1622f);
CPnt3D prone_toss(1.398064f, 5.113956f, -8.086079f);
CPnt3D stand_peek_rtoss(7.547573f, 9.781673f, -2.544316f);
CPnt3D stand_peek_ltoss(-0.24231f, 8.864734f, 5.106942f);
CPnt3D running_rifle_throw(3.070334f, 11.392076f, -2.970803f);
CPnt3D running_pistol_throw(3.056369f, 11.494563f, -3.000149f);

/* whether an anim type is anywhere on the blend stack, the top first (OVL2 0x58A470; new in
 * SOCOM II, its name unknown -- seal_env.cpp's 0x5A7AB0 asks it).
 * OPEN, 5 instructions: the accessor's index unsigned against the int counter stops MW
 * strength-reducing the walk (the target's `sll; addu` each pass -- 0x57DBB0 matches so); left, the
 * registers: the target narrows the type into a0 first and loads the array into v1, MW the other
 * way round. Tried: the compare reversed, a short parameter, the type cast or copied first, the
 * stack through a reference, the members straight off the SEAL, the body as a CBody. Round 41, by
 * MW's own colouring (tools/igvar.py): the array's hoisted load is temporary @8332, the type's
 * sign extension @8334, so the extension colours first (v1) and the array takes a0 -- the target
 * needs the extension numbered first. Still 5: the element a local, an int local for its type, a
 * (short) or (AnimType) cast, a BlendIs inline, a result flag with break (18); an InStack(stack,
 * type) inline taking an int, short or AnimType type: 6-7. */
static inline CZBodyAnimBlend *BlendAt(CBodyAnimBlendStack *stack, unsigned int i)
{
    return stack->m_stack[i];
}

extern "C" bool func_0058A470(CZSealBody *seal, AnimType type)
{
    CBodyAnimBlendStack *stack = &seal->m_blends;

    for (int i = stack->m_top; i >= 0; i--)
        if (type == BlendAt(stack, i)->m_animType)
            return true;
    return false;
}

extern "C" {
void *func_00555170(void *v);
}

/* the weapon in hand set and every blend's anim looked up again for it -- the weapon's own, else
 * weapon 0's -- the parts' poses saved, and both interpolations set to the top anim's blend-in time
 * (OVL2 0x57D710, new in SOCOM II: seal_cfg.cpp's 0x58C5D0 calls it; the name unknown).
 * OPEN, 14 instructions (the size the target's since round 33): the stack's address assigned again in
 * the loop is hoisted, and that gives the target's entry -- a test block of its own after the return
 * (`bltz i` to the exit, `b` to the bottom `bgez`: the engine core's round-11 preheader rule; before,
 * MW tested at the top, 0xE4). Left: MW puts the hoisted addiu in that block (the bltz's delay slot)
 * where the target computes it before the entry `b` (its delay slot), and the SEAL and the stack trade
 * s1/s2 (igdump: seal, stack and i all over K, coloured highest index first -- the target wants the
 * stack after the SEAL). Tried with the in-loop assignment: it at the body's end or after the
 * representative (0xE0/0xE4), another local for it, the accessor's `&seal->m_blends` alone (0xD8),
 * dead first definitions of rep, id or i ahead of the stack (14-21), the counter or the stack
 * declared first. Before round 33: the stack declared before the store, a while loop, `if (i >= 0)
 * do ... while (--i >= 0)`, the stack through a reference, a goto into the bottom test. */
extern "C" bool func_0057D710(CZSealBody *seal, short weapon)
{
    seal->m_weapon_id = weapon;
    CBodyAnimBlendStack *stack = &seal->m_blends;

    for (int i = stack->m_top; i >= 0; i--) {
        stack = &seal->m_blends;
        ACTIVE_ANIM *rep = BlendAt(stack, i)->Representative();
        short id = rep->m_id;
        CZSIScriptCommon *anim = seal->m_animset->GetScript((AnimType)id, seal->m_weapon_id);

        if (!anim)
            anim = seal->m_animset->GetScript((AnimType)id, 0);
        if (anim)
            rep->m_anim = anim;
    }
    seal->SaveParts();
    float t = ((CZBodyAnimBlend *)func_00555170((char *)seal + 0x1c0))->Representative()->m_anim->m_20;

    seal->m_interp2 = seal->m_interp1 = t;
    return true;
}

extern "C" {
/* an anim type for an item (OVL2 0x58C820, seal_cfg.cpp's) */
AnimType func_0058C820(CZSealBody *seal, AnimType type, SEAL_ITEM_BYTE item);
}

/* every blend's anim type taken over to an item -- its type for the item, its anim looked up again (the
 * weapon's own, else weapon 0's) -- and the top blend's base type kept, unless a rifle swap is on the
 * stack (OVL2 0x57D5E0, new in SOCOM II: AnimTypes::SwapRifle, SOCOM 1's static six after Stand2Prone;
 * the name unknown). The walk is 0x57D710's, the stack's address assigned in the loop as there.
 * OPEN, 9 instructions (the size the target's): 0x57D710's two misses exactly -- the hoisted addiu in
 * the preheader's bltz delay slot where the target has it in the entry `b`'s (the counter's lw before
 * the b), and the stack and the counter trading s0/s1 (igdump: seal, item, stack and i all over K and
 * coloured highest index first; the target wants the stack's index above the counter's -- a web made
 * late, as the split rule's). Tried: the counter defined or declared first (a dead `i = 0` too), the
 * prefix step, a while loop, the stack and counter both in the for's init, an inline accessor
 * returning the stack's address (22: its temporary s3), the member indexed straight (0x10C) */
extern "C" void func_0057D5E0(CZSealBody *seal, SEAL_ITEM_BYTE item)
{
    if (seal->IsCurrentBlend((AnimType)AnimTypes::SwapRifle))
        return;
    CBodyAnimBlendStack *stack = &seal->m_blends;

    for (int i = stack->m_top; i >= 0; i--) {
        stack = &seal->m_blends;
        ACTIVE_ANIM *rep = BlendAt(stack, i)->Representative();
        AnimType id = func_0058C820(seal, (AnimType)rep->m_id, item);

        rep->m_id = id;
        CZSIScriptCommon *anim = seal->m_animset->GetScript(id, seal->m_weapon_id);

        if (!anim)
            anim = seal->m_animset->GetScript(id, 0);
        if (anim)
            rep->m_anim = anim;
    }
    seal->m_curblend = (AnimType)((CZBodyAnimBlend *)func_00555170((char *)seal + 0x1c0))->Representative()->m_id;
    seal->m_curblend = seal->m_animset->BaseAnim(seal->m_curblend);
}

/* the state of the topmost blend in state 0, 1 or 2, else 0 (OVL2 0x57DBB0; new in SOCOM II, its
 * name unknown) */
extern "C" unsigned char func_0057DBB0(CZSealBody *seal)
{
    CBodyAnimBlendStack *stack = &seal->m_blends;

    for (int i = stack->m_top; i >= 0; i--) {
        unsigned char s = BlendAt(stack, i)->m_state;

        if (s == 0 || s == 1 || s == 2)
            return s;
    }
    return 0;
}

/* the state of the topmost blend not in state 3, else 0 (OVL2 0x57DC20; SOCOM 1's PeekBaseState --
 * the weapon unit's CalcAimGoal asks it in state 3 from the same place as SOCOM 1's; seal_env.cpp's
 * 0x5A82E0 too) */
unsigned char CZSealBody::PeekBaseState()
{
    CBodyAnimBlendStack *stack = &m_blends;

    for (int i = stack->m_top; i >= 0; i--) {
        unsigned char s = BlendAt(stack, i)->m_state;

        if (s != 3)
            return s;
    }
    return 0;
}

/* a blended anim pushed to everyone, for a SEAL the network plays (OVL1 0x2BBAF0: SOCOM 1's
 * NetSendPushBlend with a time, fts_remote.cpp's) and an anim pushed on the body (OVL2 0x588BC0,
 * not written yet: an anim type, the state, a flag and a kind) */
extern "C" {
void func_002BBAF0(unsigned int obj, unsigned int anim, int slot, bool on, int time);
void *func_00588BC0(CZSealBody *seal, AnimType type, int state, int flag, int kind);
bool func_00589900(CZSealBody *seal);
void *func_00555170(void *v);
unsigned short func_0058A920(unsigned short flags, unsigned short bits, bool on);
}

/* a halfword's bits tested (seal.cpp's tail copy, OVL2 0x5551A0) */
extern "C" bool func_005551A0(unsigned short flags, unsigned short bits);

/* the state the SEAL is in; asked with a flag, the top blend's anims say it first (17 stand or
 * crouch, 19 crouch or prone, 25 stand or prone, by the blend's flag 0x20) unless its flag 0x40 is
 * set, and a crouch with bit 2 of 0x21C counts as a stand (OVL2 0x58A720: SOCOM 1's GetStance, the
 * crouch bit new; seal_ai.cpp's InStance and the kit's kick ask it). Pinned out of line: short enough
 * for -inline auto, but CrouchToPickupBuddy, this unit's one caller, calls it as the target does */
#pragma auto_inline off
SEAL_STATE CZSealBody::GetStance(bool flag) const
{
    SEAL_STATE state = (SEAL_STATE)m_state;

    if (flag) {
        CZBodyAnimBlend *top = (CZBodyAnimBlend *)func_00555170((char *)this + 0x1c0);

        if (!func_005551A0(top->m_flags, 0x40)) {
            bool b = func_005551A0(top->m_flags, 0x20);

            if (IsCurrentBlend((AnimType)17))
                return b ? stateCrouch : stateStand;
            if (IsCurrentBlend((AnimType)19))
                return b ? stateProne : stateCrouch;
            if (IsCurrentBlend((AnimType)25))
                return b ? stateProne : stateStand;
        }
        if (state == stateCrouch && m_21c_bit2)
            state = stateStand;
    }
    return state;
}
#pragma auto_inline reset

/* how fast the SEAL moves, standing, crouched or prone: 1 still, 2 under 20 units a second, 3 faster
 * (1 in any other state; OVL2 0x58A820, SOCOM 1's GetSpeedState after GetStance, the speeds squared
 * -- Tick asks it) */
SPEED_STATE CZSealBody::GetSpeedState()
{
    float v = m_velM.x * m_velM.x + m_velM.y * m_velM.y + m_velM.z * m_velM.z;

    switch (m_state) {
    case 0:
    case 1:
    case 2:
        if (v < 0.25f)
            return SPEED_STILL;
        return v < 400.0f ? SPEED_SLOW : SPEED_FAST;
    }
    return SPEED_STILL;
}

/* an anim's push told to the network, for a SEAL this console plays: any anim, and anim 34 (OVL2
 * 0x588AB0, 0x588B40, new in SOCOM II; the unit's own functions inline them, no one calls the
 * copies) */
extern "C" void func_00588AB0(CZSealBody *seal, int time, unsigned int anim)
{
    if (theNetwork.m_9 && !seal->IsRemote())
        func_002BBAF0(seal->m_id, anim, 4, false, time);
}

extern "C" void func_00588B40(CZSealBody *seal, int time)
{
    if (theNetwork.m_9 && !seal->IsRemote())
        func_002BBAF0(seal->m_id, 34, 4, false, time);
}

/* whether the SEAL may look about: always without a blend; never dead (8, the top blend's state too),
 * nor in states 9 and 10; else as the last overlay of the top blend's first anim says (OVL2
 * 0x587B40; SOCOM 1's CanHeadLook, the same tests -- SetLookDir calls it in both builds) */
bool CZSealBody::CanHeadLook()
{
    CZBodyAnimBlend *top = (CZBodyAnimBlend *)func_00555170((char *)this + 0x1c0);

    if (!top)
        return true;
    if (m_state == 8 || top->m_state == 8)
        return false;
    if (m_state == 9 || m_state == 10)
        return false;
    bool r = false;

    for (ACTIVE_ANIM *a = top->m_anims.Begin(); a; a = a->m_overlay) {
        if (a->m_anim->m_4a_5)
            r = true;
        else
            r = false;
    }
    return r;
}

/* whether the SEAL may pitch its aim: never dead (8), in states 9 and 10, peeking (5) or in state 4
 * -- the SEAL's state or the top blend's -- else as the last overlay of the top blend's first anim says
 * (OVL2 0x587A30; SOCOM 1's CanPitch, the same tests, before CanHeadLook in both builds) */
bool CZSealBody::CanPitch()
{
    CZBodyAnimBlend *top = (CZBodyAnimBlend *)func_00555170((char *)this + 0x1c0);

    if (m_state == 8 || top->m_state == 8)
        return false;
    if (m_state == 9 || m_state == 10)
        return false;
    if (m_state == 5 || top->m_state == 5)
        return false;
    if (m_state == 4 || top->m_state == 4)
        return false;
    bool r = false;

    for (ACTIVE_ANIM *a = top->m_anims.Begin(); a; a = a->m_overlay) {
        if (a->m_anim->m_4a_3)
            r = true;
        else
            r = false;
    }
    return r;
}

extern "C" {
/* SOCOM II's overlaid anims (OVL1 data: the weapon change's, 0x3DEC68 -- SOCOM 1's MovingRifle2Pistol
 * by position, unproven -- and four new ones the grenade and launcher code pushes) */
extern short data_003DEC68;
extern short data_003DF0B0;
extern short data_003DF0D0;
extern short data_003DF0F0;
extern short data_003DF0F8;
}

/* an anim laid over the SEAL's blends: its trigger time, its own time and its event (an inline, the
 * name ours: the overlays SOCOM II adds share its stores) */
static inline void SetOverlay(CBlendOverlay &o, ACTIVE_ANIM *a, float trig, float t, short event)
{
    o.m_anim = a;
    o.m_triggerT = trig;
    o.m_ovlyT = t;
    o.m_state = 0;
    o.m_event = event;
}

/* an overlay anim pushed over the blends: its script looked up and an active anim taken for it, then,
 * for the overlays the SEAL knows, the overlay set -- the moving reloads, the weapon change by the item
 * in hand, and SOCOM II's four (events 3, 4, 8 and 10, the last two timed by their loop); the top blend,
 * or none when the anim is missing (OVL2 0x587780; SOCOM 1's AddBlendOverlay, its overlay a member now
 * -- 0x588BC0 calls it). The active anim declared first, uninitialised: it takes s1 and the top blend
 * s0; the type passed through an int, which the target sign-extends at the call (the bare AnimType
 * passes as it is) */
CZBodyAnimBlend *CZSealBody::AddBlendOverlay(AnimType type)
{
    ACTIVE_ANIM *a;
    CZBodyAnimBlend *top = (CZBodyAnimBlend *)func_00555170((char *)this + 0x1c0);
    CZSIScriptCommon *script = m_animset->GetScript(type);

    if (!script)
        return 0;
    a = m_active_anim_pool.GetActiveAnim(script, (int)type);

    if (!a)
        return 0;
    if (type == AnimTypes::MovingRifleReload)
        SetMovingRifleReload(a);
    else if (type == AnimTypes::MovingPistolReload)
        SetMovingPistolReload(a);
    else if (type == data_003DEC68) {
        if (m_item == 1)
            SetMovingChangeToPistol(a);
        else if (m_item == 2)
            SetMovingChangeToRifle(a);
    } else if (type == data_003DF0B0)
        SetOverlay(m_overlay, a, -4.0f, 0.0f, 3);
    else if (type == data_003DF0D0)
        SetOverlay(m_overlay, a, -4.0f, 0.0f, 4);
    else if (type == data_003DF0F0)
        SetOverlay(m_overlay, a, -4.0f, a->m_anim->LoopTime(), 8);
    else if (type == data_003DF0F8)
        SetOverlay(m_overlay, a, -4.0f, a->m_anim->LoopTime(), 10);
    return top;
}

/* the overlays of a moving reload, rifle and pistol (events 1 and 2) and of a weapon change on the move,
 * to the pistol and to the rifle (events 6 and 5: triggered at once in the air, the rifle's timed by
 * its loop) (OVL2 0x587760, 0x587740, 0x587670, 0x5876B0; SOCOM 1's, in its order -- CZSealBody::Tick
 * calls them, AddBlendOverlay inlines them) */
void CZSealBody::SetMovingRifleReload(ACTIVE_ANIM *a)
{
    SetOverlay(m_overlay, a, -4.0f, 0.0f, 1);
}

void CZSealBody::SetMovingPistolReload(ACTIVE_ANIM *a)
{
    SetOverlay(m_overlay, a, -4.0f, 0.0f, 2);
}

void CZSealBody::SetMovingChangeToRifle(ACTIVE_ANIM *a)
{
    float trig = m_airborne ? -4.0f : 0.0f;

    SetOverlay(m_overlay, a, trig, a->m_anim->LoopTime(), 5);
}

void CZSealBody::SetMovingChangeToPistol(ACTIVE_ANIM *a)
{
    float trig = m_airborne ? -4.0f : 0.0f;

    SetOverlay(m_overlay, a, trig, 0.0f, 6);
}

/* whether the SEAL's anim may be cut short: not when the top blend is done, its representative anim
 * holds, the SEAL carries a body in anim 17, or -- for the AI -- in anims 100, 99, 25, 19, 17 and the
 * pistol's stand to prone; else as the last overlay of the top blend's first anim says, the blend's time
 * against the anim's cut-in point (from the end when the blend's flag 0x20 is set) (OVL2 0x587C20;
 * SOCOM 1's CanInterrupt, between CanHeadLook and CanTurn in both builds -- SOCOM 1's AI anims 103 and
 * 102).
 * OPEN, 0x1CC against 0x1DC: the switch gives the target's descending compare chain and its closing `b`,
 * the result-variable IsAi its `andi` at the join; left: the carry and pistol tests' `move v0,zero` sits
 * in the target's conditional branch's delay slot (ours in the `b`'s), the loop's two arms assign a bool
 * (`daddiu`, the `andi` to r at the join) and the target numbers back in s0 and r in s2 (ours the other
 * way). Tried: the cut-in test through an inline bool helper (if/return or a ternary) and as nested
 * if/else assignments */
bool CZSealBody::CanInterrupt()
{
    CZBodyAnimBlend *top = (CZBodyAnimBlend *)func_00555170((char *)this + 0x1c0);

    if (top->IsDone(m_2e0))
        return false;
    short id = top->Representative()->m_id;

    if (!top->Representative()->m_bit1)
        return false;
    if (id == 17 && m_carry)
        return false;
    if (IsAi()) {
        switch (id) {
        case 17:
        case 19:
        case 25:
        case 99:
        case 100:
            return false;
        }
        if (id == AnimTypes::PistolStand2Prone)
            return false;
    }
    bool back = func_005551A0(top->m_flags, 32);
    float t = top->m_tfrac;
    bool r = false;

    for (ACTIVE_ANIM *a = top->m_anims.Begin(); a; a = a->m_overlay) {
        float x = a->m_anim->m_30;

        r = back ? t < 1.0f - x : t > x;
    }
    return r;
}

/* whether the last overlay of the top blend's first anim turns (OVL2 0x587E00; SOCOM 1's CanTurn,
 * after CanInterrupt in both builds -- PreTick asks it; each pass overwrites the answer) */
bool CZSealBody::CanTurn()
{
    CZBodyAnimBlend *top = (CZBodyAnimBlend *)func_00555170((char *)this + 0x1c0);
    bool r = false;

    for (ACTIVE_ANIM *a = top->m_anims.Begin(); a; a = a->m_overlay) {
        if (a->m_anim->m_4a_7)
            r = true;
        else
            r = false;
    }
    return r;
}

extern "C" {
/* the top blend popped (OVL2 0x589AA0, SOCOM 1's PopBlend's place; defined below) */
CZBodyAnimBlend *func_00589AA0(CZSealBody *seal);
}

/* a blend's anims (anim.cpp's CBlendable, the kit lane's: four 12-byte slots -- only the test here),
 * and the animation set's strafe blendables (SOCOM 1's DWARF holds them as pointers, m_strafeL_anims
 * 0x6c and m_strafeR_anims 0x70 after the move ones; SOCOM II's set holds the 0x30-byte blendables
 * themselves from 0x60 -- a view, the names SOCOM 1's) */
class CBlendable {
public:
    bool Contains(AnimType type);

    unsigned char _0[0x30];
};

struct AnimSetStrafes {
    unsigned char _0[0xc0];
    CBlendable m_strafeL_anims;   // 0xc0
    CBlendable m_strafeR_anims;   // 0xf0
};

/* standing still: unless the stand is on top already, the top blend's weight in strafe anims summed
 * (the right's, or the left's negated when the strafe is to the left), the blends popped down to one
 * and the step's foot kept -- 2 past 0.6, 32 below -0.6, 2 turning, else by the foot -- and the stand
 * (1) pushed in place of the top unless it is current; the turn's timing cleared, the turning and
 * moving flags cleared either side of the top made whole, which is returned (OVL2 0x586240; SOCOM 1's
 * StandStill, after StandTurning and before InStateStand in both builds) */
CZBodyAnimBlend *CZSealBody::StandStill(float x, float z)
{
    CZBodyAnimBlend *top = (CZBodyAnimBlend *)func_00555170(&m_blends);
    int foot = top->ThisFoot();

    if (!IsCurrentBlend((AnimType)1)) {
        float sum = 0.0f;

        if (z >= 0.0f) {
            CBlendable *strafe = &((AnimSetStrafes *)m_animset)->m_strafeR_anims;

            for (ACTIVE_ANIM *a = top->m_anims.Begin(); a; a = top->m_anims.Next())
                if (strafe->Contains((AnimType)a->m_id))
                    sum += a->m_wt;
            /* (the sign kept, as the left's is turned: an assignment to the sum here keeps the loop's
             * exit a label of its own, `b join` behind it, where the target's first test lands --
             * without it MW chains that test straight to the join; `(void)0;` and `sum *= 1.0f` do
             * not serve) */
            sum = +sum;
        } else {
            CBlendable *strafe = &((AnimSetStrafes *)m_animset)->m_strafeL_anims;

            for (ACTIVE_ANIM *a = top->m_anims.Begin(); a; a = top->m_anims.Next())
                if (strafe->Contains((AnimType)a->m_id))
                    sum += a->m_wt;
            sum = -sum;
        }
        ClearStack(1, true);
        if (sum > 0.6f)
            m_last_foot = 2;
        else if (sum < -0.6f)
            m_last_foot = 32;
        else if (m_is_turning)
            m_last_foot = 2;
        else {
            short last;

            if (foot >= 17 && foot < 48)
                last = 32;
            else
                last = 2;
            m_last_foot = last;
        }
        if (!IsCurrentBlend((AnimType)1)) {
            func_00589AA0(this);
            func_00588BC0(this, (AnimType)1, 0, 0, 0);
        }
    }
    m_turnip.Clear();
    SetIsTurning(false);
    SetIsMoving(false);
    CZBodyAnimBlend *b = (CZBodyAnimBlend *)func_00555170(&m_blends);

    b->SetRepresentativeWt(1.0f);
    SetIsMoving(false);
    SetIsTurning(false);
    return b;
}

/* standing, turned in place: unless the turn is on top already, the blends popped down to one, the
 * turn (16) pushed and -- limping -- the step's foot kept (32 for a foot in 17-47, its time from the
 * half, else 2), the turn set up and its timing started full; the turning flag set, the moving one
 * cleared, the turn controlled (OVL2 0x586050; SOCOM 1's StandTurning, after UpdateQuadrant and before
 * StandStill in both builds -- SOCOM II starts the turn only limping) */
CZBodyAnimBlend *CZSealBody::StandTurning(float turn)
{
    int foot = ((CZBodyAnimBlend *)func_00555170(&m_blends))->ThisFoot();

    if (!IsCurrentBlend((AnimType)16)) {
        ClearStack(1, true);
        CZBodyAnimBlend *b = (CZBodyAnimBlend *)func_00588BC0(this, (AnimType)16, 0, 0, 0);

        if (!m_limp)
            return b;
        /* (a short local set in an if/else: one store after the join -- the ternary puts the 2 in
         * the first test's delay slot, stores in each arm give two) */
        short last;

        if (foot >= 17 && foot < 48)
            last = 32;
        else
            last = 2;
        m_last_foot = last;
        if (m_last_foot == 32)
            b->m_tfrac = 0.5f;
        TurnSetup(foot);
        m_turnip.Start(0.8f, 3000.0f, 0.4f);
        m_turnip.m_currT = m_turnip.m_holdT + m_turnip.m_downT;
    }
    SetIsTurning(true);
    SetIsMoving(false);
    return TurnControl(turn);
}

extern "C" {
/* the throttles' easing rates, from and to, for the side and the move throttle (OVL1 bss: SOCOM 1's
 * four small-data floats in the same order) */
extern float data_0044C294;
extern float data_0044C298;
extern float data_0044C29C;
extern float data_0044C2A0;
}

/* a throttle's nearness to full: one minus the eighth power of what it lacks (inline: the factor of
 * the easing rate's lerp, its last factor -- the right operand of the product) */
static inline float Ease8(float t)
{
    float p = 1.0f - fabsf(t);

    p *= p;
    p *= p;
    p *= p;
    return 1.0f - p;
}

/* the throttles eased: an AI SEAL's left as they are; else each one's step from its last value held
 * to a rate that runs from its first toward its second limit as the throttle nears full (one minus
 * the eighth power of what it lacks), the change flagged; then a throttle that was past 0.78 (the
 * side) or 0.9 (the move) and jumps by more than ten times that per step taken whole, the body's
 * parts saved and their easing set to 0.2; the eased pair handed back and whether either step was held
 * (OVL2 0x586C10; SOCOM 1's SmoothXZThrottles, after StandStill's stretch and before IsStopped in both
 * builds -- SOCOM II adds the AI's exit and the snap) */
bool CZSealBody::SmoothXZThrottles(float &x, float &z)
{
    bool changed = false;

    if (!(m_control ? ((CSealCtrl *)m_control)->IsAi() : false)) {
        /* (the order of first definitions is MW's numbering -- tools/perm.py's arrangement, the dead
         * zeroes cost nothing: it gives the move pair's rates f26/f20 as the target has them, which no
         * plain declaration order tried does -- a shape for the registers, not the original's) */
        float c, nx, nz, px, d, pz, a, b;

        px = 0.0f;
        nz = 0.0f;
        nx = 0.0f;
        pz = 0.0f;
        b = 0.0f;
        a = data_0044C294;
        nz = z;
        d = 0.0f;
        b = data_0044C298;
        c = 0.0f;
        c = data_0044C29C;
        nx = x;
        d = data_0044C2A0;
        px = m_prevx;
        pz = m_prevz;
        a = a + (b - a) * Ease8(nx);
        d = c + (d - c) * Ease8(nz);
        if (fabsf((nx - px) / m_2e0) > a) {
            if (nx > px)
                nx = px + a * m_2e0;
            else
                nx = px - a * m_2e0;
            changed = true;
        }
        if (fabsf((nz - pz) / m_2e0) > d) {
            if (nz > pz)
                nz = pz + d * m_2e0;
            else
                nz = pz - d * m_2e0;
            changed = true;
        }
        bool snap = false;
        float dx = fabsf(x - px) / m_2e0;

        if (fabsf(px) > 0.78f && dx > 0.78f * 10.0f) {
            nx = x;
            snap = true;
        }
        float dz = fabsf(z - pz) / m_2e0;

        if (fabsf(pz) > 0.9f && dz > 0.9f * 10.0f) {
            nz = z;
            snap = true;
        }
        if (snap) {
            SaveParts();
            m_interp1 = 0.2f;
            m_interp2 = 0.2f;
        }
        x = nx;
        z = nz;
    }
    return changed;
}

extern "C" {
/* how far the throttles may stray for a hang to hold, and for the SEAL to count as stopped (OVL1
 * data; seal_env.cpp's ladder test reads it) */
extern float data_003F3428;
}

/* whether the SEAL counts as stopped: the side throttle within the slack of none, then -- turning --
 * the move throttle inside the band (below the slack plus the turn's square share of 0.3, above minus
 * one and a half slacks plus its share of 0.5), moving the move throttle within the slack, else the two throttles'
 * length within the slack or the move throttle inside the band (OVL2 0x586F00; SOCOM 1's IsStopped,
 * after SmoothXZThrottles and before SealAnim in both builds, the same body: InStateProne, InStateCrouch
 * and the two move controls ask it) */
bool CZSealBody::IsStopped(float x, float z, float t)
{
    static float zero_z = 0.0f;

    if (!_close_enough(z, zero_z, data_003F3428))
        return false;
    if (m_is_turning) {
        if (x < data_003F3428 + t * (t * (0.3f - data_003F3428)) &&
            x > -(1.5f * data_003F3428 + t * (t * (0.5f - 1.5f * data_003F3428))))
            return true;
        return false;
    }
    if (m_is_moving) {
        static float zero_x = 0.0f;

        return _close_enough(x, zero_x, data_003F3428);
    }
    float l = sqrtf(x * x + t * t);
    static float zero_l = 0.0f;

    if (_close_enough(l, zero_l, data_003F3428))
        return true;
    if (x < data_003F3428 + t * (t * (0.3f - data_003F3428)) &&
        x > -(1.5f * data_003F3428 + t * (t * (0.5f - 1.5f * data_003F3428))))
        return true;
    return false;
}

/* the jump begun, as the SEAL's own: bit 9 set and 10 cleared, the velocity kept, the push told to
 * the network for a SEAL this console plays (OVL2 0x57E050, new in SOCOM II; 0x5AF930 calls it after
 * 0x57DF20 and 0x57E130 -- the names ours) */
extern "C" void *func_0057E050(CZSealBody *seal)
{
    void *r = func_00588BC0(seal, (AnimType)9, seal->m_state, 0, 13);

    seal->SetBit9(true);
    seal->SetBit10(false);
    seal->m_1350 = seal->m_velW;
    func_00588B40(seal, 3);
    return r;
}

/* whether the jump can be taken: never by a remote SEAL already in anim 9, else as 0x589900 says
 * (OVL2 0x57E130, new in SOCOM II) */
extern "C" bool func_0057E130(CZSealBody *seal)
{
    if (seal->IsRemote() && seal->IsCurrentBlend((AnimType)9))
        return false;
    if (!func_00589900(seal))
        return false;
    return true;
}

/* the jump's start: bit 9 set and 10 cleared, the velocity kept, the push told to the network for a
 * SEAL this console plays, the top blend after (OVL2 0x57DF20, new in SOCOM II) */
extern "C" void *func_0057DF20(CZSealBody *seal)
{
    seal->SetBit9(true);
    seal->SetBit10(false);
    seal->m_1350 = seal->m_velW;
    func_00588B40(seal, 9);
    return func_00555170((char *)seal + 0x1c0);
}

namespace zdb {
/* the camera's collision bit demanded of the polygons a test hits (SOCOM 1's; grid_di.cpp's) */
void SetCameraCollideFlagRequired(bool on);
}

/* a polygon's camera type (DI_PARAMS's two bits at 18, as a byte enum: the target's `andi 0xff`
 * after the extraction; the names ours) */
enum CAMERA_TYPE { CAMERA_TYPE_0, CAMERA_TYPE_1, CAMERA_TYPE_2, CAMERA_TYPE_3 };

static inline CAMERA_TYPE CameraType(zdb::IntersectStruct *hit)
{
    return (CAMERA_TYPE)hit->m_poly->m_flags.m_cameratype;
}

/* whether there is room over the SEAL's head: a ray 20 units up from a point 1 above it, on the
 * SEAL's own axis, with the SEAL's node switched off and camera-collision polygons asked for -- clear
 * unless it hits a polygon of a camera type other than 1 (OVL2 0x57F150, new in SOCOM II: 0x57E1B0
 * asks it of the SEAL's controller) */
extern "C" bool func_0057F150(CSealCtrl *ctrl)
{
    /* (hit and the old setting declared ahead of the SEAL: declared first, the SEAL is numbered
     * last and takes s0 -- through an inline accessor it matches too) */
    zdb::IntersectStruct *hit;
    bool was;
    CZSealBody *seal = (CZSealBody *)ctrl->m_entity;
    CPnt3D pos;

    seal->PartInSpace(seal->m_head, 0, &pos, 0);
    pos.y += 1.0f;
    CPnt3D from(0.0f, pos.y, 0.0f);

    seal->m_node->m_matrix.Transform(&from, 1);
    CPnt3D to(from.x, 20.0f + from.y, from.z);

    was = seal->GetNode()->SetActive(false);
    zdb::SetCameraCollideFlagRequired(true);
    hit = zdb::CWorld::m_world->GetIntersect(&from, &to);
    seal->GetNode()->SetActive(was);
    if (hit && CameraType(hit) != CAMERA_TYPE_1) {
        zdb::SetCameraCollideFlagRequired(false);
        return false;
    }
    zdb::SetCameraCollideFlagRequired(false);
    return true;
}

/* whether there is room to stand: a ray from 2 above the hips, on the SEAL's axis, 12 up crouched or 19
 * otherwise, with the SEAL's node switched off and camera-collision polygons asked for -- clear unless
 * it hits a polygon of a camera type other than 1 (OVL2 0x57EFE0; SOCOM 1's OKToStand, after DropBody in
 * both builds -- SOCOM 1's from the head 7 up, its offset added to GetPosW's). SOCOM II's tests a byte
 * in a1 that none of its three callers sets: a stance never assigned */
bool OKToStand(CZSealBody *seal, SEAL_STATE stance)
{
    zdb::IntersectStruct *hit;
    bool was;
    float h;
    CPnt3D pos;

    if (stance == stateCrouch)
        h = 12.0f;
    else
        h = 19.0f;
    seal->PartInSpace(seal->m_hips, 0, &pos, 0);
    pos.y += 2.0f;
    CPnt3D from(0.0f, pos.y, 0.0f);

    from.Add(&seal->m_posW, &from);
    CPnt3D to(from.x, h + seal->m_posW.y, from.z);

    was = seal->GetNode()->SetActive(false);
    zdb::SetCameraCollideFlagRequired(true);
    hit = zdb::CWorld::m_world->GetIntersect(&from, &to);
    seal->GetNode()->SetActive(was);
    if (hit && CameraType(hit) != CAMERA_TYPE_1) {
        zdb::SetCameraCollideFlagRequired(false);
        return false;
    }
    zdb::SetCameraCollideFlagRequired(false);
    return true;
}

/* whether the SEAL may act: an AI's controller unless its bit 17 is set, anyone else's unless its
 * action bit is (OVL2 0x57F2A0, new in SOCOM II; the name ours -- the controller read once, its null
 * test only the IsAi call's) */
extern "C" bool func_0057F2A0(CZSealBody *seal)
{
    CSealCtrl *ctrl = (CSealCtrl *)seal->m_control;

    /* (one exit: early returns pad each block with a nop) */
    return (ctrl ? ctrl->IsAi() : false) ? !ctrl->m_bit17 : !ctrl->m_action;
}

extern "C" {
/* two anim types the code reads as variables (OVL1 data, -1 in the image) and the top blend popped,
 * the new top returned (OVL2 0x589AA0, not written yet) */
extern short data_003DEAE0;
extern short data_003DEF60;
CZBodyAnimBlend *func_00589AA0(CZSealBody *seal);
}

/* the blends' count (an inline: unsigned, as the target's `sltiu` compares it) */
static inline unsigned int BlendCount(CBodyAnimBlendStack *stack)
{
    return stack->m_top + 1;
}

/* the blends popped down to one, twelve at most; when that worked the top blend's control cleared
 * and the SEAL asked to go prone; then the anim of 0x3DEF60 pushed (OVL2 0x57F310, new in SOCOM II) */
extern "C" void func_0057F310(CZSealBody *seal)
{
    CZBodyAnimBlend *b = (CZBodyAnimBlend *)func_00555170(&seal->m_blends);
    int n = 12;
    CBodyAnimBlendStack *stack = &seal->m_blends;

    while (BlendCount(stack) > 1 && n > 0) {
        b = func_00589AA0(seal);
        n--;
    }
    if (n <= 0)
        b = (CZBodyAnimBlend *)func_00555170((char *)seal + 0x1c0);
    else {
        if (b)
            b->m_ctrl = 0;
        seal->m_desiredStance = STANCE_PRONE;
    }
    func_00588BC0(seal, (AnimType)data_003DEF60, 0, 0, 0);
}

extern "C" {
/* an anim's push told to the network as anim 34 (OVL2 0x588B40, below: this unit inlines it) */
void func_00588B40(CZSealBody *seal, int time);
/* an AnimTypes static (OVL1 data 0x3DEF08, 0x2C0 past Stand2Prone) */
extern short data_003DEF08;
}

/* into the air: nothing but the top blend while the 0x3DEF08 anim plays; else the blends popped down to
 * one, twelve at most, as 0x57F310 does, the crouch bit cleared, anim 26 and the 0x3DEF08 anim pushed in
 * state 2, the body in the air (6), the velocity kept level as m_1350, the move's stopped, m_1360 at 0.2,
 * the airborne bit and bit 9 set, bit 10 cleared, m_fac at the root's height, the desired state prone;
 * the push told to the network (OVL2 0x57E540, new in SOCOM II: UpdateStance, the grenade state and
 * 0x5880E0 call it -- the name unknown) */
extern "C" CZBodyAnimBlend *func_0057E540(CZSealBody *seal)
{
    if (seal->IsCurrentBlend((AnimType)data_003DEF08))
        return (CZBodyAnimBlend *)func_00555170(&seal->m_blends);

    CZBodyAnimBlend *b = (CZBodyAnimBlend *)func_00555170(&seal->m_blends);
    int n = 12;
    CBodyAnimBlendStack *stack = &seal->m_blends;

    while (BlendCount(stack) > 1 && n > 0) {
        b = func_00589AA0(seal);
        n--;
    }
    if (n <= 0)
        b = (CZBodyAnimBlend *)func_00555170((char *)seal + 0x1c0);
    else {
        if (b)
            b->m_ctrl = 0;
        seal->m_desiredStance = STANCE_PRONE;
    }
    seal->Set21cBit2(false);
    func_00588BC0(seal, (AnimType)26, 2, 0, 13);
    CZBodyAnimBlend *r = (CZBodyAnimBlend *)func_00588BC0(seal, (AnimType)data_003DEF08, 2, 0, 13);

    seal->BodySetState(6);
    seal->m_1350 = seal->m_velW;
    seal->m_1350.y = 0.0f;
    seal->m_velM.x = 0.0f;
    seal->m_velM.y = 0.0f;
    seal->m_velM.z = 0.0f;
    seal->m_1360 = 0.2f;
    seal->m_1364 = 0.0f;
    seal->SetAirborne(true);
    seal->SetBit9(true);
    seal->SetBit10(false);
    seal->m_fac = seal->m_root->m_pos.y;
    seal->m_desiredState = stateProne;
    func_00588B40(seal, 4);
    return r;
}

extern "C" {
/* a move begun one way or the other (OVL2 0x5805B0, below) and two anim types read as variables
 * (OVL1 data, -1 in the image) -- the throw anim's pick is CZSealBody::GetThrowAnim (0x57FCE0) */
CZBodyAnimBlend *func_005805B0(CZSealBody *seal, bool other, int state);
extern short data_003DEED0;
extern short data_003DEED8;
}

/* the body's state (an inline: expanded ahead of the call it is passed to, so it loads first) */
static inline int StateOf(CZSealBody *seal)
{
    return seal->m_state;
}

/* the throw anim GetThrowAnim picks pushed, and the time into it its fraction makes, past its base
 * (OVL2 0x5802B0: SOCOM 1's BeginThrowItem, after GetThrowAnim in both builds; the float passed
 * straight on, as SOCOM 1's) */
bool CZSealBody::BeginThrowItem(float t, float &out)
{
    AnimType type;
    float frac = 0.0f;
    float base = 0.0f;
    CPnt3D p;
    bool ok = false;

    if (GetThrowAnim(t, type, frac, base, p)) {
        CZBodyAnimBlend *b = (CZBodyAnimBlend *)func_00588BC0(this, type, StateOf(this), 0, 0);

        out = base + frac * b->Representative()->m_anim->LoopTime();
        ok = true;
    }
    return ok;
}

/* the throw the SEAL would make: a toss below both of the dynamics' toss thresholds (the throw's
 * power, the aim's rise), the anim, the fraction into it the item leaves and the point it leaves from
 * by the body's state -- a crouched SEAL on the move throws standing, a peeking one tosses to the side
 * its blend leans, any other state none (OVL2 0x57FCE0: SOCOM 1's GetThrowAnim, between
 * GetPutativeFirePointW and BeginThrowItem in both builds, its guarded points function-local statics in
 * both; SOCOM 1's fractions STAND_TOSS_FRAC ... RIGHT_LEAN_FRAC were variables, SOCOM II's are
 * constants, and SOCOM II asks IsCurrentBlend out of line where SOCOM 1's inlined it) */
bool CZSealBody::GetThrowAnim(float power, AnimType &type, float &frac, float &base, CPnt3D &p)
{
    float aim = m_aim_goal.y;
    CharacterDynamics::throw_params &tp = GetDynamics()->m_throw_params;
    bool toss = power < tp.toss_power_threshold && aim < tp.toss_aim_threshold;

    frac = 0.0f;
    base = 0.0f;
    bool ok = true;
    static CPnt3D stand_throw(3.343816f, 19.34775f, 3.913763f);
    static CPnt3D stand_toss(3.13f, 13.8f, -10.5f);
    static CPnt3D crouch_throw(6.05f, 15.59f, 1.96f);
    static CPnt3D prone_throw(6.29f, 15.05f, -0.97f);
    static CPnt3D prone_toss(2.39f, 7.87f, -8.53f);
    static CPnt3D right_lean(7.8f, 10.13f, -5.31f);
    static CPnt3D left_lean(-8.83f, 11.47f, -6.48f);
    /* (SOCOM 1's a guarded static as well; MW 3.0 lays a constant one out as plain data, OVL2 0x650578) */
    static float moving_sq = 225.0f;
    bool moving = Dot(m_velM, m_velM) > moving_sq;

    switch (m_state) {
    case stateStand:
        type = (AnimType)(toss ? AnimTypes::TossGrenade : AnimTypes::ThrowGrenade);
        frac = toss ? 0.69f : 0.46f;
        p = toss ? stand_toss : stand_throw;
        break;
    case stateCrouch:
        if (moving) {
            type = (AnimType)AnimTypes::ThrowGrenade;
            frac = 0.46f;
            p = stand_throw;
        } else {
            type = (AnimType)AnimTypes::CrouchThrowGrenade;
            frac = 0.7f;
            p = crouch_throw;
        }
        break;
    case stateProne:
        type = (AnimType)(toss ? AnimTypes::ProneTossGrenade : AnimTypes::ProneThrowGrenade);
        frac = toss ? 0.56f : 0.66f;
        p = toss ? prone_toss : prone_throw;
        break;
    case statePeek:
        if (IsCurrentBlend((AnimType)30) || IsCurrentBlend((AnimType)32)) {
            type = (AnimType)AnimTypes::TossGrenadePeekRight;
            frac = 0.55f;
            p = right_lean;
        } else if (IsCurrentBlend((AnimType)29) || IsCurrentBlend((AnimType)31)) {
            type = (AnimType)AnimTypes::TossGrenadePeekLeft;
            frac = 0.87f;
            p = left_lean;
        } else
            ok = false;
        break;
    default:
        ok = false;
    }
    return ok;
}

/* a SEAL this console plays, on the ground and in the anim of 0x3DEED0 or of 0x3DEED8, set on its
 * way by 0x5805B0, the second anim the other way (OVL2 0x5804E0, new in SOCOM II) */
extern "C" bool func_005804E0(CZSealBody *seal)
{
    bool other;

    if (seal->IsRemote())
        return false;
    if (seal->m_airborne)
        return false;
    if (seal->IsCurrentBlend((AnimType)data_003DEED0))
        other = false;
    else if (seal->IsCurrentBlend((AnimType)data_003DEED8))
        other = true;
    else
        return false;
    func_005805B0(seal, other, -1);
    return true;
}

extern "C" {
/* two more anim types read as variables (OVL1 data, -1 in the image) */
extern short data_003DEE08;
extern short data_003DEE48;
}

/* the body carried let go, when the SEAL is in state 9 or one of two carry anims: its blends popped
 * down to one, the drop anim of 0x3DEED8 or 0x3DEED0 pushed in state 8 at half its time, its kept
 * velocity its own plus 7 of its node's first row, bit 9 set and 10 cleared; the hold cleared (OVL2
 * 0x57EE00: SOCOM 1's DropBody, before OKToStand in both builds) */
void CZSealBody::DropBody()
{
    if (m_state == 9 || IsCurrentBlend((AnimType)data_003DEE08) || IsCurrentBlend((AnimType)data_003DEE48)) {
        CZSealBody *c = m_carry;

        if (c) {
            AnimType type;

            if (IsCurrentBlend((AnimType)data_003DEE08))
                type = (AnimType)data_003DEED8;
            else
                type = (AnimType)data_003DEED0;
            c->DetachBody();
            c->ClearStack(1, true);
            CZBodyAnimBlend *b = (CZBodyAnimBlend *)func_00588BC0(c, type, 8, 0, 0);

            b->m_tfrac = 0.5f;
            c->m_1350 = c->m_velW;
            CPnt3D v(*(const CPnt3D *)&c->m_node->m_matrix);

            v.Scale(7.0f, &v);
            c->m_1350.Add(&v, &c->m_1350);
            c->SetBit9(true);
            c->SetBit10(false);
        }
        m_carry = 0;
    }
}

extern "C" {
/* three more anim types read as variables (OVL1 data, -1 in the image) */
extern short data_003DEEF0;
extern short data_003DEEE8;
extern short data_003DEAD8;
}

extern "C" {
/* the overlay's last anim let go (OVL2 0x5A5DA0, seal_env.cpp's -- not written yet), the SEAL off its
 * ladder and its hold on one let go (OVL2 0x59D750, 0x5B14E0: seal_env.cpp's), the two item swaps
 * back to the gear (OVL2 0x5A7690 the pistol's, 0x5A7480 the rifle's: seal_env.cpp's) and an anim set
 * a SEAL may be bound to (OVL1 bss 0x44D410: one 0x5807D0 unbinds; the name unknown) */
void func_005A5DA0(CZSealBody *seal);
void func_0059D750(CZSealBody *seal);
void func_005B14E0(CZSealBody::CSealClimb *climb, CZSealBody *seal);
void func_005A7690(CZSealBody *seal);
void func_005A7480(CZSealBody *seal);
extern AnimSet *data_0044D410;
/* the top blend's held object let go (OVL2 0x5781C0, below) and a SEAL's 0x545EE0 (seal.cpp's) */
void func_005781C0(CZSealBody *seal);
void func_00545EE0(CZSealBody *seal, int n);
}

/* a fall begun: the body carried let go, the overlay's anims, the ladder and its hold let go; bound
 * to the set of 0x44D410, the SEAL unbound -- a remote one's 0x545EE0 told first -- with the item in
 * hand put back and the default set bound again; the blends cleared to the base and the anim of
 * 0x3DEED8 pushed (the other way, of 0x3DEED0) in a state -- a remote SEAL's as given unless it is
 * dead, this console's 0 alive and 8 dead -- with flag 2, and told to the network as anim 34 at 11
 * (10); bit 9 set and 10 cleared, m_1364 -1; the blend back (OVL2 0x5807D0, new in SOCOM II: 0x57E770
 * and 0x5880E0 call it, as 0x5805B0; the name unknown) */
extern "C" CZBodyAnimBlend *func_005807D0(CZSealBody *seal, bool other, int state)
{
    CZBodyAnimBlend *b;

    seal->DropBody();
    func_005781C0(seal);
    func_005A5DA0(seal);
    func_0059D750(seal);
    func_005B14E0(&seal->m_climb, seal);
    if (seal->m_animset == data_0044D410) {
        if (seal->IsRemote())
            func_00545EE0(seal, 1);
        if (seal->m_item == ITEM_PISTOL)
            func_005A7690(seal);
        else
            func_005A7480(seal);
        seal->BindAnimSet(seal->m_default_animset, false);
    }
    seal->ClearToBase(false);
    if (seal->IsRemote()) {
        if (!seal->m_isAlive)
            state = 8;
    } else if (seal->m_isAlive)
        state = 0;
    else
        state = 8;
    if (other) {
        b = (CZBodyAnimBlend *)func_00588BC0(seal, (AnimType)data_003DEED8, state, 0, 13);
        b->m_flags = func_0058A920(b->m_flags, 2, true);
        if (theNetwork.m_9 && !seal->IsRemote())
            func_002BBAF0(seal->m_id, 34, state, false, 11);
    } else {
        b = (CZBodyAnimBlend *)func_00588BC0(seal, (AnimType)data_003DEED0, state, 0, 13);
        b->m_flags = func_0058A920(b->m_flags, 2, true);
        if (theNetwork.m_9 && !seal->IsRemote())
            func_002BBAF0(seal->m_id, 34, state, false, 10);
    }
    seal->SetBit9(true);
    seal->SetBit10(false);
    seal->m_1364 = -1.0f;
    return b;
}

/* a move begun: the blends cleared to the base, the anim of 0x3DEEF0 pushed (the other way, of
 * 0x3DEEE8) in a state -- a remote SEAL's as given unless it is dead, this console's 2 alive and 8 dead
 * -- with flag 2, and told to the network as anim 34 at 18 (17); alive, the desired state stand and the
 * flag set once more; the root part let go; the blend back (OVL2 0x5805B0, new in SOCOM II: 0x5804E0
 * and 0x5880E0 call it -- the name unknown) */
extern "C" CZBodyAnimBlend *func_005805B0(CZSealBody *seal, bool other, int state)
{
    CZBodyAnimBlend *b;

    seal->ClearToBase(false);
    if (seal->IsRemote()) {
        if (!seal->m_isAlive)
            state = 8;
    } else if (seal->m_isAlive)
        state = 2;
    else
        state = 8;
    if (other) {
        b = (CZBodyAnimBlend *)func_00588BC0(seal, (AnimType)data_003DEEF0, state, 0, 13);
        b->m_flags = func_0058A920(b->m_flags, 2, true);
        if (theNetwork.m_9 && !seal->IsRemote())
            func_002BBAF0(seal->m_id, 34, state, false, 18);
    } else {
        b = (CZBodyAnimBlend *)func_00588BC0(seal, (AnimType)data_003DEEE8, state, 0, 13);
        b->m_flags = func_0058A920(b->m_flags, 2, true);
        if (theNetwork.m_9 && !seal->IsRemote())
            func_002BBAF0(seal->m_id, 34, state, false, 17);
    }
    if (seal->m_isAlive) {
        seal->m_desiredState = stateStand;
        b->m_flags = func_0058A920(b->m_flags, 2, true);
    }
    seal->m_root->SetActive(false);
    return b;
}

/* whether the top blend is in one of the two anims 0x5804E0 starts a move from (an inline: its `||`
 * is materialised before the test) */
static inline bool InMoveAnim(CZSealBody *seal)
{
    bool r = seal->IsCurrentBlend((AnimType)data_003DEED0) || seal->IsCurrentBlend((AnimType)data_003DEED8);

    return r;
}

/* whether the SEAL may go on: always with no blend; never online with bit 13 of 0x1060 set; never
 * stunned in one of four anims (a dead SEAL's top blend then put in state 8); not in a dead top blend
 * outside state 10 unless in a move's anims; not alive in the anim of 0x3DEAD8 (OVL2 0x589900, new in
 * SOCOM II; the name unknown -- 0x57E130, 0x57F7D0 and 0x57DDF0 ask it) */
extern "C" bool func_00589900(CZSealBody *seal)
{
    CZBodyAnimBlend *top = (CZBodyAnimBlend *)func_00555170(&seal->m_blends);

    if (!top)
        return true;
    if (theNetwork.m_9 && seal->m_1060_bit13)
        return false;
    if (seal->m_am_stunned) {
        if (seal->IsCurrentBlend((AnimType)data_003DEED0) || seal->IsCurrentBlend((AnimType)data_003DEED8) ||
            seal->IsCurrentBlend((AnimType)data_003DEEF0) || seal->IsCurrentBlend((AnimType)data_003DEEE8)) {
            if (!seal->m_isAlive) {
                CZBodyAnimBlend *b = (CZBodyAnimBlend *)func_00555170((char *)seal + 0x1c0);

                b->m_state = 8;
            }
            return false;
        }
    }
    if (seal->m_state != 10 && top->m_state == 8) {
        if (!InMoveAnim(seal))
            return false;
    }
    if (seal->m_isAlive && seal->IsCurrentBlend((AnimType)data_003DEAD8))
        return false;
    return true;
}

/* the top blend, in anim 46, made to play backwards from state 5 with its flag 0x20 set, and the push
 * told to the network for a SEAL this console plays (OVL2 0x57F5B0, new in SOCOM II) */
extern "C" CZBodyAnimBlend *func_0057F5B0(CZSealBody *seal)
{
    CZBodyAnimBlend *b = (CZBodyAnimBlend *)func_00555170((char *)seal + 0x1c0);

    if (seal->IsCurrentBlend((AnimType)46)) {
        b->m_flags = func_0058A920(b->m_flags, 0x20, true);
        b->m_state = 5;
        b->Representative()->SetPlayback(-1.0f);
    }
    func_00588B40(seal, 20);
    return b;
}

/* where a throw or a shot would leave from, in the world: for a throw the offset of the base stance's
 * throw or toss -- a peek's to its side, a run's (faster than 20, mostly forward) with the rifle's or
 * the pistol's -- turned by the node; else the fire point's spot while it is set, the SEAL's position
 * added (OVL2 0x57FA70: SOCOM 1's GetPutativeFirePointW by its order -- after the ladder climbs,
 * before GetThrowAnim and BeginThrowItem, as 0x57FCE0 and 0x5802B0 -- and its shape; SOCOM II asks
 * the speed squared and the node's matrix out of line; CAiSDeploy::Tick calls it) */
void CZSealBody::GetPutativeFirePointW(bool thrown, bool toss, CPnt3D &out)
{
    if (thrown) {
        const CPnt3D *off;
        /* (a float local: compared through it the z stays the compare's first operand, as in both
         * builds -- against the literal MW puts the zero first) */
        float zero = 0.0f;

        switch (PeekBaseState()) {
        case 0:
            if (m_state == statePeek)
            {
                if (m_desiredStance == STANCE_NONE)
                    off = &stand_peek_ltoss;
                else
                    off = &stand_peek_rtoss;
            }
            else if (Dot(m_velM, m_velM) > 400.0f &&
                     m_velM.z != zero && fabsf(m_velM.x / m_velM.z) < 0.5f)
            {
                if (m_item == ITEM_RIFLE)
                    off = &running_rifle_throw;
                else
                    off = &running_pistol_throw;
            } else if (toss)
                off = &stand_toss;
            else
                off = &stand_throw;
            break;
        case 1:
            if (toss)
                off = &crouch_toss;
            else
                off = &crouch_throw;
            break;
        case 2:
            if (toss)
                off = &prone_toss;
            else
                off = &prone_throw;
            break;
        default:
            off = &CPnt3D::zero;
            break;
        }
        m_node->m_matrix.Transform(off, &out, 1);
    } else {
        if (m_curr_firept)
            out = m_curr_firepos;
        else
            out = CPnt3D::zero;
        out.Add(&m_posW, &out);
    }
}

/* 0x57F7D0's twin, word for word (OVL2 0x57F9C0, new in SOCOM II: 0x5B2620 asks both in turn) */
extern "C" bool func_0057F9C0(CZSealBody *seal)
{
    if (seal->IsRemote())
        return false;
    if (seal->IsCurrentBlend((AnimType)data_003DEAE0))
        return false;
    CZBodyAnimBlend *top = (CZBodyAnimBlend *)func_00555170((char *)seal + 0x1c0);

    if (seal->m_state == 5 || top->m_state == 5)
        return false;
    return func_00589900(seal);
}

/* a point and two values kept for a SEAL this console plays, cleared for a remote one; whether they
 * were kept (OVL2 0x57ED10, new in SOCOM II: RemoteWeaponDamageVictim and three others call it) */
extern "C" bool func_0057ED10(CZSealBody *seal, const CPnt3D *p, float a, float b)
{
    if (seal->IsRemote()) {
        seal->m_1314 = CPnt3D::zero;
        seal->m_1320 = 0.0f;
        seal->m_1324 = 0.0f;
        return false;
    }
    CPnt3D t = *p;

    seal->m_1314 = t;
    seal->m_1320 = a;
    seal->m_1324 = b;
    return true;
}

/* whether the SEAL can take a stance now (OVL2 0x5857E0, below: Crouch inlines it) */
extern "C" bool func_005857E0(CZSealBody *seal);

/* a crouch: when the SEAL cannot crouch, a stand asked instead (the stance change marked); unless
 * the body or its top blend crouches already, the blends popped down to one and Crouch_0 asked from
 * the stance the body was in, at the old top's time (OVL2 0x581990: SOCOM 1's Crouch, CanCrouch and
 * Stand(1) inlined in both builds).
 * OPEN, 5 instructions (the size right): in the inlined Stand(1) the target gives Stand's stance s1
 * (b's register) and ClearStack's stack pointer s0, ours the other way round; Stand's and Crouch's
 * declaration orders (stance and t declared first, uninitialised, or the crouch path first) change
 * nothing or lose more. Round 36, MW's colouring (igdump): Stand's stance has exactly K = 25
 * neighbours (18 real; this, ClearStack's counter and stack pointer, the count and its +1, the top
 * blend's state byte, the prone constant), so it waits for the second simplify sweep and is coloured
 * ahead of the stack pointer (23, the first sweep) -- the target's graph has one interference fewer
 * somewhere; ClearStack's fallback read spelled through `stack` or `&m_blends`, or its locals
 * reordered, does not do it (5-15 off). Round 42, by the replay (tools/igreplay.py --target, --want
 * --movers): no single dropped interference gives the target (dropping one of the stance's temporaries
 * gives it s3), and the only numberings that do put the INNER ClearStack's counter (v43, @7504)
 * ahead of Stand's stance (v38, @7481): the stance moved to 43.5, or the counter anywhere in 31.5-37.5
 * -- pushed first in sweep 1 it drops the stance under K, and the stance, coloured after the inner
 * stack pointer (s0), takes s1. MW numbers Crouch's own ClearStack (v40/v41) between Stand's locals
 * and Stand's inner ClearStack (v42-v44): an inline's own inlines are numbered a level later, so the
 * counter would have to come out of that later level. Not it (igvar, build/scratch/seal/v_crouch*.py):
 * the stance or the whole body in a nested block, ClearStack's counter first or its stack declared
 * first and defined late, the stack an inline's parameter (BlendCount(&m_blends): unhoisted, 4 bytes
 * short), the stance through a value or a reference inline, `(stance = m_state) == 0` */
CZBodyAnimBlend *CZSealBody::Crouch()
{
    CZBodyAnimBlend *b = (CZBodyAnimBlend *)func_00555170(&m_blends);

    if (!func_005857E0(this)) {
        m_desiredState = stateStand;
        m_stancechanged = true;
        return Stand(1);
    }
    int stance = m_state;

    if (stance == 1 || b->m_state == 1)
        return b;
    float t = b->m_tfrac;

    ClearStack(1, true);
    return Crouch_0(stance, t);
}

/* a crouch's blends: from prone the get-up anims, from a stand the drop -- at a run (faster than
 * 20) or a walk (faster than 10) its own anim at the stand's time with the body put in state 1 and
 * its interpolation zeroed, else the plain one -- and from a hang the stand first (OVL2 0x5817D0:
 * SOCOM 1's Crouch_0, the same switch; SOCOM II asks the speed squared) */
CZBodyAnimBlend *CZSealBody::Crouch_0(int stance, float t)
{
    CZBodyAnimBlend *b = 0;

    switch (stance) {
    case 2:
        func_00588BC0(this, (AnimType)18, 1, 0, 0);
        b = (CZBodyAnimBlend *)func_00588BC0(this, (AnimType)19, 1, 1, 0);
        break;
    case 0: {
        func_00588BC0(this, (AnimType)18, 1, 0, 0);
        float d = m_velM.x * m_velM.x + m_velM.y * m_velM.y + m_velM.z * m_velM.z;

        if (d > 400.0f) {
            b = (CZBodyAnimBlend *)func_00588BC0(this, (AnimType)2, 1, 0, 0);
            b->m_tfrac = t;
            BodySetState(1);
            m_interp1 = 0.0f;
            m_interp2 = 0.0f;
        } else if (d > 100.0f) {
            b = (CZBodyAnimBlend *)func_00588BC0(this, (AnimType)20, 1, 0, 0);
            b->m_tfrac = t;
            BodySetState(1);
            m_interp1 = 0.0f;
            m_interp2 = 0.0f;
        } else
            b = (CZBodyAnimBlend *)func_00588BC0(this, (AnimType)17, 1, 0, 0);
        break;
    }
    case 4:
        Crouch_0(0, -1.0f);
        Stand_0(4, -1, -1.0f);
        break;
    }
    return b;
}

/* whether a SEAL in water is shallow enough to go prone: 2 deep at most (an inline: Prone's test,
 * materialised as 0x5857E0's is in Crouch) */
static inline bool CanProneInWater(CZSealBody *seal)
{
    if (seal->m_inwater && !(seal->m_waterdepth <= 2.0f))
        return false;
    return true;
}

/* a prone: in water too deep, a crouch asked instead (the stance change marked); unless the body or
 * its top blend is prone already, the blends popped down to one and Prone_0 asked from the stance the
 * body was in (OVL2 0x581660: SOCOM 1's Prone, its aim and crouch tests gone) */
CZBodyAnimBlend *CZSealBody::Prone()
{
    CZBodyAnimBlend *b = (CZBodyAnimBlend *)func_00555170(&m_blends);

    if (!CanProneInWater(this)) {
        m_desiredState = stateCrouch;
        m_stancechanged = true;
        return Crouch();
    }
    int stance = m_state;

    if (stance == 2 || b->m_state == 2)
        return b;
    ClearStack(1, true);
    return Prone_0(stance);
}

/* a stand: unless the body or its top blend stands already, the blends popped down to one as 0x57F310
 * does and Stand_0 asked from the stance the body was in, at the old top's time (OVL2 0x581DC0: SOCOM
 * 1's Stand, the same tests; Crouch inlines it) */
CZBodyAnimBlend *CZSealBody::Stand(int n)
{
    CZBodyAnimBlend *b = (CZBodyAnimBlend *)func_00555170(&m_blends);
    int stance = m_state;

    if (stance == 0 || b->m_state == 0)
        return b;
    float t = b->m_tfrac;

    ClearStack(1, true);
    return Stand_0(stance, n, t);
}


/* a stand: from a crouch the get-up anim (run on when moving faster than 10), from prone SOCOM 1's
 * Stand2Prone with the representative's bit 1 cleared, from a hang one way or the other, from idle
 * the stand itself (OVL2 0x581C10: SOCOM 1's Stand_0, the same switch; SOCOM II passes the crouch's
 * time in and asks the speed squared) */
CZBodyAnimBlend *CZSealBody::Stand_0(int stance, int n, float t)
{
    CZBodyAnimBlend *b = 0;

    switch (stance) {
    case 1:
        if (m_velM.x * m_velM.x + m_velM.y * m_velM.y + m_velM.z * m_velM.z > 100.0f) {
            b = (CZBodyAnimBlend *)func_00588BC0(this, (AnimType)2, 0, 0, 0);
            b->m_tfrac = t;
            BodySetState(0);
        } else
            b = (CZBodyAnimBlend *)func_00588BC0(this, (AnimType)17, 0, 1, 0);
        m_21c_bit2 = false;
        break;
    case 2:
        b = (CZBodyAnimBlend *)func_00588BC0(this, (AnimType)AnimTypes::Stand2Prone, 0, 1, 0);
        b->Representative()->SetBit1(false);
        break;
    case 4:
        if (n > 0)
            b = (CZBodyAnimBlend *)func_00588BC0(this, (AnimType)35, 0, 0, 0);
        else
            b = (CZBodyAnimBlend *)func_00588BC0(this, (AnimType)36, 0, 0, 0);
        break;
    case 7:
        b = (CZBodyAnimBlend *)func_00588BC0(this, (AnimType)1, 0, 0, 0);
        break;
    }
    return b;
}

/* a prone: the drop anims pushed from a crouch or a stand, and from a hang stood up first through
 * Stand_0; the last foot set to 2 (OVL2 0x581540: SOCOM 1's Prone_0, the same switch; SOCOM II drops
 * the representative's flag clear) */
#pragma auto_inline off
CZBodyAnimBlend *CZSealBody::Prone_0(int stance)
{
    CZBodyAnimBlend *b = 0;

    switch (stance) {
    case 1:
        func_00588BC0(this, (AnimType)26, 2, 0, 0);
        b = (CZBodyAnimBlend *)func_00588BC0(this, (AnimType)19, 2, 0, 0);
        break;
    case 0:
        func_00588BC0(this, (AnimType)26, 2, 0, 0);
        b = (CZBodyAnimBlend *)func_00588BC0(this, (AnimType)25, 2, 0, 0);
        break;
    case 4:
        func_00588BC0(this, (AnimType)26, 2, 0, 0);
        func_00588BC0(this, (AnimType)25, 2, 0, 0);
        m_last_foot = 2;
        Stand_0(4, -1, -1.0f);
        break;
    }
    m_last_foot = 2;
    return b;
}
#pragma auto_inline reset

/* unless the body or its top blend is in state 7 (idle): the blends popped down to one, twelve at
 * most, as 0x57F310 does, and the idle anim given kept and pushed in state 7 (OVL2 0x581430: SOCOM 1's
 * SetStanceIdle, the same tests and pops; SOCOM II drops the representative's flag clear after) */
CZBodyAnimBlend *CZSealBody::SetStanceIdle(AnimType type)
{
    CZBodyAnimBlend *b = (CZBodyAnimBlend *)func_00555170(&m_blends);

    if (m_state == 7 || b->m_state == 7)
        return b;
    b = (CZBodyAnimBlend *)func_00555170(&m_blends);
    int n = 12;
    CBodyAnimBlendStack *stack = &m_blends;

    while (BlendCount(stack) > 1 && n > 0) {
        b = func_00589AA0(this);
        n--;
    }
    if (n <= 0)
        b = (CZBodyAnimBlend *)func_00555170((char *)this + 0x1c0);
    else {
        if (b)
            b->m_ctrl = 0;
        m_desiredStance = STANCE_PRONE;
    }
    idlehack = type;
    return (CZBodyAnimBlend *)func_00588BC0(this, type, 7, 0, 0);
}

/* a hang: the hang anims pushed from a stand, a crouch or prone first stood up through Stand_0 (OVL2
 * 0x5812C0: SOCOM 1's Hang_0, the same switches; SOCOM II drops the representative's flag clear) */
CZBodyAnimBlend *CZSealBody::Hang_0(int stance, int n)
{
    func_00555170(&m_blends);
    CZBodyAnimBlend *b = 0;

    if (n > 0) {
        switch (stance) {
        case 0:
            func_00588BC0(this, (AnimType)34, 4, 0, 0);
            b = (CZBodyAnimBlend *)func_00588BC0(this, (AnimType)33, 4, 0, 0);
            break;
        case 1:
        case 2:
            Hang_0(0, n);
            Stand_0(stance, 0, -1.0f);
            break;
        }
    } else {
        switch (stance) {
        case 0:
            func_00588BC0(this, (AnimType)34, 4, 0, 0);
            b = (CZBodyAnimBlend *)func_00588BC0(this, (AnimType)37, 4, 0, 0);
            break;
        case 1:
        case 2:
            Hang_0(0, n);
            Stand_0(stance, 0, -1.0f);
            break;
        }
    }
    return b;
}


/* a climb blended by its height: the medium climb's blend pushed, its list filled with anim 42 at a
 * weight falling from 1 at 12 to 0 at 26.5 and the medium climb at the rest (OVL2 0x581110: SOCOM 1's
 * BlendedClimb, the same weights; anim 42 where SOCOM 1 had 44) */
CZBodyAnimBlend *CZSealBody::BlendedClimb(float h)
{
    newanims.Clear();
    CZBodyAnimBlend *b = (CZBodyAnimBlend *)func_00588BC0(this, (AnimType)AnimTypes::ClimbMedium, 0, 0, 0);
    float w;

    if (h <= 12.0f)
        w = 1.0f;
    else if (h >= 26.5f)
        w = 0.0f;
    else
        w = 1.0f - (h - 12.0f) / 14.5f;
    if (w > 0.0f) {
        ACTIVE_ANIM *a = m_active_anim_pool.GetActiveAnim(m_animset->GetScript((AnimType)42), -1);

        a->m_id = 42;
        PushAnim(&newanims, a, w);
    }
    if (w < 1.0f) {
        ACTIVE_ANIM *a = m_active_anim_pool.GetActiveAnim(m_animset->GetScript((AnimType)AnimTypes::ClimbMedium), -1);

        a->m_id = AnimTypes::ClimbMedium;
        PushAnim(&newanims, a, 1.0f - w);
    }
    b->SetBlendAnims(this, &newanims);
    return b;
}

extern "C" {
/* (OVL2 0x5A0950, seal_env.cpp's, not written yet: an anim type chosen by a kind and a variant; a
 * fall's damage is CZSealBody::HandleFallingDamage, 0x5AC1F0) */
AnimType func_005A0950(CZSealBody *seal, int kind, int variant);
}

/* a landing: the jump flag cleared; unless the SEAL is invincible or remote, the fall's damage taken
 * and, when that killed it, the blends popped down to two, as 0x57F310 does, and the death anim pushed
 * in state 8; whether the SEAL lives (OVL2 0x580360, new in SOCOM II) */
static inline bool Invincible(CZSealBody *seal)
{
    return seal->m_invincible ? true : false;
}

extern "C" bool func_00580360(CZSealBody *seal, float t)
{
    seal->m_jumped = false;
    if (Invincible(seal) || seal->IsRemote())
        return true;
    seal->HandleFallingDamage(1.2f * t);
    if (!seal->m_isAlive) {
        CZBodyAnimBlend *b = (CZBodyAnimBlend *)func_00555170(&seal->m_blends);
        int n = 12;
        CBodyAnimBlendStack *stack = &seal->m_blends;

        while (BlendCount(stack) > 2 && n > 0) {
            b = func_00589AA0(seal);
            n--;
        }
        if (n <= 0)
            func_00555170((char *)seal + 0x1c0);
        else {
            if (b)
                b->m_ctrl = 0;
            seal->m_desiredStance = STANCE_PRONE;
        }
        func_00588BC0(seal, func_005A0950(seal, 3, 2), 8, 0, 0);
        return false;
    }
    return true;
}

/* the climb over: out of AnimTypes::ClimbOver the blends popped down to one, twelve at most, as
 * 0x57F310 does, and that anim pushed; in it, only the top blend fetched (OVL2 0x580A80, SOCOM 1's
 * ClimbOver with the limit added, the round 48's name; its float never read) */
void CZSealBody::ClimbOver(float)
{
    if (!IsCurrentBlend((AnimType)AnimTypes::ClimbOver)) {
        CZBodyAnimBlend *b = (CZBodyAnimBlend *)func_00555170(&m_blends);
        int n = 12;
        CBodyAnimBlendStack *stack = &m_blends;

        while (BlendCount(stack) > 1 && n > 0) {
            b = func_00589AA0(this);
            n--;
        }
        if (n <= 0)
            b = (CZBodyAnimBlend *)func_00555170((char *)this + 0x1c0);
        else {
            if (b)
                b->m_ctrl = 0;
            m_desiredStance = STANCE_PRONE;
        }
        func_00588BC0(this, (AnimType)AnimTypes::ClimbOver, 0, 0, 0);
    } else
        func_00555170((char *)this + 0x1c0);
}

/* the blends popped down to one, as 0x57F310 does, then anim 45 pushed from its start and anim 46 on
 * top of it, told to the network with the ladder's index for a SEAL this console plays; the second
 * blend returned (OVL2 0x57F690, new in SOCOM II) */
extern "C" CZBodyAnimBlend *func_0057F690(CZSealBody *seal)
{
    CZBodyAnimBlend *b = (CZBodyAnimBlend *)func_00555170(&seal->m_blends);
    int n = 12;
    CBodyAnimBlendStack *stack = &seal->m_blends;

    while (BlendCount(stack) > 1 && n > 0) {
        b = func_00589AA0(seal);
        n--;
    }
    if (n <= 0)
        b = (CZBodyAnimBlend *)func_00555170((char *)seal + 0x1c0);
    else {
        if (b)
            b->m_ctrl = 0;
        seal->m_desiredStance = STANCE_PRONE;
    }
    {
        /* (the blend in a block-local: written into the call's result, MW keeps 0.0f in $f20) */
        CZBodyAnimBlend *start = (CZBodyAnimBlend *)func_00588BC0(seal, (AnimType)45, 5, 0, 13);

        start->m_tfrac = 0.0f;
    }
    /* (the ladder declared first: it takes the higher register) */
    unsigned int ladder;
    CZBodyAnimBlend *r = (CZBodyAnimBlend *)func_00588BC0(seal, (AnimType)46, 5, 1, 13);

    ladder = seal->m_ladder;
    if (theNetwork.m_9 && !seal->IsRemote())
        func_002BBAF0(seal->m_id, 34, ladder, false, 6);
    return r;
}

extern "C" {
/* two more anim types read as variables (OVL1 data, -1 in the image) */
extern short data_003DEF40;
extern short data_003DEF50;
}

/* the blends popped down to one, as 0x57F310 does, the anims of 0x3DEF50 and 0x3DEF40 pushed, the
 * body put in state 5 and the push told to the network; the second blend returned (OVL2 0x57F3E0,
 * new in SOCOM II) */
extern "C" CZBodyAnimBlend *func_0057F3E0(CZSealBody *seal)
{
    CZBodyAnimBlend *b = (CZBodyAnimBlend *)func_00555170(&seal->m_blends);
    int n = 12;
    CBodyAnimBlendStack *stack = &seal->m_blends;

    while (BlendCount(stack) > 1 && n > 0) {
        b = func_00589AA0(seal);
        n--;
    }
    if (n <= 0)
        b = (CZBodyAnimBlend *)func_00555170((char *)seal + 0x1c0);
    else {
        if (b)
            b->m_ctrl = 0;
        seal->m_desiredStance = STANCE_PRONE;
    }
    func_00588BC0(seal, (AnimType)data_003DEF50, 5, 0, 13);
    CZBodyAnimBlend *r = (CZBodyAnimBlend *)func_00588BC0(seal, (AnimType)data_003DEF40, 5, 0, 13);

    seal->BodySetState(5);
    func_00588B40(seal, 7);
    return r;
}

/* 0x57F690 with anim 44 alone on top and the time 5 (OVL2 0x57F880, new in SOCOM II) */
extern "C" CZBodyAnimBlend *func_0057F880(CZSealBody *seal)
{
    CZBodyAnimBlend *b = (CZBodyAnimBlend *)func_00555170(&seal->m_blends);
    int n = 12;
    CBodyAnimBlendStack *stack = &seal->m_blends;

    while (BlendCount(stack) > 1 && n > 0) {
        b = func_00589AA0(seal);
        n--;
    }
    if (n <= 0)
        b = (CZBodyAnimBlend *)func_00555170((char *)seal + 0x1c0);
    else {
        if (b)
            b->m_ctrl = 0;
        seal->m_desiredStance = STANCE_PRONE;
    }
    {
        /* (the blend in a block-local: written into the call's result, MW keeps 0.0f in $f20) */
        CZBodyAnimBlend *start = (CZBodyAnimBlend *)func_00588BC0(seal, (AnimType)45, 5, 0, 13);

        start->m_tfrac = 0.0f;
    }
    unsigned int ladder;
    CZBodyAnimBlend *r = (CZBodyAnimBlend *)func_00588BC0(seal, (AnimType)44, 5, 0, 13);

    ladder = seal->m_ladder;
    if (theNetwork.m_9 && !seal->IsRemote())
        func_002BBAF0(seal->m_id, 34, ladder, false, 5);
    return r;
}

/* the blends popped down to a count, twelve at most; when that worked and when asked, the top
 * blend's control cleared and the SEAL asked to go prone (OVL2 0x58A5E0, new in SOCOM II: 0x57F310
 * with the count and the flag its arguments) */
void CZSealBody::ClearStack(unsigned int count, bool prone)
{
    CZBodyAnimBlend *b = (CZBodyAnimBlend *)func_00555170(&m_blends);
    int n = 12;
    CBodyAnimBlendStack *stack = &m_blends;

    while (BlendCount(stack) > count && n > 0)
        b = func_00589AA0(this), n--;
    if (n <= 0)
        b = (CZBodyAnimBlend *)func_00555170((char *)this + 0x1c0);
    else if (prone) {
        if (b)
            b->m_ctrl = 0;
        m_desiredStance = STANCE_PRONE;
    }
}

/* whether a SEAL this console plays, out of the anim of 0x3DEAE0 and neither its state nor its top
 * blend's 5, can go on as 0x589900 says (OVL2 0x57F7D0, new in SOCOM II) */
extern "C" bool func_0057F7D0(CZSealBody *seal)
{
    if (seal->IsRemote())
        return false;
    if (seal->IsCurrentBlend((AnimType)data_003DEAE0))
        return false;
    CZBodyAnimBlend *top = (CZBodyAnimBlend *)func_00555170((char *)seal + 0x1c0);

    if (seal->m_state == 5 || top->m_state == 5)
        return false;
    return func_00589900(seal);
}

/* whether a SEAL this console plays, in anim 45, can go on as 0x589900 says (OVL2 0x57F530, new in
 * SOCOM II).
 * OPEN, 7 instructions: the target lays the anim test's false return right after it (`bnez v0` to
 * the call's block, which follows); MW puts the call first and the false block last. Tried: the
 * test positive with the call inside, an else, a ternary, `== false`, `||` and `&&` joins of both
 * tests, a result local. */
extern "C" bool func_0057F530(CZSealBody *seal)
{
    if (seal->IsRemote())
        return false;
    if (!seal->IsCurrentBlend((AnimType)45))
        return false;
    return func_00589900(seal);
}

/* whether one of three anim types is the blend on top (OVL2 0x588080; new in SOCOM II, its name
 * unknown -- seal.cpp's 0x552EF4 and seal_control.cpp's 0x593C20 ask it) */
extern "C" bool func_00588080(CZSealBody *seal)
{
    return seal->IsCurrentBlend((AnimType)17) || seal->IsCurrentBlend((AnimType)19) ||
           seal->IsCurrentBlend((AnimType)25);
}

extern "C" {
/* two anim types' ids (OVL1 data 0x3DEED0, 0x3DEED8) */
extern short data_003DEED0;
extern short data_003DEED8;
}

/* whether one of two anim types is the blend on top (OVL2 0x588030; new in SOCOM II, its name
 * unknown -- seal.cpp's turret boarding 0x546210 asks it) */
extern "C" bool func_00588030(CZSealBody *seal)
{
    return seal->IsCurrentBlend((AnimType)data_003DEED0) || seal->IsCurrentBlend((AnimType)data_003DEED8);
}

extern "C" {
/* (seal.cpp's 0x555170: the blend stack's top) */
void *func_00555170(void *v);
}

/* the blends' count, compared signed (SOCOM 1's ClearToBase tests it with `slti` too) */
static inline int SignedBlendCount(CBodyAnimBlendStack *stack)
{
    return stack->m_top + 1;
}

/* the blends popped down to the base stance's, twelve at most -- stopping at a stand (1), a crouch
 * (18) or a prone (26) on top -- and one more when asked and there is one; the top blend's control
 * cleared and stance 2 asked; the new top returned (OVL2 0x58A4D0: SOCOM 1's ClearToBase, the same
 * three base anims and the extra pop; CSealCtrl::PreTick reaches it through 0x57DC80 from a peek) */
CZBodyAnimBlend *CZSealBody::ClearToBase(bool pop)
{
    int n = 12;
    CBodyAnimBlendStack *stack = &m_blends;

    while (SignedBlendCount(stack) > 1 && n > 0) {
        if (IsCurrentBlend((AnimType)1) || IsCurrentBlend((AnimType)18) || IsCurrentBlend((AnimType)26))
            break;
        func_00589AA0(this);
        n--;
    }
    if (n <= 0)
        return (CZBodyAnimBlend *)func_00555170((char *)this + 0x1c0);
    if (pop && SignedBlendCount(stack) > 1)
        func_00589AA0(this);
    ((CZBodyAnimBlend *)func_00555170(&m_blends))->m_ctrl = 0;
    m_desiredStance = STANCE_PRONE;
    return (CZBodyAnimBlend *)func_00555170((char *)this + 0x1c0);
}

/* bits of a halfword set or cleared (OVL2 0x58A920; seal_body.cpp's and seal_env.cpp's callers --
 * pinned: 0x57F5B0 calls it out of line) */
#pragma auto_inline off
extern "C" unsigned short func_0058A920(unsigned short flags, unsigned short bits, bool on)
{
    flags &= ~bits;
    if (on)
        flags |= bits;
    return flags;
}
#pragma auto_inline reset

extern "C" {
/* an animation set the SEALs share (OVL1 bss 0x44D408) */
extern AnimSet *data_0044D408;
}

extern "C" {
/* (OVL2 0x5A60D0: a stance change, a kind and a flag; 0x5765F0, seal_anim.cpp's) */
void func_005A60D0(CZSealBody *seal, int a, int b);
void func_005765F0(CZSealBody *seal);
}

/* the overlay stepped: once it plays out its event's stance change made (5, 6, 8 and 10), the blend
 * pushed on when the body interpolates, the 0x10D0 timer set (OVL2 0x579EF0, new in SOCOM II;
 * 0x57A330 calls it) */
extern "C" void func_00579EF0(CZSealBody *seal)
{
    CBlendOverlay &ov = seal->m_overlay;

    if (ov.m_anim) {
        int event = ov.m_event;

        if (ov.Tick(seal, seal->m_2e0) != false) {
            switch (event) {
            case 5:
                func_005A60D0(seal, 1, 0);
                break;
            case 6:
                func_005A60D0(seal, 2, 0);
                break;
            case 8:
                func_005A60D0(seal, 1, 4);
                break;
            case 10:
                func_005A60D0(seal, 2, 4);
                break;
            }
            if (!(seal->m_interp1 <= 0.0f))
                func_005765F0(seal);
            seal->m_10d0 = 0.4f;
        }
    }
}

/* for a SEAL: the blends popped down to two, twelve at most -- when that worked the top blend's
 * control cleared and the SEAL asked to go prone, as 0x57F310 does -- its movement velocity zeroed,
 * then CrouchToPickupBuddy 0x579A30 on the buddy it was handed as an int (OVL2 0x579DA0, new in SOCOM
 * II: 0x5880E0 calls it; the name unknown). The stack's
 * address is shared by both Top calls here (both spelled alike). */
extern "C" void func_00579DA0(CZSealBody *seal, int a)
{
    if (seal) {
        CZBodyAnimBlend *b = (CZBodyAnimBlend *)func_00555170((char *)seal + 0x1c0);
        int n = 12;
        CBodyAnimBlendStack *stack = &seal->m_blends;

        while (BlendCount(stack) > 2 && n > 0) {
            b = func_00589AA0(seal);
            n--;
        }
        if (n <= 0)
            b = (CZBodyAnimBlend *)func_00555170((char *)seal + 0x1c0);
        else {
            if (b)
                b->m_ctrl = 0;
            seal->m_desiredStance = STANCE_PRONE;
        }
        seal->m_velM = CPnt3D::zero;
        seal->CrouchToPickupBuddy((CZSealBody *)a);
    }
}

/* the SEAL's animation set (an inline: in a call's argument list it is expanded first, so the set
 * loads ahead of the anim number; the first test reads it through a const view, which keeps MW from
 * sharing that load with the calls -- the target loads it again) */
static inline AnimSet *SetOf(CZSealBody *seal)
{
    return seal->m_animset;
}

/* whether the SEAL can take a stance now: always with the shared set; otherwise its set must have
 * anims 18 and 84, and a SEAL in water must be slow enough (OVL2 0x5857E0, new in SOCOM II;
 * CSealCtrl::PreTick asks it twice) */
extern "C" bool func_005857E0(CZSealBody *seal)
{
    if (((const CZSealBody *)seal)->m_animset == data_0044D408)
        return true;
    if (SetOf(seal)->AnimId((AnimType)18) < 0)
        return false;
    if (SetOf(seal)->AnimId((AnimType)84) < 0)
        return false;
    if (seal->m_inwater && !(seal->m_waterdepth <= 8.5f))
        return false;
    return true;
}

extern "C" {
/* an AnimTypes static (OVL1 data 0x3DEF08, 0x2C0 past Stand2Prone: SOCOM 1's list does not reach it) */
extern short data_003DEF08;
}

/* not in water deeper than a depth (an inline: the target builds the answer in v0 -- the false arm
 * first, the true one a `daddiu` at the branches' target -- and then tests it) */
static inline bool ShallowWater(CZSealBody *seal, float depth)
{
    bool r;

    if (seal->m_inwater && !(seal->m_waterdepth <= depth))
        r = false;
    else
        r = true;
    return r;
}

/* whether the SEAL may break into its run: not in that anim's blend, carrying nobody, in no water
 * deeper than 2, not changing weapons, moving at 30 or more, and as 0x589900 allows (OVL2 0x584B00, new
 * in SOCOM II: CSealCtrl::PreTick, 0x552D60 and 0x609190 ask it; the name unknown). 0x589900 tested
 * and true returned, as 0x57E130 does: returning its answer puts the call's argument in the speed test's
 * delay slot */
extern "C" bool func_00584B00(CZSealBody *seal)
{
    if (seal->IsCurrentBlend((AnimType)data_003DEF08))
        return false;
    if (seal->m_carry)
        return false;
    if (!ShallowWater(seal, 2.0f))
        return false;
    if (seal->IsChangingWeapons(true))
        return false;
    if (seal->m_velM.x * seal->m_velM.x + seal->m_velM.y * seal->m_velM.y + seal->m_velM.z * seal->m_velM.z < 900.0f)
        return false;
    if (!func_00589900(seal))
        return false;
    return true;
}

/* whether the SEAL counts: not a remote SEAL with bit 9 of 0x1060 set (OVL2 0x57DFE0, new in SOCOM
 * II; 0x5AF930 asks it) */
extern "C" bool func_0057DFE0(CZSealBody *seal)
{
    if (seal->IsRemote() && seal->m_bit9)
        return false;
    return true;
}

/* whether the SEAL can act on another: not on a SEAL of its own team, of the 0x20000 team bit, that
 * is no SEAL, a ghost, or whose node is inactive -- and only alive, played by this console and as
 * 0x589900 allows (OVL2 0x57DDF0, new in SOCOM II; the name unknown) */
extern "C" bool func_0057DDF0(CZSealBody *seal, CZSealBody *other)
{
    bool ok = true;

    if (other) {
        unsigned int team = other->m_team;

        if ((team & seal->m_team) || (team & 0x20000) || other->m_type != CEntity::ENT_TYPE_SEAL ||
            other->m_deathType == DEATH_GHOST || (other->GetNode() && !other->GetNode()->m_active))
            ok = false;
    }
    if (!seal->m_isAlive)
        ok = false;
    else if (seal->IsRemote())
        ok = false;
    else if (!func_00589900(seal))
        ok = false;
    return ok;
}

extern "C" {
/* an anim type read as a variable (OVL1 data, -1 in the image) */
extern short data_003DF130;
}

/* the anim of 0x3DF130 pushed over the base stance, kind 13, at a variant of the held weapon's --
 * the one given, or below zero the one the top script's id picks -- and the variant told to the
 * network for a SEAL this console plays (OVL2 0x57DCC0, new in SOCOM II; the name unknown).
 * OPEN, 6 instructions (the size right): in both AnimSet calls the target loads the kind (a2) and the
 * set (a0) before the type's global (a1), ours the type first -- the same as member calls on a
 * declared AnimSet, with the global volatile, const or typed AnimType, the arguments cast to int, or
 * the set assigned in the call; rep declared ahead of b fixed their saved registers */
extern "C" CZBodyAnimBlend *func_0057DCC0(CZSealBody *seal, int variant)
{
    ACTIVE_ANIM *rep;

    seal->ClearToBase(false);
    CZBodyAnimBlend *b = (CZBodyAnimBlend *)func_00588BC0(seal, (AnimType)data_003DF130, seal->m_state, 0, 13);

    if (!b)
        return 0;
    rep = b->Representative();

    if (variant < 0)
        variant = seal->m_animset->GetVariantIndex((AnimType)data_003DF130, seal->m_weapon_id,
                                                    rep->m_anim->m_id);
    rep->m_anim = seal->m_animset->GetVariantScript((AnimType)data_003DF130, seal->m_weapon_id, variant);
    if (theNetwork.m_9 && !seal->IsRemote())
        func_002BBAF0(seal->m_id, 34, variant, false, 12);
    return b;
}

/* (OVL2 0x57DC80; CSealCtrl::PreTick calls it twice) */
extern "C" void *func_0057DC80(CZSealBody *seal)
{
    if (seal->m_state == 3)
        return seal->ClearToBase(false);
    return func_00555170((char *)seal + 0x1c0);
}

/* the state set: the controller told of a change first (OVL2 0x58A6B0; SOCOM 1's, virtual in SOCOM
 * II -- CBody's slot 0x14, CZSealBody's 0x90) */
void CZSealBody::BodySetState(int state)
{
    if (m_control && state != m_state)
        ((CSealCtrl *)m_control)->OnStateChange((SEAL_STATE)m_state, (SEAL_STATE)state);
    CBody::BodySetState(state);   /* (OVL1 0x28DFB0, the body lane's: the state stored) */
}

/* hanging: with the throttles within the hang's reach the blends popped down to the hang (34) -- or to
 * one -- the hang pushed if it is not on top, the top made whole and the moving and turning flags
 * cleared; pushed forward, a stand from the hang (stance 4); pulled back from a top blend still
 * hanging (byte 0x2B), the stack cleared to one and a stand from the state, at the old top's time
 * (OVL2 0x584390: SOCOM 1's InStateHang by order and the state dispatcher's case 4, its throttles read
 * at 0x240/0x244, the sqrt inline, Stand_0's time new). The pulled-back arm is Stand(-1), inlined: its
 * two returns join at one `move s0,v0`, as only an inline's result does; one result for the three arms.
 * OPEN, 5 instructions (the size right): Crouch's miss (0x581990) exactly (round 42's replay there:
 * the inner ClearStack's counter must be numbered ahead of Stand's stance) -- in the inlined Stand the
 * target gives the stance s2 and ClearStack's stack pointer s0, ours the other way round. Tried: the
 * declaration orders of ClearStack's and Stand's locals (Stand and ClearStack still match out of line).
 * Round 36, MW's colouring: the same cause as Crouch's -- Stand's stance has exactly K = 25 neighbours */
CZBodyAnimBlend *CZSealBody::InStateHang()
{
    CZBodyAnimBlend *r;

    func_00555170(&m_blends);
    float x = m_240;
    float z = m_244;

    if (sqrtf(x * x + z * z) <= data_003F3428) {
        CBodyAnimBlendStack *stack = &m_blends;

        while (!IsCurrentBlend((AnimType)34) && SignedBlendCount(stack) > 1)
            func_00589AA0(this);
        if (!IsCurrentBlend((AnimType)34))
            func_00588BC0(this, (AnimType)34, 4, 0, 0);
        r = (CZBodyAnimBlend *)func_00555170(&m_blends);
        r->SetRepresentativeWt(1.0f);
        SetIsMoving(false);
        SetIsTurning(false);
    } else if (!(x <= 0.0f))
        r = Stand_0(4, 1, -1.0f);
    else
        r = Stand(-1);
    return r;
}

/* on a ladder: nothing while the 0x3DEF50 anim is on top; else the climb (anim 45) pushed if it is not,
 * and going down at the foot of the ladder the climb popped and the step off (44) pushed; climbing,
 * the top blend made whole and played at the climb's rate (OVL2 0x584240: SOCOM 1's InStateClimbLadder
 * by order -- after the carry states, before InStateHang -- and by the state dispatcher's case 5;
 * SOCOM II's reads the rate at 0x240 where SOCOM 1's took three floats) */
CZBodyAnimBlend *CZSealBody::InStateClimbLadder()
{
    CZBodyAnimBlend *top = (CZBodyAnimBlend *)func_00555170(&m_blends);
    float rate = m_240;
    const CPnt3D &pos = m_posW;

    if (IsCurrentBlend((AnimType)data_003DEF50))
        return top;
    if (!IsCurrentBlend((AnimType)45))
        func_00588BC0(this, (AnimType)45, 5, 0, 0);
    if (rate < 0.0f && pos.y < 1.0f + m_alt) {
        if (IsCurrentBlend((AnimType)45))
            func_00589AA0(this);
        return (CZBodyAnimBlend *)func_00588BC0(this, (AnimType)44, 0, 1, 0);
    }
    CZBodyAnimBlend *b = (CZBodyAnimBlend *)func_00555170(&m_blends);

    b->SetRepresentativeWt(1.0f);
    ACTIVE_ANIM *rep = b->Representative();

    rep->SetPlayback(rate * rep->m_anim->m_1c);
    return b;
}

extern "C" float fabsf(float x);

/* the top blend turned: its weight made 1 and its playback set from the turn, doubled and negated,
 * its size scaled and capped by the state (0.6 standing, 0.3 crouched, 0.35 prone, 0.6 in states 9
 * and 10), its sign kept (OVL2 0x583960: SOCOM 1's TurnControl, the same caps; SOCOM II sets the
 * representative's playback where SOCOM 1 called its slot 0x14) */
CZBodyAnimBlend *CZSealBody::TurnControl(float turn)
{
    bool neg;
    CZBodyAnimBlend *b = (CZBodyAnimBlend *)func_00555170(&m_blends);

    b->SetRepresentativeWt(1.0f);
    float r = 2.0f * -turn;

    neg = r < 0.0f;
    float a = fabsf(r);

    switch (m_state) {
    case 0:
        a *= 0.6f;
        if (a > 0.6f)
            a = 0.6f;
        break;
    case 1:
        a *= 0.3f;
        if (a > 0.3f)
            a = 0.3f;
        break;
    case 2:
        a *= 0.35f;
        if (a > 0.35f)
            a = 0.35f;
        break;
    case 9:
    case 10:
        a *= 0.6f;
        if (a > 0.6f)
            a = 0.6f;
        break;
    }
    b->Representative()->SetPlayback(neg ? -a : a);
    return b;
}

extern "C" {
/* the anim set the zoom refuses (OVL1 bss; seal_env.cpp's) and a ZAnim let go (OVL1 0x2707C0,
 * unowned: seal_ai.cpp's reading) */
extern AnimSet *data_0044D3F8;
void func_002707C0(CZAnim *anim, bool now);
void func_00585FF0(CZSealBody *seal);
/* the idle state's slack on the third throttle (OVL1 data, 0.025; the other two take 0x3F3428's) */
extern float data_003F3420;
}

/* the idle state's test of the 0x44D3F8 set, its throttles passed by value: once a base stance's blend
 * is up (anims 1, 18 and 26) and a throttle moves past its slack, the set undone; the top blend taken
 * and dropped (an inline, the name ours: the target loads the throttles ahead of the set's test and
 * stores them after it, as arguments of an inline call do -- read into locals of the case first) */
static inline void IdleSet(CZSealBody *seal, float x, float y, float z)
{
    if (seal->IsCurrentBlend((AnimType)1) || seal->IsCurrentBlend((AnimType)18) ||
        seal->IsCurrentBlend((AnimType)26)) {
        static float zero_z = 0.0f;
        static float zero_x = 0.0f;
        static float zero_y = 0.0f;

        if (!_close_enough(z, zero_z, data_003F3428) || !_close_enough(x, zero_x, data_003F3428) ||
            !_close_enough(y, zero_y, data_003F3420))
            func_00585FF0(seal);
    }
    func_00555170(&seal->m_blends);
}

/* a blend's state (an inline: through it the state is a temporary, numbered after every named local) */
static inline int StateOf(CZBodyAnimBlend *b)
{
    return b->m_state;
}

/* the anim after a step: with nothing on the blends nothing; asked to pop -- a dead SEAL's blend kept
 * in the air, else set dead with its fraction full -- the top blend popped, the climb-over that leaves
 * the SEAL above its altitude in the air turned into a fall (anim 9 in state 6, the root's live bit
 * and m_133c cleared), the state set from the popped blend's and the new top's, a blend without
 * flag 0x40 returned and the altitude kept from the node in the air; else the state from the top
 * blend's; then, the top blend flagged 0x40, the two heights reset and the state's own control run --
 * the idle state undoing the 0x44D3F8 set once a base stance's blend is up and a throttle moves --
 * and anything past the carried state popped down to one blend in state 0 (OVL2 0x5870E0; SOCOM 1's
 * SealAnim, the same body: SOCOM II's controls read the throttles at 0x23c-0x244, its dead blend's
 * test and the idle state's set new).
 * Byte-exact (round 47, the size first try): the registers by `tools/igreplay.py --want` -- the state a
 * temporary (StateOf), the popped blend a named local declared after it, uninitialised (written
 * `top = PopBlend` MW splits top's second range into a temporary coloured ahead of the state), the
 * idle state's throttles read into locals in the order x, z, y and passed to IdleSet */
CZBodyAnimBlend *CZSealBody::SealAnim(bool pop)
{
    CZBodyAnimBlend *top = (CZBodyAnimBlend *)func_00555170(&m_blends);

    if (!top)
        return 0;
    if (pop) {
        int state = StateOf(top);
        AnimType cur = m_curblend;
        CZBodyAnimBlend *b;

        if (state == stateDead && m_airborne)
            return top;
        if (state == stateDead) {
            BodySetState(stateDead);
            top->m_tfrac = 1.0f;
            return top;
        }
        b = func_00589AA0(this);
        if (!b)
            return 0;
        if (cur == AnimTypes::ClimbOver && m_airborne && !(m_posW.y <= m_alt)) {
            b = (CZBodyAnimBlend *)func_00588BC0(this, (AnimType)9, stateInAir, 0, 0);
            BodySetState(stateInAir);
            m_root->SetActive(false);
            m_133c = 0.0f;
        }
        if (b->m_state == statePeek)
            BodySetState(statePeek);
        else {
            BodySetState(state);
            if (state == stateInAir)
                BodySetState(b->m_state);
        }
        if (!func_005551A0(b->m_flags, 64))
            return b;
        if (m_state == stateInAir)
            m_alt = m_node->m_matrix.m_matrix[3][1];
    } else
        BodySetState(top->m_state);
    top = (CZBodyAnimBlend *)func_00555170(&m_blends);
    if (!func_005551A0(top->m_flags, 64))
        return top;
    m_f68 = -99.0f;
    m_f6c = -99.0f;
    switch (m_state) {
    case stateStand:
        top = InStateStand();
        break;
    case stateCrouch:
        top = InStateCrouch();
        break;
    case stateProne:
        top = InStateProne();
        break;
    case statePeek:
        break;
    case stateHang:
        top = InStateHang();
        break;
    case stateClimbLadder:
        top = InStateClimbLadder();
        break;
    case stateInAir:
        top = InAirControl();
        break;
    case stateIdle: {
        float x = m_240;
        float z = m_244;
        float y = m_23c;

        if (m_animset == data_0044D3F8)
            IdleSet(this, x, y, z);
        top = (CZBodyAnimBlend *)func_00555170(&m_blends);
        top->SetRepresentativeWt(1.0f);
        top->Representative()->SetPlayback(1.0f);
        break;
    }
    case stateDead:
        break;
    case stateCarried:
        top = InStateCarried();
        break;
    case stateCarrying:
        top = InStateCarrying();
        break;
    default: {
        CZBodyAnimBlend *b = (CZBodyAnimBlend *)func_00555170(&m_blends);
        int n = 12;
        CBodyAnimBlendStack *stack = &m_blends;

        while (BlendCount(stack) > 1 && n > 0) {
            b = func_00589AA0(this);
            n--;
        }
        if (n <= 0)
            b = (CZBodyAnimBlend *)func_00555170((char *)this + 0x1c0);
        else {
            if (b)
                b->m_ctrl = 0;
            m_desiredStance = STANCE_PRONE;
        }
        top = b;
        BodySetState(stateStand);
        break;
    }
    }
    return top;
}

/* in the air: the larger throttle's size, signed by the side throttle, as the top blend's playback
 * against its representative's script rate, its weight made 1 (SOCOM 1's InAirControl, no floats in
 * SOCOM II: SealAnim inlines it; then stripped -- src/ABSENT.txt) */
CZBodyAnimBlend *CZSealBody::InAirControl()
{
    float x = m_240;
    float z = m_244;
    float ax = fabsf(x);
    float az = fabsf(z);
    float s = ax > az ? ax : az;
    CZBodyAnimBlend *top = (CZBodyAnimBlend *)func_00555170(&m_blends);

    top->SetRepresentativeWt(1.0f);
    if (x < 0.0f)
        s = -s;
    ACTIVE_ANIM *rep = top->Representative();

    rep->SetPlayback(s * rep->m_anim->m_1c);
    return top;
}

/* the 0x44D3F8 set undone: when it is bound, the set kept at 0x530 bound again, the ZAnim at 0x538
 * let go and the weapon's timer cleared (OVL2 0x585FF0, new in SOCOM II: SealAnim and the stance
 * controls inline it, seal_env.cpp calls it -- the name unknown) */
extern "C" void func_00585FF0(CZSealBody *seal)
{
    if (seal->m_animset == data_0044D3F8) {
        seal->BindAnimSet(seal->m_530, false);
        if (seal->m_538)
            func_002707C0(seal->m_538, false);
        seal->m_538 = 0;
        CZWeapon::ClearTimer(0);
    }
}

/* a move's throttles split into two blend weights: a dead zone on the second, the first's share of
 * the length turned into an angle's fraction ((2 / pi) asin, kept within 0 and 1) and the other its
 * complement, the pair's length's inverse left in a static; the length returned, at most 1 (OVL2
 * 0x583350, new in SOCOM II: the crouched and standing move controls 0x582D10, 0x583030 ask it with
 * their throttles swapped -- `this` unread; the name ours) */
extern "C" float func_00583350(CZSealBody *seal, float *a, float *b, float x, float z)
{
    static float inv = 1.0f;

    if (fabsf(z) <= data_003F3428)
        z = 0.0f;
    float len = sqrtf(z * z + x * x);
    float ax = fabsf(x);

    *a = 0.0f;
    if (len > 0.0f)
        *a = ax / len;
    if (*a >= 1.0f)
        *a = 1.0f;
    else if (*a <= 0.0f)
        *a = 0.0f;
    else {
        *a = 0.63661975f * asinf(*a);
        if (*a > 1.0f)
            *a = 1.0f;
        else if (*a < 0.0f)
            *a = 0.0f;
    }
    *b = 1.0f - *a;
    if (len > 1.0f)
        len = 1.0f;
    inv = sqrtf(*a * *a + *b * *b);
    inv = 1.0f / inv;
    return len;
}

/* the stance's change taken, once (OVL2 0x582500; SOCOM 1's, the false return its own block) */
bool CZSealBody::StanceChanged()
{
    if (m_stancechanged) {
        m_stancechanged = false;
        return true;
    }
    return false;
}

extern "C" {
/* whether the AI's SEAL may be taken hostage, and the hostage state pushed when it may (seal_ai.cpp's
 * OVL2 0x555B80 and 0x555900: both take a third argument, which every caller passes as 0) */
bool func_00555B80(CSealCtrlAi *ai, int kind, int flag);
bool func_00555900(CSealCtrlAi *ai, int kind, int flag);
/* the restraint's capture (OVL2 0x5784A0, below); CPnt3D's implicit assignment, OVL2's out-of-line
 * copy (seal_env.cpp's note); a carried body's homing (seal_env.cpp's OVL2 0x5B1750) */
CZBodyAnimBlend *func_005784A0(CZSealBody *seal, CZSealBody *victim);
CPnt3D *func_00539E90(CPnt3D *to, const CPnt3D *from);
void func_005B1750(CZSealBody *seal, const CPnt3D *refpt, const CPnt3D *intpt, const CPnt3D *dir);
/* the stab's kill (OVL2 0x578D20, below); what a carried body's turret entity is told, with 1 (seal.cpp's
 * range, OVL2 0x545EE0); the death's weapon drop (seal_env.cpp's OVL2 0x5A0040) */
CZBodyAnimBlend *func_00578D20(CZSealBody *seal, CZSealBody *victim);
void func_00545EE0(CZSealBody *seal, int n);
void func_005A0040(CZSealBody *seal);
}

/* a blend's homing aimed (seal_env.cpp's stand-in over CHoming's layout, the same inline member: its
 * `this` in a saved register as the copies go through the assignment's out-of-line copy) */
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

/* a point turned round into another (seal_env.cpp's inline) */
static inline void NegateInto(const CPnt3D *in, CPnt3D *out)
{
    out->x = -in->x;
    out->y = -in->y;
    out->z = -in->z;
}

extern "C" {
/* the game's clock (OVL1 data, a float -- seal_ai.cpp's) */
extern float data_004365C0;
}

/* CrouchToPickupBuddy's timed call (OVL2 0x579E90, an anim callback: the body, then the buddy) --
 * the buddy picked up */
extern "C" void func_00579E90(void *body, void *buddy)
{
    ((CZSealBody *)body)->PickupBuddy((CZSealBody *)buddy);
}

/* a crouch to pick a buddy up: unless there is none or the SEAL moves faster than 8, the buddy's root
 * part taken into the world, its facing kept and then replaced by its spine's x axis turned round
 * through its matrix, flat; then, with anim 18 on top its timed call picks the buddy up at 3, in a
 * crouch (17 or 19, the top blend in state 1) at 0.99 -- from a crouched stance by 0x579DA0 -- with
 * the representative's bit 1 cleared; otherwise a crouch asked (the stance change marked) and its
 * blend left to pick up at 0.99 the same way; each the other's carry, the SEAL homed onto the pickup's
 * offset from the buddy's root along the turned axis, the controller zooming out, the buddy's AI given
 * bit 5; the top blend or the crouch's returned (OVL2 0x579A30 -- SOCOM 1's CrouchToPickupBuddy, after
 * PickupBuddy in both builds) */
CZBodyAnimBlend *CZSealBody::CrouchToPickupBuddy(CZSealBody *buddy)
{
    CZBodyAnimBlend *top = (CZBodyAnimBlend *)func_00555170(&m_blends);

    if (!buddy)
        return top;
    if (!(m_velM.x * m_velM.x + m_velM.y * m_velM.y + m_velM.z * m_velM.z <= 64.0f))
        return top;
    CPnt3D root;

    buddy->m_node->m_matrix.Transform(&buddy->GetRootPart()->m_pos, &root, 1);
    CPnt3D dir;

    dir = *(const CPnt3D *)buddy->m_node->m_matrix.m_matrix[2];
    CQuat q;

    buddy->PartInSpace(buddy->m_spinelo, 0, 0, &q);
    CPnt3D v;

    q.Apply(&CPnt3D::unit_x, &v, 1);
    v.y = 0.0f;
    v.Normalize();
    v.Scale(-1.0f, &v);
    TransformVectors(&buddy->m_node->m_matrix, &v, &dir, 1);
    if (IsCurrentBlend((AnimType)18)) {
        top->m_callback.m_func = func_00579E90;
        top->m_callback.m_tfrac = 3.0f;
        top->m_callback.m_userdata = buddy;
    } else if ((IsCurrentBlend((AnimType)17) || IsCurrentBlend((AnimType)19)) && top->m_state == 1) {
        if (GetStance(true) != 1) {
            top->m_callback.m_func = func_00579E90;
            top->m_callback.m_tfrac = 0.99f;
            top->m_callback.m_userdata = buddy;
        } else {
            top->m_callback.m_func = (void (*)(void *, void *))func_00579DA0;
            top->m_callback.m_tfrac = 0.99f;
            top->m_callback.m_userdata = buddy;
        }
        top->Representative()->SetBit1(false);
    } else {
        m_desiredState = stateCrouch;
        m_stancechanged = true;
        top = Crouch();
        top->m_callback.m_func = func_00579E90;
        top->m_callback.m_tfrac = 0.99f;
        top->m_callback.m_userdata = buddy;
        top->Representative()->SetBit1(false);
    }
    m_carry = buddy;
    buddy->m_carry = this;
    func_005B1750(this, &pickup_offset, &root, &dir);
    if (m_control)
        ((CSealCtrl *)m_control)->m_zoomout = true;
    CSealCtrlAi *ai = buddy->GetSealCtrlAi();

    if (ai)
        ai->m_aibit5 = true;
    return top;
}

/* a buddy picked up: unless there is none, its blends popped down to one, twelve at most, it carried
 * (10) with BuddyCarriedStand and BuddyCarriedPickup pushed, the carrier its carry, its root and hips
 * parts inactive, its root part parented to the carrier's, its node at the carrier's and its position
 * updated; the carrier's homing reset, its blends popped likewise, BuddyCarryStand and
 * BuddyCarryPickup pushed carrying (9), the buddy its carry, the body carrying, the zoom state 0; the
 * last push returned (OVL2 0x579720 -- SOCOM 1's PickupBuddy, after PutdownBuddy in both builds) */
CZBodyAnimBlend *CZSealBody::PickupBuddy(CZSealBody *buddy)
{
    if (!buddy)
        return (CZBodyAnimBlend *)func_00555170(&m_blends);
    CZBodyAnimBlend *b = (CZBodyAnimBlend *)func_00555170(&buddy->m_blends);
    int n = 12;
    CBodyAnimBlendStack *stack = &buddy->m_blends;

    while (BlendCount(stack) > 1 && n > 0) {
        b = func_00589AA0(buddy);
        n--;
    }
    if (n <= 0)
        b = (CZBodyAnimBlend *)func_00555170((char *)buddy + 0x1c0);
    else {
        if (b)
            b->m_ctrl = 0;
        buddy->m_desiredStance = STANCE_PRONE;
    }
    func_00588BC0(buddy, (AnimType)AnimTypes::BuddyCarriedStand, stateCarried, 0, 0);
    func_00588BC0(buddy, (AnimType)AnimTypes::BuddyCarriedPickup, stateCarried, 0, 0);
    buddy->BodySetState(stateCarried);
    buddy->m_carry = this;
    buddy->GetRootPart()->SetActive(false);
    buddy->m_hips->SetActive(false);
    buddy->GetRootPart()->m_parent = GetRootPart();
    buddy->GetNode()->SetPosition((const CPnt3D *)GetNode()->m_matrix.m_matrix[3]);
    buddy->UpdatePosW();
    ((CZBodyAnimBlend *)func_00555170(&m_blends))->m_homing.Reset();
    b = (CZBodyAnimBlend *)func_00555170(&m_blends);
    n = 12;
    stack = &m_blends;
    while (BlendCount(stack) > 1 && n > 0) {
        b = func_00589AA0(this);
        n--;
    }
    if (n <= 0)
        b = (CZBodyAnimBlend *)func_00555170((char *)this + 0x1c0);
    else {
        if (b)
            b->m_ctrl = 0;
        m_desiredStance = STANCE_PRONE;
    }
    func_00588BC0(this, (AnimType)AnimTypes::BuddyCarryStand, stateCarrying, 0, 0);
    CZBodyAnimBlend *r = (CZBodyAnimBlend *)func_00588BC0(this, (AnimType)AnimTypes::BuddyCarryPickup, stateCarrying, 0, 0);

    m_carry = buddy;
    BodySetState(stateCarrying);
    SetCurZoomState(ZOOMSTATE_3RDPERSON);
    return r;
}

/* the body carried put down: unless there is none, its blends popped down to one, twelve at most,
 * it carried (10) with BuddyCarryBody pushed dead and BuddyCarriedPickup carried, the carrier's
 * altitude hit given it (the point alone when that stands on a SEAL) and the clock kept; then the
 * carrier's blends popped likewise, it carrying (9) with anim 18 and BuddyCarryPickup pushed, the
 * last returned (OVL2 0x5794D0 -- SOCOM 1's PutdownBuddy, right after DetachBody in both builds, the
 * same pushes; SOCOM II limits the pops) */
CZBodyAnimBlend *CZSealBody::PutdownBuddy()
{
    CZSealBody *c = m_carry;
    CZBodyAnimBlend *r = 0;

    if (c) {
        CZBodyAnimBlend *b = (CZBodyAnimBlend *)func_00555170(&c->m_blends);
        int n = 12;
        CBodyAnimBlendStack *stack = &c->m_blends;

        while (BlendCount(stack) > 1 && n > 0) {
            b = func_00589AA0(c);
            n--;
        }
        if (n <= 0)
            b = (CZBodyAnimBlend *)func_00555170((char *)c + 0x1c0);
        else {
            if (b)
                b->m_ctrl = 0;
            c->m_desiredStance = STANCE_PRONE;
        }
        c->BodySetState(stateCarried);
        func_00588BC0(c, (AnimType)AnimTypes::BuddyCarryBody, stateDead, 0, 0);
        func_00588BC0(c, (AnimType)AnimTypes::BuddyCarriedPickup, stateCarried, 1, 0);
        const zdb::IntersectStruct *alt = &m_last_altitude;

        if (GetSeal(alt->m_node))
            c->m_last_altitude.m_pos = alt->m_pos;
        else
            c->m_last_altitude = *alt;
        c->m_420 = data_004365C0;
        b = (CZBodyAnimBlend *)func_00555170(&m_blends);
        n = 12;
        stack = &m_blends;
        while (BlendCount(stack) > 1 && n > 0) {
            b = func_00589AA0(this);
            n--;
        }
        if (n <= 0)
            b = (CZBodyAnimBlend *)func_00555170((char *)this + 0x1c0);
        else {
            if (b)
                b->m_ctrl = 0;
            m_desiredStance = STANCE_PRONE;
        }
        BodySetState(stateCarrying);
        func_00588BC0(this, (AnimType)18, stateCrouch, 0, 0);
        r = (CZBodyAnimBlend *)func_00588BC0(this, (AnimType)AnimTypes::BuddyCarryPickup, stateCrouch, 1, 0);
    }
    return r;
}

/* the body carried let go: the root part's place in the world turned by the node's rotation alone
 * and, level, added to the SEAL's position; the root part back at the origin but for its height,
 * unturned, its saved pose the same; the node given the carried body's matrix and the SEAL its turn,
 * the node set at the place found, the position updated, the root part oriented, unparented and the
 * parts saved; the carried body's altitude hit taken (its point alone when it stands on a SEAL), the
 * clock kept, bit 8 set, the carry cleared both ways (OVL2 0x579220; SOCOM 1's DetachBody, the order
 * changed). The carried body's matrix through CNode::GetMatrix (an inline argument, expanded ahead of
 * the call's own `this`: written `&...->m_matrix`, the SEAL's node is fetched first); its altitude
 * through a pointer local taken ahead of GetSeal, as the target keeps it in s1 */
void CZSealBody::DetachBody()
{
    CPnt3D pos;
    CQuat q;

    PartInSpace(m_root, 0, &pos, &q);
    CPnt3D w;

    TransformVectors(&m_node->m_matrix, &pos, &w, 1);
    w.y = 0.0f;
    w.Add(&m_posW, &w);
    pos.x = 0.0f;
    pos.z = 0.0f;
    m_root->m_pos = pos;
    m_root->m_saved_pos = pos;
    m_root->m_rot = CQuat::identity;
    m_root->m_saved_rot = m_root->m_rot;
    GetNode()->SetMatrix(m_carry->GetNode()->GetMatrix());
    m_quat = m_carry->m_quat;
    GetNode()->SetPosition(&w);
    UpdatePosW();
    m_root->Orient();
    m_root->m_parent = 0;
    SaveParts();
    const zdb::IntersectStruct *alt = &m_carry->m_last_altitude;

    if (GetSeal(alt->m_node))
        m_last_altitude.m_pos = alt->m_pos;
    else
        m_last_altitude = *alt;
    m_420 = data_004365C0;
    m_bit8 = true;
    if (m_carry) {
        m_carry->m_carry = 0;
        m_carry = 0;
    }
}

/* StabBuddy's timed call (OVL2 0x579210, an anim callback: the body, then the victim) -- the kill */
extern "C" void func_00579210(void *body, void *victim)
{
    func_00578D20((CZSealBody *)body, (CZSealBody *)victim);
}

/* a buddy stabbed, the attacker's half: as RestrainBuddy (above) -- unless there is no victim or a
 * carried body is already homed on; a victim working an entity told so with 1; unless the SEAL moves
 * faster than 10 -- the victim's root part taken into the world and its facing (its matrix's z row)
 * kept, the SEAL's blends popped down to one, the desired state standing; unless the top blend has
 * flag 64 anim 2 pushed with the kill (0x579210) to call at 3; each the other's carry, the SEAL homed
 * onto the stab's offset from the victim's root along the victim's facing; the controller zooming out,
 * the victim's AI given bit 5; the top blend returned (OVL2 0x578F90; SOCOM 1's StabBuddy killed at
 * once -- SOCOM II's starts it, 0x578D20 finishes it) */
CZBodyAnimBlend *CZSealBody::StabBuddy(CZSealBody *victim)
{
    bool busy;
    CZBodyAnimBlend *top = (CZBodyAnimBlend *)func_00555170(&m_blends);

    if (!victim)
        return top;
    if (m_carry && ((CZBodyAnimBlend *)func_00555170(&m_blends))->m_homing.m_blend)
        busy = true;
    else
        busy = false;
    if (busy)
        return top;
    if (victim->m_eb8)
        func_00545EE0(victim, 1);
    if (!(m_velM.x * m_velM.x + m_velM.y * m_velM.y + m_velM.z * m_velM.z <= 100.0f))
        return top;
    CPnt3D root;

    victim->m_node->m_matrix.Transform(&victim->GetRootPart()->m_pos, &root, 1);
    CPnt3D dir;

    dir = *(const CPnt3D *)victim->m_node->m_matrix.m_matrix[2];
    CZBodyAnimBlend *b = (CZBodyAnimBlend *)func_00555170(&m_blends);
    int n = 12;
    CBodyAnimBlendStack *stack = &m_blends;

    while (BlendCount(stack) > 1 && n > 0) {
        b = func_00589AA0(this);
        n--;
    }
    if (n <= 0)
        b = (CZBodyAnimBlend *)func_00555170((char *)this + 0x1c0);
    else {
        if (b)
            b->m_ctrl = 0;
        m_desiredStance = STANCE_PRONE;
    }
    m_desiredState = stateStand;
    CZBodyAnimBlend *r = b;

    if (func_005551A0(b->m_flags, 64)) {
        r = (CZBodyAnimBlend *)func_00588BC0(this, (AnimType)2, 0, 0, 0);
        r->m_callback.m_func = func_00579210;
        r->m_callback.m_tfrac = 3.0f;
        r->m_callback.m_userdata = victim;
    }
    m_carry = victim;
    victim->m_carry = this;
    func_005B1750(this, &stab_offset, &root, &dir);
    if (m_control)
        ((CSealCtrl *)m_control)->m_zoomout = true;
    CSealCtrlAi *ai = victim->GetSealCtrlAi();

    if (ai)
        ai->m_aibit5 = true;
    return r;
}

/* the stab's kill, when StabBuddy's timed call comes due: unless the victim is gone, a victim working
 * an entity told so with 1, its weapons dropped and its health zeroed by the knife; its blends popped
 * down to one, twelve at most, Stabbed pushed dead (8); each the other's carry, the victim's root part
 * parented to the attacker's, its node at the attacker's, its position updated and its root part
 * inactive; the attacker's homing reset, its blends cleared to the base and Stab pushed, which is
 * returned, the body in state 0 (OVL2 0x578D20, new in SOCOM II: SOCOM 1's StabBuddy did this at once
 * -- the name ours) */
extern "C" CZBodyAnimBlend *func_00578D20(CZSealBody *seal, CZSealBody *victim)
{
    if (!victim)
        return (CZBodyAnimBlend *)func_00555170(&seal->m_blends);
    if (victim->m_eb8)
        func_00545EE0(victim, 1);
    func_005A0040(victim);
    victim->SetHealth(0.0f, DEATH_KNIFE_KILL);
    CZBodyAnimBlend *b = (CZBodyAnimBlend *)func_00555170(&victim->m_blends);
    int n = 12;
    CBodyAnimBlendStack *stack = &victim->m_blends;

    while (BlendCount(stack) > 1 && n > 0) {
        b = func_00589AA0(victim);
        n--;
    }
    if (n <= 0)
        b = (CZBodyAnimBlend *)func_00555170((char *)victim + 0x1c0);
    else {
        if (b)
            b->m_ctrl = 0;
        victim->m_desiredStance = STANCE_PRONE;
    }
    func_00588BC0(victim, (AnimType)AnimTypes::Stabbed, stateDead, 0, 0);
    victim->m_carry = seal;
    seal->m_carry = victim;
    victim->GetRootPart()->m_parent = seal->GetRootPart();
    victim->GetNode()->SetPosition((const CPnt3D *)seal->GetNode()->m_matrix.m_matrix[3]);
    victim->UpdatePosW();
    victim->GetRootPart()->SetActive(false);
    ((CZBodyAnimBlend *)func_00555170(&seal->m_blends))->m_homing.Reset();
    seal->ClearToBase(false);
    CZBodyAnimBlend *r = (CZBodyAnimBlend *)func_00588BC0(seal, (AnimType)AnimTypes::Stab, 0, 0, 0);

    seal->BodySetState(0);
    return r;
}

/* RestrainBuddy's timed call (OVL2 0x578D10, an anim callback: the body, then the victim it was
 * handed) -- the capture made */
extern "C" void func_00578D10(void *body, void *victim)
{
    func_005784A0((CZSealBody *)body, (CZSealBody *)victim);
}

/* a buddy restrained, the captor's half: unless there is no victim, a carried body is already homed
 * on, or the SEAL moves faster than 10, the victim's root part taken into the world, the SEAL's blends
 * popped down to one, twelve at most, as 0x57F310 does, the desired state standing; unless the top
 * blend has flag 64 anim 2 pushed with the capture (0x578D10) to call at 3; each the other's carry,
 * the SEAL homed onto the restraint's offset from the victim's root along the turned difference, the
 * victim's top blend homed back along the flat reverse of it; the controller zooming out, the victim's
 * AI given bit 5; the top blend returned (OVL2 0x578990; SOCOM 1's RestrainBuddy did the capture at
 * once -- SOCOM II's starts it, 0x5784A0 finishes it). The registers, read off MW's colouring (igdump):
 * every saved-register variable has 19 real neighbours, so a variable with six deferred neighbours sits
 * at K; `busy` declared first (numbered before `top`) and set by if/else is removed in the first sweep
 * before `top` is reached, which then drops under K -- with `top` out of the deferred set, the SEAL
 * and the victim leave first in the second sweep and take s5/s4 as the target's. An inline bool or a
 * `busy = a && b` keeps `top` at K (59 off); the loop's blend and the returned one are two variables
 * (the loop's stays in v0, the result s3) */
CZBodyAnimBlend *CZSealBody::RestrainBuddy(CZSealBody *victim)
{
    bool busy;
    CZBodyAnimBlend *top = (CZBodyAnimBlend *)func_00555170(&m_blends);

    if (!victim)
        return top;
    if (m_carry && ((CZBodyAnimBlend *)func_00555170(&m_blends))->m_homing.m_blend)
        busy = true;
    else
        busy = false;
    if (busy)
        return top;
    if (!(m_velM.x * m_velM.x + m_velM.y * m_velM.y + m_velM.z * m_velM.z <= 100.0f))
        return top;
    CPnt3D root;

    victim->m_node->m_matrix.Transform(&victim->GetRootPart()->m_pos, &root, 1);
    CZBodyAnimBlend *b = (CZBodyAnimBlend *)func_00555170(&m_blends);
    int n = 12;
    CBodyAnimBlendStack *stack = &m_blends;

    while (BlendCount(stack) > 1 && n > 0) {
        b = func_00589AA0(this);
        n--;
    }
    if (n <= 0)
        b = (CZBodyAnimBlend *)func_00555170((char *)this + 0x1c0);
    else {
        if (b)
            b->m_ctrl = 0;
        m_desiredStance = STANCE_PRONE;
    }
    m_desiredState = stateStand;
    CZBodyAnimBlend *r = b;

    if (func_005551A0(b->m_flags, 64)) {
        r = (CZBodyAnimBlend *)func_00588BC0(this, (AnimType)2, 0, 0, 0);
        r->m_callback.m_func = func_00578D10;
        r->m_callback.m_tfrac = 3.0f;
        r->m_callback.m_userdata = victim;
    }
    m_carry = victim;
    victim->m_carry = this;
    CPnt3D d;

    m_posW.Sub(&root, &d);
    CQuat q;

    q.MakeYRot(0.7853982f);
    CPnt3D t;

    q.Apply(&d, &t, 1);
    d = t;
    func_005B1750(this, &restrain_offset, &root, &d);
    CZBodyAnimBlend *vb = (CZBodyAnimBlend *)func_00555170(&victim->m_blends);
    CPnt3D nrm(d);

    nrm.y = 0.0f;
    nrm.Normalize();
    NegateInto(&nrm, &nrm);
    HomingOf(vb)->Aim(vb, &CPnt3D::zero, &nrm, &root);
    vb->m_homing.m_target_refpt_m = CPnt3D::zero;
    vb->m_homing.m_2c = 1;
    if (m_control)
        ((CSealCtrl *)m_control)->m_zoomout = true;
    CSealCtrlAi *ai = victim->GetSealCtrlAi();

    if (ai)
        ai->m_aibit5 = true;
    return r;
}

/* the restraint taken hold of, when RestrainBuddy's timed call (0x578D10, three seconds into the
 * captor's anim) comes due: unless the victim is gone or its AI may not be taken hostage, the
 * "Restrained anim set" bound to the victim and kept; its AI held (bit 6) and, the hostage state
 * pushed, given the captor; the victim's blends popped down to one, twelve at most, as 0x57F310
 * does, anim 26 and ReceiveRestraints pushed in state 2; each SEAL the other's carry, the victim
 * restrained and no longer restrainable, its root part parented to the captor's, its node at the
 * captor's, its position updated and its root part inactive; the captor's homing reset, its blends
 * cleared to the base and ApplyRestraints pushed, which is returned, the body in state 0 (OVL2
 * 0x5784A0, new in SOCOM II: SOCOM 1's RestrainBuddy did all this at once, SOCOM II's 0x578990 starts
 * it -- the name ours) */
extern "C" CZBodyAnimBlend *func_005784A0(CZSealBody *seal, CZSealBody *victim)
{
    if (!victim)
        return (CZBodyAnimBlend *)func_00555170(&seal->m_blends);
    CSealCtrlAi *ai = victim->GetSealCtrlAi();

    if (!ai || !func_00555B80(ai, 3, 0))
        return (CZBodyAnimBlend *)func_00555170(&seal->m_blends);
    AnimSet *set = AnimSet::Find("Restrained anim set");

    if (set) {
        victim->BindAnimSet(set, false);
        victim->m_530 = set;
    }
    if (ai) {
        ai->m_aibit6 = true;
        if (func_00555900(ai, 3, 0)) {
            CAiState *s = ai->GetAiState();
            CAiSHostage *h = s && s->m_state == AI_STATE_HOSTAGE ? (CAiSHostage *)s : 0;

            h->SetCaptor(seal, true);
        }
    }
    CZBodyAnimBlend *b = (CZBodyAnimBlend *)func_00555170(&victim->m_blends);
    int n = 12;
    CBodyAnimBlendStack *stack = &victim->m_blends;

    while (BlendCount(stack) > 1 && n > 0) {
        b = func_00589AA0(victim);
        n--;
    }
    if (n <= 0)
        b = (CZBodyAnimBlend *)func_00555170((char *)victim + 0x1c0);
    else {
        if (b)
            b->m_ctrl = 0;
        victim->m_desiredStance = STANCE_PRONE;
    }
    func_00588BC0(victim, (AnimType)26, 2, 0, 0);
    func_00588BC0(victim, (AnimType)AnimTypes::ReceiveRestraints, 2, 0, 0);
    victim->m_carry = seal;
    seal->m_carry = victim;
    victim->SetRestrained(true);
    victim->SetRestrainable(false);
    victim->GetRootPart()->m_parent = seal->GetRootPart();
    victim->GetNode()->SetPosition((const CPnt3D *)seal->GetNode()->m_matrix.m_matrix[3]);
    victim->UpdatePosW();
    victim->GetRootPart()->SetActive(false);
    ((CZBodyAnimBlend *)func_00555170(&seal->m_blends))->m_homing.Reset();
    seal->ClearToBase(false);
    CZBodyAnimBlend *r = (CZBodyAnimBlend *)func_00588BC0(seal, (AnimType)AnimTypes::ApplyRestraints, 0, 0, 0);

    seal->BodySetState(0);
    return r;
}

/* the object the top blend holds let go: the part's node taken off its parent and given to the
 * world -- set down at the part's place in the world, at the SEAL's height, turned a quarter about x
 * -- or to the part the blend names; the node active and marked moved, the blend's hold and the
 * part's node and parent cleared; then the rifle (the node was the "rifle") or the pistol put back
 * under its own parent's node, and as gear unless that parent is the right hand (OVL2 0x5781C0, new
 * in SOCOM II: the deaths, the destructor, the mission's end and the item changes call it; the name
 * unknown. seal_body.cpp's last-defined function: the unit's first in the image). The locals
 * declared at the top in this order give the target's saved registers (the flag s4, the new parent
 * s3, the blend s2, the node s1, the part s0); declared where they are first set they number
 * blend, part, node, flag, parent */
extern "C" void func_005781C0(CZSealBody *seal)
{
    bool rifle;
    zdb::CNode *parent;
    CZBodyAnimBlend *b;
    zdb::CNode *node;
    CZBodyPart *part;

    b = (CZBodyAnimBlend *)func_00555170(&seal->m_blends);
    if (!b)
        return;
    part = (CZBodyPart *)b->m_20;
    if (!part)
        return;
    node = part->m_node;
    rifle = !strcmp(node->m_name, "rifle");
    if (!node->m_parent)
        return;
    node->m_parent->DeleteChild(node);
    if (!b->m_24) {
        CPnt3D p;
        CPnt3D w;

        seal->PartInSpace(part, 0, &p, 0);
        seal->m_quat.Apply(&p, &w, 1);
        w.Add(&seal->m_posW, &w);
        w.y = seal->m_posW.y;
        parent = zdb::CWorld::m_world;
        node->SetPosition(&w);
        CQuat q;

        q.MakeXRot(1.5707964f);
        /* (the pointer converted: MW keeps the inline's null test, as the target does) */
        node->SetRotation((const CQuat *)&q);
    } else
        parent = ((CZBodyPart *)b->m_24)->m_node;
    node->SetActive(true);
    parent->AddChild(node);
    node->m_socom2_a1 |= 2;
    b->m_20 = 0;
    part->m_node = 0;
    part->m_parent = 0;
    if (rifle) {
        if (seal->m_rifle->m_node && seal->m_rifle->m_node->m_parent)
            seal->m_rifle->m_node->m_parent->DeleteChild(seal->m_rifle->m_node);
        if (seal->m_rifle->m_parent && seal->m_rifle->m_parent->m_node) {
            seal->m_rifle->m_parent->m_node->DeleteChild(seal->m_rifle->m_node);
            seal->m_rifle->m_parent->m_node->AddChild(seal->m_rifle->m_node);
        }
        if (seal->m_rifle->m_parent != seal->m_rhand)
            seal->SetPartAsGear(seal->m_item_gear[1].m_part, seal->m_item_gear[1].m_gear);
    } else {
        if (seal->m_pistol->m_node && seal->m_pistol->m_node->m_parent)
            seal->m_pistol->m_node->m_parent->DeleteChild(seal->m_pistol->m_node);
        if (seal->m_pistol->m_parent && seal->m_pistol->m_parent->m_node) {
            seal->m_pistol->m_parent->m_node->DeleteChild(seal->m_pistol->m_node);
            seal->m_pistol->m_parent->m_node->AddChild(seal->m_pistol->m_node);
        }
        if (seal->m_pistol->m_parent != seal->m_rhand)
            seal->SetPartAsGear(seal->m_item_gear[2].m_part, seal->m_item_gear[2].m_gear);
    }
}
