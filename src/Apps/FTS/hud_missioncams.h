/* The HUD's mission cameras: a camera fly-by with its objective's name and description in the
 * letterbox (CZHudMissionCams). SOCOM 1's DWARF layout, moved for SOCOM II: C2D is 4 bytes
 * shorter, C2DString 0x10 longer (0x98), and the cross bitmap and its texture are gone -- a
 * string drawn after the descriptions stands at 0x280, the cross text at 0x318. The header's
 * name is ours. */
#ifndef HUD_MISSIONCAMS_H
#define HUD_MISSIONCAMS_H

#include "hud_2d.h"

class CMissionCam;
class CPad;
class CZAnim;

enum MENU_STATE {
    stateNone = 0,
    stateWeaponSel = 1,
    stateOrders = 2,
    statePauseTestMenu = 3,
    stateFullScreenMapMenu = 4,
    stateFullScreenMapAnim = 5,
    stateLast = 6,
};

class CZHudMissionCams : public C2D {
public:
    CZHudMissionCams();
    virtual ~CZHudMissionCams();
    virtual void Draw(zdb::CCamera *theCamera);
    bool IsPlaying();
    void PlayCamera(CMissionCam *missioncam, bool bSwapLens);
    void PlayCamera(const char *camname);
    void CleanupCamera(CPad *pad);
    void HudTick(CPad *pad, float dT);
    void UnInit();
    void Init();

    CMissionCam *m_currentcam;   // 0x18
    CZAnim *m_animhandle;   // 0x1c
    C2DString m_letterboxobjectivename;   // 0x20
    C2DString m_letterboxobjectivedescription[3];   // 0xb8
    C2DString m_socom2_280;   // 0x280: drawn after the descriptions, not listed or wiped
    C2DString m_crosstext;   // 0x318
    MENU_STATE m_lastmenustate;   // 0x3b0
    int m_pause_for_frame;   // 0x3b4
    int m_socom2_3b8;   // 0x3b8: PlayCamera keeps a call's answer here
};

#endif
