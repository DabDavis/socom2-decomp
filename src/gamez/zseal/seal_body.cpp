/* seal_body.cpp -- the SEAL's body: its actions, stances and motion (SOCOM 1's unit,
 * __sinit_seal_body.cpp; its strings put Action, RestrainBuddy, OKToStand and StanceChanged
 * there). OVL2 from about 0x578990 (RestrainBuddy) to seal_cfg.cpp. Defined in reverse of their
 * image order (-inline deferred). */

#include "zseal.h"

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
 * stack through a reference, the members straight off the SEAL, the body as a CBody. */
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
/* (OVL2 0x5E3C20, anim.cpp's, not written yet: an anim id's script for a weapon, or none) */
CZSIScriptCommon *func_005E3C20(AnimSet *set, short id, short weapon);
void *func_00555170(void *v);
}

/* the weapon in hand set and every blend's anim looked up again for it -- the weapon's own, else
 * weapon 0's -- the parts' poses saved, and both interpolations set to the top anim's blend-in time
 * (OVL2 0x57D710, new in SOCOM II: seal_cfg.cpp's 0x58C5D0 calls it; the name unknown).
 * OPEN, 0xE4 against 0xF8: the body is the target's (the BlendAt walk keeps its per-pass sll/addu);
 * left, the loop's entry -- the target jumps to a test block of its own placed after the return
 * (`bltz i` to the exit, `b` to the bottom `bgez`), where MW tests at the top -- and the SEAL and the
 * stack trading s1/s2. Tried: the stack declared before the store, a while loop with the counter
 * declared first, `if (i >= 0) do ... while (--i >= 0)`, the stack through a reference, a goto
 * into the bottom test (all 0xE4). */
extern "C" bool func_0057D710(CZSealBody *seal, short weapon)
{
    seal->m_weapon_id = weapon;
    CBodyAnimBlendStack *stack = &seal->m_blends;

    for (int i = stack->m_top; i >= 0; i--) {
        ACTIVE_ANIM *rep = BlendAt(stack, i)->Representative();
        short id = rep->m_id;
        CZSIScriptCommon *anim = func_005E3C20(seal->m_animset, id, seal->m_weapon_id);

        if (!anim)
            anim = func_005E3C20(seal->m_animset, id, 0);
        if (anim)
            rep->m_anim = anim;
    }
    seal->SaveParts();
    float t = ((CZBodyAnimBlend *)func_00555170((char *)seal + 0x1c0))->Representative()->m_anim->m_20;

    seal->m_interp2 = seal->m_interp1 = t;
    return true;
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
 * crouch bit new; seal_ai.cpp's InStance and the kit's kick ask it) */
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

/* how fast the SEAL moves, standing, crouched or prone: 1 still, 2 under 20 units a second, 3 faster
 * (1 in any other state; OVL2 0x58A820, SOCOM 1's GetSpeedState after GetStance, the speeds squared
 * -- Tick asks it) */
int CZSealBody::GetSpeedState()
{
    float v = m_velM.x * m_velM.x + m_velM.y * m_velM.y + m_velM.z * m_velM.z;

    switch (m_state) {
    case 0:
    case 1:
    case 2:
        if (v < 0.25f)
            return 1;
        return v < 400.0f ? 2 : 3;
    }
    return 1;
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
/* an anim the SEAL could take, its type, a fraction and a base time, a point (OVL2 0x57FCE0, not
 * written yet), a move begun one way or the other (OVL2 0x5805B0, not written yet) and two anim types
 * read as variables (OVL1 data, -1 in the image) */
bool func_0057FCE0(CZSealBody *seal, AnimType *type, float *frac, float *base, float *p);
void func_005805B0(CZSealBody *seal, bool other, int a);
extern short data_003DEED0;
extern short data_003DEED8;
}

/* the body's state (an inline: expanded ahead of the call it is passed to, so it loads first) */
static inline int StateOf(CZSealBody *seal)
{
    return seal->m_state;
}

/* the anim 0x57FCE0 picks pushed, and the time into it its fraction makes, past its base (OVL2
 * 0x5802B0, new in SOCOM II) */
extern "C" bool func_005802B0(CZSealBody *seal, float *out)
{
    AnimType type;
    float frac = 0.0f;
    float base = 0.0f;
    float p[3];
    bool ok = false;

    if (func_0057FCE0(seal, &type, &frac, &base, p)) {
        CZBodyAnimBlend *b = (CZBodyAnimBlend *)func_00588BC0(seal, type, StateOf(seal), 0, 0);

        *out = base + frac * b->Representative()->m_anim->LoopTime();
        ok = true;
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

extern "C" {
/* another anim type read as a variable (OVL1 data, -1 in the image) */
extern short data_003DECA0;
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
 * nothing or lose more */
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

extern "C" {
/* an anim set's script for a type (OVL2 0x5E3D90, anim.cpp's: 0x5E3C20 with no weapon) */
CZSIScriptCommon *func_005E3D90(AnimSet *set, AnimType type);
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
        ACTIVE_ANIM *a = m_active_anim_pool.GetActiveAnim(func_005E3D90(m_animset, (AnimType)42), -1);

        a->m_id = 42;
        PushAnim(&newanims, a, w);
    }
    if (w < 1.0f) {
        ACTIVE_ANIM *a = m_active_anim_pool.GetActiveAnim(func_005E3D90(m_animset, (AnimType)AnimTypes::ClimbMedium), -1);

        a->m_id = AnimTypes::ClimbMedium;
        PushAnim(&newanims, a, 1.0f - w);
    }
    b->SetBlendAnims(this, &newanims);
    return b;
}

extern "C" {
/* (OVL2 0x5AC1F0 and 0x5A0950, seal_env.cpp's, not written yet: a fall's damage taken, and an anim
 * type chosen by a kind and a variant) */
void func_005AC1F0(CZSealBody *seal, float damage);
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
    func_005AC1F0(seal, 1.2f * t);
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

/* out of the anim of 0x3DECA0: the blends popped down to one, twelve at most, as 0x57F310 does, and
 * that anim pushed; in it, only the top blend fetched (OVL2 0x580A80, new in SOCOM II) */
extern "C" void func_00580A80(CZSealBody *seal)
{
    if (!seal->IsCurrentBlend((AnimType)data_003DECA0)) {
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
        func_00588BC0(seal, (AnimType)data_003DECA0, 0, 0, 0);
    } else
        func_00555170((char *)seal + 0x1c0);
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
/* an animation set the SEALs share (OVL1 bss 0x44D408) and a set's anim of a type, negative for none
 * (OVL2 0x5E3E80, the kit lane's range) */
extern AnimSet *data_0044D408;
int func_005E3E80(AnimSet *set, int type);
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

extern "C" {
/* (OVL2 0x579A30, not written yet: what 0x579DA0 hands on to) */
void func_00579A30(CZSealBody *seal, int a);
}

/* for a SEAL: the blends popped down to two, twelve at most -- when that worked the top blend's
 * control cleared and the SEAL asked to go prone, as 0x57F310 does -- its movement velocity zeroed,
 * then 0x579A30 (OVL2 0x579DA0, new in SOCOM II: 0x5880E0 calls it; the name unknown). The stack's
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
        func_00579A30(seal, a);
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
    if (func_005E3E80(SetOf(seal), 18) < 0)
        return false;
    if (func_005E3E80(SetOf(seal), 84) < 0)
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
/* an anim type read as a variable (OVL1 data, -1 in the image) and AnimSet's variant pick and the
 * script of a variant (OVL2 0x5E3DA0, GetVariantIndex__7AnimSetF8AnimTypeii, and 0x5E3B20: anim.cpp's,
 * the kit lane's -- AnimSet is declared there alone) */
extern short data_003DF130;
int func_005E3DA0(AnimSet *set, AnimType type, int kind, int anim);
CZSIScriptCommon *func_005E3B20(AnimSet *set, AnimType type, int kind, int variant);
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
        variant = func_005E3DA0(seal->m_animset, (AnimType)data_003DF130, seal->m_weapon_id,
                                rep->m_anim->m_id);
    rep->m_anim = func_005E3B20(seal->m_animset, (AnimType)data_003DF130, seal->m_weapon_id, variant);
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

/* CBody::BodySetState (OVL1 0x28DFB0, the body lane's: the state stored) */
extern "C" void func_0028DFB0(CBody &body, int state);

/* the state set: the controller told of a change first (OVL2 0x58A6B0; SOCOM 1's, virtual in SOCOM
 * II -- CBody's slot 0x14, CZSealBody's 0x90) */
void CZSealBody::BodySetState(int state)
{
    if (m_control && state != m_state)
        ((CSealCtrl *)m_control)->OnStateChange((SEAL_STATE)m_state, (SEAL_STATE)state);
    func_0028DFB0(*this, state);
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

/* the stance's change taken, once (OVL2 0x582500; SOCOM 1's, the false return its own block) */
bool CZSealBody::StanceChanged()
{
    if (m_stancechanged) {
        m_stancechanged = false;
        return true;
    }
    return false;
}
