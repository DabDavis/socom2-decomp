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
    Rfloat() {}
    /* a fixed value and a range from a minimum to a maximum (inline: OVL2's out-of-line copies are
     * 0x5F6F30, CAiState::Create's, and 0x5609D0) */
    Rfloat(float f) : m_min(f), m_range(0.0f) {}
    Rfloat(float min, float max) : m_min(min), m_range(max - min) {}
    Rfloat &operator=(float f)
    {
        m_min = f;
        m_range = 0.0f;
        return *this;
    }
    /* through a local, and the random factor through one of its own, as the target's registers
     * and add order want (CAiSStunResponse::OnSignal) */
    operator float() const
    {
        float f;

        if (m_range == 0.0f)
            f = m_min;
        else {
            float r = 4.656613e-10f * rand();
            f = m_min + m_range * r;
        }
        return f;
    }

    float m_min;   // 0x0
    float m_range;   // 0x4
};

/* a trigger: an up time and a down time, the next switch and the mark it was set from (SOCOM 1's
 * DWARF: CAiTrigger, 0x18 bytes; its constructor sets the default times, SOCOM 1's inline) */
class CAiTrigger {
public:
    /* too big to inline whole where it is used (the squirm's constructor, SEAL_AI_PARAMS()): its
     * members' constructors, the temporaries' and SetTimes are called out of line */
    CAiTrigger() : m_up_time(0.0f), m_dn_time(0.0f) { SetTimes(Rfloat(0.5f, 1.2f), Rfloat(0.1f, 0.5f)); }
    /* the times set and the first switch drawn (inline: its out-of-line copy is OVL2 0x5FF200,
     * seal_squirm.cpp's, after the squirm's constructor; the name ours) */
    void SetTimes(const Rfloat &dn, const Rfloat &up)
    {
        m_dn_time = dn;
        m_up_time = up;
        m_marker = m_up_time;
        m_timer = m_marker + m_dn_time;
    }

    Rfloat m_up_time;   // 0x0
    Rfloat m_dn_time;   // 0x8
    float m_timer;   // 0x10
    float m_marker;   // 0x14
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

    unsigned int m_250;   // 0x250 (cleared by the constructor; the names ours)
    unsigned char m_254;   // 0x254 (cleared by the constructor, set by Initalize)
    unsigned char _255[0x258 - 0x255];
    CAiTrigger m_trigger;   // 0x258: the rest (up) and squirm (down) times
    CAiTimer m_timer;   // 0x270
    unsigned char _27c[0x280 - 0x27c];   // 0x280 bytes in all: ftsCreateSeal and
                                         // CZSealBody::NetworkPlayerCreate new 640
};

#endif
