/* zcam_main.cpp -- zdb::CCamera: its region masks, field-of-view tests, clip planes and FOV, and
 * its ZAnim commands (OVL1 0x290600-...). The unit name is SOCOM 1's (its DWARF puts these
 * functions in zcam_main.cpp, in this order; the parameter and local names here are its). Defined
 * in reverse of their image order (-inline deferred). */

#include "zcamera.h"

/* fov.cpp's VU0 code */
unsigned int sceVu0ClipBBox(float lc[4][4], PNT4D *v);
int _OutsideGuardBand(float *minv, float *maxv, float ms[4][4], float vm[][4], int n);

namespace zdb {

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

/* a point's squared distance from the camera, scaled by the range scale */
float CCamera::GetScaledRangeSquared(CPnt3D *Pos)
{
    CPnt3D d = *Pos;

    d.Sub((const CPnt3D *)m_matrix.m_matrix[3], &d);
    return m_RangeScale * (d.x * d.x + d.y * d.y + d.z * d.z);
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

}
