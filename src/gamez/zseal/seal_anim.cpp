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
};

/* a part's pose as the blender keeps it (SOCOM 1's records, 0x20 bytes: a position, the part's id
 * -- below zero for none -- and a rotation; the names ours) */
struct CSealAnimRec {
    CPnt3D m_pos;   // 0x0
    short m_id;   // 0xc
    CQuat m_rot;   // 0x10
};

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
     * SEAL's parts, the depth left returned (SOCOM 1's took the body; OVL2 0x577700) */
    void AnimPush(CZSIScriptCommon *anim, float ffrac, float wt, short frame);
    int AnimPop();
    /* frame a's rotations moved toward frame b's by b's share of their weights, a given both (SOCOM
     * 1's; OVL2 0x576E30) */
    int CombineRotations(int a, int b);

    int m_count;   // 0x0
    CZSealBody *m_seal;   // 0x4
    CSealAnimFrames *m_frames;   // 0x8
};

extern "C" {
extern CSealAnim *data_0044CDE0;
}

/* a SEAL posed by one anim alone: the blender emptied and given the SEAL, the anim pushed, the pose
 * popped onto the parts (OVL2 0x577DF0, new in SOCOM II: seal_cfg.cpp's 0x58C600 calls it; the name
 * unknown).
 * OPEN, 0x50 against 0x4C (1 instruction more, the rest reordered): the two stores through one load
 * of the global needed the inline Begin (written out, MW reloads the global between them); left, MW
 * copies the anim pointer to v1 (`move v1,a1`) and loads its m_anim into a1 second, where the target
 * loads the frame (a2), the two floats, then m_anim into a1 last, off a1 itself. Tried: the floats
 * before or around the frame in AnimPush's prototype (the same code), the fields in locals first, the
 * anim through a local, the calls as CSealAnim members (round 12: SOCOM 1's AnimPush/AnimPop), an
 * inline AnimPush(const ACTIVE_ANIM *) unpacking the fields. */
extern "C" void func_00577DF0(CZSealBody *seal, ACTIVE_ANIM *anim)
{
    data_0044CDE0->Begin(seal);
    data_0044CDE0->AnimPush(anim->m_anim, anim->m_ffrac, anim->m_wt, anim->m_frame);
    data_0044CDE0->AnimPop();
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

/* a part's node placed where the part is (CZBodyPart::Orient's body, body.cpp's, written out: SOCOM
 * II inlines it here, through the null-tested pointer setters) */
static inline void OrientPart(CZBodyPart *part)
{
    CNode *node = part->m_node;

    node->SetPosition(&part->m_pos);
    node->SetRotation(&part->m_rot);
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
