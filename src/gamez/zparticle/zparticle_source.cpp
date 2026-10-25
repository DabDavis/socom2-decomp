/* zparticle_source.cpp -- Particle::Source, an emitter: its sequences of scale, colour and
 * texture, its texture list, its tick (OVL1 0x3294B0-0x32C03F). The unit name is SOCOM 1's (its
 * __sinit_zparticle_source.cpp; SOCOM II's 0x402CE0). Defined in reverse of their image order
 * (-inline deferred). Written so far: the sequence setters, which branch the shared properties
 * before changing them, as SOCOM 1's do; RenderBBox, CheckExtents, the destructor and
 * Source::List::Uninit. */

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

/* SOCOM 1's: the box grown to hold a particle, the radius's square kept */
void Source::CheckExtents(CPnt4D p, float r)
{
    CPnt4D q(p);

    if (m_hasbbox) {
        m_bboxMin.x = m_bboxMin.x < q.x ? m_bboxMin.x : q.x;
        m_bboxMin.y = m_bboxMin.y < q.y ? m_bboxMin.y : q.y;
        m_bboxMin.z = m_bboxMin.z < q.z ? m_bboxMin.z : q.z;
        m_bboxMax.x = m_bboxMax.x > q.x ? m_bboxMax.x : q.x;
        m_bboxMax.y = m_bboxMax.y > q.y ? m_bboxMax.y : q.y;
        m_bboxMax.z = m_bboxMax.z > q.z ? m_bboxMax.z : q.z;
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

/* SOCOM 1's: every source deleted, the list emptied */
void Source::List::Uninit()
{
    PtrListBase::iterator it = ListBegin(*this), e = ListEnd(*this);

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
