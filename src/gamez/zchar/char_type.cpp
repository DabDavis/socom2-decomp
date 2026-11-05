/* char_type.cpp (OVL2, the press beta's file name: its asserts name char_type.cpp): the character
 * types. Only one function so far. */

#include <algorithm>
#include <new>
#include <stdio.h>
#include <string.h>

#include "zchar.h"
#include "../zsys/zsys.h"
#include "../zutil/util_stable.h"
#include "../zsnd/zsnd.h"
#include "../zanim/zanim.h"
#include "../zentity/zentity.h"

/* the reader file of that name, out of the loaded archives (zrdr_archive.cpp's; SOCOM 1's took the
 * name alone, SOCOM II passes flags, zero here): zrdr.h's class as far as this unit calls it (zrdr.h
 * is not included here: its finders' return types differ from the declarations below) */
class CRdrArchive {
public:
    static CRdrFile *FindRdr(const char *path, int flags);
};
_zrdr *zrdr_findtag(const _zrdr *rdr, const char *tag);
const char *zrdr_findstring(const _zrdr *rdr, const char *tag);
bool zrdr_findint(const _zrdr *rdr, const char *tag, int *out, int n);
bool zrdr_findreal(const _zrdr *rdr, const char *tag, float *out, int n);
void zrdr_free(CRdrFile *file);
extern "C" int strcasecmp(const char *, const char *);

/* zseal.h's CCharacterGear, as far as FindGear reads it (its name at 0xC; zseal.h is not included
 * here) */
class CCharacterGear {
public:
    /* nothing yet (Open's `new`: the seven words zeroed, the offset and rotation left) */
    CCharacterGear()
        : m_ref_count(0), m_partid(0), m_gearid(0), m_name(0), m_model_file(0), m_model_name(0), m_model(0)
    {
    }

    int m_ref_count;   // 0x0
    int m_partid;   // 0x4
    unsigned int m_gearid;   // 0x8
    char *m_name;   // 0xc
    char *m_model_file;   // 0x10
    char *m_model_name;   // 0x14
    void *m_model;   // 0x18
    CPnt3D m_offset;   // 0x1c
    CPnt3D m_rotation;   // 0x28
};

/* every gear let go: its model and its count zeroed; the first one still held makes a walk of the
 * types whose body was compiled out (a debug report, likely) -- new in SOCOM II, the name ours (OVL1's
 * 0x2AB3F0 calls it before CleanupTemporary; 0x53F0B0). The empty walk of the types steps postfix:
 * MW hoists its end into a preheader after the function's last block (`b` forward with the flag's
 * zero in the slot, the end computed and tested there, then `b` back into the loop -- the online
 * lane's postfix rule; open since round 11 at 0xD0 against 0xF0 with `++t`). The gear list's
 * reference declared before the flag gives the target's a0 for the list, v1 for the flag (the AI
 * core round 25) */
void CCharacterType::ResetGears()
{
    std::vector<CCharacterGear *> &gears = m_gear_list;
    bool first = true;

    for (std::vector<CCharacterGear *>::iterator i = gears.begin(); i != gears.end(); ++i) {
        CCharacterGear *gear = *i;

        if (gear->m_ref_count && first) {
            first = false;
            for (CCharTypeVec::iterator t = m_char_list.begin(); t != m_char_list.end(); t++)
                ;
        }
        gear->m_model = 0;
        gear->m_ref_count = 0;
    }
}

/* SOCOM 1's SetupCharacterWeapon (its demo's 0x24AFF0, between KorusGetPackedWeapons and AddWeapon
 * there, after KorusGetPackedWeapons here): the item stored first, the weapon looked up in the
 * weapons' list, its counts and ammunition copied when it is known */
void CCharacterWeap::SetupCharacterWeapon(EQUIP_ITEM item)
{
    m_wepId = item;
    CZWeapon *w = CZWeapon::m_listWeapons.GetWeapon(item);

    if (w) {
        m_ammoCount = w->GetAmmoCapacity();
        m_magCount = w->GetNumMags();
        m_ammoId = w->GetDefaultAmmoId();
    }
}

/* SOCOM 1's FindGear (its demo's 0x24B0C0, after AddWeapon), over SOCOM II's vector of pointers:
 * the name tested first, the end re-read every pass (CZKit::SetItemByIndex calls it on
 * CCharacterType::m_gear_list) */
CCharacterGear *CCharacterGearList::FindGear(const char *name)
{
    if (!name)
        return 0;
    for (iterator i = begin(); i != Last(); ++i)
        if (!strcmp(name, (*i)->m_name))
            return *i;
    return 0;
}

/* the record OVL1's 0x437CE8 points at, a character type's name at +300 (unnamed) */
extern char *data_00437CE8;

/* SOCOM 1's KorusGetPackedWeapons (its demo's 0x24AEA0, after UnPack as here) found its type by a
 * fixed name; SOCOM II's by the one at +300 of OVL1's 0x437CE8, through GetByNameMod (inlined) */
void CCharacterType::KorusGetPackedWeapons(CCharacterWeapPacked *packed)
{
    CCharacterType *type = GetByNameMod(data_00437CE8 + 300);

    if (type)
        PackWeapons(packed, type);
}

/* the weapons packed, eight at most (SOCOM 1's KorusGetPackedWeapons' loop; pinned: the target
 * calls it out of line) */
#pragma auto_inline off
void CCharacterType::PackWeapons(CCharacterWeapPacked *packed, CCharacterType *type)
{
    int n = 0;

    for (CCharacterWeapList::iterator i = type->m_weapons.begin(); n < 8 && i != type->m_weapons.end();
         ++i, n++) {
        packed[n].m_packed = CZWeapon::PackItemAndAmmoEnums((*i).m_wepId, (*i).m_ammoId);
        packed[n].m_ammoCount = (*i).m_ammoCount;
        packed[n].m_magCount = (*i).m_magCount;
    }
}
#pragma auto_inline reset

/* SOCOM 1's UnPack (its demo's 0x24ADA0, after Open as here): the weapons cleared, then each of the
 * eight packed ones unpacked until the first without an item, and pushed */
void CCharacterType::UnPack(const CCharacterWeapPacked *packed, CCharacterType *type)
{
    type->m_weapons.clear();
    for (int i = 0; i < 8; i++, packed++) {
        EQUIP_ITEM item;
        EQUIP_AMMO ammo;

        CZWeapon::UnpackItemAndAmmoEnums(packed->m_packed, &item, &ammo);
        if (!item)
            return;
        CCharacterWeap w;

        w.m_ammoId = ammo;
        w.m_wepId = item;
        w.m_ammoCount = packed->m_ammoCount;
        w.m_magCount = packed->m_magCount;
        type->m_weapons.push_back(w);
    }
}

/* the type of that name, or none (SOCOM 1's; SOCOM II tests the name first and walks on until
 * it has one) */
CCharacterType *CCharacterType::GetByNameMod(const char *name)
{
    CCharacterType *found = 0;

    if (name) {
        for (CCharTypeVec::iterator i = m_char_list.begin(); !found && i != m_char_list.Last(); ++i) {
            CCharacterType *t = *i;

            if (!strcmp(t->m_name, name))
                found = t;
        }
    }
    return found;
}

/* a type made, or made again in place when asked; one that exists is not handed back otherwise
 * (SOCOM 1's returned it). GetByNameMod inlines whole in the target -- byte-exact since the pointer
 * vector's end() takes the base's end into a const local (the MSL headers' round 4) */
CCharacterType *CCharacterType::Create(const char *name, const CCharacterType *base, bool reinit)
{
    CCharacterType *t = GetByNameMod(name);

    if (!t) {
        t = new CCharacterType(name, base);
        m_char_list.push_back(t);
    } else if (reinit) {
        t->~CCharacterType();
        new (t) CCharacterType(name, base);
    } else {
        t = 0;
    }
    return t;
}

CRdrFile *zrdr_read(const char *name, const char *path, int flags);
bool zrdr_toREAL(const _zrdr *rdr, float *out, int n);

/* a body part's and a gear node's id by name (OVL2 0x58CB30 -- SOCOM 1's BodyId -- and 0x58CA60, the
 * SEAL lane's rows) */
extern "C" int func_0058CB30(const char *name);
extern "C" void func_0058CA60(const char *name);

/* the AI's aiming spread (zseal.h's class; only the setter here) */
class CSealCtrlAi {
public:
    static void SetAiAccuracyAngle(float angle);
};

/* chartype.rdr, kept once read (OVL1 bss 0x44CD30: only Open stores it) */
extern CRdrFile *data_0044CD30;

/* the file's angles in degrees ("AngleUnits" "DEG"): 1, else 0 (OVL2 bss 0x66A4C0) */
static int s_angleUnitsDeg;

/* a copy of the name without its extension, none for none (SOCOM 1's helper at char_type.cpp's line
 * 93, which its debug _strdup names) */
static inline char *DupBaseName(const char *s)
{
    char *r;

    if (!s) {
        r = 0;
    } else {
        r = _strdup(s, 0, 0);
        char *dot = strchr(r, '.');

        if (dot)
            *dot = 0;
    }
    return r;
}

/* whether the item is that string (the name ours: an inline bool ternary -- the target materialises
 * the compare (xor with zero, sltiu) and masks the joined result before it branches; an if/else or
 * an && in the helper branch on the type or keep the result in another register) */
static inline bool IsToken(const _zrdr *item, const char *token)
{
    return item->type == 3 ? strcmp(token, item->string) == 0 : false;
}

/* the type file read and kept: the body items' names registered as body parts (but "gear_nodes"),
 * then the gear nodes' as nodes; the angle units; the AI's accuracy angle; each gear -- name, model
 * and a three-item "ofs" (its node, offset and rotation) -- made and listed, the walk stopped at the
 * first malformed one; then the characters, a little machine over the list: a name, an optional ":"
 * and the base type's name, then the record, each type made from its base, parsed and listed or
 * deleted. The type list's room grown by 32 (SOCOM 1's, the gear's rotation and the base new) */
bool CCharacterType::Open(const char *file)
{
    m_anim_set_names.clear();
    CRdrFile *rdr = zrdr_read(file, 0, 0);

    if (rdr) {
        data_0044CD30 = rdr;
        _zrdr *items = zrdr_findtag((_zrdr *)rdr, "body_items");

        if (items && items->type == 4) {
            for (unsigned int i = 0; i < items->Count(); i++)
                if (items->Item(i)->type == 3 && strcasecmp(items->Item(i)->String(), "gear_nodes"))
                    func_0058CB30(items->Item(i)->String());

            _zrdr *nodes = zrdr_findtag((_zrdr *)rdr, "gear_nodes");

            if (nodes && nodes->type == 4) {
                for (unsigned int i = 0; i < nodes->Count(); i++)
                    if (nodes->Item(i)->type == 3)
                        func_0058CA60(nodes->Item(i)->String());
            }
        }

        const char *units = zrdr_findstring((_zrdr *)rdr, "AngleUnits");

        if (units && !strcmp(units, "DEG"))
            s_angleUnitsDeg = 1;
        else
            s_angleUnitsDeg = 0;

        _zrdr *ai = zrdr_findtag((_zrdr *)rdr, "ai_settings");
        float angle;

        if (ai && zrdr_findreal(ai, "accuracy_angle", &angle, 1)) {
            if (s_angleUnitsDeg == 1)
                angle *= 0.017453292f;
            CSealCtrlAi::SetAiAccuracyAngle(angle);
        }

        _zrdr *gear = zrdr_findtag((_zrdr *)rdr, "gear");

        if (gear && gear->type == 4) {
            for (int i = 0; i < gear->Count(); i++) {
                bool bad = false;
                _zrdr *g = gear->Item(i);
                const char *name = zrdr_findstring(g, "name");
                const char *model = zrdr_findstring(g, "model");
                _zrdr *ofs = zrdr_findtag(g, "ofs");

                if (model && ofs && ofs->Count() == 3) {
                    const char *node = ofs->Item(0)->String();
                    float pos[3];
                    float rot[3];

                    if (node && zrdr_toREAL(ofs->Item(1), pos, 3) && zrdr_toREAL(ofs->Item(2), rot, 3)) {
                        CCharacterGear *cg = new CCharacterGear;

                        cg->m_model_file = _strdup(model, 0, 0);
                        cg->m_model_name = DupBaseName(model);
                        cg->m_name = name ? _strdup(name, 0, 0) : DupBaseName(model);
                        cg->m_offset = *(CPnt3D *)pos;
                        if (s_angleUnitsDeg == 1)
                            rot[0] *= 0.017453292f;
                        if (s_angleUnitsDeg == 1)
                            rot[1] *= 0.017453292f;
                        if (s_angleUnitsDeg == 1)
                            rot[2] *= 0.017453292f;
                        cg->m_rotation = *(CPnt3D *)rot;
                        cg->m_partid = func_0058CB30(node);
                        func_0058CA60(name);
                        m_gear_list.push_back(cg);
                    } else {
                        bad = true;
                    }
                } else {
                    bad = true;
                }
                if (bad)
                    break;
            }
        }

        _zrdr *chars = zrdr_findtag((_zrdr *)rdr, "characters");

        if (chars && chars->type == 4) {
            unsigned int i = 0;
            CCharacterType *base = 0;
            int state = 0;
            const char *name = 0;

            do {
                _zrdr *item = chars->Item(i);

                if (!item)
                    break;
                switch (state) {
                case 0:
                    name = item->String();
                    state = name ? 1 : -state;
                    i++;
                    if (strstr(name, "m83"))
                        printf("");
                    break;
                case 1:
                    if (IsToken(item, ":")) {
                        i++;
                        state = 2;
                    } else {
                        state = 3;
                    }
                    break;
                case 2: {
                    const char *b = item ? item->String() : 0;

                    base = GetByNameMod(b);
                    state = base ? 3 : -state;
                    i++;
                    break;
                }
                case 3:
                    if (item->type == 4) {
                        CCharacterType *t = new CCharacterType(name, base);

                        if (t->Parse(item))
                            m_char_list.push_back(t);
                        else
                            delete t;
                        state = 0;
                        i++;
                        name = 0;
                    } else {
                        state = -state;
                    }
                    base = 0;
                    break;
                }
            } while (state >= 0);
        }
    }
    m_char_list.reserve(m_char_list.size() + 32);
    return true;
}

/* a temporary type deleted, true if it was (SOCOM 1's; CleanupTemporary's remove_if predicate).
 * SOCOM II drops the null test of the delete (the flag's load has already used the pointer); the
 * flag set before the delete stays in a saved register across it */
bool DeleteTemporaryCharType(CCharacterType *t)
{
    bool deleted = false;

    if (t->m_istemporary) {
        deleted = true;
        delete t;
    }
    return deleted;
}

/* every temporary type deleted and dropped from the list (SOCOM 1's). The end taken once into a local
 * and passed to both: MW makes an argument list's temporaries left to right and evaluates them right
 * to left, so `erase(remove_if(...), end())` puts erase's end below remove_if's temporaries and copies
 * it before the call -- the target's stack slots are this form's (README "Tells from completing the
 * MSL headers, round 8") */
void CCharacterType::CleanupTemporary()
{
    CCharTypeVec::iterator e = m_char_list.end();
    CCharTypeVec::iterator i = std::remove_if(m_char_list.begin(), e, DeleteTemporaryCharType);

    m_char_list.erase(i, e);
}

/* every used type's sounds looked up by name, the death and damage ones set to dialogue, and a
 * male damage and death sound wherever the type names none (SOCOM 1's; SOCOM II sets the mode
 * through the new virtual where SOCOM 1 stored it) */
void CCharacterType::AcquireSoundHandles()
{
    for (CCharTypeVec::iterator it = m_char_list.begin(); it != m_char_list.end(); ++it) {
        CCharacterType *t = *it;

        if (t->m_ref_count > 0) {
            for (int i = 0; i < CHRSND_MAX; i++) {
                if (t->m_arrSoundNames[i])
                    t->m_sounds[i] = CSnd::GetSoundByName(t->m_arrSoundNames[i]);
                if (t->m_sounds[i]) {
                    t->m_sounds[i]->m_mode = 2;
                    if (i == CHRSND_PLAYER_DOWN || i == CHRSND_DAMAGE || i == CHRSND_DEATH ||
                        i == CHRSND_DEATH_QUIET || i == CHRSND_TEAMMATE_DOWN)
                        t->m_sounds[i]->SetDialogMode();
                }
            }
            if (!t->m_sounds[CHRSND_DAMAGE]) {
                t->m_sounds[CHRSND_DAMAGE] = CSnd::GetSoundByName("MALE_DAMAGE");
                if (t->m_sounds[CHRSND_DAMAGE])
                    t->m_sounds[CHRSND_DAMAGE]->SetDialogMode();
            }
            if (!t->m_sounds[CHRSND_DEATH]) {
                t->m_sounds[CHRSND_DEATH] = CSnd::GetSoundByName("DEATH_SOUNDS");
                if (t->m_sounds[CHRSND_DEATH])
                    t->m_sounds[CHRSND_DEATH]->SetDialogMode();
            }
        }
    }
}

/* a sound's name (SOCOM 1's; the cases in the image's order, which is not the values') */
const char *GetCharacterSoundTypeString(CHARACTER_SOUND s)
{
    switch (s) {
    case CHRSND_NONE:
        return "CHRSND_NONE";
    case CHRSND_YES:
        return "CHRSND_YES";
    case CHRSND_NO:
        return "CHRSND_NO";
    case CHRSND_TEAMMATE_DOWN:
        return "CHRSND_TEAMMATE_DOWN";
    case CHRSND_PLAYER_DOWN:
        return "CHRSND_PLAYER_DOWN";
    case CHRSND_UNDER_FIRE:
        return "CHRSND_UNDER_FIRE";
    case CHRSND_DAMAGE:
        return "CHRSND_DAMAGE";
    case CHRSND_DEATH_QUIET:
        return "CHRSND_DEATH_QUIET";
    case CHRSND_DEATH:
        return "CHRSND_DEATH";
    case CHRSND_WAITING_FOR_PLAYER:
        return "CHRSND_WAITING_FOR_PLAYER";
    case CHRSND_PLAYER_WARN_TOO_FAR:
        return "CHRSND_PLAYER_WARN_TOO_FAR";
    case CHRSND_ABORT_PLAYER_TOO_FAR:
        return "CHRSND_ABORT_PLAYER_TOO_FAR";
    case CHRSND_AREA_CLEARED:
        return "CHRSND_AREA_CLEARED";
    case CHRSND_BREACH_COMPLETE:
        return "CHRSND_BREACH_COMPLETE";
    case CHRSND_CLEARING_BABBLE:
        return "CHRSND_CLEARING_BABBLE";
    case CHRSND_LASE_STARTED:
        return "CHRSND_LASE_STARTED";
    case CHRSND_LASE_COMPLETE:
        return "CHRSND_LASE_COMPLETE";
    case CHRSND_DEPLOY_GENERIC_COMPLETE:
        return "CHRSND_DEPLOY_GENERIC_COMPLETE";
    case CHRSND_DEPLOY_BLUE_CHEM_COMPLETE:
        return "CHRSND_DEPLOY_BLUE_CHEM_COMPLETE";
    case CHRSND_DEPLOY_FRAG_COMPLETE:
        return "CHRSND_DEPLOY_FRAG_COMPLETE";
    case CHRSND_DEPLOY_RED_SMOKE_COMPLETE:
        return "CHRSND_DEPLOY_RED_SMOKE_COMPLETE";
    case CHRSND_DEPLOY_SMOKE_COMPLETE:
        return "CHRSND_DEPLOY_SMOKE_COMPLETE";
    case CHRSND_DEPLOY_C4:
        return "CHRSND_DEPLOY_C4";
    case CHRSND_DEPLOY_FLASHBANG:
        return "CHRSND_DEPLOY_FLASHBANG";
    case CHRSND_SATCHEL_PLACED:
        return "CHRSND_SATCHEL_PLACED";
    case CHRSND_DEPLOY_TOOFAR:
        return "CHRSND_DEPLOY_TOOFAR";
    case CHRSND_BRAVO_IS_DISTANT:
        return "CHRSND_BRAVO_IS_DISTANT";
    case CHRSND_DEFUSE_BOMB_COMPLETE:
        return "CHRSND_DEFUSE_BOMB_COMPLETE";
    case CHRSND_GOTO_COMPLETE:
        return "CHRSND_GOTO_COMPLETE";
    case CHRSND_MOVING_OUT:
        return "CHRSND_MOVING_OUT";
    case CHRSND_INTIMIDATE:
        return "CHRSND_INTIMIDATE";
    case CHRSND_TANGO_DOWN:
        return "CHRSND_TANGO_DOWN";
    case CHRSND_TANGO_ONE_OCLOCK:
        return "CHRSND_TANGO_ONE_OCLOCK";
    case CHRSND_TANGO_TWO_OCLOCK:
        return "CHRSND_TANGO_TWO_OCLOCK";
    case CHRSND_TANGO_THREE_OCLOCK:
        return "CHRSND_TANGO_THREE_OCLOCK";
    case CHRSND_TANGO_FOUR_OCLOCK:
        return "CHRSND_TANGO_FOUR_OCLOCK";
    case CHRSND_TANGO_FIVE_OCLOCK:
        return "CHRSND_TANGO_FIVE_OCLOCK";
    case CHRSND_TANGO_SIX_OCLOCK:
        return "CHRSND_TANGO_SIX_OCLOCK";
    case CHRSND_TANGO_SEVEN_OCLOCK:
        return "CHRSND_TANGO_SEVEN_OCLOCK";
    case CHRSND_TANGO_EIGHT_OCLOCK:
        return "CHRSND_TANGO_EIGHT_OCLOCK";
    case CHRSND_TANGO_NINE_OCLOCK:
        return "CHRSND_TANGO_NINE_OCLOCK";
    case CHRSND_TANGO_TEN_OCLOCK:
        return "CHRSND_TANGO_TEN_OCLOCK";
    case CHRSND_TANGO_ELEVEN_OCLOCK:
        return "CHRSND_TANGO_ELEVEN_OCLOCK";
    case CHRSND_TANGO_TWELVE_OCLOCK:
        return "CHRSND_TANGO_TWELVE_OCLOCK";
    case CHRSND_TANGO_ONE_OCLOCK_HIGH:
        return "CHRSND_TANGO_ONE_OCLOCK_HIGH";
    case CHRSND_TANGO_TWO_OCLOCK_HIGH:
        return "CHRSND_TANGO_TWO_OCLOCK_HIGH";
    case CHRSND_TANGO_THREE_OCLOCK_HIGH:
        return "CHRSND_TANGO_THREE_OCLOCK_HIGH";
    case CHRSND_TANGO_FOUR_OCLOCK_HIGH:
        return "CHRSND_TANGO_FOUR_OCLOCK_HIGH";
    case CHRSND_TANGO_FIVE_OCLOCK_HIGH:
        return "CHRSND_TANGO_FIVE_OCLOCK_HIGH";
    case CHRSND_TANGO_SIX_OCLOCK_HIGH:
        return "CHRSND_TANGO_SIX_OCLOCK_HIGH";
    case CHRSND_TANGO_SEVEN_OCLOCK_HIGH:
        return "CHRSND_TANGO_SEVEN_OCLOCK_HIGH";
    case CHRSND_TANGO_EIGHT_OCLOCK_HIGH:
        return "CHRSND_TANGO_EIGHT_OCLOCK_HIGH";
    case CHRSND_TANGO_NINE_OCLOCK_HIGH:
        return "CHRSND_TANGO_NINE_OCLOCK_HIGH";
    case CHRSND_TANGO_TEN_OCLOCK_HIGH:
        return "CHRSND_TANGO_TEN_OCLOCK_HIGH";
    case CHRSND_TANGO_ELEVEN_OCLOCK_HIGH:
        return "CHRSND_TANGO_ELEVEN_OCLOCK_HIGH";
    case CHRSND_TANGO_TWELVE_OCLOCK_HIGH:
        return "CHRSND_TANGO_TWELVE_OCLOCK_HIGH";
    case CHRSND_TANGO_MOVING:
        return "CHRSND_TANGO_MOVING";
    case CHRSND_ENEMY_SIGHTED:
        return "CHRSND_ENEMY_SIGHTED";
    case CHRSND_ENEMY_AWARE:
        return "CHRSND_ENEMY_AWARE";
    case CHRSND_ENEMY_HOSTILE:
        return "CHRSND_ENEMY_HOSTILE";
    case CHRSND_ENEMY_CLOSE:
        return "CHRSND_ENEMY_CLOSE";
    case CHRSND_ENGAGING:
        return "CHRSND_ENGAGING";
    case CHRSND_THANKYOU:
        return "CHRSND_THANKYOU";
    case CHRSND_GREETING:
        return "CHRSND_GREETING";
    case CHRSND_STATUS_OK:
        return "CHRSND_STATUS_OK";
    case CHRSND_STATUS_HOT:
        return "CHRSND_STATUS_HOT";
    case CHRSND_DEPLOY_TOO_FAR:
        return "CHRSND_DEPLOY_TOO_FAR";
    case CHRSND_WAIT_UP:
        return "CHRSND_WAIT_UP";
    case CHRSND_GRENADE_I_GOT_IT:
        return "CHRSND_GRENADE_I_GOT_IT";
    case CHRSND_GRENADE_OH_HECK:
        return "CHRSND_GRENADE_OH_HECK";
    case CHRSND_COVER_CANT_SEE:
        return "CHRSND_COVER_CANT_SEE";
    case CHRSND_GRENADE_ALERT:
        return "CHRSND_GRENADE_ALERT";
    case CHRSND_NOAMMO:
        return "CHRSND_NOAMMO";
    case CHRSND_RELOAD:
        return "CHRSND_RELOAD";
    case CHRSND_SUSPICIOUS:
        return "CHRSND_SUSPICIOUS";
    case CHRSND_SURPRISED:
        return "CHRSND_SURPRISED";
    case CHRSND_SURRENDER:
        return "CHRSND_SURRENDER";
    case CHRSND_RESTRAINED:
        return "CHRSND_RESTRAINED";
    case CHRSND_BATTLE_CRY:
        return "CHRSND_BATTLE_CRY";
    case CHRSND_PANIC_CRY:
        return "CHRSND_PANIC_CRY";
    case CHRSND_HELPME:
        return "CHRSND_HELPME";
    case CHRSND_PLAYER_IDLE:
        return "CHRSND_PLAYER_IDLE";
    case CHRSND_FRIENDLYFIRE:
        return "CHRSND_FRIENDLYFIRE";
    case CHRSND_TURNCOAT:
        return "CHRSND_TURNCOAT";
    case CHRSND_INPOSITION:
        return "CHRSND_INPOSITION";
    case CHRSND_NOTSURE:
        return "CHRSND_NOTSURE";
    }
    return "CHRSND_NONE";
}

/* the gears' models, shared by name (SOCOM 1's CCharacterModelCache::m_ModelCache, a static of
 * char_type.cpp there at the demo's 0x506420, 0x60 after m_char_list; SOCOM II's is OVL1 bss at
 * 0x44CDA8, the same 0x60 after its m_char_list) */
class CCharacterModelCache {
public:
    static zdb::CModelVector m_ModelCache;
};

/* each level of detail's model, from the model cache or else the world (then cached); when it and
 * the node each hold one visual, both meshes, the node's mesh takes the model's as a level at its
 * distance (SOCOM 1's, whose LODs were 12 bytes). The distance in a local loads it ahead of the size
 * tests, and the model's visuals through a reference keep their address in a0 (its data read at +8
 * after the node's size test), as the target does: written out, both fold away */
void CCharacterType::CreateLODs(zdb::CNode *node)
{
    for (LODVec::iterator i = m_LODs.begin(); i != m_LODs.end(); ++i) {
        zdb::CModel *model = CCharacterModelCache::m_ModelCache.GetModel(i->m_file.c_str());

        if (!model) {
            model = (zdb::CModel *)zdb::CWorld::GetModel(i->m_file.c_str());
            if (model) {
                /* a copy pushed, so the model stays in its register */
                zdb::CModel *kept = model;

                CCharacterModelCache::m_ModelCache.push_back(kept);
            }
        }
        if (model) {
            float dist = i->m_dist;
            zdb::CVisualVector &visuals = model->m_visual;

            if (visuals.size() == 1 && node->m_visual.size() == 1) {
                zdb::CVisual *lod = visuals[0];
                zdb::CVisual *mesh = node->m_visual[0];

                if (lod && lod->m_type == 1 && mesh && mesh->m_type == 1)
                    ((zdb::CMesh *)mesh)->AddLOD((zdb::CMesh *)lod, dist);
            }
        }
    }
}

/* the first holder loads the type's model, and each gear without one its own -- from the model
 * cache, else the world, then put in the cache with its mode tree set; every gear with a model
 * counted. None without a model; otherwise its mode tree set and the type counted and handed back
 * (SOCOM 1's, whose gear list was a list) */
CCharacterType *CCharacterType::Acquire() const
{
    if (m_ref_count == 0) {
        m_model = (CModel *)zdb::CWorld::GetModel(m_model_name);
        for (CCharacterGearList::iterator i = ((CCharacterGearList &)m_gear).begin();
             i != ((CCharacterGearList &)m_gear).end(); ++i) {
            CCharacterGear *g = *i;

            if (!g->m_model) {
                g->m_model = CCharacterModelCache::m_ModelCache.GetModel(g->m_model_name);
                if (!g->m_model) {
                    g->m_model = zdb::CWorld::GetModel(g->m_model_name);
                    if (g->m_model) {
                        ((zdb::CNode *)g->m_model)->SetModeTree(2);
                        /* a copy pushed: the target reloads the model after the call into a
                         * stack slot and passes its address */
                        zdb::CModel *model = (zdb::CModel *)g->m_model;

                        CCharacterModelCache::m_ModelCache.push_back(model);
                    }
                }
            }
            if (g->m_model)
                g->m_ref_count++;
        }
    }
    if (m_model) {
        ((zdb::CNode *)m_model)->SetModeTree(2);
        m_ref_count++;
        return (CCharacterType *)this;
    }
    return 0;
}

/* Parse's push_back makes this unit's tail emit the LODs' class insert (0x540410, named this round).
 * OPEN, 0x590 against 0x500: the target calls the reallocation temporary's destructor, the LOD
 * deleter's 0x540A50, out of line (with 12 string copy constructions 0x541150 and 4 assignments
 * 0x5410C0), where this build inlines it (its two frees at +0x50C/+0x530) -- the tail's generation
 * order (README: an insert generated before the deleter's destructor calls it), as the other class
 * inserts' open question; the deleter's clear and destructor themselves match only as drafts */

/* the type's record (SOCOM 1's, whose LOD names went through a string's find and replace): the
 * model and texture names, the controller, the anim sets (each name the list's copy), the default
 * gear (found by name in the gear list) and weapons (each with its ammunition and counts), the
 * strength and recovery, the health, the voice font, the levels of detail (a file's name, the
 * model's without the extension, and a distance), the AI parameters and the sounds */
bool CCharacterType::Parse(_zrdr *rdr)
{
    const char *model = zrdr_findstring(rdr, "model_name");
    const char *texture = zrdr_findstring(rdr, "texture_asset");

    zrdr_findint(rdr, "controller_id", &m_controller_id, 1);

    _zrdr *sets = zrdr_findtag(rdr, "anim_set");

    if (sets && sets->type == 4) {
        m_anim_sets.clear();
        for (int i = 0; i < sets->Count(); i++) {
            char *name = sets->Item(i)->String();

            name = AddAnimSetName(name);
            m_anim_sets.push_back(name);
        }
    }

    _zrdr *gear = zrdr_findtag(rdr, "default_gear");

    if (gear && gear->type == 4) {
        m_gear.clear();
        for (int i = 0; i < gear->Count(); i++) {
            CCharacterGear *g = ((CCharacterGearList &)m_gear_list).FindGear(gear->Item(i)->String());

            if (g)
                m_gear.push_back(g);
        }
    }

    _zrdr *weapons = zrdr_findtag(rdr, "default_weapons");

    if (weapons && weapons->type == 4) {
        m_weapons.clear();
        for (int i = 0; i < weapons->Count(); i++) {
            _zrdr *item = weapons->Item(i);
            EQUIP_ITEM wep = CZWeapon::GetWeaponId(zrdr_findstring(item, "wep_name"));

            if (wep != EQUIP_NONE) {
                CCharacterWeap w;

                w.m_wepId = wep;
                w.m_ammoId = CZAmmo::GetAmmoId(zrdr_findstring(item, "ammo_name"));
                zrdr_findint(item, "ammo_count", &w.m_ammoCount, 1);
                zrdr_findint(item, "mag_count", &w.m_magCount, 1);
                m_weapons.push_back(w);
            }
        }
    }
    zrdr_findint(rdr, "strength", &m_strength, 1);
    zrdr_findint(rdr, "recovery_factor", &m_recovery_factor, 1);
    m_healthParams.Parse(rdr);

    _zrdr *voice = zrdr_findtag(rdr, "VoiceFont");

    if (voice) {
        zrdr_findreal(voice, "SetPitchOffset", &m_voiceFont.m_pitchOffset, 1);
        zrdr_findreal(voice, "SetSpectralFactor", &m_voiceFont.m_spectralFactor, 1);
        zrdr_findreal(voice, "SetSpectralLedge", &m_voiceFont.m_spectralLedge, 1);
        zrdr_findreal(voice, "SetRoboOffset", &m_voiceFont.m_roboOffset, 1);
        zrdr_findreal(voice, "SetRoboSquash", &m_voiceFont.m_roboSquash, 1);
        zrdr_findreal(voice, "SetWhisperFactor", &m_voiceFont.m_whisperFactor, 1);
    }

    _zrdr *lods = zrdr_findtag(rdr, "lods");

    if (lods) {
        m_LODs.clear();
        int n = lods->Count();

        for (int i = 0; i < n; i++) {
            _zrdr *item = lods->Item((unsigned int)i);
            const char *s = item->Item(0)->String();
            char *file = s ? _strdup(s, 0, 0) : 0;

            if (file) {
                LOD lod;

                lod.m_name = file;
                char *dot = strchr(file, '.');

                if (dot)
                    *dot = 0;
                lod.m_file = file;
                lod.m_dist = item->Item(1)->Real();
                m_LODs.push_back(lod);
                if (file)
                    _free(file, 0, 0);
            }
        }
    }
    if (model) {
        m_model_file = _strdup(model, 0, 0);
        m_model_name = DupBaseName(model);
    }
    if (texture) {
        if (m_texture_asset)
            _free(m_texture_asset, 0, 0);
        m_texture_asset = 0;
        m_texture_asset = _strdup(texture, 0, 0);
    }
    m_aiParams.Parse(zrdr_findtag(rdr, "ai_params"));
    ParseSounds(zrdr_findtag(rdr, "sounds"));
    return true;
}

/* the count dropped; at none -- clamped there, new in SOCOM II -- each gear's count dropped and at
 * none zeroed with its model, then the type's model and sounds forgotten (SOCOM 1's, whose gear
 * list was a list and whose model stores were null-tested) */
int CCharacterType::Release()
{
    m_ref_count--;
    if (m_ref_count <= 0) {
        if (m_ref_count < 0)
            m_ref_count = 0;
        for (CCharacterGearList::iterator i = m_gear.begin(); i != m_gear.end(); ++i) {
            CCharacterGear *g = *i;

            g->m_ref_count--;
            if (g->m_ref_count <= 0) {
                g->m_ref_count = 0;
                g->m_model = 0;
            }
        }
        m_model = 0;
        memset(m_sounds, 0, sizeof(m_sounds));
    }
    return m_ref_count;
}

/* the type's model instanced and named; none without a model (SOCOM 1's). The model's byte at
 * 0xC0 is set first (new in SOCOM II: zdb's CModel is not laid out here) */
zdb::CNode *CCharacterType::CreateInstance(const char *name)
{
    zdb::CNode *node = m_model ? zdb::CNode::CreateInstance((zdb::CModel *)m_model, 0, 0) : 0;

    if (!node || !m_model)
        return 0;
    ((unsigned char *)m_model)[0xc0] = 1;
    node->SetName(name);
    return node;
}

/* a message deleted, for std::for_each (the name ours) */
struct DeleteMultiplayerMsg {
    DeleteMultiplayerMsg() {}
    void operator()(MultiplayerMsg *m) { delete m; }
};

/* the messages deleted, the vector cleared and its storage traded away (FreeVector on the base
 * vector: its temporary's constructor, clear, swap and destructor called, 0x53BAE0, 0x53BAD0,
 * 0x53BA90, 0x53BA20) */
void CCharacterType::DeleteMultiplayerMessages()
{
    if (m_mpMessages.size()) {
        std::for_each(m_mpMessages.begin(), m_mpMessages.end(), DeleteMultiplayerMsg());
        m_mpMessages.clear();
        FreeVector<std::vector<MultiplayerMsg *> >(m_mpMessages);
    }
}

/* every type's multiplayer messages' sounds dropped (SOCOM 1's): the type re-read through the
 * list's iterator at each inner pass, the stores possibly aliasing it */
void CCharacterType::ClearMultiplayerMessageSounds()
{
    for (CCharTypeVec::iterator i = m_char_list.begin(); i != m_char_list.end(); ++i)
        for (CMultiplayerMsgVec::iterator j = (*i)->m_mpMessages.begin(); j != (*i)->m_mpMessages.end(); ++j)
            (*j)->sound_pointer = 0;
}

/* a command's id by name in the SEAL lane's two tables, 0 for none (OVL2 0x58E0A0, 0x58E020:
 * seal_cfg.cpp's) -- the first CSealCtrl::GetCommandId (the SEALs' round 46 named the row: SOCOM 1's
 * loop over GetCommandName), declared through a stand-in as zseal.h is not included here */
enum FT_COMMAND { CMD_UNKNOWN = 0, CMD_ACTION = 1, CMD_LAST = 255 };
class CSealCtrl {
public:
    static FT_COMMAND GetCommandId(const char *name);
};
extern "C" unsigned char func_0058E020(const char *name);

/* the strings the messages keep (SOCOM 1's MultiplayerMsgStrings, OVL1 bss 0x44CCF0) */
extern CSTable MultiplayerMsgStrings;

/* the sound list's "multiplayer" items, each a message: its command (by the first table's name, else
 * the second's), the text, the sound and the TCM tag -- the text when the item has no fourth --
 * kept when it has a text (SOCOM 1's; SOCOM II's command lookup is two tables where SOCOM 1 asked
 * CSealCtrl::GetCommandId, and its zrdr counts are the array's own) */
void CCharacterType::InitMultiplayerMessages(_zrdr *rdr)
{
    _zrdr *list = zrdr_findtag(rdr, "multiplayer");

    if (list) {
        m_mpMessages.reserve(list->Count());
        for (unsigned int i = 0; i < list->Count(); i++) {
            _zrdr *item = list->Item(i);
            MultiplayerMsg *msg = new MultiplayerMsg;
            char message[128];
            char tcmtag[128];

            sprintf(message, "%s", item->Item(2)->String());
            char *sound = item->Item(1)->String();
            _zrdr *tag = item->Item(3);

            if (!tag)
                tag = item->Item(2);
            sprintf(tcmtag, "%s", tag->String());
            _zrdr *id = item->Item(0);

            msg->idx = CSealCtrl::GetCommandId(id->String());
            if (!msg->idx)
                msg->idx = func_0058E020(id->String());
            msg->message = MultiplayerMsgStrings.CreateString(message);
            msg->sound = MultiplayerMsgStrings.CreateString(sound);
            msg->tcmtag = MultiplayerMsgStrings.CreateString(tcmtag);
            msg->sound_pointer = 0;
            if (msg->message)
                m_mpMessages.push_back(msg);
        }
    }
}

/* a type's place in the list, -1 for none (the name ours: the constructor's parent index) */
static inline int GetTypeIndex(const CCharacterType *t)
{
    int i = 0;
    CCharTypeVec::iterator it = CCharacterType::m_char_list.begin();
    CCharTypeVec::iterator e = CCharacterType::m_char_list.end();

    for (; it != e; ++it, i++)
        if (t == *it)
            return i;
    return -1;
}

/* a type of that name, everything cleared (the weapons with room for eight) and the sounds and their
 * names zeroed; made from a base, the base's place in the list kept as the parent and everything
 * copied from it -- the strings duplicated, the anim sets and gear shared, the multiplayer messages
 * copied one by one without their sounds (SOCOM 1's, the voice font and messages new) */
CCharacterType::CCharacterType(const char *name, const CCharacterType *base)
    : m_name(0), m_ref_count(0), m_model_file(0), m_model_name(0), m_parentID((unsigned int)-1),
      m_texture_asset(0), m_model(0), m_strength(4), m_recovery_factor(1), m_controller_id(0),
      m_istemporary(0)
{
    m_anim_sets.clear();
    memset(m_sounds, 0, sizeof(m_sounds));
    memset(m_arrSoundNames, 0, sizeof(m_arrSoundNames));
    m_name = _strdup(name, 0, 0);
    if (base) {
        m_parentID = GetTypeIndex(base);
        if (base->m_anim_sets.size()) {
            for (CAnimSetVec::const_iterator a = (char *const *)base->m_anim_sets.data_;
                 a != (char *const *)base->m_anim_sets.data_ + base->m_anim_sets.size_; ++a)
                m_anim_sets.push_back(*a);
        } else if (m_anim_sets.size()) {
            m_anim_sets.clear();
        }
        m_model_file = base->m_model_file ? _strdup(base->m_model_file, 0, 0) : 0;
        m_model_name = base->m_model_name ? _strdup(base->m_model_name, 0, 0) : 0;
        m_texture_asset = base->m_texture_asset ? _strdup(base->m_texture_asset, 0, 0) : 0;
        m_gear = base->m_gear;
        m_weapons = base->m_weapons;
        m_aiParams = base->m_aiParams;
        m_strength = base->m_strength;
        m_recovery_factor = base->m_recovery_factor;
        m_healthParams = base->m_healthParams;
        SetVoiceFont(base->m_voiceFont);
        memcpy(m_sounds, base->m_sounds, sizeof(m_sounds));
        memcpy(m_arrSoundNames, base->m_arrSoundNames, sizeof(m_arrSoundNames));
        m_mpMessages.reserve(base->m_mpMessages.size());
        for (int j = 0; j < (int)base->m_mpMessages.size(); j++) {
            MultiplayerMsg *msg = new MultiplayerMsg;
            const MultiplayerMsg *src = ((CMultiplayerMsgVec &)base->m_mpMessages)[j];

            msg->idx = src->idx;
            msg->message = src->message;
            msg->sound = src->sound;
            msg->tcmtag = src->tcmtag;
            m_mpMessages.push_back(msg);
        }
        m_LODs = base->m_LODs;
    }
}

/* a type let go (SOCOM 1's): each string freed and cleared -- the name, the texture asset, the
 * model's name and file -- the animation sets emptied, the multiplayer messages deleted; then the
 * vectors' own teardown */
CCharacterType::~CCharacterType()
{
    if (m_name)
        _free(m_name, 0, 0);
    m_name = 0;
    if (m_anim_sets.size())
        m_anim_sets.clear();
    if (m_texture_asset)
        _free(m_texture_asset, 0, 0);
    m_texture_asset = 0;
    if (m_model_name)
        _free(m_model_name, 0, 0);
    m_model_name = 0;
    if (m_model_file)
        _free(m_model_file, 0, 0);
    m_model_file = 0;
    DeleteMultiplayerMessages();
}

/* a sound by kind, none out of range (new in SOCOM II; the name ours) */
CSnd *CCharacterType::GetSound(CHARACTER_SOUND s) const
{
    if (!(s > CHRSND_NONE) || s >= CHRSND_MAX)
        return 0;
    return m_sounds[s];
}

/* the sound kind a name gives, none for no name or no match (SOCOM 1's GetCharacterSoundType,
 * out of line there at 0x248110; SOCOM II inlines it into ParseSounds and keeps no copy) */
static CHARACTER_SOUND GetCharacterSoundType(const char *name)
{
    if (name) {
        for (int i = 0; i < CHRSND_MAX; i++) {
            CHARACTER_SOUND s = (CHARACTER_SOUND)i;

            if (!strcasecmp(name, GetCharacterSoundTypeString(s)))
                return s;
        }
    }
    return CHRSND_NONE;
}

/* the sound names' bytes, counted as they are parsed (OVL2 bss 0x66A4B8; nothing reads it here --
 * a debug total, likely) */
static int s_soundNameBytes;

/* the sound list's pairs: a kind's name, then a list whose first item is the file -- kept as the
 * kind's sound name (a plain pointer in SOCOM II, SOCOM 1 assigned a string); any other name is
 * compared with "multiplayer" and left (that block is read after the loop); then the multiplayer
 * messages (SOCOM 1's Parse did all of it in place) */
void CCharacterType::ParseSounds(_zrdr *list)
{
    int n = list ? list->Count() : 0;

    for (int i = 0; i < n; i += 2) {
        /* the indices passed unsigned: with the int counter MW steps a pointer for the first and
         * takes i + 1 before the second's count */
        _zrdr *tag = list->Item((unsigned int)i);
        _zrdr *value = list->Item((unsigned int)(i + 1));
        const char *name = tag ? tag->String() : 0;
        CHARACTER_SOUND s = GetCharacterSoundType(name);

        if (s != CHRSND_NONE) {
            char *file = value->Item(0)->String();

            int len;

            m_arrSoundNames[s] = file;
            /* an if/else: the ternary leaves its two joins unpadded */
            if (file)
                len = strlen(file);
            else
                len = 0;
            s_soundNameBytes += len;
        } else if (!strcmp(name, "multiplayer")) {
        }
    }
    if (list)
        InitMultiplayerMessages(list);
}

/* the list's copy of an anim set's name, else a copy pushed (SOCOM 1's Parse inlined this walk,
 * SOCOM II's Parse calls it). The iterator assigned, so it lives in memory; the end in a register.
 * Pinned: twelve statements, -inline auto would take it into Parse (the push's insert out of line
 * does not stop it) */
#pragma auto_inline off
char *CCharacterType::AddAnimSetName(const char *name)
{
    if (m_anim_set_names.size()) {
        std::list<char *>::iterator i;

        i = m_anim_set_names.begin();
        for (std::list<char *>::iterator e = m_anim_set_names.end(); i != e; ++i) {
            char *s = *i;

            if (!strcmp(name, s))
                return s;
        }
    }
    char *s = _strdup(name, 0, 0);

    m_anim_set_names.push_back(s);
    return s;
}
#pragma auto_inline reset

/* a SEAL's wait before its next pain sound (CZSealBody's float at 0x424, the name ours; zseal.h is
 * not included here) */
struct CSealPainWait {
    unsigned char _0[0x424];
    float m_painWait;   // 0x424
};

/* the positional player (OVL1 0x3428F0: the sound, the entity's id, the position, then 3.0 and
 * 1.0 here -- zanim_cmd_tick.cpp declares it after its own call) */
extern "C" void func_003428F0(CSnd *sound, unsigned int id, const CPnt3D *pos, float a, float b);

/* a character sound for the entity of that id (SOCOM 1's, void there, played every sound the
 * damage and death sounds' way). The damage and death sounds play only once the SEAL's pain wait
 * has run out, which they set to three seconds: at the position if one is given, else as the
 * entity's own; any other sound goes to the positional player, and only that counts as played */
bool CCharacterType::PlayAt(CHARACTER_SOUND s, unsigned int id, const CPnt3D *pos)
{
    bool played = false;
    CSnd *snd = GetSound(s);

    if (snd) {
        if (s == CHRSND_DAMAGE || s == CHRSND_DEATH || s == CHRSND_DEATH_QUIET) {
            CEntity *e = CEntity::m_list.GetEntityById(id);
            CSealPainWait *seal = e && e->m_type == CEntity::ENT_TYPE_SEAL ? (CSealPainWait *)e : 0;
            bool ok = false;

            if (seal && 0.0f == seal->m_painWait)
                ok = true;
            if (pos && ok) {
                if (seal)
                    seal->m_painWait = 3.0f;
                CSndInstance *inst = snd->PlaySound3D(pos, false, 1.0f, 0, 0, id);

                if (inst)
                    inst->TerminateOnFinish();
            } else if (ok) {
                if (seal)
                    seal->m_painWait = 3.0f;
                CSndInstance *inst = snd->PlaySound(true, 0, id);

                if (inst)
                    inst->TerminateOnFinish();
            }
        } else {
            /* set before the call: after it, the flag misses the jal's delay slot and the
             * arguments are scheduled apart */
            played = true;
            func_003428F0(snd, id, pos, 3.0f, 1.0f);
        }
    }
    return played;
}

/* the list's nth type: the iterator walked n steps (MSL's advance; the name ours) */
static inline CCharacterType *nth_type(unsigned int n)
{
    CCharTypeVec::iterator i = CCharacterType::m_char_list.begin();

    while (n--)
        ++i;
    return *i;
}

/* the name of the type this one was made from, or none -- the type itself not counted (SOCOM 1's,
 * the self test new): the parent's index walked along the list. The range test written `>=` with
 * the zero arm first (the target materialises `id < size` -- sltu v0, bnez to the walk, the base
 * zeroed in the delay slot -- where `<` with the walk first branches on `sltu at`), and the name a
 * result local assigned under a test (so the base keeps v1 and the zero goes in the beqz's slot;
 * a ternary return gives the base v0): the AI core round 19, 0x84 before */
char *CCharacterType::GetBaseName()
{
    CCharacterType *base = m_parentID >= m_char_list.size() ? 0 : nth_type(m_parentID);

    if (base == this)
        base = 0;

    char *name = 0;

    if (base)
        name = base->m_name;
    return name;
}

/* a carried weapon by index, none past the end (SOCOM 1's) */
EQUIP_ITEM CCharacterType::GetWeaponByIndex(unsigned int i) const
{
    return i < m_weapons.size() ? ((CCharacterWeapList &)m_weapons)[i].m_wepId : EQUIP_NONE;
}

/* a weapon of the item (SOCOM 1's compare_item, the predicate of its remove_if over the weapons:
 * a byte passed by value). The weapon's item is read as a char and converted back: remove_copy_if
 * (0x540C70, SOCOM 1's 0x24C210 alike) shares that lb with the copy's and masks it for the compare
 * (andi 0xff) where a plain EQUIP_ITEM read is a separate lbu; find_if inside remove_if folds it to
 * the lbu either way */
struct compare_item {
    compare_item(EQUIP_ITEM item) : m_item(item) {}
    bool operator()(const CCharacterWeap &w) const { return m_item == (EQUIP_ITEM)*(const char *)&w.m_wepId; }

    EQUIP_ITEM m_item;
};

/* SOCOM 1's (its demo's 0x247720, char_type.cpp's first function there and its last here): the end
 * taken first, remove_if's new end next, both copied into erase's parameters, the result whether
 * anything went */
bool CCharacterWeapList::RemoveWeapon(EQUIP_ITEM item)
{
    iterator e = end();
    iterator i = std::remove_if(begin(), e, compare_item(item));

    erase(i, e);
    return i != e;
}

/* a weapon's slots in a loadout, 1 for an unknown item (OVL1's 0x3D3750: the weapon list's
 * GetWeapon, then the weapon's count) */
extern "C" int func_003D3750(EQUIP_ITEM item);

/* the item's weapon set up and put at the index, each weapon from there on moved up by one through
 * a held copy, the last pushed at the end (new in SOCOM II, the name ours; SetupCharacterWeapon
 * inlined -- the constructor's item store dead) */
void CCharacterWeapList::InsertWeapon(int index, EQUIP_ITEM item)
{
    CCharacterWeap w;

    w.SetupCharacterWeapon(item);
    CCharacterWeap tmp;

    for (; index < size(); index++) {
        tmp = (*this)[index];
        (*this)[index] = w;
        w = tmp;
    }
    push_back(w);
}

/* a weapon taking more than one slot (its class not 145) followed by an EQUIP_END for each slot
 * past its first (new in SOCOM II, the name ours) */
void CCharacterWeapList::PadWeaponSlots()
{
    for (int i = 0; i < size(); i++) {
        EQUIP_ITEM item = (*this)[i].m_wepId;

        if (CZWeapon::GetIDType(item) != 145) {
            int slots = func_003D3750(item);

            if (slots > 1) {
                for (int j = 0; j < slots - 1; j++)
                    InsertWeapon(i + j + 1, EQUIP_END);
            }
        }
    }
}

/* SOCOM 1's (its demo's 0x24B060, with Open's stretch there; SOCOM II's sits by GetWeaponByIndex):
 * an empty weapon pushed through the class insert (0x53FD60), the item left to the caller */
void CCharacterWeapList::AddWeapon(EQUIP_ITEM)
{
    push_back(CCharacterWeap());
}

/* the character a group of chartype.rdr gives the name, or none (SOCOM 1's): the group's items
 * searched by "name", the match's "character" handed back */
const char *CCharacterType::match_name_to_chartype(CRdrFile *rdr, const char *group, const char *name)
{
    _zrdr *list = zrdr_findtag((_zrdr *)rdr, group);
    const char *tagName = "name";
    const char *tagChar = "character";

    if (list && list->type == 4) {
        unsigned int n = list->Count();

        for (unsigned int i = 0; i < n; i++) {
            _zrdr *item = i < list->Count() ? &list->array[i] : 0;
            const char *s = zrdr_findstring(item, tagName);

            if (s && !strcasecmp(s, name))
                return zrdr_findstring(item, tagChar);
        }
    }
    return 0;
}

/* the type a character name is, out of chartype.rdr's three groups (SOCOM 1's); GetByNameMod
 * inlines whole in the target, the group names held in saved registers from the start, each
 * match tested on v0 with the copy in the delay slot (assignments inside the && conditions).
 * All three group names are string locals: with "navyseals" a literal in the first call, MW set
 * rdr in a0 before the name in a1 (two instructions swapped, rounds 3-14) */
CCharacterType *CCharacterType::RdrFetch_CharacterType(const char *name)
{
    CRdrFile *rdr = CRdrArchive::FindRdr("chartype.rdr", 0);

    if (!rdr)
        return 0;
    const char *seals = "navyseals";
    const char *tangos = "terrorists";
    const char *escortees = "escortees";
    const char *type;

    if ((type = match_name_to_chartype(rdr, seals, name)) == 0 &&
        (type = match_name_to_chartype(rdr, tangos, name)) == 0)
        type = match_name_to_chartype(rdr, escortees, name);
    CCharacterType *t = GetByNameMod(type);

    zrdr_free(rdr);
    return t;
}

/* the voice font's figures (new in SOCOM II, the names after chartype.rdr's VoiceFont tags): the
 * file's last definitions, so the first of its functions in the image (0x53B010-0x53B06F, right
 * before PadWeaponSlots) -- they could as well close ai_perception.cpp, the unit before: the bytes
 * are the same either way */
float CVoiceFont::GetPitchOffset() const
{
    return m_pitchOffset;
}

float CVoiceFont::GetSpectralFactor() const
{
    return m_spectralFactor;
}

float CVoiceFont::GetSpectralLedge() const
{
    return m_spectralLedge;
}

float CVoiceFont::GetRoboOffset() const
{
    return m_roboOffset;
}

float CVoiceFont::GetRoboSquash() const
{
    return m_roboSquash;
}

float CVoiceFont::GetWhisperFactor() const
{
    return m_whisperFactor;
}

/* (the unit carried `#pragma inline_depth(6)` from the MSL headers' round 24 until the AI core's
 * round 26: with include/vector's image-own end(), four high, a walk testing `i != end()` inside an
 * inline is six deep where its caller expands it, so at level 5 MW kept end()'s body and called the
 * raw end and __iter. GetByNameMod's and FindGear's walks now end at the lists' Last() -- a
 * `data_ + size_` iterator, the online lobby's round-20 refit -- and Create, KorusGetPackedWeapons,
 * Open, Parse and RdrFetch_CharacterType inline them whole at the settled level 5, every function of
 * the unit scoring as under the stand-in) */
