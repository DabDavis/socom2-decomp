/* int_main.cpp -- the collision primitives: IntersectStruct's queries, CDI (a node's collision
 * shape: its points allocated, read and freed) and DiIntersect (a ray's or a volume's hit set)
 * (OVL1 0x2DBFB0-0x2DC79F). The unit name is SOCOM 1's: its asserts in CDI::Allocate, Free and
 * DiIntersect's constructor and destructor name int_main.cpp (both 2002 builds), and the beta's
 * last one is in DiIntersect(n), right before SetCDIBBox. The shapes after it -- SetCDIBBox, CDIBBox,
 * CDIPoly and the edge math -- are the next unit, int_shapes.cpp (0x2DC7A0-0x2DDC2F): SetCDIBBox and
 * the shapes' constructors and destructors call CDI() and ~CDI out of line and CDI::Create calls
 * CDIPoly() out of line, where one unit had needed all three pinned (README "The inlining mode,
 * measured"). Written so far: the small functions below. Unwritten: ~DiIntersect (0x2DC680),
 * IsInside, GetRegion, Release and 0x2DC620. Defined in reverse of their image order
 * (-inline deferred). */

#include "node_world.h"

/* the system allocator (SOCOM 1's _malloc__FUiPCci, 0x34EC30) */
void *_malloc(unsigned int size, const char *file, int line);

/* the materials' table (int_material.cpp's, SOCOM 1's class): the default material a hit polygon
 * without one reports, SetDefaultMaterial's (0x2DDC30) first store (new in SOCOM II; the name is
 * ours) */
class CMaterialExArray {
public:
    static int m_defaultMaterial;
};

namespace zdb {

/* a hit set for n hits: characters and proximity hit, no node ignored, every mask bit (SOCOM 1's;
 * the mask and the bit after the flags SOCOM II's) */
DiIntersect::DiIntersect(int n)
{
    m_BufCnt = n;
    m_Intersects = (IntersectStruct *)_malloc(n * sizeof(IntersectStruct), 0, 0);
    m_Node = 0;
    m_Cnt = 0;
    m_Type = 0;
    m_IntersectCharacters = true;
    m_AltitudeCharacters = false;
    m_ProximityCharacters = true;
    m_mask = -1;
    m_socom2_bit19 = false;
}

void DiIntersect::SetMask(unsigned int mask)
{
    if (mask)
        m_mask = mask | 1;
    else
        m_mask = -1;
}

/* no points and no holders, the parameters' constructor inlined first (SOCOM 1's). int_shapes.cpp's
 * constructors and destructors call it and ~CDI */
CDI::CDI()
{
    m_pnts = 0;
    m_refcount = 0;
}

/* the points freed (SOCOM 1's, through its vtable) */
CDI::~CDI()
{
    Free();
}

/* room for n points (SOCOM 1's) */
void CDI::Allocate(int n)
{
    m_flags.m_ptcount = n;
    m_pnts = (CPnt4D *)_malloc(n * sizeof(CPnt4D), 0, 0);
}

/* the points freed and counted out (SOCOM 1's) */
void CDI::Free()
{
    if (m_pnts)
        _free(m_pnts, 0, 0);
    m_pnts = 0;
    m_pnts = 0;
    m_flags.m_ptcount = 0;
}

/* a polygon read from the open key, or none (SOCOM 1's) */
CDI *CDI::Create(CSaveLoad &io)
{
    CDI *di = 0;

    if (io.m_zfile.GetOpenKey()) {
        di = new CDIPoly;
        if (!di->Read(io)) {
            delete di;
            di = 0;
        }
    }
    return di;
}

/* the parameters and then the points read from the open key, a shape without a type made a
 * polygon, no holders yet (SOCOM 1's; the result declared after the key is fetched, so it is not
 * live across that call and stays out of the saved registers) */
bool CDI::Read(CSaveLoad &io)
{
    zar::CZAR *zfile = &io.m_zfile;
    zar::CKey *key = zfile->GetOpenKey();
    bool ok = false;

    if (key) {
        ok = zfile->Fetch("params", this, 12);
        if (ok) {
            if (m_flags.m_ditype == 0)
                m_flags.m_ditype = 3;
            Allocate(m_flags.m_ptcount);
            ok = zfile->Fetch("points", m_pnts, m_flags.m_ptcount * sizeof(CPnt4D));
            m_refcount = 0;
        }
    }
    return ok;
}

int IntersectStruct::GetMaterial()
{
    if (!m_poly)
        return 0;
    int material = m_poly->m_flags.m_material;

    if (!material)
        material = CMaterialExArray::m_defaultMaterial;
    return material;
}

/* whether the hit polygon has the flag at bit 27 of its parameters (new in SOCOM II, between
 * SOCOM 1's edge-point queries and IsInside; the name is ours) */
bool IntersectStruct::GetPolyFlag27()
{
    if (!m_poly)
        return false;
    return m_poly->m_flags.m_flag27 ? true : false;
}

/* the hit polygon's edges crossed by the vertical through the hit, in the world's frame, their
 * count given back when two or more (SOCOM 1's; SOCOM II transforms through the matrix's
 * out-of-line Transform) */
bool IntersectStruct::GetIntersectedEdgePointsY(CPnt3D *out)
{
    CMatrix mtw, unused;

    if (!m_node->BuildMTW(mtw))
        return false;
    CMatrix inv;

    Vu0MatrixInverseWithUniformScale(&inv, &mtw);
    CPnt3D p;

    inv.Transform(&m_pos, &p, 1);
    int n = m_poly->GetEdgeIntersectsY(&p, out);

    if (n > 1) {
        mtw.Transform(out, n);
        return true;
    }
    return false;
}

/* the hit polygon's two edge points through the hit, in the world's frame (SOCOM 1's) */
bool IntersectStruct::GetIntersectedEdgePoints(CPnt3D *a, CPnt3D *b)
{
    CMatrix mtw, unused;

    if (!m_node->BuildMTW(mtw))
        return false;
    CMatrix inv;

    Vu0MatrixInverseWithUniformScale(&inv, &mtw);
    CPnt3D p;

    inv.Transform(&m_pos, &p, 1);
    if (m_poly->GetEdgeIntersects(&p, a, b)) {
        mtw.Transform(a, 1);
        mtw.Transform(b, 1);
        return true;
    }
    return false;
}

}
