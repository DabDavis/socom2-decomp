/* seal_control.cpp -- the SEAL's controller, CSealCtrl: the pad's throttles, the trigger, the
 * rumble, the zoom, the menus (SOCOM 1's unit, __sinit_seal_control.cpp; SOCOM 1's order, the
 * MP bomb's helpers first and the constructor last). OVL2 0x5920A0-0x598840. Defined in reverse
 * of their image order (-inline deferred). */

#include "zseal.h"
#include "../zinput/zinput.h"

extern "C" {
extern bool data_00650698;   /* the rumble switch (SOCOM 1's file static) */
extern float data_006505E8;  /* (the weapon's raise time a closed menu restarts) */
}

/* (OVL2 0x598220) */
CSealCtrl::~CSealCtrl()
{
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
