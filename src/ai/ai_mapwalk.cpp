/* ai_mapwalk.cpp (OVL2 ~0x5350C0-0x539BBF, the name ours): the AI map's walkers -- SOCOM 1's unit
 * after vehicle_rdr.cpp (the demo's 0x1A5400-0x1A9180: CountMapTravelDir, CountMapVisDir, Tile and
 * its callbacks, FloodFill, the two WalkLines, ComputeLineVisibility, CanSeeLine x2, CanTravelLine,
 * the line callbacks, GetLocationWidth, then the template tail with _saturate<f> and the CAiMapLoc
 * list's pieces -- OVL2 0x539B70 on). Only the line tests so far (0x539070-0x5390CF). Defined
 * here in reverse of the image's order (-inline deferred). */

#include "ai_maps.h"

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

/* what a visibility walk's callbacks read and add to: the line's two heights, the visibility
 * lost along it and the caller's flags (SOCOM 1's took the visibility and two ints; the names
 * ours) */
struct VIS_LINE_DATA {
    float m_y0;   // 0x0
    float m_y1;   // 0x4
    float m_vis;   // 0x8
    unsigned int m_flags;   // 0xc
};

/* the two visibility callbacks (SOCOM 1's _VisLineCallback; flag 1 picks the second) */
extern "C" bool func_005390D0(const MAP_LINEWALK_STRUCT &s, void *data);
extern "C" bool func_00539240(const MAP_LINEWALK_STRUCT &s, void *data);

/* SOCOM 1's, each end at its cell's altitude raised by a height: what the walk lost, held in
 * [0, 1], taken from 1 -- none when an end has no altitude or the walk stops */
float CAiMap::ComputeLineVisibility(const CAiMapLoc &from, const CAiMapLoc &to, unsigned int flags, float hfrom,
                                    float hto)
{
    VIS_LINE_DATA d;

    d.m_vis = 0.0f;
    d.m_flags = flags;
    float vis = 0.0f;

    if (GetAltitude(from, d.m_y0) && GetAltitude(to, d.m_y1)) {
        d.m_y0 += hfrom;
        d.m_y1 += hto;
        if (WalkLine(from, to, (flags & 1) ? func_00539240 : func_005390D0, &d, 0.2f)) {
            vis = d.m_vis;
            _saturate(&vis, 0.0f, 1.0f);
            vis = 1.0f - vis;
        }
    }
    return vis;
}

/* between two points over the maps (new in SOCOM II) */
float CAiMaps::ComputeLineVisibility(const CPnt3D *from, const CPnt3D *to, unsigned int flags)
{
    VIS_LINE_DATA d;

    d.m_vis = 0.0f;
    d.m_flags = flags;
    d.m_y0 = from->y;
    d.m_y1 = to->y;
    float vis = 0.0f;

    if (WalkLine(from, to, (flags & 1) ? func_00539240 : func_005390D0, &d, 0.2f)) {
        vis = d.m_vis;
        _saturate(&vis, 0.0f, 1.0f);
        vis = 1.0f - vis;
    }
    return vis;
}

/* from a cell's centre raised by a height to a point: the point-to-point test inlined, fully
 * visible when the cell has no centre (new in SOCOM II) */
float CAiMaps::ComputeLineVisibility(const CAiMapLoc &from, const CPnt3D *to, unsigned int flags, float height)
{
    CPnt3D p;
    float vis = 1.0f;

    if (GetCellCenter(from, &p)) {
        p.y += height;
        vis = ComputeLineVisibility(&p, to, flags);
    }
    return vis;
}

/* what the sighting walk's callback reads and sets: the range squared, the visibility left, a
 * flag, and the last cell's distance and location (the names ours) */
struct SIGHT_LINE_DATA {
    float m_rangeSq;   // 0x0
    float m_vis;   // 0x4
    bool m_flag;   // 0x8
    float m_dist;   // 0xc
    CAiMapLoc m_loc;   // 0x10
};

/* the sighting walk's callback (0x538A60, not written yet) */
extern "C" bool func_00538A60(const MAP_LINEWALK_STRUCT &s, void *data);

/* a random float: a minimum and a range (zkit.h's Rfloat, SOCOM 1's -- zkit.h itself, through
 * zseal.h, declares a CAiMaps of its own, so the class is repeated here under its own name) */
class CRandomFloat {
public:
    operator float() const
    {
        float f;

        if (m_range == 0.0f)
            f = m_min;
        else {
            float r = 4.656613e-10f * rand();
            f = m_min + m_range * r;
        }
        return f;
    }

    float m_min;   // 0x0
    float m_range;   // 0x4
};

/* a chance taken: a random float in [0, 1) against the odds (seal_airesponse.cpp's Chance: the
 * odds by value, read through a reference -- stored and read back across rand()) */
inline bool Chance(float odds)
{
    const float &c = odds;
    CRandomFloat r;

    r.m_min = 0.0f;
    r.m_range = 1.0f;
    return r <= c;
}

/* whether the line between two points is seen: fully visible it is, not at all it is not, and in
 * between by chance of what is left (new in SOCOM II, the name ours: the AI states' sightings) */
bool CAiMaps::CanSeePoint(const CPnt3D *from, const CPnt3D *to, float rangeSq)
{
    SIGHT_LINE_DATA d;

    d.m_rangeSq = rangeSq;
    d.m_vis = 1.0f;
    d.m_flag = false;
    WalkLine(from, to, func_00538A60, &d, 0.2f);
    float vis = d.m_vis;
    bool seen = false;

    if (!(vis < 1.0f))
        seen = true;
    else if (!(vis <= 0.0f))
        seen = Chance(vis);
    return seen;
}
