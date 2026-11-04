/* zwep_global.cpp -- the weapon events' callbacks (SOCOM 1's zwep_global.cpp, by its static
 * initialiser, which builds m_callbacks): OVL1 0x3C63D0-0x3C69BF, after CZFTSWeapon and before
 * CFlyoutMachine, as SOCOM 1's (0x315F40 in the demo). The three functions, the template tail's
 * erase, insert and remove_copy_if (0x3C6610-0x3C685F), the deleter's constructor, ~CWepCallbackList
 * (0x3C6920) and the initialiser (0x404B70) from source; the rest of the tail (~__list_imp 0x3C6880,
 * the begin/end copies) are drafts.
 * std::remove_if's instance (0x3C6560) is OPEN, 0x10 bytes long: the target uses its predicate
 * parameter in place (`sw a3,44(sp)`, compared from there), where include/algorithm's find_if
 * copies it first (a 4-byte block copy through $f0) -- and find_if's first/last take the other
 * registers, include/algorithm's open remove_if (0x29FE70) again. Defined in reverse of their
 * image order. */

#include <list>
#include <algorithm>

#include "zweapon.h"

/* a weapon event's callback (SOCOM 1's struct: the function, the events it wants, its data) */
struct WEAPON_CALLBACK_STRUCT {
    /* empty and user-declared: the list's insert then copies an element member by member (lw/sw
     * interleaved) where an implicit one is a block copy through the FPRs */
    WEAPON_CALLBACK_STRUCT() {}

    void (*m_callback)(CZProjectile *proj, unsigned int event, void *data);   // 0x0
    unsigned int m_events;   // 0x4
    void *m_data;   // 0x8
};

/* the callbacks (SOCOM 1's CWepCallbackList : list) */
class CWepCallbackList : public std::list<WEAPON_CALLBACK_STRUCT> {
};

/* defined here (round 17): the unit's initialiser (0x404B70) builds it through the list deleter's
 * constructor (0x3C6860, out of line) and registers ~CWepCallbackList (0x3C6920 -- the base
 * __list_imp's destructor 0x3C6880 called on it, then the delete on the flag), all three from
 * source now. The tail's ~__list_imp (0x3C6880) stays its draft's: include/list's inlines the clear
 * (0xB8) where the target calls begin(), end() and the range erase (0x3C6910, 0x3C6900, 0x3C6610)
 * out of line -- README "Level 6, or 8?", ~__list_deleter written `erase(begin(), end())` */
CWepCallbackList m_callbacks;

/* a callback's test by its function (SOCOM 1's predicate, its remove_if's) */
struct _compare_callback {
    _compare_callback(void (*callback)(CZProjectile *, unsigned int, void *)) : m_callback(callback) {}
    bool operator()(const WEAPON_CALLBACK_STRUCT &cb) { return cb.m_callback == m_callback; }

    void (*m_callback)(CZProjectile *, unsigned int, void *);
};

void CZWeapon::AddWeaponEventCallback(void (*callback)(CZProjectile *, unsigned int, void *),
                                      unsigned int events, void *data)
{
    WEAPON_CALLBACK_STRUCT cb;

    cb.m_callback = callback;
    cb.m_events = events;
    cb.m_data = data;
    m_callbacks.push_back(cb);
}

/* the callbacks of that function taken out, or every callback when given none (SOCOM 1's took out
 * the function's alone) */
void CZWeapon::RemoveWeaponEventCallback(void (*callback)(CZProjectile *, unsigned int, void *))
{
    if (!callback)
        m_callbacks.clear();
    else {
        CWepCallbackList::iterator e = m_callbacks.end();
        CWepCallbackList::iterator i = std::remove_if(m_callbacks.begin(), e, _compare_callback(callback));

        m_callbacks.erase(i, e);
    }
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
