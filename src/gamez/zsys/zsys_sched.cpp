/* zsys_sched.cpp -- the task scheduler: CSched_Manager's list of tasks, each a callback with its
 * user data, ticked by priority (OVL1 0x34E070-0x34EADF). The
 * file name is SOCOM 1's (its asserts name zsys_sched.cpp). Defined in reverse of their image order
 * (-inline deferred). The unit's initialiser 0x403420 builds zTaskScheduler through
 * CSched_Manager() 0x34E860 (the unit's first-defined function, the last of its own code) and
 * registers ~CSched_Manager 0x34EA70 -- zanim.h declares the constructor since the engine core's
 * round 3 (the world lane's round 10 measured the pair: build/scratch/wld/). */

#include <string.h>
#include <algorithm>

#include "../zanim/zanim.h"

/* the EE timer 0's count, read as the task's start and stop times */
#define T0_COUNT (*(volatile unsigned int *)0x10000000)

/* the scheduler (SOCOM 1's zTaskScheduler; the unit's initialiser 0x403420 builds it through
 * CSched_Manager() and registers ~CSched_Manager 0x34EA70) */
CSched_Manager zTaskScheduler;

/* SOCOM 1's: not ticking, no priority, the free list emptied (out of line: the initialiser calls
 * it) */
CSched_Manager::CSched_Manager() : m_inTick(false), m_priority(0.0f)
{
    m_FreeList.clear();
}

/* a task added: one off the free list, or a new one; named (a copy), its callback, priority and
 * user data set, and linked in ahead of the first task of a higher priority, else at the end
 * (SOCOM 1's AddTask; SOCOM 1 passed the task itself to insert, homed once at the join -- here
 * that keeps it in memory, 0x1CC bytes, so the reference goes to a copy). OPEN, 9 instructions:
 * every instruction but the frame slots of the three calls' iterator temporaries -- the target
 * puts the loop insert's result at 144, just under begin()'s temporary and ABOVE the free list's
 * erase (140/136), and its argument at the bottom (124), below the end insert's pair (132/128);
 * here they are made in source order. The end taken into a local fixed the end's register (14 ->
 * 9); the loop insert's result assigned (`i = insert(i, added)`, or to a named iterator, wrapper
 * or base) puts its argument at 124 (7) but the result stays below the others; pop_front against
 * erase(begin()), push_back for the end insert and the locals' declaration orders change nothing
 * or lose (build/scratch/core/v_addtask.py) */
bool CSched_Manager::AddTask(const char *name, bool (*task)(float, void *), float priority, void *user_data)
{
    CSched_Task *t;

    if (m_FreeList.size() == 0)
        t = new CSched_Task;
    else {
        t = m_FreeList.front();
        m_FreeList.pop_front();
    }

    CSched_Task *added = t;

    if (t->m_name)
        _free(t->m_name, 0, 0);
    t->m_name = 0;
    t->m_name = name ? _strdup(name, 0, 0) : 0;
    t->m_remove = false;
    t->m_priority = priority;
    t->m_task = task;
    t->m_user_data = user_data;

    bool inserted = false;
    iterator i;
    iterator e = end();

    for (i = begin(); i != e; ++i) {
        if (priority < (*i)->m_priority) {
            insert(i, added);
            inserted = true;
            break;
        }
    }
    if (!inserted)
        insert(end(), added);
    return true;
}

/* every task ticked once in list order: a task flagged for removal, or whose callback returns
 * true, goes to the free list (SOCOM 1's, line for line). erase's result bound to a const reference
 * and assigned from it: the temporary then takes its slot (120) ahead of the pushed copy's (116),
 * the target's (round 6's four-word miss: `i = erase(i);` laid them the other way round) */
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
                const iterator &n = erase(i);

                i = n;
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

/* every task by this name flagged for removal, or the first only (SOCOM 1's). The iterator assigned
 * from begin() before the end is taken: the first node is loaded ahead of the sentinel's address,
 * into a1 while `this` is still live, as the target's (the twin below takes the end first) */
bool CSched_Manager::RemoveTask(const char *name, bool all)
{
    bool found = false;
    iterator i;

    i = begin();
    iterator e = end();

    for (; i != e; ++i) {
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
