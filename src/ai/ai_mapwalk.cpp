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

/* whether a step may end on the cell: CAiMap::IsTraversable's body (ai_maps.cpp, 0x52A090), as an
 * inline of this unit's -- too big to inline whole, it is inlined one level, GetCell and the cell's
 * own test (ai_astar.cpp's copy, 0x511F40) called out of line */
inline bool _traversable(const CAiMap *map, const CAiMapLoc &loc)
{
    CAiCell *cell = map->GetCell(loc, 0);

    return cell ? cell->IsTraversable() : false;
}

/* how wide the open ground around a cell is: the square rings around it grown one cell at a time,
 * up to a limit, until one leaves the map or holds a cell no step may end on -- the ring's x runs
 * over its top and bottom rows, then its y over the two sides between them (SOCOM 1's, 0x794 bytes
 * there) */
int CAiMap::GetLocationWidth(const CAiMapLoc &loc, int max)
{
    int w;

    for (w = 1; w <= max; w++) {
        int x;
        int y0 = loc.y - w;
        int x0 = loc.x - w;
        int y1 = loc.y + w;
        int x1 = loc.x + w;

        if (x0 < 0 || y0 < 0)
            return w;
        if (x1 >= m_nx || y1 >= m_ny)
            return w;
        for (x = x0; x < x1; x++) {
            if (!_traversable(this, CAiMapLoc(x, y0, 0)))
                return w;
            if (!_traversable(this, CAiMapLoc(x, y1, 0)))
                return w;
        }
        for (int y = y0 + 1; y < y1 - 1; y++) {
            if (!_traversable(this, CAiMapLoc(x0, y, 0)))
                return w;
            if (!_traversable(this, CAiMapLoc(x1, y, 0)))
                return w;
        }
    }
    return w;
}

/* a point between two, a fraction along: each end scaled and the two added, through CPnt3D's
 * const operators (new in SOCOM II, the name ours: CAiMaps::WalkLine's step, 0x539AA0) */
static CPnt3D *_lerp(const CPnt3D *a, const CPnt3D *b, CPnt3D *out, float t)
{
    *out = *a * (1.0f - t) + *b * t;
    return out;
}

/* a cell's altitude: the map's floor plus its quantised height (SOCOM 1's took the box's height
 * over 1023 steps; the name ours) -- none for no cell, the result then unset, as both builds have it */
inline float CellAltitude(const CAiMap *map, const CAiCell *cell)
{
    float alt;

    if (cell)
        alt = map->m_bbox.m_min.y + map->m_alt_quant * cell->m_altitude;
    return alt;
}

/* the feet stop where a step may not end, and on steep ground that climbs, the blocking cell
 * handed back (SOCOM 1's) */
static bool _TravelLineCallback(const MAP_LINEWALK_STRUCT &s, void *data)
{
    CAiMapLoc *out = (CAiMapLoc *)data;
    CAiCell *cell = s.m_cell;
    unsigned int soil = cell->m_soil;
    bool travel = !(soil == 0 || soil == 10 || soil == 8 || soil == 9);

    if (travel && (CAiMap::SOIL)soil == CAiMap::SOIL_STEEP && cell != s.m_prev && s.m_prev) {
        float from = CellAltitude(s.m_map, s.m_prev);
        float to = CellAltitude(s.m_map, cell);

        if (to > from)
            travel = false;
    }
    if (!travel && out)
        *out = s.m_loc;
    return travel;
}

/* the eye stops at an opaque cell and at a shut door -- an action of kind 1 or 7 in state 0 --
 * the blocking cell handed back (SOCOM 1's, the doors new) */
static bool _SeeLineCallback(const MAP_LINEWALK_STRUCT &s, void *data)
{
    CAiMapLoc *out = (CAiMapLoc *)data;
    bool clear = true;

    if (s.m_cell->IsOpaque()) {
        clear = false;
    } else if (s.m_cell->m_action) {
        CAiMap::action *a = s.m_map->GetCellAction(s.m_loc);

        if (a) {
            if (a->GetType() == CAiMap::action::DOOR && a->m_state == 0)
                clear = false;
            else if (a->GetType() == 7 && a->m_state == 0)
                clear = false;
        }
    }
    if (!clear && out)
        *out = s.m_loc;
    return clear;
}

bool CAiMap::CanTravelLine(const CAiMapLoc &from, const CAiMapLoc &to, CAiMapLoc *out)
{
    return WalkLine(from, to, _TravelLineCallback, out, 0.2f);
}

bool CAiMap::CanSeeLine(const CAiMapLoc &from, const CAiMapLoc &to, CAiMapLoc *out)
{
    return WalkLine(from, to, _SeeLineCallback, out, 0.2f);
}

bool CAiMap::CanSeeLine(const CAiMapLoc &from, const CPnt3D *dir, CAiMapLoc *out)
{
    return WalkLine(from, dir, _SeeLineCallback, out, 0.2f);
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

/* what the visibility walk reads, file statics (OVL2 0x64F9E0-0x64FA10, 8 bytes apart: SOCOM 1's
 * guarded function-local statics; the names ours): the distance past which cover counts, and each
 * cover's height and the visibility it takes */
static float s_coverRange = 30.0f;
static float s_grassHeight = 15.0f;
static float s_brushHeight = 18.0f;
static float s_forestHeight = 20.0f;
static float s_grassCover = 0.35f;
static float s_brushCover = 0.5f;
static float s_forestCover = 0.85f;

/* the line's height at the cell, blended between its two ends: ground above it blocks it, unless
 * the cell's altitude is bad or it has none; past the cover range, without flag 2, grass, brush
 * and forest each take some visibility while they stand above it (SOCOM 1's _VisLineCallback, the
 * heights and the ground new) */
static bool _VisLineCallback(const MAP_LINEWALK_STRUCT &s, void *data)
{
    VIS_LINE_DATA *d = (VIS_LINE_DATA *)data;
    float y = d->m_y0 * (1.0f - s.m_t) + d->m_y1 * s.m_t;
    float alt;

    if (s.m_map->GetAltitude(s.m_loc, alt)) {
        if (alt > y && !s.m_cell->m_badalt && s.m_cell->m_soil != CAiMap::SOIL_NOALT) {
            d->m_vis = 1.0f;
        } else if (s.m_dist > s_coverRange && !(d->m_flags & 2)) {
            float h = 0.0f;
            float cover = h;
            unsigned int soil = s.m_cell->m_soil;

            if (soil == CAiMap::SOIL_GRASS) {
                cover = s_grassCover;
                h = s_grassHeight;
            } else if (soil == CAiMap::SOIL_BRUSH) {
                cover = s_brushCover;
                h = s_brushHeight;
            } else if (soil == CAiMap::SOIL_FOREST) {
                cover = s_forestCover;
                h = s_forestHeight;
            }
            if (!(alt + h < y))
                d->m_vis += cover;
        }
    }
    return d->m_vis < 1.0f;
}

/* the eye's test first -- an opaque cell or a shut door blocks the line whole -- then the
 * visibility walk's (new in SOCOM II, the name ours: ComputeLineVisibility's callback without
 * flag 1) */
static bool _VisSeeLineCallback(const MAP_LINEWALK_STRUCT &s, void *data)
{
    VIS_LINE_DATA *d = (VIS_LINE_DATA *)data;

    if (s.m_cell->IsOpaque()) {
        d->m_vis = 1.0f;
    } else if (s.m_cell->m_action) {
        CAiMap::action *a = s.m_map->GetCellAction(s.m_loc);

        if (a) {
            if (a->GetType() == CAiMap::action::DOOR && a->m_state == 0)
                d->m_vis = 1.0f;
            else if (a->GetType() == 7 && a->m_state == 0)
                d->m_vis = 1.0f;
        }
    }
    return _VisLineCallback(s, data);
}

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
        if (WalkLine(from, to, (flags & 1) ? _VisLineCallback : _VisSeeLineCallback, &d, 0.2f)) {
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

    if (WalkLine(from, to, (flags & 1) ? _VisLineCallback : _VisSeeLineCallback, &d, 0.2f)) {
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

/* what the sighting walk takes off at each cell (OVL2 0x64FA38-0x64FA50, file statics as the
 * visibility walk's; the names ours): another map's cell off its link, water and cover, going in
 * or out of doors, and doing so through a shut door */
static float s_sightOtherMap = 0.2f;
static float s_sightCover = 0.9f;
static float s_sightInside = 0.75f;
static float s_sightOpaque = 0.25f;

/* water, brush, forest or grass (the name ours: an inline bool, materialised before the branch) */
inline bool IsSightCover(const CAiCell *cell)
{
    unsigned int soil = cell->m_soil;

    return soil == CAiMap::SOIL_WATER || soil == CAiMap::SOIL_BRUSH || soil == CAiMap::SOIL_FOREST ||
           soil == CAiMap::SOIL_GRASS;
}

/* the sighting walk's callback: the first cell kept, the distance noted; past the range nothing is
 * seen, and each cell takes its share of what is left (new in SOCOM II, the name ours) */
static bool _SightLineCallback(const MAP_LINEWALK_STRUCT &s, void *data)
{
    SIGHT_LINE_DATA *d = (SIGHT_LINE_DATA *)data;

    if (s.m_t == 0.0f)
        d->m_loc = s.m_loc;
    d->m_dist = s.m_dist;
    if (s.m_dist * s.m_dist > d->m_rangeSq) {
        d->m_vis = 0.0f;
    } else if (s.m_cell) {
        bool opaque = s.m_cell->IsOpaque();

        if (s.m_loc.m_mapid != d->m_loc.m_mapid && !s.m_cell->m_link)
            d->m_vis *= s_sightOtherMap;
        if (s.m_cell->m_action) {
            CAiMap::action *a = s.m_map->GetCellAction(s.m_loc);

            if (a) {
                if (a->GetType() == CAiMap::action::DOOR && a->m_state == 0)
                    opaque = true;
                else if (a->GetType() == 7 && a->m_state == 0)
                    opaque = true;
            }
        }
        if (IsSightCover(s.m_cell))
            d->m_vis *= s_sightCover;
        bool inside = s.m_cell->m_inside;

        if (s.m_t > 0.0f && inside != d->m_flag) {
            d->m_vis *= s_sightInside;
            if (opaque)
                d->m_vis *= s_sightOpaque;
        }
        d->m_flag = inside;
    }
    return d->m_vis > 0.0f;
}

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
    WalkLine(from, to, _SightLineCallback, &d, 0.2f);
    float vis = d.m_vis;
    bool seen = false;

    if (!(vis < 1.0f))
        seen = true;
    else if (!(vis <= 0.0f))
        seen = Chance(vis);
    return seen;
}
