/* hud_newhudmap.cpp -- the HUD's map (OVL1 0x208FD0-0x20CFxx): SOCOM 1's unit (its DWARF names it,
 * Z:\dev\Apps\FTS\hud_newhudmap.cpp). CZNewHudMap's members to its matrix are SOCOM 1's, 4 lower
 * (C2D's); after it SOCOM II reordered them -- the circle clipper, the centre and the full-screen
 * flag right after the matrix, where SOCOM 1 kept its two background bitmaps -- then the objective
 * list and SOCOM 1's last three words. Defined in reverse of their image order (-inline deferred).
 * Written: the constructor, the explosives' items, CalcNewMatrix, UninitMap, RemoveItem, Draw and
 * the small ones, TickFullScreenMap (round 48), SetupNorthIndicator, SwitchToCompassMap and
 * SwitchMapToFullScreen (round 52), TickFullScreenAnims (round 57), FrameObjective (round 58);
 * open: HandleScrolling and GetScrollPermissions (round 48, SOCOM 1's names). The initialiser (__sinit_hud_newhudmap.cpp 0x3FDE80) is
 * code-exact once every static is defined (round 30: the legend's pictures and flag, the scroll
 * bitmaps, the three computed floats); its four __arraydtor$N references stay unresolved (scdiff
 * compares a local function by its bytes, relocation fields included). The rest by SOCOM 1's
 * order and calls (unwritten, unnamed):
 * 0x209AE0 HudTick (CHUD::Tick's
 * call; SOCOM 1's TickCompassMap inlined into it), 0x20A330 (TickFullScreenMap's second call, after
 * HandleScrolling: the zoom step, through the far zoom then the near: its m2c draft matches),
 * 0x20C080 the legend's builder (InitMap's call; the nine labels and
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
    typedef std::__list_imp<const void *, std::allocator<const void *> > base2;
    CEntity *GetEntityById(unsigned int id) const;
    const_iterator end() const { return const_iterator(base2::const_iterator(base2::end())); }
};

/* a thing in the world: only its type (2, a SEAL) at 0x10 (SOCOM 1's DWARF) */
class CEntity {
public:
    unsigned char _0[0x10];
    unsigned char m_type;   // 0x10

    /* the list, const (SOCOM 1's DWARF: HudTick's `CEntityList& list`, its walk const_iterators) */
    static const CEntityList &GetEntityList() { return m_list; }

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
float ftsGetTacMapZoom();
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
/* the north indicator's texture (SOCOM 1's g_pFov, gp-25048 there: InitMap loads it, the north
 * lines draw from it; 0x40BE70) */
zdb::CTexture *g_pFov;
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

/* the HUD's compass (hud_mapcompass.h's MapCompass: only its switch) */
class MapCompass {
public:
    void Off();
    void On();
};

/* the HUD (hud_main.cpp's CHUD): its owner, its font (0x498) and compass (0x4FE0), its own switch
 * to the compass map */
class CHUD {
public:
    static CZSealBody *GetHUDOwner();
    void SwitchToCompassMap();
    void SwitchToFullScreenMap();

    unsigned char _0[0x498];
    C2DFont *m_myfont;   // 0x498
    unsigned char _49c[0x4fe0 - 0x49c];
    MapCompass m_compass;   // 0x4fe0
};

extern CHUD *theHUD;

namespace {
/* the north indicator turned to the viewer's facing: the facing on the x/z plane normalised, the
 * front shaft from 10 to 30 ahead of (600, cy), the rear shaft from 10 to 30 behind, the barb from
 * 20 ahead and 10 aside to the front's end, and the "N" (the legend's first label) above. SOCOM 1's
 * SetupNorthIndicator (0x3C6A50 there: its centre 100, here a guarded static -- the layout's top
 * plus 40 -- its inverse length a double sqrt, here MW's `__rsqrt(1.0f, x)` intrinsic: the image's
 * only rsqrt.s, where `1.0f / sqrtf(x)` gives sqrt.s + div.s); the facing's z read through a
 * pointer as SOCOM 1's (sp+72 in a register). Byte-exact (round 52) with the three y's that read
 * cy as named locals ahead of the first Load -- the target computes every cy expression before
 * the first call and reloads the static only for the label; written in place, each Load reloads
 * it, and the y's of the front shaft's start may be named or not */
void SetupNorthIndicator(CZSealBody *pViewer)
{
    static float cy = 40.0f + g_fHudLayout60;
    SealDir dir;

    GetSealDir(pViewer, &dir);
    const float *z = &dir.z;
    float fInvLength = __rsqrt(1.0f, dir.x * dir.x + *z * *z);
    float fNormX = dir.x * fInvLength;
    float fNormZ = *z * fInvLength;
    float fFrontEndX = 600.0f - 30.0f * fNormX;
    float fFrontEndY = cy + 30.0f * fNormZ;
    float fRearStartY = cy - 10.0f * fNormZ;
    float fRearEndY = cy - 30.0f * fNormZ;
    float fBarbY = cy + 20.0f * fNormZ - 10.0f * fNormX;

    g_lineNorthIndFrontShaft.Load(600.0f - 10.0f * fNormX, cy + 10.0f * fNormZ, fFrontEndX, fFrontEndY,
                                  g_pFov);
    g_lineNorthIndFrontShaft.SetUV(0.5f, 0.9375f, 0.5f, 0.9375f);
    g_lineNorthIndRearShaft.Load(600.0f + 10.0f * fNormX, fRearStartY, 600.0f + 30.0f * fNormX,
                                 fRearEndY, g_pFov);
    g_lineNorthIndRearShaft.SetUV(0.5f, 0.9375f, 0.5f, 0.9375f);
    g_lineNorthIndBarb.Load(600.0f - 20.0f * fNormX - 10.0f * fNormZ, fBarbY,
                            fFrontEndX, fFrontEndY, g_pFov);
    g_lineNorthIndBarb.SetUV(0.5f, 0.9375f, 0.5f, 0.9375f);
    g_strLegend[0].Load(1.0f, "N", theHUD->m_myfont, 596.0f, 7.0f + cy, true);
}
}

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
    m_circleclipper = new zdb::CCircleClip;
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
 * The centre's average (round 58, open since round 29): the target sums z then x into temporaries
 * and multiplies x then z by 0.5 with the sum first -- the two sums assigned in two arms (round 53's
 * device: a value assigned in two branches is never folded back into its use; the original's two
 * arms unknown) and the 0.5 a named local (the world's round 26: a literal multiplier is moved
 * first, the same constant in a named local keeps source order). Either alone stays 2 to 7 off */
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
        float half = 0.5f;
        float oz, ox;

        if (obj) {
            oz = z + obj->m_pos.y;
            ox = x + obj->m_pos.x;
        } else {
            oz = z + obj->m_pos.y;
            ox = x + obj->m_pos.x;
        }
        x = ox * half;
        z = oz * half;
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

/* the map's sounds: the second slot plays one (zsnd.h's PlaySound(inst, flags)) */
class CSnd {
public:
    static CSnd *GetSoundByName(const char *name);
    virtual ~CSnd();
    virtual void *PlaySound(void *inst, unsigned int flags);
};

/* the texture's size as InitMap reads it (c2dbitmap.cpp's view: two halfwords at 0) */
namespace zdb {
class CTexture {
public:
    unsigned short m_width;   // 0x0
    unsigned short m_height;   // 0x2
};
}

/* the line map (hud_linemap.cpp's CZLineMap: what InitMap reaches) */
class CZLineMap : public CZMapItem {
public:
    CZLineMap();

    unsigned char _4[0x14be0 - 0x4];
    zdb::CTexture *m_tex;   // 0x14be0
    unsigned char _14be4[0x14bf0 - 0x14be4];
};

zdb::CTexture *GetTextureByName(zdb::CWorld *world, const char *name);
/* the legend's builder (0x20C080: its nine labels and pictures; unwritten) */
extern "C" void func_0020C080();

/* the map made ready (SOCOM 1's InitMap, 0x3C5D90 there: its compass bitmap and map background gone,
 * SOCOM II's three sounds): the sounds, the objectives, the line map as the first item, the items'
 * textures and their UVs (the claymore's and the bomb's alike), the scroll arrows placed by their
 * texture's size -- the two sideways ones turned, their size corrected from 480 lines to 448 -- and
 * the grid lines; the legend last. The names SOCOM 1's DWARF's */
void CZNewHudMap::InitMap(zdb::CWorld *world)
{
    static float fArrowTopY = g_fHudLayout60;
    static float fArrowBotY = g_fHudLayout365;

    m_socom2_81c = CSnd::GetSoundByName(".TCM_SELECT");
    m_socom2_820 = CSnd::GetSoundByName(".TCM_SLIDE");
    m_socom2_824 = CSnd::GetSoundByName(".TCM_SLIDE");
    m_objectivelist.Init();
    g_pFov = GetTextureByName(0, "FOV.tif");

    CZLineMap *linemap = new CZLineMap;

    linemap->m_tex = g_pFov;
    CZMapItem *item = linemap;

    m_itemlist.push_back(item);
    CZPlayerMapItem::m_conetex = GetTextureByName(0, "FOV.tif");
    CZPlayerMapItem::m_deadbliptex = CZPlayerMapItem::m_alivebliptex = GetTextureByName(0, "teammate_health.tif");
    CZClaymoreMapItem::m_claymoretex = GetTextureByName(0, "FOV.tif");
    CZClaymoreMapItem::m_U0 = 0.4375f;
    CZClaymoreMapItem::m_V0 = 0.875f;
    CZClaymoreMapItem::m_U1 = 0.53125f;
    CZClaymoreMapItem::m_V1 = 0.96875f;
    CZClaymoreMapItem::m_explosiontex = GetTextureByName(0, "compass_bomb_ring.tif");
    CZClaymoreMapItem::m_explosionU0 = 0.0f;
    CZClaymoreMapItem::m_explosionV0 = 0.0f;
    CZClaymoreMapItem::m_explosionU1 = 1.0f;
    CZClaymoreMapItem::m_explosionV1 = 1.0f;
    CZMPBombMapItem::m_mpbombtex = GetTextureByName(0, "FOV.tif");
    CZMPBombMapItem::m_U0 = 0.4375f;
    CZMPBombMapItem::m_V0 = 0.875f;
    CZMPBombMapItem::m_U1 = 0.53125f;
    CZMPBombMapItem::m_V1 = 0.96875f;
    CZMPBombMapItem::m_explosiontex = GetTextureByName(0, "compass_bomb_ring.tif");
    CZMPBombMapItem::m_explosionU0 = 0.0f;
    CZMPBombMapItem::m_explosionV0 = 0.0f;
    CZMPBombMapItem::m_explosionU1 = 1.0f;
    CZMPBombMapItem::m_explosionV1 = 1.0f;
    CZPlayerMapItem::InitStaticItems();

    zdb::CTexture *arrow = GetTextureByName(0, "hud_arrow_off2.tif");
    float fVertArrowHeight = arrow ? (float)(int)arrow->m_height : 0.0f;
    float fVertArrowWidth = arrow ? (float)(int)arrow->m_width : 0.0f;
    float fVertArrowLeftX = 0.5f * (860.0f - fVertArrowWidth);
    float fVertArrowRightX = fVertArrowLeftX + fVertArrowWidth;
    float fUpArrowBotY = fArrowTopY + fVertArrowHeight;

    g_bmpScrollUp.Load(fVertArrowLeftX, fArrowTopY, fVertArrowLeftX, fUpArrowBotY, fVertArrowRightX, fUpArrowBotY,
                       fVertArrowRightX, fArrowTopY, arrow);

    float fHorArrowWidth = 480.0f * fVertArrowHeight / 448.0f;
    float fHorArrowHeight = 448.0f * fVertArrowWidth / 480.0f;
    float fLeftArrowRightX = 230.0f + fHorArrowWidth;
    float fHorArrowTopY = 0.5f * (g_fHudLayout60 + g_fHudLayout365 - fHorArrowHeight);
    float fHorArrowBotY = fHorArrowTopY + fHorArrowHeight;

    g_bmpScrollLeft.Load(230.0f, fHorArrowBotY, fLeftArrowRightX, fHorArrowBotY, fLeftArrowRightX, fHorArrowTopY,
                         230.0f, fHorArrowTopY, arrow);

    float fRightArrowLeftX = 630.0f - fHorArrowWidth - 10.0f;

    g_bmpScrollRight.Load(630.0f, fHorArrowTopY, fRightArrowLeftX, fHorArrowTopY, fRightArrowLeftX, fHorArrowBotY,
                          630.0f, fHorArrowBotY, arrow);

    float fDownArrowTopY = fArrowBotY - fVertArrowHeight;

    g_bmpScrollDown.Load(fVertArrowRightX, fArrowBotY, fVertArrowRightX, fDownArrowTopY, fVertArrowLeftX,
                         fDownArrowTopY, fVertArrowLeftX, fArrowBotY, arrow);
    for (unsigned int iHor = 0; iHor < 7; iHor++) {
        g_arrHorGridLines[iHor].Load(0.0f, 0.0f, 0.0f, 0.0f, g_pFov);
        g_arrHorGridLines[iHor].SetUV(0.5f, 0.9375f, 0.5f, 0.9375f);
        g_arrHorGridLines[iHor].SetTrans(s_gridTrans);
    }
    for (unsigned int iVert = 0; iVert < 9; iVert++) {
        g_arrVertGridLines[iVert].Load(0.0f, 0.0f, 0.0f, 0.0f, g_pFov);
        g_arrVertGridLines[iVert].SetUV(0.5f, 0.9375f, 0.5f, 0.9375f);
        g_arrVertGridLines[iVert].SetTrans(s_gridTrans);
    }
    func_0020C080();
}

/* an item taken off the map if it is on it, given back for the caller to delete (HudTick's dead
 * items; no SOCOM 1 counterpart, the name ours). The walk's iterator is assigned from begin(), so
 * it stays in memory; remove() takes the item by reference, so the parameter is spilled */
#pragma auto_inline off   /* its one caller, HudTick, calls it out of line */
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
#pragma auto_inline reset

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

/* a SEAL's controller as the map reads it (zseal.h's CSealCtrl: IsPlayer its slot +0x2c, the menu
 * state's setter, the pad's index at 0x114, the input bit, 0x170 bit 11) */
class CSealCtrl {
public:
    void SetMenuState(MENU_STATE state, bool flag);
    void StopRumbler();
    MENU_STATE GetMenuState() const { return m_menustate; }
    virtual ~CSealCtrl();
    virtual bool PreTick(float dT);
    virtual bool Tick(float dT);
    virtual bool PostMortemPreTick(float dT);
    virtual void OnDeath();
    virtual void Slot1C();
    virtual void Slot20();
    virtual void PlayAiSound();
    virtual void AllowCommand();
    virtual bool IsPlayer() const;   /* +0x2c */

    unsigned char _4[0x114 - 0x4];
    unsigned int m_port;   // 0x114: its pad
    unsigned char _118[0x170 - 0x118];
    unsigned int _170_0 : 11;
    unsigned int m_input_enabled : 1;   // 0x171 bit 3
    unsigned int _170_12 : 20;
    unsigned char _174[0x221 - 0x174];
    MENU_STATE m_menustate;   // 0x221
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

/* the full-screen map's scroll permissions: up, left, right and down on the screen, from the centre
 * inside each bound of the scroll box and the viewer's facing -- in its eight compass sectors (0.01
 * either side of an axis counts as the axis) each screen direction is the one or two world
 * directions it maps to; facing nowhere, none (SOCOM 1's GetScrollPermissions, 0x3C50F0, the box
 * the first AI map's; its CScrollBools).
 * OPEN, 0x36C of 0x38C (27 structural, 23 registers): the logic, the bound tests and every case's
 * pairs are the target's (each `a || b` its two sltus), but MW scalarises the facing into $f20/$f21
 * (two saved FPRs, a 0x90 frame) where the target keeps it on the stack at sp+112 and reloads x
 * and z after the map lookup (frame 0x80, no FPR saved). A reference bound to a temporary (as
 * CalcNewMatrix's) keeps it in memory but holds the reference in s3 (19 off, the frame 0x90);
 * pointers to x and z, or a reference to the named facing, are further off. Round 61: the facing
 * returned by value from an inline keeps the inline's local in memory at the target's sp+112 with
 * the target's three stores, but copies it into the named local (0x3A4; a reference bound to it
 * 0x3A8, a CPnt3D built by its constructor 0x36C) */
CScrollBools CZNewHudMap::GetScrollPermissions() const
{
    CScrollBools bools;
    SealDir ptDir;

    GetSealDir(m_viewer, &ptDir);
    const CBBox *box = GetScrollBounds();
    bool bXUnderMax = m_centerx < box->m_max.x;
    bool bXOverMin = m_centerx > box->m_min.x;
    bool bZUnderMax = m_centerz < box->m_max.z;
    bool bZOverMin = m_centerz > box->m_min.z;

    if (ptDir.x < -0.01f) {
        if (ptDir.z < -0.01f) {
            bools._bUp = bXOverMin || bZOverMin;
            bools._bLeft = bZUnderMax || bXOverMin;
            bools._bRight = bZOverMin || bXUnderMax;
            bools._bDown = bXUnderMax || bZUnderMax;
        } else if (ptDir.z > 0.01f) {
            bools._bUp = bXOverMin || bZUnderMax;
            bools._bLeft = bZUnderMax || bXUnderMax;
            bools._bRight = bZOverMin || bXOverMin;
            bools._bDown = bXUnderMax || bZOverMin;
        } else {
            bools._bUp = bXOverMin;
            bools._bLeft = bZUnderMax;
            bools._bRight = bZOverMin;
            bools._bDown = bXUnderMax;
        }
    } else if (ptDir.x > 0.01f) {
        if (ptDir.z < -0.01f) {
            bools._bUp = bXUnderMax || bZOverMin;
            bools._bLeft = bZOverMin || bXOverMin;
            bools._bRight = bZUnderMax || bXUnderMax;
            bools._bDown = bXOverMin || bZUnderMax;
        } else if (ptDir.z > 0.01f) {
            bools._bUp = bXUnderMax || bZUnderMax;
            bools._bLeft = bZOverMin || bXUnderMax;
            bools._bRight = bZUnderMax || bXOverMin;
            bools._bDown = bXOverMin || bZOverMin;
        } else {
            bools._bUp = bXUnderMax;
            bools._bLeft = bZOverMin;
            bools._bRight = bZUnderMax;
            bools._bDown = bXOverMin;
        }
    } else if (ptDir.z < -0.01f) {
        bools._bUp = bZOverMin;
        bools._bLeft = bXOverMin;
        bools._bRight = bXUnderMax;
        bools._bDown = bZUnderMax;
    } else if (ptDir.z > 0.01f) {
        bools._bUp = bZUnderMax;
        bools._bLeft = bXUnderMax;
        bools._bRight = bXOverMin;
        bools._bDown = bZOverMin;
    }
    return bools;
}

/* whether a player's controller drives the seal (inline: a null test, then the slot) */
static inline bool IsPlayer(CZSealBody *seal)
{
    return seal->m_ctrl ? seal->m_ctrl->IsPlayer() : false;
}

namespace zdb {
/* the world (zdb's CWorld): the night mission's flag and its two lens colours */
class CWorld {
public:
    static CWorld *m_world;

    unsigned char _0[0x5dc];
    unsigned char m_night_mission;   // 0x5dc
    unsigned char _5dd[3];
    PNT4D m_lensfx_nvg;   // 0x5e0
    PNT4D m_lensfx_starlightscope;   // 0x5f0
};
}

/* the lens effects (thezoom.cpp's CameraZoom, SOCOM II's) */
class CameraZoom {
public:
    static void HoldLensFx(unsigned char hold);
    static void ReleaseLensFx(unsigned char hold);
    static void SetLensFx(unsigned char fx);
    static unsigned char LensFxOn();
};

/* the camera's view, its zoom at 0x470 (SOCOM 1's at appCamera+0x34, 0x444) */
struct CAppCameraView {
    unsigned char _0[0x470];
    float m_zoom;   // 0x470
};

/* the camera (camera.h's CAppCamera: its zoom, its view) */
class CAppCamera {
public:
    void SetZoom(float zoom);

    unsigned char _0[0xb4];
    CAppCameraView *m_view;   // 0xb4
};

extern CAppCamera *appCamera;

/* the camera's zoom the full-screen map put aside (OVL1 .data 0x3DC478, 1.0; the name ours) */
static float s_mapCamZoom = 1.0f;

/* the popups' trigger (hud_scorepopup.cpp's: an objective camera's popup shown) */
class CTriggerSource;
void hudTriggerPopup(CTriggerSource *source);

/* SOCOM II's (OVL1 0x2B4380, no row named: the compass map passes 0, the full-screen map 1) */
extern "C" void func_002B4380(int on);

/* zinput.cpp's (OVL1 0x2C5D10, no row named: a pad and a game function's button, the full-screen map
 * passes its close button 12 and 1) */
extern "C" void func_002C5D10(CPad *pad, int func, int state);

/* the full-screen map's animation time (OVL1 .data 0x3DC488, -1 until opened; the full-screen
 * animations' tick 0x209350 runs it; the name ours) */
static float s_fullMapAnimTime = -1.0f;

/* to full screen (SOCOM 1's SwitchMapToFullScreen, 0x3C4E20 there): full screen; the rumbler
 * stopped, the pad's close button set, the menu state put aside for the full-screen map's; for
 * the player, the lens colour taken off unless a lens effect is up, the camera's zoom put aside
 * and reset, the lens effects held, the heartbeat sensor off; the compass off; the map placed
 * (232, 62)-(628, 365) at the tactical zoom's window, centred on the viewer, its clipper four
 * half-widths round; the objectives refreshed, the north indicator set, the HUD switched, the
 * open sound, the animation restarted and the grid lines emptied. Byte-exact (round 52): the
 * viewer's position through SOCOM 1's `CPnt3D *center` (read twice through m_viewer, the store
 * between reloads it); the pad chosen as SwitchToCompassMap's */
void CZNewHudMap::SwitchMapToFullScreen()
{
    m_isFullScreen = true;

    CZSealBody *seal = CHUD::GetHUDOwner();
    CSealCtrl *ctrl = seal->m_ctrl;
    CPad *pad;

    if (ctrl->m_port < CInput::NUM_PADS)
        pad = CInput::m_pads[ctrl->m_port];
    else
        pad = 0;
    ctrl->StopRumbler();
    func_002C5D10(pad, 12, 1);
    m_oldmenustate = ctrl->GetMenuState();
    ctrl->SetMenuState(stateFullScreenMapMenu, false);
    if (IsPlayer(seal)) {
        if (!CameraZoom::LensFxOn())
            seal->m_kit.Apply_LensEffects(false, 0);
        if (s_mapCamZoom == 1.0f) {
            s_mapCamZoom = appCamera->m_view->m_zoom;
            appCamera->SetZoom(1.0f);
        }
        CameraZoom::HoldLensFx(4);

        CZKit *kit = &seal->m_kit;

        if (kit)
            kit->m_heartbeat_enabled = false;
    }
    m_parent->m_compass.Off();
    SetMapScreenPos(232.0f, 62.0f, 628.0f, 365.0f);
    SetWorldWidth(2200.0f / ftsGetTacMapZoom());
    m_scale = 1.0f;
    const CPnt3D *center = &m_viewer->m_posW;

    m_centerx = center->x;
    m_centerz = center->z;
    m_circleclipper->Set(m_screencenterx + C2D::m_topx, m_screencentery + C2D::m_topy,
                         4.0f * (m_screencenterx - m_screenx1));
    m_objectivelist.RefreshList();
    SetupNorthIndicator(m_viewer);
    theHUD->SwitchToFullScreenMap();
    if (m_socom2_81c)
        m_socom2_81c->PlaySound(0, 255);
    s_fullMapAnimTime = 0.0f;
    for (unsigned int i = 0; i < 7; i++)
        g_arrHorGridLines[i].SetPos(0, 0, 0, 0);
    for (unsigned int i = 0; i < 9; i++)
        g_arrVertGridLines[i].SetPos(0, 0, 0, 0);
    func_002B4380(1);
}

/* back to the compass (SOCOM 1's SwitchToCompassMap, 0x3C4C10 there): not full screen; the menu
 * state given back; with no lens effect up on a night mission the player's night-vision or scope
 * colour put back on its kit; the camera's zoom given back, the lens effects released, the
 * heartbeat sensor on; the map placed in the corner (486, 2)-(614, 130) at a 1200 window, scale 1;
 * the compass on, the objective list freed, the pad's triangle held down, the HUD switched, and
 * the held objective's popup shown; SetMapScreenPos and SetWorldWidth inlined, their constants
 * folded. Byte-exact (round 52, found with tools/igreplay.py --want): the zoom state a dead
 * definition after the owner (numbering it ahead of the pad) and the pad chosen by an if/else into
 * an uninitialised local declared next -- GetPad's inline or a ternary leaves the pad an inline
 * temporary numbered after every named local (s0 for the target's s2) -- then the world, the zoom
 * and the kit in that order (the target loads the world first) */
void CZNewHudMap::SwitchToCompassMap()
{
    m_isFullScreen = false;

    CZSealBody *seal = CHUD::GetHUDOwner();
    ZOOMSTATE zoomstate = ZOOMSTATE_UNKNOWN;
    CPad *pad;

    if (seal->m_ctrl->m_port < CInput::NUM_PADS)
        pad = CInput::m_pads[seal->m_ctrl->m_port];
    else
        pad = 0;

    seal->m_ctrl->SetMenuState(m_oldmenustate, false);
    m_oldmenustate = stateNone;

    zdb::CWorld *world = zdb::CWorld::m_world;

    zoomstate = seal->m_zoomstate;

    CZKit *kit = &seal->m_kit;

    if (!CameraZoom::LensFxOn() && world->m_night_mission && IsPlayer(seal)) {
        if (zoomstate == ZOOMSTATE_NVG) {
            if (!CameraZoom::LensFxOn()) {
                PNT4D color = world->m_lensfx_nvg;

                kit->Apply_LensEffects(true, (CPnt3D *)&color);
            }
            CameraZoom::SetLensFx(3);
        } else if (zoomstate >= ZOOMSTATE_BINOCS) {
            if (!CameraZoom::LensFxOn()) {
                PNT4D color = world->m_lensfx_starlightscope;

                kit->Apply_LensEffects(true, (CPnt3D *)&color);
            }
            CameraZoom::SetLensFx(4);
        }
    }
    if (s_mapCamZoom != 1.0f) {
        appCamera->SetZoom(s_mapCamZoom);
        s_mapCamZoom = 1.0f;
    }
    CameraZoom::ReleaseLensFx(4);
    if (kit)
        kit->m_heartbeat_enabled = true;
    SetMapScreenPos(486.0f, 2.0f, 614.0f, 130.0f);
    SetWorldWidth(1200.0f);
    m_scale = 1.0f;
    m_parent->m_compass.On();
    m_objectivelist.FreeList(false);
    if (pad)
        pad->SetButtonState(PAD_TRIANGLE, KEY_DOWN);
    theHUD->SwitchToCompassMap();

    CMissionObjectiveCam *retained = m_objectivelist.GetCurrentObjectiveRetainer();

    if (retained)
        hudTriggerPopup((CTriggerSource *)retained);
    func_002B4380(0);
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

/* the player (zfts's) and the network's flags (hud_mpinfobox.cpp's view) */
CZSealBody *ftsGetPlayer();

class CZNetwork {
public:
    unsigned char m_bSessionMaster;   // 0x0
    unsigned char m_bNetwork;   // 0x1
};

extern CZNetwork theNetwork;

static inline CEntity *AsSeal(CEntity *e)
{
    return e && e->m_type == 2 ? e : 0;
}

static inline void ListPush(std::list<CZMapItem *> &list, CZMapItem *const &x)
{
    list.push_back(x);
}

inline void CZNewHudMap::AddItem(CZMapItem *item)
{
    ListPush(m_itemlist, item);
}

/* a seal's controller, and whether its input is on (ours: the two ternaries, inline) */
static inline CSealCtrl *CtrlOf(CZSealBody *seal)
{
    return seal ? seal->m_ctrl : 0;
}

static inline bool InputEnabled(CZSealBody *seal)
{
    CSealCtrl *ctrl = CtrlOf(seal);

    return ctrl ? (bool)ctrl->m_input_enabled != false : false;
}

/* the compass map's pad: the game's button 12, offline, with the owner's input on, no menu up and the
 * owner the player and shown, opens the map full screen (SOCOM 1's TickCompassMap, 0x3C48B0 there,
 * inline in SOCOM II: there the same button and a text-entry test) */
inline void CZNewHudMap::TickCompassMap(CPad *pad, MENU_STATE menustate)
{
    CZSealBody *seal = CHUD::GetHUDOwner();
    bool input = InputEnabled(seal);

    if (pad && func_002C64E0(12, pad) == 1 && !theNetwork.m_bNetwork && input) {
        CZSealBody *player = ftsGetPlayer();

        if (menustate == stateNone && seal == player && seal->m_socom2_e1_4)
            SwitchMapToFullScreen();
    }
}

/* the map's tick (SOCOM 1's HudTick, 0x3C3FC0 there): the compass map's pad, or the full-screen map's
 * under its two menu states; in full screen the objectives ticked, the animations, the matrix, each
 * item ticked and those that say so taken off and deleted, a new item for each player whose view
 * cone is new. Measured for it: the second walk's iterator is the first's local reused
 * (IRO_SplitLifetimes gives it its low frame slot, 148, after the destructor's temporaries -- SOCOM 1's
 * DWARF names one `iter` for both walks); the `if` form of AsSeal flips the whole function (README "if
 * versus ternary is function-wide"); a `for` lowers to init, body, increment, test, which reads the
 * frame: the temporaries of the third push_back sit between the walk's begin and its end's, so its
 * expansion is a pass deeper than the first's -- SOCOM 1's two-level push_back (include/list's vc,
 * not applied: ListPush models the level here, AddItem the third's by-value new).
 * OPEN, 11 instructions, all s1 against s2 (0x3F8 of 0x3F8, frame exact): the target numbers the
 * scalarised member holding &m_list + 4 (const_iterator(base::end()), IRO_ScalarizeClassDataMembers
 * in pass 1, @2417) before the common subexpression (&removelist + 4) + 4 (pass 0's IRO_CommonSubs,
 * the second walk's begin and the destructor's, @2415): `igreplay --move 54=55.5` or `--move 55=50.5`
 * gives the target's every register. So the original makes the (&removelist + 4) + 4 pair in pass 1
 * -- likely the real MSL's destructor reading its begin through the compressed pair's tail(), a
 * reference temporary pass 0 cannot forward ("Level 6, or 8?") -- or hoists &m_list + 4 in pass 0.
 * Tried: the list a local subclass; a reference or pointer local for the second walk (both reads,
 * either read); an iterator copy; five spellings of the const end(); &end_ direct or a named base
 * iterator (then &m_list + 4 is rematerialised at each use, 0x3EC) */
void CZNewHudMap::HudTick(CPad *pad, float dT)
{
    MENU_STATE menustate = CHUD::GetHUDOwner()->m_ctrl->GetMenuState();

    if (!m_isFullScreen)
        TickCompassMap(pad, menustate);
    else if (menustate == stateFullScreenMapMenu || menustate == stateFullScreenMapAnim)
        TickFullScreenMap(pad, menustate);
    if (!m_isFullScreen)
        return;
    m_objectivelist.Tick(dT);
    TickFullScreenAnims(dT);
    CalcNewMatrix();

    std::list<CZMapItem *> removelist;
    std::list<CZMapItem *>::iterator it;

    for (it = m_itemlist.begin(); it != m_itemlist.end(); ++it) {
        CZMapItem *item = *it;
        bool deleteitem = item->Tick(this, dT);

        if (deleteitem)
            ListPush(removelist, item);
    }

    for (it = removelist.begin(); it != removelist.end(); ++it) {
        CZMapItem *item = *it;

        RemoveItem(item);
        delete item;
    }
    CEntityList::const_iterator e;

    const CEntityList &list = CEntity::m_list;

    for (e = list.begin(); e != list.end(); ++e) {
        CEntity *entity = AsSeal(*e);

        if (entity && ((CZSealBody *)entity)->HudUpdateViewCone() &&
            (((CZSealBody *)entity)->m_ctrl ? ((CZSealBody *)entity)->m_ctrl->IsPlayer() : false))
            AddItem(new CZPlayerMapItem(entity));
    }
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

/* the scroll arrows' pulse: its speed and its direction, reversed at each end (OVL1 .data 0x3DC4A0,
 * 0x3DC4A8; the names ours) */
static float s_fArrowPulseSpeed = 1.5f;
static float s_fArrowPulseDir = 1.0f;

/* the full-screen map's animations (SOCOM 1's TickFullScreenAnims, 0x3C3790 there; SOCOM II's own
 * pulse and line timing): the arrows pulse 0-128 where the map can scroll, 32 where it cannot; from
 * the full-screen switch on (the clock at 0) the grid lines grow in, a tenth of a second apart --
 * the horizontal seven across from x 230, the vertical nine down from the map's top (the first not
 * yet growing ends their walk) -- and once the last is grown the clock stops (-1). The line timing
 * guarded statics, after a plain one (the nine lines) */
void CZNewHudMap::TickFullScreenAnims(float dT) const
{
    static float fVertLines = 9.0f;
    static float fLastLineDelay = 0.1f * (fVertLines - 1.0f);
    static float fAnimLength = 1.0f + fLastLineDelay;
    static float fArrowPulseTime;

    fArrowPulseTime += s_fArrowPulseDir * (dT * s_fArrowPulseSpeed);
    if (!(fArrowPulseTime <= 1.0f)) {
        fArrowPulseTime = 1.0f;
        s_fArrowPulseDir *= -1.0f;
    } else if (fArrowPulseTime < 0.0f) {
        fArrowPulseTime = 0.0f;
        s_fArrowPulseDir *= -1.0f;
    }

    float fTrans = 128.0f * fArrowPulseTime;
    CScrollBools bools = GetScrollPermissions();

    g_bmpScrollUp.SetTrans(bools._bUp ? fTrans : 32.0f);
    g_bmpScrollLeft.SetTrans(bools._bLeft ? fTrans : 32.0f);
    g_bmpScrollRight.SetTrans(bools._bRight ? fTrans : 32.0f);
    g_bmpScrollDown.SetTrans(bools._bDown ? fTrans : 32.0f);

    if (!(s_fullMapAnimTime < 0.0f)) {
        s_fullMapAnimTime += dT;
        for (int iHash = 0; iHash < sizeof(g_arrHorGridLines) / sizeof(g_arrHorGridLines[0]); iHash++) {
            float fStart = 0.1f * (float)iHash;
            float f;

            if (s_fullMapAnimTime <= fStart)
                f = 0.0f;
            else {
                f = s_fullMapAnimTime - fStart;
                f = f < 1.0f ? f : 1.0f;
            }
            if (f) {
                int y = (int)(g_fHudLayout60 + s_fullMapStep * (float)(iHash + 1));

                g_arrHorGridLines[iHash].SetPos(230, y, (int)(230.0f + 400.0f * f), y);
            }
        }
        for (int iHor = 0; iHor < sizeof(g_arrVertGridLines) / sizeof(g_arrVertGridLines[0]); iHor++) {
            float fStart = 0.1f * (float)iHor;
            float f;

            if (s_fullMapAnimTime <= fStart)
                f = 0.0f;
            else {
                f = s_fullMapAnimTime - fStart;
                f = f < 1.0f ? f : 1.0f;
            }
            if (!f)
                break;
            int x = (int)(230.0f + s_fullMapStepX * (float)(iHor + 1));

            g_arrVertGridLines[iHor].SetPos(x, (int)g_fHudLayout60, x,
                                            (int)(g_fHudLayout60 + s_fullMapHeight * f));
        }
        if (!(s_fullMapAnimTime < fAnimLength))
            s_fullMapAnimTime = -1.0f;
    }
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
