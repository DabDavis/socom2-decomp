/* grid_di.cpp -- the grid's collision queries: a ray's or a point's hits gathered cell by cell
 * (CGrid's GetIntersect, GetAltitude, GetIntersectBatch and the node walks under them), and the
 * hit set's answers (DiIntersect: the highest hit under a point, the nearest along the ray, the
 * hits sorted near to far) (OVL1 ~0x2D2890-0x2D4FDB). The unit is SOCOM 1's: its static
 * initialiser, __sinit_grid_di.cpp, registers CStack's destructor for its matrix stack, whose
 * linked copy (0x2D4FE0) closes the unit; grid_main.cpp follows. Defined in reverse of their
 * image order (-inline deferred).
 * Written so far: the hit set's answers. */

#include "node_world.h"

/* MSL's qsort (MAIN 0x196D38, not named in the map: SOCOM 1's SortNearToFar calls qsort) */
extern "C" void func_00196D38(void *base, unsigned int n, unsigned int size,
                              int (*cmp)(const void *, const void *));

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

/* a vector between two points, by value (the copy out of its temporary is the target's; the name
 * is ours) */
inline CPnt3D PntMinus(const CPnt3D &a, const CPnt3D &b)
{
    CPnt3D r;

    a.Sub(&b, &r);
    return r;
}

/* the hits' indices sorted by their squared distance from the ray's tail, the distances kept
 * (SOCOM 1's).
 * OPEN, 0x148 of 0x158: the frame, the loop and the sort are the target's (the tail through a
 * reference, so its address is hoisted and spilled to 160(sp)); the target places the loop's
 * preheader -- the entry test and the hoisted addresses -- after the function's last block,
 * jumping to it from the top and back to the loop test (two `b`s and their slots). Tried: an early
 * return, a while loop, the counter declared before the loop, the reference inside the loop */
void DiIntersect::SortNearToFar(int *idx, float *dist)
{
    if (m_Cnt > 0) {
        const CPnt3D &tail = m_Tail;

        for (int i = 0; i < m_Cnt; i++) {
            idx[i] = i;
            CPnt3D d = PntMinus(m_Intersects[i].m_pos, tail);

            dist[i] = d.x * d.x + d.y * d.y + d.z * d.z;
        }
        if (m_Cnt > 1) {
            _cmpdi_rs = dist;
            _cmpdi_di = this;
            func_00196D38(idx, m_Cnt, 4, _cmpdi);
            _cmpdi_di = 0;
            _cmpdi_rs = 0;
        }
    }
}

/* the hit nearest the ray's tail, or none (SOCOM 1's) */
IntersectStruct *DiIntersect::GetNearestIntersect()
{
    if (m_Cnt <= 0)
        return 0;
    int best = 0;
    CPnt3D d = PntMinus(m_Intersects[0].m_pos, m_Tail);
    float bestDist = d.x * d.x + d.y * d.y + d.z * d.z;

    for (int i = 1; i < m_Cnt; i++) {
        CPnt3D d2 = PntMinus(m_Intersects[i].m_pos, m_Tail);
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

}
