/* seal_kit.cpp -- the SEAL's kit, CZKit (SOCOM 1's unit, __sinit_seal_kit.cpp, with its
 * unnamed-namespace CSavedFireModeInfo). OVL2 0x5B9020-0x5C93FF, in SOCOM 1's order (the screen
 * offsets and the sniper's kick first, the constructor and the item lookups last). So far only the
 * small accessors. Defined in reverse of their image order (-inline deferred). */

#include "kit.h"

extern "C" {
/* CZSealBody::AdrenalineIncr (SOCOM 1's AdrenalineIncr__10CZSealBodyFf, tail-called from the same
 * InitializeSniperKick; OVL2 0x578150, the SEAL lane's row) */
void func_00578150(CZSealBody *body, float amount);
/* the weapon's grenade launcher: an M203 rifle, a launcher, or an item of class 145 (SOCOM 1's
 * IsM203Weapon and IsGrenadeLauncherWeapon in one; OVL1 0x3C5D80, unclaimed) */
bool func_003C5D80(CZFTSWeapon *w);
/* CZWeapon::GetIDType (SOCOM 1's GetIDType__8CZWeaponFv, called from the same GetCurItemIDType;
 * OVL1 0x3D1E10, unclaimed) */
EQUIP_IDTYPE func_003D1E10(CZFTSWeapon *w);
}

/* the item kinds of the first two items and the current one, 255 without one (OVL2 0x5C89D0,
 * 0x5C8980, 0x5C8930; the current one SOCOM 1's GetCurItemEnum, the first two new -- their names
 * ours) */
EQUIP_ITEM CZKit::GetCurItemEnum() const
{
    CZFTSWeapon *w = m_item[unwrap(m_selected_item, FIREMODE_CURRENT)];

    return w ? w->m_equip : EQUIP_LAST;
}

EQUIP_ITEM CZKit::GetPrimaryEnum() const
{
    CZFTSWeapon *w = m_item[unwrap(0, FIREMODE_CURRENT)];

    return w ? w->m_equip : EQUIP_LAST;
}

EQUIP_ITEM CZKit::GetSecondaryEnum() const
{
    CZFTSWeapon *w = m_item[unwrap(1, FIREMODE_CURRENT)];

    return w ? w->m_equip : EQUIP_LAST;
}

/* whether the kit holds an item of that kind (OVL2 0x5C86F0; SOCOM 1's) */
bool CZKit::HasItem(EQUIP_ITEM item) const
{
    for (int i = 0; i < m_num_items; i++)
        if (m_item[i] && item == m_item[i]->m_equip)
            return true;
    return false;
}

/* the first item of that kind, and its ammunition (OVL2 0x5C8640, new in SOCOM II: CSealCtrl's
 * OnActionButton and PerformProximityActions call it where SOCOM 1's called HasItem and then
 * ItemHasAmmo; the name ours) */
bool CZKit::HasItemWithAmmo(EQUIP_ITEM item)
{
    for (int i = 0; i < GetNumItems(); i++) {
        if (m_item[i] && item == m_item[i]->m_equip) {
            if (ItemHasAmmo(i))
                return true;
            break;
        }
    }
    return false;
}

/* a sniper rifle in hand, loaded (OVL2 0x5C85D0; SOCOM 1's, whose class was 100) */
bool CZKit::HasSniperRifle()
{
    if (GetCurItemIDType() == IDTYPE_SNIPER_RIFLE && CurItemHasAmmo())
        return true;
    return false;
}

/* the night vision: worn (the body's zoom class 3), allowed only on a night mission (OVL2
 * 0x5C80F0, 0x5C8080; SOCOM 1's) */
bool CZKit::NVG_Enabled() const
{
    return ((CKitBody *)m_body)->m_200 == 3;
}

void CZKit::NVG_Enabled(bool on)
{
    if (on) {
        if (zdb::CWorld::m_world && zdb::CWorld::m_world->m_night_mission)
            m_nvg_enabled = true;
    } else
        m_nvg_enabled = false;
}

/* the current item's id type, 254 without one (OVL2 0x5C8040; SOCOM 1's) */
EQUIP_IDTYPE CZKit::GetCurItemIDType()
{
    CZFTSWeapon *w = m_item[m_selected_item];
    EQUIP_IDTYPE r = IDTYPE_NONE;

    if (w)
        r = func_003D1E10(w);
    return r;
}

/* an item index wrapped into the kit, and under a grenade launcher's fire mode the launcher's
 * ammunition item instead (OVL2 0x5C6600; SOCOM 1's callers wrote this out after unwrap) */
int CZKit::unwrap(int index, FIREMODE mode) const
{
    index = wrap(index, m_num_items);
    int result = index;
    FIREMODE fm;

    if (mode == FIREMODE_CURRENT)
        fm = (FIREMODE)m_firemode[index];
    else
        fm = mode;
    CZFTSWeapon *w = m_item[index];

    if (w && func_003C5D80(w) && fm > AUTOFIRE) {
        int i = FindItemWithAmmo((EQUIP_ITEM)fm);

        if (i != -1)
            result = i;
    }
    return result;
}

/* an item's ammunition, and the current item's (OVL2 0x5C6970, 0x5C6920; new in SOCOM II, where
 * SOCOM 1's callers used GetWeapon -- the names ours) */
void *CZKit::GetCurAmmo()
{
    return GetAmmoByIndex(m_selected_item);
}

void *CZKit::GetAmmoByIndex(int index)
{
    int i = unwrap(index, FIREMODE_CURRENT);

    GetCurItem();
    return m_ammo[i];
}

/* the magazines of an item that hold rounds (OVL2 0x5C4970; SOCOM 1's) */
int CZKit::GetNumMags(int index)
{
    int n = 0;

    for (int i = 0; i < 10; i++)
        if (m_reloads[index][i] > 0)
            n++;
    return n;
}

/* (OVL2 0x5C3790; SOCOM 1's) */
FIREMODE CZKit::GetCurFireMode()
{
    return (FIREMODE)m_firemode[m_selected_item];
}

/* the current item (OVL2 0x5C3780; SOCOM II's goes through GetItemByIndex, 0x5C3740) */
CZFTSWeapon *CZKit::GetCurItem() const
{
    return GetItemByIndex(m_selected_item);
}

/* an item by index, wrapped, under its current fire mode (OVL2 0x5C3740) */
CZFTSWeapon *CZKit::GetItemByIndex(int index) const
{
    return m_item[unwrap(index, FIREMODE_CURRENT)];
}

/* the fire delay still running (OVL2 0x5C2400, new in SOCOM II; the name ours) */
bool CZKit::IsFireDelayed()
{
    return m_fire_delay > 0.0f;
}

/* (OVL2 0x5BE040, 0x5BE030, 0x5BE020) */
int CZKit::GetSelectedItemNum()
{
    return m_selected_item;
}

int CZKit::GetNumItems() const
{
    return m_num_items;
}

void CZKit::SetBody(CZSealBody *body)
{
    m_body = body;
}

/* the kit's ammunition for an item kind (OVL2 0x5BDB20; SOCOM 1's walked the items itself) */
bool CZKit::ItemHasAmmo(EQUIP_ITEM item)
{
    return ItemHasAmmo(FindItemWithAmmo(item));
}

/* (OVL2 0x5BDB00; SOCOM 1's wrote the grenade launcher's test out) */
bool CZKit::CurItemHasAmmo()
{
    return ItemHasAmmo(m_selected_item, GetCurFireMode());
}

/* an item's ammunition under a fire mode (OVL2 0x5BDA70; SOCOM 1's) */
bool CZKit::ItemHasAmmo(int index, FIREMODE mode) const
{
    bool r = false;

    if (index < m_num_items) {
        index = unwrap(index, mode);
        r = ItemHasAmmo(index);
    }
    return r;
}

/* any of an item's magazines holding rounds (OVL2 0x5BDA10; SOCOM 1's) */
bool CZKit::ItemHasAmmo(int index) const
{
    bool r = false;

    if (index < m_num_items) {
        for (int i = 0; i < 10; i++) {
            if (m_reloads[index][i] > 0) {
                r = true;
                break;
            }
        }
    }
    return r;
}

/* the weapon information's change taken, once (OVL2 0x5BC2C0; SOCOM 1's) */
bool CZKit::WeaponInfoChanged()
{
    if (m_weaponinfoChanged) {
        m_weaponinfoChanged = false;
        return true;
    }
    return false;
}

/* a pickup's request answered: the flag down, the wait cleared (OVL2 0x5B99D0) */
void CZKit::PendingPickupHandled()
{
    m_current_requested_ammo = false;
    m_pickupRequestWait = 0.0f;
}

/* (OVL2 0x5B99B0) */
void CZKit::SetWeaponInfoChanged()
{
    m_weaponinfoChanged = true;
}

/* a scope's zoom class (OVL2 0x5B9990; SOCOM 1's threshold 4, SOCOM II's 5) */
bool CZKit::IsUsingScope()
{
    return ((CKitBody *)m_body)->m_200 >= 5;
}

/* the sniper rifle's kick restarted from the controller's aim, and a little adrenaline (OVL2
 * 0x5B9180) */
void CZKit::InitializeSniperKick()
{
    m_firerifle_kick_movedist = 0.0f;
    CKitCtrl *ctrl = ((CKitBody *)m_body)->m_control;

    if (ctrl)
        m_firerifle_kick_baseline = ctrl->m_130;
    func_00578150(m_body, 0.5f);
}

/* the binoculars' zoom class (OVL2 0x5B90F0; SOCOM 1's 3, SOCOM II's 4) */
bool CZKit::IsUsingBinocs()
{
    return ((CKitBody *)m_body)->m_200 == 4;
}

/* the reticle's screen offset (OVL2 0x5B90E0, 0x5B90D0, 0x5B90C0) */
float CZKit::ScreenOffsetX()
{
    return m_screenoffset_x;
}

float CZKit::ScreenOffsetY()
{
    return m_screenoffset_y;
}

void CZKit::IncrScreenOffsetY(float dy)
{
    m_screenoffset_y += dy;
}
