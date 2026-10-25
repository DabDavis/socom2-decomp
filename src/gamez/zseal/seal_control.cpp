/* seal_control.cpp -- the SEAL's controller, CSealCtrl: the pad's throttles, the trigger, the
 * rumble, the zoom, the menus (SOCOM 1's unit, __sinit_seal_control.cpp; SOCOM 1's order, the
 * MP bomb's helpers first and the constructor last). OVL2 0x5920A0-0x598840. Defined in reverse
 * of their image order (-inline deferred). */

#include "zseal.h"
#include "../zinput/zinput.h"

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

/* a straight blend (out of line at 0x597630; the name ours) */
inline float lerpf(float a, float b, float t)
{
    return a * (1.0f - t) + b * t;
}

/* OPEN, 4 instructions: the target loads pi/180 into $f1 after the mula and madds the blend into
 * $f0 (`mtc1 v0,$f1; nop; madd.s $f0,$f0,$f12`), where every form tried -- this one, the blend
 * written out, the factor first, either in a local, the blend in a local, the terms swapped
 * (0x38 bytes) -- madds into $f1 and loads the factor into $f0: SOCOM 1's register choice. */
float CDynGrenade::ComputeElevOfs(float t)
{
    static float lo = 10.0f;
    static float hi = 12.0f;

    return lerpf(lo, hi, t) * 0.017453292f;
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

/* OnMissionStart and OnMissionEnd are byte-exact by scdiff --target 0x597F50 / 0x597EB0, but the
 * map still gives their names to CRemoteCtrl's own overrides (0x5E6270/0x5E6280, the 8-byte
 * stubs in __vt__11CRemoteCtrl's slots 0x5C/0x60, which a draft matches): defined here they would
 * score against those rows. Held out until those rows are OnMissionStart/End__11CRemoteCtrlFv
 * and 0x597F50/0x597EB0 take these names (the lane's report). */
#ifdef SC_SEAL_REMOTE_RENAMED

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

#endif

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
