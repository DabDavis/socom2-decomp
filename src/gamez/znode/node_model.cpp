/* node_model.cpp -- zdb::CNode's model side: its model, its cross-references and instances, its
 * visuals' first, its bounds (OVL1 0x3178A0-...). A unit of its own: the creators here call
 * SetName and SetPosition, which node_main.cpp's own callers inline where they can (SOCOM 1's
 * order runs the same way, its template instances and CTickLightList's destructor between). The
 * unit name is ours. Defined in reverse of their image order (-inline deferred). */

#include "znode.h"

namespace zdb {

/* an instance let go of: off the list, and no longer the model's parent if it was -- written out
 * (through RemoveInstance the by-value copy shares the binder's frame block: 16 bytes short) */
void CModel::Release(CNode *node)
{
    m_list.remove(node);
    if (m_parent == node)
        m_parent = 0;
}

/* a facade's rotation (a billboard): its first three rows turned to face from the viewer's
 * position to its own -- about y only (facade 1), or about y and then x (facade 2: the direction
 * turned into the yz plane to find the x angle).
 * OPEN (0x238 of 0x240): the target keeps &m(2,0) in s2 on both paths and copies the third row
 * out through it -- the address shared with facade 2's stores -- while copying each row as a
 * block (four loads, four stores). A source through the accessor shares the address but
 * copies element by element; through the array it copies by blocks and shares nothing. */
void CNode::ComputeFacadeMatrix(const CMatrix *view, CMatrix *out)
{
    CMatrix m;
    CPnt3D dir = *(const CPnt3D *)out->m_matrix[3] - *(const CPnt3D *)view->m_matrix[3];

    switch (m_facade) {
    case 1:
        dir.y = 0.0f;
        dir.Normalize();
        m(0, 0) = dir.z;
        m(0, 2) = -dir.x;
        m(2, 0) = dir.x;
        m(2, 2) = dir.z;
        break;
    case 2: {
        dir.Normalize();
        CPnt3D flat = dir;

        flat.y = 0.0f;
        flat.Normalize();
        m(0, 0) = flat.x;
        m(0, 2) = -flat.z;
        m(2, 0) = flat.z;
        m(2, 2) = flat.x;
        CPnt4D v(dir.x, dir.y, dir.z, 0.0f);
        CPnt4D r;

        m.Transform(&v, &r, 1);
        m(0, 0) = flat.z;
        m(0, 2) = -flat.x;
        m(2, 0) = flat.x;
        m(2, 2) = flat.z;
        CMatrix rx;

        rx(1, 1) = r.x;
        rx(1, 2) = -r.y;
        rx(2, 1) = r.y;
        rx(2, 2) = r.x;
        m = m * rx;
        break;
    }
    default:
        return;
    }
    *(CPnt4D *)out->m_matrix[0] = *(CPnt4D *)m.m_matrix[0];
    *(CPnt4D *)out->m_matrix[1] = *(CPnt4D *)m.m_matrix[1];
    *(CPnt4D *)out->m_matrix[2] = *(CPnt4D *)m.m_matrix[2];
}

/* the node's bounds made current; the node's own are, a borrowed parent's are fetched (the base
 * class's virtual) */
CBBox *CNode::UpdateBBox(bool up)
{
    return (CBBox *)GetBBox();
}

/* the first visual as a mesh, if it is one (type 1) */
CMesh *CNode::GetMesh()
{
    CVisual *v = GetFirstVisual();

    if (v && v->m_type == 1)
        return (CMesh *)v;
    return 0;
}

/* the first visual as a sub-mesh, if it is one */
CSubMesh *CNode::GetSubMesh()
{
    CVisual *v = GetFirstVisual();

    if (v && v->m_type == 2)
        return dynamic_cast<CSubMesh *>(v);
    return 0;
}

/* the first of the node's visuals, if it has one */
CVisual *CNode::GetFirstVisual()
{
    CVisualVector &v = m_visual;

    return v.empty() ? 0 : v[0];
}

/* an instance of a model: a copy of it, placed and turned as asked, on the model's list (the
 * model's parent the first one), the model's and named after it -- type 2; one of the common
 * character set's also gets the 4 flag and a bump map */
CNode *CNode::CreateInstance(CModel *model, const CPnt3D *pos, const CQuat *rot)
{
    if (!model)
        return 0;
    CNode *node = model->Copy();

    if (pos)
        node->SetPosition(pos);
    if (rot)
        node->SetRotation(rot);
    model->AddInstance(node);
    node->m_model = model;
    node->SetModelname(model->m_name);
    node->m_type = 2;
    if (model->m_AssetLib->m_renderphase == CHARSETCOMMON) {
        node->m_socom2_a1_bits.bit2 = 1;
        node->m_bumpmap = 1;
    }
    return node;
}

/* an instance of a model the world has by name: the instance code inlined -- its whole tree four
 * high, so the list's push_back makes the end iterator itself (include/list); found, then passed,
 * the model (passed straight from GetModel's call, the inlined body schedules otherwise) */
CNode *CNode::CreateInstance(const char *name, const CPnt3D *pos, const CQuat *rot)
{
    if (!name)
        return 0;
    if (!CWorld::m_world)
        return 0;
    CModel *model = (CModel *)CWorld::GetModel(name);

    return CreateInstance(model, pos, rot);
}

/* a cross-reference to a model: a new type-3 node named "XRef", placed and turned as asked, with the
 * model its child; one of the common character set's also gets the 4 flag and a bump map */
CNode *CNode::CreateXRef(CModel *model, const CPnt3D *pos, const CQuat *rot)
{
    if (!model)
        return 0;
    CNode *node = new CNode;

    if (pos)
        node->SetPosition(pos);
    if (rot)
        node->SetRotation(rot);
    node->m_type = 3;
    node->SetName("XRef");
    node->AddChild(model);
    if (model->m_AssetLib->m_renderphase == CHARSETCOMMON) {
        node->m_socom2_a1_bits.bit2 = 1;
        node->m_bumpmap = 1;
    }
    return node;
}

/* a node deleted: an instance taken off its model's list first, a type-4 node's first child
 * handed to its parent in its place */
CNode *CNode::Delete(CNode *node)
{
    if (!node)
        return 0;
    switch (node->GetType()) {
    case 2: {
        CModel *model = node->m_model;

        if (model)
            model->RemoveInstance(node);
        node->RemoveFromParent();
        break;
    }
    case 4: {
        CNode *parent = node->m_parent;
        CNode *child = node->m_child.empty() ? 0 : node->m_child[0];

        node->RemoveFromParent();
        if (child)
            child->RemoveFromParent();
        if (parent)
            parent->AddChild(child);
        break;
    }
    }
    delete node;
    return 0;
}

}
