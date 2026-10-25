/* hud_healthbar.cpp -- the network game's health bar (OVL1 0x241C30-...): SOCOM II's unit (no SOCOM
 * 1 counterpart; the file name is ours), after the network weapon selector's: CHealthBar's SetTrans
 * (slot 11), SetPos and SetSize, then Update (0x241CC0), the Tick (slot 12, 0x242050), the
 * destructor (0x242300) and the constructor (0x2423D0) -- the HUD lane's range ends at 0x242000,
 * so the first four so far. Defined in reverse of their image order (-inline deferred). */

#include "hud_mpinfobox.h"

class CZNetwork {
public:
    unsigned char m_bSessionMaster;   // 0x0
    unsigned char m_bNetwork;   // 0x1
};

extern CZNetwork theNetwork;

/* the owner as the bar reads it: its health, 0 to 1 */
class CZSealBody {
public:
    unsigned char _0[0x1044];
    float m_health;   // 0x1044
};

/* whether a seal is dead (the player list's; no name yet) */
extern "C" bool func_002C2FD0(CZSealBody *seal);
/* the bar's colours alive and dead (the unit's statics, built by its initialiser; no rows yet) */
extern CPnt3D data_004148D0;
extern CPnt3D data_004148F0;

/* the bar set from its owner's health: a new owner, a rise or a death redraws it at once; a fall
 * leaves the lost part showing, draining, from where the bar ended. The name is ours */
void CHealthBar::Update(CZSealBody *owner, bool force)
{
    bool reset = false;
    bool fell = false;
    bool dead = false;
    float health = owner ? owner->m_health : 0.0f;

    if (theNetwork.m_bNetwork && func_002C2FD0(owner)) {
        dead = true;
        health = 0.0f;
        reset = dead;
    } else if (m_owner != owner) {
        reset = true;
    } else if (owner) {
        if (m_health < health)
            reset = true;
        else if (m_health > health)
            fell = true;
    }
    if (m_x != (float)m_bitmap1.m_x || m_y != (float)m_bitmap1.m_y) {
        fell = true;
        if (m_draining) {
            m_drainx = m_centered ? m_x - 0.5f * m_w : m_x;
            m_drainx += m_w * m_health;
        }
    }
    if (dead)
        m_bitmap2.SetColor(data_004148F0);
    else
        m_bitmap2.SetColor(data_004148D0);
    if (reset || fell) {
        float y;
        float x;

        if (m_centered)
            x = m_x - 0.5f * m_w;
        else
            x = m_x;
        y = m_y;
        int top = (int)y;
        int right = (int)(x + m_w * health);

        m_bitmap1.SetPos((int)x, top, right, (int)(y + m_h));
        m_bitmap2.SetPos(right, top, (int)(x + m_w), (int)(y + m_h));
        if (fell) {
            if (!m_draining) {
                m_bitmap3.SetPos(right, top, (int)(x + m_w * m_health), (int)(y + m_h));
                m_draining = 1;
            } else {
                float end = x + m_w * health;

                m_drainw += m_drainx - end;
                m_drainx = end;
                m_bitmap3.SetPos((int)m_drainx, (int)m_y, (int)(m_drainx + (m_drainw - m_drainend)),
                                 (int)(m_y + m_h));
            }
            m_bitmap3.On();
        }
        if (reset) {
            m_draining = 0;
            m_bitmap3.Off();
        }
    }
    if (!fell && !m_draining)
        m_bitmap3.Off();
    m_owner = owner;
    m_health = health;
}

void CHealthBar::SetSize(float w, float h)
{
    m_w = w;
    m_h = h;
}

void CHealthBar::SetPos(float x, float y)
{
    m_x = x;
    m_y = y;
}

void CHealthBar::SetTrans(float trans)
{
    m_bitmap1.SetTrans(trans);
    m_bitmap2.SetTrans(trans);
    m_bitmap3.SetTrans(trans);
}
