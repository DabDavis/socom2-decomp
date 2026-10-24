/* zanim_anim.cpp -- CZAnim, one anim: its nodes and references, sequences and state (OVL1
 * 0x26D370-...). The unit name is SOCOM 1's (__sinit_zanim_anim.cpp, which follows
 * anim_main.cpp's). Defined in reverse of their image order (-inline deferred).
 *
 * Nothing here is inlined automatically: CZAnim::Start (0xA8 bytes) is called at all four of
 * its sites in the unit, and an 8-byte sibling accessor (0x270BB0) at all four of its own, on
 * both sides of it in the image -- where -inline auto inlines a 0x124-byte OpenZAR elsewhere.
 * The unit turns auto-inlining off. */

#include "zanim.h"

#pragma auto_inline off

/* SOCOM II's: a name run through the wildcard expansion (0x268520) first */
extern "C" const char *func_00268520(const char *name);

/* GetName(int)'s body, written out: the index read before the set is fetched */
const char *CZAnim::GetName()
{
    int index = m_name_index;
    CZAnimSet *set = ZAnim.GetAnimSet(m_animset_index);

    return m_nameindex_table.GetName(set, index);
}

const char *CZAnim::GetName(int index)
{
    CZAnimSet *set = ZAnim.GetAnimSet(m_animset_index);

    return m_nameindex_table.GetName(set, index);
}

extern "C" int func_0026ED00(CZAnim *anim, const char *name)
{
    const char *expanded = func_00268520(name);

    return anim->m_nameindex_table.AddName(ZAnim.GetAnimSet(anim->m_animset_index), expanded);
}

int CZAnim::AddName(const char *name)
{
    return m_nameindex_table.AddName(ZAnim.GetAnimSet(m_animset_index), name);
}

int CZAnim::AddValve(const char *name)
{
    return m_nameindex_table.AddName(ZAnim.GetAnimSet(m_animset_index), name);
}

/* the interpreter's sequence calls made on this anim: SOCOM II keeps the current sequence as
 * well as the current anim around them (SOCOM 1 cleared it) */
void CZAnim::ActivateSequence(const char *name)
{
    _zsequence *seq = ZAnim.m_CurSeq;
    CZAnim *anim = ZAnim.m_CurAnim;

    ZAnim.m_CurSeq = 0;
    ZAnim.m_CurAnim = this;
    ZAnim.ActivateSequence(name);
    ZAnim.m_CurAnim = anim;
    ZAnim.m_CurSeq = seq;
}

void CZAnim::StopSequence(const char *name)
{
    _zsequence *seq = ZAnim.m_CurSeq;
    CZAnim *anim = ZAnim.m_CurAnim;

    ZAnim.m_CurSeq = 0;
    ZAnim.m_CurAnim = this;
    ZAnim.StopSequence(name);
    ZAnim.m_CurAnim = anim;
    ZAnim.m_CurSeq = seq;
}

void CZAnim::PauseSequence(const char *name)
{
    _zsequence *seq = ZAnim.m_CurSeq;
    CZAnim *anim = ZAnim.m_CurAnim;

    ZAnim.m_CurSeq = 0;
    ZAnim.m_CurAnim = this;
    ZAnim.PauseSequence(name);
    ZAnim.m_CurAnim = anim;
    ZAnim.m_CurSeq = seq;
}

void CZAnim::ResumeSequence(const char *name)
{
    _zsequence *seq = ZAnim.m_CurSeq;
    CZAnim *anim = ZAnim.m_CurAnim;

    ZAnim.m_CurSeq = 0;
    ZAnim.m_CurAnim = this;
    ZAnim.ResumeSequence(name);
    ZAnim.m_CurAnim = anim;
    ZAnim.m_CurSeq = seq;
}

bool CZAnim::SequenceExists(const char *name)
{
    _zsequence *seq = ZAnim.m_CurSeq;
    CZAnim *anim = ZAnim.m_CurAnim;

    ZAnim.m_CurSeq = 0;
    ZAnim.m_CurAnim = this;
    bool r = ZAnim.SequenceExists(name);
    ZAnim.m_CurAnim = anim;
    ZAnim.m_CurSeq = seq;
    return r;
}

bool CZAnim::SequenceRunning(const char *name)
{
    _zsequence *seq = ZAnim.m_CurSeq;
    CZAnim *anim = ZAnim.m_CurAnim;

    ZAnim.m_CurSeq = 0;
    ZAnim.m_CurAnim = this;
    bool r = ZAnim.SequenceRunning(name);
    ZAnim.m_CurAnim = anim;
    ZAnim.m_CurSeq = seq;
    return r;
}

/* SOCOM II's: the node states of the anim's saved refs kept by its set (0x26F120) */
extern "C" void func_0026F120(CZAnim *anim, bool refresh);

/* the node and valve pointers looked up afresh -- but a cloning anim's root and the nodes the
 * anim made -- and the chain's too when asked */
void CZAnim::Refresh(bool chain)
{
    for (int i = 0; i < m_node_ref_count; i++) {
        if (i == m_root_node_index && m_clone_anim)
            continue;
        if (!m_node_ref_list[i].created_here)
            m_node_ref_list[i].node_ptr = 0;
    }
    GetNodes();
    for (int i = 0; i < m_valve_ref_count; i++)
        m_valve_ref_list[i].valve = 0;
    if (chain && m_sibling_anim)
        m_sibling_anim->Refresh(chain);
}

/* refreshed, its node states saved the first time its set starts, and activated when it
 * activates on start (1) or on a weapon hit (3) */
void CZAnim::Start()
{
    Refresh(false);
    if (!ZAnim.GetAnimSet(m_animset_index)->m_WasStarted)
        func_0026F120(this, false);
    if (m_state == 1 || m_state == 5) {
        if (m_activation == 1)
            Activate(0);
        else if (m_activation == 3)
            OnWeaponHitInit();
    }
}

/* the anim and its chain started again, unless it activates on demand (3) */
void CZAnim::RestartClones()
{
    if (m_activation == 3)
        return;
    for (CZAnim *anim = this; anim; anim = anim->m_sibling_anim)
        anim->Start();
}

void CZAnim::DeleteParticleSources()
{
    for (int i = 0; i < m_particle_count; i++) {
        if (m_particle_list[i].particle) {
            delete m_particle_list[i].particle;
            m_particle_list[i].particle = 0;
        }
    }
}

void CZAnim::Priority(float priority)
{
    m_priority = priority;
}
