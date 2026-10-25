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

/* MSL's sort, declared only: its instance for the names closes the unit (0x5312D0) */
namespace std {
template <class RandomAccessIterator, class Compare>
void sort(RandomAccessIterator first, RandomAccessIterator last, Compare comp);
}

/* two names in order: case-blind over their fifteen characters (the names' sort's comparator,
 * 0x531190; the name ours) */
static bool _name_less(const CAiMap::name &a, const CAiMap::name &b)
{
    return func_001993C0(a.m_name, b.m_name, 15) < 0;
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
 * OPEN, right size, 21 instructions in the first block: scheduling only -- the target loads x and
 * m_dx for the width before it stores y - top (SOCOM 1's build has the same order), this build
 * after; the width and height spelled five ways, the four declared first and assigned x, w, y, h,
 * and one declaration of all four were tried */
void CAiMap::region::InflateRegion(int left, int top, int right, int bottom, const CAiMap *map)
{
    int x = m_loc.x - left;
    int y = m_loc.y - top;
    int w = right + (m_loc.x + m_dx) - x;
    int h = bottom + (m_loc.y + m_dy) - y;
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
 * OPEN, right size, 9 instructions: registers only -- the target walks in t2 with the end in t0
 * and the id read into t1, this build t0, t1, t2; the iterators declared together, apart, the
 * end first, the id hoisted to the top were tried */
void CAiMap::spores::GetRange(unsigned int id, range &r)
{
    int first = -1;
    int last = -1;
    int n = 0;
    iterator e = end();

    for (iterator i = begin(); i != e; ++i, ++n) {
        unsigned int sid = (*i).m_id;

        if (sid < id)
            continue;
        if (first == -1)
            first = n;
        if (id < sid) {
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

/* whether a route is marked invalid (SOCOM 1's remove_if predicate for ValidateRoutes) */
bool _invalid_route(CAiMap::route &r)
{
    return (r.m_flags & 1) != 0;
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

/* the graph given up, its flag cleared (SOCOM 1's) */
void CAiMap::DestroyGraph()
{
    CGraph::Destroy(m_graph);
    m_graph = 0;
    m_flags &= ~1;
}
