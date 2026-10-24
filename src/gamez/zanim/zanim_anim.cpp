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

/* in a running state, or with a particle source still going */
bool CZAnim::Running()
{
    bool running = ZAnim.StateRunning(m_state);

    if (!running)
        for (int i = 0; i < m_particle_count; i++)
            if (m_particle_list[i].particle && !m_particle_list[i].particle->Done())
                return true;
    return running;
}

/* SOCOM II's: the anim stopped, and the one it chains to */
extern "C" void func_00270960(CZAnim *anim, bool now)
{
    func_002707C0(anim, now);
    if (anim->m_sibling_anim)
        func_002707C0(anim->m_sibling_anim, now);
}

/* SOCOM II's: the anim it chains to (Resume and Pause walk the chain with it) */
extern "C" CZAnim *func_00270BB0(CZAnim *anim)
{
    return anim->m_sibling_anim;
}

/* CZAnim::GetRootNode, which SOCOM II gave a flag: look the root node up when it is not
 * known yet (its one caller passes 0; the type waits on another) */
extern "C" CNode *func_002703D0(CZAnim *anim, int find)
{
    if (anim->m_instance_node)
        return anim->m_instance_node;
    int root = anim->m_root_node_index;

    if (!root)
        return 0;
    if (!anim->m_node_ref_list[root].node_ptr && find)
        anim->GetNode(root, true);
    return anim->m_node_ref_list[anim->m_root_node_index].node_ptr;
}

/* one of the anim's special sequences (damage, activation, execution, cleanup -- an offset
 * into its sequence data) run for a frame: whether it finished */
bool CZAnim::ExecuteSpecialSequence(int offset)
{
    bool done = true;

    if (offset <= 0)
        return true;
    CZAnim *anim = ZAnim.m_CurAnim;
    _zsequence *seq = ZAnim.m_CurSeq;

    ZAnim.m_CurSeq = (_zsequence *)(m_seq_data + offset);
    ZAnim.m_CurAnim = this;
    ZAnim.SeqSetState(2);
    ZAnim.SeqTick(1.0f);
    if (ZAnim.SeqRunning())
        done = false;
    ZAnim.SeqSetState(1);
    ZAnim.m_CurAnim = anim;
    ZAnim.m_CurSeq = seq;
    return done;
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

/* the anim's lights put out: those it stops on exit, or all when forced -- a local light
 * deactivated, a world light's map deleted */
void CZAnim::StopLights(bool force)
{
    for (int i = 0; i < m_node_ref_count; i++) {
        _zanim_node_ref *ref = &m_node_ref_list[i];

        if (ref->is_light && ref->node_ptr && (ref->stop_on_exit || force)) {
            if (ref->is_local_light) {
                if (ref->stop_on_exit)
                    ref->node_ptr->SetActive(false);
            } else if (ref->on_world_lightlist) {
                zdb::CWorld::m_world->DeleteLightMap((zdb::CLightMap *)ref->node_ptr);
                ref->on_world_lightlist = 0;
                ref->stop_on_exit = 0;
            }
        }
    }
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
