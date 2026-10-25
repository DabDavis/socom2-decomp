/* hud_mpweaponsel.cpp -- the network game's weapon selector (CMPWeaponSel, SOCOM II's: no vtable,
 * no RTTI name; the class's and the unit's names are ours). Its slots' vtables (CWeaponSlot,
 * CategorySlot, SelectedSlot, SlotList) put it after the mission cams, OVL1 ...-0x241BDF, the
 * constructor last as in SOCOM 1's selector units. Written: Off (0x23E700), On (0x23E800) and
 * UnInit (0x240D50), which inlines Off. The layout is hud_main.cpp's (the HUD's member at 0x38E0).
 * Defined in reverse of their image order (-inline deferred). */

#include "../../gamez/ztwod/ztwod.h"

/* the network selector's slots (their vtables found by their RTTI names: CWeaponSlot 0x405E00,
 * CategorySlot 0x405D80, SelectedSlot 0x405D00, SlotList 0x405E80) */
class CWeaponSlot : public C2D {
public:
    unsigned char _18[0x20 - 0x18];
};   // 0x20 bytes

class CategorySlot : public CWeaponSlot {
public:
    C2DBitmap m_bitmap;   // 0x20
    C2DString m_name;   // 0xa0
} __attribute__((aligned(16)));   // 0x140 bytes

class SelectedSlot : public CWeaponSlot {
public:
    C2DString m_name;   // 0x20
};   // 0xb8 bytes

class SlotList : public C2D {
public:
    unsigned char _18[0x38 - 0x18];
};   // 0x38 bytes

class CMPWeaponCategories;
class CMPWeaponSel;

/* the category page's and the selector's own updates (no rows named yet) */
extern "C" void func_0023D860(CMPWeaponCategories *page, float dT);
extern "C" void func_0023E030(CMPWeaponSel *sel, float dT);
/* the selector's state switch (an unsigned char: 0 closed, 1-3 its pages) */
extern "C" void func_00240B90(CMPWeaponSel *sel, unsigned char state);

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

/* off, and closed */
void CMPWeaponSel::UnInit()
{
    Off();
    func_00240B90(this, 0);
}

/* while up: the lists, the page, the title and the backdrops on, then updated */
void CMPWeaponSel::On()
{
    if (m_active) {
        m_list.On();
        m_categories.On();
        m_title.On();
        m_bitmap1.On();
        m_bitmap2.On();
        m_bitmap3.On();
        func_0023E030(this, 0.0f);
    }
}

/* the lists, the page, the title and the backdrops off */
void CMPWeaponSel::Off()
{
    m_list.Off();
    m_categories.Off();
    m_title.Off();
    m_bitmap1.Off();
    m_bitmap2.Off();
    m_bitmap3.Off();
}
