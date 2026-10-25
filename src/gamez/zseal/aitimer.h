/* aitimer.h -- the AI's random period and its countdown (SOCOM 1's DWARF: CAiTimer, files it under
 * gamez/zseal/zseal.h). Its own header so ai/ai_states.h can hold real CAiTimer members: zseal.h
 * includes this ahead of ai_states.h. */
#ifndef AITIMER_H
#define AITIMER_H

#include <stdlib.h>

/* an Rfloat {m_min, m_range} and the timer; 0xc bytes */
class CAiTimer {
public:
    /* (empty: the classes holding one -- CSealCtrl, CTurret -- build it field by field) */
    CAiTimer() {}
    /* a fixed period, the countdown started (the kit lane's request: OVL2 0x5EA550 is its out-of-line
     * copy -- the range made t - t, the timer zeroed, then the period drawn) */
    CAiTimer(float t)
    {
        m_min = t;
        m_range = t - t;
        m_timer = 0.0f;
        m_timer = Period();
    }

    /* a period: the minimum, plus a random part of the range when there is one (SOCOM 1's Rfloat's
     * operator float, through a result local and the random factor's own) */
    float Period() const
    {
        float f;

        if (m_range == 0.0f)
            f = m_min;
        else {
            float r = 4.656612873e-10f * (float)rand();

            f = m_min + m_range * r;
        }
        return f;
    }

    float m_min;   // 0x0
    float m_range;   // 0x4
    float m_timer;   // 0x8

    /* the never-expiring period (SOCOM 1's infinite__8CAiTimer; OVL2 data, read as a variable) */
    static float infinite;
};

#endif
