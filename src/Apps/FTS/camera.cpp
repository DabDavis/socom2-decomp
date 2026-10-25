/* camera.cpp -- the player's camera (SOCOM 1's Apps/FTS/camera.cpp; OVL1 to 0x29CEBF). Defined
 * in reverse of their image order (-inline deferred). */

#include "camera.h"
#include "../../gamez/zsys/zsys.h"

/* unnamed yet */
extern "C" void *func_0032EE10(_zrdr *rdr, const char *tag);

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
    return !(appCamera->m_camera_mode == 3);
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

CAppCamera::~CAppCamera()
{
    m_track = 0;
    m_skel_root = 0;
    m_entity = 0;
    m_camera_effects = 0;
    m_camera = 0;
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
