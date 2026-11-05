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
 * hudIsScorePopupOn (0x22CBF0) and 0x22CC10. Written, all byte-exact: CScorePopup's destructor
 * and Draw, CHudPopups' Draw and destructor, hudTurnOffScorePopup, the triggers and
 * hudIsScorePopupOn; and (round 33) the message pop-ups' third trigger (0x22D270, by names), their
 * reset (0x22D190) and hud.rdr's common look (0x22D3F0). The unit's statics are anonymous-namespace globals in SOCOM 1 (g_bOn,
 * g_oSeals2Ds, g_oTerrs2Ds, the box's ten CScoreBmps); its initialiser (0x3FE980, 0x9D0 bytes)
 * builds every one, byte-exact with the objects defined in their address order (round 32; the
 * flags and the frame's ints, which it does not build, still by address). Defined in reverse
 * of their image order (-inline deferred). */

#include "hud_weaponsel.h"
#include "../../gamez/zrdr/zrdr.h"

#include <string.h>
#include <string>
#include <vector>

/* the HUD's backdrop alpha (hud_main.cpp's layout float, 76.8) and two layout floats the corners'
 * texture coordinates come from (173 and 74) */
extern float data_003E3000;
extern float data_003E3018;
extern float data_003E2FF0;

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

    unsigned char _0[0x498];
    C2DFont *m_myfont;   // 0x498
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

struct IPNT2D {
    IPNT2D() {}
    IPNT2D(int ax, int ay) { x = ax; y = ay; }

    int x, y;
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
};   // 0x20 bytes

/* each trigger's condition, a base with its own constructor (the done flag cleared): the loader's
 * `new` calls it out of line, then the message's (0x22DC40, 0x22DC70, 0x22DCA0, 0x22DDC0; the
 * names ours). A HUD objective's: its valve's name and the mission */
struct CTriggerByValve {
    CTriggerByValve() : m_done(false) {}

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


/* the message pop-up opened (0x22B4A0: true once shown; not written yet) */
extern "C" bool func_0022B4A0(void *message);

/* what a trigger is given: an object with its name at 0x20 (no name yet) */
/* a message keyed by the map and any of several objects' names (0x22D270's records: the done flag,
 * the names, the map, the message) and one keyed by the map alone (0x22D190 clears its flag) */
struct CTriggerByNames {
    CTriggerByNames() : m_done(false) {}
    /* a name appended, through the new last element (the loader's; inlined one level: push_back,
     * the temporary's constructor and destructor, end(), the reverse * and CSaferStr's assignment
     * called out of line, as the target does; the name ours) */
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
class CZSealBody {
public:
    unsigned char _0[0x14];
    int m_14;   // 0x14
    unsigned char _18[0x580 - 0x18];
    short m_kills;   // 0x580
    unsigned char _582[0x5c8 - 0x582];
    short m_kills2;   // 0x5c8
    unsigned char _5ca[0xfd1 - 0x5ca];
    unsigned char m_hidden;   // 0xfd1
};

/* a thing in the world (st_exit.cpp's stub: only the list here) */
class CEntity {
public:
    static std::list<CEntity *> m_list;
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
        if (!seal.m_14 || seal.m_hidden)
            continue;
        arrSeals[iNumSeals] = &seal;
        arrKills[iNumSeals] = seal.m_kills + seal.m_kills2;
        if (++iNumSeals >= 8)
            return iNumSeals;
    }
    return iNumSeals;
}
}

/* a tag's first two numbers as integers (below, 0x22B360) */
IPNT2D GetTagIPNT2D(const _zrdr *rdr, const char *tag);

/* one of hud.rdr's PopupDefinitions read (SOCOM II's; hudInit calls it for each; the name ours, after
 * SOCOM 1's hudAddPolyType): the Trigger block's Type picks the trigger and its list -- an action
 * icon's first use (its Icons' names and the Mission), an animation command (by Name), an object
 * first under the reticule (ObjName, Mission), a HUD objective (Valve, Mission) -- each new'ed and
 * pushed first; then the message every trigger ends with: the window, the text size scaled by
 * 1.1429, the line spacing (20 unless given) and the text id. Each new name is stored through the
 * vector's last element, `*rbegin()` (its end() 0x22DD20 and the reverse * 0x22DD10 out of line, then
 * CSaferStr's assignment 0x22DCC0), every other string through CSaferStr's inline assignment.
 * OPEN, 0x6A4 against 0x5E0 (a level: the README's "Level 6, or 8?"): the target inlines the name
 * append one level -- push_back (0x22DD60), the temporary's constructor and destructor (0x1E7B60,
 * 0x1E7A50), the reverse * (0x22DD10) with rbegin()'s end() (0x22DD20) and CSaferStr's assignment
 * (0x22DCC0) all called -- which needs AddName at least six high; with these headers it is four
 * (at `#pragma inline_depth(4)` only rbegin's end and constructor are cut, push_back still
 * inlines), so it inlines whole. The rest is written from the target: the four triggers' lists
 * (0x412D90 by names, 0x412DB0 by name, 0x412DF0 by object, 0x412DD0 by valve), their base
 * constructors and the message's out of line, each pointer pushed through a stack slot and
 * reloaded, GetTagIPNT2D called (pinned), the text size stored back before the message's copy */
void hudAddPopupDefinition(_zrdr *popup)
{
    CPopupMessage *msg = 0;
    _zrdr *trigger = zrdr_findtag(popup, "Trigger");
    const char *type = zrdr_findstring(trigger, "Type");

    if (type && !strcasecmp(type, "ActionIcon1stTime")) {
        CMessageTriggerByNames *t = new CMessageTriggerByNames;

        msg = &t->m_message;
        g_vecTriggersByNames.push_back(t);
        _zrdr *icons = zrdr_findtag(trigger, "Icons");

        if (icons) {
            for (unsigned int i = 0; i < icons->Count(); i++)
                t->AddName(icons->Item(i)->String());
        }
        t->m_map = zrdr_findstring(trigger, "Mission");
    } else if (type && !strcasecmp(type, "AnimCmd")) {
        CMessageTriggerByName *t = new CMessageTriggerByName;

        msg = &t->m_message;
        g_vecTriggersByName.push_back(t);
        t->m_name = zrdr_findstring(trigger, "Name");
    } else if (type && !strcasecmp(type, "ObjUnderRet1stTime")) {
        CMessageTriggerByMap *t = new CMessageTriggerByMap;

        msg = &t->m_message;
        g_vecTriggersByMap.push_back(t);
        t->m_name = zrdr_findstring(trigger, "ObjName");
        t->m_map = zrdr_findstring(trigger, "Mission");
    } else if (type && !strcasecmp(type, "HudObjective")) {
        CMessageTrigger *t = new CMessageTrigger;

        msg = &t->m_message;
        g_vecTriggers.push_back(t);
        t->m_name = zrdr_findstring(trigger, "Valve");
        t->m_map = zrdr_findstring(trigger, "Mission");
    }
    if (msg) {
        msg->m_pos = GetTagIPNT2D(popup, "WindowPos");
        msg->m_width = GetTagIPNT2D(popup, "WindowSize").x;
        float size = 1.0f;

        zrdr_findreal(popup, "TextSize", &size, 1);
        size *= 1.1429f;
        msg->m_textSize = size;
        float spacing = 20.0f;

        zrdr_findreal(popup, "LineInterval", &spacing, 1);
        msg->m_lineInterval = spacing;
        msg->m_textId = zrdr_findstring(popup, "TextId");
    }
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
                if (!func_0022B4A0(&t->m_message))
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

/* the message pop-ups' common look (OVL1 .data, by address: its margins 10, 20 and 20 and the
 * backdrop's opacity 1, 8 bytes apart; the forced-open time in bss) */
extern float data_003DC8B8;
extern float data_003DC8B0;
extern float data_003DC8A8;
extern float data_003DC8C0;
extern float data_0040F618;

/* hud.rdr's EventPopups/Common (SOCOM II's, hudInit's call; the name ours): the margins, the prompt,
 * the backdrop's opacity, the forced-open time and the opening and closing sounds */
void hudReadPopupCommon(_zrdr *rdr)
{
    zrdr_findreal(rdr, "TopMargin", &data_003DC8B8, 1);
    zrdr_findreal(rdr, "LeftMargin", &data_003DC8B0, 1);
    zrdr_findreal(rdr, "BottomMargin", &data_003DC8A8, 1);
    g_strPopup2 = zrdr_findstring(rdr, "PromptText");
    zrdr_findreal(rdr, "BackgroundOpacity", &data_003DC8C0, 1);
    zrdr_findreal(rdr, "ForceOpenTime", &data_0040F618, 1);
    g_strPopup0 = zrdr_findstring(rdr, "OpeningSound");
    g_strPopup1 = zrdr_findstring(rdr, "ClosingSound");
}

/* whether the pop-up is up (only in a network game; the name ours) */
bool hudIsScorePopupOn()
{
    return theNetwork.m_bNetwork ? data_0040F608 : false;
}

/* the message keyed by this name shown, once */
void hudTriggerPopup(const char *name)
{
    for (int i = 0; i < g_vecTriggersByName.size(); i++) {
        if (!strcasecmp(g_vecTriggersByName[i]->m_name.c_str(), name)) {
            CMessageTriggerByName *t = g_vecTriggersByName[i];

            if (!t->m_done && func_0022B4A0(&t->m_message))
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

            if (!t->m_done && func_0022B4A0(&t->m_message))
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
