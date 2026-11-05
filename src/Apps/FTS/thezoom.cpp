/* thezoom.cpp -- the scope's zoom: its goal, the HUD and SEAL it serves, and seeing farther while
 * zoomed (SOCOM 1's Apps/FTS/thezoom.cpp; OVL1 0x1F0490-0x1F19FF) -- and SOCOM II's lens effects
 * (the ZAnims a scope, binoculars, night vision or thermal put over the screen) ahead of it, from
 * 0x1EF940. Defined in reverse of their image order (-inline deferred). */

#include "camera.h"

#include <iostream>

/* SOCOM II's lens effects, as the anims are named (a byte under -enum min; 9 is never asked for,
 * and the effect before is taken as the current one while it is 9; the names ours) */
enum LENS_FX {
    lens_fx_init = 0,
    lens_fx_none = 1,
    lens_fx_thermal = 2,
    lens_fx_nvg = 3,
    lens_fx_starlight_scope = 4,
    lens_fx_scope = 5,
    lens_fx_binocs = 6,
    lens_fx_first_person = 7,
    lens_fx_third_person = 8,
    lens_fx_9 = 9
};

/* the clip and fog bases SeeFarther scales (SOCOM 1's globals) */
extern float BaseFarClip;
extern float BaseMidClip;
extern float BaseFogFar;

/* the zoom's other statics (the names ours) */
extern "C" {
extern bool data_00408CD8;                       /* SeeFarther's last switch */
extern float data_00408CE0;                      /* and its factor */
extern int data_00408CF0;                        /* (set: SeeFarther does not note them) */
/* SOCOM II's lens effects (the names ours) */
extern CZAnim *data_00408CA0[9];                 /* the effects' anims: init_lens_fx, to_no_lens_fx ... */
extern CZAnim *data_00408C88[3];                 /* blindplayer01..03 */
extern CZAnim *data_00408CC8;                    /* the effect playing */
extern CZAnim *data_00408CD0;                    /* and its activation */
extern int data_00408CE8;                        /* effects asked for with no anim */
extern LENS_FX data_003DC350;                    /* the effect before (8) */
extern LENS_FX data_003DC358;                    /* the effect (8) */
extern unsigned char data_003DC360;              /* lens effects on (1) */
}


/* the HUD's zoom (hud_main.cpp's: only what the zoom calls) */
class CHUD {
public:
    void ZoomIn();   /* 0x1F7360 */
    void ZoomOut();   /* 0x1F7560 */
};

extern unsigned char gopt[0x1b];

/* the base fog colour by value (the name ours: SeeFarther copies it out twice) */
static inline CPnt4D BaseFogColor() { return zdb::CVisual::m_basefog_color; }

/* a camera's fog colour set from a colour passed by value (the name ours) */
static inline void SetFogColorBy(zdb::CCamera *cam, CPnt4D c) { cam->SetFogColor(c); }

/* the scope's zoom (SOCOM 1's: a class of statics) */
class CameraZoom {
public:
    static bool Tick(float dT);
    static void OnGoal();
    static void HandleNewZoom();
    static void HandleZoomIn(float newzoom, float current);
    static void SetGoal(float goal);
    static void AttachHud(CHUD *hud);
    static void AttachSeal(CZSealBody *seal);
    static void Reset();
    static void ClearLensFx();
    static void LoadLensFx();
    static void ResetLensColors();
    static void HoldLensFx(unsigned char hold);
    static void ReleaseLensFx(unsigned char hold);
    static void SetLensFx(unsigned char fx);
    static unsigned char LensFxOn();
    static void SetLensFxAnim(CZAnim *anim);

    /* SOCOM 1's statics */
    static float m_current;
    static float m_goal;
    static CAMVIEW m_oldview;
    static float m_increment;
    static unsigned char m_iszoom;
    static CZSealBody *m_seal;
    static CHUD *m_hud;
    static unsigned char m_isGoalHit;
};

/* the zoom put back to none: no step, not zooming, at 1 and aiming for it, the old view first
 * person, the clip and fog bases forgotten; the app camera in the third person view at zoom 1 and
 * mode 2; the third person's lens effect put up, and SeeFarther's notes cleared (SOCOM II's; the
 * name ours) */
void CameraZoom::Reset()
{
    CameraZoom::m_increment = 0.0f;
    CameraZoom::m_iszoom = 0;
    CameraZoom::m_current = 1.0f;
    CameraZoom::m_goal = 1.0f;
    CameraZoom::m_oldview = cam_view_first;
    CameraZoom::m_isGoalHit = 0;
    BaseFarClip = 0.0f;
    BaseMidClip = 0.0f;
    BaseFogFar = 0.0f;
    if (appCamera) {
        appCamera->m_ctrl_view = cam_view_third;
        appCamera->SetZoom(1.0f);
        appCamera->SetCameraMode(2);
        appCamera->m_camera_last_mode = cam_mode_tether;
    }
    data_003DC350 = lens_fx_third_person;
    data_003DC358 = lens_fx_third_person;
    SetLensFx(lens_fx_third_person);
    data_00408CD0 = 0;
    data_00408CD8 = 0;
    data_00408CE0 = 0.0f;
    data_00408CE8 = 0;
    data_00408CF0 = 0;
}


void CameraZoom::AttachSeal(CZSealBody *seal)
{
    CameraZoom::m_seal = seal;
}

/* the HUD the zoom draws on; the clip and fog bases forgotten (SOCOM II's) */
void CameraZoom::AttachHud(CHUD *hud)
{
    CameraZoom::m_hud = hud;
    BaseFarClip = 0.0f;
    BaseMidClip = 0.0f;
    BaseFogFar = 0.0f;
}

void CameraZoom::SetGoal(float goal)
{
    CameraZoom::m_goal = goal;
    CameraZoom::m_isGoalHit = 1;
}

/* a zoom in asked for: back at 1 from 1 (SOCOM II's), only the view put back to the third person
 * when not zooming yet; else a new goal (but 0, and 1.01 apart) stepped toward three times as fast,
 * the HUD zoomed in (with the option), and on the first zoom from 1 the view kept and made first
 * person; 1.01 the same, the HUD zoomed in only with night vision on (SOCOM 1's HandleZoomIn) */
void CameraZoom::HandleZoomIn(float newzoom, float current)
{
    if (CameraZoom::m_goal == 1.0f && newzoom == 1.0f) {
        if (!CameraZoom::m_iszoom && CameraZoom::m_current == 1.0f) {
            CameraZoom::m_oldview = appCamera->SetCtrlView(cam_view_third);
        }
    } else if (newzoom && newzoom != 1.01f) {
        SetGoal(newzoom);
        CameraZoom::m_increment = 3.0f * newzoom;
        if (gopt[0])
            CameraZoom::m_hud->ZoomIn();
        if (!CameraZoom::m_iszoom && CameraZoom::m_current == 1.0f) {
            CameraZoom::m_iszoom = 1;
            CameraZoom::m_oldview = appCamera->SetCtrlView(cam_view_first);
        }
    } else if (newzoom == 1.01f) {
        SetGoal(newzoom);
        CameraZoom::m_increment = 3.0f * newzoom;
        if (gopt[0] && CameraZoom::m_seal->m_kit.NVG_Enabled())
            CameraZoom::m_hud->ZoomIn();
        if (!CameraZoom::m_iszoom && CameraZoom::m_current == 1.0f) {
            CameraZoom::m_iszoom = 1;
            CameraZoom::m_oldview = appCamera->SetCtrlView(cam_view_first);
        }
    }
}

/* the world's camera made to see farther by a factor (its mid and far clips and its fog pushed
 * out, the bases kept), or put back as it was -- the base fog colour returned by value and passed
 * by value again, so it is copied into two temporaries (lq/sq, then a 128-bit move -- paddub with
 * zero -- and sq) before the element copy into the camera, as SOCOM 1's SeeFarther did */
void SeeFarther(bool bEnable, float SeeFartherFactor)
{
    zdb::CCamera *cam = zdb::CWorld::m_world->m_camera;

    if (!data_00408CF0) {
        data_00408CE0 = SeeFartherFactor;
        data_00408CD8 = bEnable;
    }
    if (!bEnable) {
        if (BaseMidClip == 0.0f)
            BaseMidClip = cam->m_mid_plane;
        if (BaseFarClip == 0.0f)
            BaseFarClip = cam->m_far_plane;
        cam->SetMidClip(BaseMidClip);
        cam->SetFarClip(BaseFarClip);
        cam->Update(zdb::ZCAM_NORMAL);
        if (BaseFogFar == 0.0f)
            BaseFogFar = cam->m_fog_far;
        cam->m_fog_far = BaseFogFar;
        SetFogColorBy(cam, BaseFogColor());
    } else {
        BaseFogFar = cam->m_fog_far;
        cam->m_fog_far = BaseFogFar + SeeFartherFactor;
        BaseFarClip = cam->m_far_plane;
        cam->SetFarClip(BaseFarClip + SeeFartherFactor);
        BaseMidClip = cam->m_mid_plane;
        cam->SetMidClip(BaseMidClip + SeeFartherFactor);
        cam->Update(zdb::ZCAM_NORMAL);
    }
}

/* the goal reached (while zooming): the kit's reticule put back; at 1 the HUD zoomed out (with the
 * option) and the view and the zooming flag cleared; at SOCOM II's 1.01 the HUD zoomed out, and back
 * in with night vision on; anywhere else zoomed in unless night vision is on (SOCOM 1's OnGoal) */
void CameraZoom::OnGoal()
{
    if (CameraZoom::m_iszoom) {
        CameraZoom::m_seal->m_kit.SetCurReticule(false);
        if (CameraZoom::m_goal == 1.0f) {
            if (gopt[0])
                CameraZoom::m_hud->ZoomOut();
            appCamera->m_ctrl_view = cam_view_third;
            CameraZoom::m_iszoom = 0;
        } else if (CameraZoom::m_goal == 1.01f || CameraZoom::m_goal == 1.01f) {
            if (gopt[0] && CameraZoom::m_seal->m_kit.NVG_Enabled()) {
                CameraZoom::m_hud->ZoomOut();
                CameraZoom::m_hud->ZoomIn();
                return;
            }
            CameraZoom::m_hud->ZoomOut();
        } else {
            if (gopt[0] && !CameraZoom::m_seal->m_kit.NVG_Enabled())
                CameraZoom::m_hud->ZoomIn();
        }
    }
}

/* a frame of the zoom: a new zoom handled; at the goal, the goal's handler once; else the current
 * zoom (the camera's) stepped toward the goal and given to the app camera -- or, while the SEAL's
 * controller holds it (mode 5), only read back */
bool CameraZoom::Tick(float dT)
{
    HandleNewZoom();
    if (CameraZoom::m_goal == CameraZoom::m_current && CameraZoom::m_isGoalHit) {
        OnGoal();
        CameraZoom::m_isGoalHit = 0;
        CameraZoom::m_seal->m_zooming = 0;
        return true;
    }
    CEntityCtrl *control = CameraZoom::m_seal ? CameraZoom::m_seal->m_control : 0;

    if (control && control->m_221 == 5) {
        CameraZoom::m_current = appCamera->m_camera->m_screenAspect[0];
    } else {
        float cur = appCamera->m_camera->m_screenAspect[0];
        float step = dT * CameraZoom::m_increment;

        CameraZoom::m_current = cur;
        if (CameraZoom::m_goal >= cur) {
            CameraZoom::m_current += step;
            if (CameraZoom::m_current > CameraZoom::m_goal) {
                CameraZoom::m_current = CameraZoom::m_goal;
                CameraZoom::m_increment = 0.0f;
            }
        } else {
            CameraZoom::m_current -= step;
            if (CameraZoom::m_current < CameraZoom::m_goal) {
                CameraZoom::m_current = CameraZoom::m_goal;
                CameraZoom::m_increment = 0.0f;
            }
        }
    }
    appCamera->SetZoom(CameraZoom::m_current);
    return false;
}

/* the lens effects' colours kept (SOCOM II's: four taken from zdb::CVisual::m_lensColor, 0x4B4D40,
 * one made; unnamed) */
extern "C" {
/* the first colour kept, the unit's only dynamically initialised static (from CPnt4D::zero:
 * __sinit_thezoom.cpp 0x3FD850 copies it after <iostream>'s pair) */
PNT4D data_00408D00 = CPnt4D::zero;
extern PNT4D data_00408D10;
extern PNT4D data_00408D20;
extern PNT4D data_00408D30;
extern PNT4D data_00408D40;
PNT4D func_003B7620(int arg);
}

/* a colour by value (the name ours): the array's first copied by quadword, the others element by
 * element (MW trusts only the array's start to be 16-aligned) */
static inline PNT4D LensColor(const PNT4D &c) { return c; }

/* the lens effects loaded (SOCOM II's; the name ours): the effects before and now the third
 * person's, the anims forgotten, the three blindplayer anims, the nine effects' anims when the
 * lens effects are on (else none), the third person's effect put up, and the colours kept */
void CameraZoom::LoadLensFx()
{
    data_003DC350 = lens_fx_third_person;
    data_003DC358 = lens_fx_third_person;
    ClearLensFx();
    data_00408C88[0] = ZAnim.GetAnim("blindplayer01", 0);
    data_00408C88[1] = ZAnim.GetAnim("blindplayer02", 0);
    data_00408C88[2] = ZAnim.GetAnim("blindplayer03", 0);
    if (LensFxOn()) {
        data_00408CA0[lens_fx_init] = ZAnim.GetAnim("init_lens_fx", 0);
        data_00408CA0[lens_fx_none] = ZAnim.GetAnim("to_no_lens_fx", 0);
        data_00408CA0[lens_fx_thermal] = ZAnim.GetAnim("to_thermal_lens_fx", 0);
        data_00408CA0[lens_fx_nvg] = ZAnim.GetAnim("to_nvg_lens_fx", 0);
        data_00408CA0[lens_fx_starlight_scope] = ZAnim.GetAnim("to_starlight_scope_lens_fx", 0);
        data_00408CA0[lens_fx_scope] = ZAnim.GetAnim("to_scope_lens_fx", 0);
        data_00408CA0[lens_fx_binocs] = ZAnim.GetAnim("to_binocs_lens_fx", 0);
        data_00408CA0[lens_fx_first_person] = ZAnim.GetAnim("to_first_person_lens_fx", 0);
        data_00408CA0[lens_fx_third_person] = ZAnim.GetAnim("to_third_person_lens_fx", 0);
    } else {
        data_00408CA0[lens_fx_init] = 0;
        data_00408CA0[lens_fx_none] = 0;
        data_00408CA0[lens_fx_thermal] = 0;
        data_00408CA0[lens_fx_nvg] = 0;
        data_00408CA0[lens_fx_starlight_scope] = 0;
        data_00408CA0[lens_fx_scope] = 0;
        data_00408CA0[lens_fx_binocs] = 0;
        data_00408CA0[lens_fx_first_person] = 0;
        data_00408CA0[lens_fx_third_person] = 0;
    }
    SetLensFx(lens_fx_third_person);
    data_00408D00 = LensColor(zdb::CVisual::m_lensColor[0]);
    data_00408D10 = LensColor(zdb::CVisual::m_lensColor[1]);
    data_00408D20 = LensColor(zdb::CVisual::m_lensColor[2]);
    data_00408D30 = LensColor(zdb::CVisual::m_lensColor[3]);
    data_00408D40 = func_003B7620(0);
}

/* the effect's anim, or no lens's when it has none (the name ours) */
static inline CZAnim *LensFxAnim(unsigned char fx)
{
    CZAnim *anim = data_00408CA0[lens_fx_none];

    if (data_00408CA0[fx])
        anim = data_00408CA0[fx];
    return anim;
}

/* the lens effects' anims forgotten (the playing one stopped) */
void CameraZoom::ClearLensFx()
{
    data_00408CA0[0] = 0;
    data_00408CA0[1] = 0;
    data_00408CA0[2] = 0;
    data_00408CA0[3] = 0;
    data_00408CA0[4] = 0;
    data_00408CA0[5] = 0;
    data_00408CA0[6] = 0;
    data_00408CA0[7] = 0;
    data_00408CA0[8] = 0;
    data_00408C88[0] = 0;
    data_00408C88[1] = 0;
    data_00408C88[2] = 0;
    if (data_00408CC8)
        func_002707C0(data_00408CC8, false);
    data_00408CC8 = 0;
    data_00408CD0 = 0;
}

/* a lens effect put up (unless held): the one before noted, the playing anim stopped, the effect's
 * anim (or no lens's) activated -- counted when there is none; the one before taken as this one if
 * it was 9 */
void CameraZoom::SetLensFx(unsigned char fx)
{
    if (!data_00408CF0) {
        data_003DC350 = data_003DC358;
        data_003DC358 = (LENS_FX)fx;
        if (data_00408CC8) {
            func_002707C0(data_00408CC8, false);
            data_00408CD0 = 0;
        }
        data_00408CC8 = LensFxAnim(fx);
        if (data_00408CC8) {
            data_00408CD0 = data_00408CC8->Activate(0);
            data_00408CE8 = 0;
        } else {
            data_00408CE8++;
        }
        if (data_003DC350 == lens_fx_9)
            data_003DC350 = data_003DC358;
    }
}

unsigned char CameraZoom::LensFxOn()
{
    return data_003DC360;
}

void CameraZoom::SetLensFxAnim(CZAnim *anim)
{
    data_00408CD0 = anim;
}

/* the lens effects' colours put back to white with no alpha, the light maps' tints handed white at
 * once (CVisual::SetLightTints, 0x3B78D0: the colour by value, its copy's address passed) and two
 * words of thePipe cleared (an inline both callers share; the names ours) */
extern "C" {
void func_00270960(CZAnim *anim, bool now);
extern int data_00488E90;                       /* thePipe + 0x6D0 */
extern int data_00488EA0;                       /* thePipe + 0x6E0 */
}

static inline void ResetLensColorsOnly()
{
    for (int i = 0; i < 4; i++) {
        zdb::CVisual::m_lensColor[i].x = 1.0f;
        zdb::CVisual::m_lensColor[i].y = 1.0f;
        zdb::CVisual::m_lensColor[i].z = 1.0f;
        zdb::CVisual::m_lensColor[i].w = 0.0f;
    }
    /* a plain PNT4D: its address is passed for the by-value colour as it stands (a CPnt4D -- the
     * constructor's -- is sliced into a copy first, ten instructions more in each caller) */
    PNT4D white;

    white.x = 1.0f;
    white.y = 1.0f;
    white.z = 1.0f;
    white.w = 0.0f;
    zdb::CVisual::SetLightTints(white, 0.0f);
}

/* SetLensFx's body taking the effect as the enum itself (the restore below copies the effect before
 * into it -- lb, a byte enum's copy -- and the body narrows it for the index with andi) */
static inline void SetLensFxEnum(LENS_FX fx)
{
    if (!data_00408CF0) {
        data_003DC350 = data_003DC358;
        data_003DC358 = fx;
        if (data_00408CC8) {
            func_002707C0(data_00408CC8, false);
            data_00408CD0 = 0;
        }
        data_00408CC8 = LensFxAnim(fx);
        if (data_00408CC8) {
            data_00408CD0 = data_00408CC8->Activate(0);
            data_00408CE8 = 0;
        } else {
            data_00408CE8++;
        }
        if (data_003DC350 == lens_fx_9)
            data_003DC350 = data_003DC358;
    }
}

/* the effect before put back while nothing holds the lens effects (an inline; the name ours) */
static inline void RestoreLensFx()
{
    if (!data_00408CF0) {
        SetLensFxEnum(data_003DC350);
        data_00408CF0 = 0;
    }
}

/* the lens effects let go of (the hold xor-ed with its byte): with nothing holding them, the effect
 * before put back, the camera's sight as SeeFarther last noted it, the colours reset, the
 * blindplayer anims restarted and the first person's effect stopped if running (SOCOM II's; the
 * name ours) */
void CameraZoom::ReleaseLensFx(unsigned char hold)
{
    data_00408CF0 ^= hold;
    if (!data_00408CF0) {
        RestoreLensFx();
        if (!data_00408CF0)
            SeeFarther(data_00408CD8, data_00408CE0);
        ResetLensColorsOnly();
        if (data_00408C88[0])
            func_002709B0(data_00408C88[0], 1);
        if (data_00408C88[1])
            func_002709B0(data_00408C88[1], 1);
        if (data_00408C88[2])
            func_002709B0(data_00408C88[2], 1);
        if (data_00408CA0[lens_fx_first_person] && data_00408CA0[lens_fx_first_person]->Running())
            func_002707C0(data_00408CA0[lens_fx_first_person], false);
    }
}

/* the lens effects held (or not): held, the kit's own lens effects off and the playing effect
 * stopped (the effect then 9); the camera's sight put back; on 1 the first person's effect started
 * unless running; the colours reset and the blindplayer anims restarted (SOCOM II's; the name ours) */
void CameraZoom::HoldLensFx(unsigned char hold)
{
    data_00408CF0 |= hold;
    if (data_00408CF0) {
        CameraZoom::m_seal->m_kit.Apply_LensEffects(false, 0);
        data_00408CD0 = 0;
        if (data_00408CC8) {
            func_002707C0(data_00408CC8, false);
            data_00408CC8 = 0;
            data_003DC350 = data_003DC358;
            data_003DC358 = lens_fx_9;
        }
    }
    SeeFarther(false, 0.0f);
    if (hold == 1 && data_00408CA0[lens_fx_first_person] &&
        !data_00408CA0[lens_fx_first_person]->Running())
        data_00408CA0[lens_fx_first_person]->Activate(0);
    ResetLensColorsOnly();
    data_00488EA0 = 0;
    data_00488E90 = 0;
    if (data_00408C88[0])
        func_00270BE0(data_00408C88[0], 1);
    if (data_00408C88[1])
        func_00270BE0(data_00408C88[1], 1);
    if (data_00408C88[2])
        func_00270BE0(data_00408C88[2], 1);
}

/* the colours reset and the blindplayer anims stopped (SOCOM II's; the name ours) */
void CameraZoom::ResetLensColors()
{
    ResetLensColorsOnly();
    data_00488EA0 = 0;
    data_00488E90 = 0;
    if (data_00408C88[0])
        func_00270960(data_00408C88[0], false);
    if (data_00408C88[1])
        func_00270960(data_00408C88[1], false);
    if (data_00408C88[2])
        func_00270960(data_00408C88[2], false);
}
