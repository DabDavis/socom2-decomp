/* zsnd_positions.cpp -- SOCOM II's record of the 3D sounds played at fixed positions: every record
 * kept, the ones in hearing range, and those dropped by the two-second update (OVL1
 * 0x34BD40-0x34C55F; its static initialiser, 0x403370, builds the three vectors and registers
 * their destructor 0x34C4F0). Not in SOCOM 1; every name here is ours. Defined in reverse of their
 * image order (-inline deferred). The update (0x34BEC0) is not written yet. Round 18: at the
 * flags' -inline level=5, round 24's `#pragma inline_depth(6)` stand-in dropped (the add's walk
 * ends at Last(): the object byte-identical). */

#include "zsnd.h"

/* a sound played at a position: the position, the sound and the voice playing it (0x34C2F0 makes
 * them, 0x14 bytes) */
struct CSndPosition {
    PNT3D m_pos;                /* 0x00 */
    CSnd *m_sound;              /* 0x0C */
    CSndInstance *m_instance;   /* 0x10 */
};

/* the unit's statics, in its initialiser's order (0x403370: each vector's three words zeroed, then
 * its destructor 0x34C4F0 registered) */
std::vector<CSndPosition *> data_0049E1F0;  /* the records in hearing range */
std::vector<CSndPosition *> data_0049E210;  /* those the update drops */
std::vector<CSndPosition *> data_0049E230;  /* every record */
extern float data_0049E240;                         /* the update's countdown */

extern "C" void func_0034BEC0(float dT);

/* a pointer vector's end built straight from its storage (ours): a walk testing `i != end()`
 * inside an inline is six deep where its caller expands it (include/vector's end() four high since
 * the MSL headers round 24), so at -inline level=5 MW keeps end()'s body and calls the raw end and
 * __iter; through this end the walk inlines whole at 5, as the target (the online lobby's round 20
 * Last(), the same refit). The raw end held in a const local, as end() holds it: written straight
 * into the iterator (`iterator(data_ + size_)`, the lobby's spelling) the walk's cursor and end
 * swap a1/a0 (7 off) -- the const local numbers the end after the cursor */
static inline std::vector<CSndPosition *>::iterator Last(std::vector<CSndPosition *> &v)
{
    unsigned int *const p = v.data_ + v.size_;

    return std::vector<CSndPosition *>::iterator(p);
}

/* whether a record holds this sound at this position (ours: the add inlines it) */
static inline bool PositionKnown(CSnd *sound, const CPnt3D *pos)
{
    for (std::vector<CSndPosition *>::iterator i = data_0049E230.begin(); i != Last(data_0049E230); ++i) {
        CSndPosition *p = *i;

        if (p->m_sound == sound && *pos == *(const CPnt3D *)&p->m_pos)
            return true;
    }
    return false;
}

/* a voice's sound and position recorded, once, and the update run at once (CSnd::PlaySound3D
 * calls it) */
extern "C" void func_0034C2F0(CSndInstance *inst)
{
    CSnd *sound = inst->m_parent;
    const CPnt3D *pos = &inst->m_pos;

    if (!PositionKnown(sound, pos)) {
        CSndPosition *p;

        (p = new CSndPosition)->m_instance = inst;
        p->m_pos = *(const PNT3D *)&inst->m_pos;
        p->m_sound = sound;
        data_0049E230.push_back(p);
        CSndPosition *q = p;

        data_0049E1F0.push_back(q);
        func_0034BEC0(2.0f);
    }
}

/* the vectors' room made and the countdown zeroed (CSnd::Init calls it) */
extern "C" void func_0034C4A0()
{
    data_0049E1F0.reserve(20);
    data_0049E210.reserve(20);
    data_0049E230.reserve(60);
    data_0049E240 = 0;
}

/* every record deleted, the three vectors emptied (CSnd::Close and UnloadAllSounds call it) */
extern "C" void func_0034BE10()
{
    for (std::vector<CSndPosition *>::iterator i = data_0049E230.begin(); i != data_0049E230.end(); ++i)
        if (*i)
            delete *i;
    data_0049E1F0.clear();
    data_0049E210.clear();
    data_0049E230.clear();
}

/* two empty walks, the records in range and every record (a debug print compiled out; the update
 * calls it) */
extern "C" void func_0034BD40()
{
    for (std::vector<CSndPosition *>::iterator i = data_0049E1F0.begin(); i != data_0049E1F0.end(); i++)
        ;
    for (std::vector<CSndPosition *>::iterator i = data_0049E230.begin(); i != data_0049E230.end(); i++)
        ;
}
