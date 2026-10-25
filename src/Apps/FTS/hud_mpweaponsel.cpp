/* hud_mpweaponsel.cpp -- the network game's weapon selector (CMPWeaponSel, SOCOM II's: no vtable,
 * no RTTI name; the class's and the unit's names are ours). Its slots' vtables (CWeaponSlot,
 * CategorySlot, SelectedSlot, SlotList) put it after the mission cams, OVL1 ...-0x241BDF, the
 * constructor last as in SOCOM 1's selector units. Written: Off (0x23E700), On (0x23E800) and
 * UnInit (0x240D50), which inlines Off. The layout is hud_weaponsel.h's.
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
};

extern CHUD *theHUD;

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
static float s_titley = 105.0f;
static float s_titleheight = 35.0f;
extern float data_00414770;

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

    m_bitmap1.Load(data_00414770, s_titley, data_00414770 + s_width, s_titley + s_titleheight, tex);
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

/* the name faded */
void SelectedSlot::SetTrans(float trans)
{
    m_name.SetTrans(trans);
}
