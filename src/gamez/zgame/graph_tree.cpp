/* graph_tree.cpp -- CGEdge, and the routes and trees built over the graph: CGraph's BuildRoute,
 * PopulateTree and BuildTree, CGTree and BuildTree's queue (OVL1 0x2D1300-0x2D288F: the template
 * tail from 0x2D1D50, ~CGTreePool at 0x2D2830). SOCOM 1's graph_tree.cpp: its initialiser
 * (0x401C80, after game_engine.cpp's 0x401C10) builds the tree pool's list and registers
 * 0x2D2830, as SOCOM 1's __sinit_graph_tree.cpp builds CGTreePool and registers ~CGTreePool, the
 * last function of its unit there and here. The graph's core, CGNode and CGRoute are the unit
 * before, graph_main.cpp (0x2D04F0-0x2D12FF): every call between the two stays a call (README
 * "The inlining mode, measured"). CGNode::SetName gone: SOCOM II's nodes have no name. Only some
 * of it so far. Defined in reverse of their image order (-inline deferred). */

#include <algorithm>

#include "graph_tree.h"

/* a tree over the graph: a node, its parent and its children (SOCOM 1's CGTree, 0x14 bytes) */
class CGTree : public std::list<CGTree *> {
public:
    CGTree() : m_parent(0), m_node(0) {}
    CGNode *GetNode() const { return m_node; }
    CGTree *FindNode(CGNode *node);
    static void Destroy(CGTree *tree);

    /* the first child: the base's begin() wrapped (FindNode's self-inlined copy calls the base's
     * begin(), 0x2A00B0, then the wrapper's constructor, 0x2D22E0; the name ours) */
    iterator First() { return iterator(list_base::begin()); }

    /* a child let go of (SOCOM 1's RemoveChild, out of line there) */
    void RemoveChild(CGTree *child)
    {
        if (child->m_parent == this) {
            remove(child);
            child->m_parent = 0;
        }
    }
    /* a child taken in, from its old parent first -- the parent asked is the tree's own (sic: both
     * builds load this->m_parent where child->m_parent was meant; a new child has none anyway) */
    void AddChild(CGTree *child)
    {
        if (child->m_parent)
            m_parent->RemoveChild(child);
        push_back(child);
        child->m_parent = this;
    }

    CGTree *m_parent;   // 0xc
    CGNode *m_node;   // 0x10
};

/* a node waiting in BuildTree's queue and its cost (SOCOM 1's CGQueueAtom, 8 bytes: SOCOM 1 links
 * its copy constructor) */
class CGQueueAtom {
public:
    CGQueueAtom(CGNode *node, float weight) : m_node(node), m_weight(weight) {}
    CGNode *GetNode() const { return m_node; }
    /* the same node (the queue's remove compares the value's node with each element's) */
    bool operator==(const CGQueueAtom &x) const { return x.m_node == m_node; }

    CGNode *m_node;   // 0x0
    float m_weight;   // 0x4
};

/* BuildTree's queue: the atoms in cost order (SOCOM 1's CGQueue over list<CGQueueAtom>) */
class CGQueue : public std::list<CGQueueAtom> {
public:
    /* a node queued at the end (OVL1 0x2D1DA0) */
    void push(CGNode *node, float weight) { insert(end(), CGQueueAtom(node, weight)); }
    /* whether a node is queued, and where (OVL1 0x2D1DE0; SOCOM 1's find) */
    bool find(CGNode *node, iterator &it)
    {
        for (iterator i = begin(); i != end(); ++i)
            if ((*i).m_node == node) {
                it = i;
                return true;
            }
        return false;
    }
    bool empty() { return size() == 0; }
    /* the base's insert, the position copied (OVL1 0x2D1D50 out of line) */
    iterator insert(iterator position, const CGQueueAtom &x)
    {
        return std::__list_deleter<CGQueueAtom, std::allocator<CGQueueAtom> >::insert(position, x);
    }
    /* the cheapest node taken off the queue, none when it is empty (BuildTree inlines it one
     * level: every call in it out of line) */
    CGNode *pop()
    {
        if (empty())
            return 0;
        iterator best, i;

        best = i = begin();
        for (i++; i != end(); i++)
            if ((*i).m_weight < (*best).m_weight)
                best = i;
        CGNode *node = (*best).GetNode();

        remove(*best);
        return node;
    }
    /* a node reached at a cost: re-costed if queued dearer, else queued in cost order; true unless
     * it was queued cheaper (BuildTree inlines it one level) */
    bool update(CGNode *node, float cost)
    {
        bool cheaper = true;
        iterator qi;

        if (find(node, qi)) {
            CGQueueAtom &a = *qi;

            cheaper = cost < a.m_weight;
            if (cheaper)
                a.m_weight = cost;
        } else if (empty())
            push(node, cost);
        else {
            iterator k;

            for (k = begin(); k != end(); k++)
                if (cost <= (*k).m_weight)
                    break;
            insert(k, CGQueueAtom(node, cost));
        }
        return cheaper;
    }
};

CGEdge *CGraph::m_treeEdge;
CGNode *CGraph::m_treeNode;
CGTree *CGraph::m_leafTree;

/* the trees' pool (SOCOM 1's CGTreePool and its file static _pool, a 12-byte list): nothing in
 * SOCOM II reads it -- its initialiser __sinit_graph_tree.cpp 0x401C80 builds the list in place
 * and registers ~CGTreePool 0x2D2830, the unit's last function, as SOCOM 1's does */
class CGTreePool : public std::list<CGTree *> {
};

static CGTreePool _pool;

/* a child destroyed, its slot left null (SOCOM 1's _release_tree, std::transform's operation;
 * absent from SOCOM II's image, inlined wherever it is used) */
static CGTree *_release_tree(CGTree *tree)
{
    CGTree::Destroy(tree);
    return 0;
}

/* a tree and every tree under it deleted (OVL1 0x2D22F0: SOCOM 1's Destroy, inlining itself once).
 * OPEN, 0x1EC of 0x1F8 bytes (the states lane, round 16): the outer copy is right (transform, the
 * op, clear() and the four-test delete all inlined), the self-inlined inner copy is one level too
 * shallow everywhere. The target's inner copy calls the BASE list's pieces -- its != 0x2D2220, a
 * prefix ++ 0x2D2240 (FindNode's postfix ++ calls it too, so the base's, not the wrapper's), its *
 * 0x2D2500 and copy constructor 0x2D24F0 (FindNode's wrapper * and copy are 0x2D22C0, 0x2D2260) --
 * builds begin()/end() as SOCOM 1's MSL does, tail() (0x2D2530, `this + 4`) then the iterator's
 * constructor from a node (0x2D2520, begin's; SOCOM 1's FPQ..4node) or from the sentinel (0x2D2510,
 * end's; FPQ..9node_base), inlines the op (Destroy called, `*result = 0`), reads clear()'s
 * sentinel through 0x207560 twice (SOCOM 1's compressed_pair_imp<allocator<node>, node_base, 1>::
 * second(), the first OVL1 copy: CZObjectiveList::FreeList's, outside this lane's range) and inlines
 * ~CGTree and ~list (two tests) before calling ~__list_imp (0x1EA250). Ours calls the wrapper's
 * !=, *, copy and _release_tree itself, and ~CGTree's chain one level higher. A functor op (SOCOM
 * 1's transform takes a function pointer) gives 0x13C: Destroy not inlined at all. Needs the end
 * node as SOCOM 1's compressed pair (include/list, the MSL lane's) or a local model of it */
void CGTree::Destroy(CGTree *tree)
{
    std::transform(tree->begin(), tree->end(), tree->begin(), _release_tree);
    tree->clear();
    delete tree;
}

/* the tree node of a graph node, this or one under it, none (OVL1 0x2D2070: SOCOM 1's FindNode,
 * inlining itself twice) */
CGTree *CGTree::FindNode(CGNode *node)
{
    if (m_node == node)
        return this;
    CGTree *found = 0;

    for (iterator it = First(); !found && it != end(); it++)
        found = (*it)->FindNode(node);
    return found;
}

/* the tree of cheapest routes from a node over the edges a mask lets through (none: all): every
 * node's cost reset, then a queue in cost order drained -- each node taken off it marks its
 * neighbours with itself and their cost, queuing or re-costing them -- and the tree grown from
 * the marks (OVL1 0x2D1790: SOCOM 1's BuildTree, the mask SOCOM II's).
 * OPEN: the target calls ~30 trivial list pieces out of line here (the walks over the graph's
 * lists and the queue's members) while it inlines the edge walk and the queue's own push_back,
 * size and destructor */
CGTree *CGraph::BuildTree(CGNode *start, unsigned int mask)
{
    if (!start)
        return 0;
    Reset();
    CGQueue queue;

    queue.push_back(CGQueueAtom(start, 0.0f));
    while (queue.size()) {
        CGNode *node = queue.pop();

        node->m_cost = -node->m_cost;
        if (node->m_cost == 10000000.0f)
            node->m_cost = 0.0f;
        CGEdgeList &edges = node->m_edges;

        for (CGEdgeList::iterator j = edges.begin(); j != edges.end(); ++j) {
            CGEdge *edge = *j;
            bool back = (edge->m_1c & 0x40000000) ? edge->m_to == node : false;

            if (back)
                continue;
            bool through = mask == 0 || (edge->m_18 & mask);

            if (!through)
                continue;
            CGNode *other = edge->GetOther(node);
            float cost = fabsf(node->m_cost);

            cost += edge->m_4 * edge->m_weight;

            if (other->m_cost < 0.0f && queue.update(other, cost)) {
                other->m_18 = node;
                other->m_cost = -cost;
            }
        }
    }
    queue.erase(queue.begin(), queue.end());
    CGTree *tree = new CGTree;

    tree->m_node = start;
    PopulateTree(tree);
    return tree;
}

/* the tree under a node grown from the marks BuildTree left: every node whose mark is the tree's
 * node becomes a child and is grown in turn, until the node searched for is reached (OVL1
 * 0x2D1640, SOCOM 1's with its second walk gone) */
void CGraph::PopulateTree(CGTree *tree)
{
    CGNodeList::iterator e = m_nodes.end();

    for (CGNodeList::iterator i = m_nodes.begin(); i != e; ++i) {
        CGNode *node = *i;

        if (node->m_18 == tree->m_node) {
            node->m_18 = 0;
            CGTree *child = new CGTree;

            child->m_node = node;
            tree->AddChild(child);
            if (node == m_treeNode) {
                m_leafTree = child;
                return;
            }
            PopulateTree(child);
        }
    }
}

/* the route from one node to another over the edges a mask lets through (none: all), true when
 * it holds a node (OVL1 0x2D14A0: SOCOM 1's BuildRoute, the mask SOCOM II's -- passed on to
 * BuildTree, which tests each edge's word at 0x18 with it; the AI's plan_addleg passes 0, then its
 * own) */
bool CGraph::BuildRoute(CGNode *start, CGNode *end, unsigned int mask, CGRoute *route)
{
    m_treeNode = 0;
    m_treeEdge = 0;
    m_leafTree = 0;
    if (!start || !end || !route)
        return false;
    bool ok = true;

    route->Clear();
    if (start == end)
        route->Append(end);
    else {
        m_treeNode = end;
        CGTree *tree = BuildTree(start, mask);

        if (tree) {
            CGTree *leaf = m_leafTree ? m_leafTree : tree->FindNode(end);
            CGNodeList::iterator it = route->m_nodes.end();

            for (; leaf; leaf = leaf->m_parent)
                it = route->m_nodes.insert(it, leaf->GetNode());
            CGTree::Destroy(tree);
        } else
            ok = false;
    }
    if (ok) {
        route->PostProcess();
        ok = !IsEmpty(route->m_nodes);
    }
    return ok;
}

/* (OVL1 0x2D1470: graph_main.cpp's CreateEdge calls it, as SOCOM 1's does) */
CGEdge::CGEdge(CGNode *from, CGNode *to, float weight)
    : m_weight(weight), m_14(0), m_18(0), m_1c(0)
{
    m_from = from;
    m_to = to;
}

/* SOCOM II's: the flag 0x40000000 of the word at 0x1c set or cleared, and on setting it the two
 * ends replaced when the first differs (OVL1 0x2D1410; OVL2's CAiMaps::CreateGraph 0x52A8C0 calls it twice -- the name ours) */
extern "C" void func_002D1410(CGEdge *edge, bool on, CGNode *from, CGNode *to)
{
    if (on)
        edge->m_1c |= 0x40000000;
    else
        edge->m_1c &= ~0x40000000;
    if (on && edge->m_from != from) {
        edge->m_from = from;
        edge->m_to = to;
    }
}

/* an edge taken out of both its ends' lists and deleted (OVL1 0x2D1350: a copy of the edge's
 * pointer has its address taken for the lists' remove and lives on the stack, every access
 * through it re-read; the delete takes the parameter, kept in a register from the start.
 * graph_main.cpp's DeleteNode calls it, as SOCOM 1's does) */
CGEdge *CGEdge::Delete(CGEdge *edge)
{
    CGEdge *e = edge;

    if (e->m_from)
        e->m_from->m_edges.remove(e);
    if (e->m_to)
        e->m_to->m_edges.remove(e);
    e->m_to = 0;
    e->m_from = 0;
    delete edge;
    return 0;
}
