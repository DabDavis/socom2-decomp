/* thezoom.cpp -- the scope's zoom: its goal, the HUD and SEAL it serves, and seeing farther while
 * zoomed (SOCOM 1's Apps/FTS/thezoom.cpp; OVL1 0x1F0490-0x1F19FF). Defined in reverse of their
 * image order (-inline deferred). */

#include "camera.h"

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
}

/* the base fog colour by value (the name ours: SeeFarther copies it out twice) */
static inline CPnt4D BaseFogColor() { return data_004B4AB0; }

/* the scope's zoom (SOCOM 1's: a class of statics) */
class CameraZoom {
public:
    static bool Tick(float dT);
    static void OnGoal();
    static void HandleNewZoom();
    static void SetGoal(float goal);
    static void AttachHud(CHUD *hud);
    static void AttachSeal(CZSealBody *seal);
};


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
