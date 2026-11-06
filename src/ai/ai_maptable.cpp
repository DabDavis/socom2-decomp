/* ai_maptable.cpp (OVL2 0x52EEA0-, the name ours): CAiMap's tables -- the graph, the routes, the
 * readers, the regions, the links, the actions, the names and the per-cell extra data. SOCOM 1's
 * functions in the demo's order (0x19DCA0-0x1A045F, after ai_maps.cpp's template tail; no DWARF
 * names their file). Defined here in reverse of the image's order (-inline deferred). The tables'
 * walks test `i != end` through the vector's class iterators, so each test is materialised (xor,
 * sltiu, xori) before its branch, where SOCOM 1's compared pointers. */

#include <string.h>

#include "ai.h"

/* ai.h's _min defined (as ai_map.cpp does: never inlined, the int instance closes this unit,
 * 0x5311C0) */
namespace ai {
template <class T>
const T &_min(const T &a, const T &b)
{
    return a <= b ? a : b;
}
}

/* (MSL's sort is include/algorithm's: its instance for the names, with its median-of-three 0x5321B0
 * and short-range sort 0x5329F0, closes the unit -- 0x5312D0) */

/* two names in order: case-blind over their fifteen characters (the names' sort's comparator,
 * 0x531190; the name ours) */
static bool _name_less(const CAiMap::name &a, const CAiMap::name &b)
{
    return strncasecmp(a.m_name, b.m_name, 15) < 0;
}

/* two cells' x and y distances summed (the name ours) */
inline int loc_dist(const CAiMapLoc &a, const CAiMapLoc &b)
{
    int dx = b.x > a.x ? b.x - a.x : a.x - b.x;
    int dy = b.y > a.y ? b.y - a.y : a.y - b.y;

    return dx + dy;
}

/* the extra data given up (SOCOM 1's) */
void CAiMap::FreeEx()
{
    delete[] m_mapex;
    m_mapex = 0;
    m_mapex_bytes = 0;
}

/* the extra data: bytes per cell, zeroed, the old block given up first (SOCOM 1's; SOCOM II no
 * longer points each scanline into it). The count converted to int: only so does MW load it
 * before the bytes and multiply bytes by it, as the target does */
char *CAiMap::AllocEx(unsigned int bytes)
{
    delete[] m_mapex;
    m_mapex = 0;
    m_mapex_bytes = 0;
    if (bytes) {
        m_mapex_bytes = bytes;
        int n = m_mapex_bytes * (int)m_cellcount;

        m_mapex = new char[n];
        memset(m_mapex, 0, n);
    }
    return m_mapex;
}

/* the cell's extra data, or none: no data, a cell off the map or off its row's run (SOCOM 1's;
 * SOCOM II's rows keep an index into the data where SOCOM 1's kept a pointer) */
char *CAiMap::GetMapEx(const CAiMapLoc &loc)
{
    if (m_mapex) {
        if (IsCell(loc)) {
            const scanline &s = m_scanlines[(unsigned short)loc.y];
            int i = s.GetIndex(loc.x);

            if (i > -1)
                return m_mapex + i * m_mapex_bytes;
        }
    }
    return 0;
}

/* the name ended at its last byte (SOCOM 1's, between GetCellName and GetMapEx in both builds) */
void CAiMap::name::ValidateName()
{
    m_name[15] = 0;
}

/* the name of a cell, or none (SOCOM 1's) */
CAiMap::name *CAiMap::GetCellName(const CAiMapLoc &loc)
{
    names::iterator e = m_namesTable.end();

    for (names::iterator i = m_namesTable.begin(); i != e; ++i)
        if ((*i).m_loc == loc)
            return &*i;
    return 0;
}

void CAiMap::names::Sort()
{
    std::sort(begin(), end(), _name_less);
}

void CAiMap::name::SetName(const char *s)
{
    strncpy(m_name, s, 15);
    ValidateName();
}

/* the cell of a name, when the map has it (SOCOM 1's) */
bool CAiMap::GetLocation(const char *s, CAiMapLoc &loc)
{
    bool found = false;

    if (s) {
        name *n = FindName(s);

        if (n) {
            found = true;
            loc = n->m_loc;
        }
    }
    return found;
}

/* GetCellAction's twin (new in SOCOM II, the same code; the const overload is our guess) */
CAiMap::action *CAiMap::GetCellAction(const CAiMapLoc &loc) const
{
    actions &t = const_cast<actions &>(m_actionTable);
    actions::iterator e = t.end();

    for (actions::iterator i = t.begin(); i != e; ++i)
        if ((*i).m_loc == loc)
            return &*i;
    return 0;
}

/* the cell of an action, by its id, and the action when asked (SOCOM 1's; the id read into a
 * local, the only spelling of the compare that puts the field first in its bne) */
bool CAiMap::FindActionLocation(unsigned int id, CAiMapLoc &loc, action **pa)
{
    actions::iterator e = m_actionTable.end();

    for (actions::iterator i = m_actionTable.begin(); i != e; ++i) {
        unsigned int aid = (*i).m_actionId;

        if (aid == id) {
            if (pa)
                *pa = &*i;
            loc = (*i).m_loc;
            return true;
        }
    }
    return false;
}

/* the action on a cell, or none (SOCOM 1's: the table walked for the cell) */
CAiMap::action *CAiMap::GetCellAction(const CAiMapLoc &loc)
{
    actions::iterator e = m_actionTable.end();

    for (actions::iterator i = m_actionTable.begin(); i != e; ++i)
        if ((*i).m_loc == loc)
            return &*i;
    return 0;
}

/* a cell's link set or dropped: a set link made when the cell has none, the cell made walkable
 * when it was not and marked linked; a dropped one unmarked (new in SOCOM II). The cell read
 * through GetCell, inlined one level, the links' inlines too */
CAiMap::link *CAiMap::SetCellLink(const CAiMapLoc &loc, bool set)
{
    CAiCell *cell = GetCell(loc, 0);
    link *l = 0;

    if (!set) {
        m_linksTable.Remove(loc);
        cell->SetLink(0);
    } else {
        l = m_linksTable.Add(loc);
        if (l) {
            if (!cell->IsTraversable())
                cell->m_soil = SOIL_DEFAULT;
            cell->SetLink(1);
        }
    }
    SetModified(true);
    return l;
}

/* the link from a cell, or none (SOCOM 1's) */
CAiMap::link *CAiMap::GetCellLink(const CAiMapLoc &loc)
{
    links::iterator e = m_linksTable.end();

    for (links::iterator i = m_linksTable.begin(); i != e; ++i)
        if ((*i).m_loc == loc)
            return &*i;
    return 0;
}

/* the region whose corner is the cell (SOCOM 1's GetRegion(const CAiMapLoc &)) */
/* the shorter leg: from the cell to the link's end on its map, or from the other end to the goal
 * (new in SOCOM II) */
int CAiMap::link::GetDistance(const CAiMapLoc &from, const CAiMapLoc &to) const
{
    if (from.m_mapid == m_loc.m_mapid)
        return ai::_min(loc_dist(m_loc, from), loc_dist(m_link, to));
    return ai::_min(loc_dist(m_link, from), loc_dist(m_loc, to));
}

CAiMap::region *CAiMap::GetRegion(const CAiMapLoc &loc)
{
    regions::iterator e = m_regionTable.end();

    for (regions::iterator i = m_regionTable.begin(); i != e; ++i)
        if ((*i).m_loc == loc)
            return &*i;
    return 0;
}

/* the region of that name, its corner handed back (SOCOM 1's; no name, no region) */
CAiMap::region *CAiMap::GetRegion(const char *name, CAiMapLoc &loc)
{
    if (name) {
        for (regions::iterator i = m_regionTable.begin(); i != m_regionTable.end(); ++i) {
            if (!strcasecmp((*i).m_name, name)) {
                loc = (*i).m_loc;
                return &*i;
            }
        }
    }
    return 0;
}

/* the first region of a kind (ANY: every kind) that holds the cell, the hidden ones skipped when
 * asked (SOCOM 1's; SOCOM II's Contains tests the map too, and the hidden bit is materialised).
 * OPEN, 2 instructions short (0x138 against 0x140; unchanged at -inline level=5): the target keeps `this` and the size times
 * 28 across the loop and reloads data_ for each pass's end test; this build computes the end
 * once. A const_iterator, the iterator declared outside the for, the end spelled through begin()
 * + size() or &table[0] + size(), and the body as an if-block were tried */
CAiMap::region *CAiMap::GetRegionContaining(const CAiMapLoc &loc, region::TYPE type, bool displayed)
{
    for (regions::iterator i = m_regionTable.begin(); i != m_regionTable.end(); ++i) {
        if (displayed && !(*i).IsDisplayed())
            continue;
        if ((type == region::ANY || type == (*i).m_type) && (*i).Contains(loc))
            return &*i;
    }
    return 0;
}

/* the rectangle grown by so many cells on each side, kept inside the map (or inside the largest
 * map when none is given: 0x1FFF cells) -- SOCOM 1's, the corner clamped first, then the size.
 * Byte-exact in the AI core's round 29 (21 instructions off since round 2): the grown right and
 * bottom edges are locals of their own, each computed right after its corner -- the edge's field
 * loads then lead the next corner's store (x, the right edge, y, the bottom edge), as the target's
 * do; the width and height written straight from the fields kept those loads below the store */
void CAiMap::region::InflateRegion(int left, int top, int right, int bottom, const CAiMap *map)
{
    int x = m_loc.x - left;
    int r = right + (m_loc.x + m_dx);
    int y = m_loc.y - top;
    int b = bottom + (m_loc.y + m_dy);
    int w = r - x;
    int h = b - y;
    int maxx = 0x1FFF;
    int maxy = 0x1FFF;

    if (map) {
        maxx = map->m_nx - 1;
        maxy = map->m_ny - 1;
    }
    _saturate(&x, 0, maxx);
    m_loc.x = x;
    _saturate(&y, 0, maxy);
    m_loc.y = y;
    int mw = maxx - m_loc.x;
    _saturate(&w, 0, mw);
    m_dx = w;
    int mh = maxy - m_loc.y;
    _saturate(&h, 0, mh);
    m_dy = h;
}

/* the rectangle grown to take in the cell (SOCOM 1's; the locals declared in this order for the
 * target's registers) */
void CAiMap::region::Union(const CAiMapLoc &loc)
{
    bool grow = false;
    int bottom = 0;
    int right = 0;
    int top = 0;
    int left = 0;

    if (loc.x < m_loc.x) {
        left = m_loc.x - loc.x;
        grow = true;
    } else if (m_loc.x + m_dx < loc.x) {
        right = loc.x - (m_loc.x + m_dx);
        grow = true;
    }
    if (loc.y < m_loc.y) {
        top = m_loc.y - loc.y;
        grow = true;
    } else if (m_loc.y + m_dy < loc.y) {
        bottom = loc.y - (m_loc.y + m_dy);
        grow = true;
    }
    if (grow)
        InflateRegion(left, top, right, bottom, 0);
}

/* the run of an id's spores: the first at or past the id and the last before the next id, or
 * -1s and no count (new in SOCOM II, called from two CAiMaps functions -- 0x52B5C0, 0x52B7E0).
 * The walk's end and the spore's id temporaries (end() in the test, hoisted; the id read twice,
 * shared), numbered after the iterator: the target walks in t2 with the end in t0 and the id in t1
 * -- as named locals they rotate to t0, t1, t2 (the AI core round 19; 9 instructions off before) */
void CAiMap::spores::GetRange(unsigned int id, range &r)
{
    int first = -1;
    int last = -1;
    int n = 0;

    for (iterator i = begin(); i != end(); ++i, ++n) {
        if ((*i).m_id < id)
            continue;
        if (first == -1)
            first = n;
        if (id < (*i).m_id) {
            last = n - 1;
            break;
        }
    }
    r.m_id = id;
    if (first == -1) {
        r.m_first = -1;
        r.m_last = -1;
        r.m_count = 0;
        return;
    }
    r.m_first = first;
    if (last == -1)
        last = size() - 1;
    r.m_last = last;
    r.m_count = r.m_last - r.m_first + 1;
}

/* two cells in order, by map, then x, then y (the name ours) -- as the target computes it: the y
 * difference is taken when the x's DIFFER and the x difference (none) when they agree, the map's
 * difference last in the code (an `==` where `!=` was meant, it seems; its one caller tests for 0) */
inline int loc_compare(const CAiMapLoc &a, const CAiMapLoc &b)
{
    int r;

    if (a.m_mapid == b.m_mapid) {
        if (a.x == b.x)
            r = a.x - b.x;
        else
            r = a.y - b.y;
    } else
        r = a.m_mapid - b.m_mapid;
    return r;
}

/* the first line with cells whose first cell is the one asked for (new in SOCOM II, the name ours:
 * CTMap's Tick on the cell's map, 0x52FD90). The compare as one result local over nested if/else
 * (only that order lays the x-equal arm first and the map's arm last, with the target's delay
 * slots); the found line through a result local (the target's zero in the first test's slot).
 * The target loads begin() first (the iterator in v1, the end in a3, the first cell in t0): end()
 * in the loop test (a hoisted temporary) and the first cell read as `(*i)[0]` -- through begin()
 * the end takes t0 and the cell a3; with an end local the end comes first (the AI core round 19;
 * 23 instructions off before) */
CAiMap::line *CAiMap::lines::Find(const CAiMapLoc &loc)
{
    line *r = 0;

    for (iterator i = begin(); i != end(); ++i) {
        if ((*i).size() && !loc_compare((*i)[0], loc)) {
            r = &*i;
            break;
        }
    }
    return r;
}

/* a reader with no script (SOCOM 1's, out of line in both builds: resize<reader> calls it) */
CAiMap::reader::reader() : m_reader(0), m_flags(0), m_bytes(0)
{
}

/* the rectangle's middle cell (SOCOM 1's) */
CAiMapLoc &CAiMap::region::GetCenter(CAiMapLoc &loc) const
{
    loc.x = m_loc.x + (m_dx >> 1);
    loc.y = m_loc.y + (m_dy >> 1);
    return loc;
}

/* every reader's script freed (SOCOM 1's; the vector's own teardown follows) */
CAiMap::readers::~readers()
{
    for (iterator i = begin(); i != end(); ++i)
        (*i).SetReader(0);
}

/* SOCOM 1's */
void CAiMap::reader::SetReader(CRdrFile *r)
{
    if (r != m_reader) {
        zrdr_free(m_reader);
        m_reader = r;
    }
}

/* the region a route's end lies on (SOCOM 1's FindByLocation<regions, region>, inlined here) */
inline CAiMap::region *find_region(CAiMap::regions &t, const CAiMapLoc &loc)
{
    CAiMap::regions::iterator e = t.end();

    for (CAiMap::regions::iterator i = t.begin(); i != e; ++i)
        if ((*i).m_loc == loc)
            return &*i;
    return 0;
}

/* the points' emptiness read through count()'s reference, and a finished path with points tested
 * through it (ReRoute's test: the size's address kept in a saved register for the test, the resize
 * and the copy's first test, the state's compare materialised into a bool -- HasPoints() reads the
 * size straight and branches; the names ours) */
inline bool points_empty(CAiPath::points &v)
{
    return v.count() == 0;
}

/* none of the planner's three states, read through a reference (ReRoute's wait loop: the round 30) */
inline bool idle(const CAiPath::PATH_STATE &s)
{
    return (unsigned int)(s - 1) > 2;
}

inline bool path_found(CAiPath &p)
{
    return !points_empty(p.m_points) && p.m_state == CAiPath::COMPLETE;
}

/* a route's path rebuilt between its way points: marked invalid first, refused when either region is
 * gone, then (two way points or more) a path planned leg by leg through the planner and run to its
 * end; a finished path with points makes the route valid again, its cells copied last to first
 * (SOCOM 1's, 0x19E320; ValidateRoutes calls it when asked). True for a valid route.
 * Byte-exact since the AI core's round 32 (10 off in round 31): the wait loop's state read through a
 * reference local (s1 ahead of the loop, its two tests and path_found's reload through it -- MW
 * shares no load of a reference's field with the object's own) and tested as !idle() (both tests on
 * `sltiu at`; `< 3` materialised the bottom one); the copy loop a do-while under a guard written by
 * hand (`0 < (int)count()`: the guard reads the saved s0 the emptiness test and the resize share,
 * the guard's locals -- the counter, then the count's reference the body and the bottom test read --
 * go into the preheader after it: the weapons' round-30 rule), the counter declared ahead of the
 * reference (t0 / a3 the target's way round), the step in the bottom test (`while (++j < ...)`; a
 * separate `j++;` is 0x340). The regions' finds compute their end once (find_region's `e`, SOCOM 1's
 * FindByLocation shape): written through end() at each pass MW reloaded data_ in both walks here
 * (ValidateRoutes unchanged either way) */
bool CAiMap::ReRoute(route *r)
{
    CAiMapLocVec &w = r->m_waypts;
    int n = w.size();

    r->m_flags |= 1;
    if (!find_region(m_regionTable, r->m_region[0]) || !find_region(m_regionTable, r->m_region[1]))
        return false;
    if (n >= 2) {
        CAiPath path;

        if (ai::PathPlanBegin(&path)) {
            int legs = n - 1;
            int i;

            for (i = 0; i < legs; i++)
                if (!ai::PathPlanAddLeg(this, w[i], w[i + 1]))
                    break;
            if (legs != i) {
                ai::PathPlanAbort();
            } else {
                ai::PathPlanExecute();
                const CAiPath::PATH_STATE &state = path.m_state;

                while (!idle(state))
                    ai::TickPathPlanner(1.0f / 30.0f, 0);
                if (path_found(path)) {
                    r->m_flags &= ~1;
                    r->m_path.clear();
                    r->m_path.resize(path.m_points.count());
                    if (0 < (int)path.m_points.count()) {
                        int j = 0;
                        unsigned int &cnt = path.m_points.count();

                        do {
                            r->m_path[j] = path.m_points[cnt - j - 1].m_loc;
                        } while (++j < (int)cnt);
                    }
                }
            }
        }
    }
    return (r->m_flags & 1) == 0;
}

/* whether a route is marked invalid (SOCOM 1's remove_if predicate for ValidateRoutes) */
bool _invalid_route(CAiMap::route &r)
{
    return (r.m_flags & 1) != 0;
}

/* the routes checked against the regions: one already invalid is skipped; one whose two regions are
 * not both found, or whose first way point is not inside its first region of kind 4 or whose last is
 * not inside its second, is marked invalid; the others are rerouted when asked; then the invalid
 * ones are dropped, the end taken once for both remove_if and the erase (SOCOM 1's; SOCOM II's
 * Contains tests the map too). Written in the AI core's round 24 for its instances: remove_if
 * 0x5311E0 and remove_copy_if 0x5328E0 byte-exact through it.
 * THE AI CORE'S ROUND 33 (0x3FC -> 0x430 of 0x438; structurally 2 off, both nops, and 34
 * instructions in their registers alone): the loop's iterator declared ahead, ASSIGNED from begin(),
 * and REUSED AS THE END for the tail (`i = end(); r = remove_if(begin(), i, p); erase(r, i);`) -- the
 * end's copy into erase's parameter takes i's address, so i lives in memory (92(sp), reloaded at
 * each use: the target's loop word for word), and its second range is split off to the frame's
 * bottom, giving the target's tail slots exactly (i 92, result 88, begin 84, erase's copies 80/76,
 * the end 72); assigned alone, or initialised, the class vector's iterator is scalarised. The last
 * way point read as `(&w[0] + w.size())[-1]`: through operator[] its data pointer is CSE'd with
 * [0]'s (the target's shared v0), and the end-then-minus-one arithmetic is back()'s (`w[size - 1]`
 * indexes as (size-1)*4; back(), end()-1, data()+size-1 reload data_).
 * OPEN: the target's second region find reads loc.x through a FRESH load of i into a2 (hoisted to
 * its preheader, after the empty test: hence its two nops) and loc.y through the loop-top load
 * still in a1; this build CSEs both into one second load (a0), and takes a0/a1 the other way round
 * in the first find (`igreplay --move 35=36.5` -- the first find's result numbered after the second
 * load -- gives the target's a0/a1, but not the split). Tried: -> for each access, a reference
 * to m_region[1] (the first 65 instructions exact, the second find through the reference), the
 * find through a result local, end() per pass, a while, i then e, the two results declared
 * uninitialised, at the top, or with dead `= 0` definitions (build/scratch/ai/v_vr*.py).
 * THE AI CORE'S ROUND 34 (0x430 -> 0x438, the target's size; difflib distance 48 -> 36, every
 * difference inside the second find): the second find reaches the far end through the loop's
 * iterator itself, its x and y compared field by field (find_far_region below) -- the first find,
 * the empty test's delay slot and both of the second find's nops are now the target's; left: that
 * find's x still shares its fresh load of i with y (a3: `lhu a1,10(a3)`) where the target reads y
 * off the loop-top load (`lhu a2,10(a1)`), so the find's temporaries take a1 where the target's
 * take a2. Measured there (build/scratch/ai/v_vr34*.py): the end passed as a const CAiMapLoc
 * pointer or reference, by value (0x4DC), the route by reference / pointer / const reference /
 * iterator by value, the iterator's operator->, `loc == (*j).m_loc` swapped, both finds through the
 * iterator (94), the y compare's operands swapped, `x ? y : false`: none splits the load.
 * The AI core round 35, from the PowerPC front end's optimiser (doldecomp/mweppc IrOptimizer.c,
 * IroCSE.c; build/scratch/ai/mweppc/ -- read, not yet proven on mwcps2): pass 0 hoists loop
 * invariants (IRO_FindLoops) BEFORE common subexpressions (IRO_FindExpressions, ComputeAvail,
 * CommonSubs), and CheckCommonSub gives a use the FIRST available identical expression in
 * FindExpressions' order (flowgraph node by node). The far x is the second loop's unconditional
 * test, so it is hoisted with its own load of i; y, behind the x test, stays in the loop -- the
 * target's y then took the loop top's load (a1), this build's the hoisted x's: in the target the
 * hoisted load is not the loop top's expression (or not available there), here the second find's
 * loads are not the loop top's */
inline CAiMap::region *find_far_region(CAiMap::regions &t, CAiMap::routes::iterator &r)
{
    CAiMap::regions::iterator e = t.end();

    for (CAiMap::regions::iterator j = t.begin(); j != e; ++j)
        if (((*j).m_loc.x == (*r).m_region[1].x && (*j).m_loc.y == (*r).m_region[1].y) ? true : false)
            return &*j;
    return 0;
}

void CAiMap::ValidateRoutes(bool force)
{
    routes::iterator i;

    for (i = m_routeTable.begin(); i != m_routeTable.end(); ++i) {
        if ((*i).m_flags & 1)
            continue;
        region *a = find_region(m_regionTable, (*i).m_region[0]);
        region *b = find_far_region(m_regionTable, i);

        if (!a || !b) {
            (*i).m_flags |= 1;
            continue;
        }
        if (!a->Contains((*i).m_waypts[0]) || a->m_type != 4) {
            (*i).m_flags |= 1;
            continue;
        }
        if (!b->Contains((&(*i).m_waypts[0] + (*i).m_waypts.size())[-1]) || b->m_type != 4) {
            (*i).m_flags |= 1;
            continue;
        }
        if (force)
            ReRoute(&*i);
    }
    i = m_routeTable.end();
    routes::iterator r = std::remove_if(m_routeTable.begin(), i, _invalid_route);

    m_routeTable.erase(r, i);
}

/* the route of an index, or none (new in SOCOM II, the name ours) */
CAiMap::route *CAiMap::GetRoute(unsigned int i)
{
    return i < m_routeTable.size() ? &m_routeTable[i] : 0;
}

/* a route's index in the table, or -1 (new in SOCOM II, the name ours).
 * OPEN, 2 instructions short (0x68 against 0x70; unchanged at -inline level=5): GetRegionContaining's difference -- the target
 * reloads data_ for each pass's end test (the size times 0x24 hoisted) though the loop holds no
 * call and no store; this build computes the end once. Postfix steps, a while, the count stepped
 * in the body, a const_iterator, a raw pointer walk, !(i == end()) and the end built from a
 * reference-returning data() were tried: perhaps include/vector's end() */
int CAiMap::GetRouteIndex(const route *r)
{
    int n = 0;

    for (routes::iterator i = m_routeTable.begin(); i != m_routeTable.end(); ++i, ++n)
        if (&*i == r)
            return n;
    return -1;
}

/* the map's graph built unless it has one and no rebuild is asked: a node per region of kind 4,
 * an edge per route between its two regions' nodes, the edge's length the route's path's and its
 * route kept with it (SOCOM 1's; the old graph given up first, none for no routes) */
void CAiMap::CreateGraph(bool force)
{
    if (m_graph && !force)
        return;
    DestroyGraph();
    if (m_routeTable.size() == 0) {
        DestroyGraph();
        return;
    }
    CGraph *g = CGraph::Create("AiMap Graph");

    for (regions::iterator r = m_regionTable.begin(); r != m_regionTable.end(); ++r) {
        if ((*r).m_type == 4) {
            /* SOCOM II's CreateNode takes no name (OVL1 0x2D0930, CreateNode__6CGraphFv) */
            CGNode *node = g->CreateNode();

            ((CAiMap::region **)node)[2] = &*r;
        }
    }
    for (routes::iterator rt = m_routeTable.begin(); rt != m_routeTable.end(); ++rt) {
        if (!((*rt).m_flags & 1)) {
            region *a = find_region(m_regionTable, (*rt).m_region[0]);
            region *b = find_region(m_regionTable, (*rt).m_region[1]);
            CGEdge *edge = g->CreateEdge(g->GetNodeByNodeEx(a), g->GetNodeByNodeEx(b));

            ((float *)edge)[1] = (*rt).m_path.ComputeLength();
            ((route **)edge)[5] = &*rt;
        }
    }
    m_graph = g;
    if (m_graph)
        SetFlags(1, 0);
}

/* the graph given up, its flag cleared (SOCOM 1's) */
void CAiMap::DestroyGraph()
{
    CGraph::Destroy(m_graph);
    m_graph = 0;
    m_flags &= ~1;
}
