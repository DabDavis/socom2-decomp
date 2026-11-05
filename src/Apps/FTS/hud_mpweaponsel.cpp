/* hud_mpweaponsel.cpp -- the network game's weapon selector (CMPWeaponSel, SOCOM II's: no vtable,
 * no RTTI name; the class's and the unit's names are ours). Its slots' vtables (CWeaponSlot,
 * CategorySlot, SelectedSlot, SlotList) put it after the mission cams, OVL1 ...-0x241BDF, the
 * constructor last as in SOCOM 1's selector units. Written: Off (0x23E700), On (0x23E800) and
 * UnInit (0x240D50), which inlines Off; round 26: the slot classes' own virtuals (CWeaponSlot's
 * SetWeapon/SetPos at +0x70/+0x74, CategorySlot's and SelectedSlot's overrides and their SetColor
 * at +0x78, the three Draws: 0x23BF10-0x23C387, defined here, not inline -- they sit mid-unit in
 * reverse of their definition), the category page's SetWeapon and ShowArrows, the selector's
 * Scroll and SlotList::NextClass. The layout is hud_weaponsel.h's.
 * The initialiser (0x3FF7F0) is the file statics below, defined in address order (round 20).
 * Defined in reverse of their image order (-inline deferred). */

#include "../../gamez/ztwod/ztwod.h"
#include "../../gamez/zinput/zinput.h"
#include "hud_weaponsel.h"

/* a button's state as the controller unit reads it (OVL1 0x2C64E0, by address: hud_tcmtree.cpp's
 * TCM_BUTTON, a byte) */
extern "C" KEY_STATE func_002C64E0(int button, CPad *pad);


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

/* a character type's weapons (zchar.h's CCharacterWeap and CCharacterWeapList: a 12-byte weapon
 * and its ammunition, the vector of them; the AI core's header reaches zweapon.h, whose CZWeapon
 * this unit stands in for) */
class CCharacterWeap {
public:
    void SetupCharacterWeapon(EQUIP_ITEM item);

    EQUIP_ITEM m_wepId;   // 0x0
    unsigned char m_ammoId;   // 0x1
    int m_ammoCount;   // 0x4
    int m_magCount;   // 0x8
};

class CCharacterWeapList : public std::vector<CCharacterWeap> {
public:
    void AddWeapon(EQUIP_ITEM item);
};

class CCharacterType {
public:
    EQUIP_ITEM GetWeaponByIndex(unsigned int i) const;

    unsigned char _0[0x2e8];
    CCharacterWeapList m_weapons;   // 0x2e8
};

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

/* the seal's controller: SOCOM 1's SetMenuState with a flag added (OVL2) */
class CSealCtrl {
public:
    void SetMenuState(MENU_STATE state, bool flag);
};

class CZSealBody {
public:
    CCharacterType *GetCharacter();

    unsigned char _0[0xc0];
    CSealCtrl *m_ctrl;   // 0xc0
    unsigned char _c4[0x5e0 - 0xc4];
    CZKit m_kit;   // 0x5e0
};

class CHUD {
public:
    static CZSealBody *GetHUDOwner();
    void Add2D(C2D *p2d, int phase, bool add_to_back);
    static void HudText_to_upper(char *instr);
    static PNT4D m_hud_textcolor;   /* the HUD's text colour */

    unsigned char _0[0x498];
    C2DFont *m_myfont;   // 0x498
};

extern CHUD *theHUD;

/* the weapons' list (zweapon's CZWeaponList: a weapon's index, the weapon of an item) */
class CZWeaponList {
public:
    int GetWeaponIndex(CZWeapon *weapon);
    CZWeapon *GetWeapon(EQUIP_ITEM item);
    CZWeapon *GetWeaponByIndex(int index);

    int m_count;   // 0x0
};

class CValve;

/* a weapon as its slot shows it: its description and its icon, its item and its class
 * (zweapon's CZWeapon) */
class CZWeapon {
public:
    const char *GetDescription() const;
    EQUIP_ITEM GetIDType();
    static bool IsPrimary(EQUIP_ITEM item);
    static bool IsSecondary(EQUIP_ITEM item);
    static bool IsLauncher(EQUIP_ITEM item);
    static CValve *GetEnableValve(EQUIP_ITEM item);

    static CZWeaponList m_listWeapons;

    unsigned char _0[0x7c];
    EQUIP_ITEM m_item;   // 0x7c
    unsigned char _7d[0xbc - 0x7d];
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
/* the category page's title backdrop: its top and height (CMPWeaponCategories::Init, 0x23D860) */
static float s_cattitley = 105.0f;
static float s_cattitleheight = 35.0f;
/* the layout's further places (.data 0x3DCDD0-0x3DCE20, 8 bytes apart: variables, read with lwc1;
 * the names ours, by address but the title's) -- the selector's title backdrop top and height (its
 * Init and Layout: the category page's are the two above, the same values), the title's scale and
 * line, the cursor bar's top, height and step, the slots' left and first line (Layout) */
static float s_dd0 = 1.0f;
static float s_dd8 = 129.0f;
static float s_de0 = 18.0f;
static float s_de8 = 146.0f;
static float s_titley = 105.0f;
static float s_df8 = 129.0f;
static float s_titleheight = 35.0f;
static float s_e08 = 1.0f;
static float s_e10 = 20.0f;
static float s_e18 = 10.0f;
static float s_e20 = 159.0f;

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

/* the members built in place: the title, the backdrops, the category page (its polys, list and
 * slots through __construct_array), the list and the five selected slots (0x241A90; CHUD's
 * constructor calls it -- the unit's last function, so defined first) */
CMPWeaponSel::CMPWeaponSel() : m_1538(1.0f), m_153c(0)
{
}

/* closed; the slots listed; the backdrops loaded, added to the HUD's phases and faded out; the
 * grenades of the owner's primary weapon let through, the others' not */
void CMPWeaponSel::Init()
{
    m_state = SEL_CLOSED;
    m_list.m_34 = 0;
    m_1530 = 255;
    m_1531 = 255;
    m_list.Off();
    m_categories.Init();
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
    SetState(SEL_CLOSED);
}

/* the selector's state changed: the list (2) fills its slots from the owner's weapons when opened
 * from closed and hides the page's backdrop; the page (3) comes on, updated twice, showing the
 * list's slot; closed (0) hides the backdrop and gives the owner's controller its menu back -- the
 * state kept; 1 (chosen) and anything else change nothing (0x240B90; the name ours) */
void CMPWeaponSel::SetState(SEL_STATE state)
{
    switch (state) {
    case SEL_LIST:
        if (!m_state)
            Open();
        m_categories.m_bitmap3.Off();
        break;
    case SEL_PAGE: {
        m_categories.On();
        m_categories.m_bitmap3.On();
        func_0023D860(&m_categories, 0.0f);
        m_categories.m_state = SEL_PAGE;

        /* through a slot pointer, as Scroll's (indexed straight, the offset is added the other
         * way round) */
        SelectedSlot *slot = &m_selected[m_list.m_34];

        m_categories.SetWeapon(slot->m_weapon);
        break;
    }
    case SEL_CLOSED:
        m_categories.m_bitmap3.Off();
        if (m_state) {
            CZSealBody *owner = CHUD::GetHUDOwner();

            if (owner)
                owner->m_ctrl->SetMenuState(stateNone, false);
        }
        break;
    case SEL_CHOSEN:
    default:
        return;
    }
    m_state = state;
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


/* the list's buttons: up and down scroll the slots, the confirm button (0x2C64E0's 0) opens the
 * category page, triangle, start or 0x2C64E0's 4 close the selector (0x23EBC0; the name ours --
 * m2c's draft matched as it stood) */
void CMPWeaponSel::HandlePad(CPad *pad)
{
    if (pad->m_state[PAD_UP] == KEY_FALLING) {
        Scroll(-2);
        return;
    }
    if (pad->m_state[PAD_DOWN] == KEY_FALLING) {
        Scroll(2);
        return;
    }
    if (func_002C64E0(0, pad) == KEY_FALLING) {
        SetState(SEL_PAGE);
        return;
    }
    if (pad->m_state[PAD_TRIANGLE] == KEY_FALLING || pad->m_state[PAD_START] == KEY_FALLING ||
        func_002C64E0(4, pad) == KEY_FALLING)
        SetState(SEL_CLOSED);
}

/* the cursor a slot on or back, round the five, again while the slot is refused; the page then
 * shows the slot's weapon, updated. Pinned: the target calls itself and the page's SetWeapon out
 * of line (unpinned it inlines itself once, 0x224 bytes) */
#pragma auto_inline off
void CMPWeaponSel::Scroll(signed char dir)
{
    if (dir == 2) {
        m_list.m_34++;
        if (m_list.m_34 >= 5)
            m_list.m_34 = 0;
    } else if (dir == -2) {
        m_list.m_34--;
        if (m_list.m_34 < 0)
            m_list.m_34 = 4;
    }
    if (!CanSelect(m_list.m_34))
        Scroll(dir);

    /* through a slot pointer (indexed straight, MW adds the offset to `this` the other way round) */
    SelectedSlot *slot = &m_selected[m_list.m_34];

    m_categories.SetWeapon(slot->m_weapon);
    Layout(0.0f);
}
#pragma auto_inline reset

/* the HUD owner's side (fts code, 0x2C30B0: a null-tested seal; no row named yet) */
extern "C" bool func_002C30B0(CZSealBody *seal);

/* the item in one of the equipment slots (2-4) but the one asked about, its index, or -1 */
inline int CMPWeaponSel::FindEquipment(EQUIP_ITEM item, int skip)
{
    for (int i = 2; i < 5; i++) {
        if (i != skip && m_selected[i].m_weapon && item == m_selected[i].m_weapon->m_item)
            return i;
    }
    return -1;
}

/* items 141 and 254 never chosen, 185 and 186 only when another equipment slot holds the same
 * (by value: the item's one `lb` serves this switch and IsLocked's, each masking it) */
inline void CMPWeaponSel::CheckItem(EQUIP_ITEM item, int index, bool &ok)
{
    switch (item) {
    case 254:
    case 141:
        ok = false;
        break;
    case 186:
        ok = FindEquipment((EQUIP_ITEM)186, index) != -1;
        break;
    case 185:
        ok = FindEquipment((EQUIP_ITEM)185, index) != -1;
        break;
    }
}

/* the C4 locked out for the owner's side by the server's valve (16 one side, 32 the other; the
 * one-case switch is the target's layout: the null preset in the compare's delay slot, a `b` over
 * the case) */
static inline bool IsLocked(CZWeapon *weapon, EQUIP_ITEM item)
{
    if (!weapon)
        return false;
    CValve *valve = 0;

    switch (item) {
    case 151:
        valve = CValve::GetByName("Enable_c4");
        break;
    }
    CZSealBody *owner = CHUD::GetHUDOwner();
    int bit = 16;
    if (func_002C30B0(owner))
        bit = 32;
    return (valve ? bit & valve->m_value : 0) != 0;
}

/* whether a slot can be chosen: an empty one can; items 141 and 254 never; 185 and 186 only
 * when another equipment slot holds the same; the C4 not while its valve locks it out
 * (0x23E910; the name ours -- Scroll and the selector's update 0x23E030 call it). The slot's
 * weapon is read straight each time: a CSE temporary, numbered after the inlines' item, takes a3
 * where a named `CZWeapon *weapon` (or a slot pointer or reference) swaps it with the item's t0 */
bool CMPWeaponSel::CanSelect(int index)
{
    bool ok = true;

    if (m_selected[index].m_weapon) {
        CheckItem(m_selected[index].m_weapon->m_item, index, ok);
        if (IsLocked(m_selected[index].m_weapon, m_selected[index].m_weapon->m_item))
            ok = false;
    }
    return ok;
}

/* while up: the lists, the page, the title and the backdrops on, then updated */
void CMPWeaponSel::On()
{
    if (m_state) {
        m_list.On();
        m_categories.On();
        m_title.On();
        m_bitmap1.On();
        m_bitmap2.On();
        m_bitmap3.On();
        Layout(0.0f);
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

/* opened on the list: the slots emptied and given the owner's character's first five weapons (one
 * read again when it reads 0), the page shown the list's slot, the cursor round to the first
 * slot it allows, the selector on and updated, the owner's controller in the weapon menu, and the
 * lists told their owner (0x23E370; the name ours -- SetState's list case calls it from closed) */
void CMPWeaponSel::Open()
{
    CZSealBody *owner = CHUD::GetHUDOwner();

    if (!owner)
        return;
    m_1530 = 255;
    m_1531 = 255;

    CZWeaponList &list = CZWeapon::m_listWeapons;
    CCharacterType *character = owner->GetCharacter();
    int count = character->m_weapons.size();
    int i;

    for (i = 0; i < 5; i++)
        m_selected[i].SetWeapon(0);
    for (i = 0; i < 5 && i < count; i++) {
        EQUIP_ITEM item = character->GetWeaponByIndex(i);

        if (!(item > 0))
            item = character->GetWeaponByIndex(i);
        m_selected[i].SetWeapon(list.GetWeapon(item));
    }

    SelectedSlot *slot = &m_selected[m_list.m_34];

    m_categories.SetWeapon(slot->m_weapon);
    m_list.m_34 = -1;
    Scroll(2);
    On();
    Layout(0.0f);
    owner->m_ctrl->SetMenuState(stateWeaponSel, false);
    m_categories.m_list.m_owner = this;
    m_categories.m_owner = this;
    m_state = SEL_LIST;
}

/* the selector laid out: the title's backdrop, the list's and the cursor's bar at its slot, each
 * coloured and translucent; the five slots placed down the list, grey when they cannot be chosen,
 * else in the HUD's text colour; the title centred over its backdrop (0x23E030; the name ours --
 * Scroll, On and Open call it with 0) */
void CMPWeaponSel::Layout(float dT)
{
    m_bitmap1.SetPos((int)s_backx, (int)s_titley, (int)(s_backx + s_width), (int)(s_titley + s_titleheight));
    m_bitmap1.SetColor(s_color1.x, s_color1.y, s_color1.z);
    m_bitmap1.SetIsTrans(true);
    m_bitmap2.SetPos((int)s_listx, (int)s_listy, (int)(s_listx + s_width), (int)(s_listy + s_listheight));
    m_bitmap2.SetColor(s_color2.x, s_color2.y, s_color2.z);
    m_bitmap2.SetIsTrans(true);
    m_bitmap3.SetColor(s_color4.x, s_color4.y, s_color4.z);
    m_bitmap3.SetIsTrans(true);

    float y = s_de8 + (float)m_list.m_34 * s_e10;

    m_bitmap3.SetPos((int)s_768, (int)y, (int)(s_768 + s_750), (int)(y + s_de0));
    for (int i = 0; i < 5; i++) {
        m_selected[i].SetPos(s_e18, s_e20 + s_e10 * (float)i);
        if (!CanSelect(i)) {
            m_selected[i].SetColor(40.0f, 40.0f, 40.0f);
        } else {
            PNT4D color = CHUD::m_hud_textcolor;

            m_selected[i].SetColor(color.x, color.y, color.z);
        }
    }
    m_title.Load(s_e08, "WEAPON EXCHANGE", theHUD->m_myfont, s_backx + s_width / 2.0f, s_df8, true);
    m_title.SetColor(s_color6.x, s_color6.y, s_color6.z);
    m_title.m_isCentered = 1;
    m_title.UpdatePos();
}

/* the five selected slots' weapons given to the HUD owner's character: the weapons it has set up
 * again, the rest pushed and set up unless empty (0x23E5E0; the name ours -- its one caller,
 * 0x240600, is unwritten) */
void CMPWeaponSel::SetCharacterWeapons()
{
    CCharacterWeapList &weapons = CHUD::GetHUDOwner()->GetCharacter()->m_weapons;
    int i;

    for (i = 0; i < 5; i++) {
        if (i >= weapons.size())
            break;
        weapons[i].SetupCharacterWeapon(m_selected[i].m_weapon->m_item);
    }
    for (; i < 5; i++) {
        CZWeapon *weapon = m_selected[i].m_weapon;
        EQUIP_ITEM item;

        weapons.AddWeapon(weapon->m_item);
        item = weapon->m_item;
        if (item != EQUIP_NONE)
            weapons[i].SetupCharacterWeapon(item);
    }
}

/* the name and the bitmap faded together */
void CategorySlot::SetTrans(float trans)
{
    m_name.SetTrans(trans);
    m_bitmap.SetTrans(trans);
}

/* the weapon's index and class, and whether it is a primary (0), a secondary (1) or neither (2);
 * none (item 255) shows item 121's index as neither */
#pragma auto_inline off
void CMPWeaponCategories::SetWeapon(CZWeapon *weapon)
{
    CZWeaponList &list = CZWeapon::m_listWeapons;

    m_index = list.GetWeaponIndex(weapon);
    m_type = weapon->GetIDType();
    if (CZWeapon::IsPrimary(weapon->m_item))
        m_kind = KIND_PRIMARY;
    else if (CZWeapon::IsSecondary(weapon->m_item))
        m_kind = KIND_SECONDARY;
    else
        m_kind = KIND_OTHER;
    if (weapon->m_item == EQUIP_NONE) {
        m_type = EQUIP_NONE;
        m_index = list.GetWeaponIndex(list.GetWeapon((EQUIP_ITEM)121));
        m_kind = KIND_OTHER;
    }
}
#pragma auto_inline reset

/* the arrows' pulse, in colour blends a second (0x3DCEA8, the unit's float pool; the name ours) */
static float s_arrowpulserate = 2.0f;

/* the page's arrows pulsed between grey and yellow: the blend runs up to 1 and back down to 0, two
 * a second, each arrow given the blended colour (0x23D2B0; the name ours -- the selector's tick
 * 0x240600 calls it) */
void CMPWeaponCategories::TickArrows(float dT)
{
    static CPnt3D s_lo(80.0f, 80.0f, 80.0f);
    static CPnt3D s_hi(255.0f, 255.0f, 128.0f);

    if (m_pulseup) {
        m_pulse += dT * s_arrowpulserate;
        if (m_pulse >= 1.0f) {
            m_pulse = 1.0f;
            m_pulseup = false;
        }
    } else {
        m_pulse -= dT * s_arrowpulserate;
        if (m_pulse <= 0.0f) {
            m_pulse = 0.0f;
            m_pulseup = true;
        }
    }

    float r = s_lo.x + m_pulse * (s_hi.x - s_lo.x);
    float g = s_lo.y + m_pulse * (s_hi.y - s_lo.y);
    float b = s_lo.z + m_pulse * (s_hi.z - s_lo.z);

    for (int i = 0; i < 4; i++)
        m_polys[i].SetColor(r, g, b);
}

/* whether the page's arrows are hidden (bss 0x4147C0, a byte; the name ours) */
static bool s_noarrows;

/* the page's four arrows off when hidden, else grey (a guarded static, 100 each) */
void CMPWeaponCategories::ShowArrows()
{
    if (s_noarrows) {
        for (int i = 0; i < 4; i++)
            m_polys[i].Off();
        return;
    }

    static CPnt3D s_grey(100.0f, 100.0f, 100.0f);

    for (int i = 0; i < 4; i++)
        m_polys[i].SetColor(s_grey.x, s_grey.y, s_grey.z);
}

/* while the page is up (state 3): up and down step the weapon, left and right the class; triangle
 * or start back to the list (2), the confirm button (0x2C64E0's 0) on to state 1 (0x23D1B0; the
 * name ours -- m2c's draft matched as it stood) */
void CMPWeaponCategories::HandlePad(CPad *pad)
{
    if (m_state == SEL_PAGE) {
        if (pad->m_state[PAD_UP] == KEY_FALLING) {
            Move(-2);
            return;
        }
        if (pad->m_state[PAD_DOWN] == KEY_FALLING) {
            Move(2);
            return;
        }
        if (pad->m_state[PAD_LEFT] == KEY_FALLING) {
            Move(1);
            return;
        }
        if (pad->m_state[PAD_RIGHT] == KEY_FALLING) {
            Move(-1);
            return;
        }
        if (pad->m_state[PAD_TRIANGLE] == KEY_FALLING || pad->m_state[PAD_START] == KEY_FALLING) {
            m_state = SEL_LIST;
            return;
        }
        if (func_002C64E0(0, pad) == KEY_FALLING)
            m_state = SEL_CHOSEN;
    }
}


/* the category page made: its arrows' pulse reset, everything off, its two backdrops loaded at the
 * page's place and the third empty, all added to the HUD's phase 1 and faded out; no class, the
 * primaries' kind, the list's four category slots, and the list's first weapon (0x23CD20; the name
 * ours -- the selector's Init calls it) */
void CMPWeaponCategories::Init()
{
    m_pulse = 0.0f;
    m_pulseup = true;
    Off();

    zdb::CTexHandle *tex = zdb::CAssetMgr::m_assets.GetTexHandle("newweapnbkrnd.tif");

    m_bitmap1.Load(s_708, s_cattitley, s_708 + s_700, s_cattitley + s_cattitleheight, tex);
    m_bitmap1.SetUV(0.0f, 1.0f, 1.0f, 0.0f);
    m_bitmap2.Load(s_catx, s_caty, s_catx + s_700, s_caty + s_db8, tex);
    m_bitmap2.SetUV(0.0f, 1.0f, 1.0f, 0.0f);
    m_bitmap3.Load(0.0f, 0.0f, 0.0f, 0.0f, tex);
    m_bitmap3.SetUV(0.0f, 1.0f, 1.0f, 0.0f);
    theHUD->Add2D(&m_bitmap1, 1, true);
    theHUD->Add2D(&m_bitmap2, 1, true);
    theHUD->Add2D(&m_title, 1, true);

    int i;

    for (i = 0; i < 4; i++)
        theHUD->Add2D(&m_polys[i], 1, true);
    theHUD->Add2D(&m_bitmap3, 1, true);
    theHUD->Add2D(&m_list, 1, true);
    m_bitmap1.SetTrans(0.0f);
    m_bitmap2.SetTrans(0.0f);
    m_bitmap3.SetTrans(0.0f);
    m_title.SetTrans(0.0f);
    for (unsigned int j = 0; j < 4; j++)
        m_polys[j].SetTrans(0.0f);
    m_list.SetTrans(0.0f);
    m_type = (EQUIP_ITEM)255;
    m_kind = KIND_PRIMARY;
    m_state = SEL_CLOSED;
    m_list.m_slots[0] = &m_categories[0];
    m_list.m_slots[1] = &m_categories[1];
    m_list.m_slots[2] = &m_categories[2];
    m_list.m_slots[3] = &m_categories[3];
    m_index = m_list.NextWeapon(true, 0, m_type, m_kind);
}

/* the page's cursor moved: a weapon on (2) or back (-2) in its class, or the class on (-1) or
 * back (1) -- the grenades' class (171) passed over when the selector lets none through -- and on
 * a new class its first weapon; then the page updated (0x23D060; the name ours) */
void CMPWeaponCategories::Move(signed char dir)
{
    bool up = false;

    switch (dir) {
    case 2:
        up = true;
    case -2:
        m_index = m_list.NextWeapon(up, m_index, m_type, m_kind);
        break;
    case -1:
        up = true;
    case 1: {
        int old = m_type;

        m_type = m_list.NextCategory(up, m_type, m_kind);
        if (m_type == 171 && !m_owner->m_203 && !m_owner->m_gl && !m_owner->m_f2000)
            m_type = m_list.NextCategory(up, m_type, m_kind);
        if (old != m_type)
            m_index = m_list.NextWeapon(true, 0, m_type, m_kind);
        break;
    }
    }
    func_0023D860(this, 0.0f);
}

/* the slots drawn while the list is on */
void SlotList::Draw(zdb::CCamera *cam)
{
    if (IsOn())
        for (int i = 0; i < 5; i++)
            if (m_slots[i])
                m_slots[i]->Draw(cam);
}

/* whether an id is the first of a weapon class (GetIDType's sixteen classes; the name ours: the
 * target materialises the switch's answer, an inlined bool) */
static inline bool IsClassStart(int id)
{
    bool start = false;

    switch (id) {
    case 4:
    case 31:
    case 51:
    case 81:
    case 91:
    case 101:
    case 121:
    case 141:
    case 145:
    case 151:
    case 171:
    case 185:
    case 190:
    case 201:
    case 205:
    case 230:
        start = true;
    }
    return start;
}

/* a step up or down, then on, round 0-254, to the first id of a class */
EQUIP_ITEM SlotList::NextClass(bool up, EQUIP_ITEM cls)
{
    int id = cls;

    if (up)
        id++;
    else
        id--;
    while (!IsClassStart(id)) {
        if (up)
            id++;
        else
            id--;
        if (id > 254)
            id = 0;
        if (id < 0)
            id = 254;
    }
    return (EQUIP_ITEM)id;
}


/* whether the list takes a weapon of a class: none (255) always does; else any id from the class's
 * start to the next class's (inline; the name ours). The end an int: as the class's byte enum it is
 * numbered the other way round with the id (their saved registers swap) */
inline bool SlotList::HasWeapons(EQUIP_ITEM cls)
{
    if (cls == 255)
        return true;

    int end = NextClass(true, cls);

    for (int id = cls; id < end; id++) {
        if (CanTake(CZWeapon::m_listWeapons.GetWeapon((EQUIP_ITEM)id), cls, (WEAPON_KIND)3))
            return true;
    }
    return false;
}

/* the page's next weapon class up or down from a class, in the selector's fixed order -- the
 * primaries 31 51 81 91 101 141, the pistols 4, the others 121 145 151 171 185 190, 255 (none)
 * between the ends -- on past the classes the list takes nothing of (0x23C840; the name ours --
 * Move's class step). Pinned: the target calls itself out of line */
#pragma auto_inline off
EQUIP_ITEM SlotList::NextCategory(bool up, EQUIP_ITEM cls, WEAPON_KIND kind)
{
    EQUIP_ITEM next;

    switch (cls) {
    case 255:
        if (kind == KIND_PRIMARY)
            if (up)
                next = (EQUIP_ITEM)31;
            else
                next = (EQUIP_ITEM)141;
        else if (kind == KIND_SECONDARY)
            next = (EQUIP_ITEM)4;
        else
            if (up)
                next = (EQUIP_ITEM)121;
            else
                next = (EQUIP_ITEM)190;
        break;
    case 4:
        next = (EQUIP_ITEM)4;
        break;
    case 31:
        if (up)
            next = (EQUIP_ITEM)51;
        else
            next = (EQUIP_ITEM)255;
        break;
    case 51:
        if (up)
            next = (EQUIP_ITEM)81;
        else
            next = (EQUIP_ITEM)31;
        break;
    case 81:
        if (up)
            next = (EQUIP_ITEM)91;
        else
            next = (EQUIP_ITEM)51;
        break;
    case 91:
        if (up)
            next = (EQUIP_ITEM)101;
        else
            next = (EQUIP_ITEM)81;
        break;
    case 101:
        if (up)
            next = (EQUIP_ITEM)141;
        else
            next = (EQUIP_ITEM)91;
        break;
    case 141:
        if (up)
            next = (EQUIP_ITEM)255;
        else
            next = (EQUIP_ITEM)101;
        break;
    case 121:
        if (up)
            next = (EQUIP_ITEM)145;
        else
            next = (EQUIP_ITEM)255;
        break;
    case 145:
        if (up)
            next = (EQUIP_ITEM)151;
        else
            next = (EQUIP_ITEM)121;
        break;
    case 151:
        if (up)
            next = (EQUIP_ITEM)171;
        else
            next = (EQUIP_ITEM)145;
        break;
    case 171:
        if (up)
            next = (EQUIP_ITEM)185;
        else
            next = (EQUIP_ITEM)151;
        break;
    case 185:
        if (up)
            next = (EQUIP_ITEM)190;
        else
            next = (EQUIP_ITEM)171;
        break;
    case 190:
        if (up)
            next = (EQUIP_ITEM)255;
        else
            next = (EQUIP_ITEM)185;
        break;
    case 201:
    case 205:
    case 230:
    default:
        return (EQUIP_ITEM)255;
    }
    if (!HasWeapons(next))
        next = NextCategory(up, next, kind);
    return next;
}
#pragma auto_inline reset

/* where a search for the next weapon started (.data 0x3DCEB0, -1 between searches; the name ours) */
static int s_searchstart = -1;

/* the next weapon's index up or down the weapons' list, round it, on past the weapons the slot
 * refuses, back where the search started; none without the HUD's owner. Pinned: the target calls
 * itself out of line. The stepped index is a variable of its own, `next = index + 1` after the
 * list: numbered after the list it takes s0 as the target's does (round 49, open since round 26 --
 * the parameter stepped in place is numbered with the parameters, s3, and a plain copy `next =
 * index` coalesces back into it). The first GetWeaponByIndex re-materialises the list's address,
 * the last uses the saved one: the source spells them apart */
#pragma auto_inline off
int SlotList::NextWeapon(bool up, int index, EQUIP_ITEM type, WEAPON_KIND kind)
{
    if (index == s_searchstart)
        return index;
    if (s_searchstart == -1)
        s_searchstart = index;
    if (!CHUD::GetHUDOwner()) {
        s_searchstart = -1;
        return 0;
    }

    CZWeaponList &list = CZWeapon::m_listWeapons;
    int next;

    if (up) {
        next = index + 1;
        if (next >= list.m_count)
            next = 0;
    } else {
        next = index - 1;
        if (next < 0)
            next = list.m_count - 1;
    }
    if (!CanTake(CZWeapon::m_listWeapons.GetWeaponByIndex(next), type, kind))
        next = NextWeapon(up, next, type, kind);
    list.GetWeaponByIndex(next);
    s_searchstart = -1;
    return next;
}
#pragma auto_inline reset

/* whether a selected slot other than the cursor's holds a weapon of the class, or the weapon itself;
 * the cursor's slot's item (255 for none) */
inline bool CMPWeaponSel::HasClassElsewhere(EQUIP_ITEM cls)
{
    for (int i = 0; i < 5; i++) {
        if (m_selected[i].m_weapon->GetIDType() == cls && i != m_list.m_34)
            return true;
    }
    return false;
}

inline bool CMPWeaponSel::IsSelectedElsewhere(CZWeapon *weapon)
{
    for (int i = 0; i < 5; i++) {
        if (m_selected[i].m_weapon == weapon && i != m_list.m_34)
            return true;
    }
    return false;
}

inline EQUIP_ITEM CMPWeaponSel::CursorItem()
{
    SelectedSlot *slot = &m_selected[m_list.m_34];

    return slot->m_weapon ? slot->m_weapon->m_item : (EQUIP_ITEM)255;
}

/* the items a list may show again where another slot holds them (the names ours) */
static inline bool CanRepeat(EQUIP_ITEM item)
{
    switch (item) {
    case 146:
    case 145:
    case 185:
    case 153:
    case 158:
    case 195:
    case 194:
        return false;
    }
    return true;
}

/* whether the list's slot takes a weapon: of the kind (primary, secondary, or neither) and the class
 * asked for (255 any); 195 only beside a class-101 weapon in another slot, 186 only where another
 * slot holds it and not when the cursor's slot holds 146; a weapon only once unless CanRepeat; and
 * its valve open to the owner's side (bit 8 on the side 0x2C30B0 names, else bit 1) (0x23C390; the
 * name ours -- NextWeapon's test) */
bool SlotList::CanTake(CZWeapon *weapon, EQUIP_ITEM type, WEAPON_KIND kind)
{
    if (!weapon)
        return false;
    switch (kind) {
    case KIND_PRIMARY:
        if (!CZWeapon::IsPrimary(weapon->m_item))
            return false;
        break;
    case KIND_SECONDARY:
        if (!CZWeapon::IsSecondary(weapon->m_item))
            return false;
        break;
    case KIND_OTHER:
        if (CZWeapon::IsPrimary(weapon->m_item) || CZWeapon::IsSecondary(weapon->m_item))
            return false;
        break;
    }

    EQUIP_ITEM item = weapon->m_item;
    EQUIP_ITEM cls = weapon->GetIDType();

    if (type != 255 && cls != type)
        return false;
    if (m_owner && !m_owner->HasClassElsewhere((EQUIP_ITEM)101) && item == 195)
        return false;
    if (m_owner && !m_owner->IsSelectedElsewhere(weapon) && item == 186)
        return false;
    if (m_owner && m_owner->CursorItem() == 146 && item == 186)
        return false;
    if (m_owner && !CanRepeat(weapon->m_item) && m_owner->IsSelectedElsewhere(weapon))
        return false;

    CValve *valve = CZWeapon::GetEnableValve(item);
    CZSealBody *owner = CHUD::GetHUDOwner();
    int bit = 1;

    if (func_002C30B0(owner))
        bit = 8;
    return (valve ? bit & valve->m_value : 0) != 0;
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

/* the equipment items a pick passes over when an equipment slot holds them (the names ours) */
static inline bool IsSharedEquipment(EQUIP_ITEM item)
{
    bool shared = false;

    switch (item) {
    case 194:
    case 154:
    case 152:
    case 195:
    case 254:
    case 151:
    case 255:
    case 141:
    case 193:
    case 175:
    case 171:
    case 172:
    case 173:
    case 174:
    case 179:
    case 143:
    case 142:
    case 183:
        shared = true;
    }
    return shared;
}

static inline bool IsPickedEquipment(EQUIP_ITEM item)
{
    bool picked = false;

    switch (item) {
    case 194:
    case 154:
    case 152:
    case 195:
    case 254:
    case 151:
    case 255:
    case 141:
    case 193:
        picked = true;
    }
    return picked;
}

/* an item's valve open to the player's side: bit 8 when player_team is 0, else bit 1 */
static inline bool IsEnabledFor(CValve *valve, bool team)
{
    int bit = team ? 8 : 1;

    return (valve ? bit & valve->m_value : 0) != 0;
}

#pragma auto_inline off
EQUIP_ITEM CMPWeaponSel::NextFreeEquipment(EQUIP_ITEM item, int index)
{
    if (FindEquipment(item, -1) != -1 && IsSharedEquipment(item)) {
        bool team = false;
        CZWeaponList &list = CZWeapon::m_listWeapons;
        int count = list.m_count;
        CValve *valve = CValve::GetByName("player_team");

        if (valve && valve->m_value == 0)
            team = true;
        if (index == -1) {
            for (int i = 0; i < count; i++) {
                if (list.GetWeaponByIndex(i)->m_item == item) {
                    index = i;
                    break;
                }
            }
        }

        bool ok = false;
        EQUIP_ITEM next;

        do {
            index++;
            if (index >= count)
                index = 0;
            next = list.GetWeaponByIndex(index)->m_item;
            if (!CZWeapon::IsPrimary(next) && !CZWeapon::IsSecondary(next) && !CZWeapon::IsLauncher(next)) {
                CValve *enable = CZWeapon::GetEnableValve(next);

                if (enable)
                    ok = IsEnabledFor(enable, team);
            }
        } while (!ok);
        item = NextFreeEquipment(next, index);
    }
    return item;
}

EQUIP_ITEM CMPWeaponSel::PickEquipment(int index, EQUIP_ITEM item, EQUIP_ITEM other)
{
    int found = FindEquipment(item, -1);
    bool picked = IsPickedEquipment(item);

    if (found != -1 && picked || item == 255) {
        bool team = false;
        CZWeaponList &list = CZWeapon::m_listWeapons;
        int count = list.m_count;
        CValve *valve = CValve::GetByName("player_team");

        if (valve && valve->m_value == 0)
            team = true;
        if (index == -1) {
            for (int i = 0; i < count; i++) {
                if (list.GetWeaponByIndex(i)->m_item == item && i != index) {
                    index = i;
                    break;
                }
            }
        }

        EQUIP_ITEM next;
        bool ok = false;

        do {
            index++;
            if (index >= count)
                index = 0;
            next = list.GetWeaponByIndex(index)->m_item;
            if (!CZWeapon::IsPrimary(next) && !CZWeapon::IsSecondary(next) && !CZWeapon::IsLauncher(next)
                && next != other) {
                CValve *enable = CZWeapon::GetEnableValve(next);

                if (enable)
                    ok = IsEnabledFor(enable, team);
            }
        } while (!ok);
        item = PickEquipment(index, next, (EQUIP_ITEM)255);
    }
    return item;
}
#pragma auto_inline reset

/* the name faded */
void SelectedSlot::SetTrans(float trans)
{
    m_name.SetTrans(trans);
}
