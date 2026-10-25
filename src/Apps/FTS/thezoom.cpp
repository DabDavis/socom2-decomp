/* thezoom.cpp -- the scope's zoom: its goal, the HUD and SEAL it serves, and seeing farther while
 * zoomed (SOCOM 1's Apps/FTS/thezoom.cpp; OVL1 0x1F0490-0x1F19FF) -- and SOCOM II's lens effects
 * (the ZAnims a scope, binoculars, night vision or thermal put over the screen) ahead of it, from
 * 0x1EF940. Defined in reverse of their image order (-inline deferred). */

#include "camera.h"

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

/* the zoom's statics, unnamed yet in data.txt (SOCOM 1's names in the comments) */
extern "C" {
extern float data_003DC340;                      /* CameraZoom::m_goal */
extern float data_003DC338;                      /* CameraZoom::m_current */
extern float data_00408C48;                      /* CameraZoom::m_increment */
extern unsigned char data_00408C68;              /* CameraZoom::m_isGoalHit */
extern CHUD *data_00408C60;                      /* CameraZoom::m_hud */
extern CZSealBody *data_00408C58;                /* CameraZoom::m_seal */
extern float data_00408C70;                      /* BaseFarClip */
extern float data_00408C78;                      /* BaseMidClip */
extern float data_00408C80;                      /* BaseFogFar */
extern unsigned char data_00408CD8;              /* SeeFarther's last switch */
extern float data_00408CE0;                      /* and its factor */
extern int data_00408CF0;                        /* (set: SeeFarther does not note them) */
extern CPnt4D data_004B4AB0;                     /* zdb::CVisual::m_basefog_color */
extern unsigned char data_00408C50;              /* CameraZoom::m_iszoom */
extern CAMVIEW data_003DC348;                    /* CameraZoom::m_oldview */
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


/* the kit and the HUD, unnamed yet (SOCOM 1's names) */
extern "C" void func_005BE570(void *kit, int reticule);   /* CZKit::SetCurReticule */
extern "C" bool func_005C80F0(void *kit);                 /* CZKit::NVG_Enabled */
extern "C" void func_001F7560(CHUD *hud);                 /* CHUD::ZoomOut */
extern "C" void func_001F7360(CHUD *hud);                 /* CHUD::ZoomIn */

extern unsigned char gopt[0x1b];

/* the base fog colour by value (the name ours: SeeFarther copies it out twice) */
static inline CPnt4D BaseFogColor() { return data_004B4AB0; }

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
    static void SetLensFx(unsigned char fx);
    static unsigned char LensFxOn();
    static void SetLensFxAnim(CZAnim *anim);
};

/* the zoom put back to none: no step, not zooming, at 1 and aiming for it, the old view first
 * person, the clip and fog bases forgotten; the app camera in the third person view at zoom 1 and
 * mode 2; the third person's lens effect put up, and SeeFarther's notes cleared (SOCOM II's; the
 * name ours) */
void CameraZoom::Reset()
{
    data_00408C48 = 0.0f;
    data_00408C50 = 0;
    data_003DC338 = 1.0f;
    data_003DC340 = 1.0f;
    data_003DC348 = cam_view_first;
    data_00408C68 = 0;
    data_00408C70 = 0.0f;
    data_00408C78 = 0.0f;
    data_00408C80 = 0.0f;
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
    data_00408C58 = seal;
}

/* the HUD the zoom draws on; the clip and fog bases forgotten (SOCOM II's) */
void CameraZoom::AttachHud(CHUD *hud)
{
    data_00408C60 = hud;
    data_00408C70 = 0.0f;
    data_00408C78 = 0.0f;
    data_00408C80 = 0.0f;
}

void CameraZoom::SetGoal(float goal)
{
    data_003DC340 = goal;
    data_00408C68 = 1;
}

/* a zoom in asked for: back at 1 from 1 (SOCOM II's), only the view put back to the third person
 * when not zooming yet; else a new goal (but 0, and 1.01 apart) stepped toward three times as fast,
 * the HUD zoomed in (with the option), and on the first zoom from 1 the view kept and made first
 * person; 1.01 the same, the HUD zoomed in only with night vision on (SOCOM 1's HandleZoomIn) */
void CameraZoom::HandleZoomIn(float newzoom, float current)
{
    if (data_003DC340 == 1.0f && newzoom == 1.0f) {
        if (!data_00408C50 && data_003DC338 == 1.0f) {
            data_003DC348 = appCamera->SetCtrlView(cam_view_third);
        }
    } else if (newzoom && newzoom != 1.01f) {
        SetGoal(newzoom);
        data_00408C48 = 3.0f * newzoom;
        if (gopt[0])
            func_001F7360(data_00408C60);
        if (!data_00408C50 && data_003DC338 == 1.0f) {
            data_00408C50 = 1;
            data_003DC348 = appCamera->SetCtrlView(cam_view_first);
        }
    } else if (newzoom == 1.01f) {
        SetGoal(newzoom);
        data_00408C48 = 3.0f * newzoom;
        if (gopt[0] && func_005C80F0(data_00408C58->m_kit))
            func_001F7360(data_00408C60);
        if (!data_00408C50 && data_003DC338 == 1.0f) {
            data_00408C50 = 1;
            data_003DC348 = appCamera->SetCtrlView(cam_view_first);
        }
    }
}

/* the world's camera made to see farther by a factor (its mid and far clips and its fog pushed
 * out, the bases kept), or put back as it was.
 * OPEN (0x190 of 0x198): the target copies the base fog colour out twice (two quadword stores, the
 * second through paddub) before the element copy -- SOCOM 1's SeeFarther too; a by-value getter
 * gives one, no local, cast or assignment gave the second */
void SeeFarther(bool bEnable, float SeeFartherFactor)
{
    zdb::CCamera *cam = zdb::CWorld::m_world->m_camera;

    if (!data_00408CF0) {
        data_00408CE0 = SeeFartherFactor;
        data_00408CD8 = bEnable;
    }
    if (!bEnable) {
        if (data_00408C78 == 0.0f)
            data_00408C78 = cam->m_mid_plane;
        if (data_00408C70 == 0.0f)
            data_00408C70 = cam->m_far_plane;
        cam->SetMidClip(data_00408C78);
        cam->SetFarClip(data_00408C70);
        cam->Update(zdb::ZCAM_NORMAL);
        if (data_00408C80 == 0.0f)
            data_00408C80 = cam->m_fog_far;
        cam->m_fog_far = data_00408C80;
        cam->SetFogColor(BaseFogColor());
    } else {
        data_00408C80 = cam->m_fog_far;
        cam->m_fog_far = data_00408C80 + SeeFartherFactor;
        data_00408C70 = cam->m_far_plane;
        cam->SetFarClip(data_00408C70 + SeeFartherFactor);
        data_00408C78 = cam->m_mid_plane;
        cam->SetMidClip(data_00408C78 + SeeFartherFactor);
        cam->Update(zdb::ZCAM_NORMAL);
    }
}

/* the goal reached (while zooming): the kit's reticule put back; at 1 the HUD zoomed out (with the
 * option) and the view and the zooming flag cleared; at SOCOM II's 1.01 the HUD zoomed out, and back
 * in with night vision on; anywhere else zoomed in unless night vision is on (SOCOM 1's OnGoal) */
void CameraZoom::OnGoal()
{
    if (data_00408C50) {
        func_005BE570(data_00408C58->m_kit, 0);
        if (data_003DC340 == 1.0f) {
            if (gopt[0])
                func_001F7560(data_00408C60);
            appCamera->m_ctrl_view = cam_view_third;
            data_00408C50 = 0;
        } else if (data_003DC340 == 1.01f || data_003DC340 == 1.01f) {
            if (gopt[0] && func_005C80F0(data_00408C58->m_kit)) {
                func_001F7560(data_00408C60);
                func_001F7360(data_00408C60);
                return;
            }
            func_001F7560(data_00408C60);
        } else {
            if (gopt[0] && !func_005C80F0(data_00408C58->m_kit))
                func_001F7360(data_00408C60);
        }
    }
}

/* a frame of the zoom: a new zoom handled; at the goal, the goal's handler once; else the current
 * zoom (the camera's) stepped toward the goal and given to the app camera -- or, while the SEAL's
 * controller holds it (mode 5), only read back */
bool CameraZoom::Tick(float dT)
{
    HandleNewZoom();
    if (data_003DC340 == data_003DC338 && data_00408C68) {
        OnGoal();
        data_00408C68 = 0;
        data_00408C58->m_zooming = 0;
        return true;
    }
    CEntityCtrl *control = data_00408C58 ? data_00408C58->m_control : 0;

    if (control && control->m_221 == 5) {
        data_003DC338 = appCamera->m_camera->m_screenAspect[0];
    } else {
        float cur = appCamera->m_camera->m_screenAspect[0];
        float step = dT * data_00408C48;

        data_003DC338 = cur;
        if (data_003DC340 >= cur) {
            data_003DC338 += step;
            if (data_003DC338 > data_003DC340) {
                data_003DC338 = data_003DC340;
                data_00408C48 = 0.0f;
            }
        } else {
            data_003DC338 -= step;
            if (data_003DC338 < data_003DC340) {
                data_003DC338 = data_003DC340;
                data_00408C48 = 0.0f;
            }
        }
    }
    appCamera->SetZoom(data_003DC338);
    return false;
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
