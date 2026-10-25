/* ent_main.cpp -- CEntity: the entity lists, the recycler, the ticks over every entity, and the
 * entity's own small virtuals (OVL1 0x29E470-0x2A011F; SOCOM 1's unit, __sinit_ent_main.cpp,
 * building the recycler and the two lists, 0x3FFFB0). Defined in reverse of their image order
 * (-inline deferred). */

#include <string.h>

#include "zentity.h"

/* an entity's id is the count of those made before it (SOCOM 1's, in $gp small data there) */
static unsigned int s_next_id;

/* the place the next entity takes in the list, or -1 for the end (SOCOM 1's recycler_index) */
static int recycler_index = -1;

void CEntity::OpenRecycler(unsigned int index)
{
    recycler_index = index;
}

void CEntity::CloseRecycler()
{
    recycler_index = -1;
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

/* an entity into the world: at the end of the list, or into the place the recycler holds open
 * (SOCOM 1's, which returned nothing either) */
void CEntity::CreateEntity(CEntity *e, bool include)
{
    if (recycler_index == -1) {
        e->m_index = m_list.size();
        m_list.push_back(e);
        e->SetIncludeInTargetList(include);
        e->UpdatePosW();
    } else
        e->m_index = recycler_index;
}

/* an entity out of the world: queued for the recycler, out of the target lists at once */
bool CEntity::RecycleEntity(CEntity *e)
{
    m_recycle.push_back(e);
    e->SetIncludeInTargetList(false);
    return true;
}

/* every entity told of the tick (SOCOM 1's PreTickAll, TickAll, PostTickAll, and SOCOM II's
 * entity_preposttick between the last two -- CCoreState::Init schedules all four by name). The
 * walks assign a const_iterator from the const list's begin(): the iterator lives in memory */
bool CEntity::PreTickAll(float dT, void *data)
{
    if (m_recycle.size())
        DoRecycleEntity(m_recycle.front());
    UpdateTargetList(false);

    CEntityList::const_iterator it;

    for (it = GetEntityList().begin(); it != GetEntityList().end(); ++it)
        (*it)->PreTick(dT);
    return false;
}

bool CEntity::TickAll(float dT, void *data)
{
    const CEntityList &list = GetEntityList();

    if (list.size()) {
        CEntityList::const_iterator it;

        for (it = list.begin(); it != list.end(); ++it)
            (*it)->Tick(dT);
    }
    return false;
}

bool CEntity::PrePostTickAll(float dT, void *data)
{
    const CEntityList &list = GetEntityList();

    if (list.size()) {
        CEntityList::const_iterator it;

        for (it = list.begin(); it != list.end(); ++it)
            (*it)->PostPreTick(dT);
    }
    return false;
}

bool CEntity::PostTickAll(float dT, void *data)
{
    const CEntityList &list = GetEntityList();

    if (list.size()) {
        CEntityList::const_iterator it;

        for (it = list.begin(); it != list.end(); ++it)
            (*it)->PrePostTick(dT);
        for (it = list.begin(); it != list.end(); ++it) {
            CEntity *e = *it;

            e->PostTick(dT);
            e->UpdatePosW();
        }
    }
    return false;
}

int CEntity::GetEntityIndex(const CEntity *e)
{
    return e ? e->m_index : -1;
}

zdb::CNode *CEntity::GetNode()
{
    return m_node;
}

void CEntity::SetTeamMask(unsigned int mask)
{
    m_team = mask;
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

void CEntity::SetDisplayName(const char *name)
{
    if (m_display_name)
        _free(m_display_name, 0, 0);
    m_display_name = 0;
    m_display_name = name ? _strdup(name, 0, 0) : 0;
}

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

/* the four new virtuals that say no (the names are ours) */
int CEntity::Virtual68()
{
    return 0;
}

int CEntity::Virtual64()
{
    return 0;
}

int CEntity::Virtual60()
{
    return 0;
}

int CEntity::Virtual5C()
{
    return 0;
}

/* the entity of a name: its node's (SOCOM 1's; the node at 0x28 now) */
CEntity *CEntityList::GetEntity(const char *name) const
{
    if (!name)
        return 0;
    for (const_iterator it = begin(); it != end(); ++it)
        if (!strcasecmp(name, (*it)->m_node->m_name))
            return *it;
    return 0;
}

/* the entity of an id, or none for the invalid id (SOCOM 1's) */
CEntity *CEntityList::GetEntityById(unsigned int id) const
{
    if (id == CEntity::invalidId)
        return 0;
    for (const_iterator it = begin(); it != end(); ++it)
        if ((*it)->m_id == id)
            return *it;
    return 0;
}

/* the entity at a place in the list (SOCOM 1's) */
CEntity *CEntityList::GetEntityByIndex(unsigned int index) const
{
    if (index >= size())
        return 0;
    const_iterator it = begin();
    while (index--)
        ++it;
    return *it;
}
