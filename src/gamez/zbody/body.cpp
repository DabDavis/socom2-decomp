/* body.cpp -- the body: CBody, its parts, its blends and active anims, the overlay, CZBodyAnim and
 * the callback lists (OVL1 0x28B270-0x290360; SOCOM 1's unit, __sinit_body.cpp, CBody's RTTI and
 * strings "null", "skel_root", "do_not_use"). Its tail emits the RTList, RTArray and RTStack
 * instances, the flag helpers and the blend's member classes' destructors. Defined in reverse of
 * their image order (-inline deferred). */

#include <algorithm>
#include <string.h>

#include "zbody.h"
#include "../zsys/zsys.h"
#include "../zanim/zanim.h"

/* the ZAnim of a name (zanim.h declares only GetAnim(int, int): called by its mangled name) */
extern "C" CZAnim *GetAnim__10CZAnimMainFPCcPCc(CZAnimMain *main, const char *name, const char *set);

extern "C" {
float floorf(float);
/* VU0: v0.xyz = v1 + (v2 - v1) * t, the libvu0 helper; and zmath_pnt4d.cpp's out.xyz = a * s + b */
void sceVu0InterVectorXYZ(float *v0, float *v1, float *v2, float t);
void func_00309280(float *out, const float *a, const float *b, float s);
}

/* a part deleted and its slot cleared (the operation SOCOM 1's lists are freed with, through
 * std::transform; the name is ours) */
struct DeleteBodyPart {
    DeleteBodyPart() {}
    CZBodyPart *operator()(CZBodyPart *p) const
    {
        delete p;
        return 0;
    }
};

/* the node placed where the part is, both through CNode's null-testing pointer forms (SOCOM 1's;
 * the position's test is of `this`, m_pos being at 0) */
void CZBodyPart::Orient()
{
    CNode *node = m_node;

    node->SetPosition(&m_pos);
    node->SetRotation(&m_rot);
}

/* the body built empty: its pools (24 anims, 8 blends, both owning their items), the stack of
 * eight blends, no parts, no overlay (SOCOM 1's) */
CBody::CBody()
    : m_interp1(0.0f), m_interp2(0.0f), m_active_anim_pool(0x18, true, false),
      m_blend_pool(8, true, false), m_blends(8)
{
}

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

/* a part for a node: the null node's id for none, else the bone of the node's name (SOCOM 1's) */
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

/* an auxiliary node hung on a part, its rotation given as angles (SOCOM 1's) */
CZBodyPart *CBody::AttachAuxNode(CNode *node, CZBodyPart *parent, const CPnt3D *pos, const CPnt3D *rot)
{
    CQuat q;

    return AttachAuxNode(node, parent, pos, QFromRot(&q, rot));
}

/* an auxiliary node hung on a part: a part made for it, its node added to the parent's, placed
 * (SOCOM 1's) */
CZBodyPart *CBody::AttachAuxNode(CNode *node, CZBodyPart *parent, const CPnt3D *pos, const CQuat *rot)
{
    CZBodyPart *part = new CZBodyPart(node);

    part->m_parent = parent;
    parent->m_node->AddChild(node);
    part->m_node = node;
    m_body_parts_aux.push_back(part);
    part->m_pos = *pos;
    part->m_rot = *rot;
    part->Orient();
    return part;
}

/* a part hung on another and kept with the auxiliary parts (SOCOM 1's) */
CZBodyPart *CBody::AttachAuxPart(CZBodyPart *part, CZBodyPart *parent)
{
    part->m_parent = parent;
    CNode *node = part->m_node;

    if (parent)
        parent->m_node->AddChild(node);
    part->m_node = node;
    m_body_parts_aux.push_back(part);
    return part;
}

/* the auxiliary parts' nodes searched for a name, each node's subtree too (SOCOM 1's) */
CNode *CBody::FindAuxNode(const char *name, bool deep)
{
    for (std::vector<CZBodyPart *>::iterator i = m_body_parts_aux.begin(); i != m_body_parts_aux.end(); ++i) {
        CNode *node = (*i)->m_node;

        if (!deep && node && !strcmp(name, node->m_name))
            return node;
        CNode *found = node ? node->FindChild(name, true) : 0;

        if (found)
            return found;
    }
    return 0;
}

/* the part whose node has a name (SOCOM 1's) */
CZBodyPart *CBody::FindPart(const char *name)
{
    for (std::vector<CZBodyPart *>::iterator i = m_body_parts.begin(); i != m_body_parts.end(); ++i) {


        if (*i) {
            CNode *node = (*i)->m_node;

            if (!strcmp(node ? node->m_name : "", name))
                return *i;
        }
    }
    return 0;
}

/* a part filed at an id in range (SOCOM II's; the name ours) */
void CBody::SetBodyPart(int id, CZBodyPart *part)
{
    if (id < m_body_parts.size() && id >= 0)
        m_body_parts[id] = part;
}

/* an archive's anim of a motion.rdr index readied for the body, once: each part's object found by
 * its node's name and filed by the part's id, then the other objects whose bone ids lie past the
 * body's parts filed from the top slot down (SOCOM 1's BodyAddAnim, which took a name; SOCOM II's
 * returns its stack anim, gone by then).
 * OPEN, 16 instructions (right size): the first walk's registers -- the target keeps the iterator
 * in s3, the part in s4 and the slot pointer in s2 (this's, loaded in the walk's entry delay slot),
 * where this source gives the iterator s2, the slot s4 (loaded ahead of the test) and the part s3.
 * Tried: the slot declared before the reference, the iterator declared before the slot, an index
 * instead of the slot pointer (int or unsigned: 8 bytes longer). The stack anim through a pointer
 * and the decrement as the index's pre-decrement each took off a block of differences. */
CZBodyAnim *CBody::BodyAddAnim(CScriptArchive *archive, int index)
{
    if (index <= 0)
        return 0;
    CZSIScriptCommon *common = archive->m_byRdr[index];

    if (!common)
        return 0;
    CZBodyAnim anim;
    CZBodyAnim *a = &anim;

    a->m_pCommon = common;
    if (!common->m_objects_ready) {
        std::vector<CZBodyPart *> &parts = m_body_parts;
        CZSIObject **slot = &common->m_objects_for_body[0];

        for (std::vector<CZBodyPart *>::iterator i = parts.begin(); i != parts.end(); ++i, slot++) {
            CZBodyPart *part = *i;

            if (part && !*slot) {
                CZSIObject *o = a->GetObject(part->m_node ? part->m_node->m_name : "", true);

                if (o) {
                    o->m_body_part_id = part->m_id;
                    a->m_pCommon->m_objects_for_body[o->m_body_part_id] = o;
                }
            }
        }
        common->m_nBodyObjects = siNumBodyObjects - 4;
        CZSIObject *o = a->m_pCommon->m_pObjects;

        for (int i = 0; i < a->m_pCommon->m_nObjects; i++, o++) {
            if (o && !o->m_inUse && strcmp(o->m_pName, "do_not_use")) {
                unsigned int id = func_0058CB30(o->m_pName);

                if (id >= (unsigned int)siNumBodyObjects) {
                    o->m_body_part_id = id;
                    a->m_pCommon->m_objects_for_body[--common->m_nBodyObjects] = o;
                }
            }
        }
        common->m_objects_ready = true;
    }
    return a;
}

/* each part's pose read back from its node (SOCOM II's; the name ours) */
void CBody::ReadParts()
{
    std::vector<CZBodyPart *> &parts = m_body_parts;

    for (std::vector<CZBodyPart *>::iterator i = parts.begin(); i != parts.end(); ++i) {
        CZBodyPart *part = *i;

        if (part) {
            CNode *node = part->m_node;

            if (node) {
                part->m_pos.x = node->m_matrix.m_matrix[3][0];
                part->m_pos.y = node->m_matrix.m_matrix[3][1];
                part->m_pos.z = node->m_matrix.m_matrix[3][2];
                node->GetRotation(&part->m_rot);
            }
        }
    }
}

/* each part's pose saved (SOCOM II's; the name ours; pinned: PushBlend and PopBlend call it) */
#pragma auto_inline off
void CBody::SaveParts()
{
    std::vector<CZBodyPart *> &parts = m_body_parts;

    for (std::vector<CZBodyPart *>::iterator i = parts.begin(); i != parts.end(); ++i) {
        CZBodyPart *part = *i;

        if (part) {
            part->m_saved_pos = part->m_pos;
            part->m_saved_rot = part->m_rot;
        }
    }
}
#pragma auto_inline reset


/* one part's pose saved (SOCOM II's; the name ours) */
void CBody::SavePart(CZBodyPart *part)
{
    if (part) {
        CZBodyPart *p = m_body_parts[part->m_id];

        if (p) {
            p->m_saved_pos = p->m_pos;
            p->m_saved_rot = p->m_rot;
        }
    }
}

/* every part placed on its node (SOCOM II's; the name ours) */
void CBody::OrientParts()
{
    std::vector<CZBodyPart *> &parts = m_body_parts;

    for (std::vector<CZBodyPart *>::iterator i = parts.begin(); i != parts.end(); ++i) {
        CZBodyPart *part = *i;

        if (part && part->m_node)
            part->Orient();
    }
}

/* every auxiliary part placed on its node (SOCOM II's; the name ours) */
void CBody::OrientAuxParts()
{
    for (std::vector<CZBodyPart *>::iterator i = m_body_parts_aux.begin(); i != m_body_parts_aux.end(); ++i) {
        CZBodyPart *part = *i;

        if (part && part->m_node)
            part->Orient();
    }
}

/* the live parts eased from their saved poses toward the new ones (SOCOM 1's) */
void CBody::SmoothAnims(float t)
{
    std::vector<CZBodyPart *> &parts = m_body_parts;

    for (std::vector<CZBodyPart *>::iterator i = parts.begin(); i != parts.end(); ++i) {
        CZBodyPart *part = *i;

        if (part && part->Active() != false) {
            float w = *(float *)&part->m_node;

            sceVu0InterVectorXYZ((float *)&part->m_pos, (float *)&part->m_saved_pos, (float *)&part->m_pos, t);
            *(float *)&part->m_node = w;
            part->m_rot.Slerp(&part->m_saved_rot, t, &part->m_rot);
        }
    }
}

/* one live part eased likewise (SOCOM II's; the name ours) */
void CBody::SmoothPart(CZBodyPart *part, float t)
{
    if (part) {
        CZBodyPart *p = m_body_parts[part->m_id];

        if (p && p->Active() != false) {

            float w = *(float *)&p->m_node;

            sceVu0InterVectorXYZ((float *)&p->m_pos, (float *)&p->m_saved_pos, (float *)&p->m_pos, t);
            *(float *)&p->m_node = w;
            p->m_rot.Slerp(&p->m_saved_rot, t, &p->m_rot);
        }
    }
}

/* a part marked live or not (SOCOM II's; the name ours) */
void CBody::SetPartActive(int on, CZBodyPart *part)
{
    if (part) {
        short id = part->m_id;

        if (id >= 0 && id < m_body_parts.size()) {
            CZBodyPart *p = m_body_parts[id];

            if (p) {
                if (on)
                    p->SetActive(true);
                else
                    p->SetActive(false);
            }
        }
    }
}

/* the state (SOCOM 1's) */
void CBody::BodySetState(int state)
{
    m_state = state;
}

/* a blend started on a script: the top blend's homing reset, a blend and an anim taken from the
 * pools, the blend reset on the anim and pushed (SOCOM 1's took the anim by id). The registers
 * follow the source: the top through TopBlend() (s0) and the homing through Homing() (s1), the
 * loop flag and the two pointers declared first, uninitialised (s3, s2, s1), the fraction read
 * after the flag */
CZBodyAnimBlend *CBody::PushBlend(CZSIScriptCommon *anim, short id, char state)
{
    bool looped;
    CZBodyAnimBlend *blend;
    ACTIVE_ANIM *a;
    float tfrac = 0.0f;
    CZBodyAnimBlend *top = TopBlend();

    looped = false;
    if (top) {
        looped = func_0028F190(top->m_flags, 0x40);
        tfrac = top->m_tfrac;
        top->Homing()->Reset();
    }
    blend = m_blend_pool.GetNext();
    if (!blend)
        return top;
    a = m_active_anim_pool.GetActiveAnim(anim, id);

    if (!a) {
        m_blend_pool.PutBack(blend);
        return top;
    }
    blend->m_anims.Clear();
    blend->m_anims.Add(a);
    blend->m_flags = 0;
    blend->m_animType = -1;
    blend->Reset();
    blend->m_flags = func_0028F1B0(blend->m_flags, 0x40, a->m_anim->m_looped);
    if (a->m_anim->GetCallbackIndex(0) > 0)
        blend->m_flags = func_0028F1B0(blend->m_flags, 1, true);
    else
        blend->m_flags = func_0028F1B0(blend->m_flags, 1, false);
    a->m_wt = 1.0f;
    blend->m_callback.Clear();
    blend->m_animType = id;
    blend->m_state = state;
    if (looped && func_0028F190(blend->m_flags, 0x40))
        blend->m_tfrac = tfrac;
    if (!func_0028F190(blend->m_flags, 0x40)) {
        blend->m_flags &= ~8;
        blend->m_flags |= 8;
    }
    m_blends.Push(blend);
    if (m_interp1 > 0.0f)
        SaveParts();
    float t = anim->m_20;

    m_interp1 = t;
    m_interp2 = t;
    return blend;
}

/* the top blend popped: its anims and their overlays put back, the blend too; the new top's
 * state set and its fraction carried over when both loop (SOCOM 1's). Both walks reach the pool
 * through ActiveAnimPool(), the target's hoisted address in a preheader after each loop's test */
CZBodyAnimBlend *CBody::PopBlend()
{
    CZBodyAnimBlend *blend = func_00219390(&m_blends);

    if (m_interp1 > 0.0f)
        SaveParts();
    bool looped = func_0028F190(blend->m_flags, 0x40);
    float tfrac = blend->m_tfrac;
    ACTIVE_ANIM *a = blend->m_anims.Begin();

    while (a) {
        ACTIVE_ANIM *cur = a;

        a = blend->m_anims.Next();
        cur->ClearOverlays(this);
        ActiveAnimPool()->PutBack(cur);
    }
    m_blend_pool.PutBack(blend);
    m_blends.Pop();
    CZBodyAnimBlend *top = func_00219390(&m_blends);

    if (!top)
        return 0;
    float t = top->m_anims.Begin()->m_anim->m_20;

    m_interp1 = t;
    m_interp2 = t;
    BodySetState(top->m_state);
    if (looped && func_0028F190(top->m_flags, 0x40)) {
        top->m_tfrac = tfrac;
    } else {
        top->m_flags &= ~8;
        top->m_flags |= 8;
    }
    return top;
}

/* the overlay's anim put back and the overlay cleared (SOCOM 1's) */
void CBody::ClearBlendOverlay()
{
    if (m_overlay.m_anim)
        m_active_anim_pool.PutBack(m_overlay.m_anim);
    m_overlay.Clear();
}

/* an anim taken for a script, its id and both bits set (SOCOM 1's took the CZBodyAnim) */
ACTIVE_ANIM *CActiveAnimPool::GetActiveAnim(CZSIScriptCommon *anim, short id)
{
    ACTIVE_ANIM *a = GetNext(true);

    if (a) {
        a->Clear();
        a->m_anim = anim;
        a->m_id = id;
        a->SetBit0(true);
        a->SetBit1(true);
        return a;
    }
    return 0;
}

/* an anim cleared and put back (SOCOM 1's) */
void CActiveAnimPool::PutBack(ACTIVE_ANIM *anim)
{
    anim->Clear();
    RTArray<ACTIVE_ANIM>::PutBack(anim);
}

/* every field to rest (SOCOM 1's) */
void ACTIVE_ANIM::Clear()
{
    m_0 = 0;
    m_anim = 0;
    m_wt = 0.0f;
    m_overlay = 0;
    m_tfrac = 0.0f;
    m_dfrac = 0.0f;
    m_script_wt = 1.0f;
    m_ffrac = 0.0f;
    m_frame = 0;
    m_playback = 1.0f;
    SetBit0(false);
    SetBit1(false);
    m_id = -1;
}

/* the anim at a fraction of its script */
void ACTIVE_ANIM::SetFrac(float t)
{
    while (t > 1.0f)
        t -= 1.0f;
    while (t < 0.0f)
        t += 1.0f;
    m_tfrac = t;
    int n = m_anim->m_nFrames;

    m_ffrac = t * n;
    float f = floorf(m_ffrac);

    m_frame = (int)f;
    m_ffrac -= f;
    if (!m_anim->m_looped && m_frame + 1 >= n) {
        m_frame = n - 1;
        m_ffrac = 0.0f;
    }
}

/* every overlay put back in the body's pool (SOCOM 1's): the pool through the body's accessor, so
 * its address is hoisted into a preheader after the loop's test (the target's `b` into the walk) */
void ACTIVE_ANIM::ClearOverlays(CBody *body)
{
    ACTIVE_ANIM *o = m_overlay;

    while (o) {
        ACTIVE_ANIM *cur = o;

        o = o->m_overlay;
        body->ActiveAnimPool()->PutBack(cur);
    }
    m_overlay = 0;
}

/* the playback rate, scaled by the script's when it loops */
void ACTIVE_ANIM::SetPlayback(float rate)
{
    CZSIScriptCommon *anim = m_anim;

    if (anim->m_looped)
        rate *= anim->m_14;
    m_playback = rate;
}

/* the overlay's window around its trigger time: always when it has none (-4), else between the
 * two times, around the wrap when they cross (SOCOM 1's) */
bool CBlendOverlay::IsTriggerTime(float t0, float t1, bool reverse)
{
    float t = m_triggerT;

    if (t == -4.0f)
        return true;
    if (reverse) {
        if (t1 <= t0) {
            if (t1 <= t && t <= t0)
                return true;
            return false;
        }
        if (t1 - 1.0f <= t && t <= t0)
            return true;
        return false;
    }
    if (!(t1 < t0)) {
        if (t0 <= t && t <= t1)
            return true;
        return false;
    }
    if (t0 - 1.0f <= t && t <= t1)
        return true;
    return false;
}

/* started forwards or backwards, once (SOCOM 1's took the body too) */
bool CBlendOverlay::Trigger(bool reverse)
{
    if (!m_anim)
        return false;
    if (m_anim->m_anim) {
        if (m_state == 0) {
            if (reverse)
                m_state = -1;
            else
                m_state = 1;
        }

        return true;
    }
    return false;
}

/* the overlay run on: its time moved by dT, the anim placed, put back once played out (SOCOM 1's).
 * OPEN, 3 instructions: the first division's operands trade float registers (the target loads
 * m_ovlyT into $f0, the script's time into $f1), as in IsTriggerTime(float, float); tried a
 * (float) cast, the time through a local, a compound division */
bool CBlendOverlay::Tick(CBody *body, float dT)
{
    if (m_state == 1)
        m_ovlyT += dT;
    else if (m_state == -1)
        m_ovlyT -= dT;
    else
        return false;

    if (!m_anim)
        return false;
    CZSIScriptCommon *anim = m_anim->m_anim;
    float frac = m_ovlyT / anim->m_10;
    float done = m_ovlyT / (anim->m_10 * anim->TrueLoopTime());

    func_00219390(&body->m_blends);
    if (m_state == -1) {
        if (done <= 0.0f) {
            body->m_active_anim_pool.PutBack(m_anim);
            Clear();
            return true;
        }
        m_anim->SetFrac(frac);
    } else {
        if (!(done < 1.0f)) {
            body->m_active_anim_pool.PutBack(m_anim);
            Clear();
            return true;
        }
        m_anim->SetFrac(frac);
    }
    return false;


}

/* within dT of its anim's fraction, ahead or behind as it runs (SOCOM 1's; pinned: CallbackTick
 * calls it). OPEN, 2 instructions short: the target's two float registers trade places (the
 * length and the step in $f1, the fraction in $f0) and it pads its first arm's `b` with a nop
 * more; tried the step through one reused local and the fraction declared first */
#pragma auto_inline off
bool CBlendOverlay::IsTriggerTime(float t, float dT)
{
    if (!m_anim)
        return false;
    CZSIScriptCommon *anim = m_anim->m_anim;
    float d = dT / (anim->m_10 * anim->TrueLoopTime());
    float tf = m_anim->m_tfrac;

    if (m_state == -1)
        return tf - d <= t;
    return tf + d >= t;
}
#pragma auto_inline reset

/* its callback made once, when its time comes (SOCOM 1's) */
void CBlendOverlay::CallbackTick(void *body, float t)
{
    if (m_callback.m_func && IsTriggerTime(m_callback.m_tfrac, t)) {
        if (m_callback.m_func)
            m_callback.m_func(body, m_callback.m_userdata);
        m_callback.Clear();
    }
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

/* every field to rest, the anims' list cleared (SOCOM 1's) */
void CZBodyAnimBlend::Clear()
{
    m_quitT = 0.0f;
    m_tfrac = 0.0f;
    m_20 = 0;
    m_24 = 0;
    m_flags = 0;
    m_ctrl = 0;
    m_state = 0;
    m_animType = -1;
    m_2e = 0;
    m_anims.Clear();
    m_refpt_info.Clear();
    m_callback.Clear();
    m_homing.Reset();
}

/* the blend restarted: its fraction at the start (the end when reversed), every anim placed, the
 * homing reset (SOCOM 1's) */
void CZBodyAnimBlend::Reset()
{
    m_flags = func_0028F1B0(m_flags, 0x10, false);
    m_ctrl = 0;
    m_quitT = 1.0f;
    m_tfrac = 0.0f;
    m_dpos = CPnt3D::zero;
    m_drot = CPnt3D::zero;
    for (ACTIVE_ANIM *a = m_anims.Begin(); a; a = m_anims.Next()) {
        a->SetFrac(0.0f);
        a->m_playback = fabsf(a->m_playback);
    }
    CZSIScriptCommon *anim = m_anims.Begin()->m_anim;

    if (anim->m_looped) {
        m_quitT = 1.0f;
    } else {
        unsigned int n = anim->m_nFrames;

        if (n > 1) {

            float f = n;

            m_quitT = (f - 1.0f) / f;
        } else {
            m_quitT = 1.0f;
        }
        ACTIVE_ANIM *a = m_anims.Begin();

        if (func_0028F190(m_flags, 0x20)) {
            m_tfrac = m_quitT;
            a->SetPlayback(-1.0f);
            a->SetFrac(m_quitT);
        } else {
            m_tfrac = 0.0f;
            a->SetPlayback(1.0f);
            a->SetFrac(0.0f);
        }
    }
    m_homing.Reset();
    m_20 = 0;
    m_24 = 0;
}

/* the callbacks: the blend's own once its time comes, then each anim's archive callbacks whose
 * times fall in the step, each ZAnim looked up once by name and started on the body's node
 * (SOCOM 1's) */
void CZBodyAnimBlend::CallbackTick(CBody *body, float t0, float t1)
{
    if (m_callback.m_func && IsTriggerTime(m_callback.m_tfrac, t0, t1)) {
        if (m_callback.m_func)
            m_callback.m_func(body, m_callback.m_userdata);
        m_callback.Clear();
    }
    if (!func_0028F190(m_flags, 1))
        return;
    for (ACTIVE_ANIM *a = m_anims.Begin(); a; a = m_anims.Next()) {
        for (int i = 0; i < 4; i++) {
            int index = a->m_anim->GetCallbackIndex(i);

            if (index <= 0)
                break;
            CScriptArchive *archive = zBoneGetArchive(a->m_anim->m_archive);

            if (archive) {
                CZBodyAnimCallback *cb = archive->m_callbacks.Get(index);

                if (cb && IsTriggerTime(cb->m_tfrac, t0, t1)) {
                    if (!cb->m_anim && cb->m_lookup) {
                        cb->m_anim = GetAnim__10CZAnimMainFPCcPCc(&ZAnim, cb->m_name.c_str(), 0);
                        cb->m_lookup = false;
                    }
                    if (cb->m_anim) {
                        CZBodyPart *root = body->GetRootPart();

                        if (root) {
                            CNode *node = root->m_node;

                            if (node) {
                                CNode *parent = node->m_parent;

                                if (parent)
                                    node = parent;
                            }
                            if (node)
                                cb->m_anim->ActivateArg(1, 2, node, 0);
                        }
                    }
                }
            }
        }
    }
}

/* a time inside the step from t0 to t1, around the wrap when the blend loops (SOCOM 1's).
 * OPEN, 80 instructions: the window tests' block layout and returns -- first draft, not worked */
bool CZBodyAnimBlend::IsTriggerTime(float t, float t0, float t1)
{
    bool reverse;

    if (t0 == t1)
        return false;
    if (func_0028F190(m_flags, 0x40))
        reverse = Representative()->m_playback < 0.0f;
    else
        reverse = func_0028F190(m_flags, 0x20);
    if (func_0028F190(m_flags, 0x40)) {
        if (reverse) {
            if (t1 <= t0) {
                if (t1 <= t && t <= t0)
                    return true;
                return false;
            }
            if (t1 - m_quitT <= t && t <= t0)
                return true;
            return false;
        }
        if (!(t1 < t0)) {
            if (t0 <= t && t <= t1)
                return true;
            return false;
        }
        if (t0 - m_quitT <= t && t <= t1)
            return true;
        return false;
    }
    if (reverse) {
        if (t1 <= t0) {
            if (t1 <= t && t <= t0)
                return true;
            return false;
        }
        return false;
    }
    if (!(t1 < t0)) {
        if (t0 <= t && t <= t1)
            return true;
        return false;
    }
    return false;
}

/* half a step past the end, or before the start when reversed (SOCOM 1's) */
bool CZBodyAnimBlend::IsDone(float dT)
{
    CZSIScriptCommon *anim = Representative()->m_anim;
    float len = anim->m_10 * anim->TrueLoopTime();
    float h = 0.5f * (dT * Representative()->m_playback / len);
    float t = h + m_tfrac;

    if (func_0028F190(m_flags, 0x20)) {
        if (t < 0.0f)
            return true;
    } else if (t > m_quitT) {
        return true;
    }
    return false;
}

/* the fraction run on by dT through every anim's weighted rate, wrapped when looping, else held
 * between zero and the end; stored when asked (SOCOM 1's).
 * OPEN, 2 instructions short: the target materialises flag 8's test (andi, sltu, beqz) and reloads
 * the flags for the clear; an inline bool accessor folds (as written, `!= false`, a bool local, a
 * ternary), `== true` materialises but compares with 1 and keeps the load */
float CZBodyAnimBlend::Propagate(float dT, bool set)
{
    if (!m_anims.m_size)
        return 0.0f;
    float df = 0.0f;

    if (Holding()) {
        m_flags &= ~8;
    } else {
        for (ACTIVE_ANIM *a = m_anims.Begin(); a; a = m_anims.Next()) {
            float s = a->m_script_wt;

            if (s != 1.0f)
                dT *= s;
            CZSIScriptCommon *anim = a->m_anim;
            float pb = a->m_playback;
            float wt = a->m_wt;

            df += dT * (wt * pb) / (anim->m_10 * anim->TrueLoopTime());
        }
    }

    float t = df + m_tfrac;

    if (func_0028F190(m_flags, 0x40)) {
        while (t > 1.0f)
            t -= 1.0f;
        while (t < 0.0f)
            t += 1.0f;
    } else if (t > m_quitT) {
        t = m_quitT;
    } else if (t < 0.0f) {
        t = 0.0f;
    }
    if (set)
        m_tfrac = t;
    return t;
}

/* every anim and its overlays placed at the blend's fraction plus its own offset (SOCOM 1's took
 * the body) */
void CZBodyAnimBlend::Synch()
{
    float tfrac = m_tfrac;

    for (ACTIVE_ANIM *a = m_anims.Begin(); a; a = m_anims.Next()) {
        float t = a->m_dfrac;

        t += tfrac;

        if (func_0028F190(m_flags, 0x40)) {
            while (t > 1.0f)
                t -= 1.0f;
            while (t < 0.0f)
                t += 1.0f;
        } else if (t > m_quitT) {
            t = m_quitT;
        } else if (t < 0.0f) {
            t = 0.0f;
        }
        a->SetFrac(t);
        for (ACTIVE_ANIM *o = a->m_overlay; o; o = o->m_overlay)
            o->SetFrac(t);
    }
}

/* the anims' root velocities, each at its rate, weighted (SOCOM 1's) */
void CZBodyAnimBlend::GetVelocity(CPnt3D *vel)
{
    CPnt4D v;
    CPnt4D sum = CPnt4D::zero;
    CPnt3D *p = (CPnt3D *)&v;

    for (ACTIVE_ANIM *a = m_anims.Begin(); a; a = m_anims.Next()) {
        CZSIScriptCommon *anim = a->m_anim;

        anim->m_root->GetVelRaw(anim, a->m_frame, a->m_frame + 1, p);
        p->Scale(a->m_playback, p);

        if (!func_0028F190(m_flags, 0x40)) {
            float s = a->m_script_wt;

            if (s != 1.0f)
                sceVu0ScaleVector((float *)&v, (float *)&v, 1.0f / s);
        }
        func_00309280((float *)&sum, (float *)&v, (float *)&sum, a->m_wt);
    }
    vel->x = sum.x;
    vel->y = sum.y;
    vel->z = sum.z;
}

/* the frame of the step: the blend's time fraction in sixtieths (SOCOM 1's called fptosi).
 * OPEN, 3 instructions (right size): the target loads m_tfrac into $f1 and 60 into $f0 and
 * multiplies $f0 * $f1 -- SOCOM 1's registers too; every spelling tried (either operand order, a
 * float local for either, the product in a local) loads m_tfrac into $f0. The 60 through an inline
 * returning it (`(int)(FootRate() * m_tfrac)`) gives the target's operand order, 2 instructions off:
 * the two registers still trade places, with either order, a (float) cast or a local. */

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

/* the anims replaced by a list's: the old ones and their overlays put back (through
 * ActiveAnimPool(), as PopBlend), the weights normalised (out of line), the callback flag set when
 * an anim has callbacks (SOCOM 1's) */
void CZBodyAnimBlend::SetBlendAnims(CBody *body, CActiveAnimList *list)
{
    ACTIVE_ANIM *a = m_anims.Begin();

    while (a) {
        ACTIVE_ANIM *cur = a;

        a = m_anims.Next();
        m_anims.Remove(cur);
        cur->ClearOverlays(body);
        body->ActiveAnimPool()->PutBack(cur);
    }
    for (a = list->Begin(); a; a = list->Next())
        m_anims.Add(a);
    Normalize();
    m_flags = func_0028F1B0(m_flags, 1, false);
    for (a = m_anims.Begin(); a; a = m_anims.Next()) {
        if (a->m_anim && a->m_anim->GetCallbackIndex(0) > 0) {
            m_flags = func_0028F1B0(m_flags, 1, true);
            break;
        }
    }
}

/* the weights scaled to sum to one, unless they nearly do or sum to nothing (SOCOM 1's; pinned:
 * SetBlendAnims calls it) */
#pragma auto_inline off
void CZBodyAnimBlend::Normalize()
{
    if (m_anims.m_size) {
        float sum = 0.0f;

        for (ACTIVE_ANIM *a = m_anims.Begin(); a; a = m_anims.Next())
            sum += a->m_wt;
        if (!_close_enough(sum, 1.0f, 0.00001f) && !(sum <= 0.00001f))
            RenormAnims(&m_anims, 1.0f / sum);
    }
}
#pragma auto_inline reset

/* an overlay hung on an anim, at a weight, playing as it does (SOCOM 1's) */
void CZBodyAnimBlend::AddOverlay(ACTIVE_ANIM *anim, ACTIVE_ANIM *overlay, float wt)
{
    anim->m_overlay = overlay;
    overlay->m_overlay = 0;
    overlay->m_wt = wt;
    overlay->m_script_wt = anim->m_script_wt;
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

/* everything freed (SOCOM 1's) */
CBody::~CBody()
{
    Destroy();
}

/* the overlay cleared, every blend popped, the parts deleted (SOCOM 1's).
 * OPEN, 0x1A8 bytes short: the target inlines each transform whole -- the iterators, the functor,
 * the part's destructor and Clear's stores, then operator delete -- where MW inlines it one level
 * and calls the iterator operators and the functor out of line. Measured (probes p_tf2.cpp,
 * ims.py): with `~CZBodyPart() { Clear(); }` the transform's tree is SIX high (transform, the
 * functor, the destructor, Clear, CQuat's implicit assignment, CPnt3D's), cut at level 5 at any
 * inline_max_size; with Clear's stores written into the destructor it is five high but 299 units
 * (a CPnt3D copy costs ~49, a CQuat copy ~30, the empty transform 80, a user destructor 17), and
 * `#pragma inline_max_size(300)` then inlines it: 0x434 against 0x42C, only end() reloading data_
 * where the target reuses begin()'s load. Clear inside the functor (`if (p) { p->Clear(); delete
 * p; }`, no destructor) is five high and still over 256. The original fits both limits: its copies
 * (zmath.h) or the MSL pieces were a level shorter and ~45 units leaner -- not in this lane's files */

void CBody::Destroy()
{
    ClearBlendOverlay();
    func_00219390(&m_blends);
    while (m_blends.Size())
        PopBlend();
    std::transform(m_body_parts.begin(), m_body_parts.end(), m_body_parts.begin(), DeleteBodyPart());
    m_body_parts.clear();
    std::transform(m_body_parts_aux.begin(), m_body_parts_aux.end(), m_body_parts_aux.begin(), DeleteBodyPart());
    m_body_parts_aux.clear();
}

CZBodyAnim::~CZBodyAnim()
{
}

CZBodyAnimCallbackList::~CZBodyAnimCallbackList()
{
    Clear();
}

/* every callback deleted and the storage given up (SOCOM 1's) */
void CZBodyAnimCallbackList::Clear()
{
    for (std::vector<CZBodyAnimCallback *>::iterator i = m_callbacks.begin(); i != m_callbacks.end(); ++i)
        delete *i;
    FreeVector(m_callbacks);
}

/* every callback's ZAnim forgotten, to be looked up again (SOCOM II's; the name ours) */
void CZBodyAnimCallbackList::ResetAnims()
{
    for (std::vector<CZBodyAnimCallback *>::iterator i = m_callbacks.begin(); i != m_callbacks.end(); ++i) {
        CZBodyAnimCallback *cb = *i;

        cb->m_anim = 0;
        cb->m_lookup = true;
    }
}

/* the index of a callback of a name at a time, added when new; index 0 is a "null" one, made
 * first (SOCOM 1's) */
int CZBodyAnimCallbackList::AddCallback(const char *name, float t)
{
    if (m_callbacks.size() == 0) {
        CZBodyAnimCallback *null = new CZBodyAnimCallback("null", 0.0f);

        m_callbacks.push_back(null);
    }

    int i = 0;

    for (std::vector<CZBodyAnimCallback *>::iterator it = m_callbacks.begin(); it != m_callbacks.end(); ++it, i++) {
        CZBodyAnimCallback *cb = *it;

        if (t == cb->m_tfrac && !strcmp(cb->m_name.c_str(), name))
            return i;
    }
    CZBodyAnimCallback *cb = new CZBodyAnimCallback(name, t);

    if (m_callbacks.size() == m_callbacks.capacity())
        m_callbacks.reserve(m_callbacks.size() + 10);
    m_callbacks.push_back(cb);
    return m_callbacks.size() - 1;
}

/* the flag words' helpers (SOCOM II's function templates, so never inlined: pinned; called by
 * their placeholders until named) */
#pragma auto_inline off
extern "C" {

bool func_0028F190(unsigned short flags, unsigned short mask)
{
    return (flags & mask) != 0;
}

unsigned short func_0028F1B0(unsigned short flags, unsigned short mask, bool on)
{
    flags &= ~mask;
    if (on)
        flags |= mask;
    return flags;
}

}
#pragma auto_inline reset
