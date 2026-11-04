/* vis_load.cpp -- a node's visuals read out of a ZAR (OVL1 0x3B2CB0-0x3B36EF, between zvid_init.cpp,
 * which zVid_Assert (0x3B2C80) closes, and vis_vu.cpp; the name is ours, SOCOM 1 has no such unit).
 * Its static initialiser is 0x404500 -- <iostream>'s two objects alone (0x4B45F0, 0x4B4608), between
 * zvid_init.cpp's (0x404490) and vis_vu.cpp's (0x404570), as the unit's code sits between theirs.
 * A mesh node's data is the key "MESH_<name>"; any other node's the key of its own name, its visuals
 * read by 0x3B2FF0 (0x700 bytes, recursive: not written). Defined in reverse of their image order
 * (-inline deferred). */

#include <iostream>
#include <stdio.h>
#include <string.h>

#include "znode.h"
#include "../zar/zar.h"

using namespace zdb;
using namespace zar;

/* the visuals of a key read into the block, recursively (0x3B2FF0: not written) */
extern "C" bool func_003B2FF0(CZAR *zar, CKey *key, CNode *node, CNode *root, CVisBase *block);

/* (0x3E14A8: set to -1 before a node's visuals are read -- 0x3B2FF0's) */
extern "C" int data_003E14A8;

/* a mesh node's data: the key "MESH_<name>" fetched in place, the selects starting at the offset
 * its "_START" key holds, the matrix count, a view in both view lists, the first level of detail's
 * buffer and selects, the mesh's texture selects resolved (the name ours) */
#pragma auto_inline off
void LoadMeshNode(CZAR *zar, CNode *node)
{
    CMesh *mesh = node->GetMesh();
    char name[192];

    sprintf(name, "MESH_%s", node->m_name);
    CKey *key = zar->OpenKey(name);

    if (key) {
        void *buf = 0;

        if (zar->FetchLIP(key, &buf) != false) {
            int start;

            strcat(name, "_START");
            zar->Fetch(name, &start);
            zar->Fetch("mtx_count", &mesh->m_numMatrices);
            _word128 *head = (_word128 *)((char *)buf + start);

            mesh->m_lodMesh.push_back(0);
            mesh->m_24.push_back(0);
            _word128 *b = (_word128 *)buf;
            CMesh::CMeshLOD &lod = mesh->m_lods[0];

            lod.m_mesh = (CMesh *)head;
            lod.m_buffer = b;
            mesh->SetMeshTextureSelects(head, (_word128 *)buf);
        }
        zar->CloseKey(key);
    }
}
#pragma auto_inline reset

/* a node's visuals: a mesh's own way; otherwise its key's data fetched in place into a static block
 * the size of the key and read, the block freed when either fails. A node without a key copies its
 * visual list into a vector it then drops (likely what used it compiled out) (the name ours; OVL1's
 * 0x1E91C0 calls it). The target's shapes: the count an int and the array a local (both loaded once,
 * the node's register reused), the key's size read before the `new` (a call in an argument list
 * runs first), the two reads' result in a bool tested after them (the join's label padded ahead of
 * its branch), the delete's own null test the only one */
void LoadNodeVisuals(CZAR *zar, CNode *node)
{
    if (node->GetMesh()) {
        LoadMeshNode(zar, node);
        return;
    }
    data_003E14A8 = -1;
    CKey *key = zar->OpenKey(node->m_name);

    if (!key) {
        std::vector<CVisual *> visuals;
        int n = node->m_visual.size();
        CVisual **v = &node->m_visual[0];

        for (int i = 0; i < n; i++)
            visuals.push_back(v[i]);
        return;
    }
    int size = key->m_size;
    CVisBase *block = new CVisBase(size, true);

    bool ok = zar->FetchLIP(key, (void **)block);

    if (ok)
        ok = func_003B2FF0(zar, key, node, node, block);
    if (!ok)
        delete block;
    zar->CloseKey(key);
}
