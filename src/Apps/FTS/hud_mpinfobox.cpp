/* hud_mpinfobox.cpp -- the network game's info box (OVL1 0x238420-...): SOCOM II's unit (no SOCOM 1
 * counterpart; the file name is ours), after the ammo counts' and before the radio icons' Ticks:
 * CMPInfoBox's Off, On, Tick, the clean-up CHUD::ReInit_Uninit calls (UnInit), the re-init
 * ReInit_Init calls (ReInit) and Init (0x2388A0), then CWhosTalking's Tick (0x238AC0), ReInit and
 * Init (0x238F50), the radio icons' Ticks (0x239250, 0x2393F0) and the static icon's Init
 * (0x239570; the dynamic one's is inlined into CWhosTalking's), and the tail's basic_string(const
 * char *) (0x2396F0: out of class in MSL, so a call; the dynamic icon's Tick builds one). All
 * written, byte-exact but CMPInfoBox::Tick and CWhosTalking::Init (OPEN). Defined in reverse of
 * their image order (-inline deferred). */

#include "hud_mpinfobox.h"

#include <stdio.h>
#include <string.h>
#include <string>

namespace zdb {
class CAssetList {
public:
    CTexHandle *GetTexHandle(const char *name);
};

class CAssetMgr {
public:
    static CAssetList m_assets;
};
}

class CZNetwork {
public:
    unsigned char m_bSessionMaster;   // 0x0 (SOCOM 1's at 0x8: SOCOM II dropped the two team indices)
    unsigned char m_bNetwork;   // 0x1
};

extern CZNetwork theNetwork;

/* the radio's state (names ours): what the icons read of it */
class CRadioState {
public:
    unsigned char _0[0x24];
    int m_talker;   // 0x24: the slot talking
    unsigned char _28[0x2c - 0x28];
    int m_channel;   // 0x2c: 0 team, 1 defense, 2 offense, 3 dead, 10 SOCOM II's own
    unsigned char _30[0x5c - 0x30];
    int m_number;   // 0x5c: the channel's number shown (0-99)
    unsigned char _60[0x70 - 0x60];
    struct {
        int m_player;   // the player in the slot
        int _4;
    } m_slots[1];   // 0x70
};

/* the radio (no data row yet) and whether it is on (no name yet: it tests the state at 0x24
 * against 11) */
extern CRadioState *data_0045B718;
extern "C" bool func_0030E540(CRadioState *radio);
/* whether this player may talk, and whether anyone else is talking (the radio's; no names yet) */
extern "C" bool func_0030E710(CRadioState *radio);
extern "C" bool func_0030E790(CRadioState *radio);
/* the clock the listen icon's second runs on, and when it ends (bss; no rows yet) */
extern float data_004A44A0;
extern float data_004145F0;
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

    static PNT4D m_hud_textcolor;   /* the HUD's text colour */

    void Add2D(C2D *p2d, int phase, bool add_to_back);
};

extern CHUD *theHUD;

/* a player's clan tag, or "" (the player list's; no name yet) */
extern "C" const char *func_002C3AC0(int player);
/* a player's name, or 0 (the player list's; no name yet) */
extern "C" const char *func_002C3A70(int player);
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

/* the icon off, white and clear, fading in to half; its six textures (the spectators' falling back
 * on the team's), listed under its parent. The unit's last function: the radio icons' Ticks and
 * this Init follow CWhosTalking's here (SOCOM 1 kept the radio bitmaps in hud_actionbitmap.cpp);
 * the name is ours */
void CRadioIcon_Static::Init(C2D *parent)
{
    Off();
    SetColor(128.0f, 128.0f, 128.0f);
    SetTrans(0.0f);
    m_fade_limit = 64.0f;
    m_fade_dx = 2.0f;
    m_fade_enable = 1;
    m_pRadioOffTexture = zdb::CAssetMgr::m_assets.GetTexHandle("action_no_talk2.tif");
    m_pRadioChannelTeamTexture = zdb::CAssetMgr::m_assets.GetTexHandle("action_tune_team.tif");
    m_pRadioChannelOffenseTexture = zdb::CAssetMgr::m_assets.GetTexHandle("action_tune_offense.tif");
    m_pRadioChannelDefenseTexture = zdb::CAssetMgr::m_assets.GetTexHandle("action_tune_defense.tif");
    m_pRadioChannelDeadTexture = zdb::CAssetMgr::m_assets.GetTexHandle("action_dead_zone.tif");
    m_pRadioChannel10Texture = zdb::CAssetMgr::m_assets.GetTexHandle("action_tune_spectators.tif");
    if (!m_pRadioChannel10Texture)
        m_pRadioChannel10Texture = m_pRadioChannelTeamTexture;
    if (parent)
        parent->push_back(this);
}

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

/* where the dynamic icon sits (variables: read with lwc1; it shares the static icon's size) */
static float s_dynamicx = 545.0f;
static float s_dynamicy = 361.0f;

/* in a network game, the talk icon while this player may talk, or the listen icon for a second
 * after anyone else spoke; hidden otherwise. The texture's name kept in a string is never read
 * (left from debugging) */
void CRadioIcon_Dynamic::Tick(float dT)
{
    if (!theNetwork.m_bNetwork)
        return;
    zdb::CTexHandle *tex = 0;

    if (func_0030E790(data_0045B718))
        data_004145F0 = 1.0f + data_004A44A0;
    std::string name("none");

    if (func_0030E710(data_0045B718)) {
        tex = m_pRadioChannelCanTalkTexture;
        name = "talk texture";
    } else if (data_004145F0 >= data_004A44A0) {
        tex = m_pRadioChannelInUseTexture;
        name = "listen texture";
    }
    if (data_00437CE8 && tex) {
        TickFade();
        if (tex != m_pTexHandle) {
            Load(s_dynamicx, s_dynamicy, s_dynamicx + s_iconw, s_dynamicy + s_iconh, tex);
            On();
        } else if (!IsOn()) {
            On();
        }
    } else {
        Off();
    }
}

/* the radio part's places (the bss ones set by its Init; no rows yet) */
extern float data_004145F8;
extern float data_00414600;
extern float data_00414608;
extern float data_00414610;
extern float data_00414618;
static float s_backing_y1 = 359.0f;
static float s_text2_y = 371.0f;
static float s_text1_y = 352.0f;

inline void CRadioIcon_Dynamic::Init(C2D *parent)
{
    Off();
    SetColor(128.0f, 128.0f, 128.0f);
    SetTrans(0.0f);
    m_fade_limit = 64.0f;
    m_fade_dx = 2.0f;
    m_fade_enable = 1;
    m_pRadioChannelInUseTexture = zdb::CAssetMgr::m_assets.GetTexHandle("action_channel_busy.tif");
    m_pRadioChannelCanTalkTexture = zdb::CAssetMgr::m_assets.GetTexHandle("action_ok_talk2.tif");
    if (parent)
        parent->push_back(this);
}

/* the radio part made: the speaker's name, both icons, a dark backing and the channel's number,
 * each listed (the name is ours).
 * OPEN, 13 instructions, all two stack slots: the target homes the inlined dynamic Init's `this`
 * at sp+80, above the first push_back's iterator temporaries (sp+72/76), where MW puts it below
 * them (sp+72) -- every other slot (the three pushed values at 92/88/84, the later pushes'
 * iterators, the inlined push's at 48/52) agrees. Tried: the Init as a free inline taking the
 * icon, not declared inline (auto-inlined), defined first in the unit, an early return, a C2D *
 * cast of `this`, the pushes through a helper, the first value a named local -- no change or worse */
void CWhosTalking::Init(C2DFont *font)
{
    m_text1.SetFont(font);
    m_text1.m_scale = 0.9f;
    m_text1.m_socom2_6a = 1;
    m_text1.m_color[0] = CHUD::m_hud_textcolor.x;
    m_text1.m_color[1] = CHUD::m_hud_textcolor.y;
    m_text1.m_color[2] = CHUD::m_hud_textcolor.z;
    m_text1.m_color[3] = CHUD::m_hud_textcolor.w;
    m_text1.SetPos((int)data_00414618, (int)s_text1_y);
    push_back(&m_text1);
    m_static.Init(this);
    m_dynamic.Init(this);
    m_bitmap2.SetHasTexture(false);
    m_bitmap2.SetColor(0.0f, 0.0f, 0.0f);
    m_bitmap2.SetTrans(50.0f);
    m_bitmap2.SetPos((int)data_004145F8, (int)s_backing_y1, (int)data_00414600, (int)data_00414608);
    push_back(&m_bitmap2);
    m_text2.SetFont(font);
    m_text2.m_scale = 0.8f;
    m_text2.m_color[0] = CHUD::m_hud_textcolor.x;
    m_text2.m_color[1] = CHUD::m_hud_textcolor.y;
    m_text2.m_color[2] = CHUD::m_hud_textcolor.z;
    m_text2.m_color[3] = CHUD::m_hud_textcolor.w;
    m_text2.SetTrans(128.0f);
    m_text2.SetPos((int)data_00414610, (int)s_text2_y);
    m_text2.m_isCentered = 1;
    push_back(&m_text2);
}

/* the radio part placed again: the speaker's name and the channel's in the HUD's text colour, both
 * icons off and fading in, a dark backing */
void CWhosTalking::ReInit()
{
    m_text1.m_scale = 0.9f;
    m_text1.m_socom2_6a = 1;
    m_text1.m_color[0] = CHUD::m_hud_textcolor.x;
    m_text1.m_color[1] = CHUD::m_hud_textcolor.y;
    m_text1.m_color[2] = CHUD::m_hud_textcolor.z;
    m_text1.m_color[3] = CHUD::m_hud_textcolor.w;
    m_text1.SetPos((int)data_00414618, (int)s_text1_y);
    m_static.Off();
    m_static.SetColor(128.0f, 128.0f, 128.0f);
    m_static.SetTrans(0.0f);
    m_static.m_fade_limit = 64.0f;
    m_static.m_fade_dx = 2.0f;
    m_static.m_fade_enable = 1;
    m_dynamic.Off();
    m_dynamic.SetColor(128.0f, 128.0f, 128.0f);
    m_dynamic.SetTrans(0.0f);
    m_dynamic.m_fade_limit = 64.0f;
    m_dynamic.m_fade_dx = 2.0f;
    m_dynamic.m_fade_enable = 1;
    m_bitmap2.SetHasTexture(false);
    m_bitmap2.SetColor(0.0f, 0.0f, 0.0f);
    m_bitmap2.SetTrans(50.0f);
    m_bitmap2.SetPos((int)data_004145F8, (int)s_backing_y1, (int)data_00414600, (int)data_00414608);
    m_text2.m_scale = 0.8f;
    m_text2.m_color[0] = CHUD::m_hud_textcolor.x;
    m_text2.m_color[1] = CHUD::m_hud_textcolor.y;
    m_text2.m_color[2] = CHUD::m_hud_textcolor.z;
    m_text2.m_color[3] = CHUD::m_hud_textcolor.w;
    m_text2.SetTrans(128.0f);
    m_text2.SetPos((int)data_00414610, (int)s_text2_y);
    m_text2.m_isCentered = 1;
}

/* in a network game: who is talking (the clan's tag ahead of the name), the channel's number, and
 * the radio icons */
void CWhosTalking::Tick(float dT)
{
    if (!theNetwork.m_bNetwork)
        return;
    char number[8];
    char text[64];
    const char *speaker = 0;

    if (func_0030E790(data_0045B718)) {
        int player = data_0045B718->m_slots[data_0045B718->m_talker].m_player;
        const char *clan = func_002C3AC0(player);
        const char *name = func_002C3A70(player);

        if (!name)
            return;
        if (clan && strlen(clan))
            sprintf(text, "[%s] %s", clan, name);
        else
            strcpy(text, name);
        speaker = text;
    }
    m_text1.SetString(speaker ? speaker : " ", true);
    if (speaker) {
        m_text1.SetPos((int)data_00414618, (int)s_text1_y);
        func_00362B70(&m_text1);
        m_text1.On();
    } else {
        m_text1.Off();
    }
    int channel = data_0045B718->m_number;

    if (channel > -1 && channel < 100) {
        sprintf(number, "%d", channel);
        m_text2.SetString(number, true);
        func_00362B70(&m_text2);
    } else {
        m_text2.SetString("?", true);
        func_00362B70(&m_text2);
    }
    m_text2.On();
    m_bitmap2.On();
    m_static.Tick(dT);
    m_dynamic.Tick(dT);
}

/* the box's corners and the health bar's place (the bss ones set by Init; no rows yet) */
extern float data_00414640;
extern float data_00414650;
extern float data_00414658;
static float s_box_y1 = 418.0f;
static float s_bar_h = 18.0f;
static float s_bar_x = 488.0f;
static float s_bar_y = 396.0f;
static float s_name_scale = 0.8f;
static float s_name_x = 555.0f;
static float s_name_y = 409.0f;


/* the box made: the first bitmap off, the second the backing's texture, the health bar, the name
 * and who is talking, each added to the HUD's phases (the name is ours) */
void CMPInfoBox::Init(C2DFont *font)
{
    m_bitmap1.Off();
    m_bitmap2.m_pTexHandle = zdb::CAssetMgr::m_assets.GetTexHandle("newweapnbkrnd.tif");
    m_bitmap2.SetUV(1.0f, 1.0f, 0.0f, 0.0f);
    m_bitmap2.SetColor(128.0f, 128.0f, 128.0f);
    m_bitmap2.SetTrans(68.0f);
    m_bitmap2.SetPos((int)data_00414640, (int)s_box_y1, (int)data_00414650, (int)data_00414658);
    theHUD->Add2D(&m_bitmap2, -1, true);
    m_healthbar.SetSize(s_namewidth, s_bar_h);
    m_healthbar.SetPos(s_bar_x, s_bar_y);
    theHUD->Add2D(&m_healthbar, -1, true);
    m_text.SetFont(font);
    m_text.m_scale = s_name_scale;
    m_text.m_isCentered = 1;
    m_text.m_color[0] = CHUD::m_hud_textcolor.x;
    m_text.m_color[1] = CHUD::m_hud_textcolor.y;
    m_text.m_color[2] = CHUD::m_hud_textcolor.z;
    m_text.m_color[3] = CHUD::m_hud_textcolor.w;
    m_text.SetString("  ", true);
    m_text.SetPos((int)s_name_x, (int)s_name_y);
    func_00362B70(&m_text);
    theHUD->Add2D(&m_text, -1, true);
    m_whostalking.Init(font);
    theHUD->Add2D(&m_whostalking, 2, true);
}

/* the parts placed again for a new game: the first bitmap off, the second a faded backing, the
 * health bar, the name in the HUD's text colour, who is talking */
void CMPInfoBox::ReInit()
{
    m_bitmap1.Off();
    m_bitmap2.SetUV(1.0f, 1.0f, 0.0f, 0.0f);
    m_bitmap2.SetColor(128.0f, 128.0f, 128.0f);
    m_bitmap2.SetTrans(68.0f);
    m_bitmap2.SetPos((int)data_00414640, (int)s_box_y1, (int)data_00414650, (int)data_00414658);
    m_healthbar.SetSize(s_namewidth, s_bar_h);
    m_healthbar.SetPos(s_bar_x, s_bar_y);
    m_text.m_scale = s_name_scale;
    m_text.m_isCentered = 1;
    m_text.m_color[0] = CHUD::m_hud_textcolor.x;
    m_text.m_color[1] = CHUD::m_hud_textcolor.y;
    m_text.m_color[2] = CHUD::m_hud_textcolor.z;
    m_text.m_color[3] = CHUD::m_hud_textcolor.w;
    m_text.SetString("  ", true);
    m_text.SetPos((int)s_name_x, (int)s_name_y);
    func_00362B70(&m_text);
    m_whostalking.ReInit();
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

    m_healthbar.Update(owner, true);
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
