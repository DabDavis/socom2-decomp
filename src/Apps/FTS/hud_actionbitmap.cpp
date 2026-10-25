/* hud_actionbitmap.cpp -- the HUD's action and radio icons (OVL1 ...-0x23A1xx): SOCOM 1's unit (its
 * DWARF names it, Z:\dev\Apps\FTS\hud_actionbitmap.cpp). SOCOM II renamed the radio bitmaps
 * (CZStaticRadioBitmap and CZDynamicRadioBitmap are CRadioIcon_Static and CRadioIcon_Dynamic in
 * its RTTI) and ticks them through C2D's Tick slot. Only the static icon's Tick so far, with the
 * implicit destructor its vtable brings. Defined in reverse of their image order (-inline
 * deferred). */

#include "../../gamez/ztwod/ztwod.h"

class CHUD;

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

/* SOCOM 1's DWARF layout; a sixth texture where m_pParent was */
class CRadioIcon_Static : public C2DBitmap {
public:
    virtual void Tick(float dT);

    zdb::CTexHandle *m_pRadioOffTexture;   // 0x80
    zdb::CTexHandle *m_pRadioChannelTeamTexture;   // 0x84
    zdb::CTexHandle *m_pRadioChannelOffenseTexture;   // 0x88
    zdb::CTexHandle *m_pRadioChannelDefenseTexture;   // 0x8c
    zdb::CTexHandle *m_pRadioChannelDeadTexture;   // 0x90
    zdb::CTexHandle *m_pRadioChannel10Texture;   // 0x94 (SOCOM II's; the name ours)
};   // 0xa0 bytes

/* where the static icon sits and how big the icons are (variables: read with lwc1) */
static float s_staticx = 590.0f;
static float s_staticy = 361.0f;
static float s_iconw = 32.0f;
static float s_iconh = 32.0f;

/* in a network game, the radio's channel shown (or the radio off); hidden otherwise */
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
