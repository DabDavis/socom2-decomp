/* ai_maps.cpp (OVL2 0x5291F0-0x52EE9F, SOCOM 1's unit: __sinit_ai_maps.cpp, OVL2 0x668710; the
 * demo's 0x196E60-0x19DC9F): CAiMaps, a mission's maps -- the lookups that find a cell's map by
 * its index and the name lookups that ask every map in turn, the path registration, the map set's
 * reading and its tables. SOCOM 1's functions in the demo's order (SetOccupied ... GetLocation,
 * GetMapByIndex, Reset, Close, FreeTables, ~CAiMaps); the template tail is _max<Ui>,
 * _close_enough<f> (0x52EB60), remove_copy<CAiMapLoc> (0x52EBA0) and the names' reserve
 * (0x52EC60). Defined here in reverse of the image's order (-inline deferred). */

#include "ai_maps.h"

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

/* the maps closed: the planner's queue flushed first, the cookie bumped last (SOCOM 1's) */
void CAiMaps::Close()
{
    ai::FlushPlannerQueue();
    FreeMaps();
    ai::ClearOverlays();
    m_cookie++;
}

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
