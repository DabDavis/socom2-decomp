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

/* a thing in the world: only its type (2, a SEAL) at 0x10 (SOCOM 1's DWARF) */
class CEntity {
public:
    unsigned char _0[0x10];
    unsigned char m_type;   // 0x10

    static std::list<CEntity *> m_list;
};

/* the entity with an id, if any (SOCOM 1's CEntityList::GetEntityById, on CEntity::m_list) */
extern "C" CEntity *func_0029E4E0(std::list<CEntity *> *list, unsigned int id);

extern "C" void sceVu0RotMatrixY(sceVu0FMATRIX m0, sceVu0FMATRIX m1, float ry);

/* a seal's facing (SOCOM 1's DWARF: a CPnt3D of its own name) */
class SealDir : public CPnt3D {
};

/* the way a seal faces: its node's forward axis, reversed (the name is ours) */
static inline void GetSealDir(CZSealBody *seal, SealDir *dir)
{
    dir->x = -seal->m_node->m_matrix.m_matrix[2][0];
    dir->y = -seal->m_node->m_matrix.m_matrix[2][1];
    dir->z = -seal->m_node->m_matrix.m_matrix[2][2];
}

/* a matrix's translation row (the name is ours) */
static inline void SetTranslation(CMatrix &m, float x, float y, float z)
{
    m.m_matrix[3][0] = x;
    m.m_matrix[3][1] = y;
    m.m_matrix[3][2] = z;
}

/* the narrowest world window (a variable: read with lwc1, where MW builds a constant) */
static float s_minWorldWidth = 1300.0f;

inline void CZNewHudMap::SetWorldView(float width, float cx, float cy)
{
    if (width < s_minWorldWidth)
        width = s_minWorldWidth;
    m_width = width;
    m_worldcenterx = cx;
    m_worldcentery = cy;
}

float CZNewHudMap::GetZoomOut() const
{
    return 0.000454545458f * m_width;   /* 1/2200 */
}

void CZNewHudMap::ReInit()
{
    if (m_isFullScreen)
        SwitchToCompassMap();
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
    SetWorldView(width, m_worldcenterx, m_worldcentery);
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
            CEntity *e = func_0029E4E0(&CEntity::m_list, projectile->m_shooterID);
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
 * the world window follows the viewer (SOCOM 1's, with SOCOM II's window set at the end).
 * OPEN, 0x190 against 0x1A8: the target keeps the facing (a SealDir) on the stack at sp+128 --
 * stores x, y, z there, reloads x for atan2f -- where MW scalarises it (two registers, a frame 16
 * bytes smaller). Tried: filled through an inline taking its address, read through an inline's
 * const reference, a constructor from the matrix, from the seal, from three floats, a copy out of
 * an inline returning it by value (a return slot more) -- all scalarised. The rest lines up but
 * the clamp's FPRs at the end (width f1, centre f3/f2 in the target; f3, f2/f1 here) */
void CZNewHudMap::CalcNewMatrix()
{
    float ScaleX;
    float newangle;
    float angle;
    SealDir direction;
    CPnt3D *center;

    if (!m_viewer)
        return;
    center = &m_viewer->m_posW;
    GetSealDir(m_viewer, &direction);
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
    SetWorldView(m_width, center->x, center->z);
}
