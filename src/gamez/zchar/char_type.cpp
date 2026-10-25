/* char_type.cpp (OVL2, the press beta's file name: its asserts name char_type.cpp): the character
 * types. Only one function so far. */

#include "zchar.h"

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
