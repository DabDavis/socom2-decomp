/* CAppCamera -- the player's camera: first and third person, the tether, the death camera, the
 * wiggle -- and CameraZoom, the scope's zoom (SOCOM 1's Apps/FTS/camera.cpp and thezoom.cpp).
 * Layouts are SOCOM 1's DWARF moved to where SOCOM II's code reads them: CAppCamera grew 0x80 bytes
 * of shake presets ahead of its camera pointer (CAMERA_SHAKE's begin reads three sets of eight
 * floats from 0x50), and a few words further on. The header's name is ours. */
#ifndef CAMERA_H
#define CAMERA_H

#include "../../gamez/zcamera/zcamera.h"
#include "../../gamez/zanim/zanim.h"
#include <list>

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
    /* the hit's polygon read (int_main.cpp's, 0x2DC150-0x2DC1D0; the names the map's) */
    int GetPolyFlag27();
    bool IsInside();
    unsigned int GetRegion();
    int GetMaterial();

    CPnt3D m_pos;   // 0x0
    CDI *m_poly;   // 0xc
    CPnt3D m_norm;   // 0x10
    CNode *m_node;   // 0x1c
};

/* a ray's intersection set (SOCOM 1's DWARF; SOCOM II's hits are 4 bytes further on; its
 * constructor taking the hits' buffer size, 0x2DC6E0, and its destructor, 0x2DC680: node_world.h's
 * real class has both) */
class DiIntersect {
public:
    DiIntersect(int n);
    ~DiIntersect();

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
class CAppCamera;
class CZBodyPart;

/* a seal's controller: its throttle (zentity.h's CEntityCtrl: SOCOM 1's three axes at 0x8, SOCOM
 * II's four -- the death camera turns by the third and fourth) and its mode at 0x221 (5 hands the
 * zoom to the camera) */
class CEntityCtrl {
public:
    unsigned char _0[4];
    CEntity *m_entity;   // 0x4: what it drives (a turret's: the SEAL manning it)
    float m_throttle[4];   // 0x8
    unsigned char _18[0x220 - 0x18];
    unsigned char m_220;   // 0x220 (6 while its SEAL is down online: the camera's follow)
    unsigned char m_221;   // 0x221
};

/* which way a SEAL peeks (SOCOM 1's DWARF; a signed byte under -enum min) */
enum SEAL_PEEK {
    PEEK_LEFT = -1,
    PEEK_UP = 0,
    PEEK_RIGHT = 1,
    PEEK_NONE = 2
};

/* a SEAL's kit (zkit's CZKit: only what the zoom calls; zseal.h's member is one word, the SEAL's
 * own fields follow it) */
class CZKit {
public:
    void SetCurReticule(bool on);   /* 0x5BE570 */
    bool NVG_Enabled() const;   /* 0x5C80F0 */
    /* the lens effects on or off toward a colour (SOCOM 1's; 0x5C1800, the kit's round 38) */
    void Apply_LensEffects(bool on, CPnt3D *color);

    unsigned char _0[4];
};

/* a SEAL's body (zbody.h's CBody: only the part's place in space, OVL1 0x2869D0, and a part
 * found by name, 0x28E7B0) */
class CBody {
public:
    void PartInSpace(CZBodyPart *part, const CPnt3D *offset, CPnt3D *out, CQuat *q) const;
    CZBodyPart *FindPart(const char *name);

    unsigned char _0[4];
};

/* an entity (zentity.h's CEntity, 0x170 bytes: only what the camera reads -- zentity.h's
 * CEntityCtrl has no SEAL's mode, which the zoom tests through this header's): its matrix (the
 * second virtual, SOCOM 1's GetPosW), its type (a byte under -enum min), its position, its
 * controller, its flag word, and the list of every entity (the death camera walks it) */
class CEntity {
public:
    enum TYPE { ENT_TYPE_UNKNOWN = 0, ENT_TYPE_RECYCLE = 1, ENT_TYPE_SEAL = 2, ENT_TYPE_TURRET = 3 };

    virtual ~CEntity();
    virtual CMatrix *GetMatrix();
    /* its node (out of line, 0x29ED20: SOCOM 1's) */
    zdb::CNode *GetNode();

    unsigned char _4[0x10 - 4];
    TYPE m_type;   // 0x10
    unsigned char _11[0x1c - 0x11];
    CPnt3D m_posW;   // 0x1c (the node's position)
    unsigned char _28[0xc0 - 0x28];
    CEntityCtrl *m_control;   // 0xc0
    unsigned char _c4[0xe0 - 0xc4];
    unsigned int _e0 : 12;   // 0xe0: the flag word
    unsigned int m_isAlive : 1;
    unsigned int _e0_13 : 19;
    unsigned char _e4[0x170 - 0xe4];

    /* every entity (zentity.h's CEntityList, a list of pointers and nothing more) */
    static std::list<CEntity *> m_list;
    static const std::list<CEntity *> &GetEntityList() { return m_list; }
};

/* a SEAL (SOCOM II's CEntity at 0, its CBody at 0x170 -- the RTTI's base list, 0x3EFD20): its body's
 * state and wanted peek (SOCOM 1's CBody::m_state at 0x11D, SOCOM II's a halfword at 0x174; its
 * m_desiredPeek at 0x305, SOCOM II's 0x375) and the zoomed flag (SOCOM 1's at 0xF5C) */
class CZSealBody : public CEntity, public CBody {
public:
    /* the SEAL a node belongs to, if any (zseal.h's static) */
    static CZSealBody *GetSeal(zdb::CNode *node);
    /* the head turned to a pitch (OVL2 0x5AF470: SOCOM II's has no second float) */
    void RotateHead(float pitch);
    /* whether a ladder can be climbed from here (OVL2 0x59DA60, SOCOM 1's) */
    bool CanClimbLadder();
    /* the reticule's hit taken along the camera's matrix (OVL2 0x5AA6E0, SOCOM 1's CheckDIShoot:
     * the SEALs' round 48) */
    void CheckDIShoot(CPnt3D *pos, CMatrix *cTw);
    /* whether a player drives it (zseal.h's inline: the controller tested, IsPlayer called) */
    bool IsPlayer();

    /* the CBody base (SOCOM 1's at 0x110; the stand-in's first word only) */
    CBody &Body() { return *this; }

    short m_state;   // 0x174: 8 dead, 2 prone (SOCOM 1's SEAL_STATE: the body's, at its +4)
    unsigned char _176[0x2e8 - 0x176];
    CZBodyPart *m_root;   // 0x2e8 (SOCOM 1's 0x26c: the body's root part)
    unsigned char _2ec[0x375 - 0x2ec];
    SEAL_PEEK m_desiredPeek;   // 0x375
    unsigned char _376[0x5e0 - 0x376];
    CZKit m_kit;   // 0x5e0 (SOCOM 1: 0x520)
    unsigned char _5e4[0xea8 - 0x5e4];
    /* the camera watching the SEAL, if any (SOCOM II's: a follow online sets its mode, the name
     * ours) */
    CAppCamera *m_camera;   // 0xea8
    unsigned char _eac[0xf7a - 0xeac];
    /* the SEAL's life as the network reports it, 1 up (zseal.h's m_netlife, a NET_LIFE byte) */
    unsigned char m_netlife;   // 0xf7a
    unsigned char _f7b[0x105c - 0xf7b];
    unsigned int _105c : 23;   // 0x105c: zseal.h's flag word (SOCOM 1's at 0xf58)
    unsigned int m_inwater : 1;
    unsigned int _105c_24 : 8;
    unsigned char _b0 : 5;   // 0x1060
    unsigned char m_zooming : 1;
    unsigned char _b6 : 2;
    unsigned char _1061[0x1070 - 0x1061];
    CQuat m_fwdlook;   // 0x1070 (SOCOM 1: 0xf60; RotateHead sets it)
    unsigned char _1080[0x1344 - 0x1080];
    /* SOCOM II's: a scale the prone first-person camera reads, its y at least 0.5 (the name ours) */
    CPnt3D m_1344;   // 0x1344
    unsigned char _1350[0x1458 - 0x1350];
    CPnt3D m_aim_point;   // 0x1458 (SOCOM 1: 0x10b8)
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
    /* a gentle default (CAppCamera's constructor builds the three, then sets them) */
    CShakeParams()
        : m_xAmp(1.0f), m_xAmpRand(1.0f), m_xDecay(0.25f), m_xRate(1000.0f), m_yAmp(1.0f),
          m_yAmpRand(0.0f), m_yDecay(0.1f), m_yRate(750.0f)
    {
    }

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

/* the controller CAppCamera keeps at 0xC4 (the followed SEAL's CSealCtrl: zseal.h's virtuals to
 * IsPlayer at 0x2C, its pad at 0x114) and its float at 0x130, which looking at a point zeroes
 * (SOCOM 1's CEntityCtrl had it at the same place; the class name and m_130 ours) */
class CAppCameraCtrl {
public:
    virtual ~CAppCameraCtrl();
    virtual bool PreTick(float dT);
    virtual bool Tick(float dT);
    virtual bool PostMortemPreTick(float dT);
    virtual void OnDeath();
    virtual CAppCameraCtrl *GetRemoteControl();
    virtual void SetThrottle(unsigned int axis, float value);
    virtual bool PlayAiSound(int sound, float chance, int event);
    virtual bool AllowCommand(CZSealBody *seal, int cmd, void *data);
    virtual bool IsPlayer() const;   /* +0x2C */

    unsigned char _4[0x114 - 4];
    unsigned int m_padid;   // 0x114
    unsigned char _118[0x130 - 0x118];
    float m_130;   // 0x130
};

inline bool CZSealBody::IsPlayer()
{
    return m_control ? ((CAppCameraCtrl *)m_control)->IsPlayer() : false;
}

class CAppCamera {
public:
    CAppCamera(zdb::CCamera *cam);
    ~CAppCamera();
    virtual void Tick(float dT);
    virtual bool AttachTo(CZSealBody *prey);

    void FTSTick();
    /* SOCOM 1's camera modes and the death camera's points, in its order (APFirstPersonCam,
     * APTeatherCam, GetTetherCamPoints, GetDeadCamPoints, UpdateDeathCamState: Tick calls the two
     * views, each calls the next; SOCOM II's first person takes the frame time too) */
    void APFirstPersonCam(CMatrix &cTw, float dT);
    void APTeatherCam(CMatrix &cTw, float dT);
    unsigned char GetTetherCamPoints(CPnt3D *CamPoint, CPnt3D *CamOffset, CPnt3D *CamAimPoint, float dT);
    unsigned char GetDeadCamPoints(CPnt3D *CamPoint, CPnt3D *CamAimPoint, CMatrix *rot_mtx, float dT);
    void UpdateDeathCamState(float dT);
    /* the camera given an entity to follow -- a SEAL, or what it boards (the turret) -- and the
     * part of its body to hang from, the root if none: SOCOM 1's AttachTo's first half, split out
     * (0x2956E0; AttachTo, the constructor and the death camera call it, OVL2's turret boarding
     * too) -- and a SEAL's overload, a jump to it (0x295AF0; OVL2's turret release calls it). The
     * names ours */
    void Follow(CEntity *prey, const char *part);
    void Follow(CZSealBody *seal, const char *part);
    /* the death camera, 0xF4-0x11B (camera.cpp's CDeathCam: read through this at each use, its
     * fields are this one's offsets -- a local pointer to it takes a saved register) */
    struct CDeathCam *DeathCam() { return (struct CDeathCam *)&m_f4; }
    /* where the tether puts the camera, its offset and the tether's length handed back (SOCOM 1's,
     * 0x29A950) */
    CPnt3D GetThirdPersonCamPos(CPnt3D *CamOffset, float *tetherLen);
    /* the nearest hit of the world's along a chord, the followed SEAL and the one it carries kept out
     * of the test (SOCOM II's, 0x29CBB0: GetThirdPersonCamPos's; the name ours) */
    bool ClipChord(CPnt3D *from, CPnt3D *to, CPnt3D *hit);
    void APLookAt(float dT, CEntity *target, CMatrix &cTw);
    void UpdateCameraRegions();

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
    /* the chord from the aim to the goal cut short against the world (0x29BF70, SOCOM 1's) */
    void ClipCameraChord(float dT, CPnt3D *from, CPnt3D *to, CPnt3D *out);
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
    /* SOCOM II's: first person's eased camera point and aim point, the ease on (m_45), run over a
     * second (m_46: else six times the frame), and how far it has gone (the names ours) */
    CPnt3D m_easePos;   // 0x2c
    CPnt3D m_easeAim;   // 0x38
    /* SOCOM II's, set by the constructor (the names ours) */
    unsigned char m_44;   // 0x44 (1)
    unsigned char m_45;   // 0x45 (0)
    unsigned char m_46;   // 0x46 (1)
    unsigned char _47;
    float m_easeT;   // 0x48
    float m_short_tether_timer;   // 0x4c (SOCOM 1: 0x2c; -1)
    CShakeParams m_shake[3];   // 0x50: SMALL, MEDIUM, LARGE (SOCOM II's)
    unsigned int m_padid;   // 0xb0 (SOCOM 1: 0x30): the pad the views are cycled from
    zdb::CCamera *m_camera;   // 0xb4 (SOCOM 1: 0x34)
    zdb::CNode *m_camera_effects;   // 0xb8 (SOCOM 1: 0x38)
    CZSealBody *m_bc;   // 0xbc: SOCOM II's, a SEAL the mode setter asks after (the name ours)
    CZSealBody *m_entity;   // 0xc0 (SOCOM 1: 0x3c)
    /* SOCOM II's: the followed SEAL's controller, kept here -- APLookAt zeroes its float at 0x130
     * where SOCOM 1's reached it as m_entity->m_control (the names ours) */
    CAppCameraCtrl *m_ctrl;   // 0xc4
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
    unsigned char _f6[0xf8 - 0xf6];
    float m_f8;   // 0xf8 (SOCOM II's, the constructor zeroes these three; the names ours)
    float m_fc;   // 0xfc
    float m_100;   // 0x100
    float m_104;   // 0x104
    float m_108;   // 0x108
    int m_10c;   // 0x10c (an int: the constructor zeroes it with sw where its floats take swc1)
    int m_110;   // 0x110 (-1)
    int m_114;   // 0x114 (an int, likewise)
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
    unsigned char _14c[0x150 - 0x14c];
    /* the SEAL killed and its killer (SOCOM 1's m_killed / m_killer at 0x9C / 0xA0, their names:
     * the death camera's look carries its point about the first and aims at the second; the
     * constructor clears both) */
    CZSealBody *m_killed;   // 0x150
    CZSealBody *m_killer;   // 0x154
    unsigned char _158[0x188 - 0x158];
};   // 0x188 bytes (LoadWorld news 392)

extern CAppCamera *appCamera;

/* CAMERA_SHAKE: how hard (SMALL, MEDIUM, LARGE: the presets' index) */
struct _camera_shake_cmd {
    _zanim_cmd_hdr hdr;
    unsigned char level;   // 0x4
};

#endif
