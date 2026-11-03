/* ai_astar.cpp (OVL2 0x510280-0x51557F, SOCOM 1's unit: __sinit_ai_astar.cpp): the path planners,
 * the planner queue and the A* search. SOCOM 1's functions in the demo's order (0x180AE0-0x185200:
 * PathPlanImmediate, TickPathPlanner, ResetPathPlan, AbortPathPlan, PathPlanExecute, PathPlanAbort,
 * PathPlanAddLeg, PathPlanBegin, FlushPlannerQueue, DeletePlanner, CreatePlanner, find_path, the
 * planners); SOCOM II puts the planners' small virtuals first (0x510280-0x51038B) and adds a list
 * the base planner's cost walks (0x66A0B8, filled from CVehicle's unit through 0x510530). Defined
 * here in reverse of the image's order (-inline deferred). The map's locations are CAiMapLoc's
 * bitfields: x read from the word (unsigned compares), y from the halfword at 2 (promoted to int:
 * signed compares) -- the same source spelling for both. The A* search (find_path, its step and
 * find_open helpers, ~CAiStar, Init) runs over two unit statics, the open list (a heap of nodes by
 * f, through include/algorithm's push_heap/pop_heap/__insert_heap_down) and the closed list of
 * cells; the template tail runs from the open deleter's clear 0x5132A0 to the statics'
 * destructors (0x515420-0x51557F). */

#include "ai_maps.h"
#include "../gamez/zsys/zsys.h"

/* what the queue's functions call that is not written yet, ai_path.cpp's (OVL2 0x524950-0x5281AF) */
extern "C" void func_00527900(CAiPath *path);   /* a planning path back to the queue's state */
extern "C" void func_00527FC0(CAiPath *path, CAiPath::PATH_STATE state, int leg);

/* the overlays the planners keep off (new in SOCOM II: 0x66A0B8) */
CAiOverlayList ai::overlays;

/* the open list: the nodes left to visit, a heap by f (SOCOM 1's CAiStar::m_open; a unit static in
 * SOCOM II, 0x66A0E8 -- its destructor 0x5154A0 four this-tests, a class of its own over the
 * vector). A node goes on only while there is room: Init reserves the storage once and a full
 * list fails the search (ASTAR_FAILURE_UNKNOWN) -- the push an inline bool (the names ours) */
class CAiOpenList : public std::vector<node> {
public:
    bool Push(const node &n)
    {
        if (size() < capacity()) {
            push_back(n);
            return true;
        }
        return false;
    }
};

/* the closed list: the cells visited (SOCOM II's are cells, SOCOM 1's nodes; 0x66A108, its
 * destructor 0x515420) */
class CAiClosedList : public std::vector<CAiMapLoc> {
public:
    bool Push(const CAiMapLoc &loc)
    {
        if (size() < capacity()) {
            push_back(loc);
            return true;
        }
        return false;
    }
};

/* how long the planner runs in one tick, in T0 counts (SOCOM 1's timerlimit, 33) and when this
 * tick began */
static unsigned int timerlimit = 33;
static unsigned int s_plannerStart;
/* the heap's order, passed by value (a byte read off this static: SOCOM 1's gp byte alike) */
static std::greater<node> s_order;
static CAiOpenList s_open;
static CAiClosedList s_closed;
/* the longest each list has been (written by ~CAiStar, read by nothing in the image) */
static unsigned int s_maxOpen;
static unsigned int s_maxClosed;
/* the path's bias toward its type, the stealth and basic planners' weight (0x64F8D0, 1.0f: the
 * search sets it from the path) */
static float s_typeBias = 1.0f;
/* the directions a node is left by: all eight in order, or the few a door allows (filled per node) */
static char s_allDirs[8] = {0, 1, 2, 3, 4, 5, 6, 7};
static char s_doorDirs[8] = {0, 1, 2, 3, 4, 5, 6, 7};

/* the path a caller is building leg by leg (SOCOM 1's ai::_plan_path), and the priority from which
 * a queued path stops the one planning (ai.h) */
namespace ai {
CAiPath *_plan_path;
float s_preemptPriority = 1.0f;
/* the planner queue (SOCOM 1's; 0x66A180, built after the lists) */
CAiPathQueue path_queue;
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

/* the lists' storage reserved: 1024 open nodes, 24576 closed cells (0x5128F0) */
void CAiStar::Init()
{
    s_open.reserve(1024);
    s_closed.reserve(24576);
}

/* the lists' storage given up (the name ours): dead-stripped in the image, but its FreeVector
 * copies over the closed list's vector -- ~vector, the deleter's swap, clear and vector(), in
 * reverse of their use, 0x512820-0x5128DF between ~CAiStar and Init -- survive for CAiMaps::
 * FreeTables, and its use of the vector's destructor is what has the tail's reserves and inserts
 * inline the deleter's destructor (generated first) */
void CAiStar::Shutdown()
{
    FreeVector<std::vector<node> >(s_open);
    FreeVector<std::vector<CAiMapLoc> >(s_closed);
}

/* the search given up: every cell either list holds back to unvisited (no direction, the open
 * cells' f and g -1, the closed cells' g), the lists' longest kept, both emptied -- the deleters'
 * clears called out of line (SOCOM 1's freed the star's own lists) */
CAiStar::~CAiStar()
{
    for (std::vector<node>::iterator i = s_open.begin(); i != s_open.end(); ++i) {
        CAiCell *cell = m_map->GetCell((*i).m_loc, 0);

        cell->m_astar_dir = 8;
        cell->m_astar_f = -1;
        cell->m_astar_g = -1;
    }
    for (std::vector<CAiMapLoc>::iterator j = s_closed.begin(); j != s_closed.end(); ++j) {
        CAiCell *cell = m_map->GetCell(*j, 0);

        cell->m_astar_dir = 8;
        cell->m_astar_g = -1;
    }
    if (s_maxOpen < s_open.size())
        s_maxOpen = s_open.size();
    if (s_maxClosed < s_closed.size())
        s_maxClosed = s_closed.size();
    s_open.clear();
    s_closed.clear();
}

/* the open node on a cell: none unless the cell is open (f -1 marks it closed), each node tried
 * counted as searched */
std::vector<node>::iterator CAiStar::find_open(const CAiMapLoc &loc)
{
    CAiCell *cell = m_map->GetCell(loc, 0);

    if (cell && cell->m_astar_f != -1) {
        for (std::vector<node>::iterator i = s_open.begin(); i != s_open.end(); ++i) {
            m_stats->nodes_searched++;
            if ((*i).m_loc == loc)
                return i;
        }
    }
    return s_open.end();
}

bool CAiStar::step(CAiPath *path, const CAiMapLoc &from, CAiMap::DIR dir, CAiMapLoc &to, CAiCell *&cell)
{
    bool ok = m_map->GetNeighbor(from, dir, to, cell);

    if (ok) {
        if (!m_planner->is_traversable(m_map, path, from, dir, to, cell))
            ok = false;
        else if (cell->IsOccupied() && (path->m_flags & 0x10))
            ok = false;
    }
    return ok;
}

/* a limit passed: none set (0), or below the count (the name ours: materialised, then tested) */
inline bool over_limit(unsigned int limit, unsigned int n)
{
    return limit && limit < n;
}

/* the trace back's step: GetNeighbor's, the cell's traversability asked and its answer dropped --
 * the target calls IsTraversable on the cell reached and ignores it (the name ours) */
inline bool next_cell(CAiMap *map, const CAiMapLoc &loc, CAiMap::DIR dir, CAiMapLoc &out, CAiCell *&cell)
{
    int x = loc.x + CAiMap::neighbor[dir][0];
    int y = loc.y + CAiMap::neighbor[dir][1];

    if (x >= 0 && x < map->m_nx && y >= 0 && y < map->m_ny) {
        out.x = x;
        out.y = y;
        out.m_mapid = map->GetMapId();
        cell = map->GetCell(out, 0);
        return cell && cell->IsTraversable();
    }
    return false;
}

/* the A* search of the path's current leg, run until the tick's time is up (SOCOM 1's, over the
 * unit's lists): a new search starts from its first cell; each pass takes the cheapest open node,
 * closes it and stops at the goal or a limit, else opens or improves its neighbours -- all eight
 * directions, or the ones a door on the cell allows; done, the path's points are traced back from
 * the goal by the directions the cells keep.
 * OPEN, 0xF30 against 0xF48 (the calls, their order and the frame's slots right): the target keeps
 * the nodes' cost fields' ADDRESSES in saved registers (s0/s1 = &cur.m_g/&cur.m_h in the first
 * block, s8 = &cur.m_g and s1/s0 = &n.m_g/&n.m_h through the loop, `lh 0(s8)`), where these plain
 * field accesses fold into sp offsets -- as if the costs were reached through references (accessors
 * returning short &?); SOCOM 1's find_path does the same. The rest follows from the registers */
void CAiStar::find_path(CAiPath *path)
{
    node cur;

    m_stats->m_ticks++;
    if (path->m_state == CAiPath::INQUEUE) {
        m_planner->reset(*m_map, m_start, m_goal);
        cur.m_loc = m_start;
        cur.m_g = 0;
        cur.m_h = m_planner->dist(*m_map, m_start, m_goal);
        s_open.Push(cur);

        CAiCell *cell = m_map->GetCell(m_start, 0);

        cell->m_astar_f = cur.f();
        cell->m_astar_g = cur.m_g;
        m_stats->nodes_added++;
        path->m_state = CAiPath::PLANNING;
    }

    CAiPath::ASTAR_STATUS status = CAiPath::ASTAR_SUCCESS;
    bool intime = true;

    s_typeBias = path->m_type_bias;
    bool found = false;

    while (s_open.size() != 0 && (intime = *(volatile unsigned int *)0x10000000 - s_plannerStart < timerlimit)) {
        cur = *s_open.begin();
        std::pop_heap(s_open.begin(), s_open.end(), s_order);
        s_open.pop_back();

        CAiCell *cell = m_map->GetCell(cur.m_loc, 0);

        cell->m_astar_f = -1;
        if (!s_closed.Push(cur.m_loc)) {
            status = CAiPath::ASTAR_FAILURE_UNKNOWN;
            break;
        }
        m_stats->nodes_removed++;
        m_stats->m_current = cur.m_loc;
        found = m_planner->is_goal(*m_map, cur.m_loc, m_goal);
        if (found)
            break;
        if (m_stats->m_ticks >= 200)
            status = CAiPath::ASTAR_FAILURE_MANYTICKS;
        else if (over_limit(path->m_search_limit, m_stats->nodes_searched))
            status = CAiPath::ASTAR_FAILURE_TOOLONG;
        else if (over_limit(path->m_unk90, m_stats->m_ticks))
            status = CAiPath::ASTAR_FAILURE_TOOLONG;
        if (status != CAiPath::ASTAR_SUCCESS)
            break;

        const char *dirs = s_allDirs;
        int ndirs = 8;

        if (cell->HasAction()) {
            CAiMap::action *a = m_map->GetCellAction(cur.m_loc);

            if (a->GetType() == 6) {
                ndirs = 1;
                dirs = s_doorDirs;
                s_doorDirs[0] = a->m_direction;
            } else if (a->m_direction != 8) {
                ndirs = 2;
                s_doorDirs[0] = a->m_direction;
                s_doorDirs[1] = (s_doorDirs[0] + 4) % 8;
                if (!a->IsDoor()) {
                    s_doorDirs[2] = (s_doorDirs[0] + 9) % 8;
                    s_doorDirs[3] = (s_doorDirs[0] + 7) % 8;
                    s_doorDirs[4] = (s_doorDirs[1] + 9) % 8;
                    s_doorDirs[5] = (s_doorDirs[1] + 7) % 8;
                    s_doorDirs[6] = (s_doorDirs[0] + 10) % 8;
                    s_doorDirs[7] = (s_doorDirs[0] + 6) % 8;
                    ndirs = 8;
                }
                dirs = s_doorDirs;
            }
        }
        for (int i = 0; i < ndirs; i++) {
            CAiMap::DIR d = (CAiMap::DIR)dirs[i];
            CAiMapLoc to;
            CAiCell *ncell = 0;

            if (!step(path, cur.m_loc, d, to, ncell))
                continue;
            if (ncell->HasAction()) {
                CAiMap::action *a = m_map->GetCellAction(to);

                if (a->m_type == 1 || a->m_type == 4) {
                    unsigned int ad = a->m_direction;

                    if (ad != 8 && d != ad && (d + 4) % 8 != ad)
                        continue;
                }
            }

            node n;
            float c = 10.0f * m_planner->fcost(*m_map, cur.m_loc, d, to);

            n.m_loc = to;
            n.m_g = cur.m_g + (int)c;
            n.m_h = m_planner->dist(*m_map, to, m_goal);
            if (ncell->m_astar_dir == 8) {
                ncell->m_astar_dir = (d + 4) % 8;
                ncell->m_astar_f = n.f();
                ncell->m_astar_g = n.m_g;
                if (!s_open.Push(n)) {
                    status = CAiPath::ASTAR_FAILURE_UNKNOWN;
                    break;
                }
                std::push_heap(s_open.begin(), s_open.end(), s_order);
                m_stats->nodes_added++;
            } else if (ncell->m_astar_f != -1 && n.m_g < ncell->m_astar_g) {
                std::vector<node>::iterator it = find_open(to);

                if (it == s_open.end()) {
                    status = CAiPath::ASTAR_FAILURE_UNKNOWN;
                    break;
                }
                ncell->m_astar_dir = (d + 4) % 8;
                ncell->m_astar_g = n.m_g;
                ncell->m_astar_f = n.f();
                (*it).m_g = n.m_g;
                std::push_heap(s_open.begin(), it + 1, s_order);
            }
        }
    }
    if (!found) {
        if (s_open.size() == 0)
            status = CAiPath::ASTAR_FAILURE_NONEOPEN;
        else if (!intime)
            status = CAiPath::ASTAR_FAILURE_OUTOFTIME;
    }
    if (status == CAiPath::ASTAR_SUCCESS) {
        path->m_state = CAiPath::COMPLETE;
        path->m_status = status;

        CAiMapLoc loc(cur.m_loc);

        m_stats->path_cost = cur.m_g;

        CAiCell *cell = m_map->GetCell(loc, 0);
        CAiMap::DIR dir = CAiMap::DIR_COUNT;

        while (loc != m_start) {
            CAiMap::DIR last = dir;

            dir = (CAiMap::DIR)cell->m_astar_dir;
            if (last != CAiMap::DIR_COUNT && dir == (last + 4) % 8)
                break;
            path->m_points.push_back(CAiPath::point(CAiMapLoc(loc.x, loc.y, m_map->GetMapId())));

            CAiMapLoc next;

            next_cell(m_map, loc, dir, next, cell);
            loc = next;
            m_stats->path_length++;
        }
        path->m_points.push_back(CAiPath::point(m_start));
        m_stats->nodes_left = s_open.size();
        m_stats->nodes_visited = s_closed.size();
    } else if (status != CAiPath::ASTAR_FAILURE_OUTOFTIME) {
        path->m_state = CAiPath::FAILED;
        path->m_status = status;
    }
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
            path->PrintPath("dbg_path");
            path_queue.pop_front();
            break;
        }
    }
    return false;
}

/* an overlay pushed onto the list (new in SOCOM II, the name ours) */
void ai::AddOverlay(CAiOverlay *overlay)
{
    overlays.push_back(overlay);
}

/* the overlays dropped, not deleted (new in SOCOM II, the name ours) */
void ai::ClearOverlays()
{
    overlays.clear();
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

/* nothing to forget (SOCOM 1's cleared a flag at 4; SOCOM II's is empty, and takes the search's map
 * and ends) */
void CAiPlanner::reset(CAiMap &, const CAiMapLoc &, const CAiMapLoc &)
{
}
