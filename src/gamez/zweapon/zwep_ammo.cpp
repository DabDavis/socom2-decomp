/* zwep_ammo.cpp -- CZAmmo, a cartridge type's stats (SOCOM 1's zwep_ammo.cpp, by its static
 * initialiser, which builds m_ammolist): OVL1 0x3D4380-0x3D480F, after zwep_weapon.cpp and before
 * the pointer vector of 0x3D4810, as SOCOM 1's 0x31FA80-0x31FCEF follows its zwep_weapon.cpp.
 * SOCOM II keeps SOCOM 1's functions in SOCOM 1's order and adds twelve accessors after the two
 * statics, a getter after GetName and a setter before SetDescription. Defined in reverse of their
 * image order (-inline deferred). */

#include "zweapon.h"

/* the allocators, with a file and line SOCOM II passes as 0 (MAIN's copies) */
void _free(void *p, const char *file, int line);
char *_strdup(const char *s, const char *file, int line);

/* the animations (zanim.h's CZAnimMain: only the lookup) */
class CZAnimMain {
public:
    CZAnim *GetAnim(const char *name, const char *set_name);
};
extern CZAnimMain ZAnim;

CZAmmoList CZAmmo::m_ammolist;

/* everything cleared, the id none, one projectile a shot at full acceleration */
CZAmmo::CZAmmo()
{
    m_name = 0;
    m_description = 0;
    m_longdescription = 0;
    m_hitanim = 0;
    m_ID = AMMO_NONE;
    m_numProjectilesFired = 1;
    m_blowback[0] = 0.0f;
    m_blowback[1] = 0.0f;
    m_proximityDistance = 0.0f;
    m_accelerationFactor = 1.0f;
    SetVolatile(false);
    SetIgnoreExplosionDI(false);
}

/* the three strings freed */
CZAmmo::~CZAmmo()
{
    if (m_name)
        _free(m_name, 0, 0);
    m_name = 0;
    if (m_description)
        _free(m_description, 0, 0);
    m_description = 0;
    if (m_longdescription)
        _free(m_longdescription, 0, 0);
    m_longdescription = 0;
}

void CZAmmo::SetName(char *s)
{
    if (m_name)
        _free(m_name, 0, 0);
    m_name = 0;
    m_name = _strdup(s, 0, 0);
}

void CZAmmo::SetDescription(char *s)
{
    if (m_description)
        _free(m_description, 0, 0);
    m_description = 0;
    m_description = _strdup(s, 0, 0);
}

void CZAmmo::SetLongDescription(char *s)
{
    if (m_longdescription)
        _free(m_longdescription, 0, 0);
    m_longdescription = _strdup(s, 0, 0);
}

void CZAmmo::SetHitAnim(const char *name)
{
    m_hitanim = ZAnim.GetAnim(name, 0);
}

void CZAmmo::SetImpactDamage(float d)
{
    m_impactDamage = d;
}

void CZAmmo::SetStun(float s)
{
    m_stun = s;
}

void CZAmmo::SetArmorPierce(float p)
{
    m_pierce = p;
}

void CZAmmo::SetExplosionDamage(float d)
{
    m_explosionDamage = d;
}

void CZAmmo::SetExplosionRadius(float r)
{
    m_explosionRadius = r;
}

char *CZAmmo::GetDescription()
{
    return m_description;
}

char *CZAmmo::GetName()
{
    return m_name;
}

float CZAmmo::GetImpactDamage()
{
    return m_impactDamage;
}

float CZAmmo::GetArmorPierce()
{
    return m_pierce;
}

float CZAmmo::GetExplosionDamage()
{
    return m_explosionDamage;
}

float CZAmmo::GetExplosionRadius()
{
    return m_explosionRadius;
}

void CZAmmo::SetNumProjectilesFired(int n)
{
    m_numProjectilesFired = n;
}

int CZAmmo::GetNumProjectilesFired()
{
    return m_numProjectilesFired;
}

void CZAmmo::SetBlowback(unsigned char which, float d)
{
    m_blowback[which] = d * d;
}

/* (the early return written as a negated `!=`: `== 0.0f` and an if-block both fold its jump into
 * one bc1t, where the target branches over a `b` to the end) */
float CZAmmo::GetBlowbackScale(float d2)
{
    float s = 0.0f;

    if (!(m_blowback[1] != 0.0f))
        return s;
    if (d2 < m_blowback[0])
        s = 1.0f;
    else if (d2 < m_blowback[1])
        s = 1.0f - (d2 - m_blowback[0]) / (m_blowback[1] - m_blowback[0]);
    return s;
}

void CZAmmo::SetProximityDistance(float d)
{
    m_proximityDistance = d * d;
}

float CZAmmo::GetProximityDistance()
{
    return m_proximityDistance;
}

float CZAmmo::GetAccelerationFactor()
{
    return m_accelerationFactor;
}

void CZAmmo::SetAccelerationFactor(float f)
{
    m_accelerationFactor = f;
}

bool CZAmmo::IsVolatile()
{
    return m_volatile != 0;
}

void CZAmmo::SetVolatile(bool on)
{
    m_volatile = on;
}

bool CZAmmo::GetIgnoreExplosionDI()
{
    return m_ignoreExplosionDI != 0;
}

void CZAmmo::SetIgnoreExplosionDI(bool on)
{
    m_ignoreExplosionDI = on;
}

CZAmmo *CZAmmo::GetAmmo(EQUIP_AMMO id)
{
    return m_ammolist.GetAmmo(id);
}

EQUIP_AMMO CZAmmo::GetAmmoId(const char *name)
{
    return m_ammolist.GetAmmoId(name);
}
