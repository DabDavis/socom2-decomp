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
 * CDIBBox::GetAltitude, SetEdgeVecsAndNorm and GetBBoxCorners. Unwritten, in image order:
 * SetCDIBBox, CDIBBox::GetIntersect, 0x2DD130 (the polygon's first three points loaded on VU0,
 * vcallms 0x70: new), CDIPoly::GetIntersect, DiIntersect::LoadTipAndTail (0x2DD210),
 * GetEdgeIntersectsY, GetEdgeIntersects, intersect2d (0x2DD690) and CDIPoly::GetAltitude -- VU0 code
 * throughout. Defined in reverse of their image order (-inline deferred). */

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
