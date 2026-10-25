/* ent_main.cpp -- CEntity: the entity lists, the recycler, the ticks over every entity, and the
 * entity's own small virtuals (OVL1 0x29E470-0x2A011F; SOCOM 1's unit, __sinit_ent_main.cpp,
 * building the recycler and the two lists, 0x3FFFB0). Defined in reverse of their image order
 * (-inline deferred). */

#include "zentity.h"


/* an entity's id is the count of those made before it (SOCOM 1's, in $gp small data there) */
static unsigned int s_next_id;

/* the empty virtuals (SOCOM 1's, the same order) */
void CEntity::ResetToInitialState()
{
}

void CEntity::OnMissionStart()
{
}

void CEntity::OnMissionEnd()
{
}

/* the position copied out of the node's matrix (the name is ours) */
void CEntity::UpdatePosW()
{
    m_posW = *(CPnt3D *)m_node->m_matrix.m_matrix[3];
}

void CEntity::TeleportTo(const CMatrix *m)
{
    m_velW = CPnt3D::zero;
    m_velM = CPnt3D::zero;
    m_velR = CPnt3D::zero;
    m_next_velw = CPnt3D::zero;
    m_next_matrix = *m;
    m->ToQuat(&m_quat);
    m_quat.Normalize();
    m_next_quat = m_quat;
    m_node->SetMatrix(&m_next_matrix);
    UpdatePosW();
    UpdateAiMapLoc();
}

/* the translation row set from three floats (the name is ours) */
static inline void SetTranslate(CMatrix &m, float x, float y, float z)
{
    m.m_matrix[3][0] = x;
    m.m_matrix[3][1] = y;
    m.m_matrix[3][2] = z;
}

void CEntity::TeleportTo(const CPnt3D *pos, const CQuat *quat)
{
    CMatrix m(*m_node->GetMatrix());

    m_velW = CPnt3D::zero;
    m_velM = CPnt3D::zero;
    m_velR = CPnt3D::zero;
    m_next_velw = CPnt3D::zero;
    if (pos)
        SetTranslate(m, pos->x, pos->y, pos->z);
    if (quat) {
        m_next_quat = *quat;
        m_quat = *quat;
        quat->ToMatrix(&m);
    }
    m_next_matrix = m;
    m_node->SetMatrix(&m);
    UpdatePosW();
    UpdateAiMapLoc();
}

CPnt3D *CEntity::GetTargetPosW(CPnt3D *out)
{
    *out = m_posW;
    return out;
}

CPnt3D *CEntity::GetHeadPosW(CPnt3D *out)
{
    *out = m_posW;
    return out;
}

CPnt3D *CEntity::GetNeckPosW(CPnt3D *out)
{
    *out = m_posW;
    return out;
}

void CEntity::UpdateAiMapLoc()
{
}

void CEntity::OnRecycleEntity(CEntity *e)
{
}

void CEntityCtrl::OnDeath()
{
}

void CEntity::OnDeath()
{
    m_isAlive = false;
    m_control->OnDeath();
}

void CEntity::SetDisplayName(const char *name)
{
    if (m_display_name)
        _free(m_display_name, 0, 0);
    m_display_name = 0;
    m_display_name = name ? _strdup(name, 0, 0) : 0;
}

/* OPEN, 5 instructions: the registers. The target loads m_team into a2 and builds the bit in v1
 * (`li v1,1; sllv v1,v1,a1`), MW here the other way round (`lw v1; li a2,1; sllv a1,a2,a1`) --
 * whatever the form: |=, `m = m | bit`, a local for either, an inline setter or bit helper, 1U,
 * `bit <<= team`, an early return, int m_team; 3.0b22/b38/b50 alike. SOCOM 1's (0x1EE760) is the
 * target's code word for word */
void CEntity::JoinTeam(unsigned int team)
{
    if (team < 32)
        m_team |= 1 << team;
}

void CEntity::SetTeamMask(unsigned int mask)
{
    m_team = mask;
}

zdb::CNode *CEntity::GetNode()
{
    return m_node;
}

int CEntity::GetEntityIndex(const CEntity *e)
{
    return e ? e->m_index : -1;
}

/* whether an entity is still in the world. The iterator is assigned from begin(), so it stays in
 * memory (the copy is bitwise); the end is taken into a local first -- with end() in the test, as
 * SOCOM 1 wrote it, the sentinel and the first node trade registers (CZNewHudMap's finders, a
 * member list, match that way) */
bool CEntity::EntityIsValid(CEntity *e)
{
    std::list<CEntity *>::iterator it;
    std::list<CEntity *>::iterator end = m_list.end();

    for (it = m_list.begin(); it != end; ++it)
        if (e == *it)
            return true;
    return false;
}
