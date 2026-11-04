/* zseal.h -- the SEALs: the body (CZSealBody), its controllers (CSealCtrl and CSealCtrlAi) and
 * the pieces around them. SOCOM 1's DWARF files them under gamez/zseal/zseal.h; their code is
 * OVL2 0x542E50 on, in SOCOM 1's units and order (the initialisers name them: seal.cpp,
 * seal_ai.cpp, seal_anim.cpp, seal_body.cpp, seal_cfg.cpp, seal_command.cpp, seal_control.cpp,
 * seal_create.cpp, seal_dyn.cpp, seal_env.cpp, seal_ex.cpp, seal_ik.cpp, seal_kit.cpp, ...).
 *
 * The layouts are SOCOM 1's DWARF moved to where SOCOM II's code puts each member; only the
 * members a written function touches are named, every offset proven by that code (the others
 * are padding, sized from the stores of the constructors). */
#ifndef ZSEAL_H
#define ZSEAL_H

#include "../zentity/zentity.h"
#include "../znode/node_world.h"
#include "../zcamera/zcamera.h"
#include "aitimer.h"
#include "../../ai/ai_states.h"
/* (the kit lane, round 14: CSealCtrlAi::SetAiEvent takes CAiEvent::EVENT) */
#include "../../ai/ai.h"
#include "../zchar/zchar.h"

class CZSealBody;
class CZProjectile;
class CTarget;
class CPad;
namespace zdb {
class IntersectStruct;
class DiIntersect;
}

/* the controller's menus (SOCOM 1's DWARF, a byte) */
enum MENU_STATE {
    stateNone = 0,
    stateWeaponSel = 1,
    stateOrders = 2,
    statePauseTestMenu = 3,
    stateFullScreenMapMenu = 4,
    stateFullScreenMapAnim = 5,
    stateLast = 6
};

/* the view the player looks through (SOCOM 1's DWARF, a signed byte: SetCurZoomState's argument)
 * with SOCOM II's numbers -- thirteen states, the turret's thermal view new at 2 and SOCOM 1's
 * others one place on (DecrZoomState, IncrZoomState and SetCurZoomState switch on them; the
 * thermal view's name ours) */
enum ZOOMSTATE {
    ZOOMSTATE_UNKNOWN = -1, ZOOMSTATE_3RDPERSON = 0, ZOOMSTATE_1STPERSON = 1, ZOOMSTATE_THERMAL = 2,
    ZOOMSTATE_NVG = 3, ZOOMSTATE_BINOCS = 4, ZOOMSTATE_WEAPONDEF1 = 5, ZOOMSTATE_WEAPONDEF2 = 6,
    ZOOMSTATE_WEAPONDEF3 = 7, ZOOMSTATE_WEAPONDEF4 = 8, ZOOMSTATE_WEAPONDEF5 = 9, ZOOMSTATE_WEAPONDEF6 = 10,
    ZOOMSTATE_WEAPONDEF7 = 11, ZOOMSTATE_WEAPONDEF8 = 12, NUM_ZOOMSTATES = 13
};

/* what a SEAL holds (SOCOM 1's SEAL_ITEM, a byte enum: zweapon.h keeps a word-sized stand-in of
 * that name, so this one's name is ours) */
enum SEAL_ITEM_BYTE { ITEM_NONE = 0, ITEM_RIFLE = 1, ITEM_PISTOL = 2, ITEM_THROWABLE = 3, ITEM_DONE = 4 };

/* the fire trigger's edge state (SOCOM 1's DWARF, a byte) */
enum TRIGGER { TRIGGER_UP = 0, TRIGGER_DOWN = 1, TRIGGER_FALLING = 2, TRIGGER_RISING = 3, TRIGGER_FINISH = 4 };

/* the body's states (SOCOM 1's DWARF, a byte) */
enum SEAL_STATE {
    stateStand = 0, stateCrouch = 1, stateProne = 2, statePeek = 3, stateHang = 4,
    stateClimbLadder = 5, stateInAir = 6, stateIdle = 7, stateDead = 8, stateCarrying = 9,
    stateCarried = 10, stateDone = 11
};

/* a hand signal (SOCOM 1's SEAL_HANDSIGNAL, a byte -- no DWARF; the names ours, after the anim names
 * the kit lane's seal_handsig.cpp looks up) */
enum SEAL_HANDSIGNAL {
    HANDSIGNAL_NULL = 0, HANDSIGNAL_FOLLOW = 1, HANDSIGNAL_HOLDPOSITION = 2, HANDSIGNAL_FIREATWILL = 3,
    HANDSIGNAL_DEPLOY = 4, HANDSIGNAL_POINT = 5, HANDSIGNAL_COUNT = 6, HANDSIGNAL_LAST = 255
};

/* where a hit lands (SOCOM 1's DAMAGE_LOCATION, a byte: 0 the head -- ReportWeaponHitDamage counts
 * a head shot so; the other names not needed) */
enum DAMAGE_LOCATION { DAMAGE_LOCATION_0 = 0, DAMAGE_LOCATION_LAST = 255 };

/* a kind of collision polygon (SOCOM 1's CDIType; no DWARF, only the size matters here) */
enum CDIType { CDI_TYPE_0 = 0, CDI_TYPE_LAST = 255 };

/* an animation's kind (SOCOM 1's DWARF: AnimType, a 2-byte enum from -1 to 142 -- SOCOM II's code
 * narrows it as a short; only its bounds here) */
enum AnimType { animTypeUndefined = -1, animTypeRef = 0, animTypeLast = 142 };

/* SOCOM 1's anim types read as variables: static shorts the anim set fills in (anim.cpp's 0x5E1DF0
 * stores them; OVL1 data, 8 bytes apart in SOCOM 1's order -- SOCOM 1's 4). Only the ones read here */
class AnimTypes {
public:
    /* (SOCOM 1's KickGrenade, 33 before Stand2Prone in both builds: the grenade state's kick,
     * 0x55AC20, plays it with the timed call that throws the grenade from where it lies) */
    static short KickGrenade;   // OVL1 0x3DEB40
    static short Stand2Prone;   // OVL1 0x3DEC48
    static short PistolStand2Prone;   // OVL1 0x3DEC50
    /* (SOCOM 1's SwapRifle, six after Stand2Prone in both builds: the weapon swap's blends,
     * 0x57D5E0, are looked up again unless it is on the stack) */
    static short SwapRifle;   // OVL1 0x3DEC78
    static short ClimbMedium;   // OVL1 0x3DEC90
    /* (SOCOM II's two strafes, AnimType itself, not short -- the SEALs, round 34: SetMaxMotion passes
     * them to GetScript unconverted, the static loaded ahead of the set as an argument with no
     * conversion is) */
    static AnimType ProneRStrafe;   // OVL1 0x3DECC0 (SOCOM II adds one after ClimbMedium's four)
    static AnimType ProneLStrafe;   // OVL1 0x3DECD0
};

/* how a SEAL died (SOCOM 1's DWARF, a byte) */
/* a landing's grade by its speed (HandleFallingDamage's: assigned with li, switched on with andi and
 * returned as it is -- a byte enum; the names ours) */
enum LANDING_TYPE { LANDING_NONE = 0, LANDING_SOFT = 1, LANDING_HARD = 2, LANDING_HURT = 3 };

enum SEAL_DEATH_TYPE {
    DEATH_NONE = 0, DEATH_KNIFE_KILL = 1, DEATH_CQB = 2, DEATH_HEADSHOT = 3, DEATH_GRENADE = 4,
    DEATH_BULLET = 5, DEATH_GHOST = 6, DEATH_FALLING = 7, DEATH_MAX = 8
};

/* the orders (SOCOM 1's DWARF, a byte: only the size matters here) */
enum FT_COMMAND { CMD_UNKNOWN = 0, CMD_ACTION = 1, CMD_LAST = 255 };

/* a character's sounds are zchar.h's CHARACTER_SOUND (a signed byte, -1 to 84); the kinds of stun
 * (SOCOM 1's DWARF; only the size matters here) */
enum STUN_TYPE { STUN_NONE = 0, STUN_LAST = 6 };

/* the AI's small templates (SOCOM 1's namespace ai; never inlined -- MW auto-inlines no template
 * function -- so each is a call to the copy seal_ai.cpp's tail links) */
namespace ai {
template <class T>
const T &_max(const T &a, const T &b)
{
    return a >= b ? a : b;
}

template <class T>
const T &_min(const T &a, const T &b)
{
    return a <= b ? a : b;
}
}

/* the SEAL's kit: its weapons and gear (SOCOM 1's CZKit, 0x87c bytes there; only its start here) */
class CZKit {
public:
    /* whether the kit holds an item, and whether that item has ammunition (SOCOM 1's; OVL2
     * 0x5C86F0, 0x5BDB20, the kit lane's) */
    bool HasItem(EQUIP_ITEM item) const;
    bool ItemHasAmmo(EQUIP_ITEM item);
    /* an item's index in the kit, and whether the item at an index has ammunition (SOCOM 1's;
     * OVL2 0x5C8870, 0x5BDA10 -- both const in SOCOM II, kit.h's) */
    int GetItemIndex(EQUIP_ITEM item) const;
    bool ItemHasAmmo(int index) const;
    /* whether an item has room for rounds (SOCOM 1's; OVL2 0x5BA5B0, kit.h's -- the AI's ammunition
     * search asks it) */
    bool CanAddAmmo(int index, int rounds);
    /* the item in hand (kit.h's: const in SOCOM II; OVL2 0x5C3780) and the item made current by its
     * index (kit.h's; OVL2 0x5C4B10) */
    class CZFTSWeapon *GetCurItem() const;
    void SetCurItemByIndex(int index);
    /* the weapons parented to the hand for the rifle or the pistol (kit.h's; OVL2 0x5C6A40, 0x5C6DA0:
     * the two takes start with them) */
    void ReParentToRifle();
    void ReParentToPistol();
    /* whether the item in hand has rounds left (kit.h's; OVL2 0x5BDB00) */
    bool CurItemHasAmmo();
    /* the items held, an item set at an index, its magazines and rounds, its ammunition set -- with a
     * count of rounds (kit.h's; OVL2 0x5BE030, 0x5BDFB0, 0x5C4970, 0x5C3C10, 0x5BDE20, 0x5BDEC0) */
    int GetNumItems() const;
    bool SetItem(unsigned int index, EQUIP_ITEM item);
    int GetNumMags(int index);
    int GetAmmoTotal(int index);
    bool SetItemAmmo(unsigned int index, void *ammo);
    bool SetItemAmmo(unsigned int index, void *ammo, int rounds, int count);
    /* the item at an index let go with its ammunition (SOCOM 1's; OVL2 0x5B9BB0, the kit lane's) */
    void RemoveItemAndAmmo(int index);
    /* the first item whose ammunition is of a kind with the rounds, and the ammunition at an index
     * let go (SOCOM 1's; OVL2 0x5C8750, 0x5B9F00, the kit lane's -- CreateNetAmmoPickup asks them) */
    int GetAmmoIndex(EQUIP_AMMO ammo, int rounds);
    void RemoveAmmo(int index);
    /* the items' set-up finished (SOCOM 1's; OVL2 0x5C7840, the kit lane's -- the SEAL's OnMissionStart
     * calls it) */
    void PostSetupItems();
    /* whether a scope or the binoculars are up (SOCOM 1's; OVL2 0x5B9990, 0x5B90F0 -- kit.h's) */
    bool IsUsingScope();
    bool IsUsingBinocs();
    /* whether a weapon picked up has a slot to go in (kit.h's; OVL2 0x5BAC30, the kit lane's: the
     * SEAL's pickup tests 0x5A9C70 and 0x5A9F40 ask it) */
    bool HasPickupSlot();
    /* the sniper's kick made ready (SOCOM 1's; OVL2 0x5B9180, kit.h's: SetCurZoomState calls it) */
    void InitializeSniperKick();
    /* the weapon's display marked changed (SOCOM 1's; OVL2 0x5B99B0, kit.h's) */
    void SetWeaponInfoChanged();
    /* the reticule's offset zeroed (SOCOM 1's m_retoffsetx/y, 0x20/0x24 here), and whether the weapon
     * in hand is in its launcher mode (new in SOCOM II; OVL2 0x5B9020, 0x5BD7E0 -- the kit lane's) */
    void ResetRetOffsetXY();
    bool CurWeaponIsLauncherMode();
    /* the class of the item in hand (kit.h's byte enum; OVL2 0x5B8760, the kit lane's) */
    enum FIRE_CLASS { FIRE_CLASS_NONE = 0, FIRE_CLASS_RIFLE = 1, FIRE_CLASS_PISTOL = 2,
                      FIRE_CLASS_THROWN = 3, FIRE_CLASS_145 = 4, FIRE_CLASS_LAST = 255 };
    FIRE_CLASS GetCurItemFireClass();
    /* an item's fire class, no kit asked (the kit lane's round 32 name: a static member, the item in
     * a0; OVL2 0x5B8820 -- seal_env.cpp compares it with the pistol's 2) */
    static FIRE_CLASS GetItemFireClass(EQUIP_ITEM item);
    /* a weapon's ammunition taken in, the rounds read and handed back (SOCOM 1's PickupAmmo by its
     * arguments, the kit lane's round 32; OVL2 0x5BB000 -- HandlePickupAck's eight takes call it) */
    void PickupAmmo(class CZFTSWeapon *weapon, class CZAmmo *ammo, int *rounds);
    /* the fire mode set (kit.h's; OVL2 0x5C37B0, the kit lane's -- FIREMODE comes through zchar.h's
     * zweapon.h) */
    bool SetCurFireMode(FIREMODE mode);
    /* the frame's kit work before the SEAL's (SOCOM 1's PreTick; OVL2 0x5C1660, seal_kit.cpp's range:
     * WeaponsTick calls it last, as SOCOM 1's does) */
    void PreTick(float dT);
    /* an explosive set down (SOCOM 1's HandlePlacingExlosive, sic; OVL2 0x5C0020, kit.h's) */
    void HandlePlacingExlosive(EQUIP_ITEM item, float a, float b);
    /* a pickup's answer dealt with (SOCOM 1's; OVL2 0x5B99D0, kit.h's) */
    void PendingPickupHandled();

    /* kit.h's flags (the kit lane's names): only bit 12 here */
    unsigned int m_bits0 : 12;   // 0x0
    unsigned int m_requested_item : 1;   // bit 12: an item asked for (IsChangingWeapons tests it)
    unsigned int m_bits13 : 19;
};

class CSealCtrlAi;

/* CAiState, its stack and AI_STATE are ai/ai_states.h's (the AI-states lane's header) */

/* an AI sound's priority, passed by value: the level in the low byte (new in SOCOM II; the names
 * ours) */
struct AI_SOUND_PRIORITY {
    unsigned int m_level : 8;
    /* (its bit 0, the word's 8: the anim dropped once the alertness's band passes the level -- the
     * target tests it on the word, `andi 0x100`: 0x5712F0) */
    unsigned int m_rest : 24;
};

/* an anim's signal to its controller (AI_SIGNAL) is ai_states.h's: CAiState's slot +0x0C takes it */

/* the goals a controller pursues (SOCOM 1's DWARF, a byte) */
enum GOAL_TYPE { GOAL_NONE = 0, GOAL_SCAN = 1, GOAL_LOOK = 2, GOAL_MOVE = 3, GOAL_SHOOT = 4, GOAL_AVOID = 5, GOAL_RECOVER = 6 };

/* a SEAL's stance (SOCOM 1's DWARF, a signed byte) */
/* a SEAL's weapon state (SOCOM 1's WEAPON_STATE, a byte: GetWeaponState narrows with andi -- the
 * enumerators' names unknown, so only the extent here) */
enum WEAPON_STATE { WEAPON_STATE_0 = 0, WEAPON_STATE_LAST = 255 };

enum SEAL_STANCE {
    STANCE_NONE = -1, STANCE_STAND = 0, STANCE_CROUCH = 1, STANCE_PRONE = 2, STANCE_IDLE = 3,
    STANCE_MAX = 4
};

/* a random period and its countdown, CAiTimer, is aitimer.h's (included ahead of ai_states.h) */

/* a value easing toward a goal (SOCOM 1's DWARF: CAiDecay, 0xc bytes; only the two floats the view
 * range sets are named, the names ours) */
class CAiDecay {
public:
    float m_value;   // 0x0
    float m_goal;   // 0x4
    float m_8;   // 0x8
};

/* an action a node offers: the bomb, a breach, ... (SOCOM 1's CNodeAction: its valve at 0x18 in
 * both builds, its kind at 0x24 -- SOCOM 1's 0x20; only those here). The kinds are a byte enum
 * (copied with lb, compared with lbu); the names ours, 8 what CanDeploy takes for no action */
enum NODEACTION_TYPE { NODEACTION_0 = 0, NODEACTION_BOMB = 4, NODEACTION_5 = 5, NODEACTION_BREACH = 6, NODEACTION_NONE = 8 };

class CValve;

class CNodeAction {
public:
    /* the anim run on a SEAL (SOCOM 1's; OVL1 0x2B44E0, fts_action.cpp's) */
    int ExecuteAction(class CZSealBody *body, unsigned int index);
    /* the action a node offers, searched down its tree (SOCOM 1's; OVL2 0x5AA470, seal_env.cpp's) */
    static CNodeAction *GetAction(zdb::CNode *node, bool deep);

    unsigned char _0[0x14];
    /* (SOCOM 1's m_node: CanPlaceBomb matches it against the sides' bomb sites, TickMPBomb against the
     * bomb's node) */
    zdb::CNode *m_node;   // 0x14
    CValve *m_valve;   // 0x18
    unsigned char _1c[0x24 - 0x1c];
    NODEACTION_TYPE m_type;   // 0x24
    unsigned char _25[0x30 - 0x25];
    unsigned int m_teams;   // 0x30: the sides that may use it (fts_action.cpp's m_30)
    unsigned int m_owner;   // 0x34: the entity using it, CEntity::invalidId for none (SOCOM II's)
    unsigned char _38[0x3c - 0x38];
    unsigned short m_id;   // 0x3c (SOCOM 1's at 0x2c)
};

/* the AI's maps (SOCOM 1's CAiMaps; the global AiMaps, OVL2 bss) -- only the cell's centre */
class CAiMaps {
public:
    bool GetCellCenter(const CAiMapLoc &loc, CPnt3D *center);
    /* a named location's cell (ai_maps.h's, OVL2 0x52DB70) */
    bool GetLocation(const char *name, CAiMapLoc &loc);
    /* a point's cell (ai_maps.h's, OVL2 0x52DC70) */
    bool GetLocation(const CPnt3D *pos, CAiMapLoc &loc);
    /* a cell's map (ai_maps.h's, OVL2 0x52E6B0), and a location's (OVL2 0x52A180) */
    class CAiMap *GetMapByIndex(unsigned int i) const;
    class CAiMap *GetMap(const CAiMapLoc &loc) const;
    /* whether a straight line runs from one cell to another, where it stops handed back (ai_maps.h's,
     * OVL2 0x52CC80) */
    bool CanTravelLine(const CAiMapLoc &from, const CAiMapLoc &to, CAiMapLoc *out);
    /* a location's cell (ai_maps.h's, OVL2 0x52D6A0) */
    CAiCell *GetCell(const CAiMapLoc &loc);
    /* an action by its id, its cell and its record (ai_maps.h's, OVL2 0x52DAA0) */
    bool GetActionById(unsigned int id, CAiMapLoc &loc, CAiMap::action **pa);
    /* a path planned from a cell through a list of cells, and a cell's region (ai_maps.h's, OVL2
     * 0x52C5C0, 0x52D950) */
    bool RegisterPathPlan(const CAiMapLoc &start, const CAiMapLoc *locs, int count, CAiPath *path);
    CAiMap::region *GetRegion(const CAiMapLoc &loc);
    /* a region by its name, and its cell (ai_maps.h's, OVL2 0x52D9F0) */
    CAiMap::region *GetRegion(const char *name, CAiMapLoc &loc);
    /* the cells between two points walked over the maps, each handed to a callback (ai_maps.h's,
     * OVL2 0x5352D0) */
    bool WalkLine(const CPnt3D *from, const CPnt3D *to, bool (*f)(const MAP_LINEWALK_STRUCT &s, void *data),
        void *data, float step);
};

extern CAiMaps AiMaps;

/* a squad (SOCOM 1's CSealUnit; SOCOM II's vtable at 0x669650 has IssueCommand first, its
 * pointer at 4) -- only what the controller calls */
/* what a squad is told (SOCOM 1's UNIT_MESSAGE; 1 a member's death -- only the name matters here) */
enum UNIT_MESSAGE { UNIT_MESSAGE_0 = 0, UNIT_MESSAGE_DEATH = 1, UNIT_MESSAGE_LAST = 255 };

class CSealUnit {
public:
    /* a member's news handed to the squad (SOCOM 1's; OVL2 0x5ED7B0, the kit lane's) */
    void Notify(CZSealBody *seal, UNIT_MESSAGE msg, void *data);
    /* the squad of a team, and a member by its index (OVL2 0x5EF2F0, 0x5EDBB0: the kit lane's
     * seal_unit.cpp, which declares them alike) */
    static CSealUnit *GetUnitByTeam(unsigned int team);
    /* the squad's SEALs counted, with its sub-squads' to a depth (OVL2 0x5ED9B0, the kit lane's) */
    int GetSealCount(int depth);
    CZSealBody *GetSealByIndex(int index, int depth);
    /* the squad's leader (SOCOM 1's const GetLeader; OVL2 0x5E8A90, the kit lane's) */
    CZSealBody *GetLeader() const;
    /* a SEAL's look out of a direction (SOCOM 1's; OVL2 0x5EEF80, the kit lane's seal_unit.cpp) */
    void GetMyLookDir(CPnt3D *out, CPnt3D *dir, CZSealBody *seal);
    /* the squad's order data as a pointer, and set to one (OVL2 0x5E6D60, 0x5E6DD0: the kit lane's
     * seal_unit.cpp, which declares them alike) */
    void *GetDataPtr();
    bool SetDataPtr(void *p);
    /* a SEAL's squad's own clean-up of it (OVL2 0x5EDF90, the kit lane's seal_unit.cpp, which
     * declares it alike) */
    static void SealLeaveUnit(CZSealBody *seal);

    unsigned int m_0;   // 0x0
    virtual void IssueCommand(CZSealBody *seal, FT_COMMAND cmd, void *data, bool b);
    virtual bool AllowCommand(CZSealBody *seal, FT_COMMAND cmd, void *data);   /* +0xC (SOCOM 1's) */
    /* the top squad above this one (recursive: its out-of-line copy, OVL2 0x561D20 after 0x561B90
     * its first caller, inlines itself eight times; a caller inlines it once -- the name ours) */
    CSealUnit *GetTop() { return m_parent ? m_parent->GetTop() : this; }

    unsigned char _8[0x60 - 0x8];
    CSealUnit *m_parent;   // 0x60 (SOCOM 1's 0x3c): the squad this one is a part of
    unsigned char _64[0x9c - 0x64];
    /* the squad's current order: its kind a byte at 8 (a kill by the local player under order 19
     * counts twice: 0x545290; the names ours) */
    struct CSealUnitOrder {
        unsigned char _0[8];
        unsigned char m_8;
    } *m_9c;   // 0x9c
};

/* the network (SOCOM II's theNetwork at 0x45A0C0: SOCOM 1's flags at 8 and 9, the states lane's
 * reading) and the net game's state (OVL1 bss 0x437CE8, the HUD lane's reading) */
class CZNetwork {
public:
    /* this console's client index in the game (SOCOM 1's) */
    int zNetGetMyGameClientIndex();
    /* the network's clock (SOCOM 1's; OVL1 0x30D550) */
    int zNetGetTime();

    unsigned char m_8;   // 0x0: the session master
    unsigned char m_9;   // 0x1: an online game
};

extern CZNetwork theNetwork;

class CNetGameState {
public:
    unsigned char _0[0x38];
    /* the valves a hostage's death steps, by the killer's side (HandleKiller; the names ours) */
    CValve *m_38;   // 0x38
    CValve *m_3c;   // 0x3c
    unsigned char _40[0x70 - 0x40];
    CValve *m_70;   // 0x70: a valve each kill steps (0x545290; the name ours)
    unsigned char _74[0xd2 - 0x74];
    unsigned char m_ghost;   // 0xd2
    unsigned char _d3[0xdc - 0xd3];
    unsigned char m_dc;   // 0xdc: a SEAL's death sends no squad order unless set (the name ours)
    unsigned char _dd[0x111 - 0xdd];
    unsigned char m_mode;   // 0x111: the game's kind (the stats test 1 and 3; the name ours)
    unsigned char _112[0x12c - 0x112];
    char m_charname[32];   // 0x12c: the local player's character (the name ours)
};

/* a pickup: a weapon, ammunition, a grenade or the MP bomb lying in the world (SOCOM 1's DWARF:
 * CPickup, 0x1c bytes; SOCOM II's puts a float first -- its life, run down by the pickups' tick
 * -- so every member sits 4 further, and makes the type a whole byte and the owner's entity id a
 * word of its own at 0x10). pickup_main.cpp's; the list and the statics are OVL1 data. */
/* the weapon (zweapon.h's CZWeapon) and the ammunition (an OVL1 class: only its name) a pickup
 * holds */
class CZWeapon;
class CPickupAmmo;

class CPickup {
public:
    /* the kinds (SOCOM 1's PU_TYPE, nested; the values SOCOM II's code tests -- 8 a weapon, 9
     * ammunition (SOCOM 1's CleanupAmmoPickup tests the same two), 11 the MP bomb; the names
     * ours) */
    enum PU_TYPE {
        PU_NONE = 0, PU_TYPE_1 = 1, PU_TYPE_2 = 2, PU_TYPE_3 = 3, PU_TYPE_4 = 4, PU_TYPE_5 = 5,
        PU_TYPE_6 = 6, PU_TYPE_7 = 7, PU_WEAPON = 8, PU_AMMO = 9, PU_MPBOMB = 11, PU_LAST = 255
    };
    typedef std::list<CPickup *>::iterator iterator;

    static void DebugPickups();
    static void TickTimers(const CPnt3D *pos, float dT);
    static bool IsWeaponNear(unsigned char item, const CPnt3D *pos, float range);
    static CPickup *GetClosest(const CPnt3D *pos, float range, PU_TYPE type, CPickup **secondary,
                               bool no_special);
    static void CleanupAmmoPickup(CPickup *pickup);
    CPickupAmmo *GetAmmo();
    CZWeapon *GetWeapon();
    void *GetData4567();
    void *GetData123();
    /* the kinds' groups (inline: the accessors materialise each `||`). `m_type == 1 || m_type ==
     * 2` folds into one range test (sltiu); the ternary keeps GetData123's two compares */
    bool IsType1() { return m_type == PU_TYPE_1; }
    bool IsType2() { return m_type == PU_TYPE_2 ? true : false; }
    bool IsType12() { return IsType1() || IsType2(); }
    bool IsType123() { return IsType12() || m_type == PU_TYPE_3; }
    bool IsType75() { return m_type == PU_TYPE_7 || m_type == PU_TYPE_5; }
    bool IsType754() { return IsType75() || m_type == PU_TYPE_4; }
    bool IsType4567() { return IsType754() || m_type == PU_TYPE_6; }

    static CPickup *FindPickup(unsigned int id);
    static void DestroyNetPickups(unsigned int owner);
    static void Destroy(CPickup *pickup);
    static CPickup *GetMPBomb();
    /* SOCOM II's takes the whole id, the entity's in its upper half */
    static unsigned short GetEntityIDFromNetID(unsigned int netid);
    static iterator GetFirst();
    static CPickup *GetNext(iterator &it, PU_TYPE type);
    static CPickup *GetNext(iterator &it, const CPnt3D *pos, float range, PU_TYPE type);
    static void Open(unsigned int bits);
    /* a pickup made (from the pool when it holds one) and listed: an owner's (its id the next
     * pickup id) or one the network names (the owner in the id's upper half) -- SOCOM 1's, the
     * owner new; OVL2 0x542B90, 0x542930 */
    static CPickup *Create(unsigned int owner, PU_TYPE type, zdb::CNode *node, void *data);
    static CPickup *NetworkCreate(unsigned int netid, PU_TYPE type, zdb::CNode *node, void *data);
    /* (SOCOM 1's new CPickup: the secondary flag cleared through its bool, the bits sized) */
    CPickup(bool secondary = false) : m_secondary(secondary), m_bits(bit_count) {}
    /* a made pickup given its kind, node, contents, owner and id, its bits cleared (an inline:
     * Create and NetworkCreate store all five through one load of the pickup) */
    void Set(PU_TYPE type, zdb::CNode *node, void *data, unsigned int owner, unsigned short id)
    {
        m_type = type;
        m_node = node;
        m_data = data;
        m_owner = owner;
        m_id = id;
        m_bits.ClearAll();
    }
    /* the flags set (inlines: a constant false is `(b & 1) << n`) */
    void SetLocked(bool on) { m_locked = on; }
    void SetSecondary(bool on) { m_secondary = on; }
    /* every pickup destroyed, the MP bomb's node deleted and, when told to, the pool's pickups
     * deleted (SOCOM 1's; OVL2 0x542E50) */
    static void Close(bool all);
    /* the MP bomb's node taken out of the world and deleted (new in SOCOM II, the name ours:
     * OVL2 0x542180 touches only m_mpbombnode; Close inlines it) */
    static void DeleteMPBombNode();
    /* the pickups' tick in an online game, TickTimers' counterpart (CMission::Tick calls one or the
     * other with the player's place, ftsNetworkGameUninit this one with none): no time runs out,
     * only the MP bomb's node is deleted when flagged (OVL2 0x5423E0, new in SOCOM II; the names
     * ours) */
    static void NetTick(const CPnt3D *pos, float dT);
    static bool m_deletempbomb;   // OVL1 bss 0x44D4A8 (between m_mpbombnode and m_isOpen)
    unsigned int GetPickupNetEntityID();
    const CPnt3D *GetPosW() const;
    void SetVisibleByUnit(unsigned int unit);
    bool GetVisibleByUnit(unsigned int unit);

    static std::list<CPickup *> m_list;   // OVL1 bss 0x44D470
    static std::list<CPickup *> m_pool;   // OVL1 bss 0x44D490 (SOCOM 1's m_pool__7CPickup, after m_list)
    static zdb::CNode *m_mpbombnode;   // OVL1 bss 0x44D4A0
    static unsigned short nextPickupId;   // OVL1 data 0x3DF170 (SOCOM 1's, 1 at the start)
    static unsigned int bit_count;   // OVL1 data 0x3DF178
    static bool m_isOpen;   // OVL1 bss 0x44D4B0

    float m_timer;   // 0x0 (new in SOCOM II)
    zdb::CNode *m_node;   // 0x4
    void *m_data;   // 0x8: a grenade's projectile, a weapon's...
    /* (SOCOM 1's bit-fields of one word, 7 bits of type first: SOCOM II's code reads and writes
     * the flags a byte at a time -- Create's lbu/sb on 0xd) */
    unsigned char m_type;   // 0xc
    unsigned char m_locked : 1;   // 0xd
    unsigned char m_secondary : 1;
    unsigned char m_unused : 6;
    unsigned short m_id;   // 0xe
    unsigned int m_owner;   // 0x10: the owner's entity id (new in SOCOM II)
    /* SOCOM 1's CBits (at 0x10 there): Close's delete inlines its destructor, the constructors size
     * it, Create clears it. Spelled as CBits's layout with the same inlines and a conversion to a
     * CBits pointer, as the kit lane's seal_aistate.cpp reads it (`(CBits *)p->m_bits`); a class
     * derived from CBits would add its own destructor's this-test to Close's delete */
    struct Bits {
        Bits(unsigned int bits) : m_bits(0), m_vec(0) { ((CBits *)this)->Resize(bits); }
        ~Bits()
        {
            _free(m_vec, 0, 0);
            m_vec = 0;
        }
        operator CBits *() { return (CBits *)this; }
        void ClearAll() { ((CBits *)this)->ClearAll(); }

        unsigned int m_bits;
        unsigned int *m_vec;
    } m_bits;   // 0x14
    unsigned int m_visible_by_units;   // 0x1c (SOCOM 1's 0x18)
};

/* a thrown grenade's flight, solved (SOCOM 1's; only the statics) */
class CDynGrenade {
public:
    /* SOCOM II's takes a scale first (the signature ours) */
    static float ComputeTimeToImpact(float scale, float b, float c);
    static float ComputeElevOfs(float t);
    static float ComputeMaxVel(float dist, float height, float *vel);
};

/* where a look comes from (SOCOM 1's DWARF: AI_LOOK, a signed byte -- AIL_ALL is -1, compared
 * with `lb`; SOCOM II's SetDefaultLookParams takes twelve kinds; the names past AIL_DEPLOY ours).
 * The kit lane's seal_look.cpp defines the look setters below */
enum AI_LOOK {
    AIL_ALL = -1, AIL_NORMAL = 0, AIL_TURNING = 1, AIL_CURIOUS = 2, AIL_EVENT = 3, AIL_CORPSE = 4,
    AIL_TASK = 5, AIL_FRANTIC = 6, AIL_TARGET = 7, AIL_BUDDY = 8, AIL_DEPLOY = 9, AIL_10 = 10,
    AIL_11 = 11
};

/* a SEAL's controller: a pad, the AI or the network (SOCOM 1's DWARF: CSealCtrl, 0x240 bytes;
 * SOCOM II's is 0x250 -- ftsCreateSeal news 592). The virtual functions are SOCOM II's table,
 * __vt__9CSealCtrl at 0x6694B0: SOCOM 1's without PerformCollisionActions, with three setters of
 * the fudge velocities and one more after SlideTick, GetJumpFlag moved into the base (SOCOM 1's
 * CSealCtrlAi and CSealCtrlSquirm had their own) and a setter of the AI-map collision bit last.
 * The inline ones have an out-of-line copy per overlay (CSealCtrlCopy's OVL1 table holds the
 * other). */
class CSealCtrl : public CEntityCtrl {
public:
    CSealCtrl();
    /* the scan's tick (OVL2 0x600550, the kit lane's range: the map's ScanTick__9CSealCtrlFf; CSealCtrlAi's
     * PreTick, seal_control.cpp's, calls it) */
    void ScanTick(float dT);
    virtual ~CSealCtrl();
    virtual bool PreTick(float dT);
    virtual bool Tick(float dT);
    virtual bool PostMortemPreTick(float dT);
    virtual void OnDeath();                                                 /* +0x18 */
    /* SOCOM II's takes a chance to play it and the AI event the voice makes, and says whether it
     * played */
    virtual bool PlayAiSound(CHARACTER_SOUND sound, float chance, int event);   /* +0x24 */
    virtual bool AllowCommand(CZSealBody *seal, FT_COMMAND cmd, void *data);
    virtual bool IsPlayer() const;
    virtual bool IsAi() const { return false; }
    virtual bool IsRemote() const { return false; }                         /* +0x34 */
    virtual class CSealCtrlAi *GetSealCtrlAi() { return 0; }
    virtual bool PostTick(float dT);
    virtual void Initalize();                                               /* +0x40 */
    virtual void OnWeaponEvent(CZProjectile *proj, unsigned int event, void *data);
    virtual void OnRecycleEntity(CEntity *e);
    virtual void OnWeaponHit(zdb::IntersectStruct *hit, CZProjectile *proj);   /* +0x4C */
    virtual void OnStunDamage(STUN_TYPE type, float amount, const CPnt3D *from);
    virtual void OnSetHealth(float health, float old);
    virtual void ResetToInitialState();                                     /* +0x58 */
    virtual void OnMissionStart();
    virtual void OnMissionEnd();
    virtual void OnStateChange(SEAL_STATE from, SEAL_STATE to) {}           /* +0x64 */
    virtual void Dump();
    virtual unsigned int GetAiNetState() const;
    virtual void SetAiNetState(unsigned int state) {}                       /* +0x70 */
    virtual void PerturbAimVector(CPnt3D *v);
    virtual void OnActionButton();
    virtual void PerformReticleActions();                                   /* +0x7C */
    virtual void PerformProximityActions();
    virtual void OnCommand(CZSealBody *seal, FT_COMMAND cmd, void *data, bool b);
    /* SOCOM II's takes a flag (PreTick passes a bool set with daddiu, 0x567690 its own; the AI's
     * clears it and hands it to the base's) */
    virtual void CreepTick(float dT, bool b);                               /* +0x88 */
    virtual bool SlideTick(float dT);   /* SOCOM II's returns whether the SEAL slid */
    /* +0x90-0x98, new in SOCOM II: the fudge velocities set (the names ours) */
    virtual void SetFudgeVelM(const CPnt3D *v) { m_fudge_velM = *v; }
    virtual void SetFudgeVelR(const CPnt3D *v) { m_fudge_velR = *v; }
    virtual void SetFudgeVelW(const CPnt3D *v) { m_fudge_velW = *v; }
    /* +0x9C: the turn toward a goal direction -- none when the heading is within the dead zone,
     * proportional (clamped) inside 20 degrees, the full rate by the cross product's sign beyond --
     * with the heading's dot product handed back (SOCOM 1's non-virtual ComputeTurnThrottle, the
     * same body; CreepTick and 0x567900 call the slot with the heading, &m_goal_dir and a float) */
    virtual float ComputeTurnThrottle(const CPnt3D *dir, const CPnt3D *goal, float *dot);   /* +0x9C */
    virtual void NotifyTargetDown(CZSealBody *seal);
    /* +0xA4: the controller's current AI state kind (the base: none) -- typed AI_STATE, not bool:
     * CSealCtrlAi's 0x574800 has SOCOM 1's GetAiStateEnum body and GetAiNetState sends its value as the
     * network state's code (compared with 17 and 9); the name stays the map's (the SEALs' round 31) */
    virtual AI_STATE GetJumpFlag() const { return AI_STATE_UNKNOWN; }      /* +0xA4 */
    virtual void UpdateViewCone();
    virtual void OnSetTargetVisible(CTarget &target, bool visible, zdb::DiIntersect *di);
    virtual void SetTarget(CTarget *target);                                /* +0xB0 */
    /* +0xB4, new in SOCOM II: the AI-map collision bit (the name ours) */
    virtual void SetDoAiMapCollide(bool on) { m_do_aimap_collide = on; }

    /* the goals: a direction, a position (a point or a map cell's centre), cleared (seal_ai.cpp,
     * SOCOM 1's; OVL2 0x56D920, 0x56DAA0, 0x56DBA0, 0x56DCA0) */
    /* (SOCOM 1's returns the facing's dot product with the new direction, 1 when none is set: a
     * float in $f0 in both builds) */
    float SetGoalDir(GOAL_TYPE type, const CPnt3D *dir, bool flat);
    bool SetGoalPos(GOAL_TYPE type, const CPnt3D *pos, float tolerance);
    bool SetGoalPos(GOAL_TYPE type, const CAiMapLoc &loc, float tolerance);
    void ClearGoal(GOAL_TYPE type);
    /* the pad's rumble motors (SOCOM 1's) */
    void TickRumbler(float dT);
    void SetHighRumbleTime(float t);
    void SetLowRumbleTime(float t, float strength);
    void StopRumbler();
    /* the scope's zoom, handed to the camera (SOCOM 1's) */
    float GetZoomIncrement();
    float GetZoomLevel();
    bool UpdateHudWeapon();
    void SetCameraZoom(float level, float increment);
    void MPBomb_TimerAvailable(bool on);
    /* the MP bomb worked at: its timer run while the reticule stays on it, then the defusal or the
     * planting done (SOCOM 1's; OVL2 0x5920C0: SOCOM II's InitMPBomb is gone) */
    void TickMPBomb(float dT);
    /* SOCOM II's takes whether to keep the weapon's timer (the HUD passes false) */
    void SetMenuState(MENU_STATE state, bool keep);
    void SetTrigger(bool down);
    void SetTrigger(bool down, float power);
    /* how far the SEAL sees: none for the camera's far plane (SOCOM 1's; OVL2 0x5924B0) */
    void SetViewRange(float range);
    /* whether an action node's action can be taken now, and the bomb's planting (SOCOM 1's; OVL2
     * 0x594350, 0x594480) */
    bool CanPerformAction(class CNodeAction *action);
    bool CanPlaceBomb(class CNodeAction *action);
    /* the look and the scan (SOCOM 1's, OVL2 0x600D40-0x60187F: the kit lane's seal_look.cpp,
     * which reads the members through a view of its own). A setter answers whether the look was
     * taken -- a kind at least the current one's; SOCOM II's SetDefaultLookParams takes whether
     * the look's point changed kind, and SetLookPosW gained a cell's and an entity's overloads */
    void ClearLook(AI_LOOK look);
    /* an AI event of the controller's SEAL made (OVL2 0x572FA0; SOCOM 1's CreateAiEvent, by its
     * order and the voice's call; the event returned, where SOCOM 1's answered a bool) */
    CAiEvent *CreateAiEvent(CAiEvent::EVENT event, float duration, float range, bool visible, const CPnt3D *pos,
        unsigned int userdata);
    bool SetLookDirR(AI_LOOK look, const CPnt3D *dir, bool exact);
    bool SetLookDirW(AI_LOOK look, CAiMap::DIR dir);
    bool SetLookDirW(AI_LOOK look, const CPnt3D *dir, bool exact);
    bool SetLookPosW(AI_LOOK look, const CAiMapLoc &loc);
    bool SetLookPosW(AI_LOOK look, CEntity *e);
    bool SetLookPosW(AI_LOOK look, const CPnt3D *pos);
    void SetDefaultLookParams(AI_LOOK look, bool changed);
    void SetScanRate(float rate);
    void SetScanAngle(float angle);
    void ModifyLookFlags(unsigned int set, unsigned int clear);
    void SetLookFlags(unsigned int flags);
    void SetLookTimer(float t);
    void SetLookRate(float rate);
    /* the speed a throttle gives along an axis (SOCOM 1's sealctrl.cpp; OVL2 0x592880: the body's
     * GetMaxZMotion/GetMaxXMotion) */
    float GetSpeedFromThrottle(float throttle, int axis);

    unsigned char _18[0x24 - 0x18];
    /* SOCOM 1's AI_LOOK, a signed byte (AIL_ALL is -1: compared with `lb`); the enum is
     * seal_look.cpp's (the kit lane's), so a plain signed char here -- 9 is AIL_DEPLOY */
    signed char m_look_mode;   // 0x24 (SOCOM 1's 0x20)
    unsigned char _25[0x60 - 0x25];
    /* the scan's reach, scaling the yaw throttle's look in Tick (SOCOM 1's m_scan_limits, at 0x60 in
     * both builds: the scan timer ends there in each) */
    float m_scan_limits;   // 0x60
    unsigned char _64[0xa8 - 0x64];
    CAiDecay m_visrange;   // 0xa8 (as SOCOM 1's)
    /* the alertness: its value and the goal it holds to (SOCOM 1's m_aware, 0xc0 there -- SOCOM II
     * has no m_vissens before it; the SetAiAware pair 0x561110/0x5612F0 set it, the bands of its value
     * are 0x560A60's) */
    CAiDecay m_aware;   // 0xb4
    unsigned char _c0[0xd0 - 0xc0];
    /* the character's AI parameters (new in SOCOM II, the vehicle entry's own type, zchar.h's: the
     * constructor builds AI_PARAMS() in place, DoRespawn copies the entry's in but the respawn count
     * and keeps the entry as m_veh, 0x570D90 falls back to the courage, the view range's setter
     * stores visual_range; the name ours) */
    AI_PARAMS m_aiParams;   // 0xd0
    CSealUnit *m_unit;   // 0x10c: the squad (SOCOM 1's 0xdc)
    /* the state the network last gave (SOCOM 1's m_ai_net_state, 0xe4 there: SOCOM II has no
     * m_vismap before it; CRemoteCtrl::SetAiNetState stores it) */
    unsigned int m_ai_net_state;   // 0x110
    unsigned int m_padid;   // 0x114 (SOCOM 1's 0xe8: SOCOM II dropped the ten button readings)
    TRIGGER m_trigger;   // 0x118 (SOCOM 1's 0xec)
    float m_trigger_power;   // 0x11c
    unsigned char _120[0x128 - 0x120];
    float m_zoomlevel;   // 0x128 (as SOCOM 1's)
    float m_zoomincrement;   // 0x12c
    unsigned char _130[0x13c - 0x130];
    float m_highrumbletime;   // 0x13c (SOCOM 1's 0x138)
    float m_rumbletime;   // 0x140
    float m_rumblestr;   // 0x144
    int m_HudDoPickup;   // 0x148 (SOCOM 1's 0x144)
    int m_curpickup_ID;   // 0x14c
    int m_curpickup_secondary_ID;   // 0x150
    int m_curMPBOMBpickup_ID;   // 0x154 (SOCOM 1's 0x150)
    /* the bomb being defused, and the one being worked with its timer (SOCOM 1's, 0x154-0x15c there:
     * TickMPBomb) */
    zdb::CNode *m_bomb_defusal_node;   // 0x158
    zdb::CNode *m_mpbomb_node;   // 0x15c
    float m_mpbomb_timer;   // 0x160
    unsigned char _164[0x16c - 0x164];
    /* the entity the SEAL may work (new in SOCOM II, SOCOM 1 has no member between m_corpsebody and
     * the bits: 0x5A88C0 asks its slot 0x5C for action 19; the name ours) */
    CEntity *m_16c;   // 0x16c
    unsigned int m_goal_pos_valid : 1;   // 0x170 (SOCOM 1's 0x168, the same bits)
    unsigned int m_goal_dir_valid : 1;
    unsigned int m_action : 1;
    unsigned int m_wasthrowing : 1;
    unsigned int m_HudUpdateWeapon : 1;
    unsigned int m_HudUpdateViewCone : 1;
    unsigned int m_throwing : 1;
    unsigned int m_justthrew : 1;
    unsigned int m_update_zoom : 1;
    unsigned int m_zoomin : 1;
    unsigned int m_zoomout : 1;
    unsigned int m_input_enabled : 1;
    unsigned int m_mpbomb_bTimerAvailable : 1;
    unsigned int m_mpbomb_bTimerEngaged : 1;
    unsigned int m_has_respawned : 1;
    unsigned int m_do_aimap_collide : 1;
    unsigned int m_bit16 : 1;   // new in SOCOM II (the constructor sets both)
    unsigned int m_bit17 : 1;
    unsigned int m_bit18 : 1;   // new in SOCOM II: the reload button held (0x594B70; the name ours)
    unsigned int m_unused : 13;
    CPnt3D m_goal_dir;   // 0x174 (SOCOM 1's 0x16c)
    CPnt3D m_goal_pos;   // 0x180
    CPnt3D m_fudge_velR;   // 0x18c
    CPnt3D m_fudge_velM;   // 0x198
    CPnt3D m_fudge_velW;   // 0x1a4
    GOAL_TYPE m_goal_type;   // 0x1b0 (SOCOM 1's 0x1a8)
    float m_goal_tolerance;   // 0x1b4
    CAiTimer m_goal_timer;   // 0x1b8
    float m_speed;   // 0x1c4 (SOCOM 1's 0x1bc)
    /* when each kind of AI event was last made (SOCOM 1's m_event_time[15], 0x1c0: CreateAiEvent) */
    float m_event_time[15];   // 0x1c8
    unsigned char _204[0x221 - 0x204];
    MENU_STATE m_menustate;   // 0x221 (as SOCOM 1's)
    unsigned char _222[0x238 - 0x222];
    float m_bombpickuptimer;   // 0x238 (as SOCOM 1's: TickMPBomb sets it once a defusal starts)
    unsigned char _23c[0x240 - 0x23c];
    float m_240;   // 0x240: how long the reload button has been held (0x594B70; the name ours)
    float m_244;   // 0x244: CanPlaceBomb's wait, set to 1.5 when it refuses (the name ours)
    unsigned char _248[0x250 - 0x248];
};

/* the AI's controller (SOCOM 1's DWARF: CSealCtrlAi, 0x5d0 bytes; SOCOM II's is 0x7c0 --
 * ftsCreateSeal news 1984) -- only what the written functions touch */
class CZAnim;

/* the AI's sound loops (SOCOM 1's AI_SOUND, a signed byte -- the statics' index is sign-extended;
 * two of them, the names ours) */
enum AI_SOUND { AI_SOUND_NONE = -1, AI_SOUND_0 = 0, AI_SOUND_1 = 1, AI_SOUND_MAX = 2 };

/* the AI's modes (SOCOM 1's DWARF, a byte; only the size matters here) */
enum AI_MODE { AI_MODE_0 = 0, AI_MODE_LAST = 255 };

/* how alert the AI is, the bands of its alertness (SOCOM 1's DWARF: AI_AWARE, a byte) */
enum AI_AWARE { AIA_UNAWARE = 0, AIA_SUSPICIOUS = 1, AIA_GENERAL = 2, AIA_DIRECTION = 3, AIA_SPECIFIC = 4 };

/* the AI's valves (SOCOM 1's SEAL_VALVE, a byte: SOCOM II names two, the loops make three and 4 is
 * none -- the names ours but the valves') */
enum SEAL_VALVE { SEAL_VALVE_0 = 0, SEAL_VALVE_ALIVE = 1, SEAL_VALVE_AWARE = 2, SEAL_VALVE_3 = 3, SEAL_VALVE_NONE = 4 };

class CValve;

/* a task the AI takes on (SOCOM 1's SEAL_TASK, a byte; only the name matters here) */
enum SEAL_TASK { TASK_0 = 0, TASK_LAST = 255 };

/* a route's state (a byte enum: copied with lb, compared with lbu; the names ours -- 1 and 2 are
 * the two a reset ends) */
enum AI_ROUTE { ROUTE_NONE = 0, ROUTE_1 = 1, ROUTE_2 = 2 };

/* the destructors the AI's data runs by hand (its members' own, out of line: a route's (OVL2
 * 0x573F20, the implicit destructor of a CAiPath plus a byte), CAiEvent's (0x573F80) and
 * CAiEventQ's (0x573FF0) -- seal_ai.cpp's copies, after ~CSealCtrlAi -- by their mangled names, and
 * CAiPath's (0x5281B0, ai_states.h's __dt__7CAiPathFv: ai.h does not declare it, its implicit one
 * would be inlined) */
extern "C" {
void *__dt__9CAiEventQFv(void *queue, int flags);
void *__dt__8CAiEventFv(void *event, int flags);
void *func_00573F20(void *route, int flags);
}

/* what SEAL_AI_PARAMS's 0x444 points at while the AI climbs: only the ladder's index is read -- bits
 * 4-19 of its second word (0x56BD10; the class and its names ours) */
struct CAiLadderLeg {
    unsigned int _0;   // 0x0
    unsigned int m_4 : 4;   // 0x4
    unsigned int m_ladder : 16;
    unsigned int m_20 : 12;
};

/* a CAiPath's bytes (ai.h's class, 0xA8: built and destroyed by hand here, as ai.h does not
 * declare SOCOM II's ~CAiPath) and the fields the controller reads: the plan's state, the search's
 * status, the smoothed points' end (an empty vector's is 0) */
struct CAiPathBytes {
    unsigned char _0[0xc];
    float m_type_bias;   // 0xc (SOCOM 1's: the planner's bias, set with the type)
    unsigned char _10[0x80 - 0x10];
    unsigned char m_state;   // 0x80: CAiPath::PATH_STATE (1-3 planning, 4 complete)
    signed char m_status;   // 0x81: CAiPath::ASTAR_STATUS
    unsigned char _82[0x8c - 0x82];
    unsigned int m_search_limit;   // 0x8c (SOCOM 1's 0x84)
    unsigned char _90[0x9c - 0x90];
    void *m_9c;   // 0x9c: the points vector's second word
    unsigned char _a0[0xa8 - 0xa0];
};

/* a route: a path and its state (a byte enum: copied with lb, compared with lbu; the class and its
 * names ours -- SEAL_AI_PARAMS's constructor builds the path and zeroes the byte, its implicit
 * destructor's copy is OVL2 0x573F20) */
struct CAiRouteBytes {
    CAiPathBytes m_path;   // 0x0
    AI_ROUTE m_state;   // 0xa8
};

/* a stamp of the AI's at 0x324 (new in SOCOM II; the class and its names ours): the mission's time
 * when it was restarted and four fields its restart clears. Reset is an ordinary function of
 * seal_ai.cpp (OVL2 0x558590), which the sleep setters there inline; the avoid and path-recover
 * states, CAiGroup::AddMember and SetGroup call it */
class CAiStamp {
public:
    void Reset();

    float m_time;   // 0x0
    /* (a float: the alert records' amount -- 0x5582C0's inlined Reset stores the 0.0 its distance
     * holds in $f2) */
    float m_4;   // 0x4
    unsigned char m_8;   // 0x8
    unsigned char _9[0x1c - 0x9];
    unsigned int m_1c;   // 0x1c
    unsigned char m_20;   // 0x20
    unsigned char _21[0x24 - 0x21];
};

/* the fire parameters (SOCOM 1's DWARF AI_FIREPARAMS, 0x60 bytes; SOCOM II's are 0x70, the flags
 * at 0x6C -- the kit lane's seal_aiparams.cpp zeroes them there before the reset 0x55F6F0). Only
 * the members SOCOM II's code shows at SOCOM 1's offsets are named */
class AI_FIREPARAMS {
public:
    unsigned char _0[8];   // SOCOM 1: m_weapon, m_ammo
    CPnt3D m_targetPos;   // 0x8
    CPnt3D m_targetVel;   // 0x14
    CPnt3D m_lookDirW;   // 0x20
    CPnt3D m_aimPointW;   // 0x2c
    CPnt3D m_aimDirM;   // 0x38
    unsigned char _44[0x58 - 0x44];   // SOCOM 1: m_throwVec, m_throwPower
    float m_timeout;   // 0x58: run up by FireWhenReady (0x563BD0; SOCOM 1's 0x54)
    float m_dist;   // 0x5c: the target's distance (0x55EBE0; SOCOM 1's flags were here)
    /* the spray's spread, zeroed with each burst (0x5659E0; the kit lane's ModifyAimDir call reads
     * it -- the name ours) */
    float m_spread;   // 0x60
    /* under flag 0x40, the time left before the shot -- run down by FireWhenReady (new in SOCOM II;
     * the name ours) */
    float m_delay;   // 0x64
    unsigned char _68[0x6c - 0x68];
    unsigned int m_flags;   // 0x6c
};

/* the AI's data, CSealCtrlAi's second base (new in SOCOM II: CSealCtrlAi's RTTI, OVL2 0x65C6D0,
 * lists SEAL_AI_PARAMS at 0x250 -- after CEntityCtrl and CSealCtrl at 0 -- and SEAL_AI_PARAMS's own,
 * 0x65C660, no bases). It holds what SOCOM 1's CSealCtrlAi held, in the same order: its
 * constructor (OVL2 0x60BF40, out of line, in the AI states' range) runs before CSealCtrlAi's table
 * is stored and builds the state stack (+0x1CC), two CAiPaths (+0x21C, the route's +0x2C4), the
 * event (+0x3D8), the event queue (+0x400), the target (+0x42C, as CTurret's constructor does) and
 * the trigger (+0x4E4); ~CSealCtrlAi inlines its destructor behind the base conversion's null
 * test of `this + 0x250`, the members destroyed out of line in reverse order. Offsets below are
 * the controller's (the base's plus 0x250). Its end is not proven: everything past CSealCtrl is
 * here (CSealCtrlAi itself has no member with a destructor, none being destroyed outside the
 * null test). */
class SEAL_AI_PARAMS {
public:
    SEAL_AI_PARAMS();
    /* written out, its members' destructors called by hand where the implicit one would run them
     * (CAiPath's is undeclared), the state stack last as a real member */
    ~SEAL_AI_PARAMS()
    {
        __dt__9CAiEventQFv(m_eventQueue, -1);
        __dt__8CAiEventFv(m_event, -1);
        func_00573F20(&m_route, -1);
        __dt__7CAiPathFv(&m_path, -1);
    }

    unsigned char _250[0x254 - 0x250];
    float m_throttle_goal[3];   // 0x254 (as SOCOM 1's 0x274: the throttles the AI asks for)
    unsigned int m_260;   // 0x260
    unsigned int m_debug : 1;   // 0x264 (SOCOM 1's 0x280, the same bits as far as they are read)
    unsigned int m_navyseal : 1;   // bit 1 (also no CQB delay: CAiSCQBAttack's entry reads it)
    /* bits 2, 13, 16 and 17 as the AI states read them (the kit lane, round 6; the names ours):
     * bit 2 the one 0x56D890 sets, which a covered state keeps (0x5F6350); a breach clears 13 on
     * entry, then sets 16 from its own bit 13, clears 17 and sets 13 as it ends (0x5F3620) */
    unsigned int m_aibit2 : 1;
    unsigned int m_bits3 : 3;
    /* bit 6: held set while CTMacro::Tick (0x5D6DC0) pushes its script state, then put back (the
     * kit lane, round 23; the name ours) */
    unsigned int m_aibit6 : 1;
    /* bit 7: set and cleared by 0x561E20 (CAiSNoAmmo's Tick sets it), never with bit 1 (the name
     * ours) */
    unsigned int m_aibit7 : 1;
    unsigned int m_aibit8 : 1;   // bits 8 and 9: either refuses a state push without flag 4
    unsigned int m_aibit9 : 1;   // (0x5F5CD0) -- SOCOM 1's m_sleeping and m_snoozing (its DWARF)
    unsigned int m_nosnooze : 1;   // bit 10 (SOCOM 1's: SetSnoozing refuses the doze with it set)
    unsigned int m_bits11 : 2;
    unsigned int m_aibit13 : 1;
    unsigned int m_aibit14 : 1;
    /* bit 15: with bit 8 (SOCOM 1's m_sleeping) set, the alertness is left alone (both SetAiAwares
     * return at once; new in SOCOM II, the name ours) */
    unsigned int m_aibit15 : 1;
    unsigned int m_aibit16 : 1;
    unsigned int m_aibit17 : 1;
    unsigned int m_bits18 : 14;
    unsigned char _268[0x26c - 0x268];
    unsigned char m_26c;   // 0x26c: a mode byte (4: any target taken; the name ours)
    unsigned char _26d[0x26e - 0x26d];
    unsigned char m_26e;   // 0x26e: a mode byte (2 refuses SetSprayTimer; the name ours)
    unsigned char _26f[0x270 - 0x26f];
    /* the teams whose presence in a region wakes the SEAL, and the region (0x56D450 sets them -- 14
     * when none is given -- and 0x56D2B0 asks 0x56D530 of them; the mask unsigned: the target loads
     * it as the call's first operand; the names ours) */
    unsigned int m_270;   // 0x270
    void *m_274;   // 0x274
    SEAL_STANCE m_stance;   // 0x278 (SOCOM 1's 0x285)
    SEAL_STANCE m_stance_asked;   // 0x279: the stance SetStance was last asked for (the names ours)
    float m_stance_delay;   // 0x27c (SOCOM 1's 0x288)
    float m_280;   // 0x280: the delay before the asked stance is taken (cleared by SetStance)
    unsigned char _284[0x324 - 0x284];
    CAiStamp m_stamp;   // 0x324 (restarted by the sleep setters, the avoid and path-recover states)
    unsigned char _348[0x364 - 0x348];
    CAiTimer m_stance_timer;   // 0x364: restarted with each desired stance (0x5631B0; the name ours)
    /* the fire's parameters (the AI states hand them to FireWhenReady, 0x5B8920 its 0x55EAA0; the
     * deploy's signals drop their aim) */
    AI_FIREPARAMS m_fire;   // 0x370
    /* what 0x561E20 stores as it sets bit 7: a byte and 16 bytes zeroed (the names ours) */
    signed char m_3e0;   // 0x3e0
    unsigned char _3e1[0x3e4 - 0x3e1];
    unsigned char m_3e4[0x10];   // 0x3e4
    /* a mode (7 keeps the AI-map collision off; the name ours): an AI_MODE, copied with lb (0x557F60) */
    AI_MODE m_3f4;   // 0x3f4
    AI_MODE m_3f5;   // 0x3f5: the mode RestoreAiMode returns to, then 1 (a byte enum, copied with lb; the name ours)
    unsigned char _3f6[0x3fc - 0x3f6];
    /* the countdown to the next glance around: 0x567690 runs it down and restarts it at a random 10
     * to 15, zeroed while the AI has a target or acts (SetAiMode sets it; the name ours) */
    float m_3fc;   // 0x3fc
    /* a Navy SEAL's anger at the player: Tick and OnDeath raise it by 60 a second, 0x561B90 cools it
     * and turns the squad on the player past 190 (new in SOCOM II; the name ours) */
    float m_400;   // 0x400
    unsigned char _404[0x40c - 0x404];
    float m_40c;   // 0x40c: a time on the game's clock 0x572190 measures from (the name ours)
    unsigned char _410[0x41c - 0x410];
    /* the state on top of the stack (SOCOM 1's; OVL2 0x5F5FD0, 0x5F6010, the AI-states lane's
     * seal_aistate.cpp defines them -- which is const by SOCOM 1's order) */
    CAiStateStack m_stateStack;   // 0x41c (SOCOM 1's 0x324; 0x20 bytes in SOCOM II)
    unsigned char _43c[0x440 - 0x43c];
    /* three words 0x56BEF0 clears -- the third's low half only -- as the AI leaves a ladder (SetAiMode
     * and 0x56AF50 read them, CanTraverse the third; new in SOCOM II, the names ours) */
    unsigned int m_440;   // 0x440
    /* (a pointer: 0x56BD10 reads the ladder's index off what it points at, or takes none -- 0xFFFF --
     * when it is null) */
    struct CAiLadderLeg *m_444;   // 0x444
    unsigned int m_448;   // 0x448
    unsigned char _44c[0x450 - 0x44c];
    /* the anims the tasks play (SOCOM 1's m_task_anim[6], 0x33c there, its m_task_handle[6] 0x18
     * after: SetupAnimations binds [0] to <node>_start and [2] to <node>_ambient) -- so the
     * m_task_anim below, at 0x468, is SOCOM 1's m_task_handle[0] */
    CZAnim *m_anims[6];   // 0x450
    CZAnim *m_task_anim;   // 0x468: an anim the AI plays, stopped at the mission's end
    CAiPathBytes m_path;   // 0x46c: a CAiPath (SOCOM 1's m_path, 0x36c)
    CAiRouteBytes m_route;   // 0x514: a route (its path reset at 0x527310, its state read after
                             // the update at 0x555420)
    float m_5c0;   // 0x5c0: a time 0x564CF0 measures a span from (the name ours)
    /* the alertness's band before its value last changed (SetAiAware(float, unsigned int) 0x5612F0
     * stores it; HandleKiller tests it under 4; the name ours) */
    unsigned char m_5c4;   // 0x5c4
    unsigned char _5c5[0x5c8 - 0x5c5];
    unsigned int m_5c8;   // 0x5c8: zeroed when the alertness's band changes (0x5612F0; the name ours)
    /* an alertness asked for with a delay, and the delay (SOCOM 1's m_awareTimer, 0x5cc there too:
     * SetAiAware(AI_AWARE, const CPnt3D *, float) 0x561110 sets both; the second's name ours) */
    float m_awareTimer;   // 0x5cc
    AI_AWARE m_awarePending;   // 0x5d0
    unsigned char _5d1[0x5d4 - 0x5d1];
    /* the SEAL followed and its entity id (SOCOM 1's SetFollowMaster sets them; SetFollowMaster
     * counts the master's followers -- the kit lane, round 19, the names ours) */
    class CZSealBody *m_follow_master;   // 0x5d4
    unsigned int m_follow_master_id;   // 0x5d8
    float m_pathTolerance;   // 0x5dc (SOCOM 1's 0x47c: a new route zeroes it)
    /* the leg being walked: its two cells' centres flat on the ground, the direction between them
     * and its length, four words cleared with it, and the cell it starts from (0x56C080 sets them
     * from a path's last point; the names ours) */
    CPnt3D m_leg_from;   // 0x5e0
    CPnt3D m_leg_to;   // 0x5ec
    CPnt3D m_leg_dir;   // 0x5f8
    float m_leg_len;   // 0x604
    /* the walk along the leg: how far is done, a word, the time since the side step was last picked
     * and the side step itself (0x5698A0 reads them as floats, 0x56C080 clears them; the names ours) */
    float m_leg_done;   // 0x608
    unsigned int m_60c;   // 0x60c
    float m_strafe_time;   // 0x610
    float m_strafe;   // 0x614
    CAiMapLoc m_leg_cell;   // 0x618
    unsigned char _61c[0x628 - 0x61c];
    unsigned char m_event[0x28];   // 0x628: a CAiEvent (SOCOM 1's m_event, 0x480)
    unsigned char m_eventQueue[0xc];   // 0x650: a CAiEventQ (SOCOM 1's m_eventQueue, 0x4a8: SOCOM II's lists are 0xc bytes)
    /* the kinds of event the AI hears (SOCOM 1's m_eventMask, 0x4b8: UpdateEventMask, OVL2 0x572D70,
     * stores it at 0x65c) */
    unsigned int m_eventMask;   // 0x65c
    unsigned char m_660;   // 0x660: set at the mission's start in mode 1 (the names ours)
    unsigned char _661[0x664 - 0x661];
    CZSealBody *m_664;   // 0x664: the player, in mode 1
    unsigned char _668[0x674 - 0x668];
    float m_674;   // 0x674: a range, and its square
    float m_678;   // 0x678
    unsigned char m_67c[4];   // 0x67c: an object the state CAiSPursue tests (0x60A850)
    unsigned char _680[0x6a8 - 0x680];
    unsigned int m_6a8;   // 0x6a8: the target's flags (bit 9 lets a held target interrupt: the kit lane)
    unsigned char _6ac[0x6f4 - 0x6ac];
    /* a range and its goal, set together unless the range stands at 42 (0x570D90; the names ours) */
    float m_6f4;   // 0x6f4
    float m_6f8;   // 0x6f8
    unsigned char _6fc[0x718 - 0x6fc];
    /* what 0x572240 gathers from an alert: the amounts summed, a point the alert fills, and its word
     * at 4 (the names ours) */
    float m_718;   // 0x718
    unsigned char _71c[0x724 - 0x71c];
    CPnt3D m_724;   // 0x724
    int m_730;   // 0x730
    /* two random times, each an Rfloat {min, range} (the units' own class: read through a cast), and
     * what an attack's reset rolled from them -- the first's, and the first's plus the second's
     * (0x565B80; the names ours) */
    float m_734[2];   // 0x734
    float m_73c[2];   // 0x73c
    float m_744;   // 0x744
    float m_748;   // 0x748
    float m_spraytimer;   // 0x74c: SetSprayTimer's time (the name ours)
    CValve *m_valve[3];   // 0x750 (SOCOM 1's 0x5bc, four there)
    /* the sound the AI last made (0x571010: the sound, when it may be made again, and its
     * priority with the alertness band it was made at in the low byte; the names ours) */
    short m_sound;   // 0x75c
    unsigned char _75e[0x760 - 0x75e];
    float m_sound_time;   // 0x760
    AI_SOUND_PRIORITY m_sound_priority;   // 0x764
    unsigned char _768[0x76c - 0x768];
    unsigned char m_weapon_kind;   // 0x76c: the kind 0x560540 reads off the body's weapon (the name ours)
    unsigned char _76d[0x774 - 0x76d];
    /* a point a thrower's own grenade is measured from (the grenade state's Tick, 0x55AC20: within
     * 50 of it the throw back is not wanted; the name ours) */
    CPnt3D m_774;   // 0x774
    unsigned char _780[0x7c0 - 0x780];
};

class CSealCtrlAi : public CSealCtrl, public SEAL_AI_PARAMS {
public:
    CSealCtrlAi();
    /* (0x573E60: the stack emptied, then the bases' destructors) */
    virtual ~CSealCtrlAi();
    virtual void SetThrottle(unsigned int axis, float value);
    virtual bool SlideTick(float dT);
    /* the turn toward a goal: CSealCtrl's at a rate of 2, slowed by the speed when the alertness is
     * low or the look deploys, unless a Navy SEAL in mode 6 (OVL2 0x56C340) */
    virtual float ComputeTurnThrottle(const CPnt3D *dir, const CPnt3D *goal, float *dot);
    /* never on in the AI mode 7 (OVL2 0x557EE0) */
    virtual void SetDoAiMapCollide(bool on);
    virtual void OnSetHealth(float health, float old);
    virtual void OnMissionEnd();
    virtual void OnStateChange(SEAL_STATE from, SEAL_STATE to);
    /* the state sent to the other consoles, as a word's bits (OVL2 0x56F6C0, slot 0x6C) */
    virtual unsigned int GetAiNetState() const;
    /* slot 0xA4 (OVL2 0x574800: the current state's kind, or 0 -- SOCOM 1's GetAiStateEnum's body) */
    virtual AI_STATE GetJumpFlag() const;
    /* a target seen to fall called out (SOCOM 1's; OVL2 0x5C9B10, the kit lane's seal_sensors.cpp) */
    virtual void NotifyTargetDown(CZSealBody *seal);
    /* the view cone set from the head (SOCOM 1's; OVL2 0x5C9400, the kit lane's seal_sensors.cpp) */
    virtual void UpdateViewCone();
    /* an enemy first seen by the squad called out, by its place on the player's clock (SOCOM 1's,
     * which took the CTarget; OVL2 0x5C9930, the kit lane's seal_sensors.cpp) */
    void EnemyCallout(CEntity *e, CEntity *player);
    /* the AI's own event set: its kind, its duration and where (SOCOM 1's, the same words with the
     * event at 0x628; OVL2 0x5FAD00, the kit lane's seal_aievent.cpp) */
    CAiEvent::EVENT SetAiEvent(CAiEvent::EVENT e, float duration, const CPnt3D *pos);
    /* the alertness raised to a band's top, at once or (a positive time, or -1 for the difficulty's)
     * after a delay, its event made when the band is DIRECTION and a point is given (SOCOM 1's
     * SetAiAware(AI_AWARE, const CPnt3D *), the time new: OVL2 0x561110); its value and goal set,
     * by the flags (1 the value, 2 the goal, 4 added to the goal), the goal kept up past GENERAL
     * (SOCOM 1's SetAiAware(float, unsigned int): OVL2 0x5612F0) */
    void SetAiAware(AI_AWARE aware, const CPnt3D *pos, float time);
    void SetAiAware(float value, unsigned int flags);
    /* the band the alertness's value lies in (SOCOM 1's; OVL2 0x560A60, inlined in seal_ai.cpp) */
    AI_AWARE GetAiAware() const;
    /* SOCOM 1's m_sleeping, and bit 15 (the target materialises each bit: `? true : false`) */
    bool IsSleeping() const { return m_aibit8 ? true : false; }
    bool IsAiBit15() const { return m_aibit15 ? true : false; }

    /* the scripted group the AI is in, or none (ai_group.cpp's, OVL2 0x60E7A0) */
    class CAiGroup *GetGroup();
    /* the AI's mode set (SOCOM 1's; OVL2 0x56CBF0) and its states dropped (SOCOM 1's
     * ClearAiState, the AI-states lane's 0x5F5F20) */
    void SetAiMode(AI_MODE mode);
    bool ClearAiState(bool all);   /* returns a bool (the kit lane: 0x5F5F20 sets v0) */
    /* the kinds of event heard, by the SEAL's team and sleep (SOCOM 1's; OVL2 0x572D70) */
    void UpdateEventMask();
    /* asleep or awake, and dozing or not (SOCOM 1's SetSleeping and SetSnoozing: bits 8 and 9 at
     * 0x264; OVL2 0x56D0C0, 0x56D6A0) */
    void SetSleeping(bool on);
    void SetSnoozing(bool on);
    /* the state stack's operations as the controller offers them (SOCOM 1's, their bodies the
     * stack's now: OVL2 0x5F5BA0-0x5F5CB0 and 0x5F5FC0, the AI-states lane's seal_aistate.cpp):
     * a state set or pushed when CanPushAiState allows it -- SOCOM 1's bool now a flags word it
     * tests (0x5F5CD0) -- popped to a state or a kind, popped, and found */
    bool SetAiState(AI_STATE state, unsigned int flags);
    bool PushAiState(AI_STATE state, unsigned int flags);
    bool PopToAiState(CAiState *state, bool inclusive);
    bool PopToAiState(AI_STATE state, bool inclusive);
    AI_STATE PopAiState();
    AI_STATE PopAiState(AI_STATE state);
    CAiState *FindAiState(AI_STATE state) const;
    bool CanPushAiState(AI_STATE state, unsigned int flags);
    /* the SEAL to follow set, the old master's followers counted down and the new one's up -- a
     * live one, not the SEAL itself, or none; without a master the follow state's running bit is
     * cleared and its master forgotten; whether it was taken (SOCOM 1's SetFollowMaster, which
     * returned nothing; OVL2 0x5FCF10, seal_aifollow.cpp's) */
    bool SetFollowMaster(class CZSealBody *master);
    /* the SEAL followed, or with `recurse` the head of the chain of masters, each master's AI asked
     * in turn (SOCOM 1's GetFollowMaster(int *), which counted the chain; OVL2 0x5FCD30, recursive:
     * it inlines itself four times, the two accessors under it called out of line at the bottom --
     * 0x5FCF00 and seal_unit.cpp's 0x5EA7C0) */
    class CZSealBody *GetFollowMaster(bool recurse);

    /* the animations bound (SOCOM 1's; OVL2 0x562AD0) and a task begun: its kind, a flag, then the
     * task's own arguments (SOCOM 1's; OVL2 0x561F00) */
    bool SetupAnimations();
    bool ActivateEx(SEAL_TASK task, bool b, ...);
    /* an anim's signal taken: 1, 2, 3 and 6 handed to every state, the top last-first; 4 the suspended
     * state pushed, 5 popped; then the stack's bits (SOCOM 1's AnimSignal; OVL2 0x562030) */
    void AnimSignal(CZAnim *anim, AI_SIGNAL sig);
    /* the stance the body is to take, set when prone, when a target is held with its bit 9, or
     * when forced, its timer restarted with the delay (OVL2 0x5631B0, new in SOCOM II: CTStance's
     * Begin and End call it; the name ours) */
    void SetDesiredStance(SEAL_STANCE stance, float delay, bool force);
    /* whether to duck for the leader (or the squad's), and how (SOCOM 1's; OVL2 0x5632A0) */
    bool GetDuckStance(class CZSealBody *leader, SEAL_STANCE &stance) const;
    /* a stance taken: remembered, the word at 0x280 cleared, then DoSetStance (SOCOM 1's SetStance
     * and DoSetStance: CTStance::Begin calls the first, which tail-calls the second; OVL2 0x5631A0,
     * 0x562B70) */
    bool SetStance(SEAL_STANCE stance);
    bool DoSetStance(SEAL_STANCE stance);
    /* the mode saved at 0x3F5 set again, the saved one made 1 (OVL2 0x56D090, new in SOCOM II:
     * CTHold::End calls it where SOCOM 1's called SetAiMode; the name ours) */
    void RestoreAiMode();
    /* a stance asked for after a delay, -1 for a random one of 0.25 to 1.2 seconds (SOCOM 1's
     * SetStanceDelayed; OVL2 0x563100); the same when the SEAL stands, or was asked to stand, in a
     * given stance (OVL2 0x563050, new in SOCOM II; the name ours) */
    bool SetStanceDelayed(SEAL_STANCE stance, float delay);
    /* the throttles let go: each set to zero through SetThrottle and, unless kept, zeroed where they
     * are stored (OVL2 0x567840, new in SOCOM II: CSealCtrlAi::OnCommand and CAiSSuspended::OnSignal
     * call it; the name ours) */
    void StopThrottles(bool keep);
    /* whether the SEAL is in a stance: the AI's own, or the one its body's state stands for (OVL2
     * 0x562FC0, new in SOCOM II: CAiSFollow::OnSignal asks it; the name ours) */
    bool InStance(SEAL_STANCE stance, bool fromState);
    /* the weapon fired once ready (OVL2 0x563BD0; SOCOM 1's) */
    bool FireWhenReady(AI_FIREPARAMS &params, float delay);
    /* the bit 2 at 0x264 set, never for a SEAL whose team has bit 17 or 26 or that holds no item
     * (OVL2 0x56D890: CAiSPlan::Tick and CAiState::Signal call it; the name ours) */
    void SetAiBit2(bool on);
    bool ChangeStanceDelayed(SEAL_STANCE from, SEAL_STANCE to, float delay);
    virtual void OnMissionStart();
    /* the valves: a name for each (inline everywhere in SOCOM II: its three callers expand it),
     * the valve of a name, the valve (SOCOM 1's; OVL2 0x574730, 0x5621C0) */
    static const char *get_valve(SEAL_VALVE v)
    {
        switch (v) {
        case SEAL_VALVE_ALIVE:
            return "valve_alive";
        case SEAL_VALVE_AWARE:
            return "valve_aware";
        case SEAL_VALVE_3:
            break;
        case SEAL_VALVE_NONE:
            break;
        }
        return "error";
    }
    static SEAL_VALVE get_valve(const char *name);
    CValve *GetValve(SEAL_VALVE v) const;
    /* a loop stopped, and started when it is not playing (SOCOM 1's statics; OVL2 0x574AE0,
     * 0x574B30 -- UpdateAiMapLoc calls them), its sounds and their instances (SOCOM 1's
     * aiSoundHandle and aiSoundInstance; OVL1 bss 0x44D4D8, 0x44D4E0). The base's PlayAiSound
     * stays visible beside the static */
    static void StopAiSound(AI_SOUND sound);
    static void PlayAiSound(AI_SOUND sound);
    using CSealCtrl::PlayAiSound;
    static class CSnd *aiSoundHandle[2];
    static class CSndInstance *aiSoundInstance[2];
    /* the aim's error cone, kept as its tangent (SOCOM 1's aiAccuracyAngle; OVL1 data 0x3DF180, the
     * kit lane's data row), and its setter (SOCOM 1's; OVL2 0x574BA0 -- CCharacterType::Open calls it
     * in both builds) */
    static float aiAccuracyAngle;
    static void SetAiAccuracyAngle(float angle);
    /* the visibility tests' request pool closed and opened (SOCOM 1's statics; OVL2 0x5CC5F0,
     * 0x5CC890 -- ReleaseDatabaseHandles 0x574BD0 and AcquireDatabaseHandles 0x574C30 call them;
     * seal_sensors.cpp, the kit lane's) */
    static void CloseDiPool();
    static void OpenDiPool();
    /* the AI's handles let go and taken (SOCOM 1's statics: the two loops' sounds, the request pool
     * -- SOCOM II's take the callouts' sounds, the difficulty, the squad's radio valves, the tuning,
     * the frame slices, the idle anims' kinds and the groups' ids besides; OVL2 0x574BD0, 0x574C30,
     * seal_ai.cpp) */
    static void ReleaseDatabaseHandles();
    static void AcquireDatabaseHandles();
    /* a spray of fire timed, when the item in hand fires fully automatic (SOCOM 1's; OVL2 0x565AE0) */
    void SetSprayTimer(float t);
    /* the aim turned off its line by the target's error (slot +0x74, SOCOM 1's; OVL2 0x5C9250, the
     * kit lane's seal_kit.cpp defines it) */
    virtual void PerturbAimVector(CPnt3D *v);
    virtual void Initalize();
    /* the brain started over: the states, the goals and the senses (SOCOM 1's; OVL2 0x573130 -- not
     * written: CMission::LoadVehicle, RemoteRespawn and DoRespawn call it as SOCOM 1's do) */
    void InitAiBrain();
    /* a Navy SEAL (SOCOM 1's bit; the accessor ours, as the code tests it materialised) */
    bool IsNavySeal() const { return m_navyseal ? true : false; }
    /* the planner a new route takes (SOCOM 1's; OVL2 0x5627E0, seal_ai.cpp's) and its bias (SOCOM 1's,
     * inline in SOCOM II: seal_ai.cpp's, no copy) */
    CAiPath::PATH_TYPE GetDefaultRouteType();
    inline float GetDefaultRouteBias();
    /* a route planned from the SEAL's cell through a list of cells (SOCOM 1's; OVL2 0x562200) */
    bool SetRoute(const CAiMapLoc *locs, int count);
    /* whether the current state is of a kind (SOCOM 1's AiStateIs, const: the stack's const GetState,
     * 0x5F8330). Built whole in a register with an explicit false arm, as the target does: the first
     * operand a materialised inline bool, the second a `? true : false` of its own -- `&&`, a plain
     * ternary or an if/else let MW reuse the first operand's zero and drop the false arm */
    bool StackNotEmpty() const { return !(m_stateStack.size() == 0); }
    bool AiStateIs(AI_STATE state) const
    {
        return StackNotEmpty() ? (m_stateStack.GetState()->m_state == state ? true : false) : false;
    }
    /* the body's task argument (an inline: expanded ahead of the call it is passed with) */
    inline void *GetSelect();
    /* the state on top of SEAL_AI_PARAMS's stack (SOCOM 1's; OVL2 0x5F5FD0, 0x5F6010, the
     * AI-states lane's seal_aistate.cpp defines them -- which is const by SOCOM 1's order) */
    CAiState *GetAiState();
    const CAiState *GetAiState() const;
};

/* the network's controller: a SEAL another console drives (SOCOM 1's CRemoteCtrl : CSealCtrl;
 * SOCOM II's vtable 0x669590, its destructor and SetAiNetState in seal_ai.cpp's span) */
class CRemoteCtrl : public CSealCtrl {
public:
    /* (0x56F460, seal_ai.cpp's; declared so no other unit emits an implicit one) */
    virtual ~CRemoteCtrl();
    virtual bool PreTick(float dT);
    virtual bool Tick(float dT);
    virtual bool PostMortemPreTick(float dT);
    virtual void OnDeath();
    virtual CEntityCtrl *GetRemoteControl();
    virtual bool AllowCommand(CZSealBody *seal, FT_COMMAND cmd, void *data);
    virtual bool IsPlayer() const;
    virtual bool IsRemote() const;
    virtual bool PostTick(float dT);
    virtual void OnStunDamage(STUN_TYPE type, float amount, const CPnt3D *from);
    virtual void OnMissionStart();
    virtual void OnMissionEnd();
    virtual void SetAiNetState(unsigned int state);
    virtual void OnCommand(CZSealBody *seal, FT_COMMAND cmd, void *data, bool b);
    virtual AI_STATE GetJumpFlag() const;

    /* (inline: CreateRemoteSeal's new expands it after CSealCtrl's, the table then the fades) */
    CRemoteCtrl() : m_fade_min(0.001f), m_fade_time(2.0f) {}

    float m_fade_min;   // 0x250: the corpse's fade floor (0.001; the names ours)
    float m_fade_time;   // 0x254: the corpse's fade time (2.0)
    unsigned char _258[0x260 - 0x258];   // (0x260 bytes: CreateRemoteSeal news 608)
};

/* the animated body, CBody (0x90 bytes; its constructor OVL1 0x28EDD0), is zbody.h's (the body
 * lane's) */
#include "../zbody/zbody.h"

/* a piece of gear a character wears (SOCOM 1's DWARF: CCharacterGear, 0x34 bytes; the made ones in
 * zchar.h's CCharacterType::m_gear_list) */
class CCharacterGear {
public:
    /* the gear's model instanced under a name (SOCOM 1's; OVL2 0x53C940, char_type.cpp's) */
    zdb::CNode *CreateInstance(const char *name) const;

    int m_ref_count;   // 0x0
    int m_partid;   // 0x4
    unsigned int m_gearid;   // 0x8
    char *m_name;   // 0xc
    char *m_model_file;   // 0x10
    char *m_model_name;   // 0x14
    void *m_model;   // 0x18
    CPnt3D m_offset;   // 0x1c
    CPnt3D m_rotation;   // 0x28
};

/* a body part made of a gear: the five items worn (CZSealBody's m_item_gear) and the extra parts on
 * CObjAnims' list (SOCOM 1's AddItem news one of 8 bytes, the part and the gear). SOCOM II's
 * constructor and destructor zero both words (OVL2 0x5550B0 and 0x553D60, seal.cpp's copies: the
 * SEAL's constructor and destructor build and tear down the five through __construct_array and
 * __destroy_arr; ~CObjAnims' delete inlines the destructor) -- the name ours */
class CObjAnim {
public:
    CObjAnim() { m_part = 0; m_gear = 0; }
    CObjAnim(CZBodyPart *part, class CCharacterGear *gear) { m_part = part; m_gear = gear; }
    ~CObjAnim() { m_part = 0; m_gear = 0; }

    CZBodyPart *m_part;   // 0x0
    class CCharacterGear *m_gear;   // 0x4
};

/* the parts the SEAL adds (SOCOM 1's DWARF: CObjAnims, one vector; its GetObjAnim and destructor head
 * seal_cfg.cpp in both builds -- SOCOM II's destructor 0x58AB30, the SEAL's destructor calls it) */
class CObjAnims {
public:
    ~CObjAnims();
    /* a part and its gear added (an inline: SetupGear reads both before the allocation) */
    void Add(CZBodyPart *part, class CCharacterGear *gear) { m_obj_anims.push_back(new CObjAnim(part, gear)); }

    std::vector<CObjAnim *> m_obj_anims;   // 0x0
};

/* the SEAL's adrenaline: its level, 0 to 1 (only what the heartbeat reads; the names ours) */
struct CAdrenaline {
    float m_level;   // 0x0
};

/* how far a SEAL's aim strays, sideways and up, and the random part's scale (SOCOM 1's DWARF,
 * 0xc bytes) */
class CAccuracy {
public:
    float m_factorX;   // 0x0 (SOCOM 1's FactorX)
    float m_factorY;   // 0x4 (FactorY)
    float m_factor;   // 0x8 (Factor)
};

/* the SEAL's stats (SOCOM 1's DWARF: CSealStats, 0x54 bytes of ints; SOCOM II's counters are
 * unsigned shorts, 0x48 bytes: CZSealBody's m_stats at 0x58C ends where m_accuracy starts) -- the
 * counters by halfword offset, as the events step them (seal.cpp) */
class CSealStats {
public:
    unsigned short m_count[0x3c / 2];   // 0x0
    short m_score;   // 0x3c: the MVP score (SOCOM 1's m_sMvpScore, a short there too)
    unsigned short m_count2[3];   // 0x3e
    float m_hit_percent;   // 0x44 (SOCOM 1's 0x28)
};

/* a head's turn toward a direction, eased over a time (SOCOM 1's DWARF: CLook : CSequence, 0x80
 * bytes; SOCOM II's drops a quaternion: m_goal_dir and everything after it 0x10 earlier). SOCOM II's
 * SEAL holds its three looks (SOCOM 1's held pointers): seal_env.cpp's */
class CLook : public CSequence {
public:
    CLook();
    void SetDir(const CPnt3D *dir, float rate);
    void LookTick(float dT);
    /* the look turned toward a direction from the SEAL (SOCOM 1's; OVL2 0x5ADC70, not written yet) */
    bool SetLookDir(CZSealBody *seal, const CPnt3D *dir, float rate);

    CPnt3D m_curr_dir;   // 0x10
    float m_rate;   // 0x1c
    CQuat m_quat0;   // 0x20
    CQuat m_quat1;   // 0x30
    CPnt3D m_goal_dir;   // 0x40 (SOCOM 1's 0x50)
    float m_deltaT;   // 0x4c
    CPnt3D m_cross;   // 0x50
    float m_fracT;   // 0x5c
    unsigned char m_done;   // 0x60
    bool IsDone() const { return m_done; }
    unsigned char _61[0x70 - 0x61];   // (SOCOM II's CLook is 0x70: CBlink's m_org at 0x70)
};

/* a turn in place's timing (SOCOM 1's DWARF: CTurnInPlace : CSequence, 0x10 bytes, nothing of its
 * own) */
class CTurnInPlace : public CSequence {
};

/* the eyelids: a look down and back up (SOCOM 1's DWARF: CBlink : CLook) */
class CBlink : public CLook {
public:
    CBlink();
    void Blink(int org);
    void BlinkTick(float dT);

    int m_org;   // 0x70 (SOCOM 1's 0x80)
    float m_nextT;   // 0x74
};

/* the SEAL (SOCOM 1's DWARF: CZSealBody : CEntity, CBody, 0x1140 bytes; SOCOM II's CBody base at
 * 0x170, after its 0x170-byte CEntity, the SEAL's own members from 0x200 -- the constructor at
 * 0x553EA0 builds the CBody at 0x170 and stores the second table there) -- only what the written
 * functions touch */
class CZSealBody : public CEntity, public CBody {
public:
    /* a SEAL made from a character type with a controller (SOCOM 1's; OVL2 0x5999E0), and the local
     * player's made for a network game (SOCOM 1's; OVL2 0x5442C0) */
    static CZSealBody *Create(const CCharacterType *type, const char *name, CSealCtrl *ctrl);
    /* a SEAL made over in place, of its own character when given none (SOCOM 1's; OVL2 0x599B60 --
     * not written; true always) */
    static bool ReCreate(CZSealBody *seal, const CCharacterType *type);
    /* the SEAL a node belongs to (OVL2 0x59ACD0, seal_create.cpp's) */
    static CZSealBody *GetSeal(zdb::CNode *node);
    /* (OVL2 0x553EA0, not written yet) and the controller fitted after (OVL2 0x599F00) */
    CZSealBody(zdb::CNode *node, CCharacterType *type);
    static void PostCreateSeal(CZSealBody *seal, CSealCtrl *ctrl);
    static void NetworkPlayerCreate();
    /* the life the network reports set: a remote SEAL coming back up from 2 or 3 is marked for
     * its respawn (OVL2 0x544210, new in SOCOM II: IncrementalUpdateRemoteSeal calls it; the name
     * ours) */
    void SetNetLife(unsigned char life);
    /* a player's targets forgotten as seen (OVL2 0x544B60, new in SOCOM II: SetCurZoomState calls
     * it; the name ours) */
    void ResetTargetsSeen();
    /* the view stepped out and in one place, and the view set (SOCOM 1's; OVL2 0x544400, 0x5445B0,
     * 0x5448A0 --
     * SOCOM II's states are SOCOM 1's with one inserted at 2: night vision is 3, the binoculars 4,
     * the weapon's zooms from 5, thirteen in all) */
    ZOOMSTATE DecrZoomState();
    ZOOMSTATE IncrZoomState();
    void SetCurZoomState(ZOOMSTATE zs);
    /* a remote SEAL brought back, and the local SEAL's respawn (SOCOM 1's; OVL2 0x598B90, 0x5994A0,
     * seal_create.cpp's range: Virtual8C calls them) */
    void RemoteRespawn(bool b);
    void DoRespawn();
    /* the SEAL's stats (OVL2 0x545890: the stats events reach a teammate's through it, by member
     * pointer; the name ours) */
    class CSealStats *GetStats();
    /* the charges set down that the SEAL comes near set off (OVL2 0x543930, new in SOCOM II: Tick
     * calls it; the name ours) */
    void CheckProximityExplosives();
    /* the SEAL's own compass marker made, once, of a kind (4 and 5 a friend's, for the player's
     * team; 12 "ABLE", 13 "BRAVO"), named when a name is given; and a friend's marker's four points,
     * locked offline (OVL2 0x5434B0, 0x543680, new in SOCOM II: CRemoteCtrl::SetAiNetState and the
     * AI's 0x573130 call the first; the names ours) */
    CCompassAnchor *CreateCompassAnchor(int type, const char *name);
    void AddFriendAnchorPoints(CCompassAnchor *anchor);
    /* an AI SEAL of the team with bit 1 shows its target on the compass while it holds it or saw it
     * within three seconds, and drops the marker otherwise (OVL2 0x543030, new in SOCOM II: Tick
     * calls it; the name ours) */
    void UpdateHostileAnchor();
    /* thermal vision: the turret the SEAL works (the entity given, or its own at 0xEB8) if it is
     * one; whether the SEAL sees thermally -- the goggles (item 195) when it works nothing, the
     * turret's thermal scope when it works one; whether it works a turret with that scope; the
     * turret's nodes given the value and its scope's look set (OVL2 0x5433C0, 0x5432F0, 0x543200,
     * new in SOCOM II: CameraZoom::HandleNewZoom asks them; the names ours) */
    inline class CTurret *GetTurret(CEntity *e);
    inline class CTurret *GetTurret();
    bool HasThermal(CEntity *e);
    bool TurretHasThermal();
    void UpdateTurretScope(float f);
    /* another console's SEAL made: its client index, entity id, name, vehicle, character type, a
     * word of flags (SOCOM 1's took a byte, the type's number: SOCOM II tests the bits 17 and 28) and
     * its packed weapons (SOCOM 1's; OVL2 0x599690) */
    static CZSealBody *CreateRemoteSeal(int index, int id, const char *name, int vehicle, char *charname,
                                        unsigned int flags, unsigned char *packed);
    /* a remote SEAL unmade: its id cleared everywhere, dead, no team, the squad told (SOCOM 1's; OVL2
     * 0x5998B0) */
    static void Destroy(CZSealBody *seal);
    /* the character type taken and its model instanced, dynamic, with its LODs, in the world, its
     * textures added (SOCOM 1's; OVL2 0x59A7F0) */
    static const CCharacterType *PreCreateSeal(const CCharacterType *type, const char *name, zdb::CNode **node);
    /* every heartbeat loop stopped (SOCOM 1's; StopRumbler calls it with no body) */
    static void StopAllHeartbeatSounds();
    /* the loops let go, and looked up (SOCOM 1's; OVL2 0x543AD0, 0x543B40) */
    static void CloseHeartbeatSounds();
    static void OpenHeartbeatSounds();
    /* the stance a state stands for, or the fallback (SOCOM 1's; the AI-states lane's 0x5FC910) */
    static SEAL_STANCE GetStanceFromState(SEAL_STATE state, SEAL_STANCE fallback);
    /* the state the body stands in; asked with the flag, the top blend's anims say it first (SOCOM
     * 1's GetStance, which returns CBody's state byte; OVL2 0x58A720, seal_body.cpp) */
    SEAL_STATE GetStance(bool flag) const;
    /* a body part made a gear's (SOCOM 1's; OVL2 0x58B040, seal_cfg.cpp) */
    void SetPartAsGear(CZBodyPart *part, class CCharacterGear *gear);
    /* a gear by name, or none (SOCOM 1's; OVL2 0x59A730, seal_create.cpp: `this` unread) */
    CCharacterGear *FindGear(const char *name);
    /* the rifle, pistol and launcher found among the made gears, the launcher's part added (SOCOM 1's;
     * OVL2 0x58B0F0, seal_cfg.cpp) */
    void SetupGear();
    /* a part added to the anims' parts: a bare one by name (OVL2 0x58B630), or a gear's model attached to
     * the part it names (0x58B790; SOCOM 1's two AddItems, after SetupEyes in both builds) */
    bool AddItem(const char *name);
    bool AddItem(class CCharacterGear *gear);
    /* every bone's node given a box DI of the person material, the upper spine's box capped (SOCOM 1's;
     * OVL2 0x58B960, seal_cfg.cpp) */
    void AddDIPolys();
    /* the anim set's speeds set, the prone and ladder ones from their anims' scaled times (SOCOM 1's;
     * OVL2 0x58BCB0, seal_cfg.cpp) */
    void SetMaxMotion();
    /* the anims of a blendable's speed bands mixed into a list by a speed and a weight, how many
     * handed back (SOCOM 1's MixAnims without its flag; OVL2 0x58BDF0, seal_cfg.cpp) */
    int MixAnims(class CActiveAnimList *list, class CBlendable *blendable, float scale, float wt);
    /* a new anim set bound: a carried body let go, the blends cleared, the set swapped in (the old one
     * kept), the speeds and the weapon reset and the type's blend pushed in the state (SOCOM 1's took the
     * set alone; OVL2 0x58C350, seal_cfg.cpp) */
    void BindAnimSet(class AnimSet *set, AnimType type, SEAL_STATE state, bool force);
    /* the carried body let go of (SOCOM 1's; OVL2 0x579220, seal_body.cpp) */
    void DetachBody();
    /* the animation set's limits on the sideways and forward motion (seal_cfg.cpp) */
    void GetMaxXMotion(float &a, float &b);
    void GetMaxZMotion(float &a, float &b);

    /* (0x5533E0, not written yet: declared so no unit emits an implicit one) */
    virtual ~CZSealBody();
    virtual void PostTick(float dT);                                        /* +0x24 */
    virtual void OnMissionStart();                                          /* +0x4C */
    virtual void OnMissionEnd();
    virtual void OnRecycleEntity(CEntity *e);                               /* +0x54 */
    virtual void TeleportTo(const CPnt3D *pos, const CQuat *quat);          /* +0x3C */
    virtual void TeleportTo(const CMatrix *m);
    virtual void OnDeath();                                                 /* +0x58 */
    virtual bool Virtual5C(CZSealBody *seal, CTurret *turret);
    virtual bool Virtual60(CPnt3D *out);
    /* the fire point of a name (OVL2 0x5DFAD0, seal_weapon.cpp's range: SOCOM 1's
     * GetFirepointNode, virtual in SOCOM II -- CEntity's slot +0x64) */
    virtual zdb::CNode *GetFirepointNode(const char *name);
    /* where a named point of the item in hand lies in the world, if the item is worn (SOCOM 1's;
     * OVL2 0x5DFB70, seal_weapon.cpp -- the bool its own) */
    bool GetFirepointPos(CPnt3D &pos, const char *name);
    /* where a node of the item in hand lies, in the world or turned into it (SOCOM 1's; OVL2 0x5DF930,
     * seal_weapon.cpp) */
    bool GetNodePos(const zdb::CNode *node, CPnt3D &pos, bool world);
    virtual bool Virtual68(CPnt3D *out);
    /* the root part and the state set (CBody's virtuals, overridden), with one of the SEAL's own
     * between them: MW lists them after the CBody table, from +0x88, and calls them through the
     * primary pointer (Destroy's state set reads +0x90) */
    virtual CZBodyPart *GetRootPart() const;                                /* +0x88 */
    virtual void Virtual8C();                                               /* +0x8C (0x547350) */
    virtual void BodySetState(int state);                                   /* +0x90 */

    /* whether a player drives it (an inline the code shows: m_control tested, IsPlayer called) */
    bool IsPlayer() { return m_control ? ((CSealCtrl *)m_control)->IsPlayer() : false; }
    /* whether another console drives it (the same shape, IsRemote at +0x34) */
    bool IsRemote() { return m_control ? ((CSealCtrl *)m_control)->IsRemote() : false; }
    /* whether the AI drives it (the same inline shape: CanInterrupt) */
    bool IsAi()
    {
        bool r = false;

        if (m_control)
            r = ((CSealCtrl *)m_control)->IsAi();
        return r;
    }
    /* the ladder's polygons checked, and the check taken only when it should be (SOCOM 1's took three
     * float references; OVL2 0x5B0EB0, 0x5B10B0) */
    bool CheckDILadder();
    bool PostCheckDILadder();
    float SetHealth(float health, enum SEAL_DEATH_TYPE type);
    /* the character the SEAL was made from (SOCOM 1's; OVL2 0x599810) */
    CCharacterType *GetCharacter();
    void ResetAltitude(float y);
    /* the velocity slid along a wall it hit (SOCOM 1's; OVL2 0x5B0DD0) */
    void Slide(zdb::IntersectStruct *hit);
    /* a wounded SEAL's blood dripped on the ground now and then (SOCOM 1's; OVL2 0x5A33B0) */
    void HandleBloodDrip(float dT);
    /* a footfall's sound at a point (SOCOM 1's; OVL2 0x5A39B0) */
    void HandleFootfallSound(CPnt3D *pos);
    /* the blood decals, one picked at random (SOCOM 1's static vector<CZDecalDef>; OVL1 bss
     * 0x44D568) */
    static std::vector<class CZDecalDef> m_blooddriptexhandles;
    /* the database's handles let go (SOCOM 1's static; OVL2 0x5A3BB0) */
    static void UninitDatabaseHandles();
    /* the database's handles taken: the hand signals, the weapon-event callback, the ripple, drop
     * and footfall anims, the sounds (SOCOM 1's statics; OVL2 0x5A49F0, 0x5A4CE0 -- which CMission::
     * Open calls --, and the decal textures 0x5A4CF0) */
    static void ReInitDatabaseHandles();
    /* ReInit's handles let go: the drop anims' words cleared, the footfall anims' buffer freed, the
     * weapon-event callback removed (SOCOM 1's static; OVL2 0x5A3B50 -- UninitDatabaseHandles inlines
     * it, CMission::ReOpen_Unload1 calls it) */
    static void Uninit_ReInitDatabaseHandles();
    static void InitDatabaseHandles();
    static void InitDecalHandles();
    /* a weapon's event handed to every SEAL's controller (SOCOM 1's static callback; OVL2 0x5A4F80) */
    static void HandleWeaponEvent(CZProjectile *proj, unsigned int event, void *data);
    void SetSnakePathUpdate(bool on, unsigned int length);
    /* the follow parameters marked stale; with no followers the saved formation direction too, and
     * the snake path's update stopped unless a player drives the SEAL or its remote console says
     * it runs one (SOCOM 1's; OVL2 0x5FCC40, seal_aifollow.cpp's) */
    void InvalidateFollowParams();
    /* a follower counted on and off (inline: SetFollowMaster steps the master's count so; the names
     * ours) */
    void AddFollower() { m_followers++; }
    /* the SEAL's controller, and its AI (inline: the follow chain's walk calls their copies at its
     * bottom -- OVL2 0x5FCF00, 0x5EA7C0; the names ours) */
    CEntityCtrl *GetControl() { return m_control; }
    /* CEntity's last valid cell (an inline: 0x569AA0 reads it through one -- the name ours) */
    const CAiMapLoc &MapLocValid() const { return m_mapLocValid; }
    class CSealCtrlAi *GetSealCtrlAi() { return ((CSealCtrl *)GetControl())->GetSealCtrlAi(); }
    void RemoveFollower()
    {
        if (m_followers > 0)
            m_followers--;
    }
    void RestrainBuddy(CZSealBody *victim);
    void StabBuddy(CZSealBody *victim);
    bool ExecuteAction_ApplyRestraint(CZSealBody *victim);
    bool ExecuteAction_CQBAttack(CZSealBody *victim);
    /* a pickup taken up: a grenade thrown back, the others each their own way (SOCOM 1's; OVL2
     * 0x547570) */
    class CZAnim *PickupObject(CPickup *pickup);
    class CPickup *CurrentPickupSecondary();
    class CPickup *CurrentPickup();
    bool CompassAnchor_MPBase(CCompassAnchor *anchor);
    bool CompassAnchor_MPBomb(CCompassAnchor *anchor);
    bool ExecuteDelayedAction_Jump();
    bool ExecuteDelayedAction_Climb();
    /* an order given to the SEAL's controller, which may refuse order 17; a given order flagged
     * (SOCOM 1's; OVL2 0x590AC0) */
    void IssueCommand(CZSealBody *seal, FT_COMMAND cmd, void *data, bool b);
    /* a SEAL another console drove taken over by the AI when its console leaves, from the state that
     * console last sent (SOCOM 1's AcquireRemoteAi; OVL2 0x56F210, seal_ai.cpp's: static -- the
     * SEAL is its one argument, zDoClientLeavesCallback passes it in a0 and nothing in a1) */
    static void AcquireRemoteAi(CZSealBody *seal);
    /* an action offered or withdrawn (SOCOM 1's took an unsigned int id) */
    void ActionAvailableMask(unsigned char id, bool on);
    /* the action shown selected, its bit alone (OVL2 0x544C80, seal.cpp; SOCOM 1's took a flag too):
     * an int, narrowed inside (the HUD's round 42: its caller loads the byte with `lb`, which only an
     * int-sized parameter gives) */
    void ActionSelectionMask(int id);
    /* the restraint flag set (an inline: its bool is stored as `(b & 1)`) */
    void SetRestrained(bool on) { m_restrained = on; }
    /* the squad-joined flag set (an inline: `(b & 1) << 6` even for a constant) */
    void SetJoinedUnit(bool on) { m_joined_unit = on; }
    /* the gas and smoke flags set (inlines: a constant true is `daddiu 1 << n`) */
    void SetInGas(bool on) { m_isingas = on; }
    void SetInSmoke(bool on) { m_isinsmoke = on; }
    /* bits 9 and 10 of 0x1060 set (inlines: a constant true is `daddiu 1 << n`, a false `(0 & 1) <<
     * n`; the names ours) */
    void SetBit9(bool on) { m_bit9 = on; }
    void SetBit10(bool on) { m_bit10 = on; }
    /* the moving and turning flags set (inlines: InStateHang clears both with `(0 & 1) << n`) */
    void SetIsMoving(bool on) { m_is_moving = on; }
    void SetIsTurning(bool on) { m_is_turning = on; }
    /* the stance's change taken, once (seal_body.cpp) */
    bool StanceChanged();
    /* the throttles' change per step held to a rate that grows as each nears full, an AI SEAL's
     * left alone (SOCOM 1's; OVL2 0x586C10, seal_body.cpp) */
    bool SmoothXZThrottles(float &x, float &z);
    /* whether the throttles leave the SEAL stopped (SOCOM 1's; OVL2 0x586F00, seal_body.cpp) */
    bool IsStopped(float x, float z, float t);
    /* standing, turned in place (SOCOM 1's took three throttles; OVL2 0x586050, seal_body.cpp:
     * InStateStand returns it) */
    CZBodyAnimBlend *StandTurning(float turn);
    /* standing still: the step's foot from the strafe's weight, the stand blend asked for, the turn's
     * timing and the moving flags cleared (SOCOM 1's took three throttles; OVL2 0x586240, seal_body.cpp:
     * InStateStand returns it) */
    CZBodyAnimBlend *StandStill(float x, float z);
    /* the state set: its blend and the body's state (SOCOM 1's; OVL2 0x581ED0, seal_body.cpp's -- not
     * written yet: 0 or 1 back), and the desired state taken when it may be (SOCOM 1's; OVL2 0x552D60,
     * seal.cpp) */
    bool SetStance(SEAL_STATE state);
    void UpdateStance();
    /* the state of the topmost blend not in state 3, else 0 (SOCOM 1's PeekBaseState; OVL2
     * 0x57DC20, seal_body.cpp's -- CalcAimGoal asks it in state 3) */
    unsigned char PeekBaseState();
    /* a turn's set-up (SOCOM 1's TurnSetup(int), seal_ik.cpp's; OVL2 0x5B8270: SOCOM II only clears
     * the bits at 0x1060 6 and 7 when both feet exist, the int unread) */
    void TurnSetup(int anim);
    /* the weapon's state without its bit 4 (SOCOM 1's; OVL2 0x5E0010, seal_weapon.cpp) */
    WEAPON_STATE GetWeaponState() const;
    /* the weapon's state set (SOCOM 1's; OVL2 0x5DFE30, the kit lane's seal_weapon.cpp range) */
    void SetWeaponState(WEAPON_STATE state);
    /* the weapon state machine stepped, the aim and the kit after it (SOCOM 1's; OVL2 0x5DFC80,
     * seal_weapon.cpp) */
    void WeaponsTick(float dT);
    /* the aim's goal from a point along a direction, and the aim set along a direction -- whether a
     * goal came of it (made the unit -z when asked, should none) -- SOCOM 1's; OVL2 0x5DF600 and
     * 0x5DF8B0, seal_weapon.cpp (the kit lane) */
    bool CalcAimGoal(const CPnt3D *from, const CPnt3D *dir, CPnt3D *goal);
    /* the aim and fire points found once, by name, and their positions taken (SOCOM 1's; OVL2
     * 0x5DF450, seal_weapon.cpp -- the kit lane) */
    void UpdateAimParams();
    /* the aim at a point, with its surface normal, or none (SOCOM 1's; OVL2 0x5DF4E0, seal_weapon.cpp
     * -- the kit lane) */
    bool SetAimPointW(const CPnt3D *aimpt, const CPnt3D *aimnorm, bool keep);
    bool SetAimDirR(const CPnt3D *dir, bool reset);
    /* whether an anim type is the blend on top (SOCOM 1's; OVL2 0x577E40, seal_anim.cpp) */
    bool IsCurrentBlend(AnimType type) const;
    /* the hand signals: each one's anims looked up by name (static: no member read, its caller
     * passes none), and one played -- the anim, or -1 (SOCOM 1's; OVL2 0x600440, 0x600290: the kit
     * lane's seal_handsig.cpp) */
    static void InitHandSignals();
    AnimType DoHandSignal(SEAL_HANDSIGNAL sig);
    /* the stances' blends (SOCOM 1's seal_body.cpp, in its order: BlendedClimb 0x581110 to Stand
     * 0x581DC0 -- Stand_0 gained a float in SOCOM II) */
    CZBodyAnimBlend *BlendedClimb(float h);
    CZBodyAnimBlend *Hang_0(int stance, int n);
    CZBodyAnimBlend *SetStanceIdle(AnimType type);
    CZBodyAnimBlend *Prone_0(int stance);
    CZBodyAnimBlend *Prone();
    CZBodyAnimBlend *Crouch_0(int stance, float t);
    CZBodyAnimBlend *Crouch();
    CZBodyAnimBlend *Stand_0(int stance, int n, float t);
    CZBodyAnimBlend *Stand(int n);
    /* the blends popped down to a count, twelve at most; when that worked and asked, the top blend's
     * control cleared and the SEAL asked to go prone (SOCOM 1's ClearStack, after PopBlend: OVL2
     * 0x58A5E0 -- SOCOM II adds the limit; the stances inline it) */
    void ClearStack(unsigned int count, bool clear);
    /* the blends popped down to a base stance's (anims 1, 18 or 26 on top), twelve at most, and one
     * more when asked; the top blend's control cleared and stance 2 asked; the new top returned
     * (SOCOM 1's ClearToBase: OVL2 0x58A4D0 -- SOCOM II asks IsCurrentBlend and adds the limit) */
    CZBodyAnimBlend *ClearToBase(bool pop);
    /* the top blend turned at a rate from the turn, capped per state (SOCOM 1's TurnControl after
     * ProneControl: OVL2 0x583960 -- SOCOM II's takes the turn alone) */
    CZBodyAnimBlend *TurnControl(float turn);
    /* the ladder and hang states' anims (SOCOM 1's InStateClimbLadder and InStateHang, no floats in
     * SOCOM II; OVL2 0x584240, 0x584390) */
    CZBodyAnimBlend *InStateClimbLadder();
    CZBodyAnimBlend *InStateHang();
    /* the body carried let go (SOCOM 1's DropBody, before OKToStand in both builds: OVL2 0x57EE00) */
    void DropBody();
    /* an explosion's damage: unless another console's SEAL is hit online, its sight from the head
     * checked by a delayed intersection when it hurts here or is weapon 123 (SOCOM 1's took the
     * location by value; SOCOM II's caller, HandleDamage 0x5AB850, passes it by address, and the
     * part and location go unread; OVL2 0x5AC070, seal_env.cpp) */
    void HandleDamageExplosive(CZProjectile *proj, CZBodyPart *part, DAMAGE_LOCATION *where);
    /* a bullet's damage on a part, the location handed back (SOCOM 1's took it by value; OVL2
     * 0x5ABBC0, after HandleDamageExplosive in both builds -- not written) */
    void HandleDamageNonExplosive(CZProjectile *proj, CZBodyPart *part, DAMAGE_LOCATION *where);
    /* a projectile's damage: the part its last hit names, explosive or not, the health updated and a
     * death credited (SOCOM 1's; OVL2 0x5AB850) */
    void HandleDamage(CZProjectile *proj);
    /* the last weapon hit forgotten and the blend stack's top taken (SOCOM 1's; OVL2 0x5AC660) */
    void HandleWepHit();
    /* a weapon's hit, the damage taken (SOCOM 1's; OVL2 0x5AC670 -- CSealEx::OnWeaponHit tail-calls
     * it) */
    int OnWeaponHit(zdb::IntersectStruct *hit, CZProjectile *proj);
    /* whether team-mates' hits count (SOCOM 1's m_allow_friendly_fire, set from the mission's
     * mp_friendly_fire valve; OVL1 bss 0x44CDB8) */
    static bool m_allow_friendly_fire;
    /* a landing's damage by its speed, the severity returned (SOCOM 1's; OVL2 0x5AC1F0) */
    LANDING_TYPE HandleFallingDamage(float speed);
    /* the health taken from the six locations' values: dead when the head's or the body's runs out
     * (a bullet in the head a head shot), else their weighted average (SOCOM 1's; OVL2 0x5A54D0; the
     * six values' setter 0x5A5790 calls it too) */
    void UpdateHealth(DAMAGE_LOCATION where, SEAL_DEATH_TYPE type);
    /* a hit's damage anim and a death anim for a location, a flag handed back (SOCOM 1's, in its
     * order; OVL2 0x5A0270, 0x5A0700 -- not written) */
    AnimType FindRandomDamageAnim(DAMAGE_LOCATION where, bool *b);
    AnimType FindRandomDeathAnim(DAMAGE_LOCATION where, bool *b);
    /* a weapon hit's anim played in a state, a death anim found when none is given (SOCOM 1's,
     * without its flag; OVL2 0x5A2690 -- not written) */
    void WeaponHitAnim(AnimType anim, SEAL_STATE state);
    /* a stun taken: the kind's highest kept, the controller told (SOCOM 1's; OVL2 0x5A53D0) */
    void HandleStunDamage(STUN_TYPE type, float amount, const CPnt3D *from);
    /* the stuns run down by the character's recovery (SOCOM 1's; OVL2 0x5A50A0), each kind's
     * recovery as it reaches zero (SOCOM 1's; SOCOM II inlines it, seal_env.cpp) and the stunned
     * bit set (an inline: its bool is not folded, `andi zero,1`; the name ours) */
    void TickStun(float dT);
    inline void HandleStunRecovery(STUN_TYPE type);
    void SetAmStunned(bool on) { m_am_stunned = on; }
    /* the HUD's flags: the hit set, and each taken once (seal_env.cpp, SOCOM 1's) */
    void SetHudHitFlag();
    bool HudUpdateViewCone();
    bool GetHudHitFlag();
    /* the terrain a SEAL can cross: an int, the callers make it a bool (seal_env.cpp) */
    int CanTraverse(CDIType type);
    bool CanClimbWall();
    /* a ladder's polygons crossed (SOCOM 1's, a tail call: OVL2 0x59DA60) and the bomb's defusal
     * handled (SOCOM 1's, empty in both builds: OVL2 0x59DA70) */
    int CanClimbLadder();
    void HandleBombDefuse();
    void AttemptBombDefuse();
    /* the action under the reticule, or near it (SOCOM 1's GetReticuleAction, by its order after the
     * pickup tests 0x5A9C70-0x5AA110 and its call of CNodeAction::GetAction 0x5AA470; OVL2 0x5AA240) */
    class CNodeAction *GetReticuleAction(bool b);
    /* the MP bomb picked up (SOCOM 1's; OVL2 0x59DEF0) */
    void HandlePickupMPBomb(CPickup *pickup);
    /* a console's request for one of the SEAL's pickups answered: the ammunition taken and the
     * weapon checked, the answer sent and handed to the asker's SEAL (SOCOM 1's; OVL2 0x59E830 --
     * fts_remote.cpp's message handler calls it -- with its pieces 0x59E580 and 0x59DFB0, seal_env.cpp:
     * SOCOM II passes the pickup's id and the asker whole) */
    void CanPickupNetItem(unsigned short count, int id, unsigned short *items, int client);
    void CheckForAmmoPickups(int count, class CPickupAmmo *ammo, unsigned short *items, union AckUnion *ack);
    void HandlePickupAck(int count, int id, union AckUnion *ack);
    /* another console's dropped weapon made here as a network pickup (SOCOM 1's; OVL2 0x59EC60 --
     * WeaponPUPacketHandler calls it with the packet's item, id and place) */
    void CreateNetWeaponPickup(unsigned short item, int netid, CPnt3D *pos, CQuat *rot);
    /* another console's dropped ammunition made here (SOCOM 1's; OVL2 0x59E9E0) and this SEAL's own
     * dropped, all of it (-1) or so many rounds (SOCOM 1's DropAmmo; OVL2 0x59FD50 -- PickupItem and
     * 0x5A0040 call it) */
    void CreateNetAmmoPickup(unsigned short count, unsigned short *items, unsigned short *rounds, int id, CPnt3D *pos);
    void DropAmmo(int rounds);
    /* a kill credited to the killer, with whether the weapon was one of five (SOCOM 1's, the flag
     * new; OVL2 0x59EE20 -- not written) */
    void HandleKiller(int killer, bool b);
    /* a pickup taken (SOCOM 1's; OVL2 0x59F0C0 -- not written) */
    void PickupItem(CPickup *pickup);
    /* the MP bomb made a pickup at a point (SOCOM 1's; OVL2 0x59DAB0, not written yet) and every
     * item dropped (SOCOM 1's; OVL2 0x59DDB0) */
    static class CPickup *CreateMPBombPickup(const CPnt3D *pos, bool b);
    void DropAllItems();
    /* online, a hit on the SEAL reported to the network and the stats (SOCOM 1's, a projectile
     * first; OVL2 0x5A1FF0) */
    void ReportWeaponHitDamage(CZProjectile *proj, int shooter_id, int victim_id, DAMAGE_LOCATION where,
                               float damage, float pierce, bool hit, unsigned char weapon);
    /* the regions entered: each new one's OnEnterRegion anim run (SOCOM 1's; OVL2 0x5A2F90) */
    void CheckForRegionTrigger(unsigned int regions);
    /* the ground under a point found, the SEAL's own node off for it (SOCOM 1's; OVL2 0x5B4F30,
     * seal_env.cpp), and the hits handled (SOCOM 1's SealProcessAltitude; OVL2 0x5B5D40) */
    void SealGetAltitude(CPnt3D *pos, float f);
    /* the looks (SOCOM 1's, seal_env.cpp's in its order): the lids and the eyes turned to their looks'
     * directions, the head's direction, the head and the three parts above it turned by the look
     * anims (OVL2 0x5AD400, 0x5AD4D0, 0x5AD5A0, 0x5AD5B0); a look's goal off the current direction
     * within limits (OVL2 0x5AE040, not written yet); the eyes' look and the head's set, each saying
     * whether it was (OVL2 0x5AE4B0, 0x5AE640); the look anims built (static in SOCOM II: `this`
     * unread; OVL2 0x5AE6A0); whether the head may turn (OVL2 0x587B40, seal_body.cpp's) */
    void ApplyBlinkAnim();
    void ApplyEyeLookAnim();
    const CPnt3D *GetLookDir() const;
    void ApplyLookAnim(float yaw, float pitch);
    /* (whether a goal came of it: CalcAimGoal, 0x5DF600, keeps the bool -- the kit lane, round 14) */
    bool CalcGoalDir(const CPnt3D *cur, const CPnt3D *dir, CPnt3D *goal, float up, float down, float left,
                     float right);
    bool SetEyeLookDir(float pitch, float yaw, const CPnt3D *dir, float rate);
    bool SetLookDir(const CPnt3D *dir, float rate);
    static void InitLookAnims();
    bool CanHeadLook();
    /* whether the SEAL may pitch, be interrupted, turn (SOCOM 1's, around CanHeadLook in both builds;
     * OVL2 0x587A30 and 0x587C20 not written yet, 0x587E00), and how fast it moves (SOCOM 1's; OVL2
     * 0x58A820) -- seal_body.cpp's */
    bool CanPitch();
    bool CanInterrupt();
    bool CanTurn();
    int GetSpeedState();
    /* the head turned forward by a pitch, and the look's direction in the world (SOCOM 1's
     * RotateHead, its second float gone, and GetLookDirW; OVL2 0x5AF470, 0x5AF500) */
    void RotateHead(float pitch);
    CPnt3D *GetLookDirW(CPnt3D *out) const;
    /* the head's place and its look's direction in the world, each when asked (SOCOM 1's; OVL2
     * 0x5A3070) */
    void GetHeadParams(CPnt3D *pos, CPnt3D *dir);
    /* the aim along the item in hand, and the death a CQB attack gives its victim (SOCOM 1's; OVL2
     * 0x5A8790, 0x5A9420) */
    void AimWeapon();
    AnimType FindDeathAnimTypeCQB(CZSealBody *victim);
    /* whether the top blend is a throw's, whether the SEAL's hands are free for its item (SOCOM 1's
     * CanFire, two flags added; OVL2 0x5A7D10, 0x5A7DE0 -- not written yet) and whether it may change
     * what it holds (SOCOM 1's; OVL2 0x5A8CB0) */
    bool IsThrowing(bool started);
    bool CanFire(bool a, bool b);
    /* a weapon's node made and parented: the rifle's to the right hand when it is the item in hand,
     * else the back, the pistol's to the hand or the hips, a throwable's to the hand -- the part
     * kept as the item's and whether one was made (SOCOM 1's AddWeapon(SEAL_ITEM): zweapon.h's
     * word-sized SEAL_ITEM stand-in takes that name, so the byte enum here; OVL2 0x553290) */
    bool AddWeapon(SEAL_ITEM_BYTE item);
    /* the rifle, or the pistol, taken into the hand: the kit's own reparenting, the weapon into the
     * right hand (the pistol only when asked), the other worn (the pistol holstered only when asked),
     * the item in hand set (SOCOM 1's; OVL2 0x5A70F0, 0x5A7260) */
    void ChangeToRifleMode(bool holster);
    void ChangeToPistolMode(bool hand);
    /* whether a weapon change, or a reload, is under way (SOCOM 1's; OVL2 0x5A7980, and 0x5A7AB0 not
     * written yet) */
    bool IsChangingWeapons(bool kit);
    bool IsReloading();
    bool CanChangeItem();
    /* the SEAL's own AI event made where it stands (SOCOM 1's; OVL2 0x550E50, seal.cpp) */
    void CreateAiEvent();
    /* the SEAL's adrenaline, its level first (SOCOM 1's; OVL2 0x5781B0, seal_anim.cpp's last) */
    struct CAdrenaline *Adrenaline();
    /* the adrenaline raised, to one at most (SOCOM 1's; OVL2 0x578150, seal_anim.cpp's -- the kit lane's
     * seal_kit.cpp reaches it by its mangled name) */
    void AdrenalineIncr(float amount);
    /* the heartbeat's loop kept at the adrenaline's pace (SOCOM 1's; OVL2 0x5437D0, seal.cpp), and
     * its statics: the three loops (OVL1 bss 0x44CE00), the one playing and its index (SOCOM 1's
     * m_heartbeat_sndisnt and m_heartbeat_curindex, OVL1 bss 0x44CE10, 0x44CE18) */
    void TickHeartbeat();
    static class CSnd *m_heartbeat_sound[3];
    static class CSndInstance *m_heartbeat_sndisnt;
    static int m_heartbeat_curindex;

    ZOOMSTATE m_zoomstate;   // 0x200 (SOCOM 1's 0x160: DecrZoomState reads it, lb)
    /* the view before, the change flagged, the field of view (SOCOM 1's DWARF names, its 0x161-0x167;
     * SetCurZoomState sets them) */
    ZOOMSTATE m_zoomstate_prev;   // 0x201
    unsigned char m_zoomstate_echo;   // 0x202
    unsigned char _203;
    float m_zoom_range;   // 0x204
    unsigned char _208[0x20c - 0x208];
    /* the material underfoot and the feet's step flags (SOCOM 1's 0x168-0x16D; HandleSealStep and
     * HandleFootfallSound read them) */
    int m_onMaterial;   // 0x20c
    unsigned char m_didrstep;   // 0x210
    unsigned char m_didlstep;   // 0x211
    unsigned char _212[0x214 - 0x212];
    float m_blooddriptimer;   // 0x214 (SOCOM 1's 0x170)
    /* the SEAL whose node 0x3DC holds, kept while bit 0 of 0x21C is set (0x5AA620, the radio menu's
     * toggle; the names ours) */
    CZSealBody *m_218;   // 0x218
    unsigned char m_21c_bit0 : 1;   // 0x21c
    unsigned char m_altitude_characters : 1;   // bit 1: SealGetAltitude hits characters too (SOCOM 1's
                                               // flag at 0xde8 bit 0; the name ours)
    unsigned char m_21c_bit2 : 1;   // bit 2: a crouch 0x58A720 reports as a stand (the name ours)
    unsigned char m_fading : 1;   // bit 3: the node's opacity heads for m_fade_target (0x552780)
    unsigned char _21c_bits : 4;
    unsigned char _21d[0x22c - 0x21d];
    int m_22c;   // 0x22c: a time on the network's clock 0x5A8850 waits for (the names ours)
    /* a fade of the node's opacity: where to, how fast (its sign toward the target) and the priority
     * of whoever asked (0x552780 refuses a lower one; the names ours) */
    float m_fade_target;   // 0x230
    float m_fade_rate;   // 0x234
    unsigned char m_fade_priority;   // 0x238
    unsigned char _239[0x240 - 0x239];
    float m_240;   // 0x240: PostCheckDILadder checks the ladder only from 0.03 up (the name ours)
    float m_244;   // 0x244: InStateHang's second throttle with 0x240 (the name ours)
    /* the side and move throttles as last eased (SOCOM 1's at 0x194/0x198: SmoothXZThrottles eases
     * toward them; the names ours) */
    float m_prevx;   // 0x248
    float m_prevz;   // 0x24c
    unsigned char _250[0x2e0 - 0x250];
    float m_2e0;   // 0x2e0: the time step the overlay ticks by (0x579EF0; the names ours)
    float m_alt;   // 0x2e4 (SOCOM 1's 0xe98)
    CZBodyPart *m_root;   // 0x2e8 (SOCOM 1's 0x26c)
    /* the body's parts in SOCOM 1's order, 0x7c on (SOCOM 1's ParentPistolToRHandCallback reparents
     * m_pistol, 0x2d8 there, to m_rhand, 0x284; OVL2 0x5A7730 does the same with 0x354 and 0x300) */
    CZBodyPart *m_aimnodes;   // 0x2ec (SOCOM 1's 0x270)
    CZBodyPart *m_lfoot;   // 0x2f0 (SOCOM 1's 0x274: TurnSetup tests both feet)
    CZBodyPart *m_rfoot;   // 0x2f4 (SOCOM 1's 0x278)
    CZBodyPart *m_lhand;   // 0x2f8 (SOCOM 1's 0x27c)
    CZBodyPart *m_spinelo;   // 0x2fc (SOCOM 1's 0x280: the rifle's place on the back)
    CZBodyPart *m_rhand;   // 0x300 (SOCOM 1's 0x284)
    CZBodyPart *m_hips;   // 0x304 (SOCOM 1's 0x288)
    CZBodyPart *m_head;   // 0x308 (SOCOM 1's 0x28c, m_root + 0x20 in both: 0x57F150's ray starts above it)
    CZBodyPart *m_neck;   // 0x30c (SOCOM 1's 0x290)
    CZBodyPart *m_spinehi;   // 0x310 (SOCOM 1's 0x294: AddDIPolys caps its box)
    CZBodyPart *m_lthigh;   // 0x314 (SOCOM 1's 0x298)
    CZBodyPart *m_rthigh;   // 0x318 (SOCOM 1's 0x29c: the pistol's holster)
    /* (SOCOM 1's DWARF names on, 0x7c on: 0x5765F0 saves and 0x576700 eases twenty of them) */
    CZBodyPart *m_rcalf;   // 0x31c (SOCOM 1's 0x2a0)
    CZBodyPart *m_rbicep;   // 0x320 (SOCOM 1's 0x2a4)
    CZBodyPart *m_rforearm;   // 0x324 (SOCOM 1's 0x2a8)
    CZBodyPart *m_lbicep;   // 0x328 (SOCOM 1's 0x2ac)
    CZBodyPart *m_lforearm;   // 0x32c (SOCOM 1's 0x2b0)
    CZBodyPart *m_lscap;   // 0x330 (SOCOM 1's 0x2b4)
    CZBodyPart *m_rscap;   // 0x334 (SOCOM 1's 0x2b8)
    CZBodyPart *m_lshoulder_wgt;   // 0x338 (SOCOM 1's 0x2bc)
    CZBodyPart *m_rshoulder_wgt;   // 0x33c (SOCOM 1's 0x2c0)
    CZBodyPart *m_lcalf;   // 0x340 (SOCOM 1's 0x2c4)
    CZBodyPart *m_ltoe;   // 0x344 (SOCOM 1's 0x2c8)
    CZBodyPart *m_rtoe;   // 0x348 (SOCOM 1's 0x2cc)
    CZBodyPart *m_weapon;   // 0x34c (SOCOM 1's 0x2d0)
    CZBodyPart *m_rifle;   // 0x350 (SOCOM 1's 0x2d4)
    CZBodyPart *m_pistol;   // 0x354 (SOCOM 1's 0x2d8)
    /* the eyes and their lids (SOCOM 1's 0x2e0-0x2ec: SOCOM II drops m_grenade before them, so
     * 0x78 on; the looks' Apply* turn them) */
    CZBodyPart *m_reyeball;   // 0x358
    CZBodyPart *m_leyeball;   // 0x35c
    CZBodyPart *m_reyelid;   // 0x360
    CZBodyPart *m_leyelid;   // 0x364
    /* the nodes the SEAL made for itself, freed with it (SOCOM 1's m_deallocList, 0x2f0: AddWeapon
     * pushes each weapon's node) */
    std::vector<zdb::CNode *> m_deallocList;   // 0x368
    SEAL_STATE m_desiredState;   // 0x374 (SOCOM 1's 0x304; the HUD's pose bitmap reads it)
    SEAL_STANCE m_desiredStance;   // 0x375: the stance the AI asks for (0x5631B0; the name ours)
    unsigned char _376[0x37c - 0x376];
    CObjAnims m_obj_anims;   // 0x37c (SOCOM 1's 0x30c)
    /* the items worn as gear, by SOCOM 1's SEAL_ITEM (1 the rifle, 2 the pistol, 4 the launcher;
     * AimWeapon aims along 1, 2 and 4): the part SetPartAsGear makes of each and the gear (SOCOM 1's
     * m_rifle_as_gear and m_pistol_as_gear were the gear alone; the names ours) */
    CObjAnim m_item_gear[5];   // 0x388 (the rifle's 0x390, the pistol's 0x398)
    void *m_select;   // 0x3b0: a TASK_SELECT (SOCOM 1's 0x318), the AI's tasks' argument
    unsigned char _3b4[0x3c0 - 0x3b4];
    /* the hit under the reticule (SOCOM 1's m_last_reticule, 0x340 -- the name by SOCOM 1's
     * reticule pair, which SOCOM II moved ahead of the altitude): the weapon pickup's test 0x5A9C70
     * reads its point and material, the radio toggle 0x5AA620 takes the SEAL of its node (0x3dc) */
    zdb::IntersectStruct m_last_reticule;   // 0x3c0
    unsigned char _3e0[0x400 - 0x3e0];
    zdb::IntersectStruct m_last_altitude;   // 0x400 (SOCOM 1's 0x320)
    unsigned char _420[0x428 - 0x420];
    CAiEvent *m_aiEvent;   // 0x428: the SEAL's own AI event (SOCOM 1's 0x380)
    unsigned char _42c[0x434 - 0x42c];
    unsigned char m_snakePath[4];   // 0x434: a CAiPath (SOCOM 1's 0x384)
    unsigned char _438[0x514 - 0x438];
    /* (SOCOM 1's m_follow_params_are_current and m_saved_formation_dir_is_valid, 0x458/0x459:
     * InvalidateFollowParams clears them -- the kit lane, round 19) */
    unsigned char m_follow_params_are_current;   // 0x514
    unsigned char m_saved_formation_dir_is_valid;   // 0x515
    unsigned char _516[0x520 - 0x516];
    int m_followers;   // 0x520: how many follow this SEAL (the name ours)
    unsigned char _524[0x528 - 0x524];
    class AnimSet *m_animset;   // 0x528 (SOCOM 1's 0x45c)
    class AnimSet *m_default_animset;   // 0x52c (SOCOM 1's 0x460: BindAnimSet keeps the set it replaces)
    unsigned char _530[0x534 - 0x530];
    AnimType m_curblend;   // 0x534: the anim type on top of the blends (SOCOM 1's short at 0x464)
    unsigned char _536[0x53c - 0x536];
    /* the zoom's anim, "zoom_control", while a scope or the binoculars are up (new in SOCOM II:
     * SetCurZoomState starts and releases it; the name ours) */
    class CZAnim *m_zoom_anim;   // 0x53c
    class CCharacterType *m_charType;   // 0x540 (SOCOM 1's 0x468: TickStun reads its recovery factor)
    unsigned char _544[0x58c - 0x544];
    /* (SOCOM 1's m_stats, 0x4c0, right before m_accuracy as here: the stats' events step its
     * counters -- the CQB attack two of them -- and 0x545890 returns its address) */
    CSealStats m_stats;   // 0x58c
    CAccuracy m_accuracy;   // 0x5d4 (SOCOM 1's 0x514: PerturbAimVector reads it)
    CZKit m_kit;   // 0x5e0 (SOCOM 1's 0x520)
    unsigned char _5e4[0xeac - 0x5e4];
    /* the body carried (SOCOM 1's m_carry, 0xda8 -- right before m_adrenaline in both builds;
     * 0x5A88C0 refuses while one is) */
    CZSealBody *m_carry;   // 0xeac
    struct CAdrenaline *m_adrenaline;   // 0xeb0 (SOCOM 1's 0xdac: Adrenaline returns it)
    unsigned char _eb4[0xeb8 - 0xeb4];
    /* the entity the SEAL works -- a turret when its type is 3 (0x543200-0x5433C0); 0x5A8850 tests
     * it first (the name ours) */
    CEntity *m_eb8;   // 0xeb8
    unsigned char _ebc[0xec0 - 0xebc];
    CCompassAnchor *m_mpbomb_anchor;   // 0xec0 (SOCOM 1's 0xdb4)
    CCompassAnchor *m_mpbase_anchor;   // 0xec4 (SOCOM 1's 0xdb8)
    /* the SEAL's own marker on the compass ("FRIENDLY", "ABLE", "BRAVO"; 0x5434B0 makes it once --
     * new in SOCOM II, the name ours) */
    CCompassAnchor *m_compass_anchor;   // 0xec8
    /* the AI's hostile on the compass ("HOSTILE ACQUIRED": 0x543030 makes, moves and deletes it --
     * new in SOCOM II, the name ours) */
    CCompassAnchor *m_hostile_anchor;   // 0xecc
    unsigned int m_action_available;   // 0xed0: a bit per action (SOCOM 1's ActionAvailableMask word, 0xdc0)
    unsigned int m_action_selection;   // 0xed4: the selected action's bit (SOCOM 1's m_action_selection_mask, 0xdc4)
    unsigned char _ed8[0xee0 - 0xed8];
    float m_wep_upT;   // 0xee0 (SOCOM 1's 0xdd0)
    unsigned char _ee4[0xf44 - 0xee4];
    /* a motion the position code keeps (ComputeNextPosition and PostPreTick read and write it; the
     * collision tick 0x54F570 stops checking once its length is all but zero -- the name ours) */
    CPnt3D m_f44;   // 0xf44
    unsigned char _f50[0xf74 - 0xf50];
    /* the weapon's state, bit 4 a flag GetWeaponState masks off (SOCOM 1's m_weapon_state, 0xe54) */
    unsigned int m_weapon_state;   // 0xf74
    /* an item the anim choice falls back on with no base blend (0x58C820 copies it with lb, tests it
     * with lbu: a byte enum; new in SOCOM II, the name ours) */
    SEAL_ITEM_BYTE m_lastitem;   // 0xf78
    /* the item in hand (SOCOM 1's m_item, a SEAL_ITEM byte at 0xe58: 1 the rifle, 2 the pistol, 3 a
     * throwable; zweapon.h's SEAL_ITEM is a word-sized stand-in, so a byte here) */
    unsigned char m_item;   // 0xf79
    /* the SEAL's life as the network reports it: 2 and 3 down, 1 up again (0x544210 sets it;
     * the names ours) */
    unsigned char m_netlife;   // 0xf7a
    unsigned char _f7b[0xf88 - 0xf7b];
    float m_waterdepth;   // 0xf88 (SOCOM 1's 0xe70, by its use: in water a SEAL crouches only up to 8.5
                          // deep, 0x5857E0, and goes prone only up to 2, Prone)
    unsigned char _f8c[0xfb0 - 0xf8c];
    SEAL_DEATH_TYPE m_deathType;   // 0xfb0 (SOCOM 1's 0xea4)
    unsigned char _fb1[0xfb4 - 0xfb1];
    float m_time_of_death;   // 0xfb4 (SOCOM 1's 0xea8)
    unsigned int m_lastshooter_id;   // 0xfb8: the last hit's shooter (OnWeaponHit keeps it; the name ours)
    unsigned int m_lastkiller_id;   // 0xfbc: the killer's entity id (SOCOM 1's 0xeb4)
    int m_fc0;   // 0xfc0: the death's kind (0x547A90 sets it -- 253 a fall's -- before 0x547860 reads it; the name ours)
    int m_fc4;   // 0xfc4: the killer's id HandleKiller keeps (the name ours)
    int m_ClientIndex;   // 0xfc8 (SOCOM 1's 0xeb8: CreateRemoteSeal stores its first argument in both)
    unsigned char _fcc;
    /* the spectator's next and previous SEAL asked for (new in SOCOM II: Virtual8C walks to them;
     * the names ours) */
    unsigned char m_spectate_next;   // 0xfcd
    unsigned char m_spectate_prev;   // 0xfce
    unsigned char m_needs_respawn;   // 0xfcf (SOCOM 1's 0xec5, the byte before m_stunned)
    /* SOCOM 1's m_stunned, m_full_update_time and m_movespeedmod (0xec6, 0xec8, 0xecc there) in the
     * same order: HandleStunDamage sets the byte, CreateRemoteSeal zeroes the time and sets the
     * modifier to 1 */
    unsigned char m_stunned;   // 0xfd0
    unsigned char _fd1[0xfd4 - 0xfd1];
    unsigned int m_full_update_time;   // 0xfd4
    float m_movespeedmod;   // 0xfd8
    unsigned char _fdc[0xfe0 - 0xfdc];
    float m_stunval[7];   // 0xfe0 (SOCOM 1's 0xedc: HandleStunDamage keeps each kind's highest)
    float m_ffc[6];   // 0xffc: six values run down at the rates below by 0x5AC620 (new; the names ours)
    float m_1014[6];   // 0x1014
    unsigned char _102c[0x1044 - 0x102c];
    float m_health;   // 0x1044 (SOCOM 1's 0xea0)
    /* the last explosive whose damage was taken (SOCOM 1's DWARF m_lastprojectile, 0xf44 right
     * before m_tempprojectile as here: HandleDamage keeps a detonated one) */
    CZProjectile *m_lastprojectile;   // 0x1048
    /* the explosive whose sight from the head is being checked, the check's handle and its
     * intersection (SOCOM 1's DWARF: m_tempprojectile, m_projectileintersecthandle, m_projectiledi,
     * 0xf48-0xf50, right before m_lastregion as here: HandleDamageExplosive sets them) */
    CZProjectile *m_tempprojectile;   // 0x104c
    int m_projectileintersecthandle;   // 0x1050
    zdb::DiIntersect *m_projectiledi;   // 0x1054
    unsigned int m_lastregion;   // 0x1058 (SOCOM 1's 0xf54)
    unsigned int m_HudUpdateViewCone : 1;   // 0x105c (SOCOM 1's 0xf58, the same bits)
    unsigned int m_hudwashit : 1;
    unsigned int m_aim_point_valid : 1;
    unsigned int m_invincible : 1;
    unsigned int m_invisible : 1;
    unsigned int m_infiniteammo : 1;
    unsigned int m_doPostTick : 1;
    unsigned int m_restrainable : 1;
    unsigned int m_restrained : 1;
    unsigned int m_order_flag : 1;
    unsigned int m_limp : 1;
    unsigned int m_isinsmoke : 1;
    unsigned int m_isingas : 1;
    unsigned int m_respawning : 1;
    unsigned int m_joined_unit : 1;
    unsigned int m_send_detail : 1;
    unsigned int m_snake_path : 1;
    unsigned int m_action_climb : 1;
    unsigned int m_action_jump : 1;
    unsigned int m_is_moving : 1;
    unsigned int m_is_turning : 1;
    unsigned int m_airborne : 1;
    unsigned int m_jumped : 1;
    unsigned int m_inwater : 1;
    unsigned int m_stancechanged : 1;
    unsigned int m_have_aim_norm : 1;
    unsigned int m_was_turning : 1;
    unsigned int m_aimpt_attainable : 1;
    unsigned int m_firept_attainable : 1;
    unsigned int m_used_vel_y : 1;
    unsigned int m_echo_pickup : 1;
    unsigned int m_animate_me : 1;
    unsigned int m_do_drop_weapons : 1;   // 0x1060
    unsigned int m_fix_skel_y : 1;
    unsigned int m_inside : 1;   // bit 2 (the states lane's reading)
    unsigned int m_prev_have_aim_norm : 1;
    unsigned int m_am_stunned : 1;
    unsigned int m_iszooming : 1;   // bit 5 (camera.h's reading)
    /* bits 6 and 7: cleared together by TurnSetup when both feet exist (OVL2 0x5B8270, the kit
     * lane's seal_ik.cpp: two single-bit fields written one at a time, lbu/sb -- the names ours) */
    unsigned int m_bit6 : 1;
    unsigned int m_bit7 : 1;
    unsigned int m_unused_bits : 1;
    unsigned int m_bit9 : 1;   // bit 9: a remote SEAL with it set is skipped by 0x57DFE0 (the name ours)
    unsigned int m_bit10 : 1;   // bit 10: cleared with bit 9 set by 0x57DF20/0x57E050 (the name ours)
    unsigned int m_1060_bit11 : 1;
    unsigned int m_1060_bit12 : 1;
    unsigned int m_1060_bit13 : 1;   // bit 13: online, 0x589900 refuses the SEAL with it set (the name ours)
    unsigned int m_unused_bits2 : 2;
    /* the ladder the SEAL is on, an index into the mission's list (new in SOCOM II: read with lhu
     * and re-masked for an int argument, as a 16-bit field is; the name ours) */
    unsigned int m_ladder : 16;   // 0x1062
    unsigned char _1064[0x1070 - 0x1064];
    CQuat m_fwdlook;   // 0x1070 (SOCOM 1's 0xf60: RotateHead sets it, the app camera's transitions call that)
    unsigned char _1080[0x1090 - 0x1080];
    /* a wall's hit the SEAL holds to (new in SOCOM II: held while its node is set -- 0x5B3890 drops
     * it once the SEAL strays, faces off it or the ledge is out of reach; the name ours) */
    zdb::IntersectStruct m_1090;   // 0x1090
    unsigned char _10b0[0x10d0 - 0x10b0];
    float m_10d0;   // 0x10d0: set to 0.4 when the overlay plays out (0x579EF0)
    unsigned char _10d4[0x10e0 - 0x10d4];
    /* the last weapon hit (SOCOM 1's m_wephit, an IntersectStruct * at 0xf84; SOCOM II holds it:
     * HandleDamage takes this + 0x10E0 when its node is set, HandleWepHit clears the node) */
    zdb::IntersectStruct m_wephit;   // 0x10e0
    unsigned char _1100[0x1190 - 0x1100];
    /* the head's look, the eyes' and the lids' blink (SOCOM 1's m_look, m_eyelook, m_blink were
     * pointers at 0xf90-0xf98; SOCOM II's are held: GetLookDir returns this + 0x11A0, SetLookDir
     * hands this + 0x1190 on, the eyes and lids align to 0x1210 and 0x1280) */
    CLook m_look;   // 0x1190
    CLook m_eyelook;   // 0x1200
    CBlink m_blink;   // 0x1270
    unsigned char _12e8[0x12f0 - 0x12e8];
    /* the turn in place's timing (SOCOM 1's m_turnip, a CTurnInPlace * at 0xf9c right after m_blink:
     * SOCOM II holds it, as the looks; StandTurning starts it) */
    CTurnInPlace m_turnip;   // 0x12f0
    unsigned char _1300[0x1314 - 0x1300];
    /* a point and two values 0x57ED10 sets for a SEAL this console plays, clears for a remote one
     * (the names ours) */
    CPnt3D m_1314;   // 0x1314
    float m_1320;   // 0x1320
    float m_1324;   // 0x1324
    /* the SEAL's hold on a ladder: a flag, the ladder's index, a state (1 and 3 hold it) -- new in
     * SOCOM II (0x5B14E0 lets go of it; the names ours) */
    struct CSealClimb {
        unsigned char m_active;   // 0x0
        unsigned int m_ladder;   // 0x4: unsigned (the SEALs, round 34) -- passed to GetLadderByIndex's
                                 // int, the conversion puts its load after the list's (0x5B14E0, 0x5A2490)
        unsigned char m_state;   // 0x8
    } m_climb;   // 0x1328
    AnimType idlehack;   // 0x1334 (SOCOM 1's 0xfa4: SetStanceIdle keeps the idle anim it pushes)
    short m_weapon_id;   // 0x1336: the weapon in hand, looked up in the AI's kind table (the name ours)
    unsigned char _1338[0x1340 - 0x1338];
    float m_1340;   // 0x1340: a float the ground probes read -- not 0, 0x54F410 keeps a probe's height
                    // (the constructor, 0x5483D0 and 0x54EDF0 set it; the name ours)
    unsigned char _1344[0x1350 - 0x1344];
    CPnt3D m_1350;   // 0x1350: the velocity kept as 0x57DF20/0x57E050 set bit 9 (the name ours)
    unsigned char _135c[0x1440 - 0x135c];
    CPnt3D m_aim_dir;   // 0x1440 (SOCOM 1's 0x10a0: AimWeapon sets it)
    unsigned char _144c[0x14a0 - 0x144c];
    /* the aim and fire points and their positions (SOCOM 1's m_curr_aimpos, m_curr_aimpt,
     * m_curr_firepos and m_curr_firept, 0x1100-0x111F there: the weapon takes look the points up by
     * name again, seal_weapon.cpp's UpdateAimParams through a view of its own) */
    CPnt3D m_curr_aimpos;   // 0x14a0
    zdb::CNode *m_curr_aimpt;   // 0x14ac
    CPnt3D m_curr_firepos;   // 0x14b0 (SOCOM 1's 0x1110, m_aim_dir's 0x3a0 on as here)
    zdb::CNode *m_curr_firept;   // 0x14bc
    unsigned char _14c0[0x14cc - 0x14c0];
    short m_last_foot;   // 0x14cc (SOCOM 1's 0x112c: Prone_0 sets it to 2, as SOCOM 1's does)
    unsigned char _14ce[0x14e0 - 0x14ce];   // (0x14E0 bytes: Create news 5344)
};

inline void *CSealCtrlAi::GetSelect()
{
    return ((CZSealBody *)m_entity)->m_select;
}

/* a SEAL's node extension: the SEAL the node belongs to (SOCOM 1's CSealEx, seal_ex.cpp's -- its RTTI
 * 0x65C238 and table 0x669570 OVL2 data, OnWeaponHit 0x5B8260; CZSealBody::GetSeal casts a node's
 * extension to it) */
class CSealEx : public zdb::CNodeEx {
public:
    virtual int OnWeaponHit(zdb::CNode *node, zdb::IntersectStruct *hit, CZProjectile *projectile);

    CZSealBody *m_seal;   // 0x4
};

/* the character's dynamics: its physics, its aim and look limits, its cameras, read once from its
 * .rdr (SOCOM 1's DWARF: CharacterDynamics, 0x1bc bytes -- SOCOM II's Load writes every member at
 * SOCOM 1's offset). The one instance is OVL1's theCharacterDynamics (bss 0x44C250, the size SOCOM
 * 1's); seal_dyn.cpp reads it. */
class CharacterDynamics {
public:
    /* a camera's place behind the SEAL and the point it looks at (SOCOM 1's DWARF, 0x18 bytes) */
    class CCameraParams {
    public:
        CCameraParams(const CPnt3D &offset, const CPnt3D &aim) : m_cam_offset(offset), m_cam_aimpoint(aim) {}

        CPnt3D m_cam_offset;   // 0x0
        CPnt3D m_cam_aimpoint;   // 0xc
    };

    /* the camera's wiggle and the throw's limits (SOCOM 1's DWARF) */
    struct cam_wiggle {
        float amplitude;   // 0x0
        float duration;   // 0x4
        float rate;   // 0x8
    };
    struct throw_params {
        float m_abort_threshold;   // 0x0
        float max_distance_stand;   // 0x4
        float max_distance_crouch;   // 0x8
        float toss_power_threshold;   // 0xc
        float toss_aim_threshold;   // 0x10
    };

    void Load(const CRdrFile &rdr);

    float m_gravity;   // 0x0
    float m_DamageToNewtons;   // 0x4
    float m_jumpFactor;   // 0x8
    float m_landFallRate;   // 0xc
    float m_landHardFallRate;   // 0x10
    float m_groundTouchDistance;   // 0x14
    float m_maxSlopeCos;   // 0x18
    float m_stepHeight;   // 0x1c
    float m_blastBoostY;   // 0x20
    float m_fallDist[3];   // 0x24
    float m_landSpeed[3];   // 0x30
    float m_standTurnFactor;   // 0x3c
    float m_turn_maxRate;   // 0x40
    float m_accelXMin;   // 0x44
    float m_accelXMax;   // 0x48
    float m_accelZMin;   // 0x4c
    float m_accelZMax;   // 0x50
    float m_initAimPitch;   // 0x54
    float m_maxAimPitch;   // 0x58
    float m_minAimPitch;   // 0x5c
    float m_maxAimYaw;   // 0x60
    float m_proneMaxAimPitch;   // 0x64
    float m_proneMinAimPitch;   // 0x68
    float m_proneMaxAimYaw;   // 0x6c
    float m_proneZoomDeltaYaw;   // 0x70
    float m_maxLookPitch;   // 0x74
    float m_minLookPitch;   // 0x78
    float m_maxLookYaw;   // 0x7c
    float m_maxEyelookPitch;   // 0x80
    float m_minEyelookPitch;   // 0x84
    float m_maxEyelookYaw;   // 0x88
    float m_maxBlinkRate;   // 0x8c
    float m_maxBlinkAngle;   // 0x90
    float m_cosMaxAimPitch;   // 0x94
    float m_cosMinAimPitch;   // 0x98
    float m_cosMaxAimYaw;   // 0x9c
    float m_cosProneMaxAimPitch;   // 0xa0
    float m_cosProneMinAimPitch;   // 0xa4
    float m_cosProneMaxAimYaw;   // 0xa8
    float m_cosMaxLookPitch;   // 0xac
    float m_cosMinLookPitch;   // 0xb0
    float m_cosMaxLookYaw;   // 0xb4
    float m_cosMaxEyelookPitch;   // 0xb8
    float m_cosMinEyelookPitch;   // 0xbc
    float m_cosMaxEyelookYaw;   // 0xc0
    float m_maxZoomAimPitch;   // 0xc4
    float m_minZoomAimPitch;   // 0xc8
    float m_maxZoomAimYaw;   // 0xcc
    float m_proneMaxZoomAimPitch;   // 0xd0
    float m_proneMinZoomAimPitch;   // 0xd4
    float m_proneMaxZoomAimYaw;   // 0xd8
    float m_cosMaxZoomAimPitch;   // 0xdc
    float m_cosMinZoomAimPitch;   // 0xe0
    float m_cosMaxZoomAimYaw;   // 0xe4
    float m_cosProneMaxZoomAimPitch;   // 0xe8
    float m_cosProneMinZoomAimPitch;   // 0xec
    float m_cosProneMaxZoomAimYaw;   // 0xf0
    float m_pitchRate;   // 0xf4
    float m_turnThrottleA;   // 0xf8
    float m_turnThrottleB;   // 0xfc
    float m_pitchThrottleA;   // 0x100
    float m_pitchThrottleB;   // 0x104
    float m_minWaterFactor;   // 0x108
    float m_waterFactorSlope;   // 0x10c
    float m_fbStiffness;   // 0x110
    float m_lrStiffness;   // 0x114
    float m_throtExp;   // 0x118
    float m_throtScale;   // 0x11c
    float m_cameraRoll;   // 0x120
    float m_zoomFactor;   // 0x124
    float m_zoomRate;   // 0x128
    float m_cameraSideHeight;   // 0x12c
    float m_cameraSideDist;   // 0x130
    float m_cameraSidePitch;   // 0x134
    float m_cameraFrontHeight;   // 0x138
    float m_cameraFrontDist;   // 0x13c
    float m_cameraFrontPitch;   // 0x140
    float m_cameraBackHeight;   // 0x144
    float m_cameraBackDist;   // 0x148
    float m_cameraBackPitch;   // 0x14c
    float m_cameraDownHeight;   // 0x150
    float m_cameraDownDist;   // 0x154
    float m_cameraDownPitch;   // 0x158
    float m_cameraTetherStiffness;   // 0x15c
    float m_cameraLookDwell;   // 0x160
    float m_cameraNetPosSmooth;   // 0x164
    float m_cameraReleaseRad;   // 0x168
    float m_cameraPeekDecayRate;   // 0x16c
    float m_running_reload_speed;   // 0x170
    float m_running_chgweapon_speed;   // 0x174
    float m_lowHeight;   // 0x178
    float m_medHeight;   // 0x17c
    float m_highHeight;   // 0x180
    float m_minStandHeight;   // 0x184
    float m_minJumpHeight;   // 0x188
    cam_wiggle m_cam_wiggle;   // 0x18c
    throw_params m_throw_params;   // 0x198
    unsigned char m_loaded;   // 0x1ac
    std::vector<CCameraParams> m_cam_params;   // 0x1b0
};   // 0x1bc bytes

extern CharacterDynamics theCharacterDynamics;

/* a round-robin share of the AI's work over the frames (new in SOCOM II; the names the kit lane's):
 * this frame's window of entity numbers, [m_start, m_end), wrapped past the m_total numbers into
 * [0, m_wrap) -- m_size numbers a frame, m_count how many were served. Five of them, OVL2 bss
 * 0x66B060 on, 0x20 apart (0x18 bytes each, aligned to 16: 0x66B0F8 is another word), built by
 * __sinit_seal_ai.cpp (0x6687E0, seal_ai.cpp defines them): CSealCtrlAi::PreTick reads the first,
 * second and fifth, 0x558640 the second, 0x56D2B0 the fifth, 0x574C30 resets those three, ai_posttick
 * (0x5CCA30, seal_sensors.cpp) moves them on and ScanTick (0x600550) reads the first; only the
 * initialiser touches the third and fourth. One class for both lanes' halves (the kit's round 26:
 * the SEALs' round 24 built the initialiser on its own copy) */
struct CAiSlice {
    CAiSlice()
    {
        m_count = 0;
        Reset();
    }
    /* (inline: 0x574C30 expands it three times, the sums last) */
    void Reset()
    {
        m_start = 0;
        m_size = 0;
        m_total = 0;
        m_wrap = 0;
        m_end = m_start + m_size;
    }
    /* whether a number takes its turn this frame (an inline bool: the target materialises it,
     * `daddiu 1` on the true arm) */
    bool Has(int id) const
    {
        if (id < m_wrap || (id >= m_start && id < m_end))
            return true;
        return false;
    }
    /* the window resized: m_size numbers out of m_total, the part past the end wrapped (an inline:
     * the target compares the arguments, held in registers, and reads the members back) */
    void SetSize(int size, int total)
    {
        m_wrap = 0;
        m_total = total;
        m_size = size;
        m_end = m_start + m_size;
        if (size >= total)
            m_wrap = size;
        else if (m_end > total)
            m_wrap = m_end - total;
        else
            m_wrap = 0;
    }
    /* the window moved on and resized to a new count (inline: ai_posttick's three -- not SetSize, whose
     * stores run m_wrap, m_total, m_size where these run m_size, m_total, m_wrap). The zero arm of the
     * wrap first (`m_total == 0 ? 0 : ...`, the target's `bnez total` to the divide) and the sum built
     * in two steps from m_size (the target loads m_size first and keeps the sum in its register) */
    void Next(int total, int size)
    {
        int start = m_size;

        start += m_start;
        if (start >= m_total)
            start = m_total == 0 ? 0 : start % m_total;
        m_start = start;
        m_size = size;
        m_total = total;
        m_wrap = 0;
        m_end = m_start + m_size;
        if (size >= total)
            m_wrap = size;
        else if (m_end > total)
            m_wrap = m_end - total;
        else
            m_wrap = 0;
        m_count = 0;
    }

    int m_start;   // 0x0
    int m_end;   // 0x4
    int m_size;   // 0x8
    int m_total;   // 0xc
    int m_wrap;   // 0x10
    int m_count;   // 0x14: the frame's takers
};

extern CAiSlice ai_slice0;
extern CAiSlice ai_slice1;
extern CAiSlice ai_slice2;
extern CAiSlice ai_slice3;
extern CAiSlice ai_slice4;

/* a dying SEAL's visibility requests dropped and its targets' entries for it unhooked (SOCOM 1's
 * free function; OVL2 0x5CC8A0 -- CSealCtrlAi::OnDeath calls it; seal_sensors.cpp, the kit lane's) */
void CleanupDiPool(CZSealBody *seal);

#endif
