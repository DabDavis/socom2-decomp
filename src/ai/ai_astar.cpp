/* ai_astar.cpp (OVL2 0x510280-, SOCOM 1's unit: __sinit_ai_astar.cpp): the path planners and the
 * A* search. Only the planners' small virtuals so far (0x510280-0x51038B, the unit's first
 * functions, and CRestrainedPlanner's at 0x512CB0); defined here in reverse of the image's order
 * (-inline deferred). The map's
 * locations are CAiMapLoc's bitfields: x read from the word (unsigned compares), y from the
 * halfword at 2 (promoted to int: signed compares) -- the same source spelling for both. */

#include "ai.h"

/* the restrained planner's step: the search limited to 5000 cells, the base's test, no soil 6,
 * and no closed door (SOCOM 1's CRestrainedTravel, the same tests). The soil read once (the base's
 * test written out, so MW shares its load with the soil-6 test; through CAiCell::IsTraversable it
 * reloads it), the action's type through an inline byte enum compared `== 0` (only that order puts
 * the valid arm first without materialising the bit).
 * OPEN, right size, 24 instructions: registers only -- the target keeps the cell in a0, the soil
 * in a3 and the inlined test's bool in a2 (so `move a0,a1` for GetCellAction cannot take the
 * branch's delay slot), this build the cell in a2, the soil in a0, the bool in a3. IsTraversable
 * called on the cell gives the target's a0/a2 but loses the shared load; a local copy of the cell,
 * an unsigned char soil, the call unqualified were tried */
bool CRestrainedPlanner::is_traversable(CAiMap *map, CAiPath *path, const CAiMapLoc &from,
                                        CAiMap::DIR dir, CAiMapLoc &to, CAiCell *&cell)
{
    if (path->m_search_limit > 5000)
        path->m_search_limit = 5000;
    if (!CAiPlanner::is_traversable(map, path, from, dir, to, cell) || cell->m_soil == 6)
        return false;
    if (cell->m_action) {
        CAiMap::action *a = map->GetCellAction(to);

        if (a->GetType() == 1 && a->m_state == 0)
            return false;
    }
    return true;
}

/* whether a step may be taken into a cell: not into one of soils 0, 8, 9 or 10 (SOCOM 1's four
 * tests, which MW folds into one range test) */
bool CAiPlanner::is_traversable(CAiMap *, CAiPath *, const CAiMapLoc &, CAiMap::DIR, CAiMapLoc &,
                                CAiCell *&cell)
{
    unsigned int soil = cell->m_soil;

    return !(soil == 0 || soil == 10 || soil == 8 || soil == 9);
}

/* the Manhattan distance between two cells, ten to a cell (SOCOM 1's) */
int CAiPlanner::dist(CAiMap &, const CAiMapLoc &a, const CAiMapLoc &b)
{
    int dx = a.x > b.x ? a.x - b.x : b.x - a.x;
    int dy = a.y > b.y ? a.y - b.y : b.y - a.y;

    return (dx + dy) * 10;
}

bool CAiPlanner::is_goal(CAiMap &, const CAiMapLoc &a, const CAiMapLoc &b)
{
    return a.x == b.x && a.y == b.y;
}

/* nothing to forget (SOCOM 1's cleared a flag at 4; SOCOM II's is empty) */
void CAiPlanner::reset()
{
}
