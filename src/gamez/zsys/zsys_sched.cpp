/* zsys_sched.cpp -- the task scheduler: CSched_Manager's list of tasks, each a callback with its
 * user data, ticked by priority (OVL1 0x34E070-0x34EADF). The
 * file name is SOCOM 1's (its asserts name zsys_sched.cpp). Defined in reverse of their image order
 * (-inline deferred). */

#include <string.h>
#include <algorithm>

#include "../zanim/zanim.h"

/* the EE timer 0's count, read as the task's start and stop times */
#define T0_COUNT (*(volatile unsigned int *)0x10000000)

/* every task ticked once in list order: a task flagged for removal, or whose callback returns
 * true, goes to the free list (SOCOM 1's, line for line).
 * OPEN, 4 instructions, stack slots only: the target lays the pushed copy of the task (116) after
 * the assignment's iterator temporary (120) and before erase's two (112, 108); a named copy takes
 * 120, ahead of the temporary, and an inline taking the task by value puts it after erase's.
 * Tried: a cast or sum as the argument (a temporary at the call), an inline doing the erase and
 * the insert */
bool CSched_Manager::Tick(float dT)
{
    m_inTick = true;
    bool r = !(size() == 0);

    if (r) {
        iterator i = begin(), e = end();

        while (i != e) {
            CSched_Task *t = *i;

            t->m_startTime = T0_COUNT;
            bool done = t->m_remove;

            if (!done) {
                m_priority = t->m_priority;
                done = t->m_task ? t->m_task(dT, t->m_user_data) : true;
            }
            if (done) {
                i = erase(i);
                CSched_Task *kept = t;

                m_FreeList.insert(m_FreeList.begin(), kept);
            } else
                ++i;
            t->m_stopTime = T0_COUNT;
        }
    }
    m_inTick = false;
    return r;
}

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
