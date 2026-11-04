/* hud_newhudmap.cpp -- the HUD's map (OVL1 0x208FD0-0x20CFxx): SOCOM 1's unit (its DWARF names it,
 * Z:\dev\Apps\FTS\hud_newhudmap.cpp). The explosives' items and a few small functions so far.
 * CZNewHudMap's members to its matrix are SOCOM 1's, 4 lower (C2D's); after it SOCOM II
 * reordered them -- the circle clipper, the centre and the full-screen flag right after the matrix,
 * where SOCOM 1 kept its two background bitmaps -- and only those are placed. Defined in reverse
 * of their image order (-inline deferred). */

#include "hud_newhudmap.h"
#include "hud_claymoremapitem.h"
#include "hud_bombmapitem.h"

#include <vector>

namespace zdb {
class CNode;
}

/* the multiplayer bomb's node, while there is one (SOCOM 1's static) */
class CPickup {
public:
    static zdb::CNode *m_mpbombnode;
};

/* what UpdateExplosives reads of the weapons' projectiles: the explosives out (SOCOM 1's DWARF) */
class CZProjectileList {
public:
    CZProjectile *m_projectiles;   // 0x0
    std::vector<CZProjectile *> m_explosives;   // 0x4
};

class CZWeapon {
public:
    static CZProjectileList *m_pProjectilelist;
};

class CEntity;

/* the entities (SOCOM 1's CEntityList, a pointer list): the entity with an id, if any */
class CEntityList : public std::list<CEntity *> {
public:
    CEntity *GetEntityById(unsigned int id) const;
};

/* a thing in the world: only its type (2, a SEAL) at 0x10 (SOCOM 1's DWARF) */
class CEntity {
public:
    unsigned char _0[0x10];
    unsigned char m_type;   // 0x10

    static CEntityList m_list;
};

/* a mission objective as the map reads it (SOCOM 1's DWARF): its map position and whether the
 * compass shows it */
class CMissionObjectiveCam {
public:
    unsigned char _0[0x38];
    PNT2D m_pos;   // 0x38
    unsigned int m_isSecondary : 1;   // 0x40
    unsigned int m_isSubObjective : 1;
    unsigned int m_isCompass : 1;
    unsigned int m_bExtraction : 1;
    unsigned int m_unused : 28;
};

/* the tactical map's zoom (SOCOM 1's ftsGetTacMapZoom, 0x2AF870 here: a float global returned) */
extern "C" float func_002AF870();
/* an empty function of hud_linemap.cpp's (0x210120; SOCOM II's, unnamed) */
extern "C" void func_00210120();

extern "C" void sceVu0RotMatrixY(sceVu0FMATRIX m0, sceVu0FMATRIX m1, float ry);

/* a seal's facing (SOCOM 1's DWARF: a CPnt3D of its own name) */
class SealDir : public CPnt3D {
};

/* the way a seal faces: its node's forward axis, reversed, the matrix through a reference (the name
 * is ours) */
static inline void GetSealDir(CZSealBody *seal, SealDir *dir)
{
    const CMatrix &m = seal->m_node->m_matrix;

    dir->x = -m.m_matrix[2][0];
    dir->y = -m.m_matrix[2][1];
    dir->z = -m.m_matrix[2][2];
}

/* a matrix's translation row (the name is ours) */
static inline void SetTranslation(CMatrix &m, float x, float y, float z)
{
    m.m_matrix[3][0] = x;
    m.m_matrix[3][1] = y;
    m.m_matrix[3][2] = z;
}

namespace {
/* the full-screen map's legend labels (the legend's builder 0x20C080 loads each from the string
 * table; SOCOM 1's one g_strNorthIndN grown to nine -- the name ours). The unit's statics run
 * 0x409F20-0x40BE7F (its initialiser 0x3FDE80): two COverlayLine arrays (7 and 9) first, these at
 * 0x40A940, then nine CPlainBmps, four C2DBitmapPolys and three COverlayLines */
C2DString g_strLegend[9];
}

/* the narrowest world window (a variable: read with lwc1, where MW builds a constant) */
static float s_minWorldWidth = 1300.0f;

inline void CZNewHudMap::SetWorldView(float cx, float cy, float width)
{
    if (width < s_minWorldWidth)
        width = s_minWorldWidth;
    m_width = width;
    m_worldcenterx = cx;
    m_worldcentery = cy;
}

/* the larger of two, through a temporary (the name ours) */
static inline float MaxOf(float a, float b)
{
    return b > a ? b : a;
}

/* the full-screen map centred between the viewer and the current objective (if the compass shows
 * it -- else left alone) and widened until both fit inside the frame, 20 units in, no wider than
 * the tactical zoom allows (no SOCOM 1 counterpart; the name ours). The larger scale through
 * `b > a ? b : a` (the false arm's copy through a temporary; `b <= a ? a : b` or a ternary into
 * scale drops it), the width's clamp `w < maxw ? w : maxw` -- zmath.h's MAX/MIN shapes.
 * OPEN, 4 instructions: the centre's average. The target sums z then x into temporaries (f2, f1),
 * then multiplies x then z into the variables with 0.5 in f0; this `+=` form gets the loads, the
 * constant and the order but adds into f20/f21 directly; `x = (x + ox) * 0.5f` (either statement
 * first, named or helper temporaries, MidPoint-style inlines taking the sums, `/ 2.0f`, `0.5f *`,
 * a PNT2D or CPnt3D local) makes the x sum first and puts 0.5 in f1 (5-8 instructions) */
void CZNewHudMap::FrameObjective()
{
    float x, z;
    CMissionObjectiveCam *obj;

    z = m_viewer->m_posW.z;
    x = m_viewer->m_posW.x;
    obj = m_objectivelist.GetCurrentObjective();

    if (obj && !obj->m_isCompass)
        return;
    if (obj) {
        z += obj->m_pos.y;
        x += obj->m_pos.x;
        x *= 0.5f;
        z *= 0.5f;
    }
    m_centerx = x;
    m_centerz = z;
    CalcNewMatrix();
    if (obj) {
        CPnt3D p = MapPoint(&m_viewer->m_posW);
        float scale = 1.0f;
        float d = fabsf(m_screencenterx - p.x);
        float lim = m_screencenterx - m_screenx1 - 20.0f;

        if (!(d <= lim))
            scale = d / lim;
        d = fabsf(m_screencentery - p.y);
        lim = m_screencentery - m_screeny1 - 20.0f;
        if (!(d <= lim))
            scale = MaxOf(scale, d / lim);
        float w = m_width * scale;
        float maxw = 2200.0f / func_002AF870();

        m_width = w < maxw ? w : maxw;
        func_00210120();
    }
}

float CZNewHudMap::GetZoomOut() const
{
    return 0.000454545458f * m_width;   /* 1/2200 */
}

/* an item taken off the map if it is on it, given back for the caller to delete (HudTick's dead
 * items; no SOCOM 1 counterpart, the name ours). The walk's iterator is assigned from begin(), so
 * it stays in memory; remove() takes the item by reference, so the parameter is spilled */
CZMapItem *CZNewHudMap::RemoveItem(CZMapItem *item)
{
    std::list<CZMapItem *>::iterator i;

    for (i = m_itemlist.begin(); i != m_itemlist.end(); ++i) {
        if (*i == item) {
            m_itemlist.remove(item);
            return item;
        }
    }
    return 0;
}

void CZNewHudMap::ReInit()
{
    if (m_isFullScreen)
        SwitchToCompassMap();
}

/* the map's items deleted and the list emptied, the objectives' end cleanup, the legend's labels
 * blanked, and the map's own children dropped from the back (SOCOM 1's, the one north label grown
 * to the legend's nine) */
void CZNewHudMap::UninitMap()
{
    std::list<CZMapItem *>::iterator i;

    for (i = m_itemlist.begin(); i != m_itemlist.end(); ++i)
        delete *i;
    m_itemlist.clear();
    m_objectivelist.MissionEndCleanup();
    for (unsigned int k = 0; k < 9; k++)
        g_strLegend[k].SetString(0, true);
    while (size())
        pop_back();
}

/* the map's rectangle on screen and its centre; the compass map's clipping circle follows it */
void CZNewHudMap::SetMapScreenPos(float x1, float y1, float x2, float y2)
{
    m_screenx1 = x1;
    m_screeny1 = y1;
    m_screenx2 = x2;
    m_screeny2 = y2;
    m_screencenterx = x1 + (x2 - x1) / 2.0f;
    m_screencentery = y1 + (y2 - y1) / 2.0f;
    if (!m_isFullScreen)
        m_circleclipper->Set(m_screencenterx + C2D::m_topx, m_screencentery + C2D::m_topy,
                             m_screencentery - m_screeny1);
}

void CZNewHudMap::SetWorldWidth(float width)
{
    SetWorldView(m_worldcenterx, m_worldcentery, width);
}

/* a claymore item for every explosive out that has none yet, its owner the SEAL that set it (if
 * a SEAL did); then the bomb's (SOCOM 1's; the searches functions of their own in SOCOM II).
 * The end of the weapons' explosives is taken once, with the start */
void CZNewHudMap::UpdateExplosives()
{
    std::vector<CZProjectile *> &v = CZWeapon::m_pProjectilelist->m_explosives;

    for (std::vector<CZProjectile *>::iterator i = v.begin(), end = v.end(); i != end; ++i) {
        CZProjectile *projectile = *i;

        if (!FindClaymoreItem(projectile)) {
            CEntity *e = CEntity::m_list.GetEntityById(projectile->m_shooterID);
            CZSealBody *owner = e && e->m_type == 2 ? (CZSealBody *)e : 0;

            m_itemlist.push_back(new CZClaymoreMapItem(owner, projectile));
        }
    }
    UpdateMPBomb();
}

/* the multiplayer bomb given an item once it is on the map (SOCOM 1's UpdateExplosives' second
 * half, a function of its own in SOCOM II; the name is ours). Out of line too: it holds a call
 * MW would not inline (FindMPBombItem's, below), and MW refuses such a function -- which the
 * pragma on the callee does not reproduce */
#pragma auto_inline off
void CZNewHudMap::UpdateMPBomb()
{
    if (CPickup::m_mpbombnode && !FindMPBombItem())
        m_itemlist.push_back(new CZMPBombMapItem(CPickup::m_mpbombnode));
}

#pragma auto_inline reset

/* the bomb's item, if the map has one (SOCOM 1's). The iterator is assigned from begin(), so it
 * stays in memory (the copy is bitwise). Out of line: nine statements by MW's count, which
 * UpdateMPBomb would inline where the target calls it -- the original's had more, or a taller
 * tree */
#pragma auto_inline off
CZMPBombMapItem *CZNewHudMap::FindMPBombItem()
{
    std::list<CZMapItem *>::iterator i;

    for (i = m_itemlist.begin(); i != m_itemlist.end(); ++i) {
        CZMPBombMapItem *item = dynamic_cast<CZMPBombMapItem *>(*i);

        if (item)
            return item;
    }
    return 0;
}

#pragma auto_inline reset

/* the claymore item a projectile has, if any (SOCOM 1's UpdateExplosives searched inline; the
 * name is ours). Out of line as FindMPBombItem (UpdateExplosives calls it) */
#pragma auto_inline off
CZClaymoreMapItem *CZNewHudMap::FindClaymoreItem(CZProjectile *projectile)
{
    std::list<CZMapItem *>::iterator i;

    for (i = m_itemlist.begin(); i != m_itemlist.end(); ++i) {
        CZClaymoreMapItem *item = dynamic_cast<CZClaymoreMapItem *>(*i);

        if (item && item->m_projectile == projectile)
            return item;
    }
    return 0;
}

#pragma auto_inline reset

/* a world point taken onto the map: through its matrix, the height drawn at the map's scale (the
 * name is ours) */
CPnt3D CZNewHudMap::MapPoint(const CPnt3D *p) const
{
    CPnt3D r;

    m_matrix.Transform(p, &r, 1);
    r.y = CalcDrawPos(r.y);
    return r;
}

float CZNewHudMap::CalcDrawPos(float y) const
{
    return m_screencentery + m_scale * (y - m_screencentery);
}

/* the map's matrix from its viewer: the world moved to the viewer (the map's own centre in full
 * screen), turned to face up the way the viewer faces, scaled to the screen and centred there;
 * the world window follows the viewer (SOCOM 1's, with SOCOM II's window set at the end). The
 * facing lives on the stack (stored x, y, z at sp+128, x reloaded for atan2f) only as a reference
 * bound to a temporary whose address the filler takes -- a named SealDir, or one filled through
 * any inline, is scalarised; the filler reads the node's matrix through a reference local, or the
 * node is reloaded after each store. The window's inline takes the width LAST: the clamp's FPRs
 * (width f1, centre f3/f2) come out so, and SetWorldWidth holds */
void CZNewHudMap::CalcNewMatrix()
{
    float ScaleX;
    float newangle;
    float angle;
    CPnt3D *center;

    if (!m_viewer)
        return;
    center = &m_viewer->m_posW;
    const SealDir &direction = SealDir();
    GetSealDir(m_viewer, (SealDir *)&direction);
    angle = atan2f(direction.z, direction.x);
    m_matrix.SetIdentity();
    if (m_isFullScreen)
        SetTranslation(m_matrix, -m_centerx, 0.0f, -m_centerz);
    else
        SetTranslation(m_matrix, -center->x, center->y, -center->z);
    newangle = angle + 1.5707964f;
    if (!(newangle < 3.1415927f))
        newangle -= 6.2831855f;
    sceVu0RotMatrixY(m_matrix.m_matrix, m_matrix.m_matrix, newangle);
    ScaleX = (m_screenx2 - m_screenx1) / m_width;
    CMatrix TmpMtx;

    TmpMtx.SetZero();
    TmpMtx.m_matrix[0][0] = ScaleX;
    TmpMtx.m_matrix[1][2] = 1.0f;
    TmpMtx.m_matrix[2][1] = ScaleX;
    TmpMtx.m_matrix[3][0] = m_screencenterx;
    TmpMtx.m_matrix[3][1] = m_screencentery;
    sceVu0MulMatrix(m_matrix.m_matrix, TmpMtx.m_matrix, m_matrix.m_matrix);
    SetWorldView(center->x, center->z, m_width);
}
