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
    unsigned char _0[0x2e8];
    CCharacterWeapList m_weapons;   // 0x2e8
};

class CZSealBody {
public:
    CCharacterType *GetCharacter();

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

/* the weapons' list (zweapon's CZWeaponList: a weapon's index, the weapon of an item) */
class CZWeaponList {
public:
    int GetWeaponIndex(CZWeapon *weapon);
    CZWeapon *GetWeapon(EQUIP_ITEM item);
    CZWeapon *GetWeaponByIndex(int index);

    int m_count;   // 0x0
};

/* a weapon as its slot shows it: its description and its icon, its item and its class
 * (zweapon's CZWeapon) */
class CZWeapon {
public:
    const char *GetDescription() const;
    EQUIP_ITEM GetIDType();
    static bool IsPrimary(EQUIP_ITEM item);
    static bool IsSecondary(EQUIP_ITEM item);

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
    func_0023E030(this, 0.0f);
}
#pragma auto_inline reset

/* the HUD owner's side (fts code, 0x2C30B0: a null-tested seal; no row named yet) */
extern "C" bool func_002C30B0(CZSealBody *seal);

/* the item in one of the equipment slots (2-4) but the one asked about, its index, or -1 */
inline int CMPWeaponSel::FindEquipment(EQUIP_ITEM item, int skip)
{
    for (int i = 2; i < 5; i++) {
        if (i != skip && m_selected[i].m_weapon && m_selected[i].m_weapon->m_item == item)
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
        m_kind = 0;
    else if (CZWeapon::IsSecondary(weapon->m_item))
        m_kind = 1;
    else
        m_kind = 2;
    if (weapon->m_item == EQUIP_NONE) {
        m_type = EQUIP_NONE;
        m_index = list.GetWeaponIndex(list.GetWeapon((EQUIP_ITEM)121));
        m_kind = 2;
    }
}
#pragma auto_inline reset

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

/* whether the list's slot can show a weapon (0x23C390; no row named yet) */
extern "C" bool func_0023C390(SlotList *list, CZWeapon *weapon, int a, int b);

/* where a search for the next weapon started (.data 0x3DCEB0, -1 between searches; the name ours) */
static int s_searchstart = -1;

/* the next weapon's index up or down the weapons' list, round it, on past the weapons the slot
 * refuses, back where the search started; none without the HUD's owner. Pinned: the target calls
 * itself out of line.
 * OPEN, 28 instructions, registers only (the code is the target's, size equal): the target numbers
 * the index LAST (s0; this s5, up s4, a s3, b s2, the list s1) where MW here numbers it with the
 * parameters (s3, the list s0). A local copy of the index declared after the list coalesces back
 * into the parameter (no change). Untried: the list through a pointer, the index's first read
 * moved, a temporary the recursive call's result is split into. The first GetWeaponByIndex
 * re-materialises the list's address, the last uses the saved one: the source spells them apart */
#pragma auto_inline off
int SlotList::NextWeapon(bool up, int index, int a, int b)
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

    if (up) {
        index++;
        if (index >= list.m_count)
            index = 0;
    } else {
        index--;
        if (index < 0)
            index = list.m_count - 1;
    }
    if (!func_0023C390(this, CZWeapon::m_listWeapons.GetWeaponByIndex(index), a, b))
        index = NextWeapon(up, index, a, b);
    list.GetWeaponByIndex(index);
    s_searchstart = -1;
    return index;
}
#pragma auto_inline reset

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
