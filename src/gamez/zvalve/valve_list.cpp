/* valve_list.cpp -- the valves: every one on CValve::m_list, made and found by name, their
 * callbacks, and the pool they come from (OVL1 0x3518C0-0x35295F). The file name is SOCOM 1's (its
 * static initialiser, which builds the list, pairs with __sinit_valve_list.cpp). Defined in reverse
 * of their image order (-inline deferred). */

#include <algorithm>
#include <string.h>

#include "valve.h"

/* a valve made dead: its type 2 and its callbacks gone (SOCOM 1's) */
void CValve::Destroy(CValve *valve)
{
    if (valve) {
        if (valve->m_type != 2)
            valve->m_type = 2;
        valve->DeleteCallbacks();
    }
}

CValve *CValve::GetByName(const char *name)
{
    if (!name)
        return 0;

    std::list<CValve *>::iterator i, e = m_list.end();

    for (i = m_list.begin(); i != e; ++i) {
        CValve *valve = *i;

        if (valve->m_name && !strcmp(valve->m_name, name))
            return valve;
    }
    return 0;
}

void CValve::MakeCallbacks(VALVE_STATE state)
{
    CValveCBList::iterator i, e = m_callbacks.end();

    for (i = m_callbacks.begin(); i != e; ++i) {
        VALVE_CB *cb = *i;

        if (state & cb->mask)
            cb->func(this, cb->user);
    }
}

/* a callback freed, its slot given back as null (ours: SOCOM 1's DeleteCallbacks is the
 * std::transform of the list onto itself -- two begin()s and an end(), right to left) */
struct CValveCBDelete {
    CValveCBDelete() {}
    VALVE_CB *operator()(VALVE_CB *cb)
    {
        delete cb;
        return 0;
    }
};

/* OPEN, 3 instructions short (0x8C of 0x98): the walk and the delete match (the functor needs its
 * empty constructor, or it stores a zero byte); the target keeps &end_.next_ (this+16) in a fifth
 * saved register from before the loop and reads clear()'s begin through it, where include/list's
 * clear reads it off `this` -- a list-header shape (cf. the mission lane's "&end_.next_ in a saved
 * register"). Tried: erase(begin(), end()) written out, the list through a reference, end() in a
 * local. Pinned: Destroy calls it, and MW would inline it there */
#pragma auto_inline off
void CValve::DeleteCallbacks()
{
    std::transform(m_callbacks.begin(), m_callbacks.end(), m_callbacks.begin(), CValveCBDelete());
    m_callbacks.clear();
}
#pragma auto_inline reset
