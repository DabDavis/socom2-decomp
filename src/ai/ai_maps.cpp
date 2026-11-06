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
 * first walk's end taken into a local at the top of each kind (its saved register spills the kind
 * and `this` to the stack as the target's do). Two preheaders after the function's last block, as
 * the target's (the AI core round 25; 0x324 before, both folded inline): the end local inside the
 * kinds' loop is a loop invariant MW hoists into the kinds' loop's preheader (the field addresses,
 * both ends, then a jump to the `k < 6` test), and the count walk's postfix step hoists its end into
 * the walk's own (the online lane's postfix rule).
 * OPEN, right size, 5 instructions since round 37 (12 in round 36, 34 before), registers only: the count walk's hoisted end and its
 * byte, the target's v1 and a2, here a2 and v1. build/scratch/ai/ig2.py over every placement of the two: the target needs the byte
 * numbered below the end and below the walk's cursor, so coloured after both -- and the byte is a code-generation temporary, above
 * every variable. So the original's byte is a variable that reaches the PCode. A named one read once is expression-propagated in
 * pass 0 (IRO_ExpressionPropagation: useCount 1, whatever other blocks do; the log's Operand count falls there), one local reused by
 * both walks too (the compare's operands swap: 6 off), an inline accessor's return alike (build/scratch/ai/v_unt37*.py). What round
 * 37 fixed: `i = begin()` ahead of the named end in the push walk's head (v0 stays busy with `this` through the two address
 * temporaries: round 29's "code-generation temporaries" below, v76 / v77, 12 -> 5).
 * Before that: the target gives the push walk's names
 * iterator s0 and the name-table cursor (m_names[k], strength-reduced) s1, this build the other way
 * round (the iterator's scalar @7178 is numbered below the cursor's @7190), and the count walk's
 * cursor and end take a0/v1 where this build's take a2/a0. A named reference to m_names[k] (0x360),
 * the push walk's iterator declared outside its for, a postfix push step, `e` before `mapid`: no
 * change. The AI core round 29 (tools/igreplay.py --target): four virtual registers differ -- the
 * push iterator's scalar (v41, @7452: ours s1, the target s0) and the cursor (v42, @7464: s0 / s1),
 * and the count walk's cursor and end (v76/v77, code-generation temporaries: v1/v0 against a0/v1).
 * --movers finds each pair alone (v41 past @7498 at 50.5 with v42 anywhere from 40.5; v76 at 31.5
 * or 32.5 -- ahead of `k`, even of `this`), never the four together: the count walk's pair wants no
 * numbering a source gives, so its interference differs -- the walk's shape, not its order. The AI
 * core round 34 (build/scratch/ai/v_unt34.py): the count walk as an inline over a map's names --
 * returning its own count (43 off), adding into the caller's by reference (34: as it is), over a
 * const table (62), through a map pointer local (38) -- does not give it either.
 * The AI core round 36 (igdump --origins / --iro, build/scratch/ai/v_unt36.py): v41 is the push walk's
 * iterator member made by IRO_ScalarizeClassDataMembers in PASS 0 (its scalar read by `(*n).m_15 ==
 * k` and `nm = *n`), v42 the cursor IRO_StrengthReduction makes of `this + k * 12` in pass 0's loop
 * passes, after the scalariser -- so the target's order (the iterator's member numbered after the
 * cursor) wants the iterator scalarised only in pass 1, as MW does for an object some access conflicts
 * with in pass 0 (resize<reader>'s end().m_p temporary, ai_mapio.cpp: a named iterator scalarises in
 * pass 0, the temporary in pass 1). The push walk's iterator stepped by postfix, assigned outside its
 * for (prefix or postfix) or direct-initialised: still pass 0, 34 off each. TAKING ITS ADDRESS in pass
 * 0 does it (build/scratch/ai/v_unt36b-d.py): a reference local bound to the walk's iterator (or a
 * pointer to it), folded away by pass 0's propagation, leaves the object noregister for pass 0's
 * scalariser and scalarisable in pass 1 -- 34 -> 14 in the push walk, 12 with the count walk's alike
 * (the modelling shape below: `iterator &rn = n`, its test through rn); an inline test taking the
 * iterator by (const) reference is substituted by the plain object and changes nothing (34). Left (12
 * instructions): the count walk's cursor and end (v76 / v77, code generation's own registers: v1 / v0
 * against the target's a0 / v1) -- `igreplay --movers 76,77` still wants v76 at 31.5, ahead of `this`,
 * so the count walk's shape, not a numbering, as round 29 found */
void CAiMaps::UpdateNameTables()
{
    for (int k = 1; k < 6; k++) {
        iterator last = end();

        m_names[k].clear();
        int count = 0;
        iterator i;

        for (i = begin(); i != last; ++i) {
            CAiMap::names &names = (*i)->m_namesTable;

            for (CAiMap::names::iterator n = names.begin(); n != names.end(); n++) {
                CAiMap::names::iterator &rn = n;

                if ((*rn).m_15 == k)
                    count++;
            }
        }
        if (count > 0) {
            m_names[k].reserve(count);
            unsigned int mapid = 0;
            i = begin();
            iterator e = end();

            for (; i != e; ++i, mapid++) {
                CAiMap::names &names = (*i)->m_namesTable;

                for (CAiMap::names::iterator n = names.begin(); n != names.end(); ++n) {
                    CAiMap::names::iterator &rn = n;

                    if ((*rn).m_15 == k) {
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
    unsigned int m_mask;   // 0x18: the path's mask RegisterPathPlan's joining edges carry
};

class CGNode {
public:
    unsigned char _0[0x8];
    CAiMap::link *m_link;   // 0x8
};

/* a route through the graph: its nodes in order (graph_tree.h's CGRoute; its constructor and
 * destructor graph_main.cpp's) */
class CGRoute {
public:
    CGRoute();
    ~CGRoute();

    std::list<CGNode *> m_nodes;   // 0x0
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

/* the mission's maps (SOCOM 1's AiMaps; 0x66A1B0, 0x2E0 bytes): __sinit_ai_maps.cpp (0x668710)
 * builds it -- the list and the spores inline, the arrays' element constructors called through
 * __construct_array (their copies, CAiMapLocVec() 0x52EB00 and CAiMap::names() 0x52EB20, right
 * after ~CAiMaps), the graph and the cookie zeroed -- and registers ~CAiMaps.
 * OPEN at -inline level=5 (0x3C against 0xB4: the initialiser calls the constructor), BYTE-EXACT
 * AT LEVEL 6 (`#pragma inline_depth(6)` at the unit's end, measured): the constructor is an own
 * function of the unit, dead-stripped (ABSENT.txt) -- written out of line it instantiates the pointer
 * list's out-of-class deleter constructor, which the target's initialiser inlines (no function of
 * the image's ai_maps.cpp calls OVL2's copy 0x50FFA0, so the instantiating function is gone), and
 * the initialiser takes it whole (-inline auto: all or nothing) only if its tree is at most the
 * level high: the spores' chain (CAiMaps(), spores(), vector(), __vector_imp(), the deleter's,
 * __vector_capacity()) is six. As an inline in-class constructor it is cut to one level at 5 (0x98:
 * list() and spores() called) and needs `inline_depth(6)` plus an own function building a pointer
 * list (0xA4 with neither) */
CAiMaps AiMaps;

CAiMaps::CAiMaps() : m_graph(0), m_cookie(0) {}

/* the set's graph destroyed and cleared; the members' destructors follow (SOCOM 1's ~CAiMaps, the
 * demo's 0x19D790: the last of the unit's functions in both builds) */
CAiMaps::~CAiMaps()
{
    CGraph::Destroy(m_graph);
    m_graph = 0;
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

/* whether a step may end on the cell: its map's IsTraversable, false off the maps (GetMap and the
 * test out of line, the result narrowed in both arms) */
inline bool traversable(const CAiMaps *maps, const CAiMapLoc &loc)
{
    CAiMap *map = maps->GetMap(loc);

    return map ? map->IsTraversable(loc) : false;
}

/* a path planned from a cell through the given cells: every cell must be one a step may end on;
 * then leg by leg -- on one map the leg itself, across maps through the map set's graph: two new
 * nodes joined to every node whose link lies on the leg's first or last map (the edge as long as
 * the cells' x and y distances summed, carrying the path's mask), the route between them built
 * with the mask (or, failing that, with none), a leg added for each step of the route that stays
 * on its map, the two nodes deleted again. The plan aborted on the first failure, run otherwise
 * (SOCOM 1's, the demo's 0x199870; the mask, the second try and the map's IsTraversable new in
 * SOCOM II). Written in the AI core's round 32 (unstarted before), from the target's code; BYTE-EXACT
 * in round 37 (0x5F8 and 105 off before, build/scratch/ai/v_rpp37*.py). Each shape settles what the
 * image shows, all but the last numbering or frame:
 *  - the leg's two cells an ARRAY: `leg[1].y` is `(&leg + 4) + 2`, where `leg.to.y` is folded to `&leg + 6` by
 *    the front end. IRO hoists &leg[1] (s6), then `s6 + 2`, which is no frame constant, so it is truly
 *    spilled (176) and the frame 16 taller; `&leg[0].y` stays a constant, rematerialised at its use (290);
 *  - one counter for both loops, one iterator for both walks: IRO_SplitLifetimes splits each later web
 *    into a temporary made after every other (the route walk's two slots, 304 / 300, below all
 *    the temporaries; the counter's spill slot, 160, the last);
 *  - node declared ahead of mask (the kit's round 59): mask s2, node s3;
 *  - the walk's end named, from a reference to the list used for begin() too: `nodes` read twice is
 *    not expression-propagated (useCount, IRO_ExpressionPropagation), so `nodes + 4` keeps the image's
 *    two adds -- propagated, the generator folds them to `s4 + 8` -- and the end, scalarised in pass 0, is
 *    numbered ahead of the loop temporaries (spill slot 192, above &leg[1].y's 176). In the loop test
 *    `nodes.end()` is no invariant at all: IRO_IsSubableExpression refuses variable + constant;
 *  - the route's cell an if/else on !l (a ternary selects an address), the goals stepped in the loop's
 *    increment after the leg's copy. The route's two cells stay one 8-byte struct declared inside its
 *    block (264 / 268, below the route at 272, &step hoisted into s0) */
bool CAiMaps::RegisterPathPlan(const CAiMapLoc &start, const CAiMapLoc *goals, int count, CAiPath *path)
{
    if (!traversable(this, start))
        return false;
    int i;

    for (i = 0; i < count; i++)
        if (!traversable(this, goals[i]))
            return false;
    ai::PathPlanBegin(path);
    bool ok = true;
    CAiMapLoc leg[2];

    leg[0] = start;
    for (i = 0; ok && i < count; goals++, i++) {
        leg[1] = *goals;
        if (leg[0].m_mapid == leg[1].m_mapid) {
            CAiMap *map = GetMapByIndex(leg[0].m_mapid);

            ok = map != 0;
            if (ok)
                ai::PathPlanAddLeg(map, leg[0], leg[1]);
        } else {
            CGraph *graph = m_graph;

            ok = graph != 0;
            if (ok) {
                CGNode *n0 = graph->CreateNode();
                CGNode *n1 = graph->CreateNode();
                CGNode *node;
                unsigned int mask = path->m_unk24;

                std::list<CGNode *>::iterator it;
                std::list<CGNode *> &nodes = graph->m_nodes;
                std::list<CGNode *>::iterator last = nodes.end();

                for (it = nodes.begin(); it != last; ++it) {
                    node = *it;
                    CAiMap::link *l = node->m_link;

                    if (l) {
                        if (l->m_loc.m_mapid == leg[0].m_mapid) {
                            CGEdge *e = graph->CreateEdge(n0, node);

                            e->m_length = (int)loc_delta(l->m_loc, leg[0]);
                            e->m_mask = mask;
                        }
                        if (l->m_loc.m_mapid == leg[1].m_mapid) {
                            CGEdge *e = graph->CreateEdge(n1, node);

                            e->m_length = (int)loc_delta(l->m_loc, leg[1]);
                            e->m_mask = mask;
                        }
                    }
                }
                CGRoute route;

                if (graph->BuildRoute(n0, n1, mask, &route) || (mask && graph->BuildRoute(n0, n1, 0, &route))) {
                    struct { CAiMapLoc from; CAiMapLoc to; } sub;
                    it = route.m_nodes.begin();
                    sub.from = leg[0];
                    for (++it; it != route.m_nodes.end(); ++it) {
                        CAiMap::link *l = (*it)->m_link;

                        if (!l)
                            sub.to = leg[1];
                        else
                            sub.to = l->m_loc;
                        if (sub.from.m_mapid != sub.to.m_mapid) {
                            sub.from = sub.to;
                        } else {
                            ai::PathPlanAddLeg(GetMapByIndex(sub.from.m_mapid), sub.from, sub.to);
                            sub.from = sub.to;
                        }
                    }
                } else {
                    ok = false;
                }
                graph->DeleteNode(n0);
                graph->DeleteNode(n1);
            }
        }
        leg[0] = leg[1];
    }
    if (!ok)
        ai::PathPlanAbort();
    else
        ai::PathPlanExecute();
    return ok;
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


/* a path stretched to a cell (SOCOM 1's ExtendPath, the demo's 0x199220 after UpdateActionTables
 * as here; SOCOM II takes the walk's first cell too): an empty path takes the cell when the feet
 * reach it from the first cell; a path starting on it is done; a one-point path
 * has its point moved there when the feet reach it; else the cell is appended when the feet reach
 * it from the path's first point, or, on a path following a template (flag 8), the template's points
 * between the two cells are appended in order and then the cell -- the path finished after either.
 * The shapes: the size read in each test (a local numbers it ahead of the compares' temporaries), the
 * points through data_ (operator[]'s data() reference takes &data_ into a saved register), the
 * template declared ahead of the first point (their saved registers swap otherwise), the step an
 * if/else on `(i = i0) <= i1` (branching on the compare; `if (i1 < i) step = -1` lays the -1 out
 * after; the walk's start assigned inside the compare's left operand loads i1 first, as the target
 * does -- the right operand is evaluated first -- where `unsigned int i = i0;` ahead of the compare
 * loads i0 first, and `i = i0;` after the step reloads it), the walk top-tested (`if (i != i1) do
 * ... while`: a `for` is entered at its bottom test) */
bool CAiMaps::ExtendPath(CAiPath &path, const CAiMapLoc &from, const CAiMapLoc &to)
{
    bool smooth = false;
    bool ok = false;
    bool post = false;

    if (path.m_points.size() == 0) {
        if (CanTravelLine(from, to, 0)) {
            ok = true;
            path.AppendPathLoc(to, 0);
        }
    } else {
        CAiPath *t;
        CAiPath::point *first = path.m_points.data_;

        if (first->m_loc == to) {
            ok = true;
        } else if (path.m_points.size() == 1 && CanTravelLine(from, to, 0)) {
            path.m_points.data_[path.m_points.size() - 1].m_loc = to;
            path.IncrementPathId();
        } else if (CanTravelLine(first->m_loc, to, 0)) {
            path.AppendPathLoc(to, 0);
            ok = smooth = post = true;
        } else if (path.m_flags & 8) {
            unsigned int i0;
            unsigned int i1;

            t = path.m_template;

            if (t && t->PointInPath(first->m_loc, &i0) && t->PointInPath(to, &i1)) {
                int step;
                unsigned int i;

                ok = true;
                if ((i = i0) <= i1)
                    step = 1;
                else
                    step = -1;
                if (i != i1) {
                    do {
                        CAiPath::point &p = t->m_points[t->m_points.size() - i - 1];

                        if (!path.AppendPathLoc(p.m_loc, GetMapByIndex(p.m_loc.m_mapid))) {
                            ok = false;
                            break;
                        }
                        i += step;
                    } while (i != i1);
                }
                if (ok)
                    ok = path.AppendPathLoc(to, GetMapByIndex(to.m_mapid));
                smooth = post = true;
            }
        }
        if (post)
            path.RebuildLegs(this, smooth);
    }
    return ok;
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

/* whether a cell is on its row's run, the row read straight off the bitfield (ai_mapwalk.cpp's _on_run: the
 * target's `bltz a0` with no andi; IsMapCell, called out of line inside GetCell, narrows it) */
inline bool _on_run(const CAiMap *map, const CAiMapLoc &loc)
{
    if (loc.y >= 0 && loc.y < map->m_ny) {
        const CAiMap::scanline &sl = map->GetScanline(loc.y);
        bool in = false;
        unsigned int x = loc.x;

        if (x >= sl.m_start && x < sl.m_end)
            in = true;
        return in;
    }
    return false;
}

/* whether a step may not end on the cell: IsTraversable's soils, its bool negated by the caller (`!` of an inline's
 * bool keeps the target's andi ahead of its sltu, xori; IsTraversable's own negation has none) */
inline bool _no_ground(const CAiCell *cell)
{
    unsigned int soil = cell->m_soil;

    return soil == 0 || soil == 10 || soil == 8 || soil == 9;
}

/* by reference: the literal is an object of the function's constant pool, -10 at 0x64F958 beside _close_enough's
 * (written straight, or as a `static const float`, it is folded to lui / mtc1) */
inline bool _le(const float &a, const float &b)
{
    return a <= b;
}

/* a cell's altitude as GetLocation reads it: CAiMap::GetAltitude's words as a CONDITIONAL. Its value is dead, so its
 * two arms' temporaries die in the back end, after the branches are laid out: the target's `beqz s0,L ... b L; add.s;
 * L:`. CAiMap::GetAltitude's own form (its copy 0x516940 is that form's `li v0,1`, no sltu; the conditional's is
 * daddiu) loses the jump: IRO empties its else arm while it can still drop it (a nop-padded L) */
inline bool _cell_altitude(const CAiMap *map, const CAiCell *cell, float &alt)
{
    return cell ? (alt = map->m_bbox.m_min.y + map->m_alt_quant * (float)cell->m_altitude, true) : false;
}

/* the cell under a point: the first map whose cells hold it, its index as the map id -- passed
 * over when the first map's cell is a seam (m_mes) and the map's cell is SOIL_NOALT; when that
 * cell is no ground a step may end on or lies more than half a unit off the point's height, and
 * the first map's cell is a seam, the maps after it are tried too, each cell not SOIL_NOALT and
 * no more than 10 units above the point taken when its height lies no further off than the best's
 * (half a unit more when it is ground and the best not) (SOCOM 1's GetLocation(CPnt3D *, loc), the
 * demo's 0x19C9B0, 0xA04 bytes; the seam, the NOALT soil and the second walk new in SOCOM II, by
 * the code -- the AI core's round 31, written from m2c).
 * BYTE-EXACT in the AI core's round 37 (0xA44 and 172 off since round 32, build/scratch/ai/v_gl37*.py, fix_gl*.py). Every
 * call is the target's: the map's GetLocation inlined whole, GetCell one level, _close_enough<f> on literals, a const
 * view of the maps with postfix steps, one cell `l` for both walks (the frame's top: best 284, l 280, dh 276, the
 * iterator 272). What round 37 settled, tools/structdiff.py first (registers normalised), the numbering after:
 *  - no `j`: the second walk steps `i` on; _on_run, _no_ground, _le and _cell_altitude (each with its evidence above);
 *  - `best` built by CAiMapLoc(cell, id) (zentity.h's, additive): the three-argument form makes argument temporaries
 *    of the address-taken `l`; this reads it in place, id, x, y (a named local's field stores narrow to sb / sh);
 *  - `d` no named local: _close_enough's temporaries at 256 / 252, below the call's copies (named, 264 / 260);
 *  - the better block's dh ahead of best (the scheduler's order);
 *  - THE NUMBERING IS NOT PARAMETERS FIRST. No order with this and pos below the locals gives the target's registers
 *    (annealing over every local's slot, best 7 off; every sequence of spill picks, 0 hits): at K 25 with 19 hard
 *    neighbours map and m are symmetric, so `this` can only be pushed with its five last neighbours left after a
 *    sweep in which the others went in one pass. build/scratch/ai/igallperm.py with this and pos free finds thousands, all with
 *    this < pos < i, m < ground, m < c, seam < cell, map < cell. The one written: map, seam and cell first, the
 *    named copies of this and pos, found, then m, ground, i, c. Their shape in the original is not known (the
 *    copies stand for it): a modelling order, marked as such */
bool CAiMaps::GetLocation(const CPnt3D *pos, CAiMapLoc &loc)
{
    CAiMap *map;
    bool seam;
    CAiCell *cell;
    const CAiMaps *maps = this;
    const CPnt3D *p = pos;
    bool found;
    CAiMap *m;
    bool ground;
    int i;
    CAiCell *c;
    bool ground2;
    bool better;

    if (!maps->size())
        return false;
    CAiMapLoc best;
    CAiMapLoc l;
    float dh = 0.0f;

    found = false;
    seam = false;
    i = 0;
    for (const_iterator it = maps->begin(); it != maps->end(); it++, i++) {
        map = *it;

        if (!map->GetLocation(p, l) || !_on_run(map, l))
            continue;
        cell = map->GetCell(l, 0);

        if (i == 0)
            seam = cell->m_mes ? true : false;
        if (seam && cell->m_soil == CAiMap::SOIL_NOALT)
            continue;
        found = true;
        best = CAiMapLoc(l, i);
        float alt;

        _cell_altitude(map, cell, alt);
        dh = p->y - alt;
        ground = !_no_ground(cell);

        if ((!ground || !_close_enough(dh, 0.0f, 0.5f)) && seam) {
            for (it++, i++; it != maps->end(); it++, i++) {
                m = *it;
                if (!m->GetLocation(p, l))
                    continue;
                c = m->GetCell(l, 0);

                if (!_on_run(m, l) || c->m_soil == CAiMap::SOIL_NOALT)
                    continue;
                float alt2;

                _cell_altitude(m, c, alt2);
                float dh2 = p->y - alt2;
                if (_le(dh2, -10.0f))
                    continue;
                ground2 = !_no_ground(c);
                better = false;

                if (!ground2 || ground) {
                    if (_close_enough(dh2, 0.0f, fabsf(dh)))
                        better = true;
                } else {
                    if (_close_enough(dh2, 0.0f, 0.5f + fabsf(dh)))
                        better = true;
                }
                if (better) {
                    ground = ground2;
                    dh = dh2;
                    best = CAiMapLoc(l, i);
                }
            }
        }
        break;
    }
    if (found)
        loc = best;
    return found;
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
 * std::remove, out of line (0x529D30-0x529ED7, after it), where push_back is inlined. Byte-exact
 * (the AI core round 22) through a non-const local of the region, numbered after the parameters
 * (the region s1, the type s2), the old type read through the const parameter (the store then
 * reloads the byte), and the new type stored as an int: the narrowed type the compare made (andi
 * 0xff in v0) is reused for the store's mask, as the target's -- stored plain it is masked from the
 * parameter's register (3 off), the type through `reg` 57 off */
bool CAiMaps::SetRegionType(const CAiMap::region *r, CAiMap::region::TYPE type)
{
    CAiMap::region *reg = (CAiMap::region *)r;
    CAiMap::region::TYPE old = (CAiMap::region::TYPE)r->m_type;

    if (type == old)
        return true;
    reg->m_type = (int)type;
    m_regions[old].Remove(reg->m_loc);
    m_regions[type].push_back(reg->m_loc);
    return false;
}

/* a draw below m, an inline of its own: its exit, the end of the ternary's second arm, is the
 * label MW pads to 8 bytes after the `mfhi` -- the target's nop -- where the draw written in the
 * ternary leaves the join unpadded (and fills the y draw's `b` with the next load). The names ours */
inline int _rnd(int m)
{
    return rand() % m;
}

/* 0 to n, n not drawn for (the name ours: GetRandomLoc's two draws) */
inline int _random(int n)
{
    return n == 0 ? 0 : _rnd(n + 1);
}

/* a random cell of the region on the region's map, drawn up to eight times until one is
 * traversable (SOCOM 1's; the cell's test through the map's IsTraversable, new in SOCOM II).
 * Byte-exact (the AI core round 22) once the draw went through an inline of its own (_rnd above:
 * each draw's join padded with a nop after the `mfhi`, the y draw's `b` delay slot left empty);
 * the ternary either way round, if/else, early returns and a local all gave 0x1A4. GetMap pinned
 * (the target calls it: inlined, the map's walk adds 0x58 bytes) */
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

/* whether a step may end on the cell: CAiMap::IsTraversable(loc)'s body as an inline, cut to one
 * level -- GetCell and the cell's test called (FindTraversable's first test; the member itself is
 * called whole there. The name ours) */
inline bool _traversable_cell(const CAiMap *map, const CAiMapLoc &loc)
{
    CAiCell *cell = map->GetCell(loc, 0);

    return cell ? cell->IsTraversable() : false;
}

/* a step from a cell in a direction to a traversable cell, its cell handed back (FindTraversable's
 * step: GetMapId, GetCell and the cell's test called out of line -- ai_mapwalk.cpp's travel_step;
 * the name ours) */
inline bool _traversable_step(CAiMap *map, const CAiMapLoc &loc, CAiMap::DIR dir, CAiMapLoc &out, CAiCell *&cell)
{
    int x = loc.x + CAiMap::neighbor[dir][0];
    int y = loc.y + CAiMap::neighbor[dir][1];

    if (x >= 0 && x < map->m_nx && y >= 0 && y < map->m_ny) {
        out.x = x;
        out.y = y;
        out.m_mapid = map->GetMapId();
        cell = map->GetCell(out, 0);
        return cell ? cell->IsTraversable() : false;
    }
    return false;
}

/* a cell a step may end on at or next to one: the cell itself if it is traversable and no overlay
 * lies over it; else the free neighbours counted (the first kept), one taken as it is, and of two
 * or more the one with the most free neighbours beside it (new in SOCOM II, the names ours:
 * GetTraversableLocation's second call; the maps asked through AiMaps, not `this`) */
bool CAiMaps::FindTraversable(const CAiMapLoc &loc, CAiMapLoc &out)
{
    bool found = false;
    CAiMap *map = map_of(&AiMaps, loc);
    CAiCell *cell;

    if (_traversable_cell(map, loc) && !ai::overlays.Contains(loc)) {
        out = loc;
        found = true;
    } else {
        cell = 0;
        int counts[8] = {0};
        int n = 0;

        for (int i = 0; i < 8; i++) {
            CAiMapLoc next;

            if (_traversable_step(map, loc, (CAiMap::DIR)i, next, cell) && !ai::overlays.Contains(cell, next)) {
                if (n++ == 0)
                    out = next;
                counts[i] = 1;
                n++;
            }
        }
        if (n == 1) {
            found = true;
        } else if (n > 1) {
            int dir = -1;
            int best = -1;

            for (int i = 0; best < 3 && i < 8; i++) {
                if (counts[i] > 0) {
                    if (counts[(CAiMap::DIR)((i + 15) % 8)] > 0)
                        counts[i]++;
                    if (counts[(CAiMap::DIR)((i + 9) % 8)] > 0)
                        counts[i]++;
                    if (best < counts[i]) {
                        best = counts[i];
                        dir = i;
                    }
                }
            }
            found = _traversable_step(map, loc, (CAiMap::DIR)dir, out, cell) && !ai::overlays.Contains(cell, out);
        }
    }
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
