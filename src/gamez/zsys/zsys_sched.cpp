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

#if Z_PORT
/* the port: the implicit destructor written (zanim.h's declaration): the free list, then the base */
CSched_Manager::~CSched_Manager()
{
}
#endif

/* the task linked in before pos through the base's insert, its iterator handed back as it is --
 * two inline levels that each return the base's iterator (a STAND-IN for the original MSL's
 * list<T *>::insert, whose depths are not known: what it reproduces is the frame). MW lays out
 * the frame in creation order: the caller's own temporaries first -- an inline's return slot is
 * one when every level returns the same class, so the base insert's result lands just under
 * begin()'s temporary -- then the inline expansions breadth-first, each creating its by-value
 * parameters and temporaries as it is expanded: depth 1 (pop_front's erase, the end insert)
 * before depth 2 (this pair's inner level, whose position copy so goes to the frame's bottom) */
inline CSched_Manager::base::iterator SchedInsertNode(CSched_Manager *m, CSched_Manager::base::iterator pos,
                                                     CSched_Task *const &t)
{
    return m->base::insert(pos, (const void *const &)t);
}

inline CSched_Manager::base::iterator SchedInsert(CSched_Manager *m, CSched_Manager::iterator pos,
                                                 CSched_Task *const &t)
{
    return SchedInsertNode(m, pos, t);
}

/* a task added: one off the free list, or a new one; named (a copy), its callback, priority and
 * user data set, and linked in ahead of the first task of a higher priority, else at the end
 * (SOCOM 1's AddTask; SOCOM 1 passed the task itself to insert, homed once at the join -- here
 * that keeps it in memory, 0x1CC bytes, so the reference goes to a copy). The loop's insert
 * through SchedInsert gives the target's frame (the three calls' iterator temporaries at
 * 144/124, 140/136 and 132/128); through the wrapper's insert they were made in source order */
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
            SchedInsert(this, i, added);
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

/* a task flagged for removal deleted: remove_if's predicate (SOCOM 1's _task_cleanup) */
bool _task_cleanup(CSched_Task *t)
{
    if (t->m_remove) {
        delete t;
        return true;
    }
    return false;
}

/* std::remove_if's instance for this list (0x34E8C0) is include/algorithm's: the world's round 14
 * matched it through a stand-in specialisation here in the MSL headers' round-8 shape C (the
 * search by const reference, its two copies made in order, first s5 then last s4), which the MSL
 * headers' round 20 made the header's own for every bidirectional iterator (include/algorithm's
 * remove_if) -- the specialisation retired, remove_if and remove_copy_if 0x34E9B0 byte-exact as
 * before */

/* ClearFreeList's three list operations, each one inline level deeper than include/list's (STAND-INS
 * for SOCOM II's own MSL, whose list<T *> forwards to the base a level further; what they reproduce
 * is the frame and the registers): the range erase through an inline giving back the wrapper's
 * iterator (list<T *>'s erase over the base's -- one returning the base's iterator makes the result
 * one of the caller's own temporaries, created ahead of the walks' begin() temporaries, where the
 * target creates it with the erase's two by-value iterators), and the base's push_back (insert(end(),
 * x)), whose result and position so come at depth 2, after the clear()s */
typedef std::__list_imp<const void *, std::allocator<const void *> > SchedBase;

inline CSched_Manager::iterator SchedErase(CSched_Manager *m, CSched_Manager::iterator first,
                                           CSched_Manager::iterator last)
{
    return CSched_Manager::iterator(m->base::erase(first, last));
}

inline void SchedPushBack(std::list<CSched_Task *> *l, CSched_Task *const &t)
{
    ((SchedBase *)l)->push_back((const void *const &)t);
}

/* the same through a pointer local: the copy-back's end, this+4, is then recomputed at each pass, as
 * the target's is (through the cast MW hoists it into a saved register, and `this` moves from s2 to
 * s3) */
inline void SchedPushBackTo(std::list<CSched_Task *> *l, CSched_Task *const &t)
{
    SchedBase *b = (SchedBase *)l;

    b->push_back((const void *const &)t);
}

/* with `all`, the removed tasks deleted and taken off the list (SOCOM 1's), the list then copied
 * out and back (SOCOM II's); the free list's tasks deleted and the free list emptied (SOCOM 1's).
 * Brings remove_copy_if 0x34E9B0 byte-exact (remove_if 0x34E8C0 through the specialisation above).
 * One iterator for both copy loops: MW splits it into two webs and the
 * second, made last, takes the frame's bottom slot above the local list (112), as the target's
 * does -- two iterators, or one per loop, are numbered with the named locals at the top. The frame
 * then follows round 13's creation order: the named locals (the remove_if result, the element, the
 * iterator), the function's temporaries (remove_if's two arguments, the walks' begin()s), the depth-1
 * expansions in source order (the erase's triple, clear()'s, the free list's), the depth-2 ones (the
 * two push_backs' pairs, the local list's destructor's erase) and the split web */
void CSched_Manager::ClearFreeList(bool all)
{
    if (all) {
        iterator e = end();
        iterator r = std::remove_if(begin(), e, _task_cleanup);

        SchedErase(this, r, e);
    }
    if (all) {
        std::list<CSched_Task *> tmp;
        CSched_Task *t;
        iterator i;

        for (i = begin(); i != end(); ++i) {
            t = *i;
            SchedPushBack(&tmp, t);
        }
        clear();
        i = tmp.begin();
        if (i != tmp.end())
            do {
                t = *i;
                SchedPushBackTo(this, t);
            } while (++i != tmp.end());
    }
    if (m_FreeList.size()) {
        iterator e = m_FreeList.end();

        for (iterator i = m_FreeList.begin(); i != e; ++i)
            delete *i;
        m_FreeList.clear();
    }
}
