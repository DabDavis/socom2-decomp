/* int_material.h -- the materials' extensions: what a soil is made of (the mission's materials rdr,
 * "SOILS": a name, five footfall sounds, the weapons' animation material, opacity, penetration,
 * ricochet, elasticity, the impact radius, stealth, the foot-step offset, four flags and a decal).
 * SOCOM 1's int_material.cpp: its symbol table names the class (CMaterialEx, whose constructor it
 * links out of line), CMaterialExArray's seven statics and its three data members
 * (m_materialex_map, m_materialex_list, the list a vector<CMaterialEx *>); no DWARF covers the unit,
 * so the members' names are ours, after the rdr's keys ReadMaterials reads them from.
 *
 * Three units include this header: int_material.cpp, and the weapons' zwep_list.cpp and
 * zwep_projectile.cpp (since the weapons' round 19), each before zweapon.h. zweapon.h (the
 * weapons), seal_env.cpp (the SEALs) and fts_mission.cpp (the game states) keep stand-ins of
 * CMaterialExArray (the weapons' spells the element MATERIALEX, a name of theirs) -- zweapon.h's
 * under `#ifndef INT_MATERIAL_H`, so a unit that included this header first sees the real class --
 * and no unit may see two definitions of the class. The stand-ins' members are spelled as here, so
 * every mangled name agrees. */

#ifndef INT_MATERIAL_H
#define INT_MATERIAL_H

#include <map>
#include <string>
#include <vector>
#include "../zrdr/zrdr.h"
#include "../zsnd/zsnd.h"
#include "../zsys/zsys.h"
#include "../zutil/zutil.h"

/* one material (SOCOM 1's CMaterialEx; 0x40 bytes, the constructor 0x2DEB30) */
class CMaterialEx {
public:
    CMaterialEx();
    /* the sounds forgotten, the two names given back (ReadMaterials and UnInit inline it at their
     * deletes; no out-of-line copy is linked) */
    ~CMaterialEx()
    {
        m_stepsound = 0;
        m_stealth_stepsound = 0;
        m_crawlsound = 0;
        m_landsound = 0;
        m_fallsound = 0;
        if (m_name)
            _free(m_name, 0, 0);
        m_name = 0;
        if (m_decal)
            _free(m_decal, 0, 0);
        m_decal = 0;
    }
    /* another material's settings taken (SetDefaultMaterial's copy of the default over slot 0): the
     * names through their setters, every field but the impact radius, the sounds last. Inlined one
     * level there, so its accessors and setters are called out of line (0x2DDE30, 0x2DDE20) */
    CMaterialEx &operator=(const CMaterialEx &o)
    {
        SetName(o.GetName());
        SetDecalName(o.GetDecalName());
        m_weaponanim = o.m_weaponanim;
        m_opacity = o.m_opacity;
        m_penetration = o.m_penetration;
        m_ricochet = o.m_ricochet;
        m_elasticity = o.m_elasticity;
        m_stealth_factor = o.m_stealth_factor;
        m_foot_step_offset = o.m_foot_step_offset;
        m_volumetric = o.m_volumetric;
        m_liquid = o.m_liquid;
        m_underwater = o.m_underwater;
        m_particle = o.m_particle;
        m_pickup = o.m_pickup;
        m_unused = o.m_unused;
        m_stepsound = o.m_stepsound;
        m_stealth_stepsound = o.m_stealth_stepsound;
        m_crawlsound = o.m_crawlsound;
        m_landsound = o.m_landsound;
        m_fallsound = o.m_fallsound;
        return *this;
    }

    const char *GetName() const { return m_name; }
    const char *GetDecalName() const { return m_decal; }
    void SetName(const char *name);
    void SetDecalName(const char *name);
    void SetVolumetric(bool b) { m_volumetric = b; }
    void SetLiquid(bool b) { m_liquid = b; }
    void SetUnderwater(bool b) { m_underwater = b; }
    void SetParticle(bool b) { m_particle = b; }
    void SetPickup(bool b) { m_pickup = b; }

    char *m_name;   // 0x0: NAME
    char *m_decal;   // 0x4: DECAL
    CSnd *m_stepsound;   // 0x8: STEPSOUND
    CSnd *m_stealth_stepsound;   // 0xc: STEALTH_STEPSOUND
    CSnd *m_crawlsound;   // 0x10: CRAWLSOUND
    CSnd *m_landsound;   // 0x14: LANDSOUND
    CSnd *m_fallsound;   // 0x18: FALLSOUND
    unsigned int m_weaponanim;   // 0x1c: WEAPONANIM, a material's index (GetWeaponAnimMaterialEx's)
    float m_opacity;   // 0x20: OPACITY
    float m_penetration;   // 0x24: PENETRATION
    float m_ricochet;   // 0x28: RICOCHET
    float m_elasticity;   // 0x2c: ELASTICITY_COEFF
    float m_impact_radius_mod;   // 0x30: IMPACT_RADIUS_MOD
    float m_stealth_factor;   // 0x34: STEALTH_FACTOR
    float m_foot_step_offset;   // 0x38: FOOT_STEP_OFFSET
    unsigned int m_volumetric : 1;   // 0x3c bit 0: VOLUMETRIC
    unsigned int m_liquid : 1;   // bit 1: LIQUID
    unsigned int m_underwater : 1;   // bit 2: UNDERWATER
    unsigned int m_particle : 1;   // bit 3: no key sets it, PARTICLE_SYSTEM's default does
    unsigned int m_pickup : 1;   // bit 4: PICKUP
    unsigned int m_unused : 27;   // bits 5-31: copied, never set
};

/* the materials (SOCOM 1's CMaterialExArray, statics only) */
class CMaterialExArray {
public:
    static void SetDefaultMaterial(int index);
    static bool ReadMaterials(CRdrFile *file);
    static CMaterialEx *GetWeaponAnimMaterialEx(unsigned int i);
    static void CreateDefaultMaterials();
    static bool UnInit();
    static int ResolveMaterialIndex(const char *name);
    static int ResolveDecalMaterialIndex(const char *name, int index);

    /* a material by index, the first when out of range (ours, inline) */
    static CMaterialEx *GetMaterialEx(unsigned int i)
    {
        if (i >= m_materialex_list.size())
            i = 0;
        return m_materialex_list[i];
    }
    /* a material listed under its name (ours, inline: CreateDefaultMaterials and ReadMaterials) */
    static void Add(CMaterialEx *m)
    {
        m_materialex_map[m->m_name] = m_materialex_list.size();
        m_materialex_list.push_back(m);
    }

    static int m_defaultMaterial;   // 0x44F310 (new in SOCOM II)
    static std::map<std::string, int> m_materialex_map;   // 0x44F330
    static std::vector<CMaterialEx *> m_materialex_list;   // 0x44F350
};

#endif
