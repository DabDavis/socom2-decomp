/* vis_ripple.cpp -- SOCOM II's water ripples: the waves the ZAnim RIPPLE command starts on a map's
 * water surfaces, moving their vertices (OVL1 0x3C0CF0-0x3C356F, after vis_texfx.cpp; no SOCOM 1
 * counterpart -- SOCOM 1's zdb::CRipple, still in SOCOM II at 0x321F50, is a decal ripple). A ripple
 * runs over one node, or over a named group of nodes that RIPPLE_NODE_GROUP gathered (MZANIM's
 * define_ripple_node_groups; docs/zanim.md decodes both records), from a point: a cosine of the
 * distance carried outward at a speed, its amplitude swelling in a quarter of a second and dying
 * over the ripple's time, out to a radius that can grow, optionally lifting the surface by a
 * height. Each frame the manager (0x4B5170: its initialiser 0x404B30 builds it and registers its
 * destructor; CCoreState::Tick ticks it, the mission's unload clears it) collects the surfaces
 * drawn in the last two frames and moves each one's strip vertices -- the sum of the ripples that
 * reach it, in its own frame -- and its normals. Every name here is ours. Defined in reverse of
 * their image order (-inline deferred). */

#include "vis_ripple.h"

namespace zdb {

/* the dot product in XZ */
inline float DotXZ(const CPnt3D *a, const CPnt3D *b) { return a->x * b->x + a->z * b->z; }

/* a strip vertex's position: the strip's origin (floats) plus the vertex's offset from it, shorts
 * in sixteenths */
inline void GetVertexPos(CVisData *v, CPnt3D *p)
{
    p->x = ((float *)v->m_14)[0] + ((short *)v->m_vertex)[0] / 16.0f;
    p->y = ((float *)v->m_14)[1] + ((short *)v->m_vertex)[1] / 16.0f;
    p->z = ((float *)v->m_14)[2] + ((short *)v->m_vertex)[2] / 16.0f;
}

/* the same through a float pointer: the point's components written and read through it (SetLevel,
 * Flatten and Ripple keep the pointer in a register, computed before the strip is selected) */
inline void GetVertexPos(CVisData *v, float *p)
{
    p[0] = ((float *)v->m_14)[0] + ((short *)v->m_vertex)[0] / 16.0f;
    p[1] = ((float *)v->m_14)[1] + ((short *)v->m_vertex)[1] / 16.0f;
    p[2] = ((float *)v->m_14)[2] + ((short *)v->m_vertex)[2] / 16.0f;
}

inline void SetVertexPos(CVisData *v, const float *p)
{
    ((short *)v->m_vertex)[0] = (short)(16.0f * (p[0] - ((float *)v->m_14)[0]));
    ((short *)v->m_vertex)[1] = (short)(16.0f * (p[1] - ((float *)v->m_14)[1]));
    ((short *)v->m_vertex)[2] = (short)(16.0f * (p[2] - ((float *)v->m_14)[2]));
}

inline void SetVertexPos(CVisData *v, const CPnt3D *p)
{
    ((short *)v->m_vertex)[0] = (short)(16.0f * (p->x - ((float *)v->m_14)[0]));
    ((short *)v->m_vertex)[1] = (short)(16.0f * (p->y - ((float *)v->m_14)[1]));
    ((short *)v->m_vertex)[2] = (short)(16.0f * (p->z - ((float *)v->m_14)[2]));
}

/* its normal: shorts at 3, 6 and 7, a unit 32768 */
inline void GetVertexNormal(CVisData *v, CPnt3D *n)
{
    n->x = ((short *)v->m_vertex)[3] / 32768.0f;
    n->y = ((short *)v->m_vertex)[6] / 32768.0f;
    n->z = ((short *)v->m_vertex)[7] / 32768.0f;
}

inline void SetVertexNormal(CVisData *v, const CPnt3D *n)
{
    ((short *)v->m_vertex)[3] = (short)(32767.5f * n->x);
    ((short *)v->m_vertex)[6] = (short)(32767.5f * n->y);
    ((short *)v->m_vertex)[7] = (short)(32767.5f * n->z);
}

/* the initialiser (0x404B30) builds it and registers its destructor */
CWaterRippleManager theWaterRippleManager;

CWaterRippleManager::CWaterRippleManager()
{
    m_ripples.clear();
    m_groups.clear();
    m_nodes.clear();
}

/* the ripples ticked, and with any left every surface they reach moved; whether none is left */
bool CWaterRippleManager::Tick(float dt)
{
    if (Update(dt) == false) {
        int i = 0;
        CNode *n;

        for (n = m_nodes.GetNode(0); n; n = m_nodes.GetNode(i)) {
            Ripple(n);
            i++;
        }
    }
    m_nodes.clear();
    return m_ripples.size() == 0;
}

/* each ripple's surfaces drawn lately listed, the ripple ticked and the dead deleted */
bool CWaterRippleManager::Update(float dt)
{
    m_nodes.clear();

    iterator i;
    iterator e = m_ripples.end();

    i = m_ripples.begin();
    while (i != e) {
        CWaterRipple *r = *i;

        r->MarkNodes();
        if (r->Tick(dt) != false) {
            i = Remove(i);
            delete r;
        } else
            ++i;
    }
    m_active.clear();
    return m_ripples.size() == 0;
}

/* the live ripples that reach a surface listed, each placed in its frame. The live test is the ||
 * in jumping form with each bit still made a bool (its sltu): each bit through `x ? true : false`
 * in the caller's condition (the kit's round 55: a 1-bit field through the ternary keeps its sltu,
 * a cast, `!= 0`, a bool local or a bool inline drop it) -- through IsActive() the || is
 * materialised and tested once more at the join (6 off since round 12; the ternary on one bit
 * only, 2 or 6 off; on the whole ||, 0x12C) */
void CWaterRippleManager::Gather(CNode *node)
{
    m_active.clear();

    CMatrix m;

    node->BuildMTW(m);
    Vu0MatrixInverseWithUniformScale(&m, &m);

    iterator i;
    iterator e = m_ripples.end();

    for (i = m_ripples.begin(); i != e; ++i) {
        CWaterRipple *r = *i;

        if (((r->m_timed ? true : false) || (r->m_visible ? true : false)) && r->Affects(node, &m))
            m_active.push_back(r);
    }
}

/* the listed ripples' offsets at a point summed, the last lifting ripple's rest height added */
bool CWaterRippleManager::GetOffset(const CPnt3D *p, CPnt3D *out, bool *level)
{
    float y = 0.0f;

    *out = CPnt3D::zero;
    *level = false;

    bool any = false;
    iterator i;

    for (i = m_active.begin(); i != m_active.end(); ++i) {
        CWaterRipple *r = *i;
        CPnt3D d;

        if (r->GetOffset(p, &d)) {
            any = true;
            out->Add(&d, out);
            if (r->Lifts() != false) {
                *level = true;
                y = r->m_restY;
            }
        }
    }
    if (*level)
        out->y += y;
    return any;
}

/* OPEN, 0x330 bytes (57 instructions off as written, the right size): the target addresses the
 * vertex point through one pointer register (s3 = &p, set in SelectVertexStrips' delay slot each
 * strip, so hoisted out of the vertex loop only) and keeps this in s8; here the point's y and z
 * addresses are hoisted as well and this spills. SetLevel's shape (a float pointer to the point,
 * set before the select) matches there; here it hoists the pointer out of both loops and spills
 * this and node (0x360), declared in the vertex loop it is propagated (0x340). The locals are
 * declared in the target's slot order (n 288, p 272, off 256, the bool at 303). Round 35: p and a
 * float or CPnt3D pointer to it at the strip loop's top (the pointer then used for the reads, the
 * offset and the store) are 0x358 (build/scratch/wep/v_rip.py).
 * a surface's vertices moved: each one's normal tilted by the offset, its height set where a
 * ripple lifts it */
void CWaterRippleManager::Ripple(CNode *node)
{
    Gather(node);
    if (m_active.size()) {
        for (CVisualVector::iterator it = node->m_visual.begin(); it != node->m_visual.end(); ++it) {
            CVisData d;

            for (CVisData *v = d.SelectVertexStrips(*it, 0, 0); v; v = v->NextVertex()) {
                CPnt3D n;
                CPnt3D p;
                CPnt3D off;
                bool level;

                GetVertexPos(v, &p);
                if (GetOffset(&p, &off, &level) != false) {
                    GetVertexNormal(v, &n);
                    n.x = off.x;
                    n.z = off.z;
                    n.y = 1.0f;
                    n.Normalize();
                    SetVertexNormal(v, &n);
                    if (level) {
                        p.y = off.y;
                        SetVertexPos(v, &p);
                    }
                }
            }
        }
    }
}

/* everything deleted: the mission's unload */
void CWaterRippleManager::Clear()
{
    ClearRipples();
    ClearGroups();
}

/* every ripple's surfaces laid flat, the ripple deleted */
void CWaterRippleManager::ClearRipples()
{
    m_active.clear();
    FreeVector<std::vector<CNode *> >(m_nodes);

    iterator i;
    iterator e = m_ripples.end();

    i = m_ripples.begin();
    while (i != e) {
        CWaterRipple *r = *i;
        int j = 0;
        CNode *n;

        while ((n = r->GetNode(j)) != 0) {
            r->Flatten(n);
            j++;
        }
        i = Remove(i);
        delete r;
    }
}

/* OPEN, 0xD8 against 0xF8: the group's delete -- the target inlines its destructor whole (the
 * name freed, the vector's size zeroed in the first this-test's delay slot, then four this-tests and
 * the data's delete), here the destructor's body is inlined and calls clear() and ~CNodeVector:
 * ~CNodeVector is five high (four this-tested levels and the pod's deallocate, include/vector) and
 * the destructor above it makes six. An implicit destructor with the clean-up at the delete site
 * is six as well (~CNodeVector called); out of class the destructor is not inlined at all */
void CWaterRippleManager::ClearGroups()
{
    group_iterator i;

    i = m_groups.begin();
    while (i != m_groups.end()) {
        CWaterRippleGroup *g = *i;

        i = m_groups.erase(i);
        delete g;
    }
}

/* the ripple of a name */
CWaterRipple *CWaterRippleManager::Find(const char *name)
{
    CWaterRipple *found = 0;

    if (name) {
        iterator i;

        for (i = m_ripples.begin(); i != m_ripples.end() && !found; ++i) {
            CWaterRipple *r = *i;

            if (r->m_name && !strcmp(r->m_name, name))
                found = r;
        }
    }
    return found;
}

/* a ripple started on a node, unless one there already starts at the point */
CWaterRipple *CWaterRippleManager::Create(CNode *node, const CPnt3D *pos, const float *time,
                                          float speed, float amp, float freq)
{
    bool create = true;
    iterator i;

    for (i = m_ripples.begin(); i != m_ripples.end() && create; ++i) {
        CWaterRipple *r = *i;

        if (r->GetNode(0) == node && pos) {
            if (fabsf(r->GetPos().x - pos->x) < 1.0f && fabsf(r->GetPos().y - pos->y) < 1.0f &&
                fabsf(r->GetPos().z - pos->z) < 1.0f)
                create = false;
        }
    }

    CWaterRipple *r = 0;

    if (create)
        r = new CWaterRipple(node, pos, speed, time, amp, freq);
    return r;
}

/* a ripple started on a group by name, unless one on it already starts at the point */
CWaterRipple *CWaterRippleManager::Create(const char *name, const CPnt3D *pos, const float *time,
                                          float speed, float amp, float freq)
{
    bool create = true;
    CWaterRippleGroup *group = 0;

    if (m_groups.size()) {
        group_iterator g;

        for (g = m_groups.begin(); !group && g != m_groups.end(); ++g) {
            CWaterRippleGroup *gr = *g;

            if (!strcmp(gr->m_name, name))
                group = gr;
        }
    }

    iterator i;

    for (i = m_ripples.begin(); i != m_ripples.end() && create; ++i) {
        CWaterRipple *r = *i;

        if (r->m_group == group && pos) {
            if (fabsf(r->GetPos().x - pos->x) < 1.0f && fabsf(r->GetPos().y - pos->y) < 1.0f &&
                fabsf(r->GetPos().z - pos->z) < 1.0f)
                create = false;
        }
    }

    CWaterRipple *r = 0;

    if (create && group)
        r = new CWaterRipple(group, pos, speed, time, amp, freq);
    return r;
}

/* a ripple started on the group with the surface nearest the point */
CWaterRipple *CWaterRippleManager::Create(const CPnt3D *pos, const float *time, float speed,
                                          float amp, float freq)
{
    CWaterRippleGroup *nearest = 0;
    float best;

    if (pos && m_groups.size()) {
        group_iterator i;

        for (i = m_groups.begin(); i != m_groups.end(); ++i) {
            CWaterRippleGroup *g = *i;
            int j = 0;
            CNode *n;

            while ((n = g->GetNode(j)) != 0) {
                CPnt3D p = CPnt3D::zero;

                n->TransformToWorld(p);

                CPnt3D d = p - *pos;
                float dist = d.Normalize();

                if (!nearest) {
                    best = dist;
                    nearest = g;
                } else if (dist < best) {
                    best = dist;
                    nearest = g;
                }
                j++;
            }
        }
    }

    CWaterRipple *r = 0;

    if (nearest)
        r = new CWaterRipple(nearest, pos, speed, time, amp, freq);
    return r;
}

/* the group of a name, made if there is none */
CWaterRippleGroup *CWaterRippleManager::GetGroup(const char *name)
{
    CWaterRippleGroup *group = 0;

    if (m_groups.size()) {
        group_iterator i;

        for (i = m_groups.begin(); !group && i != m_groups.end(); ++i) {
            CWaterRippleGroup *g = *i;

            if (!strcmp(g->m_name, name))
                group = g;
        }
    }
    if (!group) {
        group = new CWaterRippleGroup(name);
        m_groups.push_back(group);
    }
    return group;
}

/* a surface added, once */
void CWaterRippleGroup::Add(CNode *node)
{
    if (node && !m_nodes.Exists(node)) {
        if (m_nodes.size() == m_nodes.capacity())
            m_nodes.reserve(m_nodes.size() + 1);
        m_nodes.push_back(node);
    }
}

/* on one node, listed with the manager */
CWaterRipple::CWaterRipple(CNode *node, const CPnt3D *pos, float speed, const float *time,
                           float amp, float freq)
{
    Init();
    m_node = node;
    if (pos) {
        m_pos.x = pos->x;
        m_pos.y = pos->y;
        m_pos.z = pos->z;
    }
    m_speed = speed;
    m_amp = amp;
    m_ampMax = amp;
    m_freq = freq;
    if (time && *time > 0.0f) {
        m_timed = true;
        m_time = *time;
        m_swelling = true;
        m_ampRate = m_ampMax / 0.25f;
        m_amp = 0.0f;
    }
    theWaterRippleManager.Add(this);
}

/* on a group */
CWaterRipple::CWaterRipple(CWaterRippleGroup *group, const CPnt3D *pos, float speed,
                           const float *time, float amp, float freq)
{
    Init();
    m_group = group;
    if (pos) {
        m_pos.x = pos->x;
        m_pos.y = pos->y;
        m_pos.z = pos->z;
    }
    m_speed = speed;
    m_amp = amp;
    m_ampMax = amp;
    m_freq = freq;
    if (time && *time > 0.0f) {
        m_timed = true;
        m_time = *time;
        m_swelling = true;
        m_ampRate = m_ampMax / 0.25f;
        m_amp = 0.0f;
    }
    theWaterRippleManager.Add(this);
}

void CWaterRipple::Init()
{
    m_name = 0;
    m_group = 0;
    m_node = 0;
    m_follow = 0;
    m_pos = CPnt3D::zero;
    m_local = CPnt3D::zero;
    m_limited = false;
    m_growing = false;
    m_directional = false;
    m_timed = false;
    m_swelling = false;
    m_level = false;
    m_rising = false;
    m_visible = false;
    m_dir = CPnt3D::zero;
    m_speed = 0.0f;
    m_phase = 0.0f;
    m_radius = 0.0f;
    m_radiusMax = 0.0f;
    m_radiusRate = 0.0f;
    m_amp = 0.0f;
    m_ampMax = 0.0f;
    m_ampRate = 0.0f;
    m_height = 0.0f;
    m_heightMax = 0.0f;
    m_heightRate = 0.0f;
    m_freq = 0.0f;
    m_time = 0.0f;
}

/* named, unless it already has the name */
void CWaterRipple::SetName(const char *name)
{
    bool set = true;

    if (m_name && name && !strcmp(m_name, name))
        set = false;
    if (set) {
        if (m_name)
            _free(m_name, 0, 0);
        if (name)
            m_name = _strdup(name, 0, 0);
    }
}

/* the surface lifted by a height, from the first vertex's rest height; a timed ripple rises to it
 * in a quarter of a second */
void CWaterRipple::SetLevel(float height)
{
    m_level = true;
    m_height = height;
    m_heightMax = height;
    m_restY = 0.0f;

    CNode *n = GetNode(0);

    if (n) {
        CVisualVector::iterator it = n->m_visual.begin();

        if (it != n->m_visual.end()) {
            CVisual *vis = *it;
            CVisData d;
            CPnt3D p;
            float *f = &p.x;
            CVisData *v = d.SelectVertexStrips(vis, 0, 0);

            if (v) {
                GetVertexPos(v, f);
                m_restY = f[1];
            }
        }
    }
    if (m_timed) {
        m_rising = true;
        m_heightRate = m_heightMax / 0.25f;
        m_height = 0.0f;
    }
}

/* reaching out to a radius only, grown to it over a time if one is given */
void CWaterRipple::SetRadius(const float *time, float radius)
{
    m_limited = true;
    m_radiusMax = radius;
    if (time) {
        m_growing = true;
        m_radius = 0.0f;
        m_radiusRate = m_radiusMax / *time;
    } else
        m_radius = m_radiusMax;
}

/* whether a surface was drawn lately, and if one was all of them listed with the manager */
void CWaterRipple::MarkNodes()
{
    m_visible = false;

    int i = 0;
    CNode *n;

    while (!m_visible && (n = GetNode(i)) != 0) {
        if (n->Rendered(2))
            m_visible = true;
        i++;
    }

    int j = 0;

    if (m_visible) {
        while ((n = GetNode(j)) != 0) {
            theWaterRippleManager.AddNode(n);
            j++;
        }
    }
}

/* the amplitude, height, radius and phase moved on; whether the ripple's time is up */
bool CWaterRipple::Tick(float dt)
{
    bool done = false;

    if (m_timed) {
        m_time -= dt;
        if (m_time <= 0.0f)
            done = true;
        if (!done) {
            if (m_swelling) {
                m_amp += m_ampRate * dt;
                if (m_amp >= m_ampMax) {
                    m_swelling = false;
                    m_amp = m_ampMax;
                    m_ampRate = m_amp / m_time;
                }
            } else {
                m_amp -= m_ampRate * dt;
                if (m_amp < 0.0f)
                    m_amp = 0.0f;
            }
            if (m_level) {
                if (m_rising) {
                    m_height += m_heightRate * dt;
                    if (m_height >= m_heightMax) {
                        m_rising = false;
                        m_height = m_heightMax;
                        m_heightRate = m_height / m_time;
                    }
                } else {
                    m_height -= m_heightRate * dt;
                    if (m_height < 0.0f)
                        m_height = 0.0f;
                }
            }
        }
    }
    if (!done) {
        if (m_growing) {
            if (m_radius < m_radiusMax) {
                m_radius += m_radiusRate * dt;
                if (m_radius > m_radiusMax)
                    m_radius = m_radiusMax;
            }
        }
        if (m_timed || m_visible)
            m_phase -= m_speed * dt;
    }
    return done;
}

/* whether the ripple reaches a surface, placed in its frame if it does */
bool CWaterRipple::Affects(CNode *node, const CMatrix *toLocal)
{
    bool place = true;
    bool hit = false;

    if (m_limited) {
        Place(toLocal);
        place = false;
        if (m_node) {
            if (m_node == node) {
                SetBox();
                if (Overlaps(node->GetBBox()))
                    hit = true;
            }
        } else if (m_group) {
            int i = 0;
            CNode *n;

            while (!hit && (n = m_group->GetNode(i)) != 0) {
                i++;
                if (n == node) {
                    SetBox();
                    if (Overlaps(node->GetBBox()))
                        hit = true;
                }
            }
        } else {
            SetBox();
            if (Overlaps(node->GetBBox()))
                hit = true;
        }
    } else {
        if (m_node) {
            if (m_node == node)
                hit = true;
        } else if (m_group) {
            int i = 0;
            CNode *n;

            while (!hit && (n = m_group->GetNode(i)) != 0) {
                i++;
                if (n == node)
                    hit = true;
            }
        }
    }
    if (hit && place)
        Place(toLocal);
    return hit;
}

/* the offset at a point of the surface's frame: along the way out from the ripple, a cosine of
 * the distance (along m_dir if directional) fading to the radius; up by the height if it lifts */
bool CWaterRipple::GetOffset(const CPnt3D *p, CPnt3D *out)
{
    bool ok = false;
    CPnt3D d;
    float dist;

    d = *p - m_local;

    if (m_directional) {
        dist = fabsf(DotXZ(&m_dir, &d));
        d.Normalize();
    } else
        dist = d.Normalize();

    bool in = false;
    float k;

    if (m_limited) {
        if (dist < m_radius) {
            in = true;
            k = 1.0f - dist / m_radius;
        } else
            k = 0.0f;
    } else {
        k = 1.0f;
        in = true;
    }
    if (in) {
        dist *= m_freq;
        dist += m_phase;

        float c = k * cosf(dist);
        float a = c * m_amp;

        out->x = a * d.x;
        out->z = a * d.z;
        if (m_level)
            out->y = c * m_height;
        else
            out->y = 0.0f;
        ok = true;
    }
    return ok;
}

/* a surface laid flat: back to its rest height where the ripple lifted it, its normals up.
 * OPEN, 0x260 against 0x274: the up normal -- the target multiplies 32767.5 by 1.0 and by 0.0 at run
 * time (both products hoisted to the function's top in f20/f21, converted to shorts once per strip,
 * stored with sh), where every spelling tried here folds them to constants (a CPnt3D local, float
 * arguments, a reference temporary, CPnt3D::unit_y, a helper per component; opt_propagation off
 * keeps the constants in registers but multiplies in the loop). The point is written and read
 * through a float pointer as in SetLevel, which matches. Round 35: IRO folds only integer constants
 * (doldecomp mweppc IroEval.c), so the fold here is the propagated constants' later; `up` built at
 * the function's top, at the visual loop's top, from float locals (0.0f / 1.0f), const -- all still
 * 0x260 (build/scratch/wep/v_flat*.py): the target's products, CSE'd for x and z and hoisted to the
 * outer loop's preheader, need the normal's components to reach the backend as variables */
void CWaterRipple::Flatten(CNode *node)
{
    for (CVisualVector::iterator it = node->m_visual.begin(); it != node->m_visual.end(); ++it) {
        CVisual *vis = *it;
        CVisData d;
        CPnt3D p;
        float *f = &p.x;

        for (CVisData *v = d.SelectVertexStrips(vis, 0, 0); v; v = v->NextVertex()) {
            if (m_level) {
                GetVertexPos(v, f);
                f[1] = m_restY;
                SetVertexPos(v, f);
            }

            CPnt3D up(0.0f, 1.0f, 0.0f);

            SetVertexNormal(v, &up);
        }
    }
}

}
