/* node_vector.cpp -- the scene graph's typed lists: CNodeVector, CDIVector, CVisualVector,
 * CModelVector, CNodeUniverse, CLightMapVector, in SOCOM 1's order -- and the CNode code around
 * them, from TransformToWorld (OVL1 0x310980) on: a unit of its own, as DeleteChild and AddVisual
 * show by calling Exists and Remove (with them in node_main.cpp MW inlines both), and AddDI
 * (0x313D60) by calling the DI list's. It ends at 0x3134BB: SetModeTree's out-of-line pieces
 * (0x313450-0x3134BB) are the cluster of template instances and inline copies that closes a unit
 * (node_main.cpp's own closes it at 0x3176B0), so node_main.cpp begins at 0x3134C0. The unit name
 * is ours. Defined in reverse of their image order (-inline deferred). */

#include <algorithm>
#include <string.h>

#include "znode.h"

namespace zdb {

/* the predicate CNodeVector::GetNode finds by: a node with a name, that name. Eight bytes, built
 * then copied into std::find_if's by-value parameter; its second word (-1) is never read here.
 * The names are ours. */
struct CNodeNamed {
    CNodeNamed(const char *name, int index = -1) : m_name(name), m_index(index) {}
    /* the answer kept in a bool, false unless compared (its andi 0xff is SOCOM 1's too) */
    bool operator()(const CNode *node) const
    {
        bool r = false;

        if (node && node->m_name)
            r = !strcmp(m_name, node->m_name);
        return r;
    }

    const char *m_name;
    int m_index;
};

/* CModelVector::GetModel's predicate: the name, any case, and nothing tested first (the shape of
 * CNodeNamed otherwise; the names, Same's among them, are ours) */
struct CModelNamed {
    CModelNamed(const char *name, int index = -1) : m_name(name), m_index(index) {}
    /* the test through an inline bool function: its `!` is materialised (sltu, xori) and the
     * answer then branched on -- which neither the test written in the if nor a bool local gives */
    static bool Same(const char *a, const char *b) { return !strcasecmp(a, b); }
    bool operator()(const CModel *model) const
    {
        bool r = false;

        if (Same(m_name, model->m_name))
            r = true;
        return r;
    }

    const char *m_name;
    int m_index;
};

/* a node taken in, unless the universe is locked; it grows to 3000 first, then by a hundred */
bool CNodeUniverse::AddNode(CNode *node)
{
    if (!m_locked) {
        if (size() == capacity()) {
            if ((int)size() < 3000)
                reserve(3000);
            else
                reserve(size() + 100);
        }
        push_back(node);
        return true;
    }
    return false;
}

/* a node's slot emptied (the indices of the others kept) */
void CNodeUniverse::RemoveNode(CNode *node)
{
    for (iterator it = begin(); it != end(); ++it) {
        if (*it == node) {
            *it = 0;
            return;
        }
    }
}

/* a node's index, or -1 */
int CNodeUniverse::GetIndex(CNode *node)
{
    int i = 0;

    for (iterator it = begin(); it != end(); ++it, ++i)
        if (*it == node)
            return i;
    return -1;
}

/* the node at an index, or none (an index equal to the size passes the test, as SOCOM 1's) */
CNode *CNodeUniverse::GetElement(int i)
{
    if (i >= 0 && i <= size())
        return at(i);
    return 0;
}

/* a model by name, or none */
CModel *CModelVector::GetModel(const char *name)
{
    if (!name)
        return 0;
    iterator it = std::find_if(begin(), end(), CModelNamed(name));

    return it == end() ? 0 : *it;
}

/* whether a light map is on the list */
bool CLightMapVector::Exists(const CLightMap *light)
{
    iterator it = std::find(begin(), end(), light);

    return it != end();
}

/* whether a visual is on the list. The found iterator is a local of its own: compared straight
 * off std::find, the temporaries' stores schedule apart from their copies */
bool CVisualVector::Exists(const CVisual *visual)
{
    iterator it = std::find(begin(), end(), visual);

    return it != end();
}

/* whether a DI is on the list */
bool CDIVector::Exists(const CDI *di)
{
    iterator it = std::find(begin(), end(), di);

    return it != end();
}

/* the element at an index of some list of nodes, or none (new in SOCOM II; its callers' lists are
 * not placed, so it keeps its placeholder) */
extern "C" CNode *func_00311EA0(CNodeVector *v, int i)
{
    if (v->size() && i >= 0 && i < v->size())
        return v->at(i);
    return 0;
}

/* a node by name, or none */
CNode *CNodeVector::GetNode(const char *name)
{
    if (!name)
        return 0;
    iterator it = std::find_if(begin(), end(), CNodeNamed(name));

    return it == end() ? 0 : *it;
}

/* whether a node is on the list */
bool CNodeVector::Exists(const CNode *node)
{
    iterator it = std::find(begin(), end(), node);

    return it != end();
}

/* a node taken off the list, every time it is on it (std::remove, then the tail erased): whether
 * it was.
 * OPEN (0x134 of 0x14C): find inlines whole inside the inlined remove (its loop tests == -- with
 * the iterator's !=, which calls ==, it would be one level too tall in remove), but the target
 * keeps two things MW drops here: the dead copy of begin() into remove's own `first` (assigned
 * from find before it is read), and a separate return slot for remove_copy, both returns
 * handing back the iterator's address and `it` copied from it after the join (here `it` is
 * remove_copy's return slot and the other return stores to it directly). Tried: direct-init,
 * an assigned default-constructed `it` (remove a level deeper: find one level), end() for e. */
bool CNodeVector::Remove(const CNode *node)
{
    iterator e = end();
    iterator it = std::remove(begin(), e, node);

    erase(it, e);
    return it != e;
}

/* the node's mode (SOCOM II's two bits) set through the whole tree: SetDynamic's walk, over the
 * children through a reference (the recursion inlines itself twice; its out-of-line pieces end
 * the unit, in reverse of their first use here -- the iterator's copy constructor, postfix ++,
 * assignment and default constructor, then SetMode's copy). The name is ours */
void CNode::SetModeTree(int mode)
{
    if (!this)
        return;
    SetMode(mode);
    CNodeVector::iterator it;
    CNodeVector &children = GetChildren();

    for (it = children.begin(); it != children.end(); it++)
        (*it)->SetModeTree(mode);
}

/* the same for collision data (DIs): the twin of SetDescendantVisuals */
bool CNode::SetDescendantDI(bool set)
{
    if (!this)
        return false;
    CNodeVector &children = m_child;

    if (set == false) {
        m_hasDI = 0;
        for (CNodeVector::iterator it = children.begin(); it != children.end(); ++it)
            (*it)->SetDescendantDI(false);
        return false;
    }
    bool any = false;
    bool own = m_di.size() != 0;

    for (CNodeVector::iterator it = children.begin(); it != children.end(); ++it)
        any |= (*it)->SetDescendantDI(true);
    m_hasDI = own || any;
    return own || any;
}

/* whether the node or anything below it has visuals, marked on each node on the way back up; or
 * the mark cleared through the whole tree (SOCOM 1's, the answers ORed as bools now; the flag is
 * compared with false, which masks it to a byte first) */
bool CNode::SetDescendantVisuals(bool set)
{
    if (!this)
        return false;
    CNodeVector &children = m_child;

    if (set == false) {
        m_hasVisuals = 0;
        for (CNodeVector::iterator it = children.begin(); it != children.end(); ++it)
            (*it)->SetDescendantVisuals(false);
        return false;
    }
    bool own = m_visual.size() != 0;
    bool any = false;

    for (CNodeVector::iterator it = children.begin(); it != children.end(); ++it)
        any |= (*it)->SetDescendantVisuals(true);
    m_hasVisuals = own || any;
    return own || any;
}

/* a light switched on or off, the old setting given back (the node's own, word for word) */
bool CLight::SetActive(bool on)
{
    bool was = m_active;

    m_active = on;
    return was;
}

/* the regions (a bit for the node's own) and the lights handed down the tree: a light keeps the
 * lights, anything else the regions, a type-10 node neither and nothing below it. Over a copy
 * of the child list, its size read once; how many lights below took some.
 * OPEN (4 differing): the mask and the constant 1 take each other's registers (a1, v1) -- not
 * moved by the statements' order, either spelling of the ORs, locals, inline accessors, or the
 * permuter's 21,048 candidates */
int CNode::InheritRegionMasks(unsigned int regions, unsigned int lights)
{
    if (m_type == 10)
        return 0;
    regions |= 1 << m_region_shift;
    lights |= m_region_mask;
    if (m_type == 8)
        m_region_mask = lights;
    else
        m_region_mask = regions;
    CNodeVector children(m_child);
    unsigned int n = children.size();
    int count = 0;

    for (unsigned int i = 0; i < n; i++)
        count += children[i]->InheritRegionMasks(regions, lights);
    if (m_type == 8 && lights)
        count++;
    return count;
}

/* whether a segment surely misses the node's bounds (both ends past one face); a node that
 * borrows its parent's bounds never says so. One expression: the six tests flat in one ||
 * chain, MW keeping a flag per level (new in SOCOM II; its callers, the grid's intersection
 * code, are not placed, so it keeps its placeholder) */
extern "C" bool func_003124F0(CNode *node, const CPnt3D *a, const CPnt3D *b)
{
    return !node->m_use_parent_bbox &&
           ((a->x < node->m_bbox.m_min.x && b->x < node->m_bbox.m_min.x) ||
           (a->x > node->m_bbox.m_max.x && b->x > node->m_bbox.m_max.x) ||
           (a->y < node->m_bbox.m_min.y && b->y < node->m_bbox.m_min.y) ||
           (a->y > node->m_bbox.m_max.y && b->y > node->m_bbox.m_max.y) ||
           (a->z < node->m_bbox.m_min.z && b->z < node->m_bbox.m_min.z) ||
           (a->z > node->m_bbox.m_max.z && b->z > node->m_bbox.m_max.z));
}

/* the name of the model the node is an instance of: its own copy, the old one freed */
void CNode::SetModelname(const char *name)
{
    if (m_modelname) {
        _free(m_modelname, 0, 0);
        m_modelname = 0;
    }
    m_modelname = name ? _strdup(name, 0, 0) : 0;
}

/* the node-to-world matrix: this node's, then each parent's up to the world (type 6, whose own
 * is the unit), multiplied in; none if the chain never reaches it. The recursion inlines itself
 * three times; the innermost copies call CMatrix's pieces out of line.
 * OPEN (right size, 44 differing): the third copy converts Multiply's arguments right to left
 * (this, b, then out: 0x310C60, 0x310C60, 0x310C50), where MW converts call arguments left to
 * right (probe build/scratch/inl/t10.cpp) -- so out first here, in every form tried (explicit
 * casts, implicit conversions to row pointers under a const prototype, a local) */
CMatrix *CNode::BuildMTW(CMatrix &m)
{
    if (m_type == 6) {
        m.SetIdentity();
        return &m;
    }
    m.Multiply(GetMatrix(), &m);
    if (m_parent && m_parent->m_type == 6)
        return &m;
    return m_parent ? m_parent->BuildMTW(m) : 0;
}

/* a point in the node's frame into the world's: each matrix up the tree, to the world's node
 * (type 6) */
void CNode::TransformToWorld(CPnt3D &p) const
{
    for (const CNode *n = this; n && n->m_type != 6; n = n->m_parent)
        n->m_matrix.Transform(&p, 1);
}

}
