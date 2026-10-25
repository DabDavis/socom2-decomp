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
 * DrawPhase, ReleaseHUD, HudText_to_upper, GetHUDOwner and the constructor. Not yet: Tick
 * (0x1158), Init (0xF98; the font's constructor inlines one level, its pieces 0x1FB370-0x1FB3F0
 * out of line), the destructor (0xA40: it inlines every member's destructor, and needs the
 * texture lists as classes -- see CTexList), and the four inline copies at the unit's head
 * (0x1F6A90-0x1F6B00: C2DString helpers the reticle's unit calls; their caller here must be
 * dead-stripped). 0x1F6A80 is CLoadMenu's (st_load.cpp's vtable slot).
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

/* the seal's kit (SOCOM 1's CZKit, at 0x520 there): its flag word, one bit past SOCOM 1's last
 * (m_heartbeat_enabled was bit 14) */
class CZKit {
public:
    unsigned int _b0 : 16;
    unsigned int m_socom2_b16 : 1;   // bit 16: cleared when the HUD goes off
    unsigned int _b17 : 15;
};

/* the seal as the HUD reads it: its controller (CEntity's, at 0xC0) and its kit */
class CZSealBody {
public:
    unsigned char _0[0xc0];
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

/* the kit's own functions the HUD calls (OVL2; no rows named yet, so called as C) */
extern "C" void func_005BE570(CZKit *kit, bool flag);   /* SOCOM 1's SetCurReticule, a flag added */
extern "C" CZFTSWeapon *func_005C3780(CZKit *kit);     /* SOCOM 1's GetCurItem */
extern "C" bool func_005B9990(CZKit *kit);             /* SOCOM 1's NVG_Enabled */
extern "C" bool func_005C80F0(CZKit *kit);             /* SOCOM II's: tested with NVG_Enabled */
extern "C" float func_005BE660(CZKit *kit);            /* SOCOM 1's GetCurZoom */

/* the HUD's elements fade in from this time (SOCOM 1's g_fElemsFadeInTime, hud_main.cpp's own;
 * other units read it in SOCOM II) */
extern float g_fElemsFadeInTime;

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

/* the font's glyph map (SOCOM II's: a std::map, 0x10 bytes -- its tree's constructor 0x1FB3F0
 * and recursive destroy 0x1FE760 are out of line; the destructor tests the root at 4) */
extern "C" void func_001FE760(void *tree, void *root);

class C2DFontTree {
public:
    ~C2DFontTree()
    {
        if (m_root)
            func_001FE760(this, m_root);
    }

    unsigned int m_size;   // 0x0
    void *m_root;   // 0x4
    unsigned char m_comp;   // 0x8
    void *m_header;   // 0xc
};

class C2DFontMap {
public:
    C2DFontTree m_tree;   // 0x0
};

/* the HUD's font as its delete sees it (SOCOM II's C2DFont: a glyph map, a pointer vector, the
 * name). The destructor's body is one call (0x35A650, the font's unload; no row named yet), then
 * the members in reverse; the whole of it is four high, so ReleaseHUD's delete inlines it all */
extern "C" void func_0035A650(C2DFont *font);

class C2DFont {
public:
    ~C2DFont() { func_0035A650(this); }

    C2DFontMap m_glyphs;   // 0x0
    std::vector<void *> m_list;   // 0x10
    unsigned char _1c[0x58 - 0x1c];
    CSaferStr m_name;   // 0x58
};

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
/* the 2D classes as SOCOM 1's DWARF derives them (C2D's vtable at 0xC: CHUD's Off and On call
 * slots 4 and 5 there). Their destructors are declared so that this unit emits none: their
 * members are not placed (the linked copies, 0x1FD8C0-0x1FDA10 and the others, are implicit) */
class CWeaponSel : public C2D {
public:
    CWeaponSel();
    virtual ~CWeaponSel();
    void UnInit();
    unsigned char _18[0xce0 - 0x18];
} __attribute__((aligned(16)));

class CInGameWeaponSel : public C2D {
public:
    CInGameWeaponSel();
    virtual ~CInGameWeaponSel();
    void UnInit();
    unsigned char _18[0x11b0 - 0x18];
} __attribute__((aligned(16)));

/* the network game's weapon selector: slot lists honouring the server's weapon valves
 * (Enable_203FRAG and the rest), UnInit only in a network game (SOCOM II's; no vtable, so no
 * RTTI name: the name is ours). Off and On are its own, not C2D's: they turn its lists and
 * bitmaps off and on (0x23E700, 0x23E800) */
class CMPWeaponSel {
public:
    CMPWeaponSel();
    void UnInit();
    void Off();
    void On();
    unsigned char _0[0x1540];
} __attribute__((aligned(16)));

class PoseBitmap : public C2DBitmap {
public:
    PoseBitmap();
    virtual ~PoseBitmap();
    void Clear();
    void ReInit();
    unsigned char _80[0x150 - 0x80];
} __attribute__((aligned(16)));

/* the reticle's bitmaps and strings (0x47B0 bytes); 0x215180 turns on its bitmap at 0x34 (the
 * paused HUD's Off keeps it) */
class BitmapReticule : public C2D {
public:
    BitmapReticule();
    virtual ~BitmapReticule();
    void UnInit();
    unsigned char _18[0x47b0 - 0x18];
} __attribute__((aligned(16)));

extern "C" void func_00215180(BitmapReticule *reticule);

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
    ZOOMIN = 0,
    ZOOMOUT = 1,
};

/* the app camera (the shake's stop only) and the seal the player is (SOCOM 1's ftsGetPlayer) */
extern "C" void StopShake__10CAppCameraFv(CAppCamera *camera);
CZSealBody *ftsGetPlayer();

/* a server setting (SOCOM 1's CValve): its value a halfword at 4 */
class CValve {
public:
    static CValve *GetByName(const char *name);

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

/* the mission: its four message queues (the HUD re-inits the fourth) and the online game */
class CMission {
public:
    unsigned char _0[0xe0];
    float m_socom2_e0;   // 0xe0: the mission's clock (the respawn wait is measured on it)
    unsigned char _e4[0x1c0 - 0xe4];
    C2DMessage_Q m_messages;   // 0x1c0
    C2DMessage_Q m_messages2;   // 0x6a0
    C2DMessage_Q m_messages3;   // 0xb80
    C2DMessage_Q m_messages4;   // 0x1060
    unsigned char _1540[0x1808 - 0x1540];
    void *m_1808;   // 0x1808
};

extern CMission theMission;
/* the mission's round time, if any (0x2A6AD0: positive while the network game has a timer; no
 * row named yet) */
extern "C" int func_002A6AD0(CMission *mission);
/* SOCOM II's spectator test, or so (0x2C2FA0: CAmmoDisplay::On asks it too; no row yet) */
extern "C" bool func_002C2FA0();

/* the jukebox's, or so: a string table registered by name (no rows named yet) */
class CStringTables;
extern "C" CStringTables *func_00361BC0();
extern "C" int func_003A4460();
extern "C" void func_003A3900(CStringTables *tables, const char *name, int lang);

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
extern "C" void func_002CA0B0(C2DMessage_Q *q);
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
    func_002CA0B0(&theMission.m_messages4);
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

/* where the piggyback lines stand (variables: read with lwc1) */
static float s_piggyback_x = 324.0f;
static float s_piggyback_y = 380.0f;
static float s_piggyback_spacing = 18.0f;

/* a C2DString's own (0x362B70, ztwod's; no row named yet) */
extern "C" void func_00362B70(C2DString *s);

/* an entity as the spectator's line reads it: its type (2 a seal) and its name (SOCOM II's
 * layout; the static list is SOCOM 1's CEntity::m_list) */
class CEntity {
public:
    static unsigned char m_list[0xc];

    unsigned char _0[0x10];
    unsigned char m_type;   // 0x10
    unsigned char _11[0x14 - 0x11];
    const char *m_name;   // 0x14
};

/* the entity list's find by id (0x29E4E0), the string table's text by id (0x361B40) and the
 * screen's width (OVL1 bss 0x4A44C4); no rows named yet */
extern "C" CEntity *func_0029E4E0(unsigned char *list, int id);
extern "C" const char *func_00361B40(unsigned int id);
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
        m_piggyback_text[i].m_socom2_69 = 1;
        m_piggyback_text[i].SetPos((int)s_piggyback_x,
                                   (int)(s_piggyback_y + i * s_piggyback_spacing));
        func_00362B70(&m_piggyback_text[i]);
        if (menustate != statePauseTestMenu)
            m_piggyback_text[i].On();
        else
            m_piggyback_text[i].Off();
    }
}

/* the seal's node as the respawn text reads it (SOCOM 1's CEntity::GetNode, 0x29ED20; a float
 * at 0x9C zero once the seal lies dead) and the pad's type (0x2C6430: 11 for one layout of the
 * buttons; no rows named yet) */
class CRespawnNode {
public:
    unsigned char _0[0x9c];
    float m_9c;   // 0x9c
};

extern "C" CRespawnNode *func_0029ED20(CZSealBody *seal);
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
            CRespawnNode *node = player ? func_0029ED20(player) : 0;
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
            CRespawnNode *node = player ? func_0029ED20(player) : 0;
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
        m_piggyback_text[i].m_socom2_69 = 1;
        m_piggyback_text[i].SetPos((int)s_piggyback_x,
                                   (int)(s_piggyback_y + i * s_piggyback_spacing));
        func_00362B70(&m_piggyback_text[i]);
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
    CEntity *entity = func_0029E4E0(CEntity::m_list, ftsGetPlayer()->m_socom2_fc4);

    if (entity) {
        CEntity *seal = entity && entity->m_type == 2 ? entity : 0;
        const char *text = func_00361B40(0xec4f);
        char name[256];

        m_piggyback_text[0].SetString(text, false);
        m_piggyback_text[0].m_socom2_69 = 1;
        m_piggyback_text[0].SetPos((int)(0.5f * data_004A44C4), 25);
        m_piggyback_text[0].On();
        strcpy(name, seal->m_name ? seal->m_name : "???");
        unsigned int len = strlen(name);

        for (unsigned int i = 0; i < len; i++)
            name[i] = toupper(name[i]);
        m_piggyback_text[1].SetString(name, true);
        m_piggyback_text[1].m_socom2_69 = 1;
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
        m_piggyback_text[i].m_socom2_69 = 1;
        m_piggyback_text[i].SetPos((int)s_piggyback_x,
                                   (int)(s_piggyback_y + i * s_piggyback_spacing));
        func_00362B70(&m_piggyback_text[i]);
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
        if (func_002A6AD0(&theMission) > 0) {
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
        func_00215180(&m_bitmapReticule);
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

/* the ammo counts' piece the zoom puts back (0x237760; no row named yet) */
extern "C" void func_00237760(CAmmoDisplay *ammo);

/* zoomed out (the full view): unless the map is in full screen, the compass and the weapon back,
 * the kit's reticle reset, the counts on unless the night vision or 0x5C80F0 holds, the message
 * queues' backgrounds grey again, the team names and map (single player), the zoom text */
void CHUD::ZoomOut()
{
    CZSealBody *owner = appCamera->m_bc;
    CZKit *kit = &owner->m_kit;

    func_005C3780(kit);
    if (!theNetwork.m_bNetwork && m_newmap->m_isFullScreen)
        return;
    m_hudmode = ZOOMOUT;
    if (!m_ison)
        return;
    m_mapcompass.On();
    weapon.On();
    if (!owner)
        return;
    func_005BE570(kit, false);
    if (!func_005B9990(kit) && !func_005C80F0(kit))
        m_ammodisplay.On();
    func_00237760(&m_ammodisplay);
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
    func_005BE570(&owner->m_kit, false);
    CZKit *kit = &owner->m_kit;
    CZFTSWeapon *pItem = func_005C3780(kit);

    if (pItem && pItem->m_type == 11)
        m_ammodisplay.Off();
    /* the target's block order: the night-vision path's Off first, jumping over the other's
     * (an if/else with an Off in each arm puts the then-arm's whole; a goto reproduces it) */
    if (!func_005B9990(kit)) {
        func_005C80F0(kit);
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
        zoom = func_005BE660(kit);
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
    int time = func_002A6AD0(&theMission);
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

float g_fElemsFadeInTime = -1.0f;
