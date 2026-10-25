/* vis_mesh.cpp -- zdb::CMesh and CSubMesh, the skinned meshes (SOCOM 1's vis_mesh.cpp, by the
 * press beta's asserts): OVL1 0x3B8000?-0x3BA2FF, after vis_main.cpp's tail (tag_GlobalLight's
 * constructor, 0x3B7FD0) and before ColConvTo3D, as SOCOM 1's 0x30ED60-0x31045F. Only some of its
 * functions so far; the rest stay generated drafts or placeholders. Defined in reverse of their
 * image order (-inline deferred). */

#include "znode.h"
#include <string.h>
#include "../zsys/zsys_public.h"

namespace zdb {

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

/* constructed empty, as a mesh (type 1): one level of detail, used out to 1e12, room for three
 * (SOCOM 1's used FLT_MAX), no matrices and no decals */
CMesh::CMesh()
{
    m_type = 1;
    m_68 = 0;
    m_70 = 0;
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
    unsigned int prev = m_lods.size() - 2;

    m_lods.begin().m_p[prev].m_dist = dist * dist;
}

/* a view's level of detail by distance, when it has more than one: the first that reaches past
 * the distance (SOCOM 1's; SOCOM II's walk recomputes the end through the class iterator).
 * OPEN, 2 instructions: every instruction but the end -- the target keeps size*12 and re-reads the
 * data pointer each pass (a pointer store in the loop blocking only the pointer load, by type?),
 * this hoists the whole end; `break` or `return`, a const_iterator, postfix ++, an assigned
 * iterator and raw pointers do not change it */
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
