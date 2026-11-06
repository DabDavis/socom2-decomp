/* camera.cpp -- the player's camera (SOCOM 1's Apps/FTS/camera.cpp; OVL1 to 0x29CEBF). Defined
 * in reverse of their image order (-inline deferred). */

#include <stdlib.h>
#include <iostream>
#include "camera.h"
#include "../../gamez/zsys/zsys.h"
#include "../../gamez/zinput/zinput.h"

/* SOCOM 1's, not written yet */
char *zrdr_findstring(const _zrdr *rdr, const char *tag);

extern bool camera_tether_intersects_something;

/* the camera's file statics, as __sinit_camera.cpp (0x3FFDD0) builds them after <iostream>'s pair
 * (its __nInit/__wInit at 0x415FD0/0x415FE8): SOCOM 1's four intersection sets dilr0-dilr3
 * (0x416050/0x4160B0/0x416110/0x416170, each registered with ~DiIntersect 0x2DC680), SOCOM 1's
 * tetherPos (0x4161C8) and three points new in SOCOM II -- GetFirstPersonCamPos's (0x4161D8),
 * GetThirdPersonCamPos's (0x4161E8) and 0x29A660's (0x4161F8); the three names ours. Their users
 * still reach them by address, so they are referenced nowhere yet */
static zdb::DiIntersect dilr0(16);
static zdb::DiIntersect dilr1(16);
static zdb::DiIntersect dilr2(16);
static zdb::DiIntersect dilr3(16);
static CPnt3D tetherPos(0.0f, 8.0f, 40.0f);
static CPnt3D firstPersonPos(0.0f, 2.5f, 4.0f);
static CPnt3D thirdPersonPos(0.0f, 5.2f, 15.0f);
static CPnt3D thirdPersonPos2(0.0f, 5.2f, 15.0f);
extern "C" bool func_002C2FD0(CZSealBody *seal);
CZSealBody *ftsGetPlayer();
extern "C" CCameraPreset *data_0044C408;          /* the views' presets */

/* the tether's file statics (data 0x3DE268-0x3DE28F, after the shake's): SOCOM 1's
 * save_short_tether_length, then -- in the place its Y_FP_OFFSET held -- the rate the short tether
 * eases by (FTSTick puts back 0.03), SOCOM 1's CamRotZ_down / CamRotZ_up (the level offset per unit
 * of look pitch, the pitch not negative / negative: -7 / -3 in SOCOM 1) and between them SOCOM II's
 * -5, a mounted or climbing SEAL's (two names ours) */
static float save_short_tether_length = 10000.0f;
static float short_tether_rate = 0.03f;
static float CamRotZ_down = -8.0f;
static float CamRotZ_mounted = -5.0f;
static float CamRotZ_up = -3.0f;

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
extern float data_00416018;   /* the shake's y angle */
extern float data_00416020;   /* its x amplitude */
extern float data_00416028;   /* its y amplitude */
extern float data_00416208;   /* DeltaRot */
extern float data_00416210;   /* DeltaAim */
extern float data_00416218;   /* (SOCOM II's) */
extern unsigned int data_0044C404;   /* the views' count (their presets follow) */
}
/* mode 1's camera position, its offset and length handed back (SOCOM II's: GetThirdPersonCamPos's
 * shape, not written) */
extern "C" CPnt3D func_0029A660(CAppCamera *cam, CPnt3D *CamOffset, float *tetherLen);
/* whether a SEAL is mounted (OVL2 0x54F6E0, seal.cpp's) */
extern "C" bool func_0054F6E0(CZSealBody *seal, bool blend, bool mount, bool anims);
/* two SEAL state tests (OVL2: seal_kit.cpp's and seal_env.cpp's) */
extern "C" bool func_005A78D0(CZSealBody *body);
extern "C" bool func_005A77E0(CZSealBody *seal, int state33, int anim36, int anim35);
/* the death camera's pad handling (SOCOM II's: a member of the object at 0xF4) */
struct CDeathCam;
extern "C" void func_00295260(CDeathCam *dc, CAppCamera *cam, CPad *pad);
/* the SEAL nearest the death camera's view line taken to watch (SOCOM II's, not written) */
extern "C" void func_00294F40(CDeathCam *dc, CAppCamera *cam);

/* what the tick reaches for (SOCOM 1's names in the comments) */
extern "C" {
float expf(float x);
bool func_00299C00();                            /* zdb::CCamera::GetDynamicsControl (m_dynamics_controlled) */
void func_0029A180(CAppCamera *cam);             /* (SOCOM II's: into mode 1 from the tether) */
void func_00295F20(CAppCamera *cam, CMatrix &cTw, float dT);   /* (mode 1's camera) */
void func_002964F0(CAppCamera *cam, CMatrix &cTw, float dT);
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
class CZNetwork {
public:
    unsigned char _0[1];
    unsigned char m_9;   // 0x1: an online game (SOCOM 1's at 0x9)
};

extern CZNetwork theNetwork;

/* the world's camera collision switch (seal_control.cpp's) and an entity's node (CEntity's,
 * 0x29ED20: camera.h's SEAL is a stand-in, so called by its map name) */
namespace zdb {
void SetCameraCollideFlagRequired(bool on);
}
extern "C" zdb::CNode *GetNode__7CEntityFv(CZSealBody *e);

#pragma auto_inline off
/* the chord tested against the world with the followed SEAL's node, and that of the SEAL it
 * carries (0xEAC, inside the stand-in's kit), switched off -- and the camera collision flag cleared --
 * then both switched back: whether it hit, the nearest hit point given (SOCOM II's). Pinned: its one
 * caller, GetThirdPersonCamPos, calls it out of line (its statements over the auto-inline limit in the
 * original, as compiled-out ones count) */
bool CAppCamera::ClipChord(CPnt3D *from, CPnt3D *to, CPnt3D *hit)
{
    bool found = false;
    bool chars = m_cam_death_state != 0;
    bool was = GetNode__7CEntityFv(m_entity)->SetActive(false);

    zdb::SetCameraCollideFlagRequired(false);

    CZSealBody *carried = m_bc ? *(CZSealBody **)((char *)m_bc + 0xeac) : 0;
    bool carriedWas;

    if (carried)
        carriedWas = GetNode__7CEntityFv(carried)->SetActive(false);

    zdb::IntersectStruct *is = GetNearestIntersectCamPoint(zdb::CWorld::m_world->GetIntersects(from, to, chars), from);

    if (is) {
        found = true;
        *hit = is->m_pos;
    }
    GetNode__7CEntityFv(m_entity)->SetActive(was);
    if (carried)
        GetNode__7CEntityFv(carried)->SetActive(carriedWas);
    return found;
}
#pragma auto_inline reset

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

/* LookAt's pieces (the names ours): three floats set by value -- the arguments loaded z, y, x
 * before the stores, as the eye's copy is -- the level right vector from the negated z it shares
 * with the matrix and the direction's x read through the reference (a second load of it, as the
 * target's), and a row set to a point's negation through the reference (its fields come off the
 * frame, where read directly MW keeps their addresses in saved registers) */
inline void LaSet3(float *r, float x, float y, float z)
{
    r[0] = x;
    r[1] = y;
    r[2] = z;
}

inline void LaRight(float *v, float nz, const CPnt3D &d)
{
    v[0] = nz;
    v[1] = 0.0f;
    v[2] = d.x;
    v[3] = 1.0f;
}

inline void LaNegRow(float *r, const CPnt3D &p)
{
    r[0] = -p.x;
    r[1] = -p.y;
    r[2] = -p.z;
}

/* the camera's matrix to look from one point at another: level (its x axis in the ground plane),
 * up from the cross product, looking down -z; the eye noted (SOCOM 1's; written in the engine
 * core's round 15, build/scratch/core/v_lookat*.py: SOCOM 1's demo build kept the view's y and z
 * addresses in saved registers, SOCOM II's 3.0 build reads every field off the frame -- exactly
 * one of the view's two readers going through a reference inline gives that; the right vector's
 * negated z and the direction's x live across the cross product only as named floats, and the
 * cross product's three results are named before row 0 is stored -- the target loads them first) */
void CAppCamera::LookAt(const CPnt3D *from, const CPnt3D *to, CMatrix &cTw)
{
    CPnt3D vView = *to - *from;

    vView.Normalize();
    CPnt3D v2D = vView;

    v2D.y = 0.0f;
    v2D.Normalize();
    float nz = -v2D.z;
    float x = v2D.x;
    sceVu0FVECTOR up;
    sceVu0FVECTOR right;
    sceVu0FVECTOR view;
    LaRight(right, nz, v2D);
    view[0] = vView.x;
    view[1] = vView.y;
    view[2] = vView.z;
    view[3] = 1.0f;
    sceVu0OuterProduct(up, right, view);
    float ux = up[0];
    float uy = up[1];
    float uz = up[2];
    cTw.m_matrix[0][0] = nz;
    cTw.m_matrix[0][1] = 0.0f;
    cTw.m_matrix[0][2] = x;
    cTw.m_matrix[1][0] = ux;
    cTw.m_matrix[1][1] = uy;
    cTw.m_matrix[1][2] = uz;
    LaNegRow(cTw.m_matrix[2], vView);
    LaSet3(cTw.m_matrix[3], from->x, from->y, from->z);
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
        if (zrdr_findstring(rdr, "SMALL"))
            cmd->level = 0;
        else if (zrdr_findstring(rdr, "MEDIUM"))
            cmd->level = 1;
        else if (zrdr_findstring(rdr, "LARGE"))
            cmd->level = 2;
    }
    return (_zanim_cmd_hdr *)cmd;
}

/* SHAKE: the app camera shaken at the command's level.
 * Byte-exact since the engine core's round 27 (36 off since round 13): the level an enum-typed local
 * (ours, the type in no DWARF). The target numbers the level below the camera (a common subexpression
 * in pass 0): folded into Shake's expanded parameter its load is a code-generation register above the
 * camera, and the pair trade v1 / a1. A plain, const or int local is propagated; enums are not (the
 * game states' round 53). */
void CAppCamera::CmdBeginShake(_zanim_cmd_hdr *hdr)
{
    if (appCamera) {
        enum ShakeLevel { SL_0 = 0, SL_MAX = 255 };
        ShakeLevel level = (ShakeLevel)((_camera_shake_cmd *)hdr)->level;

        appCamera->Shake(level);
    }
}

/* SOCOM 1's (SOCOM II's takes the camera alone): the aim, tie and goal, the camera kept, the
 * death camera's and the views' state put back, the tether's length from the goal, the player taken
 * up, the shake stopped and its three presets set -- SMALL, MEDIUM, LARGE */
CAppCamera::CAppCamera(zdb::CCamera *cam)
    : m_cameraAim(0.0f, 20.0f, 0.0f), m_cameraTie(0.0f, 20.0f, 0.0f), m_camGoalPos(40.0f, 20.0f, 0.0f),
      m_44(1), m_45(0), m_46(1), m_short_tether_timer(-1.0f), m_padid(0), m_camera(cam),
      m_camera_effects(0), m_bc(0), m_entity(0), m_ctrl(0), m_z_nominal(500.0f)
{
    m_f8 = 0.0f;
    m_fc = 0.0f;
    m_100 = 0.0f;
    m_f4 = 0;
    m_f5 = m_f4;
    m_110 = -1;
    m_114 = 0;
    m_104 = 0.0f;
    m_108 = 0.0f;
    m_10c = 0;
    m_118 = 1.0f;
    m_camera_mode = cam_mode_tether;
    m_camera_last_mode = cam_mode_tether;
    m_cam_death_state = 0;
    m_killed = 0;
    m_killer = 0;
    m_goal_t_len = m_camGoalPos.Length();
    Follow((CEntity *)ftsGetPlayer(), 0);
    m_wiggle_cur_time = 0.0f;
    m_wiggle_echo = 0;
    StopShake();
    m_shake[0].m_xAmp = 6.0f;
    m_shake[0].m_xAmpRand = 1.0f;
    m_shake[0].m_xDecay = 2.0f;
    m_shake[0].m_xRate = 2000.0f;
    m_shake[0].m_yAmp = 6.0f;
    m_shake[0].m_yAmpRand = 0.5f;
    m_shake[0].m_yDecay = 2.0f;
    m_shake[0].m_yRate = 250.0f;
    m_shake[1].m_xAmp = 20.0f;
    m_shake[1].m_xAmpRand = 10.0f;
    m_shake[1].m_xDecay = 2.0f;
    m_shake[1].m_xRate = 2000.0f;
    m_shake[1].m_yAmp = 10.0f;
    m_shake[1].m_yAmpRand = 5.0f;
    m_shake[1].m_yDecay = 2.0f;
    m_shake[1].m_yRate = 2000.0f;
    m_shake[2].m_xAmp = 60.0f;
    m_shake[2].m_xAmpRand = 30.0f;
    m_shake[2].m_xDecay = 4.0f;
    m_shake[2].m_xRate = 1000.0f;
    m_shake[2].m_yAmp = 55.0f;
    m_shake[2].m_yAmpRand = 15.0f;
    m_shake[2].m_yDecay = 4.0f;
    m_shake[2].m_yRate = 1150.0f;
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
    Follow((CEntity *)prey, 0);
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

/* SOCOM 1's FTSTick: the followed SEAL's pad (its controller's, a player's), its two-button view
 * cycle stepping past the side, first person and peek views, the death camera's own pad handling
 * (SOCOM II's, at 0xF4), then -- the view changed -- its preset taken up (ResetView, SOCOM II's
 * call), first person or the tether entered through SetCameraMode and the tether's rate put back.
 * The view's step casts it to an int: the loop's copy then reuses the test's load re-narrowed (the
 * target's andi), where the plain enum reuses it as it is */
void CAppCamera::FTSTick()
{
    if (!m_entity)
        return;
    if (m_ctrl && m_ctrl->IsPlayer())
        m_padid = m_ctrl->m_padid;
    CPad *pad = m_padid < CInput::NUM_PADS ? CInput::m_pads[m_padid] : 0;

    if (!pad)
        pad = CInput::m_pads[0];
    if (pad) {
        if (pad->GetTwoButtons(PAD_SELECT, PAD_DOWN)) {
            m_ctrl_view = (CAMVIEW)(((int)m_ctrl_view + 1) % data_0044C404);
            while (m_ctrl_view == cam_view_side || m_ctrl_view == cam_view_first ||
                   m_ctrl_view == cam_view_peek_left || m_ctrl_view == cam_view_peek_right)
                m_ctrl_view = (CAMVIEW)(((int)m_ctrl_view + 1) % data_0044C404);
        }
        func_00295260((CDeathCam *)&m_f4, this, pad);
    }
    if (m_ctrl_view != m_save_view) {
        ResetView();
        m_save_view = m_ctrl_view;
        if (m_ctrl_view == cam_view_first) {
            SetCameraMode(cam_mode_FP);
            short_tether_rate = 0.03f;
        } else {
            SetCameraMode(cam_mode_tether);
            short_tether_rate = 0.03f;
        }
    }
}

/* where first person puts the camera, in the followed SEAL's space (SOCOM 1's): with the camera on
 * the SEAL it follows, the skeleton root's place (or the origin), then -- prone -- lifted by the
 * view's pitch and scaled by the SEAL's 0x1344 (SOCOM II's), else lifted 4.4 and pulled 2.5 back
 * unless the SEAL is in one of two states, and moved aside by the peek; on another SEAL, the
 * file's fixed point */
CPnt3D GetFirstPersonCamPos(CAppCamera *cam)
{
    CPnt3D CamPoint;

    if (cam->m_entity == cam->m_bc) {
        if (cam->m_entity && cam->m_skel_root)
            cam->m_bc->PartInSpace(cam->m_skel_root, 0, &CamPoint, 0);
        else
            CamPoint = CPnt3D::zero;
        if (cam->m_bc->m_state != 2) {
            CamPoint.y += 4.4f;
            if (func_005A78D0(cam->m_bc) || func_005A77E0(cam->m_bc, 1, 1, 1))
                CamPoint.z = 0.0f;
            else
                CamPoint.z = -2.5f;
        } else {
            float lift = 6.0f * cam->m_camera->m_matrix.m_matrix[2][1];

            if (lift < 0.0f)
                lift = 0.0f;
            lift += 1.8f;
            CamPoint.z = -7.0f;
            CPnt3D scale = cam->m_bc->m_1344;

            if (scale.y < 0.5f)
                scale.y = 0.5f;
            lift *= 2.0f / scale.y;
            CamPoint.z *= 0.7f * scale.y;
            CamPoint.y += lift;
        }
        if (peekCur < 0.0f)
            CamPoint.x += 3.3f * peekCur;
        else if (peekCur > 0.0f)
            CamPoint.x += 4.35f * peekCur;
    } else
        CamPoint = firstPersonPos;
    return CamPoint;
}

/* where the tether puts the camera, in the followed SEAL's space (SOCOM 1's): on the SEAL it follows,
 * the body's root part's place lifted by its height (SOCOM II's ramp: 10 at the origin, else 5.5 to
 * 10 as it rises from 2.17 to 5.6), moved aside by the peek and pulled 28 back, the level offset and
 * the tether's length handed back, the point turned by the SEAL's look, the offset's z from the
 * look's pitch -- looking up, unless the player is mounted or on a ladder, the chord behind the
 * offset clipped against the world and its lost length added -- and the length eased toward 14 by
 * the pitch; on another SEAL, the file's fixed point. The root's height read into a local of its own:
 * numbered ahead of the ramp's t, it leaves t the lower FPR, as the target's */
CPnt3D CAppCamera::GetThirdPersonCamPos(CPnt3D *CamOffset, float *tetherLen)
{
    CPnt3D CamPoint;

    if (m_entity == m_bc) {
        CZBodyPart *part = m_skel_root;

        if (part != m_bc->m_root)
            part = m_bc->m_root;
        if (m_entity && part)
            m_bc->PartInSpace(part, 0, &CamPoint, 0);
        else
            CamPoint = CPnt3D::zero;
        float lift;
        float h = CamPoint.y;

        if (h == 0.0f)
            lift = 10.0f;
        else {
            float t = 1.0f;

            if (h < 5.6f) {
                t = 0.29147339f * (h - 2.1691549f);
                if (t < 0.0f)
                    t = 0.0f;
                else if (t > 1.0f)
                    t = 1.0f;
            }
            lift = 5.5f + 4.5f * t;
        }
        CamPoint.y += lift;
        if (peekCur < 0.0f)
            CamPoint.x += 2.5f * peekCur;
        else
            CamPoint.x += 2.8f * peekCur;
        CamPoint.z += 28.0f;
        CamOffset->x = CamPoint.x;
        CamOffset->y = CamPoint.y;
        CamPoint.y = 0.0f;
        *tetherLen = CamPoint.Length();
        if (m_bc)
            m_bc->m_fwdlook.Apply(&CamPoint, &CamPoint, 1);
        CPnt3D vec = CamPoint;

        vec.Normalize();
        if (vec.y < 0.0f)
            CamOffset->z = vec.y * CamRotZ_up;
        else {
            CZSealBody *player = ftsGetPlayer();

            if (player && (func_0054F6E0(player, true, true, true) || player->CanClimbLadder()))
                CamOffset->z = vec.y * CamRotZ_mounted;
            else {
                float z = vec.y * CamRotZ_down;

                if (z < CamRotZ_mounted) {
                    CPnt3D chord[2];

                    chord[0] = *CamOffset;
                    chord[1] = *CamOffset;
                    chord[1].z += CamRotZ_down;
                    m_entity->GetMatrix()->Transform(chord, 2);
                    CPnt3D hit;

                    if (ClipChord(&chord[0], &chord[1], &hit)) {
                        CPnt3D d = chord[1] - hit;

                        z += sqrtf(d.x * d.x + d.z * d.z);
                    }
                }
                CamOffset->z = z;
            }
        }
        float wt = fabsf(vec.y);

        *tetherLen = *tetherLen + wt * (14.0f - *tetherLen);
        CamPoint = vec * *tetherLen;
        CamPoint.Add(CamOffset, &CamPoint);
    } else {
        *CamOffset = thirdPersonPos;
        CamPoint = *CamOffset;
        *tetherLen = CamPoint.Length();
    }
    return CamPoint;
}

/* where mode 1 puts the camera (SOCOM II's, GetThirdPersonCamPos's first shape): on the SEAL it
 * follows, the skeleton root's place lifted 10, moved aside by the peek and pulled 49.5 back, the
 * level offset and the tether's length handed back, the point turned by the SEAL's look, the length
 * eased toward 50 by the look's pitch; on another SEAL, the file's fixed point */
extern "C" CPnt3D func_0029A660(CAppCamera *cam, CPnt3D *CamOffset, float *tetherLen)
{
    CPnt3D CamPoint;

    if (cam->m_entity == cam->m_bc) {
        if (cam->m_entity && cam->m_skel_root)
            cam->m_bc->PartInSpace(cam->m_skel_root, 0, &CamPoint, 0);
        else
            CamPoint = CPnt3D::zero;
        CamPoint.y += 10.0f;
        if (peekCur < 0.0f)
            CamPoint.x += 2.5f * peekCur;
        else
            CamPoint.x += 2.8f * peekCur;
        CamPoint.z += 49.5f;
        CamOffset->x = CamPoint.x;
        CamOffset->y = CamPoint.y;
        CamPoint.y = 0.0f;
        *tetherLen = CamPoint.Length();
        if (cam->m_bc)
            cam->m_bc->m_fwdlook.Apply(&CamPoint, &CamPoint, 1);
        CPnt3D vec = CamPoint;

        vec.Normalize();
        if (vec.y < 0.0f)
            CamOffset->z = vec.y * CamRotZ_up;
        else
            CamOffset->z = vec.y * CamRotZ_down;
        float wt = fabsf(vec.y);

        *tetherLen = *tetherLen + wt * (50.0f - *tetherLen);
        CamPoint = vec * *tetherLen;
        CamPoint.Add(CamOffset, &CamPoint);
    } else {
        *CamOffset = thirdPersonPos2;
        CamPoint = *CamOffset;
        *tetherLen = CamPoint.Length();
    }
    return CamPoint;
}

/* into first person from the tether (SOCOM 1's): the pitch of the SEAL's forward look from the
 * first-person point and that of its aim point, each wrapped into [0, 2pi], their difference added
 * to the controller's aim pitch and the player's head turned to it (SOCOM II's tests the SEAL
 * before using it) */
void ThirdToFirstPersonTransition(CAppCamera *cam)
{
    CPnt3D CamPoint = GetFirstPersonCamPos(cam);
    CPnt3D cam_aim_point(0.0f, 0.0f, 0.0f);
    CPnt3D lz(0.0f, 0.0f, -1.0f);

    if (cam->m_bc)
        cam->m_bc->m_fwdlook.Apply(&lz, &cam_aim_point, 1);
    cam_aim_point.Add(&CamPoint, &cam_aim_point);
    float pitch0 = atan2f(cam_aim_point.y - CamPoint.y, cam_aim_point.z - CamPoint.z);
    CZSealBody *seal = ftsGetPlayer();

    cam_aim_point = seal->m_aim_point;
    CMatrix WtoM;

    if (cam->m_bc) {
        Vu0MatrixInverseWithUniformScale(&WtoM, cam->m_bc->GetMatrix());
        WtoM.Transform(&cam_aim_point, 1);
    }
    float pitch1 = atan2f(cam_aim_point.y - CamPoint.y, cam_aim_point.z - CamPoint.z);

    while (pitch0 < 0.0f)
        pitch0 += 6.2831855f;
    while (pitch1 < 0.0f)
        pitch1 += 6.2831855f;
    while (pitch0 > 6.2831855f)
        pitch0 -= 6.2831855f;
    while (pitch1 > 6.2831855f)
        pitch1 -= 6.2831855f;
    float dp = pitch0 - pitch1;
    CAppCameraCtrl *pSealCtrl = cam->m_ctrl;
    float aimPitch = pSealCtrl->m_130;

    aimPitch += dp;
    pSealCtrl->m_130 = aimPitch;
    ftsGetPlayer()->RotateHead(aimPitch);
}

/* into mode 1 from the tether (SOCOM II's, FirstToThirdPersonTransition's twin over mode 1's camera
 * position, 0x29A660, without the clamp; Tick runs it): the forward look's pitch and the aim point's
 * from the mode's offset, wrapped, their difference added to the aim pitch, the head turned to it */
extern "C" void func_0029A180(CAppCamera *cam)
{
    float dummy;
    CPnt3D CamOffset;
    CPnt3D cam_new_pos = func_0029A660(cam, &CamOffset, &dummy);
    CPnt3D cam_aim_point(0.0f, 0.0f, 0.0f);
    CPnt3D lz(0.0f, 0.0f, -1000.0f);

    if (cam->m_bc)
        cam->m_bc->m_fwdlook.Apply(&lz, &cam_aim_point, 1);
    cam_aim_point.Add(&CamOffset, &cam_aim_point);
    float pitch0 = atan2f(cam_aim_point.y - CamOffset.y, cam_aim_point.z - CamOffset.z);
    CZSealBody *seal = ftsGetPlayer();

    cam_aim_point = seal->m_aim_point;
    CMatrix WtoM;

    if (cam->m_bc) {
        Vu0MatrixInverseWithUniformScale(&WtoM, cam->m_bc->GetMatrix());
        WtoM.Transform(&cam_aim_point, 1);
    }
    float pitch1 = atan2f(cam_aim_point.y - CamOffset.y, cam_aim_point.z - CamOffset.z);

    while (pitch0 < 0.0f)
        pitch0 += 6.2831855f;
    while (pitch1 < 0.0f)
        pitch1 += 6.2831855f;
    while (pitch0 > 6.2831855f)
        pitch0 -= 6.2831855f;
    while (pitch1 > 6.2831855f)
        pitch1 -= 6.2831855f;
    float dp = pitch0 - pitch1;
    CAppCameraCtrl *pSealCtrl = cam->m_ctrl;
    float aimPitch = pSealCtrl->m_130;

    aimPitch += dp;
    pSealCtrl->m_130 = aimPitch;
    ftsGetPlayer()->RotateHead(aimPitch);
}

/* into the tether from first person (SOCOM 1's): the pitch of the SEAL's forward look from the
 * third-person offset and that of its aim point, each wrapped into [0, 2pi], their difference added
 * to the controller's aim pitch -- held within 1.57 either way -- and the player's head turned to it
 * (SOCOM II's tests the SEAL before using it) */
void FirstToThirdPersonTransition(CAppCamera *cam)
{
    float dummy;
    CPnt3D CamOffset;
    CPnt3D cam_new_pos = cam->GetThirdPersonCamPos(&CamOffset, &dummy);
    CPnt3D cam_aim_point(0.0f, 0.0f, 0.0f);
    CPnt3D lz(0.0f, 0.0f, -1000.0f);

    if (cam->m_bc)
        cam->m_bc->m_fwdlook.Apply(&lz, &cam_aim_point, 1);
    cam_aim_point.Add(&CamOffset, &cam_aim_point);
    float pitch0 = atan2f(cam_aim_point.y - CamOffset.y, cam_aim_point.z - CamOffset.z);
    CZSealBody *seal = ftsGetPlayer();

    cam_aim_point = seal->m_aim_point;
    CMatrix WtoM;

    if (cam->m_bc) {
        Vu0MatrixInverseWithUniformScale(&WtoM, cam->m_bc->GetMatrix());
        WtoM.Transform(&cam_aim_point, 1);
    }
    float pitch1 = atan2f(cam_aim_point.y - CamOffset.y, cam_aim_point.z - CamOffset.z);

    while (pitch0 < 0.0f)
        pitch0 += 6.2831855f;
    while (pitch1 < 0.0f)
        pitch1 += 6.2831855f;
    while (pitch0 > 6.2831855f)
        pitch0 -= 6.2831855f;
    while (pitch1 > 6.2831855f)
        pitch1 -= 6.2831855f;
    float dp = pitch0 - pitch1;
    CAppCameraCtrl *pSealCtrl = cam->m_ctrl;
    float aimPitch = pSealCtrl->m_130;

    aimPitch += dp;
    if (aimPitch > 1.57f)
        aimPitch = 1.57f;
    else if (aimPitch < -1.57f)
        aimPitch = -1.57f;
    pSealCtrl->m_130 = aimPitch;
    ftsGetPlayer()->RotateHead(aimPitch);
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
            APTeatherCam(camMatrix, dT);
            break;
        case cam_mode_FP:
            if (FromTetherToFirst())
                ThirdToFirstPersonTransition(this);
            APFirstPersonCam(camMatrix, dT);
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
            APTeatherCam(camMatrix, dT);
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
            m_bc->CheckDIShoot(&m_wPos, &camMatrix);
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

/* what the camera sees from where it is (SOCOM II's; CZHudMissionCams::HudTick calls it on
 * appCamera; the name ours): the ground below its origin, unless a SEAL's -- every region when the
 * player is (func_002C2FD0), else the ground's -- and the ground's material, and with a polygon
 * whether it is indoors and its flag */
void CAppCamera::UpdateCameraRegions()
{
    CPnt3D p(0.0f, 0.0f, 0.0f);

    m_camera->TransformToWorld(p);
    zdb::IntersectStruct *is = zdb::CWorld::m_world->GetAltitude(&p, 0.0f, true);

    if (is && !CZSealBody::GetSeal(is->m_node)) {
        if (func_002C2FD0(ftsGetPlayer()))
            m_camera->SetCameraCanSeeRegions(-1);
        else
            m_camera->SetCameraCanSeeRegions(is->GetRegion());
        m_camera->m_AboveMaterial = is->GetMaterial();
        if (is->m_poly) {
            m_camera->m_IsInside = is->IsInside();
            m_camera->m_AbovePolyFlag = is->GetPolyFlag27();
        }
    }
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

/* the camera aimed past the SEAL it follows: the aim and the tie carried into the SEAL's space, the
 * goal the tie plus the view direction at the tether's length, the chord clipped against the world,
 * then looked along (SOCOM 1's; SOCOM II's controller is the camera's own at 0xC4, the target
 * argument unread as in SOCOM 1's). The controller through a local: read through the member twice,
 * MW keeps &m_130 in a register and stores through it (7 instructions off) */
void CAppCamera::APLookAt(float dT, CEntity *target, CMatrix &cTw)
{
    CAppCameraCtrl *ctrl = m_ctrl;

    if (ctrl && ctrl->m_130 != 0.0f)
        ctrl->m_130 = 0.0f;
    CPnt3D aim = m_cameraAim;

    m_entity->GetMatrix()->Transform(&aim, 1);
    CPnt3D tie = m_cameraTie;

    m_entity->GetMatrix()->Transform(&tie, 1);
    CPnt3D dir = m_rPos;

    dir.Normalize();
    dir.Scale(m_goal_t_len, &dir);
    CPnt3D goal = tie;

    goal.Add(&dir, &goal);
    ClipCameraChord(dT, &aim, &goal, &goal);
    LookAt(&goal, &aim, cTw);
}

/* the death camera put back: no death state, the automatic death camera on, the short tether's
 * saved length long, the rotation and aim deltas nothing (SOCOM 1's; SOCOM II puts back a few more) */
void CAppCamera::ResetDeathCam()
{
    m_cam_death_state = 0;
    m_auto_death_cam = 1;
    save_short_tether_length = 10000.0f;
    data_00416208 = 0.0f;
    data_00416210 = 0.0f;
    data_00416218 = 0.0f;
    m_death_state_timer = 0.0f;
    m_f4 = 0;
    m_f5 = m_f4;
    m_110 = -1;
    m_114 = 0;
    m_104 = 0.0f;
    m_108 = 0.0f;
    m_10c = 0;
    m_118 = 1.0f;
}

/* a point eased: from `from` by `delta` scaled (ours: the frame proves an inline -- the scale's and
 * the sum's results one level below the differences, which are its arguments). The scale by
 * reference (round 23): 0x297410 eases by a global, which the target loads after the difference's
 * call, where a by-value parameter loads it ahead of the call and holds it in $f20; every other
 * caller passes a local and is unchanged */
inline void EaseBy(CPnt3D &p, const CPnt3D &from, const CPnt3D &delta, const float &t)
{
    p = from + delta * t;
}

/* what the camera sees, two levels down (ours: UpdateCameraRegions called directly puts its point
 * above the differences' locals; through this its frame slot is the target's, among the sums') */
inline void UpdateRegions(CAppCamera *cam)
{
    cam->UpdateCameraRegions();
}

/* a value held within [lo, hi] (ours: an inline's own join -- the clamp's arms end on it, then
 * branch to the caller's; written in place they branch to the caller's join directly, 8 off) */
inline float ClampF(float a, float lo, float hi)
{
    if (a < lo)
        a = lo;
    else if (a > hi)
        a = hi;
    return a;
}

/* the death camera (SOCOM II's, CAppCamera's 0xF4-0x11B: ResetDeathCam puts its fields back; the
 * class and the names ours) -- its state, the view's rotation built each tick, the yaw and pitch the
 * player's stick has added up, and the SEAL it watches */
struct CDeathCam {
    /* what the camera does: 0 follows, 1 watches the SEAL it was killed by (SOCOM II's watch of
     * m_seal), 2 runs along the mission's camera path -- CROSS steps it to the state saved in m_1
     * (lbu: every value non-negative) */
    unsigned char m_state;   // 0x0 (CAppCamera's m_f4)
    signed char m_next;   // 0x1 (CAppCamera's m_f5: lb)
    unsigned char _2[2];
    CPnt3D m_rot;   // 0x4
    float m_yaw;   // 0x10
    float m_pitch;   // 0x14
    CZSealBody *m_seal;   // 0x18
    int m_point;   // 0x1c: the camera path's point (-1 none)
    const char *m_pointName;   // 0x20: its name
    float m_zoom;   // 0x24

    /* the app camera's zoom set when it moved off it (the inlined CAppCamera::SetZoom) */
    void SetZoom(float zoom)
    {
        if (fabsf(m_zoom - zoom) > 0.01f) {
            m_zoom = zoom;
            appCamera->SetZoom(zoom);
        }
    }

    /* an angle brought into [-pi, pi], then held to the pitch the view allows: looking down at a
     * SEAL that is not up, anywhere in [-1, 1] else */
    float ClampPitch(float a)
    {
        while (a < -3.1415927f)
            a += 6.2831855f;
        while (a > 3.1415927f)
            a -= 6.2831855f;
        if (m_seal && m_seal->m_netlife != 1)
            a = ClampF(a, -1.0f, -0.2f);
        else
            a = ClampF(a, -1.0f, 1.0f);
        return a;
    }
};

/* the body's blend stack and a blend's state (zbody.h's RTStack and CZBodyAnimBlend: only what the
 * camera reads -- zbody.h's CBody clashes with camera.h's stand-in) */
class CZBodyAnimBlend {
public:
    unsigned char _0[0x2b];
    char m_state;   // 0x2b: 8 dying
};

template <class T>
class RTStack {
public:
    T *Top();
};

/* the SEAL's top blend (its body's blend stack at +0x50: the two offsets added apart) */
inline CZBodyAnimBlend *TopBlend(CZSealBody *seal)
{
    return ((RTStack<CZBodyAnimBlend> *)((unsigned int)&seal->Body() + 0x50))->Top();
}

/* the player and the HUD (fts_player.cpp's CftsPlayer: only its SEAL; hud_main.cpp's CHUD) */
class CftsPlayer {
public:
    CZSealBody *m_seal;   // 0x0
};

extern CftsPlayer ftsPlayer;

class CHUD {
public:
    bool PauseGame();
};

extern CHUD *theHUD;

/* the death camera's state set, kept online while the followed SEAL is down in mode 6 (SetCameraMode's
 * test; ours) */
inline void SetDeathState(CAppCamera *cam, unsigned char state)
{
    bool keep = false;

    if (theNetwork.m_9 && cam->m_bc && func_002C2FD0(cam->m_bc) && cam->m_camera_mode == cam_mode_6)
        keep = true;
    if (!keep) {
        cam->m_cam_death_state = state;
        cam->m_death_state_timer = 0.0f;
    }
}

extern "C" void sceVu0RotMatrixY(sceVu0FMATRIX m0, sceVu0FMATRIX m1, float ry);
/* the death camera's stick turn (below) */
extern "C" void func_00294B40(CDeathCam *dc, CMatrix *sealMat, CMatrix *view, float dT);

/* an angle wrapped into [-pi, pi] (ours, round 23: by value -- the parameter's copy is a statement
 * of its own between the caller's if and the first loop, so the if's arms join on a block the
 * loop's constants are hoisted into, as the target's; written in the caller, or taking the angle by
 * reference or pointer, the if's arms branch straight to the loop's test and 2pi / -pi are
 * rebuilt inside the loop) */
inline float WrapPi(float a)
{
    while (a < -3.1415927f)
        a += 6.2831855f;
    while (a > 3.1415927f)
        a -= 6.2831855f;
    return a;
}

/* a vector laid flat, its y dropped (ours: the frame proves an inline -- each difference handed
 * to it lands among the caller's call-result temporaries, below the named points -- and x read
 * before z, as the target loads them; the three-float constructor's own arguments go z first) */
inline CPnt3D Flat(const CPnt3D &p)
{
    float x = p.x;
    float z = p.z;

    return CPnt3D(x, 0.0f, z);
}

/* a point's distance from the axis (ours: in place the point's x is kept in a register for the
 * push out after it; through the pointer the target reloads it) */
inline float LengthXZ(const CPnt3D *p)
{
    return sqrtf(p->x * p->x + p->z * p->z);
}

/* a matrix's rows, held in a local before they are returned (ours, inline, as vis_vu.cpp's: an
 * argument made of an inline with a body of its own is set up before the call's other arguments --
 * the death camera's turn loads rot_mtx into a1 first and into a0 in the call's delay slot; a
 * one-expression inline is substituted and set up in argument order, the weapons' round 36) */
static inline float (*MatrixRows(CMatrix *m))[4]
{
    float (*r)[4] = m->m_matrix;

    return r;
}


/* the death camera's own look (SOCOM 1's GetDeadCamPoints' place and signature -- GetTetherCamPoints
 * calls it, it calls UpdateDeathCamState first -- SOCOM II's body): the turn taken from the stick
 * (the followed SEAL's, a player's, else the killed SEAL's), by state -- 7 the stick's turn, 6 a
 * slow spin, 3 the point carried about the killed SEAL and turned toward the killer, looking at
 * it -- the angles wrapped, the pitch held, the point turned by both and at state 3 raised and
 * pushed out. Returns whether the camera looks at the killer.
 * BYTE-EXACT since round 24 (written in round 23, 2 off; build/scratch/core/v_dc24.py): the
 * rotation's source through MatrixRows (above) -- the target moves rot_mtx into a1 first and into a0
 * in the call's delay slot (SOCOM 1's GetDeadCamPoints too, both its calls); the direct call, casts
 * on either or both arguments, a void * or const prototype, the CMatrix conversion operator and
 * one-expression inlines all set a0 first (round 23: build/scratch/core/t_rot*.cpp). Settled on the
 * way: the angle reuses turn_x (the target keeps it in turn_x's $f22), the turn's rate a local
 * ahead of the right vector, which is built field by field (x read first) */
unsigned char CAppCamera::GetDeadCamPoints(CPnt3D *CamPoint, CPnt3D *CamAimPoint, CMatrix *rot_mtx, float dT)
{
    unsigned char dead_cam_aim_point = 0;

    UpdateDeathCamState(dT);
    CEntityCtrl *control = 0;
    float turn_x = 0.0f;
    float turn_y = 0.0f;

    if (m_ctrl && m_ctrl->IsPlayer())
        control = (CEntityCtrl *)m_ctrl;
    else if (m_killed)
        control = m_killed->m_control;
    if (control) {
        turn_x = control->m_throttle[2];
        turn_y = control->m_throttle[3];
    }
    switch (m_cam_death_state) {
    case 7:
        data_00416208 += 2.0f * turn_x * dT;
        data_00416210 += 2.0f * turn_y * dT;
        break;
    case 6:
        data_00416208 += 0.8725f * dT;
        break;
    case 3: {
        float speed_multiplier = 1.5f;
        CPnt3D cam_pos = *CamPoint;

        sceVu0RotMatrixY(rot_mtx->m_matrix, MatrixRows(rot_mtx), data_00416208);
        TransformVectors(rot_mtx, &cam_pos, &cam_pos, 1);
        rot_mtx->SetIdentity();
        CPnt3D ent_pos = m_killed->m_posW;
        CPnt3D shooter_pos = m_killer->m_posW;

        m_killed->GetMatrix()->Transform(&cam_pos, 1);
        CPnt3D cam_dir = Flat(cam_pos - ent_pos);
        CPnt3D shooter_dir = Flat(ent_pos - shooter_pos);

        cam_dir.Normalize();
        shooter_dir.Normalize();
        dead_cam_aim_point = 1;
        float rate = 0.8725f * speed_multiplier;
        CPnt3D cam_right_vec;

        cam_right_vec.x = shooter_dir.z;
        cam_right_vec.y = 0.0f;
        cam_right_vec.z = -shooter_dir.x;
        turn_x = -(cam_dir.x * cam_right_vec.x + cam_dir.z * cam_right_vec.z);
        *CamAimPoint = shooter_pos;
        data_00416208 += dT * (rate * turn_x);
        break;
    }
    }
    data_00416208 = WrapPi(data_00416208);
    float aim = WrapPi(data_00416210);

    if (m_bc && m_bc->m_netlife != 1)
        aim = ClampF(aim, -1.0f, -0.2f);
    else
        aim = ClampF(aim, -1.0f, 1.0f);
    data_00416210 = aim;
    CPnt3D euler(data_00416210, data_00416208, 0.0f);

    rot_mtx->SetRotate(&euler);
    TransformVectors(rot_mtx, CamPoint, CamPoint, 1);
    if (m_cam_death_state == 3) {
        CamPoint->y += (20.0f - CamPoint->y) * data_00416218;
        if (LengthXZ(CamPoint) < 56.0f) {
            CamPoint->x += 1.05f * CamPoint->x;
            CamPoint->z += 1.05f * CamPoint->z;
        }
    }
    return dead_cam_aim_point;
}

/* the tether's points for a SEAL down (SOCOM II's): its death begun -- a dying blend or state 8, a
 * player's -- the death camera's look taken (unpaused) or the view turned about the SEAL as it
 * falls, the death camera's stick turn added online; the point and the look carried into the SEAL's
 * space, the ease begun or primed at state 7 online, and the aim eased toward the look while the
 * death camera watches. Returns whether it does.
 * Byte-exact since round 23 by four shapes (build/scratch/core/v_ta5-9.py): the wrap through
 * WrapPi (above), look declared ahead of lz (the inline's copy swaps their frame slots otherwise),
 * EaseBy's scale by reference, and the rotation's second argument through a no-op integer
 * conversion -- an argument needing one is evaluated first (the SEALs' round 34), so the target's
 * a1 is set up and a0 copied from it; a pointer local, the CMatrix conversion operator, inlines
 * taking the matrix and pointer casts all leave a0 first. The cast is a spelling, not a claim
 * about the original */
unsigned char CAppCamera::GetTetherCamPoints(CPnt3D *pos, CPnt3D *offset, CPnt3D *aim, float dT)
{
    unsigned char watching = 0;
    CMatrix rot;

    if (!m_cam_death_state && m_bc &&
        (TopBlend(m_bc)->m_state == 8 || m_bc->m_state == 8) && m_bc->IsPlayer())
        SetDeathState(this, 1);
    if (m_cam_death_state) {
        if (!theHUD->PauseGame())
            watching = GetDeadCamPoints(pos, aim, &rot, dT);
    } else {
        float turn = 0.0f;

        if (m_bc && m_bc->m_state == 8)
            turn = -2.5132742f * dT;
        turn = WrapPi(turn);
        sceVu0RotMatrixY(rot.m_matrix, (sceVu0FVECTOR *)(unsigned int)rot.m_matrix, turn);
        TransformVectors(&rot, pos, pos, 1);
    }
    if (theNetwork.m_9 && func_002C2FD0(ftsPlayer.m_seal) && ((CDeathCam *)&m_f4)->m_state == 1) {
        func_00294B40((CDeathCam *)&m_f4, m_entity->GetMatrix(), &rot, dT);
        TransformVectors(&rot, pos, pos, 1);
    }
    m_entity->GetMatrix()->Transform(pos, 1);
    CPnt3D look;
    CPnt3D lz(0.0f, 0.0f, -1000.0f);

    if (m_entity == m_bc)
        m_bc->m_fwdlook.Apply(&lz, &look, 1);
    else
        look = lz;
    TransformVectors(&rot, &look, &look, 1);
    look.Add(offset, &look);
    m_entity->GetMatrix()->Transform(&look, 1);
    if (theNetwork.m_9 && m_cam_death_state == 7) {
        if (m_44) {
            m_44 = 0;
            m_easeAim = look;
            m_easePos = *pos;
        } else {
            m_45 = 1;
            m_46 = 0;
            m_easeT = 0.0f;
        }
    }
    if (watching) {
        data_00416218 += dT;
        if (data_00416218 > 1.0f)
            data_00416218 = 1.0f;
        EaseBy(*aim, look, *aim - look, data_00416218);
    } else
        *aim = look;
    return watching;
}

/* a - b, one level down (ours: written in place the difference's local takes the frame slot above
 * the ease's temporaries; one wrapper deeper it is the target's, among the other differences') */
inline CPnt3D DiffOf(const CPnt3D &a, const CPnt3D &b)
{
    return a - b;
}

/* the tether's camera (SOCOM 1's APTeatherCam, SOCOM II's body): the third-person point; looking at the
 * watched SEAL, the chord from just above it clipped -- else the aim put the tether's length out
 * along the view, lowered for a player's SEAL down on dry ground offline, both eased as first
 * person's and the chord from the aim clipped -- then looked along and what the camera sees */
void CAppCamera::APTeatherCam(CMatrix &cTw, float dT)
{
    CPnt3D CamOffset;
    float tetherLen;
    CPnt3D cam_new_pos = GetThirdPersonCamPos(&CamOffset, &tetherLen);
    CPnt3D cam_aim_point;

    /* a byte the call returns, narrowed again (andi) before the test */
    unsigned char watching = GetTetherCamPoints(&cam_new_pos, &CamOffset, &cam_aim_point, dT);

    if (watching && m_bc) {
        CPnt3D from = m_bc->m_posW;

        from.y += 10.0f;
        ClipCameraChord(dT, &from, &cam_new_pos, &cam_new_pos);
    } else {
        CPnt3D dir = DiffOf(cam_aim_point, cam_new_pos);

        dir.Normalize();
        EaseBy(cam_aim_point, cam_new_pos, dir, tetherLen);
        if (m_bc && !theNetwork.m_9 && m_bc->IsPlayer() && !m_bc->m_isAlive &&
            !m_bc->m_inwater) {
            cam_aim_point.y -= 5.0f;
            cam_aim_point.z += 5.0f;
        }
        if (m_45) {
            if (m_46) {
                m_easeT += dT;
                if (!(m_easeT < 1.0f))
                    m_45 = 0;
            } else
                m_easeT = 6.0f * dT;
            if (m_easeT > 1.0f)
                m_easeT = 1.0f;
            float t = m_easeT;

            EaseBy(cam_aim_point, m_easeAim, cam_aim_point - m_easeAim, t);
            m_easeAim = cam_aim_point;
            EaseBy(cam_new_pos, m_easePos, cam_new_pos - m_easePos, t);
            m_easePos = cam_new_pos;
        } else {
            m_easeAim = cam_aim_point;
            m_easePos = cam_new_pos;
        }
        ClipCameraChord(dT, &cam_aim_point, &cam_new_pos, &cam_new_pos);
    }
    LookAt(&cam_new_pos, &cam_aim_point, cTw);
    UpdateRegions(this);
}

/* the controller's aim pitched by a rate (OVL2 0x594600, the SEALs' controller: not named yet) and
 * the pitch from the current layout's sticks (fts_controller.cpp's 0x2C6280) */
extern "C" void func_00594600(CAppCameraCtrl *ctrl, float dT, float rate);
extern "C" float func_002C6280(const CPad *pad);
/* how long mode 1's camera has run with the short tether's rate at 1 (SOCOM II's static) */
extern "C" float data_00416030;

/* mode 1's camera (SOCOM II's, the tether's twin over mode 1's camera position): with the death
 * camera's state 7 the followed controller's pitch turned by the pad's stick at twice its rate,
 * then the tether's camera with the aim lowered by 8 for a player's SEAL down on dry ground */
void func_00295F20(CAppCamera *cam, CMatrix &cTw, float dT)
{
    if (cam->m_cam_death_state == 7) {
        CPad *pad = cam->m_padid < CInput::NUM_PADS ? CInput::m_pads[cam->m_padid] : 0;

        if (!pad)
            pad = CInput::m_pads[0];
        if (pad) {
            CAppCameraCtrl *ctrl = cam->m_ctrl;

            if (ctrl) {
                /* the rate doubled in place: computed into the call's argument it is one register,
                 * the target's product is f0 copied to f13 */
                float rate = func_002C6280(pad);

                rate *= 2.0f;
                func_00594600(ctrl, dT, rate);
            }
        }
    }
    CPnt3D CamOffset;
    float tetherLen;
    CPnt3D cam_new_pos = func_0029A660(cam, &CamOffset, &tetherLen);
    CPnt3D cam_aim_point;
    /* a byte the call returns, narrowed again (andi) before the test */
    unsigned char watching = cam->GetTetherCamPoints(&cam_new_pos, &CamOffset, &cam_aim_point, dT);

    if (watching && cam->m_bc) {
        CPnt3D from = cam->m_bc->m_posW;

        from.y += 10.0f;
        cam->ClipCameraChord(dT, &from, &cam_new_pos, &cam_new_pos);
    } else {
        CPnt3D dir = DiffOf(cam_aim_point, cam_new_pos);

        dir.Normalize();
        EaseBy(cam_aim_point, cam_new_pos, dir, tetherLen);
        if (cam->m_bc && !theNetwork.m_9 && cam->m_bc->IsPlayer() && !cam->m_bc->m_isAlive &&
            !cam->m_bc->m_inwater)
            cam_aim_point.y -= 8.0f;
        if (cam->m_45) {
            if (cam->m_46) {
                cam->m_easeT += dT;
                if (!(cam->m_easeT < 1.0f))
                    cam->m_45 = 0;
            } else
                cam->m_easeT = 6.0f * dT;
            if (cam->m_easeT > 1.0f)
                cam->m_easeT = 1.0f;
            float t = cam->m_easeT;

            EaseBy(cam_aim_point, cam->m_easeAim, cam_aim_point - cam->m_easeAim, t);
            cam->m_easeAim = cam_aim_point;
            EaseBy(cam_new_pos, cam->m_easePos, cam_new_pos - cam->m_easePos, t);
            cam->m_easePos = cam_new_pos;
        } else {
            cam->m_easeAim = cam_aim_point;
            cam->m_easePos = cam_new_pos;
        }
        cam->ClipCameraChord(dT, &cam_aim_point, &cam_new_pos, &cam_new_pos);
    }
    cam->LookAt(&cam_new_pos, &cam_aim_point, cTw);
    UpdateRegions(cam);
    if (short_tether_rate == 1.0f) {
        if (data_00416030 > 0.1f)
            short_tether_rate = 0.03f;
        data_00416030 += dT;
    }
}

/* first person's camera (SOCOM 1's APFirstPersonCam and its locals): the first-person point and
 * the forward look -- the watched SEAL's own when it is the one followed -- carried into the SEAL's
 * space, eased from where they were while the ease runs (over a second, else six times the frame),
 * looked along, then what the camera sees (UpdateCameraRegions, inlined). The frame's temporaries
 * go by inline depth, then by callee (README: the engine core's round 22) */
void CAppCamera::APFirstPersonCam(CMatrix &cTw, float dT)
{
    CPnt3D head_point = GetFirstPersonCamPos(this);
    CPnt3D pos = head_point;

    m_entity->GetMatrix()->Transform(&pos, 1);
    CPnt3D cam_aim_point(0.0f, 0.0f, 0.0f);
    CPnt3D lz(0.0f, 0.0f, -1.0f);

    if (m_bc && m_bc == m_entity)
        m_bc->m_fwdlook.Apply(&lz, &cam_aim_point, 1);
    else
        cam_aim_point = lz;
    cam_aim_point.Add(&head_point, &cam_aim_point);
    m_entity->GetMatrix()->Transform(&cam_aim_point, 1);
    if (m_45) {
        if (m_46) {
            m_easeT += dT;
            if (!(m_easeT < 1.0f))
                m_45 = 0;
        } else
            m_easeT = 6.0f * dT;
        if (m_easeT > 1.0f)
            m_easeT = 1.0f;
        float t = m_easeT;

        EaseBy(cam_aim_point, m_easeAim, cam_aim_point - m_easeAim, t);
        m_easeAim = cam_aim_point;
        EaseBy(pos, m_easePos, pos - m_easePos, t);
        m_easePos = pos;
    } else {
        m_easeAim = cam_aim_point;
        m_easePos = pos;
    }
    LookAt(&pos, &cam_aim_point, cTw);
    UpdateRegions(this);
}

/* a SEAL followed: the entity's own (defined after it, so a call -- an 8-byte jump) */
void CAppCamera::Follow(CZSealBody *seal, const char *part)
{
    Follow((CEntity *)seal, part);
}

/* an entity followed (SOCOM 1's AttachTo's first half, SOCOM II's): online, a SEAL down puts its
 * controller and the camera watching it -- else this one -- in mode 6, one up takes this one out
 * of it, and with the player down the death camera is set to watch it; the entity refused at
 * state 7 online unless a SEAL other than the one followed. Then the SEAL (the entity, else what
 * its controller drives) and its controller taken, the pad from a player's, the effects node and
 * the part to hang from found, and the death camera's turns put back */
void CAppCamera::Follow(CEntity *prey, const char *part)
{
    bool ok = true;

    if (theNetwork.m_9) {
        CZSealBody *seal = dynamic_cast<CZSealBody *>(prey);

        if (seal) {
            if (func_002C2FD0(seal)) {
                if (seal->m_control)
                    seal->m_control->m_220 = 6;
                if (seal->m_camera)
                    seal->m_camera->SetCameraMode(cam_mode_6);
                else
                    SetCameraMode(cam_mode_6);
            } else if (m_camera_mode == cam_mode_6)
                m_camera_mode = cam_mode_tether;
            if (func_002C2FD0(ftsGetPlayer())) {
                if (DeathCam()->m_state == 0)
                    DeathCam()->m_next = 0;
                else if (DeathCam()->m_state == 1)
                    DeathCam()->m_next = 1;
                else if (DeathCam()->m_state == 2)
                    if (fabsf(m_118 - 1.0f) > 0.01f) {
                        m_118 = 1.0f;
                        appCamera->SetZoom(1.0f);
                    }
                if (seal != ftsGetPlayer()) {
                    DeathCam()->m_state = 1;
                    DeathCam()->m_seal = seal;
                } else
                    DeathCam()->m_state = 0;
                DeathCam()->m_yaw = 0.0f;
                DeathCam()->m_pitch = 0.0f;
            }
        }
    }
    if (theNetwork.m_9 && m_cam_death_state == 7) {
        if (!dynamic_cast<CZSealBody *>(prey) || prey == m_entity)
            ok = false;
    }
    if (ok) {
        m_45 = 0;
        if (prey) {
            m_bc = dynamic_cast<CZSealBody *>(prey);
            /* the controller tested twice, as the target (two beqz on one register: a pointer
             * conversion's or an accessor's own test in the original, likely; `c ?` or an inline
             * returning 0 add a store of zero, one test is 2 short) */
            if (!m_bc) {
                CEntityCtrl *c = prey->m_control;

                if (c) {
                    if (c)
                        m_bc = (CZSealBody *)c->m_entity;
                }
            }
            if (m_bc)
                m_ctrl = (CAppCameraCtrl *)m_bc->m_control;
            else
                m_ctrl = 0;
        }
        if (m_ctrl && m_ctrl->IsPlayer())
            m_padid = m_ctrl->m_padid;
        if (m_entity != prey)
            m_44 = 1;
        m_entity = (CZSealBody *)prey;
        if (m_bc) {
            m_camera_effects = m_bc->GetNode()->FindChild("camera_effects", true);
            if (part)
                m_skel_root = m_bc->FindPart(part);
            else
                m_skel_root = m_bc->m_root;
        } else {
            m_camera_effects = 0;
            m_skel_root = 0;
        }
        data_00416208 = 0.0f;
        data_00416210 = 0.0f;
    }
}

/* the death camera turned by the player's stick: the yaw and pitch added up at twice the stick's
 * throttle a second, the rotation rebuilt from them -- the yaw within a turn, the pitch held --
 * set into the view's matrix and read back, the yaw then carried along with the watched SEAL's
 * (SOCOM II's; GetTetherCamPoints calls it with the SEAL's matrix) */
extern "C" void func_00294B40(CDeathCam *dc, CMatrix *sealMat, CMatrix *view, float dT);

/* the mission's camera path (fts_mission.cpp's CCamPath and CMission: only the path, at 0x1910) */
class CCamPath {
public:
    /* a point `step` on from `index`, its place, direction and name given (-1: none) */
    int GetPoint(int index, int step, CPnt3D *pos, CPnt3D *dir, const char **name);
};

class CMission {
public:
    unsigned char _0[0x1910];
    CCamPath m_campath;   // 0x1910
};

extern CMission theMission;

/* the death camera's pad, online with the player down and the player's controller not
 * zooming: TRIANGLE picks the SEAL nearest the view line, CROSS steps the camera to its next
 * state -- following, back to the player's own view, the camera path -- and UP / DOWN run along
 * the mission's camera path (SOCOM II's; FTSTick calls it with the followed SEAL's pad) */
extern "C" void func_00295260(CDeathCam *dc, CAppCamera *cam, CPad *pad)
{
    if (theNetwork.m_9 && func_002C2FD0(ftsGetPlayer())) {
        if (dc->m_state != 2)
            dc->m_pointName = 0;
        if (ftsGetPlayer()->m_control->m_221)
            return;
        if (pad->m_state[PAD_TRIANGLE] == KEY_RISING)
            func_00294F40(dc, cam);
        if (pad->m_state[PAD_CROSS] == KEY_RISING) {
            switch (dc->m_state) {
            case 0:
                dc->m_state = dc->m_next;
                if (dc->m_state == 2)
                    dc->m_point = theMission.m_campath.GetPoint(dc->m_point, 0, &cam->m_wPos, &dc->m_rot,
                                                                &dc->m_pointName);
                else if (dc->m_state == 1) {
                    cam->m_camera_mode = cam_mode_tether;
                    cam->Follow((CEntity *)dc->m_seal, 0);
                }
                break;
            case 1:
                cam->m_entity = ftsGetPlayer();
                cam->m_bc = ftsGetPlayer();
                cam->m_camera_mode = cam_mode_6;
                cam->m_44 = 0;
                dc->m_next = 1;
                dc->m_state = 0;
                dc->m_yaw = 0.0f;
                dc->m_pitch = 0.0f;
                break;
            case 2:
                dc->m_next = 2;
                dc->m_state = 0;
                dc->SetZoom(1.0f);
                break;
            }
        } else if (pad->m_state[PAD_UP] == KEY_RISING) {
            dc->m_point = theMission.m_campath.GetPoint(dc->m_point, 1, &cam->m_wPos, &dc->m_rot,
                                                        &dc->m_pointName);
            if (dc->m_point >= 0) {
                dc->SetZoom(1.0f);
                cam->m_camera_mode = cam_mode_6;
                dc->m_state = 2;
                cam->m_entity = ftsGetPlayer();
                cam->m_bc = ftsGetPlayer();
                cam->m_44 = 0;
            }
        } else if (pad->m_state[PAD_DOWN] == KEY_RISING) {
            dc->m_point = theMission.m_campath.GetPoint(dc->m_point, -1, &cam->m_wPos, &dc->m_rot,
                                                        &dc->m_pointName);
            if (dc->m_point >= 0) {
                dc->SetZoom(1.0f);
                cam->m_camera_mode = cam_mode_6;
                dc->m_state = 2;
                cam->m_entity = ftsGetPlayer();
                cam->m_bc = ftsGetPlayer();
                cam->m_44 = 0;
            }
        }
    }
}

/* an entity as a SEAL, if it is one (ours: inline -- the entity read into the result, the take
 * flag cleared first) */
inline CZSealBody *ToSeal(CEntity *e)
{
    return e && e->m_type == CEntity::ENT_TYPE_SEAL ? (CZSealBody *)e : 0;
}
/* the death camera's TRIANGLE (SOCOM II's): the view line put out from the camera to its far
 * plane, and of the SEALs up the one nearest it taken -- nearer the line by a tenth, else as near
 * and nearer the camera -- and followed (the death camera unused) */
extern "C" void func_00294F40(CDeathCam *dc, CAppCamera *cam)
{
    CZSealBody *best = 0;
    float bestDist = 0.0f;
    float bestLen = 0.0f;
    float dist = 0.0f;
    CPnt3D line[2];
    CPnt3D v;

    line[0] = CPnt3D::zero;
    line[1] = CPnt3D::zero;
    line[1].z = -cam->m_camera->m_far_plane;
    cam->m_camera->m_matrix.Transform(line, 2);
    std::list<CEntity *>::const_iterator it;

    for (it = CEntity::GetEntityList().begin(); it != CEntity::GetEntityList().end(); ++it) {
        bool take = false;
        CZSealBody *seal = ToSeal(*it);

        if (seal && !func_002C2FD0(seal) && seal->m_type == CEntity::ENT_TYPE_SEAL) {
            seal->m_posW.DistToVector(&line[0], &line[1], &dist);
            if (!best)
                take = true;
            else {
                float d = (dist - bestDist) / bestDist;

                if (d < -0.1f)
                    take = true;
                else if (d < 0.1f) {
                    seal->m_posW.Sub(&line[0], &v);
                    float l = (v.Length() - bestLen) / bestLen;

                    if (l < 0.1f && (d < 0.0f || l < 0.0f))
                        take = true;
                }
            }
            if (take) {
                bestDist = dist;
                best = seal;
                seal->m_posW.Sub(&line[0], &v);
                bestLen = v.Length();
            }
        }
    }
    if (best)
        cam->Follow((CEntity *)best, 0);
}

extern "C" void func_00294B40(CDeathCam *dc, CMatrix *sealMat, CMatrix *view, float dT)
{
    float dx = 0.0f;
    float dy = 0.0f;

    dc->m_rot = CPnt3D::zero;
    CEntityCtrl *ctrl = ftsGetPlayer()->m_control;

    if (ctrl) {
        dx = ctrl->m_throttle[2];
        dy = ctrl->m_throttle[3];
    }
    dc->m_yaw += 2.0f * dx * dT;
    dc->m_rot.y += dc->m_yaw;
    dc->m_pitch += 2.0f * dy * dT;
    dc->m_rot.x += dc->m_pitch;
    while (dc->m_rot.y < 0.0f)
        dc->m_rot.y += 6.2831855f;
    while (dc->m_rot.y > 6.2831855f)
        dc->m_rot.y -= 6.2831855f;
    dc->m_pitch = dc->ClampPitch(dc->m_pitch);
    dc->m_rot.x = dc->ClampPitch(dc->m_rot.x);
    view->SetRotate(&dc->m_rot);
    view->ToEuler(&dc->m_rot);
    CPnt3D seal;

    sealMat->ToEuler(&seal);
    dc->m_rot.y += seal.y;
}
