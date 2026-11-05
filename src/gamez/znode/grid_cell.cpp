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
 * Byte-exact on the headers as they stand (the core lane round 17 found its old OPEN note stale:
 * the target inlines CDynTexList's construction whole, six zero stores at 0x104-0x118, which once
 * took a level include/vector's pointer vector did not have) */
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
 * Byte-exact on the headers as they stand (the core lane round 17 found its old OPEN note stale:
 * the target inlines the light list's destructor four levels -- four this-tests, then the range
 * erase -- and CDynTexList's one level, the two class vectors' destructors 0x1FC5A0 and 0x1FC520
 * called with -1) */
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
 * Byte-exact since zmath.h's operators became inline members (the core lane round 17 found its
 * old OPEN note stale): all three inlined whole, the inner ones' results stacked below the outer
 * (Sub's at 48, Scale's 64, Add's 80, their copies 96-128) -- the one expression, as written */
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
