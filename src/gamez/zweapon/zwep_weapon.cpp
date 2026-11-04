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

/* a valve (zvalve's: the lookup by name only) */
class CValve {
public:
    static CValve *GetByName(const char *name);
};

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

/* the unit's statics, in the initialiser's order (__sinit_zwep_weapon.cpp, 0x404C10: the weapon
 * list, the placed charges and the tracers through their constructors, the decal sets' three words
 * zeroed -- each destructor registered after it; SOCOM 1's builds the same but the charges) */
CZWeaponList CZWeapon::m_listWeapons;
CPlacedExplosiveList CZWeapon::m_placedExplosives;
CFlyoutMachine CZWeapon::m_flyout_machine;
std::vector<CZDecalSet *> CZDecalSet::m_decalsets;

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
    /* a hit's mark: a decal of the hit material's (or blood) applied to the node hit (SOCOM 1's) */
    void AddNewMark(zdb::IntersectStruct *is, CZProjectile *proj, float size);

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
namespace zdb {
/* node_world.h's hit record (a stand-in: that header's decal pools clash with this unit's) */
struct IntersectStruct {
    CPnt3D m_pos;   // 0x0
    CDI *m_poly;   // 0xc
    CPnt3D m_norm;   // 0x10
    CNode *m_node;   // 0x1c

    int GetMaterial();
};
}


/* everything cleared, one fire mode, a tenth of a second between shots, no impact radius (SOCOM
 * 1's, with SOCOM II's members) */
CZWeapon::CZWeapon()
{
    m_name = 0;
    m_description = 0;
    m_socom2_c = 0;
    m_maxfiremode = (FIREMODE)0;
    m_ammocap = 0;
    m_soundradius = 0;
    m_soundradiusSq = 0;
    m_encumb = LIGHTLY_ENCUMBERED;
    m_effectiverange = 0;
    m_maxrange = 0;
    m_texname = 0;
    m_icontexname = 0;
    m_associatedgearname = 0;
    m_modelname = 0;
    m_icon = 0;
    m_ID = (EQUIP_ITEM)0;
    m_nummags = 0;
    m_specialmaterialanimname = 0;
    m_defaultspecialanimname = 0;
    m_fireanimname = 0;
    m_hitanimname = 0;
    m_hitanim = 0;
    m_specialmaterialanim = 0;
    m_defaultspecialanim = 0;
    m_zoomedfireanimTT = 0;
    m_zoomedfireanim = 0;
    m_fireanim = 0;
    m_texture = 0;
    m_firewait = 0.1f;
    m_decalsetname = 0;
    m_decalSet = 0;
    m_muzzlevel = 0;
    m_impactradius = -1.0f;
    m_socom2_4c = 0;
    m_reloadsoundname = 0;
    m_socom2_a4 = 0;
    m_socom2_a0 = 0;
    m_reloadsound = 0;
    m_reloadtime = 0;
    m_reloadAfterShot = 0;
    m_hasFireModes[0] = 1;
    m_hasFireModes[1] = 0;
    m_hasFireModes[2] = 0;
    m_hasFireModes[3] = 0;
    m_hasFireModes[4] = 0;
    for (int i = 0; i < 3; i++) {
        m_firesound[i] = 0;
        m_firesoundname[i] = 0;
    }
    m_damagebonus = 0;
    m_socom2_68 = 0;
    m_socom2_d5 = 0;
    m_socom2_6c = 0;
}

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

/* the passing rounds' sounds (0x3D37C0-0x3D379F: the getters after the setters in the image, the
 * rockets' first of each pair -- the getters' caller is OVL2's CSealCtrl::OnWeaponEvent) */
CSnd *CZWeapon::m_bulletpassSound;
CSnd *CZWeapon::m_rocketpassSound;

CSnd *CZWeapon::GetBulletPassSound()
{
    return m_bulletpassSound;
}

CSnd *CZWeapon::GetRocketPassSound()
{
    return m_rocketpassSound;
}

void CZWeapon::SetBulletPassSound(CSnd *snd)
{
    m_bulletpassSound = snd;
}

void CZWeapon::SetRocketPassSound(CSnd *snd)
{
    m_rocketpassSound = snd;
}

/* an item's place in the weapon menus -- what the UiWeapon valve holds (zanim_menu.cpp asks it of
 * every item to find the one the valve names), -1 for none (SOCOM II's, 0x3D3230: the switch as
 * m2c's draft read it; the name ours) */
int CZWeapon::GetMenuValue(EQUIP_ITEM item)
{
    switch (item) {
    case 51:
        return 1;
    case 33:
        return 2;
    case 31:
        return 3;
    case 58:
        return 4;
    case 59:
        return 5;
    case 60:
        return 7;
    case 57:
        return 8;
    case 37:
        return 9;
    case 102:
        return 10;
    case 91:
        return 11;
    case 92:
        return 12;
    case 106:
        return 13;
    case 105:
        return 14;
    case 101:
        return 15;
    case 103:
        return 17;
    case 54:
        return 18;
    case 61:
        return 19;
    case 62:
        return 20;
    case 143:
        return 21;
    case 142:
        return 22;
    case 34:
        return 23;
    case 52:
        return 24;
    case 83:
        return 25;
    case 81:
        return 26;
    case 84:
        return 27;
    case 68:
        return 28;
    case 65:
        return 29;
    case 64:
        return 30;
    case 66:
        return 31;
    case 38:
        return 32;
    case 63:
        return 33;
    case 104:
        return 34;
    case 67:
        return 65;
    case 15:
        return 75;
    case 16:
        return 76;
    case 6:
        return 77;
    case 5:
        return 81;
    case 14:
        return 79;
    case 4:
        return 80;
    case 8:
        return 78;
    case 7:
        return 82;
    case 11:
        return 83;
    case 12:
        return 84;
    case 13:
        return 85;
    case 194:
        return 35;
    case 121:
        return 36;
    case 126:
        return 37;
    case 122:
        return 49;
    case 123:
        return 39;
    case 152:
        return 54;
    case 153:
        return 41;
    case 151:
        return 42;
    case 171:
        return 43;
    case 173:
        return 44;
    case 175:
        return 45;
    case 179:
        return 46;
    case 178:
        return 47;
    case 176:
        return 48;
    case 158:
        return 38;
    case 145:
        return 50;
    case 146:
        return 51;
    case 195:
        return 52;
    case 127:
        return 53;
    case 186:
        return 40;
    case 181:
        return 55;
    case 182:
        return 56;
    case 183:
        return 57;
    case 128:
        return 58;
    case 141:
        return 252;
    case 185:
        return 254;
    case 254:
        return 255;
    case 255:
        return 0;
    default:
        return -1;
    }
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

char *CZWeapon::GetIconTextureName() const
{
    return m_icontexname;
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

/* a table of an animation per material (SOCOM 1's; its true is the table's own test) */
bool CZWeapon::AllocateSpecialMaterialAnimMem()
{
    m_specialmaterialanim = (CZAnim **)_malloc(CMaterialExArray::m_materialex_list.size() * sizeof(CZAnim *), 0, 0);
    return m_specialmaterialanim != 0;
}

bool CZWeapon::AllocateHitAnimMem()
{
    m_hitanim = (CZAnim **)_malloc(CMaterialExArray::m_materialex_list.size() * sizeof(CZAnim *), 0, 0);
    return m_hitanim != 0;
}

char *CZWeapon::GetHitAnimName()
{
    return m_hitanimname;
}

char *CZWeapon::GetDefaultSpecialAnimName()
{
    return m_defaultspecialanimname;
}

char *CZWeapon::GetSpecialMaterialAnimName()
{
    return m_specialmaterialanimname;
}

bool CZWeapon::SetHitAnim(int material, CZAnim *anim)
{
    if (material < (int)CMaterialExArray::m_materialex_list.size()) {
        m_hitanim[material] = anim;
        return true;
    }
    return false;
}

void CZWeapon::SetFireAnim(CZAnim *anim)
{
    m_fireanim = anim;
}

void CZWeapon::SetDefaultSpecialAnim(CZAnim *anim)
{
    m_defaultspecialanim = anim;
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

char *CZWeapon::GetFireAnimName()
{
    return m_fireanimname;
}

CZAnim *CZWeapon::GetHitAnim(int material)
{
    if (material < (int)CMaterialExArray::m_materialex_list.size())
        return m_hitanim[material];
    return 0;
}

CZAnim *CZWeapon::GetDefaultSpecialAnim()
{
    return m_defaultspecialanim;
}

void CZWeapon::SetZoomedFireAnim(CZAnim *anim)
{
    m_zoomedfireanim = anim;
}

void CZWeapon::SetZoomedFireAnimTT(CZAnim *anim)
{
    m_zoomedfireanimTT = anim;
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

char *CZWeapon::GetDecalSetName()
{
    return m_decalsetname;
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

/* a primary class -- SMGs, assault rifles, shotguns, machine guns, heavy weapons, sniper rifles
 * (0x3D1DB0: out of line its true is `li`, inlined into IsPrimary an assignment, daddiu; the kit's
 * SetCurItemEnum/SetCurItemByIndex, the HUD's CAmmoDisplay::Update and the SEALs call it with a
 * class -- the name ours) */
bool CZWeapon::IsPrimaryType(EQUIP_ITEM type)
{
    if (type == 31 || type == 51 || type == 81 || type == 91 || type == 141 || type == 101)
        return true;
    return false;
}

/* the pistols' class, 4 (0x3D1DA0; IsSecondary inlines it -- the name ours) */
bool CZWeapon::IsSecondaryType(EQUIP_ITEM type)
{
    return type == 4;
}

/* the launchers' class, 145 ("LAUNCHERS" to GetIDTypeName; 0x3D1D90, which IsLauncher inlines --
 * the name ours) */
bool CZWeapon::IsLauncherType(EQUIP_ITEM type)
{
    return type == 145;
}

bool CZWeapon::IsPrimary(EQUIP_ITEM id)
{
    return IsPrimaryType(GetIDType(id));
}

bool CZWeapon::IsSecondary(EQUIP_ITEM id)
{
    return IsSecondaryType(GetIDType(id));
}

bool CZWeapon::IsLauncher(EQUIP_ITEM id)
{
    return IsLauncherType(GetIDType(id));
}

/* OPEN, 2 instructions short (SOCOM 1's is byte-identical): the target zero-extends the halfword's
 * 32 bits to 64 (dsll32, dsrl32) before storing the ammo byte, and shifts the item out with srl;
 * masks, a modulo and casts through unsigned long all fold it away, an unsigned int copy gives the
 * srl alone, and a union of two 8-bit bitfields (unsigned int or unsigned short) is 4 bytes long.
 * Round 16: `*item = packed / 256u; *ammo = packed % 256ul;` is one instruction off -- the
 * int-promoted halfword is SIGN-extended to the unsigned long (dsll32/dsra32) and kept, where
 * every unsigned int intermediate (casts, `/ 1u`, `| 0u`, `+ 0u`, `& 0xFFFFu`, `(unsigned)(int)`)
 * lets MW fold the zero-extension away; `% 256L`, `& 0xFFL`, `& 0xFFFFFFFFUL` miss by more. Round
 * 21: bit-field unions (unsigned int or unsigned long words, as locals) and a bit-field view
 * through a pointer go through memory (0x20-0x24); an unsigned long or unsigned long long local, a
 * `w : 32` field, `% 0x100000000ul` and a subtracted product fold or grow; an int, unsigned short
 * or int-cast local with `% 256ul` is the same 1 off (dsra32) */
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

/* each list entry of the decals rdr a set (SOCOM 1's OpenDecals body; the rdr is a global the mark
 * list's ReadDecalNames and OVL2 share, 0x437D00 -- not named). SOCOM II's takes the file's name and
 * never reads it (a0 is overwritten before any use). Pinned: OpenDecals() calls it out of line, a
 * tail call -- likely compiled-out code counted in the original (compiled-out statements still count
 * toward the inline limit) */
#pragma auto_inline off
void CZWeapon::OpenDecals(const char *)
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
#pragma auto_inline reset

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
    int index = CMaterialExArray::ResolveMaterialIndex(material);
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

/* (the image's 0x3D0420 and Open, 0x3D03D0, sit here: generated drafts still) */

/* the decal sets (SOCOM 1's OpenDecals, CMission::Open's call): the overload above, given the file's
 * name, called out of line (a tail call) */
void CZWeapon::OpenDecals()
{
    OpenDecals("decals.rdr");
}

/* an item's name: the weapon list's, or "falling damage" for the fall's id 253 (SOCOM 1's, which
 * had no fall) */
char *CZWeapon::GetWeaponName(EQUIP_ITEM id)
{
    if (id == 253)
        return "falling damage";
    return m_listWeapons.GetWeaponName(id);
}

/* a class's display name, "UNKNOWN" for none (new in SOCOM II; the name ours). The case tests run
 * in reverse source order, 254 and 255 first */
const char *CZWeapon::GetIDTypeName(EQUIP_ITEM type)
{
    switch (type) {
    case 4:
        return "PISTOLS";
    case 31:
        return "SMGS";
    case 51:
        return "ASSAULT RIFLES";
    case 81:
        return "SHOTGUNS";
    case 91:
        return "MACHINE GUNS";
    case 101:
        return "SNIPER RIFLES";
    case 121:
        return "GRENADES";
    case 141:
        return "HEAVY WEAPONS";
    case 145:
        return "LAUNCHERS";
    case 151:
        return "EXPLOSIVES";
    case 171:
        return "LAUNCHED";
    case 185:
        return "MISSILE";
    case 190:
        return "MISC EQUIP";
    case 201:
        return "ARMOR";
    case 205:
        return "TURRETED WEAPONS";
    case 230:
        return "TURRETED LAUNCHERS";
    case 255:
    case 254:
    default:
        return "UNKNOWN";
    }
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
        for (int index = CMaterialExArray::ResolveDecalMaterialIndex(material, 0); index;
             index = CMaterialExArray::ResolveDecalMaterialIndex(material, index)) {
            m_decalminsize[index] = minsize;
            m_decalmaxsize[index] = maxsize;
            m_decalTexture[index] = tex;
        }
    }
}

/* each entry a blood decal: its texture and size range pushed, the texture kept with the decals'.
 * The push's vector<CZDecalDef> insert (0x3D3D90, this unit's template tail) is byte-exact (the
 * MSL headers' class insert, with DestroyMarkList's use of the vector's destructor generating the
 * deleter's destructor and clear ahead of it) */
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

/* the node a decal hangs on: up the parents to one of type 2, none at the world (SOCOM 1's) */
CNode *GetDecalNode(CNode *node)
{
    if (node) {
        do {
            if (node->GetType() == 2)
                break;
            node = node->m_parent;
            if (node == CWorld::m_world)
                return 0;
        } while (node);
    }
    return node;
}

/* the node that draws: up the parents to one with visuals, or the world's child (SOCOM 1's) */
CNode *GetVisualNode(CNode *node)
{
    if (node) {
        do {
            if (node->m_hasVisuals)
                break;

            CNode *parent = node->m_parent;

            if (parent == CWorld::m_world)
                break;
            node = parent;
        } while (node);
    }
    return node;
}

/* a material's decal for a hit, by the shooter's weapon's set (a set's permanence turns its decals
 * over); a hit on a person, echoed, a blood decal near or far by the distance from the last player
 * hit -- at half size, and only where the person takes it (0x5ACA10). The decal is sized between
 * its range at random and faces the shot, or the surface for the weapons that throw (SOCOM 1's,
 * with SOCOM II's blood) */
extern "C" int func_005ACA10(CNode *node);

/* one of the decals at random, or the given one when there are none */
static inline CZDecalDef *RandomDecal(std::vector<CZDecalDef> &v, CZDecalDef *def)
{
    int n = v.size();

    if (n == 0)
        return def;
    return &v[rand() % n];
}

/* a size between two at random (rand()'s 31 bits scaled to one; the maximum first, as the target
 * loads it) */
static inline float RandRange(float hi, float lo)
{
    float r = (1.0f / 2147483648.0f) * (float)rand();

    return lo + r * (hi - lo);
}

static inline bool DecalFacesShot(CZWeapon *weapon)
{
    EQUIP_ITEM t = CZWeapon::GetIDType(weapon->m_ID);

    if (t == 171 || t == 185 || t == 121 || t == 151 || t == 190 || t == 230)
        return false;
    return true;
}

void CZMarkList::AddNewMark(IntersectStruct *is, CZProjectile *proj, float size)
{
    CNode *node = is->m_node;
    CNode *root = GetVisualNode(node);

    if (!root)
        return;
    if (proj->m_echo) {
        size *= 0.5f;
        if (!func_005ACA10(node))
            return;
    } else
        node = GetDecalNode(node);

    int mat = is->GetMaterial();
    CZDecalDef *def = 0;
    float depth = 100.0f;
    unsigned char permanent = 0;

    if (proj->m_echo && mat != CMaterialExArray::ResolveMaterialIndex("PERSON")) {
        CPnt3D d = proj->m_lastPlayerHitPos - is->m_pos;
        float dist = Dot(d, d);

        if (dist < 10000.0f)
            def = RandomDecal(m_bloodDecals, def);
        else if (dist < 40000.0f)
            def = RandomDecal(m_farBloodDecals, def);
    } else {
        int m = is->GetMaterial();

        if (m >= m_decal_list_size)
            return;
        if (proj->m_weapon && proj->m_weapon->m_decalSet) {
            CZDecalSet *set = proj->m_weapon->m_decalSet;

            permanent = set->m_permenant;
            def = &set->m_decals[m];
        }
    }
    if (def) {
        float s = RandRange(def->m_decalmaxsize, def->m_decalminsize);

        s *= size;
        CPnt3D normal = CPnt3D::zero;

        is->m_norm.Scale(-1.0f, &normal);
        if (permanent)
            depth = -1.0f;
        if (def->m_texture) {
            CPnt3D dir = proj->m_vel;

            if (proj->m_weapon && DecalFacesShot(proj->m_weapon))
                dir.Normalize();
            else
                dir = normal;
            root->ApplyDecal(node, depth, &is->m_pos, &normal, &dir, def->m_texture, s, 0);
        }
        return;
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

/* the second timer (new in SOCOM II; its statics 0x4B52D8-0x4B52E8 and 0x3E1558) */
float CZWeapon::m_timer2_maxval;
float *CZWeapon::m_timer2_pCurval;
bool CZWeapon::m_timer2_bTimerRegistered;
bool CZWeapon::m_timer2_bIncrTimer = true;

void CZWeapon::RegisterTimer2(float *pCurval, float maxval, bool bIncr)
{
    m_timer2_bIncrTimer = bIncr;
    m_timer2_maxval = maxval;
    m_timer2_pCurval = pCurval;
    m_timer2_bTimerRegistered = true;
}

void CZWeapon::RetrieveTimer2Registration(float **pCurval, float *maxval, bool *bIncr)
{
    *bIncr = m_timer2_bIncrTimer;
    *maxval = m_timer2_maxval;
    *pCurval = m_timer2_pCurval;
    m_timer2_bTimerRegistered = false;
}

bool CZWeapon::Timer2Registered()
{
    return m_timer2_bTimerRegistered;
}

/* the HUD's timer (SOCOM 1's statics, 0x4B52B8-0x4B52D0 and 0x3E1550) */
float CZWeapon::m_timer_maxval;
float *CZWeapon::m_timer_pCurval;
bool CZWeapon::m_timer_bTimerRegistered;
bool CZWeapon::m_timer_bTimerCleared;
bool CZWeapon::m_timer_bIncrTimer = true;
bool CZWeapon::m_allowFireAnimations = true;
bool CZWeapon::m_allowImpactAnimations = true;
bool CZWeapon::m_allowWeaponIntersection = true;

void CZWeapon::RegisterTimer(float *pCurval, float maxval, bool bIncr)
{
    m_timer_maxval = maxval;
    m_timer_pCurval = pCurval;
    m_timer_bTimerRegistered = true;
    m_timer_bIncrTimer = bIncr;
    m_timer_bTimerCleared = false;
}

void CZWeapon::RetrieveTimerRegistration(float **pCurval, float *maxval, bool *bIncr)
{
    *maxval = m_timer_maxval;
    *bIncr = m_timer_bIncrTimer;
    *pCurval = m_timer_pCurval;
    m_timer_bTimerRegistered = false;
}

bool CZWeapon::TimerRegistered()
{
    return m_timer_bTimerRegistered;
}

/* the timer let go of when it counts that value, or whatever it counts (SOCOM 1's took no value) */
void CZWeapon::ClearTimer(float *pCurval)
{
    if (!pCurval || pCurval == m_timer_pCurval) {
        m_timer_pCurval = 0;
        m_timer_bTimerRegistered = false;
        m_timer_bTimerCleared = true;
        m_timer_maxval = -1.0f;
        m_timer_bIncrTimer = true;
    }
}

bool CZWeapon::TimerCleared()
{
    return m_timer_bTimerCleared;
}

void CZWeapon::RemoveExplosivesInRange(const CPnt3D *pos, float range)
{
    m_pProjectilelist->RemoveExplosivesInRange(pos, range);
}

/* CZItemStash (0x3CFB20-0x3CFFAF, its rows named): a 0x28-byte object of three parallel lists of
 * words (0, 0xC, 0x18) and a word (0x24) -- an entry is its three words at one index, live while its
 * third is non-zero. The kit and the pickups use it (the class and its names ours) */
class CZItemStash {
public:
    CZItemStash();
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

/* the three lists empty (0x3CFF80: the kit and the pickups build theirs with it) */
CZItemStash::CZItemStash()
{
    m_24 = 0;
}

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

/* the menus' valve for an item -- "Enable_" and its name -- none for any other (SOCOM II's, 0x3CF1F0:
 * the unit's first function in the image, so defined last; the cases in the target's block order,
 * its compare chain their reverse) */
CValve *CZWeapon::GetEnableValve(EQUIP_ITEM item)
{
    CValve *v = 0;

    switch (item) {
    case 34:
        v = CValve::GetByName("Enable_FNP");
        break;
    case 51:
        v = CValve::GetByName("Enable_M16");
        break;
    case 61:
        v = CValve::GetByName("Enable_m4Acarbine_203");
        break;
    case 52:
        v = CValve::GetByName("Enable_M16203");
        break;
    case 33:
        v = CValve::GetByName("Enable_mp5sd");
        break;
    case 31:
        v = CValve::GetByName("Enable_mp5");
        break;
    case 58:
        v = CValve::GetByName("Enable_ak47");
        break;
    case 59:
        v = CValve::GetByName("Enable_aks74");
        break;
    case 83:
        v = CValve::GetByName("Enable_jack");
        break;
    case 81:
        v = CValve::GetByName("Enable_spas");
        break;
    case 60:
        v = CValve::GetByName("Enable_m14_gun");
        break;
    case 57:
        v = CValve::GetByName("Enablesig_commando");
        break;
    case 37:
        v = CValve::GetByName("Enable_uzi");
        break;
    case 102:
        v = CValve::GetByName("Enable_remington700");
        break;
    case 91:
        v = CValve::GetByName("Enable_m60e");
        break;
    case 92:
        v = CValve::GetByName("Enable_m63a");
        break;
    case 106:
        v = CValve::GetByName("Enable_stoner_sr25");
        break;
    case 101:
        v = CValve::GetByName("Enable_barret_lightm82A1");
        break;
    case 84:
        v = CValve::GetByName("Enable_870");
        break;
    case 103:
        v = CValve::GetByName("Enable_EquipmcmillanM87");
        break;
    case 54:
        v = CValve::GetByName("Enable_m4Acarbine");
        break;
    case 62:
        v = CValve::GetByName("Enable_M4A1_SD");
        break;
    case 105:
        v = CValve::GetByName("Enable_stoner_sr25_SD");
        break;
    case 68:
        v = CValve::GetByName("Enable_STEYR");
        break;
    case 142:
        v = CValve::GetByName("Enable_mglmk1");
        break;
    case 143:
        v = CValve::GetByName("Enable_m79");
        break;
    case 63:
        v = CValve::GetByName("Enable_F2000");
        break;
    case 67:
        v = CValve::GetByName("Enable_552SD");
        break;
    case 104:
        v = CValve::GetByName("Enable_Dragonov");
        break;
    case 64:
        v = CValve::GetByName("Enable_SA-80");
        break;
    case 65:
        v = CValve::GetByName("Enable_AK105");
        break;
    case 66:
        v = CValve::GetByName("Enable_OC14");
        break;
    case 15:
        v = CValve::GetByName("Enable_Mark23");
        break;
    case 16:
        v = CValve::GetByName("Enable_mark23sd");
        break;
    case 6:
        v = CValve::GetByName("Enable_Sig226");
        break;
    case 8:
        v = CValve::GetByName("Enable_HkP9s");
        break;
    case 14:
        v = CValve::GetByName("Enable_glock18");
        break;
    case 4:
        v = CValve::GetByName("Enable_fiveseven");
        break;
    case 5:
        v = CValve::GetByName("Enable_beretta_m9");
        break;
    case 7:
        v = CValve::GetByName("Enable_desert_eagle");
        break;
    case 13:
        v = CValve::GetByName("Enable_SR1_Gyurza");
        break;
    case 12:
        v = CValve::GetByName("Enable_P228");
        break;
    case 38:
        v = CValve::GetByName("Enable_MP5K");
        break;
    case 194:
        v = CValve::GetByName("Enable_2xammo");
        break;
    case 195:
        v = CValve::GetByName("Enable_ThermScope");
        break;
    case 121:
        v = CValve::GetByName("Enable_frag");
        break;
    case 122:
        v = CValve::GetByName("Enable_smoke");
        break;
    case 123:
        v = CValve::GetByName("Enable_flash");
        break;
    case 124:
        v = CValve::GetByName("Enable_phos");
        break;
    case 152:
        v = CValve::GetByName("Enable_satchel");
        break;
    case 153:
        v = CValve::GetByName("Enable_claymore");
        break;
    case 158:
        v = CValve::GetByName("Enable_pmn");
        break;
    case 151:
        v = CValve::GetByName("Enable_c4");
        break;
    case 171:
        v = CValve::GetByName("Enable_203HE");
        break;
    case 173:
        v = CValve::GetByName("Enable_203SMK");
        break;
    case 175:
        v = CValve::GetByName("Enable_203FRAG");
        break;
    case 172:
        v = CValve::GetByName("Enable_203ILL");
        break;
    case 126:
        v = CValve::GetByName("Enable_HEgren");
        break;
    case 179:
        v = CValve::GetByName("Enable_GLFRAG");
        break;
    case 178:
        v = CValve::GetByName("Enable_GLSMK");
        break;
    case 176:
        v = CValve::GetByName("Enable_GLHE");
        break;
    case 186:
    case 146:
        v = CValve::GetByName("Enable_RPG");
        break;
    case 145:
        v = CValve::GetByName("Enable_LAW");
        break;
    case 183:
        v = CValve::GetByName("Enable_F2000FRAG");
        break;
    case 182:
        v = CValve::GetByName("Enable_F2000SMK");
        break;
    case 181:
        v = CValve::GetByName("Enable_F2000HE");
        break;
    case 127:
        v = CValve::GetByName("Enable_red_smoke");
        break;
    }
    return v;
}
