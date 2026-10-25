/* hud_mapcompass.cpp -- the HUD's compass (OVL1 0x211510-...): SOCOM 1's unit (its DWARF names it,
 * Z:\dev\Apps\FTS\hud_mapcompass.cpp), its small functions in SOCOM 1's order after Tick. SOCOM
 * II's Draw is empty, SetPos scales the rectangle by ztwod's screen scale, and On shows the
 * compass only for the local seal while it lives. Not written yet: Tick, the destructor (it frees
 * a static vector of nav packages), the constructor and InitHUD_Compass. Defined in reverse of
 * their image order (-inline deferred). */

#include <vector>

#include "hud_mapcompass.h"
#include "../../gamez/znode/znode.h"
#include "../../gamez/zsys/zsys.h"

class CHUD {
public:
    static CZSealBody *GetHUDOwner();
    void Add2D(C2D *p2d, int phase, bool add_to_back);
};

extern CHUD *theHUD;

namespace zdb {
class CAssetList {
public:
    CTexHandle *GetTexHandle(const char *name);
};

class CAssetMgr {
public:
    static CAssetList m_assets;
};
}

/* only what On reads (names ours) */
class CZSealBody {
public:
    unsigned char _0[0xe1];
    unsigned char _e1_0 : 4;
    unsigned char m_socom2_e1_4 : 1;   // 0xe1 bit 4: tested before the compass shows
    unsigned char _e1_5 : 3;
    unsigned char _e2[0x200 - 0xe2];
    signed char m_socom2_200;   // 0x200: under 2 while the seal lives
    bool IsDead() { return m_socom2_200 >= 2; }
};

/* ztwod's screen scale, x then y (no name yet) */
extern float data_0049E9F0[2];

/* the seal the camera follows (no name yet) */
extern "C" CZSealBody *func_002B3580();

/* the frame's anchors, nearest first (SOCOM 1's global; the unit's initialiser builds it) */
std::vector<nav_package> navpoint_vec;

/* an anchor's icon textured from a handle (none without one), sized to the texture (one line in
 * SOCOM 1's InitHUD_Compass: inline) */
static inline void SetAnchorTexture(CHudCompassAnchor &anchor, zdb::CTexHandle *htex)
{
    anchor.m_texture = htex ? htex->m_texture : 0;
    if (anchor.m_texture) {
        anchor.m_width = (int)anchor.m_texture->m_width;
        anchor.m_height = (int)anchor.m_texture->m_height;
    }
}

/* the anchors' textures and colours, then every anchor and arrow loaded with the first texture
 * and listed under the anchor head, the head under the strip, the strip in the HUD's phases */
void MapCompass::InitHUD_Compass(zdb::CWorld *world)
{
    m_anchor_img[0] = zdb::CAssetMgr::m_assets.GetTexHandle("ret_triangle.tif");
    m_anchor_img[1] = zdb::CAssetMgr::m_assets.GetTexHandle("ret_bomb.tif");
    m_anchor_img[2] = zdb::CAssetMgr::m_assets.GetTexHandle("ret_base.tif");
    m_anchor_img[3] = zdb::CAssetMgr::m_assets.GetTexHandle("ret_base.tif");
    m_anchor_img[4] = zdb::CAssetMgr::m_assets.GetTexHandle("ret_hostage.tif");
    m_anchor_img[5] = zdb::CAssetMgr::m_assets.GetTexHandle("ret_friendly.tif");
    m_anchor_img[6] = zdb::CAssetMgr::m_assets.GetTexHandle("ret_extraction.tif");
    m_anchor_img[7] = zdb::CAssetMgr::m_assets.GetTexHandle("ret_objective.tif");
    m_anchor_img[8] = zdb::CAssetMgr::m_assets.GetTexHandle("ret_objective.tif");
    m_anchor_img[9] = zdb::CAssetMgr::m_assets.GetTexHandle("hud_talk.tif");
    m_anchor_img[10] = zdb::CAssetMgr::m_assets.GetTexHandle("small_compass.tif");
    m_anchor_img[12] = zdb::CAssetMgr::m_assets.GetTexHandle("ret_able.tif");
    m_anchor_img[13] = zdb::CAssetMgr::m_assets.GetTexHandle("ret_bravo.tif");
    m_anchor_img[11] = zdb::CAssetMgr::m_assets.GetTexHandle("small_compass.tif");
    if (!m_anchor_img[10])
        m_anchor_img[10] = m_anchor_img[1];
    m_anchor_img[14] = zdb::CAssetMgr::m_assets.GetTexHandle("ret_nav_01.tif");
    m_anchor_img[15] = zdb::CAssetMgr::m_assets.GetTexHandle("ret_nav_02.tif");
    m_anchor_img[16] = zdb::CAssetMgr::m_assets.GetTexHandle("ret_nav_03.tif");
    m_anchor_img[17] = zdb::CAssetMgr::m_assets.GetTexHandle("ret_nav_04.tif");
    m_anchor_img[18] = zdb::CAssetMgr::m_assets.GetTexHandle("ret_nav_05.tif");
    m_anchor_img[19] = zdb::CAssetMgr::m_assets.GetTexHandle("ret_nav_06.tif");
    m_anchor_img[20] = zdb::CAssetMgr::m_assets.GetTexHandle("ret_nav_07.tif");
    m_anchor_img[21] = zdb::CAssetMgr::m_assets.GetTexHandle("ret_nav_08.tif");
    m_anchor_img[22] = zdb::CAssetMgr::m_assets.GetTexHandle("ret_nav_09.tif");
    m_anchor_img[23] = zdb::CAssetMgr::m_assets.GetTexHandle("ret_nav_10.tif");
    m_anchor_img[24] = zdb::CAssetMgr::m_assets.GetTexHandle("ret_nav_11.tif");
    m_anchor_img[25] = zdb::CAssetMgr::m_assets.GetTexHandle("ret_nav_12.tif");
    m_anchor_img[26] = zdb::CAssetMgr::m_assets.GetTexHandle("ret_nav_13.tif");
    m_anchor_img[27] = zdb::CAssetMgr::m_assets.GetTexHandle("ret_nav_14.tif");
    m_anchor_img[28] = zdb::CAssetMgr::m_assets.GetTexHandle("ret_nav_15.tif");
    m_anchor_img[29] = zdb::CAssetMgr::m_assets.GetTexHandle("ret_nav_16.tif");
    m_anchor_img[30] = zdb::CAssetMgr::m_assets.GetTexHandle("ret_nav_17.tif");
    m_anchor_img[31] = zdb::CAssetMgr::m_assets.GetTexHandle("ret_nav_18.tif");
    m_anchor_img[32] = zdb::CAssetMgr::m_assets.GetTexHandle("ret_nav_19.tif");
    m_anchor_img[33] = zdb::CAssetMgr::m_assets.GetTexHandle("ret_nav_20.tif");
    m_anchor_img[34] = zdb::CAssetMgr::m_assets.GetTexHandle("ret_nav_21.tif");
    m_anchor_img[35] = zdb::CAssetMgr::m_assets.GetTexHandle("ret_nav_22.tif");
    m_anchor_img[36] = zdb::CAssetMgr::m_assets.GetTexHandle("ret_nav_23.tif");
    m_anchor_img[37] = zdb::CAssetMgr::m_assets.GetTexHandle("ret_nav_24.tif");

    m_anchor_clr[0] = CPnt3D(86.0f, 63.0f, 27.0f);
    m_anchor_clr[1] = CPnt3D(62.0f, 62.0f, 32.0f);
    m_anchor_clr[3] = CPnt3D(62.0f, 37.0f, 36.0f);
    m_anchor_clr[2] = CPnt3D(40.0f, 62.0f, 36.0f);
    m_anchor_clr[4] = CPnt3D(62.0f, 49.0f, 36.0f);
    m_anchor_clr[5] = CPnt3D(32.0f, 49.0f, 128.0f);
    m_anchor_clr[6] = CPnt3D(36.0f, 40.0f, 62.0f);
    m_anchor_clr[7] = CPnt3D(76.0f, 70.0f, 32.0f);
    m_anchor_clr[8] = CPnt3D(70.0f, 76.0f, 76.0f);
    m_anchor_clr[9] = CPnt3D(32.0f, 62.0f, 62.0f);
    m_anchor_clr[10] = CPnt3D(64.0f, 12.0f, 12.0f);
    m_anchor_clr[12] = CPnt3D(32.0f, 62.0f, 62.0f);
    m_anchor_clr[13] = CPnt3D(32.0f, 62.0f, 62.0f);
    m_anchor_clr[11] = CPnt3D(100.0f, 64.0f, 32.0f);
    for (unsigned int jj = 14; jj < 38; jj++)
        m_anchor_clr[jj] = CPnt3D(32.0f, 62.0f, 32.0f);

    for (unsigned int ii = 0; ii < 38; ii++) {
        SetAnchorTexture(m_compass_anchors[ii], m_anchor_img[0]);
        SetAnchorTexture(m_compass_arrows[ii], m_anchor_img[0]);
        m_compass_anchors[ii].m_icon.SetUV(-0.01f, -0.01f, 1.02f, 1.02f);
        m_compass_arrows[ii].m_icon.SetUV(-0.01f, -0.01f, 1.02f, 1.02f);

        zdb::CTexture *tex = m_compass_anchors[ii].m_texture;

        if (tex) {
            m_compass_anchors[ii].m_icon.Load(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, tex);
            m_anchor_head.push_back(&m_compass_anchors[ii].m_icon);
            m_compass_arrows[ii].m_icon.Load(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, tex);
            m_compass_arrows[ii].m_icon.SetColor(m_anchor_clr[0].x, m_anchor_clr[0].y,
                                                 m_anchor_clr[0].z);
            m_anchor_head.push_back(&m_compass_arrows[ii].m_icon);
        }
    }
    m_poly.push_back(&m_anchor_head);
    theHUD->Add2D(&m_poly, -1, true);
}

/* the anchor tables built 38 to a table, everything placed at zero, room for 48 anchors */
MapCompass::MapCompass()
{
    m_newcompasstex = 0;
    m_compasstopx = 0.0f;
    m_compasstopy = 0.0f;
    m_compassbotx = 0.0f;
    m_compassboty = 0.0f;
    m_compasscenterx = 0.0f;
    m_compasscentery = 0.0f;
    m_scale = 0.0f;
    m_tex_width = 0.0f;
    m_tex_height = 0.0f;
    navpoint_vec.reserve(48);
}

MapCompass::~MapCompass()
{
    FreeVector(navpoint_vec);
}

void MapCompass::SetScale(float scale)
{
    m_scale = scale;
}

/* shown with no owner, or for the local seal while it lives and is marked; hidden otherwise (both
 * tests as bools first: the target materialises each before it branches) */
void MapCompass::On()
{
    CZSealBody *owner = CHUD::GetHUDOwner();

    if (owner) {
        bool local = owner == func_002B3580();
        bool dead = owner->IsDead();

        if (!owner->m_socom2_e1_4 || dead || !local) {
            m_poly.Off();
            m_anchor_head.Off();
            return;
        }
    }
    m_poly.On();
    m_anchor_head.On();
}

/* the strip and the anchors (the list head's children) hidden */
void MapCompass::Off()
{
    m_poly.Off();
    m_anchor_head.Off();
}

/* the strip placed (scaled to the screen), its middle kept, and the bitmap loaded around the
 * middle at the compass's scale */
void MapCompass::SetPos(float topx, float topy, float botx, float boty)
{
    m_compasstopx = topx * data_0049E9F0[0];
    m_compasstopy = topy * data_0049E9F0[1];
    m_compassbotx = botx * data_0049E9F0[0];
    m_compassboty = boty * data_0049E9F0[1];
    m_compasscenterx = (m_compasstopx + m_compassbotx) / 2.0f;
    m_compasscentery = (m_compasstopy + m_compassboty) / 2.0f;
    float top = m_compasscentery + m_scale * (m_compasstopy - m_compasscentery);
    float bot = m_compasscentery + m_scale * (m_compassboty - m_compasscentery);

    m_poly.Load(m_compasstopx, top, m_compasstopx, bot, m_compassbotx, bot, m_compassbotx, top,
                m_newcompasstex);
}

void MapCompass::SetTexture(zdb::CTexture *tex)
{
    m_newcompasstex = tex;
    m_tex_width = tex ? (int)tex->m_width : 0.0f;   /* through int: MW converts unsigned the long way */
    m_tex_height = tex ? (int)tex->m_height : 0.0f;
}

void MapCompass::SetTrans(float trans)
{
    m_poly.SetTrans(trans);
}

void MapCompass::Draw(zdb::CCamera *camera)
{
}
