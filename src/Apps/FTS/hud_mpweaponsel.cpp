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
