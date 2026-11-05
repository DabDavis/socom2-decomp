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
class CPad;

namespace zdb {
class CWorld;
}

/* a HUD text at 80 alpha (SOCOM 1's; its implicit destructor's copy is hud_main.cpp's, 0x1FC990) */
class CHudBrightText : public C2DString {
public:
    CHudBrightText() { SetTrans(80.0f); }
};   // 0x98 bytes

/* the single player's selector: C2D's 4 bytes gone, one selection backing fewer than SOCOM 1's */
class CWeaponSel : public C2D {
public:
    CWeaponSel();
    void Init(zdb::CWorld *world, C2DFont *font);
    void UnInit();
    void OnClose();
    void AfterCloseFirst();
    void AfterClose();
    void AfterScrollUp();
    void AfterScrollDown();
    void OnScrollUp();
    void OnScrollDown();
    void AfterOpenSecond();
    void ResetHotSwap(bool force);
    void PulseSelectionArrows(float dT);
    inline void AfterOpenFirst();
    void HandleOpenFirstState(float dT);
    void HandleCloseFirstState(float dT);
    void HudTick(CPad *pad, float dT);
    void OnOpen();
    void HandleOpenState(CPad *pad, float dT);
    void HandleScrollDownState(float dT);
    void HandleScrollUpState(float dT);
    /* the rows' icons placed for a state (SOCOM 1's, 0x219A80) */
    void SetItemPos(int index);
    void PlaceMiddleIcon();
    void TickHotSwap(float dT);
    /* a hot-swap shown (0x21A1F0, SOCOM II's: the name ours) */
    void CheckHotSwap();
    /* the selected weapon's texts and the hot-swap labels placed (SOCOM 1's, 0x2193D0 and
     * 0x2196F0) */
    void PlaceWeaponText();
    void PlaceHotSwapLabels();

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
    zdb::CTexHandle *m_weaponselbkrndtex;   // 0xc38 (SOCOM II has no m_selectionbacktex)
    zdb::CTexHandle *m_weapontex[4];   // 0xc3c
    zdb::CTexture *m_arrowtex;   // 0xc4c
    C2DFont *m_myfont;   // 0xc50
    int m_current_item;   // 0xc54
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
    float m_curtrans[2];   // 0xca4
    float m_goaltrans[2];   // 0xcac
    float m_curcolor;   // 0xcb4
    unsigned char m_incCurcolor;   // 0xcb8
    float m_arrowcolor;   // 0xcbc
    unsigned char m_arrowincrement;   // 0xcc0
    class CSnd *m_selectsound;   // 0xcc4
    class CSnd *m_scrollupsound;   // 0xcc8
    class CSnd *m_scrolldownsound;   // 0xccc
    int m_cursound;   // 0xcd0
} __attribute__((aligned(16)));   // 0xce0 bytes

/* an item (zweapon.h's EQUIP_ITEM, a byte enum: copied with lb, compared with lbu; 255 none) */
enum EQUIP_ITEM { EQUIP_NONE = 255 };

/* the in-game selector: SOCOM 1's DWARF moved by SOCOM II's sizes (C2D 4 bytes less, C2DString
 * 0x98, one selected name fewer). OnClose is inline (SOCOM 1 links none) and reads the HUD's owner:
 * hud_ingame_weaponsel.cpp defines it, after its CHUD */
class CInGameWeaponSel : public C2D {
public:
    CInGameWeaponSel();
    void Init(zdb::CWorld *world, C2DFont *font);
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
    float m_curtrans[2];   // 0x1174
    float m_goaltrans[2];   // 0x117c
    float m_curcolor;   // 0x1184
    unsigned char m_incCurcolor;   // 0x1188
    float m_arrowcolor;   // 0x118c
    unsigned char m_arrowincrement;   // 0x1190
    class CSnd *m_selectsound;   // 0x1194
    class CSnd *m_scrollupsound;   // 0x1198
    class CSnd *m_scrolldownsound;   // 0x119c
    int m_cursound;   // 0x11a0
    EQUIP_ITEM m_selected_item_index[5];   // 0x11a4 (SOCOM 1: six)
    unsigned char _11a9[0x11b0 - 0x11a9];
} __attribute__((aligned(16)));   // 0x11b0 bytes

/* a weapon's kind as the network selector's category page keeps it (a byte enum: copied with lb,
 * compared with andi -- the slot test 0x23C390 masks it before comparing with 2, 1 and 0; the
 * name ours) */
enum WEAPON_KIND { KIND_PRIMARY, KIND_SECONDARY, KIND_OTHER };

/* the network selector's state and its category page's (a byte enum: the state switch 0x240B90
 * masks it for its compare chain and stores it whole; the names ours) */
enum SEL_STATE { SEL_CLOSED, SEL_CHOSEN, SEL_LIST, SEL_PAGE };

/* the network selector's slots (their vtables found by their RTTI names: CWeaponSlot 0x405E00,
 * CategorySlot 0x405D80, SelectedSlot 0x405D00, SlotList 0x405E80) */
class CZWeapon;

class CWeaponSlot : public C2D {
public:
    /* slot 11: nothing to fade (0x2405F0, after SlotList::SetTrans) */
    virtual void SetTrans(float trans);
    /* slot 1: nothing drawn (0x23C380) */
    virtual void Draw(zdb::CCamera *cam);
    /* the class's own slots, +0x70 and +0x74 (the names ours): the weapon shown, kept at 0x18
     * (0x23C370), and the slot placed, nothing here (0x23C360) */
    virtual void SetWeapon(CZWeapon *weapon);
    virtual void SetPos(float x, float y);

    CZWeapon *m_weapon;   // 0x18
    unsigned char _1c[0x20 - 0x1c];
};   // 0x20 bytes

class CategorySlot : public CWeaponSlot {
public:
    /* slot 11: the name and the bitmap faded together (0x23D810) */
    virtual void SetTrans(float trans);
    /* the bitmap and the name drawn while on (0x23C220) */
    virtual void Draw(zdb::CCamera *cam);
    /* the weapon's icon and its description shown, or both off (0x23C0D0) */
    virtual void SetWeapon(CZWeapon *weapon);
    /* the bitmap at the place, the name 4 above it (0x23C2B0) */
    virtual void SetPos(float x, float y);
    /* a slot of its own, +0x78 (the name ours): the name's colour (0x23C2A0) */
    virtual void SetColor(float r, float g, float b);

    C2DBitmap m_bitmap;   // 0x20
    C2DString m_name;   // 0xa0
} __attribute__((aligned(16)));   // 0x140 bytes

class SelectedSlot : public CWeaponSlot {
public:
    /* slot 11: the name faded (0x23B990) */
    virtual void SetTrans(float trans);
    /* the name drawn while on (0x23C070) */
    virtual void Draw(zdb::CCamera *cam);
    /* the weapon's description shown, or the name off (0x23BF60) */
    virtual void SetWeapon(CZWeapon *weapon);
    /* the name at the place (0x23BF20) */
    virtual void SetPos(float x, float y);
    /* a slot of its own, +0x78 (the name ours): the name's colour (0x23BF10) */
    virtual void SetColor(float r, float g, float b);

    C2DString m_name;   // 0x20
};   // 0xb8 bytes

class SlotList : public C2D {
public:
    /* slots 1 and 11: the slots drawn while the list is on (0x23CCB0), all faded (0x240580) */
    virtual void Draw(zdb::CCamera *cam);
    virtual void SetTrans(float trans);
    /* the next weapon class's first id up or down from a class, round 0-254 (0x23CB90; the name
     * ours) */
    EQUIP_ITEM NextClass(bool up, EQUIP_ITEM cls);
    /* the next weapon's index up or down the weapons' list that the slot takes (0x23C6F0; the name
     * ours: its last two arguments passed through to the slot's test) */
    int NextWeapon(bool up, int index, EQUIP_ITEM type, WEAPON_KIND kind);
    /* whether the list's slot takes a weapon (0x23C390; the name ours) */
    bool CanTake(CZWeapon *weapon, EQUIP_ITEM type, WEAPON_KIND kind);
    /* the next weapon class up or down in the page's fixed order that the list takes a weapon of
     * (0x23C840; the name ours), and the test (inline) */
    EQUIP_ITEM NextCategory(bool up, EQUIP_ITEM cls, WEAPON_KIND kind);
    inline bool HasWeapons(EQUIP_ITEM cls);

    /* no owner, no slots yet (inline: the selector's constructor builds two) */
    SlotList()
    {
        for (int i = 0; i < 5; i++)
            m_slots[i] = 0;
        m_owner = 0;
    }

    class CMPWeaponSel *m_owner;   // 0x18: the selector (its Init sets the category page's)
    unsigned char _1c[0x20 - 0x1c];
    CWeaponSlot *m_slots[5];   // 0x20
    int m_34;   // 0x34: zeroed by the selector's Init
};   // 0x38 bytes

class CMPWeaponCategories;

/* the category page's update (no row named yet) */
extern "C" void func_0023D860(CMPWeaponCategories *page, float dT);

/* the selector's category page (no vtable, so no RTTI name: the name is ours) */
class CMPWeaponCategories {
public:
    /* inline: the selector's constructor builds it in place */
    CMPWeaponCategories() : m_f34(1.0f), m_f38(0) {}

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
    /* the page made (0x23CD20; the name ours) */
    void Init();
    /* the weapon shown: its class (GetIDType, 255 none), its kind (0 primary, 1 secondary, 2 other)
     * and its index in the weapons' list (0x23D540; the names ours) */
    void SetWeapon(CZWeapon *weapon);
    /* the four arrows off, or grey (0x23D460; the name ours) */
    void ShowArrows();
    /* the arrows pulsed (0x23D2B0; the name ours) */
    void TickArrows(float dT);
    /* the cursor moved a weapon (2, -2) or a class (-1, 1) (0x23D060; the name ours) */
    void Move(signed char dir);
    /* the page's buttons while it is up (0x23D1B0; the name ours) */
    void HandlePad(CPad *pad);

    EQUIP_ITEM m_type;   // 0xf20
    WEAPON_KIND m_kind;   // 0xf21
    SEL_STATE m_state;   // 0xf22: 3 while the page is up, 2 back to the list, 1 chosen
    int m_index;   // 0xf24
    float m_pulse;   // 0xf28: the arrows' colour blend, 0 grey to 1 yellow
    bool m_pulseup;   // 0xf2c: the blend rising
    unsigned char _f2d[0xf30 - 0xf2d];
    class CMPWeaponSel *m_owner;   // 0xf30: the selector (its Init sets it)
    float m_f34;   // 0xf34: 1 from the constructor
    int m_f38;   // 0xf38
    unsigned char _f3c[0xf40 - 0xf3c];
} __attribute__((aligned(16)));   // 0xf40 bytes

/* the network game's weapon selector: slot lists honouring the server's weapon valves
 * (Enable_203FRAG and the rest), UnInit only in a network game (SOCOM II's; no vtable, so no RTTI
 * name: the name is ours). Off and On are its own, not C2D's: they turn its lists and bitmaps off
 * and on (0x23E700, 0x23E800) */
class CMPWeaponSel {
public:
    CMPWeaponSel();
    void Init();
    void UnInit();
    void Off();
    void On();
    /* the cursor moved a slot on (2) or back (-2), round the five, on past the slots its test
     * refuses; the category page shows the slot's weapon (0x23EAE0; the name ours) */
    void Scroll(signed char dir);
    /* the list's buttons (0x23EBC0; the name ours) */
    void HandlePad(CPad *pad);
    /* the selector laid out (0x23E030; the name ours) */
    void Layout(float dT);
    /* the five selected slots' weapons given to the HUD owner's character (0x23E5E0; the name ours) */
    void SetCharacterWeapons();
    /* opened on the list, the slots filled from the owner's weapons (0x23E370; the name ours) */
    void Open();
    /* the selector's state changed (0x240B90; the name ours) */
    void SetState(SEL_STATE state);
    /* whether a slot can be chosen (0x23E910; the name ours), the equipment slot holding an item
     * (the one asked about skipped) and the item's own test (inline) */
    bool CanSelect(int index);
    /* an equipment item no equipment slot holds: the item, or, when a slot holds it, the next
     * equipment item round the weapons' list that the player's side may take (0x23BC50) -- and the
     * same for a slot left empty, passing over another item (0x23B9B0; the names ours) */
    EQUIP_ITEM NextFreeEquipment(EQUIP_ITEM item, int index);
    EQUIP_ITEM PickEquipment(int index, EQUIP_ITEM item, EQUIP_ITEM other);
    inline int FindEquipment(EQUIP_ITEM item, int skip);
    inline bool HasClassElsewhere(EQUIP_ITEM cls);
    inline bool IsSelectedElsewhere(CZWeapon *weapon);
    inline EQUIP_ITEM CursorItem();
    inline void CheckItem(EQUIP_ITEM item, int index, bool &ok);
    inline void Set203(bool on);
    inline void SetGL(bool on);
    inline void SetF2000(bool on);

    SEL_STATE m_state;   // 0x0: closed (0) or up (On does nothing closed)
    C2DString m_title;   // 0x4
    C2DBitmap m_bitmap1;   // 0xa0
    C2DBitmap m_bitmap2;   // 0x120
    C2DBitmap m_bitmap3;   // 0x1a0
    CMPWeaponCategories m_categories;   // 0x220
    SlotList m_list;   // 0x1160
    SelectedSlot m_selected[5];   // 0x1198
    unsigned char m_1530;   // 0x1530: 255 from Init
    unsigned char m_1531;   // 0x1531: 255 from Init
    bool m_203;   // 0x1532: the M203's grenades enabled (Enable_203FRAG and the rest)
    bool m_f2000;   // 0x1533: the F2000's (Enable_F2000FRAG ...)
    bool m_gl;   // 0x1534: the grenade launcher's (Enable_GLFRAG ...)
    unsigned char _1535[0x1538 - 0x1535];
    float m_1538;   // 0x1538: 1 from the constructor
    int m_153c;   // 0x153c
} __attribute__((aligned(16)));   // 0x1540 bytes

#endif
