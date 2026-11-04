/* ai_maps.cpp (OVL2 0x5291F0-0x52EE9F, SOCOM 1's unit: __sinit_ai_maps.cpp, OVL2 0x668710; the
 * demo's 0x196E60-0x19DC9F): CAiMaps, a mission's maps -- the lookups that find a cell's map by
 * its index and the name lookups that ask every map in turn, the path registration, the map set's
 * reading and its tables. SOCOM 1's functions in the demo's order (SetOccupied ... GetLocation,
 * GetMapByIndex, Reset, Close, FreeTables, ~CAiMaps); the template tail is _max<Ui>,
 * _close_enough<f> (0x52EB60), remove_copy<CAiMapLoc> (0x52EBA0) and the names' reserve
 * (0x52EC60). Defined here in reverse of the image's order (-inline deferred). */

#include "ai_maps.h"
#include "../gamez/zsys/zsys.h"
#include "../gamez/zutil/zutil.h"

/* ai.h's _max defined (as ai_map.cpp does: MW auto-inlines no template function -- the unsigned
 * instance opens this unit's template tail, 0x52EB40) */
namespace ai {
template <class T>
const T &_max(const T &a, const T &b)
{
    return a >= b ? a : b;
}
}

/* the five name tables rebuilt, one per kind of name (1 to 5, a name's second byte after its
 * characters): each emptied, its maps' names of the kind counted, the table reserved for them,
 * each copied in with its map's index, the table sorted (SOCOM 1's; SOCOM II counts and reserves
 * first, and skips a kind no map has). The copy is the name's implicit one (CAiMapLoc's written
 * copy, the characters' 2-byte loop, the two bytes), the walks' iterators assigned (in memory), the
 * first walk's end taken once ahead of the kinds' loop: that saved register spills the kind and
 * `this` to the stack as the target's do (frame, registers and spills alike).
 * OPEN, 11 instructions short (0x324 against 0x350): block layout only -- the target keeps both
 * loops' entry tests, the kinds' loop entered through a preheader laid out after the body (the
 * field addresses, both ends, then a jump to the `k < 6` test) and the count walk's set-up and first
 * `n != end` test in a block after the function's body that jumps back to the loop's test; MW folds
 * both here (k starts at 1; the count loop's test inline). A `for` with `int k` before it, `while`
 * for the count walk and either compare order give the same 0x324 */
void CAiMaps::UpdateNameTables()
{
    iterator last = end();

    for (int k = 1; k < 6; k++) {
        m_names[k].clear();
        int count = 0;
        iterator i;

        for (i = begin(); i != last; ++i) {
            CAiMap::names &names = (*i)->m_namesTable;

            for (CAiMap::names::iterator n = names.begin(); n != names.end(); ++n)
                if ((*n).m_15 == k)
                    count++;
        }
        if (count > 0) {
            m_names[k].reserve(count);
            unsigned int mapid = 0;
            iterator e = end();

            for (i = begin(); i != e; ++i, mapid++) {
                CAiMap::names &names = (*i)->m_namesTable;

                for (CAiMap::names::iterator n = names.begin(); n != names.end(); ++n) {
                    if ((*n).m_15 == k) {
                        CAiMap::name nm = *n;

                        nm.m_loc.m_mapid = mapid;
                        m_names[k].push_back(nm);
                    }
                }
            }
        }
        m_names[k].Sort();
    }
}

/* the region lists rebuilt: each list reserved for every region of every map, then each map's
 * actions given no id and the map's index, each region the map's index and its cell listed under
 * its type (SOCOM 1's; SOCOM II reserves the lists first, and gives the actions' own cells the
 * index where SOCOM 1 gave it to their six extra cells) */
void CAiMaps::UpdateActionTables()
{
    unsigned int count = 0;
    unsigned int mapid = 0;
    iterator i;
    iterator last = end();

    for (i = begin(); i != last; ++i)
        count += (*i)->m_regionTable.size();
    for (int t = 0; t < 7; t++)
        m_regions[t].reserve(count);
    iterator e = end();

    for (i = begin(); i != e; ++i, mapid++) {
        CAiMap *map = *i;
        CAiMap::actions &actions = map->m_actionTable;

        for (CAiMap::actions::iterator a = actions.begin(); a != actions.end(); ++a) {
            (*a).m_actionId = CAiMap::action::invalid;
            (*a).m_loc.m_mapid = mapid;
        }
        CAiMap::regions &regions = map->m_regionTable;

        for (CAiMap::regions::iterator r = regions.begin(); r != regions.end(); ++r) {
            (*r).m_loc.m_mapid = mapid;
            unsigned int type = (*r).m_type;

            if (type < 7)
                m_regions[type].push_back((*r).m_loc);
        }
    }
}

/* the run of an id's spores, counted (new in SOCOM II) */
int CAiMaps::GetSporeCount(unsigned int id)
{
    CAiMap::spores::range r;

    r.m_id = 0;
    r.m_first = 0;
    r.m_last = 0;
    r.m_count = 0;
    m_spores.GetRange(id, r);
    return r.GetCount();
}

/* a spore of an id as a place to start: the n-th of its run (n wrapped round the run), its cell's
 * centre and the way it faces -- its direction's unit step, turned round (SOCOM 1's, the same eight
 * vectors spelled out, not GetDirVec's; SOCOM II returns the centre's test where SOCOM 1 returned
 * true, and looks the spore's centre up on the maps, not its map) */
bool CAiMaps::GetSporeLocation(unsigned int id, unsigned int n, CPnt3D *pos, CPnt3D *dir)
{
    CAiMap::spores::range r;

    r.m_id = 0;
    r.m_first = 0;
    r.m_last = 0;
    r.m_count = 0;
    m_spores.GetRange(id, r);
    if (r.m_count <= 0)
        return false;
    if (n >= r.m_count)
        n %= r.m_count;
    CAiMap::spore &s = m_spores[n + r.m_first];
    bool ok = GetCellCenter(s.m_loc, pos) != 0;

    if (ok) {
        switch (s.m_dir) {
        case CAiMap::N:
            dir->x = 0.0f;
            dir->y = 0.0f;
            dir->z = -1.0f;
            break;
        case CAiMap::NE:
            dir->x = 0.707f;
            dir->y = 0.0f;
            dir->z = -0.707f;
            break;
        case CAiMap::E:
            dir->x = 1.0f;
            dir->y = 0.0f;
            dir->z = 0.0f;
            break;
        case CAiMap::SE:
            dir->x = 0.707f;
            dir->y = 0.0f;
            dir->z = 0.707f;
            break;
        case CAiMap::S:
            dir->x = 0.0f;
            dir->y = 0.0f;
            dir->z = 1.0f;
            break;
        case CAiMap::SW:
            dir->x = -0.707f;
            dir->y = 0.0f;
            dir->z = 0.707f;
            break;
        case CAiMap::W:
            dir->x = -1.0f;
            dir->y = 0.0f;
            dir->z = 0.0f;
            break;
        case CAiMap::NW:
            dir->x = -0.707f;
            dir->y = 0.0f;
            dir->z = -0.707f;
            break;
        default:
            dir->x = 0.0f;
            dir->y = 0.0f;
            dir->z = -1.0f;
            break;
        }
        dir->x = -dir->x;
        dir->y = -dir->y;
        dir->z = -dir->z;
    }
    return ok;
}

/* the map set's graph as its fixups see it (graph_tree.cpp's CGEdge and CGNode: a link at an edge's
 * 0x14 and at a node's 8, where the per-map graph's fixups in ai_mapio.cpp keep a route and a region) */
class CGEdge {
public:
    unsigned char _0[0x4];
    float m_length;   // 0x4: graph_tree.h's m_4 (CreateGraph stores the walk's length)
    unsigned char _8[0xc];
    CAiMap::link *m_link;   // 0x14
};

class CGNode {
public:
    unsigned char _0[0x8];
    CAiMap::link *m_link;   // 0x8
};

/* the map set's graph saved and read back (ReadMapSet hands them to CGraph::Create): each node's and
 * edge's link as the packed cell it lies on, looked up again on that cell's map -- SOCOM 1's
 * _fixupNode in this unit, which kept the region instead; the names ours, the per-map pair in
 * ai_mapio.cpp holding SOCOM 1's. The cell a CAiMapLoc left unbuilt, its fields assigned one by one
 * from a local copy of the packed word; an edge on no cell (0) has no link */
void _fixupMapsNode(CGraph *, CGNode *node, bool fix, unsigned int &n, void *data)
{
    if (fix) {
        CAiMapLoc loc;
        unsigned int packed = n;

        loc.m_mapid = packed >> 26;
        loc.x = (packed >> 13) & 0x1fff;
        loc.y = packed & 0x1fff;
        node->m_link = ((CAiMaps *)data)->GetCellLink(loc);
    } else {
        const CAiMapLoc &loc = node->m_link->m_loc;

        n = loc.m_mapid << 26 | loc.x << 13 | loc.y;
    }
}

void _fixupMapsEdge(CGraph *, CGEdge *edge, bool fix, unsigned int &n, void *data)
{
    if (fix) {
        unsigned int packed = n;

        if (!packed) {
            edge->m_link = 0;
            return;
        }
        CAiMapLoc loc;

        loc.m_mapid = packed >> 26;
        loc.x = (packed >> 13) & 0x1fff;
        loc.y = packed & 0x1fff;
        edge->m_link = ((CAiMaps *)data)->GetCellLink(loc);
    } else {
        CAiMap::link *l = edge->m_link;

        n = l ? l->m_loc.m_mapid << 26 | l->m_loc.x << 13 | l->m_loc.y : 0;
    }
}

/* ReadCellTable<spores, spore> is ai_mapio.cpp's (0x51B9A0) */
template <class C, class T>
bool ReadCellTable(CFileIO &f, C &table);

/* whether a link joins two cells' maps, either way round (the name ours: an inline bool, which the
 * caller materialises -- each way's test, then the either) */
inline bool link_joins(const CAiMap::link *l, unsigned int a, unsigned int b)
{
    return l->m_loc.m_mapid == a && l->m_link.m_mapid == b || l->m_loc.m_mapid == b && l->m_link.m_mapid == a;
}

/* the edge of the map set's graph whose link joins the two cells' maps and lies nearest the walk
 * between them, none for two cells of one map (new in SOCOM II, the name ours: CZSealBody::
 * UpdateAiMapLoc's) -- a link no farther than 0 ends the search */
CGEdge *CAiMaps::FindLinkEdge(const CAiMapLoc &from, const CAiMapLoc &to)
{
    CGEdge *found = 0;
    CGEdge *best = 0;
    int bestDist = 0;
    CGraph *g = m_graph;

    if (g && from.m_mapid != to.m_mapid) {
        std::list<CGEdge *> &edges = g->m_edges;
        std::list<CGEdge *>::iterator e = edges.end();

        for (std::list<CGEdge *>::iterator i = edges.begin(); i != e; ++i) {
            CGEdge *edge = *i;
            CAiMap::link *l = edge->m_link;

            if (l && link_joins(l, from.m_mapid, to.m_mapid)) {
                int d = l->GetDistance(from, to);

                if (!best || d < bestDist) {
                    best = edge;
                    bestDist = d;
                    if (d == 0)
                        break;
                }
            }
        }
        found = best;
    }
    return found;
}

/* graph_tree.cpp's edge flag: bit 30 set or cleared, on setting it the two ends replaced when the
 * first differs (OVL1 0x2D1410, the states lane's extern "C" stand-in) */
extern "C" void func_002D1410(CGEdge *edge, bool on, CGNode *from, CGNode *to);

/* whether a step from a cell in a direction ends on a cell a step may end on (new in SOCOM II, the
 * name ours): GetNeighbor's step, then CAiMap::IsTraversable's test -- too tall to inline whole, so
 * CreateGraph inlines it one level and calls GetMapId, GetCell and the cell's test out of line */
inline bool step_traversable(CAiMap *map, const CAiMapLoc &loc, CAiMap::DIR dir, CAiMapLoc &n)
{
    int x = loc.x + CAiMap::neighbor[dir][0];
    int y = loc.y + CAiMap::neighbor[dir][1];

    if (x >= 0 && x < map->m_nx && y >= 0 && y < map->m_ny) {
        n.x = x;
        n.y = y;
        n.m_mapid = map->GetMapId();
        CAiCell *cell = map->GetCell(n, 0);

        return cell ? cell->IsTraversable() : false;
    }
    return false;
}

/* the map set's graph built unless it is there and not forced: every link of every map a node at
 * each end and an edge between them (a link with a side, its kind, made one-way: toward its own
 * cell when the step off that side may end there), and every earlier link of the same map that a
 * plan reaches an edge as long as the plan (SOCOM 1's; the one-way edges, the edge's link and the
 * flags' reset new in SOCOM II). Byte-exact, the AI core's round 20, by these shapes: the link a
 * plain pointer (SOCOM 1's MSL iterator was one) compared through the class iterator -- as an
 * iterator object the scalariser's temporary numbers it below the nodes and its saved register
 * moves (s4 for s1); the map walk's iterator ASSIGNED from begin() before its end is taken into a
 * local, so begin's load comes first and the end, a variable, spills to the frame's 160 for want of
 * a saved register (end() in the test is rematerialised from `this`, 0x3F8; taken before the
 * assignment the two set-ups swap registers); the neighbour's cell the caller's (an inline's own
 * CAiMapLoc is a call to its empty constructor where the inline is cut one level), and the map
 * handed to the inline non-const (a const view hoists &m_nx and &m_ny out of the walk) */
void CAiMaps::CreateGraph(bool force)
{
    if (force || !m_graph) {
        CGraph *g = CGraph::Create("AiMap Graph");
        iterator i;
        CAiMapLoc n;

        i = begin();
        iterator last = end();

        for (; i != last; ++i) {
            CAiMap *map = *i;
            CAiMap::links &links = map->m_linksTable;

            for (CAiMap::link *l = &*links.begin(); CAiMap::links::iterator(l) != links.end(); ++l) {
                CGNode *a = g->GetNodeByNodeEx(l);

                if (!a) {
                    a = g->CreateNode();
                    a->m_link = l;
                    CAiMap *far = GetMapByIndex(l->m_link.m_mapid);
                    CAiMap::link *back = far->GetCellLink(l->m_link);
                    CGNode *b = g->CreateNode();

                    b->m_link = back;
                    CGEdge *e = g->CreateEdge(a, b);

                    e->m_link = l;
                    e->m_length = 0.0f;
                    if (l->m_kind != 8) {
                        if (!step_traversable(map, l->m_loc, (CAiMap::DIR)l->m_kind, n))
                            func_002D1410(e, true, a, b);
                        else
                            func_002D1410(e, true, b, a);
                    }
                }
                for (CAiMap::links::iterator o = links.begin(); o != l; ++o) {
                    CGNode *b = g->GetNodeByNodeEx(&*o);

                    ai::FlushPlannerQueue();
                    CAiPath path;

                    path.m_flags = 0;
                    if (ai::PathPlanImmediate(map, l->m_loc, (*o).m_loc, &path)) {
                        CGEdge *e = g->CreateEdge(a, b);

                        e->m_length = path.GetLength(false);
                    }
                }
            }
        }
        m_graph = g;
    }
}

/* every map's links checked: each link given its map's index, and a link whose far cell has no link
 * back to its own cell (on the far cell's map) gathered and taken off after the walk (SOCOM 1's) */
void CAiMaps::ValidateLinks()
{
    iterator i;
    iterator e = end();

    for (i = begin(); i != e; ++i) {
        CAiMap *map = *i;
        std::vector<CAiMapLoc> bad;
        CAiMap::links &links = map->m_linksTable;

        for (CAiMap::links::iterator l = links.begin(); l != links.end(); ++l) {
            (*l).m_loc.m_mapid = map->m_mapid;
            bool ok = false;
            CAiMap *far = GetMapByIndex((*l).m_link.m_mapid);

            if (far) {
                CAiMap::link *back = far->GetCellLink((*l).m_link);

                if (back && back->m_link == (*l).m_loc)
                    ok = true;
            }
            if (!ok)
                bad.push_back((*l).m_loc);
        }
        for (std::vector<CAiMapLoc>::iterator b = bad.begin(); b != bad.end(); ++b)
            map->SetCellLink(*b, false);
    }
}

/* the map set read from its file, the path put in front of the name when one is given: a word, the
 * header, each map (given its place in the set as its id), the set's graph through the two fixups
 * above, the spores, then the tables built from the maps -- false when the file does not open
 * (SOCOM 1's; SOCOM II keeps the path, closes the old set first, sets each map's id before the push
 * and marks the set loading while it reads) */
bool CAiMaps::ReadMapSet(const char *name, const char *path)
{
    CFileIO f;
    char buf[2048];
    bool ok = false;

    if (path) {
        strcpy(buf, path);
        strcat(buf, name);
    } else {
        strcpy(buf, name);
    }
    if (f.Open(buf, 1)) {
        ok = true;
        strncpy(m_path, buf, 512);
        Close();
        unsigned int version = 0;

        f.fread(&version, 4);
        AIMAPS_PARAMS *params = this;

        f.fread(params, sizeof(AIMAPS_PARAMS));
        for (unsigned int i = 0; i < params->m_map_count; i++) {
            CAiMap *map = new CAiMap(size());

            if (map->Read(f)) {
                CAiMap *m = map;

                map->m_mapid = size();
                push_back(m);
                m_flags |= 2;
            }
        }
        m_graph = CGraph::Create(f, _fixupMapsNode, _fixupMapsEdge, this);
        ReadCellTable<CAiMap::spores, CAiMap::spore>(f, m_spores);
        UpdateActionTables();
        UpdateNameTables();
        ValidateLinks();
        CreateGraph(false);
        m_flags &= ~2;
    }
    return ok;
}

/* every map deleted, front first, then the tables (SOCOM 1's Close inlined this). Pinned: Close
 * calls it, where -inline auto takes it whole (its erase called out of line does not stop it) */
#pragma auto_inline off
void CAiMaps::FreeMaps()
{
    while (size()) {
        CAiMap *map = front();

        erase(begin());
        delete map;
    }
    FreeTables();
}
#pragma auto_inline reset

/* the spores, the six name tables and the seven region lists emptied and their storage given back,
 * then the graph destroyed (SOCOM 1's FreeTables, the same body). Each a FreeVector over the plain
 * std::vector (two this-tests in the temporaries' destructors, 0x518630, 0x518700, 0x512820), its
 * four pieces called out of line: the spores' and names' copies follow this function in reverse of
 * their first use (0x52E8B0-0x52E97F), the regions' are ai_astar.cpp's (0x512820-0x5128DF) */
void CAiMaps::FreeTables()
{
    int i;

    FreeVector<std::vector<CAiMap::spore> >(m_spores);
    for (i = 0; i < 6; i++)
        FreeVector<std::vector<CAiMap::name> >(m_names[i]);
    for (i = 0; i < 7; i++)
        FreeVector<std::vector<CAiMapLoc> >(m_regions[i]);
    CGraph::Destroy(m_graph);
    m_graph = 0;
}

/* the maps closed: the planner's queue flushed first, the cookie bumped last (SOCOM 1's). Pinned:
 * ReadMapSet calls it out of line in the target */
#pragma auto_inline off
void CAiMaps::Close()
{
    ai::FlushPlannerQueue();
    FreeMaps();
    ai::ClearOverlays();
    m_cookie++;
}
#pragma auto_inline reset

/* every map's cells back to their loaded state, the overlays dropped (SOCOM 1's) */
void CAiMaps::Reset()
{
    iterator last = end();

    for (iterator it = begin(); it != last; ++it)
        (*it)->ResetCells();
    ai::ClearOverlays();
}

/* the map with that index, none past the last (SOCOM 1's; inlined into every lookup by cell) */
CAiMap *CAiMaps::GetMapByIndex(unsigned int i) const
{
    if (i >= size())
        return 0;
    const_iterator it = begin();

    while (i--)
        ++it;
    return *it;
}

/* two cells' x and y distances summed (CAiPlanner::dist's shape: the right operand of each
 * compare loaded first) */
inline unsigned int loc_delta(const CAiMapLoc &a, const CAiMapLoc &b)
{
    int dx = a.x > b.x ? a.x - b.x : b.x - a.x;
    int dy = a.y > b.y ? a.y - b.y : b.y - a.y;

    return dx + dy;
}

/* a cell's map (the name ours: an inline a level above GetMapByIndex -- GetLocationDelta's two
 * lookups by cell take their iterator temporaries after the direct GetMapByIndex(0)'s, which
 * bottom-up inlining expands first, so map 0's get the higher stack slots as the target's do) */
inline CAiMap *map_of(CAiMaps *maps, const CAiMapLoc &loc)
{
    return maps->GetMapByIndex(loc.m_mapid);
}

/* the one-goal plan: the goal as a run of one (SOCOM 1's three-argument RegisterPathPlan, the
 * demo's 0x199860 word for word: the path moved to t0 and the five-argument form tail-called) */
bool CAiMaps::RegisterPathPlan(const CAiMapLoc &start, const CAiMapLoc &goal, CAiPath *path)
{
    return RegisterPathPlan(start, &goal, 1, path);
}

/* how many cells apart two cells are: on one map their x and y distances summed; across maps
 * both carried onto map 0's grid (SOCOM 1's GetLocationDelta carried the first onto the second's)
 * and measured there, the larger cell count when either cannot be, 10000 off the maps (0x52C320,
 * by its place: between SmoothPath's and RegisterPathPlan's slots as the demo's 0x199610) */
unsigned int CAiMaps::GetLocationDelta(const CAiMapLoc &a, const CAiMapLoc &b)
{
    if (a.m_mapid == b.m_mapid)
        return loc_delta(a, b);
    CAiMap *ma = map_of(this, a);
    CAiMap *mb = map_of(this, b);
    CAiMap *m0 = GetMapByIndex(0);

    if (!ma || !mb || !m0)
        return 10000;
    CAiMapLoc la;
    CAiMapLoc lb;

    if (ma->TransformTo(a, m0, la) && mb->TransformTo(b, m0, lb))
        return loc_delta(la, lb);
    return ai::_max(ma->GetCellCount(), mb->GetCellCount());
}

/* whether the line between two cells of one map is clear to the feet, to the eye (SOCOM 1's;
 * false across maps) */
bool CAiMaps::CanTravelLine(const CAiMapLoc &from, const CAiMapLoc &to, CAiMapLoc *out)
{
    bool r = false;

    if (from.m_mapid == to.m_mapid) {
        CAiMap *map = GetMapByIndex(from.m_mapid);

        r = map ? map->CanTravelLine(from, to, out) : false;
    }
    return r;
}

bool CAiMaps::CanSeeLine(const CAiMapLoc &from, const CAiMapLoc &to, CAiMapLoc *out)
{
    bool r = false;

    if (from.m_mapid == to.m_mapid) {
        CAiMap *map = GetMapByIndex(from.m_mapid);

        r = map ? map->CanSeeLine(from, to, out) : false;
    }
    return r;
}

/* two ints (SOCOM 1's IPNT2D) */
struct IPNT2D {
    int x, y;
};

/* the point's place in the cell, 0 to 1 each way (its corner taken off, the map's inverse cell
 * sizes applied), in x and y of a three-float point; none off the map (the name ours: the frame's
 * point handed back. A PNT2D local sits at sp+8 of the 16-byte frame, the target's at sp+0: the
 * local is 12 bytes or more) */
inline PNT3D *cell_fraction(const CAiMap *map, const CAiMapLoc &loc, const CPnt3D *pos, PNT3D &f)
{
    if (map->IsCell(loc)) {
        float z = map->m_bbox.m_min.z + loc.y * map->m_dy;

        f.x = map->m_idx * (pos->x - (map->m_bbox.m_min.x + loc.x * map->m_dx));
        f.y = map->m_idy * (pos->z - z);
        return &f;
    }
    return 0;
}

/* one axis of the blend: the neighbour toward the point's half of the cell, and the cell's own
 * share (the name ours: read through the reference twice, as the target reads f.y through its
 * address) */
inline void split_axis(const float &f, int &d, float &t)
{
    if (f > 0.5f) {
        d = 1;
        t = 1.5f - f;
    } else {
        d = -1;
        t = 0.5f + f;
    }
}

/* the cell's neighbours toward the point, one step each way, and the cell's share of the blend
 * each way; false off the map (SOCOM 1's, the same body) */
bool CAiMap::GetInterpolationCells(const CAiMapLoc &loc, const CPnt3D *pos, IPNT2D *d, PNT2D *t) const
{
    PNT3D f;

    if (cell_fraction(this, loc, pos, f)) {
        split_axis(f.x, d->x, t->x);
        split_axis(f.y, d->y, t->y);
        return true;
    }
    return false;
}

/* the map's blend (SOCOM 1's CAiMaps::GetInterpolatedShadow body): the cell's three neighbours
 * toward the point found and each tap's shadow (3 for a cell indoors when asked, else its shadow
 * bits) weighed by the point's place between the centres, the sum renormalised when taps fell off
 * the map. Too big for MW to inline whole, so the maps' function inlines it one level: CPnt3D(),
 * GetCell, IsMapCell, IsInside, GetShadow and the three-argument CAiMapLoc are called out of line
 * (0x516900, 0x511F80, 0x512140, 0x52D280, 0x52D270, 0x52D200 -- the last three this unit's
 * copies, after the maps' function). Each tap's shadow an if/else: the ternary lays the 3 out
 * after the call */
inline float CAiMap::GetInterpolatedShadow(const CAiMapLoc &loc, const CPnt3D *pos, bool inside)
{
    CPnt3D p;
    PNT2D t;
    IPNT2D d;

    if (GetInterpolationCells(loc, pos, &d, &t)) {
        float w = 0.0f;
        float s = w;
        int v;
        CAiCell *cell = GetCell(loc, 0);
        if (inside && cell->IsInside())
            v = 3;
        else
            v = cell->GetShadow();
        float k = t.x * t.y;

        w += k;
        s += v * k;
        CAiMapLoc l(loc.x + d.x, loc.y, loc.m_mapid);

        if (IsMapCell(l)) {
            cell = GetCell(l, 0);
            if (inside && cell->IsInside())
                v = 3;
            else
                v = cell->GetShadow();
            k = (1.0f - t.x) * t.y;
            w += k;
            s += v * k;
        }
        l.x = loc.x;
        l.y = loc.y + d.y;
        if (IsMapCell(l)) {
            cell = GetCell(l, 0);
            if (inside && cell->IsInside())
                v = 3;
            else
                v = cell->GetShadow();
            k = t.x * (1.0f - t.y);
            w += k;
            s += v * k;
        }
        l.x = loc.x + d.x;
        if (IsMapCell(l)) {
            cell = GetCell(l, 0);
            if (inside && cell->IsInside())
                v = 3;
            else
                v = cell->GetShadow();
            k = (1.0f - t.x) * (1.0f - t.y);
            w += k;
            s += v * k;
        }
        if (w < 0.98f)
            s /= w;
        return s * 0.33333334f;
    }
    return 0.0f;
}

/* the shadow at a point over a cell, its map's blend; 1 off the maps (SOCOM 1's) */
float CAiMaps::GetInterpolatedShadow(const CAiMapLoc &loc, const CPnt3D *pos, bool inside)
{
    CAiMap *map = GetMapByIndex(loc.m_mapid);

    return map ? map->GetInterpolatedShadow(loc, pos, inside) : 1.0f;
}

/* the cell's altitude, false off the maps (SOCOM 1's; the map's lookup inlined one level) */
bool CAiMaps::GetAltitude(const CAiMapLoc &loc, float &alt)
{
    CAiMap *map = GetMapByIndex(loc.m_mapid);

    return map ? map->GetAltitude(loc, alt) : false;
}

/* the cell's boundary bits, 0 off the maps (SOCOM 1's GetBoundary computed them; SOCOM II's reads
 * the cell's) */
unsigned int CAiMaps::GetBoundary(const CAiMapLoc &loc)
{
    unsigned int b = 0;
    CAiCell *cell = GetCell(loc);

    if (cell)
        b = cell->m_boundary;
    return b;
}

/* the cell of the first layer of the cell's map (SOCOM 1's; GetCell inlined one level) */
CAiCell *CAiMaps::GetCell(const CAiMapLoc &loc)
{
    CAiMap *map = GetMapByIndex(loc.m_mapid);

    return map ? map->GetCell(loc, 0) : 0;
}

/* the cell's centre on its map (SOCOM 1's; the map's lookup inlined whole, CAiMap's centre one
 * level) */
CPnt3D *CAiMaps::GetCellCenter(const CAiMapLoc &loc, CPnt3D *center)
{
    CAiMap *map = GetMapByIndex(loc.m_mapid);

    return map ? map->GetCellCenter(loc, center) : 0;
}

/* the map that knows the name, the cell given its index (SOCOM 1's) */
bool CAiMaps::GetLocation(const char *name, CAiMapLoc &loc)
{
    const CAiMaps *maps = this;
    int i = 0;

    for (const_iterator it = maps->begin(); it != maps->end(); ++it, ++i) {
        if ((*it)->GetLocation(name, loc)) {
            loc.m_mapid = i;
            return true;
        }
    }
    return false;
}

/* the map that has the action, the cell given its index (SOCOM 1's) */
bool CAiMaps::GetActionById(unsigned int id, CAiMapLoc &loc, CAiMap::action **pa)
{
    int i = 0;
    iterator last = end();

    for (iterator it = begin(); it != last; ++it, ++i) {
        if ((*it)->FindActionLocation(id, loc, pa)) {
            loc.m_mapid = i;
            return true;
        }
    }
    return false;
}

/* the named region of the map that has it, the cell given its index; none, and the invalid cell,
 * when no map has it (SOCOM 1's) */
CAiMap::region *CAiMaps::GetRegion(const char *name, CAiMapLoc &loc)
{
    if (name) {
        int i = 0;
        iterator last = end();

        for (iterator it = begin(); it != last; ++it, ++i) {
            CAiMap::region *r = (*it)->GetRegion(name, loc);

            /* `!= 0`: a bare `if (r)` materialises the test (sltu, xori) before it branches */
            if (r != 0) {
                loc.m_mapid = i;
                return r;
            }
        }
    }
    loc = CAiMapLoc::invalid;
    return 0;
}

/* the cell's region, its action, its link: its map's (SOCOM 1's; the link new) */
CAiMap::region *CAiMaps::GetRegion(const CAiMapLoc &loc)
{
    CAiMap *map = GetMapByIndex(loc.m_mapid);

    return map ? map->GetRegion(loc) : 0;
}

CAiMap::action *CAiMaps::GetCellAction(const CAiMapLoc &loc)
{
    CAiMap *map = GetMapByIndex(loc.m_mapid);

    return map ? map->GetCellAction(loc) : 0;
}

CAiMap::link *CAiMaps::GetCellLink(const CAiMapLoc &loc)
{
    CAiMap *map = GetMapByIndex(loc.m_mapid);

    return map ? map->GetCellLink(loc) : 0;
}

/* the region of the type whose centre cell lies nearest the cell, level distance squared; one
 * holding the cell is passed over unless `inside` (SOCOM 1's). The type's list indexed in the walk
 * itself (the target keeps the size's and the data's addresses for each pass's end(), where a
 * reference local keeps the vector's), the centre by value (copied out of its temporary) */
CAiMap::region *CAiMaps::GetClosestRegion(const CAiMapLoc &loc, CAiMap::region::TYPE type, bool inside)
{
    CAiMap::region *best = 0;
    float bestd = 0.0f;
    CPnt3D c;

    if (GetCellCenter(loc, &c)) {
        for (CAiMapLocVec::iterator it = m_regions[type].begin(); it != m_regions[type].end(); ++it) {
            CAiMap *map = GetMapByIndex(it->m_mapid);
            CAiMap::region *r = map ? map->GetRegion(*it) : 0;

            if (inside || !r->Contains(loc)) {
                CPnt3D p;

                if (GetCellCenter(r->GetCenter(), &p)) {
                    p.Sub(&c, &p);
                    float d = p.x * p.x + p.z * p.z;

                    if (!best || d < bestd) {
                        best = r;
                        bestd = d;
                    }
                }
            }
        }
    }
    return best;
}

/* the first region of the type that holds the cell, through the cells listed for the type --
 * each one's map found by index (inlined) and its region asked (SOCOM 1's) */
CAiMap::region *CAiMaps::GetRegionTypeContainer(CAiMap::region::TYPE type, const CAiMapLoc &loc)
{
    CAiMapLocVec &v = m_regions[type];

    for (CAiMapLocVec::iterator it = v.begin(); it != v.end(); ++it) {
        CAiMap *map = GetMapByIndex(it->m_mapid);
        CAiMap::region *r = map ? map->GetRegion(*it) : 0;

        if (r->Contains(loc))
            return r;
    }
    return 0;
}

/* whether a step may end on the cell: on the map, and not a no-go soil (new in SOCOM II) */
bool CAiMap::IsTraversable(const CAiMapLoc &loc) const
{
    CAiCell *cell = GetCell(loc, 0);

    return cell ? cell->IsTraversable() : false;
}

/* pinned: GetRandomLoc calls it out of line in the target */
#pragma auto_inline off
CAiMap *CAiMaps::GetMap(const CAiMapLoc &loc) const
{
    return GetMapByIndex(loc.m_mapid);
}
#pragma auto_inline reset

/* the cell's name, its map's; as a string, empty when it has none (SOCOM 1's) */
CAiMap::name *CAiMaps::GetCellName(const CAiMapLoc &loc) const
{
    CAiMap::name *n = 0;
    CAiMap *map = GetMapByIndex(loc.m_mapid);

    if (map)
        n = map->GetCellName(loc);
    return n;
}

const char *CAiMaps::GetCellNameString(const CAiMapLoc &loc) const
{
    CAiMap::name *n = GetCellName(loc);

    return n ? n->m_name : "";
}

/* the region tiled on its map's first layer, the planner's queue reset after a tile (SOCOM 1's;
 * the map's lookup inlined whole, the arguments handed on in their registers) */
bool CAiMaps::Tile(const CAiMap::region *r, unsigned int value, unsigned int flags)
{
    if (!r)
        return false;
    CAiMap *map = GetMapByIndex(r->m_loc.m_mapid);
    bool ok = false;

    if (map) {
        ok = map->Tile(r, value, flags, 0);
        ai::ResetPathPlan();
    }
    return ok;
}

/* the region's type changed: its cell removed from the old type's list and added to the new
 * one's, true when it had the type already (SOCOM 1's). The removal is an inline cut to one level
 * (CAiMapLocVec::Remove): the target calls even its iterators' constructors and assignment, and
 * std::remove, out of line (0x529D30-0x529ED7, after it), where push_back is inlined.
 * OPEN, right size, 10 instructions: registers only -- the target keeps the region in s1 and the
 * type in s2, here the other way round. Tried: `old == type`, `old` unsigned, the test on the
 * field, the cell as a reference local, the region cast into a non-const local (57 off) */
bool CAiMaps::SetRegionType(const CAiMap::region *r, CAiMap::region::TYPE type)
{
    CAiMap::region::TYPE old = (CAiMap::region::TYPE)r->m_type;

    if (type == old)
        return true;
    ((CAiMap::region *)r)->m_type = type;
    m_regions[old].Remove(r->m_loc);
    m_regions[type].push_back(r->m_loc);
    return false;
}

/* 0 to n, n not drawn for (the name ours: GetRandomLoc's two draws) */
inline int _random(int n)
{
    return n == 0 ? 0 : rand() % (n + 1);
}

/* a random cell of the region on the region's map, drawn up to eight times until one is
 * traversable (SOCOM 1's; the cell's test through the map's IsTraversable, new in SOCOM II).
 * OPEN, 2 instructions short (0x1A4 against 0x1AC): the target pads each draw's join with a nop
 * after the `mfhi` (and so leaves the y draw's `b` delay slot empty, where this build fills it
 * with the next load) -- registers, branches and order otherwise as the target's. Tried: the
 * draw as a ternary either way round, if/else, early returns, through a local, and written out
 * in the body; GetMap pinned (the target calls it: inlined, the map's walk adds 0x58 bytes) */
bool CAiMaps::GetRandomLoc(const CAiMap::region *r, CAiMapLoc &loc)
{
    bool found = false;
    int tries = 8;

    while (tries--) {
        int dx = _random(r->GetDx() - 1);
        int dy = _random(r->GetDy() - 1);

        loc.x = r->m_loc.x + dx;
        loc.y = r->m_loc.y + dy;
        loc.m_mapid = r->m_loc.m_mapid;
        CAiMap *map = GetMap(loc);
        bool ok = map ? map->IsTraversable(loc) : false;

        if (ok) {
            found = true;
            break;
        }
    }
    return found;
}

/* the old cell freed unless an overlay lies over it (the overlays' walk inlined), the new one
 * taken */
void CAiMaps::SetOccupied(const CAiMapLoc &from, const CAiMapLoc &to, unsigned int)
{
    CAiCell *cell = GetCell(from);

    if (cell && !ai::overlays.Contains(cell, from))
        cell->SetOccupied(false);
    cell = GetCell(to);
    if (cell)
        cell->SetOccupied(true);
}

/* the point's cell, then a traversable one near it */
bool CAiMaps::GetTraversableLocation(const CPnt3D *pos, CAiMapLoc &out)
{
    CAiMapLoc loc;
    bool found = GetLocation(pos, loc);

    if (found)
        found = FindTraversable(loc, out);
    return found;
}

float CAiMaps::GetDistance(const CAiMapLoc &a, const CAiMapLoc &b)
{
    return sqrtf(GetDistanceSq(a, b));
}

#pragma auto_inline off
float CAiMaps::GetDistanceSq(const CAiMapLoc &a, const CAiMapLoc &b)
{
    float d = 0.0f;

    if (a != b) {
        CPnt3D pa;
        CPnt3D pb;

        if (GetCellCenter(a, &pa) && GetCellCenter(b, &pb)) {
            CPnt3D v;

            pa.Sub(&pb, &v);
            return v.x * v.x + v.y * v.y + v.z * v.z;
        }
        d = 1e12f;
    }
    return d;
}
#pragma auto_inline reset
