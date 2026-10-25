/* ai_map.cpp (OVL2 0x515580-0x51A48F, SOCOM 1's unit: __sinit_ai_map.cpp, the demo's 0x185200-
 * 0x188E8F): CAiMap's cells -- the cell's reset, the walks' callbacks, the map's construction, the
 * soils and the eight directions -- and the map tables' template tail (the deleters' destructors
 * and clears, 0x5197A0-0x51A48F). SOCOM 1's functions in the demo's order (CAiCell(), the repel
 * parameters, TransformTo, GetMapRegion, ResetCells, ValidateCells and its callbacks, ~CAiMap,
 * CAiMap(), GetSoilByName, the GetClosestDirs, GetDirDelta, GetDirByName, GetDirQuat, GetDirVec);
 * SOCOM II adds the cell layers. Its initialiser (0x668630) builds the dummy cell (0x66A198) and
 * the cosine of 45 degrees (0x65AB38). Defined here in reverse of the image's order (-inline
 * deferred). */

#include <string.h>

#include "ai.h"

/* the cell a walk hands out off the map (SOCOM 1's, returned by CAiMap::dummy_cell) and the
 * cosine of 45 degrees, computed at start-up (GetDirCos reads it from data) */
static CAiCell s_dummy_cell;
static float s_cos45 = 0.5f * sqrtf(2.0f);

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
