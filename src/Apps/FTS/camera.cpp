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
extern "C" void func_002956E0(CAppCamera *cam, CZSealBody *prey, int arg);

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
extern "C" void func_00295260(void *deathcam, CAppCamera *cam, CPad *pad);

/* what the tick reaches for (SOCOM 1's names in the comments) */
extern "C" {
float expf(float x);
bool func_00299C00();                            /* zdb::CCamera::GetDynamicsControl (m_dynamics_controlled) */
void func_0029A180(CAppCamera *cam);             /* (SOCOM II's: into mode 1 from the tether) */
void func_00295F20(CAppCamera *cam, CMatrix &cTw, float dT);   /* (mode 1's camera) */
void func_00296F10(CAppCamera *cam, CMatrix &cTw, float dT);   /* (the tether's: APTeatherCam's place) */
void func_00295B00(CAppCamera *cam, CMatrix &cTw, float dT);   /* (first person's: APFirstPersonCam's place) */
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
 * OPEN (36 of 113 instructions, registers only): the target keeps appCamera in v1 and the level in
 * a0 (the command's register, dead after the load); MW gives the level v1 and appCamera a1 -- the
 * same with a local camera, an early return, the level read first or as an int, Shake declared
 * inline, and with the switch written out here; round 15: the level a caller local defined inside
 * the null test (plain, const, int), declared at the top, the command cast into a local -- the same
 * 36 (build/scratch/core/v_shake.py): by the numbering the target's camera is numbered above the
 * inlined Shake's level, which no caller-side spelling reaches. Round 17, the replay (--want
 * @3377=v1,level=a0): the level's node must sit before the camera's (31.5 or 32.5), or the camera's
 * after it (35.5 on); a caller's named level is copy-propagated away (its node dead), and the
 * camera a named local is coalesced with the call's a0 -- the level an enum field, a local copy in
 * Shake (unsigned char, int), a cast in either, a dead first definition: the same 36
 * (build/scratch/core/v_shake*.py, the core lane's round-17 scratch). Round 18: the test and the call
 * in a free inline over the level, the command or a const reference to the level (each 0x34: Shake no
 * longer inlined -- one level more), the level a reference local (53), the command a reference or cast
 * through a leaf template (36). Round 19: the camera through a leaf template (cast or identity) or a
 * cast copy, the command a const cast copy, the level a cast local -- 36; the level read ahead of the
 * test (a named, const or inline-result local) moves the load above the branch (52):
 * build/scratch/core/v_shake19*.py */
void CAppCamera::CmdBeginShake(_zanim_cmd_hdr *hdr)
{
    if (appCamera)
        appCamera->Shake(((_camera_shake_cmd *)hdr)->level);
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
    m_150 = 0;
    m_154 = 0;
    m_goal_t_len = m_camGoalPos.Length();
    func_002956E0(this, ftsGetPlayer(), 0);
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
        func_00295260(&m_f4, this, pad);
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
            cam->m_bc->m_body.PartInSpace(cam->m_skel_root, 0, &CamPoint, 0);
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
            m_bc->m_body.PartInSpace(part, 0, &CamPoint, 0);
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
            cam->m_bc->m_body.PartInSpace(cam->m_skel_root, 0, &CamPoint, 0);
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
