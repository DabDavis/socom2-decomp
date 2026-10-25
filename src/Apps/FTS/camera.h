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

/* a SEAL: its matrix (the second virtual), its controller (CEntity's at 0xC0) and the zoomed flag
 * (SOCOM 1's at 0xF5C) */
class CZSealBody {
public:
    virtual ~CZSealBody();
    virtual CMatrix *GetMatrix();

    unsigned char _4[0xc0 - 4];
    CEntityCtrl *m_control;   // 0xc0
    unsigned char _c4[0x1060 - 0xc4];
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

class CAppCamera {
public:
    ~CAppCamera();
    virtual void Tick(float dT);
    virtual bool AttachTo(CZSealBody *prey);

    void SetZoom(float zoom);
    void SetCameraMode(unsigned char mode);
    void ResetView();
    static bool TickTask(float dT, void *data);
    void LookAt(const CPnt3D *from, const CPnt3D *to, CMatrix &cTw);
    static bool CmdTick3rdPersonTest(_zanim_cmd_hdr *hdr, float *dT);
    static _zanim_cmd_hdr *CmdParse3rdPersonTest(_zrdr *rdr);
    static _zanim_cmd_hdr *CmdParseShake(_zrdr *rdr);
    static void CmdBeginShake(_zanim_cmd_hdr *hdr);
    static void RegisterAnimCommands();

    CPnt3D m_cameraAim;   // 0x4 (SOCOM 1's)
    CPnt3D m_cameraTie;   // 0x10
    CPnt3D m_camGoalPos;   // 0x1c
    float m_goal_t_len;   // 0x28
    unsigned char _2c[0xb4 - 0x2c];
    zdb::CCamera *m_camera;   // 0xb4 (SOCOM 1: 0x34)
    zdb::CNode *m_camera_effects;   // 0xb8 (SOCOM 1: 0x38)
    CZSealBody *m_bc;   // 0xbc: SOCOM II's, a SEAL the mode setter asks after (the name ours)
    CZSealBody *m_entity;   // 0xc0 (SOCOM 1: 0x3c)
    unsigned int _c4;
    CZBodyPart *m_skel_root;   // 0xc8 (SOCOM 1: 0x40)
    CPnt3D m_rPos;   // 0xcc (SOCOM 1: 0x44)
    CPnt3D m_wPos;   // 0xd8 (SOCOM 1: 0x50)
    unsigned char m_ctrl_view;   // 0xe4 (SOCOM 1: 0x5c, a CAMVIEW)
    unsigned char _e5[0xec - 0xe5];
    float m_z_nominal;   // 0xec (SOCOM 1: 0x64)
    unsigned char _f0[0x139 - 0xf0];
    unsigned char m_camera_mode;   // 0x139 (SOCOM 1: 0x89)
    unsigned char m_camera_last_mode;   // 0x13a
    unsigned char _13b;
    CEntity *m_track;   // 0x13c (SOCOM 1: 0x8c)
    unsigned char m_save_peek;   // 0x140 (SOCOM 1: 0x90)
    unsigned char m_save_view;   // 0x141 (SOCOM 1: 0x91)
};

extern CAppCamera *appCamera;

/* CAMERA_SHAKE: how hard (SMALL, MEDIUM, LARGE: the presets' index) */
struct _camera_shake_cmd {
    _zanim_cmd_hdr hdr;
    unsigned char level;   // 0x4
};

#endif
