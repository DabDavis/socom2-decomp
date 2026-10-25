/* zparticle_source.cpp -- Particle::Source, an emitter: its sequences of scale, colour and
 * texture, its texture list, its tick (OVL1 0x3294B0-0x32C03F). The unit name is SOCOM 1's (its
 * __sinit_zparticle_source.cpp; SOCOM II's 0x402CE0). Defined in reverse of their image order
 * (-inline deferred). Written so far: the sequence setters, which branch the shared properties
 * before changing them, as SOCOM 1's do. */

#include "zparticle.h"

namespace Particle {

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
