/* hud_main.cpp -- the HUD (CHUD): SOCOM 1's unit (its DWARF names it, Z:\dev\Apps\FTS\hud_main.cpp),
 * its functions in SOCOM 1's order. The part in OVL1 0x1F9000-0x1FE33F (Init to the constructor,
 * then the inline functions' out-of-line copies) is this lane's; the functions before it
 * (DoRangeText .. PauseGame, 0x1F6xxx-0x1F8xxx) are not written. SOCOM II's CHUD is 0x1ACD0 bytes
 * (SOCOM 1's 0x14960), laid out from its constructor, destructor and ReInit_Uninit: a string in
 * front of the texture lists, the map new'ed (single player only), a network weapon selector,
 * the multiplayer boxes (teammate names, the info box with the health bar and who is talking,
 * the spectator's), six strings where SOCOM 1 had three piggyback texts, a lens flare manager
 * where it had CLensFlareSaturate, and the ammo counts after the phases.
 * Written: the constructor (byte-exact but for two calls to rows outside the range, CHealthBar's
 * and CSpectatorInfo's constructors), GetHUDOwner, HudText_to_upper, DrawPhase, GetTextureByName,
 * ReInit_Uninit and ReInit_Init. Not yet: Init (0xF98), the destructor (0xA40: it inlines every
 * member's destructor, and needs the texture lists as classes -- see CTexList) and ReleaseHUD
 * (it inlines C2DFont's destructor, which holds a std::map and a std::string in SOCOM II).
 * Defined in reverse of their image order (-inline deferred). */

#include <string.h>
#include <list>
#include <vector>

#include "../../gamez/ztwod/ztwod.h"
#include "../../gamez/znode/znode.h"
#include "hud_ammo.h"
#include "hud_fader.h"
#include "hud_lensflare.h"
#include "hud_letterbox.h"
#include "hud_mapcompass.h"
#include "hud_missioncams.h"
#include "hud_mpinfobox.h"
#include "hud_newhudmap.h"
#include "hud_teammatenames.h"

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

/* the seal as the HUD reads it: its controller (CEntity's, at 0xC0) */
class CZSealBody {
public:
    unsigned char _0[0xc0];
    CSealCtrl *m_ctrl;   // 0xc0
    unsigned char _c4[0xe1 - 0xc4];
    unsigned char _e1_0 : 4;
    unsigned char m_socom2_e1_4 : 1;   // 0xe1 bit 4 (MapCompass::On tests it too)
    unsigned char _e1_5 : 3;
};

/* MSL's toupper (MAIN; no map row yet) */
extern "C" int func_0019AFD0(int c);

/* the app camera: only the SEAL it follows (SOCOM 1's m_entity, 0x80 further on in SOCOM II) */
class CAppCamera {
public:
    unsigned char _0[0xbc];
    CZSealBody *m_bc;   // 0xbc
};

extern CAppCamera *appCamera;

class CZNetwork {
public:
    int m_SealIndex;   // 0x0
    int m_TerrIndex;   // 0x4
    unsigned char m_bSessionMaster;   // 0x8
    unsigned char m_bNetwork;   // 0x9
};

extern CZNetwork theNetwork;

/* the dynamic texture lists (SOCOM 1's DWARF: two vectors, of texture handles and palettes).
 * UNRESOLVED: SOCOM 1 makes them classes, zdb::CTexList : vector and CTexPalList : vector, and
 * so does SOCOM II's destructor -- ~CHUD inlines CDynTexList's destructor one level and calls two
 * out-of-line ones with four this tests each (0x1FC520, 0x1FC5A0: the class's level over the
 * pointer vector's three), five levels in all. But CHUD's constructor inlines CDynTexList's
 * constructor whole (six stores), which needs at most four: with the classes it calls their
 * constructors instead. The pointer vector's constructor would have to be a level shorter than
 * its destructor, which __vector_pod's user-declared copy constructor (0x259B40, so a user
 * default constructor too) rules out in include/vector's model. Typedefs here, which the
 * constructor needs and the destructor will not take */
typedef std::vector<zdb::CTexHandle *> CTexList;
typedef std::vector<zdb::CTexPal *> CTexPalList;

class CDynTexList {
public:
    CTexList m_tex_list;   // 0x0
    CTexPalList m_pal_list;   // 0xc
};   // 0x18 bytes

class UIVarMixin;

/* SOCOM 1's UIVarMixin::DetachUIVar(CUIVariable *) (0x385D90; no row yet: called as the
 * placeholder until it is named) */
extern "C" void func_00385D90(UIVarMixin *mixin, CUIVariable *var);

/* a UI variable's hook (SOCOM 1's DWARF; the vtable pointer first). The destructor detaches
 * whatever is attached, as SOCOM 1's */
class UIVarMixin {
public:
    UIVarMixin() { m_uivar = 0; }
    virtual ~UIVarMixin() { func_00385D90(this, 0); }

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

/* the HUD's brighter text: a string with its own vtable and nothing else */
class CHudBrightText : public C2DString {
public:
    CHudBrightText() { SetTrans(80.0f); }
};   // 0x98 bytes

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



/* the classes with constructors of their own */
class CWeaponSel {
public:
    CWeaponSel();
    void UnInit();
    unsigned char _0[0xce0];
} __attribute__((aligned(16)));

class CInGameWeaponSel {
public:
    CInGameWeaponSel();
    void UnInit();
    unsigned char _0[0x11b0];
} __attribute__((aligned(16)));

/* the network game's weapon selector: slot lists honouring the server's weapon valves
 * (Enable_203FRAG and the rest), UnInit only in a network game (SOCOM II's; no vtable, so no
 * RTTI name: the name is ours) */
class CMPWeaponSel {
public:
    CMPWeaponSel();
    void UnInit();
    unsigned char _0[0x1540];
} __attribute__((aligned(16)));

class PoseBitmap {
public:
    PoseBitmap();
    void Clear();
    void ReInit();
    unsigned char _0[0x150];
} __attribute__((aligned(16)));

class BitmapReticule {
public:
    BitmapReticule();
    void UnInit();
    unsigned char _0[0x47b0];
} __attribute__((aligned(16)));

class CTeamNames : public C2D {
public:
    CTeamNames();
    /* declared here so that this unit emits no copy: its members are not placed (the linked
     * copy, 0x1FD730, is an implicit one; ~CHUD will need the real shape) */
    virtual ~CTeamNames();
    void Init();
    void Clear();
    unsigned char _18[0xf30 - 0x18];
} __attribute__((aligned(16)));

class CSpectatorInfo : public C2D {
public:
    CSpectatorInfo();
    virtual ~CSpectatorInfo();
    unsigned char _18[0x680 - 0x18];
} __attribute__((aligned(16)));

class CZPauseTest {
public:
    CZPauseTest();
    void Clear();
    void ClosePauseMenu();
    unsigned char _0[0x13c0];
} __attribute__((aligned(16)));

enum HUD_MODE {
    HUD_MODE_0 = 0
};

/* the jukebox's, or so: a string table registered by name (no rows named yet) */
class CStringTables;
extern "C" CStringTables *func_00361BC0();
extern "C" int func_003A4460();
extern "C" void func_003A3900(CStringTables *tables, const char *name, int lang);

class CHUD {
public:
    CHUD();
    ~CHUD();
    void Add2D(C2D *p2d, int phase, bool add_to_back);
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

    C2DString m_socom2_0;   // 0x0
    CDynTexList m_dyntexlist;   // 0x98
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

/* the HUD's members built in order; then it is the HUD, the map is new'ed for a single-player
 * game (none in a network game), and the HUD's string table registered */
CHUD::CHUD()
    : m_hudrettype(-1), m_usehudret(false), m_hudretchanged(false), m_firemodetex(0),
      m_messageTime(0.0f)
{
    theHUD = this;
    m_newmap = !theNetwork.m_bNetwork ? new CZNewHudMap : 0;
    CStringTables *tables = func_00361BC0();

    func_003A3900(tables, "HudC", func_003A4460());
}

CZSealBody *CHUD::GetHUDOwner()
{
    return appCamera->m_bc;
}

void CHUD::HudText_to_upper(char *instr)
{
    int len = strlen(instr);

    for (int kk = 0; kk < len; kk++)
        instr[kk] = func_0019AFD0(instr[kk]);
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

class CZPlayerMapItem {
public:
    static void MissionEndCleanup();
};

void hudTurnOffScorePopup();

/* the multiplayer HUD's state (OVL1 bss, 0x413120; no row yet): a byte at 0xDCD, a list at 0xDE0
 * (func_00237310 clears it) */
struct CHudNetState {
    unsigned char _0[0xdcd];
    unsigned char m_dcd;   // 0xdcd
};

extern CHudNetState data_00413120;
extern "C" void func_00235300(CHudNetState *state, int flag);
extern "C" void func_00237310(CHudNetState *state);
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
    func_00235300(&data_00413120, 0);
    data_00413120.m_dcd = 1;
    m_lensflares.Reset();
    func_00237310(&data_00413120);
}


/* the app camera (the shake's stop only) and the seal the camera follows (SOCOM 1's
 * ftsGetPlayer, 0x2B3580: no row yet) */
extern "C" void StopShake__10CAppCameraFv(CAppCamera *camera);
extern "C" CZSealBody *func_002B3580();

/* the HUD's own, before this unit's part of the range (no rows yet): On and ZoomOut */
extern "C" void func_001F7A90(CHUD *hud);
extern "C" void func_001F7560(CHUD *hud);

/* a server setting (SOCOM 1's CValve): its value a halfword at 4 */
class CValve {
public:
    static CValve *GetByName(const char *name);

    unsigned char _0[4];
    unsigned short m_value;   // 0x4
};

/* a queue of messages on screen (SOCOM 1's C2DMessage_Q): only its font placed */
class C2DMessage_Q {
public:
    unsigned char _0[0x18];
    C2DFont *m_font;   // 0x18
};

/* the mission: its three message queues, a block the HUD re-inits and the multiplayer flag */
class CMission {
public:
    unsigned char _0[0x1c0];
    C2DMessage_Q m_messages;   // 0x1c0
    unsigned char _1dc[0x6a0 - 0x1dc];
    C2DMessage_Q m_messages2;   // 0x6a0
    unsigned char _6bc[0xb80 - 0x6bc];
    C2DMessage_Q m_messages3;   // 0xb80
    unsigned char _b9c[0x1060 - 0xb9c];
    unsigned char m_1060[0x1808 - 0x1060];   // 0x1060
    void *m_1808;   // 0x1808
};

extern CMission theMission;

class CameraZoom {
public:
    static void Reset();
};

extern "C" int sprintf(char *buf, const char *fmt, ...);

/* not named yet: SOCOM II's own re-inits and resets */
extern "C" void func_003C6980();
extern "C" void func_0022D190();
extern "C" void func_00202370(CZPauseTest *pause);
extern "C" void func_00238710(CMPInfoBox *box);
extern "C" void func_00242B20(CSpectatorInfo *info);
extern "C" void func_002CA0B0(void *block);
extern "C" void func_002B6530(C2DMessage_Q *q, const char *text, int a, int b, int c, float time);
extern "C" void func_00213610(BitmapReticule *reticule);
extern "C" void func_002371A0(CHudNetState *state);
extern "C" void func_0021A510(CWeaponSel *sel, int flag);
extern "C" void func_001FF370(CLensFlareManager *flares);

/* the HUD made ready for the next mission: on, the letterbox and the piggyback strings put away,
 * zoomed out, the team names (single player) or the network boxes set up, the message queues
 * given the HUD's font, a network round announced, the map re-inited on the player */
void CHUD::ReInit_Init()
{
    if (appCamera)
        StopShake__10CAppCameraFv(appCamera);
    func_003C6980();
    func_001F7A90(theHUD);
    theHUD->m_letterbox.RemoveLetterbox();
    for (unsigned int i = 0; i < 6; i++) {
        m_piggyback_text[i].Off();
        m_piggyback_text[i].SetTrans(100.0f);
    }
    m_socom2_1a0e8 = -1.0f;
    m_socom2_1a0f0 = -1;
    func_001F7560(this);
    if (!theNetwork.m_bNetwork)
        m_teamnames.Init();
    else
        m_teamnames.Off();
    func_00202370(&m_pausetest);
    m_poseBitmap.ReInit();
    func_00238710(&m_infobox);
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
    func_002CA0B0(theMission.m_1060);
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
        m_newmap->m_viewer = func_002B3580();
        m_newmap->ReInit();
    }
    func_0022D190();
    func_00213610(&m_bitmapReticule);
    m_ammodisplay.Reset();
    CameraZoom::Reset();
    func_002371A0(&data_00413120);
    func_0021A510(&m_weaponsel, 1);
    func_001FF370(&m_lensflares);
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

bool CHUD::IsOn()
{
    return m_ison;
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
