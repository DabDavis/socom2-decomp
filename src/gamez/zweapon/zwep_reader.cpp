/* zwep_reader.cpp -- ZWepReader, the weapons rdr's reader: OVL1 0x3CD810-0x3CF1EF, after
 * CZProjectileList (zwep_projlist.cpp) and before zwep_weapon.cpp's CZWeapon, as SOCOM 1's
 * ZWepReader::Read (0x31B920 in the demo) follows its CZProjectileList. A unit of its own: Read calls
 * CZWeapon::SetSoundDistance (zwep_weapon.cpp's, eight bytes of work) out of line, which auto-inlining
 * inside zwep_weapon.cpp would expand. The file name is ours. SOCOM II's Read reads a file and hands
 * each ZAMMO and ZWEAPON entry to a reader of its own (ReadAmmo, ReadWeapon 0x3CDA30 -- not written);
 * the destructor (0x3CF190) and 0x3CF1E0 are drafts. Defined in reverse of their image order. */

#include <stdio.h>
#include <string.h>

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

/* CZWeapon's and CZFTSWeapon's setters ReadWeapon calls that are not named yet (zwep_weapon.cpp's
 * 0x3D2010-0x3D2C30 and zwep_ftsweapon.cpp's 0x3C5960/0x3C5A90: placeholders, by their rdr keys) */
extern "C" {
void func_003D2AC0(CZWeapon *w, const char *s);   /* Description */
void func_003D2770(CZWeapon *w, CZAmmo *ammo);   /* AMMO_TYPES' NAME */
void func_003D2060(CZWeapon *w, float f);   /* Damage_Modifier */
void func_003D2040(CZWeapon *w, float f);   /* ArmingDistance */
void func_003D20E0(CZWeapon *w, const char *s);   /* ReloadSound */
void func_003D2070(CZWeapon *w, const char *s);   /* ReloadAfterShotSound */
void func_003D2C30(CZWeapon *w, float f);   /* ReloadDelay */
void func_003D2C10(CZWeapon *w, float f);   /* ReloadDelayAfterShot */
void func_003C5960(CZFTSWeapon *w, int n);   /* SlotCost */
void func_003D2020(CZWeapon *w, bool on);   /* HasBackblast */
void func_003D2010(CZWeapon *w, float f);   /* RecoilPct */
void func_003C5A90(CTargetMods *m, int n);   /* KnockCount */
}

/* one ZWEAPON entry: a new CZFTSWeapon, its names, the rumbles, the three stances' reticule
 * modifiers (each starting from the stance before's), the fire wait and the zoom modes, the
 * accuracy, the ballistics (lengths scaled to the world's), the id, magazines and legal ammunition,
 * the AI's parameters, the fire modes, the art and the sounds, the reload, the timers, the decal set,
 * then listed (SOCOM II's own: SOCOM 1's Read read the entries inline from an archive). The screen
 * shake's axes are looked up in the entry, not in the ScreenShake tag the test finds -- the target's */
void ZWepReader::ReadWeapon(_zrdr *entry)
{
    float f;
    const char *str = 0;
    int i;
    char notfound[10] = "NOT FOUND";
    CZFTSWeapon *w = new CZFTSWeapon;

    if (!zrdr_findSTRING(entry, "InternalName", &str))
        str = notfound;
    w->SetName(str);
    if (!zrdr_findSTRING(entry, "DisplayName", &str))
        str = notfound;
    w->SetDescription(str);
    if (str && strstr(str, "M60E3"))
        printf("");
    if (!zrdr_findSTRING(entry, "Description", &str))
        str = notfound;
    func_003D2AC0(w, str);

    _zrdr *rumble = zrdr_findtag(entry, "Rumble_Modifiers");

    if (rumble) {
        float t = 0.0f;

        if (zrdr_findreal(rumble, "RumbleHighTime", &t, 1))
            w->SetRumbleHigh_Time(t);
        if (zrdr_findreal(rumble, "RumbleLowTime", &t, 1))
            w->SetRumbleLow_Time(t);
        if (zrdr_findreal(rumble, "RumbleLowPower", &t, 1))
            w->SetRumbleLow_Power(t);
    }

    _zrdr *reticule = zrdr_findtag(entry, "Reticule_Modifiers");

    if (reticule) {
        float v = 0.0f;
        _zrdr *stance = 0;
        int n = 0;

        for (unsigned int s = 0; s < 3; s++) {
            switch (s) {
            case 0:
                stance = zrdr_findtag(reticule, "STANCE_STAND");
                break;
            case 1:
                stance = zrdr_findtag(reticule, "STANCE_CROUCH");
                break;
            case 2:
                stance = zrdr_findtag(reticule, "STANCE_PRONE");
                break;
            }
            CTargetMods *mods = w->TargetMods(s);

            if (s)
                *mods = *w->TargetMods(s - 1);
            if (zrdr_findreal(stance, "ReticuleKnock", &v, 1))
                mods->SetReticuleKnock(v);
            if (zrdr_findreal(stance, "ReticuleKnockReturn", &v, 1))
                mods->SetReticuleKnock_Return(v);
            if (zrdr_findreal(stance, "ReticuleKnockMax", &v, 1))
                mods->SetReticuleKnock_Max(v);
            if (zrdr_findreal(stance, "TargetDilateUponFire", &v, 1))
                mods->SetTarget_DilateUponFire(v);
            if (zrdr_findreal(stance, "TargetDilateUponMovement", &v, 1))
                mods->SetTarget_DilateUponMovement(v);
            if (zrdr_findreal(stance, "TargetDilateUponMovementMult", &v, 1))
                mods->SetTarget_DilateUponMovementMult(v);
            if (zrdr_findreal(stance, "TargetConstrict", &v, 1))
                mods->SetTarget_Constrict(v);
            if (zrdr_findreal(stance, "TargetMin", &v, 1))
                mods->SetTarget_Min(v);
            if (zrdr_findreal(stance, "TargetMax", &v, 1))
                mods->SetTarget_Max(v);
            if (zrdr_findreal(stance, "SniperDistPPFrameX", &v, 1))
                mods->SetSniper_DistPPFrame_PosX(v);
            if (zrdr_findreal(stance, "SniperDistLimitX", &v, 1))
                mods->SetSniper_DistLimit_PosX(v);
            if (zrdr_findreal(stance, "SniperDistPPFrameY", &v, 1))
                mods->SetSniper_DistPPFrame_PosY(v);
            if (zrdr_findreal(stance, "SniperDistLimitY", &v, 1))
                mods->SetSniper_DistLimit_PosY(v);
            if (zrdr_findreal(stance, "SniperDecayRate", &v, 1))
                mods->SetSniper_DecayRate(v);
            if (zrdr_findreal(stance, "FireRifleKickRate", &v, 1))
                mods->SetFireRifle_KickRate(v);
            if (zrdr_findreal(stance, "FireRifleKickReturnRate", &v, 1))
                mods->SetFireRifle_KickReturnRate(v);
            if (zrdr_findreal(stance, "FireRifleKickBaseDist", &v, 1))
                mods->SetFireRifle_KickBaseDist(v);
            if (zrdr_findreal(stance, "FireRifleKickRandomDist", &v, 1))
                mods->SetFireRifle_KickRandomDist(v);
            if (zrdr_findint(stance, "KnockCount", &n, 1))
                func_003C5A90(mods, n);
            if (zrdr_findreal(stance, "KnockEntryStrength", &v, 1))
                mods->SetWiggleReport(v);

            _zrdr *shake = zrdr_findtag(entry, "ScreenShake");

            if (shake && shake->type == ZRDR_ARRAY) {
                _zrdr *axis = zrdr_findtag(entry, "xaxis");

                if (axis && axis->type == ZRDR_ARRAY) {
                    if (zrdr_findreal(axis, "base", &v, 1))
                        mods->m_socom2_4c = v;
                    if (zrdr_findreal(axis, "rand", &v, 1))
                        mods->m_socom2_50 = v;
                    if (zrdr_findreal(axis, "decr", &v, 1))
                        mods->m_socom2_54 = v;
                    if (zrdr_findreal(axis, "dps", &v, 1))
                        mods->m_socom2_58 = v;
                }
                axis = zrdr_findtag(entry, "yaxis");
                if (axis && axis->type == ZRDR_ARRAY) {
                    if (zrdr_findreal(axis, "base", &v, 1))
                        mods->m_socom2_5c = v;
                    if (zrdr_findreal(axis, "rand", &v, 1))
                        mods->m_socom2_60 = v;
                    if (zrdr_findreal(axis, "decr", &v, 1))
                        mods->m_socom2_64 = v;
                    if (zrdr_findreal(axis, "dps", &v, 1))
                        mods->m_socom2_68 = v;
                }
            }
        }
    }

    float zoom;
    int nzoom;
    float wait = 0.1f;
    char name[64];

    zoom = 0.0f;
    nzoom = 0;

    zrdr_findreal(entry, "FireWait", &wait, 1);
    w->m_firewait = wait;
    zrdr_findint(entry, "NumZoomModes", &nzoom, 1);
    for (int z = 0; z < nzoom; z++) {
        sprintf(name, "ZoomMode%d", z);
        if (!zrdr_findreal(entry, name, &zoom, 1))
            zoom = -1.0f;
        w->AddZoommode(zoom);
    }
    if (zrdr_findint(entry, "AccBurstCnt_Min", &i, 1))
        w->AccuracyBurstCnt_Min(i);
    if (zrdr_findint(entry, "AccBurstCnt_Max", &i, 1))
        w->AccuracyBurstCnt_Max(i);
    if (zrdr_findreal(entry, "AccScalar_Min", &f, 1))
        w->AccuracyScalar_Min(f);
    if (zrdr_findreal(entry, "AccScalar_Max", &f, 1))
        w->AccuracyScalar_Max(f);
    if (zrdr_findreal(entry, "Muzzle_Velocity", &f, 1))
        w->SetMuzzleVelocity(WorldScaled(f));
    if (zrdr_findreal(entry, "Gravity_Acceleration", &f, 1))
        w->SetGravity(WorldScaled(f));
    else
        w->SetGravity(WorldScaled(9.8f));
    if (zrdr_findreal(entry, "ImpactRadius", &f, 1))
        w->SetImpactRadius(WorldScaled(f));
    if (!zrdr_findreal(entry, "Effective_Range", &f, 1))
        f = 0.0f;
    w->SetEffectiveRange(WorldScaled(f));
    if (!zrdr_findreal(entry, "Maximum_Range", &f, 1))
        f = 0.0f;
    w->SetMaxRange(WorldScaled(f));
    if (!zrdr_findint(entry, "ID", &i, 1))
        i = 0;
    w->m_ID = (EQUIP_ITEM)i;
    if (!zrdr_findint(entry, "NumMags", &i, 1))
        i = 0;
    w->SetNumMags(i);

    _zrdr *types = zrdr_findtag(entry, "AMMO_TYPES");
    int ntypes = types->Count();

    for (int k = 0; k < ntypes; k++) {
        if (!zrdr_findSTRING(types->Item(k), "NAME", &str))
            str = notfound;
        CZAmmo *ammo = CZAmmo::m_ammolist.GetAmmo(str);

        if (!ammo)
            new CZAmmo;
        func_003D2770(w, ammo);
    }
    if (!zrdr_findint(entry, "Ammo_Capacity", &i, 1))
        i = 0;
    w->SetAmmoCapacity(i);
    if (!zrdr_findreal(entry, "Damage_Modifier", &f, 1))
        f = 0.0f;
    func_003D2060(w, f);
    if (!zrdr_findreal(entry, "ArmingDistance", &f, 1))
        f = 0.0f;
    func_003D2040(w, WorldScaled(f));
    if (!zrdr_findreal(entry, "Sound_Radius", &f, 1))
        f = 0.0f;
    w->SetSoundRadius(WorldScaled(f));
    w->m_ai_params.Read(zrdr_findtag(entry, "ai_params"));
    if (!zrdr_findint(entry, "Encumbrance", &i, 1))
        i = 0;
    w->SetEncumbrance((ENCUMBRANCE)i);
    if (!zrdr_findint(entry, "MaxFireMode", &i, 1))
        i = 0;
    w->SetMaxFireMode((FIREMODE)i);
    if (zrdr_findtag(entry, "BurstMode"))
        w->SetHasFireMode(BURST);
    if (zrdr_findtag(entry, "SingleMode"))
        w->SetHasFireMode(SINGLE);
    if (zrdr_findtag(entry, "AutoMode"))
        w->SetHasFireMode(AUTOFIRE);
    if (!zrdr_findSTRING(entry, "IconTextureName", &str))
        str = notfound;
    w->SetIconTextureName(str);
    if (!zrdr_findSTRING(entry, "AssociatedGear", &str))
        str = notfound;
    w->SetAssociatedGearName(str);
    if (!zrdr_findSTRING(entry, "ModelName", &str))
        str = notfound;
    w->SetModelName(str);
    if (!zrdr_findSTRING(entry, "ReloadSound", &str))
        str = notfound;
    func_003D20E0(w, str);
    if (!zrdr_findSTRING(entry, "ReloadAfterShotSound", &str))
        str = notfound;
    func_003D2070(w, str);
    if (!zrdr_findreal(entry, "ReloadTime", &f, 1))
        f = 0.0f;
    w->m_reloadtime = f;
    if (!zrdr_findreal(entry, "ReloadDelay", &f, 1))
        f = 0.01f;
    func_003D2C30(w, f);
    if (!zrdr_findreal(entry, "ReloadDelayAfterShot", &f, 1))
        f = 0.01f;
    func_003D2C10(w, f);
    if (zrdr_findtag(entry, "ReloadAfterShot"))
        w->m_reloadAfterShot = 1;
    if (!zrdr_findSTRING(entry, "FireAnimName", &str))
        str = notfound;
    w->SetFireAnimName(str);
    if (!zrdr_findSTRING(entry, "HitAnimName", &str))
        str = notfound;
    w->SetHitAnimName(str);
    if (!zrdr_findSTRING(entry, "DefaultSpecialAnimName", &str))
        str = notfound;
    w->SetDefaultSpecialAnimName(str);
    if (!zrdr_findSTRING(entry, "SpecialMaterialAnimName", &str))
        str = notfound;
    w->SetSpecialMaterialAnimName(str);
    if (!zrdr_findreal(entry, "Timer1", &f, 1))
        f = 9999999.0f;
    w->SetTimer(0, f);
    if (!zrdr_findreal(entry, "Timer2", &f, 1))
        f = 9999999.0f;
    w->SetTimer(1, f);
    if (!zrdr_findSTRING(entry, "DecalSet", &str)) {
        str = "NODECAL";
        w->SetDecalSetName(str);
    } else {
        w->SetDecalSetName(str);
    }
    if (!zrdr_findSTRING(entry, "FireSoundClose", &str))
        str = notfound;
    w->SetFireSoundName(0, str);
    if (!zrdr_findSTRING(entry, "FireSoundMed", &str))
        str = notfound;
    w->SetFireSoundName(1, str);
    if (!zrdr_findSTRING(entry, "FireSoundFar", &str))
        str = notfound;
    w->SetFireSoundName(2, str);
    if (!zrdr_findint(entry, "SlotCost", &i, 1))
        i = 1;
    func_003C5960(w, i);
    if (zrdr_findtag(entry, "HasBackblast"))
        func_003D2020(w, true);
    if (!zrdr_findreal(entry, "RecoilPct", &f, 1))
        f = 0.0f;
    func_003D2010(w, f);
    CZWeapon::m_listWeapons.AddWeapon(w);
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
