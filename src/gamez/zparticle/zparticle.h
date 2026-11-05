/* zparticle.h -- the particle system (namespace Particle, OVL1 0x321E00-0x32C03F): sources that
 * emit instances, the properties they share (sequences of scale, colour and texture over a
 * particle's life), the draw buffer. SOCOM 1's units, by their static initialisers: zparticle_buf.cpp,
 * zparticle_instance.cpp, zparticle_source.cpp (tools/s1units.py pairs SOCOM II's 0x402BB0,
 * 0x402C40, 0x402CE0 with them), and one without an initialiser, zparticle_props.cpp (the name ours:
 * the properties' members, 0x3286D0-0x3294AF, split from the instance unit by the world lane's round
 * 23). Layouts are SOCOM 1's DWARF where SOCOM II's code agrees; the header's name is ours.
 *
 * Self-contained: the particle units define the point classes they use here. CPnt4D's copy is the
 * implicit one (the world lane round 22: its out-of-line copy 0x328D60 is still emitted -- Branch
 * calls it, ColorSequenceAdd inlines it -- and only a trivial copy passes a by-value CPnt4D as the
 * original's address, the callee copying it: Instance::Render's CheckExtents call and CheckExtents'
 * single copy of its parameter; a written copy constructor copies in the caller instead). */
#ifndef ZPARTICLE_H
#define ZPARTICLE_H

#include <vector>
#include <limits>
#include <list>

struct PNT4D {
    float x, y, z, w;
};

/* Sony's libvu0 (MAIN) */
extern "C" void sceVu0AddVector(float *v0, float *v1, float *v2);
extern "C" void sceVu0SubVector(float *v0, float *v1, float *v2);
extern "C" void sceVu0ScaleVector(float *v0, float *v1, float s);
extern "C" void sceVu0UnitMatrix(float m[4][4]);
extern "C" void sceVu0MulMatrix(float m0[4][4], float m1[4][4], float m2[4][4]);

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

    /* zmath.h's operators, by value through VU0 (Destruction_Instance::Tick: each result copied out
     * of its temporary, lq/sq between two stack slots, four FPR words into a member) */
    CPnt4D operator*(float s) const
    {
        CPnt4D r;
        sceVu0ScaleVector((float *)&r, (float *)this, s);
        return r;
    }
    CPnt4D operator+(const CPnt4D &b) const
    {
        CPnt4D r;
        sceVu0AddVector((float *)&r, (float *)this, (float *)&b);
        return r;
    }
    CPnt4D operator-(const CPnt4D &b) const
    {
        CPnt4D r;
        sceVu0SubVector((float *)&r, (float *)this, (float *)&b);
        return r;
    }
    /* in place (SOCOM 1's Destruction_Instance::Tick adds a product's temporary onto the point) */
    void operator+=(const CPnt4D &b) { sceVu0AddVector((float *)this, (float *)this, (float *)&b); }
};   // 0x10 bytes

/* a matrix (zmath.h's CMatrix: `CMatrix m;` costs a sceVu0UnitMatrix) */
class CMatrix {
public:
    CMatrix() { sceVu0UnitMatrix(m_matrix); }

    float m_matrix[4][4];
} __attribute__((aligned(16)));
/* a box (zmath.h's CBBox, 0x20 bytes: nothing built for a local) */
class CBBox {
public:
    float m_min[4];   // 0x0
    float m_max[4];   // 0x10
};

/* a point (zmath.h's CPnt3D has no written copy either: a copy is the compiler's -- over the C
 * struct, as zmath.h's: a temporary copied ld/sd + lwc1/swc1, Destruction_Instance::Tick's spin) */
struct PNT3D {
    float x, y, z;
};

class CPnt3D : public PNT3D {
public:
    CPnt3D() {}
    CPnt3D(float ax, float ay, float az)
    {
        x = ax;
        y = ay;
        z = az;
    }

    /* zmath's (out of line in both builds) and its two inline operators */
    void Scale(float s, CPnt3D *out) const;
    void Add(const CPnt3D *b, CPnt3D *out) const;
    CPnt3D operator*(float s) const
    {
        CPnt3D r;
        Scale(s, &r);
        return r;
    }
    CPnt3D &operator+=(const CPnt3D &b)
    {
        Add(&b, this);
        return *this;
    }
};   // 0xc bytes

namespace zdb {
class CTexPalette;
class CModel;
/* a texture and its handle, as much as the destruction manager reads: the handle's texture at 0xc,
 * the texture's palette at 0x1c (node_world.h's classes) */
class CTexture {
public:
    unsigned char _0[0x1c];   // 0x0
    CTexPalette *m_palette;   // 0x1c
};
class CTexHandle {
public:
    unsigned char _0[0xc];   // 0x0
    CTexture *m_texture;   // 0xc
};
/* a visual, as much as the particles read (znode.h's tag_VISUAL_HEAD flags word) */
class CVisual {
public:
    unsigned int m_flags;   // 0x0: tested by mask (znode.h's)
};
/* a buffer's dynamic textures (znode.h's classes: two pointer vectors) */
class CTexList : public std::vector<CTexHandle *> {
};
class CTexPalList : public std::vector<CTexPalette *> {
};
class CDynTexList {
public:
    /* a texture or palette kept for the frame (node_world.h's) */
    void Add(CTexHandle *tex, bool b);
    void Add(CTexPalette *pal, bool b);

    CTexList m_tex_list;   // 0x0
    CTexPalList m_pal_list;   // 0xc
};   // 0x18 bytes
class CNode;
/* a vector of nodes (znode.h's class: the particles' node pool is one, SOCOM 1's
 * __sinit_zparticle.cpp registering ~CNodeVector for it, SOCOM II's 0x1EA340 four this-tests deep) */
class CNodeVector : public std::vector<CNode *> {
};
class CDI;
/* a node's collision pieces (znode.h's class: PullVisuals gathers them; its constructor sets the
 * two search flags, the destruction manager's local inlines it) */
class CDIVector : public std::vector<CDI *> {
public:
    CDIVector()
    {
        m_bStopAtFirstIntersection = 1;
        m_bReturnBackFaceIntersections = 0;
    }

    unsigned char m_bStopAtFirstIntersection : 1;   // 0xc
    unsigned char m_bReturnBackFaceIntersections : 1;   // 0xc
    unsigned char m_padding : 6;   // 0xc
};   // 0x10 bytes
/* zdb::CNode, as much as the particles call (znode.h would bring zmath.h's CPnt4D with it) */
class CNode {
public:
    unsigned char _0[0x58];   // 0x0: tag_NODE_PARAMS
    unsigned short m_type;   // 0x58: 2 for an instance of a model (znode.h's)
    unsigned char _5a[2];   // 0x5a
    unsigned int _5c : 7;   // 0x5c
    unsigned int m_transparent : 1;   // 0x5c: bit 7
    unsigned int _5c_8 : 24;

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
    /* znode.h's: the rotation read and set, the matrix set (node_main.cpp's) */
    void GetRotation(CPnt3D *out) const;
    void SetRotation(const CPnt3D *rot);
    void SetMatrix(const CMatrix *m);
    void DeleteVisuals();
    void AddVisual(CVisual *vis);

    CNodeVector &GetChildren() { return m_child; }
    /* the collision pieces by reference (ours: PullVisuals' walk keeps the vector's address in a
     * saved register, as through GetChildren()) */
    CDIVector &GetDIs() { return m_di; }

    unsigned char _64[4];   // 0x64
    CNodeVector m_child;   // 0x68 (znode.h's)
    CDIVector m_di;   // 0x74
    unsigned char _84[0x9c - 0x84];
    float m_Opacity;   // 0x9c
    unsigned char _a0[0xb4 - 0xa0];
    CNode *m_model;   // 0xb4: an instance's model (znode.h's CModel *)
};

class CCamera {
public:
    /* the eight corners' view test (zcamera.h's: 2 out of view) */
    unsigned char TestFOV(PNT4D *bbox8, unsigned int *clipWord, bool testGuardBand);

    float m_matrix[4][4];   // 0x0: the node's matrix, its position the last row (CNode's)
};
/* the world, as much as the particles use (znode.h's class brings zmath.h's CPnt4D with it) */
class CWorld : public CNode {
public:
    static CWorld *m_world;
    /* a loaded texture's handle and a model by name (node_world.h's statics) */
    static CTexHandle *GetTexHandle(const char *name);
    static CModel *GetModel(const char *name);

    unsigned char _b8[0xc0 - 0xb8];
    PNT4D m_Wind;   // 0xc0: the wind (znode.h's CWind's first member, the world's m_Wind)
    unsigned char _d0[0x128 - 0xd0];
    CCamera *m_camera;   // 0x128
};
}

/* a particle effect's collision node (no destructor: deleted with operator delete) */
class CParticleEx {
public:
    int _0;
};

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
template <class T>
class Sequence : public std::vector<SequenceEntry<T> > {
public:
    Sequence() {}
    /* the copy (out of line: 0x328E40 the textures', 0x328DE0 the colours', 0x328D90 the scales'):
     * MSL's vector copy -- the words zeroed by the base's constructors, then the range init over
     * the other's elements (include/vector: 0x328FC0, 0x3290D0, 0x329220) */
    Sequence(const Sequence &s) : std::vector<SequenceEntry<T> >(s) {}
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
        /* the head cleared (inline: zparticle_instance.cpp's initialiser stores it after the
         * deleter's constructor, which it calls) */
        List() : m_head(0) {}
        ~List();
        void Tick(float dT);
        void Uninit();
        /* SOCOM II's: the instances past their life recycled (the name ours) */
        void RemoveDead();
        /* SOCOM II's: a dying source's instances let go of (0x327F70; the name ours) */
        void RemoveSource(Source *source);

        bool Empty() const { return !m_head; }

        Instance *m_head;   // 0xc: linked through m_listNext (SOCOM II's; the list itself unused)
    };

    /* whether a link is the list's end (ours: the target materialises each null test) */
    static bool End(const Instance *p) { return !p; }
    bool Alive() const { return m_age < m_lifespan; }
    /* whether its source ages it at the LOD's rate (SOCOM 1's test of m_doAgeLOD; the name ours) */
    bool AgeLOD() const;

    /* SOCOM 1's (0x327CD0; Source::Tick calls it): the CPnt3D is unused in SOCOM II */
    Instance(const CPnt4D &pos, const CPnt4D &vel, const CPnt3D &unused, float f, float life, float age,
             zdb::CTexHandle *tex, Props *props, Source *source);
    /* SOCOM 1's constructor's common part (inline: Destruction_Instance's constructor inlines it):
     * no properties, no source, not linked -- stored in this order, as the target's */
    Instance()
    {
        m_props = 0;
        m_source = 0;
        m_listNext = 0;
    }
    /* inline: its table stored and the virtual Expire, in every destructor that inlines it (no
     * out-of-line copy in the image) */
    ~Instance() { Expire(); }

    virtual void Expire();   // +0x08
    virtual void Tick(float dT);   // +0x0c
    /* SOCOM 1's Render(const CMatrix &): SOCOM II's takes no matrix -- neither override reads a1,
     * and Particle::Render's call (0x328040) sets none */
    virtual void Render();   // +0x10
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
    zdb::CTexHandle *m_texture;   // 0x8 (the constructor's; the name ours)
    float m_age;   // 0xc
    float m_10[4];   // 0x10: a streak's start (its position), a rotated one's angle (0x14) and spin (0x18)
    float m_pos[4];   // 0x20 (SOCOM 1's constructor copies a CPnt4D in)
    float m_vel[4];   // 0x30: the velocity (a CPnt4D copied in; the name ours)
    Props *m_props;   // 0x40
    float m_44;   // 0x44: the constructor's first float (the name ours)
    float m_lifespan;   // 0x48
    float m_invLifespan;   // 0x4c: 1 / the life (ours)
    Instance *m_listNext;   // 0x50: the next live instance (List's link), cleared when recycled
    float m_opacity;   // 0x54 (Destruction_Instance::SetVisual hands it to the node; the name ours)
};   // 0x58 bytes

/* a piece of a destroyed object flying off (SOCOM 1's Destruction_Instance): its life at 0x48, the
 * node it moves at 0x58 */
class Destruction_Instance : public Instance {
public:
    /* SOCOM 1's, the visual dropped (SetVisual hands it over): the position and velocity, the
     * spin, the first float, the life and the opacity, the axis (by value) and the properties,
     * whose count it takes (0x327800) */
    Destruction_Instance(const CPnt4D &pos, const CPnt4D &vel, const CPnt3D &spin, float f, float life,
                         float opacity, CPnt3D axis, Props *props);
    ~Destruction_Instance();
    /* SOCOM 1's: one off the pool, built in place, else a new one; linked at the live list's
     * head (0x327620) */
    static Destruction_Instance *Create(const CPnt4D &pos, const CPnt4D &vel, const CPnt3D &spin, float f,
                                        float life, float opacity, CPnt3D axis, Props *props);
    void Expire();
    void Tick(float dT);
    void Render();
    void Recycle();
    /* SOCOM II's: the piece's visual handed to its node (SOCOM 1 passed it to Create; the name
     * ours) */
    void SetVisual(zdb::CVisual *vis);

    static InstancePool<Destruction_Instance> m_instance_pool;   // 0x488468

    zdb::CNode *m_node;   // 0x58
    float m_spin[3];   // 0x5c (the names ours)
    float m_axis[3];   // 0x68
};   // 0x74 bytes (Create's new)

/* one particle as drawn (SOCOM 1's BufferElement, 0x20 bytes; no DWARF): Instance::Render fills
 * it -- the colour, the instance, the size, the texture, the distance (the members' names ours) */
class BufferElement {
public:
    /* nothing built (SOCOM 1's: the initialiser constructs the buffer through it, its out-of-line
     * copy 0x326F40 the unit's last function) */
    BufferElement() {}
    void Draw(float (&m)[4][4]);
    void DrawRotated(float (&m)[4][4]);
    void DrawXZ(float (&m)[4][4]);
    void DrawStreaked(float (&m)[4][4]);

    CPnt4D m_color;   // 0x0: the properties' colour, its alpha by the opacity, all by 128
    Instance *m_instance;   // 0x10
    float m_scale;   // 0x14
    zdb::CTexture *m_texture;   // 0x18
    float m_depth;   // 0x1c: the distance squared from the camera
};   // 0x20 bytes

/* the frame's elements (zparticle_buf.cpp's: SOCOM 1's buffer and numParticles) */
extern BufferElement buffer[5000];
extern int numParticles;

/* SOCOM II's: the dead instances recycled and both pools emptied (zparticle_instance.cpp; the
 * name ours) */
void FreeInstances();
/* a node for an instance, off the pool or new (zparticle.cpp) */
zdb::CNode *GetFreeInstance();


/* the textures a particle draws with, registered with the draw buffer (0x324A10) */
void AddDynTexture(zdb::CTexHandle *tex);

/* how a source's particles are drawn (SOCOM 1's PARTICLE_TYPE; the enumerators' names ours, by
 * the draw buffer's functions) */
enum PARTICLE_TYPE {
    PT_BILLBOARD = 0,
    PT_ROTATED = 1,
    PT_XZ = 2,
    PT_STREAKED = 3
};

/* an emitter (SOCOM 1's DWARF: 0x134 bytes there). SOCOM II's members sit one word later from
 * the texture list on (0xDC; 0xD8 in SOCOM 1) -- the word inserted before it is not located yet,
 * so the members ahead of the list are opaque here */
class Source {
public:
    /* a free source taken from the pool (0x32B120; ZAnim's particle commands call it) */
    static Source *Create();
    /* the live sources (SOCOM 1's Source::List, a list of them) */
    class List : public std::list<Source *> {
    public:
        /* every source deleted first (inline: its copy 0x32BFD0 in zparticle_source.cpp's tail,
         * the initialiser's registered destructor, calls Uninit -- in tail code Uninit's clear
         * counts the range erase, too tall to inline whole -- and ~__list_imp) */
        ~List() { Uninit(); }
        void Uninit();
    };

    ~Source();
    /* the draw type (SOCOM 1's inline: the instance's tick reads it through a call, lb + andi) */
    PARTICLE_TYPE GetDrawType() const { return m_drawType; }
    static void RenderBBox();
    void CheckExtents(CPnt4D p, float r);
    void GetBBox(CBBox *box);
    void Tick(float dT);
    void ScaleSequenceAdd(float fraction, float scale);
    void ScaleSequenceClear();
    void ColorSequenceAdd(float fraction, float r, float g, float b, float a);
    void ColorSequenceClear();
    void TextureSequenceAdd(float fraction, zdb::CTexHandle *tex);
    void TextureSequenceClear();

    unsigned char _0[2];   // 0x0: SOCOM 1's m_active, m_goodPos
    unsigned char m_frozen;   // 0x2 (SOCOM 1's: an instance of a frozen source is not ticked)
    unsigned char m_outOfRange;   // 0x3 (SOCOM 1's)
    unsigned char _4[0x19 - 4];   // 0x4: SOCOM 1's m_oldPosW .. m_landmark, and SOCOM II's word
    unsigned char m_doAgeLOD;   // 0x19 (SOCOM 1: 0x15)
    unsigned char _1a[0x2c - 0x1a];   // 0x1a: SOCOM 1's m_doSpawnLOD .. m_streakFade
    float m_minRot;   // 0x2c (SOCOM 1's names, 4 bytes later: the instance constructor's ranges)
    float m_maxRot;   // 0x30
    float m_minRotAcc;   // 0x34
    float m_maxRotAcc;   // 0x38
    PARTICLE_TYPE m_drawType;   // 0x3c (SOCOM 1: 0x38)
    unsigned char _3d[0xdc - 0x3d];   // 0x3d: SOCOM 1's m_numPeriods .. m_initialAgeRange
    std::vector<zdb::CTexHandle *> m_textures;   // 0xdc
    Props *m_props;   // 0xe8
    CPnt3D m_bboxMin;   // 0xec: SOCOM 1's m_bbox (a CBBox of two points there)
    CPnt3D m_bboxMax;   // 0xf8
    zdb::CNode *m_diNode;   // 0x104
    unsigned char _108[0x18];   // 0x108: SOCOM 1's m_diPolies
    unsigned char m_hasbbox;   // 0x120
    float m_filled_volume;   // 0x124
    CParticleEx *m_particleEx;   // 0x128
    unsigned char m_shuttingoff;   // 0x12c (SOCOM 1: 0x128)
    unsigned char m_fovFlag;   // 0x12d (SOCOM 1's tag_ZVIS_FOV: 2 never drawn, 1 drawn untested)

    static float distanceFactor;   // 0x4884C0
    static float frequencyFactor;   // 0x4884C8
    static CPnt3D referencePositionW;   // 0x4884D0: the camera's position, each tick
    static List list;   // 0x4884F0
};

/* VU0: d.xyz += s.xyz * f (SOCOM II's: the scale moved from an FPU register, where SOCOM 1's
 * inline took it as a word; the name ours) */
inline void AddScaled(float *d, const float *s, float f)
{
    asm __volatile__ (
        "lqc2 $vf7, 0(%0)\n"
        "lqc2 $vf4, 0(%1)\n"
        "mfc1 $8, %2\n"
        "nop\n"
        "qmtc2 $8, $vf5\n"
        "vmulx.xyz $vf6, $vf4, $vf5\n"
        "vadd.xyz $vf7, $vf7, $vf6\n"
        "sqc2 $vf7, 0(%0)\n"
        : : "r"(d), "r"(s), "f"(f) : "$8", "memory");
}

/* e^x by halving into [0, 0.5], a series and squaring back (SOCOM 1's inline, in both builds'
 * instance ticks; the name ours) */
inline float FastExp(float x)
{
    int neg;

    if (x < 0.0f) {
        neg = 1;
        x = -x;
    } else
        neg = 0;
    int n = 0;

    while (x > 0.5f) {
        n++;
        x *= 0.5f;
    }
    float r = 1.0f + x * (0.5f + x * (0.333333f + x * (0.25f + x * (0.2f + (0.166666f + x) * x))));

    while (n != 0) {
        n--;
        r *= r;
    }
    if (neg)
        r = 1.0f / r;
    return r;
}

inline bool Instance::AgeLOD() const
{
    bool r = false;

    if (m_source && m_source->m_doAgeLOD)
        r = true;
    return r;
}

}

#endif
