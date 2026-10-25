/* zparticle_instance.cpp -- Particle::Instance, one live particle, and the properties instances
 * share (OVL1 0x3271D0?-0x3294AF): a particle's tick and render, its list, the properties'
 * lookups by distance and by age, their reference count and copy on write. The unit name is
 * SOCOM 1's (its __sinit_zparticle_instance.cpp; SOCOM II's 0x402C40). Defined in reverse of
 * their image order (-inline deferred). Written so far: some of the properties' members. */

#include "zparticle.h"

/* Sony's libvu0 (MAIN): v0 = v1 * t + v2 * (1 - t) */
extern "C" void sceVu0InterVector(float *v0, float *v1, float *v2, float t);

namespace Particle {

/* the piece's node moved to it while it lives */
void Destruction_Instance::Render(const CMatrix &m)
{
    if (m_age < m_lifespan && m_node) {
        m_node->SetPosition(m_pos[0], m_pos[1], m_pos[2]);
        return;
    }
}

/* SOCOM 1's: one reference fewer; the last deletes the properties, their sequences with them */
void Props::DecRefCount(int n)
{
    m_refCount -= n;
    if (m_refCount <= 0)
        delete this;
}

/* SOCOM 1's: the texture of the key in force at `age` (the last key before the first one after
 * it), none when the sequence is empty */
void Props::GetTexture(float age, zdb::CTexHandle *&tex) const
{
    int n = m_textureSeq.size();

    if (n == 0) {
        tex = 0;
        return;
    }
    int k;
    int i;
    for (i = 0; i < (int)m_textureSeq.size(); i++) {
        if (age < m_textureSeq[i].m_fraction) {
            k = i - 1;
            goto found;
        }
    }
    k = n - 1;
found:
    if (k < 0)
        k = 0;
    else if (k >= n)
        k = n - 1;
    tex = m_textureSeq[k].m_data;
}

/* SOCOM 1's: the colour at `age`, interpolated between the keys around it (held at the ends);
 * with no keys white, fading out over the first unit of age.
 * OPEN, 15 instructions: registers only -- the target keeps the found index in t0 apart from the
 * loop counter (t1) and the data pointer in t1, where here the index shares t1 with the counter
 * and the data takes t0 (GetScale's identical search allocates as the target; tried: the two
 * declaration orders, `int k, i`, a copy of the index, break/for-scoped forms, a found test after
 * the loop) */
void Props::GetColor(float age, CPnt4D &color) const
{
    int n = m_colorSeq.size();

    if (n == 0) {
        color.x = 1.0f;
        color.y = 1.0f;
        color.z = 1.0f;
        if (age <= 0.0f) {
            color.w = 1.0f;
            return;
        }
        if (age >= 1.0f) {
            color.w = 0.0f;
            return;
        }
        color.w = 1.0f - age;
        return;
    }
    int k;
    int i;
    for (i = 0; i < (int)m_colorSeq.size(); i++) {
        if (age < m_colorSeq[i].m_fraction) {
            k = i - 1;
            goto found;
        }
    }
    k = n - 1;
found:
    if (k < 0) {
        color = m_colorSeq[0].m_data;
        return;
    }
    if (k >= n - 1) {
        color = m_colorSeq[n - 1].m_data;
        return;
    }
    const SequenceEntry<CPnt4D> &a = m_colorSeq[k];
    float t = (age - a.m_fraction) / (m_colorSeq[k + 1].m_fraction - a.m_fraction);
    CPnt4D c0(a.m_data);
    CPnt4D c1(m_colorSeq[k + 1].m_data);

    sceVu0InterVector((float *)&color, (float *)&c1, (float *)&c0, t);
}

/* SOCOM 1's: the scale at `age`, interpolated between the keys around it (held at the ends), 1
 * when the sequence is empty */
void Props::GetScale(float age, float &scale) const
{
    int n = m_scaleSeq.size();

    if (n == 0) {
        scale = 1.0f;
        return;
    }
    int k;
    int i;
    for (i = 0; i < (int)m_scaleSeq.size(); i++) {
        if (age < m_scaleSeq[i].m_fraction) {
            k = i - 1;
            goto found;
        }
    }
    k = n - 1;
found:
    if (k < 0) {
        scale = m_scaleSeq[0].m_data;
        return;
    }
    if (k >= n - 1) {
        scale = m_scaleSeq[n - 1].m_data;
        return;
    }
    const SequenceEntry<float> &a = m_scaleSeq[k];
    const SequenceEntry<float> &b = m_scaleSeq[k + 1];
    float t = (age - a.m_fraction) / (b.m_fraction - a.m_fraction);

    scale = a.m_data + t * (b.m_data - a.m_data);
}

/* SOCOM 1's: the opacity at a distance -- none outside the fade range, ramping in over the near
 * fade and out over the far one. One result for every arm: early returns hoist the 1.0f into
 * the last test's delay slot, as SOCOM 1's build did */
float Props::GetOpacity(float dist) const
{
    float o;

    if (dist >= m_farFade2 || dist <= m_nearFade1)
        o = 0.0f;
    else if (dist > m_farFade1)
        o = m_invFarFade * (m_farFade2 - dist);
    else if (dist < m_nearFade2)
        o = m_invNearFade * (dist - m_nearFade1);
    else
        o = 1.0f;
    return o;
}

}
