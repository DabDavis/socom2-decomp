/* seal_anim.cpp -- the SEAL animated: its blends pushed and popped, its root and vertical, its
 * adrenaline (SOCOM 1's unit, __sinit_seal_anim.cpp: Post, GetRootPos, SetVertical, the CSealAnim
 * blender, Animate, AdrenalineIncr and Adrenaline last). OVL2 0x576120 (the blender deleted: StopEngine
 * calls it where SOCOM 1's calls ~CSealAnim -- the rows before it are seal_ai.cpp's template tail, the
 * animate state's list instances) to 0x5781BF (Adrenaline), before seal_body.cpp. Defined in reverse
 * of their image order (-inline deferred). */

#include "zseal.h"

extern "C" {
/* AnimSet::BaseAnim: an anim type's base anim, or -1 (SOCOM 1's BaseAnim__7AnimSetF8AnimType, called
 * here where SOCOM 1's IsCurrentBlend calls it; OVL2 0x5E1A90, the kit lane's range) */
AnimType func_005E1A90(AnimSet *set, AnimType type);
}

/* a part's pose as the blender keeps it (SOCOM 1's records, 0x20 bytes: a position, the part's id
 * -- below zero for none -- and a rotation; the names ours) */
struct CSealAnimRec {
    CPnt3D m_pos;   // 0x0
    short m_id;   // 0xc
    CQuat m_rot;   // 0x10
};

/* a frame on the blender's stack: the parts' records and the anim pushed (0xc bytes; the names ours) */
struct CSealAnimFrame {
    int m_0;   // 0x0
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

/* the top frame's records put back on the SEAL's parts, each with a part, its position and rotation
 * (OVL2 0x577700; SOCOM 1's AnimPop, after AnimBlend in both builds, its 32 records a count now) --
 * called out of line from 0x577DF0: pinned */
/* OPEN, 0xCC both (the loop's registers): the target loads the record count into a3 before the loop
 * and keeps it for the bottom test, re-reading the global only for the entry test; MW here re-reads it
 * every pass (a local bound drops the entry test's own load: 0xC4). Tried: the count const, an int
 * counter, the record copies through an inline. The index `top` (kept, not folded into the -12) and
 * `> -1` (slti) are the target's. */
#pragma auto_inline off
int CSealAnim::AnimPop()
{
    std::vector<CZBodyPart *> &parts = m_seal->m_body_parts;
    int top = m_count - 1;
    CSealAnimRec *rec = m_frames->m_frames[top].m_recs;

    for (unsigned int i = 0; i < siNumBodyObjects; i++, rec++) {
        if (rec->m_id > -1) {
            CZBodyPart *part = parts[rec->m_id];

            if (part) {
                part->m_pos = rec->m_pos;
                part->m_rot = rec->m_rot;
            }
        }
    }
    m_count--;
    return m_count;
}
#pragma auto_inline reset

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
    return m_curblend == func_005E1A90(m_animset, type);
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
