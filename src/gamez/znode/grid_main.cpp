/* grid_main.cpp -- zdb::CGrid, the world's grid of cells: built from the world's file, its nodes
 * linked into the cells they cover through atoms, the walks over the cells a polygon covers (in
 * row order, or ring by ring from a view point), the clutter read and handed out to the cells; and
 * CCell, a grid square's node (OVL1 0x2D5060-0x2D7E9B). The unit is SOCOM 1's (its press beta's
 * asserts name grid_main.cpp), in SOCOM 1's order: ReadClutter first, the CCell constructor last.
 * grid_di.cpp's queries come before it (its static initialiser registers CStack's destructor, whose
 * linked copy, 0x2D4FE0, closes that unit), grid_clutter.cpp's clutter animation after it. Defined
 * in reverse of their image order (-inline deferred). */

#include "node_world.h"

/* the node's atom count set (node_main.cpp's SOCOM 1 CNode::SetAtomCnt(int), not named yet) */
extern "C" void func_003149D0(zdb::CNode *node, int n);

namespace zdb {

int CGrid::N_ATOMS = 8192;

/* the cell's lists cleared, its box the point and the size less a thousandth (SOCOM 1's) */
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

/* the lights and the clutter let go of (SOCOM 1's) */
CCell::~CCell()
{
    m_lightList.clear();
    m_clutterList.Free();
    m_clutterListDI.Free();
}

/* both lists' clutter let go of (SOCOM 1's an empty stub). Pinned: CGrid::DeleteClutter calls it
 * out of line */
#pragma auto_inline off
void CCell::DeleteClutter()
{
    m_clutterList.Free();
    m_clutterListDI.Free();
}
#pragma auto_inline reset

/* a clutter put in the list its node's collision says: its DI set up first when it has none
 * (SOCOM 1's an empty stub) */
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

/* the box's centre (SOCOM 1's) */
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

/* the cells' list heads, the free atoms and the ordered walk's counts cleared (SOCOM 1's; its
 * atom buffer pointer is left alone in SOCOM II's) */
CGrid::CGrid() : m_Atoms(0), m_FreeAtoms(0)
{
    m_CellCount.cx = 0;
    m_CellCount.cy = 0;
    m_posts = 0;
    m_world = 0;
    m_ring = 0;
    m_ClutterAnimManager = 0;
    m_ClutterCreated = 0;
}

/* the grid torn down and its clutter animation deleted (SOCOM 1's) */
CGrid::~CGrid()
{
    Destroy();
    if (m_ClutterAnimManager) {
        delete m_ClutterAnimManager;
        m_ClutterAnimManager = 0;
    }
}

CGridAtom **CGrid::gridGetAtomBasePtr(int x, int z)
{
    x = MAX(0, x);
    x = MIN(m_CellCount.cx - 1, x);
    z = MAX(0, z);
    z = MIN(m_CellCount.cy - 1, z);
    return &m_Atoms[x + z * m_CellCount.cx];
}

/* the node's atoms unlinked from their cells and put back on the free list, the node left with
 * none (SOCOM 1's; SOCOM II's free atoms a list) */
void CGrid::gridRemoveNodeFromGrids(CNode *node)
{
    for (int i = 0; i < node->m_AtomCnt; i++) {
        CGridAtom *atom = node->GetAtom(i);

        if (!atom->Next)
            atom->Prev->Next = 0;
        else {
            atom->Prev->Next = atom->Next;
            atom->Next->Prev = atom->Prev;
        }
        atom->Next = m_FreeAtoms;
        m_FreeAtoms = atom;
    }
    func_003149D0(node, 0);
}

/* whether the cell (x, z) lies outside the boundary: a single point never is, a segment only for a
 * line walk; else outside an edge by more than the cell's tolerance -- or, for a line walk, more
 * than the tolerance from it (SOCOM 1's) */
bool CGrid::gridCellOut(int x, int z)
{
    m_CurCellOut = 0;
    if (m_PntCnt == 1)
        return false;
    if (m_PntCnt == 2 && !m_LineWalk)
        return false;
    float half = 0.5f * m_CellDim;
    float cx = half + (m_origin.x + x * m_CellDim);
    float cz = half + (m_origin.z + z * m_CellDim);

    m_CurCellOut = 1;
    for (int i = 0; i < m_PntCnt; i++) {
        float dx = cx - m_Pnts[i].x;
        float dz = cz - m_Pnts[i].z;
        float d = dx * m_Edge[i].x + dz * m_Edge[i].z;

        if (d < -m_GridCellTol)
            return true;
        if (m_LineWalk && !(d <= m_GridCellTol))
            return true;
    }
    m_CurCellOut = 0;
    if (m_PntCnt > 2)
        m_RowCellIn = 1;
    return false;
}

/* the walk's first atom, the walk set to its first row (SOCOM 1's) */
CGridAtom *CGrid::StartTraversal()
{
    m_xx = m_ix;
    m_zz = m_iz;
    m_iz_start = m_zz;
    m_iz_next_row = m_izb;
    m_NextAtom = 0;
    m_zz--;
    m_RowCellIn = 0;
    m_CurCellOut = 1;
    m_iz_dir = 1;
    m_NextAtom = GetNextAtom();
    return m_NextAtom;
}

CGridAtom *CGrid::StartTraversalOrdered()
{
    m_NextAtom = 0;
    m_NextAtom = GetNextAtomOrdered();
    return m_NextAtom;
}

/* the walk's next cell: up a column and back down it for a polygon, row by row over the box for
 * a point or a segment (SOCOM 1's) */
bool CGrid::gridGetNextIndex()
{
    if (m_PntCnt > 2) {
        if (!m_CurCellOut && m_zz < m_iz_next_row)
            m_iz_next_row = m_zz;
        if (m_iz_dir < 0) {
            if (!m_CurCellOut && m_zz > m_iz) {
                m_zz--;
                return true;
            }
            m_zz = m_iz_start;
            m_iz_dir = 1;
            m_CurCellOut = 0;
        }
        if ((!m_RowCellIn || !m_CurCellOut) && m_zz < m_izb - 1) {
            m_zz++;
            return true;
        }
        if (m_xx < m_ixb - 1) {
            m_xx++;
            if (m_iz_next_row == m_izb) {
                m_zz = m_iz;
                m_iz_dir = 1;
            } else {
                m_zz = m_iz_next_row;
                if (m_zz > m_iz)
                    m_zz--;
                m_iz_dir = -1;
            }
            m_iz_next_row = m_izb;
            m_iz_start = m_zz;
            m_RowCellIn = 0;
            return true;
        }
        m_RowCellIn = 0;
        m_CurCellOut = 1;
    } else {
        if (m_zz < m_izb - 1) {
            m_zz++;
            return true;
        }
        if (m_xx < m_ixb - 1) {
            m_xx++;
            m_zz = m_iz;
            m_RowCellIn = 0;
            return true;
        }
    }
    return false;
}

/* a point's cell: its offset from the origin in cells, truncated on VU0 and held inside the grid
 * (SOCOM 1's) */
void CGrid::gridGetGridXandZIndex(int *ix, int *iz, float x, float z)
{
    sceVu0FVECTOR v;
    float *p = v;

    p[0] = m_InvCellDim * (x - m_origin.x);
    p[2] = m_InvCellDim * (z - m_origin.z);
    asm volatile {
        lqc2 vf3, 0(p)
        vftoi0.xyzw vf3, vf3
        sqc2 vf3, 0(p)
    }
    if (((int *)p)[0] < 0)
        *ix = 0;
    else
        *ix = ((int *)p)[0];
    if (*ix >= m_CellCount.cx)
        *ix = m_CellCount.cx - 1;
    if (((int *)p)[2] < 0)
        *iz = 0;
    else
        *iz = ((int *)p)[2];
    if (*iz >= m_CellCount.cy)
        *iz = m_CellCount.cy - 1;
}

/* the walk's next atom: the next in the cell, else the next cell's first, skipping inactive nodes
 * and those this walk already gave (SOCOM 1's) */
CGridAtom *CGrid::GetNextAtom()
{
    for (;;) {
        if (!m_NextAtom) {
            if (!gridGetNextIndex())
                return 0;
            if (gridCellOut(m_xx, m_zz)) {
                m_NextAtom = 0;
            } else
                m_NextAtom = *gridGetAtomBasePtr(m_xx, m_zz);
        } else
            m_NextAtom = m_NextAtom->Next;
        if (m_NextAtom) {
            CNode *node = m_NextAtom->Ent;

            if (node->m_active) {
                int &tick = node->m_TickNum;

                if (tick < m_TickNum) {
                    tick = m_TickNum;
                    return m_NextAtom;
                }
            }
        }
    }
}

/* the ordered walk's next atom: the cells' atoms in ring order (SOCOM 1's) */
CGridAtom *CGrid::GetNextAtomOrdered()
{
    for (;;) {
        if (!m_NextAtom) {
            if (m_orderedCells.m_rt_gridCellAtomCnt == m_orderedCells.m_gridCellAtomCnt)
                return 0;
            m_ring = m_orderedCells.m_Atoms[m_orderedCells.m_rt_gridCellAtomCnt].ring;
            m_NextAtom = m_orderedCells.m_Atoms[m_orderedCells.m_rt_gridCellAtomCnt++].cellAtom;
        } else
            m_NextAtom = m_NextAtom->Next;
        if (m_NextAtom) {
            CNode *node = m_NextAtom->Ent;

            if (node->m_active) {
                int &tick = node->m_TickNum;

                if (tick < m_TickNum) {
                    tick = m_TickNum;
                    return m_NextAtom;
                }
            }
        }
    }
}

void CGrid::Insert(CNode *node)
{
    gridAddNodeToGrids(node);
}

/* a node taken out of the grid and its atoms freed (SOCOM 1's) */
void CGrid::Remove(CNode *node)
{
    gridRemoveNodeFromGrids(node);
    node->FreeAtom();
}

/* a node (never a cell) re-linked into the cells it covers now (SOCOM 1's) */
void CGrid::Update(CNode *node)
{
    if (node && !node->IsCell()) {
        gridRemoveNodeFromGrids(node);
        gridAddNodeToGrids(node);
    }
}

/* every cell's clutter let go of (SOCOM 1's) */
void CGrid::DeleteClutter()
{
    int n = m_CellCount.cx * m_CellCount.cy;

    for (int i = 0; i < n; i++)
        m_Atoms[i]->GetCell()->DeleteClutter();
    m_ClutterCreated = 0;
}

/* the atom's node as a cell, or none (SOCOM 1's) */
CCell *CGridAtom::GetCell()
{
    CNode *node = Ent;

    if (node->GetType() == 10)
        return (CCell *)node;
    return 0;
}

/* the grid's parameters read from the world's file, defaults where it has none, and the grid
 * built (SOCOM 1's; its defaults a static's copy, SOCOM II's set field by field) */
bool CGrid::Read(CSaveLoad &io)
{
    tag_GRID_PARAMS params = {0};

    params.m_AtomCnt = N_ATOMS;
    params.m_CellDim = 640.0f;
    params.m_CellCount.cy = 8;
    params.m_CellCount.cx = 8;
    params.m_posts = 16;
    bool ok = io.m_zfile.Fetch("grid_params", &params, sizeof params);

    if (!ok)
        ok = true;
    if (ok)
        ok = Create(io.m_world, &params);
    return ok;
}

}
