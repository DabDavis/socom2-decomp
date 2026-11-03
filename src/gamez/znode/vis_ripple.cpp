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

#include "znode.h"
#include "../zsys/zsys.h"

/* the element at an index of a list of nodes, or none (node_vector.cpp's placeholder) */
extern "C" zdb::CNode *func_00311EA0(zdb::CNodeVector *v, int i);

namespace zdb {

/* a strip vertex's position: the strip's origin (floats) plus the vertex's offset from it, shorts
 * in sixteenths */
inline void GetVertexPos(CVisData *v, CPnt3D *p)
{
    p->x = ((float *)v->m_14)[0] + ((short *)v->m_vertex)[0] / 16.0f;
    p->y = ((float *)v->m_14)[1] + ((short *)v->m_vertex)[1] / 16.0f;
    p->z = ((float *)v->m_14)[2] + ((short *)v->m_vertex)[2] / 16.0f;
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

/* a named set of water surfaces (RIPPLE_NODE_GROUP's; 0x10 bytes) */
class CWaterRippleGroup {
public:
    CWaterRippleGroup(const char *name)
    {
        if (name)
            m_name = _strdup(name, 0, 0);
        else
            m_name = 0;
        m_nodes.clear();
    }
    ~CWaterRippleGroup()
    {
        if (m_name) {
            _free(m_name, 0, 0);
            m_name = 0;
        }
        m_nodes.clear();
    }
    /* the i-th surface, none past the last */
    CNode *GetNode(int i) { return func_00311EA0(&m_nodes, i); }
    void Add(CNode *node);

    char *m_name;   // 0x0
    CNodeVector m_nodes;   // 0x4
};

/* a ripple (0x88 bytes) */
class CWaterRipple {
public:
    CWaterRipple(CWaterRippleGroup *group, const CPnt3D *pos, const float *time, float speed,
                 float amp, float freq);
    CWaterRipple(CNode *node, const CPnt3D *pos, const float *time, float speed, float amp,
                 float freq);
    ~CWaterRipple()
    {
        if (m_name)
            _free(m_name, 0, 0);
    }
    void Init();
    void SetName(const char *name);
    void SetLevel(float height);
    void SetRadius(const float *time, float radius);
    void MarkNodes();
    bool Tick(float dt);
    bool Affects(CNode *node, const CMatrix *toLocal);
    bool GetOffset(const CPnt3D *p, CPnt3D *out);
    void Flatten(CNode *node);

    /* the i-th surface rippled: its own node first, else its group's */
    CNode *GetNode(int i)
    {
        CNode *n = 0;

        if (i == 0 && m_node)
            n = m_node;
        else if (m_group)
            n = m_group->GetNode(i);
        return n;
    }
    /* the position, carried with the node it follows */
    const CPnt3D &GetPos()
    {
        if (m_follow) {
            m_pos = CPnt3D::zero;
            m_follow->TransformToWorld(m_pos);
        }
        return m_pos;
    }
    /* within a unit of a point on each axis */
    bool IsAt(const CPnt3D *p)
    {
        return p && fabsf(GetPos().x - p->x) < 1.0f && fabsf(GetPos().y - p->y) < 1.0f &&
               fabsf(GetPos().z - p->z) < 1.0f;
    }
    /* the position in a surface's frame */
    void Place(const CMatrix *toLocal) { toLocal->Transform(&GetPos(), &m_local, 1); }
    /* the square the ripple reaches, in the surface's frame, overlapping the surface's bounds */
    bool Reaches(CNode *node)
    {
        m_box.m_min.x = m_local.x - m_radius;
        m_box.m_min.y = m_local.y;
        m_box.m_min.z = m_local.z - m_radius;
        m_box.m_max.x = m_local.x + m_radius;
        m_box.m_max.y = m_local.y;
        m_box.m_max.z = m_local.z + m_radius;

        const CBBox *b = node->GetBBox();

        return b->m_max.x >= m_box.m_min.x && b->m_min.x <= m_box.m_max.x &&
               b->m_max.z >= m_box.m_min.z && b->m_min.z <= m_box.m_max.z;
    }
    bool IsActive() { return m_timed || m_visible; }
    bool Lifts() { return m_level; }

    CWaterRippleGroup *m_group;   // 0x0
    CNode *m_node;   // 0x4
    CNode *m_follow;   // 0x8: RIPPLE's node
    CPnt3D m_pos;   // 0xc: the world position
    CPnt3D m_local;   // 0x18: in the surface's frame
    CBBox m_box;   // 0x24
    char *m_name;   // 0x3c
    unsigned char m_limited : 1;   // 0x40: out to m_radius only
    unsigned char m_growing : 1;   // the radius growing
    unsigned char m_directional : 1;   // the distance along m_dir
    unsigned char m_timed : 1;   // it dies after m_time
    unsigned char m_swelling : 1;   // the amplitude rising
    unsigned char m_level : 1;   // the surface lifted by m_height
    unsigned char m_rising : 1;   // the height rising
    unsigned char m_visible : 1;   // a surface drawn lately
    CPnt3D m_dir;   // 0x44
    float m_speed;   // 0x50
    float m_phase;   // 0x54
    float m_radius;   // 0x58
    float m_radiusMax;   // 0x5c
    float m_radiusRate;   // 0x60
    float m_amp;   // 0x64
    float m_ampMax;   // 0x68
    float m_ampRate;   // 0x6c
    float m_restY;   // 0x70: the surface's height at rest
    float m_height;   // 0x74
    float m_heightMax;   // 0x78
    float m_heightRate;   // 0x7c
    float m_freq;   // 0x80
    float m_time;   // 0x84: left to live
};

/* the ripples, the groups, the surfaces to move this frame and the ripples on the one being moved
 * (0x30 bytes) */
class CWaterRippleManager {
public:
    typedef std::list<CWaterRipple *>::iterator iterator;
    typedef std::list<CWaterRippleGroup *>::iterator group_iterator;

    CWaterRippleManager();
    CWaterRippleGroup *GetGroup(const char *name);
    CWaterRipple *Create(const CPnt3D *pos, const float *time, float speed, float amp, float freq);
    CWaterRipple *Create(const char *name, const CPnt3D *pos, const float *time, float speed,
                         float amp, float freq);
    CWaterRipple *Create(CNode *node, const CPnt3D *pos, const float *time, float speed,
                         float amp, float freq);
    CWaterRipple *Find(const char *name);
    void ClearGroups();
    void ClearRipples();
    void Clear();
    void Ripple(CNode *node);
    bool GetOffset(const CPnt3D *p, CPnt3D *out, bool *level);
    void Gather(CNode *node);
    bool Update(float dt);
    bool Tick(float dt);

    CWaterRippleGroup *FindGroup(const char *name)
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
        return group;
    }
    iterator Remove(iterator i) { return m_ripples.erase(i); }
    /* a ripple listed (its constructors) */
    void Add(CWaterRipple *r) { m_ripples.push_back(r); }
    /* a surface to move this frame, once */
    void AddNode(CNode *node)
    {
        if (node && !m_nodes.Exists(node)) {
            if (m_nodes.size() == m_nodes.capacity())
                m_nodes.reserve(m_nodes.size() + 1);
            m_nodes.push_back(node);
        }
    }

    std::list<CWaterRipple *> m_ripples;   // 0x0
    std::list<CWaterRippleGroup *> m_groups;   // 0xc
    CNodeVector m_nodes;   // 0x18
    std::list<CWaterRipple *> m_active;   // 0x24
};

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

        for (n = func_00311EA0(&m_nodes, 0); n; n = func_00311EA0(&m_nodes, i)) {
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

/* the live ripples that reach a surface listed, each placed in its frame */
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

        if (r->IsActive() && r->Affects(node, &m))
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

/* a surface's vertices moved: each one's normal tilted by the offset, its height set where a
 * ripple lifts it */
void CWaterRippleManager::Ripple(CNode *node)
{
    Gather(node);
    if (m_active.size()) {
        for (CVisualVector::iterator it = node->m_visual.begin(); it != node->m_visual.end(); ++it) {
            CVisData d;

            for (CVisData *v = d.SelectVertexStrips(*it, 0, 0); v; v = v->NextVertex()) {
                CPnt3D p;
                CPnt3D off;
                bool level;

                GetVertexPos(v, &p);
                if (GetOffset(&p, &off, &level)) {
                    CPnt3D n;

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

        if (r->GetNode(0) == node && r->IsAt(pos))
            create = false;
    }

    CWaterRipple *r = 0;

    if (create)
        r = new CWaterRipple(node, pos, time, speed, amp, freq);
    return r;
}

/* a ripple started on a group by name, unless one on it already starts at the point */
CWaterRipple *CWaterRippleManager::Create(const char *name, const CPnt3D *pos, const float *time,
                                          float speed, float amp, float freq)
{
    bool create = true;
    CWaterRippleGroup *group = FindGroup(name);
    iterator i;

    for (i = m_ripples.begin(); i != m_ripples.end() && create; ++i) {
        CWaterRipple *r = *i;

        if (r->m_group == group && r->IsAt(pos))
            create = false;
    }

    CWaterRipple *r = 0;

    if (create && group)
        r = new CWaterRipple(group, pos, time, speed, amp, freq);
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

                if (!nearest || dist < best) {
                    best = dist;
                    nearest = g;
                }
                j++;
            }
        }
    }

    CWaterRipple *r = 0;

    if (nearest)
        r = new CWaterRipple(nearest, pos, time, speed, amp, freq);
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
CWaterRipple::CWaterRipple(CNode *node, const CPnt3D *pos, const float *time, float speed,
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
CWaterRipple::CWaterRipple(CWaterRippleGroup *group, const CPnt3D *pos, const float *time,
                           float speed, float amp, float freq)
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
            CVisData d;
            CVisData *v = d.SelectVertexStrips(*it, 0, 0);

            if (v) {
                CPnt3D p;

                GetVertexPos(v, &p);
                m_restY = p.y;
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
            if (m_node == node && Reaches(node))
                hit = true;
        } else if (m_group) {
            int i = 0;
            CNode *n;

            while (!hit && (n = m_group->GetNode(i)) != 0) {
                i++;
                if (n == node && Reaches(node))
                    hit = true;
            }
        } else if (Reaches(node))
            hit = true;
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
    CPnt3D d = *p - m_local;
    float dist;

    if (m_directional) {
        dist = fabsf(m_dir.x * d.x + m_dir.z * d.z);
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
        float c = k * cosf(dist * m_freq + m_phase);
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

/* a surface laid flat: back to its rest height where the ripple lifted it, its normals up */
void CWaterRipple::Flatten(CNode *node)
{
    for (CVisualVector::iterator it = node->m_visual.begin(); it != node->m_visual.end(); ++it) {
        CVisData d;

        for (CVisData *v = d.SelectVertexStrips(*it, 0, 0); v; v = v->NextVertex()) {
            if (m_level) {
                CPnt3D p;

                GetVertexPos(v, &p);
                p.y = m_restY;
                SetVertexPos(v, &p);
            }

            CPnt3D up(0.0f, 1.0f, 0.0f);

            SetVertexNormal(v, &up);
        }
    }
}

}
