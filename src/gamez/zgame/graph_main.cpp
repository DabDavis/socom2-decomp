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

#include <algorithm>

#include "../zutil/zutil.h"
#include "graph_tree.h"

CGRoute::CGRoute()
{
}

/* every node forgotten, then the list's own destructor (OVL1 0x2D1210: Clear inlined) */
CGRoute::~CGRoute()
{
    Clear();
}

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

/* (OVL1 0x2D1130: pinned -- CreateNode calls it out of line, as SOCOM 1's CreateNode calls its
 * constructor) */
#pragma auto_inline off
CGNode::CGNode()
    : m_4(0), m_nodeEx(0), m_18(0), m_cost(0)
{
}
#pragma auto_inline reset

/* the edges forgotten (OVL1 0x2D1060: the list's implicit destructor after the clear tests `this`
 * four times, CGEdgeList being a class of its own) */
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

/* a graph file's block header (SOCOM 1's shape; the names ours) */
struct GraphHeader {
    unsigned int version;   // 0x0: the reader's (data 0x3F3500, 1)
    int size;   // 0x4: the block's size, this header included (a wrong version skips past it)
    unsigned int nodes;   // 0x8
    unsigned int edges;   // 0xc
};

/* an edge as the file holds it: its two nodes' indices, its length and its fixup's word */
struct GraphEdgeRecord {
    unsigned int from;   // 0x0
    unsigned int to;   // 0x4
    float length;   // 0x8
    unsigned int data;   // 0xc
};

extern "C" const unsigned int data_003F3500;   /* (the version the reader takes) */

/* the nodes and edges read from a file block, each handed to its fixup with the file's word for
 * it; false for another version (its block skipped) or an empty graph (OVL1 0x2D0C90, SOCOM 1's;
 * CreateNode and GetNodeByIndex are its out-of-line calls, hence their pins).
 * The edge loop's registers (the counter s3, the record cursor s4, the first node s5) take BOTH of
 * two shapes, each alone 5 or 11 off (the states' round 37, read off MW's colouring): one counter for
 * both loops -- the edge loop's is then a second web, numbered after the loop optimiser's cursor --
 * and the first node a named local, numbered below that cursor */
bool CGraph::Read(CFileIO &f, NodeFixup node, EdgeFixup edge, void *data)
{
    unsigned int i;
    GraphHeader hdr = {0};

    f.fread(&hdr, sizeof(hdr));
    if (hdr.version != data_003F3500) {
        f.fseek(1, hdr.size - 16);
        return false;
    }
    if (!hdr.nodes || !hdr.edges)
        return false;
    unsigned int *nodes = new unsigned int[hdr.nodes];
    GraphEdgeRecord *edges = new GraphEdgeRecord[hdr.edges];

    f.fread(nodes, hdr.nodes * 4);
    f.fread(edges, hdr.edges * 16);
    for (i = 0; i < hdr.nodes; i++) {
        CGNode *n = CreateNode();

        if (node)
            node(this, n, true, nodes[i], data);
    }
    for (i = 0; i < hdr.edges; i++) {
        CGNode *from = GetNodeByIndex(edges[i].from);
        CGEdge *e = CreateEdge(from, GetNodeByIndex(edges[i].to));

        e->m_4 = edges[i].length;
        if (edge)
            edge(this, e, true, edges[i].data, data);
    }
    delete[] nodes;
    delete[] edges;
    return true;
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

/* an object deleted, its slot left null (std::transform's operation; SOCOM 1 links no function
 * for it, so it was inlined there too; the name ours). An inline function passed by pointer: a
 * functor passed by value leaves its empty temporary's `sb zero` store, and MW 3.0 takes a
 * function template instance's address only through an explicit cast */
template <class T>
inline T *DeleteObject(T *p)
{
    delete p;
    return 0;
}

/* the name freed, every node and edge deleted and the lists emptied (OVL1 0x2D0990: SOCOM 1's,
 * the free's file and line gone) */
CGraph::~CGraph()
{
    if (m_name)
        _free(m_name, 0, 0);
    m_name = 0;
    std::transform(m_nodes.begin(), m_nodes.end(), m_nodes.begin(), (CGNode *(*)(CGNode *))DeleteObject<CGNode>);
    std::transform(m_edges.begin(), m_edges.end(), m_edges.begin(), (CGEdge *(*)(CGEdge *))DeleteObject<CGEdge>);
    m_nodes.clear();
    m_edges.clear();
}

/* a node made and added at the end of the graph's list (OVL1 0x2D0930: SOCOM 1's
 * CreateNode(const char *name) without the name -- SOCOM II's CGNode has none; CGraph::Read and
 * the AI maps' graph builders call it with a1 unset). Pinned: Read calls it out of line */
#pragma auto_inline off
CGNode *CGraph::CreateNode()
{
    CGNode *node = new CGNode;

    m_nodes.push_back(node);
    return node;
}
#pragma auto_inline reset

/* a node deleted: each of its edges taken out of the other end's list and the graph's and
 * deleted, then the node itself (OVL1 0x2D0780).
 * OPEN, 0x1AC against 0x1A8: the target re-reads the iterator from its stack slot for the second
 * element read (after the first remove) and inverts the loop (its test ahead of the body as well
 * as after it); here MW keeps the element's address in s0 across the call and enters the loop
 * through a branch to its test. Tried: the wrapper's `*` for either read, the base iterator's
 * (the walker then lives in a register), the base iterator's `*` through the conversion or
 * through i_; round 44: the loop as a hand-written guard around a do-while (the weapons' round 30
 * rule) rotates it as the target's but hoists the remove_if binders' frame set-up into the
 * preheader and shortens it to 0x194 */
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

/* the index'th node, none past the end (OVL1 0x2D0620; pinned: Read calls it out of line) */
#pragma auto_inline off
CGNode *CGraph::GetNodeByIndex(unsigned int index)
{
    CGNodeList::iterator i = m_nodes.begin();

    while (index-- && i != m_nodes.end())
        ++i;
    return i == m_nodes.end() ? 0 : *i;
}
#pragma auto_inline reset

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
