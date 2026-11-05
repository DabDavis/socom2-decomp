/* zparticle.cpp -- the particle system's driver (OVL1 0x3223D0-0x324C6F): the scene nodes particle
 * visuals borrow from a pool, the destruction pieces, the dynamic textures, the per-frame tick and
 * the start. The unit name is SOCOM 1's (its __sinit_zparticle.cpp; SOCOM II's 0x402AD0 builds the
 * node pool, 0x45C440, and the texture list, 0x45C460). SOCOM 1's demo holds these functions in
 * this order, ReleaseInstance to Init. Defined in reverse of their image order (-inline deferred).
 * Written so far: GetFreeInstance, FlushDynTexList, Close, FreeNodePool, Tick and Open. */

#include <iostream>
#include "zparticle.h"
#include "../zsys/zsys.h"
#include <list>
#include <algorithm>
#include <stdio.h>

/* the task scheduler (zanim.h's class; only its AddTask here) */
class CSched_Manager {
public:
    void AddTask(const char *name, bool (*task)(float, void *), float interval, void *data);
    void RemoveTask(const char *name, bool all);
};


extern CSched_Manager zTaskScheduler;


/* a list emptied and its storage given up (zonline.h's ReleaseVector, the same template: not
 * inline, so an instance per list follows FlushDynTexList, the textures' at 0x3247A0 and the
 * palettes' at 0x324720, each with the whole body inlined) */
template <class T>
void ReleaseVector(std::vector<T *> &v)
{
    std::vector<T *> tmp;

    v.clear();
    v.swap(tmp);
}

namespace Particle {

/* the nodes visuals borrow (SOCOM 1's pool: 160 made at the start, handed out and taken back; a
 * CNodeVector, as SOCOM 1's, whose FreeVector runs on the base vector, as node_world.cpp's) */
zdb::CNodeVector particle_node_free_list;   // 0x45C440
int particle_node_count;   // 0x45C428
/* the textures particles draw with (SOCOM 1's) */
zdb::CDynTexList m_dyntexlist;   // 0x45C460


bool Tick(float dT, void *data);
void FreeNodePool();

/* SOCOM 1's: a node's DIs gathered, then its children's, then its model's when it is an instance
 * (the destruction manager collects a hierarchy's collision pieces with it; the name SOCOM 1's) */
void PullVisuals(zdb::CNode *node, zdb::CDIVector *out)
{
    if (!node)
        return;
    for (zdb::CDIVector::iterator d = node->GetDIs().begin(); d != node->GetDIs().end(); ++d)
        out->push_back(*d);
    for (zdb::CNodeVector::iterator i = node->GetChildren().begin(); i != node->GetChildren().end(); ++i)
        PullVisuals(*i, out);
    zdb::CNode *instance = node->m_type == 2 ? node : 0;

    if (instance)
        PullVisuals(instance->m_model, out);
}

/* SOCOM 1's: a three-sided piece made for each collision piece of a node (0x322800, not written:
 * SOCOM II's call passes SOCOM 1's arguments, the second float last) */
void Create3Sided(zdb::CDIVector *dis, zdb::CNode *node, CPnt4D *color, zdb::CTexHandle *tex, float f,
                  zdb::CModel *model, CPnt3D *a, CPnt3D *b, CPnt3D *c, CPnt3D *d, Props *props, float g);

/* SOCOM 1's: a node broken into pieces -- its collision pieces gathered, the texture (or the default
 * "null_xmas.bmp") and its palette kept for the frame, the "fragment" model found, the pieces'
 * properties made (the friction and the acceleration given) and the pieces made by type: 0 three-
 * sided, 1 none (SOCOM II passes a second float on) */
void DestructionManager(zdb::CNode *node, char *texName, CPnt4D *color, float f, CPnt3D *a, CPnt3D *b,
                        CPnt3D *c, CPnt3D *d, int type, float g)
{
    if (!node)
        return;
    zdb::CDIVector dis;

    PullVisuals(node, &dis);
    zdb::CTexHandle *tex = zdb::CWorld::GetTexHandle(texName);

    if (!tex || !tex->m_texture) {
        tex = zdb::CWorld::GetTexHandle("null_xmas.bmp");
        if (!tex || !tex->m_texture)
            return;
    }
    m_dyntexlist.Add(tex, true);
    if (tex->m_texture->m_palette)
        m_dyntexlist.Add(tex->m_texture->m_palette, true);
    zdb::CModel *model = zdb::CWorld::GetModel("fragment");

    if (!model)
        return;
    Props *props = Props::Create();

    props->m_friction = c->z;
    props->m_accelW.x = d->x;
    props->m_accelW.y = d->y;
    props->m_accelW.z = d->z;
    props->m_refCount = 0;
    switch (type) {
    case 0:
        Create3Sided(&dis, node, color, tex, f, model, a, b, c, d, props, g);
        break;
    case 1:
        break;
    default:
        return;
    }
}

/* SOCOM 1's: the dynamic textures let go of */
void FlushDynTexList()
{
    ReleaseVector(m_dyntexlist.m_tex_list);
    ReleaseVector(m_dyntexlist.m_pal_list);
}

/* SOCOM 1's: everything let go of -- the textures, the sources and instances, the pooled pieces
 * (SOCOM II's purge first), the tick, the node pool */
void Close()
{
    Destruction_Instance *p;

    FlushDynTexList();
    Source::list.Uninit();
    Instance::list.Uninit();
    FreeInstances();
    while ((p = Destruction_Instance::m_instance_pool.Pop()) != 0)
        delete p;
    zTaskScheduler.RemoveTask("ParticleTick", false);
    FreeNodePool();
}

/* SOCOM 1's: every pooled node deleted, the pool's storage given back */
void FreeNodePool()
{
    std::for_each(particle_node_free_list.begin(), particle_node_free_list.end(), zdb::CNode::Delete);
    particle_node_free_list.clear();
    FreeVector<std::vector<zdb::CNode *> >(particle_node_free_list);
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
