/* zwep_weapon.cpp -- CZWeapon (SOCOM 1's zwep_weapon.cpp, by its static initialiser and the press
 * beta's asserts): OVL1 0x3CF1F0?-0x3D425F, after ZWepReader and before zwep_ammo.cpp's CZAmmo. SOCOM
 * 1's unit also holds CZDecalSet and CZMarkList. Only some of its functions so far; the rest stay
 * generated drafts or placeholders. Defined in reverse of their image order (-inline deferred). */

#include <string.h>

#include "zweapon.h"

extern "C" int strcasecmp(const char *a, const char *b);

/* every mode up to the maximum flagged (new in SOCOM II; the name is ours) */
void CZWeapon::SetMaxFireMode(FIREMODE mode)
{
    m_maxfiremode = mode;
    for (int i = 0; i <= m_maxfiremode && i < NUM_FIREMODES; i++)
        m_hasFireModes[i] = 1;
}

void CZWeapon::SetHasFireMode(FIREMODE mode)
{
    m_hasFireModes[mode] = 1;
    if (m_maxfiremode < mode)
        m_maxfiremode = mode;
}

bool CZWeapon::SetHitAnim(int material, CZAnim *anim)
{
    if (material < (int)CMaterialExArray::m_materialex_list.size()) {
        m_hitanim[material] = anim;
        return true;
    }
    return false;
}

bool CZWeapon::SetSpecialMaterialAnim(int material, CZAnim *anim)
{
    if (material < (int)CMaterialExArray::m_materialex_list.size()) {
        m_specialmaterialanim[material] = anim;
        return true;
    }
    return false;
}

CZAnim *CZWeapon::GetSpecialMaterialAnim(int material)
{
    if (material < (int)CMaterialExArray::m_materialex_list.size())
        return m_specialmaterialanim[material];
    return 0;
}

CZAnim *CZWeapon::GetHitAnim(int material)
{
    if (material < (int)CMaterialExArray::m_materialex_list.size())
        return m_hitanim[material];
    return 0;
}

/* the legal ammo list's first (SOCOM 1's; GetDefaultAmmoId inlines it) */
CZAmmo *CZWeapon::GetDefaultAmmo()
{
    CZAmmoList *list = &m_LegalAmmoList;

    if (list && (int)list->size() > 0)
        return list->GetAmmo(0);
    return 0;
}

EQUIP_AMMO CZWeapon::GetDefaultAmmoId()
{
    CZAmmo *ammo = GetDefaultAmmo();

    if (ammo)
        return ammo->m_ID;
    return AMMO_NONE;
}

/* the indirect-fire classes say no (SOCOM 1's; GetIDType() inlined) */
bool CZWeapon::IsDirectFire()
{
    EQUIP_ITEM type = GetIDType();

    if (type == 171 || type == 185 || type == 121 || type == 151 || type == 190 || type == 230)
        return false;
    return true;
}

/* the item's class's own one (inline: the member one is the static of the id byte, its out-of-line
 * copy 0x3D1E10, which other units call) */
EQUIP_ITEM CZWeapon::GetIDType()
{
    return GetIDType(m_ID);
}

/* a primary class (inline: its true an assignment in IsPrimary -- daddiu; the name is ours) */
static inline bool is_primary_type(EQUIP_ITEM type)
{
    if (type == 31 || type == 51 || type == 81 || type == 91 || type == 141 || type == 101)
        return true;
    return false;
}

bool CZWeapon::IsPrimary(EQUIP_ITEM id)
{
    return is_primary_type(GetIDType(id));
}

/* OPEN, 2 instructions short (SOCOM 1's is byte-identical): the target zero-extends the halfword's
 * 32 bits to 64 (dsll32, dsrl32) before storing the ammo byte; masks, a modulo and casts through
 * unsigned long all fold it away */
void CZWeapon::UnpackItemAndAmmoEnums(unsigned short packed, EQUIP_ITEM *item, EQUIP_AMMO *ammo)
{
    *item = (EQUIP_ITEM)(packed >> 8);
    *ammo = (EQUIP_AMMO)packed;
}

unsigned short CZWeapon::PackItemAndAmmoEnums(EQUIP_ITEM item, EQUIP_AMMO ammo)
{
    return (unsigned short)(item << 8) | ammo;
}

/* the two goggles have ids of their own (SOCOM II's 191 and 190, SOCOM 1's 184 and 182) */
EQUIP_ITEM CZWeapon::GetWeaponId(const char *name)
{
    EQUIP_ITEM id = EQUIP_NONE;

    if (name) {
        if (!strcasecmp(name, "NVG"))
            id = (EQUIP_ITEM)191;
        else if (!strcasecmp(name, "BINOCULARS"))
            id = (EQUIP_ITEM)190;
        else
            id = m_listWeapons.GetWeaponId(name);
    }
    return id;
}
