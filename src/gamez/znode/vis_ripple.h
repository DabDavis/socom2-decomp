/* vis_ripple.h -- SOCOM II's water ripples (vis_ripple.cpp, OVL1 0x3C0CF0-0x3C356F): a ripple, a named
 * group of water surfaces and the manager, theWaterRippleManager (0x4B5170), that CCoreState::Tick ticks
 * and the ZAnim RIPPLE and RIPPLE_NODE_GROUP ticks feed (zanim_cmd_tick.cpp). No SOCOM 1 counterpart:
 * every name here is ours. */

#ifndef VIS_RIPPLE_H
#define VIS_RIPPLE_H

#include "znode.h"
#include "../zsys/zsys.h"

/* the element at an index of a list of nodes, or none (node_vector.cpp's placeholder: the groups' and
 * the manager's surface accessor) */
extern "C" zdb::CNode *func_00311EA0(zdb::CNodeVector *v, int i);

namespace zdb {

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
    CWaterRipple(CWaterRippleGroup *group, const CPnt3D *pos, float speed, const float *time,
                 float amp, float freq);
    CWaterRipple(CNode *node, const CPnt3D *pos, float speed, const float *time, float amp,
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
    void SetBox()
    {
        m_box.m_min.x = m_local.x - m_radius;
        m_box.m_min.y = m_local.y;
        m_box.m_min.z = m_local.z - m_radius;
        m_box.m_max.x = m_local.x + m_radius;
        m_box.m_max.y = m_local.y;
        m_box.m_max.z = m_local.z + m_radius;
    }
    /* the square overlapping a box in XZ */
    bool Overlaps(const CBBox *b)
    {
        return (b->m_max.x >= m_box.m_min.x && b->m_min.x <= m_box.m_max.x) &&
               (b->m_max.z >= m_box.m_min.z && b->m_min.z <= m_box.m_max.z);
    }
    bool IsActive() { return m_timed || m_visible; }
    bool Lifts() { return m_level; }
    /* the distance taken along a direction in XZ, or from the point if none (RIPPLE's flag 8) */
    void SetDir(const PNT3D *p)
    {
        if (p) {
            m_directional = true;
            m_dir.x = p->x;
            m_dir.y = p->y;
            m_dir.z = p->z;
        } else
            m_directional = false;
    }
    /* dying after a time, a lift not still rising falling over it (RIPPLE's flag 2) */
    void SetTime(float t)
    {
        m_timed = true;
        m_time = t;
        if (m_level && !m_rising)
            m_heightRate = m_height / m_time;
    }

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
    /* a ripple's node unlinked (Update's and ClearRipples' erase): through an inline the erase's
     * argument and result temporaries are made after an inlined clear()'s, as the target's stack
     * slots have them -- erase called straight puts them first. A stand-in: MSL's list<T *> likely
     * has an inline erase(iterator) (include/list's wrapper has none; ClearGroups is exact either way) */
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

/* the manager (vis_ripple.cpp's initialiser 0x404B30 builds it and registers its destructor) */
extern CWaterRippleManager theWaterRippleManager;

}

#endif
