/* vis_mesh.cpp -- zdb::CMesh and CSubMesh, the skinned meshes (SOCOM 1's vis_mesh.cpp, by the
 * press beta's asserts): OVL1 0x3B8000?-0x3BA2FF, after vis_main.cpp's tail (tag_GlobalLight's
 * constructor, 0x3B7FD0) and before ColConvTo3D, as SOCOM 1's 0x30ED60-0x31045F. Only some of its
 * functions so far; the rest stay generated drafts or placeholders. Defined in reverse of their
 * image order (-inline deferred). */

#include "znode.h"
#include "../zsys/zsys_public.h"

using namespace zdb;

/* a matrix per sub-mesh, each new and the identity (SOCOM 1's) */
void CMesh::InitMatrices()
{
    for (unsigned int i = 0; i < m_numMatrices; i++)
        m_palette.push_back(new CMatrix);
}

/* a view's level of detail, clamped to the last one (SOCOM 1's took it as given) */
void CMesh::SetLOD(unsigned int view, unsigned int lod)
{
    if (lod >= m_lods.size())
        lod = m_lods.size() - 1;
    m_lodMesh[view] = m_lods[lod].m_mesh;
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
