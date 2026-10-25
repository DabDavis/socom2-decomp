/* camera.cpp -- the player's camera (SOCOM 1's Apps/FTS/camera.cpp; OVL1 to 0x29CEBF). Defined
 * in reverse of their image order (-inline deferred). */

#include <stdlib.h>
#include "camera.h"
#include "../../gamez/zsys/zsys.h"

/* unnamed yet */
extern "C" void *func_0032EE10(_zrdr *rdr, const char *tag);

extern bool camera_tether_intersects_something;
extern "C" bool func_002C2FD0(CZSealBody *seal);
extern "C" CCameraPreset *data_0044C408;          /* the views' presets */
extern "C" void func_002956E0(CAppCamera *cam, CZSealBody *prey, int arg);

/* the file's statics, unnamed yet in data.txt (SOCOM 1's names where it had them; the shake's are
 * SOCOM II's: -1 is no shake) */
extern "C" {
extern float data_003DE218;   /* degrees to radians */
extern float data_003DE220;   /* the shake's x angle (90) */
extern float data_003DE228;   /* its x rate */
extern float data_003DE230;   /* its y rate */
extern float data_003DE238;   /* its x base amplitude */
extern float data_003DE240;   /* its x random amplitude */
extern float data_003DE248;   /* its x decay */
extern float data_003DE250;   /* its y base amplitude */
extern float data_003DE258;   /* its y random amplitude */
extern float data_003DE260;   /* its y decay */
extern float data_003DE268;   /* save_short_tether_length */
extern float data_00416018;   /* the shake's y angle */
extern float data_00416020;   /* its x amplitude */
extern float data_00416028;   /* its y amplitude */
extern float data_00416208;   /* DeltaRot */
extern float data_00416210;   /* DeltaAim */
extern float data_00416218;   /* (SOCOM II's) */
}

/* what the tick reaches for (SOCOM 1's names in the comments) */
extern "C" {
float expf(float x);
bool func_00299C00();                            /* zdb::CCamera::GetDynamicsControl (m_dynamics_controlled) */
void func_0029A180(CAppCamera *cam);             /* (SOCOM II's: into mode 1 from the tether) */
void func_00295F20(CAppCamera *cam, CMatrix &cTw, float dT);   /* (mode 1's camera) */
void func_00296F10(CAppCamera *cam, CMatrix &cTw, float dT);   /* (the tether's: APTeatherCam's place) */
void func_00295B00(CAppCamera *cam, CMatrix &cTw, float dT);   /* (first person's: APFirstPersonCam's place) */
void func_002964F0(CAppCamera *cam, CMatrix &cTw, float dT);
void func_005AA6E0(CZSealBody *seal, CPnt3D *pos, CMatrix &cTw);   /* CZSealBody::CheckDIShoot */
}

/* the character's tuning (SOCOM 1's CharacterDynamics: only the peek's rate here) */
class CharacterDynamics {
public:
    unsigned char _0[0x16c];
    float m_cameraPeekDecayRate;   // 0x16c
};

extern CharacterDynamics theCharacterDynamics;
extern unsigned char OurGameName;

/* the system: its timer's seconds per tick (SOCOM 1's _zsys_public::timerScale at 0x24; SOCOM II's
 * at 0x28) */
struct _zsys_public {
    unsigned char _0[0x28];
    float timerScale;   // 0x28
};

extern _zsys_public zSys;

#define T0_COUNT ((volatile unsigned int *)0x10000000)

namespace {
/* the peek eased toward its goal (SOCOM 1's file static) */
float peekCur;
}

void FirstToThirdPersonTransition(CAppCamera *cam);
void ThirdToFirstPersonTransition(CAppCamera *cam);

/* the network (only its online flag) */
class CNetwork {
public:
    unsigned char _0[9];
    unsigned char m_9;   // 0x9: an online game
};

extern CNetwork theNetwork;

/* the hit nearest the ray's tail, skipping hits on pieces of kind 1 too close to p0 (their
 * direction from p0 measured first); the camera's tether marked as hitting something */
zdb::IntersectStruct *GetNearestIntersectCamPoint(zdb::DiIntersect *diSet, CPnt3D *p0)
{
    zdb::IntersectStruct *Di = 0;

    if (diSet->m_Cnt <= 0)
        return 0;
    camera_tether_intersects_something = true;
    float Dist = 1.0e9f;

    for (int i = 0; i < diSet->m_Cnt; i++) {
        zdb::IntersectStruct *is = &diSet->m_Intersects[i];
        CPnt3D intersect = is->m_pos;

        intersect.Sub(p0, &intersect);
        float len = intersect.Normalize();

        if (((zdb::CDIKind *)is->m_poly)->Kind() == 1 && len <= 2.75f)
            continue;
        CPnt3D newDelta = is->m_pos - diSet->m_Tail;
        float d = Dot(newDelta, newDelta);

        if (d <= Dist) {
            Di = is;
            Dist = d;
        }
    }
    return Di;
}

/* the view's preset taken up: the goal position and aim, the tether's length, and -- with a SEAL
 * attached -- the camera put there, looking at the aim, both carried into the SEAL's space (the
 * name ours) */
void CAppCamera::ResetView()
{
    m_camGoalPos = data_0044C408[m_ctrl_view].m_tie;
    m_cameraAim = data_0044C408[m_ctrl_view].m_aim;
    m_goal_t_len = m_camGoalPos.Length();
    m_cameraTie = m_cameraAim;
    if (m_entity) {
        CMatrix cTw;
        CPnt3D tie = m_camGoalPos;

        m_entity->GetMatrix()->Transform(&tie, 1);
        CPnt3D aim = m_cameraAim;

        m_entity->GetMatrix()->Transform(&aim, 1);
        LookAt(&tie, &aim, cTw);
        m_camera->SetMatrix(&cTw);
    }
}

/* the camera's matrix to look from one point at another: level (its x axis in the ground plane),
 * up from the cross product, looking down -z; the eye noted.
 * OPEN (0x184 of 0x158): MW keeps the views' second and third components' addresses in saved
 * registers (as SOCOM 1's demo build did, s3/s4) where SOCOM II's reads them off the stack, and the
 * initialised vectors come from a constant; written with element stores it is 0x170 */
void CAppCamera::LookAt(const CPnt3D *from, const CPnt3D *to, CMatrix &cTw)
{
    CPnt3D vView = *to - *from;

    vView.Normalize();
    CPnt3D v2D = vView;

    v2D.y = 0.0f;
    v2D.Normalize();
    sceVu0FVECTOR right = { -v2D.z, 0.0f, v2D.x, 1.0f };
    sceVu0FVECTOR view = { vView.x, vView.y, vView.z, 1.0f };
    sceVu0FVECTOR up;

    sceVu0OuterProduct(up, right, view);
    cTw.m_matrix[0][0] = -v2D.z;
    cTw.m_matrix[0][1] = 0.0f;
    cTw.m_matrix[0][2] = v2D.x;
    cTw.m_matrix[1][0] = up[0];
    cTw.m_matrix[1][1] = up[1];
    cTw.m_matrix[1][2] = up[2];
    cTw.m_matrix[2][0] = -vView.x;
    cTw.m_matrix[2][1] = -vView.y;
    cTw.m_matrix[2][2] = -vView.z;
    *(CPnt3D *)cTw.m_matrix[3] = *from;
    m_wPos = *from;
}

/* the camera's ZAnim commands: the third person test and SOCOM II's shake */
void CAppCamera::RegisterAnimCommands()
{
    ZAnim.AddCmd("CAMERA_3RD_PERSON", CmdParse3rdPersonTest, 0, CmdTick3rdPersonTest, 0);
    ZAnim.AddCmd("CAMERA_SHAKE", CmdParseShake, CmdBeginShake, 0, 0);
}

/* 3RD_PERSON: a timeless test */
_zanim_cmd_hdr *CAppCamera::CmdParse3rdPersonTest(_zrdr *rdr)
{
    _zanim_cmd_hdr *cmd;

    if (!rdr)
        return 0;
    cmd = (_zanim_cmd_hdr *)_calloc(1, sizeof(_zanim_cmd_hdr), 0, 0);
    cmd->data_size = sizeof(_zanim_cmd_hdr);
    cmd->timeless = 1;
    return cmd;
}

/* 3RD_PERSON: whether the app's camera is out of first person (SOCOM II's mode 3; SOCOM 1's 2) */
bool CAppCamera::CmdTick3rdPersonTest(_zanim_cmd_hdr *hdr, float *dT)
{
    return !(appCamera->m_camera_mode == cam_mode_FP);
}

/* SHAKE: SMALL, MEDIUM or LARGE (the name ours: SOCOM II's command) */
_zanim_cmd_hdr *CAppCamera::CmdParseShake(_zrdr *rdr)
{
    _camera_shake_cmd *cmd = (_camera_shake_cmd *)_calloc(1, sizeof(_camera_shake_cmd), 0, 0);

    cmd->hdr.data_size = sizeof(_camera_shake_cmd);
    cmd->hdr.timeless = 1;
    if (rdr && rdr->type == 4) {
        if (func_0032EE10(rdr, "SMALL"))
            cmd->level = 0;
        else if (func_0032EE10(rdr, "MEDIUM"))
            cmd->level = 1;
        else if (func_0032EE10(rdr, "LARGE"))
            cmd->level = 2;
    }
    return (_zanim_cmd_hdr *)cmd;
}

/* SHAKE: the app camera shaken at the command's level.
 * OPEN (36 of 113 instructions, registers only): the target keeps appCamera in v1 and the level in
 * a0 (the command's register, dead after the load); MW gives the level v1 and appCamera a1 -- the
 * same with a local camera, an early return, the level read first or as an int, Shake declared
 * inline, and with the switch written out here */
void CAppCamera::CmdBeginShake(_zanim_cmd_hdr *hdr)
{
    if (appCamera)
        appCamera->Shake(((_camera_shake_cmd *)hdr)->level);
}

CAppCamera::~CAppCamera()
{
    m_track = 0;
    m_skel_root = 0;
    m_entity = 0;
    m_camera_effects = 0;
    m_camera = 0;
}

/* the camera put on a SEAL: the first view's preset, the SEAL taken up, the camera placed behind
 * it looking at the aim, the nominal zoom read back (twice over, both builds) */
bool CAppCamera::AttachTo(CZSealBody *prey)
{
    if (!prey)
        return false;
    m_save_view = cam_view_third;
    m_ctrl_view = cam_view_third;
    m_save_peek = 2;
    m_camGoalPos = data_0044C408[m_ctrl_view].m_tie;
    m_cameraAim = data_0044C408[m_ctrl_view].m_aim;
    m_goal_t_len = m_camGoalPos.Length();
    m_cameraTie = m_cameraAim;
    func_002956E0(this, prey, 0);
    CPnt3D tie = m_camGoalPos;

    m_entity->GetMatrix()->Transform(&tie, 1);
    CPnt3D aim = m_cameraAim;

    m_entity->GetMatrix()->Transform(&aim, 1);
    CMatrix cTw;

    LookAt(&tie, &aim, cTw);
    m_camera->SetMatrix(&cTw);
    zdb::CCamera *cam = m_camera;

    m_z_nominal = cam->m_screenAspect[0];
    m_z_nominal = cam->m_screenAspect[1];
    return true;
}

/* the camera's mode -- kept at 6 online while the SEAL it follows is down (the name ours) */
void CAppCamera::SetCameraMode(unsigned char mode)
{
    bool keep = false;

    if (theNetwork.m_9 && m_bc && func_002C2FD0(m_bc) && m_camera_mode == cam_mode_6)
        keep = true;
    if (!keep)
        m_camera_mode = (PLAYER_CAM_STATE)mode;
}

/* the zoom: the screen's aspect scaled by it, the range scale divided by it */
void CAppCamera::SetZoom(float zoom)
{
    zdb::CCamera *cam = m_camera;
    float y = zoom * zVid.aspect[1];
    float x = zoom * zVid.aspect[0];

    cam->m_screenAspect[0] = x;
    cam->m_screenAspect[1] = y;
    m_camera->m_ZoomedRangeScale = m_camera->m_RangeScale / zoom;
}

/* a frame of the shake (SOCOM 1's wiggle, rewritten): each axis's angle stepped by its rate and
 * wrapped, the screen offset rippled by its amplitude (x by the cosine, y by the sine), the
 * amplitudes decayed to nothing -- and with both gone, the shake stopped */
void CAppCamera::TickCameraWiggle(float dT, zdb::CCamera *cam)
{
    if (!m_wiggle_echo)
        return;
    data_003DE220 += dT * data_003DE228;
    if (data_003DE220 > 360.0f)
        data_003DE220 -= 360.0f;
    float cosval = cosf(data_003DE220 * data_003DE218);
    float ScaledAmplitude = data_00416020 * cosval;

    cam->SetScreenOffsetX(ScaledAmplitude + cam->m_screenOffset[0]);
    data_00416018 += dT * data_003DE230;
    if (data_00416018 > 360.0f)
        data_00416018 -= 360.0f;
    float sinval = sinf(data_00416018 * data_003DE218);

    ScaledAmplitude = data_00416028 * sinval;
    cam->SetScreenOffsetY(ScaledAmplitude + cam->m_screenOffset[1]);
    data_00416020 -= data_003DE248;
    if (data_00416020 < 0.0f)
        data_00416020 = 0.0f;
    data_00416028 -= data_003DE260;
    if (data_00416028 < 0.0f)
        data_00416028 = 0.0f;
    if (data_00416020 == 0.0f && data_00416028 == 0.0f)
        StopShake();
}

/* the scheduler's task for the app camera: its tick (SOCOM II's, the name ours) */
bool CAppCamera::TickTask(float dT, void *data)
{
    appCamera->Tick(dT);
    return false;
}

/* a frame of the camera, while the dynamics hold it: the view's buttons (FTSTick), the peek eased
 * toward the SEAL's (none while dead or prone), the mode's camera -- entering mode 1 or first
 * person from the tether, or the tether from anything, through a transition -- the
 * effects' matrix on top, the camera's node placed, the SEAL's shot checked against it (SOCOM 2,
 * not in mode 6) and the shake (SOCOM 1's Tick; SOCOM II's modes 1-6 and their tick functions
 * take the frame time) */
void CAppCamera::Tick(float dT)
{
    if (func_00299C00()) {
        CMatrix camMatrix;
        float peekGoal;

        FTSTick();
        peekGoal = 0.0f;
        if (m_bc) {
            if (m_bc->m_state == 8 || m_bc->m_state == 2)
                peekGoal = 0.0f;
            else if (m_bc->m_desiredPeek == PEEK_LEFT)
                peekGoal = -1.0f;
            else if (m_bc->m_desiredPeek == PEEK_RIGHT)
                peekGoal = 1.0f;
        }
        peekCur = peekGoal + (peekCur - peekGoal) *
            expf(-theCharacterDynamics.m_cameraPeekDecayRate * ((float)*T0_COUNT * zSys.timerScale));
        switch (m_camera_mode) {
        case cam_mode_1:
            if (FromTetherToMode1())
                func_0029A180(this);
            func_00295F20(this, camMatrix, dT);
            break;
        case cam_mode_tether:
            if (IntoTether())
                FirstToThirdPersonTransition(this);
            func_00296F10(this, camMatrix, dT);
            break;
        case cam_mode_FP:
            if (FromTetherToFirst())
                ThirdToFirstPersonTransition(this);
            func_00295B00(this, camMatrix, dT);
            break;
        case cam_mode_apLook:
            APLookAt(dT, m_track, camMatrix);
            break;
        case cam_mode_5:
            break;
        case cam_mode_6:
            func_002964F0(this, camMatrix, dT);
            break;
        default:
            func_00296F10(this, camMatrix, dT);
            break;
        }
        m_camera_last_mode = m_camera_mode;
        if (m_camera_effects) {
            CMatrix tmp_mtx;

            sceVu0MulMatrix(tmp_mtx.m_matrix, camMatrix.m_matrix, *(sceVu0FMATRIX *)m_camera_effects);
            sceVu0CopyMatrix(camMatrix.m_matrix, tmp_mtx.m_matrix);
        }
        m_camera->SetMatrix(&camMatrix);
        if (OurGameName == 1 && m_bc && m_camera_mode != cam_mode_6)
            func_005AA6E0(m_bc, &m_wPos, camMatrix);
        if (m_wiggle_echo)
            TickCameraWiggle(dT, m_camera);
    }
}

/* SOCOM 1's: the wiggle's echo, its clock restarted when set */
inline void CAppCamera::WiggleEcho(bool flag)
{
    m_wiggle_echo = flag;
    if (flag)
        m_wiggle_cur_time = 0.0f;
}

/* the shake stopped: its parameters forgotten, its angles put back to a random quarter (x at 90 or
 * 270 degrees, y at 0 or 180), its amplitudes nothing (the name ours) */
void CAppCamera::StopShake()
{
    data_003DE228 = -1.0f;
    data_003DE230 = -1.0f;
    data_003DE238 = -1.0f;
    data_003DE240 = -1.0f;
    data_003DE248 = -1.0f;
    data_003DE250 = -1.0f;
    data_003DE258 = -1.0f;
    data_003DE260 = -1.0f;
    WiggleEcho(false);
    data_003DE220 = rand() % 2 ? 90.0f : 270.0f;
    data_00416018 = rand() % 2 ? 0.0f : 180.0f;
    data_00416020 = 0.0f;
    data_00416028 = 0.0f;
}

/* a shake begun or strengthened: each rate and decay the larger of old and new, each base
 * amplitude the new one if it beats the running amplitude; when both do, the amplitudes rolled
 * afresh (base plus up to 99% of the random part, kept if larger) and the shake set going (the name
 * ours) */
void CAppCamera::BeginShake(float xAmp, float xAmpRand, float xDecay, float xRate, float yAmp,
                            float yAmpRand, float yDecay, float yRate)
{
    data_003DE228 = xRate > data_003DE228 ? xRate : data_003DE228;
    data_003DE230 = yRate > data_003DE230 ? yRate : data_003DE230;
    bool restart = xAmp > data_00416020 && yAmp > data_00416028;

    data_003DE238 = xAmp > data_00416020 ? xAmp : data_003DE238;
    data_003DE240 = xAmpRand;
    data_003DE248 = xDecay > data_003DE248 ? xDecay : data_003DE248;
    data_003DE250 = yAmp > data_00416028 ? yAmp : data_003DE250;
    data_003DE258 = yAmpRand;
    data_003DE260 = yDecay > data_003DE260 ? yDecay : data_003DE260;
    if (restart) {
        float amp = data_003DE238;

        amp += data_003DE240 * (0.01f * (rand() % 100));
        if (amp > data_00416020)
            data_00416020 = amp;
        amp = data_003DE250;
        amp += data_003DE258 * (0.01f * (rand() % 100));
        if (amp > data_00416028)
            data_00416028 = amp;
        WiggleEcho(true);
    }
}

/* the camera shaken with one of its presets: SMALL, MEDIUM or LARGE; anything else stops it (the
 * name ours) */
void CAppCamera::Shake(unsigned char level)
{
    switch (level) {
    case 0:
        BeginShake(m_shake[0].m_xAmp, m_shake[0].m_xAmpRand, m_shake[0].m_xDecay, m_shake[0].m_xRate,
                   m_shake[0].m_yAmp, m_shake[0].m_yAmpRand, m_shake[0].m_yDecay, m_shake[0].m_yRate);
        break;
    case 1:
        BeginShake(m_shake[1].m_xAmp, m_shake[1].m_xAmpRand, m_shake[1].m_xDecay, m_shake[1].m_xRate,
                   m_shake[1].m_yAmp, m_shake[1].m_yAmpRand, m_shake[1].m_yDecay, m_shake[1].m_yRate);
        break;
    case 2:
        BeginShake(m_shake[2].m_xAmp, m_shake[2].m_xAmpRand, m_shake[2].m_xDecay, m_shake[2].m_xRate,
                   m_shake[2].m_yAmp, m_shake[2].m_yAmpRand, m_shake[2].m_yDecay, m_shake[2].m_yRate);
        break;
    case 4:
    case 5:
    default:
        StopShake();
        break;
    }
}

/* the death camera put back: no death state, the automatic death camera on, the short tether's
 * saved length long, the rotation and aim deltas nothing (SOCOM 1's; SOCOM II puts back a few more) */
void CAppCamera::ResetDeathCam()
{
    m_cam_death_state = 0;
    m_auto_death_cam = 1;
    data_003DE268 = 10000.0f;
    data_00416208 = 0.0f;
    data_00416210 = 0.0f;
    data_00416218 = 0.0f;
    m_death_state_timer = 0.0f;
    m_f4 = 0;
    m_f5 = m_f4;
    m_110 = -1;
    m_114 = 0.0f;
    m_104 = 0.0f;
    m_108 = 0.0f;
    m_10c = 0.0f;
    m_118 = 1.0f;
}
