/* seal.cpp -- the SEAL's body, CZSealBody: its ticks, its states, its actions, its stats (SOCOM 1's
 * unit, __sinit_seal.cpp; the press beta's asserts file OpenHeartbeatSounds and
 * ExecuteDelayedAction_Jump under it). OVL2 0x542E50-0x5552BF in SOCOM 1's order -- the heartbeat
 * first, the constructor and SealInitCharacterDynamics last, then the template tail. Defined in
 * reverse of their image order (-inline deferred). */

#include "zseal.h"

class CPickup {
public:
    static CPickup *FindPickup(unsigned int id);
    static void DestroyNetPickups(unsigned int owner);
};

extern "C" {
void func_005C7840(CZKit *kit);   /* CZKit::PostSetupItems (SOCOM 1's OnMissionStart calls it) */
void func_00525200(void *path);   /* (a CAiPath reset: its member at 0x98's, tail-called) */
}

/* the world's camera (zdb::CWorld at 0x128; only that) */
struct CWorldCamera {
    unsigned char _0[0x128];
    zdb::CCamera *m_camera;   // 0x128
};

/* teleported: the altitude and the last ground hit reset to the new place, the snake path
 * cleared; a player's camera sees every region again (OVL2 0x546E40, 0x546CF0; SOCOM II's) */
void CZSealBody::TeleportTo(const CPnt3D *pos, const CQuat *quat)
{
    if (pos) {
        ResetAltitude(pos->y);
        m_last_altitude.m_pos = CPnt3D::zero;
        m_last_altitude.m_norm = CPnt3D::zero;
        m_last_altitude.m_poly = 0;
        m_last_altitude.m_node = 0;
        m_last_altitude.m_pos = *pos;
        m_last_altitude.m_norm = CPnt3D::unit_y;
    }
    func_00525200(m_snakePath);
    CEntity::TeleportTo(pos, quat);
    if (IsPlayer() && zdb::CWorld::m_world) {
        zdb::CCamera *cam = ((CWorldCamera *)zdb::CWorld::m_world)->m_camera;

        if (cam) {
            cam->SetPlayerCanSeeRegions(-1);
            cam->SetCameraCanSeeRegions(-1);
        }
    }
}

void CZSealBody::TeleportTo(const CMatrix *m)
{
    ResetAltitude(m->m_matrix[3][1]);
    m_last_altitude.m_pos = CPnt3D::zero;
    m_last_altitude.m_norm = CPnt3D::zero;
    m_last_altitude.m_poly = 0;
    m_last_altitude.m_node = 0;
    m_last_altitude.m_pos = *(const CPnt3D *)m->m_matrix[3];
    m_last_altitude.m_norm = CPnt3D::unit_y;
    func_00525200(m_snakePath);
    CEntity::TeleportTo(m);
    if (IsPlayer() && zdb::CWorld::m_world) {
        zdb::CCamera *cam = ((CWorldCamera *)zdb::CWorld::m_world)->m_camera;

        if (cam) {
            cam->SetPlayerCanSeeRegions(-1);
            cam->SetCameraCanSeeRegions(-1);
        }
    }
}

/* the altitude taken from here and a new one asked for (OVL2 0x546CD0: SOCOM 1's
 * m_get_new_altitude is the entity's bit 8 now, zentity.h's m_bit8) */
void CZSealBody::ResetAltitude(float y)
{
    m_alt = y;
    m_bit8 = true;
}

/* the mission's start: the kit's items set up, then the controller's (OVL2 0x553E60) */
void CZSealBody::OnMissionStart()
{
    func_005C7840(&m_kit);
    ((CSealCtrl *)m_control)->OnMissionStart();
}

/* the mission's end: the SEAL's network pickups destroyed (SOCOM 1's tested m_id first), then
 * the controller's (OVL2 0x553E20) */
void CZSealBody::OnMissionEnd()
{
    CPickup::DestroyNetPickups(m_id);
    ((CSealCtrl *)m_control)->OnMissionEnd();
}

/* (OVL2 0x553E00) */
void CZSealBody::OnRecycleEntity(CEntity *e)
{
    ((CSealCtrl *)m_control)->OnRecycleEntity(e);
}

/* the delayed actions: jump and climb, taken on the next tick (OVL2 0x5469E0, 0x5469C0) */
bool CZSealBody::ExecuteDelayedAction_Climb()
{
    m_action_climb = true;
    return true;
}

bool CZSealBody::ExecuteDelayedAction_Jump()
{
    m_action_jump = true;
    return true;
}

/* the compass anchors the SEAL carries: swapped in, whether there was none returned (OVL2
 * 0x544C60, 0x544C40) */
bool CZSealBody::CompassAnchor_MPBomb(CCompassAnchor *anchor)
{
    bool none = !m_mpbomb_anchor;

    m_mpbomb_anchor = anchor;
    return none;
}

bool CZSealBody::CompassAnchor_MPBase(CCompassAnchor *anchor)
{
    bool none = !m_mpbase_anchor;

    m_mpbase_anchor = anchor;
    return none;
}

/* the pickups the controller has in reach (OVL2 0x544C30, 0x544C20) */
CPickup *CZSealBody::CurrentPickup()
{
    return CPickup::FindPickup(((CSealCtrl *)m_control)->m_curpickup_ID);
}

CPickup *CZSealBody::CurrentPickupSecondary()
{
    return CPickup::FindPickup(((CSealCtrl *)m_control)->m_curpickup_secondary_ID);
}

/* the new entity virtuals: a SEAL says no to them too (OVL2 0x543020, 0x543010, 0x543000) */
int CZSealBody::Virtual68()
{
    return 0;
}

int CZSealBody::Virtual60()
{
    return 0;
}

int CZSealBody::Virtual5C()
{
    return 0;
}
