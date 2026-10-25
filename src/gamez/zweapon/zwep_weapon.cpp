/* zwep_weapon.cpp -- CZWeapon (SOCOM 1's zwep_weapon.cpp, by its static initialiser and the press
 * beta's asserts): OVL1 0x3CF1F0?-0x3D425F, after ZWepReader and before zwep_ammo.cpp's CZAmmo. SOCOM
 * 1's unit also holds CZDecalSet and CZMarkList. Only some of its functions so far; the rest stay
 * generated drafts or placeholders. Defined in reverse of their image order (-inline deferred). */

#include <string.h>

#include "zweapon.h"
#include "../znode/znode.h"
#include "../zrdr/zrdr.h"
#include "../zsys/zsys.h"
#include "../zbody/zbody.h"

using namespace zdb;

namespace zdb {
class CDecal;
class CMeshDecal;

/* vis_decal.cpp's decal pools, as that unit lays them out (stand-ins: only what UnInit touches) */
class CDecalPool {
public:
    void Init(unsigned int count, unsigned int extra);

    unsigned int m_count;   // 0x0
    unsigned int m_max;   // 0x4
    unsigned int m_extra;   // 0x8
    unsigned int m_overflow;   // 0xc
    RTArray<CDecal> *m_pool;   // 0x10
};

class CMeshDecalPool {
public:
    void Init(unsigned int count, unsigned int extra);
    void Uninit();

    unsigned int m_count;   // 0x0
    unsigned int m_max;   // 0x4
    unsigned int m_extra;   // 0x8
    unsigned int m_overflow;   // 0xc
    RTArray<CMeshDecal> *m_pool;   // 0x10
};

/* a decal: its destructor empty and inline (its out-of-line copy 0x3BF690 is what the pool's
 * array destroys its items with), its two pools SOCOM 1's statics */
class CDecal {
public:
    ~CDecal() {}

    static CDecalPool m_tempPool;
    static CDecalPool m_permPool;
};

class CMeshDecal {
public:
    static CMeshDecalPool m_tempPool;
    static CMeshDecalPool m_permPool;
};
}

extern "C" int strcasecmp(const char *a, const char *b);
void _free(void *p, const char *file, int line);
char *_strdup(const char *s, const char *file, int line);
/* the decals rdr (0x437D00) and a material's index by name (0x2DE9E0) -- not named */
extern "C" _zrdr *data_00437D00;
extern "C" int func_002DE9E0(const char *name);
/* the next material of a name after an index, 0 past the last (0x2DE970) -- not named */
extern "C" int func_002DE970(const char *name, int index);

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

/* the marks let go of, the decal tables freed, both decal pools' arrays deleted and the mesh decal
 * pools emptied (SOCOM 1's, the file and line dropped) */
void CZMarkList::UnInit()
{
    m_marklist.clear();
    if (m_decalTexture)
        _free(m_decalTexture, 0, 0);
    m_decalTexture = 0;
    if (m_decalminsize)
        _free(m_decalminsize, 0, 0);
    m_decalminsize = 0;
    if (m_decalmaxsize)
        _free(m_decalmaxsize, 0, 0);
    m_decalmaxsize = 0;
    m_decal_list_size = 0;
    delete CDecal::m_tempPool.m_pool;
    CDecal::m_tempPool.m_count = 0;
    CDecal::m_tempPool.m_overflow = 0;
    delete CDecal::m_permPool.m_pool;
    CDecal::m_permPool.m_count = 0;
    CDecal::m_permPool.m_overflow = 0;
    CMeshDecal::m_tempPool.Uninit();
    CMeshDecal::m_permPool.Uninit();
}

void CZMarkList::Init()
{
    CModel *model = (CModel *)CWorld::GetModel("bullet_mark");

    if (model) {
        for (int i = 0; i < 40; i++) {
            CPnt3D pos = CPnt3D::zero;
            CQuat rot = CQuat::identity;
            CNode *node = CNode::CreateInstance(model, &pos, &rot);

            CWorld::m_world->AddChild(node);
            m_marklist.push_back(node);
        }
    }
    ReadDecalNames("decals.rdr");
}

bool CZMarkList::ReadDecalNames(char *file)
{
    m_decal_list_size = CMaterialExArray::m_materialex_list.size();
    if (m_decal_list_size > 0) {
        m_decalTexture = (CTexHandle **)_malloc(m_decal_list_size * sizeof(CTexHandle *), 0, 0);
        m_decalminsize = (float *)_malloc(m_decal_list_size * sizeof(float), 0, 0);
        m_decalmaxsize = (float *)_malloc(m_decal_list_size * sizeof(float), 0, 0);
        for (int i = 0; i < m_decal_list_size; i++) {
            m_decalTexture[i] = 0;
            m_decalminsize[i] = 0.3f;
            m_decalmaxsize[i] = 1.8f;
        }
    }
    _zrdr *rdr = data_00437D00;

    if (!rdr)
        return false;
    int base = 300;
    int overflow = 50;
    _zrdr *pool;

    if ((pool = zrdr_findtag(rdr, "TEMP_DECAL_POOL")) != 0) {
        zrdr_findint(pool, "BASE", &base, 1);
        zrdr_findint(pool, "OVERFLOW", &overflow, 1);
    }
    CDecal::m_tempPool.Init(base, overflow);
    CMeshDecal::m_tempPool.Init(base + 50, overflow);
    base = 1;
    overflow = 1;
    if ((pool = zrdr_findtag(rdr, "PERM_DECAL_POOL")) != 0) {
        zrdr_findint(pool, "BASE", &base, 1);
        zrdr_findint(pool, "OVERFLOW", &overflow, 1);
    }
    CDecal::m_permPool.Init(base, overflow);
    CMeshDecal::m_permPool.Init(base, overflow);
    ReadWeaponDecals(rdr);
    ReadBloodDecals(rdr);
    return true;
}

/* each entry's material's (every material of the name) decal texture and size range (the names
 * cleared before the materials are walked -- the walk starts from no name, as the target's) */
void CZMarkList::ReadWeaponDecals(_zrdr *rdr)
{
    _zrdr *list = zrdr_findtag(rdr, "WEAPON_DECALS");

    if (!list)
        return;
    const char *texture = 0;
    const char *material = 0;
    int n = list->Count();

    for (int i = 0; i < n; i++) {
        _zrdr *entry = list->Item(i);

        if (!zrdr_findSTRING(entry, "MATERIALNAME", &material))
            continue;
        float minsize = 0.3f;
        float maxsize = 1.8f;

        if (!zrdr_findSTRING(entry, "TEXTURENAME", &texture))
            continue;
        zrdr_findreal(entry, "MIN_SIZE", &minsize, 1);
        zrdr_findreal(entry, "MAX_SIZE", &maxsize, 1);
        CTexHandle *tex = CWorld::GetTexHandle(texture);

        texture = 0;
        material = 0;
        for (int index = func_002DE970(material, 0); index; index = func_002DE970(material, index)) {
            m_decalminsize[index] = minsize;
            m_decalmaxsize[index] = maxsize;
            m_decalTexture[index] = tex;
        }
    }
}

/* each entry a blood decal: its texture and size range pushed, the texture kept with the decals' */
void CZMarkList::ReadBloodDecals(_zrdr *rdr)
{
    _zrdr *list = zrdr_findtag(rdr, "BLOOD_DECALS");

    if (!list)
        return;
    const char *texture = 0;
    CZDecalDef def;

    def.m_decalminsize = 0.0f;
    def.m_decalmaxsize = 0.0f;
    def.m_texture = 0;
    int n = list->Count();

    for (int i = 0; i < n; i++) {
        _zrdr *entry = list->Item(i);

        if (!zrdr_findSTRING(entry, "TEXTURENAME", &texture))
            continue;
        float minsize = 0.0f;
        float maxsize = 0.0f;

        zrdr_findreal(entry, "MIN_SIZE", &minsize, 1);
        zrdr_findreal(entry, "MAX_SIZE", &maxsize, 1);
        CTexHandle *tex = CWorld::GetTexHandle(texture);

        def.m_decalminsize = minsize;
        def.m_decalmaxsize = maxsize;
        def.m_texture = tex;
        m_bloodDecals.push_back(def);
        CWorld::m_world->AddTextureAssetDecals(tex);
    }
}
