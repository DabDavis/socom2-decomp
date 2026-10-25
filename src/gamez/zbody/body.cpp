/* body.cpp -- the body: CBody, its parts, its blends and active anims, CZBodyAnim and the
 * callback lists (OVL1 0x28B270-0x290360; SOCOM 1's unit, __sinit_body.cpp, CBody's RTTI and
 * strings "null", "skel_root", "do_not_use"). Its tail emits the RTList, RTArray and RTStack
 * instances. Defined in reverse of their image order (-inline deferred); only some written. */

#include "zbody.h"

/* a node's tree of parts, each filed by its id (SOCOM 1's; returns the top part) */
CZBodyPart *CBody::BodySetup(CNode *node, CZBodyPart *parent)
{
    CZBodyPart *part = NewBodyPart(node, parent);

    if (!part)
        return 0;
    m_body_parts[part->m_id] = part;
    zdb::CNodeVector &kids = node->GetChildren();

    for (zdb::CNodeVector::iterator i = kids.begin(); i != kids.end(); ++i)
        BodySetup(*i, part);
    return part;
}

/* a part for a node: the null node's id for none, else the bone of the node's name (SOCOM 1's).
 * OPEN, 0 instructions: every byte right, but `new CZBodyPart(node)` calls CPnt3D's empty
 * constructor out of line for m_pos and m_saved_pos, and that copy's row, OVL1 0x1FB3C0, is still
 * func_001FB3C0 (outside this lane's range): renamed __ct__6CPnt3DFv, the two calls resolve. */
CZBodyPart *CBody::NewBodyPart(CNode *node, CZBodyPart *parent)
{
    int id = NullNode();

    if (node) {
        id = func_0058CB30(node->m_name);
        if (id < 0)
            return 0;
    }
    CZBodyPart *part = new CZBodyPart(node);

    part->m_parent = parent;
    part->m_id = id;
    return part;
}

/* a part made and filed by its id (SOCOM 1's) */
void CBody::AddBodyPart(CNode *node, CZBodyPart *parent)
{
    CZBodyPart *part = NewBodyPart(node, parent);

    if (part)
        m_body_parts[part->m_id] = part;
}

/* a blend taken from the pool, cleared (SOCOM II's) */
CZBodyAnimBlend *CBlendPool::GetNext()
{
    CZBodyAnimBlend *blend = RTArray<CZBodyAnimBlend>::GetNext(true);

    if (blend) {
        blend->Clear();
        return blend;
    }
    return 0;
}

/* a blend cleared and put back (SOCOM II's) */
void CBlendPool::PutBack(CZBodyAnimBlend *blend)
{
    blend->Clear();
    RTArray<CZBodyAnimBlend>::PutBack(blend);
}

/* the node placed where the part is, both through CNode's null-testing pointer forms (SOCOM 1's;
 * the position's test is of `this`, m_pos being at 0) */
void CZBodyPart::Orient()
{
    CNode *node = m_node;

    node->SetPosition(&m_pos);
    node->SetRotation(&m_rot);
}

/* every weight on the list scaled (SOCOM 1's) */
void RenormAnims(CActiveAnimList *list, float scale)
{
    ACTIVE_ANIM *a = list->Begin();

    while (a) {
        a->m_wt = (float)(scale * a->m_wt);
        a = list->Next();
    }
}

/* the anim weighted and added (SOCOM 1's) */
void PushAnim(CActiveAnimList *list, ACTIVE_ANIM *anim, float wt)
{
    anim->m_wt = wt;
    list->Add(anim);
}

/* the frame of the step: the blend's time fraction in sixtieths (SOCOM 1's called fptosi).
 * OPEN, 3 instructions (right size): the target loads m_tfrac into $f1 and 60 into $f0 and
 * multiplies $f0 * $f1 -- SOCOM 1's registers too; every spelling tried (either operand order, a
 * float local for either, the product in a local) loads m_tfrac into $f0. */
int CZBodyAnimBlend::ThisFoot()
{
    return (int)(m_tfrac * 60.0f);
}

/* the representative's weight (SOCOM 1's) */
void CZBodyAnimBlend::SetRepresentativeWt(float wt)
{
    m_anims.Begin()->m_wt = wt;
}

/* the blend's representative: its first anim (SOCOM 1's returned that anim's CZBodyAnim) */
ACTIVE_ANIM *CZBodyAnimBlend::Representative()
{
    return m_anims.Begin();
}

/* the blend run backwards: its first anim's playback made negative (SOCOM 1's) */
void CZBodyAnimBlend::ReverseDir()
{
    m_flags = func_0028F1B0(m_flags, 0x20, true);
    ACTIVE_ANIM *a = m_anims.Begin();

    a->m_playback = -fabsf(a->m_playback);
    if (!func_0028F190(m_flags, 0x40))
        m_tfrac = m_quitT;
}

/* an overlay hung on an anim, at a weight, playing as it does (SOCOM 1's) */
void CZBodyAnimBlend::AddOverlay(ACTIVE_ANIM *anim, ACTIVE_ANIM *overlay, float wt)
{
    anim->m_overlay = overlay;
    overlay->m_overlay = 0;
    overlay->m_wt = wt;
    overlay->m_18 = anim->m_18;
    overlay->m_playback = anim->m_playback;
}

/* the anim of an id, or none (SOCOM 1's took the CZBodyAnim) */
ACTIVE_ANIM *CZBodyAnimBlend::FindAnim(int id)
{
    for (ACTIVE_ANIM *a = m_anims.Begin(); a; a = m_anims.Next())
        if (a->m_id == id)
            return a;
    return 0;
}

/* the anims and their overlays (SOCOM 1's) */
int CZBodyAnimBlend::CountAnims()
{
    int n = 0;

    for (ACTIVE_ANIM *a = m_anims.Begin(); a; a = m_anims.Next()) {
        n++;
        for (ACTIVE_ANIM *o = a->m_overlay; o; o = o->m_overlay)
            n++;
    }
    return n;
}
