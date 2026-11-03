/* ai_group.cpp -- SOCOM II's scripted AI groups, CAiGroup, with its actions act_camera, act_zanim,
 * act_anim and act_sound (the class names their RTTIs'; no SOCOM 1 counterpart). OVL2 0x60C190 to
 * ~0x60EDFF, after SEAL_AI_PARAMS's constructor and its copy 0x60C170 (seal_aiparams.cpp) and before
 * the vehicle state (seal_aivehicle.cpp); the unit's name is ours.
 *
 * A group is the scripted ai::GROUP command's (CTGroup::Begin, seal_tasks.cpp, builds it): its member
 * AIs (a vector), a cell each may be sent to, and a list of steps, each a list of actions run
 * together -- the step done once every action is (2), the next taken after. Groups live in a global
 * list ticked once a frame (TickAll, a remove_if dropping the finished ones). Many of the unit's
 * small functions are byte-exact drafts (the actions' destructors among them). Defined in reverse of
 * their image order (-inline deferred). */

#include <list>
#include <vector>
#include <algorithm>

#include "../gamez/zmath/zmath.h"
#include "../gamez/zentity/zentity.h"
#include "../gamez/zanim/zanim.h"
#include "../gamez/zsnd/zsnd.h"
#include "ai_states.h"

class CAiGroup;

/* the AI controller as far as this unit reads it (zseal.h's CSealCtrlAi, which this unit does not
 * include): its entity (CEntityCtrl's), the bits at 0x264, its state stack, its group's id and the
 * three throttle points a group's member starts from -- and its group members, which this unit
 * defines (SetGroup, InGroup, GetGroup: OVL2 0x60E6F0, 0x60E6E0, 0x60E7A0; the names ours) */
class CSealCtrlAi : public CEntityCtrl {
public:
    bool PopToAiState(AI_STATE state, bool inclusive);
    void StopThrottles(bool keep);
    void SetGroup(CAiGroup *g);
    bool InGroup();
    CAiGroup *GetGroup();

    unsigned char _18[0x264 - 0x18];
    unsigned int m_aibits : 19;   // 0x264 (zseal.h's SEAL_AI_PARAMS bits)
    unsigned int m_aibit19 : 1;   // bit 19 (the byte at 0x266, bit 3): set while not in a group's camera
    unsigned int m_aibit20 : 1;
    unsigned int m_aibit21 : 1;   // bit 21 (0x266 bit 5)
    unsigned int m_aibits22 : 10;
    unsigned char _268[0x324 - 0x268];
    unsigned char m_324[0x41c - 0x324];   // 0x324: an object 0x558590 resets
    CAiStateStack m_stateStack;   // 0x41c
    int m_group;   // 0x43c: the group's id, or 0
    unsigned char _440[0x780 - 0x440];
    CPnt3D m_780;   // 0x780
    CPnt3D m_78c;   // 0x78c
    CPnt3D m_798;   // 0x798
};

extern "C" {
/* the AI's anim of a type played, with a mode (11, or 7 with a blend), whether it still plays, and
 * its stop (OVL2 0x571150, 0x5712F0, 0x571100: CSealCtrlAi's, seal_ai.cpp's range; unwritten) */
void func_00571150(CSealCtrlAi *ai, short anim, int mode);
bool func_005712F0(CSealCtrlAi *ai, short anim);
void func_00571100(CSealCtrlAi *ai, short anim);
/* the stack's bits brought up to date (OVL2 0x5F7470, seal_aistate.cpp's, unwritten), the object at
 * the AI's 0x324 reset (0x558590), an AI function (0x5604C0) and CSealCtrl::ClearLook (0x600D40,
 * s1:order) -- the SEAL lane's */
void func_005F7470(CAiStateStack *stack, CSealCtrlAi *ai, int x);
void func_00558590(void *p);
void func_005604C0(CSealCtrlAi *ai);
void func_00600D40(CSealCtrlAi *ai, signed char look);
}

/* the look a group clears and sets (OVL2 data 0x651130, 4: a byte the target loads with lb; the name
 * ours) */
extern signed char data_00651130;

/* a group (the names ours) */
class CAiGroup {
public:
    /* what a step runs: one action of a member, or the group's (member -1), started once the
     * step's clock reaches its time (the class name its RTTI's; slot +8 the destructor, +0xC the
     * tick, +0x10 the stop) */
    class action {
    public:
        enum STATE { WAITING = 0, RUNNING = 1, DONE = 2, STATE_LAST = 255 };

        action(int member, float time) : m_member(member), m_time(time), m_state(WAITING) {}
        virtual ~action() { Stop(); }
        /* the base's: done at once (0x60C3D0) */
        virtual STATE Tick(CSealCtrlAi *ai, float t)
        {
            m_state = DONE;
            return m_state;
        }
        /* the base's: done (0x60C210) */
        virtual void Stop() { m_state = DONE; }

        int m_member;   // 0x4: the member's index, or -1
        float m_time;   // 0x8: when the action starts on the step's clock
        STATE m_state;   // 0xc
    };

    /* a camera move (0x30 bytes; the name its RTTI's) */
    /* a camera point: a position and how to read it (the names ours) */
    struct campoint {
        CPnt3D m_pos;   // 0x0
        unsigned int m_flags;   // 0xc
    };

    /* a camera move (0x30 bytes; the name its RTTI's): the group's alone */
    class act_camera : public action {
    public:
        act_camera(const campoint &from, const campoint &to, float time)
            : action(-1, time), m_from(from), m_to(to) {}
        virtual STATE Tick(CSealCtrlAi *ai, float t);

        campoint m_from;   // 0x10
        campoint m_to;   // 0x20
    };

    /* a ZAnim activated (0x14 bytes) */
    class act_zanim : public action {
    public:
        act_zanim(int member, CZAnim *anim, float time) : action(member, time), m_anim(anim) {}
        virtual STATE Tick(CSealCtrlAi *ai, float t);

        CZAnim *m_anim;   // 0x10
    };

    /* a member's anim of a type played (0x1C bytes): stopped after its length when it has one */
    class act_anim : public action {
    public:
        act_anim(int member, short anim, float length, float time)
            : action(member, time), m_ai(0), m_length(length), m_anim(anim) {}
        virtual STATE Tick(CSealCtrlAi *ai, float t);
        virtual void Stop();

        /* a length of its own (an inline: the add after it loads the length again) */
        bool Timed() const { return m_length > 0.0f; }

        CSealCtrlAi *m_ai;   // 0x10: the member playing it
        float m_length;   // 0x14: how long it plays, or 0 for its own length
        short m_anim;   // 0x18: the anim's type
    };

    /* a sound played (0x18 bytes): at the member's position, or flat for the group's */
    class act_sound : public action {
    public:
        act_sound(int member, CSnd *sound, float time) : action(member, time), m_sound(sound), m_inst(0) {}
        virtual STATE Tick(CSealCtrlAi *ai, float t);
        virtual void Stop();

        CSnd *m_sound;   // 0x10
        CSndInstance *m_inst;   // 0x14: while it plays
    };

    /* a step: the actions run together, and its state (1 new, 2 running, 3 done; the name ours) --
     * a pointer list of its own, its destructor out of line (OVL2 0x60DA50: the actions deleted) */
    class step : public std::list<action *> {
    public:
        /* (a byte enum: the list's insert copies it with lb) */
        enum STATE { NEW = 1, RUNNING = 2, DONE = 3, STATE_LAST = 255 };

        step() : m_state(NEW) {}
        ~step();
        /* every action stopped (inline: the steps' stop walk's functor inlines it) */
        void Stop()
        {
            iterator e = end();

            for (iterator i = begin(); i != e; ++i)
                (*i)->Stop();
        }

        STATE m_state;   // 0xc
    };

    /* a new group (OVL2 0x60D400: its id the next of m_nextId, its ring 10 around no centre) and
     * its end (0x60D310: not virtual -- delete calls it with 1) */
    CAiGroup();
    ~CAiGroup();
    /* a group for `n` members made and listed, its flag 1 the default (OVL2 0x60E610) */
    static CAiGroup *Create(int n);
    /* every step stopped and dropped, the group's ZAnim restarted, every member let go (OVL2
     * 0x60D280; the name ours) */
    void Abort();
    /* the group's frame: false while it runs (OVL2 0x60C3E0) */
    bool Tick();
    /* a member let go (OVL2 0x60CE40) */
    void RemoveMember(CSealCtrlAi *ai);
    /* every group ticked, the finished ones dropped (OVL2 0x60E590), and every group dropped (OVL2
     * 0x60E4E0); the ids begun again (OVL2 0x60E580) -- the names ours */
    static void TickAll(float dT);
    static void DeleteAll();
    static void ResetIds();
    /* the flag `f` set or cleared (inlined: `on ? f : 0` set, `on ? 0 : f` cleared; the name ours) */
    void SetFlag(unsigned int f, unsigned int on) { SetFlags(on ? f : 0, on ? 0 : f); }

    /* the members, the steps and the members' cells: each a class over its container (as SOCOM 1's
     * CAiMapLocVec over vector<CAiMapLoc>: ~CAiGroup tests each member's `this` once more than the
     * container's own chain does -- the names ours) */
    class memberlist : public std::vector<CSealCtrlAi *> {
    };
    class steplist : public std::list<step> {
    public:
        /* every step stopped: std::for_each with a step stopper (OVL2 0x60D4B0, inlining for_each one
         * level -- its iterator pieces and the functor called out of line, 0x60D540-0x60D5F0) */
        void Stop();
    };
    /* the stop walk's functor (a word; its operator() OVL2 0x60D590, the step's Stop inlined; the
     * name ours) */
    struct stopper {
        stopper() : m_0(0) {}
        void operator()(step &s) { s.Stop(); }

        int m_0;
    };
    class loclist : public std::vector<CAiMapLoc> {
    };

    /* the cell a member is sent to set (OVL2 0x60CD40; the name ours) */
    void SetMemberLoc(unsigned int i, const CAiMapLoc &loc);
    /* a step begun, the actions after it added to it, and ended (OVL2 0x60CDD0, 0x60CDC0; the
     * names ours) */
    void AddStep(unsigned int n);
    void EndStep();
    /* the actions: a camera move (the group's; flag 1), a sound (the group's, flag 2, or a
     * member's), a ZAnim and a member's anim (OVL2 0x60D740, 0x60D600, 0x60D9A0, 0x60D840,
     * 0x60D8F0; the names ours) */
    void AddCamera(const campoint &from, const campoint &to, float time);
    void AddSound(const char *name, float time);
    void AddSound(int member, const char *name, float time);
    void AddZAnim(int member, const char *name, float time);
    void AddAnim(int member, short anim, float length, float time);

    /* `clear` cleared and `set` set in the flags (inlined with constants; the name ours) */
    void SetFlags(unsigned int set, unsigned int clear)
    {
        m_flags &= ~clear;
        m_flags |= set;
    }
    /* the last step, by the list's sentinel (the name ours) */
    step &LastStep() { return ((std::list<step>::node *)m_steps.end_node().prev_)->data_; }

    unsigned int m_flags;   // 0x0: 1 cameras run, 2 a group sound, 4 and 8 the command's
    int m_id;   // 0x4
    CAiMapLoc m_goal;   // 0x8: the cell the group gathers at
    CPnt3D m_center;   // 0xc: its centre (zero when new)
    float m_radius;   // 0x18: the ring the members stand on around it (10 when new)
    memberlist m_members;   // 0x1c
    steplist m_steps;   // 0x28
    unsigned char m_state;   // 0x34: 0 new (the names ours)
    unsigned char _35[0x38 - 0x35];
    float m_time;   // 0x38: the current step's clock
    unsigned char _3c[0x40 - 0x3c];
    step *m_cur;   // 0x40: the step being built
    loclist m_locs;   // 0x44: where each member is sent
    unsigned int m_50;   // 0x50
    CZAnim *m_anim;   // 0x54: restarted when the group is aborted
    unsigned int m_58;   // 0x58

    /* the group of an id, or 0 (OVL2 0x60E460: CTGroup::Tick calls it, 0x60E7A0 jumps to it) */
    static CAiGroup *Find(int id);

    /* every group (OVL1 bss 0x44D668: a pointer list, its size word, the sentinel at 0x44D66C) */
    static std::list<CAiGroup *> m_list;
    /* the next group's id (OVL1 data 0x3DF1E8, 1 to begin with), the group being ticked (OVL1 bss
     * 0x44D678) and the frame's time (0x44D680) -- the names ours */
    static int m_nextId;
    static CAiGroup *m_ticking;
    static float m_dT;
};

/* the default for a new group's flag 1 (OVL2 bss 0x66BCE8, a byte; unnamed) */
extern unsigned char data_0066BCE8;

/* the list's helpers, out of line: each holds a call that cannot be inlined whole -- a group
 * deleted (OVL2 0x60E3D0), aborted and deleted (0x60E3E0), and ticked and dropped when done, the
 * predicate TickAll's remove_if takes (0x60E410); the names ours */
static void DeleteGroup(CAiGroup *g)
{
    delete g;
}

static void KillGroup(CAiGroup *g)
{
    g->Abort();
    delete g;
}

static bool TickGroup(CAiGroup *g)
{
    bool done = g->Tick();

    if (done)
        KillGroup(g);
    return done;
}

/* (OVL2 0x60E610) */
CAiGroup *CAiGroup::Create(int n)
{
    CAiGroup *g = new CAiGroup;

    m_list.push_back(g);
    if (n > 0)
        g->m_members.reserve(n);
    g->SetFlag(1, data_0066BCE8);
    return g;
}

/* (OVL2 0x60E590: the new end a named local and the erase its own statement, so the end is built
 * once and copied into erase's parameter after remove_if's call).
 * Its remove_if<list<CAiGroup *>::iterator, bool (*)(CAiGroup *)> (0x60E9A0, the unit's tail) is
 * OPEN, 6 instructions, registers only: find_if's two iterator copies come out s4/s5 where the
 * target has s5/s4 -- the open family of the list remove_if instances (include/algorithm's
 * remove_if; README "Tells from completing the MSL headers, round 8"). Its remove_copy_if
 * (0x60ED40) is byte-exact */
void CAiGroup::TickAll(float dT)
{
    m_dT = dT;
    std::list<CAiGroup *>::iterator e = m_list.end();
    std::list<CAiGroup *>::iterator i = std::remove_if(m_list.begin(), e, TickGroup);

    m_list.erase(i, e);
}

/* (OVL2 0x60E580) */
void CAiGroup::ResetIds()
{
    m_nextId = 1;
}

/* (OVL2 0x60E4E0) */
void CAiGroup::DeleteAll()
{
    std::list<CAiGroup *>::iterator e = m_list.end();

    for (std::list<CAiGroup *>::iterator i = m_list.begin(); i != e; ++i)
        DeleteGroup(*i);
    m_list.clear();
}

/* (OVL2 0x60D400) */
CAiGroup::CAiGroup()
    : m_flags(0), m_cur(0), m_50(0), m_anim(0), m_58(0)
{
    m_id = m_nextId++;
    m_center = CPnt3D::zero;
    m_radius = 10.0f;
    m_state = 0;
}

/* (OVL2 0x60D310: the members' destructors) */
CAiGroup::~CAiGroup()
{
}

/* (OVL2 0x60E7A0) */
CAiGroup *CSealCtrlAi::GetGroup()
{
    return CAiGroup::Find(m_group);
}

/* (OVL2 0x60E6F0: in a group, the throttles stopped and the three points zeroed; out of one, the
 * stack's bits updated) */
void CSealCtrlAi::SetGroup(CAiGroup *g)
{
    if (g) {
        m_group = g->m_id;
        StopThrottles(false);
        func_00558590(m_324);
        m_780 = m_78c = m_798 = CPnt3D::zero;
    } else {
        func_005F7470(&m_stateStack, this, 1);
        m_group = 0;
    }
}

/* (OVL2 0x60E6E0) */
bool CSealCtrlAi::InGroup()
{
    return m_group != 0;
}

/* (OVL2 0x60CE40: the AI held, out of the group (SetGroup(0) inlined), its look cleared and back in
 * the target list, its bits 19 and 21 set; then taken out of the members -- std::remove, the new end
 * a named local, the erase its own statement).
 * OPEN, 6 instructions, registers only: the entity (loaded first) comes out in s2 and SetGroup's
 * inlined `this` (the AI reloaded from its frame slot, its address being std::remove's value) in
 * s1; the target has them the other way round -- the entity coloured before the inline's variable,
 * as a later-numbered or deferred node would be. Tried: the let-go steps as a static inline taking
 * (ai, entity) (the AI then kept in a register: worse). The remove/erase half is byte-exact */
void CAiGroup::RemoveMember(CSealCtrlAi *ai)
{
    CEntity *e = ai->m_entity;

    ai->PopToAiState(AI_STATE_HOLD, true);
    ai->SetGroup(0);
    func_005604C0(ai);
    func_00600D40(ai, data_00651130);
    e->SetIncludeInTargetList(true);
    func_005F7470(&ai->m_stateStack, ai, 1);
    ai->m_aibit19 = true;
    ai->m_aibit21 = true;
    memberlist::iterator end = m_members.end();
    memberlist::iterator i = std::remove(m_members.begin(), end, ai);

    m_members.erase(i, end);
}

/* (OVL2 0x60D4B0) */
void CAiGroup::steplist::Stop()
{
    std::for_each(begin(), end(), stopper());
}

/* (OVL2 0x60D280) */
void CAiGroup::Abort()
{
    m_steps.Stop();
    m_steps.clear();
    if (m_anim)
        m_anim->Activate(0);
    while (m_members.size() != 0)
        RemoveMember(m_members[0]);
}

/* (OVL2 0x60DA50: every action deleted, the list emptied) */
CAiGroup::step::~step()
{
    iterator e = end();

    for (iterator i = begin(); i != e; ++i)
        delete *i;
    clear();
}

/* (OVL2 0x60D9A0) */
void CAiGroup::AddSound(int member, const char *name, float time)
{
    m_cur->push_back(new act_sound(member, CSnd::GetSoundByName(name), time));
}

/* (OVL2 0x60D8F0) */
void CAiGroup::AddAnim(int member, short anim, float length, float time)
{
    m_cur->push_back(new act_anim(member, anim, length, time));
}

/* (OVL2 0x60D840) */
void CAiGroup::AddZAnim(int member, const char *name, float time)
{
    CZAnim *anim = ZAnim.GetAnim(name, 0);

    m_cur->push_back(new act_zanim(member, anim, time));
}

/* (OVL2 0x60D740) */
void CAiGroup::AddCamera(const campoint &from, const campoint &to, float time)
{
    m_cur->push_back(new act_camera(from, to, time));
    SetFlags(1, 0);
}

/* (OVL2 0x60D600) */
void CAiGroup::AddSound(const char *name, float time)
{
    m_cur->push_back(new act_sound(-1, CSnd::GetSoundByName(name), time));
    SetFlags(2, 0);
}

/* (OVL2 0x60CDC0) */
void CAiGroup::EndStep()
{
    m_cur = 0;
}

/* (OVL2 0x60CDD0: the new step's action count unread) */
void CAiGroup::AddStep(unsigned int n)
{
    m_steps.push_back(step());
    m_cur = &LastStep();
}

/* (OVL2 0x60E170: played once the step's clock reaches the action's time; done when it ends, at
 * once without a sound) */
CAiGroup::action::STATE CAiGroup::act_sound::Tick(CSealCtrlAi *ai, float t)
{
    switch (m_state) {
    case WAITING:
        if (!(t < m_time)) {
            if (m_sound) {
                if (ai) {
                    CEntity *e = ai->m_entity;

                    m_inst = m_sound->PlaySound3D(&e->m_posW, 1.0f, 0, 0, e->m_id);
                } else
                    m_inst = m_sound->PlaySound(0, 255);
            }
            m_state = RUNNING;
        }
        break;
    case RUNNING:
        if (m_inst) {
            if (!m_inst->IsPlaying())
                Stop();
        } else
            m_state = DONE;
        break;
    }
    return m_state;
}

/* (OVL2 0x60E120) */
void CAiGroup::act_sound::Stop()
{
    if (m_inst) {
        m_inst->Stop();
        CSndInstance::ReleaseSoundInstance(m_inst);
        m_inst = 0;
    }
    m_state = DONE;
}

/* (OVL2 0x60E000: the member's anim played once the step's clock reaches the action's time; done
 * when it ends, or stopped after its length) */
CAiGroup::action::STATE CAiGroup::act_anim::Tick(CSealCtrlAi *ai, float t)
{
    if (ai) {
        switch (m_state) {
        case WAITING:
            if (!(t < m_time)) {
                func_00571150(ai, m_anim, m_length > 0.0f ? 7 : 11);
                m_ai = ai;
                m_state = RUNNING;
            }
            break;
        case RUNNING:
            if (!func_005712F0(ai, m_anim))
                m_state = DONE;
            else if (Timed() && !(t < m_length + m_time))
                Stop();
            break;
        }
    } else
        m_state = DONE;
    return m_state;
}

/* (OVL2 0x60DFC0) */
void CAiGroup::act_anim::Stop()
{
    if (m_ai)
        func_00571100(m_ai, m_anim);
    m_state = DONE;
}

/* (OVL2 0x60DF40: the ZAnim activated once the step's clock reaches the action's time, done the
 * tick after) */
CAiGroup::action::STATE CAiGroup::act_zanim::Tick(CSealCtrlAi *ai, float t)
{
    switch (m_state) {
    case WAITING:
        if (!(t < m_time)) {
            m_anim->Activate(0);
            m_state = RUNNING;
        }
        break;
    case RUNNING:
        m_state = DONE;
        break;
    }
    return m_state;
}

/* (OVL2 0x60CD40) */
void CAiGroup::SetMemberLoc(unsigned int i, const CAiMapLoc &loc)
{
    if (i < m_members.size()) {
        if (i > m_locs.size() + 1)
            m_locs.resize(m_members.size());
        m_locs[i] = loc;
    }
}

/* (OVL2 0x60E460).
 * OPEN, 12 instructions, registers only: the shape is the target's (an assigned iterator living in
 * the frame, its begin() copied bitwise; the found group kept in a result zeroed in the first
 * test's delay slot), but the target keeps the first node in a2, the sentinel in a3 and the group
 * in a1 and compares `bne v1,a0` (the field first); here they rotate (a3, a1, a2; `bne a0,v1`).
 * Tried: the result declared after the iterator, the group in a local of its own (11), the test
 * `id == ...`, the assignment inside the test (8 bytes short) */
#pragma auto_inline off
CAiGroup *CAiGroup::Find(int id)
{
    CAiGroup *found = 0;
    std::list<CAiGroup *>::iterator i;

    for (i = m_list.begin(); i != m_list.end(); ++i)
        if ((*i)->m_id == id) {
            found = *i;
            break;
        }
    return found;
}
#pragma auto_inline reset
