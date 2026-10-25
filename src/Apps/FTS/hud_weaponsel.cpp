/* hud_weaponsel.cpp -- the single-player weapon selector (CWeaponSel): SOCOM 1's unit (its DWARF
 * names it, Z:\dev\Apps\FTS\hud_weaponsel.cpp), its functions in SOCOM 1's order (OVL1
 * 0x219330-0x21DE4F: PlaceWeaponText ... OnClose, the state handlers, OnOpen, UnInit, Init, the
 * constructor). Written: AfterClose (0x21AAA0), AfterCloseFirst (0x21B0A0), OnClose (0x21B3E0)
 * and UnInit (0x21CBA0), which inlines OnClose as SOCOM 1's called it. The layout is hud_weaponsel.h's.
 * Defined in reverse of their image order (-inline deferred). */

#include "../../gamez/ztwod/ztwod.h"
#include "../../gamez/zinput/zinput.h"
#include "hud_weaponsel.h"

/* the seal's kit: the reticule switch (SOCOM 1's SetCurReticule, a flag added) */
class CZKit {
public:
    void SetCurReticule(bool flag);
};

/* the seal's controller as the selector reads it: the pad it reads (SOCOM 1's at 0xE8) */
class CSealCtrl {
public:
    unsigned char _0[0x114];
    unsigned int m_padindex;   // 0x114
};

/* SOCOM 1's CSealCtrl::SetMenuState with a flag added (OVL2; the row unnamed: called as C) */
extern "C" void func_00597740(CSealCtrl *ctrl, int state, int flag);

/* the seal as the selector reads it: its controller and kit */
class CZSealBody {
public:
    unsigned char _0[0xc0];
    CSealCtrl *m_ctrl;   // 0xc0
    unsigned char _c4[0x5e0 - 0xc4];
    CZKit m_kit;   // 0x5e0
};

/* a pad by its index, none past the last (SOCOM 1's inline over CInput's table) */
static inline CPad *GetPad(unsigned int n)
{
    return n < CInput::NUM_PADS ? CInput::m_pads[n] : 0;
}

/* the HUD as the selector reads it: the reticule flag, the owner */
class CHUD {
public:
    static CZSealBody *GetHUDOwner();
    static PNT4D m_hud_textcolor;

    unsigned char _0[0xb1];
    bool m_usehudret;   // 0xb1
};

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

/* a texture of the world's by name (hud_main.cpp's) */
zdb::CTexture *GetTextureByName(zdb::CWorld *world, const char *name);

class CSnd {
public:
    static CSnd *GetSoundByName(const char *name);
};

/* the corner boxes' and the off arrow's textures (hud_main.cpp's data; SOCOM 1's
 * c_szCornerBoxesTx) */
extern const char *data_003E3048;
extern const char *data_003E3068;

/* the selector's width, and its rows' tops (the unit's statics: its initialiser sets them from
 * the HUD's layout) */
extern int data_0040C108;
extern float data_003E4A90;
extern float data_003E4A98;
extern float data_003E4AA0;

/* the textures fetched, the backdrops and backings loaded, the texts coloured and loaded off, the
 * pieces listed (SOCOM 1's, the backings three members now) */
void CWeaponSel::Init(zdb::CWorld *world, C2DFont *font)
{
    data_0040C108 = 160;
    m_openstate = 0;
    m_weapontex[0] = zdb::CAssetMgr::m_assets.GetTexHandle("reticuletex.tif");
    m_weapontex[1] = zdb::CAssetMgr::m_assets.GetTexHandle("reticuletex.tif");
    m_weapontex[2] = zdb::CAssetMgr::m_assets.GetTexHandle("reticuletex.tif");
    m_weapontex[3] = zdb::CAssetMgr::m_assets.GetTexHandle("reticuletex.tif");
    m_weaponselbkrndtex = zdb::CAssetMgr::m_assets.GetTexHandle(data_003E3048);
    m_arrowtex = GetTextureByName(0, data_003E3068);
    m_myfont = font;
    m_selectionback2.SetHasTexture(false);
    m_selectionback2.Load(0.0f, data_003E4A98, data_0040C108, 2.0f + (64.0f + data_003E4A98), 0);
    m_selectionback2.SetColor(80.0f, 80.0f, 80.0f);
    m_selectionback2.SetIsTrans(true);
    m_selectionback2.SetTrans(0.0f);
    m_selectionback2.m_fade_dx = 2.0f;
    m_selectionback2.Off();
    push_back(&m_weaponselbkrnd);
    push_back(&m_selectionback2);
    push_back(&m_weaponicon[0]);
    push_back(&m_weaponicon[3]);
    push_back(&m_weaponicon[2]);
    push_back(&m_weaponicon[1]);
    m_arrow[0].Off();
    m_arrow[1].Off();
    m_arrow[0].SetUV(0.0f, 0.05f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.05f);
    m_arrow[0].SetTrans(70.0f);
    m_arrow[1].SetTrans(70.0f);
    m_arrow[0].SetColor(50.0f, 50.0f, 50.0f);
    m_arrow[1].SetColor(50.0f, 50.0f, 50.0f);
    m_arrowcolor = 50.0f;
    m_arrowincrement = 1;
    push_back(&m_arrow[0]);
    push_back(&m_arrow[1]);
    m_hotswapL1text.m_color[0] = 55.0f;
    m_hotswapL1text.m_color[1] = 140.0f;
    m_hotswapL1text.m_color[2] = 130.0f;
    m_hotswapL1text.m_color[3] = 80.0f;
    push_back(&m_hotswapL1text);
    m_hotswapL2text.m_color[0] = 55.0f;
    m_hotswapL2text.m_color[1] = 140.0f;
    m_hotswapL2text.m_color[2] = 130.0f;
    m_hotswapL2text.m_color[3] = 80.0f;
    push_back(&m_hotswapL2text);
    m_weaponname.m_color[0] = CHUD::m_hud_textcolor.x;
    m_weaponname.m_color[1] = CHUD::m_hud_textcolor.y;
    m_weaponname.m_color[2] = CHUD::m_hud_textcolor.z;
    m_weaponname.m_color[3] = CHUD::m_hud_textcolor.w;
    push_back(&m_weaponname);
    m_weapondescription.m_color[0] = CHUD::m_hud_textcolor.x;
    m_weapondescription.m_color[1] = CHUD::m_hud_textcolor.y;
    m_weapondescription.m_color[2] = CHUD::m_hud_textcolor.z;
    m_weapondescription.m_color[3] = CHUD::m_hud_textcolor.w;
    push_back(&m_weapondescription);
    m_ammotype.m_color[0] = CHUD::m_hud_textcolor.x;
    m_ammotype.m_color[1] = CHUD::m_hud_textcolor.y;
    m_ammotype.m_color[2] = CHUD::m_hud_textcolor.z;
    m_ammotype.m_color[3] = CHUD::m_hud_textcolor.w;
    push_back(&m_ammotype);
    m_ammocount.m_color[0] = CHUD::m_hud_textcolor.x;
    m_ammocount.m_color[1] = CHUD::m_hud_textcolor.y;
    m_ammocount.m_color[2] = CHUD::m_hud_textcolor.z;
    m_ammocount.m_color[3] = CHUD::m_hud_textcolor.w;
    push_back(&m_ammocount);
    m_magcount.m_color[0] = CHUD::m_hud_textcolor.x;
    m_magcount.m_color[1] = CHUD::m_hud_textcolor.y;
    m_magcount.m_color[2] = CHUD::m_hud_textcolor.z;
    m_magcount.m_color[3] = CHUD::m_hud_textcolor.w;
    push_back(&m_magcount);
    push_back(&m_selectionback0);
    push_back(&m_selectionback1);
    m_weaponselbkrnd.Off();
    m_selectionback0.Off();
    m_selectionback1.Off();
    for (int i = 0; i < 4; i++)
        m_weaponicon[i].Off();
    m_weaponname.Off();
    m_weapondescription.Off();
    m_magcount.Off();
    m_ammocount.Off();
    m_ammotype.Off();
    m_hotswapL1text.Off();
    m_hotswapL2text.Off();
    m_menuposx[1] = 30.0f;
    m_menuposy[1] = data_003E4A98;
    m_weaponselbkrnd.Load(0.0f, 5.0f + data_003E4AA0, data_0040C108, data_003E4A90 - 5.0f,
                          m_weaponselbkrndtex);
    m_selectionback0.Load(0.0f, data_003E4AA0, data_0040C108, 5.0f + data_003E4AA0, m_weaponselbkrndtex);
    m_selectionback1.Load(0.0f, data_003E4A90 - 5.0f, data_0040C108, data_003E4A90, m_weaponselbkrndtex);
    m_weaponicon[1].Load(m_menuposx[1] + m_iconoffsetx, m_menuposy[1] + m_iconoffsety, m_weapontex[1]);
    m_weaponname.Load(0.9f, "3", m_myfont, m_menuposx[1] + m_nameoffsetx, m_menuposy[1] + m_nameoffsety,
                      true);
    m_weapondescription.Load(0.9f, "3", m_myfont, m_menuposx[1] + m_nameoffsetx,
                             m_menuposy[1] + m_nameoffsety, true);
    m_ammotype.Load(0.9f, "2", m_myfont, m_menuposx[1] + m_nameoffsetx,
                    12.0f + (m_menuposy[1] + m_nameoffsety), true);
    m_ammocount.Load(0.9f, "1", m_myfont, m_menuposx[1] + m_nameoffsetx,
                     24.0f + (m_menuposy[1] + m_nameoffsety), true);
    m_magcount.Load(0.9f, "4testing", m_myfont, m_menuposx[1] + m_nameoffsetx,
                    36.0f + (m_menuposy[1] + m_nameoffsety), true);
    m_weaponname.Off();
    m_weapondescription.Off();
    m_ammotype.Off();
    m_ammocount.Off();
    m_magcount.Off();
    m_hotswapL1text.Load(0.9f, "l1", m_myfont, m_menuposx[1] + m_nameoffsetx,
                         42.0f + (m_menuposy[1] + m_nameoffsety), true);
    m_hotswapL2text.Load(0.9f, "l2", m_myfont, m_menuposx[1] + m_nameoffsetx,
                         50.0f + (m_menuposy[1] + m_nameoffsety), true);
    m_selectsound = CSnd::GetSoundByName(".TCM_SELECT");
    m_scrollupsound = CSnd::GetSoundByName(".TCM_SLIDE");
    m_scrolldownsound = CSnd::GetSoundByName(".TCM_SLIDE");
}

/* open, it closes first; the texts let go of */
void CWeaponSel::UnInit()
{
    if (m_openstate)
        OnClose();
    TextClearAndWipe();
}

/* the arrows, the backdrop and two backings off, closed (6), the HUD's reticule back and the
 * seal's, the texts off -- with a seal to own the HUD */
void CWeaponSel::OnClose()
{
    CZSealBody *seal = CHUD::GetHUDOwner();

    if (seal) {
        m_arrow[0].Off();
        m_arrow[1].Off();
        m_weaponselbkrnd.Off();
        m_selectionback0.Off();
        m_selectionback1.Off();
        m_openstate = 6;
        m_parent->m_usehudret = false;
        seal->m_kit.SetCurReticule(false);
        TextOff();
    }
}

/* the menu folded to its second row: the height, its centre and the goal there; shut (7), the
 * first and third icons off */
void CWeaponSel::AfterCloseFirst()
{
    m_curheight = m_goalheight = 2.0f + (32.0f + m_menuposy[1]) - (m_menuposy[1] - 1.0f);
    m_menucentery = m_goalheight / 2.0f;
    m_menugoaly = m_menuposy[1];
    m_menugoalx = m_menuposx[1];
    m_menustartx = 0.0f;
    m_openstate = 7;
    m_weaponicon[0].Off();
    m_weaponicon[2].Off();
}

/* shut: the seal's menu state back to none, its pad's cross, circle and triangle released, the
 * icons, arrows and weapon texts off, closed (0) */
void CWeaponSel::AfterClose()
{
    int ii;
    CZSealBody *seal = CHUD::GetHUDOwner();

    if (seal) {
        func_00597740(seal->m_ctrl, 0, 0);

        CPad *pad = GetPad(seal->m_ctrl->m_padindex);

        if (pad) {
            pad->SetButtonState(PAD_CROSS, KEY_UP);
            pad->SetButtonState(PAD_CIRCLE, KEY_UP);
            pad->SetButtonState(PAD_TRIANGLE, KEY_UP);
        }
        for (ii = 0; ii < 4; ii++)
            m_weaponicon[ii].Off();
        m_arrow[0].Off();
        m_arrow[1].Off();
        m_weaponname.Off();
        m_weapondescription.Off();
        m_ammotype.Off();
        m_ammocount.Off();
        m_magcount.Off();
        m_openstate = 0;
    }
}
