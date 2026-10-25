/* ai_maptable.cpp (OVL2 0x52EEA0-, the name ours): CAiMap's tables -- the graph, the routes, the
 * readers, the regions, the links, the actions, the names and the per-cell extra data. SOCOM 1's
 * functions in the demo's order (0x19DCA0-0x1A045F, after ai_maps.cpp's template tail; no DWARF
 * names their file). Defined here in reverse of the image's order (-inline deferred). The tables'
 * walks test `i != end` through the vector's class iterators, so each test is materialised (xor,
 * sltiu, xori) before its branch, where SOCOM 1's compared pointers. */

#include <string.h>

#include "ai.h"

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

/* the action on a cell, or none (SOCOM 1's: the table walked for the cell) */
CAiMap::action *CAiMap::GetCellAction(const CAiMapLoc &loc)
{
    actions::iterator e = m_actionTable.end();

    for (actions::iterator i = m_actionTable.begin(); i != e; ++i)
        if ((*i).m_loc == loc)
            return &*i;
    return 0;
}

/* the region whose corner is the cell (SOCOM 1's GetRegion(const CAiMapLoc &)) */
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
 * OPEN, 2 instructions short (0x138 against 0x140): the target keeps `this` and the size times
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

/* the graph given up, its flag cleared (SOCOM 1's) */
void CAiMap::DestroyGraph()
{
    CGraph::Destroy(m_graph);
    m_graph = 0;
    m_flags &= ~1;
}
