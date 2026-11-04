/* hud_missioncams.cpp -- the HUD's mission cameras (OVL1 0x228ED0-0x229C9F): SOCOM 1's unit (its
 * DWARF names it, Z:\dev\Apps\FTS\hud_missioncams.cpp), the same functions in the same order,
 * from IsPlaying to the constructor; HudTick written (the HUD's round 40). Not written yet:
 * PlayCamera(CMissionCam *, bool), which SOCOM II grew from 0x3DC to 0x54C bytes, and CleanupCamera
 * -- both call into the HUD, the seal's controller, the jukebox and the camera through rows not
 * named yet. Defined in reverse of their image order (-inline deferred). */

#include <string.h>

#include "hud_missioncams.h"
#include "../../gamez/zinput/zinput.h"

class CMission {
public:
    CMissionCam *GetMissionCamByIdentifier(const char *id);
};

extern CMission theMission;

/* the HUD's owner's controller, its input bit (zseal.h's CSealCtrl: m_input_enabled, bit 11 of the
 * flags at 0x170) */
class CSealCtrl {
public:
    unsigned char _0[0x170];
    unsigned int m_flags0 : 11;
    unsigned int m_input_enabled : 1;
};

class CZSealBody {
public:
    unsigned char _0[0xc0];
    CSealCtrl *m_ctrl;   // 0xc0
};

/* the HUD: its owner and its menu state (SOCOM II's offset) */
class CHUD {
public:
    static CZSealBody *GetHUDOwner();
    bool MenuUp() const { return m_menustate != stateNone; }

    unsigned char _0[0x1a220];
    MENU_STATE m_menustate;   // 0x1a220
};

extern CHUD *theHUD;

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
