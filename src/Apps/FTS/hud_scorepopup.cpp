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
};

extern CHUD *theHUD;
class CZNetwork {
public:
    unsigned char m_bSessionMaster;   // 0x0
    unsigned char m_bNetwork;   // 0x1
};

extern CZNetwork theNetwork;

const char *GetDatabase();

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
