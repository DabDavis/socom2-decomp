/* zparticle_instance.cpp -- Particle::Instance, one live particle (OVL1 0x326F50?-0x3286CF): a
 * particle's tick and render, its list, the pools' recycling and purge. The unit name is SOCOM 1's
 * (its __sinit_zparticle_instance.cpp; SOCOM II's 0x402C40). Defined in reverse of their image
 * order (-inline deferred). Written so far: the pools' recycling and purge, Instance::List's
 * walks, Destruction_Instance's destructor and Render. Its range starts at 0x326F50 at the latest
 * (Destruction_Instance::Recycle) and ends with ~List (0x3285F0, the destructor the initialiser
 * registers): the properties the instances share are another unit's, zparticle_props.cpp (the
 * world lane round 23 -- every call into it is out of line in both builds) */

#include <iostream>
#include <stdlib.h>
#include "zparticle.h"

/* Sony's libvu0 (MAIN): v0 = v1 * t + v2 * (1 - t) */
extern "C" void sceVu0InterVector(float *v0, float *v1, float *v2, float t);


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

#if Z_PORT
            /* the velocity drawn toward the wind: w = wind * factor (vmulx.xyz), d = v - w, d *= f, v = w + d,
             * xyz; v.w kept */
            {
                float *v = m_vel;
                const float *wind = &zdb::CWorld::m_world->m_Wind.x;
                float wf = m_props->m_windFactor;

                for (int k = 0; k < 3; k++) {
                    float w = wind[k] * wf;
                    float d = v[k] - w;

                    d = d * f;
                    v[k] = w + d;
                }
            }
#else
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
#endif
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

/* Sony's libvu0 (MAIN) */
extern "C" float sceVu0InnerProduct(float *v0, float *v1);
extern "C" void sceVu0ScaleVector(float *v0, float *v1, float s);

/* a source's view test, as SOCOM 1's member type (ours: Render reads it through this, its result
 * numbered ahead of the caller's code-generation temporaries -- the README's PadButtonState rule) */
static inline zdb::tag_ZVIS_FOV SourceFOV(const Source *s)
{
    return (zdb::tag_ZVIS_FOV)s->m_fovFlag;
}

/* SOCOM 1's (its camera matrix dropped): a live particle buffered for drawing -- its distance from
 * the camera, its age as a fraction of its life, its size by the properties' scale, its source's
 * extents grown, faded out by distance, its box tested against the view unless its source says
 * otherwise, then an element filled: the colour by the opacity, by 128, the texture, the size.
 * The full buffer's flag is SOCOM 1's function-local static OutputOnceOnly (its guard dropped by MW
 * 3.0: 1 in data, the row @LOCAL@...@OutputOnceOnly; round 25 -- before, an extern inside the
 * namespace left its two relocations unresolved).
 * Byte-exact since round 26 (open since round 22 at 7, registers only: the source pointer, its fov
 * byte and the constant 1 of the second test -- the target's a1 / a1 / a0): THE FOV BYTE IS READ
 * THROUGH AN INLINE RETURNING SOCOM 1'S ENUM BY A CAST, into a local of that type (SourceFOV) -- the
 * inline's result is numbered ahead of the caller's code-generation temporaries (the UI's round-29
 * PadButtonState rule), so before `bool test = true`'s constant, the window round 25's `igreplay
 * --movers` found (v62 anywhere from 31.5 to 59.5). Read in place, as an int or unsigned char inline,
 * or through the enum inline twice: 7 each (build/scratch/wld/v_rend_*.py). Its four floats are numbered
 * age, scale, opacity, dist by dead definitions at the top (dist by its own first definition last:
 * f20). GetOpacity, GetScale, GetColor and GetTexture are zparticle_props.cpp's: called out of line,
 * as the target calls them (one unit with them inlined GetOpacity, round 22's pin) */
void Instance::Render()
{
    if (m_source->m_frozen || !(m_age < m_lifespan))
        return;
    float age = 0.0f, scale = 0.0f, opacity = 0.0f;
    CPnt4D d;
    float *cp = zdb::CWorld::m_world->m_camera->m_matrix[3];

    d.x = m_pos[0] - cp[0];
    d.y = m_pos[1] - cp[1];
    d.z = m_pos[2] - cp[2];
    float dist = sceVu0InnerProduct(&d.x, &d.x);
    age = m_age * m_invLifespan;

    if (m_props) {
        float s;

        m_props->GetScale(age, s);
        scale = s * m_44;
    } else
        scale = m_44;
    if (m_source)
        m_source->CheckExtents(*(CPnt4D *)m_pos, scale);
    opacity = m_props->GetOpacity(dist);

    if (opacity <= 0.01f)
        return;
    if (m_source->m_outOfRange)
        return;
    bool test = true;

    if (m_source) {
        zdb::tag_ZVIS_FOV fov = SourceFOV(m_source);

        if (fov == zdb::ZVIS_FOV_ALL_OUT)
            return;
        if (fov == zdb::ZVIS_FOV_ALL_IN)
            test = false;
    }
    if (test) {
        float x0 = m_pos[0] - scale, x1 = m_pos[0] + scale;
        float y0 = m_pos[1] - scale, y1 = m_pos[1] + scale;
        float z0 = m_pos[2] - scale, z1 = m_pos[2] + scale;
        PNT4D c[8] = {
            { x0, y0, z1, 1.0f }, { x0, y0, z0, 1.0f }, { x1, y0, z0, 1.0f }, { x1, y0, z1, 1.0f },
            { x0, y1, z1, 1.0f }, { x0, y1, z0, 1.0f }, { x1, y1, z0, 1.0f }, { x1, y1, z1, 1.0f }
        };

        if (zdb::CWorld::m_world->m_camera->TestFOV(c, 0, true) == 2)
            return;
    }
    static bool OutputOnceOnly = true;

    if (numParticles >= 5000) {
        if (OutputOnceOnly)
            OutputOnceOnly = false;
        return;
    }
    BufferElement *e = &buffer[numParticles++];

    m_props->GetColor(age, e->m_color);
    if (e->m_color.w <= 0.01f) {
        numParticles--;
        return;
    }
    e->m_color.w *= opacity;
    sceVu0ScaleVector(&e->m_color.x, &e->m_color.x, 128.0f);
    zdb::CTexHandle *tex = m_texture;

    if (!tex)
        m_props->GetTexture(age, tex);
    e->m_texture = tex ? tex->m_texture : 0;
    e->m_instance = this;
    e->m_scale = scale;
    e->m_depth = dist;
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

/* the matrix's diagonal set from three floats (ours: by value, MW loads them right to left) */
static inline void SetDiagonal(CMatrix &m, float x, float y, float z)
{
    m.m_matrix[0][0] = x;
    m.m_matrix[1][1] = y;
    m.m_matrix[2][2] = z;
}

/* SOCOM 1's: aged by the step, the node deactivated once dead; else the position and velocity
 * integrated over the tick, the node falling, turned by its spin past the first tick, scaled by
 * the axis and its opacity and matrix set, and the friction drawing the velocity toward the
 * world's wind (the operators zparticle.h's, as zmath.h's: CPnt3D over the C struct for its
 * ld/sd + lwc1/swc1 copy, the diagonal through an inline's by-value floats, loaded z, y, x).
 * The last line stores through CPnt4D's own (implicit) assignment, not PNT4D's (the world lane
 * round 25: open since round 24 at 4 instructions): the assignment is an inline call at the
 * expression's root, and with it the three operators' results are laid out as the target's --
 * operator+'s at sp+80, operator*'s at 64, operator-'s at 48, the outer first as their return
 * copies at 160 / 144 / 128; stored through PNT4D's the inner came first (operator-'s at 80). The
 * operators' heights, their order in the class, by-value operands, helpers and named steps
 * changed nothing or more (round 24's variants; round 25's v_dt*.py) */
void Destruction_Instance::Tick(float dT)
{
    m_age += dT;
    if (!(m_age < m_lifespan)) {
        m_node->SetActive(false);
        return;
    }
    *(CPnt4D *)m_pos += *(CPnt4D *)m_vel * m_dT;
    *(CPnt4D *)m_vel += m_props->m_accelW * m_dT;
    if (m_node) {
        CMatrix m;

        m_vel[1] -= 20.0f * m_dT;
        if (m_age != m_dT) {
            CPnt3D rot;

            m_node->GetRotation(&rot);
            rot += *(CPnt3D *)m_spin * m_dT;
            m_node->SetRotation(&rot);
        }
        SetDiagonal(m, m_axis[0], m_axis[1], m_axis[2]);
        sceVu0MulMatrix(m.m_matrix, *(float (*)[4][4])m_node, m.m_matrix);
        m_node->m_Opacity = m_opacity;
        m_node->SetMatrix(&m);
    }
    if (m_props->m_friction) {
        CPnt4D wind;

        wind.x = m_props->m_windFactor * zdb::CWorld::m_world->m_Wind.x;
        wind.y = m_props->m_windFactor * zdb::CWorld::m_world->m_Wind.y;
        wind.z = m_props->m_windFactor * zdb::CWorld::m_world->m_Wind.z;
        wind.w = 0.0f;
        float f = FastExp(-m_props->m_friction * m_dT);

        *(CPnt4D *)m_vel = wind + (*(CPnt4D *)m_vel - wind) * f;
    }
}

}
