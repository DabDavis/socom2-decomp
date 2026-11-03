/* seal_kit.cpp -- the SEAL's kit, CZKit (SOCOM 1's unit, __sinit_seal_kit.cpp, with its
 * unnamed-namespace CSavedFireModeInfo). OVL2 0x5B9020-0x5C93FF, in SOCOM 1's order (the screen
 * offsets and the sniper's kick first, the constructor and the item lookups last, then
 * sealSaveFireModes and CAiWepHint's statics). So far the small accessors and lookups, the fire
 * delay and shot count, the launcher-mode tests, FireGrenade, the items' removal and setting
 * (RemoveItemAndAmmo, SetItemByIndex), the explosive's placing dispatch, the fire-mode helpers and
 * CSealCtrlAi's two members (PerturbAimVector, the accuracy factor). Its initialiser is 0x668DB0 (the
 * iostream objects, the saved fire modes' array, the placing hit's two pointers zeroed), byte-exact
 * from the statics below, defined in its order. Defined in reverse of their image order (-inline
 * deferred). */

#include "kit.h"
#include "../zcamera/zcamera.h"
#include "../znode/node_world.h"

#include <iostream>

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
/* CZFTSWeapon::GetZoomRange (SOCOM 1's GetZoomRange__11CZFTSWeaponFUi, called from the same
 * GetCurZoom; OVL1 0x3C5980, unclaimed) */
float func_003C5980(CZFTSWeapon *w, unsigned int mode);
/* CZFTSWeapon::IsGrenadeLauncherAmmo and CZWeapon::GetName (SOCOM 1's, called from the same
 * GetFiremodeGraphic and GetItemName; OVL1 0x3C5D20, 0x3D2830, unclaimed) */
bool func_003C5D20(CZFTSWeapon *w);
const char *func_003D2830(CZFTSWeapon *w);
/* an item kind's class (the static under CZWeapon::GetIDType; OVL1 0x3D1A60, unclaimed) and a
 * weapon's magazine size (OVL1 0x3D2810, unclaimed) */
EQUIP_IDTYPE func_003D1A60(EQUIP_ITEM item);
int func_003D2810(CZWeapon *w);
/* a weapon's magazine count and its default ammunition (SOCOM 1's GetDefaultAmmo; OVL1
 * 0x3D2800, 0x3D1F00, unclaimed) and the list's weapon of a kind (SOCOM 1's
 * CZWeaponList::GetWeapon; OVL1 0x3C4B40, unclaimed) */
int func_003D2800(CZWeapon *w);
void *func_003D1F00(CZWeapon *w);
CZWeapon *func_003C4B40(CZWeaponList *list, EQUIP_ITEM item);
/* the current weapon in its grenade launcher's mode (OVL2 0x5BD7E0, this unit's, a byte-exact
 * draft: the index version 0x5BD7F0 on the selected item) */
bool func_005BD7E0(CZKit *kit);
/* a controller's word at 0x11C cleared (OVL2 0x592D40, the SEAL lane's row) */
void func_00592D40(CKitCtrl *ctrl);
/* a weapon's two shot waits (its floats at 0x60 and 0x5C; OVL1 0x3D2C20, 0x3D2C40, unclaimed) and
 * the weapons' shared record of a running delay: its address, value and a byte, marked set (OVL1
 * 0x3D00D0, unclaimed) */
float func_003D2C20(CZFTSWeapon *w);
/* the body able to fire (SOCOM 1's CZSealBody::CanFire, called from the same place in
 * HandlePlacingExlosive; SOCOM II's takes two more: OVL2 0x5A7DE0, the SEAL lane's row), the
 * placing allowed and the explosive placed (OVL2 0x5BD530, 0x5BE9A0: this unit's, unwritten) */
bool func_005A7DE0(CZSealBody *body, int a, int b);
bool func_005BD530(CZKit *kit);
void func_005BE9A0(CZKit *kit, int index, float a, float b);
/* a launcher's grenades of class 145's launcher (OVL1 0x3C5C20, unclaimed: GetFiringItem's test) */
bool func_003C5C20(CZFTSWeapon *w);
/* a fire mode found from an item index on, for the item a mode fires from and an ammunition kind
 * (OVL2 0x5C3EE0, this unit's, unwritten: NextFireMode and 0x5C45C0 call it) */
int func_005C3EE0(CZKit *kit, int index, CZFTSWeapon *w, EQUIP_ITEM kind);
/* a body's part by name (CBody::FindPart, OVL1 0x28E7B0): its node at +0xC */
struct CKitBodyPart {
    unsigned char _0[0xc];
    void *m_node;   // 0xc
};
CKitBodyPart *FindPart__5CBodyFPCc(void *body, const char *name);
float func_003D2C40(CZFTSWeapon *w);
void func_003D00D0(float *delay, float value, unsigned char flag);
/* CSealCtrl::SetCameraZoom (OVL2 0x592C70, the SEAL lane's), CZWeapon::GetAssociatedGearName (SOCOM
 * 1's, called from the same SetItemByIndex; OVL1 0x3D2760, unclaimed) and the gear list's FindGear
 * (SOCOM 1's CCharacterGearList::FindGear, the same call; OVL2 0x53EFF0, unclaimed) on
 * CCharacterType's gear list (OVL1 bss 0x44CD68) */
void SetCameraZoom__9CSealCtrlFff(CKitCtrl *ctrl, float level, float increment);
const char *func_003D2760(CZWeapon *w);
void *func_0053EFF0(void *list, const char *name);
extern unsigned char m_gear_list__14CCharacterType[12];
}

extern "C" {
/* the local player (OVL1's ftsGetPlayer) */
CZSealBody *ftsGetPlayer__Fv();
}

/* a SEAL's stance (zseal.h's SEAL_STANCE, a byte enum; not included here) */
enum SEAL_STANCE {
    STANCE_NONE = -1, STANCE_STAND = 0, STANCE_CROUCH = 1, STANCE_PRONE = 2, STANCE_IDLE = 3,
    STANCE_MAX = 4
};

extern "C" {
/* the SEAL's stance, by its body (SOCOM 1's CZSealBody::GetStance(bool), called from the same places
 * in SOCOM 1's CZKit::Fire and CSealCtrlAi::PerturbAimVector; OVL2 0x58A720, the SEAL lane's row) */
unsigned char func_0058A720(CZSealBody *seal, unsigned char flag);
}

namespace ai {
/* faster than a squared speed (ai_util.cpp's) */
bool is_moving(const CPnt3D *v, float speedSq);
}

/* the AI's target as the controller hands it the aim (SOCOM 1's CAiTarget::ModifyAimDir; OVL2
 * 0x60A000, ai_target.cpp's -- a view, 0x78 bytes as ai_target.cpp's) */
class CAiTarget {
public:
    void ModifyAimDir(CPnt3D *dir, float accuracy, float angle);

    unsigned char _0[0x78];
};

/* the AI controller as the unit's two members of it read it (CSealCtrlAi; zseal.h, which declares a
 * CZKit of its own, is not included here): its body (CEntityCtrl's entity), a base accuracy, the
 * accuracy the aim is perturbed by, the target (+0x67C, SEAL_AI_PARAMS's) and a scale -- the names
 * of the three floats ours */
class CSealCtrlAi {
public:
    float GetAccuracyFactor();
    void PerturbAimVector(CPnt3D *v);
    CZSealBody *GetBody() { return m_entity; }

    /* the aim's error cone (SOCOM 1's aiAccuracyAngle, 15 degrees: OVL1 0x3DF180) */
    static float aiAccuracyAngle;

    void *_vptr;   // 0x0
    CZSealBody *m_entity;   // 0x4
    unsigned char _8[0xd4 - 0x8];
    float m_d4;   // 0xd4
    unsigned char _d8[0x3d0 - 0xd8];
    float m_3d0;   // 0x3d0
    unsigned char _3d4[0x67c - 0x3d4];
    CAiTarget m_target;   // 0x67c
    unsigned char _6f4[0x70c - 0x6f4];
    float m_70c;   // 0x70c
};

/* the unit's statics, in the order its initialiser (__sinit_seal_kit.cpp, OVL2 0x668DB0) builds them
 * after <iostream>'s two objects: SOCOM 1's saved fire modes and their count (OVL2 bss 0x66B580,
 * 0x66B670), then the hit the explosives' placing keeps (0x66B680: 0x5BE9A0 copies a 32-byte hit
 * into it, 0x5C1970 asks its material and aligns to its normal) -- whose constructor zeroes its
 * polygon and node (the initialiser's two `sw zero`, 0x66B68C and 0x66B69C): SOCOM II's
 * IntersectStruct has a constructor node_world.h does not declare, so the unit spells it on a
 * class of its own */
namespace {

/* a weapon's fire mode as the saved table holds it (SOCOM 1's CSavedFireModeInfo, 8 bytes: the mode,
 * then the item; its constructor, 0x5C9230, zeroes the one and 255s the other) */
class CSavedFireModeInfo {
public:
    CSavedFireModeInfo();

    int m_firemode;   // 0x0
    EQUIP_ITEM m_item;   // 0x4
};

/* a hit kept from one tick to the next (zdb::IntersectStruct and SOCOM II's constructor; the name
 * ours) */
struct CKitHit : public zdb::IntersectStruct {
    CKitHit()
    {
        m_poly = 0;
        m_node = 0;
    }
};

CSavedFireModeInfo g_arrSavedFireModeInfo[30];
int g_iNumSavedFireModes;
CKitHit g_placeHit;

}

/* the middle of a weapon's AI range, none without a weapon (OVL2 0x5C93C0; SOCOM 1's, at SOCOM II's
 * AI_PARAMS offsets). The half set first and scaled in place: the ternary, the if-returns and a
 * product assigned whole all lose the false arm's trailing nop or swap the product's registers */
float CAiWepHint::GetWeaponEffectiveRange(CZWeapon *w)
{
    float r;

    if (w) {
        r = 0.5f;
        r *= w->m_ai_params.m_max_range + w->m_ai_params.m_min_range;
    } else
        r = 0.0f;
    return r;
}

/* a weapon's accuracy at a distance (none without a weapon), doubled when asked, held within [0, 1]
 * (OVL2 0x5C9340, byte-exact as a draft; SOCOM 1's had neither the factor nor the clamp): the three
 * constants function-local statics, as the data's 8-byte spacing shows */
float CAiWepHint::GetWeaponAccuracy(CZWeapon *w, float d, bool b)
{
    static float factor = 2.0f;
    static float lo = 0.0f;
    static float hi = 1.0f;
    float acc = w ? w->m_ai_params.GetAccuracy(d) : 0.0f;

    if (b)
        acc *= factor;
    _saturate(&acc, lo, hi);
    return acc;
}

/* the AI's accuracy at its stance and pace: a base (0xD4) scaled by 1.25 crouched and 1.5 prone,
 * by 0.7 while the body moves (faster than 25 a second), and by the word at 0x70C (OVL2 0x5C9270;
 * SOCOM 1's PerturbAimVector computed the same factors inline -- 0.9 standing there -- and 0x55F0A0
 * now asks this; the name ours). The cases listed backwards: the compare chain tests them in
 * reverse source order (0, 1, 2 here), standing's an empty case of its own */
float CSealCtrlAi::GetAccuracyFactor()
{
    CZSealBody *body = GetBody();
    float f = m_d4;

    switch (func_0058A720(body, false)) {
    case STANCE_PRONE:
        f *= 1.5f;
        break;
    case STANCE_CROUCH:
        f *= 1.25f;
        break;
    case STANCE_STAND:
        break;
    }
    if (ai::is_moving(&((CKitBody *)body)->m_velW, 625.0f))
        f *= 0.7f;
    f *= m_70c;
    return f;
}

/* the aim turned off its line by the target's error (SOCOM 1's PerturbAimVector: its slot, +0x74 of
 * CSealCtrlAi's table; OVL2 0x5C9250) -- SOCOM 1's computed the accuracy here, SOCOM II's keeps it
 * at 0x3D0 */
void CSealCtrlAi::PerturbAimVector(CPnt3D *v)
{
    m_target.ModifyAimDir(v, m_3d0, aiAccuracyAngle);
}

/* (OVL2 0x5C9230: out of line between them, as SOCOM 1's) */
CSavedFireModeInfo::CSavedFireModeInfo()
{
    m_firemode = 0;
    m_item = EQUIP_NONE;
}

/* the local player's items' fire modes saved (OVL2 0x5C91B0; SOCOM 1's, the items now read straight,
 * not unwrapped: CMPExitState::Init calls it). Each entry through a pointer, which might alias the
 * count, so the count is read again each pass (indexed, MW unrolls the loop 8x); the counter declared
 * ahead of the kit takes the higher argument register */
void sealSaveFireModes()
{
    int i;
    CZKit *kit = (CZKit *)((char *)ftsGetPlayer__Fv() + 0x5e0);

    g_iNumSavedFireModes = kit->m_num_items;
    for (i = 0; i < g_iNumSavedFireModes; i++) {
        CSavedFireModeInfo *info = &g_arrSavedFireModeInfo[i];

        info->m_item = kit->m_item[i]->m_ID;
        info->m_firemode = kit->m_firemode[i];
    }
}


/* the item kinds of the first two items and the current one, 255 without one (OVL2 0x5C89D0,
 * 0x5C8980, 0x5C8930; the current one SOCOM 1's GetCurItemEnum, the first two new -- their names
 * ours) */
EQUIP_ITEM CZKit::GetCurItemEnum() const
{
    CZFTSWeapon *w = m_item[unwrap(m_selected_item, FIREMODE_CURRENT)];

    return w ? w->m_ID : EQUIP_NONE;
}

EQUIP_ITEM CZKit::GetPrimaryEnum() const
{
    CZFTSWeapon *w = m_item[unwrap(0, FIREMODE_CURRENT)];

    return w ? w->m_ID : EQUIP_NONE;
}

EQUIP_ITEM CZKit::GetSecondaryEnum() const
{
    CZFTSWeapon *w = m_item[unwrap(1, FIREMODE_CURRENT)];

    return w ? w->m_ID : EQUIP_NONE;
}

/* the first item of a kind with ammunition, else the last of the kind, or -1 (OVL2 0x5C8870; SOCOM
 * 1's took the first of the kind). The ammunition's test written out: MW then steps the magazines'
 * row with the item, where FindItemWithAmmo's inlined ItemHasAmmo works it out each pass (as
 * GetAmmoItemIndex has it, 16 bytes longer) */
int CZKit::GetItemIndex(EQUIP_ITEM item) const
{
    int found = -1;

    for (int i = 0; i < m_num_items; i++) {
        if (m_item[i] && item == m_item[i]->m_ID) {
            bool r = false;

            if (i < m_num_items) {
                for (int j = 0; j < 10; j++) {
                    if (m_reloads[i][j] > 0) {
                        r = true;
                        break;
                    }
                }
            }
            if (r)
                return i;
            found = i;
        }
    }
    return found;
}

/* the first item loaded with a kind of ammunition, a count of rounds in all its magazines, or -1
 * (OVL2 0x5C8750; SOCOM 1's, GetAmmoTotal inlined where SOCOM 1's summed after unwrap) */
int CZKit::GetAmmoIndex(EQUIP_AMMO ammo, int count)
{
    for (int i = 0; i < m_num_items; i++)
        if (m_ammo[i] && ammo == ((CZAmmo *)m_ammo[i])->m_ID && count == GetAmmoTotal(i))
            return i;
    return -1;
}

/* whether the kit holds an item of that kind (OVL2 0x5C86F0; SOCOM 1's) */
bool CZKit::HasItem(EQUIP_ITEM item) const
{
    for (int i = 0; i < m_num_items; i++)
        if (m_item[i] && item == m_item[i]->m_ID)
            return true;
    return false;
}

/* the first item of that kind, and its ammunition (OVL2 0x5C8640, new in SOCOM II: CSealCtrl's
 * OnActionButton and PerformProximityActions call it where SOCOM 1's called HasItem and then
 * ItemHasAmmo; the name ours) */
bool CZKit::HasItemWithAmmo(EQUIP_ITEM item)
{
    for (int i = 0; i < GetNumItems(); i++) {
        if (m_item[i] && item == m_item[i]->m_ID) {
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

/* zoomed: a scope or the binoculars (OVL2 0x5BE6C0; SOCOM 1's) */
bool CZKit::IsZoomed()
{
    return IsUsingScope() || IsUsingBinocs();
}

/* the current item's zoom at the body's zoom class, 1 outside its zoom modes (OVL2 0x5BE660;
 * SOCOM 1's) */
float CZKit::GetCurZoom()
{
    CZFTSWeapon *w = m_item[m_selected_item];
    float zoom = 1.0f;

    if (w) {
        unsigned int mode = ((CKitBody *)m_body)->m_200 - 4;

        if (mode < w->m_zoommode.size())
            zoom = func_003C5980(w, mode);
    }
    return zoom;
}

/* (OVL2 0x5BE650, new in SOCOM II where SOCOM 1's UpdateCurItemReticule was; the name ours) */
RETICULE_TYPE CZKit::GetCurItemReticule()
{
    return m_curitemreticule;
}

/* the reticle's change taken, once, with a little adrenaline (OVL2 0x5BE5F0; SOCOM 1's) */
bool CZKit::CurReticuleTypeChanged()
{
    if (m_reticuletypeChanged) {
        m_reticuletypeChanged = false;
        func_00578150(m_body, 0.5f);
        return true;
    }
    return false;
}

/* the selected item's reticle (OVL2 0x5BE560, new in SOCOM II; the name ours) */
RETICULE_TYPE CZKit::GetCurReticule()
{
    return GetReticuleByIndex(m_selected_item);
}

/* the current item's reticle, flagged when it changes or when forced (OVL2 0x5BE570; SOCOM 1's
 * took no flag and always flagged it) */
extern "C" {
/* CZSealBody's (named: the bomb pickup made where it is placed -- static, the point and a flag --
 * and the bomb's gear shown or not; zseal.h is not included here) */
void CreateMPBombPickup__10CZSealBodyFPC6CPnt3Db(const CPnt3D *pos, bool b);
void EnableMPBombGear__10CZSealBodyFb(CZSealBody *body, bool on);
}

/* the MP bomb placed: its pickup made at the point, the bomb item (154) dropped, its gear hidden,
 * the controller's bit set and the reticule brought up to date (OVL2 0x5BC200; SOCOM 1's) */
void CZKit::HandlePlaceMPBomb(const CPnt3D *pos)
{
    CreateMPBombPickup__10CZSealBodyFPC6CPnt3Db(pos, true);
    RemoveItem((EQUIP_ITEM)154);
    EnableMPBombGear__10CZSealBodyFb(m_body, false);
    ((CKitBody *)m_body)->m_control->SetBombPlaced(true);
    SetCurReticule(false);
}

void CZKit::SetCurReticule(bool force)
{
    RETICULE_TYPE r = GetReticuleByIndex(m_selected_item);

    if (force || r != m_curitemreticule) {
        m_curitemreticule = r;
        m_reticuletypeChanged = true;
    }
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

/* the current item's loaded magazines -- under its grenade launcher's mode, the launcher's
 * ammunition item's (OVL2 0x5C49B0; SOCOM 1's) */
int CZKit::GetCurNumMags()
{
    int index = m_selected_item;

    if (func_005BD7E0(this))
        index = FindItemWithAmmo((EQUIP_ITEM)GetCurFireMode());
    if (index != -1)
        return GetNumMags(index);
    return 0;
}

/* the first explosive's (151) ammunition's damage kept, none without one (OVL2 0x5C77B0, new in
 * SOCOM II: PostSetupItems calls it; the name ours) */
void CZKit::SetupExplosiveDamage()
{
    m_explosiveDamage = 0.0f;
    for (int i = 0; i < m_num_items; i++) {
        if (m_item[i] && m_item[i]->m_ID == 151) {
            m_explosiveDamage = ((CZAmmo *)GetAmmoByIndex(i))->GetExplosionDamage();
            break;
        }
    }
}

/* the binoculars (190; SOCOM 1's 182) given unless already held (OVL2 0x5C7450; SOCOM 1's, through
 * SetItem where SOCOM 1's fetched the weapon itself) */
void CZKit::SetupBinocular()
{
    if (!HasItem(EQUIP_BINOCULARS))
        SetItem(m_num_items, EQUIP_BINOCULARS);
}

extern "C" {
/* an item kind of class 145, and a pistol's (4: SOCOM 1's CZWeapon::IsSecondary) -- CZWeapon's
 * statics before IsPrimary (OVL1 0x3D1CB0, 0x3D1CE0, unclaimed) */
bool func_003D1CB0(EQUIP_ITEM id);
bool func_003D1CE0(EQUIP_ITEM id);
}

/* the first primary weapon, pistol and class-145 item, or -1 (OVL2 0x5C7230, 0x5C71A0, 0x5C7110:
 * new in SOCOM II, where SOCOM 1's GetItemIndex(SEAL_ITEM) read each item's slot; the names ours) */
int CZKit::GetPrimaryIndex()
{
    for (int i = 0; i < m_num_items; i++)
        if (m_item[i] && CZWeapon::IsPrimary(m_item[i]->m_ID))
            return i;
    return -1;
}

int CZKit::GetSecondaryIndex()
{
    for (int i = 0; i < m_num_items; i++)
        if (m_item[i] && func_003D1CE0(m_item[i]->m_ID))
            return i;
    return -1;
}

int CZKit::GetClass145Index()
{
    for (int i = 0; i < m_num_items; i++)
        if (m_item[i] && func_003D1CB0(m_item[i]->m_ID))
            return i;
    return -1;
}

/* the item a fire mode's ammunition is: under a launcher's mode, the mode is the ammunition's item
 * kind (OVL2 0x5C6530, new in SOCOM II: CAmmoDisplay::Update asks it for the firemode graphic; the
 * name ours) */
int CZKit::GetAmmoItemIndex(FIREMODE mode)
{
    return FindItemWithAmmo((EQUIP_ITEM)mode);
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

/* the rounds in all an item's magazines (OVL2 0x5C3C10, where SOCOM 1's EmptyAmmoCountTotal
 * sat -- that one emptied them; the name ours) */
int CZKit::GetAmmoTotal(int index)
{
    index = wrap(index, m_num_items);
    int total = 0;

    for (int i = 0; i < 10; i++)
        total += m_reloads[index][i];
    return total;
}

/* the current item's fire mode set, when the item has it and the ammunition for it -- a grenade
 * launcher's modes past 3 always (OVL2 0x5C37B0; SOCOM 1's) */
bool CZKit::SetCurFireMode(FIREMODE mode)
{
    CZFTSWeapon *w = GetCurItem();

    if (!w)
        return false;
    if ((mode > AUTOFIRE && func_003C5D80(w)) ||
        (mode <= w->m_maxfiremode && ItemHasAmmo(m_selected_item, mode)))
        m_firemode[m_selected_item] = mode;
    m_weaponinfoChanged = true;
    return true;
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

/* (OVL2 0x5C4480) The mode read in place each time, MW sharing the one load: taken into a local
 * first, it lands in t1 and every temporary of the inlined walk shifts a register (21 words) */
int CZKit::NextFireMode(int index)
{
    int i = 0;
    EQUIP_ITEM kind = EQUIP_NONE;

    if (m_firemode[m_selected_item] > 3) {
        i = FindItemWithAmmo((EQUIP_ITEM)m_firemode[m_selected_item]);
        kind = (EQUIP_ITEM)m_firemode[m_selected_item];
    }
    return func_005C3EE0(this, i, GetFiringItem(m_selected_item), kind);
}

/* (OVL2 0x5C67D0) */
CZFTSWeapon *CZKit::GetFiringItem(int index)
{
    int i = unwrap(index, (FIREMODE)255);

    if (m_item[i] && func_003C5D20(m_item[i])) {
        if (func_003C5C20(m_item[i]))
            i = GetClass145Index();
        else
            i = GetPrimaryIndex();
    }
    return m_item[i];
}

/* (OVL2 0x5C6980) */
void CZKit::AttachItemNode(unsigned char kind, const char *part)
{
    CKitBody *body = (CKitBody *)m_body;
    CKitBody::Attach::Record *a = body->m_attach[kind].m_rec;

    if (a) {
        zdb::CNode *node = (zdb::CNode *)ItemPtr(m_selected_item);
        zdb::CNode *parent = (zdb::CNode *)FindPart__5CBodyFPCc(body->m_cbody, part)->m_node;

        if (parent && node) {
            node->SetActive(false);
            parent->AddChild(node);
            a->m_node = node;
        }
    }
}

/* (OVL2 0x5C48B0) */
void CZKit::SetHandFireMode(int mode)
{
    m_firemode[HandSlot()] = mode;
    m_hand_firemode = mode;
    if (((CKitBody *)m_body)->m_alive)
        m_hand_item = m_item[HandSlot()] ? m_item[HandSlot()]->m_ID : m_hand_item;
}

/* (OVL2 0x5C32B0) */
void CZKit::SetFireDelay(float delay, bool alt)
{
    if (delay == -1.0f) {
        CZFTSWeapon *w = GetCurItem();

        if (alt && w)
            m_fire_delay = func_003D2C20(w);
        else if (w)
            m_fire_delay = func_003D2C40(w);
    } else
        m_fire_delay = delay;
    func_003D00D0(&m_fire_delay, m_fire_delay, 0);
    m_alt_firedelay = alt;
}

/* (OVL2 0x5C0020) */
void CZKit::HandlePlacingExlosive(EQUIP_ITEM item, float a, float b)
{
    m_last_selected = m_selected_item;
    if (!SetCurItemEnum(item)) {
        m_last_selected = -1;
        return;
    }
    if (m_item[m_selected_item]->IsDirectFire()) {
        if (m_last_selected >= 0) {
            SetCurItemByIndex(m_last_selected);
            m_last_selected = -1;
        }
        return;
    }
    if (func_005A7DE0(m_body, 0, 0) && m_81c <= 0.0f && func_005BD530(this)) {
        func_005BE9A0(this, m_selected_item, a, b);
        return;
    }
    SetCurItemByIndex(m_last_selected);
    m_hotswap = false;
}

/* the fire delay still running (OVL2 0x5C2400, new in SOCOM II; the name ours) */
bool CZKit::IsFireDelayed()
{
    return m_fire_delay > 0.0f;
}

/* the wait after a shot: the current item's (0.1 without one), none on safety, 0.8 of it in burst
 * and automatic, 0.2 of it otherwise (OVL2 0x5C09F0; SOCOM 1's, the item through GetCurItem). Every
 * arm a return: an `else if` chain assigning the delay merges the single-shot arm's jump */
float CZKit::GetFireDelay()
{
    CZFTSWeapon *w = GetCurItem();
    float delay = 0.1f;

    if (w)
        delay = ((CKitWeapon *)w)->m_firewait;
    int fm = m_firemode[m_selected_item];

    if (fm == SAFETY)
        return 1.0f;
    if (fm == SINGLE || fm > AUTOFIRE)
        return delay;
    if (fm == BURST)
        return 0.8f * delay;
    if (fm == AUTOFIRE)
        return 0.8f * delay;
    return 0.2f * delay;
}

/* the rounds a pull of the trigger fires: one without ammunition, none on safety, three in burst,
 * all of them in automatic (OVL2 0x5C0940, new in SOCOM II: SOCOM 1's WillFireWeapon did it inline;
 * the name ours) */
int CZKit::GetShotCount()
{
    if (!CurItemHasAmmo())
        return 1;
    int fm = m_firemode[m_selected_item];

    if (fm == SAFETY)
        return 0;
    if (fm == SINGLE || fm > AUTOFIRE)
        return 1;
    if (fm == BURST)
        return 3;
    if (fm == AUTOFIRE)
        return 10000;
    return 0;
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

/* an item's name, wrapped (OVL2 0x5BE240; SOCOM 1's) */
const char *CZKit::GetItemName(int index)
{
    int i = wrap(index, m_num_items);

    return i != -1 ? func_003D2830(m_item[i]) : 0;
}

/* a grenade launcher's ammunition's icon (OVL2 0x5BE1D0; SOCOM 1's GetFiremodeName and
 * GetFiremodeGraphic in one, the index no longer wrapped).
 * OPEN, 0x70 against 0x68: the target tests the null item with the zero in the delay slot
 * (beqz s0,end; move v0,zero) and then branches over a branch on the call's result (bnez
 * v0,L3; nop; b L4; nop; L3: b end; lw v0,188(s0); L4: move v0,zero) -- neither an early return,
 * nested ifs, a ternary nor a result local gives that pair (nested ifs: 8 differing, the shared
 * return-0 block) */
void *CZKit::GetFiremodeGraphic(int index)
{
    if (index == -1)
        return 0;
    CZFTSWeapon *w = m_item[index];

    if (!w)
        return 0;
    if (func_003C5D20(w))
        return w->m_icon;
    return 0;
}

/* an item's icon, wrapped, and the selected one's (OVL2 0x5BE110, 0x5BE050; SOCOM 1's had only
 * the first -- the second's name ours) */
void *CZKit::GetIcon(int index)
{
    int i = wrap(index, m_num_items);

    if (i == -1)
        return 0;
    CZFTSWeapon *w = m_item[i];

    /* an if, not a ternary: the two compile alike out of line, but only this one gives
     * GetCurIcon's inlined copy its two padded labels */
    if (w)
        return w->m_icon;
    return 0;
}

void *CZKit::GetCurIcon()
{
    return GetIcon(m_selected_item);
}

/* an item of a kind, from the weapon list, put in a slot (OVL2 0x5BDFB0; SOCOM 1's). SetupBinocular
 * calls it out of line: SetItemByIndex is an auto-inline candidate too big to inline whole, which
 * stops its callers (no pin) */
bool CZKit::SetItem(unsigned int index, EQUIP_ITEM item)
{
    CZWeapon *w = func_003C4B40(&CZWeapon::m_listWeapons, item);

    if (w && index < 30) {
        SetItemByIndex(index, w);
        return true;
    }
    return false;
}

/* an item's ammunition set, and its first magazines filled -- with the weapon's default
 * ammunition when none is given (OVL2 0x5BDEC0, 0x5BDE20; SOCOM 1's) */
bool CZKit::SetItemAmmo(unsigned int index, void *ammo, int rounds, int count)
{
    bool r = false;

    if (!ammo) {
        CZFTSWeapon *w = m_item[index];

        if (w)
            ammo = func_003D1F00(w);
    }
    if (ammo && index < 30 && m_item[index]) {
        for (int i = 0; i < count && i < 10; i++)
            m_reloads[index][i] = rounds;
        m_ammo[index] = ammo;
        r = true;
    }
    return r;
}

bool CZKit::SetItemAmmo(unsigned int index, void *ammo)
{
    bool r = false;

    if (index < 30 && m_item[index]) {
        int size = func_003D2810(m_item[index]);

        r = SetItemAmmo(index, ammo, size, func_003D2800(m_item[index]));
    }
    return r;
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

extern "C" {
/* a weapon of class 145 (OVL1 0x3C5CA0, unclaimed) */
bool func_003C5CA0(CZFTSWeapon *w);
}

/* an M203 (or class 145) in its launcher's fire mode (OVL2 0x5BD8F0, its selected item's 0x5BDA00:
 * SOCOM 1's CurWeaponIsM203Mode on an index) */
bool CZKit::WeaponIsM203Mode(int index)
{
    index = wrap(index, m_num_items);
    CZFTSWeapon *w = GetItemByIndex(index);

    if (w && (w->IsM203Weapon() || func_003C5CA0(w)) && m_firemode[index] > AUTOFIRE)
        return true;
    return false;
}

/* a launcher in its launcher's fire mode (OVL2 0x5BD7F0, its selected item's 0x5BD7E0: new in
 * SOCOM II, the name ours) */
bool CZKit::WeaponIsLauncherMode(int index)
{
    index = wrap(index, m_num_items);
    CZFTSWeapon *w = GetFiringItem(index);

    if (w && w->IsLauncherWeapon() && m_firemode[index] > AUTOFIRE)
        return true;
    return false;
}

/* a grenade launcher in its launcher's fire mode (OVL2 0x5BD6E0, its selected item's 0x5BD6D0:
 * SOCOM 1's WeaponIsGrenadeLauncherMode, the item through 0x5C67D0 where SOCOM 1's unwrapped it) */
bool CZKit::WeaponIsGrenadeLauncherMode(int index)
{
    index = wrap(index, m_num_items);
    CZFTSWeapon *w = GetFiringItem(index);

    if (w && w->IsGrenadeLauncherWeapon() && m_firemode[index] > AUTOFIRE)
        return true;
    return false;
}

extern "C" {
/* the kit's fire (SOCOM 1's CZKit::Fire(int, const CPnt3D *, const CPnt3D *, const CPnt3D *); OVL2
 * 0x5C5340, unwritten) and an explosive placed (SOCOM 1's HandleRemotePlaceExplosive, which took the
 * point and the SEAL's rotation: SOCOM II's takes two more, here 0; OVL2 0x5BC730, unwritten) */
CZProjectile *func_005C5340(CZKit *kit, int index, const CPnt3D *pos, const CPnt3D *vel, const CPnt3D *v);
CZProjectile *func_005BC730(CZKit *kit, const CPnt3D *pos, const void *rot, int a, int b);
}

/* a grenade thrown from a point at a velocity -- the current item fired with no spin, or, an
 * explosive (151; SOCOM 1's 150), placed with the SEAL's rotation (OVL2 0x5BD030; SOCOM 1's) */
CZProjectile *CZKit::FireGrenade(const CPnt3D *pos, const CPnt3D *vel)
{
    CPnt3D v = CPnt3D::zero;
    CZFTSWeapon *w = m_item[m_selected_item];

    if (!w)
        return 0;
    if (func_003D1E10(w) == IDTYPE_EXPLOSIVE)
        return func_005BC730(this, pos, (char *)m_body + 0x50, 0, 0);
    return func_005C5340(this, m_selected_item, pos, vel, &v);
}

/* an item by index, wrapped, handed back through a pointer (OVL2 0x5BC6A0; SOCOM 1's) */
void CZKit::FillWeaponEnum(int index, CZFTSWeapon **out)
{
    index = wrap(index, m_num_items);
    *out = m_item[index];
}

/* an item put in a slot with its ammunition, the rounds dealt into its magazines (OVL2 0x5BC5D0,
 * new in SOCOM II between SetItemByIndex and FillWeaponEnum; the name ours) */
bool CZKit::SetItem(unsigned int index, CZWeapon *w, void *ammo, int rounds)
{
    if (index >= m_num_items)
        m_num_items = index + 1;
    /* a copy: it takes the lowest saved register, after the parameters' */
    int slot = index;

    SetItemByIndex(slot, w);
    m_ammo[slot] = ammo;
    m_currentmag[slot] = 0;
    if (w) {
        int size = func_003D2810(w);
        int i = 0;

        while (rounds > 0) {
            if (size < rounds) {
                m_reloads[slot][i] = size;
                rounds -= size;
            } else {
                m_reloads[slot][i] = rounds;
                rounds = 0;
            }
            i++;
        }
    }
    return true;
}

/* the item in hand changed: the throw let go, the trigger cleared, a scope's zoom undone (inline:
 * SetItemByIndex expands it; the name ours) */
inline void CZKit::ResetZoom(CKitCtrl *ctrl)
{
    ctrl->SetThrowing(false);
    func_00592D40(ctrl);
    if (GetCurZoom() > 1.01f)
        SetCameraZoom__9CSealCtrlFff(ctrl, 1.0f, 0.5f);
    m_cur_zoom = 0;
}

/* an item put in a slot (index 30 at most, the count raised past it): its fire mode and magazine
 * reset, its gear found, the HUD told, the reticle taken again; when it is the item in hand, the
 * throw let go, the trigger cleared and a zoomed-in scope's zoom undone (OVL2 0x5BC410; SOCOM 1's,
 * which cleared the trigger at the end and asked the reticle nothing). Fifteen statements by MW's
 * count, the growth and the zoom's reset two inlines: an auto-inline candidate too big to inline
 * whole, so SetItem and SetupBinocular stay calls without a pin -- written out (20) it is no
 * candidate, and SetupBinocular inlines SetItem (0xBC against 0x84) */
void CZKit::SetItemByIndex(int index, CZWeapon *w)
{
    if (w && index < 31) {
        GrowTo(index);
        m_item[index] = (CZFTSWeapon *)w;
        m_firemode[index] = 0;
        m_currentmag[index] = 0;
        m_associatedgear[index] = func_0053EFF0(&m_gear_list__14CCharacterType, func_003D2760(w));
        bool cur = m_selected_item == index;
        CKitCtrl *ctrl = ((CKitBody *)m_body)->m_control;

        ctrl->SetBombPlaced(true);
        SetCurReticule(false);
        m_weaponinfoChanged = true;
        if (cur)
            ResetZoom(ctrl);
    }
}

/* whether an item's fire mode has a graphic: not for explosives, grenades, binoculars and two
 * new classes -- the current item's class, whatever the index (OVL2 0x5BC310, 0x5BC300; SOCOM 1's
 * CurItemHasFiremodeGraphic, the first's name ours) */
bool CZKit::ItemHasFiremodeGraphic(int index)
{
    bool r = true;
    int i = wrap(index, m_num_items);

    if (!GetCurItem())
        r = false;
    else {
        switch (func_003D1A60(GetCurItem()->m_ID)) {
        case 201:
        case IDTYPE_BINOCULARS:
        case 230:
        case IDTYPE_GRENADE:
        case IDTYPE_EXPLOSIVE:
            r = false;
            break;
        }
    }
    return r;
}

bool CZKit::CurItemHasFiremodeGraphic()
{
    return ItemHasFiremodeGraphic(m_selected_item);
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

/* the hot-swap flag (OVL2 0x5BC2B0, then its setter 0x5BC2A0) */
bool CZKit::GetHotSwap()
{
    return m_hotswap;
}

void CZKit::SetHotSwap(bool b)
{
    m_hotswap = b;
}

extern "C" {
/* the items closed up over the empty slots, each slot's state, ammunition, magazines, fire mode
 * and pointer moved with it (OVL2 0x5B9A00, new in SOCOM II: unwritten, the name to come) and
 * CZSealBody::WepWaitOn (OVL2 0x5DFFC0, seal_weapon.cpp's) */
void func_005B9A00(CZKit *kit);
void WepWaitOn__10CZSealBodyFv(CZSealBody *body);
}

/* the first item of a kind with ammunition, else the last of the kind, let go (OVL2 0x5B9E10, new
 * in SOCOM II: HandlePlaceMPBomb drops the bomb, item 154, through it, where SOCOM 1's dropped it
 * inline; the name ours). Four statements, yet HandlePlaceMPBomb calls it: RemoveItemAndAmmo is an
 * auto-inline candidate (eleven statements) too big to inline whole, which stops its callers
 * (README "Tells from the kit and AI states, round 6") -- the pin it needed went with it */
void CZKit::RemoveItem(EQUIP_ITEM item)
{
    int i = FindItemWithAmmo(item);

    if (i != -1)
        RemoveItemAndAmmo(i);
}

/* an item at an index let go with its ammunition (OVL2 0x5B9BB0; SOCOM 1's RemoveItemAndAmmo, the
 * closing up and the wait new): the index wrapped on the count three times over, as SOCOM 1's
 * (one inline wrap, two calls to its unwrap) -- the item, the ammunition, the magazines */
void CZKit::RemoveItemAndAmmo(int index)
{
    int i = wrap(index, m_num_items);

    m_item[i] = 0;
    ClearAmmo(i);
    func_005B9A00(this);
    UpdateNumItems();
    if (m_selected_item > m_num_items || !m_item[m_selected_item]) {
        m_selected_item = 0;
        WepWaitOn__10CZSealBodyFv(m_body);
        m_weaponinfoChanged = true;
    }
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

/* an item kind in use: the binoculars or a scope, the night vision, or an item of the kit
 * (OVL2 0x5B98C0; SOCOM 1's) */
bool CZKit::IsUsingItem(EQUIP_ITEM item)
{
    if (item == EQUIP_BINOCULARS)
        return IsUsingBinocs() || IsUsingScope();
    if (item == EQUIP_NVG)
        return NVG_Enabled();
    for (int i = 0; i < m_num_items; i++)
        if (m_item[i] && item == m_item[i]->m_ID)
            return IsUsingItem(i);
    return false;
}

/* an item in use: by its class, the binoculars or a scope, the night vision, a weapon in hand,
 * else its state's bit 3 (OVL2 0x5B97A0; SOCOM 1's) */
bool CZKit::IsUsingItem(int index)
{
    EQUIP_IDTYPE type = func_003D1E10(m_item[index]);

    if (type == IDTYPE_BINOCULARS)
        return IsUsingBinocs() || IsUsingScope();
    if (type == IDTYPE_NVG)
        return NVG_Enabled();
    if (type == IDTYPE_PISTOL || type == IDTYPE_SMG || type == IDTYPE_ASSAULT_RIFLE ||
        type == IDTYPE_SHOTGUN || type == IDTYPE_MACHINEGUN || type == IDTYPE_SNIPER_RIFLE ||
        type == IDTYPE_GRENADE || type == IDTYPE_HEAVYWEAPON || type == IDTYPE_EXPLOSIVE)
        return m_selected_item == index;
    return (m_itemstate[index] & 8) != 0;
}

/* a scope's zoom class (OVL2 0x5B9990; SOCOM 1's threshold 4, SOCOM II's 5) */
bool CZKit::IsUsingScope()
{
    return ((CKitBody *)m_body)->m_200 >= 5;
}

extern "C" {
/* the target mods' kick distances (SOCOM 1's CTargetMods::GetFireRifle_KickRandomDist and
 * GetFireRifle_KickBaseDist, const: OVL1 0x3C5F60 and 0x3C5F70 read 0x48 and 0x44, unclaimed) */
float func_003C5F60(const CTargetMods *mods);
float func_003C5F70(const CTargetMods *mods);
int rand(void);
}

/* the sniper's kick begun: the controller's float at 0x130 the baseline, the distance the stance's
 * base plus a random part of its random distance (OVL2 0x5B91C0, new in SOCOM II: SOCOM 1's
 * CZKit::Fire did this inline, after testing the distance for zero; the name ours) */
void CZKit::StartSniperKick()
{
    CZFTSWeapon *w = GetCurItem();
    CTargetMods *mods = w->TargetMods(func_0058A720(m_body, true));
    CKitCtrl *ctrl = ((CKitBody *)m_body)->m_control;

    if (ctrl)
        m_firerifle_kick_baseline = ctrl->m_130;
    m_firerifle_kick_movedist = func_003C5F70(mods) + func_003C5F60(mods) * (4.656613e-10f * rand());
    m_firerifle_kick_state = 1;
    m_firerifle_kick_accum = 0.0f;
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

/* the binoculars put away or taken out: put away, the controller's word at 0x11C cleared (OVL2
 * 0x5B9110; SOCOM 1's only set the bit) */
void CZKit::Binocs_Enabled(bool on)
{
    if (m_binocs_enabled && !on)
        func_00592D40(((CKitBody *)m_body)->m_control);
    m_binocs_enabled = on;
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

/* the offsets cleared, and the camera's set to the scope's or to none (OVL2 0x5B9030; SOCOM 1's
 * set the camera's fields itself) */
void CZKit::ResetScreenOffsetXY()
{
    m_screenoffset_x = 0.0f;
    m_screenoffset_y = 0.0f;
    zdb::CCamera *cam = zdb::CWorld::m_world->m_camera;

    if (m_body && ((CKitBody *)m_body)->m_200 > 4) {
        cam->SetScreenOffsetX(m_sniper_posx);
        cam->SetScreenOffsetY(m_sniper_posy);
    } else {
        cam->SetScreenOffsetX(0.0f);
        cam->SetScreenOffsetY(0.0f);
    }
}

/* the per-item pointers at 0x834, one place on so that item -1 has slot 0: read (or 0 past the
 * end), cleared, and set -- the vector grown by null pushes until the slot exists (OVL2 0x5B8FF0,
 * 0x5B8FC0, 0x5B8F20: new in SOCOM II, the names ours; the environment's pickup code calls them) */
void *CZKit::GetItemPtr(int index)
{
    return index + 1 < m_itemptr.size() ? m_itemptr[index + 1] : 0;
}

void CZKit::ClearItemPtr(int index)
{
    if (index + 1 < m_itemptr.size())
        m_itemptr[index + 1] = 0;
}

void CZKit::SetItemPtr(int index, void *p)
{
    while (index + 1 >= m_itemptr.size())
        m_itemptr.push_back(0);
    m_itemptr[index + 1] = p;
}
