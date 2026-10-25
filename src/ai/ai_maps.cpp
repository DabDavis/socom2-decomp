/* ai_maps.cpp (OVL2 0x5291F0-0x52EE9F, SOCOM 1's unit: __sinit_ai_maps.cpp, OVL2 0x668710; the
 * demo's 0x196E60-0x19DC9F): CAiMaps, a mission's maps -- the lookups that find a cell's map by
 * its index and the name lookups that ask every map in turn, the path registration, the map set's
 * reading and its tables. SOCOM 1's functions in the demo's order (SetOccupied ... GetLocation,
 * GetMapByIndex, Reset, Close, FreeTables, ~CAiMaps); the template tail is _max<Ui>,
 * _close_enough<f> (0x52EB60), remove_copy<CAiMapLoc> (0x52EBA0) and the names' reserve
 * (0x52EC60). Defined here in reverse of the image's order (-inline deferred). */

#include "ai_maps.h"

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

CAiMap *CAiMaps::GetMap(const CAiMapLoc &loc) const
{
    return GetMapByIndex(loc.m_mapid);
}

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
