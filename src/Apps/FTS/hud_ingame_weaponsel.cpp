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

/* the seal's controller as the selector reads it: the pad it reads (SOCOM 1's at 0xE8) */
class CSealCtrl {
public:
    unsigned char _0[0x114];
    unsigned int m_padindex;   // 0x114
};

/* SOCOM 1's CSealCtrl::SetMenuState with a flag added (OVL2; the row unnamed: called as C) */
extern "C" void func_00597740(CSealCtrl *ctrl, int state, int flag);

/* the seal as the selector reads it: its controller */
class CZSealBody {
public:
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
        func_00597740(seal->m_ctrl, 0, 0);

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
