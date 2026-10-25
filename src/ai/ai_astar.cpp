/* ai_astar.cpp (OVL2 0x510280-, SOCOM 1's unit: __sinit_ai_astar.cpp): the path planners and the
 * A* search. Only the planners' small virtuals so far (0x510280-0x51038B, the unit's first
 * functions); defined here in reverse of the image's order (-inline deferred). The map's
 * locations are CAiMapLoc's bitfields: x read from the word (unsigned compares), y from the
 * halfword at 2 (promoted to int: signed compares) -- the same source spelling for both. */

#include "ai.h"

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
