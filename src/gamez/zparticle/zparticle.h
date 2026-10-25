/* zparticle.h -- the particle system (namespace Particle, OVL1 0x321E00-0x32C03F): sources that
 * emit instances, the properties they share (sequences of scale, colour and texture over a
 * particle's life), the draw buffer. SOCOM 1's units, by their static initialisers: zparticle_buf.cpp,
 * zparticle_instance.cpp, zparticle_source.cpp (tools/s1units.py pairs SOCOM II's 0x402BB0,
 * 0x402C40, 0x402CE0 with them). Layouts are SOCOM 1's DWARF where SOCOM II's code agrees; the
 * header's name is ours.
 *
 * Self-contained: SOCOM II's CPnt4D has a written copy constructor (its out-of-line copy is
 * 0x328D60, in this range; ColorSequenceAdd inlines it), which zmath.h's CPnt4D does not declare,
 * so the particle units define the point classes they use here. */
#ifndef ZPARTICLE_H
#define ZPARTICLE_H

#include <vector>
#include <list>

struct PNT4D {
    float x, y, z, w;
};

class CPnt4D : public PNT4D {
public:
    CPnt4D() {}
    CPnt4D(float ax, float ay, float az, float aw)
    {
        x = ax;
        y = ay;
        z = az;
        w = aw;
    }
    CPnt4D(const CPnt4D &p) : PNT4D(p) {}
};   // 0x10 bytes

class CMatrix;

/* a point (zmath.h's CPnt3D has no written copy either: a copy is the compiler's) */
class CPnt3D {
public:
    CPnt3D() {}
    CPnt3D(float ax, float ay, float az)
    {
        x = ax;
        y = ay;
        z = az;
    }

    float x, y, z;
};   // 0xc bytes

namespace zdb {
class CTexHandle;
class CTexPalette;
class CModel;
/* a buffer's dynamic textures (znode.h's classes: two pointer vectors) */
class CTexList : public std::vector<CTexHandle *> {
};
class CTexPalList : public std::vector<CTexPalette *> {
};
class CDynTexList {
public:
    CTexList m_tex_list;   // 0x0
    CTexPalList m_pal_list;   // 0xc
};   // 0x18 bytes
/* zdb::CNode, as much as the particles call (znode.h would bring zmath.h's CPnt4D with it) */
class CNode {
public:
    unsigned char _0[0x60];   // 0x0: tag_NODE_PARAMS

    /* SOCOM II's vtable order (znode.h's), the pointer at 0x60; declared only */
    virtual ~CNode();
    virtual CNode *_Copy(CNode *to) const;
    virtual void UpdateGrid();
    virtual void ReserveChildren(int n);
    virtual void AddChild(CNode *child);   // +0x18
    virtual int DeleteChild(CNode *child);   // +0x1c
    virtual void DeleteChildren();
    virtual CNode *FindChild(const char *name, bool deep);
    virtual CNode *FindChild(CNode *node, bool deep);
    virtual CNode *FindChild(const char *name, int start, int *index, bool deep);
    virtual bool IsCell();
    virtual bool Read(class CSaveLoad &io);
    virtual bool SetActive(bool on);   // +0x38

    static CNode *Create(const char *name);
    static void Delete(CNode *node);
    void SetPosition(float x, float y, float z);
    void DeleteVisuals();
};

class CCamera {
public:
    float m_matrix[4][4];   // 0x0: the node's matrix, its position the last row (CNode's)
};
/* the world, as much as the particles use (znode.h's class brings zmath.h's CPnt4D with it) */
class CWorld : public CNode {
public:
    static CWorld *m_world;

    unsigned char _64[0x128 - 0x64];
    CCamera *m_camera;   // 0x128
};
}

namespace std {

/* <limits>, not in include/: just numeric_limits<float>::max(), whose out-of-line copy Props'
 * constructor calls (0x328F40) */
template <class T>
class numeric_limits;

template <>
class numeric_limits<float> {
public:
    static float max() { return 3.40282347e+38f; }
};

}

namespace Particle {

class Source;

/* a free list threaded through its objects' second word (SOCOM 1's instance pool, SOCOM II's two;
 * the class is ours): Close and the purge pop it through an inline whose emptiness test is an
 * inline bool of its own, a negation (the target materialises it, sltu/xori, before the branch)
 * and the head read once into the result */
template <class T>
class InstancePool {
public:
    bool Empty() const { return !m_head; }
    T *Pop()
    {
        T *p = m_head;

        if (!Empty()) {
            m_head = (T *)p->m_next;
            p->m_next = 0;
        } else
            p = 0;
        return p;
    }

    /* an object given back, linked in front: the link a select (the target computes it into one
     * register on both arms and stores it in each; an if/else stores zero directly) */
    void Push(T *p)
    {
        p->m_next = m_head ? m_head : 0;
        m_head = p;
    }

    T *m_head;
};

/* one key of a sequence: the fraction of the life it applies from, its value (SOCOM 1's DWARF) */
template <class T>
class SequenceEntry {
public:
    SequenceEntry(float fraction, const T &data) : m_fraction(fraction), m_data(data) {}

    float m_fraction;   // 0x0
    T m_data;   // 0x4
};

/* a sequence of keys (SOCOM 1's DWARF names the members' type Sequence): a vector of entries, a
 * class of its own -- Props' implicit destructor tests three levels of each before the deleter's
 * destructor, where a plain vector has two */
/* MSL's range init of each sequence's vector (__vector_imp::init<const T *>(first, last,
 * forward_iterator_tag), out of line in this unit's template tail: 0x328FC0 the textures', 0x3290D0
 * the colours', 0x329220 the scales'). include/vector has neither a class vector's copy constructor
 * nor its range init yet, so the sequences' copies call them as placeholders, picked per type */
extern "C" void func_00328FC0(void *v, const void *first, const void *last, std::forward_iterator_tag);
extern "C" void func_003290D0(void *v, const void *first, const void *last, std::forward_iterator_tag);
extern "C" void func_00329220(void *v, const void *first, const void *last, std::forward_iterator_tag);

template <class T>
class SequenceEntry;

template <class T>
struct __seq_init {
};
template <>
struct __seq_init<zdb::CTexHandle *> {
    static void init(void *v, const void *first, const void *last)
    {
        func_00328FC0(v, first, last, std::forward_iterator_tag());
    }
};
template <>
struct __seq_init<CPnt4D> {
    static void init(void *v, const void *first, const void *last)
    {
        func_003290D0(v, first, last, std::forward_iterator_tag());
    }
};
template <>
struct __seq_init<float> {
    static void init(void *v, const void *first, const void *last)
    {
        func_00329220(v, first, last, std::forward_iterator_tag());
    }
};

template <class T>
class Sequence : public std::vector<SequenceEntry<T> > {
public:
    Sequence() {}
    /* the copy (out of line: 0x328E40 the textures', 0x328DE0 the colours', 0x328D90 the scales'):
     * the words zeroed by the base's constructors, then the range init over the other's elements
     * -- MSL's vector copy, five high with the base chain, so Props' copy is inlined one level */
    Sequence(const Sequence &s)
    {
        __seq_init<T>::init(this, s.data_, s.data_ + s.size_);
    }
    /* MSL's const element read (include/vector has only the non-const one) */
    const SequenceEntry<T> &operator[](unsigned int n) const { return this->data_[n]; }
};

/* the properties sources share, copied on write (Branch) and counted (SOCOM 1's DWARF, 0x68) */
class Props {
public:
    /* SOCOM 1's defaults: one reference, no fade (the far fade at FLT_MAX), wind at full. Too tall
     * to inline whole (the sequences' constructors are five high), so Create inlines it one level
     * and calls the members' constructors and numeric_limits::max() out of line */
    Props()
        : m_refCount(1), m_model(0), m_friction(0.0f), m_windFactor(1.0f), m_priority(0.0f),
          m_nearFade1(0.0f), m_nearFade2(0.0f), m_farFade1(std::numeric_limits<float>::max()),
          m_farFade2(std::numeric_limits<float>::max()), m_invNearFade(0.0f), m_invFarFade(0.0f),
          m_visualDensity(0.0f), m_checkFade(false)
    {
        m_accelW.x = 0.0f;
        m_accelW.y = 0.0f;
        m_accelW.z = 0.0f;
        m_accelW.w = 0.0f;
    }
    /* the copy Branch makes, its own reference (SOCOM 1's: the count set first, then every member
     * copied in order). Too tall to inline whole, so Branch inlines it one level and calls the
     * members' copy constructors -- CPnt4D's too (0x328D60) */
    Props(const Props &p)
        : m_refCount(1), m_textureSeq(p.m_textureSeq), m_colorSeq(p.m_colorSeq),
          m_scaleSeq(p.m_scaleSeq), m_model(p.m_model), m_friction(p.m_friction),
          m_accelW(p.m_accelW), m_windFactor(p.m_windFactor), m_priority(p.m_priority),
          m_nearFade1(p.m_nearFade1), m_nearFade2(p.m_nearFade2), m_farFade1(p.m_farFade1),
          m_farFade2(p.m_farFade2), m_invNearFade(p.m_invNearFade), m_invFarFade(p.m_invFarFade),
          m_visualDensity(p.m_visualDensity), m_checkFade(p.m_checkFade)
    {
    }
    float GetOpacity(float dist) const;
    void GetScale(float age, float &scale) const;
    void GetColor(float age, CPnt4D &color) const;
    void GetTexture(float age, zdb::CTexHandle *&tex) const;
    void DecRefCount(int n);
    Props *Branch();
    static Props *Create();

    int m_refCount;   // 0x0
    Sequence<zdb::CTexHandle *> m_textureSeq;   // 0x4
    Sequence<CPnt4D> m_colorSeq;   // 0x10
    Sequence<float> m_scaleSeq;   // 0x1c
    zdb::CModel *m_model;   // 0x28
    float m_friction;   // 0x2c
    CPnt4D m_accelW;   // 0x30
    float m_windFactor;   // 0x40
    float m_priority;   // 0x44
    float m_nearFade1;   // 0x48
    float m_nearFade2;   // 0x4c
    float m_farFade1;   // 0x50
    float m_farFade2;   // 0x54
    float m_invNearFade;   // 0x58
    float m_invFarFade;   // 0x5c
    float m_visualDensity;   // 0x60
    unsigned char m_checkFade;   // 0x64
};   // 0x68 bytes

/* a live particle (SOCOM 1's Instance; no DWARF): its age at 0xC, its position from 0x20, its
 * properties at 0x40 (the members' names ours, the rest opaque). SOCOM II's table: Expire (new,
 * first), Tick, Render, Recycle (new) */
class Instance {
public:
    /* the live instances (SOCOM 1's Instance::List; its members out of line) */
    class List : public std::list<Instance *> {
    public:
        ~List();
        void Tick(float dT);
        void Uninit();
        /* SOCOM II's: the instances past their life recycled (the name ours) */
        void RemoveDead();

        bool Empty() const { return !m_head; }

        Instance *m_head;   // 0xc: linked through m_listNext (SOCOM II's; the list itself unused)
    };

    /* whether a link is the list's end (ours: the target materialises each null test) */
    static bool End(const Instance *p) { return !p; }
    bool Alive() const { return m_age < m_lifespan; }
    /* whether its source ages it at the LOD's rate (SOCOM 1's test of m_doAgeLOD; the name ours) */
    bool AgeLOD() const;

    /* inline: its table stored and the virtual Expire, in every destructor that inlines it (no
     * out-of-line copy in the image) */
    ~Instance() { Expire(); }

    virtual void Expire();   // +0x08
    virtual void Tick(float dT);   // +0x0c
    virtual void Render(const CMatrix &m);   // +0x10
    /* SOCOM II's: expired and given back to the class's pool (the name ours) */
    virtual void Recycle();   // +0x14

    static float fadeFactor;   // 0x488440
    static float ageFactor;   // 0x488448
    static int spawnFactor;   // 0x488450
    static float m_dT;   // 0x488458: the tick's step
    static List list;   // 0x488480
    static InstancePool<Instance> m_instance_pool;   // 0x488460

    union {
        Source *m_source;   // 0x4: the emitter (SOCOM 1's; Expire drops its count)
        Instance *m_next;   // 0x4: the next free instance, in a pool
    };
    unsigned char _8[0x4];   // 0x8
    float m_age;   // 0xc
    unsigned char _10[0x10];   // 0x10
    float m_pos[3];   // 0x20
    unsigned char _2c[0x14];   // 0x2c
    Props *m_props;   // 0x40
    unsigned char _44[4];   // 0x44
    float m_lifespan;   // 0x48
    unsigned char _4c[4];   // 0x4c
    Instance *m_listNext;   // 0x50: the next live instance (List's link), cleared when recycled
    unsigned char _54[4];   // 0x54
};   // 0x58 bytes

/* a piece of a destroyed object flying off (SOCOM 1's Destruction_Instance): its life at 0x48, the
 * node it moves at 0x58 */
class Destruction_Instance : public Instance {
public:
    ~Destruction_Instance();
    void Expire();
    void Tick(float dT);
    void Render(const CMatrix &m);
    void Recycle();

    static InstancePool<Destruction_Instance> m_instance_pool;   // 0x488468

    zdb::CNode *m_node;   // 0x58
};

/* SOCOM II's: the dead instances recycled and both pools emptied (zparticle_instance.cpp; the
 * name ours) */
void FreeInstances();


/* the textures a particle draws with, registered with the draw buffer (0x324A10) */
void AddDynTexture(zdb::CTexHandle *tex);

/* an emitter (SOCOM 1's DWARF: 0x134 bytes there). SOCOM II's members sit one word later from
 * the texture list on (0xDC; 0xD8 in SOCOM 1) -- the word inserted before it is not located yet,
 * so the members ahead of the list are opaque here */
class Source {
public:
    /* the live sources (SOCOM 1's Source::List, a list of them) */
    class List : public std::list<Source *> {
    public:
        void Uninit();
    };

    void Tick(float dT);
    void ScaleSequenceAdd(float fraction, float scale);
    void ScaleSequenceClear();
    void ColorSequenceAdd(float fraction, float r, float g, float b, float a);
    void ColorSequenceClear();
    void TextureSequenceAdd(float fraction, zdb::CTexHandle *tex);
    void TextureSequenceClear();

    unsigned char _0[0x19];   // 0x0: SOCOM 1's m_active .. m_landmark, and SOCOM II's word
    unsigned char m_doAgeLOD;   // 0x19 (SOCOM 1: 0x15)
    unsigned char _1a[0xdc - 0x1a];   // 0x1a: SOCOM 1's m_doSpawnLOD .. m_initialAgeRange
    std::vector<zdb::CTexHandle *> m_textures;   // 0xdc
    Props *m_props;   // 0xe8

    static float distanceFactor;   // 0x4884C0
    static float frequencyFactor;   // 0x4884C8
    static CPnt3D referencePositionW;   // 0x4884D0: the camera's position, each tick
    static List list;   // 0x4884F0
};

inline bool Instance::AgeLOD() const
{
    bool r = false;

    if (m_source && m_source->m_doAgeLOD)
        r = true;
    return r;
}

}

#endif
