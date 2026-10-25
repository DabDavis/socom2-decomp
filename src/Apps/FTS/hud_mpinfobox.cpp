/* hud_mpinfobox.cpp -- the network game's info box (OVL1 0x238420-...): SOCOM II's unit (no SOCOM 1
 * counterpart; the file name is ours), after the ammo counts' and before the radio icons' Ticks:
 * CMPInfoBox's Off, On, Tick, the clean-up CHUD::ReInit_Uninit calls and the re-init
 * ReInit_Init calls, then CWhosTalking's Tick and the radio icons' (0x239250, 0x2393F0). Only Off,
 * On, the clean-up and the static radio icon's Tick so far. Defined in reverse of their image
 * order (-inline deferred). */

#include "hud_mpinfobox.h"

class CZNetwork {
public:
    int m_SealIndex;   // 0x0
    int m_TerrIndex;   // 0x4
    unsigned char m_bSessionMaster;   // 0x8
    unsigned char m_bNetwork;   // 0x9
};

extern CZNetwork theNetwork;

/* the radio's state (names ours): what the icons read of it */
class CRadioState {
public:
    unsigned char _0[0x2c];
    int m_channel;   // 0x2c: 0 team, 1 defense, 2 offense, 3 dead, 10 SOCOM II's own
};

/* the radio (no data row yet) and whether it is on (no name yet: it tests the state at 0x24
 * against 11) */
extern CRadioState *data_0045B718;
extern "C" bool func_0030E540(CRadioState *radio);
/* the local seal, or so (no data row yet): the icons show only while it is set */
extern void *data_00437CE8;

/* where the static icon sits and how big the icons are (variables: read with lwc1) */
static float s_staticx = 590.0f;
static float s_staticy = 361.0f;
static float s_iconw = 32.0f;
static float s_iconh = 32.0f;

/* in a network game, the radio's channel shown (or the radio off); hidden otherwise. The unit's
 * last function: the radio icons' Ticks follow CWhosTalking's here (SOCOM 1 kept the radio
 * bitmaps in hud_actionbitmap.cpp) */
void CRadioIcon_Static::Tick(float dT)
{
    if (!theNetwork.m_bNetwork)
        return;
    zdb::CTexHandle *tex = 0;

    if (!func_0030E540(data_0045B718)) {
        tex = m_pRadioOffTexture;
    } else {
        int channel = data_0045B718->m_channel;

        if (channel == 0)
            tex = m_pRadioChannelTeamTexture;
        else if (channel == 1)
            tex = m_pRadioChannelDefenseTexture;
        else if (channel == 2)
            tex = m_pRadioChannelOffenseTexture;
        else if (channel == 3)
            tex = m_pRadioChannelDeadTexture;
        else if (channel == 10)
            tex = m_pRadioChannel10Texture;
    }
    if (data_00437CE8 && tex) {
        TickFade();
        if (tex != m_pTexHandle) {
            Load(s_staticx, s_staticy, s_staticx + s_iconw, s_staticy + s_iconh, tex);
            On();
        } else if (!IsOn()) {
            On();
        }
    } else {
        Off();
    }
}

/* the strings emptied and the radio's parts hidden */
void CMPInfoBox::UnInit()
{
    Off();
    m_text.SetString(0, true);
    m_whostalking.m_text1.Off();
    m_whostalking.m_text1.SetString(0, true);
    m_whostalking.m_bitmap2.Off();
    m_whostalking.m_text2.Off();
    m_whostalking.m_text2.SetString(0, true);
}

void CMPInfoBox::On()
{
    C2D::On();
    m_bitmap2.On();
    m_healthbar.On();
    m_text.On();
    m_whostalking.On();
}

void CMPInfoBox::Off()
{
    C2D::Off();
    m_bitmap2.Off();
    m_healthbar.Off();
    m_text.Off();
    m_whostalking.Off();
}
