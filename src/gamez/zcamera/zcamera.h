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
};   // 0x90 bytes

class _tag_SAVE_CAMERA_PARAMS {
public:
    tag_CAMERA_PARAMS m_CameraParams;   // 0x0
    float m_RangeScale;   // 0x90
};   // 0xa0 bytes

class CCamera : public CNode, public tag_CAMERA_PARAMS {
public:
    /* an inline read of the mid plane (InsideMidPlane's evaluation order shows one; the name is
     * ours) */
    float GetMidPlane() { return m_mid_plane; }
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

    static unsigned char m_do_region_test;

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
    unsigned char m_IsInside;   // 0x2ac
    int m_AboveMaterial;   // 0x2b0
    float m_landmark_far_plane;   // 0x2b4
    unsigned char _socom2_2b8[0x2c8 - 0x2b8];
    float m_RangeScale;   // 0x2c8 (SOCOM 1: 0x2b8; the constructor sets it and 0x2cc to 1)
    float m_socom2_2cc;   // 0x2cc
    float m_guardBand_min[4];   // 0x2d0 (SOCOM 1: 0x480)
    float m_guardBand_max[4];   // 0x2e0 (SOCOM 1: 0x490)
    tag_ZCAM_MTX_SET m_mtxSet;   // 0x2f0 (SOCOM 1: 0x2c0)
    unsigned char _socom2_400[0x564 - 0x400];
    unsigned int m_fovMask;   // 0x564 (SOCOM 1: 0x514)
};

}

#endif
