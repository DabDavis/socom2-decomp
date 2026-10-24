/* zcam_main.cpp -- zdb::CCamera: its region masks, field-of-view tests, clip planes and FOV, and
 * its ZAnim commands (OVL1 0x290600-...). The unit name is SOCOM 1's (its DWARF puts these
 * functions in zcam_main.cpp, in this order; the parameter and local names here are its). Defined
 * in reverse of their image order (-inline deferred). */

#include "zcamera.h"

/* fov.cpp's VU0 code */
unsigned int sceVu0ClipBBox(float lc[4][4], PNT4D *v);
int _OutsideGuardBand(float *minv, float *maxv, float ms[4][4], float vm[][4], int n);

namespace zdb {

/* fog is tested at all (SOCOM 1's file static, 0x3DE210 here) */
static bool fogCheck = true;

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

/* the horizontal field of view, in degrees (SOCOM II's; the name is ours) */
void CCamera::SetHorizontalFOV(float fov)
{
    m_hfov = 0.017453292f * (0.5f * fov);
    SetHalfHorizontalFOVRadians(m_hfov, true);
}

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
void CCamera::SetVerticalFOV(float fov)
{
    m_vfov = 0.017453292f * (0.5f * fov);
    SetHalfVerticalFOVRadians(m_vfov, true);
}

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

    for (CNodeVector::const_iterator it = world->GetLandmarks()->begin();
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
