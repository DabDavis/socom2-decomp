/* char_type.cpp (OVL2, the press beta's file name: its asserts name char_type.cpp): the character
 * types. Only one function so far. */

#include <algorithm>
#include <new>
#include <string.h>

#include "zchar.h"

/* a type made, or made again in place when asked; one that exists is not handed back otherwise
 * (SOCOM 1's returned it). GetByNameMod inlines whole in the target.
 * OPEN (0x168 against 0x158): an include/vector height -- inlined here, GetByNameMod's loop test
 * calls the pointer vector's base end() and __iter out of line (end() as an argument of the inline
 * != sits a level deeper, five high under Create), where the target computes data + size inline;
 * GetByNameMod alone matches. `!(i == end())` and `end() != i` do not change it */
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
 * OPEN (0x84 against 0x80): CWorld::DeleteLightMap's and DeleteChild's include/vector question --
 * the target builds end() once (44(sp)), passes it to remove_if and copies it for erase's last
 * after the call (to 28(sp)), where this builds it twice and copies before; an end iterator in a
 * local passed to both is 0x70 (no copy for erase at all), an explicit copy of it 0x88 */
void CCharacterType::CleanupTemporary()
{
    m_char_list.erase(std::remove_if(m_char_list.begin(), m_char_list.end(), DeleteTemporaryCharType),
                      m_char_list.end());
}
