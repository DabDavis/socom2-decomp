/* hud_missioncams.cpp -- the HUD's mission cameras (OVL1 0x228ED0-0x229C9F): SOCOM 1's unit (its
 * DWARF names it, Z:\dev\Apps\FTS\hud_missioncams.cpp), the same functions in the same order,
 * from IsPlaying to the constructor; HudTick written (the HUD's round 40), CleanupCamera (round 51).
 * Not written yet: PlayCamera(CMissionCam *, bool), which SOCOM II grew from 0x3DC to 0x54C bytes
 * (it sets the two file statics CleanupCamera gives back). Defined in reverse of their image order
 * (-inline deferred). */

#include <string.h>

#include "hud_missioncams.h"
#include "../../gamez/zinput/zinput.h"

class CMission {
public:
    CMissionCam *GetMissionCamByIdentifier(const char *id);
};

extern CMission theMission;

/* the HUD's owner's controller, its input bit (zseal.h's CSealCtrl: m_input_enabled, bit 11 of the
 * flags at 0x170), and the menu state put back (OVL2 0x597740, SOCOM II's with a flag) */
class CSealCtrl {
public:
    void SetMenuState(MENU_STATE state, bool flag);

    unsigned char _0[0x170];
    unsigned int m_flags0 : 11;
    unsigned int m_input_enabled : 1;
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

class CZSealBody {
public:
    unsigned char _0[0xc0];
    CSealCtrl *m_ctrl;   // 0xc0
    unsigned char _c4[0x5e0 - 0xc4];
    CZKit m_kit;   // 0x5e0
};

CZSealBody *ftsGetPlayer();

/* the letterbox's bars taken away (hud_letterbox.cpp's) */
class CLetterBox {
public:
    void RemoveLetterbox();
};

/* the HUD: its owner, its letterbox and its menu state (SOCOM II's offsets), shown again by On */
class CHUD {
public:
    static CZSealBody *GetHUDOwner();
    void On();
    bool MenuUp() const { return m_menustate != stateNone; }

    unsigned char _0[0x1a100];
    CLetterBox m_letterbox;   // 0x1a100
    unsigned char _1a101[0x1a220 - 0x1a101];
    MENU_STATE m_menustate;   // 0x1a220
};

extern CHUD *theHUD;

/* the camera's mission film: its music at 0x18 (SOCOM 1's DWARF: identifier, name, three
 * descriptions, the animation, the music) */
class CSnd;

class CMissionCam {
public:
    unsigned char _0[0x18];
    CSnd *m_music;   // 0x18
};

/* the jukebox and the paused voices (zsnd's), the master volume's wrapper (OVL1 0x3414C0, a tail call
 * to snd_SetMasterVolume) */
class CSndJukebox {
public:
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
    static void ResumeAllSounds(SOUND_MODE mode);
};

extern "C" void func_003414C0(int which, int volume);

/* the weapons' tick switch (SOCOM 1's CZWeapon::m_weaponTickDisabled, cleared here as SOCOM 1's
 * CleanupCamera did -- OVL1 bss 0x4B52B0, no data row yet) */
extern unsigned char data_004B52B0;

/* OVL2 0x598840 (a byte flag set when it changes) and 0x592CA0 (on the player's controller): their
 * names not known */
extern "C" void func_00598840(bool on);
extern "C" void func_00592CA0(CSealCtrl *ctrl);

/* the lens effects (thezoom.cpp's CameraZoom, SOCOM II's) */
class CameraZoom {
public:
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

    unsigned char _0[0x5e0];
    PNT4D m_lensfx_nvg;   // 0x5e0
    PNT4D m_lensfx_starlightscope;   // 0x5f0
};
}

/* the zoom state and the camera's zoom a film put aside, given back at its end (SOCOM 1's file
 * statics, by its symbols: cur_zoomstate gp-30240, cur_camzoom gp-30236; OVL1 .data 0x3DC888 and
 * 0x3DC890) */
static signed char cur_zoomstate = -1;
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
    unsigned char _0[0x110];
    CGameState m_coreState;   // 0x110
};

extern COurGame theGame;

/* the camera's animation stopped (zanim_anim.cpp's, out of line) and the camera's regions */
extern "C" void func_002707C0(CZAnim *anim, bool now);

class CZAnim {
public:
    bool Running();
};

class CAppCamera {
public:
    void UpdateCameraRegions();
    void SetZoom(float zoom);
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
    func_003414C0(1, m_socom2_3b8);
    if (m_currentcam->m_music) {
        CSndJukebox::FadeOut();
        CSndJukebox::ClearQueue();
    }
    CSndInstance::ResumeAllSounds(SND_MODE_DIALOG);
    data_004B52B0 = 0;
    CSealCtrlAi::SetAiEnabled(true, true);
    func_00598840(true);

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
        cur_zoomstate = -1;
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

bool CZHudMissionCams::IsPlaying()
{
    return m_animhandle != 0;
}
