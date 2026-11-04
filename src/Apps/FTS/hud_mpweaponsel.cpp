/* hud_mpweaponsel.cpp -- the network game's weapon selector (CMPWeaponSel, SOCOM II's: no vtable,
 * no RTTI name; the class's and the unit's names are ours). Its slots' vtables (CWeaponSlot,
 * CategorySlot, SelectedSlot, SlotList) put it after the mission cams, OVL1 ...-0x241BDF, the
 * constructor last as in SOCOM 1's selector units. Written: Off (0x23E700), On (0x23E800) and
 * UnInit (0x240D50), which inlines Off. The layout is hud_weaponsel.h's.
 * The initialiser (0x3FF7F0) is the file statics below, defined in address order (round 20).
 * Defined in reverse of their image order (-inline deferred). */

#include "../../gamez/ztwod/ztwod.h"
#include "hud_weaponsel.h"

/* the selector's own update (no row named yet) */
extern "C" void func_0023E030(CMPWeaponSel *sel, float dT);
/* the selector's state switch (an unsigned char: 0 closed, 1-3 its pages) */
extern "C" void func_00240B90(CMPWeaponSel *sel, unsigned char state);

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

/* the seal's kit: its primary weapon (OVL2) */
class CZKit {
public:
    EQUIP_ITEM GetPrimaryEnum() const;
};

class CZSealBody {
public:
    unsigned char _0[0x5e0];
    CZKit m_kit;   // 0x5e0
};

class CHUD {
public:
    static CZSealBody *GetHUDOwner();
    void Add2D(C2D *p2d, int phase, bool add_to_back);
    static void HudText_to_upper(char *instr);

    unsigned char _0[0x498];
    C2DFont *m_myfont;   // 0x498
};

extern CHUD *theHUD;

/* a weapon as its slot shows it: its description and its icon (zweapon's CZWeapon) */
class CZWeapon {
public:
    const char *GetDescription() const;

    unsigned char _0[0xbc];
    zdb::CTexHandle *m_icon;   // 0xbc
};

class CZFTSWeapon {
public:
    static bool IsM203Weapon(EQUIP_ITEM item);
};

/* a valve as the selector sets it (zanim.h's CValve: its inline setter) */
enum VALVE_STATE {
    VALVE_SAME,
    VALVE_ON,
    VALVE_OFF
};

class CValve {
public:
    static CValve *GetByName(const char *name);
    void MakeCallbacks(VALVE_STATE state);
    bool SetValue(int value)
    {
        VALVE_STATE state = VALVE_SAME;

        if (m_value != value)
            state = value == 0 ? VALVE_OFF : VALVE_ON;
        m_value = value;
        if (state != VALVE_SAME) {
            MakeCallbacks(state);
            return true;
        }
        return false;
    }

    char *m_name;   // 0x0
    unsigned int m_value : 16;   // 0x4
};

/* a valve set by name, if there is one */
static inline void SetValve(const char *name, int value)
{
    CValve *valve = CValve::GetByName(name);

    if (valve)
        valve->SetValue(value);
}

/* the category page made (0x23CD20; no row named yet) */
extern "C" void func_0023CD20(CMPWeaponCategories *page);

/* the selector's layout: the backdrop's top, the list's left and top, the boxes' sizes (plain
 * initialised data 8 bytes apart, the unit's statics) and the backdrop's left (bss: the unit's
 * initialiser copies the list's left into it) */
static float s_listheight = 110.0f;
static float s_width = 170.0f;
static float s_listx = -10.0f;
static float s_listy = 140.0f;
static float s_gap = 10.0f;
static float s_rowheight = 14.0f;
static float s_slotheight = 64.0f;
static float s_db8 = 225.0f;
static float s_titley = 105.0f;
static float s_titleheight = 35.0f;

/* the pages' colours and places, computed by the unit's initialiser (0x3FF7F0) in this order --
 * bss 0x4146C0-0x4147AB; the names ours, their users (0x23CD20, 0x23D860, 0x23E030, 0x240600)
 * unwritten but the backdrop's left */
static CPnt3D s_color1(80.0f, 80.0f, 80.0f);
static CPnt3D s_color2(128.0f, 128.0f, 128.0f);
static float s_catx = s_listx + s_width + s_gap;
static float s_caty = s_listy;
static float s_6f0 = s_catx;
static float s_6f8 = 14.0f + s_caty + s_rowheight;
static float s_700 = s_width;
static float s_708 = s_catx;
static CPnt3D s_color3(128.0f, 128.0f, 64.0f);
static float s_720 = s_slotheight;
static float s_728 = s_width;
static CPnt3D s_color4(11.0f, 45.0f, 64.0f);
static float s_740 = s_catx;
static float s_748 = s_6f8 + s_slotheight - 6.0f;
static float s_750 = s_width;
static CPnt3D s_color5(11.0f, 45.0f, 64.0f);
static float s_768 = s_listx;
static float s_backx = s_listx;
static float s_778 = 5.0f + s_listx;
static CPnt3D s_color6(128.0f, 128.0f, 64.0f);
static CPnt3D s_color7(80.0f, 120.0f, 120.0f);
static CPnt3D s_color8(160.0f, 250.0f, 250.0f);

/* the 203's, the grenade launcher's and the F2000's grenades let through or not (the server's
 * valves: nine rounds each when they are) */
inline void CMPWeaponSel::Set203(bool on)
{
    m_203 = on;
    SetValve("Enable_203FRAG", on ? 9 : 0);
    SetValve("Enable_203SMK", on ? 9 : 0);
    SetValve("Enable_203HE", on ? 9 : 0);
}

inline void CMPWeaponSel::SetGL(bool on)
{
    m_gl = on;
    SetValve("Enable_GLFRAG", on ? 9 : 0);
    SetValve("Enable_GLSMK", on ? 9 : 0);
    SetValve("Enable_GLHE", on ? 9 : 0);
}

inline void CMPWeaponSel::SetF2000(bool on)
{
    m_f2000 = on;
    SetValve("Enable_F2000FRAG", on ? 9 : 0);
    SetValve("Enable_F2000SMK", on ? 9 : 0);
    SetValve("Enable_F2000HE", on ? 9 : 0);
}

/* closed; the slots listed; the backdrops loaded, added to the HUD's phases and faded out; the
 * grenades of the owner's primary weapon let through, the others' not */
void CMPWeaponSel::Init()
{
    m_active = false;
    m_list.m_34 = 0;
    m_1530 = 255;
    m_1531 = 255;
    m_list.Off();
    func_0023CD20(&m_categories);
    m_categories.m_list.m_owner = this;
    m_categories.m_owner = this;
    m_list.m_slots[0] = &m_selected[0];
    m_list.m_slots[1] = &m_selected[1];
    m_list.m_slots[2] = &m_selected[2];
    m_list.m_slots[3] = &m_selected[3];
    m_list.m_slots[4] = &m_selected[4];

    zdb::CTexHandle *tex = zdb::CAssetMgr::m_assets.GetTexHandle("newweapnbkrnd.tif");

    m_bitmap1.Load(s_backx, s_titley, s_backx + s_width, s_titley + s_titleheight, tex);
    m_bitmap1.SetUV(0.0f, 1.0f, 1.0f, 0.0f);
    m_bitmap2.Load(s_listx, s_listy, s_listx + s_width, s_listy + s_listheight, tex);
    m_bitmap2.SetUV(0.0f, 1.0f, 1.0f, 0.0f);
    m_bitmap3.Load(0.0f, 0.0f, 0.0f, 0.0f, tex);
    m_bitmap3.SetUV(0.0f, 1.0f, 1.0f, 0.0f);
    theHUD->Add2D(&m_bitmap1, -1, true);
    theHUD->Add2D(&m_bitmap2, -1, true);
    theHUD->Add2D(&m_bitmap3, -1, true);
    theHUD->Add2D(&m_title, -1, true);
    theHUD->Add2D(&m_list, -1, true);
    m_bitmap1.SetTrans(0.0f);
    m_bitmap2.SetTrans(0.0f);
    m_bitmap3.SetTrans(0.0f);
    m_title.SetTrans(0.0f);
    m_list.SetTrans(0.0f);

    CZSealBody *owner = CHUD::GetHUDOwner();
    EQUIP_ITEM primary = owner ? owner->m_kit.GetPrimaryEnum() : EQUIP_NONE;

    if (CZFTSWeapon::IsM203Weapon(primary) || primary == (EQUIP_ITEM)142) {
        Set203(true);
        SetGL(false);
        SetF2000(false);
    } else if (primary == (EQUIP_ITEM)143) {
        SetGL(true);
        Set203(false);
        SetF2000(false);
    } else if (primary == (EQUIP_ITEM)63) {
        SetF2000(true);
        Set203(false);
        SetGL(false);
    } else {
        Set203(false);
        SetGL(false);
        SetF2000(false);
    }
}

/* off, and closed */
void CMPWeaponSel::UnInit()
{
    Off();
    func_00240B90(this, 0);
}

void CWeaponSlot::SetTrans(float trans)
{
}

void SlotList::SetTrans(float trans)
{
    for (unsigned int i = 0; i < 5; i++)
        if (m_slots[i])
            m_slots[i]->SetTrans(trans);
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

/* the name and the bitmap faded together */
void CategorySlot::SetTrans(float trans)
{
    m_name.SetTrans(trans);
    m_bitmap.SetTrans(trans);
}

/* the slots drawn while the list is on */
void SlotList::Draw(zdb::CCamera *cam)
{
    if (IsOn())
        for (int i = 0; i < 5; i++)
            if (m_slots[i])
                m_slots[i]->Draw(cam);
}

/* the slots' text: the weapon's description, upper-cased into a buffer of its own, at 0.9 (each slot
 * class its buffer, bss 0x414810 and 0x414860), and the slots' offsets (the unit's 8-byte-spaced
 * .data, 0x3DCE28 and 0x3DCEB8-0x3DCED0: variables, loaded with lwc1; the names ours) */
static float s_slotscale = 0.9f;
static float s_catslotx = 4.0f;
static float s_catsloty = 2.0f;
static float s_selslotx = 2.0f;
static float s_selsloty = 2.0f;
static char s_catname[80];
static char s_selname[80];

/* nothing drawn */
void CWeaponSlot::Draw(zdb::CCamera *cam)
{
}

/* the weapon kept */
void CWeaponSlot::SetWeapon(CZWeapon *weapon)
{
    m_weapon = weapon;
}

/* nothing to place */
void CWeaponSlot::SetPos(float x, float y)
{
}

/* the bitmap at the place and the name 4 above it, both at the slot's offset */
void CategorySlot::SetPos(float x, float y)
{
    m_bitmap.SetPos((int)(x + s_catslotx), (int)(y + s_catsloty));
    m_name.SetPos((int)(x + s_catslotx), (int)(y + s_catsloty - 4.0f));
}

/* the name's colour */
void CategorySlot::SetColor(float r, float g, float b)
{
    m_name.SetColor(r, g, b);
}

/* the bitmap and the name, each while on */
void CategorySlot::Draw(zdb::CCamera *cam)
{
    if (m_bitmap.IsOn())
        m_bitmap.Draw(cam);
    if (m_name.IsOn())
        m_name.Draw(cam);
}

/* the weapon kept; none, the bitmap and the name off; else both on, the bitmap the weapon's icon
 * where it is, the name its description where the name is (its place taken before the text) */
void CategorySlot::SetWeapon(CZWeapon *weapon)
{
    m_weapon = weapon;
    if (!weapon) {
        m_bitmap.Off();
        m_name.Off();
    } else {
        m_name.On();
        m_bitmap.On();
        m_bitmap.Load((float)m_bitmap.m_x, (float)m_bitmap.m_y, weapon->m_icon);

        const char *desc;
        int x = (int)(m_name.m_posx + m_name.m_socom2_7c);
        int y = (int)(m_name.m_posy + m_name.m_socom2_80);
        desc = weapon->GetDescription();

        strlen(desc);
        strncpy(s_catname, desc, 80);
        CHUD::HudText_to_upper(s_catname);
        m_name.Load(s_slotscale, s_catname, theHUD->m_myfont, (float)x, (float)y, true);
    }
}

/* the name, while on */
void SelectedSlot::Draw(zdb::CCamera *cam)
{
    if (m_name.IsOn())
        m_name.Draw(cam);
}

/* the weapon kept; none, the name off; else on, the weapon's description where the name is */
void SelectedSlot::SetWeapon(CZWeapon *weapon)
{
    m_weapon = weapon;
    if (!weapon) {
        m_name.Off();
    } else {
        m_name.On();

        const char *desc;
        int x = (int)(m_name.m_posx + m_name.m_socom2_7c);
        int y = (int)(m_name.m_posy + m_name.m_socom2_80);
        desc = weapon->GetDescription();

        strlen(desc);
        strncpy(s_selname, desc, 80);
        CHUD::HudText_to_upper(s_selname);
        m_name.Load(s_slotscale, s_selname, theHUD->m_myfont, (float)x, (float)y, true);
    }
}

/* the name at the place, at the slot's offset */
void SelectedSlot::SetPos(float x, float y)
{
    m_name.SetPos((int)(x + s_selslotx), (int)(y + s_selsloty));
}

/* the name's colour */
void SelectedSlot::SetColor(float r, float g, float b)
{
    m_name.SetColor(r, g, b);
}

/* the name faded */
void SelectedSlot::SetTrans(float trans)
{
    m_name.SetTrans(trans);
}
