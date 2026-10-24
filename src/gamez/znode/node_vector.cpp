/* node_vector.cpp -- the scene graph's typed lists: CNodeVector, CDIVector, CVisualVector,
 * CModelVector, CNodeUniverse, CLightMapVector (OVL1 0x311CC0-...), in SOCOM 1's order. The unit
 * name is ours. Defined in reverse of their image order (-inline deferred). */

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

}
