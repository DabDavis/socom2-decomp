/* int_main.cpp -- the collision primitives: IntersectStruct's queries, CDI (a node's collision
 * shape: its points allocated, read and freed), DiIntersect (a ray's or a volume's hit set) and
 * the box and polygon shapes (OVL1 0x2DBFB0-0x2DDBEB). The unit name is SOCOM 1's: its asserts in
 * CDI::Allocate, Free and DiIntersect's constructor and destructor name int_main.cpp (both 2002
 * builds); int_material.cpp follows it there and here. Written so far: the small functions below.
 * Defined in reverse of their image order (-inline deferred). */

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

/* the last polygon's normal, w zero (new in SOCOM II: SOCOM 1's SetEdgeVecsAndNorm stored it in
 * the DI itself; the name is ours) */
static PNT4D diNorm;

namespace zdb {

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

}
