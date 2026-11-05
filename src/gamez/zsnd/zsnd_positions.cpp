/* zsnd_positions.cpp -- SOCOM II's record of the 3D sounds played at fixed positions: every record
 * kept, the ones in hearing range, and those dropped by the two-second update (OVL1
 * 0x34BD40-0x34C55F; its static initialiser, 0x403370, builds the three vectors and registers
 * their destructor 0x34C4F0). Not in SOCOM 1; every name here is ours. Defined in reverse of their
 * image order (-inline deferred). The update (0x34BEC0) and the add (0x34C2F0, CSnd::PlaySound3D
 * calls it) are not written yet. */

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

/* whether a record holds this sound at this position (ours: the add inlines it) */
static inline bool PositionKnown(CSnd *sound, const CPnt3D *pos)
{
    for (std::vector<CSndPosition *>::iterator i = data_0049E230.begin(); i != data_0049E230.end(); ++i) {
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

/* THE UNIT INLINES TO DEPTH 6, not the flags' 5 (a pragma acts through its state at the unit's
 * end; the MSL headers round 24). include/vector's pointer iterator is the image's since that
 * round -- end() four high: the raw end, the pod's end, data() -- and a walk testing `i != end()`
 * inside an inline is then six deep where its caller inlines it: at 5 MW keeps end()'s body and
 * calls the raw end and __iter. At 6 0x34C2F0 inlines the position list's walks whole, as the
 * target, with the source as it was written against the three-high end, and every function of the
 * unit scores as it did at 5 (measured over the unit). The end taken into a local first (round
 * 23's suggested refit) does not give the target: in fts_missionlist.cpp Find's own copy then
 * colours its end, flag and element differently (9 off) and in util_loc.cpp FindTable's callers
 * come out 12 bytes short. So the original's walks are the ones written here and its level deeper
 * than 5 (README "Level 6, or 8?")
 */
#pragma inline_depth(6)
