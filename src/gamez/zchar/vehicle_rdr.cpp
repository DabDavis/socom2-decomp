/* vehicle_rdr.cpp (OVL2 0x532D10-, the name ours): the vehicle file -- each entry's starts, its
 * character's name, the entries by name. SOCOM 1's functions in the demo's order (0x1A2BD0-, after
 * ai_vismap.cpp); defined here in reverse of the image's order (-inline deferred). Open and Close
 * with Close's FreeVector pieces, which close the unit's template tail (0x533970-0x533A33), and the
 * entry vector's deleter clear and destructor, emitted after AI_PARAMS (0x535040). */

#include <stdlib.h>
#include <string.h>

#include "zchar.h"
#include "../zanim/zanim.h"
#include "../zsys/zsys.h"

class CRdrFile;

_zrdr *zrdr_read(const char *name, const char *file, int line);
void zrdr_free(CRdrFile *file);
/* the entry vector's resize, out of line (0x534320, the map's resize__Q23std72__vector_imp<16CVehicleRdrEntry,
 * ...,0>FUi); include/vector has no resize yet, so it is called through its placeholder */
extern "C" void func_00534320(std::vector<CVehicleRdrEntry> *v, unsigned int n);

extern "C" int strcasecmp(const char *, const char *);

/* an array's item, or none: an unsigned index (zanim.h's Item takes an int, which lets MW
 * strength-reduce Open's loop -- the target recomputes &array[i] each pass; the name is ours) */
static inline _zrdr *item(_zrdr *r, unsigned int i)
{
    return i < r->Count() ? &r->array[i] : 0;
}

/* the vehicle file read: one entry per item of its first array, true if it had one (SOCOM 1's
 * Open handed the file to CVehicleRdr::Load; SOCOM II sizes the vector and loads each entry here).
 * The list is the root's item 0 (`sltu at,zero,count`), each entry taken by reference before its
 * item is fetched, the entry indexed through operator[](unsigned) with an int counter (so not
 * strength-reduced) */
bool CVehicleRdr::Open(const char *file)
{
    _zrdr *rdr = zrdr_read(file, 0, 0);
    _zrdr *list = rdr ? item(rdr, 0) : 0;
    if (list) {
        int n = list->Count();

        func_00534320(this, n);
        for (int i = 0; i < n; i++) {
            CVehicleRdrEntry &e = (*this)[i];

            e.Load(item(list, i));
        }
    }
    zrdr_free((CRdrFile *)rdr);
    return list != 0;
}

/* every entry forgotten and the storage given up (SOCOM 1's: a clear, then FreeVector's four
 * pieces out of line) */
void CVehicleRdr::Close()
{
    clear();
    FreeVector<std::vector<CVehicleRdrEntry> >(*this);
}

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

/* a random index up to max (the name ours: SOCOM 1's Randomize had the same `n - 1` and
 * modulo, its zero arm first) */
static inline int RandomIndex(int max)
{
    int i;

    if (max == 0)
        i = 0;
    else
        i = rand() % (max + 1);
    return i;
}

/* the start, and the respawn start, picked at random, or the first when asked to reset or when
 * there are none (SOCOM 1's picked the one start).
 * OPEN (0x140 against 0x150): label padding only -- the target pads the random pick's block and
 * the store after its mfhi to 8 bytes (a nop each, both halves); the zero arm first, a ternary,
 * an early return, the pick written out, a non-inline helper and the tests inverted all compile
 * without the four nops */
void CVehicleRdrEntry::Randomize(bool reset)
{
    if (reset || !m_start.m_count)
        m_startIndex = 0;
    else
        m_startIndex = RandomIndex(m_start.m_count - 1);
    if (reset || !m_respawn.m_count)
        m_respawnIndex = 0;
    else
        m_respawnIndex = RandomIndex(m_respawn.m_count - 1);
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
