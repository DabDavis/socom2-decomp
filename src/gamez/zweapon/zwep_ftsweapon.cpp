/* zwep_ftsweapon.cpp -- CZFTSWeapon, the weapon as the game uses it, and its CTargetMods: OVL1
 * 0x3C5890-0x3C63C3, after zwep_explosive.cpp's PlacedExplosive and before zwep_global.cpp's
 * weapon callbacks, as SOCOM 1's (0x315760-0x315F3F in the demo). SOCOM 1 gives the unit no
 * static initialiser and no assert, so the file name is ours. Recoil is the class's first
 * non-inline virtual, so its vtable and the implicit destructor (0x3C5890, first in the image)
 * come out here. Only the functions named so far are here; the accessors between them stay
 * generated drafts. Defined in reverse of their image order (-inline deferred). */

#include "../zmath/zmath.h"
#include "zweapon.h"

CZFTSWeapon::CZFTSWeapon()
{
    m_rumblehigh_enable = 0;
    m_rumblehigh_time = 0.0f;
    m_rumblelow_enable = 0;
    m_rumblelow_time = 0.0f;
    m_rumblelow_power = 0.0f;
    m_accuracy_burstcnt_min = 0xFFFFFF;
    m_accuracy_burstcnt_max = 0xFFFFFF;
    m_accuracy_scalar_min = 1.0f;
    m_accuracy_scalar_max = 1.0f;
    m_accuracy_slope = 0.0f;
    m_type = (SEAL_ITEM)1;
}

int CZFTSWeapon::AccuracyBurstCnt_Min() const
{
    return m_accuracy_burstcnt_min;
}

int CZFTSWeapon::AccuracyBurstCnt_Max() const
{
    return m_accuracy_burstcnt_max;
}

/* each setter stores its value and recomputes the slope of the accuracy over the burst count
 * (SOCOM 1's four, the same shape) */
void CZFTSWeapon::AccuracyBurstCnt_Min(int n)
{
    m_accuracy_burstcnt_min = n;
    float count = m_accuracy_burstcnt_max - m_accuracy_burstcnt_min;

    m_accuracy_slope = count != 0.0f ? (m_accuracy_scalar_max - m_accuracy_scalar_min) / count : 0.0f;
}

void CZFTSWeapon::AccuracyBurstCnt_Max(int n)
{
    m_accuracy_burstcnt_max = n;
    float count = m_accuracy_burstcnt_max - m_accuracy_burstcnt_min;

    m_accuracy_slope = count != 0.0f ? (m_accuracy_scalar_max - m_accuracy_scalar_min) / count : 0.0f;
}

void CZFTSWeapon::AccuracyScalar_Min(float s)
{
    m_accuracy_scalar_min = s;
    float count = m_accuracy_burstcnt_max - m_accuracy_burstcnt_min;

    m_accuracy_slope = count != 0.0f ? (m_accuracy_scalar_max - m_accuracy_scalar_min) / count : 0.0f;
}

void CZFTSWeapon::AccuracyScalar_Max(float s)
{
    m_accuracy_scalar_max = s;
    float count = m_accuracy_burstcnt_max - m_accuracy_burstcnt_min;

    m_accuracy_slope = count != 0.0f ? (m_accuracy_scalar_max - m_accuracy_scalar_min) / count : 0.0f;
}

void CZFTSWeapon::Recoil()
{
}

void CTargetMods::SetTarget_DilateUponMovement(float d)
{
    m_target_dilateuponmovement = d;
    m_target_dilateuponmovementSqrt = sqrtf(d);
}

void CZFTSWeapon::AddZoommode(float zoom)
{
    m_zoommode.push_back(zoom);
}

/* the static test inlined (its true an assignment here: daddiu) */
bool CZFTSWeapon::IsM203Weapon()
{
    return IsM203Weapon(m_ID);
}

bool CZFTSWeapon::IsM203Weapon(EQUIP_ITEM id)
{
    if (id == 52 || id == 61)
        return true;
    return false;
}

bool CZFTSWeapon::IsGrenadeLauncherWeapon()
{
    if (m_ID == 52 || m_ID == 61 || m_ID == 142 || m_ID == 143 || m_ID == 63)
        return true;
    return false;
}

float CTargetMods::GetWiggleReport()
{
    return m_wiggle_report;
}

void CTargetMods::SetWiggleReport(float r)
{
    m_wiggle_report = r;
}

CTargetMods *CZFTSWeapon::TargetMods(int i)
{
    if (i > 2)
        i = 0;
    return &targetmods[i];
}

CTargetMods::CTargetMods()
{
    m_reticuleknock = 0.0f;
    m_reticuleknock_return = 0.0f;
    m_reticuleknock_max = 0.0f;
    m_distppframe_posx = 0.0f;
    m_distppframe_posy = 0.0f;
    m_distlimit_posx = 0.0f;
    m_distlimit_posy = 0.0f;
    m_sniper_decayrate = 0.0f;
    m_target_dilateuponfire = 0.0f;
    m_target_dilateuponmovement = 0.0f;
    m_socom2_28 = 1.0f;
    m_target_dilateuponmovementSqrt = 0.0f;
    m_target_constrict = 0.0f;
    m_target_min = 0.0f;
    m_target_max = 0.0f;
    m_firerifle_kickrate = 0.0f;
    m_firerifle_kickbasedist = 0.0f;
    m_firerifle_kickrandomdist = 0.0f;
    m_socom2_4c = 2.0f;
    m_socom2_50 = 1.0f;
    m_socom2_54 = 2.0f;
    m_socom2_58 = 2000.0f;
    m_socom2_5c = 1.0f;
    m_socom2_60 = 1.0f;
    m_socom2_64 = 2.0f;
    m_socom2_68 = 250.0f;
    m_socom2_6c = 3;
    m_wiggle_report = 1.0f;
}

float CZFTSWeapon::GetZoomRange(unsigned int i)
{
    if (m_zoommode.size() && i < m_zoommode.size())
        return m_zoommode[i];
    return 500.0f;
}

float CZFTSWeapon::ResolveFTSDamage(float damage)
{
    return 14.0f * damage;
}
