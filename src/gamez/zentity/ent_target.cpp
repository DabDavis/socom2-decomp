/* ent_target.cpp -- what entities know of each other: CTarget, the target lists and their updates
 * (OVL1 0x2A0120-0x2A250F, closed by its template instances; SOCOM 1's unit, whose initialiser
 * builds the three pending lists -- SOCOM II's is 0x400030). Only some of it so far. Defined in
 * reverse of their image order (-inline deferred). */

#include <algorithm>

#include "zentity.h"

CEntityList CEntity::m_target_inc;
CEntityList CEntity::m_target_add;
CEntityList CEntity::m_target_rem;

/* the nearer target first (SOCOM 1's, std::sort's predicate) */
bool _sort_target(const CTarget &a, const CTarget &b)
{
    return a.m_distSq < b.m_distSq;
}

/* whether the entity keeps a target list: dropping it empties the list (SOCOM 1's).
 * OPEN (right size, one call): the target calls the deleter's clear (0x2A0090, the one
 * ~__vector_deleter<CTarget> calls); include/vector's vector<class>::clear() inlines a level less
 * here and calls __vector_imp's */
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
 * OPEN (right size, 8 instructions): only the temporaries' stack slots -- the target's binder
 * copy at sp+76 and the push_backs' end iterators 4 bytes lower than these; the find written
 * straight into the test is longer (its begin()/end() a level deeper, partly out of line) */
void CEntity::SetIncludeInTargetList(bool on)
{
    if (on == m_include_in_targetlist)
        return;
    if (on) {
        m_target_rem.remove(this);

        CEntityList::iterator it = std::find(m_target_inc.begin(), m_target_inc.end(), this);

        if (m_target_inc.end() == it)
            m_target_add.push_back(this);
    } else {
        m_target_rem.push_back(this);
        m_target_add.remove(this);
    }
    m_include_in_targetlist = on;
}
