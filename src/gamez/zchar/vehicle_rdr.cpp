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
#include "../../ai/ai.h"
#include "../../ai/ai_maps.h"

class CRdrFile;

_zrdr *zrdr_read(const char *name, const char *file, int line);
int zrdr_free(CRdrFile *file);
/* zrdr.h's readers (its _zrdr is zanim.h's, so the header itself stays out) */
char *zrdr_findstring(const _zrdr *rdr, const char *tag);
_zrdr *zrdr_findtag(const _zrdr *rdr, const char *tag);
bool zrdr_findbool(const _zrdr *rdr, const char *tag, bool *out);
extern "C" float cosf(float);

/* an array's item, or none: an unsigned index (zanim.h's Item takes an int, which lets MW
 * strength-reduce Open's loop -- the target recomputes &array[i] each pass; the name is ours) */
static inline _zrdr *item(_zrdr *r, unsigned int i)
{
    return i < r->Count() ? &r->array[i] : 0;
}

/* the starts: the array's count kept to four (a `_saturate<unsigned int>` with its two literals in
 * the function's constant pool, 0x64F9A8/0x64F9B0), though every item of the array is read -- each
 * a direction (item 1, a number) and a location's name (item 0, a string), the name copied or
 * emptied (new in SOCOM II: SOCOM 1's CVehicleRdr::Load read its one set in place) */
void startvec::Load(_zrdr *rdr)
{
    if (rdr) {
        int n = rdr->Count();

        m_count = n;
        _saturate(&m_count, 0u, 4u);
        for (int i = 0; i < n; i++) {
            _zrdr *e = item(rdr, i);
            int dir = item(e, 1)->Int();
            const char *name = item(e, 0)->String();
            start &s = m_start[(unsigned int)i];

            if (name)
                strncpy(s.m_name, name, sizeof(s.m_name));
            else
                s.m_name[0] = 0;
            s.m_dir = dir;
        }
    }
}

/* a team block: its numbers as a mask, or the first number out of range (new in SOCOM II, the name
 * ours: the same code as fts_player.cpp's ParseTeamBlock (OVL1 0x2B34A0), a second global function
 * -- the turret and vehicle definitions' parsers, CTRegion::Parse and OVL1's 0x2B4F40 call this one,
 * only CTInView::Parse the other). Short enough for -inline auto only with the item's number through
 * _zrdr::Int(): CVehicleRdrEntry::Load inlines it whole (the written-out type switch is over the
 * 15 statements) */
unsigned int ParseTeams(_zrdr *rdr)
{
    unsigned int mask = 0;

    if (!rdr || rdr->type != 4)
        return 0;
    for (unsigned int i = 0; i < rdr->Count(); i++) {
        int team = rdr->Item(i)->Int();

        if (team < 0 || team > 31) {
            mask = team;
            break;
        }
        mask |= 1 << team;
    }
    return mask;
}

/* SOCOM 1's Merge (its demo's 0x1A3930, before Parse as here: 0x533DB0): a field set in the first
 * set's mask comes from it, else from the second when its mask has it, else from the first -- each
 * an `||` (two loads: the first set's arm shared), the respawn count, an int, a nested ternary (its
 * three loads) */
void AI_PARAMS::Merge(const AI_PARAMS &a, const AI_PARAMS &b)
{
    accuracy = (a.mask & 0x1) || !(b.mask & 0x1) ? a.accuracy : b.accuracy;
    courage = (a.mask & 0x2) || !(b.mask & 0x2) ? a.courage : b.courage;
    surrender = (a.mask & 0x4) || !(b.mask & 0x4) ? a.surrender : b.surrender;
    stealth = (a.mask & 0x8) || !(b.mask & 0x8) ? a.stealth : b.stealth;
    visual_range = (a.mask & 0x20) || !(b.mask & 0x20) ? a.visual_range : b.visual_range;
    visual_fov = (a.mask & 0x40) || !(b.mask & 0x40) ? a.visual_fov : b.visual_fov;
    visual_sens = (a.mask & 0x10) || !(b.mask & 0x10) ? a.visual_sens : b.visual_sens;
    aware_decay = (a.mask & 0x80) || !(b.mask & 0x80) ? a.aware_decay : b.aware_decay;
    training = (a.mask & 0x100) || !(b.mask & 0x100) ? a.training : b.training;
    thip_range = (a.mask & 0x200) || !(b.mask & 0x200) ? a.thip_range : b.thip_range;
    respawn = (a.mask & 0x400) ? a.respawn : (b.mask & 0x400) ? b.respawn : a.respawn;
    target_delay = (a.mask & 0x800) || !(b.mask & 0x800) ? a.target_delay : b.target_delay;
    target_discipline = (a.mask & 0x1000) || !(b.mask & 0x1000) ? a.target_discipline : b.target_discipline;
    mask = a.mask | b.mask;
}

/* the aiming cone's cosine, blended from 10 degrees to 2 by the discipline (the cosines guarded
 * function-local statics: cosf is no constant) */
float AI_PARAMS::GetAimConeCos() const
{
    static float wide = cosf(0.17453292f);
    static float narrow = cosf(0.034906585f);

    return wide * (1.0f - target_discipline) + narrow * target_discipline;
}

extern "C" int strcasecmp(const char *, const char *);

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

        resize(n);
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

/* a draw below n, an inline of its own: its exit ends the pick's arm, the label MW pads to 8 bytes
 * after the `mfhi` (the target's nop) -- the draw written in the arm leaves it unpadded (the AI core
 * round 22, as CAiMaps::GetRandomLoc's; the name ours) */
static inline int RandomDraw(int n)
{
    return rand() % n;
}

/* a random index up to max (the name ours: SOCOM 1's Randomize had the same `n - 1` and
 * modulo, its zero arm first) */
static inline int RandomIndex(int max)
{
    int i;

    if (max == 0)
        i = 0;
    else
        i = RandomDraw(max + 1);
    return i;
}

/* the entry kinds' names, by kind (OVL2 0x64F9C0: "Undefined", then the two tags an entry's
 * character can be named by, then "Max"; the name ours) -- Load keeps the kind found at 0x70 */
static const char *s_entryTypeNames[] = {"Undefined", "character", "vehicle", "Max"};

/* one entry out of the file's item (SOCOM 1's CVehicleRdr::Load did every entry in place; SOCOM II's
 * Open hands each item here): the names and the teams found first, the six flags read, the AI's
 * parameters parsed, the names copied or emptied, the character's name by the first tag present,
 * then the team mask, the flags (byte read-mask-writes) and both start sets. ParseTeams and
 * SetCharacterName inline; the found test written `!= 0` branches (a bare test materialised) */
void CVehicleRdrEntry::Load(_zrdr *rdr)
{
    bool debug = false;
    bool disabled = false;
    bool nofade = false;
    bool noshoot = false;
    bool recycle = false;
    bool nosnooze = false;
    const char *name = zrdr_findstring(rdr, "name");
    const char *display = zrdr_findstring(rdr, "display");
    unsigned int teams = ParseTeams(zrdr_findtag(rdr, "team"));

    zrdr_findbool(rdr, "debug", &debug);
    zrdr_findbool(rdr, "disabled", &disabled);
    zrdr_findbool(rdr, "nofade", &nofade);
    zrdr_findbool(rdr, "noshoot", &noshoot);
    zrdr_findbool(rdr, "recycle", &recycle);
    zrdr_findbool(rdr, "nosnooze", &nosnooze);
    m_aiParams.Parse(zrdr_findtag(rdr, "ai_params"));
    if (name)
        strncpy(m_name, name, sizeof(m_name));
    else
        m_name[0] = 0;
    if (display)
        strncpy(m_display_name, display, sizeof(m_display_name));
    else
        m_display_name[0] = 0;
    for (int i = 1; i < 3; i++) {
        const char *ch = zrdr_findstring(rdr, s_entryTypeNames[i]);

        if (ch != 0) {
            SetCharacterName(ch);
            m_unk70 = i;
            break;
        }
    }
    m_teamMask = teams;
    m_debug = debug;
    m_disabled = disabled;
    m_nofade = nofade;
    m_noshoot = noshoot;
    m_recycle = recycle;
    m_nosnooze = nosnooze;
    m_start.Load(zrdr_findtag(rdr, "setup"));
    m_respawn.Load(zrdr_findtag(rdr, "respawn_setup"));
}

/* an empty entry: the byte at 0x70, both start sets, the team mask, the AI's defaults and the flags
 * built in declaration order, then the three names emptied -- the name, the display name, the
 * character (SOCOM 1's had no constructor of its own; out of line, the resize's fill value) */
CVehicleRdrEntry::CVehicleRdrEntry()
    : m_unk70(0), m_teamMask(0), m_debug(0), m_disabled(0), m_nofade(0), m_noshoot(0), m_recycle(0),
      m_nosnooze(0), m_startIndex(0)
{
    m_name[0] = 0;
    m_display_name[0] = 0;
    m_character[0] = 0;
}

/* the start, and the respawn start, picked at random, or the first when asked to reset or when
 * there are none (SOCOM 1's picked the one start). Byte-exact (the AI core round 22) once the draw
 * went through RandomDraw: the four padding nops the target has after each pick's block and its
 * `mfhi` -- the zero arm first, a ternary, an early return, the pick written out, a non-inline
 * helper and the tests inverted had all compiled without them */
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

/* a matrix's translation set from three values (the name is ours): taken by value, they are read
 * right to left before the stores, as the target's last block loads z, y, x and then stores x, y, z */
static inline void SetTranslation(CMatrix &m, float x, float y, float z)
{
    m.m_matrix[3][0] = x;
    m.m_matrix[3][1] = y;
    m.m_matrix[3][2] = z;
}

/* the start's place as a matrix (SOCOM 1's): the start picked -- at random for -1, Randomize
 * inlined -- then its name and direction, its location, its cell's centre, the direction's rotation
 * with the centre as the translation; false at the first step that fails.
 * Byte-exact since the AI core's round 23 (66 instructions from round 22, every saved register one
 * off): the start read through startvec::Get, whose bool is the caller's `ok` only from the join on
 * (MW's colouring had a function-wide ok, `false` at the top, take s0 first and push every other
 * saved register up one; the target's pick count in s4 follows), and the translation through
 * SetTranslation */
bool CVehicleRdrEntry::GetStartMatrix(CMatrix &m, unsigned int index)
{
    if (index == (unsigned int)-1)
        Randomize(false);
    else
        m_startIndex = index;
    const char *name;
    int dir;
    bool ok = m_start.Get(m_startIndex, name, dir);

    if (ok) {
        CAiMapLoc loc;

        ok = AiMaps.GetLocation(name, loc);
        if (ok) {
            CPnt3D pos;

            ok = AiMaps.GetCellCenter(loc, &pos) != 0;
            if (ok) {
                CQuat q;

                CAiMap::GetDirQuat((CAiMap::DIR)dir, q, true);
                q.ToMatrix(&m);
                SetTranslation(m, pos.x, pos.y, pos.z);
            }
        }
    }
    return ok;
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
