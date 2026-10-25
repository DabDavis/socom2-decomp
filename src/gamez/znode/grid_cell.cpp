/* grid_cell.cpp -- zdb::CCell, a grid square's node: its box, its lights and its two clutter lists
 * (OVL1 0x2D79D0-0x2D7E9B, in SOCOM 1's order: Copy first, the constructor last). Its own unit,
 * between grid_main.cpp and grid_clutter.cpp: CGrid::AddClutter and DeleteClutter call CCell's
 * AddClutter and DeleteClutter out of line, and CGrid::Create its constructor -- in SOCOM 1 too, where
 * CCell's AddClutter and DeleteClutter were EMPTY stubs (0x2170E0, 0x2170F0) that CGrid's called out
 * of line, which -inline auto does only across units (README "The inlining mode, measured"; one unit
 * needed both pinned). No template tail (SOCOM 1's instances after ~CCell and CCell() are dropped
 * here as earlier units' copies) and no initialiser. SOCOM 1 names no file for CCell; the unit's
 * name is ours. Defined in reverse of their image order (-inline deferred). */

#include "node_world.h"

namespace zdb {

/* the cell's lists cleared, its box the point and the size less a thousandth (SOCOM 1's).
 * OPEN (0x130 of 0x12C): the target inlines CDynTexList's construction whole (six zero stores at
 * 0x104-0x118), where include/vector's pointer vector puts CTexList's and CTexPalList's
 * constructors a level too deep and calls them -- CHUD's unresolved case (hud_main.cpp): the same
 * class's destructor wants the extra level (0x1FC520, 0x1FC5A0 are called with four this-tests) */
CCell::CCell(const CPnt3D *pos, float size)
{
    size -= 0.001f;
    *(PNT3D *)&m_bbox.m_max = *pos;
    m_bbox.m_min = m_bbox.m_max;
    CPnt3D d(size, 0.0f, size);

    m_bbox.m_max.Add(&d, &m_bbox.m_max);
    m_type = 10;
    m_lightList.clear();
    m_clutterList.clear();
    m_clutterListDI.clear();
}

/* the box is the cell's own, or its parent's when it borrows it (SOCOM 1's) */
CBBox *CCell::UpdateBBox(bool up)
{
    return (CBBox *)GetBBox();
}

/* the lights and the clutter let go of (SOCOM 1's).
 * OPEN (0x148 of 0x180): the target inlines the light list's destructor four levels (four
 * this-tests, then the range erase) and CDynTexList's one level (the two class vectors'
 * destructors called with -1, 0x1FC5A0 and 0x1FC520 -- placeholders, named here as the classes'
 * destructors they are once renamed); here ~CLightList is one call and the texture lists' calls
 * resolve to unnamed rows. Turns on the list destructors' inlining in tail-less code and on the
 * two rows' names */
CCell::~CCell()
{
    m_lightList.clear();
    m_clutterList.Free();
    m_clutterListDI.Free();
}

/* both lists' clutter let go of (SOCOM 1's an empty stub; grid_main.cpp's CGrid::DeleteClutter
 * calls it) */
void CCell::DeleteClutter()
{
    m_clutterList.Free();
    m_clutterListDI.Free();
}

/* a clutter put in the list its node's collision says: its DI set up first when it has none
 * (SOCOM 1's an empty stub; grid_main.cpp's CGrid::AddClutter calls it) */
bool CCell::AddClutter(CClutter *clutter)
{
    bool ok;

    if (!clutter->m_node->m_hasDI)
        clutter->m_node->SetDescendantDI(true);
    if (clutter->m_node->m_hasDI)
        ok = m_clutterListDI.Add(clutter);
    else
        ok = m_clutterList.Add(clutter);
    return ok;
}

void CCell::SetClutterFlag(bool on)
{
    m_clutterList.m_socom2_19 = on;
    m_clutterListDI.m_socom2_19 = on;
}

/* both lists drawn (SOCOM 1's an empty stub) */
void CCell::RenderClutter(CCell *cell)
{
    m_clutterList.Render(cell);
    m_clutterListDI.Render(cell);
}

/* the box's centre (SOCOM 1's).
 * OPEN, 16 instructions (right size): the target inlines all three operators whole, the inner
 * ones' results stacked below the outer (Sub's at 48, Scale's 64, Add's 80, their copies 96-128);
 * with zmath.h's free operators the nested argument sits too deep and operator- is inlined one
 * level (CPnt3D's default and copy constructors called out of line). Split into named statements
 * every operator inlines and only the stack slots differ (9 instructions: the first statement's
 * objects at the top of the frame) -- the original's operators were likely members (a temporary's
 * member call is no nested argument); zmath.h is not this lane's */
void CCell::GetMidPoint(CPnt3D *out)
{
    *out = m_bbox.m_min + (m_bbox.m_max - m_bbox.m_min) * 0.5f;
}

bool CCell::IsCell()
{
    return true;
}

/* a cell is never copied (SOCOM 1's) */
CNode *CCell::Copy(CNode *to) const
{
    return 0;
}

}
