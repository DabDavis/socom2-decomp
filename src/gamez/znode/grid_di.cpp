/* grid_di.cpp -- the grid's collision queries: a ray's or a point's hits gathered cell by cell
 * (CGrid's GetIntersect, GetAltitude, GetIntersectBatch and the node walks under them), and the
 * hit set's answers (DiIntersect: the highest hit under a point, the nearest along the ray, the
 * hits sorted near to far) (OVL1 ~0x2D2890-0x2D4FDB). The unit is SOCOM 1's: its static
 * initialiser, __sinit_grid_di.cpp, registers CStack's destructor for its matrix stack, whose
 * linked copy (0x2D4FE0) closes the unit; grid_main.cpp follows. Defined in reverse of their
 * image order (-inline deferred).
 * Written so far: the hit set's answers. Before the unit, 0x2D2070-0x2D2830 are not grid_di.cpp's
 * (checked, round 6): a recursive search over the graph's edge lists through the pointer list's
 * out-of-line begin() (0x2A00B0) and tiny iterator helpers -- graph_tree.cpp's neighbourhood.
 * The node walk NodeGetIntersect 0x2D2890 (SOCOM 1's NodeGetIntersect and NodeGetAltitude in one)
 * since the engine core's round 25; the batch walk 0x2D3030 (SOCOM 1's NodeGetIntersectBatch) and
 * CGrid::GetIntersectBatch 0x2D3CF0 since round 26 (both open). The unit is now whole. */

#include "node_world.h"
#include "int_material.h"

/* MSL's qsort (MAIN 0x196D38: SOCOM 1's SortNearToFar calls qsort) */
extern "C" void qsort(void *base, unsigned int n, unsigned int size,
                      int (*cmp)(const void *, const void *));
/* whether a matrix is the unit one (zmath_matrix.cpp's, new in SOCOM II, not named yet) */
extern "C" bool func_003084F0(const CMatrix *m);
/* whether a segment surely misses a node's bounds (node_main.cpp's, new in SOCOM II, not named
 * yet) */
extern "C" bool func_003124F0(zdb::CNode *node, const CPnt3D *a, const CPnt3D *b);
/* whether a set's region mask takes a region bit, none counting as the first (int_main.cpp's
 * neighbourhood, a generated draft: DiIntersect's, new in SOCOM II, not named yet) */
extern "C" int func_002DC620(zdb::DiIntersect *d, unsigned int bit);

/* the node the walk is in (SOCOM 1's file static m_node) and the matrix stack the props' matrices
 * are pushed on (SOCOM 1's file static stack; SOCOM II's initialiser, 0x401CC0, clears its owner
 * byte and pushes nothing -- CStack(bool), not CStack(), which pushes the unit matrix; SOCOM 1's
 * initialiser has the same `sb zero` and registration) */
static zdb::CNode *m_node;
static CStack stack(false);

/* whether the camera wants the collision flag (SOCOM 1's grid_di.cpp keeps it in an unnamed
 * namespace: cameraCollideFlagRequired__21@unnamed@grid_di_cpp@) */
namespace {
bool cameraCollideFlagRequired;
}

/* the hit set and the distances the sort compares, held for qsort's comparator (SOCOM 1's file
 * statics, by its symbols) */
static zdb::DiIntersect *_cmpdi_di;
static float *_cmpdi_rs;

/* the batch walk's cell marks, a bit per set for each cell of the box (SOCOM 1's global CellCheck,
 * a flag per byte there), and the marks of the cell being searched (SOCOM 1's global NodeCheck, a
 * byte there; each stored then read back, as SOCOM 1's) */
unsigned int CellCheck[1600];
unsigned int NodeCheck;
/* the batch walk switched off (SOCOM II's: nothing sets it; SOCOM 1 tested CGrid::m_doIntersect,
 * set, the other way round; the name ours) */
static bool noBatchIntersect;

/* two hits' indices by their squared distances, nearer first (SOCOM 1's) */
static int _cmpdi(const void *a, const void *b)
{
    float da = _cmpdi_rs[*(const int *)a];
    float db = _cmpdi_rs[*(const int *)b];

    if (da < db)
        return -1;
    if (da == db)
        return 0;
    return 1;
}

namespace zdb {

/* SOCOM 1's */
void SetCameraCollideFlagRequired(bool on)
{
    cameraCollideFlagRequired = on;
}

/* a hit set's ray tail (an inline accessor, the name ours: read through it inside a loop, the
 * tail's address is a loop invariant MW hoists -- into a preheader laid after the function's last
 * block, the loop entered with a `b` to it and the entry test repeated there) */
static inline const CPnt3D &RayTail(const DiIntersect *di)
{
    return di->m_Tail;
}

/* the hits' indices sorted by their squared distance from the ray's tail, the distances kept
 * (SOCOM 1's). The tail is read through RayTail() each pass (the target's hoisted address,
 * spilled to 160(sp), in a preheader after the sort's block: a reference local declared ahead of
 * the loop computes it before the entry test instead, the preheader inline); the count test an
 * early return (an `if` around the body sends its false edge to the body's closing `b`, not the
 * epilogue) */
void DiIntersect::SortNearToFar(int *idx, float *dist)
{
    if (m_Cnt <= 0)
        return;
    for (int i = 0; i < m_Cnt; i++) {
        idx[i] = i;
        CPnt3D d = m_Intersects[i].m_pos - RayTail(this);

        dist[i] = d.x * d.x + d.y * d.y + d.z * d.z;
    }
    if (m_Cnt > 1) {
        _cmpdi_rs = dist;
        _cmpdi_di = this;
        qsort(idx, m_Cnt, 4, _cmpdi);
        _cmpdi_di = 0;
        _cmpdi_rs = 0;
    }
}

/* the hit nearest the ray's tail, or none (SOCOM 1's) */
IntersectStruct *DiIntersect::GetNearestIntersect()
{
    if (m_Cnt <= 0)
        return 0;
    int best = 0;
    CPnt3D d = m_Intersects[0].m_pos - m_Tail;
    float bestDist = d.x * d.x + d.y * d.y + d.z * d.z;

    for (int i = 1; i < m_Cnt; i++) {
        CPnt3D d2 = m_Intersects[i].m_pos - m_Tail;
        float dist = d2.x * d2.x + d2.y * d2.y + d2.z * d2.z;

        if (dist <= bestDist) {
            bestDist = dist;
            best = i;
        }
    }
    return &m_Intersects[best];
}

/* every cell of the grid walked: its props with collision (SOCOM II's second clutter list) and
 * the node itself given their descendants' collision (SOCOM 1's) */
void CGrid::InitDescendantsDI()
{
    CPnt3D box[2];

    box[0].x = m_extents.left;
    box[1].x = m_extents.right;
    box[0].z = m_extents.top;
    box[1].z = m_extents.bottom;
    SetTraversalBoundary(box, 2, false);
    for (CGridAtom *atom = StartTraversal(); atom; atom = GetNextAtom()) {
        CCell *cell = atom->GetCell();

        if (cell) {
            CClutterList &list = cell->m_clutterListDI;

            if (list.size()) {
                for (CClutterList::iterator it = list.begin(); it != list.end(); ++it)
                    (*it)->m_node->SetDescendantDI(true);
            }
        }
        atom->Ent->SetDescendantDI(true);
    }
}

/* the same walk for the visuals (SOCOM 1's) */
void CGrid::InitDescendantsVisuals()
{
    CPnt3D box[2];

    box[0].x = m_extents.left;
    box[1].x = m_extents.right;
    box[0].z = m_extents.top;
    box[1].z = m_extents.bottom;
    SetTraversalBoundary(box, 2, false);
    for (CGridAtom *atom = StartTraversal(); atom; atom = GetNextAtom()) {
        CCell *cell = atom->GetCell();

        if (cell) {
            CClutterList &list = cell->m_clutterListDI;

            if (list.size()) {
                for (CClutterList::iterator it = list.begin(); it != list.end(); ++it)
                    (*it)->m_node->SetDescendantVisuals(true);
            }
        }
        atom->Ent->SetDescendantVisuals(true);
    }
}

/* a point set from three values, by value: z and x loaded before the stores (the name is ours) */
inline void DiPntSet(CPnt3D &p, float x, float y, float z)
{
    p.x = x;
    p.y = y;
    p.z = z;
}

/* a point's altitude hits: the ray made vertical through it, 50000 up and down, back faces
 * masked, then the cell walk of GetIntersect (SOCOM 1's; its ray was 500000 long) */
bool CGrid::GetAltitude(DiIntersect *di)
{
    di->m_Cnt = 0;
    DiPntSet(di->m_Tail, di->m_Tail.x, 50000.0f, di->m_Tail.z);
    DiPntSet(di->m_Tip, di->m_Tail.x, -50000.0f, di->m_Tail.z);
    CDI::m_backface_mask = 32;
    stack.Push(&CMatrix::identity);
    SetTraversalBoundary(&di->m_Tail, 1, false);
    for (CGridAtom *atom = StartTraversal(); atom; atom = GetNextAtom()) {
        CCell *cell = atom->GetCell();

        if (cell) {
            CClutterList &list = cell->m_clutterListDI;

            if (list.size()) {
                for (CClutterList::iterator it = list.begin(); it != list.end(); ++it) {
                    m_node = (*it)->m_node;
                    if (func_003084F0((*it)->GetMatrix()) == false) {
                        stack.Multiply((*it)->GetMatrix(), true);
                        NodeGetIntersect(m_node, di);
                        stack.Pop();
                    } else
                        NodeGetIntersect(m_node, di);
                }
            }
        }
        m_node = atom->Ent;
        NodeGetIntersect(atom->Ent, di);
    }
    stack.Pop();
    return true;
}

/* a ray's hits gathered from the cells it crosses: each prop with collision searched under its
 * matrix (pushed unless the unit one), then the cell's node (SOCOM 1's; SOCOM II skips the push
 * for a unit matrix) */
bool CGrid::GetIntersect(DiIntersect *di)
{
    CPnt3D ray[2];

    di->m_Cnt = 0;
    ray[0] = di->m_Tail;
    ray[1] = di->m_Tip;
    SetTraversalBoundary(ray, 2, true);
    CGridAtom *atom = StartTraversal();

    stack.Push(&CMatrix::identity);
    for (; atom; atom = GetNextAtom()) {
        CCell *cell = atom->GetCell();

        if (cell) {
            CClutterList &list = cell->m_clutterListDI;

            if (list.size()) {
                for (CClutterList::iterator it = list.begin(); it != list.end(); ++it) {
                    m_node = (*it)->m_node;
                    if (func_003084F0((*it)->GetMatrix()) == false) {
                        stack.Multiply((*it)->GetMatrix(), true);
                        NodeGetIntersect(m_node, di);
                        stack.Pop();
                    } else
                        NodeGetIntersect(m_node, di);
                }
            }
        }
        m_node = atom->Ent;
        NodeGetIntersect(atom->Ent, di);
    }
    stack.Pop();
    return true;
}

/* the smaller and the larger of two (macros: each operand read twice, the reads shared -- the
 * accumulated bound first, `<` and `>`; the names ours) */
#define BATCH_MIN(a, b) ((a) < (b) ? (a) : (b))
#define BATCH_MAX(a, b) ((a) > (b) ? (a) : (b))

/* a batch of intersect sets run in one walk of the grid (SOCOM 1's): the box round every set's ray
 * (2), sphere (4: the tip's x its radius) or point (1) taken in x and z, each set's hits cleared;
 * every cell inside marked clear, then each set's walk marks the cells it enters with the set's bit
 * (SOCOM 1 a flag per byte); then the marked cells' nodes, each once per walk (the tick), searched
 * for the whole batch -- the sets' tree marks cleared, the cell's props with collision under their
 * matrices (pushed unless the unit one, as GetIntersect), then the node itself.
 * OPEN, 25 instructions (unwritten before the engine core's round 26; the target's size, 0xA90, its
 * frame, every GPR and call): the four `ext[0].x` bounds (case 2's two, 4's and 1's first) swap
 * $f0 and $f1 -- the bound, read at the array's offset 0, is a plain object to pass 0's
 * IRO_CommonSubs, so its temporary is made in pass 1 (@3331, v64) where every other bound is read
 * through a hoisted address (&ext + 8, + 12, + 20) and made in pass 0 ahead of its operand's;
 * `igreplay --cls 1 --move 64=31.5` gives the target's colours. Tried: `(float)` casts, `ext->x`,
 * `(void)&ext`, lo / hi pointer locals (123-139 off), the bound through a reference or pointer inline
 * (21: the radius inline's order then lost), the extents loop as an inline (0xAE4). Measured on the
 * way: one counter reused by all four loops gives every GPR (its later webs split off, numbered after
 * the strength-reduced pointers: 77 -> 38); the min / max are value macros, the bound first (`<` and
 * `>`: a reference min selects addresses); the radius through an inline, read ahead of the centre;
 * NodeCheck stored and read back as a statement of its own, m_node assigned in the last call's
 * argument (the store in its delay slot); the tick tested `m_TickNum > Ent->m_TickNum` through
 * m_NextAtom (the address taken ahead of the test) */
/* a sphere set's radius, its tip's x (inline: the walk reads it ahead of the centre; ours) */
static inline float Radius(const DiIntersect *d) { return d->m_Tip.x; }

/* a set's search marks cleared: the node its tree search stopped at and the type's done bit
 * (inline: the set read once for both; ours) */
static inline void BatchReset(DiIntersect *d)
{
    d->m_TreeDoneNode = 0;
    d->m_Type &= ~0x8000;
}

int CGrid::GetIntersectBatch(DiIntersect **di, int n)
{
    CPnt3D ext[2];
    CPnt3D pts[2];
    int i;

    if (n == 0)
        return 0;
    if (noBatchIntersect)
        return 0;
    ext[0].x = di[0]->m_Tail.x;
    ext[0].z = di[0]->m_Tail.z;
    ext[1].x = di[0]->m_Tail.x;
    ext[1].z = di[0]->m_Tail.z;
    for (i = 0; i < n; i++) {
        di[i]->m_Cnt = 0;
        switch (di[i]->m_Type & ~0x8000) {
        case 2:
            ext[0].x = BATCH_MIN(ext[0].x, di[i]->m_Tail.x);
            ext[0].z = BATCH_MIN(ext[0].z, di[i]->m_Tail.z);
            ext[1].x = BATCH_MAX(ext[1].x, di[i]->m_Tail.x);
            ext[1].z = BATCH_MAX(ext[1].z, di[i]->m_Tail.z);
            ext[0].x = BATCH_MIN(ext[0].x, di[i]->m_Tip.x);
            ext[0].z = BATCH_MIN(ext[0].z, di[i]->m_Tip.z);
            ext[1].x = BATCH_MAX(ext[1].x, di[i]->m_Tip.x);
            ext[1].z = BATCH_MAX(ext[1].z, di[i]->m_Tip.z);
            break;
        case 4:
            ext[0].x = BATCH_MIN(ext[0].x, di[i]->m_Tail.x - Radius(di[i]));
            ext[0].z = BATCH_MIN(ext[0].z, di[i]->m_Tail.z - Radius(di[i]));
            ext[1].x = BATCH_MAX(ext[1].x, di[i]->m_Tail.x + Radius(di[i]));
            ext[1].z = BATCH_MAX(ext[1].z, di[i]->m_Tail.z + Radius(di[i]));
            break;
        case 1:
            ext[0].x = BATCH_MIN(ext[0].x, di[i]->m_Tail.x);
            ext[0].z = BATCH_MIN(ext[0].z, di[i]->m_Tail.z);
            ext[1].x = BATCH_MAX(ext[1].x, di[i]->m_Tail.x);
            ext[1].z = BATCH_MAX(ext[1].z, di[i]->m_Tail.z);
            break;
        }
    }
    SetTraversalExtents(ext, 2, false);
    int ix = m_ix;
    int iz = m_iz;
    int w = m_ixb - ix;
    int cells = w * (m_izb - iz);

    for (i = 0; i < cells; i++)
        CellCheck[i] = 0;
    for (i = 0; i < n; i++) {
        BatchReset(di[i]);
        switch (di[i]->m_Type & ~0x8000) {
        case 2:
            pts[0] = di[i]->m_Tail;
            pts[1] = di[i]->m_Tip;
            SetTraversalBoundary(pts, 2, true);
            break;
        case 4:
            pts[0].x = di[i]->m_Tail.x - di[i]->m_Tip.x;
            pts[0].z = di[i]->m_Tail.z - di[i]->m_Tip.x;
            pts[1].x = di[i]->m_Tail.x + di[i]->m_Tip.x;
            pts[1].z = di[i]->m_Tail.z + di[i]->m_Tip.x;
            SetTraversalBoundary(pts, 2, false);
            break;
        case 1:
            pts[0].x = di[i]->m_Tail.x;
            pts[0].z = di[i]->m_Tail.z;
            SetTraversalBoundary(pts, 1, false);
            break;
        }
        m_xx = m_ix;
        m_zz = m_iz - 1;
        while (gridGetNextIndex()) {
            if (!gridCellOut(m_xx, m_zz))
                CellCheck[(m_xx - ix) + w * (m_zz - iz)] |= 1 << (i % 32);
        }
    }
    SetTraversalExtents(ext, 2, false);
    stack.Push(&CMatrix::identity);
    for (m_zz = m_iz; m_zz < m_izb; m_zz++) {
        int row = w * (m_zz - iz);

        for (m_xx = m_ix; m_xx < m_ixb; m_xx++) {
            NodeCheck = CellCheck[row + (m_xx - ix)];
            if (NodeCheck) {
                for (m_NextAtom = *gridGetAtomBasePtr(m_xx, m_zz); m_NextAtom; m_NextAtom = m_NextAtom->Next) {
                    if (m_NextAtom->Ent->m_active && m_TickNum > m_NextAtom->Ent->m_TickNum) {
                        m_NextAtom->Ent->m_TickNum = m_TickNum;
                        for (i = 0; i < n; i++)
                            BatchReset(di[i]);
                        CCell *cell = m_NextAtom->GetCell();

                        if (cell) {
                            CClutterList &list = cell->m_clutterListDI;

                            if (list.size()) {
                                for (CClutterList::iterator it = list.begin(); it != list.end(); ++it) {
                                    m_node = (*it)->m_node;
                                    if (func_003084F0((*it)->GetMatrix()) == false) {
                                        stack.Multiply((*it)->GetMatrix(), true);
                                        NodeGetIntersectBatch(m_node, di, n);
                                        stack.Pop();
                                    } else
                                        NodeGetIntersectBatch(m_node, di, n);
                                }
                            }
                        }
                        NodeGetIntersectBatch(m_node = m_NextAtom->Ent, di, n);
                    }
                }
            }
        }
    }
    stack.Pop();
    return 1;
}

/* the highest hit no more than alt above the point, or none (SOCOM 1's) */
IntersectStruct *DiIntersect::GetHighestAltitude(const CPnt3D *p, float alt)
{
    if (m_Cnt <= 0)
        return 0;
    float top = p->y + alt;
    IntersectStruct *best = 0;

    for (int i = 0; i < m_Cnt; i++) {
        if (m_Intersects[i].m_pos.y <= top) {
            if (best) {
                if (m_Intersects[i].m_pos.y > best->m_pos.y)
                    best = &m_Intersects[i];
            } else
                best = &m_Intersects[i];
        }
    }
    return best;
}

/* CDI's slots as the walks call them: GetIntersect (slot 0x14) answers a bool -- the walk narrows
 * the result (andi 0xff) and keeps it unconverted, as SOCOM 1's NodeGetIntersect did -- where
 * znode.h's CDI declares an int (a marked stand-in: a virtual's return type is in no symbol, and
 * CDI's overrides are node_world.h's) */
class CDIHit : public tag_DI_HEAD {
public:
    virtual ~CDIHit();
    virtual void Allocate(int n);
    virtual void Free();
    virtual bool GetIntersect(DiIntersect *di) = 0;
};

/* the set's tail and its model-space pair (ours, inline: MW sets up a call's arguments holding an
 * inline's call first, right to left -- Transform's and the segment test's a2, a1, a0 order) */
static inline const CPnt3D *TailOf(DiIntersect *di) { return &di->m_Tail; }
static inline CPnt3D *MTailOf(DiIntersect *di) { return &di->m_MTail; }
static inline CPnt3D *MTipOf(DiIntersect *di) { return &di->m_MTip; }

/* a polygon's camera type and DI type as bytes (ours, inline) */
static inline unsigned char CameraTypeOf(const CDI *d)
{
    return d->m_flags.m_cameratype;
}

static inline unsigned char DiTypeOf(const CDI *d)
{
    return d->m_flags.m_ditype;
}

/* whether a material lets the camera through (CMaterialEx's VOLUMETRIC, ours inline) */
static inline bool Volumetric(const CMaterialEx *m)
{
    if (m->m_volumetric)
        return true;
    return false;
}

/* whether a polygon stops the ray: with the camera's collision flag wanted, one the camera may
 * not pass (camera type bit 1) or of a volumetric material lets it through; else one of camera
 * type bit 0 is left out (SOCOM II's; the name ours) */
static inline bool PolyCollides(const CDI *d)
{
    if (cameraCollideFlagRequired) {
        if (CameraTypeOf(d) & 2)
            return false;
        CMaterialEx *m = CMaterialExArray::GetMaterialEx(d->m_flags.m_material);

        if (m && Volumetric(m))
            return false;
    } else if (CameraTypeOf(d) & 1)
        return false;
    return true;
}

/* a set's search ended at a node: the node noted and the type's done bit set (ours, inline) */
static inline void MarkDone(DiIntersect *d, CNode *node)
{
    d->m_TreeDoneNode = node;
    d->m_Type |= 0x8000;
}

/* whether the batch walk's cell marks the i-th set: SOCOM II's test as the target has it, the
 * cell's marks tested for any and the set's bit for itself (a logical and, never the bitwise one;
 * ours, inline) */
static inline int BatchMarked(int i)
{
    return NodeCheck && (1 << (i % 32));
}

/* a vector's first polygon (ours, inline: the vector's address held for the read) */
static inline CDI *FirstDI(CDIVector &v)
{
    return *v.begin();
}

/* a coordinate within a radius of a centre's (ours, inline) */
static inline bool InSpan(float v, float r, float c)
{
    return v + r >= c && v - r <= c;
}

/* a point within a radius of a centre on every axis (ours, inline) */
static inline bool InCube(const CPnt3D *p, float r, const CPnt3D &c)
{
    bool xy = InSpan(p->x, r, c.x) && InSpan(p->y, r, c.y);

    return xy && InSpan(p->z, r, c.z);
}

/* a node's tree searched for a batch of sets (SOCOM 1's NodeGetIntersectBatch, SOCOM II's per-type
 * body): a node with collision, unless every set is done with it -- a set is done here when the node
 * is its own, the cell's marks miss it or its region mask leaves the node out; its matrix pushed
 * (unless the unit one) and inverted; each set not done taken into the node's space by its type --
 * a ray (2) and a point's altitude (1, the ray 50000 up and down) as NodeGetIntersect's, skipped when
 * the segment surely misses the node's bounds (a character's node, the one being searched, against
 * its box); a sphere (4) recorded as a hit on the node itself when its centre's cube holds the box's
 * centre, a character's node ending the set, the node's first polygon noted -- then the node's
 * polygons tested in turn for the rays and points; then the children; then every set this node
 * ended released.
 * OPEN, 101 instructions (unwritten before the engine core's round 26; the target's size, 0xCB4,
 * its blocks and calls): (1) the sphere's cube test -- the target tests each axis's and the x-and-y
 * results unnarrowed and narrows the whole (andi 0xff) once, where this narrows every axis's and
 * the pair's and tests the whole as it is; int returns, bool / int locals, explicit flags, the test
 * written in the body: no shape gives it (build/scratch/core/v_ngib1-4.py, 7); (2) `hit = stop` --
 * NodeGetIntersect's one open word again: the spilled flag narrowed where the target moves it;
 * (3) the frame: the point case's two ray ends sit above the sphere's centre in the target (declared
 * first), and declaring them ahead (assigned, not copy-initialised) costs 35 more. Measured on the
 * way: the cell-mark test an int inline (`!BatchMarked(i)` narrowed as the target does), the tail
 * pairs through NodeGetIntersect's accessors (Transform's and the segment test's a2, a1, a0 order),
 * the axis test `v + r >= c` (an explicit `!(v + r < c)` materialises the not), the node's first
 * polygon read through an inline taking the vector by reference (its address held, the target's
 * `addiu a1,s5,116`), `node == di[k]->m_TreeDoneNode` in the release loop */
void CGrid::NodeGetIntersectBatch(CNode *node, DiIntersect **di, int n)
{
    CDIVector::iterator it;
    CNodeVector::iterator c;

    if (!node->m_active || !node->m_hasDI)
        return;
    CDIVector *dis = &node->m_di;
    bool skip = true;

    for (int i = 0; i < n; i++) {
        DiIntersect *d = di[i];

        if (node == d->m_Node || !BatchMarked(i) || !func_002DC620(d, 1 << node->m_region_shift))
            MarkDone(d, node);
        if (skip && !(d->m_Type & 0x8000))
            skip = false;
    }
    if (!skip) {
        bool identity = func_003084F0(&node->m_matrix);

        if (!identity)
            stack.Multiply(&node->m_matrix, true);
        CMatrix inv;

        Vu0MatrixInverseWithUniformScale(&inv, CStack::m_top);
        for (int j = 0; j < n && !skip; j++) {
            DiIntersect *d = di[j];
            bool stop;

            if (d->m_Type & 0x8000)
                continue;
            switch (d->m_Type & ~0x8000) {
            case 2:
                if ((m_node->m_socom2_a1 & 4) && !d->m_IntersectCharacters) {
                    MarkDone(d, node);
                    break;
                }
                inv.Transform(TailOf(d), MTailOf(d), 2);
                if (func_003124F0(node, MTipOf(d), MTailOf(d)))
                    MarkDone(d, node);
                else if ((m_node->m_socom2_a1 & 4) && node == m_node) {
                    CPnt4D hit, hit2;
                    CPnt4D tail(d->m_MTail.x, d->m_MTail.y, d->m_MTail.z, 1.0f);
                    CPnt4D tip(d->m_MTip.x, d->m_MTip.y, d->m_MTip.z, 1.0f);

                    if (!((CBBox *)node->GetBBox())->GetIntersect(&tip, &tail, &hit, &hit2, true))
                        MarkDone(d, node);
                }
                if (!(d->m_Type & 0x8000)) {
                    stop = dis->m_bStopAtFirstIntersection;
                    if (dis->m_bReturnBackFaceIntersections)
                        CDI::m_backface_mask = 48;
                    else
                        CDI::m_backface_mask = 32;
                    d->LoadTipAndTail();
                }
                break;
            case 4:
                if ((m_node->m_socom2_a1 & 4) && !d->m_ProximityCharacters || node->m_NOTUSED) {
                    MarkDone(d, node);
                    break;
                }
                {
                    inv.Transform(TailOf(d), MTailOf(d), 1);
                    const CBBox *box = node->GetBBox();
                    float r = d->m_Tip.x;
                    CPnt3D *p = &d->m_MTail;
                    CPnt3D centre = box->m_min + (box->m_max - box->m_min) * 0.5f;

                    if (!InCube(p, r, centre)) {
                        MarkDone(d, node);
                        break;
                    }
                    int cnt = d->m_Cnt;

                    if (cnt < d->m_BufCnt) {
                        if (m_node->m_socom2_a1 & 4)
                            MarkDone(d, node);
                        if (!d->m_socom2_bit19 || d->m_socom2_bit19 && node->m_nodeEx) {
                            if (node->m_di.size())
                                d->m_Intersects[d->m_Cnt].m_poly = FirstDI(node->m_di);
                            d->m_Intersects[cnt].m_node = node;
                            d->m_Cnt++;
                        }
                    }
                }
                break;
            case 1:
                if ((m_node->m_socom2_a1 & 4) && !d->m_AltitudeCharacters) {
                    MarkDone(d, node);
                    break;
                }
                {
                    CPnt3D top = d->m_Tail;
                    top.y = 50000.0f;
                    CPnt3D bottom = d->m_Tail;
                    bottom.y = -50000.0f;

                    inv.Transform(&top, MTailOf(d), 1);
                    inv.Transform(&bottom, MTipOf(d), 1);
                }
                if (func_003124F0(node, MTipOf(d), MTailOf(d)))
                    MarkDone(d, node);
                else if ((m_node->m_socom2_a1 & 4) && node == m_node) {
                    CPnt4D hit, hit2;
                    CPnt4D tail(d->m_MTail.x, d->m_MTail.y, d->m_MTail.z, 1.0f);
                    CPnt4D tip(d->m_MTip.x, d->m_MTip.y, d->m_MTip.z, 1.0f);

                    if (!((CBBox *)node->GetBBox())->GetIntersect(&tip, &tail, &hit, &hit2, true))
                        MarkDone(d, node);
                }
                if (!(d->m_Type & 0x8000)) {
                    stop = dis->m_bStopAtFirstIntersection;
                    if (dis->m_bReturnBackFaceIntersections)
                        CDI::m_backface_mask = 48;
                    else
                        CDI::m_backface_mask = 32;
                    d->LoadTipAndTail();
                }
                break;
            }
            if (!(d->m_Type & 0x8000) && !skip) {
                bool hit = false;

                for (it = dis->begin(); !hit && it != dis->end(); it++) {
                    CDI *p = *it;

                    if (PolyCollides(p)) {
                        if (p->m_flags.m_ptcount)
                            p->LoadPlaneVU0();
                        switch (d->m_Type & ~0x8000) {
                        case 2:
                            if (DiTypeOf(p) & 2) {
                                int cnt = d->m_Cnt;

                                if (cnt < d->m_BufCnt) {
                                    hit = ((CDIHit *)p)->GetIntersect(d);
                                    if (hit) {
                                        hit = stop;
                                        CMatrix *m = CStack::m_top;
                                        IntersectStruct *is = &d->m_Intersects[cnt];
                                        CPnt3D *norm = &is->m_norm;

                                        asm {
                                            lqc2 vf4, 0(m)
                                            lqc2 vf5, 16(m)
                                            lqc2 vf6, 32(m)
                                            lqc2 vf7, 48(m)
                                            lqc2 vf27, 0(is)
                                            lqc2 vf26, 0(norm)
                                            vcallms 0x80
                                            qmtc2.i a0, vf0
                                            sqc2 vf27, 0(is)
                                            sqc2 vf26, 0(norm)
                                        }
                                        d->m_Intersects[cnt].m_node = node;
                                        d->m_Intersects[cnt].m_poly = p;
                                        d->m_Cnt++;
                                    }
                                }
                            }
                            break;
                        case 1:
                            if (DiTypeOf(p) & 1) {
                                int cnt = d->m_Cnt;

                                if (cnt < d->m_BufCnt) {
                                    hit = ((CDIHit *)p)->GetIntersect(d);
                                    if (hit) {
                                        CMatrix *m = CStack::m_top;
                                        IntersectStruct *is = &d->m_Intersects[cnt];
                                        CPnt3D *norm = &is->m_norm;

                                        asm {
                                            lqc2 vf4, 0(m)
                                            lqc2 vf5, 16(m)
                                            lqc2 vf6, 32(m)
                                            lqc2 vf7, 48(m)
                                            lqc2 vf27, 0(is)
                                            lqc2 vf26, 0(norm)
                                            vcallms 0x80
                                            qmtc2.i a0, vf0
                                            sqc2 vf27, 0(is)
                                            sqc2 vf26, 0(norm)
                                        }
                                        d->m_Intersects[cnt].m_node = node;
                                        d->m_Intersects[cnt].m_poly = p;
                                        d->m_Cnt++;
                                    }
                                }
                            }
                            break;
                        default:
                            hit = 1;
                            break;
                        }
                    }
                }
            }
        }
        CNodeVector &kids = node->m_child;

        for (c = kids.begin(); c != kids.end(); c++)
            NodeGetIntersectBatch(*c, di, n);
        if (!identity)
            stack.Pop();
    }
    for (int k = 0; k < n; k++) {
        if (node == di[k]->m_TreeDoneNode)
            BatchReset(di[k]);
    }
}

/* a node's tree searched for the hit set: a node with collision, not a character's unless
 * characters are hit, not the set's own node -- its matrix pushed (unless the unit one), the ray
 * taken into its space, skipped when the segment surely misses its bounds (a character's node, the
 * one being searched, tested against its box: a miss returns at once); its polygons tested in
 * turn, each hit taken back to the world through the stack's top and recorded, the first stopping
 * the search when the vector says so; then its children (SOCOM II's one walk under GetIntersect
 * and GetAltitude, where SOCOM 1 had NodeGetIntersect and NodeGetAltitude).
 * OPEN, 1 instruction (unwritten before the engine core's round 25; the target's size, 0x560, its
 * frame, every register and call): `done = stop` narrows the spilled flag (andi 0xff, MW's 8-bit
 * zero-extension in the PCode) where the target moves it -- bool, unsigned char, char or int flags,
 * `!!stop`, `(bool)`, `!= 0`, const, the flag assigned apart or declared at the top, read through an
 * inline: the same or worse (build/scratch/core/v_ngi3-9.py). Measured on the way: a PLAIN `asm { }`
 * block -- a volatile one disables the whole IR optimiser for the function (DisableDueToAsm: no CSE,
 * no loop invariants, the hit's n * 32 recomputed after the block, 0x568); MW passes a call's
 * arguments that hold a call -- an inline accessor's, one expression is enough -- first, right to
 * left, then the plain ones in parameter order (mweppc's evaluate_nested_function_calls): the tail
 * pair through accessors gives Transform's and the segment test's a2, a1, a0 order; the iterators
 * declared at the top and assigned from begin() (their slots 348 / 344, begin's temporaries above
 * the postfix copies); the DI vector a POINTER declared after the flag (its address in s1 for the
 * flag reads, the size read through the node) and the children through a reference (end()'s
 * vector address held in s0, begin read through the node), SOCOM 1's shape too; GetIntersect
 * answering a bool (the CDIHit stand-in above: SOCOM 1's caller narrows it the same way) */
void CGrid::NodeGetIntersect(CNode *node, DiIntersect *di)
{
    CDIVector::iterator it;
    CNodeVector::iterator c;

    if (!node->m_active || !node->m_hasDI)
        return;
    if (!di->m_IntersectCharacters && (m_node->m_socom2_a1 & 4))
        return;
    if (node == di->m_Node)
        return;
    bool skip = false;
    bool identity = func_003084F0(&node->m_matrix);

    if (!identity)
        stack.Multiply(&node->m_matrix, true);
    CMatrix inv;

    Vu0MatrixInverseWithUniformScale(&inv, CStack::m_top);
    inv.Transform(TailOf(di), MTailOf(di), 2);
    if (func_003124F0(node, MTipOf(di), MTailOf(di)))
        skip = true;
    else if ((m_node->m_socom2_a1 & 4) && node == m_node) {
        CBBox *box = (CBBox *)node->GetBBox();
        CPnt4D hit, hit2;
        CPnt4D tail(di->m_MTail.x, di->m_MTail.y, di->m_MTail.z, 1.0f);
        CPnt4D tip(di->m_MTip.x, di->m_MTip.y, di->m_MTip.z, 1.0f);

        if (!box->GetIntersect(&tip, &tail, &hit, &hit2, true)) {
            if (!identity)
                stack.Pop();
            return;
        }
    }
    if (!skip) {
        bool done = false;
        CDIVector *dis = &node->m_di;

        if (dis->size()) {
            bool stop = dis->m_bStopAtFirstIntersection;

            if (dis->m_bReturnBackFaceIntersections)
                CDI::m_backface_mask = 48;
            else
                CDI::m_backface_mask = 32;
            di->LoadTipAndTail();
            for (it = dis->begin(); !done && it != dis->end(); it++) {
                CDI *d = *it;

                if (DiTypeOf(d) & 2) {
                    if (PolyCollides(d)) {
                        if (d->m_flags.m_ptcount)
                            d->LoadPlaneVU0();
                        int n = di->m_Cnt;

                        if (n < di->m_BufCnt) {
                            done = ((CDIHit *)d)->GetIntersect(di);
                            if (done) {
                                done = stop;
                                CMatrix *m = CStack::m_top;
                                IntersectStruct *is = &di->m_Intersects[n];
                                CPnt3D *norm = &is->m_norm;

                                asm {
                                    lqc2 vf4, 0(m)
                                    lqc2 vf5, 16(m)
                                    lqc2 vf6, 32(m)
                                    lqc2 vf7, 48(m)
                                    lqc2 vf27, 0(is)
                                    lqc2 vf26, 0(norm)
                                    vcallms 0x80
                                    qmtc2.i a0, vf0
                                    sqc2 vf27, 0(is)
                                    sqc2 vf26, 0(norm)
                                }
                                di->m_Intersects[n].m_poly = d;
                                di->m_Intersects[n].m_node = node;
                                di->m_Cnt++;
                            }
                        }
                    }
                }
            }
        }
    }
    if (!skip) {
        CNodeVector &kids = node->m_child;

        for (c = kids.begin(); c != kids.end(); c++)
            NodeGetIntersect(*c, di);
    }
    if (!identity)
        stack.Pop();
}

}
