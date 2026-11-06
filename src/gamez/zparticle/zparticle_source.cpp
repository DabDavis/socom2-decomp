/* zparticle_source.cpp -- Particle::Source, an emitter: its sequences of scale, colour and
 * texture, its texture list, its tick (OVL1 0x3294B0-0x32C03F). The unit name is SOCOM 1's (its
 * __sinit_zparticle_source.cpp; SOCOM II's 0x402CE0). Defined in reverse of their image order
 * (-inline deferred). Written so far: the sequence setters, which branch the shared properties
 * before changing them, as SOCOM 1's do; RenderBBox, CheckExtents, the destructor and
 * Source::List::Uninit. */

#include <iostream>
#include "zparticle.h"

/* SOCOM II's pointer-list begin() builds its first node from a reference to the sentinel's link,
 * so `&end_.next_` is a temporary computed once and kept for the clear after a walk: znode.h's
 * stand-ins, the same shapes (include/list's begin() takes the link by value; the names ours) */
typedef std::__list_deleter<const void *, std::allocator<const void *> > PtrListBase;
struct PtrListFirst : public PtrListBase::iterator {
    PtrListFirst(PtrListBase::node_base *const &link) : PtrListBase::iterator(link) {}
};
inline PtrListBase::iterator ListBegin(PtrListBase &l) { return PtrListFirst(l.end_node().next_); }
inline PtrListBase::iterator ListEnd(PtrListBase &l) { return PtrListBase::iterator(&l.end_node()); }
inline void ListClear(PtrListBase &l) { l.erase(ListBegin(l), ListEnd(l)); }

namespace Particle {

/* the live sources (SOCOM 1's Source::list: the unit's initialiser 0x402CE0 builds it after
 * <iostream>'s pair and registers ~List 0x32BFD0) */
Source::List Source::list;

/* SOCOM 1's: switched on or off when it changes -- its collision node too, when it has one and the
 * flag at 0x120 says so (SOCOM II tests the node) -- then, switched on, the shut-off flag cleared,
 * else the good position. The flag narrowed into a byte local and compared member first (SOCOM 1's
 * compares the parameter: the narrowed copy in v1, the raw one stored); the node's SetActive takes
 * the byte (zparticle.h's marked device) */
void Source::SetActive(bool on)
{
    unsigned char b = on;

    if (m_active != b) {
        m_active = b;
        if (m_hasbbox && m_diNode)
            m_diNode->SetActive(b);
        if (on)
            m_shuttingoff = 0;
        else
            m_goodPos = 0;
    }
}

/* SOCOM 1's: the box grown to hold a particle, the radius's square kept */
void Source::CheckExtents(CPnt4D p, float r)
{
    if (m_hasbbox) {
        m_bboxMin.x = m_bboxMin.x < p.x ? m_bboxMin.x : p.x;
        m_bboxMin.y = m_bboxMin.y < p.y ? m_bboxMin.y : p.y;
        m_bboxMin.z = m_bboxMin.z < p.z ? m_bboxMin.z : p.z;
        m_bboxMax.x = m_bboxMax.x > p.x ? m_bboxMax.x : p.x;
        m_bboxMax.y = m_bboxMax.y > p.y ? m_bboxMax.y : p.y;
        m_bboxMax.z = m_bboxMax.z > p.z ? m_bboxMax.z : p.z;
        m_filled_volume = r * r;
    }
}

/* SOCOM 1's: every source's box worked out (the drawing compiled out) */
void Source::RenderBBox()
{
    CBBox box;
    List::iterator e = list.end();

    for (List::iterator i = list.begin(); i != e; ++i)
        (*i)->GetBBox(&box);
}

/* the source's collision box made once (new in SOCOM II as a function of its own: SOCOM 1's
 * constructor built it) -- six four-point polygons of material 1 and camera type 2, the first and
 * last of type 3, the others 2; a node holding them, named, in the world, its extension pointing
 * back at the source, its DI flag and back-face search set (0x32AD20, not named: the box's switch
 * 0x329A40 calls it when the source has no node yet) */
extern "C" void func_0032AD20(Source *s)
{
    if (!s->m_diPolies[0]) {
        s->m_diPolies[0] = new zdb::CDIPoly;
        s->m_diPolies[1] = new zdb::CDIPoly;
        s->m_diPolies[2] = new zdb::CDIPoly;
        s->m_diPolies[3] = new zdb::CDIPoly;
        s->m_diPolies[4] = new zdb::CDIPoly;
        s->m_diPolies[5] = new zdb::CDIPoly;
        s->m_diPolies[0]->Allocate(4);
        s->m_diPolies[1]->Allocate(4);
        s->m_diPolies[2]->Allocate(4);
        s->m_diPolies[3]->Allocate(4);
        s->m_diPolies[4]->Allocate(4);
        s->m_diPolies[5]->Allocate(4);
        s->m_diPolies[0]->SetMaterial(1);
        s->m_diPolies[1]->SetMaterial(1);
        s->m_diPolies[2]->SetMaterial(1);
        s->m_diPolies[3]->SetMaterial(1);
        s->m_diPolies[4]->SetMaterial(1);
        s->m_diPolies[5]->SetMaterial(1);
        s->m_diPolies[0]->SetCameraType(2);
        s->m_diPolies[1]->SetCameraType(2);
        s->m_diPolies[2]->SetCameraType(2);
        s->m_diPolies[3]->SetCameraType(2);
        s->m_diPolies[4]->SetCameraType(2);
        s->m_diPolies[5]->SetCameraType(2);
        s->m_diPolies[0]->SetDiType(3);
        s->m_diPolies[1]->SetDiType(2);
        s->m_diPolies[2]->SetDiType(2);
        s->m_diPolies[3]->SetDiType(2);
        s->m_diPolies[4]->SetDiType(2);
        s->m_diPolies[5]->SetDiType(3);
        s->m_diNode = new zdb::CNode;
        s->m_diNode->AddDI(s->m_diPolies[0]);
        s->m_diNode->AddDI(s->m_diPolies[1]);
        s->m_diNode->AddDI(s->m_diPolies[2]);
        s->m_diNode->AddDI(s->m_diPolies[3]);
        s->m_diNode->AddDI(s->m_diPolies[4]);
        s->m_diNode->AddDI(s->m_diPolies[5]);
        s->m_diNode->SetName("PARTICLE DI NODE");
        zdb::CWorld::m_world->AddChild(s->m_diNode);
        s->m_particleEx = new CParticleEx(s);
        s->m_diNode->SetNodeEx(s->m_particleEx, false);
        s->m_diNode->m_hasDI = 1;
        s->m_diNode->m_di.m_bReturnBackFaceIntersections = 1;
    }
}

/* SOCOM 1's: every source deleted, the list emptied. The end declared first: byte-exact under
 * include/list's iterator as it stands and under SOCOM 1's (node *node_), where `it, e` swaps the
 * two saved registers (the world lane r6) */
void Source::List::Uninit()
{
    PtrListBase::iterator e = ListEnd(*this), it = ListBegin(*this);

    for (; it != e; ++it)
        delete (Source *)*it;
    ListClear(*this);
}

/* SOCOM 1's: the source off the list, its instances let go of, its properties and collision
 * node given up */
Source::~Source()
{
    List::iterator e = list.end();

    for (List::iterator i = list.begin(); i != e; ++i) {
        if (*i == this) {
            list.erase(i);
            break;
        }
    }
    Instance::list.RemoveSource(this);
    if (m_props)
        m_props->DecRefCount(1);
    if (m_diNode) {
        zdb::CWorld::m_world->DeleteChild(m_diNode);
        delete m_diNode;
    }
    if (m_particleEx)
        delete m_particleEx;
}

/* SOCOM 1's: the texture sequence emptied */
void Source::TextureSequenceClear()
{
    m_props = m_props->Branch();
    m_props->m_textureSeq.clear();
}

/* SOCOM 1's: a texture keyed at a fraction of the life, registered with the draw buffer */
void Source::TextureSequenceAdd(float fraction, zdb::CTexHandle *tex)
{
    m_props = m_props->Branch();
    m_props->m_textureSeq.push_back(SequenceEntry<zdb::CTexHandle *>(fraction, tex));
    AddDynTexture(tex);
}

/* SOCOM 1's: the colour sequence emptied */
void Source::ColorSequenceClear()
{
    m_props = m_props->Branch();
    m_props->m_colorSeq.clear();
}

/* SOCOM 1's: a colour keyed at a fraction of the life. The colour a named local, after the branch:
 * the target's sits above the entry's temporary (SOCOM 1's below it, the colour built in the
 * argument list) */
void Source::ColorSequenceAdd(float fraction, float r, float g, float b, float a)
{
    m_props = m_props->Branch();
    CPnt4D color(r, g, b, a);
    m_props->m_colorSeq.push_back(SequenceEntry<CPnt4D>(fraction, color));
}

/* SOCOM 1's: the scale sequence emptied */
void Source::ScaleSequenceClear()
{
    m_props = m_props->Branch();
    m_props->m_scaleSeq.clear();
}

/* SOCOM 1's: a scale keyed at a fraction of the life */
void Source::ScaleSequenceAdd(float fraction, float scale)
{
    m_props = m_props->Branch();
    m_props->m_scaleSeq.push_back(SequenceEntry<float>(fraction, scale));
}

}
