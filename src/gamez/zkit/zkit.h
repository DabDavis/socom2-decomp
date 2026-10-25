/* zkit.h -- the SEAL's kit (CZKit) and the controllers and squads around the SEAL that this lane
 * writes: the network's controller (CRemoteCtrl), the squad (CSealUnit), the squirming hostage's
 * controller (CSealCtrlSquirm). SOCOM 1 files them all under gamez/zseal (seal_kit.cpp,
 * remote_control.cpp, seal_unit.cpp, seal_squirm.cpp); zseal.h is the SEAL lane's, so this lane's
 * classes live here (the header's name and directory are ours). Layouts are SOCOM 1's DWARF moved
 * to where SOCOM II's code puts each member; only what a written function touches is named. */
#ifndef ZKIT_H
#define ZKIT_H

#include <stdlib.h>

#include "../zseal/zseal.h"

/* the network's controller, CRemoteCtrl, is declared in zseal.h (the SEAL lane's seal_ai.cpp
 * defines its destructor) */

/* a random float: a minimum and a range (SOCOM 1's DWARF, 8 bytes; its operator float and
 * operator= are SOCOM 1's __opf__6RfloatCFv and __as__6RfloatFf, inlined here) */
class Rfloat {
public:
    Rfloat &operator=(float f)
    {
        m_min = f;
        m_range = 0.0f;
        return *this;
    }
    /* through a local, as the target's registers and add order want */
    operator float() const
    {
        float f;

        if (m_range == 0.0f)
            f = m_min;
        else
            f = m_min + m_range * (4.656613e-10f * rand());
        return f;
    }

    float m_min;   // 0x0
    float m_range;   // 0x4
};

/* the hostage's controller: a captive who squirms (SOCOM 1's CSealCtrlSquirm : CSealCtrl;
 * SOCOM II's vtable 0x6699F0) */
class CSealCtrlSquirm : public CSealCtrl {
public:
    CSealCtrlSquirm();
    virtual ~CSealCtrlSquirm();
    virtual bool PreTick(float dT);
    virtual bool Tick(float dT);
    virtual bool PostMortemPreTick(float dT);
    virtual void OnDeath();
    virtual void Initalize();
    virtual void OnWeaponHit(zdb::IntersectStruct *hit, CZProjectile *proj);
    virtual void OnStunDamage(STUN_TYPE type, float amount, const CPnt3D *from);
    virtual void OnSetHealth(float health, float old);
    virtual void ResetToInitialState();
    virtual void OnActionButton();

    unsigned char _250[0x254 - 0x250];
    unsigned char m_254;   // 0x254 (cleared by the constructor, set by Initalize; the names ours)
    unsigned char _255[0x258 - 0x255];
    Rfloat m_rest;   // 0x258
    Rfloat m_squirm;   // 0x260
    float m_next;   // 0x268
    float m_rest_until;   // 0x26c
    Rfloat m_timer;   // 0x270
    float m_timer_value;   // 0x278
};

#endif
