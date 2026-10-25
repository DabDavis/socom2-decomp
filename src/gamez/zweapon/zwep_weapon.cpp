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
#include "../zsnd/zsnd.h"

using namespace zdb;

/* the decal sets and the bullet marks: this unit's classes alone (seal_env.cpp, which includes
 * zweapon.h through zseal.h, has a CZDecalDef of its own) */
namespace zdb {
class CTexHandle;
}
struct _zrdr;
void *_malloc(unsigned int size, const char *file, int line);
void _free(void *p, const char *file, int line);
char *_strdup(const char *s, const char *file, int line);

/* one material's decal: its size range and texture (SOCOM 1's DWARF: 0xc bytes) */
class CZDecalDef {
public:
    /* empty and user-declared: the vector's insert (0x3D3D90) then copies an element member by
     * member (lwc1/swc1, lwc1/swc1, lw/sw) where an implicit one is copied as a block */
    CZDecalDef() {}

    float m_decalminsize;   // 0x0
    float m_decalmaxsize;   // 0x4
    zdb::CTexHandle *m_texture;   // 0x8
};

/* a named set of decals, one per material (SOCOM 1's DWARF: 0xc bytes -- the decal definitions, the
 * name, a byte; zwep_weapon.cpp's). The sets are SOCOM 1's static vector m_decalsets (0x4B5290
 * here) */
class CZDecalSet {
public:
    /* a definition for every material, cleared, and the sets' room reserved (inline: ReadDecalSet's
     * new builds it in place -- SOCOM 1's alike) */
    CZDecalSet()
    {
        int n = CMaterialExArray::m_materialex_list.size();

        m_decals = (CZDecalDef *)_malloc(n * sizeof(CZDecalDef), 0, 0);
        m_name = 0;
        for (int i = 0; i < n; i++) {
            CZDecalDef *d = &m_decals[i];

            d->m_decalminsize = 0.0f;
            d->m_decalmaxsize = 0.0f;
            d->m_texture = 0;
        }
        m_decalsets.reserve(128);
        m_permenant = 0;
    }
    /* the name copied in (inline: the by-value name is loaded ahead of the old one's free, kept in a
     * saved register; the name ours) */
    void SetName(const char *name)
    {
        if (m_name)
            _free(m_name, 0, 0);
        m_name = 0;
        m_name = _strdup(name, 0, 0);
    }
    /* an entry of a set's rdr: its name, its permanence or a material's decal (SOCOM 1's) */
    void ReadDecalEntry(_zrdr *entry);
    /* a set read from its rdr and listed (SOCOM 1's) */
    static void ReadDecalSet(_zrdr *rdr);
    static CZDecalSet *GetDecalSetByName(const char *name);
    /* every set freed and the vector's storage given up; false when there were none */
    static bool DestroyDecalSet();

    CZDecalDef *m_decals;   // 0x0
    char *m_name;   // 0x4
    unsigned char m_permenant;   // 0x8

    static std::vector<CZDecalSet *> m_decalsets;
};   // 0xc bytes

namespace zdb {
class CNode;
class CTexHandle;
}
struct _zrdr;

/* the bullet marks and the decals they are drawn from (SOCOM 1's DWARF: 0x34 bytes, as SOCOM II's:
 * the free marks' nodes, the decal textures and their size ranges by index, the blood decals near and
 * far; CZWeapon's static m_pMarklist points at one; zwep_weapon.cpp's) */
class CZMarkList {
public:
    /* the tables cleared after the members are built (inline: CreateMarkList's new builds it in
     * place, the list's constructor called out of line, the two vectors' words zeroed -- SOCOM 1's
     * order) */
    CZMarkList()
    {
        m_decalTexture = 0;
        m_decalminsize = 0;
        m_decalmaxsize = 0;
    }
    /* the tables freed (inline: DestroyMarkList's delete inlines it, the members' destructors
     * called out of line -- the vectors' through an earlier unit's copy, 0x2CDB30) */
    ~CZMarkList()
    {
        if (m_decalTexture)
            _free(m_decalTexture, 0, 0);
        m_decalTexture = 0;
        if (m_decalminsize)
            _free(m_decalminsize, 0, 0);
        m_decalminsize = 0;
        if (m_decalmaxsize)
            _free(m_decalmaxsize, 0, 0);
        m_decalmaxsize = 0;
    }
    /* forty "bullet_mark" instances made and added to the world, then the decal names read */
    void Init();
    void UnInit();
    /* the decal tables sized to the materials and defaulted, the decal pools sized from the decals
     * rdr (the global one: the file named is not read), then the weapon and blood decals read; false
     * without the rdr */
    bool ReadDecalNames(char *file);
    void ReadWeaponDecals(_zrdr *rdr);
    void ReadBloodDecals(_zrdr *rdr);

    std::list<zdb::CNode *> m_marklist;   // 0x0
    zdb::CTexHandle **m_decalTexture;   // 0xc
    std::vector<CZDecalDef> m_bloodDecals;   // 0x10
    std::vector<CZDecalDef> m_farBloodDecals;   // 0x1c
    float *m_decalminsize;   // 0x28
    float *m_decalmaxsize;   // 0x2c
    int m_decal_list_size;   // 0x30
};   // 0x34 bytes

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

/* the projectiles in flight made for a count (SOCOM 1's) */
bool CZWeapon::CreateProjectileList(int count)
{
    m_pProjectilelist = new CZProjectileList(count);
    if (!m_pProjectilelist)
        return false;
    return true;
}

bool CZWeapon::DestroyProjectileList()
{
    if (!m_pProjectilelist)
        return false;
    delete m_pProjectilelist;
    m_pProjectilelist = 0;
    return true;
}

/* the bullet marks made (SOCOM 1's) */
bool CZWeapon::CreateMarkList()
{
    m_pMarklist = new CZMarkList;
    if (!m_pMarklist)
        return false;
    return true;
}

/* the bullet marks deleted: its tables freed, its vectors and list destroyed (SOCOM 1's). Its
 * delete is the unit's own use of the blood decals' vector destructor, which makes the template
 * tail generate the deleter's destructor and clear ahead of the vector's insert (0x3D3D90), so that
 * the insert inlines its temporary's destruction */
bool CZWeapon::DestroyMarkList()
{
    if (!m_pMarklist)
        return false;
    delete m_pMarklist;
    m_pMarklist = 0;
    return true;
}

bool CZWeapon::CreateWeaponList()
{
    return true;
}

/* the sound listener: the app camera's camera node (0x48DB48, set where the mission opens) */
extern "C" CNode *data_0048DB48;

/* the fire sound of the listener's distance played at the point: past the medium distance the
 * far one, past the close distance the medium one, else the close one (new in SOCOM II) */
void CZWeapon::PlayFireSound(const CPnt3D *pos)
{
    CPnt3D d = data_0048DB48->GetPosition();

    d = d - *pos;
    float d2 = Dot(d, d);
    CSnd *snd;

    if (d2 > m_sounddistance[2])
        snd = m_firesound[2];
    else if (d2 > m_sounddistance[1])
        snd = m_firesound[1];
    else
        snd = m_firesound[0];
    if (snd)
        snd->PlaySound3D(pos, 1.0f, 0, 0, 255);
}

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

/* the ammo's name among the legal ammo's (SOCOM 1's) */
bool CZWeapon::IsLegalAmmo(CZAmmo *ammo)
{
    CZAmmoList::iterator it;

    if (!ammo)
        return false;
    it = m_LegalAmmoList.begin();
    for (CZAmmoList::iterator e = m_LegalAmmoList.end(); it != e; ++it) {
        if (!strcmp(ammo->GetName(), ((CZAmmo *)*it.i_)->GetName()))
            return true;
    }
    return false;
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

void CZWeapon::SetTimer(int i, float t)
{
    m_timer[i] = t;
}

/* the mission's timer for the item 152 ("never" when it has none), the entry's otherwise (the
 * mission's word at 0x1864 -- not named) */
extern "C" float data_00437D44;

float CZWeapon::GetTimer(int i)
{
    float t;

    if (m_ID == 152) {
        t = data_00437D44;
        if (t < 0.0f)
            t = 9999999.0f;
    } else
        t = m_timer[i];
    return t;
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

/* each entry a blood decal: its texture and size range pushed, the texture kept with the decals'.
 * The push's vector<CZDecalDef> insert (0x3D3D90, this unit's template tail) is OPEN, 16 bytes
 * short: include/vector's class insert (the in-place branch's end pointer register, open for the
 * other class inserts too) and the temporary's destructor, which the target inlines and this tail
 * calls -- the tail's generation order */
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

/* the AI's range read from the weapon entry's "ai_params" (new in SOCOM II) */
void CZWeapon::AI_PARAMS::Read(_zrdr *rdr)
{
    if (!rdr)
        return;
    SetCustom(true);
    float range[2];

    if (zrdr_findreal(rdr, "optimal_range", range, 2) && range[0] < range[1]) {
        m_min_range = range[0];
        m_max_range = range[1];
    }
    if (zrdr_findtag(rdr, "hard_min"))
        SetHardMin(true);
}

float CZWeapon::AI_PARAMS::GetAccuracy(float d) const
{
    float r = 1.0f;

    if (d <= 0.0f)
        r = 0.0f;
    else if (d < m_min_range) {
        if (m_hard_min)
            r = 0.0f;
        else
            r = d / m_min_range;
    } else if (d <= m_max_range)
        r = 1.0f;
    else {
        d -= m_max_range;
        if (!(d < m_max_range))
            r = 0.0f;
    }
    return r;
}

void CZWeapon::SetFireSound(unsigned char i, CSnd *snd)
{
    m_firesound[i] = snd;
}

void CZWeapon::SetFireSoundName(unsigned char i, const char *name)
{
    if (m_firesoundname[i]) {
        if (m_firesoundname[i])
            _free(m_firesoundname[i], 0, 0);
        m_firesoundname[i] = 0;
    }
    if (name)
        m_firesoundname[i] = _strdup(name, 0, 0);
}

char *CZWeapon::GetFireSoundName(unsigned char i)
{
    return m_firesoundname[i];
}

float CZWeapon::m_sounddistance[3];

void CZWeapon::SetSoundDistance(unsigned char i, float d)
{
    m_sounddistance[i] = d * d;
}

/* CZItemStash (0x3CFB20-0x3CFF7F, its rows named): a 0x28-byte object of three parallel lists of
 * words (0, 0xC, 0x18) and a word (0x24) -- an entry is its three words at one index, live while its
 * third is non-zero. The kit and the pickups use it (the class and its names ours) */
class CZItemStash {
public:
    int Count();
    void Add(int a, int b, int c);
    void Set(int i, int a, int b, int c);
    bool Get(int i, int *a, int *b, int *c);
    void Clear(int i);
    void Compact();

    std::vector<int> m_a;   // 0x0
    std::vector<int> m_b;   // 0xc
    std::vector<int> m_c;   // 0x18
    int m_24;   // 0x24
};

/* an entry's three words appended */
void CZItemStash::Add(int a, int b, int c)
{
    m_a.push_back(a);
    m_b.push_back(b);
    m_c.push_back(c);
}

/* the live entries: the third list's non-zero words. The postfix step is the target's: with it MW
 * hoists the whole end into a preheader after the loop (entered with a `b`, the entry test repeated
 * there, the data's load shared with begin's); `++i` re-reads the data every pass, 0x18 short */
int CZItemStash::Count()
{
    int n = 0;

    for (std::vector<int>::iterator i = m_c.begin(); i != m_c.end(); i++)
        if (*i)
            n++;
    return n;
}

/* entry i's three words set when enough entries are live (`if (i > Count()) return;` branches
 * straight to the epilogue on `slt at`, as the target; the if-block form keeps a join) */
void CZItemStash::Set(int i, int a, int b, int c)
{
    if (i > Count())
        return;
    m_a[i] = a;
    m_b[i] = b;
    m_c[i] = c;
}

/* entry i's three words out; false when fewer entries are live */
bool CZItemStash::Get(int i, int *a, int *b, int *c)
{
    if (Count() < i)
        return false;
    *a = m_a[i];
    *b = m_b[i];
    *c = m_c[i];
    return true;
}

/* entry i's three words cleared when enough entries are live */
void CZItemStash::Clear(int i)
{
    if (i > Count())
        return;
    m_c[i] = 0;
    m_b[i] = 0;
    m_a[i] = 0;
}

/* every entry whose third word is zero erased from all three lists */
void CZItemStash::Compact()
{
    std::vector<int>::iterator a = m_a.begin();
    std::vector<int>::iterator b = m_b.begin();
    std::vector<int>::iterator c = m_c.begin();

    while (a != m_a.end()) {
        if (*c == 0) {
            a = m_a.erase(a);
            b = m_b.erase(b);
            c = m_c.erase(c);
        } else {
            ++c;
            ++a;
            ++b;
        }
    }
}
