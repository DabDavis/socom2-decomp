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
 * since the engine core's round 25. Unwritten: the batch walk 0x2D3030 (SOCOM 1's
 * NodeGetIntersectBatch) and CGrid::GetIntersectBatch. */

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

/* a node's tree searched for the hit set: a node with collision, not a character's unless
 * characters are hit, not the set's own node -- its matrix pushed (unless the unit one), the ray
 * taken into its space, skipped when the segment surely misses its bounds (a character's node, the
 * one being searched, tested against its box: a miss returns at once); its polygons tested in
 * turn, each hit taken back to the world through the stack's top and recorded, the first stopping
 * the search when the vector says so; then its children (SOCOM II's one walk under GetIntersect
 * and GetAltitude, where SOCOM 1 had NodeGetIntersect and NodeGetAltitude).
 * OPEN, 5 instructions (unwritten before the engine core's round 25; the target's size, 0x560, its
 * frame and every register): Transform's and the segment test's arguments are set up left to right
 * where the target's go right to left (a2, a1, then a0 in the delay slot: a pointer local for the
 * matrix, a node member stand-in, the tail in a local -- no change), and `done = stop` narrows the
 * spilled flag (andi 0xff) where the target moves it (bool, unsigned char or int flags, `!!stop`,
 * the flag declared bare at the top: the same). Measured on the way: a PLAIN `asm { }` block -- a
 * volatile one disables the whole IR optimiser for the function (DisableDueToAsm: no CSE, no loop
 * invariants, the hit's n * 32 recomputed after the block, 0x568); the iterators declared at the
 * top and assigned from begin() (their slots 348 / 344, begin's temporaries above the postfix
 * copies); the DI vector a POINTER declared after the flag (its address in s1 for the flag reads,
 * the size read through the node) and the children through a reference (end()'s vector address
 * held in s0, begin read through the node), SOCOM 1's shape too; GetIntersect answering a bool
 * (the CDIHit stand-in above: SOCOM 1's caller narrows it the same way) */
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
    inv.Transform(&di->m_Tail, &di->m_MTail, 2);
    if (func_003124F0(node, &di->m_MTip, &di->m_MTail))
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
