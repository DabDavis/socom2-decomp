/* hud_newhudmap.cpp -- the HUD's map (OVL1 0x208FD0-0x20CFxx): SOCOM 1's unit (its DWARF names it,
 * Z:\dev\Apps\FTS\hud_newhudmap.cpp). CZNewHudMap's members to its matrix are SOCOM 1's, 4 lower
 * (C2D's); after it SOCOM II reordered them -- the circle clipper, the centre and the full-screen
 * flag right after the matrix, where SOCOM 1 kept its two background bitmaps -- then the objective
 * list and SOCOM 1's last three words. Defined in reverse of their image order (-inline deferred).
 * Written: the constructor, the explosives' items, CalcNewMatrix, UninitMap, RemoveItem, Draw and
 * the small ones; FrameObjective open. The initialiser (__sinit_hud_newhudmap.cpp 0x3FDE80) is
 * code-exact once every static is defined (round 30: the legend's pictures and flag, the scroll
 * bitmaps, the three computed floats); its four __arraydtor$N references stay unresolved (scdiff
 * compares a local function by its bytes, relocation fields included). The rest by SOCOM 1's
 * order and calls (unwritten, unnamed):
 * 0x209350 TickFullScreenAnims (it calls GetScrollPermissions), 0x209AE0 HudTick (CHUD::Tick's
 * call), 0x209EE0 TickFullScreenMap (HudTick's; SOCOM 1's TickCompassMap inlined into HudTick),
 * 0x20A120 and 0x20A330 (TickFullScreenMap's two calls; 0x20A330 is the zoom step, through the far
 * zoom then the near: its m2c draft matches), 0x20A780 SwitchMapToFullScreen (HudTick's; it calls
 * 0x20BE00 and ftsGetTacMapZoom, as SOCOM 1's calls the zoom), 0x20AAC0 GetScrollPermissions,
 * 0x20BE00 SetupNorthIndicator(CZSealBody *) (the north lines twice each), 0x20C080 the
 * legend's builder (InitMap's call; the nine labels and
 * bitmaps through MapCompass's anchor images and the string table). */

#include "hud_newhudmap.h"
#include "hud_claymoremapitem.h"
#include "hud_bombmapitem.h"
#include "hud_playermapitem.h"
#include "../../gamez/zvid/zvid.h"
#include "../../gamez/zinput/zinput.h"

#include <vector>

/* the video's scissor (zvid_init.cpp's) */
void zVid_Scissor(int x0, int y0, int x1, int y1);

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

/* the tactical map's far zoom (fts_mission.cpp's ftsGetTacMapZoomFar, 0x2AF870: SOCOM 1's, the
 * second of the three getters -- the game states' round 26 settled it by SOCOM 1's order, its
 * globals' order, PreOpen's "TacMapZoom" items and the callers; the zoom step 0x20A330 calls it,
 * then ftsGetTacMapZoomNear 0x2AF860, and SwitchMapToFullScreen 0x20A780 ftsGetTacMapZoom 0x2AF880) */
float ftsGetTacMapZoomFar();
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

/* a UI variable's hook (zui_uivar.h's, SOCOM 1's DWARF; the vtable pointer first) */
class UIVarMixin {
public:
    UIVarMixin() { m_uivar = 0; }
    virtual ~UIVarMixin() { DetachUIVar(0); }
    void DetachUIVar(CUIVariable *var);
    virtual void UpdateUIVar(CUIVariable *var);

    CUIVariable *m_uivar;   // 0x4
};   // 8 bytes

/* a plain bitmap of the UI's (zui_2d.h's, SOCOM 1's DWARF): the legend's nine pictures. Its
 * constructor is inline (out of line 0x20CD50, __construct_array's), its camera Draw a tail call
 * into the bitmap's (0x20D5E0), inlined where the map draws them; the first non-inline virtual
 * keeps the table out of this unit */
class CPlainBmp : public C2DBitmap, public UIVarMixin {
public:
    CPlainBmp()
    {
        SetColor(128.0f, 128.0f, 128.0f);
        SetTrans(128.0f);
        SetIsTrans(true);
        SetCanHandleInput(false);
        m_pulsing = false;
        m_period = 1.0f;
    }
    virtual void Draw(zdb::CCamera *cam) { C2DBitmap::Draw(cam); }
    virtual void Draw(const CMatrix &m, zdb::CTextureRelocMgr *mgr);
    virtual void Tick(float dT);
    virtual void SetMapOffset(int x, int y);

    bool m_pulsing;   // 0x88
    float m_period;   // 0x8c
    float m_curtrans;   // 0x90
} __attribute__((aligned(16)));   // 0xa0 bytes

namespace {
/* a line over the map, coloured as the north indicator's (SOCOM 1's class; its constructor's copy
 * 0x20CEE0 is __construct_array's, its destructor 0x20CE70) */
class COverlayLine : public C2DLine {
public:
    COverlayLine() { SetColor(64.0f, 192.0f, 255.0f); }
};

/* the unit's statics, 0x409F20-0x40BE7F (its initialiser 0x3FDE80), in SOCOM 1's order: the grid
 * lines (SOCOM 1's names; its sixty hash lines gone), then SOCOM II's nine C2DStrings -- the
 * full-screen map legend's labels (the legend's builder 0x20C080 loads each from the string table;
 * [0] takes SOCOM 1's g_strNorthIndN's colour in the constructor, and UninitMap blanks all nine as
 * SOCOM 1 blanked the one: the name ours) -- the legend's nine pictures (CPlainBmps, 0x40AEB0, the
 * name ours) and whether it shows (a bool, 0x40B450: the full-screen tick 0x209EE0 sets it, Draw
 * tests it; the name ours), the four scroll bitmaps (SOCOM 1's g_bmpScroll*, from 0x40B470), and
 * the north indicator's three lines. Each object follows its destructor record */
COverlayLine g_arrHorGridLines[7];
COverlayLine g_arrVertGridLines[9];
C2DString g_strLegend[9];
CPlainBmp g_bmpLegend[9];
bool g_bShowLegend;
C2DBitmapPoly g_bmpScrollDown;
C2DBitmapPoly g_bmpScrollLeft;
C2DBitmapPoly g_bmpScrollRight;
C2DBitmapPoly g_bmpScrollUp;
COverlayLine g_lineNorthIndBarb;
COverlayLine g_lineNorthIndFrontShaft;
COverlayLine g_lineNorthIndRearShaft;
}

/* the grid lines' alpha (a variable: read with lwc1) */
static float s_gridTrans = 24.0f;

/* the narrowest world window (a variable: read with lwc1, where MW builds a constant) */
static float s_minWorldWidth = 1300.0f;

/* hud_main.cpp's layout floats (365 and 60: the full-screen map's bottom and top, as InitMap
 * reads them) */
extern float g_fHudLayout365;
extern float g_fHudLayout60;

/* the full-screen map's height, an eighth of it, and that eighth corrected from 448 lines to 480
 * -- computed by the unit's initialiser after the objects (each reusing the value just stored),
 * read by the full-screen animations' tick 0x209350 (OVL1 .data 0x3E4090-0x3E40A0, 8 apart; the
 * names ours) */
static float s_fullMapHeight = g_fHudLayout365 - g_fHudLayout60;
static float s_fullMapStep = s_fullMapHeight / 8.0f;
static float s_fullMapStepX = 480.0f / 448.0f * s_fullMapStep;

inline void CZNewHudMap::SetWorldView(float cx, float cy, float width)
{
    if (width < s_minWorldWidth)
        width = s_minWorldWidth;
    m_width = width;
    m_worldcenterx = cx;
    m_worldcentery = cy;
}

/* the map's members and the unit's lines set up (SOCOM 1's, with SOCOM II's three words zeroed;
 * the lines' SetTrans goes through the vtable, COverlayLine inheriting it) */
CZNewHudMap::CZNewHudMap()
{
    m_viewer = 0;
    m_radar_distance = 100.0f;
    m_screeny2 = 0.0f;
    m_screenx2 = 0.0f;
    m_screeny1 = 0.0f;
    m_screenx1 = 0.0f;
    m_worldcentery = 0.0f;
    m_worldcenterx = 0.0f;
    m_width = 0.0f;
    m_screencentery = 0.0f;
    m_screencenterx = 0.0f;
    m_scale = 1.0f;
    m_circleclipper = new CCircleClip;
    m_centerx = 0.0f;
    m_centerz = 0.0f;
    m_isFullScreen = false;
    m_zoomoutsound = 0;
    m_zoominsound = 0;
    m_socom2_81c = 0;
    m_socom2_820 = 0;
    m_socom2_824 = 0;
    g_strLegend[0].SetColor(64.0f, 192.0f, 255.0f);
    g_lineNorthIndFrontShaft.SetTrans(128.0f);
    g_lineNorthIndRearShaft.SetTrans(128.0f);
    g_lineNorthIndBarb.SetTrans(128.0f);
    g_strLegend[0].SetTrans(128.0f);
    for (unsigned int i = 0; i < 7; i++)
        g_arrHorGridLines[i].SetTrans(s_gridTrans);
    for (unsigned int i = 0; i < 9; i++)
        g_arrVertGridLines[i].SetTrans(s_gridTrans);
}

/* the larger of two, through a temporary (the name ours) */
static inline float MaxOf(float a, float b)
{
    return b > a ? b : a;
}

/* the full-screen map centred between the viewer and the current objective (if the compass shows
 * it -- else left alone) and widened until both fit inside the frame, 20 units in, no wider than
 * the far tactical zoom allows (no SOCOM 1 counterpart; the name ours). The larger scale through
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
        float maxw = 2200.0f / ftsGetTacMapZoomFar();

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

/* the HUD's owner (hud_main.cpp's CHUD: only the static) */
class CHUD {
public:
    static CZSealBody *GetHUDOwner();
};

/* a SEAL's controller as the map reads it (zseal.h's CSealCtrl: the input bit, 0x170 bit 11) */
class CSealCtrl {
public:
    unsigned char _0[0x170];
    unsigned int _170_0 : 11;
    unsigned int m_input_enabled : 1;   // 0x171 bit 3
    unsigned int _170_12 : 20;
};

/* the map's sounds: the second slot plays one (zsnd.h's PlaySound(inst, flags)) */
class CSnd {
public:
    virtual ~CSnd();
    virtual void *PlaySound(void *inst, unsigned int flags);
};

/* the pad's button state as the game reads it (zinput.cpp's 0x2C64E0: a game function's button,
 * KEY_STATE; no row named yet) */
extern "C" unsigned char func_002C64E0(int func, CPad *pad);

/* the full-screen map's zoom step (0x20A330: its m2c draft matches, under its placeholder) */
extern "C" void func_0020A330(CZNewHudMap *map, float stick);

/* the full-screen map's pad, while no objective plays and the owner's input is on: the map closed
 * (the game's button 12 or triangle; the close sound), down / up stepping the objectives and cross
 * selecting one (each with its sound), circle framing it, square centring the map on the viewer;
 * then, the pad connected and the map full screen, the sticks scroll and zoom it; the legend shows
 * while R2 is held (SOCOM 1's TickFullScreenMap, its zoom moved out to 0x20A330 and the menu state
 * unread; the name SOCOM 1's) */
void CZNewHudMap::TickFullScreenMap(CPad *pad, MENU_STATE menustate)
{
    if (m_objectivelist.IsPlayingAnimation())
        return;
    CZSealBody *owner = CHUD::GetHUDOwner();
    CSealCtrl *ctrl = owner ? owner->m_ctrl : 0;

    bool input = ctrl ? (bool)ctrl->m_input_enabled != false : false;

    if (!input)
        return;
    if ((func_002C64E0(12, pad) == 1) != false || pad->m_state[PAD_TRIANGLE] == KEY_RISING) {
        if (m_socom2_81c)
            m_socom2_81c->PlaySound(0, 255);
        SwitchToCompassMap();
    } else if (pad->m_state[PAD_DOWN] == KEY_FALLING) {
        m_objectivelist.NextObjective();
        if (m_socom2_824)
            m_socom2_824->PlaySound(0, 255);
    } else if (pad->m_state[PAD_UP] == KEY_FALLING) {
        m_objectivelist.PreviousObjective();
        if (m_socom2_820)
            m_socom2_820->PlaySound(0, 255);
    } else if (pad->m_state[PAD_CROSS] == KEY_RISING) {
        m_objectivelist.SelectObjective();
        if (m_socom2_81c)
            m_socom2_81c->PlaySound(0, 255);
    } else if (pad->m_state[PAD_CIRCLE] == KEY_RISING) {
        FrameObjective();
    } else if (pad->m_state[PAD_SQUARE] == KEY_RISING) {
        m_centerx = m_viewer->m_posW.x;
        m_centerz = m_viewer->m_posW.z;
    }
    if (pad->m_0 && m_isFullScreen) {
        HandleScrolling(((float *)pad->m_210)[0], ((float *)pad->m_210)[1]);
        func_0020A330(this, ((float *)pad->m_210)[3]);
    }
    g_bShowLegend = pad->m_state[PAD_TRIGGER_R2] == KEY_DOWN;
}

/* a map's bounds (ai.h's CAiMap, SOCOM 1's DWARF: AIMAP_PARAMS's box first) and the AI maps' lookup
 * by index (ai_maps.h's CAiMaps; AiMaps at 0x66A1B0) -- only what the scroll reads */
class CAiMap {
public:
    CBBox m_bbox;   // 0x0
};

class CAiMaps {
public:
    CAiMap *GetMapByIndex(unsigned int i) const;
};

extern CAiMaps AiMaps;

/* the scroll's bounds without a map (SOCOM 1's DWARF: HandleScrolling's static CBBox boxFallback,
 * guarded -- an empty inline constructor; the class name ours) */
class CFallbackBox : public CBBox {
public:
    CFallbackBox() {}
};

/* the bounds the full-screen map scrolls inside: the first AI map's, else an empty box -- one
 * guarded static that HandleScrolling and GetScrollPermissions 0x20AAC0 both read, as SOCOM 1's
 * both do (an inline's static; the name ours) */
inline const CBBox *CZNewHudMap::GetScrollBounds() const
{
    static CFallbackBox boxFallback;
    CAiMap *map = AiMaps.GetMapByIndex(0);

    return map ? &map->m_bbox : &boxFallback;
}

/* the scroll's speed at the map's scale (36 in data; SOCOM 1's literal 20; the name ours) */
static float s_fScrollSpeed = 36.0f;

/* the full-screen map's centre moved by the stick, turned to the viewer's facing and kept inside
 * the first AI map's bounds -- each axis moved only while it is inside the bound it moves toward
 * (SOCOM 1's HandleScrolling, 0x3C4950: there the viewer's own map and 20 for the speed; the name
 * SOCOM 1's). The facing is a stack SealDir whose z is read through a pointer kept across the map
 * lookup (the target's s1 = sp+72, as SOCOM 1's s0); the two early returns one `||` (they share one
 * `b` to the epilogue); the speed a static (loaded from data), the fallback box an inline's guarded
 * static (GetScrollBounds 0x20AAC0 reads the same box and guard).
 * OPEN, 2 instructions and 16 registers, one swap: `this` takes s1 and the z pointer s0 (SOCOM 1's
 * own colouring), the target the reverse. Replayed (build/scratch/hud-r48/ig: rl_hs.py), z has 25
 * neighbours -- this, the node pointer, the box, the guard's load and its 1, bMoveX -- one at K, so
 * it is deferred with `this` (27) and coloured first; any one interference fewer, or `this`
 * numbered above z, gives the target. MW hoists z's `addiu` to the block's top (right after the
 * node load) wherever it is written. Tried: z declared first / at the top / after the length / in
 * the length, through the facing inline's returned pointer, a reference (the store then goes
 * through it), the move tests without bool locals or with one bool, a copy of `this` for the
 * moves, the bounds an inline member, a dead first definition of the box pointer */
void CZNewHudMap::HandleScrolling(float fStickX, float fStickY)
{
    float fZMove;
    float fXMove;
    float fConst;
    float fDirLength;

    if ((fStickX == 0.0f && fStickY == 0.0f) || !m_viewer)
        return;
    SealDir ptDir;

    GetSealDir(m_viewer, &ptDir);
    const float *z = &ptDir.z;

    fDirLength = sqrtf(ptDir.x * ptDir.x + *z * *z);
    fConst = s_fScrollSpeed / fDirLength;
    const CBBox *box = GetScrollBounds();

    fXMove = fConst * (fStickX * *z + fStickY * ptDir.x);
    bool bMoveX = fXMove > 0.0f ? m_centerx < box->m_max.x : m_centerx > box->m_min.x;

    if (bMoveX)
        m_centerx += fXMove;
    fZMove = fConst * (fStickX * -ptDir.x + fStickY * *z);
    bool bMoveZ = fZMove > 0.0f ? m_centerz < box->m_max.z : m_centerz > box->m_min.z;

    if (bMoveZ)
        m_centerz += fZMove;
}

/* each item's PostDraw (ours: an inline, so its iterator is numbered after Draw's temporaries) */
inline void CZNewHudMap::PostDrawItems(const std::list<CZMapItem *>::iterator &first,
                                       zdb::CCamera *cam)
{
    std::list<CZMapItem *>::iterator j;

    for (j = first; j != m_itemlist.end(); ++j)
        (*j)->PostDraw(cam);
}

/* the map drawn, in full screen only: the objectives (nothing more while their camera plays), the
 * map's children and items inside its rectangle (each item clipped, then drawn; then each one's
 * PostDraw), the team brackets, then the north indicator, the legend's first label, the scroll
 * arrows and the grid -- the legend's pictures and other labels while it shows -- and the scissor
 * opened again (SOCOM 1's, its sixty hash lines gone; SOCOM 1 drew the compass map too) */
void CZNewHudMap::Draw(zdb::CCamera *cam)
{
    if (!IsOn() || !m_isFullScreen)
        return;
    m_objectivelist.Draw(cam);
    if (m_objectivelist.IsPlayingAnimation())
        return;
    zVid_Scissor((int)m_screenx1, (int)CalcDrawPos(m_screeny1), (int)m_screenx2,
                 (int)CalcDrawPos(m_screeny2) - 1);
    C2D::Draw(cam);

    std::list<CZMapItem *>::iterator i;

    for (i = m_itemlist.begin(); i != m_itemlist.end(); ++i) {
        CZMapItem *item = *i;

        item->Clip(this);
        item->Draw(cam);
    }

    PostDrawItems(m_itemlist.begin(), cam);
    CZPlayerMapItem::DrawAlphaBravo(cam);
    if (m_isFullScreen) {
        g_lineNorthIndFrontShaft.Draw(cam);
        g_lineNorthIndRearShaft.Draw(cam);
        g_lineNorthIndBarb.Draw(cam);
        g_strLegend[0].Draw(cam);
        g_bmpScrollUp.Draw(cam);
        g_bmpScrollLeft.Draw(cam);
        g_bmpScrollRight.Draw(cam);
        g_bmpScrollDown.Draw(cam);
        for (unsigned int k = 0; k < 7; k++)
            g_arrHorGridLines[k].Draw(cam);
        for (int k = 0; k < 9; k++)
            g_arrVertGridLines[k].Draw(cam);
        if (g_bShowLegend) {
            for (int k = 0; k < 9; k++)
                g_bmpLegend[k].Draw(cam);
            for (int k = 1; k <= 8; k++)
                g_strLegend[k].Draw(cam);
        }
    }
    zVid_Scissor(0, 0, zVid.renderWidth, zVid.renderHeight);
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
