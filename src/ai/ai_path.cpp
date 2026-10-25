/* ai_path.cpp (OVL2, around 0x524000-0x52852F; SOCOM 1's unit, 0x193540-0x196E60 in the demo): the
 * planned path, CAiPath -- its points, legs and plan states. SOCOM 1's functions in the demo's
 * order (PointInPath ... reset__12CAiPathStats, then the template tail with _min<Ui>); SOCOM II's
 * fields sit where ai.h says. The plan's states and legs so far (0x5267A0, 0x527300-0x527CD7)
 * and the constructor (0x528340);
 * defined here in reverse of the image's order (-inline deferred). */

#include "ai.h"

/* what the path's functions call that is not written yet (this unit's) */
extern "C" void func_00526380(CAiPath *path, int leg);   /* the points made from the legs */
extern "C" void func_00525520(CAiPath *path);   /* SOCOM 1's UpdateTurnAngles, by position */

/* a path with no plan: the vectors empty, the tallies reset, flags 3 and the type's bias 1
 * (SOCOM 1's, the new fields zeroed in among the old) */
CAiPath::CAiPath()
{
    m_state = UNKNOWN;
    m_type = PATH_DEFAULT;
    m_type_bias = 1.0f;
    m_astar = 0;
    m_planner = 0;
    m_pathId = 0;
    m_legcount = 0;
    m_planleg = 0;
    m_curleg = 0;
    m_app_flags = 0;
    m_unk24 = 0;
    m_flags = 3;
    m_length = 0.0f;
    m_template = 0;
    m_overlay = 0;
    m_snake_len = 0;
    m_priority = 0.0f;
    m_custom = 0;
    m_tolerance = 0;
    m_search_limit = 0;
    m_unk90 = 0;
}

/* the legs' two ends, for one leg or the one being planned (SOCOM 1's; true either way) */
bool CAiPath::GetLegPoints(CAiMapLoc &start, CAiMapLoc &goal, int leg) const
{
    if (leg == -1)
        leg = m_planleg;
    if (leg < m_legcount) {
        start = m_waypts.data_[leg * 2];
        goal = m_waypts.data_[leg * 2 + 1];
    }
    return true;
}

/* a leg's map, or the planned leg's, or none past the last (SOCOM 1's) */
CAiMap *CAiPath::GetLegMap(int leg)
{
    if (leg == -1)
        leg = m_planleg;
    return leg < m_legcount ? m_maps[leg] : 0;
}

/* a plan begun afresh: the points, the maps and the cells dropped, the tallies reset, no state
 * and no legs (SOCOM 1's, the stats' reset inlined in SOCOM II) */
void CAiPath::plan_setup()
{
    m_points.clear();
    m_maps.clear();
    m_waypts.clear();
    m_stats.reset();
    m_state = UNKNOWN;
    m_legcount = 0;
    m_planleg = 0;
}

/* the plan finished in a state: the leg counters back to the start, the points made and its id
 * moved on (SOCOM 1's, inlined there; SOCOM II calls it from ai_astar.cpp and from
 * plan_legcomplete, never inlined) */
#pragma auto_inline off
void CAiPath::plan_complete(PATH_STATE state)
{
    m_state = state;
    m_planleg = 0;
    m_curleg = 0;
    PostProcess();
    m_pathId++;
}
#pragma auto_inline reset

/* a leg's search over: true while legs remain to plan, the next leg queued; else the plan
 * completes in the state the search left (SOCOM 1's).
 * OPEN (0x7C against 0xAC): the target keeps two compares of m_planleg against m_legcount whose
 * bodies compiled to nothing -- the first a branch to the next instruction, the second with
 * `more = true` in its delay slot (SOCOM 1's called the maps' data() out of line in both). Every
 * spelling tried -- an if, a ternary statement, a dead local, GetLegMap(-1) -- is deleted whole */
bool CAiPath::plan_legcomplete(PATH_STATE state)
{
    bool more = false;

    m_astar = 0;
    if (state == COMPLETE) {
        if (m_planleg) {
            m_planleg < m_legcount ? m_maps[m_planleg] : 0;
            m_planleg--;
            m_state = INQUEUE;
            m_stats.path_length = 0;
            m_stats.path_cost = 0;
            m_stats.m_ticks = 0;
            m_stats.m_time = 0.0f;
            more = true;
            m_planleg < m_legcount ? m_maps[m_planleg] : 0;
        } else {
            plan_complete(state);
        }
    } else {
        plan_complete(state);
    }
    return more;
}

/* the path emptied, its plan aborted first; false while it is still planning (SOCOM 1's). The
 * second test written out as `> 2` (through IsPlanning or as `>= 3` it is materialised) */
bool CAiPath::Clear()
{
    if (IsPlanning())
        ai::AbortPathPlan(this);
    if ((unsigned int)(m_state - 1) > 2) {
        m_points.clear();
        m_waypts.clear();
        m_maps.clear();
        m_stats.reset();
        m_legcount = 0;
        m_planleg = 0;
        return true;
    }
    return false;
}

void CAiPath::SetTemplatePath(CAiPath *path)
{
    m_template = path;
}

/* the finished path's points made, smoothed and widened, and drawn when asked (SOCOM 1's;
 * plan_complete calls it, never inlined) */
#pragma auto_inline off
void CAiPath::PostProcess()
{
    func_00526380(this, 0);
    func_00525520(this);
    if (m_flags & 4)
        UpdatePathWidth(5);
    if (m_flags & 0x40)
        PrintPath("dbg_path");
}
#pragma auto_inline reset
