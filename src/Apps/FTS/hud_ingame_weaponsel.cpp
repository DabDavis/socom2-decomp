/* hud_ingame_weaponsel.cpp -- the in-game weapon selector (CInGameWeaponSel): SOCOM 1's unit (its
 * DWARF names it, Z:\dev\Apps\FTS\hud_ingame_weaponsel.cpp), its functions in SOCOM 1's order
 * (OVL1 ...-0x228653: ... AfterClose, SwitchOffText's neighbours, Clear, UnInit, Init, the
 * constructor). Written: SwitchOffText (0x227650), AfterClose (0x227730), Clear (0x227880) and
 * UnInit (0x227960), which
 * inlines the close and AfterCloseFirst as SOCOM 1's did (SOCOM 1 has no OnClose of its own:
 * inline; SOCOM II no AfterCloseFirst either -- only UnInit called it, so it was stripped). The layout is
 * hud_weaponsel.h's. Defined in reverse of their image order
 * (-inline deferred). */

#include "../../gamez/ztwod/ztwod.h"
#include "../../gamez/zinput/zinput.h"
#include "hud_weaponsel.h"

/* the controller's menu states (hud_missioncams.h's MENU_STATE, SOCOM 1's DWARF) */
enum MENU_STATE {
    stateNone = 0,
    stateWeaponSel = 1,
    stateOrders = 2,
    statePauseTestMenu = 3,
    stateFullScreenMapMenu = 4,
    stateFullScreenMapAnim = 5,
    stateLast = 6,
};

/* the seal's controller as the selector reads it: the pad it reads (SOCOM 1's at 0xE8) and SOCOM 1's
 * SetMenuState with a flag added (OVL2) */
class CSealCtrl {
public:
    void SetMenuState(MENU_STATE state, bool flag);

    unsigned char _0[0x114];
    unsigned int m_padindex;   // 0x114
};


/* a character's weapon and its list (SOCOM 1's CCharacterWeap, 12 bytes, and CCharacterWeapList, a
 * vector of them: its size at 4, its elements at 8) */
class CCharacterWeap {
public:
    /* SOCOM 1's SetupCharacterWeapon (OVL2; AddWeapon is the list's member below, 0x53B3A0) */
    void SetupCharacterWeapon(EQUIP_ITEM item);

    unsigned char _0[0xc];
};

class CCharacterWeapList {
public:
    unsigned int size() const { return m_size; }
    CCharacterWeap &operator[](unsigned int n) { return m_data[n]; }

    unsigned int _0;
    unsigned int m_size;   // 0x4
    CCharacterWeap *m_data;   // 0x8

    void AddWeapon(EQUIP_ITEM item);
};

/* a character's type as the selector reads it: its weapons (SOCOM 1's at 0x250) */
class CCharacterType {
public:
    unsigned char _0[0x2e8];
    CCharacterWeapList m_weapons;   // 0x2e8
};

/* the seal as the selector reads it: its controller */
class CZSealBody {
public:
    CCharacterType *GetCharacter();

    unsigned char _0[0xc0];
    CSealCtrl *m_ctrl;   // 0xc0
};

/* a pad by its index, none past the last (SOCOM 1's inline over CInput's table) */
static inline CPad *GetPad(unsigned int n)
{
    return n < CInput::NUM_PADS ? CInput::m_pads[n] : 0;
}

/* the HUD as the selector reads it: the owner */
class CHUD {
public:
    static CZSealBody *GetHUDOwner();
};

/* closed: the arrows and the backdrop off, the backings on, the texts off (inline: SOCOM 1 links
 * no OnClose) */
inline void CInGameWeaponSel::OnClose()
{
    if (CHUD::GetHUDOwner()) {
        m_arrow[0].Off();
        m_arrow[1].Off();
        m_weaponselbkrnd.Off();
        m_selectionback[0].On();
        m_selectionback[1].On();
        m_selectionback[2].On();
        m_selectionback[3].On();
        m_openstate = 6;
        SwitchOffText();
        UpdateCharacter();
    }
}

/* the selector made closed: no sound, the menu's places and offsets cleared, the icons 4 apart,
 * the colours' pulses at their starts, nothing selected (SOCOM 1's; OVL1 0x2284C0, the unit's last
 * function -- CHUD's constructor builds CHUD+0x2730 with it; its table 0x405150) */
CInGameWeaponSel::CInGameWeaponSel()
{
    int i;

    m_cursound = -1;
    m_openstate = 0;
    for (i = 0; i < 4; i++) {
        m_menuposx[i] = 0.0f;
        m_menuposy[i] = 0.0f;
    }
    m_nameoffsetx = 0.0f;
    m_nameoffsety = 0.0f;
    m_iconoffsetx = 4.0f;
    m_iconoffsety = 4.0f;
    m_current_item = 0;
    m_curheight = 0.0f;
    m_goalheight = 0.0f;
    m_incCurcolor = 0;
    m_curcolor = 1.0f;
    for (i = 3; i >= 0; i--)
        m_selected[i] = 0;
    for (i = 3; i >= 0; i--)
        m_icon_selected[i] = 0;
    m_arrowcolor = 50.0f;
    m_arrowincrement = 1;
}

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

/* the textures fetched, the pieces listed, the texts coloured, everything off, then the backdrop,
 * the second backing, the icon and the texts loaded at the second slot */
void CInGameWeaponSel::Init(zdb::CWorld *world, C2DFont *font)
{
    int i;

    m_openstate = 0;
    m_weapontex[0] = zdb::CAssetMgr::m_assets.GetTexHandle("reticuletex.tif");
    m_weapontex[1] = zdb::CAssetMgr::m_assets.GetTexHandle("reticuletex.tif");
    m_weapontex[2] = zdb::CAssetMgr::m_assets.GetTexHandle("reticuletex.tif");
    m_weapontex[3] = zdb::CAssetMgr::m_assets.GetTexHandle("reticuletex.tif");
    m_weaponselbkrndtex = zdb::CAssetMgr::m_assets.GetTexHandle("weapon_selection.tif");
    m_selectionbacktex[0] = zdb::CAssetMgr::m_assets.GetTexHandle("hud_weapon_selected_on.tif");
    m_selectionbacktex[1] = zdb::CAssetMgr::m_assets.GetTexHandle("hud_weapon_selected_on.tif");
    m_selectionbacktex[2] = zdb::CAssetMgr::m_assets.GetTexHandle("hud_weapon_selected_on.tif");
    m_selectionbacktex[3] = zdb::CAssetMgr::m_assets.GetTexHandle("hud_weapon_selected_on.tif");
    m_arrowtex = GetTextureByName(world, "hud_arrow_off.tif");
    m_myfont = font;
    push_back(&m_weaponselbkrnd);
    push_back(&m_selectionback[0]);
    push_back(&m_weaponicon[0]);
    push_back(&m_selectedtext[0]);
    push_back(&m_selectionback[3]);
    push_back(&m_weaponicon[3]);
    push_back(&m_selectionback[2]);
    push_back(&m_weaponicon[2]);
    push_back(&m_selectedtext[2]);
    m_selectionback[2].SetUV(0.01f, 0.99f, 0.99f, 0.01f);
    m_selectionback[0].SetUV(0.1f, 0.01f, 0.99f, 0.99f);
    push_back(&m_selectionback[1]);
    push_back(&m_weaponicon[1]);
    push_back(&m_selectedtext[1]);
    for (i = 0; i < 5; i++)
        push_back(&m_weaponselectedname[i]);
    push_back(&m_weaponselectedtitle);
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
    m_weaponname.m_color[0] = 128.0f;
    m_weaponname.m_color[1] = 128.0f;
    m_weaponname.m_color[2] = 128.0f;
    m_weaponname.m_color[3] = 80.0f;
    push_back(&m_weaponname);
    m_weapondescription.m_color[0] = 128.0f;
    m_weapondescription.m_color[1] = 128.0f;
    m_weapondescription.m_color[2] = 128.0f;
    m_weapondescription.m_color[3] = 80.0f;
    push_back(&m_weapondescription);
    m_ammotype.m_color[0] = 128.0f;
    m_ammotype.m_color[1] = 128.0f;
    m_ammotype.m_color[2] = 128.0f;
    m_ammotype.m_color[3] = 80.0f;
    push_back(&m_ammotype);
    m_ammocount.m_color[0] = 128.0f;
    m_ammocount.m_color[1] = 128.0f;
    m_ammocount.m_color[2] = 128.0f;
    m_ammocount.m_color[3] = 80.0f;
    push_back(&m_ammocount);
    m_magcount.m_color[0] = 128.0f;
    m_magcount.m_color[1] = 128.0f;
    m_magcount.m_color[2] = 128.0f;
    m_magcount.m_color[3] = 80.0f;
    push_back(&m_magcount);
    m_weaponselbkrnd.Off();
    m_weaponselbkrnd.SetTrans(100.0f);
    for (i = 0; i < 4; i++) {
        m_selectionback[i].Off();
        m_weaponicon[i].Off();
    }
    for (i = 0; i < 3; i++)
        m_selectedtext[i].Off();
    for (i = 0; i < 5; i++)
        m_weaponselectedname[i].Off();
    m_weaponselectedtitle.Off();
    m_weaponname.Off();
    m_weapondescription.Off();
    m_magcount.Off();
    m_ammocount.Off();
    m_ammotype.Off();
    m_menuposx[1] = 30.0f;
    m_menuposy[1] = 200.0f;

    float y = m_menuposy[1] - 1.0f - 100.0f;
    float x = m_menuposx[1] - 1.0f;

    m_weaponselbkrnd.Load(x, y, 171.0f + x, 256.0f + y, m_weaponselbkrndtex);
    m_selectionback[1].Load(m_menuposx[1] - 1.0f, m_menuposy[1] - 1.0f, 2.0f + (128.0f + m_menuposx[1]),
                            2.0f + (32.0f + m_menuposy[1]), m_selectionbacktex[1]);
    m_selectionback[1].SetUV(0.01f, 0.01f, 0.99f, 0.99f);
    m_weaponicon[1].Load(m_menuposx[1] + m_iconoffsetx, m_menuposy[1] + m_iconoffsety, m_weapontex[1]);
    m_weaponname.Load(1.0f, "3", m_myfont, m_menuposx[1] + m_nameoffsetx, m_menuposy[1] + m_nameoffsety,
                      true);
    m_weapondescription.Load(1.0f, "3", m_myfont, m_menuposx[1] + m_nameoffsetx,
                             m_menuposy[1] + m_nameoffsety, true);
    m_ammotype.Load(1.0f, "2", m_myfont, m_menuposx[1] + m_nameoffsetx,
                    12.0f + (m_menuposy[1] + m_nameoffsety), true);
    m_ammocount.Load(1.0f, "1", m_myfont, m_menuposx[1] + m_nameoffsetx,
                     24.0f + (m_menuposy[1] + m_nameoffsety), true);
    m_magcount.Load(1.0f, "4testing", m_myfont, m_menuposx[1] + m_nameoffsetx,
                    36.0f + (m_menuposy[1] + m_nameoffsety), true);
    m_weaponname.Off();
    m_weapondescription.Off();
    m_ammotype.Off();
    m_ammocount.Off();
    m_magcount.Off();
    m_selectsound = CSnd::GetSoundByName(".TCM_SELECT");
    m_scrollupsound = CSnd::GetSoundByName(".TCM_SLIDE");
    m_scrolldownsound = CSnd::GetSoundByName(".TCM_SLIDE");
}

/* open, it is closed and shut at once; the texts let go of */
void CInGameWeaponSel::UnInit()
{
    if (m_openstate) {
        OnClose();
        AfterCloseFirst();
        AfterClose();
    }
    Clear();
}

/* every text let go of */
void CInGameWeaponSel::Clear()
{
    int ii;

    m_weaponname.ClearAndWipe();
    m_weapondescription.ClearAndWipe();
    m_ammotype.ClearAndWipe();
    m_ammocount.ClearAndWipe();
    m_magcount.ClearAndWipe();
    for (ii = 0; ii < 3; ii++)
        m_selectedtext[ii].ClearAndWipe();
    for (ii = 0; ii < 5; ii++)
        m_weaponselectedname[ii].ClearAndWipe();
    m_weaponselectedtitle.ClearAndWipe();
}

/* shut: the seal's menu state back to none, its pad's triangle held, the backings, icons, arrows
 * and weapon texts off, closed (0) */
void CInGameWeaponSel::AfterClose()
{
    int ii;
    CZSealBody *seal = CHUD::GetHUDOwner();

    if (seal) {
        seal->m_ctrl->SetMenuState(stateNone, false);

        CPad *pad = GetPad(seal->m_ctrl->m_padindex);

        if (pad)
            pad->SetButtonState(PAD_TRIANGLE, KEY_DOWN);
        for (ii = 0; ii < 4; ii++)
            m_selectionback[ii].Off();
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

/* every text off */
void CInGameWeaponSel::SwitchOffText()
{
    int ii;

    m_weaponname.Off();
    m_weapondescription.Off();
    m_ammotype.Off();
    m_ammocount.Off();
    m_magcount.Off();
    for (ii = 0; ii < 3; ii++)
        m_selectedtext[ii].Off();
    for (ii = 0; ii < 5; ii++)
        m_weaponselectedname[ii].Off();
    m_weaponselectedtitle.Off();
}

/* the menu folded to its second row: the height, its centre and the goal there; shut (7), the
 * first and third icons and backings off */
void CInGameWeaponSel::AfterCloseFirst()
{
    m_curheight = m_goalheight = 2.0f + (32.0f + m_menuposy[1]) - (m_menuposy[1] - 1.0f);
    m_menucentery = m_goalheight / 2.0f;
    m_menugoaly = m_menuposy[1];
    m_menugoalx = m_menuposx[1];
    m_menustartx = 0.0f;
    m_openstate = 7;
    m_weaponicon[0].Off();
    m_selectionback[0].Off();
    m_weaponicon[2].Off();
    m_selectionback[2].Off();
}

/* the HUD owner's character given the selected items: its weapons set up as far as it has them,
 * then the rest added (and set up, if any) -- SOCOM 1's, five items in SOCOM II */
void CInGameWeaponSel::UpdateCharacter()
{
    CZSealBody *seal = CHUD::GetHUDOwner();
    CCharacterWeapList &weaps = seal->GetCharacter()->m_weapons;
    int ii;

    for (ii = 0; ii < 5; ii++) {
        if (ii >= weaps.size())
            break;
        weaps[ii].SetupCharacterWeapon(m_selected_item_index[ii]);
    }
    for (; ii < 5; ii++) {
        weaps.AddWeapon(m_selected_item_index[ii]);
        if (m_selected_item_index[ii] != EQUIP_NONE)
            weaps[ii].SetupCharacterWeapon(m_selected_item_index[ii]);
    }
}
