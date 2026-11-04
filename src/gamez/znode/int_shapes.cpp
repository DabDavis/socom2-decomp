/* int_shapes.cpp -- the collision shapes: SetCDIBBox (0x2DC7A0, a static: the unit box's polygons
 * built once behind a guard byte -- CMission::Init and CZSealBody's constructor call it), CDIBBox
 * and CDIPoly with their queries, and CDI's and DiIntersect's VU0 edge and plane math (OVL1
 * 0x2DC7A0-0x2DDC2F). Its own unit, after int_main.cpp and before int_material.cpp: SetCDIBBox calls
 * CDI() out of line, which inlines into any caller of its own unit (the shapes' constructors needed
 * a pin for it in one unit), and the shapes call CDI() and ~CDI out of line, CDI::Create CDIPoly()
 * (README "The inlining mode, measured"). Its static, diNorm, is read and written only here
 * (SetEdgeVecsAndNorm, CDIPoly::GetAltitude) and built by this unit's initialiser (0x401D80, between
 * grid_clutter.cpp's and int_material.cpp's in the table). SOCOM 1 names no file for
 * these functions (the beta's int_main.cpp asserts end at DiIntersect(n), before SetCDIBBox); the
 * unit's name is ours. Written so far: the shapes' constructors and destructors,
 * CDIBBox::GetAltitude, SetEdgeVecsAndNorm, LoadPlaneVU0, GetBBoxCorners and (round 10) the edge
 * queries GetEdgeIntersectsY, GetEdgeIntersects and intersect2d (0x2DD690). Unwritten, in image
 * order: SetCDIBBox, CDIBBox::GetIntersect, CDIPoly::GetIntersect, DiIntersect::LoadTipAndTail
 * (0x2DD210) and CDIPoly::GetAltitude -- VU0 code throughout. Defined in reverse of their image
 * order (-inline deferred). */

#include "node_world.h"

/* the last polygon's normal, w zero (new in SOCOM II: SOCOM 1's SetEdgeVecsAndNorm stored it in
 * the DI itself; the name is ours). Built as (0, 1, 0, 0) by the unit's initialiser,
 * __sinit_int_shapes.cpp (0x401D80, byte-exact) */
static CPnt4D diNorm(0.0f, 1.0f, 0.0f, 0.0f);

namespace zdb {

/* a polygon: CDI's (SOCOM 1's, the unit's last function; int_main.cpp's CDI::Create calls it) */
CDIPoly::CDIPoly()
{
}

/* the points freed through the table, then CDI's (SOCOM 1's) */
CDIPoly::~CDIPoly()
{
    Free();
}

/* a box not yet given its bounds (SOCOM 1's) */
CDIBBox::CDIBBox()
{
    m_bbox = 0;
}

/* the points freed through the table, then CDI's (SOCOM 1's) */
CDIBBox::~CDIBBox()
{
    Free();
}

}

/* where the point falls along the edge from a to b, measured in the xz plane: its projection
 * onto the edge past either end (beyond a -1e-5 tolerance) is no crossing; at an end, the end
 * itself goes into out, still no crossing; between them the point on the edge at that fraction
 * goes into out (SOCOM 1's intersect2d, the same Sub calls, tolerance and frame). The two
 * projections declared first (numbered before the points: s takes $f21, the edge's z -- read into a
 * local ahead of the second Sub, as the target loads it -- and t share $f20) */
bool intersect2d(const CPnt3D *p, const CPnt3D *a, const CPnt3D *b, CPnt3D *out)
{
    float s, t;
    CPnt3D e = *b - *a;
    float ez = e.z;
    CPnt3D d = *p - *a;
    s = e.x * d.x + ez * d.z;

    if (s < -1e-5f)
        return false;
    if (s <= 0.0f) {
        *out = *a;
        return false;
    }
    CPnt3D d2 = *p - *b;
    t = -(e.x * d2.x + ez * d2.z);

    if (t < -1e-5f)
        return false;
    if (t <= 0.0f) {
        *out = *b;
        return false;
    }
    *out = *a + (*b - *a) * (s / (t + s));
    return true;
}

namespace zdb {

/* a polygon point as a CPnt3D (its w ignored; ours) */
inline CPnt3D &Pnt3(CPnt4D &p) { return *(CPnt3D *)&p; }

/* the polygon's edges whose height range holds the point's (each edge from the previous point, up
 * or down), the point at that height on each kept in out -- the edge's first point when the edge is
 * level; how many (SOCOM 1's) */
int CDI::GetEdgeIntersectsY(CPnt3D *p, CPnt3D *out)
{
    if (m_flags.m_ptcount < 3)
        return 0;
    int count = 0;
    int prev = m_flags.m_ptcount - 1;
    int i = 0;

    do {
        if ((p->y > m_pnts[prev].y && p->y <= m_pnts[i].y) ||
            (p->y >= m_pnts[i].y && p->y < m_pnts[prev].y)) {
            float d = m_pnts[i].y - m_pnts[prev].y;

            if (d == 0.0f)
                out[count++] = Pnt3(m_pnts[prev]);
            else
                out[count++] = Pnt3(m_pnts[prev]) +
                               (Pnt3(m_pnts[i]) - Pnt3(m_pnts[prev])) * ((p->y - m_pnts[prev].y) / d);
        }
        prev = i;
    } while (++i < m_flags.m_ptcount);
    return count;
}

/* the polygon's edges the point's line crosses (each edge from the previous point), their
 * crossings kept -- at most four -- and the highest and the lowest of them handed back; how many
 * (SOCOM 1's: the count read in place, no local -- an int local adds an andi -- and re-read at each
 * pass of a do/while, which has no entry test; the second walk reuses the first's counter) */
int CDI::GetEdgeIntersects(CPnt3D *p, CPnt3D *hi, CPnt3D *lo)
{
    if (m_flags.m_ptcount < 3)
        return 0;
    CPnt3D pts[4];
    int count = 0;
    int prev = m_flags.m_ptcount - 1;
    int i = 0;

    do {
        if (intersect2d(p, (CPnt3D *)&m_pnts[prev], (CPnt3D *)&m_pnts[i], &pts[count]))
            count++;
        prev = i;
    } while (++i < m_flags.m_ptcount);
    if (count == 0)
        return 0;
    *hi = pts[0];
    *lo = pts[0];
    for (i = 1; i < count; i++) {
        if (pts[i].y > hi->y)
            *hi = pts[i];
        if (pts[i].y < lo->y)
            *lo = pts[i];
    }
    return count;
}

/* (hand-written VU0 code, an asm function: the points' pointer at 0x10, three quadwords) */
asm void CDI::LoadPlaneVU0()
{
    lw v1, 16(a0)
    lqc2 vf4, 0(v1)
    lqc2 vf5, 16(v1)
    lqc2 vf6, 32(v1)
    vcallms 0x70
    jr ra
    nop
}

/* the polygon's normal from its first three points, normalised on VU0 (SOCOM 1's) */
void CDI::SetEdgeVecsAndNorm()
{
    CPnt4D *p = m_pnts;
    CPnt3D e0 = PntMinus(*(CPnt3D *)&p[1], *(CPnt3D *)&p[0]);
    CPnt3D e1 = PntMinus(*(CPnt3D *)&p[2], *(CPnt3D *)&p[0]);
    CPnt4D n;
    CPnt4D a(e0.x, e0.y, e0.z, 1.0f);
    CPnt4D b(e1.x, e1.y, e1.z, 1.0f);

    sceVu0OuterProduct((float *)&n, (float *)&a, (float *)&b);
    sceVu0Normalize((float *)&n, (float *)&n);
    VecSet(&diNorm.x, n.x, n.y, n.z, 0.0f);
}

/* a box answers no altitude query (SOCOM 1's, the same two words) */
int CDIBBox::GetAltitude(DiIntersect *di)
{
    return 0;
}

/* a point set from three values (the arguments evaluated right to left, so z, y and x are loaded
 * before the stores; an inline leaves no symbol: the name is ours) */
inline void PntSet(CPnt3D &p, float x, float y, float z)
{
    p.x = x;
    p.y = y;
    p.z = z;
}

/* a box's eight corners: the bottom four, then the top four, each ring from the low x, high z
 * corner (new in SOCOM II, after SOCOM 1's SetCDIBBox; the name is ours) */
void GetBBoxCorners(CPnt3D *out, const CBBox *b)
{
    PntSet(out[0], b->m_min.x, b->m_min.y, b->m_max.z);
    out[1] = b->m_min;
    PntSet(out[2], b->m_max.x, b->m_min.y, b->m_min.z);
    PntSet(out[3], b->m_max.x, b->m_min.y, b->m_max.z);
    PntSet(out[4], b->m_min.x, b->m_max.y, b->m_max.z);
    PntSet(out[5], b->m_min.x, b->m_max.y, b->m_min.z);
    PntSet(out[6], b->m_max.x, b->m_max.y, b->m_min.z);
    out[7] = b->m_max;
}

}
