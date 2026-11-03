/* graph_tree.h -- the graph shared by its two units: CGraph, CGNode, CGEdge and CGRoute.
 * graph_main.cpp (OVL1 0x2D04F0-0x2D12FF) holds CGraph's core, CGNode and CGRoute; graph_tree.cpp
 * (0x2D1300-0x2D288F) CGEdge, the route and tree building and the trees' tail. SOCOM 1's classes;
 * the header's name is ours. Every list is a pointer list, so every call into one is
 * __list_deleter<const void *>'s. */
#ifndef GRAPH_TREE_H
#define GRAPH_TREE_H

#include <list>

void _free(void *p, const char *file, int line);
extern "C" float fabsf(float x);
char *_strdup(const char *s, const char *file, int line);

class CGNode;
class CGEdge;
class CGTree;

/* the node and edge lists, each a class of its own over the pointer list (the names ours): ~CGRoute
 * (0x2D1210) and ~CGNode (0x2D1060) test their member list's `this` four times before the erase, one
 * level more than a plain list<T *> gives (the states lane, round 16) */
class CGNodeList : public std::list<CGNode *> {
};
class CGEdgeList : public std::list<CGEdge *> {
};

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

class CFileIO;

class CGraph {
public:
    /* a node's or an edge's fixup, handed each one read with the file's word for it (SOCOM 1's
     * signatures, the map file's _fixupNode/_fixupEdge) */
    typedef void (*NodeFixup)(CGraph *graph, CGNode *node, bool read, unsigned int &n, void *data);
    typedef void (*EdgeFixup)(CGraph *graph, CGEdge *edge, bool read, unsigned int &n, void *data);

    CGraph(const char *name);
    ~CGraph();
    bool Read(CFileIO &f, NodeFixup node, EdgeFixup edge, void *data);
    static CGraph *Create(const char *name);
    static void Destroy(CGraph *graph);
    /* a node made and added (SOCOM II's takes no name: CGraph::Read and the AI maps leave a1 unset) */
    CGNode *CreateNode();
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

#endif
