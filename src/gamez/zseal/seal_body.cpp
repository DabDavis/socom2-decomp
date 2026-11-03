/* seal_body.cpp -- the SEAL's body: its actions, stances and motion (SOCOM 1's unit,
 * __sinit_seal_body.cpp; its strings put Action, RestrainBuddy, OKToStand and StanceChanged
 * there). OVL2 from about 0x578990 (RestrainBuddy) to seal_cfg.cpp. Defined in reverse of their
 * image order (-inline deferred). */

#include "zseal.h"

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

/* the state of the topmost blend not in state 3, else 0 (OVL2 0x57DC20; new in SOCOM II, its name
 * unknown -- seal_env.cpp's 0x5A82E0 and the weapon unit ask it) */
extern "C" unsigned char func_0057DC20(CZSealBody *seal)
{
    CBodyAnimBlendStack *stack = &seal->m_blends;

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
extern "C" void func_0058A5E0(CZSealBody *seal, unsigned int count, bool prone)
{
    CZBodyAnimBlend *b = (CZBodyAnimBlend *)func_00555170(&seal->m_blends);
    int n = 12;
    CBodyAnimBlendStack *stack = &seal->m_blends;

    while (BlendCount(stack) > count && n > 0) {
        b = func_00589AA0(seal);
        n--;
    }
    if (n <= 0)
        b = (CZBodyAnimBlend *)func_00555170((char *)seal + 0x1c0);
    else if (prone) {
        if (b)
            b->m_ctrl = 0;
        seal->m_desiredStance = STANCE_PRONE;
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
/* (OVL2 0x58A4D0 and seal.cpp's 0x555170: what the SEAL holds, on a ladder or off it) */
void *func_0058A4D0(CZSealBody *seal, int a);
void *func_00555170(void *v);
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
    if (seal->m_inwater && !(seal->m_f88 <= 8.5f))
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

/* a table of three-word entries with three words of state (the names ours) */
struct CTripleTable {
    unsigned int m_count;   // 0x0
    int m_4;   // 0x4
    int m_8;   // 0x8
    int m_c;   // 0xc
    struct Entry {
        int m_0, m_4, m_8;
    } *m_items;   // 0x10
};

/* the table cleared: its state zeroed, every entry zeroed (OVL2 0x58A950, new in SOCOM II; three of
 * the body's functions call it) */
extern "C" void func_0058A950(CTripleTable *t)
{
    t->m_4 = 0;
    t->m_8 = 0;
    t->m_c = 0;
    CTripleTable::Entry *e = t->m_items;

    for (unsigned int i = 0; i < t->m_count; i++, e++) {
        e->m_0 = 0;
        e->m_4 = 0;
        e->m_8 = 0;
    }
}

/* (OVL2 0x57DC80; CSealCtrl::PreTick calls it twice) */
extern "C" void *func_0057DC80(CZSealBody *seal)
{
    if (seal->m_state == 3)
        return func_0058A4D0(seal, 0);
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

/* the stance's change taken, once (OVL2 0x582500; SOCOM 1's, the false return its own block) */
bool CZSealBody::StanceChanged()
{
    if (m_stancechanged) {
        m_stancechanged = false;
        return true;
    }
    return false;
}
