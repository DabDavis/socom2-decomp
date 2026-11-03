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

/* the fire trigger's edge state (SOCOM 1's DWARF, a byte) */
enum TRIGGER { TRIGGER_UP = 0, TRIGGER_DOWN = 1, TRIGGER_FALLING = 2, TRIGGER_RISING = 3, TRIGGER_FINISH = 4 };

/* the body's states (SOCOM 1's DWARF, a byte) */
enum SEAL_STATE {
    stateStand = 0, stateCrouch = 1, stateProne = 2, statePeek = 3, stateHang = 4,
    stateClimbLadder = 5, stateInAir = 6, stateIdle = 7, stateDead = 8, stateCarrying = 9,
    stateCarried = 10, stateDone = 11
};

/* a kind of collision polygon (SOCOM 1's CDIType; no DWARF, only the size matters here) */
enum CDIType { CDI_TYPE_0 = 0, CDI_TYPE_LAST = 255 };

/* an animation's kind (SOCOM 1's DWARF: AnimType, a 2-byte enum from -1 to 142 -- SOCOM II's code
 * narrows it as a short; only its bounds here) */
enum AnimType { animTypeUndefined = -1, animTypeRef = 0, animTypeLast = 142 };

/* how a SEAL died (SOCOM 1's DWARF, a byte) */
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
    /* the item in hand (kit.h's: const in SOCOM II; OVL2 0x5C3780) */
    class CZFTSWeapon *GetCurItem() const;
    /* the item at an index let go with its ammunition (SOCOM 1's; OVL2 0x5B9BB0, the kit lane's) */
    void RemoveItemAndAmmo(int index);

    /* kit.h's flags (the kit lane's names): only bit 12 here */
    unsigned int m_bits0 : 12;   // 0x0
    unsigned int m_requested_item : 1;   // bit 12: an item asked for (IsChangingWeapons tests it)
    unsigned int m_bits13 : 19;
};

class CSealCtrlAi;

/* CAiState, its stack and AI_STATE are ai/ai_states.h's (the AI-states lane's header) */

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
    unsigned char _0[0x14];
    unsigned int m_team;   // 0x14: the team it is for (CanPlaceBomb matches it; the name ours)
    CValve *m_valve;   // 0x18
    unsigned char _1c[0x24 - 0x1c];
    NODEACTION_TYPE m_type;   // 0x24
};

/* the AI's maps (SOCOM 1's CAiMaps; the global AiMaps, OVL2 bss) -- only the cell's centre */
class CAiMaps {
public:
    bool GetCellCenter(const CAiMapLoc &loc, CPnt3D *center);
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

    unsigned int m_0;   // 0x0
    virtual void IssueCommand(CZSealBody *seal, FT_COMMAND cmd, void *data, bool b);
    virtual bool AllowCommand(CZSealBody *seal, FT_COMMAND cmd, void *data);   /* +0xC (SOCOM 1's) */

    unsigned char _8[0x9c - 0x8];
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
    unsigned char _0[0x70];
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
    virtual void CreepTick(float dT);                                       /* +0x88 */
    virtual bool SlideTick(float dT);   /* SOCOM II's returns whether the SEAL slid */
    /* +0x90-0x98, new in SOCOM II: the fudge velocities set (the names ours) */
    virtual void SetFudgeVelM(const CPnt3D *v) { m_fudge_velM = *v; }
    virtual void SetFudgeVelR(const CPnt3D *v) { m_fudge_velR = *v; }
    virtual void SetFudgeVelW(const CPnt3D *v) { m_fudge_velW = *v; }
    virtual void Virtual9C();                                               /* +0x9C (new) */
    virtual void NotifyTargetDown(CZSealBody *seal);
    virtual bool GetJumpFlag() const { return false; }                     /* +0xA4 */
    virtual void UpdateViewCone();
    virtual void OnSetTargetVisible(CTarget &target, bool visible, zdb::DiIntersect *di);
    virtual void SetTarget(CTarget *target);                                /* +0xB0 */
    /* +0xB4, new in SOCOM II: the AI-map collision bit (the name ours) */
    virtual void SetDoAiMapCollide(bool on) { m_do_aimap_collide = on; }

    /* the goals: a direction, a position (a point or a map cell's centre), cleared (seal_ai.cpp,
     * SOCOM 1's; OVL2 0x56D920, 0x56DAA0, 0x56DBA0, 0x56DCA0) */
    bool SetGoalDir(GOAL_TYPE type, const CPnt3D *dir, bool b);
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

    unsigned char _18[0xa8 - 0x18];
    CAiDecay m_visrange;   // 0xa8 (as SOCOM 1's)
    unsigned char _b4[0xd8 - 0xb4];
    float m_d8;   // 0xd8: a range 0x570D90 falls back to (the name ours)
    unsigned char _dc[0xec - 0xdc];
    float m_ec;   // 0xec: the view range again (new in SOCOM II; the name ours)
    unsigned char _f0[0x10c - 0xf0];
    CSealUnit *m_unit;   // 0x10c: the squad (SOCOM 1's 0xdc)
    unsigned char _110[0x114 - 0x110];
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
    unsigned char _154[0x170 - 0x154];
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
    unsigned int m_unused : 14;
    CPnt3D m_goal_dir;   // 0x174 (SOCOM 1's 0x16c)
    CPnt3D m_goal_pos;   // 0x180
    CPnt3D m_fudge_velR;   // 0x18c
    CPnt3D m_fudge_velM;   // 0x198
    CPnt3D m_fudge_velW;   // 0x1a4
    GOAL_TYPE m_goal_type;   // 0x1b0 (SOCOM 1's 0x1a8)
    float m_goal_tolerance;   // 0x1b4
    CAiTimer m_goal_timer;   // 0x1b8
    unsigned char _1c4[0x221 - 0x1c4];
    MENU_STATE m_menustate;   // 0x221 (as SOCOM 1's)
    unsigned char _222[0x244 - 0x222];
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
 * CAiEventQ's (0x573FF0) -- seal_ai.cpp's copies, after ~CSealCtrlAi -- and CAiPath's (0x5281B0,
 * which ai.h does not declare: its implicit one would be inlined) */
extern "C" {
void *func_00573FF0(void *queue, int flags);
void *func_00573F80(void *event, int flags);
void *func_00573F20(void *route, int flags);
void *func_005281B0(void *path, int flags);
}

/* a CAiPath's bytes (ai.h's class, 0xA8: built and destroyed by hand here, as ai.h does not
 * declare SOCOM II's ~CAiPath) and the fields the controller reads: the plan's state, the search's
 * status, the smoothed points' end (an empty vector's is 0) */
struct CAiPathBytes {
    unsigned char _0[0x80];
    unsigned char m_state;   // 0x80: CAiPath::PATH_STATE (1-3 planning, 4 complete)
    signed char m_status;   // 0x81: CAiPath::ASTAR_STATUS
    unsigned char _82[0x9c - 0x82];
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
        func_00573FF0(m_eventQueue, -1);
        func_00573F80(m_event, -1);
        func_00573F20(&m_route, -1);
        func_005281B0(&m_path, -1);
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
    unsigned int m_bits3 : 5;
    unsigned int m_aibit8 : 1;   // bits 8 and 9: either refuses a state push without flag 4
    unsigned int m_aibit9 : 1;   // (0x5F5CD0)
    unsigned int m_bits10 : 3;
    unsigned int m_aibit13 : 1;
    unsigned int m_bits14 : 2;
    unsigned int m_aibit16 : 1;
    unsigned int m_aibit17 : 1;
    unsigned int m_bits18 : 14;
    unsigned char _268[0x26c - 0x268];
    unsigned char m_26c;   // 0x26c: a mode byte (4: any target taken; the name ours)
    unsigned char _26d[0x26e - 0x26d];
    unsigned char m_26e;   // 0x26e: a mode byte (2 refuses SetSprayTimer; the name ours)
    unsigned char _26f[0x270 - 0x26f];
    int m_270;   // 0x270: a count 0x56D450 sets with the cell it finds (the names ours)
    void *m_274;   // 0x274: that cell
    SEAL_STANCE m_stance;   // 0x278 (SOCOM 1's 0x285)
    SEAL_STANCE m_stance_asked;   // 0x279: the stance SetStance was last asked for (the names ours)
    float m_stance_delay;   // 0x27c (SOCOM 1's 0x288)
    float m_280;   // 0x280: the delay before the asked stance is taken (cleared by SetStance)
    unsigned char _284[0x364 - 0x284];
    CAiTimer m_stance_timer;   // 0x364: restarted with each desired stance (0x5631B0; the name ours)
    unsigned char _370[0x3f4 - 0x370];
    unsigned char m_3f4;   // 0x3f4: a mode (7 keeps the AI-map collision off; the name ours)
    AI_MODE m_3f5;   // 0x3f5: the mode RestoreAiMode returns to, then 1 (a byte enum, copied with lb; the name ours)
    unsigned char _3f6[0x41c - 0x3f6];
    /* the state on top of the stack (SOCOM 1's; OVL2 0x5F5FD0, 0x5F6010, the AI-states lane's
     * seal_aistate.cpp defines them -- which is const by SOCOM 1's order) */
    CAiStateStack m_stateStack;   // 0x41c (SOCOM 1's 0x324; 0x20 bytes in SOCOM II)
    unsigned char _43c[0x450 - 0x43c];
    /* the anims the tasks play (SOCOM 1's m_task_anim[6], 0x33c there, its m_task_handle[6] 0x18
     * after: SetupAnimations binds [0] to <node>_start and [2] to <node>_ambient) -- so the
     * m_task_anim below, at 0x468, is SOCOM 1's m_task_handle[0] */
    CZAnim *m_anims[6];   // 0x450
    CZAnim *m_task_anim;   // 0x468: an anim the AI plays, stopped at the mission's end
    CAiPathBytes m_path;   // 0x46c: a CAiPath (SOCOM 1's m_path, 0x36c)
    CAiRouteBytes m_route;   // 0x514: a route (its path reset at 0x527310, its state read after
                             // the update at 0x555420)
    unsigned char _5c0[0x628 - 0x5c0];
    unsigned char m_event[0x28];   // 0x628: a CAiEvent (SOCOM 1's m_event, 0x480)
    unsigned char m_eventQueue[0x10];   // 0x650: a CAiEventQ (SOCOM 1's m_eventQueue, 0x4a8)
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
    unsigned char _6fc[0x74c - 0x6fc];
    float m_spraytimer;   // 0x74c: SetSprayTimer's time (the name ours)
    CValve *m_valve[3];   // 0x750 (SOCOM 1's 0x5bc, four there)
    unsigned char _75c[0x76c - 0x75c];
    unsigned char m_weapon_kind;   // 0x76c: the kind 0x560540 reads off the body's weapon (the name ours)
    unsigned char _76d[0x7c0 - 0x76d];
};

class CSealCtrlAi : public CSealCtrl, public SEAL_AI_PARAMS {
public:
    CSealCtrlAi();
    /* (0x573E60: the stack emptied, then the bases' destructors) */
    virtual ~CSealCtrlAi();
    virtual void SetThrottle(unsigned int axis, float value);
    virtual bool SlideTick(float dT);
    /* never on in the AI mode 7 (OVL2 0x557EE0) */
    virtual void SetDoAiMapCollide(bool on);
    virtual void OnSetHealth(float health, float old);
    virtual void OnMissionEnd();
    virtual void OnStateChange(SEAL_STATE from, SEAL_STATE to);
    /* a target seen to fall called out (SOCOM 1's; OVL2 0x5C9B10, the kit lane's seal_sensors.cpp) */
    virtual void NotifyTargetDown(CZSealBody *seal);

    /* the AI's mode set (SOCOM 1's; OVL2 0x56CBF0) and its states dropped (SOCOM 1's
     * ClearAiState, the AI-states lane's 0x5F5F20) */
    void SetAiMode(AI_MODE mode);
    bool ClearAiState(bool all);   /* returns a bool (the kit lane: 0x5F5F20 sets v0) */
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

    /* the animations bound (SOCOM 1's; OVL2 0x562AD0) and a task begun: its kind, a flag, then the
     * task's own arguments (SOCOM 1's; OVL2 0x561F00) */
    bool SetupAnimations();
    bool ActivateEx(SEAL_TASK task, bool b, ...);
    /* the stance the body is to take, set when prone, when a target is held with its bit 9, or
     * when forced, its timer restarted with the delay (OVL2 0x5631B0, new in SOCOM II: CTStance's
     * Begin and End call it; the name ours) */
    void SetDesiredStance(SEAL_STANCE stance, float delay, bool force);
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
    /* a spray of fire timed, when the item in hand fires fully automatic (SOCOM 1's; OVL2 0x565AE0) */
    void SetSprayTimer(float t);
    /* the aim turned off its line by the target's error (slot +0x74, SOCOM 1's; OVL2 0x5C9250, the
     * kit lane's seal_kit.cpp defines it) */
    virtual void PerturbAimVector(CPnt3D *v);
    virtual void Initalize();
    /* a Navy SEAL (SOCOM 1's bit; the accessor ours, as the code tests it materialised) */
    bool IsNavySeal() const { return m_navyseal ? true : false; }
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
    virtual bool GetJumpFlag() const;

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
    /* the animation set's limits on the sideways and forward motion (seal_cfg.cpp) */
    void GetMaxXMotion(float &a, float &b);
    void GetMaxZMotion(float &a, float &b);

    /* (0x5533E0, not written yet: declared so no unit emits an implicit one) */
    virtual ~CZSealBody();
    virtual void OnMissionStart();                                          /* +0x4C */
    virtual void OnMissionEnd();
    virtual void OnRecycleEntity(CEntity *e);                               /* +0x54 */
    virtual void TeleportTo(const CPnt3D *pos, const CQuat *quat);          /* +0x3C */
    virtual void TeleportTo(const CMatrix *m);
    virtual void OnDeath();                                                 /* +0x58 */
    virtual int Virtual5C();
    virtual bool Virtual60(CPnt3D *out);
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
    /* the blood decals, one picked at random (SOCOM 1's static vector<CZDecalDef>; OVL1 bss
     * 0x44D568) */
    static std::vector<class CZDecalDef> m_blooddriptexhandles;
    /* the database's handles let go (SOCOM 1's static; OVL2 0x5A3BB0) */
    static void UninitDatabaseHandles();
    /* a weapon's event handed to every SEAL's controller (SOCOM 1's static callback; OVL2 0x5A4F80) */
    static void HandleWeaponEvent(CZProjectile *proj, unsigned int event, void *data);
    void SetSnakePathUpdate(bool on, unsigned int length);
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
    /* an action offered or withdrawn (SOCOM 1's took an unsigned int id) */
    void ActionAvailableMask(unsigned char id, bool on);
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
    /* the stance's change taken, once (seal_body.cpp) */
    bool StanceChanged();
    /* the weapon's state without its bit 4 (SOCOM 1's; OVL2 0x5E0010, seal_weapon.cpp) */
    WEAPON_STATE GetWeaponState() const;
    /* the weapon's state set (SOCOM 1's; OVL2 0x5DFE30, the kit lane's seal_weapon.cpp range) */
    void SetWeaponState(WEAPON_STATE state);
    /* whether an anim type is the blend on top (SOCOM 1's; OVL2 0x577E40, seal_anim.cpp) */
    bool IsCurrentBlend(AnimType type) const;
    /* a stun taken: the kind's highest kept, the controller told (SOCOM 1's; OVL2 0x5A53D0) */
    void HandleStunDamage(STUN_TYPE type, float amount, const CPnt3D *from);
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
    /* the MP bomb picked up (SOCOM 1's; OVL2 0x59DEF0) */
    void HandlePickupMPBomb(CPickup *pickup);
    /* the MP bomb made a pickup at a point (SOCOM 1's; OVL2 0x59DAB0, not written yet) and every
     * item dropped (SOCOM 1's; OVL2 0x59DDB0) */
    static class CPickup *CreateMPBombPickup(const CPnt3D *pos, bool b);
    void DropAllItems();
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
    void CalcGoalDir(const CPnt3D *cur, const CPnt3D *dir, CPnt3D *goal, float up, float down, float left,
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
    /* whether a weapon change, or a reload, is under way (SOCOM 1's; OVL2 0x5A7980, and 0x5A7AB0 not
     * written yet) */
    bool IsChangingWeapons(bool kit);
    bool IsReloading();
    bool CanChangeItem();
    /* the SEAL's own AI event made where it stands (SOCOM 1's; OVL2 0x550E50, seal.cpp) */
    void CreateAiEvent();
    /* the SEAL's adrenaline, its level first (SOCOM 1's; OVL2 0x5781B0, seal_anim.cpp's last) */
    struct CAdrenaline *Adrenaline();
    /* the heartbeat's loop kept at the adrenaline's pace (SOCOM 1's; OVL2 0x5437D0, seal.cpp), and
     * its statics: the three loops (OVL1 bss 0x44CE00), the one playing and its index (SOCOM 1's
     * m_heartbeat_sndisnt and m_heartbeat_curindex, OVL1 bss 0x44CE10, 0x44CE18) */
    void TickHeartbeat();
    static class CSnd *m_heartbeat_sound[3];
    static class CSndInstance *m_heartbeat_sndisnt;
    static int m_heartbeat_curindex;

    unsigned char _200[0x214 - 0x200];
    float m_blooddriptimer;   // 0x214 (SOCOM 1's 0x170)
    /* the SEAL whose node 0x3DC holds, kept while bit 0 of 0x21C is set (0x5AA620, the radio menu's
     * toggle; the names ours) */
    CZSealBody *m_218;   // 0x218
    unsigned char m_21c_bit0 : 1;   // 0x21c
    unsigned char m_altitude_characters : 1;   // bit 1: SealGetAltitude hits characters too (SOCOM 1's
                                               // flag at 0xde8 bit 0; the name ours)
    unsigned char m_21c_bit2 : 1;   // bit 2: a crouch 0x58A720 reports as a stand (the name ours)
    unsigned char _21c_bits : 5;
    unsigned char _21d[0x22c - 0x21d];
    int m_22c;   // 0x22c: a time on the network's clock 0x5A8850 waits for (the names ours)
    unsigned char _230[0x240 - 0x230];
    float m_240;   // 0x240: PostCheckDILadder checks the ladder only from 0.03 up (the name ours)
    unsigned char _244[0x2e0 - 0x244];
    float m_2e0;   // 0x2e0: the time step the overlay ticks by (0x579EF0; the names ours)
    float m_alt;   // 0x2e4 (SOCOM 1's 0xe98)
    CZBodyPart *m_root;   // 0x2e8 (SOCOM 1's 0x26c)
    /* the body's parts in SOCOM 1's order, 0x7c on (SOCOM 1's ParentPistolToRHandCallback reparents
     * m_pistol, 0x2d8 there, to m_rhand, 0x284; OVL2 0x5A7730 does the same with 0x354 and 0x300) */
    unsigned char _2ec[0x2fc - 0x2ec];
    CZBodyPart *m_spinelo;   // 0x2fc (SOCOM 1's 0x280: the rifle's place on the back)
    CZBodyPart *m_rhand;   // 0x300 (SOCOM 1's 0x284)
    unsigned char _304[0x308 - 0x304];
    CZBodyPart *m_head;   // 0x308 (SOCOM 1's 0x28c, m_root + 0x20 in both: 0x57F150's ray starts above it)
    unsigned char _30c[0x318 - 0x30c];
    CZBodyPart *m_rthigh;   // 0x318 (SOCOM 1's 0x29c: the pistol's holster)
    unsigned char _31c[0x350 - 0x31c];
    CZBodyPart *m_rifle;   // 0x350 (SOCOM 1's 0x2d4)
    CZBodyPart *m_pistol;   // 0x354 (SOCOM 1's 0x2d8)
    /* the eyes and their lids (SOCOM 1's 0x2e0-0x2ec: SOCOM II drops m_grenade before them, so
     * 0x78 on; the looks' Apply* turn them) */
    CZBodyPart *m_reyeball;   // 0x358
    CZBodyPart *m_leyeball;   // 0x35c
    CZBodyPart *m_reyelid;   // 0x360
    CZBodyPart *m_leyelid;   // 0x364
    unsigned char _368[0x374 - 0x368];
    SEAL_STATE m_desiredState;   // 0x374 (SOCOM 1's 0x304; the HUD's pose bitmap reads it)
    SEAL_STANCE m_desiredStance;   // 0x375: the stance the AI asks for (0x5631B0; the name ours)
    unsigned char _376[0x388 - 0x376];
    /* the items worn as gear, by SOCOM 1's SEAL_ITEM (1 the rifle, 2 the pistol; AimWeapon aims along
     * 1, 2 and 4): the part SetPartAsGear makes of each and the gear (SOCOM 1's m_rifle_as_gear and
     * m_pistol_as_gear were the gear alone; the names ours) */
    struct CItemGear {
        CZBodyPart *m_part;   // 0x0
        class CCharacterGear *m_gear;   // 0x4
    } m_item_gear[5];   // 0x388 (the rifle's 0x390, the pistol's 0x398)
    void *m_select;   // 0x3b0: a TASK_SELECT (SOCOM 1's 0x318), the AI's tasks' argument
    unsigned char _3b4[0x3dc - 0x3b4];
    zdb::CNode *m_3dc;   // 0x3dc: a node 0x5AA620 takes the SEAL of (the name ours)
    unsigned char _3e0[0x400 - 0x3e0];
    zdb::IntersectStruct m_last_altitude;   // 0x400 (SOCOM 1's 0x320)
    unsigned char _420[0x428 - 0x420];
    CAiEvent *m_aiEvent;   // 0x428: the SEAL's own AI event (SOCOM 1's 0x380)
    unsigned char _42c[0x434 - 0x42c];
    unsigned char m_snakePath[4];   // 0x434: a CAiPath (SOCOM 1's 0x384)
    unsigned char _438[0x528 - 0x438];
    class AnimSet *m_animset;   // 0x528 (SOCOM 1's 0x45c)
    unsigned char _52c[0x534 - 0x52c];
    AnimType m_curblend;   // 0x534: the anim type on top of the blends (SOCOM 1's short at 0x464)
    unsigned char _536[0x58c - 0x536];
    /* (SOCOM 1's m_stats, 0x4c0, right before m_accuracy as here: the stats' events step its
     * counters -- the CQB attack two of them -- and 0x545890 returns its address) */
    CSealStats m_stats;   // 0x58c
    CAccuracy m_accuracy;   // 0x5d4 (SOCOM 1's 0x514: PerturbAimVector reads it)
    CZKit m_kit;   // 0x5e0 (SOCOM 1's 0x520)
    unsigned char _5e4[0xeb0 - 0x5e4];
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
    unsigned char _ed4[0xee0 - 0xed4];
    float m_wep_upT;   // 0xee0 (SOCOM 1's 0xdd0)
    unsigned char _ee4[0xf74 - 0xee4];
    /* the weapon's state, bit 4 a flag GetWeaponState masks off (SOCOM 1's m_weapon_state, 0xe54) */
    unsigned int m_weapon_state;   // 0xf74
    unsigned char _f78[0xf79 - 0xf78];
    /* the item in hand (SOCOM 1's m_item, a SEAL_ITEM byte at 0xe58: 1 the rifle, 2 the pistol, 3 a
     * throwable; zweapon.h's SEAL_ITEM is a word-sized stand-in, so a byte here) */
    unsigned char m_item;   // 0xf79
    /* the SEAL's life as the network reports it: 2 and 3 down, 1 up again (0x544210 sets it;
     * the names ours) */
    unsigned char m_netlife;   // 0xf7a
    unsigned char _f7b[0xf88 - 0xf7b];
    float m_f88;   // 0xf88: a speed (0x5857E0 refuses a SEAL in water above 8.5; the name ours)
    unsigned char _f8c[0xfb0 - 0xf8c];
    SEAL_DEATH_TYPE m_deathType;   // 0xfb0 (SOCOM 1's 0xea4)
    unsigned char _fb1[0xfb4 - 0xfb1];
    float m_time_of_death;   // 0xfb4 (SOCOM 1's 0xea8)
    unsigned char _fb8[0xfbc - 0xfb8];
    unsigned int m_lastkiller_id;   // 0xfbc: the killer's entity id (SOCOM 1's 0xeb4)
    unsigned char _fc0[0xfc8 - 0xfc0];
    int m_ClientIndex;   // 0xfc8 (SOCOM 1's 0xeb8: CreateRemoteSeal stores its first argument in both)
    unsigned char _fcc[0xfcf - 0xfcc];
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
    unsigned char _1048[0x1058 - 0x1048];
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
    unsigned int m_unused_bits : 3;
    unsigned int m_bit9 : 1;   // bit 9: a remote SEAL with it set is skipped by 0x57DFE0 (the name ours)
    unsigned int m_bit10 : 1;   // bit 10: cleared with bit 9 set by 0x57DF20/0x57E050 (the name ours)
    unsigned int m_unused_bits2 : 5;
    /* the ladder the SEAL is on, an index into the mission's list (new in SOCOM II: read with lhu
     * and re-masked for an int argument, as a 16-bit field is; the name ours) */
    unsigned int m_ladder : 16;   // 0x1062
    unsigned char _1064[0x1070 - 0x1064];
    CQuat m_fwdlook;   // 0x1070 (SOCOM 1's 0xf60: RotateHead sets it, the app camera's transitions call that)
    unsigned char _1080[0x10d0 - 0x1080];
    float m_10d0;   // 0x10d0: set to 0.4 when the overlay plays out (0x579EF0)
    unsigned char _10d4[0x1190 - 0x10d4];
    /* the head's look, the eyes' and the lids' blink (SOCOM 1's m_look, m_eyelook, m_blink were
     * pointers at 0xf90-0xf98; SOCOM II's are held: GetLookDir returns this + 0x11A0, SetLookDir
     * hands this + 0x1190 on, the eyes and lids align to 0x1210 and 0x1280) */
    CLook m_look;   // 0x1190
    CLook m_eyelook;   // 0x1200
    CBlink m_blink;   // 0x1270
    unsigned char _12e8[0x1314 - 0x12e8];
    /* a point and two values 0x57ED10 sets for a SEAL this console plays, clears for a remote one
     * (the names ours) */
    CPnt3D m_1314;   // 0x1314
    float m_1320;   // 0x1320
    float m_1324;   // 0x1324
    /* the SEAL's hold on a ladder: a flag, the ladder's index, a state (1 and 3 hold it) -- new in
     * SOCOM II (0x5B14E0 lets go of it; the names ours) */
    struct CSealClimb {
        unsigned char m_active;   // 0x0
        int m_ladder;   // 0x4
        unsigned char m_state;   // 0x8
    } m_climb;   // 0x1328
    unsigned char _1334[0x1336 - 0x1334];
    short m_weapon_id;   // 0x1336: the weapon in hand, looked up in the AI's kind table (the name ours)
    unsigned char _1338[0x1350 - 0x1338];
    CPnt3D m_1350;   // 0x1350: the velocity kept as 0x57DF20/0x57E050 set bit 9 (the name ours)
    unsigned char _135c[0x1440 - 0x135c];
    CPnt3D m_aim_dir;   // 0x1440 (SOCOM 1's 0x10a0: AimWeapon sets it)
    unsigned char _144c[0x14e0 - 0x144c];   // (0x14E0 bytes: Create news 5344)
};

inline void *CSealCtrlAi::GetSelect()
{
    return ((CZSealBody *)m_entity)->m_select;
}

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

#endif
