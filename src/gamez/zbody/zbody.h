/* zbody.h -- the animated body: CBody (a base of CZSealBody), its parts (CZBodyPart), the motion
 * scripts it plays (CZSIScript and its objects, CZBodyAnim), the archives they load from
 * (CScriptArchive) and the small helpers between them (CSequence, the plane and quaternion
 * functions). OVL1 0x2869D0-0x290360, five units in SOCOM 1's link order: bone_math.cpp (ours: the
 * helpers), bone_archive.cpp and bone_init.cpp (the press beta's asserts name both), si_object.cpp
 * (the same), si_script.cpp and body.cpp (SOCOM 1's static initialisers name them).
 *
 * The layouts are SOCOM 1's DWARF moved to where SOCOM II's code puts each member; only what a
 * written function touches is named. Self-contained over zmath.h, znode.h and zar.h, so zseal.h
 * can include it for CZSealBody's CBody base. The header's name is ours. */
#ifndef ZBODY_H
#define ZBODY_H

#include <list>
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
    LIST_ITEM *p = m_pool;

    if (!m_tail) {
        p->m_item = item;
        m_head = p;
        m_tail = p;
        return item;
    }
    for (unsigned int i = 0; i < m_size; i++, p++) {
        if (p != m_tail && !p->m_item) {
            m_tail->m_next = p;
            p->m_prev = m_tail;
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
    void PutBack(T *item);
    T *GetNext(bool b);

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

class CZBodyAnim;

/* an animation playing in a blend (SOCOM 1's DWARF: 0x20 bytes; SOCOM II's is 0x2c) */
class ACTIVE_ANIM {
public:
    CZBodyAnim *m_anim;   // 0x0
    float m_4;   // 0x4
    float m_wt;   // 0x8: (SetRepresentativeWt)
    float m_c;   // 0xc
    ACTIVE_ANIM *m_overlay;   // 0x10
    float m_14;   // 0x14
    float m_18;   // 0x18: 1 when cleared, copied to an overlay
    float m_1c;   // 0x1c
    short m_20;   // 0x20
    short m_id;   // 0x22: -1 when cleared (FindAnim)
    float m_playback;   // 0x24: 1 when cleared, negated by ReverseDir
    unsigned char m_bit0 : 1;   // 0x28
    unsigned char m_bit1 : 1;
};

class CActiveAnimList : public RTList<ACTIVE_ANIM> {
public:
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

/* a blend of animations (SOCOM 1's DWARF: 0x84 bytes; SOCOM II's list of anims is a member) --
 * only what the written functions touch */
class CZBodyAnimBlend {
public:
    /* every field to rest, its anims' list cleared (0x28CEA0, not written yet) */
    void Clear();
    int CountAnims();
    ACTIVE_ANIM *FindAnim(int id);
    void AddOverlay(ACTIVE_ANIM *anim, ACTIVE_ANIM *overlay, float wt);
    void ReverseDir();
    ACTIVE_ANIM *Representative();
    void SetRepresentativeWt(float wt);
    int ThisFoot();

    CPnt3D m_dpos;   // 0x0
    float m_quitT;   // 0xc
    CPnt3D m_drot;   // 0x10
    float m_tfrac;   // 0x1c
    unsigned char _20[0x28 - 0x20];
    unsigned short m_flags;   // 0x28
    unsigned char _2a[0x30 - 0x2a];
    CActiveAnimList m_anims;   // 0x30
};

/* the body's pools (SOCOM 1's DWARF: CActiveAnimPool and CBlendPool, each an RTArray) and its stack
 * of blends (CBodyAnimBlendStack : RTStack) */
class CActiveAnimPool : public RTArray<ACTIVE_ANIM> {
public:
};

class CBlendPool : public RTArray<CZBodyAnimBlend> {
public:
    /* a blend cleared as it goes back, or as it comes out (SOCOM II's; the names ours) */
    void PutBack(CZBodyAnimBlend *blend);
    CZBodyAnimBlend *GetNext();
};

class CBodyAnimBlendStack : public RTStack<CZBodyAnimBlend> {
public:
};

/* the bone id of a node's name (OVL2 0x58CB30, the SEAL lane's range; not named yet) */
extern "C" int func_0058CB30(const char *name);

/* the animated body (SOCOM 1's DWARF: 0x48 bytes, its table first; SOCOM II's is 0x90 -- its
 * constructor, 0x28EDD0, zeroes through 0x8C). SOCOM II's table (0x406100) has two slots, the
 * destructor and NullNode, an inline returning -1. */
class CBody {
public:
    virtual ~CBody();
    virtual int NullNode() const { return -1; }

    /* a part's position and rotation in the body's space: up its chain of parents (SOCOM 1's) */
    void PartInSpace(CZBodyPart *part, const CPnt3D *offset, CPnt3D *pos, CQuat *rot) const;
    /* the parts: one made for a node (its bone id from the node's name), filed by id, and a
     * node's whole tree of them (SOCOM 1's) */
    CZBodyPart *NewBodyPart(CNode *node, CZBodyPart *parent);
    void AddBodyPart(CNode *node, CZBodyPart *parent);
    CZBodyPart *BodySetup(CNode *node, CZBodyPart *parent);

    short m_state;   // 0x4: the SEAL's state (SOCOM 1's char at 0xd)
    unsigned char _6[0x10 - 0x6];   // 0x8, 0xc: zeroed by the constructor
    CActiveAnimPool m_active_anim_pool;   // 0x10 (SOCOM 1's a pointer)
    CBlendPool m_blend_pool;   // 0x30
    CBodyAnimBlendStack m_blends;   // 0x50
    std::vector<CZBodyPart *> m_body_parts;   // 0x5c: by bone id
    std::vector<CZBodyAnim *> m_body_anims;   // 0x68
    unsigned char m_overlay[0x1c];   // 0x74: a CBlendOverlay (SOCOM 1's a pointer at 0x8)
};

class CZSIScriptCommon;

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
    bool Initalize(CZSIScriptCommon *common);

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

/* the plane and quaternion helpers (bone_math.cpp, SOCOM 1's free functions) */
float LineIntersectPlane(float d, const CPnt3D &n, const CPnt3D &a, const CPnt3D &b);
float PlaneConstant(const CPnt3D &n, const CPnt3D &p);
void ClipToBox(const CPnt3D &a, const CPnt3D &b, CPnt3D &out, float x0, float x1, float z0, float z1);
void GetFractionalRotations(const CPnt3D *v, float *a, float *b);
float quadW(float t);
CQuat *QFromRot(CQuat *q, const CPnt3D *rot);

#endif
