/* ai_astar.cpp (OVL2 0x510280-0x51541F, SOCOM 1's unit: __sinit_ai_astar.cpp): the path planners,
 * the planner queue and the A* search. SOCOM 1's functions in the demo's order (0x180AE0-0x185200:
 * PathPlanImmediate, TickPathPlanner, ResetPathPlan, AbortPathPlan, PathPlanExecute, PathPlanAbort,
 * PathPlanAddLeg, PathPlanBegin, FlushPlannerQueue, DeletePlanner, CreatePlanner, find_path, the
 * planners); SOCOM II puts the planners' small virtuals first (0x510280-0x51038B) and adds a list
 * the base planner's cost walks (0x66A0B8, filled from CVehicle's unit through 0x510530). Defined
 * here in reverse of the image's order (-inline deferred). The map's locations are CAiMapLoc's
 * bitfields: x read from the word (unsigned compares), y from the halfword at 2 (promoted to int:
 * signed compares) -- the same source spelling for both. */

#include "ai_maps.h"

/* what the queue's functions call that is not written yet, ai_path.cpp's (OVL2 0x524950-0x5281AF) */
extern "C" void func_00524950(CAiPath *path, const char *name);   /* the path drawn ("dbg_path") */
extern "C" void func_00527900(CAiPath *path);   /* a planning path back to the queue's state */
extern "C" void func_00527FC0(CAiPath *path, CAiPath::PATH_STATE state, int leg);

/* how long the planner runs in one tick, in T0 counts (SOCOM 1's timerlimit, 33) and when this
 * tick began */
static unsigned int timerlimit = 33;
static unsigned int s_plannerStart;

/* the path a caller is building leg by leg (SOCOM 1's ai::_plan_path), and the priority from which
 * a queued path stops the one planning (ai.h) */
namespace ai {
CAiPath *_plan_path;
float s_preemptPriority = 1.0f;
}

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

void CAiPath::SetPlanner(CAiPlanner *planner)
{
    m_planner = planner;
    m_custom = 1;
}

/* the search made for the path's current leg, with the planner its type asks for unless it has
 * its own (SOCOM 1's; the four planners are function statics, built by their vtable alone; the
 * CAiStar's two cells unbuilt -- SOCOM II's empty CAiMapLoc(), zentity.h's now) */
CAiStar *CAiPath::CreatePlanner()
{
    static CAiPlanner defaultPlanner;
    static CBasicPlanner basicPlanner;
    static CStealthPlanner stealthPlanner;
    static CRestrainedPlanner restrainedPlanner;

    if (!m_custom) {
        switch (m_type) {
        case PATH_DEFAULT:
            m_planner = &defaultPlanner;
            break;
        case PATH_BASIC:
            m_planner = &basicPlanner;
            break;
        case PATH_STEALTH:
            m_planner = &stealthPlanner;
            break;
        case PATH_RESTRAINED:
            m_planner = &restrainedPlanner;
            break;
        }
    }
    m_astar = new CAiStar(this);
    return m_astar;
}

void CAiPath::DeletePlanner(bool all)
{
    delete m_astar;
    m_astar = 0;
    if (m_custom && all) {
        delete m_planner;
        m_planner = 0;
        m_custom = 0;
    } else if (!m_custom) {
        m_planner = 0;
    }
}

/* the queue emptied and every map's cells checked again (SOCOM 1's) */
void ai::FlushPlannerQueue()
{
    path_queue.clear();
    CAiMaps::iterator i;

    i = AiMaps.begin();
    for (CAiMaps::iterator e = AiMaps.end(); i != e; ++i)
        (*i)->ValidateCells();
}

/* a path begun leg by leg: out of the queue, set up, and the one being built (SOCOM 1's) */
bool ai::PathPlanBegin(CAiPath *path)
{
    AbortPathPlan(path);
    path->plan_setup();
    _plan_path = path;
    return true;
}

bool ai::PathPlanAddLeg(CAiMap *map, const CAiMapLoc &start, const CAiMapLoc &goal)
{
    bool ok = _plan_path->plan_addleg(map, start, goal);

    return ok;
}

/* the path being built given up (SOCOM 1's cleared it and its state; SOCOM II's calls a setter) */
void ai::PathPlanAbort()
{
    CAiPath *path = _plan_path;

    path->Clear();
    func_00527FC0(path, CAiPath::UNKNOWN, -1);
    _plan_path = 0;
}

/* the path being built queued for planning: none of its legs, it is given up; queued already, it
 * stays where it is; else it goes into the queue by priority (CAiPathQueue::Insert, inlined one
 * level: every piece out of line). Its last leg is the one planned first (SOCOM 1's, with SOCOM II's
 * ordered insert where SOCOM 1 pushed onto the back) */
void ai::PathPlanExecute()
{
    CAiPath *path = _plan_path;

    if (path->m_legcount == 0) {
        AbortPathPlan(path);
        _plan_path = 0;
        return;
    }
    if (path->m_state != CAiPath::INQUEUE) {
        path->m_state = CAiPath::INQUEUE;
        path_queue.Insert(path);
    }
    path->m_planleg = path->m_legcount - 1;
    _plan_path = 0;
}

/* a path taken off the planner: a search under way is dropped and the path told it is unplanned,
 * a queued one told it failed (SOCOM 1's two arms); SOCOM II also looks for it in the queue when
 * its state says neither.
 * OPEN (0x174 against 0x194, re-measured with master's list<T *> pieces, 2026-10-25): an inline
 * height. The target's third arm calls list<CAiPath *>'s begin() (0x5109C0), end() (0x5109B0,
 * twice), std::find (0x510970) and != (0x510950) out of line, in that order, with a copy of the
 * path (sp+80) as find's value -- CAiPathQueue::Contains taking the path by value, inlined one
 * level. With include/list and include/algorithm as they stand Contains is at most five high and
 * inlines whole (find's loop in place, 0x174); the original's MSL was deeper (README "Level 6?"),
 * as PathPlanExecute's Insert, inlined one level by its size, does not show */
bool ai::AbortPathPlan(CAiPath *path)
{
    if (path->m_state == CAiPath::PLANNING) {
        path->DeletePlanner(true);
        path->plan_complete(CAiPath::UNKNOWN);
        path_queue.remove(path);
    } else if (path->m_state == CAiPath::INQUEUE) {
        path->plan_complete(CAiPath::FAILED);
        path_queue.remove(path);
    } else if (path_queue.Contains(path)) {
        path->plan_complete(CAiPath::FAILED);
        path_queue.remove(path);
    }
    return true;
}

/* the search at the head of the queue dropped and its path back to waiting (SOCOM 1's; SOCOM II
 * keeps a planner the path owns) */
void ai::ResetPathPlan()
{
    if (path_queue.size()) {
        CAiPath *path = path_queue.front();

        if (path->m_state == CAiPath::PLANNING) {
            path->DeletePlanner(false);
            func_00527900(path);
        }
    }
}

/* the scheduler's task: the queue's searches run until the tick's time is up (SOCOM 1's) */
bool ai::TickPathPlanner(float, void *)
{
    s_plannerStart = *(volatile unsigned int *)0x10000000;
    while (path_queue.size() != 0 && *(volatile unsigned int *)0x10000000 - s_plannerStart < timerlimit) {
        CAiPath *path = path_queue.front();

        switch (path->m_state) {
        case CAiPath::INQUEUE:
            if (!path->CreatePlanner()) {
                path->m_state = CAiPath::FAILED;
                break;
            }
        case CAiPath::PLANNING:
            path->m_astar->find_path(path);
            if (path->m_state != CAiPath::PLANNING) {
                path->DeletePlanner(true);
                if (!path->plan_legcomplete(path->m_state)) {
                    path_queue.pop_front();
                    path_queue.m_completed++;
                }
            }
            path_queue.m_ticks++;
            break;
        case CAiPath::UNKNOWN:
        case CAiPath::COMPLETE:
        case CAiPath::FAILED:
            func_00524950(path, "dbg_path");
            path_queue.pop_front();
            break;
        }
    }
    return false;
}

/* a path planned at once, the queue bypassed and the tick's time limit lifted (SOCOM 1's) */
bool ai::PathPlanImmediate(CAiMap *map, const CAiMapLoc &start, const CAiMapLoc &goal, CAiPath *path)
{
    if (path->m_state == CAiPath::PLANNING)
        return false;
    if (path->m_state == CAiPath::INQUEUE)
        path_queue.remove(path);
    unsigned int limit = timerlimit;

    timerlimit = -1;
    path->plan_setup();
    if (path->plan_addleg(map, start, goal)) {
        path->m_state = CAiPath::INQUEUE;
        CAiStar *star = path->CreatePlanner();

        star->find_path(path);
        path->DeletePlanner(true);
        path->plan_complete(path->m_state);
    }
    timerlimit = limit;
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
