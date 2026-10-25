/* zwep_global.cpp -- the weapon events' callbacks (SOCOM 1's zwep_global.cpp, by its static
 * initialiser, which builds m_callbacks): OVL1 0x3C63D0-0x3C69BF, after CZFTSWeapon and before
 * CFlyoutMachine, as SOCOM 1's (0x315F40 in the demo). Only the walk and the append so far: the
 * list's template members (remove_if, erase, insert) are include/list's to model. Defined in
 * reverse of their image order. */

#include <list>

#include "zweapon.h"

/* a weapon event's callback (SOCOM 1's struct: the function, the events it wants, its data) */
struct WEAPON_CALLBACK_STRUCT {
    void (*m_callback)(CZProjectile *proj, unsigned int event, void *data);   // 0x0
    unsigned int m_events;   // 0x4
    void *m_data;   // 0x8
};

/* the callbacks (SOCOM 1's CWepCallbackList : list) */
class CWepCallbackList : public std::list<WEAPON_CALLBACK_STRUCT> {
};

extern CWepCallbackList m_callbacks;

void CZWeapon::AddWeaponEventCallback(void (*callback)(CZProjectile *, unsigned int, void *),
                                      unsigned int events, void *data)
{
    WEAPON_CALLBACK_STRUCT cb;

    cb.m_callback = callback;
    cb.m_events = events;
    cb.m_data = data;
    m_callbacks.push_back(cb);
}

void CZWeapon::MakeWeaponEventCallbacks(unsigned int event, CZProjectile *proj)
{
    CWepCallbackList::iterator e = m_callbacks.end();

    for (CWepCallbackList::iterator it = m_callbacks.begin(); it != e; ++it) {
        WEAPON_CALLBACK_STRUCT &cb = *it;

        if (cb.m_events & event)
            cb.m_callback(proj, event, cb.m_data);
    }
}
