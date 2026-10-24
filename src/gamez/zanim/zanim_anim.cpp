/* zanim_anim.cpp -- CZAnim, one anim: its nodes and references, sequences and state (OVL1
 * 0x26D370-...). The unit name is SOCOM 1's (__sinit_zanim_anim.cpp, which follows
 * anim_main.cpp's). Defined in reverse of their image order (-inline deferred).
 *
 * Inlining is bottom-up (-inline bottomup: a callee arrives with its own callees in it), and
 * -inline auto takes a function of at most 15 statements of MW's own count -- every statement
 * of its lowered body but the labels, the closing return included, measured before anything
 * is inlined into it (tools/autoprobe.py measures it). Resume and Pause inline
 * themselves twice and CZAnim::Running (10) into the first copy, then call -- which is where
 * the out-of-line copies of the sibling accessor (0x270BB0) and the Source test (0x270BC0)
 * come from. CZAnim::Start (11 by that count), StopAnims (12) and ExecuteSpecialSequence (15)
 * are never inlined (RestartClones and func_00271110; State; Tick and State), while
 * GetNodeRef (11) is, into GetNodes: no one limit separates them, so the original's three had
 * more statements than these -- debug code compiled to nothing still counts (`if (0) f();`
 * is two) -- or a pin. A pragma around each definition reproduces that (the pragma state at
 * the callee decides). */

#include <algorithm>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "zanim.h"

extern "C" CZAnim *func_00270BB0(CZAnim *anim);
extern "C" bool func_00270BC0(Particle::Source *source);
extern "C" CNode *func_002703D0(CZAnim *anim, int find);
extern "C" int func_00270460(CZAnim *anim, CNode *node);

/* the extension a weapon hit reaches the anim through: made for its health's node (else its own
 * node) unless that node has one already -- from the anim's health when it has one -- and
 * attached to the node and to the current set */
void CZAnim::OnWeaponHitInit()
{
    CNode *node = 0;

    if (m_health)
        node = GetNode(m_health->m_health_node_index, false);
    if (!node)
        node = GetRootNode();
    if (node && !node->m_nodeEx) {
        if (m_health)
            m_animEx = new CZAnimEx(this, node, m_health->shield, m_health->armor, m_health->health,
                                    GetRootNode());
        else
            m_animEx = new CZAnimEx(this, node, GetRootNode());
        if (m_animEx) {
            node->SetNodeEx(m_animEx, true);
            if (ZAnim.m_CurAnimSet)
                ZAnim.m_CurAnimSet->AddAnimEx(m_animEx);
        }
    }
}

/* SOCOM II's: alive, and its shield no more than the given amount (not named) */
extern "C" bool func_002739B0(CZAnimEx *ex, float amount)
{
    return ex->m_health > 0.0f && !(ex->m_shield > amount);
}

/* the list's extensions reset to full: an MSL for_each over the vector (its class iterators
 * kept in stack slots), the function object ours */
struct CZAnimExReset {
    void operator()(CZAnimEx *ex) const { ex->Reset(0, 0); }
};

void CZAnimExVector::Reset()
{
    std::for_each(begin(), end(), CZAnimExReset());
}

/* an extension added, the list grown by 15 when full */
void CZAnimExVector::Add(CZAnimEx *ex)
{
    if (size() == capacity())
        reserve(size() + 15);
    push_back(ex);
}

/* the list's extensions deleted, CZAnimEx's destructor inlined into each delete, and the list
 * emptied (the function object ours, as Reset's) */
struct CZAnimExDelete {
    void operator()(CZAnimEx *ex) const { delete ex; }
};

void CZAnimExVector::Free()
{
    std::for_each(begin(), end(), CZAnimExDelete());
    size_ = 0;
}

/* the extension's armour and health from the given values, or their maxima */
void CZAnimEx::Reset(float *armor, float *health)
{
    if (armor)
        m_armor = *armor;
    else
        m_armor = m_armor_max;
    if (health)
        m_health = *health;
    else
        m_health = m_health_max;
}

/* SOCOM II's: a health back to its maxima -- inlined but in CZAnim::Reset's innermost copy;
 * its name is not known */
extern "C" void func_00273960(CZAnimHealth *health)
{
    health->health = health->max_health;
    health->armor = health->max_armor;
}

/* the arguments' copies (Copy__12CZAnimArgCmdFv and its kind): a new argument of the same type
 * and value -- the command and valve kinds construct with their own type and are given the
 * original's after */
CZAnimArg *CZAnimArgCharP::Copy()
{
    return new CZAnimArgCharP(GetType(), m_str);
}

CZAnimArg *CZAnimArgNode::Copy()
{
    return new CZAnimArgNode(GetType(), m_node);
}

CZAnimArg *CZAnimArgValve::Copy()
{
    CZAnimArg *arg = new CZAnimArgValve(m_valve);

    arg->m_type = m_type;
    return arg;
}

CZAnimArg *CZAnimArgCmd::Copy()
{
    CZAnimArg *arg = new CZAnimArgCmd(m_cmd);

    arg->m_type = m_type;
    return arg;
}

CZAnimArg *CZAnimArgPnt3D::Copy()
{
    return new CZAnimArgPnt3D(m_type, &m_pnt);
}

CZAnimArg *CZAnimArgInt::Copy()
{
    return new CZAnimArgInt(GetType(), m_value);
}

/* find_if's test for the n-th argument of a type, kept by value in the walk (SOCOM II's
 * function object; the name ours) */
struct CZAnimArgFind {
    CZAnimArgFind(CZAnimArg::TYPE type, int n) : type(type), n(n), count(0) {}
    bool operator()(const void *p)
    {
        CZAnimArg *arg = (CZAnimArg *)p;

        if (arg->m_type == type) {
            if (count == n)
                return true;
            count++;
        }
        return false;
    }

    CZAnimArg::TYPE type;
    int n;
    int count;
};

/* the first point argument of a type.
 * OPEN, as the destructor below: the target keeps the found iterator (and the destructor's walk)
 * in a stack slot, copied bitwise out of a temporary, where <list>'s iterators here stay in
 * registers -- every shape tried (const_iterator, assignment, a pointer-list wrapper, a
 * destructor, free comparisons) either keeps that or stops the inlining */
CPnt3D *CZAnimArgList::GetPnt3D(CZAnimArg::TYPE type)
{
    iterator it = std::find_if(begin(), end(), CZAnimArgFind(type, 0));

    if (it == end())
        return 0;
    return &((CZAnimArgPnt3D *)*it)->m_pnt;
}

/* the arguments deleted with the list */
CZAnimArgList::~CZAnimArgList()
{
    for (iterator it = begin(); it != end(); ++it)
        delete (CZAnimArg *)*it;
    clear();
}

/* what SOCOM II's network activation adds after the packet: a value at 4 the anim gets as an
 * integer argument (not named) */
struct ZAnimNetworkExtra {
    int _0;
    int value;   // 0x4
};

/* SOCOM II's CZAnimMain::NetworkReceive (SOCOM 1's, given a third argument; not named): the
 * packet's anim found, an available copy given the packet's arguments and the extra value,
 * and activated with the activation callback held off; the packet's size.
 * OPEN, registers only (19 instructions): the target gives the extra argument s0 and the anim
 * s1, as if the anim were declared first -- declaration order, an outer `value`, and an inlined
 * helper (cut by the inline level) do not swap them */
extern "C" int func_00273780(CZAnimMain *main, ZAnimNetworkPacket *packet, ZAnimNetworkExtra *extra)
{
    CZAnim *anim = ZAnim.GetAnim(packet->anim_set_index, packet->anim_index);

    if (anim) {
        main->m_block_activation_callback = 1;
        anim = anim->GetAvailable();
        if (anim) {
            if (anim->m_arg_list)
                delete anim->m_arg_list;
            anim->m_arg_list = 0;
            if (packet->packet_size > sizeof(ZAnimNetworkPacket)) {
                if (!anim->m_arg_list)
                    anim->m_arg_list = new CZAnimArgList;
                anim->m_arg_list->ParseNetworkPacketArgs(anim, packet);
            }
            if (extra) {
                int value = extra->value;
                CZAnimArg *arg = new CZAnimArgInt(CZAnimArg::TYPE_INT, value);

                if (!anim->m_arg_list)
                    anim->m_arg_list = new CZAnimArgList;
                anim->m_arg_list->push_back(arg);
            }
            anim->Activate(0);
        }
        main->m_block_activation_callback = 0;
    }
    return packet->packet_size;
}

/* SOCOM II's: an available copy activated with commands (SOCOM 1's ActivateEx, its list read by
 * ActivateExV now) */
CZAnim *CZAnim::ActivateEx(int cmd, ...)
{
    va_list args;

    va_start(args, cmd);
    return ActivateExV(args);
}

/* activated with arguments -- none, or a list ActivateArgV reads */
CZAnim *CZAnim::ActivateArg(int type, ...)
{
    if (!type)
        return Activate(0);
    va_list args;

    va_start(args, type);
    return ActivateArgV(args);
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


/* health back to full (the chain's too, when it has health), the extension's from it, and the
 * anim reset to state 1. It inlines itself twice, the health and extension resets into both
 * outer copies, and calls them from the innermost. */
void CZAnim::Reset()
{
    if (m_health) {
        func_00273960(m_health);
        if (m_sibling_anim)
            m_sibling_anim->Reset();
    }
    if (m_animEx)
        m_animEx->Reset(&m_health->armor, &m_health->health);
    State(1);
}

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
        SaveNodeStates(false);
    if (m_state == 1 || m_state == 5) {
        if (m_activation == 1)
            Activate(0);
        else if (m_activation == 3)
            OnWeaponHitInit();
    }
}

#pragma auto_inline reset

/* an anim of the chain free to run: the first not running, else -- if it copies itself and
 * the copies are unlimited or fewer than its maximum -- a new copy chained on; none otherwise,
 * or when the anim is invalid */
CZAnim *CZAnim::GetAvailable()
{
    if (m_invalid)
        return 0;
    int count = 1;
    CZAnim *anim = this;

    while (anim->Running() && anim->m_sibling_anim) {
        anim = anim->m_sibling_anim;
        count++;
    }
    if (anim->Running()) {
        if (anim->m_auto_copy_anim) {
            if (m_max_anim_copies && count >= m_max_anim_copies)
                anim = 0;
            else {
                anim->m_sibling_anim = anim->Copy();
                anim = anim->m_sibling_anim;
            }
        } else
            anim = 0;
    }
    return anim;
}

/* the nodes under one named like the root: the root itself made this anim's root and started,
 * every other match a copy's -- chained after the last anim, unless the anim activates on a
 * weapon hit (3), which marks it instead -- and started; the new last returned */
CZAnim *CZAnim::FindClones(CNode *root, CNode *node, CZAnim *last)
{
    if (node->m_name && !strcmp(node->m_name, root->m_name)) {
        if (node == root) {
            func_00270460(this, root);
            Start();
        } else {
            if (m_activation != 3) {
                last->m_sibling_anim = last->Copy();
                last = last->m_sibling_anim;
            } else
                m_auto_copy_anim = 1;
            func_00270460(last, node);
            last->Start();
        }
    }
    for (zdb::CNodeVector::iterator it = node->m_child.begin(), end = node->m_child.end(); it != end; ++it)
        last = FindClones(root, *it, last);
    return last;
}

/* the anim cloned for every node in the world matching its root, chained after its last anim
 * (the walk to the last re-reads this anim's own link -- as SOCOM 1's does) */
void CZAnim::MakeClones()
{
    Refresh(false);
    CNode *root = func_002703D0(this, 0);

    if (root && root->m_name) {
        CZAnim *last = this;

        while (m_sibling_anim)
            last = m_sibling_anim;
        zdb::CNodeVector &children = zdb::CWorld::m_world->m_child;

        for (zdb::CNodeVector::iterator it = children.begin(), end = children.end(); it != end; ++it)
            last = FindClones(root, *it, last);
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
                if (root->m_socom2_a1 & 4)
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
 * still runs. Each inlines itself twice, Running into the first two copies (Running's Source
 * test inlined into the outer one only) -- bottom-up inlining's shape. Resume's flag is set
 * by if/else as Pause's: assigned straight from Running, the inlined result lands in the
 * variable unnormalised, and the return narrows it instead. */
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
 * out): SOCOM II's, inlined but in the innermost copies of Resume and Pause -- their names are
 * not known */
extern "C" CZAnim *func_00270BB0(CZAnim *anim)
{
    return anim->m_sibling_anim;
}

extern "C" bool func_002709B0(CZAnim *anim, int chain)
{
    bool running;

    anim->m_paused = 0;
    if (anim->Running())
        running = true;
    else
        running = false;
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

/* an argument's use of a node, by the argument list (0x2722A0; not named) */
extern "C" bool func_002722A0(CZAnimArgList *list, CNode *node);

/* the reference to a node, if the anim has one (inlined into func_00270570; the name ours) */
inline _zanim_node_ref *CZAnim::GetNodeRef(CNode *node)
{
    _zanim_node_ref *ref = 0;

    if (!node)
        return 0;
    for (int i = 0; !ref && i < m_node_ref_count; i++)
        if (m_node_ref_list[i].node_ptr == node)
            ref = &m_node_ref_list[i];
    return ref;
}

/* SOCOM II's: a node going away -- a running anim that references it (or whose arguments do)
 * stopped, its lights and particle sources let go, unless it is cleaning up already, when its
 * references to the node are only dropped; the chain likewise. Not named */
extern "C" void func_00270570(CZAnim *anim, CNode *node)
{
    if (anim->Running()) {
        if (anim->GetNodeRef(node) || (anim->m_arg_list && func_002722A0(anim->m_arg_list, node))) {
            if (!anim->m_in_cleanup) {
                func_002707C0(anim, true);
                anim->StopLights(true);
                for (int i = 0; i < anim->m_particle_count; i++) {
                    if (anim->m_particle_list[i].particle) {
                        delete anim->m_particle_list[i].particle;
                        anim->m_particle_list[i].particle = 0;
                    }
                }
            } else {
                for (int i = 0; i < anim->m_node_ref_count; i++) {
                    _zanim_node_ref *ref = &anim->m_node_ref_list[i];

                    if (ref->node_ptr == node)
                        ref->node_ptr = 0;
                }
            }
        }
    }
    if (anim->m_sibling_anim)
        func_00270570(anim->m_sibling_anim, node);
}


/* SOCOM II's: a node made the anim's root -- the pointers cleared, an instance node deleted,
 * the nodes looked up again; none without a node or a root reference */
extern "C" int func_00270460(CZAnim *anim, CNode *node)
{
    if (!node || !anim->m_root_node_index)
        return 0;
    for (int i = 0; i < anim->m_node_ref_count; i++) {
        if (i == anim->m_root_node_index && anim->m_clone_anim)
            continue;
        if (!anim->m_node_ref_list[i].created_here)
            anim->m_node_ref_list[i].node_ptr = 0;
    }
    if (anim->m_instance_node) {
        delete anim->m_instance_node;
        anim->m_instance_node = 0;
    }
    anim->m_node_ref_list[anim->m_root_node_index].node_ptr = node;
    anim->GetNodes();
    anim->CreateInstanceNode();
    return 1;
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

/* the anim's nodes that save a state (translation, rotation, activity, parent) handed to its
 * set to keep, refreshed first when asked */
void CZAnim::SaveNodeStates(bool refresh)
{
    if (refresh)
        Refresh(false);
    _zanim_node_ref *ref = m_node_ref_list;

    for (int i = 0; i < m_node_ref_count; i++, ref++) {
        bool save;

        if (ref->save_active || ref->save_parent || ref->save_rotation || ref->save_translation)
            save = true;
        else
            save = false;
        if (save && ref->node_ptr)
            ZAnim.GetAnimSet(m_animset_index)->SaveNodeStates(ref);
    }
}

/* the anim's instance of its root model, when it makes one: a copy of the model or a cross
 * reference to it, named <model>:zanim_instance, made the root reference and the nodes looked
 * up under it; the anim is then added to the world */
void CZAnim::CreateInstanceNode()
{
    if (!m_create_instance)
        return;
    if (m_instance_node)
        return;
    CNode *root = func_002703D0(this, 1);

    if (!root)
        return;
    zdb::CModel *model = dynamic_cast<zdb::CModel *>(root);

    if (!model)
        return;
    if (m_copy_instance)
        m_instance_node = zdb::CNode::CreateInstance(model, 0, 0);
    else
        m_instance_node = zdb::CNode::CreateXRef(model, 0, 0);
    if (m_instance_node) {
        char *name;
        char *src = model->m_name;

        name = (char *)_malloc(strlen(src) + 18, 0, 0);

        sprintf(name, "%s:zanim_instance", src);
        m_instance_node->SetName(name);
        func_00180280(name, 0, 0);
        int root_index = m_root_node_index;

        if (root_index) {
            m_node_ref_list[root_index].node_ptr = m_instance_node;
            m_node_ref_list[m_root_node_index].created_here = 1;
        }
        for (int i = 0; i < m_node_ref_count; i++) {
            if (i == m_root_node_index && m_clone_anim)
                continue;
            if (!m_node_ref_list[i].created_here)
                m_node_ref_list[i].node_ptr = 0;
        }
        GetNodes();
        if (root->m_socom2_a1 & 4) {
            CNode *instance = m_instance_node;

            instance->m_socom2_a1_bits.bit2 = 1;
            instance->m_bumpmap = 1;
        }
    }
    m_auto_add_to_world = 1;
}

/* the anim's nodes looked up: a root made first when the anim makes its own (named
 * <anim>::created_root), then the root, then every reference still without a node */
void CZAnim::GetNodes()
{
    if (m_create_root) {
        if (!m_root_node_index)
            m_root_node_index = AddNewNodeRef(0, 0, 0);
        _zanim_node_ref *ref = GetNodeRef(m_root_node_index);

        if (!ref->node_ptr) {
            char name[128];

            ref->node_ptr = new CNode;
            sprintf(name, "%s::created_root", GetName());
            ref->node_ptr->SetName(name);
            ref->name_index = m_nameindex_table.AddName(ZAnim.GetAnimSet(m_animset_index), name);
            ref->created_here = 1;
        }
    }
    int root = m_root_node_index;

    if (root)
        GetNode(root, true);
    for (int i = 0; i < m_node_ref_count; i++)
        if (!m_node_ref_list[i].node_ptr)
            GetNode(i, true);
    CreateInstanceNode();
}

/* an argument's node, by index: the argument list's (CZAnimArgList, 0x272390; not named) */
extern "C" CNode *func_00272390(CZAnimArgList *list, int index);

/* the anim's own node: its instance, else its root reference's node (SOCOM II's; inlined into
 * GetNode, the name ours) */
inline CNode *CZAnim::GetRootNode()
{
    if (m_instance_node)
        return m_instance_node;
    int root = m_root_node_index;

    if (!root)
        return 0;
    return m_node_ref_list[root].node_ptr;
}

/* the node a reference names, looked up when asked and not yet known -- under its parent's node,
 * else under the root's, else (unless it searches under a root it has) the world's model for an
 * instanced root or the database's node; world lights are found elsewhere. 250 is an argument's
 * node, 249 the instance node or the root's */
CNode *CZAnim::GetNode(int index, bool lookup)
{
    if (!index)
        return 0;
    if (index == 250)
        return m_arg_list ? func_00272390(m_arg_list, 250) : 0;
    if (index == 249)
        return GetRootNode();
    while (index >= m_node_ref_count)
        AddNewNodeRef(0, 0, 0);
    CNode *node = m_node_ref_list[index].node_ptr;

    if (lookup && !node && (!m_node_ref_list[index].is_light || m_node_ref_list[index].is_local_light)) {
        int parent_index = m_node_ref_list[index].parent_index;

        if (parent_index) {
            CNode *parent = GetNode(parent_index, lookup);

            if (parent)
                node = parent->FindChild(GetName(m_node_ref_list[index].name_index), true);
        } else {
            if (index != m_root_node_index) {
                CNode *root = GetNode(m_root_node_index, lookup);

                if (root)
                    node = root->FindChild(GetName(m_node_ref_list[index].name_index), true);
            }
            if (!node && !(m_node_ref_list[index].node_search == 1 && index != m_root_node_index
                           && (index == m_root_node_index || m_root_node_index))) {
                if (m_create_instance && index == m_root_node_index)
                    node = zdb::CWorld::GetModel(GetName(m_node_ref_list[index].name_index));
                else
                    node = ZAnim.GetDatabaseNode(GetName(m_node_ref_list[index].name_index),
                                                 m_node_ref_list[index].node_search);
            }
        }
        m_node_ref_list[index].node_ptr = node;
    }
    return node;
}

/* a node reference by its index, the references grown to reach it, its node looked up when
 * not known; none for 0 and 250 */
_zanim_node_ref *CZAnim::GetNodeRef(int index)
{
    if (!index)
        return 0;
    if (index == 250)
        return 0;
    while (index >= m_node_ref_count)
        AddNewNodeRef(0, 0, 0);
    _zanim_node_ref *ref = &m_node_ref_list[index];

    if (!ref->node_ptr)
        GetNode(index, false);
    return ref;
}

/* the anim's sounds let go: those it stops on exit faded out or stopped, the others left to
 * finish (or released when done); a sound with no instance stopped through the manager */
void CZAnim::StopSounds()
{
    for (int i = 0; i < m_sound_ref_count; i++) {
        if (m_sound_ref_list[i].sound_instance) {
            if (m_sound_ref_list[i].stop_on_exit) {
                if (m_sound_ref_list[i].fade > 0.0f) {
                    func_00346DD0(m_sound_ref_list[i].sound_instance, m_sound_ref_list[i].fade);
                    m_sound_ref_list[i].sound_instance->TerminateOnFinish();
                } else {
                    m_sound_ref_list[i].sound_instance->Stop();
                    CSndInstance::ReleaseSoundInstance(m_sound_ref_list[i].sound_instance);
                }
            } else if (!func_00346D70(m_sound_ref_list[i].sound_instance))
                CSndInstance::ReleaseSoundInstance(m_sound_ref_list[i].sound_instance);
            else
                m_sound_ref_list[i].sound_instance->TerminateOnFinish();
            m_sound_ref_list[i].sound_instance = 0;
        } else if (m_sound_ref_list[i].sound_handle && m_sound_ref_list[i].sound_handle->m_socom2_bit3) {
            void *manager = func_0034BB80();

            if (manager)
                func_0034A910(manager, 255, m_sound_ref_list[i].sound_handle);
        }
        m_sound_ref_list[i].stop_on_exit = 0;
    }
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

/* what the anim can rebuild let go: its arguments and user commands, its chain (garbage
 * collected when it clones itself and does not activate on a weapon hit, unloaded otherwise),
 * its particle list, the nodes it made for world lights, and its other lists */
int CZAnim::GarbageCollect()
{
    if (m_arg_list)
        delete m_arg_list;
    m_arg_list = 0;
    if (m_user_cmd) {
        func_00180280(m_user_cmd, 0, 0);
        m_user_cmd = 0;
    }
    if (m_sibling_anim) {
        if (m_clone_anim && m_activation != 3)
            m_sibling_anim->GarbageCollect();
        else {
            m_sibling_anim->Unload();
            func_00180280(m_sibling_anim, 0, 0);
            m_sibling_anim = 0;
        }
    }
    if (m_particle_list) {
        func_00180280(m_particle_list, 0, 0);
        m_particle_list = 0;
        m_particle_count = 0;
    }
    if (m_node_ref_list) {
        for (int i = 0; i < m_node_ref_count; i++) {
            if (m_node_ref_list[i].node_ptr && m_node_ref_list[i].is_light) {
                if (m_node_ref_list[i].on_world_lightlist)
                    zdb::CWorld::m_world->DeleteLightMap((zdb::CLightMap *)m_node_ref_list[i].node_ptr);
                if (m_node_ref_list[i].created_here) {
                    delete m_node_ref_list[i].node_ptr;
                    m_node_ref_list[i].node_ptr = 0;
                }
            }
        }
    }
    if (m_sound_ref_list) {
        func_00180280(m_sound_ref_list, 0, 0);
        m_sound_ref_list = 0;
        m_sound_ref_count = 0;
    }
    if (m_tex_ref_list) {
        func_00180280(m_tex_ref_list, 0, 0);
        m_tex_ref_list = 0;
        m_tex_ref_count = 0;
    }
    if (m_valve_ref_list) {
        func_00180280(m_valve_ref_list, 0, 0);
        m_valve_ref_list = 0;
        m_valve_ref_count = 0;
    }
    if (m_call_anim_ref_list) {
        func_00180280(m_call_anim_ref_list, 0, 0);
        m_call_anim_ref_list = 0;
        m_call_anim_ref_count = 0;
    }
    return 1;
}

void CZAnim::Priority(float priority)
{
    m_priority = priority;
}

/* SOCOM 1's Particle::Source::Create and CWorld::GetTexHandle, both matched in units of their
 * own under their addresses' names */
extern "C" Particle::Source *func_0032B120();
extern "C" CTexHandle *func_001EC940(const char *name);

/* a node reference by name under a parent, made when missing (ours; inline in both
 * branches of LoadNodeNames -- its returns are their jumps on) */
inline int CZAnim::FindNodeRef(const char *name, int parent, int &search)
{
    const char *expanded = ZAnimWildcardApply(name);
    int index = m_nameindex_table.AddName(ZAnim.GetAnimSet(m_animset_index), expanded);

    for (int i = 0; i < m_node_ref_count; i++)
        if (m_node_ref_list[i].name_index == index && m_node_ref_list[i].parent_index == parent)
            return i;
    return AddNewNodeRef(index, parent, search);
}

/* a node reference's index for a name -- or for a ZRDR path of them (its NAME, or itself), each
 * the child of the one before, a scope keyword among them setting the search -- under a
 * parent; INPUT_NODE and ROOT_NODE are 250 and 249.
 * OPEN: one saved register too many -- MW here strength-reduces the path walk's index (an i*8
 * register beside i), the target shifts i at each Item; an unsigned i, an unsigned Item index,
 * i hoisted and the continue as an else leave it. The array's count is unsigned (the target's
 * sltu) and the search is read late (by reference). */
int CZAnim::LoadNodeNames(_zrdr *rdr, const char *name, int parent, int search)
{
    if (!rdr && !name)
        return 0;
    if (!m_root_node_index && search == 1)
        search = 0;
    if (name) {
        if (!strcasecmp(name, "INPUT_NODE"))
            return 250;
        if (!strcasecmp(name, "ROOT_NODE"))
            return 249;
        return FindNodeRef(name, parent, search);
    }
    _zrdr *path = func_0032F0D0(rdr, "NAME");

    if (!path)
        path = rdr;
    if (path->type != 4)
        return 0;
    for (int i = 0; i < path->Count(); i++) {
        if (path->Item(i)->type != 3)
            break;
        if (ZAnim.GetNodeSearchScope(path->Item(i)->String(), &search))
            continue;
        if (!strcasecmp(path->Item(i)->String(), "INPUT_NODE"))
            return 250;
        if (!strcasecmp(path->Item(i)->String(), "ROOT_NODE"))
            return 249;
        parent = FindNodeRef(path->Item(i)->String(), parent, search);
    }
    return parent;
}

/* SOCOM II's: a ZRDR path's node reference, searched as the current anim searches */
extern "C" int func_0026E2D0(CZAnim *anim, _zrdr *rdr)
{
    return anim->LoadNodeNames(rdr, 0, 0, ZAnim.m_CurAnim->node_search_scope);
}

/* a node reference appended, its index returned: entry 0 is made first, empty; none when the
 * name or parent index will not fit its bitfield */
int CZAnim::AddNewNodeRef(int name_index, int parent_index, int search)
{
    if (m_node_ref_count == 0) {
        m_node_ref_list = (_zanim_node_ref *)_realloc(m_node_ref_list, (m_node_ref_count + 1) * sizeof(_zanim_node_ref), 0, 0);
        memset(&m_node_ref_list[m_node_ref_count], 0, sizeof(_zanim_node_ref));
        m_node_ref_list[m_node_ref_count].node_search = 0;
        m_node_ref_count++;
    }
    if (name_index > 2047)
        return 0;
    if (parent_index > 255)
        return 0;
    m_node_ref_list = (_zanim_node_ref *)_realloc(m_node_ref_list, (m_node_ref_count + 1) * sizeof(_zanim_node_ref), 0, 0);
    memset(&m_node_ref_list[m_node_ref_count], 0, sizeof(_zanim_node_ref));
    m_node_ref_list[m_node_ref_count].node_search = search;
    m_node_ref_list[m_node_ref_count].name_index = name_index;
    m_node_ref_list[m_node_ref_count].node_ptr = 0;
    m_node_ref_list[m_node_ref_count].parent_index = parent_index;
    m_node_ref_count++;
    return m_node_ref_count - 1;
}

/* a new call reference's anim looked up: none when there is no such anim (name ours; inline
 * in both GetCallAnimRefs -- its early return is their jump to the end) */
inline _zanim_call_anim_ref *CZAnim::ResolveCallAnim(_zanim_call_anim_ref *ref, int name_index)
{
    if (ref && !ref->base_anim) {
        ref->base_anim = ZAnim.GetAnim(GetName(name_index), 0);
        if (!ref->base_anim)
            return 0;
    }
    return ref;
}

/* the anim called by a name's index and its local name's, the reference made when missing */
_zanim_call_anim_ref *CZAnim::GetCallAnimRef(int name_index, int local_index)
{
    _zanim_call_anim_ref *ref = 0;

    for (int i = 0; i < m_call_anim_ref_count; i++)
        if (name_index == m_call_anim_ref_list[i].name_index && local_index == m_call_anim_ref_list[i].local_name_index)
            ref = &m_call_anim_ref_list[i];
    if (!ref) {
        m_call_anim_ref_list = (_zanim_call_anim_ref *)_realloc(m_call_anim_ref_list, (m_call_anim_ref_count + 1) * sizeof(_zanim_call_anim_ref), 0, 0);
        ref = &m_call_anim_ref_list[m_call_anim_ref_count];
        memset(ref, 0, sizeof(_zanim_call_anim_ref));
        ref->name_index = name_index;
        ref->local_name_index = local_index;
        m_call_anim_ref_count++;
        ref = ResolveCallAnim(ref, name_index);
    }
    return ref;
}

/* the anim called by a name's index (the last reference by local name, else by name); a new
 * reference gets its anim looked up, and is none when there is no such anim */
_zanim_call_anim_ref *CZAnim::GetCallAnimRef(int name_index)
{
    _zanim_call_anim_ref *ref = 0;

    for (int i = 0; i < m_call_anim_ref_count; i++)
        if (name_index == m_call_anim_ref_list[i].local_name_index)
            ref = &m_call_anim_ref_list[i];
    if (!ref)
        for (int i = 0; i < m_call_anim_ref_count; i++)
            if (name_index == m_call_anim_ref_list[i].name_index)
                ref = &m_call_anim_ref_list[i];
    if (!ref) {
        m_call_anim_ref_list = (_zanim_call_anim_ref *)_realloc(m_call_anim_ref_list, (m_call_anim_ref_count + 1) * sizeof(_zanim_call_anim_ref), 0, 0);
        ref = &m_call_anim_ref_list[m_call_anim_ref_count];
        memset(ref, 0, sizeof(_zanim_call_anim_ref));
        ref->name_index = name_index;
        ref->local_name_index = 0;
        m_call_anim_ref_count++;
        ref = ResolveCallAnim(ref, name_index);
    }
    return ref;
}

/* the sound by its name's index (the last reference to it), the reference made -- with its
 * handle -- when missing; none when the sound is not found */
_zanim_sound *CZAnim::GetSoundRef(int name_index)
{
    _zanim_sound *ref = 0;

    for (int i = 0; i < m_sound_ref_count; i++)
        if (name_index == m_sound_ref_list[i].name_index)
            ref = &m_sound_ref_list[i];
    if (!ref) {
        m_sound_ref_list = (_zanim_sound *)_realloc(m_sound_ref_list, (m_sound_ref_count + 1) * sizeof(_zanim_sound), 0, 0);
        ref = &m_sound_ref_list[m_sound_ref_count];
        memset(ref, 0, sizeof(_zanim_sound));
        ref->fade = -0.1f;
        ref->name_index = name_index;
        ref->sound_handle = CSnd::GetSoundByName(GetName(name_index));
        m_sound_ref_count++;
    }
    if (ref && ref->sound_handle)
        return ref;
    return 0;
}

/* the particle source by its name's index, made with its reference when missing */
Particle::Source *CZAnim::GetParticleSource(int name_index)
{
    _zanim_particle *ref = 0;
    Particle::Source *source = 0;

    for (int i = 0; i < m_particle_count; i++) {
        if (name_index == m_particle_list[i].name_index) {
            ref = &m_particle_list[i];
            break;
        }
    }
    if (!ref) {
        m_particle_list = (_zanim_particle *)_realloc(m_particle_list, (m_particle_count + 1) * sizeof(_zanim_particle), 0, 0);
        ref = &m_particle_list[m_particle_count];
        memset(ref, 0, sizeof(_zanim_particle));
        ref->name_index = name_index;
        ref->particle = 0;
        m_particle_count++;
    }
    if (ref) {
        if (!ref->particle)
            ref->particle = func_0032B120();
        source = ref->particle;
    }
    return source;
}

/* the texture by its name's index (the last reference to it), its handle found on first use */
CTexHandle *CZAnim::GetTexture(int name_index)
{
    _zanim_tex_ref *ref = 0;

    for (int i = 0; i < m_tex_ref_count; i++)
        if (name_index == m_tex_ref_list[i].name_index)
            ref = &m_tex_ref_list[i];
    if (!ref) {
        m_tex_ref_list = (_zanim_tex_ref *)_realloc(m_tex_ref_list, (m_tex_ref_count + 1) * sizeof(_zanim_tex_ref), 0, 0);
        ref = &m_tex_ref_list[m_tex_ref_count];
        memset(ref, 0, sizeof(_zanim_tex_ref));
        ref->name_index = name_index;
        ref->texh = 0;
        m_tex_ref_count++;
    }
    if (ref && !ref->texh)
        ref->texh = func_001EC940(GetName(name_index));
    return ref->texh;
}

/* the valve by its name's index: its reference made when it has none, the valve created on
 * first use */
CValve *CZAnim::GetValve(int name_index)
{
    _zanim_valve_ref *ref = 0;
    CValve *valve = 0;

    for (int i = 0; i < m_valve_ref_count; i++) {
        if (name_index == m_valve_ref_list[i].name_index) {
            ref = &m_valve_ref_list[i];
            break;
        }
    }
    if (!ref) {
        m_valve_ref_list = (_zanim_valve_ref *)_realloc(m_valve_ref_list, (m_valve_ref_count + 1) * sizeof(_zanim_valve_ref), 0, 0);
        ref = &m_valve_ref_list[m_valve_ref_count];
        memset(ref, 0, sizeof(_zanim_valve_ref));
        ref->name_index = name_index;
        ref->valve = 0;
        m_valve_ref_count++;
    }
    if (ref) {
        if (!ref->valve)
            ref->valve = CValve::Create(GetName(ref->name_index), (VALVE_TYPE)2);
        valve = ref->valve;
    }
    return valve;
}

/* SOCOM II's: a name run through the wildcard expansion first */

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
    const char *expanded = ZAnimWildcardApply(name);

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

/* everything the anim holds let go: its chain first, its particle sources, arguments and health,
 * the nodes it made (a world light's map taken off first), every list, its sequences, and the
 * instance node */
int CZAnim::Unload()
{
    if (m_sibling_anim) {
        m_sibling_anim->Unload();
        func_00180280(m_sibling_anim, 0, 0);
        m_sibling_anim = 0;
    }
    func_002707C0(this, false);
    for (int i = 0; i < m_particle_count; i++) {
        if (m_particle_list[i].particle) {
            delete m_particle_list[i].particle;
            m_particle_list[i].particle = 0;
        }
    }
    m_animEx = 0;
    if (m_arg_list)
        delete m_arg_list;
    m_arg_list = 0;
    if (m_health) {
        delete m_health;
        m_health = 0;
    }
    if (m_node_ref_list) {
        for (int i = 0; i < m_node_ref_count; i++) {
            if (m_node_ref_list[i].node_ptr) {
                if (m_node_ref_list[i].is_light && m_node_ref_list[i].on_world_lightlist)
                    zdb::CWorld::m_world->DeleteLightMap((zdb::CLightMap *)m_node_ref_list[i].node_ptr);
                if (m_node_ref_list[i].created_here) {
                    if (m_node_ref_list[i].node_ptr == m_instance_node)
                        m_instance_node = 0;
                    delete m_node_ref_list[i].node_ptr;
                }
            }
        }
        func_00180280(m_node_ref_list, 0, 0);
        m_node_ref_list = 0;
        m_node_ref_count = 0;
    }
    if (m_sound_ref_list) {
        func_00180280(m_sound_ref_list, 0, 0);
        m_sound_ref_list = 0;
        m_sound_ref_count = 0;
    }
    if (m_tex_ref_list) {
        func_00180280(m_tex_ref_list, 0, 0);
        m_tex_ref_list = 0;
        m_tex_ref_count = 0;
    }
    if (m_call_anim_ref_list) {
        func_00180280(m_call_anim_ref_list, 0, 0);
        m_call_anim_ref_list = 0;
        m_call_anim_ref_count = 0;
    }
    if (m_valve_ref_list) {
        func_00180280(m_valve_ref_list, 0, 0);
        m_valve_ref_list = 0;
        m_valve_ref_count = 0;
    }
    if (m_particle_list) {
        func_00180280(m_particle_list, 0, 0);
        m_particle_list = 0;
        m_particle_count = 0;
    }
    if (m_user_cmd) {
        func_00180280(m_user_cmd, 0, 0);
        m_user_cmd = 0;
    }
    if (m_seq_data)
        func_00180280(m_seq_data, 0, 0);
    m_seq_data = 0;
    m_seq_data_size = 0;
    if (m_instance_node) {
        delete m_instance_node;
        m_instance_node = 0;
    }
    m_nameindex_table.Unload();
    return 1;
}

/* the anim from its ZAR key: its parameters, health (made when missing), name index table,
 * node references (their nodes looked up later) and sequence data */
int CZAnim::Load(zar::CZAR &archive)
{
    int count;
    zar::CKey *key = archive.GetOpenKey();

    archive.Fetch("Anim_Params", (_zanim_anim_params *)this, sizeof(_zanim_anim_params));
    State(1);
    if (key->FindKey("Anim_Health")) {
        if (!m_health) {
            m_health = new CZAnimHealth;
            memset(m_health, 0, sizeof(CZAnimHealth));
            m_health->m_health_node_index = m_root_node_index;
        }
        archive.Fetch("Anim_Health", m_health, sizeof(CZAnimHealth));
    }
    m_nameindex_table.Load(archive);
    archive.Fetch("Node_Ref_Count", &count, 4);
    m_node_ref_count = count;
    if (count > 0) {
        int size = m_node_ref_count * sizeof(_zanim_node_ref);

        m_node_ref_list = (_zanim_node_ref *)_malloc(size, 0, 0);
        archive.Fetch("Node_Ref_List", m_node_ref_list, size);
        for (int i = 0; i < m_node_ref_count; i++)
            m_node_ref_list[i].node_ptr = 0;
    } else
        m_node_ref_list = 0;
    archive.Fetch("Seq_Data_Size", &count, 4);
    m_seq_data_size = count;
    if (count > 0) {
        m_seq_data = (char *)_malloc(m_seq_data_size, 0, 0);
        archive.Fetch("Seq_Data", m_seq_data, m_seq_data_size);
    } else
        m_seq_data = 0;
    return 1;
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
