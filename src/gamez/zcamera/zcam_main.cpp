/* zcam_main.cpp -- zdb::CCamera: its region masks, field-of-view tests, clip planes and FOV, and
 * its ZAnim commands (OVL1 0x290600-...). The unit name is SOCOM 1's (its DWARF puts these
 * functions in zcam_main.cpp, in this order; the parameter and local names here are its). Defined
 * in reverse of their image order (-inline deferred). */

#include "zcamera.h"
#include "../zanim/zanim.h"
#include "../zsys/zsys.h"

/* fov.cpp's VU0 code */
unsigned int sceVu0ClipBBox(float lc[4][4], PNT4D *v);
int _OutsideGuardBand(float *minv, float *maxv, float ms[4][4], float vm[][4], int n);

namespace zdb {

/* fog is tested at all (SOCOM 1's file static, 0x3DE210 here) */
static bool fogCheck = true;

/* the PARAMS command: what to save, restore and switch at its start, then the run time and the
 * keyed tracks of fov, clip planes, fog colour, range and altitude and LOD scale (offsets and
 * counts into the command's data) -- SOCOM 1's DWARF, with SOCOM II's fog colour track (0x18,
 * the command 0x28 bytes, as CmdParseParams allocates it) */
class _camera_params_cmd {
public:
    _zanim_cmd_hdr hdr;   // 0x0
    unsigned int save_params : 1;   // 0x4
    unsigned int restore_params : 1;
    unsigned int fog_enabled_set : 1;
    unsigned int fog_enabled : 1;
    unsigned int fog_altitude_enabled_set : 1;
    unsigned int fog_altitude_enabled : 1;
    float run_time;   // 0x8
    unsigned int fov_offset : 16;   // 0xc
    unsigned int fov_count : 16;
    unsigned int clip_offset : 16;   // 0x10
    unsigned int clip_count : 16;
    unsigned int clip_all_offset : 16;   // 0x14
    unsigned int clip_all_count : 16;
    unsigned int fog_color_offset : 16;   // 0x18
    unsigned int fog_color_count : 16;
    unsigned int fog_range_offset : 16;   // 0x1c
    unsigned int fog_range_count : 16;
    unsigned int fog_altitude_offset : 16;   // 0x20
    unsigned int fog_altitude_count : 16;
    unsigned int lod_multiplier_offset : 16;   // 0x24
    unsigned int lod_multiplier_count : 16;
};   // 0x28 bytes

/* a key of a track: its time and one, two or three values (SOCOM 1's DWARF) */
class _camera_param_1d_data {
public:
    float time;   // 0x0
    float a;   // 0x4
};   // 0x8 bytes

class _camera_param_2d_data {
public:
    float time;   // 0x0
    float a;   // 0x4
    float b;   // 0x8
};   // 0xc bytes

class _camera_param_3d_data {
public:
    float time;   // 0x0
    float a;   // 0x4
    float b;   // 0x8
    float c;   // 0xc
};   // 0x10 bytes

/* SET_REGIONS: the region test on or off */
class _camera_set_regions_cmd {
public:
    _zanim_cmd_hdr hdr;   // 0x0
    int set_on : 1;   // 0x4
};   // 0x8 bytes

/* ACQUIRE and RELEASE: who drives the camera */
class _camera_control_cmd {
public:
    _zanim_cmd_hdr hdr;   // 0x0
    int release : 1;   // 0x4
    int acquire : 1;
};   // 0x8 bytes

/* the camera's defaults: a 70 x 49 degree view from 4 to 300, the fog from 0.4 to 0.9 of the far
 * plane under an altitude fog 20 high (the landmarks' from 100 to 120), every region seen, the
 * screen from zVid, and the VU's frame block in scratchpad */
CCamera::CCamera()
{
    SetHorizontalFOV(70.0f);
    SetVerticalFOV(49.0f);
    SetNearClip(4.0f);
    SetFarClip(300.0f);
    m_mid_plane = 300.0f;
    m_landmark_far_plane = 300.0f;
    m_fullfrustum_points = 0;
    m_Zmin = zVid.minZ;
    m_Zmax = zVid.maxZ;

    m_fog_enabled = true;
    m_fog_near = 0.4f * m_far_plane;
    m_fog_far = 0.9f * m_far_plane;
    m_fog_mid = m_fog_far;
    m_fog_density = 1.0f;
    m_fog_color = CPnt4D(0.4f, 0.0f, 0.0f, 1.0f);
    m_directional_fog_enabled = false;
    m_px_fog_color = CPnt3D(1.0f, 0.0f, 0.0f);
    m_nx_fog_color = CPnt3D(0.0f, 1.0f, 0.0f);
    m_pz_fog_color = CPnt3D(1.0f, 0.0f, 1.0f);
    m_nz_fog_color = CPnt3D(1.0f, 1.0f, 0.0f);
    m_fog_alt_enabled = true;
    m_fog_top = 20.0f;
    m_fog_bottom = 0.0f;
    m_landmark_fog_top = 120.0f;
    m_landmark_fog_bottom = 100.0f;
    m_ZoomedRangeScale = 1.0f;
    m_RangeScale = 1.0f;
    m_fog_alt[0] = CPnt4D(0.0f, m_fog_bottom, 0.0f, 1.0f);
    m_fog_alt[1] = CPnt4D(0.0f, 1.0f / (m_fog_top - m_fog_bottom), 0.0f, 0.0f);
    m_landmark_fog_alt[0] = CPnt4D(0.0f, m_landmark_fog_bottom, 0.0f, 1.0f);
    m_landmark_fog_alt[1] = CPnt4D(0.0f, 1.0f / (m_landmark_fog_top - m_landmark_fog_bottom), 0.0f, 0.0f);
    UpdateFogAlt();
    UpdateFogAltVU();
    m_flags_unused = 0;

    m_PlayerCanSeeRegions = 0;
    m_CameraCanSeeRegions = 0;
    m_SeeRegions = 1;

    m_screenAspect[0] = zVid.aspect[0];
    m_screenAspect[1] = zVid.aspect[1];
    m_screenCenter[0] = 2048.0f;
    m_screenCenter[1] = 2048.0f;
    m_screenOffset[0] = 0.0f;
    m_screenOffset[1] = 0.0f;
    m_screen.left = zVid.screenLeft;
    m_screen.right = zVid.screenRight;
    m_screen.top = zVid.screenTop;
    m_screen.bottom = zVid.screenBottom;
    m_screenConstant[0] = (m_screen.right - m_screen.left) / zVid.renderWidth;
    m_screenConstant[1] = (m_screen.bottom - m_screen.top) / zVid.renderHeight;
    *(CPnt4D *)m_guardBand_min = CPnt4D(0.0f, 0.0f, 0.0f, m_near_plane);
    *(CPnt4D *)m_guardBand_max = CPnt4D(4096.0f, 4096.0f, 0.0f, m_far_plane);
    m_vecClipVolume[0] = zVid.renderWidth / 2;
    m_vecClipVolume[1] = zVid.renderHeight;
    m_vecClipVolume[2] = m_Zmin;
    m_vecClipVolume[3] = m_Zmax;
    m_mtxSet.m_vu = (tag_FrameSetup *)0x70003E60;
    m_fovMask = 0x3F3F;
}

/* the lens-flare lists go with the camera (their destructors inline, three levels each) */
CCamera::~CCamera()
{
}

/* the camera's ZAnim commands */
void CCamera::RegisterAnimCommands()
{
    ZAnim.AddCmd("DYNAMICS_RELEASE_CAMERA", CmdParseRelease, 0, CmdTickControlSwitch, 0);
    ZAnim.AddCmd("DYNAMICS_ACQUIRE_CAMERA", CmdParseAcquire, 0, CmdTickControlSwitch, 0);
    ZAnim.AddCmd("CAMERA_INDOORS", CmdParseIndoorsTest, 0, CmdTickIndoorsTest, 0);
    ZAnim.AddCmd("SET_CAMERA_REGION_TEST", CmdParseSetRegions, 0, CmdTickSetRegions, 0);
    ZAnim.AddCmd("GET_CAMERA_REGION_TEST", CmdParseGetRegions, 0, CmdTickGetRegions, 0);
    ZAnim.AddCmd("CAMERA_PARAMS", CmdParseParams, CmdBeginParams, CmdTickParams, 0);
}

/* RELEASE: a timeless command the camera's control switch reads */
_zanim_cmd_hdr *CCamera::CmdParseRelease(_zrdr *rdr)
{
    _camera_control_cmd *cmd;

    if (!rdr)
        return 0;
    cmd = (_camera_control_cmd *)_calloc(1, sizeof(_camera_control_cmd), 0, 0);
    cmd->hdr.data_size = sizeof(_camera_control_cmd);
    cmd->hdr.timeless = 1;
    cmd->release = 1;
    return (_zanim_cmd_hdr *)cmd;
}

/* ACQUIRE: the same, the other way */
_zanim_cmd_hdr *CCamera::CmdParseAcquire(_zrdr *rdr)
{
    _camera_control_cmd *cmd;

    if (!rdr)
        return 0;
    cmd = (_camera_control_cmd *)_calloc(1, sizeof(_camera_control_cmd), 0, 0);
    cmd->hdr.data_size = sizeof(_camera_control_cmd);
    cmd->hdr.timeless = 1;
    cmd->acquire = 1;
    return (_zanim_cmd_hdr *)cmd;
}

/* ACQUIRE / RELEASE: the camera's dynamics control taken or given back */
bool CCamera::CmdTickControlSwitch(_zanim_cmd_hdr *hdr, float *dT)
{
    _camera_control_cmd *cmd = (_camera_control_cmd *)hdr;

    if (cmd->release)
        m_dynamics_controlled = false;
    else if (cmd->acquire)
        m_dynamics_controlled = true;
    return true;
}

/* INDOORS_TEST: a bare timeless command */
_zanim_cmd_hdr *CCamera::CmdParseIndoorsTest(_zrdr *rdr)
{
    _zanim_cmd_hdr *cmd;

    if (!rdr)
        return 0;
    cmd = (_zanim_cmd_hdr *)_calloc(1, sizeof(_zanim_cmd_hdr), 0, 0);
    cmd->data_size = sizeof(_zanim_cmd_hdr);
    cmd->timeless = 1;
    return cmd;
}

/* INDOORS_TEST: whether the world's camera is indoors */
bool CCamera::CmdTickIndoorsTest(_zanim_cmd_hdr *hdr, float *dT)
{
    return CWorld::m_world->m_camera->m_IsInside;
}

/* SET_REGIONS: ON switches the region test on */
_zanim_cmd_hdr *CCamera::CmdParseSetRegions(_zrdr *rdr)
{
    _camera_set_regions_cmd *cmd;

    if (!rdr)
        return 0;
    cmd = (_camera_set_regions_cmd *)_calloc(1, sizeof(_camera_set_regions_cmd), 0, 0);
    cmd->hdr.data_size = sizeof(_camera_set_regions_cmd);
    cmd->hdr.timeless = 1;
    if (func_0032F0D0(rdr, "ON"))
        cmd->set_on = 1;
    else
        cmd->set_on = 0;
    return (_zanim_cmd_hdr *)cmd;
}

bool CCamera::CmdTickSetRegions(_zanim_cmd_hdr *hdr, float *dT)
{
    if (((_camera_set_regions_cmd *)hdr)->set_on)
        m_do_region_test = true;
    else
        m_do_region_test = false;
    return true;
}

/* GET_REGIONS: a bare timeless command */
_zanim_cmd_hdr *CCamera::CmdParseGetRegions(_zrdr *rdr)
{
    _zanim_cmd_hdr *cmd;

    if (!rdr)
        return 0;
    cmd = (_zanim_cmd_hdr *)_calloc(1, sizeof(_zanim_cmd_hdr), 0, 0);
    cmd->data_size = sizeof(_zanim_cmd_hdr);
    cmd->timeless = 1;
    return cmd;
}

/* GET_REGIONS: whether the region test is on */
bool CCamera::CmdTickGetRegions(_zanim_cmd_hdr *hdr, float *dT)
{
    return m_do_region_test;
}

/* the PARAMS command's start: the parameters saved if asked, and the fog switches set */
void CCamera::CmdBeginParams(_zanim_cmd_hdr *hdr)
{
    CCamera *cam = CWorld::m_world->m_camera;
    _camera_params_cmd *cmd_ptr = (_camera_params_cmd *)hdr;

    if (!cam)
        return;
    if (cmd_ptr->save_params)
        cam->SaveParams();
    if (cmd_ptr->fog_enabled_set)
        cam->SetFogEnabled(cmd_ptr->fog_enabled);
    if (cmd_ptr->fog_altitude_enabled_set)
        cam->SetFogAltEnabled(cmd_ptr->fog_altitude_enabled);
}

/* CAMERA_PARAMS: the flags (save, restore, fog and altitude fog on or off), then each keyed track
 * -- its keys appended to the command's data, the track's offset taken at its first key, the run
 * time the latest key's time (SOCOM 1's, with the fog colour track new) */
_zanim_cmd_hdr *CCamera::CmdParseParams(_zrdr *rdr)
{
    _camera_params_cmd *cmd;
    _zrdr *tag;

    if (!rdr)
        return 0;
    cmd = (_camera_params_cmd *)_calloc(1, sizeof(_camera_params_cmd), 0, 0);
    cmd->hdr.data_size = sizeof(_camera_params_cmd);
    cmd->hdr.timeless = 1;
    cmd->run_time = 0.0f;
    if (func_0032F0D0(rdr, "SAVE_PARAMS"))
        cmd->save_params = 1;
    if (func_0032F0D0(rdr, "RESTORE_PARAMS"))
        cmd->restore_params = 1;
    if (func_0032F0D0(rdr, "FOG_ON")) {
        cmd->fog_enabled_set = 1;
        cmd->fog_enabled = 1;
    } else if (func_0032F0D0(rdr, "FOG_OFF")) {
        cmd->fog_enabled_set = 1;
        cmd->fog_enabled = 0;
    }
    if (func_0032F0D0(rdr, "FOG_ALTITUDE_ON")) {
        cmd->fog_altitude_enabled_set = 1;
        cmd->fog_altitude_enabled = 1;
    } else if (func_0032F0D0(rdr, "FOG_ALTITUDE_OFF")) {
        cmd->fog_altitude_enabled_set = 1;
        cmd->fog_altitude_enabled = 0;
    }
    tag = func_0032F0D0(rdr, "FOV");
    if (tag) {
        for (unsigned int i = 0; i < tag->Count(); i += 3) {
            _camera_param_2d_data data;

            data.time = tag->Item(i)->Real();
            data.a = tag->Item(i + 1)->Real();
            data.b = tag->Item(i + 2)->Real();
            if (data.time > cmd->run_time)
                cmd->run_time = data.time;
            if (i == 0)
                cmd->fov_offset = cmd->hdr.data_size;
            cmd->fov_count++;
            cmd = (_camera_params_cmd *)zAnimCmdAddData(&cmd->hdr, &data, sizeof(data));
        }
    }
    tag = func_0032F0D0(rdr, "CLIP");
    if (tag) {
        for (unsigned int i = 0; i < tag->Count(); i += 3) {
            _camera_param_2d_data data;

            data.time = tag->Item(i)->Real();
            data.a = tag->Item(i + 1)->Real();
            data.b = tag->Item(i + 2)->Real();
            if (data.time > cmd->run_time)
                cmd->run_time = data.time;
            if (i == 0)
                cmd->clip_offset = cmd->hdr.data_size;
            cmd->clip_count++;
            cmd = (_camera_params_cmd *)zAnimCmdAddData(&cmd->hdr, &data, sizeof(data));
        }
    }
    tag = func_0032F0D0(rdr, "CLIP_ALL");
    if (tag) {
        for (unsigned int i = 0; i < tag->Count(); i += 4) {
            _camera_param_3d_data data;

            data.time = tag->Item(i)->Real();
            data.a = tag->Item(i + 1)->Real();
            data.b = tag->Item(i + 2)->Real();
            data.c = tag->Item(i + 3)->Real();
            if (data.time > cmd->run_time)
                cmd->run_time = data.time;
            if (i == 0)
                cmd->clip_all_offset = cmd->hdr.data_size;
            cmd->clip_all_count++;
            cmd = (_camera_params_cmd *)zAnimCmdAddData(&cmd->hdr, &data, sizeof(data));
        }
    }
    tag = func_0032F0D0(rdr, "FOG_COLOR");
    if (tag) {
        for (unsigned int i = 0; i < tag->Count(); i += 4) {
            _camera_param_3d_data data;

            data.time = tag->Item(i)->Real();
            data.a = tag->Item(i + 1)->Real();
            data.b = tag->Item(i + 2)->Real();
            data.c = tag->Item(i + 3)->Real();
            if (data.time > cmd->run_time)
                cmd->run_time = data.time;
            /* a component past 1 is a byte value, brought to 0..1 */
            if (data.a > 1.0f)
                data.a *= 0.00392157f;
            if (data.b > 1.0f)
                data.b *= 0.00392157f;
            if (data.c > 1.0f)
                data.c *= 0.00392157f;
            if (i == 0)
                cmd->fog_color_offset = cmd->hdr.data_size;
            cmd->fog_color_count++;
            cmd = (_camera_params_cmd *)zAnimCmdAddData(&cmd->hdr, &data, sizeof(data));
        }
    }
    tag = func_0032F0D0(rdr, "FOG_RANGE");
    if (tag) {
        for (unsigned int i = 0; i < tag->Count(); i += 3) {
            _camera_param_2d_data data;

            data.time = tag->Item(i)->Real();
            data.a = tag->Item(i + 1)->Real();
            data.b = tag->Item(i + 2)->Real();
            if (data.time > cmd->run_time)
                cmd->run_time = data.time;
            if (i == 0)
                cmd->fog_range_offset = cmd->hdr.data_size;
            cmd->fog_range_count++;
            cmd = (_camera_params_cmd *)zAnimCmdAddData(&cmd->hdr, &data, sizeof(data));
        }
    }
    tag = func_0032F0D0(rdr, "FOG_ALTITUDE");
    if (tag) {
        for (unsigned int i = 0; i < tag->Count(); i += 3) {
            _camera_param_2d_data data;

            data.time = tag->Item(i)->Real();
            data.a = tag->Item(i + 1)->Real();
            data.b = tag->Item(i + 2)->Real();
            if (data.time > cmd->run_time)
                cmd->run_time = data.time;
            if (i == 0)
                cmd->fog_altitude_offset = cmd->hdr.data_size;
            cmd->fog_altitude_count++;
            cmd = (_camera_params_cmd *)zAnimCmdAddData(&cmd->hdr, &data, sizeof(data));
        }
    }
    tag = func_0032F0D0(rdr, "LOD_MULTIPLIER");
    if (tag) {
        for (unsigned int i = 0; i < tag->Count(); i += 2) {
            _camera_param_1d_data data;

            data.time = tag->Item(i)->Real();
            if (i + 1 < tag->Count())
                data.a = tag->Item(i + 1)->Real();
            else
                data.a = 1.0f;
            if (data.time > cmd->run_time)
                cmd->run_time = data.time;
            if (i == 0)
                cmd->lod_multiplier_offset = cmd->hdr.data_size;
            cmd->lod_multiplier_count++;
            cmd = (_camera_params_cmd *)zAnimCmdAddData(&cmd->hdr, &data, sizeof(data));
        }
    }
    return (_zanim_cmd_hdr *)cmd;
}

/* the parameters CmdBeginParams saved, put back through their setters (SOCOM II's; CmdTickParams
 * calls it; the name is ours). SetMidClip inlines here, as the FOV setters do */
void CCamera::RestoreParams()
{
    SetFogEnabled(m_saveParams.m_CameraParams.m_fog_enabled);
    SetFogAltEnabled(m_saveParams.m_CameraParams.m_fog_alt_enabled);
    SetHalfHorizontalFOVRadians(m_saveParams.m_CameraParams.m_hfov, true);
    SetHalfVerticalFOVRadians(m_saveParams.m_CameraParams.m_vfov, true);
    SetNearClip(m_saveParams.m_CameraParams.m_near_plane);
    SetMidClip(m_saveParams.m_CameraParams.m_mid_plane);
    SetFarClip(m_saveParams.m_CameraParams.m_far_plane);
    SetFogColor(m_saveParams.m_CameraParams.GetFogColor());
    m_fog_near = m_saveParams.m_CameraParams.m_fog_near;
    m_fog_far = m_saveParams.m_CameraParams.m_fog_far;
    m_fog_mid = m_saveParams.m_CameraParams.m_fog_mid;
    SetFogBottom(m_saveParams.m_CameraParams.m_fog_bottom);
    SetFogTop(m_saveParams.m_CameraParams.m_fog_top);
    m_RangeScale = m_saveParams.m_RangeScale;
}

/* the screen offset, each axis held within 200 */
void CCamera::SetScreenOffsetX(float x)
{
    if (x > 200.0f)
        m_screenOffset[0] = 200.0f;
    else if (x < -200.0f)
        m_screenOffset[0] = -200.0f;
    else
        m_screenOffset[0] = x;
}

void CCamera::SetScreenOffsetY(float y)
{
    if (y > 200.0f)
        m_screenOffset[1] = 200.0f;
    else if (y < -200.0f)
        m_screenOffset[1] = -200.0f;
    else
        m_screenOffset[1] = y;
}

/* the horizontal half angle (radians), its sine, cosine and cotangent -- the tangent kept when
 * asked -- and the screen distance from the cotangent. SOCOM 1's setter, with the flag */
void CCamera::SetHalfHorizontalFOVRadians(float f, bool tan)
{
    m_hfov = f;
    m_sin[0] = sinf(f);
    m_cos[0] = cosf(f);
    if (tan) {
        m_tan[0] = m_sin[0] / m_cos[0];
        m_cot[0] = 1.0f / m_tan[0];
    } else
        m_cot[0] = 1.0f / (m_sin[0] / m_cos[0]);
    m_scrZ = m_vecClipVolume[0] * m_cot[0];
}

/* the horizontal field of view, in degrees (SOCOM II's; the name is ours). Pinned, as is
 * SetVerticalFOV: three statements, but the constructor and CmdTickParams call both */
#pragma auto_inline off
void CCamera::SetHorizontalFOV(float fov)
{
    m_hfov = 0.017453292f * (0.5f * fov);
    SetHalfHorizontalFOVRadians(m_hfov, true);
}

#pragma auto_inline reset

/* the vertical half angle from the horizontal one and the render size: the angle passed in is
 * replaced, as SOCOM 1 replaced it (its aspect fixed at 448/640). Both calls take the value, which
 * stays in a register (`sinf(m_vfov)` reloads the member for cosf) */
void CCamera::SetHalfVerticalFOVRadians(float f, bool tan)
{
    f = m_hfov * (float)zVid.renderHeight / (float)zVid.renderWidth;
    m_vfov = f;
    m_sin[1] = sinf(f);
    m_cos[1] = cosf(f);
    if (tan) {
        m_tan[1] = m_sin[1] / m_cos[1];
        m_cot[1] = 1.0f / m_tan[1];
    } else
        m_cot[1] = 1.0f / (m_sin[1] / m_cos[1]);
}

/* the vertical field of view, in degrees -- set, and then derived again from the horizontal one
 * (SOCOM II's; the name is ours) */
#pragma auto_inline off
void CCamera::SetVerticalFOV(float fov)
{
    m_vfov = 0.017453292f * (0.5f * fov);
    SetHalfVerticalFOVRadians(m_vfov, true);
}

#pragma auto_inline reset

/* the mid plane, the far plane pushed out to it */
void CCamera::SetMidClip(float f)
{
    m_mid_plane = f;
    if (f > m_far_plane)
        SetFarClip(f);
}

/* the frustum in the world: the camera's position and the far plane's left and right edges at
 * eye height (m_frustum), then the four far corners (m_fullfrustum) and their hull in x/z, the
 * position among them. The position is read through CMatrix's element operator (the matrix row
 * cast to a point keeps the row's element addresses in registers instead) */
void CCamera::UpdateFrustum()
{
    float l = m_far_plane * m_tan[0];

    m_frustum[0] = *(CPnt3D *)&m_matrix(3, 0);
    m_frustum[1] = CPnt3D(l, 0.0f, -m_far_plane);
    m_frustum[2] = CPnt3D(-l, 0.0f, -m_far_plane);
    m_matrix.Transform(&m_frustum[1], 2);

    float h = m_far_plane * m_tan[1];

    m_fullfrustum[0] = *(CPnt3D *)&m_matrix(3, 0);
    m_fullfrustum[1] = CPnt3D(l, -h, -m_far_plane);
    m_fullfrustum[2] = CPnt3D(-l, -h, -m_far_plane);
    m_fullfrustum[3] = CPnt3D(l, h, -m_far_plane);
    m_fullfrustum[4] = CPnt3D(-l, h, -m_far_plane);
    m_matrix.Transform(&m_fullfrustum[1], 4);
    m_fullfrustum_points = 5;
    m_fullfrustum_points = find_convex_hull_xz(m_fullfrustum, m_fullfrustum_points);
}

/* the convex hull of points in x/z, gift-wrapped in place (Sedgewick's package wrap): from the
 * point furthest in z, each next point the least pseudo-angle on (the furthest of equals),
 * swapped forward; the first point is copied past the last to close the walk. The count of hull
 * points, or 0 when the walk never closes. SOCOM 1's locals (max is the index of the point found);
 * its DWARF has no `t`, which lives in registers only. The swap assigns: `CPnt3D t(pnts[m])` would
 * copy-construct it into memory */
int CCamera::find_convex_hull_xz(CPnt3D *pnts, int pnt_count)
{
    int i;
    int m;
    int max;
    float mintheta;
    float theta_m_i;
    float dist_m_i;
    float besttheta;
    float bestdist;
    CPnt3D t;

    max = 0;
    for (i = 1; i < pnt_count; i++) {
        if (pnts[i].z > pnts[max].z || (pnts[i].z == pnts[max].z && pnts[i].x < pnts[max].x))
            max = i;
    }
    pnts[pnt_count] = pnts[max];
    besttheta = 0.0f;
    for (m = 0; m < pnt_count; m++) {
        t = pnts[m];
        pnts[m] = pnts[max];
        pnts[max] = t;
        mintheta = besttheta;
        besttheta = 4.0f;
        bestdist = 0.0f;
        max = pnt_count;
        for (i = m + 1; i <= pnt_count; i++) {
            theta_x_z(&pnts[m], &pnts[i], &theta_m_i, &dist_m_i);
            if (theta_m_i < mintheta)
                continue;
            if (dist_m_i <= 0.0f)
                continue;
            if (theta_m_i < besttheta || (theta_m_i == besttheta && dist_m_i > bestdist)) {
                max = i;
                besttheta = theta_m_i;
                bestdist = dist_m_i;
            }
        }
        if (max == pnt_count)
            return m + 1;
    }
    return 0;
}

/* the pseudo-angle of p1 -> p2 across x/z, 0 to 4 around the circle: dz over the Manhattan
 * distance (which comes back too), folded by quadrant -- Sedgewick's theta(), as
 * find_convex_hull_xz is his package wrap. A member that never reads `this` (the points arrive in
 * a1 and a2). Pinned: 13 statements here, but find_convex_hull_xz calls it in both builds */
#pragma auto_inline off
void CCamera::theta_x_z(CPnt3D *p1, CPnt3D *p2, float *angle, float *dist)
{
    float dx = p2->x - p1->x;
    float dz = p1->z - p2->z;
    float t = fabsf(dx) + fabsf(dz);

    *dist = t;
    if (t != 0.0f)
        t = dz / t;
    if (dx < 0.0f)
        t = 2.0f - t;
    else if (dz < 0.0f)
        t = t + 4.0f;
    *angle = t;
}

#pragma auto_inline reset

/* a box's eight corners against the view: all outside a plane the mask tests, all inside every
 * plane it tests, or straddling one -- which, asked to test the guard band, is a clip only when
 * a corner falls outside the band too */
tag_ZVIS_FOV CCamera::TestFOV(PNT4D *bbox8, unsigned int *clipWord, bool testGuardBand)
{
    unsigned int _clipWord;

    clipWord = clipWord ? clipWord : &_clipWord;
    *clipWord = sceVu0ClipBBox(m_mtxSet.mtxWorldToClip, bbox8);
    if (*clipWord & (m_fovMask & 0x3f))
        return ZVIS_FOV_ALL_OUT;
    if (!(*clipWord & (m_fovMask & 0x3f00)))
        return ZVIS_FOV_ALL_IN;
    if (testGuardBand && _OutsideGuardBand(m_guardBand_min, m_guardBand_max,
                                           m_mtxSet.m_vu->mtxWorldToScreen, (float(*)[4])bbox8, 8))
        return ZVIS_FOV_CLIP;
    return ZVIS_FOV_ALL_IN;
}

/* the regions the camera itself may see (region 1, the outdoors, always) */
void CCamera::SetCameraCanSeeRegions(unsigned int regions)
{
    if (regions) {
        m_CameraCanSeeRegions = regions;
        m_SeeRegions = m_PlayerCanSeeRegions | 1 | m_CameraCanSeeRegions;
    }
}

/* the regions the player may see */
void CCamera::SetPlayerCanSeeRegions(unsigned int regions)
{
    if (regions) {
        m_PlayerCanSeeRegions = regions;
        m_SeeRegions = m_PlayerCanSeeRegions | 1 | m_CameraCanSeeRegions;
    }
}

/* whether the camera may see into a region (0 the outdoors): always, with the region test off */
bool CCamera::CanSeeRegion(unsigned int region)
{
    if (!region)
        region = 1;
    if (!m_do_region_test || (m_SeeRegions & region))
        return true;
    return false;
}

/* whether the player may see into a region */
bool CCamera::PlayerCanSeeRegion(unsigned int region)
{
    if (!region)
        region = 1;
    unsigned int SeeRegions = GetPlayerCanSeeRegion();

    if (!m_do_region_test || (SeeRegions & region))
        return true;
    return false;
}

unsigned int CCamera::GetPlayerCanSeeRegion()
{
    return m_PlayerCanSeeRegions | 1;
}

/* SOCOM II's: the camera's own regions, the outdoors among them */
unsigned int CCamera::GetCameraCanSeeRegion()
{
    return m_CameraCanSeeRegions | 1;
}

/* a point's squared distance from the camera, scaled by the range scale -- SOCOM II's, over the
 * zoom (SOCOM 1 scaled by m_RangeScale itself) */
float CCamera::GetScaledRangeSquared(CPnt3D *Pos)
{
    CPnt3D d = *Pos;

    d.Sub((const CPnt3D *)m_matrix.m_matrix[3], &d);
    return m_ZoomedRangeScale * (d.x * d.x + d.y * d.y + d.z * d.z);
}

/* whether a cell reaches inside the mid plane: its depth less its radius. The mid plane is read
 * through an inline accessor -- an inline call in an expression is expanded ahead of it, so the
 * plane is loaded before GetRadius is called and kept in a saved register over it (read as a
 * member, it is loaded after the call and `this` is kept instead) */
bool CCamera::InsideMidPlane(CCell *cell)
{
    float DistToCell = GetZ((CPnt3D *)cell->m_matrix.m_matrix[3]);

    if (GetMidPlane() > DistToCell - cell->GetRadius())
        return true;
    return false;
}

/* whether a node's bounds reach inside the mid plane: the depth of their centre, in the world,
 * less half their diagonal. The locals are SOCOM 1's (its line table gives each step a line) */
bool CCamera::InsideMidPlane(CNode *node)
{
    float DistToNode;
    float NodeRad;
    CPnt3D NodeCtr;
    CPnt3D NodeDim;

    node->GetBBox()->GetCenter(&NodeCtr);
    node->m_matrix.Transform(&NodeCtr, 1);
    DistToNode = GetZ(&NodeCtr);
    node->GetBBox()->GetSize(&NodeDim);
    NodeRad = 0.5f * sqrtf(NodeDim.x * NodeDim.x + NodeDim.y * NodeDim.y + NodeDim.z * NodeDim.z);
    if (GetMidPlane() > DistToNode - NodeRad)
        return true;
    return false;
}

/* a point's depth along the view: (the camera's position - the point) . its direction, on VU0.
 * The locals are SOCOM 1's DWARF's in reverse -- MW's DWARF 1 lists a scope's locals backwards.
 * The VU code is a volatile asm BLOCK, Metrowerks' own syntax naming the C variables: SOCOM 1's
 * line table gives each instruction its own line (a GCC-style asm string shares the first two),
 * and -inline auto counts a block's instructions only, where a GCC asm statement costs its
 * operands too -- 14 statements against 17, and InsideMidPlane inlines this. Without `volatile`
 * the block lets MW see aPosPtr is &aPos and fold it into the return (an instruction longer) */
float CCamera::GetZ(CPnt3D *pos)
{
    CPnt4D *CamDir = (CPnt4D *)m_matrix.m_matrix[2];
    CPnt4D *CamPos = (CPnt4D *)m_matrix.m_matrix[3];
    CPnt4D aPos(pos->x, pos->y, pos->z, 1.0f);
    CPnt4D *aPosPtr = &aPos;

    asm volatile {
        lqc2 vf3, 0(CamDir)
        lqc2 vf4, 0(CamPos)
        lqc2 vf5, 0(aPosPtr)
        vsub.xyz vf6, vf4, vf5
        vmul.xyz vf6, vf6, vf3
        vmulax.w ACC, vf0, vf6
        vmadday.w ACC, vf0, vf6
        vmaddz.w vf6, vf0, vf6
        sqc2 vf6, 0(aPosPtr)
    }
    return aPosPtr->w;
}

/* the landmarks' far plane: twice the largest landmark's radius, at least the world's diagonal
 * across x/z, and four times the far plane when that is further still. The list is reached
 * through an inline accessor, whose result the loop keeps in a register of its own (SOCOM 1's
 * code does too, and its DWARF has no local for it) */
void CCamera::GetLandmarkFarPlane(CWorld *world)
{
    float maxRadius = 0.0f;

    for (CNodeVector::iterator it = world->GetLandmarks()->begin();
         it != world->GetLandmarks()->end(); ++it) {
        float lmRadius = (*it)->GetRadiusSq();

        if (lmRadius > maxRadius)
            maxRadius = lmRadius;
    }
    m_landmark_far_plane = 2.0f * sqrtf(maxRadius);

    float xx = world->m_grid.m_extents.left - world->m_grid.m_extents.right;
    float zz = world->m_grid.m_extents.top - world->m_grid.m_extents.bottom;
    float WorldDiagDist = sqrtf(xx * xx + zz * zz);

    if (WorldDiagDist > m_landmark_far_plane)
        m_landmark_far_plane = WorldDiagDist;
    if (m_far_plane > m_landmark_far_plane)
        m_landmark_far_plane = 4.0f * m_far_plane;
}

/* whether a sphere is in the fog: its far side past the fog's near distance, or (the altitude
 * fog) its bottom under the fog's top. The parameters are read through inline accessors: the
 * altitude switch is converted to a bool before it is tested (an sltu SOCOM 1's code has too, where
 * TestLandmarkFOG tests the bit), and the near distance is loaded ahead of GetZ's inlined code */
bool CCamera::TestFOG(const CPnt3D *worldPos, float radius)
{
    bool doFog = m_fog_enabled;
    bool doAltFog = false;

    if (doFog) {
        doAltFog = GetFogAltEnabled() ? worldPos->y - radius < GetFogTop() : false;
        if (radius + GetZ((CPnt3D *)worldPos) < GetFogNear())
            doFog = false;
    }
    return doAltFog | (doFog & fogCheck);
}

/* TestFOG for a landmark: its distance from the camera rather than its depth, and the
 * landmarks' own fog top */
bool CCamera::TestLandmarkFOG(const CPnt3D *worldPos, float radius)
{
    bool doFog = m_fog_enabled;
    bool doAltFog = false;

    if (doFog) {
        doAltFog = m_fog_alt_enabled ? worldPos->y - radius < m_landmark_fog_top : false;
        CPnt3D pos = *worldPos - *(const CPnt3D *)&m_matrix(3, 0);

        if (radius + sqrtf(pos.x * pos.x + pos.y * pos.y + pos.z * pos.z) < m_fog_near)
            doFog = false;
    }
    return doAltFog | (doFog & fogCheck);
}

/* the matrices the camera last built for the VU */
tag_FrameSetup *CCamera::GetVuMtxSet()
{
    return m_mtxSet.m_vu;
}

}
