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

/* the node number in the visuals' keys (0x3E14A8, SOCOM 1's node_index, a gp global there) -- set
 * to -1 before a model's visuals are read, counted up for each node with visuals */
extern int node_index;


/* a node's visuals read into the block, and its children's (SOCOM 1's hookupVisuals, 0x30A330 in the
 * demo: the same keys, calls and order). For each of the model's instances, the node's counterpart
 * in it (or the instance itself) takes the instance's index as its view; each visual gains a
 * view's level of detail and its buffer is the data at the offset the key "N<node>_I<instance>_V<
 * visual>" holds -- its "_L" key's when there is one, which marks the counterpart lit -- or failing
 * both the key "N<node>_<visual>", which takes view 0 and trims the views a level (the buffers then
 * keeping their owners' counts). A model without instances, or a mesh, or one marked (SOCOM II's
 * byte at 0xC0, where SOCOM 1 tested m_bForceExport at 0xC4) reads the node's own visuals the same
 * way for view 0, both keys with their "_L" variants. A visual no key holds is let go of; the
 * children that are not instances are read in turn. True when any buffer was set.
 * BYTE-EXACT (the weapons' round 32), by these shapes: the format strings and the two containers
 * (the node's visuals, the model's instances) locals defined after the null test, before the drop
 * list (the target's frame slots 224-176); the instances' end beside its begin in the for; each
 * visual walk's iterator, end, then the visual (address-taken: the drop list's push takes it) and
 * the count; the two pushes and the trim through inlines taking the visual by value (the target
 * copies it into a register for them and re-reads the local after); `i = 0` ahead of the fallback's
 * sprintf (the target's delay slot); ONE count j for both walks -- MW splits it and numbers the
 * second range after the copy's temporary (`tools/igreplay.py --move j=73.5`: 5 -> 0 differing); the
 * own walk's drop-and-continue inside its retry (the first `!ok` jumps straight to the set-up);
 * the drop walk's and the children's ends taken with their begins; a child's result materialised
 * (`r = false; if (!instance) r = ...; if (r) result = true;`) */
/* a view's level of detail added to a visual, its two lists each a word longer (inline: the visual
 * copied into a register for the two pushes, re-read from its address-taken local after; the name
 * ours) */
static inline void AddView(CVisual *v)
{
    v->m_lodMesh.push_back(0);
    v->m_24.push_back(0);
}

/* a visual's last view's level of detail dropped while it has more than one, the views' buffers then
 * keeping their owners' counts (inline, the visual by value as AddView's; the name ours) */
static inline void DropView(CVisual *v)
{
    if (v->m_lodMesh.size() > 1) {
        v->m_lodMesh.pop_back();
        v->m_24.pop_back();
        CVisual::m_noBufferRefs = 1;
    }
}

bool hookupVisuals(CZAR *zar, CKey *key, CNode *node, CModel *model, CVisBase *block)
{
    char name[1024];
    char lname[1024];
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
        node_index++;

    int i = 0;
    int j;

    for (CRefList::iterator it = list.begin(), last = list.end(); it != last; ++it, i++) {
        CNode *instance = *it;
        CNode *inst = instance->FindChild(node->m_name, true);

        if (!inst)
            inst = instance;
        inst->m_vid = i;

        CVisualVector::iterator v = visuals.begin();
        CVisualVector::iterator e = visuals.end();
        CVisual *visual = 0;

        j = 0;

        for (; v != e; ++v, j++) {
            CVisual::m_noBufferRefs = 0;
            visual = *v;
            AddView(visual);
            sprintf(name, fmtI, node_index, i, j);
            strcpy(lname, name);
            strcat(lname, "_L");

            bool ok = zar->Fetch(lname, &off, 4);

            if (ok)
                inst->SetDynamicLight(true, false);
            else
                ok = zar->Fetch(name, &off, 4);
            if (!ok) {
                i = 0;
                sprintf(name, fmtN, node_index, j);
                inst->m_vid = 0;
                DropView(visual);
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
    if (list.size() == 0 || model->m_bForceExport || model->GetMesh()) {
        CVisualVector::iterator v = visuals.begin();
        CVisualVector::iterator e = visuals.end();
        CVisual *visual = 0;

        j = 0;

        for (; v != e; ++v, j++) {
            CVisual::m_noBufferRefs = 0;
            visual = *v;
            AddView(visual);
            sprintf(name, fmtI, node_index, 0, j);
            strcpy(lname, name);
            strcat(lname, "_L");

            bool ok = zar->Fetch(lname, &off, 4);

            if (ok)
                node->SetDynamicLight(true, false);
            else
                ok = zar->Fetch(name, &off, 4);
            if (!ok) {
                sprintf(name, fmtN, node_index, j);
                strcpy(lname, name);
                strcat(lname, "_L");
                ok = zar->Fetch(lname, &off, 4);
                if (ok)
                    node->SetDynamicLight(true, false);
                else
                    ok = zar->Fetch(name, &off, 4);
                if (!ok) {
                    drop.push_back(visual);
                    continue;
                }
            }
            block->m_c = off;
            visual->SetBuffer((_word128 *)((char *)block->m_data + block->m_c), 0, block);
            result = true;
        }
    }
    if (drop.size()) {
        for (std::vector<CVisual *>::iterator d = drop.begin(), de = drop.end(); d != de; ++d)
            node->DeleteVisual(*d, true);   /* SOCOM 1's CNode::DeleteVisual (0x3162B0) */
        drop.clear();
    }
    for (CNodeVector::iterator c = node->m_child.begin(), ce = node->m_child.end(); c != ce; ++c) {
        bool r = false;
        CNode *child = *c;
        CNode *instance = child->m_type == 2 ? child : 0;

        if (!instance)
            r = hookupVisuals(zar, key, child, model, block);
        if (r)
            result = true;
    }
    return result;
}

/* a mesh model's data (SOCOM 1's hookupMesh(CZAR *, CModel *), 0x30A1F0 in the demo, after
 * hookupVisuals(CZAR *, CModel *) as here: the same calls and order): the key "MESH_<name>" fetched
 * in place, the selects starting at the offset its "_START" key holds, the matrix count, a view in
 * both view lists, the first level of detail's buffer and selects, the mesh's texture selects
 * resolved */
#pragma auto_inline off
void hookupMesh(CZAR *zar, CModel *node)
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

/* a model's visuals (SOCOM 1's hookupVisuals(CZAR *, CModel *), 0x30A040 in the demo, before
 * hookupMesh as here: GetMesh, then node_index set to -1 and the key of the model's name opened):
 * a mesh's own way; otherwise its key's data fetched in place into a static block the size of the
 * key and read, the block freed when either fails. A model without a key copies its visual list
 * into a vector it then drops (likely what used it compiled out) (OVL1's 0x1E91C0 calls it). The
 * target's shapes: the count an int and the array a local (both loaded once, the node's register
 * reused), the key's size read before the `new` (a call in an argument list runs first), the two
 * reads' result in a bool tested after them (the join's label padded ahead of its branch), the
 * delete's own null test the only one */
void hookupVisuals(CZAR *zar, CModel *node)
{
    if (node->GetMesh()) {
        hookupMesh(zar, node);
        return;
    }
    node_index = -1;
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
        ok = hookupVisuals(zar, key, node, node, block);
    if (!ok)
        delete block;
    zar->CloseKey(key);
}
