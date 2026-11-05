/* seal_anim.cpp -- the SEAL animated: its blends pushed and popped, its root and vertical, its
 * adrenaline (SOCOM 1's unit, __sinit_seal_anim.cpp: Post, GetRootPos, SetVertical, the CSealAnim
 * blender, Animate, AdrenalineIncr and Adrenaline last). OVL2 0x576120 (the blender deleted: StopEngine
 * calls it where SOCOM 1's calls ~CSealAnim -- the rows before it are seal_ai.cpp's template tail, the
 * animate state's list instances) to 0x5781BF (Adrenaline), before seal_body.cpp. Defined in reverse
 * of their image order (-inline deferred). */

#include "zseal.h"

/* the animation set (anim.cpp's, the kit lane's, which declares the whole class): only AnimSet::BaseAnim,
 * an anim type's base anim or -1 (SOCOM 1's BaseAnim__7AnimSetF8AnimType, called here where SOCOM 1's
 * IsCurrentBlend calls it; OVL2 0x5E1A90) */
class AnimSet {
public:
    AnimType BaseAnim(AnimType type);
    /* a type's motion script with no weapon (anim.cpp's, OVL2 0x5E3D90) */
    CZSIScriptCommon *GetScript(AnimType type);
};

/* a part's pose as the blender keeps it (SOCOM 1's records, 0x20 bytes: a position, the part's id
 * -- below zero for none -- and a rotation; the names ours). SOCOM II's halfword at 0xe: AnimPush sets
 * it on every record, AnimOverlay lays a position over only where one of the two lacks it -- the
 * position's fourth word, which VU0's XYZ interpolation writes, so AnimOverlay keeps the word across
 * the call */
struct CSealAnimRec {
    CPnt3D m_pos;   // 0x0
    short m_id;   // 0xc
    short m_nopos;   // 0xe
    CQuat m_rot;   // 0x10
};

extern "C" {
/* v0 = v1 * r + v2 * (1 - r), the x, y and z (Sony's libvu0, MAIN 0x1C0768: a quadword written) */
void sceVu0InterVectorXYZ(sceVu0FVECTOR v0, sceVu0FVECTOR v1, sceVu0FVECTOR v2, float r);
}

/* a frame on the blender's stack: the parts' records and the anim pushed (0xc bytes; the names ours) */
struct CSealAnimFrame {
    float m_wt;   // 0x0: the frame's weight (CombineRotations sums two)
    CSealAnimRec *m_recs;   // 0x4
    CZSIScriptCommon *m_anim;   // 0x8
};

/* the blender's eight frames and the one block their records come from, n records a frame (0x64
 * bytes; new in SOCOM II, where SOCOM 1's frames held 32 records each; the names ours) */
struct CSealAnimFrames {
    /* the block allocated and zeroed, the frames zeroed, each frame given its n records */
    CSealAnimFrames(unsigned int n)
    {
        m_pool = (CSealAnimRec *)_malloc(n << 8, 0, 0);
        memset(m_pool, 0, n << 8);
        memset(this, 0, sizeof(m_frames));
        CSealAnimRec *p = m_pool;
        for (int i = 0; i < 8; i++) {
            m_frames[i].m_recs = p;
            p += n;
        }
    }
    /* the block freed and the frames zeroed (the blender's destructor calls it; nothing frees the
     * frames themselves) */
    void Free()
    {
        _free(m_pool, 0, 0);
        m_pool = 0;
        memset(this, 0, sizeof(m_frames));
    }

    CSealAnimFrame m_frames[8];   // 0x0
    CSealAnimRec *m_pool;   // 0x60
};

/* the number of records a frame holds is zbody.h's siNumBodyObjects (OVL1 data 0x3DE1E8; SOCOM 1's 32
 * were a constant) -- unsigned: as an `int` the blender's constructor comes out three instructions
 * longer (0x134 against 0x128) */

/* the SEAL's animation blender (SOCOM 1's CSealAnim: a stack of frames of part records -- SOCOM II's
 * frames point at their records; only what the written members touch) and the one the unit uses (OVL1
 * bss 0x44CDE0, a pointer) */
class CSealAnim {
public:
    /* empty, its frames built and every record marked unused (SOCOM 1's CSealAnim(), out of line
     * there) */
    CSealAnim() : m_count(0), m_seal(0)
    {
        m_frames = new CSealAnimFrames(siNumBodyObjects);
        CSealAnimRec *r = m_frames->m_pool;
        for (unsigned int i = 0; i < siNumBodyObjects * 8; i++, r++)
            r->m_id = -1;
    }
    /* emptied, the frames' block freed (SOCOM 1's ~CSealAnim, out of line there) */
    ~CSealAnim()
    {
        m_count = 0;
        m_seal = 0;
        if (m_frames)
            m_frames->Free();
        m_frames = 0;
    }
    /* emptied and given a SEAL (an inline: both stores through one load of the global) */
    void Begin(CZSealBody *seal)
    {
        m_count = 0;
        m_seal = seal;
    }
    /* an anim's pose at its frame and fraction pushed at a weight (SOCOM 1's took the SEAL and the
     * active anim; OVL2 0x5777D0, not written yet), and the top frame's records put back on the
     * SEAL's parts, the depth left returned (SOCOM 1's took the body; OVL2 0x577700). The frame an
     * `int` (round 34: 0x577DF0's call evaluates the short field's conversion first) */
    void AnimPush(CZSIScriptCommon *anim, float ffrac, float wt, int frame);
    int AnimPop();
    /* frame a's rotations moved toward frame b's by b's share of their weights, a given both (SOCOM
     * 1's; OVL2 0x576E30) */
    int CombineRotations(int a, int b);
    /* the top frame laid over the one below it at a fraction and popped (SOCOM 1's; OVL2 0x576BB0) */
    int AnimOverlay(float t);
    /* the top count frames blended into one (SOCOM 1's AnimBlend, between CombineRotations and AnimPop in
     * both builds; OVL2 0x577000, not written yet) */
    void AnimBlend(int count);

    int m_count;   // 0x0
    CZSealBody *m_seal;   // 0x4
    CSealAnimFrames *m_frames;   // 0x8
};

extern "C" {
extern CSealAnim *data_0044CDE0;
}

/* a SEAL posed by one anim alone: the blender emptied and given the SEAL, the anim pushed, the pose
 * popped onto the parts (OVL2 0x577DF0, new in SOCOM II: seal_cfg.cpp's 0x58C600 calls it; the name
 * unknown). The two stores through one load of the global need the inline Begin (written out, MW
 * reloads the global between them); the target loads the frame (a2), the two floats, then m_anim into
 * a1 last, off a1 itself -- AnimPush takes the frame as an `int` (round 34): an argument needing a
 * conversion is evaluated earlier, so the short field goes first; with a `short` (or `unsigned short`)
 * parameter MW loads m_anim first and copies the anim pointer to v1 (0x50 bytes). */
extern "C" void func_00577DF0(CZSealBody *seal, ACTIVE_ANIM *anim)
{
    data_0044CDE0->Begin(seal);
    data_0044CDE0->AnimPush(anim->m_anim, anim->m_ffrac, anim->m_wt, anim->m_frame);
    data_0044CDE0->AnimPop();
}

/* a quaternion's conjugate, in place (an inline: its y and z addressed off the quaternion) */
static inline void Conjugate(CQuat *q)
{
    q->vec.x = -q->vec.x;
    q->vec.y = -q->vec.y;
    q->vec.z = -q->vec.z;
}

/* the cross product through VU0, not normalised (CPnt3D::Cross's body, inline here) */
static inline void CrossVU0(const CPnt3D *a, const CPnt3D *b, CPnt3D *out)
{
    sceVu0FVECTOR r;
    sceVu0FVECTOR va;
    sceVu0FVECTOR vb;

    va[0] = a->x;
    va[1] = a->y;
    va[2] = a->z;
    va[3] = 1.0f;
    vb[0] = b->x;
    vb[1] = b->y;
    vb[2] = b->z;
    vb[3] = 1.0f;
    sceVu0OuterProduct(r, va, vb);
    out->x = r[0];
    out->y = r[1];
    out->z = r[2];
}

/* each part a script moves turned by a share of the turn between two of its frames: the turn
 * carrying -z to where the later frame faces, as an axis, scaled by half the share and exponentiated
 * (OVL2 0x577BA0, new in SOCOM II: 0x57A330 passes the head-look script, anim 82, frames 1 and 2 or
 * 1 and 0 by the look's sign; the name unknown). The code is the target's word for word (the stack:
 * dir copy-initialised where it is used -- declared first it is copied member by member, 0x18 long --
 * the cross product an inline over three VU0 vectors, the conjugate an inline whose y and z addresses
 * MW hoists into s7/fp); the frames `unsigned int`: their conversion to GetRot's int is a loop-invariant
 * temporary hoisted into the preheader, as the target's `move s0,a2` / `move s6,a3` there (as `int` the
 * parameters are copied in the prologue).
 * OPEN, 28 instructions: registers only. The target hoists `&seal->m_body_parts` into the preheader
 * (s1) and the replay (build/ig, tools/igreplay.py --movers) has exactly one numbering giving every
 * saved register: the parts' address numbered between part and the first hoisted conversion -- an
 * inlined function's variable or the first loop-optimiser temporary. Tried: the reference at the top
 * (numbered first: s4), inside the loop or the if (forwarded: `lw v0,468(seal)`, seal kept), assigned
 * there through an uninitialised pointer (forwarded, or the loop no longer rotated: 0x10 long), inline
 * helpers taking the vector, its address or the SEAL (forwarded), with the bounds test inside (a null
 * test more), begin() + id, a const view -- the operator[] whose `this` the original kept unforwarded
 * is not found. The SEALs' round 44, with igreplay's --target: `--move 36=40.5` (parts after part,
 * before @5370) scores 0 differing against the shipped code, so the numbering is certain; still
 * forwarded (`seal` kept in s5, parts gone): the vector's address assigned through an inline's
 * reference or pointer out-parameter (the game states' round-39 rule), with part given a dead first
 * definition at function or block scope and `i` declared ahead of it (round 12's), and a static
 * inline returning the address -- by CZSealBody * or CBody * (zbody.h's ActiveAnimPool shape, whose
 * result MW hoists, is a member; the parts' load base folds into `468(seal)` here). */
extern "C" void func_00577BA0(CZSealBody *seal, CZSIScriptCommon *script, unsigned int f0, unsigned int f1, float t)
{
    std::vector<CZBodyPart *> &parts = seal->m_body_parts;
    CZSIObject *obj = script->m_pObjects;

    for (unsigned int i = 0; i < script->m_nObjects; i++, obj++) {
        unsigned int id = obj->m_body_part_id;

        if (id < siNumBodyObjects) {
            CZBodyPart *part = parts[id];
            CQuat qd;
            CQuat q0;
            CQuat q1;

            obj->GetRot(f0, &q0);
            obj->GetRot(f1, &q1);
            Conjugate(&q0);
            q0.Mul(&q1, &qd);
            CPnt3D dir = CPnt3D::unit_nz;
            CPnt3D to;
            CPnt3D axis;
            qd.Apply(&dir, &to, 1);
            CrossVU0(&dir, &to, &axis);
            axis.Scale(0.5f * t, &axis);
            qd.Exp(&axis);
            part->m_rot.Mul(&qd, &q1);
            part->m_rot = q1;
        }
    }
}

/* THE RECORD LOOPS (the SEALs, round 34): every loop here over siNumBodyObjects records -- AnimPop,
 * CombineRotations, 0x576BB0, 0x577000 -- loads the count into a register ahead of the loop for its
 * bottom test and reads the global AGAIN for the entry test alone (`lw v0; sltu at,zero,v0; beqz`).
 * MW never hoists the global out of a loop with a store or a call in it (probes: extern, static, extern
 * const alike), and with the count in a local it folds the entry test onto that local (one load, 8
 * bytes short); `for` loops, inline helpers taking the count, a struct copy or a reference all do the
 * same. What gives the target's two loads is a count taken into a local, then a guard comparing the
 * counter with the global READ THROUGH A CONST VIEW -- which MW's CSE keeps apart from the plain read
 * (the kit's round 34 rule) -- around a do-while on the local. The original's spelling (an accessor
 * returning a const reference, a macro) is not known; this one is the target's, byte for byte. */

/* the top frame's records put back on the SEAL's parts, each with a part, its position and rotation
 * (OVL2 0x577700; SOCOM 1's AnimPop, after AnimBlend in both builds, its 32 records a count now) --
 * called out of line from 0x577DF0: pinned. The frame through a pointer local (indexed in place, MW
 * adds the base last: 9 off), the index `top` kept (inline, the -1 folds into the -12) and `> -1`
 * (slti) the target's; the counter declared before the count (the other order swaps a3/t0). */
#pragma auto_inline off
int CSealAnim::AnimPop()
{
    std::vector<CZBodyPart *> &parts = m_seal->m_body_parts;
    int top = m_count - 1;
    CSealAnimFrame *f = &m_frames->m_frames[top];
    CSealAnimRec *rec = f->m_recs;
    unsigned int i = 0;
    unsigned int n = siNumBodyObjects;

    if (i < *(const unsigned int *)&siNumBodyObjects) {
        do {
            if (rec->m_id > -1) {
                CZBodyPart *part = parts[rec->m_id];

                if (part) {
                    part->m_pos = rec->m_pos;
                    part->m_rot = rec->m_rot;
                }
            }
            i++;
            rec++;
        } while (i < n);
    }
    m_count--;
    return m_count;
}
#pragma auto_inline reset

/* frame a's rotations moved toward frame b's by b's share of the two weights -- a part b lacks
 * takes b's, near enough to either end it snaps -- and a given the sum (OVL2 0x576E30; SOCOM 1's
 * CombineRotations, the fraction clamped to [0, 1] in SOCOM II, the records a count now and the
 * rotation copies inline), the loop the record loops' shape (above AnimPop). The saved registers (a s5,
 * i s4, ra s3, rb s2, the count s1, frame a s0) want the counter, the two record cursors and the count
 * declared uninitialised at the top, ahead of the frames (MW numbers them there). */
int CSealAnim::CombineRotations(int a, int b)
{
    unsigned int i;
    CSealAnimRec *ra;
    CSealAnimRec *rb;
    unsigned int n;
    CSealAnimFrame *fa = &m_frames->m_frames[a];
    CSealAnimFrame *fb = &m_frames->m_frames[b];
    float sum = fa->m_wt + fb->m_wt;
    float frac = fb->m_wt / sum;

    if (frac < 0.0f)
        frac = 0.0f;
    if (frac > 1.0f)
        frac = 1.0f;
    rb = fb->m_recs;
    ra = fa->m_recs;
    n = siNumBodyObjects;
    i = 0;
    if (i < *(const unsigned int *)&siNumBodyObjects) {
        do {
            if (ra->m_id != -1) {
                if (rb->m_id != -1) {
                    if (frac > 0.001f) {
                        if (frac >= 0.999f)
                            ra->m_rot = rb->m_rot;
                        else
                            ra->m_rot.Slerp(&rb->m_rot, frac, &ra->m_rot);
                    }
                } else
                    ra->m_rot = rb->m_rot;
            }
            i++;
            ra++;
            rb++;
        } while (i < n);
    }
    fa->m_wt = sum;
    return a;
}

/* the top frame laid over the one below it at a fraction and popped: every part the top holds given to
 * the frame below, its position moved there by the fraction where either lacks one (VU0's XYZ
 * interpolation, the record's fourth word kept), then its rotation (Slerp); near enough to either end
 * each snaps (OVL2 0x576BB0: SOCOM 1's AnimOverlay by its place, before CombineRotations in both
 * builds -- SOCOM 1's weighted the positions with Scale and Add over 32 records), the depth left
 * returned. The record loops' shape (above AnimPop); `> -1` (slti) the target's tests.
 * Round 36, read off MW's colouring (igdump): the eight locals all wait for the last simplify sweep
 * and are coloured in descending declaration order, so the counter is declared first (s5) and the
 * count sixth (s1); the cursors are reassigned for the second loop, so each gets a second web, and
 * with rb's first-loop assignment written first ra's second web is numbered above rb's -- ra s2 and
 * rb s3 there, as the target (round 34's perm.py order had the counter and the count the other way
 * round, 22 off) */
int CSealAnim::AnimOverlay(float t)
{
    unsigned int i;
    int top;
    CSealAnimRec *rb;
    CSealAnimRec *ra;
    float r;
    unsigned int n;
    CSealAnimFrame *fb;
    CSealAnimFrame *fa;
    top = m_count - 1;
    r = 1.0f - t;
    fb = &m_frames->m_frames[top - 1];
    fa = &m_frames->m_frames[top];
    n = siNumBodyObjects;
    rb = fb->m_recs;
    ra = fa->m_recs;
    i = 0;

    if (i < *(const unsigned int *)&siNumBodyObjects) {
        do {
            if (ra->m_id > -1) {
                rb->m_id = ra->m_id;
                if (!rb->m_nopos || !ra->m_nopos) {
                    if (t >= 0.999f)
                        rb->m_pos = ra->m_pos;
                    else if (t > 0.001f) {
                        float w = *(float *)&rb->m_id;

                        sceVu0InterVectorXYZ((float *)rb, (float *)rb, (float *)ra, r);
                        *(float *)&rb->m_id = w;
                    }
                }
            }
            i++;
            rb++;
            ra++;
        } while (i < n);
    }
    rb = fb->m_recs;
    ra = fa->m_recs;
    i = 0;
    if (i < n) {
        do {
            if (ra->m_id > -1) {
                if (rb->m_id == -1)
                    rb->m_rot = ra->m_rot;
                else if (t >= 0.999f)
                    rb->m_rot = ra->m_rot;
                else if (t > 0.001f)
                    rb->m_rot.Slerp(&ra->m_rot, t, &rb->m_rot);
            }
            i++;
            rb++;
            ra++;
        } while (i < n);
    }
    m_count = top;
    return m_count;
}

/* the SEAL's adrenaline (OVL2 0x5781B0, the unit's last; SOCOM 1's, the same load) */
CAdrenaline *CZSealBody::Adrenaline()
{
    return m_adrenaline;
}

/* whether an anim type is the blend on top: itself, or its base anim (OVL2 0x577E40; SOCOM 1's,
 * which compared the base anim only) */
bool CZSealBody::IsCurrentBlend(AnimType type) const
{
    if (type == m_curblend)
        return true;
    return m_curblend == m_animset->BaseAnim(type);
}

/* CBody::SmoothPart (OVL1 0x28E040, body.cpp's) TAKES THE FRACTION FIRST: every call here loads it
 * before the part (MW evaluates the arguments left to right) -- the (part, t) declaration gave the
 * same 0x160 bytes with each pair of loads swapped. SOCOM II's own SetPartActive(int, part) has the
 * same shape. zbody.h and the row say `SmoothPart__5CBodyFfP10CZBodyPart` since the engine core's
 * round 2, so the calls are member calls (the CBody base at 0x170 reached without a null test) */

/* twenty of the SEAL's parts eased toward their saved poses by the overlay's time left (OVL2 0x576700,
 * new in SOCOM II: 0x576860 calls it while the overlay runs out; the name unknown) -- the parts
 * 0x5765F0 saves, in its order */
extern "C" void func_00576700(CZSealBody *seal)
{
    seal->SmoothPart(seal->m_10d0, seal->m_root);
    seal->SmoothPart(seal->m_10d0, seal->m_aimnodes);
    seal->SmoothPart(seal->m_10d0, seal->m_lhand);
    seal->SmoothPart(seal->m_10d0, seal->m_rhand);
    seal->SmoothPart(seal->m_10d0, seal->m_hips);
    seal->SmoothPart(seal->m_10d0, seal->m_head);
    seal->SmoothPart(seal->m_10d0, seal->m_neck);
    seal->SmoothPart(seal->m_10d0, seal->m_spinehi);
    seal->SmoothPart(seal->m_10d0, seal->m_spinelo);
    seal->SmoothPart(seal->m_10d0, seal->m_rbicep);
    seal->SmoothPart(seal->m_10d0, seal->m_rforearm);
    seal->SmoothPart(seal->m_10d0, seal->m_lbicep);
    seal->SmoothPart(seal->m_10d0, seal->m_lforearm);
    seal->SmoothPart(seal->m_10d0, seal->m_lscap);
    seal->SmoothPart(seal->m_10d0, seal->m_rscap);
    seal->SmoothPart(seal->m_10d0, seal->m_lshoulder_wgt);
    seal->SmoothPart(seal->m_10d0, seal->m_rshoulder_wgt);
    seal->SmoothPart(seal->m_10d0, seal->m_rifle);
    seal->SmoothPart(seal->m_10d0, seal->m_pistol);
    seal->SmoothPart(seal->m_10d0, seal->m_weapon);
}

/* and the same twenty saved (OVL2 0x5765F0, new in SOCOM II: the overlay's starts call it,
 * 0x579EF0 and 0x57A010; the name unknown) */
extern "C" void func_005765F0(CZSealBody *seal)
{
    seal->SavePart(seal->m_root);
    seal->SavePart(seal->m_aimnodes);
    seal->SavePart(seal->m_lhand);
    seal->SavePart(seal->m_rhand);
    seal->SavePart(seal->m_hips);
    seal->SavePart(seal->m_head);
    seal->SavePart(seal->m_neck);
    seal->SavePart(seal->m_spinehi);
    seal->SavePart(seal->m_spinelo);
    seal->SavePart(seal->m_rbicep);
    seal->SavePart(seal->m_rforearm);
    seal->SavePart(seal->m_lbicep);
    seal->SavePart(seal->m_lforearm);
    seal->SavePart(seal->m_lscap);
    seal->SavePart(seal->m_rscap);
    seal->SavePart(seal->m_lshoulder_wgt);
    seal->SavePart(seal->m_rshoulder_wgt);
    seal->SavePart(seal->m_rifle);
    seal->SavePart(seal->m_pistol);
    seal->SavePart(seal->m_weapon);
}

extern "C" {
/* the body-part ids of the rifle and the pistol, and the third one the weapon in hand moves to when
 * a script animates it (OVL1 data, read as variables -- seal_env.cpp's) */
extern int data_003DF148;
extern int data_003DF150;
extern int data_003DF160;
}

/* a script's object for a body part, none past its table (the name ours) */
static inline CZSIObject *ObjectFor(CZSIScriptCommon *s, int id)
{
    CZSIObject *o;

    if (id >= 0 && (unsigned int)id < s->m_objects_for_body.size())
        o = s->m_objects_for_body[id];
    else
        o = 0;
    return o;
}

/* the weapon in hand put on the part a pushed script animates: the rifle's or the pistol's own part
 * when the script has an object for it, else the third part when it has one for that (OVL2 0x5763E0,
 * new in SOCOM II: 0x577EA0 calls it for each anim it pushes unless the weapons are changing; the
 * name unknown) */
extern "C" void func_005763E0(CZSealBody *seal, CZSIScriptCommon *s)
{
    unsigned char item = seal->m_item;

    if (item == 1) {
        int id = data_003DF148;

        if (ObjectFor(s, id)) {
            seal->SetBodyPart(id, seal->m_rifle);
            seal->SetBodyPart(data_003DF160, 0);
            return;
        }
        if (ObjectFor(s, data_003DF160)) {
            seal->SetBodyPart(id, 0);
            seal->SetBodyPart(data_003DF160, seal->m_rifle);
            return;
        }
    }
    if (item == 2) {
        int id = data_003DF150;

        if (ObjectFor(s, id)) {
            seal->SetBodyPart(id, seal->m_pistol);
            seal->SetBodyPart(data_003DF160, 0);
            return;
        }
        if (ObjectFor(s, data_003DF160)) {
            seal->SetBodyPart(id, 0);
            seal->SetBodyPart(data_003DF160, seal->m_pistol);
            return;
        }
    }
}

/* a part's node placed where the part is (CZBodyPart::Orient's body, body.cpp's, written out: SOCOM
 * II inlines it here, through the null-tested pointer setters) */
static inline void OrientPart(CZBodyPart *part)
{
    CNode *node = part->m_node;

    node->SetPosition(&part->m_pos);
    node->SetRotation(&part->m_rot);
}

extern "C" {
/* the set whose SEALs show the pickup's anim (OVL1 bss 0x44D400), its anim type (OVL1 data 0x3DEE60),
 * and whether the top blend is one of five anims (seal_body.cpp's, OVL2 0x587E80) */
extern AnimSet *data_0044D400;
extern short data_003DEE60;
bool func_00587E80(CZSealBody *seal);
/* the body's top blend (seal.cpp's tail copy, OVL2 0x555170) */
void *func_00555170(void *stack);
}

/* the top blend's flag 4 (an inline bool: the target materialises it, sltu, before the branch) */
static inline bool BlendFlag4(const CZBodyAnimBlend *b)
{
    return b->m_flags & 4 ? true : false;
}

/* the SEAL posed from its anims, when it is animated: every anim of the top blend pushed with its
 * overlays laid over it -- each one's weapon put on its part first unless the weapons are changing --
 * the anims blended, the body's own overlay laid over while it runs, and for the 0x44D400 set, dead,
 * flagged 4 or in one of 0x587E80's anims, the 0x3DEE60 anim laid over whole; the pose put back on the
 * parts (OVL2 0x577EA0, new in SOCOM II: 0x57A330 calls it -- the name unknown) */
extern "C" void func_00577EA0(CZSealBody *seal)
{
    if (!seal->m_animate_me)
        return;
    CZBodyAnimBlend *top = (CZBodyAnimBlend *)func_00555170(&seal->m_blends);
    int n = 0;

    data_0044CDE0->Begin(seal);
    bool changing = seal->IsChangingWeapons(true);

    for (ACTIVE_ANIM *a = top->m_anims.Begin(); a; a = top->m_anims.Next()) {
        if (!changing)
            func_005763E0(seal, a->m_anim);
        data_0044CDE0->AnimPush(a->m_anim, a->m_ffrac, a->m_wt, a->m_frame);
        n++;
        for (ACTIVE_ANIM *o = a->m_overlay; o; o = o->m_overlay) {
            if (!changing)
                func_005763E0(seal, o->m_anim);
            data_0044CDE0->AnimPush(o->m_anim, o->m_ffrac, o->m_wt, o->m_frame);
            data_0044CDE0->AnimOverlay(o->m_wt);
        }
    }
    data_0044CDE0->AnimBlend(n);
    CBlendOverlay &ov = seal->m_overlay;

    if (ov.m_anim && ov.m_state) {
        data_0044CDE0->AnimPush(ov.m_anim->m_anim, ov.m_anim->m_ffrac, ov.m_anim->m_wt, ov.m_anim->m_frame);
        data_0044CDE0->AnimOverlay(1.0f);
    }
    if (seal->m_animset == data_0044D400 && (top->m_state == 8 || BlendFlag4(top) || func_00587E80(seal))) {
        CZSIScriptCommon *s = seal->m_animset->GetScript((AnimType)data_003DEE60);

        if (s) {
            data_0044CDE0->AnimPush(s, 0.0f, 1.0f, 0);
            data_0044CDE0->AnimOverlay(1.0f);
        }
    }
    data_0044CDE0->AnimPop();
}

/* the parts' poses saved unless the body interpolates, then every live part with a node placed on
 * it (OVL2 0x5762E0, new in SOCOM II: PrePostTick calls it; the name unknown) */
extern "C" void func_005762E0(CZSealBody *seal)
{
    if (seal->m_interp1 <= 0.0f)
        seal->SaveParts();
    std::vector<CZBodyPart *> &parts = seal->m_body_parts;

    for (std::vector<CZBodyPart *>::iterator i = parts.begin(); i != parts.end(); ++i) {
        CZBodyPart *part = *i;

        if (part && part->m_id >= 0 && part->m_node)
            OrientPart(part);
    }
}

/* the SEAL's blender made (OVL2 0x5761B0: CGame::StartEngine calls it where SOCOM 1's builds a
 * CSealAnim with its constructor out of line; the name unknown) */
extern "C" void func_005761B0()
{
    data_0044CDE0 = new CSealAnim;
}

/* and done with (OVL2 0x576120, the unit's first: CGame::StopEngine calls it where SOCOM 1's calls
 * ~CSealAnim; the name unknown) */
extern "C" void func_00576120()
{
    delete data_0044CDE0;
    data_0044CDE0 = 0;
}
