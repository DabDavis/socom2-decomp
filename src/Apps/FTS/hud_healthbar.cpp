/* hud_healthbar.cpp -- the network game's health bar (OVL1 0x241C30-0x2425D3): SOCOM II's unit (no
 * SOCOM 1 counterpart; the file name is ours), after the network weapon selector's: CHealthBar's
 * SetTrans (slot 11), SetPos and SetSize, then Update (0x241CC0), the Tick (slot 12, 0x242050), the
 * destructor (0x242300) and the constructor (0x2423D0). Its initialiser (0x3FF9C0) builds the five
 * colours. Defined in reverse of their image order (-inline deferred). */

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
/* the bar's colours (the unit's statics, built by its initialiser 0x3FF9C0; the names ours): the
 * health left, the flash it starts from, the rest of the bar alive, the part just lost, and the rest
 * of the bar dead */
static CPnt3D s_hbHealthColor(0.0f, 128.0f, 64.0f);
static CPnt3D s_hbFlashColor(128.0f, 128.0f, 128.0f);
static CPnt3D s_hbRestColor(128.0f, 64.0f, 64.0f);
static CPnt3D s_hbLostColor(128.0f, 128.0f, 64.0f);
static CPnt3D s_hbDeadColor(100.0f, 100.0f, 100.0f);
/* the constructor's and the Tick's tuning: plain initialised data 8 bytes apart (0x3DCED8-0x3DCF08),
 * MW 3.0's layout of function-local statics -- written as file statics, which scdiff compares by
 * content (the names ours): the alphas of the health left and of the rest of the bar, the lost
 * part's, the flash's fade speed, the pause before the lost part drains, its drain rate (bar widths
 * a second), and whether a rise flashes the bar */
static float s_healthtrans = 78.6f;
static float s_resttrans = 78.6f;
static float s_losttrans = 128.0f;
static float s_flashspeed = 3.0f;
static float s_draindelay = 0.6f;
static float s_drainrate = 1.0f / 1.85f;
static bool s_hbFlash = true;

CHealthBar::CHealthBar()
{
    m_health = -1.0f;
    m_x = 0.0f;
    m_y = 0.0f;
    m_w = 0.0f;
    m_h = 0.0f;
    m_bitmap1.SetColor(s_hbHealthColor);
    m_bitmap1.SetTrans(s_healthtrans);
    m_bitmap1.SetHasTexture(false);
    m_bitmap1.On();
    push_back(&m_bitmap1);
    m_bitmap2.SetColor(s_hbRestColor);
    m_bitmap2.SetTrans(s_resttrans);
    m_bitmap2.SetHasTexture(false);
    m_bitmap2.On();
    push_back(&m_bitmap2);
    m_bitmap3.SetColor(s_hbLostColor);
    m_bitmap3.SetTrans(0.0f);
    m_bitmap3.SetHasTexture(false);
    m_bitmap3.On();
    push_back(&m_bitmap3);
    m_flash = 0;
    m_1b1 = 0;
    m_draining = 0;
    SetCentered(false);
    Set1b4(0);
    m_delay = 0.0f;
    m_drainend = 0.0f;
    m_drainx = 0.0f;
    m_drainw = 0.0f;
    m_owner = 0;
}

CHealthBar::~CHealthBar()
{
}

/* a difference by value (an inline: its return slot sits below the named locals; the name ours) */
static inline CPnt3D Difference(const CPnt3D &a, const CPnt3D &b)
{
    CPnt3D d;

    a.Sub(&b, &d);
    return d;
}

/* the flash faded back to the bar's colour, and the lost part drained after its pause */
void CHealthBar::Tick(float dT)
{
    switch (m_flash) {
    case 1:
        if (s_hbFlash) {
            m_bitmap1.SetColor(s_hbFlashColor);
            m_flash = 2;
        } else
            m_flash = 0;
        break;
    case 2: {
        CPnt3D color = *(CPnt3D *)m_bitmap1.GetColor();
        CPnt3D step = Difference(s_hbFlashColor, s_hbHealthColor);

        step.Scale(s_flashspeed * dT, &step);
        color.Sub(&step, &color);
        if (color.x < s_hbHealthColor.x) {
            color = s_hbHealthColor;
            m_flash = 0;
        }
        m_bitmap1.SetColor(color);
        break;
    }
    }
    switch (m_draining) {
    case 1:
        m_delay = s_draindelay;
        m_drainend = 0.0f;
        m_drainx = m_centered ? m_x - 0.5f * m_w : m_x;
        m_drainw = (float)m_bitmap3.m_iWidth;
        m_drainx += m_w * m_health;
        m_bitmap3.SetTrans(s_losttrans);
        m_draining = 2;
        break;
    case 2:
        if (m_delay < 0.0f) {
            m_drainend += dT * (m_w * s_drainrate);
            if (m_drainend > m_drainw) {
                m_drainend = m_drainw;
                m_draining = 0;
            }
            m_bitmap3.SetPos((int)m_drainx, (int)m_y, (int)(m_drainx + (m_drainw - m_drainend)),
                             (int)(m_y + m_h));
        }
        if (m_delay > 0.0f)
            m_delay -= dT;
        break;
    }
}

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
        m_bitmap2.SetColor(s_hbDeadColor);
    else
        m_bitmap2.SetColor(s_hbRestColor);
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
