/* zwep_weapon.cpp -- CZWeapon (SOCOM 1's zwep_weapon.cpp, by its static initialiser and the press
 * beta's asserts): OVL1 0x3CF1F0?-0x3D425F, after ZWepReader and before zwep_ammo.cpp's CZAmmo. SOCOM
 * 1's unit also holds CZDecalSet and CZMarkList. Only some of its functions so far; the rest stay
 * generated drafts or placeholders. Defined in reverse of their image order (-inline deferred). */

#include <string.h>

#include "zweapon.h"
#include "../znode/znode.h"
#include "../zrdr/zrdr.h"
#include "../zsys/zsys.h"

using namespace zdb;

extern "C" int strcasecmp(const char *a, const char *b);
void _free(void *p, const char *file, int line);
char *_strdup(const char *s, const char *file, int line);
/* the decals rdr (0x437D00) and a material's index by name (0x2DE9E0) -- not named */
extern "C" _zrdr *data_00437D00;
extern "C" int func_002DE9E0(const char *name);

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

int CZWeapon::GetAmmoCapacity()
{
    return m_ammocap;
}

int CZWeapon::GetNumMags()
{
    return m_nummags;
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

/* every set freed, its definitions and name first, then the vector's storage given up through
 * FreeVector, whose four pieces the target calls (SOCOM 1's, the file and line dropped) */
bool CZDecalSet::DestroyDecalSet()
{
    if (m_decalsets.size() == 0)
        return false;
    if (m_decalsets.size()) {
        do {
            CZDecalSet *set = m_decalsets.back();

            m_decalsets.pop_back();
            if (set) {
                if (set->m_decals)
                    _free(set->m_decals, 0, 0);
                set->m_decals = 0;
                if (set->m_name)
                    _free(set->m_name, 0, 0);
                set->m_name = 0;
                delete set;
            }
        } while (m_decalsets.size());
    }
    FreeVector<std::vector<CZDecalSet *> >(m_decalsets);
    return true;
}

/* the set of that name, or 0 (SOCOM 1's; an int counter keeps the element address recomputed) */
CZDecalSet *CZDecalSet::GetDecalSetByName(const char *name)
{
    for (int i = 0; i < m_decalsets.size(); i++) {
        if (!strcmp(m_decalsets[i]->m_name, name))
            return m_decalsets[i];
    }
    return 0;
}

/* each list entry of the decals rdr a set (SOCOM 1's; the rdr is a global the mark list's
 * ReadDecalNames and OVL2 share, 0x437D00 -- not named) */
void CZWeapon::OpenDecals()
{
    if (data_00437D00) {
        _zrdr *list = zrdr_findtag(data_00437D00, "DECAL_SETS");

        if (list) {
            int n = list->Count();

            for (int i = 0; i < n; i++)
                CZDecalSet::ReadDecalSet(list->Item(i));
        }
    }
}

/* (called, never inlined: OpenDecals calls it out of line; an int counter against the unsigned
 * count keeps Item's address recomputed each pass, as the target's) */
#pragma auto_inline off
void CZDecalSet::ReadDecalSet(_zrdr *rdr)
{
    if (!rdr)
        return;
    CZDecalSet *set = new CZDecalSet;
    unsigned int n = rdr->Count();

    for (int i = 0; i < n; i++)
        set->ReadDecalEntry(rdr->Item(i));
    m_decalsets.push_back(set);
}
#pragma auto_inline reset

/* a material's decal definition (inline: each use reloads the definitions, base first) */
static inline CZDecalDef &Decal(CZDecalSet *set, int i)
{
    return set->m_decals[i];
}

/* the set's name, its permanence, or a material's decal: its texture and size range, the texture
 * kept with every character's textures for "PERSON" and with the decals' otherwise */
void CZDecalSet::ReadDecalEntry(_zrdr *entry)
{
    if (!entry)
        return;
    const char *name = 0;
    const char *perm = 0;

    if (zrdr_findSTRING(entry, "SETNAME", &name)) {
        SetName(name);
        return;
    }
    if (zrdr_findSTRING(entry, "PERMENANT", &perm)) {
        m_permenant = 1;
        return;
    }
    const char *material = 0;

    if (!zrdr_findSTRING(entry, "MATERIALNAME", &material))
        return;
    int index = func_002DE9E0(material);
    const char *texture = 0;

    if (!zrdr_findSTRING(entry, "TEXTURENAME", &texture))
        return;
    float minsize = 0.0f;
    float maxsize = 0.0f;

    zrdr_findreal(entry, "MIN_SIZE", &minsize, 1);
    zrdr_findreal(entry, "MAX_SIZE", &maxsize, 1);
    CTexHandle *tex = CWorld::GetTexHandle(texture);

    Decal(this, index).m_decalminsize = minsize;
    Decal(this, index).m_decalmaxsize = maxsize;
    Decal(this, index).m_texture = tex;
    if (!strcmp(material, "PERSON"))
        CWorld::m_world->AddTextureAssetCharacterAll(tex);
    else
        CWorld::m_world->AddTextureAssetDecals(tex);
}
