/* node_model.cpp -- zdb::CNode's model side: its model, its cross-references and instances, its
 * visuals' first, its bounds (OVL1 0x3178A0-...). A unit of its own: the creators here call
 * SetName and SetPosition, which node_main.cpp's own callers inline where they can (SOCOM 1's
 * order runs the same way, its template instances and CTickLightList's destructor between). The
 * unit name is ours. Defined in reverse of their image order (-inline deferred). */

#include "znode.h"

namespace zdb {

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

}
