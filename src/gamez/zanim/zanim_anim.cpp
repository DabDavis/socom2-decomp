/* zanim_anim.cpp -- CZAnim, one anim: its nodes and references, sequences and state (OVL1
 * 0x26D370-...). The unit name is SOCOM 1's (__sinit_zanim_anim.cpp, which follows
 * anim_main.cpp's). Defined in reverse of their image order (-inline deferred).
 *
 * -inline auto works here as elsewhere, to depth 3: Resume and Pause inline themselves twice
 * and CZAnim::Running twice, then call both -- which is where the out-of-line copies of the
 * sibling accessor (0x270BB0) and the Source test (0x270BC0) come from. CZAnim::Start,
 * ExecuteSpecialSequence and StopAnims are never inlined (RestartClones and func_00271110; Tick
 * and State; State), Start no bigger than Running and none in a call cycle; a pragma around each
 * definition reproduces that (the pragma state at the callee decides). The original's means is
 * unknown -- debug code compiled to nothing would count against -inline auto the same way. */

#include "zanim.h"

extern "C" CZAnim *func_00270BB0(CZAnim *anim);
extern "C" bool func_00270BC0(Particle::Source *source);
extern "C" CNode *func_002703D0(CZAnim *anim, int find);

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
#pragma auto_inline off
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

#pragma auto_inline reset

/* the anim and its chain started again, unless it activates on demand (3) */
void CZAnim::RestartClones()
{
    if (m_activation == 3)
        return;
    for (CZAnim *anim = this; anim; anim = anim->m_sibling_anim)
        anim->Start();
}

/* SOCOM II's: a particle source stopped by the particle manager (0x329AB0), and a world call on
 * a node it held (0x31D0A0) -- not named */
extern "C" void func_00329AB0(Particle::Source *source);
extern "C" void func_0031D0A0(zdb::CWorld *world, int what, zdb::CNode *node);

/* a new state: stopping (5) a running anim runs its cleanup; stopping or resetting (1) lets go
 * of its sources, sounds, lights and called anims and takes a root it added out of the world;
 * starting (2) unpauses it. The cleanup runs once, then the done callback, if any. */
void CZAnim::State(int state)
{
    bool cleanup = false;

    switch (state) {
    case 5:
        if (Running())
            cleanup = true;
    case 1:
        for (int i = 0; i < m_particle_count; i++)
            if (m_particle_list[i].particle && !func_00270BC0(m_particle_list[i].particle))
                func_00329AB0(m_particle_list[i].particle);
        StopSounds();
        StopLights(false);
        StopAnims();
        if (m_added_to_world_internally) {
            CNode *root = func_002703D0(this, 0);

            if (root) {
                zdb::CWorld::m_world->DeleteChild(root);
                m_added_to_world_internally = 0;
                if (((unsigned char *)root)[0xA1] & 4)
                    func_0031D0A0(zdb::CWorld::m_world, 2, root);
            }
        }
    case 2:
        m_paused = 0;
        if (state == 2)
            m_in_cleanup = 0;
        break;
    }
    if (cleanup && !m_in_cleanup) {
        m_in_cleanup = 1;
        ExecuteSpecialSequence(m_cleanup_seq_offset);
        if (m_done_callback)
            m_done_callback(this, m_done_arg);
        m_done_callback = 0;
        m_done_arg = 0;
        m_in_cleanup = 0;
    }
    m_state = state;
}

/* a frame of the anim: its execution sequence, then (once activated) every sequence while it
 * neither stops nor pauses; finished when no sequence is left running */
bool CZAnim::Tick(float dT)
{
    if (m_paused)
        return false;
    CZAnim *cur = ZAnim.m_CurAnim;
    _zsequence *seq = ZAnim.m_CurSeq;

    ZAnim.m_anim_complete = 1;
    ZAnim.m_CurAnim = this;
    ZAnim.m_CurSeq = 0;
    if (ExecuteSpecialSequence(m_execution_seq_offset)) {
        if (m_state == 2)
            ZAnim.ActivateBegin(this);
        for (ZAnim.SeqFirst(); ZAnim.SeqValid() && m_state != 5 && (bool)m_paused == false; ZAnim.SeqNext()) {
            ZAnim.SeqTick(dT);
            if (ZAnim.SeqRunning())
                ZAnim.m_anim_complete = 0;
        }
        m_timer += dT;
        if (ZAnim.m_anim_complete && (bool)m_paused == false)
            State(5);
        ZAnim.m_CurAnim = 0;
        ZAnim.m_CurSeq = 0;
    } else
        ZAnim.m_anim_complete = 0;
    ZAnim.m_CurAnim = cur;
    ZAnim.m_CurSeq = seq;
    return ZAnim.m_anim_complete;
}

/* in a running state, or with a particle source still going */
bool CZAnim::Running()
{
    bool running = ZAnim.StateRunning(m_state);

    if (!running)
        for (int i = 0; i < m_particle_count; i++)
            if (m_particle_list[i].particle && !func_00270BC0(m_particle_list[i].particle))
                return true;
    return running;
}

/* CZAnim::Pause and Resume, which SOCOM II gave a flag (CZAnimMain's handlers pass 0): the
 * anim, and its chain when asked, paused (if running) or resumed; whether any was paused, or
 * still runs.
 * OPEN, both: the target inlines each into itself twice and Running into the first copy (its
 * Source test called there, depth 3) -- MW here inlines the recursion three deep and calls
 * Running in every copy, member function or not. */
extern "C" bool func_00270BE0(CZAnim *anim, int chain)
{
    bool paused;

    if (anim->Running()) {
        anim->m_paused = 1;
        paused = true;
    } else
        paused = false;
    if (chain && func_00270BB0(anim))
        paused |= func_00270BE0(func_00270BB0(anim), chain);
    return paused;
}

extern "C" bool func_00270BC0(Particle::Source *source)
{
    return !(source->m_active || source->m_shuttingoff);
}

/* the anim it chains to, and a particle source that has stopped (neither emitting nor fading
 * out): SOCOM II's, inlined but where Resume and Pause have inlined themselves to depth 3 --
 * their names are not known */
extern "C" CZAnim *func_00270BB0(CZAnim *anim)
{
    return anim->m_sibling_anim;
}

extern "C" bool func_002709B0(CZAnim *anim, int chain)
{
    anim->m_paused = 0;
    bool running = anim->Running();

    if (chain && func_00270BB0(anim))
        running |= func_002709B0(func_00270BB0(anim), chain);
    return running;
}

/* SOCOM II's: the anim stopped, and the one it chains to */
extern "C" void func_00270960(CZAnim *anim, bool now)
{
    func_002707C0(anim, now);
    if (anim->m_sibling_anim)
        func_002707C0(anim->m_sibling_anim, now);
}

/* SOCOM II's: a particle source handed to the particle manager at 0x488480 -- not named */
extern "C" void func_00329AA0(Particle::Source *source);

/* CZAnim::Stop, which SOCOM II gave a flag: a running anim taken off its set's scheduler and
 * stopped (state 5), its particle sources let go at once when asked */
extern "C" void func_002707C0(CZAnim *anim, bool now)
{
    int i;

    if (!anim->Running())
        return;
    CZAnim *cur = ZAnim.m_CurAnim;
    _zsequence *seq = ZAnim.m_CurSeq;

    ZAnim.GetAnimSet(anim->m_animset_index)->TaskScheduler()->RemoveTask(anim, false);
    ZAnim.m_CurAnim = anim;
    ZAnim.m_CurSeq = 0;
    anim->State(5);
    if (now)
        for (i = 0; i < anim->m_particle_count; i++)
            if (anim->m_particle_list[i].particle)
                func_00329AA0(anim->m_particle_list[i].particle);
    ZAnim.m_CurAnim = cur;
    ZAnim.m_CurSeq = seq;
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

void CZAnim::DeleteParticleSources()
{
    for (int i = 0; i < m_particle_count; i++) {
        if (m_particle_list[i].particle) {
            delete m_particle_list[i].particle;
            m_particle_list[i].particle = 0;
        }
    }
}

/* the anims it called let go: those it stops on exit stopped first, if still running */
#pragma auto_inline off
void CZAnim::StopAnims()
{
    for (int i = 0; i < m_call_anim_ref_count; i++) {
        _zanim_call_anim_ref *ref = &m_call_anim_ref_list[i];

        if (ref->executing_anim) {
            if (ref->stop_on_exit) {
                if (ref->executing_anim->Running())
                    func_002707C0(ref->executing_anim, false);
                ref->stop_on_exit = 0;
            }
            ref->executing_anim = 0;
        }
    }
}
#pragma auto_inline reset

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

void CZAnim::Priority(float priority)
{
    m_priority = priority;
}

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

/* one of the anim's special sequences (damage, activation, execution, cleanup -- an offset
 * into its sequence data) run for a frame: whether it finished */
#pragma auto_inline off
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
#pragma auto_inline reset
