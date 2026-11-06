/* ai_group.cpp -- SOCOM II's scripted AI groups, CAiGroup, with its actions act_camera, act_zanim,
 * act_anim and act_sound (the class names their RTTIs'; no SOCOM 1 counterpart). OVL2 0x60C190 to
 * ~0x60EDFF, after SEAL_AI_PARAMS's constructor and its copy 0x60C170 (seal_aiparams.cpp) and before
 * the vehicle state (seal_aivehicle.cpp); the unit's name is ours.
 *
 * A group is the scripted ai::GROUP command's (CTGroup::Begin, seal_tasks.cpp, builds it): its member
 * AIs (a vector), a cell each may be sent to, and a list of steps, each a list of actions run
 * together -- the step done once every action is (2), the next taken after. Groups live in a global
 * list ticked once a frame (TickAll, a remove_if dropping the finished ones).
 *
 * Round 16 (the kit lane) wrote the unit's classes and 45 of its rows: the actions' Ticks, Stops and
 * table copies, the step builders, the group's lifecycle, Abort with the steps' stop walk (std::
 * for_each inlined one level -- its iterator pieces, functor and copy out of line after it), the AI's
 * group members, Find, EndCameras and the template tail's remove_copy/remove_copy_if. Each member is
 * a class over its container (one this-test more in ~CAiGroup than the container's own chain).
 * Round 17 wrote the rest: Tick, AddMember, act_camera::Tick with its point reader (campoint::Get,
 * byte-exact) and LookAt, and FaceXZ -- with act_camera::Tick in the unit, its table and destructor
 * (0x60C190) come out byte-exact too.
 * With the MSL headers' round 13 (include/list's erases destroy their elements) the step list's range
 * erase 0x60EA90 and Tick's erase(position) 0x60EB60 come out of this unit byte-exact.
 * OPEN: Start and TickStep (registers / the camera flag's negation), RemoveMember (two registers),
 * the group remove_if (the list remove_if family), Tick (the cell index's strength reduction),
 * and AddMember (Is() materialised) -- each with its note. Defined in reverse of
 * their image order (-inline deferred) where it matters. */

#include <list>
#include <vector>
#include <algorithm>

#include "../gamez/zmath/zmath.h"
#include "../gamez/zentity/zentity.h"
#include "../gamez/zanim/zanim.h"
#include "../gamez/zsnd/zsnd.h"
#include "ai_states.h"

class CAiGroup;
class CTarget;

/* zseal.h's enums, as this unit's calls mangle them (GOAL_TYPE is ai_states.h's) */
enum SEAL_STANCE {
    STANCE_NONE = -1, STANCE_STAND = 0, STANCE_CROUCH = 1, STANCE_PRONE = 2, STANCE_IDLE = 3,
    STANCE_MAX = 4
};
enum AI_MODE { AI_MODE_0 = 0, AI_MODE_LAST = 255 };

/* the SEAL controller as far as this unit calls it (zseal.h's CSealCtrl): ClearGoal, and the
 * virtual SetTarget at +0xB0 -- the slots between CEntityCtrl's last (+0x20) and it placeholders */
class CSealCtrl : public CEntityCtrl {
public:
    virtual void V24();
    virtual void V28();
    virtual void V2C();
    virtual void V30();
    virtual void V34();
    virtual void V38();
    virtual void V3C();
    virtual void V40();
    virtual void V44();
    virtual void V48();
    virtual void V4C();
    virtual void V50();
    virtual void V54();
    virtual void V58();
    virtual void V5C();
    virtual void V60();
    virtual void V64();
    virtual void V68();
    virtual void V6C();
    virtual void V70();
    virtual void V74();
    virtual void V78();
    virtual void V7C();
    virtual void V80();
    virtual void V84();
    virtual void V88();
    virtual void V8C();
    virtual void V90();
    virtual void V94();
    virtual void V98();
    virtual void V9C();
    virtual void VA0();
    virtual void VA4();
    virtual void VA8();
    virtual void VAC();
    virtual void SetTarget(CTarget *target);   /* +0xB0 */

    void ClearGoal(GOAL_TYPE type);
};

/* the AI's object at 0x324 (zseal.h's CAiStamp, the SEALs' name: its Reset, OVL2 0x558590, is an
 * ordinary function of seal_ai.cpp -- only the size is this view's) */
class CAiStamp {
public:
    void Reset();

    unsigned char _0[0x24];
};

/* how alert the AI is, the bands of its alertness (zseal.h's, SOCOM 1's DWARF: AI_AWARE, a byte) */
enum AI_AWARE { AIA_UNAWARE = 0, AIA_SUSPICIOUS = 1, AIA_GENERAL = 2, AIA_DIRECTION = 3, AIA_SPECIFIC = 4 };

/* the AI controller as far as this unit reads it (zseal.h's CSealCtrlAi, which this unit does not
 * include): its entity (CEntityCtrl's), the bits at 0x264, its state stack, its group's id and the
 * three throttle points a group's member starts from -- and its group members, which this unit
 * defines (SetGroup, InGroup, GetGroup: OVL2 0x60E6F0, 0x60E6E0, 0x60E7A0; the names ours) */
class CSealCtrlAi : public CSealCtrl {
public:
    /* asleep or awake, and dozing or not (zseal.h's, SOCOM 1's; OVL2 0x56D0C0, 0x56D6A0) */
    void SetSleeping(bool on);
    void SetSnoozing(bool on);
    bool PopToAiState(AI_STATE state, bool inclusive);
    AI_STATE PopAiState(AI_STATE state);
    bool PushAiState(AI_STATE state, unsigned int flags);
    CAiState *GetAiState();
    void SetAiMode(AI_MODE mode);
    void SetStance(SEAL_STANCE stance);
    void StopThrottles(bool keep);
    void SetGroup(CAiGroup *g);
    bool InGroup();
    CAiGroup *GetGroup();
    /* the band the alertness's value lies in (zseal.h's, SOCOM 1's; OVL2 0x560A60) */
    AI_AWARE GetAiAware() const;
    /* the bit at 0x264 (inline; the name ours) */
    void SetAiBit0(bool b) { m_aibit0 = b; }
    /* whether the current state is of a kind (zseal.h's AiStateIs, SOCOM 1's: the stack's const
     * GetState called, the test built whole with its explicit false arm) */
    bool StackNotEmpty() const { return !(m_stateStack.size() == 0); }
    bool AiStateIs(AI_STATE state) const
    {
        return StackNotEmpty() ? (m_stateStack.GetState()->m_state == state ? true : false) : false;
    }

    unsigned char _18[0x264 - 0x18];
    unsigned int m_aibit0 : 1;   // 0x264 bit 0 (cleared when the AI joins a group)
    unsigned int m_aibits : 18;   // (zseal.h's SEAL_AI_PARAMS bits)
    unsigned int m_aibit19 : 1;   // bit 19 (the byte at 0x266, bit 3): set while not in a group's camera
    unsigned int m_aibit20 : 1;
    unsigned int m_aibit21 : 1;   // bit 21 (0x266 bit 5)
    unsigned int m_aibits22 : 10;
    unsigned char _268[0x324 - 0x268];
    CAiStamp m_stamp;   // 0x324 (zseal.h's; restarted by SetGroup)
    unsigned char _348[0x41c - 0x348];
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
/* the stack's bits brought up to date (OVL2 0x5F7470, seal_aistate.cpp's, unwritten), an AI
 * function (0x5604C0) and CSealCtrl::ClearLook (0x600D40, s1:order; by its mangled name: zseal.h
 * stays out of this unit) -- the SEAL lane's */
void func_005F7470(CAiStateStack *stack, CSealCtrlAi *ai, int x);
void func_005604C0(CSealCtrlAi *ai);
void ClearLook__9CSealCtrlF7AI_LOOK(CSealCtrlAi *ai, signed char look);
}

/* the AI's maps, as far as the group asks them (SOCOM 1's CAiMaps; the global AiMaps) */
class CAiMaps {
public:
    bool GetCellCenter(const CAiMapLoc &loc, CPnt3D *center);
};

extern CAiMaps AiMaps;

/* the player's camera as far as a group's camera move sets it (camera.h's CAppCamera, which this
 * unit does not include): its camera's node at 0xb4 */
class CAppCamera {
public:
    unsigned char _0[0xb4];
    zdb::CNode *m_camera;   // 0xb4
};

extern CAppCamera *appCamera;

extern "C" {
/* libvu0's turns (libvu0.h declares the z one) and the maths library's */
void sceVu0RotMatrixX(sceVu0FMATRIX m0, sceVu0FMATRIX m1, float rx);
void sceVu0RotMatrixY(sceVu0FMATRIX m0, sceVu0FMATRIX m1, float ry);
float sinf(float);
float cosf(float);
/* a cell for a point near another (OVL2 0x58F6E0, unnamed: the members' ring points of a group
 * gathering without cells of their own) */
bool func_0058F6E0(const CPnt3D *p, const CPnt3D *near, CAiMapLoc *loc, int x);
/* CSealCtrl's look setters (seal_look.cpp's, by their mangled names: zseal.h stays out of this
 * unit): SOCOM 1's SetLookDirR (OVL2 0x600DD0), SetLookPosW (0x601330), SetLookFlags (0x601850)
 * and SetLookTimer (0x601860) */
void SetLookDirR__9CSealCtrlF7AI_LOOKPC6CPnt3Db(CSealCtrlAi *ai, signed char look, const CPnt3D *dir, bool b);
void SetLookPosW__9CSealCtrlF7AI_LOOKPC6CPnt3D(CSealCtrlAi *ai, signed char look, const CPnt3D *pos);
void SetLookFlags__9CSealCtrlFUi(CSealCtrlAi *ai, unsigned int flags);
void SetLookTimer__9CSealCtrlFf(CSealCtrlAi *ai, float t);
/* a setting (OVL2 0x565B80, the SEAL lane's) and another (0x5604D0) */
void func_00565B80(CSealCtrlAi *ai, int x);
void func_005604D0(CSealCtrlAi *ai, int x);
}

/* two cells apart by their x or y (an inline: the target compares x, then y, materialised; the map
 * id unread -- the name ours) */
static inline bool CellDiffers(const CAiMapLoc &a, const CAiMapLoc &b)
{
    return a.x != b.x || a.y != b.y;
}

/* the look a group clears and sets (OVL2 data 0x651130, 4: a byte the target loads with lb), the look
 * flags (0x651138) and time (0x651140) it gives its gathering members, and a byte that keeps a
 * suspended AI suspended when a group takes it (OVL1 data 0x3DF188) -- the names ours */
extern signed char data_00651130;
extern unsigned int data_00651138;
extern float data_00651140;
extern unsigned char data_003DF188;

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
    /* a camera point: a position and how to read it -- 0x100 it has one, 0x400 in a member's frame
     * (the member's index the low byte), 0x800 around the group's centre, else a world point; 0x200
     * (a camera's end point only) Euler angles to turn by (the names ours) */
    struct campoint {
        /* the point in the world, for a group; whether it has one (OVL2 0x60DE30) */
        bool Get(CAiGroup *g, CPnt3D *out) const;

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
    /* the group begun: each member's bits set (19 while the group runs no cameras), its stack's bits
     * updated; the centre taken from the goal cell when it has one (state 1), else done (OVL2
     * 0x60CBD0; the name ours) */
    void Start();
    /* whether the group runs its cameras (flag 1; inline) */
    bool CamerasOn() { return (m_flags & 1) ? true : false; }
    /* a member let go (OVL2 0x60CE40), and an AI taken in -- whether it joined (OVL2 0x60D020) */
    void RemoveMember(CSealCtrlAi *ai);
    bool AddMember(CSealCtrlAi *ai);
    /* every group ticked, the finished ones dropped (OVL2 0x60E590), and every group dropped (OVL2
     * 0x60E4E0); the ids begun again (OVL2 0x60E580) -- the names ours */
    static void TickAll(float dT);
    /* every group's cameras ended when asked: its members' bits 19 and 21 set and their stacks'
     * bits updated, flag 1 cleared, the group aborted on flag 8 (OVL2 0x60E280: SetAiEnabled calls
     * it; the name ours) */
    static void EndCameras(bool end);
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
    /* the current step ticked: its actions started on its first frame, then ticked on the step's
     * clock until every one is done (OVL2 0x60C220; the name ours) */
    step::STATE TickStep();
    /* the cell a member is sent to, when it has one (inline; the name ours) */
    bool GetLoc(unsigned int i, CAiMapLoc *loc)
    {
        if (i < m_locs.size()) {
            *loc = m_locs[i];
            return true;
        }
        return false;
    }
    /* a member by index, or 0 past the members (inline; the name ours) */
    CSealCtrlAi *GetMember(unsigned int i)
    {
        CSealCtrlAi *ai = 0;

        if (i < m_members.size())
            ai = m_members[i];
        return ai;
    }
    /* the first step, by the list's sentinel (the name ours) */
    step &FirstStep() { return ((std::list<step>::node *)m_steps.end_node().next_)->data_; }
    /* the last step, by the list's sentinel (the name ours) */
    step &LastStep() { return ((std::list<step>::node *)m_steps.end_node().prev_)->data_; }

    unsigned int m_flags;   // 0x0: 1 cameras run, 2 a group sound, 4 and 8 the command's
    int m_id;   // 0x4
    CAiMapLoc m_goal;   // 0x8: the cell the group gathers at
    CPnt3D m_center;   // 0xc: its centre (zero when new)
    float m_radius;   // 0x18: the ring the members stand on around it (10 when new)
    memberlist m_members;   // 0x1c
    steplist m_steps;   // 0x28
    /* the group's state: 0 new, 1 gathering at the goal, 2 its members on their way, 4 a pause, 3
     * its steps run, 5 done -- Tick's bool (a byte enum; the names ours) */
    enum STATE { NEW = 0, GATHER = 1, GOING = 2, STEPPING = 3, PAUSE = 4, DONE = 5, STATE_LAST = 255 };
    STATE m_state;   // 0x34
    unsigned char _35[0x38 - 0x35];
    float m_time;   // 0x38: the current step's clock
    float m_wait;   // 0x3c: the pause's time left
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

/* (OVL2 0x60E280: the groups walked with an assigned iterator, as Find) */
void CAiGroup::EndCameras(bool end)
{
    if (end) {
        std::list<CAiGroup *>::iterator e = m_list.end();
        std::list<CAiGroup *>::iterator i;

        for (i = m_list.begin(); i != e; ++i) {
            CAiGroup *g = *i;

            if (g->m_flags & 1) {
                for (memberlist::iterator j = g->m_members.begin(); j != g->m_members.end(); ++j) {
                    CSealCtrlAi *ai = *j;

                    ai->m_aibit19 = true;
                    ai->m_aibit21 = true;
                    func_005F7470(&ai->m_stateStack, ai, 1);
                }
                g->m_flags &= ~1;
                if (g->m_flags & 8)
                    g->Abort();
            }
        }
    }
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
    m_state = NEW;
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
        m_stamp.Reset();
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
 * a named local, the erase its own statement). The entity through an inline accessor: its result a
 * temporary, numbered after SetGroup's inlined `this` (the AI reloaded from its frame slot, its address
 * being std::remove's value), which puts the entity in s1 and the inline's variable in s2 -- read into
 * the named local straight, the entity is numbered first and the two swap (round 42, by MW's colouring
 * replayed with the vregs relabelled). The remove/erase half was byte-exact already */
static inline CEntity *GroupAiEntity(CSealCtrlAi *ai)
{
    return ai->m_entity;
}

void CAiGroup::RemoveMember(CSealCtrlAi *ai)
{
    CEntity *e = GroupAiEntity(ai);

    ai->PopToAiState(AI_STATE_HOLD, true);
    ai->SetGroup(0);
    func_005604C0(ai);
    ClearLook__9CSealCtrlF7AI_LOOK(ai, data_00651130);
    e->SetIncludeInTargetList(true);
    func_005F7470(&ai->m_stateStack, ai, 1);
    ai->m_aibit19 = true;
    ai->m_aibit21 = true;
    memberlist::iterator end = m_members.end();
    memberlist::iterator i = std::remove(m_members.begin(), end, ai);

    m_members.erase(i, end);
}

/* the matrix at a point facing another in the ground plane: the xz direction normalised into rows 0
 * and 2, the point the translation (OVL2 0x60CAD0; Tick turns a member at its cell to the group's
 * centre with it -- the name ours; pinned: Tick calls it). Byte-exact (round 58) under `#pragma
 * opt_common_subs off` in the body (reset after it): with CSE on MW holds &d.z in a register and keeps
 * d.x in $f2 into the normalise, where the target reads d.x and d.z at sp offsets in both blocks (the
 * normalise's loads repeated); with it off, the target's single loads of x and z for the length need
 * x read through a no-op `(float)` cast (a plain `x = d.x` is propagated into both factors, d.x being
 * the local's own offset-0 field) and z declared bare and assigned after x (numbered at its
 * declaration: x $f1, z $f2, the length's sum into x's register); -d.z shared by a local; the
 * translation through CPnt3D's three-float constructor (its arguments bound right to left: the
 * loads z, y, x, then the stores x, y, z -- a struct copy pairs each load with its store) */
#pragma auto_inline off
static void FaceXZ(const CPnt3D *from, const CPnt3D *to, CMatrix *m)
{
#pragma opt_common_subs off
    m->SetIdentity();
    CPnt3D d = *to - *from;

    float z;
    float x = (float)d.x;

    z = d.z;
    float len = sqrtf(x * x + z * z);

    if (len != 0.0f) {
        float inv = 1.0f / len;

        d.x *= inv;
        d.z *= inv;
    }
    float nz = -d.z;

    m->m_matrix[0][0] = nz;
    m->m_matrix[0][2] = -d.x;
    m->m_matrix[2][0] = d.x;
    m->m_matrix[2][2] = nz;
    *(CPnt3D *)m->m_matrix[3] = CPnt3D(from->x, from->y, from->z);
}
#pragma opt_common_subs reset
#pragma auto_inline reset

/* (OVL2 0x60C3E0: m_ticking set for the actions; the state a byte-enum local, stored back at the
 * end; the cases listed 0, 1, 2, 4, 3 for the target's 3, 4, 2, 1, 0 compare chain; sin and cos
 * taken before the ring point's stores; FaceXZ pinned).
 * OPEN, 285 instructions at the right size, registers and the cell index: MW strength-reduces
 * m_locs[i]'s `i * 4` into an induction register of its own (s0) where the target shifts the
 * counter in the loop (`sll v1,s3,2`); with that register taken every saved register shifts, and
 * &p.z (the target's s8, hoisted with &p.y into s7) is computed in the loop. `#pragma
 * opt_strength_reduction off` drops the index register but the &p.y/&p.z hoisting with it (0x6C8).
 * Tried: the increments in the for header, `int i`, an inline GetLoc (0x6FC). Its erase(begin())
 * emits the step list's erase(position), 0x60EB60, byte-exact. The GOING walk's test is the
 * controller's AiStateIs (round 21: the target's const GetState call, xor/sltiu/xori and explicit
 * false arm at 0x60C898) -- with it the function is 0x6EC, the stack's inline Is() gave 0x6E4 */
bool CAiGroup::Tick()
{
    STATE state = m_state;

    m_ticking = this;
    switch (state) {
    case NEW:
        break;
    case GATHER: {
        int n = m_members.size();
        float step = 6.2831855f / (float)n;
        CPnt3D sum = CPnt3D::zero;
        unsigned int i = 0;
        float angle = 0.0f;

        for (memberlist::iterator it = m_members.begin(); it != m_members.end(); ++it, angle += step, i++) {
            CSealCtrlAi *ai = *it;
            CEntity *e = ai->m_entity;
            CAiMapLoc loc(CAiMapLoc::invalid);
            bool have;

            if (i < m_locs.size()) {
                loc = m_locs[i];
                have = true;
            } else {
                float sn = sinf(angle);
                float cs = cosf(angle);
                CPnt3D p;

                p.x = sn;
                p.y = 0.0f;
                p.z = -cs;
                p.Scale(m_radius);
                p.Add(&m_center);
                have = func_0058F6E0(&p, &m_center, &loc, 1);
            }
            if (have) {
                CPnt3D c;

                AiMaps.GetCellCenter(loc, &c);
                sum.Add(&c);
                if (m_flags & 4) {
                    CMatrix m;
                    CPnt3D at;

                    AiMaps.GetCellCenter(loc, &at);
                    FaceXZ(&at, &m_center, &m);
                    e->TeleportTo(&m);
                    ai->StopThrottles(true);
                    SetLookDirR__9CSealCtrlF7AI_LOOKPC6CPnt3Db(ai, data_00651130, &CPnt3D::unit_nz, true);
                    SetLookFlags__9CSealCtrlFUi(ai, data_00651138);
                    state = STEPPING;
                } else if (ai->PushAiState(AI_STATE_GOTO, 0)) {
                    CAiState *s = ai->GetAiState();
                    CAiSGoto *g = s && s->m_state == AI_STATE_GOTO ? (CAiSGoto *)s : 0;

                    state = GOING;
                    g->m_6c = 0.5f;
                    g->m_cell = loc;
                }
            }
        }
        if (n > 0) {
            sum = sum * (1.0f / (float)n);
            m_center = sum;
            sum.y += 18.0f;
            for (memberlist::iterator it = m_members.begin(); it != m_members.end(); ++it) {
                CSealCtrlAi *ai = *it;

                ClearLook__9CSealCtrlF7AI_LOOK(ai, -1);
                SetLookPosW__9CSealCtrlF7AI_LOOKPC6CPnt3D(ai, data_00651130, &sum);
                SetLookFlags__9CSealCtrlFUi(ai, data_00651138 | 1);
                SetLookTimer__9CSealCtrlFf(ai, data_00651140);
            }
        }
        break;
    }
    case GOING: {
        bool there = true;

        for (memberlist::iterator it = m_members.begin(); it != m_members.end(); ++it) {
            if ((*it)->AiStateIs(AI_STATE_GOTO)) {
                there = false;
                break;
            }
        }
        if (there) {
            state = PAUSE;
            m_wait = 1.0f;
        }
        break;
    }
    case PAUSE:
        m_wait -= m_dT;
        if (m_wait <= 0.0f)
            state = STEPPING;
        break;
    case STEPPING:
        for (memberlist::iterator it = m_members.begin(); it != m_members.end(); ++it) {
            CSealCtrlAi *ai = *it;

            if (!(m_flags & 1) && ai->GetAiAware() == AIA_SPECIFIC)
                state = DONE;
            else {
                SetLookDirR__9CSealCtrlF7AI_LOOKPC6CPnt3Db(ai, data_00651130, &CPnt3D::unit_nz, true);
                SetLookFlags__9CSealCtrlFUi(ai, data_00651138);
            }
        }
        if (state != DONE) {
            switch (TickStep()) {
            case step::DONE:
                m_steps.erase(m_steps.begin());
                state = PAUSE;
                m_wait = 0.5f;
                break;
            case 0:
                state = DONE;
                break;
            }
        }
        break;
    }
    m_state = state;
    m_ticking = 0;
    return m_state == DONE;
}

/* (OVL2 0x60D020: the AI held in the frame -- its address is push_back's). The suspension test is
 * the controller's AiStateIs (the stack's const GetState called, the test built whole with its
 * explicit false arm: the kit lane, round 21) -- the stack's inline Is() branches on each operand */
bool CAiGroup::AddMember(CSealCtrlAi *ai)
{
    bool ok = false;

    if (ai && !ai->InGroup()) {
        if (ai->AiStateIs(AI_STATE_SUSPENDED) && !data_003DF188)
            ai->PopAiState(AI_STATE_SUSPENDED);
        ai->SetSleeping(false);
        ai->SetSnoozing(false);
        if (ai->PushAiState(AI_STATE_HOLD, 3)) {
            CAiState *s = ai->GetAiState();
            CAiState *h = s && s->m_state == AI_STATE_HOLD ? s : 0;

            h->SetFlags(0x108, 0);
            ok = true;
            m_members.push_back(ai);
            ai->SetAiBit0(false);
            ai->SetGroup(this);
            ai->SetAiMode((AI_MODE)2);
            ai->ClearGoal(GOAL_NONE);
            func_00565B80(ai, 2);
            func_005604D0(ai, 6);
            ai->SetStance(STANCE_STAND);
            ai->SetTarget(0);
        }
    }
    return ok;
}

/* (OVL2 0x60C220: the first member found running is kept, and never read; begin() taken once
 * before the switch, the result zeroed first so the empty list branches straight to the end)
 * The iterator numbered after both ends (igreplay, round 43): the address-taken device below */
CAiGroup::step::STATE CAiGroup::TickStep()
{
    step::STATE r = (step::STATE)0;

    if (m_steps.size() != 0) {
        step &s = FirstStep();
        step::iterator i = s.begin();
        /* (a modelling device: the iterator's address taken defers its scalarisation to the optimiser's second
         * pass, so its member is numbered after both ends -- the AI core's round-36 rule) */
        step::iterator &ir = i;
        CSealCtrlAi *first = 0;

        switch (s.m_state) {
        case step::NEW: {
            step::iterator e = s.end();

            m_time = 0.0f;
            for (; i != e; ++i) {
                action *a = *i;
                CSealCtrlAi *ai = GetMember(a->m_member);
                action::STATE st = a->Tick(ai, m_time);

                if (!first && st == action::RUNNING)
                    first = ai;
            }
            s.m_state = step::RUNNING;
            break;
        }
        case step::RUNNING: {
            step::iterator e = s.end();
            int done = 1;

            m_time += m_dT;
            for (; i != e; ++i) {
                action *a = *i;

                if (a->Tick(GetMember(a->m_member), m_time) != action::DONE)
                    done = 0;
            }
            if (done)
                s.m_state = step::DONE;
            break;
        }
        }
        r = s.m_state;
    }
    return r;
}

/* (OVL2 0x60CBD0: the state a byte-enum local (`li`); the members walked with the vector's class
 * iterators, the end re-read each pass, the test materialised).
 * OPEN, 5 instructions: the camera flag's negation. The target computes CamerasOn() into a0 in the
 * loop test's delay slot and then, in the preheader, normalises it again before flipping it (`sltu
 * v0,zero,a0; xori; andi s0,v0,0xff`); every form here flips the first normalisation directly
 * (`sltu; xori` before the test, `andi 0xff` twice after). Tried: `!CamerasOn()` into a bool local
 * before the loop, `CamerasOn() == false` (xor/sltiu), the accessor as `? true : false`, the
 * negation inside the loop (not hoisted: recomputed each pass, one saved register fewer) */
void CAiGroup::Start()
{
    if (m_state == NEW) {
        STATE state = DONE;
        bool cams = !CamerasOn();

        for (memberlist::iterator i = m_members.begin(); i != m_members.end(); ++i) {
            CSealCtrlAi *ai = *i;

            ai->m_aibit19 = cams;
            ai->m_aibit21 = true;
            func_005F7470(&ai->m_stateStack, ai, 1);
        }
        if (CellDiffers(m_goal, CAiMapLoc::invalid) && AiMaps.GetCellCenter(m_goal, &m_center))
            state = GATHER;
        m_state = state;
    }
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

/* (OVL2 0x60DE30) */
bool CAiGroup::campoint::Get(CAiGroup *g, CPnt3D *out) const
{
    bool ok = false;
    unsigned int f = m_flags;

    if (f & 0x100) {
        if (f & 0x400) {
            CSealCtrlAi *ai = g->GetMember(m_flags & 0xff);

            if (ai) {
                ok = true;
                ai->m_entity->m_node->m_matrix.Transform(&m_pos, out, 1);
            }
        } else if (f & 0x800) {
            *out = g->m_center + m_pos;
            ok = true;
        } else {
            ok = true;
            *out = m_pos;
        }
    }
    return ok;
}

/* the matrix at one point looking at another: level (its x axis in the ground plane), up from the
 * cross product, looking down -z (OVL2 0x60DCF0: CAppCamera::LookAt's body, 0x29BC90, the eye not
 * noted; the name ours). Byte-exact in the kit's round 59 by camera.cpp's own source shape (the
 * engine core's round 15, byte-exact there): the optimiser left on, the right vector and the
 * negated row through reference inlines, the translation through a by-value setter -- with them
 * the copy's destination and Normalize's `this` share a0 as in the target (rounds 16-58 had fitted
 * the body by hand, 0x140-0x154, CSE off at the last) */
/* LookAt's pieces, camera.cpp's (the names ours): three floats set by value, the level right vector
 * from the negated z it shares with the matrix and the direction's x read through the reference, and a
 * row set to a point's negation through the reference */
inline void LaSet3(float *r, float x, float y, float z)
{
    r[0] = x;
    r[1] = y;
    r[2] = z;
}

inline void LaRight(float *v, float nz, const CPnt3D &d)
{
    v[0] = nz;
    v[1] = 0.0f;
    v[2] = d.x;
    v[3] = 1.0f;
}

inline void LaNegRow(float *r, const CPnt3D &p)
{
    r[0] = -p.x;
    r[1] = -p.y;
    r[2] = -p.z;
}

static void LookAt(const CPnt3D *from, const CPnt3D *to, CMatrix *m)
{
    CPnt3D vView = *to - *from;

    vView.Normalize();
    CPnt3D v2D = vView;

    v2D.y = 0.0f;
    v2D.Normalize();
    float nz = -v2D.z;
    float x = v2D.x;
    sceVu0FVECTOR up;
    sceVu0FVECTOR right;
    sceVu0FVECTOR view;
    LaRight(right, nz, v2D);
    view[0] = vView.x;
    view[1] = vView.y;
    view[2] = vView.z;
    view[3] = 1.0f;
    sceVu0OuterProduct(up, right, view);
    float ux = up[0];
    float uy = up[1];
    float uz = up[2];
    m->m_matrix[0][0] = nz;
    m->m_matrix[0][1] = 0.0f;
    m->m_matrix[0][2] = x;
    m->m_matrix[1][0] = ux;
    m->m_matrix[1][1] = uy;
    m->m_matrix[1][2] = uz;
    LaNegRow(m->m_matrix[2], vView);
    LaSet3(m->m_matrix[3], from->x, from->y, from->z);
}

/* (OVL2 0x60DB60: the camera put at the start point once the step's clock reaches the move's time,
 * turned by the end point's angles or to look at it; done the tick after -- at once when the
 * group runs no cameras: the flag tested bare, not through CamerasOn(); the unused matrix is the
 * target's two sceVu0UnitMatrix calls on one slot; the group read into a local ahead of the end
 * point's flags, as the target loads it there for the second Get).
 * Byte-exact in the kit's round 59 (0x194 since round 17): the optimiser's common subexpressions
 * and propagation off in the body -- with them on MW shares the translation row's address between
 * the start point's copy and LookAt's argument, holding it in a saved register from the copy on,
 * where the target stores the copy at frame offsets and makes the address again in the look-at
 * arm (in Get's delay slot); what the target does share is shared by hand: the end point's flags
 * in a local, and each rotation's matrix in a block local of its own (one value for both
 * arguments, `move a1,a0`; propagation off keeps it from being folded back into two addresses).
 * The flag and the row pointer declared uninitialised at the block's top, ahead of the camera:
 * numbered before it they put `this` in s1 and the camera in s2 as the target (igreplay --permute
 * over the four holders: set and the pointer before the camera, the camera's own place free) */
CAiGroup::action::STATE CAiGroup::act_camera::Tick(CSealCtrlAi *ai, float t)
{
#pragma opt_common_subs off
#pragma opt_propagation off
    if (!(m_ticking->m_flags & 1))
        return DONE;
    switch (m_state) {
    case WAITING:
        if (!(t < m_time)) {
            bool set;
            CPnt3D *tr;
            zdb::CNode *cam = appCamera->m_camera;
            CMatrix unused;
            CPnt3D pos;

            unused.SetIdentity();
            if (m_from.Get(m_ticking, &pos)) {
                CMatrix m = CMatrix::identity;
                CAiGroup *g;

                set = false;
                m.m_matrix[3][0] = pos.x;
                m.m_matrix[3][1] = pos.y;
                m.m_matrix[3][2] = pos.z;
                g = m_ticking;
                unsigned int f = m_to.m_flags;

                if (f & 0x200) {
                    {
                        float (*mm)[4] = m.m_matrix;

                        sceVu0RotMatrixY(mm, mm, m_to.m_pos.y);
                    }
                    {
                        float (*mm)[4] = m.m_matrix;

                        sceVu0RotMatrixX(mm, mm, m_to.m_pos.x);
                    }
                    {
                        float (*mm)[4] = m.m_matrix;

                        sceVu0RotMatrixZ(mm, mm, m_to.m_pos.z);
                    }
                    set = true;
                } else if (f & 0x100) {
                    CPnt3D at;

                    tr = (CPnt3D *)m.m_matrix[3];
                    if (m_to.Get(g, &at)) {
                        LookAt(tr, &at, &m);
                        set = true;
                    }
                }
                if (set)
                    cam->SetMatrix(&m);
            }
            m_state = RUNNING;
        }
        break;
    case RUNNING:
        m_state = DONE;
        break;
    }
    return m_state;
}
#pragma opt_common_subs reset
#pragma opt_propagation reset

/* (OVL2 0x60CD40) */
void CAiGroup::SetMemberLoc(unsigned int i, const CAiMapLoc &loc)
{
    if (i < m_members.size()) {
        if (i > m_locs.size() + 1)
            m_locs.resize(m_members.size());
        m_locs[i] = loc;
    }
}

/* (OVL2 0x60E460: the end taken into a local first, as EndCameras and DeleteAll do, the iterator
 * assigned (it lives in the frame, begin() copied bitwise), the group read into a local before its
 * id is compared -- which puts the field first in the `bne`. Pinned: GetGroup (0x60E7A0) tail-jumps
 * to it, never inlining it) */
#pragma auto_inline off
CAiGroup *CAiGroup::Find(int id)
{
    CAiGroup *found = 0;
    std::list<CAiGroup *>::iterator e = m_list.end();
    std::list<CAiGroup *>::iterator i;

    for (i = m_list.begin(); i != e; ++i)
        {
            CAiGroup *g = *i;

            if (g->m_id == id) {
                found = g;
                break;
            }
        }
    return found;
}
#pragma auto_inline reset
