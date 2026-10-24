/* zdb::CCamera -- the scene camera: its parameters, frustum and fog, the matrices it builds for
 * the VU each frame, and the region masks that decide what it may see. SOCOM 1's DWARF describes
 * it in full (CCamera : CNode, tag_CAMERA_PARAMS; 0x520 bytes), and SOCOM II keeps that layout up
 * to the region masks at 0x2A0; past them its fields move (the guard band comes forward before the
 * matrices, which sit 0x30 later; the FOV mask 0x50 later). The members SOCOM II's code has not
 * placed yet are padding named by offset. The header's name is ours. */
#ifndef ZCAMERA_H
#define ZCAMERA_H

#include <vector>

#include "../zmath/zmath.h"
#include "../znode/znode.h"
#include "../zvid/zvid.h"

class _zanim_cmd_hdr;
struct _zrdr;

namespace zdb {

enum tag_ZCAM_TYPE {
    ZCAM_NORMAL = 0,
    ZCAM_LANDMARK = 1,
    ZCAM_NUM_TYPES = 2
};

/* a field-of-view test's verdict */
enum tag_ZVIS_FOV {
    ZVIS_FOV_CLIP = 0,
    ZVIS_FOV_ALL_IN = 1,
    ZVIS_FOV_ALL_OUT = 2,
    ZVIS_FOV_NUM = 3
};

typedef unsigned int _word128 __attribute__((mode(TI)));

/* the per-frame block the camera hands the VU (SOCOM 1's DWARF) */
class tag_FrameSetup {
public:
    _word128 dmaTag;   // 0x0
    _word128 vifTag;   // 0x10
    float mtxWorldToScreen[4][4];   // 0x20
    float mtxWorldToClip[4][4];   // 0x60
    float mtxClipToScreen[4][4];   // 0xa0
    float mtxLightNormal[4][4];   // 0xe0
    float mtxLightColor[4][4];   // 0x120
    float fogA[4];   // 0x160
    float fogB[4];   // 0x170
    _word128 shadowGifTag;   // 0x180
};   // 0x190 bytes

class tag_ZCAM_MTX_SET {
public:
    float mtxWorldToView[4][4];   // 0x0
    float mtxWorldToClip[4][4];   // 0x40
    float mtxViewToClip[4][4];   // 0x80
    float mtxViewToScreen[4][4];   // 0xc0
    tag_FrameSetup *m_vu;   // 0x100
} __attribute__((aligned(16)));   // 0x110 bytes

/* the frustum's clip planes in the view (SOCOM 1's DWARF) */
class ZCAM_CLIP_DATA {
public:
    CPnt4D ViewPlanePoint;   // 0x0
    CPnt4D NearPlanePoint;   // 0x10
    CPnt4D NearPlaneNormal;   // 0x20
    CPnt4D LeftPlaneNormal;   // 0x30
    CPnt4D RightPlaneNormal;   // 0x40
    CPnt4D TopPlaneNormal;   // 0x50
    CPnt4D BottomPlaneNormal;   // 0x60
};   // 0x70 bytes

class tag_RECT {
public:
    int left;   // 0x0
    int top;   // 0x4
    int right;   // 0x8
    int bottom;   // 0xc
};   // 0x10 bytes

class tag_CAMERA_PARAMS {
public:
    CQuat m_quat;   // 0x0
    CPnt4D m_fog_color;   // 0x10
    float m_hfov;   // 0x20
    float m_vfov;   // 0x24
    float m_near_plane;   // 0x28
    float m_mid_plane;   // 0x2c
    float m_far_plane;   // 0x30
    CPnt3D m_px_fog_color;   // 0x34
    CPnt3D m_nx_fog_color;   // 0x40
    CPnt3D m_pz_fog_color;   // 0x4c
    CPnt3D m_nz_fog_color;   // 0x58
    float m_fog_near;   // 0x64
    float m_fog_far;   // 0x68
    float m_fog_mid;   // 0x6c
    float m_fogA;   // 0x70
    float m_fogB;   // 0x74
    float m_fog_density;   // 0x78
    float m_fog_top;   // 0x7c
    float m_fog_bottom;   // 0x80
    float m_landmark_fog_top;   // 0x84
    float m_landmark_fog_bottom;   // 0x88
    unsigned int m_flags_unused : 29;   // 0x8c
    unsigned int m_fog_enabled : 1;
    unsigned int m_directional_fog_enabled : 1;
    unsigned int m_fog_alt_enabled : 1;

    /* by value: CCamera::RestoreParams copies the saved colour out through one pointer onto the
     * stack, then into the camera's (the name is ours) */
    CPnt4D GetFogColor() const { return m_fog_color; }
};   // 0x90 bytes

class _tag_SAVE_CAMERA_PARAMS {
public:
    tag_CAMERA_PARAMS m_CameraParams;   // 0x0
    float m_RangeScale;   // 0x90
};   // 0xa0 bytes

class CCamera : public CNode, public tag_CAMERA_PARAMS {
public:
    CCamera();
    virtual ~CCamera();

    /* inline reads of the parameters, which the evaluation order shows (InsideMidPlane's mid
     * plane and TestFOG's fog loaded ahead of inlined code, TestFOG's altitude switch converted to
     * a bool before it is tested); the names are ours */
    float GetMidPlane() { return m_mid_plane; }
    float GetFogNear() { return m_fog_near; }
    float GetFogTop() { return m_fog_top; }
    bool GetFogAltEnabled() { return m_fog_alt_enabled; }
    /* the clip planes; SOCOM 1's near and far setters are out of line, SOCOM II's inline (it
     * links no copy), each mirrored into the guard band's w -- stored first, where SOCOM 1 stores
     * the plane first (SetMidClip, zAnimCameraTick) */
    void SetNearClip(float f)
    {
        m_guardBand_min[3] = f;
        m_near_plane = f;
    }
    void SetFarClip(float f)
    {
        m_guardBand_max[3] = f;
        m_far_plane = f;
    }
    void SetMidClip(float f);
    /* SOCOM II's parameter setters, inline (the parameter restore shows each; the names are
     * ours): the switches go through a bool, each fog bound recomputes the altitude terms */
    void SetFogEnabled(bool on) { m_fog_enabled = on; }
    void SetFogAltEnabled(bool on) { m_fog_alt_enabled = on; }
    void SetFogColor(const CPnt4D &c) { m_fog_color = c; }
    void UpdateFogAlt()
    {
        m_fog_alt[1].y = 1.0f / (m_fog_top - m_fog_bottom);
        m_fog_alt[0].y = m_fog_bottom;
        m_landmark_fog_alt[1].y = 1.0f / (m_landmark_fog_top - m_landmark_fog_bottom);
        m_landmark_fog_alt[0].y = m_landmark_fog_bottom;
    }
    void SetFogBottom(float f)
    {
        m_fog_bottom = f;
        UpdateFogAlt();
    }
    void SetFogTop(float f)
    {
        m_fog_top = f;
        UpdateFogAlt();
    }
    void RestoreParams();
    void Update(tag_ZCAM_TYPE type);
    void UpdateDirectionalFog();
    float GetFogVisibility(const CPnt3D *p1, const CPnt3D *p2) const;
    void UpdateFogAltVU();
    /* the parameters copied aside (CmdBeginParams; inline in both builds -- SOCOM 1's is one
     * line of it; SOCOM II's saves the fog colour too; the name is ours) */
    void SaveParams()
    {
        m_saveParams.m_CameraParams.m_fog_enabled = m_fog_enabled;
        m_saveParams.m_CameraParams.m_fog_alt_enabled = m_fog_alt_enabled;
        m_saveParams.m_CameraParams.m_hfov = m_hfov;
        m_saveParams.m_CameraParams.m_vfov = m_vfov;
        m_saveParams.m_CameraParams.m_near_plane = m_near_plane;
        m_saveParams.m_CameraParams.m_mid_plane = m_mid_plane;
        m_saveParams.m_CameraParams.m_far_plane = m_far_plane;
        m_saveParams.m_CameraParams.m_fog_color = m_fog_color;
        m_saveParams.m_CameraParams.m_fog_near = m_fog_near;
        m_saveParams.m_CameraParams.m_fog_far = m_fog_far;
        m_saveParams.m_CameraParams.m_fog_mid = m_fog_mid;
        m_saveParams.m_CameraParams.m_fog_bottom = m_fog_bottom;
        m_saveParams.m_CameraParams.m_fog_top = m_fog_top;
        m_saveParams.m_RangeScale = m_RangeScale;
    }
    static bool CmdTickParams(_zanim_cmd_hdr *hdr, float *dT);
    static void CmdBeginParams(_zanim_cmd_hdr *hdr);
    static _zanim_cmd_hdr *CmdParseParams(_zrdr *rdr);
    static bool CmdTickGetRegions(_zanim_cmd_hdr *hdr, float *dT);
    static _zanim_cmd_hdr *CmdParseGetRegions(_zrdr *rdr);
    static bool CmdTickSetRegions(_zanim_cmd_hdr *hdr, float *dT);
    static _zanim_cmd_hdr *CmdParseSetRegions(_zrdr *rdr);
    static bool CmdTickIndoorsTest(_zanim_cmd_hdr *hdr, float *dT);
    static _zanim_cmd_hdr *CmdParseIndoorsTest(_zrdr *rdr);
    static bool CmdTickControlSwitch(_zanim_cmd_hdr *hdr, float *dT);
    static _zanim_cmd_hdr *CmdParseAcquire(_zrdr *rdr);
    static _zanim_cmd_hdr *CmdParseRelease(_zrdr *rdr);
    static void RegisterAnimCommands();
    void SetHalfVerticalFOVRadians(float f, bool tan);
    void SetVerticalFOV(float fov);
    void SetHalfHorizontalFOVRadians(float f, bool tan);
    void SetHorizontalFOV(float fov);
    void SetScreenOffsetY(float y);
    void SetScreenOffsetX(float x);
    tag_FrameSetup *GetVuMtxSet();
    void GetLandmarkFarPlane(CWorld *world);
    bool TestLandmarkFOG(const CPnt3D *worldPos, float radius);
    bool TestFOG(const CPnt3D *worldPos, float radius);
    float GetZ(CPnt3D *pos);
    bool InsideMidPlane(CNode *node);
    bool InsideMidPlane(CCell *cell);
    float GetScaledRangeSquared(CPnt3D *Pos);
    unsigned int GetCameraCanSeeRegion();
    unsigned int GetPlayerCanSeeRegion();
    bool PlayerCanSeeRegion(unsigned int region);
    bool CanSeeRegion(unsigned int region);
    void SetPlayerCanSeeRegions(unsigned int regions);
    void SetCameraCanSeeRegions(unsigned int regions);
    tag_ZVIS_FOV TestFOV(PNT4D *bbox8, unsigned int *clipWord, bool testGuardBand);
    void theta_x_z(CPnt3D *p1, CPnt3D *p2, float *angle, float *dist);
    int find_convex_hull_xz(CPnt3D *pnts, int pnt_count);
    void UpdateFrustum();

    /* bools (MW's DWARF writes bool as unsigned char): the ticks return them as they are */
    static bool m_dynamics_controlled;
    static bool m_do_region_test;

    CPnt3D m_frustum[3];   // 0x150
    CPnt3D m_fullfrustum[6];   // 0x174
    int m_fullfrustum_points;   // 0x1bc
    float m_sin[2];   // 0x1c0 (SOCOM 1's PNT2Ds)
    float m_cos[2];   // 0x1c8
    float m_tan[2];   // 0x1d0
    float m_cot[2];   // 0x1d8
    std::vector<void *> m_LensFlareList;   // 0x1e0
    _tag_SAVE_CAMERA_PARAMS m_saveParams;   // 0x1f0
    float m_scrZ;   // 0x290
    std::vector<void *> m_lensflare_vec;   // 0x294
    unsigned int m_PlayerCanSeeRegions;   // 0x2a0
    unsigned int m_CameraCanSeeRegions;   // 0x2a4
    unsigned int m_SeeRegions;   // 0x2a8
    CPnt3D m_worldPos;   // 0x2ac: SOCOM II's, the camera's origin in the world (Update; the name
                         //   is ours)
    bool m_IsInside;   // 0x2b8 (CmdTickIndoorsTest; SOCOM 1: 0x2ac)
    unsigned char _socom2_2b9[0x2c4 - 0x2b9];   // SOCOM 1's m_AboveMaterial among them
    float m_landmark_far_plane;   // 0x2c4 (GetLandmarkFarPlane; SOCOM 1: 0x2b4)
    float m_ZoomedRangeScale;   // 0x2c8: SOCOM II's, the range scale over the zoom (CAppCamera
                                //   sets it; GetScaledRangeSquared reads it; the name is ours)
    float m_RangeScale;   // 0x2cc (SOCOM 1: 0x2b8; saved and restored with the parameters)
    float m_guardBand_min[4];   // 0x2d0 (SOCOM 1: 0x480)
    float m_guardBand_max[4];   // 0x2e0 (SOCOM 1: 0x490)
    tag_ZCAM_MTX_SET m_mtxSet;   // 0x2f0 (SOCOM 1: 0x2c0)
    /* SOCOM 1's members after the matrices, 0x30 later (the screen offset's setters; the clip
     * volume, 0x10 later, with the guard band moved out from before it) */
    ZCAM_CLIP_DATA m_ClipSet;   // 0x400
    float m_screenAspect[2];   // 0x470 (SOCOM 1's PNT2Ds)
    float m_screenCenter[2];   // 0x478
    float m_screenOffset[2];   // 0x480
    tag_RECT m_screen;   // 0x488
    float m_screenConstant[2];   // 0x498
    float m_Zmin;   // 0x4a0
    float m_Zmax;   // 0x4a4
    sceVu0FVECTOR m_vecClipVolume;   // 0x4b0 (its x scales the cotangent into m_scrZ)
    float m_farCorner[2][4];   // 0x4c0
    CPnt4D m_fog_alt[2];   // 0x4e0: the altitude fog's bottom (y) and 1 / its depth (y)
    CPnt4D m_landmark_fog_alt[2];   // 0x500: the same for the landmarks
    /* SOCOM II's: the altitude fog's vectors as the VU takes them, the fog's A and B in w, twice
     * (UpdateFogAltVU; the names are ours) */
    CPnt4D m_vuFogAlt[2];   // 0x520
    CPnt4D m_vuLandmarkFogAlt[2];   // 0x540
    tag_ZCAM_TYPE m_type;   // 0x560 (SOCOM 1: 0x510)
    unsigned int m_fovMask;   // 0x564 (SOCOM 1: 0x514)
};

}

#endif
