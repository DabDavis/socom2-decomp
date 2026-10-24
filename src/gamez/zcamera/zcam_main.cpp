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

}
