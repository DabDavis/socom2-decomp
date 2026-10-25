/* remote_control.cpp -- the controller of a SEAL another console drives, CRemoteCtrl (SOCOM 1's
 * unit, __sinit_remote_control.cpp). OVL2 0x5E6230-0x5E6A9B, in SOCOM 1's order; SOCOM II dropped
 * GetRemoteAiFollowMaster and GetRemoteAiState and added the alive valve, the mission hooks and
 * GetJumpFlag. Defined in reverse of their image order (-inline deferred). */

#include <stdio.h>
#include <string.h>

#include "zkit.h"
#include "../zanim/zanim.h"

/* a valve set, its callbacks told whether it went on or off (CValve::SetValue, inlined in both
 * builds; zanim_menu.h's SetValveValue, repeated) */
inline void SetValveValue(CValve *valve, int value)
{
    VALVE_STATE state = VALVE_SAME;

    if (valve->m_value != value)
        state = value == 0 ? VALVE_OFF : VALVE_ON;
    valve->m_value = value;
    if (state != VALVE_SAME)
        valve->MakeCallbacks(state);
}

extern "C" {
/* CSealCtrl::AllowCommandFOLLOW and AllowCommandHOLDPOS (SOCOM 1's, called from the same switch;
 * OVL2 0x591AB0, 0x591770 -- the SEAL lane's rows) */
bool func_00591AB0(CSealCtrl *ctrl, CZSealBody *seal);
bool func_00591770(CSealCtrl *ctrl, CZSealBody *seal);
/* NetSendAiOrders (SOCOM 1's NetSendAiOrders__FUiUiUiPv; OVL1 0x2BB970), SOCOM II's call passing
 * no data */
void func_002BB970(unsigned int seal, unsigned int from, unsigned int cmd, void *data);
}

/* the mission's clock (theMission at 0x4364E0; only that member -- SOCOM 1's at 0x60) */
class CMission {
public:
    unsigned char _0[0xe0];
    float m_time;   // 0xe0
};

extern CMission theMission;

/* the SEAL's time of death (CZSealBody at 0xFB4, SOCOM 1's at 0xEA8; zseal.h does not name it --
 * the name ours) */
struct CSealDeathTime {
    unsigned char _0[0xfb4];
    float m_deathTime;   // 0xfb4
};

extern "C" const float data_00650AD8;   /* 0.0f, the fade's floor (a file constant) */

/* the controller's network state, as the remote console sent it (SOCOM 1's; only its first byte,
 * the jump, read here -- the name ours) */
struct AI_NET_STATE {
    bool m_jump;   // 0x0
    unsigned char _1[3];
};

/* jumping, as the other console says -- only while in a vehicle's seat? (OVL2 0x5E6A50; SOCOM
 * II's GetJumpFlag, moved into CSealCtrl's table) */
bool CRemoteCtrl::GetJumpFlag() const
{
    bool jump = false;

    if (m_entity->m_vehicle_index) {
        AI_NET_STATE s;
        unsigned int state = GetAiNetState();

        memcpy(&s, &state, sizeof(s));
        jump = s.m_jump;
    }
    return jump;
}

/* an order to follow or to hold, passed on to the console that drives the SEAL (OVL2 0x5E6A00) */
void CRemoteCtrl::OnCommand(CZSealBody *seal, FT_COMMAND cmd, void *data, bool b)
{
    switch (cmd) {
    case 4:
    case 5:
        func_002BB970(seal->m_id, m_entity->m_id, cmd, 0);
    }
}

/* only follow and hold are taken (OVL2 0x5E69A0) */
bool CRemoteCtrl::AllowCommand(CZSealBody *seal, FT_COMMAND cmd, void *data)
{
    bool ok = false;

    switch (cmd) {
    case 4:
        ok = func_00591AB0(this, seal);
        break;
    case 5:
        ok = func_00591770(this, seal);
        break;
    }
    return ok;
}

/* (OVL2 0x5E6980) */
bool CRemoteCtrl::Tick(float dT)
{
    ClearGoal(GOAL_NONE);
    return true;
}

/* a dead remote SEAL's body faded out over two seconds and then hidden, once its id is gone
 * (OVL2 0x5E6310; SOCOM 1's, the same code) */
bool CRemoteCtrl::PostMortemPreTick(float dT)
{
    zdb::CNode *node;
    CEntity *seal = m_entity;

    if (seal->m_id == CEntity::invalidId) {
        node = seal->GetNode();

        if (node->m_active) {
            m_fade_time = 2.0f;
            m_fade_min = 0.001f;
            float t = theMission.m_time - ((CSealDeathTime *)seal)->m_deathTime;

            _saturate(&t, data_00650AD8, m_fade_time);
            node->m_Opacity = 1.0f - t / m_fade_time;
            if (node->m_Opacity <= m_fade_min) {
                node->m_Opacity = 0.0f;
                node->SetActive(false);
            }
        }
    }
    return true;
}

/* (OVL2 0x5E6300) */
bool CRemoteCtrl::PostTick(float dT)
{
    return true;
}

/* dead: the SEAL's "<node>_valve_alive" valve shut (OVL2 0x5E6290; SOCOM 1's did nothing) */
void CRemoteCtrl::OnDeath()
{
    char name[100];

    sprintf(name, "%s_valve_alive", m_entity->m_node->m_name);
    CValve *valve = CValve::GetByName(name);

    if (valve)
        SetValveValue(valve, 0);
}

/* the mission hooks, empty (OVL2 0x5E6280, 0x5E6270) */
void CRemoteCtrl::OnMissionEnd()
{
}

void CRemoteCtrl::OnMissionStart()
{
}

/* (OVL2 0x5E6260, 0x5E6250, 0x5E6240, 0x5E6230) */
bool CRemoteCtrl::IsRemote() const
{
    return true;
}

bool CRemoteCtrl::IsPlayer() const
{
    return false;
}

void CRemoteCtrl::OnStunDamage(STUN_TYPE type, float amount, const CPnt3D *from)
{
}

CEntityCtrl *CRemoteCtrl::GetRemoteControl()
{
    return this;
}
