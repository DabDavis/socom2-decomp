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

/* the front taken off (MSL's pop_front, which include/list's class list lacks: a level above the
 * erase as push_back is above the insert, so FloodFill's push_back takes its temporaries first and
 * they get the higher stack slots, as the target's do -- erase(begin()) written in place swaps
 * the two pairs) */
inline void pop_front(std::list<CAiMapLoc> &l)
{
    l.erase(l.begin());
}

/* the cells around one flooded out breadth first, up to (2 * radius) squared of them: each cell
 * taken off the queue handed to the callback (mode 1), then each of its eight neighbours not yet
 * marked (mode 0) -- one the callback takes is marked, its region grown and queued, unless the
 * callback said it is done (mode 2); the marks cleared over the region after (SOCOM 1's). The
 * directions' order from the given one is built and never used: each neighbour's read of it (a
 * dead load) is what keeps the last element's store, which MW drops without it; the walk at the
 * end is the region ForEachCell Tile inlines too (the two share its constant pool, 0x64F9D0) */
bool CAiMap::FloodFill(const CAiMapLoc &loc, DIR dir, int radius, bool (*f)(MAP_FLOODFILL_STRUCT &s, void *data),
                       void *data)
{
    MAP_FLOODFILL_STRUCT s;

    s.m_map = this;
    s.m_first = loc;
    s.m_loc = loc;
    s.m_firstDir = dir;
    std::list<CAiMapLoc> open;
    int max = radius * 2 * (radius * 2);
    int count = 0;
    bool done = false;
    DIR order[8] = {N, NE, E, SE, S, SW, W, NW};

    if (dir != DIR_COUNT) {
        order[0] = dir;
        order[1] = (DIR)((order[0] + 4) % 8);
        order[2] = (DIR)((order[0] + 9) % 8);
        order[3] = (DIR)((order[0] + 7) % 8);
        order[4] = (DIR)((order[1] + 9) % 8);
        order[5] = (DIR)((order[1] + 7) % 8);
        order[6] = (DIR)((order[0] + 10) % 8);
        order[7] = (DIR)((order[0] + 6) % 8);
    }
    region r;

    r.m_loc = loc;
    open.push_back(loc);
    while (!done && open.size() != 0 && count < max) {
        s.m_loc = *open.begin();
        pop_front(open);
        count++;
        s.m_cell = GetCell(s.m_loc, 0);
        s.m_mode = 1;
        if (!f(s, data)) {
            done = true;
        } else {
            s.m_mode = 0;
            for (int i = 0; !done && i < 8; i++) {
                DIR d = order[i];

                if (GetNeighbor(s.m_loc, (DIR)i, s.m_start, s.m_cell) && !s.m_cell->m_rtflag) {
                    s.m_dir = (DIR)i;
                    if (f(s, data)) {
                        s.m_cell->SetRtFlag(1);
                        r.Union(s.m_start);
                        if (s.m_mode == 2)
                            done = true;
                        else
                            open.push_back(s.m_start);
                    }
                }
            }
        }
    }
    ForEachCell(&r, _reset_rtflag, 0, 0);
    return done;
}

/* a cell's shadow, boundary and occupied bits set from the tile's value, the change noted (SOCOM
 * 1's _tile_boundary; the shadow's and the occupied bit's new in SOCOM II) */
/* a cell's soil set from the tile's value where it may be: anywhere (flag 1) or over the default
 * soil, and over steep ground with flag 2 (SOCOM 1's _tile_soil, the steep case new). The two
 * bools assigned true and cleared, the steep test materialised into a bool of its own: each
 * measured. The flags are read twice, unnamed: MW merges the loads into a temporary, numbered after
 * every named local (the old soil, first read in a test, moved to their end), which gives the
 * target's a0 for the flags and a2 for the old soil -- a named `flags` swapped the two */
static bool _tile_soil(const MAP_FOREACH_STRUCT &s, void *data)
{
    TILE_DATA *t = (TILE_DATA *)data;
    CAiCell *cell = s.m_cell;
    unsigned char soil = t->m_value;
    unsigned int old = cell->GetSoil();

    if (old != soil) {
        bool set = true;
        bool any = true;

        if (!(t->m_flags & 1) && old != CAiMap::SOIL_DEFAULT)
            any = false;
        if (!any) {
            bool steep = (t->m_flags & 2) != 0 && old == CAiMap::SOIL_STEEP;

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

/* a region of a layer's cells (the whole map for none) set to a value: the flags pick one tile --
 * the soil (4), the boundary (8), the shadow (16), the occupied bit (32) -- whether any cell
 * changed (SOCOM 1's Tile(region, value, flags); the layer new). Each tile is the region walk
 * inlined one level, its region's constructor and GetMapRegion called (FloodFill's walk).
 * OPEN (0x8BC against 0x8B4), 2 instructions at the start, all else the target's (the four walks
 * byte for byte): the target stores the flags and reads them back sp-relative (`sw a3,356(sp);
 * lw v1,356(sp)`, the value's store in the first test's delay slot), this build takes &t.m_flags
 * into v0 for the pair and fills the delay slot with the region's address instead. Tried: the
 * stores in either order, `= {value, flags}` (with and without the third), a local copy of the
 * flags, the struct through a pointer local (0x8E4), the tests in an inline over the data
 * pointer (not inlined), the value stored inside the first arm */
bool CAiMap::Tile(const region *r, unsigned int value, unsigned int flags, int layer)
{
    TILE_DATA t = {0};

    t.m_value = value;
    t.m_flags = flags;
    if (t.m_flags & 4)
        ForEachCell(r, _tile_soil, &t, layer);
    else if (t.m_flags & 8)
        ForEachCell(r, _tile_boundary, &t, layer);
    else if (t.m_flags & 0x10)
        ForEachCell(r, _tile_shadow, &t, layer);
    else if (t.m_flags & 0x20)
        ForEachCell(r, _tile_occupied, &t, layer);
    return t.m_changed;
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

/* the direction of the most counts: each count summed with its two neighbours' when asked, and only
 * the heading's closest direction and the two either side of it weighed when a heading is given --
 * the closest the first choice, a later one taken only with more (new in SOCOM II, the names ours:
 * 0x5350C0, the walker unit's first function, CAiSRush::Tick's call after a counter's 0x535A40). The
 * eight directions a local aggregate, copied off its initialiser (0x64FA68, ld/sd); the counter
 * declared ahead of the count (so n takes a1, i a2) and the candidate's count compared on the left
 * (loaded first: `c[best] < c[dirs[i]]` was 15 off, 6 with one of the two) */
CAiMap::DIR MAP_DIRCOUNT::GetBestDir(bool smooth, const CPnt3D *heading)
{
    CAiMap::DIR best = CAiMap::N;
    int c[8];

    if (!smooth) {
        memcpy(c, m_count, sizeof(c));
    } else {
        for (int i = 0; i < 8; i++)
            c[i] = m_count[(i + 7) % 8] + (m_count[i] + m_count[(i + 1) % 8]);
    }
    CAiMap::DIR dirs[8] = {CAiMap::N, CAiMap::NE, CAiMap::E, CAiMap::SE, CAiMap::S, CAiMap::SW, CAiMap::W, CAiMap::NW};
    int i;
    int n = 8;

    if (heading) {
        CPnt3D h;

        heading->Normalize(&h);
        best = dirs[0] = CAiMap::GetClosestDir(&h);
        dirs[1] = (CAiMap::DIR)((best + 7) % 8);
        dirs[2] = (CAiMap::DIR)((best + 9) % 8);
        dirs[3] = (CAiMap::DIR)((best + 6) % 8);
        dirs[4] = (CAiMap::DIR)((best + 10) % 8);
        n = 5;
    }
    for (i = 0; i < n; i++)
        if (c[dirs[i]] > c[best])
            best = dirs[i];
    return best;
}
