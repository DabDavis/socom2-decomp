/* hud_main.cpp -- the HUD (CHUD): SOCOM 1's unit (its DWARF names it, Z:\dev\Apps\FTS\hud_main.cpp),
 * its functions in SOCOM 1's order (OVL1 0x1F6A90-0x1FE33F, then the inline functions'
 * out-of-line copies). SOCOM II's CHUD is 0x1ACD0 bytes (SOCOM 1's 0x14960), laid out from its
 * constructor, destructor and ReInit_Uninit: a string in front of the texture lists, the map
 * new'ed (single player only), a network weapon selector, the multiplayer boxes (teammate names,
 * the info box with the health bar and who is talking, the spectator's), six strings where SOCOM 1
 * had three piggyback texts, a lens flare manager where it had CLensFlareSaturate, and the ammo
 * counts after the phases.
 * Written, all byte-exact: the timer and zoom texts, Add2D, the two map switches, ZoomIn, ZoomOut,
 * IsOn, Off, On, PostDITick, PreDITick, Flash, the four network texts Tick calls (spectator,
 * spectating, respawn, ghost), PauseGame, ReInit_Init, ReInit_Uninit, GetTextureByName,
 * DrawPhase, ReleaseHUD, HudText_to_upper, GetHUDOwner, the constructor and the unit's last four
 * (round 33: hudToggleHelpWindows, hudHelpWindowsEnabled, hudInit -- SOCOM 1's, the unit's last
 * there too -- and FormatStringInt, 0x1FDE30-0x1FE18F); and, with the
 * members laid out, the implicit destructors the unit links (the selectors', the team names',
 * the stance icon's, the network selector's slots and category page). Written and OPEN: the
 * destructor (0xA40; two headers' inline heights, diagnosed there). Not yet: Tick (0x1158),
 * Init (0xF98; the font's constructor inlines one level, its pieces 0x1FB370-0x1FB3F0 out of
 * line), and the four inline copies at the unit's head
 * (0x1F6A90-0x1F6B00: C2DString helpers the reticle's unit calls; their caller here must be
 * dead-stripped). 0x1F6A80 is CLoadMenu's (st_load.cpp's vtable slot).
 * Defined in reverse of their image order (-inline deferred). */

#include <string.h>
#include <list>
#include <vector>

#include "../../gamez/ztwod/ztwod.h"
#include "../../gamez/znode/znode.h"
#include "../../gamez/zrdr/zrdr.h"
#include "hud_ammo.h"
#include "hud_fader.h"
#include "hud_lensflare.h"
#include "hud_letterbox.h"
#include "hud_mapcompass.h"
#include "hud_missioncams.h"
#include "hud_mpinfobox.h"
#include "hud_newhudmap.h"
#include "hud_tcm.h"
#include "hud_teammatenames.h"
#include "hud_weaponsel.h"

namespace zdb {
class CWorld;
class CTexPal;

class CAssetList {
public:
    CTexHandle *GetTexHandle(const char *name);
};

class CAssetMgr {
public:
    static CAssetList m_assets;
};
}

class CUIVariable;

/* the seal's controller as the HUD reads it: the menu it is in (SOCOM 1's
 * CSealCtrl::GetMenuState, a call there, inline in SOCOM II: its byte read in place) */
class CSealCtrl {
public:
    MENU_STATE GetMenuState() { return m_menustate; }

    unsigned char _0[0x221];
    MENU_STATE m_menustate;   // 0x221
};

class CZFTSWeapon;

/* the seal's kit (SOCOM 1's CZKit, at 0x520 there): its flag word, one bit past SOCOM 1's last
 * (m_heartbeat_enabled was bit 14), and the kit's own functions the HUD calls (OVL2) */
class CZKit {
public:
    void SetCurReticule(bool flag);   /* SOCOM 1's, a flag added */
    CZFTSWeapon *GetCurItem() const;
    bool IsUsingScope();
    bool NVG_Enabled() const;
    float GetCurZoom();

    unsigned int _b0 : 16;
    unsigned int m_socom2_b16 : 1;   // bit 16: cleared when the HUD goes off
    unsigned int _b17 : 15;
};

class CRespawnNode;
class CEntity;

/* the entities (SOCOM 1's CEntityList, CEntity::m_list): the find by id (0x29E4E0) */
class CEntityList {
public:
    CEntity *GetEntityById(unsigned int id) const;

    unsigned char _0[0xc];
};

/* an entity as the spectator's line reads it: its type (2 a seal) and its name (SOCOM II's
 * layout), and its node (SOCOM 1's CEntity::GetNode, 0x29ED20) */
class CEntity {
public:
    CRespawnNode *GetNode();

    static CEntityList m_list;

    unsigned char _0[0x10];
    unsigned char m_type;   // 0x10
    unsigned char _11[0x14 - 0x11];
    const char *m_name;   // 0x14
};

/* the seal as the HUD reads it: its controller (CEntity's, at 0xC0) and its kit */
class CZSealBody : public CEntity {
public:
    unsigned char _18[0xc0 - 0x18];
    CSealCtrl *m_ctrl;   // 0xc0
    unsigned char _c4[0xe1 - 0xc4];
    unsigned char _e1_0 : 4;
    unsigned char m_socom2_e1_4 : 1;   // 0xe1 bit 4 (MapCompass::On tests it too)
    unsigned char _e1_5 : 3;
    unsigned char _e2[0x200 - 0xe2];
    signed char m_socom2_200;   // 0x200: the seal's state (2-3 none, 4 the binoculars' 9x, 5-12 the kit's zoom)
    unsigned char _201[0x5e0 - 0x201];
    CZKit m_kit;   // 0x5e0
    unsigned char _5e4[0xfb4 - 0x5e4];
    float m_socom2_fb4;   // 0xfb4: when the seal died, against the mission's clock at 0xE0
    unsigned char _fb8[0xfc4 - 0xfb8];
    int m_socom2_fc4;   // 0xfc4: the id of the entity a spectator watches
};

/* a weapon of the kit's: its type byte (11 for the one ZoomIn turns the ammo counts off for) */
class CZFTSWeapon {
public:
    unsigned char _0[0x7c];
    unsigned char m_type;   // 0x7c
};

/* the HUD's elements fade in from this time (SOCOM 1's g_fElemsFadeInTime, hud_main.cpp's own;
 * other units read it in SOCOM II) */
extern float g_fElemsFadeInTime;

/* MSL's toupper (MAIN) */
extern "C" int toupper(int c);

/* the app camera: only the SEAL it follows (SOCOM 1's m_entity, 0x80 further on in SOCOM II) */
class CAppCamera {
public:
    unsigned char _0[0xbc];
    CZSealBody *m_bc;   // 0xbc
};

extern CAppCamera *appCamera;

class CZNetwork {
public:
    unsigned char m_bSessionMaster;   // 0x0 (SOCOM 1's at 0x8: SOCOM II dropped the two team indices)
    unsigned char m_bNetwork;   // 0x1
};

extern CZNetwork theNetwork;

/* the dynamic texture lists are znode.h's zdb::CDynTexList, SOCOM 1's classes over the pointer
 * vectors (zdb::CTexList, CTexPalList): at -inline level=5 CHUD's constructor inlines its
 * constructor whole (six stores) while ~CHUD inlines its destructor one level and calls the two
 * class destructors (0x1FC520, 0x1FC5A0, four this tests each) -- the typedefs level 4 needed
 * here are gone (round 12) */

/* MSL's basic_string, only as CSaferStr's base: its destructor is never inlined (called with 0
 * for a base, as the states' model has it) */
namespace std {
template <class charT> struct char_traits {};

template <class charT, class traits = char_traits<charT>, class Allocator = allocator<charT> >
class basic_string {
public:
    ~basic_string();

    unsigned int w[3];
};

typedef basic_string<char> string;
}

/* a string that is never null (SOCOM 1's: MSL's basic_string and nothing more; its constructor
 * out of line, 0x1E7B60) */
class CSaferStr : public std::string {
public:
    CSaferStr();
};

/* the HUD's font as its delete sees it: c2dfont.h's C2DFont (a glyph map, a pointer vector, the
 * name). Its destructor's body is one call (0x35A650, the font's unload; no row named yet), then
 * the members in reverse -- ReleaseHUD's delete inlines it all, the glyph tree's recursive
 * destroy (0x1FE760) called out of line */
#include "../../gamez/ztwod/c2dfont.h"

class UIVarMixin;

/* a UI variable's hook (SOCOM 1's DWARF; the vtable pointer first). The destructor detaches
 * whatever is attached, as SOCOM 1's (DetachUIVar, 0x385D90) */
class UIVarMixin {
public:
    UIVarMixin() { m_uivar = 0; }
    virtual ~UIVarMixin() { DetachUIVar(0); }
    void DetachUIVar(CUIVariable *var);

    CUIVariable *m_uivar;   // 0x4
};   // 8 bytes

/* a plain bitmap of the UI's (SOCOM 1's DWARF) */
class CPlainBmp : public C2DBitmap, public UIVarMixin {
public:
    CPlainBmp()
    {
        SetColor(128.0f, 128.0f, 128.0f);
        SetTrans(128.0f);
        SetIsTrans(true);
        SetCanHandleInput(false);
        m_pulsing = false;
        m_period = 1.0f;
    }
    virtual void Draw(zdb::CCamera *cam);
    virtual void Draw(const CMatrix &m, zdb::CTextureRelocMgr *mgr);
    virtual void Tick(float dT);
    virtual void SetMapOffset(int x, int y);

    bool m_pulsing;   // 0x88
    float m_period;   // 0x8c
    float m_curtrans;   // 0x90
};   // 0xa0 bytes

/* the action icon (SOCOM 1's DWARF; its constructor inline there too) */
class CHUD;

class CZActionBitmap : public C2DBitmap {
public:
    CZActionBitmap()
    {
        m_canCarryBodyTexture = 0;
        m_canCQBTexture = 0;
        m_canRestrainTexture = 0;
        m_canPickupTexture = 0;
        m_canKnifeTexture = 0;
        m_pttTexture = 0;
        m_canClimbTexture = 0;
        m_parent = 0;
    }
    void UnInit();

    zdb::CTexHandle *m_canClimbTexture;   // 0x80
    zdb::CTexHandle *m_pttTexture;   // 0x84
    zdb::CTexHandle *m_canDropMPBomb;   // 0x88
    zdb::CTexHandle *m_canKnifeTexture;   // 0x8c
    zdb::CTexHandle *m_canPickupTexture;   // 0x90
    zdb::CTexHandle *m_canRestrainTexture;   // 0x94
    zdb::CTexHandle *m_canCQBTexture;   // 0x98
    zdb::CTexHandle *m_canCarryBodyTexture;   // 0x9c
    zdb::CTexHandle *m_actionXTexture;   // 0xa0
    CHUD *m_parent;   // 0xa4
};   // 0xb0 bytes



/* the stance icon (hud_posebitmap.cpp's layout) */
class PoseBitmap : public C2DBitmap {
public:
    PoseBitmap();
    void Clear();
    void ReInit();

    int m_curpose;   // 0x80
    zdb::CTexture *m_posetex[3];   // 0x84
    float m_posx;   // 0x90
    float m_posy;   // 0x94
    unsigned char m_error;   // 0x98
    C2DString m_text;   // 0x9c
    float m_textposx;   // 0x134
    float m_textposy;   // 0x138
    C2DFont *m_myfont;   // 0x13c
    float m_trans;   // 0x140
};   // 0x150 bytes

/* the reticle's bitmaps and strings (0x47B0 bytes); 0x215180 turns on its bitmap at 0x34 (the
 * paused HUD's Off keeps it). Its destructor is its unit's own (0x219090): ~CHUD calls it */
class BitmapReticule : public C2D {
public:
    BitmapReticule();
    virtual ~BitmapReticule();
    void UnInit();
    void OnPauseTest();
    void HideTimer();
    unsigned char _18[0x47b0 - 0x18];
} __attribute__((aligned(16)));

class CTeamNames : public C2D {
public:
    CTeamNames();
    void Init();
    void Clear();

    unsigned char _18[0x28 - 0x18];
    CHudBrightText m_names[4];   // 0x28
    C2DString m_texts[4];   // 0x288
    C2DBitmap m_bitmap1;   // 0x4f0
    C2DBitmap m_bitmap2;   // 0x570
    CHealthBar m_healthbars[4];   // 0x5f0
    C2DBitmap m_icons[4];   // 0xd30
} __attribute__((aligned(16)));   // 0xf30 bytes

class CSpectatorInfo : public C2D {
public:
    CSpectatorInfo();
    virtual ~CSpectatorInfo();
    unsigned char _18[0x680 - 0x18];
} __attribute__((aligned(16)));

class CZPauseTest {
public:
    CZPauseTest();
    ~CZPauseTest();
    void ReInit();
    void Clear();
    void ClosePauseMenu();
    unsigned char _0[0x13c0];
} __attribute__((aligned(16)));

enum HUD_MODE {
    ZOOMIN = 0,
    ZOOMOUT = 1,
};

/* the app camera (the shake's stop only) and the seal the player is (SOCOM 1's ftsGetPlayer) */
extern "C" void StopShake__10CAppCameraFv(CAppCamera *camera);
CZSealBody *ftsGetPlayer();

/* a valve's change of state (zanim.h's) */
enum VALVE_STATE {
    VALVE_SAME,
    VALVE_ON,
    VALVE_OFF
};

/* a server setting (SOCOM 1's CValve): its value a halfword at 4 */
class CValve {
public:
    static CValve *GetByName(const char *name);
    void MakeCallbacks(VALVE_STATE state);
    /* zanim.h's inline setter: the callbacks told when it turns on or off */
    bool SetValue(int value)
    {
        VALVE_STATE state = VALVE_SAME;

        if (m_value != value)
            state = value == 0 ? VALVE_OFF : VALVE_ON;
        m_value = value;
        if (state != VALVE_SAME) {
            MakeCallbacks(state);
            return true;
        }
        return false;
    }

    unsigned char _0[4];
    unsigned short m_value;   // 0x4
};

/* a queue of messages on screen (SOCOM 1's C2DMessage_Q, a C2D: the HUD turns them off and on
 * through C2D's slots): only its font placed */
class C2DMessage_Q : public C2D {
public:
    void SetBkgColor(float r, float g, float b);
    void Clear_Messages();

    C2DFont *m_font;   // 0x18
    unsigned char _1c[0x4e0 - 0x1c];
};   // 0x4e0 bytes (the queues' stride)

/* the message editor, the chat line (fts_msgeditor.cpp's CMsgEditor, the states lane's: a C2D,
 * 0x150 bytes -- its Clear 0x2CA0B0, its UnInit 0x2CA050) */
class CMsgEditor : public C2D {
public:
    void UnInit();
    void Clear();

    unsigned char _18[0x150 - 0x18];
} __attribute__((aligned(16)));

/* the mission: its three message queues, the chat line (the HUD re-inits it) and the online game */
class CMission {
public:
    /* the round time, if any (0x2A6AD0: positive while the network game has a timer) */
    int GetTimeLeft();

    unsigned char _0[0xe0];
    float m_socom2_e0;   // 0xe0: the mission's clock (the respawn wait is measured on it)
    unsigned char _e4[0x1c0 - 0xe4];
    C2DMessage_Q m_messages;   // 0x1c0
    C2DMessage_Q m_messages2;   // 0x6a0
    C2DMessage_Q m_messages3;   // 0xb80
    CMsgEditor m_messages4;   // 0x1060: the chat line
    unsigned char _11b0[0x1808 - 0x11b0];
    void *m_1808;   // 0x1808
};

extern CMission theMission;
/* SOCOM II's spectator test, or so (0x2C2FA0: CAmmoDisplay::On asks it too; no row yet) */
extern "C" bool func_002C2FA0();

/* the localised string tables (util_loc.cpp's; only what is called): a table loaded and unloaded by
 * name, the language it is loaded in, and the tables' global (GetStringTables, 0x361BC0, returns it) */
enum LANGUAGE {
    LANG_STATES,
    LANG_UK,
    LANG_FRANCE,
    LANG_GERMANY,
    LANG_SPAIN,
    LANG_ITALY,
    LANG_JAPAN,
    LANG_KOREA,
    LANG_TEST,
    LANG_COUNT,
    LANG_UNSET = 99
};

LANGUAGE GetLanguage();

class CStringTables {
public:
    void Load(const char *name, LANGUAGE lang);
    void Unload(const char *name);
};
CStringTables *GetStringTables();

class CHUD {
public:
    CHUD();
    ~CHUD();
    void SetupTimerText();
    void SetupSpectatorText(MENU_STATE menustate);
    void SetupGhostText(MENU_STATE menustate);
    void SetupSpectatingText(MENU_STATE menustate);
    void SetupRespawnText(MENU_STATE menustate);
    void SetupZoomText();
    void Add2D(C2D *p2d, int phase, bool add_to_back);
    void SwitchToCompassMap();
    void SwitchToFullScreenMap();
    void ZoomIn();
    void ZoomOut();
    bool IsOn();
    void Off();
    void On();
    void PostDITick(float dT);
    void PreDITick(float dT);
    void Flash(float brightness, const CPnt3D &color, float fadeout_time, float fade_time,
               float fadein_time);
    bool PauseGame();
    void ReInit_Init();
    void ReInit_Uninit();
    void DrawPhase(zdb::_RenderPhase phase, zdb::CCamera *cam);
    static void HudText_to_upper(char *instr);
    static CZSealBody *GetHUDOwner();
    static bool ReleaseHUD(CHUD **ppHud);
    /* the range finder's text, m_socom2_0 (SOCOM II's: BitmapReticule's code prints "%dm" or
     * "RANGE(m): ----" into it and places it; SOCOM 1's DoRangeText was CHUD's own) -- the names
     * ours */
    void SetRangeText(const char *text);
    void SetRangeTextPos(float x, float y);
    void ShowRangeText();
    void HideRangeText();

    static CPnt4D m_hud_textcolor;   /* the HUD's text colour (OVL1 bss 0x408E40) */

    C2DString m_socom2_0;   // 0x0
    zdb::CDynTexList m_dyntexlist;   // 0x98 (znode.h's: the classes over the pointer vectors)
    signed char m_hudrettype;   // 0xb0
    bool m_usehudret;   // 0xb1
    bool m_hudretchanged;   // 0xb2
    zdb::CTexture *reticuletexture;   // 0xb4
    zdb::CTexHandle *m_firemodetex;   // 0xb8
    float m_weapontopx;   // 0xbc
    float m_weapontopy;   // 0xc0
    float m_weaponbotx;   // 0xc4
    float m_weaponboty;   // 0xc8
    C2DBitmap weapon;   // 0xd0
    C2DString m_firemodename;   // 0x150
    C2DBitmapPoly m_firemode;   // 0x1f0 (SOCOM 1: a C2DBitmap)
    CPlainBmp m_WeaponBackground;   // 0x3e0
    float m_compasstopx;   // 0x480
    float m_compasstopy;   // 0x484
    float m_compassbotx;   // 0x488
    float m_compassboty;   // 0x48c
    float m_compasscenterx;   // 0x490
    float m_compasscentery;   // 0x494
    C2DFont *m_myfont;   // 0x498
    int m_curweapon;   // 0x49c
    int m_curseal;   // 0x4a0
    float pointpos[4][2];   // 0x4a4
    float pointvel[4][2];   // 0x4c4
    CHudBrightText m_ammocount;   // 0x4e4
    CHudBrightText m_magscount;   // 0x57c
    C2DString m_ammotype;   // 0x614
    CHudBrightText m_weaponname;   // 0x6ac
    C2DString m_zoomleveltext;   // 0x744
    float m_messageTime;   // 0x7dc
    C2DString m_message;   // 0x7e0
    C2DString m_perfstats[30];   // 0x878
    CWeaponSel m_weaponsel;   // 0x1a50
    CInGameWeaponSel m_ingameweaponsel;   // 0x2730
    CMPWeaponSel m_mpweaponsel;   // 0x38e0
    C2DString m_timertext;   // 0x4e20
    C2DBitmap m_socom2_4ec0;   // 0x4ec0
    C2DString m_rangetext;   // 0x4f40
    MapCompass m_mapcompass;   // 0x4fe0
    CZNewHudMap *m_newmap;   // 0x11c90 (SOCOM 1: the map itself)
    PoseBitmap m_poseBitmap;   // 0x11ca0
    BitmapReticule m_bitmapReticule;   // 0x11df0
    CTeamNames m_teamnames;   // 0x165a0
    CZActionBitmap m_actionbitmap;   // 0x174d0
    CMPTeammateNames m_teamMateNames;   // 0x17580
    CMPInfoBox m_infobox;   // 0x17b70
    CSpectatorInfo m_spectatorinfo;   // 0x18280
    CZPauseTest m_pausetest;   // 0x18900
    C2DString m_num_connected;   // 0x19cc0
    C2DString m_piggyback_text[6];   // 0x19d58
    float m_socom2_1a0e8;   // 0x1a0e8: -1 after ReInit_Uninit
    int m_socom2_1a0ec;   // 0x1a0ec
    int m_socom2_1a0f0;   // 0x1a0f0: -1 after ReInit_Uninit
    CLetterBox m_letterbox;   // 0x1a100
    CFader m_bmpFade;   // 0x1a230
    CFlashFXVector m_FlashFXVector;   // 0x1a2d0
    CZHudMissionCams m_missioncams;   // 0x1a2dc
    CLensFlareManager m_lensflares;   // 0x1a698
    C2D m_phase[3];   // 0x1a6ac
    HUD_MODE m_hudmode;   // 0x1a6f4
    /* SOCOM 1's m_ison byte, now two bits: IsOn reads bit 1 (Off clears it, On and Init set
     * it), DrawPhase draws only while bit 0 is set (Init sets it; the name is ours) */
    unsigned char m_draw : 1;   // 0x1a6f5
    unsigned char m_ison : 1;
    CAmmoDisplay m_ammodisplay;   // 0x1a700
};   // 0x1acd0 bytes

extern CHUD *theHUD;

/* c2dstring.cpp's: a string table entry with its `$<n>s` arguments filled in (the name ours) */
void FormatString(char *out, int size, int id, const char **args, int nargs);

/* a number as a string table entry's one argument (SOCOM II's, the unit's last function: the weapon
 * selector's magazine count; the name ours) */
void FormatStringInt(char *out, int size, int id, int value)
{
    char buf[16];
    const char *arg;

    sprintf(buf, "%d", value);
    arg = buf;
    FormatString(out, size, id, &arg, 1);
}

/* the help windows' valve (SOCOM II's; OVL1 bss 0x408E30, the name ours): hudInit finds it */
static CValve *s_helpWindowsValve;

/* hud_linemap.cpp's type readers, and the score popup unit's event popups (SOCOM II's) */
void hudAddPolyType(_zrdr *pRdr);
void hudAddLineType(_zrdr *pRdr);
void hudReadPopupCommon(_zrdr *rdr);
extern "C" void func_0022D660(_zrdr *popup);

/* hud.rdr read (SOCOM 1's hudInit, CMission::Init's call): the tactical map's polygon and line
 * types, then SOCOM II's event popups -- the common look, each definition -- and the help windows'
 * valve */
void hudInit()
{
    _zrdr *rdr = zrdr_read("hud.rdr", "data/common", 0);
    _zrdr *tacmap = zrdr_findtag(rdr, "TacMap");
    _zrdr *polys = zrdr_findtag(tacmap, "Polygons");

    if (polys)
        for (int i = 0; i < polys->Count(); i++)
            hudAddPolyType(polys->Item(i));

    _zrdr *lines = zrdr_findtag(tacmap, "Lines");

    if (lines)
        for (int i = 0; i < lines->Count(); i++)
            hudAddLineType(lines->Item(i));
    hudReadPopupCommon(zrdr_findtag(zrdr_findtag(rdr, "EventPopups"), "Common"));

    _zrdr *popups = zrdr_findtag(rdr, "PopupDefinitions");

    if (popups)
        for (int i = 0; i < popups->Count(); i++)
            func_0022D660(popups->Item(i));
    zrdr_free((CRdrFile *)rdr);
    s_helpWindowsValve = CValve::GetByName("HelpWindowsEnabled");
}

/* whether the help windows are on: so unless the valve is there and off (SOCOM II's; the pause
 * menu and the score popup ask; the name ours) */
bool hudHelpWindowsEnabled()
{
    CValve *v = s_helpWindowsValve;

    return !v || v->m_value != 0;
}

/* the help windows turned the other way (SOCOM II's; the pause menu's; the name ours) */
void hudToggleHelpWindows()
{
    if (s_helpWindowsValve)
        s_helpWindowsValve->SetValue(!hudHelpWindowsEnabled());
}

/* the unit's dynamically initialised statics, in SOCOM 1's order (__sinit_hud_main.cpp 0x3FDA40
 * stores the colour's four words, then builds the bitmap and registers ~C2DBitmap 0x1FCB10; SOCOM
 * 1's built the colour through a temporary, 86/63/27/128 there) */
CPnt4D CHUD::m_hud_textcolor(120.0f, 60.0f, 95.0f, 80.0f);
/* SOCOM 1's file static X_WeaponBackground (0x80 bytes, OVL1 bss 0x408E70): Init sets it up */
static C2DBitmap X_WeaponBackground;

/* the HUD's members built in order; then it is the HUD, the map is new'ed for a single-player
 * game (none in a network game), and the HUD's string table registered */
CHUD::CHUD()
    : m_hudrettype(-1), m_usehudret(false), m_hudretchanged(false), m_firemodetex(0),
      m_messageTime(0.0f)
{
    theHUD = this;
    m_newmap = !theNetwork.m_bNetwork ? new CZNewHudMap : 0;
    CStringTables *tables = GetStringTables();

    tables->Load("HudC", GetLanguage());
}

CZSealBody *CHUD::GetHUDOwner()
{
    return appCamera->m_bc;
}

void CHUD::HudText_to_upper(char *instr)
{
    int len = strlen(instr);

    for (int kk = 0; kk < len; kk++)
        instr[kk] = toupper(instr[kk]);
}

/* the map items' textures (SOCOM 1's statics; ReleaseHUD lets them go) */
class CZPlayerMapItem {
public:
    static void MissionEndCleanup();

    static zdb::CTexture *m_conetex;
    static zdb::CTexture *m_alivebliptex;
    static zdb::CTexture *m_deadbliptex;
};

class CZClaymoreMapItem {
public:
    static zdb::CTexture *m_claymoretex;
};

class CZMPBombMapItem {
public:
    static zdb::CTexture *m_mpbombtex;
};

/* the HUD let go of: the message queues emptied, the flares deleted, the font deleted, the map
 * uninitialised, the map items' textures forgotten, and the HUD deleted */
bool CHUD::ReleaseHUD(CHUD **ppHud)
{
    if (!ppHud)
        return false;
    CHUD *hud = *ppHud;

    if (!hud)
        return false;
    theMission.m_messages.Clear_Messages();
    theMission.m_messages3.Clear_Messages();
    theMission.m_messages2.Clear_Messages();
    hud->m_lensflares.Clear();
    hud->reticuletexture = 0;
    hud->m_curweapon = 0;
    hud->m_curseal = 0;
    if (hud->m_myfont) {
        delete hud->m_myfont;
        hud->m_myfont = 0;
    }
    if (hud->m_newmap)
        hud->m_newmap->UninitMap();
    CZPlayerMapItem::m_conetex = 0;
    CZPlayerMapItem::m_alivebliptex = 0;
    CZPlayerMapItem::m_deadbliptex = 0;
    CZClaymoreMapItem::m_claymoretex = 0;
    CZMPBombMapItem::m_mpbombtex = 0;
    delete *ppHud;
    *ppHud = 0;
    return true;
}

/* what ~CHUD calls, not named yet: the network HUD's clean-ups (0x2370D0 on the state at
 * 0x413120, 0x22F3B0 a flag, 0x23B200 on the bss object at 0x414680, in single player), the
 * string table's unregister by name (0x3A3740), the fourth queue's (0x2CA050) and two of ztwod's
 * static setters (0x363E60, 0x363E50: floats at 0x49E9A0 and 0x49E9A8) */
class CameraZoom {
public:
    static void Reset();
};

void hudTurnOffScorePopup();
/* the voice recogniser (reco.cpp's; only what is called) */
class CReco {
public:
    void UnInit();
};
extern CReco theReco;
extern unsigned char data_003DC3E0;
void tcmResetVoteAll();   // hud_tcmcmds.cpp's: VOTE ALL's choice cleared
extern "C" void func_00363E60(float f);
extern "C" void func_00363E50(float f);

/* the HUD taken down: the action icon's textures let go, the network HUD's states cleared, the
 * map deleted, the HUD's string table unregistered, the zoom reset, the fourth queue's, ztwod's
 * two statics zeroed, the flag at 0x3DC3E0 set and the score popup off; then every member's
 * destructor, inlined -- whole, but the weapon selectors' (CWeaponSel, CMPWeaponSel), the info
 * box's and the texture lists', each one level (their trees too tall), calling their members'.
 * Byte-exact at -inline level=5 (round 12): the lens flare manager's destructor inlines whole
 * (four this tests, the vtable store, the range erase), and with znode.h's zdb::CDynTexList the
 * texture lists' destructor goes one level and calls zdb::CTexList's and CTexPalList's
 * (0x1FC520, 0x1FC5A0) while CHUD's constructor still inlines CDynTexList's constructor whole --
 * the typedefs level 4 forced had inlined both vectors whole here, 18 instructions long (0xA88) */
CHUD::~CHUD()
{
    m_actionbitmap.UnInit();
    theTCM.Reset();
    tcmResetVoteAll();
    if (!theNetwork.m_bNetwork)
        theReco.UnInit();
    if (m_newmap) {
        delete m_newmap;
        m_newmap = 0;
    }
    theHUD = 0;
    GetStringTables()->Unload("HudCommon");
    CameraZoom::Reset();
    theMission.m_messages4.UnInit();
    func_00363E60(0.0f);
    func_00363E50(0.0f);
    data_003DC3E0 = 1;
    hudTurnOffScorePopup();
}

/* while the HUD draws, one of its three phase lists drawn (the HUD's three render phases) */
void CHUD::DrawPhase(zdb::_RenderPhase phase, zdb::CCamera *cam)
{
    if (m_draw)
        m_phase[phase - zdb::HUDSET1].Draw(cam);
}

zdb::CTexture *GetTextureByName(zdb::CWorld *world, const char *name)
{
    zdb::CTexHandle *h = zdb::CAssetMgr::m_assets.GetTexHandle(name);

    if (h)
        return h->m_texture;
    return 0;
}

void hudTurnOffScorePopup();

/* the members' clean-ups not named yet: the info box's, the spectator's, the ammo counts', the
 * lens flares' */
extern "C" void func_00242AB0(CSpectatorInfo *info);

/* everything a mission set up taken down for the next: the map and the selectors, the cameras,
 * icons and reticle, every string wiped, the pause menu closed and cleared, the flashes, the
 * network boxes, the map items and the score popup, and the network state */
void CHUD::ReInit_Uninit()
{
    if (!theNetwork.m_bNetwork) {
        m_teamnames.Clear();
        m_newmap->UninitMap();
    }
    m_weaponsel.UnInit();
    m_ingameweaponsel.UnInit();
    if (theNetwork.m_bNetwork)
        m_mpweaponsel.UnInit();
    m_missioncams.UnInit();
    m_actionbitmap.UnInit();
    m_bitmapReticule.UnInit();
    m_firemodename.ClearAndWipe();
    m_ammocount.ClearAndWipe();
    m_magscount.ClearAndWipe();
    m_ammotype.ClearAndWipe();
    m_weaponname.ClearAndWipe();
    m_rangetext.ClearAndWipe();
    for (unsigned int i = 0; i < 6; i++)
        m_piggyback_text[i].ClearAndWipe();
    m_socom2_1a0e8 = -1.0f;
    m_socom2_1a0f0 = -1;
    m_ammodisplay.Reset();
    m_zoomleveltext.ClearAndWipe();
    m_socom2_0.ClearAndWipe();
    m_message.ClearAndWipe();
    m_timertext.ClearAndWipe();
    for (int i = 0; i < 30; i++)
        m_perfstats[i].ClearAndWipe();
    m_pausetest.ClosePauseMenu();
    m_pausetest.Clear();
    m_poseBitmap.Clear();
    m_FlashFXVector.Reset();
    m_teamMateNames.UnInit();
    m_infobox.UnInit();
    func_00242AB0(&m_spectatorinfo);
    CZPlayerMapItem::MissionEndCleanup();
    hudTurnOffScorePopup();
    theTCM.Close(false);
    theTCM.m_dcd = 1;
    m_lensflares.Reset();
    theTCM.ClearItems();
}

extern "C" int sprintf(char *buf, const char *fmt, ...);

/* the projectiles' last delayed detonation (zweapon.h's static members, the weapons lane's names:
 * ClearNewExplosion 0x3C6980 clears its flag) */
class CZProjectile {
public:
    static void ClearNewExplosion();
};

/* not named yet: SOCOM II's own re-inits and resets */
void hudResetPopupTriggers();
extern "C" void func_00242B20(CSpectatorInfo *info);
extern "C" void func_002B6530(C2DMessage_Q *q, const char *text, int a, int b, int c, float time);


/* the HUD made ready for the next mission: on, the letterbox and the piggyback strings put away,
 * zoomed out, the team names (single player) or the network boxes set up, the message queues
 * given the HUD's font, a network round announced, the map re-inited on the player */
void CHUD::ReInit_Init()
{
    if (appCamera)
        StopShake__10CAppCameraFv(appCamera);
    CZProjectile::ClearNewExplosion();
    theHUD->On();
    theHUD->m_letterbox.RemoveLetterbox();
    for (unsigned int i = 0; i < 6; i++) {
        m_piggyback_text[i].Off();
        m_piggyback_text[i].SetTrans(100.0f);
    }
    m_socom2_1a0e8 = -1.0f;
    m_socom2_1a0f0 = -1;
    ZoomOut();
    if (!theNetwork.m_bNetwork)
        m_teamnames.Init();
    else
        m_teamnames.Off();
    m_pausetest.ReInit();
    m_poseBitmap.ReInit();
    m_infobox.ReInit();
    func_00242B20(&m_spectatorinfo);
    if (theNetwork.m_bNetwork) {
        m_teamMateNames.Init();
    } else {
        m_infobox.Off();
        m_spectatorinfo.Off();
        m_newmap->InitMap(zdb::CWorld::m_world);
    }
    theMission.m_messages2.m_font = m_myfont;
    theMission.m_messages.m_font = m_myfont;
    theMission.m_messages3.m_font = m_myfont;
    theMission.m_messages4.Clear();
    if (theMission.m_1808 && theNetwork.m_bNetwork) {
        CValve *max = CValve::GetByName("mp_max_rounds");
        CValve *count = CValve::GetByName("mp_round_count");
        int maxrounds = max ? max->m_value : -1;
        int round = count ? count->m_value + 1 : -1;
        char text[128];

        if (round <= maxrounds)
            sprintf(text, "STARTING ROUND %d OF %d", round, maxrounds);
        else
            sprintf(text, "PLAYING TIEBREAKER ROUND", round, maxrounds);
        func_002B6530(&theMission.m_messages, text, 0, 1, 0, 0.9f);
    }
    if (!theNetwork.m_bNetwork) {
        m_newmap->m_viewer = ftsGetPlayer();
        m_newmap->ReInit();
    }
    hudResetPopupTriggers();
    m_bitmapReticule.HideTimer();
    m_ammodisplay.Reset();
    CameraZoom::Reset();
    theTCM.SetupItemBars();
    m_weaponsel.ResetHotSwap(true);
    m_lensflares.ReInit();
    m_socom2_1a0e8 = 0.0f;
}

/* the game paused under a menu: the pause menu, the full-screen map or the orders, single player
 * only (SOCOM 1 asked the pause menu and the controller's menu state; SOCOM II only the state).
 * Each test is a bool of its own: written in the || itself, MW folds the three into one range
 * test (ms - 2 < 3), where the target materialises each and chains them */
bool CHUD::PauseGame()
{
    MENU_STATE menustate = GetHUDOwner()->m_ctrl->GetMenuState();
    bool pausemenu = menustate == statePauseTestMenu;
    bool mapmenu = menustate == stateFullScreenMapMenu;
    bool orders = menustate == stateOrders;

    return (pausemenu || mapmenu || orders) && !theNetwork.m_bNetwork;
}

/* where the piggyback lines stand (variables: read with lwc1) */
static float s_piggyback_x = 324.0f;
static float s_piggyback_y = 380.0f;
static float s_piggyback_spacing = 18.0f;

/* the string table's text by id (0x361B40) and the screen's width (OVL1 bss 0x4A44C4); no rows
 * named yet */
const char *GetString(int id);
extern unsigned int data_004A44C4;
extern "C" int toupper(int c);

/* the network game's state as the ghost's text reads it (OVL1 bss 0x437CE8, the pointer the radio
 * icons test too: a byte at 0xD2 set for a ghost; no row named yet) */
class CNetGameState {
public:
    unsigned char _0[0xd2];
    unsigned char m_ghost;   // 0xd2
    unsigned char _d3[0xdc - 0xd3];
    unsigned char m_respawns;   // 0xdc: the game respawns the dead (the name ours)
};

extern CNetGameState *data_00437CE8;

/* the ghost text's local static point and its guard (OVL1 bss; no rows named yet) */
extern signed char data_00408EF0;
extern CPnt3D data_00408EF8;

/* the dead player's lines: a ghost's four (it plays the next round) or the dead's three, the rest
 * blank, the HUD off, the boxes, queues and counts on (the spectator box only while spectating),
 * and the six lines placed and shown (SOCOM II's: CHUD::Tick calls it; the name ours) */
void CHUD::SetupGhostText(MENU_STATE menustate)
{
    CZSealBody *player = ftsGetPlayer();

    if (data_00437CE8->m_ghost) {
        m_piggyback_text[0].SetString("You are a ghost.  You will play the next", true);
        m_piggyback_text[1].SetString("round as a real player.  Use the \xa3 and", true);
        m_piggyback_text[2].SetString("\xaf directional buttons to cycle through", true);
        m_piggyback_text[3].SetString("living teammates.", true);
        for (unsigned int i = 4; i < 6; i++)
            m_piggyback_text[i].SetString(" ", true);
    } else {
        m_piggyback_text[0].SetString("You have died.  Use the \xa3 and \xaf", true);
        m_piggyback_text[1].SetString("directional buttons to cycle through", true);
        m_piggyback_text[2].SetString("living teammates.", true);
        for (unsigned int i = 3; i < 6; i++)
            m_piggyback_text[i].SetString(" ", true);
    }
    theHUD->Off();
    if (menustate != statePauseTestMenu) {
        m_teamMateNames.On();
        m_infobox.On();
        theMission.m_messages.On();
        theMission.m_messages4.On();
        m_ammodisplay.On();
        if (theNetwork.m_bNetwork && func_002C2FA0())
            m_spectatorinfo.On();
    }
    /* a function-local `static CPnt3D color(100.0f, 100.0f, 20.0f);`, unused but constructed under
     * its guard (that form matches too, but its two BSS objects go unchecked: the placeholders
     * hold them to their addresses) */
    if (!data_00408EF0) {
        data_00408EF8 = CPnt3D(100.0f, 100.0f, 20.0f);
        data_00408EF0 = 1;
    }

    for (unsigned int i = 0; i < 6; i++) {
        m_piggyback_text[i].m_isCentered = 1;
        m_piggyback_text[i].SetPos((int)s_piggyback_x,
                                   (int)(s_piggyback_y + i * s_piggyback_spacing));
        m_piggyback_text[i].UpdatePos();
        if (menustate != statePauseTestMenu)
            m_piggyback_text[i].On();
        else
            m_piggyback_text[i].Off();
    }
}

/* the seal's node as the respawn text reads it (CEntity::GetNode's; a float at 0x9C zero once the
 * seal lies dead) and the pad's type (0x2C6430: 11 for one layout of the buttons; no row named
 * yet) */
class CRespawnNode {
public:
    unsigned char _0[0x9c];
    float m_9c;   // 0x9c
};

extern "C" unsigned char func_002C6430(int pad);

/* the dead player's respawn lines: a ghost's (it plays the next round, may select new weapons
 * and respawn) or the dead's, the button glyphs by the pad's layout, the respawn offered five
 * seconds after death while the seal's node lies still; the HUD on (the queues and compass off)
 * unless under the pause menu, and the six lines placed and shown (SOCOM II's: CHUD::Tick calls
 * it; the name ours) */
void CHUD::SetupRespawnText(MENU_STATE menustate)
{
    CZSealBody *player = ftsGetPlayer();

    if (data_00437CE8->m_ghost) {
        m_piggyback_text[0].SetString("You are a ghost.  You will play the next", true);
        bool layout = func_002C6430(4) == 11;
        char line1[64];

        sprintf(line1, "round as a real player.  %c Select new", layout ? '\xbe' : '\xbd');
        m_piggyback_text[1].SetString(line1, true);
        if (data_00437CE8->m_respawns) {
            float waited = theMission.m_socom2_e0 - player->m_socom2_fb4;
            CRespawnNode *node = player ? player->GetNode() : 0;
            bool still = node ? node->m_9c == 0.0f : false;

            if (!func_002C2FA0() && still && !(waited <= 5.0f)) {
                char glyph = layout ? '\xa6' : '\xb7';
                char line2[128];

                if (m_piggyback_text[2].m_color[3] < 100.0f) {
                    sprintf(line2, "Press the %c button to respawn.", glyph);
                    m_piggyback_text[2].SetString(line2, true);
                    m_piggyback_text[2].SetTrans(100.0f);
                } else if (layout) {
                    sprintf(line2, "weapons. Press the %c button to respawn.", glyph);
                    m_piggyback_text[2].SetString(line2, true);
                }
            } else {
                m_piggyback_text[2].SetString("weapons.", true);
            }
            m_piggyback_text[3].SetString(" ", true);
        } else {
            m_piggyback_text[2].SetString("weapons. Use the \xa3 and \xaf directional", true);
            m_piggyback_text[3].SetString("buttons to cycle through living teammates.", true);
        }
        for (unsigned int i = 4; i < 6; i++)
            m_piggyback_text[i].SetString(" ", true);
    } else {
        bool layout = func_002C6430(4) == 11;
        char line0[64];

        sprintf(line0, "You have died.  %c Select new weapons.", layout ? '\xbe' : '\xbd');
        m_piggyback_text[0].SetString(line0, true);
        if (data_00437CE8->m_respawns) {
            float waited = theMission.m_socom2_e0 - player->m_socom2_fb4;
            CRespawnNode *node = player ? player->GetNode() : 0;
            bool still = node ? node->m_9c == 0.0f : false;

            if (!func_002C2FA0() && still && !(waited <= 5.0f)) {
                if (layout)
                    m_piggyback_text[1].SetString("Press the \xa6 button to respawn.", true);
                else
                    m_piggyback_text[1].SetString("Press the \xb7 button to respawn.", true);
                m_piggyback_text[1].SetTrans(100.0f);
            } else {
                m_piggyback_text[1].SetString(" ", true);
            }
            m_piggyback_text[2].SetString(" ", true);
        } else {
            m_piggyback_text[1].SetString("Use the \xa3 and \xaf directional buttons", true);
            m_piggyback_text[2].SetString("to cycle through living teammates", true);
        }
        for (unsigned int i = 3; i < 6; i++)
            m_piggyback_text[i].SetString(" ", true);
    }
    if (menustate != statePauseTestMenu) {
        theHUD->On();
        theMission.m_messages3.Off();
        theMission.m_messages2.Off();
        m_mapcompass.Off();
        if (theNetwork.m_bNetwork && !func_002C2FA0())
            m_spectatorinfo.Off();
    } else {
        theHUD->Off();
    }
    for (unsigned int i = 0; i < 6; i++) {
        m_piggyback_text[i].m_isCentered = 1;
        m_piggyback_text[i].SetPos((int)s_piggyback_x,
                                   (int)(s_piggyback_y + i * s_piggyback_spacing));
        m_piggyback_text[i].UpdatePos();
        if (menustate != statePauseTestMenu)
            m_piggyback_text[i].On();
        else
            m_piggyback_text[i].Off();
    }
}

/* the spectator's top lines: "spectating" (the string table's 0xEC4F) and the watched seal's name
 * in capitals, centred, or blanks; blanks for a ghost; the rest blank, all shown unless under the
 * pause menu, the HUD off and the letterbox in (SOCOM II's: CHUD::Tick calls it, in no menu
 * only; the name ours) */
void CHUD::SetupSpectatingText(MENU_STATE menustate)
{
    if (menustate != stateNone)
        return;
    CEntity *entity = CEntity::m_list.GetEntityById(ftsGetPlayer()->m_socom2_fc4);

    if (entity) {
        CEntity *seal = entity && entity->m_type == 2 ? entity : 0;
        const char *text = GetString(0xec4f);
        char name[256];

        m_piggyback_text[0].SetString(text, false);
        m_piggyback_text[0].m_isCentered = 1;
        m_piggyback_text[0].SetPos((int)(0.5f * data_004A44C4), 25);
        m_piggyback_text[0].On();
        strcpy(name, seal->m_name ? seal->m_name : "???");
        unsigned int len = strlen(name);

        for (unsigned int i = 0; i < len; i++)
            name[i] = toupper(name[i]);
        m_piggyback_text[1].SetString(name, true);
        m_piggyback_text[1].m_isCentered = 1;
        m_piggyback_text[1].SetPos((int)(0.5f * data_004A44C4), (int)(25.0f + s_piggyback_spacing));
        m_piggyback_text[1].On();
    } else {
        m_piggyback_text[0].SetString(" ", true);
        m_piggyback_text[1].SetString(" ", true);
    }
    if (data_00437CE8->m_ghost) {
        m_piggyback_text[0].SetString(" ", true);
        m_piggyback_text[1].SetString(" ", true);
    }
    for (unsigned int i = 2; i < 6; i++)
        m_piggyback_text[i].SetString(" ", true);
    for (unsigned int i = 0; i < 6; i++) {
        if (menustate != statePauseTestMenu)
            m_piggyback_text[i].On();
        else
            m_piggyback_text[i].Off();
    }
    theHUD->Off();
    theHUD->m_letterbox.AddLetterbox();
}

/* the spectator's four lines on the piggyback strings, the HUD off, the network boxes and message
 * queues on (unless under the pause menu), and the six lines placed and shown (SOCOM II's: CHUD::
 * Tick calls it; the name ours) */
void CHUD::SetupSpectatorText(MENU_STATE menustate)
{
    CZSealBody *player = ftsGetPlayer();

    m_piggyback_text[0].SetString("You are a spectator.  Use the directional", true);
    m_piggyback_text[1].SetString("buttons to cycle through active players", true);
    m_piggyback_text[2].SetString("and fixed views.  \xa6 Use the Free-Motion", true);
    m_piggyback_text[3].SetString("Camera.  \xac Jump to visible players.", true);
    theHUD->Off();
    if (menustate != statePauseTestMenu) {
        m_teamMateNames.On();
        m_infobox.On();
        m_spectatorinfo.On();
        theMission.m_messages.On();
        theMission.m_messages4.On();
    }
    for (unsigned int i = 0; i < 6; i++) {
        m_piggyback_text[i].m_isCentered = 1;
        m_piggyback_text[i].SetPos((int)s_piggyback_x,
                                   (int)(s_piggyback_y + i * s_piggyback_spacing));
        m_piggyback_text[i].UpdatePos();
        if (menustate != statePauseTestMenu)
            m_piggyback_text[i].On();
        else
            m_piggyback_text[i].Off();
    }
}

/* a flash: a fader made, darkened no further than the brightness, coloured, flashed and kept */
void CHUD::Flash(float brightness, const CPnt3D &color, float fadeout_time, float fade_time,
                 float fadein_time)
{
    CFlashFX *fader = new CFlashFX;

    fader->SetMinBrightness(brightness);
    fader->SetColor(color.x, color.y, color.z);
    fader->Flash(fadeout_time, fade_time, fadein_time);
    m_FlashFXVector.Add(fader);
}

void CHUD::PreDITick(float dT)
{
    m_lensflares.PreDITick(dT);
}

void CHUD::PostDITick(float dT)
{
    m_lensflares.Tick(dT);
}

/* the HUD shown: what the player's own view has (the action icon, the reticle and the stance)
 * only on the player, the selectors and counts unless spectating, the team names in single
 * player while the seal is in play, the zoom's state put back, the map in full screen if it was */
void CHUD::On()
{
    CZSealBody *owner = GetHUDOwner();
    CZSealBody *player = ftsGetPlayer();

    m_ison = true;
    if (theNetwork.m_bNetwork)
        m_num_connected.On();
    if (!theNetwork.m_bNetwork) {
        if (theMission.GetTimeLeft() > 0) {
            m_timertext.On();
            m_socom2_4ec0.On();
        }
    } else if (player == owner) {
        m_timertext.On();
        m_socom2_4ec0.On();
    }
    if (!theNetwork.m_bNetwork || theNetwork.m_bNetwork && !func_002C2FA0()) {
        if (player == owner) {
            m_actionbitmap.On();
            m_bitmapReticule.On();
            m_poseBitmap.On();
        }
        m_ingameweaponsel.On();
        if (theNetwork.m_bNetwork)
            m_mpweaponsel.On();
        m_zoomleveltext.On();
        m_socom2_0.On();
        m_ammodisplay.On();
        if (ftsGetPlayer()->m_socom2_e1_4)
            theMission.m_messages3.On();
    }
    if (!theNetwork.m_bNetwork && !ftsGetPlayer()->m_socom2_e1_4)
        m_teamnames.Off();
    else
        m_teamnames.On();
    if (!theNetwork.m_bNetwork)
        m_newmap->On();
    m_rangetext.On();
    m_weaponsel.On();
    theMission.m_messages.On();
    theMission.m_messages4.On();
    if (m_hudmode == ZOOMOUT)
        ZoomOut();
    else
        ZoomIn();
    if (m_newmap ? m_newmap->m_isFullScreen : false)
        SwitchToFullScreenMap();
    m_teamMateNames.On();
    if (theNetwork.m_bNetwork) {
        m_infobox.On();
        m_spectatorinfo.On();
    }
    m_mapcompass.On();
}

/* the HUD hidden: every item off (the timer and the connection count in single player, the
 * network boxes in a network game -- the info box stays under the pause menu, where the
 * reticle's bitmap comes back on), the message queues off and the kit's flag cleared */
void CHUD::Off()
{
    if (!theNetwork.m_bNetwork) {
        m_timertext.Off();
        m_socom2_4ec0.Off();
        m_num_connected.Off();
    }
    m_actionbitmap.Off();
    m_teamnames.Off();
    m_bitmapReticule.Off();
    m_poseBitmap.Off();
    if (!theNetwork.m_bNetwork)
        m_newmap->Off();
    m_rangetext.Off();
    m_weaponsel.Off();
    m_ingameweaponsel.Off();
    if (theNetwork.m_bNetwork)
        m_mpweaponsel.Off();
    m_zoomleveltext.Off();
    m_socom2_0.Off();
    m_ammodisplay.Off();
    m_teamMateNames.Off();
    MENU_STATE menustate = ftsGetPlayer()->m_ctrl->GetMenuState();

    if (theNetwork.m_bNetwork) {
        if (menustate != statePauseTestMenu)
            m_infobox.Off();
        m_spectatorinfo.Off();
    }
    if (menustate == statePauseTestMenu)
        m_bitmapReticule.OnPauseTest();
    m_ison = false;
    theMission.m_messages.Off();
    theMission.m_messages3.Off();
    theMission.m_messages4.Off();
    GetHUDOwner()->m_kit.m_socom2_b16 = false;
    m_mapcompass.Off();
}

bool CHUD::IsOn()
{
    return m_ison;
}

/* zoomed out (the full view): unless the map is in full screen, the compass and the weapon back,
 * the kit's reticle reset, the counts on unless the night vision or 0x5C80F0 holds, the message
 * queues' backgrounds grey again, the team names and map (single player), the zoom text */
void CHUD::ZoomOut()
{
    CZSealBody *owner = appCamera->m_bc;
    CZKit *kit = &owner->m_kit;

    kit->GetCurItem();
    if (!theNetwork.m_bNetwork && m_newmap->m_isFullScreen)
        return;
    m_hudmode = ZOOMOUT;
    if (!m_ison)
        return;
    m_mapcompass.On();
    weapon.On();
    if (!owner)
        return;
    kit->SetCurReticule(false);
    if (!kit->IsUsingScope() && !kit->NVG_Enabled())
        m_ammodisplay.On();
    m_ammodisplay.Update();
    theMission.m_messages.SetBkgColor(128.0f, 128.0f, 128.0f);
    theMission.m_messages3.SetBkgColor(128.0f, 128.0f, 128.0f);
    if (!theNetwork.m_bNetwork) {
        if (ftsGetPlayer()->m_socom2_e1_4)
            m_teamnames.On();
        m_newmap->On();
    }
    m_poseBitmap.On();
    SetupZoomText();
    m_actionbitmap.On();
    if (theNetwork.m_bNetwork) {
        m_teamMateNames.On();
        m_infobox.On();
        m_spectatorinfo.On();
    }
}

/* zoomed in (a scope or the binoculars): unless the map is in full screen, the compass, the
 * weapon, the counts, the team names, the stance and the map off, the kit's reticle set, the
 * zoom text shown */
void CHUD::ZoomIn()
{
    if (!theNetwork.m_bNetwork && m_newmap->m_isFullScreen)
        return;
    m_hudmode = ZOOMIN;
    if (!m_ison)
        return;
    m_mapcompass.Off();
    weapon.Off();
    CZSealBody *owner = appCamera->m_bc;

    if (!owner)
        return;
    owner->m_kit.SetCurReticule(false);
    CZKit *kit = &owner->m_kit;
    CZFTSWeapon *pItem = kit->GetCurItem();

    if (pItem && pItem->m_type == 11)
        m_ammodisplay.Off();
    /* the target's block order: the night-vision path's Off first, jumping over the other's
     * (an if/else with an Off in each arm puts the then-arm's whole; a goto reproduces it) */
    if (!kit->IsUsingScope()) {
        kit->NVG_Enabled();
        goto off;
    }
    m_ammodisplay.Off();
    goto counted;
off:
    m_ammodisplay.Off();
counted:
    if (!theNetwork.m_bNetwork)
        m_teamnames.Off();
    m_poseBitmap.Off();
    if (!theNetwork.m_bNetwork)
        m_newmap->Off();
    SetupZoomText();
    m_actionbitmap.Off();
    if (theNetwork.m_bNetwork) {
        m_infobox.Off();
        m_spectatorinfo.Off();
    }
}

/* the map in full screen: the reticle, the names, the timer, the counts, the stance, the zoom
 * text and the message queues off, the map on (single player), the network boxes off */
void CHUD::SwitchToFullScreenMap()
{
    m_bitmapReticule.Off();
    m_teamnames.Off();
    m_timertext.Off();
    m_socom2_4ec0.Off();
    m_ammodisplay.Off();
    m_poseBitmap.Off();
    if (!theNetwork.m_bNetwork)
        m_newmap->On();
    m_zoomleveltext.Off();
    m_socom2_0.Off();
    theMission.m_messages.Off();
    theMission.m_messages2.Off();
    theMission.m_messages4.Off();
    m_actionbitmap.Off();
    if (theNetwork.m_bNetwork) {
        m_teamMateNames.Off();
        m_infobox.Off();
    }
}

/* back to the compass: everything the full-screen map hid shown again, the zoom's state put back,
 * and the elements' fade-in restarted */
void CHUD::SwitchToCompassMap()
{
    m_bitmapReticule.On();
    m_teamnames.On();
    m_timertext.On();
    m_socom2_4ec0.On();
    m_ammodisplay.On();
    m_poseBitmap.On();
    m_zoomleveltext.On();
    m_socom2_0.On();
    theMission.m_messages.On();
    theMission.m_messages2.On();
    theMission.m_messages4.On();
    if (m_hudmode == ZOOMOUT)
        ZoomOut();
    else
        ZoomIn();
    if (theNetwork.m_bNetwork) {
        m_teamMateNames.On();
        m_infobox.On();
    }
    g_fElemsFadeInTime = 0.0f;
}

/* a 2D object put on one of the HUD's three phase lists, at the back or the front; phase -1 takes
 * the render phase of the first of its asset libraries that has one (a HUD set, else the third).
 * The shape is the target's: the phase zeroed before the call (it shares the parameter's saved
 * register), the walk guarded by the size and written as a while (the size test, then the
 * materialised compare twice, as the target), each library's phase read before the test, and the
 * back insert returning early (as an else, both arms meet at a branch of their own) */
void CHUD::Add2D(C2D *p2d, int phase, bool add_to_back)
{
    if (phase == -1) {
        C2D::CLibList liblist;

        zdb::_RenderPhase rphase = zdb::WORLD;

        p2d->GetAssetLibs(liblist);
        if (liblist.size()) {
            C2D::CLibList::iterator ii = liblist.begin();

            while (ii != liblist.end()) {
                zdb::_RenderPhase libphase = (*ii)->m_renderphase;

                if (rphase == zdb::WORLD)
                    rphase = libphase;
                ii++;
            }
        }
        if (rphase >= zdb::HUDSET1 && rphase <= zdb::HUDSETSMALL)
            phase = rphase - zdb::HUDSET1;
        else
            phase = 2;
    }
    if (add_to_back) {
        m_phase[phase].push_back(p2d);
        return;
    }
    m_phase[phase].push_front(p2d);
}

/* the zoom's text ("ZOOM: 2.0x"): the binoculars' fixed 9x or the kit's zoom by the seal's state,
 * shown only above 1.01 (SOCOM 1's ZoomIn printed it into its static zoomtext; SOCOM II's ZoomIn
 * and ZoomOut share this, the name ours) */
void CHUD::SetupZoomText()
{
    static char zoomtext[32] = "uninit";
    CZSealBody *owner = GetHUDOwner();

    if (!owner)
        return;
    CZKit *kit = &owner->m_kit;
    float zoom = 0.0f;

    switch (owner->m_socom2_200) {
    case 2:
    case 3:
        zoom = 0.0f;
        break;
    case 4:
        zoom = 9.0f;
        break;
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
        zoom = kit->GetCurZoom();
        break;
    }
    if (zoom <= 1.01f) {
        m_zoomleveltext.Off();
        return;
    }
    sprintf(zoomtext, "ZOOM: %2.1fx", zoom);
    m_zoomleveltext.On();
    m_zoomleveltext.Load(0.9f, zoomtext, m_myfont, 20.0f, 420.0f, 1);
}

/* the timer text's buffer and the seconds it last showed (OVL1 bss; no rows named yet) */
extern char data_00408F10[];
extern int data_00408F88;

/* the mission's clock ("%02d:%02d"), reprinted when its seconds change: shown with its
 * background unless the seal is out of play in single player; blanked once the time runs out
 * (SOCOM II's: CHUD::Tick calls it; the name ours) */
void CHUD::SetupTimerText()
{
    int time = theMission.GetTimeLeft();
    int secs = time % 60;

    if (secs != data_00408F88) {
        data_00408F88 = secs;
        sprintf(data_00408F10, "%02d:%02d", time / 60, secs);
        m_timertext.SetString(data_00408F10, false);
        CZSealBody *owner = GetHUDOwner();

        if (owner && !owner->m_socom2_e1_4 && !theNetwork.m_bNetwork) {
            m_timertext.Off();
            m_socom2_4ec0.Off();
        } else {
            m_timertext.On();
            if (!theNetwork.m_bNetwork)
                m_socom2_4ec0.On();
        }
    }
    if (time < 0 && !theNetwork.m_bNetwork) {
        m_timertext.SetString("", false);
        m_socom2_4ec0.Off();
    }
}

/* the range text given, or blanked while it is off (BitmapReticule's tick, 0x216770, passes the
 * range it printed; the four range-text members are the unit's first in the image, so defined
 * last -- the names ours) */
void CHUD::SetRangeText(const char *text)
{
    m_socom2_0.SetString(m_socom2_0.IsOn() ? text : "", true);
}

/* the range text moved (whole pixels) */
void CHUD::SetRangeTextPos(float x, float y)
{
    m_socom2_0.SetPos((int)x, (int)y);
}

/* the range text shown */
void CHUD::ShowRangeText()
{
    m_socom2_0.On();
}

/* the range text hidden and moved off the screen */
void CHUD::HideRangeText()
{
    m_socom2_0.Off();
    m_socom2_0.SetPos(-100, -100);
}

float g_fElemsFadeInTime = -1.0f;
