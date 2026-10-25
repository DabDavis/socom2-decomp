/* grid_main.cpp -- zdb::CGrid, the world's grid of cells: built from the world's file, its nodes
 * linked into the cells they cover through atoms, the walks over the cells a polygon covers (in
 * row order, or ring by ring from a view point), the clutter read and handed out to the cells; and
 * CCell, a grid square's node (OVL1 0x2D5060-0x2D7E9B). The unit is SOCOM 1's (its press beta's
 * asserts name grid_main.cpp), in SOCOM 1's order: ReadClutter first, the CCell constructor last.
 * grid_di.cpp's queries come before it (its static initialiser registers CStack's destructor, whose
 * linked copy, 0x2D4FE0, closes that unit), grid_clutter.cpp's clutter animation after it. Defined
 * in reverse of their image order (-inline deferred).
 * Unwritten: ReadClutter (0x2D5060), AddClutter (0x2D55C0), Destroy (0x2D58B0), 0x2D5B50 (new),
 * Create (0x2D5C80), SetTraversalBoundaryOrdered (0x2D6920), 0x2D6AB0 (new), SetTraversalExtents
 * (0x2D6C40), gridAddNodeToGrids (0x2D7580). CGrid() is written and byte-exact once its CFRect
 * constructor's row is named (below). */

#include "node_world.h"

/* the node's atom count set (node_main.cpp's SOCOM 1 CNode::SetAtomCnt(int), not named yet) */
extern "C" void func_003149D0(zdb::CNode *node, int n);
/* MSL's abs (MAIN, 0x191428, named): called by its address, since MW expands a call spelled abs
 * through <stdlib.h> inline (its builtin) where addOrderedCellAtom's target calls it twice */
extern "C" int func_00191428(int x);

/* the cell box SetCellBoundary set and GetNextCell walks (SOCOM 1's file statics) */
static int xMinCell;
static int zMinCell;
static int xMaxCell;
static int zMaxCell;
static int xCurCell;
static int zCurCell;

namespace zdb {

int CGrid::N_ATOMS = 8192;

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

/* the cells' list heads, the free atoms and the ordered walk's counts cleared (SOCOM 1's; its
 * atom buffer pointer is left alone in SOCOM II's). Byte-exact once 0x306650 is named
 * __ct__6CFRectFv (measured): until then its row is a placeholder outside this lane's ranges, the
 * constructor's call cannot resolve, and the generated draft (src/auto/ovl1/002D7970.c) stands */
#ifdef SC_CFRECT_NAMED
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
#endif

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

/* the ordered list built from the view point: its cell first, then ring after ring of the
 * neighbours of the last ring's cells, until a ring adds none or the list is full (SOCOM 1's).
 * OPEN (0x1F4 of 0x1E8): the target recomputes &m_Atoms[i] from the index at every step of the
 * inner loop (sll, addu) and tests the loop's bool twice (sltu, xori before the branch); MW
 * strength-reduces the element pointer here (a register stepped by 16) -- not an unsigned or short
 * index, pointer arithmetic, nor an accessor taking another index type */
void CGrid::buildOrderedCellAtomList(const CPnt3D *view)
{
    m_orderedCells.m_gridCellAtomCnt = 0;
    m_orderedCells.m_rt_gridCellAtomCnt = 0;
    int vx = (int)(m_InvCellDim * (0.0001f + (view->x - m_origin.x)));

    vx = MAX(0, vx);
    vx = MIN(m_CellCount.cx - 1, vx);
    m_orderedCells.m_viewX = vx;
    int vz = (int)(m_InvCellDim * (0.0001f + (view->z - m_origin.z)));

    vz = MAX(0, vz);
    vz = MIN(m_CellCount.cy - 1, vz);
    m_orderedCells.m_viewZ = vz;
    addOrderedCellAtom(vx, vz, true);
    int ring = 0;
    int i = 0;
    GRIDCELLATOM *a = &m_orderedCells.m_Atoms[0];
    bool more = true;
    int n;

    while (more && (n = m_orderedCells.m_gridCellAtomCnt) < 1600) {
        GRIDCELLATOM *last = &m_orderedCells.m_Atoms[n];

        while (a->ring == ring) {
            int x = a->x;
            int z = a->z;

            addOrderedCellAtom(x - 1, z, false);
            addOrderedCellAtom(x + 1, z, false);
            addOrderedCellAtom(x, z - 1, false);
            addOrderedCellAtom(x, z + 1, false);
            i++;
            a = &m_orderedCells.m_Atoms[i];
        }
        a = last;
        i = n;
        ring++;
        more = n != m_orderedCells.m_gridCellAtomCnt;
    }
}

/* a cell of the walk's box added to the ordered list, with its ring (its distance in cells from
 * the view cell), once per walk and only inside the boundary unless forced (SOCOM 1's) */
void CGrid::addOrderedCellAtom(int x, int z, bool force)
{
    if (x >= m_ix && x < m_ixb && z >= m_iz && z < m_izb) {
        CGridAtom *atom = *gridGetAtomBasePtr(x, z);

        if (atom && atom->Ent->m_TickNum < m_TickNum) {
            if (force || !gridCellOut(x, z)) {
                atom->Ent->m_TickNum = m_TickNum;
                GRIDCELLATOM *g = &m_orderedCells.m_Atoms[m_orderedCells.m_gridCellAtomCnt];

                g->cellAtom = atom;
                g->ring = func_00191428(x - m_orderedCells.m_viewX) + func_00191428(z - m_orderedCells.m_viewZ);
                g->x = x;
                g->z = z;
                m_orderedCells.m_gridCellAtomCnt++;
            }
        }
    }
}

/* the cell box a pair of corners spans, its first cell given back (SOCOM 1's; the corners by
 * value, the box's cells kept in the unit's statics for GetNextCell).
 * OPEN, 9 instructions (right size): registers in the inlined gridGetAtomBasePtr -- the target
 * keeps z in a1 and the clamped x in a2 (SOCOM 1's in a0/a1), here z takes a2 and x a1; not the
 * result local, its place, nor a helper taking the grid */
CCell *CGrid::SetCellBoundary(CPnt3D min, CPnt3D max)
{
    gridGetGridXandZIndex(&xMinCell, &zMinCell, min.x, min.z);
    gridGetGridXandZIndex(&xMaxCell, &zMaxCell, max.x, max.z);
    xCurCell = xMinCell;
    zCurCell = zMinCell;
    CCell *cell = 0;
    CGridAtom **p = gridGetAtomBasePtr(xCurCell, zCurCell);

    if (p && *p)
        cell = (*p)->GetCell();
    return cell;
}

/* the box's next cell, row by row, or none past its last (SOCOM 1's).
 * OPEN, 9 instructions (right size): SetCellBoundary's register pair, in both lookups */
CCell *CGrid::GetNextCell()
{
    xCurCell++;
    if (xCurCell <= xMaxCell) {
        CCell *cell = 0;
        CGridAtom **p = gridGetAtomBasePtr(xCurCell, zCurCell);

        if (p && *p)
            cell = (*p)->GetCell();
        return cell;
    }
    xCurCell = xMinCell;
    zCurCell++;
    if (zCurCell <= zMaxCell) {
        CCell *cell = 0;
        CGridAtom **p = gridGetAtomBasePtr(xCurCell, zCurCell);

        if (p && *p)
            cell = (*p)->GetCell();
        return cell;
    }
    return 0;
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

/* the walk's boundary: its points and extents set, each edge's inward normal in x/z made unit
 * length (none for a single point), and a new walk's tick (SOCOM 1's) */
int CGrid::SetTraversalBoundary(const CPnt3D *pnts, int n, bool b)
{
    SetTraversalExtents(pnts, n, b);
    if (n == 1)
        return ++m_TickNum;
    for (int i = 0; i < m_PntCnt; i++) {
        int j = (i + 1) % m_PntCnt;

        m_Edge[i].z = m_Pnts[i].x - m_Pnts[j].x;
        m_Edge[i].x = m_Pnts[j].z - m_Pnts[i].z;
        float len = sqrtf(m_Edge[i].x * m_Edge[i].x + m_Edge[i].z * m_Edge[i].z);

        if (len != 0.0f) {
            len = 1.0f / len;
            m_Edge[i].z *= len;
            m_Edge[i].x *= len;
        }
    }
    return ++m_TickNum;
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
 * and those this walk already gave (SOCOM 1's).
 * OPEN, 7 instructions (right size): the target takes the atom's node in an argument register (a0
 * here, a1 in GetNextAtomOrdered where a0 holds the grid) and &node->m_TickNum in v1; this puts the
 * node in v1 and the address in a0 -- SOCOM 1's own registers. Not the test's shape, the reference
 * or pointer, nor an inline taking the node */
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

/* the ordered walk's next atom: the cells' atoms in ring order (SOCOM 1's).
 * OPEN, 7 instructions (right size): GetNextAtom's registers (the node in a1 in the target) */
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
