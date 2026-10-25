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
 * hudIsScorePopupOn. The unit's statics are anonymous-namespace globals in SOCOM 1 (g_bOn,
 * g_oSeals2Ds, g_oTerrs2Ds, the box's ten CScoreBmps); its initialiser (0x3FE980, 0x9D0 bytes)
 * builds every one, so they are declared by address here until it is written. Defined in
 * reverse of their image order (-inline deferred). */

#include "hud_weaponsel.h"
#include "../../gamez/zrdr/zrdr.h"

#include <string.h>
#include <string>
#include <vector>

namespace {

/* a bitmap of the box (SOCOM 1's CScoreBmp: C2DBitmap with its own alpha set at construction; its
 * vtable 0x405A70 overrides nothing but the destructor, so MW 3.0 calls even a global one's Draw
 * through the table) */
class CScoreBmp : public C2DBitmap {
};

/* a team's column (SOCOM 1's CTeamDisplay, 0xF60 there with 27 rows; SOCOM II's 0x1B50: its title
 * bitmap, 45 rows, and 0x18 bytes of its own) */
class CTeamDisplay {
public:
    CScoreBmp _bmpTitleBg;   // 0x0
    CHudBrightText _arrStrs[45];   // 0x80
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

/* the unit's globals (the initialiser 0x3FE980 builds them; by address until it is written): */
extern bool data_0040F608;   // SOCOM 1's g_bOn: the pop-up is up
extern CTeamDisplay data_0040F670;   // SOCOM 1's g_oSeals2Ds
extern CTeamDisplay data_004111C0;   // SOCOM 1's g_oTerrs2Ds
extern CScoreBmp data_0040E4A0;   // the box's nine pieces (SOCOM 1's corners, edges and centre)
extern CScoreBmp data_0040E530;
extern CScoreBmp data_0040E5C0;
extern CScoreBmp data_0040E650;
extern CScoreBmp data_0040E6E0;
extern CScoreBmp data_0040E770;
extern CScoreBmp data_0040E800;
extern CScoreBmp data_0040E890;
extern CScoreBmp data_0040E920;
extern C2DBitmap data_0040E9B0;
extern C2DString data_0040EA40;
extern CScoreBmp data_0040EAF0;
extern CScoreBmp data_0040EB80;
extern C2DString data_0040EC10;
extern CScoreBmp data_0040ECC0;
extern C2DString data_0040ED50[8];
extern CScoreBmp data_0040F380;   // SOCOM 1's g_bmpTeamDivider (between the two columns)
extern C2DString data_0040F410;
extern C2DString data_0040F4C0;
extern C2DString data_0040F570;
extern C2DBitmap data_0040DBF0;
extern C2DBitmap data_0040DC80[16];
/* the second pop-up (0x22AB80's): whether it is up, its two bitmaps and its items */
extern bool data_0040E480;
extern C2DBitmap data_0040F220;
extern C2DBitmap data_0040F2D0;
extern std::vector<C2D *> data_00412D70;

/* a message shown once (SOCOM II's; the names ours): a done flag, the map it is for and the object
 * or event that shows it, then what the opener (0x22B4A0) takes */
struct CMessageTrigger {
    bool m_done;   // 0x0
    std::string m_name;   // 0x4
    std::string m_map;   // 0x10
    int m_message[1];   // 0x1c
};

/* a message keyed by name alone (0x22BD50's records: the name, the done flag, the message) */
struct CMessageTriggerByName {
    std::string m_name;   // 0x0
    bool m_done;   // 0xc
    int m_message[1];   // 0x10
};

extern std::vector<CMessageTrigger *> data_00412DD0;
extern std::vector<CMessageTriggerByName *> data_00412DB0;

/* the message pop-up opened (0x22B4A0: true once shown; not written yet) */
extern "C" bool func_0022B4A0(void *message);

/* what a trigger is given: an object with its name at 0x20 (no name yet) */
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

/* whether the pop-up is up (only in a network game; the name ours) */
bool hudIsScorePopupOn()
{
    return theNetwork.m_bNetwork ? data_0040F608 : false;
}

/* the message keyed by this name shown, once */
void hudTriggerPopup(const char *name)
{
    for (int i = 0; i < data_00412DB0.size(); i++) {
        if (!strcasecmp(data_00412DB0[i]->m_name.c_str(), name)) {
            CMessageTriggerByName *t = data_00412DB0[i];

            if (!t->m_done && func_0022B4A0(t->m_message))
                t->m_done = true;
            break;
        }
    }
}

/* the message for this map and this object shown, once */
void hudTriggerPopup(CTriggerSource *source)
{
    for (int i = 0; i < data_00412DD0.size(); i++) {
        CMessageTrigger *t = data_00412DD0[i];

        if (!strcasecmp(t->m_map.c_str(), GetDatabase()) && !strcasecmp(t->m_name.c_str(), source->GetName())) {
            CMessageTrigger *t = data_00412DD0[i];

            if (!t->m_done && func_0022B4A0(t->m_message))
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
        data_0040F670._arrStrs[i].ClearAndWipe();
    for (unsigned int i = 0; i < 45; i++)
        data_004111C0._arrStrs[i].ClearAndWipe();
    data_0040F410.ClearAndWipe();
    data_0040F4C0.ClearAndWipe();
    data_0040F570.ClearAndWipe();
    data_0040EA40.ClearAndWipe();
    data_0040EC10.ClearAndWipe();
    for (unsigned int i = 0; i < 8; i++)
        data_0040ED50[i].ClearAndWipe();
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

/* the pop-up's frame as SetupBackgroundBox leaves it (by address until the initialiser is written):
 * its left, its right and its top */
extern int data_0040F640;
extern int data_0040F648;
extern int data_0040F650;
/* the box's colour (a CPnt3D the initialiser builds) */
extern CPnt3D data_00412E68;

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

    data_0040EB80.Load((float)data_0040F640, top, (float)data_0040F648, bottom, tex);
    data_0040EB80.SetColor(data_00412E68);
    data_0040EB80.SetTrans(80.0f);
    data_0040EC10.Load(1.0f, "SPECTATORS", font, 0.0f, 0.0f, false);
    data_0040EC10.SetPos((int)(10.0f + (float)data_0040F640), (int)(20.0f + top));
    data_0040EC10.SetTrans(s_spectitletrans);
    data_0040ECC0.Load((float)data_0040F640, bottom, (float)data_0040F648, s_specboxbottom, tex);
    for (unsigned int i = 0; i < 8; i++) {
        data_0040ED50[i].m_scale = s_specnamescale;
        data_0040ED50[i].Load(s_specnameload, 0, font, 0.0f, 0.0f, false);
        data_0040ED50[i].SetPos(24, (int)(16.0f + bottom + s_specnamespacing * (float)i));
        data_0040ED50[i].SetTrans(s_specnametrans);
    }
}
}

/* the pop-up's frame (by address until the initialiser is written): its left, right and top, and
 * the corners' texture coordinates (zero in the image: the initialiser sets them) */
extern int data_0040F628;
extern int data_0040F630;
extern int data_0040F638;
extern float data_003E5718;
extern float data_003E5720;
/* the box's inner bottom, SetupBackgroundBox's local static, and its guard */
extern signed char data_00412E80;
extern float data_003E5728;

namespace {
/* the pop-up's frame: its nine pieces loaded and UV'd from one texture (SOCOM 1's, the bottom now
 * the spectators' box's), and the sixteen bitmaps the tick places later.
 * OPEN, 2 instructions: the bottom-right corner's SetUV loads the corner u before v, where MW
 * schedules v's load first (it feeds the `1 - v`), as the target itself does for the bottom-left
 * corner; a local u or v, `-v + 1`, a local 1.0f all keep MW's order. Each corner is a block of four
 * locals (x0, y0, x1, y1 declared in that order give f23-f20; one set of locals reused across the
 * corners re-colours the second corner) */
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

        data_0040E4A0.Load(x0, y0, x1, y1, zdb::CAssetMgr::m_assets.GetTexHandle(s_specboxtex));
        data_0040E4A0.SetUV(1.0f, 0.0f, data_003E5718, data_003E5720);
    }
    {
        float x0 = (float)data_0040F630 - 20.0f;
        float y0 = (float)data_0040F638;
        float x1 = (float)data_0040F630;
        float y1 = 20.0f + (float)data_0040F638;

        data_0040E530.Load(x0, y0, x1, y1, zdb::CAssetMgr::m_assets.GetTexHandle(s_specboxtex));
        data_0040E530.SetUV(data_003E5718, 0.0f, 1.0f, data_003E5720);
    }
    {
        float x0 = (float)data_0040F628;
        float y0 = s_specboxbottom - 20.0f;
        float x1 = 20.0f + (float)data_0040F628;
        float y1 = s_specboxbottom;

        data_0040E5C0.Load(x0, y0, x1, y1, zdb::CAssetMgr::m_assets.GetTexHandle(s_specboxtex));
        data_0040E5C0.SetUV(1.0f, 1.0f - data_003E5720, data_003E5718, 1.0f);
    }
    {
        float x0 = (float)data_0040F630 - 20.0f;
        float y0 = s_specboxbottom - 20.0f;
        float x1 = (float)data_0040F630;
        float y1 = s_specboxbottom;

        data_0040E650.Load(x0, y0, x1, y1, zdb::CAssetMgr::m_assets.GetTexHandle(s_specboxtex));
        data_0040E650.SetUV(data_003E5718, 1.0f - data_003E5720, 1.0f, 1.0f);
    }

    float left = 20.0f + (float)data_0040F628;
    float right = (float)data_0040F630 - 20.0f;
    float top = 20.0f + (float)data_0040F638;
    zdb::CTexHandle *tex = zdb::CAssetMgr::m_assets.GetTexHandle(s_specboxtex);

    data_0040E6E0.Load(left, (float)data_0040F638, right, top, tex);
    data_0040E6E0.SetUV(0.5f, 0.0f, 0.5f, data_003E5720);
    data_0040E770.Load((float)data_0040F628, top, left, data_003E5728, tex);
    data_0040E770.SetUV(1.0f, 0.5f, data_003E5718, 0.5f);
    data_0040E800.Load(right, top, (float)data_0040F630, data_003E5728, tex);
    data_0040E800.SetUV(data_003E5718, 0.5f, 1.0f, 0.5f);
    data_0040E890.Load(left, data_003E5728, right, s_specboxbottom, tex);
    data_0040E890.SetUV(0.5f, 1.0f - data_003E5720, 0.5f, 1.0f);
    data_0040E920.Load(left, top, right, data_003E5728, tex);
    data_0040E920.SetUV(0.5f, 0.5f, 0.5f, 0.5f);
    for (unsigned int i = 0; i < 16; i++) {
        data_0040DC80[i].Load(0.0f, 0.0f, 0.0f, 0.0f, tex);
        data_0040DC80[i].SetUV(0.5f, 0.5f, 0.5f, 0.5f);
    }
}
}

/* a tag's first two numbers as integers, 0 for what is missing (the message loader's reader, called
 * twice by 0x22D660; the name ours) */
IPNT2D GetTagIPNT2D(const _zrdr *rdr, const char *tag)
{
    _zrdr *t = zrdr_findtag(rdr, tag);
    _zrdr *a = t ? t->Item(0) : 0;
    int x = a ? a->Int() : 0;
    _zrdr *b = t ? t->Item(1) : 0;
    int y = b ? b->Int() : 0;

    return IPNT2D(x, y);
}

/* the message pop-up: its two bitmaps and its items (the object itself holds nothing) */
void CHudPopups::Draw(zdb::CCamera *cam)
{
    if (data_0040E480) {
        data_0040F2D0.Draw(cam);
        data_0040F220.Draw(cam);
        for (int i = 0; i < data_00412D70.size(); i++)
            data_00412D70[i]->Draw(cam);
    }
}

/* everything drawn while the pop-up is up */
void CScorePopup::Draw(zdb::CCamera *cam)
{
    if (!data_0040F608)
        return;
    data_0040F670._bmpTitleBg.Draw(cam);
    data_004111C0._bmpTitleBg.Draw(cam);
    data_0040E4A0.Draw(cam);
    data_0040E530.Draw(cam);
    data_0040E5C0.Draw(cam);
    data_0040E650.Draw(cam);
    data_0040E6E0.Draw(cam);
    data_0040E770.Draw(cam);
    data_0040E800.Draw(cam);
    data_0040E890.Draw(cam);
    data_0040E920.Draw(cam);
    data_0040F380.Draw(cam);
    data_0040EAF0.Draw(cam);
    data_0040E9B0.Draw(cam);
    data_0040EA40.Draw(cam);
    for (unsigned int i = 0; i < 16; i++)
        data_0040DC80[i].Draw(cam);
    for (unsigned int i = 0; i < 45; i++)
        data_0040F670._arrStrs[i].Draw(cam);
    for (unsigned int i = 0; i < 45; i++)
        data_004111C0._arrStrs[i].Draw(cam);
    data_0040F410.Draw(cam);
    data_0040F4C0.Draw(cam);
    data_0040F570.Draw(cam);
    data_0040ECC0.Draw(cam);
    data_0040EB80.Draw(cam);
    data_0040EC10.Draw(cam);
    for (unsigned int i = 0; i < 8; i++)
        data_0040ED50[i].Draw(cam);
    data_0040DBF0.Draw(cam);
}
