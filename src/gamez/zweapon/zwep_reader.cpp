/* zwep_reader.cpp -- ZWepReader, the weapons rdr's reader: OVL1 0x3CD810-0x3CF1EF, after
 * CZProjectileList (zwep_projlist.cpp) and before zwep_weapon.cpp's CZWeapon, as SOCOM 1's
 * ZWepReader::Read (0x31B920 in the demo) follows its CZProjectileList. A unit of its own: Read calls
 * CZWeapon::SetSoundDistance (zwep_weapon.cpp's, eight bytes of work) out of line, which auto-inlining
 * inside zwep_weapon.cpp would expand. The file name is ours. SOCOM II's Read reads a file and hands
 * each ZAMMO and ZWEAPON entry to a reader of its own (ReadAmmo, ReadWeapon 0x3CDA30 -- not written);
 * the destructor (0x3CF190) and 0x3CF1E0 are drafts. Defined in reverse of their image order. */

#include "zweapon.h"
#include "../zrdr/zrdr.h"


/* an empty reader (0x3CF1E0: `this` handed back) and its destructor (0x3CF190: the delete flag's
 * test alone) -- the unit's last two in the image, defined first */
ZWepReader::ZWepReader()
{
}

ZWepReader::~ZWepReader()
{
}

/* the world's scale (0x3DFE10, CWorld::m_scale in data.txt: node_world.h does not declare it) */
extern "C" float data_003DFE10;

/* a length in the world's units (inline: the parameter scaled in place -- the target loads the
 * value straight into $f12 and multiplies it there; `f * scale` as the argument loads it into a
 * register of its own, as does a named local holding the product) */
static inline float WorldScaled(float f)
{
    f *= data_003DFE10;
    return f;
}

/* one AMMO entry: a new cartridge type, its names ("NOT FOUND" for a missing one), its id, damage,
 * hit animation, stun, piercing and explosion, the projectiles a shot fires, the blowback's two
 * lengths, the proximity and the acceleration (lengths scaled to the world's), the two tags, then
 * listed (SOCOM II's own: SOCOM 1's Read read the entries inline) */
void ZWepReader::ReadAmmo(_zrdr *entry)
{
    int i;
    float f;
    char notfound[10] = "NOT FOUND";
    const char *str = 0;
    CZAmmo *ammo = new CZAmmo;

    if (!zrdr_findSTRING(entry, "InternalName", &str))
        str = notfound;
    ammo->SetName((char *)str);
    if (!zrdr_findSTRING(entry, "DisplayName", &str))
        str = notfound;
    ammo->SetDescription((char *)str);
    if (!zrdr_findSTRING(entry, "Description", &str))
        str = notfound;
    ammo->SetLongDescription((char *)str);
    if (!zrdr_findint(entry, "ID", &i, 1))
        i = 0;
    ammo->m_ID = (EQUIP_AMMO)i;
    if (!zrdr_findreal(entry, "ImpactDamage", &f, 1))
        f = 0.0f;
    ammo->SetImpactDamage(f);
    if (zrdr_findSTRING(entry, "HitAnimName", &str))
        ammo->SetHitAnim(str);
    if (!zrdr_findreal(entry, "Stun", &f, 1))
        f = 0.0f;
    ammo->SetStun(f);
    if (!zrdr_findreal(entry, "Piercing", &f, 1))
        f = 0.0f;
    ammo->SetArmorPierce(f);
    if (!zrdr_findreal(entry, "Explosion_Damage", &f, 1))
        f = 0.0f;
    ammo->SetExplosionDamage(f);
    if (!zrdr_findreal(entry, "Explosion_Radius", &f, 1))
        f = 0.0f;
    ammo->SetExplosionRadius(WorldScaled(f));
    if (!zrdr_findint(entry, "NumProjectilesFired", &i, 1))
        i = 1;
    ammo->SetNumProjectilesFired(i);
    if (!zrdr_findreal(entry, "Blowback_Falloff", &f, 1))
        f = 0.0f;
    ammo->SetBlowback(0, WorldScaled(f));
    if (!zrdr_findreal(entry, "Blowback_End", &f, 1))
        f = 0.0f;
    ammo->SetBlowback(1, WorldScaled(f));
    if (!zrdr_findreal(entry, "ProximityDistance", &f, 1))
        f = 0.0f;
    ammo->SetProximityDistance(WorldScaled(f));
    if (!zrdr_findreal(entry, "AccelerationFactor", &f, 1))
        f = 1.0f;
    ammo->SetAccelerationFactor(WorldScaled(f));
    if (zrdr_findtag(entry, "Volitile"))
        ammo->SetVolatile(true);
    if (zrdr_findtag(entry, "IgnoreExplosionDI"))
        ammo->SetIgnoreExplosionDI(true);
    CZAmmo::m_ammolist.AddAmmo(ammo);
}

/* the weapons rdr read: the three sound distances (WEAPON_GLOBAL, scaled to the world's), every
 * ZAMMO entry and every ZWEAPON entry handed to their readers, the file freed (SOCOM II's
 * ZWepReader::Read, 0x3CD810: SOCOM 1's read an archive with the readers inline). The counter is
 * declared first and one array local serves both lists, each assigned inside its count's
 * expression: the target's registers (i s4, the array s3, the count s2) and its count read through
 * the finder's v0 with the array's copy in the branch's delay slot; a local per list, a count local
 * of its own or the count written as an if/else give neither */
void ZWepReader::Read(const char *file)
{
    int i;
    _zrdr *rdr = zrdr_read(file, 0, 0);

    if (!rdr)
        return;

    _zrdr *global = zrdr_findtag(rdr, "WEAPON_GLOBAL");
    float f = 0.0f;

    if (zrdr_findreal(global, "SoundDistanceClose", &f, 1))
        CZWeapon::SetSoundDistance(0, WorldScaled(f));
    if (zrdr_findreal(global, "SoundDistanceMed", &f, 1))
        CZWeapon::SetSoundDistance(1, WorldScaled(f));
    if (zrdr_findreal(global, "SoundDistanceFar", &f, 1))
        CZWeapon::SetSoundDistance(2, WorldScaled(f));

    _zrdr *arr;
    int n = (arr = zrdr_findtag(rdr, "ZAMMO"))->Count();

    for (i = 0; i < n; i++)
        ReadAmmo(arr->Item(i));
    n = (arr = zrdr_findtag(rdr, "ZWEAPON"))->Count();
    for (i = 0; i < n; i++)
        ReadWeapon(arr->Item(i));
    zrdr_free((CRdrFile *)rdr);
}
