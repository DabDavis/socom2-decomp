/* ai_events.cpp (OVL2 0x50E730-0x51027F): the AI events -- the pool, the running and the
 * persistent lists, the walks over them, the callbacks told of each new one -- and a SEAL's queue
 * of handled events. SOCOM 1's unit (__sinit_ai_events.cpp, exact at 0x668480; its directory is
 * not known), its functions in the demo's order (0x17F070-0x180AE0); defined here in reverse of
 * the image's order (-inline deferred). The template instances that close it (0x50F670 on) are
 * still the generated drafts', but for the callback list's deleter constructor; the initialiser
 * matches, calling that constructor. */

#include <algorithm>
#include <list>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "ai.h"

extern "C" int strcasecmp(const char *, const char *);

/* no explicit instantiation of the callback list's deleter: it would make the constructor an
 * ordinary auto-inline function, which the initialiser then inlines; uninstantiated by the unit's
 * own functions, it is called there (0x50FFC0) and emitted in the tail, as the target has it */

unsigned int CAiEvent::bit_count = 32;
unsigned int CAiEvent::nextId;
CAiEventList CAiEvent::m_pool;
CAiEventList CAiEvent::m_inuse;
CAiEventList CAiEvent::m_persist;
CAiEventCallbackList CAiEvent::m_callbacks;

/* new in SOCOM II, read from data (variables, so not folded; the names are ours): the shortest
 * duration a running event gets, and what the three kinds of event are heard across */
static float s_eventMinDuration = 0.1f;
static float s_eventRangeDefault = 200.0f;
static float s_eventRangeGeneric = 250.0f;
static float s_eventRangeEntity = 80.0f;
/* the time the event tick has gathered (bss; the name is ours) */
static float s_eventTickTime;
/* a byte Open and Close clear, which nothing in the image reads (bss; the name is ours) */
static bool s_eventFlag;

/* a kind's name (SOCOM 1's; the cases in this order, the enumerators in another) */
const char *CAiEvent::GetEventString(EVENT e)
{
    switch (e) {
    case AIE_UNKNOWN:
        return "UNKNOWN";
    case AIE_GENERIC:
        return "GENERIC";
    case AIE_ALARM:
        return "ALARM";
    case AIE_POINT:
        return "POINT";
    case AIE_ENTITY:
        return "ENTITY";
    case AIE_VOICE:
        return "VOICE";
    case AIE_WEPHIT:
        return "WEPHIT";
    case AIE_CORPSE:
        return "CORPSE";
    case AIE_DOOR:
        return "DOOR";
    case AIE_WEPFIRE:
        return "WEPFIRE";
    case AIE_EXPLOSION:
        return "EXPLOSION";
    case AIE_HELPME:
        return "HELPME";
    case AIE_LIGHTSOFF:
        return "LIGHTS";
    }
    return "UNKNOWN";
}

/* how often a kind repeats: only a call for help does (SOCOM 1's) */
float CAiEvent::GetEventRate(EVENT e)
{
    float rate = 0.0f;

    switch (e) {
    case AIE_GENERIC:
    case AIE_POINT:
    case AIE_ENTITY:
    case AIE_VOICE:
    case AIE_CORPSE:
    case AIE_WEPHIT:
    case AIE_WEPFIRE:
    case AIE_EXPLOSION:
    case AIE_ALARM:
    case AIE_LIGHTSOFF:
    case AIE_DOOR:
        return 0.0f;
    case AIE_HELPME:
        rate = 5.0f;
    default:
        return rate;
    }
}

/* how far a kind is heard (new in SOCOM II; the name is ours) */
float CAiEvent::GetEventRange(EVENT e)
{
    float range = s_eventRangeDefault;

    switch (e) {
    case AIE_GENERIC:
        range = s_eventRangeGeneric;
        break;
    case AIE_ENTITY:
        range = s_eventRangeEntity;
        break;
    }
    return range;
}

/* the kind a name gives, or the default (SOCOM 1's) */
CAiEvent::EVENT CAiEvent::GetEventFromString(const char *s, EVENT def)
{
    if (s) {
        for (unsigned int i = 0; i < AIE_MAX; i++) {
            if (!strcasecmp(GetEventString((EVENT)i), s))
                return (EVENT)i;
        }
    }
    return def;
}

/* every event back in the pool and the pool freed (SOCOM 1's Close did this itself; SOCOM II's
 * calls it. The name is ours).
 * OPEN (0x16C against 0x180): the target's first walk keeps its iterator in memory, assigned
 * (sp+152/156, bitwise copies) and re-read each pass, and its delete loop walks two cursors;
 * a list-iterator shape for the list remodel */
void CAiEvent::FreeEvents()
{
    CAiEventList::iterator i;

    for (i = m_inuse.begin(); i != m_inuse.end(); ++i)
        m_pool.push_back(*i);
    m_inuse.clear();
    for (CAiEventList::iterator j = m_pool.begin(), e = m_pool.end(); j != e; ++j) {
        delete *j;
        *j = 0;
    }
    m_pool.clear();
}

/* a mission's events begin: walks of so many bits, nothing running */
void CAiEvent::Open(unsigned int bits)
{
    bit_count = bits;
    s_eventFlag = false;
    if (m_inuse.size())
        m_inuse.clear();
}

void CAiEvent::Close()
{
    bit_count = 32;
    FreeEvents();
    s_eventFlag = false;
}

/* a fresh event: from the pool if it has one, the next id given (SOCOM 1's) */
CAiEvent *CAiEvent::Acquire()
{
    CAiEvent *e;

    if (m_pool.size() == 0) {
        e = new CAiEvent;
    } else {
        e = m_pool.front();
        m_pool.pop_front();
        e->Initalize();
    }
    e->m_id = nextId++;
    return e;
}

/* an event made: running down its duration (at least the shortest) or kept for ever, and every
 * callback told (SOCOM 1's, the shortest duration new). The test is the inline IsPersistent:
 * written out, MW reuses its load of the duration for the clamp, where the target reloads it */
void CAiEvent::Create(CAiEvent *e)
{
    if (e->IsPersistent()) {
        m_persist.push_back(e);
    } else {
        if (e->m_duration < s_eventMinDuration)
            e->m_duration = s_eventMinDuration;
        m_inuse.push_back(e);
    }
    for (CAiEventCallbackList::iterator i = m_callbacks.begin(), end = m_callbacks.end(); i != end; ++i)
        (*i).m_func(e, (*i).m_data);
}

/* an event taken off its list and back to the pool (SOCOM 1's).
 * OPEN (right size, 5 instructions): only stack slots -- the target puts push_back's end
 * iterator and insert's result (sp+48/52) below the removes' equal_to temporaries (sp+56/60),
 * this build the other way round; a list-shape question (include/list is being remodelled) */
void CAiEvent::Release(CAiEvent *e)
{
    if (e->IsPersistent())
        m_persist.remove(e);
    else
        m_inuse.remove(e);
    m_pool.push_back(e);
}

const float CAiEvent::NoExpire = -99.0f;

/* the running events counted down, the expired ones back to the pool (SOCOM 1's function
 * object).
 * OPEN, operator()'s out-of-line copy (0x9C against 0x8C) and remove_copy_if (0x144 against 0xEC):
 * list heights. The target's operator() inlines Tick(float), keeps its bool in s0 and CALLS the
 * event list's push_back (0x50F810, which inlines end() around insert); remove_copy_if inlines the
 * predicate one level -- Tick(float) called -- and calls the pointer lists' BASE push_back
 * (0x50FE80: end() and insert called). include/list's list<T *>::push_back goes straight to the
 * base's insert, two high, so both inline it whole here: SOCOM II's must be taller (through the
 * base's push_back?), the entity lane's header to settle -- written as a bool local it is the
 * same 0x9C */
struct _tick {
    _tick(CAiEventList &pool, float dT) : m_pool(&pool), m_dT(dT) {}
    bool operator()(CAiEvent *e)
    {
        if (e->Tick(m_dT)) {
            m_pool->push_back(e);
            return true;
        }
        return false;
    }

    CAiEventList *m_pool;
    float m_dT;
};

/* the scheduler's task, "CAiEventTick" (SOCOM 1's, the time gathered new). The target compares
 * the gathered time with itself (c.lt.s $f0,$f0: MW reuses the value just stored to the global
 * for both operands -- whatever bound the original named, MW saw this variable) and branches
 * over the body to the one `return false`; the end iterator is a local taken before remove_if,
 * kept in s0 for the erase, and the erase a statement of its own (its end() after the call) */
bool CAiEvent::Tick(float dT, void *)
{
    s_eventTickTime += dT;
    if (s_eventTickTime >= s_eventTickTime) {
        float t = s_eventTickTime;

        s_eventTickTime = 0.0f;
        CAiEventList::iterator e = m_inuse.end();
        CAiEventList::iterator i = std::remove_if(m_inuse.begin(), e, _tick(m_pool, t));

        m_inuse.erase(i, e);
    }
    return false;
}

/* a walk over one list (SOCOM 1's) */
CAiEvent::iterator CAiEvent::GetFirstEvent(EVENT_CLASS c)
{
    switch (c) {
    case EC_INUSE:
        return iterator(&m_inuse);
    case EC_PERSIST:
        return iterator(&m_persist);
    }
    return iterator();
}

/* the walk's next event heard at a point: of a wanted kind, not the walk's owner's, not seen,
 * and within its range scaled (SOCOM 1's; SOCOM II measures the distance first). The kind is read
 * once into an unsigned short (andi 0xffff, then narrowed to the byte enum for the shift and the
 * kind test, and re-extended for the entity test -- SOCOM 1's code has the same three uses); the
 * kind test is `!(any || same)`: its fall-through gets the target's padding nop.
 * OPEN, 2 instructions (0x19C against 0x1A4): the seen test's two arms. The target fills the
 * kind compare's delay slot with the false arm's `move v0,zero`, leaves the `b` over the bit test
 * unfilled and pads the bit test's label to 8 (b; nop; nop), and schedules the mask before the
 * word (andi, srl, li, sllv, sll, lw: SOCOM 1's order too); this build steals the bit test's srl
 * into the delay slot instead. Every spelling of the test compiles to the same block here --
 * operand orders, the mask or the word in locals first, if/else on a bool, an early-return
 * inline taking the kind (build/scratch/ai/spec_gne.py) -- so it is structural, not the test */
CAiEvent *CAiEvent::GetNextEvent(const CPnt3D *pos, iterator &it, float scale)
{
    CAiEvent *found = 0;

    while (!found && it.m_it != it.m_list->end()) {
        CAiEvent *e = *it.m_it;

        ++it.m_it;
        unsigned short type = e->m_type;

        if (!(it.m_mask & (1 << (EVENT)type)))
            continue;
        if (!(it.m_type == AIE_MAX || it.m_type == (EVENT)type))
            continue;
        if (it.m_owner == e->m_owner)
            continue;
        if (it.m_bit != ~0 && (type == AIE_ENTITY ? false : e->m_bits.Test(it.m_bit)))
            continue;
        CPnt3D d;

        e->m_pos.Sub(pos, &d);
        float distSq = d.x * d.x + d.y * d.y + d.z * d.z;

        if (e->m_rangeSq == 0.0f || distSq < e->m_rangeSq * scale) {
            found = e;
            it.m_distSq = distSq;
        }
    }
    return found;
}

CAiEvent::CAiEvent()
    : m_bits(bit_count)
{
    Initalize();
}

/* an event as new (SOCOM 1's) */
void CAiEvent::Initalize()
{
    m_duration = 0.0f;
    m_pos = CPnt3D::zero;
    m_type = AIE_UNKNOWN;
    m_owner = -1;
    m_visible = 0;
    m_id = 0;
    m_userdata = 0;
    m_bits.ClearAll();
}

/* whether the event has run out, counting it down unless it never expires (SOCOM 1's) */
bool CAiEvent::Tick(float dT)
{
    if (m_duration == NoExpire)
        return false;
    m_duration -= dT;
    return m_duration <= 0.0f;
}

/* the kinds listed, AIE_NONE last, as a mask (SOCOM 1's) */
unsigned int CAiEvent::MakeMaskEx(EVENT e, ...)
{
    unsigned int mask = 0;
    va_list ap;

    va_start(ap, e);
    while (e != AIE_NONE) {
        mask |= 1 << e;
        e = va_arg(ap, EVENT);
    }
    va_end(ap);
    return mask;
}

/* the handle of an event, added if the queue has none (SOCOM 1's).
 * OPEN (right size, 9 instructions): the walk's registers and the push_back's temporaries (the
 * handle at sp+44, the end iterator at sp+36) -- list shapes, left for the list remodel */
CAiEventHandle *CAiEventQ::Create(unsigned int id)
{
    CAiEventHandle *h = 0;

    for (iterator i = begin(); i != end(); ++i) {
        if ((*i).m_id == id) {
            h = &*i;
            break;
        }
    }
    if (!h) {
        push_back(CAiEventHandle(id));
        h = &((node *)end_.prev_)->data_;
    }
    return h;
}
