/* graph_main.cpp -- the graph's core: CGraph's nodes and edges, CGNode, and CGRoute (OVL1
 * 0x2D04F0-0x2D12FF, in SOCOM 1's order). Its own unit, linked right before graph_tree.cpp: its
 * CreateEdge and DeleteNode call CGEdge's constructor and CGEdge::Delete out of line, and
 * graph_tree.cpp's BuildRoute calls CGRoute's Clear, Append and PostProcess out of line, while
 * ~CGRoute (0x2D1210, its draft) inlines Clear -- -inline auto inlines inside a unit and never
 * across (README "The inlining mode, measured"; as one unit all five needed pins). No template
 * tail of its own (its pointer lists' instances are an earlier unit's) and no initialiser:
 * graph_tree.cpp's (0x401C80) builds the tree pool. SOCOM 1 links these functions in the same
 * place, but no DWARF or assert names their file there; this unit's name is ours (the module's
 * _main, as game_main.cpp or grid_main.cpp). Defined in reverse of their image order
 * (-inline deferred). */

#include "graph_tree.h"

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
