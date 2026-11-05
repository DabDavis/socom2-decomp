/* vis_load.cpp -- a node's visuals read out of a ZAR (OVL1 0x3B2CB0-0x3B36EF, between zvid_init.cpp,
 * which zVid_Assert (0x3B2C80) closes, and vis_vu.cpp; the name is ours, SOCOM 1 has no such unit).
 * Its static initialiser is 0x404500 -- <iostream>'s two objects alone (0x4B45F0, 0x4B4608), between
 * zvid_init.cpp's (0x404490) and vis_vu.cpp's (0x404570), as the unit's code sits between theirs.
 * A mesh node's data is the key "MESH_<name>"; any other node's the key of its own name, its visuals
 * read by hookupVisuals (SOCOM 1's, 0x3B2FF0, recursive). Defined in reverse of their image order
 * (-inline deferred). */

#include <iostream>
#include <stdio.h>
#include <string.h>

#include "znode.h"
#include "../zar/zar.h"

using namespace zdb;
using namespace zar;

/* (0x3E14A8: the node number in the visuals' keys -- set to -1 before a node's visuals are read,
 * counted up for each node with visuals; SOCOM 1's is a gp global) */
extern "C" int data_003E14A8;

/* whether the views' buffers share their owners' counts (vis_main.cpp's 0x4B5010: set while a
 * visual's level-of-detail views are trimmed) */
extern "C" unsigned char data_004B5010;

/* a visual let go of by its node (SOCOM 1's CNode::DeleteVisual(CVisual *, bool) by its place in
 * the drop walk; node_main.cpp's range, not written) */
extern "C" void func_003162B0(CNode *node, CVisual *visual, bool b);

/* a node's visuals read into the block, and its children's (SOCOM 1's hookupVisuals, 0x30A330 in the
 * demo: the same keys, calls and order). For each of the model's instances, the node's counterpart
 * in it (or the instance itself) takes the instance's index as its view; each visual gains a
 * view's level of detail and its buffer is the data at the offset the key "N<node>_I<instance>_V<
 * visual>" holds -- its "_L" key's when there is one, which marks the counterpart lit -- or failing
 * both the key "N<node>_<visual>", which takes view 0 and trims the views a level (the buffers then
 * keeping their owners' counts). A model without instances, or a mesh, or one marked (SOCOM II's
 * byte at 0xC0, where SOCOM 1 tested m_bForceExport at 0xC4) reads the node's own visuals the same
 * way for view 0, both keys with their "_L" variants. A visual no key holds is let go of; the
 * children that are not instances are read in turn. True when any buffer was set */
/* a view's level of detail added to a visual, its two lists each a word longer (inline: the visual
 * copied into a register for the two pushes, re-read from its address-taken local after; the name
 * ours) */
static inline void AddView(CVisual *v)
{
    v->m_lodMesh.push_back(0);
    v->m_24.push_back(0);
}

bool hookupVisuals(CZAR *zar, CKey *key, CNode *node, CModel *model, CVisBase *block)
{
    char lname[1024];
    char name[1024];
    bool result = false;

    if (!node)
        return false;

    const char *fmtI = "N%03d_I%03d_V%02d";
    const char *fmtN = "N%03d_%03d";
    CVisualVector &visuals = node->m_visual;
    CRefList &list = model->m_list;
    std::vector<CVisual *> drop;
    int off = 0;

    if (!node->m_hasMesh)
        node->GetSubMesh();
    if (node->m_visual.size() || node->m_has_visuals_prior_to_export)
        data_003E14A8++;

    int i = 0;

    for (CRefList::iterator it = list.begin(), last = list.end(); it != last; ++it, i++) {
        CNode *instance = *it;
        CNode *inst = instance->FindChild(node->m_name, true);

        if (!inst)
            inst = instance;
        inst->m_vid = i;

        int j = 0;
        CVisual *visual = 0;
        CVisualVector::iterator e = visuals.end();

        for (CVisualVector::iterator v = visuals.begin(); v != e; ++v, j++) {
            data_004B5010 = 0;
            visual = *v;
            AddView(visual);
            sprintf(name, fmtI, data_003E14A8, i, j);
            strcpy(lname, name);
            strcat(lname, "_L");

            bool ok = zar->Fetch(lname, &off, 4);

            if (ok)
                inst->SetDynamicLight(true, false);
            else
                ok = zar->Fetch(name, &off, 4);
            if (!ok) {
                sprintf(name, fmtN, data_003E14A8, j);
                i = 0;
                inst->m_vid = 0;
                if (visual->m_lodMesh.size() > 1) {
                    visual->m_lodMesh.pop_back();
                    visual->m_24.pop_back();
                    data_004B5010 = 1;
                }
                ok = zar->Fetch(name, &off, 4);
            }
            if (!ok)
                drop.push_back(visual);
            else {
                block->m_c = off;
                visual->SetBuffer((_word128 *)((char *)block->m_data + block->m_c), i, block);
                result = true;
            }
        }
    }
    if (list.size() == 0 || *(unsigned char *)&model->m_variant || model->GetMesh()) {
        int j = 0;
        CVisual *visual = 0;
        CVisualVector::iterator e = visuals.end();

        for (CVisualVector::iterator v = visuals.begin(); v != e; ++v, j++) {
            data_004B5010 = 0;
            visual = *v;
            AddView(visual);
            sprintf(name, fmtI, data_003E14A8, 0, j);
            strcpy(lname, name);
            strcat(lname, "_L");

            bool ok = zar->Fetch(lname, &off, 4);

            if (ok)
                node->SetDynamicLight(true, false);
            else
                ok = zar->Fetch(name, &off, 4);
            if (!ok) {
                sprintf(name, fmtN, data_003E14A8, j);
                strcpy(lname, name);
                strcat(lname, "_L");
                ok = zar->Fetch(lname, &off, 4);
                if (ok)
                    node->SetDynamicLight(true, false);
                else
                    ok = zar->Fetch(name, &off, 4);
            }
            if (!ok)
                drop.push_back(visual);
            else {
                block->m_c = off;
                visual->SetBuffer((_word128 *)((char *)block->m_data + block->m_c), 0, block);
                result = true;
            }
        }
    }
    if (drop.size()) {
        for (std::vector<CVisual *>::iterator d = drop.begin(); d != drop.end(); ++d)
            func_003162B0(node, *d, true);
        drop.clear();
    }
    for (CNodeVector::iterator c = node->m_child.begin(); c != node->m_child.end(); ++c) {
        CNode *child = *c;
        CNode *instance = child->m_type == 2 ? child : 0;

        if (!instance && hookupVisuals(zar, key, child, model, block))
            result = true;
    }
    return result;
}

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
        ok = hookupVisuals(zar, key, node, (CModel *)node, block);
    if (!ok)
        delete block;
    zar->CloseKey(key);
}
