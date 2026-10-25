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

namespace zdb {
class CTexHandle;
class CModel;
/* zdb::CNode, as much as the particles call (znode.h would bring zmath.h's CPnt4D with it) */
class CNode {
public:
    void SetPosition(float x, float y, float z);
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
template <class T>
class Sequence : public std::vector<SequenceEntry<T> > {
public:
    /* MSL's const element read (include/vector has only the non-const one) */
    const SequenceEntry<T> &operator[](unsigned int n) const { return this->data_[n]; }
};

/* the properties sources share, copied on write (Branch) and counted (SOCOM 1's DWARF, 0x68) */
class Props {
public:
    float GetOpacity(float dist) const;
    void GetScale(float age, float &scale) const;
    void GetColor(float age, CPnt4D &color) const;
    void GetTexture(float age, zdb::CTexHandle *&tex) const;
    void DecRefCount(int n);
    Props *Branch();

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
    virtual void Render(const CMatrix &m);

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
    void ScaleSequenceAdd(float fraction, float scale);
    void ScaleSequenceClear();
    void ColorSequenceAdd(float fraction, float r, float g, float b, float a);
    void ColorSequenceClear();
    void TextureSequenceAdd(float fraction, zdb::CTexHandle *tex);
    void TextureSequenceClear();

    unsigned char _0[0xdc];   // 0x0: SOCOM 1's m_active .. m_initialAgeRange, and SOCOM II's word
    std::vector<zdb::CTexHandle *> m_textures;   // 0xdc
    Props *m_props;   // 0xe8
};

}

#endif
