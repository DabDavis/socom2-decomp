/* hud_missioncams.cpp -- the HUD's mission cameras (OVL1 0x228ED0-0x229C9F): SOCOM 1's unit (its
 * DWARF names it, Z:\dev\Apps\FTS\hud_missioncams.cpp), the same functions in the same order,
 * from IsPlaying to the constructor; HudTick written (the HUD's round 40), PlayCamera(CMissionCam *,
 * bool) -- SOCOM II's 0x54C against SOCOM 1's 0x3DC -- and CleanupCamera (round 51): the unit whole.
 * Defined in reverse of their image order (-inline deferred). */

#include <string.h>

#include "hud_missioncams.h"
#include "../../gamez/zinput/zinput.h"

/* a named value (zanim.h's CValve: the mission's abort valve, its 16-bit value at 4) */
class CValve {
public:
    char *m_name;   // 0x0
    unsigned short m_value;   // 0x4
};

class CMission {
public:
    CMissionCam *GetMissionCamByIdentifier(const char *id);

    unsigned char _0[0x1ac];
    CValve *m_valve_abort;   // 0x1ac: the "mission_abort" valve (fts_state.h's; the game states' round 41:
                             // at 1 or 2 no film starts)
};

extern CMission theMission;

/* the HUD's owner's controller, its input bit (zseal.h's CSealCtrl: m_input_enabled, bit 11 of the
 * flags at 0x170), and the menu state put back (OVL2 0x597740, SOCOM II's with a flag) */
class CSealCtrl {
public:
    void SetMenuState(MENU_STATE state, bool flag);
    /* the menu state (SOCOM 1's GetMenuState, out of line there: SOCOM II reads it inline) */
    MENU_STATE GetMenuState() const { return m_menustate; }
    /* the input flushed and the heartbeat stopped (OVL2 0x592AC0, SOCOM 1's) */
    void StopRumbler();
    /* the controllers switched on or off (OVL2 0x598840, a byte flag set when it changes: the SEALs'
     * round 55 named the row, seal_control.cpp's first definition) */
    static void SetEnabled(bool on);

    unsigned char _0[0x170];
    unsigned int m_flags0 : 11;
    unsigned int m_input_enabled : 1;
    unsigned char _172[0x221 - 0x172];
    MENU_STATE m_menustate;   // 0x221
};

/* the AI's switch (OVL2 0x574830) */
class CSealCtrlAi {
public:
    static void SetAiEnabled(bool on, bool flag);
};

/* the seal's kit: the lens effects (OVL2 0x5C1800) and the heartbeat sensor's switch (kit.h's
 * m_heartbeat_enabled, bit 15 of the flags at 0) */
class CZKit {
public:
    void Apply_LensEffects(bool on, CPnt3D *color);

    unsigned int m_bits0 : 15;
    unsigned int m_heartbeat_enabled : 1;
};

/* zseal.h's (SOCOM 1's DWARF; a byte enum under -enum min) */
enum ZOOMSTATE {
    ZOOMSTATE_UNKNOWN = -1, ZOOMSTATE_3RDPERSON = 0, ZOOMSTATE_1STPERSON = 1, ZOOMSTATE_THERMAL = 2,
    ZOOMSTATE_NVG = 3, ZOOMSTATE_BINOCS = 4, ZOOMSTATE_WEAPONDEF1 = 5, ZOOMSTATE_WEAPONDEF2 = 6,
    ZOOMSTATE_WEAPONDEF3 = 7, ZOOMSTATE_WEAPONDEF4 = 8, ZOOMSTATE_WEAPONDEF5 = 9, ZOOMSTATE_WEAPONDEF6 = 10,
    ZOOMSTATE_WEAPONDEF7 = 11, ZOOMSTATE_WEAPONDEF8 = 12, NUM_ZOOMSTATES = 13
};

/* the seal: its controller, the bit the HUD tests before a film (0xE1 bit 4, as hud_ammo.cpp's), its
 * zoom state (SOCOM 1's m_zoomstate, 0x160 there) and its kit */
class CZSealBody {
public:
    unsigned char _0[0xc0];
    CSealCtrl *m_ctrl;   // 0xc0
    unsigned char _c4[0xe1 - 0xc4];
    unsigned char _e1_0 : 4;
    unsigned char m_socom2_e1_4 : 1;   // 0xe1 bit 4
    unsigned char _e2[Z_SEALBODY_OFS(0x200) - 0xe2];
    ZOOMSTATE m_zoomstate;   // 0x200
    unsigned char _201[Z_SEALBODY_OFS(0x5e0) - Z_SEALBODY_OFS(0x201)];
    CZKit m_kit;   // 0x5e0
};

CZSealBody *ftsGetPlayer();

/* the letterbox's bars taken away (hud_letterbox.cpp's) */
class CLetterBox {
public:
    void AddLetterbox();
    void RemoveLetterbox();
};

/* the HUD: its owner, its letterbox and its menu state (SOCOM II's offsets), shown again by On */
class CHUD {
public:
    static CZSealBody *GetHUDOwner();
    void Off();
    void On();
    bool MenuUp() const { return m_menustate != stateNone; }
    /* the HUD's font (inline: a description's text is chosen after it is read, the target's order;
     * the name ours) */
    C2DFont *GetFont() { return m_myfont; }

    unsigned char _0[Z_CHUD_OFS(0x498)];
    C2DFont *m_myfont;   // 0x498
    unsigned char _49c[Z_CHUD_OFS(0x1a100) - Z_CHUD_OFS(0x49c)];
    CLetterBox m_letterbox;   // 0x1a100
    unsigned char _1a101[Z_CHUD_OFS(0x1a220) - Z_CHUD_OFS(0x1a101)];
    MENU_STATE m_menustate;   // 0x1a220
};

extern CHUD *theHUD;

/* the sound (zsnd.h's CSnd: the zdb world's round 21 named its statics) -- a running group fade
 * ended at once (0x341060) and a group's master volume read and set straight (0x3414B0, 0x3414C0:
 * tail calls to snd_GetMasterVolume and snd_SetMasterVolume) */
class CSnd {
public:
    static void EndGroupVolTemp();
    static int GetMasterVolume(int group);
    static void SetMasterVolume(int group, int vol);
};

/* the camera's mission film: its music at 0x18 (SOCOM 1's DWARF: identifier, name, three
 * descriptions, the animation, the music) */

class CMissionCam {
public:
    char *m_identifier;   // 0x0
    char *m_name;   // 0x4
    char *m_description[3];   // 0x8
    CZAnim *m_anim;   // 0x14
    CSnd *m_music;   // 0x18
};

/* the jukebox and the paused voices (zsnd's) */
enum MUSIC_MODE {
    MUSIC_MODE_0 = 0,
    MUSIC_MODE_3 = 3
};

class CSndJukebox {
public:
    static void Play(MUSIC_MODE mode, CSnd *snd);
    static void ClearQueue();
    static void FadeOut();
};

enum SOUND_MODE {
    SND_MODE_UNKNOWN = 0,
    SND_MODE_EFX = 1,
    SND_MODE_MUSIC = 2,
    SND_MODE_DIALOG = 3,
    SND_MODE_BANK_DEFINED = 4
};

class CSndInstance {
public:
    static void PauseAllSounds(SOUND_MODE mode);
    static void ResumeAllSounds(SOUND_MODE mode);
};

/* the score pop-up is up (hud_scorepopup.cpp's) */
bool hudIsScorePopupOn();

/* the voice recogniser (reco.cpp's theReco) suspended */
class CReco {
public:
    void Suspend(bool now);
};

extern CReco theReco;

/* the weapons' ticks switched off (zweapon.h's SOCOM 1 static, OVL1 bss 0x4B52B0: set here by the
 * film start and cleared by its end, as SOCOM 1's CleanupCamera did) */
class CZWeapon {
public:
    static bool m_weaponTickDisabled;
};

/* OVL2 0x592CA0 (on the player's controller): its name not known */
extern "C" void func_00592CA0(CSealCtrl *ctrl);

/* the lens effects (thezoom.cpp's CameraZoom, SOCOM II's) */
class CameraZoom {
public:
    static void HoldLensFx(unsigned char hold);
    static void ReleaseLensFx(unsigned char hold);
    static void SetLensFx(unsigned char fx);
    static unsigned char LensFxOn();
};

/* the world's night-vision and starlight-scope lens colours (znode.h's, SOCOM 1's CPnt3Ds grown to
 * four floats) */
namespace zdb {
class CWorld {
public:
    static CWorld *m_world;

    unsigned char _0[Z_OFS(0x5dc, 0x5e8)];   /* the host's: CNode's vptr first, its 16-aligned base after */
    unsigned char m_night_mission;   // 0x5dc (host 0x5e8)
    unsigned char _5dd[3];
    PNT4D m_lensfx_nvg;   // 0x5e0
    PNT4D m_lensfx_starlightscope;   // 0x5f0
};
}

/* the zoom state and the camera's zoom a film put aside, given back at its end (SOCOM 1's file
 * statics, by its symbols: cur_zoomstate gp-30240, cur_camzoom gp-30236; OVL1 .data 0x3DC888 and
 * 0x3DC890) */
static ZOOMSTATE cur_zoomstate = ZOOMSTATE_UNKNOWN;
static float cur_camzoom = 1.0f;

/* the game state the skip tells to skip its buffer swaps (fts_state.h's CGameState: SetSkipSwap its
 * slot 0x28; theGame's core state, theGame + 0x110) */
class CGameState {
public:
    virtual ~CGameState();
    virtual void PreInit();
    virtual bool Init();
    virtual bool Tick(float dT);
    virtual void PreUnInit();
    virtual void UnInit();
    virtual void OnPush(int userdata);
    virtual void OnPop(int userdata);
    virtual void SetSkipSwap(int swap);
};

class COurGame {
public:
    unsigned char _0[Z_OFS(0x110, 0x120)];   /* the host's: fts_state.h's COurGame, CGame 0x80 bytes, CCoreState 16-aligned */
    CGameState m_coreState;   // 0x110 (host 0x120)
};

extern COurGame theGame;

/* the camera's animation stopped (zanim_anim.cpp's, out of line) and the camera's regions */
extern "C" void func_002707C0(CZAnim *anim, bool now);

class CZAnim {
public:
    bool Running();
    void Refresh(bool all);
    CZAnim *ActivateArg(int arg, ...);
};

/* the camera's view, its zoom at 0x470 (SOCOM 1's at appCamera+0x34, 0x444) */
struct CAppCameraView {
    unsigned char _0[0x470];
    float m_zoom;   // 0x470
};

class CAppCamera {
public:
    void UpdateCameraRegions();
    void SetZoom(float zoom);

    unsigned char _0[0xb4];
    CAppCameraView *m_view;   // 0xb4
};

extern CAppCamera *appCamera;

/* a string's text from its first character shown */
static inline const char *ShownText(C2DString &s)
{
    return s.m_string + s.m_firstCharPos;
}

CZHudMissionCams::CZHudMissionCams()
{
    m_currentcam = 0;
    m_animhandle = 0;
    m_pause_for_frame = 0;
}

CZHudMissionCams::~CZHudMissionCams()
{
}

/* the strings shown in the letterbox listed as the HUD's children, each off (the list's end node,
 * this + 4, hoisted out of the loop: include/list's push_back builds its iterator from the end
 * node's reference accessor) */
void CZHudMissionCams::Init()
{
    push_back(&m_letterboxobjectivename);
    m_letterboxobjectivename.Off();
    for (int i = 0; i < 3; i++) {
        push_back(&m_letterboxobjectivedescription[i]);
        m_letterboxobjectivedescription[i].Off();
    }
    push_back(&m_crosstext);
    m_crosstext.Off();
}

void CZHudMissionCams::UnInit()
{
    for (int i = 0; i < 3; i++)
        m_letterboxobjectivedescription[i].ClearAndWipe();
    m_letterboxobjectivename.ClearAndWipe();
    m_crosstext.ClearAndWipe();
    m_lastmenustate = stateNone;
}

/* while a camera plays: cross (released) skips it while the owner's input is on -- the animation
 * stopped, the core state's swaps and the letterbox held three frames -- the held frames counted
 * down, the camera cleaned up once its animation ends, else, the menu down, the letterbox's strings
 * with text shown and the camera's regions updated (SOCOM 1's, 0x1CC there: SOCOM II's skip is the
 * cross alone and tells the core state, SOCOM 1 set the menu state). The target's shapes: the three
 * cases an if/else chain (returns branch to the epilogue without the target's join nops), the two
 * out-of-line bool calls compared `== false` (their `andi`), the input bit `(bool)... != false`
 * (both its `sltu`s) and the menu test an inline bool `== false` (its `sltu`) */
void CZHudMissionCams::HudTick(CPad *pad, float dT)
{
    if (!m_animhandle)
        return;

    CZSealBody *owner = CHUD::GetHUDOwner();
    CSealCtrl *ctrl = owner ? owner->m_ctrl : 0;
    bool input = ctrl ? (bool)ctrl->m_input_enabled != false : false;

    if (pad->m_state[PAD_CROSS] == KEY_RISING && input) {
        if (m_animhandle)
            func_002707C0(m_animhandle, false);
        theGame.m_coreState.SetSkipSwap(3);
        m_pause_for_frame = 3;
    }
    if (m_pause_for_frame > 0) {
        m_pause_for_frame--;
    } else if (m_animhandle->Running() == false) {
        CleanupCamera(pad);
    } else {
        if (m_letterboxobjectivename.IsOn() == false && theHUD->MenuUp() == false) {
            if (ShownText(m_letterboxobjectivename))
                m_letterboxobjectivename.On();
            for (int i = 0; i < 3; i++) {
                const char *text = ShownText(m_letterboxobjectivedescription[i]);

                if (text && strlen(text) > 1) {
                    m_letterboxobjectivedescription[i].On();
                    m_socom2_280.On();
                }
            }
            m_crosstext.On();
        }
        appCamera->UpdateCameraRegions();
    }
}

/* a film's end: the letterbox's strings off, the animation forgotten, the master volume given back,
 * the film's music faded and its queue cleared, the dialogue voices resumed, the weapons' tick, the
 * AI and the menu state given back; the lens effect put aside at the start put up again unless one
 * is up (night vision or the starlight scope with the world's colour), the camera's zoom given back,
 * the lens effects released, the heartbeat sensor on, the player's controller told, the core state
 * skipping a swap, the letterbox down and the HUD on (SOCOM 1's, 0x204 there: SOCOM II adds the
 * volume, the voices, the lens checks and the player's controller; the pad no longer flushed).
 * The registers by MW's colouring (tools/igreplay.py): the kit numbered before the loop's counter --
 * a dead first definition at the top (the world's round-12 rule; declared there uninitialised it is
 * numbered at its assignment, after the owner: this, the counter and the kit then take s2/s1/s0 for
 * the target's s1/s2/s0) -- and the world read into a local ahead of the lens chain (s2, loaded in
 * the first test's delay slot; read in each case it is loaded there instead). `> 3` keeps the
 * target's slti into at, `>= 4` computes into v0 */
void CZHudMissionCams::CleanupCamera(CPad *pad)
{
    CZKit *kit = 0;

    m_letterboxobjectivename.Off();
    m_socom2_280.Off();
    for (int i = 0; i < 3; i++)
        m_letterboxobjectivedescription[i].Off();
    m_animhandle = 0;
    m_crosstext.Off();
    CSnd::SetMasterVolume(1, m_socom2_3b8);
    if (m_currentcam->m_music) {
        CSndJukebox::FadeOut();
        CSndJukebox::ClearQueue();
    }
    CSndInstance::ResumeAllSounds(SND_MODE_DIALOG);
    CZWeapon::m_weaponTickDisabled = false;
    CSealCtrlAi::SetAiEnabled(true, true);
    CSealCtrl::SetEnabled(true);

    CZSealBody *owner = CHUD::GetHUDOwner();

    owner->m_ctrl->SetMenuState(m_lastmenustate, false);
    kit = &owner->m_kit;

    if (!CameraZoom::LensFxOn() && kit && cur_zoomstate != -1) {
        zdb::CWorld *world = zdb::CWorld::m_world;

        if (cur_zoomstate == 2) {
            if (!CameraZoom::LensFxOn())
                kit->Apply_LensEffects(false, 0);
            CameraZoom::SetLensFx(2);
        } else if (cur_zoomstate == 3) {
            if (!CameraZoom::LensFxOn()) {
                PNT4D color = world->m_lensfx_nvg;

                kit->Apply_LensEffects(true, (CPnt3D *)&color);
            }
            CameraZoom::SetLensFx(3);
        } else if (cur_zoomstate > 3) {
            if (!CameraZoom::LensFxOn()) {
                PNT4D color = world->m_lensfx_starlightscope;

                kit->Apply_LensEffects(true, (CPnt3D *)&color);
            }
            CameraZoom::SetLensFx(4);
        }
        cur_zoomstate = ZOOMSTATE_UNKNOWN;
    }
    if (cur_camzoom != 1.0f) {
        appCamera->SetZoom(cur_camzoom);
        cur_camzoom = 1.0f;
    }
    CameraZoom::ReleaseLensFx(1);
    if (kit)
        kit->m_heartbeat_enabled = true;

    CZSealBody *player = ftsGetPlayer();
    CSealCtrl *ctrl = player ? player->m_ctrl : 0;

    if (ctrl)
        func_00592CA0(ctrl);
    m_currentcam = 0;
    theGame.m_coreState.SetSkipSwap(1);
    theHUD->m_letterbox.RemoveLetterbox();
    theHUD->On();
}

/* the letterbox's strings drawn while a camera plays */
void CZHudMissionCams::Draw(zdb::CCamera *theCamera)
{
    if (IsPlaying()) {
        m_letterboxobjectivename.Draw(theCamera);
        for (int i = 0; i < 3; i++)
            m_letterboxobjectivedescription[i].Draw(theCamera);
        m_socom2_280.Draw(theCamera);
        m_crosstext.Draw(theCamera);
    }
}

/* a camera of the mission's, by its identifier, played with the lens swap */
void CZHudMissionCams::PlayCamera(const char *camname)
{
    CMissionCam *cam = theMission.GetMissionCamByIdentifier(camname);

    if (cam)
        PlayCamera(cam, true);
}

/* a film started: none while one plays, the HUD's owner is down, the film block's halfword is 1 or 2,
 * the score pop-up is up or the player is down; the held frames cleared and the core state skipping
 * three swaps, the film's animation refreshed and started (else nothing), the film kept, the master
 * volume kept and capped at 768, the film's music played, the dialogue voices paused, the letterbox
 * up, the HUD off, the weapons' tick, the AI and 0x598840 off; the film's name, a mark with the first
 * description, the three descriptions (a space for a missing one) and the cross text loaded and
 * coloured; the camera's zoom kept and set to 1, the zoom state kept and its lens effect taken off
 * when the lens swaps on a night mission, the kit's lens effects off, the lens effects held, the
 * heartbeat sensor off, the rumble stopped, the recogniser suspended and the menu state kept and set
 * to the film's (SOCOM 1's, 0x3DC there) */
void CZHudMissionCams::PlayCamera(CMissionCam *missioncam, bool bSwapLens)
{
    if (!missioncam)
        return;
    if (m_currentcam)
        return;

    CZSealBody *seal = CHUD::GetHUDOwner();

    if (!seal || !seal->m_socom2_e1_4)
        return;
    if (theMission.m_valve_abort && (theMission.m_valve_abort->m_value == 1 || theMission.m_valve_abort->m_value == 2))
        return;
    if (hudIsScorePopupOn())
        return;

    CZSealBody *player = ftsGetPlayer();

    if (player && !player->m_socom2_e1_4)
        return;
    m_pause_for_frame = 0;
    theGame.m_coreState.SetSkipSwap(3);

    CZAnim *anim = missioncam->m_anim;

    if (!anim)
        return;
    anim->Refresh(false);
    m_animhandle = anim->ActivateArg(0, 0);
    if (!m_animhandle)
        return;
    m_currentcam = missioncam;
    CSnd::EndGroupVolTemp();
    m_socom2_3b8 = CSnd::GetMasterVolume(1);
    if (m_socom2_3b8 > 768)
        CSnd::SetMasterVolume(1, 768);

    CSnd *music = missioncam->m_music;

    if (music) {
        CSndJukebox::FadeOut();
        CSndJukebox::ClearQueue();
        CSndJukebox::Play(MUSIC_MODE_3, music);
    }
    CSndInstance::PauseAllSounds(SND_MODE_DIALOG);
    theHUD->m_letterbox.AddLetterbox();
    theHUD->Off();
    CZWeapon::m_weaponTickDisabled = true;
    CSealCtrlAi::SetAiEnabled(false, false);
    CSealCtrl::SetEnabled(false);

    m_letterboxobjectivename.Off();
    m_letterboxobjectivename.Load(1.2f, missioncam->m_name, theHUD->m_myfont, 0.0f, 0.0f, true);
    m_letterboxobjectivename.m_color[0] = 128.0f;
    m_letterboxobjectivename.m_color[1] = 128.0f;
    m_letterboxobjectivename.m_color[2] = 128.0f;
    m_letterboxobjectivename.m_color[3] = 80.0f;
    m_letterboxobjectivename.SetPos(40, 54);
    m_socom2_280.Off();
    if (missioncam->m_description[0]) {
        m_socom2_280.Load(0.9f, "\xa7", theHUD->m_myfont, 0.0f, 0.0f, true);
        m_socom2_280.SetPos(20, 405);
        m_socom2_280.m_color[0] = 128.0f;
        m_socom2_280.m_color[1] = 128.0f;
        m_socom2_280.m_color[2] = 128.0f;
        m_socom2_280.m_color[3] = 80.0f;
    }
    for (int ii = 0; ii < 3; ii++) {
        const char *desc = missioncam->m_description[ii];

        m_letterboxobjectivedescription[ii].Off();
        m_letterboxobjectivedescription[ii].Load(0.9f, desc ? desc : " ", theHUD->GetFont(), 0.0f, 0.0f,
                                                 true);
        m_letterboxobjectivedescription[ii].SetPos(40, (int)(403.0f + 16.0f * ii));
        m_letterboxobjectivedescription[ii].m_color[0] = 128.0f;
        m_letterboxobjectivedescription[ii].m_color[1] = 128.0f;
        m_letterboxobjectivedescription[ii].m_color[2] = 128.0f;
        m_letterboxobjectivedescription[ii].m_color[3] = 80.0f;
    }
    m_crosstext.Off();
    m_crosstext.Load(0.8f, "\xa6 TO ABORT", theHUD->m_myfont, 520.0f, 54.0f, true);
    m_crosstext.m_color[0] = 128.0f;
    m_crosstext.m_color[1] = 128.0f;
    m_crosstext.m_color[2] = 128.0f;
    m_crosstext.m_color[3] = 80.0f;

    CZSealBody *owner = CHUD::GetHUDOwner();
    CZKit *kit = &owner->m_kit;

    if (cur_camzoom == 1.0f) {
        cur_camzoom = appCamera->m_view->m_zoom;
        appCamera->SetZoom(1.0f);
    }
    if (!CameraZoom::LensFxOn()) {
        if (kit && bSwapLens && cur_zoomstate == ZOOMSTATE_UNKNOWN &&
            zdb::CWorld::m_world->m_night_mission) {
            ZOOMSTATE zoomstate = owner->m_zoomstate;

            if (zoomstate >= ZOOMSTATE_THERMAL)
                cur_zoomstate = zoomstate;
            if (zoomstate != ZOOMSTATE_UNKNOWN) {
                kit->Apply_LensEffects(false, 0);
                CameraZoom::SetLensFx(1);
            }
        }
        kit->Apply_LensEffects(false, 0);
    }
    CameraZoom::HoldLensFx(1);
    if (kit)
        kit->m_heartbeat_enabled = false;
    owner->m_ctrl->StopRumbler();
    theReco.Suspend(true);
    m_lastmenustate = owner->m_ctrl->GetMenuState();
    owner->m_ctrl->SetMenuState(stateFullScreenMapAnim, false);
}

bool CZHudMissionCams::IsPlaying()
{
    return m_animhandle != 0;
}
