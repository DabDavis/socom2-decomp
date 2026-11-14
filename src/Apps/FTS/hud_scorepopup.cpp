/* hud_scorepopup.cpp -- the network game's score pop-up (OVL1 0x229CA0-~0x22EA2F: CHudPopups'
 * destructor is 0x22D600 and CScoreBmp's, the unit's tail, 0x22E9C0 -- their RTTI names say so;
 * between 0x22D170 and 0x22E9C0 sit string, vector and pop-up helpers not yet sorted): SOCOM 1's unit (its
 * DWARF names it, Z:\dev\Apps\FTS\hud_scorepopup.cpp), grown in SOCOM II. SOCOM 1's order holds at
 * both ends: CScorePopup's destructor (0x229CA0) and Draw (slot 1 of its vtable 0x405990, 0x22A8B0)
 * open it as there, after CTeamDisplay::SetupTeam (0x229D00) and AddRow (0x22A290); then come
 * SOCOM II's pieces -- CHudPopups::Draw (0x22AB80), 0x22AC30, SetupBackgroundBox (0x22AE90: ten
 * bitmaps loaded and UV'd, as SOCOM 1's), 0x22B360, the pop-up opener (0x22B4A0) and right after
 * it, as its first user, a class vector's destructor copy (0x22BAD0: two this tests, then the
 * deleter's destructor 0x22E460 with 0) -- then hudTurnOffScorePopup (0x22BB30), the two pop-up
 * triggers (0x22BC40, 0x22BD50), the tick (0x22BE20, SOCOM 1's hudTickScorePopup by its place),
 * hudIsScorePopupOn (0x22CBF0) and hudTurnOnScorePopup (0x22CC10, round 65). Written, all byte-exact: CScorePopup's destructor
 * and Draw, CHudPopups' Draw and destructor, hudTurnOffScorePopup, the triggers and
 * hudIsScorePopupOn; and (round 33) the message pop-ups' third trigger (0x22D270, by names), their
 * reset (0x22D190) and hud.rdr's common look (0x22D3F0). The unit's statics are anonymous-namespace globals in SOCOM 1 (g_bOn,
 * g_oSeals2Ds, g_oTerrs2Ds, the box's ten CScoreBmps); its initialiser (0x3FE980, 0x9D0 bytes)
 * builds every one, byte-exact with the objects defined in their address order (round 32; the
 * flags and the frame's ints, which it does not build, still by address). Defined in reverse
 * of their image order (-inline deferred). */

#include "hud_weaponsel.h"
#include "../../gamez/zinput/zinput.h"
#include "../../gamez/zrdr/zrdr.h"

#include <string.h>
#include <string>
#include <vector>

/* the HUD's backdrop alpha (hud_main.cpp's layout float, 76.8) and two layout floats the corners'
 * texture coordinates come from (173 and 74) */
extern float data_003E3000;
extern float data_003E3018;
extern float data_003E2FF0;

class CZSealBody;
/* the online game (SOCOM 1's zfts.h: only the two wins; their addresses are the pop-up's column's, so
 * they are emitted here, 0x22D170 and 0x22D180, as in SOCOM 1) */
class CZNetGame {
public:
    int GetE3ScoreTeam00() const { return m_e3score00; }
    int GetE3ScoreTeam08() const { return m_e3score08; }

    unsigned char _0[0x111];
    signed char m_gametype;   // 0x111: the game's type, 1 to 5 (the pop-up's details)
    unsigned char _112[0x120 - 0x112];
    int m_e3score00;   // 0x120
    int m_e3score08;   // 0x124
};
/* the game (theMission's, no data row yet: the wins come from it) */
class CMission;
extern CMission theMission;
#define data_00437CE8 (*(CZNetGame **)((char *)&::theMission + 0x1808))   /* theMission+0x1808 */
int FormatString(char *buf, int size, int id, const char **args, int count);
const char *GetString(int id);

namespace {

/* a bitmap of the box (SOCOM 1's CScoreBmp: C2DBitmap with its own alpha set at construction --
 * the HUD's backdrop alpha, through the vtable as the initialiser calls it; its vtable 0x405A70
 * overrides nothing but the destructor, so MW 3.0 calls even a global one's Draw through the
 * table) */
class CScoreBmp : public C2DBitmap {
public:
    CScoreBmp() { SetTrans(data_003E3000); }
};

/* a team's column (SOCOM 1's CTeamDisplay, 0xF60 there with 27 rows; SOCOM II's 0x1B50: its title
 * bitmap, 45 rows, and 0x18 bytes of its own). The initialiser builds the title with C2DBitmap()
 * and the rows with C2DString's constructor through __construct_array, and the implicit destructor
 * 0x22E920 destroys them so: a plain bitmap and plain strings */
class CTeamDisplay {
public:
    /* the column's title, its three headers, its stripes and its rows (SOCOM 1's SetupTeam, 0x421140
     * there: the team's name by string id now, its wins through the game's member function) */
    void SetupTeam(float fTopY, float fBgR, float fBgG, float fBgB, int iTeam, int (CZNetGame::*pWinsFunc)() const,
                   bool bTeam0);
    /* one row: the name, its clan's tag and the three scores (SOCOM 1's AddRow, 0x421510 there; the
     * kills gone from its arguments) */
    void AddRow(CZSealBody *const *arrSeals, int iRow, float fTopY);

    C2DBitmap _bmpTitleBg;   // 0x0
    C2DString _arrStrs[45];   // 0x80
    unsigned char _1b38[0x1b50 - 0x1b38];
};

/* the pop-up's 2D object (SOCOM 1's: it overrides only Draw; vtable 0x405990, RTTI
 * "@unnamed@hud_scorepopup_cpp@::CScorePopup") */
class CScorePopup : public C2D {
public:
    virtual void Draw(zdb::CCamera *cam);
};

/* the message pop-ups' 2D object (SOCOM II's, its RTTI "@unnamed@hud_scorepopup_cpp@::CHudPopups";
 * vtable 0x405A08: its destructor 0x22D600, Draw 0x22AB80) */
class CHudPopups : public C2D {
public:
    virtual void Draw(zdb::CCamera *cam);
};

}

class CUIVariable;

/* a UI variable's hook (zui_uivar.h's, SOCOM 1's DWARF; the vtable pointer first) */
class UIVarMixin {
public:
    UIVarMixin() { m_uivar = 0; }
    virtual ~UIVarMixin() { DetachUIVar(0); }
    void DetachUIVar(CUIVariable *var);
    virtual void UpdateUIVar(CUIVariable *var);

    CUIVariable *m_uivar;   // 0x4
};   // 8 bytes

/* a plain bitmap of the UI's (zui_2d.h's, as hud_newhudmap.cpp declares it): the message pop-up's
 * two bitmaps. Its constructor is inline (the initialiser inlines it), its camera Draw the
 * bitmap's; the first non-inline virtual keeps the table out of this unit, the implicit destructor
 * 0x1FDBE0 is registered */
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
    virtual void Draw(zdb::CCamera *cam) { C2DBitmap::Draw(cam); }
    virtual void Draw(const CMatrix &m, zdb::CTextureRelocMgr *mgr);
    virtual void Tick(float dT);
    virtual void SetMapOffset(int x, int y);

    bool m_pulsing;   // 0x88
    float m_period;   // 0x8c
    float m_curtrans;   // 0x90
} __attribute__((aligned(16)));   // 0xa0 bytes

/* a string that is never null (SOCOM 1's: MSL's basic_string and nothing more; its implicit
 * destructor's copy is main.cpp's, 0x1E7A50) */
class CSaferStr : public std::string {
public:
    CSaferStr() {}
    /* SOCOM 1's zutil.h: a null text empties it (0x22DCC0 is an out-of-line copy) */
    void operator=(const char *sz)
    {
        if (sz)
            assign(sz);
        else
            erase();
    }
};

class CHUD {
public:
    void On();

    static CZSealBody *GetHUDOwner();

    unsigned char _0[0x498];
    C2DFont *m_myfont;   // 0x498
    /* the HUD's font (ours: SetupTeam's local, numbered as a temporary at the inline's expansion) */
    C2DFont *GetFont() const { return m_myfont; }
};

extern CHUD *theHUD;
class CZNetwork {
public:
    unsigned char m_bSessionMaster;   // 0x0
    unsigned char m_bNetwork;   // 0x1
};

extern CZNetwork theNetwork;

const char *GetDatabase();

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

/* a point read from a tag (SOCOM 1's IPNT2D is two ints; each coordinate here a one-int struct -- a
 * STAND-IN, the real type unknown: hudAddPopupDefinition copies the window's position coordinate by
 * coordinate through $f0, each source address in a register, as MW's implicit assignment block-copies
 * class-typed members (two int members copy with lw/sw), and reads WindowSize's x with one plain lw,
 * not through a conversion inline -- which would keep the temporary's address in a register; SOCOM 1's
 * CInit<int, 0> zeroes and costs GetTagIPNT2D and CPopupMessage()) */
struct IPNT2D {
    struct coord {
        int v;
    };

    IPNT2D() {}
    IPNT2D(int ax, int ay) { x.v = ax; y.v = ay; }

    coord x, y;
};

/* the unit's flags, by address (bss, no constructors): the message pop-up is up (0x40E480; CHudPopups
 * draws behind it), and SOCOM 1's g_bOn, the score pop-up is up (0x40F608) */
extern bool data_0040F608;
extern bool data_0040E480;

struct CMessageTrigger;
struct CMessageTriggerByName;
struct CMessageTriggerByNames;
struct CMessageTriggerByMap;

/* the unit's objects, OVL1 bss 0x40DBF0-0x412E73, in the order its initialiser (0x3FE980) builds
 * them, each after its destructor record -- SOCOM 1's anonymous namespace (its names: the box's nine
 * pieces g_bmpUlCorner ... g_bmpCenter, g_bmpTeamDivider, g_oSeals2Ds, g_oTerrs2Ds; SOCOM II keeps
 * their order); the rest SOCOM II's, the names ours: the bitmap drawn last and the sixteen pieces
 * SetupSpectatorBox loads, two bitmaps and a string drawn after the divider, the spectators' title
 * bar, title, box and eight names, the message pop-up's two plain bitmaps, three strings, three
 * CSaferStrs the pop-up's tick and opener read, the message pop-up's items, the four trigger lists
 * and the spectators' box colour; then the corners' texture coordinates (OVL1 .data, set here) */
namespace {
C2DBitmap g_bmpBoxOverlay;
C2DBitmap g_arrBmpSpecPieces[16];
CScoreBmp g_bmpUlCorner;
CScoreBmp g_bmpUrCorner;
CScoreBmp g_bmpLlCorner;
CScoreBmp g_bmpLrCorner;
CScoreBmp g_bmpTopEdge;
CScoreBmp g_bmpLeftEdge;
CScoreBmp g_bmpRightEdge;
CScoreBmp g_bmpBottomEdge;
CScoreBmp g_bmpCenter;
C2DBitmap g_bmpHeaderBar;
C2DString g_strHeader;
CScoreBmp g_bmpHeaderBg;
CScoreBmp g_bmpSpecTitleBg;
C2DString g_strSpecTitle;
CScoreBmp g_bmpSpecBox;
C2DString g_arrStrSpecNames[8];
CPlainBmp g_bmpPopupBg;
CPlainBmp g_bmpPopupFrame;
CScoreBmp g_bmpTeamDivider;
C2DString g_strTeamLabel0;
C2DString g_strTeamLabel1;
C2DString g_strTeamLabel2;
CTeamDisplay g_oSeals2Ds;
CTeamDisplay g_oTerrs2Ds;
CSaferStr g_strPopup0;
CSaferStr g_strPopup1;
CSaferStr g_strPopup2;
std::vector<C2D *> g_vecPopupItems;
std::vector<CMessageTriggerByNames *> g_vecTriggersByNames;
std::vector<CMessageTriggerByName *> g_vecTriggersByName;
std::vector<CMessageTrigger *> g_vecTriggers;
std::vector<CMessageTriggerByMap *> g_vecTriggersByMap;
}

/* the corners' texture coordinates (OVL1 .data 0x3E5718 and 0x3E5720, zero in the image), computed
 * after the vectors and before the box colour: their two loads are scheduled above the colour's
 * three stores, as the target has them. Globals, not the anonymous namespace (the rows' names) */
float s_fCornerU = 1.0f - 20.0f / data_003E3018;
float s_fCornerV = 20.0f / data_003E2FF0;

namespace {
CPnt3D g_vSpecBoxColor(64.0f, 128.0f, 64.0f);
}

/* the window a message pop-up opens and its text (SOCOM II's; the names ours): hud.rdr's
 * WindowPos, WindowSize's width, TextSize (scaled by 1.1429), LineInterval and TextId -- what the
 * opener (0x22B4A0) takes; every trigger ends with one. Its constructor 0x22DD90 is out of line
 * (the loader's copy: the triggers' implicit constructors inline one level) */
struct CPopupMessage {
    CPopupMessage() : m_pos(0, 0), m_width(0), m_textSize(0.0f), m_lineInterval(0.0f) {}

    IPNT2D m_pos;   // 0x0
    int m_width;   // 0x8
    float m_textSize;   // 0xc
    float m_lineInterval;   // 0x10
    CSaferStr m_textId;   // 0x14

    void Read(_zrdr *popup);
};   // 0x20 bytes

/* each trigger's condition, a base with its own constructor (the done flag cleared): the loader's
 * `new` calls it out of line, then the message's (0x22DC40, 0x22DC70, 0x22DCA0, 0x22DDC0; the
 * names ours). A HUD objective's: its valve's name and the mission, read by an inline of its own
 * (ours: the loader reloads the pushed trigger from its stack slot once, as this inline's `this`,
 * where the strings assigned through the trigger reload it after every call) */
struct CTriggerByValve {
    CTriggerByValve() : m_done(false) {}
    void Read(_zrdr *trigger)
    {
        m_name = zrdr_findstring(trigger, "Valve");
        m_map = zrdr_findstring(trigger, "Mission");
    }

    bool m_done;   // 0x0
    CSaferStr m_name;   // 0x4: Valve
    CSaferStr m_map;   // 0x10: Mission
};

/* a message shown once (SOCOM II's; the names ours): a done flag, the map it is for and the object
 * or event that shows it, then what the opener (0x22B4A0) takes (hud.rdr's HudObjective) */
struct CMessageTrigger : public CTriggerByValve {
    CPopupMessage m_message;   // 0x1c
};

/* an animation command's (AnimCmd): the name, then the flag */
struct CTriggerByName {
    CTriggerByName() : m_done(false) {}

    CSaferStr m_name;   // 0x0: Name
    bool m_done;   // 0xc
};

/* a message keyed by name alone (0x22BD50's records: the name, the done flag, the message) */
struct CMessageTriggerByName : public CTriggerByName {
    CPopupMessage m_message;   // 0x10
};


/* the message pop-up opened (0x22B4A0: true once shown; defined below) */
bool hudOpenPopup(CPopupMessage *message);

/* what a trigger is given: an object with its name at 0x20 (no name yet) */
/* a message keyed by the map and any of several objects' names (0x22D270's records: the done flag,
 * the names, the map, the message) and one keyed by the map alone (0x22D190 clears its flag) */
struct CTriggerByNames {
    CTriggerByNames() : m_done(false) {}
    /* a name appended, through the new last element (the loader's; inlined one level: push_back,
     * the temporary's constructor and destructor, rbegin(), the reverse * and CSaferStr's
     * assignment called out of line, as the target does; the name ours) */
    void AddName(const char *s)
    {
        m_names.push_back(CSaferStr());
        *m_names.rbegin() = s;
    }

    bool m_done;   // 0x0
    std::vector<CSaferStr> m_names;   // 0x4: Icons
    CSaferStr m_map;   // 0x10: Mission
};

struct CMessageTriggerByNames : public CTriggerByNames {
    CPopupMessage m_message;   // 0x1c
};

/* an object first under the reticule (ObjUnderRet1stTime): its name and the mission */
struct CTriggerByObject {
    CTriggerByObject() : m_done(false) {}
    void Read(_zrdr *trigger)
    {
        m_name = zrdr_findstring(trigger, "ObjName");
        m_map = zrdr_findstring(trigger, "Mission");
    }

    bool m_done;   // 0x0
    CSaferStr m_name;   // 0x4: ObjName
    CSaferStr m_map;   // 0x10: Mission
};

struct CMessageTriggerByMap : public CTriggerByObject {
    CPopupMessage m_message;   // 0x1c
};

/* what the action icons hand the by-names trigger: an object with its name first (ours) */
struct CNamedObject {
    const char *m_name;   // 0x0
};

struct CTriggerSource {
    unsigned char _0[0x20];
    const char *m_name;   // 0x20
    /* inline: the compare loads it before the record name's c_str() -- two inline calls in one
     * argument list expand right to left (the name ours) */
    const char *GetName() { return m_name; }
};

/* a SEAL as the pop-up reads it: alive in the world (0x14), the two kill counts summed for its row,
 * and a flag that keeps it off the pop-up (the names ours) */
class CSealCtrl;

/* a round's and a game's stats (zseal.h's CSealStats, 0x48 bytes: only what the rows read) */
class CSealStats {
public:
    unsigned char _0[0xc];
    unsigned short m_c;   // 0xc
    unsigned char _e[0x12 - 0xe];
    unsigned short m_12;   // 0x12
    unsigned char _14[0x3c - 0x14];
    short m_kills;   // 0x3c
    unsigned char _3e[0x48 - 0x3e];
};

/* the seal's kit: whether it holds a charged item (kit.h's) */
class CZKit {
public:
    bool HasItemWithAmmo(EQUIP_ITEM item);
};

/* what is under its reticule (a node: its name) */
struct CObjUnderReticule {
    /* inline: the compare loads it before the record name's c_str(), as CTriggerSource's */
    const char *GetName() { return m_name; }

    unsigned char _0[0x90];
    const char *m_name;   // 0x90
};

class CZSealBody {
public:
    unsigned char _0[0x14];
    const char *m_name;   // 0x14: set while it is alive in the world
    unsigned char _18[0xc0 - 0x18];
    CSealCtrl *m_ctrl;   // 0xc0
    unsigned char _c4[0xe1 - 0xc4];
    unsigned char _e1_0 : 4;
    unsigned char m_socom2_e1_4 : 1;   // 0xe1 bit 4
    unsigned char _e1_5 : 3;
    unsigned char _e2[Z_SEALBODY_OFS(0x3dc) - 0xe2];
    CObjUnderReticule *m_reticuleObject;   // 0x3dc
    unsigned char _3e0[Z_SEALBODY_OFS(0x544) - Z_SEALBODY_OFS(0x3e0)];
    CSealStats m_stats[2];   // 0x544
    unsigned char _5d4[Z_SEALBODY_OFS(0x5e0) - Z_SEALBODY_OFS(0x5d4)];
    CZKit m_kit;   // 0x5e0
    unsigned char _5e1[Z_SEALBODY_OFS(0xfc8) - Z_SEALBODY_OFS(0x5e1)];
    int m_persona;   // 0xfc8
    unsigned char _fcc[Z_SEALBODY_OFS(0xfd1) - Z_SEALBODY_OFS(0xfcc)];
    unsigned char m_hidden;   // 0xfd1
};

/* its controller's menu state (zseal.h's, a byte) */
enum MENU_STATE { MENU_STATE_NONE = 0, MENU_STATE_POPUP = 3, MENU_STATE_ANY = 0x7f };

/* its controller: whether a player drives it (slot +0x2c: zseal.h's) */
class CSealCtrl {
public:
    void SetMenuState(MENU_STATE state, bool now);
    void StopRumbler();
    virtual ~CSealCtrl();
    virtual bool PreTick(float dT);
    virtual bool Tick(float dT);
    virtual bool PostMortemPreTick(float dT);
    virtual void OnDeath();
    virtual void Slot1C();
    virtual void Slot20();
    virtual void PlayAiSound();
    virtual void AllowCommand();
    virtual bool IsPlayer() const;   /* +0x2c */

    unsigned char _4[0x221 - 4];
    signed char m_menustate;   // 0x221
};

class CZPersonaState {
public:
    static const char *GetPersonaClanTag(int client);
};

/* a thing in the world (st_exit.cpp's stub: only the list here) */
class CEntity {
public:
    static std::list<CEntity *> m_list;

    unsigned char _0[0x10];
    unsigned char m_type;   // 0x10: 2 a SEAL
};

/* a walk over the world's entities, counting (SOCOM 1's DWARF; st_exit.cpp writes its members) */
class CEntityItr {
public:
    CEntityItr() : _it(GetEntityList().begin()), _iIdx(0) {}
    /* whether the walk has entities left (ours: the end test through an inline, which alone gives
     * the begin() temporaries the higher stack slots, as the target has them) */
    bool More() const { return _it != GetEntityList().end(); }

    CZSealBody &GetSeal() const;
    bool IsSeal() const;
    void Advance();

    static const std::list<CEntity *> &GetEntityList() { return CEntity::m_list; }

    std::list<CEntity *>::const_iterator _it;   // 0x0
    int _iIdx;   // 0x4
};

/* whether a SEAL is dead, and on each side (the SEAL lane's, no names yet) */
extern "C" {
bool func_002C2FD0(CZSealBody *seal);
bool func_002C31D0(const CZSealBody *seal);
bool func_002C30B0(const CZSealBody *seal);
}

namespace {
/* a side's SEALs, at most eight, with their kills: in a network game only the living, and none the
 * world has let go of or that is kept off the pop-up (SOCOM 1's) */
int GetTeamMembers(CZSealBody **arrSeals, int *arrKills, bool bTeam0)
{
    int iNumSeals = 0;

    for (CEntityItr it; it.More(); it.Advance()) {
        if (!it.IsSeal())
            continue;
        CZSealBody &seal = it.GetSeal();
        if (theNetwork.m_bNetwork && func_002C2FD0(&seal))
            continue;
        if (bTeam0 && !func_002C31D0(&seal))
            continue;
        if (!bTeam0 && !func_002C30B0(&seal))
            continue;
        if (!seal.m_name || seal.m_hidden)
            continue;
        arrSeals[iNumSeals] = &seal;
        arrKills[iNumSeals] = seal.m_stats[0].m_kills + seal.m_stats[1].m_kills;
        if (++iNumSeals >= 8)
            return iNumSeals;
    }
    return iNumSeals;
}
}

/* a tag's first two numbers as integers (below, 0x22B360) */
IPNT2D GetTagIPNT2D(const _zrdr *rdr, const char *tag);

/* the score pop-up's 2D object: one guarded static (0x412E20, its guard 0x412E00, its destructor
 * record 0x412E08), C2D's constructor and CScorePopup's vtable 0x405990, ~CScorePopup registered --
 * hud_main.cpp's CHUD::Init adds it (OVL1 0x22DDF0; the name not known) */
extern "C" C2D *func_0022DDF0()
{
    static CScorePopup s_popup;

    return &s_popup;
}

/* the message's window and text read (an inline of its own, ours: the erase temporaries of the
 * message's text id take the frame slot below the triggers' -- MW lays out an inline's inlines after
 * the caller's direct ones) */
inline void CPopupMessage::Read(_zrdr *popup)
{
    m_pos = GetTagIPNT2D(popup, "WindowPos");
    m_width = GetTagIPNT2D(popup, "WindowSize").x.v;
    float size = 1.0f;

    zrdr_findreal(popup, "TextSize", &size, 1);
    size *= 1.1429f;
    m_textSize = size;
    float spacing = 20.0f;

    zrdr_findreal(popup, "LineInterval", &spacing, 1);
    m_lineInterval = spacing;
    m_textId = zrdr_findstring(popup, "TextId");
}

/* one of hud.rdr's PopupDefinitions read (SOCOM II's; hudInit calls it for each; the name ours, after
 * SOCOM 1's hudAddPolyType): the Trigger block's Type picks the trigger and its list -- an action
 * icon's first use (its Icons' names and the Mission), an animation command (by Name), an object
 * first under the reticule (ObjName, Mission), a HUD objective (Valve, Mission) -- each new'ed and
 * pushed first; then the message every trigger ends with: the window, the text size scaled by
 * 1.1429, the line spacing (20 unless given) and the text id. Each new name is stored through the
 * vector's last element, `*rbegin()` (rbegin() 0x22DD20 -- its return slot handed straight to the
 * reverse * 0x22DD10 as `this` -- out of line, then CSaferStr's assignment 0x22DCC0), every other
 * string through CSaferStr's inline assignment. Since the MSL headers' round 27 (the class push_back
 * through end()) the name append makes the target's six calls in order; the HUD's round 51 wrote the
 * rest from the target: an `int` counter against the unsigned Count() (`i * 8` recomputed each pass,
 * where `unsigned` strength-reduces it into a saved register), the names appended through a plain copy
 * of the pushed pointer and the object's and valve's strings read by inlines of their own (each loads
 * the pushed trigger from its stack slot once, as the target does; the by-name trigger's one string
 * reloads it at its use), the message read by an inline (its erase temporaries' slots below the
 * triggers'), and IPNT2D's coordinates copied one by one through $f0 (its stand-in coord struct).
 * OPEN, 0x600 against 0x5E0 (a level: the README's "Level 6, or 8?"): the target calls the three
 * other triggers' base constructors (0x22DCA0 by name, 0x22DC70 by object, 0x22DC40 by valve) and
 * the message's (0x22DD90) out of line from their implicit constructors inlined one level, which
 * needs those six high; with include/string's two-high constructor they are five and inline whole
 * (+8 instructions). CSaferStr one level taller (an intermediate base under it -- invented, not
 * adopted: ~CSaferStr's copy and the name vector's clear lose a match) makes the function byte-exact,
 * the three constructors' rows waiting for their names (__ct__14CTriggerByNameFv,
 * __ct__16CTriggerByObjectFv, __ct__15CTriggerByValveFv) */
void hudAddPopupDefinition(_zrdr *popup)
{
    CPopupMessage *msg = 0;
    _zrdr *trigger = zrdr_findtag(popup, "Trigger");
    const char *type = zrdr_findstring(trigger, "Type");

    if (type && !strcasecmp(type, "ActionIcon1stTime")) {
        CMessageTriggerByNames *t = new CMessageTriggerByNames;

        msg = &t->m_message;
        g_vecTriggersByNames.push_back(t);
        CTriggerByNames *names = t;
        _zrdr *icons = zrdr_findtag(trigger, "Icons");

        if (icons) {
            for (int i = 0; i < icons->Count(); i++)
                names->AddName(icons->Item(i)->String());
        }
        names->m_map = zrdr_findstring(trigger, "Mission");
    } else if (type && !strcasecmp(type, "AnimCmd")) {
        CMessageTriggerByName *t = new CMessageTriggerByName;

        msg = &t->m_message;
        g_vecTriggersByName.push_back(t);
        t->m_name = zrdr_findstring(trigger, "Name");
    } else if (type && !strcasecmp(type, "ObjUnderRet1stTime")) {
        CMessageTriggerByMap *t = new CMessageTriggerByMap;

        msg = &t->m_message;
        g_vecTriggersByMap.push_back(t);
        t->Read(trigger);
    } else if (type && !strcasecmp(type, "HudObjective")) {
        CMessageTrigger *t = new CMessageTrigger;

        msg = &t->m_message;
        g_vecTriggers.push_back(t);
        t->Read(trigger);
    }
    if (msg) {
        msg->Read(popup);
    }
}

/* the message pop-ups' 2D object: one guarded static (0x412E50, its guard 0x412E38, its destructor
 * record 0x412E40), CHudPopups' vtable 0x405A08, ~CHudPopups (0x22D600, emitted after it)
 * registered -- st_core.cpp's draw calls it (OVL1 0x22D590; the name not known) */
extern "C" C2D *func_0022D590()
{
    static CHudPopups s_popups;

    return &s_popups;
}

/* the message for this map and any of these objects' names shown, once -- never in a network game
 * (SOCOM II's; the action icons call it, 0x21F850; the name ours) */
void hudTriggerPopup(CNamedObject *source)
{
    if (theNetwork.m_bNetwork)
        return;
    if (!source)
        return;
    for (int i = 0; i < g_vecTriggersByNames.size(); i++) {
        CMessageTriggerByNames *t = g_vecTriggersByNames[i];
        bool here = !strcasecmp(t->m_map.c_str(), GetDatabase());

        if (here) {
            bool found = false;

            if (source) {
                for (int j = 0; j < t->m_names.size(); j++) {
                    const char *n = t->m_names[j].c_str();

                    if (n && !strcasecmp(n, source->m_name))
                        found = true;
                }
            }
            if (found) {
                if (t->m_done)
                    return;
                if (!hudOpenPopup(&t->m_message))
                    return;
                t->m_done = true;
                return;
            }
        }
    }
}

/* every message to be shown again (SOCOM II's: CHUD::Init and ReInit_Init call it; the name ours) */
void hudResetPopupTriggers()
{
    for (int i = 0; i < g_vecTriggersByName.size(); i++)
        g_vecTriggersByName[i]->m_done = false;
    for (int i = 0; i < g_vecTriggers.size(); i++)
        g_vecTriggers[i]->m_done = false;
    for (int i = 0; i < g_vecTriggersByMap.size(); i++)
        g_vecTriggersByMap[i]->m_done = false;
}

/* the message pop-ups' common look (OVL1 .data: its margins 10, 20 and 20 -- defined below, with the
 * pop-up's draw -- and the backdrop's opacity 1, 8 bytes apart, by address; the forced-open time in bss) */
extern float s_fPopupTextY;
extern float s_fPopupTextX;
extern float s_fPopupMargin;
extern float data_003DC8C0;
extern float data_0040F618;

/* hud.rdr's EventPopups/Common (SOCOM II's, hudInit's call; the name ours): the margins, the prompt,
 * the backdrop's opacity, the forced-open time and the opening and closing sounds */
void hudReadPopupCommon(_zrdr *rdr)
{
    zrdr_findreal(rdr, "TopMargin", &s_fPopupTextY, 1);
    zrdr_findreal(rdr, "LeftMargin", &s_fPopupTextX, 1);
    zrdr_findreal(rdr, "BottomMargin", &s_fPopupMargin, 1);
    g_strPopup2 = zrdr_findstring(rdr, "PromptText");
    zrdr_findreal(rdr, "BackgroundOpacity", &data_003DC8C0, 1);
    zrdr_findreal(rdr, "ForceOpenTime", &data_0040F618, 1);
    g_strPopup0 = zrdr_findstring(rdr, "OpeningSound");
    g_strPopup1 = zrdr_findstring(rdr, "ClosingSound");
}

/* the frame's layout floats (OVL1 .data 0x3DC908, 0x3DC910, 0x3DC918 -- 153, 477, 100 -- and
 * 0x3DC8C8, 430: this function's alone; by address, the names to come), the HUD text's frame the
 * pop-up's top comes from (0x436AA8, 0x436AE8: read here only) */
extern const char *data_003E3048;
extern float data_003DC908;
extern float data_003DC910;
extern float data_003DC918;
extern float data_003DC8C8;
#define data_00436AA8 (*(int *)((char *)&::theMission + 0x5C8))   /* theMission+0x5C8 */
#define data_00436AE8 (*(int *)((char *)&::theMission + 0x608))   /* theMission+0x608 */
namespace {
extern float s_fTeamTextTrans;   /* (defined below, with the column's look) */
}
extern int data_0040F628;
extern int data_0040F630;
extern int data_0040F638;
extern int data_0040F640;
extern int data_0040F648;
extern int data_0040F650;
extern int data_0040F658;

namespace {
void SetupBackgroundBox();
void SetupSpectatorBox();
/* a thing in the list as a SEAL (the entity test the walks use) */
static inline CZSealBody *AsSeal(CEntity *e)
{
    return e && e->m_type == 2 ? (CZSealBody *)e : 0;
}
}

/* the pop-up opened (SOCOM 1's hudTickScorePopup had it inline: the frame laid out, the box, the
 * spectators' box and both teams' columns set up; in a network game the living spectators' names
 * listed, shortened to fit, the rest cleared; the name ours, after hudTurnOffScorePopup) */
void hudTurnOnScorePopup()
{
    data_0040F608 = true;
    data_0040F640 = 10;
    data_0040F628 = (int)data_003DC908;
    data_0040F630 = (int)(data_0040F628 + data_003DC910);
    data_0040F638 = data_00436AA8 + data_00436AE8 + 5;
    data_0040F648 = data_0040F628 - 1;
    data_0040F650 = data_0040F638;
    data_0040F658 = (int)(25.0f + data_0040F650 + data_003DC918);
    g_bmpBoxOverlay.Load(0.0f, 0.0f, 26.0f, 26.0f, zdb::CAssetMgr::m_assets.GetTexHandle("hud_satchel.tif"));
    g_bmpBoxOverlay.Off();
    SetupBackgroundBox();

    zdb::CTexHandle *tex = zdb::CAssetMgr::m_assets.GetTexHandle(data_003E3048);
    C2DFont *font = theHUD->GetFont();

    g_bmpHeaderBar.Load(data_0040F640, data_0040F650, data_0040F648, 25.0f + data_0040F650, tex);
    g_bmpHeaderBar.SetColor(g_vSpecBoxColor);
    g_bmpHeaderBar.SetTrans(80.0f);
    {
        float y = 25.0f + data_0040F650;

        g_bmpHeaderBg.Load(data_0040F640, y, data_0040F648, y + data_003DC918, tex);
    }
    g_strHeader.Load(1.0f, "GAME DETAILS", font, 0.0f, 0.0f, false);
    g_strHeader.SetPos((int)(10.0f + data_0040F640), (int)(20.0f + data_0040F650));
    g_strHeader.m_color[3] = s_fTeamTextTrans;
    SetupSpectatorBox();
    g_bmpBoxOverlay.Off();
    g_oSeals2Ds.SetupTeam(data_0040F638, 32.0f, 32.0f, 64.0f, 0xEC69, &CZNetGame::GetE3ScoreTeam00, true);
    g_oTerrs2Ds.SetupTeam(0.5f * (data_0040F638 + data_003DC8C8), 64.0f, 32.0f, 32.0f, 0xEC6A,
                          &CZNetGame::GetE3ScoreTeam08, false);
    if (theNetwork.m_bNetwork) {
        const std::list<CEntity *> &list = CEntity::m_list;
        unsigned int count = 0;
        std::list<CEntity *>::const_iterator it = list.begin();

        for (; it != list.end() && count < 8; ++it) {
            CZSealBody *seal = AsSeal(*it);

            if (func_002C2FD0(seal)) {
                g_arrStrSpecNames[count].SetString(seal->m_name, true);

                int width = (int)((data_0040F648 - data_0040F640) - 24.0f);

                g_arrStrSpecNames[count].UpdatePos();
                if (g_arrStrSpecNames[count].GetLength() > width) {
                    do {
                        char buf[64];

                        strcpy(buf, g_arrStrSpecNames[count].m_string + g_arrStrSpecNames[count].m_firstCharPos);

                        int n = strlen(buf);

                        buf[n - 2] = '-';
                        buf[n - 1] = 0;
                        ((C2DString *)&g_arrStrSpecNames[count])->SetString(buf, true);
                        g_arrStrSpecNames[count].UpdatePos();
                    } while (g_arrStrSpecNames[count].GetLength() > width);
                }
                count++;
            }
        }
        for (; count < 8; count++)
            g_arrStrSpecNames[count].SetString(0, true);
    }
}

/* whether the pop-up is up (only in a network game; the name ours) */
bool hudIsScorePopupOn()
{
    return theNetwork.m_bNetwork ? data_0040F608 : false;
}

/* the sound the pop-up opens with (zsnd.h's: found by name, played through slot +0x1C) */
class CSndPopup {
public:
    virtual ~CSndPopup();
    virtual void Slot0C();
    virtual void Slot10();
    virtual void Slot14();
    virtual void Slot18();
    virtual void Play(int a, int b, int c);   // +0x1C
};
/* (a view of CSnd, by its own static's name) */
class CSnd {
public:
    static CSndPopup *GetSoundByName(const char *name);
};

/* what the opener keeps while a message pop-up is up: the menu state it took over and the time it has
 * been up (the opener zeroes the time as an int) */
extern signed char data_0040F610;
extern int data_0040F620;

/* the seconds since the score pop-up was rebuilt, the layout's two floats (.data; no names yet) and
 * the pad's pop-up button (SOCOM 1's CCtrlrConfigs::GetButtonState, as hud_weaponsel.cpp's) */
extern float data_00412E78;
namespace {
extern float s_fRowScale;   /* (defined below, with the column's look) */
extern float s_fRowTrans;
}
extern float data_003DC940;
extern float data_003DC948;
extern "C" unsigned char func_002C64E0(int func, CPad *pad);

/* the UI variables (zui.h's: the name's value as a string, when it is one) */
class CUIVariable : public _zrdr {
public:
    const char *GetString() const { return type == 3 ? string : 0; }
};

void hudTurnOffScorePopup();

class CUIVarManager {
public:
    CUIVariable *Find(const char *name);
};
extern CUIVarManager theUIVarManager;

class CZLANLobby {
public:
    bool IsActive();
};
extern CZLANLobby data_0044FE10;   /* the LAN lobby */

/* the pop-up's tick (SOCOM 1's hudTickScorePopup, 0x421DF0 there: up, the select button's rise or a
 * forced close takes it down, and it is rebuilt each second; down, in a network game, the button's
 * fall opens it, and SOCOM II's game details are laid out, each shortened to fit); a message pop-up's
 * time counted, cross then closing it past its minimum and giving the menu state back, its items
 * deleted, its sound played; or, with none up, this map's object under the owner's reticule opening
 * its message once. Byte-exact (round 69, from an interrupted lane's draft): the saved menu state is
 * given back as `(MENU_STATE &)data_0040F610` -- a reference cast reads the signed byte with `lb`,
 * every value cast (int, short, a local, a signed enum) narrows it to `lbu` */
void hudTickScorePopup(CPad *pad, MENU_STATE menustate, float dT)
{
    if (data_0040F608) {
        data_00412E78 += dT;
        if (menustate != MENU_STATE_NONE || func_002C64E0(12, pad) == KEY_RISING)
            hudTurnOffScorePopup();
        else if (data_00412E78 > 1.0f) {
            hudTurnOnScorePopup();
            data_00412E78 = 0.0f;
        }
    } else if (theNetwork.m_bNetwork && menustate == MENU_STATE_NONE && func_002C64E0(12, pad) == KEY_FALLING) {
        C2DFont *font = theHUD->GetFont();

        hudTurnOnScorePopup();
        char text[512];

        if (!data_0044FE10.IsActive()) {
            CUIVariable *var = theUIVarManager.Find("MEDIUS_LOBBY_NAME");

            strcpy(text, var ? var->GetString() : "UNKNOWN");

            float y = 25.0f + data_0040F650 + data_003DC940;

            g_strTeamLabel0.Load(s_fRowScale, text, font, 24.0f, y, true);
            g_strTeamLabel0.SetColor(115.0f, 115.0f, 115.0f);
            g_strTeamLabel0.m_color[3] = s_fRowTrans;
            {
                int width = (int)((data_0040F648 - data_0040F640) - 24.0f);

                g_strTeamLabel0.UpdatePos();
                if (g_strTeamLabel0.GetLength() > width) {
                    do {
                        char buf[64];

                        strcpy(buf, g_strTeamLabel0.m_string + g_strTeamLabel0.m_firstCharPos);

                        int n = strlen(buf);

                        buf[n - 2] = '-';
                        buf[n - 1] = 0;
                        ((C2DString *)&g_strTeamLabel0)->SetString(buf, true);
                        g_strTeamLabel0.UpdatePos();
                    } while (g_strTeamLabel0.GetLength() > width);
                }
            }
            var = theUIVarManager.Find("GAMENAMEVAR");
            strcpy(text, var ? var->GetString() : "UNKNOWN");
            g_strTeamLabel1.Load(s_fRowScale, "???", font, 24.0f, y + data_003DC948, true);
            g_strTeamLabel1.Format(text);
            g_strTeamLabel1.SetColor(115.0f, 115.0f, 115.0f);
            g_strTeamLabel1.m_color[3] = s_fRowTrans;
            {
                int width = (int)((data_0040F648 - data_0040F640) - 24.0f);

                g_strTeamLabel1.UpdatePos();
                if (g_strTeamLabel1.GetLength() > width) {
                    do {
                        char buf[64];

                        strcpy(buf, g_strTeamLabel1.m_string + g_strTeamLabel1.m_firstCharPos);

                        int n = strlen(buf);

                        buf[n - 2] = '-';
                        buf[n - 1] = 0;
                        ((C2DString *)&g_strTeamLabel1)->SetString(buf, true);
                        g_strTeamLabel1.UpdatePos();
                    } while (g_strTeamLabel1.GetLength() > width);
                }
            }
            {
                unsigned char type = data_00437CE8 ? data_00437CE8->m_gametype : 0;
                const char *name = "GAMETYPE ERROR02";

                switch (type) {
                case 1:
                    name = "BREACH";
                    break;
                case 2:
                    name = "DEMOLITION";
                    break;
                case 3:
                    name = "ESCORT";
                    break;
                case 4:
                    name = "EXTRACT";
                    break;
                case 5:
                    name = "SUPPRESSION";
                    break;
                }
                g_strTeamLabel2.Load(s_fRowScale, name, font, 24.0f, y + 2.0f * data_003DC948, true);
                g_strTeamLabel2.SetColor(115.0f, 115.0f, 115.0f);
                g_strTeamLabel2.m_color[3] = s_fRowTrans;
                {
                    int width = (int)((data_0040F648 - data_0040F640) - 24.0f);

                    g_strTeamLabel2.UpdatePos();
                    if (g_strTeamLabel2.GetLength() > width) {
                        do {
                            char buf[64];

                            strcpy(buf, g_strTeamLabel2.m_string + g_strTeamLabel2.m_firstCharPos);

                            int n = strlen(buf);

                            buf[n - 2] = '-';
                            buf[n - 1] = 0;
                            ((C2DString *)&g_strTeamLabel2)->SetString(buf, true);
                            g_strTeamLabel2.UpdatePos();
                        } while (g_strTeamLabel2.GetLength() > width);
                    }
                }
            }
        } else {
            float y = 25.0f + data_0040F650 + data_003DC940;

            g_strTeamLabel0.Load(s_fRowScale, "LAN game", font, 24.0f, y, true);
            g_strTeamLabel0.SetColor(115.0f, 115.0f, 115.0f);
            g_strTeamLabel0.m_color[3] = s_fRowTrans;
            {
                int width = (int)((data_0040F648 - data_0040F640) - 24.0f);

                g_strTeamLabel0.UpdatePos();
                if (g_strTeamLabel0.GetLength() > width) {
                    do {
                        char buf[64];

                        strcpy(buf, g_strTeamLabel0.m_string + g_strTeamLabel0.m_firstCharPos);

                        int n = strlen(buf);

                        buf[n - 2] = '-';
                        buf[n - 1] = 0;
                        ((C2DString *)&g_strTeamLabel0)->SetString(buf, true);
                        g_strTeamLabel0.UpdatePos();
                    } while (g_strTeamLabel0.GetLength() > width);
                }
            }
            CUIVariable *var = theUIVarManager.Find("GAMENAMEVAR");
            const char *game = var ? var->GetString() : "UNKNOWN";

            if (!game)
                game = "GAME TYPE ERROR";
            strcpy(text, game);
            g_strTeamLabel1.Load(s_fRowScale, "???", font, 24.0f, y + data_003DC948, true);
            g_strTeamLabel1.Format(text);
            g_strTeamLabel1.SetColor(115.0f, 115.0f, 115.0f);
            g_strTeamLabel1.m_color[3] = s_fRowTrans;
            {
                int width = (int)((data_0040F648 - data_0040F640) - 24.0f);

                g_strTeamLabel1.UpdatePos();
                if (g_strTeamLabel1.GetLength() > width) {
                    do {
                        char buf[64];

                        strcpy(buf, g_strTeamLabel1.m_string + g_strTeamLabel1.m_firstCharPos);

                        int n = strlen(buf);

                        buf[n - 2] = '-';
                        buf[n - 1] = 0;
                        ((C2DString *)&g_strTeamLabel1)->SetString(buf, true);
                        g_strTeamLabel1.UpdatePos();
                    } while (g_strTeamLabel1.GetLength() > width);
                }
            }
            {
                unsigned char type = data_00437CE8 ? data_00437CE8->m_gametype : 0;
                const char *name = "GAMETYPE ERROR02";

                switch (type) {
                case 1:
                    name = "BREACH";
                    break;
                case 2:
                    name = "DEMOLITION";
                    break;
                case 3:
                    name = "ESCORT";
                    break;
                case 4:
                    name = "EXTRACT";
                    break;
                case 5:
                    name = "SUPPRESSION";
                    break;
                }
                g_strTeamLabel2.Load(s_fRowScale, name, font, 24.0f, y + 2.0f * data_003DC948, true);
                g_strTeamLabel2.SetColor(115.0f, 115.0f, 115.0f);
                g_strTeamLabel2.m_color[3] = s_fRowTrans;
                {
                    int width = (int)((data_0040F648 - data_0040F640) - 24.0f);

                    g_strTeamLabel2.UpdatePos();
                    if (g_strTeamLabel2.GetLength() > width) {
                        do {
                            char buf[64];

                            strcpy(buf, g_strTeamLabel2.m_string + g_strTeamLabel2.m_firstCharPos);

                            int n = strlen(buf);

                            buf[n - 2] = '-';
                            buf[n - 1] = 0;
                            ((C2DString *)&g_strTeamLabel2)->SetString(buf, true);
                            g_strTeamLabel2.UpdatePos();
                        } while (g_strTeamLabel2.GetLength() > width);
                    }
                }
            }
        }
    }
    if (data_0040E480) {
        (float &)data_0040F620 += dT;
        if (pad->m_state[PAD_CROSS] == KEY_RISING && (float &)data_0040F620 > data_0040F618) {
            pad->SetButtonState(PAD_CROSS, KEY_UP);
            data_0040E480 = false;
            CHUD::GetHUDOwner()->m_ctrl->SetMenuState((MENU_STATE &)data_0040F610, false);
            for (int i = 0; i < g_vecPopupItems.size(); i++)
                delete g_vecPopupItems[i];
            g_vecPopupItems.~vector();
            new (&g_vecPopupItems) std::vector<C2D *>;

            CSndPopup *snd = CSnd::GetSoundByName(g_strPopup1.c_str());

            if (snd)
                snd->Play(1, 0, 255);
        }
    } else {
        for (int i = 0; i < g_vecTriggersByMap.size(); i++) {
            CMessageTriggerByMap *t = g_vecTriggersByMap[i];

            if (!strcasecmp(t->m_map.c_str(), GetDatabase())) {
                CObjUnderReticule *obj = CHUD::GetHUDOwner()->m_reticuleObject;

                if (obj && !strcasecmp(t->m_name.c_str(), obj->GetName())) {
                    CMessageTriggerByMap *t = g_vecTriggersByMap[i];

                    if (!t->m_done && hudOpenPopup(&t->m_message))
                        t->m_done = true;
                    break;
                }
            }
        }
    }
}

/* the message keyed by this name shown, once */
void hudTriggerPopup(const char *name)
{
    for (int i = 0; i < g_vecTriggersByName.size(); i++) {
        if (!strcasecmp(g_vecTriggersByName[i]->m_name.c_str(), name)) {
            CMessageTriggerByName *t = g_vecTriggersByName[i];

            if (!t->m_done && hudOpenPopup(&t->m_message))
                t->m_done = true;
            break;
        }
    }
}

/* the message for this map and this object shown, once */
void hudTriggerPopup(CTriggerSource *source)
{
    for (int i = 0; i < g_vecTriggers.size(); i++) {
        CMessageTrigger *t = g_vecTriggers[i];

        if (!strcasecmp(t->m_map.c_str(), GetDatabase()) && !strcasecmp(t->m_name.c_str(), source->GetName())) {
            CMessageTrigger *t = g_vecTriggers[i];

            if (!t->m_done && hudOpenPopup(&t->m_message))
                t->m_done = true;
            break;
        }
    }
}

/* the pop-up off and its texts wiped (CHUD::ReInit_Uninit's call, after the map items' clean-up) */
void hudTurnOffScorePopup()
{
    data_0040F608 = false;
    for (unsigned int i = 0; i < 45; i++)
        g_oSeals2Ds._arrStrs[i].ClearAndWipe();
    for (unsigned int i = 0; i < 45; i++)
        g_oTerrs2Ds._arrStrs[i].ClearAndWipe();
    g_strTeamLabel0.ClearAndWipe();
    g_strTeamLabel1.ClearAndWipe();
    g_strTeamLabel2.ClearAndWipe();
    g_strHeader.ClearAndWipe();
    g_strSpecTitle.ClearAndWipe();
    for (unsigned int i = 0; i < 8; i++)
        g_arrStrSpecNames[i].ClearAndWipe();
    if (theHUD)
        theHUD->On();
}

/* reco.cpp's */
class CReco {
public:
    void Suspend(bool now);
};
extern CReco theReco;
/* the player (fts_main's) */
CZSealBody *ftsGetPlayer();
bool hudHelpWindowsEnabled();
const char *TranslateString(const char *name);
bool SkbWrapText(std::vector<CSaferStr> &lines, const char *text, C2DFont *font, bool byWord, float scale,
                 float width);
/* the mission's abort valve, a halfword at +4 (fts_state.h's CMission::m_valve_abort, theMission + 0x1AC) */
struct CAbortValve {
    unsigned char _0[4];
    unsigned short m_value;   // 0x4
};
#define data_0043668C (*(CAbortValve **)((char *)&::theMission + 0x1AC))   /* theMission+0x1AC */
/* the pop-ups are on at all (OVL2 bss 0x64FF90; no row yet), and what the opener keeps while one is up */
extern unsigned char data_0064FF90;

/* the pop-up's layout: the text's inset and the bottom margin (.data, the names ours) */
float s_fPopupMargin = 20.0f;
float s_fPopupTextX = 20.0f;
float s_fPopupTextY = 10.0f;
float s_fPopupTextScale = 1.0f;

/* the message pop-up opened, if the player can be interrupted: its text wrapped to the window's width
 * and laid out in lines, the prompt below, the backdrop and frame round them, the opening sound.
 * OPEN, 33 instructions, size equal (round 66, written): the target's colours are x s2, the text's new temporary
 * s3, i s4 where this gives the temporary s2, i s3, x s4 -- x needs a number above the new's CInline temporary (same
 * sweep, igreplay): a one-member coord local is scalarised and then reloaded (one saved register fewer), an accessor at
 * one or two levels or a reused local changes nothing else useful; and a few float orders at the first block. The
 * rest is the target's shape: `state` declared before ctrl, bottom before right, wrap and ptext named ahead of their
 * calls, the two widths const */
bool hudOpenPopup(CPopupMessage *message)
{
    signed char state;
    CZSealBody *owner = ftsGetPlayer();
    CSealCtrl *ctrl = owner->m_ctrl;

    state = ctrl->m_menustate;

    if ((MENU_STATE)state != MENU_STATE_NONE)
        return false;
    if (!owner->m_socom2_e1_4)
        return false;
    if (!(data_0064FF90 && !data_0040E480 && hudHelpWindowsEnabled()))
        return false;
    if (data_0043668C && (data_0043668C->m_value == 1 || data_0043668C->m_value == 2))
        return false;
    CZSealBody *player = ftsGetPlayer();

    if (player && !player->m_socom2_e1_4)
        return false;
    theReco.Suspend(true);
    data_0040F610 = state;
    data_0040E480 = true;
    data_0040F620 = 0;
    ctrl->SetMenuState(MENU_STATE_POPUP, false);
    ctrl->StopRumbler();

    int x = message->m_pos.x.v;
    float textx = (float)x + s_fPopupTextX;
    int y = message->m_pos.y.v;
    float textSize = message->m_textSize;
    float texty = (float)y + s_fPopupTextY;
    const CSaferStr &id = message->m_textId;
    std::vector<CSaferStr> lines;

    const char *wrap = TranslateString(id.c_str());

    const float scale = s_fPopupTextScale * message->m_textSize;
    const float wide = (float)(int)((float)message->m_width - 2.0f * s_fPopupTextX);

    SkbWrapText(lines, wrap, theHUD->m_myfont, false, scale, wide);

    float interval = message->m_lineInterval;

    for (int i = 0; i < lines.size(); i++) {
        const char *line = lines[i].c_str();
        CHudBrightText *text = new CHudBrightText;

        g_vecPopupItems.push_back(text);
        text->Load(textSize, line, theHUD->m_myfont, textx, texty + interval * (float)(i + 1), false);
    }
    CHudBrightText *prompt = new CHudBrightText;

    g_vecPopupItems.push_back(prompt);

    const char *ptext = g_strPopup2.c_str();

    texty = 14.0f + (texty + interval * (float)(lines.size() + 1));
    prompt->Load(textSize, ptext, theHUD->m_myfont, textx, texty, true);
    g_bmpPopupBg.SetHasTexture(false);
    g_bmpPopupFrame.SetHasTexture(false);

    int bottom;
    int right = message->m_pos.x.v + message->m_width;

    bottom = (int)(texty + s_fPopupMargin);

    g_bmpPopupBg.Load((float)x, (float)y, (float)right, (float)bottom, 0);
    g_bmpPopupBg.SetColor(11.0f, 21.0f, 24.0f);
    g_bmpPopupFrame.Load((float)(x - 7), (float)(y - 7), (float)(right + 7), (float)(bottom + 7), 0);
    g_bmpPopupFrame.SetColor(35.0f, 51.0f, 56.0f);
    g_bmpPopupBg.SetTrans(100.0f);
    g_bmpPopupFrame.SetTrans(80.0f);

    CSndPopup *snd = CSnd::GetSoundByName(g_strPopup0.c_str());

    if (snd)
        snd->Play(1, 0, 255);
    return true;
}

/* the spectators' box (SOCOM II's; the names ours): its tuning, plain initialised data 8 bytes
 * apart (MW 3.0's function-local statics, written as file statics: scdiff compares them by
 * content) -- the box's offset below the pop-up's top, the title's alpha, the box's bottom, the
 * names' scale and load scale, their spacing and alpha -- and the box's texture */
static float s_specboxy = 100.0f;
static float s_spectitletrans = 90.0f;
static float s_specboxbottom = 430.0f;
static float s_specnamescale = 0.7f;
static float s_specnameload = 0.8f;
static float s_specnamespacing = 16.8f;
static float s_specnametrans = 110.0f;
static const char *s_specboxtex = "newweapnbkrnd.tif";

/* the spectators' box's frame as 0x22CC10 lays it out (by address: the initialiser does not build
 * them): its left, its right and its top */
extern int data_0040F640;
extern int data_0040F648;
extern int data_0040F650;

/* the spectators' box laid out under the pop-up: its title bar, the title, the box and eight name
 * lines (called once by 0x22CC10, after SetupBackgroundBox; the name ours) */
namespace {
void SetupSpectatorBox()
{
    C2DFont *font;   // declared first: it takes s1, the texture s0
    zdb::CTexHandle *tex = zdb::CAssetMgr::m_assets.GetTexHandle(s_specboxtex);

    font = theHUD->m_myfont;
    float top = 25.0f + (float)data_0040F650 + s_specboxy;
    float bottom = 25.0f + top;

    g_bmpSpecTitleBg.Load((float)data_0040F640, top, (float)data_0040F648, bottom, tex);
    g_bmpSpecTitleBg.SetColor(g_vSpecBoxColor);
    g_bmpSpecTitleBg.SetTrans(80.0f);
    g_strSpecTitle.Load(1.0f, "SPECTATORS", font, 0.0f, 0.0f, false);
    g_strSpecTitle.SetPos((int)(10.0f + (float)data_0040F640), (int)(20.0f + top));
    g_strSpecTitle.SetTrans(s_spectitletrans);
    g_bmpSpecBox.Load((float)data_0040F640, bottom, (float)data_0040F648, s_specboxbottom, tex);
    for (unsigned int i = 0; i < 8; i++) {
        g_arrStrSpecNames[i].m_scale = s_specnamescale;
        g_arrStrSpecNames[i].Load(s_specnameload, 0, font, 0.0f, 0.0f, false);
        g_arrStrSpecNames[i].SetPos(24, (int)(16.0f + bottom + s_specnamespacing * (float)i));
        g_arrStrSpecNames[i].SetTrans(s_specnametrans);
    }
}
}

/* the pop-up's frame (SOCOM 1's g_iBoxLeftX, g_iBoxRightX, g_iBoxTopY; by address: the
 * initialiser does not build them): its left, right and top */
extern int data_0040F628;
extern int data_0040F630;
extern int data_0040F638;
/* the box's inner bottom, SetupBackgroundBox's local static, and its guard */
extern signed char data_00412E80;
extern float data_003E5728;

namespace {

/* the column's look (OVL1 .data, 8 apart; the names ours) */
static float s_fRowTop = 38.0f;
static float s_fRowSpacing = 16.8f;
float s_fRowScale = 0.8f;   /* (no `static`: declared extern above; the namespace makes it internal) */
static float s_fTeamTextScale = 1.0f;
static int s_iNameX = 70;
static int s_iScoreColumns = 175;
float s_fTeamTextTrans = 90.0f;
float s_fRowTrans = 110.0f;
static float s_fBombX = 200.0f;
static float s_fBombSize = 16.0f;
static float s_fBombDY = 12.0f;
static float s_fStripeX = 161.0f;
static float s_fStripeW = 463.0f;
static float s_fStripeH = 16.0f;
static float s_fStripeDY = 12.0f;
static float s_fStripeTrans = 60.0f;

/* one row (SOCOM 1's AddRow, 0x421510 there): grey, the AI's and the out's colours dimmed; the bomb's
 * mark beside whoever holds it; the name, the clan's tag and the three scores (SOCOM 1's CIntStr
 * formats them, a %d in place here).
 * OPEN, 74 instructions, 0x614 of 0x614, a pure saved-register permutation: the target's order of
 * colouring is seal s0, iRow s1, this s2, then the stats' two s3 and s4 where ours colours stats1, stats0
 * first (s0, s1) and seal, iRow, this after (s2-s4) -- all five are pushed in one sweep here, ascending,
 * so the stats would need to be pushed a sweep earlier (one interference fewer: they have the most
 * neighbours). Tried: the stats as pointers, swapped, after the row's string, the kills summed first, the
 * seal through a pointer local. The copies of the colour go through an inline taking a const reference
 * (through it IRO_CommonSubs finds the three addresses only at the reads, s6-s8, the target's; written
 * in place they are made at the first stores, 0x620) */
static inline void SetColor4(C2DString &s, const sceVu0FVECTOR &c)
{
    s.m_color[0] = c[0];
    s.m_color[1] = c[1];
    s.m_color[2] = c[2];
    s.m_color[3] = c[3];
}

void CTeamDisplay::AddRow(CZSealBody *const *arrSeals, int iRow, float fTopY)
{
    CZSealBody &seal = *arrSeals[iRow];
    C2DFont *font = theHUD->GetFont();
    sceVu0FVECTOR color;
    sceVu0FVECTOR colorClan;

    color[0] = 115.0f;
    color[1] = 115.0f;
    color[2] = 115.0f;
    color[3] = 128.0f;
    colorClan[0] = 128.0f;
    colorClan[1] = 64.0f;
    colorClan[2] = 32.0f;
    colorClan[3] = 128.0f;

    float fY = fTopY + s_fRowTop + (float)iRow * s_fRowSpacing;
    bool bPlayer = seal.m_ctrl ? seal.m_ctrl->IsPlayer() : false;

    if (bPlayer)
        color[2] = 12.0f;
    if (!seal.m_socom2_e1_4) {
        sceVu0ScaleVector(color, color, 0.6f);
        sceVu0ScaleVector(colorClan, colorClan, 0.6f);
    }
    if (seal.m_kit.HasItemWithAmmo((EQUIP_ITEM)154)) {
        g_bmpBoxOverlay.SetPos((int)s_fBombX, (int)(fY - s_fBombDY), (int)(s_fBombX + s_fBombSize),
                               (int)(fY + s_fBombSize - s_fBombDY));
        g_bmpBoxOverlay.On();
    }

    C2DString &strName = _arrStrs[iRow + 5];

    strName.Load(s_fRowScale, seal.m_name, font, (float)(data_0040F628 + s_iNameX), fY, false);
    SetColor4(strName, color);
    strName.SetTrans(s_fRowTrans);

    C2DString &strClan = _arrStrs[iRow + 37];
    const char *clan = CZPersonaState::GetPersonaClanTag(seal.m_persona);
    char arrClan[16] = " ";

    if (clan && strlen(clan)) {
        strcpy(arrClan, "[");
        strcat(arrClan, clan);
        strcat(arrClan, "]");
    }
    strClan.Load(s_fRowScale, arrClan, font, (float)(data_0040F628 + 10), fY, false);
    SetColor4(strClan, colorClan);
    strClan.SetTrans(s_fRowTrans);

    CSealStats &stats0 = seal.m_stats[0];
    CSealStats &stats1 = seal.m_stats[1];
    C2DString &strKills = _arrStrs[iRow + 29];
    char arrKills[16];

    sprintf(arrKills, "%d", seal.m_stats[0].m_kills + seal.m_stats[1].m_kills);
    strKills.Load(s_fRowScale, arrKills, font, (float)(int)((float)(data_0040F630 - s_iScoreColumns / 3) - -16.0f), fY, false);
    SetColor4(strKills, color);
    strKills.m_isCentered = 1;
    strKills.SetTrans(s_fRowTrans);

    C2DString &strFrags = _arrStrs[iRow + 13];
    char arrFrags[16];

    sprintf(arrFrags, "%d", stats0.m_c + stats1.m_c);
    strFrags.Load(s_fRowScale, arrFrags, font, (float)(data_0040F630 - s_iScoreColumns), fY, false);
    SetColor4(strFrags, color);
    strFrags.m_isCentered = 1;
    strFrags.SetTrans(s_fRowTrans);

    C2DString &strDeaths = _arrStrs[iRow + 21];
    char arrDeaths[16];

    sprintf(arrDeaths, "%d", stats0.m_12 + stats1.m_12);
    strDeaths.Load(s_fRowScale, arrDeaths, font, (float)(int)((float)(data_0040F630 - s_iScoreColumns / 3 * 2) - -8.0f), fY, false);
    SetColor4(strDeaths, color);
    strDeaths.m_isCentered = 1;
    strDeaths.SetTrans(s_fRowTrans);
}

void CTeamDisplay::SetupTeam(float fTopY, float fBgR, float fBgG, float fBgB, int iTeam,
                             int (CZNetGame::*pWinsFunc)() const, bool bTeam0)
{
    char arrWinsStr[16];
    char arrTitle[64];
    CZSealBody *arrSortedSeals[8];
    CZSealBody *arrUnsortedSeals[8];
    int arrUnsortedKills[8];
    C2DFont *font = theHUD->GetFont();

    _bmpTitleBg.SetHasTexture(false);
    _bmpTitleBg.SetColor(fBgR, fBgG, fBgB);
    _bmpTitleBg.SetTrans(80.0f);
    _bmpTitleBg.Load((float)data_0040F628, fTopY, (float)data_0040F630, 25.0f + fTopY, 0);
    sprintf(arrWinsStr, "%d", (data_00437CE8->*pWinsFunc)());

    int iTextX = (int)(10.0f + (float)data_0040F628 - -8.0f);
    int iTextY = (int)(20.0f + fTopY);

    FormatString(arrTitle, 32, iTeam, 0, 0);
    strcat(arrTitle, " :   ");
    strcat(arrTitle, arrWinsStr);
    _arrStrs[0].Load(s_fTeamTextScale, arrTitle, font, (float)iTextX, (float)iTextY, false);
    _arrStrs[0].SetTrans(s_fTeamTextTrans);
    _arrStrs[1].Load(s_fTeamTextScale, GetString(0xEC6B), font, (float)(data_0040F630 - s_iScoreColumns), (float)iTextY, false);
    _arrStrs[1].m_isCentered = 1;
    _arrStrs[1].SetTrans(s_fTeamTextTrans);
    const char *szDeath = "DEATH";

    _arrStrs[2].Load(s_fTeamTextScale, szDeath, font, (float)(int)((float)(data_0040F630 - s_iScoreColumns / 3 * 2) - -8.0f), (float)iTextY, false);
    _arrStrs[2].m_isCentered = 1;
    _arrStrs[2].SetTrans(s_fTeamTextTrans);
    const char *szScore = "SCORE";

    _arrStrs[3].Load(s_fTeamTextScale, szScore, font, (float)(int)((float)(data_0040F630 - s_iScoreColumns / 3) - -16.0f), (float)iTextY, false);
    _arrStrs[3].m_isCentered = 1;
    _arrStrs[3].SetTrans(s_fTeamTextTrans);

    int iNumSeals = GetTeamMembers(arrUnsortedSeals, arrUnsortedKills, bTeam0);

    for (int iNumMoved = 0; iNumMoved < iNumSeals; iNumMoved++) {
        int iBest = 0;

        for (int i = 1; i < iNumSeals; i++) {
            bool bBetter = false;

            if (arrUnsortedSeals[iBest]) {
                if (arrUnsortedSeals[i])
                    bBetter = arrUnsortedKills[i] > arrUnsortedKills[iBest];
            } else {
                bBetter = true;
            }
            if (bBetter)
                iBest = i;
        }
        arrSortedSeals[iNumMoved] = arrUnsortedSeals[iBest];
        arrUnsortedSeals[iBest] = 0;
    }
    for (unsigned int iRow = 0; iRow < 8; iRow++) {
        if (iRow & 1) {
            float y = fTopY + s_fRowTop + (float)iRow * s_fRowSpacing;
            C2DBitmap *bmp = &g_arrBmpSpecPieces[iRow + (bTeam0 ? 0 : 8)];

            bmp->SetPos((int)s_fStripeX, (int)(y - s_fStripeDY), (int)(s_fStripeX + s_fStripeW),
                        (int)(y + s_fStripeH - s_fStripeDY));
            bmp->C2DBitmap::SetTrans(s_fStripeTrans);
            bmp->On();
        }
    }
    for (int i = 0; i < iNumSeals; i++)
        AddRow(arrSortedSeals, i, fTopY);
}

/* the pop-up's frame: its nine pieces loaded and UV'd from one texture (SOCOM 1's, the bottom now
 * the spectators' box's), and the sixteen bitmaps the tick places later. The bottom-right corner's
 * SetUV loads the corner u before v, where MW hoists v's load (it feeds the `1 - v`), as the target
 * itself does for the bottom-left corner: a volatile read of u is the one shape found that keeps
 * u's load first (round 42 -- a STAND-IN: the variable volatile throughout moves six other loads; a
 * local u or v, `-v + 1`, a local 1.0f, an inline accessor, a `const &`, internal linkage for either,
 * a folded `1.0f * u` or `u + 0.0f`, a same-type cast all keep MW's order). Each corner is a block of
 * four locals (x0, y0, x1, y1 declared in that order give f23-f20; one set of locals reused across
 * the corners re-colours the second corner) */
void SetupBackgroundBox()
{
    if (!data_00412E80) {
        data_003E5728 = s_specboxbottom - 20.0f;
        data_00412E80 = 1;
    }

    {
        float x0 = (float)data_0040F628;
        float y0 = (float)data_0040F638;
        float x1 = 20.0f + (float)data_0040F628;
        float y1 = 20.0f + (float)data_0040F638;

        g_bmpUlCorner.Load(x0, y0, x1, y1, zdb::CAssetMgr::m_assets.GetTexHandle(s_specboxtex));
        g_bmpUlCorner.SetUV(1.0f, 0.0f, s_fCornerU, s_fCornerV);
    }
    {
        float x0 = (float)data_0040F630 - 20.0f;
        float y0 = (float)data_0040F638;
        float x1 = (float)data_0040F630;
        float y1 = 20.0f + (float)data_0040F638;

        g_bmpUrCorner.Load(x0, y0, x1, y1, zdb::CAssetMgr::m_assets.GetTexHandle(s_specboxtex));
        g_bmpUrCorner.SetUV(s_fCornerU, 0.0f, 1.0f, s_fCornerV);
    }
    {
        float x0 = (float)data_0040F628;
        float y0 = s_specboxbottom - 20.0f;
        float x1 = 20.0f + (float)data_0040F628;
        float y1 = s_specboxbottom;

        g_bmpLlCorner.Load(x0, y0, x1, y1, zdb::CAssetMgr::m_assets.GetTexHandle(s_specboxtex));
        g_bmpLlCorner.SetUV(1.0f, 1.0f - s_fCornerV, s_fCornerU, 1.0f);
    }
    {
        float x0 = (float)data_0040F630 - 20.0f;
        float y0 = s_specboxbottom - 20.0f;
        float x1 = (float)data_0040F630;
        float y1 = s_specboxbottom;

        g_bmpLrCorner.Load(x0, y0, x1, y1, zdb::CAssetMgr::m_assets.GetTexHandle(s_specboxtex));
        g_bmpLrCorner.SetUV(*(volatile float *)&s_fCornerU, 1.0f - s_fCornerV, 1.0f, 1.0f);
    }

    float left = 20.0f + (float)data_0040F628;
    float right = (float)data_0040F630 - 20.0f;
    float top = 20.0f + (float)data_0040F638;
    zdb::CTexHandle *tex = zdb::CAssetMgr::m_assets.GetTexHandle(s_specboxtex);

    g_bmpTopEdge.Load(left, (float)data_0040F638, right, top, tex);
    g_bmpTopEdge.SetUV(0.5f, 0.0f, 0.5f, s_fCornerV);
    g_bmpLeftEdge.Load((float)data_0040F628, top, left, data_003E5728, tex);
    g_bmpLeftEdge.SetUV(1.0f, 0.5f, s_fCornerU, 0.5f);
    g_bmpRightEdge.Load(right, top, (float)data_0040F630, data_003E5728, tex);
    g_bmpRightEdge.SetUV(s_fCornerU, 0.5f, 1.0f, 0.5f);
    g_bmpBottomEdge.Load(left, data_003E5728, right, s_specboxbottom, tex);
    g_bmpBottomEdge.SetUV(0.5f, 1.0f - s_fCornerV, 0.5f, 1.0f);
    g_bmpCenter.Load(left, top, right, data_003E5728, tex);
    g_bmpCenter.SetUV(0.5f, 0.5f, 0.5f, 0.5f);
    for (unsigned int i = 0; i < 16; i++) {
        g_arrBmpSpecPieces[i].Load(0.0f, 0.0f, 0.0f, 0.0f, tex);
        g_arrBmpSpecPieces[i].SetUV(0.5f, 0.5f, 0.5f, 0.5f);
    }
}
}

/* a tag's first two numbers as integers, 0 for what is missing (the pop-up loader's reader, called
 * twice by hudAddPopupDefinition 0x22D660, which the target does not inline it into: pinned; the
 * name ours) */
#pragma auto_inline off
IPNT2D GetTagIPNT2D(const _zrdr *rdr, const char *tag)
{
    _zrdr *t = zrdr_findtag(rdr, tag);
    _zrdr *a = t ? t->Item(0) : 0;
    int x = a ? a->Int() : 0;
    _zrdr *b = t ? t->Item(1) : 0;
    int y = b ? b->Int() : 0;

    return IPNT2D(x, y);
}
#pragma auto_inline reset

/* the message pop-up: its two bitmaps and its items (the object itself holds nothing) */
void CHudPopups::Draw(zdb::CCamera *cam)
{
    if (data_0040E480) {
        g_bmpPopupFrame.Draw(cam);
        g_bmpPopupBg.Draw(cam);
        for (int i = 0; i < g_vecPopupItems.size(); i++)
            g_vecPopupItems[i]->Draw(cam);
    }
}

/* everything drawn while the pop-up is up */
void CScorePopup::Draw(zdb::CCamera *cam)
{
    if (!data_0040F608)
        return;
    g_oSeals2Ds._bmpTitleBg.Draw(cam);
    g_oTerrs2Ds._bmpTitleBg.Draw(cam);
    g_bmpUlCorner.Draw(cam);
    g_bmpUrCorner.Draw(cam);
    g_bmpLlCorner.Draw(cam);
    g_bmpLrCorner.Draw(cam);
    g_bmpTopEdge.Draw(cam);
    g_bmpLeftEdge.Draw(cam);
    g_bmpRightEdge.Draw(cam);
    g_bmpBottomEdge.Draw(cam);
    g_bmpCenter.Draw(cam);
    g_bmpTeamDivider.Draw(cam);
    g_bmpHeaderBg.Draw(cam);
    g_bmpHeaderBar.Draw(cam);
    g_strHeader.Draw(cam);
    for (unsigned int i = 0; i < 16; i++)
        g_arrBmpSpecPieces[i].Draw(cam);
    for (unsigned int i = 0; i < 45; i++)
        g_oSeals2Ds._arrStrs[i].Draw(cam);
    for (unsigned int i = 0; i < 45; i++)
        g_oTerrs2Ds._arrStrs[i].Draw(cam);
    g_strTeamLabel0.Draw(cam);
    g_strTeamLabel1.Draw(cam);
    g_strTeamLabel2.Draw(cam);
    g_bmpSpecBox.Draw(cam);
    g_bmpSpecTitleBg.Draw(cam);
    g_strSpecTitle.Draw(cam);
    for (unsigned int i = 0; i < 8; i++)
        g_arrStrSpecNames[i].Draw(cam);
    g_bmpBoxOverlay.Draw(cam);
}
