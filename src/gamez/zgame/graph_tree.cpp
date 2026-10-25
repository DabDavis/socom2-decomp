/* graph_tree.cpp -- a graph of nodes and weighted edges, the routes through it and the trees built
 * over it: CGraph, CGNode, CGEdge, CGRoute (SOCOM 1's graph_tree.cpp, its static initialiser after
 * game_state.cpp's; the directory is ours). OVL1 0x2D04F0 to BuildTree's end, in SOCOM 1's order
 * (CGNode::SetName gone: SOCOM II's nodes have no name). Every list is a pointer list, so every
 * call into one is __list_deleter<const void *>'s. Only some of it so far. Defined in reverse of
 * their image order (-inline deferred). */

#include <list>

void _free(void *p, const char *file, int line);
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

    int m_4;   // 0x4
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

    int m_4;   // 0x4
    const void *m_nodeEx;   // 0x8
    CGEdgeList m_edges;   // 0xc
    int m_18;   // 0x18
    int m_1c;   // 0x1c
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

    void SetName(const char *name)
    {
        if (m_name)
            _free(m_name, 0, 0);
        m_name = 0;
        m_name = name ? _strdup(name, 0, 0) : 0;
    }

    char *m_name;   // 0x0
    CGNodeList m_nodes;   // 0x4
    CGEdgeList m_edges;   // 0x10
};   // 0x1c bytes

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

CGNode::CGNode()
    : m_4(0), m_nodeEx(0), m_18(0), m_1c(0)
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
