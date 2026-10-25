/* zparticle.cpp -- the particle system's driver (OVL1 0x3223D0-0x324C6F): the scene nodes particle
 * visuals borrow from a pool, the destruction pieces, the dynamic textures, the per-frame tick and
 * the start. The unit name is SOCOM 1's (its __sinit_zparticle.cpp; SOCOM II's 0x402AD0 builds the
 * node pool, 0x45C440, and the texture list, 0x45C460). SOCOM 1's demo holds these functions in
 * this order, ReleaseInstance to Init. Defined in reverse of their image order (-inline deferred).
 * Written so far: Tick and Open. */

#include "zparticle.h"
#include "../zsys/zsys.h"
#include <list>
#include <algorithm>
#include <stdio.h>

/* the task scheduler (zanim.h's class; only its AddTask here) */
class CSched_Manager {
public:
    void AddTask(const char *name, bool (*task)(float, void *), float interval, void *data);
};
extern CSched_Manager zTaskScheduler;

namespace zdb {
class CCamera {
public:
    float m_matrix[4][4];   // 0x0: the node's matrix, its position the last row (CNode's)
};
/* the world, as much as the particles use (znode.h's class brings zmath.h's CPnt4D with it) */
class CWorld : public CNode {
public:
    static CWorld *m_world;

    unsigned char _64[0x128 - 0x64];
    CCamera *m_camera;   // 0x128
};
}

namespace Particle {

/* the nodes visuals borrow (SOCOM 1's pool: 160 made at the start, handed out and taken back) */
std::vector<zdb::CNode *> particle_node_free_list;   // 0x45C440
int particle_node_count;   // 0x45C428

bool Tick(float dT, void *data);

/* SOCOM 1's: every pooled node deleted, the pool's storage given back */
void FreeNodePool()
{
    std::for_each(particle_node_free_list.begin(), particle_node_free_list.end(), zdb::CNode::Delete);
    particle_node_free_list.clear();
    FreeVector(particle_node_free_list);
}

/* SOCOM 1's: a node from the pool (one more made when it is empty, the pool grown ten at a time),
 * put in the world and switched on */
zdb::CNode *GetFreeInstance()
{
    if (particle_node_free_list.size() == 0) {
        char name[28];
        zdb::CNode *node;

        sprintf(name, "PNODE%d", particle_node_free_list.size());
        node = zdb::CNode::Create(name);
        if (particle_node_free_list.size() == particle_node_free_list.capacity())
            particle_node_free_list.reserve(particle_node_free_list.capacity() + 10);
        particle_node_free_list.push_back(node);
    }
    zdb::CNode *node = particle_node_free_list.back();

    particle_node_free_list.pop_back();
    zdb::CWorld::m_world->AddChild(node);
    node->SetActive(true);
    particle_node_count++;
    return node;
}

/* SOCOM 1's: the instances and the sources ticked, the camera's position kept for their fades */
bool Tick(float dT, void *data)
{
    float *m = zdb::CWorld::m_world->m_camera->m_matrix[3];

    Source::referencePositionW = CPnt3D(m[0], m[1], m[2]);
    Instance::list.Tick(dT);
    Source::List::iterator e = Source::list.end();

    for (Source::List::iterator i = Source::list.begin(); i != e; ++i)
        (*i)->Tick(dT);
    return false;
}

/* SOCOM 1's: the factors at full, the tick scheduled, the node pool filled (SOCOM II reserves it
 * first) */
void Open()
{
    Instance::fadeFactor = 1.0f;
    Instance::spawnFactor = 1;
    Instance::ageFactor = 1.0f;
    Source::distanceFactor = 1.0f;
    Source::frequencyFactor = 1.0f;
    zTaskScheduler.AddTask("ParticleTick", Tick, 0.2f, 0);
    if (particle_node_free_list.capacity() < 160)
        particle_node_free_list.reserve(160);
    particle_node_count = 0;
    for (int i = 0; i < 160; i++) {
        char name[28];
        zdb::CNode *node;

        sprintf(name, "PNODE%d", i);
        node = zdb::CNode::Create(name);
        particle_node_free_list.push_back(node);
    }
}

}
