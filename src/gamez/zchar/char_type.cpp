/* char_type.cpp (OVL2, the press beta's file name: its asserts name char_type.cpp): the character
 * types. Only one function so far. */

#include <algorithm>
#include <new>
#include <string.h>

#include "zchar.h"
#include "../zsys/zsys.h"
#include "../zsnd/zsnd.h"
#include "../zanim/zanim.h"

/* the reader file of that name, out of the loaded archives (SOCOM 1's CRdrArchive::FindRdr took the
 * name alone; SOCOM II passes a second argument, zero here) */
extern "C" CRdrFile *func_0032C040(const char *name, const char *archive);
const char *zrdr_findstring(const _zrdr *rdr, const char *tag);
void zrdr_free(CRdrFile *file);
extern "C" int strcasecmp(const char *, const char *);

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

/* every temporary type deleted and dropped from the list (SOCOM 1's).
 * OPEN (0x84 against 0x80; unchanged at -inline level=5): CWorld::DeleteLightMap's and DeleteChild's include/vector question --
 * the target builds end() once (44(sp)), passes it to remove_if and copies it for erase's last
 * after the call (to 28(sp)), where this builds it twice and copies before; an end iterator in a
 * local passed to both is 0x70 (no copy for erase at all), an explicit copy of it 0x88 */
void CCharacterType::CleanupTemporary()
{
    m_char_list.erase(std::remove_if(m_char_list.begin(), m_char_list.end(), DeleteTemporaryCharType),
                      m_char_list.end());
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

/* a sound by kind, none out of range (new in SOCOM II; the name ours) */
CSnd *CCharacterType::GetSound(CHARACTER_SOUND s) const
{
    if (!(s > CHRSND_NONE) || s >= CHRSND_MAX)
        return 0;
    return m_sounds[s];
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
