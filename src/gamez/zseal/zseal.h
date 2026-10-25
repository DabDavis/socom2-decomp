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

    unsigned int m_0;
};

class CSealCtrlAi;

/* CAiState, its stack and AI_STATE are ai/ai_states.h's (the AI-states lane's header) */

/* the goals a controller pursues (SOCOM 1's DWARF, a byte) */
enum GOAL_TYPE { GOAL_NONE = 0, GOAL_SCAN = 1, GOAL_LOOK = 2, GOAL_MOVE = 3, GOAL_SHOOT = 4, GOAL_AVOID = 5, GOAL_RECOVER = 6 };

/* a SEAL's stance (SOCOM 1's DWARF, a signed byte) */
enum SEAL_STANCE {
    STANCE_NONE = -1, STANCE_STAND = 0, STANCE_CROUCH = 1, STANCE_PRONE = 2, STANCE_IDLE = 3,
    STANCE_MAX = 4
};

/* a random period and its countdown (SOCOM 1's DWARF: CAiTimer, an Rfloat {m_min, m_range} and
 * the timer; 0xc bytes) */
class CAiTimer {
public:
    float m_min;   // 0x0
    float m_range;   // 0x4
    float m_timer;   // 0x8

    /* the never-expiring period (SOCOM 1's infinite__8CAiTimer; OVL2 data, read as a variable) */
    static float infinite;
};

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
    unsigned char _0[0x18];
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
class CSealUnit {
public:
    unsigned int m_0;   // 0x0
    virtual void IssueCommand(CZSealBody *seal, FT_COMMAND cmd, void *data, bool b);
    virtual bool AllowCommand(CZSealBody *seal, FT_COMMAND cmd, void *data);   /* +0xC (SOCOM 1's) */
};

/* the network (SOCOM II's theNetwork at 0x45A0C0: SOCOM 1's flags at 8 and 9, the states lane's
 * reading) and the net game's state (OVL1 bss 0x437CE8, the HUD lane's reading) */
class CZNetwork {
public:
    /* this console's client index in the game (SOCOM 1's) */
    int zNetGetMyGameClientIndex();

    unsigned char m_8;   // 0x0: the session master
    unsigned char m_9;   // 0x1: an online game
};

extern CZNetwork theNetwork;

class CNetGameState {
public:
    unsigned char _0[0xd2];
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
    unsigned int GetPickupNetEntityID();
    const CPnt3D *GetPosW() const;
    void SetVisibleByUnit(unsigned int unit);
    bool GetVisibleByUnit(unsigned int unit);

    static std::list<CPickup *> m_list;   // OVL1 bss 0x44D470
    static zdb::CNode *m_mpbombnode;   // OVL1 bss 0x44D4A0
    static unsigned int bit_count;   // OVL1 data 0x3DF178
    static bool m_isOpen;   // OVL1 bss 0x44D4B0

    float m_timer;   // 0x0 (new in SOCOM II)
    zdb::CNode *m_node;   // 0x4
    void *m_data;   // 0x8: a grenade's projectile, a weapon's...
    unsigned int m_type : 8;   // 0xc (SOCOM 1's 7 bits)
    unsigned int m_locked : 1;
    unsigned int m_secondary : 1;
    unsigned int m_unused : 6;
    unsigned int m_id : 16;   // 0xe
    unsigned int m_owner;   // 0x10: the owner's entity id (new in SOCOM II)
    unsigned char m_bits[8];   // 0x14: a CBits (SOCOM 1's 0x10)
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
    unsigned char _b4[0xec - 0xb4];
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
    unsigned char _222[0x250 - 0x222];
};

/* the AI's controller (SOCOM 1's DWARF: CSealCtrlAi, 0x5d0 bytes; SOCOM II's is 0x7c0 --
 * ftsCreateSeal news 1984) -- only what the written functions touch */
class CZAnim;

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

class CSealCtrlAi : public CSealCtrl {
public:
    CSealCtrlAi();
    /* (0x573E60, not written yet: declared so no unit emits an implicit one) */
    virtual ~CSealCtrlAi();
    virtual void SetThrottle(unsigned int axis, float value);
    virtual bool SlideTick(float dT);
    /* never on in the AI mode 7 (OVL2 0x557EE0) */
    virtual void SetDoAiMapCollide(bool on);
    virtual void OnSetHealth(float health, float old);
    virtual void OnMissionEnd();
    virtual void OnStateChange(SEAL_STATE from, SEAL_STATE to);

    /* the AI's mode set (SOCOM 1's; OVL2 0x56CBF0) and its states dropped (SOCOM 1's
     * ClearAiState, the AI-states lane's 0x5F5F20) */
    void SetAiMode(AI_MODE mode);
    void ClearAiState(bool all);

    /* the animations bound (SOCOM 1's; OVL2 0x562AD0) and a task begun: its kind, a flag, then the
     * task's own arguments (SOCOM 1's; OVL2 0x561F00) */
    void SetupAnimations();
    bool ActivateEx(SEAL_TASK task, bool b, ...);
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
    virtual void Initalize();
    /* a Navy SEAL (SOCOM 1's bit; the accessor ours, as the code tests it materialised) */
    bool IsNavySeal() const { return m_navyseal ? true : false; }
    /* the body's task argument (an inline: expanded ahead of the call it is passed with) */
    inline void *GetSelect();

    unsigned char _250[0x254 - 0x250];
    float m_throttle_goal[3];   // 0x254 (as SOCOM 1's 0x274: the throttles the AI asks for)
    unsigned int m_260;   // 0x260
    unsigned int m_debug : 1;   // 0x264 (SOCOM 1's 0x280, the same bits as far as they are read)
    unsigned int m_navyseal : 1;
    unsigned int m_bits264 : 30;
    unsigned char _268[0x26c - 0x268];
    unsigned char m_26c;   // 0x26c: a mode byte (4: any target taken; the name ours)
    unsigned char _26d[0x278 - 0x26d];
    SEAL_STANCE m_stance;   // 0x278 (SOCOM 1's 0x285)
    float m_stance_delay;   // 0x27c (SOCOM 1's 0x288)
    unsigned char _280[0x3f4 - 0x280];
    unsigned char m_3f4;   // 0x3f4: a mode (7 keeps the AI-map collision off; the name ours)
    unsigned char _3f5[0x41c - 0x3f5];
    /* the state on top of the stack at 0x41C (SOCOM 1's; OVL2 0x5F5FD0, 0x5F6010, the AI-states
     * lane's seal_aistate.cpp defines them -- which is const by SOCOM 1's order) */
    CAiState *GetAiState();
    const CAiState *GetAiState() const;
    unsigned int m_41c;   // 0x41c: a state stack's size word (SOCOM 1's m_stateStack, a CAiStateStack)
    unsigned char _420[0x468 - 0x420];
    CZAnim *m_task_anim;   // 0x468: an anim the AI plays, stopped at the mission's end
    unsigned char _46c[0x4ec - 0x46c];
    unsigned char m_4ec;   // 0x4ec: a state (1-3 and, with 0x508 set, 4 hold the next byte back; the name ours)
    signed char m_4ed;   // 0x4ed
    unsigned char _4ee[0x508 - 0x4ee];
    void *m_508;   // 0x508
    unsigned char _50c[0x514 - 0x50c];
    unsigned char m_path[0x5bc - 0x514];   // 0x514: a CAiPath (SOCOM 1's m_path, 0x36c), reset at 0x527310
    AI_ROUTE m_route;   // 0x5bc: the route's state (read after the update at 0x555420)
    unsigned char _5bd[0x660 - 0x5bd];
    unsigned char m_660;   // 0x660: set at the mission's start in mode 1 (the names ours)
    unsigned char _661[0x664 - 0x661];
    CZSealBody *m_664;   // 0x664: the player, in mode 1
    unsigned char _668[0x674 - 0x668];
    float m_674;   // 0x674: a range, and its square
    float m_678;   // 0x678
    unsigned char m_67c[4];   // 0x67c: an object the state CAiSPursue tests (0x60A850)
    unsigned char _680[0x750 - 0x680];
    CValve *m_valve[3];   // 0x750 (SOCOM 1's 0x5bc, four there)
    unsigned char _75c[0x76c - 0x75c];
    unsigned char m_weapon_kind;   // 0x76c: the kind 0x560540 reads off the body's weapon (the name ours)
    unsigned char _76d[0x7c0 - 0x76d];
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

    float m_fade_min;   // 0x250: the corpse's fade floor (0.001; the names ours)
    float m_fade_time;   // 0x254: the corpse's fade time (2.0)
};

/* the animated body, CBody (0x90 bytes; its constructor OVL1 0x28EDD0), is zbody.h's (the body
 * lane's) */
#include "../zbody/zbody.h"

/* the SEAL (SOCOM 1's DWARF: CZSealBody : CEntity, CBody, 0x1140 bytes; SOCOM II's CBody base at
 * 0x170, after its 0x170-byte CEntity, the SEAL's own members from 0x200 -- the constructor at
 * 0x553EA0 builds the CBody at 0x170 and stores the second table there) -- only what the written
 * functions touch */
class CZSealBody : public CEntity, public CBody {
public:
    /* a SEAL made from a character type with a controller (SOCOM 1's; OVL2 0x5999E0), and the local
     * player's made for a network game (SOCOM 1's; OVL2 0x5442C0) */
    static CZSealBody *Create(const CCharacterType *type, const char *name, CSealCtrl *ctrl);
    /* (OVL2 0x553EA0, not written yet) and the controller fitted after (OVL2 0x599F00) */
    CZSealBody(zdb::CNode *node, CCharacterType *type);
    static void PostCreateSeal(CZSealBody *seal, CSealCtrl *ctrl);
    static void NetworkPlayerCreate();
    /* a remote SEAL unmade: its id cleared everywhere, dead, no team, the squad told (SOCOM 1's; OVL2
     * 0x5998B0) */
    static void Destroy(CZSealBody *seal);
    /* the character type taken and its model instanced, dynamic, with its LODs, in the world, its
     * textures added (SOCOM 1's; OVL2 0x59A7F0) */
    static const CCharacterType *PreCreateSeal(const CCharacterType *type, const char *name, zdb::CNode **node);
    /* every heartbeat loop stopped (SOCOM 1's; StopRumbler calls it with no body) */
    static void StopAllHeartbeatSounds();
    /* the stance a state stands for, or the fallback (SOCOM 1's; the AI-states lane's 0x5FC910) */
    static SEAL_STANCE GetStanceFromState(SEAL_STATE state, SEAL_STANCE fallback);
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
    virtual int Virtual60();
    virtual int Virtual68();
    /* the root part and the state set (CBody's virtuals, overridden), with one of the SEAL's own
     * between them: MW lists them after the CBody table, from +0x88, and calls them through the
     * primary pointer (Destroy's state set reads +0x90) */
    virtual CZBodyPart *GetRootPart() const;                                /* +0x88 */
    virtual void Virtual8C();                                               /* +0x8C (0x547350) */
    virtual void BodySetState(int state);                                   /* +0x90 */

    /* whether a player drives it (an inline the code shows: m_control tested, IsPlayer called) */
    bool IsPlayer() { return m_control ? ((CSealCtrl *)m_control)->IsPlayer() : false; }
    float SetHealth(float health, enum SEAL_DEATH_TYPE type);
    /* the character the SEAL was made from (SOCOM 1's; OVL2 0x599810) */
    CCharacterType *GetCharacter();
    void ResetAltitude(float y);
    void SetSnakePathUpdate(bool on, unsigned int length);
    void RestrainBuddy(CZSealBody *victim);
    void StabBuddy(CZSealBody *victim);
    bool ExecuteAction_ApplyRestraint(CZSealBody *victim);
    bool ExecuteAction_CQBAttack(CZSealBody *victim);
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
    /* the stance's change taken, once (seal_body.cpp) */
    bool StanceChanged();
    /* the HUD's flags: the hit set, and each taken once (seal_env.cpp, SOCOM 1's) */
    void SetHudHitFlag();
    bool HudUpdateViewCone();
    bool GetHudHitFlag();
    /* the terrain a SEAL can cross: an int, the callers make it a bool (seal_env.cpp) */
    int CanTraverse(CDIType type);
    bool CanClimbWall();
    void AttemptBombDefuse();

    unsigned char _200[0x2e4 - 0x200];
    float m_alt;   // 0x2e4 (SOCOM 1's 0xe98)
    CZBodyPart *m_root;   // 0x2e8 (SOCOM 1's 0x26c)
    unsigned char _2ec[0x374 - 0x2ec];
    SEAL_STATE m_desiredState;   // 0x374 (SOCOM 1's 0x304; the HUD's pose bitmap reads it)
    unsigned char _375[0x3b0 - 0x375];
    void *m_select;   // 0x3b0: a TASK_SELECT (SOCOM 1's 0x318), the AI's tasks' argument
    unsigned char _3b4[0x400 - 0x3b4];
    zdb::IntersectStruct m_last_altitude;   // 0x400 (SOCOM 1's 0x320)
    unsigned char _420[0x434 - 0x420];
    unsigned char m_snakePath[4];   // 0x434: a CAiPath (SOCOM 1's 0x384)
    unsigned char _438[0x528 - 0x438];
    class AnimSet *m_animset;   // 0x528 (SOCOM 1's 0x45c)
    unsigned char _52c[0x5b0 - 0x52c];
    unsigned short m_5b0;   // 0x5b0: two of the stats' counters (the CQB attack adds 2 and 1; the
    unsigned char _5b2[0x5b8 - 0x5b2];   //  names ours)
    unsigned short m_5b8;   // 0x5b8
    unsigned char _5ba[0x5e0 - 0x5ba];
    CZKit m_kit;   // 0x5e0 (SOCOM 1's 0x520)
    unsigned char _5e4[0xec0 - 0x5e4];
    CCompassAnchor *m_mpbomb_anchor;   // 0xec0 (SOCOM 1's 0xdb4)
    CCompassAnchor *m_mpbase_anchor;   // 0xec4 (SOCOM 1's 0xdb8)
    unsigned char _ec8[0xed0 - 0xec8];
    unsigned int m_action_available;   // 0xed0: a bit per action (SOCOM 1's ActionAvailableMask word, 0xdc0)
    unsigned char _ed4[0xee0 - 0xed4];
    float m_wep_upT;   // 0xee0 (SOCOM 1's 0xdd0)
    unsigned char _ee4[0xfb0 - 0xee4];
    SEAL_DEATH_TYPE m_deathType;   // 0xfb0 (SOCOM 1's 0xea4)
    unsigned char _fb1[0xfb4 - 0xfb1];
    float m_time_of_death;   // 0xfb4 (SOCOM 1's 0xea8)
    unsigned char _fb8[0xfbc - 0xfb8];
    unsigned int m_lastkiller_id;   // 0xfbc: the killer's entity id (SOCOM 1's 0xeb4)
    unsigned char _fc0[0x1044 - 0xfc0];
    float m_health;   // 0x1044 (SOCOM 1's 0xea0)
    unsigned char _1048[0x105c - 0x1048];
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
    unsigned int m_unused_bits : 26;
    unsigned char _1064[0x1336 - 0x1064];
    short m_weapon_id;   // 0x1336: the weapon in hand, looked up in the AI's kind table (the name ours)
    unsigned char _1338[0x14e0 - 0x1338];   // (0x14E0 bytes: Create news 5344)
};

inline void *CSealCtrlAi::GetSelect()
{
    return ((CZSealBody *)m_entity)->m_select;
}

#endif
