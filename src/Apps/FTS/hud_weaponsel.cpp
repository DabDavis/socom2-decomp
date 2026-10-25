/* hud_weaponsel.cpp -- the single-player weapon selector (CWeaponSel): SOCOM 1's unit (its DWARF
 * names it, Z:\dev\Apps\FTS\hud_weaponsel.cpp), its functions in SOCOM 1's order (OVL1
 * 0x219330-0x21DE4F: PlaceWeaponText ... OnClose, the state handlers, OnOpen, UnInit, Init, the
 * constructor). Written: OnClose (0x21B3E0) and UnInit (0x21CBA0), which inlines it as SOCOM 1's
 * called it. The layout is hud_main.cpp's (the HUD's member, 0x1A50 there), named from SOCOM 1's
 * DWARF where SOCOM II kept the member: C2D's 4 bytes gone, one selection backing fewer.
 * Defined in reverse of their image order (-inline deferred). */

#include "../../gamez/ztwod/ztwod.h"

/* the seal's kit: the reticule switch (SOCOM 1's SetCurReticule, a flag added) */
class CZKit {
public:
    void SetCurReticule(bool flag);
};

/* the seal as the selector reads it: its kit */
class CZSealBody {
public:
    unsigned char _0[0x5e0];
    CZKit m_kit;   // 0x5e0
};

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
    unsigned char _c38[0xc58 - 0xc38];
    int m_openstate;   // 0xc58
    unsigned char _c5c[0xce0 - 0xc5c];
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
