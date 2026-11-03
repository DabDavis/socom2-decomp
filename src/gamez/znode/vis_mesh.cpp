/* vis_mesh.cpp -- zdb::CMesh and CSubMesh, the skinned meshes (SOCOM 1's vis_mesh.cpp, by the
 * press beta's asserts): OVL1 0x3B8000?-0x3BA2FF, after vis_main.cpp's tail (tag_GlobalLight's
 * constructor, 0x3B7FD0) and before ColConvTo3D, as SOCOM 1's 0x30ED60-0x31045F. Only some of its
 * functions so far; the rest stay generated drafts or placeholders. Defined in reverse of their
 * image order (-inline deferred). */

#include "znode.h"
#include <string.h>
#include "../zsys/zsys_public.h"

namespace zdb {

class CMeshDecal;

/* the mesh decals' pools (vis_decal.cpp's class: only the take here) */
class CMeshDecalPool {
public:
    CMeshDecal *Get(bool b);
};

/* a decal on a mesh: what the mesh's walks touch (vis_decal.cpp's class; SOCOM 1's, no DWARF --
 * the names ours). A decal's life below zero is a permanent one */
class CMeshDecal {
public:
    void FreeMem();
    static void PutTempDecal(CMeshDecal *d);
    static void PutPermDecal(CMeshDecal *d);
    /* cleared for reuse: the life, the link and the vertex words (inline; SOCOM 1's stores alike) */
    void Reset()
    {
        m_life = 0.0f;
        m_next = 0;
        m_8 = 0;
        m_verts = 0;
        m_34 = 0;
        m_30 = 0;
    }

    static CMeshDecalPool m_tempPool;
    static CMeshDecalPool m_permPool;

    float m_life;   // 0x0
    CMeshDecal *m_next;   // 0x4
    unsigned int m_8;   // 0x8
    unsigned int m_c;   // 0xc
    void *m_verts;   // 0x10
    unsigned char _14[0x30 - 0x14];
    unsigned int m_30;   // 0x30
    unsigned int m_34;   // 0x34
};

}

using namespace zdb;

/* a mesh decal built over a strip of the mesh's vertices, posed by its matrices (SOCOM II's, not
 * named: vis_data.cpp's span) */
extern "C" void func_003BD9E0(CMeshDecal *d, void *strip, CTexHandle *tex, CMesh *mesh,
                              std::vector<CMatrix *> *palette, float life);

/* the decal list's end reached (inline: the walk's materialised test; the name ours) */
static inline bool DecalEnd(CMeshDecal *d) { return !d; }

/* constructed empty, as a mesh (type 1): one level of detail, used out to 1e12, room for three
 * (SOCOM 1's used FLT_MAX), no matrices and no decals */
CMesh::CMesh()
{
    m_type = 1;
    m_original = 0;
    m_owner = 0;
    m_1c = 3;

    CMeshLOD lod;

    lod.m_dist = 1e12f;
    lod.m_mesh = 0;
    lod.m_buffer = 0;
    m_lods.reserve(3);
    m_lods.push_back(lod);
    m_numMatrices = 0;
    m_74 = 0;
    m_decals = 0;
}

/* a matrix per sub-mesh, each new and the identity (SOCOM 1's) */
void CMesh::InitMatrices()
{
    for (unsigned int i = 0; i < m_numMatrices; i++)
        m_palette.push_back(new CMatrix);
}

/* another mesh's first level of detail appended: its select block copied and pointed at its
 * buffer, its textures listed, and the previous level used within the distance, squared
 * (SOCOM 1's) */
void CMesh::AddLOD(CMesh *mesh, float dist)
{
    m_lods.push_back(*mesh->m_lods.begin());

    _word128 *buffer = (*mesh->m_lods.begin()).m_buffer;
    _word128 *selects = (_word128 *)(*mesh->m_lods.begin()).m_mesh;
    unsigned int size = (selects->u16[0] + 1) << 4;
    _word128 *copy = (_word128 *)_malloc(size, 0, 0);

    memcpy(copy, selects, size);
    SetMeshBuffer(copy, buffer, selects);
    m_dynTexList.Add(&mesh->m_dynTexList);

    CMeshLOD &lod = m_lods.back();

    lod.m_mesh = (CMesh *)copy;
    lod.m_buffer = buffer;
    m_lods[m_lods.size() - 2].m_dist = dist * dist;
}

/* a view's level of detail by distance, when it has more than one: the first that reaches past
 * the distance (SOCOM 1's; SOCOM II's walk re-reads the data pointer each pass: include/vector's
 * class end() takes `data() + size` into a const local) */
void CMesh::SetLOD(unsigned int view, float dist)
{
    if (m_lods.size() > 1) {
        for (std::vector<CMeshLOD>::iterator it = m_lods.begin(); it != m_lods.end(); ++it) {
            if (dist < (*it).m_dist) {
                m_lodMesh[view] = (*it).m_mesh;
                break;
            }
        }
    }
}

/* a view's level of detail, clamped to the last one (SOCOM 1's took it as given) */
void CMesh::SetLOD(unsigned int view, unsigned int lod)
{
    if (lod >= m_lods.size())
        lod = m_lods.size() - 1;
    m_lodMesh[view] = m_lods[lod].m_mesh;
}

/* a decal added over a strip of the mesh, last in its list: taken from the permanent pool for a
 * life below zero, else from the temporary one, cleared and built (SOCOM II's; the name ours).
 * OPEN, 0x11C against 0x124: the target pads both arms of the pool choice with a nop before the
 * next label and fills the second call's delay slot with the li; the decal's clear then interleaves
 * the life's mov.s earlier -- a ternary, the tests reversed, a bool, a pool pointer, inline helpers
 * (if/return, a result local, a ternary) and an early return all miss */
void CMesh::AddDecal(void *strip, CTexHandle *tex, float life)
{
    CMeshDecal *d;

    if (life < 0.0f)
        d = CMeshDecal::m_permPool.Get(false);
    else
        d = CMeshDecal::m_tempPool.Get(true);
    if (d) {
        CMeshDecal *last = m_decals;

        while (!DecalEnd(last) && last->m_next)
            last = last->m_next;
        d->Reset();
        func_003BD9E0(d, strip, tex, this, &m_palette, life);
        d->m_next = 0;
        if (last)
            last->m_next = d;
        else
            m_decals = d;
        FlushCache(0);
    }
}

/* a copy of the mesh for a node: the visual's and the mesh's members assigned, a reference to this
 * mesh taken, its own matrices, and its first level's selects copied and pointed at this mesh's
 * buffer (SOCOM 1's took no node) */
CMesh *CMesh::Clone(CNode *owner)
{
    CMesh *mesh = new CMesh;

    *mesh = *this;
    mesh->m_refs = 0;
    m_refs++;
    mesh->m_original = dynamic_cast<CMesh *>((CVisual *)this);
    mesh->InitMatrices();
    mesh->m_owner = owner;
    m_74 = 0;

    _word128 *buffer = (*m_lods.begin()).m_buffer;
    _word128 *selects = (_word128 *)(*m_lods.begin()).m_mesh;
    unsigned int size = (selects->u16[0] + 1) << 4;
    _word128 *copy = (_word128 *)_malloc(size, 0, 0);

    memcpy(copy, selects, size);
    mesh->SetMeshBuffer(copy, buffer, selects);
    mesh->m_dynTexList.Add(&m_dynTexList);

    CMeshLOD &lod = mesh->m_lods.back();

    lod.m_mesh = (CMesh *)copy;
    lod.m_buffer = buffer;
    mesh->m_lodMesh[0] = (CMesh *)copy;
    return mesh;
}

/* its decals let go of, each to the pool its life says; a clone's reference given back, the
 * original deleted by its last holder, and the levels' select blocks freed; its matrices deleted
 * (SOCOM 1's gave every decal back as temporary and marked the original 0xDEADBEEF) */
CMesh::~CMesh()
{
    CMeshDecal *d;
    bool clone = m_original != 0;

    d = m_decals;

    if (d) {
        do {
            CMeshDecal *next = d->m_next;
            float life = d->m_life;

            d->FreeMem();
            d->Reset();
            if (life >= 0.0f)
                CMeshDecal::PutTempDecal(d);
            else
                CMeshDecal::PutPermDecal(d);
            d = next;
        } while (d);
    }
    m_decals = 0;
    if (clone) {
        if (m_original->Release() == 0) {
            delete m_original;
            m_original = 0;
        }
        while (m_lods.size()) {
            CMeshLOD &lod = m_lods.back();

            if (lod.m_mesh)
                _free(lod.m_mesh, 0, 0);
            lod.m_mesh = 0;
            m_lods.pop_back();
        }
    }
    while (m_palette.size()) {
        CMatrix *m = m_palette.back();

        m_palette.pop_back();
        if (m)
            delete m;
    }
}

/* a block of selects pointed at a buffer: its decals dropped, the buffer recorded in the head,
 * each matrix select given its matrix (the palette grown to reach it -- SOCOM 1's took the
 * palette as it was) and each data select offset into the buffer (SOCOM 1's; the names unused) */
void CMesh::SetMeshBuffer(_word128 *head, _word128 *base, _word128 *names)
{
    m_decals = 0;
    head->u32[2] = (unsigned int)base;
    unsigned int n = head->u16[0];

    head++;
    for (unsigned int i = 0; i < n; i++, head++) {
        switch (head->u8[2]) {
        case 9: {
            unsigned int idx = head->u32[1];

            while (idx >= m_palette.size())
                m_palette.push_back(new CMatrix);
            head->u32[1] = (unsigned int)m_palette[idx];
            break;
        }
        case 7:
        case 10:
        case 11:
        case 12:
            head->u32[1] += (unsigned int)base;
            break;
        case 5:
        case 6:
        case 8:
            break;
        }
    }
}

/* a block of texture selects resolved once: the head's count, its done flag set, each select of
 * type 6 (a name) resolved (SOCOM 1's) */
void CMesh::SetMeshTextureSelects(_word128 *head, _word128 *names)
{
    unsigned char *done = &head->u8[2];

    if (*done == 0) {
        unsigned int n = head->u16[0];

        *done = 1;
        head++;
        for (unsigned int i = 0; i < n; i++, head++) {
            switch (head->u8[2]) {
            case 6:
                ResolveTextureName(names, head);
                break;
            }
        }
    }
}

/* a decal unlinked from the mesh's list, its link cleared (SOCOM 1's) */
void CMesh::RemoveMeshDecal(CMeshDecal *decal)
{
    CMeshDecal *d = m_decals;
    CMeshDecal *prev = 0;

    while (d) {
        if (d == decal) {
            CMeshDecal *next = d->m_next;

            if (prev)
                prev->m_next = next;
            else
                m_decals = next;
            d->m_next = 0;
            break;
        }
        prev = d;
        d = d->m_next;
    }
}

/* every decal of the mesh let go of: freed, cleared, and given back to the pool its life says it
 * came from (SOCOM 1's gave each back as temporary) */
void CMesh::DereferenceDecals(unsigned int vid)
{
    CMeshDecal *d = m_decals;

    if (d) {
        do {
            bool temp;
            CMeshDecal *next = d->m_next;

            temp = d->m_life >= 0.0f;

            d->FreeMem();
            d->Reset();
            if (temp)
                CMeshDecal::PutTempDecal(d);
            else
                CMeshDecal::PutPermDecal(d);
            d = next;
        } while (d);
    }
    m_decals = 0;
}
