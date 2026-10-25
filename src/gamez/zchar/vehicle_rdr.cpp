/* vehicle_rdr.cpp (OVL2 0x532D10-, the name ours): the vehicle file -- each entry's starts, its
 * character's name, the entries by name. SOCOM 1's functions in the demo's order (0x1A2BD0-, after
 * ai_vismap.cpp); defined here in reverse of the image's order (-inline deferred). Only the small
 * functions so far. */

#include <string.h>

#include "zchar.h"

extern "C" int strcasecmp(const char *, const char *);

/* the entry of that name, or none (SOCOM 1's; SOCOM II tests the name first). A class iterator
 * walk: its != materialises (xor, sltiu, xori) and the end is re-read after each strcasecmp */
CVehicleRdrEntry *CVehicleRdr::GetEntryByName(const char *name) const
{
    if (name) {
        for (const_iterator i = const_iterator(data_); i != const_iterator(data_ + size_); ++i) {
            if (!strcasecmp((*i).m_name, name))
                return (CVehicleRdrEntry *)&*i;
        }
    }
    return 0;
}

/* the character the entry places, or none (SOCOM 1's, strncpy for its strcpy) */
void CVehicleRdrEntry::SetCharacterName(const char *name)
{
    if (name)
        strncpy(m_character, name, sizeof(m_character));
    else
        m_character[0] = 0;
}

/* the current start's location name and direction, or none (SOCOM 1's) */
const char *CVehicleRdrEntry::GetStartPos() const
{
    return m_start.GetName(m_startIndex);
}

int CVehicleRdrEntry::GetStartDir() const
{
    return m_start.GetDir(m_startIndex);
}

/* the same for the respawning set (new in SOCOM II) */
const char *CVehicleRdrEntry::GetRespawnPos() const
{
    return m_respawn.GetName(m_respawnIndex);
}

int CVehicleRdrEntry::GetRespawnDir() const
{
    return m_respawn.GetDir(m_respawnIndex);
}
