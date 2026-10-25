/* ai.h -- the AI core (OVL2): the events and their callbacks, the path planners, the AI map. The
 * layouts are SOCOM 1's DWARF where it has them, moved where SOCOM II's code puts them; each
 * difference is said where it is. The directory is ours (SOCOM 1's DWARF names no AI file; its
 * unit names -- ai_events.cpp, ai_astar.cpp, ai_map.cpp, ai_mapio.cpp, ai_path.cpp, ai_maps.cpp,
 * ai_vismap.cpp -- come from its static initialisers and the press beta's asserts). */
#ifndef AI_H
#define AI_H

#include <list>

#include "../gamez/zmath/zmath.h"

class CAiEvent;

/* every event of a kind, or the pool of free ones (SOCOM 1: a list of pointers and nothing more) */
class CAiEventList : public std::list<CAiEvent *> {
};

/* a function told of each event made (SOCOM 1's; the names are ours) */
class CAiEventCallback {
public:
    void (*m_func)(CAiEvent *e, unsigned int data);   // 0x0
    unsigned int m_data;   // 0x4
};

class CAiEventCallbackList : public std::list<CAiEventCallback> {
public:
    /* declared, not defined here: implicit, it calls ~__list_imp<CAiEventCallback> (0x50FFE0),
     * whose target copy calls begin() and end() out of line -- include/list's inlines them (0xB8
     * against 0x80), a list destructor height to settle with the list remodel. Until then the
     * generated draft is 0x5101C0 and its callee */
    ~CAiEventCallbackList();
};

/* an event a SEAL has heard of, by id: two bytes in SOCOM II (SOCOM 1's was eight, a word of
 * flags after the id) -- the list node is 12 bytes and copies it with lhu/sh */
class CAiEventHandle {
public:
    CAiEventHandle(unsigned int id) : m_id(id) {}

    unsigned short m_id;   // 0x0
};

/* the events a SEAL is handling (SOCOM 1's DWARF: a list of handles and a flag) */
class CAiEventQ : public std::list<CAiEventHandle> {
public:
    CAiEventHandle *Create(unsigned int id);

    unsigned char m_diRequested;   // 0xc
};

/* something the AI can notice: a noise, a body, a door, a shot (SOCOM 1's DWARF, 0x28 bytes, the
 * same in SOCOM II). The events live in three lists: the free pool, the ones running down their
 * duration and the ones that never expire */
class CAiEvent {
public:
    /* SOCOM II's thirteen kinds (GetEventString's cases; SOCOM 1 had fifteen in another order:
     * SURRENDER, GETDOWN and HELPATTACK went, HELPME came) -- a signed byte (-enum min, AIE_NONE) */
    enum EVENT {
        AIE_NONE = -1,
        AIE_UNKNOWN = 0,
        AIE_GENERIC = 1,
        AIE_POINT = 2,
        AIE_ENTITY = 3,
        AIE_VOICE = 4,
        AIE_CORPSE = 5,
        AIE_WEPHIT = 6,
        AIE_WEPFIRE = 7,
        AIE_EXPLOSION = 8,
        AIE_HELPME = 9,
        AIE_ALARM = 10,
        AIE_LIGHTSOFF = 11,
        AIE_DOOR = 12,
        AIE_MAX = 13
    };
    /* which list a walk reads (the names are ours) */
    enum EVENT_CLASS { EC_INUSE = 0, EC_PERSIST = 1 };

    /* a walk over one list, filtered (SOCOM 1's CAiEvent::iterator; the names are ours) */
    class iterator {
    public:
        iterator()
        {
            m_list = 0;
            m_type = AIE_MAX;
            m_mask = ~0;
            m_owner = -1;
            m_bit = ~0;
        }
        iterator(CAiEventList *list)
        {
            m_list = list;
            m_it = m_list->begin();
            m_mask = ~0;
            m_owner = -1;
            m_bit = ~0;
            m_type = AIE_MAX;
        }

        CAiEventList *m_list;   // 0x0
        CAiEventList::iterator m_it;   // 0x4
        unsigned int m_bit;   // 0x8: events that have it set are skipped (~0: none)
        int m_owner;   // 0xc: this owner's events are skipped
        EVENT m_type;   // 0x10: the kind wanted (AIE_MAX: any)
        unsigned int m_mask;   // 0x14: the kinds wanted, a bit each
        float m_distSq;   // 0x18: the last event found, its distance squared
    };

    CAiEvent();
    void Initalize();
    bool Tick(float dT);
    bool IsPersistent() const { return m_duration == NoExpire; }
    /* whether a walk has already seen this event: its bit, which an entity's event never has
     * (GetNextEvent's test; the name is ours) */
    bool Seen(unsigned int bit) const { return m_type == AIE_ENTITY ? false : m_bits.Test(bit); }

    static unsigned int MakeMaskEx(EVENT e, ...);
    static CAiEvent *GetNextEvent(const CPnt3D *pos, iterator &it, float scale);
    static iterator GetFirstEvent(EVENT_CLASS c);
    static bool Tick(float dT, void *data);
    static void Release(CAiEvent *e);
    static void Create(CAiEvent *e);
    static CAiEvent *Acquire();
    static void Close();
    static void Open(unsigned int bits);
    static void FreeEvents();
    static EVENT GetEventFromString(const char *s, EVENT def);
    static float GetEventRange(EVENT e);
    static float GetEventRate(EVENT e);
    static const char *GetEventString(EVENT e);

    CPnt3D m_pos;   // 0x0
    float m_rangeSq;   // 0xc: 0 = heard everywhere
    float m_duration;   // 0x10: NoExpire = forever
    int m_owner;   // 0x14
    unsigned int m_id : 16;   // 0x18
    unsigned int m_visible : 1;   // 0x1a
    unsigned int m_type : 15;
    unsigned int m_userdata;   // 0x1c
    CBits m_bits;   // 0x20: which walks have seen it

    static const float NoExpire;
    static unsigned int bit_count;
    static unsigned int nextId;
    static CAiEventList m_pool;
    static CAiEventList m_inuse;
    static CAiEventList m_persist;
    static CAiEventCallbackList m_callbacks;
};

#endif
