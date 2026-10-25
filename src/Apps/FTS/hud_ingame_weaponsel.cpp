/* hud_ingame_weaponsel.cpp -- the in-game weapon selector (CInGameWeaponSel): SOCOM 1's unit (its
 * DWARF names it, Z:\dev\Apps\FTS\hud_ingame_weaponsel.cpp), its functions in SOCOM 1's order
 * (OVL1 ...-0x228653: ... AfterClose, SwitchOffText's neighbours, Clear, UnInit, Init, the
 * constructor). Written: SwitchOffText (0x227650), Clear (0x227880) and UnInit (0x227960), which
 * inlines the close and AfterCloseFirst as SOCOM 1's did (SOCOM 1 has no OnClose of its own:
 * inline; SOCOM II no AfterCloseFirst either -- only UnInit called it, so it was stripped). The layout is
 * SOCOM 1's DWARF moved by SOCOM II's sizes (C2D 4 bytes less, C2DString 0x98, one selected name
 * fewer): the HUD's member at 0x2730, hud_main.cpp's. Defined in reverse of their image order
 * (-inline deferred). */

#include "../../gamez/ztwod/ztwod.h"

class C2DFont;

namespace zdb {
class CTexHandle;
class CTexture;
}

/* the HUD as the selector reads it: the owner */
class CZSealBody;

class CHUD {
public:
    static CZSealBody *GetHUDOwner();
};

class CInGameWeaponSel : public C2D {
public:
    CInGameWeaponSel();
    void UnInit();
    void Clear();
    void UpdateCharacter();
    void SwitchOffText();
    void AfterClose();

    /* closed: the arrows and the backdrop off, the backings on, the texts off (inline: SOCOM 1
     * links no OnClose) */
    void OnClose()
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
    void AfterCloseFirst();

    CHUD *m_parent;   // 0x18
    C2DBitmap m_weaponicon[4];   // 0x20
    C2DString m_selectedtext[3];   // 0x220
    int m_selected[4];   // 0x3e8
    int m_icon_selected[4];   // 0x3f8
    C2DBitmap m_weaponselbkrnd;   // 0x410
    C2DBitmap m_selectionback[4];   // 0x490
    C2DBitmapPoly m_arrow[2];   // 0x690
    C2DString m_weaponname;   // 0xa70
    C2DString m_weapondescription;   // 0xb08
    C2DString m_ammotype;   // 0xba0
    C2DString m_ammocount;   // 0xc38
    C2DString m_magcount;   // 0xcd0
    C2DString m_weaponselectedname[5];   // 0xd68 (SOCOM 1: six)
    C2DString m_weaponselectedtitle;   // 0x1060
    zdb::CTexHandle *m_weaponselbkrndtex;   // 0x10f8
    zdb::CTexHandle *m_weapontex[4];   // 0x10fc
    zdb::CTexHandle *m_selectionbacktex[4];   // 0x110c
    zdb::CTexture *m_arrowtex;   // 0x111c
    C2DFont *m_myfont;   // 0x1120
    int m_current_item;   // 0x1124
    int m_openstate;   // 0x1128
    float m_nameoffsetx;   // 0x112c
    float m_nameoffsety;   // 0x1130
    float m_iconoffsetx;   // 0x1134
    float m_iconoffsety;   // 0x1138
    float m_menugoalx;   // 0x113c
    float m_menugoaly;   // 0x1140
    float m_menuposx[4];   // 0x1144
    float m_menuposy[4];   // 0x1154
    float m_menucentery;   // 0x1164
    float m_menustartx;   // 0x1168
    float m_curheight;   // 0x116c
    float m_goalheight;   // 0x1170
    unsigned char _1174[0x11b0 - 0x1174];
} __attribute__((aligned(16)));   // 0x11b0 bytes

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
