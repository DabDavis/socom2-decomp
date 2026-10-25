/* valve_list.cpp -- the valves: every one on CValve::m_list, made and found by name, their
 * callbacks, and the pool they come from (OVL1 0x3518C0-0x35295F). The file name is SOCOM 1's (its
 * static initialiser, which builds the list, pairs with __sinit_valve_list.cpp). Defined in reverse
 * of their image order (-inline deferred). */

#include <algorithm>
#include <string.h>

#include "valve.h"

_zrdr *zrdr_read(const char *name, const char *path, int flags);   /* zrdr.h's (which redefines
                                                                    zanim.h's _zrdr) */
int zrdr_free(CRdrFile *file);

/* the pool's names flushed when no valve holds one, or by force; whether any did (SOCOM 1's).
 * Pinned: Close, Reset and Uninit call it, and MW would inline it there (SOCOM 1's Close did) */
#pragma auto_inline off
bool CValvePool::FlushNames(bool force)
{
    int n = 0;
    std::list<CValve *>::iterator e = CValve::m_list.end();

    for (std::list<CValve *>::iterator i = CValve::m_list.begin(); i != e; ++i)
        if ((*i)->m_name_pooled)
            n++;
    if (!n || force)
        m_namesNext = m_names;
    return n != 0;
}
#pragma auto_inline reset

/* a valve from the pool, or a new one: a type-2 valve's name copied into the name buffer while it
 * has room, the value cleared through the setter (SOCOM 1's) */
CValve *CValvePool::Acquire(const char *name, VALVE_TYPE type)
{
    CValve *valve;
    char *pooled = 0;

    if (m_initialized && name && type == (VALVE_TYPE)2) {
        int len = strlen(name) + 1;

        if (len <= m_namesSize - (int)(m_namesNext - m_names)) {
            pooled = m_namesNext;
            m_namesNext += len;
            strcpy(pooled, name);
        }
    }
    if (m_free.size() == 0) {
        valve = new CValve(pooled ? 0 : name, 0, type);
        if (pooled)
            valve->AssignName(pooled);
    } else {
        valve = m_free.back();
        m_free.pop_back();
        if (pooled)
            valve->AssignName(pooled);
        else
            valve->SetName(name);
        valve->m_type = type;
    }
    if (valve)
        valve->SetValue(0);
    return valve;
}

/* a valve by name with its value set, made and listed if new; an existing one's type raised to 1
 * (unless it is 3) or to 3 (SOCOM 1's) */
CValve *CValve::Create(const char *name, unsigned int value, VALVE_TYPE type)
{
    CValve *valve = name ? GetByName(name) : 0;

    if (!valve) {
        valve = data_0049E820.Acquire(name, type);
        valve->SetValue(value);
        m_list.push_back(valve);
    } else {
        valve->SetValue(value);
        if (type == (VALVE_TYPE)1 && valve->m_type != 3)
            valve->m_type = 1;
        if (type == (VALVE_TYPE)3)
            valve->m_type = 3;
    }
    return valve;
}

/* a valve made dead: its type 2 and its callbacks gone (SOCOM 1's) */
void CValve::Destroy(CValve *valve)
{
    if (valve) {
        if (valve->m_type != 2)
            valve->m_type = 2;
        valve->DeleteCallbacks();
    }
}

/* pinned: both Creates call it, and MW would inline it there */
#pragma auto_inline off
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
#pragma auto_inline reset

/* SOCOM 1's Init, which has nothing to do */
bool CValve::Init()
{
    return true;
}

/* every valve deleted or pooled (SOCOM 1's Uninit did nothing; SOCOM II's is Close for any type) */
void CValve::Uninit()
{
    std::list<CValve *>::iterator e = m_list.end();
    std::list<CValve *>::iterator i = std::remove_if(m_list.begin(), e, CValveOpDelete((VALVE_TYPE)0));

    m_list.erase(i, e);
    data_0049E820.FlushNames(false);
}

/* a valve file parsed, and the pool made once: 256 pooled valves and 4 KB of names (SOCOM 1's) */
bool CValve::Open(const char *name, VALVE_TYPE type)
{
    if (name) {
        CRdrFile *rdr = (CRdrFile *)zrdr_read(name, 0, 0);

        Parse(rdr, type);
        zrdr_free(rdr);
    }
    if (!data_0049E820.m_initialized) {
        data_0049E820.m_free.reserve(256);
        for (unsigned int i = 0; i < 256; i++) {
            CValve *valve = new CValve(0, 0, (VALVE_TYPE)2);

            valve->m_pooled = true;
            data_0049E820.m_free.push_back(valve);
        }
        data_0049E820.m_namesSize = 4096;
        data_0049E820.m_names = new char[4096];
        data_0049E820.m_initialized = true;
        data_0049E820.m_namesNext = data_0049E820.m_names;
    }
    return true;
}

/* the type-2 valves deleted or pooled, and the names flushed if no valve holds one (SOCOM 1's
 * Reset and Close, the same code in both builds -- SOCOM II flushes through the pool's
 * FlushNames where SOCOM 1 inlined it) */
void CValve::Reset()
{
    std::list<CValve *>::iterator e = m_list.end();
    std::list<CValve *>::iterator i = std::remove_if(m_list.begin(), e, CValveOpDelete((VALVE_TYPE)2));

    m_list.erase(i, e);
    data_0049E820.FlushNames(false);
}

void CValve::Close()
{
    std::list<CValve *>::iterator e = m_list.end();
    std::list<CValve *>::iterator i = std::remove_if(m_list.begin(), e, CValveOpDelete((VALVE_TYPE)2));

    m_list.erase(i, e);
    data_0049E820.FlushNames(false);
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
