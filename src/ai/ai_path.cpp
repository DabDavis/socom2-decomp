/* ai_path.cpp (OVL2, around 0x524000-0x52852F; SOCOM 1's unit, 0x193540-0x196E60 in the demo): the
 * planned path, CAiPath -- its points, legs and plan states. SOCOM 1's functions in the demo's
 * order (PointInPath ... reset__12CAiPathStats, then the template tail with _min<Ui>); SOCOM II's
 * fields sit where ai.h says. Only the plan's setup so far; defined here in reverse of the image's
 * order (-inline deferred). */

#include "ai.h"

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
