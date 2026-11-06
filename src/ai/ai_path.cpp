/* ai_path.cpp (OVL2, around 0x524000-0x52852F; SOCOM 1's unit, 0x193540-0x196E60 in the demo): the
 * planned path, CAiPath -- its points, legs and plan states. SOCOM 1's functions in the demo's
 * order (PointInPath ... reset__12CAiPathStats, then the template tail with _min<Ui>); SOCOM II's
 * fields sit where ai.h says. The plan's states and legs so far (0x5267A0, 0x527300-0x527CD7)
 * and the constructor (0x528340);
 * defined here in reverse of the image's order (-inline deferred). */

#include "ai_maps.h"

/* ai.h's _min defined (as ai_map.cpp does: MW auto-inlines no template function -- the unsigned
 * instance, AddSnakePoint's, sits in this unit's template tail, 0x528510) */
namespace ai {
template <class T>
const T &_min(const T &a, const T &b)
{
    return a <= b ? a : b;
}
}

/* the cell no map has: every field's bits set (SOCOM 1's invalid__9CAiMapLoc). This unit's static
 * (OVL2 .data 0x65AED0, just ahead of s_preemptPriority and PrintPath's strings), built by
 * __sinit_ai_path.cpp (0x668680) through the three-field constructor -- the mem-initializers' word
 * read-mask-writes, the map id first */
const CAiMapLoc CAiMapLoc::invalid(0x1FFF, 0x1FFF, 0x3F);

/* the priority from which a queued path stops the one planning (ai.h; 1.0f). This unit's constant,
 * not ai_astar.cpp's (the AI core's round 23): it sits at 0x65AED8 between invalid and PrintPath's
 * strings, in the area that holds the units' const objects (CAiEvent::NoExpire, the planners' RTTI,
 * CAiMap::action::invalid, invalid) and their strings, unit by unit in link order -- ai_astar.cpp's
 * initialised variables (s_typeBias, timerlimit, the direction lists) are at 0x64F8D0 on, in the
 * other area, with ai_events.cpp's (a probe: MW puts const objects in .rodata, initialised
 * variables in .data). Only PathPlanExecute's CAiPathQueue::Insert reads it, from the other unit,
 * so the lwc1 stays */
namespace ai {
const float s_preemptPriority = 1.0f;
}


/* a vector's storage cut to its size: a copy of it swapped in (node_world.cpp's, the name ours):
 * inlined one level, its copy constructor, swap and destructor called out of line */
template <class V>
inline void TrimVector(V &v)
{
    V tmp(v);

    v.swap(tmp);
}

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

/* the planner let go, the plan aborted and the path emptied (SOCOM 1's: Clear inlined).
 * OPEN (0x180 against 0x188): only the stats' visited cells' destruction -- the target tests
 * their address three times before ~__vector_deleter (a CAiMapLocVec, as SOCOM 1's DWARF has
 * it), the plain std::vector here twice. Declared CAiMapLocVec the destructor is byte-exact but
 * for 0x515340's name (the points' deleter destructor), and CAiPath() loses: CAiPathStats() is
 * then six high (CAiMapLocVec() five) and inlined one level -- with the body written out instead
 * of through reset() as well. A level-6 question (README "Open, and next") */
CAiPath::~CAiPath()
{
    DeletePlanner(true);
    Clear();
}

/* another path's finished plan taken over -- its flags, legs, maps, tallies, state, length, limits
 * and points -- unless this one is queued or planning; the id moves on and the path is drawn when
 * asked (new in SOCOM II, the name ours: 0x527FD0, the SEALs' 0x555420, 0x5556A0 and 0x562790 call it).
 * The vectors through their assignments: the cells' and the stats' through do_assign<const
 * CAiMapLoc *> (0x51E7A0), the maps' through the 4-byte pod's operator= (0x528C30), the points'
 * through do_assign<const point *> (0x5287D0, this unit's tail, byte-exact). The state's two tests
 * written out on one read (IsPlanning() reloads it and materialises the range test).
 * Open until round 26 (21 instructions, the right size): the stats' copy -- the target takes the other stats' visited
 * cells' address into a register before the nine words (v1; the words through v0) and reads their
 * data through it, the size straight; the implicit CAiPathStats assignment here folds both reads into
 * offsets of `path` and swaps the two registers. As a CAiMapLocVec (SOCOM 1's DWARF) the visited cells
 * give the tag temporaries' slots (19 off) but cost CAiPath() a level (~CAiPath's note); a written
 * inline assignment of the stats does not help (21 and 23). The AI core round 21: the visited cells
 * reached through a reference local (`const std::vector<CAiMapLoc> &v = path->m_stats.m_visited;`,
 * the other words copied field by field, then `m_stats.m_visited = v;`) gives the target's v1 for
 * their address and the data read through it -- 8 off, 7 with the copy in an inline taking both stats
 * by pointer -- but then reads the size through v1 too (the target: off s1), copies m_current with
 * lw/sw (the implicit assignment's lwc1/swc1 lost) and keeps the tag slots swapped
 * (build/scratch/ai/v_copy.py). The AI core round 25: include/vector's class operator= reading the
 * other's data through the deleter's data() reference (`(const T *)const_cast<__vector_imp &>(x).
 * data()`, both ends) gives the target's v1 and `lw a1,8(v1)` with the size off s1 -- 2 off, only
 * the two forward_iterator_tag slots (68/72) swapped; with m_visited as CAiMapLocVec as well, this
 * and ~CAiPath match and CAiPath() is the level too tall (an MSL-lane header change: char_type.cpp
 * scores the same either way). In include/vector since the MSL headers round 24 (no other
 * hand-written object moves): 2 off here. BYTE-EXACT since the AI core round 26: the stats copied
 * through an inline taking both by reference (by pointer alike) -- the implicit assignment is no
 * inline level of its own, so written in place the visited cells' operator= sat at the points' level
 * and its tag temporary took the slot above theirs; one level down it takes the frame's lowest slot
 * (68) as the target's, the kit's round-33 rule (build/scratch/ai/v_cp.py) */
inline void _copy_stats(CAiPathStats &d, const CAiPathStats &s)
{
    d = s;
}

bool CAiPath::CopyPlan(const CAiPath *path)
{
    bool ok = false;

    if (m_state != INQUEUE && (unsigned int)(m_state - 1) > 2 && path->HasPoints()) {
        ok = true;
        m_flags = path->m_flags;
        m_app_flags = path->m_app_flags;
        m_legcount = path->m_legcount;
        m_planleg = path->m_planleg;
        m_curleg = path->m_curleg;
        m_waypts = path->m_waypts;
        m_maps = path->m_maps;
        _copy_stats(m_stats, path->m_stats);
        m_state = path->m_state;
        m_status = path->m_status;
        m_length = path->m_length;
        m_tolerance = path->m_tolerance;
        m_search_limit = path->m_search_limit;
        m_unk90 = path->m_unk90;
        m_snake_len = path->m_snake_len;
        m_points = path->m_points;
        m_pathId++;
        if (m_flags & 0x40)
            PrintPath("dbg_path");
    }
    return ok;
}

/* the points' length: summed afresh when none is kept or when forced, each step between two
 * points on one map its straight distance; 0 without points (SOCOM 1's GetLength, the demo's
 * 0x196510, the same shape: SOCOM II's distance a sqrt.s where SOCOM 1 called sqrtf) */
float CAiPath::GetLength(bool force)
{
    if (m_points.size() != 0 && (m_length == 0.0f || force)) {
        m_length = 0.0f;
        points::iterator i = m_points.begin();
        CAiMapLoc prev;
        CAiMapLoc cur;

        prev = (*i).m_loc;
        for (++i; i != m_points.end(); ++i) {
            cur = (*i).m_loc;
            if (prev.m_mapid == cur.m_mapid)
                m_length += GetCellDistance(prev, cur);
            prev = cur;
        }
    } else if (m_points.size() == 0) {
        m_length = 0.0f;
    }
    return m_length;
}

/* the points on no cell dropped through MSL's remove and the range erase, the count dropped handed
 * back (new in SOCOM II, the name ours; SmoothPath calls it, never inlined). The end taken once into a
 * local and passed to both (the MSL headers' round 8: the callers' form gives the target's slots);
 * resize<point> (0x528430) stays byte-exact beside it now that include/vector declares resize ahead
 * of erase. Its remove_copy (0x528530) matches once point == CAiMapLoc reads the cell first (ai.h) */
#pragma auto_inline off
int CAiPath::RemoveInvalidPoints()
{
    unsigned int n = m_points.size();
    points::iterator e = m_points.end();
    points::iterator i = std::remove(m_points.begin(), e, CAiMapLoc::invalid);

    m_points.erase(i, e);
    return n - m_points.size();
}
#pragma auto_inline reset

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

/* the indices of a leg's first and last points: the points walked from the back, the legs' ends
 * met in turn -- a leg's start before its goal -- each next leg's ends read on its last one's goal
 * (SOCOM 1's GetLegBoundary, the same shape: GetLegPoints inlined, the reverse walk's iterators in
 * stack slots). Byte-exact since the AI core's round 27: the reverse iterator declared before the
 * loop and ASSIGNED rbegin() stays in memory for the whole walk -- rbegin()'s end temporary copied
 * through $f0 into the reverse iterator, a word copy into the returned temporary, $f0 again into `r`
 * (0x240 against 0x260 with `r` initialised in the for: MW scalarised it); include/iterator's
 * reverse_iterator is the target's as it stands. The point's index declared before the leg's count
 * (n t1, i t2) */
bool CAiPath::GetLegBoundary(int &first, int &last, int leg) const
{
    first = last = -1;
    if (leg == -1)
        leg = m_planleg;
    if (leg >= m_legcount)
        return false;
    if (m_legcount == 1) {
        first = m_points.size() - 1;
        last = 0;
        return true;
    }
    int i = m_points.size() - 1;
    unsigned int n = 0;
    CAiMapLoc start;
    CAiMapLoc goal;
    points::reverse_iterator r;

    GetLegPoints(start, goal, n);
    for (r = ((points &)m_points).rbegin(); r != ((points &)m_points).rend(); ++r, --i) {
        if (*r == start && n == leg)
            first = i;
        if (*r == goal) {
            if (n == leg) {
                last = i;
                return true;
            }
            n++;
            GetLegPoints(start, goal, n);
        }
    }
    return true;
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

/* the graph's pieces as the detour reads them (graph_tree.h's CGNode, CGEdge and CGRoute, as
 * ai_mapio.cpp's fixups lay out the first two: a region at a node's 8, a route at an edge's 0x14;
 * the route a node list, its constructor and destructor graph_main.cpp's) */
class CGNode {
public:
    unsigned char _0[0x8];
    CAiMap::region *m_region;   // 0x8
};

class CGEdge {
public:
    unsigned char _0[0x14];
    CAiMap::route *m_route;   // 0x14
};

class CGRoute {
public:
    CGRoute();
    ~CGRoute();

    std::list<CGNode *> m_nodes;   // 0x0
};

/* whether a step may end on the cell: CAiMap::IsTraversable's body (ai_maps.cpp, 0x52A090), as an
 * inline of this unit's (ai_map.cpp and ai_mapwalk.cpp have the same) -- too big to inline whole,
 * plan_addleg inlines it one level, GetCell and the cell's test (0x511F40) called out of line */
inline bool _traversable(const CAiMap *map, const CAiMapLoc &loc)
{
    CAiCell *cell = map->GetCell(loc, 0);

    return cell ? cell->IsTraversable() : false;
}

/* a leg added between two cells a step may end on: across regions of kind 4 that the map's graph
 * joins -- while the path allows it (flag 0x20, cleared for the legs it adds) -- one leg for each
 * step of each route the graph's route between the two regions takes, then the last to the goal;
 * otherwise the leg itself, its two cells and its map pushed (SOCOM 1's, the demo's 0x195B90).
 * Byte-exact since the AI core's round 32 (127 off in round 31, every call, branch and spill already
 * the target's): the cell tests through the unit's _traversable inlined one level (GetCell and the
 * cell's test out of line), the body under one `&&` so both failures reach the final `return false`;
 * the walk's two cells ONE 8-BYTE STRUCT (from, to) -- as two CAiMapLoc locals they took the frame's
 * top slots above the route, wherever declared (4-byte class locals go first), where the struct
 * follows declaration order: the route at 224, from 216, to 220 -- and the same struct gave `a` s1
 * with &to hoisted into s0 ahead of the outer loop; `from` assigned after the route's first node is
 * read (its store scheduled after that load), the next cell through a reference local kept in a
 * saved register across the recursive call (`int j = 0;` ahead of it, round 31's numbering); the
 * reverse step's count read into a local ahead of the compare (live across it, so a0: MW hoists the
 * load into the first branch's delay slot -- read in the arm it took v1); the flag restored through
 * SetFlags(0x20, 0) (its `&= ~0` the zero extension, dsll32 / dsrl32); `if (!fail) { ... return
 * true; } return false;` puts the inner `return false` after the pushes, as the target does */
bool CAiPath::plan_addleg(CAiMap *map, const CAiMapLoc &start, const CAiMapLoc &goal)
{
    if (_traversable(map, start) && _traversable(map, goal)) {
        bool done = false;
        bool fail = false;

        if (m_flags & 0x20) {
            m_flags &= ~0x20;
            CGraph *graph = map->m_graph;

            if (graph) {
                CAiMap::region *r0 = map->GetRegionContaining(start, (CAiMap::region::TYPE)4, false);
                CAiMap::region *r1 = map->GetRegionContaining(goal, (CAiMap::region::TYPE)4, false);

                if (r0 && r1 && r0 != r1) {
                    CGNode *n0 = graph->GetNodeByNodeEx(r0);
                    CGNode *n1 = graph->GetNodeByNodeEx(r1);
                    CGRoute route;

                    if (graph->BuildRoute(n0, n1, 0, &route) && route.m_nodes.size()) {
                        struct { CAiMapLoc from; CAiMapLoc to; } leg;

                        std::list<CGNode *>::iterator i = route.m_nodes.begin();
                        CGNode *a = *i;

                        leg.from = start;

                        for (++i; i != route.m_nodes.end(); ++i) {
                            CGNode *b = *i;
                            CGEdge *e = graph->FindEdge(a, b);

                            if (!e) {
                                fail = true;
                                break;
                            }
                            CAiMap::region *reg = a->m_region;

                            if (!reg) {
                                fail = true;
                                break;
                            }
                            CAiMap::route *rt = e->m_route;

                            if (!rt) {
                                fail = true;
                                break;
                            }
                            int n = rt->m_path.size();
                            int j = 0;
                            CAiMapLoc &next = leg.to;

                            for (; j < n; j++) {
                                int m = rt->m_path.size();

                                next = reg->m_loc == rt->m_region[0] ? rt->m_path[j] : rt->m_path[m - j - 1];
                                if (!plan_addleg(map, leg.from, next)) {
                                    fail = true;
                                    break;
                                }
                                leg.from = next;
                            }
                            if (fail)
                                break;
                            leg.from = next;
                            a = b;
                        }
                        if (!fail) {
                            plan_addleg(map, leg.to, goal);
                            done = true;
                        }
                    }
                }
            }
            SetFlags(0x20, 0);
        }
        if (done)
            return true;
        if (!fail) {
            m_waypts.push_back(start);
            m_waypts.push_back(goal);
            m_maps.push_back(map);
            m_legcount++;
            return true;
        }
        return false;
    }
    return false;
}

/* the id moved on, the path drawn when asked (SOCOM 1's IncrementPathId; called from ai_maps.cpp
 * and the path-recover state) */
void CAiPath::IncrementPathId()
{
    m_pathId++;
    if (m_flags & 0x40)
        PrintPath("dbg_path");
}

/* a leg's search over: true while legs remain to plan, the next leg queued; else the plan
 * completes in the state the search left (SOCOM 1's).
 * OPEN (0x7C against 0xAC): the target keeps two compares of m_planleg against m_legcount whose
 * bodies compiled to nothing -- the first a branch to the next instruction, the second with
 * `more = true` in its delay slot (SOCOM 1's called the maps' data() out of line in both). Every
 * spelling tried -- an if, a ternary statement, a dead local, GetLegMap(-1) -- is deleted whole;
 * the AI core round 19 added an inline member returning the planned leg's map called as a
 * statement (ternary, if-assign and early-return bodies, `(void)` cast) and empty-bodied ifs
 * (`{}`, `;`, `(void)0`, `m_maps[m_planleg];`, either sense): deleted whole too. MW keeps such a
 * branch only for a body the front end cannot see is empty (a compiled-out macro?). The AI core round
 * 21: dead locals assigned in the arms (if, if/else, ternary, through data()'s reference) are deleted
 * whole as well; an inline taking a CONST path, declared before this function and defined after it,
 * keeps both branches and the reloads of m_planleg after them -- but calls the pointer vector's const
 * operator[] out of line inside them (0xC8): the shape the target's is, with the access inlined away
 * after the branches were built (build/scratch/ai/v_plc.py). The AI core round 22 (v_plc*.py): with
 * the const operator[] instantiated by an own function first, that inline takes it and the whole is
 * deleted again (0x7C), as are raw `data_[i]` loads in it (const or not, by value or pointer) and
 * empty asm statements (`asm {}`, `asm volatile ("")`); `asm volatile { nop }` as each body gives the
 * target's size and branches, the nop the only difference -- the target's bodies are empty after a
 * front end that could not see them so. The AI core round 29 (build/scratch/ai/v_plc*.py): an asm
 * DIRECTIVE as each body is no empty statement to MW -- `asm { .set reorder }` keeps the first
 * compare (its branch dropped, a nop left) and the second branch aimed at the exit (0xA4),
 * `asm { .set noreorder }` gives the target's size (0xAC) but reorders the block after it (18 off);
 * a label alone in asm, `if (0) ...`, `while (0)`, `for (;0;)`, `do {} while (0)`, `switch (0) {}`,
 * `goto L; L:;`, self-assignments, data()/begin() calls and a dead reference local: deleted whole.
 * The AI core round 34 (build/scratch/ai/v_plc1-6.py): IRO'S DEAD-ASSIGNMENT PASS is what deletes
 * them -- under `#pragma opt_dead_assignments off` (a probe only) both compares stay, an if-then
 * dead load (`if (m_planleg < m_legcount) map = m_maps[m_planleg];`) giving 0xA4: the first
 * compare left as a bare sltu and nop (its branch to the next block dropped), the second chained
 * to the exit -- so the target's bodies died AFTER the branch optimiser, later than any dead load
 * does. A redundant store to a live variable survives IRO and dies in the PCode's CSE (`more =
 * true; if (c) more = true;`: the second compare's shape, `more = true` in its delay slot, but
 * the branch chained to the exit, 0x94; `opt_common_subs off` keeps it); in the first compare
 * `more` is dead (rewritten before any read), so its redundant store goes whole. Dead chains (`n
 * = m`), empty inlines taking the map, an inline returning `c ? true : true`, an empty asm
 * elsewhere in the function: deleted whole. The only other MW row in the image with an emptied
 * if-body is OVL1's func_003B1DD0 (0x3B1EB0, `srl; bltz +1` -- unmatched) */
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

/* a cell put at the head of a finished path (its points run last to first): an empty path made
 * of it and complete, a cell already at the head taken as done, else its point inserted first and
 * the last waypoint moved onto it -- or, on another map, a leg begun there; the id moved on after
 * (SOCOM 1's AppendPathLoc, 0x526F20: CAiMaps::ExtendPath's appends; SOCOM 1 compared the head's
 * map where SOCOM II compares the cell). Byte-exact in the AI core's round 28 (116 off when first
 * written: MW coloured `ok` s0 where the target has the cell s0, `this` s1, `ok` s2). The
 * planning test is the inline IsIdle(): its state load is the one HasPoints() reuses, two
 * temporaries fewer around `ok`, which then leaves MW's colouring sweep before `this` and the
 * cell (build/scratch/ai/ig/nbdeg.py: every spill cost 0, 19 hardware neighbours each); written
 * out, or as !IsPlanning() (materialised), the colouring turns. The head compared through
 * CAiMapLoc's == (the friend operator gives the loads' other registers), the point inserted a named
 * local (its slot above the empty path's temporary), and the last waypoint taken by address inside
 * the test, as the target's &back() is computed there and kept for the store */
bool CAiPath::AppendPathLoc(const CAiMapLoc &loc, CAiMap *map)
{
    bool ok = false;

    if (IsIdle()) {
        if (m_points.empty()) {
            m_points.push_back(point(loc));
            m_state = COMPLETE;
            ok = true;
        } else if (HasPoints()) {
            if ((*m_points.begin()).m_loc == loc) {
                ok = true;
            } else {
                point p(loc);

                m_points.insert(m_points.begin(), p);
                CAiMapLoc *last;

                if (!m_waypts.size() || (last = &m_waypts.back(), last->m_mapid != loc.m_mapid)) {
                    m_waypts.push_back(loc);
                    m_waypts.push_back(loc);
                    m_maps.push_back(map);
                    m_legcount++;
                } else {
                    *last = loc;
                }
                ok = true;
            }
        }
    }
    if (ok)
        IncrementPathId();
    return ok;
}

/* a cell put at the start of a finished path, the end of its points: an empty path made of it and
 * complete, a cell already last refused, else its point pushed; the id moved on after (SOCOM 1's
 * InsertPathLoc, 0x526C10, by its place before AppendPathLoc; FindDetour's last call, the map
 * unread) */
bool CAiPath::InsertPathLoc(const CAiMapLoc &loc, CAiMap *map)
{
    bool ok = false;

    if (IsIdle()) {
        if (m_points.empty()) {
            m_points.push_back(point(loc));
            m_state = COMPLETE;
            ok = true;
        } else if (HasPoints()) {
            if (m_points.back().m_loc != loc) {
                m_points.push_back(point(loc));
                ok = true;
            }
        }
    }
    if (ok)
        IncrementPathId();
    return ok;
}

/* a cell put into a path at a point index, counted from the end when asked: an empty path made of
 * it -- its one leg, the cell and the map -- and complete; a finished path drawn ("aipath"), the
 * index's leg found, the point inserted there and the path drawn again; false while planning, for no
 * finished plan or an index off the points (new in SOCOM II, the name ours: 0x526810, after
 * PostProcess; the AI states' 0x555420 and 0x5556A0 call it).
 * OPEN, 66 off at the target's size and frame (the AI core round 37; 0x3EC and d 108 before, d 75 now): the iterator a
 * named local declared before the loop (sp+132, above the loop's boundaries, 128 / 124: frame slots are creation
 * order, so it cannot be the argument's temporary, which comes after the loop's), the point a named local (112, below
 * the position's by-value copy, 120: as a temporary argument it moves to 104). What is left is one block's temporaries,
 * registers only: the target's PCode holds `index * 8` (v0) across the point's construction -- the shift first, x's temp
 * taking index's s0, the add after the point -- where `it += index` computes the two together. Offset locals (with the add
 * after the point, build/scratch/ai/v_ipl37e.py): 85 off and more. An inline helper for the leg search (round 37: v_ipl37.py,
 * by value, by reference, the counter in the caller) keeps its sixth saved register (0x3F8).
 * Earlier: the registers are the target's (the counter shares `ok`'s s3, the leg from_end's s4); `begin() += index` as the
 * argument keeps the stores (0x3F4) in sp+124; inline insert helpers taking the iterator by value scalarise it (0x3EC) or
 * copy it once more (0x3FC) */
bool CAiPath::InsertPathLoc(const CAiMapLoc &loc, CAiMap *map, int index, bool from_end)
{
    bool ok = false;

    if (IsIdle()) {
        if (m_points.empty()) {
            m_points.push_back(point(loc));
            m_waypts.push_back(loc);
            m_waypts.push_back(loc);
            m_maps.push_back(map);
            m_state = COMPLETE;
            ok = true;
            m_legcount = 1;
        } else if (HasPoints()) {
            PrintPath("aipath");
            if (from_end)
                index = m_points.size() - index;
            if (index >= 0 && index <= (int)m_points.size()) {
                int first, last;
                int leg = -1;
                points::iterator it;

                for (int i = 0; leg == -1 && i < m_legcount; i++) {
                    int f, l;

                    if (!GetLegBoundary(f, l, i))
                        break;
                    if (index >= l && index <= f)
                        leg = i;
                }
                GetLegBoundary(first, last, leg);
                it = m_points.begin();
                it += index;
                point p(loc);

                m_points.insert(it, p);
                PrintPath("aipath");
                ok = true;
            }
        }
    }
    return ok;
}

/* the finished path's points made, smoothed and widened, and drawn when asked (SOCOM 1's;
 * plan_complete calls it, never inlined) */
#pragma auto_inline off
void CAiPath::PostProcess()
{
    SmoothPath(false);
    UpdatePointFlags();
    if (m_flags & 4)
        UpdatePathWidth(5);
    if (m_flags & 0x40)
        PrintPath("dbg_path");
}
#pragma auto_inline reset

/* MSL's reverse_iterator over the points (ours: include/vector has none; the HUD's c2dfont.cpp
 * carries the same shape) -- the base iterator held, `current - 1` at each access */
class CPointRevIter {
public:
    typedef CAiPath::points::iterator It;

    explicit CPointRevIter(It x) : current(x) {}
    CAiPath::point &operator*() const
    {
        It tmp = current;

        return *--tmp;
    }
    CAiPath::point *operator->() const { return &(operator*()); }
    CPointRevIter &operator++()
    {
        --current;
        return *this;
    }
    CPointRevIter operator++(int)
    {
        CPointRevIter t(*this);

        ++*this;
        return t;
    }

    It current;
};

inline CPointRevIter rbegin(CAiPath::points &v)
{
    return CPointRevIter(v.end());
}

inline CPointRevIter rend(CAiPath::points &v)
{
    return CPointRevIter(v.begin());
}

inline bool operator!=(const CPointRevIter &x, const CPointRevIter &y)
{
    return x.current != y.current;
}

/* the legs rebuilt from a finished path's points, last to first (new in SOCOM II, the name ours:
 * 0x526540, CAiMaps::ExtendPath's last call): one leg to start, its map the last point's, and a new leg
 * -- the point before and the point after as its ends, the after point's map -- wherever the points
 * cross maps; the first point closes the last leg, and the path is smoothed when asked. The walk is
 * UpdatePathWidth's reverse iterator; each map pushed straight from the lookup (two call
 * temporaries) */
void CAiPath::RebuildLegs(CAiMaps *maps, bool smooth)
{
    if (!maps)
        return;
    if (!HasPoints())
        return;
    m_waypts.clear();
    m_maps.clear();
    m_legcount = 1;
    CPointRevIter i = rbegin(m_points);
    CAiPath::point *prev = &*i;

    m_waypts.push_back(prev->m_loc);
    m_maps.push_back(maps->GetMapByIndex((*i).m_loc.m_mapid));
    for (++i; i != rend(m_points); ++i) {
        if ((*i).m_loc.m_mapid != m_waypts.back().m_mapid) {
            m_waypts.push_back(prev->m_loc);
            m_waypts.push_back((*i).m_loc);
            m_maps.push_back(maps->GetMapByIndex((*i).m_loc.m_mapid));
            m_legcount++;
        }
        prev = &*i;
    }
    m_waypts.push_back(prev->m_loc);
    if (smooth)
        SmoothPath(true);
}

/* each leg smoothed, the invalid points dropped and the points' storage trimmed, when there are
 * points, the plan is complete and smoothing is forced or on (SOCOM 1's dropped the invalid points
 * itself and trimmed nothing) */
bool CAiPath::SmoothPath(bool force)
{
    bool done = false;

    if (HasPoints() && (force || (m_flags & 1))) {
        for (unsigned int i = 0; i < m_legcount; i++)
            SmoothPathLeg(i);
        RemoveInvalidPoints();
        TrimVector<std::vector<point> >(m_points);
        done = true;
    }
    return done;
}

/* whether a cell suits the path's kind (new in SOCOM II, the name ours): a stealthy path keeps to
 * soils 2, 4, 5 and 12 (water, brush, forest, grass -- one bit test), a basic one to roads, any
 * other takes every cell. An inline: the smoothing callback's switch inlines it whole, its ends'
 * test (inlined one level) calls it, so its copy 0x526310 follows the callback */
inline bool CAiPath::IsPreferredCell(const CAiCell *cell) const
{
    bool ok = true;

    if (m_type == PATH_STEALTH) {
        unsigned int soil = cell->m_soil;

        ok = soil == 2 || soil == 4 || soil == 5 || soil == 12;
    } else if (m_type == PATH_BASIC) {
        ok = cell->m_soil == 3;
    }
    return ok;
}

/* what the smoothing walk carries (the name ours): the path, and whether its two ends' cells suit
 * it (1 the start's, 2 the end's) */
struct SMOOTH_DATA {
    CAiPath *m_path;   // 0x0
    int m_state;   // 0x4
};

/* the open test's steps over the cell's soil, read once by the caller (the names ours: inlines
 * returning the tests' negations, which MW materialises -- sltiu or sltu, then xori -- before it
 * branches, as CAiCell's NoAction does; the soils' bit test is IsTraversable's) */
inline bool traversable_soil(unsigned int soil)
{
    return !(soil == 0 || soil == 10 || soil == 8 || soil == 9);
}

inline bool not_collision_soil(unsigned int soil)
{
    return !((unsigned char)soil == CAiMap::SOIL_COLLISION);
}

inline bool not_occupied(const CAiCell *cell)
{
    return !cell->IsOccupied();
}

/* a cell a smoothed line may cross: a traversable soil, not collision, not occupied -- each step a
 * bool of its own, as CAiCell::IsConcealing's (the name ours) */
inline bool smooth_open(const CAiCell *cell, unsigned int soil)
{
    bool r = false;
    bool o = false;

    if (traversable_soil(soil) && not_collision_soil(soil))
        o = true;
    if (o && not_occupied(cell))
        r = true;
    return r;
}

/* whether a line's end cell suits the path (the name ours) */
inline bool smooth_end(const CAiMap *map, const CAiPath *path, const CAiPath::point &p)
{
    CAiCell *cell = map->GetCell(p.m_loc, 0);
    bool r = false;

    if (cell)
        r = path->IsPreferredCell(cell);
    return r;
}

/* the smoothing walk's cells: each open, no climb up a steep cell, on the template path when the
 * path keeps to it; the first cell notes whether the line's two ends suit the path, the others
 * must suit it as the ends do -- both ends suiting, every cell must; neither, none may; the end's
 * alone, none before the end (SOCOM 1's _SmoothLineCallback, the template and the kinds new).
 * Byte-exact so (the AI core's round 30, igreplay --movers): `ok` declared uninitialised at the
 * top and the soil a named local read from the field -- both numbered among the callback's own
 * locals, so ok takes s0 and the soil t1 as the target's; through GetSoil() or IsTraversable's own
 * local the soil was numbered as a temporary (t0/a3/a2 rotated). The switch's cases in the order
 * the compare chain reverses (3, 2, 0, 1; the bodies in that order), the cell re-read for them */
static bool _SmoothLineCallback(const MAP_LINEWALK_STRUCT &s, void *data)
{
    bool ok;
    const CAiCell *cell = s.m_cell;

    if (!cell)
        return false;
    unsigned int soil = cell->m_soil;

    ok = smooth_open(cell, soil);
    if (ok && (unsigned char)soil == CAiMap::SOIL_STEEP) {
        const CAiCell *prev = s.m_prev;

        if (cell != prev && prev) {
            float a;
            float b;

            s.m_map->GetAltitude(prev, a);
            s.m_map->GetAltitude(cell, b);
            if (b > a)
                ok = false;
        }
    }
    if (ok) {
        SMOOTH_DATA *d = (SMOOTH_DATA *)data;
        CAiPath *path = d->m_path;

        if ((path->m_flags & 8) && path->m_template)
            ok = path->m_template->PointInPath(s.m_loc, 0);
        if (ok) {
            if (s.m_t == 0.0f) {
                if (smooth_end(s.m_map, path, s.m_start))
                    d->m_state |= 1;
                if (smooth_end(s.m_map, path, s.m_end))
                    d->m_state |= 2;
            } else {
                switch (d->m_state) {
                case 3:
                    ok = path->IsPreferredCell(s.m_cell);
                    break;
                case 2:
                    ok = s.m_loc == s.m_end || !path->IsPreferredCell(s.m_cell);
                    break;
                case 0:
                    ok = !path->IsPreferredCell(s.m_cell);
                    break;
                case 1:
                    ok = !path->IsPreferredCell(s.m_cell);
                    break;
                }
            }
        }
    }
    return ok;
}

/* whether a point's cell lets the leg be smoothed across it: no action of kind 1 or 4 on it (nor
 * 7 for the point dropped) and, without an action, no climb (the name ours) */
inline bool smooth_cell(const CAiMap *map, const CAiPath::point &p, bool mid)
{
    CAiCell *cell = map->GetCell(p.m_loc, 0);
    bool ok = true;

    if (cell) {
        if (cell->HasAction()) {
            switch (map->GetCellAction(p.m_loc)->m_type) {
            case 1:
            case 4:
                ok = false;
                break;
            case 7:
                ok = !mid;
                break;
            }
        } else if (cell->IsSoil(CAiMap::SOIL_CLIMB)) {
            ok = false;
        }
    }
    return ok;
}

/* a leg smoothed: from its first point, each point dropped (made invalid) when the line from the
 * last point kept to the one after it is clear and their cells allow it, else kept as the new
 * anchor; true when any was dropped (SOCOM 1's SmoothPathLeg, the demo's 0x194700) */
bool CAiPath::SmoothPathLeg(int leg)
{
    CAiMap *map = GetLegMap(leg);
    int first;
    int last;

    if (!map || !GetLegBoundary(first, last, leg))
        return false;
    int anchor = first;
    bool changed = false;

    if (anchor - last > 2) {
        int i = anchor - 1;

        while (i > last) {
            CAiMapLoc &start = m_points[anchor].m_loc;
            point &mid = m_points[i];
            point &next = m_points[i - 1];
            bool smooth = smooth_cell(map, start, false);

            if (smooth) {
                smooth = smooth_cell(map, mid, true);
                if (smooth) {
                    smooth = smooth_cell(map, next, false);
                    if (smooth) {
                        SMOOTH_DATA d;

                        d.m_path = this;
                        d.m_state = 0;
                        smooth = map->WalkLine(start, next.m_loc, _SmoothLineCallback, &d, 0.2f);
                    }
                }
            }

            if (smooth) {
                m_points[i].m_loc = CAiMapLoc::invalid;
                i--;
                changed = true;
            } else {
                anchor = i;
                i--;
            }
        }
    }
    return changed;
}

/* the points' flags, last to first (SOCOM 1's UpdatePointFlags by its place and body, the demo's
 * 0x1940A0 right after UpdatePathWidth -- its UpdateTurnAngles, next, is gone: SOCOM II's points carry
 * no angle; the same reverse walk over the legs as UpdatePathWidth's): at each leg's goal
 * the leg moves on, the goal and the point after it flagged 2 when the leg crosses maps; elsewhere the
 * point before a cell holding an action is flagged 1, one before cover (water, brush, forest, grass)
 * 4. The first point through the reverse iterator's postfix ++ (the old position copied, then
 * stepped), the size test `<= 2` (sltiu at), the soils an `||` chain MW folds into a bit test, held
 * in a bool (a switch gives a compare chain) */
void CAiPath::UpdatePointFlags()
{
    if (m_points.size() <= 2)
        return;
    CPointRevIter i = rbegin(m_points);
    CAiPath::point *prev = &*i++;
    CAiMap *map;
    int leg = 0;
    CAiMapLoc start;
    CAiMapLoc goal;

    map = GetLegMap(leg);
    GetLegPoints(start, goal, leg);
    for (; i != rend(m_points); ++i) {
        if (prev->m_loc == goal) {
            leg++;
            if (prev->m_loc.m_mapid != goal.m_mapid) {
                prev->m_flags |= 2;
                (*i).m_flags |= 2;
            }
            map = GetLegMap(leg);
            GetLegPoints(start, goal, leg);
        } else if (map) {
            CAiCell *cell = map->GetCell((*i).m_loc, 0);

            if (cell) {
                if (cell->HasAction())
                    prev->m_flags |= 1;
                unsigned int soil = cell->m_soil;
                bool cover = soil == 2 || soil == 4 || soil == 5 || soil == 12;

                if (cover)
                    prev->m_flags |= 4;
            }
        }
        prev = &*i;
    }
}

/* each point's width on its leg's map, last to first, the leg moving on at each leg's goal (SOCOM 1's,
 * the demo's 0x193E80, after SetSnakeLength in both builds; SOCOM II's walk a reverse iterator: the end
 * stored and copied into the iterator through $f0, the begin likewise at each test). The cell compared
 * through CAiMapLoc's == (the point's x loaded first), the map declared ahead of the leg and assigned
 * after it (the map s4, the leg s3, the leg's zero in the first test's delay slot), rbegin()/rend()
 * inlines of their own (the iterator then takes the slot above rend's temporary) */
void CAiPath::UpdatePathWidth(unsigned int width)
{
    CPointRevIter i = rbegin(m_points);
    CAiMap *map;
    int leg = 0;

    map = GetLegMap(leg);
    CAiMapLoc start;
    CAiMapLoc goal;

    GetLegPoints(start, goal, leg);
    for (; i != rend(m_points); ++i) {
        (*i).m_width = map->GetLocationWidth((*i).m_loc, width);
        if ((*i).m_loc == goal) {
            leg++;
            map = GetLegMap(leg);
            GetLegPoints(start, goal, leg);
        }
    }
}

/* the snake's length: none (when one was set) clears flag 2 and empties the path, Clear inlined;
 * a new one cuts the points to it, reserves them, sets flag 2 and completes the plan (SOCOM 1's) */
void CAiPath::SetSnakeLength(unsigned int len)
{
    if (len == 0 && m_snake_len != 0) {
        m_flags &= ~4;
        Clear();
    } else if (len != m_snake_len) {
        if (len < m_points.size())
            m_points.resize(len);
        m_points.reserve(len);
        m_snake_len = len;
        SetFlags(4, 0);
        m_state = COMPLETE;
    }
}

/* the snake's head moved to a cell (SOCOM 1's AddSnakePoint, the demo's 0x193930, between SetPathType
 * and SetSnakeLength as here: CZSealBody::UpdateAiMapLoc's call 0x5B7DE8). The first point reserves
 * the snake and is pushed; SOCOM II then looks for the cell among the first `search` points -- the
 * head itself only takes the flags, a later one cuts the points before it off -- and otherwise,
 * as SOCOM 1, keeps the longest step from the old head as the length, grows the snake by one up to
 * its length, shifts the points down and puts the cell first. The head's width is 0 under flag 8,
 * else the rings of open cells around it, up to 5; the path's id moves on. Byte-exact, the AI core's
 * round 20, by these shapes: the search's end the plain vector's (hoisted: points::end() re-reads
 * the count), its compare the cell's own ==, the moves through operator[] (data()'s reference: the
 * address of data_ is taken once, ahead of the first move, as the target's s3) and the shift's count
 * in a local (_min's call written in memmove's arguments loads data_ before it) */
void CAiPath::AddSnakePoint(const CAiMapLoc &loc, unsigned int search, unsigned int flags)
{
    if (!m_snake_len)
        return;
    unsigned int n = m_points.size();

    if (n == 0) {
        m_points.reserve(m_snake_len);
        m_points.push_back(point(loc));
        m_points[0].m_flags = flags;
    } else {
        int found = -1;

        if (search) {
            unsigned int i = 0;

            for (points::iterator it = m_points.begin(); it != m_points.std::vector<point>::end() && i <= search; ++it, ++i) {
                if ((*it).m_loc == loc) {
                    found = i;
                    break;
                }
            }
        }
        if (found == 0) {
            m_points[0].m_flags = flags;
        } else if (found > 0) {
            n -= found;
            memmove(&m_points[0], &m_points[found], n * sizeof(point));
            m_points.resize(n);
            m_points[0].m_flags = flags;
        } else {
            point &last = m_points.back();

            if (last.m_loc.m_mapid == loc.m_mapid) {
                int dx = last.m_loc.x > loc.x ? last.m_loc.x - loc.x : loc.x - last.m_loc.x;
                int dy = last.m_loc.y > loc.y ? last.m_loc.y - loc.y : loc.y - last.m_loc.y;
                float d = dx + dy;

                if (!(d <= m_length))
                    m_length = d;
            }
            if (n < m_snake_len)
                m_points.resize(n + 1);
            unsigned int k = m_snake_len - 1;
            unsigned int count = ai::_min(n, k);

            memmove(&m_points[1], &m_points[0], count * sizeof(point));
            m_points[0] = point(loc);
            m_points[0].m_flags = flags;
        }
    }
    if (flags & 8) {
        m_points[0].m_width = 0;
    } else {
        CAiMap *map = AiMaps.GetMapByIndex(loc.m_mapid);

        if (map)
            m_points[0].m_width = map->GetLocationWidth(loc, 5);
    }
    m_pathId++;
}

/* the last point taken off, its cell handed back when asked (new in SOCOM II, the name ours: the
 * SEALs call it). Emptiness tested through the inline bool, so back() reads the size again */
bool CAiPath::PopPoint(CAiMapLoc *loc)
{
    bool popped = false;

    if (!m_points.empty()) {
        point &p = m_points.back();

        if (loc)
            *loc = p.m_loc;
        popped = true;
        m_points.pop_back();
    }
    return popped;
}

/* the planner kind set unless a plan is running (SOCOM 1's SetPathType word for word, the state
 * now at 0x80), whether the path has that kind handed back -- the kind compared parameter first
 * (`type == m_type`: the xor's operands) */
bool CAiPath::SetPathType(PATH_TYPE type)
{
    if (m_state != PLANNING)
        m_type = type;
    return type == m_type;
}

/* a cell as text, the map, x and y, into one buffer (the name ours: PrintPath's, inlined -- the
 * buffer at OVL2 0x66A000, its only reader). The buffer handed on through a pointer assigned in
 * the call's argument: so the target keeps its address in a saved register for the caller's
 * sprintf (returned straight, MW rebuilt it there with lui/addiu, 4 bytes short) */
inline const char *LocText(const CAiMapLoc &loc)
{
    static char buf[32];
    char *p;

    sprintf(p = buf, "%2d:%4d,%4d", loc.m_mapid, loc.x, loc.y);
    return p;
}

/* the path drawn: each leg's bounds found, then each point from the last, its index, cell, width,
 * link and action bits, the legs' ends and the cell's name built into a line -- whose output, and
 * the name's, compiled out (SOCOM 1's PrintPath(), the text new in SOCOM II: its walk only, the
 * prints compiled out there too). The walk is GetLegBoundary's, its reverse iterator assigned; the
 * count read into an int local ahead of the text (unsigned, it and the buffer swapped registers),
 * the point's index declared after the first leg's ends */
void CAiPath::PrintPath(const char *name) const
{
    for (unsigned int leg = 0; leg < m_legcount; leg++) {
        int first = -1;
        int last = -1;

        GetLegBoundary(first, last, leg);
    }
    unsigned int n = 0;
    CAiMapLoc start;
    CAiMapLoc goal;
    points::reverse_iterator r;

    GetLegPoints(start, goal, n);
    int i = 0;

    for (r = ((points &)m_points).rbegin(); r != ((points &)m_points).rend(); ++r, i++) {
        char line[128];
        int sz = m_points.size();

        sprintf(line, "  pt%2d:  %s width=%d", sz - 1 - i, LocText((*r).m_loc), (*r).m_width);
        if ((*r).m_flags & 2)
            strcat(line, " LINK");
        if ((*r).m_flags & 1)
            strcat(line, " ACTION");
        if (*r == start)
            strcat(line, " LEG START");
        if (*r == goal) {
            strcat(line, " LEG_END");
            n++;
            GetLegPoints(start, goal, n);
        }
        CAiMap::name *nm = AiMaps.GetCellName((*r).m_loc);

        if (nm) {
            strcat(line, " (");
            strcat(line, nm->m_name);
            strcat(line, ")");
        }
    }
}

/* the point on the cell itself, its index handed back when asked (new in SOCOM II, the name ours:
 * 0x524880, which CAiSBreach::Tick calls -- the row carried PointInPath by its place until the AI
 * core's round 17 read SOCOM 1's PointInPath body in 0x524760, below). NOT const: the target
 * re-reads the points' data and count through `this` for each pass's end test (the first test
 * reusing the emptiness test's count and begin's data), which only a non-const member walking
 * m_points itself over the points' end() through count() gives (ai.h; the AI core's round 18,
 * byte-exact). As a const member through const_cast the end was hoisted (0xC8 against 0xD0);
 * postfix steps, a cast `this`, `!(it == end())`, the iterator declared outside and the
 * const_iterator walk did not move it */
bool CAiPath::FindPoint(const CAiMapLoc &loc, unsigned int *index)
{
    if (m_points.count() == 0)
        return false;
    bool found = false;
    unsigned int i = 0;

    for (points::iterator it = m_points.begin(); it != m_points.end(); ++it, ++i) {
        if ((*it).m_loc == loc) {
            if (index)
                *index = i;
            found = true;
            break;
        }
    }
    return found;
}

/* whether a snake path passes within a point's width of the cell, the point's index counted from
 * the path's end handed back when asked (SOCOM 1's PointInPath, the demo's 0x193540: the snake flag
 * 4, the widths, the index from the back -- SOCOM II adds the map's test; 0x524760, which the leg
 * smoother, ExtendPath and CAiSFollow::Tick call). Not const in SOCOM II: FindPoint's walk (above).
 * The point's map id read into a local first (the target compares it ahead of the cell's), the
 * cell's coordinates before the point's with `p > l ? p - l : l - p` (MW loads the right-hand
 * operand of a comparison first), and the width test `sum <= width` (branching on the compare;
 * `!(width < sum)` materialised it) -- the AI core's round 18, byte-exact */
bool CAiPath::PointInPath(const CAiMapLoc &loc, unsigned int *index)
{
    if (m_points.count() == 0 || !(m_flags & 4))
        return false;
    unsigned int n = 0;

    for (points::iterator it = m_points.begin(); it != m_points.end(); ++it, ++n) {
        unsigned int id = (*it).m_loc.m_mapid;

        if (id == loc.m_mapid) {
            unsigned int lx = loc.x;
            unsigned int px = (*it).m_loc.x;
            int dx = px > lx ? px - lx : lx - px;
            int ly = loc.y;
            int py = (*it).m_loc.y;
            int dy = py > ly ? py - ly : ly - py;

            if (dx + dy <= (*it).m_width) {
                if (index)
                    *index = m_points.count() - n - 1;
                return true;
            }
        }
    }
    return false;
}

/* a cell on a map (ai_astar.cpp's: through the reference MW shares the x the found cell's `!=` read,
 * which then stays a materialised bool) */
inline CAiMapLoc on_map(const CAiMapLoc &l, unsigned int id)
{
    return CAiMapLoc(l.x, l.y, id);
}

/* a point turned round into another (the name ours: FindDetour's second walk, the input read before
 * each output store) */
inline void negate(const CPnt3D *in, CPnt3D *out)
{
    out->x = -in->x;
    out->y = -in->y;
    out->z = -in->z;
}

/* what FindDetour's walks carry (the name ours): the two ends a detour joins, the cell found
 * (CAiMapLoc::invalid for none) and how far a walk goes */
struct DETOUR_DATA {
    CAiMapLoc m_from;   // 0x0
    CAiMapLoc m_to;   // 0x4
    CAiMapLoc m_found;   // 0x8
    float m_range;   // 0xc
};

/* a walk's cell taken as the detour when the feet reach it from the first end and the second end
 * from it; the walk goes on to its range (new in SOCOM II, the name ours: 0x524660) */
static bool _DetourLineCallback(const MAP_LINEWALK_STRUCT &s, void *data)
{
    DETOUR_DATA *d = (DETOUR_DATA *)data;

    if (s.m_loc == d->m_from)
        return true;
    if (s.m_map->CanTravelLine(d->m_from, s.m_loc, 0) && s.m_map->CanTravelLine(s.m_loc, d->m_to, 0)) {
        d->m_found = s.m_loc;
        return false;
    }
    if (!(s.m_dist <= d->m_range))
        return false;
    return true;
}

/* a cell to step aside to, put into the path: from the cell toward the path's last point (the one
 * before when the last is the goal), lines walked out from the point square to that way -- one
 * side, then the other -- for a cell both legs reach (new in SOCOM II, the name ours: 0x524170,
 * called by an AI state's 0x602270). The found cell rebuilt through on_map, so its `!=` stays the
 * target's materialised bool (the AI core's round 19 rule: the x its first operand read is used after).
 * The head's test is HasPoints() written out through a bool local -- `b = !empty(); if (b) b = m_state
 * == COMPLETE;` -- which narrows the negation (andi 0xff) and materialises the state's compare into the
 * same register (HasPoints() in any form gives MW's `daddiu 1` bool); the second walk's turn is a
 * negation into an out-parameter (negate(&side, &side)): the input's loads fold into sp offsets while
 * the output's stores take the field-address registers s7/s8, as the target's do (negating the fields
 * in place, a copy, a pointer or reference local, a by-value helper and the three-float constructor all
 * load through the registers too) */
bool CAiPath::FindDetour(const CAiMapLoc &from, const CPnt3D *pos, const CAiMapLoc &goal, int cells)
{
    bool ok = false;
    bool b = !m_points.empty();

    if (b)
        b = m_state == COMPLETE;
    point *p = b ? &m_points[m_points.size() - 1] : 0;

    if (!p)
        return false;
    CAiMapLoc to(p->m_loc);

    if (to == goal && !m_points.empty()) {
        PopPoint(0);
        p = &m_points[m_points.size() - 1];
        if (!p)
            return false;
        to = p->m_loc;
    }
    if (from.m_mapid != to.m_mapid)
        return false;
    CAiMap *map = AiMaps.GetMapByIndex(from.m_mapid);
    CPnt3D v;

    CAiMap::GetDirVec(CAiMap::GetClosestDir(from, to), v);
    DETOUR_DATA d;

    d.m_from = from;
    d.m_to = to;
    d.m_found = CAiMapLoc::invalid;
    d.m_range = 10.0f * cells;
    CPnt3D side;

    side.x = -v.z;
    side.y = 0.0f;
    side.z = v.x;
    CPnt3D end = *pos + side * d.m_range;

    map->WalkLine(from, &end, _DetourLineCallback, &d, 0.2f);
    if (d.m_found == CAiMapLoc::invalid) {
        negate(&side, &side);
        end = *pos + side * d.m_range;
        map->WalkLine(from, &end, _DetourLineCallback, &d, 0.2f);
    }
    if (d.m_found != CAiMapLoc::invalid) {
        CAiMapLoc l = on_map(d.m_found, map->m_mapid);

        ok = InsertPathLoc(l, map);
    }
    return ok;
}
