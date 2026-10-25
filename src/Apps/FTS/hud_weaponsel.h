/* The weapon selectors as every unit sees them: the single player's (CWeaponSel, SOCOM 1's
 * hud_weaponsel.cpp), the in-game one (CInGameWeaponSel, SOCOM 1's hud_ingame_weaponsel.cpp) and the
 * network game's (CMPWeaponSel, SOCOM II's, hud_mpweaponsel.cpp), with the HUD's bright text and the
 * network selector's slots. Laid out from their constructors and the implicit destructors
 * hud_main.cpp links (0x1FC700-0x1FC98F, 0x1FD730-0x1FDBDF: the HUD's members at 0x1A50, 0x2730 and
 * 0x38E0); the members named from SOCOM 1's DWARF where SOCOM II kept them. The header's name is
 * ours. */
#ifndef HUD_WEAPONSEL_H
#define HUD_WEAPONSEL_H

#include "../../gamez/ztwod/ztwod.h"

class CHUD;
class C2DFont;

/* a HUD text at 80 alpha (SOCOM 1's; its implicit destructor's copy is hud_main.cpp's, 0x1FC990) */
class CHudBrightText : public C2DString {
public:
    CHudBrightText() { SetTrans(80.0f); }
};   // 0x98 bytes

/* the single player's selector: C2D's 4 bytes gone, one selection backing fewer than SOCOM 1's */
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
    /* SOCOM 1's array of four backings is three members: the implicit destructor destroys each on
     * its own (0x1FDA10), where an array goes through __destroy_arr as m_weaponicon does */
    C2DBitmap m_selectionback0;   // 0x2b0
    C2DBitmap m_selectionback1;   // 0x330
    C2DBitmap m_selectionback2;   // 0x3b0
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

/* an item (zweapon.h's EQUIP_ITEM, a byte enum: copied with lb, compared with lbu; 255 none) */
enum EQUIP_ITEM { EQUIP_NONE = 255 };

/* the in-game selector: SOCOM 1's DWARF moved by SOCOM II's sizes (C2D 4 bytes less, C2DString
 * 0x98, one selected name fewer). OnClose is inline (SOCOM 1 links none) and reads the HUD's owner:
 * hud_ingame_weaponsel.cpp defines it, after its CHUD */
class CInGameWeaponSel : public C2D {
public:
    CInGameWeaponSel();
    void UnInit();
    void Clear();
    void UpdateCharacter();
    void SwitchOffText();
    void AfterClose();
    inline void OnClose();
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
    unsigned char _1174[0x11a4 - 0x1174];
    EQUIP_ITEM m_selected_item_index[5];   // 0x11a4 (SOCOM 1: six)
    unsigned char _11a9[0x11b0 - 0x11a9];
} __attribute__((aligned(16)));   // 0x11b0 bytes

/* the network selector's slots (their vtables found by their RTTI names: CWeaponSlot 0x405E00,
 * CategorySlot 0x405D80, SelectedSlot 0x405D00, SlotList 0x405E80) */
class CWeaponSlot : public C2D {
public:
    unsigned char _18[0x20 - 0x18];
};   // 0x20 bytes

class CategorySlot : public CWeaponSlot {
public:
    /* slot 11: the name and the bitmap faded together (0x23D810) */
    virtual void SetTrans(float trans);

    C2DBitmap m_bitmap;   // 0x20
    C2DString m_name;   // 0xa0
} __attribute__((aligned(16)));   // 0x140 bytes

class SelectedSlot : public CWeaponSlot {
public:
    /* slot 11: the name faded (0x23B990) */
    virtual void SetTrans(float trans);

    C2DString m_name;   // 0x20
};   // 0xb8 bytes

class SlotList : public C2D {
public:
    /* slots 1 and 11: the slots drawn while the list is on (0x23CCB0), all faded (0x240580) */
    virtual void Draw(zdb::CCamera *cam);
    virtual void SetTrans(float trans);

    unsigned char _18[0x20 - 0x18];
    CWeaponSlot *m_slots[5];   // 0x20
    unsigned char _34[0x38 - 0x34];
};   // 0x38 bytes

class CMPWeaponCategories;

/* the category page's update (no row named yet) */
extern "C" void func_0023D860(CMPWeaponCategories *page, float dT);

/* the selector's category page (no vtable, so no RTTI name: the name is ours) */
class CMPWeaponCategories {
public:
    /* the page's arrows, title, backdrops and list off (its category slots stay) */
    void Off()
    {
        for (int i = 0; i < 4; i++)
            m_polys[i].Off();
        m_title.Off();
        m_bitmap1.Off();
        m_bitmap2.Off();
        m_list.Off();
        m_bitmap3.Off();
    }
    /* on again, the last backdrop through the page's update */
    void On()
    {
        for (int i = 0; i < 4; i++)
            m_polys[i].On();
        m_title.On();
        m_bitmap1.On();
        m_bitmap2.On();
        m_list.On();
        func_0023D860(this, 0.0f);
    }

    C2DString m_title;   // 0x0
    C2DBitmap m_bitmap1;   // 0xa0
    C2DBitmap m_bitmap2;   // 0x120
    C2DBitmapPoly m_polys[4];   // 0x1a0
    SlotList m_list;   // 0x960
    CategorySlot m_categories[4];   // 0x9a0
    C2DBitmap m_bitmap3;   // 0xea0
    unsigned char _f20[0xf40 - 0xf20];
} __attribute__((aligned(16)));   // 0xf40 bytes

/* the network game's weapon selector: slot lists honouring the server's weapon valves
 * (Enable_203FRAG and the rest), UnInit only in a network game (SOCOM II's; no vtable, so no RTTI
 * name: the name is ours). Off and On are its own, not C2D's: they turn its lists and bitmaps off
 * and on (0x23E700, 0x23E800) */
class CMPWeaponSel {
public:
    CMPWeaponSel();
    void UnInit();
    void Off();
    void On();

    bool m_active;   // 0x0: set while the selector is up (On does nothing without it)
    C2DString m_title;   // 0x4
    C2DBitmap m_bitmap1;   // 0xa0
    C2DBitmap m_bitmap2;   // 0x120
    C2DBitmap m_bitmap3;   // 0x1a0
    CMPWeaponCategories m_categories;   // 0x220
    SlotList m_list;   // 0x1160
    SelectedSlot m_selected[5];   // 0x1198
    unsigned char _1530[0x1540 - 0x1530];
} __attribute__((aligned(16)));   // 0x1540 bytes

#endif
