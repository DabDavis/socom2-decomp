/* ent_ctrl.cpp -- the entity's per-tick motion and its controller, CEntityCtrl (OVL1
 * 0x2A2510-0x2A27DF). After ent_target.cpp's template instances, before the path registry; SOCOM
 * 1 has the same functions in the same order between the same neighbours (0x1F1E20-0x1F20E3),
 * in a unit with no static initialiser, so no name survives: the file name is ours. Defined in
 * reverse of their image order (-inline deferred). */

#include <string.h>

#include "zentity.h"

/* the controller: nothing to drive yet (SOCOM 1's) */
CEntityCtrl::CEntityCtrl(int)
{
    m_entity = 0;
    memset(m_throttle, 0, sizeof(m_throttle));
}

CEntityCtrl::~CEntityCtrl()
{
}

bool CEntityCtrl::PreTick(float dT)
{
    return false;
}

bool CEntityCtrl::Tick(float dT)
{
    return false;
}

/* the controller handed over: the old one let go, the new one told whose it is (SOCOM 1's) */
void CEntity::SetController(CEntityCtrl *ctrl)
{
    if (m_control)
        m_control->m_entity = 0;
    m_control = ctrl;
    if (m_control)
        m_control->m_entity = this;
}

bool CEntityCtrl::PostMortemPreTick(float dT)
{
    return true;
}

/* the ticks an entity takes no part in (SOCOM 1's, the same order) */
void CEntity::PreTick(float dT)
{
}

void CEntity::Tick(float dT)
{
}

void CEntity::PostPreTick(float dT)
{
}

void CEntity::PrePostTick(float dT)
{
}

void CEntity::PostTick(float dT)
{
}

/* the translation row set from three floats (the name is ours; ent_main.cpp has one too) */
static inline void SetTranslate(CMatrix &m, float x, float y, float z)
{
    m.m_matrix[3][0] = x;
    m.m_matrix[3][1] = y;
    m.m_matrix[3][2] = z;
}

/* the next frame's orientation, velocity and matrix from this one's: the spin half a step through
 * the quaternion exponential, the model velocity turned into the world, the position moved on --
 * the vertical velocity held out of it and put back (SOCOM 1's) */
void CEntity::ComputeNextPosition(float dT)
{
    float vy = m_velM.y;

    m_velW.y = 0.0f;
    m_velM.y = 0.0f;
    m_next_quat = m_quat;

    CPnt3D spin;
    CPnt3D step;
    CPnt3D pos;
    CQuat dq;
    CQuat q;

    m_velR.Scale(0.5f * dT, &spin);
    dq.Exp(&spin);
    dq.Mul(&m_next_quat, &q);
    q.Normalize(&m_next_quat);
    m_next_quat.Apply(&m_velM, &m_next_velw, 1);
    m_next_velw.Scale(dT, &step);
    ((CPnt3D *)m_node->m_matrix.m_matrix[3])->Add(&step, &pos);
    SetTranslate(m_next_matrix, pos.x, pos.y, pos.z);
    m_next_quat.ToMatrix(&m_next_matrix);
    m_velW.y = vy;
    m_velM.y = vy;
}

/* the next frame's state made this one's (SOCOM 1's) */
void CEntity::UpdateNextPosition()
{
    m_quat = m_next_quat;
    m_velW = m_next_velw;
    m_node->SetMatrix(&m_next_matrix);
}
