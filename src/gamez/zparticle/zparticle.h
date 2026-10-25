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
class CModel;
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

/* a live particle (SOCOM 1's Instance; no DWARF): its age at 0xC, its position from 0x20 (the
 * members' names ours, the rest opaque) */
class Instance {
public:
    /* the live instances (SOCOM 1's Instance::List; its members out of line) */
    class List {
    public:
        void Tick(float dT);

        unsigned char _0[0xc];
    };   // 0xc bytes

    virtual void Render(const CMatrix &m);

    static float fadeFactor;   // 0x488440
    static float ageFactor;   // 0x488448
    static int spawnFactor;   // 0x488450
    static List list;   // 0x488480

    unsigned char _4[0x8];   // 0x4
    float m_age;   // 0xc
    unsigned char _10[0x10];   // 0x10
    float m_pos[3];   // 0x20
};

/* a piece of a destroyed object flying off (SOCOM 1's Destruction_Instance): its life at 0x48, the
 * node it moves at 0x58 */
class Destruction_Instance : public Instance {
public:
    void Render(const CMatrix &m);

    unsigned char _2c[0x1c];   // 0x2c
    float m_lifespan;   // 0x48
    unsigned char _4c[0xc];   // 0x4c
    zdb::CNode *m_node;   // 0x58
};

/* the textures a particle draws with, registered with the draw buffer (0x324A10) */
void AddDynTexture(zdb::CTexHandle *tex);

/* an emitter (SOCOM 1's DWARF: 0x134 bytes there). SOCOM II's members sit one word later from
 * the texture list on (0xDC; 0xD8 in SOCOM 1) -- the word inserted before it is not located yet,
 * so the members ahead of the list are opaque here */
class Source {
public:
    /* the live sources (SOCOM 1's Source::List, a list of them) */
    class List : public std::list<Source *> {
    };

    void Tick(float dT);
    void ScaleSequenceAdd(float fraction, float scale);
    void ScaleSequenceClear();
    void ColorSequenceAdd(float fraction, float r, float g, float b, float a);
    void ColorSequenceClear();
    void TextureSequenceAdd(float fraction, zdb::CTexHandle *tex);
    void TextureSequenceClear();

    unsigned char _0[0xdc];   // 0x0: SOCOM 1's m_active .. m_initialAgeRange, and SOCOM II's word
    std::vector<zdb::CTexHandle *> m_textures;   // 0xdc
    Props *m_props;   // 0xe8

    static float distanceFactor;   // 0x4884C0
    static float frequencyFactor;   // 0x4884C8
    static CPnt3D referencePositionW;   // 0x4884D0: the camera's position, each tick
    static List list;   // 0x4884F0
};

}

#endif
