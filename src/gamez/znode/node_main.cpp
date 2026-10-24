/* node_main.cpp -- zdb::CNode, the scene graph's node: its place in the world, its bounds, its
 * children, its grid atoms (OVL1 0x310980-...). The unit name is SOCOM 1's (CNode::SetName's
 * allocations name it). Defined in reverse of their image order (-inline deferred). */

#include <string.h>

#include "znode.h"
#include "../zanim/zanim.h"
#include "../zsys/zsys.h"

/* the name a node without one shares (SOCOM 1's file static, 0x3DFD10 here) */
extern char *UNNAMED_NODE;

namespace zdb {

/* the child list's search and removal (CNodeVector's, not named yet) */
extern "C" bool func_00311E10(CNodeVector *v, CNode *node);
extern "C" void func_00311CC0(CNodeVector *v, CNode *node);
/* a node's visuals let go of (not named yet) */
extern "C" void func_00313620(CNode *node, int a);

/* a position set: the translation, the grid told, the matrix marked changed */
void CNode::SetPosition(float x, float y, float z)
{
    m_matrix.m_matrix[3][0] = x;
    m_matrix.m_matrix[3][1] = y;
    m_matrix.m_matrix[3][2] = z;
    UpdateGrid();
    m_socom2_a1 |= 2;
}

/* the squared radius of the bounds about the node's origin: the further corner's */
float CNode::GetRadiusSq()
{
    float a = m_bbox.m_min.x * m_bbox.m_min.x + m_bbox.m_min.y * m_bbox.m_min.y + m_bbox.m_min.z * m_bbox.m_min.z;
    float b = m_bbox.m_max.x * m_bbox.m_max.x + m_bbox.m_max.y * m_bbox.m_max.y + m_bbox.m_max.z * m_bbox.m_max.z;

    return MAX(a, b);
}

float CNode::GetRadius()
{
    return sqrtf(GetRadiusSq());
}

/* the first visual as a sub-mesh, if it is one */
CSubMesh *CNode::GetSubMesh()
{
    CVisual *v = GetFirstVisual();

    if (v && v->m_type == 2)
        return dynamic_cast<CSubMesh *>(v);
    return 0;
}

/* the node's bounds made current; the node's own are, a borrowed parent's are fetched (the base
 * class's virtual) */
CBBox *CNode::UpdateBBox(bool up)
{
    return (CBBox *)GetBBox();
}

/* the first of the node's visuals, if it has one */
CVisual *CNode::GetFirstVisual()
{
    CVisualVector &v = m_visual;

    return v.empty() ? 0 : v[0];
}

/* a reference given back; never below none */
int CNode::Release()
{
    m_count--;
    if (m_count < 0)
        m_count = 0;
    return m_count;
}

/* the node's name, its own copy (or the shared default when given none); a name of its own freed */
void CNode::SetName(const char *name)
{
    if (m_name && m_name != UNNAMED_NODE)
        func_00180280(m_name, 0, 0);
    m_name = name ? func_001801E0(name, 0, 0) : UNNAMED_NODE;
}

/* a child let go of: its reference given back (a type-3 node's children are not counted) and
 * the child out of the list; its count after, or -1 */
int CNode::DeleteChild(CNode *child)
{
    int count = child->m_count;

    if (func_00311E10(&m_child, child)) {
        func_00313620(child, 0);
        if (m_type == 3) {
            func_00311CC0(&m_child, child);
            m_socom2_a1 |= 2;
            count = -1;
        } else {
            count = child->Release();
            func_00311CC0(&m_child, child);
            child->m_parent = 0;
            m_socom2_a1 |= 2;
        }
    }
    return count;
}

/* every child let go of, each deleted that nothing else holds */
void CNode::DeleteChildren()
{
    while (m_child.size()) {
        CNode *child = m_child[0];

        if (!DeleteChild(child))
            delete child;
    }
}

/* the atom pointers sized for a grid's cells, x by z (one more each way for a dynamic, lit
 * node) */
void CNode::AllocateAtomBasePtrs(int x, int z)
{
    int n = m_dynamic_motion && m_dynamic_light ? (x + 1) * (z + 1) : x * z;
    if (!m_Atom) {
        m_AtomAlloc = n;
        m_Atom = (CGridAtom **)_malloc(m_AtomAlloc * 4, 0, 0);
    } else if (m_AtomAlloc < n) {
        m_AtomAlloc = n;
        m_Atom = (CGridAtom **)_realloc(m_Atom, n * 4, 0, 0);
    }
}

/* the atom pointers cut to those in use */
void CNode::ShrinkAtomBasePtrs()
{
    if (m_AtomCnt < m_AtomAlloc) {
        CGridAtom **atoms = (CGridAtom **)_malloc(m_AtomCnt * 4, 0, 0);

        memcpy(atoms, m_Atom, m_AtomCnt * 4);
        func_00180280(m_Atom, 0, 0);
        m_Atom = atoms;
        m_AtomAlloc = m_AtomCnt;
    }
}

/* the grid atoms the node is in: all freed (the pointer zeroed twice over), one added, the n-th */
void CNode::FreeAtom()
{
    if (m_Atom)
        func_00180280(m_Atom, 0, 0);
    m_Atom = 0;
    m_Atom = 0;
    m_AtomAlloc = 0;
}

void CNode::InsertAtom(CGridAtom *atom)
{
    m_Atom[m_AtomCnt++] = atom;
}

CGridAtom *CNode::GetAtom(int i)
{
    if (i >= m_AtomAlloc)
        return 0;
    return m_Atom[i];
}

/* a point in the node's frame into the world's: each matrix up the tree, to the world's node
 * (type 6) */
void CNode::TransformToWorld(CPnt3D &p) const
{
    for (const CNode *n = this; n && n->m_type != 6; n = n->m_parent)
        n->m_matrix.Transform(&p, 1);
}

}
