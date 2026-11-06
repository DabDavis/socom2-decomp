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

/* (a non-const point: through a `const CPnt3D *` MW hoists &p->y and &p->z as addresses of their own,
 * through this every access stays on the one pointer register with offsets -- Ripple's s3) */
inline void SetVertexPos(CVisData *v, CPnt3D *p)
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

/* A MARKED DEVICE (the weapons' round 40): a flag that folds between pass 0's loop phase and pass 1 --
 * see Ripple's note. The original's spelling of that per-strip assignment is unknown. */
struct RipFlag {
    int m_on;
};

/* BYTE-EXACT since the weapons' round 40. Before: OPEN, 0x330 bytes (57 instructions off as written, the right size): the target addresses the
 * vertex point through one pointer register (s3 = &p, set in SelectVertexStrips' delay slot each
 * strip, so hoisted out of the vertex loop only) and keeps this in s8; here the point's y and z
 * addresses are hoisted as well and this spills. SetLevel's shape (a float pointer to the point,
 * set before the select) matches there; here it hoists the pointer out of both loops and spills
 * this and node (0x360), declared in the vertex loop it is propagated (0x340). The locals are
 * declared in the target's slot order (n 288, p 272, off 256, the bool at 303). Round 35: p and a
 * float or CPnt3D pointer to it at the strip loop's top (the pointer then used for the reads, the
 * offset and the store) are 0x358 (build/scratch/wep/v_rip.py). Round 36: SetVertexPos and
 * SetVertexNormal taking non-const pointers (Flatten's one-register rule), or the point through a
 * `float *` with the non-const setter, leave it at 57 (v_rip36.py); a member Get on the point or a
 * reference parameter hoists its field addresses as here (build/scratch/wep/t/f8-f9.cpp: only a
 * pointer variable, `P *pp = &p` in the outer body, gives one register with offsets -- hoisted to
 * the function's top, not set per strip as the target's s3). Round 37 (v_rip37*.py): the point
 * through an inline returning its pointer (the HUD's round-61 temporary MW does not propagate) in the
 * strip loop 0x358, in the vertex loop 0x340, at each use 57; the outer loop written as an `if`
 * around a do-while, or a while, 57-59.
 * Round 38 (build/scratch/wep/v_rip38*.py): 4 INSTRUCTIONS OFF, the target's code but one placement.
 * The target loads *it before d's constructor (`lw s3,0(s2)` ahead of Clear: `CVisual *vis = *it`
 * declared first), keeps the point behind one pointer set per strip (s3, reused from vis) with
 * offsets -- `CPnt3D *pp = &p` declared inside a hand-written guard around a do-while (round 30's
 * rule), SetVertexPos taking a non-const point (a `const CPnt3D *` hoists &pp->y and &pp->z) -- and
 * declares n before p in the strip body (n 288, p 272) and steps `v = v->NextVertex(); } while (v);`
 * (the target tests s4 after the move). Left: the target sets s3 in SelectVertexStrips' delay slot,
 * BEFORE the guard's beqz (whose slot is a nop); here it is the guard's first instruction, moved into
 * the beqz's slot -- `pp = &p` anywhere in the strip body ahead of the guard (after the select, before
 * it, in a for-init, as a cast, `&p + 0`, an inline's return) is pulled out of the visual loop by
 * IRO_PullOutInvariantAssignments (the loop is then entered by a `b` to an out-of-line guard: 0x348-
 * 0x360), and p's other uses written directly do not stop it; `if ((v = d.SelectVertexStrips(...)) !=
 * 0)` tests v0 instead (4 off). Flatten's target has the same per-strip s3 in the select's slot: the
 * shape that keeps an invariant assignment in the strip body ahead of the vertex loop is the open
 * question of both. A per-strip inline taking the point (the vertex code written out: a leaf) is
 * inlined but builds n and off by calls (0x340); with the vertex inlines it is cut to one level.
 * Round 39 (igdump --iro, build/scratch/wep/v_rip39*.py): mwccps2's log runs the loop phase
 * (IRO_PullOutInvariantAssignments, RotateLoop, the invariants, strength reduction) in pass 0 only,
 * after that pass's first propagation and before its common subexpressions; pass 1 moves nothing out
 * of a loop -- so an assignment that becomes `pp = &p` only after pass 0's loop phase stays per strip.
 * The AI core's round-36 lever was tried for it: the pointer held in a struct (scalarised in pass 1
 * when its address, taken through a reference or pointer, is folded away) -- bound inside the strip
 * body or the guard, pass 0's first propagation folds the reference before the loop phase and the
 * store `r.m_p = &p` is hoisted with the out-of-line guard (0x350; 0x358 with the pointer at offset
 * 4); bound at the function's top, through a cast or a ternary, it is never folded and r stays in
 * the frame (0x338); a plain pointer kept address-taken stays in the frame to the end (only class
 * objects are scalarised in pass 1). A fold that lands between pass 0's loop phase and pass 1 is
 * the shape still wanted.
 * Round 40 (build/scratch/wep/v_rip40*.py): THAT SHAPE, A LATE-FOLDING GUARD. A struct flag stored at
 * the function's top and read through a reference bound at the read (`RipFlag &rf = f; if (rf.m_on)
 * pp = &p;`): pass 0's scalariser sees f's address taken and leaves it; pass 0's first propagation
 * folds the reference; the loop phase sees a conditional `pp = &p`, not must-reach, so it stays per
 * strip; pass 1 scalarises f, the flag's constant folds the `if` away and nothing is left of it but
 * the assignment, ahead of the select -- its dead-argument rule moves vis's a1 first, so vis and pp
 * do not interfere and share s3 (v declared before pp: the target's s4). The store has to be outside
 * the loops (inside, it is an invariant assignment and pulled with the out-of-line guard), the
 * reference inside (bound at the top it is never folded and the flag stays in the frame).
 * a surface's vertices moved: each one's normal tilted by the offset, its height set where a
 * ripple lifts it */
void CWaterRippleManager::Ripple(CNode *node)
{
    RipFlag f;

    f.m_on = 1;
    Gather(node);
    if (m_active.size()) {
        for (CVisualVector::iterator it = node->m_visual.begin(); it != node->m_visual.end(); ++it) {
            CVisual *vis = *it;
            CVisData d;
            CPnt3D n;
            CPnt3D p;
            RipFlag &rf = f;
            CVisData *v;
            CPnt3D *pp;

            if (rf.m_on)
                pp = &p;

            v = d.SelectVertexStrips(vis, 0, 0);

            if (v) {
                do {
                    CPnt3D off;
                    bool level;

                    GetVertexPos(v, pp);
                    if (GetOffset(pp, &off, &level) != false) {
                        GetVertexNormal(v, &n);
                        n.x = off.x;
                        n.z = off.z;
                        n.y = 1.0f;
                        n.Normalize();
                        SetVertexNormal(v, &n);
                        if (level) {
                            pp->y = off.y;
                            SetVertexPos(v, pp);
                        }
                    }
                    v = v->NextVertex();
                } while (v);
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

/* BYTE-EXACT since the weapons' round 40, at level 5: the group's delete written out, the name's free,
 * the clear and ~CNodeVector called on the member from here, so ~CNodeVector (five high) inlines whole.
 * A marked stand-in for "Level 6, or 8?" (`#pragma inline_depth(6)` at the unit's end also gives it,
 * 42 of 42, the source as it was): the original's delete of a destructor six high.
 * Before: OPEN, 0xD8 against 0xF8: the group's delete -- the target inlines its destructor whole (the
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
        if (g) {
            if (g->m_name) {
                _free(g->m_name, 0, 0);
                g->m_name = 0;
            }
            g->m_nodes.clear();
            g->m_nodes.~CNodeVector();
            operator delete(g);
        }
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
 * Before round 40: OPEN, 0x260 against 0x274: the up normal -- the target multiplies 32767.5 by 1.0 and by 0.0 at run
 * time (both products hoisted to the function's top in f20/f21, converted to shorts once per strip,
 * stored with sh), where every spelling tried here folds them to constants (a CPnt3D local, float
 * arguments, a reference temporary, CPnt3D::unit_y, a helper per component; opt_propagation off
 * keeps the constants in registers but multiplies in the loop). The point is written and read
 * through a float pointer as in SetLevel, which matches. Round 35: IRO folds only integer constants
 * (doldecomp mweppc IroEval.c), so the fold here is the propagated constants' later; `up` built at
 * the function's top, at the visual loop's top, from float locals (0.0f / 1.0f), const -- all still
 * 0x260 (build/scratch/wep/v_flat*.py): the target's products, CSE'd for x and z and hoisted to the
 * outer loop's preheader, need the normal's components to reach the backend as variables.
 * Round 36: IroEval.c does fold two float constants (EMUL of EFLOATCONSTs; only a monadic cast of
 * a FLOAT constant is left), and mwcps2 folds every spelling measured -- the components through
 * float, int-converted, inline-returned, aggregate-initialised or const-qualified locals, at the
 * top or in either loop (build/scratch/wep/t/f1-f10.cpp); a const aggregate at the top keeps the
 * products hoisted into saved FPRs but loads the components from the stack. The point's ONE
 * pointer register is measured: SetVertexPos(CVisData *, const float *) makes MW hoist &p.y and
 * &p.z as addresses of their own (here s1/s0); with a `float *` parameter every access goes
 * through one register with offsets, as the target's s3 -- but hoisted out of both loops (0x248),
 * where the target sets it in SelectVertexStrips' delay slot each strip (no placement of the
 * pointer, in the strip loop, after the select or in the lift's arm, keeps it there).
 * Round 37 (igdump --iro, build/scratch/wep/flat.iro.log; v_flat37*.py): IRO_ScalarizeClassDataMembers
 * makes up's fields scalars, the FIRST IRO_CopyAndConstantPropagation substitutes 0 / 1 into the
 * products and the first IRO_ConstantFolding folds them -- all before the loop passes, so the target's
 * products (hoisted by IRO_MoveInvariantExpressions, multiplied at run time) reached the loop passes
 * with an operand IRO could not propagate. Double literals or a `(float)1.0` fold at parse time; an
 * inline taking double, int or float parameters folds the whole store to `li 32767` (0x240); a static
 * const array is loaded per vertex (0x288); the internal pragmas opt_foldconstants, convertfpconsts,
 * scalarize, decomposeaggregates, pulloutconstants (and seven more) change nothing; opt_propagation
 * off (it governs the whole function, wherever it sits -- scoped around up's declaration it is
 * ignored) with up at the top gives run-time products but in the strip loop, the 16.0 hoisted too
 * (0x27C). AND THE TARGET'S OUTER LOOP IS ROTATED (its test copied ahead of the body, the products in
 * that preheader), ours is not -- the log rotates only the vertex loop (header 5); Ripple's target
 * rotates its outer loop too.
 * Round 39 (build/scratch/wep/v_flat39*.py): the loop phase runs in pass 0 only (Ripple's note), so
 * the products want operands that are not constants until after it -- up's scalarisation deferred to
 * pass 1 (a reference or pointer bound to it, in the vertex body or at the function's top) leaves up
 * in the frame and the outer loop unrotated (0x2A8); Ripple's vertex loop (an `if (v)` guard around a
 * do-while, the float pointer inside the guard or at the strip's top, up in the body or at the
 * function's top) gives 0x234 / 0x260.
 * Round 40 (build/scratch/wep/v_flat40*.py): BYTE-EXACT, by three shapes. (1) Ripple's late-folding
 * guard for the per-strip pointer. (2) THE PRODUCTS: `32767.5f * (float)x` with x a float local
 * holding 0 or 1 -- the no-op cast is what IRO leaves (propagation reaches the constant, and a
 * monadic cast of a float constant is not folded, so the product's operand is never a constant): the
 * multiplication stays to the back end, hoisted to the outer loop's preheader by
 * IRO_MoveInvariantExpressions, its constants in f0 / f1 / f2. (3) THE SHORTS: named `short` locals
 * defined at the vertex body's top and read after the `if`, so in another block -- not expression-
 * propagated, they are the invariant assignments IRO_PullOutInvariantAssignments pulls into the
 * out-of-line guard-plus-preheader (with the sign extension a short variable costs) -- sx read twice,
 * x and z sharing it. A third local for z makes a third pull (0x288). What the original wrote for
 * the 0 and the 1 is not known: the cast and the flag are marked devices */
void CWaterRipple::Flatten(CNode *node)
{
    RipFlag fl;

    fl.m_on = 1;
    for (CVisualVector::iterator it = node->m_visual.begin(); it != node->m_visual.end(); ++it) {
        CVisual *vis = *it;
        CVisData d;
        CPnt3D p;
        RipFlag &rf = fl;
        CVisData *v;
        CPnt3D *pp;

        if (rf.m_on)
            pp = &p;
        v = d.SelectVertexStrips(vis, 0, 0);

        for (; v; v = v->NextVertex()) {
            float x = 0.0f;
            float y = 1.0f;
            short sx = (short)(32767.5f * (float)x);
            short sy = (short)(32767.5f * (float)y);

            if (m_level) {
                GetVertexPos(v, pp);
                pp->y = m_restY;
                SetVertexPos(v, pp);
            }

            ((short *)v->m_vertex)[3] = sx;
            ((short *)v->m_vertex)[6] = sy;
            ((short *)v->m_vertex)[7] = sx;
        }
    }
}

}
