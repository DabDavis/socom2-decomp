/* hud_mpinfobox.cpp -- the network game's info box (OVL1 0x238420-...): SOCOM II's unit (no SOCOM 1
 * counterpart; the file name is ours), after the ammo counts' and before the radio icons' Ticks:
 * CMPInfoBox's Off, On, Tick, the clean-up CHUD::ReInit_Uninit calls and the re-init
 * ReInit_Init calls, then CWhosTalking's Tick and the radio icons' (0x239250, 0x2393F0). Only Off,
 * On, the clean-up and the static radio icon's Tick so far. Defined in reverse of their image
 * order (-inline deferred). */

#include "hud_mpinfobox.h"

#include <stdio.h>
#include <string.h>

class CZNetwork {
public:
    unsigned char m_bSessionMaster;   // 0x0 (SOCOM 1's at 0x8: SOCOM II dropped the two team indices)
    unsigned char m_bNetwork;   // 0x1
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

/* the HUD's owner, as the box reads it: its name and its player's index */
class CZSealBody {
public:
    unsigned char _0[0x14];
    const char *m_name;   // 0x14
    unsigned char _18[0xfc8 - 0x18];
    int m_player;   // 0xfc8
};

class CHUD {
public:
    static CZSealBody *GetHUDOwner();
};

/* the health bar's update from its owner (its own unit's; no name yet) */
extern "C" void func_00241CC0(CHealthBar *bar, CZSealBody *owner, bool b);
/* a player's clan tag, or "" (the player list's; no name yet) */
extern "C" const char *func_002C3AC0(int player);
/* a string's width measured into m_charheight (its slot 0x3C; ztwod's, no name yet) */
extern "C" void func_00362B70(C2DString *s);

/* the name's room in the box (a variable: read with lwc1) */
static float s_namewidth = 134.0f;

/* a seal's clan tag, if it has a seal */
static inline const char *ClanTag(CZSealBody *owner)
{
    return owner ? func_002C3AC0(owner->m_player) : 0;
}

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

/* the owner's health, then its name (the clan's tag ahead of it, cut short with a dash when it
 * runs past the box) and who is talking.
 * OPEN, 11 instructions, all one swap of s1 and s2: the target keeps the owner in s1 and the clan
 * tag in s2 (the name then reusing s1), this the other way round. Tried: the tag uninitialised
 * ahead of the owner or after it, zero-initialised and set in an if (8 bytes longer), the name
 * declared first or not a local, the health bar's update as a member call -- the same swap. */
void CMPInfoBox::Tick(float dT)
{
    CZSealBody *owner = CHUD::GetHUDOwner();

    func_00241CC0(&m_healthbar, owner, true);
    m_healthbar.Tick(dT);
    const char *clan = ClanTag(owner);
    const char *name = owner->m_name;
    char text[64];

    if (clan && strlen(clan))
        sprintf(text, "[%s] %s", clan, name);
    else
        strcpy(text, name);
    m_text.SetString(text, true);
    func_00362B70(&m_text);
    if (m_text.m_charheight > s_namewidth - 4.0f) {
        char cut[64];

        strcpy(cut, m_text.m_string + m_text.m_firstCharPos);
        cut[9] = '-';
        cut[10] = 0;
        m_text.SetString(cut, true);
        func_00362B70(&m_text);
    }
    m_whostalking.Tick(dT);
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
