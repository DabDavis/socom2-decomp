/* seal_control.cpp -- the SEAL's controller, CSealCtrl: the pad's throttles, the trigger, the
 * rumble, the zoom, the menus (SOCOM 1's unit, __sinit_seal_control.cpp; SOCOM 1's order, the
 * MP bomb's helpers first and the constructor last). OVL2 0x5920A0-0x598840. Defined in reverse
 * of their image order (-inline deferred). */

#include "zseal.h"
#include "../zinput/zinput.h"
#include "../zanim/zanim.h"

extern "C" int rand(void);

extern "C" {
extern bool data_00650698;   /* the rumble switch (SOCOM 1's file static) */
extern float data_006505E8;  /* (the weapon's raise time a closed menu restarts) */
/* ai::quadratic (SOCOM 1's quadratic__2aiFfffRfRf, an earlier unit's): the roots of a x^2 + b x
 * + c, false when there are none */
bool func_0050DE20(float a, float b, float c, float &r0, float &r1);
/* a team's squad (SOCOM 1's CSealUnit::GetUnitByTeam, by its use; OVL2 0x5EF2F0) */
CSealUnit *func_005EF2F0(unsigned int team);
extern CNetGameState *data_00437CE8;
}

/* a vector's length squared on the ground plane (OVL2 0x5975B0: an inline's copy, before the
 * grenade's statics; PostTick and the kit call it) */
extern "C" float func_005975B0(const CPnt3D *v)
{
    return v->x * v->x + v->z * v->z;
}

/* the world's camera and its far plane (zdb::CWorld holds it at 0x128, the plane at 0xEC) */
struct CViewCamera {
    unsigned char _0[0xec];
    float m_far;   // 0xec
};

struct CViewWorld {
    unsigned char _0[0x128];
    CViewCamera *m_camera;   // 0x128
};

/* the MP bomb's state machine (SOCOM 1's CZBombState, OVL1's): its status, the state first */
class CZBombState {
public:
    struct STATUS {
        unsigned char m_state;   // 0x0
        unsigned char _1[0x20 - 1];
    };

    static STATUS m_BombStatus;   // OVL1 bss 0x4413B0
};

/* whether an action can be taken: none by the dead; the bomb's by its state (planting asks
 * CanPlaceBomb), a breach with a charge and a closed valve, a kind 5 never, any other always
 * (OVL2 0x594350; SOCOM 1's, the life test and kind 5 new).
 * OPEN, 13 instructions: the target reads the bomb's state again for the 1-or-3 test, through its
 * address built in the first test's delay slot (lbu 0(a3)); MW here reuses the first load (SOCOM
 * 1's code has the same reload). Tried: the first test through inline accessors (a parameter, an
 * int, a ternary), the second as inline calls, a reference, a pointer, a switch. */
static inline bool IsBombState(unsigned char state)
{
    return CZBombState::m_BombStatus.m_state == state;
}

/* a breach: a charge with ammunition in the kit and the door's valve closed */
static inline bool CanBreach(CZSealBody *seal, CValve *valve)
{
    if (seal && seal->m_kit.HasItem((EQUIP_ITEM)152) && seal->m_kit.ItemHasAmmo((EQUIP_ITEM)152) && valve &&
        valve->m_value == 0)
        return true;
    return false;
}

bool CSealCtrl::CanPerformAction(CNodeAction *action)
{
    CZSealBody *seal = (CZSealBody *)m_entity;

    if (!seal->m_isAlive)
        return false;
    bool can = false;

    switch (action->m_type) {
    case 4:
        if (IsBombState(2))
            can = CanPlaceBomb(action);
        else if (CZBombState::m_BombStatus.m_state == 1 || CZBombState::m_BombStatus.m_state == 3)
            can = false;
        break;
    case 6:
        can = CanBreach(seal, action->m_valve);
        break;
    case 5:
        can = false;
        break;
    default:
        can = true;
        break;
    }
    return can;
}

/* the view range's floor (OVL2 data 0x6506A0, read as a variable) */
extern "C" float data_006506A0;

/* the view range: a given one held between the floor and the camera's reach (95 % of its far
 * plane, or 1000), none taking that reach (OVL2 0x5924B0; SOCOM 1's took any positive range).
 * OPEN, 3 instructions: the target loads the far plane into $f1 and 0.95 into $f0, the product
 * over the constant; here the other way round (SOCOM 1's code is the target's). Tried: the
 * factor on either side, (float) casts, the plane in a local, `*=`, a ternary. */
void CSealCtrl::SetViewRange(float range)
{
    float far = 1000.0f;
    CViewCamera *cam = ((CViewWorld *)zdb::CWorld::m_world)->m_camera;

    if (cam)
        far = 0.95f * cam->m_far;
    if (range <= 0.0f)
        range = far;
    else
        _saturate(&range, data_006506A0, far);
    m_visrange.m_goal = m_visrange.m_value = range;
    m_ec = range;
}

/* a grenade's flight time and launch speed; SOCOM 1's function-local statics, which MW 3.0 lays
 * out initialised, with no guard (OVL2 0x5976E0, 0x5976A0, 0x5975D0) */
float CDynGrenade::ComputeMaxVel(float dist, float height, float *vel)
{
    static float k_rise = 1.0f;
    static float k_cos = 0.707107f;
    float g = -98.0f;
    float v = sqrtf(2.0f * (height + k_rise * dist) / -g);
    float t = dist / (k_cos * v);

    if (vel)
        *vel = v;
    return t;
}

/* the elevation offset: a blend of the two statics, made radians by a scaling statement of its
 * own -- `d *= k;` is the only form that madds the blend into $f0 and loads pi/180 into $f1 after
 * the mula; `return blend * k`, an inline DegToRad, a lerp inline or out of line, a local factor
 * all give SOCOM 1's registers (probes: build/scratch/seal/elev.spec, v7). */
float CDynGrenade::ComputeElevOfs(float t)
{
    static float lo = 10.0f;
    static float hi = 12.0f;
    float d = lo * (1.0f - t) + hi * t;

    d *= 0.017453292f;
    return d;
}

float CDynGrenade::ComputeTimeToImpact(float scale, float b, float c)
{
    float t0, t1;

    return scale * (func_0050DE20(-49.0f, b, c, t0, t1) ? ai::_max(t0, t1) : 0.0f);
}

/* (OVL2 0x598220) */
CSealCtrl::~CSealCtrl()
{
}

/* the mission begun: online, the squad told to take up its places (unless the player is a
 * ghost); the body crouched and the controls given (OVL2 0x597F50; SOCOM 1's did nothing) */
void CSealCtrl::OnMissionStart()
{
    if (theNetwork.m_9) {
        ClearGoal(GOAL_NONE);
        CSealUnit *unit = func_005EF2F0(24);

        if (unit && !data_00437CE8->m_ghost)
            unit->IssueCommand((CZSealBody *)m_entity, (FT_COMMAND)35, 0, false);
    }
    ((CZSealBody *)m_entity)->m_desiredState = stateCrouch;
    m_input_enabled = true;
}

/* the mission over: no goal, no controls, the motors stopped (OVL2 0x597EB0; SOCOM 1's did
 * nothing) */
void CSealCtrl::OnMissionEnd()
{
    ClearGoal(GOAL_NONE);
    m_input_enabled = false;
    StopRumbler();
}

extern "C" {
/* (OVL1 0x30E6E0, unowned: told of a death online, with the global at 0x45B718) */
void func_0030E6E0(void *obj, int a);
extern void *data_0045B718;
/* (OVL2 0x552780, seal.cpp's: the body told something with two times) */
void func_00552780(CZSealBody *seal, int a, float b, float c);
}

/* dead: the squad told, the death cry (none for a ghost), the restraint undone; online, the squad
 * ordered on unless the game holds the body back, which then waits (OVL2 0x5979A0; SOCOM 1's with
 * SOCOM II's network ending) */
void CSealCtrl::OnDeath()
{
    m_unit->Notify((CZSealBody *)m_entity, UNIT_MESSAGE_DEATH, 0);
    switch (((CZSealBody *)m_entity)->m_deathType) {
    case DEATH_KNIFE_KILL:
    case DEATH_CQB:
    case DEATH_HEADSHOT:
        PlayAiSound((CHARACTER_SOUND)61, 1.0f, -1);
        break;
    case DEATH_GHOST:
        break;
    default:
        PlayAiSound((CHARACTER_SOUND)60, 1.0f, -1);
        break;
    }
    CZSealBody *seal = (CZSealBody *)m_entity;

    seal->SetRestrained(false);
    if (theNetwork.m_9) {
        if (!data_00437CE8->m_dc || data_00437CE8->m_ghost) {
            func_0030E6E0(data_0045B718, 0);
            func_005EF2F0(24)->IssueCommand(seal, (FT_COMMAND)34, 0, false);
        } else
            func_00552780(seal, 2, 0.0f, 0.1f);
    }
}

/* a random value in a range (SOCOM 1's Rfloat; seal_ai.cpp holds its operator's out-of-line copy) */
class Rfloat {
public:
    Rfloat(float min, float range) : m_min(min), m_range(range) {}
    operator float() const
    {
        if (m_range == 0.0f)
            return m_min;
        float r = 4.656613e-10f * (float)rand();

        return m_min + m_range * r;
    }

    float m_min;   // 0x0
    float m_range;   // 0x4
};

/* whether a chance comes up (an inline: seal_ai.cpp holds its out-of-line copy, 0x5608B0) */
static inline bool Chance(float chance)
{
    const float &c = chance;

    return Rfloat(0.0f, 1.0f) <= c;
}

extern "C" {
/* CCharacterType::PlayAt (SOCOM 1's PlayAt__14CCharacterTypeF15CHARACTER_SOUNDUiPC6CPnt3D, which
 * SOCOM II's returns a bool from; OVL2 0x53B710, the AI-core lane's) */
bool func_0053B710(CCharacterType *type, CHARACTER_SOUND sound, unsigned int id, const CPnt3D *pos);
/* the voice's AI event made where the SEAL stands (OVL2 0x55FEE0, seal_ai.cpp's) */
void func_0055FEE0(CSealCtrl *ctrl, int event);
}

/* a sound of the SEAL's character played where it stands, if the chance comes up, and its AI
 * event made; whether it played (OVL2 0x5978A0; SOCOM 1's played it always) */
bool CSealCtrl::PlayAiSound(CHARACTER_SOUND sound, float chance, int event)
{
    bool played;
    CZSealBody *seal = (CZSealBody *)m_entity;
    bool go = true;

    played = false;
    if (chance < 1.0f)
        go = Chance(chance);
    if (go) {
        played = func_0053B710(seal->GetCharacter(), sound, seal->m_id, &seal->m_posW);
        func_0055FEE0(this, event);
    }
    return played;
}

/* hurt: the pain cry, and the pad shaken unless the SEAL is a remote one (OVL2 0x597B00) */
void CSealCtrl::OnSetHealth(float health, float old)
{
    PlayAiSound((CHARACTER_SOUND)59, 1.0f, -1);
    if (IsRemote())
        return;
    SetLowRumbleTime(0.18f, 175.0f);
    SetHighRumbleTime(0.04f);
}

/* the trigger stepped with its pressure (OVL2 0x597810) */
void CSealCtrl::SetTrigger(bool down, float power)
{
    m_trigger_power = power;
    SetTrigger(down);
}

/* the trigger's edge state stepped by the button (OVL2 0x597790) */
void CSealCtrl::SetTrigger(bool down)
{
    TRIGGER t = m_trigger;

    switch (m_trigger) {
    case TRIGGER_UP:
        t = down ? TRIGGER_FALLING : TRIGGER_UP;
        break;
    case TRIGGER_DOWN:
        t = down ? TRIGGER_DOWN : TRIGGER_RISING;
        break;
    case TRIGGER_FALLING:
        t = down ? TRIGGER_DOWN : TRIGGER_RISING;
        break;
    case TRIGGER_RISING:
        t = down ? TRIGGER_FALLING : TRIGGER_UP;
        break;
    }
    m_trigger = t;
}

/* a menu opened or closed; closing them all restarts the weapon's raise unless told to keep it
 * (OVL2 0x597740: SOCOM II's second parameter) */
void CSealCtrl::SetMenuState(MENU_STATE state, bool keep)
{
    if (state < stateLast) {
        if (state != m_menustate && state == stateNone && !keep)
            ((CZSealBody *)m_entity)->m_wep_upT = data_006505E8;
        m_menustate = state;
    }
}

/* the reticle's actions: the squad asked whether orders 4 and 5 are allowed now, actions 14 and
 * 15 offered if so (OVL2 0x593D00; SOCOM 1's without its flag and the reticle seal's tail). The
 * squad declared first: the SEAL takes s0, the squad s1, as the target's. */
void CSealCtrl::PerformReticleActions()
{
    CSealUnit *unit;
    CZSealBody *seal = (CZSealBody *)m_entity;

    unit = func_005EF2F0(17);

    if (unit) {
        if (unit->AllowCommand(seal, (FT_COMMAND)4, 0))
            seal->ActionAvailableMask(14, true);
        if (unit->AllowCommand(seal, (FT_COMMAND)5, 0))
            seal->ActionAvailableMask(15, true);
    }
}

/* the scope's zoom (OVL2 0x592C70, 0x592C30, 0x592C10, 0x592BF0) */
void CSealCtrl::SetCameraZoom(float level, float increment)
{
    m_zoomlevel = level;
    m_zoomincrement = increment;
    m_update_zoom = true;
}

bool CSealCtrl::UpdateHudWeapon()
{
    if (m_HudUpdateWeapon) {
        m_HudUpdateWeapon = false;
        return true;
    }
    return false;
}

float CSealCtrl::GetZoomLevel()
{
    m_update_zoom = false;
    return m_zoomlevel;
}

float CSealCtrl::GetZoomIncrement()
{
    m_update_zoom = false;
    return m_zoomincrement;
}

/* the rumble switch (OVL2 0x592BE0, 0x592BD0) */
void SetRumbleOn(bool on)
{
    data_00650698 = on;
}

bool IsRumbleOn()
{
    return data_00650698;
}

/* the motors: the strong one's time, the weak one's time and strength, kept at their longest
 * (OVL2 0x592BA0, 0x592B40) */
void CSealCtrl::SetHighRumbleTime(float t)
{
    if (data_00650698) {
        if (m_highrumbletime < t)
            m_highrumbletime = t;
    }
}

void CSealCtrl::SetLowRumbleTime(float t, float strength)
{
    if (data_00650698) {
        if (m_rumblestr < strength || m_rumbletime <= 0.0f)
            m_rumblestr = strength;
        if (m_rumbletime < t)
            m_rumbletime = t;
    }
}

/* the motors run down: the weak one at its strength while its time lasts, the strong one on or
 * off (OVL2 0x5929A0) */
void CSealCtrl::TickRumbler(float dT)
{
    CPad *pad = m_padid < CInput::NUM_PADS ? CInput::m_pads[m_padid] : 0;

    if (pad) {
        if (m_rumbletime > 0.0f) {
            pad->m_ActData[1] = m_rumblestr;
            m_rumbletime -= dT;
        } else {
            m_rumblestr = 0.0f;
            pad->m_ActData[1] = m_rumblestr;
        }
        if (m_highrumbletime > 0.0f) {
            pad->m_ActData[0] = 1;
            m_highrumbletime -= dT;
        } else
            pad->m_ActData[0] = 0;
    }
}

/* the motors stopped, the pad told at once, the input flushed and the heartbeat silenced
 * (OVL2 0x592AC0) */
void CSealCtrl::StopRumbler()
{
    CPad *pad = m_padid < CInput::NUM_PADS ? CInput::m_pads[m_padid] : 0;

    if (pad) {
        pad->m_ActData[1] = 0;
        pad->m_ActData[0] = 0;
        pad->Tick(0.0f);
    }
    m_highrumbletime = 0.0f;
    m_rumbletime = 0.0f;
    m_rumblestr = 0.0f;
    CInput::Flush();
    CZSealBody::StopAllHeartbeatSounds();
}

/* the MP bomb's timer offered (OVL2 0x5920A0) */
void CSealCtrl::MPBomb_TimerAvailable(bool on)
{
    m_mpbomb_bTimerAvailable = on;
}
