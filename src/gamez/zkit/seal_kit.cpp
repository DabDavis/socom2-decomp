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

/* (the materials' real class ahead of zweapon.h's stand-in: GetHandFloorHit's inline lookup) */
#include "../znode/int_material.h"
#include "kit.h"
#include "../zcamera/zcamera.h"
#include "../zentity/zentity.h"
#include "../znode/node_world.h"
#include "../zsnd/zsnd.h"

#include <iostream>

extern "C" {
/* item 193 given for item 153 (OVL2 0x5C74E0, this unit's: a CZKit member, unnamed) */
void func_005C74E0(CZKit *kit);
/* CZSealBody::AdrenalineIncr (SOCOM 1's AdrenalineIncr__10CZSealBodyFf, tail-called from the same
 * InitializeSniperKick; OVL2 0x578150, the SEAL lane's row) */
void AdrenalineIncr__10CZSealBodyFf(CZSealBody *body, float amount);
/* (the weapons' members this unit calls -- IsLauncherWeapon, GetIDType, GetZoomRange,
 * IsGrenadeLauncherAmmo, GetDescription, the static GetIDType and class tests, GetAmmoCapacity,
 * GetNumMags, GetDefaultAmmo, CZWeaponList::GetWeapon, RegisterTimer2, CTargetMods' kick
 * distances -- are zweapon.h's, called as members since the rows carry their names) */
/* a controller's word at 0x11C cleared (OVL2 0x592D40, the SEAL lane's row) */
void func_00592D40(CKitCtrl *ctrl);
/* a weapon's two shot waits (its floats at 0x60 and 0x5C; OVL1 0x3D2C20, 0x3D2C40, unclaimed) */
float func_003D2C20(CZFTSWeapon *w);
/* the body able to fire: CZSealBody::CanFire(bool, bool) (OVL2 0x5A7DE0, the SEAL lane's, named
 * CanFire__10CZSealBodyFbb; SOCOM 1's took no flags), called from the same place in
 * HandlePlacingExlosive as SOCOM 1's -- spelled by its mangled name because CZSealBody is
 * incomplete here (kit.h's stand-ins keep zseal.h out of this unit) -- and the explosive placed
 * (OVL2 0x5BE9A0: this unit's, unwritten) */
bool CanFire__10CZSealBodyFbb(CZSealBody *body, bool a, bool b);
/* a launcher's grenades of class 145's launcher (OVL1 0x3C5C20, unclaimed: GetFiringItem's test) */
bool func_003C5C20(CZFTSWeapon *w);
/* a fire mode found from an item index on, for the item a mode fires from and an ammunition kind
 * (OVL2 0x5C3EE0, this unit's, unwritten: NextFireMode and 0x5C45C0 call it) */
int func_005C3EE0(CZKit *kit, int index, CZFTSWeapon *w, EQUIP_ITEM kind);
/* whether a weapon's ammunition is of a launcher class (OVL1 0x3C5B20) */
bool func_003C5B20(CZFTSWeapon *w, int cls);
/* a body's part by name (CBody::FindPart, OVL1 0x28E7B0): its node at +0xC */
struct CKitBodyPart {
    unsigned char _0[0xc];
    void *m_node;   // 0xc
};
CKitBodyPart *FindPart__5CBodyFPCc(void *body, const char *name);
float func_003D2C40(CZFTSWeapon *w);
/* CSealCtrl::SetCameraZoom (OVL2 0x592C70, the SEAL lane's), CZWeapon::GetAssociatedGearName (SOCOM
 * 1's, called from the same SetItemByIndex; OVL1 0x3D2760, unclaimed) and the gear list's FindGear
 * (SOCOM 1's CCharacterGearList::FindGear, the same call; OVL2 0x53EFF0, unclaimed) on
 * CCharacterType's gear list (OVL1 bss 0x44CD68) */
void SetCameraZoom__9CSealCtrlFff(CKitCtrl *ctrl, float level, float increment);
void WepWaitOn__10CZSealBodyFv(CZSealBody *body);
void WepWaitOff__10CZSealBodyFv(CZSealBody *body);
const char *GetAssociatedGearName__8CZWeaponFv(CZWeapon *w);
void *FindGear__18CCharacterGearListFPCc(void *list, const char *name);
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
unsigned char GetStance__10CZSealBodyCFb(CZSealBody *seal, unsigned char flag);
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

    switch (GetStance__10CZSealBodyCFb(body, false)) {
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

/* an item of a kind made the current one: a launcher's ammunition selects the launcher, in that ammunition's fire mode; the
 * models changed, the wait for the weapon on unless it is thrown; a primary (or, with the second hand, a sidearm) noted for
 * the hand; the reticle and weapon information updated, the throw flag cleared, the camera's zoom eased back from a scope.
 * Whether it had one (OVL2 0x5C8A20; SOCOM 1's by its place after ~CZKit's neighbours) */
bool CZKit::SetCurItemEnum(EQUIP_ITEM item)
{
    int i = FindItemWithAmmo(item);

    if (i == -1)
        return false;
    int prev = m_selected_item;

    ((CKitBody *)m_body)->m_control->SetBombPlaced(true);
    if (i == -1)
        return false;
    if (m_item[i]->IsGrenadeLauncherAmmo()) {
        m_selected_item = GetLauncherSlot();
        if (m_selected_item == -1) {
            m_selected_item = prev;
            return false;
        }
        EQUIP_ITEM ammo = m_item[i]->m_ID;
        EQUIP_ITEM id = m_item[m_selected_item]->m_ID;

        ChangeItemModel(FireClassOfId(m_item[prev]->m_ID), FireClassOfId(id), prev, false);
        if (FireClassOfId(m_item[m_selected_item]->m_ID) == FIRE_CLASS_THROWN)
            WepWaitOff__10CZSealBodyFv(m_body);
        else
            WepWaitOn__10CZSealBodyFv(m_body);
        m_firemode[m_selected_item] = (unsigned char)ammo;
    } else {
        m_selected_item = i;
        if (m_item[prev]) {
            EQUIP_ITEM id = m_item[i]->m_ID;

            ChangeItemModel(FireClassOfId(m_item[prev]->m_ID), FireClassOfId(id), prev, false);
            if (FireClassOfId(m_item[i]->m_ID) == FIRE_CLASS_THROWN)
                WepWaitOff__10CZSealBodyFv(m_body);
            else
                WepWaitOn__10CZSealBodyFv(m_body);
        }
        if (m_firemode[m_selected_item] == 0)
            CycleFireMode();
    }
    if (CZWeapon::IsPrimary(item) || (m_second_hand && CZWeapon::IsSecondary(item))) {
        m_hand_firemode = m_firemode[m_selected_item];
        if (((CKitBody *)m_body)->m_alive)
            m_hand_item = m_item[m_selected_item] ? m_item[m_selected_item]->m_ID : m_hand_item;
    }
    CKitCtrl *ctrl = ((CKitBody *)m_body)->m_control;

    ctrl->SetBombPlaced(true);
    SetCurReticule(false);
    m_weaponinfoChanged = true;
    ctrl->SetThrowing(false);
    func_00592D40(ctrl);
    if (GetCurZoom() > 1.01f)
        SetCameraZoom__9CSealCtrlFff(ctrl, 1.0f, 0.5f);
    m_cur_zoom = 0;
    return true;
}

/* a valve by name, its state read (zanim.h's CValve, which this unit cannot include: its _zrdr rivals node_world.h's --
 * the view ours) */
struct CKitValve {
    unsigned char _0[4];
    unsigned short m_state;   // 0x4
};

extern "C" CKitValve *GetByName__6CValveFPCc(const char *name);

/* the kit made: empty, its items' slots zeroed, its state cleared in SOCOM 1's order and the screen offsets reset; the
 * zoomed reticle's valve read (OVL2 0x5C8190; SOCOM 1's by its place after ~CZKit, CZSealBody's constructor calls it) */
CZKit::CZKit()
    : m_num_items(0), m_body(0), m_flashlight_model(0), m_lightMap(0)
{
    for (int i = 0; i < 30; i++) {
        m_item[i] = 0;
        m_ammo[i] = 0;
        m_firemode[i] = 0;
        for (int j = 0; j < 10; j++)
            m_reloads[i][j] = 0;
        m_currentmag[i] = 0;
        m_associatedgear[i] = 0;
        m_iteminuse[i] = 0;
        m_itemstate[i] = 0;
    }
    m_requested_item = false;
    m_bit13 = false;
    m_tracercount = 0;
    m_cur_zoom = 0;
    m_restore_zoom = m_cur_zoom;
    m_selected_item = 0;
    m_last_selected = -1;
    m_itemptr.reserve(8);
    m_81c = 0.0f;
    m_burst_count = 0;
    m_firetime = 0.0f;
    m_waiting_to_fire = false;
    m_waiting_to_raise = false;
    m_retsize = 0.0f;
    m_prev_retsize = 0.0f;
    m_prev_factor = 0.0f;
    m_prev_axis_a = 0.0f;
    m_prev_axis_b = 0.0f;
    m_retblockedsize = 0.0f;
    m_retsizegoal = 0.0f;
    m_retmin = 0.0f;
    m_retmax = 0.0f;
    m_reticuletypeChanged = false;
    m_retoffsetx = 0.0f;
    m_18 = 0.0f;
    m_retoffsety = 0.0f;
    m_1c = 0.0f;
    m_retblockedX = 0.0f;
    m_retblockedY = 0.0f;
    m_retblocked = false;
    m_curitemreticule = (RETICULE_TYPE)2;
    m_first = true;
    m_changeToRifle = false;
    m_current_requested_ammo = false;
    m_pickupRequestWait = 0.0f;
    m_chill_parabola = 0.0f;
    m_heartbeat_state = 0;
    m_heartbeat_accum = 0.0f;
    m_sniper_posx = 0.0f;
    m_sniper_posy = 0.0f;
    m_sniper_prev_posx = 0.0f;
    m_sniper_prev_posx = 0.0f;
    m_firerifle_kick_baseline = 0.0f;
    m_firerifle_kick_movedist = 0.0f;
    m_firerifle_kick_accum = 0.0f;
    m_firerifle_kick_state = 0;
    m_nvg_enabled = false;
    m_binocs_enabled = false;
    m_heartbeat_enabled = true;
    m_bit16 = false;
    m_lase_timer = 0.0f;
    m_lase_anim = 0;
    m_lase_mapname = 0;
    m_lase_echo = false;
    m_lase_deploy_echo = false;
    m_second_hand = false;
    m_lasesnd_start = 0;
    m_lasesnd_fin = 0;
    m_lase_inst = 0;
    m_lase_inst2 = 0;
    CKitValve *v = GetByName__6CValveFPCc("EQlimit");
    if (v && v->m_state == 1)
        m_second_hand = true;
    m_unused = 0;
    ResetScreenOffsetXY();
    m_fire_delay = 0.0f;
    m_alt_firedelay = false;
    m_explosiveDamage = 0.0f;
    m_hand_firemode = 0;
    m_hand_item = (EQUIP_ITEM)255;
    m_8 = 0;
    m_8bc = -1.0f;
    m_8a4 = 0.2f;
    m_8ac = CPnt3D::zero;
    m_8a8 = 0;
}

/* a sniper rifle in hand, loaded (OVL2 0x5C85D0; SOCOM 1's, whose class was 100) */
bool CZKit::HasSniperRifle()
{
    if (GetCurItemIDType() == IDTYPE_SNIPER_RIFLE && CurItemHasAmmo())
        return true;
    return false;
}

/* the kit gone: its members' destructors only, the item pointers' vector freed (SOCOM 1's ~CZKit by
 * its place after NVG_Enabled, before the constructor; OVL2 0x5C8110, a draft before) */
CZKit::~CZKit()
{
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

/* the reticule node's settle countdown (OVL2 0x5BE990, new in SOCOM II: 0x5BE6F0 below runs it; the
 * explosive is placed only once it reads 0 -- the name ours) */
float CZKit::GetReticuleSettle()
{
    return m_8a4;
}

/* the node under the reticule followed from frame to frame: one that an explosive's damage would
 * move (its extension's slot 0x10 asked with the kit's explosive damage) and was under the reticule
 * last frame too has its world place taken -- the countdown held at 0.2 while that place moves, run
 * down to 0 once it rests; any other node, or none, starts it over at 0.2 from nowhere. Then the node
 * kept (OVL2 0x5BE6F0, new in SOCOM II: 0x5C0FD0 calls it with the frame's time -- a CZKit member,
 * unnamed) */
extern "C" void func_005BE6F0(CZKit *kit, float dT)
{
    zdb::IntersectStruct hit = ((CKitBody *)kit->m_body)->m_last_reticule;
    zdb::CNode *node = hit.m_node;
    zdb::CNodeEx *ex = node ? node->m_nodeEx : 0;

    if (kit->m_8a8 && !kit->m_8a8->m_active)
        kit->m_8a8 = 0;
    if (ex && node == kit->m_8a8 && ex->Slot10(kit->m_explosiveDamage)) {
        CPnt3D pos = node->GetPosition();
        CPnt3D last = kit->m_8ac;
        zdb::CNode *parent = node->m_parent;
        CMatrix m;

        if (parent) {
            parent->BuildMTW(m);
            pos = pos + *(const CPnt3D *)m.m_matrix[3];
        }
        last = last - pos;
        bool moved = Dot(last, last) > 0.0f;

        kit->m_8ac = pos;
        if (moved)
            kit->m_8a4 = 0.2f;
        else {
            if (kit->m_8a4 > 0.0f)
                kit->m_8a4 -= dT;
            if (kit->m_8a4 < 0.0f)
                kit->m_8a4 = 0.0f;
        }
    } else {
        kit->m_8ac = CPnt3D::zero;
        kit->m_8a4 = 0.2f;
    }
    kit->m_8a8 = node;
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
            zoom = w->GetZoomRange(mode);
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
        AdrenalineIncr__10CZSealBodyFf(m_body, 0.5f);
        return true;
    }
    return false;
}

/* the selected item's reticle (OVL2 0x5BE560, new in SOCOM II; the name ours) */
RETICULE_TYPE CZKit::GetCurReticule()
{
    return GetReticuleByIndex(m_selected_item);
}

/* an item's reticle: the binoculars' (7), a zoomed scope's (5), a launcher's (3; 1 for an M203 --
 * or item 63 -- in a fire mode under 5), none for 151/152, then by the item's range (OVL2 0x5BE300;
 * SOCOM 1's, the ranges and kinds moved on: the binoculars' zoom class 4 where SOCOM 1's was 3,
 * GetCurZoom inlined, the firing item where SOCOM 1's unwrapped the index). The id preset and
 * assigned under the test (`w ? w->m_ID : 255` drops the target's two alignment nops) */
RETICULE_TYPE CZKit::GetReticuleByIndex(int index)
{
    CZFTSWeapon *w = GetFiringItem(index);
    EQUIP_ITEM id = (EQUIP_ITEM)255;

    if (w)
        id = w->m_ID;
    RETICULE_TYPE r;

    if (((CKitBody *)m_body)->m_200 == 4)
        r = (RETICULE_TYPE)7;
    else if (GetCurZoom() > 1.01f)
        r = (RETICULE_TYPE)5;
    else if (w && w->IsLauncherWeapon()) {
        r = (RETICULE_TYPE)3;
        if (w->IsM203Weapon() || id == 63)
            if (m_firemode[index] < 5)
                r = (RETICULE_TYPE)1;
    } else if (id == 152 || id == 151)
        r = RETICULE_NONE;
    else if (id == 11)
        r = (RETICULE_TYPE)9;
    else if (id >= 4 && id < 31)
        r = (RETICULE_TYPE)0;
    else if (id >= 31 && id < 81)
        r = (RETICULE_TYPE)1;
    else if (id >= 81 && id < 91)
        r = (RETICULE_TYPE)2;
    else if (id >= 91 && id < 121)
        r = (RETICULE_TYPE)1;
    else if (id >= 121 && id < 141)
        r = (RETICULE_TYPE)4;
    else if (id >= 151 && id < 190)
        r = (RETICULE_TYPE)4;
    else if (id >= 190 && id < 254)
        r = (RETICULE_TYPE)0;
    else
        r = RETICULE_NONE;
    return r;
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

extern "C" {
/* the MP bomb's pickup and node (CPickup's statics, zseal.h's; OVL2 0x541C60, 0x542470, OVL1 bss
 * 0x44D4A0), the SEAL's bomb anchors and drop (zseal.h's CZSealBody members: OVL2 0x544C60, 0x544C40,
 * 0x59DD30), the network game's side test and the mission's message queue (hud_tcmcmds.cpp's
 * spellings: OVL1 0x2C30B0, 0x2B6530) */
class CPickup;
CPickup *GetMPBomb__7CPickupFv();
void Destroy__7CPickupFP7CPickup(CPickup *pickup);
extern zdb::CNode *m_mpbombnode__7CPickup;
bool CompassAnchor_MPBomb__10CZSealBodyFP14CCompassAnchor(CZSealBody *seal, CCompassAnchor *anchor);
bool CompassAnchor_MPBase__10CZSealBodyFP14CCompassAnchor(CZSealBody *seal, CCompassAnchor *anchor);
void DropMPBomb__10CZSealBodyFv(CZSealBody *seal);
bool func_002C30B0(const CZSealBody *seal);
}

/* the HUD's message list (fts_message.cpp's C2DMessage_Q, OVL1 0x2B6530 its Add_Message: SOCOM 1's
 * Add_Message(text, r, g, b) grown) and the message kind it is coloured by (fts_state.h's MSGTYPE) */
enum MSGTYPE { MSGTYPE_PLAYER = 0 };

class C2DMessage_Q {
public:
    void Add_Message(const char *text, MSGTYPE type, bool centered, int time, float scale);
    const char *LastMessage();
};

/* the mission as the bomb pickup reads it (theMission, OVL1 0x4364E0: its message queue at 0xB80
 * and the two bases' nodes at 0x18EC/0x18F0 -- the names ours) */
class CKitMission {
public:
    unsigned char _0[0xb80];
    unsigned char m_messages[0x18ec - 0xb80];   // 0xb80
    zdb::CNode *m_base0;   // 0x18ec
    zdb::CNode *m_base1;   // 0x18f0
};

extern CKitMission theMission;

/* the compass's base anchors, friendly and enemy (a byte: the kind is masked to 8 bits before it
 * goes into the anchor's 16-bit field; the names ours) */
enum KIT_ANCHOR_KIND { KIT_ANCHOR_OUR_BASE = 2, KIT_ANCHOR_THEIR_BASE = 3 };

static inline bool KitIsPlayer(struct CKitBody *body);

/* the SEAL as the bomb pickup reads it: its team (CEntity's, 0xC8) and its bomb anchor (zseal.h's
 * at 0xEC0) */
struct CKitBombBody {
    unsigned char _0[0xc8];
    unsigned int m_team;   // 0xc8
    unsigned char _cc[0xec0 - 0xcc];
    CCompassAnchor *m_mpbomb_anchor;   // 0xec0
};

/* the MP bomb picked up: the bomb item (154) and its ammunition (19) put in the next slot with one
 * round, the player told and the bomb's sound played at the SEAL, the grenade model hung, the
 * pickup destroyed, the gear shown, the player's bomb anchor taken down and the base's put up
 * (friendly or enemy by the teams); with no bomb only the gear shown; a dead player drops it again
 * (OVL2 0x5BBF10; SOCOM 1's, without the message, the sound and the drop) */
void CZKit::PickupMPBomb()
{
    CPickup *bomb = GetMPBomb__7CPickupFv();

    if (bomb) {
        int n = m_num_items;
        CZWeapon *w = CZWeapon::m_listWeapons.GetWeapon((EQUIP_ITEM)154);
        CZAmmo *ammo = CZAmmo::m_ammolist.GetAmmo((EQUIP_AMMO)19);

        SetItemByIndex(n, w);
        m_ammo[n] = ammo;
        AddAmmo(n, 1);
        m_currentmag[n] = 0;
        if (KitIsPlayer((CKitBody *)m_body))
            ((C2DMessage_Q *)theMission.m_messages)->Add_Message("Satchel", MSGTYPE_PLAYER, false, 0, 0.0f);
        CSnd *snd = CSnd::GetSoundByName(".FOOTBOMB");

        if (snd)
            snd->PlaySound3D(&((CEntity *)m_body)->m_posW, 1.0f, 0, 0, 255);
        InitItemModel(w->GetModelName(), "grenade");
        Destroy__7CPickupFP7CPickup(bomb);
        m_mpbombnode__7CPickup = 0;
        EnableMPBombGear__10CZSealBodyFb(m_body, true);
        CZSealBody *p = KitIsPlayer((CKitBody *)m_body) ? m_body : ftsGetPlayer__Fv();

        if (((CKitBombBody *)p)->m_mpbomb_anchor)
            CCompass::DeleteAnchor(((CKitBombBody *)p)->m_mpbomb_anchor);
        CompassAnchor_MPBomb__10CZSealBodyFP14CCompassAnchor(p, 0);
        bool side = func_002C30B0(m_body);
        zdb::CNode *base = side ? theMission.m_base0 : theMission.m_base1;

        if (base) {
            CCompassAnchor *a = CCompass::CreateAnchor(*(const CPnt3D *)base->m_matrix.m_matrix[3], "ALL_YOUR_BASE");

            a->m_type = ((CKitBombBody *)m_body)->m_team & ((CKitBombBody *)p)->m_team ? KIT_ANCHOR_OUR_BASE
                                                                                       : KIT_ANCHOR_THEIR_BASE;
            CompassAnchor_MPBase__10CZSealBodyFP14CCompassAnchor(p, a);
        } else
            CompassAnchor_MPBase__10CZSealBodyFP14CCompassAnchor(p, 0);
    } else
        EnableMPBombGear__10CZSealBodyFb(m_body, true);
    if (KitIsPlayer((CKitBody *)m_body) && !(((CKitBody *)m_body)->m_alive ? true : false))
        DropMPBomb__10CZSealBodyFv(m_body);
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
        r = (EQUIP_IDTYPE)w->GetIDType();
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

    if (w && w->IsLauncherWeapon() && fm > AUTOFIRE) {
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

    if (CurWeaponIsLauncherMode())
        index = FindItemWithAmmo((EQUIP_ITEM)GetCurFireMode());
    if (index != -1)
        return GetNumMags(index);
    return 0;
}

/* the SEAL's controller as far as Init asks it: CSealCtrl's table to IsPlayer at +0x2C (zseal.h's,
 * which this unit does not include; the two slots before it unnamed here), and zseal.h's
 * CZSealBody::IsPlayer over it (the controller tested, slot 0x2C called; the name zseal.h's) */
class CKitSealCtrl : public CEntityCtrl {
public:
    virtual void Virtual24();
    virtual void Virtual28();
    virtual bool IsPlayer() const;   /* +0x2C */
    virtual bool IsAi() const;   /* +0x30 */
    virtual bool IsRemote() const;   /* +0x34 */
    virtual CSealCtrlAi *GetSealCtrlAi();   /* +0x38 */
};

/* the local player as the recoil's screen scale reads it: the point its view starts from and the
 * offset its aim adds to its position (new places in SOCOM II: SOCOM 1's at 0x10B8, 0x1110; the names
 * ours) */
struct CKitPlayerView {
    unsigned char _0[0x1458];
    CPnt3D m_1458;   // 0x1458
    unsigned char _1464[0x14b0 - 0x1464];
    CPnt3D m_14b0;   // 0x14b0
};

static inline bool KitIsPlayer(CKitBody *body)
{
    return body->m_control ? ((CKitSealCtrl *)body->m_control)->IsPlayer() : false;
}

static inline bool KitIsAi(CKitBody *body)
{
    return body->m_control ? ((CKitSealCtrl *)body->m_control)->IsAi() : false;
}

static inline bool KitIsRemote(CKitBody *body)
{
    return body->m_control ? ((CKitSealCtrl *)body->m_control)->IsRemote() : false;
}

/* the kit set up (OVL2 0x5C7E00; SOCOM 1's Init, 0x2AE9E0, call for call): the reload, empty-click
 * and laser designator's two sounds found by name; the flashlight's light map made, placed at the
 * origin as -0.0 -- the projection's translation and the node's position both, through one pointer
 * held (CLightMap::SetPos(0, 0, 0) passes +0.0 to SetPosition, mtc1 zero, where both builds pass the
 * stores' 0x80000000; m_lightMap read for each store reloads it), ranged 20-100, named, its opacity 256 and its blend word 73; a player's lens effects
 * dropped, its night vision off and the timer reset; the sniper, kick, heartbeat and lase state
 * cleared -- the previous sniper x twice, as SOCOM 1's -- and SOCOM II's new words set */
void CZKit::Init()
{
    m_rldsnd = CSnd::GetSoundByName(".M4A1_RLD");
    m_clicksnd = CSnd::GetSoundByName(".GUN_EMPTY");
    m_lasesnd_start = CSnd::GetSoundByName("~LSR_DES_LOCK");
    m_lasesnd_fin = CSnd::GetSoundByName("~LSR_DES_MISS");
    m_lightMap = new zdb::CLightMap;
    zdb::CLightMap *l = m_lightMap;

    l->m_worldToUV.m_matrix[3][0] = -0.0f;
    l->m_worldToUV.m_matrix[3][1] = -0.0f;
    l->m_worldToUV.m_matrix[3][2] = -0.0f;
    l->SetPosition(-0.0f, -0.0f, -0.0f);
    m_lightMap->SetRange(20.0f, 100.0f);
    m_lightMap->SetName("flash_lightmap");
    m_lightMap->m_opacity = 256.0f;
    zdb::BLENDMODE blend;

    blend.m_alphaWORD = 73;
    m_lightMap->m_blendmode = blend;
    if (m_body && KitIsPlayer((CKitBody *)m_body)) {
        Apply_LensEffects(false, 0);
        m_nvg_enabled = false;
        CZWeapon::ClearTimer(0);
    }
    m_chill_parabola = 0.0f;
    m_heartbeat_state = 0;
    m_sniper_posx = 0.0f;
    m_sniper_posy = 0.0f;
    m_sniper_prev_posx = 0.0f;
    m_sniper_prev_posx = 0.0f;
    m_firerifle_kick_movedist = 0.0f;
    m_firerifle_kick_accum = 0.0f;
    m_firerifle_kick_state = 0;
    m_lase_mapname = 0;
    m_lase_anim = 0;
    m_lase_timer = 0.0f;
    m_lase_echo = false;
    CZWeapon::ClearTimer(&m_lase_timer);
    m_hotswap = false;
    m_8 = 0;
    m_8bc = -1.0f;
    m_8a4 = 0.2f;
    m_8ac = CPnt3D::zero;
    m_8a8 = 0;
}

/* an item's node deleted unless something holds it, its slot zeroed (std::transform's function
 * object: an empty user constructor, so it is built with no stores; the name ours) */
struct DeleteUnparented {
    DeleteUnparented() {}
    void *operator()(void *p) const
    {
        zdb::CNode *n = (zdb::CNode *)p;

        if (n && !n->GetParent())
            zdb::CNode::Delete(n);
        return 0;
    }
};

/* (OVL2 0x5C7CE0; SOCOM 1's UnInit over its m_items_avail -- SOCOM II's item pointers -- and its
 * light map). The three iterators the target keeps in the frame, copied through $f0, are
 * std::transform's by-value parameters (the MSL lane's round 17: no header change) */
void CZKit::UnInit()
{
    std::transform(m_itemptr.begin(), m_itemptr.end(), m_itemptr.begin(), DeleteUnparented());
    m_itemptr.clear();
    if (m_lightMap) {
        delete m_lightMap;
        m_lightMap = 0;
    }
}

extern "C" {
/* a weapon's slots (OVL1 0x3C5970) and whether an ammunition suits it (SOCOM 1's; OVL1 0x3D2680) */
int func_003C5970(CZFTSWeapon *w);
bool IsLegalAmmo__8CZWeaponFP6CZAmmo(CZFTSWeapon *w, void *ammo);
}

/* the bomb gear, the one the kit's items ask for or the plain kit's (OVL2 0x5A0DA0, 0x5A0CD0, 0x5A0C00: the SEAL's) */
extern "C" {
void func_005A0DA0(CZSealBody *body);
void func_005A0CD0(CZSealBody *body);
void func_005A0C00(CZSealBody *body);
}

/* the items in place: the gear they ask for, a human player's kit made up (0x5C74E0), the 2X ammunition, the weapon
 * information changed, the explosive's damage, and the MP bomb's gear on with item 152 or 154 loaded (OVL2 0x5C7840;
 * SOCOM 1's, 0x1D8) */
void CZKit::PostSetupItems()
{
    if (HasItem((EQUIP_ITEM)202))
        func_005A0DA0(m_body);
    else if (HasItem((EQUIP_ITEM)201))
        func_005A0CD0(m_body);
    else
        func_005A0C00(m_body);
    if (!KitIsAi((CKitBody *)m_body) && !(((CKitBombBody *)m_body)->m_team & 0x20000))
        func_005C74E0(this);
    KitIsAi((CKitBody *)m_body);
    Setup2XAmmo();
    m_weaponinfoChanged = true;
    SetupExplosiveDamage();
    if ((HasItem((EQUIP_ITEM)152) && ItemHasAmmo((EQUIP_ITEM)152))
        || (HasItem((EQUIP_ITEM)154) && ItemHasAmmo((EQUIP_ITEM)154)))
        EnableMPBombGear__10CZSealBodyFb(m_body, true);
    else
        EnableMPBombGear__10CZSealBodyFb(m_body, false);
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

/* with item 194 held, every weapon of id types 4, 31, 51, 81, 91 and 101 gets twice its magazines:
 * the extra ones (up to ten) filled as the first is, the rest emptied (OVL2 0x5C75F0; SOCOM 1's
 * Setup2XAmmo, its item 188 and its count of magazines asked of the weapon list's own weapon in
 * SOCOM II) */
void CZKit::Setup2XAmmo()
{
    if (HasItem((EQUIP_ITEM)194)) {
        for (int i = 0; i < m_num_items; i++) {
            if (m_item[i]) {
                EQUIP_ITEM t = m_item[i]->GetIDType();

                if (t == 4 || t == 31 || t == 51 || t == 81 || t == 91 || t == 101) {
                    int n = CZWeapon::m_listWeapons.GetWeapon(m_item[i]->m_ID)->GetNumMags();
                    int j;

                    for (j = n; j < 2 * n; j++)
                        if (j < 10)
                            m_reloads[i][j] = m_reloads[i][0];
                    for (; j < 10; j++)
                        m_reloads[i][j] = 0;
                }
            }
        }
    }
}

/* item 193 given when the body's kit holds item 153 and this one lacks it, the weapon info then
 * changed (OVL2 0x5C74E0, new in SOCOM II: SOCOM 1's AddAmmo did it inline, items 153 and 187 --
 * a CZKit member, unnamed) */
/* the body as far as its kit (zseal.h's CZSealBody::m_kit at 0x5E0; zseal.h stays out of this
 * unit): taken into a local first, the member's offset folds into the inlined HasItem's loads --
 * cast in place (or the kit's address taken into a local), MW adds it once into a register */
struct CKitBodyKit {
    unsigned char _0[0x5e0];
    CZKit m_kit;   // 0x5e0
};

extern "C" void func_005C74E0(CZKit *kit)
{
    CKitBodyKit *b = (CKitBodyKit *)kit->m_body;

    if (b->m_kit.HasItem((EQUIP_ITEM)153) && !kit->HasItem((EQUIP_ITEM)193)) {
        kit->SetItem(kit->m_num_items, (EQUIP_ITEM)193);
        kit->m_weaponinfoChanged = true;
    }
}

/* the binoculars (190; SOCOM 1's 182) given unless already held (OVL2 0x5C7450; SOCOM 1's, through
 * SetItem where SOCOM 1's fetched the weapon itself) */
void CZKit::SetupBinocular()
{
    if (!HasItem(EQUIP_BINOCULARS))
        SetItem(m_num_items, EQUIP_BINOCULARS);
}

/* the gears' models, shared by name (char_type.cpp's CCharacterModelCache, a static there) */
class CCharacterModelCache {
public:
    static zdb::CModelVector m_ModelCache;
};

/* an item's model -- from the character model cache, else the world's (its tree mode set, then
 * cached) -- lit, instanced at the origin unless it is the grenade, named, and the node (or 0)
 * pushed onto the item nodes; whether there was a model (OVL2 0x5C72C0; SOCOM 1's InitItemModel,
 * whose models came from zdb's cache and whose grenade node was the body's). The three pushed
 * words are named locals declared up front in the order of their frame slots, highest first (node,
 * none, kept), the model pushed through a copy assigned after SetModeTree so it stays in its
 * register, and the cache's result tested as the assignment (`bnez v0` with the copy into s0 in the
 * delay slot) */
bool CZKit::InitItemModel(const char *model, const char *name)
{
    CPnt3D pos = CPnt3D::zero;
    CQuat rot = CQuat::identity;
    zdb::CNode *node;
    zdb::CNode *none;
    zdb::CModel *kept;
    zdb::CModel *m;

    if (!(m = CCharacterModelCache::m_ModelCache.GetModel(model))) {
        m = (zdb::CModel *)zdb::CWorld::GetModel(model);
        if (m) {
            m->SetModeTree(2);
            kept = m;
            CCharacterModelCache::m_ModelCache.push_back(kept);
        }
    }
    if (m) {
        m->SetDynamicLight(true, true);
        node = 0;
        if (!name || strcmp(name, "grenade")) {
            node = zdb::CNode::CreateInstance(m, &pos, &rot);
            node->SetName(name);
        }
        m_itemptr.push_back(node);
        return true;
    }
    none = 0;
    m_itemptr.push_back(none);
    return false;
}

extern "C" {
/* an item kind of class 145, and a pistol's (4: SOCOM 1's CZWeapon::IsSecondary) -- CZWeapon's
 * statics before IsPrimary (OVL1 0x3D1CB0, 0x3D1CE0, unclaimed) */
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
        if (m_item[i] && CZWeapon::IsSecondary(m_item[i]->m_ID))
            return i;
    return -1;
}

int CZKit::GetClass145Index()
{
    for (int i = 0; i < m_num_items; i++)
        if (m_item[i] && CZWeapon::IsLauncher(m_item[i]->m_ID))
            return i;
    return -1;
}

/* the pistol's version (OVL2 0x5C6DA0): the rifle's and the pistol's models taken only for an index
 * found, and hung without testing their parts */
void CZKit::ReParentToPistol()
{
    int pi = GetPrimaryIndex();
    CZFTSWeapon *primary = 0;
    if (pi != -1)
        primary = m_item[pi];
    int si = GetSecondaryIndex();
    CZFTSWeapon *secondary = 0;
    if (si != -1)
        secondary = m_item[si];
    int li = GetClass145Index();

    if (primary)
        FireClassOf(primary);
    if (secondary)
        FireClassOf(secondary);

    CKitBody *body = (CKitBody *)m_body;
    CKitBody::Attach::Record *rifle = body->m_rifle;
    CKitBody::Attach::Record *pistol = body->m_pistol;
    CKitBody::Attach::Record *launcher = body->AttachPart(4);
    zdb::CNode *rm = 0;
    zdb::CNode *pm = 0;
    zdb::CNode *lm = 0;

    if (pi + 1 < (int)m_itemptr.size() && pi != -1)
        rm = (zdb::CNode *)m_itemptr[pi + 1];
    if (si + 1 < (int)m_itemptr.size() && si != -1)
        pm = (zdb::CNode *)m_itemptr[si + 1];
    if (li + 1 < (int)m_itemptr.size())
        lm = (zdb::CNode *)m_itemptr[li + 1];

    if (rm) {
        rm->SetActive(true);
        rifle->m_node = rm;
    }
    if (pm) {
        pm->SetActive(true);
        pistol->m_node = pm;
    }
    if (lm && launcher) {
        lm->SetActive(true);
        launcher->m_node = lm;
        ((zdb::CNode *)launcher->m_parent->m_node)->DeleteChild((zdb::CNode *)launcher->m_node);
        ((zdb::CNode *)launcher->m_parent->m_node)->AddChild((zdb::CNode *)launcher->m_node);
    }
}

/* the first primary weapon's, pistol's and class-145 item's models hung back on the body's rifle,
 * pistol and launcher parts, each made active, the launcher's moved last among its parent's
 * children (OVL2 0x5C6A40: SOCOM 1's ReParentToRifle hung items 0's and 1's on the rifle and
 * pistol parts); the two weapons' fire classes are asked and unused */
void CZKit::ReParentToRifle()
{
    int pi = GetPrimaryIndex();
    CZFTSWeapon *primary = 0;
    if (pi != -1)
        primary = m_item[pi];
    int si = GetSecondaryIndex();
    CZFTSWeapon *secondary = 0;
    if (si != -1)
        secondary = m_item[si];
    int li = GetClass145Index();

    if (primary)
        FireClassOf(primary);
    if (secondary)
        FireClassOf(secondary);

    CKitBody *body = (CKitBody *)m_body;
    CKitBody::Attach::Record *rifle = body->m_rifle;
    CKitBody::Attach::Record *pistol = body->m_pistol;
    CKitBody::Attach::Record *launcher = body->AttachPart(4);
    zdb::CNode *rm = 0;
    zdb::CNode *pm = 0;
    zdb::CNode *lm = 0;

    if (pi + 1 < (int)m_itemptr.size())
        rm = (zdb::CNode *)m_itemptr[pi + 1];
    if (si + 1 < (int)m_itemptr.size())
        pm = (zdb::CNode *)m_itemptr[si + 1];
    if (li + 1 < (int)m_itemptr.size())
        lm = (zdb::CNode *)m_itemptr[li + 1];
    if (rm && rifle) {
        rm->SetActive(true);
        rifle->m_node = rm;
    }
    if (pm && pistol) {
        pm->SetActive(true);
        pistol->m_node = pm;
    }
    if (lm && launcher) {
        lm->SetActive(true);
        launcher->m_node = lm;
        ((zdb::CNode *)launcher->m_parent->m_node)->DeleteChild((zdb::CNode *)launcher->m_node);
        ((zdb::CNode *)launcher->m_parent->m_node)->AddChild((zdb::CNode *)launcher->m_node);
    }
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

/* the rounds an item's magazine holds: under a launcher's mode, the capacity of the item its
 * ammunition is -- the first of that kind with rounds, else the last -- none without one (OVL2
 * 0x5C3CF0; SOCOM 1's, whose grenade launcher indexed its ammunition by the mode).
 * The last arm's call through a named local: `int r = call; return r;` keeps the target's layout (the return-0 block,
 * then the call falling into the epilogue) where `return call;` -- every if / ternary spelling of rounds 30-56 --
 * lays out the call first */
int CZKit::GetMaxAmmoCount(int index)
{
    index = wrap(index, m_num_items);
    if (WeaponIsLauncherMode(index)) {
        int i = FindItemWithAmmo((EQUIP_ITEM)m_firemode[index]);

        if (i == -1 || !m_item[i])
            return 0;
        return m_item[i]->GetAmmoCapacity();
    }
    if (!m_item[index])
        return 0;
    int r = m_item[index]->GetAmmoCapacity();
    return r;
}

/* (OVL2 0x5C3CE0; SOCOM 1's, a tail call here) */
int CZKit::GetCurMaxAmmoCount()
{
    return GetMaxAmmoCount(m_selected_item);
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

/* the rounds in an item's current magazine: none without the item; under a launcher's mode those of
 * the item its ammunition is (the first of that kind with rounds, else the last; none without one);
 * for a launcher, its fire mode stepped once or twice as the selected item (until the item in hand
 * is a launcher), then -- in a launcher's mode -- the item its ammunition is, -1 when that is out of
 * range (OVL2 0x5C38A0; SOCOM 1's, the item read through the grenade launcher's mode, the launcher's
 * stepping new) */
int CZKit::GetAmmoCount(int index)
{
    int i = wrap(index, m_num_items);
    CZFTSWeapon *w = m_item[i];

    if (!w)
        return 0;
    if (WeaponIsLauncherMode(i)) {
        int j = FindItemWithAmmo((EQUIP_ITEM)m_firemode[i]);

        /* (the ternary this way round: `j == -1 ? 0 : ...` branches over a `b` to the end) */
        return j != -1 ? m_reloads[j][m_currentmag[j]] : 0;
    }
    if (CZWeapon::IsLauncher(w->m_ID)) {
        int sel = m_selected_item;

        m_selected_item = i;
        CycleFireMode();
        if (!GetCurItem()->IsLauncherWeapon())
            CycleFireMode();
        m_selected_item = sel;
        if (m_firemode[i] > AUTOFIRE)
            i = FindItemWithAmmo((EQUIP_ITEM)m_firemode[i]);
        if (i < 0 || i > m_num_items)
            return -1;
        return m_reloads[i][m_currentmag[i]];
    }
    return m_reloads[i][m_currentmag[i]];
}

/* (OVL2 0x5C3890; SOCOM 1's, a tail call here) */
int CZKit::GetCurAmmoCount()
{
    return GetAmmoCount(m_selected_item);
}

/* the current item's fire mode set, when the item has it and the ammunition for it -- a grenade
 * launcher's modes past 3 always (OVL2 0x5C37B0; SOCOM 1's) */
bool CZKit::SetCurFireMode(FIREMODE mode)
{
    CZFTSWeapon *w = GetCurItem();

    if (!w)
        return false;
    if ((mode > AUTOFIRE && w->IsLauncherWeapon()) ||
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

extern "C" {
/* a launcher's fire mode past 3 found for an item (OVL2 0x5C45C0, this unit's draft) and a
 * weapon's fire mode allowed (CZWeapon::HasFireMode, OVL1 0x3D2A60: the weapons') */
bool func_005C45C0(CZKit *kit, int index);
bool HasFireMode__8CZWeaponCFi(const CZWeapon *w, int mode);
}

/* the selected item's fire mode stepped, unless it is zoomed past 1.01: past the item's last mode a
 * launcher takes its next mode (and steps again from 0 when it has a launcher's mode past 3), any
 * other item with modes goes back to 1; a mode the item lacks stepped over, else the weapon info
 * and reticule marked changed and the reticule taken; then -- the item a rifle, or a sidearm in the
 * second hand -- the hand's mode kept, and its item kind while the body lives (OVL2 0x5C4600;
 * SOCOM 1's CycleFireMode, the zoom test, the launcher's mode from NextFireMode and the hand's
 * tail new) */
void CZKit::CycleFireMode()
{
    if (m_item[m_selected_item] && GetCurZoom() <= 1.01f) {
        m_firemode[m_selected_item]++;
        /* (the last mode on the left: the mode takes v0, the last mode v1 -- `mode > last` swaps them) */
        if (m_item[m_selected_item]->m_maxfiremode < m_firemode[m_selected_item]) {
            if (m_item[m_selected_item]->IsLauncherWeapon()) {
                m_firemode[m_selected_item]--;
                m_firemode[m_selected_item] = NextFireMode(m_selected_item);
                if (m_firemode[m_selected_item] == 0 && func_005C45C0(this, m_selected_item))
                    CycleFireMode();
            } else if (m_item[m_selected_item]->m_maxfiremode)
                m_firemode[m_selected_item] = 1;
        }
        if (!HasFireMode__8CZWeaponCFi(m_item[m_selected_item], m_firemode[m_selected_item]))
            CycleFireMode();
        else {
            m_weaponinfoChanged = true;
            m_reticuletypeChanged = true;
            m_curitemreticule = GetReticuleByIndex(m_selected_item);
        }
    }
    if (m_item[m_selected_item] && (CZWeapon::IsPrimary(m_item[m_selected_item]->m_ID) ||
                                    (m_second_hand && CZWeapon::IsSecondary(m_item[m_selected_item]->m_ID)))) {
        m_hand_firemode = m_firemode[m_selected_item];
        if (((CKitBody *)m_body)->m_alive)
            m_hand_item = m_item[m_selected_item] ? m_item[m_selected_item]->m_ID : m_hand_item;
    }
}

/* a weapon's byte at 0x24 (the view and the name ours) */
struct CZKitModeView {
    unsigned char _0[0x24];
    unsigned char m_24;   // 0x24
};

/* the next item past an index of a launcher's ammunition class, with rounds, other than the kind in use -- its kind is the
 * fire mode -- else the weapon's own mode (inlined per class; the name ours) */
inline int NextLauncherMode(CZKit *kit, int index, int cls, EQUIP_ITEM kind)
{
    for (int i = index + 1; i < kit->m_num_items; i++)
        if (kit->m_item[i] && func_003C5B20(kit->m_item[i], cls) && kit->ItemHasAmmo(i) && kind != kit->m_item[i]->m_ID)
            return kit->m_item[i]->m_ID;
    if (((CZKitModeView *)kit->m_item[kit->m_selected_item])->m_24)
        return 1;
    return 0;
}

/* a launcher's next fire mode (OVL2 0x5C3EE0, new in SOCOM II where SOCOM 1's FindNextGrenadeLauncherAmmo was: NextFireMode
 * and 0x5C45C0 call it -- the name unknown); -1 when the weapon has none */
int func_005C3EE0(CZKit *kit, int index, CZFTSWeapon *w, EQUIP_ITEM kind)
{
    if (w) {
        unsigned char id = w->m_ID;

        if (id == 143)
            return NextLauncherMode(kit, index, 1, kind);
        if (id == 142 || w->IsM203Weapon())
            return NextLauncherMode(kit, index, 2, kind);
        if (id == 145)
            return NextLauncherMode(kit, index, 3, kind);
        if (id == 146)
            return NextLauncherMode(kit, index, 4, kind);
        if (id == 63)
            return NextLauncherMode(kit, index, 5, kind);
    } else
        return -1;
    return -1;
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

    if (m_item[i] && m_item[i]->IsGrenadeLauncherAmmo()) {
        if (func_003C5C20(m_item[i]))
            i = GetClass145Index();
        else
            i = GetPrimaryIndex();
    }
    return m_item[i];
}

/* (OVL2 0x5C6980; pinned: ChangeItemModel, its only caller, calls it) */
#pragma auto_inline off
void CZKit::AttachItemNode(int kind, const char *part)
{
    CKitBody *body = (CKitBody *)m_body;
    CKitBody::Attach::Record *a = body->m_attach[(unsigned char)kind].m_rec;

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
#pragma auto_inline reset

extern "C" {
/* the body's carrying modes and its gear (seal_body.cpp's CZSealBody members, OVL2 0x5A70F0,
 * 0x5A7260, 0x58B040 -- spelled by their mangled names, CZSealBody being incomplete here) and the
 * hand a body's item goes to (OVL2 0x57D5E0, the SEAL lane's, unnamed) */
void ChangeToRifleMode__10CZSealBodyFb(CZSealBody *body, bool now);
void ChangeToPistolMode__10CZSealBodyFb(CZSealBody *body, bool now);
void SetPartAsGear__10CZSealBodyFP10CZBodyPartP14CCharacterGear(CZSealBody *body, void *part, unsigned int gear);
void func_0057D5E0(CZSealBody *body, int hand);
}

/* (OVL2 0x5C50B0: SOCOM 1's ChangeItemModel, its two modes, the item unread in both) -- at once,
 * the rifle's or the pistol's carrying mode by the first item's fire class (FireClassOf inlined),
 * the matching attachment's gear set and the hand told, then the fifth attachment's gear; else, by
 * kind: 1 or 2 asks for the item unless the body's byte at 0xF79 already says so (kind 1 sets the
 * bit from an int 1 -- the `andi` of the compare's register, as SOCOM 1's -- kind 2 from true), 4
 * asks whatever, 3 asks and hangs the node on "aimnodes", any other only hangs it */
void CZKit::ChangeItemModel(int item, int kind, int x, bool now)
{
    if (now) {
        CKitBody *body;

        if (FireClassOf(m_item[0]) == FIRE_CLASS_RIFLE) {
            ChangeToRifleMode__10CZSealBodyFb(m_body, true);
            body = (CKitBody *)m_body;
            SetPartAsGear__10CZSealBodyFP10CZBodyPartP14CCharacterGear(m_body, body->m_attach[2].m_rec,
                                                                        body->m_attach[2].m_4);
            func_0057D5E0(m_body, 1);
        } else {
            ChangeToPistolMode__10CZSealBodyFb(m_body, true);
            body = (CKitBody *)m_body;
            SetPartAsGear__10CZSealBodyFP10CZBodyPartP14CCharacterGear(m_body, body->m_attach[1].m_rec,
                                                                        body->m_attach[1].m_4);
            func_0057D5E0(m_body, 2);
        }
        body = (CKitBody *)m_body;
        SetPartAsGear__10CZSealBodyFP10CZBodyPartP14CCharacterGear(m_body, body->m_attach[4].m_rec,
                                                                    body->m_attach[4].m_4);
        return;
    }
    switch (kind) {
    case 1:
        if (((CKitBody *)m_body)->m_f79 != 1) {
            m_requested_item = 1;
            m_bit13 = false;
        }
        break;
    case 2:
        if (((CKitBody *)m_body)->m_f79 != 2) {
            m_requested_item = true;
            m_bit13 = false;
        }
        break;
    case 4:
        m_requested_item = true;
        m_bit13 = false;
        break;
    case 3:
        m_requested_item = true;
        m_bit13 = false;
        AttachItemNode(kind, "aimnodes");
        break;
    default:
        AttachItemNode(kind, "aimnodes");
        break;
    }
}

/* whether a mission's world is at night (the world's byte at 0x5DC: the HUD's view; the name ours) and the scope settings
 * for a primary and a sidearm: whether each is forced, and to what (OVL1 data; the names unknown) */
struct CKitWorldView {
    unsigned char _0[0x5dc];
    unsigned char m_night_mission;   // 0x5dc
};

extern unsigned char data_00436674;
extern int data_00436678;
extern unsigned char data_0043667C;
extern int data_00436680;

extern "C" {
void SetCurZoomState__10CZSealBodyF9ZOOMSTATE(CZSealBody *body, int state);
void WepWaitOn__10CZSealBodyFv(CZSealBody *body);
void WepWaitOff__10CZSealBodyFv(CZSealBody *body);
}

/* an item made the current one: the same one only checks its reticle; else a night scope view left unless the item's
 * forced setting keeps it, the zoom reset, the hot-swap flag marking a change, the wait for the weapon on or off by its
 * class, the models changed, the reticle and the weapon information updated, the throw flag cleared, the camera's zoom
 * eased back from a scope, and a fire mode chosen when the item has none (OVL2 0x5C4B10; SOCOM 1's) */
void CZKit::SetCurItemByIndex(int index)
{
    int i = wrap(index, m_num_items);

    if (i == -1)
        return;
    if (i == m_selected_item) {
        SetCurReticule(false);
        return;
    }
    if (((CKitBody *)m_body)->m_200 == 3) {
        unsigned char night = zdb::CWorld::m_world ? ((CKitWorldView *)zdb::CWorld::m_world)->m_night_mission : 0;
        CZFTSWeapon *w = (CZFTSWeapon *)GetItemByIndex(i);

        if (data_00436674 && CZWeapon::IsPrimary(w->m_ID))
            night = data_00436678 != 0;
        else if (data_0043667C && CZWeapon::IsSecondary(w->m_ID))
            night = data_00436680 != 0;
        if (!night)
            SetCurZoomState__10CZSealBodyF9ZOOMSTATE(m_body, 1);
    }
    m_cur_zoom = 0;
    int prev = GetSelectedItemNum();

    m_hotswap = index != prev;
    m_selected_item = i;
    if (m_selected_item >= m_num_items)
        m_selected_item = 0;
    EQUIP_ITEM t = m_item[m_selected_item]->GetIDType();

    if (CZWeapon::IsPrimaryType(t) || CZWeapon::IsSecondaryType(t) || CZWeapon::IsLauncherType(t))
        WepWaitOn__10CZSealBodyFv(m_body);
    else
        WepWaitOff__10CZSealBodyFv(m_body);
    EQUIP_ITEM id = m_item[m_selected_item]->m_ID;

    ChangeItemModel(FireClassOfId(m_item[prev]->m_ID), FireClassOfId(id), prev, false);
    CKitCtrl *ctrl = ((CKitBody *)m_body)->m_control;

    ctrl->SetBombPlaced(true);
    SetCurReticule(false);
    m_weaponinfoChanged = true;
    ctrl->SetThrowing(false);
    func_00592D40(ctrl);
    if (GetCurZoom() > 1.01f)
        SetCameraZoom__9CSealCtrlFff(ctrl, 1.0f, 0.5f);
    m_cur_zoom = 0;
    if (m_firemode[m_selected_item] == 0)
        CycleFireMode();
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
    CZWeapon::RegisterTimer2(&m_fire_delay, m_fire_delay, 0);
    m_alt_firedelay = alt;
}

extern "C" {
/* the SEAL's held item put through its reload, false when it cannot run (seal_env.cpp's, OVL2
 * 0x5A82E0 -- the kit's Reload calls it three times) */
bool Reload__10CZSealBodyFb(CZSealBody *body, bool b);
}

/* the launcher's next round chambered: with the grenade launcher's mode on item 146, its
 * ammunition's item (the first of the mode's kind with rounds) steps on to its next magazine
 * holding any, an empty one stopping it; then the SEAL's reload run and the item's reload sound
 * (the weapon's word at 0xA0) played at the SEAL (OVL2 0x5C3000, new in SOCOM II: the kit's Tick
 * calls it; the name ours) */
void CZKit::ChamberLauncherRound()
{
    EQUIP_ITEM item = m_item[m_selected_item]->m_ID;
    bool ok = true;

    if (CurWeaponIsLauncherMode() && item == 146) {
        int i = FindItemWithAmmo((EQUIP_ITEM)GetCurFireMode());

        if (i != -1) {
            int mag = m_currentmag[i] + 1;

            if (ItemHasAmmo(i)) {
                bool found = false;

                if (mag > 9)
                    mag = 0;
                while (mag != m_currentmag[i] && !found) {
                    if (m_reloads[i][mag] > 0)
                        found = true;
                    else if (++mag > 9)
                        mag = 0;
                }
            }
            m_currentmag[i] = mag;
            if (m_reloads[i][m_currentmag[i]] == 0)
                ok = false;
        } else
            ok = false;
    }
    if (ok) {
        int sel = m_selected_item;

        Reload__10CZSealBodyFb(m_body, true);
        CSnd *snd = (CSnd *)m_item[sel]->m_socom2_a0;

        if (snd)
            snd->PlaySound3D(&((CEntity *)m_body)->m_posW, 1.0f, 0, 0, 255);
    }
}

extern "C" {
/* the body's tests Reload makes (seal_env.cpp's: OVL2 0x5A7D10 IsThrowing, 0x5A7AB0 IsReloading --
 * spelled by their mangled names, CZSealBody being incomplete here -- and 0x5A78D0 between them,
 * unnamed) and a state the body may leave for 3 (OVL2 0x5B4340, the SEAL lane's, unnamed) */
bool IsThrowing__10CZSealBodyFb(CZSealBody *body, bool started);
bool IsReloading__10CZSealBodyFv(CZSealBody *body);
bool func_005A78D0(CZSealBody *body);
bool func_005B4340(CZSealBody *body, int state);
}

/* the network as Reload reads it (zseal.h's CZNetwork, which this unit does not include: SOCOM 1's
 * m_bSessionMaster and m_bNetwork bytes, OVL1 bss 0x45A0C0) and the network game's state (OVL1 bss
 * 0x437CE8; hud_ammo.cpp's view: whether players respawn, the byte at 0xDC) */
class CZNetwork {
public:
    unsigned char m_bSessionMaster;   // 0x0
    unsigned char m_bNetwork;   // 0x1
};
extern CZNetwork theNetwork;

class CKitNetGame {
public:
    unsigned char _0[0xdc];
    unsigned char m_respawns;   // 0xdc
};
extern "C" CKitNetGame *data_00437CE8;

/* the body off the ground, as a bool (the target materialises the bit: sltu; the name ours) */
static inline bool Airborne(CKitBody *body)
{
    return body->m_airborne ? true : false;
}

/* the item in hand reloaded (OVL2 0x5C2A90; SOCOM 1's Reload, its tests and walk the same): not
 * without a body, airborne, dead, throwing, reloading, 0x5A78D0 or carrying (state 9), nor when the
 * body cannot take state 3 or looks through binoculars; under the grenade launcher's mode the
 * launcher's ammunition item is the one reloaded. With rounds left, the next magazine holding any
 * becomes the current one, the reload run and its sound played; online, a weapon with none left
 * besides has its magazine refilled for nothing -- a pistol always, a primary weapon but items 142
 * and 143 while players respawn. Either way the weapon's display is told and true
 * is returned -- false when the reload cannot run */
bool CZKit::Reload()
{
    if (!m_body || Airborne((CKitBody *)m_body) || !((CKitBody *)m_body)->m_alive
        || IsThrowing__10CZSealBodyFb(m_body, false) || IsReloading__10CZSealBodyFv(m_body)
        || func_005A78D0(m_body) || ((CKitBody *)m_body)->m_state == 9)
        return false;
    if (!func_005B4340(m_body, 3))
        return false;
    if (IsUsingBinocs())
        return false;
    int index = m_selected_item;
    bool reloaded = false;

    if (CurWeaponIsLauncherMode()) {
        int i = FindItemWithAmmo((EQUIP_ITEM)GetCurFireMode());

        if (i != -1)
            index = i;
    }
    if (ItemHasAmmo(index)) {
        int mag = m_currentmag[index] + 1;
        bool found = false;

        if (mag > 9)
            mag = 0;
        while (mag != m_currentmag[index] && !found) {
            if (m_reloads[index][mag] > 0)
                found = true;
            else if (++mag > 9)
                mag = 0;
        }
        if (found) {
            if (!Reload__10CZSealBodyFb(m_body, false))
                return false;
            m_currentmag[index] = mag;
            CSnd *snd = m_item[index]->m_reloadsound;

            if (snd)
                snd->PlaySound3D(&((CEntity *)m_body)->m_posW, 1.0f, 0, 0, 255);
        } else if (theNetwork.m_bNetwork
               && (m_item[index]->GetIDType() == 4
                   || CZWeapon::IsPrimary(m_item[index]->m_ID) && m_item[index]->m_ID != 142
                          && m_item[index]->m_ID != 143 && data_00437CE8->m_respawns)) {
            m_reloads[index][m_currentmag[index]] = m_item[index]->GetAmmoCapacity();
            if (!Reload__10CZSealBodyFb(m_body, false))
                return false;
            CSnd *snd = m_item[index]->m_reloadsound;

            if (snd)
                snd->PlaySound3D(&((CEntity *)m_body)->m_posW, 1.0f, 0, 0, 255);
        }
        m_weaponinfoChanged = true;
        reloaded = true;
    } else if (theNetwork.m_bNetwork
               && (m_item[index]->GetIDType() == 4
                   || CZWeapon::IsPrimary(m_item[index]->m_ID) && m_item[index]->m_ID != 142
                          && m_item[index]->m_ID != 143 && data_00437CE8->m_respawns)) {
        m_reloads[index][m_currentmag[index]] = m_item[index]->GetAmmoCapacity();
        if (!Reload__10CZSealBodyFb(m_body, false))
            return false;
        CSnd *snd = m_item[index]->m_reloadsound;

        if (snd)
            snd->PlaySound3D(&((CEntity *)m_body)->m_posW, 1.0f, 0, 0, 255);
        m_weaponinfoChanged = true;
        reloaded = true;
    }
    return reloaded;
}

/* every remote charge the SEAL set down within 500 of him set off, then the detonator (153) taken
 * up and the count stepped (OVL2 0x5C0130; SOCOM 1's DetonateRemoteExplosives, its unused pod
 * vector kept -- zeroed at entry, freed at the end -- the position read straight off the SEAL where
 * SOCOM 1 asked a virtual, and the remote test PlacedExplosive's own). The walk's begin and end
 * initialised together, the iterator first: the end declared ahead swaps their registers */
void CZKit::DetonateRemoteExplosives()
{
    std::vector<unsigned int> unused;
    CZProjectileList *list = CZWeapon::m_pProjectilelist;
    unsigned int id = ((CEntity *)m_body)->m_id;

    for (std::vector<PlacedExplosive *>::iterator it = list->m_explosives.begin(), e = list->m_explosives.end(); it != e; ++it) {
        PlacedExplosive *x = *it;

        if (id == x->m_shooterID && x->IsRemote() && x->IsInRange(&((CEntity *)m_body)->m_posW, 500.0f))
            x->RemoteDetonate();
    }
    SetCurItemEnum((EQUIP_ITEM)153);
    m_burst_count++;
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
    if (CanFire__10CZSealBodyFbb(m_body, false, false) && m_81c <= 0.0f && CurMagHasAmmo()) {
        HandleFireWeapon(m_selected_item, a, b);
        return;
    }
    SetCurItemByIndex(m_last_selected);
    m_hotswap = false;
}

/* the AI maps' laser-target names (CAiMaps::m_names[3], AiMaps+0x60: a vector of CAiMap::name, 0x18
 * bytes -- ai_maps.h's, which this unit does not include: the views ours), the anim's state byte
 * (zanim.h's _zanim_anim_params::m_state) and the SEAL's aim point (zseal.h's CZSealBody at
 * 0x1458, SOCOM 1's m_aim_point after m_aim_dir and m_aim_goal) */
struct CKitMapName {
    CAiMapLoc m_loc;   // 0x0
    char m_name[16];   // 0x4
    unsigned char m_14;   // 0x14
    unsigned char m_15;   // 0x15
};
struct CKitAiMapsView {
    unsigned char _0[0x60];
    std::vector<CKitMapName> m_lase_names;   // 0x60
};
struct CKitAnimView {
    unsigned char _0[3];
    unsigned char m_state;   // 0x3
};
struct CKitAimView {
    unsigned char _0[0x1458];
    CPnt3D m_aim_point;   // 0x1458
};
extern CKitAiMapsView AiMaps;
class CZAnimMain;
extern CZAnimMain ZAnim;

extern "C" {
/* CAiMaps::GetLocation(const CPnt3D *, CAiMapLoc &) and GetCellCenter (OVL2 0x52DC70, 0x52D480),
 * CZAnimMain::GetAnim(name, set) (OVL1 0x26A250) */
bool GetLocation__7CAiMapsFPC6CPnt3DR9CAiMapLoc(CKitAiMapsView *maps, const CPnt3D *pos, CAiMapLoc &loc);
CPnt3D *GetCellCenter__7CAiMapsFRC9CAiMapLocP6CPnt3D(CKitAiMapsView *maps, const CAiMapLoc &loc, CPnt3D *c);
CZAnim *GetAnim__10CZAnimMainFPCcPCc(CZAnimMain *zanim, const char *name, const char *set);
/* CZAnim::Activate(CZAnim *by) (OVL1 0x2734B0: TickLaserDesignator sets the lased anim off) */
CZAnim *Activate__6CZAnimFP6CZAnim(CZAnim *anim, CZAnim *by);
/* the laser's reach (an int, converted at the compare) and its time (OVL1 data 0x3DF1E0, 0x3DF1D8) */
extern int data_003DF1E0;
extern float data_003DF1D8;
}

/* (OVL2 0x5BFE60; SOCOM 1's FetchLaseTarget, its grid distance a point's squared distance in SOCOM II).
 * The SEAL in a local declared first (its load leads the zero's copy), the location's bool tested
 * `== false` (the andi), the names through a reference (their address computed once) */
bool CZKit::FetchLaseTarget()
{
    CKitAimView *body = (CKitAimView *)m_body;
    CPnt3D c = CPnt3D::zero;
    CAiMapLoc loc;

    if (GetLocation__7CAiMapsFPC6CPnt3DR9CAiMapLoc(&AiMaps, &body->m_aim_point, loc) == false)
        return false;
    std::vector<CKitMapName> &names = AiMaps.m_lase_names;

    for (std::vector<CKitMapName>::iterator n = names.begin(), e = names.end(); n != e; ++n) {
        if (GetCellCenter__7CAiMapsFRC9CAiMapLocP6CPnt3D(&AiMaps, n->m_loc, &c)) {
            CPnt3D d;

            c.Sub(&((CEntity *)m_body)->m_posW, &d);
            if (d.x * d.x + d.y * d.y + d.z * d.z < (float)data_003DF1E0) {
                const char *name = n->m_name;
                CZAnim *a = name ? GetAnim__10CZAnimMainFPCcPCc(&ZAnim, name, 0) : 0;

                if (a && ((CKitAnimView *)a)->m_state == 1) {
                    m_lase_mapname = &*n;
                    m_lase_anim = a;
                    m_lase_timer = 0.0001f;
                    m_lase_echo = true;
                    CZWeapon::RegisterTimer(&m_lase_timer, data_003DF1D8, true);
                    return true;
                }
            }
        }
    }
    return false;
}

/* a placed C4's timer callback: its clock word cleared (SOCOM 1's Timer_PlaceC4, word for word; OVL2
 * 0x5BFE40 -- HandleFireWeapon 0x5BE9A0 passes its address) */
void Timer_PlaceC4(void *, void *data)
{
    if (data)
        *(int *)data = 0;
}

/* the SEAL controller's trigger (zseal.h's CSealCtrl::m_trigger at 0x118, a byte enum: 0 up, 1 down,
 * 3 rising -- zseal.h is not included here; the view ours) */
struct CKitTriggerView {
    unsigned char _0[0x118];
    unsigned char m_trigger;   // 0x118
    unsigned char _119[0x11c - 0x119];
    float m_11c;   // 0x11c: the float the trigger's pull hands HandleFireWeapon (zseal.h's; the name ours)
};

/* a map cell's grid distance from another, x and y steps summed (seal_aifollow.cpp's FollowCellDist;
 * the name ours) */
static inline int KitCellDist(const CAiMapLoc &a, const CAiMapLoc &b)
{
    int dx = a.x > b.x ? a.x - b.x : b.x - a.x;
    int dy = a.y > b.y ? a.y - b.y : b.y - a.y;

    return dx + dy;
}

/* the laser designator's tick (SOCOM 1's TickLaserDesignator, its grid test kept; SOCOM II plays the
 * lock sound for a player and stops it once the lase is gone, where SOCOM 1 played the error sound on
 * the anim's start; OVL2 0x5BFAC0). Without a target the lock sound is stopped and released. With
 * the trigger down the aim must stay in the target's cell's reach and the anim ready, the timer
 * runs; up or rising, the lase is dropped; past its time the target's anim is set off. The location's
 * bool tested `!= false` (the target's andi before the branch; plain, MW branches on v0) */
void CZKit::TickLaserDesignator(float dT)
{
    if (!m_lase_mapname || !m_lase_anim) {
        if (KitIsPlayer((CKitBody *)m_body) && m_lase_inst) {
            m_lase_inst->Stop();
            CSndInstance::ReleaseSoundInstance(m_lase_inst);
            m_lase_inst = 0;
        }
        return;
    }
    unsigned char trig = ((CKitTriggerView *)((CEntity *)m_body)->m_control)->m_trigger;

    if (trig == 1) {
        CAiMapLoc loc;
        bool ok = false;

        if (GetLocation__7CAiMapsFPC6CPnt3DR9CAiMapLoc(&AiMaps, &((CKitAimView *)m_body)->m_aim_point, loc) != false)
            ok = true;
        if (ok && KitCellDist(m_lase_mapname->m_loc, loc) < data_003DF1E0) {
            if (((CKitAnimView *)m_lase_anim)->m_state != 1)
                ok = false;
        } else
            ok = false;
        if (!ok)
            ClearLase();
        if (ok) {
            m_lase_timer += dT;
            if (KitIsPlayer((CKitBody *)m_body)) {
                if (m_lasesnd_start && !m_lase_inst)
                    m_lase_inst = m_lasesnd_start->PlaySound((CSndInstance *)0, 255);
                if (m_lase_inst2) {
                    m_lase_inst2->Stop();
                    m_lase_inst2 = 0;
                }
            }
        } else
            ClearLase();
    } else if (trig == 0)
        ClearLase();
    else if (trig == 3)
        ClearLase();
    if (m_lase_timer > data_003DF1D8) {
        Activate__6CZAnimFP6CZAnim(m_lase_anim, 0);
        m_lase_deploy_echo = true;
        ClearLase();
    }
}

/* the aim raised by a step and rescaled so it stays a unit vector, false past straight up; the
 * horizontal part's length handed back (inline; the name ours). `s` declared ahead of the squares'
 * sum (the sum takes z's register, the scale the next) and the length computed after the three
 * stores (before them MW schedules it ahead of x's store, its product in f1) */
static inline bool RaiseAim(CPnt3D &aim, float step, float &horiz)
{
    float y = aim.y + step;

    if (y > 1.0f)
        return false;
    float s;
    float xz = aim.x * aim.x + aim.z * aim.z;

    s = sqrtf((1.0f - y * y) / xz);
    aim.x *= s;
    aim.y = y;
    aim.z *= s;
    horiz = sqrtf(xz * (s * s));
    return true;
}

/* a point's length in the horizontal (inline; the name ours): LobAim's distance, as its result
 * temporary, is numbered after the named locals -- $f20 under the length's $f21 (a plain
 * `sqrtf(...)` local swaps them) */
static inline float LengthXZ(const CPnt3D &p)
{
    return sqrtf(p.x * p.x + p.z * p.z);
}

/* (OVL2 0x5BF8A0, new in SOCOM II: the fire path 0x5BE9A0 calls it for an AI SEAL's weapon with its
 * muzzle velocity -- the aim raised until the round, falling at 98 a second squared, is within 5 of
 * the target's height at its distance, at most 12 steps of 0.025; the name ours). The constants are
 * function-local statics, as the data's 8-byte spacing shows (OVL2 0x650958-0x650978). The height
 * adds a zero the target keeps in a register (`mtc1 zero` / add.s): a literal 0.0f folds away -- a
 * zero local is a marked stand-in for whatever the original added (an offset, a scalarised point's
 * y); the body unread */
void CZKit::LobAim(CZSealBody *body, const CPnt3D *from, const CPnt3D *to, CPnt3D *dir, float speed)
{
    static float gravity = 98.0f;
    static int tries = 12;
    static float step = 0.025f;
    static float target = 0.0f;
    static float tolerance = 5.0f;
    CPnt3D d = *to - *from;
    float zero = 0.0f;
    float height = d.y + zero;
    float dist = LengthXZ(d);

    d.Normalize();
    CPnt3D aim = *dir;
    float horiz;

    for (int i = 0; i < tries; i++) {
        if (!RaiseAim(aim, step, horiz))
            break;
        horiz *= speed;
        if (horiz <= 0.0f)
            break;
        float t = dist / horiz;
        float err = speed * aim.y * t - 0.5f * gravity * (t * t) - height;

        if (err > 0.0f)
            break;
        if (_close_enough(err, target, tolerance))
            break;
    }
    *dir = aim;
}

extern "C" {
/* the controller's high rumble (CSealCtrl::SetHighRumbleTime, OVL2 0x592BA0: the SEALs'), the target
 * mods' reported burst shot (an int at 0x6C, new in SOCOM II: OVL1 0x3C5AB0) and the body's recoil
 * counter raised by the weapon's recoil share (CZWeapon::GetRecoilPct, OVL1 0x3D2000), to 10 at most
 * (a float at 0x378: OVL2 0x57D510) -- the weapons' and the SEALs' rows, unnamed */
void SetHighRumbleTime__9CSealCtrlFf(CKitCtrl *ctrl, float time);
void SetLowRumbleTime__9CSealCtrlFff(CKitCtrl *ctrl, float power, float time);
int func_003C5AB0(CTargetMods *mods);
void func_0057D510(CZSealBody *body, float step);
}

/* the knock's vertical screen scale, kept (SOCOM 1's recoil_constscl; OVL2 bss 0x66B678) */
static float recoil_constscl;

/* a point copied field by field through its address (the target keeps &a.y and &a.z in saved
 * registers across the turret's call; the name ours) */
static inline void CopyPoint(CPnt3D *dst, const CPnt3D *src)
{
    dst->x = src->x;
    dst->y = src->y;
    dst->z = src->z;
}

/* (OVL2 0x5C3360: kit.h's; SOCOM 1's Recoil) */
void CZKit::Recoil(CZFTSWeapon *w)
{
    if (!KitIsPlayer((CKitBody *)m_body))
        return;
    CTargetMods *mods = w->TargetMods(GetStance__10CZSealBodyCFb(m_body, true));
    float knock = mods->GetReticuleKnock();
    float knockmax = mods->GetReticuleKnock_Max();
    float dilate = mods->GetTarget_DilateUponFire();
    float tmin = mods->GetTarget_Min();
    float tmax = mods->GetTarget_Max();
    int n = m_burst_count + 1 - w->AccuracyBurstCnt_Min();
    float burst = 0.0f;

    if (n > 0) {
        float shots = w->AccuracyBurstCnt_Max() < n ? (float)w->AccuracyBurstCnt_Max() : (float)n;

        burst = shots * w->m_accuracy_slope;
    }
    if (w->GetRumbleHigh_Enable()) {
        CKitCtrl *ctrl = ((CKitBody *)m_body)->m_control;

        SetHighRumbleTime__9CSealCtrlFf(ctrl, w->GetRumbleHigh_Time());
    }
    if (w->GetRumbleLow_Enable()) {
        CKitCtrl *ctrl = ((CKitBody *)m_body)->m_control;

        SetLowRumbleTime__9CSealCtrlFff(ctrl, w->GetRumbleLow_Time(), w->GetRumbleLow_Power());
    }
    signed char zc = ((CKitBody *)m_body)->m_200;

    if (zc > 4 || zc == 4)
        return;
    int shot = func_003C5AB0(mods);

    if (m_burst_count == shot)
        m_retoffsety -= knock * mods->GetWiggleReport();
    else if (shot < m_burst_count)
        m_retoffsety -= knock;
    zdb::CCamera *cam = zdb::CWorld::m_world->m_camera;
    float aspx = cam->m_screenAspect[0];
    float scl = cam->m_screenAspect[1];
    CZSealBody *p = ftsGetPlayer__Fv();
    CPnt3D a;
    CPnt3D b;

    CopyPoint(&a, &((CKitPlayerView *)p)->m_1458);

    ((CKitPlayerView *)p)->m_14b0.Add(&((CEntity *)p)->m_posW, &b);
    if (((CKitBody *)m_body)->m_eb8) {
        ((CEntity *)((CKitBody *)m_body)->m_eb8)->Virtual60(&b);
        CopyPoint(&a, &b);
    }
    float z = cam->GetZ(&a);
    float d = z - cam->GetZ(&b);

    scl = d == 0.0f ? scl : scl * d / z;
    recoil_constscl = scl;
    float ylim = knockmax * scl;
    float xlim = knockmax * aspx;

    if (m_retoffsety > ylim)
        m_retoffsety = ylim;
    else if (m_retoffsety < -ylim)
        m_retoffsety = -ylim;
    if (m_retoffsetx > xlim)
        m_retoffsetx = xlim;
    else if (m_retoffsetx < -xlim)
        m_retoffsetx = -xlim;
    m_prev_retsize = m_retsize;
    m_retsize += dilate + dilate * burst;
    if (m_retsize < tmin)
        m_retsize = tmin;
    else if (m_retsize > tmax)
        m_retsize = tmax;
    func_0057D510(m_body, w->GetRecoilPct());
}

/* (OVL2 0x5C2670: kit.h's; SOCOM 1's TickReticule). One rate variable for the offsets' return and the
 * size's constriction, each a call result multiplied by dT in place: MW splits the reused variable into
 * a new web numbered after every temporary, so both rates take $f0 and their multiplies sink into the
 * branches' delay slots (two locals, or the products written in one statement, take $f2); the item's
 * class declared uninitialised ahead of the cast (numbered before the weapon: the class in s3) */
void CZKit::TickReticule(float dT)
{
    EQUIP_IDTYPE type;

    if (!KitIsPlayer((CKitBody *)m_body))
        return;
    CZFTSWeapon *w = dynamic_cast<CZFTSWeapon *>((CZWeapon *)m_item[m_selected_item]);

    type = IDTYPE_NONE;

    if (w)
        type = (EQUIP_IDTYPE)w->GetIDType();
    if (!w)
        return;
    CTargetMods *mods = w->TargetMods(GetStance__10CZSealBodyCFb(m_body, true));

    if (type == IDTYPE_EXPLOSIVE || type == IDTYPE_GRENADE || type == IDTYPE_BINOCULARS || type == 201) {
        m_retmin = 0.0f;
        m_retmax = 0.0f;
    } else {
        m_retmin = mods->GetTarget_Min();
        m_retmax = mods->GetTarget_Max();
    }
    if (m_retsize < m_retmin)
        m_retsize = m_retmin;
    float rate = mods->GetReticuleKnock_Return();

    rate *= dT;

    if (m_retoffsetx > 0.0f) {
        m_retoffsetx -= rate;
        if (m_retoffsetx < 0.0f)
            m_retoffsetx = 0.0f;
    }
    if (m_retoffsetx < 0.0f) {
        m_retoffsetx += rate;
        if (m_retoffsetx > 0.0f)
            m_retoffsetx = 0.0f;
    }
    if (m_retoffsety > 0.0f) {
        m_retoffsety -= rate;
        if (m_retoffsety < 0.0f)
            m_retoffsety = 0.0f;
    }
    if (m_retoffsety < 0.0f) {
        m_retoffsety += rate;
        if (m_retoffsety > 0.0f)
            m_retoffsety = 0.0f;
    }
    CKitMover *e = ((CKitBody *)m_body)->m_eb8;

    if (!e)
        e = (CKitMover *)m_body;
    float dilate = 1.0f / 65.0f * (e->m_velM.x * e->m_velM.x + e->m_velM.y * e->m_velM.y + e->m_velM.z * e->m_velM.z)
                   * mods->GetTarget_DilateUponMovementMult();

    dilate += 30.0f * (5.29f * (e->m_velR.x * e->m_velR.x + e->m_velR.y * e->m_velR.y + e->m_velR.z * e->m_velR.z));
    dilate += 30.0f * (5.29f * (e->m_60 * e->m_60));

    if (Airborne((CKitBody *)m_body)) {
        CPnt3D &v = ((CKitBody *)m_body)->m_1350;

        dilate += 1.0f / 65.0f * (v.x * v.x + v.y * v.y + v.z * v.z);
    }
    float lim = m_retmin + (m_retmax - m_retmin);

    if (dilate > lim)
        dilate = lim;
    if (dilate < m_retmin)
        m_retsizegoal = m_retmin;
    else if (dilate > m_retmax)
        m_retsizegoal = m_retmax;
    else
        m_retsizegoal = dilate;
    if (m_item[m_selected_item]) {
        rate = mods->GetTarget_Constrict();
        rate *= dT;

        m_prev_retsize = m_retsize;
        if (m_retsize < m_retsizegoal) {
            m_retsize += mods->GetTarget_DilateUponMovement();
            if (m_retsize > m_retsizegoal)
                m_retsize = m_retsizegoal;
        } else if (m_retsize > m_retsizegoal) {
            m_retsize -= rate;
            if (m_retsize < m_retsizegoal)
                m_retsize = m_retsizegoal;
        }
    }
}

extern "C" {
/* a part's place in its body (CBody::PartInSpace, OVL1 0x2869D0: the body's CBody base at 0x170) */
void PartInSpace__5CBodyCFP10CZBodyPartPC6CPnt3DP6CPnt3DP5CQuat(const void *body, void *part,
                                                                 const CPnt3D *off, CPnt3D *pos, CQuat *rot);
}

/* a point added to another, by value (an inline returning a named point: the sum's slot below the
 * caller's points, its copy into the caller's local -- the name ours) */
static inline CPnt3D Added(const CPnt3D &a, const CPnt3D *b)
{
    CPnt3D r;

    a.Add(b, &r);
    return r;
}

/* (OVL2 0x5C2430: kit.h's; the hand's place copied through its address -- its y kept in a saved
 * register for the hits' test, as the target does -- and each material bit tested `? true : false`,
 * materialised before its branch) */
zdb::IntersectStruct *CZKit::GetHandFloorHit(bool liquid)
{
    CPnt3D p;
    CPnt3D zero = CPnt3D::zero;
    CPnt3D pos = zero;
    CPnt3D off = zero;

    PartInSpace__5CBodyCFP10CZBodyPartPC6CPnt3DP6CPnt3DP5CQuat(((CKitBody *)m_body)->m_cbody,
                                                               ((CKitBody *)m_body)->m_rhand, &off, &pos, 0);
    ((CEntity *)m_body)->m_quat.Apply(&pos, &pos, 1);
    CPnt3D t = Added(pos, &((CEntity *)m_body)->m_posW);

    CopyPoint(&p, &t);
    zdb::DiIntersect *di = zdb::CWorld::m_world->GetAltitudes(&p);
    zdb::IntersectStruct *below = 0;
    zdb::IntersectStruct *lowest = 0;

    if (di->m_Cnt) {
        float y = p.y;

        for (int i = 0; i < di->m_Cnt; i++) {
            zdb::IntersectStruct *hit = &di->m_Intersects[i];
            CMaterialEx *mat = CMaterialExArray::GetMaterialEx(hit->GetMaterial());

            if (mat) {
                if (mat->m_volumetric ? true : false)
                    continue;
                if ((mat->m_liquid ? true : false) && !liquid)
                    continue;
            }
            if (hit->m_pos.y <= y && (!below || !(hit->m_pos.y < below->m_pos.y)))
                below = hit;
            if (!lowest || !(lowest->m_pos.y < hit->m_pos.y))
                lowest = hit;
        }
    }
    if (!below)
        below = lowest;
    return below;
}

/* the fire delay still running (OVL2 0x5C2400, new in SOCOM II; the name ours) */
bool CZKit::IsFireDelayed()
{
    return m_fire_delay > 0.0f;
}

extern "C" {
/* the light maps' white switch (a byte at OVL1 0x4835E0 set: OVL1 0x326C20, unclaimed) */
void func_00326C20(int on);
}

/* the base fog colour by value, and a camera's fog set from a colour by value (thezoom.cpp's pair,
 * whose SeeFarther puts the fog back the same way; the names ours) */
static inline CPnt4D BaseFogColor() { return zdb::CVisual::m_basefog_color; }
static inline void SetFogColorBy(zdb::CCamera *cam, CPnt4D c) { cam->SetFogColor(c); }

/* (OVL2 0x5C1800: kit.h's; SOCOM 1's, the white switch and the tints' quarter second new -- SOCOM
 * 1's set the lens colour itself) */
void CZKit::Apply_LensEffects(bool on, CPnt3D *color)
{
    zdb::CCamera *cam = zdb::CWorld::m_world->m_camera;

    if (!on) {
        /* (a plain PNT4D: its address passed for the by-value colour, as thezoom.cpp's) */
        PNT4D white;

        white.x = 1.0f;
        white.y = 1.0f;
        white.z = 1.0f;
        white.w = 0.0f;
        zdb::CVisual::SetLightTints(white, 0.25f);
        func_00326C20(1);
        SetFogColorBy(cam, BaseFogColor());
        SetLensFxEnable(false);
    } else {
        if (color) {
            /* (the colour's own address passed for the by-value tint) */
            zdb::CVisual::SetLightTints(*(PNT4D *)color, 0.0f);
            func_00326C20(0);
            /* (the base colour read three times: MW shares the static's load, a temporary copied
             * once per channel -- one local, the copies gone, 0x20 short) */
            cam->m_fog_color = CPnt4D(color->x * BaseFogColor().x, color->y * BaseFogColor().y,
                                      color->z * BaseFogColor().z, 0.0f);
        }
        SetLensFxEnable(true);
    }
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

extern "C" {
/* (OVL1's sequence weight: the SEAL's weapon sequence at 0x1160 -- SOCOM 1's WillFireWeapon's) */
float Weight__9CSequenceFv(void *seq);
}
/* (whether the game is online: OVL1 data, the SEALs' name) */
extern unsigned char data_0045A0C1;

/* the kit's flag bits set (inline: each store keeps the bool's `andi 1` unfolded, as a setter's
 * argument; the names ours) */
inline void SetRequestedItem(CZKit *k, bool b) { k->m_requested_item = b; }
inline void SetBit13(CZKit *k, bool b) { k->m_bit13 = b; }
inline void SetReticuleTypeChanged(CZKit *k, bool b) { k->m_reticuletypeChanged = b; }

/* the local player's first frame as a pistol (OVL2 .data 0x650950, 1: read by Tick alone) */
extern unsigned char data_00650950;

/* the trigger down or held (inline: Tick's; the name ours) */
static inline bool TriggerHeld(unsigned char t)
{
    bool b = true;

    if (!(t == 1 || t == 2))
        b = false;
    return b;
}

/* (OVL2 0x5C0FD0: kit.h's; SOCOM 1's Tick) */
void CZKit::Tick(float dT)
{
    if (data_00650950 && KitIsPlayer((CKitBody *)m_body)) {
        data_00650950 = 0;
        ReParentToPistol();
    }
    TickReticule(dT);
    TickLaserDesignator(dT);
    if (!(m_pickupRequestWait <= 0.0f))
        m_pickupRequestWait -= dT;
    func_005BE6F0(this, dT);
    bool go = TickFireDelay(dT);

    if (go)
        go = TickExplosives(dT);
    if (!go)
        return;
    float b;
    bool fire = WillFireWeapon();
    float a = ((CKitTriggerView *)((CKitBody *)m_body)->m_control)->m_11c;

    b = GetFireDelay();
    int shots = GetShotCount();

    if (fire) {
        HandleFireWeapon(m_selected_item, a, b);
        if (m_waiting_to_raise)
            m_waiting_to_raise = false;
    } else if (m_item[m_selected_item] && !CurMagHasAmmo()) {
        if (UsesAmmo(m_selected_item) && m_item[m_selected_item]->IsDirectFire()) {
            bool more = m_burst_count < shots;
            signed char t = ((CKitTriggerView *)((CKitBody *)m_body)->m_control)->m_trigger;
            bool click;

            if (more)
                click = TriggerHeld(t);
            else
                click = false;
            bool refill = false;
            bool notb = false;
            bool nota = false;

            if (data_0045A0C1 && m_item[m_selected_item]->GetId() != 142)
                nota = true;
            if (nota && m_item[m_selected_item]->m_ID != 143)
                notb = true;
            if (notb) {
                bool k = m_item[m_selected_item]->GetIDType() == 4
                    || (data_00437CE8->m_respawns && CZWeapon::IsPrimaryType(m_item[m_selected_item]->GetIDType()));

                if (k)
                    refill = true;
            }
            if (click || refill) {
                if (m_clicksnd && m_burst_count < 2)
                    m_clicksnd->PlaySound((CSndInstance *)0, 255);
                if ((more && CurItemHasAmmo()) || refill) {
                    Reload();
                    if (refill && WillFireWeapon()) {
                        HandleFireWeapon(m_selected_item, a, b);
                        if (m_waiting_to_raise)
                            m_waiting_to_raise = false;
                    }
                } else if (more)
                    m_burst_count++;
            }
        }
    }
    m_81c -= dT;
    if (m_81c <= 0.0f)
        m_81c = 0.0f;
    if (!ItemHasAmmo(m_selected_item, (FIREMODE)m_firemode[m_selected_item]) && !WeaponIsLauncherMode(m_selected_item)) {
        if (m_item[m_selected_item] && m_item[m_selected_item]->IsLauncherWeapon() && m_firemode[m_selected_item] > 3)
            CycleFireMode();
    }
    if (!(FIREMODE)m_firemode[m_selected_item] && m_item[m_selected_item]) {
        int def = m_item[m_selected_item]->m_maxfiremode;

        if (def < 4 && def > 0) {
            while (m_firemode[m_selected_item] != def)
                CycleFireMode();
        } else
            CycleFireMode();
    }
    if (!(m_8 <= 0.0f)) {
        m_8 -= dT;
        if (m_8 < 0.0f)
            m_8 = 0.0f;
    }
    CKitBody::Attach::Record *rec = ((CKitBody *)m_body)->m_attach[4].m_rec;

    if (rec && rec->m_node) {
        zdb::CNode *node = (zdb::CNode *)rec->m_node;

        if (m_8bc == 0.0f)
            node->m_Opacity = 0.0f;
        if (node->m_Opacity < 1.0f)
            m_8bc += dT;
        if (!(m_8bc <= 0.75f)) {
            node->m_Opacity = 1.0f;
            m_8bc = -1.0f;
        }
    }
}

extern "C" {
/* whether a kind of weapon carries a silencer (67, 33, 105, 62, 16: OVL1 0x3C5AC0, unclaimed; the weapons') */
bool func_003C5AC0(EQUIP_ITEM id);
bool HasFireMode__8CZWeaponCFi(const CZWeapon *w, int mode);
}

/* the hand's item kind the one noted, its id read ahead of the note (an enum local: FirstFrame's; the name ours) */
static inline bool SameHandItem(CZKit *k)
{
    EQUIP_ITEM id = k->m_item[k->HandSlot()]->m_ID;

    return k->m_hand_item == id;
}

/* (OVL2 0x5C0250: kit.h's) */
void CZKit::FirstFrame(float dT)
{
    if (m_item[0] && m_item[0]->GetId() != 254) {
        ChangeItemModel(FireClassOfId(m_item[0]->GetId()), FireClassOfId(m_item[0]->GetId()), 0, true);
        SetCurItemByIndex(0);
    } else if (m_item[1] && m_item[1]->GetId() != 254) {
        ChangeItemModel(FireClassOfId(m_item[1]->GetId()), FireClassOfId(m_item[1]->GetId()), 0, true);
        SetCurItemByIndex(1);
    }
    UpdateScopeModel();
    int i = GetPrimaryIndex();

    if (i != -1) {
        bool silenced = func_003C5AC0(m_item[i]->GetId());
        zdb::CNode *n = ((zdb::CNode *)((CKitBody *)m_body)->m_attach[1].m_rec->m_node)->FindChild("silencer", true);

        if (n)
            n->SetActive(silenced);
    }
    bool second = true;

    if (((CKitBody *)m_body)->m_fd3 && m_hand_firemode && m_item[HandSlot()] && SameHandItem(this)) {
        SetHandFireMode(m_hand_firemode);
        if (m_second_hand)
            second = false;
    } else {
        CycleFireMode();
        if (m_item[0] && HasFireMode__8CZWeaponCFi(m_item[0], 2))
            m_firemode[0] = 2;
        m_hand_firemode = m_firemode[0];
        m_hand_item = m_item[0] ? m_item[0]->GetId() : EQUIP_NONE;
    }
    if (second && m_item[1] && HasFireMode__8CZWeaponCFi(m_item[1], 2))
        m_firemode[1] = 2;
    if (m_second_hand)
        m_hand_item = m_item[1] ? m_item[1]->GetId() : EQUIP_NONE;
    if (KitIsPlayer((CKitBody *)m_body) && data_0045A0C1) {
        for (int j = 0; j < g_iNumSavedFireModes; j++) {
            CSavedFireModeInfo *info = &g_arrSavedFireModeInfo[j];

            if (m_item[j]) {
                EQUIP_ITEM id = m_item[j]->m_ID;

                if (info->m_item == id)
                    m_firemode[j] = info->m_firemode;
            }
        }
    }
    for (int j = 0; j < m_num_items; j++) {
        if (m_item[j] && (CZWeapon::IsPrimary(m_item[j]->GetId()) || (m_second_hand && CZWeapon::IsSecondary(m_item[j]->GetId())))) {
            m_hand_firemode = m_firemode[j];
            m_hand_item = m_item[m_selected_item] ? m_item[m_selected_item]->GetId() : m_hand_item;
            break;
        }
    }
    m_first = false;
    SetReticuleTypeChanged(this, true);
}

/* whether the current item fires this frame: the trigger up (or let go) ends the burst and a player's lock sound; a held trigger or
 * a waiting raise fires once the weapon is up -- a direct-fire weapon raised first --, the burst not spent, the SEAL able
 * and the fire delay out; rounds in the magazine (or no ammunition to use); under six explosives laid online (OVL2
 * 0x5C0AE0; SOCOM 1's WillFireWeapon: CAiSDeploy::Tick and 0x563E10 call it) */
bool CZKit::WillFireWeapon()
{
#pragma opt_propagation off
    int shots;

    if (!m_item[m_selected_item])
        return false;
    EQUIP_IDTYPE t = (EQUIP_IDTYPE)m_item[m_selected_item]->GetIDType();
    bool notThrown = false;

    if (t != IDTYPE_GRENADE && t != IDTYPE_EXPLOSIVE)
        notThrown = true;
    unsigned char trigger = ((CKitTriggerView *)((CKitBody *)m_body)->m_control)->m_trigger;

    if (trigger == 0) {
        m_burst_count = 0;
        if (KitIsPlayer((CKitBody *)m_body) && m_lase_inst2) {
            m_lase_inst2->Stop();
            CSndInstance::ReleaseSoundInstance(m_lase_inst2);
            m_lase_inst2 = 0;
        }
    } else if (trigger == 3) {
        m_burst_count = 0;
        if (KitIsPlayer((CKitBody *)m_body) && m_lase_inst2) {
            m_lase_inst2->Stop();
            CSndInstance::ReleaseSoundInstance(m_lase_inst2);
            m_lase_inst2 = 0;
        }
    } else {
        shots = GetShotCount();
        GetFireDelay();
        bool go = true;

        if (!(trigger == 1 || trigger == 2 ? go : false) && !m_waiting_to_raise)
            go = false;
        if (go && !m_waiting_to_raise && Weight__9CSequenceFv((char *)m_body + 0x1160) < 1.0f && m_item[m_selected_item]->IsDirectFire()) {
            m_waiting_to_raise = true;
            return false;
        }
        if (go) {
            if (!(m_item[m_selected_item]->IsDirectFire() && Weight__9CSequenceFv((char *)m_body + 0x1160) < 1.0f)
                || m_item[m_selected_item]->GetIDType() == 205) {
                if (m_burst_count < shots && CanFire__10CZSealBodyFbb(m_body, true, notThrown) && m_81c <= 0.0f) {
                    if (CurMagHasAmmo() || !UsesAmmo(m_selected_item)) {
                        bool ok;

                        if (m_item[m_selected_item] && m_item[m_selected_item]->GetIDType() != 121
                            && m_item[m_selected_item]->GetIDType() != 151 && m_item[m_selected_item]->GetIDType() != 171
                            && m_item[m_selected_item]->GetIDType() != 185)
                            ok = true;
                        else {
                            CZProjectileList *list = CZWeapon::m_pProjectilelist;

                            if (list && data_0045A0C1 && !KitIsRemote((CKitBody *)m_body)
                                && list->GetNumExplosives(((CEntity *)m_body)->m_id) >= 6)
                                ok = false;
                            else
                                ok = true;
                        }
                        if (ok) {
                            if (CurWeaponIsLauncherMode() && !CurItemHasAmmo())
                                return false;
                            return true;
                        }
                    }
                }
            }
        }
    }
    return false;
#pragma opt_propagation reset
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

    return i != -1 ? m_item[i]->GetDescription() : 0;
}

/* a grenade launcher's ammunition's icon (OVL2 0x5BE1D0; SOCOM 1's GetFiremodeName and
 * GetFiremodeGraphic in one, the index no longer wrapped). One ternary over an `&&`: the target's
 * null test with the zero in its delay slot, then the call's branch over a branch (`bnez v0,L3; nop;
 * b L4; nop; L3: b end; lw v0,188(s0); L4: move v0,zero`) -- the `&&`'s true and false exits kept
 * as blocks of their own (an early return, nested ifs, `if (w && ...)` -- 8 differing --, a switch,
 * `== true`, a loop and a result local all fold them) */
void *CZKit::GetFiremodeGraphic(int index)
{
    if (index == -1)
        return 0;
    CZFTSWeapon *w = m_item[index];

    return w && w->IsGrenadeLauncherAmmo() ? w->m_icon : 0;
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
    CZWeapon *w = CZWeapon::m_listWeapons.GetWeapon(item);

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
            ammo = w->GetDefaultAmmo();
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
        int size = m_item[index]->GetAmmoCapacity();

        r = SetItemAmmo(index, ammo, size, m_item[index]->GetNumMags());
    }
    return r;
}

extern "C" {
/* a launcher's fire mode past 3 found for an item (OVL2 0x5C45C0, this unit's draft) and the body's
 * test the launcher's selection asks (OVL2 0x5857E0, the SEALs' row) */
bool func_005C45C0(CZKit *kit, int index);
bool func_005857E0(CZSealBody *body);
}

/* whether the weapon selector may pick an item: not a launcher's grenades, a launcher only with a
 * mode past 3 and the body's leave, no item past 201 nor any of nine kinds -- the detonator (193)
 * only while the SEAL has remote charges down (OVL2 0x5BDC30; SOCOM 1's, its kinds renumbered and
 * the launcher's test new) */
bool CZKit::isSelectableItem(int index)
{
    index = wrap(index, m_num_items);
    bool r;

    if (index == -1)
        return false;
    CZFTSWeapon *w = m_item[index];
    EQUIP_ITEM id = w->m_ID;

    r = true;

    if (w->IsGrenadeLauncherAmmo())
        return false;
    if (CZWeapon::IsLauncher(id) && (!func_005C45C0(this, index) || !func_005857E0(m_body)))
        return false;
    if (id > 201)
        r = false;
    else {
        switch (id) {
        case 194:
        case 154:
        case 152:
        case 195:
        case 254:
        case 151:
        case 255:
        case 141:
            r = false;
            break;
        case 193:
            if (!CZWeapon::m_pProjectilelist->GetNumRemoteExplosives(((CEntity *)m_body)->m_id))
                return false;
            break;
        }
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

/* the item in hand an M203 in its launcher's fire mode (OVL2 0x5BDA00, a tail call: SOCOM 1's
 * CurWeaponIsM203Mode, which tested the item itself; Reload and three unwritten SEAL functions call
 * it) */
bool CZKit::CurWeaponIsM203Mode()
{
    return WeaponIsM203Mode(m_selected_item);
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

/* the item in hand in its launcher's fire mode (OVL2 0x5BD7E0, a tail call: every caller -- this
 * unit's CurMagHasAmmo, GetCurNumMags, 0x5C0AE0, 0x5C2A90, 0x5C3000, and the SEAL's IncrZoomState --
 * calls it out of line, so it is pinned; the name ours, after SOCOM 1's CurWeaponIsM203Mode) */
#pragma auto_inline off
bool CZKit::CurWeaponIsLauncherMode()
{
    return WeaponIsLauncherMode(m_selected_item);
}
#pragma auto_inline reset

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

/* the item in hand a grenade launcher in its launcher's fire mode (OVL2 0x5BD6D0, a tail call
 * between CurMagHasAmmo and WeaponIsGrenadeLauncherMode; the SEAL's Reload and 0x5BE9A0 call it --
 * the name ours) */
bool CZKit::CurWeaponIsGrenadeLauncherMode()
{
    return WeaponIsGrenadeLauncherMode(m_selected_item);
}

/* the body as the magazine test reads it (zseal.h's CZSealBody::m_infiniteammo, bit 5 of the word at
 * 0x105C, read as a byte; a view -- kit.h's stand-ins keep zseal.h out of this unit) */
struct CKitAmmoBody {
    unsigned char _0[0x105c];
    unsigned int _bits : 5;
    unsigned int m_infiniteammo : 1;   // bit 5
};

/* rounds in the current magazine: always with infinite ammunition; under a launcher's mode, in the
 * current magazine of the item its ammunition is (the first of that kind with rounds, else the last);
 * otherwise in the selected item's (OVL2 0x5BD530; SOCOM 1's, in its place before
 * WeaponIsGrenadeLauncherMode -- SOCOM II still asks the current item's kind and no longer reads it,
 * where SOCOM 1's answered true for two kinds).
 * Round 43: the launcher arm's `&&` is an int result preset to the index test and set under it --
 * the bool value branched on (nor/sltu/beqz) and the inlined find's counter in v0, as the target --
 * returned through an unsigned char (the return type is not mangled; kit.h's declaration), so the
 * join narrows with `andi 0xff`; a bool result swaps the find's registers (38 off), an int or
 * unsigned char one returned as bool converts with sltu (1 off), `i == -1 ? 0 : ...` gives 7, inline
 * compositions `i != -1 ? MagLoaded() : false` 10-15 (round 42's CTMacro::Tick shape) */
unsigned char CZKit::CurMagHasAmmo()
{
    if (((CKitAmmoBody *)m_body)->m_infiniteammo ? true : false)
        return true;
    if (CurWeaponIsLauncherMode()) {
        int i = FindItemWithAmmo((EQUIP_ITEM)m_firemode[m_selected_item]);
        int r = i != -1;

        if (r)
            r = m_reloads[i][m_currentmag[i]] > 0;
        return r;
    }
    GetCurItemEnum();
    return m_reloads[m_selected_item][m_currentmag[m_selected_item]] > 0;
}

/* the screen's size (zVid's, OVL1 bss 0x4A44C4 / 0x4A44C8; no rows) and tanf (MAIN's) */
extern "C" unsigned int data_004A44C4;
extern "C" unsigned int data_004A44C8;
extern "C" float tanf(float);

/* the body's zoom class held in a local through an enum (enums are not propagated: the class's lb then
 * numbered below the -1.0's lui, the target's a0 and v1; as a char the two swap) */
enum KIT_ZOOM_CLASS { KIT_ZOOM_NONE = -1, KIT_ZOOM_LAST = 127 };

/* the reticle's offsets and size made the body's accuracy factors: a scope's or binoculars' zoom class
 * holds the size at zero and takes the sniper's drift into the offsets, the point the player's aim
 * reaches scales them, the half-angle per pixel turns them to an angle (OVL2 0x5BD100; SOCOM 1's
 * SetCurAccuracy, after FireGrenade and its GetTotalEnc, its constants -- 640, 448, 320, 224 -- the
 * screen's size here, its burst term gone) */
void CZKit::SetCurAccuracy()
{
    zdb::CCamera *cam = zdb::CWorld::m_world->m_camera;
    const float scrz = cam->m_scrZ;
    float fovx = data_004A44C4 / scrz;
    float fovy = fovx * data_004A44C8 / data_004A44C4;
    float x = m_retoffsetx;
    float y = -1.0f * m_retoffsety;
    float size = m_retsize;
    KIT_ZOOM_CLASS zc = (KIT_ZOOM_CLASS)((CKitBody *)m_body)->m_200;

    if (zc > 4 || zc == 4) {
        size = 0.0f;
        m_screenoffset_x += m_sniper_posx;
        m_retoffsetx = -m_sniper_posx;
        m_screenoffset_y -= m_sniper_posy;
        m_retoffsety = -m_sniper_posy;
        x = m_retoffsetx;
        y = m_retoffsety;
    }
    float scl = cam->m_screenAspect[1];
    CZSealBody *p = ftsGetPlayer__Fv();
    CPnt3D a;
    CPnt3D b;

    CopyPoint(&a, &((CKitPlayerView *)p)->m_1458);
    ((CKitPlayerView *)p)->m_14b0.Add(&((CEntity *)p)->m_posW, &b);
    if (((CKitBody *)m_body)->m_eb8) {
        ((CEntity *)((CKitBody *)m_body)->m_eb8)->Virtual60(&b);
        CopyPoint(&a, &b);
    }
    float z = cam->GetZ(&a);
    float d = z - cam->GetZ(&b);

    if (d == 0.0f)
        scl = 1.0f;
    else
        scl = z / (scl * d);
    float h = data_004A44C8;
    CKitBody::CKitAccuracy &acc = ((CKitBody *)m_body)->m_accuracy;
    float hw = 0.5f * data_004A44C4;

    if (hw == 0.0f)
        acc.m_factorX = 0.0f;
    else
        acc.m_factorX = x * scl * (tanf(0.5f * fovx) / hw);
    float hh = 0.5f * h;

    if (hh == 0.0f)
        acc.m_factorY = 0.0f;
    else
        acc.m_factorY = y * scl * (tanf(0.5f * fovy) / hh);
    CKitBody::CKitAccuracy &acc2 = ((CKitBody *)m_body)->m_accuracy;

    hh = 0.5f * data_004A44C8;
    if (hh == 0.0f)
        acc2.m_factor = 0.0f;
    else
        acc2.m_factor = 0.707f * (size * (tanf(0.5f * fovy) / hh));
}

extern "C" {
/* the kit's fire (SOCOM 1's CZKit::Fire(int, const CPnt3D *, const CPnt3D *, const CPnt3D *); OVL2
 * 0x5C5340, unwritten) and an explosive placed (SOCOM 1's HandleRemotePlaceExplosive, which took the
 * point and the SEAL's rotation: SOCOM II's takes two more, here 0; OVL2 0x5BC730, unwritten) */
CZProjectile *func_005C5340(CZKit *kit, int index, const CPnt3D *pos, const CPnt3D *vel, const CPnt3D *v);
CZProjectile *func_005BC730(CZKit *kit, const CPnt3D *pos, const void *rot, zdb::CNode *a, zdb::CNode *b);
}

/* a grenade thrown from a point at a velocity -- the current item fired with no spin, or, an
 * explosive (151; SOCOM 1's 150), placed with the SEAL's rotation (OVL2 0x5BD030; SOCOM 1's) */
CZProjectile *CZKit::FireGrenade(const CPnt3D *pos, const CPnt3D *vel)
{
    CPnt3D v = CPnt3D::zero;
    CZFTSWeapon *w = m_item[m_selected_item];

    if (!w)
        return 0;
    if (w->GetIDType() == IDTYPE_EXPLOSIVE)
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
        int size = w->GetAmmoCapacity();
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
        m_associatedgear[index] = FindGear__18CCharacterGearListFPCc(&m_gear_list__14CCharacterType, GetAssociatedGearName__8CZWeaponFv(w));
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
        switch (CZWeapon::GetIDType(GetCurItem()->m_ID)) {
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

/* a pickup's answer, one byte (fts_remote.cpp's AckUnion, seal_env.cpp's spelling: a bit per entry of the stash), its
 * message and the request's (OVL1 0x2BA1F0, 0x2BA2C0, fts_remote.cpp's) */
union AckUnion {
    struct {
        unsigned char m_0 : 1;
        unsigned char m_1 : 1;
        unsigned char m_2 : 1;
        unsigned char m_3 : 1;
        unsigned char m_4 : 1;
        unsigned char m_5 : 1;
        unsigned char m_6 : 1;
        unsigned char m_7 : 1;
    } ammo;
    unsigned char b;
};

extern "C" {
void SendPickupAckMessage__Fiii8AckUnion(int who, int id, int obj, AckUnion ack);
void func_002BA2C0(int count, unsigned int netid, short *ids, unsigned int id, unsigned int owner);
int strcasecmp(const char *a, const char *b);
}

/* the pickup as the kit reads it (zseal.h's CPickup: its stash and its owner's id; zseal.h is not included here) */
class CPickup {
public:
    unsigned int GetPickupNetEntityID();

    unsigned char _0[0x8];
    CZItemStash *m_data;   // 0x8
    unsigned char _c[0x10 - 0xc];
    unsigned int m_owner;   // 0x10
};

/* the body's entity id (CEntity's, +4) */
struct CKitIdBody {
    unsigned char _0[4];
    unsigned int m_id;   // 0x4
};

/* a pickup's stash taken into the kit -- entry by entry, each entry's rounds put back and, the entry emptied, the ack
 * bit set; in an online game a pickup another console owns is asked for instead (OVL2 0x5BB990; SOCOM 1's PickupAmmo,
 * the stash's now) */
void CZKit::PickupAmmo(CPickup *pickup)
{
    if (PickupPending())
        return;
    CZItemStash *stash = pickup->m_data;
    int count = stash->Count();
    int w = 0;
    int a = 0;
    unsigned short asked = 0;
    int n = 0;

    if (data_0045A0C1 && PickupPending())
        return;
    AckUnion ack;
    short ids[8];
    unsigned int owner = pickup->m_owner;
    bool send = false;

    ack.ammo.m_0 = false;
    ack.ammo.m_1 = false;
    ack.ammo.m_2 = false;
    ack.ammo.m_3 = false;
    ack.ammo.m_4 = false;
    ack.ammo.m_5 = false;
    ack.ammo.m_6 = false;
    ack.ammo.m_7 = false;
    for (int i = 0; i < count; i++) {
        stash->Get(i, &w, &a, &n);
        if (data_0045A0C1 && owner != ((CKitIdBody *)m_body)->m_id) {
            if (CanPickup((CZFTSWeapon *)w, (void *)a, n)) {
                /* (a call in the arm keeps the if -- RegenerateExpressions would fold two plain assignments into a
                 * conditional, no labels, no pads) */
                unsigned short wid;

                if (w)
                    wid = ((CZWeapon *)w)->GetId();
                else
                    wid = 255;
                ids[asked++] = (unsigned short)(wid << 8) | ((CZAmmo *)a)->m_ID;
                m_current_requested_ammo = true;
                m_pickupRequestWait = 1.0f;
            }
        } else {
            PickupAmmo((CZFTSWeapon *)w, (CZAmmo *)a, &n);
            stash->Set(i, (CZWeapon *)w, (CZAmmo *)a, n);
            int j;
            int left = n;

            if (left == 0 && data_0045A0C1)
                send = true;
            for (j = 0; j < count; j++) {
                if (j == 0 && left == 0)
                    ack.ammo.m_0 = true;
                if (j == 1 && left == 0)
                    ack.ammo.m_1 = true;
                if (j == 2 && left == 0)
                    ack.ammo.m_2 = true;
                if (j == 3 && left == 0)
                    ack.ammo.m_3 = true;
                if (j == 4 && left == 0)
                    ack.ammo.m_4 = true;
                if (j == 5 && left == 0)
                    ack.ammo.m_5 = true;
                if (j == 6 && left == 0)
                    ack.ammo.m_6 = true;
                if (j == 7 && left == 0)
                    ack.ammo.m_7 = true;
            }
        }
    }
    if (!data_0045A0C1 && stash->Count() == 0)
        Destroy__7CPickupFP7CPickup(pickup);
    if (send)
        SendPickupAckMessage__Fiii8AckUnion(8, pickup->GetPickupNetEntityID(), ((CKitIdBody *)m_body)->m_id, ack);
    if (data_0045A0C1 && asked > 0)
        func_002BA2C0(asked, pickup->GetPickupNetEntityID(), ids, ((CKitIdBody *)m_body)->m_id, pickup->m_owner);
}

/* a weapon's or ammunition's rounds taken into the kit: with a weapon, its launcher's grenades need the launcher, a
 * weapon held tops up (FindItemWithAmmo), else a slot takes it (two for a weapon of two slots), its model hung; with
 * ammunition, every weapon it suits tops up; rounds taken, the reload sound at the SEAL, the player's message, the bomb's
 * gear by the kit (OVL2 0x5BB000; SOCOM 1's, without the launcher's test, the slots and the gear) */
void CZKit::PickupAmmo(CZFTSWeapon *w, CZAmmo *ammo, int *rounds)
{
    int before = *rounds;

    if (w && w->m_ID != 255) {
        bool ok = true;

        if (w->GetIDType() == 185 && GetLauncherIndex() == -1)
            ok = false;
        if (ok) {
            int i = FindItemWithAmmo(w->GetId());

            if (i != -1)
                *rounds = AddAmmo(i, *rounds);
            else {
                int slot = GetPickupSlot(-1);

                if (func_003C5970(w) == 2) {
                    int slot2 = -1;

                    if (slot)
                        slot2 = GetPickupSlot(slot2);
                    if (slot2 == -1)
                        slot = -1;
                }
                if (slot != -1) {
                    SetItemByIndex(slot, w);
                    m_ammo[slot] = ammo;
                    *rounds = AddAmmo(slot, *rounds);
                    InitItemModel(w->GetModelName(), "grenade");
                }
            }
        }
    } else {
        for (int i = 0; i < m_num_items; i++) {
            if (m_item[i] && IsLegalAmmo__8CZWeaponFP6CZAmmo(m_item[i], ammo)) {
                m_ammo[i] = ammo;
                *rounds = AddAmmo(i, *rounds);
            }
        }
    }
    if (*rounds < before) {
        if (m_rldsnd)
            m_rldsnd->PlaySound3D(&((CEntity *)m_body)->m_posW, 1.0f, 0, 0, 255);
        if (KitIsPlayer((CKitBody *)m_body)) {
            if (strcasecmp(((C2DMessage_Q *)theMission.m_messages)->LastMessage(), ammo->GetDescription()))
                ((C2DMessage_Q *)theMission.m_messages)->Add_Message(ammo->GetDescription(), MSGTYPE_PLAYER, false, 0, 0.0f);
        }
        if ((HasItem((EQUIP_ITEM)152) && ItemHasAmmo((EQUIP_ITEM)152))
            || (HasItem((EQUIP_ITEM)154) && ItemHasAmmo((EQUIP_ITEM)154)))
            EnableMPBombGear__10CZSealBodyFb(m_body, true);
        else
            EnableMPBombGear__10CZSealBodyFb(m_body, false);
    }
}

/* whether a weapon or ammunition pickup can be taken: a weapon the kit holds tops it up, a new one needs a slot (two
 * for a weapon of two slots); ammunition must suit a weapon held with room for it (OVL2 0x5BA770, new in SOCOM II:
 * 0x5BB990's test -- the name ours).
 * OPEN, registers only (0x4C8 against 0x4C0, the size off by two delay slots that follow the colouring; the kit's round 61):
 * the walk's registers, the target's cursor a0, i t0, item t1, r a3, count a2 against item a0, i t1, r t0, count a3, cursor a2.
 * The -1 of the first test and of GetPickupSlot(-1) is one temporary -- s3 in the target, shared with `ammo`; coloured after
 * every volatile, so numbered low. Tried without gain: a const -1 holder, an unsigned enum holder, a named id, and GetId()
 * here, which closed PickupAmmo and HasPickupSlot (kept: `(EQUIP_ITEM)w->m_ID`) */
bool CZKit::CanPickup(CZFTSWeapon *w, void *ammo, int count)
{
    if (w && w->m_ID != 255) {
        int i = FindItemWithAmmo((EQUIP_ITEM)w->m_ID);

        if (i != -1)
            return CanAddAmmo(i, count);
        int slot = GetPickupSlot(-1);

        if (func_003C5970(w) == 2) {
            int slot2 = -1;

            if (slot)
                slot2 = GetPickupSlot(slot);
            if (slot2 == -1)
                slot = -1;
        }
        return slot != -1;
    }
    for (int i = 0; i < m_num_items; i++)
        if (m_item[i] && IsLegalAmmo__8CZWeaponFP6CZAmmo(m_item[i], ammo))
            return CanAddAmmo(i, count);
    return false;
}

/* whether a picked-up item has a slot: no launcher held, and a slot of 2-4 free for it (OVL2
 * 0x5BAC30, new in SOCOM II: GetPickupSlot inlined whole; the SEAL's pickup tests call it -- the
 * name ours).
 * `r = l == -1; if (r) ...` gives the target's materialised test, GetPickupSlot(-1) takes the hoisted -1 (the kit's
 * round 60); byte-exact once GetPickupSlot's last test read the item through GetId() (round 61: the target's numbering) */
bool CZKit::HasPickupSlot()
{
    bool r = GetLauncherIndex() == -1;

    if (r)
        r = GetPickupSlot(-1) != -1;
    return r;
}

/* the slot a picked-up item takes, of slots 2-4 but one: the count when the slot lies past it, a slot
 * whose item uses ammunition and has none, or a slot holding item 255; else -1 (OVL2 0x5BAE60, new
 * in SOCOM II: the environment's pickup code calls it; the name ours). UsesAmmo's weapon a named local: one more variable numbered
 * below the strength-reduced cursors lets the simplifier push them in its first sweep (igreplay: any lower neighbour
 * gives all 25 registers, 39 off without; a local in this body instead stops HasPickupSlot's auto-inlining) */
int CZKit::GetPickupSlot(int skip)
{
    for (int i = 2; i < 5; i++) {
        if (i == skip)
            continue;
        if (m_num_items < i)
            return m_num_items;
        if (!ItemHasAmmo(i) && UsesAmmo(i))
            return i;
        if (m_item[i] && m_item[i]->GetId() == 255)
            return i;
    }
    return -1;
}

/* whether an item has room for more rounds: never past the ten slots, always past the count (an
 * item to come), else when one of its magazines (twice as many for a sidearm or rifle class while
 * the kit holds item 194, ten at most) is short of full (OVL2 0x5BA5B0; SOCOM 1's CanAddAmmo, item
 * 188 and the classes one lower there -- the count, unread in both builds). The item read in place each time: MW keeps its address,
 * index first (a pointer local takes `this` first) */
bool CZKit::CanAddAmmo(int index, int count)
{
    if (index >= 10)
        return false;
    if (index >= m_num_items)
        return true;

    int mags = m_item[index]->GetNumMags();
    int cap = m_item[index]->GetAmmoCapacity();

    if (m_item[index] && HasItem((EQUIP_ITEM)194)) {
        EQUIP_IDTYPE t = (EQUIP_IDTYPE)m_item[index]->GetIDType();

        if (t == IDTYPE_PISTOL || t == IDTYPE_SMG || t == IDTYPE_ASSAULT_RIFLE || t == IDTYPE_SHOTGUN ||
            t == IDTYPE_MACHINEGUN || t == IDTYPE_SNIPER_RIFLE)
            mags *= 2;
    }
    if (mags > 10)
        mags = 10;
    for (int i = 0; i < 10 && i < mags; i++)
        if (cap - m_reloads[index][i] > 0)
            return true;
    return false;
}

/* rounds added to an item's magazines, each topped up in turn (as many as CanAddAmmo counts, the
 * rest emptied); the item 193 given for item 153 (0x5C74E0), the weapon info changed; the rounds
 * left over returned (OVL2 0x5BA3D0; SOCOM 1's AddAmmo, whose tail SOCOM II made 0x5C74E0) */
int CZKit::AddAmmo(int index, int rounds)
{
    int mags = m_item[index]->GetNumMags();
    int cap = m_item[index]->GetAmmoCapacity();

    if (m_item[index] && HasItem((EQUIP_ITEM)194)) {
        EQUIP_IDTYPE t = (EQUIP_IDTYPE)m_item[index]->GetIDType();

        if (t == IDTYPE_PISTOL || t == IDTYPE_SMG || t == IDTYPE_ASSAULT_RIFLE || t == IDTYPE_SHOTGUN ||
            t == IDTYPE_MACHINEGUN || t == IDTYPE_SNIPER_RIFLE)
            mags *= 2;
    }
    if (mags > 10)
        mags = 10;
    for (int i = 0; i < 10; i++) {
        if (i >= mags)
            m_reloads[index][i] = 0;
        else {
            int room = cap - m_reloads[index][i];

            if (room < rounds) {
                m_reloads[index][i] += room;
                rounds -= room;
            } else {
                m_reloads[index][i] += rounds;
                rounds = 0;
            }
        }
    }
    func_005C74E0(this);
    m_weaponinfoChanged = true;
    return rounds;
}

extern "C" {
/* CZSealBody::WepWaitOn (OVL2 0x5DFFC0, seal_weapon.cpp's) */
void WepWaitOn__10CZSealBodyFv(CZSealBody *body);
}

/* the kit's ammunition put into a stash, every item's (-1) or one index's: a held item's
 * ammunition, its rounds counted, added with the item -- none for a rifle or sidearm, which stay
 * -- when a magazine holds rounds and the item is not of class 185; the ammunition let go, the item
 * too unless kept or a launcher, the magazines emptied; then the items closed up, the count made
 * the last item's place plus one, the selection reset when it fell off (OVL2 0x5BA040; SOCOM 1's
 * FillAmmoPickupInstance, the class tests, the launcher's keeping and the tail new) */
void CZKit::FillAmmoPickupInstance(CZItemStash *stash, int index)
{
    for (int i = 0; i < m_num_items; i++) {
        if (index == -1 || index == i) {
            CZFTSWeapon *w = m_item[i];

            if (w) {
                /* (the class declared first: it takes the id's register, s3, the id s2) */
                EQUIP_IDTYPE type;
                EQUIP_ITEM id = w->m_ID;

                type = (EQUIP_IDTYPE)w->GetIDType();

                if (CZWeapon::IsPrimary(id) || CZWeapon::IsSecondary(id))
                    w = 0;
                void *ammo = m_ammo[i];
                int total = GetAmmoTotal(i);

                if (ammo && ItemHasAmmo(i) && type != 185)
                    stash->Add(w, (CZAmmo *)ammo, total);
                m_ammo[i] = 0;
                if (w && !w->IsLauncherWeapon())
                    m_item[i] = 0;
                ClearReloads(i);
            }
        }
    }
    CompactItems();
    UpdateNumItems();
    if (m_selected_item > m_num_items || !m_item[m_selected_item]) {
        m_selected_item = 0;
        WepWaitOn__10CZSealBodyFv(m_body);
        m_weaponinfoChanged = true;
    }
}

/* an item's ammunition let go: its magazines emptied, its pointer cleared, the index wrapped
 * twice as the inlines do (OVL2 0x5B9F00; SOCOM 1's RemoveAmmo, which wrapped through unwrap) */
void CZKit::RemoveAmmo(int index)
{
    ClearAmmo(index);
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
    CompactItems();
    UpdateNumItems();
    if (m_selected_item > m_num_items || !m_item[m_selected_item]) {
        m_selected_item = 0;
        WepWaitOn__10CZSealBodyFv(m_body);
        m_weaponinfoChanged = true;
    }
}

/* the items closed up over the empty slots: at each empty slot with an item after it, every later
 * slot's state, item, ammunition, magazines, current magazine, fire mode, gear, in-use byte and
 * pointer (one place on, in the vector at 0x834) moved down one, and the slot looked at again
 * (OVL2 0x5B9A00, new in SOCOM II: RemoveItemAndAmmo calls it; the name ours). The slot test is an
 * inline bool's (kit.h's AnyItemFrom: the target materialises `!SlotEmpty(j)`, sltu/xori), non-const
 * (const, it reloads the count the loop holds). The size test `size > j + 2`: a compare generates its right operand
 * last, so `j + 2` -- a back-end temporary, IRO refuses `register local +- constant` -- is numbered after the size's load
 * and coloured ahead of the moved values (igreplay: 15 off with it a named local, which no spelling could place) */
void CZKit::CompactItems()
{
    int i = 0;

    while (i < m_num_items) {
        if (!m_item[i] && AnyItemFrom(i)) {
            for (int j = i; j < m_num_items; j++) {
                if (j + 1 < 30) {
                    m_itemstate[j] = m_itemstate[j + 1];
                    m_item[j] = m_item[j + 1];
                    m_ammo[j] = m_ammo[j + 1];
                    for (int k = 0; k < 10; k++)
                        m_reloads[j][k] = m_reloads[j + 1][k];
                    m_currentmag[j] = m_currentmag[j + 1];
                    m_firemode[j] = m_firemode[j + 1];
                    m_associatedgear[j] = m_associatedgear[j + 1];
                    m_iteminuse[j] = m_iteminuse[j + 1];
                    if (m_itemptr.size() > j + 2) {
                        m_itemptr[j + 1] = m_itemptr[j + 2];
                        m_itemptr[j + 2] = 0;
                    }
                }
            }
        } else
            i++;
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
    EQUIP_IDTYPE type = (EQUIP_IDTYPE)m_item[index]->GetIDType();

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
/* the body's adrenaline (CZSealBody::Adrenaline, OVL2 0x5781B0: the SEALs'), the controller's low
 * rumble (CSealCtrl::SetLowRumbleTime, OVL2 0x592B40: the SEALs'), the kick's and the drift's
 * switches (OVL2 0x650938, 0x650940) and the baseline's settling rate (OVL2 0x650980) */
float *Adrenaline__10CZSealBodyFv(CZSealBody *body);
void SetLowRumbleTime__9CSealCtrlFff(CKitCtrl *ctrl, float power, float time);
extern unsigned char data_00650938;
extern unsigned char data_00650940;
extern float data_00650980;
}

/* (OVL2 0x5B9280: kit.h's) */
void CZKit::SniperTick(float dT, CTargetMods &mods, float settle)
{
    float *a = Adrenaline__10CZSealBodyFv(m_body);
    float adr = a ? *a : 1.0f;
    float calm = 1.0f - adr;

    if (adr > 0.2f && ((CKitBody *)m_body)->m_200 != 4) {
        m_heartbeat_accum += dT;
        if (m_heartbeat_state == 0) {
            if (m_heartbeat_accum > 0.3f + 0.3f * 0.8f * calm) {
                float t = 20.0f + 100.0f * adr;

                if (t > 0.0f)
                    SetLowRumbleTime__9CSealCtrlFff(((CKitBody *)m_body)->m_control, 0.1f, t);
                m_heartbeat_accum = 0.0f;
                m_heartbeat_state = 1;
            }
        } else if (m_heartbeat_accum > 0.8f + 0.8f * 0.8f * calm) {
            float t = 20.0f + 100.0f * adr;

            if (t > 0.0f)
                SetLowRumbleTime__9CSealCtrlFff(((CKitBody *)m_body)->m_control, 0.16f, t);
            m_heartbeat_accum = 0.0f;
            m_heartbeat_state = 0;
        }
    }
    if (data_00650938) {
        if (m_firerifle_kick_state == 1 && m_firerifle_kick_movedist > 0.0f) {
            float rate = mods.GetFireRifle_KickRate();
            CKitCtrl *ctrl = ((CKitBody *)m_body)->m_control;

            if (ctrl) {
                /* (the controller's value read ahead of the accumulator, in both states) */
                float step = rate * dT;
                float cur = ctrl->m_130;

                m_firerifle_kick_accum += step;
                if (m_firerifle_kick_accum > m_firerifle_kick_movedist)
                    m_firerifle_kick_state = 2;
                else
                    ctrl->m_130 = cur + step;
            }
        } else if (m_firerifle_kick_state == 2) {
            float rate = mods.GetFireRifle_KickReturnRate();
            CKitCtrl *ctrl = ((CKitBody *)m_body)->m_control;

            if (ctrl) {
                float step = rate * dT;
                float v = ctrl->m_130 - step;

                m_firerifle_kick_accum -= step;

                if (settle > 0.0f) {
                    m_firerifle_kick_baseline += settle * data_00650980;
                    if (m_firerifle_kick_baseline > v)
                        m_firerifle_kick_baseline = v;
                }
                if (v <= m_firerifle_kick_baseline) {
                    ctrl->m_130 = m_firerifle_kick_baseline;
                    m_firerifle_kick_state = 0;
                    m_firerifle_kick_movedist = 0.0f;
                    m_firerifle_kick_accum = 0.0f;
                } else
                    ctrl->m_130 = v;
            }
        }
    }
    if (data_00650940) {
        float lim = mods.GetSniper_DistLimit_PosX();
        float limit = 0.2f * lim + 0.8f * lim * adr;

        if (adr > 0.2f && limit != 0.0f) {
            float r = m_sniper_posx / limit;

            if (r < 0.0f)
                r *= -1.0f;
            /* (the pace's first read a local: the second read, its 0.25 and the ratio's product
             * fuse after the call -- adda/madd) */
            float pace = 0.75f + mods.GetSniper_DistPPFrame_PosX();

            m_sniper_posx += dT * (0.25f + mods.GetSniper_DistPPFrame_PosX() + pace * r);
            if (m_sniper_posx > limit) {
                mods.SetSniper_DistPPFrame_PosX(-mods.GetSniper_DistPPFrame_PosX());
                m_sniper_posx = limit;
            } else if (m_sniper_posx < -limit) {
                mods.SetSniper_DistPPFrame_PosX(-mods.GetSniper_DistPPFrame_PosX());
                m_sniper_posx = -limit;
            }
            lim = mods.GetSniper_DistLimit_PosY();
            limit = 0.2f * lim + 0.8f * lim * adr;
            if (adr < 0.1f)
                limit = 0.0f;
            r = m_sniper_posy / limit;
            if (r < 0.0f)
                r *= -1.0f;
            pace = 0.75f + mods.GetSniper_DistPPFrame_PosY();
            m_sniper_posy += dT * (0.25f + mods.GetSniper_DistPPFrame_PosY() + pace * r);
            if (m_sniper_posy > limit) {
                mods.SetSniper_DistPPFrame_PosY(-mods.GetSniper_DistPPFrame_PosY());
                m_sniper_posy = limit;
            } else if (m_sniper_posy < -limit) {
                mods.SetSniper_DistPPFrame_PosY(-mods.GetSniper_DistPPFrame_PosY());
                m_sniper_posy = -limit;
            }
        }
    }
}

extern "C" {
/* the target mods' kick distances (SOCOM 1's CTargetMods::GetFireRifle_KickRandomDist and
 * GetFireRifle_KickBaseDist, const: OVL1 0x3C5F60 and 0x3C5F70 read 0x48 and 0x44, unclaimed) */
int rand(void);
}

/* the sniper's kick begun: the controller's float at 0x130 the baseline, the distance the stance's
 * base plus a random part of its random distance (OVL2 0x5B91C0, new in SOCOM II: SOCOM 1's
 * CZKit::Fire did this inline, after testing the distance for zero; the name ours) */
void CZKit::StartSniperKick()
{
    CZFTSWeapon *w = GetCurItem();
    CTargetMods *mods = w->TargetMods(GetStance__10CZSealBodyCFb(m_body, true));
    CKitCtrl *ctrl = ((CKitBody *)m_body)->m_control;

    if (ctrl)
        m_firerifle_kick_baseline = ctrl->m_130;
    m_firerifle_kick_movedist = mods->GetFireRifle_KickBaseDist() + mods->GetFireRifle_KickRandomDist() * (4.656613e-10f * rand());
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
    AdrenalineIncr__10CZSealBodyFf(m_body, 0.5f);
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

/* the reticle's offsets cleared (OVL2 0x5B9020, new in SOCOM II: the SEAL's DecrZoomState calls it
 * as it steps out of a scope, where SOCOM 1's called InitializeSniperKick; the name ours) */
void CZKit::ResetRetOffsetXY()
{
    m_retoffsetx = 0.0f;
    m_retoffsety = 0.0f;
}

/* a thrown grenade's flight (zseal.h's CDynGrenade, which this unit cannot include -- it declares a CZKit
 * of its own; the same class by name, so the inline's statics are seal_throw.h's: OVL2 data 0x65E630,
 * 0x65E638, 0x6505C8) */
class CDynGrenade {
public:
    static float ComputeTimeToImpact(float scale, float b, float c);
    static float ComputeElevOfs(float t);
    static float ComputeMaxVel(float dist, float height, float *vel);
    void Set(const CPnt3D &pos, const CPnt3D &dir, float speed)
    {
        m_pos.x = pos.x;
        m_pos.y = pos.y;
        m_pos.z = pos.z;
        m_vel.x = dir.x;
        m_vel.y = dir.y;
        m_vel.z = dir.z;
        m_vel.Scale(speed);
    }
    void Initalize(const CMatrix *m, const CPnt3D &aim, float dist, float power, CPnt3D &p);

    CPnt3D m_pos;   // 0x0
    CPnt3D m_vel;   // 0xc
    float m_18;   // 0x18
};

/* zseal.h's: the inlines the throw solver calls out of line, kept out of line (their copies 0x5975B0 ...) */
inline float Lerp(float a, float b, float t)
{
    return a * (1.0f - t) + b * t;
}

inline void SinCos(float a, float &s, float &c)
{
    s = sinf(a);
    c = cosf(a);
}

inline float LengthSqXZ(const CPnt3D &v)
{
    return v.x * v.x + v.z * v.z;
}

inline void RotateVectors(const CMatrix *m, CPnt3D *p, int count)
{
    TransformVectors(m, p, p, count);
}

extern "C" float asinf(float x);
namespace ai {
void elevate(CPnt3D &p, float y, float scale);
}

/* seal_throw.h's body, word for word (see there) */
inline void CDynGrenade::Initalize(const CMatrix *m, const CPnt3D &aim, float dist, float power, CPnt3D &p)
{
    static const float elev_min = 0.0f;
    static const float elev_max = 0.9599311f;
    static bool from_range = true;
    float height = p.y;
    float unused = 0.0f;
    float maxvel = ComputeMaxVel(dist, height, &unused);
    float elev = asinf(aim.y);

    _saturate(&elev, elev_min, elev_max);
    elev += ComputeElevOfs(power);

    float s, c;

    SinCos(elev, s, c);

    float speed = Lerp(0.05f * maxvel, maxvel, power);
    float t = m_18 = ComputeTimeToImpact(speed * c, speed * s, height);

    if (from_range) {
        speed = sqrtf(t * t + LengthSqXZ(p) + 2.0f * t * p.z);
        _saturate(&speed, 0.0f, maxvel);
        m_vel.x = -p.x;
        m_vel.z = -(t + p.z);
        m_vel.y = 0.0f;
        m_vel.Normalize();
        ai::elevate(m_vel, s, c);
    } else {
        m_vel.x = 0.0f;
        m_vel.z = -c;
        m_vel.y = s;
    }

    CPnt3D q;

    RotateVectors(m, &m_vel, 1);
    m->Transform(&p, &q, 1);
    Set(q, (const CPnt3D &)m_vel, speed);
}

/* the throw of a grenade the SEAL held: the hand's place, a player's solved at no power, an AI's by its
 * fire parameters (0x55EAA0, zseal.h's) -- each fired through 0x5C5340 */
extern "C" bool func_0055EAA0(void *ai, void *params, bool clear);
CZProjectile *func_005C5340(CZKit *kit, int index, const CPnt3D *pos, const CPnt3D *vel, const CPnt3D *v);

/* what the throw reads of the SEAL (zseal.h's CZSealBody and CSealCtrl) */
struct CKitThrowBody {
    unsigned char _0[0x28];
    const CMatrix *m_node;   // 0x28: its node (the matrix first)
    unsigned char _2c[0x2f8 - 0x2c];
    void *m_lhand;   // 0x2f8
    unsigned char _2fc[0x300 - 0x2fc];
    void *m_rhand;   // 0x300
    unsigned char _304[0x404 - 0x304];
    float m_404;   // 0x404
    unsigned char _408[0x105c - 0x408];
    bool InfiniteAmmo() const { return m_infiniteammo ? true : false; }

    unsigned int m_105c_0 : 5;   // 0x105c
    unsigned int m_infiniteammo : 1;   // zseal.h's, bit 5
    unsigned int _105c_6 : 26;
    unsigned char _1060[0x144c - 0x1060];
    CPnt3D m_aim;   // 0x144c
};

struct CKitThrowDyn {
    float m_abort_threshold;   // 0x0
    float max_distance_stand;   // 0x4
    float max_distance_crouch;   // 0x8
};

/* the AI's fire parameters as this reads them (zseal.h's AI_FIREPARAMS at CSealCtrlAi + 0x370) */
struct CKitFireParams {
    unsigned char _0[0x44];
    CPnt3D m_vel;   // 0x44
    unsigned char _50[0x58 - 0x50];
    float m_58;   // 0x58
    unsigned char _5c[0x6c - 0x5c];
    unsigned int m_flags;   // 0x6c
};

extern "C" int IsCurrentBlend__10CZSealBodyCF8AnimType(CZSealBody *seal, short type);
extern "C" void *GetDynamics__10CZSealBodyFv(CZSealBody *seal);
extern short TossGrenadePeekLeft__9AnimTypes;
extern bool data_00650988;

/* the kit told of the SEAL's death: the rifle's model opaque again, the launcher modes put back and, with
 * a throw under way -- not of the placed explosives --, the round taken and the grenade let go where it
 * is held (OVL2 0x5B8920, new in SOCOM II: CZSealBody::OnDeath calls it; the name ours) */
void CZKit::OnDeath()
{
    CKitBody::Attach::Record *rec = ((CKitBody *)m_body)->AttachPart(4);

    if (rec) {
        zdb::CNode *node = (zdb::CNode *)rec->m_node;

        if (node)
            node->m_Opacity = 1.0f;
    }
    for (int i = 0; i < m_num_items; i++)
        if (m_firemode[i] > 3)
            m_firemode[i] = 0;
    if (m_waiting_to_fire) {
        m_waiting_to_fire = false;
        m_firetime = 0.0f;
        unsigned char id = m_item[m_selected_item]->m_ID;

        if (id != 153 && id != 152 && id != 154 && id != 151 && id != 158) {
            m_burst_count++;
            int sel = m_selected_item;

            if (!((CKitThrowBody *)m_body)->InfiniteAmmo()) {
                int n = wrap(sel, m_num_items);

                m_weaponinfoChanged = true;
                if (m_reloads[n][m_currentmag[n]] > 0)
                    m_reloads[n][m_currentmag[n]]--;
            }
            m_81c = 0.0f;
            static CPnt3D s_handOfs(2.0f, 0.0f, 0.0f);
            static CPnt3D s_handOfs2(2.0f, 0.0f, 0.0f);
            void *part = ((CKitThrowBody *)m_body)->m_rhand;

            if (IsCurrentBlend__10CZSealBodyCF8AnimType(m_body, TossGrenadePeekLeft__9AnimTypes))
                part = ((CKitThrowBody *)m_body)->m_lhand;
            CPnt3D p = CPnt3D::zero;

            PartInSpace__5CBodyCFP10CZBodyPartPC6CPnt3DP6CPnt3DP5CQuat(((CKitBody *)m_body)->m_cbody, part, &s_handOfs, &p, 0);
            if (data_00650988) {
                int state = ((CKitBody *)m_body)->m_state;
                CKitThrowDyn *dyn = (CKitThrowDyn *)((char *)GetDynamics__10CZSealBodyFv(m_body) + 0x198);
                float dist = state == 0 ? dyn->max_distance_stand : dyn->max_distance_crouch;
                const CPnt3D *zero = &CPnt3D::zero;

                if (KitIsPlayer((CKitBody *)m_body)) {
                    CKitThrowBody *b = (CKitThrowBody *)m_body;
                    CDynGrenade g;

                    g.Initalize(b->m_node, b->m_aim, dist, 0.0f, p);
                    func_005C5340(this, m_selected_item, &g.m_pos, &g.m_vel, zero);
                } else if (KitIsAi((CKitBody *)m_body)) {
                    CSealCtrlAi *ai = ((CKitSealCtrl *)((CKitBody *)m_body)->m_control)->GetSealCtrlAi();
                    CPnt3D q;

                    ((CKitThrowBody *)m_body)->m_node->Transform(&p, &q, 1);
                    CKitFireParams *fp = (CKitFireParams *)((char *)ai + 0x370);

                    func_0055EAA0(ai, fp, true);
                    func_005C5340(this, m_selected_item, &q, &fp->m_vel, zero);
                    fp->m_flags &= 0xFBDFFFFF;
                    fp->m_58 = 0.0f;
                }
                if (m_last_selected >= 0) {
                    SetCurItemByIndex(m_last_selected);
                    m_last_selected = -1;
                }
            }
        }
    }
}

/* the node the explosive is being placed on, its place and how far it was then (0x66B680's neighbours:
 * HandleFireWeapon 0x5BE9A0 sets them, this tick reads them; the names ours) */
extern zdb::CNode *data_0066B6A0;
extern CPnt3D data_0066B6A8;
extern CPnt3D data_0066B6B8;
extern bool data_00650948;

/* metres in the world's units: the argument cast so that the product by 1 stays (a plain `1.0f * scale`
 * folds; the name ours) */
static inline float Metres(float m)
{
    return (float)m * zdb::CWorld::m_scale;
}

/* a point less another, by value (Added's shape; the name ours) */
static inline CPnt3D Subbed(const CPnt3D &a, const CPnt3D *b)
{
    CPnt3D r;

    a.Sub(b, &r);
    return r;
}

/* the explosives' frame: the wait after the pull run down and, when it is out, by the item's kind -- the
 * placed explosive set on the node it was aimed at (when that node still takes it and has not moved) or
 * back in the bag, the mines and the bomb set on the floor under the hand, or a grenade let go, a player's
 * solved at the controller's power, an AI's by its fire parameters; false while one is being placed (OVL2
 * 0x5C1970; SOCOM 1's TickExplosives) */
bool CZKit::TickExplosives(float dT)
{
    if (KitIsRemote((CKitBody *)m_body))
        return true;
    if (!m_item[m_selected_item])
        return false;
    bool place = false;
    bool onfloor = false;
    bool thrown = false;

    if (m_waiting_to_fire && !(m_firetime < 0.0f))
        m_firetime -= dT;
    if (m_waiting_to_fire && m_firetime <= 0.0f) {
        unsigned char id = m_item[m_selected_item]->m_ID;

        if (id == 152 || id == 153 || id == 154 || id == 158)
            onfloor = true;
        else if (id == 151)
            place = true;
        else
            thrown = true;
        m_waiting_to_fire = false;
        m_firetime = 0.0f;
    }
    if (place || onfloor) {
        CPnt3D pos;

        if (place) {
            CKitHit *hit = &g_placeHit;

            if (!hit)
                return false;
            zdb::CNode *node = data_0066B6A0;
            zdb::CNodeEx *ex = 0;

            ex = node ? node->m_nodeEx : ex;
            CPnt3D d = CPnt3D::zero;

            if (node) {
                CopyPoint(&d, (const CPnt3D *)node->m_matrix.m_matrix[3]);
                CMatrix m;

                if (node->m_parent) {
                    node->m_parent->BuildMTW(m);
                    CopyPoint(&d, &(d + *(const CPnt3D *)m.m_matrix[3]));
                }
                CopyPoint(&d, &(d - data_0066B6B8));
            }
            float len = d.x * d.x + d.y * d.y + d.z * d.z;

            if (!(ex && ex->Slot10(m_explosiveDamage) && len <= 0.0f)) {
                if (m_last_selected == m_selected_item || m_last_selected < 0)
                    SetCurItemByIndex(0);
                else
                    SetCurItemByIndex(m_last_selected);
                m_last_selected = -1;
                data_0066B6A0 = 0;
                g_placeHit.m_poly = 0;
                g_placeHit.m_node = 0;
                data_0066B6B8 = CPnt3D::zero;
                g_placeHit.m_pos = CPnt3D::zero;
                g_placeHit.m_norm = CPnt3D::zero;
                return false;
            }
            CMaterialEx *mat = CMaterialExArray::GetMaterialEx(hit->GetMaterial());

            if (hit->GetMaterial() == 1)
                return false;
            if (!strcmp(mat->m_name, "INVISIBLE_DI"))
                return false;
            if ((mat->m_volumetric ? true : false) || (mat->m_liquid ? true : false))
                return false;
            CQuat q;

            pos.x = data_0066B6A8.x;
            pos.y = data_0066B6A8.y;
            pos.z = data_0066B6A8.z;

            q.Align(CPnt3D::unit_z, hit->m_norm);
            func_005BC730(this, &pos, &q, node, node);
            if (m_last_selected == m_selected_item || m_last_selected < 0)
                SetCurItemByIndex(0);
            else
                SetCurItemByIndex(m_last_selected);
            m_last_selected = -1;
            return false;
        }
        if (onfloor) {
            zdb::IntersectStruct *hit = GetHandFloorHit(false);

            if (hit) {
            float one = 1.0f;
            float k;

            k = (float)one;
            k *= zdb::CWorld::m_scale;
            if (!(hit->m_pos.y <= ((CKitThrowBody *)m_body)->m_404 - k)) {

                pos.x = hit->m_pos.x;
                pos.y = hit->m_pos.y;
                pos.z = hit->m_pos.z;
                func_005BC730(this, &pos, (CQuat *)((char *)m_body + 0x50), hit->m_node, 0);
                if (m_last_selected >= 0) {
                    SetCurItemByIndex(m_last_selected);
                    m_last_selected = -1;
                }
                return false;
            }
            }
            return false;
        }
    } else if (thrown) {
        float power = ((CKitBody *)m_body)->m_control->m_11c;

        m_burst_count++;
        int sel = m_selected_item;

        if (!((CKitThrowBody *)m_body)->InfiniteAmmo()) {
            int n = wrap(sel, m_num_items);

            m_weaponinfoChanged = true;
            if (m_reloads[n][m_currentmag[n]] > 0)
                m_reloads[n][m_currentmag[n]]--;
        }
        m_81c = 0.0f;
        static CPnt3D s_handOfs(2.0f, 0.0f, 0.0f);
        static CPnt3D s_handOfs2(2.0f, 0.0f, 0.0f);
        void *part = ((CKitThrowBody *)m_body)->m_rhand;

        if (IsCurrentBlend__10CZSealBodyCF8AnimType(m_body, TossGrenadePeekLeft__9AnimTypes))
            part = ((CKitThrowBody *)m_body)->m_lhand;
        CPnt3D p = CPnt3D::zero;

        PartInSpace__5CBodyCFP10CZBodyPartPC6CPnt3DP6CPnt3DP5CQuat(((CKitBody *)m_body)->m_cbody, part, &s_handOfs, &p, 0);
        if (data_00650948) {
            int state = ((CKitBody *)m_body)->m_state;
            CKitThrowDyn *dyn = (CKitThrowDyn *)((char *)GetDynamics__10CZSealBodyFv(m_body) + 0x198);
            float dist = state == 0 ? dyn->max_distance_stand : dyn->max_distance_crouch;
            const CPnt3D *zero = &CPnt3D::zero;

            if (KitIsPlayer((CKitBody *)m_body)) {
                CKitThrowBody *b = (CKitThrowBody *)m_body;
                CDynGrenade g;

                g.Initalize(b->m_node, b->m_aim, dist, power, p);
                func_005C5340(this, m_selected_item, &g.m_pos, &g.m_vel, zero);
            } else if (KitIsAi((CKitBody *)m_body)) {
                CSealCtrlAi *ai = ((CKitSealCtrl *)((CKitBody *)m_body)->m_control)->GetSealCtrlAi();
                CPnt3D q;

                ((CKitThrowBody *)m_body)->m_node->Transform(&p, &q, 1);
                CKitFireParams *fp = (CKitFireParams *)((char *)ai + 0x370);

                func_0055EAA0(ai, fp, true);
                func_005C5340(this, m_selected_item, &q, &fp->m_vel, zero);
                fp->m_flags &= 0xFBDFFFFF;
                fp->m_58 = 0.0f;
            }
            if (m_last_selected >= 0) {
                SetCurItemByIndex(m_last_selected);
                m_last_selected = -1;
            }
        }
    }
    return true;
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

/* (a class's tests -- OVL1 0x3D1DB0: the rifles, 31, 51, 81, 91, 141, 101; 0x3D1DA0: the pistol, 4;
 * 0x3D1D90: 145 -- are zweapon.h's IsPrimaryType, IsSecondaryType and IsLauncherType, the byte enum
 * masked in each) */

/* the current item's fire class: 1 for the rifles, 2 the pistol, 4 class 145, 3 the grenades and
 * explosives (121, 151), else none (OVL2 0x5B8760, new in SOCOM II among the kit's additions at
 * the unit's start: CHUD::Tick and the SEAL's fire point lookups call it) */
extern "C" {
/* the body's change to an item (OVL2 0x5A64C0,
 * the SEALs' range: the item in hand, the new item's fire class and the kit's bit 13) */
void func_005A64C0(CZSealBody *body, int item, CZKit::FIRE_CLASS c, bool b);
}

/* the item in hand as the kit's frame reads it (CZSealBody's m_item, zseal.h's; the view ours) */
struct CKitBodyItem {
    unsigned char _0[0xf79];
    signed char m_item;   // 0xf79
};

/* the frame's kit work: the first frame's set-up, or else an item asked for handed to the body with
 * its fire class, and the reticle brought up to date; the request bits cleared either way (OVL2
 * 0x5C1660: SOCOM 1's PreTick by WeaponsTick's call, SOCOM II's own body) */
void CZKit::PreTick(float dT)
{
    if (m_first)
        FirstFrame(dT);
    else if (m_requested_item) {
        int item = ((CKitBodyItem *)m_body)->m_item;
        FIRE_CLASS c = FireClassOf(GetFiringItem(m_selected_item));

        func_005A64C0(m_body, item, c, m_bit13 != 0);
        SetBit13(this, false);
        RETICULE_TYPE r = GetReticuleByIndex(m_selected_item);

        if (r != m_curitemreticule) {
            m_curitemreticule = r;
            SetReticuleTypeChanged(this, true);
        }
    }
    SetRequestedItem(this, false);
    SetBit13(this, false);
}

CZKit::FIRE_CLASS CZKit::GetCurItemFireClass()
{
    FIRE_CLASS c = FIRE_CLASS_NONE;
    CZFTSWeapon *w = m_item[m_selected_item];

    if (w) {
        /* zweapon.h's GetIDType answers an EQUIP_ITEM: kept as one (converted to EQUIP_IDTYPE, MW
         * narrows the copy it keeps with an andi) */
        EQUIP_ITEM t = CZWeapon::GetIDType(w->m_ID);

        if (CZWeapon::IsPrimaryType(t))
            c = FIRE_CLASS_RIFLE;
        else if (CZWeapon::IsSecondaryType(t))
            c = FIRE_CLASS_PISTOL;
        else if (CZWeapon::IsLauncherType(t))
            c = FIRE_CLASS_145;
        else if (t == IDTYPE_GRENADE || t == IDTYPE_EXPLOSIVE)
            c = FIRE_CLASS_THROWN;
    }
    return c;
}

/* the first item of a fire class, or -1 (OVL2 0x5B8630, new in SOCOM II beside GetCurItemFireClass:
 * CZSealBody::Tick and the environment's pickup code call it; the name ours) */
int CZKit::GetFireClassIndex(FIRE_CLASS c)
{
    for (int i = 0; i < m_num_items; i++)
        if (m_item[i] && FireClassOf(m_item[i]) == c)
            return i;
    return -1;
}

/* for the two items that show their round (186 and 146), the fire point's "projectile" child made
 * active (OVL2 0x5B85A0, new in SOCOM II among the kit's additions; 0x5A78C0 jumps to it). The fire
 * point through CEntity's slot +0x64 (zentity.h's GetFirepointNode; CZSealBody's override is
 * 0x5DFAD0, SOCOM 1's GetFirepointNode -- the body is CZSealBody, its CEntity at 0). The fire
 * point and the name are locals, the name defined after the fire point's call (round 39: then the
 * target's schedule -- the name materialised whole before the fire point moves into a0; passed as
 * a literal the move splits lui/addiu, a name defined before the call is kept in a saved register,
 * 0x98) */
void CZKit::ShowProjectile()
{
    EQUIP_ITEM id = m_item[m_selected_item]->m_ID;

    if (id == 186 || id == 146) {
        zdb::CNode *fp = ((CEntity *)m_body)->GetFirepointNode(0);
        const char *nm = "projectile";
        zdb::CNode *n = fp->FindChild(nm, true);

        if (n)
            n->SetActive(true);
    }
}

/* the sniper rifle's sight shown to suit the thermal scope: with item 195 in the kit, the first
 * primary weapon -- when a sniper rifle (101) -- has its "scope" node hidden and its
 * "thermal_scope" shown, without it the other way round; both looked up under the node the body's
 * second attachment keeps (OVL2 0x5B82E0, new in SOCOM II after ShowProjectile: 0x5C0250 calls it;
 * the names ours). HasItem and GetPrimaryIndex inline in each arm */
static inline void ShowScopes(CZKit *kit, bool thermal)
{
    zdb::CNode *node = (zdb::CNode *)((CKitBody *)kit->m_body)->m_attach[1].m_rec->m_node;
    zdb::CNode *scope = node->FindChild("scope", true);
    zdb::CNode *tt = node->FindChild("thermal_scope", true);

    if (scope)
        scope->SetActive(!thermal);
    if (tt)
        tt->SetActive(thermal);
}

void CZKit::UpdateScopeModel()
{
    if (HasItem((EQUIP_ITEM)195)) {
        int i = GetPrimaryIndex();

        if (i != -1 && m_item[i]->GetIDType() == IDTYPE_SNIPER_RIFLE)
            ShowScopes(this, true);
    } else {
        int i = GetPrimaryIndex();

        if (i != -1 && m_item[i]->GetIDType() == IDTYPE_SNIPER_RIFLE)
            ShowScopes(this, false);
    }
}
