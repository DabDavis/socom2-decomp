/* hud_spectatorinfo.cpp -- the network game's spectator box (OVL1 0x2425E0-0x2430EB): SOCOM II's unit
 * (no SOCOM 1 counterpart; the file name and every name in it are ours), after the health bar's
 * (hud_healthbar.cpp) and before the heap library's: CSpectatorInfo's Tick (slot 14, 0x2425E0), Off
 * and On (slots 6 and 7), the strings wiped (0x242AB0: CHUD::ReInit_Uninit's) and emptied
 * (0x242B20: CHUD::ReInit_Init's), Init (0x242BB0), the destructor and the constructor. Its
 * initialiser (0x3FFA50) builds the box's colours and the values' column top. Defined in reverse of
 * their image order (-inline deferred). */

#include "../../gamez/ztwod/ztwod.h"

#include <stdio.h>
#include <string.h>

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

class C2DFont;

/* a round's and a game's stats (zseal.h's CSealStats, 0x48 bytes: only what the box reads) */
class CSealStats {
public:
    unsigned short m_headshots;   // 0x0
    unsigned char _2[0xc - 0x2];
    unsigned short m_kills;   // 0xc
    unsigned char _e[0x12 - 0xe];
    unsigned short m_deaths;   // 0x12
    unsigned char _14[0x3c - 0x14];
    short m_score;   // 0x3c
    unsigned char _3e[0x48 - 0x3e];
};

/* the SEAL the HUD follows, as the box reads it */
class CZSealBody {
public:
    unsigned char _0[0x14];
    const char *m_name;   // 0x14
    unsigned char _18[0xc8 - 0x18];
    unsigned int m_c8;   // 0xc8: bit 17 a spectator's flag (the name ours)
    unsigned char _cc[Z_SEALBODY_OFS(0x544) - 0xcc];
    CSealStats m_stats[2];   // 0x544: the game's and the round's
    unsigned char _5d4[Z_SEALBODY_OFS(0xfc8) - Z_SEALBODY_OFS(0x5d4)];
    int m_player;   // 0xfc8
};

CZSealBody *ftsGetPlayer();

/* whether a SEAL is on each side (the SEAL lane's, no names yet) and whether the local player is
 * spectating (no name yet) */
extern "C" {
bool func_002C2FA0();
bool func_002C31D0(const CZSealBody *seal);
bool func_002C30B0(const CZSealBody *seal);
}

class CHUD {
public:
    static CZSealBody *GetHUDOwner();

    void Add2D(C2D *p2d, int phase, bool add_to_back);
};

extern CHUD *theHUD;

class CZPersonaState {
public:
    static const char *GetPersonaClanTag(int client);
};

/* the app camera as the box reads it: a part at 0xF4 whose word at 0x20 names the view the spectator
 * watches through (camera.h has 0xF4-0x117 as SOCOM II's death-camera words; the names ours) */
class CCameraView {
public:
    unsigned char _0[0x20];
    const char *m_name;   // 0x20
};

class CAppCamera {
public:
    unsigned char _0[0xf4];
    CCameraView m_view;   // 0xf4
};

extern CAppCamera *appCamera;

/* the camera's view, none without a camera (an inline's ternary: `p ? p + 0xF4 : 0` with the caller's
 * own test of the result after it, unthreaded -- a derived-to-base conversion in its place, inline or
 * not, is threaded into the caller's test, 8 bytes short) */
static inline CCameraView *GetCameraView()
{
    return appCamera ? &appCamera->m_view : 0;
}

class CSpectatorInfo : public C2D {
public:
    CSpectatorInfo();
    virtual ~CSpectatorInfo();
    virtual void Off();
    virtual void On();
    virtual void Tick(float dT);
    void Init(C2DFont *font);
    void Empty();
    void Wipe();

    unsigned char _18[0x20 - 0x18];
    C2DBitmap m_back;   // 0x20: the name's backing, its colour the owner's side
    C2DString m_name;   // 0xa0
    C2DBitmap m_statsback;   // 0x140: the stats' backing, shown for a side's SEAL
    C2DString m_labels[4];   // 0x1c0
    C2DString m_values[4];   // 0x420
};   // 0x680 bytes

/* the box's places (plain data, 8 bytes apart: MW 3.0's layout of function-local statics, written as
 * file statics), then the colours and the values' column top its initialiser builds */
static const char *s_spectex = "newweapnbkrnd.tif";
static float s_specx = 10.0f;
static float s_specy = 410.0f;
static float s_specw = 164.0f;
static float s_spech = 28.0f;
static float s_spectrans = 72.0f;
static float s_namescale = 0.9f;
static float s_namex = 20.0f;
static float s_namey = 430.0f;
static float s_statsx = 10.0f;
static float s_statsy = 326.0f;
static float s_statsw = 164.0f;
static float s_statsh = 84.0f;
static float s_statstrans = 64.0f;
static float s_linescale = 0.9f;
static float s_linedy = 18.0f;
static float s_labelx = 20.0f;
static float s_labely = 346.0f;
static float s_valuex = 160.0f;

static CPnt3D s_specColor(0.0f, 0.0f, 0.0f);
static CPnt3D s_statsColor(128.0f, 128.0f, 128.0f);
static float s_valuey = s_labely;
static CPnt3D s_team0Color(32.0f, 32.0f, 64.0f);
static CPnt3D s_team1Color(64.0f, 32.0f, 32.0f);
static CPnt3D s_otherColor(128.0f, 128.0f, 64.0f);
static CPnt3D s_selfColor(64.0f, 128.0f, 64.0f);

CSpectatorInfo::CSpectatorInfo()
{
}

CSpectatorInfo::~CSpectatorInfo()
{
}

/* the box made: the backing and the name over it, the stats' backing and its four lines, a label and
 * a value each, listed under it; the box added to the HUD */
void CSpectatorInfo::Init(C2DFont *font)
{
    zdb::CTexHandle *tex = zdb::CAssetMgr::m_assets.GetTexHandle(s_spectex);

    m_back.Load(s_specx, s_specy, s_specx + s_specw, s_specy + s_spech, tex);
    m_back.SetColor(s_specColor);
    m_back.SetTrans(s_spectrans);
    push_back(&m_back);
    m_name.Load(s_namescale, "SPECTATOR INFO", font, 0.0f, 0.0f, false);
    m_name.SetPos((int)s_namex, (int)s_namey);
    m_name.On();
    m_back.push_back(&m_name);
    m_statsback.Load(s_statsx, s_statsy, s_statsx + s_statsw, s_statsy + s_statsh, tex);
    m_statsback.SetColor(s_statsColor);
    m_statsback.SetTrans(s_statstrans);
    push_back(&m_statsback);
    for (unsigned int i = 0; i < 4; i++) {
        m_labels[i].Load(s_linescale, "INFO TEXT", font, 0.0f, 0.0f, false);
        m_labels[i].SetPos((int)s_labelx, (int)(s_labely + (float)i * s_linedy));
        m_labels[i].On();
        m_statsback.push_back(&m_labels[i]);
        m_values[i].Load(s_linescale, "000", font, 0.0f, 0.0f, false);
        m_values[i].SetPos((int)s_valuex, (int)(s_valuey + (float)i * s_linedy));
        m_values[i].On();
        m_statsback.push_back(&m_values[i]);
        m_values[i].m_socom2_6a = 1;
        m_values[i].UpdatePos();
    }
    theHUD->Add2D(this, 1, true);
}

/* every string emptied */
void CSpectatorInfo::Empty()
{
    m_name.SetString(0, true);
    for (unsigned int i = 0; i < 4; i++) {
        m_labels[i].SetString(0, true);
        m_values[i].SetString(0, true);
    }
}

/* every string cleared and wiped */
void CSpectatorInfo::Wipe()
{
    m_name.ClearAndWipe();
    for (unsigned int i = 0; i < 4; i++) {
        m_labels[i].ClearAndWipe();
        m_values[i].ClearAndWipe();
    }
}

/* shown only while the local player spectates */
void CSpectatorInfo::On()
{
    if (func_002C2FA0()) {
        C2D::On();
        m_back.On();
        m_statsback.On();
        Tick(0.0f);
    }
}

void CSpectatorInfo::Off()
{
    C2D::Off();
}

/* the followed SEAL's name (the clan's tag ahead of it), its side's colour and, for a side's SEAL,
 * its stats; following no one, the camera's view, or FREE CAMERA */
void CSpectatorInfo::Tick(float dT)
{
    CZSealBody *owner = CHUD::GetHUDOwner();
    bool stats = false;
    bool freecam = false;

    if (owner == ftsGetPlayer()) {
        m_back.SetColor(s_selfColor);
        m_statsback.Off();
        CCameraView *view = GetCameraView();

        if (view) {
            const char *name = view->m_name;

            if (name && strlen(name))
                m_name.SetString(name, true);
            else
                freecam = true;
        } else
            freecam = true;
    } else if (owner) {
        m_back.On();
        m_name.On();
        const char *name = owner->m_name;
        const char *clan = CZPersonaState::GetPersonaClanTag(owner->m_player);
        char text[64];

        if (!name)
            name = " ";
        if (clan && strlen(clan) > 1)
            sprintf(text, "[%s] %s", clan, name);
        else
            strcpy(text, name);
        m_name.SetString(text, true);
        if (func_002C31D0(owner)) {
            m_back.SetColor(s_team0Color);
            m_statsback.On();
            stats = true;
        } else if (func_002C30B0(owner)) {
            m_back.SetColor(s_team1Color);
            m_statsback.On();
            stats = true;
        } else if (owner->m_c8 & 0x20000) {
            m_back.SetColor(s_otherColor);
            m_statsback.Off();
        } else {
            m_back.SetColor(s_selfColor);
            m_statsback.Off();
            CCameraView *view = GetCameraView();

            if (view) {
                const char *name = view->m_name;

                if (name && strlen(name))
                    m_name.SetString(name, true);
            }
        }
    }
    if (stats) {
        char num[48];
        CSealStats *game = &owner->m_stats[0];
        CSealStats *round = &owner->m_stats[1];

        m_labels[0].SetString("Kill Count :", true);
        sprintf(num, "%d", game->m_kills + round->m_kills);
        m_values[0].SetString(num, true);
        m_labels[1].SetString("Deaths :", true);
        sprintf(num, "%d", game->m_deaths + round->m_deaths);
        m_values[1].SetString(num, true);
        m_labels[2].SetString("Headshots :", true);
        sprintf(num, "%d", game->m_headshots + round->m_headshots);
        m_values[2].SetString(num, true);
        m_labels[3].SetString("Score :", true);
        sprintf(num, "%d", game->m_score + round->m_score);
        m_values[3].SetString(num, true);
    }
    if (freecam) {
        m_name.SetString("FREE CAMERA", true);
        m_back.SetColor(100.0f, 100.0f, 100.0f);
    }
}
