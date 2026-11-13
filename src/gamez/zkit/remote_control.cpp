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
bool AllowCommandFOLLOW__9CSealCtrlFP10CZSealBody(CSealCtrl *ctrl, CZSealBody *seal);
bool AllowCommandHOLDPOS__9CSealCtrlFP10CZSealBody(CSealCtrl *ctrl, CZSealBody *seal);
/* NetSendAiOrders (SOCOM 1's NetSendAiOrders__FUiUiUiPv; OVL1 0x2BB970), SOCOM II's call passing
 * no data */
void NetSendAiOrders__FUiUiUiPv(unsigned int seal, unsigned int from, unsigned int cmd, void *data);
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
    unsigned char _1[2];
    unsigned char m_opacity : 4;   // 0x3: the body's opacity in fifteenths (15: hidden? the name ours)
};

/* jumping, as the other console says -- only while in a vehicle's seat? (OVL2 0x5E6A50; SOCOM
 * II's GetJumpFlag, moved into CSealCtrl's table) */
AI_STATE CRemoteCtrl::GetJumpFlag() const
{
    bool jump = false;

    if (m_entity->m_vehicle_index) {
        AI_NET_STATE s;
        unsigned int state = GetAiNetState();

        memcpy(&s, &state, sizeof(s));
        jump = s.m_jump;
    }
    return (AI_STATE)jump;
}

/* an order to follow or to hold, passed on to the console that drives the SEAL (OVL2 0x5E6A00) */
void CRemoteCtrl::OnCommand(CZSealBody *seal, FT_COMMAND cmd, void *data, bool b)
{
    switch (cmd) {
    case 4:
    case 5:
        NetSendAiOrders__FUiUiUiPv(seal->m_id, m_entity->m_id, cmd, 0);
    }
}

/* only follow and hold are taken (OVL2 0x5E69A0) */
bool CRemoteCtrl::AllowCommand(CZSealBody *seal, FT_COMMAND cmd, void *data)
{
    bool ok = false;

    switch (cmd) {
    case 4:
        ok = AllowCommandFOLLOW__9CSealCtrlFP10CZSealBody(this, seal);
        break;
    case 5:
        ok = AllowCommandHOLDPOS__9CSealCtrlFP10CZSealBody(this, seal);
        break;
    }
    return ok;
}

/* the network game (OVL1 bss 0x437CE8, the HUD lane's reading): only the rate the remote SEALs ease
 * toward the sent position by (SOCOM 1's at 0xe0; the view and the name ours) */
class CNetGameEase {
public:
    unsigned char _0[0x118];
    float m_ease_rate;   // 0x118
};

extern CNetGameEase *data_00437CE8;

extern "C" {
float expf(float);
/* whether the SEAL's top blend is 8 or 9 (OVL2 0x587F50, the SEAL lane's row; the name ours) */
bool func_00587F50(CZSealBody *seal);
/* the firepoint (OVL2 0x5DF370, seal_weapon.cpp's) */
bool GetFP__10CZSealBodyFR6CPnt3D(CZSealBody *seal, CPnt3D &fp);
/* the kit's current item fired (SOCOM 1's CZKit::FireCur; OVL2 0x5C6510) */
CZProjectile *FireCur__5CZKitFPC6CPnt3DPC6CPnt3DPC6CPnt3D(CZKit *kit, const CPnt3D *pos, const CPnt3D *vel, const CPnt3D *v);
}

/* the remote SEAL moved toward where its console last said it was: snapped when far off or a
 * floor away, eased by a factor taken once per network tick otherwise; its opacity and the shots
 * it was told of played out (OVL2 0x5E6400; SOCOM 1's PreTick, 0x2CE310 in the demo, with the
 * collision bits, the far snap's altitude reset and the turret's firepoint new). Byte-exact (the
 * kit's round 63, unwritten before): `node` declared first and assigned at the GetNode gives the
 * target's saved registers (this s4, node s3, seal s2, pos s1 -- declared at its use, node and pos
 * swap); the eased factor read ahead of the two throttles gives its load in the first branch's slot;
 * the aim negated through a `const CPnt3D &` keeps its first reads frame-direct and only the stores
 * through the address common subexpressions (a1 y, a2 z), as the target; the sqrt ahead of the aim's
 * copy; the quaternion copied by the implicit assignment (a member-wise vec/w pair interleaves) */
bool CRemoteCtrl::PreTick(float dT)
{
    static int s_time = 0;
    static float s_ease;
    zdb::CNode *node;
    CZSealBody *seal = (CZSealBody *)m_entity;
    CPnt3D *pos = &m_net_pos;
    float y = m_net_pos.y;
    float rate = data_00437CE8->m_ease_rate;

    seal->m_check_player_collision = true;
    seal->m_get_new_altitude = false;
    if (s_time != theNetwork.zNetGetTime()) {
        s_time = theNetwork.zNetGetTime();
        s_ease = expf(-dT * rate);
    }

    AI_NET_STATE s;
    unsigned int state = m_ai_net_state;

    memcpy(&s, &state, sizeof(s));
    node = seal->GetNode();

    if (node) {
        node->SetActive(s.m_opacity != 15);
        node->m_Opacity = 1.0f - s.m_opacity / 15.0f;
    }

    CPnt3D p;
    CPnt3D d;

    pos->Sub(&seal->m_posW, &d);
    if (d.x * d.x + d.y * d.y + d.z * d.z > 1000.0f) {
        seal->TeleportTo(pos, &m_net_quat);
    } else {
        if (fabsf(y - seal->m_posW.y) > 5.0f) {
            seal->m_bit8 = true;
            seal->GetNode()->SetPosition(pos->x, 0.25f + pos->y, pos->z);
            seal->UpdatePosW();
            seal->ResetAltitude(0.25f + pos->y);
            seal->m_last_altitude.m_pos = CPnt3D::zero;
            seal->m_last_altitude.m_norm = CPnt3D::zero;
            seal->m_last_altitude.m_poly = 0;
            seal->m_last_altitude.m_node = 0;
            seal->m_last_altitude.m_pos = *pos;
            seal->m_last_altitude.m_norm = CPnt3D::unit_y;
            if (seal->m_alt < seal->m_posW.y)
                seal->m_alt = seal->m_posW.y;
        }

        float t = s_ease;
        float tx = m_throttle[0];
        float tz = m_throttle[1];

        if (tx == 0.0f && tz == 0.0f)
            t = 1.0f;

        seal->m_posW.Interp(pos, t, &p);
        if (func_00587F50(seal))
            p.y = y;
        seal->GetNode()->SetPosition(p.x, p.y, p.z);
        seal->UpdatePosW();
    }

    float t = 2.0f * s_ease;

    if (t > 1.0f)
        t = 1.0f;
    else if (t < 0.0f)
        t = 0.0f;

    CQuat q;

    seal->m_quat.SlerpFaster(&m_net_quat, t, &q);
    seal->m_quat = q;

    float pitch = *(float *)&seal->m_228;
    float c = sqrtf(1.0f - pitch * pitch);
    CPnt3D aim = *(const CPnt3D *)seal->m_node->m_matrix.m_matrix[2];

    const CPnt3D &ca = aim;

    aim.x = -ca.x;
    aim.y = -ca.y;
    aim.z = -ca.z;
    aim.x *= c;
    aim.z *= c;
    aim.y = pitch;

    CPnt3D dir(0.0f, pitch, -c);

    seal->AimWeapon();
    seal->SetAimDirR(&dir, false);
    if (seal->m_224) {
        seal->m_224--;

        CPnt3D fp;

        if (seal->m_eb8) {
            seal->m_eb8->Virtual60(&fp);
            seal->m_eb8->Virtual68(&aim);
            FireCur__5CZKitFPC6CPnt3DPC6CPnt3DPC6CPnt3D(&seal->m_kit, &fp, &aim, &m_entity->m_velW);
        } else if (GetFP__10CZSealBodyFR6CPnt3D(seal, fp)) {
            FireCur__5CZKitFPC6CPnt3DPC6CPnt3DPC6CPnt3D(&seal->m_kit, &fp, &aim, &m_entity->m_velW);
        }
    }
    return true;
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
