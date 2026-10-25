/* CAppCamera -- the player's camera: first and third person, the tether, the death camera, the
 * wiggle -- and CameraZoom, the scope's zoom (SOCOM 1's Apps/FTS/camera.cpp and thezoom.cpp).
 * Layouts are SOCOM 1's DWARF moved to where SOCOM II's code reads them: CAppCamera grew 0x80 bytes
 * of shake presets ahead of its camera pointer (CAMERA_SHAKE's begin reads three sets of eight
 * floats from 0x50), and a few words further on. The header's name is ours. */
#ifndef CAMERA_H
#define CAMERA_H

#include "../../gamez/zcamera/zcamera.h"
#include "../../gamez/zanim/zanim.h"

namespace zdb {
/* the collision piece's kind (two bits of its byte at 0xA: SOCOM 1's at 0x1A; the name ours) */
struct CDIKind {
    unsigned char _0[0xa];
    unsigned char _b0 : 2;
    unsigned char m_kind : 2;   // 0xa
    unsigned char _b4 : 4;

    /* the kind as a byte (the hit test compares it masked to one) */
    unsigned char Kind() const { return m_kind; }
};

/* a hit along a ray (SOCOM 1's DWARF) */
struct IntersectStruct {
    CPnt3D m_pos;   // 0x0
    CDI *m_poly;   // 0xc
    CPnt3D m_norm;   // 0x10
    CNode *m_node;   // 0x1c
};

/* a ray's intersection set (SOCOM 1's DWARF; SOCOM II's hits are 4 bytes further on) */
class DiIntersect {
public:
    unsigned int m_Type : 16;   // 0x0
    unsigned int m_IntersectCharacters : 1;
    unsigned int m_AltitudeCharacters : 1;
    unsigned int m_ProximityCharacters : 1;
    unsigned int m_Unused : 13;
    CPnt3D m_Tail;   // 0x4
    CPnt3D m_Tip;   // 0x10
    CPnt3D m_MTail;   // 0x1c
    CPnt3D m_MTip;   // 0x28
    int m_Cnt;   // 0x34
    int m_BufCnt;   // 0x38
    CNode *m_Node;   // 0x3c
    CNode *m_TreeDoneNode;   // 0x40
    unsigned int _44;
    IntersectStruct *m_Intersects;   // 0x48 (SOCOM 1: 0x44)
};
}

zdb::IntersectStruct *GetNearestIntersectCamPoint(zdb::DiIntersect *diSet, CPnt3D *p0);

class CHUD;
class CEntity;
class CZBodyPart;

/* a seal's controller: its mode at 0x221 (5 hands the zoom to the camera) */
class CEntityCtrl {
public:
    unsigned char _0[0x221];
    unsigned char m_221;   // 0x221
};

/* which way a SEAL peeks (SOCOM 1's DWARF; a signed byte under -enum min) */
enum SEAL_PEEK {
    PEEK_LEFT = -1,
    PEEK_UP = 0,
    PEEK_RIGHT = 1,
    PEEK_NONE = 2
};

/* a SEAL: its matrix (the second virtual), its controller (CEntity's at 0xC0), its body's state
 * and wanted peek (SOCOM 1's CBody::m_state at 0x11D, SOCOM II's a halfword at 0x174; its
 * m_desiredPeek at 0x305, SOCOM II's 0x375) and the zoomed flag (SOCOM 1's at 0xF5C) */
class CZSealBody {
public:
    virtual ~CZSealBody();
    virtual CMatrix *GetMatrix();

    unsigned char _4[0xc0 - 4];
    CEntityCtrl *m_control;   // 0xc0
    unsigned char _c4[0x174 - 0xc4];
    short m_state;   // 0x174: 8 dead, 2 prone (SOCOM 1's SEAL_STATE)
    unsigned char _176[0x375 - 0x176];
    SEAL_PEEK m_desiredPeek;   // 0x375
    unsigned char _376[0x5e0 - 0x376];
    unsigned char m_kit[0x1060 - 0x5e0];   // 0x5e0: its CZKit (SOCOM 1: 0x520)
    unsigned char _b0 : 5;   // 0x1060
    unsigned char m_zooming : 1;
    unsigned char _b6 : 2;
};

/* a camera position the app camera keeps per view: where it sits and where it looks, in the
 * SEAL's space (the name ours) */
struct CCameraPreset {
    CPnt3D m_tie;   // 0x0
    CPnt3D m_aim;   // 0xc
};

/* a shake: per screen axis a base amplitude, a random part added to it (scaled by 0..0.99), the
 * amplitude's decay per frame and the ripple's rate in degrees a second -- x's four, then y's
 * (SOCOM II's; BeginShake's arguments in order, the names ours) */
struct CShakeParams {
    float m_xAmp;   // 0x0
    float m_xAmpRand;   // 0x4
    float m_xDecay;   // 0x8
    float m_xRate;   // 0xc
    float m_yAmp;   // 0x10
    float m_yAmpRand;   // 0x14
    float m_yDecay;   // 0x18
    float m_yRate;   // 0x1c
};

/* the camera's modes (SOCOM 1's PLAYER_CAM_STATE: 1 the tether, 2 first person, 3 looking at the
 * tracked entity, 4 the network's). SOCOM II renumbered them: its third person tether is 2 (the
 * zoom's Reset sets it, FirstToThirdPersonTransition enters it) and first person 3
 * (CmdTick3rdPersonTest, CCoreState::Tick hiding the camera's own entity, ThirdToFirstPerson-
 * Transition entering it from 2); 1 is a new view entered from the tether, 4 looks at the tracked
 * entity, 5 ticks no camera, 6 is new -- the new names ours) */
enum PLAYER_CAM_STATE {
    cam_mode_unk = 0,
    cam_mode_1 = 1,
    cam_mode_tether = 2,
    cam_mode_FP = 3,
    cam_mode_apLook = 4,
    cam_mode_5 = 5,
    cam_mode_6 = 6
};

/* the app camera's views (SOCOM 1's DWARF; a byte under -enum min) */
enum CAMVIEW {
    cam_view_third = 0,
    cam_view_side = 1,
    cam_view_first = 2,
    cam_view_peek_left = 3,
    cam_view_peek_right = 4,
    cam_view_last = 5
};

class CAppCamera {
public:
    ~CAppCamera();
    virtual void Tick(float dT);
    virtual bool AttachTo(CZSealBody *prey);

    void FTSTick();
    void APLookAt(float dT, CEntity *target, CMatrix &cTw);

    /* the mode changes Tick runs a transition for (inline: their bools are materialised before the
     * branch, SOCOM 1's too; the names ours) */
    bool FromTetherToMode1() const
    {
        return m_camera_mode == cam_mode_1 && m_camera_last_mode == cam_mode_tether;
    }
    bool IntoTether() const
    {
        return m_camera_mode == cam_mode_tether && m_camera_last_mode != cam_mode_tether;
    }
    bool FromTetherToFirst() const
    {
        return m_camera_mode == cam_mode_FP && m_camera_last_mode == cam_mode_tether;
    }
    void SetZoom(float zoom);
    void SetCameraMode(unsigned char mode);
    void ResetView();
    void ResetDeathCam();
    void WiggleEcho(bool flag);
    void TickCameraWiggle(float dT, zdb::CCamera *cam);
    void Shake(unsigned char level);
    void BeginShake(float xAmp, float xAmpRand, float xDecay, float xRate, float yAmp,
                    float yAmpRand, float yDecay, float yRate);
    void StopShake();
    static bool TickTask(float dT, void *data);
    void LookAt(const CPnt3D *from, const CPnt3D *to, CMatrix &cTw);
    static bool CmdTick3rdPersonTest(_zanim_cmd_hdr *hdr, float *dT);
    static _zanim_cmd_hdr *CmdParse3rdPersonTest(_zrdr *rdr);
    static _zanim_cmd_hdr *CmdParseShake(_zrdr *rdr);
    static void CmdBeginShake(_zanim_cmd_hdr *hdr);
    static void RegisterAnimCommands();

    /* the view set, the old one returned (the zoom keeps it; the name ours) */
    CAMVIEW SetCtrlView(CAMVIEW view)
    {
        CAMVIEW old = m_ctrl_view;

        m_ctrl_view = view;
        return old;
    }

    CPnt3D m_cameraAim;   // 0x4 (SOCOM 1's)
    CPnt3D m_cameraTie;   // 0x10
    CPnt3D m_camGoalPos;   // 0x1c
    float m_goal_t_len;   // 0x28
    unsigned char _2c[0x50 - 0x2c];
    CShakeParams m_shake[3];   // 0x50: SMALL, MEDIUM, LARGE (SOCOM II's)
    unsigned char _b0[0xb4 - 0xb0];
    zdb::CCamera *m_camera;   // 0xb4 (SOCOM 1: 0x34)
    zdb::CNode *m_camera_effects;   // 0xb8 (SOCOM 1: 0x38)
    CZSealBody *m_bc;   // 0xbc: SOCOM II's, a SEAL the mode setter asks after (the name ours)
    CZSealBody *m_entity;   // 0xc0 (SOCOM 1: 0x3c)
    unsigned int _c4;
    CZBodyPart *m_skel_root;   // 0xc8 (SOCOM 1: 0x40)
    CPnt3D m_rPos;   // 0xcc (SOCOM 1: 0x44)
    CPnt3D m_wPos;   // 0xd8 (SOCOM 1: 0x50)
    CAMVIEW m_ctrl_view;   // 0xe4 (SOCOM 1: 0x5c)
    unsigned char _e5[0xec - 0xe5];
    float m_z_nominal;   // 0xec (SOCOM 1: 0x64)
    unsigned char _f0[0xf4 - 0xf0];
    /* SOCOM II's, put back with the death camera (the names ours) */
    signed char m_f4;   // 0xf4
    signed char m_f5;   // 0xf5
    unsigned char _f6[0x104 - 0xf6];
    float m_104;   // 0x104
    float m_108;   // 0x108
    float m_10c;   // 0x10c
    int m_110;   // 0x110 (-1)
    float m_114;   // 0x114
    float m_118;   // 0x118 (1)
    unsigned char _11c[0x130 - 0x11c];
    float m_wiggle_cur_time;   // 0x130 (SOCOM 1: 0x80)
    unsigned char _134[0x138 - 0x134];
    unsigned char m_wiggle_echo;   // 0x138 (SOCOM 1: 0x88; SOCOM II's shake running)
    PLAYER_CAM_STATE m_camera_mode;   // 0x139 (SOCOM 1: 0x89)
    PLAYER_CAM_STATE m_camera_last_mode;   // 0x13a
    unsigned char _13b;
    CEntity *m_track;   // 0x13c (SOCOM 1: 0x8c)
    unsigned char m_save_peek;   // 0x140 (SOCOM 1: 0x90)
    CAMVIEW m_save_view;   // 0x141 (SOCOM 1: 0x91)
    unsigned char m_auto_death_cam;   // 0x142 (SOCOM 1: 0x92)
    unsigned char m_allow_player_death_cam_control;   // 0x143 (SOCOM 1: 0x93)
    unsigned char m_cam_death_state;   // 0x144 (SOCOM 1: 0x94, a CAM_DEATH_STATE)
    unsigned char _145[0x148 - 0x145];
    float m_death_state_timer;   // 0x148 (SOCOM 1: 0x98)
};

extern CAppCamera *appCamera;

/* CAMERA_SHAKE: how hard (SMALL, MEDIUM, LARGE: the presets' index) */
struct _camera_shake_cmd {
    _zanim_cmd_hdr hdr;
    unsigned char level;   // 0x4
};

#endif
