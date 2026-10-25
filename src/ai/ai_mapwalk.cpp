/* ai_mapwalk.cpp (OVL2 ~0x5350C0-0x539BBF, the name ours): the AI map's walkers -- SOCOM 1's unit
 * after vehicle_rdr.cpp (the demo's 0x1A5400-0x1A9180: CountMapTravelDir, CountMapVisDir, Tile and
 * its callbacks, FloodFill, the two WalkLines, ComputeLineVisibility, CanSeeLine x2, CanTravelLine,
 * the line callbacks, GetLocationWidth, then the template tail with _saturate<f> and the CAiMapLoc
 * list's pieces -- OVL2 0x539B70 on). Only the line tests so far (0x539070-0x5390CF). Defined
 * here in reverse of the image's order (-inline deferred). */

#include "ai.h"

/* what Tile hands its callbacks: the value to set, the soils a soil tile spares, and whether any
 * cell changed (SOCOM 1's Tile's data; the names ours) */
struct TILE_DATA {
    unsigned int m_value;   // 0x0: read as a byte by the boundary's
    unsigned int m_flags;   // 0x4
    bool m_changed;   // 0x8
};

/* a cell's shadow, boundary and occupied bits set from the tile's value, the change noted (SOCOM
 * 1's _tile_boundary; the shadow's and the occupied bit's new in SOCOM II) */
/* a cell's soil set from the tile's value where it may be: anywhere (flag 1) or over the default
 * soil, and over steep ground with flag 2 (SOCOM 1's _tile_soil, the steep case new). The two
 * bools assigned true and cleared, the steep test materialised into a bool of its own, the flags
 * declared after the bools: each measured.
 * OPEN, right size, 7 instructions: registers only -- the target keeps the old soil in a2 and the
 * flags in a0, here the other way round (declaration orders of soil/old, cell/t, set/any and the
 * comparison's operand order tried) */
static bool _tile_soil(const MAP_FOREACH_STRUCT &s, void *data)
{
    TILE_DATA *t = (TILE_DATA *)data;
    CAiCell *cell = s.m_cell;
    unsigned char soil = t->m_value;
    unsigned int old = cell->GetSoil();

    if (old != soil) {
        bool set = true;
        bool any = true;
        unsigned int flags = t->m_flags;

        if (!(flags & 1) && old != CAiMap::SOIL_DEFAULT)
            any = false;
        if (!any) {
            bool steep = (flags & 2) != 0 && old == CAiMap::SOIL_STEEP;

            if (!steep)
                set = false;
        }

        if (set) {
            cell->m_soil = soil;
            t->m_changed = true;
        }
    }
    return true;
}

static bool _tile_shadow(const MAP_FOREACH_STRUCT &s, void *data)
{
    TILE_DATA *t = (TILE_DATA *)data;
    CAiCell *cell = s.m_cell;
    unsigned int shadow = t->m_value;

    if (shadow != cell->GetShadow()) {
        cell->SetShadow(shadow & 3);
        t->m_changed = true;
    }
    return true;
}

static bool _tile_boundary(const MAP_FOREACH_STRUCT &s, void *data)
{
    TILE_DATA *t = (TILE_DATA *)data;
    CAiCell *cell = s.m_cell;
    unsigned char boundary = t->m_value;

    if (boundary != cell->GetBoundary()) {
        cell->SetBoundary(boundary);
        t->m_changed = true;
    }
    return true;
}

static bool _tile_occupied(const MAP_FOREACH_STRUCT &s, void *data)
{
    TILE_DATA *t = (TILE_DATA *)data;
    CAiCell *cell = s.m_cell;
    bool occupied = t->m_value != 0;

    if (occupied != cell->IsOccupied()) {
        cell->SetOccupied(occupied);
        t->m_changed = true;
    }
    return true;
}

/* the line walks' callbacks (SOCOM 1's _SeeLineCallback and _TravelLineCallback, static here:
 * placeholders until written) */
extern "C" bool func_00539400(const MAP_LINEWALK_STRUCT &s, void *data);
extern "C" bool func_00539580(const MAP_LINEWALK_STRUCT &s, void *data);

bool CAiMap::CanTravelLine(const CAiMapLoc &from, const CAiMapLoc &to, CAiMapLoc *out)
{
    return WalkLine(from, to, func_00539580, out, 0.2f);
}

bool CAiMap::CanSeeLine(const CAiMapLoc &from, const CAiMapLoc &to, CAiMapLoc *out)
{
    return WalkLine(from, to, func_00539400, out, 0.2f);
}

bool CAiMap::CanSeeLine(const CAiMapLoc &from, const CPnt3D *dir, CAiMapLoc *out)
{
    return WalkLine(from, dir, func_00539400, out, 0.2f);
}
