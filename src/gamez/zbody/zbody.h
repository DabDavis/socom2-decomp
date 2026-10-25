/* zbody.h -- the animated body: CBody (a base of CZSealBody), its parts (CZBodyPart), the motion
 * scripts it plays (CZSIScript and its objects, CZBodyAnim), the archives they load from
 * (CScriptArchive) and the small helpers between them (CSequence, the plane and quaternion
 * functions). OVL1 0x2869D0-0x290360, six units in SOCOM 1's link order: bone_math.cpp (ours: the
 * helpers, 0x2869D0-0x2875C4), bone_archive.cpp (0x2875D0-0x28891F) and bone_init.cpp
 * (0x288920-0x28936C; the press beta's asserts name both), si_object.cpp (0x289380-0x28A0FC, the
 * same), si_script.cpp (0x28A100-0x28B26C) and body.cpp (0x28B270-0x29035C; SOCOM 1's static
 * initialisers name them).
 *
 * The layouts are SOCOM 1's DWARF moved to where SOCOM II's code puts each member; only what a
 * written function touches is named. Self-contained over zmath.h, znode.h and zar.h, so zseal.h
 * can include it for CZSealBody's CBody base. The header's name is ours. */
#ifndef ZBODY_H
#define ZBODY_H

#include <list>
#include <string>
#include <vector>

#include "../zmath/zmath.h"
#include "../znode/znode.h"
#include "../zar/zar.h"

using zdb::CNode;

/* an up-hold-down envelope: its weight rises over m_upT, holds, and falls over m_downT while
 * m_currT counts down (SOCOM 1's DWARF, 0x10 bytes) */
class CSequence {
public:
    float Weight();
    float LinearWeight();
    float Tick(float dT);
    void Clear();
    void Start(float up, float hold, float down);

    float m_upT;   // 0x0
    float m_holdT;   // 0x4
    float m_downT;   // 0x8
    float m_currT;   // 0xc
};

/* a bone of the body: its node and its pose relative to its parent (SOCOM 1's DWARF, 0x50 bytes
 * there; SOCOM II's is 0x44 -- the parent moved ahead of the rotation, the id and two bits last) */
class CZBodyPart {
public:
    /* built empty: the constructor's body is Clear, which new inlines one level (NewBodyPart and
     * AttachAuxNode call the four members' constructors and Clear out of line) */
    CZBodyPart(CNode *node)
    {
        Clear();
        m_node = node;
    }
    ~CZBodyPart() { Clear(); }
    /* the live bit: eased by SmoothAnims, set through a bool setter (SOCOM II's; the names ours) */
    bool Active() const { return m_bit0; }
    void SetActive(bool on) { m_bit0 = on; }
    /* every field to its rest value (SOCOM II's; the name ours) */
    void Clear()
    {
        m_node = 0;
        m_parent = 0;
        m_pos = CPnt3D::zero;
        m_rot = CQuat::identity;
        m_saved_pos = CPnt3D::zero;
        m_saved_rot = CQuat::identity;
        m_bit0 = true;
        m_bit1 = true;
        m_id = -1;
    }
    /* the node placed where the part is (SOCOM 1's) */
    void Orient();

    CPnt3D m_pos;   // 0x0
    CNode *m_node;   // 0xc
    CPnt3D m_saved_pos;   // 0x10
    CZBodyPart *m_parent;   // 0x1c
    CQuat m_rot;   // 0x20
    CQuat m_saved_rot;   // 0x30
    short m_id;   // 0x40
    unsigned char m_bit0 : 1;   // 0x42
    unsigned char m_bit1 : 1;
    unsigned char _44[0x50 - 0x44];   // (NewBodyPart news 0x50 bytes, as SOCOM 1's)
};

/* SOCOM 1's real-time containers: a fixed pool of items handed out and put back (RTArray), a
 * doubly linked list over a fixed pool of links (RTList) and a fixed stack (RTStack). Their members
 * are defined after the classes and not inline: body.cpp's tail emits every instance and its own
 * functions call them (MW never inlines an out-of-class template member into them). */
template <class T>
class RTList {
public:
    /* a link: SOCOM 1's DWARF leaves it opaque; 12 bytes, every field zeroed */
    class LIST_ITEM {
    public:
        LIST_ITEM() { m_prev = 0; m_item = 0; m_next = 0; }
        ~LIST_ITEM() {}

        LIST_ITEM *m_prev;   // 0x0
        T *m_item;   // 0x4
        LIST_ITEM *m_next;   // 0x8
    };

    RTList(unsigned int size);
    ~RTList();
    T *Begin();
    T *Next();
    void Clear();
    T *Remove(T *item);
    T *Add(T *item);

    unsigned int m_size;   // 0x0
    LIST_ITEM *m_head;   // 0x4
    LIST_ITEM *m_tail;   // 0x8
    LIST_ITEM *m_curritem;   // 0xc
    LIST_ITEM *m_pool;   // 0x10
};

template <class T>
T *RTList<T>::Begin()
{
    m_curritem = m_head;
    return m_curritem ? m_curritem->m_item : 0;
}

template <class T>
T *RTList<T>::Next()
{
    if (m_curritem)
        m_curritem = m_curritem->m_next;
    else
        return 0;
    return m_curritem ? m_curritem->m_item : 0;
}

template <class T>
void RTList<T>::Clear()
{
    m_head = 0;
    m_tail = 0;
    m_curritem = 0;
    LIST_ITEM *p = m_pool;

    for (unsigned int i = 0; i < m_size; i++, p++) {
        p->m_prev = 0;
        p->m_item = 0;
        p->m_next = 0;
    }
}

template <class T>
T *RTList<T>::Remove(T *item)
{
    LIST_ITEM *p = m_pool;

    for (unsigned int i = 0; i < m_size; i++, p++) {
        if (p->m_item == item) {
            if (p->m_prev) {
                if (p->m_next) {
                    p->m_next->m_prev = p->m_prev;
                    p->m_prev->m_next = p->m_next;
                } else {
                    p->m_prev->m_next = 0;
                    m_tail = p->m_prev;
                }
            } else if (p->m_next) {
                p->m_next->m_prev = 0;
                m_head = p->m_next;
            } else {
                m_head = 0;
                m_tail = 0;
            }
            p->m_prev = 0;
            p->m_item = 0;
            p->m_next = 0;
            return item;
        }
    }
    return 0;
}

template <class T>
T *RTList<T>::Add(T *item)
{
    LIST_ITEM *tail = m_tail;
    LIST_ITEM *p = m_pool;
    unsigned int i;

    if (!tail) {
        p->m_item = item;
        m_head = p;
        m_tail = p;
        return item;
    }
    for (i = 0; i < m_size; i++, p++) {
        if (p != tail && !p->m_item) {
            tail->m_next = p;
            p->m_prev = tail;
            p->m_item = item;
            p->m_next = 0;
            m_tail = p;
            return item;
        }
    }
    return 0;
}

template <class T>
RTList<T>::RTList(unsigned int size)
{
    m_size = size;
    m_head = 0;
    m_tail = 0;
    m_curritem = 0;
    m_pool = new LIST_ITEM[m_size];
    LIST_ITEM *p = m_pool;

    for (unsigned int i = 0; i < m_size; i++, p++) {
        p->m_prev = 0;
        p->m_item = 0;
        p->m_next = 0;
    }
}

template <class T>
RTList<T>::~RTList()
{
    delete[] m_pool;
}

template <class T>
class RTStack {
public:
    RTStack(unsigned int size);
    ~RTStack() { delete[] m_stack; }
    void Push(T *item);
    T *Pop();
    /* the top item, none when empty (SOCOM 1's): out of class and defined in another unit, whose
     * tail holds the one copy (OVL1 0x219390), so body.cpp calls it (func_00219390 below) */
    T *Top();

    T **m_stack;   // 0x0
    int m_size;   // 0x4
    int m_top;   // 0x8
};

template <class T>
RTStack<T>::RTStack(unsigned int size)
{
    m_size = size;
    m_top = -1;
    m_stack = new T *[m_size];
    for (unsigned int i = 0; i < m_size; i++)
        m_stack[i] = 0;
}

template <class T>
void RTStack<T>::Push(T *item)
{
    if (m_top < m_size - 1)
        m_stack[++m_top] = item;
}

template <class T>
T *RTStack<T>::Pop()
{
    if (m_top < 0)
        return 0;
    T *item = m_stack[m_top];

    m_stack[m_top--] = 0;
    return item;
}

/* a fixed pool: the free items stacked in m_pool, those handed out listed when recycling (SOCOM 1's
 * DWARF: 0x1c bytes; SOCOM II's keeps the items' own array at 0x10, 0x20 bytes) */
template <class T>
class RTArray {
public:
    RTArray(unsigned int size, bool alloc, bool recycle);
    ~RTArray();
    void PutBack(T *item);
    T *GetNext(bool b);
    T *GetOldestUsed();

    T **m_pool;   // 0x0
    std::list<T *> m_inuse;   // 0x4
    T *m_items;   // 0x10
    unsigned int m_size;   // 0x14
    unsigned int m_available;   // 0x18
    bool m_supportRecycle;   // 0x1c
};

template <class T>
void RTArray<T>::PutBack(T *item)
{
    if (m_available < m_size) {
        m_pool[m_available] = item;
        m_available++;
        if (m_supportRecycle)
            m_inuse.remove(item);
    }
}

/* the item handed out longest ago taken back off the in-use list (SOCOM 1's GetOldestUsed, added
 * by the MSL headers round 5; SOCOM II's binder holds the item by reference, so it lives in the
 * frame and is reloaded for the return: OVL1 0x3BF3D0 and 0x3BF6F0 byte-exact as probes) */
template <class T>
T *RTArray<T>::GetOldestUsed()
{
    if (m_inuse.size()) {
        T *item = m_inuse.front();

        m_inuse.remove(item);
        return item;
    }
    return 0;
}

template <class T>
T *RTArray<T>::GetNext(bool b)
{
    if (m_available) {
        m_available--;
        T *item = m_pool[m_available];

        m_pool[m_available] = 0;
        if (m_supportRecycle)
            m_inuse.push_back(item);
        return item;
    }
    return 0;
}

/* the pool's constructor and destructor (SOCOM 1's RTArray(unsigned int, bool, bool)): the free
 * items stacked, or empty slots when the pool owns no items; out of class like the rest */
template <class T>
RTArray<T>::RTArray(unsigned int size, bool alloc, bool recycle)
{
    m_size = size;
    m_pool = new T *[m_size];
    if (alloc) {
        m_items = new T[m_size];
        for (unsigned int i = 0; i < m_size; i++)
            m_pool[i] = &m_items[i];
        m_available = m_size;
    } else {
        m_items = 0;
        for (unsigned int i = 0; i < m_size; i++)
            m_pool[i] = 0;
        m_available = 0;
    }
    m_supportRecycle = recycle;
}

template <class T>
RTArray<T>::~RTArray()
{
    if (m_items) {
        delete[] m_items;
    } else {
        for (unsigned int i = 0; i < m_size; i++)
            delete m_pool[i];
    }
    m_items = 0;
    for (unsigned int i = 0; i < m_size; i++)
        m_pool[i] = 0;
    delete[] m_pool;
    m_pool = 0;
}

class CZSIScriptCommon;
class CBody;

/* an animation playing in a blend (SOCOM 1's DWARF: 0x20 bytes; SOCOM II's is 0x2c, its script at
 * 4 -- the common data, where SOCOM 1 pointed at its CZBodyAnim) */
class ACTIVE_ANIM {
public:
    ACTIVE_ANIM() { Clear(); }
    ~ACTIVE_ANIM() { Clear(); }
    /* every field to rest (SOCOM 1's; body.cpp's, which inlines it, the bits through bool setters) */
    void Clear();
    void SetBit0(bool b) { m_bit0 = b; }
    void SetBit1(bool b) { m_bit1 = b; }
    /* the playback rate, scaled by the script's own when it carries one (SOCOM II's; the name ours) */
    void SetPlayback(float rate);
    /* every overlay hung on the anim put back in the body's pool (SOCOM 1's) */
    void ClearOverlays(CBody *body);
    /* the anim placed at a fraction of its script: wrapped into 0..1, split into a frame and the
     * fraction past it, held on the last frame when the script does not loop (SOCOM II's; ours) */
    void SetFrac(float t);

    int m_0;   // 0x0
    CZSIScriptCommon *m_anim;   // 0x4
    float m_wt;   // 0x8: (SetRepresentativeWt)
    float m_dfrac;   // 0xc: added to the blend's fraction (Synch)
    ACTIVE_ANIM *m_overlay;   // 0x10
    float m_tfrac;   // 0x14
    float m_script_wt;   // 0x18: 1 when cleared, copied to an overlay
    float m_ffrac;   // 0x1c: the fraction past m_frame
    short m_frame;   // 0x20
    short m_id;   // 0x22: -1 when cleared (FindAnim)
    float m_playback;   // 0x24: 1 when cleared, negated by ReverseDir
    unsigned char m_bit0 : 1;   // 0x28
    unsigned char m_bit1 : 1;
};

class CActiveAnimList : public RTList<ACTIVE_ANIM> {
public:
    CActiveAnimList(unsigned int size) : RTList<ACTIVE_ANIM>(size) {}
};

/* an anim pushed on a list at a weight; every weight on a list scaled (SOCOM 1's free functions) */
void PushAnim(CActiveAnimList *list, ACTIVE_ANIM *anim, float wt);
void RenormAnims(CActiveAnimList *list, float scale);

/* the flag words' helpers: function templates, so never inlined (SOCOM II's, no SOCOM 1 copy;
 * called by their placeholders until named) */
extern "C" {
bool func_0028F190(unsigned short flags, unsigned short mask);
unsigned short func_0028F1B0(unsigned short flags, unsigned short mask, bool on);
}

/* a function called at a time in an anim (SOCOM 1's DWARF, 0xc bytes): cleared function first */
class ANIM_CALLBACK {
public:
    ANIM_CALLBACK() { Clear(); }
    ~ANIM_CALLBACK() { Clear(); }
    void Clear()
    {
        m_func = 0;
        m_tfrac = 0.0f;
        m_userdata = 0;
    }

    float m_tfrac;   // 0x0
    void (*m_func)(void *body, void *userdata);   // 0x4
    void *m_userdata;   // 0x8
};

/* where a blend wants its reference point to land (SOCOM 1's DWARF, 0x1c bytes) */
class REFPT_INFO {
public:
    REFPT_INFO() { Clear(); }
    ~REFPT_INFO() { Clear(); }
    void Clear()
    {
        m_goal_pos = CPnt3D::zero;
        m_goal_norm = CPnt3D::zero;
        m_valid = 0;
    }

    CPnt3D m_goal_pos;   // 0x0
    unsigned char m_valid;   // 0xc
    CPnt3D m_goal_norm;   // 0x10
};

/* the 12-byte copy the homing's points are reset through, out of line wherever its reset is
 * inlined (OVL1 0x20F1F0, another unit's; its inline height is what keeps it a call) */
extern "C" CPnt3D *func_0020F1F0(CPnt3D *to, const CPnt3D *from);

/* a blend steering its reference point onto a target (SOCOM 1's DWARF: 0x40 bytes; SOCOM II's
 * 0x48, two words more) */
class CHoming {
public:
    CHoming() { Reset(); }
    ~CHoming() { Reset(); }
    /* every point zeroed (SOCOM II's; the name ours) */
    void Reset()
    {
        m_blend = 0;
        func_0020F1F0(&m_refpt_m, &CPnt3D::zero);
        func_0020F1F0(&m_norm_w, &CPnt3D::zero);
        func_0020F1F0(&m_int_w, &CPnt3D::zero);
        m_1c = 0;
        m_2c = 0;
        func_0020F1F0(&m_refvec_m, &CPnt3D::zero);
        func_0020F1F0(&m_target_refpt_m, &CPnt3D::zero);
    }

    CPnt3D m_refpt_m;   // 0x0
    class CZBodyAnimBlend *m_blend;   // 0xc
    CPnt3D m_norm_w;   // 0x10
    int m_1c;   // 0x1c
    CPnt3D m_int_w;   // 0x20
    unsigned char m_2c;   // 0x2c
    CPnt3D m_refvec_m;   // 0x30
    CPnt3D m_target_refpt_m;   // 0x3c
};

/* a blend of animations (SOCOM 1's DWARF: 0x84 bytes; SOCOM II's is 0xb4: its list of anims, the
 * reference point and the homing are members) */
class CZBodyAnimBlend {
public:
    CZBodyAnimBlend() : m_anims(8) { Clear(); }
    ~CZBodyAnimBlend() { Clear(); }
    /* every field to rest, its anims' list cleared */
    void Clear();
    int CountAnims();
    ACTIVE_ANIM *FindAnim(int id);
    void AddOverlay(ACTIVE_ANIM *anim, ACTIVE_ANIM *overlay, float wt);
    /* the anims' weights scaled to sum to one */
    void Normalize();
    /* the anims replaced by a list's (their overlays put back in the body's pool) */
    void SetBlendAnims(CBody *body, CActiveAnimList *list);
    void ReverseDir();
    ACTIVE_ANIM *Representative();
    void SetRepresentativeWt(float wt);
    int ThisFoot();
    /* the weighted velocity of the anims' root objects */
    void GetVelocity(CPnt3D *vel);
    /* every anim placed at the blend's fraction */
    void Synch();
    /* the fraction run on by dT, wrapped or held; stored when asked */
    float Propagate(float dT, bool set);
    bool IsDone(float dT);
    bool IsTriggerTime(float t, float t0, float t1);
    void CallbackTick(CBody *body, float t0, float t1);
    /* the step to be held back once: flag 8 (SOCOM II's; the name ours). Const: called through the
     * non-const this, the caller's next statement loads the flags again (Propagate) */
    bool Holding() const { return m_flags & 8 ? true : false; }
    /* the blend restarted from its first anim */
    void Reset();
    /* the homing (the name ours): through this inline its address stays in a register of its own
     * across the reset's calls (PushBlend), as the pool's does through CBody::ActiveAnimPool() */
    CHoming *Homing() { return &m_homing; }

    CPnt3D m_dpos;   // 0x0
    float m_quitT;   // 0xc
    CPnt3D m_drot;   // 0x10
    float m_tfrac;   // 0x1c
    void *m_20;   // 0x20
    void *m_24;   // 0x24
    unsigned short m_flags;   // 0x28
    char m_ctrl;   // 0x2a
    char m_state;   // 0x2b
    short m_animType;   // 0x2c
    short m_2e;   // 0x2e
    CActiveAnimList m_anims;   // 0x30
    REFPT_INFO m_refpt_info;   // 0x44
    ANIM_CALLBACK m_callback;   // 0x60
    CHoming m_homing;   // 0x6c
};

/* the body's pools (SOCOM 1's DWARF: CActiveAnimPool and CBlendPool, each an RTArray) and its stack
 * of blends (CBodyAnimBlendStack : RTStack) */
class CActiveAnimPool : public RTArray<ACTIVE_ANIM> {
public:
    CActiveAnimPool(unsigned int size, bool alloc, bool recycle)
        : RTArray<ACTIVE_ANIM>(size, alloc, recycle) {}
    /* an anim cleared and put back; one taken out for a script (SOCOM 1's) */
    void PutBack(ACTIVE_ANIM *anim);
    ACTIVE_ANIM *GetActiveAnim(CZSIScriptCommon *anim, short id);
};

class CBlendPool : public RTArray<CZBodyAnimBlend> {
public:
    CBlendPool(unsigned int size, bool alloc, bool recycle)
        : RTArray<CZBodyAnimBlend>(size, alloc, recycle) {}
    /* a blend cleared as it goes back, or as it comes out (SOCOM II's; the names ours) */
    void PutBack(CZBodyAnimBlend *blend);
    CZBodyAnimBlend *GetNext();
};

class CBodyAnimBlendStack : public RTStack<CZBodyAnimBlend> {
public:
    CBodyAnimBlendStack(unsigned int size) : RTStack<CZBodyAnimBlend>(size) {}
    /* the blends stacked */
    int Size() { return m_top + 1; }
};

/* the stack's top: its out-of-line copy is an earlier unit's (OVL1 0x219390), which the linker
 * kept over body.cpp's */
extern "C" CZBodyAnimBlend *func_00219390(CBodyAnimBlendStack *stack);

/* an anim laid over the body's blends for a while (SOCOM 1's DWARF, 0x1c bytes) */
class CBlendOverlay {
public:
    CBlendOverlay() { Clear(); }
    ~CBlendOverlay() { Clear(); }
    void Clear()
    {
        m_anim = 0;
        m_triggerT = 0.0f;
        m_ovlyT = 0.0f;
        m_state = 0;
        m_event = 0;
        m_callback.Clear();
    }
    /* its callback made once its time comes (SOCOM 1's) */
    void CallbackTick(void *body, float t);
    bool IsTriggerTime(float t, float dT);
    /* the overlay run on by dT, put back when it has played out; true then */
    bool Tick(CBody *body, float dT);
    /* started, forwards or backwards (SOCOM 1's took the body too) */
    bool Trigger(bool reverse);
    bool IsTriggerTime(float t0, float t1, bool reverse);

    ACTIVE_ANIM *m_anim;   // 0x0
    float m_triggerT;   // 0x4
    float m_ovlyT;   // 0x8
    short m_state;   // 0xc
    short m_event;   // 0xe
    ANIM_CALLBACK m_callback;   // 0x10
};

/* the bone id of a node's name (OVL2 0x58CB30, the SEAL lane's range; not named yet) */
extern "C" int func_0058CB30(const char *name);

/* the animated body (SOCOM 1's DWARF: 0x48 bytes, its table first; SOCOM II's is 0x90 -- its
 * constructor, 0x28EDD0, zeroes through 0x8C). SOCOM II's table (0x406100) has four slots: the
 * destructor, NullNode (an inline returning -1), a pure virtual (the skeleton's root part:
 * CZBodyAnimBlend::CallbackTick starts the callbacks' ZAnims on its node's parent) and BodySetState. */
class CBody {
public:
    CBody();
    virtual ~CBody();
    virtual int NullNode() const { return -1; }
    /* +0x10 and +0x14 (the table at 0x406100 runs on past NullNode: a pure slot, then 0x28DFB0,
     * which stores the state; CZSealBody overrides both -- SOCOM 1's GetRootPart and BodySetState,
     * by CZSealBody's overrides) -- the SEAL lane */
    virtual CZBodyPart *GetRootPart() const = 0;
    virtual void BodySetState(int state);

    /* a part's position and rotation in the body's space: up its chain of parents (SOCOM 1's) */
    void PartInSpace(CZBodyPart *part, const CPnt3D *offset, CPnt3D *pos, CQuat *rot) const;
    /* the parts: one made for a node (its bone id from the node's name), filed by id, and a
     * node's whole tree of them (SOCOM 1's) */
    CZBodyPart *NewBodyPart(CNode *node, CZBodyPart *parent);
    void AddBodyPart(CNode *node, CZBodyPart *parent);
    CZBodyPart *BodySetup(CNode *node, CZBodyPart *parent);
    /* everything freed: the overlay, the blends, the parts (SOCOM 1's) */
    void Destroy();
    void ClearBlendOverlay();
    CZBodyAnimBlend *PopBlend();
    CZBodyAnimBlend *PushBlend(CZSIScriptCommon *anim, short id, char state);
    /* the parts: one marked live or not, eased toward their saved poses, placed on their nodes,
     * their poses saved, read back from their nodes (SOCOM II's; the names ours but SOCOM 1's
     * SmoothAnims) */
    void SetPartActive(int on, CZBodyPart *part);
    void SmoothPart(CZBodyPart *part, float t);
    void SmoothAnims(float t);
    void OrientAuxParts();
    void OrientParts();
    void SavePart(CZBodyPart *part);
    void SaveParts();
    void ReadParts();
    void SetBodyPart(int id, CZBodyPart *part);
    CZBodyPart *FindPart(const char *name);
    CNode *FindAuxNode(const char *name, bool deep);
    CZBodyPart *AttachAuxPart(CZBodyPart *part, CZBodyPart *parent);
    CZBodyPart *AttachAuxNode(CNode *node, CZBodyPart *parent, const CPnt3D *pos, const CQuat *rot);
    CZBodyPart *AttachAuxNode(CNode *node, CZBodyPart *parent, const CPnt3D *pos, const CPnt3D *rot);
    /* an archive's anim readied for the body (SOCOM 1's BodyAddAnim, which took a name) */
    class CZBodyAnim *BodyAddAnim(class CScriptArchive *archive, int index);
    /* the anim pool (SOCOM 1's was a pointer member; the name ours): reached through this inline,
     * the pool's address is a temporary MW hoists out of a loop into a preheader of its own, after
     * the loop's bottom test -- the walks that put anims back (ClearOverlays, PopBlend,
     * SetBlendAnims) need it; `&body->m_active_anim_pool` is recomputed every pass */
    CActiveAnimPool *ActiveAnimPool() { return &m_active_anim_pool; }
    /* the blend on top of the stack (the name ours): the inline's result takes the lowest saved
     * register (PushBlend keeps it in s0, the homing's address in s1) where a call made in the
     * function itself gives it the next one up */
    CZBodyAnimBlend *TopBlend() { return m_blends.Top(); }

    short m_state;   // 0x4: the SEAL's state (SOCOM 1's char at 0xd)
    float m_interp1;   // 0x8
    float m_interp2;   // 0xc
    CActiveAnimPool m_active_anim_pool;   // 0x10 (SOCOM 1's a pointer)
    CBlendPool m_blend_pool;   // 0x30
    CBodyAnimBlendStack m_blends;   // 0x50
    std::vector<CZBodyPart *> m_body_parts;   // 0x5c: by bone id
    std::vector<CZBodyPart *> m_body_parts_aux;   // 0x68
    CBlendOverlay m_overlay;   // 0x74 (SOCOM 1's a pointer at 0x8)
};

class CZSIScriptCommon;
class CZSIScript;

/* one object's track in a motion script: its positions and rotations, a frame each or one when
 * constant (SOCOM 1's DWARF, 0x10 bytes; SOCOM II packs a position into three shorts and a
 * rotation into four 1.15 fixed-point shorts, where SOCOM 1 kept floats and a CQuat) */
class CZSIObject {
public:
    CZSIObject();
    ~CZSIObject();
    /* the track laid over a script's buffer: its flags word, the positions, the rotations from the
     * next word boundary; returns the bytes used (SOCOM 1's, which copied the name to a table) */
    int Read(unsigned char *data, unsigned int nFrames, const char *name);
    /* the track read from an archive's key (SOCOM 1's) */
    bool Read(CZSIScript *script, zar::CZAR &zar);
    bool Initalize(CZSIScriptCommon *common);
    /* the track's velocity between two frames (SOCOM 1's took the CZSIScript) */
    CPnt3D *GetVelRaw(CZSIScriptCommon *script, int f0, int f1, CPnt3D *vel);
    /* the track at a frame: the rotation as four 1.15 shorts, the position as three 8.8 shorts,
     * converted through VU0 (SOCOM II's; SOCOM 1 kept floats) -- one frame, the nearer of two, or
     * between two (SOCOM 1's Fast and Raw readers) */
    void GetRot(int frame, CQuat *rot);
    void GetPos(int frame, CPnt3D *pos);
    CQuat *GetRotFast(int f0, int f1, float t, CQuat *rot);
    CQuat *GetRotRaw(int f0, int f1, float t, CQuat *rot);
    CPnt3D *GetPosFast(int f0, int f1, float t, CPnt3D *pos);
    CPnt3D *GetPosRaw(int f0, int f1, float t, CPnt3D *pos);

    short *m_positionData;   // 0x0
    short *m_rotations;   // 0x4
    unsigned int m_inUse : 1;   // 0x8
    unsigned int m_constPos : 1;
    unsigned int m_constRot : 1;
    unsigned int m_padding : 21;
    unsigned int m_body_part_id : 8;
    char *m_pName;   // 0xc
};

/* the objects alive (SOCOM 1's numSiObject, si_object.cpp's) */
extern int numSiObject;

/* a motion script's shared data (SOCOM 1's DWARF: CZSIScriptCommon, 0x38 bytes; SOCOM II's is 0x5c --
 * Read and CreateAndReadIt new 92 bytes -- and carries what SOCOM 1's CZBodyAnim held too: the loop
 * flag, the archive, the callback indices; it is what an ACTIVE_ANIM plays). si_script.cpp's; the
 * names past SOCOM 1's are ours. */
class CZSIScriptCommon {
public:
    /* built empty, its defaults set and one empty object slot per body part (0x28AFE0) */
    CZSIScriptCommon();
    /* its name freed, its slots given up, its defaults set again (0x28A370) */
    ~CZSIScriptCommon();
    /* every tuning value to its default: rates 1, times unset (-1), every flag clear (0x28AE40) */
    void SetDefaults();
    /* the share of the script one loop covers: all of it when it loops, else up to its last frame
     * (SOCOM 1's CZBodyAnim::TrueLoopTime; si_script.cpp's, 0x28ADA0) */
    float TrueLoopTime();
    /* the time one loop takes (ReadMotionRdr scales a callback's time by it; the name ours) */
    float LoopTime() { return m_10 * TrueLoopTime(); }
    /* the archive callback each of four slots names, 0 for none (SOCOM 1's CZBodyAnim's) */
    int GetCallbackIndex(int i);
    void SetCallbackIndex(int i, unsigned int index);
    /* the distance the root object covers over the script, flat or (flag) vertical (0x28AB10) */
    void CalcDistance();
    /* a looped script's distance unit: 1, or 100 unless flag 7 (0x28AAC0) */
    float DistanceScale();
    /* the rate a looped script plays at: its time over its distance, 1 for a short one (0x28AA20) */
    void CalcRate();
    /* the bit-field setters, inline (their stores take `(x & mask) << n` even for constants) */
    void SetNumFrames(int n) { m_nFrames = n; }
    void SetId(int n) { m_id = n; }
    void SetArchive(int n) { m_archive = n; }
    void SetRead(bool b) { m_49_4 = b; }
    void SetObjectsReady(bool b) { m_objects_ready = b; }
    void SetLooped(bool b) { m_looped = b; }
    void SetFlag7(bool b) { m_49_7 = b; }
    /* motion.rdr's flags (ReadMotionRdr; each named after its tag, the members kept as named) */
    void SetLateral(bool b) { m_4a_0 = b; }
    void SetVertical(bool b) { m_vertical = b; }
    void SetBlendOverlay(bool b) { m_4a_2 = b; }
    void SetPitchtwist(bool b) { m_4a_3 = b; }
    void SetUseVelY(bool b) { m_4a_4 = b; }
    void SetHeadlooks(bool b) { m_4a_5 = b; }
    void SetOverrideControl(bool b) { m_4a_6 = b; }
    void SetTurn(bool b) { m_4a_7 = b; }
    void SetRefPtValid(bool b) { m_4b_0 = b; }
    void SetRootId(int n) { m_4_hi = n; }

    char *m_pName;   // 0x0
    unsigned short m_nFrames : 10;   // 0x4
    unsigned short m_4_hi : 6;
    unsigned char m_nObjects;   // 0x6
    unsigned char m_nBodyObjects;   // 0x7
    CZSIObject *m_pObjects;   // 0x8
    short m_refCount;   // 0xc: the scripts sharing it
    short m_e;   // 0xe: -1 when built
    float m_10;   // 0x10: the script's time, scaled by TrueLoopTime
    float m_14;   // 0x14: the rate a looped script plays at (SetPlayback)
    float m_18;   // 0x18: the distance covered (CalcDistance)
    float m_1c;   // 0x1c: 1 by default
    float m_20;   // 0x20: the blend-in time (PushBlend, PopBlend); -1 by default
    float m_24;   // 0x24 .. 0x34: -1 by default
    float m_28;
    float m_2c;
    float m_30;
    float m_34;
    std::vector<CZSIObject *> m_objects_for_body;   // 0x38: by bone id
    CZSIObject *m_root;   // 0x44: the root object (GetVelocity)
    unsigned short m_id : 10;   // 0x48
    unsigned char m_archive : 2;   // 0x49: the archive it came from, 1 or 2 (0 none)
    unsigned char m_49_4 : 1;   // set once Read has filled it
    unsigned char m_objects_ready : 1;
    unsigned char m_looped : 1;
    unsigned char m_49_7 : 1;   // the distance unit 1, not 100
    unsigned char m_4a_0 : 1;   // 0x4a
    unsigned char m_vertical : 1;   // the distance is the rise, not the flat run
    unsigned char m_4a_2 : 1;
    unsigned char m_4a_3 : 1;
    unsigned char m_4a_4 : 1;
    unsigned char m_4a_5 : 1;
    unsigned char m_4a_6 : 1;
    unsigned char m_4a_7 : 1;
    unsigned char m_4b_0 : 1;   // 0x4b
    CPnt3D m_4c;   // 0x4c: zero by default
    unsigned int m_callback0_index : 8;   // 0x58: SOCOM 1's CZBodyAnim's four
    unsigned int m_callback1_index : 8;
    unsigned int m_callback2_index : 8;
    unsigned int m_callback3_index : 8;
};

/* the body parts a script has object slots for (32; si_script.cpp's, the SEAL's code reads it too) */
extern int siNumBodyObjects;
/* the scripts and their common data alive (SOCOM 1's, si_script.cpp's) */
extern int numSiScript;
extern int numSiCommon;

/* a script: a table and its common data (SOCOM 1's DWARF: 0x18 bytes; SOCOM II's keeps only the
 * common, 8 bytes) and CZBodyAnim, a script a body plays (SOCOM 1's added the id, type and
 * callback indices, which SOCOM II's common holds) */
class CScriptArchive;

class CZSIScript {
public:
    CZSIScript();
    /* made for a name: its common shared from the archive (CreateAndReadIt's, inline) */
    CZSIScript(const char *name, CScriptArchive *archive)
    {
        m_pCommon = 0;
        Share(name, archive);
        numSiScript++;
    }
    virtual ~CZSIScript();
    /* the script read from an archive's key: its common's header, then each object (SOCOM 1's) */
    virtual bool Read(zar::CZAR &zar, int size);
    /* the script laid over a loaded LIP block: the header, the objects' tracks, their names
     * (SOCOM 1's; returns the objects) */
    virtual int ReadLIP(zar::CZAR &zar, const char *name, void *data, int size, void *user);
    /* the object of a name, whole or as a prefix, marked in use (SOCOM 1's) */
    CZSIObject *GetObject(const char *name, bool exact);
    /* the archive walk's callback: a script made for a key, read, kept by the current archive
     * (SOCOM 1's) */
    static int CreateAndReadIt(zar::CZAR &zar, const char *name, void *data, int size, void *user);
    /* the common data of a name, the archive's or made and filed there; counted */
    void Share(const char *const &name, CScriptArchive *archive);

    /* the version an archive's scripts carry, set by each LIP header (SOCOM 1's) */
    static int m_version;

    CZSIScriptCommon *m_pCommon;   // 0x4
};

class CZBodyAnim : public CZSIScript {
public:
    virtual ~CZBodyAnim();
};

class CZAnim;

/* the pointer vectors' at(), out of line (OVL1 0x2679F0; include/vector has no at()) */
extern "C" void *func_002679F0(void *vec, unsigned int i);

/* a ZAnim an anim fires at a time, found by name once (SOCOM 1's, 0x18 bytes) */
class CZBodyAnimCallback {
public:
    CZBodyAnimCallback(const char *name, float t) : m_anim(0)
    {
        m_name = name;
        m_tfrac = t;
        m_lookup = true;
    }

    std::string m_name;   // 0x0
    CZAnim *m_anim;   // 0xc
    bool m_lookup;   // 0x10: the name still to be looked up
    float m_tfrac;   // 0x14
};

/* an archive's callbacks, by index (SOCOM 1's) */
class CZBodyAnimCallbackList {
public:
    ~CZBodyAnimCallbackList();
    int AddCallback(const char *name, float t);
    void ResetAnims();
    void Clear();
    /* the callback of an index, none out of range */
    CZBodyAnimCallback *Get(int i)
    {
        if (i < 0 || i >= m_callbacks.size())
            return 0;
        return *(CZBodyAnimCallback **)func_002679F0(&m_callbacks, i);
    }

    std::vector<CZBodyAnimCallback *> m_callbacks;   // 0x0
};

class CRdrFile;

/* a script's transition speeds from motion.rdr (transition_speed_A and _B), kept by the script's
 * index there when either is set; 12 bytes (ReadMotionRdr news them; the names ours) */
struct MOTION_TRANSITION {
    MOTION_TRANSITION(int index, float a, float b)
    {
        m_index = index;
        m_speedA = a;
        m_speedB = b;
    }

    int m_index;   // 0x0
    float m_speedA;   // 0x4
    float m_speedB;   // 0x8
};

/* an archive of motion scripts (SOCOM 1's; 0x108 bytes in SOCOM II: bone_init.cpp's two statics at
 * 0x415D40 and 0x415E60, the second's destructor record at 0x415E48 right after the first) */
class CScriptArchive {
public:
    /* built closed and empty: the members' constructors out of line (it is five high), then Init,
     * another unit's (bone_init.cpp's initialiser inlines it for both statics) */
    CScriptArchive() : m_zar(0, 0) { Init(); }
    ~CScriptArchive();
    /* every field to rest: no name, no buffer, no scripts, no motion.rdr (0x2888D0) */
    void Init();
    /* every script of the ZAR read through CreateAndReadIt; true when there were some (SOCOM 1's,
     * 0x288710) */
    bool LoadAll();
    /* motion.rdr: read unless held, taken from a caller unless held, given up unless the caller's
     * (0x2886C0, 0x2886A0, 0x288660) */
    CRdrFile *ReadRdr(const char *name, const char *path);
    CRdrFile *SetRdr(CRdrFile *rdr);
    void FreeRdr(CRdrFile *keep);
    /* the common data of a name, or none (0x2885B0) */
    CZSIScriptCommon *FindCommon(const char *name);
    /* each common's id filed under the archive's index in the SEAL's tables, and taken out
     * (0x288510, 0x2884C0) */
    void Register();
    void Unregister();
    /* the scripts' settings read from motion.rdr: false without it or its "animations" (0x287620) */
    bool ReadMotionRdr();
    void SetPresent(bool b) { m_present = b; }
    void SetIndex(int i) { m_index = i; }
    /* the name replaced by a copy (freed first; none for 0) */
    void SetName(const char *name)
    {
        if (m_name)
            _free(m_name, 0, 0);
        m_name = 0;
        m_name = name ? _strdup(name, 0, 0) : 0;
    }

    std::vector<CZSIScript *> m_scripts;   // 0x0: every script read (CreateAndReadIt)
    zar::CZAR m_zar;   // 0xc
    char *m_name;   // 0xc4
    void *m_buffer;   // 0xc8: the ZAR's data, kept once loaded
    int m_bufsize;   // 0xcc
    unsigned short m_nScripts;   // 0xd0
    unsigned char m_index : 2;   // 0xd2: 1 or 2, set while loaded
    unsigned char m_present : 1;   // the archive has been looked for
    CZBodyAnimCallbackList m_callbacks;   // 0xd4
    CRdrFile *m_rdr;   // 0xe0: motion.rdr, while it is read
    std::vector<CZSIScriptCommon *> m_commons;   // 0xe4: as the scripts were read
    std::vector<CZSIScriptCommon *> m_byRdr;   // 0xf0: in motion.rdr's order (0 for a missing one)
    std::vector<MOTION_TRANSITION *> m_transitions;   // 0xfc
};

/* the archive scripts are being read into (bone_init.cpp's, 0x415F70) */
extern CScriptArchive *curArchive;

/* bone_init.cpp's: the common archive (SOCOM 1's boneArchive) and the map's, the version their ZARs
 * open with, and the loaders (SOCOM 1's zBoneOpen, zBoneLoadAll and zBoneClose became one each) */
extern CScriptArchive boneArchive;
extern CScriptArchive mapBoneArchive;
extern int bone_archive_version;
void zBoneUnloadAll(CScriptArchive *archive);
void zBoneLoad();
void zBoneLoadMap(const char *dir, CRdrFile *rdr);
void zBoneUnloadMap();
int zBoneMotionRdrSize();
void *zBoneMotionRdrBuffer();
CScriptArchive *zBoneGetArchive(int which);
CScriptArchive *zBoneArchive();
CScriptArchive *zBoneMapArchive();
void zBoneSetCurArchive(CScriptArchive *archive);
CScriptArchive *zBoneCurArchive();
/* both archives' callbacks to be looked up again (bone_archive.cpp's) */
void zBoneResetCallbacks();


/* the plane and quaternion helpers (bone_math.cpp, SOCOM 1's free functions) */
float LineIntersectPlane(float d, const CPnt3D &n, const CPnt3D &a, const CPnt3D &b);
float PlaneConstant(const CPnt3D &n, const CPnt3D &p);
bool ClipToBox(const CPnt3D &a, const CPnt3D &b, CPnt3D &out, float y0, float y1, float x0, float x1);
void GetFractionalRotations(const CPnt3D *v, float *a, float *b);
float quadW(float t);
CQuat *QFromRot(CQuat *q, const CPnt3D *rot);

#endif
