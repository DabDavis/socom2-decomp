/* ent_target.cpp -- what entities know of each other: CTarget, the target lists and their updates
 * (OVL1 0x2A0120-0x2A250F, closed by its template instances; SOCOM 1's unit, whose initialiser
 * builds the three pending lists -- SOCOM II's is 0x400030). Only some of it so far. Defined in
 * reverse of their image order (-inline deferred). */

#include <algorithm>

#include "zentity.h"

CEntityList CEntity::m_target_inc;
CEntityList CEntity::m_target_add;
CEntityList CEntity::m_target_rem;
int CEntity::rrTargetListFirst;
int CEntity::rrTargetListCount = 3;

/* SOCOM 1's round robin over the entities (its setup(first, count, size) out of line there, 0x1F0180;
 * SOCOM II's inline, its fields scalarised into registers): the slice runs from m_first for m_count
 * entities, the part past the end wrapping to the list's front (m_wrap of them) */
class CRobin {
public:
    void setup(int first, int count, int size)
    {
        m_first = first;
        m_count = count;
        m_size = size;
        m_last = m_first + m_count;
        if (count >= size)
            m_wrap = count;
        else if (m_last > size)
            m_wrap = m_last - size;
        else
            m_wrap = 0;
    }
    bool InSlice(int i) const
    {
        if (i < m_wrap || (i >= m_first && i < m_last))
            return true;
        return false;
    }
    /* where the next slice starts */
    int Next() const
    {
        int n = m_first + m_count;

        if (m_wrap > 0)
            n = 0;
        else if (n >= m_size)
            if (m_size == 0)
                n = 0;
            else
                n %= m_size;
        return n;
    }

    int m_first;
    int m_last;
    int m_count;
    int m_size;
    int m_wrap;
};

/* the nearer target first (SOCOM 1's, std::sort's predicate) */
bool _sort_target(const CTarget &a, const CTarget &b)
{
    return a.m_distSq < b.m_distSq;
}

/* whether the entity keeps a target list: dropping it empties the list (SOCOM 1's).
 * The list's clear() calls the deleter's clear (0x2A0090) out of line, which include/vector
 * defines after the class as MSL does: vector::clear and __vector_imp::clear inline around it */
void CEntity::SetUpdateTargetList(bool on)
{
    if (on != m_update_targetlist) {
        if (!on)
            m_targets.clear();
        m_update_targetlist = on;
    }
}

/* whether the entity is in the others' target lists: queued for adding unless it is already
 * there, or for removal (SOCOM 1's). `this` is handed to the lists by reference, so MW homes it
 * on the stack and reads every member through it (a local copy of it reads the flag from a0).
 * Byte-exact with the first removal through the implementation's remove, one level shallower than
 * the wrapper's: its binder then takes the frame's top slot (sp+76) above the push_backs'
 * temporaries, as the target's (round 5; the wrapper's remove put it below them -- the README's
 * level-6 list counted this function; the second removal's spelling changes nothing) */
void CEntity::SetIncludeInTargetList(bool on)
{
    if (on == m_include_in_targetlist)
        return;
    if (on) {
        m_target_rem.list_imp::remove((const void *const &)this);

        CEntityList::iterator it = std::find(m_target_inc.begin(), m_target_inc.end(), this);

        if (m_target_inc.end() == it)
            m_target_add.push_back(this);
    } else {
        m_target_rem.push_back(this);
        m_target_add.list_imp::remove((const void *const &)this);
    }
    m_include_in_targetlist = on;
}

/* an entity's place in the others' target lists handed to its stand-in: every target on it, in
 * each live entity's list, seen again as the new one (SOCOM 1's ReplaceTargetList, 0x1F0930;
 * DoRecycleEntity's). The walk assigns its iterator, which lives in memory, the end taken first */
void CEntity::ReplaceTargetList(CEntity *from, CEntity *to)
{
    CEntityList::iterator it, e = m_list.end();

    for (it = m_list.begin(); it != e; ++it) {
        CEntity *ent = *it;

        if (ent->m_type == ENT_TYPE_RECYCLE || ent == from)
            continue;

        CTargetList &targets = ent->m_targets;

        for (CTargetList::iterator t = targets.begin(); t != targets.end(); ++t)
            if ((*t).m_ent == from)
                (*t).Update(ent, to);
    }
}

/* the entity's targets brought up to date (SOCOM 1's OnUpdateTargetList(bool, bool), 0x1F0380:
 * rebuilt from the others when forced or when their count changed, else each seen again and the
 * run that fell out of order marked for the sort; SOCOM II's Update reads the positions straight) */
bool CEntity::OnUpdateTargetList(bool force, bool sort)
{
    CTargetList &targets = m_targets;
    CTargetList::iterator last = targets.end();
    unsigned int n = m_include_in_targetlist ? m_target_inc.size() - 1 : m_target_inc.size();
    bool rebuild = force || n != targets.size();
    bool changed = false;

    if (rebuild) {
        targets.resize(n);
        CTargetList::iterator t = targets.begin();
        last = targets.end();
        CEntityList::iterator e = m_target_inc.end();
        for (CEntityList::iterator i = m_target_inc.begin(); i != e; ++i)
            if (*i != this)
                (*t++).Update(this, *i);
        changed = true;
    } else {
        static float range_tolerance = 100.0f;
        float prev = 0.0f;
        CTargetList::iterator t = targets.begin();
        while (t != targets.end()) {
            (*t).Update(this, (*t).m_ent);
            float d = (*t).m_distSq;
            changed = d < prev && d < m_max_target_range_sq;
            if (changed) {
                ++t;
                last = t;
                break;
            }
            prev = (*t).m_distSq - range_tolerance;
            ++t;
        }
        while (t != targets.end()) {
            CTarget &c = *t++;
            c.Update(this, c.m_ent);
            if (c.m_distSq < prev && c.m_distSq < m_max_target_range_sq)
                last = t;
        }
    }
    if (sort && changed) {
        std::sort(targets.begin(), last, _sort_target);
        return true;
    }
    return false;
}

/* how many enemies are aware of each entity: every live entity that keeps a target list counts
 * itself against each target it is aware of (flag 0x200) that shares none of its teams (new in
 * SOCOM II, called from OVL2; the names are ours). The const walk initialises its iterator from
 * begin() (two copies, then a register; the ticks assign theirs); the target walk runs through
 * TargetsOf (the size and the data re-read through &m_targets each pass: the MSL's round 30); the
 * target's entity is read ahead of the flag test (its load in the test's delay slot), the team
 * test a materialised bool written in place -- IsEnemy (member or free, const or not) has MW hoist
 * &e->m_team out of the walk (0x150), the plain `!(a & b)` branches on the and (bnez) */
/* an entity's target list, through an inline (seal_aivehicle.cpp's TargetsOf: the MSL headers' round
 * 30 -- the walk through it re-reads the size and the data each pass; the name ours) */
inline CTargetList *TargetsOf(CEntity *e)
{
    return &e->m_targets;
}

void CEntity::CountAwareness()
{
    for (CEntityList::const_iterator it = GetEntityList().begin(); it != GetEntityList().end(); ++it) {
        CEntity *e = *it;

        e->m_aware_counter = 0;
        if (e->m_type != ENT_TYPE_RECYCLE && e->m_isAlive && e->m_update_targetlist)
            for (CTargetList::iterator t = TargetsOf(e)->begin(); t != TargetsOf(e)->end(); ++t)
            {
                CEntity *te = (*t).m_ent;

                if (((*t).m_flags & 0x200) && (!(te->m_team & e->m_team) ? true : false))
                    te->m_aware_counter++;
            }
    }
}

/* every entity's target list kept: the entities dropped from the lists first, then the ones added
 * (each a tick of its own), else a slice of the entities seen again -- rrTargetListCount of them from
 * rrTargetListFirst on, and the first SEAL every tick; an entity whose list needed no sort lets the
 * slice run one further (SOCOM 1's UpdateTargetList, 0x1EFBF0; the bool is read by neither build).
 * The m_list walks initialise their iterators (in registers); the clears are written as erases of the
 * whole list (clear() puts their temporaries in other frame slots); InSlice's test returns true or
 * false (the jump form the target materialises), the next start's modulo an if/else (its label
 * padded as the target's).
 * OPEN, 20 instructions, registers only (28 and 3 structural before round 75): (1) closed in round
 * 75 -- the first inner walk's end, computed once per outer pass into s2 and tested twice, is a
 * local declared INSIDE a hand-rotated walk's guard (`if (j != end()) { iterator je = end(); do ...
 * while (++j != je); }`: no loop to hoist it out of until the do), the outer walk rotated by hand too
 * (its iterator then numbered after je: s1 against je's s2 -- igreplay wanted je below it or it
 * after je); (2) the slice loop's entity, last and count trade s0/s1/s3 -- the
 * target's colouring wants in < count < size < last < entity in MW's numbering, where the scalarised
 * CRobin makes size and count inliner temporaries (setup's arguments, right to left), last a
 * scalariser one and the entity a named local numbered before them all (as a CSE temporary, (*it)->
 * at each use, its load moves into the conditional SEAL test); plain named locals for the robin are
 * the shape to try next */
void CEntity::UpdateTargetList(bool b)
{
    if (m_target_rem.size()) {
        CEntityList::iterator it = m_list.begin();

        if (it != m_list.end())
            do {
                CEntity *e = *it;

                e->UpdateAiMapLoc();

                CTargetList &targets = e->m_targets;

            CEntityList::iterator j = m_target_rem.begin();

            if (j != m_target_rem.end()) {
                CEntityList::iterator je = m_target_rem.end();

                do {
                    targets.RemoveTarget(*j);
                } while (++j != je);
            }
            } while (++it != m_list.end());
        CEntityList::iterator re = m_target_rem.end();

        for (CEntityList::iterator j = m_target_rem.begin(); j != re; ++j)
            m_target_inc.list_imp::remove((const void *const &)*j);
        m_target_rem.erase(m_target_rem.begin(), m_target_rem.end());
        return;
    }
    if (m_target_add.size()) {
        CEntityList::iterator e = m_list.end();

        for (CEntityList::iterator it = m_list.begin(); it != e; ++it) {
            CEntity *ent = *it;

            ent->UpdateAiMapLoc();
            if (ent->m_update_targetlist) {
                CTargetList &targets = ent->m_targets;
                CEntityList::iterator ae = m_target_add.end();

                for (CEntityList::iterator j = m_target_add.begin(); j != ae; ++j)
                    if (ent != *j)
                        targets.AddTarget(ent, *j);
            }
        }
        for (CEntityList::iterator j = m_target_add.begin(); j != m_target_add.end(); ++j)
            m_target_inc.push_back(*j);
        m_target_add.erase(m_target_add.begin(), m_target_add.end());
        return;
    }

    CRobin robin;

    robin.setup(rrTargetListFirst, rrTargetListCount, m_list.size());

    bool seal_seen = false;
    int i = 0;
    CEntityList::iterator e = m_list.end();

    for (CEntityList::iterator it = m_list.begin(); it != e; ++it, ++i) {
        bool in;
        CEntity *ent = *it;

        in = robin.InSlice(i);

        if (!in && !seal_seen && ent->m_type == ENT_TYPE_SEAL)
            seal_seen = in = true;

        bool sorted = false;

        ent->UpdateAiMapLoc();
        if (ent->m_update_targetlist)
            sorted = ent->OnUpdateTargetList(false, in);
        if (in && !sorted)
            robin.setup(robin.m_first, robin.m_count + 1, robin.m_size);
    }
    rrTargetListFirst = robin.Next();
}

/* the targets on an entity dropped (new in SOCOM II, UpdateTargetList's: std::remove inlined,
 * its remove_copy and the erase called; the name is ours). The erase (0x2A19C0) matches. The end
 * built once into a local and handed to remove and to erase, the new end a local -- CWorld::
 * DeleteLightMap's slots exactly (the MSL headers round 8: include/algorithm's remove joins its two
 * results through one copy, the class erase copies its iterators into its parameters).
 * remove_copy's copy (0x2A1210) stores the m_ent its compare loaded through zentity.h's non-const
 * operator== (the const one reloads it: 8 bytes long) */
void CTargetList::RemoveTarget(CEntity *e)
{
    iterator l = end();
    iterator i = std::remove(begin(), l, e);

    erase(i, l);
}

/* a target added for an entity and seen at once (new in SOCOM II, UpdateTargetList's; the name
 * is ours) */
void CTargetList::AddTarget(CEntity *self, CEntity *e)
{
    push_back(CTarget());
    back().Update(self, e);
}

/* the target on an entity, or none (SOCOM 1's; the end taken once, the walk's test the iterator's
 * != materialised) */
CTarget *CTargetList::FindTarget(CEntity *e)
{
    iterator last = end();

    for (iterator it = begin(); it != last; ++it)
        if ((*it).m_ent == e)
            return &*it;
    return 0;
}

/* nothing known yet (SOCOM 1's, out of line as there; SOCOM II's clears the flags word whole) */
CTarget::CTarget()
    : m_ent(0), m_distSq(0), m_dist(0), m_visibility(0), m_unk20(0), m_unk24(0), m_unk28(0), m_flags(0)
{
}

/* the range, its square root taken once (SOCOM 1's) */
float CTarget::GetRange()
{
    if (!(m_flags & 1)) {
        m_dist = sqrtf(m_distSq);
        m_flags |= 1;
    }
    return m_dist;
}
