/* seal_cfg.cpp -- the SEAL's configuration: its gear, its animation set's limits (SOCOM 1's unit,
 * __sinit_seal_cfg.cpp; its strings put SetupGear there, GetMaxXMotion and GetMaxZMotion after
 * it). OVL2 before seal_command.cpp (0x58FBE0). Defined in reverse of their image order
 * (-inline deferred). */

#include "zseal.h"

/* an animation set's speeds (SOCOM 1's DWARF: ANIMSET_PARAMS, 0x34 bytes) */
struct ANIMSET_PARAMS {
    float m_stand_max_fwd;   // 0x0
    float m_stand_max_back;   // 0x4
    float m_stand_max_rt;   // 0x8
    float m_stand_max_left;   // 0xc
    float m_crouch_max_fwd;   // 0x10
    float m_crouch_max_back;   // 0x14
    float m_crouch_max_rt;   // 0x18
    float m_crouch_max_left;   // 0x1c
    float m_prone_max_fwd;   // 0x20
    float m_prone_max_back;   // 0x24
    float m_prone_max_rt;   // 0x28
    float m_prone_max_left;   // 0x2c
    float m_ladder_updn;   // 0x30
};

/* a SEAL's animations (SOCOM 1's DWARF: AnimSet, its speeds first) -- only those here */
class AnimSet {
public:
    ANIMSET_PARAMS m_aparams;   // 0x0
};

/* the forward and backward limits for the state: standing, crouched, prone or on a ladder (OVL2
 * 0x58BC00, 0x58BB50; SOCOM 1's, the state now CBody's halfword) */
void CZSealBody::GetMaxZMotion(float &a, float &b)
{
    AnimSet *set = m_animset;

    switch (m_state) {
    case stateStand:
        a = set->m_aparams.m_stand_max_fwd;
        b = set->m_aparams.m_stand_max_back;
        break;
    case stateCrouch:
        a = set->m_aparams.m_crouch_max_fwd;
        b = set->m_aparams.m_crouch_max_back;
        break;
    case stateProne:
        a = set->m_aparams.m_prone_max_fwd;
        b = set->m_aparams.m_prone_max_back;
        break;
    case stateClimbLadder:
        a = set->m_aparams.m_ladder_updn;
        b = set->m_aparams.m_ladder_updn;
        break;
    default:
        a = 0.0f;
        b = 0.0f;
        break;
    }
}

void CZSealBody::GetMaxXMotion(float &a, float &b)
{
    AnimSet *set = m_animset;

    switch (m_state) {
    case stateStand:
        a = set->m_aparams.m_stand_max_left;
        b = set->m_aparams.m_stand_max_rt;
        break;
    case stateCrouch:
        a = set->m_aparams.m_crouch_max_left;
        b = set->m_aparams.m_crouch_max_rt;
        break;
    case stateProne:
        a = set->m_aparams.m_prone_max_left;
        b = set->m_aparams.m_prone_max_rt;
        break;
    case stateClimbLadder:
        a = set->m_aparams.m_ladder_updn;
        b = set->m_aparams.m_ladder_updn;
        break;
    default:
        a = 0.0f;
        b = 0.0f;
        break;
    }
}
