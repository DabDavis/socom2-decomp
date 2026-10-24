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

/* a node's visuals let go of (not named yet) */
extern "C" void func_00313620(CNode *node, int a);
/* the world told of a shadow-casting node: a new 0x70-byte object made from it, pushed on a list
 * at 0x580 (new in SOCOM II, not named yet; CWorld::AddChild calls it too) */
extern "C" void func_0031F500(CWorld *world, CNode *node);

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

/* an extension attached to the node and, when told to, to every node below it. The recursion
 * inlines itself twice, and the copies call the child vector's pieces out of line (begin, end,
 * the iterator's ==, !=, *, ++ and copy) where MW's inline budget runs out -- the walk's ++ is
 * the postfix one, its copy and the prefix ++ called in the second copy (include/vector) */
void CNode::SetNodeEx(CNodeEx *ex, bool recurse)
{
    m_nodeEx = ex;
    if (recurse) {
        for (CNodeVector::iterator it = m_child.begin(); it != m_child.end(); it++)
            (*it)->SetNodeEx(ex, recurse);
    }
}

/* a child by name (its length first, then its text), among the children and then, when deep,
 * below each. The second walk's iterator is its own, initialised from begin(): the address of
 * the list's storage it reloads begin through is computed once, before the first walk (an
 * iterator assigned from begin() leaves data() a call) */
CNode *CNode::FindChild(const char *name, bool deep)
{
    if (!name)
        return 0;
    CNode *found = 0;
    size_t len = strlen(name);
    CNodeVector::iterator it = m_child.begin();

    for (; !found && it != m_child.end(); ++it) {
        CNode *child = *it;

        if (strlen(child->m_name) == len && !strcmp(child->m_name, name))
            found = child;
    }
    if (!found && deep) {
        for (CNodeVector::iterator i2 = m_child.begin(); !found && i2 != m_child.end(); ++i2)
            found = (*i2)->FindChild(name, deep);
    }
    return found;
}

/* a node among the children and then, when deep, below each. The first walk's `!found` is always
 * true (a hit returns) and folds away, but not before MW has laid the loop out as a compound
 * test's (a branch to the test at the bottom, where a lone compare is tested once on entry too) */
CNode *CNode::FindChild(CNode *node, bool deep)
{
    if (!node)
        return 0;
    CNode *found = 0;
    CNodeVector::iterator it = m_child.begin();

    for (; !found && it != m_child.end(); ++it)
        if (*it == node)
            return *it;
    if (!found && deep) {
        for (CNodeVector::iterator i2 = m_child.begin(); !found && i2 != m_child.end(); ++i2)
            found = (*i2)->FindChild(node, deep);
    }
    return found;
}

/* a child by name from the given index on (a negative one counting as 0), and where it stood: the
 * index after it. Then, when deep, the same search below every child, all of them -- the walk
 * does not stop at a hit, and what it returns is the last child's answer. */
CNode *CNode::FindChild(const char *name, int start, int *index, bool deep)
{
    CNode *found = 0;

    if (name) {
        CNode *child;
        unsigned int n = m_child.size();
        unsigned int i = start > 0 ? start : 0;
        size_t len = strlen(name);

        for (; !found && i < n; i++) {
            child = m_child[i];

            if (strlen(child->m_name) == len && !strcmp(child->m_name, name)) {
                found = child;
                *index = i + 1;
            }
        }
        if (!found && deep) {
            for (i = 0; i < n; i++)
                found = m_child[i]->FindChild(name, start, index, deep);
        }
    }
    return found;
}

/* a child taken in: a shadow caster told to the world, a character's common flag copied into a
 * cell, the child taken from its old parent and counted (not by a type-3 node), and put on the
 * list -- which grows by a hundred at a time once past a thousand. A child with visuals marks
 * the node as having them. */
void CNode::AddChild(CNode *child)
{
    if (!child)
        return;
    if (child->m_shadow == 1)
        func_0031F500(CWorld::m_world, child);
    if (child->GetType() == 7 && GetType() == 3)
        m_char_common = child->m_char_common != 0;
    if (child->m_parent == this)
        return;
    if (m_type != 3) {
        if (child->m_parent)
            child->m_parent->DeleteChild(child);
        child->m_parent = this;
        child->m_count++;
    }
    int n = m_child.size();
    if (n == m_child.capacity() && n > 1000)
        m_child.reserve(n + 100);
    m_child.push_back(child);
    if (child->m_hasVisuals)
        m_hasVisuals = 1;
    m_socom2_a1 |= 2;
}

/* a child let go of: its reference given back (a type-3 node's children are not counted) and
 * the child out of the list; its count after, or -1 */
int CNode::DeleteChild(CNode *child)
{
    int count = child->m_count;

    if (m_child.Exists(child)) {
        func_00313620(child, 0);
        if (m_type == 3) {
            m_child.Remove(child);
            m_socom2_a1 |= 2;
            count = -1;
        } else {
            count = child->Release();
            m_child.Remove(child);
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
