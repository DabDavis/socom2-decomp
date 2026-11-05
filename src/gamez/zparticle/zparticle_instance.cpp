/* zparticle_instance.cpp -- Particle::Instance, one live particle, and the properties instances
 * share (OVL1 0x3271D0?-0x3294AF): a particle's tick and render, its list, the properties'
 * lookups by distance and by age, their reference count and copy on write. The unit name is
 * SOCOM 1's (its __sinit_zparticle_instance.cpp; SOCOM II's 0x402C40). Defined in reverse of
 * their image order (-inline deferred). Written so far: the properties' members, the pools'
 * recycling and purge, Instance::List's walks, Destruction_Instance's destructor and Render.
 * Its range starts at 0x326F50 at the latest (Destruction_Instance::Recycle). */

#include <iostream>
#include <stdlib.h>
#include "zparticle.h"

/* Sony's libvu0 (MAIN): v0 = v1 * t + v2 * (1 - t) */
extern "C" void sceVu0InterVector(float *v0, float *v1, float *v2, float t);

/* the draw buffer's count (zparticle_buf.cpp's unit; unnamed) */
namespace Particle {
/* the elements buffered this frame (SOCOM 1's numParticles) */
extern int numParticles;
}

namespace Particle {

/* the live instances (SOCOM 1's Instance::list: the unit's initialiser 0x402C40 builds it after
 * <iostream>'s pair and registers ~List 0x3285F0) */
Instance::List Instance::list;

/* SOCOM II's: a dying source's instances unlinked and recycled, the head's run first */
void Instance::List::RemoveSource(Source *source)
{
    while (!Empty() && m_head->m_source == source) {
        Instance *p = m_head;

        m_head = p->m_listNext;
        p->Recycle();
    }
    for (Instance *prev = m_head; prev; ) {
        Instance *p = prev->m_listNext;

        while (!End(p) && p->m_source == source) {
            Instance *d = p;

            p = p->m_listNext;
            prev->m_listNext = p;
            d->Recycle();
        }
        prev = p;
    }
}

/* SOCOM 1's (its node's spin dropped): aged by the step; while alive a rotated one turns and
 * stops turning when its spin changes sign, a streak's start follows the position, the position
 * and velocity are integrated over the tick on VU0, and the friction draws the velocity toward
 * the world's wind. Byte-exact (the world lane round 22): the draw type through the getter in each
 * test (one `lb + andi 0xff` -- read into a local, or switched on, it folds into lbu), the step in a
 * local for the spin (one load), `if (friction)` (its `!= 0.0f` puts the zero first in c.eq.s), the
 * pull toward the wind written in place (its operands evaluated in their order: through an inline's
 * parameters MW loads the world before the velocity's address) */
void Instance::Tick(float dT)
{
    if (m_source->m_frozen)
        return;
    m_age += dT;
    if (m_age < m_lifespan) {
        if (m_source->GetDrawType() == PT_ROTATED) {
            float dt = m_dT;

            m_10[0] += dt * m_10[1];
            float old = m_10[1];

            m_10[1] = old + dt * m_10[2];
            if ((old > 0.0f || m_10[1] > 0.0f) && (old < 0.0f || m_10[1] < 0.0f)) {
                m_10[1] = 0.0f;
                m_10[2] = 0.0f;
            }
        } else if (m_source->GetDrawType() == PT_STREAKED) {
            m_10[0] = m_pos[0];
            m_10[1] = m_pos[1];
            m_10[2] = m_pos[2];
            m_10[3] = m_pos[3];
        }
        AddScaled(m_pos, m_vel, m_dT);
        AddScaled(m_vel, &m_props->m_accelW.x, m_dT);
        if (m_props->m_friction) {
            float f = FastExp(-m_props->m_friction * m_dT);

            asm __volatile__ (
                "lqc2 $vf7, 0(%0)\n"
                "lqc2 $vf4, 0(%1)\n"
                "mfc1 $8, %2\n"
                "nop\n"
                "qmtc2 $8, $vf5\n"
                "vmulx.xyz $vf4, $vf4, $vf5\n"
                "vsub.xyz $vf6, $vf7, $vf4\n"
                "mfc1 $8, %3\n"
                "nop\n"
                "qmtc2 $8, $vf5\n"
                "vmulx.xyz $vf6, $vf6, $vf5\n"
                "vadd.xyz $vf7, $vf4, $vf6\n"
                "sqc2 $vf7, 0(%0)\n"
                : : "r"(m_vel), "r"(&zdb::CWorld::m_world->m_Wind.x), "f"(m_props->m_windFactor), "f"(f) : "$8", "memory");
        }
    }
}

/* SOCOM 1's: every live instance ticked -- at the LOD's age rate when its source asks -- and the
 * dead ones recycled (SOCOM II's list: linked through the instances, not a std::list) */
void Instance::List::Tick(float dT)
{
    m_dT = dT;
    while (!Empty() && !m_head->Alive()) {
        Instance *p = m_head;

        m_head = p->m_listNext;
        p->Recycle();
    }
    for (Instance *prev = m_head; prev; ) {
        if (prev->AgeLOD())
            prev->Tick(dT * ageFactor);
        else
            prev->Tick(dT);
        Instance *p = prev->m_listNext;

        while (!End(p) && !p->Alive()) {
            Instance *d = p;

            p = p->m_listNext;
            prev->m_listNext = p;
            d->Recycle();
        }
        prev = p;
    }
}

/* SOCOM II's: the instances whose age has reached their life unlinked and recycled, the head's
 * run first */
void Instance::List::RemoveDead()
{
    while (!Empty() && !m_head->Alive()) {
        Instance *p = m_head;

        m_head = p->m_listNext;
        p->Recycle();
    }
    for (Instance *prev = m_head; prev; ) {
        Instance *p = prev->m_listNext;

        while (!End(p) && !p->Alive()) {
            Instance *d = p;

            p = p->m_listNext;
            prev->m_listNext = p;
            d->Recycle();
        }
        prev = p;
    }
}

/* SOCOM 1's: every live instance deleted, then the list's own nodes (none) */
Instance::List::~List()
{
    Uninit();
}

/* SOCOM 1's: every live instance deleted */
void Instance::List::Uninit()
{
    Instance *p;

    while ((p = m_head) != 0) {
        m_head = p->m_listNext;
        delete p;
    }
}

/* SOCOM II's: the piece given its visual (DestructionManager calls it after Create) -- added to the
 * node, the node transparent when the visual's first flag is set and its ninth is not, the piece's
 * opacity copied, the node activated */
void Destruction_Instance::SetVisual(zdb::CVisual *vis)
{
    if (m_node && vis) {
        m_node->AddVisual(vis);
        m_node->m_transparent = ((vis->m_flags & 1) & ((vis->m_flags & 0x100) == 0)) != 0;
        m_node->m_Opacity = m_opacity;
        m_node->SetActive(true);
    }
}

/* SOCOM II's: the dead instances recycled, then every pooled instance and piece deleted */
/* SOCOM 1's Render(): the draw buffer emptied (its count, 0x45C4C0), then every live instance
 * rendered -- SOCOM 1 handed each the camera's matrix, SOCOM II's Render takes none (RenderWorld
 * calls this after its pass) */
void Render()
{
    numParticles = 0;
    for (Instance *p = Instance::list.m_head; p; p = p->m_listNext)
        p->Render();
}

void FreeInstances()
{
    Instance *p;
    Destruction_Instance *d;

    Instance::list.RemoveDead();
    while ((p = Instance::m_instance_pool.Pop()) != 0)
        delete p;
    while ((d = Destruction_Instance::m_instance_pool.Pop()) != 0)
        delete d;
}

/* SOCOM II's: the piece expired and given back to its pool */
void Destruction_Instance::Recycle()
{
    Expire();
    m_listNext = 0;
    m_instance_pool.Push(this);
}

/* SOCOM II's: the particle expired and given back to its pool */
void Instance::Recycle()
{
    Expire();
    m_listNext = 0;
    m_instance_pool.Push(this);
}

/* SOCOM 1's: the instance's links, texture, age, properties and life set, its position and
 * velocity copied; a rotated one given a random angle and spin from its source, a streak its start;
 * the properties counted */
Instance::Instance(const CPnt4D &pos, const CPnt4D &vel, const CPnt3D &unused, float f, float life, float age,
                   zdb::CTexHandle *tex, Props *props, Source *source)
{
    m_source = source;
    m_texture = tex;
    m_age = age;
    m_props = props;
    m_44 = f;
    m_lifespan = life;
    m_invLifespan = 1.0f / life;
    m_listNext = 0;
    m_pos[0] = pos.x;
    m_pos[1] = pos.y;
    m_pos[2] = pos.z;
    m_pos[3] = pos.w;
    m_vel[0] = vel.x;
    m_vel[1] = vel.y;
    m_vel[2] = vel.z;
    m_vel[3] = vel.w;
    if (m_source->m_drawType == PT_ROTATED) {
        m_10[1] = m_source->m_minRot + (m_source->m_maxRot - m_source->m_minRot) * (rand() * 4.656613e-10f);
        m_10[2] = m_source->m_minRotAcc + (m_source->m_maxRotAcc - m_source->m_minRotAcc) * (rand() * 4.656613e-10f);
    } else if (m_source->m_drawType == PT_STREAKED) {
        m_10[0] = pos.x;
        m_10[1] = pos.y;
        m_10[2] = pos.z;
        m_10[3] = pos.w;
    }
    m_props->m_refCount++;
}

/* SOCOM 1's, the visual left to SetVisual: the base's three links cleared, the properties
 * counted, the piece's motion copied in field by field, its node taken off the pool */
Destruction_Instance::Destruction_Instance(const CPnt4D &pos, const CPnt4D &vel, const CPnt3D &spin, float f,
                                           float life, float opacity, CPnt3D axis, Props *props)
{
    props->m_refCount++;
    m_pos[0] = pos.x;
    m_pos[1] = pos.y;
    m_pos[2] = pos.z;
    m_pos[3] = pos.w;
    m_vel[0] = vel.x;
    m_vel[1] = vel.y;
    m_vel[2] = vel.z;
    m_vel[3] = vel.w;
    m_age = 0.0f;
    m_spin[0] = spin.x;
    m_spin[1] = spin.y;
    m_spin[2] = spin.z;
    m_props = props;
    m_opacity = opacity;
    m_44 = f;
    m_lifespan = life;
    m_invLifespan = 1.0f / life;
    m_node = GetFreeInstance();
    m_axis[0] = axis.x;
    m_axis[1] = axis.y;
    m_axis[2] = axis.z;
}

/* the piece's node taken out of the world (SOCOM II's) */
Destruction_Instance::~Destruction_Instance()
{
    zdb::CWorld::m_world->DeleteChild(m_node);
}

/* SOCOM 1's: a piece off the pool, built in place, else a new one; linked in at the live list's
 * head */
Destruction_Instance *Destruction_Instance::Create(const CPnt4D &pos, const CPnt4D &vel, const CPnt3D &spin,
                                                   float f, float life, float opacity, CPnt3D axis,
                                                   Props *props)
{
    Destruction_Instance *p = m_instance_pool.m_head;

    if (p) {
        m_instance_pool.m_head = (Destruction_Instance *)p->m_next;
        p->m_next = 0;
    } else
        p = 0;
    if (p)
        new (p) Destruction_Instance(pos, vel, spin, f, life, opacity, axis, props);
    else
        p = new Destruction_Instance(pos, vel, spin, f, life, opacity, axis, props);
    p->m_listNext = Instance::list.m_head;
    Instance::list.m_head = p;
    return p;
}

/* the piece's node moved to it while it lives */
void Destruction_Instance::Render()
{
    if (m_age < m_lifespan && m_node) {
        m_node->SetPosition(m_pos[0], m_pos[1], m_pos[2]);
        return;
    }
}

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
