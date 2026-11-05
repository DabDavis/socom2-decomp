/* ai_map.cpp (OVL2 0x515580-0x51A48F, SOCOM 1's unit: __sinit_ai_map.cpp, the demo's 0x185200-
 * 0x188E8F): CAiMap's cells -- the cell's reset, the walks' callbacks, the map's construction, the
 * soils and the eight directions -- and the map tables' template tail (the deleters' destructors
 * and clears, 0x5197A0-0x51A48F). SOCOM 1's functions in the demo's order (CAiCell(), the repel
 * parameters, TransformTo, GetMapRegion, ResetCells, ValidateCells and its callbacks, ~CAiMap,
 * CAiMap(), GetSoilByName, the GetClosestDirs, GetDirDelta, GetDirByName, GetDirQuat, GetDirVec);
 * SOCOM II adds the cell layers (CAiMapLayer, whose own code sits after vehicle_rdr.cpp, from
 * ~0x53A730). Its initialiser (0x668630) builds the dummy cell (0x66A198) and the cosine of 45
 * degrees (0x65AB38). Defined here in reverse of the image's order (-inline deferred).
 * Not written yet: the
 * repel parameters 0x5169B0/0x516F10 (TransformTo 0x517050 since round 26), ValidateCells 0x517850 (the walk with
 * _validate_cell plus GetCell inlined one level -- IsCell 0x512140, IsLayer 0x512120 and the
 * pieces after them -- and out of line, 0x511F80), GetClosestDir(cell, point)
 * 0x518900, GetDirByName 0x518D80 (its draft matches), the scanlines' resize/insert/erase
 * (0x519700, 0x519DB0, 0x519B90) and the pod insert copy 0x5192E0. */

#include <string.h>

#include "ai_maps.h"

/* MAIN's debug calloc (SOCOM 1's _calloc__FUiUiPCci: a file and line, passed none) */
void *_calloc(unsigned int n, unsigned int size, const char *file, int line);

/* ai.h's two templates defined (as zseal.h defines them: never inlined, MW auto-inlines no
 * template function -- the instances close this unit) */
namespace ai {
template <class T>
const T &_max(const T &a, const T &b)
{
    return a >= b ? a : b;
}

template <class T>
const T &_min(const T &a, const T &b)
{
    return a <= b ? a : b;
}
}

/* the cell a walk hands out off the map (SOCOM 1's, returned by CAiMap::dummy_cell) and the
 * cosine of 45 degrees, computed at start-up (GetDirCos reads it from data) */
static CAiCell s_dummy_cell;
/* no action's id (SOCOM 1's static member; OVL2 0x65AB30, just ahead of the cosine) */
const unsigned int CAiMap::action::invalid = 0xffff;
/* each direction's step in cells, N to NW (SOCOM 1's CAiMap::neighbor: OVL2 0x64F910, after the
 * cell walk's constant pool 0x64F900 that ResetCells and ValidateCells share) */
const int CAiMap::neighbor[8][2] = {{0, -1}, {1, -1}, {1, 0}, {1, 1}, {0, 1}, {-1, 1}, {-1, 0}, {-1, -1}};
static float s_cos45 = 0.5f * sqrtf(2.0f);

/* a vector's three components set from values taken first (the name ours: the diagonals' static
 * is read once for both of its uses, as only arguments let it be) */
static inline void set_vec(CPnt3D &v, float x, float y, float z)
{
    v.x = x;
    v.y = y;
    v.z = z;
}

/* a direction's unit vector, north -z and east +x (SOCOM 1's, its diagonals now the static) */
CPnt3D &CAiMap::GetDirVec(DIR dir, CPnt3D &v)
{
    switch (dir) {
    case N:
        set_vec(v, 0.0f, 0.0f, -1.0f);
        break;
    case NE:
        set_vec(v, s_cos45, 0.0f, -s_cos45);
        break;
    case E:
        set_vec(v, 1.0f, 0.0f, 0.0f);
        break;
    case SE:
        set_vec(v, s_cos45, 0.0f, s_cos45);
        break;
    case S:
        set_vec(v, 0.0f, 0.0f, 1.0f);
        break;
    case SW:
        set_vec(v, -s_cos45, 0.0f, s_cos45);
        break;
    case W:
        set_vec(v, -1.0f, 0.0f, 0.0f);
        break;
    case NW:
        set_vec(v, -s_cos45, 0.0f, -s_cos45);
        break;
    default:
        set_vec(v, 0.0f, 0.0f, -1.0f);
        break;
    }
    return v;
}

/* how alike two directions are: the cosine of the angle between them (new in SOCOM II, the name
 * ours; CSealCtrlAi's code calls it) */
float CAiMap::GetDirCos(DIR a, DIR b)
{
    int d = a - b;

    if (d == 0)
        return 1.0f;
    if (d == -1 || d == 1)
        return s_cos45;
    if (d == -2 || d == 2)
        return 0.0f;
    if (d == -3 || d == 3)
        return -s_cos45;
    return -1.0f;
}

/* the turn about y that faces a direction (north is pi: a model faces +z), turned about once more
 * when asked (SOCOM 1's; CMission::LoadVehicle and the respawns call it) */
CQuat &CAiMap::GetDirQuat(DIR dir, CQuat &q, bool reverse)
{
    switch (dir) {
    case N:
        q.MakeYRot(3.14159274f);
        break;
    case NE:
        q.MakeYRot(2.3561945f);
        break;
    case E:
        q.MakeYRot(1.57079637f);
        break;
    case SE:
        q.MakeYRot(0.785398185f);
        break;
    case S:
        q.MakeYRot(0.0f);
        break;
    case SW:
        q.MakeYRot(5.49778748f);
        break;
    case W:
        q.MakeYRot(4.71238899f);
        break;
    case NW:
        q.MakeYRot(3.92699099f);
        break;
    default:
        q.MakeYRot(3.14159274f);
        break;
    }
    if (reverse) {
        CQuat r;

        r.MakeYRot(3.14159274f);
        q = q * r;
        q.Normalize();
    }
    return q;
}

/* the turn from one direction to another, -4 to 4 (SOCOM 1's) */
int CAiMap::GetDirDelta(DIR a, DIR b)
{
    int d = b - a;

    if (d > 4)
        d -= 8;
    else if (d < -4)
        d += 8;
    return d;
}

/* the direction nearest a unit vector: the five on its side of the east-west line tried, the one
 * least away from it kept (SOCOM 1's) */
CAiMap::DIR CAiMap::GetClosestDir(const CPnt3D *v)
{
    CPnt3D n;

    GetDirVec(N, n);
    if (Dot(*v, n) >= 0.0f) {
        DIR best = W;
        float least = 1.0f;

        for (unsigned int i = W; i <= W + 4; i++) {
            DIR d = (DIR)((int)(i + 8) % 8);

            if (d != DIR_COUNT) {
                CPnt3D t;

                GetDirVec(d, t);
                float e = 1.0f - Dot(*v, t);

                if (e < least) {
                    best = d;
                    least = e;
                }
            }
        }
        return best;
    } else {
        DIR best = E;
        float least = 1.0f;

        for (unsigned int i = E; i <= E + 4; i++) {
            DIR d = (DIR)((int)(i + 8) % 8);

            if (d != DIR_COUNT) {
                CPnt3D t;

                GetDirVec(d, t);
                float e = 1.0f - Dot(*v, t);

                if (e < least) {
                    best = d;
                    least = e;
                }
            }
        }
        return best;
    }
}

/* the direction from one cell to another (SOCOM 1's) */
CAiMap::DIR CAiMap::GetClosestDir(const CAiMapLoc &from, const CAiMapLoc &to)
{
    CPnt3D v((float)(int)(to.x - from.x), 0.0f, (float)(int)(to.y - from.y));

    v.Normalize();
    return GetClosestDir(&v);
}

/* the direction from a cell's centre toward a point, level (SOCOM 1's; the centre inlined one
 * level, as CAiMaps::GetCellCenter has it), none off the map */
CAiMap::DIR CAiMap::GetClosestDir(const CAiMapLoc &from, const CPnt3D *to)
{
    DIR dir = DIR_COUNT;
    CPnt3D c;

    if (GetCellCenter(from, &c)) {
        to->Sub(&c, &c);
        c.y = 0.0f;
        c.Normalize();
        dir = GetClosestDir(&c);
    }
    return dir;
}

/* a soil's name (new in SOCOM II, the name ours: SOCOM 1's GetSoilByName had the switch inline) */
const char *CAiMap::GetSoilString(SOIL soil)
{
    switch (soil) {
    case SOIL_NOGO:
        return "nogo";
    case SOIL_DEFAULT:
        return "default";
    case SOIL_WATER:
        return "water";
    case SOIL_ROAD:
        return "road";
    case SOIL_BRUSH:
        return "brush";
    case SOIL_FOREST:
        return "forest";
    case SOIL_CLIMB:
        return "climb";
    case SOIL_STAIRS:
        return "stairs";
    case SOIL_CLUTTER:
        return "clutter";
    case SOIL_TRANS:
        return "trans";
    case SOIL_NOALT:
        return "noalt";
    case SOIL_STEEP:
        return "steep";
    case SOIL_GRASS:
        return "grass";
    case SOIL_COLLISION:
        return "collision";
    }
    return "default";
}

/* the soil of a name, the default soil when none has it (SOCOM 1's) */
CAiMap::SOIL CAiMap::GetSoilByName(const char *name)
{
    if (name) {
        for (int i = 0; i < SOIL_COUNT; i++) {
            SOIL soil = (SOIL)i;

            if (!strcasecmp(name, GetSoilString(soil)))
                return soil;
        }
    }
    return SOIL_DEFAULT;
}

/* the version a map file is written in (23, OVL2 data: CAiMap() starts each map at it; the name
 * ours, SOCOM 1's an unnamed word) */
extern unsigned int s_aimap_version;

/* a box with both corners at one point (CBBox has no constructor in zmath.h: this stand-in of the
 * same layout builds the same temporary -- the point copied by value, the first corner
 * copy-constructed from it, the second assigned as a block, through W: MW copies a class-type
 * member as a block, three loads then three stores through the two corners' addresses, where a
 * CPnt3D assignment is the fields' one by one -- for a CBBox to be assigned from, the temporary
 * named: the assignment reads the second corner back through the register it was stored through) */
struct CBBoxInit {
    struct W { CPnt3D p; };
    CBBoxInit(CPnt3D p) : m_min(p) { *(W *)&m_max = *(const W *)&p; }

    CPnt3D m_min;
    CPnt3D m_max;
};

/* the map with its first layer pushed onto the layers and nothing read: an empty box, no size,
 * floor -1, resolution 0.5 and slope pi/4 (SOCOM 1's, the id and the layers new); the dummy
 * cell's soil set to noalt again. Byte-exact since the AI core's round 24 (open from round 16 at
 * 0x1D8 against 0x1DC: the target addresses the point's copy (sp+80) and the second corner (sp+60)
 * through registers -- the block copy of a class-type member, CBBoxInit's W -- and m_bbox is
 * assigned from the named temporary, its second corner read back through sp+60's register; a
 * temporary assigned straight from the cast reads it at +12 off the first's). ~CAiMap (0x5181F0)
 * opens with the same box */
CAiMap::CAiMap(int mapid) : m_layer0(0)
{
    s_dummy_cell.m_soil = SOIL_NOALT;
    {
        CBBoxInit t(CPnt3D::zero);

        m_bbox = (const CBBox &)t;
    }
    m_ny = 0;
    m_nx = 0;
    m_dy = 0.0f;
    m_dx = 0.0f;
    m_mapid = mapid;
    m_floor = -1;
    m_name[0] = 0;
    m_resolution = 0.5f;
    m_slope_threshold = 0.785398185f;
    m_flags = 0;
    m_version = s_aimap_version;
    m_mapex = 0;
    m_mapex_bytes = 0;
    m_compressed = 0;
    m_cellcount = 0;
    m_light[1] = 0.0f;
    m_light[0] = 0.0f;
    m_graph = 0;
    m_overlay = 0;
    m_layers.push_back(&m_layer0);
}

/* the map emptied: the box and the size back to nothing, the first layer's cells, the extra data,
 * the other layers and the graph given up, then the tables destroyed (SOCOM 1's, the layers new).
 * Byte-exact since the AI core's round 24 with CAiMap(int)'s box (0x250 against 0x258 before) */
CAiMap::~CAiMap()
{
    {
        CBBoxInit t(CPnt3D::zero);

        m_bbox = (const CBBox &)t;
    }
    m_ny = 0;
    m_nx = 0;
    m_dy = 0.0f;
    m_dx = 0.0f;
    m_layer0.FreeCells();
    FreeEx();
    DeleteLayers();
    DestroyGraph();
}

/* bit 1 of the map's flags set or cleared, the old value returned (new in SOCOM II, the name ours:
 * SetCellLink and the layers' removal set it after a change) */
bool CAiMap::SetModified(bool modified)
{
    bool old = IsModified();

    if (modified)
        m_flags |= 2;
    else
        m_flags &= ~2;
    return old;
}

/* a walk's callback: the cell's route flag cleared (SOCOM 1's) */
bool CAiMap::_reset_rtflag(const MAP_FOREACH_STRUCT &s, void *)
{
    s.m_cell->SetRtFlag(false);
    return true;
}

/* a walk's callback: the cell's action valid and in its first state, a collision cell back to the
 * default soil, the cell unoccupied (new in SOCOM II, the name ours) */
bool CAiMap::_reset_cell(const MAP_FOREACH_STRUCT &s, void *)
{
    CAiCell *cell = s.m_cell;

    if (cell->m_action) {
        action *a = s.m_map->GetCellAction(s.m_loc);

        if (a) {
            a->SetTypeInvalid(false);
            a->SetState(0);
        }
    }
    if (cell->GetSoil() == SOIL_COLLISION)
        cell->m_soil = SOIL_DEFAULT;
    cell->SetOccupied(false);
    return true;
}

/* a walk's callback: the cell's search fields cleared, then the two resets above (inlined), then
 * each of its name, action and link bits kept only where the map's table has an entry for it
 * (SOCOM 1's, the resets new) */
bool CAiMap::_validate_cell(const MAP_FOREACH_STRUCT &s, void *data)
{
    CAiCell *c = s.m_cell;

    c->m_astar_dir = 8;
    c->m_astar_f = -1;
    c->m_astar_g = -1;
    _reset_rtflag(s, data);
    _reset_cell(s, data);
    CAiCell *cell = s.m_cell;

    if (cell->m_name && !s.m_map->GetCellName(s.m_loc))
        cell->SetName(false);
    if (cell->m_action && !s.m_map->GetCellAction(s.m_loc))
        cell->SetAction(false);
    if (cell->m_link && !s.m_map->GetCellLink(s.m_loc))
        cell->SetLink(false);
    return true;
}

/* whether a step may end on the cell: CAiMap::IsTraversable's body (ai_maps.cpp's), an inline of
 * this unit's too big to inline whole -- ValidateCells inlines it one level, GetCell and the cell's
 * test (ai_astar.cpp's copy, 0x511F40) called out of line (ai_mapwalk.cpp has the same) */
inline bool _traversable(const CAiMap *map, const CAiMapLoc &loc)
{
    CAiCell *cell = map->GetCell(loc, 0);

    return cell ? cell->IsTraversable() : false;
}

/* every name ended and given the map's id, its cell marked named; every action's and link's cell
 * looked up (the result unused: whatever read it is not in this build); the links whose cell no
 * step may end on counted; the regions given the map's id; then every cell validated (SOCOM 1's;
 * the walks over the tables through their class iterators, each test materialised; each map id
 * read into a local before the compare, which puts the field first in the beq) */
int CAiMap::ValidateCells()
{
    int bad = 0;

    for (names::iterator n = m_namesTable.begin(); n != m_namesTable.end(); ++n) {
        (*n).ValidateName();
        unsigned int id = (*n).m_loc.m_mapid;

        if (id != m_mapid)
            (*n).m_loc.m_mapid = m_mapid;
        if (IsMapCell((*n).m_loc)) {
            CAiCell *cell = GetCell((*n).m_loc, 0);

            if (cell && !cell->HasName())
                cell->SetName(true);
        }
    }
    for (actions::iterator a = m_actionTable.begin(); a != m_actionTable.end(); ++a)
        GetCell((*a).m_loc, 0);
    for (links::iterator l = m_linksTable.begin(); l != m_linksTable.end(); ++l) {
        GetCell((*l).m_loc, 0);
        if (!_traversable(this, (*l).m_loc))
            bad++;
    }
    for (regions::iterator r = m_regionTable.begin(); r != m_regionTable.end(); ++r) {
        unsigned int id = (*r).m_loc.m_mapid;

        if (id != m_mapid)
            (*r).m_loc.m_mapid = m_mapid;
    }
    ForEachCell(0, _validate_cell, 0, 0);
    return bad;
}

/* every cell of the first layer reset through _reset_cell (SOCOM 1's ResetCells, a loop of its own
 * there): the walk inlined one level, its pieces called out of line in reverse of their first use
 * after it (0x5176D0-0x51784F, all byte-exact from here); false whatever the walk did. The walk
 * is the region walk FloodFill and Tile inline, given no region: its pointer, assigned in the
 * walk's branch, is what keeps &r in s5 and the row read through it twice (a walk always over its
 * own region let MW resolve the pointer to sp+176 and share the loads -- 44 instructions off) */
bool CAiMap::ResetCells()
{
    ForEachCell(0, _reset_cell, 0, 0);
    return false;
}

/* the whole map as a region: its first cell (map 0), the map's width and height (SOCOM 1's;
 * called, not inlined, by every walk: pinned) */
#pragma auto_inline off
void CAiMap::GetMapRegion(region &r) const
{
    r.m_loc = CAiMapLoc(0, 0, 0);
    r.SetSize(m_nx, m_ny);
}
#pragma auto_inline reset

/* a cell of this map as the cell of another map under the same point, false when that map has none
 * there (SOCOM 1's TransformTo, the demo's 0x185B50: the same map hands the cell back as it is; the
 * cell's centre -- GetCellCenter inlined one level, its pieces called out of line -- is found on the
 * other map through GetLocation inlined whole, and the other map's id stored again) */
bool CAiMap::TransformTo(const CAiMapLoc &loc, CAiMap *map, CAiMapLoc &out)
{
    bool ok = false;

    if (this == map) {
        ok = true;
        out = loc;
    } else if (map) {
        CPnt3D c;

        if (GetCellCenter(loc, &c)) {
            if (ok = map->GetLocation(&c, out))
                out.m_mapid = map->m_mapid;
        }
    }
    return ok;
}

/* n cells allocated zeroed and each reset (new in SOCOM II, the name ours: the layers' allocation
 * 0x53A8F0 calls it) */
CAiCell *CAiCell::Alloc(unsigned int n)
{
    CAiCell *cells = 0;

    if (n) {
        cells = (CAiCell *)_calloc(n, sizeof(CAiCell), 0, 0);
        for (unsigned int i = 0; i < n; i++)
            cells[i].Reset();
    }
    return cells;
}

/* the repel parameters of a cell on its map, false off the maps (SOCOM 1's; like SOCOM 1's it asks
 * the maps through AiMaps, its `this` unread) */
bool CAiMaps::GetCellRepelParams(const CAiMapLoc &loc, const CPnt3D *pos, float range, CPnt3D &dir, float &len,
                                 CAiMapLoc &cell)
{
    CAiMap *map = AiMaps.GetMapByIndex(loc.m_mapid);

    return map ? map->GetCellRepelParams(loc, pos, range, dir, len, cell) : false;
}

/* how the cells beside one push a point away: none for no cell or a linked one; else, from the
 * cell's centre, the two neighbours on the point's side (the quadrant's first direction and the
 * next but one) that are not traversable each push it back by how far within half a cell of them
 * (less the range) it is, along their direction -- the pushes summed into `dir`, its length into
 * `len`, the first pushing cell handed back (SOCOM 1's, the demo's 0x1853A0; the cell new).
 * OPEN, 0x570 against 0x558 (written in the AI core's round 30): the code is the target's in shape --
 * GetCell inlined one level, GetCellCenter one level (its pieces the calls 0x5168E0-0x516940), the
 * quadrant, the two neighbours through GetNeighbor (one level), the pushes -- but the frame and the
 * float registers differ: the target's slots run the neighbour's cell 300, the centre 288, the zero
 * copy n 272, the difference v 256, the push 240 (n above v: as if declared first, yet copy-built in
 * the loop -- ld/sd, which an assignment does not give), and it loads v.z and v.y once into $f20/$f21
 * ahead of the quadrant test, reading n.y and n.z through address registers in the dot product; here v
 * sits above n and v.z is read through an address register in each arm. Tried (build/scratch/ai/
 * v_gcrp*.py): the neighbour's cell and pointer at the top, Dot(v, n), the products written out, v and
 * the push bound to const references (0x560: the FPR caching lost), a const v (0x568), n declared at
 * the top and assigned (0x580). The registers (the out cell s7, pos s1, the cell s0) follow the rest.
 * Round 31: the neighbour's cell initialised (`= 0`) gives the target's loop -- its `move v0,zero` at
 * the loop's top, the cell in v0, nd in s0 -- where uninitialised it lived round the loop in s0; and
 * `CPnt3D n;` declared at the top gives exactly the target's frame (n 272, v 256) but its assignment
 * is memberwise (three lwc1/swc1, the copy constructor's ld/sd gone: also through PNT3D, a pointer
 * cast, memcpy 0x568, placement new 0x590); `float vz = v.z;` loads v.z once for both arms (0x560,
 * through an address register still); v made an inline's result (`_cell_offset`, by value or bound
 * to a reference), `dir += n * k`, the quadrant through an inline: no change of order
 * (build/scratch/ai/v_gcrp4-8.py)
 * Round 32 (difflib distance 58 -> 27 of 342): `CPnt3D n;` at the top (the target's frame) with v.z
 * read once into a named float for the quadrant -- the quadrant's tests and the loop's dot then read
 * v.y / v.z from $f21 / $f20 almost as the target does. Left: n's reset in the loop is memberwise
 * (lui/lwc1/swc1 x3) where the target copy-constructs it (ld/sd + lwc1/swc1 from CPnt3D::zero, n
 * at 272 above v at 256, as if declared first but built in the loop); vz lives in $f2 for the
 * quadrant and the loop reloads v.z into $f20 (the target loads it once into $f20, sp-relative);
 * tried on top (build/scratch/ai/v_gc1-4.py): direct-init v / n, the quadrant as ifs, vx / vy locals,
 * the products written out (either operand order), Dot(v, n), v bound to a const reference (with n
 * in the loop or at the top: 54-96)
 * Round 33 (build/scratch/ai/v_gc33*.py): the 6-instruction gap is n's reset (11 instructions
 * memberwise against the target's 7: lui/addiu, ld/sd, lwc1/swc1) and the second load of v.z (2).
 * Only a DECLARATION's initialiser gives the block copy: `CPnt3D n(CPnt3D::zero)` in the loop does
 * (0x560) but lays n out below v (n 256, v 272: declaration order), as `= CPnt3D::zero` does;
 * placement new copies member by member under a null test (0x588), as do the PNT3D-cast
 * assignments, `n = CPnt3D(zero)`, a by-value helper's result (0x588) and `*&zero` (0x570). vz / vy
 * named floats fed to the dot product: 0x568-0x570 */
bool CAiMap::GetCellRepelParams(const CAiMapLoc &loc, const CPnt3D *pos, float range, CPnt3D &dir, float &len,
                                CAiMapLoc &cell)
{
    CAiCell *c = GetCell(loc, 0);

    if (!c)
        return false;
    if (c->HasLink())
        return false;
    CPnt3D center;
    CPnt3D n;

    GetCellCenter(loc, &center);
    CPnt3D v = *pos - center;
    float vz = v.z;
    DIR d;

    if (v.x > 0.0f)
        d = vz < 0.0f ? (DIR)0 : (DIR)2;
    else
        d = vz < 0.0f ? (DIR)6 : (DIR)4;
    bool found = false;
    float half = 0.5f * m_dx;

    for (int i = 0; i < 3; i += 2) {
        n = CPnt3D::zero;
        DIR nd = (DIR)((d + i + 8) % 8);
        CAiMapLoc nloc;
        CAiCell *ncell = 0;

        if (GetNeighbor(loc, nd, nloc, ncell) && !ncell->IsTraversable()) {
            GetDirVec(nd, n);
            float dot = Dot(n, v);

            if (dot > 0.0f) {
                float k = half - (dot + range);

                if (k < 0.0f) {
                    CPnt3D push = n * k;

                    dir.Add(&push, &dir);
                    if (!found) {
                        found = true;
                        cell = nloc;
                    }
                }
            }
        }
    }
    if (found)
        len = sqrtf(Dot(dir, dir));
    return found;
}

/* called, not inlined, by the initialiser (the target's jal 0x515720): pinned (the cell's altitude,
 * 0x516940, was defined here until the AI core's round 24: an inline in ai.h now, its copy emitted
 * after GetClosestDir(loc, point) here) */
#pragma auto_inline off
/* (CAiMap::GetLocation(point) and its two box helpers, defined here until the AI core's round 26,
 * are ai.h's inlines now: this unit still emits OVL2's copy, 0x515F80) */

/* the rectangle between two cells, either corner first: the map of the first, the lesser x and y
 * as the corner, the spans inclusive (new in SOCOM II, the name ours: 0x515DE0, which 0x515900 calls
 * after the footprint's two corner cells). The tests `a <= b` (`b >= a` lays the arms out 8 bytes
 * short); x compares unsigned off the word, y signed off the halfword, as the target's (the AI core
 * round 22; an inline since round 24, called out of line by the region fit BuildFootprint inlines
 * one level) */
inline void CAiMap::region::SetCorners(const CAiMapLoc &a, const CAiMapLoc &b)
{
    m_loc.m_mapid = a.m_mapid;
    if (a.x <= b.x) {
        m_loc.x = a.x;
        m_dx = b.x - a.x + 1;
    } else {
        m_loc.x = b.x;
        m_dx = a.x - b.x + 1;
    }
    if (a.y <= b.y) {
        m_loc.y = a.y;
        m_dy = b.y - a.y + 1;
    } else {
        m_loc.y = b.y;
        m_dy = a.y - b.y + 1;
    }
}

/* a point's offset from another, by value through a temporary (the name ours: the footprint test's
 * edges, each copied out of the Sub's result slot) */
inline CPnt3D _offset(const CPnt3D &a, const CPnt3D &b)
{
    CPnt3D r;

    a.Sub(&b, &r);
    return r;
}

/* whether a point lies inside a footprint's four corners: on the inner side of every edge, the
 * cross product's y of the edge and the point's offset from the edge's start not negative (the name
 * ours) */
inline bool _in_footprint(const CPnt3D *corners, const CPnt3D &p)
{
    bool in = true;

    for (int k = 0; k < 4; k++) {
        CPnt3D e = _offset(corners[(k + 1) % 4], corners[k]);
        CPnt3D d = _offset(p, corners[k]);

        if (e.z * d.x - e.x * d.z < 0.0f) {
            in = false;
            break;
        }
    }
    return in;
}

/* each row's run of cells whose centres lie inside a footprint's four edges, the region copied into
 * the overlay first (new in SOCOM II, the name ours: 0x516180, BuildFootprint's last call --
 * GetCellCenter inlined one level twice, a row's first cell inside, then its first cell outside;
 * its out-of-line pieces follow it, 0x5168E0-0x5169AF). The row's cell built through CAiMapLoc's
 * three-field constructor (its mem-initializer list: three word read-mask-writes, every source field
 * loaded first -- assigned field by field the map id is a byte store and y a halfword one); each
 * edge and offset a by-value Sub copied out of its result slot (ld/sd, lwc1/swc1) */
bool CAiMap::ScanFootprint(const CPnt3D *corners, const region &r, CAiOverlay *o)
{
    bool ok = false;

    o->m_region = r;
    int ny = o->m_region.m_dy;
    int nx = o->m_region.m_dx;

    if (ny > 0) {
        ok = true;
        o->m_scanlines.resize(ny);
        for (int y = 0; y < ny; y++) {
            scanline &sl = o->m_scanlines[y];

            sl.m_start = 0;
            sl.m_end = 0;
            CAiMapLoc loc(o->m_region.m_loc.x, o->m_region.m_loc.y + y, o->m_region.m_loc.m_mapid);
            CPnt3D c;
            int x;

            for (x = 0; x < nx; x++) {
                loc.x = o->m_region.m_loc.x + x;
                GetCellCenter(loc, &c);
                if (_in_footprint(corners, c)) {
                    sl.m_start = x;
                    break;
                }
            }
            if (x < nx) {
                for (x++; x < nx; x++) {
                    loc.x = o->m_region.m_loc.x + x;
                    GetCellCenter(loc, &c);
                    if (!_in_footprint(corners, c))
                        break;
                }
                sl.m_end = x;
            }
        }
    }
    return ok;
}

/* a box over points (CBBox's layout, the name ours): both corners at the first point, then grown by
 * each -- a coordinate beyond a corner taken (`p <= min ? p : min`, `p >= max ? p : max`: the target's
 * compare and arm order) -- and a written copy constructor, called out of line where the region fit
 * below is inlined one level (0x516140). MW COPIES A CLASS-TYPE MEMBER AS A BLOCK (three loads, then
 * three stores) where CPnt3D's own assignment interleaves its floats' loads and stores through a
 * pointer: the target's corners are blocks, so each is assigned as a struct holding a CPnt3D (W; a
 * float[3] does the same), the copy as one CBBox assignment (blocks per corner). zmath.h's CBBox has
 * no constructors, and giving it these would change every unit's CBBox -- hence the stand-in */
class CAiBox : public CBBox {
public:
    struct W { CPnt3D p; };
    CAiBox(const CPnt3D &p)
    {
        *(W *)&m_min = *(const W *)&p;
        *(W *)&m_max = *(const W *)&p;
    }
    CAiBox(const CAiBox &b) { *(CBBox *)this = b; }
    void Grow(const CPnt3D &p)
    {
        m_min.x = p.x <= m_min.x ? p.x : m_min.x;
        m_min.y = p.y <= m_min.y ? p.y : m_min.y;
        m_min.z = p.z <= m_min.z ? p.z : m_min.z;
        m_max.x = p.x >= m_max.x ? p.x : m_max.x;
        m_max.y = p.y >= m_max.y ? p.y : m_max.y;
        m_max.z = p.z >= m_max.z ? p.z : m_max.z;
    }
};

/* the region of cells under a box: the box clipped to the map's in x and z, its corners' cells, the
 * rectangle between them (the name ours: too big to inline whole -- GetLocation twice -- so the box's
 * copy, GetBBox, CAiMapLoc(), GetLocation and SetCorners are called out of line, their copies
 * 0x515DE0-0x516177 after 0x515900 in reverse of their first use) */
inline bool _box_region(const CAiMap *map, const CAiBox &box, CAiMap::region &r)
{
    CAiBox b(box);
    const CBBox &mb = map->GetBBox();

    if (b.m_min.x < mb.m_min.x)
        b.m_min.x = mb.m_min.x;
    if (b.m_max.x > mb.m_max.x)
        b.m_max.x = mb.m_max.x;
    if (b.m_min.z < mb.m_min.z)
        b.m_min.z = mb.m_min.z;
    if (b.m_max.z > mb.m_max.z)
        b.m_max.z = mb.m_max.z;
    bool ok = false;
    CAiMapLoc lo, hi;

    if (map->GetLocation(&b.m_min, lo) && map->GetLocation(&b.m_max, hi)) {
        r.SetCorners(lo, hi);
        ok = true;
    }
    return ok;
}

/* the overlay from a footprint's four corners (new in SOCOM II, the name ours: 0x515900,
 * TileFootprint's) */
bool CAiMap::BuildFootprint(const CPnt3D *corners, CAiOverlay *o)
{
    bool ok = false;
    CAiBox box(corners[0]);

    box.Grow(corners[1]);
    box.Grow(corners[2]);
    box.Grow(corners[3]);
    region r;

    if (_box_region(this, box, r))
        ok = ScanFootprint(corners, r, o);
    return ok;
}

/* the footprint's old cells cleared, the region rebuilt, the new ones set (new in SOCOM II, the name
 * ours) */
bool CAiMap::TileFootprint(const CPnt3D *corners, region *r)
{
    Tile(r, 0, 32, 0);
    BuildFootprint(corners, (CAiOverlay *)r);
    Tile(r, 1, 32, 0);
    return true;
}

/* whether the cell is under the overlay: inside its region (the region's Contains, materialised),
 * then on its row's run, the row and column taken again from the region's corner (new in SOCOM II,
 * the name ours: the base planner's cost and FindTraversable call it) */
bool CAiOverlay::Contains(const CAiMapLoc &loc) const
{
    bool r = false;

    if (m_region.Contains(loc)) {
        int y = loc.y - m_region.m_loc.y;
        int x = loc.x - m_region.m_loc.x;

        if (y >= 0 && y < m_scanlines.size()) {
            const CAiMap::scanline &sl = m_scanlines[y];

            if (x >= sl.m_start && x < sl.m_end)
                r = true;
        }
    }
    return r;
}

CAiCell::CAiCell()
{
    Reset();
}
#pragma auto_inline reset

void CAiCell::Reset()
{
    m_soil = CAiMap::SOIL_DEFAULT;
    SetOccupied(false);
    SetName(false);
    SetAction(false);
    SetLink(false);
    SetBoundary(0);
    SetAltitude(0);
    SetShadow(0);
    SetInside(false);
    SetBadAlt(false);
    SetRtFlag(false);
    SetBilinear(false);
    m_astar_dir = 8;
    m_astar_f = -1;
    m_astar_g = -1;
}
