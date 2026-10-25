/* zsys_sched.cpp -- the task scheduler: CSched_Manager's list of tasks, each a callback with its
 * user data, ticked by priority (OVL1 0x34DED0?-0x34EADF; only the removals written so far). The
 * file name is SOCOM 1's (its asserts name zsys_sched.cpp). Defined in reverse of their image order
 * (-inline deferred). */

#include <string.h>
#include <algorithm>

#include "../zanim/zanim.h"

/* every task by this name flagged for removal, or the first only (SOCOM 1's).
 * OPEN, 3 instructions long (0xD0 of 0xC4): the walk matches its twin below, but across strcmp the
 * wrapper iterator's reference-returning * keeps the element's address (node + 8) in a fifth saved
 * register, where the target re-reads the node from the iterator in memory after the call; the
 * base deleter's iterator (the element cast out by value) keeps the iterator in a register
 * instead (0xB4) -- an include/list shape */
bool CSched_Manager::RemoveTask(const char *name, bool all)
{
    bool found = false;
    iterator i, e = end();

    for (i = begin(); i != e; ++i) {
        if (!strcmp((*i)->m_name, name)) {
            (*i)->m_remove = true;
            if (!all)
                return true;
            found = true;
        }
    }
    return found;
}

/* and by its user data (SOCOM 1's) */
bool CSched_Manager::RemoveTask(void *task, bool all)
{
    bool found = false;
    iterator i, e = end();

    for (i = begin(); i != e; ++i) {
        if ((*i)->m_user_data == task) {
            (*i)->m_remove = true;
            if (!all)
                return true;
            found = true;
        }
    }
    return found;
}

/* a task deleted (Clear's functor; the name ours) */
struct DeleteTask {
    void operator()(CSched_Task *t) { delete t; }
};

/* every task deleted and the list emptied, then the free list (SOCOM 1's; OVL1 0x34E480 --
 * the MSL lane's probe, byte-exact) */
void CSched_Manager::Clear()
{
    if (size()) {
        std::for_each(begin(), end(), DeleteTask());
        clear();
    }
    ClearFreeList(false);
}
