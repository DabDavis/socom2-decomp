/* zparticle_props.cpp -- Particle::Props, the properties particles share (OVL1 0x3286D0-0x3294AF):
 * their lookups by distance and by age, their reference count and copy on write, and the tail their
 * sequences need (CPnt4D's and the sequences' copy constructors, the sequences' constructors, the
 * vectors' init, deleters and clears). The unit name is ours (the world lane round 23): split from
 * zparticle_instance.cpp, whose functions call every one of these out of line in both builds --
 * Instance::Render calls GetOpacity, GetScale, GetColor and GetTexture, Expire and the pool's
 * recycle DecRefCount -- while one unit inlined GetOpacity into Render (an `auto_inline off`
 * stand-in held it out); auto-inlining never crosses units (README "The inlining mode, measured"):
 * tools/unitscan.py's cut at 0x3286D0 explains that pin, separates no inlined pair, and falls right
 * after ~List, the instance unit's registered destructor and last function, as in SOCOM 1's demo
 * (its GetOpacity 0x237930 follows ~List's 0x2377F0, and its Render calls it out of line too). No
 * static initialiser between the instance unit's (0x402C40) and the source unit's: this unit holds
 * no <iostream>. Defined in reverse of their image order (-inline deferred). */

#include "zparticle.h"

/* Sony's libvu0 (MAIN): v0 = v1 * t + v2 * (1 - t) */
extern "C" void sceVu0InterVector(float *v0, float *v1, float *v2, float t);

namespace Particle {

/* SOCOM 1's: a fresh set of properties, with the defaults. The call for m_accelW goes to CPnt4D's
 * empty constructor (`jr ra; move v0,a0`), whose linked copy is another unit's, 0x2110E0
 * (__ct__6CPnt4DFv; CPipe's 0x336230 calls it too) */
Props *Props::Create()
{
    return new Props;
}

/* SOCOM 1's: the properties made this source's own before it changes them -- a copy when others
 * share them, this reference given up */
Props *Props::Branch()
{
    if (m_refCount > 1) {
        DecRefCount(1);
        return new Props(*this);
    }
    return this;
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
 * with no keys white, fading out over the first unit of age. The key is read as m_colorSeq[k] at
 * each use: a reference local for it (`const SequenceEntry<CPnt4D> &a = m_colorSeq[k];`) is a
 * front-end temporary numbered below the data pointer's, which then colours first and takes k's t0
 * (15 differing; tools/igreplay.py --want: the data pointer anywhere below the reference -- the world
 * lane's rounds 14 and 21) */
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
    float t = (age - m_colorSeq[k].m_fraction) / (m_colorSeq[k + 1].m_fraction - m_colorSeq[k].m_fraction);
    CPnt4D c0(m_colorSeq[k].m_data);
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
