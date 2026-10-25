/* ai_map.cpp (OVL2 0x515580-0x51A48F, SOCOM 1's unit: __sinit_ai_map.cpp, the demo's 0x185200-
 * 0x188E8F): CAiMap's cells -- the cell's reset, the walks' callbacks, the map's construction, the
 * soils and the eight directions -- and the map tables' template tail (the deleters' destructors
 * and clears, 0x5197A0-0x51A48F). SOCOM 1's functions in the demo's order (CAiCell(), the repel
 * parameters, TransformTo, GetMapRegion, ResetCells, ValidateCells and its callbacks, ~CAiMap,
 * CAiMap(), GetSoilByName, the GetClosestDirs, GetDirDelta, GetDirByName, GetDirQuat, GetDirVec);
 * SOCOM II adds the cell layers (CAiMapLayer, whose own code sits after vehicle_rdr.cpp, from
 * ~0x53A730). Its initialiser (0x668630) builds the dummy cell (0x66A198) and the cosine of 45
 * degrees (0x65AB38). Defined here in reverse of the image's order (-inline deferred).
 * Not written yet: 0x515750 (the cell's altitude?, called from the fcosts), the vehicle's cell
 * marking 0x515880 and its helpers 0x515900-0x5168DF (copies 0x515DE0, 0x515F80, 0x516180), the
 * repel parameters 0x5169B0/0x516F10, 0x517050, ValidateCells 0x517850 (the walk with
 * _validate_cell plus GetCell inlined one level -- IsCell 0x512140, IsLayer 0x512120 and the
 * pieces after them -- and out of line, 0x511F80), ~CAiMap 0x5181F0, GetClosestDir(cell, point)
 * 0x518900, GetDirByName 0x518D80 (its draft matches), the scanlines' resize/insert/erase
 * (0x519700, 0x519DB0, 0x519B90) and the pod insert copy 0x5192E0. */

#include <string.h>

#include "ai.h"

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
 * copy-constructed from it, the second assigned -- for a CBBox to be assigned from) */
struct CBBoxInit {
    CBBoxInit(CPnt3D p) : m_min(p) { m_max = p; }

    CPnt3D m_min;
    CPnt3D m_max;
};

/* the map with its first layer pushed onto the layers and nothing read: an empty box, no size,
 * floor -1, resolution 0.5 and slope pi/4 (SOCOM 1's, the id and the layers new); the dummy
 * cell's soil set to noalt again.
 * OPEN (0x1D8 against 0x1DC): the box's temporaries. Every store and call is the target's, in its
 * order; the target addresses the point's copy (sp+80) and the second corner (sp+60) through
 * registers for the field-by-field copy between them, as an inlined assignment taking references
 * would (SOCOM 1's PNT3D had a written operator=, 0x17FC00), where MW folds this stand-in's to
 * sp offsets -- one register and one instruction fewer. A setter taking `const CPnt3D &` and one
 * copying the fields through a `const PNT3D &` fold the same; the real spelling likely needs
 * zmath.h's CBBox constructor (and CPnt3D's assignment) as SOCOM II wrote them. ~CAiMap (0x5181F0)
 * opens with the same box */
CAiMap::CAiMap(int mapid) : m_layer0(0)
{
    s_dummy_cell.m_soil = SOIL_NOALT;
    m_bbox = (const CBBox &)CBBoxInit(CPnt3D::zero);
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
 * OPEN (0x250 against 0x258): only the box, CAiMap(int)'s question above -- the target copies the
 * point into the second corner and that corner into m_bbox through registers (sp+80 in a2, sp+60
 * in v0), where MW folds the stand-in's to sp offsets; every call and member destructor after it
 * is the target's, in its order */
CAiMap::~CAiMap()
{
    m_bbox = (const CBBox &)CBBoxInit(CPnt3D::zero);
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

/* every cell of the first layer reset through _reset_cell (SOCOM 1's ResetCells, a loop of its own
 * there): the walk inlined one level, its pieces called out of line in reverse of their first use
 * after it (0x5176D0-0x51784F, all byte-exact from here); false whatever the walk did.
 * OPEN, right size, 44 instructions: the region's reads. The target keeps &r in s5 and reads the
 * row through it twice (`lhu v0,2(s5)` for y0, again for y1 ahead of GetDy's call, held in s0
 * across it); here MW resolves the pointer to sp+176, shares the two loads and calls GetDy first.
 * Tried: r read directly (the same), through the walk struct's m_region (the loads and the call's
 * order right, the pointer reloaded from the struct after each call), the walk taking the region
 * by reference with region() and GetMapRegion in ResetCells (resolved to sp+176 again), y1 built
 * in a register local (the loads shared). The target's reads behave as through a pointer MW cannot
 * resolve, kept in a register: what spelling gives that is open */
bool CAiMap::ResetCells()
{
    ForEachCell(_reset_cell, 0, 0);
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

/* called, not inlined, by the initialiser (the target's jal 0x515720): pinned */
#pragma auto_inline off
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
