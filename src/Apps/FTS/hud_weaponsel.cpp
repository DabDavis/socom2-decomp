/* hud_weaponsel.cpp -- the single-player weapon selector (CWeaponSel): SOCOM 1's unit (its DWARF
 * names it, Z:\dev\Apps\FTS\hud_weaponsel.cpp), its functions in SOCOM 1's order (OVL1
 * 0x219330-0x21DE4F: PlaceWeaponText ... OnClose, the state handlers, OnOpen, UnInit, Init, the
 * constructor). Written: AfterClose (0x21AAA0), AfterCloseFirst (0x21B0A0), OnClose (0x21B3E0)
 * and UnInit (0x21CBA0), which inlines OnClose as SOCOM 1's called it. The layout is hud_main.cpp's (the HUD's member, 0x1A50 there), named from SOCOM 1's
 * DWARF where SOCOM II kept the member: C2D's 4 bytes gone, one selection backing fewer.
 * Defined in reverse of their image order (-inline deferred). */

#include "../../gamez/ztwod/ztwod.h"
#include "../../gamez/zinput/zinput.h"

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

    unsigned char _0[0xb1];
    bool m_usehudret;   // 0xb1
};

/* a HUD text at 80 alpha (SOCOM 1's; hud_main.cpp's) */
class CHudBrightText : public C2DString {
public:
    CHudBrightText() { SetTrans(80.0f); }
};

class CWeaponSel : public C2D {
public:
    CWeaponSel();
    void UnInit();
    void OnClose();
    void AfterCloseFirst();
    void AfterClose();

    /* the texts off, and let go of (one line each in SOCOM 1's OnClose and UnInit: inline) */
    void TextOff()
    {
        m_hotswapL1text.Off();
        m_hotswapL2text.Off();
        m_weaponname.Off();
        m_weapondescription.Off();
        m_ammotype.Off();
        m_ammocount.Off();
        m_magcount.Off();
    }
    void TextClearAndWipe()
    {
        m_hotswapL1text.ClearAndWipe();
        m_hotswapL2text.ClearAndWipe();
        m_weaponname.ClearAndWipe();
        m_weapondescription.ClearAndWipe();
        m_ammotype.ClearAndWipe();
        m_ammocount.ClearAndWipe();
        m_magcount.ClearAndWipe();
    }

    CHUD *m_parent;   // 0x18
    C2DBitmap m_weaponicon[4];   // 0x20
    int m_weaponnum[4];   // 0x220
    C2DBitmap m_weaponselbkrnd;   // 0x230
    C2DBitmap m_selectionback[3];   // 0x2b0 (SOCOM 1: four)
    C2DBitmapPoly m_arrow[2];   // 0x430
    CHudBrightText m_hotswapL1text;   // 0x810
    CHudBrightText m_hotswapL2text;   // 0x8a8
    CHudBrightText m_weaponname;   // 0x940
    C2DString m_weapondescription;   // 0x9d8
    C2DString m_ammotype;   // 0xa70
    CHudBrightText m_ammocount;   // 0xb08
    CHudBrightText m_magcount;   // 0xba0
    unsigned char _c38[0xc58 - 0xc38];   // the textures (SOCOM 1: 12 words here, SOCOM II 8)
    int m_openstate;   // 0xc58
    float m_nameoffsetx;   // 0xc5c
    float m_nameoffsety;   // 0xc60
    float m_iconoffsetx;   // 0xc64
    float m_iconoffsety;   // 0xc68
    float m_menugoalx;   // 0xc6c
    float m_menugoaly;   // 0xc70
    float m_menuposx[4];   // 0xc74
    float m_menuposy[4];   // 0xc84
    float m_menucentery;   // 0xc94
    float m_menustartx;   // 0xc98
    float m_curheight;   // 0xc9c
    float m_goalheight;   // 0xca0
    unsigned char _ca4[0xce0 - 0xca4];
} __attribute__((aligned(16)));   // 0xce0 bytes

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
        m_selectionback[0].Off();
        m_selectionback[1].Off();
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
