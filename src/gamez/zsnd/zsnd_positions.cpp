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

extern std::vector<CSndPosition *> data_0049E1F0;  /* the records in hearing range */
extern std::vector<CSndPosition *> data_0049E210;  /* those the update drops */
extern std::vector<CSndPosition *> data_0049E230;  /* every record */
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
