/* char_type.cpp (OVL2, the press beta's file name: its asserts name char_type.cpp): the character
 * types. Only one function so far. */

#include <algorithm>
#include <new>
#include <string.h>

#include "zchar.h"
#include "../zsys/zsys.h"
#include "../zsnd/zsnd.h"
#include "../zanim/zanim.h"
#include "../zentity/zentity.h"

/* the reader file of that name, out of the loaded archives (SOCOM 1's CRdrArchive::FindRdr took the
 * name alone; SOCOM II passes a second argument, zero here) */
extern "C" CRdrFile *func_0032C040(const char *name, const char *archive);
const char *zrdr_findstring(const _zrdr *rdr, const char *tag);
void zrdr_free(CRdrFile *file);
extern "C" int strcasecmp(const char *, const char *);

/* zseal.h's CCharacterGear, as far as FindGear reads it (its name at 0xC; zseal.h is not included
 * here) */
class CCharacterGear {
public:
    int m_ref_count;   // 0x0
    int m_partid;   // 0x4
    unsigned int m_gearid;   // 0x8
    char *m_name;   // 0xc
    char *m_model_file;   // 0x10
    char *m_model_name;   // 0x14
    void *m_model;   // 0x18
};

/* every gear let go: its model and its count zeroed; the first one still held makes a walk of the
 * types whose body was compiled out (a debug report, likely) -- new in SOCOM II, the name ours (OVL1's
 * 0x2AB3F0 calls it before CleanupTemporary; 0x53F0B0).
 * OPEN (0xD0 against 0xF0): the inner walk's layout -- the target enters the empty walk of the types
 * through a block placed after the outer loop (`b` forward with the flag's zero in the slot, the end
 * computed and tested there, then `b` back into the loop), README's "the inner loop entered through
 * a block placed after the function's end" (SortNearToFar, CGrid::Create); it also keeps the gear
 * list's address in a0 and reads its size and data each pass. A reference local to the list was
 * tried */
void CCharacterType::ResetGears()
{
    bool first = true;
    std::vector<CCharacterGear *> &gears = m_gear_list;

    for (std::vector<CCharacterGear *>::iterator i = gears.begin(); i != gears.end(); ++i) {
        CCharacterGear *gear = *i;

        if (gear->m_ref_count && first) {
            first = false;
            for (CCharTypeVec::iterator t = m_char_list.begin(); t != m_char_list.end(); ++t)
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
    for (iterator i = begin(); i != end(); ++i)
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
        for (CCharTypeVec::iterator i = m_char_list.begin(); !found && i != m_char_list.end(); ++i) {
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
            (*j)->m_sound = 0;
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
 * SOCOM II's calls it -- the push's insert out of line keeps -inline auto off it). The iterator
 * assigned, so it lives in memory; the end in a register */
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
 * the self test new): the parent's index walked along the list.
 * OPEN (0x84 against 0x88): the range test. The target materialises it (sltu v0, then bnez to the
 * walk, the base zeroed in the delay slot and a `b` to the self test; SOCOM 1's has the same
 * shape), this build branches on `sltu at`; the test as `size() > id`, an if/else, the base
 * zeroed first and an inline bool helper all compile to one of two shorter forms. An inline
 * `type_at(n)` (`if (n >= size()) return 0; return nth_type(n);`) gives the target's sltu/bnez
 * shape, but is an inline height too tall at level 4 (size() and the walk called); at level 5 it
 * is 0x8C, the zero in v0 where the target's base is v1 (a result local in the helper: 0x7C) */
char *CCharacterType::GetBaseName()
{
    CCharacterType *base = m_parentID < m_char_list.size() ? nth_type(m_parentID) : 0;

    if (base == this)
        base = 0;
    return base ? base->m_name : 0;
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
    _zrdr *list = func_0032F0D0((_zrdr *)rdr, group);
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
 * OPEN, right size at -inline level=5 (was 0x150 against 0x140 at 4), 2 instructions: the first
 * call's arguments -- the target sets "navyseals" in a1 before rdr in a0, this build the other
 * way round; the string locals' and the result's declaration orders and a negated test tried */
CCharacterType *CCharacterType::RdrFetch_CharacterType(const char *name)
{
    CRdrFile *rdr = func_0032C040("chartype.rdr", 0);

    if (!rdr)
        return 0;
    const char *tangos = "terrorists";
    const char *escortees = "escortees";
    const char *type;

    if ((type = match_name_to_chartype(rdr, "navyseals", name)) == 0 &&
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
