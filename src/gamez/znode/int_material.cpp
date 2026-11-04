/* int_material.cpp -- the materials' extensions: the mission's soils read from its materials rdr into
 * a list of CMaterialEx, a name map to their indices, the two built-in materials (UNKNOWN and
 * PARTICLE_SYSTEM), the default copied over the first slot, and the list freed (OVL1
 * 0x2DDC30-0x2DEBF3, then its template tail to 0x2DF153: the string map's find_or_insert, find,
 * clear and insert_node_at, the string's two compares). SOCOM 1's file and functions (its symbol
 * table: CMaterialExArray's seven, CMaterialEx's constructor, the vector's destructor; no DWARF), in
 * SOCOM 1's order, after int_shapes.cpp; its initialiser builds the map and the list (0x401DB0, the
 * last of the zdb world's in the table). Defined in reverse of their image order (-inline deferred). */

#include "int_material.h"

/* the unit's statics, in the initialiser's order (__sinit_int_material.cpp, 0x401DB0: the map built
 * and ~map registered, the list zeroed and its destructor registered). OPEN only on two names: it
 * calls the map's constructor at 0x2CD670 and registers ~map at 0x2CE210, an earlier unit's copies
 * still unnamed (outside this lane's range) -- every other word is the target's */
int CMaterialExArray::m_defaultMaterial;
std::map<std::string, int> CMaterialExArray::m_materialex_map;
std::vector<CMaterialEx *> CMaterialExArray::m_materialex_list;

/* (0x2DEB30: ReadMaterials and CreateDefaultMaterials call it out of line) */
CMaterialEx::CMaterialEx()
{
    m_name = 0;
    m_decal = 0;
    m_opacity = 1.0f;
    m_penetration = 0.0f;
    m_ricochet = 0.0f;
    m_elasticity = 0.0f;
    m_stealth_factor = 0.5f;
    m_foot_step_offset = 0.0f;
    SetVolumetric(false);
    SetLiquid(false);
    SetParticle(false);
    SetUnderwater(false);
    SetPickup(false);
    m_weaponanim = 0;
    m_stepsound = 0;
    m_stealth_stepsound = 0;
    m_crawlsound = 0;
    m_landsound = 0;
    m_fallsound = 0;
    m_impact_radius_mod = 1.0f;
}

/* the name replaced by a copy of another (0x2DEAC0; ReadMaterials and CreateDefaultMaterials
 * inline it, the default's copy calls it) */
void CMaterialEx::SetName(const char *name)
{
    if (m_name)
        _free(m_name, 0, 0);
    m_name = 0;
    m_name = name ? _strdup(name, 0, 0) : 0;
}

/* the decal's name likewise (0x2DEA50) */
void CMaterialEx::SetDecalName(const char *name)
{
    if (m_decal)
        _free(m_decal, 0, 0);
    m_decal = 0;
    m_decal = name ? _strdup(name, 0, 0) : 0;
}

/* a material's index by name, 0 (UNKNOWN) for none or an unknown one (SOCOM 1's; the mission's
 * default soil, the weapons' decal sets and marks, the SEALs' footfalls) */
int CMaterialExArray::ResolveMaterialIndex(const char *name)
{
    if (name) {
        std::map<std::string, int>::iterator i = m_materialex_map.find(name);
        std::map<std::string, int>::iterator e = m_materialex_map.end();

        if (i != e)
            return (*i).second;
    }
    return 0;
}

/* the same lookup for a decal's material (SOCOM 1's; CZMarkList::ReadWeaponDecals walks a name's
 * materials with it, passing the last index -- unread in both builds) */
int CMaterialExArray::ResolveDecalMaterialIndex(const char *name, int index)
{
    if (name) {
        std::map<std::string, int>::iterator i = m_materialex_map.find(name);

        if (i != m_materialex_map.end())
            return (*i).second;
    }
    return 0;
}

/* every material deleted, the list given up, the names forgotten; false when there were none
 * (SOCOM 1's; CMission::Close calls it). The loop's own test after the early return is the
 * target's: `if (n) do ... while (n)` keeps it, a `while` (or a for/break) lets MW thread the
 * first test past it, 8 bytes short */
bool CMaterialExArray::UnInit()
{
    if (!m_materialex_list.size()) {
        FreeVector(m_materialex_list);
        return false;
    }
    if (m_materialex_list.size()) {
        do {
            CMaterialEx *m = m_materialex_list.back();

            m_materialex_list.pop_back();
            delete m;
        } while (m_materialex_list.size());
    }
    FreeVector(m_materialex_list);
    m_materialex_map.clear();
    return true;
}

/* the two materials every mission has, first in the list: UNKNOWN (index 0, the fallback) and
 * PARTICLE_SYSTEM (SOCOM 1's) */
void CMaterialExArray::CreateDefaultMaterials()
{
    CMaterialEx *m = new CMaterialEx;

    m->SetName("UNKNOWN");
    m->m_opacity = 1.0f;
    m->m_penetration = 0.0f;
    m->m_ricochet = 0.0f;
    m->m_elasticity = 0.3f;
    m->SetVolumetric(false);
    m->SetLiquid(false);
    m->SetPickup(false);
    Add(m);

    m = new CMaterialEx;
    m->SetName("PARTICLE_SYSTEM");
    m->m_opacity = 0.0f;
    m->m_penetration = 1.0f;
    m->m_ricochet = 0.0f;
    m->m_elasticity = 0.0f;
    m->SetVolumetric(true);
    m->SetParticle(true);
    m->SetLiquid(false);
    m->SetPickup(false);
    m->SetUnderwater(false);
    m->m_stepsound = 0;
    m->m_stealth_stepsound = 0;
    m->m_crawlsound = 0;
    m->m_landsound = 0;
    m->m_fallsound = 0;
    Add(m);
}

/* the material a weapon animates for one: its WEAPONANIM's, else itself (SOCOM 1's; the weapons'
 * InitializeWeapons) */
CMaterialEx *CMaterialExArray::GetWeaponAnimMaterialEx(unsigned int i)
{
    CMaterialEx *m = GetMaterialEx(i);

    if (m && m->m_weaponanim)
        return GetMaterialEx(m->m_weaponanim);
    return m;
}

/* the mission's soils read: the two defaults, then one material per SOILS entry with a NAME
 * (SOCOM 1's; CMission::Open calls it with its materials rdr). The counter an int: an unsigned one
 * is strength-reduced into a seventh saved register (Item(i)'s i*8), the int recomputes it.
 * OPEN, 10 instructions, stack slots only: the two inlines' frame slots trade places -- the target
 * puts Add's pointer (152) and string (136) above ResolveMaterialIndex's iterator (148) and string
 * (120), MW here the other way round (Resolve's 152/136, Add's 148/120), as if Add were expanded
 * first. Tried: Add out of line above or below (auto-inlined, the same), declared inline after the
 * class, by reference; Resolve's iterators in either order, `->`, the call nested or into a local.
 * ResolveMaterialIndex's end() taken into a local (not `!=`'s argument) is what keeps its tail()
 * inline here, as the target's: written as the argument it sits a level deeper and tail() is called */
bool CMaterialExArray::ReadMaterials(CRdrFile *file)
{
    if (!file)
        return false;
    _zrdr *soils = zrdr_findtag(file, "SOILS");
    if (!soils)
        return false;
    unsigned int count = soils->Count();

    m_materialex_list.reserve(count + 2);
    CreateDefaultMaterials();
    for (int i = 0; i < count; i++) {
        _zrdr *item = soils->Item(i);
        CMaterialEx *m = new CMaterialEx;
        float value = 0.0f;
        const char *name = 0;

        if (!zrdr_findSTRING(item, "NAME", &name)) {
            delete m;
            continue;
        }
        m->SetName(name);
        if (zrdr_findreal(item, "OPACITY", &value, 1))
            m->m_opacity = value;
        const char *step = 0;
        if (zrdr_findSTRING(item, "STEPSOUND", &step))
            m->m_stepsound = CSnd::GetSoundByName(step);
        const char *stealth = 0;
        if (zrdr_findSTRING(item, "STEALTH_STEPSOUND", &stealth))
            m->m_stealth_stepsound = CSnd::GetSoundByName(stealth);
        const char *crawl = 0;
        if (zrdr_findSTRING(item, "CRAWLSOUND", &crawl))
            m->m_crawlsound = CSnd::GetSoundByName(crawl);
        const char *land = 0;
        if (zrdr_findSTRING(item, "LANDSOUND", &land))
            m->m_landsound = CSnd::GetSoundByName(land);
        const char *fall = 0;
        if (zrdr_findSTRING(item, "FALLSOUND", &fall))
            m->m_fallsound = CSnd::GetSoundByName(fall);
        if (zrdr_findreal(item, "PENETRATION", &value, 1))
            m->m_penetration = value;
        if (zrdr_findreal(item, "RICOCHET", &value, 1))
            m->m_ricochet = value;
        if (zrdr_findreal(item, "ELASTICITY_COEFF", &value, 1))
            m->m_elasticity = value;
        if (zrdr_findreal(item, "STEALTH_FACTOR", &value, 1))
            m->m_stealth_factor = value;
        if (zrdr_findreal(item, "FOOT_STEP_OFFSET", &value, 1))
            m->m_foot_step_offset = value;
        if (zrdr_findtag(item, "VOLUMETRIC"))
            m->SetVolumetric(true);
        if (zrdr_findreal(item, "IMPACT_RADIUS_MOD", &value, 1))
            m->m_impact_radius_mod = value;
        if (zrdr_findtag(item, "LIQUID"))
            m->SetLiquid(true);
        if (zrdr_findtag(item, "UNDERWATER"))
            m->SetUnderwater(true);
        if (zrdr_findtag(item, "PICKUP"))
            m->SetPickup(true);
        const char *anim = 0;
        if (zrdr_findSTRING(item, "WEAPONANIM", &anim) && anim)
            m->m_weaponanim = ResolveMaterialIndex(anim);
        const char *decal = 0;
        if (zrdr_findSTRING(item, "DECAL", &decal))
            m->SetDecalName(decal);
        Add(m);
    }
    return true;
}

/* the default material: its index kept (IntersectStruct::GetMaterial reports it for a polygon
 * without one -- new in SOCOM II), its settings copied over the list's first (SOCOM 1's) */
void CMaterialExArray::SetDefaultMaterial(int index)
{
    m_defaultMaterial = index;
    CMaterialEx *m = GetMaterialEx(index);
    CMaterialEx *first = m_materialex_list[0];

    if (m != first)
        *first = *m;
}
