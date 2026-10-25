/* ent_main.cpp -- CEntity: the entity lists, the recycler, the ticks over every entity, and the
 * entity's own small virtuals (OVL1 0x29E470-0x2A011F; SOCOM 1's unit, __sinit_ent_main.cpp,
 * building the recycler and the two lists, 0x3FFFB0). Defined in reverse of their image order
 * (-inline deferred). */

#include <algorithm>
#include <string.h>

#include "zentity.h"

/* the one entity standing in for every recycled one: made on first use, counted, freed when the
 * count runs out (SOCOM 1's `recycler`, 12 bytes, cleared by __sinit_ent_main.cpp; the type and
 * its members' names are ours) */
struct CRecycler {
    CRecycler() : m_count(0), m_entity(0), m_node(0) {}

    /* the stand-in, made if there is none, counted once more */
    CEntity *Get()
    {
        if (!m_count) {
            m_node = zdb::CNode::Create("recycler");
            m_entity = new CEntity(CEntity::ENT_TYPE_RECYCLE, m_node);
            m_entity->SetUpdateTargetList(false);
            m_entity->SetIncludeInTargetList(false);
        }
        m_count++;
        return m_entity;
    }
    void Free()
    {
        delete m_entity;
        delete m_node;
        m_entity = 0;
        m_node = 0;
        m_count = 0;
    }
    /* whether an entity is the stand-in: its count taken down, freed at none */
    bool Release(CEntity *e)
    {
        bool was = false;

        if (e == m_entity) {
            m_count--;
            if (!m_count)
                Free();
            was = true;
        }
        return was;
    }

    int m_count;
    CEntity *m_entity;
    zdb::CNode *m_node;
};

static CRecycler recycler;
CEntityList CEntity::m_list;
CEntityList CEntity::m_recycle;
/* an entity's id is the count of those made before it (SOCOM 1's, in $gp small data there) */
unsigned int CEntity::nextEntityId;

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

/* an entity whose id was taken away is deleted (the purge's predicate, new in SOCOM II; the name
 * is ours) */
static bool DeleteIfIdless(CEntity *e)
{
    bool gone = false;

    if (e->m_id == 0xFFFFFFFF) {
        delete e;
        gone = true;
    }
    return gone;
}

/* the list rid of the entities without an id, the rest renumbered (new in SOCOM II, called from
 * CMPExitState::Tick; the name is ours).
 * OPEN, 24 instructions: the target keeps the end in s1 and the address of the first node's link
 * (m_list + 8) in s0 -- both begin() calls read through it -- builds the erase's last argument
 * after remove_if returns, and walks to a copy of the end (`move a2,s1`); this form (a list
 * reference, the end in a local, remove_if a statement of its own) has the shape with s0/s1 the
 * other way round and the last walk's temporaries one slot off. The end through end() each
 * time, the list itself, one statement, the walk to end(): all further off */
void CEntity::PurgeEntities()
{
    CEntityList &list = m_list;
    CEntityList::iterator end = list.end();
    CEntityList::iterator first = std::remove_if(list.begin(), end, DeleteIfIdless);

    list.erase(first, end);

    CEntityList::iterator it;
    unsigned int index = 0;

    for (it = list.begin(); it != end; ++it)
        (*it)->m_index = index++;
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

/* an entity into a recycled place: the stand-in at that place traded for it (SOCOM 1's) */
bool CEntity::RecycleEntity(unsigned int index, CEntity *e, bool include)
{
    bool done = false;

    if (index >= m_list.size())
        return false;

    CEntityList::iterator it = m_list.begin();

    while (index--)
        ++it;
    if (*it && (*it)->m_type == ENT_TYPE_RECYCLE) {
        *it = e;
        e->SetIncludeInTargetList(include);
        UpdateTargetList(true);
        done = true;
    }
    return done;
}

/* the stand-in entered into the world, as an entity is (SOCOM 1's) */
CEntity *CEntity::CreateRecycler()
{
    CEntity *e = recycler.Get();

    CreateEntity(e, false);
    return e;
}

/* a queued entity out of the world: every entity told, its targets handed to the stand-in, its
 * place in the list given to the stand-in, and deleted (SOCOM 1's) */
void CEntity::DoRecycleEntity(CEntity *e)
{
    m_recycle.remove(e);

    CEntity *r = recycler.Get();
    CEntityList::iterator it;
    CEntityList::iterator end = m_list.end();
    CEntityList::iterator found = m_list.end();

    for (it = m_list.begin(); it != end; ++it) {
        CEntity *x = *it;

        x->OnRecycleEntity(e);
        if (x == e)
            found = it;
    }
    ReplaceTargetList(e, r);
    if (found != m_list.end()) {
        *found = r;
        delete e;
    }
}

/* every entity deleted, the stand-in too, and every list emptied (SOCOM 1's) */
void CEntity::RemoveAll()
{
    m_recycle.clear();
    while (m_list.size()) {
        CEntity *e = m_list.front();

        m_list.pop_front();
        if (!recycler.Release(e))
            delete e;
    }
    recycler.Free();
    nextEntityId = 0;
    m_target_inc.clear();
    m_target_rem.clear();
    m_target_add.clear();
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

/* an entity of a type on a node: the next id, at rest, unit orientation, in the target lists'
 * reach, alive (SOCOM 1's, with SOCOM II's position, anchor and three more flags set). The new
 * bit 13 is cleared in the initialiser list -- a word written whole, where the body's flags are
 * written a byte at a time (probe: build/scratch/ent/bf.cpp).
 * OPEN, 4 bytes short: the target inlines m_targets' constructor whole (CTargetList, then the
 * vector's three stores); include/vector's class vector is four constructor levels, so
 * CTargetList's is five and MW calls vector<CTarget>'s. With a three-high class vector this
 * matches byte for byte (all relocations but CAiMapLoc::invalid's, now in data.txt) -- see
 * CCompass::CreateAnchor for why include/vector cannot simply change. Compiled out for the same
 * reason as the destructor: a constructor emits its members' destructors, and this one's
 * ~__vector_deleter<CTarget> would supersede 0x29FF60's draft and miss */
#if 0
CEntity::CEntity(TYPE type, zdb::CNode *node)
    : m_type(type), m_node(node), m_control(0), m_team(0), m_bit13(0)
{
    m_quat = CQuat::identity;
    m_next_quat = CQuat::identity;
    m_velW = CPnt3D::zero;
    m_velM = CPnt3D::zero;
    m_velR = CPnt3D::zero;
    m_posW = CPnt3D::zero;
    m_next_velw = CPnt3D::zero;
    m_UnitsSeenBy = 0;
    m_id = nextEntityId;
    nextEntityId++;
    m_aware_counter = 0;
    m_display_name = 0;
    m_interpolate_animations = 1;
    m_noshoot = 0;
    m_update_targetlist = 1;
    m_include_in_targetlist = 0;
    m_max_target_range_sq = 25000000.0f;
    m_isAlive = 1;
    m_check_player_collision = 1;
    m_get_new_altitude = 1;
    m_anchor = 0;
}
#endif

/* the name freed, out of the target lists, the compass anchor deleted (SOCOM 1's, the anchor
 * new). The target list's destructor inlines ~CTargetList, ~vector and ~__vector_imp (three
 * this-tests) and calls ~__vector_deleter<CTarget> (0x29FF60), which include/vector defines out of
 * class as MSL does; that copy (it calls the deleter's clear out of line, 0x2A0090) is still the
 * draft's */
CEntity::~CEntity()
{
    if (m_display_name)
        _free(m_display_name, 0, 0);
    m_display_name = 0;
    SetIncludeInTargetList(false);
    CCompass::DeleteAnchor(m_anchor);
    SetCompassAnchor(0);
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

/* the entity's anchor on the compass, made once: a vehicle's (type 11) at the origin, its point
 * cursor cleared, hidden and locked unless it is a turret; then any anchor named and typed, and
 * kept (new in SOCOM II; the name is ours) */
CCompassAnchor *CEntity::CreateCompassAnchor(int type, const char *name)
{
    CCompassAnchor *anchor = 0;

    if (m_anchor)
        return 0;
    switch (type) {
    case 11:
        anchor = CCompass::CreateAnchor(CPnt3D::zero, "VEHICLE");
        anchor->SetPoint(-1);
        if (name && !strstr(name, "Turret")) {
            anchor->SetEnabled(false);
            anchor->SetArrowEnabled(false);
            anchor->m_locked = 1;
        }
        break;
    }
    if (anchor) {
        if (name)
            anchor->SetName(name);
        anchor->m_type = type;
    }
    SetCompassAnchor(anchor);
    return anchor;
}

/* the anchor moved to the entity and shown while it lives, unless it is locked (slot 0x6C, new in
 * SOCOM II; the name is ours) */
void CEntity::UpdateCompassAnchor()
{
    if (m_anchor && !m_anchor->m_locked) {
        m_anchor->m_posW = m_posW;
        m_anchor->SetEnabled(m_isAlive);
    }
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

/* the ids no entity has, defined after their readers: an earlier definition lets MW fold -1 into
 * GetEntityById's test, where the target reads the word (SOCOM 1's likewise) */
const unsigned int CEntity::invalidId = 0xFFFFFFFF;
const unsigned int CEntity::deletedId = 0xFFFFFFFE;
