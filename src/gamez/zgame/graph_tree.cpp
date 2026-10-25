/* graph_tree.cpp -- a graph of nodes and weighted edges, the routes through it and the trees built
 * over it: CGraph, CGNode, CGEdge, CGRoute (SOCOM 1's graph_tree.cpp, its static initialiser after
 * game_state.cpp's; the directory is ours). OVL1 0x2D04F0 to BuildTree's end, in SOCOM 1's order
 * (CGNode::SetName gone: SOCOM II's nodes have no name). Every list is a pointer list, so every
 * call into one is __list_deleter<const void *>'s. Only some of it so far. Defined in reverse of
 * their image order (-inline deferred). */

#include <list>

void _free(void *p, const char *file, int line);
extern "C" float fabsf(float x);
char *_strdup(const char *s, const char *file, int line);

class CGNode;
class CGEdge;

typedef std::list<CGNode *> CGNodeList;
typedef std::list<CGEdge *> CGEdgeList;

/* the pointer lists' own base: a walk on its iterator reads each element by value, afresh after
 * every call (DeleteNode), where the wrapper's reference keeps the element's address */
typedef std::__list_deleter<const void *, std::allocator<const void *> > list_base;

/* an edge: its two nodes and its weight (SOCOM 1's, 0x18 bytes; SOCOM II's 0x20) */
class CGEdge {
public:
    CGEdge(CGNode *from, CGNode *to, float weight);
    virtual ~CGEdge() {}
    static CGEdge *Delete(CGEdge *edge);

    /* whether n is one of its ends (the name ours) */
    bool Has(const CGNode *n) const { return m_from == n || m_to == n; }
    /* whether it joins a and b (the name ours: the && materialised, as an inline bool's) */
    bool Joins(const CGNode *a, const CGNode *b) const { return Has(a) && Has(b); }

    /* the node at the other end from n, or none when n is not one of its ends (the name ours) */
    CGNode *GetOther(const CGNode *n) const
    {
        if (m_from == n)
            return m_to;
        if (m_to == n)
            return m_from;
        return 0;
    }

    /* the flags word's bits cleared and set (OVL1 0x2D1F60 out of line: BuildTree clears bit 31 of
     * every edge with it; the name ours) */
    void SetFlags(unsigned int set, unsigned int clear)
    {
        m_1c &= ~clear;
        m_1c |= set;
    }

    float m_4;   // 0x4: a length (BuildTree's step costs it times the weight)
    float m_weight;   // 0x8
    CGNode *m_from;   // 0xc
    CGNode *m_to;   // 0x10
    int m_14;   // 0x14
    int m_18;   // 0x18
    unsigned int m_1c;   // 0x1c
};

/* a node: the object it stands for and its edges (SOCOM 1's had a name) */
class CGNode {
public:
    CGNode();
    virtual ~CGNode();
    CGEdge *FindEdge(CGNode *other);
    /* unreached: no parent, the cost a large negative (OVL1 0x2D2010 out of line: BuildTree passes
     * -10000000; the name ours) */
    void Reset(int cost)
    {
        m_18 = 0;
        m_cost = cost;
    }

    int m_4;   // 0x4
    const void *m_nodeEx;   // 0x8
    CGEdgeList m_edges;   // 0xc
    CGNode *m_18;   // 0x18: the node a tree search reached it from
    float m_cost;   // 0x1c: BuildTree's cost to it, negative until it is taken off the queue
};

/* a tree over the graph: a node, its parent and its children (SOCOM 1's CGTree, 0x14 bytes) */
class CGTree : public std::list<CGTree *> {
public:
    CGTree() : m_parent(0), m_node(0) {}
    CGNode *GetNode() const { return m_node; }

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

extern "C" {
CGTree *func_002D2070(CGTree *tree, CGNode *node);   /* CGTree::FindNode */
void func_002D22F0(CGTree *tree);                     /* CGTree::Destroy */
}

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

/* a route: the nodes in order */
class CGRoute {
public:
    CGRoute();
    ~CGRoute();
    void PostProcess();
    void Clear();
    void Append(CGNode *node);

    CGNodeList m_nodes;   // 0x0
};

class CGraph {
public:
    CGraph(const char *name);
    ~CGraph();
    static CGraph *Create(const char *name);
    static void Destroy(CGraph *graph);
    CGNode *CreateNode(const char *name);
    void DeleteNode(CGNode *node);
    CGEdge *CreateEdge(CGNode *a, CGNode *b);
    CGNode *GetNodeByIndex(unsigned int index);
    CGNode *GetNodeByNodeEx(const void *ex);
    CGEdge *FindEdge(CGNode *a, CGNode *b);
    bool BuildRoute(CGNode *start, CGNode *end, unsigned int mask, CGRoute *route);
    void PopulateTree(CGTree *tree);
    CGTree *BuildTree(CGNode *start, unsigned int mask);

    /* the search's state (SOCOM 1's three statics, in its order) */
    static CGEdge *m_treeEdge;
    static CGNode *m_treeNode;   // the node searched for
    static CGTree *m_leafTree;   // and its tree node, once PopulateTree reaches it

    void SetName(const char *name)
    {
        if (m_name)
            _free(m_name, 0, 0);
        m_name = 0;
        m_name = name ? _strdup(name, 0, 0) : 0;
    }
    /* every node unreached and every edge's bit 31 cleared (BuildTree inlines it one level) */
    void Reset()
    {
        CGNodeList::iterator it;

        for (it = m_nodes.begin(); it != m_nodes.end(); it++)
            (*it)->Reset(-10000000);
        CGEdgeList::iterator eit;

        for (eit = m_edges.begin(); eit != m_edges.end(); eit++)
            (*eit)->SetFlags(0, 0x80000000);
    }

    char *m_name;   // 0x0
    CGNodeList m_nodes;   // 0x4
    CGEdgeList m_edges;   // 0x10
};   // 0x1c bytes

/* whether a list is empty (MSL's empty(), which include/list lacks: the test materialised) */
inline bool IsEmpty(const CGNodeList &l)
{
    return l.size() == 0;
}

CGEdge *CGraph::m_treeEdge;
CGNode *CGraph::m_treeNode;
CGTree *CGraph::m_leafTree;

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
            CGTree *leaf = m_leafTree ? m_leafTree : func_002D2070(tree, end);
            CGNodeList::iterator it = route->m_nodes.end();

            for (; leaf; leaf = leaf->m_parent)
                it = route->m_nodes.insert(it, leaf->GetNode());
            func_002D22F0(tree);
        } else
            ok = false;
    }
    if (ok) {
        route->PostProcess();
        ok = !IsEmpty(route->m_nodes);
    }
    return ok;
}

/* (OVL1 0x2D1470: called, never inlined -- CreateEdge calls it, as SOCOM 1's does) */
#pragma auto_inline off
CGEdge::CGEdge(CGNode *from, CGNode *to, float weight)
    : m_weight(weight), m_14(0), m_18(0), m_1c(0)
{
    m_from = from;
    m_to = to;
}
#pragma auto_inline reset

/* SOCOM II's: the flag 0x40000000 of the word at 0x1c set or cleared, and on setting it the two
 * ends replaced when the first differs (OVL1 0x2D1410; no caller found -- the name ours) */
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
 * through it re-read; the delete takes the parameter, kept in a register from the start. Pinned:
 * DeleteNode calls it, as SOCOM 1's does) */
#pragma auto_inline off
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
#pragma auto_inline reset

CGRoute::CGRoute()
{
}

/* ~CGRoute (OVL1 0x2D1210) is Clear() and the list's implicit destructor -- left to its draft
 * (src/auto, byte-exact): written here it comes out 0x14 short for the reason ~CGNode does */

/* CGRoute's three are pinned: BuildRoute calls each of them, as SOCOM 1's does */
#pragma auto_inline off
/* a node added at the end (OVL1 0x2D11D0) */
void CGRoute::Append(CGNode *node)
{
    m_nodes.push_back(node);
}

/* every node forgotten (OVL1 0x2D1190, as SOCOM 1's) */
void CGRoute::Clear()
{
    m_nodes.clear();
}

/* nothing (OVL1 0x2D1180, as SOCOM 1's) */
void CGRoute::PostProcess()
{
}
#pragma auto_inline reset

CGNode::CGNode()
    : m_4(0), m_nodeEx(0), m_18(0), m_cost(0)
{
}

/* the edges forgotten (OVL1 0x2D1060).
 * OPEN, 0xBC against 0xD0: the list's implicit destructor after the clear tests `this` four times
 * in the target (and in ~CGRoute's), three here -- the pointer list's destructor chain is a level
 * taller in SOCOM II's MSL than include/list's (list<T *>, __list_imp, __list_deleter) */
CGNode::~CGNode()
{
    m_edges.clear();
}

/* the node's edge to another (OVL1 0x2D0F90) */
CGEdge *CGNode::FindEdge(CGNode *other)
{
    if (!other)
        return 0;
    CGEdgeList::iterator i;

    for (i = m_edges.begin(); i != m_edges.end(); ++i)
        if ((*i)->Joins(this, other))
            return *i;
    return 0;
}

/* a graph made (OVL1 0x2D0C00) */
CGraph *CGraph::Create(const char *name)
{
    return new CGraph(name);
}

/* a graph deleted (OVL1 0x2D0BF0) */
void CGraph::Destroy(CGraph *graph)
{
    delete graph;
}

/* (OVL1 0x2D0B70) */
CGraph::CGraph(const char *name)
    : m_name(0)
{
    SetName(name);
}

/* a node deleted: each of its edges taken out of the other end's list and the graph's and
 * deleted, then the node itself (OVL1 0x2D0780).
 * OPEN, 0x1AC against 0x1A8: the target re-reads the iterator from its stack slot for the second
 * element read (after the first remove) and inverts the loop (its test ahead of the body as well
 * as after it); here MW keeps the element's address in s0 across the call and enters the loop
 * through a branch to its test. Tried: the wrapper's `*` for either read, the base iterator's
 * (the walker then lives in a register), the base iterator's `*` through the conversion or
 * through i_ */
void CGraph::DeleteNode(CGNode *node)
{
    if (node) {
        CGEdgeList::iterator i;

        for (i = node->m_edges.begin(); i != node->m_edges.end(); ++i) {
            CGEdge *edge = (CGEdge *)*(list_base::iterator)i;

            edge->GetOther(node)->m_edges.remove(edge);
            edge->m_from = 0;
            edge->m_to = 0;
            CGEdge *e = (CGEdge *)*(list_base::iterator)i;

            m_edges.remove(e);
            CGEdge::Delete(e);
        }
        node->m_edges.clear();
    }
    m_nodes.remove(node);
    delete node;
}

/* the edge made, both ends taking it in and so does the graph; the one there is if it exists
 * (OVL1 0x2D0680) */
CGEdge *CGraph::CreateEdge(CGNode *a, CGNode *b)
{
    if (!a || !b || a == b)
        return 0;
    CGEdge *edge = a->FindEdge(b);

    if (!edge) {
        edge = new CGEdge(a, b, 1.0f);
        a->m_edges.push_back(edge);
        b->m_edges.push_back(edge);
        m_edges.push_back(edge);
    }
    return edge;
}

/* the index'th node, none past the end (OVL1 0x2D0620) */
CGNode *CGraph::GetNodeByIndex(unsigned int index)
{
    CGNodeList::iterator i = m_nodes.begin();

    while (index-- && i != m_nodes.end())
        ++i;
    return i == m_nodes.end() ? 0 : *i;
}

/* the node standing for an object (OVL1 0x2D05A0) */
CGNode *CGraph::GetNodeByNodeEx(const void *ex)
{
    CGNodeList::iterator i;

    for (i = m_nodes.begin(); i != m_nodes.end(); ++i)
        if ((*i)->m_nodeEx == ex)
            return *i;
    return 0;
}

/* the edge between two nodes (OVL1 0x2D04F0) */
CGEdge *CGraph::FindEdge(CGNode *a, CGNode *b)
{
    CGEdgeList::iterator i;

    for (i = m_edges.begin(); i != m_edges.end(); ++i)
        if ((*i)->GetOther(a) == b)
            return *i;
    return 0;
}
