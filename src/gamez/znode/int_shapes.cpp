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
 * queries GetEdgeIntersectsY, GetEdgeIntersects and intersect2d (0x2DD690), and
 * DiIntersect::LoadTipAndTail (0x2DD210) and CDIPoly::GetIntersect (both open);
 * CDIPoly::GetAltitude (0x2DD8E0) and CDIBBox::GetIntersect in round 14, SetCDIBBox in round 24
 * (byte-exact since round 27) with its two statics, SOCOM 1's bbox_poly and initCDIBBoxOnce.
 * Defined in reverse of their image order (-inline deferred). */

#include "node_world.h"
#include "int_shapes.h"
#include <string.h>

/* the last polygon's normal, w zero (new in SOCOM II: SOCOM 1's SetEdgeVecsAndNorm stored it in
 * the DI itself; the name is ours). Built as (0, 1, 0, 0) by the unit's initialiser,
 * __sinit_int_shapes.cpp (0x401D80, byte-exact) */
static CPnt4D diNorm(0.0f, 1.0f, 0.0f, 0.0f);

namespace zdb {
class CDI;
}

/* the unit box's six faces and the guard that builds them once (SOCOM 1's statics, its symbol
 * table's names: bbox_poly, 0x18 bytes at 0x4D7900, and initCDIBBoxOnce in its small data; SOCOM
 * II's at 0x44F2F0 and 0x44F308, after diNorm. SetCDIBBox stores and reads them, nothing else) */
static zdb::CDI *bbox_poly[6];
static bool initCDIBBoxOnce;

/* a polygon point as a CPnt3D (its w ignored; ours) */
inline CPnt3D &Pnt3(CPnt4D &p) { return *(CPnt3D *)&p; }
/* whether a segment lies wholly beyond one of a box's six faces (ours: CDIBBox::GetIntersect's
 * rejection -- an inline's `||` of six `&&`s written in place materialises each level as the
 * target's, each pair's bool unnarrowed; through inline bool helpers each pair gains an andi, by
 * value they load first; the int return gives the final andi) */
inline int SegmentOutside(const CBBox *box, const CPnt3D *a, const CPnt3D *b)
{
    return (a->x < box->m_min.x && b->x < box->m_min.x) || (a->x > box->m_max.x && b->x > box->m_max.x) ||
           (a->y < box->m_min.y && b->y < box->m_min.y) || (a->y > box->m_max.y && b->y > box->m_max.y) ||
           (a->z < box->m_min.z && b->z < box->m_min.z) || (a->z > box->m_max.z && b->z > box->m_max.z);
}

/* two points' differences in x and in z (ours: CDIPoly::GetAltitude's edges -- the arguments
 * evaluated right to left put the second point's address first, as the target computes it, where
 * the subtraction written in place takes the first's) */
inline float SubX(const CPnt4D &a, const CPnt4D &b) { return a.x - b.x; }
inline float SubZ(const CPnt4D &a, const CPnt4D &b) { return a.z - b.z; }

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

/* the polygon's height under the model-space tail: the plane of its first three points loaded into
 * VU0 (LoadPlaneVU0's body, the normal into diNorm), none for an upright plane or a point outside an
 * edge -- walked from the last point back, each edge's xz perpendicular against the point beyond a
 * -1e-5 tolerance -- else the next hit's position (the tail at the plane's height over the first
 * point) and normal (SOCOM 1's, the normal its own field there; SOCOM II leaves the poly to the
 * caller as GetIntersect does). The edge vector a local the stores keep in memory (the points
 * re-read after each), the slopes named locals (otherwise MW multiplies the second before the first
 * sum), the hit's index read into a local ahead of the normal's temporary (built from a reference
 * to diNorm, its fields loaded z first), the position's statement reading the count again */
int CDIPoly::GetAltitude(DiIntersect *di)
{
    CPnt3D p;
    CPnt3D e;
    CPnt4D *pnts = m_pnts;

    asm volatile {
        lqc2 vf4, 0(pnts)
        lqc2 vf5, 16(pnts)
        lqc2 vf6, 32(pnts)
        vcallms 0x70
    }
    int i = m_flags.m_ptcount - 1;

    p.x = di->m_MTail.x;
    p.z = di->m_MTail.z;
    if (diNorm.y == 0.0f)
        return 0;
    int j = 0;

    do {
        e.z = SubX(m_pnts[i], m_pnts[j]);
        e.x = SubZ(m_pnts[j], m_pnts[i]);
        if (e.x * (p.x - m_pnts[i].x) + e.z * (p.z - m_pnts[i].z) <= -1e-5f)
            return 0;
        j = i;
    } while (--i >= 0);
    float inv = 1.0f / diNorm.y;
    float a = inv * -diNorm.x;
    float b = inv * -diNorm.z;

    p.y = m_pnts[0].y + a * (p.x - m_pnts[0].x) + b * (p.z - m_pnts[0].z);
    const CPnt4D &nn = diNorm;
    int c = di->m_Cnt;

    di->m_Intersects[c].m_norm = CPnt3D(nn.x, nn.y, nn.z);
    di->m_Intersects[di->m_Cnt].m_pos = p;
    return 1;
}

/* the ray against the polygon, edge by edge on VU0 (the tip and tail already loaded): the last
 * point and the first through the microprogram at 0x10, then each next point through 0x20 -- its
 * result, the hit point with a word at 12 the program zeroes for a miss, into the next hit's slot,
 * and from 0x20 the plane's normal into the slot's +0x10 -- whether every edge kept the hit (SOCOM
 * 1's, slimmed: SOCOM II syncs once a call and leaves the poly and count to the caller).
 * THE ZERO MADE INSIDE THE ASM BLOCK (`mtc1 zero, z`, the test `z == w`) gives the target's tail at
 * both tests -- the hit's lwc1, then the mtc1, a hazard nop, c.eq.s with the zero first -- where a C
 * zero (a literal, a local, a C or volatile load, GCC-style asm, two blocks, an inline: the engine
 * core's round 15, build/scratch/core/p1-p7.cpp) is scheduled into the gap MW leaves between the
 * asm's sqc2 and its aliasing lwc1, ahead of the load; plain C code with no such store gives the
 * lwc1-mtc1-nop order (164 matched sites) -- likely a stand-in for whatever the original wrote.
 * OPEN, 15-16 instructions, the target's size (0xC0): the set-up's registers and order only -- the
 * target extracts the count (a3) before it forms the hit's address and keeps p in a1 and the
 * normal's address in v1, ours p a3, normal a1, count v1; all 120 declaration orders and every
 * dependency-respecting initialisation order give 15 at best (build/scratch/core/v_poly_ord*.py).
 * Round 16, the replay (tools/igreplay.py --want n=a3,p=a1,norm=v1,is=a2,last=v0): every hit numbers
 * n first of the locals (33.5) and p after the hit count's shift temporary (v40, >= 40.5) -- p
 * defined after `is`'s address is formed; the target's temporaries differ too (its Intersects load
 * v0, the flags v1), so the flags are read before the hit's address in the original. n first, p
 * late, a dead first definition, the count or the list in a local: 15-21 (build/scratch/core/v_poly3.py);
 * a goto label, a do/while(0) or a label alone between the asm and the test merges away (MW keeps
 * one block: the C zero still goes ahead of the load, v_bbt_lbl.py); round 21: p through an
 * identity inline (numbered late, the trick that fixed buildOrderedCellAtomList) -- 16, 20 with n first */
bool CDIPoly::GetIntersect(DiIntersect *di)
{
    IntersectStruct *is = &di->m_Intersects[di->m_Cnt];
    CPnt3D *norm = &is->m_norm;
    int n = m_flags.m_ptcount;
    CPnt4D *p = m_pnts;
    CPnt4D *last = p + n - 1;
    float w, z;

    asm volatile {
        lqc2 vf4, 0(last)
        lqc2 vf5, 0(p)
        vcallms 0x10
        qmtc2.i a0, vf0
        sqc2 vf29, 0(is)
        lwc1 w, 12(is)
        mtc1 zero, z
    }
    if (z == w)
        return 0;
    do {
        p++;
        asm volatile {
            vadd.xyz vf4, vf5, vf0
            lqc2 vf5, 0(p)
            vcallms 0x20
            qmtc2.i a0, vf0
            sqc2 vf29, 0(is)
            sqc2 vf3, 0(norm)
            lwc1 w, 12(is)
            mtc1 zero, z
        }
        if (z == w)
            return 0;
    } while (--n);
    return 1;
}

/* the back-face mask as the tail's w, through an enum-typed by-value parameter (ours -- the mask's
 * type in no DWARF -- the engine core's round 27): enums are not propagated, so the mask stays in the
 * inline's temporary, numbered after the asm's operands and ahead of the 1.0f constant */
enum BackfaceMask { MASK_NONE = 0, MASK_ALL = 0x7FFFFFFF };
static inline void SetMaskW(CPnt4D &p, BackfaceMask m)
{
    *(int *)&p.w = m;
}

/* the model-space tip and tail as points (w 1), the tail's w then the back-face mask's bits, loaded
 * into VU0 -- the tip in vf1, the tail in vf2 -- for the polygon queries' microprograms (SOCOM 1's,
 * which wrote the pair to VU0's data memory instead). Byte-exact since the engine core's round 27
 * (5 off since round 10): the mask's load and the 1.0f constant traded v1 / a1 until the mask went
 * through SetMaskW's enum parameter -- every other mask local, const or dead first definition
 * folded back into the store, a named enum local took the first number (a3), an `int` parameter
 * was substituted (build/scratch/core/v_ltt1-3.py). The asm's operands listed tail first fix the
 * address pair. */
void DiIntersect::LoadTipAndTail()
{
    CPnt4D tip, tail;

    tail = CPnt4D(m_MTail.x, m_MTail.y, m_MTail.z, 1.0f);
    SetMaskW(tail, (BackfaceMask)CDI::m_backface_mask);
    tip = CPnt4D(m_MTip.x, m_MTip.y, m_MTip.z, 1.0f);
    asm __volatile__ (
        "lqc2 $vf1, 0(%1)\n"
        "lqc2 $vf2, 0(%0)\n"
        : : "r"(&tail), "r"(&tip) : "memory");
}

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
    CPnt3D e0 = *(CPnt3D *)&p[1] - *(CPnt3D *)&p[0];
    CPnt3D e1 = *(CPnt3D *)&p[2] - *(CPnt3D *)&p[0];
    CPnt4D n;
    CPnt4D a(e0.x, e0.y, e0.z, 1.0f);
    CPnt4D b(e1.x, e1.y, e1.z, 1.0f);

    sceVu0OuterProduct((float *)&n, (float *)&a, (float *)&b);
    sceVu0Normalize((float *)&n, (float *)&n);
    VecSet(&diNorm.x, n.x, n.y, n.z, 0.0f);
}

/* the ray against the box: none when tail and tip lie beyond the same face, else the box's corners
 * through VU0's microprogram at 0x30 (the high corner's w the back-face flag: 0 when the mask is 48,
 * 224 else), its hit and normal into the next hit's slot, whether the hit's w is set (SOCOM 1's;
 * written in the engine core's round 14). The tail numbered before the tip but set after it (the
 * target computes the tip's address first and gives the pair t3/t2).
 * The zero is made inside the asm block, as in CDIPoly::GetIntersect (above), which gives the target's
 * lwc1-mtc1-nop order and its size.
 * Byte-exact since the engine core's round 27 (2 off since round 14): the return 0's `move v0,zero` in
 * the bc1f's delay slot. MW's filler does not hoist it across the branch while the branch's scheduling
 * block also writes or reads v0 (the normal's address); the `.set reorder` after the last store splits
 * that block off (the zdb world's round 33: a `.set` in an asm block splits MW's scheduling block at no
 * code cost; ahead of the store or after the block's last instruction it changes nothing; `.set
 * noreorder` is 8 bytes shorter -- it drops the mtc1's load-delay nop). */
bool CDIBBox::GetIntersect(DiIntersect *di)
{
    const CBBox *box = m_bbox;
    const CPnt3D *tail;
    const CPnt3D *tip;

    tip = &di->m_MTip;
    tail = &di->m_MTail;
    if (SegmentOutside(box, tail, tip))
        return 0;
    CPnt4D lo;
    CPnt4D hi;
    CPnt4D *plo = &lo;
    CPnt4D *phi = &hi;

    lo.x = m_bbox->m_min.x;
    lo.y = m_bbox->m_min.y;
    lo.z = m_bbox->m_min.z;
    hi.x = m_bbox->m_max.x;
    hi.y = m_bbox->m_max.y;
    hi.z = m_bbox->m_max.z;
    int *flag = (int *)&phi->w;

    if (m_backface_mask != 48)
        *flag = 224;
    else
        *flag = 0;
    IntersectStruct *is = &di->m_Intersects[di->m_Cnt];
    CPnt3D *norm = &is->m_norm;
    float w, z;

    asm volatile {
        lqc2 vf3, 0(plo)
        lqc2 vf4, 0(phi)
        vcallms 0x30
        qmtc2.i a0, vf0
        sqc2 vf10, 0(is)
        sqc2 vf30, 0(norm)
        .set reorder
        lwc1 w, 12(is)
        mtc1 zero, z
    }
    if (z == w)
        return 0;
    return 1;
}

/* a box answers no altitude query (SOCOM 1's, the same two words) */
int CDIBBox::GetAltitude(DiIntersect *di)
{
    return 0;
}

/* GetBBoxCorners (0x2DCCD0): int_shapes.h's inline, its out-of-line copy generated after
 * SetCDIBBox, the function needing it (the engine core's round 26: a plain definition here before) */

/* a box's eight corners as a member array (ours -- the device of zparticle_source.cpp's FaceCorners
 * and node_model.cpp's BoxCorners: the target builds the corners through CPnt3D()'s out-of-line copy
 * 0x1FB3C0 in a loop, which a member array built by a constructor MW cannot expand whole gives; a
 * local array leaves no construction at all) */
struct UnitBoxCorners {
    UnitBoxCorners(const CBBox *box) { GetBBoxCorners(m_pts, box); }

    CPnt3D m_pts[8];
};

/* the unit box's six faces, built once behind a guard (SOCOM 1's static, its symbol table's
 * zdb::CDIBBox::SetCDIBBox() at 0x21C7F0, 0xD1C bytes there: SOCOM 1's built the corners from
 * CPnt3D(f, f, f)s and each normal from two Subs and a Cross in place, SOCOM II's through
 * GetBBoxCorners and SetEdgeVecsAndNorm): the box -0.5..0.5, its corners, and a quad newed into
 * bbox_poly for each face -- 7654, 0374, 2673, 5621, 4510 and 0123 -- each read back from the
 * array into one local (the target's s0 for all six, stored then reloaded).
 * Byte-exact since the engine core's round 27 (32 off since round 24): the eight corner addresses'
 * saved registers (c7, c6, c5, c4, c0, c3, c2, c1 in s1, s2, s6, s7, s3, s5, s4, s1) are a pure numbering
 * -- all eight were IRO_CommonSubs temporaries, numbered by each corner's first reuse (c7, c4, c6, c3,
 * c5, c2, c1, c0 as the faces read), and `igreplay --permute` (8 of the 40,320 orders, round 25) wants
 * c3, c2, c0, c6, c7, c5, c4 with c1 anywhere. The source shape: each address held in an int local
 * (ours, a marked device: a type conversion stops propagation and the common subexpression, so the
 * local keeps its number; an enum holds it too) declared in that order and assigned at its first use.
 * The six faces are written out so the assignments land at their memcpys; EndFace keeps
 * SetEdgeVecsAndNorm called (its tree is a level taller than the level: only EndFace's body is
 * inlined, where written in place MW inlines it). Settled on the way: the poly one named local (s0
 * throughout), the guard a bool (li 1), the box built field by field before the corners (the target's
 * stores ahead of the loop). */
/* a face's normal worked out (ours, inline; see SetCDIBBox) */
inline void EndFace(CDI *poly)
{
    poly->SetEdgeVecsAndNorm();
}

void CDIBBox::SetCDIBBox()
{
    if (initCDIBBoxOnce)
        return;
    initCDIBBoxOnce = true;

    CBBox box;
    CDI *poly;

    box.m_min.x = -0.5f;
    box.m_min.y = -0.5f;
    box.m_min.z = -0.5f;
    box.m_max.x = 0.5f;
    box.m_max.y = 0.5f;
    box.m_max.z = 0.5f;
    int w3, w2, w0, w6, w7, w5, w4, w1;
    UnitBoxCorners c(&box);

    bbox_poly[0] = new CDIPoly;
    poly = bbox_poly[0];
    poly->Allocate(4);
    poly->m_flags.m_ditype = 2;
    memcpy(&poly->m_pnts[0], (CPnt3D *)(w7 = (int)&c.m_pts[7]), 12);
    memcpy(&poly->m_pnts[1], (CPnt3D *)(w6 = (int)&c.m_pts[6]), 12);
    memcpy(&poly->m_pnts[2], (CPnt3D *)(w5 = (int)&c.m_pts[5]), 12);
    memcpy(&poly->m_pnts[3], (CPnt3D *)(w4 = (int)&c.m_pts[4]), 12);
    EndFace(poly);
    bbox_poly[1] = new CDIPoly;
    poly = bbox_poly[1];
    poly->Allocate(4);
    poly->m_flags.m_ditype = 2;
    memcpy(&poly->m_pnts[0], (CPnt3D *)(w0 = (int)&c.m_pts[0]), 12);
    memcpy(&poly->m_pnts[1], (CPnt3D *)(w3 = (int)&c.m_pts[3]), 12);
    memcpy(&poly->m_pnts[2], (CPnt3D *)w7, 12);
    memcpy(&poly->m_pnts[3], (CPnt3D *)w4, 12);
    EndFace(poly);
    bbox_poly[2] = new CDIPoly;
    poly = bbox_poly[2];
    poly->Allocate(4);
    poly->m_flags.m_ditype = 2;
    memcpy(&poly->m_pnts[0], (CPnt3D *)(w2 = (int)&c.m_pts[2]), 12);
    memcpy(&poly->m_pnts[1], (CPnt3D *)w6, 12);
    memcpy(&poly->m_pnts[2], (CPnt3D *)w7, 12);
    memcpy(&poly->m_pnts[3], (CPnt3D *)w3, 12);
    EndFace(poly);
    bbox_poly[3] = new CDIPoly;
    poly = bbox_poly[3];
    poly->Allocate(4);
    poly->m_flags.m_ditype = 2;
    memcpy(&poly->m_pnts[0], (CPnt3D *)w5, 12);
    memcpy(&poly->m_pnts[1], (CPnt3D *)w6, 12);
    memcpy(&poly->m_pnts[2], (CPnt3D *)w2, 12);
    memcpy(&poly->m_pnts[3], (CPnt3D *)(w1 = (int)&c.m_pts[1]), 12);
    EndFace(poly);
    bbox_poly[4] = new CDIPoly;
    poly = bbox_poly[4];
    poly->Allocate(4);
    poly->m_flags.m_ditype = 2;
    memcpy(&poly->m_pnts[0], (CPnt3D *)w4, 12);
    memcpy(&poly->m_pnts[1], (CPnt3D *)w5, 12);
    memcpy(&poly->m_pnts[2], (CPnt3D *)w1, 12);
    memcpy(&poly->m_pnts[3], (CPnt3D *)w0, 12);
    EndFace(poly);
    bbox_poly[5] = new CDIPoly;
    poly = bbox_poly[5];
    poly->Allocate(4);
    poly->m_flags.m_ditype = 2;
    memcpy(&poly->m_pnts[0], (CPnt3D *)w0, 12);
    memcpy(&poly->m_pnts[1], (CPnt3D *)w1, 12);
    memcpy(&poly->m_pnts[2], (CPnt3D *)w2, 12);
    memcpy(&poly->m_pnts[3], (CPnt3D *)w3, 12);
    EndFace(poly);
}

}
