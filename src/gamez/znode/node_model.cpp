/* node_model.cpp -- zdb::CNode's model side: its model, its cross-references and instances, its
 * visuals' first, its bounds (OVL1 0x3178A0-...). A unit of its own: the creators here call
 * SetName and SetPosition, which node_main.cpp's own callers inline where they can (SOCOM 1's
 * order runs the same way, its template instances and CTickLightList's destructor between). The
 * unit name is ours. Defined in reverse of their image order (-inline deferred). */

#include "node_world.h"

namespace zdb {

/* a model let go of: taken off its library (the library's copy of it found by name), each
 * instance and cross-reference of it untied (SetModel(0) inlined: a cross-reference's children
 * deleted, the model and its name cleared, the dead arm of the name's ternary left in), the list
 * emptied (SOCOM 1's). The library's copy taken off by name through RemoveModel(const char *),
 * whose tree is taller than the level: its body inlined, RemoveModel(CModel *) and GetModel called
 * (their copies 0x318AE0 and 0x318C30 follow, last call first) -- written out here, MW inlines
 * RemoveModel's body (0x1B4 of 0x1AC, round 28). SetModel is this unit's own (its last function,
 * generated first), inlined */
CModel::~CModel()
{
    CAssetLib *lib = m_AssetLib;

    if (lib) {
        lib->RemoveModel(m_name);
        m_AssetLib = 0;
    }
    if (m_list.size()) {
        for (CRefList::iterator it = m_list.begin(); it != m_list.end(); ++it) {
            CNode *node = *it;

            node->SetModel(0);
            node->SetModelname(0);
        }
    }
    m_list.clear();
}

/* an instance let go of: off the list, and no longer the model's parent if it was -- written out
 * (through RemoveInstance the by-value copy shares the binder's frame block: 16 bytes short) */
void CModel::Release(CNode *node)
{
    m_list.remove(node);
    if (m_parent == node)
        m_parent = 0;
}

/* a box's eight corners through a matrix, the box about them made (ours -- the device of
 * zparticle_source.cpp's FaceCorners: the target builds the corners through CPnt3D()'s out-of-line
 * copy 0x1FB3C0 in a loop and assigns the first through CPnt3D's out-of-line assignment 0x20F1F0,
 * which a MEMBER array built by a constructor too big to expand whole gives -- MW inlines the
 * constructor one level, its callees called) */
struct BoxCorners {
    BoxCorners(const CMatrix *m, const CBBox *in, CBBox *out)
    {
        GetBBoxCorners(m_pts, in);
        m->Transform(m_pts, m_pts, 8);
        out->m_min = m_pts[0];
        out->m_max = m_pts[0];
        for (int i = 1; i < 8; i++) {
            out->m_min.x = m_pts[i].x <= out->m_min.x ? m_pts[i].x : out->m_min.x;
            out->m_min.y = m_pts[i].y <= out->m_min.y ? m_pts[i].y : out->m_min.y;
            out->m_min.z = m_pts[i].z <= out->m_min.z ? m_pts[i].z : out->m_min.z;
            out->m_max.x = m_pts[i].x >= out->m_max.x ? m_pts[i].x : out->m_max.x;
            out->m_max.y = m_pts[i].y >= out->m_max.y ? m_pts[i].y : out->m_max.y;
            out->m_max.z = m_pts[i].z >= out->m_max.z ? m_pts[i].z : out->m_max.z;
        }
    }

    CPnt3D m_pts[8];
};

/* a box through a matrix (0x3186B0, new in SOCOM II: UpdateBBoxFromDI's, the name ours): exactly,
 * the box about its eight transformed corners; else its two corners transformed and each axis put
 * back in order. Not auto-inlined: it holds the corners' constructor, which cannot inline whole */
CBBox *TransformBBox(const CMatrix *m, const CBBox *in, CBBox *out, bool exact)
{
    if (exact) {
        BoxCorners corners(m, in, out);
    } else {
        m->Transform(&in->m_min, &out->m_min, 2);
        if (!(out->m_min.x <= out->m_max.x)) {
            float t = out->m_min.x;

            out->m_min.x = out->m_max.x;
            out->m_max.x = t;
        }
        if (!(out->m_min.y <= out->m_max.y)) {
            float t = out->m_min.y;

            out->m_min.y = out->m_max.y;
            out->m_max.y = t;
        }
        if (!(out->m_min.z <= out->m_max.z)) {
            float t = out->m_min.z;

            out->m_min.z = out->m_max.z;
            out->m_max.z = t;
        }
    }
    return out;
}

/* a box of one point: a two-point array of it assigned as a CBBox (ours -- the device of CNode()
 * in node_main.cpp; inline, so the pair is a callee's local, below the caller's named box) */
static inline void PointBox(CBBox *box, const CPnt3D &p)
{
    CPnt3D corners[2] = { p, p };

    *box = *(CBBox *)corners;
}

/* the node's box made from its DIs' points and, going up, its children's boxes taken through
 * their matrices -- the first point or box the box itself, each later one widening it; then the
 * box GetBBox gives (SOCOM 1's, the same 0x478). The first point's box through PointBox above; the
 * DIs walked through a reference to the vector (the target's a2 = this + 0x74 for the loop's end),
 * the children's box a named local (the box's field addresses in s0-s2/s6/s7 across the loop).
 * OPEN, 39 instructions (the target's size), registers only: the DI walk's temporaries -- the
 * vector reference, the iterator, the pair's addresses and the point offset (target a2, v1, t0 /
 * v0, a3; here v1, v0, a3 / t0, a2); igreplay finds the target's colouring only with the vector
 * reference and the iterator numbered ahead of `this`. From the draft: 93 off; the vector through
 * a reference 47, the pair an inline's local (below the named box, as the target's frame) 39 */
CBBox *CNode::UpdateBBoxFromDI(bool up)
{
    bool found = false;

    m_bbox.m_min = m_bbox.m_max = CPnt3D::zero;
    CDIVector &dis = m_di;

    for (CDIVector::iterator it = dis.begin(); it != dis.end(); ++it) {
        CDI *di = *it;

        for (int i = 0; i < di->m_flags.m_ptcount; i++) {
            if (!found) {
                found = true;
                PointBox(&m_bbox, *(const CPnt3D *)&di->m_pnts[i]);
            } else {
                const CPnt3D &v = *(const CPnt3D *)&di->m_pnts[i];

                m_bbox.m_min.x = v.x <= m_bbox.m_min.x ? v.x : m_bbox.m_min.x;
                m_bbox.m_min.y = v.y <= m_bbox.m_min.y ? v.y : m_bbox.m_min.y;
                m_bbox.m_min.z = v.z <= m_bbox.m_min.z ? v.z : m_bbox.m_min.z;
                m_bbox.m_max.x = v.x >= m_bbox.m_max.x ? v.x : m_bbox.m_max.x;
                m_bbox.m_max.y = v.y >= m_bbox.m_max.y ? v.y : m_bbox.m_max.y;
                m_bbox.m_max.z = v.z >= m_bbox.m_max.z ? v.z : m_bbox.m_max.z;
            }
        }
    }
    if (up) {
        for (CNodeVector::iterator it = m_child.begin(); it != m_child.end(); ++it) {
            CBBox box = *(*it)->UpdateBBox(true);

            TransformBBox((*it)->GetMatrix(), &box, &box, false);
            if (!found) {
                found = true;
                m_bbox = box;
            } else {
                m_bbox.m_min.x = box.m_min.x <= m_bbox.m_min.x ? box.m_min.x : m_bbox.m_min.x;
                m_bbox.m_min.y = box.m_min.y <= m_bbox.m_min.y ? box.m_min.y : m_bbox.m_min.y;
                m_bbox.m_min.z = box.m_min.z <= m_bbox.m_min.z ? box.m_min.z : m_bbox.m_min.z;
                m_bbox.m_max.x = box.m_max.x >= m_bbox.m_max.x ? box.m_max.x : m_bbox.m_max.x;
                m_bbox.m_max.y = box.m_max.y >= m_bbox.m_max.y ? box.m_max.y : m_bbox.m_max.y;
                m_bbox.m_max.z = box.m_max.z >= m_bbox.m_max.z ? box.m_max.z : m_bbox.m_max.z;
            }
        }
    }
    return (CBBox *)GetBBox();
}

/* a facade's rotation (a billboard): its first three rows turned to face from the viewer's
 * position to its own -- about y only (facade 1), or about y and then x (facade 2: the direction
 * turned into the yz plane to find the x angle).
 * OPEN (0x238 of 0x240): the target keeps &m(2,0) in s2 on both paths and copies the third row
 * out through it -- the address shared with facade 2's stores -- while copying each row as a
 * block (four loads, four stores). A source through the accessor shares the address but
 * copies element by element; through the array it copies by blocks and shares nothing. Round 29:
 * the rows copied out through the accessor (`*(CPnt4D *)&m(r, 0)`) give the size, 0x240, the
 * shared address held in s4 (not s2) and the rows copied element by element (32 off); a row-2
 * pointer local for the stores and the copy is 0x23C */
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

/* the node's model set: a cross-reference's children replaced by the model, an instance's model
 * and model name -- only those two types (SOCOM 1's, the image's first function of the unit; its
 * draft was 0x3178A0's). Defined last, so generated first: ~CModel inlines it, the dead `lw a1,
 * 144(zero)` of its name's ternary left in */
bool CNode::SetModel(CModel *model)
{
    if (m_type != 3 && m_type != 2)
        return false;
    if (m_type == 3) {
        DeleteChildren();
        if (model)
            AddChild(model);
    }
    m_model = model;
    SetModelname(model ? model->m_name : 0);
    return true;
}

}
